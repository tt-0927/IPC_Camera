/**
  Copyright (c), 2001-2025, Shenshu Tech. Co., Ltd.
 */

#include "drv_klad.h"
#include "drv_klad_inner.h"

#include "crypto_drv_common.h"
#include "crypto_common_macro.h"

#include "hal_klad.h"
#include "hal_rkp.h"

static drv_klad_context g_klad_ctx = {0};

#define KLAD_VALID_HANDLE       0x2D3C4B5A
drv_klad_context *inner_klad_get_ctx(void)
{
    return &g_klad_ctx;
}

td_s32 drv_klad_create(td_handle *klad_handle)
{
    crypto_drv_func_enter();

    km_null_ptr_chk(klad_handle);

    (td_void)memset_s(&g_klad_ctx, sizeof(drv_klad_context), 0, sizeof(drv_klad_context));

    g_klad_ctx.is_open = TD_TRUE;

    *klad_handle = KLAD_VALID_HANDLE;
    crypto_drv_func_enter();
    return TD_SUCCESS;
}

td_s32 drv_klad_destroy(td_handle klad_handle)
{
    crypto_drv_func_enter();

    crypto_chk_return(klad_handle != KLAD_VALID_HANDLE, KM_COMPAT_ERRNO(ERROR_INVALID_HANDLE), "invalid klad_handle\n");
    if (g_klad_ctx.is_open == TD_FALSE) {
        return TD_SUCCESS;
    }
    (td_void)memset_s(&g_klad_ctx, sizeof(drv_klad_context), 0, sizeof(drv_klad_context));

    crypto_drv_func_enter();
    return TD_SUCCESS;
}

td_s32 drv_klad_attach(td_handle klad_handle, crypto_klad_dest klad_dest, td_handle keyslot_handle)
{
    volatile td_s32 ret = TD_FAILURE;
    crypto_klad_flash_key_type flash_key_type = CRYPTO_KLAD_FLASH_KEY_TYPE_INVALID;
    crypto_drv_func_enter();

    crypto_chk_return(klad_handle != KLAD_VALID_HANDLE, KM_COMPAT_ERRNO(ERROR_INVALID_HANDLE), "invalid klad_handle\n");
    crypto_chk_return(g_klad_ctx.is_open == TD_FALSE, KM_COMPAT_ERRNO(ERROR_CTX_CLOSED), "call create first\n");

    if (klad_dest == CRYPTO_KLAD_DEST_FLASH) {
        flash_key_type = (crypto_klad_flash_key_type)keyslot_handle;
    } else if (klad_dest == CRYPTO_KLAD_DEST_MCIPHER || klad_dest == CRYPTO_KLAD_DEST_HMAC) {
        ret = hal_klad_set_key_addr(klad_dest, keyslot_handle);
        crypto_chk_return(ret != TD_SUCCESS, ret, "hal_klad_set_key_addr failed\n");
    } else if (klad_dest == CRYPTO_KLAD_DEST_NPU) {
        ret = hal_klad_set_key_addr(klad_dest, keyslot_handle);
        crypto_chk_return(ret != TD_SUCCESS, ret, "hal_klad_set_key_addr failed\n");
    } else {
        crypto_log_err("invalid klad_dest\n");
        return KM_COMPAT_ERRNO(ERROR_INVALID_PARAM);
    }

    ret = hal_klad_set_key_dest_cfg(klad_dest, flash_key_type);
    crypto_chk_return(ret != TD_SUCCESS, ret, "hal_klad_set_key_dest_cfg failed\n");

    g_klad_ctx.keyslot_handle = keyslot_handle;
    g_klad_ctx.klad_dest = klad_dest;
    g_klad_ctx.is_attached = TD_TRUE;
    crypto_drv_func_enter();
    return ret;
}

td_s32 drv_klad_detach(td_handle klad_handle, crypto_klad_dest klad_dest, td_handle keyslot_handle)
{
    crypto_unused(keyslot_handle);
    crypto_chk_return(klad_handle != KLAD_VALID_HANDLE, KM_COMPAT_ERRNO(ERROR_INVALID_HANDLE), "invalid klad_handle\n");
    crypto_chk_return(g_klad_ctx.is_open == TD_FALSE, KM_COMPAT_ERRNO(ERROR_CTX_CLOSED), "call create first\n");
    crypto_chk_return(g_klad_ctx.klad_dest != klad_dest, KM_COMPAT_ERRNO(ERROR_INVALID_PARAM), "invalid klad_dest\n");
    crypto_chk_return(g_klad_ctx.keyslot_handle != keyslot_handle, KM_COMPAT_ERRNO(ERROR_INVALID_HANDLE),
        "invalid keyslot_handle\n");

    if (g_klad_ctx.is_attached == TD_FALSE) {
        return TD_SUCCESS;
    }

    g_klad_ctx.is_attached = TD_FALSE;
    g_klad_ctx.keyslot_handle = 0;
    g_klad_ctx.klad_dest = 0;

    return TD_SUCCESS;
}

td_s32 drv_klad_set_attr(td_handle klad_handle, const crypto_klad_attr *klad_attr)
{
    volatile td_s32 ret = TD_FAILURE;
    const crypto_klad_key_config *key_cfg = TD_NULL;
    const crypto_klad_key_secure_config *key_sec_cfg = TD_NULL;
    crypto_drv_func_enter();

    km_null_ptr_chk(klad_attr);
    crypto_chk_return(klad_attr->key_sec_cfg.key_sec != TD_FALSE && klad_attr->key_sec_cfg.key_sec != TD_TRUE,
        KM_COMPAT_ERRNO(ERROR_INVALID_PARAM), "klad_attr.key_sec_cfg.key_sec is invalid\n");
    crypto_chk_return(klad_attr->key_cfg.engine >= CRYPTO_KLAD_ENGINE_MAX, KM_COMPAT_ERRNO(ERROR_INVALID_PARAM),
        "klad_attr.key_cfg.engine >= CRYPTO_KLAD_ENGINE_MAX\n");
    crypto_chk_return(klad_handle != KLAD_VALID_HANDLE, KM_COMPAT_ERRNO(ERROR_INVALID_HANDLE), "invalid klad_handle\n");
    crypto_chk_return(g_klad_ctx.is_open == TD_FALSE, KM_COMPAT_ERRNO(ERROR_CTX_CLOSED), "call create first\n");

    ret = memcpy_s(&g_klad_ctx.klad_attr, sizeof(crypto_klad_attr), klad_attr, sizeof(crypto_klad_attr));
    crypto_chk_return(ret != EOK, KM_COMPAT_ERRNO(ERROR_MEMCPY_S), "memcpy_s failed\n");

    key_cfg = &(klad_attr->key_cfg);
    key_sec_cfg = &(klad_attr->key_sec_cfg);

    ret = hal_klad_set_key_crypto_cfg(key_cfg->encrypt_support, key_cfg->decrypt_support, key_cfg->engine);
    crypto_chk_return(ret != TD_SUCCESS, ret, "hal_klad_set_key_crypto_cfg failed\n");

    ret = hal_klad_set_key_secure_cfg(key_sec_cfg);
    crypto_chk_return(ret != TD_SUCCESS, ret, "hal_klad_set_key_secure_cfg failed\n");

    g_klad_ctx.hard_key_type = klad_attr->klad_cfg.rootkey_type;
    g_klad_ctx.is_set_attr = TD_TRUE;
    g_klad_ctx.rkp_sw_cfg = klad_attr->rkp_sw_cfg;

    crypto_drv_func_enter();
    return ret;
}

#if defined(CONFIG_KLAD_GET_ATTR)
td_s32 drv_klad_get_attr(td_handle klad_handle, crypto_klad_attr *klad_attr)
{
    volatile td_s32 ret = TD_FAILURE;
    crypto_drv_func_enter();

    km_null_ptr_chk(klad_attr);
    crypto_chk_return(klad_handle != KLAD_VALID_HANDLE, KM_COMPAT_ERRNO(ERROR_INVALID_HANDLE), "invalid klad_handle\n");
    crypto_chk_return(g_klad_ctx.is_open == TD_FALSE, KM_COMPAT_ERRNO(ERROR_CTX_CLOSED), "call create first\n");
    crypto_chk_return(g_klad_ctx.is_set_attr == TD_FALSE, KM_COMPAT_ERRNO(ERROR_NOT_SET_CONFIG),
        "call set_attr first\n");

    ret = memcpy_s(klad_attr, sizeof(crypto_klad_attr), &g_klad_ctx.klad_attr, sizeof(crypto_klad_attr));
    crypto_chk_return(ret != EOK, KM_COMPAT_ERRNO(ERROR_MEMCPY_S), "memcpy_s failed\n");

    crypto_drv_func_enter();
    return ret;
}
#endif