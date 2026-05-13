/*
  Copyright (c), 2001-2025, Shenshu Tech. Co., Ltd.
 */
#if defined(CONFIG_SYMC_CMAC_SUPPORT) || defined(CONFIG_SYMC_CBC_MAC_SUPPORT)

#include "kapi_symc.h"
#include "kapi_inner.h"

#include "drv_symc.h"
#include "drv_symc_outer.h"
#include "crypto_drv_common.h"
#include "kapi_symc_inner.h"
static td_s32 inner_kapi_mac_process(crypto_kapi_symc_ctx *ctx, const td_u8 *data, td_u32 data_len);

static td_s32 inner_kapi_mac_start_short_term(crypto_kapi_symc_ctx *symc_ctx, const crypto_symc_mac_attr *mac_attr)
{
    td_s32 ret;
    ret = memcpy_s(&symc_ctx->mac_attr, sizeof(crypto_symc_mac_attr), mac_attr, sizeof(crypto_symc_mac_attr));
    crypto_chk_return(ret != EOK, SYMC_COMPAT_ERRNO(ERROR_MEMCPY_S), "memcpy_s failed\n");

    return TD_SUCCESS;
}

static td_s32 inner_kapi_mac_update_long_term(crypto_kapi_symc_ctx *symc_ctx,
    const crypto_buf_attr *src_buf, td_u32 length)
{
    return inner_kapi_mac_process(symc_ctx, src_buf->virt_addr, length);
}

static td_s32 inner_kapi_mac_update_short_term(crypto_kapi_symc_ctx *symc_ctx,
    const crypto_buf_attr *src_buf, td_u32 length)
{
    td_s32 ret;

    ret = drv_cipher_mac_start(&symc_ctx->drv_symc_handle, &symc_ctx->mac_attr);
    crypto_chk_return(ret != TD_SUCCESS, ret, "priv_drv_lock_mac_start failed, ret is 0x%x\n", ret);

    ret = inner_drv_symc_cbc_mac_set_ctx(symc_ctx->drv_symc_handle, symc_ctx->mac_ctx.mac,
        sizeof(symc_ctx->mac_ctx.mac));
    crypto_chk_goto(ret != TD_SUCCESS, error_symc_destroy, "inner_drv_symc_cbc_mac_set_ctx failed, ret is 0x%x\n", ret);

    ret = inner_drv_mac_set_ctx(symc_ctx->drv_symc_handle, &symc_ctx->cmac_ctx);
    crypto_chk_goto(ret != TD_SUCCESS, error_symc_destroy, "inner_drv_mac_set_ctx failed, ret is 0x%x\n", ret);

    ret = inner_kapi_mac_process(symc_ctx, src_buf->virt_addr, length);
    crypto_chk_goto(ret != TD_SUCCESS, error_symc_destroy, "inner_kapi_mac_process failed, ret is 0x%x\n", ret);

    if (symc_ctx->work_mode == CRYPTO_SYMC_WORK_MODE_CBC_MAC) {
        symc_ctx->mac_ctx.mac_length = CRYPTO_AES_BLOCK_SIZE_IN_BYTES;
        ret = drv_cipher_mac_finish(symc_ctx->drv_symc_handle, symc_ctx->mac_ctx.mac, &symc_ctx->mac_ctx.mac_length);
        crypto_chk_goto(ret != TD_SUCCESS, error_symc_destroy, "drv_cipher_mac_update failed, ret is 0x%x\n", ret);
    } else {
        ret = inner_drv_mac_get_ctx(symc_ctx->drv_symc_handle, &symc_ctx->cmac_ctx);
        crypto_chk_goto(ret != TD_SUCCESS, error_symc_destroy, "drv_cipher_mac_update failed, ret is 0x%x\n", ret);

        (td_void)drv_cipher_symc_destroy(symc_ctx->drv_symc_handle);
    }

    return ret;
error_symc_destroy:
    (td_void)drv_cipher_symc_destroy(symc_ctx->drv_symc_handle);
    return ret;
}

static td_s32 inner_kapi_mac_finish_long_term(crypto_kapi_symc_ctx *symc_ctx, td_u8 *mac, td_u32 *mac_length)
{
    td_s32 ret;
    crypto_buf_attr src_buf;

    if ((symc_ctx->work_mode == CRYPTO_SYMC_WORK_MODE_CMAC) && (symc_ctx->mac_ctx.tail_length != 0)) {
        src_buf.virt_addr = symc_ctx->dma_buf;
        ret = memcpy_s(src_buf.virt_addr, symc_ctx->dma_buf_len, symc_ctx->mac_ctx.tail, symc_ctx->mac_ctx.tail_length);
        crypto_chk_return(ret != EOK, SYMC_COMPAT_ERRNO(ERROR_MEMCPY_S), "memcpy_s failed\n");

        ret = drv_cipher_mac_update(symc_ctx->drv_symc_handle, &src_buf, symc_ctx->mac_ctx.tail_length);
        crypto_chk_return(ret != TD_SUCCESS, ret, "drv_cipher_mac_update failed\n");
    }
    ret = drv_cipher_mac_finish(symc_ctx->drv_symc_handle, mac, mac_length);
    crypto_chk_return(ret != TD_SUCCESS, ret, "priv_drv_lock_mac_finish failed\n");

    return ret;
}

static td_s32 inner_kapi_cmac_finish_short_term(crypto_kapi_symc_ctx *symc_ctx)
{
    td_s32 ret;
    crypto_buf_attr src_buf;

    ret = drv_cipher_mac_start(&symc_ctx->drv_symc_handle, &symc_ctx->mac_attr);
    crypto_chk_return(ret != TD_SUCCESS, ret, "priv_drv_lock_mac_start failed, ret is 0x%x\n", ret);

    ret = inner_drv_mac_set_ctx(symc_ctx->drv_symc_handle, &symc_ctx->cmac_ctx);
    crypto_chk_goto(ret != TD_SUCCESS, error_symc_destroy, "inner_drv_mac_set_ctx failed, ret is 0x%x\n", ret);

    if (symc_ctx->mac_ctx.tail_length != 0) {
        src_buf.virt_addr = symc_ctx->dma_buf;
        ret = memcpy_s(src_buf.virt_addr, symc_ctx->dma_buf_len, symc_ctx->mac_ctx.tail, symc_ctx->mac_ctx.tail_length);
        crypto_chk_goto_with_ret(ret, ret != EOK, error_symc_destroy,
            SYMC_COMPAT_ERRNO(ERROR_MEMCPY_S), "memcpy_s failed\n");

        ret = drv_cipher_mac_update(symc_ctx->drv_symc_handle, &src_buf, symc_ctx->mac_ctx.tail_length);
        crypto_chk_goto(ret != TD_SUCCESS, error_symc_destroy, "drv_cipher_mac_update failed\n");
    }

    symc_ctx->mac_ctx.mac_length = CRYPTO_AES_BLOCK_SIZE_IN_BYTES;
    ret = drv_cipher_mac_finish(symc_ctx->drv_symc_handle, symc_ctx->mac_ctx.mac, &symc_ctx->mac_ctx.mac_length);
    crypto_chk_goto(ret != TD_SUCCESS, error_symc_destroy, "drv_cipher_mac_update failed, ret is 0x%x\n", ret);

    return ret;
error_symc_destroy:
    (td_void)drv_cipher_symc_destroy(symc_ctx->drv_symc_handle);
    return ret;
}

static td_s32 inner_kapi_mac_finish_short_term(crypto_kapi_symc_ctx *symc_ctx, td_u8 *mac, td_u32 *mac_length)
{
    td_s32 ret;

    if (symc_ctx->work_mode == CRYPTO_SYMC_WORK_MODE_CMAC) {
        ret = inner_kapi_cmac_finish_short_term(symc_ctx);
        crypto_chk_return(ret != TD_SUCCESS, ret, "inner_kapi_cmac_finish_short_term failed\n");
    }

    ret = memcpy_s(mac, *mac_length, symc_ctx->mac_ctx.mac, symc_ctx->mac_ctx.mac_length);
    crypto_chk_return(ret != EOK, SYMC_COMPAT_ERRNO(ERROR_MEMCPY_S), "memcpy_s failed\n");

    return ret;
}

td_s32 kapi_cipher_mac_start(td_handle *kapi_symc_handle, const crypto_symc_mac_attr *mac_attr)
{
    td_s32 ret = CRYPTO_SUCCESS;
    crypto_kapi_symc_process *symc_channel = TD_NULL;
    crypto_kapi_symc_ctx *symc_ctx = TD_NULL;
    td_u32 idx;

    crypto_kapi_func_enter();
    symc_null_ptr_chk(kapi_symc_handle);
    symc_null_ptr_chk(mac_attr);

    symc_channel = inner_get_current_symc_channel();
    crypto_chk_return(symc_channel == TD_NULL, SYMC_COMPAT_ERRNO(ERROR_NOT_INIT), "call init first\n");

    symc_ctx = priv_occupy_symc_soft_chn(symc_channel, &idx);
    crypto_chk_return(symc_ctx == TD_NULL, SYMC_COMPAT_ERRNO(ERROR_CHN_BUSY), "all symc soft chns are busy\n");
    crypto_chk_return(symc_ctx->is_open == TD_FALSE, SYMC_COMPAT_ERRNO(ERROR_CTX_CLOSED), "ctx is closed\n");

    kapi_symc_lock();

    symc_ctx->dma_buf = crypto_malloc_mmz(CONFIG_SYMC_KAPI_DMA_BUF_LEN, "symc_dma_buf");
    crypto_chk_goto_with_ret(ret, symc_ctx->dma_buf == TD_NULL, error_unlock_symc_ctx,
        SYMC_COMPAT_ERRNO(ERROR_MALLOC), "crypto_malloc_mmz failed\n");
    symc_ctx->dma_buf_len = CONFIG_SYMC_KAPI_DMA_BUF_LEN;

    if (mac_attr->is_long_term == TD_TRUE) {
        ret = drv_cipher_mac_start(&symc_ctx->drv_symc_handle, mac_attr);
        crypto_chk_goto(ret != TD_SUCCESS, error_free, "inner_kapi_mac_start_long_term failed, ret is 0x%x\n", ret);
    } else {
        ret = inner_kapi_mac_start_short_term(symc_ctx, mac_attr);
        crypto_chk_goto(ret != TD_SUCCESS, error_free, "inner_kapi_mac_start_short_term failed, ret is 0x%x\n", ret);
    }
    symc_ctx->is_mac = TD_TRUE;
    symc_ctx->is_long_term = mac_attr->is_long_term;
    symc_ctx->work_mode = mac_attr->work_mode;
    kapi_symc_unlock();
    *kapi_symc_handle = synthesize_kapi_handle(KAPI_SYMC_MODULE_ID, idx);
    crypto_kapi_func_exit();
    return ret;

error_free:
    crypto_free_coherent(symc_ctx->dma_buf);
error_unlock_symc_ctx:
    kapi_symc_unlock();
    priv_release_symc_soft_chn(symc_ctx);
    crypto_kapi_func_exit();
    return ret;
}
CRYPTO_EXPORT_SYMBOL(kapi_cipher_mac_start);

static td_s32 inner_kapi_mac_process(crypto_kapi_symc_ctx *ctx, const td_u8 *data, td_u32 data_len)
{
    td_s32 ret;
    td_u8 *dma_buf = ctx->dma_buf;
    td_u32 dma_buf_len = ctx->dma_buf_len;
    td_u8 *tail = ctx->mac_ctx.tail;
    td_u32 tail_len = ctx->mac_ctx.tail_length;
    td_u32 block_size = sizeof(ctx->mac_ctx.tail);
    td_u32 processing_len = 0;
    td_u32 processed_len = 0;
    td_u32 left = data_len;
    crypto_buf_attr src_buf;

    src_buf.virt_addr = dma_buf;
    if ((tail_len + data_len) < block_size) {
        ret = memcpy_s(tail + tail_len, block_size - tail_len, data, data_len);
        crypto_chk_return(ret != EOK, SYMC_COMPAT_ERRNO(ERROR_MEMCPY_S), "memcpy_s failed\n");
        ctx->mac_ctx.tail_length += data_len;
        return CRYPTO_SUCCESS;
    }
    if (tail_len != 0) {
        processing_len = block_size - tail_len;
        ret = memcpy_s(tail + tail_len, processing_len, data, processing_len);
        crypto_chk_return(ret != EOK, SYMC_COMPAT_ERRNO(ERROR_MEMCPY_S), "memcpy_s failed\n");
        ret = memcpy_s(dma_buf, block_size, tail, block_size);
        crypto_chk_return(ret != EOK, SYMC_COMPAT_ERRNO(ERROR_MEMCPY_S), "memcpy_s failed\n");

        ret = drv_cipher_mac_update(ctx->drv_symc_handle, &src_buf, block_size);
        crypto_chk_return(ret != TD_SUCCESS, ret, "drv_cipher_mac_update failed\n");

        left -= processing_len;
        processed_len += processing_len;
    }
    while (left >= block_size) {
        if (left >= dma_buf_len) {
            processing_len = dma_buf_len;
        } else {
            processing_len = left - left % block_size;
        }
        ret = memcpy_s(dma_buf, dma_buf_len, data + processed_len, processing_len);
        crypto_chk_return(ret != EOK, SYMC_COMPAT_ERRNO(ERROR_MEMCPY_S), "memcpy_s failed\n");

        ret = drv_cipher_mac_update(ctx->drv_symc_handle, &src_buf, processing_len);
        crypto_chk_return(ret != TD_SUCCESS, ret, "drv_cipher_mac_update failed\n");

        left -= processing_len;
        processed_len += processing_len;
    }
    if (left != 0) {
        ret = memcpy_s(tail, block_size, data + processed_len, left);
        crypto_chk_return(ret != EOK, SYMC_COMPAT_ERRNO(ERROR_MEMCPY_S), "memcpy_s failed\n");
    }
    ctx->mac_ctx.tail_length = left;

    return ret;
}

td_s32 kapi_cipher_mac_update(td_handle kapi_symc_handle, const crypto_buf_attr *src_buf, td_u32 length)
{
    td_s32 ret;
    td_u32 idx = 0;
    crypto_kapi_symc_process *symc_channel = TD_NULL;
    crypto_kapi_symc_ctx *symc_ctx = TD_NULL;
    crypto_kapi_func_enter();

    symc_channel = inner_get_current_symc_channel();
    crypto_chk_return(symc_channel == TD_NULL, SYMC_COMPAT_ERRNO(ERROR_NOT_INIT), "call init first\n");

    symc_ctx = inner_kapi_get_symc_ctx(kapi_symc_handle, &idx);
    crypto_chk_return(symc_ctx == TD_NULL, SYMC_COMPAT_ERRNO(ERROR_CHN_BUSY), "all symc soft chns are busy\n");
    crypto_chk_return(symc_ctx->is_open == TD_FALSE, SYMC_COMPAT_ERRNO(ERROR_CTX_CLOSED), "ctx is closed\n");
    crypto_chk_return(symc_ctx->is_mac == TD_FALSE, SYMC_COMPAT_ERRNO(ERROR_NOT_MAC_START), "call mac_start first\n");

    kapi_symc_lock();

    if (symc_ctx->is_long_term == TD_TRUE) {
        ret = inner_kapi_mac_update_long_term(symc_ctx, src_buf, length);
        crypto_chk_goto(ret != TD_SUCCESS, unlock_exit, "inner_kapi_mac_update_long_term failed, ret is 0x%x\n", ret);
    } else {
        ret = inner_kapi_mac_update_short_term(symc_ctx, src_buf, length);
        crypto_chk_goto(ret != TD_SUCCESS, unlock_exit, "inner_kapi_mac_update_short_term failed, ret is 0x%x\n", ret);
    }

    kapi_symc_unlock();
    crypto_kapi_func_exit();
    return ret;

unlock_exit:
    if (symc_ctx->dma_buf != TD_NULL) {
        crypto_free_coherent(symc_ctx->dma_buf);
        symc_ctx->dma_buf = TD_NULL;
    }
    kapi_symc_unlock();
    crypto_kapi_func_exit();
    return ret;
}
CRYPTO_EXPORT_SYMBOL(kapi_cipher_mac_update);

td_s32 kapi_cipher_mac_finish(td_handle kapi_symc_handle, td_u8 *mac, td_u32 *mac_length)
{
    td_s32 ret;
    td_u32 idx = 0;
    crypto_kapi_symc_process *symc_channel = TD_NULL;
    crypto_kapi_symc_ctx *symc_ctx = TD_NULL;
    crypto_kapi_func_enter();

    symc_null_ptr_chk(mac);
    symc_null_ptr_chk(mac_length);

    crypto_chk_return(*mac_length < CRYPTO_AES_BLOCK_SIZE_IN_BYTES, SYMC_COMPAT_ERRNO(ERROR_INVALID_PARAM),
        "mac_length is not enough\n");

    symc_channel = inner_get_current_symc_channel();
    crypto_chk_return(symc_channel == TD_NULL, SYMC_COMPAT_ERRNO(ERROR_NOT_INIT), "call init first\n");

    symc_ctx = inner_kapi_get_symc_ctx(kapi_symc_handle, &idx);
    crypto_chk_return(symc_ctx == TD_NULL, SYMC_COMPAT_ERRNO(ERROR_CHN_BUSY), "all symc soft chns are busy\n");
    crypto_chk_return(symc_ctx->is_open == TD_FALSE, SYMC_COMPAT_ERRNO(ERROR_CTX_CLOSED), "ctx is closed\n");
    crypto_chk_return(symc_ctx->is_mac == TD_FALSE, SYMC_COMPAT_ERRNO(ERROR_NOT_MAC_START), "call mac_start first\n");

    kapi_symc_lock();

    if (symc_ctx->is_long_term == TD_TRUE) {
        ret = inner_kapi_mac_finish_long_term(symc_ctx, mac, mac_length);
        crypto_chk_goto(ret != TD_SUCCESS, error_exit, "inner_kapi_mac_finish_long_term failed, ret is 0x%x\n", ret);
    } else {
        ret = inner_kapi_mac_finish_short_term(symc_ctx, mac, mac_length);
        crypto_chk_goto(ret != TD_SUCCESS, error_exit, "inner_kapi_mac_finish_short_term failed, ret is 0x%x\n", ret);
    }

error_exit:
    if (symc_ctx->dma_buf != TD_NULL) {
        crypto_free_coherent(symc_ctx->dma_buf);
        symc_ctx->dma_buf = TD_NULL;
    }
    kapi_symc_unlock();
    priv_release_symc_soft_chn(symc_ctx);
    crypto_kapi_func_exit();
    return ret;
}
CRYPTO_EXPORT_SYMBOL(kapi_cipher_mac_finish);
#endif