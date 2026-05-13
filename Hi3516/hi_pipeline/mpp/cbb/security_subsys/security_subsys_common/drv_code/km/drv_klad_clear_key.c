/**
  Copyright (c), 2001-2025, Shenshu Tech. Co., Ltd.
 */
#if defined(CONFIG_KM_CLEAR_KEY_SUPPORT)
#include "drv_klad.h"
#include "drv_klad_inner.h"

#include "crypto_drv_common.h"
#include "crypto_common_macro.h"

#include "hal_klad.h"
#include "hal_rkp.h"

td_s32 drv_klad_set_clear_key(td_handle klad_handle, const crypto_klad_clear_key *clear_key)
{
    volatile td_s32 ret = TD_FAILURE;
    drv_klad_context *klad_ctx = inner_klad_get_ctx();

    crypto_drv_func_enter();

    crypto_chk_return(klad_handle != KLAD_VALID_HANDLE, KM_COMPAT_ERRNO(ERROR_INVALID_HANDLE), "invalid klad_handle\n");
    km_null_ptr_chk(clear_key);
    km_null_ptr_chk(clear_key->key);

    crypto_chk_return(clear_key->key_parity != TD_FALSE && clear_key->key_parity != TD_TRUE,
        KM_COMPAT_ERRNO(ERROR_INVALID_PARAM), "clear_key.key_parity is invalid\n");
    crypto_chk_return(klad_ctx->is_open == TD_FALSE, KM_COMPAT_ERRNO(ERROR_CTX_CLOSED), "call create first\n");
    crypto_chk_return(klad_ctx->is_set_attr == TD_FALSE, KM_COMPAT_ERRNO(ERROR_NOT_SET_CONFIG),
        "call set_attr first\n");

    ret = hal_klad_start_clr_route(klad_ctx->klad_dest, clear_key);
    crypto_chk_return(ret != TD_SUCCESS, ret, "hal_klad_start_clr_route failed\n");

    klad_ctx->is_set_key = TD_TRUE;

    crypto_drv_func_enter();
    return ret;
}
#endif