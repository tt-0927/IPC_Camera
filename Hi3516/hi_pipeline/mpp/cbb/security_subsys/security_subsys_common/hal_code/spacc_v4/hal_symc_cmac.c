/**
  Copyright (c), 2001-2025, Shenshu Tech. Co., Ltd.
 */
#if defined(CONFIG_SYMC_CMAC_SUPPORT) || defined(CONFIG_SYMC_CBC_MAC_SUPPORT)

#include "hal_symc.h"
#include "hal_spacc_reg.h"
#include "hal_spacc_inner.h"

#include "crypto_drv_common.h"

int32_t hal_cipher_symc_cmac_update(uint32_t chn_num, uint64_t data_phys_addr, uint32_t data_len, crypto_cmac_ctx *ctx)
{
    td_s32 ret;
    uint8_t *cmac_buf = NULL;
    uint32_t cmac_buf_len = CRYPTO_AES_BLOCK_SIZE_IN_BYTES;
    uint32_t left = data_len;
    uint32_t processing_len = 0;

    crypto_hal_func_enter();
#if defined(CONFIG_SYMC_CMAC_TRACE_ENABLE)
    crypto_dump_data("cmac before", ctx->mac, sizeof(ctx->mac));
#endif
    crypto_chk_return(data_len == 0, SYMC_COMPAT_ERRNO(ERROR_INVALID_PARAM), "data_len is zero!\n");

    /* process the left block. */
    if (ctx->left_len != 0) {
        crypto_chk_return(ctx->left_len != CRYPTO_AES_BLOCK_SIZE_IN_BYTES, SYMC_COMPAT_ERRNO(ERROR_INVALID_PARAM),
            "cmac left must be 16 Bytes\n");
        cmac_buf = crypto_malloc_coherent(cmac_buf_len, "cmac_buf");
        crypto_chk_return(cmac_buf == NULL, SYMC_COMPAT_ERRNO(ERROR_MALLOC), "crypto_malloc_coherent failed\n");
#if defined(CONFIG_SYMC_CMAC_TRACE_ENABLE)
        crypto_dump_data("cmac update left data", ctx->left, ctx->left_len);
#endif
        (void)memcpy_s(cmac_buf, cmac_buf_len, ctx->left, ctx->left_len);
        ret = hal_cipher_symc_cbc_mac_update(chn_num, crypto_get_phys_addr(cmac_buf), cmac_buf_len,
            ctx->mac, sizeof(ctx->mac));
        crypto_free_coherent(cmac_buf);
        crypto_chk_return(ret != TD_SUCCESS, ret, "hal_cipher_symc_cbc_mac_update failed\n");
    }

    /* store the final block. */
    processing_len = crypto_min(CRYPTO_AES_BLOCK_SIZE_IN_BYTES, data_len);
    ret = crypto_copy_from_phys_addr(ctx->left, sizeof(ctx->left),
        data_phys_addr + data_len - processing_len, processing_len);
    crypto_chk_return(ret != TD_SUCCESS, ret, "crypto_phys_copy_to_virt failed\n");
    ctx->left_len = processing_len;
    left -= processing_len;

    if (left != 0) {
#if defined(CONFIG_SYMC_CMAC_TRACE_ENABLE)
        crypto_dump_phys_addr("cmac update data", data_phys_addr, left);
#endif
        ret = hal_cipher_symc_cbc_mac_update(chn_num, data_phys_addr, left, ctx->mac, sizeof(ctx->mac));
        crypto_chk_return(ret != ret, ret, "hal_cipher_symc_cbc_mac_update failed\n");
    }

#if defined(CONFIG_SYMC_CMAC_TRACE_ENABLE)
    crypto_dump_data("cmac after", ctx->mac, sizeof(ctx->mac));
    crypto_dump_data("cmac left", ctx->left, ctx->left_len);
#endif

    crypto_hal_func_exit();
    return TD_SUCCESS;
}

int32_t hal_cipher_symc_cmac_finish(uint32_t chn_num, crypto_cmac_ctx *ctx)
{
    td_s32 ret;
    crypto_symc_hard_context *hard_ctx = TD_NULL;
    in_sym_chn_key_ctrl in_key_ctrl;
    uint8_t *cmac_buf = NULL;
    uint32_t cmac_buf_len = CRYPTO_AES_BLOCK_SIZE_IN_BYTES;

    crypto_hal_func_enter();
#if defined(CONFIG_SYMC_CMAC_TRACE_ENABLE)
    crypto_dump_data("cmac_finish_data", ctx->left, ctx->left_len);
    crypto_dump_data("cmac before", ctx->mac, sizeof(ctx->mac));
#endif

    hard_ctx = inner_hal_get_symc_ctx(chn_num);
    crypto_chk_return(hard_ctx == TD_NULL, SYMC_COMPAT_ERRNO(ERROR_INVALID_HANDLE), "invalid chn_num");

    ret = hal_cipher_symc_set_iv(chn_num, ctx->mac, sizeof(ctx->mac));
    crypto_chk_return(ret != TD_SUCCESS, ret, "hal_cipher_symc_set_iv failed\n");

    in_key_ctrl.u32 = spacc_reg_read(IN_SYM_CHN_KEY_CTRL(chn_num));
    in_key_ctrl.bits.sym_alg_sel = SYMC_ALG_AES_VAL;
    in_key_ctrl.bits.sym_alg_mode = SYMC_ALG_MODE_CMAC_VAL;
    spacc_reg_write(IN_SYM_CHN_KEY_CTRL(chn_num), in_key_ctrl.u32);

    cmac_buf = crypto_malloc_coherent(cmac_buf_len, "cmac_buf");
    crypto_chk_return(cmac_buf == NULL, SYMC_COMPAT_ERRNO(ERROR_MALLOC), "crypto_malloc_coherent failed\n");

    ret = memcpy_s(cmac_buf, cmac_buf_len, ctx->left, sizeof(ctx->left));
    crypto_chk_goto_with_ret(ret, ret != EOK, exit_free, SYMC_COMPAT_ERRNO(ERROR_MEMCPY_S), "memcpy_s failed\n");

    ret = hal_cipher_symc_add_in_node(chn_num, crypto_get_phys_addr(cmac_buf), ctx->left_len,
        IN_NODE_TYPE_FIRST | IN_NODE_TYPE_LAST);
    crypto_chk_goto(ret != TD_SUCCESS, exit_free, "hal_cipher_symc_add_in_node failed\n");

    ret = hal_cipher_symc_add_out_node(chn_num, crypto_get_phys_addr(cmac_buf), CRYPTO_AES_BLOCK_SIZE_IN_BYTES);
    crypto_chk_goto(ret != TD_SUCCESS, exit_free, "hal_cipher_symc_add_out_node failed\n");

    ret = hal_cipher_symc_start(chn_num);
    crypto_chk_goto(ret != TD_SUCCESS, exit_free, "hal_cipher_symc_start failed\n");

    ret = hal_cipher_symc_wait_done(chn_num, TD_FALSE);
    crypto_chk_goto(ret != TD_SUCCESS, exit_free, "hal_cipher_symc_wait_done failed\n");

    ret = hal_cipher_symc_get_iv(chn_num, ctx->mac, sizeof(ctx->mac));
    crypto_chk_goto(ret != TD_SUCCESS, exit_free, "hal_cipher_symc_get_iv failed\n");

#if defined(CONFIG_SYMC_CMAC_TRACE_ENABLE)
    crypto_dump_data("cmac after", ctx->mac, sizeof(ctx->mac));
#endif
    ret = TD_SUCCESS;
exit_free:
    crypto_free_coherent(cmac_buf);
    crypto_hal_func_exit();
    return ret;
}
#endif