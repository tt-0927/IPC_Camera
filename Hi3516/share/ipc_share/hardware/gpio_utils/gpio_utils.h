/*
 * @FilePath     : gpio_utils.h
 * @Author       : 李辉 lihui@kfb.cn
 * @Date         : 2025-02-21 10:40:11
 * @LastEditors: 李辉 lihui@kfb.cn
 * @LastEditTime: 2025-02-21 11:16:53
 * @Description  : 
 */
#ifndef _GPIO_UTILS_
#define _GPIO_UTILS_

typedef struct GPIO_NEEDPARAM_S
{
    unsigned int nGpio;         /* GPIO 编号 */
    unsigned int nIsOutput;     /* 是否为输出模式 */
    unsigned int nLowActive;    /* 是否为低电平有效 */
    unsigned int nValue;        /* GPIO 默认输出的引脚值 */
} GpioNeedParam_S;

typedef struct _GPIO_HANDLE_S GpioHandle_S;

struct _GPIO_HANDLE_S
{
/******************功能****************************************************/
    int (*set_direction) ( GpioHandle_S* pHandle, unsigned int nIsOutput );
    int (*get_direction) ( GpioHandle_S* pHandle, unsigned int *nIsOutput );
    int (*set_lowActive) ( GpioHandle_S* pHandle, unsigned int nLowActive );
    int (*set_value) ( unsigned int nGpio, unsigned int nValue );
    int (*get_value) ( unsigned int nGpio, unsigned int *nValue );

    int (*gpio_init) ( GpioHandle_S* pHandle );
    int (*gpio_uninit) ( GpioHandle_S* pHandle );

/******************属性****************************************************/
    /*必需参数*/
    GpioNeedParam_S stNeedParam;

};


/*分配一个GPIO句柄*/
GpioHandle_S* gpio_alloc(GpioNeedParam_S stNeedParam);

/*释放一个GPIO句柄*/
int gpio_release( GpioHandle_S* pHandle );





#endif