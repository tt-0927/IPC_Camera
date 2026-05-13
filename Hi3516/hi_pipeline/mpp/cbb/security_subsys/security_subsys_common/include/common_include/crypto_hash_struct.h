/**
  Copyright (c), 2001-2025, Shenshu Tech. Co., Ltd.
 */

#ifndef CRYPTO_HASH_STRUCT_H
#define CRYPTO_HASH_STRUCT_H

#include "ot_common_crypto.h"
#include "crypto_common_struct.h"

#define CRYPTO_HASH_BLOCK_SIZE_20BYTE       0x14
#define CRYPTO_HASH_BLOCK_SIZE_32BYTE       0x20
#define CRYPTO_HASH_BLOCK_SIZE_64BYTE       0x40

#define crypto_hash_get_attr(value, mask, shift)         (((td_u32)(value) & (td_u32)(mask)) >> (shift))
#define crypto_hash_macth(value, mask, target, shift)    (crypto_hash_get_attr(value, mask, shift) == (target))

#define crypto_hash_get_alg(hash_type)              \
    crypto_hash_get_attr(hash_type, CRYPTO_HASH_ALG_MASK, CRYPTO_HASH_ALG_SHIFT)
#define crypto_hash_get_mode(hash_type)             \
    crypto_hash_get_attr(hash_type, CRYPTO_HASH_MODE_MASK, CRYPTO_HASH_MODE_SHIFT)
#define crypto_hash_is_hmac(hash_type)              \
    crypto_hash_macth(hash_type, CRYPTO_IS_HMAC_MASK, CRYPTO_HMAC_TYPE, CRYPTO_IS_HMAC_SHIFT)
#define crypto_hash_get_message_len(hash_type)      \
    (1 << crypto_hash_get_attr(hash_type, CRYPTO_HASH_MAX_MESSAGE_LEN_MASK, CRYPTO_HASH_MAX_MESSAGE_LEN_SHIFT))
#define crypto_hash_get_block_size(hash_type)       \
    (1 << crypto_hash_get_attr(hash_type, CRYPTO_HASH_BLOCK_SIZE_MASK, CRYPTO_HASH_BLOCK_SIZE_SHIFT))
#define crypto_hash_get_result_size(hash_type)      \
    crypto_hash_get_attr(hash_type, CRYPTO_HASH_RESULT_SIZE_MASK, CRYPTO_HASH_RESULT_SIZE_SHIFT)
#define crypto_hash_remove_hmac_flag(hash_type)     \
    ((hash_type) & (0x0FFFFFFF))

typedef struct {
    crypto_hash_type hmac_type;
    td_u8 *salt;
    td_u32 salt_length;
    td_u8 *ikm;
    td_u32 ikm_length;
} crypto_hkdf_extract_t;

typedef struct {
    crypto_hash_type hmac_type;
    td_u8 *prk;
    td_u32 prk_length;
    td_u8 *info;
    td_u32 info_length;
} crypto_hkdf_expand_t;

typedef struct {
    crypto_hash_type hmac_type;
    td_u8 *salt;
    td_u32 salt_length;
    td_u8 *ikm;
    td_u32 ikm_length;
    td_u8 *info;
    td_u32 info_length;
} crypto_hkdf_t;

#endif
