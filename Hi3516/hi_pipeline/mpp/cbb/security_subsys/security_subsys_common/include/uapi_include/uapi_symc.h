/*
  Copyright (c), 2001-2025, Shenshu Tech. Co., Ltd.
 */

#ifndef UAPI_SYMC_H
#define UAPI_SYMC_H

#include "crypto_type.h"
#include "crypto_symc_struct.h"

td_s32 unify_uapi_cipher_symc_init(td_void);

td_s32 unify_uapi_cipher_symc_deinit(td_void);

td_s32 unify_uapi_cipher_symc_create(td_handle *symc_handle, const crypto_symc_attr *symc_attr);

td_s32 unify_uapi_cipher_symc_destroy(td_handle symc_handle);

td_s32 unify_uapi_cipher_symc_set_config(td_handle symc_handle, const crypto_symc_ctrl_t *symc_ctrl);

td_s32 unify_uapi_cipher_symc_get_config(td_handle symc_handle, crypto_symc_ctrl_t *symc_ctrl);

td_s32 unify_uapi_cipher_symc_attach(td_handle symc_handle, td_handle keyslot_handle);

td_s32 unify_uapi_cipher_symc_get_keyslot_handle(td_handle symc_handle, td_handle *keyslot_handle);

td_s32 unify_uapi_cipher_symc_set_key(td_handle symc_handle, td_u8 *key, td_u32 key_len);

td_s32 unify_uapi_cipher_symc_encrypt(td_handle symc_handle, const crypto_buf_attr *src_buf,
    const crypto_buf_attr *dst_buf, td_u32 length);

td_s32 unify_uapi_cipher_symc_decrypt(td_handle symc_handle, const crypto_buf_attr *src_buf,
    const crypto_buf_attr *dst_buf, td_u32 length);

td_s32 unify_uapi_cipher_symc_encrypt_multi(td_handle symc_handle, const crypto_symc_ctrl_t *symc_ctrl,
    const crypto_symc_pack *src_buf_pack, const crypto_symc_pack *dst_buf_pack, td_u32 pack_num);

td_s32 unify_uapi_cipher_symc_decrypt_multi(td_handle symc_handle, const crypto_symc_ctrl_t *symc_ctrl,
    const crypto_symc_pack *src_buf_pack, const crypto_symc_pack *dst_buf_pack, td_u32 pack_num);

td_s32 unify_uapi_cipher_symc_cenc_decrypt(td_handle symc_handle, const crypto_symc_cenc_param *cenc_param,
    const crypto_buf_attr *src_buf, const crypto_buf_attr *dst_buf, td_u32 length);

td_s32 unify_uapi_cipher_symc_crypt_impl(crypto_symc_ctrl_t *symc_ctrl, td_handle keyslot_handle,
    const crypto_buf_attr *src, const crypto_buf_attr *dst, td_u32 data_len, td_bool is_decrypt);

td_s32 unify_uapi_cipher_ccm_upadte_ad_impl(mbedtls_ccm_context *ctx,  crypto_symc_key_length key_len,
    const td_u8 *add, td_u32 add_len);

td_s32 unify_uapi_cipher_ccm_upadte_impl(mbedtls_ccm_context *ctx, crypto_symc_key_length key_len,
    const crypto_buf_attr *src, const crypto_buf_attr *dst, td_u32 process_len);

td_s32 unify_uapi_cipher_ccm_finish_impl(mbedtls_ccm_context *ctx, unsigned int key_len,
    unsigned char *tag, unsigned int tag_buf_len);

td_s32 unify_uapi_cipher_symc_get_tag(td_handle symc_handle, td_u8 *tag, td_u32 tag_length);

td_s32 unify_uapi_cipher_mac_start(td_handle *symc_handle, const crypto_symc_mac_attr *mac_attr);

td_s32 unify_uapi_cipher_mac_update(td_handle symc_handle, const crypto_buf_attr *src_buf, td_u32 length);

td_s32 unify_uapi_cipher_mac_finish(td_handle symc_handle, td_u8 *mac, td_u32 *mac_length);

#endif