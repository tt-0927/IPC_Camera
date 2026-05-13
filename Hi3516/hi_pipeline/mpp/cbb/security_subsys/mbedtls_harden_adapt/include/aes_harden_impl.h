/**
  Copyright (c), 2001-2025, Shenshu Tech. Co., Ltd.
*/

#ifndef AES_HARDEN_IMPL_H
#define AES_HARDEN_IMPL_H

#include "ot_type.h"
#include "crypto_symc_struct.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif /* __cplusplus */
#endif /* __cplusplus */

typedef struct kslot_info {
    td_handle kslot_handle;
    unsigned int key_len;
    unsigned char is_used;
} kslot_info;

int aes_harden_buff_alloc(void);

void aes_harden_buff_free(void);

int mbedtls_alt_aes_create_key_impl(td_handle *keyslot_handle);

int mbedtls_alt_aes_set_key_impl(td_handle keyslot_handle, const unsigned char *key, unsigned int key_len);

int mbedtls_alt_aes_destroy_key_impl(const td_handle keyslot_handle);

int mbedtls_alt_aes_encrypt_impl(const td_handle keyslot_handle, const unsigned char *key, unsigned int key_len,
    const unsigned char src[16], unsigned char dst[16]);

int mbedtls_alt_aes_decrypt_impl(const td_handle keyslot_handle, const unsigned char *key, unsigned int key_len,
    const unsigned char src[16], unsigned char dst[16]);

int mbedtls_alt_aes_cbc_encrypt_impl(const td_handle keyslot_handle, const unsigned char *key, unsigned int key_len,
    unsigned char iv[16], const unsigned char *src, unsigned char *dst, unsigned int data_len);

int mbedtls_alt_aes_cbc_decrypt_impl(const td_handle keyslot_handle, const unsigned char *key,
    unsigned int key_len, unsigned char iv[16], const unsigned char *src, unsigned char *dst, unsigned int data_len);

int mbedtls_alt_aes_cfb128_encrypt_impl(const td_handle keyslot_handle, const unsigned char *key, unsigned int key_len,
    unsigned char iv[16], const unsigned char *src, unsigned char *dst, unsigned int data_len);

int mbedtls_alt_aes_cfb128_decrypt_impl(const td_handle keyslot_handle, const unsigned char *key, unsigned int key_len,
    unsigned char iv[16], const unsigned char *src, unsigned char *dst, unsigned int data_len);

int mbedtls_alt_aes_cfb8_encrypt_impl(const td_handle keyslot_handle, const unsigned char *key, unsigned int key_len,
    unsigned char iv[16], const unsigned char *src, unsigned char *dst, unsigned int data_len);

int mbedtls_alt_aes_cfb8_decrypt_impl(const td_handle keyslot_handle, const unsigned char *key, unsigned int key_len,
    unsigned char iv[16], const unsigned char *src, unsigned char *dst, unsigned int data_len);

int mbedtls_alt_aes_ofb_crypt_impl(const td_handle keyslot_handle, const unsigned char *key, unsigned int key_len,
    unsigned char *iv_off, unsigned char iv[16], const unsigned char *src, unsigned char *dst, unsigned int data_len);

int mbedtls_alt_aes_ctr_crypt_impl(const td_handle keyslot_handle, const unsigned char *key, unsigned int key_len,
    unsigned char *nc_off, unsigned char nonce_counter[16], unsigned char stream_block[16],
    const unsigned char *src, unsigned char *dst, unsigned int data_len);

int mbedtls_alt_aes_ccm_start_impl(unsigned long long *phys_addr, void **vir_addr);

int mbedtls_alt_aes_ccm_update_ad_impl(mbedtls_ccm_context *ctx, const unsigned char *add, unsigned int add_len);

int mbedtls_alt_aes_ccm_update_impl(mbedtls_ccm_context *ctx, const unsigned char *input, unsigned int input_len,
    unsigned char *output, unsigned int output_len);

int mbedtls_alt_aes_ccm_finish_impl(mbedtls_ccm_context *ctx, unsigned char *tag, unsigned int tag_buf_len);

#ifdef __cplusplus
#if __cplusplus
}
#endif /* __cplusplus */
#endif /* __cplusplus */

#endif