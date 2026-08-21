/**
 * @FilePath     : motion_detect.cpp
 * @Author       : 梁浩尧 lianghaoyao@kfb.cn
 * @Date         : 2025-11-05 10:38:00
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-08-11 13:43:07
 * @Description  : 移动侦测
 */

#include "motion_detect.hpp"
#include "event_configure.h"
#include "isp_dayNight.h"
#include <sys/times.h>
#include "time_utils.h"

/* 数据队列 */
#define QUEUE_MAX (2)

CMotionDetect::CMotionDetect()
    : m_dateQueue(QUEUE_MAX)
{
    /* 默认侦测区域 */
    m_stRect.nWidth = m_nWidth;
    m_stRect.nHeight = m_nHeight;

    /* 启动线程 */
    m_bRunning.store(true);
    m_thread = std::thread(&CMotionDetect::run, this);
}

CMotionDetect::~CMotionDetect()
{
    /* 通知线程停止 */
    m_bRunning.store(false);
    m_condition.notify_all();
    // note 调用 shutdown() 来唤醒可能阻塞在 pop() 的线程
    m_dateQueue.shutdown();
    if (m_thread.joinable())
    {
        m_thread.join();
    }
    m_dateQueue.clear();
    unInit();
}

/* 接受媒体数据 */
void CMotionDetect::recvMediaData(MediaData_S stMediaData)
{
    if (!m_stMotionDetCfg.bEnable || !m_pMotionDetHandle || !m_bIsDraw)
    {
        return;
    }

    m_nChannelId = stMediaData.stMediaParam.nChannel;

    if (m_RecvManager.handleEvent(stMediaData.stMediaParam.nChannel))
    {
        if (m_dateQueue.size() >= QUEUE_MAX)
        {
            dlog_error("移动侦测-数据队列满了 [%d]", m_dateQueue.size());
        }
        m_dateQueue.pushOrReplace(stMediaData);
    }

    return ;
}

/**
 * @brief 更新算法配置参数
 * @param stAlgoConfig 
 */
void CMotionDetect::setAlgoEnCfg(const Event::AlgorithmConfig &stAlgoConfig)
{
    m_stMotionDetCfg.bEnable = stAlgoConfig.nEnMotionDetect;

    if (m_stMotionDetCfg.bEnable)
    {
        Alarm::MotionDetection_S stInfo;
        CEventConfigure::instance()->get_configure(stInfo);
        setAlgoParamCfg(stInfo);
    }
}

void CMotionDetect::setAlgoParamCfg(const Alarm::MotionDetection_S &stAlgoCfg)
{
    dlog_debug("ai_app: 设置移动侦测参数");
    m_stMotionDetCfg = stAlgoCfg;

    if(m_stMotionDetCfg.enMode == Alarm::MotionType_E::MOTION_NORMAL) // 普通模式
    {
        if(m_stMotionDetCfg.stMotionNormalMode.nRegionType) // 网格
        {
            /* 网格二维向量 */
            auto &grid = std::get<Alarm::MotionNormalMode_S::AreaGrid>(m_stMotionDetCfg.stMotionNormalMode.varRegion);
            Common::Rect_S stRect;
            /* 转换网格区域至矩形数据结构 */
            convert_gridRegion_to_rect(grid, m_nWidth, m_nHeight, stRect);   
            /* 必须是宏块宽的偶数倍，宽度需16字节向下对齐，防止超限 */
            stRect.nX &= ~1;
            stRect.nY &= ~1;
            stRect.nWidth &= ~1;
            stRect.nHeight &= ~1;
            
            dlog_debug("[移动侦测] : m_stRect: [%d,%d][%d,%d]", m_stRect.nX, m_stRect.nY, m_stRect.nWidth, m_stRect.nHeight);
            dlog_debug("[移动侦测] :   stRect: [%d,%d][%d,%d]", stRect.nX, stRect.nY, stRect.nWidth, stRect.nHeight);
            if(stRect.nWidth == 0 || stRect.nHeight == 0)
            {
                m_bIsDraw = false;
                return;
            }

            m_bIsDraw = true;
            /* 如果改变了侦测区域的宽高，就重启 */
            if(stRect.nWidth != m_stRect.nWidth || stRect.nHeight != m_stRect.nHeight)
            {
                m_stRect = stRect;
                m_pMotionDetHandle->set_resolution(m_stRect.nWidth, m_stRect.nHeight);
                dlog_debug("[移动侦测] : m_stRect: [%d,%d][%d,%d]", m_stRect.nX, m_stRect.nY, m_stRect.nWidth, m_stRect.nHeight);
                reboot();
            }
        }
    }
    else if(m_stMotionDetCfg.enMode == Alarm::MotionType_E::MOTION_EXPERT) // 专家模式
    {
        m_stRect.nWidth = m_nWidth;
        m_stRect.nHeight = m_nHeight;

        if (m_pMotionDetHandle)
        { 
            m_pMotionDetHandle->set_resolution(m_stRect.nWidth, m_stRect.nHeight);
        }
        m_bIsDraw = false;
        m_bIsCrop = false;
        if (!m_stMotionDetCfg.stMotionExpertMode.vstMotionRegion.empty())
        {
            /* 转换区域坐标分辨率至算法分辨率 */
            for (auto &rule : m_stMotionDetCfg.stMotionExpertMode.vstMotionRegion)
            {
                rule.stRect.ConvertResolution(PIXEL_WIDTH_1920, PIXEL_HEIGHT_1080, m_nWidth, m_nHeight);
                if (rule.stRect.nWidth != 0 && rule.stRect.nHeight != 0)
                {
                    m_bIsDraw = true;
                }
            }
        }
    }
}

/* 初始化 */
bool CMotionDetect::init()
{
    if (!m_pMotionDetHandle)
    {
        MoveDetect_NS::InParam_S stInParam;
        stInParam.bDebug = false;
        // stInParam.strAnalyzeDataPath = m_strAnalyzeDataPath + "record";
        // stInParam.strOriginalDataPath = "/root/OriginalImage";
        dlog_debug("[移动侦测] 分辨率: [%d,%d]", m_stRect.nWidth, m_stRect.nHeight);
        m_pMotionDetHandle = new MoveDetect_NS::CMoveDetectV2_0(stInParam);
        if (m_pMotionDetHandle)
        { 
            if (m_pMotionDetHandle->init())
            {
                dlog_info("移动侦测算法初始化成功");
                /* 判断是否需要裁剪源视频 */
                if(m_nWidth != m_stRect.nWidth || m_nHeight != m_stRect.nHeight)
                {
                    m_pMotionDetHandle->set_resolution(m_stRect.nWidth, m_stRect.nHeight);
                    m_bIsCrop = true;
                }
                else
                {
                    m_bIsCrop = false;
                }
                return true;
            }
            else
            {
                delete m_pMotionDetHandle;
                m_pMotionDetHandle = nullptr;
                dlog_error("移动侦测算法初始化失败");
            }
        }
    }
    return false;
}

/* 反初始化 */
bool CMotionDetect::unInit()
{
    if (m_pMotionDetHandle)
    {
        delete m_pMotionDetHandle;
        m_pMotionDetHandle = nullptr;
    }
    return true;
}

bool CMotionDetect::reboot()
{
    if(!unInit())
    {
        return false;
    }
    if(!init())
    {
        return false;
    }

    return true;
}

static int rect_to_point(const std::vector<std::vector<int>> &stInRectsInfo, std::vector<Common::RectInfo_S> &vstRectsInfo)
{
    Common::RectInfo_S stRectsInfo;

    for(unsigned int i = 0; i < stInRectsInfo.size(); i++)
    {
        stRectsInfo.nX1 = stInRectsInfo[i][0];
        stRectsInfo.nY1 = stInRectsInfo[i][1];
        stRectsInfo.nX2 = stInRectsInfo[i][0] + stInRectsInfo[i][2];
        stRectsInfo.nY2 = stInRectsInfo[i][1] + stInRectsInfo[i][3];

        vstRectsInfo.push_back(stRectsInfo);
    }
    return 0;
}

float CMotionDetect::calculate_sensitivity(const std::vector<Common::RectInfo_S> &vstRectsInfo, int &nMaxAreaIndex, int nWidth, int nHeight)
{
    // float fSensitivity = 0.0;
    int nMaxArea = 0;
    int nArea = 0;
    for(unsigned int i = 0; i < vstRectsInfo.size(); i++)
    {
        nArea = (vstRectsInfo[i].nX2 - vstRectsInfo[i].nX1) * (vstRectsInfo[i].nY2 - vstRectsInfo[i].nY1);
        if(nArea > nMaxArea)
        {
            nMaxArea = nArea;
            nMaxAreaIndex = i;
        }
    }
    // fSensitivity = (float)nMaxArea / (m_stRect.nWidth * m_stRect.nHeight);

    return (float)nMaxArea / (nWidth * nHeight);
}

/* 安全校验并修正 ROI 函数*/
static cv::Rect CheckAndFixRect(const Common::Rect_S &srcRect, int imgWidth, int imgHeight)
{
    cv::Rect outRect;
    outRect.x = srcRect.nX;
    outRect.y = srcRect.nY;
    outRect.width = srcRect.nWidth;
    outRect.height = srcRect.nHeight;

    // 左上角校验
    if (outRect.x < 0) { outRect.x = 0; }
    if (outRect.y < 0) { outRect.y = 0; }

    // 宽高越界校验
    if (outRect.x + outRect.width > imgWidth)
    {
        outRect.width = imgWidth - outRect.x;
    }
    if (outRect.y + outRect.height > imgHeight)
    {
        outRect.height = imgHeight - outRect.y;
    }

    // 最小尺寸保护
    if (outRect.width <= 0) outRect.width = 1;
    if (outRect.height <= 0) outRect.height = 1;

    return outRect;
}

/* 线程函数 */
void CMotionDetect::run()
{
    MediaData_S stMediaData;

    while (m_bRunning.load())
    {
        if (!m_pMotionDetHandle)
        {
            if (!init())
            {
                dlog_error("等待移动侦测初始化");
                /* 延迟等待 1s */
                std::unique_lock<std::mutex> lock(m_mutex);
                m_condition.wait_for(lock, std::chrono::seconds(1), [this] {
                    return !m_bRunning.load();
                });
            }
            continue;
        }
        /* 阻塞获取 */
        m_dateQueue.pop(stMediaData, -1);
        if (stMediaData.nSize == 0 || !stMediaData.pData)
        {
            continue;
        }
        CStatisticsTimer runTime("移动侦测完整耗时");

        frameRate("移动侦测-分析数据", 5);
        MoveDetect_NS::InData_S stInData;
        std::vector<std::vector<int>> stOutData;

        int sw = stMediaData.stMediaParam.nVideoWidth;
        int sh = stMediaData.stMediaParam.nVideoHeight;
        
        // 确定目标尺寸和裁剪参数
        int cx = 0, cy = 0, cw = sw, ch = sh;
        int dw = sw, dh = sh;

        if (m_bIsCrop)
        {
            cv::Rect safeCrop = CheckAndFixRect(m_stRect, sw, sh);
            cx = (safeCrop.x >> 1) << 1;
            cy = (safeCrop.y >> 1) << 1;
            cw = (safeCrop.width >> 2) << 2;  // 宽度强制 4 对齐，防止RGA Stride 报错
            ch = (safeCrop.height >> 1) << 1; // 高度保持偶数
            if (cx + cw > sw) cw = (sw - cx) & ~3; // 剩余宽度也要 4 对齐
            if (cy + ch > sh) ch = (sh - cy) & ~1; // 剩余高度保持偶数
            // 目标 Mat 的大小直接设为裁剪后的大小
            dw = cw;
            dh = ch;
        }
        else
        {
            // 对齐校验,宽度强制 4 对齐,高度保持偶数
            dw = (sw >> 2) << 2;
            dh = (sh >> 1) << 1;
        }

        // 预分配/复用内存
        if (dw > 0 && dh > 0) 
        {
            m_fullRgbMat.create(dh, dw, CV_8UC3);
        } 
        else 
        {
            dlog_error("计算得到的有效宽度或高度为0，请检查裁剪参数");
            continue; 
        }

        // 调用 RGA 硬件处理转换
        bool rga_ok = rga_image_transform(
            stMediaData.pData.get(), sw, sh, RK_FORMAT_YCbCr_420_SP,   // 源：NV12
            m_fullRgbMat.data, dw, dh, RK_FORMAT_RGB_888,             // 目：RGB
            cx, cy, cw, ch,                                                              // 裁剪
            0                                                                 // 旋转
        );

        // RGA 失败，回退CPU 处理
        if (!rga_ok)
        {
            dlog_warn("RGA 转换失败，回退至 CPU 处理");
            cv::Mat i420Mat(sh * 3 / 2, sw, CV_8UC1, stMediaData.pData.get());
            cv::Mat tempRgb;
            cv::cvtColor(i420Mat, tempRgb, cv::COLOR_YUV2RGB_NV12);
            if (m_bIsCrop)
            {
                cv::Rect safeCrop = CheckAndFixRect(m_stRect, sw, sh);
                m_fullRgbMat = tempRgb(safeCrop).clone(); 
            }
            else
            {
                m_fullRgbMat = tempRgb;
            }
        }

        stInData.inMat = m_fullRgbMat;

        if (!stInData.inMat.empty())
        {
            // Debug
            if (access("/debugMotionImage", F_OK) == 0)
            {
                uint64_t llTime = times(nullptr) * 10;
                std::string strFileName = "/mnt/algo/motion/" + std::to_string(llTime) + ".jpg";
                cv::imwrite(strFileName, stInData.inMat);
            }

            /* 分析数据 */
            {  
                std::unique_lock<std::mutex> lock(m_mutex);
                if (!m_pMotionDetHandle->process(stInData, stOutData))
                {
                    dlog_error("移动侦测处理失败");
                }
                else 
                {
                    std::vector<Common::RectInfo_S> vstRectsInfo;
                    rect_to_point(stOutData, vstRectsInfo);

                    /*获取算法内部使用的限制分辨率*/
                    int algLimitW = m_pMotionDetHandle->getLimitWidth(); 
                    int algLimitH = m_pMotionDetHandle->getLimitHeight();

                    /*执行坐标映射*/
                    /*当设置了限制分辨率，且限制分辨率与原图不一致时才转换*/
                    if (algLimitW > 0 && algLimitH > 0 && 
                        (stInData.inMat.cols != algLimitW || stInData.inMat.rows != algLimitH))
                    {
                        for (size_t k = 0; k < vstRectsInfo.size(); k++)
                        {
                            auto &rect = vstRectsInfo[k];
                            rect.ConvertResolution(algLimitW, algLimitH, stInData.inMat.cols, stInData.inMat.rows);
                            
                            if (rect.nX2 > stInData.inMat.cols) rect.nX2 = stInData.inMat.cols;
                            if (rect.nY2 > stInData.inMat.rows) rect.nY2 = stInData.inMat.rows;
                            if (rect.nX1 < 0) rect.nX1 = 0;
                            if (rect.nY1 < 0) rect.nY1 = 0;
                        }
                    }

                    if(m_stMotionDetCfg.enMode == 0)    /* 普通模式 */
                    {
                        /* 调用普通模式处理函数 */
                        processNormalMode(vstRectsInfo);
                    }
                    else if(m_stMotionDetCfg.enMode == 1)   /* 专家模式 */
                    {
                        /* 调用专家模式处理函数 */
                        processExpertMode(vstRectsInfo);
                    }
                }
            }
        }
        else
        {
            dlog_error("ai_app: 图片数据为空");
        }
    }
}

bool CMotionDetect::isDaytime() const
{
    if (m_stMotionDetCfg.stMotionExpertMode.nExpertDayNightCtrl == 0)
    {
        /* 关闭日夜切换，始终返回false（使用关闭时的灵敏度） */
        return false;
    }
    else if (m_stMotionDetCfg.stMotionExpertMode.nExpertDayNightCtrl == 1)
    {
        /* 自动切换 - 这里需要根据光线传感器或其他硬件信息判断 */
        /* 获取白天黑夜状态 */
        /* note: 只读取共享层最后一次成功状态，不重新启动独立日夜检测或硬件控制。 */
        return CDayNightController::instance()->is_night_mode();
    }
    else if (m_stMotionDetCfg.stMotionExpertMode.nExpertDayNightCtrl == 2)
    {
        /* 定时切换 */
        /*自当天开始的秒数*/
        int nCurrentTime = TimeUtils_NS::getSecondsSinceStartOfDay();
        /* 将开始时间和结束时间转换为秒 */
        int nStartTime = m_stMotionDetCfg.stMotionExpertMode.stDayTime.stStart.nHour * 3600 + m_stMotionDetCfg.stMotionExpertMode.stDayTime.stStart.nMinute * 60 + m_stMotionDetCfg.stMotionExpertMode.stDayTime.stStart.nSecond;
        int nEndTime = m_stMotionDetCfg.stMotionExpertMode.stDayTime.stStop.nHour * 3600 + m_stMotionDetCfg.stMotionExpertMode.stDayTime.stStop.nMinute * 60 + m_stMotionDetCfg.stMotionExpertMode.stDayTime.stStop.nSecond;

        return (nCurrentTime >= nStartTime && nCurrentTime <= nEndTime);
    }

    return false;
}


int CMotionDetect::calculateOverlapArea(const Common::Rect_S &rect1, const Common::Rect_S &rect2) const
{
    int left = std::max(rect1.nX, rect2.nX);
    int top = std::max(rect1.nY, rect2.nY);
    int right = std::min(rect1.nX + rect1.nWidth, rect2.nX + rect2.nWidth);
    int bottom = std::min(rect1.nY + rect1.nHeight, rect2.nY + rect2.nHeight);

    if (left < right && top < bottom)
    {
        return (right - left) * (bottom - top);
    }

    return OK;
}

void CMotionDetect::processNormalMode(std::vector<Common::RectInfo_S> &vstRectsInfo)
{
    /* 处理检测到的矩形区域 */
    if (m_bIsCrop) /* 判断是否需要换算结果坐标至算法默认分辨率 */
    {
        for (int j = 0; j < vstRectsInfo.size(); j++)
        {
            vstRectsInfo[j].nX1 += m_stRect.nX;
            vstRectsInfo[j].nY1 += m_stRect.nY;

            vstRectsInfo[j].nX2 += m_stRect.nX;
            vstRectsInfo[j].nY2 += m_stRect.nY;
        }
    }
    /* 打印输出数据 */
    if (!access("testPrint", F_OK))
    {
        printResult(vstRectsInfo);
    }
    /* 是否报警 */
    bool bIsAlarm = false;
    int nMaxAreaIndex = 0;

    /* 灵敏度判断：将用户配置的[0,100]转换为[0,1]进行比较 */
    float fSensitivityThreshold = 1 - m_stMotionDetCfg.stMotionNormalMode.nSensitivity / 100.0f;

    /*  检查是否满足报警触发条件 */
    if (fSensitivityThreshold < calculate_sensitivity(vstRectsInfo, nMaxAreaIndex, m_stRect.nWidth, m_stRect.nHeight))
    {
        bIsAlarm = true;
        if (!access("testPrint", F_OK))
        {
            dlog_debug("[移动侦测] 灵敏度: %f > %f", fSensitivityThreshold, m_stMotionDetCfg.stMotionNormalMode.nSensitivity);
        }
        /* 动态分析 */
        if (m_stMotionDetCfg.bDynamicAnalysisEnable)
        {
            /* 发送结果至OSD模块，进行框选显示 */
            send_detectionResult_to_osd(m_nWidth, m_nHeight, vstRectsInfo[nMaxAreaIndex]);
        }
    }

    /* 判断是否报警 */
#ifdef ENABLE_TVSDK_SRC
    /* 构建事件触发上下文 */
    EventTriggerContext_S stContext;
    stContext.enEventType = Event::Type_E::MOTION_DETECT;
    stContext.nChnId = m_nChannelId;
    stContext.llTimestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
                               std::chrono::system_clock::now().time_since_epoch())
                               .count();

    if (bIsAlarm && !m_fullRgbMat.empty())
    {
        auto pPayload = std::make_shared<EventTvSdkPayload_S>();
        pPayload->enType = get_tvsdk_payload_type(stContext.enEventType);
        if (encode_mat_to_tvsdk_image(m_fullRgbMat, pPayload->stPanoramaImage))
        {
            stContext.pTvSdkPayload = pPayload;
        }
    }

    m_motionAlarmStateMachine.handleAlarmState(bIsAlarm, stContext);
#else
    m_motionAlarmStateMachine.handleAlarmState(bIsAlarm, Event::Type_E::MOTION_DETECT);
#endif
}

Common::RectInfo_S CMotionDetect::intersectRect(const Common::RectInfo_S& r1, const Common::RectInfo_S& r2)
{
    Common::RectInfo_S out;
    out.nX1 = 0;
    out.nY1 = 0;
    out.nX2 = 0;
    out.nY2 = 0;

    /* 求交集矩形 */
    int ix_min = std::max(r1.nX1, r2.nX1);
    int iy_min = std::max(r1.nY1, r2.nY1);
    int ix_max = std::min(r1.nX2, r2.nX2);
    int iy_max = std::min(r1.nY2, r2.nY2);

    /* 没有交集 返回全 0 */
    if (ix_min >= ix_max || iy_min >= iy_max) 
    {
        return out;
    }

    out.nX1 = ix_min;
    out.nY1 = iy_min;
    out.nX2 = ix_max;
    out.nY2 = iy_max;

    return out;
}

void CMotionDetect::processExpertMode(std::vector<Common::RectInfo_S> &stRectInfo)
{
    /* 打印输出数据 */
    if (!access("testPrint", F_OK))
    {
        printResult(stRectInfo);
    }

    /* 判断当前是白天还是夜晚 true：白天 false：夜晚 */
    bool bIsDaytime = isDaytime();

    /* 存储每个配置区域的触发状态 */
    std::vector<bool> vbRegionTriggered(m_stMotionDetCfg.stMotionExpertMode.vstMotionRegion.size(), false);
    std::vector<float> vfRegionSensitivity(m_stMotionDetCfg.stMotionExpertMode.vstMotionRegion.size(), 0.0f);
    /* 各个区域最大的矩形 */
    std::vector<Common::RectInfo_S> vstRegionMaxRectInfo;

    if (!m_stMotionDetCfg.stMotionExpertMode.vstMotionRegion.empty())
    {
        /* 遍历每个配置的侦测区域 */
        for (size_t configIdx = 0; configIdx < m_stMotionDetCfg.stMotionExpertMode.vstMotionRegion.size(); configIdx++)
        {
            const auto &configRegion = m_stMotionDetCfg.stMotionExpertMode.vstMotionRegion[configIdx];

            /* 获取当前应使用的灵敏度阈值 */
            float fSensitivityThreshold;
            if (m_stMotionDetCfg.stMotionExpertMode.nExpertDayNightCtrl == 0)
            {
                /* 关闭日夜切换，使用关闭时的灵敏度 */
                fSensitivityThreshold = 1.0f - configRegion.nCloseSensitivity / 100.0f;
            }
            else if (bIsDaytime)
            {
                /* 白天灵敏度 */
                fSensitivityThreshold = 1.0f - configRegion.nDaytimeSensitivity / 100.0f;
            }
            else
            {
                /* 夜晚灵敏度 */
                fSensitivityThreshold = 1.0f - configRegion.nNightSensitivity / 100.0f;
            }

            /* 计算配置区域面积 */
            int nConfigAreaSize = configRegion.stRect.nWidth * configRegion.stRect.nHeight;
            if (nConfigAreaSize <= 0)
            {
                continue;
            }

            /* 计算该配置区域内的灵敏度 */
            float fRegionSensitivityThreshold = 0.0;

            /* 遍历算法输出的所有移动区域 */
            Common::RectInfo_S stConfigRegionRectInfo;
            stConfigRegionRectInfo.nX1 = configRegion.stRect.nX;
            stConfigRegionRectInfo.nX2 = configRegion.stRect.nX + configRegion.stRect.nWidth;
            stConfigRegionRectInfo.nY1 = configRegion.stRect.nY;
            stConfigRegionRectInfo.nY2 = configRegion.stRect.nY + configRegion.stRect.nHeight;
            std::vector<Common::RectInfo_S> vstMotionRect;
            int nMaxAreaIndex = 0;
            for (size_t i = 0; i < stRectInfo.size(); i++)
            {
                /* 将算法输出的矩形求出与侦测区域重叠的矩形 */
                Common::RectInfo_S motionRect = intersectRect(stConfigRegionRectInfo, stRectInfo[i]);
                if( (motionRect.nX2 - motionRect.nX1) && (motionRect.nY2 - motionRect.nY1) )
                {
                    vstMotionRect.push_back(motionRect);
                }
            }
            
            if(vstMotionRect.size() > 0)   
            {
                /* 计算该配置区域的灵敏度（最大重叠面积占配置区域面积的比例） */
                fRegionSensitivityThreshold = calculate_sensitivity(vstMotionRect, nMaxAreaIndex, configRegion.stRect.nWidth, configRegion.stRect.nHeight);
                /* 记录当前区域最大的重叠面积 */
                vstRegionMaxRectInfo.push_back(vstMotionRect[nMaxAreaIndex]);
            }

            vfRegionSensitivity[configIdx] = fRegionSensitivityThreshold;

            /* 判断是否触发报警 */
            if (fRegionSensitivityThreshold > fSensitivityThreshold)
            {
                vbRegionTriggered[configIdx] = true;
                dlog_debug("[移动侦测 专家模式] 区域[%d] 触发灵敏度[%.3f] > [%.3f] 日夜模式:[%s]",
                           configRegion.nAreaNo,
                           fRegionSensitivityThreshold,
                           fSensitivityThreshold,
                           bIsDaytime ? "白天" : "夜晚");
            }
        }
    }

    /* 检查是否有任何区域触发报警 */
    bool anyRegionTriggered = false;
    for (bool triggered : vbRegionTriggered)
    {
        if (triggered)
        {
            anyRegionTriggered = true;
            break;
        }
    }

    if (anyRegionTriggered)
    {
        /* 动态分析 */
        if (m_stMotionDetCfg.bDynamicAnalysisEnable)
        {
            /* 发送结果至OSD模块，进行框选显示 */
            /* 只显示最大的区域 */
            send_detectionResult_to_osd(m_nWidth, m_nHeight, vstRegionMaxRectInfo);
        }
    }

    /* 判断是否报警 */
#ifdef ENABLE_TVSDK_SRC
    /* 构建事件触发上下文 */
    EventTriggerContext_S stContext;
    stContext.enEventType = Event::Type_E::MOTION_DETECT;
    stContext.nChnId = m_nChannelId;
    stContext.llTimestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
                               std::chrono::system_clock::now().time_since_epoch())
                               .count();

    if (anyRegionTriggered && !m_fullRgbMat.empty())
    {
        auto pPayload = std::make_shared<EventTvSdkPayload_S>();
        pPayload->enType = get_tvsdk_payload_type(stContext.enEventType);
        if (encode_mat_to_tvsdk_image(m_fullRgbMat, pPayload->stPanoramaImage))
        {
            stContext.pTvSdkPayload = pPayload;
        }
    }

    m_motionAlarmStateMachine.handleAlarmState(anyRegionTriggered, stContext);
#else
    m_motionAlarmStateMachine.handleAlarmState(anyRegionTriggered, Event::Type_E::MOTION_DETECT);
#endif
}
