/**
  Copyright (c), 2001-2025, Shenshu Tech. Co., Ltd.
 */

/* According to RFC2313. */

#if defined(CONFIG_PKE_RSA_V15_SUPPORT)
#include "drv_pke_inner.h"
#include "crypto_drv_common.h"

#define RSA_PADLEN_1                1
#define RSA_PADLEN_3                3
#define RSA_PADLEN_11               11

#if defined(CONFIG_PKE_RSA_SIGN_SUPPORT)
/* EM = 00 || 01 || PS || 00 || T */
td_s32 pkcs1_v15_sign(const drv_pke_hash_type hash_type, const rsa_pkcs1_pack *pack)
{
    td_s32 ret = TD_FAILURE;
    rsa_pkcs1_hash_info hash_info = {0};
    td_u8 *p = TD_NULL;
    td_u32 idx = 0;

    ret = pkcs1_get_hash(hash_type, TD_NULL, &hash_info);
    crypto_chk_return(ret != TD_SUCCESS, ret, "pkcs1_get_hash failed, ret is 0x%x\n", ret);

    p = pack->em;
    p[idx++] = 0x00;
    p[idx++] = 0x01;

    /* PS */
    while (idx < (pack->em_len - hash_info.hash_len - hash_info.asn1_len - 1)) {
        p[idx++] = CRYPTO_BYTE_MAX;
    }

    p[idx++] = 0x00;

    /* T */
    ret = memcpy_s(&p[idx], pack->em_len - idx, hash_info.asn1_data, hash_info.asn1_len);
    crypto_chk_return(ret != TD_SUCCESS, ret, "memcpy_s failed, ret is 0x%x\n", ret);
    idx += hash_info.asn1_len;
    ret = memcpy_s(&p[idx], pack->em_len - idx, pack->hash, hash_info.hash_len);
    crypto_chk_return(ret != TD_SUCCESS, ret, "memcpy_s failed, ret is 0x%x\n", ret);

    return ret;
}
#endif

#if defined(CONFIG_PKE_RSA_VERIFY_SUPPORT)
/* check EM = 00 || 01 || PS || 00 || T */
td_s32 pkcs1_v15_verify(const drv_pke_hash_type hash_type, const rsa_pkcs1_pack *pack)
{
    td_s32 ret = TD_FAILURE;
    rsa_pkcs1_hash_info hash_info = {0};
    td_u8 *p = TD_NULL;
    td_u32 idx = 0;

    ret = pkcs1_get_hash(hash_type, TD_NULL, &hash_info);
    crypto_chk_return(ret != TD_SUCCESS, ret, "pkcs1_get_hash failed, ret is 0x%x\n", ret);

    p = pack->em;
    if (p[idx++] != 0x00) {
        return PKE_COMPAT_ERRNO(ERROR_PKE_RSA_VERIFY_V15_CHECK);
    }
    if (p[idx++] != 0x01) {
        return PKE_COMPAT_ERRNO(ERROR_PKE_RSA_VERIFY_V15_CHECK);
    }

    /* PS */
    while (idx < (pack->em_len - hash_info.hash_len - hash_info.asn1_len - 1)) {
        if (p[idx++] != CRYPTO_BYTE_MAX) {
            return PKE_COMPAT_ERRNO(ERROR_PKE_RSA_VERIFY_V15_CHECK);
        }
    }
    if (p[idx++] != 0x00) {
        return PKE_COMPAT_ERRNO(ERROR_PKE_RSA_VERIFY_V15_CHECK);
    }

    /* T */
    if (memcmp(&p[idx], hash_info.asn1_data, hash_info.asn1_len) != 0) {
        crypto_log_err("check v15 asn1 failed!\n");
        return PKE_COMPAT_ERRNO(ERROR_PKE_RSA_VERIFY_V15_CHECK);
    }

    idx += hash_info.asn1_len;
    if (memcmp(&p[idx], pack->hash, hash_info.hash_len) != 0) {
        crypto_log_err("check v15 hash failed!\n");
        return PKE_COMPAT_ERRNO(ERROR_PKE_RSA_VERIFY_V15_CHECK);
    }

    return ret;
}
#endif

#if defined(CONFIG_PKE_RSA_PUB_ENC_SUPPORT)
/* EM = 00 || 02 || PS || 00 || M */
td_s32 pkcs1_v15_encrypt(const rsa_pkcs1_pack *pack)
{
    td_s32 ret = TD_FAILURE;
    td_u8 *p = TD_NULL;
    td_u32 idx = 0;
    td_u32 ps_len = pack->em_len - pack->data_len - RSA_PADLEN_3;

    p = pack->em;
    p[idx++] = 0x00;
    p[idx++] = 0x02; // 0x02 see comment above func

    /* PS */
    ret = inner_get_random_with_nonzero_octets(&p[idx], ps_len);
    if (ret != TD_SUCCESS) {
        crypto_log_err("get random ps failed!\n");
        return ret;
    }
    idx += ps_len;

    p[idx++] = 0x00;
    /* M */
    if (pack->data_len != 0) {
        ret = memcpy_s(&p[idx], pack->em_len - idx, pack->data, pack->data_len);
        crypto_chk_return(ret != TD_SUCCESS, ret, "memcpy_s failed, ret is 0x%x\n", ret);
    }

    return ret;
}
#endif

#if defined(CONFIG_PKE_RSA_PRIV_DEC_SUPPORT)
/* check EM = 00 || 02 || PS || 00 || M */
td_s32 pkcs1_v15_decrypt(rsa_pkcs1_pack *pack)
{
    td_s32 ret = TD_FAILURE;
    td_u8 *p = TD_NULL;
    td_u32 idx = 0;
    td_u32 len = 0;

    p = pack->em;
    if (p[idx++] != 0x00) {
        crypto_log_err("padding check failed!\n");
        return PKE_COMPAT_ERRNO(ERROR_PKE_RSA_CRYPTO_V15_CHECK);
    }

    if (p[idx++] != 0x02) { // 0x02 see comment above func
        crypto_log_err("padding check failed!\n");
        return PKE_COMPAT_ERRNO(ERROR_PKE_RSA_CRYPTO_V15_CHECK);
    }

    /* PS */
    while ((idx < (pack->em_len - RSA_PADLEN_1)) && p[idx] != 0x00) {
        idx++;
    }

    if (p[idx++] != 0x00) {
        crypto_log_err("padding check failed!\n");
        return PKE_COMPAT_ERRNO(ERROR_PKE_RSA_CRYPTO_V15_CHECK);
    }

    len = pack->em_len - idx;
    if (len > pack->em_len - RSA_PADLEN_11) {
        crypto_log_err("padding check failed!\n");
        return PKE_COMPAT_ERRNO(ERROR_PKE_RSA_CRYPTO_V15_CHECK);
    }

    /* M */
    if (len != 0) {
        ret = memcpy_s(pack->data, pack->data_len, &p[idx], len);
        crypto_chk_return(ret != TD_SUCCESS, ret, "memcpy_s failed, ret is 0x%x\n", ret);
    }
    pack->data_len = len; // record length of plain text for return

    return TD_SUCCESS;
}
#endif
#endif