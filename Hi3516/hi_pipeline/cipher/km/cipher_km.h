/**
 * @FilePath     : cipher_km.h
 * @Author       : zhouzirui
 * @Date         : 2025-04-03 09:56:04
 * @LastEditors  : zhouzirui
 * @LastEditTime : 2025-04-07 20:57:55
 * @Description  : 密钥管理模块
 */

#ifndef __CIPHER_KM_H__
#define __CIPHER_KM_H__
#ifdef __cplusplus
extern "C"
{
#endif

#include <string.h>
#include "ot_mpi_km.h"
#include "mpi_common.h"
#include "mpi_buf.h"

/*最大盐值长度*/
#define MAX_SALT_LEN      28

/*cipher hash必需参数*/
typedef struct _CipherKmNeedParam_S
{
    /*密钥槽类型*/
    km_keyslot_type enKeyslot_type;
    /*派生出的密钥将被送往的模块*/
    km_klad_dest_type enKlad_type;
    /*根密钥属性*/
    td_u32 u32RootKey_type; //KM_KLAD_KEY_TYPE_XXXX
    /*密钥可用于的模块和算法类型*/
    km_crypto_alg enEngine;
    /*密钥是否可用于解密*/
    td_bool bDecrypt_support;
    /*密钥是否可用于加密*/
    td_bool bEncrypt_support;
    /*KDF硬件派生算法*/
    crypto_kdf_hard_alg enKdf_hard_alg;
    /*密钥奇偶属性配置*/
    td_bool bKey_parity;
    /*指定密钥大小*/
    crypto_klad_key_size enKey_size;
    /*盐值*/
    // td_u8 u8Salt;
    /*盐值长度*/
    // td_u32 u32Salt_length;
    /*ONEWAY寄存器配置*/
    td_bool bOneway;

} CipherKmNeedParam_S;

typedef struct _CipherKm_S CipherKm_S;
struct _CipherKm_S
{
    //info /**********************必需参数***************************/
    CipherKmNeedParam_S stNeedParam;
    /*密钥槽句柄*/
    crypto_handle mpi_keyslot_handle;
    /*密钥层级派生模块句柄*/
    crypto_handle mpi_klad_handle;
    
    //info /**********************辅助参数***************************/
    
    //info /**********************功能列表***************************/

    /*密钥管理模块初始化*/
    int (*cipherKm_init)(CipherKm_S *pHandle);

    /*密钥管理模块去初始化*/
    int (*cipherKm_uninit)(CipherKm_S *pHandle);

    /*根密钥传递，生成工作密钥，直接存储在密钥槽中*/
    int (*cipherKm_rootKey_delivery)(CipherKm_S *pHandle);
};

/**
 * @brief       : 分配密钥管理模块句柄
 * @author      : zhouzirui
 * @param        {CipherKmNeedParam_S} stNeedParam：必须参数
 * @return       {*}成功返回句柄，失败返回NULL
 */
CipherKm_S *cipherKm_alloc(CipherKmNeedParam_S stNeedParam);

/**
 * @brief       : 释放密钥管理模块句柄
 * @author      : zhouzirui
 * @param        {CipherKm_S} *pHandle：句柄
 * @return       {*}
 */
void cipherKm_release(CipherKm_S *pHandle);

#ifdef __cplusplus
}
#endif
#endif // __CIPHER_KM_H__