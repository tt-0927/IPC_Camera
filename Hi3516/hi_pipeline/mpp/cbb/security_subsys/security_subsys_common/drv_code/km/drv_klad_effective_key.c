/**
  Copyright (c), 2001-2025, Shenshu Tech. Co., Ltd.
 */
#if defined(CONFIG_KM_EFFECTIVE_KEY_SUPPORT)

#include "drv_klad.h"
#include "drv_klad_inner.h"

#include "crypto_drv_common.h"
#include "crypto_common_macro.h"

#include "hal_klad.h"
#include "hal_rkp.h"

td_s32 drv_klad_set_effective_key(td_handle klad_handle, const crypto_klad_effective_key *effective_key)
{
    volatile td_s32 ret = TD_FAILURE;
    crypto_kdf_hard_calc_param param = {0};
    drv_klad_context *klad_ctx = inner_klad_get_ctx();

    crypto_chk_return(effective_key == TD_NULL, TD_FAILURE, "effective_key is NULL\n");
    crypto_chk_return(effective_key->salt == TD_NULL, TD_FAILURE, "effective_key->salt is NULL\n");
    if (effective_key->kdf_hard_alg == CRYPTO_KDF_HARD_ALG_SM3) {
        crypto_chk_return(crypto_sm3_support() == TD_FALSE, TD_FAILURE,
            "alg is unsupport\n");
    }

    crypto_chk_return(klad_handle != KLAD_VALID_HANDLE, TD_FAILURE, "invalid klad_handle\n");
    crypto_chk_return(klad_ctx->is_open == TD_FALSE, TD_FAILURE, "call create first\n");
    crypto_chk_return(klad_ctx->is_set_attr == TD_FALSE, TD_FAILURE, "call set_attr first\n");

    switch (effective_key->key_size) {
        case CRYPTO_KLAD_KEY_SIZE_128BIT:
            param.hard_key_size = CRYPTO_KDF_HARD_KEY_SIZE_128BIT;
            break;
        case CRYPTO_KLAD_KEY_SIZE_192BIT:
            param.hard_key_size = CRYPTO_KDF_HARD_KEY_SIZE_192BIT;
            break;
        case CRYPTO_KLAD_KEY_SIZE_256BIT:
            param.hard_key_size = CRYPTO_KDF_HARD_KEY_SIZE_256BIT;
            break;
        default:
            crypto_log_err("invalid key_size\n");
            return TD_FAILURE;
    }

    param.hard_key_type = klad_ctx->hard_key_type;
    param.hard_alg = effective_key->kdf_hard_alg;
    param.salt = effective_key->salt;
    param.salt_length = effective_key->salt_length;
    param.is_oneway = effective_key->oneway;

    /* klad lock. */
    ret = hal_klad_lock();
    crypto_chk_return(ret != TD_SUCCESS, ret, "hal_klad_lock failed\n");

    /* rkp lock. */
    ret = hal_rkp_lock();
    crypto_chk_goto(ret != TD_SUCCESS, exit_klad_unlock, "hal_rkp_lock failed\n");

    /* rkp hard calculation. */
    ret = hal_rkp_kdf_hard_calculation(&param);
    crypto_chk_goto(ret != TD_SUCCESS, exit_rkp_unlock, "hal_rkp_kdf_hard_calculation failed\n");

    ret = hal_klad_start_com_route(param.hard_key_type, effective_key, klad_ctx->klad_dest);
    crypto_chk_goto(ret != TD_SUCCESS, exit_rkp_unlock, "hal_klad_start_com_route failed\n");

exit_rkp_unlock:
    (td_void)hal_rkp_unlock();
exit_klad_unlock:
    (td_void)hal_klad_unlock();
    return ret;
}
#endif