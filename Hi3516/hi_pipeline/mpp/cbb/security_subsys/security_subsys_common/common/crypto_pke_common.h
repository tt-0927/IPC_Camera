/**
  Copyright (c), 2001-2025, Shenshu Tech. Co., Ltd.
 */
#ifndef CRYPTO_PKE_COMMON_H
#define CRYPTO_PKE_COMMON_H

#include "crypto_pke_struct.h"
#include "crypto_drv_common.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif /* __cplusplus */
#endif /* __cplusplus */

td_void hal_pke_alg_init_ecc_param(const pke_default_parameters *ecc_params, uint32_t ecc_num);

const drv_pke_ecc_curve *get_ecc_curve(drv_pke_ecc_curve_type curve_type);

const pke_ecc_init_param *get_ecc_init_param(drv_pke_ecc_curve_type curve_type);

int get_ecc_param(drv_pke_ecc_curve_type curve_type,
    const drv_pke_ecc_curve **ecc_curve, const pke_ecc_init_param **ecc_init_param);

td_void crypto_curve_param_init(td_void);

td_void crypto_curve_param_deinit(td_void);

#ifdef __cplusplus
#if __cplusplus
}
#endif /* __cplusplus */
#endif /* __cplusplus */

#endif