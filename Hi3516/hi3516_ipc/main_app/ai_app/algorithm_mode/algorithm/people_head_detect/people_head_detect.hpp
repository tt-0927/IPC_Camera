/**
 * @FilePath     : people_head_detect.hpp
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2026-04-22 18:44:55
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-04-23 09:38:47
 * @Description  : 人头检测主控模块
 */

#pragma once

#include <atomic>
#include <chrono>
#include <mutex>
#include <thread>

#include "algorithm.hpp"
#include "blocking_queue.hpp"
#include "common_process.h"
#include "internal/processors/crowd/people_head_crowd_gathering_processor.hpp"
#if CAP_AI_PEOPLE_STATISTICS
#include "internal/processors/density/people_head_density_processor.hpp"
#endif

class CPeopleHeadDetect : public CAlgorithm
{
public:
    /**
     * @brief   : 构造人头检测主控对象
     */
    CPeopleHeadDetect();

    /**
     * @brief   : 析构人头检测主控对象
     */
    ~CPeopleHeadDetect();

    /**
     * @brief   : 接受媒体数据
     * @param    {MediaData_S} stMediaData：媒体数据
     * @return   {void}
     */
    void recvMediaData(MediaData_S stMediaData) override;

    /**
     * @brief   : 更新算法配置参数
     * @param    {AlgorithmConfig} &stAlgoConfig：算法配置
     * @return   {void}
     */
    void setAlgoEnCfg(const Event::AlgorithmConfig &stAlgoConfig) override;

    /**
     * @brief   : 更新人员聚集侦测参数
     * @param    {CrowdGathering_S} &stAlgoCfg：人员聚集侦测配置
     * @return   {void}
     */
    void setAlgoParamCfg(const Alarm::CrowdGathering_S &stAlgoCfg);

#if CAP_AI_PEOPLE_STATISTICS
    /**
     * @brief   : 更新人员密度检测参数
     * @param    {PeopleDensityDetection_S} &stAlgoCfg：人员密度检测配置
     * @return   {void}
     */
    void setAlgoParamCfg(const Alarm::PeopleDensityDetection_S &stAlgoCfg);

    /**
     * @brief   : 设置事件统计上报器
     * @param    {IEventStatisticsReporter} &pReporter：统计上报器
     * @return   {void}
     */
    void setEventStatisticsReporter(
        const std::shared_ptr<EventStatistics_NS::IEventStatisticsReporter> &pReporter) override;
#endif

private:
    /**
     * @brief   : 初始化人头模型
     * @return   {bool} true：成功 false：失败
     */
    bool init();

    /**
     * @brief   : 反初始化人头模型
     * @return   {bool} true：成功 false：失败
     */
    bool unInit();

    /**
     * @brief   : 人头模型线程函数
     * @return   {void}
     */
    void run();

    /**
     * @brief   : 判断当前是否存在任一使能中的人头类事件
     * @return   {bool} true：存在使能事件 false：全部关闭
     */
    bool hasEnabledAlgorithm() const;

private:
    /* 人头检测句柄 */
    Inference_NS::CYoloUltralytics *m_pHeadDetHandle = nullptr;
    /* 媒体数据队列 */
    BQ_NS::CBlockingQueue<MediaData_S> m_dateQueue;
    /* 用于控制线程运行 */
    std::atomic<bool> m_bRunning;
    /* 互斥锁保护模型句柄访问 */
    std::mutex m_mutex;
    /* 数据获取线程 */
    std::thread m_thread;
    /* 检测频率控制 */
    EventManager m_RecvManager{ 500 };
    /* 人员聚集处理器 */
    PeopleHeadDetectInternal::CPeopleHeadCrowdGatheringProcessor m_crowdGatheringProcessor;
#if CAP_AI_PEOPLE_STATISTICS
    /* 人员密度处理器 */
    PeopleHeadDetectInternal::CPeopleHeadDensityProcessor m_densityProcessor;
#endif
    /* 算法默认分辨率宽度 */
    int m_nWidth = PIXEL_WIDTH_640;
    /* 算法默认分辨率高度 */
    int m_nHeight = PIXEL_HEIGHT_384;
    /* 目标视频帧 */
    ot_video_frame_info m_stDstFrameInfo;
};
