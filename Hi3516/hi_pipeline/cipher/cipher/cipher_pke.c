/**
 * @FilePath     : cipher_pke.c
 * @Author       : zhouzirui
 * @Date         : 2025-04-03 09:55:09
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2025-08-27 19:55:03
 * @Description  : 非对称加解密算法模块
 */

#include "cipher_pke.h"
#include "securec.h"

/**
 * @brief       : 安全协议加速器非对称加解密算法模块初始化
 * @author      : zhouzirui
 * @param        {CipherPke_S} *pHandle：句柄
 * @return       {*}成功返回0,失败返回-1
 * @note        : 
 */
static int cipherPke_init(CipherPke_S *pHandle)
{
    if (NULL == pHandle)
    {
        return TD_FAILURE;
    }
    /*pke 模块初始化*/
    CHECK_API_RETURN(ot_mpi_cipher_pke_init());

    return TD_SUCCESS;
}

/**
 * @brief       : 安全协议加速器非对称加解密算法模块去初始化
 * @author      : zhouzirui
 * @param        {CipherPke_S} *pHandle：句柄
 * @return       {*}成功返回0,失败返回-1
 * @note        :
 */
static int cipherPke_uninit(CipherPke_S *pHandle)
{
    if (NULL == pHandle)
    {
        return TD_FAILURE;
    }
    /*pke 模块去初始化*/
    CHECK_API_RETURN(ot_mpi_cipher_pke_deinit());

    return TD_SUCCESS;
}

/**
 * @brief       : RSA 公钥加密
 * @author      : zhouzirui
 * @param        {CipherPke_S} *pHandle：：句柄
 * @param        {drv_pke_rsa_scheme} enScheme：RSA公钥加密模式，支持PKCSv15、PKCSv21
 * @param        {drv_pke_hash_type} enHashType：HASH算法类型
 * @param        {drv_pke_rsa_pub_key} *pPubKey：公钥 （e, n）
 * @param        {drv_pke_data} *pInput：待加密的消息
 * @param        {drv_pke_data} *pLabel：OAEP Padding方式中的buff er标识选项
 * @param        {drv_pke_data} *pOutput：加密完成的密文
 * @return       {*}成功返回0,失败返回-1
 * @note        : 
 */
static int cipherPke_rsa_encryption(CipherPke_S *pHandle, drv_pke_rsa_scheme enScheme, drv_pke_hash_type enHashType, const drv_pke_rsa_pub_key *pPubKey, const drv_pke_data *pInput, const drv_pke_data *pLabel, drv_pke_data *pOutput)
{
    if (NULL == pHandle || NULL == pPubKey || NULL == pInput || NULL == pLabel || NULL == pOutput)
    {
        return TD_FAILURE;
    }

    /*RSA公钥加密，与RSA私钥解密相对应*/
    CHECK_API_RETURN(ot_mpi_cipher_pke_rsa_public_encrypt(enScheme, enHashType, pPubKey, pInput, pLabel, pOutput));

    return TD_SUCCESS;
}

/**
 * @brief       : RSA 私钥解密
 * @author      : zhouzirui
 * @param        {CipherPke_S} *pHandle：句柄
 * @param        {drv_pke_rsa_scheme} enScheme：RSA私钥解密模式，支持PKCSv15、PKCSv21
 * @param        {drv_pke_hash_type} enHashType：HASH算法类型
 * @param        {drv_pke_rsa_priv_key} *pPrivKey：私钥 （d, n）
 * @param        {drv_pke_data} *pInput：待解密的密文
 * @param        {drv_pke_data} *pLabel：OAEP Padding方式中的buff er标识选项
 * @param        {drv_pke_data} *pOutput：解密完成的消息
 * @return       {*}成功返回0,失败返回-1
 * @note        : 
 */
static int cipherPke_rsa_decryption(CipherPke_S *pHandle, drv_pke_rsa_scheme enScheme, drv_pke_hash_type enHashType, const drv_pke_rsa_priv_key *pPrivKey, const drv_pke_data *pInput, const drv_pke_data *pLabel, drv_pke_data *pOutput)
{
    if (NULL == pHandle || NULL == pPrivKey || NULL == pInput || NULL == pLabel || NULL == pOutput)
    {
        return TD_FAILURE;
    }

    /*RSA私钥解密，与RSA公钥加密相对应*/
    CHECK_API_RETURN(ot_mpi_cipher_pke_rsa_private_decrypt(enScheme, enHashType, pPrivKey, pInput, pLabel, pOutput));

    return TD_SUCCESS;
}

/**
 * @brief       : RSA 签名
 * @author      : zhouzirui
 * @param        {CipherPke_S} *pHandle：句柄
 * @param        {drv_pke_rsa_priv_key} *pPrivKey：RSA私钥
 * @param        {drv_pke_rsa_scheme} enScheme：RSA签名模式，支持PKCSv15、PKCSv21
 * @param        {drv_pke_hash_type} enHashType：HASH算法类型
 * @param        {drv_pke_data} *pInputHash：待签名的HASH值
 * @param        {drv_pke_data} *pSign：HASH签名结果
 * @return       {*}成功返回0,失败返回-1
 * @note        : 
 */
static int cipherPke_rsa_sign(CipherPke_S *pHandle, const drv_pke_rsa_priv_key *pPrivKey, drv_pke_rsa_scheme enScheme, drv_pke_hash_type enHashType, const drv_pke_data *pInputHash, drv_pke_data *pSign)
{
    if (NULL == pHandle || NULL == pPrivKey || NULL == pInputHash || NULL == pSign)
    {
        return TD_FAILURE;
    }

    /*RSA签名，签名对象必须为HASH值*/
    CHECK_API_RETURN(ot_mpi_cipher_pke_rsa_sign(pPrivKey, enScheme, enHashType, pInputHash, pSign));

    return TD_SUCCESS;
}

/**
 * @brief       : RSA 验签
 * @author      : zhouzirui
 * @param        {CipherPke_S} *pHandle：句柄
 * @param        {drv_pke_rsa_pub_key} *pPubKey：RSA公钥
 * @param        {drv_pke_rsa_scheme} enScheme：RSA签名模式，支持PKCSv15、PKCSv21
 * @param        {drv_pke_hash_type} enHashType：HASH算法类型
 * @param        {drv_pke_data} *pInputHash：已签名的HASH值
 * @param        {drv_pke_data} *pSign：HASH签名结果
 * @return       {*}成功返回0,失败返回-1
 * @note        : 
 */
static int cipherPke_rsa_verify(CipherPke_S *pHandle, const drv_pke_rsa_pub_key *pPubKey, drv_pke_rsa_scheme enScheme, drv_pke_hash_type enHashType, const drv_pke_data *pInputHash, drv_pke_data *pSign)
{
    if (NULL == pHandle || NULL == pPubKey || NULL == pInputHash || NULL == pSign)
    {
        return TD_FAILURE;
    }

    /*RSA验签，与RSA签名相对应*/
    // CHECK_API_RETURN(ot_mpi_cipher_pke_rsa_verify(pPubKey, enScheme, enHashType, pInputHash, pSign));

    return TD_SUCCESS;
}

/**
 * @brief       : SM2公钥加密
 * @author      : zhouzirui
 * @param        {CipherPke_S} *pHandle：句柄
 * @param        {drv_pke_ecc_point} *pPubKey：Sm2公钥
 * @param        {drv_pke_data} *pPlainText：待加密的消息
 * @param        {drv_pke_data} *pCipherText：密文
 * @return       {*}成功返回0,失败返回-1
 * @note        : 
 */
static int cipherPke_sm2_encryption(CipherPke_S *pHandle, const drv_pke_ecc_point *pPubKey, const drv_pke_data *pPlainText, drv_pke_data *pCipherText)
{
    if (NULL == pHandle || NULL == pPubKey || NULL == pPlainText || NULL == pCipherText)
    {
        return TD_FAILURE;
    }

    /*SM2公钥加密*/
    CHECK_API_RETURN(ot_mpi_cipher_pke_sm2_public_encrypt(pPubKey, pPlainText, pCipherText));

    return TD_SUCCESS;
}

/**
 * @brief       : SM2私钥解密
 * @author      : zhouzirui
 * @param        {CipherPke_S} *pHandle：句柄
 * @param        {drv_pke_data} *pPrivKey：Sm2私钥
 * @param        {drv_pke_data} *pPlainText：待解密的密文
 * @param        {drv_pke_data} *pCipherText：解密的消息
 * @return       {*}成功返回0,失败返回-1
 * @note        : 
 */
static int cipherPke_sm2_decryption(CipherPke_S *pHandle, const drv_pke_data *pPrivKey, const drv_pke_data *pPlainText, drv_pke_data *pCipherText)
{
    if (NULL == pHandle || NULL == pPrivKey || NULL == pPlainText || NULL == pCipherText)
    {
        return TD_FAILURE;
    }

    /*SM2私钥解密*/
    CHECK_API_RETURN(ot_mpi_cipher_pke_sm2_private_decrypt(pPrivKey, pPlainText, pCipherText));

    return TD_SUCCESS;
}

/**
 * @brief       : SM2 签名
 * @author      : zhouzirui
 * @param        {CipherPke_S} *pHandle：句柄
 * @param        {drv_pke_ecc_curve_type} enCurveType：ECC曲线参数
 * @param        {drv_pke_data} *pPrivKey：ECC私钥
 * @param        {drv_pke_data} *pHash：待签名的HASH值
 * @param        {drv_pke_ecc_sig} *pSig：签名结果
 * @return       {*}成功返回0,失败返回-1
 * @note        : 
 */
static int cipherPke_sm2_sign(CipherPke_S *pHandle, drv_pke_ecc_curve_type enCurveType, const drv_pke_data *pPrivKey, const drv_pke_data *pHash, const drv_pke_ecc_sig *pSig)
{
    if (NULL == pHandle || NULL == pPrivKey || NULL == pHash || NULL == pSig)
    {
        return TD_FAILURE;
    }

    /*SM2 私钥签名 ECC签名，签名对象必须为HASH值*/
    CHECK_API_RETURN(ot_mpi_cipher_pke_ecdsa_sign(enCurveType, pPrivKey, pHash, pSig));

    return TD_SUCCESS;
}

/**
 * @brief       : SM2 验签
 * @author      : zhouzirui
 * @param        {CipherPke_S} *pHandle：句柄
 * @param        {drv_pke_ecc_curve_type} enCurveType：ECC曲线参数
 * @param        {drv_pke_ecc_point} *pPubKey：ECC公钥
 * @param        {drv_pke_data} *pHash：待签名的HASH值
 * @param        {drv_pke_ecc_sig} *pSig：待验证的签名
 * @return       {*}成功返回0,失败返回-1
 * @note        : 
 */
static int cipherPke_sm2_verify(CipherPke_S *pHandle, drv_pke_ecc_curve_type enCurveType, const drv_pke_ecc_point *pPubKey, const drv_pke_data *pHash, const drv_pke_ecc_sig *pSig)
{
    if (NULL == pHandle || NULL == pPubKey || NULL == pHash || NULL == pSig)
    {
        return TD_FAILURE;
    }

    /*SM2 公钥验签 ECC签名验证，签名对象必须为HASH值*/
    CHECK_API_RETURN(ot_mpi_cipher_pke_ecdsa_verify(enCurveType, pPubKey, pHash, pSig));

    return TD_SUCCESS;
}

/**
 * @brief       : ECDSA签名
 * @author      : zhouzirui
 * @param        {CipherPke_S} *pHandle：句柄
 * @param        {drv_pke_ecc_curve_type} enCurveType：ECC曲线参数
 * @param        {drv_pke_data} *pPrivKey：ECC私钥
 * @param        {drv_pke_data} *pHash：待签名的HASH值
 * @param        {drv_pke_ecc_sig} *pSig：签名结果
 * @return       {*}成功返回0,失败返回-1
 * @note        : 
 */
static int cipherPke_ecdsa_sign(CipherPke_S *pHandle, drv_pke_ecc_curve_type enCurveType, const drv_pke_data *pPrivKey, const drv_pke_data *pHash, const drv_pke_ecc_sig *pSig)
{
    if (NULL == pHandle || NULL == pPrivKey || NULL == pHash || NULL == pSig)
    {
        return TD_FAILURE;
    }

    /*ECDSA 私钥签名 ECC签名，签名对象必须为HASH值*/
    CHECK_API_RETURN(ot_mpi_cipher_pke_ecdsa_sign(enCurveType, pPrivKey, pHash, pSig));

    return TD_SUCCESS;
}

/**
 * @brief       : ECDSA验签
 * @author      : zhouzirui
 * @param        {CipherPke_S} *pHandle：句柄
 * @param        {drv_pke_ecc_curve_type} enCurveType：ECC曲线参数
 * @param        {drv_pke_ecc_point} *pPubKey：ECC公钥
 * @param        {drv_pke_data} *pHash：待签名的HASH值
 * @param        {drv_pke_ecc_sig} *pSig：待验证的签名
 * @return       {*}成功返回0,失败返回-1
 * @note        : 
 */
static int cipherPke_ecdsa_verify(CipherPke_S *pHandle, drv_pke_ecc_curve_type enCurveType, const drv_pke_ecc_point *pPubKey, const drv_pke_data *pHash, const drv_pke_ecc_sig *pSig)
{
    if (NULL == pHandle || NULL == pPubKey || NULL == pHash || NULL == pSig)
    {
        return TD_FAILURE;
    }

    /*ECDSA 公钥验签 ECC签名验证，签名对象必须为HASH值*/
    CHECK_API_RETURN(ot_mpi_cipher_pke_ecdsa_verify(enCurveType, pPubKey, pHash, pSig));

    return TD_SUCCESS;
}

/**
 * @brief       : 使用ed25519曲线签名
 * @author      : zhouzirui
 * @param        {CipherPke_S} *pHandle：句柄
 * @param        {drv_pke_ecc_curve_type} enCurveType：ECC曲线参数
 * @param        {drv_pke_data} *pPrivKey：私钥
 * @param        {drv_pke_msg} *pMsg：待签名的消息
 * @param        {drv_pke_ecc_sig} *pSig：签名结果
 * @return       {*}成功返回0,失败返回-1
 * @note        : 
 */
static int cipherPke_eddsa_sign(CipherPke_S *pHandle, drv_pke_ecc_curve_type enCurveType, const drv_pke_data *pPrivKey, const drv_pke_msg *pMsg, const drv_pke_ecc_sig *pSig)
{
    if (NULL == pHandle || NULL == pPrivKey || NULL == pMsg || NULL == pSig)
    {
        return TD_FAILURE;
    }

    /*EDDSA 私钥签名 使用爱德华曲线进行EDDSA签名*/
    CHECK_API_RETURN(ot_mpi_cipher_pke_eddsa_sign(enCurveType, pPrivKey, pMsg, pSig));

    return TD_SUCCESS;
}

/**
 * @brief       : 使用ed25519曲线验签
 * @author      : zhouzirui
 * @param        {CipherPke_S} *pHandle：句柄
 * @param        {drv_pke_ecc_curve_type} enCurveType：ECC曲线参数
 * @param        {drv_pke_ecc_point} *pPubKey：公钥
 * @param        {drv_pke_msg} *pMsg：签名的消息
 * @param        {drv_pke_ecc_sig} *pSig：签名
 * @return       {*}成功返回0,失败返回-1
 * @note        : 
 */
static int cipherPke_eddsa_verify(CipherPke_S *pHandle, drv_pke_ecc_curve_type enCurveType, const drv_pke_ecc_point *pPubKey, const drv_pke_msg *pMsg, const drv_pke_ecc_sig *pSig)
{
    if (NULL == pHandle || NULL == pPubKey || NULL == pMsg || NULL == pSig)
    {
        return TD_FAILURE;
    }

    /*EDDSA 公钥验签 使用爱德华曲线进行eddsa签名验证*/
    CHECK_API_RETURN(ot_mpi_cipher_pke_eddsa_verify(enCurveType, pPubKey, pMsg, pSig));

    return TD_SUCCESS;
}

CipherPke_S *cipherPke_alloc(CipherPkeNeedParam_S stNeedParam)
{
    CipherPke_S *pHandle = (CipherPke_S *)malloc(sizeof(CipherPke_S));
    memset(pHandle, 0, sizeof(CipherPke_S));

    //info /**********************必需参数***************************/
        
    //info /**********************功能参数***************************/

    //info /**********************函数列表***************************/
    pHandle->cipherPke_init                 = cipherPke_init;
    pHandle->cipherPke_uninit               = cipherPke_uninit;
    pHandle->cipherPke_rsa_encryption       = cipherPke_rsa_encryption;
    pHandle->cipherPke_rsa_decryption       = cipherPke_rsa_decryption;
    pHandle->cipherPke_rsa_sign             = cipherPke_rsa_sign;
    pHandle->cipherPke_rsa_verify           = cipherPke_rsa_verify;
    pHandle->cipherPke_sm2_encryption       = cipherPke_sm2_encryption;
    pHandle->cipherPke_sm2_decryption       = cipherPke_sm2_decryption;
    pHandle->cipherPke_sm2_sign             = cipherPke_sm2_sign;
    pHandle->cipherPke_sm2_verify           = cipherPke_sm2_verify;
    pHandle->cipherPke_ecdsa_sign           = cipherPke_ecdsa_sign;
    pHandle->cipherPke_ecdsa_verify         = cipherPke_ecdsa_verify;
    pHandle->cipherPke_eddsa_sign           = cipherPke_eddsa_sign;
    pHandle->cipherPke_eddsa_verify         = cipherPke_eddsa_verify;

    return pHandle;
}

void cipherPke_release(CipherPke_S *pHandle)
{
    if (pHandle)
    {
        free(pHandle);
        pHandle = NULL;
    }
}

