/**
 * @FilePath     : cipher_km.c
 * @Author       : zhouzirui
 * @Date         : 2025-04-03 09:56:01
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-06-22 17:45:44
 * @Description  : 密钥管理模块
 */

#include "cipher_km.h"
#include "cipher_otp.h"
#include "cipher_trng.h"

/**
 * @brief       : TEE 启用检测的 OTP 偏移地址
 * @note        : 读取 OTP[0x12] 地址，当值为 0x42 时表示 TEE 安全模式已启用
 */
#define CIPHER_KM_TEE_ENABLE_OTP_OFFSET 0x12

/**
 * @brief       : TEE 启用检测的 OTP 期望值
 * @note        : 当 OTP[0x12] == 0x42 时，必须使用 secure key 配置，否则 SYMC 可能等待超时
 */
#define CIPHER_KM_TEE_ENABLE_OTP_VALUE 0x42

/**
 * @brief       : 配置 REE 非安全 keyslot 属性
 * @author      : zhouzr
 * @param        {km_klad_key_secure_config} *pKeySecCfg：KLAD key 安全属性
 * @return       {void}
 * @note        : 未开启 TEE secure key 的平台继续走非安全 buffer，保持旧平台兼容性。
 */
static void cipherKm_set_non_secure_key_cfg(km_klad_key_secure_config *pKeySecCfg)
{
    if (NULL == pKeySecCfg)
    {
        return;
    }

    memset(pKeySecCfg, 0, sizeof(km_klad_key_secure_config));
    pKeySecCfg->key_sec = KM_KLAD_SEC_DISABLE;
    pKeySecCfg->master_only_enable = TD_FALSE;
    pKeySecCfg->dest_buf_sec_support = TD_FALSE;
    pKeySecCfg->dest_buf_non_sec_support = TD_TRUE;
    pKeySecCfg->src_buf_sec_support = TD_FALSE;
    pKeySecCfg->src_buf_non_sec_support = TD_TRUE;
}

/**
 * @brief       : 配置 TEE 安全 keyslot 属性
 * @author      : zhouzr
 * @param        {km_klad_key_secure_config} *pKeySecCfg：KLAD key 安全属性
 * @return       {void}
 * @note        : 对齐官方 sample_cipher；OTP[0x12] 为 0x42 时必须使用 secure key，否则 SYMC 可能等待超时。
 */
static void cipherKm_set_secure_key_cfg(km_klad_key_secure_config *pKeySecCfg)
{
    if (NULL == pKeySecCfg)
    {
        return;
    }

    memset(pKeySecCfg, 0, sizeof(km_klad_key_secure_config));
    pKeySecCfg->key_sec = KM_KLAD_SEC_ENABLE;
    pKeySecCfg->master_only_enable = TD_TRUE;
    pKeySecCfg->dest_buf_sec_support = TD_TRUE;
    pKeySecCfg->src_buf_sec_support = TD_TRUE;
    pKeySecCfg->src_buf_non_sec_support = TD_FALSE;
    pKeySecCfg->dest_buf_non_sec_support = TD_FALSE;
}

/**
 * @brief       : 根据 OTP 状态选择 KLAD key 安全属性
 * @author      : zhouzr
 * @param        {km_klad_key_secure_config} *pKeySecCfg：KLAD key 安全属性
 * @return       {void}
 * @note        : OTP 读取失败时降级为 REE 非安全配置，避免没有 OTP 库或未烧写 TEE 标志的平台无法启动。
 */
static void cipherKm_config_key_security(km_klad_key_secure_config *pKeySecCfg)
{
    td_s32 ret = TD_SUCCESS;
    td_u8 u8TeeEnable = 0;

    if (NULL == pKeySecCfg)
    {
        return;
    }

    ret = cipherOtp_read_byte(CIPHER_KM_TEE_ENABLE_OTP_OFFSET, &u8TeeEnable);
    if (ret == TD_SUCCESS && u8TeeEnable == CIPHER_KM_TEE_ENABLE_OTP_VALUE)
    {
        /* note: 海思官方 sample 在 TEE enable fuse 为 0x42 时启用 secure key 配置。 */
        cipherKm_set_secure_key_cfg(pKeySecCfg);
        return;
    }

    /* warn: OTP 读取失败或 fuse 未开启时保留非安全路径，便于同一套代码适配不同平台。 */
    cipherKm_set_non_secure_key_cfg(pKeySecCfg);
    if (ret != TD_SUCCESS)
    {
        mpi_cipher_log("cipherOtp_read_byte offset 0x%02X failed, ret: 0x%08X, fallback to non-secure key cfg",
                       CIPHER_KM_TEE_ENABLE_OTP_OFFSET,
                       (unsigned int) ret);
    }
}

/**
 * @brief       : 按已完成的初始化阶段反向释放 KM 资源
 * @author      : zhouzr
 * @param        {CipherKm_S} *pHandle：句柄
 * @return       {int} TD_SUCCESS：成功，非 TD_SUCCESS：失败
 * @note        : 失败回滚必须尽量释放所有已创建资源，返回首个失败错误码。
 */
static int cipherKm_release_resources(CipherKm_S *pHandle)
{
    td_s32 ret = TD_SUCCESS;
    td_s32 first_ret = TD_SUCCESS;

    if (NULL == pHandle)
    {
        return TD_FAILURE;
    }

    if (pHandle->bKladAttached == TD_TRUE)
    {
        ret = ot_mpi_klad_detach(pHandle->mpi_klad_handle, pHandle->stNeedParam.enKlad_type, pHandle->mpi_keyslot_handle);
        if (ret != TD_SUCCESS && first_ret == TD_SUCCESS)
        {
            first_ret = ret;
        }
        pHandle->bKladAttached = TD_FALSE;
    }

    if (pHandle->bKladCreated == TD_TRUE)
    {
        ret = ot_mpi_klad_destroy(pHandle->mpi_klad_handle);
        if (ret != TD_SUCCESS && first_ret == TD_SUCCESS)
        {
            first_ret = ret;
        }
        pHandle->bKladCreated = TD_FALSE;
        pHandle->mpi_klad_handle = 0;
    }

    if (pHandle->bKeyslotCreated == TD_TRUE)
    {
        ret = ot_mpi_keyslot_destroy(pHandle->mpi_keyslot_handle);
        if (ret != TD_SUCCESS && first_ret == TD_SUCCESS)
        {
            first_ret = ret;
        }
        pHandle->bKeyslotCreated = TD_FALSE;
        pHandle->mpi_keyslot_handle = 0;
    }

    if (pHandle->bKmInited == TD_TRUE)
    {
        ret = ot_mpi_km_deinit();
        if (ret != TD_SUCCESS && first_ret == TD_SUCCESS)
        {
            first_ret = ret;
        }
        pHandle->bKmInited = TD_FALSE;
    }

    return first_ret;
}

/**
 * @brief       : 密钥管理模块初始化
 * @author      : zhouzirui
 * @param        {CipherKm_S} *pHandle：句柄
 * @return       {*}成功返回0,失败返回-1
 */
static int cipherKm_init(CipherKm_S *pHandle)
{
    td_s32 ret = TD_SUCCESS;

    if (NULL == pHandle)
    {
        return TD_FAILURE;
    }

    if (pHandle->bKmInited == TD_TRUE)
    {
        return TD_SUCCESS;
    }

    /*HASH 模块初始化*/
    ret = ot_mpi_km_init();
    if (ret != TD_SUCCESS)
    {
        mpi_cipher_log("ot_mpi_km_init failed, error code: 0x%08X", (unsigned int) ret);
        return ret;
    }
    pHandle->bKmInited = TD_TRUE;

    /*创建KEYSLOT句柄*/
    ret = ot_mpi_keyslot_create(&pHandle->mpi_keyslot_handle, pHandle->stNeedParam.enKeyslot_type);
    if (ret != TD_SUCCESS)
    {
        mpi_cipher_log("ot_mpi_keyslot_create failed, error code: 0x%08X", (unsigned int) ret);
        goto cleanup;
    }
    pHandle->bKeyslotCreated = TD_TRUE;

    /*创建KLAD句柄*/
    ret = ot_mpi_klad_create(&pHandle->mpi_klad_handle);
    if (ret != TD_SUCCESS)
    {
        mpi_cipher_log("ot_mpi_klad_create failed, error code: 0x%08X", (unsigned int) ret);
        goto cleanup;
    }
    pHandle->bKladCreated = TD_TRUE;

    /*绑定KLAD句柄与KEYSLOT通道*/
    ret = ot_mpi_klad_attach(pHandle->mpi_klad_handle, pHandle->stNeedParam.enKlad_type, pHandle->mpi_keyslot_handle);
    if (ret != TD_SUCCESS)
    {
        mpi_cipher_log("ot_mpi_klad_attach failed, error code: 0x%08X", (unsigned int) ret);
        goto cleanup;
    }
    pHandle->bKladAttached = TD_TRUE;
    
    km_klad_attr stKladAttr;
    memset(&stKladAttr, 0, sizeof(km_klad_attr));
    stKladAttr.klad_cfg.rootkey_type = pHandle->stNeedParam.u32RootKey_type;
    stKladAttr.key_cfg.engine = pHandle->stNeedParam.enEngine;
    stKladAttr.key_cfg.decrypt_support = pHandle->stNeedParam.bDecrypt_support;
    stKladAttr.key_cfg.encrypt_support = pHandle->stNeedParam.bEncrypt_support;
    cipherKm_config_key_security(&stKladAttr.key_sec_cfg);

    // stKladAttr.rkp_sw_cfg
    /*设置KLAD句柄属性*/
    ret = ot_mpi_klad_set_attr(pHandle->mpi_klad_handle, &stKladAttr);
    if (ret != TD_SUCCESS)
    {
        mpi_cipher_log("ot_mpi_klad_set_attr failed, error code: 0x%08X", (unsigned int) ret);
        goto cleanup;
    }

    return TD_SUCCESS;

cleanup:
    cipherKm_release_resources(pHandle);
    return ret;
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

    return cipherKm_release_resources(pHandle);
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

/**
 * @brief       : 明文密钥传递，生成工作密钥并存储到密钥槽中
 * @author      : zhouzr
 * @param        {CipherKm_S} *pHandle：句柄
 * @param        {td_u8} *pKey：明文密钥数据
 * @param        {td_u32} u32KeySize：明文密钥长度，SM4 当前使用16字节
 * @return       {*}成功返回0,失败返回-1
 * @note        : 明文 key 只在调用栈内短暂组装，不写入句柄长期保存。
 */
static int cipherKm_set_clear_key(CipherKm_S *pHandle, const td_u8 *pKey, td_u32 u32KeySize)
{
    td_s32 ret = TD_SUCCESS;

    if (NULL == pHandle || NULL == pKey || u32KeySize != 16 || pHandle->bKladCreated != TD_TRUE)
    {
        return TD_FAILURE;
    }

    km_klad_clear_key stClearKey;
    memset(&stClearKey, 0, sizeof(km_klad_clear_key));
    stClearKey.key_size = u32KeySize;
    stClearKey.key = (td_u8 *) pKey;
    stClearKey.key_parity = KM_KLAD_KEY_ODD;

    ret = ot_mpi_klad_set_clear_key(pHandle->mpi_klad_handle, &stClearKey);
    if (ret != TD_SUCCESS)
    {
        mpi_cipher_log("ot_mpi_klad_set_clear_key failed, error code: 0x%08X", (unsigned int) ret);
        return ret;
    }

    /*
     * note: 明文 key 写入 keyslot 后，KLAD 只承担传递职责；参考 SDK 的 hardware_cryptodev/mbedtls
     * 路径都会立即 detach/destroy KLAD，只保留 keyslot 给 SYMC 使用，避免 KLAD 长时间占用路由资源。
     */
    ret = ot_mpi_klad_detach(pHandle->mpi_klad_handle, pHandle->stNeedParam.enKlad_type, pHandle->mpi_keyslot_handle);
    if (ret != TD_SUCCESS)
    {
        mpi_cipher_log("ot_mpi_klad_detach after clear key failed, error code: 0x%08X", (unsigned int) ret);
        return ret;
    }
    pHandle->bKladAttached = TD_FALSE;

    ret = ot_mpi_klad_destroy(pHandle->mpi_klad_handle);
    if (ret != TD_SUCCESS)
    {
        mpi_cipher_log("ot_mpi_klad_destroy after clear key failed, error code: 0x%08X", (unsigned int) ret);
        return ret;
    }
    pHandle->bKladCreated = TD_FALSE;
    pHandle->mpi_klad_handle = 0;

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
    pHandle->cipherKm_set_clear_key         = cipherKm_set_clear_key;

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
