/**
  Copyright (c), 2001-2025, Shenshu Tech. Co., Ltd.
 */
#include "pke_alg_inner.h"

#include "hal_pke_v5.h"
#include "hal_pke_alg.h"
#include "crypto_drv_common.h"
#include "pke_alg_inner.h"
#include "drv_pke_inner.h"

#define JAC_TO_AFF_INSTR_NUM    6
static td_s32 inner_jac_to_aff_cal(const rom_lib *batch_instr, const td_u32 batch_instr_num,
    const drv_pke_data *mod_p)
{
    td_s32 ret = TD_FAILURE;
    td_u32 i = 0;
    td_s32 j = 0;
    const td_u8 *mod_p_2 = NULL;
    td_u8 bit_2 = 0;
    td_bool start_flag = TD_FALSE;
    td_u32 work_len = mod_p->length / ALIGNED_TO_WORK_LEN_IN_BYTE;
    crypto_drv_func_enter();

    mod_p_2 = hal_pke_alg_ecc_get_p_minus_2();

    /* 1. set necessary data into DRAM, which should have been set by former process. */
    /* get work_len */
    hal_crypto_pke_check_param(batch_instr_num != JAC_TO_AFF_INSTR_NUM);

    /* 2. start transform */
    /* 2.1 preprocess, initialize the data, use batch process step. */
    ret = inner_batch_instr_process(&batch_instr[0], work_len);
    crypto_chk_func_return(inner_batch_instr_process, ret);

    /* 2.2 expand (p-2)'s bit length to 2n, and calculate from high bit */
    /*
     * 1) mod_p_2 is one byte array, mod_p->length is the byte number.
     * 2) For each byte in mod_p_2, get bit2 = byte[7:6], byte[5:4], byte[3:2], byte[1:0] in order.
     * 3) If there is one bit2 is not zero, then all the next bit2(contain this time) will call barch_process(),
            the first param depends on bit2's value.
     */
    for (i = 0; i < mod_p->length; i++) {
        for (j = 3; j >= 0; j--) {                  // 3: get byte[7:6], byte[5:4], byte[3:2], byte[1:0] in order.
            bit_2 = (mod_p_2[i] >> (j * 2)) & 0x3;   // 2, 0x3: get bit2
            if ((start_flag == TD_FALSE) && (bit_2 != 0)) {
                start_flag = TD_TRUE;
                ret = inner_batch_instr_process(&batch_instr[bit_2 + 1], work_len);
                crypto_chk_return(ret != TD_SUCCESS, ret, "inner_batch_instr_process failed\n");
                crypto_timer_end(TIMER_ID_5, "Step 2.2 process");
            } else if (start_flag == TD_TRUE) {
                ret = inner_batch_instr_process(&batch_instr[bit_2 + 1], work_len);
                crypto_chk_return(ret != TD_SUCCESS, ret, "inner_batch_instr_process failed\n");
                crypto_timer_end(TIMER_ID_5, "Step 2.2 process");
            }
        }
    }

    /* 2.3 postprocess, demontgomery and complete reduction */
    ret = inner_batch_instr_process(&batch_instr[5], work_len); /* 5: instr index. */
    crypto_chk_func_return(inner_batch_instr_process, ret);

    crypto_timer_end(TIMER_ID_5, "Step 2.3");
    crypto_drv_func_exit();
    return TD_SUCCESS;
}

/* use when the process only have ecc related data address. */
td_s32 inner_ecfp_montgomery_data_aff(const drv_pke_ecc_point *in, const drv_pke_data *mod_p,
    const drv_pke_ecc_point *out)
{
    td_s32 ret = TD_FAILURE;
    td_u32 in_aligned_len = 0;
    td_u32 out_aligned_len = 0;
    td_u32 work_len = 0;
    crypto_drv_func_enter();

    hal_crypto_pke_check_param(in == TD_NULL || in->x == TD_NULL || in->y == TD_NULL);
    hal_crypto_pke_check_param(mod_p == TD_NULL || mod_p->data == TD_NULL);
    /* the out should be enough to save result. */
    hal_crypto_pke_check_param(out != TD_NULL && out->x != TD_NULL &&
                               out->y != TD_NULL && mod_p->length != out->length);

    /* P = P * rrp mod p to Montgomery data, point mul_mud */
    /* 1. wait for PKE free */
    ret = hal_pke_check_free();
    crypto_chk_func_return(hal_pke_check_free, ret);

    /* 2. set data to DRAM */
    hal_pke_get_align_val(in->length, &in_aligned_len);
    if (out != TD_NULL) {
        hal_pke_get_align_val(out->length, &out_aligned_len);
        in_aligned_len = (out_aligned_len >= in_aligned_len) ? out_aligned_len : in_aligned_len;
    }
    work_len = in_aligned_len / ALIGNED_TO_WORK_LEN_IN_BYTE;
    hal_pke_set_ram(sec_arg_add_cs(ecc_addr_px, in->x, in->length, in_aligned_len));
    hal_pke_set_ram(sec_arg_add_cs(ecc_addr_py, in->y, in->length, in_aligned_len));
    hal_pke_set_ram(sec_arg_add_cs(ecc_addr_m, mod_p->data, mod_p->length, in_aligned_len));

    /* 3. start calculate */
    ret = inner_batch_instr_process(&instr_ecfp_mont_p_2, work_len);
    crypto_chk_func_return(inner_batch_instr_process, ret);

    /* 6. get result from DRAM */
    if (out != TD_NULL && out->x != TD_NULL && out->y != TD_NULL) {
        hal_pke_get_ram(sec_arg_add_cs(ecc_addr_px, out->x, out->length));
        hal_pke_get_ram(sec_arg_add_cs(ecc_addr_py, out->y, out->length));
    }

    crypto_drv_func_exit();
    return TD_SUCCESS;
}

td_s32 inner_ecfp_demontgomery_data_aff(const drv_pke_ecc_point *r, td_u32 work_len)
{
    td_s32 ret = TD_FAILURE;
    crypto_drv_func_enter();

    /* Step 1: start calculate. */
    ret = inner_batch_instr_process(&instr_ecfp_demont_c_6, work_len);
    crypto_chk_func_return(inner_batch_instr_process, ret);

    /* Step 2: get data from PKE DRAM */
    if (r != TD_NULL && r->x != TD_NULL && r->y != TD_NULL) {
        hal_pke_get_ram(sec_arg_add_cs(ecc_addr_cx, r->x, r->length));
        hal_pke_get_ram(sec_arg_add_cs(ecc_addr_cy, r->y, r->length));
    }

    crypto_drv_func_exit();
    return TD_SUCCESS;
}

td_s32 inner_update_modulus(const td_u8 *n, const td_u32 n_len, td_u32 low_bit, td_u32 high_bit)
{
    td_s32 ret = TD_FAILURE;
    td_u32 work_len = n_len / ALIGNED_TO_WORK_LEN_IN_BYTE;

    /* 1. set new modulus and const_0(by initial parameters set) into PKE DRAM. */
    hal_pke_set_ram(sec_arg_add_cs(ecc_addr_n, n, n_len, n_len));

    /* 2. start calculate. */
    ret = inner_batch_instr_process(&instr_ecfp_prime_n_1, work_len);
    crypto_chk_func_return(inner_batch_instr_process, ret);

    /* 5. get new montgomery parameters and set it into register. */
    ret = hal_pke_set_mont_para(low_bit, high_bit);
    crypto_chk_return(ret != CRYPTO_SUCCESS, ret, "hal_pke_set_mont_para failed\n");

    return TD_SUCCESS;
}

#define JAC_TO_AFF_INSTR_NUM    6
td_s32 inner_ecfp_jac_to_aff(const drv_pke_data *mod_p)
{
    td_s32 ret = TD_FAILURE;

    /* Step 1: prepare instructions to use */
    rom_lib batch_instr_block[JAC_TO_AFF_INSTR_NUM] = {instr_ecfp_j2a_pre_5, instr_ecfp_j2a_exp_00,
        instr_ecfp_j2a_exp_01, instr_ecfp_j2a_exp_10, instr_ecfp_j2a_exp_11,
        instr_ecfp_j2a_post_4};

    /* Step 2: start transfer calculation. */
    ret = inner_jac_to_aff_cal(batch_instr_block, JAC_TO_AFF_INSTR_NUM, mod_p);
    crypto_chk_func_return(inner_jac_to_aff_cal, ret);

    return ret;
}

/* (ecc_addr_px, ecc_addr_py) -> (ecc_addr_cx, ecc_addr_cy, ecc_addr_cz) */
td_s32 inner_ecfp_aff_to_jac(const drv_pke_ecc_point *in, const drv_pke_data *mod_p, pke_ecc_point_jac *out)
{
    td_s32 ret = TD_FAILURE;
    td_u32 work_len = 0;
    crypto_drv_func_enter();

    /* Step 1: wait for PKE free */
    ret = hal_pke_check_free();
    crypto_chk_func_return(hal_pke_check_free, ret);

    /* Step 2: set input data into DRAM. */
    if (in != TD_NULL && in->x != TD_NULL && in->y != TD_NULL) {
        hal_pke_set_ram(sec_arg_add_cs(ecc_addr_px, in->x, in->length, mod_p->length));
        hal_pke_set_ram(sec_arg_add_cs(ecc_addr_py, in->y, in->length, mod_p->length));
    }
    /* need to reset modulur. */
    hal_pke_set_ram(sec_arg_add_cs(ecc_addr_m, mod_p->data, mod_p->length, mod_p->length));

    /* Step 3: start calculate. */
    work_len = mod_p->length / ALIGNED_TO_WORK_LEN_IN_BYTE;
    ret = inner_batch_instr_process(&instr_ecfp_cpy_p2c_3, work_len);
    crypto_chk_return(ret != CRYPTO_SUCCESS, ret, "inner_batch_instr_process failed\n");

    /* Step 4: get result from DRAM. */
    /* no need to get result out, only a common process. */
    if (out != TD_NULL && out->x != TD_NULL && out->y != TD_NULL && out->z != TD_NULL) {
        hal_pke_get_ram(sec_arg_add_cs(ecc_addr_cx, out->x, out->length));
        hal_pke_get_ram(sec_arg_add_cs(ecc_addr_cy, out->y, out->length));
        hal_pke_get_ram(sec_arg_add_cs(ecc_addr_cz, out->z, out->length));
    }

    crypto_drv_func_exit();
    return TD_SUCCESS;
}