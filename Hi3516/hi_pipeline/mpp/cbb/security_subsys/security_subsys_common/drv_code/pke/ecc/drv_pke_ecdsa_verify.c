/**
  Copyright (c), 2001-2025, Shenshu Tech. Co., Ltd.
 */

#include "drv_pke_inner.h"

#include "crypto_drv_common.h"


#define MAX_ECC_SIZE        72

static int inner_ecdsa_verify(const drv_pke_ecc_point *pub_key, const drv_pke_data *hash, const drv_pke_ecc_sig *sig)
{
    int ret;
    unsigned char w_buf[MAX_ECC_SIZE];
    unsigned char u1_buf[MAX_ECC_SIZE];
    unsigned char u2_buf[MAX_ECC_SIZE];
    unsigned char x_buf[MAX_ECC_SIZE];
    drv_pke_data sig_s_data = {
        .data = sig->s,
        .length = sig->length
    };
    drv_pke_data w_data = {
        .data = w_buf,
        .length = pub_key->length
    };
    drv_pke_data r_data = {
        .data = sig->r,
        .length = sig->length
    };
    drv_pke_data u1_data = {
        .data = u1_buf,
        .length = pub_key->length
    };
    drv_pke_data u2_data = {
        .data = u2_buf,
        .length = pub_key->length
    };
    drv_pke_data x_data = {
        .data = x_buf,
        .length = pub_key->length
    };
    const unsigned char *ecc_n = hal_pke_alg_ecc_get_n();

    /* Step 1. Check if r is in [1, n - 1], s is in [1, n - 1]. */
    crypto_chk_return(inner_drv_is_in_range(sig->r, ecc_n, pub_key->length) == TD_FALSE,
        CRYPTO_FAILURE, "sig->r is not in [1, n - 1]!\n");
    crypto_chk_return(inner_drv_is_in_range(sig->s, ecc_n, pub_key->length) == TD_FALSE,
        CRYPTO_FAILURE, "sig->s is not in [1, n - 1]!\n");
    /* Step 2. compute e = H(m), we can skip the step. */
    /* Step 3. compute w = s^-1 mod n. */

#if defined(CONFIG_PKE_ECC_TRACE_ENABLE)
    crypto_print("========== Start Compute w = s^-1 mod n ==========\n");
    crypto_dump_data("s", sig_s_data.data, sig_s_data.length);
#endif
    ret = hal_pke_alg_ecc_inv_mod(&sig_s_data, &w_data);
    crypto_chk_return(ret != CRYPTO_SUCCESS, ret, "hal_pke_alg_ecc_inv_mod failed\n");

#if defined(CONFIG_PKE_ECC_TRACE_ENABLE)
    crypto_dump_data("w=s^-1 mod n", w_data.data, w_data.length);
    crypto_print("========== End Compute w = s^-1 mod n ==========\n");
#endif
    /* Step 4. compute u1 = ew mod n, u2 = rw mod n. */
#if defined(CONFIG_PKE_ECC_TRACE_ENABLE)
    crypto_print("========== Start Compute u1 = ew mod n, u2 = rw mod n ==========\n");
    crypto_dump_data("e", hash->data, hash->length);
    crypto_dump_data("r", r_data.data, r_data.length);
    crypto_dump_data("w", w_data.data, w_data.length);
#endif
    ret = hal_pke_alg_ecc_compute_u1_and_u2(&w_data, hash, &r_data, &u1_data, &u2_data);
    crypto_chk_return(ret != CRYPTO_SUCCESS, ret, "hal_pke_alg_ecc_compute_u1_and_u2 failed\n");
#if defined(CONFIG_PKE_ECC_TRACE_ENABLE)
    crypto_dump_data("u1 = ew mod n", u1_data.data, u1_data.length);
    crypto_dump_data("u2 = ew mod n", u2_data.data, u2_data.length);
    crypto_print("========== End Compute u1 = ew mod n, u2 = rw mod n ==========\n");
#endif
    /**
     * Step 5. compute x = u1 * p(x, y) + u2 * q(x, y).
     * Step 6. check if x is infinite point.
    **/
#if defined(CONFIG_PKE_ECC_TRACE_ENABLE)
    crypto_print("========== Start Compute x = u1 * p(x, y) + u2 * q(x, y) ==========\n");
    crypto_dump_data("u1", u1_data.data, u1_data.length);
    crypto_dump_data("u2", u2_data.data, u2_data.length);
    crypto_dump_data("Q(x)", pub_key->x, pub_key->length);
    crypto_dump_data("Q(y)", pub_key->y, pub_key->length);
#endif
    ret = hal_pke_alg_ecc_compute_x(&u1_data, &u2_data, pub_key, &x_data);
    crypto_chk_return(ret != CRYPTO_SUCCESS, ret, "hal_pke_alg_ecc_compute_x failed\n");

#if defined(CONFIG_PKE_ECC_TRACE_ENABLE)
    crypto_dump_data("x(x)", x_data.data, x_data.length);
    crypto_print("========== End Compute x = u1 * p(x, y) + u2 * q(x, y) ==========\n");
#endif
    /**
     * Step 7. compute v = x(x).
     * Step 8. check if v == r.
    **/
    if (memcmp(x_data.data, sig->r, x_data.length) == 0) {
        return CRYPTO_SUCCESS;
    }

    crypto_log_err("v != r\n");
    return CRYPTO_FAILURE;
}

td_s32 drv_cipher_pke_ecdsa_verify(drv_pke_ecc_curve_type curve_type, const drv_pke_ecc_point *pub_key,
    const drv_pke_data *hash, const drv_pke_ecc_sig *sig)
{
    int ret;
    unsigned int klen;
    drv_pke_data hash_data;

    /* param check. */
    pke_null_ptr_chk(pub_key);
    pke_null_ptr_chk(pub_key->x);
    pke_null_ptr_chk(pub_key->y);
    pke_null_ptr_chk(hash);
    pke_null_ptr_chk(hash->data);
    pke_null_ptr_chk(sig);
    pke_null_ptr_chk(sig->r);
    pke_null_ptr_chk(sig->s);

    /* length check. */
    klen = crypto_pke_get_klen(curve_type);
    crypto_chk_return(pub_key->length != klen, PKE_COMPAT_ERRNO(ERROR_INVALID_PARAM),
        "pub_key->length is invalid\n");
    crypto_chk_return(sig->length != klen, PKE_COMPAT_ERRNO(ERROR_INVALID_PARAM),
        "sig->length is invalid\n");

    /* increase security strength check. for nist-fips-224, it's the same security strength with SHA224 */
#if defined(CONFIG_PKE_ECC_SECURITY_STRENGTH_CHECK)
    drv_crypto_pke_check_param((hash->length < klen) &&
        !(hash->length == HASH_SIZE_SHA_224 && klen == DRV_PKE_LEN_224) &&
        !(hash->length == HASH_SIZE_SHA_512 && (klen == DRV_PKE_LEN_521 || klen == DRV_PKE_LEN_576)));
#endif
    drv_crypto_pke_check_param(hash->length != HASH_SIZE_SHA_224 && hash->length != HASH_SIZE_SHA_256 &&
        hash->length != HASH_SIZE_SHA_384 && hash->length != HASH_SIZE_SHA_512);
    hash_data.data = hash->data;
    hash_data.length = crypto_min(klen, hash->length);

    ret = hal_pke_alg_ecc_init(curve_type);
    crypto_chk_return(ret != CRYPTO_SUCCESS, ret, "hal_pke_alg_ecc_init failed\n");

    ret = inner_ecdsa_verify(pub_key, &hash_data, sig);
    crypto_chk_goto(ret != CRYPTO_SUCCESS, exit_deinit, "inner_ecdsa_verify failed\n");

exit_deinit:
    hal_pke_alg_ecc_deinit();
    return ret;
}