/**
  Copyright (c), 2001-2025, Shenshu Tech. Co., Ltd.
*/
#ifndef CRYPTO_KM_STRUCT_H
#define CRYPTO_KM_STRUCT_H

#include "ot_common_crypto.h"
#include "crypto_common_struct.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif  /* __cplusplus */

/* Define the flash online decryption key type it's valid for non-TEE platform,  it's will be ignored on TEE platform */
typedef enum {
    KM_KLAD_FLASH_KEY_TYPE_REE_DEC = 0x00,  /* REE flash online decryption key */
    KM_KLAD_FLASH_KEY_TYPE_TEE_DEC,         /* TEE flash online decryption key */
    KM_KLAD_FLASH_KEY_TYPE_TEE_AUT,         /* TEE flash online authentication key */
    KM_KLAD_FLASH_KEY_TYPE_MAX,
    KM_KLAD_FLASH_KEY_TYPE_INVALID = 0xffffffff,
} km_klad_flash_key_type;

typedef enum {
    CRYPTO_KDF_HARD_KEY_SIZE_128BIT = 0,
    CRYPTO_KDF_HARD_KEY_SIZE_192BIT,
    CRYPTO_KDF_HARD_KEY_SIZE_256BIT,
} crypto_kdf_hard_key_size;

/**
 * @brief  Root key selection during KDF key derivation.
 */
typedef enum {
    CRYPTO_KDF_OTP_KEY_MRK1 = 0,
    CRYPTO_KDF_OTP_KEY_USK,
    CRYPTO_KDF_OTP_KEY_RUSK,
    CRYPTO_KDF_OTP_KEY_MAX
} crypto_kdf_otp_key;
 
/**
 * @brief  Symmetric algorithm selection during KDF key derivation.
 */
typedef enum {
    CRYPTO_KDF_UPDATE_ALG_AES = 0,
    CRYPTO_KDF_UPDATE_ALG_SM4,
    CRYPTO_KDF_UPDATE_ALG_MAX
} crypto_kdf_update_alg;
 
/**
 * @brief  Hash algorithm selection when the software PBKDF2 algorithm is used.
 */
typedef enum {
    CRYPTO_KDF_SW_ALG_SHA1 = 0,
    CRYPTO_KDF_SW_ALG_SHA256,
    CRYPTO_KDF_SW_ALG_SHA384,
    CRYPTO_KDF_SW_ALG_SHA512,
    CRYPTO_KDF_SW_ALG_SM3
} crypto_kdf_sw_alg;

typedef struct {
    crypto_kdf_hard_key_type hard_key_type;
    crypto_kdf_hard_alg hard_alg;
    crypto_kdf_hard_key_size hard_key_size;
    td_u32 rkp_sw_cfg;
    td_u8 *salt;
    td_u32 salt_length;
    td_bool is_oneway;
} crypto_kdf_hard_calc_param;
 
/**
 * @brief  The klad target module's algorithm engine, determining the algorithm supported by the sent key.
 */
typedef enum {
    CRYPTO_KLAD_ENGINE_AES = 0x20,
    CRYPTO_KLAD_ENGINE_LEA = 0x40,
    CRYPTO_KLAD_ENGINE_SM4 = 0x50,
    CRYPTO_KLAD_ENGINE_TDES = 0x70,
    CRYPTO_KLAD_ENGINE_SHA1_HMAC = 0xA0,
    CRYPTO_KLAD_ENGINE_SHA2_HMAC = 0xA1,
    CRYPTO_KLAD_ENGINE_SM3_HMAC = 0xA2,
    CRYPTO_KLAD_ENGINE_MAX
} crypto_klad_engine;

/**
 * @brief  The klad target module, determining the module to which the key is sent.
 */
typedef enum {
    CRYPTO_KLAD_DEST_MCIPHER = 0,
    CRYPTO_KLAD_DEST_HMAC,
    CRYPTO_KLAD_DEST_FLASH,
    CRYPTO_KLAD_DEST_NPU,
    CRYPTO_KLAD_DEST_AIDSP,
    CRYPTO_KLAD_DEST_MAX,
} crypto_klad_dest;
 
/**
 * @brief  Flash online decryption mode, determining the mode used after the key is sent.
 */
typedef enum {
    CRYPTO_KLAD_FLASH_KEY_TYPE_REE_DEC = 0x00,  /* REE flash online decryption key */
    CRYPTO_KLAD_FLASH_KEY_TYPE_TEE_DEC,         /* TEE flash online decryption key */
    CRYPTO_KLAD_FLASH_KEY_TYPE_TEE_AUT,         /* TEE flash online authentication key */
    CRYPTO_KLAD_FLASH_KEY_TYPE_INVALID,
} crypto_klad_flash_key_type;
 
/**
 * @brief  When the target engine is HMAC, determine the HAMC algorithm to be used.
 */
typedef enum {
    CRYPTO_KLAD_HMAC_TYPE_SHA1 = 0x20, /* Insecure algorithm, not recommended. */
    CRYPTO_KLAD_HMAC_TYPE_SHA224,
    CRYPTO_KLAD_HMAC_TYPE_SHA256,
    CRYPTO_KLAD_HMAC_TYPE_SHA384,
    CRYPTO_KLAD_HMAC_TYPE_SHA512,
    CRYPTO_KLAD_HMAC_TYPE_SM3 = 0x30,
    CRYPTO_KLAD_HMAC_TYPE_MAX,
    CRYPTO_KLAD_HMAC_TYPE_INVALID = 0xffffffff,
} crypto_klad_hmac_type;
 
/**
 * @brief  Determines the current derived key level during klad key derivation.
 */
typedef enum {
    CRYPTO_KLAD_LEVEL_SEL_FIRST = 0,
    CRYPTO_KLAD_LEVEL_SEL_SECOND
} crypto_klad_level_sel;
 
/**
 * @brief  Determines the symmetric algorithm used for derivation during klad key derivation.
 */
typedef enum {
    CRYPTO_KLAD_ALG_SEL_TDES = 0,
    CRYPTO_KLAD_ALG_SEL_AES,
    CRYPTO_KLAD_ALG_SEL_SM4,
    CRYPTO_KLAD_ALG_SEL_MAX,
} crypto_klad_alg_sel;
 
/**
 * @brief  Clear key structure when klad sends a clear key.
 */
typedef struct {
    crypto_klad_hmac_type hmac_type; /* Indicates the HMAC algorithm.
                                            Valid only when the target is the HMAC algorithm engine. */
    td_u32 key_length;  /* Length of the clear key, in bytes.
                                For the symmetric algorithm, the value can only be 16, 24, or 32.
                                For HMAC-SH1/SHA224/SHA256/SM3, the value cannot exceed 64.
                                For HMAC-SHA384/SHA512, the value cannot exceed 128. */
    td_u8 *key;     /* Clear key content. */
    td_bool key_parity; /* Indicates the parity attribute of a key.
                                Valid when the target is a symmetric algorithm engine and key_length is set to 16. */
} crypto_klad_clear_key;

/**
 * @brief  Keyladder root key type selection.
 */
typedef struct {
    crypto_kdf_hard_key_type rootkey_type;
} crypto_klad_config;
 
/**
 * @brief  Keyladder working key attribute configuration.
 */
typedef struct {
    crypto_klad_engine engine;  /* The working key can be used for which algorithm of the crypto engine. */
    td_bool decrypt_support;    /* The working key can be used for decrypting. */
    td_bool encrypt_support;    /* The working key can be used for encrypting. */
} crypto_klad_key_config;
 
/**
 * @brief  Security attribute of the key.
    when cipher work mode is CBC_MAC, dest_buf_sec_support and dest_buf_non_sec_support cannot be both false
 */
typedef struct {
    td_bool key_sec;    /* Secure key can only be used by TEE CPU and AIDSP locked cipher and hash channel. */
    td_bool master_only_enable; /* Only the cipher or hash channel which is locked by same CPU as keyladder
                                        can use this key, valid only for TEE CPU and AIDSP. */
    td_bool dest_buf_sec_support;   /* The destination buffer of target engine can be secure. */
    td_bool dest_buf_non_sec_support; /* The destination buffer of target engine can be secure. */
    td_bool src_buf_sec_support;      /* The destination buffer of target engine can be secure. */
    td_bool src_buf_non_sec_support;  /* The destination buffer of target engine can be secure. */
} crypto_klad_key_secure_config;
 
/**
 * @brief  Keyladder configuration attributes.
 */
typedef struct {
    crypto_klad_config klad_cfg;    /* The keyladder configuration, valid for harware key. */
    crypto_klad_key_config key_cfg; /* The working key configuration. */
    crypto_klad_key_secure_config key_sec_cfg;  /* The working key security configuration. */
    td_u32 rkp_sw_cfg;
} crypto_klad_attr;

typedef struct {
    td_u8 key[16];
    td_u32 key_length;
    crypto_klad_level_sel level;
    crypto_klad_alg_sel alg;
} crypto_klad_session_key;

typedef struct {
    td_u32 key_length;
    crypto_klad_alg_sel alg;
    td_bool key_parity;
    td_u8 key[32];
} crypto_klad_content_key;

typedef struct {
    crypto_kdf_hard_alg kdf_hard_alg;
    td_bool key_parity;
    crypto_klad_key_size key_size;
    td_u8 *salt;
    td_u32 salt_length;
    td_bool oneway;
} crypto_klad_effective_key;

/**
 * @brief  Keyslot type selection.
 */
typedef enum {
    CRYPTO_KEYSLOT_TYPE_MCIPHER = 0,
    CRYPTO_KEYSLOT_TYPE_HMAC,
    CRYPTO_KEYSLOT_TYPE_FLASH,
} crypto_keyslot_type;

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif  /* __cplusplus */

#endif /* OT_KM_STRUCT_H */
