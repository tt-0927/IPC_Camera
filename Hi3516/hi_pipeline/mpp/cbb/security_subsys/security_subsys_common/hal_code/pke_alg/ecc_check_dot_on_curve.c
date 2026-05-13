/**
  Copyright (c), 2001-2025, Shenshu Tech. Co., Ltd.
 */
#if defined(CONFIG_PKE_ECC_CHECK_DOT_ON_CURVE_SUPPORT) || defined(CONFIG_PKE_ECC_SM2_PRIV_DEC_SUPPORT)

#include "hal_pke_alg.h"
#include "hal_pke_v5.h"
#include "pke_alg_inner.h"

#include "drv_pke_inner.h"

#include "crypto_drv_common.h"

static td_s32 inner_ecfp_demontgomery_data_jac_z(const drv_pke_data *z, td_u32 work_len)
{
    td_s32 ret = TD_FAILURE;
    crypto_drv_func_enter();

    /* Step 1: start calculate. */
    ret = inner_batch_instr_process(&instr_ecfp_demont_cz_3, work_len);
    crypto_chk_func_return(inner_batch_instr_process, ret);

    /* Step 2: get data from PKE DRAM */
    if (z != TD_NULL && z->data != TD_NULL) {
        hal_pke_get_ram(sec_arg_add_cs(ecc_addr_cz, z->data, z->length));
    }

    crypto_drv_func_exit();
    return TD_SUCCESS;
}

static td_s32 inner_ecc_ecfp_point_valid_standard(const drv_pke_ecc_curve *curve, const drv_pke_ecc_point *pub_key,
    td_bool *is_on_curve)
{
    td_s32 ret = TD_FAILURE;
    td_u8 z[DRV_PKE_LEN_576] = {0};
    drv_pke_data check_z = { .data = z, .length = curve->ksize };
    drv_pke_data mod_p = { .data = (td_u8 *)curve->p, .length = curve->ksize };
    drv_pke_data kk = { .data = (td_u8 *)curve->n, .length = curve->ksize };
    const pke_ecc_init_param *init_param = NULL;
    crypto_drv_func_enter();

    init_param = hal_pke_alg_ecc_get_init_param();

    ret = inner_update_modulus(curve->p, curve->ksize, init_param->mont_param_p[1], init_param->mont_param_p[0]);
    crypto_chk_return(ret != CRYPTO_SUCCESS, ret, "inner_update_modulus failed\n");

    /* Step 1: montgomerize the pub_key point */
    ret = inner_ecfp_montgomery_data_aff(pub_key, &mod_p, TD_NULL);
    crypto_chk_return(ret != CRYPTO_SUCCESS, ret, "inner_ecfp_montgomery_data_aff failed\n");

    /* Step 2: trans Affine coordinate system to Jacobin coordinate system */
    ret = inner_ecfp_aff_to_jac(TD_NULL, &mod_p, TD_NULL);
    crypto_chk_func_return(inner_ecfp_aff_to_jac, ret);

    /* Step 3: calculate [n] * P */
    ret = inner_ecfp_mul_naf_cal(curve->ksize / ALIGNED_TO_WORK_LEN_IN_BYTE, &kk);
    crypto_chk_func_return(inner_ecfp_mul_naf_cal, ret);

    /* Step 4: no need to trans result data from jacobin coordinate system to affine coordinate system. */

    /* Step 5: demontgomery data of point.z. */
    ret = inner_ecfp_demontgomery_data_jac_z(&check_z, curve->ksize / ALIGNED_TO_WORK_LEN_IN_BYTE);
    crypto_chk_func_return(inner_ecfp_demontgomery_data_jac_z, ret);

    *is_on_curve = TD_FALSE;
    if (inner_drv_is_zero(check_z.data, check_z.length) == TD_TRUE) {
        *is_on_curve = TD_TRUE;
    };

    crypto_drv_func_exit();
    return TD_SUCCESS;
}

int hal_pke_alg_ecc_check_dot_on_curve(const drv_pke_ecc_point *point, td_bool *is_on_curve)
{
    const drv_pke_ecc_curve *curve = TD_NULL;
    hal_pke_alg_ecc_init_check();

    curve = hal_pke_alg_ecc_get_curve();
    crypto_chk_return(curve == TD_NULL, CRYPTO_FAILURE, "hal_pke_alg_ecc_get_curve failed\n");

    return inner_ecc_ecfp_point_valid_standard(curve, point, is_on_curve);
}
#endif