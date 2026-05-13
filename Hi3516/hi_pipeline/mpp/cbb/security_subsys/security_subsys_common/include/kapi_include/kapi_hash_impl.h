/**
  Copyright (c), 2001-2025, Shenshu Tech. Co., Ltd.
*/

#ifndef KAPI_HASH_IMPL_H
#define KAPI_HASH_IMPL_H
#include "crypto_hash_struct.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif /* __cplusplus */
#endif /* __cplusplus */

td_s32 kapi_hash_start_impl(crypto_hash_clone_ctx *clone_ctx, crypto_hash_type hash_type);

td_s32 kapi_hash_update_impl(crypto_hash_clone_ctx *clone_ctx, const td_u8 *data, td_u32 data_len);

td_s32 kapi_hash_finish_impl(crypto_hash_clone_ctx *clone_ctx, td_u8 *out, td_u32 *out_len);

td_s32 kapi_hash_init_impl(td_void);

td_void kapi_hash_deinit_impl(td_void);

#ifdef __cplusplus
#if __cplusplus
}
#endif /* __cplusplus */
#endif /* __cplusplus */

#endif /* KAPI_HASH_IMPL_H */