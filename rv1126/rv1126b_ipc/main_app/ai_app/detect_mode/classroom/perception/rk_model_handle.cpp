/**
 * @FilePath     : rk_model_handle.cpp
 * @Author       : leiyy
 * @Date         : 2026-10-19 16:34:00
 * @LastEditors  : leiyy
 * @LastEditTime : 2026-10-19 15:34:00
 * @Description  : rknn 检测模型适配器实现
 */

#include "rk_model_handle.h"

#include "Yolov5.hpp"
#include "YoloUltralytics.hpp"
#include "YoloUltralyticsPoint.hpp"

#include "classroom_mem.h"
#include "dlog.h"

namespace Classroom
{
namespace
{
/* 驻留实测的下限：小于这个量的增量基本都是 malloc 抖动，不是模型权重，
   拿它当真实驻留会让预算越算越松，不如报 0 让池子回退到配置申报值 */
constexpr int64_t LL_RESIDENT_VALID_MIN_KB = 256;

int64_t rssNowKb(void)
{
    const CMemInfo_S stMem = classroomQueryMem();
    return stMem.bValid ? stMem.llVmRssKb : 0;
}
}

CRkModelHandle::CRkModelHandle()
{
    /* 预留而不是每帧 reserve：postprocess 内部是 push_back，容量不足会当场 new */
    m_vecBox.reserve(CLASSROOM_MAX_PERSONS * 2);
    m_vecPoint.reserve(CLASSROOM_MAX_PERSONS * 2);
}

CRkModelHandle::~CRkModelHandle()
{
    /* unInit() 必须在析构里显式调：rknn 上下文与 dma 缓冲挂在封装类的裸指针上，
       只靠成员析构函数不会归还 */
    if (m_spV5)
    {
        m_spV5->unInit();
    }
    if (m_spV8)
    {
        m_spV8->unInit();
    }
    if (m_spV8Point)
    {
        m_spV8Point->unInit();
    }
}

ModelPtr CRkModelHandle::create(const CModelDesc_S &stDesc)
{
    if (stDesc.strJsonPath.empty())
    {
        dlog_error("rk_handle: %s 无 json_path，后端不推断模型 dims",
                   stDesc.strKey.c_str());
        return nullptr;
    }
    Kind enKind = Kind::V5;
    if (CLASSROOM_BACKEND_YOLOV5 == stDesc.strBackend)
    {
        enKind = Kind::V5;
    }
    else if (CLASSROOM_BACKEND_YOLOV8 == stDesc.strBackend)
    {
        enKind = Kind::V8;
    }
    else if (CLASSROOM_BACKEND_YOLOV8_POINT == stDesc.strBackend)
    {
        enKind = Kind::V8_POINT;
    }
    else
    {
        dlog_error("rk_handle: %s backend=%s 本适配器不支持 (raw 模型另有适配器)",
                   stDesc.strKey.c_str(), stDesc.strBackend.c_str());
        return nullptr;
    }

    std::shared_ptr<CRkModelHandle> spHandle(new CRkModelHandle());
    spHandle->m_strKey = stDesc.strKey;
    spHandle->m_enKind = enKind;

    const int64_t llBeforeKb = rssNowKb();
    if (!spHandle->initByKind(stDesc.strJsonPath))
    {
        dlog_error("rk_handle: %s init 失败, json=%s backend=%s", stDesc.strKey.c_str(),
                   stDesc.strJsonPath.c_str(), stDesc.strBackend.c_str());
        return nullptr;
    }

    /* 真实驻留以加载前后的 RSS 增量为准 */
    const int64_t llDeltaKb = rssNowKb() - llBeforeKb;
    if (llDeltaKb >= LL_RESIDENT_VALID_MIN_KB)
    {
        spHandle->m_llResidentKb = llDeltaKb;
    }

    int nW = 0;
    int nH = 0;
    int nCh = 0;
    if (!spHandle->inputSize(nW, nH, nCh) || nW <= 0 || nH <= 0)
    {
        dlog_error("rk_handle: %s 无法读取模型输入尺寸", stDesc.strKey.c_str());
        return nullptr;
    }
    spHandle->m_u32InputW = static_cast<uint32_t>(nW);
    spHandle->m_u32InputH = static_cast<uint32_t>(nH);
    spHandle->m_u32Channel = (nCh > 0) ? static_cast<uint32_t>(nCh) : 3;

    if (0 != stDesc.u32InputW && (stDesc.u32InputW != spHandle->m_u32InputW ||
                                  stDesc.u32InputH != spHandle->m_u32InputH))
    {
        /* 不因此失败：裁剪档按配置建、张量按模型走，两者不等时 run() 会因字节数不符当场拒绝，
           但把原因在这里说清楚，省得现场只看到一条 "大小不一致" 的数字对不上 */
        dlog_warn("rk_handle: %s 输入 %ux%u 与配置 %ux%u 不符, 按模型值",
                  stDesc.strKey.c_str(), spHandle->m_u32InputW, spHandle->m_u32InputH,
                  stDesc.u32InputW, stDesc.u32InputH);
    }

    dlog_info("rk_handle: %s 就绪 backend=%s input=%ux%u resident=%lldKB%s",
              stDesc.strKey.c_str(), stDesc.strBackend.c_str(), spHandle->m_u32InputW,
              spHandle->m_u32InputH, static_cast<long long>(spHandle->m_llResidentKb),
              (spHandle->m_llResidentKb > 0) ? "" : "(not measurable, use declared)");
    return spHandle;
}

bool CRkModelHandle::initByKind(const std::string &strJsonPath)
{
    switch (m_enKind)
    {
    case Kind::V5:
        m_spV5.reset(new Inference_NS::CYolov5(strJsonPath));
        return m_spV5->init();
    case Kind::V8:
        m_spV8.reset(new Inference_NS::CYoloUltralytics(strJsonPath));
        return m_spV8->init();
    case Kind::V8_POINT:
        m_spV8Point.reset(new Inference_NS::CYoloUltralyticsPoint(strJsonPath));
        return m_spV8Point->init();
    }
    /* 到不了这里：三个值都在上面。留着而不是省掉，是为了将来加第四家时，
       漏写分支的表现是「加载失败并被日志说明」，不是「拿着 V5 的句柄跑 V8 的模型」 */
    return false;
}

bool CRkModelHandle::inputSize(int &rnW, int &rnH, int &rnCh)
{
    /* 三家的 getSizeLimit 都是基类的非虚函数，且没有对外暴露可用的基类指针，
       所以只能按 Kind 分派 */
    switch (m_enKind)
    {
    case Kind::V5:
        return m_spV5 && m_spV5->getSizeLimit(0, rnW, rnH, rnCh);
    case Kind::V8:
        return m_spV8 && m_spV8->getSizeLimit(0, rnW, rnH, rnCh);
    case Kind::V8_POINT:
        return m_spV8Point && m_spV8Point->getSizeLimit(0, rnW, rnH, rnCh);
    default:
        return false;
    }
}

size_t CRkModelHandle::expectedInputBytes() const
{
    return static_cast<size_t>(m_u32InputW) * m_u32InputH * m_u32Channel;
}

void CRkModelHandle::setThresholds(float fConfidence, float fNms)
{
    /* 非法值按「不改」处理而不是记下来：后端拿到 0 或 2.0 会把所有框都过掉，
       表现成「模型在跑但永远 0 人」，这类静默失效最难查 */
    if (fConfidence > 0.0f && fConfidence < 1.0f)
    {
        m_fConfidence = fConfidence;
    }
    if (fNms > 0.0f && fNms < 1.0f)
    {
        m_fNms = fNms;
    }
}

bool CRkModelHandle::applyThreshold(void)
{
    if (m_fConfidence < 0.0f)
    {
        return true;    /* 没人下发过门限：沿用模型 JSON 里的值，一次 setParam 都不做 */
    }
    if (m_fConfidence == m_fAppliedConf && m_fNms == m_fAppliedNms)
    {
        return true;    /* 门限是低频配置，逐拍比对即可，不必每拍都下进后端 */
    }

    const float fNms = (m_fNms > 0.0f) ? m_fNms : -1.0f;    /* -1 = 让后端保持自己的 nms */
    /* 三家的 setParam 签名相同但都是各自的非虚函数，只能按 Kind 分派。
       这里以前写成「有 V5 用 V5，否则用 V8」——那种判法加一家就会静默地把 point 那一路
       下发到错的句柄上，门限配了没生效且不报错 */
    bool bOk = false;
    switch (m_enKind)
    {
    case Kind::V5:
        bOk = m_spV5->setParam(m_fConfidence, fNms);
        break;
    case Kind::V8:
        bOk = m_spV8->setParam(m_fConfidence, fNms);
        break;
    default:
        bOk = m_spV8Point->setParam(m_fConfidence, fNms);
        break;
    }
    if (!bOk)
    {
        dlog_error("rk_handle: %s setParam 被拒绝 conf=%.2f nms=%.2f", m_strKey.c_str(),
                   m_fConfidence, fNms);
        return false;
    }
    m_fAppliedConf = m_fConfidence;
    m_fAppliedNms = m_fNms;
    return true;
}

bool CRkModelHandle::run(const uint8_t *pucInput, size_t nInBytes, std::vector<float> &rvfOutput)
{
    rvfOutput.clear();
    m_u32LastBoxNum = 0;

    if (!pucInput)
    {
        return false;
    }
    /* Kind 与手里的句柄必须对得上：对不上只可能是构造走错了分支，
       此时按另一家的输出布局解这一家的张量，出来的每个数都是错的 */
    const bool bHaveHandle = (Kind::V5 == m_enKind)   ? (nullptr != m_spV5)
                            : (Kind::V8 == m_enKind)  ? (nullptr != m_spV8)
                                                      : (nullptr != m_spV8Point);
    if (!bHaveHandle)
    {
        dlog_error("rk_handle: %s kind/handle 失配", m_strKey.c_str());
        return false;
    }
    /* 字节数硬校验：RGA 档按配置尺寸建，模型按自身尺寸收，两边不一致时必须失败在这里，
       而不是让后端按错的布局解出一堆乱框（现网「能跑、框乱飞」的根因之一） */
    if (nInBytes != expectedInputBytes())
    {
        dlog_error("rk_handle: %s 输入 %zu 字节, 模型要 %u x %u x %u = %zu",
                   m_strKey.c_str(), nInBytes, m_u32InputW, m_u32InputH, m_u32Channel,
                   expectedInputBytes());
        return false;
    }
    if (!applyThreshold())
    {
        return false;
    }

    Inference_NS::InputData_S stIn;
    stIn.pData = reinterpret_cast<float *>(const_cast<uint8_t *>(pucInput));
    /* 共享仓按 fp32 张量校验输入字节数（CVInferenceRK::inferenceInfe 里乘了 sizeof(float)），
       而我们喂的是 uint8 张量：不乘 4 会在这里被拒，且报的是「大小不一致」这种误导性错误 */
    stIn.nDataSize = static_cast<int>(nInBytes * sizeof(float));
    stIn.stBoxs.fConfidence = m_fConfidence;
    stIn.stBoxs.fNms = m_fNms;

    m_vecBox.clear();
    m_vecPoint.clear();
    /* 注意：Yolov5/YoloUltralytics::inference 解析了 stBoxs 却把成员变量传给 postprocess，
       所以 stBoxs 这条路是无效的，门限真正的生效点在上面的 applyThreshold() */
    bool bOk = false;
    switch (m_enKind)
    {
    case Kind::V5:
        bOk = m_spV5->inference(stIn, m_vecBox);
        break;
    case Kind::V8:
        bOk = m_spV8->inference(stIn, m_vecBox);
        break;
    default:
        bOk = m_spV8Point->inference(stIn, m_vecPoint);
        break;
    }
    if (!bOk)
    {
        return false;
    }

    if (Kind::V8_POINT == m_enKind)
    {
        packPoints(rvfOutput);
    }
    else
    {
        packBoxes(rvfOutput);
    }
    return true;
}

void CRkModelHandle::packPoints(std::vector<float> &rvfOutput)
{
    const size_t nNeed = m_vecPoint.size() * CLASSROOM_FACE_BOX_FLOAT_NUM;
    if (rvfOutput.capacity() < nNeed)
    {
        rvfOutput.reserve(nNeed);
    }
    uint32_t nPacked = 0;
    for (const Inference_NS::PointData_S &stOne : m_vecPoint)
    {
        /* 点数不够一行声明的那些就丢掉整框：下游（face_decode）是按固定行长跳的，
           少写一个点等于把后面每一框都挪一位——那是一整帧的结果错乱，
           而不是少一个人脸。点数够不够由模型的 point_num 决定，与本框的检出质量无关，
           所以这里真的发生就说明封装类与模型不配，属于必须在日志里露出来的那类错 */
        if (static_cast<int>(stOne.vPoints.size()) < CLASSROOM_FACE_LANDMARK_NUM)
        {
            dlog_warn("rk_handle: %s 人脸行携带 %d 个点，期望 %d，整行丢弃",
                      m_strKey.c_str(), static_cast<int>(stOne.vPoints.size()),
                      CLASSROOM_FACE_LANDMARK_NUM);
            continue;
        }
        rvfOutput.push_back(static_cast<float>(stOne.stBoxs.nX1));
        rvfOutput.push_back(static_cast<float>(stOne.stBoxs.nY1));
        rvfOutput.push_back(static_cast<float>(stOne.stBoxs.nX2));
        rvfOutput.push_back(static_cast<float>(stOne.stBoxs.nY2));
        rvfOutput.push_back(stOne.fConfidence);
        rvfOutput.push_back(static_cast<float>(stOne.nLabel));
        for (int nPt = 0; nPt < CLASSROOM_FACE_LANDMARK_NUM; ++nPt)
        {
            /* 点是共享后处理给的**模型输入像素**整数（与框同源，换算留给 face_decode）；
               分数一列推 1.0 而不是 nShow：共享仓的 processKeyPoint 把「分数」读成了
               Y 坐标通道（YOLOV8PostProcess.cpp 里 nP*3+1，正确通道是 +2 且要过 sigmoid），
               推下去的值是格内相对纵坐标，一半左右为负——solveFaceAlign 的分数门
               (score >= point_score_min) 会把这些点当低置信整点剔除，眼睛又在格子中心
               上方、原始 y 系统性偏负，等于专门剔眼睛。真实分数在 share 内部已被丢弃、无从恢复，推 1.0 的语义是
               「分数维度不可用」，门在 point_score_min>=0 时自然全通 */
            const Inference_NS::Point_S &stPt = stOne.vPoints[static_cast<size_t>(nPt)];
            rvfOutput.push_back(static_cast<float>(stPt.nX));
            rvfOutput.push_back(static_cast<float>(stPt.nY));
            rvfOutput.push_back(1.0f);
        }
        ++nPacked;
    }
    m_u32LastBoxNum = nPacked;
}

void CRkModelHandle::packBoxes(std::vector<float> &rvfOutput)
{
    const size_t nNeed = m_vecBox.size() * CLASSROOM_BOX_FLOAT_NUM;
    if (rvfOutput.capacity() < nNeed)
    {
        rvfOutput.reserve(nNeed);
    }
    for (const Inference_NS::BoxData_S &stBox : m_vecBox)
    {
        rvfOutput.push_back(static_cast<float>(stBox.stBoxs.nX1));
        rvfOutput.push_back(static_cast<float>(stBox.stBoxs.nY1));
        rvfOutput.push_back(static_cast<float>(stBox.stBoxs.nX2));
        rvfOutput.push_back(static_cast<float>(stBox.stBoxs.nY2));
        rvfOutput.push_back(stBox.fConfidence);
        rvfOutput.push_back(static_cast<float>(stBox.nLabel));
    }
    m_u32LastBoxNum = static_cast<uint32_t>(m_vecBox.size());
}
}
