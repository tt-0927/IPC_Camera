/**
  Copyright (c), 2001-2025, Shenshu Tech. Co., Ltd.
 */
#include "hal_pke_alg.h"

#include "crypto_drv_common.h"
#include "hal_pke_v5.h"
#include "pke_alg_inner.h"
#include "drv_pke_inner.h"

int hal_pke_alg_ecc_base_point_mul(const drv_pke_data *k, const drv_pke_ecc_point *r)
{
    drv_pke_ecc_point base;
    const drv_pke_ecc_curve *ecc_curve = NULL;
    hal_pke_alg_ecc_init_check();

    ecc_curve = hal_pke_alg_ecc_get_curve();

    base.x = (unsigned char *)ecc_curve->gx;
    base.y = (unsigned char *)ecc_curve->gy;
    base.length = ecc_curve->ksize;

    return hal_pke_alg_ecc_point_mul(k, &base, r);
}

int hal_pke_alg_ecc_point_mul(const drv_pke_data *k, const drv_pke_ecc_point *p, const drv_pke_ecc_point *r)
{
    int ret = CRYPTO_FAILURE;
    unsigned int klen;
    drv_pke_data mod_p = {0};
    const drv_pke_ecc_curve *ecc_curve = NULL;
    const pke_ecc_init_param *init_param = NULL;
    hal_pke_alg_ecc_init_check();

    ecc_curve = hal_pke_alg_ecc_get_curve();
    init_param = hal_pke_alg_ecc_get_init_param();
    klen = ecc_curve->ksize;
    mod_p.data = (unsigned char *)ecc_curve->p;
    mod_p.length = klen;

    /* Step 0: set montgomery param into register. */
    ret = inner_update_modulus(ecc_curve->p, klen, init_param->mont_param_p[1], init_param->mont_param_p[0]);
    crypto_chk_return(ret != CRYPTO_SUCCESS, ret, "inner_update_modulus failed\n");

    /* Step 1: montgomery point p */
    ret = inner_ecfp_montgomery_data_aff(p, &mod_p, r);
    crypto_chk_return(ret != CRYPTO_SUCCESS, ret, "inner_ecfp_montgomery_data_aff failed\n");

    /* Step 2: trans Affine coordinate system to Jacobin coordinate system */
    ret = inner_ecfp_aff_to_jac(r, &mod_p, TD_NULL);
    crypto_chk_return(ret != CRYPTO_SUCCESS, ret, "inner_ecfp_aff_to_jac failed\n");

    /* Step 3: start NAF-multiplication. */
    ret = inner_ecfp_mul_naf_cal(klen / ALIGNED_TO_WORK_LEN_IN_BYTE, k);
    crypto_chk_return(ret != CRYPTO_SUCCESS, ret, "inner_ecfp_mul_naf_cal failed\n");

    /* Step 4: trans result data from jacobin coordinate system to affine coordinate system. */
    ret = inner_ecfp_jac_to_aff(&mod_p);
    crypto_chk_return(ret != CRYPTO_SUCCESS, ret, "inner_ecfp_jac_to_aff failed\n");

    /* Step 5. demontgomery data. */
    ret = inner_ecfp_demontgomery_data_aff(r, klen / ALIGNED_TO_WORK_LEN_IN_BYTE);
    crypto_chk_return(ret != CRYPTO_SUCCESS, ret, "inner_ecfp_demontgomery_data_aff failed\n");

    return ret;
}