/**
  Copyright (c), 2001-2025, Shenshu Tech. Co., Ltd.
 */
#include "drv_hash.h"
#include "drv_hash_inner.h"

#include "hal_hash.h"

#include "crypto_drv_common.h"

#if defined(CONFIG_HASH_SOFT_HMAC_SUPPORT)
td_s32 inner_drv_hmac_start(drv_hash_simple_context *ctx, const crypto_hash_attr *hash_attr)
{
    td_s32 ret = TD_SUCCESS;
    td_u32 i;
    td_u32 block_size = crypto_hash_get_block_size(hash_attr->hash_type) / CRYPTO_BITS_IN_BYTE;
    td_u8 *i_key_pad_buf = NULL;

    if (hash_attr->key_len != 0) {
        ret = memcpy_s(ctx->tail, sizeof(ctx->tail), hash_attr->key, hash_attr->key_len);
        crypto_chk_return(ret != EOK, HASH_COMPAT_ERRNO(ERROR_MEMCPY_S), "memcpy_s failed\n");
    }

    /* Calc o_key_pad and i_key_pad */
    for (i = 0; i < block_size; i++) {
        ctx->o_key_pad[i] = 0x5c ^ ctx->tail[i];
        ctx->i_key_pad[i] = 0x36 ^ ctx->tail[i];
    }
    inner_drv_hash_length_add(ctx->length, block_size * CRYPTO_BITS_IN_BYTE);

    i_key_pad_buf = crypto_malloc_mmz(block_size, "i_key_pad_buf");
    crypto_chk_return(i_key_pad_buf == NULL, HASH_COMPAT_ERRNO(ERROR_MALLOC), "crypto_malloc_mmz failed\n");

    ret = memcpy_s(i_key_pad_buf, block_size, ctx->i_key_pad, block_size);
    crypto_chk_goto_with_ret(ret, ret != EOK, exit_free, HASH_COMPAT_ERRNO(ERROR_MEMCPY_S), "memcpy_s failed\n");

    ret = inner_drv_hash_process(ctx->chn_num, ctx, i_key_pad_buf, block_size,
        IN_NODE_TYPE_FIRST | IN_NODE_TYPE_LAST);
    crypto_chk_goto(ret != TD_SUCCESS, exit_free, "inner_drv_hash_process failed\n");

exit_free:
    memset_s(i_key_pad_buf, block_size, 0, block_size);
    crypto_free_coherent(i_key_pad_buf);
    return ret;
}

td_s32 inner_drv_hmac_finish(drv_hash_simple_context *ctx, td_u8 *out, td_u32 out_len)
{
    td_s32 ret = TD_SUCCESS;
    td_u8 *buffer = NULL;
    td_u32 *length = ctx->length;
    td_u32 processed_len = 0;
    td_u32 block_size = crypto_hash_get_block_size(ctx->hash_type) / 8; // 8 means 1 byte = 8 bits
    td_u32 buffer_len = block_size * 2;

    buffer = crypto_malloc_mmz(buffer_len, "hmac_tail_buf");
    crypto_chk_return(buffer == NULL, HASH_COMPAT_ERRNO(ERROR_MALLOC), "crypto_malloc_mmz failed\n");

    (td_void)memset_s(buffer, buffer_len, 0, buffer_len);
    /* Copy o_key_pad */
    ret = memcpy_s(buffer, buffer_len, ctx->o_key_pad, block_size);
    crypto_chk_goto_with_ret(ret, ret != EOK, exit_free, HASH_COMPAT_ERRNO(ERROR_MEMCPY_S), "memcpy_s failed\n");

    processed_len += block_size;

    /* Copy hash. */
    ret = memcpy_s(buffer + processed_len, buffer_len - processed_len, out, out_len);
    crypto_chk_goto_with_ret(ret, ret != EOK, exit_free, HASH_COMPAT_ERRNO(ERROR_MEMCPY_S), "memcpy_s failed\n");

    processed_len += out_len;
    buffer[processed_len] = 0x80;
    length[1] = crypto_cpu_to_be32(processed_len * 8); // 8 means 1 byte = 8 bits
    ret = memcpy_s(buffer + 2 * block_size - sizeof(ctx->length), // 2 block offset
        sizeof(ctx->length), length, sizeof(ctx->length));
    crypto_chk_goto_with_ret(ret, ret != EOK, exit_free, HASH_COMPAT_ERRNO(ERROR_MEMCPY_S), "memcpy_s failed\n");

    ret = drv_hash_get_state_iv(ctx->hash_type, TD_NULL, ctx->state, sizeof(ctx->state));
    crypto_chk_goto(ret != TD_SUCCESS, exit_free, "drv_hash_get_state_iv failed\n");

    ret = hal_cipher_hash_config(ctx->chn_num, ctx->hash_type, ctx->state);
    crypto_chk_goto(ret != TD_SUCCESS, exit_free, "hal_cipher_hash_config failed\n");

    ret = inner_drv_hash_process(ctx->chn_num, ctx, buffer, 2 * block_size,  // 2 block_size needed
        IN_NODE_TYPE_FIRST | IN_NODE_TYPE_LAST);
    crypto_chk_goto(ret != TD_SUCCESS, exit_free, "inner_drv_hash_process failed\n");

exit_free:
    memset_s(buffer, buffer_len, 0, buffer_len);
    crypto_free_coherent(buffer);
    return ret;
}
#endif