/*
* @Author: 梁浩尧 lianghaoyao@kfb.cn
* @Date: 2025-11-05 10:38:00
 * @LastEditors: lianghy lianghy@kfb.cn
 * @LastEditTime: 2026-01-30 14:12:02
* @FilePath: /1126/rv1126b_ipc/main_app/ai_app/algorithm_mode/algorithm/licensePlateCognition_detect/licensePlateCognition_detect.hpp
* @Description: 车牌识别
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
#include "LicensePlateCognitionV1_0.hpp"


class CLicensePlateCognitionDetect : public CAlgorithm
{
public:

    CLicensePlateCognitionDetect();
    ~CLicensePlateCognitionDetect();

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
     * @brief   : 更新车牌识别参数 
     * @param    {HideAlarm_S} &stAlgoCfg：遮挡侦测
     */
    void setAlgoParamCfg(const Alarm::LicensePlateCognitionDetection_S &stAlgoCfg, Event::Type_E enType = Event::Type_E::CONSTRUCTION_OCCUPY_ROAD);

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
     * @param maxFrames 最低灵敏度 (1) 对应的帧数
     * @return int 触发报警所需的连续帧数
     */
    int sensitivityToFrames(int sensitivity, int minFrames = 1, int maxFrames = 10);

    /**
     * @brief 线程函数
     * @return [*]
     * @note
     */
    void run();

    /**
     * @brief 动态分析函数
     * @param vecAllResult 模型识别结果
     * @return int
     */
    int dynamicAnalysis(const std::vector<LicensePlateCognition_NS::Result_S> &vecResult);

private:
    /* 句柄 */
    LicensePlateCognition_NS::CLicensePlateCognitionV1_0* m_pHandle = nullptr;

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

    /* 车牌识别配置 */
    Alarm::LicensePlateCognitionDetection_S m_stAlgoLicensePlateCognitionCfg;
    /* 报警状态管理 */
    CAlarmStateMachine m_LicensePlateStateMachine; 

    /* 检测频率控制 */
    EventManager m_RecvManager{500};

    /* 算法默认分辨率 */
    int m_nWidth = 640;
    int m_nHeight = 640;
};