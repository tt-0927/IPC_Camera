/**
 * @FilePath     : hi_provider.c
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2026-05-27 11:32:29
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-06-23 09:13:30
 * @Description  : HiSilicon 硬件加速 OpenSSL Provider 实现
 *               支持 SM3(Digest)、TRNG(RAND)
 *               SM4-CBC 与 SM2 硬件桥接预留 TODO
 */

#include "hi_provider.h"
#include "dlog.h"
#include "IpcRet.h"

#include <string.h>
#include <stdlib.h>

#include <openssl/core.h>
#include <openssl/core_dispatch.h>
#include <openssl/core_names.h>
#include <openssl/params.h>
#include <openssl/provider.h>
#include <openssl/evp.h>

#include "cipher_context.h"

/* ===================== 宏定义 ===================== */

#define HI_PROVIDER_NAME    "hisi"
#define HI_PROVIDER_VERSION "1.0.0"

/**
 * @brief 返回值检查宏，若失败则记录日志并返回
 */
#define HI_CHECK_RET(expr, fmt, ...)                                                                                             \
    do                                                                                                                           \
    {                                                                                                                            \
        int _ret = (expr);                                                                                                       \
        if (_ret != 0)                                                                                                           \
        {                                                                                                                        \
            dlog_error("[HiProvider] " fmt ": %d", ##__VA_ARGS__, _ret);                                                         \
            return _ret;                                                                                                         \
        }                                                                                                                        \
    }                                                                                                                            \
    while (0)

/* ===================== Provider 上下文 ===================== */

typedef struct
{
    /* OpenSSL 库上下文指针 */
    OSSL_LIB_CTX *libctx;
    /* Provider 初始化标志，1 表示已初始化，0 表示未初始化 */
    int initialized;
    /* 密码学硬件上下文指针，用于 SM3 和 TRNG 操作 */
    CipherContext_S *pCipherContext;
} HI_PROVIDER_CTX;

/* HiProvider 全局上下文实例 */
static HI_PROVIDER_CTX g_stProviderCtx = { 0 };
/* OpenSSL Provider 实例指针 */
static OSSL_PROVIDER *g_pHiProvider = NULL;

/* ===================== SM3 上下文 ===================== */

typedef struct
{
    /* 指向 Provider 全局上下文的指针 */
    HI_PROVIDER_CTX *provctx;
    /* 存储待计算摘要数据的动态缓冲区指针 */
    unsigned char *pData;
    /* 当前缓冲区中有效数据的长度（字节） */
    size_t u32Len;
    /* 当前缓冲区的总容量（字节） */
    size_t u32Capacity;
    /* SM3 上下文初始化标志 */
    int initialized;
} HI_SM3_CTX;

/* SM3 输出长度 */
#define SM3_DIGEST_LENGTH 32
#define SM3_BLOCK_SIZE    64

/* ===================== SM4-CBC 上下文 ===================== */

// todo: 实现 cipher_context 硬件桥接后注册 SM4-CBC

/* ===================== RAND 上下文 ===================== */

typedef struct
{
    /* 指向 Provider 全局上下文的指针 */
    HI_PROVIDER_CTX *provctx;
    /* RAND 实例化标志，1 表示已实例化，0 表示未实例化 */
    int instantiated;
} HI_RAND_CTX;

/* ===================== SM3 Provider 实现 ===================== */

/**
 * @brief   : 创建 SM3 摘要计算上下文
 * @param    {void *} provctx：Provider 全局上下文指针
 * @return   {void *} 成功返回 SM3 上下文指针，失败返回 NULL
 * @note    : 使用 calloc 分配并自动清零上下文结构体
 */
static void *hi_sm3_newctx(void *provctx)
{
    // memory: calloc 自动清零上下文，防止未初始化访问
    HI_SM3_CTX *ctx = calloc(1, sizeof(HI_SM3_CTX));
    if (ctx == NULL)
    {
        dlog_error("[HiProvider] SM3 newctx: 内存分配失败");
        return NULL;
    }

    /* 初始化上下文成员 */
    ctx->provctx = (HI_PROVIDER_CTX *) provctx;
    ctx->pData = NULL;
    ctx->u32Len = 0;
    ctx->u32Capacity = 0;
    ctx->initialized = 0;

    dlog_trace("[HiProvider] SM3 newctx 成功");
    return ctx;
}

/**
 * @brief   : 释放 SM3 摘要计算上下文及内部数据缓冲区
 * @param    {void *} vctx：SM3 上下文指针
 * @note    : 释放内部数据缓冲区 pData 后再释放上下文结构体，防止内存泄漏
 */
static void hi_sm3_freectx(void *vctx)
{
    HI_SM3_CTX *ctx = (HI_SM3_CTX *) vctx;
    if (ctx == NULL)
    {
        return;
    }

    // memory: 先释放内部缓冲区，防止部分清理时泄漏
    if (ctx->pData != NULL)
    {
        free(ctx->pData);
        ctx->pData = NULL;
    }

    // memory: 所有内部资源释放后再释放上下文结构体
    free(ctx);
    dlog_trace("[HiProvider] SM3 freectx 完成");
}

/**
 * @brief   : 复制 SM3 上下文
 * @param    {void *} vctx：源 SM3 上下文指针
 * @return   {void *} 当前返回 NULL，表示 clone 操作未实现
 * @note    : SM3 hash clone 需要调用 ot_mpi_cipher_hash_get/set，当前暂不支持
 */
static void *hi_sm3_dupctx(void *vctx)
{
    (void) vctx;
    // todo: 使用 ot_mpi_cipher_hash_get/set 实现 SM3 上下文克隆
    dlog_warn("[HiProvider] SM3 dupctx 未实现");
    return NULL;
}

/**
 * @brief   : 初始化 SM3 摘要计算上下文
 * @param    {void *} vctx：SM3 上下文指针
 * @param    {const OSSL_PARAM []} params：参数列表（当前未使用）
 * @return   {int} 1 表示初始化成功，0 表示失败
 * @note    : 重置数据长度并设置初始化标志，不释放已分配的缓冲区
 */
static int hi_sm3_init(void *vctx, const OSSL_PARAM params[])
{
    (void) params;
    HI_SM3_CTX *ctx = (HI_SM3_CTX *) vctx;
    if (ctx == NULL)
    {
        dlog_error("[HiProvider] SM3 init: 上下文为空");
        return 0;
    }

    /* 重置数据长度为 0，保留已有缓冲区 */
    ctx->u32Len = 0;
    ctx->initialized = 1;
    dlog_trace("[HiProvider] SM3 init 成功");
    return 1;
}

/**
 * @brief   : 更新 SM3 摘要数据（追加输入数据到内部缓冲区）
 * @param    {void *} vctx：SM3 上下文指针
 * @param    {const unsigned char *} in：输入数据指针
 * @param    {size_t} inl：输入数据长度（字节）
 * @return   {int} 1 表示成功，0 表示失败
 * @note    : 当内部缓冲区容量不足时，按 2 倍策略自动扩容，单次最大支持 0xFFFF0000 字节
 */
static int hi_sm3_update(void *vctx, const unsigned char *in, size_t inl)
{
    HI_SM3_CTX *ctx = (HI_SM3_CTX *) vctx;
    if (ctx == NULL || (in == NULL && inl != 0))
    {
        dlog_error("[HiProvider] SM3 update: 参数错误");
        return 0;
    }

    /* 检查上下文是否已初始化 */
    if (!ctx->initialized)
    {
        dlog_error("[HiProvider] SM3 update: 未初始化");
        return 0;
    }

    /* 输入长度为 0 时直接返回成功 */
    if (inl == 0)
    {
        return 1;
    }

    // ! check total length overflow to prevent heap corruption
    if (inl > 0xFFFF0000 || ctx->u32Len > 0xFFFF0000 - inl)
    {
        dlog_error("[HiProvider] SM3 update: 输入长度超限 %zu", inl);
        return 0;
    }

    const size_t new_len = ctx->u32Len + inl;
    // perf: 2x growth strategy reduces realloc frequency for incremental hashing
    if (new_len > ctx->u32Capacity)
    {
        size_t new_capacity = ctx->u32Capacity == 0 ? SM3_BLOCK_SIZE : ctx->u32Capacity;
        while (new_capacity < new_len)
        {
            // ! prevent capacity overflow during exponential growth
            if (new_capacity > 0xFFFF0000 / 2)
            {
                new_capacity = new_len;
                break;
            }
            new_capacity *= 2;
        }

        // memory: realloc 可能失败，保留原始缓冲区以防错误
        unsigned char *pNewData = (unsigned char *) realloc(ctx->pData, new_capacity);
        if (pNewData == NULL)
        {
            dlog_error("[HiProvider] SM3 update: 缓存扩容失败");
            return 0;
        }
        ctx->pData = pNewData;
        ctx->u32Capacity = new_capacity;
    }

    // perf: 批量拷贝输入数据以减少内存操作次数
    memcpy(ctx->pData + ctx->u32Len, in, inl);
    ctx->u32Len = new_len;
    return 1;
}

/**
 * @brief   : 完成 SM3 摘要计算并输出结果
 * @param    {void *} vctx：SM3 上下文指针
 * @param    {unsigned char *} out：摘要结果输出缓冲区
 * @param    {size_t *} outl：输出实际摘要长度
 * @param    {size_t} outsz：输出缓冲区大小
 * @return   {int} 1 表示成功，0 表示失败
 * @note    : 调用底层硬件加速接口计算摘要，完成后重置上下文状态
 */
static int hi_sm3_final(void *vctx, unsigned char *out, size_t *outl, size_t outsz)
{
    HI_SM3_CTX *ctx = (HI_SM3_CTX *) vctx;
    if (ctx == NULL || out == NULL || outl == NULL)
    {
        dlog_error("[HiProvider] SM3 final: 参数错误");
        return 0;
    }

    /* 检查上下文是否已初始化 */
    if (!ctx->initialized)
    {
        dlog_error("[HiProvider] SM3 final: 未初始化");
        return 0;
    }

    /* 检查输出缓冲区是否足够存放 SM3 摘要结果 */
    if (outsz < SM3_DIGEST_LENGTH)
    {
        dlog_error("[HiProvider] SM3 final: 输出缓冲区不足");
        return 0;
    }

    // ! null provider context would cause segmentation fault on hardware call
    if (ctx->provctx == NULL || ctx->provctx->pCipherContext == NULL)
    {
        dlog_error("[HiProvider] SM3 final: cipher_context 未初始化");
        return 0;
    }

    // ! data inconsistency: non-zero length with null buffer indicates corruption
    if (ctx->u32Len > 0 && ctx->pData == NULL)
    {
        dlog_error("[HiProvider] SM3 final: 输入缓存为空");
        return 0;
    }

    // perf: 通过海思硬件密码引擎加速 SM3 摘要计算
    td_u32 result_len = 0;
    td_s32 ret = ctx->provctx->pCipherContext->cipherContext_sm3_compute(ctx->provctx->pCipherContext,
                                                                         ctx->pData,
                                                                         (td_u32) ctx->u32Len,
                                                                         out,
                                                                         (td_u32) outsz,
                                                                         &result_len);
    if (ret != 0)
    {
        dlog_error("[HiProvider] SM3 final: cipherContext_sm3_compute 失败 %d", ret);
        return 0;
    }

    // step: 输出摘要长度并重置上下文状态供下次操作使用
    *outl = result_len;
    ctx->u32Len = 0;
    ctx->initialized = 0;

    dlog_trace("[HiProvider] SM3 final 成功，结果长度 %u", result_len);
    return 1;
}

/**
 * @brief   : 获取 SM3 摘要算法参数
 * @param    {OSSL_PARAM []} params：参数数组，用于接收算法属性
 * @return   {int} 1 表示成功，0 表示失败
 * @note    : 设置 SM3 的块大小、摘要长度、XOF 标志和 ALGID_ABSENT 标志
 */
static int hi_sm3_digest_get_params(OSSL_PARAM params[])
{
    OSSL_PARAM *p = NULL;

    p = OSSL_PARAM_locate(params, OSSL_DIGEST_PARAM_BLOCK_SIZE);
    if (p != NULL && !OSSL_PARAM_set_size_t(p, SM3_BLOCK_SIZE))
    {
        return 0;
    }

    p = OSSL_PARAM_locate(params, OSSL_DIGEST_PARAM_SIZE);
    if (p != NULL && !OSSL_PARAM_set_size_t(p, SM3_DIGEST_LENGTH))
    {
        return 0;
    }

    p = OSSL_PARAM_locate(params, OSSL_DIGEST_PARAM_XOF);
    if (p != NULL && !OSSL_PARAM_set_int(p, 0))
    {
        return 0;
    }

    p = OSSL_PARAM_locate(params, OSSL_DIGEST_PARAM_ALGID_ABSENT);
    if (p != NULL && !OSSL_PARAM_set_int(p, 0))
    {
        return 0;
    }

    return 1;
}

static const OSSL_PARAM hi_sm3_known_gettable_params[] = { OSSL_PARAM_size_t(OSSL_DIGEST_PARAM_BLOCK_SIZE, NULL),
                                                           OSSL_PARAM_size_t(OSSL_DIGEST_PARAM_SIZE, NULL),
                                                           OSSL_PARAM_int(OSSL_DIGEST_PARAM_XOF, NULL),
                                                           OSSL_PARAM_int(OSSL_DIGEST_PARAM_ALGID_ABSENT, NULL),
                                                           OSSL_PARAM_END };

/**
 * @brief   : 获取 SM3 可获取的参数列表
 * @param    {void *} provctx：Provider 上下文指针（当前未使用）
 * @return   {const OSSL_PARAM *} 参数列表指针
 */
static const OSSL_PARAM *hi_sm3_gettable_params(void *provctx)
{
    (void) provctx;
    return hi_sm3_known_gettable_params;
}

/**
 * @brief   : 获取 SM3 上下文可设置的参数列表
 * @param    {void *} vctx：SM3 上下文指针（当前未使用）
 * @param    {void *} provctx：Provider 上下文指针（当前未使用）
 * @return   {const OSSL_PARAM *} 当前返回 NULL，表示无上下文参数可设置
 */
static const OSSL_PARAM *hi_sm3_settable_ctx_params(void *vctx, void *provctx)
{
    (void) vctx;
    (void) provctx;
    return NULL;
}

/**
 * @brief   : 设置 SM3 上下文参数
 * @param    {void *} vctx：SM3 上下文指针（当前未使用）
 * @param    {const OSSL_PARAM []} params：参数列表（当前未使用）
 * @return   {int} 1 表示成功
 * @note    : 当前 SM3 实现无需上下文参数，直接返回成功
 */
static int hi_sm3_set_ctx_params(void *vctx, const OSSL_PARAM params[])
{
    (void) vctx;
    (void) params;
    return 1;
}

/**
 * @brief   : 获取 SM3 上下文可获取的参数列表
 * @param    {void *} vctx：SM3 上下文指针（当前未使用）
 * @param    {void *} provctx：Provider 上下文指针（当前未使用）
 * @return   {const OSSL_PARAM *} 当前返回 NULL，表示无上下文参数可获取
 */
static const OSSL_PARAM *hi_sm3_gettable_ctx_params(void *vctx, void *provctx)
{
    (void) vctx;
    (void) provctx;
    return NULL;
}

/**
 * @brief   : 获取 SM3 上下文参数
 * @param    {void *} vctx：SM3 上下文指针（当前未使用）
 * @param    {OSSL_PARAM []} params：参数数组（当前未使用）
 * @return   {int} 1 表示成功
 * @note    : 当前 SM3 实现无需上下文参数，直接返回成功
 */
static int hi_sm3_get_ctx_params(void *vctx, OSSL_PARAM params[])
{
    (void) vctx;
    (void) params;
    return 1;
}

/**
 * @brief   : SM3 摘要操作分发表
 * @note    : 将 OpenSSL Digest 操作函数指针与 SM3 Provider 实现函数绑定
 */
static const OSSL_DISPATCH hi_sm3_functions[] = {
    {              OSSL_FUNC_DIGEST_NEWCTX,              (void (*)(void)) hi_sm3_newctx },
    {             OSSL_FUNC_DIGEST_FREECTX,             (void (*)(void)) hi_sm3_freectx },
    {              OSSL_FUNC_DIGEST_DUPCTX,              (void (*)(void)) hi_sm3_dupctx },
    {                OSSL_FUNC_DIGEST_INIT,                (void (*)(void)) hi_sm3_init },
    {              OSSL_FUNC_DIGEST_UPDATE,              (void (*)(void)) hi_sm3_update },
    {               OSSL_FUNC_DIGEST_FINAL,               (void (*)(void)) hi_sm3_final },
    {          OSSL_FUNC_DIGEST_GET_PARAMS,   (void (*)(void)) hi_sm3_digest_get_params },
    {     OSSL_FUNC_DIGEST_GETTABLE_PARAMS,     (void (*)(void)) hi_sm3_gettable_params },
    { OSSL_FUNC_DIGEST_SETTABLE_CTX_PARAMS, (void (*)(void)) hi_sm3_settable_ctx_params },
    {      OSSL_FUNC_DIGEST_SET_CTX_PARAMS,      (void (*)(void)) hi_sm3_set_ctx_params },
    { OSSL_FUNC_DIGEST_GETTABLE_CTX_PARAMS, (void (*)(void)) hi_sm3_gettable_ctx_params },
    {      OSSL_FUNC_DIGEST_GET_CTX_PARAMS,      (void (*)(void)) hi_sm3_get_ctx_params },
    {                                    0,                                        NULL }
};

/* ===================== TRNG RAND Provider 实现 ===================== */

/**
 * @brief   : 创建 RAND（随机数生成器）上下文
 * @param    {void *} provctx：Provider 全局上下文指针
 * @return   {void *} 成功返回 RAND 上下文指针，失败返回 NULL
 */
static void *hi_rand_newctx(void *provctx)
{
    // memory: calloc 自动清零上下文，防止未初始化访问
    HI_RAND_CTX *ctx = calloc(1, sizeof(HI_RAND_CTX));
    if (ctx == NULL)
    {
        dlog_error("[HiProvider] RAND newctx: 内存分配失败");
        return NULL;
    }

    ctx->provctx = (HI_PROVIDER_CTX *) provctx;
    ctx->instantiated = 0;

    dlog_trace("[HiProvider] RAND newctx 成功");
    return ctx;
}

/**
 * @brief   : 释放 RAND 随机数生成器上下文
 * @param    {void *} vctx：RAND 上下文指针
 * @note    : 释放 RAND 上下文内存，TRNG 无需额外反初始化操作
 */
static void hi_rand_freectx(void *vctx)
{
    HI_RAND_CTX *ctx = (HI_RAND_CTX *) vctx;
    if (ctx == NULL)
    {
        return;
    }

    // memory: RAND 上下文无内部缓冲区，直接释放即可
    free(ctx);
    dlog_trace("[HiProvider] RAND freectx 完成");
}

/**
 * @brief   : 实例化 RAND 上下文
 * @param    {void *} vctx：RAND 上下文指针
 * @param    {unsigned int} strength：安全强度（当前未使用）
 * @param    {int} prediction_resistance：预测抵抗标志（当前未使用）
 * @param    {const unsigned char *} pstr：个性化字符串（当前未使用）
 * @param    {size_t} pstr_len：个性化字符串长度（当前未使用）
 * @param    {const OSSL_PARAM []} params：参数列表（当前未使用）
 * @return   {int} 1 表示成功，0 表示失败
 * @note    : TRNG 为真随机数生成器，无需真正的种子，仅设置实例化标志
 */
static int hi_rand_instantiate(void *vctx,
                               unsigned int strength,
                               int prediction_resistance,
                               const unsigned char *pstr,
                               size_t pstr_len,
                               const OSSL_PARAM params[])
{
    (void) strength;
    (void) prediction_resistance;
    (void) pstr;
    (void) pstr_len;
    (void) params;

    HI_RAND_CTX *ctx = (HI_RAND_CTX *) vctx;
    if (ctx == NULL)
    {
        dlog_error("[HiProvider] RAND instantiate: 上下文为空");
        return 0;
    }

    ctx->instantiated = 1;
    dlog_trace("[HiProvider] RAND instantiate 成功");
    return 1;
}

/**
 * @brief   : 反实例化 RAND 上下文
 * @param    {void *} vctx：RAND 上下文指针
 * @return   {int} 1 表示成功，0 表示失败
 * @note    : 清除实例化标志，使后续 generate 调用失败
 */
static int hi_rand_uninstantiate(void *vctx)
{
    HI_RAND_CTX *ctx = (HI_RAND_CTX *) vctx;
    if (ctx == NULL)
    {
        return 0;
    }

    ctx->instantiated = 0;
    dlog_trace("[HiProvider] RAND uninstantiate 完成");
    return 1;
}

/**
 * @brief   : 生成硬件随机数
 * @param    {void *} vctx：RAND 上下文指针
 * @param    {unsigned char *} out：随机数输出缓冲区
 * @param    {size_t} outlen：请求生成的随机数长度（字节）
 * @param    {unsigned int} strength：安全强度（当前未使用）
 * @param    {int} prediction_resistance：预测抵抗标志（当前未使用）
 * @param    {const unsigned char *} addin：附加输入（当前未使用）
 * @param    {size_t} addin_len：附加输入长度（当前未使用）
 * @return   {int} 1 表示成功，0 表示失败
 * @note    : 调用底层硬件 TRNG 接口获取真随机数
 */
static int hi_rand_generate(void *vctx,
                            unsigned char *out,
                            size_t outlen,
                            unsigned int strength,
                            int prediction_resistance,
                            const unsigned char *addin,
                            size_t addin_len)
{
    (void) strength;
    (void) prediction_resistance;
    (void) addin;
    (void) addin_len;

    HI_RAND_CTX *ctx = (HI_RAND_CTX *) vctx;
    if (ctx == NULL || out == NULL)
    {
        dlog_error("[HiProvider] RAND generate: 参数错误");
        return 0;
    }

    if (!ctx->instantiated)
    {
        dlog_error("[HiProvider] RAND generate: 未实例化");
        return 0;
    }

    if (ctx->provctx == NULL || ctx->provctx->pCipherContext == NULL)
    {
        dlog_error("[HiProvider] RAND generate: cipher_context 未初始化");
        return 0;
    }

    if (outlen == 0)
    {
        return 1;
    }

    // perf: 通过海思硬件密码引擎加速 TRNG 随机数生成
    td_s32 ret = ctx->provctx->pCipherContext->cipherContext_trng_get_bytes(ctx->provctx->pCipherContext, out, (td_u32) outlen);
    if (ret != 0)
    {
        dlog_error("[HiProvider] RAND generate: 硬件获取随机数失败 %d", ret);
        return 0;
    }

    dlog_trace("[HiProvider] RAND generate 成功，%zu 字节", outlen);
    return 1;
}

/**
 * @brief   : 重新种子 RAND 上下文
 * @param    {void *} vctx：RAND 上下文指针（当前未使用）
 * @param    {int} prediction_resistance：预测抵抗标志（当前未使用）
 * @param    {const unsigned char *} ent：熵输入（当前未使用）
 * @param    {size_t} ent_len：熵输入长度（当前未使用）
 * @param    {const unsigned char *} addin：附加输入（当前未使用）
 * @param    {size_t} addin_len：附加输入长度（当前未使用）
 * @return   {int} 1 表示成功
 * @note    : TRNG 为真随机数生成器，不需要重新种子，直接返回成功
 */
static int hi_rand_reseed(void *vctx,
                          int prediction_resistance,
                          const unsigned char *ent,
                          size_t ent_len,
                          const unsigned char *addin,
                          size_t addin_len)
{
    (void) vctx;
    (void) prediction_resistance;
    (void) ent;
    (void) ent_len;
    (void) addin;
    (void) addin_len;
    /* TRNG 不需要重新种子 */
    return 1;
}

/**
 * @brief   : 设置 RAND 参数
 */
/**
 * @brief   : 设置 RAND 上下文参数
 * @param    {void *} vctx：RAND 上下文指针（当前未使用）
 * @param    {const OSSL_PARAM []} params：参数列表（当前未使用）
 * @return   {int} 1 表示成功
 * @note    : 当前 TRNG 实现无需上下文参数，直接返回成功
 */
static int hi_rand_set_ctx_params(void *vctx, const OSSL_PARAM params[])
{
    (void) vctx;
    (void) params;
    return 1;
}

/**
 * @brief   : 获取 RAND 上下文可设置的参数列表
 * @param    {void *} vctx：RAND 上下文指针（当前未使用）
 * @param    {void *} provctx：Provider 上下文指针（当前未使用）
 * @return   {const OSSL_PARAM *} 当前返回 NULL，表示无上下文参数可设置
 */
static const OSSL_PARAM *hi_rand_settable_ctx_params(void *vctx, void *provctx)
{
    (void) vctx;
    (void) provctx;
    return NULL;
}

/**
 * @brief   : 获取 RAND 上下文参数
 * @param    {void *} vctx：RAND 上下文指针
 * @param    {OSSL_PARAM []} params：参数数组
 * @return   {int} 1 表示成功，0 表示失败
 * @note    : 返回当前实例化状态、安全强度（256 位）和最大请求长度（1024 字节）
 */
static int hi_rand_get_ctx_params(void *vctx, OSSL_PARAM params[])
{
    HI_RAND_CTX *ctx = (HI_RAND_CTX *) vctx;
    OSSL_PARAM *p;

    if (ctx == NULL || params == NULL)
    {
        return 0;
    }

    p = OSSL_PARAM_locate(params, OSSL_RAND_PARAM_STATE);
    if (p != NULL && !OSSL_PARAM_set_int(p, ctx->instantiated))
    {
        return 0;
    }

    p = OSSL_PARAM_locate(params, OSSL_RAND_PARAM_STRENGTH);
    if (p != NULL && !OSSL_PARAM_set_int(p, 256))
    {
        return 0;
    }

    p = OSSL_PARAM_locate(params, OSSL_RAND_PARAM_MAX_REQUEST);
    if (p != NULL && !OSSL_PARAM_set_size_t(p, 1024))
    {
        return 0;
    }

    return 1;
}

/**
 * @brief   : 获取 RAND 上下文可获取的参数列表
 * @param    {void *} vctx：RAND 上下文指针（当前未使用）
 * @param    {void *} provctx：Provider 上下文指针（当前未使用）
 * @return   {const OSSL_PARAM *} 参数列表指针，包含 STATE、STRENGTH 和 MAX_REQUEST
 */
static const OSSL_PARAM *hi_rand_gettable_ctx_params(void *vctx, void *provctx)
{
    (void) vctx;
    (void) provctx;
    static const OSSL_PARAM known_gettable_ctx_params[] = { OSSL_PARAM_int(OSSL_RAND_PARAM_STATE, NULL),
                                                            OSSL_PARAM_int(OSSL_RAND_PARAM_STRENGTH, NULL),
                                                            OSSL_PARAM_size_t(OSSL_RAND_PARAM_MAX_REQUEST, NULL),
                                                            OSSL_PARAM_END };
    return known_gettable_ctx_params;
}

/**
 * @brief   : 获取 RAND 全局参数
 * @param    {OSSL_PARAM []} params：参数数组
 * @return   {int} 1 表示成功，0 表示失败
 * @note    : 设置安全强度为 256 位，最大请求长度为 1024 字节
 */
static int hi_rand_get_params(OSSL_PARAM params[])
{
    OSSL_PARAM *p;

    /* 设置安全强度为 256 位 */
    p = OSSL_PARAM_locate(params, OSSL_RAND_PARAM_STRENGTH);
    if (p != NULL && !OSSL_PARAM_set_int(p, 256))
    {
        return 0;
    }

    /* 设置单次最大请求长度为 1024 字节 */
    p = OSSL_PARAM_locate(params, OSSL_RAND_PARAM_MAX_REQUEST);
    if (p != NULL && !OSSL_PARAM_set_size_t(p, 1024))
    {
        return 0;
    }

    return 1;
}

/**
 * @brief   : 获取 RAND 全局可获取的参数列表
 * @param    {void *} provctx：Provider 上下文指针（当前未使用）
 * @return   {const OSSL_PARAM *} 参数列表指针，包含 STRENGTH 和 MAX_REQUEST
 */
static const OSSL_PARAM *hi_rand_gettable_params(void *provctx)
{
    (void) provctx;
    static const OSSL_PARAM known_gettable_params[] = { OSSL_PARAM_int(OSSL_RAND_PARAM_STRENGTH, NULL),
                                                        OSSL_PARAM_size_t(OSSL_RAND_PARAM_MAX_REQUEST, NULL),
                                                        OSSL_PARAM_END };
    return known_gettable_params;
}

/**
 * @brief   : RAND（随机数生成器）操作分发表
 * @note    : 将 OpenSSL RAND 操作函数指针与 Provider 实现函数绑定
 */
static const OSSL_DISPATCH hi_rand_functions[] = {
    {              OSSL_FUNC_RAND_NEWCTX,              (void (*)(void)) hi_rand_newctx },
    {             OSSL_FUNC_RAND_FREECTX,             (void (*)(void)) hi_rand_freectx },
    {         OSSL_FUNC_RAND_INSTANTIATE,         (void (*)(void)) hi_rand_instantiate },
    {       OSSL_FUNC_RAND_UNINSTANTIATE,       (void (*)(void)) hi_rand_uninstantiate },
    {            OSSL_FUNC_RAND_GENERATE,            (void (*)(void)) hi_rand_generate },
    {              OSSL_FUNC_RAND_RESEED,              (void (*)(void)) hi_rand_reseed },
    {      OSSL_FUNC_RAND_SET_CTX_PARAMS,      (void (*)(void)) hi_rand_set_ctx_params },
    { OSSL_FUNC_RAND_SETTABLE_CTX_PARAMS, (void (*)(void)) hi_rand_settable_ctx_params },
    {      OSSL_FUNC_RAND_GET_CTX_PARAMS,      (void (*)(void)) hi_rand_get_ctx_params },
    { OSSL_FUNC_RAND_GETTABLE_CTX_PARAMS, (void (*)(void)) hi_rand_gettable_ctx_params },
    {          OSSL_FUNC_RAND_GET_PARAMS,          (void (*)(void)) hi_rand_get_params },
    {     OSSL_FUNC_RAND_GETTABLE_PARAMS,     (void (*)(void)) hi_rand_gettable_params },
    {                                  0,                                         NULL }
};

/* ===================== SM2 桥接（TODO） ===================== */

/**
 * @brief   : SM2 签名/验签桥接
 * @note    : TODO - 需要实现以下内容：
 *            1. EVP_PKEY 解析为硬件 drv_pke_data / drv_pke_ecc_point 结构
 *            2. SM2 ZA 哈希计算（ot_mpi_cipher_pke_sm2_dsa_hash）
 *            3. 调用 ot_mpi_cipher_pke_sm2_sign / sm2_verify
 *            4. DER 签名格式与硬件 r||s 格式转换
 *            5. 注册 SM2 dispatch table 到 Provider
 *
 *            当前 SM2 操作继续使用 OpenSSL 软件实现，
 *            由 COpenSSLProvider::sm2_sign / sm2_verify 处理。
 */

/* ===================== Provider 查询与注册 ===================== */

/**
 * @brief   : Provider 卸载回调函数
 * @param    {void *} provctx：Provider 全局上下文指针
 * @note    : 卸载时重置初始化标志
 */
static void hi_teardown(void *provctx)
{
    HI_PROVIDER_CTX *ctx = (HI_PROVIDER_CTX *) provctx;
    if (ctx != NULL)
    {
        ctx->initialized = 0;
        dlog_info("[HiProvider] Provider 已卸载");
    }
}

/**
 * @brief   : 支持的摘要算法列表
 * @note    : 当前仅注册 SM3 摘要算法，OID 为 1.2.156.10197.1.401
 */
static const OSSL_ALGORITHM hi_digests[] = {
    { "SM3:1.2.156.10197.1.401", "provider=hi", hi_sm3_functions, "HiSilicon SM3" },
    {                      NULL,          NULL,             NULL,            NULL }
};

static const OSSL_ALGORITHM hi_ciphers[] = {
    { NULL, NULL, NULL, NULL }
};

static const OSSL_ALGORITHM hi_rands[] = {
    {  "CTR-DRBG", "provider=hi", hi_rand_functions, "HiSilicon TRNG" },
    { "HASH-DRBG", "provider=hi", hi_rand_functions, "HiSilicon TRNG" },
    { "HMAC-DRBG", "provider=hi", hi_rand_functions, "HiSilicon TRNG" },
    {        NULL,          NULL,              NULL,             NULL }
};

/**
 * @brief   : Provider 查询函数，根据操作类型返回对应的算法分发表
 * @param    {void *} provctx：Provider 全局上下文指针
 * @param    {int} operation_id：操作类型标识（如 OSSL_OP_DIGEST、OSSL_OP_RAND 等）
 * @param    {const int *} no_cache：是否禁用缓存标志（当前未使用）
 * @return   {const OSSL_ALGORITHM *} 对应操作类型的算法分发表指针，不支持则返回 NULL
 * @note    : 当前支持 SM3 摘要算法和 TRNG 随机数生成
 */
static const OSSL_ALGORITHM *hi_query(void *provctx, int operation_id, const int *no_cache)
{
    (void) provctx;
    (void) no_cache;

    switch (operation_id)
    {
    case OSSL_OP_DIGEST:
        dlog_trace("[HiProvider] query OSSL_OP_DIGEST");
        return hi_digests;
    case OSSL_OP_CIPHER:
        dlog_trace("[HiProvider] query OSSL_OP_CIPHER");
        return hi_ciphers;
    case OSSL_OP_RAND:
        dlog_trace("[HiProvider] query OSSL_OP_RAND");
        return hi_rands;
    default:
        return NULL;
    }
}

/**
 * @brief   : Provider 初始化函数，由 OpenSSL 在加载 Provider 时调用
 * @param    {const OSSL_CORE_HANDLE *} handle：OpenSSL 核心句柄
 * @param    {const OSSL_DISPATCH *} in：OpenSSL 核心提供的函数分发表
 * @param    {const OSSL_DISPATCH **} out：返回本 Provider 支持的操作分发表
 * @param    {void **} provctx：返回 Provider 全局上下文指针
 * @return   {int} 1 表示初始化成功，0 表示失败
 * @note    : 初始化全局上下文并注册 teardown 和 query 回调
 */
static int hi_provider_init(const OSSL_CORE_HANDLE *handle, const OSSL_DISPATCH *in, const OSSL_DISPATCH **out, void **provctx)
{
    (void) handle;
    (void) in;

    /* 初始化 Provider 全局上下文 */
    g_stProviderCtx.libctx = NULL;
    g_stProviderCtx.initialized = 1;

    /* 定义 Provider 操作分发表 */
    static const OSSL_DISPATCH hi_dispatch_table[] = {
        {        OSSL_FUNC_PROVIDER_TEARDOWN, (void (*)(void)) hi_teardown },
        { OSSL_FUNC_PROVIDER_QUERY_OPERATION,    (void (*)(void)) hi_query },
        {                                  0,                         NULL }
    };

    /* 返回分发表和上下文指针给 OpenSSL 核心 */
    *out = hi_dispatch_table;
    *provctx = &g_stProviderCtx;

    dlog_info("[HiProvider] Provider 初始化完成");
    return 1;
}

/* ===================== 公共注册接口 ===================== */

/**
 * @brief   : 注册并加载 HiProvider 到 OpenSSL
 * @return   {int} OK 表示注册成功，ERR 表示失败
 * @note    : 注册流程：
 *            1. 检查是否已注册
 *            2. 分配并初始化底层密码学硬件上下文（启用 Hash 和 TRNG）
 *            3. 使用 OSSL_PROVIDER_add_builtin 注册 Provider
 *            4. 加载 Provider 使其生效
 */
int hi_provider_register()
{
    // step 1: 检查 Provider 是否已注册，防止重复初始化
    if (g_pHiProvider != NULL)
    {
        dlog_warn("[HiProvider] Provider 已注册");
        return OK;
    }

    // step 2: 配置硬件能力，仅启用 SM3 摘要和 TRNG 随机数
    CipherContextNeedParam_S stNeedParam = {
        .bEnableHash = TD_TRUE,
        .bEnableSymc = TD_FALSE,
        .bEnablePke = TD_FALSE,
        .bEnableTrng = TD_TRUE,
    };

    // step 3: 分配密码学硬件上下文
    g_stProviderCtx.pCipherContext = cipherContext_alloc(stNeedParam);
    if (NULL == g_stProviderCtx.pCipherContext)
    {
        dlog_error("cipherContext_alloc 失败");
        return ERR;
    }

    // step 4: 初始化密码学硬件
    if (g_stProviderCtx.pCipherContext->cipherContext_init(g_stProviderCtx.pCipherContext) != TD_SUCCESS)
    {
        dlog_error("cipherContext_init 失败");
        // memory: 初始化失败时释放已分配的上下文，防止内存泄漏
        cipherContext_release(g_stProviderCtx.pCipherContext);
        g_stProviderCtx.pCipherContext = NULL;
        return ERR;
    }

    // step 5: 向 OpenSSL 注册 Provider
    if (!OSSL_PROVIDER_add_builtin(NULL, HI_PROVIDER_NAME, hi_provider_init))
    {
        dlog_error("[HiProvider] OSSL_PROVIDER_add_builtin 失败");
        g_stProviderCtx.pCipherContext->cipherContext_uninit(g_stProviderCtx.pCipherContext);
        cipherContext_release(g_stProviderCtx.pCipherContext);
        g_stProviderCtx.pCipherContext = NULL;
        return ERR;
    }

    // step 6: 加载 Provider 使其生效
    g_pHiProvider = OSSL_PROVIDER_load(NULL, HI_PROVIDER_NAME);
    if (g_pHiProvider == NULL)
    {
        dlog_error("[HiProvider] OSSL_PROVIDER_load 失败");
        g_stProviderCtx.pCipherContext->cipherContext_uninit(g_stProviderCtx.pCipherContext);
        cipherContext_release(g_stProviderCtx.pCipherContext);
        g_stProviderCtx.pCipherContext = NULL;
        return ERR;
    }

    dlog_info("[HiProvider] Provider 注册成功");
    return OK;
}

/**
 * @brief   : 注销 HiProvider 并释放相关资源
 * @return   {int} OK 表示注销成功
 * @note    : 注销流程：
 *            1. 卸载 OpenSSL Provider
 *            2. 反初始化并释放底层密码学硬件上下文
 */
int hi_provider_unregister()
{
    /* 卸载 Provider */
    if (g_pHiProvider != NULL)
    {
        OSSL_PROVIDER_unload(g_pHiProvider);
        g_pHiProvider = NULL;
        dlog_info("[HiProvider] Provider 已注销");
    }

    /* 释放密码学硬件上下文 */
    if (g_stProviderCtx.pCipherContext != NULL)
    {
        g_stProviderCtx.pCipherContext->cipherContext_uninit(g_stProviderCtx.pCipherContext);
        cipherContext_release(g_stProviderCtx.pCipherContext);
        g_stProviderCtx.pCipherContext = NULL;
    }

    return OK;
}
