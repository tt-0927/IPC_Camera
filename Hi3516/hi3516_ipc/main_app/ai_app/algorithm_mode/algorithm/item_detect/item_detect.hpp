/**
 * @FilePath     : item_detect.hpp
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2025-07-29 20:10:37
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2025-12-31 17:02:07
 * @Description  : 物品侦测
 */

#pragma once

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <mutex>
#include <thread>
#include <algorithm>
#include <bitset>
#include <sys/time.h>
#include <unordered_set>

#include "blocking_queue.hpp"
#include "common_process.h"
#include "algorithm.hpp"
#include "share_data.h"
#include "stream_ai_detect.h"
#include "target_index_manager.hpp"

extern "C"
{
    #include "svp_ai_detect.h"
    #include "mpp_vgs.h"
    #include "svp_ld.h"
}

#if 1
/* 物品侦测参考帧更新频率控制:10秒 */
#define REFERENCE_FRAME_UPDATE_FREQUENCY (10);

/* 区域触发状态结构体 */
struct RegionTriggerState_S
{
    bool bTriggering;              /* 当前是否正在触发 */
    long long llTriggerStartTime;  /* 触发开始时间（毫秒） */

    /* 构造函数：初始化为未触发状态 */
    RegionTriggerState_S() :
        bTriggering(false),
        llTriggerStartTime(0)
    {
    }
};

class CItemDetect : public CAlgorithm
{
public:

    CItemDetect();
    ~CItemDetect();

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
     * @brief   : 更新物品遗留侦测参数
     * @param    {FaceDetection_S} &stAlgoCfg：物品遗留侦测配置
     */
    void setAlgoParamCfg(const Alarm::UnattendedObject_S &stAlgoCfg);

    /**
     * @brief   : 更新物品拿取侦测参数
     * @param    {FaceDetection_S} &stAlgoCfg：物品拿取侦测配置
     */
    void setAlgoParamCfg(const Alarm::ObjectRemoval_S &stAlgoCfg);

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

    // info /*----------------------- 算法后处理 -----------------------*/

    /**
     * @brief   : 物品遗留侦测处理函数
     * @param    {HiLdRegionResult_S} *pstResult 算法输出结果
     * @param    {vector<Common::RectInfo_S>} &vstRectInfo OSD 动态分析显示数组
     * @param    {bool} &bForceRefUpdate：[输出] 是否需要强制立即更新参考帧
     * @param    {SEventProcessContext} &stCtx：事件处理上下文
     * @return   {bool} 是否延迟参考帧更新，延迟：true，不延迟：false
     * @note    : 展会版本下会额外输出左上角汇总面板结果
     */
    bool processUnattendedObjectDetect(HiLdRegionResult_S *pstResult,
                                       std::vector<Common::RectInfo_S> &vstRectInfo,
                                       bool &bForceRefUpdate,
                                       const SEventProcessContext &stCtx
#if CAP_EXHIBITION_OSD_PANEL
                                       , OsdPanel::PanelFrame_S *pstPanelFrame = nullptr
#endif
    );

    /**
     * @brief   : 物品拿取侦测处理函数
     * @param    {HiLdRegionResult_S} *pstResult 算法输出结果
     * @param    {vector<Common::RectInfo_S>} &vstRectInfo OSD 动态分析显示数组
     * @param    {bool} &bForceRefUpdate：[输出] 是否需要强制立即更新参考帧
     * @param    {SEventProcessContext} &stCtx：事件处理上下文
     * @return   {bool} 是否延迟参考帧更新，延迟：true，不延迟：false
     * @note    : 展会版本下会额外输出左上角汇总面板结果
     */
    bool processObjectRemovalDetect(HiLdRegionResult_S *pstResult,
                                    std::vector<Common::RectInfo_S> &vstRectInfo,
                                    bool &bForceRefUpdate,
                                    const SEventProcessContext &stCtx
#if CAP_EXHIBITION_OSD_PANEL
                                    , OsdPanel::PanelFrame_S *pstPanelFrame = nullptr
#endif
    );

    // info /*----------------------- 工具函数 -----------------------*/

    /**
     * @brief   : 转换区域坐标并判断是否使能算法
     */
    template <typename T> 
    void convertResolutionAndEnable(T &stConfig);

private:

    /* 物品侦测句柄 svp_ld句柄 */
    HiLd_S *m_pItemDetHandle = nullptr;
    /* 队列 */
    BQ_NS::CBlockingQueue<MediaData_S> m_dateQueue;
    /* 用于控制线程的运行 */
    std::atomic<bool>       m_bRunning;
    /* 互斥锁保护指针访问 */
    std::mutex              m_mutex;
    /* 条件变量 */
    // std::condition_variable m_condition;
    /* 数据获取线程 */
    std::thread             m_thread;
    /* 检测频率控制 */
    EventManager m_RecvManager{500};
    /* 物品遗留侦测报警首次时间 */
    // long long m_llUnattendedTime = 0;
    /* 物品遗留侦测报警首次时间 */
    // long long m_llRemovalTime = 0;
    /* 每个区域的触发状态 */
    RegionTriggerState_S m_astRegionTriggerState[SVP_LD_MAX_REGION_NUM];
    /* 物品遗留侦测报警状态机，判断是否进行报警 */
    CAlarmStateMachine m_unattendedObjectAlarmStateMachine;
    /* 物品拿取侦测报警状态机，判断是否进行报警 */
    CAlarmStateMachine m_objectRemovalAlarmStateMachine;

    /* 配置参数 */
    /* 物品遗留侦测 */ 
    Alarm::UnattendedObject_S m_stUnattendedObjectDetCfg;
    /* 物品拿取侦测 */
    Alarm::ObjectRemoval_S m_stObjectRemovalDetCfg;

    /* 算法默认分辨率 */
    int m_nWidth = PIXEL_WIDTH_640;
    int m_nHeight = PIXEL_HEIGHT_384;
    /* 目标视频帧 */
    ot_video_frame_info m_stDstFrameInfo;
};
#else
/* 需要判断的结果个数 SVP_AIDETECT_MAX_OUTPUT_RECT_NUM * 4 （垃圾，包裹，钱包，手机） */
#define RESULT_NUM  (80)

class CItemDetect : public CAlgorithm
{
public:

    CItemDetect();
    ~CItemDetect();

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
     * @brief   : 更新物品遗留侦测参数
     * @param    {FaceDetection_S} &stAlgoCfg：物品遗留侦测配置
     */
    void setAlgoParamCfg(const Alarm::UnattendedObject_S &stAlgoCfg);

    /**
     * @brief   : 更新物品拿取侦测参数
     * @param    {FaceDetection_S} &stAlgoCfg：物品拿取侦测配置
     */
    void setAlgoParamCfg(const Alarm::ObjectRemoval_S &stAlgoCfg);

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

    // info /*----------------------- 算法后处理 -----------------------*/

    /**
     * @brief   : 物品遗留侦测处理函数
     * @param    {ot_aidetect_result_array} &stResult：算法输出结果
     * @param    {vector<Common::RectInfo_S>} &vstRectInfo：OSD 动态分析显示数组
     * @param    {SEventProcessContext} &stCtx：事件处理上下文
     */
    void processUnattendedObjectDetect(ot_aidetect_result_array &stResult,
                                       std::vector<Common::RectInfo_S> &vstRectInfo,
                                       const SEventProcessContext &stCtx);

    /**
     * @brief   : 物品拿取侦测处理函数
     * @param    {ot_aidetect_result_array} &stResult：算法输出结果
     * @param    {vector<Common::RectInfo_S>} &vstRectInfo：OSD 动态分析显示数组
     * @param    {SEventProcessContext} &stCtx：事件处理上下文
     */
    void processObjectRemovalDetect(ot_aidetect_result_array &stResult,
                                    std::vector<Common::RectInfo_S> &vstRectInfo,
                                    const SEventProcessContext &stCtx);

    // info /*----------------------- 工具函数 -----------------------*/

    /**
     * @brief   : 转换区域坐标并判断是否使能算法
     */
    template <typename T> 
    void convertResolutionAndEnable(T &stConfig);

    /**
     * @brief   : 物品遗留检测专用函数
     * @param    {ot_aidetect_object_of_one_class} *pstObjectClass：目标类别结果
     * @param    {vector<Common::RectInfo_S>} &vstRectInfo：OSD 动态分析显示数组
     * @return   {bool} 是否有报警
     */
    bool processUnattendedObjectRegionDetection(const ot_aidetect_object_of_one_class *pstObjectClass, std::vector<Common::RectInfo_S> &vstRectInfo);

    /**
     * @brief   : 物品拿取检测专用函数
     * @param    {ot_aidetect_object_of_one_class} *pstObjectClass：目标类别结果
     * @param    {vector<Common::RectInfo_S>} &vstRectInfo：OSD 动态分析显示数组
     * @return   {bool} 是否有报警
     */
    bool processObjectRemovalRegionDetection(const ot_aidetect_object_of_one_class *pstObjectClass, std::vector<Common::RectInfo_S> &vstRectInfo);

private:

    /* 物品侦测句柄 */
    HiAiDetect_S *m_pItemDetHandle = nullptr;
    /* 队列 */
    BQ_NS::CBlockingQueue<MediaData_S> m_dateQueue;
    /* 用于控制线程的运行 */
    std::atomic<bool>       m_bRunning;
    /* 互斥锁保护指针访问 */
    std::mutex              m_mutex;
    /* 条件变量 */
    std::condition_variable m_condition;
    /* 数据获取线程 */
    std::thread             m_thread;
    /* 检测频率控制 */
    EventManager m_RecvManager{500};
    /* 物品遗留侦测报警状态机，判断是否进行报警 */
    CAlarmStateMachine m_unattendedObjectAlarmStateMachine;
    /* 物品拿取侦测报警状态机，判断是否进行报警 */
    CAlarmStateMachine m_objectRemovalAlarmStateMachine;

    /* 配置参数 */
    /* 物品遗留侦测 */ 
    Alarm::UnattendedObject_S m_stUnattendedObjectDetCfg;
    /* 物品拿取侦测 */
    Alarm::ObjectRemoval_S m_stObjectRemovalDetCfg;
    /* 物品遗留侦测状态 - 支持多区域(4个) [区域索引][内部索引] */
    AreaStatus_S m_stUnattendedObjectStatus[UNATTENDED_OBJECT_DETECT_REGION_DEFAULT][RESULT_NUM];
    /* 物品拿取侦测状态 - 支持多区域(4个) [区域索引][内部索引]*/
    AreaStatus_S m_stObjectRemovalStatus[OBJECT_REMOVAL_DETECT_REGION_DEFAULT][RESULT_NUM];
    /* 物品遗留侦测的目标索引管理器 */
    CTargetIndexManager80 m_unattendedObjectIndexManager;
    /* 物品拿取侦测的目标索引管理器 */
    CTargetIndexManager80 m_objectRemovalIndexManager;

    /* 算法默认分辨率 */
    int m_nWidth = PIXEL_WIDTH_1024;
    int m_nHeight = PIXEL_HEIGHT_576;
};
#endif
