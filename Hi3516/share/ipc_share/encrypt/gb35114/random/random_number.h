/**
 * @FilePath     : random_number.h
 * @Author       : zhouzirui
 * @Date         : 2025-04-09 15:07:38
 * @LastEditors  : zhouzirui
 * @LastEditTime : 2025-04-09 17:13:59
 * @Description  : 随机数模块
 */

#ifndef _RANDOM_NUMBER_H_
#define _RANDOM_NUMBER_H_

#include "IpcRet.h"

#ifdef  __cplusplus
extern "C" {
#endif
    
#include <stdint.h>
#include <stdlib.h>
#include "gmssl/rand.h"

#ifdef  __cplusplus
}
#endif

/**
 * @brief       : 随机数获取
 * @author      : zhouzirui
 * @param        {uint8_t} *buf：存储随机数缓冲区指针
 * @param        {size_t} buflen：需要多少位随机数
 * @return       {*}0：成功 非0：失败
 */
int randomNumber_get(uint8_t *buf, size_t buflen);

#endif //_RANDOM_NUMBER_H_