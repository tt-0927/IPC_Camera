/**
 * @FilePath     : ai_detect.h
 * @Author       : zhouzirui
 * @Date         : 2025-05-16 09:54:06
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2025-08-13 10:38:25
 * @Description  : AI检测结果分析模块
 */

#pragma once

#include <iostream>
#include <mutex>
#include <thread>
#include <atomic>
#include <vector>
#include <condition_variable>
#include <stdexcept>
#include <queue>
#include "dlog.h"
#include "stream_ai_detect.h"
#include "event_linkage.h"

extern "C"
{
#include "get_time.h"
#include "svp_ai_detect.h"
}

// 跟踪目标状态记录，使用track_id作为索引
#define MAX_TRACK_ID 100

/*区域入侵检测相关参数结构体定义*/
typedef struct
{
    bool bIsInRegion;  // 目标是否在入侵区域
    double dEnterTime; // 目标进入区域的时间戳
    bool bAlarmed;     // 是否已经报警
} IntrusionStatus_S;

/*区域入侵检测配置结构体定义*/
typedef struct
{
    /*入侵区域定义 (x, y, width, height)*/
    ot_rect stRegion;
    /*入侵阈值时间（单位：秒）*/
    uint32_t unSec;
    void clear()
    {
        stRegion.x = 0;
        stRegion.y = 0;
        stRegion.width = 1024;
        stRegion.height = 576;
        unSec = 5;
    }
} IntrusionDetectConfig_S;

/*AI检测配置结构体定义*/
typedef struct
{
    Event::Type_E enEventType;
    union{
        IntrusionDetectConfig_S stIntrusionConfig;
    };
} AiDetectConfig_S;

class CAiDetect
{
public:
    CAiDetect();
    ~CAiDetect();

    /**
     * @brief       : 初始化AI检测结果分析
     * @author      : zhouzirui
     * @return       {*} 0：成功 <0：失败
     */
    int init();

    /**
     * @brief       : 反初始化AI检测结果分析
     * @author      : zhouzirui
     * @return       {*} 0：成功 <0：失败
     */
    int deinit();

    /**
     * @brief       : 设置AI检测的配置
     * @author      : zhouzirui
     * @param        {AiDetectConfig_S} stConfig：*AI检测配置结构体
     */
    void set_DetectConfig(AiDetectConfig_S &stConfig);

    /**
     * @brief       : 发送AI检测结果分析至AI检测队列
     * @author      : zhouzirui
     * @param        {ot_aidetect_result_array} stResult：AI检测结果
     */
    void send_results(ot_aidetect_result_array &stResult);
    
private:
    /**
     * @brief       : 深拷贝AI检测结果
     * @author      : zhouzirui
     * @param        {ot_aidetect_result_array} *pDst：目标结构体指针 
     * @param        {ot_aidetect_result_array} *pSrc：源结构体指针
     * @return       {*}0：成功 <0：失败
     */
    int deepCopyResult(ot_aidetect_result_array *pDst, const ot_aidetect_result_array *pSrc);

    /**
     * @brief       : 释放结果结构体
     * @author      : zhouzirui
     * @param        {ot_aidetect_result_array} &stResult：AI检测结果结构体引用
     */
    void freeResult(ot_aidetect_result_array &stResult);

    /**
     * @brief       : 检查一个目标是否在入侵区域内
     * @author      : zhouzirui
     * @param        {ot_rect} *target_rect
     * @return       {}true：目标在入侵区域内 false：目标不在入侵区域内
     */
    bool is_in_intrusion_region(const ot_rect *target_rect);

    /**
     * @brief       : 处理区域入侵报警
     * @author      : zhouzirui
     * @param        {uint32_t} track_id：跟踪ID
     * @param        {ot_aidetect_class} class_type：检测类型
     * @param        {ot_rect} *rect：坐标
     */
    void process_intrusion_alarm(uint32_t track_id, ot_aidetect_class class_type, const ot_rect *rect);

    /**
     * @brief       : AI检测结果分析
     * @author      : zhouzirui
     * @param        {ot_aidetect_result_array} stResult：AI检测结果
     */
    void results_analysis(ot_aidetect_result_array &stResult);

    // info /*----------------------- 私有线程函数 -----------------------*/
    /**
     * @brief       : AI检测结果分析线程
     * @author      : zhouzirui
     */
    void results_analysis_thr();

    /*异常处理*/
    void HandleThreadException(const std::string &thread_name)
    {
        std::lock_guard<std::mutex> lock(exception_mutex_);
        last_error_ = thread_name + " crashed";
        // RestartThread(thread_name); // 线程重启逻辑
    }

private:
    // info /*----------------------- 模块句柄 -----------------------*/
    /*互斥锁*/
    std::mutex m_mutex;
    /*AI检测结果分析线程句柄*/
    std::thread m_resultsThread;
    /*AI检测结果分析线程运行标志*/
    std::atomic_bool m_bResultsFlag;
    // info /*----------------------- 参数变量 -----------------------*/
    /* 是否初始化 */
    bool m_bInitFlag = false;
    /*线程异常处理互斥锁*/
    std::mutex exception_mutex_;
    /*线程异常处理的线程名称*/
    std::string last_error_;
    /*AI检测结果队列*/
    std::queue<ot_aidetect_result_array> m_AiResultQueue;
    /*AI检测类型定义*/
    char m_aClassTypes[OT_AIDETECT_CLASS_BUTT][SVP_AIDETECT_BUFFER_LEN] = {
        "人脸",
        "人形",
        "机动车",
        "宠物(主要是猫狗)",
        "垃圾(主要是垃圾袋)",
        "包裹(快递包裹、书包)",
        "钱包",
        "手机"
    };
    /*AI跟踪状态定义*/
    char m_aTrackStatus[OT_AIDETECT_TRACK_STATUS_BUTT][SVP_AIDETECT_BUFFER_LEN] = {
        "单目标首次跟踪",
        "已跟踪上的目标状态更新",
        "当前目标断开跟踪",
        "未开启跟踪"
    };
    /*区域入侵检测状态*/
    // std::vector<IntrusionStatus_S> m_vstTargetStatus;
    IntrusionStatus_S stIntrusionStatus[MAX_TRACK_ID];
    /*AI检测配置*/
    AiDetectConfig_S m_stAiDetectConfig;
};
