/**
  Copyright (c), 2001-2025, Shenshu Tech. Co., Ltd.
 */

#include "drv_pke_inner.h"

#include "crypto_drv_common.h"

#if defined(CONFIG_PKE_RSA_SIGN_SUPPORT)
static td_s32 rsa_sign(
    drv_pke_rsa_scheme scheme,
    drv_pke_hash_type hash_type,
    td_u8 *em,
    td_u32 klen,
    td_u32 em_bit,
    const drv_pke_data *msg)
{
    td_s32 ret = TD_FAILURE;
    rsa_pkcs1_pack sg_pack = {
        .klen = klen,
        .em_bit = em_bit,
        .em = em,
        .em_len = klen,
        .hash = msg->data,
        .hash_len = msg->length,
        .data = TD_NULL,
        .data_len = 0
    };

    crypto_unused(sg_pack);
    crypto_unused(hash_type);

    switch (scheme) {
#if defined(CONFIG_PKE_RSA_V15_SUPPORT)
        case DRV_PKE_RSA_SCHEME_PKCS1_V15: {
            ret = pkcs1_v15_sign(hash_type, &sg_pack);
            crypto_chk_return(ret != TD_SUCCESS, ret, "pkcs1_v15_sign failed, ret is 0x%x\n", ret);
            break;
        }
#endif
#if defined(CONFIG_PKE_RSA_V21_SUPPORT)
        case DRV_PKE_RSA_SCHEME_PKCS1_V21: {
            ret = pkcs1_pss_sign(hash_type, &sg_pack);
            crypto_chk_return(ret != TD_SUCCESS, ret, "pkcs1_pss_sign failed, ret is 0x%x\n", ret);
            break;
        }
#endif
        default:
            crypto_log_err("Invalid rsa sign padding type.\n");
            return PKE_COMPAT_ERRNO(ERROR_INVALID_PARAM);
    }
    return ret;
}

static td_s32 inner_pke_rsa_sign(
    const drv_pke_rsa_priv_key *priv_key,
    drv_pke_rsa_scheme scheme,
    drv_pke_hash_type hash_type,
    const drv_pke_data *input_hash,
    drv_pke_data *sign)
{
    td_s32 ret = TD_FAILURE;
    td_u8 *em;
    td_u32 klen = priv_key->n_len;
    td_u32 em_bit = 0;
    drv_pke_data n_data = { .data = priv_key->n, .length = klen };
    drv_pke_data d_data = { . data = priv_key->d, .length = klen };
    drv_pke_data em_data = { .length = klen };
    em = crypto_malloc(DRV_PKE_LEN_4096);
    if (em == TD_NULL) {
        crypto_log_err("%s:%d Error! Malloc memory failed!\n", __FUNCTION__, __LINE__);
        return PKE_COMPAT_ERRNO(ERROR_MALLOC);
    }
    (void)memset_s(em, DRV_PKE_LEN_4096, 0x00, DRV_PKE_LEN_4096);

    em_bit = rsa_get_bit_num((const td_u8 *)priv_key->n, priv_key->n_len);
    ret = rsa_sign(scheme, hash_type, em, klen, em_bit, input_hash);
    crypto_chk_goto_with_ret(ret, ret != TD_SUCCESS, exit__, ret, "rsa_sign failed, ret is 0x%x\n", ret);

    em_data.data = em;
    ret = hal_pke_alg_rsa_exp_mod(&n_data, &d_data, &em_data, sign);
    crypto_chk_goto_with_ret(ret, ret != TD_SUCCESS, exit__, ret, "hal_pke_alg_rsa_exp_mod failed, ret is 0x%x\n", ret);

    if (memcmp(input_hash->data, sign->data, input_hash->length) == 0) {
        ret = PKE_COMPAT_ERRNO(ERROR_PKE_RSA_SAME_DATA);
    }

exit__:
    if (em != TD_NULL) {
        (void)memset_s(em, DRV_PKE_LEN_4096, 0, DRV_PKE_LEN_4096);
        crypto_free(em);
    }
    return ret;
}

td_s32 drv_cipher_pke_rsa_sign(
    const drv_pke_rsa_priv_key *priv_key,
    const drv_pke_rsa_scheme scheme,
    const drv_pke_hash_type hash_type,
    const drv_pke_data *input_hash,
    drv_pke_data *sign)
{
    td_s32 ret = TD_FAILURE;
    td_u32 klen = 0;
    td_u32 hash_len = 0;
    /* check ptr. */
    pke_null_ptr_chk(priv_key);
    pke_null_ptr_chk(priv_key->n);
    pke_null_ptr_chk(priv_key->d);
    pke_null_ptr_chk(input_hash);
    pke_null_ptr_chk(input_hash->data);
    pke_null_ptr_chk(sign);
    pke_null_ptr_chk(sign->data);

    /* check enum. */
    crypto_chk_return(scheme >= DRV_PKE_RSA_SCHEME_MAX, PKE_COMPAT_ERRNO(ERROR_INVALID_PARAM),
        "scheme is Invalid\n");
    crypto_chk_return(hash_type < DRV_PKE_HASH_TYPE_SHA1 || hash_type > DRV_PKE_HASH_TYPE_SHA512,
        PKE_COMPAT_ERRNO(ERROR_INVALID_PARAM), "hash_type is Invalid\n");

    /* check length. */
    klen = priv_key->n_len;
    hash_len = drv_get_hash_len(hash_type);
    crypto_chk_return(input_hash->length != hash_len, PKE_COMPAT_ERRNO(ERROR_INVALID_PARAM),
        "input_hash->length is Invalid\n");

    crypto_chk_return(crypto_rsa_klen_support(klen) != TD_SUCCESS, PKE_COMPAT_ERRNO(ERROR_INVALID_PARAM),
        "klen is Invalid\n");
    if ((priv_key->d_len != klen)) {
        ret = PKE_COMPAT_ERRNO(ERROR_INVALID_PARAM);
        crypto_log_err("d_len is Invalid!\n");
        return ret;
    }
    crypto_chk_return(sign->length < klen, PKE_COMPAT_ERRNO(ERROR_INVALID_PARAM), "sign_len is Invalid\n");
    crypto_chk_return(crypto_rsa_support(klen, scheme) == TD_FALSE, PKE_COMPAT_ERRNO(ERROR_UNSUPPORT),
        "alg is unsupport\n");

    ret = inner_pke_rsa_sign(priv_key, scheme, hash_type, input_hash, sign);
    if (ret == TD_SUCCESS) {
        sign->length = klen;
    }
    return ret;
}
#else
td_s32 drv_cipher_pke_rsa_sign(
    const drv_pke_rsa_priv_key *priv_key,
    const drv_pke_rsa_scheme scheme,
    const drv_pke_hash_type hash_type,
    const drv_pke_data *input_hash,
    drv_pke_data *sign)
{
    crypto_unused(priv_key);
    crypto_unused(scheme);
    crypto_unused(hash_type);
    crypto_unused(input_hash);
    crypto_unused(sign);

    return PKE_COMPAT_ERRNO(ERROR_UNSUPPORT);
}
#endif