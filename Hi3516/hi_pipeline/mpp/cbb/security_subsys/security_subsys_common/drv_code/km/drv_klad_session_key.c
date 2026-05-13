/**
  Copyright (c), 2001-2025, Shenshu Tech. Co., Ltd.
 */
#if defined(CONFIG_KM_SESSION_KEY_SUPPORT)

#include "drv_klad.h"
#include "drv_klad_inner.h"

#include "crypto_drv_common.h"
#include "crypto_common_macro.h"

#include "hal_klad.h"
#include "hal_rkp.h"

td_s32 drv_klad_set_session_key(td_handle klad_handle, const crypto_klad_session_key *session_key)
{
    volatile td_s32 ret = TD_FAILURE;
    drv_klad_context *klad_ctx = inner_klad_get_ctx();
    crypto_drv_func_enter();

    crypto_chk_return(klad_handle != KLAD_VALID_HANDLE, KM_COMPAT_ERRNO(ERROR_INVALID_HANDLE), "invalid klad_handle\n");
    km_null_ptr_chk(session_key);

    ret = memcpy_s(&klad_ctx->session_key, sizeof (crypto_klad_session_key),
        session_key, sizeof(crypto_klad_session_key));
    crypto_chk_return(ret != EOK, KM_COMPAT_ERRNO(ERROR_MEMCPY_S), "memcpy_s failed\n");

    klad_ctx->is_set_session_key = TD_TRUE;

    crypto_drv_func_enter();
    return 0;
}
#endif