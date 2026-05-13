/**
  Copyright (c), 2001-2025, Shenshu Tech. Co., Ltd.
 */

#include "drv_hash_inner.h"

#include "crypto_drv_common.h"

#if defined(CONFIG_HASH_CLONE_SUPPORT)
td_s32 drv_cipher_hash_get(td_handle drv_hash_handle, crypto_hash_clone_ctx *hash_ctx)
{
    td_s32 ret = TD_SUCCESS;
    drv_hash_simple_context *ctx = TD_NULL;
    crypto_drv_func_enter();

    hash_null_ptr_chk(hash_ctx);

    ctx = inner_get_hash_ctx(drv_hash_handle);
    crypto_chk_return(ctx == TD_NULL, HASH_COMPAT_ERRNO(ERROR_INVALID_HANDLE), "invalid hash_handle\n");

    (td_void)memset_s(hash_ctx, sizeof(crypto_hash_clone_ctx), 0, sizeof(crypto_hash_clone_ctx));
    /* Clone Length. */
    ret = memcpy_s(hash_ctx->length, sizeof(hash_ctx->length), ctx->length, sizeof(ctx->length));
    crypto_chk_goto_with_ret(ret, ret != EOK, error_clear_hash_ctx,
        HASH_COMPAT_ERRNO(ERROR_MEMCPY_S), "memcpy_s failed\n");

    /* Clone state. */
    ret = memcpy_s(hash_ctx->state, sizeof(hash_ctx->state), ctx->state, sizeof(ctx->state));
    crypto_chk_goto_with_ret(ret, ret != EOK, error_clear_hash_ctx,
        HASH_COMPAT_ERRNO(ERROR_MEMCPY_S), "memcpy_s failed\n");

    /* Copy tail. */
    ret = memcpy_s(hash_ctx->tail, sizeof(hash_ctx->tail), ctx->tail, sizeof(ctx->tail));
    crypto_chk_goto_with_ret(ret, ret != EOK, error_clear_hash_ctx,
        HASH_COMPAT_ERRNO(ERROR_MEMCPY_S), "memcpy_s failed\n");
    hash_ctx->tail_len = ctx->tail_length;
#if defined(CONFIG_HASH_SOFT_HMAC_SUPPORT)
    /* Clone o_key_pad. */
    ret = memcpy_s(hash_ctx->o_key_pad, sizeof(hash_ctx->o_key_pad), ctx->o_key_pad, sizeof(ctx->o_key_pad));
    crypto_chk_goto_with_ret(ret, ret != EOK, error_clear_hash_ctx,
        HASH_COMPAT_ERRNO(ERROR_MEMCPY_S), "memcpy_s failed\n");

    /* Clone i_key_pad. */
    ret = memcpy_s(hash_ctx->i_key_pad, sizeof(hash_ctx->i_key_pad), ctx->i_key_pad, sizeof(ctx->i_key_pad));
    crypto_chk_goto_with_ret(ret, ret != EOK, error_clear_hash_ctx,
        HASH_COMPAT_ERRNO(ERROR_MEMCPY_S), "memcpy_s failed\n");
#endif
    /* Clone Hash Type. */
    hash_ctx->hash_type = ctx->hash_type;
    crypto_drv_func_exit();

    return ret;
error_clear_hash_ctx:
    (td_void)memset_s(hash_ctx, sizeof(crypto_hash_clone_ctx), 0, sizeof(crypto_hash_clone_ctx));
    return ret;
}

td_s32 drv_cipher_hash_set(td_handle drv_hash_handle, const crypto_hash_clone_ctx *hash_ctx)
{
    td_s32 ret = TD_SUCCESS;
    td_u32 chn_num = (td_u32)drv_hash_handle;
    drv_hash_simple_context *ctx = TD_NULL;

    crypto_drv_func_enter();

    hash_null_ptr_chk(hash_ctx);

    ctx = inner_get_hash_ctx(drv_hash_handle);
    crypto_chk_return(ctx == TD_NULL, HASH_COMPAT_ERRNO(ERROR_INVALID_HANDLE), "invalid hash_handle\n");

    /* Clone Length. */
    ret = memcpy_s(ctx->length, sizeof(ctx->length), hash_ctx->length, sizeof(hash_ctx->length));
    crypto_chk_goto_with_ret(ret, ret != EOK, error_clear_hash_ctx,
        HASH_COMPAT_ERRNO(ERROR_MEMCPY_S), "memcpy_s failed\n");

    /* Clone state. */
    ret = memcpy_s(ctx->state, sizeof(ctx->state), hash_ctx->state, sizeof(hash_ctx->state));
    crypto_chk_goto_with_ret(ret, ret != EOK, error_clear_hash_ctx,
        HASH_COMPAT_ERRNO(ERROR_MEMCPY_S), "memcpy_s failed\n");

    /* Copy tail. */
    ret = memcpy_s(ctx->tail, sizeof(ctx->tail), hash_ctx->tail, sizeof(hash_ctx->tail));
    crypto_chk_goto_with_ret(ret, ret != EOK, error_clear_hash_ctx,
        HASH_COMPAT_ERRNO(ERROR_MEMCPY_S), "memcpy_s failed\n");
    ctx->tail_length = hash_ctx->tail_len;
#if defined(CONFIG_HASH_SOFT_HMAC_SUPPORT)
    /* Clone o_key_pad. */
    ret = memcpy_s(ctx->o_key_pad, sizeof(ctx->o_key_pad), hash_ctx->o_key_pad, sizeof(hash_ctx->o_key_pad));
    crypto_chk_goto_with_ret(ret, ret != EOK, error_clear_hash_ctx,
        HASH_COMPAT_ERRNO(ERROR_MEMCPY_S), "memcpy_s failed\n");

    /* Clone i_key_pad. */
    ret = memcpy_s(ctx->i_key_pad, sizeof(ctx->i_key_pad), hash_ctx->i_key_pad, sizeof(hash_ctx->i_key_pad));
    crypto_chk_goto_with_ret(ret, ret != EOK, error_clear_hash_ctx,
        HASH_COMPAT_ERRNO(ERROR_MEMCPY_S), "memcpy_s failed\n");
#endif
    /* Clone Hash Type. */
    ctx->hash_type = hash_ctx->hash_type;

    /* Set Hash Config. */
    ret = hal_cipher_hash_config(chn_num, ctx->hash_type, ctx->state);
    if (ret != TD_SUCCESS) {
        crypto_print("hal_cipher_hash_config expected is 0x%x, real ret is 0x%x\n", TD_SUCCESS, ret);
        goto error_clear_hash_ctx;
    }
    crypto_drv_func_exit();

    return ret;
error_clear_hash_ctx:
#if defined(CONFIG_HASH_SOFT_HMAC_SUPPORT)
    (td_void)memset_s(ctx->o_key_pad, sizeof(ctx->o_key_pad), 0, sizeof(ctx->o_key_pad));
    (td_void)memset_s(ctx->i_key_pad, sizeof(ctx->i_key_pad), 0, sizeof(ctx->i_key_pad));
#endif
    return ret;
}
#else
td_s32 drv_cipher_hash_get(td_handle drv_hash_handle, crypto_hash_clone_ctx *hash_ctx)
{
    crypto_unused(drv_hash_handle);
    crypto_unused(hash_ctx);

    return HASH_COMPAT_ERRNO(ERROR_UNSUPPORT);
}

td_s32 drv_cipher_hash_set(td_handle drv_hash_handle, const crypto_hash_clone_ctx *hash_ctx)
{
    crypto_unused(drv_hash_handle);
    crypto_unused(hash_ctx);

    return HASH_COMPAT_ERRNO(ERROR_UNSUPPORT);
}
#endif