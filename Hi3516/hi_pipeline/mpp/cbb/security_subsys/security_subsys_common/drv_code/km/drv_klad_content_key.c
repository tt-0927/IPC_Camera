/**
  Copyright (c), 2001-2025, Shenshu Tech. Co., Ltd.
 */
#if defined(CONFIG_KM_CONTENT_KEY_SUPPORT)
#include "drv_klad.h"
#include "drv_klad_inner.h"

#include "crypto_drv_common.h"
#include "crypto_common_macro.h"

#include "hal_klad.h"
#include "hal_rkp.h"

static td_s32 inner_drv_set_content_key(const crypto_klad_content_key *content_key)
{
    volatile td_s32 ret = TD_FAILURE;
    td_bool is_odd = content_key->key_parity;
    crypto_klad_key_size key_size = CRYPTO_KLAD_KEY_SIZE_128BIT;
    drv_klad_context *klad_ctx = inner_klad_get_ctx();

    /* set session key. */
    ret = hal_klad_set_data(klad_ctx->session_key.key, CRYPTO_128_KEY_LEN);
    crypto_chk_goto(ret != TD_SUCCESS, exit_clean, "hal_klad_set_data failed\n");

    ret = hal_klad_com_start(CRYPTO_KLAD_LEVEL_SEL_FIRST, klad_ctx->session_key.alg,
        CRYPTO_KLAD_KEY_SIZE_128BIT, klad_ctx->hard_key_type);
    crypto_chk_goto(ret != TD_SUCCESS, exit_clean, "hal_klad_com_start failed\n");

    ret = hal_klad_wait_com_route_done();
    crypto_chk_goto(ret != TD_SUCCESS, exit_clean, "hal_klad_wait_com_route_done failed\n");

    /*
     * For MCipher:
     * If key_length is 16, the key_parity is passed by caller;
     * If key_length is 32, the high 128bit's key_parity is even,
        the low 128bit's key_parity is odd.
     */
    if (content_key->key_length == CRYPTO_256_KEY_LEN) {
        is_odd = TD_FALSE;
        key_size = CRYPTO_KLAD_KEY_SIZE_256BIT;
    }
    /* config the high 128bit  */
    if (klad_ctx->klad_dest == CRYPTO_KLAD_DEST_MCIPHER) {
        hal_klad_set_key_odd(is_odd);
    }

    ret = hal_klad_set_data(content_key->key, CRYPTO_128_KEY_LEN);
    crypto_chk_goto(ret != TD_SUCCESS, exit_clean, "hal_klad_set_data failed\n");

    ret = hal_klad_com_start(CRYPTO_KLAD_LEVEL_SEL_SECOND, klad_ctx->session_key.alg,
        key_size, klad_ctx->hard_key_type);
    crypto_chk_goto(ret != TD_SUCCESS, exit_clean, "hal_klad_com_start failed\n");

    ret = hal_klad_wait_com_route_done();
    crypto_chk_goto(ret != TD_SUCCESS, exit_clean, "hal_klad_wait_com_route_done failed\n");

    /* config the low 128bit */
    if (content_key->key_length == CRYPTO_256_KEY_LEN) {
        if (klad_ctx->klad_dest == CRYPTO_KLAD_DEST_MCIPHER) {
            hal_klad_set_key_odd(TD_TRUE);
        }
        ret = hal_klad_set_data(content_key->key + CRYPTO_128_KEY_LEN, CRYPTO_128_KEY_LEN);
        crypto_chk_goto(ret != TD_SUCCESS, exit_clean, "hal_klad_set_data failed\n");

        ret = hal_klad_com_start(CRYPTO_KLAD_LEVEL_SEL_SECOND, klad_ctx->session_key.alg,
            key_size, klad_ctx->hard_key_type);
        crypto_chk_goto(ret != TD_SUCCESS, exit_clean, "hal_klad_com_start failed\n");

        ret = hal_klad_wait_com_route_done();
        crypto_chk_goto(ret != TD_SUCCESS, exit_clean, "hal_klad_wait_com_route_done failed\n");
    }
exit_clean:
    hal_klad_clear_data();
    return ret;
}

td_s32 drv_klad_set_content_key(const td_handle klad_handle, const crypto_klad_content_key *content_key)
{
    volatile td_s32 ret = TD_FAILURE;
    crypto_kdf_hard_calc_param param = {0};
    drv_klad_context *klad_ctx = inner_klad_get_ctx();
    (td_void)(klad_handle);

    km_null_ptr_chk(content_key);
    crypto_chk_return(content_key->key_length != CRYPTO_128_KEY_LEN && content_key->key_length != CRYPTO_256_KEY_LEN,
        KM_COMPAT_ERRNO(ERROR_INVALID_PARAM), "invalid key_length\n");

    (td_void)memset_s(&param, sizeof(crypto_kdf_hard_calc_param), 0, sizeof(crypto_kdf_hard_calc_param));

    /* lock klad */
    ret = hal_klad_lock();
    crypto_chk_return((ret != TD_SUCCESS), ret, "hal_klad_lock failed, ret is 0x%x\n", ret);

    /* lock rkp */
    ret = hal_rkp_lock();
    crypto_chk_goto((ret != TD_SUCCESS), exit_klad_unlock, "hal_rkp_lock failed, ret is 0x%x\n", ret);

    param.hard_key_type = klad_ctx->hard_key_type;
    param.hard_alg = CRYPTO_KDF_HARD_ALG_SHA256;        /* default is SHA256. */
    param.rkp_sw_cfg = klad_ctx->rkp_sw_cfg;

    ret = hal_rkp_kdf_hard_calculation(&param);
    crypto_chk_goto(ret != TD_SUCCESS, exit_rkp_unlock, "hal_rkp_kdf_hard_calculation failed\n");

    ret = inner_drv_set_content_key(content_key);
    crypto_chk_goto(ret != TD_SUCCESS, exit_rkp_unlock, "inner_drv_set_content_key failed\n");

exit_rkp_unlock:
    hal_rkp_unlock();
exit_klad_unlock:
    hal_klad_unlock();
    return ret;
}
#endif