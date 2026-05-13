/**
  Copyright (c), 2001-2025, Shenshu Tech. Co., Ltd.
 */

#ifndef ECC_CURVE_PARAM_H
#define ECC_CURVE_PARAM_H

#include "crypto_pke_common.h"

#define PKE_MONT_PARAM_LEN 2
#define PKE_LEN_160_OFFSET 48
#define PKE_LEN_192_OFFSET 48
#define PKE_LEN_224_OFFSET 40
#define PKE_LEN_256_OFFSET 40
#define PKE_LEN_320_OFFSET 32
#define PKE_LEN_384_OFFSET 24
#define PKE_LEN_448_OFFSET 16
#define PKE_LEN_512_OFFSET 8

extern const td_u8 g_const_0[DRV_PKE_LEN_576];
extern const td_u8 g_const_1[DRV_PKE_LEN_576];

/* NIST-P192 */
extern const drv_pke_ecc_curve g_nist_p192_param;
extern const pke_ecc_init_param g_nist_p192_init_param;

/* NIST-P224 */
extern const drv_pke_ecc_curve g_nist_p224_param;
extern const pke_ecc_init_param g_nist_p224_init_param;

/* NIST-P256 */
extern const drv_pke_ecc_curve g_nist_p256_param;
extern const pke_ecc_init_param g_nist_p256_init_param;

/* NIST-P384 */
extern const drv_pke_ecc_curve g_nist_p384_param;
extern const pke_ecc_init_param g_nist_p384_init_param;

/* NIST-P521 */
extern const drv_pke_ecc_curve g_nist_p521_param;
extern const pke_ecc_init_param g_nist_p521_init_param;

/* Brainpool-160R1 */
extern const drv_pke_ecc_curve g_brainpool_160r1_param;
extern const pke_ecc_init_param g_brainpool_160r1_init_param;

/* Brainpool-192R1 */
extern const drv_pke_ecc_curve g_brainpool_192r1_param;
extern const pke_ecc_init_param g_brainpool_192r1_init_param;

/* Brainpool-224R1 */
extern const drv_pke_ecc_curve g_brainpool_224r1_param;
extern const pke_ecc_init_param g_brainpool_224r1_init_param;

/* Brainpool-256R1 */
extern const drv_pke_ecc_curve g_brainpool_256r1_param;
extern const pke_ecc_init_param g_brainpool_256r1_init_param;

/* Brainpool-320R1 */
extern const drv_pke_ecc_curve g_brainpool_320r1_param;
extern const pke_ecc_init_param g_brainpool_320r1_init_param;

/* Brainpool-384R1 */
extern const drv_pke_ecc_curve g_brainpool_384r1_param;
extern const pke_ecc_init_param g_brainpool_384r1_init_param;

/* Brainpool-512R1 */
extern const drv_pke_ecc_curve g_brainpool_512r1_param;
extern const pke_ecc_init_param g_brainpool_512r1_init_param;

/* SM2 */
extern const drv_pke_ecc_curve g_sm2_param;
extern const pke_ecc_init_param g_sm2_init_param;

/* ED25519 */

/* Curve448 */
extern const drv_pke_ecc_curve g_curve_448_param;
extern const pke_ecc_init_param g_curve_448_init_param;

/* Curve25519 */

#endif