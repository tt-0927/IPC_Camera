/**
 * @FilePath     : gpio_ctrl.cpp
 * @Author       : cyc
 * @Date         : 2025-04-18 09:07:07
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-04-27 16:01:28
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

namespace
{
/**
 * @brief   : 设置指定 GPIO 电平并输出失败上下文
 * @param    {GpioHandle_S*} pHandle：GPIO 句柄
 * @param    {unsigned int} nGpio：GPIO 编号
 * @param    {unsigned int} nValue：目标电平
 * @param    {const char*} pStep：当前动作描述
 * @return   {int} 0：成功，非0：失败
 */
int set_ircut_gpio_value(GpioHandle_S* pHandle, unsigned int nGpio, unsigned int nValue, const char* pStep)
{
    int nRet = pHandle->set_value(nGpio, nValue);
    if (nRet != OK)
    {
        dlog_error("IR-CUT GPIO设置失败, step:%s, gpio:%u, value:%u, ret:%d", pStep, nGpio, nValue, nRet);
    }

    return nRet;
}

/**
 * @brief   : 释放 IR-CUT GPIO 句柄
 * @param    {GpioHandle_S*} pHandle：GPIO 句柄
 * @param    {bool} bInitialized：是否已经完成 gpio_init
 * @return   {void}
 */
void release_ircut_gpio_handle(GpioHandle_S* pHandle, bool bInitialized)
{
    if (pHandle == nullptr)
    {
        return;
    }

    if (bInitialized)
    {
        pHandle->gpio_uninit(pHandle);
    }
    gpio_release(pHandle);
}

/**
 * @brief   : 按指定脉冲电平执行一次 IR-CUT GPIO 切换
 * @param    {unsigned int} nGpioPin1：IR-CUT 第一路 GPIO
 * @param    {unsigned int} nGpioPin2：IR-CUT 第二路 GPIO
 * @param    {unsigned int} nPin1PulseValue：第一路动作脉冲电平
 * @param    {unsigned int} nPin2PulseValue：第二路动作脉冲电平
 * @param    {const char*} pModeName：切换模式名称，用于日志定位
 * @return   {int} 0：成功，非0：失败
 */
int switch_ircut_gpio(unsigned int nGpioPin1,
                      unsigned int nGpioPin2,
                      unsigned int nPin1PulseValue,
                      unsigned int nPin2PulseValue,
                      const char *pModeName)
{
    GpioNeedParam_S stIrcut1 = {nGpioPin1, true, false, GPIO_LOW};
    GpioNeedParam_S stIrcut2 = {nGpioPin2, true, false, GPIO_LOW};
    GpioHandle_S* pHandle1 = gpio_alloc(stIrcut1);
    GpioHandle_S* pHandle2 = gpio_alloc(stIrcut2);
    bool bHandle1Initialized = false;
    bool bHandle2Initialized = false;

    if (pHandle1 == nullptr || pHandle2 == nullptr)
    {
        dlog_error("IR-CUT %s模式GPIO句柄申请失败, gpio:%u %u", pModeName, nGpioPin1, nGpioPin2);
        release_ircut_gpio_handle(pHandle1, bHandle1Initialized);
        release_ircut_gpio_handle(pHandle2, bHandle2Initialized);
        return IpcRet_E::ERR;
    }

    int nRet = pHandle1->gpio_init(pHandle1);
    if (nRet != OK)
    {
        dlog_error("IR-CUT %s模式GPIO初始化失败, gpio:%u, ret:%d", pModeName, nGpioPin1, nRet);
        release_ircut_gpio_handle(pHandle1, bHandle1Initialized);
        release_ircut_gpio_handle(pHandle2, bHandle2Initialized);
        return IpcRet_E::ERR;
    }
    bHandle1Initialized = true;

    nRet = pHandle2->gpio_init(pHandle2);
    if (nRet != OK)
    {
        dlog_error("IR-CUT %s模式GPIO初始化失败, gpio:%u, ret:%d", pModeName, nGpioPin2, nRet);
        release_ircut_gpio_handle(pHandle1, bHandle1Initialized);
        release_ircut_gpio_handle(pHandle2, bHandle2Initialized);
        return IpcRet_E::ERR;
    }
    bHandle2Initialized = true;

    nRet = set_ircut_gpio_value(pHandle1, nGpioPin1, nPin1PulseValue, pModeName);
    if (nRet == OK)
    {
        nRet = set_ircut_gpio_value(pHandle2, nGpioPin2, nPin2PulseValue, pModeName);
    }

    if (nRet == OK)
    {
        usleep(1000000);
        nRet = set_ircut_gpio_value(pHandle1, nGpioPin1, GPIO_LOW, pModeName);
    }
    if (nRet == OK)
    {
        nRet = set_ircut_gpio_value(pHandle2, nGpioPin2, GPIO_LOW, pModeName);
    }

    release_ircut_gpio_handle(pHandle1, bHandle1Initialized);
    release_ircut_gpio_handle(pHandle2, bHandle2Initialized);
    return nRet == OK ? IpcRet_E::OK : IpcRet_E::ERR;
}
} // namespace

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

/* ir_cut控制,夜晚模式 */
int CGpioCtrl::ir_cut_switch_night()
{
    std::lock_guard<std::mutex> lock(m_ircutMutex);
    unsigned int gpio_pin1 = ir_output_gpio_pins[0];
    unsigned int gpio_pin2 = ir_output_gpio_pins[1];

#if CAP_GPIO_IR_CUT_JSON
    return switch_ircut_gpio(gpio_pin1, gpio_pin2, GPIO_HIGHT, GPIO_LOW, "夜晚");
#else
    return switch_ircut_gpio(gpio_pin1, gpio_pin2, GPIO_LOW, GPIO_HIGHT, "夜晚");
#endif
}

/* ir_cut控制,白天模式 */
int CGpioCtrl::ir_cut_switch_day()
{
    std::lock_guard<std::mutex> lock(m_ircutMutex);
    unsigned int gpio_pin1 = ir_output_gpio_pins[0];
    unsigned int gpio_pin2 = ir_output_gpio_pins[1];

#if CAP_GPIO_IR_CUT_JSON
    return switch_ircut_gpio(gpio_pin1, gpio_pin2, GPIO_LOW, GPIO_HIGHT, "白天");
#else
    return switch_ircut_gpio(gpio_pin1, gpio_pin2, GPIO_HIGHT, GPIO_LOW, "白天");
#endif
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
