/*
  Copyright (c), 2001-2025, Shenshu Tech. Co., Ltd.
 */

#include "kapi_symc.h"
#include "kapi_inner.h"

#include "drv_symc.h"
#include "drv_symc_outer.h"
#include "crypto_drv_common.h"
#include "kapi_symc_inner.h"

#define CRYPTO_KAPI_DMA_SIZE_ONE_PROCESS    (CONFIG_DRV_AAD_SIZE * CONFIG_SYMC_VIRT_CHN_NUM)
#define CRYPTO_KAPI_TOTAL_DMA_SIZE          (CRYPTO_KAPI_DMA_SIZE_ONE_PROCESS * CONFIG_MAX_PROCESS_NUM)
#define CRYPTO_SYMC_INIT_MAX_NUM            0xffffffff

typedef struct {
    crypto_owner owner;
    crypto_symc_ctrl_t symc_ctrl;
    td_handle keyslot_handle;
    td_u32 processed_length;
    union {
        drv_symc_ccm_ctx ccm_ctx;
        drv_symc_gcm_ctx gcm_ctx;
        crypto_cmac_ctx cmac_ctx;
    };
#if defined(CONFIG_SYMC_CMAC_SUPPORT) || defined(CONFIG_SYMC_CBC_MAC_SUPPORT)
    td_u8 *dma_buf;
    td_u32 dma_buf_len;
    crypto_symc_mac_attr mac_attr;
    unsigned char tail[CRYPTO_AES_BLOCK_SIZE_IN_BYTES];
    unsigned int tail_len;
#endif
#if defined(CONFIG_SYMC_CTR_NON_ALIGN_SUPPORT)
    td_u32 ctr_offset;
    td_u8 ctr_last_block[CRYPTO_AES_BLOCK_SIZE_IN_BYTES];
#endif
    td_u32 is_attached  : 1;
} kapi_symc_context;

static kapi_symc_context *g_kapi_symc_ctx_list[CONFIG_SYMC_VIRT_CHN_NUM];
static crypto_mutex g_symc_mutex;
static td_handle g_drv_symc_handle;

#define kapi_symc_lock() do {                   \
    crypto_log_trace("symc mutext lock");   \
    crypto_mutex_lock(&g_symc_mutex);           \
} while (0)
#define kapi_symc_unlock() do {                 \
    crypto_log_trace("symc mutext unlock");   \
    crypto_mutex_unlock(&g_symc_mutex);         \
} while (0)

static td_s32 inner_kapi_symc_handle_check(td_handle kapi_symc_handle)
{
    crypto_chk_return(kapi_get_module_id(kapi_symc_handle) != KAPI_SYMC_MODULE_ID,
        SYMC_COMPAT_ERRNO(ERROR_INVALID_HANDLE), "Invalid Symc Handle! 0x%x\n", kapi_symc_handle);
    crypto_chk_return(kapi_get_ctx_idx(kapi_symc_handle) >= CONFIG_SYMC_VIRT_CHN_NUM,  \
        SYMC_COMPAT_ERRNO(ERROR_INVALID_HANDLE), "Invalid Symc Handle! 0x%x\n", kapi_symc_handle);
    return CRYPTO_SUCCESS;
}

static kapi_symc_context *inner_get_symc_ctx(td_handle symc_handle)
{
    unsigned int idx = kapi_get_ctx_idx(symc_handle);
    crypto_owner owner  = {0};

    if (inner_kapi_symc_handle_check(symc_handle) != TD_SUCCESS) {
        return NULL;
    }
    if (g_kapi_symc_ctx_list[idx] == NULL) {
        return NULL;
    }
    crypto_get_owner(&owner);
    if (memcmp(&g_kapi_symc_ctx_list[idx]->owner, &owner, sizeof(crypto_owner)) != 0) {
        return NULL;
    }

    return g_kapi_symc_ctx_list[idx];
}

static int inner_kapi_alloc_symc_ctx(td_handle *symc_handle, kapi_symc_context **symc_ctx)
{
    int ret = TD_FAILURE;
    unsigned int i;
    kapi_symc_lock();
    for (i = 0; i < crypto_array_size(g_kapi_symc_ctx_list); i++) {
        if (g_kapi_symc_ctx_list[i] == NULL) {
            break;
        }
    }
    if (i >= crypto_array_size(g_kapi_symc_ctx_list)) {
        ret = SYMC_COMPAT_ERRNO(ERROR_CHN_BUSY);
        goto exit_unlock;
    }
    *symc_ctx = crypto_malloc(sizeof(kapi_symc_context));
    crypto_chk_goto_with_ret(ret, *symc_ctx == NULL, exit_unlock,
        SYMC_COMPAT_ERRNO(ERROR_MALLOC), "crypto_malloc failed\n");

    (void)memset_s(*symc_ctx, sizeof(kapi_symc_context), 0, sizeof(kapi_symc_context));

    g_kapi_symc_ctx_list[i] = *symc_ctx;
    crypto_get_owner(&(g_kapi_symc_ctx_list[i]->owner));
    *symc_handle = synthesize_kapi_handle(KAPI_SYMC_MODULE_ID, i);

    ret = CRYPTO_SUCCESS;
exit_unlock:
    kapi_symc_unlock();
    return ret;
}

static int inner_kapi_free_symc_ctx(td_handle symc_handle)
{
    kapi_symc_context *symc_ctx = NULL;
    unsigned int idx;

    if (inner_kapi_symc_handle_check(symc_handle) != CRYPTO_SUCCESS) {
        return SYMC_COMPAT_ERRNO(ERROR_INVALID_HANDLE);
    }
    idx = kapi_get_ctx_idx(symc_handle);
    symc_ctx = inner_get_symc_ctx(symc_handle);
    crypto_chk_return(symc_ctx == NULL, SYMC_COMPAT_ERRNO(ERROR_INVALID_HANDLE), "inner_get_symc_ctx failed!\n");
    
    kapi_symc_lock();
    if (symc_ctx->dma_buf != NULL) {
        crypto_free_coherent(symc_ctx->dma_buf);
        symc_ctx->dma_buf = NULL;
    }
    crypto_free(symc_ctx);
    g_kapi_symc_ctx_list[idx] = NULL;
    symc_ctx = NULL;
    kapi_symc_unlock();
    return CRYPTO_SUCCESS;
}

td_s32 inner_get_symc_handle(td_handle *symc_handle)
{
    *symc_handle = g_drv_symc_handle;

    return CRYPTO_SUCCESS;
}

td_s32 inner_get_symc_mutex(crypto_mutex *symc_mutex)
{
    symc_mutex->mutex = g_symc_mutex.mutex;
    if (symc_mutex->mutex == NULL) {
        crypto_log_err("symc_mutex->mutex is NULL\n");
        return CRYPTO_FAILURE;
    }

    return CRYPTO_SUCCESS;
}

td_s32 kapi_cipher_symc_env_init(td_void)
{
    td_s32 ret = TD_SUCCESS;
    crypto_symc_attr symc_attr;

    crypto_kapi_func_enter();
    ret = drv_cipher_symc_init();
    crypto_chk_return(ret != TD_SUCCESS, ret, "drv_cipher_symc_init failed\n");

    ret = crypto_mutex_init(&g_symc_mutex);
    crypto_chk_goto_with_ret(ret, ret != TD_SUCCESS, error_symc_deinit, SYMC_COMPAT_ERRNO(ERROR_MUTEX_INIT),
        "crypto_mutex_init failed\n");

    ret = drv_cipher_symc_create(&g_drv_symc_handle, &symc_attr);
    crypto_chk_goto(ret != TD_SUCCESS, error_mutex_destroy, "drv_cipher_symc_create failed\n");

    crypto_kapi_func_exit();
    return ret;

error_mutex_destroy:
    crypto_mutex_destroy(&g_symc_mutex);
error_symc_deinit:
    drv_cipher_symc_deinit();
    return ret;
}

td_s32 kapi_cipher_symc_env_deinit(td_void)
{
    crypto_kapi_func_enter();

    crypto_mutex_destroy(&g_symc_mutex);
    drv_cipher_symc_destroy(g_drv_symc_handle);
    drv_cipher_symc_deinit();
    crypto_kapi_func_exit();
    return TD_SUCCESS;
}

td_s32 kapi_cipher_symc_init(td_void)
{
    crypto_kapi_func_enter();
    crypto_kapi_func_exit();
    return TD_SUCCESS;
}
CRYPTO_EXPORT_SYMBOL(kapi_cipher_symc_init);

td_s32 kapi_cipher_symc_deinit(td_void)
{
    crypto_kapi_func_enter();
    crypto_kapi_func_exit();
    return TD_SUCCESS;
}
CRYPTO_EXPORT_SYMBOL(kapi_cipher_symc_deinit);

td_void kapi_cipher_symc_process_release(td_void)
{
    unsigned int i;
    crypto_owner owner  = {0};
    crypto_get_owner(&owner);
    crypto_kapi_func_enter();
    kapi_symc_lock();
    for (i = 0; i < crypto_array_size(g_kapi_symc_ctx_list); i++) {
        if (g_kapi_symc_ctx_list[i] == NULL) {
            continue;
        }
        if (memcmp(&(g_kapi_symc_ctx_list[i]->owner), &owner, sizeof(crypto_owner)) == 0) {
            if (g_kapi_symc_ctx_list[i]->dma_buf != NULL) {
                crypto_free_coherent(g_kapi_symc_ctx_list[i]->dma_buf);
                g_kapi_symc_ctx_list[i]->dma_buf = NULL;
            }
            crypto_free(g_kapi_symc_ctx_list[i]);
            g_kapi_symc_ctx_list[i] = NULL;
        }
    }

    kapi_symc_unlock();
    crypto_kapi_func_exit();
}

td_s32 kapi_cipher_symc_create(td_handle *kapi_symc_handle, const crypto_symc_attr *symc_attr)
{
    td_s32 ret = TD_SUCCESS;
    kapi_symc_context *symc_ctx = NULL;
    crypto_kapi_func_enter();

    ret = inner_kapi_alloc_symc_ctx(kapi_symc_handle, &symc_ctx);
    crypto_chk_return(ret != CRYPTO_SUCCESS, ret, "inner_kapi_alloc_symc_ctx failed\n");

    crypto_kapi_func_exit();
    return ret;
}
CRYPTO_EXPORT_SYMBOL(kapi_cipher_symc_create);

td_s32 kapi_cipher_symc_destroy(td_handle kapi_symc_handle)
{
    int ret;
    crypto_kapi_func_enter();
    ret = inner_kapi_free_symc_ctx(kapi_symc_handle);
    crypto_kapi_func_exit();
    return ret;
}
CRYPTO_EXPORT_SYMBOL(kapi_cipher_symc_destroy);

#if defined(CONFIG_SYMC_CCM_SUPPORT) || defined(CONFIG_SYMC_GCM_SUPPORT)
static td_s32 inner_kapi_symc_set_ctx(kapi_symc_context *symc_ctx)
{
    int ret;
    crypto_symc_work_mode work_mode = symc_ctx->symc_ctrl.work_mode;

    if (work_mode == CRYPTO_SYMC_WORK_MODE_CCM) {
        ret = inner_drv_symc_ccm_set_ctx(g_drv_symc_handle, &symc_ctx->ccm_ctx);
        crypto_chk_return(ret != TD_SUCCESS, ret, "inner_drv_symc_ccm_set_ctx failed, ret is 0x%x\n", ret);
    } else if (work_mode == CRYPTO_SYMC_WORK_MODE_GCM) {
        ret = inner_drv_symc_gcm_set_ctx(g_drv_symc_handle, &symc_ctx->gcm_ctx);
        crypto_chk_return(ret != TD_SUCCESS, ret, "inner_drv_symc_gcm_set_ctx failed, ret is 0x%x\n", ret);
    }

    return CRYPTO_SUCCESS;
}

static td_s32 inner_kapi_symc_get_ctx(kapi_symc_context *symc_ctx)
{
    int ret;
    crypto_symc_work_mode work_mode = symc_ctx->symc_ctrl.work_mode;

    if (work_mode == CRYPTO_SYMC_WORK_MODE_CCM) {
        ret = inner_drv_symc_ccm_get_ctx(g_drv_symc_handle, &symc_ctx->ccm_ctx);
        crypto_chk_return(ret != TD_SUCCESS, ret, "inner_drv_symc_ccm_get_ctx failed, ret is 0x%x\n", ret);
    } else if (work_mode == CRYPTO_SYMC_WORK_MODE_GCM) {
        ret = inner_drv_symc_gcm_get_ctx(g_drv_symc_handle, &symc_ctx->gcm_ctx);
        crypto_chk_return(ret != TD_SUCCESS, ret, "inner_drv_symc_gcm_get_ctx failed, ret is 0x%x\n", ret);
    }

    return CRYPTO_SUCCESS;
}

static td_s32 inner_kapi_symc_set_config_ex(kapi_symc_context *symc_ctx,
    const crypto_symc_ctrl_t *symc_ctrl)
{
    td_s32 ret;
    symc_null_ptr_chk(symc_ctrl->param);

    ret = drv_cipher_symc_attach(g_drv_symc_handle, symc_ctx->keyslot_handle);
    crypto_chk_return(ret != CRYPTO_SUCCESS, ret, "drv_cipher_symc_attach failed\n");

    if (symc_ctrl->iv_change_flag == CRYPTO_SYMC_CCM_IV_CHANGE_UPDATE ||
        symc_ctrl->iv_change_flag == CRYPTO_SYMC_GCM_IV_CHANGE_UPDATE) {
        ret = inner_kapi_symc_set_ctx(symc_ctx);
        crypto_chk_return(ret != CRYPTO_SUCCESS, ret, "inner_kapi_symc_set_ctx failed\n");
    }

    ret = drv_cipher_symc_set_config(g_drv_symc_handle, symc_ctrl);
    crypto_chk_return(ret != CRYPTO_SUCCESS, ret, "drv_cipher_symc_set_config failed\n");

    ret = inner_kapi_symc_get_ctx(symc_ctx);
    crypto_chk_return(ret != CRYPTO_SUCCESS, ret, "inner_kapi_symc_get_ctx failed\n");

    if (symc_ctrl->iv_change_flag == CRYPTO_SYMC_CCM_IV_CHANGE_START ||
        symc_ctrl->iv_change_flag == CRYPTO_SYMC_GCM_IV_CHANGE_START) {
        ret = inner_drv_symc_get_iv(g_drv_symc_handle, symc_ctx->symc_ctrl.iv, sizeof(symc_ctx->symc_ctrl.iv));
        crypto_chk_return(ret != CRYPTO_SUCCESS, ret, "inner_drv_symc_get_iv failed\n");
    }

    return ret;
}
#else
static td_s32 inner_kapi_symc_set_config_ex(kapi_symc_context *symc_ctx,
    const crypto_symc_ctrl_t *symc_ctrl)
{
    crypto_unused(symc_ctx);
    crypto_unused(symc_ctrl);
    return SYMC_COMPAT_ERRNO(ERROR_UNSUPPORT);
}

static td_s32 inner_kapi_symc_set_ctx(const kapi_symc_context *symc_ctx)
{
    crypto_unused(symc_ctx);

    return CRYPTO_SUCCESS;
}

static td_s32 inner_kapi_symc_get_ctx(kapi_symc_context *symc_ctx)
{
    crypto_unused(symc_ctx);

    return CRYPTO_SUCCESS;
}
#endif

td_s32 kapi_cipher_symc_set_config(td_handle kapi_symc_handle, const crypto_symc_ctrl_t *symc_ctrl)
{
    int ret;
    crypto_symc_work_mode work_mode;
    kapi_symc_context *symc_ctx = NULL;
    crypto_kapi_func_enter();

    symc_null_ptr_chk(symc_ctrl);
    work_mode = symc_ctrl->work_mode;

    kapi_symc_lock();
    symc_ctx = inner_get_symc_ctx(kapi_symc_handle);
    crypto_chk_goto_with_ret(ret, symc_ctx == NULL, exit_unlock,
        SYMC_COMPAT_ERRNO(ERROR_INVALID_HANDLE), "inner_get_symc_ctx failed\n");

    if (work_mode == CRYPTO_SYMC_WORK_MODE_CCM || work_mode == CRYPTO_SYMC_WORK_MODE_GCM) {
        symc_ctx->symc_ctrl.symc_alg = symc_ctrl->symc_alg;
        symc_ctx->symc_ctrl.work_mode = symc_ctrl->work_mode;
        symc_ctx->symc_ctrl.symc_key_length = symc_ctrl->symc_key_length;
        symc_ctx->symc_ctrl.symc_bit_width = symc_ctrl->symc_bit_width;
        ret = inner_kapi_symc_set_config_ex(symc_ctx, symc_ctrl);
        crypto_chk_goto(ret != CRYPTO_SUCCESS, exit_unlock, "inner_kapi_symc_set_config_ex failed\n");
    } else {
        ret = memcpy_s(&symc_ctx->symc_ctrl, sizeof(crypto_symc_ctrl_t), symc_ctrl, sizeof(crypto_symc_ctrl_t));
        crypto_chk_goto_with_ret(ret, ret != EOK, exit_unlock,
            SYMC_COMPAT_ERRNO(ERROR_MEMCPY_S), "memcpy_s failed\n");
        symc_ctx->symc_ctrl.param = NULL;
    }

#if defined(CONFIG_SYMC_CTR_NON_ALIGN_SUPPORT)
    symc_ctx->ctr_offset = 0;
    (void)memset_s(symc_ctx->ctr_last_block, sizeof(symc_ctx->ctr_last_block), 0, sizeof(symc_ctx->ctr_last_block));
#endif
    ret = CRYPTO_SUCCESS;
exit_unlock:
    kapi_symc_unlock();
    crypto_kapi_func_exit();
    return ret;
}
CRYPTO_EXPORT_SYMBOL(kapi_cipher_symc_set_config);

td_s32 kapi_cipher_symc_get_config(td_handle kapi_symc_handle, crypto_symc_ctrl_t *symc_ctrl)
{
    crypto_unused(kapi_symc_handle);
    crypto_unused(symc_ctrl);

    return CRYPTO_SUCCESS;
}
CRYPTO_EXPORT_SYMBOL(kapi_cipher_symc_get_config);

td_s32 kapi_cipher_symc_attach(td_handle kapi_symc_handle, td_handle keyslot_handle)
{
    td_s32 ret = TD_SUCCESS;
    kapi_symc_context *symc_ctx = NULL;
    crypto_kapi_func_enter();

    kapi_symc_lock();
    symc_ctx = inner_get_symc_ctx(kapi_symc_handle);
    crypto_chk_goto_with_ret(ret, symc_ctx == NULL, exit_unlock,
        SYMC_COMPAT_ERRNO(ERROR_INVALID_HANDLE), "inner_get_symc_ctx failed\n");

    symc_ctx->keyslot_handle = keyslot_handle;
    symc_ctx->is_attached = TD_TRUE;

exit_unlock:
    kapi_symc_unlock();
    crypto_kapi_func_exit();
    return ret;
}
CRYPTO_EXPORT_SYMBOL(kapi_cipher_symc_attach);

td_s32 kapi_cipher_symc_detach(td_handle kapi_symc_handle, td_handle keyslot_handle)
{
    td_s32 ret = TD_SUCCESS;
    kapi_symc_context *symc_ctx = NULL;
    crypto_kapi_func_enter();

    kapi_symc_lock();
    symc_ctx = inner_get_symc_ctx(kapi_symc_handle);
    crypto_chk_goto_with_ret(ret, symc_ctx == NULL, exit_unlock,
        SYMC_COMPAT_ERRNO(ERROR_INVALID_HANDLE), "inner_get_symc_ctx failed\n");

    symc_ctx->keyslot_handle = 0xFFFFFFFF;
    symc_ctx->is_attached = TD_FALSE;

exit_unlock:
    kapi_symc_unlock();
    crypto_kapi_func_exit();
    return ret;
}
CRYPTO_EXPORT_SYMBOL(kapi_cipher_symc_detach);

td_s32 inner_kapi_symc_crypto(kapi_symc_context *symc_ctx,
    const crypto_buf_attr *src_buf, const crypto_buf_attr *dst_buf, td_u32 length, td_u32 crypto_type)
{
    td_s32 ret;

    ret = drv_cipher_symc_attach(g_drv_symc_handle, symc_ctx->keyslot_handle);
    crypto_chk_return(ret != CRYPTO_SUCCESS, ret, "drv_cipher_symc_attach failed\n");

    if (symc_ctx->symc_ctrl.work_mode == CRYPTO_SYMC_WORK_MODE_CCM) {
        symc_ctx->symc_ctrl.iv_change_flag = CRYPTO_SYMC_CCM_IV_CHANGE_FINISH;
    } else if (symc_ctx->symc_ctrl.work_mode == CRYPTO_SYMC_WORK_MODE_GCM) {
        symc_ctx->symc_ctrl.iv_change_flag = CRYPTO_SYMC_GCM_IV_CHANGE_FINISH;
    }
    ret = drv_cipher_symc_set_config(g_drv_symc_handle, &symc_ctx->symc_ctrl);
    crypto_chk_return(ret != CRYPTO_SUCCESS, ret, "drv_cipher_symc_attach failed\n");

    ret = inner_drv_symc_set_iv(g_drv_symc_handle, symc_ctx->symc_ctrl.iv, sizeof(symc_ctx->symc_ctrl.iv));
    crypto_chk_return(ret != CRYPTO_SUCCESS, ret, "inner_drv_symc_set_iv failed\n");

    ret = inner_kapi_symc_set_ctx(symc_ctx);
    crypto_chk_return(ret != CRYPTO_SUCCESS, ret, "inner_kapi_symc_set_ctx failed\n");

#if defined(CONFIG_SYMC_CTR_NON_ALIGN_SUPPORT)
    ret = inner_drv_symc_set_ctr_block(g_drv_symc_handle, symc_ctx->ctr_last_block,
        sizeof(symc_ctx->ctr_last_block), symc_ctx->ctr_offset);
    crypto_chk_return(ret != CRYPTO_SUCCESS, ret, "inner_drv_symc_set_ctr_block failed\n");
#endif

    if (crypto_type == CRYPTO_TYPE_ENCRYPT) {
        ret = drv_cipher_symc_encrypt(g_drv_symc_handle, src_buf, dst_buf, length);
    } else {
        ret = drv_cipher_symc_decrypt(g_drv_symc_handle, src_buf, dst_buf, length);
    }
    crypto_chk_return(ret != CRYPTO_SUCCESS, ret, "inner_kapi_cipher_symc_crypto for crypto failed\n");

#if defined(CONFIG_SYMC_CTR_NON_ALIGN_SUPPORT)
    ret = inner_drv_symc_get_ctr_block(g_drv_symc_handle, symc_ctx->ctr_last_block,
        sizeof(symc_ctx->ctr_last_block), &symc_ctx->ctr_offset);
    crypto_chk_return(ret != CRYPTO_SUCCESS, ret, "inner_drv_symc_get_ctr_block failed\n");
#endif

    /* Update iv after crypto. */
    ret = inner_drv_symc_get_iv(g_drv_symc_handle, symc_ctx->symc_ctrl.iv, sizeof(symc_ctx->symc_ctrl.iv));
    crypto_chk_return(ret != CRYPTO_SUCCESS, ret, "inner_drv_symc_get_iv failed\n");

    ret = inner_kapi_symc_get_ctx(symc_ctx);
    crypto_chk_return(ret != CRYPTO_SUCCESS, ret, "inner_kapi_symc_get_ctx failed\n");

    return ret;
}

static td_s32 kapi_cipher_symc_crypto(td_handle kapi_symc_handle, const crypto_buf_attr *src_buf,
    const crypto_buf_attr *dst_buf, td_u32 length, td_u32 crypto_type)
{
    td_s32 ret;
    kapi_symc_context *symc_ctx = NULL;
    crypto_kapi_func_enter();

    kapi_symc_lock();
    symc_ctx = inner_get_symc_ctx(kapi_symc_handle);
    crypto_chk_goto_with_ret(ret, symc_ctx == NULL, exit_unlock,
        SYMC_COMPAT_ERRNO(ERROR_INVALID_HANDLE), "inner_get_symc_ctx failed\n");

    ret = inner_kapi_symc_crypto(symc_ctx, src_buf, dst_buf, length, crypto_type);
    crypto_chk_goto(ret != TD_SUCCESS, exit_unlock, "inner_kapi_symc_crypto failed, ret is 0x%x\n", ret);

    symc_ctx->processed_length += length;

exit_unlock:
    kapi_symc_unlock();
    return ret;
}

td_s32 kapi_cipher_symc_encrypt(td_handle kapi_symc_handle, const crypto_buf_attr *src_buf,
    const crypto_buf_attr *dst_buf, td_u32 length)
{
    td_s32 ret;
    crypto_kapi_func_enter();

    /* Param Check. */
    symc_null_ptr_chk(src_buf);
    symc_null_ptr_chk(dst_buf);
    crypto_chk_return(crypto_data_buf_check(src_buf, length) == TD_FALSE, SYMC_COMPAT_ERRNO(ERROR_MEMORY_ACCESS),
        "src_buf access refused\n");
    crypto_chk_return(crypto_data_buf_check(dst_buf, length) == TD_FALSE, SYMC_COMPAT_ERRNO(ERROR_MEMORY_ACCESS),
        "dst_buf access refused\n");

    ret = kapi_cipher_symc_crypto(kapi_symc_handle, src_buf, dst_buf, length, CRYPTO_TYPE_ENCRYPT);

    crypto_kapi_func_exit();
    return ret;
}
CRYPTO_EXPORT_SYMBOL(kapi_cipher_symc_encrypt);

td_s32 kapi_cipher_symc_decrypt(td_handle kapi_symc_handle, const crypto_buf_attr *src_buf,
    const crypto_buf_attr *dst_buf, td_u32 length)
{
    td_s32 ret;
    crypto_kapi_func_enter();

    /* Param Check. */
    symc_null_ptr_chk(src_buf);
    symc_null_ptr_chk(dst_buf);
    crypto_chk_return(crypto_data_buf_check(src_buf, length) == TD_FALSE, SYMC_COMPAT_ERRNO(ERROR_MEMORY_ACCESS),
        "src_buf access refused\n");
    crypto_chk_return(crypto_data_buf_check(dst_buf, length) == TD_FALSE, SYMC_COMPAT_ERRNO(ERROR_MEMORY_ACCESS),
        "dst_buf access refused\n");

    ret = kapi_cipher_symc_crypto(kapi_symc_handle, src_buf, dst_buf, length, CRYPTO_TYPE_DECRYPT);

    crypto_kapi_func_exit();
    return ret;
}
CRYPTO_EXPORT_SYMBOL(kapi_cipher_symc_decrypt);

#if defined(CONFIG_SYMC_CCM_SUPPORT) || defined(CONFIG_SYMC_GCM_SUPPORT)
td_s32 inner_kapi_symc_get_tag(kapi_symc_context *symc_ctx, td_u8 *tag, td_u32 tag_len)
{
    td_s32 ret;

    ret = drv_cipher_symc_attach(g_drv_symc_handle, symc_ctx->keyslot_handle);
    crypto_chk_return(ret != CRYPTO_SUCCESS, ret, "drv_cipher_symc_attach failed\n");

    ret = drv_cipher_symc_set_config(g_drv_symc_handle, &symc_ctx->symc_ctrl);
    crypto_chk_return(ret != CRYPTO_SUCCESS, ret, "drv_cipher_symc_set_config failed\n");

    ret = inner_kapi_symc_set_ctx(symc_ctx);
    crypto_chk_return(ret != CRYPTO_SUCCESS, ret, "inner_kapi_symc_set_ctx failed\n");

    ret = drv_cipher_symc_get_tag(g_drv_symc_handle, tag, tag_len);
    crypto_chk_return(ret != CRYPTO_SUCCESS, ret, "drv_cipher_symc_get_tag failed\n");

    return ret;
}

td_s32 kapi_cipher_symc_get_tag(td_handle kapi_symc_handle, td_u8 *tag, td_u32 tag_length)
{
    td_s32 ret;
    kapi_symc_context *symc_ctx = TD_NULL;
    crypto_kapi_func_enter();

    symc_null_ptr_chk(tag);
    kapi_symc_lock();
    symc_ctx = inner_get_symc_ctx(kapi_symc_handle);
    crypto_chk_goto_with_ret(ret, symc_ctx == NULL, exit_unlock,
        SYMC_COMPAT_ERRNO(ERROR_INVALID_HANDLE), "inner_get_symc_ctx failed\n");

    if (symc_ctx->symc_ctrl.work_mode == CRYPTO_SYMC_WORK_MODE_CCM) {
        symc_ctx->symc_ctrl.iv_change_flag = CRYPTO_SYMC_CCM_IV_CHANGE_FINISH;
    } else if (symc_ctx->symc_ctrl.work_mode == CRYPTO_SYMC_WORK_MODE_GCM) {
        symc_ctx->symc_ctrl.iv_change_flag = CRYPTO_SYMC_GCM_IV_CHANGE_FINISH;
    }

    ret = inner_kapi_symc_get_tag(symc_ctx, tag, tag_length);
    crypto_chk_goto(ret != TD_SUCCESS, exit_unlock, "inner_kapi_symc_get_tag failed\n");

exit_unlock:
    kapi_symc_unlock();
    crypto_kapi_func_exit();
    return ret;
}
CRYPTO_EXPORT_SYMBOL(kapi_cipher_symc_get_tag);
#endif
#if defined(CONFIG_SYMC_CMAC_SUPPORT) || defined(CONFIG_SYMC_CBC_MAC_SUPPORT)
static td_s32 inner_kapi_mac_process(kapi_symc_context *ctx, td_handle mac_handle, const td_u8 *data, td_u32 data_len);

static td_s32 inner_kapi_mac_update(kapi_symc_context *symc_ctx, const crypto_buf_attr *src_buf, td_u32 length)
{
    td_s32 ret;
    td_handle mac_handle;

    ret = drv_cipher_mac_start(&mac_handle, &symc_ctx->mac_attr);
    crypto_chk_return(ret != TD_SUCCESS, ret, "drv_cipher_mac_start failed, ret is 0x%x\n", ret);

    ret = inner_drv_mac_set_ctx(mac_handle, &symc_ctx->cmac_ctx);
    crypto_chk_goto(ret != TD_SUCCESS, error_symc_destroy, "inner_drv_mac_set_ctx failed, ret is 0x%x\n", ret);

    ret = inner_kapi_mac_process(symc_ctx, mac_handle, src_buf->virt_addr, length);
    crypto_chk_goto(ret != TD_SUCCESS, error_symc_destroy, "inner_kapi_mac_process failed, ret is 0x%x\n", ret);

    if (symc_ctx->mac_attr.work_mode == CRYPTO_SYMC_WORK_MODE_CBC_MAC) {
        symc_ctx->cmac_ctx.mac_len = CRYPTO_AES_BLOCK_SIZE_IN_BYTES;
        ret = drv_cipher_mac_finish(mac_handle, symc_ctx->cmac_ctx.mac, &symc_ctx->cmac_ctx.mac_len);
        crypto_chk_goto(ret != TD_SUCCESS, error_symc_destroy, "drv_cipher_mac_finish failed, ret is 0x%x\n", ret);
    } else {
        ret = inner_drv_mac_get_ctx(mac_handle, &symc_ctx->cmac_ctx);
        crypto_chk_goto(ret != TD_SUCCESS, error_symc_destroy, "inner_drv_mac_get_ctx failed, ret is 0x%x\n", ret);

        (td_void)drv_cipher_symc_destroy(mac_handle);
    }
    return CRYPTO_SUCCESS;
error_symc_destroy:
    (td_void)drv_cipher_symc_destroy(mac_handle);
    return ret;
}

static td_s32 inner_kapi_cmac_finish(kapi_symc_context *symc_ctx)
{
    td_s32 ret;
    crypto_buf_attr src_buf;
    td_handle mac_handle;

    ret = drv_cipher_mac_start(&mac_handle, &symc_ctx->mac_attr);
    crypto_chk_return(ret != TD_SUCCESS, ret, "drv_cipher_mac_start failed, ret is 0x%x\n", ret);

    ret = inner_drv_mac_set_ctx(mac_handle, &symc_ctx->cmac_ctx);
    crypto_chk_goto(ret != TD_SUCCESS, error_symc_destroy, "inner_drv_mac_set_ctx failed, ret is 0x%x\n", ret);

    if (symc_ctx->tail_len != 0) {
        src_buf.virt_addr = symc_ctx->dma_buf;
        ret = memcpy_s(src_buf.virt_addr, symc_ctx->dma_buf_len, symc_ctx->tail, symc_ctx->tail_len);
        crypto_chk_goto_with_ret(ret, ret != EOK, error_symc_destroy, SYMC_COMPAT_ERRNO(ERROR_MEMCPY_S),
            "memcpy_s failed\n");

        ret = drv_cipher_mac_update(mac_handle, &src_buf, symc_ctx->tail_len);
        crypto_chk_goto(ret != TD_SUCCESS, error_symc_destroy, "drv_cipher_mac_update failed\n");
    }

    symc_ctx->cmac_ctx.mac_len = CRYPTO_AES_BLOCK_SIZE_IN_BYTES;
    ret = drv_cipher_mac_finish(mac_handle, symc_ctx->cmac_ctx.mac, &symc_ctx->cmac_ctx.mac_len);
    crypto_chk_goto(ret != TD_SUCCESS, error_symc_destroy, "drv_cipher_mac_update failed, ret is 0x%x\n", ret);

    return ret;
error_symc_destroy:
    (td_void)drv_cipher_symc_destroy(mac_handle);
    return ret;
}

static td_s32 inner_kapi_mac_finish(kapi_symc_context *symc_ctx, td_u8 *mac, td_u32 *mac_length)
{
    td_s32 ret;

    if (symc_ctx->mac_attr.work_mode == CRYPTO_SYMC_WORK_MODE_CMAC) {
        ret = inner_kapi_cmac_finish(symc_ctx);
        crypto_chk_return(ret != TD_SUCCESS, ret, "inner_kapi_cmac_finish failed\n");
    }

    ret = memcpy_s(mac, *mac_length, symc_ctx->cmac_ctx.mac, symc_ctx->cmac_ctx.mac_len);
    crypto_chk_return(ret != EOK, SYMC_COMPAT_ERRNO(ERROR_MEMCPY_S), "memcpy_s failed\n");

    return ret;
}

td_s32 kapi_cipher_mac_start(td_handle *kapi_symc_handle, const crypto_symc_mac_attr *mac_attr)
{
    td_s32 ret = TD_SUCCESS;
    kapi_symc_context *symc_ctx = NULL;
    crypto_kapi_func_enter();

    symc_null_ptr_chk(kapi_symc_handle);
    symc_null_ptr_chk(mac_attr);

    ret = inner_kapi_alloc_symc_ctx(kapi_symc_handle, &symc_ctx);
    crypto_chk_return(ret != CRYPTO_SUCCESS, ret, "inner_kapi_alloc_symc_ctx failed\n");

    symc_ctx->dma_buf = crypto_malloc_mmz(CONFIG_SYMC_KAPI_DMA_BUF_LEN, "symc_dma_buf");
    crypto_chk_goto_with_ret(ret, symc_ctx->dma_buf == TD_NULL, error_free_ctx, SYMC_COMPAT_ERRNO(ERROR_MALLOC),
        "crypto_malloc_mmz failed\n");
    symc_ctx->dma_buf_len = CONFIG_SYMC_KAPI_DMA_BUF_LEN;

    ret = memcpy_s(&symc_ctx->mac_attr, sizeof(crypto_symc_mac_attr), mac_attr, sizeof(crypto_symc_mac_attr));
    crypto_chk_goto_with_ret(ret, ret != EOK, error_free, SYMC_COMPAT_ERRNO(ERROR_MEMCPY_S),
        "crypto_malloc_mmz failed\n");

    crypto_kapi_func_exit();
    return CRYPTO_SUCCESS;
error_free:
    crypto_free_coherent(symc_ctx->dma_buf);
    symc_ctx->dma_buf = TD_NULL;
error_free_ctx:
    inner_kapi_free_symc_ctx(*kapi_symc_handle);
    crypto_kapi_func_exit();
    return ret;
}
CRYPTO_EXPORT_SYMBOL(kapi_cipher_mac_start);

static td_s32 inner_kapi_mac_process(kapi_symc_context *ctx, td_handle mac_handle, const td_u8 *data, td_u32 data_len)
{
    td_s32 ret;
    td_u8 *dma_buf = ctx->dma_buf;
    td_u32 dma_buf_len = ctx->dma_buf_len;
    td_u8 *tail = ctx->tail;
    td_u32 tail_len = ctx->tail_len;
    td_u32 block_size = sizeof(ctx->tail);
    td_u32 processing_len = 0;
    td_u32 processed_len = 0;
    td_u32 left = data_len;
    crypto_buf_attr src_buf;

    src_buf.virt_addr = dma_buf;
    if ((tail_len + data_len) < block_size) {
        ret = memcpy_s(tail + tail_len, block_size - tail_len, data, data_len);
        crypto_chk_return(ret != EOK, SYMC_COMPAT_ERRNO(ERROR_MEMCPY_S), "memcpy_s failed\n");
        ctx->tail_len += data_len;
        return CRYPTO_SUCCESS;
    }
    if (tail_len != 0) {
        processing_len = block_size - tail_len;
        ret = memcpy_s(tail + tail_len, processing_len, data, processing_len);
        crypto_chk_return(ret != EOK, SYMC_COMPAT_ERRNO(ERROR_MEMCPY_S), "memcpy_s failed\n");
        ret = memcpy_s(dma_buf, block_size, tail, block_size);
        crypto_chk_return(ret != EOK, SYMC_COMPAT_ERRNO(ERROR_MEMCPY_S), "memcpy_s failed\n");

        ret = drv_cipher_mac_update(mac_handle, &src_buf, block_size);
        crypto_chk_return(ret != TD_SUCCESS, ret, "drv_cipher_mac_update failed\n");

        left -= processing_len;
        processed_len += processing_len;
    }
    while (left >= block_size) {
        if (left >= dma_buf_len) {
            processing_len = dma_buf_len;
        } else {
            processing_len = left - left % block_size;
        }
        ret = memcpy_s(dma_buf, dma_buf_len, data + processed_len, processing_len);
        crypto_chk_return(ret != EOK, SYMC_COMPAT_ERRNO(ERROR_MEMCPY_S), "memcpy_s failed\n");

        ret = drv_cipher_mac_update(mac_handle, &src_buf, processing_len);
        crypto_chk_return(ret != TD_SUCCESS, ret, "drv_cipher_mac_update failed\n");

        left -= processing_len;
        processed_len += processing_len;
    }
    if (left != 0) {
        ret = memcpy_s(tail, block_size, data + processed_len, left);
        crypto_chk_return(ret != EOK, SYMC_COMPAT_ERRNO(ERROR_MEMCPY_S), "memcpy_s failed\n");
    }
    ctx->tail_len = left;

    return ret;
}

td_s32 kapi_cipher_mac_update(td_handle kapi_symc_handle, const crypto_buf_attr *src_buf, td_u32 length)
{
    td_s32 ret;
    kapi_symc_context *symc_ctx = NULL;
    crypto_kapi_func_enter();

    kapi_symc_lock();
    symc_ctx = inner_get_symc_ctx(kapi_symc_handle);
    crypto_chk_goto_with_ret(ret, symc_ctx == NULL, exit_unlock,
        SYMC_COMPAT_ERRNO(ERROR_INVALID_HANDLE), "inner_get_symc_ctx failed\n");

    ret = inner_kapi_mac_update(symc_ctx, src_buf, length);
    crypto_chk_goto(ret != TD_SUCCESS, exit_unlock, "inner_kapi_mac_update failed, ret is 0x%x\n", ret);

exit_unlock:
    kapi_symc_unlock();
    crypto_kapi_func_exit();
    return ret;
}
CRYPTO_EXPORT_SYMBOL(kapi_cipher_mac_update);

td_s32 kapi_cipher_mac_finish(td_handle kapi_symc_handle, td_u8 *mac, td_u32 *mac_length)
{
    td_s32 ret;
    kapi_symc_context *symc_ctx = NULL;
    crypto_kapi_func_enter();

    symc_null_ptr_chk(mac);
    symc_null_ptr_chk(mac_length);

    crypto_chk_return(*mac_length < CRYPTO_AES_BLOCK_SIZE_IN_BYTES, SYMC_COMPAT_ERRNO(ERROR_INVALID_PARAM),
        "mac_length is not enough\n");

    kapi_symc_lock();
    symc_ctx = inner_get_symc_ctx(kapi_symc_handle);
    crypto_chk_goto_with_ret(ret, symc_ctx == NULL, exit_unlock,
        SYMC_COMPAT_ERRNO(ERROR_INVALID_HANDLE), "inner_get_symc_ctx failed\n");

    ret = inner_kapi_mac_finish(symc_ctx, mac, mac_length);
    crypto_chk_goto(ret != TD_SUCCESS, exit_unlock, "inner_kapi_mac_finish failed, ret is 0x%x\n", ret);

    if (symc_ctx->dma_buf != TD_NULL) {
        crypto_free_coherent(symc_ctx->dma_buf);
        symc_ctx->dma_buf = TD_NULL;
    }
exit_unlock:
    kapi_symc_unlock();
    inner_kapi_free_symc_ctx(kapi_symc_handle);
    crypto_kapi_func_exit();
    return ret;
}
CRYPTO_EXPORT_SYMBOL(kapi_cipher_mac_finish);
#endif