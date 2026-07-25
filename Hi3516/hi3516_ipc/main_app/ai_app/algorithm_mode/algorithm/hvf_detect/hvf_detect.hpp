/**
 * @FilePath     : hvf_detect.hpp
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2025-07-29 20:10:37
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-03-28 10:43:44
 * @Description  : 脸人车侦测
 */

#pragma once

#include <atomic>
#include <chrono>
#include <thread>

#include "blocking_queue.hpp"
#include "common_process.h"
#include "algorithm.hpp"
#include "share_data.h"
#include "stream_ai_detect.h"
#include "internal/processors/face/hvf_face_processor.hpp"
#include "internal/processors/boundary/hvf_boundary_processor.hpp"
#include "internal/processors/region/hvf_intrusion_processor.hpp"
#include "internal/processors/region/hvf_loitering_processor.hpp"
#include "internal/processors/region/hvf_parking_processor.hpp"
#include "internal/processors/region/hvf_enter_exit_processor.hpp"
#if CAP_AI_PEOPLE_STATISTICS
#include "internal/processors/statistics/hvf_people_flow_processor.hpp"
#endif
#if CAP_AI_PEOPLE_DENSITY_V2
#include "internal/processors/statistics/hvf_people_density_processor.hpp"
#endif

extern "C"
{
#include "svp_ai_detect.h"
#include "mpp_vgs.h"
}

/**
 * @brief   : 脸人车侦测主控类
 */
class CHVFDetect : public CAlgorithm
{
public:
    /**
     * @brief   : 构造脸人车侦测主控对象
     */
    CHVFDetect();

    /**
     * @brief   : 析构脸人车侦测主控对象
     */
    ~CHVFDetect();

    /**
     * @brief   : 接收媒体数据
     * @param    {MediaData_S} stMediaData：媒体数据
     */
    void recvMediaData(MediaData_S stMediaData) override;

    /**
     * @brief   : 更新算法配置参数
     * @param    {AlgorithmConfig} &stAlgoConfig：算法配置
     */
    void setAlgoEnCfg(const Event::AlgorithmConfig &stAlgoConfig) override;

    /**
     * @brief   : 更新人脸侦测参数
     * @param    {FaceDetection_S} &stAlgoCfg：人脸侦测
     */
    void setAlgoParamCfg(const Alarm::FaceDetection_S &stAlgoCfg);

    /**
     * @brief   : 更新徘徊侦测参数
     * @param    {FaceDetection_S} &stAlgoCfg：徘徊侦测
     */
    void setAlgoParamCfg(const Alarm::LoiteringDetection_S &stAlgoCfg);

    /**
     * @brief   : 更新停车侦测参数
     * @param    {ParkingDetection_S} &stAlgoCfg：停车侦测
     */
    void setAlgoParamCfg(const Alarm::ParkingDetection_S &stAlgoCfg);

    /**
     * @brief   : 更新越界侦测参数
     * @param    {BoundaryDetection_S} &stAlgoCfg：越界侦测
     */
    void setAlgoParamCfg(const Alarm::BoundaryDetection_S &stAlgoCfg);

    /**
     * @brief   : 更新区域入侵侦测参数
     * @param    {FieldDetection_S} &stAlgoCfg：区域侦测
     */
    void setAlgoParamCfg(const Alarm::FieldDetection_S &stAlgoCfg);

    /**
     * @brief   : 更新进入区域侦测参数
     * @param    {EntranceDetection_S} &stAlgoCfg：区域侦测
     */
    void setAlgoParamCfg(const Alarm::EntranceDetection_S &stAlgoCfg);

    /**
     * @brief   : 更新离开区域侦测参数
     * @param    {ExitingDetection_S} &stAlgoCfg：区域侦测
     */
    void setAlgoParamCfg(const Alarm::ExitingDetection_S &stAlgoCfg);

#if CAP_AI_PEOPLE_STATISTICS
    /**
     * @brief   : 更新人流统计参数
     * @param    {PeopleFlowStatistics_S} &stAlgoCfg：人流统计配置
     * @return   {void}
     */
    void setAlgoParamCfg(const Alarm::PeopleFlowStatistics_S &stAlgoCfg);
#endif

#if CAP_AI_PEOPLE_DENSITY_V2
    /**
     * @brief   : 更新人员密度检测参数
     * @param    {PeopleDensityDetection_S} &stAlgoCfg：人员密度检测配置
     * @return   {void}
     */
    void setAlgoParamCfg(const Alarm::PeopleDensityDetection_S &stAlgoCfg);
#endif

#if CAP_AI_PEOPLE_STATISTICS || CAP_AI_PEOPLE_DENSITY_V2
    /**
     * @brief   : 设置事件统计上报器
     * @param    {IEventStatisticsReporter} &pReporter：统计上报器
     * @return   {void}
     */
    void setEventStatisticsReporter(
        const std::shared_ptr<EventStatistics_NS::IEventStatisticsReporter> &pReporter) override;
#endif

#if CAP_AI_PEOPLE_STATISTICS
    /**
     * @brief   : 处理 HVF 运行时命令
     * @param    {RuntimeCommand_S} &stCommand：运行时命令
     * @return   {int} OK：处理成功 ERR：未处理或失败
     */
    int handleRuntimeCommand(const RuntimeCommand_S &stCommand) override;
#endif

private:
    /**
     * @brief   : 初始化
     * @return   {bool} true：成功 false：失败
     */
    bool init();

    /**
     * @brief   : 反初始化
     * @return   {bool} true：成功 false：失败
     */
    bool unInit();

    /**
     * @brief   : 重新启动
     * @return   {bool} true：成功 false：失败
     */
    bool reboot();

    /**
     * @brief   : 线程函数
     */
    void run();

    /**
     * @brief   : 判断当前是否存在任一使能中的 HVF 事件
     * @return   {bool} true：存在使能事件 false：全部关闭
     */
    bool hasEnabledAlgorithm() const;

private:
    /* 脸人车侦测句柄 */
    HiAiDetect_S *m_pHVFDetHandle = nullptr;
    /* 队列 */
    BQ_NS::CBlockingQueue<MediaData_S> m_dateQueue;
    /* 用于控制线程的运行 */
    std::atomic<bool> m_bRunning;
    /* 数据获取线程 */
    std::thread m_thread;
    /* 检测频率控制 */
    EventManager m_RecvManager{ 200 };
    /* 人脸侦测处理器 */
    HVFDetectInternal::CHVFFaceProcessor m_faceProcessor;
    /* 越界侦测处理器 */
    HVFDetectInternal::CHVFBoundaryProcessor m_boundaryProcessor;
    /* 区域入侵处理器 */
    HVFDetectInternal::CHVFIntrusionProcessor m_intrusionProcessor;
    /* 徘徊侦测处理器 */
    HVFDetectInternal::CHVFLoiteringProcessor m_loiteringProcessor;
    /* 停车侦测处理器 */
    HVFDetectInternal::CHVFParkingProcessor m_parkingProcessor;
    /* 进入/离开区域处理器 */
    HVFDetectInternal::CHVFEnterExitProcessor m_enterExitProcessor;
    /* 目标视频帧 */
    ot_video_frame_info m_stDstFrameInfo;
#if CAP_AI_PEOPLE_STATISTICS
    /* 人流统计处理器 */
    HVFDetectInternal::CHVFPeopleFlowProcessor m_peopleFlowProcessor;
#endif
#if CAP_AI_PEOPLE_DENSITY_V2
    /* 人员密度 V2 处理器 */
    HVFDetectInternal::CHVFPeopleDensityProcessor m_peopleDensityProcessor;
#endif
    /* 算法默认分辨率 */
    int m_nWidth = PIXEL_WIDTH_1024;
    /* 算法默认分辨率高度 */
    int m_nHeight = PIXEL_HEIGHT_576;
};
