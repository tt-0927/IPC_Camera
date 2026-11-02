/**
 * @FilePath     : rv1126b_ipc/main_app/ai_app/algorithm_mode/channel_occupation_detect_model/channel_occupation_detect_model.cpp
 * @Date         : 2026-10-23
 * @Description  : 通道占用检测纯组件实现
 */

#include "channel_occupation_detect_model.hpp"

#include <unistd.h>

#include "ChannelOccupationDetect.hpp"
#include "dlog.h"

/* 当系统中存在 /printAlgoResults 文件时打印检测结果 */
static void printDetectResult(const CChannelOccupationDetectModel::OutData_S &stOutData)
{
    if (access("/printAlgoResults", F_OK))
    {
        return;
    }

    if (stOutData.bOccupied)
    {
        dlog_info("CHANNEL_OCCUPANCY: 占用 bbox=(%d, %d, %d, %d) area=%.4f moving=%d motion=%.4f",
                  stOutData.stBbox.x, stOutData.stBbox.y,
                  stOutData.stBbox.width, stOutData.stBbox.height,
                  stOutData.fAreaRatio,
                  static_cast<int>(stOutData.bMoving),
                  stOutData.fMotionRatio);
    }
    else
    {
        dlog_info("CHANNEL_OCCUPANCY: 未占用");
    }
}

/* 非法档位按中灵敏度处理 */
static unsigned int normalizeSensitivity(unsigned int nSensitivity)
{
    if (nSensitivity == CHANNEL_OCCUPANCY_SENSITIVITY_LOW ||
        nSensitivity == CHANNEL_OCCUPANCY_SENSITIVITY_HIGH)
    {
        return nSensitivity;
    }
    return CHANNEL_OCCUPANCY_SENSITIVITY_MID;
}

CChannelOccupationDetectModel::CChannelOccupationDetectModel(const InParam_S &stInParam)
    : m_stInParam(stInParam)
{
}

CChannelOccupationDetectModel::~CChannelOccupationDetectModel()
{
    unInit();
}

bool CChannelOccupationDetectModel::init()
{
    /* 重复 init 先释放旧句柄，保证可重试 */
    if (m_pDetector != nullptr)
    {
        unInit();
    }

    /* 对比帧/背景帧是建立检测基准的必要数据 */
    if (m_stInParam.prevMat.empty() || m_stInParam.bgMat.empty())
    {
        dlog_error("CHANNEL_OCCUPANCY: 对比帧或背景帧为空，无法初始化");
        return false;
    }

    if (m_stInParam.prevMat.size() != m_stInParam.bgMat.size() ||
        m_stInParam.prevMat.type() != m_stInParam.bgMat.type())
    {
        dlog_error("CHANNEL_OCCUPANCY: 对比帧与背景帧尺寸或类型不一致 prev[%dx%d] bg[%dx%d]",
                   m_stInParam.prevMat.cols, m_stInParam.prevMat.rows,
                   m_stInParam.bgMat.cols, m_stInParam.bgMat.rows);
        return false;
    }

    if (m_stInParam.prevMat.type() != CV_8UC3)
    {
        dlog_error("CHANNEL_OCCUPANCY: 基准帧非 CV_8UC3 RGB 格式");
        return false;
    }

    if (!m_stInParam.vecPoints.empty())
    {
        if (m_stInParam.vecPoints.size() < 3)
        {
            dlog_error("CHANNEL_OCCUPANCY: 检测区域有效点数不足 3 个");
            return false;
        }

        /* ROI 包围盒须与画面有交集，否则算法内部裁剪区域为空 */
        cv::Rect stRoiRect = cv::boundingRect(m_stInParam.vecPoints) &
                             cv::Rect(0, 0, m_stInParam.prevMat.cols, m_stInParam.prevMat.rows);
        if (stRoiRect.width <= 0 || stRoiRect.height <= 0)
        {
            dlog_error("CHANNEL_OCCUPANCY: 检测区域超出画面范围");
            return false;
        }
    }

    m_pDetector = new Inference_NS::CChannelOccupationDetect(m_stInParam.prevMat,
                                                             m_stInParam.bgMat,
                                                             m_stInParam.vecPoints);
    if (m_pDetector == nullptr)
    {
        dlog_error("CHANNEL_OCCUPANCY: 算法初始化失败");
        return false;
    }

    /* 按默认灵敏度档位设置算法参数 */
    applySensitivity(m_nSensitivity);

    dlog_info("CHANNEL_OCCUPANCY: 初始化成功 区域点数[%d] 分辨率[%dx%d]",
              static_cast<int>(m_stInParam.vecPoints.size()),
              m_stInParam.prevMat.cols, m_stInParam.prevMat.rows);
    return true;
}

bool CChannelOccupationDetectModel::unInit()
{
    if (m_pDetector != nullptr)
    {
        delete m_pDetector;
        m_pDetector = nullptr;
    }
    return true;
}

int CChannelOccupationDetectModel::process(const InData_S &stInData, OutData_S &stOutData)
{
    /* 清零输出 避免调用方复用结构体时读到上一帧的旧值 */
    stOutData = OutData_S();

    if (m_pDetector == nullptr)
    {
        dlog_error("CHANNEL_OCCUPANCY: 未初始化，无法检测");
        return -1;
    }

    if (stInData.inMat.empty() || stInData.inMat.type() != CV_8UC3)
    {
        dlog_error("CHANNEL_OCCUPANCY: 输入图片为空或非 CV_8UC3 RGB 格式");
        return -2;
    }

    /* 算法内部裁剪区域基于初始化帧计算，尺寸变化会越界 */
    if (stInData.inMat.size() != m_stInParam.prevMat.size())
    {
        dlog_error("CHANNEL_OCCUPANCY: 输入尺寸[%dx%d]与初始化尺寸[%dx%d]不一致",
                   stInData.inMat.cols, stInData.inMat.rows,
                   m_stInParam.prevMat.cols, m_stInParam.prevMat.rows);
        return -2;
    }

    /* 灵敏度档位变化时重新设置算法参数 */
    unsigned int nSensitivity = normalizeSensitivity(stInData.stParam.nSensitivity);
    if (nSensitivity != m_nSensitivity)
    {
        applySensitivity(nSensitivity);
    }

    /* inference 入参为非 const 引用，拷贝 Mat 头即可 */
    cv::Mat matInput = stInData.inMat;

    Inference_NS::ChannelDetectResult stResult;
    if (!m_pDetector->inference(matInput, stResult))
    {
        dlog_error("CHANNEL_OCCUPANCY: 推理失败");
        return -3;
    }

    /* 装载结果：占用标志与目标信息，时间/状态判断由调用方维护 */
    stOutData.bOccupied    = stResult.target_exists;
    stOutData.stBbox       = stResult.bbox;
    stOutData.fAreaRatio   = stResult.area_ratio;
    stOutData.bMoving      = stResult.moving;
    stOutData.fMotionRatio = stResult.motion_ratio;

    printDetectResult(stOutData);

    return 0;
}

void CChannelOccupationDetectModel::applySensitivity(unsigned int nSensitivity)
{
    nSensitivity = normalizeSensitivity(nSensitivity);

    Inference_NS::ChannelDetectConfig stConfig;

    if (nSensitivity == CHANNEL_OCCUPANCY_SENSITIVITY_LOW)
    {
        /* 低灵敏度 只报明显占用 抗干扰强 小目标易漏 */
        stConfig.background_threshold   = 45;
        stConfig.min_area               = 300;
        stConfig.motion_ratio_threshold = 0.03f;
    }
    else if (nSensitivity == CHANNEL_OCCUPANCY_SENSITIVITY_HIGH)
    {
        /* 高灵敏度 抓小目标和细微变化 漏报少 误报相对多 */
        stConfig.background_threshold   = 18;
        stConfig.min_area               = 60;
        stConfig.motion_ratio_threshold = 0.08f;
    }
    else
    {
        /* 中灵敏度 漏报误报均衡 */
        stConfig.background_threshold   = 30;
        stConfig.min_area               = 150;
        stConfig.motion_ratio_threshold = 0.05f;
    }

    m_nSensitivity = nSensitivity;
    m_pDetector->setParam(stConfig);
}
