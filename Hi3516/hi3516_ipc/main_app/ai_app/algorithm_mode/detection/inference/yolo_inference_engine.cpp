/**
 * @FilePath     : yolo_inference_engine.cpp
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2026-08-25 16:30:00
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-08-25 16:30:00
 * @Description  : YOLO 推理引擎实现（包装 CYoloUltralytics_rpn）
 */

#include "yolo_inference_engine.hpp"

#include <cmath>
#include <cstdio>
#include <fstream>
#include <iterator>

#include "IpcRet.h"
#include "dlog.h"
#include "video_define.h"

#if CAP_AI_USE_SIMPLE_JSON
#include "Json.h"
#else
#include "JsonInterfase.h"
#endif

namespace
{
/* YVU420SP 单像素数据量 = 1.5 字节（Y 全量 + UV 半量）；
 * 模型输入按 float 存储（与 CYoloUltralytics 输入约定一致） */
constexpr double YUV420SP_BYTES_PER_PIXEL = 1.5;
} // namespace

CYoloInferenceEngine::~CYoloInferenceEngine()
{
    unInit();
}

int CYoloInferenceEngine::init(const std::string &strConfigPath)
{
    /* 仅引擎 Worker 线程调用（引擎保证 init/unInit/infer 同线程串行） */
    if (m_pYoloHandle != nullptr)
    {
        return OK;
    }

    /* new (std::nothrow)：引擎 Worker 线程不允许异常逃逸（会 std::terminate），
     * 分配失败返回 nullptr 走引擎 1s 重试路径 */
    m_pYoloHandle = new (std::nothrow) Inference_NS::CYoloUltralytics(strConfigPath);
    if (m_pYoloHandle == nullptr)
    {
        dlog_error("YOLO 引擎初始化失败-创建模型对象失败");
        return ERR;
    }
    if (!m_pYoloHandle->init())
    {
        delete m_pYoloHandle;
        m_pYoloHandle = nullptr;
        dlog_error("YOLO 引擎初始化失败 [%s]", strConfigPath.c_str());
        return ERR;
    }

    /* 模型输入分辨率：优先读配置 pre_process.size（[0]=宽 [1]=高），失败回退
     * 1024×576（展馆模型固定分辨率）并告警；与 CHVFDetect 读 model_info 不同——
     * YOLO 引擎的模型尺寸来自配置文件 */
    m_stModelInputSize = AiPipeline_NS::FrameSize_S();
    if (!parse_model_input_size(strConfigPath, m_stModelInputSize))
    {
        m_stModelInputSize.nWidth = PIXEL_WIDTH_1024;
        m_stModelInputSize.nHeight = PIXEL_HEIGHT_576;
        dlog_warn("YOLO 引擎读取模型输入尺寸失败，回退 [%ux%u]",
                  m_stModelInputSize.nWidth, m_stModelInputSize.nHeight);
    }
    return OK;
}

void CYoloInferenceEngine::unInit()
{
    /* 仅引擎 Worker 线程调用 */
    if (m_pYoloHandle != nullptr)
    {
        /* memory: 析构内部调用 unInit 释放模型资源 */
        delete m_pYoloHandle;
        m_pYoloHandle = nullptr;
    }
    m_stModelInputSize = AiPipeline_NS::FrameSize_S();
}

int CYoloInferenceEngine::infer(const ot_video_frame_info *pPreparedFrame,
                                std::vector<DetectionBox_S> &vOutBoxes)
{
    /* 仅引擎 Worker 线程调用 */
    if (pPreparedFrame == nullptr || m_pYoloHandle == nullptr)
    {
        return ERR_PARAM;
    }
    if (m_stModelInputSize.nWidth == 0U || m_stModelInputSize.nHeight == 0U)
    {
        dlog_warn("YOLO 引擎模型输入尺寸未初始化，本帧跳过");
        return ERR_UNINIT;
    }

    /* 1. 构造模型输入：指向已准备的 YUV 帧（YVU420SP），
     *    数据量 = 宽*高*1.5 个 float（与 exhibition_detect_model 原实现一致） */
    Inference_NS::InputData_S stInputData;
    stInputData.pData = reinterpret_cast<float *>(pPreparedFrame->video_frame.virt_addr[0]);
    stInputData.nDataSize = static_cast<int>(
        static_cast<double>(m_stModelInputSize.nWidth) * static_cast<double>(m_stModelInputSize.nHeight) *
        YUV420SP_BYTES_PER_PIXEL * static_cast<double>(sizeof(float)));

    std::vector<Inference_NS::BoxData_S> vRawBoxes;
    if (!m_pYoloHandle->inference(stInputData, vRawBoxes))
    {
        dlog_warn("YOLO 引擎推理失败，本帧跳过");
        return ERR;
    }

    /* 2. BoxData_S（stBoxs 左上角/右下角 + nLabel + fConfidence）-> DetectionBox_S，
     *    字段一一对应；非法框（宽高<=0、置信度非有限或超 [0,1]）跳过并累计丢弃计数 */
    vOutBoxes.clear();
    vOutBoxes.reserve(vRawBoxes.size());
    for (const auto &stRawBox : vRawBoxes)
    {
        if (stRawBox.stBoxs.nX2 <= stRawBox.stBoxs.nX1 || stRawBox.stBoxs.nY2 <= stRawBox.stBoxs.nY1)
        {
            ++m_ullDroppedObjectCount;
            continue;
        }
        if (!std::isfinite(stRawBox.fConfidence) || stRawBox.fConfidence < 0.0F ||
            stRawBox.fConfidence > 1.0F)
        {
            ++m_ullDroppedObjectCount;
            continue;
        }

        DetectionBox_S stBox;
        stBox.nClassId = stRawBox.nLabel;
        stBox.fConfidence = stRawBox.fConfidence;
        stBox.nX1 = stRawBox.stBoxs.nX1;
        stBox.nY1 = stRawBox.stBoxs.nY1;
        stBox.nX2 = stRawBox.stBoxs.nX2;
        stBox.nY2 = stRawBox.stBoxs.nY2;
        /* YOLO 引擎不提供 Track ID/生命周期（optTrackId 不填、UNAVAILABLE），
         * 由模型适配器用跟踪器补充 */
        vOutBoxes.emplace_back(stBox);
    }

    /* 丢弃诊断：仅在本帧发生丢弃时输出（帧级热路径依赖 dlog 全局限流按调用点合并，
     * 不自行维护限频时间戳） */
    if (m_ullDroppedObjectCount != m_ullLastWarnDropCount)
    {
        m_ullLastWarnDropCount = m_ullDroppedObjectCount;
        dlog_warn("YOLO 引擎丢弃非法检测框，累计[%llu]",
                  static_cast<unsigned long long>(m_ullDroppedObjectCount));
    }
    return OK;
}

AiPipeline_NS::FrameSize_S CYoloInferenceEngine::model_input_size() const
{
    return m_stModelInputSize;
}

bool CYoloInferenceEngine::parse_model_input_size(const std::string &strConfigPath,
                                                  AiPipeline_NS::FrameSize_S &stOut) const
{
    /* 与 CYoloUltralytics::checkModelProConfig 相同的 Json 读取方式 */
    std::ifstream inFile(strConfigPath);
    if (!inFile)
    {
        return false;
    }
    const std::string strJson((std::istreambuf_iterator<char>(inFile)), std::istreambuf_iterator<char>());

    Json::Object *pJsonHandle = Json::init(strJson);
    if (pJsonHandle == nullptr)
    {
        return false;
    }

    bool bRet = false;
    Json::Object *pPreProcess = Json::get(pJsonHandle, "pre_process");
    if (pPreProcess != nullptr)
    {
        Json::Object *pSizeArray = Json::get(pPreProcess, "size");
        if (pSizeArray != nullptr && Json::Array::size(pSizeArray) >= 2)
        {
            Json::Object *pWidthItem = Json::Array::get(pSizeArray, 0);
            Json::Object *pHeightItem = Json::Array::get(pSizeArray, 1);
            int nWidth = 0;
            int nHeight = 0;
            if (pWidthItem != nullptr && pHeightItem != nullptr &&
                Json::Value::get(pWidthItem, nWidth) && Json::Value::get(pHeightItem, nHeight) &&
                nWidth > 0 && nHeight > 0)
            {
                stOut.nWidth = static_cast<uint32_t>(nWidth);
                stOut.nHeight = static_cast<uint32_t>(nHeight);
                bRet = true;
            }
        }
    }

    Json::deinit(pJsonHandle);
    return bRet;
}
