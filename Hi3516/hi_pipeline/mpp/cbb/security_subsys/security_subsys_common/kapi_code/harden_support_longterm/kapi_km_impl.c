/*
  Copyright (c), 2001-2025, Shenshu Tech. Co., Ltd.
 */
#if defined(CONFIG_SYMC_HARDEN_INTERFACE_SUPPORT)
#include "kapi_km_impl.h"
#include "drv_klad.h"
#include "drv_keyslot.h"
#include "crypto_drv_common.h"

#define KM_COMPAT_ERRNO(err_code)     KAPI_COMPAT_ERRNO(ERROR_MODULE_KM, err_code)

#define CRYPTO_MCIPHER_KEYSLOT_NUM      8
static crypto_mutex g_harden_km_lock;

typedef struct {
    crypto_owner owner;
    td_bool is_open;
    td_handle keyslot_handle;
} crypto_keyslot_ctx_impl;

static crypto_keyslot_ctx_impl g_kslot_ctx_arr[CRYPTO_MCIPHER_KEYSLOT_NUM] = {0};

static td_s32 inner_occupy_kslot_channel(td_handle keyslot_handle)
{
    int i;
    for (i = 0; i < CRYPTO_MCIPHER_KEYSLOT_NUM; i++) {
        if (g_kslot_ctx_arr[i].is_open == TD_FALSE) {
            break;
        }
    }
    if (i >= CRYPTO_MCIPHER_KEYSLOT_NUM) {
        crypto_log_err("kslot channel overflow\n");
        return KM_COMPAT_ERRNO(ERROR_CHN_BUSY);
    }
    crypto_get_owner(&g_kslot_ctx_arr[i].owner);
    g_kslot_ctx_arr[i].keyslot_handle = keyslot_handle;
    g_kslot_ctx_arr[i].is_open = TD_TRUE;
    return TD_SUCCESS;
}

static td_bool inner_check_kslot_channel_exit(td_handle keyslot_handle)
{
    int i;
    crypto_owner owner  = {0};
    crypto_get_owner(&owner);
    for (i = 0; i < CRYPTO_MCIPHER_KEYSLOT_NUM; i++) {
        if (g_kslot_ctx_arr[i].keyslot_handle == keyslot_handle && g_kslot_ctx_arr[i].owner == owner
            && g_kslot_ctx_arr[i].is_open == TD_TRUE) {
            return TD_TRUE;
        }
    }
    return TD_FALSE;
}

static td_void inner_release_kslot_channel(td_handle keyslot_handle)
{
    int i;
    crypto_owner owner = {0};
    crypto_get_owner(&owner);
    for (i = 0; i < CRYPTO_MCIPHER_KEYSLOT_NUM; i++) {
        if (g_kslot_ctx_arr[i].keyslot_handle == keyslot_handle && g_kslot_ctx_arr[i].owner == owner
            && g_kslot_ctx_arr[i].is_open == TD_TRUE) {
            (td_void)memset_s(&g_kslot_ctx_arr[i], sizeof(crypto_keyslot_ctx_impl), 0, sizeof(crypto_keyslot_ctx_impl));
        }
    }
}

static td_s32 inner_klad_key_set(td_handle keyslot_handle, const td_u8 *key, td_u32 key_len, td_bool tee_open)
{
    td_s32 ret;
    td_handle klad_handle = 0;
    crypto_klad_clear_key clear_key = {
        .key = (td_u8 *)key,
        .key_length = key_len
    };
    crypto_klad_attr klad_attr = {
        .key_cfg = {
            .engine = CRYPTO_KLAD_ENGINE_AES,
            .decrypt_support = TD_TRUE,
            .encrypt_support = TD_TRUE
        },
        .key_sec_cfg = {
            .key_sec = TD_TRUE,
            .master_only_enable = TD_TRUE,
            .dest_buf_sec_support = TD_TRUE,
            .dest_buf_non_sec_support = TD_FALSE,
            .src_buf_sec_support = TD_TRUE,
            .src_buf_non_sec_support = TD_FALSE
        }
    };

    if (tee_open == TD_TRUE) {
        klad_attr.key_sec_cfg.key_sec = TD_TRUE;
        klad_attr.key_sec_cfg.master_only_enable = TD_FALSE;
        klad_attr.key_sec_cfg.dest_buf_sec_support = TD_TRUE;
        klad_attr.key_sec_cfg.src_buf_sec_support = TD_TRUE;
        klad_attr.key_sec_cfg.dest_buf_non_sec_support = TD_TRUE;
        klad_attr.key_sec_cfg.src_buf_non_sec_support = TD_TRUE;
    }

    ret = drv_klad_create(&klad_handle);
    crypto_chk_return(ret != TD_SUCCESS, ret, "drv_klad_create failed\n");

    ret = drv_klad_attach(klad_handle, CRYPTO_KLAD_DEST_MCIPHER, keyslot_handle);
    crypto_chk_goto(ret != TD_SUCCESS, klad_destroy, "drv_klad_attach failed, ret is 0x%x\n", ret);

    ret = drv_klad_set_attr(klad_handle, &klad_attr);
    crypto_chk_goto(ret != TD_SUCCESS, klad_detach, "drv_klad_set_attr failed\n");

    ret = drv_klad_set_clear_key(klad_handle, &clear_key);
    crypto_chk_goto(ret != TD_SUCCESS, klad_detach, "drv_klad_set_clear_key failed\n");
klad_detach:
    drv_klad_detach(klad_handle, CRYPTO_KLAD_DEST_MCIPHER, keyslot_handle);
klad_destroy:
    drv_klad_destroy(klad_handle);
    return ret;
}

td_s32 kapi_km_create_key_impl(td_handle *kapi_keyslot_handle)
{
    td_s32 ret;
    td_handle kslot_handle;

    km_null_ptr_chk(kapi_keyslot_handle);

    ret = crypto_mutex_lock(&g_harden_km_lock);
    crypto_chk_return(ret != OSAL_SUCCESS, KM_COMPAT_ERRNO(ERROR_MUTEX_LOCK), "crypto_mutex_lock failed\n");

    ret = drv_keyslot_create(&kslot_handle, CRYPTO_KEYSLOT_TYPE_MCIPHER);
    crypto_chk_goto(ret != TD_SUCCESS, exit_unlock, "drv_keyslot_create failed\n");

    ret = inner_occupy_kslot_channel(kslot_handle);
    crypto_chk_goto(ret != TD_SUCCESS, kslot_destroy, "inner_occupy_kslot_channel failed\n");

    *kapi_keyslot_handle = kslot_handle;
    crypto_mutex_unlock(&g_harden_km_lock);
    return TD_SUCCESS;
kslot_destroy:
    drv_keyslot_destroy(kslot_handle);
exit_unlock:
    crypto_mutex_unlock(&g_harden_km_lock);
    return ret;
}

td_s32 kapi_km_set_key_impl(td_handle kapi_keyslot_handle, const td_u8 *key, td_u32 key_len, td_bool tee_open)
{
    td_s32 ret;

    ret = crypto_mutex_lock(&g_harden_km_lock);
    crypto_chk_return(ret != OSAL_SUCCESS, KM_COMPAT_ERRNO(ERROR_MUTEX_LOCK), "crypto_mutex_lock failed\n");

    if (inner_check_kslot_channel_exit(kapi_keyslot_handle) != TD_TRUE) {
        crypto_log_err("inner_check_kslot_channel_exit check failed\n");
        ret = TD_FAILURE;
        goto exit_unlock;
    }

    ret = inner_klad_key_set(kapi_keyslot_handle, key, key_len, tee_open);
    crypto_chk_goto(ret != TD_SUCCESS, exit_unlock, "inner_klad_key_set failed\n");
exit_unlock:
    crypto_mutex_unlock(&g_harden_km_lock);
    return ret;
}

td_s32 kapi_km_desroy_key_impl(td_handle kapi_keyslot_handle)
{
    td_s32 ret;
    ret = crypto_mutex_lock(&g_harden_km_lock);
    crypto_chk_return(ret != OSAL_SUCCESS, KM_COMPAT_ERRNO(ERROR_MUTEX_LOCK), "crypto_mutex_lock failed\n");

    ret = drv_keyslot_destroy(kapi_keyslot_handle);
    crypto_chk_goto(ret != TD_SUCCESS, exit_unlock, "drv_keyslot_destroy failed\n");

    inner_release_kslot_channel(kapi_keyslot_handle);
exit_unlock:
    crypto_mutex_unlock(&g_harden_km_lock);
    return ret;
}

td_s32 kapi_km_init_impl(td_void)
{
    td_s32 ret = TD_SUCCESS;

    ret = crypto_mutex_init(&g_harden_km_lock);
    crypto_chk_return(ret != TD_SUCCESS, KM_COMPAT_ERRNO(ERROR_MUTEX_INIT),
        "priv_symc_handle_check failed, ret is 0x%x\n", ret);
    return TD_SUCCESS;
}

td_void kapi_km_deinit_impl(td_void)
{
    (void)crypto_mutex_destroy(&g_harden_km_lock);
}

#endif