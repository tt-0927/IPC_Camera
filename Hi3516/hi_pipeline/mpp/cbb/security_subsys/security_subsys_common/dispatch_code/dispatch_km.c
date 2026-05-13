/*
  Copyright (c), 2001-2025, Shenshu Tech. Co., Ltd.
 */

#include "dispatch_km.h"
#include "crypto_common_macro.h"
#include "kapi_km.h"
#if defined(CONFIG_SYMC_HARDEN_INTERFACE_SUPPORT)
#include "kapi_km_impl.h"
#endif

#define KM_COMPAT_ERRNO(err_code)     DISPATCH_COMPAT_ERRNO(ERROR_MODULE_KM, err_code)

static td_s32 dispatch_keyslot_create(unsigned int cmd, td_void *argp, void *private_data)
{
    td_s32 ret = TD_SUCCESS;
    keyslot_create_ctl_t *keyslot_create_t = TD_NULL;
    td_handle keyslot_handle;
    crypto_dispatch_func_enter();

    crypto_unused(cmd);
    crypto_unused(private_data);

    keyslot_create_t = (keyslot_create_ctl_t *)argp;
    ret = kapi_keyslot_create(&keyslot_handle, keyslot_create_t->keyslot_type);
    crypto_chk_return(ret != TD_SUCCESS, ret, "kapi_keyslot_create failed, ret is 0x%x\n", ret);
    keyslot_create_t->kapi_keyslot_handle = keyslot_handle;
    crypto_dispatch_func_exit();
    return ret;
}

static td_s32 dispatch_keyslot_destroy(unsigned int cmd, td_void *argp, void *private_data)
{
    td_s32 ret = TD_SUCCESS;
    keyslot_destroy_ctl_t *keyslot_destroy_t = TD_NULL;
    crypto_dispatch_func_enter();

    crypto_unused(cmd);
    crypto_unused(private_data);

    keyslot_destroy_t = (keyslot_destroy_ctl_t *)argp;
    ret = kapi_keyslot_destroy(keyslot_destroy_t->kapi_keyslot_handle);
    crypto_chk_return(ret != TD_SUCCESS, ret, "kapi_keyslot_destroy failed, ret is 0x%x\n", ret);

    crypto_dispatch_func_exit();
    return ret;
}

static td_s32 dispatch_klad_create(unsigned int cmd, td_void *argp, void *private_data)
{
    td_s32 ret = TD_SUCCESS;
    klad_handle_ctl_t *klad_handle_ctl = TD_NULL;
    td_handle klad_handle;
    crypto_dispatch_func_enter();

    crypto_unused(cmd);
    crypto_unused(private_data);

    klad_handle_ctl = (klad_handle_ctl_t *)argp;
    ret = kapi_klad_create(&klad_handle);
    crypto_chk_return(ret != TD_SUCCESS, ret, "kapi_keyslot_create failed, ret is 0x%x\n", ret);
    klad_handle_ctl->kapi_klad_handle = (td_handle)klad_handle;
    crypto_dispatch_func_exit();
    return ret;
}

static td_s32 dispatch_klad_destroy(unsigned int cmd, td_void *argp, void *private_data)
{
    td_s32 ret = TD_SUCCESS;
    klad_handle_ctl_t *klad_handle_ctl = TD_NULL;
    crypto_dispatch_func_enter();

    crypto_unused(cmd);
    crypto_unused(private_data);

    klad_handle_ctl = (klad_handle_ctl_t *)argp;
    ret = kapi_klad_destroy(klad_handle_ctl->kapi_klad_handle);
    crypto_chk_return(ret != TD_SUCCESS, ret, "kapi_klad_destroy failed, ret is 0x%x\n", ret);

    crypto_dispatch_func_exit();
    return ret;
}

static td_s32 dispatch_klad_set_attr(unsigned int cmd, td_void *argp, void *private_data)
{
    td_s32 ret = TD_SUCCESS;
    klad_attr_ctl_t *klad_attr_ctl = TD_NULL;
    crypto_dispatch_func_enter();

    crypto_unused(cmd);
    crypto_unused(private_data);

    klad_attr_ctl = (klad_attr_ctl_t *)argp;
    ret = kapi_klad_set_attr(klad_attr_ctl->kapi_klad_handle, &klad_attr_ctl->attr);
    crypto_chk_return(ret != TD_SUCCESS, ret, "kapi_klad_set_attr failed, ret is 0x%x\n", ret);

    crypto_dispatch_func_exit();
    return ret;
}

static td_s32 dispatch_klad_get_attr(unsigned int cmd, td_void *argp, void *private_data)
{
    td_s32 ret = TD_SUCCESS;
    klad_attr_ctl_t *klad_attr_ctl = TD_NULL;
    crypto_dispatch_func_enter();

    crypto_unused(cmd);
    crypto_unused(private_data);

    klad_attr_ctl = (klad_attr_ctl_t *)argp;

    ret = kapi_klad_get_attr(klad_attr_ctl->kapi_klad_handle, &klad_attr_ctl->attr);
    crypto_chk_return(ret != TD_SUCCESS, ret, "kapi_klad_get_attr failed, ret is 0x%x\n", ret);

    crypto_dispatch_func_exit();
    return ret;
}

static td_s32 dispatch_klad_attach(unsigned int cmd, td_void *argp, void *private_data)
{
    td_s32 ret = TD_SUCCESS;
    klad_attach_ctl_t *klad_attach_ctl = TD_NULL;
    crypto_dispatch_func_enter();

    crypto_unused(cmd);
    crypto_unused(private_data);

    klad_attach_ctl = (klad_attach_ctl_t *)argp;
    ret = kapi_klad_attach(klad_attach_ctl->kapi_klad_handle, klad_attach_ctl->klad_type,
        klad_attach_ctl->kapi_keyslot_handle);
    crypto_chk_return(ret != TD_SUCCESS, ret, "kapi_klad_attach failed, ret is 0x%x\n", ret);

    crypto_dispatch_func_exit();
    return ret;
}

static td_s32 dispatch_klad_detach(unsigned int cmd, td_void *argp, void *private_data)
{
    td_s32 ret = TD_SUCCESS;
    klad_attach_ctl_t *klad_attach_ctl = TD_NULL;
    crypto_dispatch_func_enter();

    crypto_unused(cmd);
    crypto_unused(private_data);

    klad_attach_ctl = (klad_attach_ctl_t *)argp;
    ret = kapi_klad_detach(klad_attach_ctl->kapi_klad_handle, klad_attach_ctl->klad_type,
        klad_attach_ctl->kapi_keyslot_handle);
    crypto_chk_return(ret != TD_SUCCESS, ret, "kapi_klad_detach failed, ret is 0x%x\n", ret);

    crypto_dispatch_func_exit();
    return ret;
}

#define MAX_KEY_LEN     128
#if defined(CONFIG_KM_CLEAR_KEY_SUPPORT)
static td_s32 dispatch_klad_set_clear_key(unsigned int cmd, td_void *argp, void *private_data)
{
    td_s32 ret = TD_SUCCESS;
    klad_set_clear_key_ctl_t *klad_set_clear_key_ctl = TD_NULL;
    td_u8 key[MAX_KEY_LEN] = {0};
    crypto_klad_clear_key clear_key = {0};
    crypto_dispatch_func_enter();

    crypto_unused(cmd);
    crypto_unused(private_data);

    klad_set_clear_key_ctl = (klad_set_clear_key_ctl_t *)argp;

    crypto_chk_return(klad_set_clear_key_ctl->key_size > MAX_KEY_LEN, KM_COMPAT_ERRNO(ERROR_INVALID_PARAM),
        "Unsupported key_len, the max lenth of the key is: %d\n", MAX_KEY_LEN);
    crypto_param_require(klad_set_clear_key_ctl->key.p != TD_NULL);

    ret = crypto_copy_from_user(key, MAX_KEY_LEN, klad_set_clear_key_ctl->key.p,  \
        klad_set_clear_key_ctl->key_size);
    crypto_chk_return(ret != TD_SUCCESS, ret, "crypto_copy_from_user failed, ret is 0x%x\n", ret);

    clear_key.key = key;
    clear_key.hmac_type = klad_set_clear_key_ctl->hmac_type;
    clear_key.key_length = klad_set_clear_key_ctl->key_size;
    clear_key.key_parity = klad_set_clear_key_ctl->key_parity;

    ret = kapi_klad_set_clear_key(klad_set_clear_key_ctl->kapi_klad_handle, &clear_key);
    crypto_chk_goto(ret != TD_SUCCESS, exit, "kapi_klad_set_clear_key failed, ret is 0x%x\n", ret);

exit:
    (td_void)memset_s(key, MAX_KEY_LEN, 0, MAX_KEY_LEN);
    crypto_dispatch_func_exit();
    return ret;
}
#endif

#if defined(CONFIG_KM_SESSION_KEY_SUPPORT)
static td_s32 dispatch_klad_set_session_key(unsigned int cmd, td_void *argp, void *private_data)
{
    td_s32 ret = TD_SUCCESS;
    klad_set_session_key_ctl_t *klad_set_session_key_ctl = TD_NULL;
    crypto_klad_session_key session_key = {0};
    crypto_dispatch_func_enter();

    crypto_unused(cmd);
    crypto_unused(private_data);

    klad_set_session_key_ctl = (klad_set_session_key_ctl_t *)argp;

    crypto_chk_return(klad_set_session_key_ctl->key_size > MAX_KEY_LEN, KM_COMPAT_ERRNO(ERROR_INVALID_PARAM),
        "Unsupported key_len, the max lenth of the key is: %d\n", MAX_KEY_LEN);
    crypto_param_require(klad_set_session_key_ctl->key.p != TD_NULL);

    ret = crypto_copy_from_user(session_key.key, sizeof(session_key.key), klad_set_session_key_ctl->key.p,  \
        klad_set_session_key_ctl->key_size);
    crypto_chk_return(ret != TD_SUCCESS, ret, "crypto_copy_from_user failed, ret is 0x%x\n", ret);
    session_key.alg = klad_set_session_key_ctl->alg;
    session_key.key_length = klad_set_session_key_ctl->key_size;
    session_key.level = klad_set_session_key_ctl->level;

    ret = kapi_klad_set_session_key(klad_set_session_key_ctl->kapi_klad_handle, &session_key);
    crypto_chk_return(ret != TD_SUCCESS, ret, "kapi_klad_set_session_key failed, ret is 0x%x\n", ret);

    crypto_dispatch_func_exit();
    return ret;
}
#endif

#if defined(CONFIG_KM_CONTENT_KEY_SUPPORT)
static td_s32 dispatch_klad_set_content_key(unsigned int cmd, td_void *argp, void *private_data)
{
    td_s32 ret = TD_SUCCESS;
    klad_set_content_key_ctl_t *klad_set_content_key_ctl = TD_NULL;
    crypto_klad_content_key content_key = {0};
    crypto_dispatch_func_enter();

    crypto_unused(cmd);
    crypto_unused(private_data);

    klad_set_content_key_ctl = (klad_set_content_key_ctl_t *)argp;

    crypto_chk_return(klad_set_content_key_ctl->key_size > MAX_KEY_LEN, KM_COMPAT_ERRNO(ERROR_INVALID_PARAM),
        "Unsupported key_len, the max lenth of the key is: %d\n", MAX_KEY_LEN);
    crypto_param_require(klad_set_content_key_ctl->key.p != TD_NULL);

    ret = crypto_copy_from_user(content_key.key, sizeof(content_key.key), klad_set_content_key_ctl->key.p,  \
        klad_set_content_key_ctl->key_size);
    crypto_chk_return(ret != TD_SUCCESS, ret, "crypto_copy_from_user failed, ret is 0x%x\n", ret);

    content_key.alg = klad_set_content_key_ctl->alg;
    content_key.key_length = klad_set_content_key_ctl->key_size;
    content_key.key_parity = klad_set_content_key_ctl->key_parity;

    ret = kapi_klad_set_content_key(klad_set_content_key_ctl->kapi_klad_handle, &content_key);
    crypto_chk_return(ret != TD_SUCCESS, ret, "kapi_klad_set_content_key failed, ret is 0x%x\n", ret);

    crypto_dispatch_func_exit();
    return ret;
}
#endif

#define EFFECTIVE_KEY_LENGTH 28
#if defined(CONFIG_KM_EFFECTIVE_KEY_SUPPORT)
static td_s32 dispatch_klad_set_effective_key(unsigned int cmd, td_void *argp, void *private_data)
{
    td_s32 ret = TD_SUCCESS;
    klad_set_effective_key_ctl_t *klad_set_effective_key_ctl = TD_NULL;
    td_u8 salt[EFFECTIVE_KEY_LENGTH] = {0};
    crypto_klad_effective_key effective_key = {0};
    crypto_dispatch_func_enter();

    crypto_unused(cmd);
    crypto_unused(private_data);

    klad_set_effective_key_ctl = (klad_set_effective_key_ctl_t *)argp;

    crypto_chk_return(klad_set_effective_key_ctl == NULL, KM_COMPAT_ERRNO(ERROR_INVALID_PARAM),
        "effective_key is NULL\n");

    effective_key.kdf_hard_alg = klad_set_effective_key_ctl->kdf_hard_alg;
    effective_key.key_parity = klad_set_effective_key_ctl->key_parity;
    effective_key.key_size = klad_set_effective_key_ctl->key_size;
    effective_key.oneway = klad_set_effective_key_ctl->oneway;
    effective_key.salt_length = klad_set_effective_key_ctl->salt_length;

    ret = crypto_copy_from_user(salt, EFFECTIVE_KEY_LENGTH, klad_set_effective_key_ctl->salt,
        klad_set_effective_key_ctl->salt_length);
    crypto_chk_return(ret != TD_SUCCESS, ret, "crypto_copy_from_user failed, ret is 0x%x\n", ret);

    effective_key.salt = salt;

    ret = kapi_klad_set_effective_key(klad_set_effective_key_ctl->kapi_klad_handle, &effective_key);
    crypto_chk_goto(ret != TD_SUCCESS, exit, "kapi_klad_set_effective_key failed, ret is 0x%x\n", ret);

exit:
    (td_void)memset_s(salt, EFFECTIVE_KEY_LENGTH, 0, EFFECTIVE_KEY_LENGTH);
    crypto_dispatch_func_exit();
    return ret;
}
#endif

#if defined(CONFIG_SYMC_HARDEN_INTERFACE_SUPPORT)
static td_s32 dispatch_km_create_key_impl(unsigned int cmd, td_void *argp, void *private_data)
{
    td_s32 ret = TD_SUCCESS;
    km_create_key_impl_ctl_t *km_key = TD_NULL;

    crypto_dispatch_func_enter();
    crypto_unused(cmd);
    crypto_unused(private_data);

    km_key = (km_create_key_impl_ctl_t *)argp;
    crypto_chk_return(km_key == NULL, KM_COMPAT_ERRNO(ERROR_INVALID_PARAM), "km_key is NULL\n");

    ret = kapi_km_create_key_impl(&km_key->kapi_kslot_handle);
    crypto_chk_return(ret != TD_SUCCESS, ret, "kapi_km_create_key_impl failed, ret is 0x%x\n", ret);

    crypto_dispatch_func_exit();
    return ret;
}

static td_s32 dispatch_km_set_key_impl(unsigned int cmd, td_void *argp, void *private_data)
{
    td_s32 ret = TD_SUCCESS;
    km_set_impl_ctl_t *km_key = TD_NULL;
    td_u8 key[MAX_KEY_LEN] = {0};

    crypto_dispatch_func_enter();
    crypto_unused(cmd);
    crypto_unused(private_data);

    km_key = (km_set_impl_ctl_t *)argp;
    crypto_chk_return(km_key->key_size > MAX_KEY_LEN, KM_COMPAT_ERRNO(ERROR_INVALID_PARAM),
        "Unsupported key_len, the max lenth of the key is: %d\n", MAX_KEY_LEN);
    crypto_param_require(km_key->key.p != TD_NULL);

    ret = crypto_copy_from_user(key, sizeof(key), km_key->key.p, km_key->key_size);
    crypto_chk_return(ret != TD_SUCCESS, ret, "crypto_copy_from_user failed, ret is 0x%x\n", ret);

    ret = kapi_km_set_key_impl(km_key->kapi_kslot_handle, key, km_key->key_size, km_key->tee_open);
    crypto_chk_goto(ret != TD_SUCCESS, clean_key, "kapi_km_set_key_impl failed, ret is 0x%x\n", ret);
clean_key:
    (td_void)memset_s(key, sizeof(key), 0, sizeof(key));
    crypto_dispatch_func_exit();
    return ret;
}

static td_s32 dispatch_km_destroy_key_impl(unsigned int cmd, td_void *argp, void *private_data)
{
    td_s32 ret = TD_SUCCESS;
    km_destroy_key_impl_ctl_t *km_handle = TD_NULL;

    crypto_dispatch_func_enter();
    crypto_unused(cmd);
    crypto_unused(private_data);

    km_handle = (km_destroy_key_impl_ctl_t *)argp;
    crypto_chk_return(km_handle == NULL, KM_COMPAT_ERRNO(ERROR_INVALID_PARAM), "km_handle is NULL\n");

    ret = kapi_km_desroy_key_impl(km_handle->kapi_keyslot_handle);
    crypto_chk_return(ret != TD_SUCCESS, ret, "kapi_km_desroy_key_impl failed, ret is 0x%x\n", ret);

    crypto_dispatch_func_exit();
    return ret;
}
#endif

static crypto_ioctl_cmd g_km_func_list[] = {
    {CMD_KEYSLOT_CREATE_HANDLE,             dispatch_keyslot_create},
    {CMD_KEYSLOT_DESTROY_HANDLE,            dispatch_keyslot_destroy},
    {CMD_KLAD_CREATE_HANDLE,                dispatch_klad_create},
    {CMD_KLAD_DESTROY_HANDLE,               dispatch_klad_destroy},
    {CMD_KLAD_ATTACH,                       dispatch_klad_attach},
    {CMD_KLAD_DETACH,                       dispatch_klad_detach},
    {CMD_KLAD_SET_ATTR,                     dispatch_klad_set_attr},
    {CMD_KLAD_GET_ATTR,                     dispatch_klad_get_attr},
#if defined(CONFIG_KM_CONTENT_KEY_SUPPORT)
    {CMD_KLAD_SET_SESSION_KEY,              dispatch_klad_set_session_key},
    {CMD_KLAD_SET_CONTENT_KEY,              dispatch_klad_set_content_key},
#else
    {CMD_KLAD_SET_SESSION_KEY,              NULL},
    {CMD_KLAD_SET_CONTENT_KEY,              NULL},
#endif
#if defined(CONFIG_KM_CLEAR_KEY_SUPPORT)
    {CMD_KLAD_SET_CLEAR_KEY,                dispatch_klad_set_clear_key},
#else
    {CMD_KLAD_SET_CLEAR_KEY,                NULL},
#endif
#if defined(CONFIG_KM_EFFECTIVE_KEY_SUPPORT)
    {CMD_KLAD_SET_EFFECTVE_KEY,             dispatch_klad_set_effective_key},
#else
    {CMD_KLAD_SET_EFFECTVE_KEY,             NULL},
#endif
#if defined(CONFIG_SYMC_HARDEN_INTERFACE_SUPPORT)
    {CMD_KM_CREATE_KEY_IMPL,                dispatch_km_create_key_impl},
    {CMD_KM_SET_KEY_IMPL,                   dispatch_km_set_key_impl},
    {CMD_KM_DESTROY_KEY_IMPL,               dispatch_km_destroy_key_impl},
#else
    {CMD_KM_CREATE_KEY_IMPL,                NULL},
    {CMD_KM_SET_KEY_IMPL,                   NULL},
    {CMD_KM_DESTROY_KEY_IMPL,               NULL},
#endif
};

crypto_ioctl_cmd *get_km_func_list(td_void)
{
    return g_km_func_list;
}

td_u32 get_km_func_num(td_void)
{
    return crypto_array_size(g_km_func_list);
}
