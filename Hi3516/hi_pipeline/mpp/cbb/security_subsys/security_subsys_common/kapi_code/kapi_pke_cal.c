/**
  Copyright (c), 2001-2025, Shenshu Tech. Co., Ltd.
*/

#include "kapi_pke_cal.h"
#include "drv_pke_cal.h"
#include "crypto_drv_common.h"

#if defined(CONFIG_PKE_ADD_MOD_SUPPORT)
td_s32 kapi_cipher_pke_add_mod(const drv_pke_data *a, const drv_pke_data *b, const drv_pke_data *p,
    const drv_pke_data *c)
{
    crypto_unused(a);
    crypto_unused(b);
    crypto_unused(p);
    crypto_unused(c);

    return ERROR_UNSUPPORT;
}
#endif

#if defined(CONFIG_PKE_SUB_MOD_SUPPORT)
td_s32 kapi_cipher_pke_sub_mod(const drv_pke_data *a, const drv_pke_data *b, const drv_pke_data *p,
    const drv_pke_data *c)
{
    crypto_unused(a);
    crypto_unused(b);
    crypto_unused(p);
    crypto_unused(c);

    return ERROR_UNSUPPORT;
}
#endif

#if defined(CONFIG_PKE_MUL_MOD_SUPPORT)
td_s32 kapi_cipher_pke_mul_mod(const drv_pke_data *a, const drv_pke_data *b, const drv_pke_data *p,
    const drv_pke_data *c)
{
    crypto_unused(a);
    crypto_unused(b);
    crypto_unused(p);
    crypto_unused(c);

    return ERROR_UNSUPPORT;
}
#endif

#if defined(CONFIG_PKE_INV_MOD_SUPPORT)
td_s32 kapi_cipher_pke_inv_mod(const drv_pke_data *a, const drv_pke_data *p, const drv_pke_data *c)
{
    crypto_unused(a);
    crypto_unused(p);
    crypto_unused(c);

    return ERROR_UNSUPPORT;
}
#endif

#if defined(CONFIG_PKE_MOD_SUPPORT)
td_s32 kapi_cipher_pke_mod(const drv_pke_data *a, const drv_pke_data *p, const drv_pke_data *c)
{
    td_s32 ret = TD_SUCCESS;

#if defined(CONFIG_HI3519DV500)
    ret = drv_cipher_pke_lock_secure();
    crypto_chk_return(ret != TD_SUCCESS, ret, "drv_cipher_pke_lock_secure failed, ret is 0x%x\n", ret);
#endif
    ret = drv_cipher_pke_mod(a, p, c);
#if defined(CONFIG_HI3519DV500)
    crypto_chk_goto(ret != TD_SUCCESS, unlock_exit, "drv_cipher_pke_mod failed, ret is 0x%x\n", ret);
unlock_exit:
    (void)drv_cipher_pke_unlock_secure();
#endif

    return ret;
}
#else
td_s32 kapi_cipher_pke_mod(const drv_pke_data *a, const drv_pke_data *p, const drv_pke_data *c)
{
    crypto_unused(a);
    crypto_unused(p);
    crypto_unused(c);

    return ERROR_UNSUPPORT;
}
#endif

#if defined(CONFIG_PKE_MUL_SUPPORT)
td_s32 kapi_cipher_pke_mul(const drv_pke_data *a, const drv_pke_data *b, const drv_pke_data *c)
{
    crypto_unused(a);
    crypto_unused(b);
    crypto_unused(c);

    return ERROR_UNSUPPORT;
}
#endif

#if defined(CONFIG_PKE_EXP_MOD_SUPPORT)
td_s32 kapi_cipher_pke_exp_mod(const drv_pke_data *n, const drv_pke_data *k, const drv_pke_data *in,
    const drv_pke_data *out)
{
    td_s32 ret = TD_SUCCESS;

#if defined(CONFIG_HI3519DV500)
    ret = drv_cipher_pke_lock_secure();
    crypto_chk_return(ret != TD_SUCCESS, ret, "drv_cipher_pke_lock_secure failed, ret is 0x%x\n", ret);
#endif
    ret = drv_cipher_pke_exp_mod(n, k, in, out);
#if defined(CONFIG_HI3519DV500)
    crypto_chk_goto(ret != TD_SUCCESS, unlock_exit, "drv_cipher_pke_exp_mod failed, ret is 0x%x\n", ret);
unlock_exit:
    (void)drv_cipher_pke_unlock_secure();
#endif
    return ret;
}
#else
td_s32 kapi_cipher_pke_exp_mod(const drv_pke_data *n, const drv_pke_data *k, const drv_pke_data *in,
    const drv_pke_data *out)
{
    crypto_unused(n);
    crypto_unused(k);
    crypto_unused(in);
    crypto_unused(out);

    return ERROR_UNSUPPORT;
}
#endif