/**
  Copyright (c), 2001-2025, Shenshu Tech. Co., Ltd.
 */

#include "hal_pke_alg.h"

#include "crypto_drv_common.h"
#include "hal_pke_v5.h"
#include "pke_alg_inner.h"

int hal_pke_alg_ecc_compute_u1_and_u2(const drv_pke_data *w, const drv_pke_data *e, const drv_pke_data *r,
    const drv_pke_data *u1, const drv_pke_data *u2)
{
    int ret;
    unsigned int klen;
    unsigned int work_len;
    const drv_pke_ecc_curve *ecc_curve = NULL;
    hal_pke_alg_ecc_init_check();

    klen = hal_pke_alg_ecc_get_klen();
    ecc_curve = hal_pke_alg_ecc_get_curve();
    work_len = klen / ALIGNED_TO_WORK_LEN_IN_BYTE;

    /* Step 1: set data into PKE DRAM. the ecc_addr_rrn and ecc_addr_const_1 is set in initial parameters. */
    hal_pke_set_ram(sec_arg_add_cs(ecc_addr_e, e->data, e->length, klen));
    hal_pke_set_ram(sec_arg_add_cs(ecc_addr_r, r->data, r->length, klen));
    hal_pke_set_ram(sec_arg_add_cs(ecc_addr_s, w->data, w->length, klen));
    hal_pke_set_ram(sec_arg_add_cs(ecc_addr_m, ecc_curve->n, klen, klen));

    /* Step 2: start calculate. */
    ret = inner_batch_instr_process(&instr_ecfn_verify_u_10, work_len);
    crypto_chk_return(ret != CRYPTO_SUCCESS, ret, "inner_batch_instr_process failed\n");

    /* Step 3: get result out of PKE DRAM */
    hal_pke_get_ram(sec_arg_add_cs(ecc_addr_u1, u1->data, u1->length));
    hal_pke_get_ram(sec_arg_add_cs(ecc_addr_u2, u2->data, u2->length));

    return ret;
}