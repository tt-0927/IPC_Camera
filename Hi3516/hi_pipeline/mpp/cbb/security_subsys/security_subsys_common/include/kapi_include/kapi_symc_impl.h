/**
  Copyright (c), 2001-2025, Shenshu Tech. Co., Ltd.
*/

#ifndef KAPI_SYMC_IMPL_H
#define KAPI_SYMC_IMPL_H
#include "crypto_symc_struct.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif /* __cplusplus */
#endif /* __cplusplus */

td_s32 kapi_cipher_symc_crypt_impl(crypto_symc_ctrl_t *symc_ctrl, const crypto_buf_attr *src_buf,
    const crypto_buf_attr *dst_buf, td_u32 data_len, td_handle keyslot_handle, td_bool is_decrypt);

td_s32 kapi_cipher_ccm_update_ad_impl(crypto_symc_ctrl_t *symc_ctrl,
    crypto_symc_ccm_ctx *ccm_ctx, td_handle keyslot_handle);

td_s32 kapi_cipher_ccm_update_impl(crypto_symc_ctrl_t *symc_ctrl, crypto_symc_ccm_ctx *ccm_ctx,
    const crypto_buf_attr *src_buf, const crypto_buf_attr *dst_buf, td_u32 data_len, td_handle keyslot_handle,
    td_bool is_decrypt);

td_s32 kapi_cipher_ccm_finish_impl(crypto_symc_ctrl_t *symc_ctrl, crypto_symc_ccm_ctx *ccm_ctx,
    td_handle keyslot_handle, td_u8 *tag, td_u32 tag_len);

td_s32 kapi_symc_init_impl(td_void);

td_void kapi_symc_destroy_impl(td_void);


#ifdef __cplusplus
#if __cplusplus
}
#endif /* __cplusplus */
#endif /* __cplusplus */

#endif /* KAPI_HASH_IMPL_H */