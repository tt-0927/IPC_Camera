/**
  Copyright (c), 2001-2025, Shenshu Tech. Co., Ltd.
 */
#if defined(CONFIG_SYMC_CMAC_SUPPORT) || defined(CONFIG_SYMC_CBC_MAC_SUPPORT)

#include "drv_symc.h"
#include "drv_symc_inner.h"
#include "drv_symc_outer.h"
#include "hal_symc.h"
#include "crypto_drv_common.h"

td_s32 drv_cipher_mac_start(td_handle *symc_handle, const crypto_symc_mac_attr *mac_attr)
{
    td_s32 ret;
    crypto_symc_attr symc_attr = {0};
    drv_symc_context_t *symc_ctx = TD_NULL;
    td_u32 chn_num;
    hal_symc_config_t symc_config = {0};
    crypto_drv_func_enter();
    symc_null_ptr_chk(symc_handle);
    symc_null_ptr_chk(mac_attr);

#if !defined(CONFIG_SYMC_SM4_MAC_SUPPORT)
    crypto_chk_return(mac_attr->symc_alg != CRYPTO_SYMC_ALG_AES, SYMC_COMPAT_ERRNO(ERROR_UNSUPPORT),
        "MAC only support AES\n");
#endif
    crypto_chk_return(mac_attr->work_mode != CRYPTO_SYMC_WORK_MODE_CBC_MAC &&
        mac_attr->work_mode != CRYPTO_SYMC_WORK_MODE_CMAC, SYMC_COMPAT_ERRNO(ERROR_INVALID_PARAM), "mode is invalid\n");

    ret = drv_cipher_symc_create(symc_handle, &symc_attr);
    crypto_chk_return(ret != TD_SUCCESS, ret, "drv_cipher_symc_create failed\n");

    symc_ctx = inner_get_symc_ctx(*symc_handle);
    crypto_chk_goto_with_ret(ret, symc_ctx == TD_NULL, error_symc_destroy,
        SYMC_COMPAT_ERRNO(ERROR_INVALID_HANDLE), "symc_handle is invalid\n");
    crypto_chk_goto_with_ret(ret, symc_ctx->is_open == TD_FALSE, error_symc_destroy,
        SYMC_COMPAT_ERRNO(ERROR_CTX_CLOSED), "ctx is closed\n");

    chn_num = *symc_handle;
    symc_config.symc_alg = mac_attr->symc_alg;
    /* For Both CBC_MAC and CMAC, the first work_mdoe is CBC_MAC. */
    symc_config.work_mode = CRYPTO_SYMC_WORK_MODE_CBC_MAC;
    symc_config.symc_bit_width = CRYPTO_SYMC_BIT_WIDTH_128BIT;
    symc_config.symc_key_length = mac_attr->symc_key_length;

    ret = hal_cipher_symc_config(chn_num, &symc_config);
    crypto_chk_goto(ret != TD_SUCCESS, error_symc_destroy, "hal_cipher_symc_config failed\n");

    ret = hal_cipher_symc_attach(chn_num, mac_attr->keyslot_chn);
    crypto_chk_goto(ret != TD_SUCCESS, error_symc_destroy, "hal_cipher_symc_config failed\n");

    (void)memset_s(symc_ctx->mac, sizeof(symc_ctx->mac), 0, sizeof(symc_ctx->mac));
    ret = hal_cipher_symc_set_iv(chn_num, symc_ctx->mac, sizeof(symc_ctx->mac));
    crypto_chk_goto(ret != TD_SUCCESS, error_symc_destroy, "hal_cipher_symc_set_iv failed\n");

    symc_ctx->work_mode = mac_attr->work_mode;
    return TD_SUCCESS;
error_symc_destroy:
    drv_cipher_symc_destroy(*symc_handle);
    crypto_drv_func_exit();
    return ret;
}

td_s32 drv_cipher_mac_update(td_handle symc_handle, const crypto_buf_attr *src_buf, td_u32 length)
{
    td_s32 ret;
    td_u32 chn_num;
    drv_symc_context_t *symc_ctx = TD_NULL;
    crypto_drv_func_enter();

    symc_null_ptr_chk(src_buf);
    crypto_chk_return(length == 0 || length > CRYPTO_SYMC_MAC_UPDATE_MAX_LEN,
        SYMC_COMPAT_ERRNO(ERROR_INVALID_PARAM), "length is Invalid\n");

    symc_ctx = inner_get_symc_ctx(symc_handle);
    crypto_chk_return(symc_ctx == TD_NULL, SYMC_COMPAT_ERRNO(ERROR_INVALID_HANDLE), "symc_handle is invalid\n");
    crypto_chk_return(symc_ctx->is_open == TD_FALSE, SYMC_COMPAT_ERRNO(ERROR_CTX_CLOSED), "ctx is closed\n");

    chn_num = symc_handle;

    ret = hal_cipher_symc_set_iv(chn_num, symc_ctx->mac, sizeof(symc_ctx->mac));
    crypto_chk_return(ret != TD_SUCCESS, ret, "hal_cipher_symc_set_iv failed\n");

    if (symc_ctx->work_mode == CRYPTO_SYMC_WORK_MODE_CMAC) {
        ret = hal_cipher_symc_cmac_update(chn_num, crypto_get_phys_addr(src_buf->virt_addr), length,
            &symc_ctx->cmac_ctx);
        crypto_chk_return(ret != TD_SUCCESS, ret, "hal_cipher_symc_cmac_update failed, ret is 0x%x\n", ret);
    } else {
        ret = hal_cipher_symc_cbc_mac_update(chn_num, crypto_get_phys_addr(src_buf->virt_addr), length,
            symc_ctx->cmac_ctx.mac, sizeof(symc_ctx->cmac_ctx.mac));
        crypto_chk_return(ret != TD_SUCCESS, ret, "hal_cipher_symc_cbc_mac_update failed, ret is 0x%x\n", ret);
    }

    crypto_drv_func_exit();
    return ret;
}

td_s32 drv_cipher_mac_finish(td_handle symc_handle, td_u8 *mac, td_u32 *mac_length)
{
    td_s32 ret;
    td_u32 chn_num;
    drv_symc_context_t *symc_ctx = TD_NULL;
    crypto_drv_func_enter();

    symc_null_ptr_chk(mac);
    symc_null_ptr_chk(mac_length);
    crypto_chk_return(*mac_length < CRYPTO_AES_BLOCK_SIZE_IN_BYTES, SYMC_COMPAT_ERRNO(ERROR_INVALID_PARAM),
        "mac_length is not enough\n");

    symc_ctx = inner_get_symc_ctx(symc_handle);
    crypto_chk_return(symc_ctx == TD_NULL, SYMC_COMPAT_ERRNO(ERROR_INVALID_HANDLE), "symc_handle is invalid\n");
    crypto_chk_return(symc_ctx->is_open == TD_FALSE, SYMC_COMPAT_ERRNO(ERROR_CTX_CLOSED), "ctx is closed\n");

    chn_num = symc_handle;

    if (symc_ctx->work_mode == CRYPTO_SYMC_WORK_MODE_CMAC) {
        ret = hal_cipher_symc_cmac_finish(chn_num, &symc_ctx->cmac_ctx);
        crypto_chk_return(ret != TD_SUCCESS, ret, "hal_cipher_symc_cmac_finish failed, ret is 0x%x\n", ret);

        ret = memcpy_s(mac, *mac_length, symc_ctx->cmac_ctx.mac, sizeof(symc_ctx->cmac_ctx.mac));
        crypto_chk_return(ret != TD_SUCCESS, SYMC_COMPAT_ERRNO(ERROR_MEMCPY_S), "memcpy_s failed, ret is 0x%x\n", ret);
    } else {
        ret = memcpy_s(mac, *mac_length, symc_ctx->cmac_ctx.mac, sizeof(symc_ctx->cmac_ctx.mac));
        crypto_chk_return(ret != TD_SUCCESS, SYMC_COMPAT_ERRNO(ERROR_MEMCPY_S), "memcpy_s failed, ret is 0x%x\n", ret);
    }

#if defined(CONFIG_SYMC_MAC_TRACE_ENABLE)
    crypto_dump_data("mac_out_mac", mac, *mac_length);
#endif
    *mac_length = CRYPTO_AES_BLOCK_SIZE_IN_BYTES;

    (td_void)drv_cipher_symc_destroy(symc_handle);
    crypto_drv_func_exit();
    return 0;
}

td_s32 inner_drv_symc_cbc_mac_set_ctx(td_handle symc_handle, const td_u8 *mac, uint32_t mac_len)
{
    td_s32 ret;
    drv_symc_context_t *symc_ctx = TD_NULL;
    crypto_drv_func_enter();

    symc_ctx = inner_get_symc_ctx(symc_handle);
    crypto_chk_return(symc_ctx == TD_NULL, SYMC_COMPAT_ERRNO(ERROR_INVALID_HANDLE), "symc_handle is invalid\n");
    crypto_chk_return(symc_ctx->is_open == TD_FALSE, SYMC_COMPAT_ERRNO(ERROR_CTX_CLOSED), "ctx is closed\n");

    ret = memcpy_s(symc_ctx->mac, sizeof(symc_ctx->mac), mac, mac_len);
    crypto_chk_return(ret != EOK, SYMC_COMPAT_ERRNO(ERROR_MEMCPY_S), "memcpy_s failed\n");

    crypto_drv_func_exit();
    return TD_SUCCESS;
}

int32_t inner_drv_mac_set_ctx(td_handle symc_handle, const crypto_cmac_ctx *mac_ctx)
{
    td_s32 ret;
    drv_symc_context_t *symc_ctx = TD_NULL;
    crypto_drv_func_enter();

    symc_ctx = inner_get_symc_ctx(symc_handle);
    crypto_chk_return(symc_ctx == TD_NULL, SYMC_COMPAT_ERRNO(ERROR_INVALID_HANDLE), "symc_handle is invalid\n");
    crypto_chk_return(symc_ctx->is_open == TD_FALSE, SYMC_COMPAT_ERRNO(ERROR_CTX_CLOSED), "ctx is closed\n");

    ret = memcpy_s(&symc_ctx->cmac_ctx, sizeof(crypto_cmac_ctx), mac_ctx, sizeof(crypto_cmac_ctx));
    crypto_chk_return(ret != EOK, SYMC_COMPAT_ERRNO(ERROR_MEMCPY_S), "memcpy_s failed\n");

    crypto_drv_func_exit();
    return TD_SUCCESS;
}

int32_t inner_drv_mac_get_ctx(td_handle symc_handle, crypto_cmac_ctx *mac_ctx)
{
    td_s32 ret;
    drv_symc_context_t *symc_ctx = TD_NULL;
    crypto_drv_func_enter();

    symc_ctx = inner_get_symc_ctx(symc_handle);
    crypto_chk_return(symc_ctx == TD_NULL, SYMC_COMPAT_ERRNO(ERROR_INVALID_HANDLE), "symc_handle is invalid\n");
    crypto_chk_return(symc_ctx->is_open == TD_FALSE, SYMC_COMPAT_ERRNO(ERROR_CTX_CLOSED), "ctx is closed\n");

    ret = memcpy_s(mac_ctx, sizeof(crypto_cmac_ctx), &symc_ctx->cmac_ctx, sizeof(crypto_cmac_ctx));
    crypto_chk_return(ret != EOK, SYMC_COMPAT_ERRNO(ERROR_MEMCPY_S), "memcpy_s failed\n");

    crypto_drv_func_exit();
    return TD_SUCCESS;
}
#endif