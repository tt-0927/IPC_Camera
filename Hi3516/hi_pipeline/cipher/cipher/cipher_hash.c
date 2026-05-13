/**
 * @FilePath     : cipher_hash.c
 * @Author       : zhouzirui
 * @Date         : 2025-04-03 09:54:47
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2025-08-27 19:54:36
 * @Description  : HASH及HMAC摘要算法
 */

#include "cipher_hash.h"
#include "securec.h"
/**
 * @brief       : 安全协议加速器HASH及HMAC摘要算法模块初始化
 * @author      : zhouzirui
 * @param        {CipherHash_S} *pHandle：句柄
 * @return       {*}成功返回0,失败返回-1
 * @note        : 
 */
static int cipherHash_init(CipherHash_S *pHandle)
{
    if (NULL == pHandle)
    {
        return TD_FAILURE;
    }
    /*HASH 模块初始化*/
    CHECK_API_RETURN(ot_mpi_cipher_hash_init());

    return TD_SUCCESS;
}

/**
 * @brief       : 安全协议加速器HASH及HMAC摘要算法模块去初始化
 * @author      : zhouzirui
 * @param        {CipherHash_S} *pHandle：句柄
 * @return       {*}成功返回0,失败返回-1
 * @note        : 
 */
static int cipherHash_uninit(CipherHash_S *pHandle)
{
    if (NULL == pHandle)
    {
        return TD_FAILURE;
    }
    /*HASH 模块去初始化*/
    CHECK_API_RETURN(ot_mpi_cipher_hash_deinit());

    return TD_SUCCESS;
}

/**
 * @brief       : HASH 计算(获取消息摘要)
 * @author      : zhouzirui
 * @param        {CipherHash_S} *pHandle：句柄
 * @param        {crypto_hash_attr} stHashAttr：HASH 句柄属性
 * @param        {MpiBuf_S} *pSrcBuf：源数据buf
 * @param        {MpiBuf_S} *pDstBuf：目的数据buf
 * @return       {*}成功返回0,失败返回-1
 */
static int cipherHash_HashCompute(CipherHash_S *pHandle, crypto_hash_attr stHashAttr, const MpiBuf_S *pSrcBuf, MpiBuf_S *pDstBuf)
{
    if (NULL == pHandle || NULL == pSrcBuf || NULL == pDstBuf || pSrcBuf->u32Len == 0 || pSrcBuf->u32Len > 0xFFFF0000)
    {
        return TD_FAILURE;
    }
    /*HASH句柄*/
    td_handle hashHandle;
    /*算法参数*/
    // crypto_hash_attr stHashAttr = {
    //     .key = NULL,
    //     .key_len = 0,
    //     .drv_keyslot_handle =NULL,
    //     .hash_type = pHandle->stNeedParam.enHashType,
    //     .is_keyslot = TD_FALSE,
    //     .is_long_term = TD_FALSE,
    // };
    /*HASH 模块初始化*/
    CHECK_API_RETURN(ot_mpi_cipher_hash_create(&hashHandle, &stHashAttr));
    crypto_buf_attr stBufAttr;
    /*HASH update 计算*/
    // td_s32 ot_mpi_cipher_hash_update(td_handle mpi_hash_handle, const crypto_buf_attr *src_buf, const td_u32 len);
    CHECK_API_RETURN(ot_mpi_cipher_hash_update(hashHandle, &stBufAttr, pSrcBuf->u32Len));
    /*HASH计算获取摘要信息，并在计算成功的时候销毁HASH句柄*/
    CHECK_API_RETURN(ot_mpi_cipher_hash_finish(hashHandle, pDstBuf->pData, pDstBuf->u32Size, &pDstBuf->u32Len));

    return TD_SUCCESS;
}

/**
 * @brief       : HMAC 计算(计算一块数据的 HMAC值)
 * @author      : zhouzirui
 * @param        {CipherHash_S} *pHandle：句柄
 * @param        {crypto_hash_attr} stHashAttr：HASH 句柄属性
 * @param        {MpiBuf_S} *pSrcBuf：源数据buf
 * @param        {MpiBuf_S} *pDstBuf：目的数据buf
 * @return       {*}成功返回0,失败返回-1
 */
static int cipherHash_HmacCompute(CipherHash_S *pHandle, crypto_hash_attr stHashAttr, const MpiBuf_S *pSrcBuf, MpiBuf_S *pDstBuf)
{
    if (NULL == pHandle || NULL == pSrcBuf || NULL == pDstBuf || pSrcBuf->u32Len == 0 || pSrcBuf->u32Len > 0xFFFF0000)
    {
        return TD_FAILURE;
    }
    /*HASH句柄*/
    td_handle hashHandle;
    /*HASH 模块初始化 创建一路HASH，配置当前HMAC算法及密钥并获取HASH句柄*/
    CHECK_API_RETURN(ot_mpi_cipher_hash_create(&hashHandle, &stHashAttr));
    /*HMAC update 计算 传入消息数据，对消息进行摘要计算*/
    // CHECK_API_RETURN(ot_mpi_cipher_hash_update(hashHandle, pSrcBuf->pData, pSrcBuf->u32Len));
    // /*HMAC计算获取摘要信息，并在计算成功的时候销毁HASH句柄*/
    // CHECK_API_RETURN(ot_mpi_cipher_hash_finish(hashHandle, pDstBuf->pData, pDstBuf->u32Size, pDstBuf->u32Len));

    return TD_SUCCESS;
}

/**
 * @brief       :  HASH Clone 计算
 * @author      : zhouzirui
 * @param        {CipherHash_S} *pHandle：句柄
 * @param        {crypto_hash_attr} stHashAttr：HASH 句柄属性
 * @param        {MpiBuf_S} *pSrcBuf1：源数据buf1
 * @param        {MpiBuf_S} *pSrcBuf2：源数据buf2
 * @param        {MpiBuf_S} *pDstBuf：目的数据buf
 * @return       {*}成功返回0,失败返回-1
 * @note        : 计算HASH(Message1 || Mesage2),其中Message1的处理由hash_handle1完成，Message2的处理由hash_handle2完成，
 * 最终由hash_handle2计算出HASH(Message1 || Message2)的结果）
 */
static int cipherHash_HashCloneCompute(CipherHash_S *pHandle, crypto_hash_attr stHashAttr, const MpiBuf_S *pSrcBuf1, const MpiBuf_S *pSrcBuf2, MpiBuf_S *pDstBuf)
{
    if (NULL == pHandle || NULL == pSrcBuf1 || NULL == pSrcBuf2 || NULL == pDstBuf || pSrcBuf1->u32Len == 0 || pSrcBuf1->u32Len > 0xFFFF0000 || pSrcBuf2->u32Len == 0 || pSrcBuf2->u32Len > 0xFFFF0000)
    {
        return TD_FAILURE;
    }
    /*HASH句柄 1*/
    td_handle hashHandle1;
    /*HASH句柄 2*/
    td_handle hashHandle2;
    /*算法参数*/
    // crypto_hash_attr stHashAttr = {
    //     .key = NULL,
    //     .key_len = 0,
    //     .drv_keyslot_handle =NULL,
    //     .hash_type = pHandle->stNeedParam.enHashType,
    //     .is_keyslot = TD_FALSE,
    //     .is_long_term = TD_FALSE,
    // };
    /*HASH 模块初始化 创建一路HASH，配置当前HMAC算法及密钥并获取HASH句柄*/
    CHECK_API_RETURN(ot_mpi_cipher_hash_create(&hashHandle1, &stHashAttr));
    /*HASH 模块初始化 创建第二路HASH，配置当前HMAC算法及密钥并获取HASH句柄*/
    CHECK_API_RETURN(ot_mpi_cipher_hash_create(&hashHandle2, &stHashAttr));

    /*HASH update 计算 传入消息数据，对消息进行摘要计算*/
    // CHECK_API_RETURN(ot_mpi_cipher_hash_update(hashHandle1, pSrcBuf1->pData, pSrcBuf1->u32Len));
    crypto_hash_clone_ctx stHashCloneCtx;
    /*获取 HASH句柄 1的计算中间结果*/
    CHECK_API_RETURN(ot_mpi_cipher_hash_get(hashHandle1, &stHashCloneCtx));
    /*销毁HASH1句柄*/
    CHECK_API_RETURN(ot_mpi_cipher_hash_destroy(hashHandle1));
    /*设置HASH1句柄的中间结果到HASH2句柄*/
    CHECK_API_RETURN(ot_mpi_cipher_hash_set(hashHandle1, &stHashCloneCtx));
    /*HASH update 计算 传入消息数据，对消息进行摘要计算*/
    // CHECK_API_RETURN(ot_mpi_cipher_hash_update(hashHandle2, pSrcBuf2->pData, pSrcBuf2->u32Len));
    /*HASH计算获取摘要信息，并在计算成功的时候销毁HASH句柄*/
    // CHECK_API_RETURN(ot_mpi_cipher_hash_finish(hashHandle2, pDstBuf->pData, pDstBuf->u32Size, pDstBuf->u32Len));

    return TD_SUCCESS;
}

/**
 * @brief       :  HMAC Clone 计算
 * @author      : zhouzirui
 * @param        {CipherHash_S} *pHandle：句柄
 * @param        {crypto_hash_attr} stHashAttr：HASH 句柄属性
 * @param        {MpiBuf_S} *pSrcBuf1：源数据buf1
 * @param        {MpiBuf_S} *pSrcBuf2：源数据buf2
 * @param        {MpiBuf_S} *pDstBuf：目的数据buf
 * @return       {*}成功返回0,失败返回-1
 * @note        : HMAC在使用硬件算法时，不支持Clone计算,HMAC在使用明文key软件算法时，支持Clone计算
 */
static int cipherHash_HmacCloneCompute(CipherHash_S *pHandle, crypto_hash_attr stHashAttr, const MpiBuf_S *pSrcBuf1, const MpiBuf_S *pSrcBuf2, MpiBuf_S *pDstBuf)
{
    if (NULL == pHandle || NULL == pSrcBuf1 || NULL == pSrcBuf2 || NULL == pDstBuf || pSrcBuf1->u32Len == 0 || pSrcBuf1->u32Len > 0xFFFF0000 || pSrcBuf2->u32Len == 0 || pSrcBuf2->u32Len > 0xFFFF0000)
    {
        return TD_FAILURE;
    }
    /*HASH句柄 1*/
    td_handle hashHandle1;
    /*HASH句柄 2*/
    td_handle hashHandle2;
    /*算法参数*/
    // crypto_hash_attr stHashAttr = {
    //     .key = NULL,
    //     .key_len = 0,
    //     .drv_keyslot_handle =NULL,
    //     .hash_type = pHandle->stNeedParam.enHashType,
    //     .is_keyslot = TD_FALSE,
    //     .is_long_term = TD_FALSE,
    // };
    /*HASH 模块初始化 创建一路HASH，配置当前HMAC算法及密钥并获取HASH句柄*/
    CHECK_API_RETURN(ot_mpi_cipher_hash_create(&hashHandle1, &stHashAttr));
    /*HASH 模块初始化 创建第二路HASH，配置当前HMAC算法及密钥并获取HASH句柄*/
    CHECK_API_RETURN(ot_mpi_cipher_hash_create(&hashHandle2, &stHashAttr));

    /*HMAC update 计算 传入消息数据，对消息进行摘要计算*/
    // CHECK_API_RETURN(ot_mpi_cipher_hash_update(hashHandle1, pSrcBuf1->pData, pSrcBuf1->u32Len));
    crypto_hash_clone_ctx stHashCloneCtx;
    /*获取 HASH句柄 1的计算中间结果*/
    CHECK_API_RETURN(ot_mpi_cipher_hash_get(hashHandle1, &stHashCloneCtx));
    /*销毁HASH1句柄*/
    CHECK_API_RETURN(ot_mpi_cipher_hash_destroy(hashHandle1));
    /*设置HASH1句柄的中间结果到HASH2句柄*/
    CHECK_API_RETURN(ot_mpi_cipher_hash_set(hashHandle1, &stHashCloneCtx));
    /*HMAC update 计算 传入消息数据，对消息进行摘要计算*/
    // CHECK_API_RETURN(ot_mpi_cipher_hash_update(hashHandle2, pSrcBuf2->pData, pSrcBuf2->u32Len));
    /*HMAC计算获取摘要信息，并在计算成功的时候销毁HASH句柄*/
    // CHECK_API_RETURN(ot_mpi_cipher_hash_finish(hashHandle2, pDstBuf->pData, pDstBuf->u32Size, pDstBuf->u32Len));

    return TD_SUCCESS;
}

/**
 * @brief       : PBKDF2 计算
 * @author      : zhouzirui
 * @param        {CipherHash_S} *pHandle
 * @param        {crypto_kdf_pbkdf2_param} stParam
 * @param        {MpiBuf_S} *pDstBuf
 * @return       {*}成功返回0,失败返回-1
 * @note        : 此处stParam的hash_type成员支持的HASH算法类型仅为HMAC算法
 */
static int cipherHash_pbkdf2Compute(CipherHash_S *pHandle, crypto_kdf_pbkdf2_param stParam, MpiBuf_S *pDstBuf)
{
    if (NULL == pHandle || NULL == pDstBuf)
    {
        return TD_FAILURE;
    }

    CHECK_API_RETURN(ot_mpi_cipher_pbkdf2(&stParam, pDstBuf->pData, pDstBuf->u32Len));

    return TD_SUCCESS;
}

CipherHash_S *cipherHash_alloc(CipherHashNeedParam_S stNeedParam)
{
    CipherHash_S *pHandle = (CipherHash_S *)malloc(sizeof(CipherHash_S));
    memset_s(pHandle, sizeof(CipherHash_S), 0, sizeof(CipherHash_S));

    //info /**********************必需参数***************************/
    pHandle->stNeedParam.enHashType     = stNeedParam.enHashType;
    
    //info /**********************功能参数***************************/

    //info /**********************函数列表***************************/
    pHandle->cipherHash_init                = cipherHash_init;
    pHandle->cipherHash_uninit              = cipherHash_uninit;
    pHandle->cipherHash_HashCompute         = cipherHash_HashCompute;
    pHandle->cipherHash_HmacCompute         = cipherHash_HmacCompute;
    pHandle->cipherHash_HashCloneCompute    = cipherHash_HashCloneCompute;
    pHandle->cipherHash_HmacCloneCompute    = cipherHash_HmacCloneCompute;
    pHandle->cipherHash_pbkdf2Compute       = cipherHash_pbkdf2Compute;

    return pHandle;
}

void cipherHash_release(CipherHash_S *pHandle)
{
    if (pHandle)
    {
        free(pHandle);
        pHandle = NULL;
    }
}

