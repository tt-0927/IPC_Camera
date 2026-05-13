/**
  Copyright (c), 2001-2025, Shenshu Tech. Co., Ltd.
 */

#include "drv_pke_cal.h"
#include "hal_pke_alg.h"
#include "drv_pke_inner.h"

#include "crypto_drv_common.h"

td_s32 drv_cipher_pke_exp_mod(const drv_pke_data *n, const drv_pke_data *k, const drv_pke_data *in,
    const drv_pke_data *out)
{
    int ret;
    pke_null_ptr_chk(n);
    pke_null_ptr_chk(n->data);
    pke_null_ptr_chk(k);
    pke_null_ptr_chk(k->data);
    pke_null_ptr_chk(in);
    pke_null_ptr_chk(in->data);
    pke_null_ptr_chk(out);
    pke_null_ptr_chk(out->data);

#if defined(CONFIG_PKE_TRACE_ENABLE)
    crypto_dump_data("n", n->data, n->length);
    crypto_dump_data("k", k->data, k->length);
    crypto_dump_data("in", in->data, in->length);
#endif
    ret = hal_pke_alg_rsa_exp_mod(n, k, in, out);

#if defined(CONFIG_PKE_TRACE_ENABLE)
    crypto_dump_data("out = in^k mod n", out->data, out->length);
#endif
    return ret;
}