/**
 * @FilePath     : algorithm.hpp
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2025-06-09 11:04:21
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-08-13 18:07:12
 * @Description  : 算法基类
 */

#pragma once

#include <iostream>
#include <memory>
#include <string>

#include "IpcRet.h"
#include "SignalSlot.h"
#include "dlog.h"
#include "event_define.h"
#include "alarm_define.h"
#include "stream_process_ext.hpp"
#include "face_manage_ext.hpp"
#include "event_manager.hpp"
#include "event_linkage.h"
#include "execution_timer.hpp"
#include "statistics_timer.hpp"

#include "YoloUltralytics_rpn.hpp"
#include "event_alarm/statistics/event_statistics_reporter.hpp"

extern "C"
{
    #include "mpp_vgs.h"
}

/* 事件结束事件阈值 x秒 （事件触发后，超过阈值时间未再次触发事件，则视为事件结束） */
#define EVENT_END_TIME_THRESHOLD (3)
/* 事件两次开始之间的最小间隔（秒），默认10秒 */
#define EVENT_MIN_TRIGGER_INTERVAL_SECONDS (10)
/* 事件冷却期时长（秒），需扣除结束判定阈值，保证整体触发间隔约为10秒 */
#define EVENT_COOLDOWN_SECONDS                                                                                      \
    ((EVENT_MIN_TRIGGER_INTERVAL_SECONDS > EVENT_END_TIME_THRESHOLD) ?                                               \
         (EVENT_MIN_TRIGGER_INTERVAL_SECONDS - EVENT_END_TIME_THRESHOLD) :                                           \
         0)
/* 移动侦测单次事件最长持续时间，超时后强制结束以允许下一轮联动 */
#define MOTION_EVENT_MAX_DURATION_SECONDS (60)

/* 算法基类 */
struct RuntimeCommand_S
{
    /* 运行时命令码，通常复用 action_code.h 中的 AC_* 常量 */
    int nCode = 0;
    /* 字符串载荷，预留给后续 JSON 或轻量参数传递 */
    std::string strPayload;
    /* 扩展数据指针，仅用于同进程内短生命周期参数传递 */
    void *pData = nullptr;
};

/**
 * @brief   : 单帧事件处理上下文
 * @note    : 封装当前帧处理所需的通用上下文信息，用于简化后处理函数签名
 */
struct SEventProcessContext
{
    /* 当前媒体通道号 */
    int nChnId = 0;
    /* 当前帧时间戳，单位毫秒 */
    long long llTimestamp = 0;
    /* 当前帧视频帧指针，用于事件报警时编码触发帧图片，无视频帧时为 nullptr（如音频检测） */
    ot_video_frame_info *pFrameInfo = nullptr;
};

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
     * @brief   : 注入事件统计上报器
     * @param    {std::shared_ptr<EventStatistics_NS::IEventStatisticsReporter>} &pReporter：上报器
     * @return   {void}
     */
    virtual void setEventStatisticsReporter(
        const std::shared_ptr<EventStatistics_NS::IEventStatisticsReporter> &pReporter)
    {
        (void)pReporter;
    }

    /**
     * @brief   : 处理运行时命令
     * @param    {RuntimeCommand_S} &stCommand：运行时命令
     * @return   {int} OK：处理成功 ERR：未处理或失败
     */
    virtual int handleRuntimeCommand(const RuntimeCommand_S &stCommand)
    {
        (void)stCommand;
        return ERR;
    }

    /**
     * @brief 添加人脸名单库
     * @param stFaceLibData
     */
    virtual int addFaceLibGroup(FaceDataDB_NS::FaceLibsInfo_S &stFaceLibData) { return 0; }

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
    /**
     * @brief   : 构造函数
     * @param    {int} nCooldownSeconds 冷却期时长（秒）
     */
    explicit CAlarmStateMachine(int nCooldownSeconds = EVENT_COOLDOWN_SECONDS) : m_nCooldownSeconds(nCooldownSeconds)
    {
    }

    ~CAlarmStateMachine() = default;

    /**
     * @brief   : 处理报警状态，兼容旧事件类型入口
     * @param    {bool} bAlarm：是否满足报警条件
     * @param    {Event::Type_E} enEventType：事件类型
     * @return   {bool} true：报警状态活跃 false：报警状态未激活
     * @note    : 内部会构造默认 EventTriggerContext_S，并统一走新的上下文联动接口
     */
    bool handleAlarmState(bool bAlarm, Event::Type_E enEventType)
    {
        /* 默认事件触发上下文，仅携带旧接口已有的事件类型 */
        EventTriggerContext_S stContext;
        stContext.enEventType = enEventType;
        return handleAlarmState(bAlarm, stContext);
    }

    /**
     * @brief   : 处理报警状态，使用新事件触发上下文进行联动
     * @param    {bool} bAlarm：是否满足报警条件
     * @param    {EventTriggerContext_S} &stContext：事件触发上下文
     * @return   {bool} true：报警状态活跃 false：报警状态未激活
     * @note    : 事件开始和结束都会调用 CEventLinkage::handleEvent(const EventTriggerContext_S &)
     */
    bool handleAlarmState(bool bAlarm, const EventTriggerContext_S &stContext)
    {
        if (stContext.enEventType == Event::Type_E::UNKNOWN)
        {
            dlog_warn("报警状态机收到未知事件类型，忽略本次状态处理");
            return m_bIsAlarmActive;
        }

        /* 当前单调时钟时间点，用于状态机内部冷却与结束阈值判断 */
        auto now = std::chrono::steady_clock::now();

        if (bAlarm)
        {
            /* 条件满足，检查是否是新事件 */
            if (!m_bIsAlarmActive)
            {
                /* 检查是否在冷却期内 */
                if (m_bInCooldown)
                {
                    auto cooldownElapsed = std::chrono::duration_cast<std::chrono::seconds>(now - m_lastAlarmEndTimestamp);

                    if (cooldownElapsed.count() < m_nCooldownSeconds)
                    {
                        /* 在冷却期内，不允许开始新事件 */
                        dlog_info("事件[%d]在冷却期内，剩余时间：%d秒",
                                  static_cast<int>(stContext.enEventType),
                                  static_cast<int>(m_nCooldownSeconds - cooldownElapsed.count()));
                        return false;
                    }
                    else
                    {
                        /* 冷却期已过 */
                        m_bInCooldown = false;
                    }
                }

                /* 事件开始上下文，强制设置为开始阶段，避免调用方误传结束标记 */
                EventTriggerContext_S stStartContext = stContext;
                stStartContext.bEventEnded = false;

                dlog_info("事件[%d]开始", static_cast<int>(stStartContext.enEventType));
                /* 使用新的上下文联动入口，支持通道、规则和扩展属性匹配 */
                CEventLinkage::instance()->handleEvent(stStartContext);

                /* 缓存最近一次激活上下文，结束事件需要复用同一事件属性 */
                m_stActiveContext = build_active_context_cache(stStartContext);
                m_bIsAlarmActive = true;
                m_alarmStartTimestamp = now;
            }
            else
            {
                const auto activeElapsed = std::chrono::duration_cast<std::chrono::seconds>(now - m_alarmStartTimestamp);
                if ((stContext.enEventType == Event::Type_E::MOTION_DETECT || 
                    stContext.enEventType == Event::Type_E::OCCLUSION_DETECT ||
                    stContext.enEventType == Event::Type_E:: GARBAGE_OVERFLOW ||
                    stContext.enEventType == Event::Type_E:: GARBAGE_EXPOSURE) &&
                    activeElapsed.count() >= MOTION_EVENT_MAX_DURATION_SECONDS)
                {
                    dlog_info("移动侦测事件持续[%lld]秒，达到单次事件上限，强制结束",
                              static_cast<long long>(activeElapsed.count()));
                    endAlarmImmediately(stContext);
                    return m_bIsAlarmActive;
                }
                /* 事件持续中，不做任何操作 */
                dlog_debug("事件[%d]持续中", static_cast<int>(stContext.enEventType));

                /* 持续报警阶段刷新上下文，确保结束事件能拿到最新摘要属性 */
                m_stActiveContext = build_active_context_cache(stContext);
                m_stActiveContext.bEventEnded = false;
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
                    /* 事件结束上下文优先复用激活上下文，保证联动规则属性前后一致 */
                    EventTriggerContext_S stEndContext = m_stActiveContext;
                    if (stEndContext.enEventType == Event::Type_E::UNKNOWN)
                    {
                        stEndContext = stContext;
                    }
                    stEndContext.bEventEnded = true;
                    if (stContext.llTimestamp > 0)
                    {
                        /* 结束事件使用当前处理帧时间，避免沿用开始阶段时间戳 */
                        stEndContext.llTimestamp = stContext.llTimestamp;
                    }

                    dlog_info("事件[%d]结束", static_cast<int>(stEndContext.enEventType));
                    /* 使用新的上下文联动入口，结束阶段也保留属性匹配能力 */
                    CEventLinkage::instance()->handleEvent(stEndContext);
                    m_bIsAlarmActive = false;

                    /* 进入冷却期 */
                    m_bInCooldown = true;
                    m_lastAlarmEndTimestamp = now;
                    m_alarmStartTimestamp = std::chrono::steady_clock::time_point();
                    m_stActiveContext = EventTriggerContext_S();
                }
            }
        }
        return m_bIsAlarmActive;
    }

    /**
     * @brief   : 强制立即结束当前报警状态，兼容旧事件类型入口
     * @param    {Event::Type_E} enEventType：事件类型
     * @return   {bool} 是否成功结束
     * @note    : 内部会构造默认 EventTriggerContext_S，并统一走新的上下文联动接口
     */
    bool endAlarmImmediately(Event::Type_E enEventType)
    {
        /* 默认事件触发上下文，仅携带旧接口已有的事件类型 */
        EventTriggerContext_S stContext;
        stContext.enEventType = enEventType;
        return endAlarmImmediately(stContext);
    }

    /**
     * @brief   : 强制立即结束当前报警状态
     * @param    {EventTriggerContext_S} &stContext：事件触发上下文
     * @return   {bool} 是否成功结束
     * @note    : 不检查 EVENT_END_TIME_THRESHOLD，直接触发事件结束联动，适用于人脸抓拍等瞬时事件
     */
    bool endAlarmImmediately(const EventTriggerContext_S &stContext)
    {
        if (!m_bIsAlarmActive)
        {
            return false;
        }

        /* 当前单调时钟时间点 */
        auto now = std::chrono::steady_clock::now();

        /* 事件结束上下文优先复用激活上下文，保证联动规则属性前后一致 */
        EventTriggerContext_S stEndContext = m_stActiveContext;
        if (stEndContext.enEventType == Event::Type_E::UNKNOWN)
        {
            stEndContext = stContext;
        }
        stEndContext.bEventEnded = true;
        if (stContext.llTimestamp > 0)
        {
            /* 结束事件使用当前处理帧时间，避免沿用开始阶段时间戳 */
            stEndContext.llTimestamp = stContext.llTimestamp;
        }

        dlog_info("事件[%d]强制结束", static_cast<int>(stEndContext.enEventType));
        /* 使用新的上下文联动入口，结束阶段也保留属性匹配能力 */
        CEventLinkage::instance()->handleEvent(stEndContext);
        m_bIsAlarmActive = false;

        /* 进入冷却期 */
        m_bInCooldown = true;
        m_lastAlarmEndTimestamp = now;
        m_stActiveContext = EventTriggerContext_S();

        return true;
    }

    /**
     * @brief   : 判断当前帧是否可能开启新的报警事件
     * @return  : true 表示允许当前调用方准备事件开始负载，false 表示事件活跃或仍在冷却
     * @note    : 该接口只做无副作用预判，调用方应紧接着调用 handleAlarmState()；用于避免在持续帧上重复编码大图。
     */
    bool canStartAlarm() const
    {
        if (m_bIsAlarmActive)
        {
            return false;
        }

        if (!m_bInCooldown)
        {
            return true;
        }

        const auto now = std::chrono::steady_clock::now();
        const auto cooldownElapsed = std::chrono::duration_cast<std::chrono::seconds>(now - m_lastAlarmEndTimestamp);
        return cooldownElapsed.count() >= m_nCooldownSeconds;
    }

    /**
     * @brief   : 重置状态机
     */
    void reset()
    {
        m_bIsAlarmActive = false;
        m_bInCooldown = false;
        m_lastAlarmTimestamp = std::chrono::steady_clock::time_point();
        m_lastAlarmEndTimestamp = std::chrono::steady_clock::time_point();
        m_alarmStartTimestamp = std::chrono::steady_clock::time_point();
        m_stActiveContext = EventTriggerContext_S();
    }

    /**
     * @brief   : 获取是否在冷却期
     * @return   {bool} 是否在冷却期
     */
    bool isInCooldown() const
    {
        return m_bInCooldown;
    }

    /**
     * @brief   : 设置冷却期时长
     * @param    {int} nCooldownSeconds 冷却期时长（秒）
     */
    void setCooldownSeconds(int nCooldownSeconds)
    {
        if (nCooldownSeconds >= 0)
        {
            m_nCooldownSeconds = nCooldownSeconds;
        }
    }

    /**
     * @brief   : 获取冷却期时长
     * @return   {int} 冷却期时长（秒）
     */
    int getCooldownSeconds() const
    {
        return m_nCooldownSeconds;
    }

private:
    /**
     * @brief   : 构造状态机内部缓存上下文
     * @param    {EventTriggerContext_S} &stContext：待缓存的事件触发上下文
     * @return   {EventTriggerContext_S} 可安全长期缓存的上下文
     * @note    : 默认不缓存 TVSDK 负载，避免持续报警阶段长期占用大块内存
     */
    EventTriggerContext_S build_active_context_cache(const EventTriggerContext_S &stContext) const
    {
        EventTriggerContext_S stCachedContext = stContext;
        stCachedContext.stPanoramaImage = EventTvSdkImage_S();
        stCachedContext.stTargetImage = EventTvSdkImage_S();
        if (stCachedContext.pTvSdkPayload && !stCachedContext.pTvSdkPayload->bAllowCacheInStateMachine)
        {
            stCachedContext.pTvSdkPayload.reset();
        }
        return stCachedContext;
    }

    /* 报警状态是否处于活跃状态 */
    bool m_bIsAlarmActive = false;

    /* 是否在冷却期（事件结束后的间隔期） */
    bool m_bInCooldown = false;

    /* 冷却期时长（秒） */
    int m_nCooldownSeconds = EVENT_COOLDOWN_SECONDS;

    /* 上次触发报警的时间点 */
    std::chrono::steady_clock::time_point m_lastAlarmTimestamp;

    /* 当前事件开始时间，用于限制单次移动侦测事件最长持续 60 秒 */
    std::chrono::steady_clock::time_point m_alarmStartTimestamp;
    
    /* 上次事件结束的时间点 */
    std::chrono::steady_clock::time_point m_lastAlarmEndTimestamp;

    /* 最近一次激活事件上下文，用于事件结束阶段复用属性与通道信息 */
    EventTriggerContext_S m_stActiveContext;
};
