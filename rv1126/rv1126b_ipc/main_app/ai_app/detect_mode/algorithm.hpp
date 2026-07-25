/**
 * @FilePath     : algorithm.hpp
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2025-06-09 11:04:21
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2025-07-21 15:07:23
 * @Description  : 算法基类
 */

#pragma once

#include <iostream>

#include "IpcRet.h"
#include "SignalSlot.h"
#include "dlog.h"
#include "event_define.h"
#include "alarm_define.h"
#include "stream_process_ext.hpp"
#include "face_manage_ext.hpp"
#include "event_manager.hpp"
#include "execution_timer.hpp"
#include "StatisticsTimer.hpp"
#include "event_linkage.h"

/* 事件结束事件阈值 x秒 （事件触发后，超过阈值时间未再次触发事件，则视为事件结束） */
#define EVENT_END_TIME_THRESHOLD (3)

/* 算法基类 */
class CAlgorithm
{
public:

    CAlgorithm()
    {
    }

    virtual ~CAlgorithm()
    {
    }

    /**
     * @brief 接受媒体数据
     * @param [MediaData_S] stMediaData:
     * @return [*]
     * @note
     */
    virtual void recvMediaData(MediaData_S stMediaData) = 0;

    /**
     * @brief 更新算法配置参数
     * @param stAlgoConfig 
     */
    virtual void setAlgoEnCfg(const Event::AlgorithmConfig &stAlgoConfig) {}
    
    /**
     * @brief 更新检测划线数据
     * @param stRuleInfo 
     */
    virtual void changeRuleInfos(const Event::RuleInfo &stRuleInfo, bool bNeedClear) {}

    /**
     * @brief 添加人脸名单库
     * @param stFaceLibData 
     */
    virtual bool addFaceLibGroup(FaceDataDB_NS::FaceLibsInfo_S &stFaceLibData) { return 0; }

    /**
     * @brief 人员检索人脸比对
     * @param strPicPath 
     */
    virtual bool faceRetrieval(std::string strPicPath) { return 0; }

    /**
     * @brief 两张人脸相似度比对
     * @param strPicPath1 
     * @param strPicPath2 
     * @return float 
     */
    virtual float facesCompared(std::string strPicPath1, std::string strPicPath2) { return 0; }

    /**
     * @brief 更新算法配置参数
     * @param stAlgoCfg 
     */
    // virtual void setAlgoParamCfg(const Alarm::TargetDetection_S &stAlgoCfg) {}

    /**
     * @brief   : 获取当前实时音量
     * @note    : 音频异常侦测
     * @return   {float} 当前音量(dB)
     */
    virtual float getCurrentDb() const { return 0.0f; }
};


class TVTimer
{
public:
    /* 开始计时 */
    void start()
    {
        gettimeofday(&tv_start, nullptr);
        running = true;
    }

    /* 结束计时 */
    void stop()
    {
        gettimeofday(&tv_end, nullptr);
        running = false;
    }

    /* 打印耗时（毫秒），如果未调用 stop() 则以当前时间为终点 */
    void print(const char *tag = "") const
    {
        timeval tv_now;
        if (running)
        {
            gettimeofday(&tv_now, nullptr);
        }
        else
        {
            tv_now = tv_end;
        }

        long seconds = tv_now.tv_sec - tv_start.tv_sec;
        long useconds = tv_now.tv_usec - tv_start.tv_usec;
        long ms = seconds * 1000 + useconds / 1000;

        if (*tag)
        {
            std::cout << "[" << tag << "] ";
        }
        std::cout << "Elapsed: " << ms << " ms\n";
    }

private:
    struct timeval tv_start{};
    struct timeval tv_end{};
    bool running = false;
};

/* 报警状态机 */
class CAlarmStateMachine
{
public:
    CAlarmStateMachine() = default;
    ~CAlarmStateMachine() = default;

    /**
     * @brief   : 处理报警状态，进行事件触发
     * @param    {bool} bAlarm 是否报警
     * @param    {Type_E} enEventType 事件类型
     */
    void handleAlarmState(bool bAlarm, Event::Type_E enEventType)
    {
        /* 全局告警状态机逻辑 */
        auto now = std::chrono::steady_clock::now();

        if (bAlarm)
        {
            /* 条件满足，检查是否是新事件 */
            if (!m_bIsAlarmActive)
            {
                dlog_info("事件[%d]开始", enEventType);
                /* 发送事件开始消息 */
                CEventLinkage::instance()->handleEvent(enEventType, false);
                m_bIsAlarmActive = true;
            }
            else
            {
                /* 事件持续中，不做任何操作 */
                dlog_info("事件[%d]持续中", enEventType);
            }
            /* 更新最后满足条件的时间戳 */
            m_lastAlarmTimestamp = now;
        }
        else
        {
            /* 条件不满足，检查是否需要结束事件 */
            if (m_bIsAlarmActive)
            {
                auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - m_lastAlarmTimestamp);
                /* 如果距离上一次满足条件已超过阈值，则判定事件结束 */
                if (elapsed.count() >= EVENT_END_TIME_THRESHOLD)
                {
                    dlog_info("事件[%d]结束", enEventType);
                    /* 发送事件结束消息 */
                    CEventLinkage::instance()->handleEvent(enEventType, true);
                    m_bIsAlarmActive = false;
                }
            }
        }
    }

private:
    /* 报警状态是否处于活跃状态 */
    bool m_bIsAlarmActive = false;
    /* 上次触发报警的时间点 */
    std::chrono::steady_clock::time_point m_lastAlarmTimestamp;
};
