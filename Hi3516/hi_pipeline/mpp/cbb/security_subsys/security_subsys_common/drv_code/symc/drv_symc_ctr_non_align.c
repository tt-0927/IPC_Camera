/**
  Copyright (c), 2001-2025, Shenshu Tech. Co., Ltd.
 */
#if defined(CONFIG_SYMC_CTR_NON_ALIGN_SUPPORT)

#include "drv_symc.h"
#include "drv_symc_inner.h"
#include "hal_symc.h"
#include "crypto_drv_common.h"

#if defined(CONFIG_SYMC_SET_CTR_BLOCK)
td_s32 inner_drv_symc_set_ctr_block(td_handle symc_handle, const td_u8 *block, td_u32 block_size, td_u32 ctr_offset)
{
    td_s32 ret;
    drv_symc_context_t *symc_ctx = NULL;
    drv_symc_param_trace("symc_handle is 0x%x\n", symc_handle);
    drv_symc_param_dump_trace("block", block, block_size);
    drv_symc_param_trace("block_size is 0x%x\n", block_size);
    drv_symc_param_trace("ctr_offset is 0x%x\n", ctr_offset);

    symc_ctx = inner_get_symc_ctx(symc_handle);
    crypto_chk_return(symc_ctx == TD_NULL, SYMC_COMPAT_ERRNO(ERROR_INVALID_HANDLE), "invalid handle\n");
    crypto_chk_return(symc_ctx->is_open == TD_FALSE, SYMC_COMPAT_ERRNO(ERROR_CTX_CLOSED), "ctx is closed\n");

    symc_null_ptr_chk(block);
    crypto_chk_return(block_size != CRYPTO_AES_BLOCK_SIZE_IN_BYTES, SYMC_COMPAT_ERRNO(ERROR_INVALID_PARAM),
        "block_size is invalid\n");

    ret = memcpy_s(symc_ctx->ctr_last_block, sizeof(symc_ctx->ctr_last_block), block, block_size);
    crypto_chk_return(ret != EOK, SYMC_COMPAT_ERRNO(ERROR_MEMCPY_S), "memcpy_s failed\n");

    symc_ctx->ctr_offset = ctr_offset;
    return TD_SUCCESS;
}
#endif

#if defined(CONFIG_SYMC_GET_CTR_BLOCK)
td_s32 inner_drv_symc_get_ctr_block(td_handle symc_handle, td_u8 *block, td_u32 block_size, td_u32 *ctr_offset)
{
    td_s32 ret;
    drv_symc_context_t *symc_ctx = NULL;
    drv_symc_param_trace("symc_handle is 0x%x\n", symc_handle);
    drv_symc_param_trace("block_size is 0x%x\n", block_size);

    symc_ctx = inner_get_symc_ctx(symc_handle);
    crypto_chk_return(symc_ctx == TD_NULL, SYMC_COMPAT_ERRNO(ERROR_INVALID_HANDLE), "invalid handle\n");
    crypto_chk_return(symc_ctx->is_open == TD_FALSE, SYMC_COMPAT_ERRNO(ERROR_CTX_CLOSED), "ctx is closed\n");

    symc_null_ptr_chk(block);
    symc_null_ptr_chk(ctr_offset);
    crypto_chk_return(block_size != CRYPTO_AES_BLOCK_SIZE_IN_BYTES, SYMC_COMPAT_ERRNO(ERROR_INVALID_PARAM),
        "block_size is invalid\n");

    ret = memcpy_s(block, block_size, symc_ctx->ctr_last_block, sizeof(symc_ctx->ctr_last_block));
    crypto_chk_return(ret != EOK, SYMC_COMPAT_ERRNO(ERROR_MEMCPY_S), "memcpy_s failed\n");

    *ctr_offset = symc_ctx->ctr_offset;

    drv_symc_param_dump_trace("get block", block, block_size);
    drv_symc_param_trace("get ctr_offset is 0x%x\n", *ctr_offset);
    return TD_SUCCESS;
}
#endif

static td_s32 inner_drv_cipher_ctr_process_one_zero_block(td_u32 chn_num, drv_symc_context_t *symc_ctx)
{
    td_u32 ret;
    td_u8 *zero_block = crypto_malloc_coherent(CRYPTO_AES_BLOCK_SIZE_IN_BYTES, "zero_block");
    crypto_chk_return(zero_block == TD_NULL, SYMC_COMPAT_ERRNO(ERROR_MALLOC), "crypto_malloc_coherent failed\n");

    (td_void)memset_s(zero_block, CRYPTO_AES_BLOCK_SIZE_IN_BYTES, 0, CRYPTO_AES_BLOCK_SIZE_IN_BYTES);
#if defined(CRYPTO_CTR_TRACE_ENABLE)
    crypto_dump_data("zero_block_before_iv", symc_ctx->iv, sizeof(symc_ctx->iv));
#endif
    ret = hal_cipher_symc_set_iv(chn_num, symc_ctx->iv, sizeof(symc_ctx->iv));
    crypto_chk_goto(ret != TD_SUCCESS, exit_free, "hal_cipher_symc_set_iv failed\n");

    ret = inner_drv_symc_common_process(symc_ctx, crypto_get_phys_addr(zero_block), crypto_get_phys_addr(zero_block),
        CRYPTO_AES_BLOCK_SIZE_IN_BYTES, IN_NODE_TYPE_FIRST | IN_NODE_TYPE_LAST);
    crypto_chk_goto(ret != TD_SUCCESS, exit_free, "inner_drv_symc_common_process failed\n");

    ret = memcpy_s(symc_ctx->ctr_last_block, sizeof(symc_ctx->ctr_last_block),
        zero_block, CRYPTO_AES_BLOCK_SIZE_IN_BYTES);
    crypto_chk_goto_with_ret(ret, ret != EOK, exit_free, SYMC_COMPAT_ERRNO(ERROR_MEMCPY_S), "memcpy_s failed\n");
#if defined(CRYPTO_CTR_TRACE_ENABLE)
    crypto_dump_data("zero_block_after_iv", symc_ctx->iv, sizeof(symc_ctx->iv));
    crypto_dump_data("zero_block_after", zero_block, CRYPTO_AES_BLOCK_SIZE_IN_BYTES);
#endif
exit_free:
    crypto_free_coherent(zero_block);
    return ret;
}

td_s32 inner_drv_cipher_symc_ctr_process_non_align(drv_symc_context_t *symc_ctx,
    td_u64 src_phys_addr, td_u64 dst_phys_addr, td_u32 length)
{
    td_s32 ret;
    td_u32 processing_length = length;
    td_u32 chn_num = symc_ctx->chn_num;
    if (symc_ctx->ctr_offset != 0) {
        symc_ctx->ctr_used = crypto_min(CRYPTO_AES_BLOCK_SIZE_IN_BYTES - symc_ctx->ctr_offset, length);
        ret = crypto_virt_xor_phys_copy_to_phys(dst_phys_addr, symc_ctx->ctr_last_block + symc_ctx->ctr_offset,
            src_phys_addr, symc_ctx->ctr_used);
        crypto_chk_return(ret != TD_SUCCESS, ret, "crypto_virt_xor_copy_to_phys failed\n");
        src_phys_addr += symc_ctx->ctr_used;
        dst_phys_addr += symc_ctx->ctr_used;
        crypto_log_dbg("ctr skip %d bytes\n", symc_ctx->ctr_used);
        if (length == symc_ctx->ctr_used) {
            symc_ctx->ctr_offset = (symc_ctx->ctr_offset + symc_ctx->ctr_used) % CRYPTO_AES_BLOCK_SIZE_IN_BYTES;
            return TD_SUCCESS;
        }
        processing_length -= symc_ctx->ctr_used;
    }

    symc_ctx->ctr_offset = processing_length % CRYPTO_AES_BLOCK_SIZE_IN_BYTES;
    processing_length -= symc_ctx->ctr_offset;
    ret = hal_cipher_symc_set_iv(chn_num, symc_ctx->iv, sizeof(symc_ctx->iv));
    crypto_chk_return(ret != TD_SUCCESS, ret, "hal_cipher_symc_set_iv failed\n");

    if (processing_length != 0) {
        ret = inner_drv_symc_common_process(symc_ctx, src_phys_addr, dst_phys_addr, processing_length,
            IN_NODE_TYPE_FIRST | IN_NODE_TYPE_LAST);
        crypto_chk_return(ret != TD_SUCCESS, ret, "inner_drv_symc_common_process failed\n");
    }

    if (symc_ctx->ctr_offset != 0) {
        ret = inner_drv_cipher_ctr_process_one_zero_block(chn_num, symc_ctx);
        crypto_chk_return(ret != TD_SUCCESS, ret, "inner_drv_cipher_ctr_process_one_zero_block failed\n");

        crypto_log_dbg("ctr xor %d bytes, processing_length is %d, ctr_used is %d\n",
            symc_ctx->ctr_offset, processing_length, symc_ctx->ctr_used);
        ret = crypto_virt_xor_phys_copy_to_phys(dst_phys_addr + processing_length,
            symc_ctx->ctr_last_block, src_phys_addr + processing_length, symc_ctx->ctr_offset);
        crypto_chk_return(ret != TD_SUCCESS, ret, "crypto_virt_xor_copy_to_phys failed\n");
    }
    return ret;
}
#endif