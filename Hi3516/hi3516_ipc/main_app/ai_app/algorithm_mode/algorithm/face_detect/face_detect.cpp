/**
 * @FilePath     : face_detect.cpp
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2025-06-06 16:02:10
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-04-28 16:20:20
 * @Description  : 人脸检测类实现，负责检测模型调度与 processor 编排
 */

#include "face_detect.hpp"

#include <unistd.h>

#include "event_configure.h"

namespace
{
/* 数据队列容量，保持和旧实现一致，满队列时用最新帧替换旧帧 */
constexpr int FACE_DETECT_QUEUE_MAX = 2;
}

CFaceDetect::CFaceDetect()
    : m_dateQueue(FACE_DETECT_QUEUE_MAX)
{
    memset_s(&m_stDstFrameInfo, sizeof(ot_video_frame_info), 0, sizeof(ot_video_frame_info));
    m_bRunning.store(true);
    m_thread = std::thread(&CFaceDetect::run, this);
}

CFaceDetect::~CFaceDetect()
{
    m_bRunning.store(false);
    m_dateQueue.shutdown();
    if (m_thread.joinable())
    {
        m_thread.join();
    }
    m_dateQueue.clear();
    unInit();
}

void CFaceDetect::recvMediaData(MediaData_S stMediaData)
{
    if (!hasEnabledAlgorithm())
    {
        return;
    }

    if (m_RecvManager.handleEvent(stMediaData.stMediaParam.nChannel))
    {
        if (m_dateQueue.size() >= FACE_DETECT_QUEUE_MAX)
        {
            dlog_error("人脸检测-数据队列满了 [%d]", m_dateQueue.size());
        }
        m_dateQueue.pushOrReplace(stMediaData);
    }
}

void CFaceDetect::setAlgoEnCfg(const Event::AlgorithmConfig &stAlgoConfig)
{
    /* 当前顶层开关是否使能人脸抓拍 */
    const bool bEnableFaceCapture = stAlgoConfig.nEnFaceCapture;
    /* 当前顶层开关是否使能人脸比对 */
    const bool bEnableFaceCompare = stAlgoConfig.nEnFaceCompare;

    if (bEnableFaceCapture || bEnableFaceCompare)
    {
        /* 人脸比对没有独立规则配置，因此需要始终加载抓拍规则作为前置过滤条件 */
        Alarm::FaceCapture_S stCaptureInfo;
        CEventConfigure::instance()->get_configure(stCaptureInfo);
        setAlgoParamCfg(stCaptureInfo);
    }
    else
    {
        m_stAlgoFaceCapCfg.bEnable = false;
        m_captureProcessor.setEnabled(false);
    }

    if (bEnableFaceCompare)
    {
        Alarm::FaceCompare_S stCompareInfo;
        CEventConfigure::instance()->get_configure(stCompareInfo);
        setFaceCmpCfg(stCompareInfo);
    }
    else
    {
        m_stAlgoFaceCompCfg.bEnable = false;
        m_featureProcessor.setEnabled(false);
    }

    /* 顶层算法开关与具体业务配置同时生效，避免单侧关闭后仍进入处理链路 */
    m_stAlgoFaceCapCfg.bEnable = m_stAlgoFaceCapCfg.bEnable && bEnableFaceCapture;
    m_captureProcessor.setEnabled(m_stAlgoFaceCapCfg.bEnable);
    m_stAlgoFaceCompCfg.bEnable = m_stAlgoFaceCompCfg.bEnable && bEnableFaceCompare;
    m_featureProcessor.setEnabled(m_stAlgoFaceCompCfg.bEnable);
}

void CFaceDetect::setAlgoParamCfg(const Alarm::FaceCapture_S &stAlgoCfg)
{
    m_stAlgoFaceCapCfg = stAlgoCfg;
    m_RecvManager.set_time_window(stAlgoCfg.stRule.nInterval * 1000);
    m_captureProcessor.setAlgoParamCfg(stAlgoCfg, m_nWidth, m_nHeight);
}

void CFaceDetect::setFaceCmpCfg(const Alarm::FaceCompare_S &stAlgoCfg)
{
    m_stAlgoFaceCompCfg = stAlgoCfg;
    m_featureProcessor.setAlgoParamCfg(stAlgoCfg);
}

bool CFaceDetect::addFaceLibGroup(FaceDataDB_NS::FaceLibsInfo_S &stFaceLibData)
{
    dlog_info("=== [FaceLib] 开始添加人脸库===");
    if (!m_pFaceDetHandle)
    {
        std::string strModelPath = AI_FACE_DETECTION_CONFIG_FILE;
        m_pFaceDetHandle = new Inference_NS::CYoloUltralyticsPoint(strModelPath);
        if (!m_pFaceDetHandle || !m_pFaceDetHandle->init())
        {
            dlog_error("检测模型初始化失败");
            delete m_pFaceDetHandle;
            m_pFaceDetHandle = nullptr;
            return false;
        }
    }

    return m_featureProcessor.addFaceLibGroup(stFaceLibData,
                                              m_pFaceDetHandle,
                                              m_npuMutex,
                                              m_nWidth,
                                              m_nHeight);
}

bool CFaceDetect::init()
{
    if (!m_pFaceDetHandle)
    {
        std::string strModelPath = AI_FACE_DETECTION_CONFIG_FILE;
        m_pFaceDetHandle = new Inference_NS::CYoloUltralyticsPoint(strModelPath);
        if (!m_pFaceDetHandle || !m_pFaceDetHandle->init())
        {
            delete m_pFaceDetHandle;
            m_pFaceDetHandle = nullptr;
            dlog_error("人脸检测初始化失败");
            return false;
        }

        dlog_info("人脸检测初始化成功, %s", strModelPath.c_str());
        memset_s(&m_stDstFrameInfo, sizeof(ot_video_frame_info), 0, sizeof(ot_video_frame_info));
        if (TD_SUCCESS != mppVgs_create_video_frame_info(m_nWidth,
                                                         m_nHeight,
                                                         OT_PIXEL_FORMAT_YVU_SEMIPLANAR_420,
                                                         &m_stDstFrameInfo))
        {
            delete m_pFaceDetHandle;
            m_pFaceDetHandle = nullptr;
            dlog_error("人脸检测初始化失败-创建目标视频帧失败");
            return false;
        }
    }

    if (m_featureProcessor.isEnabled() && !m_featureProcessor.isInitialized() && !m_featureProcessor.init())
    {
        return false;
    }

    return m_pFaceDetHandle != nullptr;
}

bool CFaceDetect::unInit()
{
    mppVgs_destroy_video_frame_info(&m_stDstFrameInfo);
    memset_s(&m_stDstFrameInfo, sizeof(ot_video_frame_info), 0, sizeof(ot_video_frame_info));

    if (m_pFaceDetHandle)
    {
        delete m_pFaceDetHandle;
        m_pFaceDetHandle = nullptr;
    }

    m_featureProcessor.deinit();
    return true;
}

void CFaceDetect::run()
{
    pthread_setname_np(pthread_self(), "FaceDetect");

    /* 当前线程从队列取出的媒体数据 */
    MediaData_S stMediaData;
    /* 邮件联动的附件图片路径列表，按单帧复用，处理完成后立即清空 */
    std::vector<std::string> vecImageFile;

    while (m_bRunning.load())
    {
        if (!m_pFaceDetHandle || (m_featureProcessor.isEnabled() && !m_featureProcessor.isInitialized()))
        {
            if (!hasEnabledAlgorithm())
            {
                sleep(1);
                continue;
            }

            if (!init())
            {
                dlog_error("等待人脸检测初始化");
                std::this_thread::sleep_for(std::chrono::seconds(1));
                if (!m_bRunning.load())
                {
                    break;
                }
                continue;
            }
        }

        if (!m_dateQueue.pop(stMediaData, TIMEOUT_1000_MS) || stMediaData.pVideoFrameInfo == nullptr)
        {
            continue;
        }

        /* 当前帧原始视频帧指针 */
        ot_video_frame_info *pSrcFrameInfo = stMediaData.pVideoFrameInfo.get();
        if (!pSrcFrameInfo)
        {
            dlog_error("原始数据帧为空");
            continue;
        }

        /* 当前帧是否需要缩放到算法分辨率 */
        bool bIsScale = false;
        if (m_nWidth != stMediaData.stMediaParam.nVideoWidth || m_nHeight != stMediaData.stMediaParam.nVideoHeight)
        {
            bIsScale = true;
        }

        /* 当前送算法的视频帧指针，必要时指向缩放后的目标帧 */
        ot_video_frame_info *pFrameInfo = pSrcFrameInfo;
        if (bIsScale)
        {
            if (TD_SUCCESS != mppVgs_scale(pSrcFrameInfo, &m_stDstFrameInfo))
            {
                continue;
            }
            pFrameInfo = &m_stDstFrameInfo;
        }

        Inference_NS::InputData_S stInputData;
        stInputData.pData = reinterpret_cast<float *>(pFrameInfo->video_frame.virt_addr[0]);
        stInputData.nDataSize = static_cast<int>(m_nWidth * m_nHeight * 1.5) * sizeof(float);

        /* 当前帧人脸检测模型输出结果 */
        std::vector<Inference_NS::PointData_S> vPointDatas;
        {
            std::lock_guard<std::mutex> lock(m_npuMutex);
            m_pFaceDetHandle->inference(stInputData, vPointDatas);
        }

        /* 当前帧汇总角框输出数组，仅供抓拍联动与 OSD 显示复用 */
        std::vector<Common::RectInfo_S> vstRectInfo;
        FaceDetectInternal::SFaceProcessContext stContext{ vPointDatas,
                                                           vstRectInfo,
                                                           pFrameInfo,
                                                           m_nWidth,
                                                           m_nHeight,
                                                           stMediaData.stMediaParam.nChannel,
                                                           &m_npuMutex };

        if (m_captureProcessor.isEnabled())
        {
            m_captureProcessor.process(stContext, vecImageFile);
        }

        if (m_featureProcessor.isEnabled())
        {
            /* 人脸比对沿用抓拍规则做前置过滤，即便抓拍功能关闭也仍可复用该过滤逻辑 */
            std::vector<Common::RectInfo_S> vstCompareRectInfo;
            m_captureProcessor.collectTargets(vPointDatas, vstCompareRectInfo);
            m_featureProcessor.processCompare(stContext, vstCompareRectInfo, m_captureProcessor, vecImageFile);
        }

        if (m_captureProcessor.isEnabled() && !vstRectInfo.empty())
        {
            send_detectionResult_to_osd(m_nWidth, m_nHeight, vstRectInfo);
        }
    }
}

bool CFaceDetect::hasEnabledAlgorithm() const
{
    return m_captureProcessor.isEnabled() || m_featureProcessor.isEnabled();
}
