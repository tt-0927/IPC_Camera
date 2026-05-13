/**
  Copyright (c), 2001-2025, Shenshu Tech. Co., Ltd.
 */

#include "hal_pke_alg.h"

#include "crypto_drv_common.h"
#include "hal_pke_v5.h"
#include "pke_alg_inner.h"

int hal_pke_alg_ecc_inv_mod(const drv_pke_data *a, const drv_pke_data *c)
{
    int ret;
    const unsigned char *n_2 = NULL;
    unsigned int aligned_len = 0;
    unsigned int work_len = 0;
    unsigned int i = 0;
    int j = 0;
    const rom_lib *rom_lib_list[4] = {
        &instr_ecfn_inv_exp_00, &instr_ecfn_inv_exp_01, &instr_ecfn_inv_exp_10, &instr_ecfn_inv_exp_11
    };
    unsigned char bit_2 = 0;
    bool start_flag = false;
    unsigned int klen;
    const drv_pke_ecc_curve *ecc_curve = NULL;
    const pke_ecc_init_param *init_param = NULL;
    hal_pke_alg_ecc_init_check();
    ecc_curve = hal_pke_alg_ecc_get_curve();
    init_param = hal_pke_alg_ecc_get_init_param();

    klen = ecc_curve->ksize;
    n_2 = init_param->n_minus_2;

    ret = hal_pke_set_mont_para(sec_arg_add_cs(init_param->mont_param_n[1], init_param->mont_param_n[0]));
    crypto_chk_return(ret != CRYPTO_SUCCESS, ret, "sec_arg_add_cs failed\n");

    aligned_len = crypto_max(a->length, klen);
    ret = hal_pke_get_align_val(aligned_len, &aligned_len);
    crypto_chk_return(ret != CRYPTO_SUCCESS, ret, "hal_pke_get_align_val failed\n");
    work_len  = aligned_len / ALIGNED_TO_WORK_LEN_IN_BYTE;

    /* Step 1. former 5 process. data montgomery and copy */
    /* 1.set data into DRAM, ecc_addr_rrn and ecc_addr_mont_1_n will be set in initial parameters. */
    hal_pke_set_ram(ecc_addr_k, a->data, a->length, aligned_len);
    hal_pke_set_ram(ecc_addr_m, ecc_curve->n, klen, aligned_len);

    /* 2. start calculate. */
    ret = inner_batch_instr_process(&instr_ecfn_inv_pre_5, work_len);
    crypto_chk_return(ret != CRYPTO_SUCCESS, ret, "inner_batch_instr_process failed\n");

    /* Step 2. start inv calculate. */
    /* calculate by binary data, from high address to low address. the result store in ecc_addr_s */
    /*
     * 1) n_2 is one byte array, n->length is the byte number.
     * 2) For each byte in n_2, get bit2 = byte[7:6], byte[5:4], byte[3:2], byte[1:0] in order.
     * 3) If there is one bit2 is not zero, then all the next bit2(contain this time) will call
        inner_batch_instr_process(), the first param depends on bit2's value.
     */
    for (i = 0; i < klen; i++) {
        for (j = 3; j >= 0; j--) {              // 3: get byte[7:6], byte[5:4], byte[3:2], byte[1:0] in order.
            bit_2 = (n_2[i] >> (j * 2)) & 0x3;  // 2, 0x3: get bit2
            if ((start_flag == false) && (bit_2 != 0)) {
                start_flag = true;
                ret = inner_batch_instr_process(rom_lib_list[bit_2], work_len);
                crypto_chk_return(ret != CRYPTO_SUCCESS, ret, "inner_batch_instr_process failed\n");
            } else if (start_flag == true) {
                ret = inner_batch_instr_process(rom_lib_list[bit_2], work_len);
                crypto_chk_return(ret != CRYPTO_SUCCESS, ret, "inner_batch_instr_process failed\n");
            }
        }
    }
    /* get data out from DRAM ecc_addr_s, the result is montgomeried. */
    hal_pke_get_ram(ecc_addr_s, c->data, c->length);
    return ret;
}