/**
 * @FilePath     : cipher_trng.h
 * @Author       : zhouzirui
 * @Date         : 2025-04-03 09:54:59
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-05-27 15:06:59
 * @Description  : 
 */

#ifndef __CIPHER_TRNG_H__
#define __CIPHER_TRNG_H__
#ifdef __cplusplus
extern "C"
{
#endif

#include "ot_mpi_cipher.h"
#include "mpi_common.h"
#include "mpi_buf.h"

/**
 * @brief       : 获取4字节的硬件随机数
 * @author      : zhouzirui
 * @param        {td_u32} *pRandnum：指向存储生成的随机数的缓冲区的指针
 * @return       {*}成功返回0,失败返回-1
 */
int cipherTrng_getRandom(td_u32 *pRandnum);

/**
 * @brief       : 获取任意字节的硬件随机数
 * @author      : zhouzirui
 * @param        {td_u32} u32Size：需要生成的随机数的字节数
 * @param        {td_u32} *pRandnum：指向存储生成的随机数的缓冲区的指针
 * @return       {*}成功返回0,失败返回-1
 */
int cipherTrng_getMultiRandom(td_u32 u32Size,td_u8 *pRandnum);

#ifdef __cplusplus
}
#endif
#endif // __CIPHER_TRNG_H__