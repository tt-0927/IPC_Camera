/**
  Copyright (c), 2001-2025, Shenshu Tech. Co., Ltd.
*/

#include "ecc_curve_param.h"

const td_u8 g_const_0[DRV_PKE_LEN_576] = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

const td_u8 g_const_1[DRV_PKE_LEN_576] = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01
};

static const pke_default_parameters g_crypto_ecc_params[] = {
#ifdef CONFIG_PKE_SUPPORT_ECC_BP160R
    {&g_brainpool_160r1_param, &g_brainpool_160r1_init_param},
#endif
#ifdef CONFIG_PKE_SUPPORT_ECC_BP192R
    {&g_brainpool_192r1_param, &g_brainpool_192r1_init_param},
#endif
#ifdef CONFIG_PKE_SUPPORT_ECC_BP224R
    {&g_brainpool_224r1_param, &g_brainpool_224r1_init_param},
#endif
#ifdef CONFIG_PKE_SUPPORT_ECC_BP256R
    {&g_brainpool_256r1_param, &g_brainpool_256r1_init_param},
#endif
#ifdef CONFIG_PKE_SUPPORT_ECC_BP320R
    {&g_brainpool_320r1_param, &g_brainpool_320r1_init_param},
#endif
#ifdef CONFIG_PKE_SUPPORT_ECC_BP384R
    {&g_brainpool_384r1_param, &g_brainpool_384r1_init_param},
#endif
#ifdef CONFIG_PKE_SUPPORT_ECC_BP512R
    {&g_brainpool_512r1_param, &g_brainpool_512r1_init_param},
#endif
#ifdef CONFIG_PKE_SUPPORT_ECC_FIPS_P192R
    {&g_nist_p192_param, &g_nist_p192_init_param},
#endif
#ifdef CONFIG_PKE_SUPPORT_ECC_FIPS_P224R
    {&g_nist_p224_param, &g_nist_p224_init_param},
#endif
#ifdef CONFIG_PKE_SUPPORT_ECC_FIPS_P256R
    {&g_nist_p256_param, &g_nist_p256_init_param},
#endif
#ifdef CONFIG_PKE_SUPPORT_ECC_FIPS_P384R
    {&g_nist_p384_param, &g_nist_p384_init_param},
#endif
#ifdef CONFIG_PKE_SUPPORT_ECC_FIPS_P521R
    {&g_nist_p521_param, &g_nist_p521_init_param},
#endif
#ifdef CONFIG_PKE_SUPPORT_ECC_CURVE_448
    {&g_curve_448_param, &g_curve_448_init_param},
#endif
#if defined(CONFIG_PKE_SUPPORT_SM2)
    {&g_sm2_param, &g_sm2_init_param},
#endif
};
static const td_u32 g_crypto_ecc_num = sizeof(g_crypto_ecc_params) / sizeof(g_crypto_ecc_params[0]);

td_void crypto_curve_param_init(td_void)
{
    hal_pke_alg_init_ecc_param(g_crypto_ecc_params, g_crypto_ecc_num);
}

td_void crypto_curve_param_deinit(td_void)
{
    hal_pke_alg_init_ecc_param(NULL, 0);
}