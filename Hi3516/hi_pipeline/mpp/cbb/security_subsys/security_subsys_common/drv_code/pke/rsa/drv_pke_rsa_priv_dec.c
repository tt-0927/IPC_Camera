/**
  Copyright (c), 2001-2025, Shenshu Tech. Co., Ltd.
 */

#include "drv_pke_inner.h"

#include "crypto_drv_common.h"
#define RSA_PADLEN_11              11
#if defined(CONFIG_PKE_RSA_PRIV_DEC_SUPPORT)

static td_s32 rsa_decrypt(const drv_pke_rsa_scheme scheme, const drv_pke_hash_type hash_type, td_u8 *em,
    const td_u32 klen, const drv_pke_data *label, drv_pke_data *msg)
{
    td_s32 ret = TD_FAILURE;
    rsa_pkcs1_pack de_pack = {
        .klen = klen,
        .em_bit = 0,
        .em = em,
        .em_len = klen,
        .hash = TD_NULL,
        .hash_len = 0,
        .data = msg->data,
        .data_len = msg->length
    };

    crypto_unused(de_pack);
    crypto_unused(hash_type);
    crypto_unused(label);

    switch (scheme) {
#if defined(CONFIG_PKE_RSA_V15_SUPPORT)
        case DRV_PKE_RSA_SCHEME_PKCS1_V15: {
            ret = pkcs1_v15_decrypt(&de_pack);
            crypto_chk_return(ret != TD_SUCCESS, ret, "pkcs1_v15_decrypt failed, ret is 0x%x\n", ret);
            msg->length = de_pack.data_len; // record length of plain text for return
            break;
        }
#endif
#if defined(CONFIG_PKE_RSA_V21_SUPPORT)
        case DRV_PKE_RSA_SCHEME_PKCS1_V21: {
            ret = pkcs1_oaep_decrypt(hash_type, &de_pack, label);
            crypto_chk_return(ret != TD_SUCCESS, ret, "pkcs1_oaep_decrypt failed, ret is 0x%x\n", ret);
            msg->length = de_pack.data_len; // record length of plain text for return
            break;
        }
#endif
        default:
            crypto_log_err("Invalid rsa decrypt padding type.\n");
            return PKE_COMPAT_ERRNO(ERROR_INVALID_PARAM);
    }
    return ret;
}

static td_s32 pke_rsa_decrypt(
    drv_pke_rsa_scheme scheme,
    drv_pke_hash_type hash_type,
    const drv_pke_rsa_priv_key *priv_key,
    const drv_pke_data *input,
    const drv_pke_data *label,
    drv_pke_data *output)
{
    td_s32 ret = TD_FAILURE;
    td_u8 em[DRV_PKE_LEN_4096];
    td_u32 klen = priv_key->n_len;
    drv_pke_data n_data = { .data = priv_key->n, .length = klen };
    drv_pke_data d_data = { .data = priv_key->d, .length = klen };
    drv_pke_data em_data = { .data = em, .length = klen };

    (void)memset_s(em, sizeof(em), 0x00, sizeof(em));

    ret = hal_pke_alg_rsa_exp_mod(&n_data, &d_data, input, &em_data);
    crypto_chk_return(ret != TD_SUCCESS, ret, "hal_pke_alg_rsa_exp_mod failed, ret is 0x%x\n", ret);
    if (memcmp(input->data, em, klen) == 0) {
        return PKE_COMPAT_ERRNO(ERROR_PKE_RSA_SAME_DATA);
    }

    ret = rsa_decrypt(scheme, hash_type, em, klen, label, output);
    crypto_chk_return(ret != TD_SUCCESS, ret, "rsa_decrypt failed, ret is 0x%x\n", ret);
    
    return ret;
}

td_s32 drv_cipher_pke_rsa_private_decrypt(
    const drv_pke_rsa_scheme scheme,
    const drv_pke_hash_type hash_type,
    const drv_pke_rsa_priv_key *priv_key,
    const drv_pke_data *input,
    const drv_pke_data *label,
    drv_pke_data *output)
{
    td_s32 ret = TD_FAILURE;

    td_u32 klen = 0;
    td_u32 hash_len = 0;
    td_u32 output_len = 0;
    /* check ptr. */
    pke_null_ptr_chk(priv_key);
    pke_null_ptr_chk(priv_key->n);
    pke_null_ptr_chk(priv_key->d);
    pke_null_ptr_chk(input);
    pke_null_ptr_chk(input->data);
    pke_null_ptr_chk(output);
    pke_null_ptr_chk(output->data);
    if (label != TD_NULL) {
        pke_null_ptr_chk(label->data);
    }

    /* check enum. */
    crypto_chk_return(scheme >= DRV_PKE_RSA_SCHEME_MAX, PKE_COMPAT_ERRNO(ERROR_INVALID_PARAM),
        "scheme is Invalid\n");
    crypto_chk_return(hash_type < DRV_PKE_HASH_TYPE_SHA1 || hash_type > DRV_PKE_HASH_TYPE_SHA512,
        PKE_COMPAT_ERRNO(ERROR_INVALID_PARAM), "hash_type is Invalid\n");

    /* check length. */
    klen = priv_key->n_len;
    output_len = output->length;
    hash_len = drv_get_hash_len(hash_type);
    crypto_chk_return(hash_len == 0, PKE_COMPAT_ERRNO(ERROR_INVALID_PARAM), "hash_type is Invalid\n");

    crypto_chk_return(crypto_rsa_klen_support(klen) != TD_SUCCESS, PKE_COMPAT_ERRNO(ERROR_UNSUPPORT),
        "alg is unsupport\n");
    crypto_chk_return(priv_key->d_len != klen, PKE_COMPAT_ERRNO(ERROR_INVALID_PARAM), "d_len is Invalid\n");
    crypto_chk_return(input->length != klen, PKE_COMPAT_ERRNO(ERROR_INVALID_PARAM), "input_len is Invalid\n");

    /*
     * For V15, the plain_text's max length is klen - 11.
     * For OAEP, the plain_text's max length is klen - 2 * hash_len - 2.
     */
    if (scheme == DRV_PKE_RSA_SCHEME_PKCS1_V15) {
        crypto_chk_return(output_len < klen - RSA_PADLEN_11, PKE_COMPAT_ERRNO(ERROR_INVALID_PARAM),
            "output_len is not enough for V15\n");
    } else {
        crypto_chk_return(output_len < klen - 2 * hash_len - 2,  // 2: refer to comment.
            PKE_COMPAT_ERRNO(ERROR_INVALID_PARAM), "output_len is not enough for V21\n");
    }
    crypto_chk_return(crypto_rsa_support(klen, scheme) == TD_FALSE, PKE_COMPAT_ERRNO(ERROR_UNSUPPORT),
        "alg is unsupport\n");

    ret = pke_rsa_decrypt(scheme, hash_type, priv_key, input, label, output);
    return ret;
}
#else
td_s32 drv_cipher_pke_rsa_private_decrypt(
    const drv_pke_rsa_scheme scheme,
    const drv_pke_hash_type hash_type,
    const drv_pke_rsa_priv_key *priv_key,
    const drv_pke_data *input,
    const drv_pke_data *label,
    drv_pke_data *output)
{
    crypto_unused(scheme);
    crypto_unused(hash_type);
    crypto_unused(priv_key);
    crypto_unused(input);
    crypto_unused(label);
    crypto_unused(output);

    return PKE_COMPAT_ERRNO(ERROR_UNSUPPORT);
}
#endif