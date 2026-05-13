/**
  Copyright (c), 2001-2025, Shenshu Tech. Co., Ltd.
 */

#include "hal_klad.h"
#include "hal_klad_reg.h"
#include "hal_km_inner.h"

#include "crypto_drv_common.h"
#include "crypto_common_macro.h"

#define KM_COMPAT_ERRNO(err_code)     HAL_COMPAT_ERRNO(ERROR_MODULE_KM, err_code)

#define KLAD_CLR_ROUTE_TIMEOUT          1000000
static td_s32 inner_klad_wait_clr_route_done(td_void)
{
    volatile td_s32 ret = TD_FAILURE;
    kl_clr_ctrl clr_ctrl = { 0 };
    kl_int_raw int_raw = {0};
    td_u32 i = 0;

    for (i = 0; i < KLAD_CLR_ROUTE_TIMEOUT; ++i) {
        clr_ctrl.u32 = km_reg_read(KL_CLR_CTRL);
        if (clr_ctrl.bits.kl_clr_start == 0) {
            int_raw.u32 = km_reg_read(KL_INT_RAW);
            int_raw.bits.clr_kl_int_raw = 0x1;
            km_reg_write(KL_INT_RAW, int_raw.u32);
            break;
        }
        crypto_udelay(1);
    }
    if (i >= KLAD_CLR_ROUTE_TIMEOUT) {
        return KM_COMPAT_ERRNO(ERROR_KLAD_TIMEOUT);
    }

    ret = inner_klad_error_check();
    crypto_chk_return(ret != TD_SUCCESS, ret, "inner_klad_error_check failed\n");

    return ret;
}

#define HKL_CLR_KEY_SIZE_128            0x1
#define HKL_CLR_KEY_SIZE_192            0x2
#define HKL_CLR_KEY_SIZE_256            0x3
static td_s32 inner_klad_symc_start_clr_route(const td_u8 *key, td_u32 key_length, td_bool odd)
{
    volatile td_s32 ret = TD_FAILURE;
    td_bool key_parity = TD_FALSE;
    td_u32 key_size_reg_val = 0;
    kl_clr_ctrl clr_ctrl = {0};

    switch (key_length) {
        case CRYPTO_128_KEY_LEN:
            key_size_reg_val = HKL_CLR_KEY_SIZE_128;
            break;
        case CRYPTO_192_KEY_LEN:
            key_size_reg_val = HKL_CLR_KEY_SIZE_192;
            break;
        case CRYPTO_256_KEY_LEN:
            key_size_reg_val = HKL_CLR_KEY_SIZE_256;
            break;
        default:
            crypto_log_err("invalid key_length\n");
            return KM_COMPAT_ERRNO(ERROR_INVALID_PARAM);
    }
    /*
     * If key_length is 16, the key_parity is passed by caller;
     * If key_length is 24/32, the high 128bit's key_parity is even,
        the low 128bit's(padding 0 for 192bit) key_parity is odd.
     */
    if (key_length == HKL_KEY_LEN) {
        key_parity = TD_FALSE;
    }
    /* config the high 128bit  */
    hal_klad_set_key_odd(key_parity);
    ret = hal_klad_set_data(key, HKL_KEY_LEN);
    crypto_chk_goto(ret != TD_SUCCESS, exit_clean, "hal_klad_set_data failed\n");

    /* config clr_ctrl */
    clr_ctrl.u32 = km_reg_read(KL_CLR_CTRL);
    clr_ctrl.bits.kl_clr_key_size = key_size_reg_val;   /* symc key need to config */
    clr_ctrl.bits.kl_clr_start = 0x1;                   /* start calculation */
    km_reg_write(KL_CLR_CTRL, clr_ctrl.u32);

    ret = inner_klad_wait_clr_route_done();
    crypto_chk_goto(ret != TD_SUCCESS, exit_clean, "inner_klad_wait_clr_route_done failed\n");

    /* config the low 64bit/128bit */
    if (key_length != HKL_KEY_LEN) {
        hal_klad_set_key_odd(TD_TRUE);
        ret = hal_klad_set_data(key + HKL_KEY_LEN, key_length - HKL_KEY_LEN);
        crypto_chk_goto(ret != TD_SUCCESS, exit_clean, "hal_klad_set_data failed\n");

        /* config clr_ctrl */
        clr_ctrl.u32 = km_reg_read(KL_CLR_CTRL);
        clr_ctrl.bits.kl_clr_key_size = key_size_reg_val;   /* symc key need to config */
        clr_ctrl.bits.kl_clr_start = 0x1;                   /* start calculation */
        km_reg_write(KL_CLR_CTRL, clr_ctrl.u32);

        ret = inner_klad_wait_clr_route_done();
        crypto_chk_goto(ret != TD_SUCCESS, exit_clean, "inner_klad_wait_clr_route_done failed\n");
    }

exit_clean:
    hal_klad_clear_data();
    return ret;
}

#define HMAC_KEY_BLOCK_SIZE_512     64
#define HMAC_KEY_BLOCK_SIZE_1024    128
#define HMAC_KEY_CAL_CNT_512        4
#define HMAC_KEY_CAL_CNT_1024       8
#define HKL_HMAC_KEY_MAX_SIZE       128
static td_s32 inner_klad_hmac_start_clr_route(const td_u8 *key, td_u32 key_length, crypto_klad_hmac_type hmac_type)
{
    volatile td_s32 ret = TD_FAILURE;
    td_u32 hmac_cal_cnt = HMAC_KEY_CAL_CNT_512;
    td_u32 i;
    kl_clr_ctrl clr_ctrl = {0};
    td_u8 key_padding[128] = {0};

    km_null_ptr_chk(key);
    crypto_chk_return(hmac_type != CRYPTO_KLAD_HMAC_TYPE_SHA1 && hmac_type != CRYPTO_KLAD_HMAC_TYPE_SHA224 &&
        hmac_type != CRYPTO_KLAD_HMAC_TYPE_SHA256 && hmac_type != CRYPTO_KLAD_HMAC_TYPE_SHA384 &&
        hmac_type != CRYPTO_KLAD_HMAC_TYPE_SHA512 && hmac_type != CRYPTO_KLAD_HMAC_TYPE_SM3,
        KM_COMPAT_ERRNO(ERROR_INVALID_PARAM), "invalid hmac_type\n");

    ret = memcpy_s(key_padding, sizeof(key_padding), key, key_length);
    crypto_chk_return(ret != EOK, KM_COMPAT_ERRNO(ERROR_MEMCPY_S), "memcpy_s failed\n");

#if defined(CONFIG_HMAC_SHA384_SUPPORT) || defined(CONFIG_HMAC_SHA512_SUPPORT)
    if (hmac_type == CRYPTO_KLAD_HMAC_TYPE_SHA384 || hmac_type == CRYPTO_KLAD_HMAC_TYPE_SHA512) {
        hmac_cal_cnt = HMAC_KEY_CAL_CNT_1024;
    }
#else
    crypto_chk_return(hmac_type == CRYPTO_KLAD_HMAC_TYPE_SHA384 || hmac_type == CRYPTO_KLAD_HMAC_TYPE_SHA512,
        KM_COMPAT_ERRNO(ERROR_UNSUPPORT), "unsupport hmac_type\n");
#endif

    for (i = 0; i < hmac_cal_cnt; i++) {
        ret = hal_klad_set_data(key_padding + HKL_KEY_LEN * i, HKL_KEY_LEN);
        crypto_chk_goto(ret != TD_SUCCESS, exit_clean, "hal_klad_set_data failed\n");

        clr_ctrl.bits.kl_clr_key_cnt = i;       /* only software Hmac mode need to config */
        clr_ctrl.bits.kl_clr_start = 0x1;       /* start calculation */
        km_reg_write(KL_CLR_CTRL, clr_ctrl.u32);

        ret = inner_klad_wait_clr_route_done();
        crypto_chk_goto(ret != TD_SUCCESS, exit_clean, "inner_klad_wait_clr_route_done failed\n");
    }

exit_clean:
    hal_klad_clear_data();
    return ret;
}

td_s32 hal_klad_start_clr_route(crypto_klad_dest klad_dest, const crypto_klad_clear_key *clear_key)
{
    volatile td_s32 ret = TD_FAILURE;

    km_null_ptr_chk(clear_key);
    km_null_ptr_chk(clear_key->key);

    if (klad_dest == CRYPTO_KLAD_DEST_MCIPHER || klad_dest == CRYPTO_KLAD_DEST_FLASH) {
        ret = inner_klad_symc_start_clr_route(clear_key->key, clear_key->key_length, clear_key->key_parity);
        crypto_chk_return(ret != TD_SUCCESS, ret, "inner_klad_symc_start_clr_route failed\n");
    } else if (klad_dest == CRYPTO_KLAD_DEST_HMAC) {
        ret = inner_klad_hmac_start_clr_route(clear_key->key, clear_key->key_length, clear_key->hmac_type);
        crypto_chk_return(ret != TD_SUCCESS, ret, "inner_klad_hmac_start_clr_route failed\n");
    } else {
        crypto_log_err("invalid klad_dest\n");
        return KM_COMPAT_ERRNO(ERROR_INVALID_PARAM);
    }
    return ret;
}