/**
 * @FilePath     : cipher_hash.h
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2025-04-03 09:54:43
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-05-27 15:06:32
 * @Description  : HASH及HMAC摘要算法
 */

 #ifndef __CIPHER_HASH_H__
#define __CIPHER_HASH_H__

#ifdef __cplusplus
extern "C"
{
#endif

#include "ot_mpi_cipher.h"
#include "mpi_common.h"
#include "mpi_buf.h"

/*cipher hash必需参数*/
typedef struct _CipherHashNeedParam_S
{
    /*HASH 算法类型*/
    crypto_hash_type enHashType;
    
} CipherHashNeedParam_S;

typedef struct _CipherHash_S CipherHash_S;
struct _CipherHash_S
{
    //info /**********************必需参数***************************/
    CipherHashNeedParam_S stNeedParam;
    //info /**********************辅助参数***************************/
    
    //info /**********************功能列表***************************/

    /*安全协议加速器HASH及HMAC摘要算法模块初始化*/
    int (*cipherHash_init)(CipherHash_S *pHandle);

    /*安全协议加速器HASH及HMAC摘要算法模块去初始化*/
    int (*cipherHash_uninit)(CipherHash_S *pHandle);

    /*HASH 计算*/
    int (*cipherHash_HashCompute)(CipherHash_S *pHandle, crypto_hash_attr stHashAttr, const MpiBuf_S *pSrcBuf, MpiBuf_S *pDstBuf);

    /*HMAC 计算*/
    int (*cipherHash_HmacCompute)(CipherHash_S *pHandle, crypto_hash_attr stHashAttr, const MpiBuf_S *pSrcBuf, MpiBuf_S *pDstBuf);

    /*HASH Clone 计算*/
    int (*cipherHash_HashCloneCompute)(CipherHash_S *pHandle, crypto_hash_attr stHashAttr, const MpiBuf_S *pSrcBuf1, const MpiBuf_S *pSrcBuf2, MpiBuf_S *pDstBuf);

    /*HMAC Clone 计算*/
    int (*cipherHash_HmacCloneCompute)(CipherHash_S *pHandle, crypto_hash_attr stHashAttr, const MpiBuf_S *pSrcBuf1, const MpiBuf_S *pSrcBuf2, MpiBuf_S *pDstBuf);

    /*PBKDF2 计算*/
    int (*cipherHash_pbkdf2Compute)(CipherHash_S *pHandle, crypto_kdf_pbkdf2_param stParam, MpiBuf_S *pDstBuf);
};

/**
 * @brief       : 分配安全协议加速器HASH及HMAC摘要算法模块句柄
 * @author      : zhouzirui
 * @param        {CipherHashNeedParam_S} stNeedParam：必须参数
 * @return       {*}成功返回句柄，失败返回NULL
 */
CipherHash_S *cipherHash_alloc(CipherHashNeedParam_S stNeedParam);

/**
 * @brief       : 释放安全协议加速器HASH及HMAC摘要算法模块句柄
 * @author      : zhouzirui
 * @param        {CipherHash_S} *pHandle：句柄
 * @return       {*}
 */
void cipherHash_release(CipherHash_S *pHandle);

#ifdef __cplusplus
}
#endif
#endif // __CIPHER_HASH_H__