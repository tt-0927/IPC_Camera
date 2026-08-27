/*
 * @Author: 梁浩尧 lianghaoyao@kfb.cn
 * @Date: 2025-11-05 10:38:00
 * @LastEditors: 梁浩尧 lianghaoyao@kfb.cn
 * @LastEditTime: 2025-11-24 11:35:11
 * @FilePath: /1126/rv1126b_ipc/main_app/ai_app/algorithm_mode/algorithm/scene_change_detect/scene_change_detect.hpp
 * @Description: 场景变更
 */

#pragma once

#include <atomic>
#include <condition_variable>
#include <mutex>
#include <thread>

#include "algorithm.hpp"
#include "blocking_queue.hpp"
#include "MoveDetectV2_0.hpp"
#include "algo_control_deal.h"
#include "share_data.h"



class SceneChangeDetect : public CAlgorithm
{
public:

    SceneChangeDetect();
    ~SceneChangeDetect();

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

    /**
     * @brief   : 更新场景变更参数 
     * @param    {SceneChange_Ss} &stAlgoCfg：场景变更
     */
    void setAlgoParamCfg(const Alarm::SceneChange_S &stAlgoCfg);

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
     * @brief   : 重新启动
     * @return   {bool} true：成功 false：失败
     */
    bool reboot();

    /**
     * @brief 线程函数
     * @return [*]
     * @note
     */
    void run();

    /**
     * @brief   : 普通模式处理函数
     * @param    {std::vector<std::vector<int>>} stRectInfo：算法输出结果
     */
    bool sceneChangeDetectProcess(std::vector<std::vector<int>> &vstRectsInfo);

private:

    /* 句柄 */
    MoveDetect_NS::CMoveDetectV2_0* m_pSceneChangeDetHandle = nullptr;

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
    
    /* 配置参数 */
    Alarm::SceneChange_S m_stSceneChangeDetCfg;

    /* 检测频率控制 */
    EventManager m_RecvManager{500};
    /* 当前通道号 */
    int m_nChannelId = 0;
    /* 缓存最新RGB帧用于TVSDK图像推送 */
    cv::Mat m_stInDataMat;
    /* 场景变更参考帧更新频率控制:默认10分钟检测一次变更 */
    time_t m_UpdateLastFrameDuration = 10 * 60;
    /* 更新上一帧缓存的时间戳 */
    time_t m_LastFrameFrameTime = 0;

    /* 场景变更侦测报警状态机，判断是否进行报警 */
    CAlarmStateMachine m_sceneChangeAlarmStateMachine;
    /* 用于计算触发场景变更的时长是否达到阈值 */
    time_t m_duration = 0;

    /* 算法默认分辨率 */
    int m_nWidth = PIXEL_WIDTH_AI;
    int m_nHeight = PIXEL_HEIGHT_AI;
};
