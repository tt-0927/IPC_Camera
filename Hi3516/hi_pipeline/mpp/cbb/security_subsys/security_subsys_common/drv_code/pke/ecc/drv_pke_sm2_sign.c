/**
  Copyright (c), 2001-2025, Shenshu Tech. Co., Ltd.
 */
#if defined(CONFIG_PKE_ECC_SM2_SIGN_SUPPORT)
#include "drv_pke_inner.h"

#include "crypto_drv_common.h"


#define MAX_TRY_TIMES   100
#define SM2_KEY_SIZE        32

/* According to SM2 Elliptic Curve Public Key cryptographic algorithm.
 * "Section 6.1. Digital Signature Generation Algorithm"
 */
static td_s32 inner_sm2_sign(const drv_pke_data *priv_key, const drv_pke_data *hash, const drv_pke_ecc_sig *sig)
{
    int ret;
    unsigned char k[SM2_KEY_SIZE];
    unsigned char kg_buf[SM2_KEY_SIZE * 2];
    drv_pke_data k_data = {
        .data = k,
        .length = SM2_KEY_SIZE
    };
    drv_pke_ecc_point kg_point = {
        .x = kg_buf,
        .y = kg_buf + SM2_KEY_SIZE,
        .length = SM2_KEY_SIZE
    };
    drv_pke_data r_data = {
        .data = sig->r,
        .length = SM2_KEY_SIZE
    };
    drv_pke_data x1_data = {
        .data = kg_point.x,
        .length = SM2_KEY_SIZE
    };
    drv_pke_data s_data = {
        .data = sig->s,
        .length = SM2_KEY_SIZE
    };
    const unsigned char *sm2_n = hal_pke_alg_ecc_get_n();

    /* Step A1~A2: compute e = H(ZA || M), we can skip. */
    /* Step A3. generate k in [1, n - 1] */
    ret = inner_generate_random_in_range(k, sm2_n, priv_key->length);
    crypto_chk_return(ret != CRYPTO_SUCCESS, ret, "inner_drv_generate_random_in_range failed\n");
#if defined(CONFIG_PKE_ECC_TRACE_ENABLE)
    crypto_dump_data("random k", k, priv_key->length);
#endif
    /* Step A4. compute k * G. */
#if defined(CONFIG_PKE_ECC_TRACE_ENABLE)
    crypto_print("========== Start Compute kg = k * G ==========\n");
    crypto_dump_data("k", k, priv_key->length);
#endif
    ret = hal_pke_alg_ecc_base_point_mul(&k_data, &kg_point);
    crypto_chk_return(ret != CRYPTO_SUCCESS, ret, "hal_pke_alg_ecc_base_point_mul failed\n");
#if defined(CONFIG_PKE_ECC_TRACE_ENABLE)
    crypto_dump_data("kg(x)", kg_point.x, kg_point.length);
    crypto_dump_data("kg(y)", kg_point.y, kg_point.length);
    crypto_print("========== End Compute kg = k * G ==========\n");
#endif
    /* Step A5. compute r = (e + x1) mod n, x1 is kg(x), if r = 0 or r + k = n, continue. */
#if defined(CONFIG_PKE_ECC_TRACE_ENABLE)
    crypto_print("========== Start Compute r = (e + x1) mod n ==========\n");
    crypto_dump_data("e", hash->data, hash->length);
    crypto_dump_data("x1", x1_data.data, x1_data.length);
#endif
    ret = hal_pke_alg_sm2_add_mod(hash, &x1_data, &r_data);
    crypto_chk_return(ret != CRYPTO_SUCCESS, ret, "hal_pke_alg_sm2_add_mod failed\n");
#if defined(CONFIG_PKE_ECC_TRACE_ENABLE)
    crypto_dump_data("r", r_data.data, r_data.length);
    crypto_print("========== End Compute r = (e + x1) mod n==========\n");
#endif
    /* check if r is zero. */
    if (inner_drv_is_zero(r_data.data, r_data.length)) {
        crypto_log_err("r is zero!\n");
        return CRYPTO_FAILURE;
    }
    /* check if (r + k) mod n is zero, which means r + k = n. */
    ret = hal_pke_alg_sm2_add_mod(&r_data, &k_data, &s_data);
    crypto_chk_return(ret != CRYPTO_SUCCESS, ret, "hal_pke_alg_sm2_add_mod failed\n");
    if (inner_drv_is_zero(s_data.data, s_data.length)) {
        crypto_log_err("s is zero!\n");
        return CRYPTO_FAILURE;
    }
    /* Step A6. compute s = (((1 + d)^-1) * (k - r * d)) mod n. */
#if defined(CONFIG_PKE_ECC_TRACE_ENABLE)
    crypto_print("========== Start Compute s = (((1 + d)^-1) * (k - r * d)) mod n ==========\n");
    crypto_dump_data("d", priv_key->data, priv_key->length);
    crypto_dump_data("k", k_data.data, k_data.length);
    crypto_dump_data("r", r_data.data, r_data.length);
#endif

    ret = hal_pke_alg_sm2_compute_s(priv_key, &k_data, &r_data, &s_data);
    crypto_chk_return(ret != CRYPTO_SUCCESS, ret, "hal_pke_alg_sm2_compute_s failed\n");
#if defined(CONFIG_PKE_ECC_TRACE_ENABLE)
    crypto_dump_data("s", s_data.data, s_data.length);
    crypto_print("========== End Compute s = (((1 + d)^-1) * (k - r * d)) mod n==========\n");
#endif
    return ret;
}

td_s32 drv_cipher_pke_sm2_sign(const drv_pke_data *priv_key, const drv_pke_data *hash, const drv_pke_ecc_sig *sig)
{
    int ret;
    unsigned int i;
    unsigned int klen = DRV_PKE_LEN_256;
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
    crypto_chk_return(priv_key->length != klen, PKE_COMPAT_ERRNO(ERROR_INVALID_PARAM),
        "priv_key->length is invalid\n");
    crypto_chk_return(sig->length != klen, PKE_COMPAT_ERRNO(ERROR_INVALID_PARAM),
        "sig->length is invalid\n");
#if defined(CONFIG_PKE_ECC_SECURITY_STRENGTH_CHECK)
    /* increase security strength check. */
    drv_crypto_pke_check_param(hash->length < klen);
#endif

    ret = hal_pke_alg_ecc_init(DRV_PKE_ECC_TYPE_SM2);
    crypto_chk_return(ret != CRYPTO_SUCCESS, ret, "hal_pke_alg_ecc_init failed\n");

    hash_data.data = hash->data;
    hash_data.length = crypto_min(SM2_KEY_SIZE, hash->length);

    for (i = 0; i < MAX_TRY_TIMES; i++) {
        ret = inner_sm2_sign(priv_key, &hash_data, sig);
        if (ret == PKE_COMPAT_ERRNO(ERROR_ECDSA_SIGN_R_IS_ZERO) ||
            ret == PKE_COMPAT_ERRNO(ERROR_ECDSA_SIGN_S_IS_ZERO)) {
            continue;
        }
        if (ret != CRYPTO_SUCCESS) {
            crypto_log_err("inner_sm2_sign failed\n");
            goto exit_deinit;
        } else {
            ret = CRYPTO_SUCCESS;
            goto exit_deinit;
        }
    }

    ret = PKE_COMPAT_ERRNO(ERROR_TRY_TIMES);
exit_deinit:
    hal_pke_alg_ecc_deinit();
    return ret;
}
#endif