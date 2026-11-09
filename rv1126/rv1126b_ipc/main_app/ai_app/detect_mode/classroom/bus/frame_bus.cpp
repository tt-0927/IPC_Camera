/**
 * @FilePath     : frame_bus.cpp
 * @Author       : leiyy
 * @Date         : 2026-10-23 16:45:00
 * @LastEditors  : leiyy
 * @LastEditTime : 2026-10-23 16:45:00
 * @Description  : 固定槽位帧总线实现
 */

#include "frame_bus.h"

#include <cstring>

#include "classroom_clock.h"
#include "dlog.h"

namespace Classroom
{
namespace
{
/* NV12 整帧字节数：Y 平面 W×H + 交织 UV 平面 W×H/2 */
size_t nv12Bytes(int nW, int nH)
{
    return static_cast<size_t>(nW) * static_cast<size_t>(nH) * 3 / 2;
}
}

CFrameBus::~CFrameBus()
{
    deinit();
}

bool CFrameBus::init(int nSrcW, int nSrcH, int nDstW, int nDstH, MirrorMode_E enMirror, int nRotate)
{
    if (nSrcW <= 0 || nSrcH <= 0 || nDstW <= 0 || nDstH <= 0)
    {
        dlog_error("frame_bus init: 尺寸非法 %dx%d -> %dx%d", nSrcW, nSrcH, nDstW, nDstH);
        return false;
    }

    CFrameGeometry stGeom;
    if (!stGeom.setup(nSrcW, nSrcH, nDstW, nDstH, enMirror, nRotate) || !stGeom.selfCheck())
    {
        /* 几何自校验不过必须直接判失败：镜像/旋转配置写错会让所有朝向类判定整体判反，
           静默继续比崩掉更难查 */
        dlog_error("frame_bus init: 几何 setup/selfCheck 失败, mirror=%d rotate=%d",
                   static_cast<int>(enMirror), nRotate);
        return false;
    }

    if (0 != (nSrcW % 16))
    {
        /* 整帧按 W×H×1.5 连续 memcpy，前提是上游水平 stride 等于宽度。VPSS 的 vir_width 会向上
           对齐到 16，宽度非 16 对齐时 stride 空隙会被当成像素拷进来——画面整体斜切但不崩不报错，
           属于最难查的一类失效，故在此留一条与现象对得上的告警 */
        dlog_warn("frame_bus: src 宽度 %d 非 16 对齐，stride 缝隙会把拷出的帧斜切", nSrcW);
    }

    const size_t nFrameBytes = nv12Bytes(nSrcW, nSrcH);
    std::vector<std::shared_ptr<std::vector<uint8_t>>> apPixel;

    try
    {
        apPixel.reserve(CLASSROOM_FRAME_BUS_SLOTS);
        for (int i = 0; i < CLASSROOM_FRAME_BUS_SLOTS; ++i)
        {
            apPixel.push_back(std::make_shared<std::vector<uint8_t>>(nFrameBytes));
        }
    }
    catch (const std::bad_alloc &)
    {
        /*失败必须可观测且让上层走降级 */
        dlog_error("frame_bus init: 分配 %d x %zu 字节失败", CLASSROOM_FRAME_BUS_SLOTS, nFrameBytes);
        return false;
    }

    std::lock_guard<std::mutex> stLock(m_mtx);
    m_apPixel  = std::move(apPixel);
    m_apSlot.assign(CLASSROOM_FRAME_BUS_SLOTS, FramePtr());
    m_nSrcW    = nSrcW;
    m_nSrcH    = nSrcH;
    m_nDstW    = nDstW;
    m_nDstH    = nDstH;
    m_enMirror = enMirror;
    m_nRotate  = nRotate;
    m_nFrameBytes = nFrameBytes;
    m_stGeom   = stGeom;
    m_nLatest  = -1;
    m_stStat   = CFrameBusStat_S();
    m_stStat.u32Slots = static_cast<uint32_t>(m_apPixel.size());
    m_stStat.u32PixelBytesPerSlot = static_cast<uint32_t>(nFrameBytes);
    m_bReady   = true;

    dlog_info("frame_bus ready: 源 %dx%d 目标 %dx%d 镜像=%d 旋转=%d 槽位=%d 驻留=%zuKB",
              nSrcW, nSrcH, nDstW, nDstH, static_cast<int>(enMirror), nRotate,
              CLASSROOM_FRAME_BUS_SLOTS, nFrameBytes * CLASSROOM_FRAME_BUS_SLOTS / 1024);
    return true;
}

void CFrameBus::deinit()
{
    std::lock_guard<std::mutex> stLock(m_mtx);

    if (!m_bReady)
    {
        return;
    }
    m_bReady = false;

    /* 先放槽位句柄再放缓冲：仍在被消费者使用的槽会因 shared_ptr 而多活一会儿，
       这是刻意的——宁可短暂驻留，也不能让消费者手上的指针悬空 */
    const uint32_t u32Busy = static_cast<uint32_t>([this]() {
        uint32_t nCount = 0;
        for (const auto &spSlot : m_apSlot)
        {
            if (spSlot && spSlot.use_count() > 1)
            {
                ++nCount;
            }
        }
        return nCount;
    }());
    if (u32Busy > 0)
    {
        dlog_warn("frame_bus deinit: 仍有 %u 个槽被消费者持有", u32Busy);
    }

    m_apSlot.clear();
    m_apPixel.clear();
    m_nLatest = -1;
    m_nFrameBytes = 0;
    m_stGeom = CFrameGeometry();
    dlog_info("frame_bus 已释放");
}

bool CFrameBus::isReady() const
{
    std::lock_guard<std::mutex> stLock(m_mtx);
    return m_bReady;
}

bool CFrameBus::publish(const void *pNv12, int64_t nLen, uint64_t u64PtsUs, uint32_t u32Seq)
{
    std::lock_guard<std::mutex> stLock(m_mtx);

    if (!m_bReady)
    {
        return false;
    }
    if (nullptr == pNv12 || nLen < static_cast<int64_t>(m_nFrameBytes))
    {
        ++m_stStat.u64RejectedArg;
        /* 只在首次与每 250 次打日志，避免上游格式错误把日志刷爆（现网日志限流默认是关的） */
        if (1 == m_stStat.u64RejectedArg || 0 == m_stStat.u64RejectedArg % 250)
        {
            dlog_warn("frame_bus 丢弃非法帧: ptr=%p len=%lld 需要=%zu",
                      pNv12, static_cast<long long>(nLen), m_nFrameBytes);
        }
        return false;
    }

    const int nSlot = findFreeSlotLocked();
    if (nSlot < 0)
    {
        /* 无空闲槽这一拍本身就是"全槽在持"的唯一确定证据，必须在这里采样：
           成功发布末尾那个采样点给不出它——能被选中写入的槽在那一刻确实是空的，
           峰值最多只能数到 槽数-1。少这一句，statusJson 里 busy_max/slots 就会
           印成 "3/4" 而 drop_no_slot 一直在涨，看的人被告知"没满，别扩容"，
           而真实现象是每次发布都在丢帧 */
        updateBusyStatLocked();
        ++m_stStat.u64DroppedNoSlot;
        return false;
    }

    auto &stBuf = *m_apPixel[nSlot];
    std::memcpy(stBuf.data(), pNv12, m_nFrameBytes);

    /* 每帧一次 make_shared（百字节级）是刻意保留的：
       槽位对象复用会让消费者手上的帧内容被原地改写，产生像素级数据竞争。
       句柄分配相对每帧 3.11MB 的整帧分配可以忽略，稳态堆占用不随时间增长 */
    auto spFrame = std::make_shared<CFrame_S>();
    spFrame->spPixel = m_apPixel[nSlot];
    spFrame->pucNv12 = stBuf.data();
    spFrame->nPixelBytes = m_nFrameBytes;
    spFrame->nSrcW = m_nSrcW;
    spFrame->nSrcH = m_nSrcH;
    spFrame->u64PtsUs = u64PtsUs;
    spFrame->llMonotonicMs = classroomNowMs();
    spFrame->u32Seq = u32Seq;
    spFrame->stGeom = m_stGeom;

    /* 必须先交出局部句柄再采样：spFrame 与槽共享同一个控制块，
       带着它数就等于把"我正在发布的这一帧"算成外部持有者，busy_max 恒比真实现持数大 1。
       4 格总线上 3 个真实持有者就会读成 4——那一栏是和 slots 并排印给现场看"该不该扩容"的，
       多出来的这一格恰好把建议提前一格 */
    m_apSlot[nSlot] = std::move(spFrame);
    m_nLatest = nSlot;
    ++m_stStat.u64Published;
    updateBusyStatLocked();
    return true;
}

FramePtr CFrameBus::latest() const
{
    std::lock_guard<std::mutex> stLock(m_mtx);

    if (!m_bReady || m_nLatest < 0 || m_nLatest >= static_cast<int>(m_apSlot.size()))
    {
        return FramePtr();
    }
    /* 只加引用不做拷贝：返回值出锁后由调用方独立持有 */
    return m_apSlot[m_nLatest];
}

CFrameGeometry CFrameBus::geometry() const
{
    std::lock_guard<std::mutex> stLock(m_mtx);
    return m_stGeom;
}

GeomSwap_E CFrameBus::setupGeometry(int nSrcW, int nSrcH, int nDstW, int nDstH,
                                    MirrorMode_E enMirror, int nRotate)
{
    std::lock_guard<std::mutex> stLock(m_mtx);

    if (!m_bReady || nSrcW <= 0 || nSrcH <= 0 || nDstW <= 0 || nDstH <= 0)
    {
        return GeomSwap_E::REJECTED;
    }
    if (nSrcW == m_nSrcW && nSrcH == m_nSrcH && nDstW == m_nDstW && nDstH == m_nDstH &&
        enMirror == m_enMirror && nRotate == m_nRotate)
    {
        return GeomSwap_E::SAME;
    }
    if (nSrcW != m_nSrcW || nSrcH != m_nSrcH)
    {
        /* 源尺寸是槽缓冲的分配口径（m_nFrameBytes）也是发布长度门的口径，
           两处都改不动就放行几何 = 帧槽装不下新画面还不报错。
           上游 VPSS AI 通道按源分辨率建，本总线热换也拿不到新尺寸的像素——重启才做得到 */
        dlog_warn("frame_bus: src 尺寸变化 %dx%d -> %dx%d 需重启，缓冲保持每槽 %zu 字节",
                  m_nSrcW, m_nSrcH, nSrcW, nSrcH, m_nFrameBytes);
        return GeomSwap_E::REJECTED;
    }

    CFrameGeometry stGeom;
    if (!stGeom.setup(nSrcW, nSrcH, nDstW, nDstH, enMirror, nRotate) || !stGeom.selfCheck())
    {
        /* 与 init 同一条纪律：镜像/旋转配错会让所有朝向类判定整体判反，
           宁可拒（调用方把 stale 亮出来）也不把一枚未就绪/配反的几何换进去 */
        dlog_error("frame_bus: 热换时几何 setup/selfCheck 失败, mirror=%d rotate=%d，保留旧几何",
                   static_cast<int>(enMirror), nRotate);
        return GeomSwap_E::REJECTED;
    }

    m_nDstW = nDstW;
    m_nDstH = nDstH;
    m_enMirror = enMirror;
    m_nRotate = nRotate;
    m_stGeom = stGeom;
    /* m_nSrcW/m_nSrcH/m_nFrameBytes/m_apPixel/m_apSlot 全部不动：源没变，缓冲没变 */
    dlog_info("frame_bus: 几何已热换到画布 %dx%d (mirror=%d rotate=%d)，槽缓冲不动",
              nDstW, nDstH, static_cast<int>(enMirror), nRotate);
    return GeomSwap_E::SWAPPED;
}

CFrameBusStat_S CFrameBus::stat() const
{
    std::lock_guard<std::mutex> stLock(m_mtx);
    return m_stStat;
}

void CFrameBus::resetStat()
{
    std::lock_guard<std::mutex> stLock(m_mtx);

    const uint32_t u32Slots = m_stStat.u32Slots;
    const uint32_t u32Bytes = m_stStat.u32PixelBytesPerSlot;
    m_stStat = CFrameBusStat_S();
    m_stStat.u32Slots = u32Slots;
    m_stStat.u32PixelBytesPerSlot = u32Bytes;
}

int CFrameBus::findFreeSlotLocked() const
{
    const int nTotal = static_cast<int>(m_apSlot.size());
    if (nTotal <= 0)
    {
        return -1;
    }

    /* 从最新帧之后开始环形查找：优先复用「最老的没人用的槽」，
       让最新帧尽量活得久，消费者取到旧帧的概率降到最低 */
    const int nStart = (m_nLatest < 0) ? 0 : (m_nLatest + 1) % nTotal;
    for (int i = 0; i < nTotal; ++i)
    {
        const int nIdx = (nStart + i) % nTotal;
        const FramePtr &spSlot = m_apSlot[nIdx];
        if (!spSlot || spSlot.use_count() <= 1)
        {
            return nIdx;
        }
    }
    return -1;
}

void CFrameBus::updateBusyStatLocked()
{
    uint32_t nBusy = 0;
    for (const auto &spSlot : m_apSlot)
    {
        if (spSlot && spSlot.use_count() > 1)
        {
            ++nBusy;
        }
    }
    if (nBusy > m_stStat.u32BusySlotsMax)
    {
        m_stStat.u32BusySlotsMax = nBusy;
    }
}
}
