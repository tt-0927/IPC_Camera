/**
  Copyright (c), 2001-2025, Shenshu Tech. Co., Ltd.
 */
#if defined(CONFIG_HASH_PBKDF2_SOFT_SUPPORT)
#include "drv_hash.h"

#include "hal_hash.h"

#include "crypto_drv_common.h"

#define HASH_COMPAT_ERRNO(err_code)     DRV_COMPAT_ERRNO(ERROR_MODULE_HASH, err_code)

static td_s32 priv_hmac(const crypto_kdf_pbkdf2_param *param, td_u8 *data, td_u32 data_len,
    td_u8 *hmac, td_u32 *out_hmac_len)
{
    td_s32 ret;
    td_handle drv_hash_handle = 0xffffffff;
    crypto_hash_attr hash_attr = {0};
    crypto_buf_attr src_buf = {0};
    td_u8 *key = param->password;
    td_u32 key_len = param->plen;
    crypto_hash_type hash_type = param->hash_type;

    src_buf.virt_addr = data;
    hash_attr.hash_type = hash_type;
    hash_attr.key = key;
    hash_attr.key_len = key_len;
    ret = drv_cipher_hash_start(&drv_hash_handle, &hash_attr);
    if (ret != TD_SUCCESS) {
        crypto_log_err("drv_cipher_hash_start failed, ret is 0x%x\n", ret);
        return ret;
    }

    ret = drv_cipher_hash_update(drv_hash_handle, &src_buf, data_len);
    if (ret != TD_SUCCESS) {
        crypto_log_err("drv_cipher_hash_update failed, ret is 0x%x\n", ret);
        (td_void)drv_cipher_hash_finish(drv_hash_handle, hmac, out_hmac_len);
        return ret;
    }

    ret = drv_cipher_hash_finish(drv_hash_handle, hmac, out_hmac_len);
    if (ret != TD_SUCCESS) {
        crypto_log_err("drv_cipher_hash_finish failed, ret is 0x%x\n", ret);
    }
    return ret;
}

static td_void priv_pbkdf2_parse_param(const crypto_kdf_pbkdf2_param *param, td_u32 out_len,
    td_u32 *result_size, td_u32 *times)
{
    td_u32 remain_size = 0;
    *result_size = crypto_hash_get_result_size(param->hash_type) / CRYPTO_BITS_IN_BYTE;

    *times = out_len / (*result_size);
    remain_size = out_len - (*result_size) * (*times);

    if (remain_size != 0) {
        (*times)++;
    }
}

td_s32 inner_pbkdf2_param_chk(const crypto_kdf_pbkdf2_param *param, td_u8 *out, td_u32 out_len)
{
    crypto_hash_type type;
    td_u32 block_size = 0;

    crypto_chk_return(param == TD_NULL, HASH_COMPAT_ERRNO(ERROR_PARAM_IS_NULL), "param is NULL\n");
    crypto_chk_return(out == TD_NULL, HASH_COMPAT_ERRNO(ERROR_PARAM_IS_NULL), "param is NULL\n");

    if (param->plen != 0) {
        crypto_chk_return(param->password == TD_NULL, HASH_COMPAT_ERRNO(ERROR_PARAM_IS_NULL), "password is NULL\n");
    }
    if (param->slen != 0) {
        crypto_chk_return(param->salt == TD_NULL, HASH_COMPAT_ERRNO(ERROR_PARAM_IS_NULL), "salt is NULL\n");
    }
    crypto_chk_return(param->count == 0, HASH_COMPAT_ERRNO(ERROR_INVALID_PARAM), "count is invalid\n");

    crypto_chk_return(out_len == 0 || out_len >= CRYPTO_PBKDF2_OUT_MAX_LENGTH,
        HASH_COMPAT_ERRNO(ERROR_INVALID_PARAM), "out_len is invalid\n");
    crypto_chk_return(param->slen > CRYPTO_PBKDF2_SALT_MAX_LENGTH,
        HASH_COMPAT_ERRNO(ERROR_INVALID_PARAM), "slen is invalid!\n");

    type = param->hash_type;
    crypto_chk_return(type != CRYPTO_HASH_TYPE_HMAC_SHA224 &&
        type != CRYPTO_HASH_TYPE_HMAC_SHA256 && type != CRYPTO_HASH_TYPE_HMAC_SHA384 &&
        type != CRYPTO_HASH_TYPE_HMAC_SHA512 && type != CRYPTO_HASH_TYPE_HMAC_SM3,
        HASH_COMPAT_ERRNO(ERROR_INVALID_PARAM), "hash_type is invalid!\n");
    block_size = crypto_hash_get_block_size(type) / CRYPTO_BITS_IN_BYTE;
    /* plen should not larger than block_size. */
    crypto_chk_return(param->plen > block_size, HASH_COMPAT_ERRNO(ERROR_INVALID_PARAM),
        "plen should not larger than block_size\n");

    return TD_SUCCESS;
}

td_s32 drv_cipher_pbkdf2(const crypto_kdf_pbkdf2_param *param, td_u8 *out, const td_u32 out_len)
{
    td_s32 ret = TD_SUCCESS;
    td_u8 *data = TD_NULL;
    td_u32 data_len;
    td_u8 hmac[64] = {0};
    td_u8 result_hmac[64] = {0};
    td_u32 hmac_len = 64;
    td_u32 i, j, k;
    td_u32 result_size;
    td_u32 times;
    td_u32 value;
    crypto_drv_func_enter();

    ret = inner_pbkdf2_param_chk(param, out, out_len);
    crypto_chk_return(ret != TD_SUCCESS, ret, "inner_pbkdf2_param_chk failed\n");

    priv_pbkdf2_parse_param(param, out_len, &result_size, &times);

    data_len = param->slen + sizeof(td_u32);
    data = (td_u8 *)crypto_malloc_mmz(data_len, "crypto_pbkdf2_buffer");
    crypto_chk_return(data == TD_NULL, HASH_COMPAT_ERRNO(ERROR_MALLOC), "crypto_malloc_mmz failed\n");

    if (param->slen != 0) {
        ret = memcpy_s(data, data_len, param->salt, param->slen);
        crypto_chk_goto_with_ret(ret, ret != EOK, exit_free, HASH_COMPAT_ERRNO(ERROR_MEMCPY_S), "memcpy_s failed\n");
    }

    for (i = 0; i < times; i++) {
        value = crypto_cpu_to_be32(i + 1);
        (td_void)memcpy_s(data + param->slen, sizeof(td_u32), &value, sizeof(td_u32));

        ret = priv_hmac(param, data, data_len, hmac, &hmac_len);
        crypto_chk_goto(ret != EOK, exit_free, "priv_hmac failed\n");

        ret = memcpy_s(result_hmac, sizeof(result_hmac), hmac, hmac_len);
        crypto_chk_goto_with_ret(ret, ret != EOK, exit_free, HASH_COMPAT_ERRNO(ERROR_MEMCPY_S), "memcpy_s failed\n");

        for (j = 1; j < param->count; j++) {
            ret = priv_hmac(param, hmac, hmac_len, hmac, &hmac_len);
            crypto_chk_goto(ret != EOK, exit_free, "priv_hmac failed\n");

            for (k = 0; k < hmac_len; k++) {
                result_hmac[k] ^= hmac[k];
            }
        }
        if (hmac_len <= out_len - i * result_size) {
            ret = memcpy_s(out + i * result_size, out_len - i * result_size, result_hmac, hmac_len);
        } else {
            ret = memcpy_s(out + i * result_size, out_len - i * result_size, result_hmac, out_len - i * result_size);
        }
        crypto_chk_goto_with_ret(ret, ret != EOK, exit_free, HASH_COMPAT_ERRNO(ERROR_MEMCPY_S), "memcpy_s failed\n");
    }
exit_free:
    (td_void)memset_s(data, data_len, 0, data_len);
    crypto_free_coherent(data);
    crypto_drv_func_exit();
    return ret;
}
#endif