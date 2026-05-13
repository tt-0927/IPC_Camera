/**
  Copyright (c), 2001-2025, Shenshu Tech. Co., Ltd.
*/

#ifndef HASH_HARDEN_IMPL_H
#define HASH_HARDEN_IMPL_H

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif /* __cplusplus */
#endif /* __cplusplus */

#define MBEDTLS_ALT_HASH_RESULT_SIZE_MAX_IN_WORD    16 // for SHA-512
#define MBEDTLS_ALT_HASH_BLOCK_SIZE_MAX             128 // for SHA-512
#define HASH_INIT_SHA_MAX_LENGTH                    64

typedef enum {
    MBEDTLS_ALT_HASH_TYPE_SHA1 = 0xf690a0,
    MBEDTLS_ALT_HASH_TYPE_SHA224 = 0x10690e0,
    MBEDTLS_ALT_HASH_TYPE_SHA256 = 0x1169100,
    MBEDTLS_ALT_HASH_TYPE_SHA384 = 0x127a180,
    MBEDTLS_ALT_HASH_TYPE_SHA512 = 0x137a200,
    MBEDTLS_ALT_HASH_TYPE_SM3 = 0x2169100,

    MBEDTLS_ALT_HASH_TYPE_HMAC_SHA1 = 0x10f690a0,
    MBEDTLS_ALT_HASH_TYPE_HMAC_SHA224 = 0x110690e0,
    MBEDTLS_ALT_HASH_TYPE_HMAC_SHA256 = 0x11169100,
    MBEDTLS_ALT_HASH_TYPE_HMAC_SHA384 = 0x1127a180,
    MBEDTLS_ALT_HASH_TYPE_HMAC_SHA512 = 0x1137a200,
    MBEDTLS_ALT_HASH_TYPE_HMAC_SM3 = 0x12169100,

    MBEDTLS_ALT_HASH_TYPE_INVALID = 0xffffffff,
} mbedtls_alt_hash_type;

typedef struct {
    unsigned int length[2];
    unsigned int state[MBEDTLS_ALT_HASH_RESULT_SIZE_MAX_IN_WORD];
    unsigned int tail_len;
    mbedtls_alt_hash_type hash_type;
    unsigned char o_key_pad[MBEDTLS_ALT_HASH_BLOCK_SIZE_MAX];
    unsigned char i_key_pad[MBEDTLS_ALT_HASH_BLOCK_SIZE_MAX];
    unsigned char tail[MBEDTLS_ALT_HASH_BLOCK_SIZE_MAX];
} mbedtls_alt_hash_clone_ctx;

int mbedtls_alt_hash_start_adapt(mbedtls_alt_hash_clone_ctx *clone_ctx, mbedtls_alt_hash_type hash_type);

int mbedtls_alt_hash_update_adapt(mbedtls_alt_hash_clone_ctx *clone_ctx,
    const unsigned char *data, unsigned int data_len);

int mbedtls_alt_hash_finish_adapt(mbedtls_alt_hash_clone_ctx *clone_ctx, unsigned char *out, unsigned int *out_len);

#ifdef __cplusplus
#if __cplusplus
}
#endif /* __cplusplus */
#endif /* __cplusplus */

#endif