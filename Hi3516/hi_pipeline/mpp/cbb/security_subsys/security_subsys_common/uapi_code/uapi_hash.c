/*
  Copyright (c), 2001-2025, Shenshu Tech. Co., Ltd.
 */

#include "uapi_hash.h"

#include <securec.h>

#include "uapi_common.h"
#include "crypto_ioctl_cmd.h"
#include "crypto_osal_user_lib.h"

#define HASH_COMPAT_ERRNO(err_code)     UAPI_COMPAT_ERRNO(ERROR_MODULE_HASH, err_code)

#define CRYPTO_HASH_MAX_PROCESS_LEN         (4 * 1024)

#define crypto_hash_init_chk() do {                 \
    if (crypto_cipher_is_init() == 0) {                 \
        return HASH_COMPAT_ERRNO(ERROR_NOT_INIT);   \
    }                                               \
} while (0)

#if defined(CONFIG_HASH_SUPPORT)
td_s32 unify_uapi_cipher_hash_init(td_void)
{
    td_s32 ret = CRYPTO_SUCCESS;

    crypto_uapi_func_enter();

    ret = crypto_cipher_open();
    if (ret != TD_SUCCESS) {
        return ret;
    }

    ret = crypto_cipher_ioctl(CRYPTO_CMD_HASH_INIT, TD_NULL);
    if (ret != CRYPTO_SUCCESS) {
        printf("crypto_ioctl failed, ret is 0x%x\n", ret);
        crypto_cipher_close();
        return ret;
    }

    crypto_uapi_func_exit();
    return ret;
}
#else
td_s32 unify_uapi_cipher_hash_init(td_void)
{
    return ERROR_UNSUPPORT;
}
#endif

#if defined(CONFIG_HASH_SUPPORT)
td_s32 unify_uapi_cipher_hash_deinit(td_void)
{
    td_s32 ret = CRYPTO_SUCCESS;

    crypto_uapi_func_enter();

    if (crypto_cipher_is_init() == 0) {
        return CRYPTO_SUCCESS;
    }

    ret = crypto_cipher_ioctl(CRYPTO_CMD_HASH_DEINIT, TD_NULL);
    if (ret != CRYPTO_SUCCESS) {
        printf("crypto_ioctl failed, ret is 0x%x\n", ret);
    }

    crypto_cipher_close();

    crypto_uapi_func_exit();
    return ret;
}
#else
td_s32 unify_uapi_cipher_hash_deinit(td_void)
{
    return ERROR_UNSUPPORT;
}
#endif

#if defined(CONFIG_HASH_SUPPORT)
td_s32 unify_uapi_cipher_hash_start(td_handle *uapi_hash_handle, const crypto_hash_attr *hash_attr)
{
    td_s32 ret = CRYPTO_SUCCESS;
    hash_start_ctl_t start_ctl;

    crypto_uapi_func_enter();
    crypto_hash_init_chk();

    crypto_chk_return(uapi_hash_handle == TD_NULL, HASH_COMPAT_ERRNO(ERROR_PARAM_IS_NULL),
        "uapi_hash_handle is NULL\n");
    crypto_chk_return(hash_attr == TD_NULL, HASH_COMPAT_ERRNO(ERROR_PARAM_IS_NULL), "hash_attr is NULL\n");

    (td_void)memset_s(&start_ctl, sizeof(start_ctl), 0, sizeof(start_ctl));
    start_ctl.hash_type = hash_attr->hash_type;
    start_ctl.is_long_term = hash_attr->is_long_term;
    start_ctl.is_keyslot = hash_attr->is_keyslot;
    start_ctl.key.p = hash_attr->key;
    start_ctl.key_len = hash_attr->key_len;
    start_ctl.drv_keyslot_handle = hash_attr->drv_keyslot_handle;

    ret = crypto_cipher_ioctl(CRYPTO_CMD_HASH_START, &start_ctl);
    if (ret != TD_SUCCESS) {
        printf("crypto_ioctl failed, ret is 0x%x\n", ret);
        return ret;
    }

    *uapi_hash_handle = start_ctl.kapi_hash_handle;

    crypto_uapi_func_exit();
    return ret;
}
#else
td_s32 unify_uapi_cipher_hash_start(td_handle *uapi_hash_handle, const crypto_hash_attr *hash_attr)
{
    crypto_unused(uapi_hash_handle);
    crypto_unused(hash_attr);
    return ERROR_UNSUPPORT;
}
#endif

#if defined(CONFIG_HASH_SUPPORT)
td_s32 unify_uapi_cipher_hash_update(td_handle uapi_hash_handle, const crypto_buf_attr *src_buf, const td_u32 len)
{
    td_s32 ret = CRYPTO_SUCCESS;
    hash_update_ctl_t update_ctl;

    crypto_uapi_func_enter();
    crypto_hash_init_chk();

    crypto_chk_return(src_buf == TD_NULL, HASH_COMPAT_ERRNO(ERROR_PARAM_IS_NULL), "src_buf is NULL\n");

    (td_void)memset_s(&update_ctl, sizeof(update_ctl), 0, sizeof(update_ctl));
    update_ctl.kapi_hash_handle = uapi_hash_handle;
    update_ctl.src_buf.p = src_buf->virt_addr;
    update_ctl.len = len;
    ret = crypto_cipher_ioctl(CRYPTO_CMD_HASH_UPDATE, &update_ctl);
    if (ret != TD_SUCCESS) {
        printf("crypto_ioctl failed, ret is 0x%x\n", ret);
        return ret;
    }

    crypto_uapi_func_exit();
    return ret;
}
#else
td_s32 unify_uapi_cipher_hash_update(td_handle uapi_hash_handle, const crypto_buf_attr *src_buf, const td_u32 len)
{
    crypto_unused(uapi_hash_handle);
    crypto_unused(src_buf);
    crypto_unused(len);
    return ERROR_UNSUPPORT;
}
#endif

#if defined(CONFIG_HASH_SUPPORT)
td_s32 unify_uapi_cipher_hash_finish(td_handle uapi_hash_handle, td_u8 *out, td_u32 *out_len)
{
    td_s32 ret = CRYPTO_SUCCESS;
    hash_finish_ctl_t finish_ctl;

    crypto_uapi_func_enter();
    crypto_hash_init_chk();

    crypto_chk_return(out_len == TD_NULL, HASH_COMPAT_ERRNO(ERROR_PARAM_IS_NULL), "out_len is NULL\n");

    (td_void)memset_s(&finish_ctl, sizeof(finish_ctl), 0, sizeof(finish_ctl));

    finish_ctl.kapi_hash_handle = uapi_hash_handle;
    finish_ctl.out.p = out;
    finish_ctl.out_len = *out_len;

    ret = crypto_cipher_ioctl(CRYPTO_CMD_HASH_FINISH, &finish_ctl);
    if (ret != TD_SUCCESS) {
        printf("crypto_ioctl failed, ret is 0x%x\n", ret);
        return ret;
    }
    crypto_chk_return(*out_len < finish_ctl.out_len, HASH_COMPAT_ERRNO(ERROR_INVALID_PARAM), "out_len is not enough\n");
    *out_len = finish_ctl.out_len;

    crypto_uapi_func_exit();
    return ret;
}
#else
td_s32 unify_uapi_cipher_hash_finish(td_handle uapi_hash_handle, td_u8 *out, td_u32 *out_len)
{
    crypto_unused(uapi_hash_handle);
    crypto_unused(out);
    crypto_unused(out_len);
    return ERROR_UNSUPPORT;
}
#endif

#if defined(CONFIG_HASH_CLONE_SUPPORT)
td_s32 unify_uapi_cipher_hash_get(td_handle uapi_hash_handle, crypto_hash_clone_ctx *hash_clone_ctx)
{
    td_s32 ret = CRYPTO_SUCCESS;
    hash_clone_ctl_t clone_ctl;

    crypto_uapi_func_enter();
    crypto_hash_init_chk();

    (td_void)memset_s(&clone_ctl, sizeof(clone_ctl), 0, sizeof(clone_ctl));
    clone_ctl.kapi_hash_handle = uapi_hash_handle;
    clone_ctl.hash_clone_ctx.p = (td_void *)hash_clone_ctx;
    clone_ctl.ctx_size = sizeof(crypto_hash_clone_ctx);

    ret = crypto_cipher_ioctl(CRYPTO_CMD_HASH_GET, &clone_ctl);
    if (ret != TD_SUCCESS) {
        printf("crypto_ioctl failed, ret is 0x%x\n", ret);
        return ret;
    }

    crypto_uapi_func_exit();
    return ret;
}

td_s32 unify_uapi_cipher_hash_set(td_handle uapi_hash_handle, const crypto_hash_clone_ctx *hash_clone_ctx)
{
    td_s32 ret = CRYPTO_SUCCESS;
    hash_clone_ctl_t clone_ctl;

    crypto_uapi_func_enter();
    crypto_hash_init_chk();

    (td_void)memset_s(&clone_ctl, sizeof(clone_ctl), 0, sizeof(clone_ctl));
    clone_ctl.kapi_hash_handle = uapi_hash_handle;
    clone_ctl.hash_clone_ctx.p = (td_void *)hash_clone_ctx;
    clone_ctl.ctx_size = sizeof(crypto_hash_clone_ctx);

    ret = crypto_cipher_ioctl(CRYPTO_CMD_HASH_SET, &clone_ctl);
    if (ret != TD_SUCCESS) {
        printf("crypto_ioctl failed, ret is 0x%x\n", ret);
        return ret;
    }

    crypto_uapi_func_exit();
    return ret;
}
#else
td_s32 unify_uapi_cipher_hash_get(td_handle uapi_hash_handle, crypto_hash_clone_ctx *hash_clone_ctx)
{
    crypto_unused(uapi_hash_handle);
    crypto_unused(hash_clone_ctx);
    return ERROR_UNSUPPORT;
}

td_s32 unify_uapi_cipher_hash_set(td_handle uapi_hash_handle, const crypto_hash_clone_ctx *hash_clone_ctx)
{
    crypto_unused(uapi_hash_handle);
    crypto_unused(hash_clone_ctx);
    return ERROR_UNSUPPORT;
}
#endif

#if defined(CONFIG_HASH_SUPPORT)
td_s32 unify_uapi_cipher_hash_destroy(td_handle uapi_hash_handle)
{
    td_s32 ret = CRYPTO_SUCCESS;
    hash_handle_ctl_t handle_ctl;

    crypto_uapi_func_enter();
    crypto_hash_init_chk();

    (td_void)memset_s(&handle_ctl, sizeof(handle_ctl), 0, sizeof(handle_ctl));
    handle_ctl.kapi_hash_handle = uapi_hash_handle;

    ret = crypto_cipher_ioctl(CRYPTO_CMD_HASH_DESTROY, &handle_ctl);
    if (ret != TD_SUCCESS) {
        printf("crypto_ioctl failed, ret is 0x%x\n", ret);
        return ret;
    }

    crypto_uapi_func_exit();
    return ret;
}
#else
td_s32 unify_uapi_cipher_hash_destroy(td_handle uapi_hash_handle)
{
    crypto_unused(uapi_hash_handle);
    return ERROR_UNSUPPORT;
}
#endif

#if defined(CONFIG_HASH_HARDEN_INTERFACE_SUPPORT)
td_s32 unify_uapi_cipher_hash_start_impl(crypto_hash_clone_ctx *clone_ctx, crypto_hash_type hash_type)
{
    td_s32 ret = CRYPTO_SUCCESS;
    hash_start_impl_ctl_t start_ctl = {0};

    crypto_uapi_func_enter();
    crypto_chk_return(clone_ctx == TD_NULL, HASH_COMPAT_ERRNO(ERROR_PARAM_IS_NULL), "clone_ctx is NULL\n");

    ret = memcpy_s(&start_ctl.ctx, sizeof(start_ctl.ctx), clone_ctx, sizeof(crypto_hash_clone_ctx));
    crypto_chk_return(ret != EOK, HASH_COMPAT_ERRNO(ERROR_MEMCPY_S), "memcpy_s failed\n");
    start_ctl.hash_type = hash_type;
    ret = crypto_cipher_open();
    if (ret != TD_SUCCESS) {
        return ret;
    }

    ret = crypto_cipher_ioctl(CRYPTO_CMD_HASH_START_IMPL, &start_ctl);
    if (ret != TD_SUCCESS) {
        printf("crypto_ioctl failed, ret is 0x%x\n", ret);
        goto dev_close;
    }

    ret = memcpy_s(clone_ctx, sizeof(crypto_hash_clone_ctx), &start_ctl.ctx, sizeof(start_ctl.ctx));
    if (ret != EOK) {
        printf("memcpy_s failed, ret is 0x%x\n", ret);
        goto dev_close;
    }
    ret = CRYPTO_SUCCESS;
dev_close:
    crypto_cipher_close();
    crypto_uapi_func_exit();
    return ret;
}

td_s32 unify_uapi_cipher_hash_update_impl(crypto_hash_clone_ctx *clone_ctx, const td_u8 *data, td_u32 data_len)
{
    td_s32 ret = CRYPTO_SUCCESS;
    hash_update_impl_ctl_t update_ctl = {0};

    crypto_uapi_func_enter();

    crypto_chk_return(clone_ctx == TD_NULL, HASH_COMPAT_ERRNO(ERROR_PARAM_IS_NULL), "clone_ctx is NULL\n");
    crypto_chk_return(data == TD_NULL, HASH_COMPAT_ERRNO(ERROR_PARAM_IS_NULL), "data is NULL\n");

    ret = memcpy_s(&update_ctl.ctx, sizeof(update_ctl.ctx), clone_ctx, sizeof(crypto_hash_clone_ctx));
    crypto_chk_return(ret != EOK, HASH_COMPAT_ERRNO(ERROR_MEMCPY_S), "memcpy_s failed\n");
    update_ctl.data.p = (td_void *)data;
    update_ctl.data_len = data_len;

    ret = crypto_cipher_open();
    if (ret != TD_SUCCESS) {
        return ret;
    }

    ret = crypto_cipher_ioctl(CRYPTO_CMD_HASH_UPDATE_IMPL, &update_ctl);
    if (ret != TD_SUCCESS) {
        printf("crypto_ioctl failed, ret is 0x%x\n", ret);
        goto dev_close;
    }

    ret = memcpy_s(clone_ctx, sizeof(crypto_hash_clone_ctx), &update_ctl.ctx, sizeof(update_ctl.ctx));
    if (ret != EOK) {
        printf("memcpy_s failed, ret is 0x%x\n", ret);
        goto dev_close;
    }
    ret = CRYPTO_SUCCESS;
dev_close:
    crypto_cipher_close();
    crypto_uapi_func_exit();
    return ret;
}

td_s32 unify_uapi_cipher_hash_finish_impl(crypto_hash_clone_ctx *clone_ctx, td_u8 *out, td_u32 *out_len)
{
    td_s32 ret = CRYPTO_SUCCESS;
    hash_finish_impl_ctl_t finish_ctl = {0};

    crypto_uapi_func_enter();

    crypto_chk_return(clone_ctx == TD_NULL, HASH_COMPAT_ERRNO(ERROR_PARAM_IS_NULL), "clone_ctx is NULL\n");
    crypto_chk_return(out_len == TD_NULL, HASH_COMPAT_ERRNO(ERROR_PARAM_IS_NULL), "out_len is NULL\n");
    crypto_chk_return(out_len == TD_NULL, HASH_COMPAT_ERRNO(ERROR_PARAM_IS_NULL), "out_len is NULL\n");

    ret = memcpy_s(&finish_ctl.ctx, sizeof(finish_ctl.ctx), clone_ctx, sizeof(crypto_hash_clone_ctx));
    crypto_chk_return(ret != EOK, HASH_COMPAT_ERRNO(ERROR_MEMCPY_S), "memcpy_s failed\n");
    finish_ctl.out.p = out;
    finish_ctl.out_len = *out_len;

    ret = crypto_cipher_open();
    if (ret != TD_SUCCESS) {
        return ret;
    }

    ret = crypto_cipher_ioctl(CRYPTO_CMD_HASH_FINISH_IMPL, &finish_ctl);
    if (ret != TD_SUCCESS) {
        printf("crypto_ioctl failed, ret is 0x%x\n", ret);
        goto dev_close;
    }

    *out_len = finish_ctl.out_len;
    ret = CRYPTO_SUCCESS;
dev_close:
    crypto_cipher_close();
    crypto_uapi_func_exit();
    return ret;
}
#endif