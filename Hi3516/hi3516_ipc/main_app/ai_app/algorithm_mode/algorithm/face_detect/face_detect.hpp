/**
 * @FilePath     : face_detect.hpp
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2025-06-06 16:02:10
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-04-28 20:23:02
 * @Description  : 人脸检测
 */

#pragma once

#include <atomic>
#include <array>
#include <chrono>
#include <algorithm>
#include <mutex>
#include <queue>
#include <thread>
#include <vector>
#include <sys/time.h>

#include "blocking_queue.hpp"
#include "algo_control_deal.h"
#include "algo_stream_deal.h"
#include "algorithm.hpp"
#include "common_process.h"
#include "YoloUltralyticsPoint_rpn.hpp"
#include "face_detect_context.hpp"
#include "face_capture_processor.hpp"
#include "face_feature_processor.hpp"
#include "face_detect_worker.hpp"
class CFaceDetect : public CAlgorithm
{
public:
    CFaceDetect();
    ~CFaceDetect();

    /**
     * @brief   : 接受媒体数据
     * @param    {MediaData_S} stMediaData：媒体数据
     */
    void recvMediaData(MediaData_S stMediaData) override;

    /**
     * @brief   : 更新算法配置参数
     * @param    {AlgorithmConfig} &stAlgoConfig：算法配置
     */
    void setAlgoEnCfg(const Event::AlgorithmConfig &stAlgoConfig) override;

    /**
     * @brief   : 更新人脸抓拍参数
     * @param    {FaceCapture_S} &stAlgoCfg：人脸侦测
     */
    void setAlgoParamCfg(const Alarm::FaceCapture_S &stAlgoCfg);

    /**
     * @brief   : 更新人脸比对联动
     * @param    {FaceCompare_S} &stAlgoCfg：人脸比对
     */
    void setFaceCmpCfg(const Alarm::FaceCompare_S &stAlgoCfg);

    /**
     * @brief 添加人脸名单库
     * @param stFaceLibData
     */
#if CAP_AI_FACE_COMPARE
    int addFaceLibGroup(FaceDataDB_NS::FaceLibsInfo_S &stFaceLibData);
#endif

private:
    /**
     * @brief 初始化
     * @return [*]
     * @note
     */
    bool init();

    /**
     * @brief 反初始化
     * @return [*]
     */
    bool unInit();

    /**
     * @brief 线程函数
     * @return [*]
     */
    void run();

    /**
     * @brief   : 判断当前是否存在使能的人脸业务
     * @return   {bool} true：至少有一项人脸业务使能 false：全部关闭
     */
    bool hasEnabledAlgorithm() const;

    bool initAsyncFramePool();
    void deinitAsyncFramePool();
    ot_video_frame_info *acquireAsyncFrame();
    void releaseAsyncFrame(ot_video_frame_info *pFrameInfo);


private:
    /* 人脸检测句柄 */
    // Inference_NS::CYoloUltralyticsPoint *m_pFaceDetHandle = nullptr;
    /* 队列 */
    BQ_NS::CBlockingQueue<MediaData_S> m_dateQueue;
    /* 用于控制线程的运行 */
    std::atomic<bool> m_bRunning;
    /* 数据获取线程 */
    std::thread m_thread;
    /* 检测频率控制 */
    EventManager m_RecvManager{ 3000 };
    /* 人脸抓拍 */
    Alarm::FaceCapture_S m_stAlgoFaceCapCfg;
    #if CAP_AI_FACE_COMPARE
    /* 人脸比对 */
    Alarm::FaceCompare_S m_stAlgoFaceCompCfg;
    #endif
    /* NPU 推理互斥锁，检测模型与特征模型切换上下文时共享 */
    // std::mutex m_npuMutex;

    float g_fSimilarity = 0.7;
    /* 算法默认最小瞳距 */
    int m_nMinIpd = 20;
    /* 算法最小瞳距范围 */
    const int MIN_IPD = 10;
    const int MAX_IPD = 75;
    /* 算法默认分辨率 */
    int m_nWidth = PIXEL_WIDTH_640;
    int m_nHeight = PIXEL_HEIGHT_640;
    // int m_nWidth = PIXEL_WIDTH_1024;
    // int m_nHeight = PIXEL_HEIGHT_576;
        /* 固定预申请异步帧池，避免运行中频繁申请和释放VB块。 */
    #if   CAP_IO_EXTERNAL_DDR_00S
    static constexpr size_t ASYNC_FRAME_POOL_SIZE = 5;
    #else
    static constexpr size_t ASYNC_FRAME_POOL_SIZE = 3;
    #endif
    std::array<ot_video_frame_info, ASYNC_FRAME_POOL_SIZE> m_astAsyncFrames;
    std::queue<ot_video_frame_info *> m_availableAsyncFrames;
    std::mutex m_asyncFramePoolMutex;
    bool m_bAsyncFramePoolInitialized = false;
    /* 目标视频帧 */
    ot_video_frame_info m_stDstFrameInfo;
    /* 人脸抓拍处理器 */
    FaceDetectInternal::CFaceCaptureProcessor m_captureProcessor;
    #if CAP_AI_FACE_COMPARE
    /* 人脸特征提取与比对处理器 */
    FaceDetectInternal::CFaceFeatureProcessor m_featureProcessor;
    #endif
    /*异步人脸检测Worker*/
    CFaceDetectWorker m_detectWorker;
};
