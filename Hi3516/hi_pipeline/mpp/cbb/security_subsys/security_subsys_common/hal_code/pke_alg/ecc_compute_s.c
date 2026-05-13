/**
  Copyright (c), 2001-2025, Shenshu Tech. Co., Ltd.
 */
#if defined(CONFIG_PKE_ECC_ECDSA_SIGN_SUPPORT)

#include "hal_pke_alg.h"

#include "crypto_drv_common.h"
#include "hal_pke_v5.h"
#include "pke_alg_inner.h"

static td_s32 ecc_ecfn_sign_s(const drv_pke_data *k_inv, const drv_pke_data *e, const drv_pke_data *d,
    const drv_pke_data *r, const drv_pke_data *n, const drv_pke_data *s)
{
    td_s32 ret = TD_FAILURE;
    td_u32 work_len;
    crypto_drv_func_enter();
    check_sum_inspect(PKE_COMPAT_ERRNO(ERROR_INVALID_PARAM), k_inv, e, d, r, n, s);

    work_len = n->length / ALIGNED_TO_WORK_LEN_IN_BYTE;
    /* 1. check free, no need here, for the former calculation will definitely wait for PKE not busy. */

    /* Step 1: set data into PKE DRAM. the ecc_addr_rrn and ecc_addr_const_1 have been set in intial parameters. */
    hal_pke_set_ram(sec_arg_add_cs(ecc_addr_r, r->data, r->length, n->length));
    hal_pke_set_ram(sec_arg_add_cs(ecc_addr_e, e->data, e->length, n->length));
    hal_pke_set_ram(sec_arg_add_cs(ecc_addr_d, d->data, d->length, n->length));
    hal_pke_set_ram(sec_arg_add_cs(ecc_addr_s, k_inv->data, k_inv->length, n->length));
    hal_pke_set_ram(sec_arg_add_cs(ecc_addr_n, n->data, n->length, n->length));

    /* Step 2. start calculate. */
    ret = inner_batch_instr_process(&instr_ecfn_sign_s_12, work_len);
    crypto_chk_func_return(inner_batch_instr_process, ret);

    /* Step 3: get data from PKE DRAM */
    hal_pke_get_ram(sec_arg_add_cs(ecc_addr_s, s->data, s->length));

    crypto_drv_func_exit();
    return TD_SUCCESS;
}

/* s = k^-1(e + dr) mod n */
int hal_pke_alg_ecc_compute_s(const drv_pke_data *k, const drv_pke_data *e, const drv_pke_data *d,
    const drv_pke_data *r, const drv_pke_data *s)
{
    int ret;
    unsigned char k_inv_buf[MAX_ECC_SIZE];
    drv_pke_data k_inv_data = {
        .data = k_inv_buf,
        .length = k->length
    };
    drv_pke_data n = {0};
    const drv_pke_ecc_curve *ecc_curve = NULL;
    hal_pke_alg_ecc_init_check();

    ecc_curve = hal_pke_alg_ecc_get_curve();
    n.data = (unsigned char *)ecc_curve->n;
    n.length = ecc_curve->ksize;
    /* Step 1. Compute k_inv. */
    ret = hal_pke_alg_ecc_inv_mod(k, &k_inv_data);
    crypto_chk_return(ret != CRYPTO_SUCCESS, ret, "hal_pke_alg_ecc_inv_mod failed\n");

    /* Step 2. Compute s = k_inv * (e + d * r) mod n. */
    ret = ecc_ecfn_sign_s(&k_inv_data, e, d, r, &n, s);
    crypto_chk_return(ret != CRYPTO_SUCCESS, ret, "ecc_ecfn_sign_s failed\n");

    return ret;
}
#endif