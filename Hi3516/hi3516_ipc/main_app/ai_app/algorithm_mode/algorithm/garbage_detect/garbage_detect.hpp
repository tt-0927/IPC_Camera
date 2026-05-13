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
#include <thread>

#include "algorithm.hpp"
#include "algo_control_deal.h"
#include "blocking_queue.hpp"
#include "common_process.h"
#include "YoloUltralytics_rpn.hpp"

class CGarbageDetect : public CAlgorithm
{
public:
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
};

#endif
