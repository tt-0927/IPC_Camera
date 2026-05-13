/**
  Copyright (c), 2001-2025, Shenshu Tech. Co., Ltd.
*/

#include "kapi_km.h"

#include "crypto_common_def.h"
#include "crypto_common_macro.h"
#include "crypto_drv_common.h"
#include "drv_klad.h"
#include "drv_keyslot.h"

#if defined(CONFIG_KAPI_KM_PARAM_TRACE_ENABLE)
#define kapi_km_param_trace            crypto_param_trace
#else
#define kapi_km_param_trace(...)
#endif

typedef struct {
    crypto_owner owner;
    td_bool is_open;
    td_handle keyslot_handle;
    crypto_keyslot_type type;
} crypto_kapi_keyslot_ctx;

typedef struct {
    crypto_owner owner;
    td_bool is_attached;
    td_bool is_set_attr;
    td_bool is_set_session_key;
    td_handle drv_klad_handle;
    td_handle drv_keyslot_handle;
    crypto_klad_dest klad_type;
    crypto_klad_attr klad_attr;
    crypto_klad_session_key session_key;
    td_handle keyslot_handle;
} crypto_kapi_klad_ctx;

crypto_mutex g_klad_mutex;
crypto_mutex g_keyslot_mutex;

#define CRYPTO_MCIPHER_KEYSLOT_NUM      8
#define CRYPTO_HMAC_KEYSLOT_NUM         2
#define CRYPTO_KLAD_VIRT_NUM            16

static crypto_kapi_keyslot_ctx g_keyslot_symc_ctx_list[CRYPTO_MCIPHER_KEYSLOT_NUM] = {0};
static crypto_kapi_keyslot_ctx g_keyslot_hmac_ctx_list[CRYPTO_HMAC_KEYSLOT_NUM] = {0};

static crypto_kapi_klad_ctx *g_klad_ctx_list[CRYPTO_KLAD_VIRT_NUM];

#define kapi_klad_mutex_lock() do {          \
    crypto_log_trace("klad mutext lock");   \
    crypto_mutex_lock(&g_klad_mutex);            \
} while (0)

#define kapi_klad_mutex_unlock() do {        \
    crypto_log_trace("klad mutext unlock");   \
    crypto_mutex_unlock(&g_klad_mutex);          \
} while (0)

#define kapi_keyslot_mutex_lock() do {          \
    crypto_log_trace("keyslot mutext lock");   \
    crypto_mutex_lock(&g_keyslot_mutex);            \
} while (0)

#define kapi_keyslot_mutex_unlock() do {        \
    crypto_log_trace("keyslot mutext unlock");   \
    crypto_mutex_unlock(&g_keyslot_mutex);          \
} while (0)

#define KM_COMPAT_ERRNO(err_code)     KAPI_COMPAT_ERRNO(ERROR_MODULE_KM, err_code)

static td_void inner_kapi_release_all_hardware_resource(td_void)
{
    td_s32 i;
    crypto_kapi_keyslot_ctx *keyslot_ctx = TD_NULL;
    /* mcipher keyslot. */
    for (i = 0; i < CRYPTO_MCIPHER_KEYSLOT_NUM; i++) {
        keyslot_ctx = &g_keyslot_symc_ctx_list[i];
        if (keyslot_ctx->is_open == TD_TRUE) {
            (td_void)drv_keyslot_destroy(keyslot_ctx->keyslot_handle);
        }
    }
    /* hmac keyslot. */
    for (i = 0; i < CRYPTO_HMAC_KEYSLOT_NUM; i++) {
        keyslot_ctx = &g_keyslot_hmac_ctx_list[i];
        if (keyslot_ctx->is_open == TD_TRUE) {
            (td_void)drv_keyslot_destroy(keyslot_ctx->keyslot_handle);
        }
    }
}

td_s32 kapi_km_env_init(td_void)
{
    td_s32 ret;
    crypto_kapi_func_enter();

    ret = crypto_mutex_init(&g_klad_mutex);
    crypto_chk_return(ret != TD_SUCCESS, ret, "crypto_mutex_init failed, ret is 0x%x\n", ret);

    ret = crypto_mutex_init(&g_keyslot_mutex);
    crypto_chk_goto(ret != TD_SUCCESS, klad_mutex_destroy_exit, "crypto_mutex_init failed, ret is 0x%x\n", ret);

    ret = drv_keyslot_init();
    crypto_chk_goto(ret != TD_SUCCESS, keyslot_mutex_destroy_exit, "drv_keyslot_init failed, ret is 0x%x\n", ret);

    crypto_kapi_func_exit();
    return TD_SUCCESS;

keyslot_mutex_destroy_exit:
    crypto_mutex_destroy(&g_keyslot_mutex);
klad_mutex_destroy_exit:
    crypto_mutex_destroy(&g_klad_mutex);
    return TD_FAILURE;
}

td_s32 kapi_km_env_deinit(td_void)
{
    unsigned int i;
    crypto_kapi_func_enter();

    crypto_mutex_destroy(&g_keyslot_mutex);
    crypto_mutex_destroy(&g_klad_mutex);

    inner_kapi_release_all_hardware_resource();
    (td_void)memset_s(g_keyslot_symc_ctx_list, sizeof(g_keyslot_symc_ctx_list), 0, sizeof(g_keyslot_symc_ctx_list));
    (td_void)memset_s(g_keyslot_hmac_ctx_list, sizeof(g_keyslot_hmac_ctx_list), 0, sizeof(g_keyslot_hmac_ctx_list));

    (td_void)drv_keyslot_deinit();

    for (i = 0; i < crypto_array_size(g_klad_ctx_list); i++) {
        if (g_klad_ctx_list[i] != NULL) {
            crypto_free(g_klad_ctx_list[i]);
            g_klad_ctx_list[i] = NULL;
        }
    }

    crypto_kapi_func_exit();
    return TD_SUCCESS;
}

td_s32 kapi_km_deinit(td_void)
{
    td_s32 i;
    crypto_owner owner = {0};
    crypto_kapi_keyslot_ctx *keyslot_ctx = TD_NULL;
    crypto_kapi_klad_ctx *klad_ctx = TD_NULL;
    crypto_kapi_func_exit();

    crypto_get_owner(&owner);
    kapi_keyslot_mutex_lock();
    /* mcipher keyslot. */
    for (i = 0; i < CRYPTO_MCIPHER_KEYSLOT_NUM; i++) {
        keyslot_ctx = &g_keyslot_symc_ctx_list[i];
        if (memcmp(&owner, &keyslot_ctx->owner, sizeof(crypto_owner)) == 0 && keyslot_ctx->is_open == TD_TRUE) {
            (td_void)drv_keyslot_destroy(keyslot_ctx->keyslot_handle);
            (td_void)memset_s(keyslot_ctx, sizeof(crypto_kapi_keyslot_ctx), 0, sizeof(crypto_kapi_keyslot_ctx));
        }
    }
    /* hmac keyslot. */
    for (i = 0; i < CRYPTO_HMAC_KEYSLOT_NUM; i++) {
        keyslot_ctx = &g_keyslot_hmac_ctx_list[i];
        if (memcmp(&owner, &keyslot_ctx->owner, sizeof(crypto_owner)) == 0 && keyslot_ctx->is_open == TD_TRUE) {
            (td_void)drv_keyslot_destroy(keyslot_ctx->keyslot_handle);
            (td_void)memset_s(keyslot_ctx, sizeof(crypto_kapi_keyslot_ctx), 0, sizeof(crypto_kapi_keyslot_ctx));
        }
    }
    kapi_keyslot_mutex_unlock();

    /* klad. */
    kapi_klad_mutex_lock();
    for (i = 0; i < CRYPTO_KLAD_VIRT_NUM; i++) {
        klad_ctx = g_klad_ctx_list[i];
        if (klad_ctx == NULL) {
            continue;
        }
        if (memcmp(&owner, &klad_ctx->owner, sizeof(crypto_owner)) == 0) {
            crypto_free(g_klad_ctx_list[i]);
            g_klad_ctx_list[i] = NULL;
        }
    }
    kapi_klad_mutex_unlock();
    crypto_kapi_func_exit();
    return TD_SUCCESS;
}

static crypto_kapi_keyslot_ctx *inner_keyslot_get_ctx(td_handle keyslot_handle)
{
    td_u32 i = 0;
    crypto_owner owner = {0};
    crypto_kapi_keyslot_ctx *keyslot_ctx;
    crypto_get_owner(&owner);
    /* check mcipher keyslot. */
    for (i = 0; i < CRYPTO_MCIPHER_KEYSLOT_NUM; i++) {
        keyslot_ctx = &g_keyslot_symc_ctx_list[i];
        if (memcmp(&owner, &keyslot_ctx->owner, sizeof(crypto_owner)) == 0 && keyslot_ctx->is_open == TD_TRUE &&
            keyslot_handle == keyslot_ctx->keyslot_handle) {
            return keyslot_ctx;
        }
    }
    /* check hmac keyslot. */
    for (i = 0; i < CRYPTO_HMAC_KEYSLOT_NUM; i++) {
        keyslot_ctx = &g_keyslot_hmac_ctx_list[i];
        if (memcmp(&owner, &keyslot_ctx->owner, sizeof(crypto_owner)) == 0 && keyslot_ctx->is_open == TD_TRUE &&
            keyslot_handle == keyslot_ctx->keyslot_handle) {
            return keyslot_ctx;
        }
    }
    crypto_log_err("invalid keyslot_handle\n");
    return TD_NULL;
}

static td_s32 inner_klad_handle_chk(td_handle klad_handle)
{
    td_u32 idx = 0;
    if (kapi_get_module_id(klad_handle) != KAPI_KLAD_MODULE_ID) {
        crypto_log_err("invalid module id\n");
        return KM_COMPAT_ERRNO(ERROR_INVALID_HANDLE);
    }
    idx = kapi_get_ctx_idx(klad_handle);
    crypto_chk_return(idx >= CRYPTO_KLAD_VIRT_NUM, KM_COMPAT_ERRNO(ERROR_INVALID_HANDLE),
        "idx overflow for klad\n");
    
    return TD_SUCCESS;
}

static crypto_kapi_klad_ctx *inner_kapi_get_klad_ctx(td_handle kapi_klad_handle)
{
    int ret;
    unsigned int idx;
    crypto_owner owner = {0};

    crypto_get_owner(&owner);

    ret = inner_klad_handle_chk(kapi_klad_handle);
    if (ret != CRYPTO_SUCCESS) {
        return NULL;
    }

    idx = kapi_get_ctx_idx(kapi_klad_handle);
    if (g_klad_ctx_list[idx] == NULL) {
        return NULL;
    }
    if (memcmp(&g_klad_ctx_list[idx]->owner, &owner, sizeof(owner)) != 0) {
        crypto_log_err("invalid owner\n");
        return NULL;
    }
    return g_klad_ctx_list[idx];
}

/* Keyslot. Long-term occupation by default */
td_s32 kapi_keyslot_create(td_handle *kapi_keyslot_handle, crypto_keyslot_type keyslot_type)
{
    td_u32 i;
    td_s32 ret = TD_SUCCESS;
    crypto_kapi_keyslot_ctx *ctx_list = TD_NULL;
    crypto_kapi_keyslot_ctx *current_ctx = TD_NULL;
    td_u32 ctx_num = 0;

    kapi_km_param_trace("keyslot_type is 0x%x\n", keyslot_type);

    km_null_ptr_chk(kapi_keyslot_handle);
    if (keyslot_type == CRYPTO_KEYSLOT_TYPE_MCIPHER) {
        ctx_list = g_keyslot_symc_ctx_list;
        ctx_num = crypto_array_size(g_keyslot_symc_ctx_list);
    } else if (keyslot_type == CRYPTO_KEYSLOT_TYPE_HMAC) {
        ctx_list = g_keyslot_hmac_ctx_list;
        ctx_num = crypto_array_size(g_keyslot_hmac_ctx_list);
    } else {
        crypto_log_err("invalid keyslot_type\n");
        return KM_COMPAT_ERRNO(ERROR_INVALID_PARAM);
    }

    kapi_keyslot_mutex_lock();
    for (i = 0; i < ctx_num; i++) {
        if (ctx_list[i].is_open == TD_FALSE) {
            current_ctx = &ctx_list[i];
            break;
        }
    }
    if (current_ctx == TD_NULL) {
        crypto_log_err("all keyslot contexts are busy\n");
        ret = KM_COMPAT_ERRNO(ERROR_CHN_BUSY);
        goto exit_unlock;
    }

    ret = drv_keyslot_create(&current_ctx->keyslot_handle, (crypto_keyslot_type)keyslot_type);
    crypto_chk_goto(ret != TD_SUCCESS, exit_unlock, "drv_keyslot_create failed, ret is 0x%x\n", ret);

    current_ctx->type = (crypto_keyslot_type)keyslot_type;
    current_ctx->is_open = TD_TRUE;
    crypto_get_owner(&current_ctx->owner);

    *kapi_keyslot_handle = current_ctx->keyslot_handle;

    kapi_km_param_trace("create kapi_keyslot_handle is 0x%x\n", *kapi_keyslot_handle);

exit_unlock:
    kapi_keyslot_mutex_unlock();
    return ret;
}
CRYPTO_EXPORT_SYMBOL(kapi_keyslot_create);

td_s32 kapi_keyslot_destroy(td_handle kapi_keyslot_handle)
{
    td_s32 ret = TD_SUCCESS;
    crypto_owner owner = {0};
    crypto_kapi_keyslot_ctx *current_ctx = TD_NULL;

    current_ctx = inner_keyslot_get_ctx(kapi_keyslot_handle);
    crypto_chk_return(current_ctx == TD_NULL, KM_COMPAT_ERRNO(ERROR_INVALID_HANDLE), "inner_keyslot_get_ctx failed\n");

    kapi_keyslot_mutex_lock();
    if (current_ctx->is_open == TD_FALSE) {
        ret = TD_SUCCESS;
        goto exit_unlock;
    }

    crypto_get_owner(&owner);
    if (memcmp(&owner, &current_ctx->owner, sizeof(crypto_owner)) != 0) {
        crypto_log_err("invalid process\n");
        ret = KM_COMPAT_ERRNO(ERROR_INVALID_PROCESS);
        goto exit_unlock;
    }

    ret = drv_keyslot_destroy(current_ctx->keyslot_handle);
    crypto_chk_goto(ret != TD_SUCCESS, exit_unlock, "drv_keyslot_destroy failed, ret is 0x%x\n", ret);

    (td_void)memset_s(current_ctx, sizeof(crypto_kapi_keyslot_ctx), 0, sizeof(crypto_kapi_keyslot_ctx));

exit_unlock:
    kapi_keyslot_mutex_unlock();

    return ret;
}
CRYPTO_EXPORT_SYMBOL(kapi_keyslot_destroy);

/* Klad. Short-term occupation by default */
td_s32 kapi_klad_create(td_handle *kapi_klad_handle)
{
    td_s32 i;
    td_s32 ret = TD_SUCCESS;
    crypto_kapi_klad_ctx *ctx = TD_NULL;
    crypto_kapi_func_enter();

    km_null_ptr_chk(kapi_klad_handle);

    kapi_klad_mutex_lock();
    for (i = 0; i < CRYPTO_KLAD_VIRT_NUM; i++) {
        if (g_klad_ctx_list[i] == NULL) {
            break;
        }
    }
    if (i >= CRYPTO_KLAD_VIRT_NUM) {
        ret = KM_COMPAT_ERRNO(ERROR_CHN_BUSY);
        goto exit_unlock;
    }
    ctx = crypto_malloc(sizeof(crypto_kapi_klad_ctx));
    if (ctx == TD_NULL) {
        ret = KM_COMPAT_ERRNO(ERROR_MALLOC);
        goto exit_unlock;
    }
    (void)memset_s(ctx, sizeof(crypto_kapi_klad_ctx), 0, sizeof(crypto_kapi_klad_ctx));

    crypto_get_owner(&ctx->owner);
    g_klad_ctx_list[i] = ctx;
    *kapi_klad_handle = synthesize_kapi_handle(KAPI_KLAD_MODULE_ID, i);

    kapi_km_param_trace("create kapi_klad_handle is 0x%x\n", *kapi_klad_handle);

exit_unlock:
    kapi_klad_mutex_unlock();
    crypto_kapi_func_exit();
    return ret;
}
CRYPTO_EXPORT_SYMBOL(kapi_klad_create);

td_s32 kapi_klad_destroy(td_handle kapi_klad_handle)
{
    td_s32 ret = TD_SUCCESS;
    td_u32 idx = 0;
    crypto_kapi_klad_ctx *ctx = TD_NULL;
    crypto_owner owner = {0};
    crypto_kapi_func_enter();

    kapi_km_param_trace("destroy kapi_klad_handle is 0x%x\n", kapi_klad_handle);

    ret = inner_klad_handle_chk(kapi_klad_handle);
    crypto_chk_return(ret != TD_SUCCESS, ret, "inner_klad_handle_chk failed\n");

    idx = kapi_get_ctx_idx(kapi_klad_handle);
    ctx = g_klad_ctx_list[idx];
    if (ctx == NULL) {
        return KM_COMPAT_ERRNO(ERROR_CTX_CLOSED);
    }
    kapi_klad_mutex_lock();

    crypto_get_owner(&owner);
    if (memcmp(&owner, &ctx->owner, sizeof(crypto_owner)) != 0) {
        crypto_log_err("invalid process\n");
        ret = KM_COMPAT_ERRNO(ERROR_INVALID_PROCESS);
        goto exit_unlock;
    }
    (td_void)memset_s(ctx, sizeof(crypto_kapi_klad_ctx), 0, sizeof(crypto_kapi_klad_ctx));
    crypto_free(ctx);
    g_klad_ctx_list[idx] = NULL;

exit_unlock:
    kapi_klad_mutex_unlock();
    crypto_kapi_func_exit();
    return ret;
}
CRYPTO_EXPORT_SYMBOL(kapi_klad_destroy);

td_s32 kapi_klad_attach(td_handle kapi_klad_handle, crypto_klad_dest klad_type,
    td_handle kapi_keyslot_handle)
{
    td_s32 ret = TD_SUCCESS;
    crypto_kapi_klad_ctx *klad_ctx = TD_NULL;
    crypto_kapi_keyslot_ctx *keyslot_ctx = TD_NULL;
    crypto_kapi_func_enter();

    crypto_chk_return (klad_type >= CRYPTO_KLAD_DEST_MAX, KM_COMPAT_ERRNO(ERROR_INVALID_PARAM),
        "Invalid klad_dest_type\n");

    klad_ctx = inner_kapi_get_klad_ctx(kapi_klad_handle);
    if (klad_ctx == NULL) {
        return KM_COMPAT_ERRNO(ERROR_CTX_CLOSED);
    }

    kapi_klad_mutex_lock();
    if (klad_type != CRYPTO_KLAD_DEST_NPU) {
        keyslot_ctx = inner_keyslot_get_ctx(kapi_keyslot_handle);
        crypto_chk_goto_with_ret(ret, keyslot_ctx == TD_NULL, exit_unlock, KM_COMPAT_ERRNO(ERROR_INVALID_HANDLE),
            "inner_keyslot_get_ctx failed\n");
        if (memcmp(&klad_ctx->owner, &keyslot_ctx->owner, sizeof(crypto_owner)) != 0) {
            crypto_log_err("invalid owner\n");
            ret = KM_COMPAT_ERRNO(ERROR_INVALID_PROCESS);
            goto exit_unlock;
        }
        klad_ctx->keyslot_handle = keyslot_ctx->keyslot_handle;
    } else {
        klad_ctx->keyslot_handle = kapi_keyslot_handle;
    }

    klad_ctx->is_attached = TD_TRUE;
    klad_ctx->klad_type = (crypto_klad_dest)klad_type;

exit_unlock:
    kapi_klad_mutex_unlock();
    crypto_kapi_func_exit();
    return ret;
}
CRYPTO_EXPORT_SYMBOL(kapi_klad_attach);

td_s32 kapi_klad_detach(td_handle kapi_klad_handle, crypto_klad_dest klad_type,
    td_handle kapi_keyslot_handle)
{
    td_s32 ret = TD_SUCCESS;
    crypto_kapi_klad_ctx *klad_ctx = TD_NULL;
    crypto_kapi_keyslot_ctx *keyslot_ctx = TD_NULL;

    crypto_chk_return (klad_type >= CRYPTO_KLAD_DEST_MAX, KM_COMPAT_ERRNO(ERROR_INVALID_PARAM),
        "Invalid klad_dest_type\n");

    klad_ctx = inner_kapi_get_klad_ctx(kapi_klad_handle);
    if (klad_ctx == NULL) {
        return KM_COMPAT_ERRNO(ERROR_CTX_CLOSED);
    }
    kapi_klad_mutex_lock();
    crypto_chk_goto_with_ret(ret, klad_ctx->is_attached == TD_FALSE, exit_unlock, KM_COMPAT_ERRNO(ERROR_NOT_ATTACHED),
        "call klad_attach first\n");

    crypto_chk_goto_with_ret(ret, klad_ctx->klad_type != (crypto_klad_dest)klad_type, exit_unlock,
        KM_COMPAT_ERRNO(ERROR_INVALID_PARAM), "invalid klad_type\n");
    if (klad_type != CRYPTO_KLAD_DEST_NPU) {
        keyslot_ctx = inner_keyslot_get_ctx(kapi_keyslot_handle);
        crypto_chk_goto_with_ret(ret, keyslot_ctx == TD_NULL, exit_unlock, KM_COMPAT_ERRNO(ERROR_INVALID_HANDLE),
            "inner_keyslot_get_ctx failed\n");
        crypto_chk_goto_with_ret(ret, keyslot_ctx->keyslot_handle != klad_ctx->keyslot_handle, exit_unlock,
            KM_COMPAT_ERRNO(ERROR_INVALID_HANDLE), "invalid keyslot_handle\n");
    } else {
        crypto_chk_goto_with_ret(ret, klad_ctx->keyslot_handle != kapi_keyslot_handle, exit_unlock,
            KM_COMPAT_ERRNO(ERROR_INVALID_HANDLE), "invalid npu keyslot_handle\n");
    }

    klad_ctx->is_attached = TD_FALSE;
    klad_ctx->keyslot_handle = 0;
    klad_ctx->klad_type = 0;

exit_unlock:
    kapi_klad_mutex_unlock();
    return ret;
}
CRYPTO_EXPORT_SYMBOL(kapi_klad_detach);

td_s32 kapi_klad_set_attr(td_handle kapi_klad_handle, const crypto_klad_attr *attr)
{
    td_s32 ret = TD_SUCCESS;
    crypto_kapi_klad_ctx *klad_ctx = TD_NULL;
    crypto_kapi_func_enter();

    km_null_ptr_chk(attr);

    klad_ctx = inner_kapi_get_klad_ctx(kapi_klad_handle);
    if (klad_ctx == NULL) {
        return KM_COMPAT_ERRNO(ERROR_CTX_CLOSED);
    }
    kapi_klad_mutex_lock();
    (void)memcpy_s(&klad_ctx->klad_attr, sizeof(crypto_klad_attr), attr, sizeof(crypto_klad_attr));

    klad_ctx->is_set_attr = TD_TRUE;
    kapi_klad_mutex_unlock();
    crypto_kapi_func_exit();
    return ret;
}
CRYPTO_EXPORT_SYMBOL(kapi_klad_set_attr);

td_s32 kapi_klad_get_attr(td_handle kapi_klad_handle, crypto_klad_attr *attr)
{
    td_s32 ret = TD_SUCCESS;
    crypto_kapi_klad_ctx *klad_ctx = TD_NULL;
    crypto_kapi_func_enter();

    km_null_ptr_chk(attr);
    klad_ctx = inner_kapi_get_klad_ctx(kapi_klad_handle);
    if (klad_ctx == NULL) {
        return KM_COMPAT_ERRNO(ERROR_CTX_CLOSED);
    }

    kapi_klad_mutex_lock();
    crypto_chk_goto_with_ret(ret, klad_ctx->is_set_attr == TD_FALSE, exit_unlock,
        KM_COMPAT_ERRNO(ERROR_NOT_SET_CONFIG), "call klad_set_attr first\n");

    (void)memcpy_s(attr, sizeof(crypto_klad_attr), &klad_ctx->klad_attr, sizeof(crypto_klad_attr));

exit_unlock:
    kapi_klad_mutex_unlock();
    crypto_kapi_func_exit();
    return ret;
}
CRYPTO_EXPORT_SYMBOL(kapi_klad_get_attr);

td_s32 kapi_klad_set_clear_key(td_handle kapi_klad_handle, const crypto_klad_clear_key *key)
{
#if defined(CONFIG_KM_CLEAR_KEY_SUPPORT)
    td_s32 ret = TD_SUCCESS;
    td_handle hard_klad_handle;
    crypto_kapi_klad_ctx *klad_ctx = TD_NULL;
    crypto_klad_clear_key clear_key;
    crypto_kapi_func_enter();

    km_null_ptr_chk(key);
    klad_ctx = inner_kapi_get_klad_ctx(kapi_klad_handle);
    if (klad_ctx == NULL) {
        return KM_COMPAT_ERRNO(ERROR_CTX_CLOSED);
    }

    clear_key.key = key->key;
    clear_key.key_parity = key->key_parity;
    clear_key.key_length = key->key_length;
    clear_key.hmac_type = key->hmac_type;

    kapi_klad_mutex_lock();
    crypto_chk_goto_with_ret(ret, klad_ctx->is_attached == TD_FALSE, exit_unlock,
        KM_COMPAT_ERRNO(ERROR_NOT_ATTACHED), "call klad_attach first\n");
    crypto_chk_goto_with_ret(ret, klad_ctx->is_set_attr == TD_FALSE, exit_unlock,
        KM_COMPAT_ERRNO(ERROR_NOT_SET_CONFIG), "call klad_set_attr first\n");

    ret = drv_klad_create(&hard_klad_handle);
    crypto_chk_goto(ret != TD_SUCCESS, exit_unlock, "drv_klad_create failed, ret is 0x%x\n", ret);

    ret = drv_klad_set_attr(hard_klad_handle, &klad_ctx->klad_attr);
    crypto_chk_goto(ret != TD_SUCCESS, exit_destroy, "drv_klad_set_attr failed, ret is 0x%x\n", ret);

    ret = drv_klad_attach(hard_klad_handle, klad_ctx->klad_type, klad_ctx->keyslot_handle);
    crypto_chk_goto(ret != TD_SUCCESS, exit_destroy, "drv_klad_attach failed, ret is 0x%x\n", ret);

    ret = drv_klad_set_clear_key(hard_klad_handle, &clear_key);
    crypto_chk_goto(ret != TD_SUCCESS, exit_detach, "drv_klad_set_clear_key failed, ret is 0x%x\n", ret);

exit_detach:
    drv_klad_detach(hard_klad_handle, klad_ctx->klad_type, klad_ctx->keyslot_handle);
exit_destroy:
    drv_klad_destroy(hard_klad_handle);
exit_unlock:
    kapi_klad_mutex_unlock();
    crypto_kapi_func_exit();
    return ret;
#else
    crypto_unused(kapi_klad_handle);
    crypto_unused(key);
    return ERROR_UNSUPPORT;
#endif
}
CRYPTO_EXPORT_SYMBOL(kapi_klad_set_clear_key);

#if defined(CONFIG_KM_SESSION_KEY_SUPPORT)
td_s32 kapi_klad_set_session_key(td_handle kapi_klad_handle, const crypto_klad_session_key *key)
{
    td_s32 ret = TD_SUCCESS;
    crypto_kapi_klad_ctx *klad_ctx = TD_NULL;
    crypto_kapi_func_enter();

    km_null_ptr_chk(key);
    km_null_ptr_chk(key->key);
    crypto_chk_return(key->key_length != CRYPTO_128_KEY_LEN, KM_COMPAT_ERRNO(ERROR_INVALID_PARAM),
        "Session key's size is invalid\n");
    crypto_chk_return(key->alg >= CRYPTO_KLAD_ALG_SEL_MAX, KM_COMPAT_ERRNO(ERROR_INVALID_PARAM),
        "session_key.alg >= CRYPTO_KLAD_ALG_SEL_MAX\n");
    crypto_chk_return(key->level >= CRYPTO_KLAD_LEVEL_SEL_SECOND, KM_COMPAT_ERRNO(ERROR_INVALID_PARAM),
        "session_key.level >= CRYPTO_KLAD_LEVEL_SEL_SECOND\n");

    klad_ctx = inner_kapi_get_klad_ctx(kapi_klad_handle);
    if (klad_ctx == NULL) {
        return KM_COMPAT_ERRNO(ERROR_CTX_CLOSED);
    }
    kapi_klad_mutex_lock();
    crypto_chk_goto_with_ret(ret, klad_ctx->is_attached == TD_FALSE, exit_unlock,
        KM_COMPAT_ERRNO(ERROR_NOT_ATTACHED), "call klad_attach first\n");
    crypto_chk_goto_with_ret(ret, klad_ctx->is_set_attr == TD_FALSE, exit_unlock,
        KM_COMPAT_ERRNO(ERROR_NOT_SET_CONFIG), "call klad_set_attr first\n");

    ret = memcpy_s(klad_ctx->session_key.key, sizeof(klad_ctx->session_key.key), key->key, key->key_length);
    crypto_chk_goto_with_ret(ret, ret != EOK, exit_unlock, KM_COMPAT_ERRNO(ERROR_MEMCPY_S), "memcpy_s failed\n");

    klad_ctx->is_set_session_key = TD_TRUE;
    klad_ctx->session_key.level = (crypto_klad_level_sel)key->level;
    klad_ctx->session_key.alg = (crypto_klad_alg_sel)key->alg;
    klad_ctx->session_key.key_length = key->key_length;

exit_unlock:
    kapi_klad_mutex_unlock();
    crypto_kapi_func_exit();
    return ret;
}
CRYPTO_EXPORT_SYMBOL(kapi_klad_set_session_key);
#endif

#if defined(CONFIG_KM_CONTENT_KEY_SUPPORT)
td_s32 kapi_klad_set_content_key(td_handle kapi_klad_handle, const crypto_klad_content_key *key)
{
    td_s32 ret = TD_SUCCESS;
    crypto_kapi_klad_ctx *klad_ctx = TD_NULL;
    td_handle hard_klad_handle = 0;
    crypto_klad_content_key content_key = {0};
    crypto_kapi_func_enter();

    km_null_ptr_chk(key);
    km_null_ptr_chk(key->key);

    klad_ctx = inner_kapi_get_klad_ctx(kapi_klad_handle);
    if (klad_ctx == NULL) {
        return KM_COMPAT_ERRNO(ERROR_CTX_CLOSED);
    }
    crypto_chk_return(key->key_length != CRYPTO_128_KEY_LEN && key->key_length != CRYPTO_256_KEY_LEN,
        KM_COMPAT_ERRNO(ERROR_INVALID_PARAM), "Content key's size is invalid\n");
    crypto_chk_return(key->alg >= CRYPTO_KLAD_ALG_SEL_MAX, KM_COMPAT_ERRNO(ERROR_INVALID_PARAM),
        "content_key.alg >= CRYPTO_KLAD_ALG_SEL_MAX\n");
    crypto_chk_return(key->key_parity >= (td_bool)KM_KLAD_KEY_PARITY_MAX, KM_COMPAT_ERRNO(ERROR_INVALID_PARAM),
        "content_key.key_parity >= KM_KLAD_KEY_PARITY_MAX\n");

    kapi_klad_mutex_lock();
    crypto_chk_goto_with_ret(ret, klad_ctx->is_attached == TD_FALSE, exit_unlock,
        KM_COMPAT_ERRNO(ERROR_NOT_ATTACHED), "call klad_attach first\n");
    crypto_chk_goto_with_ret(ret, klad_ctx->is_set_attr == TD_FALSE, exit_unlock,
        KM_COMPAT_ERRNO(ERROR_NOT_SET_CONFIG), "call klad_set_attr first\n");
    crypto_chk_goto_with_ret(ret, klad_ctx->is_set_session_key == TD_FALSE, exit_unlock,
        KM_COMPAT_ERRNO(ERROR_NOT_SET_SESSION_KEY), "call klad_set_session_key first\n");

    content_key.key_length = key->key_length;
    content_key.alg = (crypto_klad_alg_sel)key->alg;
    content_key.key_parity = (td_bool)key->key_parity;
    ret = memcpy_s(content_key.key, sizeof(content_key.key), key->key, key->key_length);
    crypto_chk_goto_with_ret(ret, ret != EOK, exit_unlock, KM_COMPAT_ERRNO(ERROR_MEMCPY_S), "memcpy_s failed\n");

    ret = drv_klad_create(&hard_klad_handle);
    crypto_chk_goto(ret != TD_SUCCESS, exit_unlock, "drv_klad_create failed, ret is 0x%x\n", ret);

    ret = drv_klad_set_attr(hard_klad_handle, (crypto_klad_attr *)&klad_ctx->klad_attr);
    crypto_chk_goto(ret != TD_SUCCESS, destroy_exit, "drv_klad_set_attr failed, ret is 0x%x\n", ret);

    ret = drv_klad_attach(hard_klad_handle, klad_ctx->klad_type, klad_ctx->keyslot_handle);
    crypto_chk_goto(ret != TD_SUCCESS, destroy_exit, "drv_klad_attach failed, ret is 0x%x\n", ret);

    ret = drv_klad_set_session_key(hard_klad_handle, &klad_ctx->session_key);
    crypto_chk_goto(ret != TD_SUCCESS, detach_exit, "drv_klad_set_session_key failed, ret is 0x%x\n", ret);

    ret = drv_klad_set_content_key(hard_klad_handle, &content_key);
    crypto_chk_goto(ret != TD_SUCCESS, detach_exit, "drv_klad_set_content_key failed, ret is 0x%x\n", ret);
detach_exit:
    drv_klad_detach(hard_klad_handle, klad_ctx->klad_type, klad_ctx->keyslot_handle);
destroy_exit:
    drv_klad_destroy(hard_klad_handle);
exit_unlock:
    kapi_klad_mutex_unlock();
    crypto_kapi_func_exit();
    return ret;
}
CRYPTO_EXPORT_SYMBOL(kapi_klad_set_content_key);
#endif

#define CRYPTO_EFFECTIVE_KEY_SALT_LENGTH_MAX 28
td_s32 kapi_klad_set_effective_key(td_handle kapi_klad_handle, const crypto_klad_effective_key *key)
{
#if defined(CONFIG_KM_EFFECTIVE_KEY_SUPPORT)
    td_s32 ret = TD_SUCCESS;
    crypto_kapi_klad_ctx *klad_ctx = TD_NULL;
    td_handle hard_klad_handle = 0;
    crypto_klad_effective_key effective_key = {0};
    crypto_kapi_func_enter();

    km_null_ptr_chk(key);
    km_null_ptr_chk(key->salt);
    crypto_chk_return(key->salt_length > CRYPTO_EFFECTIVE_KEY_SALT_LENGTH_MAX,
        KM_COMPAT_ERRNO(ERROR_INVALID_PARAM), "effective_key.kdf_hard_alg >= CRYPTO_KDF_HARD_ALG_MAX\n");
    crypto_chk_return(key->kdf_hard_alg >= CRYPTO_KDF_HARD_ALG_MAX, KM_COMPAT_ERRNO(ERROR_INVALID_PARAM),
        "effective_key.kdf_hard_alg >= CRYPTO_KDF_HARD_ALG_MAX\n");
    crypto_chk_return(key->key_size < CRYPTO_KLAD_KEY_SIZE_128BIT ||
        key->key_size > CRYPTO_KLAD_KEY_SIZE_256BIT,
        KM_COMPAT_ERRNO(ERROR_INVALID_PARAM), "effective key's size is invalid\n");

    klad_ctx = inner_kapi_get_klad_ctx(kapi_klad_handle);
    if (klad_ctx == NULL) {
        return KM_COMPAT_ERRNO(ERROR_CTX_CLOSED);
    }

    effective_key.kdf_hard_alg = key->kdf_hard_alg;
    effective_key.key_parity = key->key_parity;
    effective_key.key_size = key->key_size;
    effective_key.oneway = key->oneway;
    effective_key.salt_length = key->salt_length;
    effective_key.salt = key->salt;

    kapi_klad_mutex_lock();
    crypto_chk_goto_with_ret(ret, klad_ctx->is_attached == TD_FALSE, exit_unlock,
        KM_COMPAT_ERRNO(ERROR_NOT_ATTACHED), "call klad_attach first\n");
    crypto_chk_goto_with_ret(ret, klad_ctx->is_set_attr == TD_FALSE, exit_unlock,
        KM_COMPAT_ERRNO(ERROR_NOT_SET_CONFIG), "call klad_set_attr first\n");
    crypto_chk_goto_with_ret(ret, klad_ctx->klad_attr.klad_cfg.rootkey_type == CRYPTO_KDF_HARD_KEY_TYPE_ODRK1 &&
        effective_key.key_size != CRYPTO_KLAD_KEY_SIZE_128BIT, exit_unlock,
        KM_COMPAT_ERRNO(ERROR_INVALID_PARAM), "ODRK1 only support 128 key_size\n");

    ret = drv_klad_create(&hard_klad_handle);
    crypto_chk_goto(ret != TD_SUCCESS, exit_unlock, "drv_klad_create failed, ret is 0x%x\n", ret);

    ret = drv_klad_set_attr(hard_klad_handle, (crypto_klad_attr *)&klad_ctx->klad_attr);
    crypto_chk_goto(ret != TD_SUCCESS, destroy_exit, "drv_klad_set_attr failed, ret is 0x%x\n", ret);

    ret = drv_klad_attach(hard_klad_handle, klad_ctx->klad_type, klad_ctx->keyslot_handle);
    crypto_chk_goto(ret != TD_SUCCESS, destroy_exit, "drv_klad_attach failed, ret is 0x%x\n", ret);

    ret = drv_klad_set_effective_key(hard_klad_handle, &effective_key);
    crypto_chk_goto(ret != TD_SUCCESS, detach_exit, "drv_klad_set_effective_key failed, ret is 0x%x\n", ret);
detach_exit:
    drv_klad_detach(hard_klad_handle, klad_ctx->klad_type, klad_ctx->keyslot_handle);
destroy_exit:
    drv_klad_destroy(hard_klad_handle);
exit_unlock:
    kapi_klad_mutex_unlock();
    crypto_kapi_func_exit();
    return ret;
#else
    crypto_unused(kapi_klad_handle);
    crypto_unused(key);

    return ERROR_UNSUPPORT;
#endif
}
CRYPTO_EXPORT_SYMBOL(kapi_klad_set_effective_key);

#if defined(CONFIG_KM_KDF_UPDATE_SUPPORT)
td_s32 kapi_kdf_update(crypto_kdf_otp_key otp_key, crypto_kdf_update_alg alg)
{
    crypto_unused(otp_key);
    crypto_unused(alg);

    return ERROR_UNSUPPORT;
}
#endif