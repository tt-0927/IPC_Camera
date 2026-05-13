/**
  Copyright (c), 2001-2025, Shenshu Tech. Co., Ltd.
 */
#if defined(CONFIG_PKE_ECC_ECDSA_SIGN_SUPPORT)

#include "drv_pke_inner.h"

#include "crypto_drv_common.h"


#define MAX_TRY_TIMES   100
#define MAX_ECC_SIZE        72

static td_s32 inner_ecdsa_sign(const drv_pke_data *priv_key, const drv_pke_data *hash, const drv_pke_ecc_sig *sig)
{
    int ret;
    unsigned char k[MAX_ECC_SIZE];
    unsigned char kp_point_y[MAX_ECC_SIZE];
    drv_pke_data k_data = {
        .data = k,
        .length = priv_key->length
    };
    drv_pke_data r_data = {
        .data = sig->r,
        .length = sig->length
    };
    drv_pke_data s_data = {
        .data = sig->s,
        .length = sig->length
    };
    drv_pke_ecc_point kp_point = {
        .x = sig->r,
        .y = kp_point_y,
        .length = priv_key->length
    };
    const unsigned char *ecc_n = hal_pke_alg_ecc_get_n();

    /* Step 1. Generate k, which is in [1, n - 1] */
    ret = inner_get_limit_random(k, ecc_n, priv_key->length);
    crypto_chk_return(ret != CRYPTO_SUCCESS, ret, "inner_get_limit_random failed\n");
#if defined(CONFIG_PKE_ECC_TRACE_ENABLE)
    crypto_dump_data("random k", k, priv_key->length);
#endif

    /* Step 2. Compute kp = k * P(x, y) */
#if defined(CONFIG_PKE_ECC_TRACE_ENABLE)
    crypto_print("========== Start Compute kp = k * P(x, y) ==========\n");
    crypto_dump_data("k", k, priv_key->length);
#endif
    ret = hal_pke_alg_ecc_base_point_mul(&k_data, &kp_point);
    crypto_chk_return(ret != CRYPTO_SUCCESS, ret, "hal_pke_alg_ecc_base_point_mul failed\n");
#if defined(CONFIG_PKE_ECC_TRACE_ENABLE)
    crypto_dump_data("kp(x)", kp_point.x, kp_point.length);
    crypto_dump_data("kp(y)", kp_point.y, kp_point.length);
    crypto_print("========== End Compute kp = k * P(x, y) ==========\n");
#endif
    /* Step 3. r = kp(x) mod n, if r == 0, return failure and try again. */
    if (inner_drv_is_zero(kp_point.x, kp_point.length)) {
        return PKE_COMPAT_ERRNO(ERROR_ECDSA_SIGN_R_IS_ZERO);
    }
    /* Step 4. compute e = H(m), we can skip the step. */
    /* Step 5. s = k^-1(e + dr) mod n. if s == 0, return failure and try again */
    r_data.data = kp_point.x;
    r_data.length = kp_point.length;
#if defined(CONFIG_PKE_ECC_TRACE_ENABLE)
    crypto_print("========== Start Compute s = k^-1(e + dr) mod n ==========\n");
    crypto_dump_data("k", k, priv_key->length);
    crypto_dump_data("e", hash->data, hash->length);
    crypto_dump_data("d", priv_key->data, priv_key->length);
    crypto_dump_data("r", r_data.data, r_data.length);
#endif
    ret = hal_pke_alg_ecc_compute_s(&k_data, hash, priv_key, &r_data, &s_data);
    crypto_chk_return(ret != CRYPTO_SUCCESS, ret, "hal_pke_alg_ecc_compute_s failed\n");
#if defined(CONFIG_PKE_ECC_TRACE_ENABLE)
    crypto_dump_data("s", s_data.data, s_data.length);
    crypto_print("========== End Compute s = k^-1(e + dr) mod n ==========\n");
#endif
    if (inner_drv_is_zero(s_data.data, s_data.length)) {
        return PKE_COMPAT_ERRNO(ERROR_ECDSA_SIGN_S_IS_ZERO);
    }

    return ret;
}

td_s32 drv_cipher_pke_ecdsa_sign(drv_pke_ecc_curve_type curve_type, const drv_pke_data *priv_key,
    const drv_pke_data *hash, const drv_pke_ecc_sig *sig)
{
    int ret;
    unsigned int i;
    unsigned int klen;
    drv_pke_data hash_data;

    /* param check. */
    pke_null_ptr_chk(priv_key);
    pke_null_ptr_chk(priv_key->data);
    pke_null_ptr_chk(hash);
    pke_null_ptr_chk(hash->data);
    pke_null_ptr_chk(sig);
    pke_null_ptr_chk(sig->r);
    pke_null_ptr_chk(sig->s);

    /* length check. */
    klen = crypto_pke_get_klen(curve_type);
    crypto_chk_return(priv_key->length != klen, PKE_COMPAT_ERRNO(ERROR_INVALID_PARAM),
        "priv_key->length is invalid\n");
    crypto_chk_return(sig->length != klen, PKE_COMPAT_ERRNO(ERROR_INVALID_PARAM),
        "sig->length is invalid\n");

    /* increase security strength check. for nist-fips-224, it's the same security strength with SHA224 */
#if defined(CONFIG_PKE_ECC_SECURITY_STRENGTH_CHECK)
    drv_crypto_pke_check_param((hash->length < klen) &&
        !(hash->length == HASH_SIZE_SHA_224 && klen == DRV_PKE_LEN_224) &&
        !(hash->length == HASH_SIZE_SHA_512 && (klen == DRV_PKE_LEN_521 || klen == DRV_PKE_LEN_576)));
#endif

    hash_data.data = hash->data;
    hash_data.length = crypto_min(priv_key->length, hash->length);
    if (curve_type == DRV_PKE_ECC_TYPE_SM2) {
        return drv_cipher_pke_sm2_sign(priv_key, &hash_data, sig);
    }

    ret = hal_pke_alg_ecc_init(curve_type);
    crypto_chk_return(ret != CRYPTO_SUCCESS, ret, "hal_pke_alg_ecc_init failed\n");

    for (i = 0; i < MAX_TRY_TIMES; i++) {
        ret = inner_ecdsa_sign(priv_key, &hash_data, sig);
        if (ret == PKE_COMPAT_ERRNO(ERROR_ECDSA_SIGN_R_IS_ZERO) ||
            ret == PKE_COMPAT_ERRNO(ERROR_ECDSA_SIGN_S_IS_ZERO)) {
            continue;
        }
        if (ret != CRYPTO_SUCCESS) {
            crypto_log_err("inner_ecdsa_sign failed\n");
            goto exit_deinit;
        } else {
            ret = CRYPTO_SUCCESS;
            goto exit_deinit;
        }
    }

    crypto_log_err("pke sign try max time!\n");
    ret = PKE_COMPAT_ERRNO(ERROR_TRY_TIMES);
exit_deinit:
    hal_pke_alg_ecc_deinit();
    return ret;
}
#endif