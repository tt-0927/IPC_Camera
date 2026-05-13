/*
  Copyright (c), 2001-2025, Shenshu Tech. Co., Ltd.
 */
#if defined(CONFIG_HASH_HARDEN_INTERFACE_SUPPORT)
#include "kapi_hash_impl.h"
#include "drv_hash.h"
#include "drv_hash_inner.h"
#include "crypto_drv_common.h"

#define HASH_INIT_SHA_MAX_LENGTH                    64
#define INVALID_HANDLE      0xFFFFFFFF
static td_handle g_mbedtls_hash_handle = INVALID_HANDLE;
static crypto_mutex g_mbedtls_hash_lock;

td_s32 kapi_hash_start_impl(crypto_hash_clone_ctx *clone_ctx, crypto_hash_type hash_type)
{
    int ret = TD_FAILURE;
    crypto_drv_func_enter();

    crypto_chk_return(clone_ctx == TD_NULL, HASH_COMPAT_ERRNO(ERROR_PARAM_IS_NULL), "clone_ctx is NULL\n");

    (void)memset_s(clone_ctx, sizeof(crypto_hash_clone_ctx), 0, sizeof(crypto_hash_clone_ctx));

    ret = drv_hash_get_state_iv((crypto_hash_type)hash_type, NULL, clone_ctx->state, sizeof(clone_ctx->state));
    crypto_chk_return(ret != TD_SUCCESS, ret, "drv_hash_get_state_iv failed\n");

    clone_ctx->hash_type = hash_type;
    crypto_drv_func_exit();
    return 0;
}

td_s32 kapi_hash_update_impl(crypto_hash_clone_ctx *clone_ctx,
    const td_u8 *data, td_u32 data_len)
{
    int ret;
    crypto_buf_attr src_buf;
    crypto_drv_func_enter();

    crypto_chk_return(clone_ctx == TD_NULL, HASH_COMPAT_ERRNO(ERROR_PARAM_IS_NULL), "clone_ctx is NULL\n");
    crypto_chk_return(data == TD_NULL, HASH_COMPAT_ERRNO(ERROR_PARAM_IS_NULL), "update data is NULL\n");

    src_buf.virt_addr = (unsigned char *)data;

    ret = crypto_mutex_lock(&g_mbedtls_hash_lock);
    crypto_chk_return(ret != 0, ret, "crypto_mutex_lock failed\n");

    ret = drv_cipher_hash_set(g_mbedtls_hash_handle, clone_ctx);
    crypto_chk_goto(ret != 0, exit_mutex_unlock, "drv_cipher_hash_set failed\n");

    ret = drv_cipher_hash_update(g_mbedtls_hash_handle, &src_buf, data_len);
    crypto_chk_goto(ret != 0, exit_mutex_unlock, "drv_cipher_hash_update failed\n");

    ret = drv_cipher_hash_get(g_mbedtls_hash_handle, clone_ctx);
    crypto_chk_goto(ret != 0, exit_mutex_unlock, "drv_cipher_hash_get failed\n");

exit_mutex_unlock:
    crypto_mutex_unlock(&g_mbedtls_hash_lock);
    crypto_drv_func_exit();
    return ret;
}

td_s32 kapi_hash_finish_impl(crypto_hash_clone_ctx *clone_ctx, td_u8 *out, td_u32 *out_len)
{
    int ret;
    crypto_drv_func_enter();

    crypto_chk_return(clone_ctx == TD_NULL, HASH_COMPAT_ERRNO(ERROR_PARAM_IS_NULL), "clone_ctx is NULL\n");
    crypto_chk_return(out == TD_NULL, HASH_COMPAT_ERRNO(ERROR_PARAM_IS_NULL), "out is NULL\n");
    crypto_chk_return(out_len == TD_NULL, HASH_COMPAT_ERRNO(ERROR_PARAM_IS_NULL), "out_len is NULL\n");

    ret = crypto_mutex_lock(&g_mbedtls_hash_lock);
    crypto_chk_return(ret != 0, ret, "crypto_mutex_lock failed\n");

    ret = drv_cipher_hash_set(g_mbedtls_hash_handle, clone_ctx);
    crypto_chk_goto(ret != 0, exit_mutex_unlock, "drv_cipher_hash_set failed\n");

    ret = drv_cipher_hash_finish_data(g_mbedtls_hash_handle, out, out_len);
    crypto_chk_goto(ret != 0, exit_mutex_unlock, "drv_cipher_hash_finish_data failed\n");

exit_mutex_unlock:
    crypto_mutex_unlock(&g_mbedtls_hash_lock);
    crypto_drv_func_exit();

    return 0;
}

td_s32 kapi_hash_init_impl(td_void)
{
    td_s32 ret = TD_SUCCESS;
    crypto_hash_attr hash_attr = {0};

    if (g_mbedtls_hash_handle == INVALID_HANDLE) {
        hash_attr.hash_type = CRYPTO_HASH_TYPE_SHA256;
        (void)drv_cipher_hash_start(&g_mbedtls_hash_handle, &hash_attr);
    }

    ret = crypto_mutex_init(&g_mbedtls_hash_lock);
    crypto_chk_return(ret != TD_SUCCESS, HASH_COMPAT_ERRNO(ERROR_MUTEX_INIT), "crypto_mutex_lock init failed\n");
    return ret;
}

td_void kapi_hash_deinit_impl(td_void)
{
    if (g_mbedtls_hash_handle != INVALID_HANDLE) {
        (void)drv_cipher_hash_destroy(g_mbedtls_hash_handle);
        g_mbedtls_hash_handle = INVALID_HANDLE;
    }
    crypto_mutex_destroy(&g_mbedtls_hash_lock);
}
#endif