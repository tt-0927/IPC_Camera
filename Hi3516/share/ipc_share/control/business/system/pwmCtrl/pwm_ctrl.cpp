/**
 * @FilePath     : pwm_ctrl.cpp
 * @Author       : cyc
 * @Date         : 2025-06-09 08:35:00
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-04-27 16:17:01
 * @Description  : pwm控制
 */

#include "pwm_ctrl.h"
#include <unistd.h>
#include <pthread.h>
#include <cerrno>
#include "dlog.h"
#include "IpcRet.h"
#include "pwm_utils.h"

CPwmCtrl::CPwmCtrl()
{

}
CPwmCtrl::~CPwmCtrl()
{
    
}

static int light_output_on(const unsigned int nPwmNum,const unsigned int nPin)
{
    PwmNeedParam_S stLight = {nPwmNum, nPin, CHN_PERIOD_VALUE, CHN_DUTYCYCLE_VALUE, 0};
    PwmHandle_S* pHandle = pwm_alloc(stLight);
    if (pHandle)
    {
        int nRet = pHandle->pwm_init(pHandle);
        if (nRet != IpcRet_E::OK)
        {
            dlog_error("初始化pwm引脚失败: pwm:%u, chn:%u, ret:%d", nPwmNum, nPin, nRet);
            pwm_release(pHandle);
            return IpcRet_E::ERR;
        }

        nRet = pHandle->set_enable(pHandle, 1);
        if (nRet != IpcRet_E::OK)
        {
            dlog_error("打开pwm引脚失败: pwm:%u, chn:%u, ret:%d", nPwmNum, nPin, nRet);
            pwm_release(pHandle);
            return IpcRet_E::ERR;
        }

        pwm_release(pHandle);
        return IpcRet_E::OK;
    }
    dlog_error("申请pwm引脚: %d,chn:%u句柄失败", nPwmNum,nPin);
    return IpcRet_E::ERR;
}

static int light_output_off(const unsigned int nPwmNum,const unsigned int nPin)
{
    PwmNeedParam_S stLight = {nPwmNum, nPin, CHN_PERIOD_VALUE, CHN_DUTYCYCLE_VALUE, 0};
    PwmHandle_S* pHandle = pwm_alloc(stLight);
    if (pHandle)
    {
        int nRet = pHandle->pwm_init(pHandle);
        if (nRet != IpcRet_E::OK)
        {
            dlog_error("初始化pwm引脚失败: pwm:%u, chn:%u, ret:%d", nPwmNum, nPin, nRet);
        }

        nRet = pHandle->set_enable(pHandle, 0);
        if (nRet != IpcRet_E::OK)
        {
            dlog_error("关闭pwm引脚失败: pwm:%u, chn:%u, ret:%d", nPwmNum, nPin, nRet);
            pwm_release(pHandle);
            return IpcRet_E::ERR;
        }

        pwm_release(pHandle);
        return IpcRet_E::OK;
    }
    dlog_error("申请pwm引脚: %d,chn:%u句柄失败", nPwmNum,nPin);
    return IpcRet_E::ERR;
}

static int get_light_pin_status(const unsigned int nPwmNum,const unsigned int nPin, bool* pIsOn) 
{
    PwmNeedParam_S stLight = {nPwmNum, nPin, CHN_PERIOD_VALUE, CHN_DUTYCYCLE_VALUE, 0};
    PwmHandle_S* pHandle = pwm_alloc(stLight);
    if (pHandle) 
    {
        unsigned int nEnable;
        if (pHandle->get_enable(pHandle, &nEnable) == IpcRet_E::OK) 
        {
            *pIsOn = (nEnable == 1);
            pwm_release(pHandle);
            return IpcRet_E::OK;
        }
        pwm_release(pHandle);
    }
    dlog_error("申请pwm引脚: %d,chn:%u句柄失败", nPwmNum,nPin);
    return IpcRet_E::ERR;
}

int CPwmCtrl::light_turn_on(LightType_E enLightType) 
{
    switch (enLightType) 
    {
        case LIGHT_TYPE_WHITE:
            return light_output_on(white_light_output_pwm_num,white_light_output_pwm_pins);
        case LIGHT_TYPE_RED:
            return light_output_on(red_light_output_pwm_num,red_light_output_pwm_pins);
        case LIGHT_TYPE_BOTH:
            if (light_output_on(red_light_output_pwm_num,red_light_output_pwm_pins) != IpcRet_E::OK)
            {
                return IpcRet_E::ERR;
            }
            return light_output_on(white_light_output_pwm_num,white_light_output_pwm_pins);
        case LIGHT_TYPE_RED_ON_WHITE_OFF:
        {
            /* 互斥切灯时先关闭相反灯，最后开启目标灯，避免最后一次关灯动作影响目标灯输出。 */
            int nRet = light_output_off(white_light_output_pwm_num,white_light_output_pwm_pins);
            if (nRet != IpcRet_E::OK)
            {
                dlog_warn("切换红外灯前关闭白灯失败: %d", nRet);
            }
            usleep(100000);
            return light_output_on(red_light_output_pwm_num,red_light_output_pwm_pins);
        }
        case LIGHT_TYPE_WHITE_ON_RED_OFF:
        {
            /* 互斥切灯时先关闭相反灯，最后开启目标灯，避免最后一次关灯动作影响目标灯输出。 */
            int nRet = light_output_off(red_light_output_pwm_num,red_light_output_pwm_pins);
            if (nRet != IpcRet_E::OK)
            {
                dlog_warn("切换白灯前关闭红外灯失败: %d", nRet);
            }
            usleep(100000);
            return light_output_on(white_light_output_pwm_num,white_light_output_pwm_pins);
        }
        default:
            dlog_error("不支持的灯光类型");
            return IpcRet_E::ERR;
    }
}

int CPwmCtrl::light_turn_off(LightType_E enLightType) 
{
    switch (enLightType) 
    {
        case LIGHT_TYPE_WHITE:
            return light_output_off(white_light_output_pwm_num,white_light_output_pwm_pins);
        case LIGHT_TYPE_RED:
            return light_output_off(red_light_output_pwm_num,red_light_output_pwm_pins);
        case LIGHT_TYPE_BOTH:
            if (light_output_off(red_light_output_pwm_num,red_light_output_pwm_pins) != IpcRet_E::OK)
            {
                return IpcRet_E::ERR;
            }
            return light_output_off(white_light_output_pwm_num,white_light_output_pwm_pins);
        default:
            dlog_error("不支持的灯光类型");
            return IpcRet_E::ERR;
    }
}

 int CPwmCtrl::get_light_status(LightType_E enLightType,bool &pIsOn)
 {
    int nRet = IpcRet_E::OK;
    bool bRedOn, bWhiteOn;
    switch (enLightType) 
    {
        case LIGHT_TYPE_WHITE:
            return get_light_pin_status(white_light_output_pwm_num,white_light_output_pwm_pins, &pIsOn);
        case LIGHT_TYPE_RED:
            return get_light_pin_status(red_light_output_pwm_num,red_light_output_pwm_pins, &pIsOn);
        case LIGHT_TYPE_BOTH:
        {
            nRet = get_light_pin_status(red_light_output_pwm_num,red_light_output_pwm_pins, &bRedOn);
            if (nRet != IpcRet_E::OK) 
            {
                return nRet;
            }
            nRet = get_light_pin_status(white_light_output_pwm_num,white_light_output_pwm_pins, &bWhiteOn);
            if (nRet != IpcRet_E::OK) 
            {
                return nRet;
            }
            pIsOn = bRedOn && bWhiteOn;
            return IpcRet_E::OK;
        }
        case LIGHT_TYPE_RED_ON_WHITE_OFF: 
        {
            nRet = get_light_pin_status(red_light_output_pwm_num,red_light_output_pwm_pins, &bRedOn);
            if (nRet != IpcRet_E::OK) 
            {
                return nRet;
            }
            nRet = get_light_pin_status(white_light_output_pwm_num,white_light_output_pwm_pins, &bWhiteOn);
            if (nRet != IpcRet_E::OK) 
            {
                return nRet;
            }
            pIsOn = bRedOn && !bWhiteOn;
            return IpcRet_E::OK;
        }
        case LIGHT_TYPE_WHITE_ON_RED_OFF: 
        {
            nRet = get_light_pin_status(red_light_output_pwm_num,red_light_output_pwm_pins, &bRedOn);
            if (nRet != IpcRet_E::OK) 
            {
                return nRet;
            }
            nRet = get_light_pin_status(white_light_output_pwm_num,white_light_output_pwm_pins, &bWhiteOn);
            if (nRet != IpcRet_E::OK) 
            {
                return nRet;
            }
            pIsOn = bWhiteOn && !bRedOn;
            return IpcRet_E::OK;
        }
        default:
            dlog_error("不支持的灯光类型，enLightType：%u",enLightType);
            return IpcRet_E::ERR;
    }
 }

 int CPwmCtrl::control_light_intensity(LightType_E enLightType, unsigned int nDutyCycle) 
{
    unsigned int nPwmNum;
    unsigned int nPin;
    switch (enLightType) 
    {
        case LIGHT_TYPE_WHITE:
            nPwmNum = white_light_output_pwm_num;
            nPin = white_light_output_pwm_pins;
            break;
        case LIGHT_TYPE_RED:
            nPwmNum = red_light_output_pwm_num;
            nPin = red_light_output_pwm_pins;
            break;
        default:
            dlog_error("不支持的灯光类型");
            return IpcRet_E::ERR;
    }

    PwmNeedParam_S stLight = {nPwmNum, nPin, CHN_PERIOD_VALUE, nDutyCycle, 1};
    PwmHandle_S* pHandle = pwm_alloc(stLight);
    if (pHandle) 
    {
        int nRet = pHandle->pwm_init(pHandle);
        if (nRet != IpcRet_E::OK)
        {
            dlog_error("初始化pwm引脚失败: pwm:%u, chn:%u, ret:%d", nPwmNum, nPin, nRet);
            pwm_release(pHandle);
            return IpcRet_E::ERR;
        }

        nRet = pHandle->set_duty_cycle(pHandle, nDutyCycle);
        if (nRet != IpcRet_E::OK)
        {
            dlog_error("设置pwm占空比失败: pwm:%u, chn:%u, duty:%u, ret:%d", nPwmNum, nPin, nDutyCycle, nRet);
            pwm_release(pHandle);
            return IpcRet_E::ERR;
        }

        pwm_release(pHandle);
        return IpcRet_E::OK;
    }
    dlog_error("申请pwm引脚: %d,chn:%u句柄失败", nPwmNum, nPin);
    return IpcRet_E::ERR;
}

int CPwmCtrl::get_light_duty_cycle(LightType_E enLightType, unsigned int &nDutyCycle) 
{
    unsigned int nPwmNum;
    unsigned int nPin;
    switch (enLightType) 
    {
        case LIGHT_TYPE_WHITE:
            nPwmNum = white_light_output_pwm_num;
            nPin = white_light_output_pwm_pins;
            break;
        case LIGHT_TYPE_RED:
            nPwmNum = red_light_output_pwm_num;
            nPin = red_light_output_pwm_pins;
            break;
        default:
            dlog_error("不支持的灯光类型");
            return IpcRet_E::ERR;
    }

    PwmNeedParam_S stLight = {nPwmNum, nPin, CHN_PERIOD_VALUE, 0, 0};
    PwmHandle_S* pHandle = pwm_alloc(stLight);
    if (pHandle) 
    {
        if (pHandle->get_duty_cycle(pHandle, &nDutyCycle) == IpcRet_E::OK) 
        {
            pwm_release(pHandle);
            return IpcRet_E::OK;
        }
        pwm_release(pHandle);
    }
    dlog_error("申请pwm引脚: %d,chn:%u句柄失败", nPwmNum, nPin);
    return IpcRet_E::ERR;
}


