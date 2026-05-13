/**
  Copyright (c), 2001-2025, Shenshu Tech. Co., Ltd.
 */
#if defined(CONFIG_PKE_ECC_SM2_PRIV_DEC_SUPPORT)

#include "drv_pke.h"

#include "drv_pke_inner.h"
#include "hal_pke_alg.h"

#include "crypto_drv_common.h"

#define SM2_KEY_SIZE            32
#define SM3_HASH_LEN            32
#define SM2_PC_UNCOMPRESS       0x04
#define MAX_TRY_TIMES   100
/* According to SM2 Elliptic Curve Public Key cryptographic algorithm Part 4 public key encryption algorithm
 * "Section 7.1. Decryption algorithm"
 */
static td_s32 inner_sm2_private_decrypt(const drv_pke_data *priv_key, const drv_pke_data *cipher_text,
    drv_pke_data *plain_text)
{
    int ret = CRYPTO_FAILURE;
    unsigned int i;
    td_bool is_on_curve = TD_FALSE;
    unsigned int idx = 0;
    unsigned char u_buf[SM2_KEY_SIZE];
    unsigned char x2[SM2_KEY_SIZE];
    unsigned char y2[SM2_KEY_SIZE];
    drv_pke_ecc_point c1_point = {
        .length = SM2_KEY_SIZE
    };
    drv_pke_data u_data = {
        .data = u_buf,
        .length = SM2_KEY_SIZE
    };
    drv_pke_ecc_point dc1_point = {
        .x = x2,
        .y = y2,
        .length = SM2_KEY_SIZE
    };
    drv_pke_data arr[3];    /* 3: Compute u = Hash(x2 || M' || y2). */
    unsigned char *t = NULL;
    unsigned char *c2 = NULL;
    unsigned char *c3 = NULL;
    unsigned int plain_text_len = cipher_text->length - 1 - SM2_KEY_SIZE * 2 - SM3_HASH_LEN;
    /* cipher_text = C1 || C3 || C2 */
    /* According Part1 4.2.8, using uncompress representational form. */
    if (cipher_text->data[idx] != SM2_PC_UNCOMPRESS) {
        crypto_log_err("invalid uncompress representational form!\n");
        return CRYPTO_FAILURE;
    }
    idx++;
    c1_point.x = &cipher_text->data[idx];
    c1_point.y = &cipher_text->data[idx + SM2_KEY_SIZE];
    idx += SM2_KEY_SIZE * 2;    // 2: for x and y.
    c3 = &cipher_text->data[idx];
    idx += SM3_HASH_LEN;
    c2 = &cipher_text->data[idx];
    idx += plain_text_len;

#if defined(CONFIG_PKE_SM2_TRACE_ENABLE)
    crypto_dump_data("c2", c2, plain_text_len);
    crypto_dump_data("cipher_text", cipher_text->data, cipher_text->length);
#endif

    t = plain_text->data;

    /* Step B1. Get C1 from C and check if C1 is on the curve. */
    ret = hal_pke_alg_ecc_check_dot_on_curve(&c1_point, &is_on_curve);
    crypto_chk_return(ret != CRYPTO_SUCCESS, ret, "hal_pke_alg_ecc_check_dot_on_curve failed\n");
    crypto_chk_return(is_on_curve != TD_TRUE, CRYPTO_FAILURE, "C1 is not on the curve!\n");

    /* Step B2. Compute S = h * C1, cause h is 1, so we skip this step. */
    /* Step B3. Compute dC1(x2, y2) = dB * C1. */
#if defined(CONFIG_PKE_SM2_TRACE_ENABLE)
    crypto_print("========== Start Compute dC1(x2, y2) = dB * C1. ==========\n");
    crypto_dump_data("dB", priv_key->data, priv_key->length);
    crypto_dump_data("C1(x)", c1_point.x, c1_point.length);
    crypto_dump_data("C1(y)", c1_point.y, c1_point.length);
#endif
    ret = hal_pke_alg_ecc_point_mul(priv_key, &c1_point, &dc1_point);
    crypto_chk_return(ret != CRYPTO_SUCCESS, ret, "hal_pke_alg_ecc_point_mul failed\n");
#if defined(CONFIG_PKE_SM2_TRACE_ENABLE)
    crypto_dump_data("dC1(x1)", dc1_point.x, dc1_point.length);
    crypto_dump_data("dC1(y1)", dc1_point.y, dc1_point.length);
    crypto_print("========== End Compute dC1(x2, y2) = dB * C1. ==========\n");
#endif
    /*
     * Step B4. Compute t = kdf(x2 || y2, klen), if t = 0, return failure.
     * Here klen is plain_text_len.
     */
#if defined(CONFIG_PKE_SM2_TRACE_ENABLE)
    crypto_print("========== Start Compute t = kdf(x2 || y2, klen) ==========\n");
    crypto_dump_data("x2", dc1_point.x, dc1_point.length);
    crypto_dump_data("y2", dc1_point.y, dc1_point.length);
#endif
    ret = inner_sm2_kdf(&dc1_point, t, plain_text_len);
    crypto_chk_return(ret != CRYPTO_SUCCESS, ret, "inner_sm2_kdf failed\n");
#if defined(CONFIG_PKE_SM2_TRACE_ENABLE)
    crypto_dump_data("t", t, plain_text_len);
    crypto_print("========== End Compute t = kdf(x2 || y2, klen) ==========\n");
#endif
    /* Step B5. Get C2 from C and Compute M' = C2 ^ t. */
#if defined(CONFIG_PKE_SM2_TRACE_ENABLE)
    crypto_print("========== Start Compute Compute M' = C2 ^ t. ==========\n");
    crypto_dump_data("C2", c2, plain_text_len);
    crypto_dump_data("t", t, plain_text_len);
#endif
    for (i = 0; i < plain_text_len; i++) {
        t[i] = t[i] ^ c2[i];
    }
#if defined(CONFIG_PKE_SM2_TRACE_ENABLE)
    crypto_dump_data("M'", t, plain_text_len);
    crypto_print("========== End Compute Compute M' = C2 ^ t. ==========\n");
#endif
    /* Step B6. Compute u = Hash(x2 || M' || y2), get C3 from C. If u != C3, return failure. */
#if defined(CONFIG_PKE_SM2_TRACE_ENABLE)
    crypto_print("========== Start Compute Compute u = Hash(x2 || M' || y2). ==========\n");
    crypto_dump_data("x2", x2, dc1_point.length);
    crypto_dump_data("M'", t, plain_text_len);
    crypto_dump_data("y2", y2, dc1_point.length);
#endif
    arr[0].data = x2;
    arr[0].length = SM2_KEY_SIZE;
    arr[1].data = t;
    arr[1].length = plain_text_len;
    arr[2].data = y2;   // 2: position to store y2_data
    arr[2].length = SM2_KEY_SIZE;   // 2: position to store y2_length
    ret = hal_pke_alg_calc_hash(arr, crypto_array_size(arr), DRV_PKE_HASH_TYPE_SM3, &u_data);
    crypto_chk_return(ret != CRYPTO_SUCCESS, ret, "hal_pke_alg_calc_hash failed\n");
#if defined(CONFIG_PKE_SM2_TRACE_ENABLE)
    crypto_dump_data("u", u_buf, SM3_HASH_LEN);
    crypto_print("========== End Compute Compute u = Hash(x2 || M' || y2) ==========\n");
#endif
    if (memcmp(u_buf, c3, SM3_HASH_LEN) != 0) {
        crypto_log_err("Invalid u!\n");
        return CRYPTO_FAILURE;
    }

    /* Step B7. Output plain text M'. */
    return ret;
}

td_s32 drv_cipher_pke_sm2_private_decrypt(const drv_pke_data *priv_key, const drv_pke_data *cipher_text,
    drv_pke_data *plain_text)
{
    unsigned int klen = DRV_PKE_LEN_256;
    int ret;
    unsigned int i;

    /* param check. */
    pke_null_ptr_chk(priv_key);
    pke_null_ptr_chk(priv_key->data);
    pke_null_ptr_chk(cipher_text);
    pke_null_ptr_chk(cipher_text->data);
    pke_null_ptr_chk(plain_text);
    pke_null_ptr_chk(plain_text->data);

    /* length check. */
    crypto_chk_return(priv_key->length != klen, PKE_COMPAT_ERRNO(ERROR_INVALID_PARAM),
        "priv_key->length is invalid\n");
    crypto_chk_return(cipher_text->length <= CRYPTO_SM2_ADD_LENGTH_IN_BYTE, PKE_COMPAT_ERRNO(ERROR_INVALID_PARAM),
        "cipher_text->length is not enough\n");
    /* For SM2 Crypto, the cipher_text is 97 longer than plain_text. */
    crypto_chk_return(plain_text->length < (cipher_text->length - CRYPTO_SM2_ADD_LENGTH_IN_BYTE),
        PKE_COMPAT_ERRNO(ERROR_INVALID_PARAM), "plain_text->length is not enough\n");

    ret = hal_pke_alg_ecc_init(DRV_PKE_ECC_TYPE_SM2);
    crypto_chk_return(ret != CRYPTO_SUCCESS, ret, "hal_pke_alg_ecc_init failed\n");

    for (i = 0; i < MAX_TRY_TIMES; i++) {
        ret = inner_sm2_private_decrypt(priv_key, cipher_text, plain_text);
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

    plain_text->length = cipher_text->length - CRYPTO_SM2_ADD_LENGTH_IN_BYTE;
exit_deinit:
    hal_pke_alg_ecc_deinit();
    return ret;
}
#endif