/**
  Copyright (c), 2001-2025, Shenshu Tech. Co., Ltd.
 */
#if defined(CONFIG_PKE_RSA_V21_SUPPORT)

#include "drv_pke_inner.h"
#include "crypto_drv_common.h"

#define RSA_PAD_XBC                0xBC
#define BOUND_VALUE_1              1
#define MAX_LOW_3BITS              7
#define MAX_LOW_8BITS              0xFF

#define WORD_INDEX_0               0
#define WORD_INDEX_1               1
#define WORD_INDEX_2               2
#define WORD_INDEX_3               3

#define SHIFT_4BITS                4
#define SHIFT_8BITS                8
#define SHIFT_16BITS               16
#define SHIFT_24BITS               24

#define RSA_PADLEN_2               2
#define RSA_PADLEN_8               8

#if defined(CONFIG_PKE_RSA_V21_SUPPORT)
#if defined(CONFIG_PKE_RSA_SIGN_SUPPORT) || defined(CONFIG_PKE_RSA_VERIFY_SUPPORT)
/* H = Hash( M' ) = Hash( Padding1 || mHash || salt ) */
static td_s32 inner_pkcs1_pss_hash(const rsa_pkcs1_hash_info *hash_info, const td_u8 *m_hash, td_u32 klen,
    const drv_pke_data *salt, drv_pke_data *hash)
{
    td_s32 ret = TD_FAILURE;
    td_u8 ps[RSA_PADLEN_8] = {0};
    drv_pke_data arr[3]; // The capacity of the array is 3.

    /* H = Hash(PS || MHash || SALT) */
    arr[0].data = ps; // 0 th element is ps
    arr[0].length = RSA_PADLEN_8; // 0 element is PS
    arr[1].data = (td_u8 *)m_hash; // 1 element is MHash
    arr[1].length = klen; // 1 element is MHash
    arr[2].data = (td_u8 *)salt->data; // 2 element is SALT
    arr[2].length = salt->length; // 2 element is SALT
    ret = hal_pke_alg_calc_hash(arr, sizeof(arr) / sizeof(arr[0]), hash_info->hash_type, hash);
    crypto_chk_return(ret != TD_SUCCESS, ret, "hal_pke_alg_calc_hash failed, ret is 0x%x\n", ret);

    return ret;
}
#endif

#if defined(CONFIG_PKE_RSA_SUPPORT)
static td_s32 inner_pkcs1_mgf(const rsa_pkcs1_hash_info *hash_info, const td_u8 *seed, const td_u32 seed_len,
    td_u8 *mask, const td_u32 mask_len)
{
    td_s32 ret = TD_FAILURE;
    td_u32 i = 0;
    td_u32 j = 0;
    td_u32 out_len = 0;
    td_u8 cnt[CRYPTO_WORD_WIDTH];
    td_u8 md[HASH_SIZE_SHA_MAX];
    drv_pke_data hash = {0};
    td_u8 seed_buf[DRV_PKE_LEN_4096];
    drv_pke_data arr[2];

    (void)memset_s(cnt, CRYPTO_WORD_WIDTH, 0x00, CRYPTO_WORD_WIDTH);
    (void)memset_s(md, HASH_SIZE_SHA_MAX, 0x00, HASH_SIZE_SHA_MAX);
    ret = memcpy_s(seed_buf, DRV_PKE_LEN_4096, seed, seed_len);
    crypto_chk_return(ret != EOK, ret, "memcpy_s failed, ret is 0x%x\n", ret);

    hash.length = HASH_SIZE_SHA_MAX;
    hash.data = md;

    arr[0].data = seed_buf;
    arr[0].length = seed_len;
    arr[1].data = cnt;
    arr[1].length = (uintptr_t)sizeof(cnt);

    for (i = 0; out_len < mask_len; i++) {
        cnt[WORD_INDEX_0] = (td_u8)((i >> SHIFT_24BITS) & MAX_LOW_8BITS);
        cnt[WORD_INDEX_1] = (td_u8)((i >> SHIFT_16BITS) & MAX_LOW_8BITS);
        cnt[WORD_INDEX_2] = (td_u8)((i >> SHIFT_8BITS)) & MAX_LOW_8BITS;
        cnt[WORD_INDEX_3] = (td_u8)(i & MAX_LOW_8BITS);

        /* H = Hash(seedbuf || cnt) */
        ret = hal_pke_alg_calc_hash(arr, sizeof(arr) / sizeof(arr[0]), hash_info->hash_type, &hash);
        crypto_chk_return(ret != TD_SUCCESS, ret, "hal_pke_alg_calc_hash failed, ret is 0x%x\n", ret);

        for (j = 0; (j < hash_info->hash_len) && (out_len < mask_len); j++) {
            mask[out_len++] ^= md[j];
        }
    }
    return TD_SUCCESS;
}
#endif

#if defined(CONFIG_PKE_RSA_SIGN_SUPPORT)
td_s32 pkcs1_pss_sign(const drv_pke_hash_type hash_type, const rsa_pkcs1_pack *pack)
{
    td_s32 ret = TD_FAILURE;
    rsa_pkcs1_hash_info hash_info = {0};
    td_u8 salt_data[HASH_SIZE_SHA_MAX];
    td_u32 salt_len = 0;
    td_u32 tmp_len = 0;
    td_u32 ms_bits = 0;
    td_u8 *masked_db = TD_NULL;
    td_u32 masked_db_len = pack->em_len - pack->hash_len - 1; /* 1 byte for bound. */
    drv_pke_data masked_seed;
    drv_pke_data salt = {0};

    ret = pkcs1_get_hash(hash_type, TD_NULL, &hash_info);
    crypto_chk_return(ret != TD_SUCCESS, ret, "pkcs1_get_hash failed, ret is 0x%x\n", ret);

    salt_len = hash_info.hash_len;

    /* em_bit is the number of bits of key n. */
    ms_bits = (pack->em_bit - BOUND_VALUE_1) & MAX_LOW_3BITS;

    ret = inner_get_random((td_u8 *)salt_data, salt_len);
    crypto_chk_return(ret != TD_SUCCESS, ret, "inner_get_random failed, ret is 0x%x\n", ret);

    /* EM = masked_db || masked_seed || 0xbc */
    masked_db = pack->em;
    masked_seed.data = pack->em + masked_db_len;
    masked_seed.length = pack->hash_len;
    salt.data = salt_data;
    salt.length = salt_len;
    ret = inner_pkcs1_pss_hash(&hash_info, pack->hash, pack->hash_len, &salt, &masked_seed);
    crypto_chk_return(ret != TD_SUCCESS, ret, "inner_pkcs1_pss_hash failed, ret is 0x%x\n", ret);

    /* DB = PS || 0x01 || salt */
    /* set PS, here tmp_len means the length of PS */
    tmp_len = pack->em_len - pack->hash_len - salt_len - RSA_PADLEN_2; /* padding2.length - 1 */
    (void)memset_s(masked_db, tmp_len, 0x00, tmp_len);

    /* set 0x01 after PS */
    masked_db[tmp_len++] = 0x01;

    /* set salt */
    ret = memcpy_s(masked_db + tmp_len, pack->em_len - tmp_len, salt_data, salt_len);
    crypto_chk_return(ret != EOK, ret, "memcpy_s failed, ret is 0x%x\n", ret);

    ret = inner_pkcs1_mgf(&hash_info, masked_seed.data, pack->hash_len, masked_db, masked_db_len);
    crypto_chk_return(ret != TD_SUCCESS, ret, "inner_pkcs1_mgf failed, ret is 0x%x\n", ret);

    pack->em[0] &= MAX_LOW_8BITS >> (CRYPTO_BITS_IN_BYTE - ms_bits);
    pack->em[pack->em_len - 1] = RSA_PAD_XBC; /* 1 byte for bound. */

    return ret;
}
#endif

#if defined(CONFIG_PKE_RSA_VERIFY_SUPPORT)
td_s32 pkcs1_pss_verify(const drv_pke_hash_type hash_type, const rsa_pkcs1_pack *pack)
{
    td_s32 ret = TD_FAILURE;
    rsa_pkcs1_hash_info hash_info = {0};
    td_u8 em[DRV_PKE_LEN_4096];
    td_u8 hash[HASH_SIZE_SHA_MAX];
    td_u32 salt_len = 0;
    td_u32 ms_bits = 0;
    td_u8 *masked_db = TD_NULL;
    td_u32 masked_db_len = pack->em_len - pack->hash_len - 1; /* 1 byte for bound. */
    td_u8 *masked_seed = TD_NULL;
    drv_pke_data hash_data = { .data = hash, .length = sizeof(hash) };
    drv_pke_data salt = {0};

    ret = pkcs1_get_hash(hash_type, TD_NULL, &hash_info);
    crypto_chk_return(ret != TD_SUCCESS, ret, "pkcs1_get_hash failed, ret is 0x%x\n", ret);

    if (pack->em[pack->em_len - 1] != RSA_PAD_XBC) {
        return PKE_COMPAT_ERRNO(ERROR_PKE_RSA_VERIFY_PSS_CHECK);
    }

    ms_bits = (pack->em_bit - BOUND_VALUE_1) & MAX_LOW_3BITS;
    if ((pack->em[0] & (MAX_LOW_8BITS << ms_bits)) != 0) {
        return PKE_COMPAT_ERRNO(ERROR_PKE_RSA_VERIFY_PSS_CHECK);
    }

    ret = memcpy_s(em, DRV_PKE_LEN_4096, pack->em, pack->em_len);
    crypto_chk_return(ret != EOK, ret, "memcpy_s failed, ret is 0x%x\n", ret);

    masked_db = em;
    masked_seed = em + masked_db_len;
    ret = inner_pkcs1_mgf(&hash_info, masked_seed, pack->hash_len, masked_db, masked_db_len);
    crypto_chk_return(ret != TD_SUCCESS, ret, "inner_pkcs1_mgf failed, ret is 0x%x\n", ret);

    em[0] &= MAX_LOW_8BITS >> (CRYPTO_BITS_IN_BYTE - ms_bits);

    while (masked_db < masked_seed - 1 && *masked_db == 0) {
        masked_db++;
    }

    if (*masked_db++ != 0x01) {
        crypto_log_err("pss padding check failed!\n");
        return PKE_COMPAT_ERRNO(ERROR_PKE_RSA_VERIFY_PSS_CHECK);
    }

    salt_len = masked_seed - masked_db;
    salt.length = salt_len;
    salt.data = masked_db;

    ret = inner_pkcs1_pss_hash(&hash_info, pack->hash, pack->hash_len, &salt, &hash_data);
    crypto_chk_return(ret != TD_SUCCESS, ret, "inner_pkcs1_pss_hash failed, ret is 0x%x\n", ret);

    if (memcmp(masked_seed, hash, hash_info.hash_len) != 0) {
        crypto_log_err("pss hash check failed!\n");
        return PKE_COMPAT_ERRNO(ERROR_PKE_RSA_VERIFY_PSS_CHECK);
    }

    return TD_SUCCESS;
}
#endif

#if defined(CONFIG_PKE_RSA_PUB_ENC_SUPPORT)
td_s32 pkcs1_oaep_encrypt(const drv_pke_hash_type hash_type, const rsa_pkcs1_pack *pack,
    const drv_pke_data *label)
{
    td_s32 ret = TD_FAILURE;
    rsa_pkcs1_hash_info hash_info = {0};
    td_u8 *p = TD_NULL;
    td_u8 *db = TD_NULL;
    td_u8 *seed = TD_NULL;
    td_u32 idx = 0;
    td_u32 hash_len = 0;
    td_u32 tmp_len = 0;

    crypto_param_check(pack == TD_NULL);
    crypto_param_check(pack->em == TD_NULL);
    crypto_param_check(pack->data == TD_NULL && pack->data_len != 0);

    ret = pkcs1_get_hash(hash_type, label, &hash_info);
    crypto_chk_return(ret != TD_SUCCESS, ret, "pkcs1_get_hash failed, ret is 0x%x\n", ret);
    hash_len = hash_info.hash_len;

    crypto_param_check(pack->klen < 2 * hash_len + 2); /* Prevent inversion. eg. RSA-1024-v21-sha512 */
    crypto_param_check(pack->data_len > pack->klen - 2 * hash_len - 2); /* 2 */

    p = pack->em;
    seed = p + 1;
    db = p + hash_len + 1;

    /* 1. set data[0] = 0 */
    p[idx++] = 0x00;

    /* 2. set data[1, hash_len + 1] = random */
    ret = inner_get_random(&p[idx], hash_len);
    crypto_chk_return(ret != TD_SUCCESS, ret, "inner_get_random failed, ret is 0x%x\n", ret);
    idx += hash_len;

    /* 3. set data[hash_len + 1, 2hash_len + 1] = lHash */
    (void)memcpy_s(p + idx, hash_len, hash_info.lhash_data, hash_len);
    idx += hash_len;

    /* 4. set PS with 0x00 */
    tmp_len = pack->klen - pack->data_len - 2 * hash_len - 2; /* 2 */
    (void)memset_s(p + idx, tmp_len, 0x00, tmp_len);
    idx += tmp_len;

    /* 5. set 0x01 after PS */
    p[idx++] = 0x01;

    /* 6. set M */
    if (pack->data_len != 0) {
        (void)memcpy_s(p + idx, pack->data_len, pack->data, pack->data_len);
    }
    /* 7. MGF: seed -> db */
    tmp_len = pack->klen - hash_len - 1;
    ret = inner_pkcs1_mgf(&hash_info, seed, hash_len, db, tmp_len);
    crypto_chk_return(ret != TD_SUCCESS, ret, "inner_pkcs1_mgf failed, ret is 0x%x\n", ret);

    /* 8. MGF: db -> seed */
    ret = inner_pkcs1_mgf(&hash_info, db, tmp_len, seed, hash_len);
    crypto_chk_return(ret != TD_SUCCESS, ret, "inner_pkcs1_mgf failed, ret is 0x%x\n", ret);

    return TD_SUCCESS;
}
#endif

#if defined(CONFIG_PKE_RSA_PRIV_DEC_SUPPORT)
td_s32 pkcs1_oaep_decrypt(const drv_pke_hash_type hash_type, rsa_pkcs1_pack *pack,
    const drv_pke_data *label)
{
    td_s32 ret = TD_FAILURE;
    rsa_pkcs1_hash_info hash_info = {0};
    td_u32 idx = 0;
    td_u32 hash_len = 0;
    td_u32 tmp_len = 0;
    td_u8 *p = TD_NULL;
    td_u8 *db = TD_NULL;
    td_u8 *seed = TD_NULL;

    ret = pkcs1_get_hash(hash_type, label, &hash_info);
    crypto_chk_return(ret != TD_SUCCESS, ret, "pkcs1_get_hash failed, ret is 0x%x\n", ret);

    hash_len = hash_info.hash_len;

    p = pack->em;
    seed = p + 1;
    db = p + hash_len + 1;

    /* 1. check data[0] = 0 */
    if (p[idx++] != 0x00) {
        crypto_log_err("oaep padding check failed!\n");
        return PKE_COMPAT_ERRNO(ERROR_PKE_RSA_CRYPTO_OAEP_CHECK);
    }

    /* 2. MGF: db -> seed */
    tmp_len = pack->klen - hash_len - 1;
    ret = inner_pkcs1_mgf(&hash_info, db, tmp_len, seed, hash_len);
    crypto_chk_return(ret != TD_SUCCESS, ret, "inner_pkcs1_mgf failed, ret is 0x%x\n", ret);

    /* 3. MGF: seed -> db */
    ret = inner_pkcs1_mgf(&hash_info, seed, hash_len, db, tmp_len);
    crypto_chk_return(ret != TD_SUCCESS, ret, "inner_pkcs1_mgf failed, ret is 0x%x\n", ret);

    /* 4. check data[hash + 1, 2hash + 1] */
    idx += hash_len;
    if (memcmp(p + idx, hash_info.lhash_data, hash_len) != 0) {
        crypto_log_err("oaep lhash check failed!\n");
        return PKE_COMPAT_ERRNO(ERROR_PKE_RSA_CRYPTO_OAEP_CHECK);
    }

    /* 5. remove PS */
    idx += hash_len;
    for (; idx < pack->klen - 1; idx++) {
        if (p[idx] != 0x00) {
            break;
        }
    }

    /* 6. check 0x01 */
    if (p[idx++] != 0x01) {
        crypto_log_err("oaep check failed!\n");
        return PKE_COMPAT_ERRNO(ERROR_PKE_RSA_CRYPTO_OAEP_CHECK);
    }

    /* 7. check data length */
    tmp_len = pack->klen - idx;
    if (tmp_len > pack->klen - 2 * hash_len - 2) { /* 2 */
        crypto_log_err("PS error.\n");
        return PKE_COMPAT_ERRNO(ERROR_PKE_RSA_CRYPTO_OAEP_CHECK);
    }

    if (tmp_len > pack->data_len) {
        crypto_log_err("Input buffer too small.\n");
        return PKE_COMPAT_ERRNO(ERROR_PKE_RSA_CRYPTO_OAEP_CHECK);
    }

    if (tmp_len != 0) {
        ret = memcpy_s(pack->data, pack->data_len, p + idx, tmp_len);
        crypto_chk_return(ret != EOK, ret, "memcpy_s failed, ret is 0x%x\n", ret);
    }
    pack->data_len = tmp_len; // record length of plain text for return

    return TD_SUCCESS;
}
#endif
#endif

#endif