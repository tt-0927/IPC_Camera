/**
  Copyright (c), 2001-2025, Shenshu Tech. Co., Ltd.
 */

#include "drv_symc.h"
#include "drv_symc_inner.h"
#include "hal_symc.h"
#include "crypto_drv_common.h"

td_s32 drv_cipher_symc_get_config(td_handle symc_handle, crypto_symc_ctrl_t *symc_ctrl)
{
    td_s32 ret;
    td_u32 chn_num;
    hal_symc_config_t hal_symc_config = {0};
    drv_symc_context_t *symc_ctx = TD_NULL;

    crypto_drv_func_enter();
    ret = inner_symc_drv_handle_chk(symc_handle);
    if (ret != TD_SUCCESS) {
        return ret;
    }
    symc_null_ptr_chk(symc_ctrl);

    chn_num = symc_handle;
    symc_ctx = inner_get_symc_ctx(symc_handle);
    crypto_chk_return(symc_ctx == TD_NULL, SYMC_COMPAT_ERRNO(ERROR_INVALID_HANDLE), "invalid handle\n");
    crypto_chk_return(symc_ctx->is_open == TD_FALSE, SYMC_COMPAT_ERRNO(ERROR_CTX_CLOSED), "ctx is closed\n");
    crypto_chk_return(symc_ctx->is_config == TD_FALSE,
        SYMC_COMPAT_ERRNO(ERROR_NOT_SET_CONFIG), "call set_config first\n");

    ret = hal_cipher_symc_get_config(chn_num, &hal_symc_config);
    crypto_chk_return(ret != TD_SUCCESS, ret, "hal_cipher_symc_unlock_chn failed, ret is 0x%x\n", ret);

    ret = memcpy_s(symc_ctrl->iv, sizeof(symc_ctx->iv), symc_ctx->iv, sizeof(symc_ctx->iv));
    crypto_chk_return(ret != EOK, SYMC_COMPAT_ERRNO(ERROR_MEMCPY_S), "memcpy_s failed, ret is 0x%x\n", ret);

    crypto_drv_func_exit();
    return ret;
}