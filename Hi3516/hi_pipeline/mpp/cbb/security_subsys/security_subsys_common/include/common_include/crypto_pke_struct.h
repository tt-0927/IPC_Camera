/**
  Copyright (c), 2001-2025, Shenshu Tech. Co., Ltd.
 */
#ifndef CRYPTO_PKE_STRUCT_H
#define CRYPTO_PKE_STRUCT_H

#include "ot_common_crypto.h"
#include "crypto_common_struct.h"


#define CRYPTO_PKE_CURVE_ID_MASK     0x000FFC00
#define CRYPTO_PKE_CURVE_ID_SHIFT    10
#define CRYPTO_PKE_KLEN_MASK        0x000003FF
#define CRYPTO_PKE_KLEN_SHIFT       0

#define crypto_pke_get_attr(value, mask, shift)         (((td_u32)(value) & (td_u32)(mask)) >> (shift))

#define crypto_pke_get_klen(curve_type)              \
    crypto_pke_get_attr(curve_type, CRYPTO_PKE_KLEN_MASK, CRYPTO_PKE_KLEN_SHIFT)
\

/* * struct of ecc curves parameters. */
typedef struct {
    const td_u8 *p;   /* prime specifying the base field. It is p (RFC5639), p (FIPS), p (RFC7748). */
    const td_u8 *a;   /* Curve parameter a. It is A (RFC5639), c (FIPS), A24 (RFC7748), d(RFC8032). */
    const td_u8 *b;   /* Curve parameter b. It is B (RFC5639), b (FIPS), N/A (RFC7748, RFC8032). */
    const td_u8 *gx;  /* X coordinates of G which is a base point on the curve.
                         It is x (RFC5639), Gx (FIPS), U(P) (RFC7748). */
    const td_u8 *gy;  /* Y coordinates of G which is a base point on the curve.
                         It is y (RFC5639), Gy (FIPS), N/A (RFC7748). */
    const td_u8 *n;   /* Prime which is the order of G point. It is q (RFC5639), n (FIPS, RFC7748). */
    td_u32 h;         /* Cofactor, which is the order of the elliptic curve divided by the order of the point G.
                         It is h (RFC5639), h (FIPS), Cofactor (RFC7748). */
    drv_pke_len ksize;         /* Ecc key size in bytes. It corresponds to the size in bytes of the prime. */
    drv_pke_ecc_curve_type ecc_type; /* Type of ECC curve */
} drv_pke_ecc_curve;

typedef struct {
    const td_u8 *mont_a;    /* the montgomerized of parameter a(RFC5639, FIPS, SM2), a24(RFC7748), d(RFC8032). */
    const td_u8 *mont_b;    /* the montgomerized of parameter b(RFC5639, FIPS, SM2), N/A(RFC7748), sqrt_m1(RFC8032). */
    const td_u8 *mont_1_p;  /* the montgomerized of const value 1 (modp) */
    const td_u8 *mont_1_n;  /* the montgomerized of const value 1 (modn) */
    const td_u8 *rrp;  /* the montgomery parameter when modulur is p */
    const td_u8 *rrn;  /* the montgomery parameter when modulur is n */
    const td_u8 *const_1;
    const td_u8 *const_0;
    const td_u32 *mont_param_n;  /* the montgomerized parameter when the modulur is n. */
    const td_u32 *mont_param_p;  /* the montgomerized parameter when the modulur is p. */
    const td_u8 *n_minus_2;     /* the const value of n minus 2 */
    const td_u8 *p_minus_2;     /* the const value of p minus 2 */
} pke_ecc_init_param;

typedef struct {
    const drv_pke_ecc_curve *curve_param;
    const pke_ecc_init_param *default_param;
} pke_default_parameters;

/* Here the ram should have been set into the address, and the instructions have shown which DRAM will be used for
calculate. */
typedef struct {
    td_u32 instr_addr;     /* the start address for instructions, which is ROM or SRAM address. */
    td_u32 instr_num;       /* the number of instructions from the instr_addr, which will be used for calculate. */
} rom_lib;

#endif