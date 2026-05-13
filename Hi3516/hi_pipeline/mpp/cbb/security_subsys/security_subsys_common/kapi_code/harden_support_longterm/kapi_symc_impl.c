/*
  Copyright (c), 2001-2025, Shenshu Tech. Co., Ltd.
 */
#if defined(CONFIG_SYMC_HARDEN_INTERFACE_SUPPORT)
#include "kapi_symc_impl.h"
#include "kapi_symc_inner.h"
#include "drv_symc.h"
#include "drv_symc_outer.h"
#include "crypto_drv_common.h"

#define SYMC_COMPAT_ERRNO(err_code)     KAPI_COMPAT_ERRNO(ERROR_MODULE_SYMC, err_code)

#define INVALID_HANDLE      0xFFFFFFFF
static td_handle g_symc_handle = INVALID_HANDLE;
static crypto_mutex g_harden_symc_lock;

td_s32 kapi_symc_init_impl(td_void)
{
    td_s32 ret = TD_SUCCESS;
    crypto_symc_attr symc_attr = {0};

    ret = drv_cipher_symc_create(&g_symc_handle, &symc_attr);
    crypto_chk_return(ret != TD_SUCCESS, ret, "drv_cipher_symc_create failed, ret is 0x%x\n", ret);

    ret = crypto_mutex_init(&g_harden_symc_lock);
    crypto_chk_goto(ret != TD_SUCCESS, symc_release, "crypto_mutex_init failed, ret is 0x%x\n", ret);

    return TD_SUCCESS;
symc_release:
    drv_cipher_symc_destroy(g_symc_handle);
    g_symc_handle = INVALID_HANDLE;
    return ret;
}

td_void kapi_symc_destroy_impl(td_void)
{
    if (g_symc_handle != INVALID_HANDLE) {
        drv_cipher_symc_destroy(g_symc_handle);
        g_symc_handle = INVALID_HANDLE;
    }
 
    (void)crypto_mutex_destroy(&g_harden_symc_lock);
}
#endif

#if defined(CONFIG_SYMC_HARDEN_INTERFACE_SUPPORT)
td_s32 kapi_cipher_symc_crypt_impl(crypto_symc_ctrl_t *symc_ctrl, const crypto_buf_attr *src_buf,
    const crypto_buf_attr *dst_buf, td_u32 data_len, td_handle keyslot_handle, td_bool is_decrypt)
{
    td_s32 ret = TD_SUCCESS;

    symc_null_ptr_chk(symc_ctrl);
    symc_null_ptr_chk(src_buf);
    symc_null_ptr_chk(dst_buf);
    crypto_chk_return(g_symc_handle == INVALID_HANDLE, SYMC_COMPAT_ERRNO(ERROR_NOT_INIT), "handle is not created\n");

    ret = crypto_mutex_lock(&g_harden_symc_lock);
    crypto_chk_return(ret != 0, ret, "crypto_mutex_lock failed\n");

    ret = drv_cipher_symc_attach(g_symc_handle, keyslot_handle);
    crypto_chk_goto(ret != CRYPTO_SUCCESS, exit_unlock, "drv_cipher_symc_set_key failed\n");

    ret = drv_cipher_symc_set_config(g_symc_handle, symc_ctrl);
    crypto_chk_goto(ret != CRYPTO_SUCCESS, exit_unlock, "drv_cipher_symc_set_config failed\n");

    if (is_decrypt == TD_FALSE) {
        ret = drv_cipher_symc_encrypt(g_symc_handle, src_buf, dst_buf, data_len);
    } else {
        ret = drv_cipher_symc_decrypt(g_symc_handle, src_buf, dst_buf, data_len);
    }
    crypto_chk_goto(ret != CRYPTO_SUCCESS, exit_unlock, "drv_cipher_symc_crypto failed\n");

    ret = drv_cipher_symc_get_config(g_symc_handle, symc_ctrl);
    crypto_chk_goto(ret != CRYPTO_SUCCESS, exit_unlock, "drv_cipher_symc_get_config failed\n");

exit_unlock:
    crypto_mutex_unlock(&g_harden_symc_lock);
    return ret;
}
#endif