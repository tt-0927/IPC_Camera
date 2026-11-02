/*
 * @Author: zhengxh zhengxh@kfb.cn
 * @Date: 2026-10-10 10:30:00
 * @LastEditors: zhengxh
 * @LastEditTime: 2026-10-26 17:24:32
 * @FilePath: /1126/rv1126b_ipc/main_app/ai_app/detect_mode/channel_occupation_detect/channel_occupation_detect.cpp
 * @Description: 通道占用检测
 */

#include "channel_occupation_detect.hpp"
#include "event_configure.h"

/* 数据队列 */
#define QUEUE_MAX (2)

CChannelOccupationDetect::CChannelOccupationDetect()
    : m_dateQueue(QUEUE_MAX)
{
    /* 启动线程 */
    m_bRunning.store(true);
    m_thread = std::thread(&CChannelOccupationDetect::run, this);
}

CChannelOccupationDetect::~CChannelOccupationDetect()
{
    /* 通知线程停止 */
    m_bRunning.store(false);
    // note 调用 shutdown() 来唤醒可能阻塞在 pop() 的线程
    m_dateQueue.shutdown();
    if (m_thread.joinable())
    {
        m_thread.join();
    }
    m_dateQueue.clear();
    /* 关闭时立即结束未完结的报警状态 */
    m_alarmStateMachine.endAlarmImmediately(Event::Type_E::CHANNEL_OCCUPATION);
    unInit();
}

/* 接受媒体数据 */
void CChannelOccupationDetect::recvMediaData(MediaData_S stMediaData)
{
    /* 句柄是否建立不作为门禁 首帧需进入队列建立检测基准 */
    if (!m_stChannelOccupationCfg.bEnable)
    {
        return;
    }

    m_nChannelId = stMediaData.stMediaParam.nChannel;

    if (m_RecvManager.handleEvent(stMediaData.stMediaParam.nChannel))
    {
        if (m_dateQueue.size() >= QUEUE_MAX)
        {
            dlog_error("通道占用检测-数据队列满了 [%d]", m_dateQueue.size());
        }
        m_dateQueue.pushOrReplace(stMediaData);
    }

    return ;
}

/**
 * @brief 更新算法配置参数
 * @param stAlgoConfig
 */
void CChannelOccupationDetect::setAlgoEnCfg(const Event::AlgorithmConfig &stAlgoConfig)
{
    m_stChannelOccupationCfg.bEnable = stAlgoConfig.nChannelOccupation;

    if (m_stChannelOccupationCfg.bEnable)
    {
        Alarm::ChannelOccupation_S stInfo;
        CEventConfigure::instance()->get_configure(stInfo);
        setAlgoParamCfg(stInfo);
        /* 任何配置变更，以当前画面重建 */
        m_bBuildBaseline.store(true);
        m_bLastAlarmState = false;
    }
}

void CChannelOccupationDetect::setAlgoParamCfg(const Alarm::ChannelOccupation_S &stAlgoCfg)
{
    dlog_debug("ai_app: 设置通道占用检测参数");
    bool bRegionChanged = (m_stChannelOccupationCfg.stRegion != stAlgoCfg.stRegion);
    m_stChannelOccupationCfg = stAlgoCfg;

    if (bRegionChanged)
    {
        /* 灵敏度变化模块内热切换生效 仅区域变更需重建检测基准 */
        m_bBuildBaseline.store(true);
    }
    return ;
}

/* 初始化 */
bool CChannelOccupationDetect::init(const cv::Mat &stFrameMat)
{
    if (stFrameMat.empty())
    {
        dlog_error("通道占用检测-基准帧为空 无法初始化");
        return false;
    }

    CChannelOccupationDetectModel::InParam_S stInParam;
    /* 首帧同时作为对比帧与背景帧 建立静态检测基准 */
    stInParam.prevMat = stFrameMat.clone();
    stInParam.bgMat = stFrameMat.clone();
    stInParam.vecPoints = convertRegionToPoints();

    m_pChannelOccupationDetHandle = new CChannelOccupationDetectModel(stInParam);
    if (m_pChannelOccupationDetHandle)
    {
        if (m_pChannelOccupationDetHandle->init())
        {
            dlog_info("通道占用检测基准建立成功 区域点数[%d] 分辨率[%dx%d]",
                      static_cast<int>(stInParam.vecPoints.size()), m_nWidth, m_nHeight);
            return true;
        }
        unInit();
        dlog_error("通道占用检测算法初始化失败");
    }

    return false;
}

/* 反初始化 */
bool CChannelOccupationDetect::unInit()
{
    if (m_pChannelOccupationDetHandle)
    {
        delete m_pChannelOccupationDetHandle;
        m_pChannelOccupationDetHandle = nullptr;
    }
    return true;
}

/* 线程函数 */
void CChannelOccupationDetect::run()
{
    MediaData_S      stMediaData;

    while (m_bRunning.load())
    {
        /* 阻塞获取 */
        m_dateQueue.pop(stMediaData, -1);
        if (m_stChannelOccupationCfg.bEnable == false)
        {
            continue;
        }

        if (stMediaData.nSize == 0 || !stMediaData.pData)
        {
            dlog_error("ai_app: 通道占用检测-原始数据帧为空");
            continue;
        }

        const int nFrameW = stMediaData.stMediaParam.nVideoWidth;
        const int nFrameH = stMediaData.stMediaParam.nVideoHeight;
        if (nFrameW <= 0 || nFrameH <= 0)
        {
            dlog_error("ai_app: 通道占用检测-帧分辨率异常");
            continue;
        }

        frameRate("通道占用检测-分析数据", 5);

        /* NV12→RGB：RGA硬件优先 失败回退CPU */
        cv::Mat stFrameMat;
        {
            cv::Mat nv12Mat(nFrameH * 3 / 2, nFrameW, CV_8UC1, stMediaData.pData.get());
            if (m_stFullRgbMat.cols != nFrameW || m_stFullRgbMat.rows != nFrameH || m_stFullRgbMat.type() != CV_8UC3)
            {
                m_stFullRgbMat.create(nFrameH, nFrameW, CV_8UC3);
            }
            if (!rga_image_transform(stMediaData.pData.get(), nFrameW, nFrameH, RK_FORMAT_YCbCr_420_SP,
                                     m_stFullRgbMat.data, nFrameW, nFrameH, RK_FORMAT_RGB_888,
                                     0, 0, nFrameW, nFrameH, 0))
            {
                /* RGA失败：CPU回退 */
                cv::cvtColor(nv12Mat, m_stFullRgbMat, cv::COLOR_YUV2RGB_NV12);
            }

            /* 统一到算法输入分辨率 与基准帧尺寸保持一致 */
            cv::resize(m_stFullRgbMat, stFrameMat,
                       cv::Size(m_nWidth, m_nHeight), 0, 0, cv::INTER_LINEAR);
        }

        if (stFrameMat.empty())
        {
            dlog_error("ai_app: 通道占用检测-图片数据为空");
            continue;
        }

        std::unique_lock<std::mutex> lock(m_mutex);

        /* 重建基准 */
        if (m_bBuildBaseline.load())
        {
            m_bBuildBaseline.store(false);
            unInit();
            dlog_info("通道占用检测-检测基准重建");
        }

        /* 模块要求对比帧/背景帧非空 句柄只能由首帧建立 */
        if (!m_pChannelOccupationDetHandle)
        {
            if (!init(stFrameMat))
            {
                dlog_error("通道占用检测-等待建立基准");
                continue;
            }
        }

        /* 分析数据 */
        CChannelOccupationDetectModel::InData_S stInData;
        CChannelOccupationDetectModel::OutData_S stOutData;
        stInData.inMat = stFrameMat;
        stInData.stParam.nSensitivity = m_stChannelOccupationCfg.nSensitivity;

        int nRet = m_pChannelOccupationDetHandle->process(stInData, stOutData);
        if (nRet != 0)
        {
            dlog_error("通道占用检测处理失败 [%d]", nRet);
            continue;
        }

        /* 结果交报警状态机 时间与状态判断由状态机统一维护 */
        channelOccupationProcess(stOutData);
    }
}

/* 配置区域转算法检测多边形 */
std::vector<cv::Point> CChannelOccupationDetect::convertRegionToPoints()
{
    std::vector<cv::Point> vecPoints;

    Alarm::Region_S stRegion = m_stChannelOccupationCfg.stRegion;
    if (!stRegion.IsValid())
    {
        /* 区域无效时不传点 由算法按整幅画面检测 */
        return vecPoints;
    }

    /* 配置区域为1920x1080基准 转换到算法输入分辨率 */
    stRegion.ConvertResolution(PIXEL_WIDTH_1920, PIXEL_HEIGHT_1080, m_nWidth, m_nHeight);

    vecPoints.reserve(stRegion.nPointNum);
    for (unsigned int i = 0; i < stRegion.nPointNum; ++i)
    {
        vecPoints.emplace_back(static_cast<int>(stRegion.aPoint[i].fX),
                               static_cast<int>(stRegion.aPoint[i].fY));
    }

    return vecPoints;
}

/* 检测结果交报警状态机 */
void CChannelOccupationDetect::channelOccupationProcess(const CChannelOccupationDetectModel::OutData_S &stOutData)
{
    /* 目标运动中视为途经 不触发通道占用报警 */
    m_bLastAlarmState = stOutData.bOccupied && (!stOutData.bMoving || m_bLastAlarmState);

    /* 构建事件触发上下文 */
    EventTriggerContext_S stContext;
    stContext.enEventType = Event::Type_E::CHANNEL_OCCUPATION;
    stContext.nChnId = m_nChannelId;
    stContext.llTimestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
                               std::chrono::system_clock::now().time_since_epoch())
                               .count();

    m_alarmStateMachine.handleAlarmState(m_bLastAlarmState, stContext);
}
