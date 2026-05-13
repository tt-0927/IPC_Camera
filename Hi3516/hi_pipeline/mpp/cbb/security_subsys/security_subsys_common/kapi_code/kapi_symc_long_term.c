/**
  Copyright (c), 2001-2025, Shenshu Tech. Co., Ltd.
 */

#include "kapi_symc_inner.h"
#include "drv_symc.h"

td_s32 inner_kapi_symc_create_long_term(crypto_kapi_symc_ctx *symc_ctx, const crypto_symc_attr *symc_attr)
{
    symc_ctx->symc_attr.is_long_term = TD_TRUE;
    return drv_cipher_symc_create(&symc_ctx->drv_symc_handle, symc_attr);
}

td_s32 inner_kapi_symc_set_config_long_term(crypto_kapi_symc_ctx *symc_ctx, const crypto_symc_ctrl_t *symc_ctrl)
{
    return drv_cipher_symc_set_config(symc_ctx->drv_symc_handle, symc_ctrl);
}

td_s32 inner_kapi_symc_get_config_long_term(crypto_kapi_symc_ctx *symc_ctx, crypto_symc_ctrl_t *symc_ctrl)
{
    return drv_cipher_symc_get_config(symc_ctx->drv_symc_handle, symc_ctrl);
}

td_s32 inner_kapi_symc_attach_long_term(crypto_kapi_symc_ctx *symc_ctx, td_handle keyslot_handle)
{
    return drv_cipher_symc_attach(symc_ctx->drv_symc_handle, keyslot_handle);
}

td_s32 inner_kapi_symc_crypto_long_term(crypto_kapi_symc_ctx *symc_ctx, const crypto_buf_attr *src_buf,
    const crypto_buf_attr *dst_buf, td_u32 length, td_u32 crypto_type)
{
    td_s32 ret;
    if (crypto_type == CRYPTO_TYPE_ENCRYPT) {
        ret = drv_cipher_symc_encrypt(symc_ctx->drv_symc_handle, src_buf, dst_buf, length);
    } else {
        ret = drv_cipher_symc_decrypt(symc_ctx->drv_symc_handle, src_buf, dst_buf, length);
    }
    return ret;
}

td_s32 inner_kapi_symc_get_tag_long_term(crypto_kapi_symc_ctx *symc_ctx, td_u8 *tag, td_u32 tag_length)
{
    return drv_cipher_symc_get_tag(symc_ctx->drv_symc_handle, tag, tag_length);
}