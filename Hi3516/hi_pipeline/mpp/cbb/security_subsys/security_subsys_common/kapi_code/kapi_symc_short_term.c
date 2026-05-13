/**
  Copyright (c), 2001-2025, Shenshu Tech. Co., Ltd.
 */

#include "kapi_symc_inner.h"
#include "drv_symc_outer.h"
#include "drv_symc.h"

td_s32 inner_kapi_symc_create_short_term(crypto_kapi_symc_ctx *symc_ctx, const crypto_symc_attr *symc_attr)
{
    td_s32 ret;

    ret = memcpy_s(&symc_ctx->symc_attr, sizeof(crypto_symc_attr), symc_attr, sizeof(crypto_symc_attr));
    crypto_chk_return(ret != EOK, SYMC_COMPAT_ERRNO(ERROR_MEMCPY_S), "memcpy_s failed\n");

    return CRYPTO_SUCCESS;
}

static td_s32 inner_kapi_symc_set_ctx(crypto_kapi_symc_ctx *symc_ctx)
{
    int ret;
    crypto_symc_work_mode work_mode = symc_ctx->symc_ctrl.work_mode;

    if (work_mode == CRYPTO_SYMC_WORK_MODE_CCM) {
        ret = inner_drv_symc_ccm_set_ctx(symc_ctx->drv_symc_handle, &symc_ctx->ccm_ctx);
        crypto_chk_return(ret != TD_SUCCESS, ret, "inner_drv_symc_ccm_set_ctx failed, ret is 0x%x\n", ret);
    } else if (work_mode == CRYPTO_SYMC_WORK_MODE_GCM) {
        ret = inner_drv_symc_gcm_set_ctx(symc_ctx->drv_symc_handle, &symc_ctx->gcm_ctx);
        crypto_chk_return(ret != TD_SUCCESS, ret, "inner_drv_symc_gcm_set_ctx failed, ret is 0x%x\n", ret);
    }

    return CRYPTO_SUCCESS;
}

static td_s32 inner_kapi_symc_get_ctx(crypto_kapi_symc_ctx *symc_ctx)
{
    int ret;
    crypto_symc_work_mode work_mode = symc_ctx->symc_ctrl.work_mode;

    if (work_mode == CRYPTO_SYMC_WORK_MODE_CCM) {
        ret = inner_drv_symc_ccm_get_ctx(symc_ctx->drv_symc_handle, &symc_ctx->ccm_ctx);
        crypto_chk_return(ret != TD_SUCCESS, ret, "inner_drv_symc_ccm_get_ctx failed, ret is 0x%x\n", ret);
    } else if (work_mode == CRYPTO_SYMC_WORK_MODE_GCM) {
        ret = inner_drv_symc_gcm_get_ctx(symc_ctx->drv_symc_handle, &symc_ctx->gcm_ctx);
        crypto_chk_return(ret != TD_SUCCESS, ret, "inner_drv_symc_gcm_get_ctx failed, ret is 0x%x\n", ret);
    }

    return CRYPTO_SUCCESS;
}

static td_s32 inner_kapi_symc_set_config_ex_short_term(crypto_kapi_symc_ctx *symc_ctx,
    const crypto_symc_ctrl_t *symc_ctrl)
{
    td_s32 ret;
    symc_null_ptr_chk(symc_ctrl->param);

    ret = drv_cipher_symc_create(&symc_ctx->drv_symc_handle, &symc_ctx->symc_attr);
    crypto_chk_return(ret != TD_SUCCESS, ret, "inner_kapi_drv_symc_create_lock failed, ret is 0x%x\n", ret);

    ret = drv_cipher_symc_attach(symc_ctx->drv_symc_handle, symc_ctx->keyslot_handle);
    crypto_chk_goto(ret != TD_SUCCESS, symc_destroy_exit,
        "drv_cipher_symc_attach failed, ret is 0x%x\n", ret);

    if (symc_ctrl->iv_change_flag == CRYPTO_SYMC_CCM_IV_CHANGE_UPDATE ||
        symc_ctrl->iv_change_flag == CRYPTO_SYMC_GCM_IV_CHANGE_UPDATE) {
        ret = inner_kapi_symc_set_ctx(symc_ctx);
        crypto_chk_goto(ret != TD_SUCCESS, symc_destroy_exit, "inner_kapi_symc_set_ctx failed, ret is 0x%x\n", ret);
    }

    ret = drv_cipher_symc_set_config(symc_ctx->drv_symc_handle, symc_ctrl);
    crypto_chk_goto(ret != TD_SUCCESS, symc_destroy_exit, "drv_cipher_symc_set_config failed, ret is 0x%x\n", ret);

    ret = inner_kapi_symc_get_ctx(symc_ctx);
    crypto_chk_goto(ret != TD_SUCCESS, symc_destroy_exit, "inner_kapi_symc_get_ctx failed, ret is 0x%x\n", ret);

    if (symc_ctrl->iv_change_flag == CRYPTO_SYMC_CCM_IV_CHANGE_START ||
        symc_ctrl->iv_change_flag == CRYPTO_SYMC_GCM_IV_CHANGE_START) {
        ret = inner_drv_symc_get_iv(symc_ctx->drv_symc_handle, symc_ctx->symc_ctrl.iv, sizeof(symc_ctx->symc_ctrl.iv));
        crypto_chk_goto(ret != TD_SUCCESS, symc_destroy_exit, "inner_drv_symc_get_iv failed, ret is 0x%x\n", ret);
    }

symc_destroy_exit:
    (td_void)drv_cipher_symc_destroy(symc_ctx->drv_symc_handle);
    return ret;
}

td_s32 inner_kapi_symc_set_config_short_term(crypto_kapi_symc_ctx *symc_ctx, const crypto_symc_ctrl_t *symc_ctrl)
{
    td_s32 ret;
    crypto_symc_work_mode work_mode = symc_ctrl->work_mode;
    crypto_chk_return(
        work_mode == CRYPTO_SYMC_WORK_MODE_CCM && symc_ctrl->iv_change_flag != CRYPTO_SYMC_CCM_IV_CHANGE_START,
        SYMC_COMPAT_ERRNO(ERROR_INVALID_PARAM),
        "error setting iv_change_flag, it should be set to CRYPTO_SYMC_CCM_IV_CHANGE_START\n");
    crypto_chk_return(
        work_mode == CRYPTO_SYMC_WORK_MODE_GCM && symc_ctrl->iv_change_flag != CRYPTO_SYMC_GCM_IV_CHANGE_START,
        SYMC_COMPAT_ERRNO(ERROR_INVALID_PARAM),
        "error setting iv_change_flag, it should be set to CRYPTO_SYMC_GCM_IV_CHANGE_START\n");

    if (work_mode == CRYPTO_SYMC_WORK_MODE_CCM || work_mode == CRYPTO_SYMC_WORK_MODE_GCM) {
        symc_ctx->symc_ctrl.symc_alg = symc_ctrl->symc_alg;
        symc_ctx->symc_ctrl.work_mode = symc_ctrl->work_mode;
        symc_ctx->symc_ctrl.symc_key_length = symc_ctrl->symc_key_length;
        symc_ctx->symc_ctrl.symc_bit_width = symc_ctrl->symc_bit_width;
        return inner_kapi_symc_set_config_ex_short_term(symc_ctx, symc_ctrl);
    } else {
        ret = memcpy_s(&symc_ctx->symc_ctrl, sizeof(crypto_symc_ctrl_t), symc_ctrl, sizeof(crypto_symc_ctrl_t));
        crypto_chk_return(ret != EOK, SYMC_COMPAT_ERRNO(ERROR_MEMCPY_S), "memcpy_s failed\n");
        symc_ctx->symc_ctrl.param = NULL;
    }

    return CRYPTO_SUCCESS;
}

td_s32 inner_kapi_symc_get_config_short_term(crypto_kapi_symc_ctx *symc_ctx, crypto_symc_ctrl_t *symc_ctrl)
{
    crypto_unused(symc_ctx);
    crypto_unused(symc_ctrl);

    return CRYPTO_SUCCESS;
}

td_s32 inner_kapi_symc_attach_short_term(crypto_kapi_symc_ctx *symc_ctx, td_handle keyslot_handle)
{
    symc_ctx->keyslot_handle = keyslot_handle;
    return CRYPTO_SUCCESS;
}

td_s32 inner_kapi_symc_crypto_short_term(crypto_kapi_symc_ctx *symc_ctx,
    const crypto_buf_attr *src_buf, const crypto_buf_attr *dst_buf, td_u32 length, td_u32 crypto_type)
{
    td_s32 ret;

    ret = drv_cipher_symc_create(&symc_ctx->drv_symc_handle, &symc_ctx->symc_attr);
    crypto_chk_return(ret != TD_SUCCESS, ret, "inner_kapi_drv_symc_create_lock failed, ret is 0x%x\n", ret);

    ret = drv_cipher_symc_attach(symc_ctx->drv_symc_handle, symc_ctx->keyslot_handle);
    crypto_chk_goto(ret != TD_SUCCESS, symc_destroy_exit,
        "drv_cipher_symc_attach for crypto failed, ret is 0x%x\n", ret);

    if (symc_ctx->symc_ctrl.work_mode == CRYPTO_SYMC_WORK_MODE_CCM) {
        symc_ctx->symc_ctrl.iv_change_flag = CRYPTO_SYMC_CCM_IV_CHANGE_FINISH;
    } else if (symc_ctx->symc_ctrl.work_mode == CRYPTO_SYMC_WORK_MODE_GCM) {
        symc_ctx->symc_ctrl.iv_change_flag = CRYPTO_SYMC_GCM_IV_CHANGE_FINISH;
    }
    ret = drv_cipher_symc_set_config(symc_ctx->drv_symc_handle, &symc_ctx->symc_ctrl);
    crypto_chk_goto(ret != TD_SUCCESS, symc_destroy_exit,
        "drv_cipher_symc_set_config for crypto failed, ret is 0x%x\n", ret);

    ret = inner_drv_symc_set_iv(symc_ctx->drv_symc_handle, symc_ctx->symc_ctrl.iv, sizeof(symc_ctx->symc_ctrl.iv));
    crypto_chk_goto(ret != TD_SUCCESS, symc_destroy_exit, "inner_drv_symc_set_iv failed, ret is 0x%x\n", ret);

    ret = inner_kapi_symc_set_ctx(symc_ctx);
    crypto_chk_goto(ret != TD_SUCCESS, symc_destroy_exit, "inner_kapi_symc_set_ctx failed, ret is 0x%x\n", ret);

    if (crypto_type == CRYPTO_TYPE_ENCRYPT) {
        ret = drv_cipher_symc_encrypt(symc_ctx->drv_symc_handle, src_buf, dst_buf, length);
    } else {
        ret = drv_cipher_symc_decrypt(symc_ctx->drv_symc_handle, src_buf, dst_buf, length);
    }
    crypto_chk_goto(ret != TD_SUCCESS, symc_destroy_exit,
        "inner_kapi_cipher_symc_crypto for crypto failed, ret is 0x%x\n", ret);

    /* Update iv after crypto. */
    ret = inner_drv_symc_get_iv(symc_ctx->drv_symc_handle, symc_ctx->symc_ctrl.iv, sizeof(symc_ctx->symc_ctrl.iv));
    crypto_chk_goto(ret != TD_SUCCESS, symc_destroy_exit,
        "inner_drv_symc_get_iv for crypto failed, ret is 0x%x\n", ret);

    ret = inner_kapi_symc_get_ctx(symc_ctx);
    crypto_chk_goto(ret != TD_SUCCESS, symc_destroy_exit, "inner_kapi_symc_get_ctx failed, ret is 0x%x\n", ret);

symc_destroy_exit:
    (td_void)drv_cipher_symc_destroy(symc_ctx->drv_symc_handle);
    return ret;
}

td_s32 inner_kapi_symc_get_tag_short_term(crypto_kapi_symc_ctx *symc_ctx, td_u8 *tag, td_u32 tag_len)
{
    td_s32 ret;

    ret = drv_cipher_symc_create(&symc_ctx->drv_symc_handle, &symc_ctx->symc_attr);
    crypto_chk_return(ret != TD_SUCCESS, ret, "inner_kapi_drv_symc_create_lock failed, ret is 0x%x\n", ret);

    ret = drv_cipher_symc_attach(symc_ctx->drv_symc_handle, symc_ctx->keyslot_handle);
    crypto_chk_goto(ret != TD_SUCCESS, symc_destroy_exit,
        "drv_cipher_symc_attach for crypto failed, ret is 0x%x\n", ret);

    ret = drv_cipher_symc_set_config(symc_ctx->drv_symc_handle, &symc_ctx->symc_ctrl);
    crypto_chk_goto(ret != TD_SUCCESS, symc_destroy_exit,
        "drv_cipher_symc_set_config for crypto failed, ret is 0x%x\n", ret);

    ret = inner_kapi_symc_set_ctx(symc_ctx);
    crypto_chk_goto(ret != TD_SUCCESS, symc_destroy_exit, "inner_kapi_symc_set_ctx failed, ret is 0x%x\n", ret);

    ret = drv_cipher_symc_get_tag(symc_ctx->drv_symc_handle, tag, tag_len);
    crypto_chk_goto(ret != TD_SUCCESS, symc_destroy_exit, "drv_cipher_symc_get_tag failed\n");

symc_destroy_exit:
    (td_void)drv_cipher_symc_destroy(symc_ctx->drv_symc_handle);
    return ret;
}