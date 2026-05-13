/**
  Copyright (c), 2001-2025, Shenshu Tech. Co., Ltd.
 */
#if defined(CONFIG_PKE_ECC_ECDH_SUPPORT)

#include "drv_pke_inner.h"
#include "crypto_drv_common.h"

#define MAX_ECC_LENGTH      72
/* According to SEC1 "3.3.2 Elliptic Curve Cofactor Diffie-Hellman Primitive" */
static td_s32 inner_ecc_gen_ecdh_key(const drv_pke_ecc_point *input_pub_key, const drv_pke_data *input_priv_key,
    const drv_pke_data *output_shared_key)
{
    volatile td_s32 ret = TD_FAILURE;
    drv_pke_ecc_point rr = {0};
    unsigned char rr_y[MAX_ECC_LENGTH];

    rr.x = output_shared_key->data;
    rr.y = rr_y;
    rr.length = output_shared_key->length;

#if defined(CONFIG_PKE_ECDH_TRACE_ENABLE)
    crypto_dump_data("d", input_priv_key->data, input_priv_key->length);
    crypto_dump_data("Q(x)", input_pub_key->x, input_pub_key->length);
    crypto_dump_data("Q(y)", input_pub_key->y, input_pub_key->length);
#endif
    /* Step 1. Compute point P(x, y) = h*Du*Qv.
     * 1) h is the ecc parameter, for brainpool/fips/sm2, the h is 1, so we can compute P(x, y) = Du*Qv directly.
     * 2) Du is the input priv_key.
     * 3) Qv is the output pub_key.
    */
    ret = hal_pke_alg_ecc_point_mul(input_priv_key, input_pub_key, &rr);
    crypto_chk_return(ret != TD_SUCCESS, ret, "hal_pke_alg_ecc_point_mul failed\n");
#if defined(CONFIG_PKE_ECDH_TRACE_ENABLE)
    crypto_dump_data("P(x)", rr.x, input_priv_key->length);
#endif
    /* Step 2. Check that if P(x, y) is ZERO. */
    /* Step 3. Output z = Xp. */

    return TD_SUCCESS;
}

td_s32 drv_cipher_pke_ecc_gen_ecdh_key(drv_pke_ecc_curve_type curve_type, const drv_pke_ecc_point *input_pub_key,
    const drv_pke_data *input_priv_key, const drv_pke_data *output_shared_key)
{
    td_s32 ret = TD_FAILURE;
    unsigned int klen;

    /* param null check. */
    pke_null_ptr_chk(input_pub_key);
    pke_null_ptr_chk(input_pub_key->x);
    pke_null_ptr_chk(input_pub_key->y);
    pke_null_ptr_chk(input_priv_key);
    pke_null_ptr_chk(input_priv_key->data);
    pke_null_ptr_chk(output_shared_key);
    pke_null_ptr_chk(output_shared_key->data);

    /* length check. */
    klen = crypto_pke_get_klen(curve_type);
    crypto_chk_return(input_priv_key->length != klen, PKE_COMPAT_ERRNO(ERROR_INVALID_PARAM),
        "input_priv_key->length is invalid\n");
    crypto_chk_return(input_pub_key->length != klen, PKE_COMPAT_ERRNO(ERROR_INVALID_PARAM),
        "input_pub_key->length is invalid\n");
    crypto_chk_return(output_shared_key->length != klen, PKE_COMPAT_ERRNO(ERROR_INVALID_PARAM),
        "output_shared_key->length is invalid\n");

    ret = hal_pke_alg_ecc_init(curve_type);
    crypto_chk_return(ret != CRYPTO_SUCCESS, ret, "hal_pke_alg_ecc_init failed\n");

    ret = inner_ecc_gen_ecdh_key(input_pub_key, input_priv_key, output_shared_key);
    crypto_chk_goto(ret != CRYPTO_SUCCESS, exit_deinit, "inner_ecc_gen_ecdh_key failed\n");

exit_deinit:
    hal_pke_alg_ecc_deinit();
    return ret;
}
#endif