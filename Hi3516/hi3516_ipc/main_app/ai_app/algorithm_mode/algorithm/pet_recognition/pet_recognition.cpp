/**
 * @FilePath     : pet_recognition.cpp
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2025-07-29 20:10:37
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2025-12-29 17:03:38
 * @Description  : 宠物识别
 */

#include "pet_recognition.hpp"
#include "video_frame_jpeg_encoder.hpp"

/* 数据队列 */
#define QUEUE_MAX (2)

CPetRecognition::CPetRecognition()
    : m_dateQueue(QUEUE_MAX)
{
    /* 启动线程 */
    m_bRunning.store(true);
    m_thread = std::thread(&CPetRecognition::run, this);
}

CPetRecognition::~CPetRecognition()
{
    /* 通知线程停止 */
    m_bRunning.store(false);
    // m_condition.notify_all();
    // note 调用 shutdown() 来唤醒可能阻塞在 pop() 的线程
    m_dateQueue.shutdown();
    if (m_thread.joinable())
    {
        m_thread.join();
    }
    m_dateQueue.clear();
    unInit();
}

void CPetRecognition::recvMediaData(MediaData_S stMediaData)
{
    if (!m_stPetDetCfg.bEnable)
    {
        return;
    }

    if (m_RecvManager.handleEvent(stMediaData.stMediaParam.nChannel))
    {
        if (m_dateQueue.size() >= QUEUE_MAX)
        {
            dlog_error("宠物识别-数据队列满了 [%d]", m_dateQueue.size());
        }
        m_dateQueue.pushOrReplace(stMediaData);
    }
}

void CPetRecognition::setAlgoEnCfg(const Event::AlgorithmConfig &stAlgoConfig)
{
    m_stPetDetCfg.bEnable = stAlgoConfig.nEnPetRecognition;

    if (m_stPetDetCfg.bEnable)
    {
        Alarm::PetRecognition_S stInfo;
        CEventConfigure::instance()->get_configure(stInfo);
        setAlgoParamCfg(stInfo);
    }
}

void CPetRecognition::setAlgoParamCfg(const Alarm::PetRecognition_S &stAlgoCfg)
{
    dlog_debug("ai_app: 设置宠物识别参数");
    m_stPetDetCfg = stAlgoCfg;
    /* 转换区域坐标分辨率至算法分辨率 */
    m_stPetDetCfg.stRegion.ConvertResolution(PIXEL_WIDTH_1920, PIXEL_HEIGHT_1080, m_nWidth, m_nHeight);
}

bool CPetRecognition::init()
{
    if (!m_pPetDetHandle)
    {
        m_pPetDetHandle = streamAiDetect_init(AI_DETECT_CHN_PET, AI_PET_NORMAL_MODEL_PATH);
        if (!m_pPetDetHandle)
        {
            dlog_error("宠物识别初始化失败");
            return false;
        }
        dlog_info("宠物识别初始化成功");
    }

    return true;
}

/* 反初始化 */
bool CPetRecognition::unInit()
{
    if (m_pPetDetHandle)
    {
        streamAiDetect_uninit(m_pPetDetHandle);
    }

    return true;
}

bool CPetRecognition::reboot()
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

void CPetRecognition::run()
{
    pthread_setname_np(pthread_self(), "PetDetect");

    /* 媒体信息 */
    MediaData_S stMediaData;

    while (m_bRunning.load())
    {
        if (!m_pPetDetHandle)
        {
            /* 没有算法使能，不进行算法初始化 */
            if (!m_stPetDetCfg.bEnable)
            {
                sleep(1);
                continue;
            }

            if (!init())
            {
                dlog_error("等待宠物识别初始化");
                /* 延迟等待 1s */
                std::this_thread::sleep_for(std::chrono::seconds(1));
                if (!m_bRunning.load())
                {
                    break;
                }
                continue;
            }
        }

        /* 阻塞获取 */
        if(!m_dateQueue.pop(stMediaData, TIMEOUT_1000_MS) || stMediaData.pVideoFrameInfo == nullptr)
        {
            continue;
        }

        /* 直接使用 stMediaData.pVideoFrameInfo，避免内存拷贝 */
        ot_video_frame_info *pFrameInfo = stMediaData.pVideoFrameInfo.get();
        if (!pFrameInfo)
        {
            dlog_error("原始数据帧为空");
            continue;
        }

        /* 宠物识别 */
        if (m_pPetDetHandle->svpAiDetect_sendFrame(m_pPetDetHandle, &pFrameInfo->video_frame) == TD_SUCCESS)
        {
            /* 打印输出数据 */
            if (!access("testPrint", F_OK))
            {
                m_pPetDetHandle->svpAiDetect_printResult(&m_pPetDetHandle->stResult);
            }

            /* 宠物识别 */
            if (m_stPetDetCfg.bEnable)
            {
                // double time = time_get_ms();
                SEventProcessContext stCtx;
                stCtx.nChnId = stMediaData.stMediaParam.nChannel;
                stCtx.llTimestamp = TimeUtils_NS::get_currentTimestampMs();
                stCtx.pFrameInfo = pFrameInfo;
                /* 宠物识别后处理函数 */
                processPetRecognition(m_pPetDetHandle->stResult, stCtx);
                // dlog_debug("宠物识别后处理函数耗时：%f", time_get_ms() - time);
            }
        }
    }
}

// info /*----------------------- 算法后处理 -----------------------*/

void CPetRecognition::processPetRecognition(ot_aidetect_result_array &stResult, const SEventProcessContext &stCtx)
{
    /* 宠物类目标算法分析的结果 */
    ot_aidetect_object_of_one_class *pstObjectClass = nullptr;
    for (size_t i = 0; i < stResult.class_num; i++)
    {
        if (stResult.object_class[i].class_type == OT_AIDETECT_CLASS_PET)
        {
            /* 获取宠物类目标算法分析的结果 */
            pstObjectClass = &stResult.object_class[i];
        }
    }

    /* 是否报警 */
    bool bIsAlarm = false;
    /* OSD 动态分析显示数组 */
    std::vector<Common::RectInfo_S> vstRectInfo;

    /* 未检测出宠物类结果 */
    if (pstObjectClass)
    {
        /* 宠物识别配置的灵敏度阈值 */
        float fSensitivityThreshold = 1.0f - m_stPetDetCfg.nSensitivity / 100.0f;
        /* 遍历出检测到的目标个数 */
        for (size_t i = 0; i < pstObjectClass->object_num; i++)
        {
            /* 判断灵敏度 */
            if (pstObjectClass->objects[i].detect_confidence >= fSensitivityThreshold)
            {
                /* 判断识别结果是否在检测框内 */
                if (is_in_region(m_stPetDetCfg.stRegion, pstObjectClass->objects[i]))
                {
                    /* 宠物识别 动态分析 */
                    if (m_stPetDetCfg.bDynamicAnalysisEnable)
                    {
                        Common::RectInfo_S stInfo;
                        stInfo.nX1 = pstObjectClass->objects[i].detect_rect.x;
                        stInfo.nY1 = pstObjectClass->objects[i].detect_rect.y;
                        stInfo.nX2 = pstObjectClass->objects[i].detect_rect.width + stInfo.nX1;
                        stInfo.nY2 = pstObjectClass->objects[i].detect_rect.height + stInfo.nY1;
                        vstRectInfo.emplace_back(stInfo);
                    }
                    bIsAlarm = true;
                }
            }
        }
    }

    /* 判断是否报警 */
    if (bIsAlarm)
    {
        /* 宠物识别 动态分析 */
        if (m_stPetDetCfg.bDynamicAnalysisEnable)
        {
            /* 发送结果至OSD模块，进行框选显示 */
            send_detectionResult_to_osd(m_nWidth, m_nHeight, vstRectInfo);
        }
    }
    EventTriggerContext_S stContext;
    stContext.enEventType = Event::Type_E::PET_RECOGNITION;
    stContext.nChnId = stCtx.nChnId;
    stContext.llTimestamp = stCtx.llTimestamp;
#ifdef ENABLE_TVSDK_SRC
    if (bIsAlarm && stCtx.pFrameInfo != nullptr)
    {
        auto pPayload = std::make_shared<EventTvSdkPayload_S>();
        pPayload->enType = get_tvsdk_payload_type(stContext.enEventType);
        if (AiAppCommon::encode_video_frame_to_jpeg_memory(stCtx.pFrameInfo, pPayload->stPanoramaImage) == OK)
        {
            stContext.pTvSdkPayload = pPayload;
        }
    }
#endif
    m_petAlarmStateMachine.handleAlarmState(bIsAlarm, stContext);
}
