/**
  Copyright (c), 2001-2025, Shenshu Tech. Co., Ltd.
 */

#ifndef HAL_PKE_ALG_H
#define HAL_PKE_ALG_H

#include "crypto_pke_struct.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif /* __cplusplus */
#endif /* __cplusplus */

#define MAX_ECC_SIZE                    72
#define ALIGNED_TO_WORK_LEN_IN_BYTE     8

#define hal_pke_alg_ecc_init_check()    \
    crypto_chk_return(hal_pke_alg_ecc_is_init() == TD_FALSE, CRYPTO_FAILURE, "call hal_pke_alg_ecc_init first!\n")

int hal_pke_alg_ecc_init(drv_pke_ecc_curve_type curve_type);

void hal_pke_alg_ecc_deinit(void);

const unsigned char *hal_pke_alg_ecc_get_n(void);

const unsigned char *hal_pke_alg_ecc_get_p_minus_2(void);

unsigned int hal_pke_alg_ecc_get_klen(void);

const drv_pke_ecc_curve *hal_pke_alg_ecc_get_curve(void);

const pke_ecc_init_param *hal_pke_alg_ecc_get_init_param(void);

td_bool hal_pke_alg_ecc_is_init(void);

/**
 * @ingroup hal_pke_alg_ecc
 * @brief   point multiplication with base point
 * @details r(x, y) = k * G(x, y)
 *
 * @param   ecc [IN] curve parameters
 * @param   k [IN] multiplication number
 * @param   r [OUT] result point
 *
 * @attention inner function, assuming all the parameters are legal.
 * @retval  CRYPTO_SUCCESS
 * @retval  CRYPTO_FAILURE
 */
int hal_pke_alg_ecc_base_point_mul(const drv_pke_data *k, const drv_pke_ecc_point *r);

/**
 * @ingroup hal_pke_alg_ecc
 * @brief   point multiplication
 * @details r(x, y) = k * p(x, y)
 *
 * @param   ecc [IN] curve parameters
 * @param   k [IN] multiplication number
 * @param   p [IN] multiplication point
 * @param   r [OUT] result point
 *
 * @attention inner function, assuming all the parameters are legal.
 * @retval  CRYPTO_SUCCESS
 * @retval  CRYPTO_FAILURE
 */
int hal_pke_alg_ecc_point_mul(const drv_pke_data *k, const drv_pke_ecc_point *p, const drv_pke_ecc_point *r);

/**
 * @ingroup hal_pke_alg_ecc
 * @brief   ecc compute s
 * @details s = k^-1(e + dr) mod n
 *
 * @param   k [IN]
 * @param   e [IN]
 * @param   d [IN]
 * @param   r [IN]
 * @param   s [OUT]
 *
 * @attention inner function, assuming all the parameters are legal.
 * @retval  CRYPTO_SUCCESS
 * @retval  CRYPTO_FAILURE
 */
int hal_pke_alg_ecc_compute_s(const drv_pke_data *k, const drv_pke_data *e, const drv_pke_data *d,
    const drv_pke_data *r, const drv_pke_data *s);

/**
 * @ingroup hal_pke_alg_ecc
 * @brief   modulo inverse
 * @details c = a^-1 mod n
 *
 * @param   a [IN] base number
 * @param   c [OUT] result bignumber
 *
 * @attention inner function, assuming all the parameters are legal.
 * @retval  CRYPTO_SUCCESS
 * @retval  CRYPTO_FAILURE
 */
int hal_pke_alg_ecc_inv_mod(const drv_pke_data *a, const drv_pke_data *c);

/**
 * @ingroup hal_pke_alg_ecc
 * @brief   compute u1 and u2
 * @details u1 = w * e mod n, u2 = w * r mod n
 *
 * @param   w [IN] multiplication number
 * @param   e [IN] multiplication number
 * @param   r [IN] multiplication number
 * @param   u1 [OUT] result bignumber
 * @param   u2 [OUT] result bignumber
 *
 * @attention inner function, assuming all the parameters are legal.
 * @retval  CRYPTO_SUCCESS
 * @retval  CRYPTO_FAILURE
 */
int hal_pke_alg_ecc_compute_u1_and_u2(const drv_pke_data *w, const drv_pke_data *e, const drv_pke_data *r,
    const drv_pke_data *u1, const drv_pke_data *u2);

/**
 * @ingroup hal_pke_alg_ecc
 * @brief   compute point x
 * @details x(x, y) = u1 * p(x, y) + u2 * q(x, y), p(x, y) is the base point of ecc curve.
 *
 * @param   u1 [IN] multiplication number
 * @param   u2 [IN] multiplication number
 * @param   q [IN] multiplication point
 * @param   x [OUT] result point
 *
 * @attention inner function, assuming all the parameters are legal.
 * @retval  CRYPTO_SUCCESS
 * @retval  CRYPTO_FAILURE
 */
int hal_pke_alg_ecc_compute_x(const drv_pke_data *u1, const drv_pke_data *u2,
    const drv_pke_ecc_point *q, const drv_pke_data *x);

/**
 * @ingroup hal_pke_alg
 * @brief   sm2 compute s * G + t * P.
 * @details r(x, y) = s * G(x, y) + t * P(x, y), G(x, y) is the base point of ecc curve.
 *
 * @param   ecc [IN] curve parameters
 * @param   s [IN] multiplication number
 * @param   t [IN] multiplication number
 * @param   p [IN] multiplication point
 * @param   r [OUT] result point
 *
 * @attention inner function, assuming all the parameters are legal.
 * @retval  CRYPTO_SUCCESS
 * @retval  CRYPTO_FAILURE
 */
int32_t hal_pke_alg_sm2_point_mul_add(const drv_pke_ecc_curve *ecc, const drv_pke_data *s, const drv_pke_data *t,
    const drv_pke_ecc_point *p, const drv_pke_ecc_point *r);

/**
 * @ingroup hal_pke_alg
 * @brief   sm2 compute signature s part.
 * @details compute s = ((1 + d)^(-1) * (k - r*d)) mod n
 *
 * @param   d [IN] private key
 * @param   k [IN] random key
 * @param   r [IN] signature r part
 * @param   n [IN] modulo number
 * @param   s [IN] result signature s part
 *
 * @attention inner function, assuming all the parameters are legal.
 * @retval  CRYPTO_SUCCESS
 * @retval  CRYPTO_FAILURE
 */
int hal_pke_alg_sm2_compute_s(const drv_pke_data *d, const drv_pke_data *k,
    const drv_pke_data *r, const drv_pke_data *s);

/**
 * @ingroup hal_pke_alg
 * @brief   sm2 add mod
 * @details compute c = (a + b) mod n.
 *
 * @param   a [IN] addition number
 * @param   b [IN] addition number
 * @param   n [IN] modulo number
 * @param   c [OUT] result number
 *
 * @attention inner function, assuming all the parameters are legal.
 * @retval  CRYPTO_SUCCESS
 * @retval  CRYPTO_FAILURE
 */
int hal_pke_alg_sm2_add_mod(const drv_pke_data *a, const drv_pke_data *b, const drv_pke_data *c);

int hal_pke_alg_rsa_mod(const drv_pke_data *a, const drv_pke_data *p, const drv_pke_data *c);

int hal_pke_alg_rsa_exp_mod(const drv_pke_data *n, const drv_pke_data *k, const drv_pke_data *in,
    const drv_pke_data *out);

int hal_pke_alg_calc_hash(const drv_pke_data *arr, unsigned int arr_len,
    const drv_pke_hash_type hash_type, drv_pke_data *hash);

int hal_pke_alg_ecc_check_dot_on_curve(const drv_pke_ecc_point *point, td_bool *is_on_curve);

td_s32 drv_cipher_pke_get_multi_random(td_u8 *random, td_u32 size);

#ifdef __cplusplus
#if __cplusplus
}
#endif /* __cplusplus */
#endif /* __cplusplus */

#endif