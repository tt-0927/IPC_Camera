/**
 * @FilePath     : cipher_context.c
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2026-05-27 08:57:21
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-05-27 15:12:27
 * @Description  : 安全子系统统一上下文封装实现
 */

#include "cipher_context.h"
#include "cipher_km.h"
#include "cipher_symc.h"
#include "securec.h"
#include "ss_mpi_sys_mem.h"

#include <stdlib.h>

/* SM3 摘要固定输出长度 */
#define CIPHER_CONTEXT_SM3_DIGEST_LEN 32
/* SM4-CBC raw block 固定块大小，同时也是 SM4-128 key/iv 长度 */
#define CIPHER_CONTEXT_SM4_BLOCK_LEN 16
/* SM2 私钥、公钥坐标、签名 r/s 固定长度 */
#define CIPHER_CONTEXT_SM2_KEY_LEN 32
/* 海思 SM2 raw 密文固定附加长度：0x04 + X + Y + C3 */
#define CIPHER_CONTEXT_SM2_CIPHER_ADD_LEN 97
/* 空输入 SM3 计算使用的只读占位缓冲，避免向底层 update 传入空指针 */
static const td_u8 CIPHER_CONTEXT_EMPTY_INPUT = 0;

typedef struct _CipherContextMmzBuf_S
{
    /* MMZ 物理地址，SYMC 硬件只接受物理连续内存 */
    td_phys_addr_t phys_addr;
    /* MMZ 用户态虚拟地址，用于填充输入和读取输出 */
    td_u8 *virt_addr;
    /* MMZ 缓冲区大小 */
    td_u32 size;
} CipherContextMmzBuf_S;

/**
 * @brief   : 初始化安全子系统上下文中声明需要的硬件子模块
 * @param    {CipherContext_S*} pHandle：安全子系统上下文句柄
 * @return   {int} TD_SUCCESS：成功，非 TD_SUCCESS：失败
 */
static int cipherContext_init(CipherContext_S *pHandle);
/**
 * @brief   : 按反向顺序去初始化已启用的硬件子模块
 * @param    {CipherContext_S*} pHandle：安全子系统上下文句柄
 * @return   {int} TD_SUCCESS：成功，非 TD_SUCCESS：失败
 */
static int cipherContext_uninit(CipherContext_S *pHandle);
/**
 * @brief   : 使用硬件 HASH 子模块执行一次性 SM3 摘要计算
 * @param    {CipherContext_S*} pHandle：安全子系统上下文句柄
 * @param    {td_u8*} pData：输入数据指针，长度为 0 时允许为空
 * @param    {td_u32} u32Len：输入数据长度
 * @param    {td_u8*} pOut：输出摘要缓冲区
 * @param    {td_u32} u32OutSize：输出摘要缓冲区大小
 * @param    {td_u32*} pOutLen：实际输出摘要长度
 * @return   {int} TD_SUCCESS：成功，非 TD_SUCCESS：失败
 */
static int cipherContext_sm3_compute(CipherContext_S *pHandle,
                                     const td_u8 *pData,
                                     td_u32 u32Len,
                                     td_u8 *pOut,
                                     td_u32 u32OutSize,
                                     td_u32 *pOutLen);
/**
 * @brief   : 使用硬件 TRNG 获取随机字节
 * @param    {CipherContext_S*} pHandle：安全子系统上下文句柄
 * @param    {td_u8*} pOut：输出随机字节缓冲区
 * @param    {td_u32} u32Len：请求随机字节长度
 * @return   {int} TD_SUCCESS：成功，非 TD_SUCCESS：失败
 */
static int cipherContext_trng_get_bytes(CipherContext_S *pHandle, td_u8 *pOut, td_u32 u32Len);
/**
 * @brief   : 使用硬件 SYMC 子模块执行 SM4-CBC raw block 加解密
 * @param    {CipherContext_S*} pHandle：安全子系统上下文句柄
 * @param    {td_u8*} pKey：16 字节 SM4 明文密钥
 * @param    {td_u32} u32KeyLen：密钥长度，必须为 16
 * @param    {td_u8*} pIv：16 字节 CBC IV
 * @param    {td_u32} u32IvLen：IV 长度，必须为 16
 * @param    {td_u8*} pInput：输入数据，长度必须 16 字节对齐
 * @param    {td_u32} u32InputLen：输入数据长度
 * @param    {td_u8*} pOutput：输出缓冲区
 * @param    {td_u32} u32OutputSize：输出缓冲区大小
 * @param    {td_u32*} pOutputLen：实际输出长度
 * @param    {td_bool} bEncrypt：TD_TRUE 加密，TD_FALSE 解密
 * @return   {int} TD_SUCCESS：成功，非 TD_SUCCESS：失败
 */
static int cipherContext_sm4_cbc_crypt(CipherContext_S *pHandle,
                                       const td_u8 *pKey,
                                       td_u32 u32KeyLen,
                                       const td_u8 *pIv,
                                       td_u32 u32IvLen,
                                       const td_u8 *pInput,
                                       td_u32 u32InputLen,
                                       td_u8 *pOutput,
                                       td_u32 u32OutputSize,
                                       td_u32 *pOutputLen,
                                       td_bool bEncrypt);
/**
 * @brief   : 申请硬件加解密使用的 MMZ 物理连续缓冲区
 * @param    {CipherContextMmzBuf_S*} pBuf：缓冲区描述
 * @param    {td_u32} u32Size：申请大小
 * @return   {int} TD_SUCCESS：成功，非 TD_SUCCESS：失败
 */
static int cipherContext_mmz_alloc(CipherContextMmzBuf_S *pBuf, td_u32 u32Size);
/**
 * @brief   : 释放硬件加解密使用的 MMZ 物理连续缓冲区
 * @param    {CipherContextMmzBuf_S*} pBuf：缓冲区描述
 * @return   {void}
 */
static void cipherContext_mmz_free(CipherContextMmzBuf_S *pBuf);
static int cipherContext_sm2_keygen(CipherContext_S *pHandle,
                                    td_u8 *pPrivKey,
                                    td_u32 u32PrivKeyLen,
                                    td_u8 *pPubX,
                                    td_u32 u32PubXLen,
                                    td_u8 *pPubY,
                                    td_u32 u32PubYLen);
static int cipherContext_sm2_dsa_hash(CipherContext_S *pHandle,
                                      const td_u8 *pId,
                                      td_u32 u32IdLen,
                                      const td_u8 *pPubX,
                                      td_u32 u32PubXLen,
                                      const td_u8 *pPubY,
                                      td_u32 u32PubYLen,
                                      const td_u8 *pMsg,
                                      td_u32 u32MsgLen,
                                      td_u8 *pHash,
                                      td_u32 u32HashLen);
static int cipherContext_sm2_sign(CipherContext_S *pHandle,
                                  const td_u8 *pPrivKey,
                                  td_u32 u32PrivKeyLen,
                                  const td_u8 *pHash,
                                  td_u32 u32HashLen,
                                  td_u8 *pSigR,
                                  td_u32 u32SigRLen,
                                  td_u8 *pSigS,
                                  td_u32 u32SigSLen);
static int cipherContext_sm2_verify(CipherContext_S *pHandle,
                                    const td_u8 *pPubX,
                                    td_u32 u32PubXLen,
                                    const td_u8 *pPubY,
                                    td_u32 u32PubYLen,
                                    const td_u8 *pHash,
                                    td_u32 u32HashLen,
                                    const td_u8 *pSigR,
                                    td_u32 u32SigRLen,
                                    const td_u8 *pSigS,
                                    td_u32 u32SigSLen);
static int cipherContext_sm2_encrypt(CipherContext_S *pHandle,
                                     const td_u8 *pPubX,
                                     td_u32 u32PubXLen,
                                     const td_u8 *pPubY,
                                     td_u32 u32PubYLen,
                                     const td_u8 *pPlain,
                                     td_u32 u32PlainLen,
                                     td_u8 *pCipher,
                                     td_u32 u32CipherSize,
                                     td_u32 *pCipherLen);
static int cipherContext_sm2_decrypt(CipherContext_S *pHandle,
                                     const td_u8 *pPrivKey,
                                     td_u32 u32PrivKeyLen,
                                     const td_u8 *pCipher,
                                     td_u32 u32CipherLen,
                                     td_u8 *pPlain,
                                     td_u32 u32PlainSize,
                                     td_u32 *pPlainLen);

/**
 * @brief   : 分配安全子系统统一上下文并绑定能力函数表
 * @param    {CipherContextNeedParam_S} stNeedParam：调用方声明需要启用的子模块
 * @return   {CipherContext_S*} 成功返回上下文指针，失败返回 NULL
 */
CipherContext_S *cipherContext_alloc(CipherContextNeedParam_S stNeedParam)
{
    CipherContext_S *pHandle = (CipherContext_S *) malloc(sizeof(CipherContext_S));
    if (NULL == pHandle)
    {
        mpi_cipher_log("cipherContext_alloc malloc失败");
        return NULL;
    }

    memset_s(pHandle, sizeof(CipherContext_S), 0, sizeof(CipherContext_S));
    pHandle->stNeedParam = stNeedParam;
    pHandle->cipherContext_init = cipherContext_init;
    pHandle->cipherContext_uninit = cipherContext_uninit;
    pHandle->cipherContext_sm3_compute = cipherContext_sm3_compute;
    pHandle->cipherContext_trng_get_bytes = cipherContext_trng_get_bytes;
    pHandle->cipherContext_sm4_cbc_crypt = cipherContext_sm4_cbc_crypt;
    pHandle->cipherContext_sm2_keygen = cipherContext_sm2_keygen;
    pHandle->cipherContext_sm2_dsa_hash = cipherContext_sm2_dsa_hash;
    pHandle->cipherContext_sm2_sign = cipherContext_sm2_sign;
    pHandle->cipherContext_sm2_verify = cipherContext_sm2_verify;
    pHandle->cipherContext_sm2_encrypt = cipherContext_sm2_encrypt;
    pHandle->cipherContext_sm2_decrypt = cipherContext_sm2_decrypt;
    return pHandle;
}

/**
 * @brief   : 释放安全子系统统一上下文内存
 * @param    {CipherContext_S*} pHandle：安全子系统上下文句柄
 * @return   {void}
 */
void cipherContext_release(CipherContext_S *pHandle)
{
    if (NULL != pHandle)
    {
        free(pHandle);
    }
}

static int cipherContext_init(CipherContext_S *pHandle)
{
    td_s32 ret = TD_SUCCESS;

    if (NULL == pHandle)
    {
        return TD_FAILURE;
    }

    if (pHandle->stNeedParam.bEnableHash == TD_TRUE && pHandle->bHashInited == TD_FALSE)
    {
        ret = ot_mpi_cipher_hash_init();
        if (ret != TD_SUCCESS)
        {
            mpi_cipher_log("ot_mpi_cipher_hash_init failed, error code: 0x%08X", (unsigned int) ret);
            goto cleanup;
        }
        pHandle->bHashInited = TD_TRUE;
    }

    if (pHandle->stNeedParam.bEnableSymc == TD_TRUE && pHandle->bSymcInited == TD_FALSE)
    {
        ret = ot_mpi_cipher_symc_init();
        if (ret != TD_SUCCESS)
        {
            mpi_cipher_log("ot_mpi_cipher_symc_init failed, error code: 0x%08X", (unsigned int) ret);
            goto cleanup;
        }
        pHandle->bSymcInited = TD_TRUE;
    }

    if (pHandle->stNeedParam.bEnablePke == TD_TRUE && pHandle->bPkeInited == TD_FALSE)
    {
        ret = ot_mpi_cipher_pke_init();
        if (ret != TD_SUCCESS)
        {
            mpi_cipher_log("ot_mpi_cipher_pke_init failed, error code: 0x%08X", (unsigned int) ret);
            goto cleanup;
        }
        pHandle->bPkeInited = TD_TRUE;
    }

    return TD_SUCCESS;

cleanup:
    cipherContext_uninit(pHandle);
    return ret;
}

static int cipherContext_uninit(CipherContext_S *pHandle)
{
    if (NULL == pHandle)
    {
        return TD_FAILURE;
    }

    if (pHandle->bPkeInited == TD_TRUE)
    {
        CHECK_API_RETURN(ot_mpi_cipher_pke_deinit());
        pHandle->bPkeInited = TD_FALSE;
    }

    if (pHandle->bSymcInited == TD_TRUE)
    {
        CHECK_API_RETURN(ot_mpi_cipher_symc_deinit());
        pHandle->bSymcInited = TD_FALSE;
    }

    if (pHandle->bHashInited == TD_TRUE)
    {
        CHECK_API_RETURN(ot_mpi_cipher_hash_deinit());
        pHandle->bHashInited = TD_FALSE;
    }

    return TD_SUCCESS;
}

static int cipherContext_sm3_compute(CipherContext_S *pHandle,
                                     const td_u8 *pData,
                                     td_u32 u32Len,
                                     td_u8 *pOut,
                                     td_u32 u32OutSize,
                                     td_u32 *pOutLen)
{
    const td_u8 *pHashData = pData;
    td_u32 u32HashLen = u32Len;

    if (NULL == pHandle || NULL == pOut || NULL == pOutLen || u32Len > 0xFFFF0000 || u32OutSize < CIPHER_CONTEXT_SM3_DIGEST_LEN)
    {
        mpi_cipher_log("cipherContext_sm3_compute 参数错误");
        return TD_FAILURE;
    }
    if (u32Len == 0)
    {
        pHashData = &CIPHER_CONTEXT_EMPTY_INPUT;
        u32HashLen = 0;
    }
    else if (NULL == pData)
    {
        mpi_cipher_log("cipherContext_sm3_compute 输入数据为空");
        return TD_FAILURE;
    }

    crypto_hash_attr stHashAttr;
    memset_s(&stHashAttr, sizeof(stHashAttr), 0, sizeof(stHashAttr));
    stHashAttr.hash_type = CRYPTO_HASH_TYPE_SM3;
    stHashAttr.is_keyslot = TD_FALSE;
    stHashAttr.is_long_term = TD_FALSE;

    td_handle hashHandle = 0;
    CHECK_API_RETURN(ot_mpi_cipher_hash_create(&hashHandle, &stHashAttr));

    crypto_buf_attr stBufAttr;
    memset_s(&stBufAttr, sizeof(stBufAttr), 0, sizeof(stBufAttr));
    stBufAttr.virt_addr = (td_void *) pHashData;
    stBufAttr.buf_sec = CRYPTO_BUF_NONSECURE;

    td_s32 ret = TD_SUCCESS;
    if (u32HashLen > 0)
    {
        ret = ot_mpi_cipher_hash_update(hashHandle, &stBufAttr, u32HashLen);
        if (ret != TD_SUCCESS)
        {
            mpi_cipher_log("ot_mpi_cipher_hash_update failed, error code: 0x%08X", (unsigned int) ret);
            ot_mpi_cipher_hash_destroy(hashHandle);
            return ret;
        }
    }

    ret = ot_mpi_cipher_hash_finish(hashHandle, pOut, u32OutSize, pOutLen);
    if (ret != TD_SUCCESS)
    {
        mpi_cipher_log("ot_mpi_cipher_hash_finish failed, error code: 0x%08X", (unsigned int) ret);
        ot_mpi_cipher_hash_destroy(hashHandle);
        return ret;
    }

    /* note: PDF 明确 hash_finish 成功时会销毁 HASH 句柄，禁止再次调用 destroy。 */
    return TD_SUCCESS;
}

static int cipherContext_trng_get_bytes(CipherContext_S *pHandle, td_u8 *pOut, td_u32 u32Len)
{
    if (NULL == pHandle || NULL == pOut || u32Len == 0 || u32Len > 1024)
    {
        mpi_cipher_log("cipherContext_trng_get_bytes 参数错误");
        return TD_FAILURE;
    }

    CHECK_API_RETURN(ot_mpi_cipher_trng_get_multi_random(u32Len, pOut));
    return TD_SUCCESS;
}

static int cipherContext_mmz_alloc(CipherContextMmzBuf_S *pBuf, td_u32 u32Size)
{
    td_s32 ret = TD_SUCCESS;

    if (NULL == pBuf || u32Size == 0)
    {
        return TD_FAILURE;
    }

    memset_s(pBuf, sizeof(CipherContextMmzBuf_S), 0, sizeof(CipherContextMmzBuf_S));
    ret = ss_mpi_sys_mmz_alloc(&pBuf->phys_addr, (td_void **) &pBuf->virt_addr, "cipher_context", TD_NULL, u32Size);
    if (ret != TD_SUCCESS)
    {
        mpi_cipher_log("ss_mpi_sys_mmz_alloc failed, error code: 0x%08X", (unsigned int) ret);
        return ret;
    }

    pBuf->size = u32Size;
    memset_s(pBuf->virt_addr, u32Size, 0, u32Size);
    return TD_SUCCESS;
}

static void cipherContext_mmz_free(CipherContextMmzBuf_S *pBuf)
{
    if (NULL == pBuf || NULL == pBuf->virt_addr)
    {
        return;
    }

    (td_void) ss_mpi_sys_mmz_free(pBuf->phys_addr, pBuf->virt_addr);
    pBuf->phys_addr = 0;
    pBuf->virt_addr = NULL;
    pBuf->size = 0;
}

static int cipherContext_sm4_cbc_crypt(CipherContext_S *pHandle,
                                       const td_u8 *pKey,
                                       td_u32 u32KeyLen,
                                       const td_u8 *pIv,
                                       td_u32 u32IvLen,
                                       const td_u8 *pInput,
                                       td_u32 u32InputLen,
                                       td_u8 *pOutput,
                                       td_u32 u32OutputSize,
                                       td_u32 *pOutputLen,
                                       td_bool bEncrypt)
{
    td_s32 ret = TD_SUCCESS;
    CipherKmNeedParam_S stKmParam;
    CipherSymcNeedParam_S stSymcParam;
    CipherKm_S *pKm = NULL;
    CipherSymc_S *pSymc = NULL;
    CipherContextMmzBuf_S stSrcBuf = {0};
    CipherContextMmzBuf_S stDstBuf = {0};
    crypto_buf_attr stSrcAttr;
    crypto_buf_attr stDstAttr;
    crypto_symc_attr stSymcAttr;
    crypto_symc_ctrl_t stSymcCtrl;

    if (NULL == pHandle || NULL == pKey || NULL == pIv || NULL == pInput || NULL == pOutput || NULL == pOutputLen ||
        pHandle->bSymcInited != TD_TRUE || u32KeyLen != CIPHER_CONTEXT_SM4_BLOCK_LEN ||
        u32IvLen != CIPHER_CONTEXT_SM4_BLOCK_LEN || u32InputLen == 0 ||
        u32InputLen % CIPHER_CONTEXT_SM4_BLOCK_LEN != 0 || u32OutputSize < u32InputLen)
    {
        mpi_cipher_log("cipherContext_sm4_cbc_crypt 参数错误");
        return TD_FAILURE;
    }

    memset_s(&stKmParam, sizeof(stKmParam), 0, sizeof(stKmParam));
    stKmParam.enKeyslot_type = KM_KEYSLOT_TYPE_MCIPHER;
    stKmParam.enKlad_type = KM_KLAD_DEST_TYPE_MCIPHER;
    stKmParam.enEngine = KM_CRYPTO_ALG_SM4;
    stKmParam.bDecrypt_support = TD_TRUE;
    stKmParam.bEncrypt_support = TD_TRUE;

    memset_s(&stSymcParam, sizeof(stSymcParam), 0, sizeof(stSymcParam));
    pKm = cipherKm_alloc(stKmParam);
    pSymc = cipherSymc_alloc(stSymcParam);
    if (NULL == pKm || NULL == pSymc)
    {
        ret = TD_FAILURE;
        goto cleanup;
    }

    ret = pKm->cipherKm_init(pKm);
    if (ret != TD_SUCCESS)
    {
        goto cleanup;
    }
    ret = pKm->cipherKm_set_clear_key(pKm, pKey, u32KeyLen);
    if (ret != TD_SUCCESS)
    {
        goto cleanup;
    }

    ret = cipherContext_mmz_alloc(&stSrcBuf, u32InputLen);
    if (ret != TD_SUCCESS)
    {
        goto cleanup;
    }
    ret = cipherContext_mmz_alloc(&stDstBuf, u32InputLen);
    if (ret != TD_SUCCESS)
    {
        goto cleanup;
    }

    ret = memcpy_s(stSrcBuf.virt_addr, stSrcBuf.size, pInput, u32InputLen);
    if (ret != EOK)
    {
        ret = TD_FAILURE;
        goto cleanup;
    }
    ss_mpi_sys_flush_cache(stSrcBuf.phys_addr, stSrcBuf.virt_addr, u32InputLen);

    memset_s(&stSrcAttr, sizeof(stSrcAttr), 0, sizeof(stSrcAttr));
    stSrcAttr.phys_addr = stSrcBuf.phys_addr;
    stSrcAttr.virt_addr = stSrcBuf.virt_addr;
    stSrcAttr.buf_sec = CRYPTO_BUF_NONSECURE;

    memset_s(&stDstAttr, sizeof(stDstAttr), 0, sizeof(stDstAttr));
    stDstAttr.phys_addr = stDstBuf.phys_addr;
    stDstAttr.virt_addr = stDstBuf.virt_addr;
    stDstAttr.buf_sec = CRYPTO_BUF_NONSECURE;

    memset_s(&stSymcAttr, sizeof(stSymcAttr), 0, sizeof(stSymcAttr));
    stSymcAttr.symc_type = CRYPTO_SYMC_TYPE_NORMAL;
    stSymcAttr.symc_alg = CRYPTO_SYMC_ALG_SM4;
    stSymcAttr.work_mode = CRYPTO_SYMC_WORK_MODE_CBC;
    stSymcAttr.is_long_term = TD_FALSE;

    memset_s(&stSymcCtrl, sizeof(stSymcCtrl), 0, sizeof(stSymcCtrl));
    stSymcCtrl.symc_alg = CRYPTO_SYMC_ALG_SM4;
    stSymcCtrl.work_mode = CRYPTO_SYMC_WORK_MODE_CBC;
    stSymcCtrl.symc_key_length = CRYPTO_SYMC_KEY_128BIT;
    stSymcCtrl.key_parity = CRYPTO_SYMC_KEY_ODD;
    stSymcCtrl.symc_bit_width = CRYPTO_SYMC_BIT_WIDTH_128BIT;
    stSymcCtrl.iv_change_flag = CRYPTO_SYMC_IV_DO_NOT_CHANGE;
    stSymcCtrl.iv_length = CIPHER_CONTEXT_SM4_BLOCK_LEN;
    ret = memcpy_s(stSymcCtrl.iv, sizeof(stSymcCtrl.iv), pIv, u32IvLen);
    if (ret != EOK)
    {
        ret = TD_FAILURE;
        goto cleanup;
    }

    if (bEncrypt == TD_TRUE)
    {
        ret = pSymc->cipherSymc_encryption(pSymc, stSymcAttr, pKm->mpi_keyslot_handle, stSymcCtrl, &stSrcAttr,
                                           &stDstAttr, u32InputLen, NULL);
    }
    else
    {
        ret = pSymc->cipherSymc_decryption(pSymc, stSymcAttr, pKm->mpi_keyslot_handle, stSymcCtrl, &stSrcAttr,
                                           &stDstAttr, u32InputLen, NULL);
    }
    if (ret != TD_SUCCESS)
    {
        goto cleanup;
    }

    ss_mpi_sys_flush_cache(stDstBuf.phys_addr, stDstBuf.virt_addr, u32InputLen);
    ret = memcpy_s(pOutput, u32OutputSize, stDstBuf.virt_addr, u32InputLen);
    if (ret != EOK)
    {
        ret = TD_FAILURE;
        goto cleanup;
    }
    *pOutputLen = u32InputLen;

cleanup:
    cipherContext_mmz_free(&stDstBuf);
    cipherContext_mmz_free(&stSrcBuf);
    if (NULL != pKm)
    {
        (td_void) pKm->cipherKm_uninit(pKm);
        cipherKm_release(pKm);
    }
    if (NULL != pSymc)
    {
        cipherSymc_release(pSymc);
    }
    return ret;
}

static int cipherContext_check_pke_ready(CipherContext_S *pHandle)
{
    if (NULL == pHandle || pHandle->bPkeInited != TD_TRUE)
    {
        mpi_cipher_log("cipherContext PKE 未初始化");
        return TD_FAILURE;
    }
    return TD_SUCCESS;
}

static int cipherContext_sm2_keygen(CipherContext_S *pHandle,
                                    td_u8 *pPrivKey,
                                    td_u32 u32PrivKeyLen,
                                    td_u8 *pPubX,
                                    td_u32 u32PubXLen,
                                    td_u8 *pPubY,
                                    td_u32 u32PubYLen)
{
    if (cipherContext_check_pke_ready(pHandle) != TD_SUCCESS || NULL == pPrivKey || NULL == pPubX || NULL == pPubY ||
        u32PrivKeyLen != CIPHER_CONTEXT_SM2_KEY_LEN || u32PubXLen != CIPHER_CONTEXT_SM2_KEY_LEN ||
        u32PubYLen != CIPHER_CONTEXT_SM2_KEY_LEN)
    {
        mpi_cipher_log("cipherContext_sm2_keygen 参数错误");
        return TD_FAILURE;
    }

    drv_pke_data stPriv = {CIPHER_CONTEXT_SM2_KEY_LEN, pPrivKey};
    drv_pke_ecc_point stPub = {pPubX, pPubY, CIPHER_CONTEXT_SM2_KEY_LEN};
    CHECK_API_RETURN(ot_mpi_cipher_pke_ecc_gen_key(DRV_PKE_ECC_TYPE_SM2, NULL, &stPriv, &stPub));
    return TD_SUCCESS;
}

static int cipherContext_sm2_dsa_hash(CipherContext_S *pHandle,
                                      const td_u8 *pId,
                                      td_u32 u32IdLen,
                                      const td_u8 *pPubX,
                                      td_u32 u32PubXLen,
                                      const td_u8 *pPubY,
                                      td_u32 u32PubYLen,
                                      const td_u8 *pMsg,
                                      td_u32 u32MsgLen,
                                      td_u8 *pHash,
                                      td_u32 u32HashLen)
{
    if (cipherContext_check_pke_ready(pHandle) != TD_SUCCESS || NULL == pId || u32IdLen == 0 || NULL == pPubX ||
        NULL == pPubY || NULL == pMsg || u32MsgLen == 0 || NULL == pHash ||
        u32PubXLen != CIPHER_CONTEXT_SM2_KEY_LEN || u32PubYLen != CIPHER_CONTEXT_SM2_KEY_LEN ||
        u32HashLen != CIPHER_CONTEXT_SM3_DIGEST_LEN)
    {
        mpi_cipher_log("cipherContext_sm2_dsa_hash 参数错误");
        return TD_FAILURE;
    }

    drv_pke_data stId = {u32IdLen, (td_u8 *) pId};
    drv_pke_ecc_point stPub = {(td_u8 *) pPubX, (td_u8 *) pPubY, CIPHER_CONTEXT_SM2_KEY_LEN};
    drv_pke_msg stMsg = {u32MsgLen, (td_u8 *) pMsg, DRV_PKE_BUF_NONSECURE};
    drv_pke_data stHash = {CIPHER_CONTEXT_SM3_DIGEST_LEN, pHash};
    CHECK_API_RETURN(ot_mpi_cipher_pke_sm2_dsa_hash(&stId, &stPub, &stMsg, &stHash));
    return TD_SUCCESS;
}

static int cipherContext_sm2_sign(CipherContext_S *pHandle,
                                  const td_u8 *pPrivKey,
                                  td_u32 u32PrivKeyLen,
                                  const td_u8 *pHash,
                                  td_u32 u32HashLen,
                                  td_u8 *pSigR,
                                  td_u32 u32SigRLen,
                                  td_u8 *pSigS,
                                  td_u32 u32SigSLen)
{
    if (cipherContext_check_pke_ready(pHandle) != TD_SUCCESS || NULL == pPrivKey || NULL == pHash || NULL == pSigR ||
        NULL == pSigS || u32PrivKeyLen != CIPHER_CONTEXT_SM2_KEY_LEN ||
        u32HashLen != CIPHER_CONTEXT_SM3_DIGEST_LEN || u32SigRLen != CIPHER_CONTEXT_SM2_KEY_LEN ||
        u32SigSLen != CIPHER_CONTEXT_SM2_KEY_LEN)
    {
        mpi_cipher_log("cipherContext_sm2_sign 参数错误");
        return TD_FAILURE;
    }

    drv_pke_data stPriv = {CIPHER_CONTEXT_SM2_KEY_LEN, (td_u8 *) pPrivKey};
    drv_pke_data stHash = {CIPHER_CONTEXT_SM3_DIGEST_LEN, (td_u8 *) pHash};
    drv_pke_ecc_sig stSig = {pSigR, pSigS, CIPHER_CONTEXT_SM2_KEY_LEN};
    CHECK_API_RETURN(ot_mpi_cipher_pke_ecdsa_sign(DRV_PKE_ECC_TYPE_SM2, &stPriv, &stHash, &stSig));
    return TD_SUCCESS;
}

static int cipherContext_sm2_verify(CipherContext_S *pHandle,
                                    const td_u8 *pPubX,
                                    td_u32 u32PubXLen,
                                    const td_u8 *pPubY,
                                    td_u32 u32PubYLen,
                                    const td_u8 *pHash,
                                    td_u32 u32HashLen,
                                    const td_u8 *pSigR,
                                    td_u32 u32SigRLen,
                                    const td_u8 *pSigS,
                                    td_u32 u32SigSLen)
{
    if (cipherContext_check_pke_ready(pHandle) != TD_SUCCESS || NULL == pPubX || NULL == pPubY || NULL == pHash ||
        NULL == pSigR || NULL == pSigS || u32PubXLen != CIPHER_CONTEXT_SM2_KEY_LEN ||
        u32PubYLen != CIPHER_CONTEXT_SM2_KEY_LEN || u32HashLen != CIPHER_CONTEXT_SM3_DIGEST_LEN ||
        u32SigRLen != CIPHER_CONTEXT_SM2_KEY_LEN || u32SigSLen != CIPHER_CONTEXT_SM2_KEY_LEN)
    {
        mpi_cipher_log("cipherContext_sm2_verify 参数错误");
        return TD_FAILURE;
    }

    drv_pke_ecc_point stPub = {(td_u8 *) pPubX, (td_u8 *) pPubY, CIPHER_CONTEXT_SM2_KEY_LEN};
    drv_pke_data stHash = {CIPHER_CONTEXT_SM3_DIGEST_LEN, (td_u8 *) pHash};
    drv_pke_ecc_sig stSig = {(td_u8 *) pSigR, (td_u8 *) pSigS, CIPHER_CONTEXT_SM2_KEY_LEN};
    CHECK_API_RETURN(ot_mpi_cipher_pke_ecdsa_verify(DRV_PKE_ECC_TYPE_SM2, &stPub, &stHash, &stSig));
    return TD_SUCCESS;
}

static int cipherContext_sm2_encrypt(CipherContext_S *pHandle,
                                     const td_u8 *pPubX,
                                     td_u32 u32PubXLen,
                                     const td_u8 *pPubY,
                                     td_u32 u32PubYLen,
                                     const td_u8 *pPlain,
                                     td_u32 u32PlainLen,
                                     td_u8 *pCipher,
                                     td_u32 u32CipherSize,
                                     td_u32 *pCipherLen)
{
    td_u32 u32NeedLen = u32PlainLen + CIPHER_CONTEXT_SM2_CIPHER_ADD_LEN;

    if (cipherContext_check_pke_ready(pHandle) != TD_SUCCESS || NULL == pPubX || NULL == pPubY || NULL == pPlain ||
        NULL == pCipher || NULL == pCipherLen || u32PubXLen != CIPHER_CONTEXT_SM2_KEY_LEN ||
        u32PubYLen != CIPHER_CONTEXT_SM2_KEY_LEN || u32PlainLen == 0 || u32CipherSize < u32NeedLen)
    {
        mpi_cipher_log("cipherContext_sm2_encrypt 参数错误");
        return TD_FAILURE;
    }

    drv_pke_ecc_point stPub = {(td_u8 *) pPubX, (td_u8 *) pPubY, CIPHER_CONTEXT_SM2_KEY_LEN};
    drv_pke_data stPlain = {u32PlainLen, (td_u8 *) pPlain};
    drv_pke_data stCipher = {u32NeedLen, pCipher};
    CHECK_API_RETURN(ot_mpi_cipher_pke_sm2_public_encrypt(&stPub, &stPlain, &stCipher));
    *pCipherLen = stCipher.length;
    return TD_SUCCESS;
}

static int cipherContext_sm2_decrypt(CipherContext_S *pHandle,
                                     const td_u8 *pPrivKey,
                                     td_u32 u32PrivKeyLen,
                                     const td_u8 *pCipher,
                                     td_u32 u32CipherLen,
                                     td_u8 *pPlain,
                                     td_u32 u32PlainSize,
                                     td_u32 *pPlainLen)
{
    td_u32 u32ExpectedPlainLen = 0;

    if (cipherContext_check_pke_ready(pHandle) != TD_SUCCESS || NULL == pPrivKey || NULL == pCipher || NULL == pPlain ||
        NULL == pPlainLen || u32PrivKeyLen != CIPHER_CONTEXT_SM2_KEY_LEN ||
        u32CipherLen <= CIPHER_CONTEXT_SM2_CIPHER_ADD_LEN)
    {
        mpi_cipher_log("cipherContext_sm2_decrypt 参数错误");
        return TD_FAILURE;
    }

    u32ExpectedPlainLen = u32CipherLen - CIPHER_CONTEXT_SM2_CIPHER_ADD_LEN;
    if (u32PlainSize < u32ExpectedPlainLen)
    {
        mpi_cipher_log("cipherContext_sm2_decrypt 明文缓冲区不足");
        return TD_FAILURE;
    }

    drv_pke_data stPriv = {CIPHER_CONTEXT_SM2_KEY_LEN, (td_u8 *) pPrivKey};
    drv_pke_data stCipher = {u32CipherLen, (td_u8 *) pCipher};
    drv_pke_data stPlain = {u32ExpectedPlainLen, pPlain};
    CHECK_API_RETURN(ot_mpi_cipher_pke_sm2_private_decrypt(&stPriv, &stCipher, &stPlain));
    *pPlainLen = stPlain.length;
    return TD_SUCCESS;
}
