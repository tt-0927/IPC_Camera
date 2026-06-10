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
#include "time_utils.h"

namespace
{
/* 数据队列容量，保持和旧实现一致，满队列时用最新帧替换旧帧 */
constexpr int FACE_DETECT_QUEUE_MAX = 2;
}

CFaceDetect::CFaceDetect() : m_dateQueue(FACE_DETECT_QUEUE_MAX)
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
    bool bNeedAlgo = bEnableFaceCapture || bEnableFaceCompare;
    if (bNeedAlgo)
    {
        if (!m_detectWorker.isRunning())
        {
            dlog_info("启动FaceDetectWorker");

            m_detectWorker.start();
        }
    }
    else
    {
        /*
         * 关闭worker
         */
        dlog_info("关闭FaceDetectWorker");

        m_detectWorker.deinit();
    }
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
    bool bTempStart = false;

    /*
     * 没启动则临时启动
     */

    if (!m_detectWorker.isRunning())
    {

        if (!m_detectWorker.start())
        {
            return false;
        }

        bTempStart = true;
    }

    bool bRet = m_featureProcessor.addFaceLibGroup(stFaceLibData, m_detectWorker, m_nWidth, m_nHeight);

    /*
     * 临时启动的则关闭
     */
    if (bTempStart)
    {
        m_detectWorker.deinit();
    }

    return bRet;
}

bool CFaceDetect::init()
{

    if (0 == m_stDstFrameInfo.video_frame.width)
    {
        memset_s(&m_stDstFrameInfo, sizeof(ot_video_frame_info), 0, sizeof(ot_video_frame_info));

        if (TD_SUCCESS !=
            mppVgs_create_video_frame_info(m_nWidth, m_nHeight, OT_PIXEL_FORMAT_YVU_SEMIPLANAR_420, &m_stDstFrameInfo))
        {
            dlog_error("创建目标视频帧失败");

            return false;
        }
    }

    /*
     * 初始化特征模型
     */
    if (m_featureProcessor.isEnabled() && !m_featureProcessor.isInitialized() && !m_featureProcessor.init())
    {
        return false;
    }

    return true;
}

bool CFaceDetect::unInit()
{
    mppVgs_destroy_video_frame_info(&m_stDstFrameInfo);
    memset_s(&m_stDstFrameInfo, sizeof(ot_video_frame_info), 0, sizeof(ot_video_frame_info));
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
        if (!hasEnabledAlgorithm())
        {
            sleep(1);

            continue;
        }

        /*
         * 初始化检测线程
         */
        if (!init())
        {
            dlog_error("等待人脸检测初始化");

            std::this_thread ::sleep_for(std::chrono ::seconds(1));

            continue;
        }

        /*
         * 取视频帧
         */
        if (!m_dateQueue.pop(stMediaData, TIMEOUT_1000_MS))
        {
            continue;
        }

        if (!stMediaData.pVideoFrameInfo)
        {
            continue;
        }

        /*
         * 原始帧
         */
        ot_video_frame_info *pSrcFrameInfo = stMediaData.pVideoFrameInfo.get();

        if (!pSrcFrameInfo)
        {
            continue;
        }

        /*
         * 是否需要缩放
         */
        bool bIsScale = false;

        if (m_nWidth != stMediaData.stMediaParam.nVideoWidth || m_nHeight != stMediaData.stMediaParam.nVideoHeight)
        {
            bIsScale = true;
        }

        /*
         * 算法输入帧
         */
        ot_video_frame_info *pFrameInfo = pSrcFrameInfo;

        /*
         * 缩放到检测分辨率
         */
        if (bIsScale)
        {
            if (TD_SUCCESS != mppVgs_scale(pSrcFrameInfo, &m_stDstFrameInfo))
            {
                dlog_error("mppVgs_scale失败");

                continue;
            }

            pFrameInfo = &m_stDstFrameInfo;
        }

        /*
         * 创建异步独占frame
         *
         * 防止：
         * 1. m_stDstFrameInfo被覆盖
         * 2. MediaData释放
         * 3. worker线程访问野指针
         */
        ot_video_frame_info *pAsyncFrame = new ot_video_frame_info;

        memset(pAsyncFrame, 0, sizeof(ot_video_frame_info));

        if (TD_SUCCESS != mppVgs_create_video_frame_info(m_nWidth, m_nHeight, OT_PIXEL_FORMAT_YVU_SEMIPLANAR_420, pAsyncFrame))
        {
            dlog_error("创建异步frame失败");

            delete pAsyncFrame;

            continue;
        }

        /*
         * 拷贝NV21数据
         */
        const size_t frameSize = static_cast<size_t>(m_nWidth * m_nHeight * 3 / 2);

        memcpy(pAsyncFrame->video_frame.virt_addr[0],

               pFrameInfo->video_frame.virt_addr[0],

               frameSize);

        /*
         * 投递异步检测任务
         *
         * 注意：
         * worker线程内部：
         *   detection
         *   feature
         * 已经串行
         */
        m_detectWorker.submitVideoFrame(
            pAsyncFrame,
            m_nWidth,
            m_nHeight,

            [this, pAsyncFrame, stMediaData](std::vector<Inference_NS ::PointData_S> vPointDatas)
            {
                /*
                 * 邮件附件列表
                 */
                std::vector<std::string> vecImageFile;

                /*
                 * OSD框
                 */
                std::vector<Common::RectInfo_S> vstRectInfo;

                /*
                 * 当前处理上下文
                 */
                FaceDetectInternal ::SFaceProcessContext stContext{
                    vPointDatas,
                    vstRectInfo,
                    pAsyncFrame,
                    m_nWidth,
                    m_nHeight,
                    stMediaData.stMediaParam.nChannel,
                    TimeUtils_NS::get_currentTimestampMs(),
                    &m_detectWorker
                };

                /*
                 * 人脸抓拍
                 */
                if (m_captureProcessor.isEnabled())
                {
                    m_captureProcessor.process(stContext, vecImageFile);
                }

                /*
                 * 人脸比对
                 */
                if (m_featureProcessor.isEnabled())
                {
                    std::vector<Common::RectInfo_S> vstCompareRectInfo;

                    // m_captureProcessor
                    //     .collectTargets(
                    //         vPointDatas,
                    //         vstCompareRectInfo);
                    m_featureProcessor.collectCompareTargets(vPointDatas, vstCompareRectInfo);

                    m_featureProcessor.processCompare(stContext, vstCompareRectInfo, m_captureProcessor, vecImageFile);
                }

                /*
                 * OSD显示
                 */
                if (m_captureProcessor.isEnabled() && !vstRectInfo.empty())
                {
                    send_detectionResult_to_osd(m_nWidth, m_nHeight, vstRectInfo);
                }

                /*
                 * 释放异步frame
                 */
                mppVgs_destroy_video_frame_info(pAsyncFrame);

                delete pAsyncFrame;
            });
    }
}

bool CFaceDetect::hasEnabledAlgorithm() const
{
    return m_captureProcessor.isEnabled() || m_featureProcessor.isEnabled();
}
