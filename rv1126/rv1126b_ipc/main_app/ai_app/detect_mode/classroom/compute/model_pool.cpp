/**
 * @FilePath     : model_pool.cpp
 * @Author       : leiyy
 * @Date         : 2026-09-29 16:28:37
 * @LastEditors  : leiyy
 * @LastEditTime : 2026-09-29 16:28:37
 * @Description  : 模型池实现（驻留 / 时间片轮转 / 空闲卸载 / 内存准入）
 */

#include "model_pool.h"

#include <pthread.h>
#include <unistd.h>

#include <algorithm>
#include <chrono>

#include "classroom_clock.h"
#include "dlog.h"

namespace Classroom
{
namespace
{
/* 重资源组*/
bool isHeavyGroup(ModelGroup_E enGroup)
{
    return enGroup == ModelGroup_E::GROUP_POSE || enGroup == ModelGroup_E::GROUP_FACE;
}

ModelGroup_E oppositeHeavyGroup(ModelGroup_E enGroup)
{
    if (enGroup == ModelGroup_E::GROUP_POSE)
    {
        return ModelGroup_E::GROUP_FACE;
    }
    if (enGroup == ModelGroup_E::GROUP_FACE)
    {
        return ModelGroup_E::GROUP_POSE;
    }
    return ModelGroup_E::GROUP_OTHER;
}

int64_t sliceOfGroup(ModelGroup_E enGroup, const CModelPoolParam_S &stParam)
{
    if (enGroup == ModelGroup_E::GROUP_POSE)
    {
        return stParam.llSlicePoseMs;
    }
    if (enGroup == ModelGroup_E::GROUP_FACE)
    {
        return stParam.llSliceFaceMs;
    }
    return 0;
}

/* 驻留量未知的模型按此保守值记账*/
int64_t residentOf(const CModelDesc_S &stDesc)
{
    return (stDesc.llResidentKb > 0) ? stDesc.llResidentKb : CLASSROOM_MODEL_ASSUME_RESIDENT_KB;
}
}

IModelHandle::~IModelHandle()
{
}

/* ================================= CModelLease ================================= */

CModelLease::CModelLease(ModelPtr spHandle, std::shared_ptr<Ctrl> spCtrl)
    : m_spHandle(std::move(spHandle))
    , m_spCtrl(std::move(spCtrl))
{
}

CModelLease::~CModelLease()
{
    if (m_spCtrl)
    {
        if (m_spCtrl->nUsers > 0)
        {
            --m_spCtrl->nUsers;
        }
        m_spCtrl->llLastUsedMs = classroomNowMs();
    }
}

CModelLease::CModelLease(CModelLease &&stOther)
    : m_spHandle(std::move(stOther.m_spHandle))
    , m_spCtrl(std::move(stOther.m_spCtrl))
{
    stOther.m_spCtrl = nullptr;    /* 移交所有权，避免析构时重复减计数 */
}

CModelLease &CModelLease::operator=(CModelLease &&stOther)
{
    if (this != &stOther)
    {
        CModelLease stOwn(std::move(*this));

        m_spHandle = std::move(stOther.m_spHandle);
        m_spCtrl = std::move(stOther.m_spCtrl);
        stOther.m_spCtrl = nullptr; 
    }
    return *this;
}

/* ================================= CModelPool ================================= */

CModelPool::~CModelPool()
{
    stop();
}

void CModelPool::setLoader(ModelLoaderFn fnLoader)
{
    std::lock_guard<std::mutex> stLock(m_mtx);
    m_fnLoader = std::move(fnLoader);
}

void CModelPool::setParam(const CModelPoolParam_S &stParam)
{
    std::lock_guard<std::mutex> stLock(m_mtx);
    m_stParam = stParam;

    if (m_stParam.llIdleUnloadMs < 0)
    {
        m_stParam.llIdleUnloadMs = CLASSROOM_MODEL_IDLE_UNLOAD_MS;
    }
    if (m_stParam.llReserveKb < 0)
    {
        m_stParam.llReserveKb = 0;
    }
    if (m_stParam.llLoadFailCooldownMs < 0)
    {
        m_stParam.llLoadFailCooldownMs = CLASSROOM_MODEL_LOAD_FAIL_COOLDOWN_MS;
    }
    /* 时间片为 0 会让两组在每次 acquire 时来回卸载重载（load 是百 ms 级），必须留下限 */
    if (m_stParam.llSlicePoseMs < 1000)
    {
        m_stParam.llSlicePoseMs = 1000;
    }
    if (m_stParam.llSliceFaceMs < 1000)
    {
        m_stParam.llSliceFaceMs = 1000;
    }
}

CModelPoolParam_S CModelPool::param() const
{
    std::lock_guard<std::mutex> stLock(m_mtx);
    return m_stParam;
}

bool CModelPool::registerModel(const CModelDesc_S &stDesc)
{
    if (stDesc.strKey.empty())
    {
        dlog_error("model_pool: 注册请求的模型 key 为空");
        return false;
    }

    std::lock_guard<std::mutex> stLock(m_mtx);
    if (m_mapEntries.size() >= CLASSROOM_MAX_MODELS &&
        m_mapEntries.find(stDesc.strKey) == m_mapEntries.end())
    {
        dlog_error("model_pool: 模型总数超过上限 %d，拒绝注册 %s",
                   CLASSROOM_MAX_MODELS, stDesc.strKey.c_str());
        return false;
    }

    auto it = m_mapEntries.find(stDesc.strKey);
    if (it != m_mapEntries.end())
    {
        if (it->second.spHandle)
        {
            /* 已驻留时改描述会让预算记账与实际不符，必须拒绝而不是静默覆盖 */
            dlog_error("model_pool: %s 已驻留，拒绝重复注册", stDesc.strKey.c_str());
            return false;
        }
        dlog_warn("model_pool: %s 重复注册，描述已覆盖", stDesc.strKey.c_str());
        it->second.stDesc = stDesc;
        it->second.enState = BackendState::UNLOADED;
        /* 换了描述＝换了一个要装着试的东西，旧描述留下的失败证据（30s 冷却窗）不再适用 */
        it->second.llLastLoadFailMs = 0;
        it->second.enLoggedState = BackendState::DISABLED;
        return true;
    }

    Entry stEntry;
    stEntry.stDesc = stDesc;
    stEntry.spCtrl = std::make_shared<CModelLease::Ctrl>();
    stEntry.spCtrl->llLastUsedMs = classroomNowMs();
    m_mapEntries.emplace(stDesc.strKey, std::move(stEntry));
    return true;
}

bool CModelPool::start()
{
    std::lock_guard<std::mutex> stLock(m_mtx);

    if (!m_stParam.bEvictThread)
    {
        return true;
    }
    if (m_bRunning)
    {
        return true;
    }
    m_bRunning = true;
    m_thEvict = std::thread(&CModelPool::evictLoop, this);
    return true;
}

void CModelPool::stop()
{
    {
        std::lock_guard<std::mutex> stLock(m_mtx);
        if (!m_bRunning)
        {
            return;
        }
        m_bRunning = false;
    }
    m_cvLoaded.notify_all();
    if (m_thEvict.joinable())
    {
        m_thEvict.join();
    }
}

CModelLease CModelPool::acquire(const std::string &strKey, BackendState_E &renState, int64_t llDeadlineMs)
{
    if (llDeadlineMs > 0 && llDeadlineMs < classroomNowMs())
    {
        renState = BackendState::LOAD_FAILED;
        return CModelLease();
    }

    std::unique_lock<std::mutex> stLock(m_mtx);
    auto it = m_mapEntries.find(strKey);
    if (it == m_mapEntries.end())
    {
        renState = BackendState::DISABLED;    /* 未注册=本型号没有该功能，与「模型丢了」区分开 */
        return CModelLease();
    }

    Entry &stEntry = it->second;

    for (;;)
    {
        if (stEntry.spHandle)
        {
            stEntry.spCtrl->nUsers++;
            stEntry.spCtrl->llLastUsedMs = classroomNowMs();
            renState = BackendState::READY;
            return CModelLease(stEntry.spHandle, stEntry.spCtrl);
        }

        if (stEntry.bLoading)
        {
            const int64_t llLeft = (0 == llDeadlineMs) ? 0 : (llDeadlineMs - classroomNowMs());
            if (llLeft <= 0 ||
                !m_cvLoaded.wait_for(stLock, std::chrono::milliseconds(llLeft),
                                     [&stEntry] { return !stEntry.bLoading; }))
            {
                renState = BackendState::LOAD_FAILED;    /* 没等到结果：上层按「证据不足」出 unknown */
                return CModelLease();
            }
            continue;
        }

        /* 需要由本线程加载：先在锁外做判定，任何一条不过就直接返回，不留状态残留 */
        if (0 != access(stEntry.stDesc.strModelPath.c_str(), F_OK))
        {
            stEntry.enState = BackendState::MODEL_MISSING;
            ++m_stStat.u64RejectMissing;
            renState = stEntry.enState;
            if (logIfChanged(stEntry, renState))
            {
                dlog_warn("model_pool: %s 模型文件缺失: %s", strKey.c_str(),
                          stEntry.stDesc.strModelPath.c_str());
            }
            return CModelLease();
        }
        if (!m_fnLoader)
        {
            stEntry.enState = BackendState::LOAD_FAILED;
            renState = stEntry.enState;
            if (logIfChanged(stEntry, renState))
            {
                dlog_warn("model_pool: %s 未注入加载器", strKey.c_str());
            }
            return CModelLease();
        }

        /* 加载失败冷却：上次失败距今不足冷却窗时不再尝试加载 */
        if (stEntry.llLastLoadFailMs > 0 && m_stParam.llLoadFailCooldownMs > 0 &&
            classroomNowMs() - stEntry.llLastLoadFailMs < m_stParam.llLoadFailCooldownMs)
        {
            stEntry.enState = BackendState::LOAD_FAILED;
            ++m_stStat.u64LoadFailCooling;
            renState = stEntry.enState;
            if (logIfChanged(stEntry, renState))
            {
                dlog_warn("model_pool: %s 上次加载失败，%.1fs 内不重试（冷却中）",
                          strKey.c_str(),
                          static_cast<double>(m_stParam.llLoadFailCooldownMs) / 1000.0);
            }
            return CModelLease();
        }

        const int64_t llNeedKb = residentOf(stEntry.stDesc);
        const CMemInfo_S stMem = classroomQueryMem();

        if (!canAdmit(stMem, m_llResidentKb, stEntry.stDesc, m_stParam.llReserveKb))
        {
            if (!makeRoomLocked(stEntry, classroomNowMs()))
            {
                stEntry.enState = BackendState::GROUP_BUSY;
                ++m_stStat.u64RejectSlice;
                renState = stEntry.enState;
                if (logIfChanged(stEntry, renState))
                {
                    dlog_warn("model_pool: %s 被拒，对侧组仍在时间片内", strKey.c_str());
                }
                return CModelLease();
            }
            /* 腾完之后按同一份内存读数再判一次：这里变的是驻留账，不是整机可用量 */
            if (!canAdmit(stMem, m_llResidentKb, stEntry.stDesc, m_stParam.llReserveKb))
            {
                stEntry.enState = BackendState::OUT_OF_BUDGET;
                ++m_stStat.u64RejectBudget;
                renState = stEntry.enState;

                if (logIfChanged(stEntry, renState))
                {
                    dlog_warn("model_pool: %s 因预算被拒, need=%lldKB resident=%lldKB avail=%lldKB mmz=%lldKB",
                              strKey.c_str(), static_cast<long long>(llNeedKb),
                              static_cast<long long>(m_llResidentKb),
                              static_cast<long long>(stMem.llMemAvailableKb),
                              static_cast<long long>(stMem.llMmzUsedKb));
                }
                return CModelLease();
            }
        }

        stEntry.bLoading = true;
        const Entry stSnapshot = stEntry;    /* loader 只能用描述字段，句柄/状态此刻不该被读 */
        stLock.unlock();
        {
            /* 全局只允许一个加载在飞：瞬时内存峰值与 NPU 争抢都在这里被挡住 */
            std::lock_guard<std::mutex> stLoadLock(m_mtxLoad);
            int64_t llCostMs = 0;
            ModelPtr spHandle = loadViaLoader(stSnapshot, llCostMs);
            stLock.lock();
            stEntry.bLoading = false;

            const int64_t llNow = classroomNowMs();
            if (!spHandle)
            {
                stEntry.enState = BackendState::LOAD_FAILED;
                ++m_stStat.u64LoadFail;
                stEntry.llLastLoadFailMs = llNow;    /* 冷却窗起点；成功路径清零 */
                renState = stEntry.enState;
                m_cvLoaded.notify_all();
                if (logIfChanged(stEntry, renState))
                {
                    dlog_error("model_pool: %s 加载失败，耗时 %lldms", strKey.c_str(),
                               static_cast<long long>(llCostMs));
                }
                return CModelLease();
            }

            /* 在飞期间描述被热更替换 */
            if (!stEntry.stDesc.sameAs(stSnapshot.stDesc))
            {
                ++m_stStat.u64LoadDiscard;
                stEntry.enState = BackendState::UNLOADED;
                renState = BackendState::LOAD_FAILED;
                m_cvLoaded.notify_all();
                dlog_warn("model_pool: %s 加载完成时描述已被热更替换，丢弃本次结果, "
                          "下拍按新描述重装 (%s -> %s)",
                          strKey.c_str(), stSnapshot.stDesc.strModelPath.c_str(),
                          stEntry.stDesc.strModelPath.c_str());
                return CModelLease();
            }

            const int64_t llRealKb = std::max(spHandle->residentKb(), residentOf(stSnapshot.stDesc));
            stEntry.spHandle = spHandle;
            /* 不做 nUsers=0 之类的清零：能走到这里说明驻留曾被放掉，而放掉的前提是 nUsers==0 */
            stEntry.spCtrl->llLastUsedMs = llNow;
            stEntry.llLastLoadFailMs = 0;        /* 加载成功：失败账一笔勾销，不再冷却 */
            stEntry.enState = BackendState::READY;
            logIfChanged(stEntry, BackendState::READY);
            stEntry.llChargedKb = llRealKb;
            m_llResidentKb += llRealKb;
            m_stStat.u64LoadOk++;
            m_stStat.llResidentKb = m_llResidentKb;
            m_stStat.nResidentNum++;
            m_stStat.llLastLoadMs = llCostMs;

            /* 时间片从「该组首次驻留」起算，卸载后清零，下一轮重新给满一片 */
            if (isHeavyGroup(stSnapshot.stDesc.enGroup) && 0 == stEntry.llSliceUntilMs)
            {
                stEntry.llSliceUntilMs = llNow + sliceOfGroup(stSnapshot.stDesc.enGroup, m_stParam);
            }
            renState = BackendState::READY;
            m_cvLoaded.notify_all();

            stEntry.spCtrl->nUsers++;
            dlog_info("model_pool: %s 加载完成 耗时=%lldms 本组驻留=%lldKB 进程合计=%lldKB",
                      strKey.c_str(), static_cast<long long>(llCostMs),
                      static_cast<long long>(llRealKb), static_cast<long long>(m_llResidentKb));
            return CModelLease(stEntry.spHandle, stEntry.spCtrl);
        }
    }
}

BackendState_E CModelPool::stateOf(const std::string &strKey) const
{
    std::lock_guard<std::mutex> stLock(m_mtx);
    auto it = m_mapEntries.find(strKey);
    return (it == m_mapEntries.end()) ? BackendState::DISABLED : it->second.enState;
}

bool CModelPool::descOf(const std::string &strKey, CModelDesc_S &stDesc) const
{
    std::lock_guard<std::mutex> stLock(m_mtx);
    auto it = m_mapEntries.find(strKey);
    if (it == m_mapEntries.end())
    {
        return false;
    }
    stDesc = it->second.stDesc;
    return true;
}

bool CModelPool::peek(const std::string &strKey, const std::function<void(IModelHandle &)> &fnHandle)
{
    if (!fnHandle)
    {
        return false;
    }

    std::lock_guard<std::mutex> stLock(m_mtx);
    auto it = m_mapEntries.find(strKey);
    if (it == m_mapEntries.end() || !it->second.spHandle)
    {
        return false;
    }

    fnHandle(*it->second.spHandle);
    return true;
}

void CModelPool::releaseLocked(Entry &stEntry)
{
    stEntry.spHandle.reset();
    stEntry.llSliceUntilMs = 0;
    stEntry.enState = BackendState::UNLOADED;   /* 没驻留 ≠ 文件不在 */

    m_llResidentKb -= stEntry.llChargedKb;
    stEntry.llChargedKb = 0;
    if (m_llResidentKb < 0)
    {
        m_llResidentKb = 0;
    }
    m_stStat.llResidentKb = m_llResidentKb;
    if (m_stStat.nResidentNum > 0)
    {
        m_stStat.nResidentNum--;
    }
    ++m_stStat.u64Unload;
}

int32_t CModelPool::unloadGroup(ModelGroup_E enGroup)
{
    int32_t nUnloaded = 0;
    std::lock_guard<std::mutex> stLock(m_mtx);

    for (auto &pair : m_mapEntries)
    {
        Entry &stEntry = pair.second;
        if (stEntry.stDesc.enGroup != enGroup || !stEntry.spHandle)
        {
            continue;
        }
        if (stEntry.spCtrl->nUsers > 0)
        {
            /* 有人正在用：本次不卸，等它归还后由巡检线程处理，绝不把句柄从推理线程脚下抽走 */
            continue;
        }
        releaseLocked(stEntry);
        ++nUnloaded;
    }

    if (nUnloaded > 0)
    {
        dlog_info("model_pool: 组 %d 已卸载 %d 个模型，当前常驻 %lldKB",
                  static_cast<int>(enGroup), nUnloaded, static_cast<long long>(m_llResidentKb));
    }
    return nUnloaded;
}

bool CModelPool::unloadKey(const std::string &strKey)
{
    std::lock_guard<std::mutex> stLock(m_mtx);

    const auto it = m_mapEntries.find(strKey);
    if (it == m_mapEntries.end() || !it->second.spHandle)
    {
        return false;
    }
    if (it->second.spCtrl->nUsers > 0)
    {
        return false;
    }
    releaseLocked(it->second);
    dlog_info("model_pool: %s 已卸载（单模型，组仍需要），当前常驻 %lldKB",
              strKey.c_str(), static_cast<long long>(m_llResidentKb));
    return true;
}

void CModelPool::unloadAll()
{
    for (int32_t nGroup = static_cast<int32_t>(ModelGroup_E::GROUP_POSE);
         nGroup <= static_cast<int32_t>(ModelGroup_E::GROUP_OTHER); ++nGroup)
    {
        unloadGroup(static_cast<ModelGroup_E>(nGroup));
    }
    unloadGroup(ModelGroup_E::GROUP_BASE);
}

int32_t CModelPool::evictIdle()
{
    const int64_t llNow = classroomNowMs();
    int32_t nEvicted = 0;
    std::lock_guard<std::mutex> stLock(m_mtx);

    for (auto &pair : m_mapEntries)
    {
        Entry &stEntry = pair.second;
        if (!stEntry.spHandle || stEntry.spCtrl->nUsers > 0)
        {
            continue;
        }
        if (llNow - stEntry.spCtrl->llLastUsedMs < m_stParam.llIdleUnloadMs)
        {
            continue;
        }

        const int64_t llIdleMs = llNow - stEntry.spCtrl->llLastUsedMs;
        dlog_info("model_pool: %s 空闲 %lldms 已超时，卸载并归还 %lldKB", stEntry.stDesc.strKey.c_str(),
                  static_cast<long long>(llIdleMs), static_cast<long long>(stEntry.llChargedKb));
        releaseLocked(stEntry);
        ++nEvicted;
    }
    return nEvicted;
}

std::vector<CModelState_S> CModelPool::snapshot() const
{
    std::lock_guard<std::mutex> stLock(m_mtx);
    std::vector<CModelState_S> vecState;
    vecState.reserve(m_mapEntries.size());

    for (const auto &pair : m_mapEntries)
    {
        const Entry &stEntry = pair.second;
        CModelState_S stItem;
        stItem.strKey = stEntry.stDesc.strKey;
        stItem.enGroup = stEntry.stDesc.enGroup;
        stItem.enState = stEntry.enState;
        stItem.bResident = (nullptr != stEntry.spHandle);
        stItem.nUsers = stEntry.spCtrl ? stEntry.spCtrl->nUsers.load() : 0;
        stItem.llResidentKb = stItem.bResident ? stEntry.llChargedKb : 0;
        vecState.push_back(stItem);
    }
    return vecState;
}

CModelPoolStat_S CModelPool::stat() const
{
    std::lock_guard<std::mutex> stLock(m_mtx);
    return m_stStat;
}

bool CModelPool::canAdmit(const CMemInfo_S &stMem, int64_t llResidentKb,
                          const CModelDesc_S &stDesc, int64_t llReserveKb)
{
    const int64_t llNeedKb = residentOf(stDesc);
    if (llNeedKb <= 0 || llResidentKb < 0)
    {
        return false;
    }

    return classroomCanAfford(stMem, llResidentKb + llNeedKb, llReserveKb);
}

bool CModelPool::makeRoomLocked(const Entry &stWant, int64_t llNowMs)
{
    if (!isHeavyGroup(stWant.stDesc.enGroup))
    {
        return true;
    }

    const ModelGroup_E enRival = oppositeHeavyGroup(stWant.stDesc.enGroup);
    for (const auto &pair : m_mapEntries)
    {
        const Entry &stOther = pair.second;
        if (stOther.stDesc.enGroup != enRival || !stOther.spHandle)
        {
            continue;
        }
        if (stOther.spCtrl->nUsers > 0)
        {
            return false;
        }
        if (0 != stOther.llSliceUntilMs && llNowMs < stOther.llSliceUntilMs)
        {
            return false;
        }
    }

    /* 判定通过后真正腾地方 */
    int32_t nGivenUp = 0;
    for (auto &pair : m_mapEntries)
    {
        Entry &stOther = pair.second;
        if (stOther.stDesc.enGroup != enRival || !stOther.spHandle || stOther.spCtrl->nUsers > 0)
        {
            continue;
        }
        releaseLocked(stOther);
        ++nGivenUp;
    }
    if (nGivenUp > 0)
    {
        ++m_stStat.u64GroupSwitch;
        dlog_info("model_pool: 时间片切换，组 %d 让出 %d 个模型给组 %d",
                  static_cast<int>(enRival), nGivenUp, static_cast<int>(stWant.stDesc.enGroup));
    }
    return true;
}

ModelPtr CModelPool::loadViaLoader(const Entry &stEntry, int64_t &llCostMs)
{
    const int64_t llBegin = classroomNowMs();
    ModelPtr spHandle;

    try
    {
        spHandle = m_fnLoader(stEntry.stDesc);
    }
    catch (const std::exception &stEx)
    {
        /* 后端加载抛异常（含 bad_alloc）不能掀翻整个进程：按加载失败降级 */
        dlog_error("model_pool: %s 的加载器抛出异常: %s", stEntry.stDesc.strKey.c_str(), stEx.what());
        spHandle.reset();
    }
    catch (...)
    {
        dlog_error("model_pool: %s 的加载器抛出未知异常", stEntry.stDesc.strKey.c_str());
        spHandle.reset();
    }

    llCostMs = classroomNowMs() - llBegin;
    return spHandle;
}

bool CModelPool::logIfChanged(Entry &stEntry, BackendState_E enNew)
{
    if (stEntry.enLoggedState == enNew)
    {
        return false;
    }
    stEntry.enLoggedState = enNew;
    return true;
}

void CModelPool::evictLoop()
{
    pthread_setname_np(pthread_self(), "ClsEvict");

    while (true)
    {
        {
            std::lock_guard<std::mutex> stLock(m_mtx);
            if (!m_bRunning)
            {
                return;
            }
        }

        evictIdle();

        /* 1s 巡检一次：卸载精度到秒即可，再快也只是白耗 CPU */
        std::unique_lock<std::mutex> stLock(m_mtx);
        m_cvLoaded.wait_for(stLock, std::chrono::milliseconds(1000),
                            [this] { return !m_bRunning; });
    }
}
}
