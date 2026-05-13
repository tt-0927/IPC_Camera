/**
  Copyright (c), 2001-2025, Shenshu Tech. Co., Ltd.
 */
#ifndef DRV_HASH_INNER_H
#define DRV_HASH_INNER_H

#include "crypto_hash_common.h"
#include "hal_hash.h"

#define HASH_COMPAT_ERRNO(err_code)     DRV_COMPAT_ERRNO(ERROR_MODULE_HASH, err_code)
#if defined(CONFIG_DRV_HASH_PARAM_TRACE_ENABLE)
#define drv_hash_param_trace            crypto_param_trace
#else
#define drv_hash_param_trace(...)
#endif

typedef struct {
    td_u8 chn_num;
    crypto_hash_type hash_type;
    td_u32 state[CRYPTO_HASH_RESULT_SIZE_MAX_IN_WORD];
    td_u8 tail[CRYPTO_HASH_BLOCK_SIZE_MAX];
    td_u32 tail_length;
    td_u32 length[2];
    td_bool is_keyslot;
#if defined(CONFIG_HASH_SOFT_HMAC_SUPPORT)
    td_u8 o_key_pad[CRYPTO_HASH_BLOCK_SIZE_MAX];
    td_u8 i_key_pad[CRYPTO_HASH_BLOCK_SIZE_MAX];
#endif
    td_u8 *data_buffer;
    td_u32 data_buffer_len;
} drv_hash_simple_context;

typedef struct {
    td_u32 length[2];
    td_u32 state[CRYPTO_HASH_RESULT_SIZE_MAX_IN_WORD];
#if defined(CONFIG_HASH_SOFT_HMAC_SUPPORT)
    td_u8 o_key_pad[CRYPTO_HASH_BLOCK_SIZE_MAX];
    td_u8 i_key_pad[CRYPTO_HASH_BLOCK_SIZE_MAX];
#endif
    td_u8 tail[CRYPTO_HASH_BLOCK_SIZE_MAX];
    td_u32 tail_len;
    td_u8 *data_buffer;
    td_u32 data_buffer_len;
} crypto_hash_ctx;

typedef struct {
    td_bool open;
    td_bool is_keyslot;
    crypto_hash_type hash_type;
    crypto_hash_ctx hash_ctx;
} hal_hash_channel_context;

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif /* __cplusplus */
#endif /* __cplusplus */

td_void inner_drv_hash_length_add(td_u32 length[2], td_u32 addition);

td_s32 inner_drv_hmac_start(drv_hash_simple_context *ctx, const crypto_hash_attr *hash_attr);

td_s32 inner_drv_hmac_finish(drv_hash_simple_context *ctx, td_u8 *out, td_u32 out_len);

td_u8 *inner_drv_hash_get_dma_buf(td_void);

drv_hash_simple_context *inner_get_hash_ctx(td_handle hash_handle);

td_s32 inner_drv_hash_process(td_u32 chn_num, drv_hash_simple_context *ctx,
    td_u8 *data_buffer, td_u32 data_length, td_u32 node_type);

td_s32 inner_hash_drv_handle_chk(td_handle hash_handle);

td_s32 inner_hash_start_param_chk(td_handle *drv_hash_handle, const crypto_hash_attr *hash_attr);

#ifdef __cplusplus
#if __cplusplus
}
#endif /* __cplusplus */
#endif /* __cplusplus */

#endif