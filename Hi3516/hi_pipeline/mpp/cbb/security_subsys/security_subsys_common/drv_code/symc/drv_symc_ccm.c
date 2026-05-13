/**
  Copyright (c), 2001-2025, Shenshu Tech. Co., Ltd.
 */

#include "drv_symc.h"
#include "drv_symc_inner.h"
#include "drv_symc_outer.h"
#include "hal_symc.h"
#include "crypto_drv_common.h"
#if defined(CONFIG_SYMC_CCM_SUPPORT)

static td_s32 inner_symc_ccm_process_N(drv_symc_context_t *symc_ctx);
static td_s32 inner_symc_ccm_process_aad(drv_symc_context_t *symc_ctx, td_u64 aad_phys_addr, td_u32 aad_len);

static td_s32 inner_symc_ccm_cfg_param_check(const crypto_symc_ctrl_t *symc_ctrl)
{
    crypto_symc_config_aes_ccm_gcm *ccm_gcm_param = (crypto_symc_config_aes_ccm_gcm *)symc_ctrl->param;
    td_u32 iv_length = symc_ctrl->iv_length;
    td_u32 tag_len;
    td_u32 data_len;
    td_u32 diff = 0;

#if !defined(CONFIG_SYMC_SM4_CCM_GCM_SUPPORT)
        crypto_chk_return(symc_ctrl->symc_alg == CRYPTO_SYMC_ALG_SM4,
            SYMC_COMPAT_ERRNO(ERROR_UNSUPPORT), "sm4 unsupport this mode\n");
#endif

    if (symc_ctrl->iv_change_flag == CRYPTO_SYMC_CCM_IV_CHANGE_START) {
        crypto_chk_return(ccm_gcm_param == TD_NULL, SYMC_COMPAT_ERRNO(ERROR_PARAM_IS_NULL), "param is NULL\n");
        tag_len = ccm_gcm_param->tag_len;
        data_len = ccm_gcm_param->data_len;
        /* IV lenght for CCM, which should be 7~13. */
        crypto_chk_return(iv_length > 13 || iv_length < 7,      // 7: min iv_len for ccm, 13: max iv_len for ccm.
            SYMC_COMPAT_ERRNO(ERROR_INVALID_PARAM), "iv_len must be 7~13 for CCM\n");
        /* Tag length for CCM should be 4,6,8,10,12,14,16. */
        crypto_chk_return(
            tag_len % 2 != 0 || tag_len < 4 || tag_len > 16,  // 4: min tag_len, 16: max tag_len, 2: judge if even.
            SYMC_COMPAT_ERRNO(ERROR_INVALID_PARAM), "tag_len must be 4,6,8,10,12,14,16 for CCM\n");
        diff = CRYPTO_IV_LEN_IN_BYTES - iv_length - 1;
        if (diff == 2) {    // 2: 2 * 8 bits to represent data_len.
            crypto_chk_return(data_len > 0xFFFF, SYMC_COMPAT_ERRNO(ERROR_INVALID_PARAM), // 0xFFFF: 2^16 - 1
                "data_len is too long\n");
        }
        if (diff == 3) {    // 3: 3 * 8 bits to represent data_len.
            crypto_chk_return(data_len > 0xFFFFFF, SYMC_COMPAT_ERRNO(ERROR_INVALID_PARAM), // 0xFFFFFF: 2^24 - 1
                "data_len is too long\n");
        }
    }

    return TD_SUCCESS;
}

static td_s32 inner_format_ccm_iv(const td_u8 *iv, td_u32 iv_length, td_u8 *ccm_iv, td_u32 *ccm_iv_length)
{
    td_s32 ret;

    ccm_iv[0] = CRYPTO_AES_CCM_NQ_LEN - iv_length;
    ret = memcpy_s(&ccm_iv[1], *ccm_iv_length - 1, iv, iv_length);
    if (ret != EOK) {
        crypto_log_err("memcpy_s failed\n");
        return TD_FAILURE;
    }
    *ccm_iv_length = iv_length + 1;
    return TD_SUCCESS;
}

static td_void inner_format_ccm_header(td_u32 aad_len, td_u8 *ccm_header, td_u32 *ccm_header_len)
{
    td_u32 idx = 0;
    ccm_header[idx++] = (td_u8)(aad_len >> 8);  /* 8: bits 15-8 */
    ccm_header[idx++] = (td_u8)(aad_len);
    *ccm_header_len = idx;
}

static void inner_ccm_iv_plus_one(uint8_t *iv, uint32_t iv_len)
{
    uint32_t value = CRYPTO_BYTE_MAX + 1;
    int32_t i = iv_len - 1;

    while (value > CRYPTO_BYTE_MAX && i >= 0) {
        value = iv[i] + 1;
        iv[i] = value % CRYPTO_BYTE_MAX;
        iv--;
    }
}

td_s32 drv_cipher_symc_ccm_set_config(drv_symc_context_t *symc_ctx, const crypto_symc_ctrl_t *symc_ctrl)
{
    td_s32 ret;
    hal_symc_config_t hal_symc_config = {0};
    crypto_symc_config_aes_ccm_gcm *ccm_gcm_config = TD_NULL;

    crypto_assert_neq(symc_ctx, TD_NULL);
    crypto_assert_neq(symc_ctrl, TD_NULL);

    ret = inner_symc_ccm_cfg_param_check(symc_ctrl);
    crypto_chk_return(ret != TD_SUCCESS, ret, "inner_symc_ccm_cfg_param_check failed\n");

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

    if (symc_ctrl->iv_change_flag == CRYPTO_SYMC_CCM_IV_CHANGE_FINISH) {
        ret = hal_cipher_symc_set_iv(symc_ctx->chn_num, symc_ctrl->iv, sizeof(symc_ctrl->iv));
        crypto_chk_return(ret != TD_SUCCESS, ret, "hal_cipher_symc_set_iv failed, ret is 0x%x\n", ret);
        symc_ctx->is_config = TD_TRUE;
        return CRYPTO_SUCCESS;
    }

    if (symc_ctrl->iv_change_flag == CRYPTO_SYMC_CCM_IV_CHANGE_START) {
        ccm_gcm_config = (crypto_symc_config_aes_ccm_gcm *)symc_ctrl->param;
        symc_ctx->total_aad_len = ccm_gcm_config->total_aad_len;
        symc_ctx->data_len = ccm_gcm_config->data_len;
        symc_ctx->tag_len = ccm_gcm_config->tag_len;
        symc_ctx->processed_len = 0;
        (void)memset_s(symc_ctx->iv, sizeof(symc_ctx->iv), 0, sizeof(symc_ctx->iv));
        (void)memset_s(symc_ctx->ctr0, sizeof(symc_ctx->ctr0), 0, sizeof(symc_ctx->ctr0));
        symc_ctx->iv_length = sizeof(symc_ctx->iv);
        ret = inner_format_ccm_iv(symc_ctrl->iv, symc_ctrl->iv_length, symc_ctx->iv, &(symc_ctx->iv_length));
        crypto_chk_return(ret != TD_SUCCESS, ret, "inner_format_ccm_iv failed, ret is 0x%x\n", ret);

        ret = memcpy_s(symc_ctx->ctr0, sizeof(symc_ctx->ctr0), symc_ctx->iv, symc_ctx->iv_length);
        crypto_chk_return(ret != EOK, SYMC_COMPAT_ERRNO(ERROR_MEMCPY_S), "memcpy_s failed\n");
#if defined(CONFIG_SYMC_CCM_TRACE_ENABLE)
        crypto_dump_data("ccm_iv", symc_ctx->iv, sizeof(symc_ctx->iv));
#endif
        /* ccm cmac N. */
        ret = inner_symc_ccm_process_N(symc_ctx);
        crypto_chk_return(ret != TD_SUCCESS, ret, "inner_symc_ccm_process_N failed, ret is 0x%x\n", ret);

        inner_ccm_iv_plus_one(symc_ctx->iv, sizeof(symc_ctx->iv));
        ret = hal_cipher_symc_set_iv(symc_ctx->chn_num, symc_ctx->iv, sizeof(symc_ctx->iv));
        crypto_chk_return(ret != TD_SUCCESS, ret, "hal_cipher_symc_set_iv failed, ret is 0x%x\n", ret);
    }

    if (symc_ctrl->iv_change_flag == CRYPTO_SYMC_CCM_IV_CHANGE_START ||
        symc_ctrl->iv_change_flag == CRYPTO_SYMC_CCM_IV_CHANGE_UPDATE) {
        ccm_gcm_config = (crypto_symc_config_aes_ccm_gcm *)symc_ctrl->param;
        /* ccm cmac aad. */
        if (ccm_gcm_config->aad_len != 0) {
            ret = inner_symc_ccm_process_aad(symc_ctx, ccm_gcm_config->aad_buf.phys_addr, ccm_gcm_config->aad_len);
            crypto_chk_return(ret != TD_SUCCESS, ret, "inner_symc_ccm_process_aad failed, ret is 0x%x\n", ret);
        }
    }
    symc_ctx->is_config = TD_TRUE;

    return TD_SUCCESS;
}

static td_s32 inner_symc_ccm_process_N(drv_symc_context_t *symc_ctx)
{
    td_s32 ret;
    td_u32 data_len = symc_ctx->data_len;
    td_u32 idx = 0;
    td_u32 q;
    td_u32 N_len = CRYPTO_AES_BLOCK_SIZE_IN_BYTES;
    td_u8 *N = crypto_malloc_coherent(N_len, "ccm_n");
    crypto_chk_return(N == NULL, SYMC_COMPAT_ERRNO(ERROR_MALLOC), "crypto_malloc_coherent failed\n");

    /* CCM ADD N. */
    (td_void)memset_s(N, N_len, 0, N_len);
    /* First byte of N. */
    N[idx] = (symc_ctx->total_aad_len > 0 ? 1 : 0) << CRYPTO_BIT_6;
    N[idx] |= ((symc_ctx->tag_len - 2) / 2) << CRYPTO_BIT_3;  /* 2: ccm require */
    N[idx] |= (CRYPTO_AES_BLOCK_SIZE_IN_BYTES - 1 - symc_ctx->iv_length);
    idx++;
    /* copy ccm_iv[1..] to N[1..] */
    ret = memcpy_s(&N[idx], N_len - idx, &(symc_ctx->iv[1]), symc_ctx->iv_length - 1);
    crypto_chk_goto_with_ret(ret, ret != EOK, exit_free, SYMC_COMPAT_ERRNO(ERROR_MEMCPY_S), "memcpy_s failed\n");
    idx += symc_ctx->iv_length - 1;

    q = N_len - idx;
    if (q >= CRYPTO_SYMC_CCM_Q_LEN_4B) {
        idx = CRYPTO_AES_BLOCK_SIZE_IN_BYTES - CRYPTO_SYMC_CCM_Q_LEN_4B;
        N[idx++] = (td_u8)(data_len >> 24);     /* 24: bits 31-24 */
        N[idx++] = (td_u8)(data_len >> 16);     /* 16: bits 23-16 */
        N[idx++] = (td_u8)(data_len >> 8);      /* 8: bits 15-8 */
        N[idx++] = (td_u8)(data_len);
    } else if (q == CRYPTO_SYMC_CCM_Q_LEN_3B) {
        N[idx++] = (td_u8)(data_len >> 16);     /* 16: bits 23-16 */
        N[idx++] = (td_u8)(data_len >> 8);      /* 8: bits 15-8 */
        N[idx++] = (td_u8)(data_len);
    } else if (q == CRYPTO_SYMC_CCM_Q_LEN_2B) {
        N[idx++] = (td_u8)(data_len >> 8);      /* 8: bits 15-8 */
        N[idx++] = (td_u8)(data_len);
    } else {
        crypto_log_err("Invalid iv_len\n");
        ret = SYMC_COMPAT_ERRNO(ERROR_INVALID_PARAM);
        goto exit_free;
    }
#ifdef CONFIG_SYMC_CCM_TRACE_ENABLE
    crypto_dump_data("N", N, CRYPTO_AES_BLOCK_SIZE_IN_BYTES);
#endif
    ret = inner_drv_cipher_symc_ccm_cmac_start(symc_ctx);
    crypto_chk_goto_with_ret(ret, ret != TD_SUCCESS, exit_free, ret, "inner_drv_cipher_symc_ccm_cmac_start failed\n");

    ret = inner_drv_cipher_symc_ccm_cmac_update(symc_ctx, crypto_get_phys_addr(N), N_len);
    crypto_chk_goto_with_ret(ret, ret != CRYPTO_SUCCESS, exit_free, ret,
        "inner_drv_cipher_symc_ccm_cmac_update failed\n");

exit_free:
    (void)memset_s(N, N_len, 0, N_len);
    crypto_free_coherent(N);
    return ret;
}

static td_s32 inner_symc_ccm_process_aad(drv_symc_context_t *symc_ctx, td_u64 aad_phys_addr, td_u32 aad_len)
{
    td_s32 ret = TD_SUCCESS;
    td_u32 ccm_header_len = CRYPTO_AES_BLOCK_SIZE_IN_BYTES;
    td_u32 ccm_padding_len = CRYPTO_AES_BLOCK_SIZE_IN_BYTES;
    td_u8 *ccm_header = NULL;
    td_u8 *ccm_padding = NULL;
    td_u8 *ccm_buf = crypto_malloc_coherent(ccm_header_len + ccm_padding_len, "ccm_buf");
    crypto_chk_return(ccm_buf == NULL, SYMC_COMPAT_ERRNO(ERROR_MALLOC), "crypto_malloc_coherent failed\n");

    ccm_header = ccm_buf;
    ccm_padding = ccm_buf + ccm_header_len;

    if (symc_ctx->iv_change_flag == CRYPTO_SYMC_CCM_IV_CHANGE_START) {
        symc_ctx->processed_aad_len = 0;
        /* AAD Header. */
        inner_format_ccm_header(symc_ctx->total_aad_len, ccm_header, &ccm_header_len);
#ifdef CONFIG_SYMC_CCM_TRACE_ENABLE
        crypto_dump_data("ccm_header", ccm_header, ccm_header_len);
#endif
        ret = inner_drv_cipher_symc_ccm_cmac_update(symc_ctx, crypto_get_phys_addr(ccm_header), ccm_header_len);
        crypto_chk_goto(ret != CRYPTO_SUCCESS, exit_free, "inner_drv_cipher_symc_ccm_cmac_update failed\n");
    }

    if (aad_len != 0) {
        /* AAD Body. */
#ifdef CONFIG_SYMC_CCM_TRACE_ENABLE
        crypto_dump_phys_addr("aad", aad_phys_addr, aad_len);
#endif
        ret = inner_drv_cipher_symc_ccm_cmac_update(symc_ctx, aad_phys_addr, aad_len);
        crypto_chk_goto(ret != CRYPTO_SUCCESS, exit_free, "inner_drv_cipher_symc_ccm_cmac_update failed\n");

        symc_ctx->processed_aad_len += aad_len;
    }

    /* AAD Padding. */
    if (symc_ctx->processed_aad_len == symc_ctx->total_aad_len) {
        ccm_padding_len = CRYPTO_AES_BLOCK_SIZE_IN_BYTES -
            (CRYPTO_SYMC_CCM_HEADER_LEN + symc_ctx->total_aad_len) % CRYPTO_AES_BLOCK_SIZE_IN_BYTES;
        /* AAD Padding. */
        if (ccm_padding_len != CRYPTO_AES_BLOCK_SIZE_IN_BYTES) {
            (td_void)memset_s(ccm_padding, ccm_padding_len, 0, ccm_padding_len);
#ifdef CONFIG_SYMC_CCM_TRACE_ENABLE
            crypto_dump_data("ccm_padding", ccm_padding, ccm_padding_len);
#endif
            ret = inner_drv_cipher_symc_ccm_cmac_update(symc_ctx, crypto_get_phys_addr(ccm_padding), ccm_padding_len);
            crypto_chk_goto(ret != CRYPTO_SUCCESS, exit_free, "inner_drv_cipher_symc_ccm_cmac_update failed\n");
        }
    }

exit_free:
    crypto_free_coherent(ccm_buf);
    return ret;
}

td_s32 inner_drv_cipher_symc_ccm_process(drv_symc_context_t *symc_ctx, td_ulong src_phys_addr,
    td_ulong dst_phys_addr, td_u32 length)
{
    td_s32 ret;
    td_u32 ccm_p_node_type = 0;
    hal_symc_config_t symc_config = {0};

    symc_null_ptr_chk(symc_ctx);
    if (length == 0) {
        return CRYPTO_SUCCESS;
    }

    /* CCM ADD P. */
    crypto_log_dbg("ccm start process P\n");
    if (symc_ctx->is_decrypt == TD_FALSE) { /* Encrypt, cmac update the plain text. */
        ret = inner_drv_cipher_symc_ccm_cmac_update(symc_ctx, src_phys_addr, length);
        crypto_chk_return(ret != CRYPTO_SUCCESS, ret, "inner_drv_cipher_symc_ccm_cmac_update failed\n");
    }

    ccm_p_node_type = IN_NODE_TYPE_CCM_P | IN_NODE_TYPE_FIRST | IN_NODE_TYPE_LAST;
    if (symc_ctx->processed_len + length == symc_ctx->data_len) {
        ccm_p_node_type |= IN_NODE_TYPE_CCM_LAST;
    }

    symc_config.symc_alg = symc_ctx->symc_alg;
    symc_config.work_mode = symc_ctx->work_mode;
    symc_config.symc_key_length = symc_ctx->symc_key_length;
    symc_config.symc_bit_width = symc_ctx->symc_bit_width;

    ret = hal_cipher_symc_config(symc_ctx->chn_num, &symc_config);
    crypto_chk_return(ret != TD_SUCCESS, ret, "hal_cipher_symc_config failed\n");

    ret = inner_drv_symc_common_process(symc_ctx, src_phys_addr, dst_phys_addr, length, ccm_p_node_type);
    crypto_chk_return(ret != TD_SUCCESS, ret, "inner_drv_symc_common_process failed\n");

    if (symc_ctx->is_decrypt == TD_TRUE) { /* Decrypt, cmac update the plain text. */
        ret = inner_drv_cipher_symc_ccm_cmac_update(symc_ctx, dst_phys_addr, length);
        crypto_chk_return(ret != CRYPTO_SUCCESS, ret, "inner_drv_cipher_symc_ccm_cmac_update failed\n");
    }

    symc_ctx->processed_len += length;
    return ret;
}

td_s32 inner_drv_cipher_symc_ccm_get_tag(drv_symc_context_t *symc_ctx, td_u8 *tag, td_u32 tag_length)
{
    uint32_t i;
    td_s32 ret;
    td_u8 s0[CRYPTO_AES_BLOCK_SIZE_IN_BYTES];
#if defined(CONFIG_SYMC_CCM_TRACE_ENABLE)
    crypto_dump_data("ccm ctr0", symc_ctx->ctr0, sizeof(symc_ctx->ctr0));
#endif
    ret = hal_cipher_ccm_compute_s0(symc_ctx->chn_num, symc_ctx->ctr0, sizeof(symc_ctx->ctr0), s0, sizeof(s0));
    crypto_chk_return(ret != CRYPTO_SUCCESS, ret, "hal_cipher_ccm_compute_s0");

    ret = inner_drv_cipher_symc_ccm_cmac_finish(symc_ctx);
    crypto_chk_return(ret != CRYPTO_SUCCESS, ret, "inner_drv_cipher_symc_ccm_cmac_finish");

    for (i = 0; i < tag_length; i++) {
        tag[i] = symc_ctx->ccm_cmac[i] ^ s0[i];
    }

#if defined(CONFIG_SYMC_CCM_TRACE_ENABLE)
    crypto_dump_data("ccm s0", s0, sizeof(s0));
    crypto_dump_data("ccm cmac", symc_ctx->ccm_cmac, sizeof(symc_ctx->ccm_cmac));
    crypto_dump_data("ccm tag", tag, tag_length);
#endif
    return ret;
}

int32_t inner_drv_cipher_symc_ccm_cmac_start(drv_symc_context_t *symc_ctx)
{
    symc_ctx->cmac_tail_len = 0;
    (void)memset_s(symc_ctx->ccm_cmac, sizeof(symc_ctx->ccm_cmac), 0, sizeof(symc_ctx->ccm_cmac));
    return CRYPTO_SUCCESS;
}

int32_t inner_drv_cipher_symc_ccm_cmac_update(drv_symc_context_t *symc_ctx,
    uint64_t data_phys_addr, uint32_t data_len)
{
    int32_t ret;
    uint32_t left = data_len;
    uint32_t processed_len = 0;
    uint32_t processing_len = 0;
    uint32_t tail_len = symc_ctx->cmac_tail_len;
    uint32_t tail_buf_len = sizeof(symc_ctx->cmac_tail);
    uint8_t *ccm_tail_buf = TD_NULL;

    ccm_tail_buf = crypto_malloc_coherent(tail_buf_len, "ccm_tail_buf");
    crypto_chk_return(ccm_tail_buf == TD_NULL, SYMC_COMPAT_ERRNO(ERROR_MALLOC), "crypto_malloc_coherent failed\n");
#if defined(CONFIG_SYMC_CCM_TRACE_ENABLE)
    crypto_dump_data("cmac before", symc_ctx->ccm_cmac, sizeof(symc_ctx->ccm_cmac));
    crypto_dump_data("cmac_tail before", symc_ctx->cmac_tail, tail_len);
    crypto_dump_phys_addr("cmac data", data_phys_addr, data_len);
#endif

    processing_len = crypto_min(tail_buf_len - tail_len, data_len);
    ret = crypto_copy_from_phys_addr(symc_ctx->cmac_tail + tail_len, tail_buf_len - tail_len,
        data_phys_addr, processing_len);
    crypto_chk_goto(ret != TD_SUCCESS, exit_free, "crypto_copy_from_phys_addr failed\n");

    symc_ctx->cmac_tail_len += processing_len;

    /* process one block. */
    if (symc_ctx->cmac_tail_len == tail_buf_len) {
        (void)memcpy_s(ccm_tail_buf, tail_buf_len, symc_ctx->cmac_tail, tail_buf_len);
        ret = hal_cipher_ccm_cmac_update(symc_ctx->chn_num, crypto_get_phys_addr(ccm_tail_buf),
            tail_buf_len, symc_ctx->ccm_cmac, sizeof(symc_ctx->ccm_cmac));
        crypto_chk_goto(ret != CRYPTO_SUCCESS, exit_free, "hal_cipher_ccm_cmac_update failed\n");
        symc_ctx->cmac_tail_len = 0;
    }

    left -= processing_len;
    processed_len += processing_len;
    if (left == 0) {
#if defined(CONFIG_SYMC_CCM_TRACE_ENABLE)
        crypto_dump_data("cmac after", symc_ctx->ccm_cmac, sizeof(symc_ctx->ccm_cmac));
#endif
        ret = CRYPTO_SUCCESS;
        goto exit_free;
    }
    /* process blocks. */
    while (left >= tail_buf_len) {
        ret = crypto_copy_from_phys_addr(ccm_tail_buf, tail_buf_len, data_phys_addr + processed_len, tail_buf_len);
        crypto_chk_goto(ret != TD_SUCCESS, exit_free, "crypto_copy_from_phys_addr failed\n");
        ret = hal_cipher_ccm_cmac_update(symc_ctx->chn_num, crypto_get_phys_addr(ccm_tail_buf), tail_buf_len,
            symc_ctx->ccm_cmac, sizeof(symc_ctx->ccm_cmac));
        crypto_chk_goto(ret != CRYPTO_SUCCESS, exit_free, "hal_cipher_ccm_cmac_update failed\n");
        left -= tail_buf_len;
        processed_len += tail_buf_len;
    }
    if (left != 0) {
        ret = crypto_copy_from_phys_addr(symc_ctx->cmac_tail, tail_buf_len, data_phys_addr + processed_len, left);
        crypto_chk_goto(ret != TD_SUCCESS, exit_free, "crypto_copy_from_phys_addr failed\n");
    }
    symc_ctx->cmac_tail_len = left;

#if defined(CONFIG_SYMC_CCM_TRACE_ENABLE)
    crypto_dump_data("cmac after", symc_ctx->ccm_cmac, sizeof(symc_ctx->ccm_cmac));
#endif
    ret = TD_SUCCESS;
exit_free:
    crypto_free_coherent(ccm_tail_buf);
    return ret;
}

int32_t inner_drv_cipher_symc_ccm_cmac_finish(drv_symc_context_t *symc_ctx)
{
    int32_t ret;
    uint32_t tail_len = symc_ctx->cmac_tail_len;
    uint32_t tail_buf_len = sizeof(symc_ctx->cmac_tail);
    uint8_t *ccm_tail_buf = TD_NULL;

    if (symc_ctx->cmac_tail_len != 0) {
        (void)memset_s(symc_ctx->cmac_tail + tail_len, tail_buf_len - tail_len, 0, tail_buf_len - tail_len);

        ccm_tail_buf = crypto_malloc_coherent(tail_buf_len, "ccm_tail_buf");
        crypto_chk_return(ccm_tail_buf == TD_NULL, SYMC_COMPAT_ERRNO(ERROR_MALLOC), "crypto_malloc_coherent failed\n");

        (void)memcpy_s(ccm_tail_buf, tail_buf_len, symc_ctx->cmac_tail, tail_buf_len);
        ret = hal_cipher_ccm_cmac_update(symc_ctx->chn_num, crypto_get_phys_addr(ccm_tail_buf), tail_buf_len,
            symc_ctx->ccm_cmac, sizeof(symc_ctx->ccm_cmac));
        crypto_free_coherent(ccm_tail_buf);

        crypto_chk_return(ret != CRYPTO_SUCCESS, CRYPTO_FAILURE, "hal_cipher_ccm_cmac_update failed\n");
    }

    return CRYPTO_SUCCESS;
}

td_s32 inner_drv_symc_ccm_set_ctx(td_handle symc_handle, const drv_symc_ccm_ctx *ccm_ctx)
{
    td_s32 ret;
    drv_symc_context_t *symc_ctx = NULL;

    symc_ctx = inner_get_symc_ctx(symc_handle);
    crypto_chk_return(symc_ctx == TD_NULL, SYMC_COMPAT_ERRNO(ERROR_INVALID_HANDLE), "invalid handle\n");
    crypto_chk_return(symc_ctx->is_open == TD_FALSE, SYMC_COMPAT_ERRNO(ERROR_CTX_CLOSED), "ctx is closed\n");

    symc_ctx->data_len = ccm_ctx->data_len;
    symc_ctx->processed_aad_len = ccm_ctx->processed_aad_len;
    symc_ctx->total_aad_len = ccm_ctx->total_aad_len;

    symc_ctx->tag_len = ccm_ctx->tag_len;
    symc_ctx->cmac_tail_len = ccm_ctx->cmac_tail_len;
    ret = memcpy_s(symc_ctx->ccm_cmac, sizeof(symc_ctx->ccm_cmac), ccm_ctx->ccm_cmac, sizeof(ccm_ctx->ccm_cmac));
    crypto_chk_return(ret != EOK, SYMC_COMPAT_ERRNO(ERROR_MEMCPY_S), "memcpy_s failed\n");

    ret = memcpy_s(symc_ctx->ctr0, sizeof(symc_ctx->ctr0), ccm_ctx->ctr0, sizeof(ccm_ctx->ctr0));
    crypto_chk_return(ret != EOK, SYMC_COMPAT_ERRNO(ERROR_MEMCPY_S), "memcpy_s failed\n");

    ret = memcpy_s(symc_ctx->cmac_tail, sizeof(symc_ctx->cmac_tail), ccm_ctx->cmac_tail, sizeof(ccm_ctx->cmac_tail));
    crypto_chk_return(ret != EOK, SYMC_COMPAT_ERRNO(ERROR_MEMCPY_S), "memcpy_s failed\n");

    return CRYPTO_SUCCESS;
}

td_s32 inner_drv_symc_ccm_get_ctx(td_handle symc_handle, drv_symc_ccm_ctx *ccm_ctx)
{
    td_s32 ret;
    drv_symc_context_t *symc_ctx = NULL;

    symc_ctx = inner_get_symc_ctx(symc_handle);
    crypto_chk_return(symc_ctx == TD_NULL, SYMC_COMPAT_ERRNO(ERROR_INVALID_HANDLE), "invalid handle\n");
    crypto_chk_return(symc_ctx->is_open == TD_FALSE, SYMC_COMPAT_ERRNO(ERROR_CTX_CLOSED), "ctx is closed\n");

    ccm_ctx->data_len = symc_ctx->data_len;
    ccm_ctx->processed_aad_len = symc_ctx->processed_aad_len;
    ccm_ctx->total_aad_len = symc_ctx->total_aad_len;

    ccm_ctx->tag_len = symc_ctx->tag_len;
    ccm_ctx->cmac_tail_len = symc_ctx->cmac_tail_len;
    ret = memcpy_s(ccm_ctx->ccm_cmac, sizeof(ccm_ctx->ccm_cmac), symc_ctx->ccm_cmac, sizeof(symc_ctx->ccm_cmac));
    crypto_chk_return(ret != EOK, SYMC_COMPAT_ERRNO(ERROR_MEMCPY_S), "memcpy_s failed\n");

    ret = memcpy_s(ccm_ctx->ctr0, sizeof(ccm_ctx->ctr0), symc_ctx->ctr0, sizeof(symc_ctx->ctr0));
    crypto_chk_return(ret != EOK, SYMC_COMPAT_ERRNO(ERROR_MEMCPY_S), "memcpy_s failed\n");

    ret = memcpy_s(ccm_ctx->cmac_tail, sizeof(ccm_ctx->cmac_tail), symc_ctx->cmac_tail, sizeof(symc_ctx->cmac_tail));
    crypto_chk_return(ret != EOK, SYMC_COMPAT_ERRNO(ERROR_MEMCPY_S), "memcpy_s failed\n");

    return CRYPTO_SUCCESS;
}
#else
td_s32 inner_drv_symc_ccm_set_ctx(td_handle symc_handle, const drv_symc_ccm_ctx *ccm_ctx)
{
    crypto_unused(symc_handle);
    crypto_unused(ccm_ctx);

    return SYMC_COMPAT_ERRNO(ERROR_UNSUPPORT);
}

td_s32 inner_drv_symc_ccm_get_ctx(td_handle symc_handle, drv_symc_ccm_ctx *ccm_ctx)
{
    crypto_unused(symc_handle);
    crypto_unused(ccm_ctx);

    return SYMC_COMPAT_ERRNO(ERROR_UNSUPPORT);
}
#endif