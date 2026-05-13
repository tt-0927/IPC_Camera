/**
  Copyright (c), 2001-2025, Shenshu Tech. Co., Ltd.
 */
#include "hal_pke_alg.h"

#include "crypto_pke_common.h"
#include "hal_pke_v5.h"
#include "drv_pke_inner.h"
#include "pke_alg_inner.h"

/* ECC Curve Parameters. */
static const drv_pke_ecc_curve *g_ecc_curve = NULL;
static const pke_ecc_init_param *g_init_param = NULL;
static td_bool g_ecc_init_flag = TD_FALSE;

int hal_pke_alg_ecc_init(drv_pke_ecc_curve_type curve_type)
{
    int ret = CRYPTO_FAILURE;
    if (g_ecc_init_flag == TD_TRUE) {
        return CRYPTO_SUCCESS;
    }

    ret = inner_pke_alg_resume();
    crypto_chk_return(ret != CRYPTO_SUCCESS, ret, "inner_pke_alg_resume failed\n");

    ret = get_ecc_param(curve_type, &g_ecc_curve, &g_init_param);
    crypto_chk_goto(ret != CRYPTO_SUCCESS, error_unlock, "get_ecc_param failed\n");
    crypto_chk_goto_with_ret(ret, g_ecc_curve == NULL, error_unlock, CRYPTO_FAILURE,
        "get_ecc_param for ecc_curve failed\n");
    crypto_chk_goto_with_ret(ret, g_init_param == NULL, error_unlock, CRYPTO_FAILURE,
        "get_ecc_param for init_param failed\n");

    ret = hal_pke_set_init_param(g_init_param, g_ecc_curve);
    crypto_chk_goto(ret != CRYPTO_SUCCESS, error_unlock, "hal_pke_set_init_param failed\n");

    g_ecc_init_flag = TD_TRUE;
    return ret;

error_unlock:
    inner_pke_alg_suspend();
    g_ecc_curve = NULL;
    g_init_param = NULL;
    return ret;
}

void hal_pke_alg_ecc_deinit(void)
{
    if (g_ecc_init_flag == TD_FALSE) {
        return;
    }
    g_ecc_curve = TD_NULL;
    g_init_param = TD_NULL;
    inner_pke_alg_suspend();
    g_ecc_init_flag = TD_FALSE;
}

td_bool hal_pke_alg_ecc_is_init(void)
{
    return g_ecc_init_flag;
}

const unsigned char *hal_pke_alg_ecc_get_n(void)
{
    return g_ecc_curve->n;
}

const unsigned char *hal_pke_alg_ecc_get_p_minus_2(void)
{
    return g_init_param->p_minus_2;
}

unsigned int hal_pke_alg_ecc_get_klen(void)
{
    if (g_ecc_init_flag == TD_FALSE) {
        return 0;
    }
    return g_ecc_curve->ksize;
}

const drv_pke_ecc_curve *hal_pke_alg_ecc_get_curve(void)
{
    return g_ecc_curve;
}

const pke_ecc_init_param *hal_pke_alg_ecc_get_init_param(void)
{
    return g_init_param;
}