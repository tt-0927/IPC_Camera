/**
 * @FilePath     : cipher_symc.c
 * @Author       : zhouzirui
 * @Date         : 2025-04-03 09:55:14
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2025-08-27 19:55:16
 * @Description  : 对称加解密算法模块
 */

#include "cipher_symc.h"
#include "securec.h"

/**
 * @brief       : 安全协议加速器对称加解密算法模块初始化
 * @author      : zhouzirui
 * @param        {CipherSymc_S} *pHandle：句柄
 * @return       {*}成功返回0,失败返回-1
 * @note        : 
 */
static int cipherSymc_init(CipherSymc_S *pHandle)
{
    if (NULL == pHandle)
    {
        return TD_FAILURE;
    }
    /*SYMC 模块初始化*/
    CHECK_API_RETURN(ot_mpi_cipher_symc_init());

    return TD_SUCCESS;
}

/**
 * @brief       : 安全协议加速器对称加解密算法模块去初始化
 * @author      : zhouzirui
 * @param        {CipherSymc_S} *pHandle：句柄
 * @return       {*}成功返回0,失败返回-1
 * @note        : 
 */
static int cipherSymc_uninit(CipherSymc_S *pHandle)
{
    if (NULL == pHandle)
    {
        return TD_FAILURE;
    }
    /*SYMC 模块去初始化*/
    CHECK_API_RETURN(ot_mpi_cipher_symc_deinit());

    return TD_SUCCESS;
}

/**
 * @brief       :  对称加密
 * @author      : zhouzirui
 * @param        {CipherSymc_S} *pHandle：句柄
 * @param        {crypto_symc_attr} stSymcAttr：SYMC 句柄属性
 * @param        {td_handle} keyslotHandle：KEYSLOT句柄
 * @param        {crypto_symc_ctrl_t} stSymcCtrl：SYMC 算法参数
 * @param        {crypto_buf_attr} *pSrcBuf：源数据buf指针
 * @param        {crypto_buf_attr} *pDstBuf：目的数据buf指针
 * @param        {td_u32} u32Length：数据长度
 * @param        {MpiBuf_S} *pDstTagBuf：目的数据tag buf指针
 * @return       {*}成功返回0,失败返回-1
 * @note        : src_buf和dst_buf 仅支持 phys_addr 内存类型
 */
static int cipherSymc_encryption(CipherSymc_S *pHandle, crypto_symc_attr stSymcAttr, td_handle keyslotHandle, crypto_symc_ctrl_t stSymcCtrl, crypto_buf_attr *pSrcBuf, crypto_buf_attr *pDstBuf, td_u32 u32Length, MpiBuf_S *pDstTagBuf)
{
    if (NULL == pHandle || NULL == pSrcBuf || NULL == pDstBuf)
    {
        return TD_FAILURE;
    }
    if (stSymcAttr.work_mode == CRYPTO_SYMC_WORK_MODE_GCM || stSymcAttr.work_mode == CRYPTO_SYMC_WORK_MODE_CCM)
        if (NULL == pDstTagBuf)
            return TD_FAILURE;

    td_handle symcHandle;
    // crypto_symc_attr stSymcAttr;
    /*SYMC 模块创建通道*/
    CHECK_API_RETURN(ot_mpi_cipher_symc_create(&symcHandle, &stSymcAttr));
    /*SYMC 模块绑定 KEYSLOT 句柄*/
    CHECK_API_RETURN(ot_mpi_cipher_symc_attach(symcHandle, keyslotHandle));
    // crypto_symc_ctrl_t stSymcCtrl;

    /*SYMC 模块设置算法参数*/
    CHECK_API_RETURN(ot_mpi_cipher_symc_set_config(symcHandle, &stSymcCtrl));
    /*SYMC 模块加密*/
    CHECK_API_RETURN(ot_mpi_cipher_symc_encrypt(symcHandle, pSrcBuf, pDstBuf, u32Length));

    /*工作模式为GCM、CCM时，需要获取tag值*/
    if (stSymcAttr.work_mode == CRYPTO_SYMC_WORK_MODE_GCM || stSymcAttr.work_mode == CRYPTO_SYMC_WORK_MODE_CCM)
    {
        /*SYMC 模块获取 Tag 值*/
        CHECK_API_RETURN(ot_mpi_cipher_symc_get_tag(symcHandle, pDstTagBuf->pData, pDstTagBuf->u32Len));
    }

    /*SYMC 模块销毁通道*/
    CHECK_API_RETURN(ot_mpi_cipher_symc_destroy(symcHandle));

    return TD_SUCCESS;
}

/**
 * @brief       :  对称解密
 * @author      : zhouzirui
 * @param        {CipherSymc_S} *pHandle：句柄
 * @param        {crypto_symc_attr} stSymcAttr：SYMC 句柄属性
 * @param        {td_handle} keyslotHandle：KEYSLOT句柄
 * @param        {crypto_symc_ctrl_t} stSymcCtrl：SYMC 算法参数
 * @param        {crypto_buf_attr} *pSrcBuf：源数据buf指针
 * @param        {crypto_buf_attr} *pDstBuf：目的数据buf指针
 * @param        {td_u32} u32Length：数据长度
 * @param        {MpiBuf_S} *pDstTagBuf：目的数据tag buf指针
 * @return       {*}成功返回0,失败返回-1
 * @note        : src_buf和dst_buf 仅支持 phys_addr 内存类型
 */
static int cipherSymc_decryption(CipherSymc_S *pHandle, crypto_symc_attr stSymcAttr, td_handle keyslotHandle, crypto_symc_ctrl_t stSymcCtrl, crypto_buf_attr *pSrcBuf, crypto_buf_attr *pDstBuf, td_u32 u32Length, MpiBuf_S *pDstTagBuf)
{
    if (NULL == pHandle || NULL == pSrcBuf || NULL == pDstBuf)
    {
        return TD_FAILURE;
    }
    if (stSymcAttr.work_mode == CRYPTO_SYMC_WORK_MODE_GCM || stSymcAttr.work_mode == CRYPTO_SYMC_WORK_MODE_CCM)
        if (NULL == pDstTagBuf)
            return TD_FAILURE;

    td_handle symcHandle;
    // crypto_symc_attr stSymcAttr;
    /*SYMC 模块创建通道*/
    CHECK_API_RETURN(ot_mpi_cipher_symc_create(&symcHandle, &stSymcAttr));
    /*SYMC 模块绑定 KEYSLOT 句柄*/
    CHECK_API_RETURN(ot_mpi_cipher_symc_attach(symcHandle, keyslotHandle));
    // crypto_symc_ctrl_t stSymcCtrl;
    /*SYMC 模块设置算法参数*/
    CHECK_API_RETURN(ot_mpi_cipher_symc_set_config(symcHandle, &stSymcCtrl));
    /*SYMC 模块加密*/
    CHECK_API_RETURN(ot_mpi_cipher_symc_decrypt(symcHandle, pSrcBuf, pDstBuf, u32Length));
    /*工作模式为GCM、CCM时，需要获取tag值*/
    if (stSymcAttr.work_mode == CRYPTO_SYMC_WORK_MODE_GCM || stSymcAttr.work_mode == CRYPTO_SYMC_WORK_MODE_CCM)
    {
        /*SYMC 模块获取 Tag 值*/
        CHECK_API_RETURN(ot_mpi_cipher_symc_get_tag(symcHandle, pDstTagBuf->pData, pDstTagBuf->u32Len));
    }
    /*SYMC 模块销毁通道*/
    CHECK_API_RETURN(ot_mpi_cipher_symc_destroy(symcHandle));

    return TD_SUCCESS;
}

/**
 * @brief       : MAC计算
 * @author      : zhouzirui
 * @param        {CipherSymc_S} *pHandle：句柄
 * @param        {crypto_symc_mac_attr} stMacAttr
 * @param        {crypto_buf_attr} *pSrcBuf：源数据buf指针
 * @param        {td_u32} u32Length：源数据长度
 * @param        {MpiBuf_S} *pDstBuf：目的数据buf指针
 * @return       {*}成功返回0,失败返回-1
 * @note        : length 要求为16字节的整数倍
 */
static int cipherSymc_mac(CipherSymc_S *pHandle, crypto_symc_mac_attr stMacAttr, crypto_buf_attr *pSrcBuf, td_u32 u32Length, MpiBuf_S *pDstBuf)
{
    if (NULL == pHandle || NULL == pSrcBuf || NULL == pDstBuf || u32Length == 0 || u32Length > 0xFFFF0000)
    {
        return TD_FAILURE;
    }

    /*MAC 句柄*/
    td_handle symcHandle;
    /*SYMC 模块创建 MAC 句柄*/
    CHECK_API_RETURN(ot_mpi_cipher_mac_start(&symcHandle, &stMacAttr));
    /*对传入 SYMC 模块的数据进行 MAC 计算*/
    CHECK_API_RETURN(ot_mpi_cipher_mac_update(symcHandle, pSrcBuf, MPI_ALIGN_UP(u32Length, 16)));
    /*获取 SYMC 模块数据的 MAC 计算结果*/
    CHECK_API_RETURN(ot_mpi_cipher_mac_finish(symcHandle, pDstBuf->pData, &pDstBuf->u32Len));

    return TD_SUCCESS;
}

CipherSymc_S *cipherSymc_alloc(CipherSymcNeedParam_S stNeedParam)
{
    CipherSymc_S *pHandle = (CipherSymc_S *)malloc(sizeof(CipherSymc_S));
    memset(pHandle, 0, sizeof(CipherSymc_S));

    //info /**********************必需参数***************************/
    // pHandle->stNeedParam.enHashType     = stNeedParam.enHashType;
    
    //info /**********************功能参数***************************/

    //info /**********************函数列表***************************/
    pHandle->cipherSymc_init                = cipherSymc_init;
    pHandle->cipherSymc_uninit              = cipherSymc_uninit;
    pHandle->cipherSymc_encryption          = cipherSymc_encryption;
    pHandle->cipherSymc_decryption          = cipherSymc_decryption;
    pHandle->cipherSymc_mac                 = cipherSymc_mac;

    return pHandle;
}

void cipherSymc_release(CipherSymc_S *pHandle)
{
    if (pHandle)
    {
        free(pHandle);
        pHandle = NULL;
    }
}

