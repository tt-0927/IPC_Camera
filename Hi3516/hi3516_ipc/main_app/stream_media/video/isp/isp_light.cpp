/*** 
 * @FilePath     : isp_light.cpp
 * @Author       : cyc
 * @Date         : 2025-08-27 19:18:11
 * @LastEditors  : cyc
 * @LastEditTime : 2025-08-28 19:57:33
 * @Description  : 补光控制器
 */

 #include "isp_light.h"
 #include "dlog.h"
 #include "IpcRet.h"
 #include "ot_mpi_isp.h"
 #include "pwm_ctrl.h"
 
CLightingController::CLightingController()
{
    m_viPipe = 0;
}

void CLightingController::controlLighting(bool isNight, const FillLight_S& lightConfig)
{
    if (!isNight) 
    {
        /* 白天模式，关闭所有补光灯 */ 
        CPwmCtrl::instance()->light_turn_off(LIGHT_TYPE_BOTH);
        return;
    }
    
    /* 夜晚模式，根据配置控制补光 */ 
    switch (lightConfig.enLightType) 
    {
        case LIGHT_TYPE_SMART:
            handleSmartLighting();
            break;
        default:
            handleManualLighting(lightConfig);
            break;
    }
}

void CLightingController::handleSmartLighting()
{
    #if 0
    /* 获取当前曝光信息 */ 
    ot_isp_exposure_attr exp_attr;
    int nRet = ot_mpi_isp_get_expander_attr(m_viPipe, &exp_attr);
    if (nRet != IpcRet_E::OK) {
        dlog_error("Failed to get exposure attr for smart light control");
        return;
    }
    
    uint32_t currentISO = exp_attr.auto_attr.iso_cal_coef;
    
    if (currentISO < LIGHT_OFF_THRESHOLD) {
        // 环境光充足，关闭补光灯
        CPwmCtrl::instance()->light_turn_off(LIGHT_TYPE_BOTH);
        dlog_debug("智能模式：环境光充足，关闭补光灯");
    } else if (currentISO < IR_SWITCH_THRESHOLD) {
        // 中等光照，使用白光灯
        CPwmCtrl::instance()->light_turn_on(LIGHT_TYPE_WHITE);
        CPwmCtrl::instance()->light_turn_off(LIGHT_TYPE_RED);
        
        int lightLevel = calculateWhiteLightLevel(currentISO);
        int nLightIntensity = SET_BRIGHT_VALUE(lightLevel);
        CPwmCtrl::instance()->control_light_intensity(LIGHT_TYPE_WHITE, nLightIntensity);
        
        dlog_debug("智能模式：使用白光灯，亮度：%d%%", lightLevel);
    } else {
        // 极低光照，切换到红外灯
        CPwmCtrl::instance()->light_turn_off(LIGHT_TYPE_WHITE);
        CPwmCtrl::instance()->light_turn_on(LIGHT_TYPE_RED);
        
        int nLightIntensity = SET_BRIGHT_VALUE(80);
        CPwmCtrl::instance()->control_light_intensity(LIGHT_TYPE_RED, nLightIntensity);
        
        dlog_debug("智能模式：切换到红外灯，亮度：80%%");
    }
    #endif
}

void CLightingController::handleManualLighting(const FillLight_S& lightConfig)
{
    switch (lightConfig.enLightType) {
        case LIGHT_TYPE_WHITE:
            dlog_debug("夜晚模式：开启白光灯，亮度：%d%%", lightConfig.stWhiteAttr.nLightLevel);
            CPwmCtrl::instance()->light_turn_on(LIGHT_TYPE_WHITE_ON_RED_OFF);
            {
                int nLightIntensity = SET_BRIGHT_VALUE(lightConfig.stWhiteAttr.nLightLevel);
                CPwmCtrl::instance()->control_light_intensity(LIGHT_TYPE_WHITE, nLightIntensity);
            }
            break;
            
        case LIGHT_TYPE_RED:
            dlog_debug("夜晚模式：开启红外灯，亮度：%d%%", lightConfig.stRedAttr.nLightLevel);
            CPwmCtrl::instance()->light_turn_on(LIGHT_TYPE_RED_ON_WHITE_OFF);
            {
                int nLightIntensity = SET_BRIGHT_VALUE(lightConfig.stRedAttr.nLightLevel);
                CPwmCtrl::instance()->control_light_intensity(LIGHT_TYPE_RED, nLightIntensity);
            }
            break;
            
        case LIGHT_TYPE_BOTH:
            {
                CPwmCtrl::instance()->light_turn_on(LIGHT_TYPE_WHITE);
                int nWhiteIntensity = SET_BRIGHT_VALUE(lightConfig.stWhiteAttr.nLightLevel);
                CPwmCtrl::instance()->control_light_intensity(LIGHT_TYPE_WHITE, nWhiteIntensity);
                
                CPwmCtrl::instance()->light_turn_on(LIGHT_TYPE_RED);
                int nRedIntensity = SET_BRIGHT_VALUE(lightConfig.stRedAttr.nLightLevel);
                CPwmCtrl::instance()->control_light_intensity(LIGHT_TYPE_RED, nRedIntensity);
                
                dlog_debug("夜晚模式：开启双灯补光");
            }
            break;
            
        case LIGHT_TYPE_WHITE_ON_RED_OFF:
            {
                CPwmCtrl::instance()->light_turn_on(LIGHT_TYPE_WHITE);
                int nWhiteIntensity = SET_BRIGHT_VALUE(lightConfig.stWhiteAttr.nLightLevel);
                CPwmCtrl::instance()->control_light_intensity(LIGHT_TYPE_WHITE, nWhiteIntensity);
                
                CPwmCtrl::instance()->light_turn_off(LIGHT_TYPE_RED);
                dlog_debug("夜晚模式：白灯开，红灯关");
            }
            break;
            
        case LIGHT_TYPE_RED_ON_WHITE_OFF:
            {
                CPwmCtrl::instance()->light_turn_on(LIGHT_TYPE_RED);
                int nRedIntensity = SET_BRIGHT_VALUE(lightConfig.stRedAttr.nLightLevel);
                CPwmCtrl::instance()->control_light_intensity(LIGHT_TYPE_RED, nRedIntensity);
                
                CPwmCtrl::instance()->light_turn_off(LIGHT_TYPE_WHITE);
                dlog_debug("夜晚模式：红灯开，白灯关");
            }
            break;
            
        case LIGHT_TYPE_CLOSE:
        default:
            dlog_debug("夜晚模式：关闭所有补光灯");
            CPwmCtrl::instance()->light_turn_off(LIGHT_TYPE_BOTH);
            break;
    }
}

int CLightingController::calculateWhiteLightLevel(uint32_t isoValue) const
{
    if (isoValue <= LIGHT_OFF_THRESHOLD) return MIN_LIGHT_LEVEL;
    if (isoValue >= IR_SWITCH_THRESHOLD) return MAX_LIGHT_LEVEL;
    
    /* 线性插值计算亮度 */ 
    int lightLevel = MIN_LIGHT_LEVEL + 
        ((isoValue - LIGHT_OFF_THRESHOLD) * (MAX_LIGHT_LEVEL - MIN_LIGHT_LEVEL)) / 
        (IR_SWITCH_THRESHOLD - LIGHT_OFF_THRESHOLD);
    
    return lightLevel;
}
