/**
 * @FilePath     : cipher_symc.h
 * @Author       : zhouzirui
 * @Date         : 2025-04-03 09:55:17
 * @LastEditors  : zhouzirui
 * @LastEditTime : 2025-04-03 15:49:09
 * @Description  : 对称加解密算法模块
 */

#ifndef __CIPHER_SYMC_H__
#define __CIPHER_SYMC_H__
#ifdef __cplusplus
extern "C"
{
#endif

#include "ot_mpi_cipher.h"
#include "mpi_common.h"
#include "mpi_buf.h"

/*cipher symc必需参数*/
typedef struct _CipherSymcNeedParam_S
{
    // /*HASH 算法类型*/
    // crypto_hash_type enHashType;
    
} CipherSymcNeedParam_S;

typedef struct _CipherSymc_S CipherSymc_S;
struct _CipherSymc_S
{
    //info /**********************必需参数***************************/
    CipherSymcNeedParam_S stNeedParam;
    //info /**********************辅助参数***************************/
    
    //info /**********************功能列表***************************/

    /*安全协议加速器对称加解密算法模块初始化*/
    int (*cipherSymc_init)(CipherSymc_S *pHandle);

    /*安全协议加速器对称加解密算法模块去初始化*/
    int (*cipherSymc_uninit)(CipherSymc_S *pHandle);

    /*对称加密*/
    int (*cipherSymc_encryption)(CipherSymc_S *pHandle, crypto_symc_attr stSymcAttr, td_handle keyslotHandle, crypto_symc_ctrl_t stSymcCtrl, crypto_buf_attr *pSrcBuf, crypto_buf_attr *pDstBuf, td_u32 u32Length, MpiBuf_S *pDstTagBuf);
    
    /*对称解密*/
    int (*cipherSymc_decryption)(CipherSymc_S *pHandle, crypto_symc_attr stSymcAttr, td_handle keyslotHandle, crypto_symc_ctrl_t stSymcCtrl, crypto_buf_attr *pSrcBuf, crypto_buf_attr *pDstBuf, td_u32 u32Length, MpiBuf_S *pDstTagBuf);
    
    /*MAC计算*/
    int (*cipherSymc_mac)(CipherSymc_S *pHandle, crypto_symc_mac_attr stMacAttr, crypto_buf_attr *pSrcBuf, td_u32 u32Length, MpiBuf_S *pDstBuf);

};

/**
 * @brief       : 分配安全协议加速器对称加解密算法模块句柄
 * @author      : zhouzirui
 * @param        {CipherSymcNeedParam_S} stNeedParam：必须参数
 * @return       {*}成功返回句柄，失败返回NULL
 */
CipherSymc_S *cipherSymc_alloc(CipherSymcNeedParam_S stNeedParam);

/**
 * @brief       : 释放安全协议加速器对称加解密算法模块句柄
 * @author      : zhouzirui
 * @param        {CipherSymc_S} *pHandle：句柄
 * @return       {*}
 */
void cipherSymc_release(CipherSymc_S *pHandle);

#ifdef __cplusplus
}
#endif
#endif // __CIPHER_SYMC_H__