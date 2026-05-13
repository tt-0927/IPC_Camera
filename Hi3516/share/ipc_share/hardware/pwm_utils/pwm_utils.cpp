/*
 * @FilePath     : pwm_utils.c
 * @Author       : cyc
 * @Date         : 2025-06-06 16:16:10
 * @LastEditors  : cyc
 * @LastEditTime : 2025-06-07 09:59:04
 * @Description  : pwm控制通用类
 */

#include "pwm_utils.h"
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <stdbool.h>
#include <iostream>
#include "dlog.h"
#include "IpcRet.h"

/**
 * @brief  检查 PWM 引脚是否已经导出
 * @param  [unsigned int] nPwm - PWM 编号
 * @param  [unsigned int] nPwmChn - PWM 通道
 * @return [bool] 返回 true 表示已经导出，false 表示未导出
 */
static bool is_pwm_exported(unsigned int nPwm, unsigned int nPwmChn) 
{
    std::string strPath = "/sys/class/pwm/pwmchip" + std::to_string(nPwm) + "/pwm" + std::to_string(nPwmChn);
    int nFd = open(strPath.c_str(), O_RDONLY);
    if (nFd >= 0) {
        close(nFd);
        return true;
    }
    return false;
}

/**
 * @brief  导出 PWM 引脚
 * @param  [unsigned int] nPwm - PWM 编号
 * @param  [unsigned int] nPwmChn - PWM 通道
 * @return [int] 返回 0 表示成功，其他值表示失败
 */
static int pwm_export(unsigned int nPwm,unsigned int nPwmChn)
{
    if (is_pwm_exported(nPwm, nPwmChn))
    {
        dlog_info("PWM %u channel %u is already exported.", nPwm, nPwmChn);
        return IpcRet_E::OK;
    }

    int nRet = IpcRet_E::ERR;
    std::string strPath = "/sys/class/pwm/pwmchip" + std::to_string(nPwm) + "/export";
    int nFd = open(strPath.c_str(), O_WRONLY);
    if (nFd < IpcRet_E::OK) 
    {
        dlog_error("pwm_export open error nPwm:%u,nPwmChn:%u",nPwm,nPwmChn);
        return IpcRet_E::ERR;
    }
    std::string strValue = std::to_string(nPwmChn);
    nRet = write(nFd, strValue.c_str(), strValue.length());
    if(nRet <= IpcRet_E::OK)
    {
        dlog_error("pwm_export write error nPwm:%u,nPwmChn:%u",nPwm,nPwmChn);
        close(nFd);
        return IpcRet_E::ERR;
    }
    close(nFd);
    return IpcRet_E::OK;
}

/**
 * @brief  取消导出 PWM 引脚
 * @param  [unsigned int] nPwm - PWM 编号
 * @return [int] 返回 0 表示成功，其他值表示失败
 */
static int unexport(unsigned int nPwm)
{
    std::string strPath = "/sys/class/pwm/pwmchip" + std::to_string(nPwm) + "/unexport";
    int nFd = open(strPath.c_str(), O_WRONLY);
    if (nFd < IpcRet_E::OK) 
    {
        dlog_error("unexport open error nPwm:%u",nPwm);
        return IpcRet_E::ERR;
    }

    close(nFd);
    return IpcRet_E::OK;
}

/**
 * @brief  设置 PWM 通道周期
 * @param  [unsigned int] nPwm - PWM 编号
 * @param  [unsigned int] nPwmChn - PWM 通道
 * @param  [unsigned int] nPeriodVal - 通道周期值
 * @return [int] 返回 0 表示成功，其他值表示失败
 */
static int set_period(unsigned int nPwm,unsigned int nPwmChn, unsigned int nPeriodVal)
{
    int nRet = IpcRet_E::ERR;
    std::string strPath = "/sys/class/pwm/pwmchip" + std::to_string(nPwm) + "/pwm" + std::to_string(nPwmChn)+"/period";
    int nFd = open(strPath.c_str(), O_WRONLY);
    if (nFd < IpcRet_E::OK) 
    {
        dlog_error("set_period open error nPwm:%u,nPwmChn:%u",nPwm,nPwmChn);
        return IpcRet_E::ERR;
    }
    std::string strValue = std::to_string(nPeriodVal);
    nRet = write(nFd, strValue.c_str(), strValue.length());
    if(nRet <= IpcRet_E::OK)
    {
        dlog_error("set_period write error nPwm:%u,nPwmChn:%u",nPwm,nPwmChn);
        close(nFd);
        return IpcRet_E::ERR;
    }

    close(nFd);
    return IpcRet_E::OK;
}

/**
 * @brief  获取 PWM 通道周期值
 * @param  [unsigned int] nPwm - PWM 编号
 * @param  [unsigned int] nPwmChn - PWM 通道
 * @param  [unsigned int*] pPeriodVal - 通道周期值
 * @return [int] 返回 0 表示成功，其他值表示失败
 */
static int get_period(unsigned int nPwm, unsigned int nPwmChn,unsigned int *pPeriodVal)
{
    if (pPeriodVal == NULL) 
    {
        dlog_error("pPeriodVal is NULL nPwm:%u,nPwmChn:%u",nPwm,nPwmChn);
        return IpcRet_E::ERR;
    }

    int nRet = IpcRet_E::ERR;
    std::string strPath = "/sys/class/pwm/pwmchip" + std::to_string(nPwm) + "/pwm" + std::to_string(nPwmChn)+"/period";
    int nFd = open(strPath.c_str(), O_RDONLY);
    if (nFd < IpcRet_E::OK) 
    {
        dlog_error("get_period open error nPwm:%u,nPwmChn:%u",nPwm,nPwmChn);
        return IpcRet_E::ERR;
    }

    char strBuf[128] = {0};
    nRet = read(nFd, strBuf, sizeof(strBuf) - 1);
    if (nRet <= IpcRet_E::OK) 
    {
        dlog_error("get_period read error nPwm:%u,nPwmChn:%u",nPwm,nPwmChn);
        close(nFd);
        return IpcRet_E::ERR;
    }
    else
    {
        *pPeriodVal = std::stoul(strBuf);
    }

    close(nFd);
    return IpcRet_E::OK;
}

/**
 * @brief  设置 PWM polarity
 * @param  [unsigned int] nPwm - PWM 编号
 * @param  [unsigned int] nPwmChn - PWM 通道
 * @return [int] 返回 0 表示成功，其他值表示失败
 */
 static int set_polarity(unsigned int nPwm,unsigned int nPwmChn)
 {
     int nRet = IpcRet_E::ERR;
     std::string strPath = "/sys/class/pwm/pwmchip" + std::to_string(nPwm) + "/pwm" + std::to_string(nPwmChn)+"/polarity";
     int nFd = open(strPath.c_str(), O_WRONLY);
     if (nFd < IpcRet_E::OK) 
     {
         dlog_error("set_period open error nPwm:%u,nPwmChn:%u",nPwm,nPwmChn);
         return IpcRet_E::ERR;
     }
     std::string strValue = "normal";
     nRet = write(nFd, strValue.c_str(), strValue.length());
     if(nRet <= IpcRet_E::OK)
     {
         dlog_error("set_period write error nPwm:%u,nPwmChn:%u",nPwm,nPwmChn);
         close(nFd);
         return IpcRet_E::ERR;
     }
 
     close(nFd);
     return IpcRet_E::OK;
 }
 
/**
 * @brief  设置 PWM 通道占空比
 * @param  [unsigned int] nPwm - PWM 编号
 * @param  [unsigned int] nPwmChn - PWM 通道
 * @param  [unsigned int] nDutyCycleVal - 占空比值
 * @return [int] 返回 0 表示成功，其他值表示失败
 */
static int set_duty_cycle(unsigned int nPwm, unsigned int nPwmChn, unsigned int nDutyCycleVal)
{
    int nRet = IpcRet_E::ERR;
    std::string strPath = "/sys/class/pwm/pwmchip" + std::to_string(nPwm) + "/pwm" + std::to_string(nPwmChn)+"/duty_cycle";
    int nFd = open(strPath.c_str(), O_WRONLY);
    if (nFd < IpcRet_E::OK) 
    {
        dlog_error("set_duty_cycle open error nPwm:%u,nPwmChn:%u",nPwm,nPwmChn);
        return IpcRet_E::ERR;
    }
    std::string strValue = std::to_string(nDutyCycleVal);
    nRet = write(nFd, strValue.c_str(),strValue.length());
    if(nRet <= IpcRet_E::OK)
    {
        dlog_error("set_duty_cycle write error nPwm:%u,nPwmChn:%u",nPwm,nPwmChn);
        close(nFd);
        return IpcRet_E::ERR;
    }

    close(nFd);
    return IpcRet_E::OK;
}

/**
 * @brief  获取 PWM 通道占空比
 * @param  [unsigned int] nPwm - PWM 编号
 * @param  [unsigned int] nPwmChn - PWM 通道
 * @param  [unsigned *int] nDutyCycleVal - 占空比值
 * @return [int] 返回 0 表示成功，其他值表示失败
 */
static int get_duty_cycle(unsigned int nPwm, unsigned int nPwmChn, unsigned int *pDutyCycleVal)
{
    if (pDutyCycleVal == NULL) 
    {
        dlog_error("pDutyCycleVal is NULL nPwm:%u,nPwmChn:%u",nPwm,nPwmChn);
        return IpcRet_E::ERR;
    }

    int nRet = IpcRet_E::ERR;
    std::string strPath = "/sys/class/pwm/pwmchip" + std::to_string(nPwm) + "/pwm" + std::to_string(nPwmChn)+"/duty_cycle";
    int nFd = open(strPath.c_str(), O_RDONLY);
    if (nFd < IpcRet_E::OK) 
    {
        dlog_error("get_duty_cycle open error nPwm:%u,nPwmChn:%u",nPwm,nPwmChn);
        return IpcRet_E::ERR;
    }

    char strBuf[128] = {0};
    nRet = read(nFd, strBuf, sizeof(strBuf) - 1);
    if (nRet <= IpcRet_E::OK) 
    {
        dlog_error("get_duty_cycle read error nPwm:%u,nPwmChn:%u",nPwm,nPwmChn);
        close(nFd);
        return IpcRet_E::ERR;
    }
    else
    {
        *pDutyCycleVal = std::stoul(strBuf);
    }

    close(nFd);  
    return IpcRet_E::OK;
}

/**
 * @brief  设置 PWM 通道使能
 * @param  [unsigned int] nPwm - PWM 编号
 * @param  [unsigned int] nPwmChn - PWM 通道
 * @param  [unsigned int] nEnable - 使能
 * @return [int] 返回 0 表示成功，其他值表示失败
 */
static int set_enable(unsigned int nPwm, unsigned int nPwmChn, unsigned int nEnable)
{
    int nRet = IpcRet_E::ERR;
    std::string strPath = "/sys/class/pwm/pwmchip" + std::to_string(nPwm) + "/pwm" + std::to_string(nPwmChn)+"/enable";
    int nFd = open(strPath.c_str(), O_WRONLY);
    if (nFd < IpcRet_E::OK) 
    {
        dlog_error("set_enable open error nPwm:%u,nPwmChn:%u",nPwm,nPwmChn);
        return IpcRet_E::ERR;
    }
    std::string strValue = std::to_string(nEnable);
    nRet = write(nFd, strValue.c_str(), strValue.length());
    if(nRet <= IpcRet_E::OK)
    {
        dlog_error("set_enable write error nPwm:%u,nPwmChn:%u",nPwm,nPwmChn);
        close(nFd);
        return IpcRet_E::ERR;
    }
    close(nFd);
    return IpcRet_E::OK;
}

/**
 * @brief  获取 PWM 通道使能
 * @param  [unsigned int] nPwm - PWM 编号
 * @param  [unsigned int] nPwmChn - PWM 通道
 * @param  [unsigned *int] nEnable - 使能
 * @return [int] 返回 0 表示成功，其他值表示失败
 */
static int get_enable(unsigned int nPwm, unsigned int nPwmChn, unsigned int *pEnable)
{
    if (pEnable == NULL) 
    {
        dlog_error("pEnable is NULL nPwm:%u,nPwmChn:%u",nPwm,nPwmChn);
        return IpcRet_E::ERR;
    }

    int nRet = IpcRet_E::ERR;
    std::string strPath = "/sys/class/pwm/pwmchip" + std::to_string(nPwm) + "/pwm" + std::to_string(nPwmChn)+"/enable";
    int nFd = open(strPath.c_str(), O_RDONLY);
    if (nFd < 0) 
    {
        dlog_error("get_enable open error nPwm:%u,nPwmChn:%u",nPwm,nPwmChn);
        return IpcRet_E::ERR;
    }

    char strBuf[32];
    nRet = read(nFd, strBuf, sizeof(strBuf) - 1);
    if(nRet <= IpcRet_E::OK)
    {
        dlog_error("get_enable read error nPwm:%u,nPwmChn:%u",nPwm,nPwmChn);
        close(nFd);
        return IpcRet_E::ERR;

    }
    else
    {
        *pEnable = atoi(strBuf);
    }
    close(nFd);
    return IpcRet_E::OK;
}

static int pwm_init(PwmHandle_S* pHandle)
{
    /* 尝试导出PWM，如果已经存在则继续，如果导出失败但PWM已存在也继续 */ 
    int nRet = pwm_export(pHandle->stNeedParam.nPwm, pHandle->stNeedParam.nPwmChn);
    if (nRet != IpcRet_E::OK)
    {
        /* 检查PWM是否已经存在，如果存在则继续初始化，否则返回错误 */ 
        if (!is_pwm_exported(pHandle->stNeedParam.nPwm, pHandle->stNeedParam.nPwmChn))
        {
            dlog_error("pwm_init: PWM %u channel %u export failed and not exists", 
                    pHandle->stNeedParam.nPwm, pHandle->stNeedParam.nPwmChn);
            return nRet;
        }
        dlog_info("pwm_init: PWM %u channel %u export failed but already exists, continue initialization", 
                pHandle->stNeedParam.nPwm, pHandle->stNeedParam.nPwmChn);
    }
    
    nRet = set_period(pHandle->stNeedParam.nPwm, pHandle->stNeedParam.nPwmChn, pHandle->stNeedParam.nPeriod);
    if (nRet != IpcRet_E::OK)
    {
        return nRet;
    }
    nRet = set_duty_cycle(pHandle->stNeedParam.nPwm, pHandle->stNeedParam.nPwmChn, pHandle->stNeedParam.nDutyCycle);
    if (nRet != IpcRet_E::OK)
    {
        return nRet;
    }

#if CAP_PWM_NEED_POLARITY // PWM 极性配置
    nRet = set_polarity(pHandle->stNeedParam.nPwm, pHandle->stNeedParam.nPwmChn);
    if (nRet != IpcRet_E::OK)
    {
        return nRet;
    }
#endif

    return IpcRet_E::OK;
}

static int pwm_uninit(PwmHandle_S* pHandle)
{
    unexport(pHandle->stNeedParam.nPwm);
    return IpcRet_E::OK;
}

/*分配一个PWM句柄*/
PwmHandle_S* pwm_alloc(PwmNeedParam_S stNeedParam)
{
    PwmHandle_S *pHandle = (PwmHandle_S *)malloc(sizeof(PwmHandle_S));
    memset(pHandle, 0, sizeof(PwmHandle_S));
    memcpy(&pHandle->stNeedParam, &stNeedParam, sizeof(PwmNeedParam_S));

    pHandle->set_period = [](PwmHandle_S* pHandle, unsigned int nPeriodVal) {
        return set_period(pHandle->stNeedParam.nPwm, pHandle->stNeedParam.nPwmChn, nPeriodVal);
    };
    pHandle->get_period = [](PwmHandle_S* pHandle, unsigned int *pPeriodVal) {
        return get_period(pHandle->stNeedParam.nPwm, pHandle->stNeedParam.nPwmChn, pPeriodVal);
    };
    pHandle->set_duty_cycle = [](PwmHandle_S* pHandle, unsigned int nDutyCycleVal) {
        return set_duty_cycle(pHandle->stNeedParam.nPwm, pHandle->stNeedParam.nPwmChn, nDutyCycleVal);
    };
    pHandle->get_duty_cycle = [](PwmHandle_S* pHandle, unsigned int *pDutyCycleVal) {
        return get_duty_cycle(pHandle->stNeedParam.nPwm, pHandle->stNeedParam.nPwmChn, pDutyCycleVal);
    };
    pHandle->set_enable = [](PwmHandle_S* pHandle, unsigned int nEnable) {
        return set_enable(pHandle->stNeedParam.nPwm, pHandle->stNeedParam.nPwmChn, nEnable);
    };
    pHandle->get_enable = [](PwmHandle_S* pHandle, unsigned int *pEnable) {
        return get_enable(pHandle->stNeedParam.nPwm, pHandle->stNeedParam.nPwmChn, pEnable);
    };
    pHandle->pwm_init = pwm_init;
    pHandle->pwm_uninit = pwm_uninit;
    return pHandle;
}

/*释放一个PWM句柄*/
int pwm_release( PwmHandle_S* pHandle )
{
    if (pHandle)
    {
        free(pHandle);
    }
    return IpcRet_E::OK;
}
