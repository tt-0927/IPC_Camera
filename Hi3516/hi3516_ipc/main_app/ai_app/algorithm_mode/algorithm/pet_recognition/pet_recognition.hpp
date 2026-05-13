/**
 * @FilePath     : pet_recognition.hpp
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2025-07-29 20:10:37
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2025-12-29 17:03:34
 * @Description  : 宠物识别
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

extern "C"
{
    #include "svp_ai_detect.h"
    #include "mpp_vgs.h"
}

class CPetRecognition : public CAlgorithm
{
public:

    CPetRecognition();
    ~CPetRecognition();

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
     * @brief   : 更新宠物识别参数
     * @param    {FaceDetection_S} &stAlgoCfg：宠物识别配置
     */
    void setAlgoParamCfg(const Alarm::PetRecognition_S &stAlgoCfg);

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
     * @brief   : 宠物识别处理函数
     * @param    {ot_aidetect_result_array} &stResult：算法输出结果
     * @param    {SEventProcessContext} &stCtx：事件处理上下文（包含通道号、时间戳、帧信息）
     */
    void processPetRecognition(ot_aidetect_result_array &stResult,
                               const SEventProcessContext &stCtx);

private:

    /* 宠物识别句柄 */
    HiAiDetect_S *m_pPetDetHandle = nullptr;
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
    /* 宠物识别报警状态机，判断是否进行报警 */
    CAlarmStateMachine m_petAlarmStateMachine;

    /* 配置参数 */
    /* 宠物识别 */
    Alarm::PetRecognition_S m_stPetDetCfg;

    /* 算法默认分辨率 */
    int m_nWidth = PIXEL_WIDTH_1024;
    int m_nHeight = PIXEL_HEIGHT_576;
};
