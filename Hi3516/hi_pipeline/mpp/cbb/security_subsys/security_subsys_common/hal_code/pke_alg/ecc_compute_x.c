/**
  Copyright (c), 2001-2025, Shenshu Tech. Co., Ltd.
 */
#if defined(CONFIG_PKE_ECC_ECDSA_VERIFY_SUPPORT) || defined(CONFIG_PKE_ECC_SM2_VERIFY_SUPPORT)

#include "hal_pke_alg.h"

#include "crypto_drv_common.h"
#include "hal_pke_v5.h"
#include "pke_alg_inner.h"

/* Only used for hal_pke_alg_ecc_compute_x. */
static td_s32 point_multi_mul_sim(const drv_pke_ecc_curve *ecc, const drv_pke_data *u1,
    const drv_pke_data *u2)
{
    td_s32 ret = TD_FAILURE;
    td_s32 i = 0;
    td_s32 j = 0;
    td_u32 work_len = ecc->ksize / ALIGNED_TO_WORK_LEN_IN_BYTE;
    td_u8 u1_bit = 0;
    td_u8 u2_bit = 0;
    td_u8 bit_2 = 0;
    td_bool first_flag = TD_TRUE;
    const rom_lib *rom_lib_first_list[4] = {    // 4: select by bit_2
        TD_NULL, &instr_ecfp_cpy_p2c_3, &instr_ecfp_cpy_g2c_3, &instr_ecfp_cpy_a2c_3
    };
    const rom_lib *rom_lib_second_list[4] = {   // 4: select by bit_2
        &instr_ecfp_mul_c_double_22, &instr_ecfp_mul_p_22_18, &instr_ecfp_mul_g_22_18, &instr_ecfp_mul_jj_22_23
    };

    /*
     * 1) u1->data and u2->data are two byte arrays, their length u1->length and u2->length is the sample.
     * 2) For each bit in u1->data and u2->data, get bit_2 = u1_bit | u2_bit in order.
     * 3) If there is one bit_2 is not zero, then all the next bit_2(contain this time) will call
        inner_batch_instr_process(), the first param depends on bit_2's value.
     */
    for (i = 0; i < (td_s32)u1->length; i++) {
        if (i >= (td_s32)u1->length || i >= (td_s32)u2->length) {
            break;
        }
        for (j = CRYPTO_BITS_IN_BYTE - 1; j >= 0; j--) {
            u1_bit = (u1->data[i] >> j) & 1;
            u2_bit = (u2->data[i] >> j) & 1;
            bit_2 = ((u1_bit << 1) | u2_bit);       // bit_2 = u1_bit | u2_bit
            if (first_flag == TD_TRUE) {
                if (bit_2 == 0) {
                    continue;
                }
                ret = inner_batch_instr_process(rom_lib_first_list[bit_2], work_len);
                crypto_chk_return(ret != TD_SUCCESS, ret, "inner_batch_instr_process failed\n");
                first_flag = TD_FALSE;
            } else {
                ret = inner_batch_instr_process(rom_lib_second_list[bit_2], work_len);
                crypto_chk_return(ret != TD_SUCCESS, ret, "inner_batch_instr_process failed\n");
            }
        }
    }

    return TD_SUCCESS;
}

int hal_pke_alg_ecc_compute_x(const drv_pke_data *u1, const drv_pke_data *u2,
    const drv_pke_ecc_point *q, const drv_pke_data *x)
{
    int ret;
    unsigned int aligned_len;
    drv_pke_ecc_point g = {0};
    unsigned char g_x_jac[DRV_PKE_LEN_576] = {0};
    unsigned char g_y_jac[DRV_PKE_LEN_576] = {0};
    unsigned char q_x_jac[DRV_PKE_LEN_576] = {0};
    unsigned char q_y_jac[DRV_PKE_LEN_576] = {0};
    /* get const value 0, and montgomerized p */
    drv_pke_data pp = {0};
    drv_pke_ecc_point g_jac = {.x = g_x_jac, .y = g_y_jac};
    drv_pke_ecc_point q_jac = {.x = q_x_jac, .y = q_y_jac};
    const drv_pke_ecc_curve *ecc_curve = NULL;
    const pke_ecc_init_param *init_param = NULL;
    hal_pke_alg_ecc_init_check();
    ecc_curve = hal_pke_alg_ecc_get_curve();
    init_param = hal_pke_alg_ecc_get_init_param();

    g.x = (unsigned char *)ecc_curve->gx;
    g.y = (unsigned char *)ecc_curve->gy;
    g.length = ecc_curve->ksize;
    pp.data = (unsigned char *)ecc_curve->p;
    pp.length = ecc_curve->ksize;
    g_jac.length = ecc_curve->ksize;
    q_jac.length = ecc_curve->ksize;

    ret = hal_pke_set_mont_para(init_param->mont_param_p[1], init_param->mont_param_p[0]);
    crypto_chk_return(ret != CRYPTO_SUCCESS, ret, "hal_pke_set_mont_para failed\n");

    /* Step 1: montgomery the input point. */
    ret = inner_ecfp_montgomery_data_aff(&g, &pp, &g_jac);
    crypto_chk_return(ret != CRYPTO_SUCCESS, ret, "inner_ecfp_montgomery_data_aff failed\n");
    ret = inner_ecfp_montgomery_data_aff(q, &pp, &q_jac);
    crypto_chk_return(ret != CRYPTO_SUCCESS, ret, "inner_ecfp_montgomery_data_aff failed\n");

    /* Step 2: A = P + Q, from affine to jacobin. */
    /* 2.1 set data into PKE DRAM. */
    ret = hal_pke_get_align_val(ecc_curve->ksize, &aligned_len);
    crypto_chk_return(ret != CRYPTO_SUCCESS, ret, "hal_pke_get_align_val failed\n");

    hal_pke_set_ram(sec_arg_add_cs(ecc_addr_px, q_jac.x, q_jac.length, aligned_len));
    hal_pke_set_ram(sec_arg_add_cs(ecc_addr_py, q_jac.y, q_jac.length, aligned_len));
    hal_pke_set_ram(sec_arg_add_cs(ecc_addr_gx, g_jac.x, g_jac.length, aligned_len));
    hal_pke_set_ram(sec_arg_add_cs(ecc_addr_gy, g_jac.y, g_jac.length, aligned_len));

#if defined(CONFIG_PKE_ECC_TRACE_ENABLE)
    crypto_dump_data("q_jac.x", q_jac.x, q_jac.length);
    crypto_dump_data("q_jac.y", q_jac.y, q_jac.length);
    crypto_dump_data("g_jac.x", g_jac.x, g_jac.length);
    crypto_dump_data("g_jac.y", g_jac.y, g_jac.length);
#endif
    /* point add A = G + Q. */
    ret = inner_batch_instr_process(&instr_ecfp_add_ja_verify_18,
        aligned_len / ALIGNED_TO_WORK_LEN_IN_BYTE);
    crypto_chk_return(ret != CRYPTO_SUCCESS, ret, "inner_batch_instr_process failed\n");

    /* Step 3: simultaneous calculate multi-multiplication. */
    ret = point_multi_mul_sim(sec_arg_add_cs(ecc_curve, u1, u2));
    crypto_chk_return(ret != CRYPTO_SUCCESS, ret, "point_multi_mul_sim failed\n");

    /* Step 4: trans Jacobin to Affine. */
    ret = inner_ecfp_jac_to_aff(&pp);
    crypto_chk_return(ret != CRYPTO_SUCCESS, ret, "inner_ecfp_jac_to_aff failed\n");

    /* Step 5: demontgomery, just process, not get data out of the DRAM. */
    ret = inner_ecfp_demontgomery_data_aff(TD_NULL, aligned_len / ALIGNED_TO_WORK_LEN_IN_BYTE);
    crypto_chk_return(ret != CRYPTO_SUCCESS, ret, "inner_ecfp_demontgomery_data_aff failed\n");

    /* Step 6: get result out from PKE data RAM. only need to get r.x for compare. */
    hal_pke_get_ram(sec_arg_add_cs(ecc_addr_cx, x->data, x->length));

    return ret;
}
#endif