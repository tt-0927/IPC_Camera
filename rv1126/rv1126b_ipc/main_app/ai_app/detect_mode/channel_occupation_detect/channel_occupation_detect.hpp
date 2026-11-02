/*
 * @Author: zhengxh
 * @Date: 2026-10-10 10:30:00
 * @LastEditors: zhengxh
 * @LastEditTime: 2026-10-26 16:47:33
 * @FilePath: main_app/ai_app/detect_mode/channel_occupation_detect/channel_occupation_detect.hpp
 * @Description: 通道占用检测
 */

#pragma once

#include <atomic>
#include <mutex>
#include <thread>

#include "algorithm.hpp"
#include "blocking_queue.hpp"
#include "channel_occupation_detect_model.hpp"
#include "share_data.h"

class CChannelOccupationDetect : public CAlgorithm
{
public:

    CChannelOccupationDetect();
    ~CChannelOccupationDetect();

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
     * @brief   : 更新通道占用检测参数
     * @param    {ChannelOccupation_S} &stAlgoCfg：通道占用检测配置
     */
    void setAlgoParamCfg(const Alarm::ChannelOccupation_S &stAlgoCfg);

private:

    /**
     * @brief 用指定帧建立检测基准并初始化算法
     * @param [cv::Mat] stFrameMat：基准帧(对比帧+背景帧同源)
     * @return [*]
     * @note 模块要求对比帧/背景帧非空且与输入图片同尺寸 句柄只能由首帧建立
     */
    bool init(const cv::Mat &stFrameMat);

    /**
     * @brief 反初始化
     * @return [*]
     * @note
     */
    bool unInit();

    /**
     * @brief 线程函数
     * @return [*]
     * @note
     */
    void run();

    /**
     * @brief   : 配置区域转算法检测多边形
     * @return   {std::vector<cv::Point>} 多边形顶点 空表示整幅画面参与检测
     * @note    : 配置区域为1920x1080基准 需转换到算法输入分辨率
     */
    std::vector<cv::Point> convertRegionToPoints();

    /**
     * @brief   : 检测结果交报警状态机
     * @param    {CChannelOccupationDetectModel::OutData_S} &stOutData：检测结果
     * @note    : 目标运动中视为途经不报警 时间与状态类判断由状态机统一维护
     */
    void channelOccupationProcess(const CChannelOccupationDetectModel::OutData_S &stOutData);

private:

    /* 句柄 */
    CChannelOccupationDetectModel* m_pChannelOccupationDetHandle = nullptr;

    /* 队列 */
    BQ_NS::CBlockingQueue<MediaData_S> m_dateQueue;

    /* 用于控制线程的运行 */
    std::atomic<bool>       m_bRunning;
    /* 互斥锁保护指针访问 */
    std::mutex              m_mutex;
    /* 数据获取线程 */
    std::thread             m_thread;

    /* 配置参数 */
    Alarm::ChannelOccupation_S m_stChannelOccupationCfg;

    /* 检测频率控制 纯CPU算法默认1秒分析一帧 */
    EventManager m_RecvManager{1000};
    /* 当前通道号 */
    int m_nChannelId = 0;
    /* RGA输出的原始分辨率RGB帧 */
    cv::Mat m_stFullRgbMat;
    /* 需以当前帧重建基准(区域变更/配置推送) */
    std::atomic<bool> m_bBuildBaseline{false};

    /* 通道占用检测报警状态机，判断是否进行报警 */
    CAlarmStateMachine m_alarmStateMachine;

    /* 算法输入分辨率 与基准帧尺寸保持一致 */
    int m_nWidth = PIXEL_WIDTH_AI;
    int m_nHeight = PIXEL_HEIGHT_AI;

    /* 占用判定锁存 运动不打断已成立的占用 */
    bool m_bLastAlarmState = false;
};
