/*
 * @Author: 梁浩尧 lianghaoyao@kfb.cn
 * @Date: 2025-11-05 10:38:00
 * @LastEditors: 梁浩尧 lianghaoyao@kfb.cn
 * @LastEditTime: 2025-12-04 20:38:43
 * @FilePath: /1126/rv1126b_ipc/main_app/ai_app/algorithm_mode/algorithm/electricScooter_detect/electricScooter_detect.hpp
 * @Description: 电瓶车检测
 */
 #pragma once

#include <atomic>
#include <condition_variable>
#include <mutex>
#include <thread>
#include <sys/time.h>
#include "blocking_queue.hpp"
#include <opencv2/opencv.hpp>
#include "event_manager.hpp"
#include "stream_process_ext.hpp"
#include "algorithm.hpp"
#include "ElectricScooterDetectV1_0.hpp"

class CElectricScooterDetect : public CAlgorithm
{
public:
    typedef struct _VehicleDetectEventType_
    {
      /* 识别到电瓶车 */
      bool bElectricScooterInElevator = false;
    }ElectricScooterDetectEventType_S;

    CElectricScooterDetect();
    ~CElectricScooterDetect();

    /**
     * @brief 接受媒体数据
     * @param [MediaData_S] stMediaData:
     * @return [*]
     * @note
     */
    void recvMediaData(MediaData_S stMediaData) override;
    
    /**
     * @brief 更新算法配置参数
     * @param stAlgoConfig 
     */
    void setAlgoEnCfg(const Event::AlgorithmConfig &stAlgoConfig) override;
    void setAlgoParamCfg(const Alarm::ElectricScooterDetection_S &stAlgoCfg,Event::Type_E enType = Event::Type_E::SLEEP_ON_DUTY);

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
     * @note
     */
    bool unInit();
    /**
     * @brief 灵敏度转置信度（反向映射：灵敏度越高，置信度越低）
     * @param sensitivity 输入灵敏度（范围：1~100，超出会被 clamp 到该范围）
     * @param minConfidence 最低置信度（默认 0.5f，float 类型）
     * @param maxConfidence 最高置信度（默认 0.9f，float 类型）
     * @return float 置信度（范围：minConfidence ~ maxConfidence）
     */
    float sensitivityToConfidence(int sensitivity, float minConfidence = 0.5f, float maxConfidence = 0.9f);
    
     /**
     * @brief 将 1-100 的灵敏度转换为触发报警所需的连续帧数
     * @param sensitivity 灵敏度 (1-100)
     * @param minFrames 最高灵敏度 (100) 对应的帧数
     * @param maxFrames 最低灵敏度 (1) 对应的帧数s
     * @return int 触发报警所需的连续帧数
     */
    int sensitivityToFrames(int sensitivity, int minFrames = 1, int maxFrames = 10);
    /**
     * @brief 事件转换函数
     * @param eventFlags 
     * @return int 
     */
    int convertMaskToType( int eventFlags, ElectricScooterDetectEventType_S &stEventStatus);
    /**
     * @brief 线程函数
     * @return [*]
     * @note
     */
    void run();

private:
    /* 句柄 */
    ElectricScooterDetect_NS::CElectricScooterDetectV1_0* m_pHandle = nullptr;

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

    /* 电瓶车识别相关配置 */
    Alarm::ElectricScooterDetection_S m_stAlgoElectricScooterCfg;

    /* 检测频率控制 */
    EventManager m_RecvManager{500};
    /* 报警状态管理 */
    CAlarmStateMachine m_ElectricScooterStateMachine;  /* 电瓶车进入电梯 */

    /* 算法默认分辨率 */
    int m_nWidth = 640;
    int m_nHeight = 384;
};