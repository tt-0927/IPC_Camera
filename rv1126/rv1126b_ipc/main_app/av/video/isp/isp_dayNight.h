/*** 
 * @FilePath     : isp_dayNight.h
 * @Author       : cyc
 * @Date         : 2025-08-27 19:03:00
 * @LastEditors  : cyc
 * @LastEditTime : 2025-11-25 19:36:23
 * @Description  : 日夜切换控制器实现
 */

 #pragma once
 #include <thread>
 #include <atomic>
 #include <chrono>
 #include <functional>
 #include <memory>
 #include "isp_define.h"
 #include "Singleton.h"  
 #include <rk_aiq_user_api2_camgroup.h>
 #include <rk_aiq_user_api2_imgproc.h>
 #include <rk_aiq_user_api2_sysctl.h>
 #include "rk_smart_ir_api.h"
 
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
    DayNightMode_E getMode() const { return m_currentMode.load(); }
    
    /**
    * @brief 设置时间范围（定时模式使用）
    */
    void setTimeRange(const TimeRange_S& timeRange);
    
    /**
    * @brief 设置灵敏度（自动模式使用）
    * @note 灵敏度映射到d2n/n2d阈值，1-10。这里简化处理，实际可根据需求调整。
    */
    void setSensitivity(unsigned int sensitivity);
    
    /**
    * @brief 设置过滤时间（自动模式使用）
    */
    void setFilterTime(unsigned int filterTimeSeconds);
    
    /**
    * @brief 获取当前是否为夜晚
    */
    bool isNightMode() const { return m_isNight.load(); }
    
    /**
    * @brief 设置状态变化回调
    */
    void setStateChangeCallback(StateChangeCallback callback);
         
 private:
    /**
    * @brief 日夜切换主线程 (主要用于定时模式)
    */
    void workerThread();
        
    /**
    * @brief 定时模式处理
    */
    void handleTimeMode();
    
    /**
    * @brief 检查是否应该处于夜晚模式（定时模式）
    */
    bool shouldBeNightByTime() const;
    
    /**
    * @brief 执行状态切换的核心逻辑
    */
    void performStateChange(bool toNight);

    /**
    * @brief 更新 rk_smart_ir 的属性
    */
    void updateSmartIrAttr();

    /**
    * @brief [核心] rk_smart_ir 的静态回调函数
    */
    static void smartIr_cb(rk_smart_ir_result_t result);

    /**
    * @brief [核心] 处理 rk_smart_ir 回调的实例成员函数
    */
    void handleSmartIrCallback(rk_smart_ir_result_t result);

    void sample_smartIr_calib();
    
 private:
    /* 线程控制 */ 
    std::unique_ptr<std::thread> m_workerThread;
    std::atomic<bool> m_running{false};
    
    /* 状态管理 */ 
    std::atomic<bool> m_isNight{false};
    std::atomic<DayNightMode_E> m_currentMode{AUTO_MODE};
    
    /* 配置参数 */ 
    TimeRange_S m_timeRange;
    std::atomic<unsigned int> m_sensitivity{5}; // 灵敏度范围 1-10
    
    /* ISP相关 */ 
    rk_aiq_sys_ctx_t* aiq_ctx;
    rk_smart_ir_ctx_t* smartIr_ctx;
    rk_smart_ir_attr_t m_stSmartAttr;
    
    /* 回调 */ 
    StateChangeCallback m_stateChangeCallback;
    
    static constexpr auto THREAD_SLEEP_INTERVAL = std::chrono::seconds(1); // 定时模式每秒检查一次即可
 };
 