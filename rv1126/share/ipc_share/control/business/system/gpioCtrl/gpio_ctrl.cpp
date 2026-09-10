/**
 * @FilePath     : gpio_ctrl.cpp
 * @Author       : cyc
 * @Date         : 2025-04-18 09:07:07
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-07-17 13:40:47
 * @Description  : 硬件节点控制
 */

#include "gpio_ctrl.h"
#include <unistd.h>
#include <pthread.h>
#include <cerrno>
#include "dlog.h"
#include "IpcRet.h"

extern "C"
{
#include "gpio_utils.h"
}

int CGpioCtrl::init()
{
    int nRet = OK;

    return nRet;
}

int CGpioCtrl::deinit()
{
    int nRet = OK;
    return nRet;
}

int CGpioCtrl::alarm_output_state(int order)
{
    #if CAP_ALARM_IO
    if (order < 0 || order >= GPIO_OUTPUT_COUNT)
    {
        dlog_error("Invalid order: %u",order);
        return -1;
    }

    unsigned int gpio_pin = alarm_output_gpio_pins[order];
    GpioNeedParam_S stNeedParam = {gpio_pin, true, false, GPIO_LOW};
    GpioHandle_S* pHandle = gpio_alloc(stNeedParam);
    if (pHandle)
    {
        unsigned int value = 0;
        int ret = pHandle->get_value(gpio_pin, &value);
        gpio_release(pHandle);
        if (ret != OK)
        {
            dlog_error("Failed to get value for GPIO: %u",gpio_pin);
        }
        return value;
    }
    else
    {
        dlog_error("Failed to allocate GPIO handle for pin: %u",gpio_pin);
        return -1;
    }
    #endif
    return -1;
}

/* 报警 GPIO 控制 */
void CGpioCtrl::alarm_output_on(int order)
{
    #if CAP_ALARM_IO
    if (order < 0 || order >= GPIO_OUTPUT_COUNT)
    {
        dlog_error("Invalid order: %u",order);
        return;
    }

    unsigned int gpio_pin = alarm_output_gpio_pins[order];
    GpioNeedParam_S stNeedParam = {gpio_pin, true, false, GPIO_HIGHT};
    GpioHandle_S* pHandle = gpio_alloc(stNeedParam);
    if (pHandle)
    {
        pHandle->gpio_init(pHandle);
        pHandle->set_value(gpio_pin, GPIO_HIGHT); /* 拉高电平 */
        gpio_release(pHandle);
    }
    else
    {
        dlog_error("Failed to allocate GPIO handle for pin: %u",gpio_pin);
    }
    #endif
}

void CGpioCtrl::alarm_output_off(int order)
{
    #if CAP_ALARM_IO
    if (order < 0 || order >= GPIO_OUTPUT_COUNT)
    {
        dlog_error("Invalid order: %u",order);
        return;
    }

    unsigned int gpio_pin = alarm_output_gpio_pins[order];
    GpioNeedParam_S stNeedParam = {gpio_pin, true, false, GPIO_LOW};
    GpioHandle_S* pHandle = gpio_alloc(stNeedParam);
    if (pHandle)
    {
        pHandle->gpio_init(pHandle);
        pHandle->set_value(gpio_pin, GPIO_LOW); /* 拉低电平 */
        gpio_release(pHandle);
    }
    else
    {
        dlog_error("Failed to allocate GPIO handle for pin: %u",gpio_pin);
    }
    #endif
}

int CGpioCtrl::alarm_input_read(int num)
{
    #if CAP_ALARM_IO
    if (num < 0 || num >= GPIO_INPUT_COUNT)
    {
        dlog_error("无效的序号: %d", num);
        return IpcRet_E::ERR;
    }

    unsigned int gpio_pin       = alarm_input_gpio_pins[num];
    GpioNeedParam_S stNeedParam = {gpio_pin, false, false, GPIO_LOW};
    GpioHandle_S* pHandle       = gpio_alloc(stNeedParam);
    if (pHandle)
    {
        unsigned int value = 0;
        pHandle->gpio_init(pHandle);
        int ret = pHandle->get_value(gpio_pin, &value);
        gpio_release(pHandle);
        if (ret != 0)
        {
            dlog_error("读取gpio引脚: %d失败", gpio_pin);
            return IpcRet_E::ERR;
        }
        return value;
    }
    else
    {
        dlog_error("申请gpio引脚: %d句柄失败", gpio_pin);
        return IpcRet_E::ERR;
    }
    #endif
    return IpcRet_E::ERR;
}
