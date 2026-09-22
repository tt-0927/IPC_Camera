/**
 * @FilePath     : garbage_detect.hpp
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2026-04-01 11:22:54
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-04-28 20:31:08
 * @Description  : 垃圾检测
 */

#pragma once

#if CAP_AI_GARBAGE_DETECT

#include <atomic>
#include <condition_variable>
#include <mutex>
#include <thread>
#include <vector>

#include "algorithm.hpp"
#include "algo_control_deal.h"
#include "blocking_queue.hpp"
#include "common_process.h"
#include "YoloUltralytics_rpn.hpp"

class CGarbageDetect : public CAlgorithm
{
public:
    /**
     * @brief   : 手动抓拍单次检测结果
     */
    typedef struct SnapshotResult
    {
        /* 全景 JPEG 图片数据 */
        std::vector<unsigned char> vecJpeg;
        /* 检测框（复用区域限制后的结果） */
        std::vector<Common::RectInfo_S> vstRectInfo;
        /* 是否命中垃圾满溢 */
        bool bGarbageOverflow = false;
        /* 是否命中垃圾暴露 */
        bool bGarbageExposure = false;
    } SnapshotResult_S;

    CGarbageDetect();
    ~CGarbageDetect();

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
     * @brief   : 更新垃圾暴露检测参数
     * @param    {GarbageExposureDetection_S} &stAlgoCfg：垃圾暴露检测配置
     */
    void setAlgoParamCfg(const Alarm::GarbageExposureDetection_S &stAlgoCfg);

    /**
     * @brief   : 更新垃圾满溢检测参数
     * @param    {GarbageOverflowDetection_S} &stAlgoCfg：垃圾满溢检测配置
     */
    void setAlgoParamCfg(const Alarm::GarbageOverflowDetection_S &stAlgoCfg);

    void handleDetectResult(const std::vector<Inference_NS::BoxData_S> &vBoxDatas, ot_video_frame_info *pFrameInfo);

    /**
     * @brief   : 手动抓拍单次检测
     *            向流式线程投递一次快照请求，由该线程在下一次循环中串行执行推理，
     *            以规避推理句柄的并发调用；仅复用区域限制，不判断事件使能、不走报警状态机。
     * @param    {SnapshotResult_S} &stResult 输出：JPEG 图片与检测结果
     * @param    {int} nTimeoutMs 等待超时，单位毫秒
     * @return   {bool} 成功返回 true，超时或算法未就绪返回 false
     */
    bool detectOnce(SnapshotResult_S &stResult, int nTimeoutMs = 3000);

private:
    /* 初始化 */
    bool init();
    /* 反初始化 */
    bool unInit();
    /* 线程函数 */
    void run();
    /* 当前是否有垃圾检测算法启用 */
    bool isEnabled() const;
    /* 等待算法句柄准备完成 */
    bool waitForHandle();
    /* 获取送分析的视频帧 */
    ot_video_frame_info *getFrameForInference(MediaData_S &stMediaData);
    /* 垃圾检测推理 */
    bool inferFrame(ot_video_frame_info *pFrameInfo, std::vector<Inference_NS::BoxData_S> &vBoxDatas);
    /* 垃圾检测后处理 */
    void processGarbageDetect(std::vector<Inference_NS::BoxData_S> &vBoxDatas,
                              std::vector<Common::RectInfo_S> &vstRectInfo,
                              const SEventProcessContext &stCtx);
    /* 快照检测后处理：仅复用区域限制，不判断事件使能 */
    void processSnapshotDetect(std::vector<Inference_NS::BoxData_S> &vBoxDatas, SnapshotResult_S &stResult);
    /* 在流式线程内执行快照请求并唤醒等待方 */
    void handleSnapshotRequest(ot_video_frame_info *pFrameInfo,
                               const std::vector<Inference_NS::BoxData_S> &vBoxDatas);

private:
    /* 垃圾检测句柄 */
    Inference_NS::CYoloUltralytics *m_pGarbageDetHandle = nullptr;
    /* 队列 */
    BQ_NS::CBlockingQueue<MediaData_S> m_dateQueue;
    /* 用于控制线程的运行 */
    std::atomic<bool> m_bRunning;
    /* 数据获取线程 */
    std::thread m_thread;
    /* 检测频率控制 */
    EventManager m_RecvManager{500};
    /* 垃圾暴露报警状态机 */
    CAlarmStateMachine m_garbageExposureAlarmStateMachine;
    /* 垃圾满溢报警状态机 */
    CAlarmStateMachine m_garbageOverflowAlarmStateMachine;
    /* 垃圾暴露检测配置 */
    Alarm::GarbageExposureDetection_S m_stAlgoGarbageExposureCfg;
    /* 垃圾满溢检测配置 */
    Alarm::GarbageOverflowDetection_S m_stAlgoGarbageOverflowCfg;
    /* 算法默认分辨率 */
    int m_nWidth = PIXEL_WIDTH_640;
    int m_nHeight = PIXEL_HEIGHT_384;
    /* 目标视频帧 */
    ot_video_frame_info m_stDstFrameInfo;

    /* 手动抓拍快照：请求标志与结果回填 */
    std::atomic<bool> m_bSnapshotPending{false};
    std::mutex m_snapshotMutex;
    std::condition_variable m_snapshotCv;
    /* 快照是否已完成（受 m_snapshotMutex 保护） */
    bool m_bSnapshotDone = false;
    /* 快照检测结果（受 m_snapshotMutex 保护） */
    SnapshotResult_S m_stSnapshotResult;
};

#endif
