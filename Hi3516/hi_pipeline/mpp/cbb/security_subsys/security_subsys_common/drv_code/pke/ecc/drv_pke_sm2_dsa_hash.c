/**
  Copyright (c), 2001-2025, Shenshu Tech. Co., Ltd.
*/
#if defined(CONFIG_PKE_ECC_SM2_DSA_HASH_SUPPORT)

#include "drv_pke_inner.h"

#include "crypto_drv_common.h"


#define ENTLA_LEN                   2
#define ZA_ARR_SIZE                 8
#define E_ARR_SIZE                  2

/* According to SM2 Elliptic Curve Public Key cryptographic algorithm.
 * Refer to "Patr1 Section 5.5 Other user information" to Compute Za.
 * Refer to "Part2 Section 6.1. Digital Signature Generation Algorithm Step A1~A2" to Compute the e = H(M')
 */
static td_s32 inner_sm2_dsa_hash(const drv_pke_data *sm2_id, const drv_pke_ecc_point *pub_key,
    const drv_pke_msg *msg, drv_pke_data *hash)
{
    volatile td_s32 ret = TD_FAILURE;
    td_u8 entla[ENTLA_LEN];
    drv_pke_data arr[ZA_ARR_SIZE] = {0};
    const drv_pke_ecc_curve *sm2 = TD_NULL;
    drv_pke_data hash_data = {
        .data = hash->data,
        .length = hash->length
    };

    sm2 = hal_pke_alg_ecc_get_curve();
    /* Za = Hash(ENTLa||IDa||a||b||Xg||Yg||Xa||Ya). */
    entla[0] = (td_u8)((sm2_id->length * CRYPTO_BITS_IN_BYTE) >> CRYPTO_BITS_IN_BYTE);
    entla[1] = (td_u8)((sm2_id->length * CRYPTO_BITS_IN_BYTE));

    arr[0] = (const drv_pke_data) { .data = entla, .length = ENTLA_LEN };
    arr[1] = (const drv_pke_data) { .data = sm2_id->data, .length = sm2_id->length };
    arr[2] = (const drv_pke_data) { .data = (td_u8 *)sm2->a, .length = SM2_LEN_IN_BYTES };
    arr[3] = (const drv_pke_data) { .data = (td_u8 *)sm2->b, .length = SM2_LEN_IN_BYTES };
    arr[4] = (const drv_pke_data) { .data = (td_u8 *)sm2->gx, .length = SM2_LEN_IN_BYTES };
    arr[5] = (const drv_pke_data) { .data = (td_u8 *)sm2->gy, .length = SM2_LEN_IN_BYTES };
    arr[6] = (const drv_pke_data) { .data = (td_u8 *)pub_key->x, .length = SM2_LEN_IN_BYTES };
    arr[7] = (const drv_pke_data) { .data = (td_u8 *)pub_key->y, .length = SM2_LEN_IN_BYTES };

    ret = hal_pke_alg_calc_hash(arr, ZA_ARR_SIZE, DRV_PKE_HASH_TYPE_SM3, &hash_data);
    crypto_chk_return(ret != CRYPTO_SUCCESS, ret, "hal_pke_alg_calc_hash failed\n");

    /* e = Hash(Za||M) */
    arr[0] = (const drv_pke_data) { .data = hash_data.data, .length = SM2_LEN_IN_BYTES };
    arr[1] = (const drv_pke_data) { .data = msg->data, .length = msg->length };
    ret = hal_pke_alg_calc_hash(arr, E_ARR_SIZE, DRV_PKE_HASH_TYPE_SM3, &hash_data);
    crypto_chk_return(ret != CRYPTO_SUCCESS, ret, "hal_pke_alg_calc_hash failed\n");

    return ret;
}

td_s32 drv_cipher_pke_sm2_dsa_hash(const drv_pke_data *sm2_id, const drv_pke_ecc_point *pub_key,
    const drv_pke_msg *msg, drv_pke_data *hash)
{
    unsigned int klen = DRV_PKE_LEN_256;
    volatile td_s32 ret = TD_FAILURE;
    crypto_drv_func_enter();

    /* param check. */
    pke_null_ptr_chk(sm2_id);
    pke_null_ptr_chk(sm2_id->data);
    pke_null_ptr_chk(pub_key);
    pke_null_ptr_chk(pub_key->x);
    pke_null_ptr_chk(pub_key->y);
    pke_null_ptr_chk(msg);
    pke_null_ptr_chk(msg->data);
    pke_null_ptr_chk(hash);
    pke_null_ptr_chk(hash->data);

    /* length check. */
    crypto_chk_return(pub_key->length != klen, PKE_COMPAT_ERRNO(ERROR_INVALID_PARAM),
        "pub_key->length is invalid\n");
    crypto_chk_return(hash->length != klen, PKE_COMPAT_ERRNO(ERROR_INVALID_PARAM),
        "hash->length is invalid\n");

    ret = hal_pke_alg_ecc_init(DRV_PKE_ECC_TYPE_SM2);
    crypto_chk_return(ret != CRYPTO_SUCCESS, ret, "hal_pke_alg_ecc_init failed\n");

    ret = inner_sm2_dsa_hash(sm2_id, pub_key, msg, hash);
    crypto_chk_goto(ret != CRYPTO_SUCCESS, exit_deinit, "inner_sm2_dsa_hash failed\n");

exit_deinit:
    hal_pke_alg_ecc_deinit();
    crypto_drv_func_exit();
    return ret;
}
#endif