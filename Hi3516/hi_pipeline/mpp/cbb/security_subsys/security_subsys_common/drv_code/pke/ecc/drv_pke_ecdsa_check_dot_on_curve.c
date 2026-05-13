/**
  Copyright (c), 2001-2025, Shenshu Tech. Co., Ltd.
 */
#if defined(CONFIG_PKE_ECC_CHECK_DOT_ON_CURVE_SUPPORT)

#include "drv_pke_inner.h"
#include "crypto_drv_common.h"

static td_s32 inner_ecc_check_dot_on_curve(const drv_pke_ecc_point *pub_key, td_bool *is_on_curve)
{
    volatile td_s32 ret = TD_FAILURE;
    ret = hal_pke_alg_ecc_check_dot_on_curve(pub_key, is_on_curve);
    return ret;
}

td_s32 drv_cipher_pke_check_dot_on_curve(drv_pke_ecc_curve_type curve_type, const drv_pke_ecc_point *pub_key,
    td_bool *is_on_curve)
{
    td_u32 klen;
    td_s32 ret = TD_FAILURE;

    /* param check. */
    pke_null_ptr_chk(pub_key);
    pke_null_ptr_chk(pub_key->x);
    pke_null_ptr_chk(pub_key->y);
    pke_null_ptr_chk(is_on_curve);

    /* length check. */
    klen = crypto_pke_get_klen(curve_type);
    crypto_chk_return(pub_key->length != klen, PKE_COMPAT_ERRNO(ERROR_INVALID_PARAM),
        "pub_key->length is invalid\n");

    ret = hal_pke_alg_ecc_init(curve_type);
    crypto_chk_return(ret != CRYPTO_SUCCESS, ret, "hal_pke_alg_ecc_init failed\n");

    ret = inner_ecc_check_dot_on_curve(pub_key, is_on_curve);
    crypto_chk_goto(ret != CRYPTO_SUCCESS, exit_deinit, "inner_ecc_check_dot_on_curve failed\n");

exit_deinit:
    hal_pke_alg_ecc_deinit();
    return ret;
}
#endif