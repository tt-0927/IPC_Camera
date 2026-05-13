/**
 * @FilePath     : parking_detect.cpp
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2025-07-10 11:28:50
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2025-12-29 17:02:57
 * @Description  : 停车侦测
 */

#include "parking_detect.hpp"

/* 数据队列 */
#define QUEUE_MAX (2)

CParkingDetect::CParkingDetect()
    : m_dateQueue(QUEUE_MAX)
{
    m_bRunning.store(true);
    m_thread = std::thread(&CParkingDetect::run, this);
}

CParkingDetect::~CParkingDetect()
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

void CParkingDetect::recvMediaData(MediaData_S stMediaData)
{
    if (!m_stAlgoParkDetCfg.bEnable)
    {
        return;
    }

    if (m_RecvManager.handleEvent(stMediaData.stMediaParam.nChannel))
    {
        if (m_dateQueue.size() >= QUEUE_MAX)
        {
            dlog_error("停车侦测-数据队列满了 [%d]", m_dateQueue.size());
        }
        m_dateQueue.pushOrReplace(stMediaData);
    }
}

void CParkingDetect::setAlgoEnCfg(const Event::AlgorithmConfig &stAlgoConfig)
{
    m_stAlgoParkDetCfg.bEnable = stAlgoConfig.nEnParkingDetect;

    if (m_stAlgoParkDetCfg.bEnable)
    {
        Alarm::ParkingDetection_S stInfo;
        CEventConfigure::instance()->get_configure(stInfo);
        setAlgoParamCfg(stInfo);
    }
}

void CParkingDetect::setAlgoParamCfg(const Alarm::ParkingDetection_S &stAlgoCfg)
{
    dlog_debug("ai_app: 设置停车侦测参数");
    m_stAlgoParkDetCfg = stAlgoCfg;
}

bool CParkingDetect::init()
{
    if (!m_pParkDetHandle)
    {
        std::string strModelPath = AI_PARKING_DETECTION_CONFIG_FILE;

        m_pParkDetHandle = new Inference_NS::CYoloUltralytics(strModelPath);
        if (m_pParkDetHandle)
        {
            if (m_pParkDetHandle->init())
            {
                dlog_info("停车侦测算法初始化成功, %s", strModelPath.c_str());
                return true;
            }
            else
            {
                delete m_pParkDetHandle;
                m_pParkDetHandle = nullptr;
                dlog_error("停车侦测算法初始化失败");
            }
        }
    }
    return false;
}

/* 反初始化 */
bool CParkingDetect::unInit()
{
    if (m_pParkDetHandle)
    {
        delete m_pParkDetHandle;
        m_pParkDetHandle = nullptr;
    }

    return true;
}

void CParkingDetect::run()
{
    pthread_setname_np(pthread_self(), "ParkingDetect");

    MediaData_S stMediaData;

    while (m_bRunning.load())
    {
        if (!m_pParkDetHandle)
        {
            if (!init())
            {
                dlog_error("等待停车侦测初始化");
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

        /* 送分析 */
        Inference_NS::InputData_S stInputData;
        stInputData.pData = (float *)pFrameInfo->video_frame.virt_addr[0];
        stInputData.nDataSize = static_cast<int>(m_nWidth * m_nHeight * 1.5) * sizeof(float);

        std::vector<Inference_NS::BoxData_S> vBoxDatas;

        /* 停车侦测 */
        m_pParkDetHandle->inference(stInputData, vBoxDatas);
        
        // {
        //     /* 打印输出数据 */
        //     printResult(vBoxDatas);
        //     /* 发送结果至OSD模块，进行框选显示 */
        //     send_detectionResult_to_osd(stMediaData.stMediaParam.nVideoWidth, stMediaData.stMediaParam.nVideoHeight, vBoxDatas);
        // }
    }
}
