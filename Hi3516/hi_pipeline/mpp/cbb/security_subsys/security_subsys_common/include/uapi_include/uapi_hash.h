/*
  Copyright (c), 2001-2025, Shenshu Tech. Co., Ltd.
 */

#ifndef UAPI_HASH_H
#define UAPI_HASH_H

#include "crypto_type.h"
#include "crypto_hash_struct.h"

td_s32 unify_uapi_cipher_hash_init(td_void);

td_s32 unify_uapi_cipher_hash_deinit(td_void);

td_s32 unify_uapi_cipher_hash_start(td_handle *uapi_hash_handle, const crypto_hash_attr *hash_attr);

td_s32 unify_uapi_cipher_hash_update(td_handle uapi_hash_handle, const crypto_buf_attr *src_buf, const td_u32 len);

td_s32 unify_uapi_cipher_hash_finish(td_handle uapi_hash_handle, td_u8 *out, td_u32 *out_len);

td_s32 unify_uapi_cipher_hash_get(td_handle uapi_hash_handle, crypto_hash_clone_ctx *hash_clone_ctx);

td_s32 unify_uapi_cipher_hash_set(td_handle uapi_hash_handle, const crypto_hash_clone_ctx *hash_clone_ctx);

td_s32 unify_uapi_cipher_hash_destroy(td_handle uapi_hash_handle);

td_s32 unify_uapi_cipher_hash_start_impl(crypto_hash_clone_ctx *clone_ctx, crypto_hash_type hash_type);

td_s32 unify_uapi_cipher_hash_update_impl(crypto_hash_clone_ctx *clone_ctx, const td_u8 *data, td_u32 data_len);

td_s32 unify_uapi_cipher_hash_finish_impl(crypto_hash_clone_ctx *clone_ctx, td_u8 *out, td_u32 *out_len);

#endif