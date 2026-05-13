/**
  Copyright (c), 2001-2025, Shenshu Tech. Co., Ltd.
 */

#include "drv_symc.h"
#include "drv_symc_inner.h"
#include "hal_symc.h"
#include "crypto_drv_common.h"

static drv_symc_context_t g_drv_symc_ctx_list[CONFIG_SYMC_HARD_CHN_CNT];

td_s32 inner_symc_drv_handle_chk(td_handle symc_handle)
{
    td_u32 chn_num = (td_u32)symc_handle;
    if (chn_num >= CONFIG_SYMC_HARD_CHN_CNT) {
        return SYMC_COMPAT_ERRNO(ERROR_INVALID_HANDLE);
    }
    if (((1 << chn_num) & CONFIG_SYMC_HARD_CHANNEL_MASK) == 0) {
        return SYMC_COMPAT_ERRNO(ERROR_INVALID_HANDLE);
    }
    return TD_SUCCESS;
}

drv_symc_context_t *inner_get_symc_ctx(td_handle symc_handle)
{
    td_s32 ret;
    td_u32 chn_num = (td_u32)symc_handle;
    ret = inner_symc_drv_handle_chk(symc_handle);
    if (ret != TD_SUCCESS) {
        return TD_NULL;
    }
    return &g_drv_symc_ctx_list[chn_num];
}

td_s32 drv_cipher_symc_init(td_void)
{
    td_s32 ret;
    crypto_drv_func_enter();
    ret = hal_cipher_symc_init();
    if (ret != TD_SUCCESS) {
        crypto_log_err("hal_cipher_symc_init failed, ret is 0x%x\n", ret);
        return ret;
    }

    crypto_drv_func_exit();
    return ret;
}

td_s32 drv_cipher_symc_deinit(td_void)
{
    crypto_drv_func_enter();
    hal_cipher_symc_deinit();
    crypto_drv_func_exit();
    return TD_SUCCESS;
}

td_s32 drv_cipher_symc_create(td_handle *symc_handle, const crypto_symc_attr *symc_attr)
{
    td_u32 i;
    td_s32 ret = CRYPTO_SUCCESS;
    drv_symc_context_t *symc_ctx = TD_NULL;
    crypto_drv_func_enter();

    symc_null_ptr_chk(symc_handle);
    symc_null_ptr_chk(symc_attr);

    for (i = 0; i < CONFIG_SYMC_HARD_CHN_CNT; i++) {
        symc_ctx = &g_drv_symc_ctx_list[i];
        if (symc_ctx->is_open == TD_TRUE) {
            continue;
        }
        ret = hal_cipher_symc_lock_chn(i);
        if (ret == CRYPTO_SUCCESS) {
            break;
        }
    }
    if (i >= CONFIG_SYMC_HARD_CHN_CNT) {
        crypto_log_err("All SYMC Channels are busy!\n");
        return SYMC_COMPAT_ERRNO(ERROR_CHN_BUSY);
    }
    symc_ctx = &g_drv_symc_ctx_list[i];
    (td_void)memset_s(symc_ctx, sizeof(drv_symc_context_t), 0, sizeof(drv_symc_context_t));

    symc_ctx->chn_num = i;
    symc_ctx->is_open = TD_TRUE;
    *symc_handle = i;

    crypto_drv_func_exit();
    return CRYPTO_SUCCESS;
}

td_s32 drv_cipher_symc_destroy(td_handle symc_handle)
{
    td_s32 ret;
    drv_symc_context_t *symc_ctx = TD_NULL;
    td_u32 chn_num = (td_u32)symc_handle;
    crypto_drv_func_enter();

    ret = inner_symc_drv_handle_chk(symc_handle);
    if (ret != TD_SUCCESS) {
        crypto_log_err("Invalid Handle\n");
        return ret;
    }
    symc_ctx = &g_drv_symc_ctx_list[chn_num];

#if defined(CONFIG_CRYPTO_SOFT_CENC_SUPPORT)
    cenc_ddr_release();
    cenc_ddr_unmap_input(chn_num);
#endif
    (td_void)memset_s(symc_ctx, sizeof(drv_symc_context_t), 0, sizeof(drv_symc_context_t));
    ret = hal_cipher_symc_unlock_chn(chn_num);
    if (ret != TD_SUCCESS) {
        crypto_log_err("hal_cipher_symc_unlock_chn failed, ret is 0x%x\n", ret);
    }

    crypto_drv_func_exit();
    return TD_SUCCESS;
}

td_s32 inner_symc_cfg_param_check(const crypto_symc_ctrl_t *symc_ctrl)
{
    td_bool is_support = TD_FALSE;
    crypto_symc_alg symc_alg = symc_ctrl->symc_alg;
    crypto_symc_work_mode mode = symc_ctrl->work_mode;
    crypto_symc_key_length key_length = symc_ctrl->symc_key_length;
    crypto_symc_bit_width bit_width = symc_ctrl->symc_bit_width;
    crypto_symc_iv_change_type iv_change_flag = symc_ctrl->iv_change_flag;
    td_u32 iv_length = symc_ctrl->iv_length;

    /* Check Common Params. */
    crypto_chk_return(symc_alg != CRYPTO_SYMC_ALG_AES && symc_alg != CRYPTO_SYMC_ALG_SM4,
        SYMC_COMPAT_ERRNO(ERROR_INVALID_PARAM), "symc_alg is invalid\n");
    crypto_chk_return(symc_alg == CRYPTO_SYMC_ALG_AES && mode > CRYPTO_SYMC_WORK_MODE_GCM,
        SYMC_COMPAT_ERRNO(ERROR_INVALID_PARAM), "aes mode is invalid\n");
    crypto_chk_return(symc_alg == CRYPTO_SYMC_ALG_SM4 && mode > CRYPTO_SYMC_WORK_MODE_CFB,
        SYMC_COMPAT_ERRNO(ERROR_INVALID_PARAM), "sm4 mode is invalid\n");
    crypto_chk_return(key_length < CRYPTO_SYMC_KEY_128BIT || key_length > CRYPTO_SYMC_KEY_256BIT,
        SYMC_COMPAT_ERRNO(ERROR_INVALID_PARAM), "key_length is invalid\n");
    crypto_chk_return(bit_width >= CRYPTO_SYMC_BIT_WIDTH_MAX,
        SYMC_COMPAT_ERRNO(ERROR_INVALID_PARAM), "bit_width is invalid\n");
    crypto_chk_return(iv_change_flag >= CRYPTO_SYMC_IV_CHANGE_MAX, SYMC_COMPAT_ERRNO(ERROR_INVALID_PARAM),
        "iv_change_flag is invalid\n");

    /* Check Mode. && Check key_length. */
    if (symc_alg == CRYPTO_SYMC_ALG_SM4) {
        /* SM4 only support ECB/CBC/CTR. */
#if !defined(CONFIG_SYMC_SM4_CFB_OFB_SUPPORT)
        crypto_chk_return(mode != CRYPTO_SYMC_WORK_MODE_ECB && mode != CRYPTO_SYMC_WORK_MODE_CBC &&
            mode != CRYPTO_SYMC_WORK_MODE_CTR, SYMC_COMPAT_ERRNO(ERROR_UNSUPPORT), "sm4 unsupport this mode\n");
#endif
        /* SM4's keylength must be 128. */
        crypto_chk_return(key_length != CRYPTO_SYMC_KEY_128BIT, SYMC_COMPAT_ERRNO(ERROR_UNSUPPORT),
            "sm4's key_len only support 128-bit\n");
    }

    /* Check bit_width. */
    if (mode == CRYPTO_SYMC_WORK_MODE_CFB) {
        crypto_chk_return(bit_width == CRYPTO_SYMC_BIT_WIDTH_64BIT, SYMC_COMPAT_ERRNO(ERROR_UNSUPPORT),
            "CFB's bit-width don't support 64-bit\n");
    } else {
        crypto_chk_return(bit_width != CRYPTO_SYMC_BIT_WIDTH_128BIT, SYMC_COMPAT_ERRNO(ERROR_UNSUPPORT),
            "bit-width only support 128-bit\n");
    }

    /* Check iv_change_flag. */

    /* Check iv_length. */
    if (mode != CRYPTO_SYMC_WORK_MODE_ECB) {
        crypto_chk_return(iv_length != CRYPTO_IV_LEN_IN_BYTES, SYMC_COMPAT_ERRNO(ERROR_INVALID_PARAM),
            "iv_length is invalid\n");
    }

    is_support = crypto_symc_support(symc_ctrl->symc_alg, symc_ctrl->work_mode,
        symc_ctrl->symc_key_length, symc_ctrl->symc_bit_width);
    crypto_chk_return(is_support == TD_FALSE, SYMC_COMPAT_ERRNO(ERROR_UNSUPPORT), "alg is unsupport\n");
    if (symc_ctrl->symc_alg == CRYPTO_SYMC_ALG_SM4) {
        is_support = crypto_sm4_support();
    }
    crypto_chk_return(is_support == TD_FALSE, SYMC_COMPAT_ERRNO(ERROR_UNSUPPORT), "alg is unsupport\n");

    return TD_SUCCESS;
}

static td_s32 drv_cipher_symc_normal_set_config(drv_symc_context_t *symc_ctx, const crypto_symc_ctrl_t *symc_ctrl)
{
    td_s32 ret;
    hal_symc_config_t hal_symc_config = {0};

    ret = inner_symc_cfg_param_check(symc_ctrl);
    crypto_chk_return(ret != TD_SUCCESS, ret, "inner_symc_cfg_param_check failed, ret is 0x%x\n", ret);

    symc_ctx->iv_change_flag = symc_ctrl->iv_change_flag;
    ret = memcpy_s(symc_ctx->iv, sizeof(symc_ctx->iv), symc_ctrl->iv, sizeof(symc_ctx->iv));
    crypto_chk_return(ret != EOK, SYMC_COMPAT_ERRNO(ERROR_MEMCPY_S), "memcpy_s failed\n");

    ret = hal_cipher_symc_set_iv(symc_ctx->chn_num, symc_ctx->iv, symc_ctrl->iv_length);
    crypto_chk_return(ret != TD_SUCCESS, ret, "hal_cipher_symc_set_iv failed, ret is 0x%x\n", ret);

    hal_symc_config.symc_alg = symc_ctrl->symc_alg;
    hal_symc_config.work_mode = symc_ctrl->work_mode;
    hal_symc_config.symc_bit_width = symc_ctrl->symc_bit_width;
    hal_symc_config.symc_key_length = symc_ctrl->symc_key_length;
    hal_symc_config.iv_change_flag = symc_ctrl->iv_change_flag;
    ret = hal_cipher_symc_config(symc_ctx->chn_num, &hal_symc_config);
    crypto_chk_return(ret != TD_SUCCESS, ret, "hal_cipher_symc_config failed, ret is 0x%x\n", ret);

#if defined(CONFIG_SYMC_CTR_NON_ALIGN_SUPPORT)
    symc_ctx->ctr_offset = 0;
#endif
    crypto_drv_func_exit();
    return ret;
}

td_s32 drv_cipher_symc_set_config(td_handle symc_handle, const crypto_symc_ctrl_t *symc_ctrl)
{
    td_s32 ret;
    td_u32 chn_num = symc_handle;
    drv_symc_context_t *symc_ctx = TD_NULL;
    crypto_drv_func_enter();

    ret = inner_symc_drv_handle_chk(symc_handle);
    if (ret != TD_SUCCESS) {
        return ret;
    }
    symc_null_ptr_chk(symc_ctrl);
    symc_ctx = &g_drv_symc_ctx_list[chn_num];
    crypto_chk_return(symc_ctx->is_open == TD_FALSE, SYMC_COMPAT_ERRNO(ERROR_CTX_CLOSED), "ctx is closed\n");

    symc_ctx->work_mode = symc_ctrl->work_mode;
    symc_ctx->iv_change_flag = symc_ctrl->iv_change_flag;
    switch (symc_ctrl->work_mode) {
        case CRYPTO_SYMC_WORK_MODE_CCM:
#if defined(CONFIG_SYMC_CCM_SUPPORT)
            ret = drv_cipher_symc_ccm_set_config(symc_ctx, symc_ctrl);
            break;
#else
            crypto_log_err("ccm unsupport\n");
            return SYMC_COMPAT_ERRNO(ERROR_UNSUPPORT);
#endif
        case CRYPTO_SYMC_WORK_MODE_GCM:
#if defined(CONFIG_SYMC_GCM_SUPPORT)
            ret = drv_cipher_symc_gcm_set_config(symc_ctx, symc_ctrl);
            break;
#else
            crypto_log_err("gcm unsupport\n");
            return SYMC_COMPAT_ERRNO(ERROR_UNSUPPORT);
#endif
        default:
            ret = drv_cipher_symc_normal_set_config(symc_ctx, symc_ctrl);
            break;
    }

    symc_ctx->is_config = TD_TRUE;
    crypto_drv_func_exit();
    return ret;
}

td_s32 drv_cipher_symc_attach(td_handle symc_handle, td_handle keyslot_handle)
{
    td_s32 ret;
    td_u32 symc_chn_num = symc_handle;
    td_u32 keyslot_chn_num = keyslot_handle;
    drv_symc_context_t *drv_symc_ctx = TD_NULL;

    crypto_drv_func_enter();
    ret = inner_symc_drv_handle_chk(symc_handle);
    if (ret != TD_SUCCESS) {
        return ret;
    }

    drv_symc_ctx = &g_drv_symc_ctx_list[symc_chn_num];
    crypto_chk_return(drv_symc_ctx->is_open == TD_FALSE, SYMC_COMPAT_ERRNO(ERROR_CTX_CLOSED), "ctx is closed\n");
    crypto_chk_return(drv_symc_ctx->is_create_keyslot == TD_TRUE, SYMC_COMPAT_ERRNO(ERROR_CTX_CLOSED),
        "has been create\n");

    ret = hal_cipher_symc_attach(symc_chn_num, keyslot_chn_num);
    if (ret != TD_SUCCESS) {
        crypto_log_err("hal_cipher_symc_attach failed, ret is 0x%x\n", ret);
        return ret;
    }

    drv_symc_ctx->keyslot_handle = keyslot_handle;
    drv_symc_ctx->is_attached = TD_TRUE;

    crypto_drv_func_exit();
    return ret;
}

int32_t inner_drv_symc_common_process(drv_symc_context_t *symc_ctx, td_u64 src_phys_addr, td_u64 dst_phys_addr,
    td_u32 length, td_u32 node_type)
{
    td_s32 ret;
    td_u32 chn_num = symc_ctx->chn_num;

#if defined(CONFIG_SYMC_TRACE_ENABLE)
    crypto_dump_data("set iv", symc_ctx->iv, sizeof(symc_ctx->iv));
    crypto_dump_phys_addr("src_buf", src_phys_addr, length);
#endif

    ret = hal_cipher_symc_set_iv(chn_num, symc_ctx->iv, sizeof(symc_ctx->iv));
    crypto_chk_return(ret != TD_SUCCESS, ret, "hal_cipher_symc_get_iv failed\n");

    ret = hal_cipher_symc_add_in_node(chn_num, src_phys_addr, length, node_type);
    crypto_chk_return(ret != CRYPTO_SUCCESS, ret, "hal_cipher_symc_add_in_node failed\n");

    ret = hal_cipher_symc_add_out_node(chn_num, dst_phys_addr, length);
    crypto_chk_return(ret != CRYPTO_SUCCESS, ret, "hal_cipher_symc_add_out_node failed\n");

    ret = hal_cipher_symc_start(chn_num);
    crypto_chk_return(ret != TD_SUCCESS, ret, "hal_cipher_symc_start failed\n");

    ret = hal_cipher_symc_wait_done(chn_num, TD_TRUE);
    crypto_chk_return(ret != TD_SUCCESS, ret, "hal_cipher_symc_wait_done failed\n");

    ret = hal_cipher_symc_get_iv(chn_num, symc_ctx->iv, sizeof(symc_ctx->iv));
    crypto_chk_return(ret != TD_SUCCESS, ret, "hal_cipher_symc_get_iv failed\n");

#if defined(CONFIG_SYMC_TRACE_ENABLE)
    crypto_dump_data("get iv", symc_ctx->iv, sizeof(symc_ctx->iv));
    crypto_dump_phys_addr("dst_buf", dst_phys_addr, length);
#endif

    return ret;
}

static td_s32 inner_drv_cipher_symc_crypto(td_handle symc_handle, const crypto_buf_attr *src_buf,
    const crypto_buf_attr *dst_buf, td_u32 length, td_bool is_decrypt)
{
    td_s32 ret;
    drv_symc_context_t *symc_ctx = TD_NULL;
    td_u32 chn_num = (td_u32)symc_handle;
    crypto_drv_func_enter();

    ret = inner_drv_symc_crypto_chk(symc_handle, src_buf, dst_buf, length);
    crypto_chk_return(ret != TD_SUCCESS, ret, "inner_drv_symc_crypto_chk failed\n");

    symc_ctx = &g_drv_symc_ctx_list[chn_num];
    crypto_chk_return(symc_ctx->is_open == TD_FALSE, SYMC_COMPAT_ERRNO(ERROR_CTX_CLOSED), "ctx is closed\n");
    crypto_chk_return(symc_ctx->is_config == TD_FALSE,
        SYMC_COMPAT_ERRNO(ERROR_NOT_SET_CONFIG), "call set_config first\n");
    crypto_chk_return(symc_ctx->is_attached == TD_FALSE, SYMC_COMPAT_ERRNO(ERROR_NOT_ATTACHED), "call attach first\n");

    symc_ctx->is_decrypt = is_decrypt;

    (td_void)hal_cipher_set_chn_secure(chn_num, dst_buf->buf_sec == CRYPTO_BUF_SECURE,
        src_buf->buf_sec == CRYPTO_BUF_SECURE);

    (void)hal_cipher_symc_set_flag(chn_num, is_decrypt);

    switch (symc_ctx->work_mode) {
        case CRYPTO_SYMC_WORK_MODE_CCM:
#if defined(CONFIG_SYMC_CCM_SUPPORT)
            ret = inner_drv_cipher_symc_ccm_process(symc_ctx, src_buf->phys_addr, dst_buf->phys_addr, length);
            crypto_chk_return(ret != TD_SUCCESS, ret, "inner_drv_cipher_symc_ccm_process failed\n");
            break;
#else
            crypto_log_err("ccm unsupport\n");
            return SYMC_COMPAT_ERRNO(ERROR_UNSUPPORT);
#endif
        case CRYPTO_SYMC_WORK_MODE_GCM:
#if defined(CONFIG_SYMC_GCM_SUPPORT)
            ret = inner_drv_cipher_symc_gcm_process(symc_ctx, src_buf->phys_addr, dst_buf->phys_addr, length);
            crypto_chk_return(ret != TD_SUCCESS, ret, "inner_drv_cipher_symc_gcm_process failed\n");
            break;
#else
            crypto_log_err("gcm unsupport\n");
            return SYMC_COMPAT_ERRNO(ERROR_UNSUPPORT);
#endif
#if defined(CONFIG_SYMC_CTR_NON_ALIGN_SUPPORT)
        case CRYPTO_SYMC_WORK_MODE_CTR:
            ret = inner_drv_cipher_symc_ctr_process_non_align(symc_ctx, src_buf->phys_addr,
                dst_buf->phys_addr, length);
            crypto_chk_return(ret != TD_SUCCESS, ret, "inner_drv_cipher_symc_ctr_process_non_align failed\n");
            break;
#endif
        default:
            ret = inner_drv_symc_common_process(symc_ctx, src_buf->phys_addr, dst_buf->phys_addr, length,
                IN_NODE_TYPE_FIRST | IN_NODE_TYPE_LAST);
            crypto_chk_return(ret != TD_SUCCESS, ret, "inner_drv_symc_common_process failed\n");
            break;
    }

    crypto_drv_func_exit();
    return ret;
}

td_s32 drv_cipher_symc_encrypt(td_handle symc_handle, const crypto_buf_attr *src_buf, const crypto_buf_attr *dst_buf,
    td_u32 length)
{
    td_s32 ret;
    crypto_drv_func_enter();

    ret = inner_drv_cipher_symc_crypto(symc_handle, src_buf, dst_buf, length, TD_FALSE);
    if (ret != TD_SUCCESS) {
        crypto_log_err("inner_drv_cipher_symc_crypto failed, ret is 0x%x\n", ret);
        return ret;
    }

    crypto_drv_func_exit();
    return ret;
}

td_s32 drv_cipher_symc_decrypt(td_handle symc_handle, const crypto_buf_attr *src_buf, const crypto_buf_attr *dst_buf,
    td_u32 length)
{
    td_s32 ret;
    crypto_drv_func_enter();

    ret = inner_drv_cipher_symc_crypto(symc_handle, src_buf, dst_buf, length, TD_TRUE);
    if (ret != TD_SUCCESS) {
        crypto_log_err("inner_drv_cipher_symc_crypto failed, ret is 0x%x\n", ret);
        return ret;
    }

    crypto_drv_func_exit();
    return ret;
}

td_s32 drv_cipher_symc_get_tag(td_handle symc_handle, td_u8 *tag, td_u32 tag_length)
{
    td_s32 ret;
    td_u32 chn_num;
    drv_symc_context_t *symc_ctx = TD_NULL;
    crypto_drv_func_enter();
    ret = inner_symc_drv_handle_chk(symc_handle);
    if (ret != TD_SUCCESS) {
        return ret;
    }
    symc_null_ptr_chk(tag);
    crypto_chk_return(tag_length == 0 || tag_length > CRYPTO_AES_MAX_TAG_SIZE,
        SYMC_COMPAT_ERRNO(ERROR_INVALID_PARAM), "tag_length is invalid\n");
    chn_num = symc_handle;
    symc_ctx = &g_drv_symc_ctx_list[chn_num];
    crypto_chk_return(symc_ctx->is_open == TD_FALSE, SYMC_COMPAT_ERRNO(ERROR_CTX_CLOSED), "ctx is closed\n");
    crypto_chk_return(symc_ctx->is_config == TD_FALSE,
        SYMC_COMPAT_ERRNO(ERROR_NOT_SET_CONFIG), "call set_config first\n");
    crypto_chk_return(symc_ctx->is_attached == TD_FALSE, SYMC_COMPAT_ERRNO(ERROR_NOT_ATTACHED), "call attach first\n");

    switch (symc_ctx->work_mode) {
        case CRYPTO_SYMC_WORK_MODE_CCM:
#if defined(CONFIG_SYMC_CCM_SUPPORT)
            ret = inner_drv_cipher_symc_ccm_get_tag(symc_ctx, tag, tag_length);
            break;
#else
            crypto_log_err("unsupport ccm\n");
            return SYMC_COMPAT_ERRNO(ERROR_UNSUPPORT);
#endif
        case CRYPTO_SYMC_WORK_MODE_GCM:
#if defined(CONFIG_SYMC_GCM_SUPPORT)
            ret = inner_drv_cipher_symc_gcm_get_tag(symc_ctx, tag, tag_length);
            break;
#else
            crypto_log_err("unsupport gcm\n");
            return SYMC_COMPAT_ERRNO(ERROR_UNSUPPORT);
#endif
        default:
            crypto_log_err("invalid work_mode\n");
            return SYMC_COMPAT_ERRNO(ERROR_UNSUPPORT);
    }

    crypto_drv_func_exit();
    return ret;
}

td_s32 inner_drv_symc_crypto_chk(td_handle symc_handle, const crypto_buf_attr *src_buf,
    const crypto_buf_attr *dst_buf, td_u32 length)
{
    td_s32 ret;
    drv_symc_context_t *symc_ctx = TD_NULL;
    crypto_symc_work_mode mode;

    ret = inner_symc_drv_handle_chk(symc_handle);
    if (ret != TD_SUCCESS) {
        return ret;
    }
    symc_ctx = &g_drv_symc_ctx_list[symc_handle];

    symc_null_ptr_chk(src_buf);
    symc_null_ptr_chk(dst_buf);
    crypto_chk_return(src_buf->phys_addr == 0, SYMC_COMPAT_ERRNO(ERROR_INVALID_PARAM),
        "src_buf's phys_addr is invalid\n");
    crypto_chk_return(dst_buf->phys_addr == 0, SYMC_COMPAT_ERRNO(ERROR_INVALID_PARAM),
        "dst_buf's phys_addr is invalid\n");
    crypto_chk_return(src_buf->buf_sec != CRYPTO_BUF_SECURE && src_buf->buf_sec != CRYPTO_BUF_NONSECURE,
        SYMC_COMPAT_ERRNO(ERROR_INVALID_PARAM), "src_buf's buf_sec is invalid\n");
    crypto_chk_return(dst_buf->buf_sec != CRYPTO_BUF_SECURE && dst_buf->buf_sec != CRYPTO_BUF_NONSECURE,
        SYMC_COMPAT_ERRNO(ERROR_INVALID_PARAM), "dst_buf's buf_sec is invalid\n");

    crypto_chk_return(length == 0, SYMC_COMPAT_ERRNO(ERROR_INVALID_PARAM), "length is zero!\n");
    mode = symc_ctx->work_mode;
    if (mode == CRYPTO_SYMC_WORK_MODE_CBC || mode == CRYPTO_SYMC_WORK_MODE_ECB) {
        crypto_chk_return((length % CRYPTO_AES_BLOCK_SIZE_IN_BYTES) != 0, SYMC_COMPAT_ERRNO(ERROR_SYMC_LEN_NOT_ALIGNED),
            "length must be aligned to 16-Byte\n");
    }
    return TD_SUCCESS;
}

td_s32 inner_drv_symc_set_iv(td_handle symc_handle, const td_u8 *iv, td_u32 iv_len)
{
    td_s32 ret;
    drv_symc_context_t *symc_ctx = TD_NULL;

    symc_ctx = inner_get_symc_ctx(symc_handle);
    crypto_chk_return(symc_ctx == TD_NULL, SYMC_COMPAT_ERRNO(ERROR_INVALID_HANDLE), "invalid handle\n");
    crypto_chk_return(symc_ctx->is_open == TD_FALSE, SYMC_COMPAT_ERRNO(ERROR_CTX_CLOSED), "ctx is closed\n");

    ret = memcpy_s(symc_ctx->iv, sizeof(symc_ctx->iv), iv, iv_len);
    crypto_chk_return(ret != EOK, SYMC_COMPAT_ERRNO(ERROR_MEMCPY_S), "memcpy_s failed\n");

    return TD_SUCCESS;
}

td_s32 inner_drv_symc_get_iv(td_handle symc_handle, td_u8 *iv, td_u32 iv_len)
{
    td_s32 ret;
    drv_symc_context_t *symc_ctx = TD_NULL;

    symc_ctx = inner_get_symc_ctx(symc_handle);
    crypto_chk_return(symc_ctx == TD_NULL, SYMC_COMPAT_ERRNO(ERROR_INVALID_HANDLE), "invalid handle\n");
    crypto_chk_return(symc_ctx->is_open == TD_FALSE, SYMC_COMPAT_ERRNO(ERROR_CTX_CLOSED), "ctx is closed\n");

    ret = memcpy_s(iv, iv_len, symc_ctx->iv, sizeof(symc_ctx->iv));
    crypto_chk_return(ret != EOK, SYMC_COMPAT_ERRNO(ERROR_MEMCPY_S), "memcpy_s failed\n");

    return TD_SUCCESS;
}