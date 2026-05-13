/**
 * @FilePath     : isp_dayNight.h
 * @Author       : cyc
 * @Date         : 2025-08-27 19:03:00
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-04-21 17:03:58
 * @Description  : 日夜切换控制器实现
 */

#pragma once
#include <thread>
#include <atomic>
#include <chrono>
#include <functional>
#include <memory>
#include "isp_define.h"
#include "ot_mpi_isp.h"

#define BRIGHT_DAY_TO_NIGHT_VALUE     (150000000) /* 白天切夜间当前亮度阈值  */
#define BRIGHT_RED_LIGHT_TO_DAY_VALUE (10000000)  /* 夜间红外切白天当前亮度阈值  */
#ifdef FOCAL_4MM
#define BRIGHT_WHITE_LIGHT_TO_DAY_VALUE (7000000) /* 夜间白灯切白天当前亮度阈值  */
#elif defined(FOCAL_6MM)
#define BRIGHT_WHITE_LIGHT_TO_DAY_VALUE (3000000)  /* 夜间白灯切白天当前亮度阈值  */
#else                                              /* 2.8、8mm */
#define BRIGHT_WHITE_LIGHT_TO_DAY_VALUE (40000000) /* 夜间白灯切白天当前亮度阈值  */
#endif

#define RED_RG_VALUE (240) /* 夜间红外切白天rg阈值  */
#define RED_BG_VALUE (240) /* 夜间红外切白天bg阈值  */

#ifdef FOCAL_4MM
#define WHITE_RG_VALUE (145) /* 夜间白灯切白天rg阈值  */
#else
#define WHITE_RG_VALUE (160) /* 夜间白灯切白天rg阈值  */
#endif
#define WHITE_BG_VALUE (185) /* 夜间白灯切白天bg阈值  */

#define GAIN_MAX_COEF (280)
#define GAIN_MIN_COEF (190)
#define SHIFT_8BIT    (8)
#define IR_SENSITIVITY_VALUE (1) /* ir切换灵敏度默认值 */

using namespace ISP;

class CDayNightController : public CSingleton<CDayNightController>
{
public:
    /* 日夜切换状态变化回调 */
    using StateChangeCallback = std::function<void(bool isNight, DayNightMode_E mode)>;

    CDayNightController();
    ~CDayNightController();
    friend class CSingleton<CDayNightController>;

    /**
     * @brief 启动日夜切换控制
     */
    bool start();

    /**
     * @brief 停止日夜切换控制
     */
    void stop();

    /**
     * @brief 设置日夜切换模式
     */
    void setMode(DayNightMode_E mode);

    /**
     * @brief 获取当前模式
     */
    DayNightMode_E getMode() const
    {
        return m_currentMode;
    }

    /**
     * @brief 设置时间范围（定时模式使用）
     */
    void setTimeRange(const TimeRange_S &timeRange);

    /**
     * @brief 设置灵敏度（自动模式使用）
     */
    void setSensitivity(unsigned int sensitivity);

    /**
     * @brief 设置过滤时间（自动模式使用）
     */
    void setFilterTime(unsigned int filterTimeSeconds);

    /**
     * @brief 获取当前是否为夜晚
     */
    bool isNightMode() const
    {
        return m_isNight;
    }

    /**
     * @brief 设置状态变化回调
     */
    void setStateChangeCallback(StateChangeCallback callback);

    /**
     * @brief 强制切换到指定状态（手动模式使用）
     */
    void forceSwitch(bool toNight);

    void setFillLight();

private:
    /**
     * @brief 日夜切换主线程
     */
    void workerThread();

    /**
     * @brief 自动模式处理
     */
    void handleAutoMode();

    /**
     * @brief 定时模式处理
     */
    void handleTimeMode();

    /**
     * @brief 检查是否应该处于夜晚模式（定时模式）
     */
    bool shouldBeNightByTime() const;

    /**
     * @brief 检查过滤时间是否满足
     */
    bool isFilterTimeSatisfied() const;

    /**
     * @brief 执行状态切换
     */
    void performStateChange(bool toNight);

    /**
     * @brief 检查是否满足最小切换间隔（防止过于频繁切换）
     */
    bool isMinIntervalSatisfied() const;

private:
    enum class SwitchState_E
    {
        IDLE,      /* 条件未满足（计时器未启动） */
        FIRST_MET, /* 条件首次满足（计时器已启动） */
        WAITING,   /* 条件持续满足（计时中） */
        TRIGGERED  /* 已达到过滤时间，等待切换 */
    };

    std::atomic<SwitchState_E> m_switchState{ SwitchState_E::IDLE };

    /* 线程控制 */
    std::unique_ptr<std::thread> m_workerThread;
    std::atomic<bool> m_running{ false };

    /* 状态管理 */
    std::atomic<bool> m_isNight{ false };
    std::atomic<DayNightMode_E> m_currentMode{ AUTO_MODE };
    /* 首次强制切换标志 */
    std::atomic<bool> m_firstForcedSwitch{ true };
    /* 首次状态同步标志，确保启动后至少执行一次硬件同步 */
    std::atomic<bool> m_firstStateSynced{ false };

    /* 配置参数 */
    TimeRange_S m_timeRange;
    std::atomic<unsigned int> m_sensitivity{ IR_SENSITIVITY_VALUE };
    std::atomic<unsigned int> m_filterTime{ FILTER_TIME_MIN };
    /* 灯光类型 */
    LightType_E m_enLightType;

    /* ISP相关 */
    ot_isp_ir_auto_attr m_irAttr;
    ot_isp_exp_info isp_exp_info;
    int m_viPipe{ 0 };

    /* 过滤控制 */
    std::chrono::steady_clock::time_point m_lastSwitchTime;
    std::chrono::steady_clock::time_point m_conditionStartTime; /* 条件开始满足的时间 */
    std::atomic<bool> m_needSwitch{ false };
    std::atomic<bool> m_conditionMet{ false }; /* 标记条件是否满足 */
    std::atomic<bool> m_thresholdNeedsUpdate{ false };

    /* 回调 */
    StateChangeCallback m_stateChangeCallback;

    static constexpr auto THREAD_SLEEP_INTERVAL = std::chrono::milliseconds(40);
    static constexpr unsigned int NORMAL_TO_IR_THRESHOLD = 24000; // 白天到夜晚（开灯的最大阈值）
    static constexpr unsigned int IR_TO_NORMAL_THRESHOLD = 500;   // 夜晚到白天（关灯的最小阈值）

    /* 灵敏度与白天切夜间的亮度阈值对照 */
    std::unordered_map<int, uint32_t> m_mapBrightDayToNight = {
        { 0, 160000000 }, // 极低灵敏度
        { 1, 155000000 },
        { 2, 150000000 }, // 默认/标准灵敏度
        { 3, 125000000 }, // 略微提升灵敏度
        { 4, 100000000 }, // 中高灵敏度
        { 5,  80000000 },
        { 6,  60000000 },
        { 7,  40000000 }, // 极高灵敏度：较早切夜间
    };
};
