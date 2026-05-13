/**
  Copyright (c), 2001-2025, Shenshu Tech. Co., Ltd.
 */

#if defined(CONFIG_PKE_CAL_HASH_SUPPORT)

#include "hal_pke_alg.h"
#include "drv_pke_inner.h"
#include "drv_hash.h"

#include "crypto_drv_common.h"
#include "crypto_hash_common.h"

static crypto_hash_type drv_pke_get_hash_type(drv_pke_hash_type hash_type)
{
    switch (hash_type) {
        case DRV_PKE_HASH_TYPE_SHA1:
            return CRYPTO_HASH_TYPE_SHA1;
        case DRV_PKE_HASH_TYPE_SHA224:
            return CRYPTO_HASH_TYPE_SHA224;
        case DRV_PKE_HASH_TYPE_SHA256:
            return CRYPTO_HASH_TYPE_SHA256;
        case DRV_PKE_HASH_TYPE_SHA384:
            return CRYPTO_HASH_TYPE_SHA384;
        case DRV_PKE_HASH_TYPE_SHA512:
            return CRYPTO_HASH_TYPE_SHA512;
        case DRV_PKE_HASH_TYPE_SM3:
            return CRYPTO_HASH_TYPE_SM3;
        default:
            crypto_log_err("Invalid hash_type\n");
            return CRYPTO_HASH_TYPE_INVALID;
    }
}

static int inner_hash_update(td_handle hash_handle, unsigned char *buf, unsigned int buf_len)
{
    int ret;
    crypto_buf_attr src_buf;
    unsigned char *data_buf = NULL;

    data_buf = crypto_malloc_coherent(buf_len, "calc_hash_buf");
    crypto_chk_return(data_buf == NULL, ERROR_MALLOC, "crypto_malloc_coherent failed\n");

    ret = memcpy_s(data_buf, buf_len, buf, buf_len);
    crypto_chk_goto_with_ret(ret, ret != CRYPTO_SUCCESS, exit_free, ERROR_MEMCPY_S, "drv_cipher_hash_update failed\n");

    src_buf.virt_addr = data_buf;
    ret = drv_cipher_hash_update(hash_handle, &src_buf, buf_len);
    crypto_chk_goto(ret != CRYPTO_SUCCESS, exit_free, "drv_cipher_hash_update failed\n");

exit_free:
    (void)memset_s(data_buf, buf_len, 0, buf_len);
    crypto_free_coherent(data_buf);
    return ret;
}

#define INVALID_HANDLE  0xFFFF0000
td_s32 hal_pke_alg_calc_hash(const drv_pke_data *arr, td_u32 arr_len,
    const drv_pke_hash_type hash_type, drv_pke_data *hash)
{
    td_s32 ret = TD_FAILURE;
    td_handle h_handle = INVALID_HANDLE;
    td_u32 i;
    crypto_hash_type hash_alg;
    crypto_hash_attr hash_attr;
    crypto_param_check(arr == TD_NULL);
    crypto_param_check(hash == TD_NULL);
    crypto_param_check(hash->data == TD_NULL);

    hash_common_lock();
    hash_alg = drv_pke_get_hash_type(hash_type);
    crypto_chk_goto_with_ret(ret, hash_alg == CRYPTO_HASH_TYPE_INVALID, exit_free,
        PKE_COMPAT_ERRNO(ERROR_INVALID_PARAM), "hash type invalid\n");
    hash_attr = (crypto_hash_attr){.hash_type = hash_alg};

    ret = drv_cipher_hash_start(&h_handle, &hash_attr);
    crypto_chk_goto(ret != CRYPTO_SUCCESS, exit_free, "drv_cipher_hash_start failed\n");

    for (i = 0;i < arr_len; i++) {
        ret = inner_hash_update(h_handle, arr[i].data, arr[i].length);
        crypto_chk_goto(ret != CRYPTO_SUCCESS, hash_destroy, "inner_hash_update failed\n");
    }
    ret = drv_cipher_hash_finish(h_handle, hash->data, &(hash->length));
    crypto_chk_goto(ret != CRYPTO_SUCCESS, hash_destroy, "drv_cipher_hash_finish failed\n");
    h_handle = INVALID_HANDLE;

hash_destroy:
    if (h_handle != INVALID_HANDLE) {
        drv_cipher_hash_destroy(h_handle);
    }
exit_free:
    hash_common_unlock();
    return ret;
}
#endif