/**
  Copyright (c), 2001-2025, Shenshu Tech. Co., Ltd.
 */
#if defined(CONFIG_PKE_SUPPORT_SM2)
#include "hal_pke_alg.h"

#include "crypto_drv_common.h"
#include "hal_pke_v5.h"
#include "pke_alg_inner.h"

int hal_pke_alg_sm2_add_mod(const drv_pke_data *a, const drv_pke_data *b, const drv_pke_data *c)
{
    int ret = CRYPTO_FAILURE;
    unsigned int klen;
    unsigned int work_len;
    const drv_pke_ecc_curve *ecc_curve = NULL;
    hal_pke_alg_ecc_init_check();

    ecc_curve = hal_pke_alg_ecc_get_curve();

    klen = ecc_curve->ksize;
    work_len = klen / ALIGNED_TO_WORK_LEN_IN_BYTE;

    /* Step 1: set data into DRAM. */
    hal_pke_set_ram(sec_arg_add_cs(ecc_addr_r, a->data, a->length, klen));
    hal_pke_set_ram(sec_arg_add_cs(ecc_addr_s, b->data, b->length, klen));
    hal_pke_set_ram(sec_arg_add_cs(ecc_addr_m, ecc_curve->n, klen, klen));

    /* Step 2: start calculate. */
    ret = inner_batch_instr_process(&instr_sm2_verify_t_3, work_len);
    crypto_chk_return(ret != CRYPTO_SUCCESS, ret, "inner_batch_instr_process failed\n");

    /* Step 3: get data out of DRAM. */
    hal_pke_get_ram(sec_arg_add_cs(ecc_addr_t, c->data, c->length));

    return CRYPTO_SUCCESS;
}
#endif