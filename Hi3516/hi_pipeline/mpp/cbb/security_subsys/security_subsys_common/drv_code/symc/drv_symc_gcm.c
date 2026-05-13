/**
  Copyright (c), 2001-2025, Shenshu Tech. Co., Ltd.
 */
#include "drv_symc.h"
#include "drv_symc_inner.h"
#include "drv_symc_outer.h"
#include "hal_symc.h"
#include "crypto_drv_common.h"

#if defined(CONFIG_SYMC_GCM_SUPPORT)

static td_s32 inner_drv_symc_gcm_process_aad(drv_symc_context_t *symc_ctx, td_u64 aad_phys_addr, td_u32 aad_len);

static int32_t inner_drv_cipher_symc_gcm_ghash_start(drv_symc_context_t *symc_ctx)
{
    symc_ctx->ghash_tail_len = 0;
    (void)memset_s(symc_ctx->ghash, sizeof(symc_ctx->ghash), 0, sizeof(symc_ctx->ghash));
    return CRYPTO_SUCCESS;
}

static int32_t inner_drv_cipher_symc_gcm_ghash_update(drv_symc_context_t *symc_ctx,
    uint64_t data_phys_addr, uint32_t data_len)
{
    int32_t ret;
    uint32_t left = data_len;
    uint32_t processed_len = 0;
    uint32_t processing_len = 0;
    uint32_t tail_len = symc_ctx->ghash_tail_len;
    uint32_t tail_buf_len = sizeof(symc_ctx->ghash_tail);
    uint8_t *ghash = symc_ctx->ghash;
    uint32_t ghash_len = sizeof(symc_ctx->ghash);
    uint8_t *ghash_buf = NULL;
    uint32_t ghash_buf_len = CRYPTO_AES_BLOCK_SIZE_IN_BYTES;

    if (data_len == 0) {
        return CRYPTO_SUCCESS;
    }

#if defined(CONFIG_SYMC_GCM_TRACE_ENABLE)
    crypto_dump_data("ghash before", ghash, ghash_len);
    crypto_dump_phys_addr("ghash data", data_phys_addr, data_len);
#endif

    ghash_buf = crypto_malloc_coherent(ghash_buf_len, "ghash_buf");
    crypto_chk_return(ghash_buf == NULL, SYMC_COMPAT_ERRNO(ERROR_MALLOC), "crypto_malloc_coherent failed\n");

    processing_len = crypto_min(tail_buf_len - tail_len, data_len);
    ret = crypto_copy_from_phys_addr(symc_ctx->ghash_tail + tail_len, tail_buf_len - tail_len,
        data_phys_addr, processing_len);
    crypto_chk_goto(ret != CRYPTO_SUCCESS, exit_free, "crypto_copy_from_phys_addr failed\n");
    symc_ctx->ghash_tail_len += processing_len;

    /* process one block. */
    if (symc_ctx->ghash_tail_len == tail_buf_len) {
        ret = memcpy_s(ghash_buf, ghash_buf_len, symc_ctx->ghash_tail, tail_buf_len);
        crypto_chk_goto_with_ret(ret, ret != EOK, exit_free, SYMC_COMPAT_ERRNO(ERROR_MEMCPY_S), "memcpy_s failed\n");

        ret = hal_cipher_gcm_ghash_update(symc_ctx->chn_num, crypto_get_phys_addr(ghash_buf),
            tail_buf_len, ghash, ghash_len);
        crypto_chk_goto(ret != CRYPTO_SUCCESS, exit_free, "hal_cipher_gcm_ghash_update failed\n");
        symc_ctx->ghash_tail_len = 0;
    }

    left -= processing_len;
    processed_len += processing_len;
    if (left == 0) {
#if defined(CONFIG_SYMC_GCM_TRACE_ENABLE)
        crypto_dump_data("ghash after", ghash, ghash_len);
#endif
        ret = CRYPTO_SUCCESS;
        goto exit_free;
    }
    /* process blocks. */
    while (left >= tail_buf_len) {
        ret = crypto_copy_from_phys_addr(ghash_buf, ghash_buf_len, data_phys_addr + processed_len, tail_buf_len);
        crypto_chk_goto(ret != EOK, exit_free, "crypto_copy_from_phys_addr failed\n");

        ret = hal_cipher_gcm_ghash_update(symc_ctx->chn_num, crypto_get_phys_addr(ghash_buf), tail_buf_len,
            ghash, ghash_len);
        crypto_chk_goto(ret != CRYPTO_SUCCESS, exit_free, "hal_cipher_gcm_ghash_update failed\n");
        left -= tail_buf_len;
        processed_len += tail_buf_len;
    }
    if (left != 0) {
        ret = crypto_copy_from_phys_addr(symc_ctx->ghash_tail, tail_buf_len, data_phys_addr + processed_len, left);
        crypto_chk_goto(ret != EOK, exit_free, "crypto_copy_from_phys_addr failed\n");
    }
    symc_ctx->ghash_tail_len = left;

#if defined(CONFIG_SYMC_GCM_TRACE_ENABLE)
    crypto_dump_data("ghash after", ghash, ghash_len);
#endif

    ret = TD_SUCCESS;
exit_free:
    crypto_free_coherent(ghash_buf);
    return ret;
}

static int32_t inner_drv_cipher_symc_gcm_ghash_finish(drv_symc_context_t *symc_ctx,
    uint8_t *ghash, uint32_t ghash_len)
{
    int32_t ret;
    if (symc_ctx->ghash_tail_len != 0) {
        crypto_log_err("ghash data is not aligned to 16-Byte\n");
        return SYMC_COMPAT_ERRNO(ERROR_HASH_LEN_NOT_ALIGNED);
    }
    ret = memcpy_s(ghash, ghash_len, symc_ctx->ghash, sizeof(symc_ctx->ghash));
    crypto_chk_return(ret != EOK, SYMC_COMPAT_ERRNO(ERROR_MEMCPY_S), "memcpy_s failed\n");

    return CRYPTO_SUCCESS;
}

static int32_t inner_drv_cipher_symc_gcm_compute_j0(drv_symc_context_t *symc_ctx,
    const uint8_t *gcm_iv, uint32_t gcm_iv_len)
{
    int32_t ret;
    uint8_t iv_padding[CRYPTO_AES_BLOCK_SIZE_IN_BYTES * 2];
    uint8_t iv_padding_len = sizeof(iv_padding);
    uint32_t iv_len_in_bit = gcm_iv_len * CRYPTO_BITS_IN_BYTE;
    uint8_t *gcm_j0_buf = TD_NULL;
    uint8_t buf_len = CRYPTO_AES_BLOCK_SIZE_IN_BYTES * 2;
    crypto_hal_func_enter();

    gcm_j0_buf = crypto_malloc_coherent(buf_len, "gcm_j0_buf");
    crypto_chk_return(gcm_j0_buf == NULL, SYMC_COMPAT_ERRNO(ERROR_MALLOC), "crypto_malloc_coherent failed\n");

    iv_len_in_bit = crypto_cpu_to_be32(iv_len_in_bit);

    (void)memset_s(symc_ctx->j0, sizeof(symc_ctx->j0), 0, sizeof(symc_ctx->j0));
    /* If the IV length is 12 bytes, J0 = IV || 00000000 00000000 00000000 00000001 */
    if (gcm_iv_len == CRYPTO_GCM_SPECIAL_IV_BYTES) { // for 96 bits, J0 = IV || 00000001
        ret = memcpy_s(symc_ctx->j0, sizeof(symc_ctx->j0), gcm_iv, gcm_iv_len);
        crypto_chk_goto_with_ret(ret, ret != EOK, exit_free, SYMC_COMPAT_ERRNO(ERROR_MEMCPY_S), "memcpy_s failed\n");
        symc_ctx->j0[sizeof(symc_ctx->j0) - 1] = 0x01;  // j0[15] = 0x01
    } else { /* If the IV length is not 12 bytes, J0 = GHASH{IV || [00..00] || len(IV)} */
        iv_padding_len = iv_padding_len - gcm_iv_len % CRYPTO_AES_BLOCK_SIZE_IN_BYTES;
        (void)memset_s(iv_padding, sizeof(iv_padding), 0, sizeof(iv_padding));
        ret = memcpy_s(iv_padding + iv_padding_len - sizeof(td_u32), sizeof(td_u32), &iv_len_in_bit, sizeof(td_u32));
        crypto_chk_goto_with_ret(ret, ret != EOK, exit_free, SYMC_COMPAT_ERRNO(ERROR_MEMCPY_S), "memcpy_s failed\n");

        ret = inner_drv_cipher_symc_gcm_ghash_start(symc_ctx);
        crypto_chk_goto(ret != CRYPTO_SUCCESS, exit_free, "inner_drv_cipher_symc_gcm_ghash_start failed\n");

        ret = memcpy_s(gcm_j0_buf, buf_len, gcm_iv, gcm_iv_len);
        crypto_chk_goto_with_ret(ret, ret != EOK, exit_free, SYMC_COMPAT_ERRNO(ERROR_MEMCPY_S), "memcpy_s failed\n");

        ret = inner_drv_cipher_symc_gcm_ghash_update(symc_ctx, crypto_get_phys_addr(gcm_j0_buf), gcm_iv_len);
        crypto_chk_goto(ret != CRYPTO_SUCCESS, exit_free, "inner_drv_cipher_symc_gcm_ghash_update failed\n");

        ret = memcpy_s(gcm_j0_buf, buf_len, iv_padding, iv_padding_len);
        crypto_chk_goto_with_ret(ret, ret != EOK, exit_free, SYMC_COMPAT_ERRNO(ERROR_MEMCPY_S), "memcpy_s failed\n");

        ret = inner_drv_cipher_symc_gcm_ghash_update(symc_ctx, crypto_get_phys_addr(gcm_j0_buf), iv_padding_len);
        crypto_chk_goto(ret != CRYPTO_SUCCESS, exit_free, "inner_drv_cipher_symc_gcm_ghash_update failed\n");
        ret = inner_drv_cipher_symc_gcm_ghash_finish(symc_ctx, symc_ctx->j0, sizeof(symc_ctx->j0));
        crypto_chk_goto(ret != CRYPTO_SUCCESS, exit_free, "inner_drv_cipher_symc_gcm_ghash_finish failed\n");
    }

#if defined(CONFIG_SYMC_GCM_TRACE_ENABLE)
    crypto_dump_data("gcm j0", symc_ctx->j0, sizeof(symc_ctx->j0));
#endif
    ret = TD_SUCCESS;
exit_free:
    crypto_free_coherent(gcm_j0_buf);
    crypto_hal_func_exit();
    return ret;
}

static td_s32 inner_symc_gcm_cfg_param_check(const crypto_symc_ctrl_t *symc_ctrl)
{
    crypto_symc_work_mode mode = symc_ctrl->work_mode;
    crypto_symc_config_aes_ccm_gcm *ccm_gcm_param = (crypto_symc_config_aes_ccm_gcm *)symc_ctrl->param;
    td_u32 iv_length = symc_ctrl->iv_length;
    td_u32 tag_len;

#if !defined(CONFIG_SYMC_SM4_CCM_GCM_SUPPORT)
        crypto_chk_return(symc_ctrl->symc_alg == CRYPTO_SYMC_ALG_SM4,
            SYMC_COMPAT_ERRNO(ERROR_UNSUPPORT), "sm4 unsupport this mode\n");
#endif

    /* Check for GCM. */
    if (mode == CRYPTO_SYMC_WORK_MODE_GCM && symc_ctrl->iv_change_flag == CRYPTO_SYMC_GCM_IV_CHANGE_START) {
        crypto_chk_return(ccm_gcm_param == TD_NULL, SYMC_COMPAT_ERRNO(ERROR_PARAM_IS_NULL), "param is NULL\n");
        tag_len = ccm_gcm_param->tag_len;
        /* IV lenght for GCM, which should be 1~16. */
        crypto_chk_return(iv_length == 0 || iv_length > CRYPTO_IV_LEN_IN_BYTES, SYMC_COMPAT_ERRNO(ERROR_INVALID_PARAM),
            "iv_len must be 1~16 for GCM.\n");
        /* Tag length for GCM should be 1~16.. */
        crypto_chk_return(tag_len == 0 || tag_len > CRYPTO_AES_MAX_TAG_SIZE, SYMC_COMPAT_ERRNO(ERROR_INVALID_PARAM),
            "tag_len must be 1~16 for GCM.\n");
    }
    return TD_SUCCESS;
}

static void inner_gcm_j0_plus_one(uint8_t *j0, uint32_t j0_len)
{
    uint32_t value = CRYPTO_BYTE_MAX + 1;
    int32_t i = j0_len - 1;

    while (value > CRYPTO_BYTE_MAX && i >= 0) {
        value = j0[i] + 1;
        j0[i] = value % CRYPTO_BYTE_MAX;
        j0--;
    }
}

td_s32 drv_cipher_symc_gcm_set_config(drv_symc_context_t *symc_ctx, const crypto_symc_ctrl_t *symc_ctrl)
{
    td_s32 ret;
    hal_symc_config_t hal_symc_config = {0};
    crypto_symc_config_aes_ccm_gcm *gcm_config = TD_NULL;

    symc_null_ptr_chk(symc_ctx);
    symc_null_ptr_chk(symc_ctrl);

    ret = inner_symc_gcm_cfg_param_check(symc_ctrl);
    crypto_chk_return(ret != TD_SUCCESS, ret, "inner_symc_gcm_cfg_param_check failed\n");

    hal_symc_config.symc_alg = symc_ctrl->symc_alg;
    hal_symc_config.work_mode = symc_ctrl->work_mode;
    hal_symc_config.symc_bit_width = symc_ctrl->symc_bit_width;
    hal_symc_config.symc_key_length = symc_ctrl->symc_key_length;
    hal_symc_config.iv_change_flag = symc_ctrl->iv_change_flag;
    ret = hal_cipher_symc_config(symc_ctx->chn_num, &hal_symc_config);
    crypto_chk_return(ret != TD_SUCCESS, ret, "hal_cipher_symc_config failed, ret is 0x%x\n", ret);

    symc_ctx->symc_alg = symc_ctrl->symc_alg;
    symc_ctx->work_mode = symc_ctrl->work_mode;
    symc_ctx->symc_bit_width = symc_ctrl->symc_bit_width;
    symc_ctx->symc_key_length = symc_ctrl->symc_key_length;

    if (symc_ctrl->iv_change_flag == CRYPTO_SYMC_GCM_IV_CHANGE_FINISH) {
        ret = hal_cipher_symc_set_iv(symc_ctx->chn_num, symc_ctrl->iv, sizeof(symc_ctrl->iv));
        crypto_chk_return(ret != TD_SUCCESS, ret, "hal_cipher_symc_set_iv failed, ret is 0x%x\n", ret);
        symc_ctx->is_config = TD_TRUE;
        return CRYPTO_SUCCESS;
    }

    gcm_config = (crypto_symc_config_aes_ccm_gcm *)symc_ctrl->param;
    crypto_chk_return(gcm_config == TD_NULL, SYMC_COMPAT_ERRNO(ERROR_INVALID_PARAM),
        "gcm_config is NULL\n");
    crypto_chk_return(gcm_config->aad_len > CRYPTO_SYMC_AAD_MAX_SIZE, SYMC_COMPAT_ERRNO(ERROR_INVALID_PARAM),
        "aad_len is too long\n");

    if (symc_ctrl->iv_change_flag == CRYPTO_SYMC_GCM_IV_CHANGE_START) {
        symc_ctx->total_aad_len = gcm_config->total_aad_len;
        symc_ctx->data_len = gcm_config->data_len;
        symc_ctx->processed_aad_len = 0;
        symc_ctx->processed_len = 0;

        /* compute gcm j0 and iv. */
        ret = inner_drv_cipher_symc_gcm_compute_j0(symc_ctx, symc_ctrl->iv, symc_ctrl->iv_length);
        crypto_chk_return(ret != EOK, ret, "inner_drv_cipher_symc_gcm_compute_j0 failed, ret is 0x%x\n", ret);

        ret = memcpy_s(symc_ctx->iv, sizeof(symc_ctx->iv), symc_ctx->j0, sizeof(symc_ctx->j0));
        crypto_chk_return(ret != EOK, SYMC_COMPAT_ERRNO(ERROR_MEMCPY_S), "memcpy_s failed, ret is 0x%x\n", ret);

        inner_gcm_j0_plus_one(symc_ctx->iv, sizeof(symc_ctx->iv));
    }
    /* GCM ADD AAD. */
    crypto_log_dbg("gcm start process aad\n");
    ret = inner_drv_symc_gcm_process_aad(symc_ctx, gcm_config->aad_buf.phys_addr, gcm_config->aad_len);
    crypto_chk_return(ret != TD_SUCCESS, ret, "inner_drv_symc_gcm_process_aad failed, ret is 0x%x\n", ret);

    symc_ctx->is_config = TD_TRUE;
    return TD_SUCCESS;
}

static td_s32 inner_drv_symc_gcm_process_aad(drv_symc_context_t *symc_ctx, td_u64 aad_phys_addr, td_u32 aad_len)
{
    td_s32 ret = TD_SUCCESS;
    uint32_t aad_padding_len = 0;
    uint8_t *aad_padding = NULL;

    aad_padding = crypto_malloc_coherent(CRYPTO_AES_BLOCK_SIZE_IN_BYTES, "aad_padding_buf");
    crypto_chk_return(aad_padding == NULL, SYMC_COMPAT_ERRNO(ERROR_MALLOC), "crypto_malloc_coherent failed\n");

    if (symc_ctx->processed_aad_len == 0) {
        ret = inner_drv_cipher_symc_gcm_ghash_start(symc_ctx);
        crypto_chk_goto(ret != TD_SUCCESS, exit_free,
            "inner_drv_cipher_symc_gcm_ghash_start failed, ret is 0x%x\n", ret);
    }

    /* GCM ADD AAD. */
    if (aad_len != 0) {
#if defined(CONFIG_SYMC_GCM_TRACE_ENABLE)
        crypto_dump_phys_addr("gcm aad", aad_phys_addr, aad_len);
#endif
        ret = inner_drv_cipher_symc_gcm_ghash_update(symc_ctx, aad_phys_addr, aad_len);
        crypto_chk_goto(ret != TD_SUCCESS, exit_free,
            "inner_drv_cipher_symc_gcm_ghash_update failed, ret is 0x%x\n", ret);
    }
    if (symc_ctx->processed_aad_len + aad_len == symc_ctx->total_aad_len) {
        aad_padding_len = CRYPTO_AES_BLOCK_SIZE_IN_BYTES - symc_ctx->total_aad_len % CRYPTO_AES_BLOCK_SIZE_IN_BYTES;
        if (aad_padding_len != CRYPTO_AES_BLOCK_SIZE_IN_BYTES) {
            /* padding must be zeros. */
            (td_void)memset_s(aad_padding, aad_padding_len, 0, aad_padding_len);
#if defined(CONFIG_SYMC_GCM_TRACE_ENABLE)
            crypto_dump_data("gcm aad_padding", aad_padding, aad_padding_len);
#endif
            ret = inner_drv_cipher_symc_gcm_ghash_update(symc_ctx, crypto_get_phys_addr(aad_padding), aad_padding_len);
            crypto_chk_goto(ret != TD_SUCCESS, exit_free,
                "inner_drv_cipher_symc_gcm_ghash_update failed, ret is 0x%x\n", ret);
        }
    }
    symc_ctx->processed_aad_len += aad_len;

exit_free:
    crypto_free_coherent(aad_padding);
    return ret;
}

static td_s32 inner_drv_symc_gcm_process_P(drv_symc_context_t *symc_ctx,
    td_ulong src_phys_addr, td_ulong dst_phys_addr, td_u32 length)
{
    td_s32 ret = TD_SUCCESS;
    td_u32 gcm_p_node_type = 0;

    /* GCM ADD P. */
    if (length != 0) {
        gcm_p_node_type = IN_NODE_TYPE_GCM_P | IN_NODE_TYPE_FIRST | IN_NODE_TYPE_LAST;
        if (symc_ctx->total_aad_len == 0) {
            gcm_p_node_type |= IN_NODE_TYPE_GCM_FIRST;
        }
        ret = inner_drv_symc_common_process(symc_ctx, src_phys_addr, dst_phys_addr, length, gcm_p_node_type);
        crypto_chk_return(ret != TD_SUCCESS, ret, "inner_drv_symc_common_process failed, ret is 0x%x\n", ret);
    }
    symc_ctx->processed_len += length;
    return ret;
}

static td_s32 inner_drv_symc_gcm_process_clen(drv_symc_context_t *symc_ctx)
{
    td_s32 ret = TD_SUCCESS;
    td_u32 aad_len_in_bit = 0;
    td_u32 data_len_in_bit = 0;
    td_u8 *clen_buf = TD_NULL;
    td_u32 clen_buf_len = CRYPTO_AES_BLOCK_SIZE_IN_BYTES;

    clen_buf = crypto_malloc_coherent(clen_buf_len, "clen_buf");
    crypto_chk_return(clen_buf == TD_NULL, SYMC_COMPAT_ERRNO(ERROR_MALLOC), "crypto_malloc_coherent failed\n");

    /* GCM ADD LEN. */
    aad_len_in_bit = crypto_cpu_to_be32(symc_ctx->processed_aad_len * CRYPTO_BITS_IN_BYTE);
    data_len_in_bit = crypto_cpu_to_be32(symc_ctx->processed_len * CRYPTO_BITS_IN_BYTE);
    (td_void)memset_s(clen_buf, clen_buf_len, 0x00, clen_buf_len);
    (td_void)memcpy_s(clen_buf + 8 - sizeof(td_u32),        /* 8: bit15~8 for aad_len. */
        sizeof(td_u32), &aad_len_in_bit, sizeof(td_u32));
    (td_void)memcpy_s(clen_buf + 16 - sizeof(td_u32), sizeof(td_u32),   /* 16: bit7~0 for data_len. */
        &data_len_in_bit, sizeof(td_u32));

#if defined(CONFIG_SYMC_GCM_TRACE_ENABLE)
    crypto_dump_data("gcm_clen_buf", clen_buf, clen_buf_len);
#endif
    ret = inner_drv_cipher_symc_gcm_ghash_update(symc_ctx, crypto_get_phys_addr(clen_buf), clen_buf_len);
    crypto_chk_goto(ret != TD_SUCCESS, exit_free, "inner_drv_cipher_symc_gcm_ghash_update failed, ret is 0x%x\n", ret);

exit_free:
    crypto_free_coherent(clen_buf);
    return ret;
}

td_s32 inner_drv_cipher_symc_gcm_process(drv_symc_context_t *symc_ctx, td_ulong src_phys_addr,
    td_ulong dst_phys_addr, td_u32 length)
{
    td_s32 ret;
    td_u8 p_padding_len = CRYPTO_AES_BLOCK_SIZE_IN_BYTES;
    td_u8 *p_padding = NULL;
    td_u8 *gcm_buf = NULL;
    hal_symc_config_t symc_config = {0};

    symc_null_ptr_chk(symc_ctx);
    gcm_buf = crypto_malloc_coherent(p_padding_len, "gcm_buf");
    crypto_chk_return(gcm_buf == NULL, SYMC_COMPAT_ERRNO(ERROR_MALLOC), "crypto_malloc_coherent failed\n");
    p_padding = gcm_buf;

#if defined(CONFIG_SYMC_GCM_TRACE_ENABLE)
    crypto_dump_phys_addr("gcm_buf", crypto_get_phys_addr(gcm_buf), p_padding_len);
    crypto_dump_phys_addr("p_padding", crypto_get_phys_addr(p_padding), p_padding_len);
#endif
    (void)memset_s(p_padding, CRYPTO_AES_BLOCK_SIZE_IN_BYTES, 0, CRYPTO_AES_BLOCK_SIZE_IN_BYTES);
    if (length % CRYPTO_AES_BLOCK_SIZE_IN_BYTES == 0) {
        p_padding_len = 0;
    } else {
        p_padding_len = CRYPTO_AES_BLOCK_SIZE_IN_BYTES - length % CRYPTO_AES_BLOCK_SIZE_IN_BYTES;
    }
    if (symc_ctx->is_decrypt == TD_TRUE) { /* Decrypt, ghash update the cipher text. */
        ret = inner_drv_cipher_symc_gcm_ghash_update(symc_ctx, src_phys_addr, length);
        crypto_chk_goto(ret != TD_SUCCESS, exit_free,
            "inner_drv_cipher_symc_gcm_ghash_update failed, ret is 0x%x\n", ret);

        ret = inner_drv_cipher_symc_gcm_ghash_update(symc_ctx, crypto_get_phys_addr(p_padding), p_padding_len);
        crypto_chk_goto(ret != TD_SUCCESS, exit_free,
            "inner_drv_cipher_symc_gcm_ghash_update failed, ret is 0x%x\n", ret);
    }

    symc_config.symc_alg = symc_ctx->symc_alg;
    symc_config.work_mode = symc_ctx->work_mode;
    symc_config.symc_key_length = symc_ctx->symc_key_length;
    symc_config.symc_bit_width = symc_ctx->symc_bit_width;

    ret = hal_cipher_symc_config(symc_ctx->chn_num, &symc_config);
    crypto_chk_goto(ret != TD_SUCCESS, exit_free, "hal_cipher_symc_config failed\n");

    /* GCM ADD P. */
    crypto_log_dbg("gcm start process p\n");
    ret = inner_drv_symc_gcm_process_P(symc_ctx, src_phys_addr, dst_phys_addr, length);
    crypto_chk_goto(ret != TD_SUCCESS, exit_free, "inner_drv_symc_gcm_process_P failed, ret is 0x%x\n", ret);

    if (symc_ctx->is_decrypt == TD_FALSE) { /* Encrypt, ghash update the cipher text. */
        ret = inner_drv_cipher_symc_gcm_ghash_update(symc_ctx, dst_phys_addr, length);
        crypto_chk_goto(ret != TD_SUCCESS, exit_free,
            "inner_drv_cipher_symc_gcm_ghash_update failed, ret is 0x%x\n", ret);

        ret = inner_drv_cipher_symc_gcm_ghash_update(symc_ctx, crypto_get_phys_addr(p_padding), p_padding_len);
        crypto_chk_goto(ret != TD_SUCCESS, exit_free,
            "inner_drv_cipher_symc_gcm_ghash_update failed, ret is 0x%x\n", ret);
    }

exit_free:
    crypto_free_coherent(gcm_buf);
    return ret;
}

td_s32 inner_drv_cipher_symc_gcm_get_tag(drv_symc_context_t *symc_ctx, td_u8 *tag, td_u32 tag_length)
{
    td_s32 ret;

    /* GCM ADD LEN. */
    crypto_log_dbg("gcm start process clen\n");
    ret = inner_drv_symc_gcm_process_clen(symc_ctx);
    crypto_chk_return(ret != TD_SUCCESS, ret, "inner_drv_symc_gcm_process_clen failed, ret is 0x%x\n", ret);

    /* Calculate ghash. */
    ret = inner_drv_cipher_symc_gcm_ghash_finish(symc_ctx, symc_ctx->ghash, sizeof(symc_ctx->ghash));
    crypto_chk_return(ret != TD_SUCCESS, ret, "inner_drv_cipher_symc_gcm_ghash_finish failed, ret is 0x%x\n", ret);
#if defined(CONFIG_SYMC_GCM_TRACE_ENABLE)
    crypto_dump_data("gcm ghash finish", symc_ctx->ghash, sizeof(symc_ctx->ghash));
#endif
    ret = hal_cipher_gcm_compute_tag(symc_ctx->chn_num, symc_ctx->ghash, sizeof(symc_ctx->ghash),
        symc_ctx->j0, sizeof(symc_ctx->j0), tag, tag_length);
#if defined(CONFIG_SYMC_GCM_TRACE_ENABLE)
    crypto_dump_data("gcm tag", tag, tag_length);
#endif
    return ret;
}

td_s32 inner_drv_symc_gcm_set_ctx(td_handle symc_handle, const drv_symc_gcm_ctx *gcm_ctx)
{
    td_s32 ret;
    drv_symc_context_t *symc_ctx = NULL;

    symc_ctx = inner_get_symc_ctx(symc_handle);
    crypto_chk_return(symc_ctx == TD_NULL, SYMC_COMPAT_ERRNO(ERROR_INVALID_HANDLE), "invalid handle\n");
    crypto_chk_return(symc_ctx->is_open == TD_FALSE, SYMC_COMPAT_ERRNO(ERROR_CTX_CLOSED), "ctx is closed\n");

    symc_ctx->data_len = gcm_ctx->data_len;
    symc_ctx->processed_aad_len = gcm_ctx->processed_aad_len;
    symc_ctx->total_aad_len = gcm_ctx->total_aad_len;
    symc_ctx->processed_len = gcm_ctx->processed_len;

    symc_ctx->ghash_tail_len = gcm_ctx->ghash_tail_len;
    ret = memcpy_s(symc_ctx->ghash_tail, sizeof(symc_ctx->ghash_tail),
        gcm_ctx->ghash_tail, sizeof(gcm_ctx->ghash_tail));
    crypto_chk_return(ret != EOK, SYMC_COMPAT_ERRNO(ERROR_MEMCPY_S), "memcpy_s failed\n");

    ret = memcpy_s(symc_ctx->ghash, sizeof(symc_ctx->ghash), gcm_ctx->ghash, sizeof(gcm_ctx->ghash));
    crypto_chk_return(ret != EOK, SYMC_COMPAT_ERRNO(ERROR_MEMCPY_S), "memcpy_s failed\n");

    ret = memcpy_s(symc_ctx->j0, sizeof(symc_ctx->j0), gcm_ctx->j0, sizeof(gcm_ctx->j0));
    crypto_chk_return(ret != EOK, SYMC_COMPAT_ERRNO(ERROR_MEMCPY_S), "memcpy_s failed\n");

    return CRYPTO_SUCCESS;
}

td_s32 inner_drv_symc_gcm_get_ctx(td_handle symc_handle, drv_symc_gcm_ctx *gcm_ctx)
{
    td_s32 ret;
    drv_symc_context_t *symc_ctx = NULL;

    symc_ctx = inner_get_symc_ctx(symc_handle);
    crypto_chk_return(symc_ctx == TD_NULL, SYMC_COMPAT_ERRNO(ERROR_INVALID_HANDLE), "invalid handle\n");
    crypto_chk_return(symc_ctx->is_open == TD_FALSE, SYMC_COMPAT_ERRNO(ERROR_CTX_CLOSED), "ctx is closed\n");

    gcm_ctx->data_len = symc_ctx->data_len;
    gcm_ctx->processed_aad_len = symc_ctx->processed_aad_len;
    gcm_ctx->total_aad_len = symc_ctx->total_aad_len;
    gcm_ctx->processed_len = symc_ctx->processed_len;

    gcm_ctx->ghash_tail_len = symc_ctx->ghash_tail_len;
    ret = memcpy_s(gcm_ctx->ghash_tail, sizeof(gcm_ctx->ghash_tail),
        symc_ctx->ghash_tail, sizeof(symc_ctx->ghash_tail));
    crypto_chk_return(ret != EOK, SYMC_COMPAT_ERRNO(ERROR_MEMCPY_S), "memcpy_s failed\n");

    ret = memcpy_s(gcm_ctx->ghash, sizeof(gcm_ctx->ghash), symc_ctx->ghash, sizeof(symc_ctx->ghash));
    crypto_chk_return(ret != EOK, SYMC_COMPAT_ERRNO(ERROR_MEMCPY_S), "memcpy_s failed\n");

    ret = memcpy_s(gcm_ctx->j0, sizeof(gcm_ctx->j0), symc_ctx->j0, sizeof(symc_ctx->j0));
    crypto_chk_return(ret != EOK, SYMC_COMPAT_ERRNO(ERROR_MEMCPY_S), "memcpy_s failed\n");

    return CRYPTO_SUCCESS;
}
#else
td_s32 inner_drv_symc_gcm_set_ctx(td_handle symc_handle, const drv_symc_gcm_ctx *gcm_ctx)
{
    crypto_unused(symc_handle);
    crypto_unused(gcm_ctx);

    return SYMC_COMPAT_ERRNO(ERROR_UNSUPPORT);
}

td_s32 inner_drv_symc_gcm_get_ctx(td_handle symc_handle, drv_symc_gcm_ctx *gcm_ctx)
{
    crypto_unused(symc_handle);
    crypto_unused(gcm_ctx);

    return SYMC_COMPAT_ERRNO(ERROR_UNSUPPORT);
}
#endif