/*** 
 * @FilePath     : light_manager.h
 * @Author       : cyc
 * @Date         : 2025-08-29 14:31:43
 * @LastEditors  : cyc
 * @LastEditTime : 2025-09-01 19:31:39
 * @Description  : 灯光管理模块
 */

 #pragma once

 #include "Singleton.h"
 #include "isp_define.h"
 #include "system_define.h"
 #include "alarm_define.h"
 #include <condition_variable>
 #include <mutex>
 #include <atomic>
 #include <thread>
 
using namespace ISP;

/* 闪烁参数结构 */
typedef struct _FlashParam_S_
{
    LightType_E enLightType;        /* 灯光类型 */
    int nFlashTime;                 /* 闪烁时间，单位：秒 [0-300] */
    Alarm::FlashFrequency_E enFrequency;   /* 闪烁频率 */
    bool bIsFlashing;               /* 是否正在闪烁 */
    bool bCurrentState;             /* 当前灯光状态，true=亮，false=灭 */
    std::chrono::steady_clock::time_point startTime; /* 开始时间 */
    std::chrono::steady_clock::time_point lastToggleTime; /* 上次切换时间 */
    
    _FlashParam_S_() : enLightType(LIGHT_TYPE_WHITE), nFlashTime(0), 
                      enFrequency(Alarm::FlashFrequency_E::FLASH_LOW_FREQ), 
                      bIsFlashing(false), bCurrentState(false) {}
} FlashParam_S;

class CLightManager : public CSingleton<CLightManager>
{
public:
    CLightManager();
    ~CLightManager();
    friend class CSingleton<CLightManager>;

    /*** 
     * @description : 灯光管理初始化
     * @author      : cyc
     * @return       {*}
     */    
    int init();

    /*** 
     * @description : 灯光管理去初始化
     * @author      : cyc
     * @return       {*}
     */  
    int deinit();

    /**
    * @brief 设置外设配置（影响白灯控制）
    */
    int set_peripheral_config(const System::Peripheral_S& config);
    
    /**
    * @brief 获取外设配置
    */
    int get_peripheral_config(System::Peripheral_S& config);
    
    /**
    * @brief 处理日夜切换信号（来自ISP模块）
    */
    void on_dayNight_changed(bool isNight, const FillLight_S& lightConfig);
    
    /**
    * @brief 手动控制灯光（用于闪烁等功能）
    */
    int manual_light_control(LightType_E lightType, bool enable, unsigned int brightness = 50);

    /*** 
     * @description : 开始闪烁
     * @author      : cyc
     * @param        {LightType_E} enLightType，灯光类型
     * @param        {int} nFlashTime，闪烁时间（秒），1-300
     * @param        {FlashFrequency_E} enFrequency，闪烁频率
     * @return       [int] - 0成功，-1失败
     */
     int start_flashing(LightType_E enLightType, int nFlashTime, Alarm::FlashFrequency_E enFrequency);
     /*** 
      * @description : 停止闪烁
      * @author      : cyc
      * @param        {LightType_E} enLightType，灯光类型
      * @return       [int] - 0成功，-1失败
      */
     int stop_flashing(LightType_E enLightType);
     /*** 
      * @description : 获取闪烁状态
      * @author      : cyc
      * @param        {LightType_E} enLightType，灯光类型
      * @param        {bool} &bIsFlashing，是否正在闪烁
      * @param        {int} &nRemainTime，剩余闪烁时间（秒）
      * @return       [int] - 0成功，-1失败
      */
     int get_flashing_status(LightType_E enLightType, bool &bIsFlashing, int &nRemainTime);
     /*** 
      * @description : 停止所有闪烁
      * @author      : cyc
      * @return       [int] - 0成功，-1失败
      */
     int stop_all_flashing();

private:

    /**
    * @brief 控制灯
    */
    void control_light(const FillLight_S& stLightConfig);
    
    /**
    * @brief 检查当前是否在定时时间内
    */
    bool isInScheduledTime() const;
    
    /**
    * @brief 应用定时模式的白灯控制
    */
    void applyScheduledWhiteLight();
    
    /**
    * @brief 启动定时检查线程
    */
    void startScheduleThread();
    
    /**
    * @brief 停止定时检查线程
    */
    void stopScheduleThread();
    
    /**
    * @brief 定时检查线程函数
    */
    void scheduleThreadFunc();

    /*** 
    * @description : 闪烁控制线程函数
    * @author      : cyc
    */
    void flashControlThread();

    /*** 
    * @description : 获取闪烁间隔（毫秒）
    * @author      : cyc
    * @param        {FlashFrequency_E} enFrequency，闪烁频率
    * @return       [int] 间隔毫秒数
    */
    int getFlashInterval(Alarm::FlashFrequency_E enFrequency);

    /*** 
    * @description : 获取闪烁参数引用
    * @author      : cyc
    * @param        {LightType_E} enLightType，灯光类型
    * @return       [FlashParam_S&] 闪烁参数引用
    */
    FlashParam_S& getFlashParam(LightType_E enLightType);

    /*** 
    * @description : 执行单次闪烁控制
    * @author      : cyc
    * @param        {FlashParam_S&} flashParam，闪烁参数
    */
    void executeFlash(FlashParam_S& flashParam);

private:
    System::Peripheral_S m_peripheralConfig;
    mutable std::mutex m_configMutex;
    
    /* 当前日夜状态 */ 
    std::atomic<bool> m_isNight{false};
    std::mutex m_lightConfigMutex;
    
    /* 定时检查线程 */ 
    std::atomic<bool> m_scheduleRunning{false};
    std::thread m_scheduleThread;

    /* 白光灯 PWM 输出引脚 */
    const unsigned int white_light_output_pwm_pins = 0;
    /* 红外 PWM 输出引脚 */
    const unsigned int red_light_output_pwm_pins = 2;

    /* 闪烁控制相关 */
    std::atomic<bool> m_bRunning;               /* 线程运行标志 */
    std::thread m_flashThread;                  /* 闪烁控制线程 */
    std::mutex m_flashMutex;                    /* 闪烁参数互斥锁 */
    std::condition_variable m_flashCondition;   /* 条件变量 */
    
    /* 不同灯光类型的闪烁参数 */
    FlashParam_S m_whiteFlashParam;             /* 白光闪烁参数 */
    FlashParam_S m_redFlashParam;               /* 红光闪烁参数 */
};
 