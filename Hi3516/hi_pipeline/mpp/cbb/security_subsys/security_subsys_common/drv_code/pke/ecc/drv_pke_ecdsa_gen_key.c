/**
  Copyright (c), 2001-2025, Shenshu Tech. Co., Ltd.
 */
#if defined(CONFIG_PKE_ECC_GEN_KEY_SUPPORT)

#include "drv_pke_inner.h"
#include "crypto_drv_common.h"

/* According to SEC1 "3.2.1 Elliptic Curve Key Pair Generation Primitiv"
*/
static td_s32 inner_ecc_gen_key(const drv_pke_data *input_priv_key, const drv_pke_data *output_priv_key,
    const drv_pke_ecc_point *output_pub_key)
{
    volatile td_s32 ret = TD_FAILURE;
    const unsigned char *ecc_n = hal_pke_alg_ecc_get_n();
    unsigned int klen = hal_pke_alg_ecc_get_klen();

    /* Step 1. Randomly or pseudorandomly select an integer d in the interval [1, n − 1].
     * Notice: If input_priv_key->data != NULL, then use it as d and skip step1.
     */
    if (input_priv_key != NULL && input_priv_key->data != NULL) {
        ret = memcpy_s(output_priv_key->data, output_priv_key->length, input_priv_key->data, input_priv_key->length);
        crypto_chk_return(ret != EOK, PKE_COMPAT_ERRNO(ERROR_MEMCPY_S), "memcpy_s failed\n");
    } else {
        ret = inner_get_limit_random(output_priv_key->data, ecc_n, klen);
        crypto_chk_return(ret != CRYPTO_SUCCESS, ret, "inner_get_limit_random failed\n");
    }

    /* Step 2. Compute Q = d*G. */
    /* Step 3. Output (d, Q). */
#if defined(CONFIG_PKE_ECC_GEN_KEY_TRACE_ENABLE)
    crypto_print("========== Start Compute Q = d*G. ==========\n");
    crypto_dump_data("d", output_priv_key->data, output_priv_key->length);
#endif
    ret = hal_pke_alg_ecc_base_point_mul(output_priv_key, output_pub_key);
    crypto_chk_return(ret != CRYPTO_SUCCESS, ret, "drv_ecc_middle_point_mul failed\n");
#if defined(CONFIG_PKE_ECC_GEN_KEY_TRACE_ENABLE)
    crypto_dump_data("Q(x)", output_pub_key->x, output_pub_key->length);
    crypto_dump_data("Q(y)", output_pub_key->y, output_pub_key->length);
    crypto_print("========== End Compute Q = d*G. ==========\n");
#endif
    return ret;
}

td_s32 drv_cipher_pke_ecc_gen_key(drv_pke_ecc_curve_type curve_type, const drv_pke_data *input_priv_key,
    const drv_pke_data *output_priv_key, const drv_pke_ecc_point *output_pub_key)
{
    td_s32 ret = TD_FAILURE;
    unsigned int klen;

    /* param null check */
    pke_null_ptr_chk(output_priv_key);
    pke_null_ptr_chk(output_priv_key->data);
    pke_null_ptr_chk(output_pub_key);
    pke_null_ptr_chk(output_pub_key->x);
    pke_null_ptr_chk(output_pub_key->y);

    /* length check. */
    klen = crypto_pke_get_klen(curve_type);
    crypto_chk_return(output_priv_key->length != klen, PKE_COMPAT_ERRNO(ERROR_INVALID_PARAM),
        "output_priv_key->length is invalid\n");
    crypto_chk_return(output_pub_key->length != klen, PKE_COMPAT_ERRNO(ERROR_INVALID_PARAM),
        "output_pub_key->length is invalid\n");
    if (input_priv_key != NULL && input_priv_key->length !=0) {
        crypto_chk_return(input_priv_key->data == NULL, PKE_COMPAT_ERRNO(ERROR_PARAM_IS_NULL),
            "input_priv_key->data is null\n");
        crypto_chk_return(input_priv_key->length != klen, PKE_COMPAT_ERRNO(ERROR_INVALID_PARAM),
            "input_priv_key->length is invalid\n");
    }

    ret = hal_pke_alg_ecc_init(curve_type);
    crypto_chk_return(ret != CRYPTO_SUCCESS, ret, "hal_pke_alg_ecc_init failed\n");

    ret = inner_ecc_gen_key(input_priv_key, output_priv_key, output_pub_key);
    crypto_chk_goto(ret != CRYPTO_SUCCESS, exit_deinit, "inner_ecc_gen_ecdh_key failed\n");

exit_deinit:
    hal_pke_alg_ecc_deinit();
    return ret;
}
#endif