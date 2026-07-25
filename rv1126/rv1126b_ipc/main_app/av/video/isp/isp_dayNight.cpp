/*** 
 * @FilePath     : isp_dayNight.cpp
 * @Author       : cyc
 * @Date         : 2025-08-27 19:02:28
 * @LastEditors  : cyc
 * @LastEditTime : 2026-01-29 17:10:45
 * @Description  : 日夜切换控制器实现
 */

 #include "isp_dayNight.h"
 #include "dlog.h"
 #include "IpcRet.h"
 #include "path_define.h"
 #include "isp_control.h"
 #include <ctime>
 
 
CDayNightController::CDayNightController()
{
    aiq_ctx = NULL;
    smartIr_ctx = NULL;
    memset(&m_stSmartAttr, 0, sizeof(rk_smart_ir_attr_t));
}
  
 
CDayNightController::~CDayNightController()
{
    stop();
}

void CDayNightController::smartIr_cb(rk_smart_ir_result_t stResult)
{
    CDayNightController::instance()->handleSmartIrCallback(stResult);
}

 void CDayNightController::handleSmartIrCallback(rk_smart_ir_result_t stResult)
 {
    /* 只有在自动模式下，才响应 smart_ir 的回调 */ 
    if (m_currentMode.load() != AUTO_MODE) 
    {
        return;
    }

    bool shouldBeNight = (stResult.status == RK_SMART_IR_STATUS_NIGHT);
    
    /* 仅当状态发生变化时才处理 */ 
    if (stResult.is_status_change && shouldBeNight != m_isNight.load()) 
    {
        dlog_info("SmartIR Callback: Switch to %s", shouldBeNight ? "Night" : "Day");
        performStateChange(shouldBeNight);
    }
 }
 
 
 bool CDayNightController::start()
 {
    if (m_running.load()) 
    {
        dlog_warn("DayNight controller already running");
        return true;
    }

    m_running.store(true);

    aiq_ctx = CIspControl::instance()->get_aiq_ctx();

    XCamReturn nRet;
 
    /* smartIr 初始化 */
    smartIr_ctx = rk_smart_ir_init(aiq_ctx);
    if (!smartIr_ctx)
    {
        dlog_error("rk_smart_ir_init error");
        m_running.store(false);
        return false;
    } 

    /* 获取当前 smartIr 的属性 */
    nRet = rk_smart_ir_getAttr(smartIr_ctx, &m_stSmartAttr);
    if (nRet != XCAM_RETURN_NO_ERROR) 
    {
        dlog_error("rk_smart_ir_getAttr error,%u",nRet);
        rk_smart_ir_deInit(smartIr_ctx);
        smartIr_ctx = NULL;
        m_running.store(false);
        return false;
    }

    /* 设置 smartIr 的属性 */
    /* 配置⽇夜状态 */
    m_stSmartAttr.init_status = RK_SMART_IR_STATUS_DAY;
    /* ⽇夜转化模式 */
    m_stSmartAttr.switch_mode = RK_SMART_IR_SWITCH_MODE_AUTO;
    /* 补光灯亮度调节模式，包括⾃动亮度补光模式和固定亮度补光模式。 */
    m_stSmartAttr.light_mode = RK_SMART_IR_LIGHT_MODE_MANUAL;
    /* 补光灯光源类型，包括可⻅光、红外光、⽆补光、混合光四种类型。 */
    m_stSmartAttr.light_type = RK_SMART_IR_LIGHT_TYPE_VIS;
    /* 补光灯亮度值 */
    m_stSmartAttr.light_value = 50;
    /* ⽇转夜亮度阈值 */
    m_stSmartAttr.params.d2n_envL_th = 0.01f;
    /* 夜转⽇亮度阈值 */
    m_stSmartAttr.params.n2d_envL_th = 0.20f;
    /* ⿊夜切⽩天的Rgain/Ggain基准值 */
    m_stSmartAttr.params.rggain_base = 1.00f;
    /* ⿊夜切⽩天的Bgain/Ggain基准值 */
    m_stSmartAttr.params.bggain_base = 1.00f;
    /* ⿊夜切⽩天的awbgain滤波半径 */
    m_stSmartAttr.params.awbgain_rad = 0.10f;
    /* ⿊夜切⽩天的awbgain离散度阈值 */
    m_stSmartAttr.params.awbgain_dis = 0.20f;
    /* 切换阈值， 保持相同状态次数⼤于该阈值时才允许状态切换。状态保持时⻓ (ms) = 1 / fps x 1000 x switch_cnts_th*/
    m_stSmartAttr.params.switch_cnts_th = 100;
    /* 是否加快⽇夜转化，对转化速度有要求的可以将此参数配置为true,切换速度提升了，相应的误切概率也会增加 */
    m_stSmartAttr.en_quick_switch = false;
    /* 是否启⽤带分块权重的⽇夜转化，该权重值与AE模块中的GridWeights参数是⼀致的。 */
    m_stSmartAttr.en_grid_weight = false;
    /* 补光灯类型配置为⾃动，⿊夜切⽩天时，是否⾃动调整的⿊切⽩阈值。true为 ⾃动调整，false为不调整。*/
    m_stSmartAttr.en_auto_n2dth = true;

    updateSmartIrAttr();


    /* 注册回调函数 */
    nRet = rk_smart_ir_runCb(smartIr_ctx, false, CDayNightController::smartIr_cb);
    if (nRet != XCAM_RETURN_NO_ERROR) 
    {
        dlog_error("rk_smart_ir_runCb failed!");
        rk_smart_ir_deInit(smartIr_ctx);
        smartIr_ctx = NULL;
        m_running.store(false);
        return false;
    }

    m_workerThread = std::make_unique<std::thread>(&CDayNightController::workerThread, this);
    if (m_workerThread) 
    {
        dlog_info("DayNight controller started successfully");
        return true;
    } 
    else 
    {
        dlog_error("Failed to start day night worker thread");
        m_running.store(false);
        rk_smart_ir_deInit(smartIr_ctx);
        smartIr_ctx = NULL;
        return false;
    }
 }
  
 
 void CDayNightController::stop()
 {
    if (!m_running.load()) 
    {
        return;
    }
    
    m_running.store(false);
    
    if (m_workerThread && m_workerThread->joinable()) 
    {
        m_workerThread->join();
    }
    m_workerThread.reset();

    /* smartIr 去初始化 */
    if (smartIr_ctx) 
    {
        rk_smart_ir_deInit(smartIr_ctx);
        smartIr_ctx = NULL;
    }

    aiq_ctx = NULL;
    
    dlog_info("DayNight controller stopped");
 }
  
 void CDayNightController::setMode(DayNightMode_E enDayNightMode)
 {
    dlog_info("Set DayNight mode to: %d", enDayNightMode);
    m_currentMode.store(enDayNightMode);
    
    if (!smartIr_ctx) 
    {
        dlog_warn("smartIr not initialized, mode will be applied on start.");
        return;
    }

    switch (enDayNightMode) 
    {
        case DAY_MODE:
            m_stSmartAttr.switch_mode = RK_SMART_IR_SWITCH_MODE_DAY;
            performStateChange(false);
            break;
        case NIGHT_MODE:
            m_stSmartAttr.switch_mode = RK_SMART_IR_SWITCH_MODE_NIGHT;
            performStateChange(true);
            break;
        case AUTO_MODE:
            m_stSmartAttr.switch_mode = RK_SMART_IR_SWITCH_MODE_AUTO;
            break;
        case TIME_MODE:
        {
            /* 立即检查当前时间应该处于什么状态 */ 
            bool shouldBeNight = shouldBeNightByTime();
            m_stSmartAttr.switch_mode = shouldBeNight ? RK_SMART_IR_SWITCH_MODE_NIGHT:RK_SMART_IR_SWITCH_MODE_DAY;
            dlog_info("定时模式：当前应该处于%s模式", shouldBeNight ? "夜晚" : "白天");
            performStateChange(shouldBeNight);
            break;
        }
        default:
            dlog_error("Unknown day night mode: %d", enDayNightMode);
            return;
    }
    
    updateSmartIrAttr();
 }
  
 
 void CDayNightController::setTimeRange(const TimeRange_S& timeRange)
 {
    m_timeRange = timeRange;
    dlog_info("Time range updated: %02d:%02d:%02d - %02d:%02d:%02d",
            timeRange.stStartTime.nHour, timeRange.stStartTime.nMinute, timeRange.stStartTime.nSecond,
            timeRange.stEndTime.nHour, timeRange.stEndTime.nMinute, timeRange.stEndTime.nSecond);
    
    /* 如果当前是定时模式，立即重新评估状态 */ 
    if (m_currentMode.load() == TIME_MODE) 
    {
        bool shouldBeNight = shouldBeNightByTime();
        if (shouldBeNight != m_isNight.load()) 
        {
            m_stSmartAttr.switch_mode = shouldBeNight ? RK_SMART_IR_SWITCH_MODE_NIGHT:RK_SMART_IR_SWITCH_MODE_DAY;
            updateSmartIrAttr();
            dlog_info("Time range updated, immediately adjusting state to: %s", shouldBeNight ? "Night" : "Day");
            performStateChange(shouldBeNight);
        }
    }
 }
  

 void CDayNightController::setSensitivity(unsigned int sensitivity)
 {
    if (sensitivity < 1)
    {
        sensitivity = 1;
    } 
    if (sensitivity > 10) 
    {
        sensitivity = 10;
    }
    m_sensitivity.store(sensitivity);
    m_stSmartAttr.params.switch_cnts_th = sensitivity * 30;
    dlog_info("sensitivity %u,switch_cnts_th: %u", sensitivity, m_stSmartAttr.params.switch_cnts_th);
    updateSmartIrAttr();
 }
  

 void CDayNightController::setFilterTime(unsigned int filterTimeSeconds)
 {
    if (filterTimeSeconds < FILTER_TIME_MIN) 
    {
        filterTimeSeconds = FILTER_TIME_MIN;
    } 
    else if (filterTimeSeconds > FILTER_TIME_MAX) 
    {
        filterTimeSeconds = FILTER_TIME_MAX;
    }
    
    /* 状态保持时⻓ (ms) = 1 / fps x 1000 x switch_cnts_th */
    m_stSmartAttr.params.switch_cnts_th = filterTimeSeconds * 30;
    dlog_info("Filter time updated to %u seconds (switch_cnts_th: %u)", 
            filterTimeSeconds, m_stSmartAttr.params.switch_cnts_th);

    updateSmartIrAttr();
 }
 
 /* 更新 rk_smart_ir 的属性 */ 
 void CDayNightController::updateSmartIrAttr()
 {
    if (smartIr_ctx && m_running.load()) 
    {
        XCamReturn ret = rk_smart_ir_setAttr(smartIr_ctx, &m_stSmartAttr);
        if (ret != XCAM_RETURN_NO_ERROR) 
        {
            dlog_error("rk_smart_ir_setAttr failed!");
        } 
        else 
        {
            dlog_debug("rk_smart_ir_setAttr successfully, switch_mode=%d", m_stSmartAttr.switch_mode);
        }
    }
 }

 
void CDayNightController::sample_smartIr_calib()
{
    rk_smart_ir_calib_t calib_cfg;
    calib_cfg.calib_en = true;

    /* calib d2n_envL_th
     * 1. Switch to ISP day mode
     * 2. Enable IR-cutter, disable IR-LED
     * 3. Adjust brightness threshold for day-to-night switch
     */
    calib_cfg.calib_mode = RK_SMART_IR_CALIB_MODE_D2N;
    calib_cfg.calib_params.d2n_envL_th = 0.04f; //need cfg, init calib params
    rk_smart_ir_calib(smartIr_ctx, &calib_cfg);

    /* calib n2d_envL_th
     * 1. Switch to ISP night mode
     * 2. Disable IR-cutter, enable IR-LED
     * 3. Adjust brightness threshold for night-to-day switch
     */
    calib_cfg.calib_mode = RK_SMART_IR_CALIB_MODE_N2D;
    calib_cfg.calib_params.n2d_envL_th = 0.2f; //need cfg, init calib params
    rk_smart_ir_calib(smartIr_ctx, &calib_cfg);

    /* calib awbgain base
     * 1. Switch to ISP night mode
     * 2. Disable IR-cutter, enable IR-LED
     * 3. Ensure no visible light
     */
    calib_cfg.calib_mode = RK_SMART_IR_CALIB_MODE_BASE;
    rk_smart_ir_calib(smartIr_ctx, &calib_cfg);

    /* tune awbgain dis
     * 1. Switch to ISP night mode
     * 2. Disable IR-cutter, enable IR-LED
     * 3. Test various scenarios and get the maximum value
     */
    calib_cfg.calib_mode = RK_SMART_IR_CALIB_MODE_DIS;
    calib_cfg.calib_params.rggain_base = 1.00f; //need cfg
    calib_cfg.calib_params.bggain_base = 1.00f; //need cfg
    calib_cfg.calib_params.awbgain_rad = 0.10f; //need cfg
    rk_smart_ir_calib(smartIr_ctx, &calib_cfg);
    dlog_debug("calib result: d2n_envL_th = %f,n2d_envL_th = %f,awbgain_base = %f, %f,awbgain_rad = %f", calib_cfg.calib_params.d2n_envL_th,calib_cfg.calib_params.n2d_envL_th,
        calib_cfg.calib_params.rggain_base,calib_cfg.calib_params.bggain_base,calib_cfg.calib_params.awbgain_rad);
}

  
 void CDayNightController::workerThread()
 {  
    while (m_running.load()) 
    {  
        /* test */
        //  sample_smartIr_calib();
        if (m_currentMode.load() == TIME_MODE) 
        {
            handleTimeMode();
        }
            
     std::this_thread::sleep_for(THREAD_SLEEP_INTERVAL);
    }
    
    dlog_info("DayNight worker thread exited");
 }
  

 void CDayNightController::handleTimeMode()
 {
    bool shouldBeNight = shouldBeNightByTime();
    
    if (shouldBeNight != m_isNight.load()) 
    {
        performStateChange(shouldBeNight);
    }
 }
  
 bool CDayNightController::shouldBeNightByTime() const
 {
    auto now = std::time(nullptr);
    auto* localTime = std::localtime(&now);
    
    int currentSeconds = localTime->tm_hour * 3600 + 
                        localTime->tm_min * 60 + 
                        localTime->tm_sec;
    
    int startSeconds = m_timeRange.stStartTime.nHour * 3600 + 
                    m_timeRange.stStartTime.nMinute * 60 + 
                    m_timeRange.stStartTime.nSecond;
    
    int endSeconds = m_timeRange.stEndTime.nHour * 3600 + 
                    m_timeRange.stEndTime.nMinute * 60 + 
                    m_timeRange.stEndTime.nSecond;
    
    /* 判断是否在白天时间范围内 */ 
    bool isDayTime;
    if (startSeconds < endSeconds) 
    {
        /* 正常情况：如 08:00 - 18:00 */ 
        isDayTime = (currentSeconds >= startSeconds) && (currentSeconds < endSeconds);
    } 
    else 
    {
        /* 跨天情况：如 18:00 - 08:00 (次日) */ 
        isDayTime = (currentSeconds >= startSeconds) || (currentSeconds < endSeconds);
    }
    
    return !isDayTime;
 }
  
 
 void CDayNightController::performStateChange(bool bToNight)
 {
    if (!aiq_ctx) 
    {
        dlog_error("Cannot perform state change, aiq_ctx is null");
        return;
    }

    dlog_info("Performing state change to %s", bToNight ? "Night" : "Day");

    #if 0  /* 这款不做夜间黑白,灯光控制用rk本身 */
    if (bToNight) 
    {
        /* 切换到夜晚 */ 
        rk_aiq_uapi2_sysctl_switch_scene(aiq_ctx, "normal", "night");
    } 
    else 
    {
        /* 切换到白天 */ 
        rk_aiq_uapi2_sysctl_switch_scene(aiq_ctx, "normal", "day");
    }
    #endif
    

    // 更新内部状态
    m_isNight.store(bToNight);
    
    if (m_stateChangeCallback) 
    {
        m_stateChangeCallback(bToNight, m_currentMode.load());
    }
    
 }

 void CDayNightController::setStateChangeCallback(StateChangeCallback callback)
 {
    m_stateChangeCallback = std::move(callback);
 }
 
 