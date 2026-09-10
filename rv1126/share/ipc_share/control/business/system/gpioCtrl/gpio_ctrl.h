/**
 * @FilePath     : gpio_ctrl.h
 * @Author       : cyc
 * @Date         : 2025-04-18 09:07:07
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-07-17 13:40:47
 * @Description  : 硬件节点控制
 */

#pragma once

#include <stdio.h>
#include <iostream>
#include "Singleton.h"

/* 高电平 */
#define  GPIO_HIGHT (1)
/* 低电平 */
#define  GPIO_LOW   (0)

/* GPIO 数量 */
#if CAP_ALARM_IO
#define GPIO_OUTPUT_COUNT 1
#define GPIO_INPUT_COUNT  1
#else
#define GPIO_OUTPUT_COUNT 0
#define GPIO_INPUT_COUNT  0
#endif
//GPIO定义
#if CAP_GPIO_LAYOUT_3852_SERIES // 3852 系列 GPIO 布局
    #define GPIO_ALARM_IN_1         9
    #define GPIO_ALARM_OUT_1        10
#elif CAP_GPIO_LAYOUT_RV1126 // RV1126 系列 GPIO 布局
    #define GPIO_ALARM_IN_1         192
    #define GPIO_ALARM_IN_2         193
    #define GPIO_ALARM_OUT_1        194
    #define GPIO_ALARM_OUT_2        195
#else
    #define GPIO_ALARM_IN_1         8
    #define GPIO_ALARM_IN_2         10
    #define GPIO_ALARM_OUT_1        11
    #define GPIO_ALARM_OUT_2        9
#endif

class CGpioCtrl: public CSingleton<CGpioCtrl>
{
public:
    CGpioCtrl() = default;
    ~CGpioCtrl()  = default;
    friend class CSingleton<CGpioCtrl>;

    /*** 
     * @description : gpio模块初始化
     * @author      : cyc
     * @return       {*}成功返回0，失败不为0
     */  
    int init();

    /*** 
     * @description : gpio模块反初始化
     * @author      : cyc
     * @return       {*}成功返回0，失败不为0
     */  
    int deinit();

    /*** 
     * @description : 读取指定序号对应的gpio引脚值
     * @author      : liuhm
     * param        : gpio引脚对应的序号
     * @return       {0:读取到低电平，1:读取到高电平 -1:失败}
     */
    int alarm_input_read(int num);

    /**
    * @brief 获取指定序号的报警 GPIO 输出状态
    * @param order 报警序号（0 到 GPIO_OUTPUT_COUNT-1）
    * @return int 0: 输出关闭, 1: 输出开启, -1: 序号错误
    */
    int alarm_output_state(int order);

    /* 报警 GPIO 输出控制 */
    /**
    * @brief 打开指定序号的报警 GPIO 输出
    * @param order 报警序号（0 到 GPIO_OUTPUT_COUNT-1）
    */
    void alarm_output_on(int order);

    /**
    * @brief 关闭指定序号的报警 GPIO 输出
    * @param order 报警序号（0 到 GPIO_OUTPUT_COUNT-1）
    */
    void alarm_output_off(int order);

private:
#if CAP_ALARM_IO
    /* 报警 GPIO 输入引脚数组 */
    const unsigned int alarm_input_gpio_pins[GPIO_INPUT_COUNT] = {GPIO_ALARM_IN_1};

    /* 报警 GPIO 输出引脚数组 */
    const unsigned int alarm_output_gpio_pins[GPIO_OUTPUT_COUNT] = {GPIO_ALARM_OUT_1};
#endif
};

