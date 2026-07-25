/*** 
 * @FilePath     : pwm_ctrl.h
 * @Author       : cyc
 * @Date         : 2025-06-09 08:35:00
 * @LastEditors  : cyc
 * @LastEditTime : 2025-08-29 17:12:21
 * @Description  : pwm控制
 */

#pragma once

#include <stdio.h>
#include <iostream>
#include <mutex>
#include <thread>
#include "Singleton.h"
#include "isp_define.h"
#include "system_define.h"
#include <atomic>

using namespace ISP;
/* 灯光周期值 */
#define CHN_PERIOD_VALUE (20000)
/* 灯光占空比默认值 */
#define CHN_DUTYCYCLE_VALUE (10000)

class CPwmCtrl: public CSingleton<CPwmCtrl>
{
public:
    CPwmCtrl();
    ~CPwmCtrl();
    friend class CSingleton<CPwmCtrl>;

    /*** 
     * @description : 打开灯光
     * @author      : cyc
     * @param        {LightType_E} enLightType，灯光类型
     * @return       [int] - 0成功，-1失败
     */    
    int light_turn_on(LightType_E enLightType);

    /*** 
     * @description : 关闭
     * @author      : cyc
     * @param        {LightType_E} enLightType，灯光类型
     * @return       [int] - 0成功，-1失败
     */     
    int light_turn_off(LightType_E enLightType);

    /*** 
     * @description : 获取灯光状态
     * @author      : cyc
     * @param        {LightType_E} enLightType，灯光类型
     * @param        {bool} &pIsOn，是否启动
     * @return       [int] - 0成功，-1失败
     */    
    int get_light_status(LightType_E enLightType,bool &pIsOn);

    /*** 
     * @description : 控制灯光的强度
     * @author      : cyc
     * @param        {LightType_E} enLightType
     * @param        {unsigned int} nDutyCycle
     * @return       [int] - 0成功，-1失败
     */    
    int control_light_intensity(LightType_E enLightType, unsigned int nDutyCycle);

     /*** 
     * @description : 获取灯光的占空比
     * @author      : cyc
     * @param        {LightType_E} enLightType，灯光类型
     * @param        {unsigned int} &nDutyCycle，占空比
     * @return       [int] - 0成功，-1失败
     */    
    int get_light_duty_cycle(LightType_E enLightType, unsigned int &nDutyCycle);

private:
#if CAP_LIGHT_WHITE_ONLY // 仅白光灯能力
    /* 白光灯 灯光PWM编号 */
    const unsigned int white_light_output_pwm_num = 1;
    /* 红外 灯光PWM编号 */
    const unsigned int red_light_output_pwm_num = 2;
    /* 白光灯 PWM 输出引脚 */
    const unsigned int white_light_output_pwm_pins = 0;
    /* 红外 PWM 输出引脚 */
    const unsigned int red_light_output_pwm_pins = 0;

#else

    #if CAP_IO_EXTERNAL_DDR_00S

        /* 白光灯 灯光PWM编号 */
        const unsigned int white_light_output_pwm_num = 3;
        /* 红外 灯光PWM编号 */
        const unsigned int red_light_output_pwm_num = 3;
        /* 白光灯 PWM 输出引脚 */
        const unsigned int white_light_output_pwm_pins = 1;
        /* 红外 PWM 输出引脚 */
        const unsigned int red_light_output_pwm_pins = 3;
    #else


    /* 白光灯 灯光PWM编号 */
    const unsigned int white_light_output_pwm_num = 0;
    /* 红外 灯光PWM编号 */
    const unsigned int red_light_output_pwm_num = 0;
    /* 白光灯 PWM 输出引脚 */
    const unsigned int white_light_output_pwm_pins = 0;
    /* 红外 PWM 输出引脚 */
    const unsigned int red_light_output_pwm_pins = 2;

    #endif//CAP_IO_EXTERNAL_DDR_00S
#endif
       
};

