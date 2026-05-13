/**
  Copyright (c), 2001-2025, Shenshu Tech. Co., Ltd.
 */

#include "drv_pke_cal.h"
#include "hal_pke_alg.h"
#include "drv_pke_inner.h"
#include "crypto_drv_common.h"

#define CONST_1_LEN     32
/* c = a mod p. */
td_s32 drv_cipher_pke_mod(const drv_pke_data *a, const drv_pke_data *p, const drv_pke_data *c)
{
    int ret;

    pke_null_ptr_chk(a);
    pke_null_ptr_chk(a->data);
    pke_null_ptr_chk(p);
    pke_null_ptr_chk(p->data);
    pke_null_ptr_chk(c);
    pke_null_ptr_chk(c->data);

#if defined(CONFIG_PKE_TRACE_ENABLE)
    crypto_dump_data("a", a->data, a->length);
    crypto_dump_data("p", p->data, p->length);
#endif
    ret = hal_pke_alg_rsa_mod(a, p, c);
#if defined(CONFIG_PKE_TRACE_ENABLE)
    crypto_dump_data("c = a mod p", c->data, c->length);
#endif
    return ret;
}