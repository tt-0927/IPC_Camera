/*** 
 * @FilePath     : pwm_utils.h
 * @Author       : cyc
 * @Date         : 2025-06-06 16:16:10
 * @LastEditors  : cyc
 * @LastEditTime : 2025-11-27 15:42:45
 * @Description  : pwm控制通用类
 */

#ifndef _PWM_UTILS_
#define _PWM_UTILS_

typedef struct PWM_NEEDPARAM_S
{
    unsigned int nPwm;         /* pwm 编号 */
    unsigned int nPwmChn;       /* pwm 通道号 */
    unsigned int nPeriod;      /* 通道周期 */
    unsigned int nDutyCycle;   /* 通道占空比 */
    unsigned int nEnable;      /* Pwm，使能,1-开启，0-关闭 */
} PwmNeedParam_S;

typedef struct _PWM_HANDLE_S PwmHandle_S;

struct _PWM_HANDLE_S
{
/******************功能****************************************************/
    int (*set_period) ( PwmHandle_S* pHandle, unsigned int nPeriodVal );
    int (*get_period) ( PwmHandle_S* pHandle, unsigned int *pPeriodVal );
    int (*set_duty_cycle) ( PwmHandle_S* pHandle, unsigned int nDutyCycleVal );
    int (*get_duty_cycle) ( PwmHandle_S* pHandle, unsigned int *pDutyCycleVal);
    int (*set_enable) ( PwmHandle_S* pHandle, unsigned int nEnable );
    int (*get_enable) ( PwmHandle_S* pHandle, unsigned int *pEnable );
    int (*set_polarity) ( PwmHandle_S* pHandle, unsigned int pEnable );

    int (*pwm_init) ( PwmHandle_S* pHandle );
    int (*pwm_uninit) ( PwmHandle_S* pHandle );

/******************属性****************************************************/
    /*必需参数*/
    PwmNeedParam_S stNeedParam;

};


/*分配一个pwm句柄*/
PwmHandle_S* pwm_alloc(PwmNeedParam_S stNeedParam);

/*释放一个pwm句柄*/
int pwm_release( PwmHandle_S* pHandle );





#endif