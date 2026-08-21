/*
 * @Author: 梁浩尧 lianghaoyao@kfb.cn
 * @Date: 2025-11-05 10:38:00
 * @LastEditors: leiyy leiyy@kfb.cn
 * @LastEditTime: 2026-04-30 11:13:17
 * @FilePath: /1126/rv1126b_ipc/main_app/ai_app/algorithm_mode/algorithm/motion_detect/motion_detect.hpp
 * @Description: 移动侦测
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

class CMotionDetect : public CAlgorithm
{
public:

    CMotionDetect();
    ~CMotionDetect();

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
     * @brief   : 更新移动侦测参数 
     * @param    {MotionDetection_S} &stAlgoCfg：移动侦测
     */
    void setAlgoParamCfg(const Alarm::MotionDetection_S &stAlgoCfg);

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
     * @brief   : 计算灵敏度
     * @param    {std::vector<Common::RectInfo_S} &vstRectsInfo：移动侦测位置信息
     * @param    {int} &nMaxAreaIndex：移动侦测矩形面积最大下标
     * @param    {int} &nWidth：侦测区域的宽
     * @param    {int} &nHeight：侦测区域的高
     * @return   {bool} true：成功 false：失败
     */
    float calculate_sensitivity(const std::vector<Common::RectInfo_S> &vstRectsInfo, int &nMaxAreaIndex, int nWidth, int nHeight);

    /**
     * @brief 线程函数
     * @return [*]
     * @note
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
     * @param    {Common::RectInfo_S} r1：矩形1
     * @param    {Common::RectInfo_S} r2：矩形2
     * @return   {Common::RectInfo_S} 两个矩形的重叠矩形信息
     */
    Common::RectInfo_S intersectRect(const Common::RectInfo_S& r1, const Common::RectInfo_S& r2);
    /**
     * @brief   : 普通模式处理函数
     * @param    {std::vector<Common::RectInfo_S>} stRectInfo：算法输出结果
     */
    void processNormalMode(std::vector<Common::RectInfo_S> &vstRectsInfo);

    /**
     * @brief   : 专家模式处理函数 
     * @param    {std::vector<Common::RectInfo_S>} stRectInfo：算法输出结果
     */
    void processExpertMode(std::vector<Common::RectInfo_S>& stRectInfo);


private:

    /* 句柄 */
    MoveDetect_NS::CMoveDetectV2_0* m_pMotionDetHandle = nullptr;

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
    Alarm::MotionDetection_S m_stMotionDetCfg;

    /* 检测频率控制 */
    EventManager m_RecvManager{500};
    /* 当前通道号 */
    int m_nChannelId = 0;
    /* 移动侦测报警状态机，判断是否进行报警 */
    CAlarmStateMachine m_motionAlarmStateMachine;

    /* 侦测区域 */
    Common::Rect_S m_stRect;

    /* 缓存 RGA 处理后的结果*/
    cv::Mat m_fullRgbMat; 

    /* 是否需要裁剪源视频 */
    bool m_bIsCrop = false;

    /* 算法默认分辨率 */
    int m_nWidth = PIXEL_WIDTH_1280;
    int m_nHeight = PIXEL_HEIGHT_720;
    /* 是否绘制了区域 */
    bool m_bIsDraw = false;
};
