/**
  Copyright (c), 2001-2025, Shenshu Tech. Co., Ltd.
 */
#include "crypto_hash_common.h"

static crypto_mutex g_hash_mutex;

#if defined(CONFIG_HASH_SHA1_SUPPORT)
// SHA-1, the initial hash value
const td_u8 g_sha1_ival[20] = {
    0x67, 0x45, 0x23, 0x01,
    0xEF, 0xCD, 0xAB, 0x89,
    0x98, 0xBA, 0xDC, 0xFE,
    0x10, 0x32, 0x54, 0x76,
    0xC3, 0xD2, 0xE1, 0xF0
};
#endif

#if defined(CONFIG_HASH_SHA224_SUPPORT)
// SHA-224, the initial hash value
static const td_u8 g_sha224_ival[32] = {
    0xC1, 0x05, 0x9E, 0xD8,
    0x36, 0x7C, 0xD5, 0x07,
    0x30, 0x70, 0xDD, 0x17,
    0xF7, 0x0E, 0x59, 0x39,
    0xFF, 0xC0, 0x0B, 0x31,
    0x68, 0x58, 0x15, 0x11,
    0x64, 0xF9, 0x8F, 0xA7,
    0xBE, 0xFA, 0x4F, 0xA4
};
#endif

#if defined(CONFIG_HASH_SHA256_SUPPORT)
// SHA-256, the initial hash value
static const td_u8 g_sha256_ival[32] = {
    0x6A, 0x09, 0xE6, 0x67,
    0xBB, 0x67, 0xAE, 0x85,
    0x3C, 0x6E, 0xF3, 0x72,
    0xA5, 0x4F, 0xF5, 0x3A,
    0x51, 0x0E, 0x52, 0x7F,
    0x9B, 0x05, 0x68, 0x8C,
    0x1F, 0x83, 0xD9, 0xAB,
    0x5B, 0xE0, 0xCD, 0x19
};
#endif

#if defined(CONFIG_HASH_SHA384_SUPPORT)
// SHA-384, the initial hash value
static const td_u8 g_sha384_ival[64] = {
    0xCB, 0xBB, 0x9D, 0x5D, 0xC1, 0x05, 0x9E, 0xD8,
    0x62, 0x9A, 0x29, 0x2A, 0x36, 0x7C, 0xD5, 0x07,
    0x91, 0x59, 0x01, 0x5A, 0x30, 0x70, 0xDD, 0x17,
    0x15, 0x2F, 0xEC, 0xD8, 0xF7, 0x0E, 0x59, 0x39,
    0x67, 0x33, 0x26, 0x67, 0xFF, 0xC0, 0x0B, 0x31,
    0x8E, 0xB4, 0x4A, 0x87, 0x68, 0x58, 0x15, 0x11,
    0xDB, 0x0C, 0x2E, 0x0D, 0x64, 0xF9, 0x8F, 0xA7,
    0x47, 0xB5, 0x48, 0x1D, 0xBE, 0xFA, 0x4F, 0xA4
};
#endif

#if defined(CONFIG_HASH_SHA512_SUPPORT)
// SHA-512, the initial hash value
static const td_u8 g_sha512_ival[64] = {
    0x6A, 0x09, 0xE6, 0x67, 0xF3, 0xBC, 0xC9, 0x08,
    0xBB, 0x67, 0xAE, 0x85, 0x84, 0xCA, 0xA7, 0x3B,
    0x3C, 0x6E, 0xF3, 0x72, 0xFE, 0x94, 0xF8, 0x2B,
    0xA5, 0x4F, 0xF5, 0x3A, 0x5F, 0x1D, 0x36, 0xF1,
    0x51, 0x0E, 0x52, 0x7F, 0xAD, 0xE6, 0x82, 0xD1,
    0x9B, 0x05, 0x68, 0x8C, 0x2B, 0x3E, 0x6C, 0x1F,
    0x1F, 0x83, 0xD9, 0xAB, 0xFB, 0x41, 0xBD, 0x6B,
    0x5B, 0xE0, 0xCD, 0x19, 0x13, 0x7E, 0x21, 0x79
};
#endif

#if defined(CONFIG_HASH_SM3_SUPPORT)
// SM3, the initial hash value
static const td_u8 g_sm3_ival[32] = {
    0x73, 0x80, 0x16, 0x6F,
    0x49, 0x14, 0xB2, 0xB9,
    0x17, 0x24, 0x42, 0xD7,
    0xDA, 0x8A, 0x06, 0x00,
    0xA9, 0x6F, 0x30, 0xBC,
    0x16, 0x31, 0x38, 0xAA,
    0xE3, 0x8D, 0xEE, 0x4D,
    0xB0, 0xFB, 0x0E, 0x4E
};
#endif

typedef struct {
    crypto_hash_type hash_type;
    const td_u8 *state_val;
    td_u32 state_length;
} inner_drv_hash_state_iv_table_t;

static inner_drv_hash_state_iv_table_t g_state_iv_table[] = {
#if defined(CONFIG_HASH_SHA1_SUPPORT)
    {
        .hash_type = CRYPTO_HASH_TYPE_SHA1,
        .state_val = g_sha1_ival,
        .state_length = sizeof(g_sha1_ival)
    },
#endif
#if defined(CONFIG_HASH_SHA224_SUPPORT)
    {
        .hash_type = CRYPTO_HASH_TYPE_SHA224,
        .state_val = g_sha224_ival,
        .state_length = sizeof(g_sha224_ival)
    },
#endif
#if defined(CONFIG_HASH_SHA256_SUPPORT)
    {
        .hash_type = CRYPTO_HASH_TYPE_SHA256,
        .state_val = g_sha256_ival,
        .state_length = sizeof(g_sha256_ival)
    },
#endif
#if defined(CONFIG_HASH_SHA384_SUPPORT)
    {
        .hash_type = CRYPTO_HASH_TYPE_SHA384,
        .state_val = g_sha384_ival,
        .state_length = sizeof(g_sha384_ival)
    },
#endif
#if defined(CONFIG_HASH_SHA512_SUPPORT)
    {
        .hash_type = CRYPTO_HASH_TYPE_SHA512,
        .state_val = g_sha512_ival,
        .state_length = sizeof(g_sha512_ival)
    },
#endif
#if defined(CONFIG_HASH_SM3_SUPPORT)
    {
        .hash_type = CRYPTO_HASH_TYPE_SM3,
        .state_val = g_sm3_ival,
        .state_length = sizeof(g_sm3_ival)
    },
#endif
};

td_s32 drv_hash_get_state_iv(crypto_hash_type hash_type, td_u32 *iv_size, td_u32 *state_iv, td_u32 state_iv_len)
{
    const td_u8 *state_val = TD_NULL;
    td_s32 ret;
    td_u32 length = 0;
    td_u32 type = crypto_hash_remove_hmac_flag(hash_type);
    td_u32 i;

    crypto_chk_return(state_iv == TD_NULL,
        DRV_COMPAT_ERRNO(ERROR_MODULE_HASH, ERROR_INVALID_PARAM), "state_iv is NULL!\n");

    for (i = 0; i < crypto_array_size(g_state_iv_table); i++) {
        if (type == g_state_iv_table[i].hash_type) {
            state_val = g_state_iv_table[i].state_val;
            length = g_state_iv_table[i].state_length;
            break;
        }
    }
    if (state_val == TD_NULL) {
        crypto_log_err("Invalid Hash Mode!\n");
        return DRV_COMPAT_ERRNO(ERROR_MODULE_HASH, ERROR_INVALID_PARAM);
    }

    ret = memcpy_s(state_iv, state_iv_len, state_val, length);
    crypto_chk_return(ret != EOK, DRV_COMPAT_ERRNO(ERROR_MODULE_HASH, ERROR_MEMCPY_S), "memcpy_s failed\n");

    if (iv_size != TD_NULL) {
        *iv_size = length;
    }
    return TD_SUCCESS;
}

td_s32 hash_mutex_init(td_void)
{
    return crypto_mutex_init(&g_hash_mutex);
}

td_void hash_mutex_destroy(td_void)
{
    return crypto_mutex_destroy(&g_hash_mutex);
}

td_void hash_common_lock(td_void)
{
    crypto_mutex_lock(&g_hash_mutex);
}

td_void hash_common_unlock(td_void)
{
    crypto_mutex_unlock(&g_hash_mutex);
}