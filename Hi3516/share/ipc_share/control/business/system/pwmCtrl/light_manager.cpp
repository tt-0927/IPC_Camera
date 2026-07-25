/*** 
 * @FilePath     : light_manager.cpp
 * @Author       : cyc
 * @Date         : 2025-08-29 14:31:36
 * @LastEditors  : cyc
 * @LastEditTime : 2025-12-23 09:55:22
 * @Description  : 灯光管理模块
 */

 #include "light_manager.h"
 #include "pwm_ctrl.h"
 #include "dlog.h"
 #include "IpcRet.h"
 #include "path_define.h"
 #include "convert_interface.h"
 #include "isp_configure.h"
 #include "isp_dayNight.h"
 #include <chrono>
 
CLightManager::CLightManager() : m_bRunning(false)
{

}

CLightManager::~CLightManager()
{

}

int CLightManager::init()
{
    /* 外设信息配置 */ 
    ::System::Peripheral_S stInfo;
    Convert::read_file(PERIPHERAL_CONFIG_FILE, stInfo);
    m_peripheralConfig = stInfo;
    
    /* 启动闪烁控制线程 */ 
    m_bRunning.store(true);
    m_flashThread = std::thread(&CLightManager::flashControlThread, this);

    return OK;
}

int CLightManager::deinit()
{
    stopScheduleThread();

    /* 停止所有闪烁 */ 
    stop_all_flashing();
    
    /* 停止线程 */ 
    m_bRunning.store(false);
    m_flashCondition.notify_all();
    
    if (m_flashThread.joinable())
    {
        m_flashThread.join();
    }

    return OK;
}

int CLightManager::set_peripheral_config(const System::Peripheral_S& config)
{
    dlog_info("设置外设配置 - 启用: %s, 模式: %d, 亮度: %u", 
            config.bEnable ? "是" : "否", 
            config.nLightMode, 
            config.nLevel);
    
    bool needRestartSchedule = false;
    bool needImmediateApply = false;
    
    {
        std::lock_guard<std::mutex> lock(m_configMutex);
        
        /* 检查是否需要重启定时线程 */ 
        if (m_peripheralConfig.nLightMode != config.nLightMode) 
        {
            needRestartSchedule = true;
        }
        
        /* 检查是否需要立即应用 */ 
        if (m_peripheralConfig.bEnable != config.bEnable ||
            m_peripheralConfig.nLightMode != config.nLightMode ||
            m_peripheralConfig.nLevel != config.nLevel   ) 
        {
            needImmediateApply = true;
        }
        
        m_peripheralConfig = config;
    }
    
    /* 重启定时线程 */ 
    if (needRestartSchedule) 
    {
        stopScheduleThread();
        if (config.bEnable && config.nLightMode == 0) 
        {
            startScheduleThread();
        }
    }
    
    /* 立即应用新配置 */ 
    if (needImmediateApply) 
    {
        if (!config.bEnable) 
        {
            /* 外设禁用，关闭白灯 */ 
            CPwmCtrl::instance()->light_turn_off(LIGHT_TYPE_WHITE);
        } 
        else if (config.nLightMode == 0) 
        {
            /* 定时模式，立即检查并应用 */ 
            applyScheduledWhiteLight();
        } 
        else if (config.nLightMode == 1) 
        {
            ISP::DayNightAttr_S stDayNightAttr;
            CIspConfigure::instance()->get_configure(stDayNightAttr);
            control_light(stDayNightAttr.stFillLight);
        }

    }
    
    return IpcRet_E::OK;
}

int CLightManager::get_peripheral_config(System::Peripheral_S& config)
{
    std::lock_guard<std::mutex> lock(m_configMutex);
    config = m_peripheralConfig;
    return IpcRet_E::OK;
}

int CLightManager::apply_peripheral_config()
{
    System::Peripheral_S config;
    {
        std::lock_guard<std::mutex> lock(m_configMutex);
        /* lock: 只在锁内复制配置快照，后续恢复动作可能再次进入灯光配置锁 */
        config = m_peripheralConfig;
    }
    
    dlog_info("应用外设配置 - 启用: %s, 模式: %d, 亮度: %u", 
            config.bEnable ? "是" : "否", 
            config.nLightMode, 
            config.nLevel);
    
    /* 根据配置类型立即应用；定时模式内部会读取配置锁，必须在无锁状态调用 */
    if (!config.bEnable) 
    {
        CPwmCtrl::instance()->light_turn_off(LIGHT_TYPE_WHITE);
    }
    else if (config.nLightMode == 0) 
    {
        applyScheduledWhiteLight();
    } 
    else if (config.nLightMode == 1) 
    {
        ISP::DayNightAttr_S stDayNightAttr;
        CIspConfigure::instance()->get_configure(stDayNightAttr);
        control_light(stDayNightAttr.stFillLight);
    }
    
    return IpcRet_E::OK;
}

void CLightManager::on_dayNight_changed(bool isNight, const FillLight_S& lightConfig)
{
    dlog_info("接收日夜切换信号: %s", isNight ? "夜间" : "白天");
    
    m_isNight.store(isNight);
    
    if (m_peripheralConfig.bEnable && m_peripheralConfig.nLightMode == 1) 
    {
        dlog_info("自动模式：响应日夜切换控制白灯");
        control_light(lightConfig);
    } 
    else if (m_peripheralConfig.nLightMode == 0) 
    {
        dlog_info("定时模式：忽略日夜切换信号");
    } 
    else 
    {
        CPwmCtrl::instance()->light_turn_off(LIGHT_TYPE_WHITE);
        dlog_info("外设未启用或非自动模式：忽略白灯日夜切换");
    }
    
}

void CLightManager::control_light( const FillLight_S& stLightConfig)
{ 
    if(m_isNight.load())
    {
        if (stLightConfig.enLightType == LIGHT_TYPE_WHITE) 
        {
            dlog_info("夜间模式：开启白灯，亮度 %d%%", stLightConfig.stWhiteAttr.nLightLevel);
            
            unsigned int nDutyCycle = SET_BRIGHT_VALUE(stLightConfig.stWhiteAttr.nLightLevel);
            
            int ret = CPwmCtrl::instance()->light_turn_on(LIGHT_TYPE_WHITE_ON_RED_OFF);
            if (ret == IpcRet_E::OK) 
            {
                CPwmCtrl::instance()->control_light_intensity(LIGHT_TYPE_WHITE, nDutyCycle);
            }
        } 
        else if(stLightConfig.enLightType == LIGHT_TYPE_RED || stLightConfig.enLightType == LIGHT_TYPE_SMART)
        {
            dlog_info("夜间模式：开启红外灯，亮度 %d%%", stLightConfig.stRedAttr.nLightLevel);
            
            unsigned int nDutyCycle = SET_IR_BRIGHT_VALUE(stLightConfig.stRedAttr.nLightLevel);
            
            int ret = CPwmCtrl::instance()->light_turn_on(LIGHT_TYPE_RED_ON_WHITE_OFF);
            if (ret == IpcRet_E::OK) 
            {
                CPwmCtrl::instance()->control_light_intensity(LIGHT_TYPE_RED, nDutyCycle);
            }
        }
        else 
        {
#if CAP_LIGHT_WHITE_ONLY // 仅白光灯能力
            int ret = CPwmCtrl::instance()->light_turn_off(LIGHT_TYPE_WHITE);
            if (ret != OK) 
            {
                return ;     
            }
#else
            int ret = CPwmCtrl::instance()->light_turn_off(LIGHT_TYPE_BOTH);
            if (ret != OK) 
            {
                return ;     
            }
#endif
        }
    } 
    else 
    {
#if CAP_LIGHT_WHITE_ONLY // 仅白光灯能力
        int ret = CPwmCtrl::instance()->light_turn_off(LIGHT_TYPE_WHITE);
        if (ret != OK) 
        {
            return ;     
        }
#else
        int ret = CPwmCtrl::instance()->light_turn_off(LIGHT_TYPE_BOTH);
        if (ret != OK) 
        {
            return ;     
        }
#endif
    }
   
}

bool CLightManager::isInScheduledTime() const
{
    auto now = std::chrono::system_clock::now();
    std::time_t currentTime = std::chrono::system_clock::to_time_t(now);
    std::tm *localTime = std::localtime(&currentTime);
    
    int currentSeconds = localTime->tm_hour * 3600 + 
                        localTime->tm_min * 60 + 
                        localTime->tm_sec;
    
    int startSeconds = m_peripheralConfig.stLightTime.stStart.nHour * 3600 + 
                    m_peripheralConfig.stLightTime.stStart.nMinute * 60 + 
                    m_peripheralConfig.stLightTime.stStart.nSecond;
    
    int stopSeconds = m_peripheralConfig.stLightTime.stStop.nHour * 3600 + 
                    m_peripheralConfig.stLightTime.stStop.nMinute * 60 + 
                    m_peripheralConfig.stLightTime.stStop.nSecond;
    
    bool inRange = false;
    if (startSeconds <= stopSeconds) 
    {
        inRange = (currentSeconds >= startSeconds && currentSeconds < stopSeconds);
    } 
    else 
    {
        inRange = (currentSeconds >= startSeconds || currentSeconds < stopSeconds);
    }
    
    return inRange;
}

void CLightManager::applyScheduledWhiteLight()
{
    
    bool shouldTurnOn = false;
    unsigned int dutyCycle = 0;
    bool configValid = false;
    
    {
        std::lock_guard<std::mutex> lock(m_configMutex);
        
        
        if (!m_peripheralConfig.bEnable || m_peripheralConfig.nLightMode != 0) 
        {
            return;
        }
        
        shouldTurnOn = isInScheduledTime();
        dutyCycle = (CHN_PERIOD_VALUE * m_peripheralConfig.nLevel) / 100;
        configValid = true;
    }

    if (configValid) 
    {
        if (shouldTurnOn) 
        {
            dlog_info("定时模式：在时间范围内，开启白灯");
            int ret = CPwmCtrl::instance()->light_turn_on(LIGHT_TYPE_WHITE);
            if (ret == IpcRet_E::OK) 
            {
                CPwmCtrl::instance()->control_light_intensity(LIGHT_TYPE_WHITE, dutyCycle);
            }
        } 
        else 
        {
            dlog_info("定时模式：不在时间范围内，关闭白灯");
            CPwmCtrl::instance()->light_turn_off(LIGHT_TYPE_WHITE);
        }
    }
}

void CLightManager::startScheduleThread()
{
    if (m_scheduleRunning.load()) 
    {
        return;
    }
    
    m_scheduleRunning.store(true);
    m_scheduleThread = std::thread(&CLightManager::scheduleThreadFunc, this);
    dlog_info("定时检查线程启动");
}

void CLightManager::stopScheduleThread()
{
    if (!m_scheduleRunning.load()) 
    {
        return;
    }
    
    dlog_info("正在停止定时检查线程...");
    m_scheduleRunning.store(false);
    
    if (m_scheduleThread.joinable()) 
    {
        m_scheduleThread.join();
    }
    dlog_info("定时检查线程停止完成");
}

void CLightManager::scheduleThreadFunc()
{
    dlog_info("定时检查线程启动");
    
    while (m_scheduleRunning.load()) 
    {
        applyScheduledWhiteLight();
        
        /* 使用可中断的睡眠，每秒检查一次退出条件 */ 
        for (int i = 0; i < 30 && m_scheduleRunning.load(); ++i) 
        {
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
    }
    
    dlog_info("定时检查线程退出");
}

int CLightManager::manual_light_control(LightType_E lightType, bool enable, unsigned int brightness)
{
    /* 手动控制（用于闪烁等功能） */ 
    if (enable) 
    {
        unsigned int nDutyCycle = (CHN_PERIOD_VALUE * brightness) / 100;
        int ret = CPwmCtrl::instance()->light_turn_on(lightType);
        if (ret == IpcRet_E::OK) 
        {
            return CPwmCtrl::instance()->control_light_intensity(lightType, nDutyCycle);
        }
        return ret;
    } 
    else 
    {
        return CPwmCtrl::instance()->light_turn_off(lightType);
    }
}

int CLightManager::start_flashing(LightType_E enLightType, int nFlashTime, Alarm::FlashFrequency_E enFrequency)
{
    /* 参数检查 */ 
    if (nFlashTime < 1 || nFlashTime > 300)
    {
        dlog_error("闪烁时间参数错误，nFlashTime：%d，范围应为[1-300]", nFlashTime);
        return IpcRet_E::ERR;
    }
    if (enLightType != LIGHT_TYPE_WHITE && enLightType != LIGHT_TYPE_RED)
    {
        dlog_error("闪烁功能暂只支持单色灯光，enLightType：%d", static_cast<int>(enLightType));
        return IpcRet_E::ERR;
    }
    std::lock_guard<std::mutex> lock(m_flashMutex);
    
    FlashParam_S& flashParam = getFlashParam(enLightType);
    
    /* 设置参数 */ 
    flashParam.nFlashTime = nFlashTime;
    flashParam.enFrequency = enFrequency;
    flashParam.bIsFlashing = true;
    flashParam.startTime = std::chrono::steady_clock::now();
    flashParam.lastToggleTime = std::chrono::steady_clock::time_point(); /* 重置切换时间 */
    
    /* 根据频率设置初始状态 */
    if (enFrequency == Alarm::FlashFrequency_E::FLASH_STEADY_ON)
    {
        /* 常亮模式：直接点亮，但仍然需要定时关闭 */
        flashParam.bCurrentState = true;
        CPwmCtrl::instance()->light_turn_on(enLightType);
        dlog_info("开始定时常亮，灯光类型：%d，时间：%d秒", 
                  static_cast<int>(enLightType), nFlashTime);
    }
    else
    {
        /* 闪烁模式：从关闭状态开始 */
        flashParam.bCurrentState = false;
        dlog_info("开始闪烁，灯光类型：%d，时间：%d秒，频率：%d", 
                  static_cast<int>(enLightType), nFlashTime, static_cast<int>(enFrequency));
    }
    
    /* 通知闪烁线程 */ 
    m_flashCondition.notify_one();
    
    return IpcRet_E::OK;
}


int CLightManager::stop_flashing(LightType_E enLightType)
{
    if (enLightType != LIGHT_TYPE_WHITE && enLightType != LIGHT_TYPE_RED)
    {
        dlog_error("闪烁功能暂只支持单色灯光，enLightType：%d", static_cast<int>(enLightType));
        return IpcRet_E::ERR;
    }
    std::lock_guard<std::mutex> lock(m_flashMutex);
    
    FlashParam_S& flashParam = getFlashParam(enLightType);
    flashParam.bIsFlashing = false;
    
    /* 关闭灯光 */ 
    CPwmCtrl::instance()->light_turn_off(enLightType);
    
    dlog_info("停止闪烁，灯光类型：%d", static_cast<int>(enLightType));
    
    return IpcRet_E::OK;
}
int CLightManager::get_flashing_status(LightType_E enLightType, bool &bIsFlashing, int &nRemainTime)
{
    if (enLightType != LIGHT_TYPE_WHITE && enLightType != LIGHT_TYPE_RED)
    {
        dlog_error("闪烁功能暂只支持单色灯光，enLightType：%d", static_cast<int>(enLightType));
        return IpcRet_E::ERR;
    }
    std::lock_guard<std::mutex> lock(m_flashMutex);
    
    FlashParam_S& flashParam = getFlashParam(enLightType);
    bIsFlashing = flashParam.bIsFlashing;
    
    if (bIsFlashing && flashParam.nFlashTime > 0)
    {
        auto now = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - flashParam.startTime).count();
        nRemainTime = std::max(0, flashParam.nFlashTime - static_cast<int>(elapsed));
    }
    else
    {
        nRemainTime = 0;
    }
    
    return IpcRet_E::OK;
}
int CLightManager::stop_all_flashing()
{
    std::lock_guard<std::mutex> lock(m_flashMutex);
    
    m_whiteFlashParam.bIsFlashing = false;
    m_redFlashParam.bIsFlashing = false;
    
    /* 关闭所有灯光 */ 
    CPwmCtrl::instance()->light_turn_off(LIGHT_TYPE_BOTH);
    
    dlog_info("停止所有闪烁");
    
    return IpcRet_E::OK;
}
FlashParam_S& CLightManager::getFlashParam(LightType_E enLightType)
{
    switch (enLightType)
    {
        case LIGHT_TYPE_WHITE:
            return m_whiteFlashParam;
        case LIGHT_TYPE_RED:
            return m_redFlashParam;
        default:
            return m_whiteFlashParam; /* 默认返回白光参数 */
    }
}
int CLightManager::getFlashInterval(Alarm::FlashFrequency_E enFrequency)
{
    switch (enFrequency)
    {
        case Alarm::FlashFrequency_E::FLASH_STEADY_ON:
            return 0;    /* 常亮，不闪烁 */ 
        case Alarm::FlashFrequency_E::FLASH_LOW_FREQ:
            return 1000; /* 低频：1秒间隔（0.5Hz） */ 
        case Alarm::FlashFrequency_E::FLASH_MID_FREQ:
            return 500;  /* 中频：0.5秒间隔（1Hz） */ 
        case Alarm::FlashFrequency_E::FLASH_HIGH_FREQ:
            return 200;  /* 高频：0.2秒间隔（2.5Hz） */ 
        default:
            return 500;  /* 默认中频 */ 
    }
}
void CLightManager::executeFlash(FlashParam_S& flashParam)
{
    auto now = std::chrono::steady_clock::now();
    int interval = getFlashInterval(flashParam.enFrequency);
    
    /* 检查是否到了切换时间 */
    if (flashParam.lastToggleTime.time_since_epoch().count() == 0 || 
        std::chrono::duration_cast<std::chrono::milliseconds>(now - flashParam.lastToggleTime).count() >= interval)
    {
        /* 切换灯光状态 */
        if (flashParam.bCurrentState)
        {
            CPwmCtrl::instance()->light_turn_off(flashParam.enLightType);
            flashParam.bCurrentState = false;
        }
        else
        {
            CPwmCtrl::instance()->light_turn_on(flashParam.enLightType);
            flashParam.bCurrentState = true;
        }
        
        flashParam.lastToggleTime = now;
    }
}

void CLightManager::flashControlThread()
{
    pthread_setname_np(pthread_self(), "FlashControlThr");

    dlog_info("闪烁控制线程启动");
    
    while (m_bRunning.load())
    {
        std::unique_lock<std::mutex> lock(m_flashMutex);
        
        /* 检查是否有闪烁任务 */ 
        bool hasFlashTask = m_whiteFlashParam.bIsFlashing || m_redFlashParam.bIsFlashing;
        
        if (!hasFlashTask)
        {
            /* 没有闪烁任务，等待唤醒 */ 
            m_flashCondition.wait(lock, [this]() {
                return !m_bRunning.load() || m_whiteFlashParam.bIsFlashing || m_redFlashParam.bIsFlashing;
            });
            continue;
        }
        
        auto now = std::chrono::steady_clock::now();
        
        /* 处理白光闪烁 */ 
        if (m_whiteFlashParam.bIsFlashing)
        {
            /* 检查是否超时 */ 
            auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - m_whiteFlashParam.startTime).count();
            if (elapsed >= m_whiteFlashParam.nFlashTime)
            {
                m_whiteFlashParam.bIsFlashing = false;
                CPwmCtrl::instance()->light_turn_off(LIGHT_TYPE_WHITE);
                dlog_info("白光定时结束，停止亮灯 - 持续了%ld秒", elapsed);
                continue;
            }
            
            /* 如果不是常亮模式，执行闪烁 */ 
            if (m_whiteFlashParam.enFrequency != Alarm::FlashFrequency_E::FLASH_STEADY_ON)
            {
                executeFlash(m_whiteFlashParam);
            }
            /* 常亮模式下不需要执行闪烁，只等待时间到 */
        }
        
        /* 处理红光闪烁 */ 
        if (m_redFlashParam.bIsFlashing)
        {
            /* 检查是否超时 */ 
            auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - m_redFlashParam.startTime).count();
            if (elapsed >= m_redFlashParam.nFlashTime)
            {
                m_redFlashParam.bIsFlashing = false;
                CPwmCtrl::instance()->light_turn_off(LIGHT_TYPE_RED);
                dlog_info("红光定时结束，停止亮灯 - 持续了%ld秒", elapsed);
                continue;
            }
            
            /* 如果不是常亮模式，执行闪烁 */ 
            if (m_redFlashParam.enFrequency != Alarm::FlashFrequency_E::FLASH_STEADY_ON)
            {
                executeFlash(m_redFlashParam);
            }
            /* 常亮模式下不需要执行闪烁，只等待时间到 */
        }
        
        lock.unlock();
        
        /* 根据最小的闪烁间隔来控制线程睡眠时间 */ 
        int minInterval = 1000; /* 默认1秒 */ 
        if (m_whiteFlashParam.bIsFlashing)
        {
            if (m_whiteFlashParam.enFrequency == Alarm::FlashFrequency_E::FLASH_STEADY_ON)
            {
                /* 常亮模式下，每秒检查一次是否到时 */
                minInterval = std::min(minInterval, 1000);
            }
            else
            {
                int whiteInterval = getFlashInterval(m_whiteFlashParam.enFrequency);
                if (whiteInterval > 0)
                {
                    minInterval = std::min(minInterval, whiteInterval);
                }
            }
        }
        if (m_redFlashParam.bIsFlashing)
        {
            if (m_redFlashParam.enFrequency == Alarm::FlashFrequency_E::FLASH_STEADY_ON)
            {
                /* 常亮模式下，每秒检查一次是否到时 */
                minInterval = std::min(minInterval, 1000);
            }
            else
            {
                int redInterval = getFlashInterval(m_redFlashParam.enFrequency);
                if (redInterval > 0)
                {
                    minInterval = std::min(minInterval, redInterval);
                }
            }
        }
        
        /* 睡眠等待 */ 
        std::this_thread::sleep_for(std::chrono::milliseconds(minInterval));
    }
    
    dlog_info("闪烁控制线程退出");
}

 
