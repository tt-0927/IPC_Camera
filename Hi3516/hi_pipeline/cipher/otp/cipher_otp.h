/**
 * @FilePath     : cipher_otp.h
 * @Author       : zhouzirui
 * @Date         : 2025-04-03 09:56:17
 * @LastEditors  : zhouzirui
 * @LastEditTime : 2025-04-07 20:11:46
 * @Description  : OTP非易失性存储器的读写
 */

#ifndef __CIPHER_OTP_H__
#define __CIPHER_OTP_H__
#ifdef __cplusplus
extern "C"
{
#endif

#include "ot_mpi_otp.h"
#include "mpi_common.h"

/**
 * @brief       : 按字读取指定OTP地址中的值
 * @author      : zhouzirui
 * @param        {td_u32} u32Offset：要执行读操作的OTP地址
 * @param        {td_u32} *pData：用于存储从指定OTP地址中读取出的值的指针
 * @return       {*}成功返回0,失败返回-1
 */
int cipherOtp_read_word(td_u32 u32Offset, td_u32 *pData);

/**
 * @brief       : 按字节读取指定OTP地址中的值
 * @author      : zhouzirui
 * @param        {td_u32} u32Offset：要执行读操作的OTP地址
 * @param        {td_u8} *pData：用于存储从指定OTP地址中读取出的值的指针
 * @return       {*}成功返回0,失败返回-1
 */
int cipherOtp_read_byte(td_u32 u32Offset, td_u8 *pData);

/**
 * @brief       : 按字节往指定OTP地址写值
 * @author      : zhouzirui
 * @param        {td_u32} u32Offset：要执行写操作的OTP地址
 * @param        {td_u8} u8Data：指定OTP地址中要写入的值
 * @return       {*}成功返回0,失败返回-1
 */
    int cipherOtp_write_byte(td_u32 u32Offset, td_u8 u8Data);

#ifdef __cplusplus
}
#endif
#endif // __CIPHER_OTP_H__