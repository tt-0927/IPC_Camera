/**
  Copyright (c), 2001-2025, Shenshu Tech. Co., Ltd.
 */
#if defined(CONFIG_PKE_ECC_SM2_PUB_ENC_SUPPORT)
#include "drv_pke.h"

#include "drv_pke_inner.h"
#include "hal_pke_alg.h"

#include "crypto_drv_common.h"

#define SM2_KEY_SIZE        32
#define SM2_PC_UNCOMPRESS   0x04
#define SM3_HASH_LEN        32
#define MAX_TRY_TIMES   100

/* According to SM2 Elliptic Curve Public Key cryptographic algorithm Part 4 public key encryption algorithm
 * "Section 6.1. Encryption algorithm"
 */
static td_s32 inner_sm2_public_encrypt(const drv_pke_ecc_point *pub_key, const drv_pke_data *plain_text,
    drv_pke_data *cipher_text)
{
    int ret = CRYPTO_FAILURE;
    unsigned int i;
    unsigned char k[SM2_KEY_SIZE];
    unsigned char kpb_x[SM2_KEY_SIZE];
    unsigned char kpb_y[SM2_KEY_SIZE];
    const unsigned char *sm2_n = hal_pke_alg_ecc_get_n();
    drv_pke_ecc_point c1_point = {
        .length = SM2_KEY_SIZE
    };
    drv_pke_data c3_data = {
        .length = SM2_KEY_SIZE
    };
    drv_pke_ecc_point kpb_point = {
        .x = kpb_x,
        .y = kpb_y,
        .length = SM2_KEY_SIZE
    };
    drv_pke_data k_data = {
        .data = k,
        .length = SM2_KEY_SIZE
    };
    unsigned int idx = 0;
    uint8_t *t = NULL;
    drv_pke_data arr[3];    /* 3: Compute C3 = Hash(x2 || M || y2). */

    /* cipher_text = C1 || C3 || C2 */
    /* According Part1 4.2.8, using uncompress representational form. */
    cipher_text->data[idx++] = SM2_PC_UNCOMPRESS;
    c1_point.x = &cipher_text->data[idx];
    c1_point.y = &cipher_text->data[idx + SM2_KEY_SIZE];
    idx += SM2_KEY_SIZE * 2;    // 2: for x and y.
    c3_data.data = &cipher_text->data[idx];
    idx += SM3_HASH_LEN;
    t = &cipher_text->data[idx];
    idx += plain_text->length;
    /* Step A1. generate k in [1, n - 1] */
    ret = inner_generate_random_in_range(k, sm2_n, pub_key->length);
    crypto_chk_return(ret != CRYPTO_SUCCESS, ret, "inner_drv_generate_random_in_range failed\n");

#if defined(CONFIG_PKE_SM2_TRACE_ENABLE)
    crypto_dump_data("random k", k, pub_key->length);
#endif

    /* Step A2. Compte C1(x1, y1) = k * G. */
#if defined(CONFIG_PKE_SM2_TRACE_ENABLE)
    crypto_print("========== Start Compute C1 = k * G ==========\n");
    crypto_dump_data("random k", k, pub_key->length);
#endif
    ret = hal_pke_alg_ecc_base_point_mul(&k_data, &c1_point);
    crypto_chk_return(ret != CRYPTO_SUCCESS, ret, "hal_pke_alg_ecc_base_point_mul failed\n");
#if defined(CONFIG_PKE_SM2_TRACE_ENABLE)
    crypto_dump_data("C1(x1)", c1_point.x, c1_point.length);
    crypto_dump_data("C1(y1)", c1_point.y, c1_point.length);
    crypto_print("========== End Compute C1 = k * G ==========\n");
#endif
    /* Step A3. Compute S = h * Pb, cause h is 1, so we skip this step.  */
    /* Step A4. Compute kPb(x2, y2) = k * Pb. */
#if defined(CONFIG_PKE_SM2_TRACE_ENABLE)
    crypto_print("========== Start Compute kPb(x2, y2) = k * Pb ==========\n");
    crypto_dump_data("random k", k, pub_key->length);
    crypto_dump_data("Pb(x)", pub_key->x, pub_key->length);
    crypto_dump_data("Pb(y)", pub_key->y, pub_key->length);
#endif
    ret = hal_pke_alg_ecc_point_mul(&k_data, pub_key, &kpb_point);
    crypto_chk_return(ret != CRYPTO_SUCCESS, ret, "hal_pke_alg_ecc_point_mul failed\n");
#if defined(CONFIG_PKE_SM2_TRACE_ENABLE)
    crypto_dump_data("kPb(x)", kpb_point.x, kpb_point.length);
    crypto_dump_data("kPb(y)", kpb_point.y, kpb_point.length);
    crypto_print("========== End Compute kPb(x2, y2) = k * Pb ==========\n");
#endif
    /*
     * Step A5. Compute t = kdf(x2 || y2, klen), if t = 0, goto A1.
     * Here klen is plain_text->length.
     */
#if defined(CONFIG_PKE_SM2_TRACE_ENABLE)
    crypto_print("========== Start Compute t = kdf(x2 || y2, klen) ==========\n");
    crypto_dump_data("x2", kpb_point.x, kpb_point.length);
    crypto_dump_data("y2", kpb_point.y, kpb_point.length);
#endif
    ret = inner_sm2_kdf(&kpb_point, t, plain_text->length);
    crypto_chk_return(ret != CRYPTO_SUCCESS, ret, "inner_sm2_kdf failed\n");
#if defined(CONFIG_PKE_SM2_TRACE_ENABLE)
    crypto_dump_data("t", t, plain_text->length);
    crypto_print("========== End Compute t = kdf(x2 || y2, klen) ==========\n");
#endif

    /* Step A6. Compute C2 = M ^ t. */
#if defined(CONFIG_PKE_SM2_TRACE_ENABLE)
    crypto_print("========== Start Compute Compute C2 = M ^ t ==========\n");
    crypto_dump_data("M", plain_text->data, plain_text->length);
    crypto_dump_data("t", t, plain_text->length);
#endif
    for (i = 0; i < plain_text->length; i++) {
        t[i] = t[i] ^ plain_text->data[i];
    }
#if defined(CONFIG_PKE_SM2_TRACE_ENABLE)
    crypto_dump_data("C2", t, plain_text->length);
    crypto_print("========== End Compute Compute C2 = M ^ t ==========\n");
#endif
    /* Step A7. Compute C3 = Hash(x2 || M || y2). */
#if defined(CONFIG_PKE_SM2_TRACE_ENABLE)
    crypto_print("========== Start Compute Compute C3 = Hash(x2 || M || y2). ==========\n");
    crypto_dump_data("x2", kpb_point.x, kpb_point.length);
    crypto_dump_data("M", plain_text->data, plain_text->length);
    crypto_dump_data("y2", kpb_point.y, kpb_point.length);
#endif
    arr[0].data = kpb_x;
    arr[0].length = SM2_KEY_SIZE;
    arr[1].data = plain_text->data;
    arr[1].length = plain_text->length;
    arr[2].data = kpb_y;    // 2: position to store kpb_y_data
    arr[2].length = SM2_KEY_SIZE;   // 2: position to store kpb_y_length
    ret = hal_pke_alg_calc_hash(arr, crypto_array_size(arr), DRV_PKE_HASH_TYPE_SM3, &c3_data);
    crypto_chk_return(ret != CRYPTO_SUCCESS, ret, "hal_pke_alg_calc_hash failed\n");
#if defined(CONFIG_PKE_SM2_TRACE_ENABLE)
    crypto_dump_data("C3", c3_data.data, c3_data.length);
    crypto_print("========== End Compute Compute C3 = Hash(x2 || M || y2) ==========\n");
#endif
    /* Step A8. Compute C = C1 || C2 || C3. We can skip this Step. */
    return ret;
}

td_s32 drv_cipher_pke_sm2_public_encrypt(const drv_pke_ecc_point *pub_key, const drv_pke_data *plain_text,
    drv_pke_data *cipher_text)
{
    unsigned int klen = DRV_PKE_LEN_256;
    int ret;
    unsigned int i;

    /* param check. */
    pke_null_ptr_chk(pub_key);
    pke_null_ptr_chk(pub_key->x);
    pke_null_ptr_chk(pub_key->y);
    pke_null_ptr_chk(plain_text);
    pke_null_ptr_chk(plain_text->data);
    pke_null_ptr_chk(cipher_text);
    pke_null_ptr_chk(cipher_text->data);

    /* length check. */
    crypto_chk_return(pub_key->length != klen, PKE_COMPAT_ERRNO(ERROR_INVALID_PARAM),
        "pub_key->length is invalid\n");
    /* For SM2 Crypto, the cipher_text is 97 longer than plain_text. */
    crypto_chk_return(cipher_text->length < plain_text->length + CRYPTO_SM2_ADD_LENGTH_IN_BYTE,
        PKE_COMPAT_ERRNO(ERROR_INVALID_PARAM), "cipher_text->length is not enough\n");

    ret = hal_pke_alg_ecc_init(DRV_PKE_ECC_TYPE_SM2);
    crypto_chk_return(ret != CRYPTO_SUCCESS, ret, "hal_pke_alg_ecc_init failed\n");

    for (i = 0; i < MAX_TRY_TIMES; i++) {
        ret = inner_sm2_public_encrypt(pub_key, plain_text, cipher_text);
        if (ret == PKE_COMPAT_ERRNO(ERROR_PKE_SM2_T_IS_ZERO)) {
            continue;
        }
        if (ret != CRYPTO_SUCCESS) {
            crypto_log_err("inner_sm2_public_encrypt failed\n");
            goto exit_deinit;
        } else {
            ret = CRYPTO_SUCCESS;
            goto exit_deinit;
        }
    }

    cipher_text->length = plain_text->length + CRYPTO_SM2_ADD_LENGTH_IN_BYTE;
exit_deinit:
    hal_pke_alg_ecc_deinit();
    return ret;
}
#endif