/**
  Copyright (c), 2001-2025, Shenshu Tech. Co., Ltd.
 */
#if defined(CONFIG_PKE_ECC_SM2_SIGN_SUPPORT)

#include "hal_pke_alg.h"

#include "crypto_drv_common.h"
#include "hal_pke_v5.h"
#include "pke_alg_inner.h"

static td_s32 sm2_ecfn_sign_s(const drv_pke_data *k, const drv_pke_data *d, const drv_pke_data *n,
    const drv_pke_data *r, const drv_pke_data *s)
{
    td_s32 ret = TD_FAILURE;
    td_u32 work_len = n->length / ALIGNED_TO_WORK_LEN_IN_BYTE;
    td_u8 k_inv[DRV_PKE_LEN_576] = {0};
    drv_pke_data aa = {0};
    crypto_drv_func_enter();

    /* Step 1: set data into DRAM. */
    hal_pke_set_ram(sec_arg_add_cs(ecc_addr_k, k->data, k->length, n->length));
    hal_pke_set_ram(sec_arg_add_cs(ecc_addr_d, d->data, d->length, n->length));
    hal_pke_set_ram(sec_arg_add_cs(ecc_addr_r, r->data, r->length, n->length));
    /* set module, the addr is ecc_addr_m */
    hal_pke_set_ram(sec_arg_add_cs(ecc_addr_m, n->data, n->length, n->length));

    /* Step 2: start calculate. montgomery the data, and calculate (1 + dA) & (k - r*dA). */
    ret = inner_batch_instr_process(&instr_sm2_sign_s_pre_6, work_len);
    crypto_chk_func_return(inner_batch_instr_process, ret);
    /* Step 2.1 get data from the DRAM. 1+da from ecc_addr_k, k-r*da from ecc_addr_e */
    hal_pke_get_ram(sec_arg_add_cs(ecc_addr_k, k_inv, n->length));

    /* Step 3: calculate the (1+da)^(-1). */
    aa = (drv_pke_data) {.data = k_inv, .length = n->length};

    ret = hal_pke_alg_ecc_inv_mod(&aa, s);
    crypto_chk_return(ret != CRYPTO_SUCCESS, ret, "hal_pke_alg_ecc_inv_mod failed\n");

    /* Step 4: calculate k_inv * (k - r*da) mod n. and demontgomery the r & s. */
    ret = inner_batch_instr_process(&instr_sm2_sign_s_post_6, work_len);
    crypto_chk_func_return(inner_batch_instr_process, ret);

    /* Step 5: get data out from the DRAM. */
    hal_pke_get_ram(sec_arg_add_cs(ecc_addr_s, s->data, s->length));
    hal_pke_get_ram(sec_arg_add_cs(ecc_addr_r, r->data, r->length));

    crypto_drv_func_exit();
    return TD_SUCCESS;
}

int hal_pke_alg_sm2_compute_s(const drv_pke_data *d, const drv_pke_data *k,
    const drv_pke_data *r, const drv_pke_data *s)
{
    int ret = CRYPTO_FAILURE;
    drv_pke_data n;
    const drv_pke_ecc_curve *ecc_curve = NULL;
    const pke_ecc_init_param *init_param = NULL;
    hal_pke_alg_ecc_init_check();

    ecc_curve = hal_pke_alg_ecc_get_curve();
    init_param = hal_pke_alg_ecc_get_init_param();
    n.data = (unsigned char *)ecc_curve->n;
    n.length = ecc_curve->ksize;

    ret = inner_update_modulus(n.data, n.length, init_param->mont_param_n[1], init_param->mont_param_n[0]);
    crypto_chk_func_return(inner_update_modulus, ret);

    ret = sm2_ecfn_sign_s(k, d, &n, r, s);
    crypto_chk_return(ret != CRYPTO_SUCCESS, ret, "sm2_ecfn_sign_s failed\n");

    return ret;
}
#endif