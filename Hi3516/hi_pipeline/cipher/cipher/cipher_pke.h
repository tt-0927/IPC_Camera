/**
 * @FilePath     : cipher_pke.h
 * @Author       : zhouzirui
 * @Date         : 2025-04-03 09:55:06
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-05-27 15:06:45
 * @Description  : 非对称加解密算法模块
 */

#ifndef __CIPHER_PKE_H__
#define __CIPHER_PKE_H__

#ifdef __cplusplus
extern "C"
{
#endif

#include "ot_mpi_cipher.h"
#include "mpi_common.h"
#include "mpi_buf.h"

/*cipher hash必需参数*/
typedef struct _CipherPkeNeedParam_S
{
    // /*HASH 算法类型*/
    // crypto_hash_type enHashType;
    
} CipherPkeNeedParam_S;

typedef struct _CipherPke_S CipherPke_S;
struct _CipherPke_S
{
    //info /**********************必需参数***************************/
    CipherPkeNeedParam_S stNeedParam;
    //info /**********************辅助参数***************************/
    
    //info /**********************功能列表***************************/

    /*安全协议加速器非对称加解密算法模块初始化*/
    int (*cipherPke_init)(CipherPke_S *pHandle);

    /*安全协议加速器非对称加解密算法模块去初始化*/
    int (*cipherPke_uninit)(CipherPke_S *pHandle);

    /*ECC 密钥生成*/
    int (*cipherPke_ecc_gen_key)(CipherPke_S *pHandle,
                                 drv_pke_ecc_curve_type enCurveType,
                                 const drv_pke_data *pInputPrivKey,
                                 const drv_pke_data *pOutputPrivKey,
                                 const drv_pke_ecc_point *pOutputPubKey);

    /*检查公钥点是否在指定曲线上*/
    int (*cipherPke_check_dot_on_curve)(CipherPke_S *pHandle,
                                        drv_pke_ecc_curve_type enCurveType,
                                        const drv_pke_ecc_point *pPubKey,
                                        td_bool *pIsOnCurve);

    /*SM2 签名验签前的 ZA HASH 计算*/
    int (*cipherPke_sm2_dsa_hash)(CipherPke_S *pHandle,
                                  const drv_pke_data *pSm2Id,
                                  const drv_pke_ecc_point *pPubKey,
                                  const drv_pke_msg *pMsg,
                                  drv_pke_data *pHash);

    /*RSA 公钥加密*/
    int (*cipherPke_rsa_encryption)(CipherPke_S *pHandle, drv_pke_rsa_scheme enScheme, drv_pke_hash_type enHashType, const drv_pke_rsa_pub_key *pPubKey, const drv_pke_data *pInput, const drv_pke_data *pLabel, drv_pke_data *pOutput);

    /*RSA 私钥解密*/
    int (*cipherPke_rsa_decryption)(CipherPke_S *pHandle, drv_pke_rsa_scheme enScheme, drv_pke_hash_type enHashType, const drv_pke_rsa_priv_key *pPrivKey, const drv_pke_data *pInput, const drv_pke_data *pLabel, drv_pke_data *pOutput);

    /*RSA 签名*/
    int (*cipherPke_rsa_sign)(CipherPke_S *pHandle, const drv_pke_rsa_priv_key *pPrivKey, drv_pke_rsa_scheme enScheme, drv_pke_hash_type enHashType, const drv_pke_data *pInputHash, drv_pke_data *pSign);

    /*RSA 验签*/
    int (*cipherPke_rsa_verify)(CipherPke_S *pHandle, const drv_pke_rsa_pub_key *pPubKey, drv_pke_rsa_scheme enScheme, drv_pke_hash_type enHashType, const drv_pke_data *pInputHash, drv_pke_data *pSign);

    /*SM2公钥加密*/
    int (*cipherPke_sm2_encryption)(CipherPke_S *pHandle, const drv_pke_ecc_point *pPubKey, const drv_pke_data *pPlainText, drv_pke_data *pCipherText);

    /*SM2私钥解密*/
    int (*cipherPke_sm2_decryption)(CipherPke_S *pHandle, const drv_pke_data *pPrivKey, const drv_pke_data *pPlainText, drv_pke_data *pCipherText);

    /*SM2 签名*/
    int (*cipherPke_sm2_sign)(CipherPke_S *pHandle, drv_pke_ecc_curve_type enCurveType, const drv_pke_data *pPrivKey, const drv_pke_data *pHash, const drv_pke_ecc_sig *pSig);

    /*SM2 验签*/
    int (*cipherPke_sm2_verify)(CipherPke_S *pHandle, drv_pke_ecc_curve_type enCurveType, const drv_pke_ecc_point *pPubKey, const drv_pke_data *pHash, const drv_pke_ecc_sig *pSig);

    /*ECDSA签名*/
    int (*cipherPke_ecdsa_sign)(CipherPke_S *pHandle, drv_pke_ecc_curve_type enCurveType, const drv_pke_data *pPrivKey, const drv_pke_data *pHash, const drv_pke_ecc_sig *pSig);

    /*ECDSA验签*/
    int (*cipherPke_ecdsa_verify)(CipherPke_S *pHandle, drv_pke_ecc_curve_type enCurveType, const drv_pke_ecc_point *pPubKey, const drv_pke_data *pHash, const drv_pke_ecc_sig *pSig);

    /*使用ed25519曲线签名*/
    int (*cipherPke_eddsa_sign)(CipherPke_S *pHandle, drv_pke_ecc_curve_type enCurveType, const drv_pke_data *pPrivKey, const drv_pke_msg *pMsg, const drv_pke_ecc_sig *pSig);

    /*使用ed25519曲线验签*/
    int (*cipherPke_eddsa_verify)(CipherPke_S *pHandle, drv_pke_ecc_curve_type enCurveType, const drv_pke_ecc_point *pPubKey, const drv_pke_msg *pMsg, const drv_pke_ecc_sig *pSig);

};

/**
 * @brief       : 分配安全协议加速器HASH及HMAC摘要算法模块句柄
 * @author      : zhouzirui
 * @param        {CipherPkeNeedParam_S} stNeedParam：必须参数
 * @return       {*}成功返回句柄，失败返回NULL
 */
CipherPke_S *cipherPke_alloc(CipherPkeNeedParam_S stNeedParam);

/**
 * @brief       : 释放安全协议加速器HASH及HMAC摘要算法模块句柄
 * @author      : zhouzirui
 * @param        {CipherPke_S} *pHandle：句柄
 * @return       {*}
 */
void cipherPke_release(CipherPke_S *pHandle);

#ifdef __cplusplus
}
#endif
#endif // __CIPHER_PKE_H__
