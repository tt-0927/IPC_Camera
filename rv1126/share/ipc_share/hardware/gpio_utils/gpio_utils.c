/*
 * @FilePath     : gpio_utils.c
 * @Author       : 李辉 lihui@kfb.cn
 * @Date         : 2025-02-21 10:40:11
 * @LastEditors: 李辉 lihui@kfb.cn
 * @LastEditTime: 2025-02-21 11:39:57
 * @Description  :
 */

#include "IpcRet.h"
#include "dlog.h"
#include "gpio_utils.h"
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <stdbool.h>

/**
 * @brief  检查 GPIO 引脚是否已经导出
 * @param  [unsigned int] nGpio - GPIO 编号
 * @return [bool] 返回 true 表示已经导出，false 表示未导出
 */
static bool is_gpio_exported(unsigned int nGpio) 
{
    char strPath[128];
    snprintf(strPath, sizeof(strPath), "/sys/class/gpio/gpio%u", nGpio);
    int nFd = open(strPath, O_RDONLY);
    if (nFd >= 0) {
        close(nFd);
        return true;
    }
    return false;
}

/**
 * @brief  导出 GPIO 引脚
 * @param  [unsigned int] nGpio - GPIO 编号
 * @return [int] 返回 0 表示成功，其他值表示失败
 */
static int export(unsigned int nGpio)
{
    if (is_gpio_exported(nGpio))
    {
        // dlog_info("gpio%d is already exported.", nGpio);
        return ERR;
    }

    if (nGpio == 0) 
    {
        dlog_error("Invalid GPIO number: 0");
        return ERR;
    }
    int nFd = open("/sys/class/gpio/export", O_WRONLY);
    if (nFd < 0) 
    {
        dlog_error("Failed to open /sys/class/gpio/export for GPIO %u", nGpio);
        return ERR;
    }
    char strBuf[32];
    snprintf(strBuf, sizeof(strBuf), "%u", nGpio);
    ssize_t nWritten = write(nFd, strBuf, strlen(strBuf));
    close(nFd);
    
    if (nWritten < 0) {
        dlog_error("Failed to write GPIO %u to export", nGpio);
        return ERR;
    }
    
    return OK;
}

/**
 * @brief  取消导出 GPIO 引脚
 * @param  [unsigned int] nGpio - GPIO 编号
 * @return [int] 返回 0 表示成功，其他值表示失败
 */
static int unexport(unsigned int nGpio)
{
    if (nGpio == 0) {
        dlog_error("Invalid GPIO number: 0");
        return ERR;
    }
    int nFd = open("/sys/class/gpio/unexport", O_WRONLY);
    if (nFd < 0) {
        dlog_error("Failed to open /sys/class/gpio/unexport for GPIO %u", nGpio);
        return ERR;
    }
    char strBuf[32];
    snprintf(strBuf, sizeof(strBuf), "%u", nGpio);
    ssize_t nWritten = write(nFd, strBuf, strlen(strBuf));
    close(nFd);
    
    if (nWritten < 0) {
        dlog_error("Failed to write GPIO %u to unexport", nGpio);
        return ERR;
    }
    
    return OK;
}

/**
 * @brief  设置 GPIO 引脚方向
 * @param  [unsigned int] nGpio - GPIO 编号
 * @param  [unsigned int] nIsOutput - 是否为输出模式
 * @return [int] 返回 0 表示成功，其他值表示失败
 */
static int set_direction(unsigned int nGpio, unsigned int nIsOutput)
{
    if (nGpio == 0) {
        dlog_error("Invalid GPIO number: 0");
        return ERR;
    }
    char strPath[128];
    snprintf(strPath, sizeof(strPath), "/sys/class/gpio/gpio%u/direction", nGpio);
    int nFd = open(strPath, O_WRONLY);
    if (nFd < 0) {
        dlog_error("Failed to open direction file for GPIO %u: %s", nGpio, strPath);
        return ERR;
    }
    const char *szDirection = nIsOutput ? "out" : "in";
    ssize_t nWritten = write(nFd, szDirection, strlen(szDirection));
    close(nFd);
    
    if (nWritten < 0) {
        dlog_error("Failed to set direction '%s' for GPIO %u", szDirection, nGpio);
        return ERR;
    }
    
    return OK;
}

/**
 * @brief  获取 GPIO 引脚方向
 * @param  [unsigned int] nGpio - GPIO 编号
 * @param  [unsigned int*] pnIsOutput - 输出方向指针（true: 输出模式, false: 输入模式）
 * @return [int] 返回 0 表示成功，其他值表示失败
 */
static int get_direction(unsigned int nGpio, unsigned int *pnIsOutput)
{
    if (nGpio == 0) {
        dlog_error("Invalid GPIO number: 0");
        return ERR;
    }
    
    if (pnIsOutput == NULL) {
        dlog_error("Output parameter is NULL for GPIO %u", nGpio);
        return ERR;
    }
    char szPath[128];
    snprintf(szPath, sizeof(szPath), "/sys/class/gpio/gpio%u/direction", nGpio);
    int nFd = open(szPath, O_RDONLY);
    if (nFd < 0) {
        dlog_error("Failed to open direction file for GPIO %u: %s", nGpio, szPath);
        return ERR;
    }
    char strBuf[8] = {0};
    ssize_t nBytesRead = read(nFd, strBuf, sizeof(strBuf) - 1);
    close(nFd);
    if (nBytesRead <= 0) {
        dlog_error("Failed to read direction for GPIO %u", nGpio);
        return ERR;
    }
    strBuf[nBytesRead] = '\0';
    if (strncmp(strBuf, "out", 3) == 0) {
        *pnIsOutput = true;
    } else if (strncmp(strBuf, "in", 2) == 0) {
        *pnIsOutput = false;
    } else {
        dlog_error("Unknown direction value '%s' for GPIO %u", strBuf, nGpio);
        return ERR;
    }
    return OK;
}

/**
 * @brief  设置 GPIO 引脚是否为低电平有效
 * @param  [unsigned int] nGpio - GPIO 编号
 * @param  [unsigned int] nLowActive - 是否为低电平有效
 * @return [int] 返回 0 表示成功，其他值表示失败
 */
static int set_lowActive(unsigned int nGpio, unsigned int nLowActive)
{
    if (nGpio == 0) {
        dlog_error("Invalid GPIO number: 0");
        return ERR;
    }
    char strPath[128];
    snprintf(strPath, sizeof(strPath), "/sys/class/gpio/gpio%u/active_low", nGpio);
    int nFd = open(strPath, O_WRONLY);
    if (nFd < 0) {
        dlog_error("Failed to open active_low file for GPIO %u: %s", nGpio, strPath);
        return ERR;
    }
    const char *szValue = nLowActive ? "1" : "0";
    ssize_t nWritten = write(nFd, szValue, strlen(szValue));
    close(nFd);
    
    if (nWritten < 0) {
        dlog_error("Failed to set active_low '%s' for GPIO %u", szValue, nGpio);
        return ERR;
    }
    
    return OK;
}

/**
 * @brief  设置 GPIO 引脚值
 * @param  [unsigned int] nGpio - GPIO 编号
 * @param  [unsigned int] nValue - GPIO 值（true: 高电平, false: 低电平）
 * @return [int] 返回 0 表示成功，其他值表示失败
 */
static int set_value(unsigned int nGpio, unsigned int nValue)
{
    if (nGpio == 0) {
        dlog_error("Invalid GPIO number: 0");
        return ERR;
    }
    char strPath[128];
    snprintf(strPath, sizeof(strPath), "/sys/class/gpio/gpio%u/value", nGpio);
    int nFd = open(strPath, O_WRONLY);
    if (nFd < 0) {
        dlog_error("Failed to open value file for GPIO %u: %s", nGpio, strPath);
        return ERR;
    }
    const char *szVal = nValue ? "1" : "0";
    ssize_t nWritten = write(nFd, szVal, strlen(szVal));
    close(nFd);
    
    if (nWritten < 0) {
        dlog_error("Failed to set value '%s' for GPIO %u", szVal, nGpio);
        return ERR;
    }
    
    return OK;
}

/**
 * @brief  获取 GPIO 引脚值
 * @param  [unsigned int] nGpio - GPIO 编号
 * @param  [unsigned int*] pnOutValue - 输出 GPIO 值（true: 高电平, false: 低电平）
 * @return [int] 返回 0 表示成功，其他值表示失败
 */
static int get_value(unsigned int nGpio, unsigned int *nValue)
{
    if (nGpio == 0) {
        dlog_error("Invalid GPIO number: 0");
        return ERR;
    }
    
    if (nValue == NULL) {
        dlog_error("Output parameter is NULL for GPIO %u", nGpio);
        return ERR;
    }
    char strPath[128];
    snprintf(strPath, sizeof(strPath), "/sys/class/gpio/gpio%u/value", nGpio);
    int nFd = open(strPath, O_RDONLY);
    if (nFd < 0) {
        dlog_error("Failed to open value file for GPIO %u: %s", nGpio, strPath);
        return ERR;
    }
    char strBuf[4];
    ssize_t nBytesRead = read(nFd, strBuf, sizeof(strBuf) - 1);
    close(nFd);
    
    if (nBytesRead <= 0) {
        dlog_error("Failed to read value for GPIO %u", nGpio);
        return ERR;
    }
    
    *nValue = atoi(strBuf) != 0;
    return OK;
}

static int gpio_init(GpioHandle_S* pHandle)
{
    if (pHandle == NULL) {
        dlog_error("GPIO handle is NULL");
        return ERR;
    }
    int nRet = export(pHandle->stNeedParam.nGpio);
    if (nRet != OK) 
    {
        /* 检查GPIO是否已经存在，如果存在则继续初始化，否则返回错误 */ 
        if (!is_gpio_exported(pHandle->stNeedParam.nGpio))
        {
            dlog_error("gpio_init: GPIO %u export failed and not exists", 
                      pHandle->stNeedParam.nGpio);
            return nRet;
        }
        // dlog_info("gpio_init: GPIO %u export failed but already exists, continue initialization", 
        //           pHandle->stNeedParam.nGpio);
    }
    
    nRet = set_direction(pHandle->stNeedParam.nGpio, pHandle->stNeedParam.nIsOutput);
    if (nRet != OK) 
    {
        dlog_error("Failed to set direction for GPIO %u", pHandle->stNeedParam.nGpio);
        return ERR;
    }
    nRet = set_lowActive(pHandle->stNeedParam.nGpio, pHandle->stNeedParam.nLowActive);
    if (nRet != OK) 
    {
        dlog_error("Failed to set low active for GPIO %u", pHandle->stNeedParam.nGpio);
        return ERR;
    }
    if(pHandle->stNeedParam.nIsOutput)
    {
        nRet = set_value(pHandle->stNeedParam.nGpio, pHandle->stNeedParam.nValue);
        if (nRet != OK) {
            dlog_error("Failed to set initial value for GPIO %u", pHandle->stNeedParam.nGpio);
            return ERR;
        }
    }
    return OK;
}

static int gpio_uninit(GpioHandle_S* pHandle)
{
    if (pHandle == NULL) {
        dlog_error("GPIO handle is NULL");
        return ERR;
    }
    
    int nRet = unexport(pHandle->stNeedParam.nGpio);
    if (nRet != OK) {
        dlog_error("Failed to unexport GPIO %u", pHandle->stNeedParam.nGpio);
        return ERR;
    }
    return OK;
}

/*分配一个GPIO句柄*/
GpioHandle_S* gpio_alloc(GpioNeedParam_S stNeedParam)
{
    GpioHandle_S *pHandle = (GpioHandle_S *)malloc(sizeof(GpioHandle_S));
    if (pHandle == NULL) {
        dlog_error("Failed to allocate memory for GPIO handle");
        return NULL;
    }
    
    memset(pHandle, 0, sizeof(GpioHandle_S));
    memcpy(&pHandle->stNeedParam, &stNeedParam, sizeof(GpioNeedParam_S));
    pHandle->get_direction = get_direction;
    pHandle->set_direction = set_direction;
    pHandle->get_value = get_value;
    pHandle->set_value = set_value;
    pHandle->set_lowActive = set_lowActive;
    pHandle->gpio_init = gpio_init;
    pHandle->gpio_uninit = gpio_uninit;
    return pHandle;
}

/*释放一个GPIO句柄*/
int gpio_release( GpioHandle_S* pHandle )
{
    if (pHandle == NULL) {
        dlog_error("Attempting to release NULL GPIO handle");
        return ERR;
    }
    
    free(pHandle);
    return OK;
}
