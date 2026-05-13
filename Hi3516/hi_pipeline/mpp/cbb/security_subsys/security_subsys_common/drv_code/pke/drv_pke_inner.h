/**
  Copyright (c), 2001-2025, Shenshu Tech. Co., Ltd.
 */

#ifndef DRV_PKE_INNER_H
#define DRV_PKE_INNER_H

#include "drv_pke.h"
#include "hal_pke_alg.h"

typedef struct {
    td_u32 klen;
    td_u32 em_bit;
    td_u8 *em;
    td_u32 em_len;
    td_u8 *hash;
    td_u32 hash_len;
    td_u8 *data;
    td_u32 data_len;
} rsa_pkcs1_pack;

typedef struct {
    drv_pke_hash_type hash_type;
    td_u32 hash_len;
    td_u8 *lhash_data;
    td_u8 *asn1_data;
    td_u32 asn1_len;
} rsa_pkcs1_hash_info;

/* result size */
#define HASH_SIZE_SHA_1                            20
#define HASH_SIZE_SHA_224                          28
#define HASH_SIZE_SHA_256                          32
#define HASH_SIZE_SHA_384                          48
#define HASH_SIZE_SHA_512                          64
#define HASH_SIZE_SHA_MAX                          64

#define PKE_COMPAT_ERRNO(err_code)      DRV_COMPAT_ERRNO(ERROR_MODULE_PKE, err_code)

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif /* __cplusplus */
#endif /* __cplusplus */

/* Common. */
td_s32 inner_get_random(td_u8 *rand, const td_u32 size);

td_s32 inner_get_random_with_nonzero_octets(td_u8 *random, td_u32 size);

td_s32 inner_get_limit_random(td_u8 *rand, const td_u8 *limit, const td_u32 size);

td_s32 inner_generate_random_in_range(td_u8 *rand, const td_u8 *limit, const td_u32 size);

/* RSA Common */
td_s32 drv_get_hash_len(const drv_pke_hash_type hash_type);

td_u32 rsa_get_bit_num(const td_u8 *big_num, td_u32 num_len);

td_s32 pkcs1_get_hash(const drv_pke_hash_type hash_type, const drv_pke_data *label,
    rsa_pkcs1_hash_info *hash_info);

/* out = in^k mod n. */
td_s32 inner_rsa_exp_mod(const td_u8 *n, const td_u8 *k, td_u32 key_len,
    const td_u8 *in, td_u8 *out);

/* c = (a + b) mod n.
 * a or b could be NULL, if a = NULL, means a = 0.
 */
td_s32 inner_add_mod(const td_u8 *a, const td_u8 *b, const td_u8 *n, td_u8 *c, td_u32 length);

/* RSA V15. */
td_s32 pkcs1_v15_sign(const drv_pke_hash_type hash_type, const rsa_pkcs1_pack *pack);

td_s32 pkcs1_v15_verify(const drv_pke_hash_type hash_type, const rsa_pkcs1_pack *pack);

td_s32 pkcs1_v15_encrypt(const rsa_pkcs1_pack *pack);

td_s32 pkcs1_v15_decrypt(rsa_pkcs1_pack *pack);

/* RSA V21. */
td_s32 pkcs1_pss_sign(const drv_pke_hash_type hash_type, const rsa_pkcs1_pack *pack);

td_s32 pkcs1_pss_verify(const drv_pke_hash_type hash_type, const rsa_pkcs1_pack *pack);

td_s32 pkcs1_oaep_encrypt(const drv_pke_hash_type hash_type, const rsa_pkcs1_pack *pack,
    const drv_pke_data *label);

td_s32 pkcs1_oaep_decrypt(const drv_pke_hash_type hash_type, rsa_pkcs1_pack *pack,
    const drv_pke_data *label);

/* r = k * p. */
td_s32 inner_drv_pke_ecc_mul_dot(const drv_pke_ecc_curve *ecc, const td_u8 *k, const drv_pke_ecc_point *p,
    const drv_pke_ecc_point *r);

/* c = (a + b) mod p. */
td_s32 inner_drv_pke_add_mod(const td_u8 *a, const td_u8 *b, const td_u8 *p, td_u8 *c, td_u32 length);

td_bool inner_drv_is_zero(const td_u8 *val, td_u32 length);

td_bool inner_drv_is_in_range(const uint8_t *value, const uint8_t *range, uint32_t len);

td_s32 inner_sm2_kdf(const drv_pke_ecc_point *param, td_u8 *out, const td_u32 klen);

td_s32 crypto_rsa_klen_support(td_u32 klen);

#ifdef __cplusplus
#if __cplusplus
}
#endif /* __cplusplus */
#endif /* __cplusplus */

#endif