/**
 * @FilePath     : cipher_km.c
 * @Author       : zhouzirui
 * @Date         : 2025-04-03 09:56:01
 * @LastEditors  : zhouzirui
 * @LastEditTime : 2025-04-07 20:58:12
 * @Description  : 密钥管理模块
 */

#include "cipher_km.h"
#include "cipher_otp.h"
#include "cipher_trng.h"

/**
 * @brief       : 密钥管理模块初始化
 * @author      : zhouzirui
 * @param        {CipherKm_S} *pHandle：句柄
 * @return       {*}成功返回0,失败返回-1
 */
static int cipherKm_init(CipherKm_S *pHandle)
{
    if (NULL == pHandle)
    {
        return TD_FAILURE;
    }
    /*存储是否为可信执行环境地址*/
    td_u32 offset = 0x12;
    /*是否为可信执行环境*/
    td_u8 tee_enable = 0;

    /*HASH 模块初始化*/
    CHECK_API_RETURN(ot_mpi_km_init());
    /*创建KEYSLOT句柄*/
    CHECK_API_RETURN(ot_mpi_keyslot_create(&pHandle->mpi_keyslot_handle, pHandle->stNeedParam.enKeyslot_type));
    /*创建KLAD句柄*/
    CHECK_API_RETURN(ot_mpi_klad_create(&pHandle->mpi_klad_handle));
    /*绑定KLAD句柄与KEYSLOT通道*/
    CHECK_API_RETURN(ot_mpi_klad_attach(pHandle->mpi_klad_handle, pHandle->stNeedParam.enKlad_type, pHandle->mpi_keyslot_handle));
    
    km_klad_attr stKladAttr;
    memset(&stKladAttr, 0, sizeof(km_klad_attr));
    stKladAttr.klad_cfg.rootkey_type = pHandle->stNeedParam.u32RootKey_type;
    stKladAttr.key_cfg.engine = pHandle->stNeedParam.enEngine;
    stKladAttr.key_cfg.decrypt_support = pHandle->stNeedParam.bDecrypt_support;
    stKladAttr.key_cfg.encrypt_support = pHandle->stNeedParam.bEncrypt_support;
    stKladAttr.key_sec_cfg;
    if (tee_enable == 0x42) {
        stKladAttr.key_sec_cfg.key_sec = KM_KLAD_SEC_ENABLE;
        stKladAttr.key_sec_cfg.master_only_enable = TD_TRUE;
        stKladAttr.key_sec_cfg.dest_buf_sec_support = TD_TRUE;
        stKladAttr.key_sec_cfg.src_buf_sec_support = TD_TRUE;
        stKladAttr.key_sec_cfg.src_buf_non_sec_support = TD_FALSE;
        stKladAttr.key_sec_cfg.dest_buf_non_sec_support = TD_FALSE;
    } else {
        stKladAttr.key_sec_cfg.key_sec = KM_KLAD_SEC_DISABLE;
        stKladAttr.key_sec_cfg.master_only_enable = TD_FALSE;
        stKladAttr.key_sec_cfg.dest_buf_sec_support = TD_FALSE;
        stKladAttr.key_sec_cfg.dest_buf_non_sec_support = TD_TRUE;
        stKladAttr.key_sec_cfg.src_buf_sec_support = TD_FALSE;
        stKladAttr.key_sec_cfg.src_buf_non_sec_support = TD_TRUE;
    }
    // stKladAttr.rkp_sw_cfg
    /*设置KLAD句柄属性*/
    CHECK_API_RETURN(ot_mpi_klad_set_attr(pHandle->mpi_klad_handle, &stKladAttr));

    return TD_SUCCESS;
}

/**
 * @brief       : 密钥管理模块去初始化
 * @author      : zhouzirui
 * @param        {CipherKm_S} *pHandle：句柄
 * @return       {*}成功返回0,失败返回-1
 */
static int cipherKm_uninit(CipherKm_S *pHandle)
{
    if (NULL == pHandle)
    {
        return TD_FAILURE;
    }
    /*解绑KLAD句柄与KEYSLOT通道*/
    CHECK_API_RETURN(ot_mpi_klad_detach(pHandle->mpi_klad_handle, pHandle->stNeedParam.enKlad_type, pHandle->mpi_keyslot_handle));
    /*销毁KLAD句柄*/
    CHECK_API_RETURN(ot_mpi_klad_destroy(pHandle->mpi_klad_handle));
    /*销毁KEYSLOT句柄*/
    CHECK_API_RETURN(ot_mpi_keyslot_destroy(pHandle->mpi_keyslot_handle));
    /*HASH 模块去初始化*/
    CHECK_API_RETURN(ot_mpi_km_deinit());

    return TD_SUCCESS;
}

/**
 * @brief       : 根密钥传递，生成工作密钥，直接存储在密钥槽中
 * @author      : zhouzirui
 * @param        {CipherKm_S} *pHandle：句柄
 * @return       {*}成功返回0,失败返回-1
 * @note        : 不支持二级KLAD派生，只支持硬件派生密钥
 */
static int cipherKm_rootKey_delivery(CipherKm_S *pHandle)
{
    if (NULL == pHandle)
    {
        return TD_FAILURE;
    }
    km_klad_effective_key stKeyAttr;
    memset(&stKeyAttr, 0, sizeof(km_klad_effective_key));
    stKeyAttr.kdf_hard_alg = pHandle->stNeedParam.enKdf_hard_alg;
    stKeyAttr.key_parity = pHandle->stNeedParam.bKey_parity;
    stKeyAttr.key_size = pHandle->stNeedParam.enKey_size;
    td_u8 aSalt[MAX_SALT_LEN];
    CHECK_API_RETURN(cipherTrng_getMultiRandom(sizeof(aSalt), aSalt));
    stKeyAttr.salt = aSalt;
    stKeyAttr.salt_length = sizeof(aSalt);
    stKeyAttr.oneway = pHandle->stNeedParam.bOneway;
    /*设置硬件派生密钥*/
    CHECK_API_RETURN(ot_mpi_klad_set_effective_key(pHandle->mpi_klad_handle, &stKeyAttr));

    return TD_SUCCESS;
}

CipherKm_S *cipherKm_alloc(CipherKmNeedParam_S stNeedParam)
{
    CipherKm_S *pHandle = (CipherKm_S *)malloc(sizeof(CipherKm_S));
    memset(pHandle, 0, sizeof(CipherKm_S));

    //info /**********************必需参数***************************/
    pHandle->stNeedParam.enKeyslot_type         = stNeedParam.enKeyslot_type;
    pHandle->stNeedParam.enKlad_type            = stNeedParam.enKlad_type;
    pHandle->stNeedParam.u32RootKey_type        = stNeedParam.u32RootKey_type;
    pHandle->stNeedParam.enEngine               = stNeedParam.enEngine;
    pHandle->stNeedParam.bDecrypt_support       = stNeedParam.bDecrypt_support;
    pHandle->stNeedParam.bEncrypt_support       = stNeedParam.bEncrypt_support;
    pHandle->stNeedParam.enKdf_hard_alg         = stNeedParam.enKdf_hard_alg;
    pHandle->stNeedParam.bKey_parity            = stNeedParam.bKey_parity;
    pHandle->stNeedParam.enKey_size             = stNeedParam.enKey_size;
    pHandle->stNeedParam.bOneway                = stNeedParam.bOneway;
    
    //info /**********************功能参数***************************/

    //info /**********************函数列表***************************/
    pHandle->cipherKm_init                  = cipherKm_init;
    pHandle->cipherKm_uninit                = cipherKm_uninit;
    pHandle->cipherKm_rootKey_delivery      = cipherKm_rootKey_delivery;

    return pHandle;
}

void cipherKm_release(CipherKm_S *pHandle)
{
    if (pHandle)
    {
        free(pHandle);
        pHandle = NULL;
    }
}

