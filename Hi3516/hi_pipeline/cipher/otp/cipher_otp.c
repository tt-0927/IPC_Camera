/**
 * @FilePath     : cipher_otp.c
 * @Author       : zhouzirui
 * @Date         : 2025-04-03 09:56:12
 * @LastEditors  : zhouzirui
 * @LastEditTime : 2025-04-07 20:12:08
 * @Description  : OTP非易失性存储器的读写
 */

#include "cipher_otp.h"

int cipherOtp_read_word(td_u32 u32Offset, td_u32 *pData)
{
    /*OTP模块初始化*/
    CHECK_API_RETURN(ot_mpi_otp_init());
    /*按字读取指定OTP地址中的值*/
    CHECK_API_RETURN(ot_mpi_otp_read_word(u32Offset, pData));
    /*OTP模块去初始化*/
    CHECK_API_RETURN(ot_mpi_otp_deinit());
    
    return TD_SUCCESS;
}

int cipherOtp_read_byte(td_u32 u32Offset, td_u8 *pData)
{
    /*OTP模块初始化*/
    CHECK_API_RETURN(ot_mpi_otp_init());
    /*按字节读取指定OTP地址中的值*/
    CHECK_API_RETURN(ot_mpi_otp_read_byte(u32Offset, pData));
    /*OTP模块去初始化*/
    CHECK_API_RETURN(ot_mpi_otp_deinit());
    
    return TD_SUCCESS;
}

int cipherOtp_write_byte(td_u32 u32Offset, td_u8 u8Data)
{
    /*OTP模块初始化*/
    CHECK_API_RETURN(ot_mpi_otp_init());
    /*按字节往指定OTP地址写值*/
    CHECK_API_RETURN(ot_mpi_otp_write_byte(u32Offset, u8Data));
    /*OTP模块去初始化*/
    CHECK_API_RETURN(ot_mpi_otp_deinit());
    
    return TD_SUCCESS;
}