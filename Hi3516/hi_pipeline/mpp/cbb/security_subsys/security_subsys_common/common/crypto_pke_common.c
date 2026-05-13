/**
  Copyright (c), 2001-2025, Shenshu Tech. Co., Ltd.
 */
#include "crypto_pke_common.h"

static const pke_default_parameters *g_ecc_params;
static td_u32 g_ecc_num = 0;

td_void hal_pke_alg_init_ecc_param(const pke_default_parameters *ecc_params, uint32_t ecc_num)
{
    g_ecc_params = ecc_params;
    g_ecc_num = ecc_num;
}

int get_ecc_param(drv_pke_ecc_curve_type curve_type,
    const drv_pke_ecc_curve **ecc_curve, const pke_ecc_init_param **ecc_init_param)
{
    uint32_t i;
    const pke_default_parameters *param = NULL;
    if (g_ecc_params == NULL) {
        crypto_log_err("call drv_cipher_pke_init_ecc_param first!\n");
        return CRYPTO_FAILURE;
    }

    crypto_chk_return(ecc_curve == NULL, CRYPTO_FAILURE, "ecc_curve is NULL\n");
    crypto_chk_return(ecc_init_param == NULL, CRYPTO_FAILURE, "ecc_init_param is NULL\n");

    for (i = 0; i < g_ecc_num; i++) {
        param = &g_ecc_params[i];
        if (param->curve_param == NULL) {
            continue;
        }
        if (param->curve_param->ecc_type == curve_type) {
            *ecc_curve = param->curve_param;
            *ecc_init_param = param->default_param;
            return CRYPTO_SUCCESS;
        }
    }

    *ecc_curve = NULL;
    *ecc_init_param = NULL;
    crypto_log_err("curve_type 0x%x not register!\n", curve_type);
    return DRV_COMPAT_ERRNO(ERROR_MODULE_PKE, ERROR_UNSUPPORT);
}