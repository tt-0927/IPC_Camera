/**
  Copyright (c), 2001-2025, Shenshu Tech. Co., Ltd.
 */

#ifndef PKE_ALG_INNER_H
#define PKE_ALG_INNER_H

#include "crypto_pke_struct.h"
#include "rom_lib.h"


/*
 *
 * Used in:
 *      ecc_common.c
 *      rsa_exp_mod.c
 */
td_s32 inner_pke_alg_resume(void);
void inner_pke_alg_suspend(void);

/*
 *
 * Used in:
 *      ecc_compute_x.c
 *      ecc_point_mul.c
 */
td_s32 inner_ecfp_montgomery_data_aff(const drv_pke_ecc_point *in, const drv_pke_data *mod_p,
    const drv_pke_ecc_point *out);

/*
 *
 * Used in:
 *      ecc_compute_x.c
 *      ecc_point_mul.c
 */
td_s32 inner_ecfp_demontgomery_data_aff(const drv_pke_ecc_point *r, td_u32 work_len);

/*
 *
 * Used in:
 *      ecc_compute_x.c
 *      ecc_point_mul.c
 */
td_s32 inner_ecfp_jac_to_aff(const drv_pke_data *mod_p);

/* * struct of ecc point */
typedef struct {
    td_u8 *x;   /* X coordinates of the point in Jacobian coordinate system. */
    td_u8 *y;   /* Y coordinates of the point in Jacobian coordinate system. */
    td_u8 *z;   /* Z coordinates of the point in Jacobian coordinate system. */
    td_u32 length;
} pke_ecc_point_jac;
td_s32 inner_ecfp_aff_to_jac(const drv_pke_ecc_point *in, const drv_pke_data *mod_p, pke_ecc_point_jac *out);

/*
 *
 * Used in:
 *      ecc_check_dot_on_curve.c
 *      ecc_point_mul.c
 */
td_s32 inner_ecfp_mul_naf_cal(td_u32 work_len, const drv_pke_data *k);

/*
 *
 * Used in:
 *      ecc_compute_s.c
 *      ecc_compute_u1_and_u2.c
 *      ecc_compute_x.c
 *      ecc_inv_mod.c
 *      sm2_add_mod.c
 *      sm2_compute_s.c
 *      rsa_exp_mod.c
 */
td_s32 inner_batch_instr_process(const rom_lib *batch_instr, td_u32 work_len);

/*
 *
 * Used in:
 *      ecc_point_mul.c
 *      sm2_compute_s.c
 */
td_s32 inner_update_modulus(const td_u8 *n, const td_u32 n_len, td_u32 low_bit, td_u32 high_bit);

/*
 *
 * Used in:
 *      rsa_exp_mod.c
 */
td_s32 inner_update_rsa_modulus(const td_u8 *n, const td_u32 n_len, const td_u32 aligned_len);

#endif