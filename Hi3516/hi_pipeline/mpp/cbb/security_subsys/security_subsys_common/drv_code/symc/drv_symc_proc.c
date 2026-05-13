/**
  Copyright (c), 2001-2025, Shenshu Tech. Co., Ltd.
 */

#include "drv_symc.h"
#include "drv_symc_inner.h"
#include "hal_symc.h"
#include "crypto_drv_common.h"
#if defined(CONFIG_SYMC_PROC_SUPPORT)

td_s32 drv_cipher_symc_get_proc_info(td_handle symc_handle, crypto_symc_proc_info *proc_symc_info)
{
    td_s32 ret;
    td_u32 chn_num;
    drv_symc_context_t *drv_symc_ctx = TD_NULL;

    crypto_drv_func_enter();
    symc_null_ptr_chk(proc_symc_info);

    drv_symc_ctx = inner_get_symc_ctx(symc_handle);
    crypto_chk_return(drv_symc_ctx == TD_NULL, SYMC_COMPAT_ERRNO(ERROR_INVALID_HANDLE), "inner_get_symc_ctx failed\n");

    chn_num = drv_symc_ctx->chn_num;
    proc_symc_info->chn_id = chn_num;
    proc_symc_info->open = drv_symc_ctx->is_open;

    if (drv_symc_ctx->is_open == TD_FALSE) {
        return SYMC_COMPAT_ERRNO(ERROR_CTX_CLOSED);
    }

    if (drv_symc_ctx->is_config == TD_FALSE) {
        return SYMC_COMPAT_ERRNO(ERROR_NOT_SET_CONFIG);
    }

    ret = hal_cipher_symc_get_proc_info(chn_num, proc_symc_info);
    crypto_chk_return(ret != TD_SUCCESS, ret, "hal_cipher_symc_get_proc_info, ret is 0x%x\n", ret);

    ret = hal_cipher_symc_get_iv(chn_num, proc_symc_info->iv, sizeof(proc_symc_info->iv));
    crypto_chk_return(ret != TD_SUCCESS, ret, "hal_cipher_symc_get_iv failed, ret is 0x%x\n", ret);

    crypto_drv_func_exit();
    return ret;
}
#else
td_s32 drv_cipher_symc_get_proc_info(td_handle symc_handle, crypto_symc_proc_info *proc_symc_info)
{
    crypto_unused(symc_handle);
    crypto_unused(proc_symc_info);

    return SYMC_COMPAT_ERRNO(ERROR_UNSUPPORT);
}
#endif