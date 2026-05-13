/**
 * @FilePath     : scene_change_detect.hpp
 * @Author       : zhouzirui
 * @Date         : 2025-06-06 16:02:10
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2025-12-29 17:03:53
 * @Description  : 场景变更侦测
 */

#pragma once

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <mutex>
#include <thread>
#include <algorithm>
#include <sys/time.h>

#include "blocking_queue.hpp"
#include "common_process.h"
#include "algorithm.hpp"

extern "C"
{
    #include "svp_md.h"
    #include "mpp_vgs.h"
}

class CSceneChangeDetect : public CAlgorithm
{
public:

    CSceneChangeDetect();
    ~CSceneChangeDetect();

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
     * @brief   : 更新场景变更侦测参数 
     * @param    {SceneChange_S} &stAlgoCfg：场景变更侦测
     */
    void setAlgoParamCfg(const Alarm::SceneChange_S &stAlgoCfg);

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
     * @brief   : 判断当前是否为白天（专家模式用）
     * @return   {bool} true：白天 false：夜晚
     */
    bool isDaytime() const;

    /**
     * @brief   : 计算两个矩形的重叠面积
     * @param    {Common::Rect_S} rect1：矩形1
     * @param    {Common::Rect_S} rect2：矩形2
     * @return   {int} 重叠面积
     */
    int calculateOverlapArea(const Common::Rect_S& rect1, const Common::Rect_S& rect2) const;

    /**
     * @brief   : 将算法输出的点坐标转换为矩形
     * @param    {ot_sample_svp_rect} rect：算法输出的矩形点
     * @return   {Common::Rect_S} 转换后的矩形
     */
    Common::Rect_S convertToRect(const ot_sample_svp_rect& rect) const;

    /**
     * @brief   : 场景变更侦测后处理函数
     * @param    {ot_sample_svp_rect_info} stRectInfo：算法输出结果
     * @param    {SEventProcessContext} stCtx：事件处理上下文
     */
    void processSceneChangeMode(ot_sample_svp_rect_info& stRectInfo,
                                const SEventProcessContext &stCtx);

private:

    /* 场景变更侦测句柄 */
    HiMd_S *m_pSceneChangeDetHandle = nullptr;
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
    /* 场景变更参考帧更新频率控制:默认10分钟检测一次变更 */
    EventManager m_UpdateManager{10 * 60 * 1000};
    /* 场景变更侦测报警状态机，判断是否进行报警 */
    CAlarmStateMachine m_sceneChangeAlarmStateMachine;
    /* 配置参数 */
    /* 场景变更侦测 */
    Alarm::SceneChange_S m_stSceneChangeDetCfg;

    /* 算法默认分辨率 */
    int m_nWidth = PIXEL_WIDTH_1024;
    int m_nHeight = PIXEL_HEIGHT_576;
};
