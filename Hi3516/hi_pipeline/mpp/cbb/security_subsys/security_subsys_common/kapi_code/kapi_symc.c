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

static td_u8 *g_kapi_dma_addr = TD_NULL;

static crypto_kapi_symc_process g_kapi_symc_channel[CONFIG_MAX_PROCESS_NUM];

static crypto_mutex g_symc_mutex;

void kapi_symc_lock(void)
{
    crypto_mutex_lock(&g_symc_mutex);
}

void kapi_symc_unlock(void)
{
    crypto_mutex_unlock(&g_symc_mutex);
}

static td_s32 priv_symc_handle_check(td_handle kapi_symc_handle)
{
    crypto_chk_return(kapi_get_module_id(kapi_symc_handle) != KAPI_SYMC_MODULE_ID,
        SYMC_COMPAT_ERRNO(ERROR_INVALID_HANDLE), "Invalid Symc Handle! 0x%x\n", kapi_symc_handle);
    crypto_chk_return(kapi_get_ctx_idx(kapi_symc_handle) >= CONFIG_SYMC_VIRT_CHN_NUM,  \
        SYMC_COMPAT_ERRNO(ERROR_INVALID_HANDLE), "Invalid Symc Handle! 0x%x\n", kapi_symc_handle);
    return TD_SUCCESS;
}

crypto_kapi_symc_process *inner_get_current_symc_channel(td_void)
{
    td_u32 i;
    crypto_owner owner = {0};
    if (crypto_get_owner(&owner) != CRYPTO_SUCCESS) {
        return TD_NULL;
    }
    for (i = 0; i < CONFIG_MAX_PROCESS_NUM; i++) {
        if (memcmp(&owner, &g_kapi_symc_channel[i].owner, sizeof(crypto_owner)) == 0) {
            return &g_kapi_symc_channel[i];
        }
    }
    return TD_NULL;
}

crypto_kapi_symc_ctx *priv_occupy_symc_soft_chn(crypto_kapi_symc_process *symc_channel, td_u32 *idx)
{
    td_u32 i;
    td_u32 tid = crypto_gettid();
    crypto_kapi_symc_ctx *symc_ctx_list = TD_NULL;
    crypto_kapi_symc_ctx *symc_ctx = TD_NULL;

    kapi_symc_lock();

    symc_ctx_list = symc_channel->symc_ctx_list;
    for (i = 0; i < CONFIG_SYMC_VIRT_CHN_NUM; i++) {
        if (symc_ctx_list[i].is_open == TD_FALSE) {
            break;
        }
    }
    if (i >= CONFIG_SYMC_VIRT_CHN_NUM) {
        crypto_log_err("All Symc Channels are busy!\n");
        goto exit_unlock;
    }
    symc_ctx = &symc_channel->symc_ctx_list[i];
    (td_void)memset_s(symc_ctx, sizeof(crypto_kapi_symc_ctx), 0, sizeof(crypto_kapi_symc_ctx));
    symc_ctx->is_open = TD_TRUE;
    symc_ctx->tid = tid;

    *idx = i;

exit_unlock:
    kapi_symc_unlock();
    return symc_ctx;
}

td_void priv_release_symc_soft_chn(crypto_kapi_symc_ctx *symc_ctx)
{
    kapi_symc_lock();
    (td_void)memset_s(symc_ctx, sizeof(crypto_kapi_symc_ctx), 0, sizeof(crypto_kapi_symc_ctx));
    kapi_symc_unlock();
}

static td_bool priv_check_is_init(crypto_owner *owner)
{
    td_u32 i;
    for (i = 0; i < CONFIG_MAX_PROCESS_NUM; i++) {
        if (memcmp(owner, &g_kapi_symc_channel[i].owner, sizeof(crypto_owner)) == 0) {
            return TD_TRUE;
        }
    }
    return TD_FALSE;
}

static td_s32 priv_process_symc_init(td_void)
{
    td_u32 i;
    td_u32 ret = TD_SUCCESS;
    crypto_owner owner = {0};
    crypto_kapi_symc_process *symc_channel = TD_NULL;

    crypto_kapi_func_enter();
    kapi_symc_lock();
    ret = crypto_get_owner(&owner);
    crypto_chk_goto_with_ret(ret, ret != CRYPTO_SUCCESS, exit_unlock, SYMC_COMPAT_ERRNO(ERROR_GET_OWNER),
        "crypto_get_owner failed\n");
    if (priv_check_is_init(&owner) == TD_TRUE) {
        symc_channel = inner_get_current_symc_channel();
        if (symc_channel->init_counter >= CRYPTO_SYMC_INIT_MAX_NUM) {
            ret = SYMC_COMPAT_ERRNO(ERROR_COUNT_OVERFLOW);
        } else {
            ret = TD_SUCCESS;
            ++(symc_channel->init_counter);
        }
        goto exit_unlock;
    }
    for (i = 0; i < CONFIG_MAX_PROCESS_NUM; i++) {
        if (g_kapi_symc_channel[i].is_used == TD_FALSE) {
            break;
        }
    }
    if (i >= CONFIG_MAX_PROCESS_NUM) {
        crypto_log_err("Process Num is More Than %u\n", CONFIG_MAX_PROCESS_NUM);
        ret = SYMC_COMPAT_ERRNO(ERROR_MAX_PROCESS);
        goto exit_unlock;
    }
    symc_channel = &g_kapi_symc_channel[i];
    (td_void)memset_s(symc_channel, sizeof(crypto_kapi_symc_process), 0, sizeof(crypto_kapi_symc_process));
    /* Alloc dma memory. */
    symc_channel->dma_addr = g_kapi_dma_addr + i * CRYPTO_KAPI_DMA_SIZE_ONE_PROCESS;
    for (i = 0; i < CONFIG_SYMC_VIRT_CHN_NUM; ++i) {
        ret = crypto_mutex_init(&symc_channel->symc_ctx_mutex[i]);
        crypto_chk_goto_with_ret(ret, ret != TD_SUCCESS, exit_destroy, SYMC_COMPAT_ERRNO(ERROR_MUTEX_INIT),
            "symc ctx mutex init failed at chn: %u\n", i);
    }
    (td_void)memcpy_s(&symc_channel->owner, sizeof(crypto_owner), &owner, sizeof(crypto_owner));
    symc_channel->is_used = TD_TRUE;
    symc_channel->ctx_num = CONFIG_SYMC_VIRT_CHN_NUM;
    ++(symc_channel->init_counter);
    (td_void)memset_s(symc_channel->symc_ctx_list, sizeof(crypto_kapi_symc_ctx) * CONFIG_SYMC_VIRT_CHN_NUM,
        0, sizeof(crypto_kapi_symc_ctx) * CONFIG_SYMC_VIRT_CHN_NUM);
    kapi_symc_unlock();

    crypto_kapi_func_exit();
    return ret;

exit_destroy:
    for (i = 0; i < CONFIG_SYMC_VIRT_CHN_NUM; ++i) {
        crypto_mutex_destroy(&symc_channel->symc_ctx_mutex[i]);
    }
exit_unlock:
    kapi_symc_unlock();
    crypto_kapi_func_exit();
    return ret;
}

static td_void priv_process_release(crypto_kapi_symc_process *symc_channel)
{
    td_u32 i;
    for (i = 0; i < CONFIG_SYMC_VIRT_CHN_NUM; ++i) {
        crypto_mutex_destroy(&symc_channel->symc_ctx_mutex[i]);
        if (symc_channel->symc_ctx_list[i].is_open == TD_TRUE &&
            symc_channel->symc_ctx_list[i].symc_attr.is_long_term == TD_TRUE) {
            (td_void)drv_cipher_symc_destroy(symc_channel->symc_ctx_list[i].drv_symc_handle);
        }
    }
    (td_void)memset_s(symc_channel, sizeof(crypto_kapi_symc_process), 0, sizeof(crypto_kapi_symc_process));
}

static td_void priv_process_symc_deinit(td_void)
{
    crypto_kapi_symc_process *symc_channel = TD_NULL;
    crypto_kapi_func_enter();
    kapi_symc_lock();
    symc_channel = inner_get_current_symc_channel();
    if (symc_channel == TD_NULL) {
        kapi_symc_unlock();
        return;
    }
    if (symc_channel->init_counter > 1) {
        --(symc_channel->init_counter);
        kapi_symc_unlock();
        return;
    }
    priv_process_release(symc_channel);
    kapi_symc_unlock();
    crypto_kapi_func_exit();
}

crypto_kapi_symc_ctx *inner_kapi_get_symc_ctx(td_handle symc_handle, td_u32 *idx)
{
    td_s32 ret;
    crypto_kapi_symc_process *symc_channel = TD_NULL;

    ret =  priv_symc_handle_check(symc_handle);
    crypto_chk_return(ret != TD_SUCCESS, NULL, "priv_symc_handle_check failed, ret is 0x%x\n", ret);

    *idx = kapi_get_ctx_idx(symc_handle);
    symc_channel = inner_get_current_symc_channel();
    crypto_chk_return(symc_channel == TD_NULL, NULL, "inner_get_current_symc_channel failed\n");

    return &symc_channel->symc_ctx_list[*idx];
}

td_void kapi_cipher_symc_process_release(td_void)
{
    crypto_kapi_symc_process *symc_channel = TD_NULL;
    crypto_kapi_func_enter();
    kapi_symc_lock();
    symc_channel = inner_get_current_symc_channel();
    if (symc_channel == TD_NULL) {
        kapi_symc_unlock();
        return;
    }
    priv_process_release(symc_channel);
    kapi_symc_unlock();
    crypto_kapi_func_exit();
}

td_s32 kapi_cipher_symc_env_init(td_void)
{
    td_s32 ret = TD_SUCCESS;
    crypto_kapi_func_enter();
    ret = drv_cipher_symc_init();
    if (ret != TD_SUCCESS) {
        crypto_log_err("drv_cipher_symc_init failed\n");
        return ret;
    }

    (td_void)memset_s(&g_kapi_symc_channel, sizeof(g_kapi_symc_channel), 0, sizeof(g_kapi_symc_channel));
    ret = crypto_mutex_init(&g_symc_mutex);
    if (ret != TD_SUCCESS) {
        crypto_log_err("crypto_mutex_init failed\n");
        ret = SYMC_COMPAT_ERRNO(ERROR_MUTEX_INIT);
        goto error_symc_deinit;
    }

    g_kapi_dma_addr = crypto_malloc_coherent(CRYPTO_KAPI_TOTAL_DMA_SIZE, "crypto_kapi_symc_dma_buffer");
    crypto_chk_goto_with_ret(ret, g_kapi_dma_addr == TD_NULL, error_mutex_destroy, SYMC_COMPAT_ERRNO(ERROR_MALLOC),
        "crypto_malloc_coherent failed\n");

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
    td_u32 i, j;
    crypto_kapi_symc_process *symc_channel = TD_NULL;
    crypto_kapi_symc_ctx *symc_ctx = TD_NULL;
    crypto_kapi_func_enter();

    if (g_kapi_dma_addr != TD_NULL) {
        (td_void)memset_s(g_kapi_dma_addr, CRYPTO_KAPI_TOTAL_DMA_SIZE, 0, CRYPTO_KAPI_TOTAL_DMA_SIZE);
        crypto_free_coherent(g_kapi_dma_addr);
        g_kapi_dma_addr = TD_NULL;
    }

    for (i = 0; i < CONFIG_MAX_PROCESS_NUM; i++) {
        symc_channel = &g_kapi_symc_channel[i];
        if (symc_channel->is_used == TD_FALSE) {
            continue;
        }
        for (j = 0; j < CONFIG_SYMC_VIRT_CHN_NUM; j++) {
            symc_ctx = &symc_channel->symc_ctx_list[j];
            if (symc_ctx->is_open == TD_FALSE) {
                continue;
            }
            priv_release_symc_soft_chn(symc_ctx);
            symc_ctx->is_open = TD_FALSE;
        }
        symc_channel->is_used = TD_FALSE;
    }
    crypto_mutex_destroy(&g_symc_mutex);
    (td_void)memset_s(&g_kapi_symc_channel, sizeof(g_kapi_symc_channel), 0, sizeof(g_kapi_symc_channel));
    drv_cipher_symc_deinit();
    crypto_kapi_func_exit();
    return TD_SUCCESS;
}

td_s32 kapi_cipher_symc_init(td_void)
{
    td_s32 ret = TD_SUCCESS;
    crypto_kapi_func_enter();

    ret = priv_process_symc_init();
    if (ret != TD_SUCCESS) {
        crypto_log_err("symc priv_process_symc_init failed\n");
        return ret;
    }

    crypto_kapi_func_exit();
    return ret;
}
CRYPTO_EXPORT_SYMBOL(kapi_cipher_symc_init);

td_s32 kapi_cipher_symc_deinit(td_void)
{
    crypto_kapi_func_enter();
    priv_process_symc_deinit();
    crypto_kapi_func_exit();
    return TD_SUCCESS;
}
CRYPTO_EXPORT_SYMBOL(kapi_cipher_symc_deinit);

td_s32 kapi_cipher_symc_create(td_handle *kapi_symc_handle, const crypto_symc_attr *symc_attr)
{
    td_s32 ret = TD_SUCCESS;
    crypto_kapi_symc_process *symc_channel = TD_NULL;
    crypto_kapi_symc_ctx *symc_ctx = TD_NULL;
    td_u32 idx;

    crypto_kapi_func_enter();
    symc_null_ptr_chk(kapi_symc_handle);
    symc_null_ptr_chk(symc_attr);

    symc_channel = inner_get_current_symc_channel();
    crypto_chk_return(symc_channel == TD_NULL, SYMC_COMPAT_ERRNO(ERROR_NOT_INIT), "call init first!\n");

    symc_ctx = priv_occupy_symc_soft_chn(symc_channel, &idx);
    crypto_chk_return(symc_ctx == TD_NULL, SYMC_COMPAT_ERRNO(ERROR_CHN_BUSY), "all symc soft chns are busy\n");
    crypto_chk_return(symc_ctx->is_open == TD_FALSE, SYMC_COMPAT_ERRNO(ERROR_CTX_CLOSED), "ctx is closed\n");

    kapi_symc_lock();

    if (symc_attr->is_long_term == TD_TRUE) {
        ret = inner_kapi_symc_create_long_term(symc_ctx, symc_attr);
        crypto_chk_goto(ret != TD_SUCCESS, error_unlock_symc_ctx,
            "inner_kapi_symc_create_long_term failed, ret is 0x%x\n", ret);
    } else {
        ret = inner_kapi_symc_create_short_term(symc_ctx, symc_attr);
        crypto_chk_goto(ret != TD_SUCCESS, error_unlock_symc_ctx,
            "inner_kapi_symc_create_short_term failed, ret is 0x%x\n", ret);
    }

    kapi_symc_unlock();
    *kapi_symc_handle = synthesize_kapi_handle(KAPI_SYMC_MODULE_ID, idx);
    crypto_kapi_func_exit();
    return ret;

error_unlock_symc_ctx:
    kapi_symc_unlock();
    priv_release_symc_soft_chn(symc_ctx);
    crypto_kapi_func_exit();
    return ret;
}
CRYPTO_EXPORT_SYMBOL(kapi_cipher_symc_create);

td_s32 kapi_cipher_symc_destroy(td_handle kapi_symc_handle)
{
    td_s32 ret = TD_SUCCESS;
    td_u32 idx = 0;
    crypto_kapi_symc_process *symc_channel = TD_NULL;
    crypto_kapi_symc_ctx *symc_ctx = TD_NULL;
    crypto_kapi_func_enter();

    symc_channel = inner_get_current_symc_channel();
    crypto_chk_return(symc_channel == TD_NULL, SYMC_COMPAT_ERRNO(ERROR_NOT_INIT), "call init first!\n");

    symc_ctx = inner_kapi_get_symc_ctx(kapi_symc_handle, &idx);
    crypto_chk_return(symc_ctx == TD_NULL, SYMC_COMPAT_ERRNO(ERROR_INVALID_HANDLE), "handle is invalid!\n");
    crypto_chk_return(symc_ctx->is_open == TD_FALSE, SYMC_COMPAT_ERRNO(ERROR_CTX_CLOSED), "ctx is closed\n");

    kapi_symc_lock();
    if (symc_ctx->symc_attr.is_long_term == TD_TRUE) {
        (td_void)drv_cipher_symc_destroy(symc_ctx->drv_symc_handle);
    }
    kapi_symc_unlock();
    priv_release_symc_soft_chn(symc_ctx);
    crypto_kapi_func_exit();
    return ret;
}
CRYPTO_EXPORT_SYMBOL(kapi_cipher_symc_destroy);

td_s32 kapi_cipher_symc_set_config(td_handle kapi_symc_handle, const crypto_symc_ctrl_t *symc_ctrl)
{
    td_s32 ret;
    td_u32 idx = 0;
    crypto_kapi_symc_process *symc_channel = TD_NULL;
    crypto_kapi_symc_ctx *symc_ctx = TD_NULL;
    crypto_kapi_func_enter();

    symc_null_ptr_chk(symc_ctrl);

    symc_channel = inner_get_current_symc_channel();
    crypto_chk_return(symc_channel == TD_NULL, SYMC_COMPAT_ERRNO(ERROR_NOT_INIT), "call init first!\n");

    symc_ctx = inner_kapi_get_symc_ctx(kapi_symc_handle, &idx);
    crypto_chk_return(symc_ctx == TD_NULL, SYMC_COMPAT_ERRNO(ERROR_NOT_INIT), "call init first!\n");
    crypto_chk_return(symc_ctx->is_open == TD_FALSE, SYMC_COMPAT_ERRNO(ERROR_CTX_CLOSED), "ctx is closed\n");

    kapi_symc_lock();

    if (symc_ctx->symc_attr.is_long_term == TD_TRUE) {
        ret = inner_kapi_symc_set_config_long_term(symc_ctx, symc_ctrl);
        crypto_chk_goto(ret != TD_SUCCESS, unlock_exit, "inner_kapi_symc_set_config_long_term failed\n");
    } else {
        ret = inner_kapi_symc_set_config_short_term(symc_ctx, symc_ctrl);
        crypto_chk_goto(ret != TD_SUCCESS, unlock_exit, "inner_kapi_symc_set_config_short_term failed\n");
    }
    symc_ctx->is_set_config = TD_TRUE;

unlock_exit:
    kapi_symc_unlock();
    crypto_kapi_func_exit();
    return ret;
}
CRYPTO_EXPORT_SYMBOL(kapi_cipher_symc_set_config);

td_s32 kapi_cipher_symc_get_config(td_handle kapi_symc_handle, crypto_symc_ctrl_t *symc_ctrl)
{
    td_s32 ret;
    td_u32 idx = 0;
    crypto_kapi_symc_process *symc_channel = TD_NULL;
    crypto_kapi_symc_ctx *symc_ctx = TD_NULL;
    crypto_kapi_func_enter();

    symc_null_ptr_chk(symc_ctrl);

    symc_channel = inner_get_current_symc_channel();
    crypto_chk_return(symc_channel == TD_NULL, SYMC_COMPAT_ERRNO(ERROR_NOT_INIT), "call init first!\n");

    symc_ctx = inner_kapi_get_symc_ctx(kapi_symc_handle, &idx);
    crypto_chk_return(symc_ctx == TD_NULL, SYMC_COMPAT_ERRNO(ERROR_NOT_INIT), "call init first!\n");
    crypto_chk_return(symc_ctx->is_open == TD_FALSE, SYMC_COMPAT_ERRNO(ERROR_CTX_CLOSED), "ctx is closed\n");
    crypto_chk_return(symc_ctx->is_set_config == TD_FALSE, SYMC_COMPAT_ERRNO(ERROR_NOT_SET_CONFIG),
        "set config first\n");

    kapi_symc_lock();

    if (symc_ctx->symc_attr.is_long_term == TD_TRUE) {
        ret = inner_kapi_symc_get_config_long_term(symc_ctx, symc_ctrl);
        crypto_chk_goto(ret != TD_SUCCESS, unlock_exit, "inner_kapi_symc_get_config_long_term failed\n");
    } else {
        ret = inner_kapi_symc_get_config_short_term(symc_ctx, symc_ctrl);
        crypto_chk_goto(ret != TD_SUCCESS, unlock_exit, "inner_kapi_symc_get_config_short_term failed\n");
    }
    (td_void)memcpy_s(symc_ctrl, sizeof(crypto_symc_ctrl_t), &symc_ctx->symc_ctrl, sizeof(crypto_symc_ctrl_t));
    if (symc_ctrl->work_mode == CRYPTO_SYMC_WORK_MODE_CCM || symc_ctrl->work_mode == CRYPTO_SYMC_WORK_MODE_GCM) {
        crypto_chk_goto_with_ret(ret, symc_ctrl->param == TD_NULL, unlock_exit, SYMC_COMPAT_ERRNO(ERROR_PARAM_IS_NULL),
            "symc_ctrl->param is null\n");
        (td_void)memcpy_s(symc_ctrl->param, sizeof(crypto_symc_config_aes_ccm_gcm),
            &symc_ctx->ccm_gcm_config, sizeof(crypto_symc_config_aes_ccm_gcm));
    }

unlock_exit:
    kapi_symc_unlock();
    crypto_kapi_func_exit();
    return ret;
}
CRYPTO_EXPORT_SYMBOL(kapi_cipher_symc_get_config);

td_s32 kapi_cipher_symc_attach(td_handle kapi_symc_handle, td_handle keyslot_handle)
{
    td_s32 ret = TD_SUCCESS;
    td_u32 idx = 0;
    crypto_kapi_symc_process *symc_channel = TD_NULL;
    crypto_kapi_symc_ctx *symc_ctx = TD_NULL;
    crypto_kapi_func_enter();

    symc_channel = inner_get_current_symc_channel();
    crypto_chk_return(symc_channel == TD_NULL, SYMC_COMPAT_ERRNO(ERROR_NOT_INIT), "call init first!\n");

    symc_ctx = inner_kapi_get_symc_ctx(kapi_symc_handle, &idx);
    crypto_chk_return(symc_ctx == TD_NULL, SYMC_COMPAT_ERRNO(ERROR_NOT_INIT), "call init first!\n");
    crypto_chk_return(symc_ctx->is_open == TD_FALSE, SYMC_COMPAT_ERRNO(ERROR_CTX_CLOSED), "ctx is closed\n");

    kapi_symc_lock();

    if (symc_ctx->symc_attr.is_long_term == TD_TRUE) {
        ret = inner_kapi_symc_attach_long_term(symc_ctx, keyslot_handle);
        crypto_chk_goto(ret != TD_SUCCESS, unlock_exit, "inner_kapi_symc_attach_long_term failed, ret is 0x%x\n", ret);
    } else {
        ret = inner_kapi_symc_attach_short_term(symc_ctx, keyslot_handle);
        crypto_chk_goto(ret != TD_SUCCESS, unlock_exit, "inner_kapi_symc_attach_short_term failed, ret is 0x%x\n", ret);
    }

    symc_ctx->is_attached = TD_TRUE;

unlock_exit:
    kapi_symc_unlock();
    crypto_kapi_func_exit();
    return ret;
}
CRYPTO_EXPORT_SYMBOL(kapi_cipher_symc_attach);

#if defined(CONFIG_SYMC_DETACH_SUPPORT)
td_s32 kapi_cipher_symc_detach(td_handle kapi_symc_handle, td_handle keyslot_handle)
{
    td_s32 ret = TD_SUCCESS;
    td_u32 idx = 0;
    crypto_kapi_symc_process *symc_channel = TD_NULL;
    crypto_kapi_symc_ctx *symc_ctx = TD_NULL;
    crypto_kapi_func_enter();

    crypto_unused(keyslot_handle);

    symc_channel = inner_get_current_symc_channel();
    crypto_chk_return(symc_channel == TD_NULL, SYMC_COMPAT_ERRNO(ERROR_NOT_INIT), "call init first!\n");

    symc_ctx = inner_kapi_get_symc_ctx(kapi_symc_handle, &idx);
    crypto_chk_return(symc_ctx == TD_NULL, SYMC_COMPAT_ERRNO(ERROR_NOT_INIT), "call init first!\n");
    crypto_chk_return(symc_ctx->is_open == TD_FALSE, SYMC_COMPAT_ERRNO(ERROR_CTX_CLOSED), "ctx is closed\n");
    if (symc_ctx->is_attached == TD_FALSE) {
        return TD_SUCCESS;
    }

    kapi_symc_lock();

    symc_ctx->keyslot_handle = 0;
    symc_ctx->is_attached = TD_FALSE;

    kapi_symc_unlock();
    crypto_kapi_func_exit();
    return ret;
}
CRYPTO_EXPORT_SYMBOL(kapi_cipher_symc_detach);
#endif

static td_s32 kapi_cipher_symc_crypto(td_handle kapi_symc_handle, const crypto_buf_attr *src_buf,
    const crypto_buf_attr *dst_buf, td_u32 length, td_u32 crypto_type)
{
    td_s32 ret;
    td_u32 idx = 0;
    crypto_kapi_symc_process *symc_channel = TD_NULL;
    crypto_kapi_symc_ctx *symc_ctx = TD_NULL;

    symc_channel = inner_get_current_symc_channel();
    crypto_chk_return(symc_channel == TD_NULL, SYMC_COMPAT_ERRNO(ERROR_NOT_INIT), "call init first!\n");

    symc_ctx = inner_kapi_get_symc_ctx(kapi_symc_handle, &idx);
    crypto_chk_return(symc_ctx == TD_NULL, SYMC_COMPAT_ERRNO(ERROR_NOT_INIT), "call init first!\n");
    crypto_chk_return(symc_ctx->is_open == TD_FALSE, SYMC_COMPAT_ERRNO(ERROR_CTX_CLOSED), "ctx is closed\n");
    crypto_chk_return(symc_ctx->is_set_config == TD_FALSE, SYMC_COMPAT_ERRNO(ERROR_NOT_SET_CONFIG),
        "set_config first\n");
    crypto_chk_return(symc_ctx->is_attached == TD_FALSE, SYMC_COMPAT_ERRNO(ERROR_NOT_ATTACHED), "attach first\n");

    kapi_symc_lock();

    if (symc_ctx->symc_attr.is_long_term == TD_TRUE) {
        ret = inner_kapi_symc_crypto_long_term(symc_ctx, src_buf, dst_buf, length, crypto_type);
        crypto_chk_goto(ret != TD_SUCCESS, unlock_exit,
            "inner_kapi_symc_crypto_long_term for Encrypt failed, ret is 0x%x\n", ret);
    } else {
        ret = inner_kapi_symc_crypto_short_term(symc_ctx, src_buf, dst_buf, length, crypto_type);
        crypto_chk_goto(ret != TD_SUCCESS, unlock_exit, "inner_kapi_symc_crypto_short_term failed, ret is 0x%x\n", ret);
    }
    kapi_symc_unlock();

    symc_ctx->processed_length += length;
    return ret;

unlock_exit:
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

td_s32 kapi_cipher_symc_get_tag(td_handle kapi_symc_handle, td_u8 *tag, td_u32 tag_length)
{
    td_s32 ret;
    td_u32 idx = 0;
    crypto_kapi_symc_process *symc_channel = TD_NULL;
    crypto_kapi_symc_ctx *symc_ctx = TD_NULL;
    crypto_kapi_func_enter();

    symc_null_ptr_chk(tag);

    symc_channel = inner_get_current_symc_channel();
    crypto_chk_return(symc_channel == TD_NULL, SYMC_COMPAT_ERRNO(ERROR_NOT_INIT), "Invalid Process!\n");

    symc_ctx = inner_kapi_get_symc_ctx(kapi_symc_handle, &idx);
    crypto_chk_return(symc_ctx == TD_NULL, SYMC_COMPAT_ERRNO(ERROR_NOT_INIT), "call init first!\n");
    crypto_chk_return(symc_ctx->is_open == TD_FALSE, SYMC_COMPAT_ERRNO(ERROR_CTX_CLOSED), "ctx is closed\n");
    crypto_chk_return(symc_ctx->is_set_config == TD_FALSE, SYMC_COMPAT_ERRNO(ERROR_NOT_SET_CONFIG),
        "set_config first\n");
    crypto_chk_return(symc_ctx->is_attached == TD_FALSE, SYMC_COMPAT_ERRNO(ERROR_NOT_ATTACHED), "attach first\n");

    kapi_symc_lock();

    if (symc_ctx->symc_attr.is_long_term == TD_TRUE) {
        ret = inner_kapi_symc_get_tag_long_term(symc_ctx, tag, tag_length);
        crypto_chk_goto(ret != TD_SUCCESS, exit_unlock_mutex, "inner_kapi_symc_get_tag_long_term failed\n");
    } else {
        ret = inner_kapi_symc_get_tag_short_term(symc_ctx, tag, tag_length);
        crypto_chk_goto(ret != TD_SUCCESS, exit_unlock_mutex, "inner_kapi_symc_get_tag_short_term failed\n");
    }

exit_unlock_mutex:
    kapi_symc_unlock();
    crypto_kapi_func_exit();
    return ret;
}
CRYPTO_EXPORT_SYMBOL(kapi_cipher_symc_get_tag);