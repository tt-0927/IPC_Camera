/**
  Copyright (c), 2001-2025, Shenshu Tech. Co., Ltd.
*/

#ifndef KAPI_PKE_HAL_H
#define KAPI_PKE_HAL_H

#include "crypto_type.h"
#include "crypto_pke_struct.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif /* __cplusplus */
#endif /* __cplusplus */

/*
 * normal big number calculate function.
 */
td_s32 kapi_cipher_pke_add_mod(const drv_pke_data *a, const drv_pke_data *b, const drv_pke_data *p,
    const drv_pke_data *c);

td_s32 kapi_cipher_pke_sub_mod(const drv_pke_data *a, const drv_pke_data *b, const drv_pke_data *p,
    const drv_pke_data *c);

td_s32 kapi_cipher_pke_mul_mod(const drv_pke_data *a, const drv_pke_data *b, const drv_pke_data *p,
    const drv_pke_data *c);

td_s32 kapi_cipher_pke_inv_mod(const drv_pke_data *a, const drv_pke_data *p, const drv_pke_data *c);

td_s32 kapi_cipher_pke_mod(const drv_pke_data *a, const drv_pke_data *p, const drv_pke_data *c);

td_s32 kapi_cipher_pke_mul(const drv_pke_data *a, const drv_pke_data *b, const drv_pke_data *c);

td_s32 kapi_cipher_pke_exp_mod(const drv_pke_data *n, const drv_pke_data *k, const drv_pke_data *in,
    const drv_pke_data *out);

#ifdef __cplusplus
#if __cplusplus
}
#endif /* __cplusplus */
#endif /* __cplusplus */


#endif