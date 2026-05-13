/*
  Copyright (c), 2001-2025, Shenshu Tech. Co., Ltd.
 */

#include "uapi_km.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <securec.h>

#include "ot_type.h"
#include "crypto_osal_user_lib.h"
#include "crypto_km_struct.h"
#include "crypto_errno.h"
#include "ioctl_km.h"
#include "crypto_common_macro.h"

#define REF_COUNT_MAX_NUM     0x7FFFFFFF
#define KM_COMPAT_ERRNO(err_code)     UAPI_COMPAT_ERRNO(ERROR_MODULE_KM, err_code)

typedef struct mpi_mgnt {
    crypto_mutex_t      lock;
    td_s32              ref_count;
    td_s32              dev_fd;
} mpi_mgnt_t;

static mpi_mgnt_t km_mgnt = {
    .lock = CRYPTO_PTHREAD_MUTEX_INITIALIZER,
    .ref_count = -1,
    .dev_fd = -1
};

static td_bool inner_km_is_init(td_void)
{
    if (km_mgnt.ref_count >= 0) {
        return TD_TRUE;
    }
    return TD_FALSE;
}

td_s32 uapi_km_init(td_void)
{
    td_s32 ret = TD_SUCCESS;
    crypto_uapi_func_enter();
    crypto_pthread_mutex_lock(&km_mgnt.lock);

    if (km_mgnt.ref_count >= REF_COUNT_MAX_NUM) {
        ret = ERROR_COUNT_OVERFLOW;
        goto exit;
    }

    if (km_mgnt.ref_count >= 0) {
        km_mgnt.ref_count++;
        goto exit;
    }
    km_mgnt.dev_fd = crypto_open("/dev/km", O_RDWR, 0);
    if (km_mgnt.dev_fd < 0) {
        printf("open /dev/km failed\n");
        ret = KM_COMPAT_ERRNO(ERROR_DEV_OPEN_FAILED);
        goto exit;
    }

    km_mgnt.ref_count = 0;

exit:
    crypto_pthread_mutex_unlock(&km_mgnt.lock);
    crypto_uapi_func_exit();
    return ret;
}

td_s32 uapi_km_deinit(td_void)
{
    td_s32 ret = TD_SUCCESS;
    crypto_uapi_func_enter();
    crypto_pthread_mutex_lock(&km_mgnt.lock);

    if (km_mgnt.ref_count > 0) {
        km_mgnt.ref_count--;
        goto exit;
    }

    if (km_mgnt.ref_count == 0) {
        km_mgnt.ref_count--;
        crypto_close(km_mgnt.dev_fd);
        km_mgnt.dev_fd = -1;
    }

exit:
    crypto_pthread_mutex_unlock(&km_mgnt.lock);
    crypto_uapi_func_exit();
    return ret;
}

/* Keyslot. */
td_s32 uapi_keyslot_create(td_handle *mpi_keyslot_handle, crypto_keyslot_type keyslot_type)
{
    td_s32 ret = TD_SUCCESS;
    keyslot_create_ctl_t keyslot_create_ctl = {0};

    crypto_uapi_func_enter();
    crypto_chk_return(inner_km_is_init() != TD_TRUE, KM_COMPAT_ERRNO(ERROR_NOT_INIT), "KM not init\n");
    km_null_ptr_chk(mpi_keyslot_handle);

    keyslot_create_ctl.keyslot_type = keyslot_type;
    ret = crypto_km_ioctl(km_mgnt.dev_fd, CMD_KEYSLOT_CREATE_HANDLE, &keyslot_create_ctl);
    crypto_chk_return(ret != TD_SUCCESS, ret, "crypto_km_ioctl failed, ret is 0x%x\n", ret);

    *mpi_keyslot_handle = keyslot_create_ctl.kapi_keyslot_handle;

    crypto_uapi_func_exit();
    return ret;
}

td_s32 uapi_keyslot_destroy(td_handle mpi_keyslot_handle)
{
    td_s32 ret = TD_SUCCESS;

    keyslot_destroy_ctl_t keyslot_destroy_ctl = {0};

    crypto_uapi_func_enter();
    crypto_chk_return(inner_km_is_init() != TD_TRUE, KM_COMPAT_ERRNO(ERROR_NOT_INIT), "KM not init\n");

    keyslot_destroy_ctl.kapi_keyslot_handle = mpi_keyslot_handle;
    ret = crypto_km_ioctl(km_mgnt.dev_fd, CMD_KEYSLOT_DESTROY_HANDLE, &keyslot_destroy_ctl);
    crypto_chk_return(ret != TD_SUCCESS, ret, "crypto_km_ioctl failed, ret is 0x%x\n", ret);

    crypto_uapi_func_exit();
    return ret;
}

/* Klad. */
td_s32 uapi_klad_create(td_handle *mpi_klad_handle)
{
    td_s32 ret = TD_SUCCESS;
    klad_handle_ctl_t klad_handle_ctl = {0};

    crypto_uapi_func_enter();
    crypto_chk_return(inner_km_is_init() != TD_TRUE, KM_COMPAT_ERRNO(ERROR_NOT_INIT), "KM not init\n");
    km_null_ptr_chk(mpi_klad_handle);

    ret = crypto_km_ioctl(km_mgnt.dev_fd, CMD_KLAD_CREATE_HANDLE, &klad_handle_ctl);
    crypto_chk_return(ret != TD_SUCCESS, ret, "crypto_km_ioctl failed, ret is 0x%x\n", ret);

    *mpi_klad_handle = klad_handle_ctl.kapi_klad_handle;

    crypto_uapi_func_exit();
    return ret;
}

td_s32 uapi_klad_destroy(td_handle mpi_klad_handle)
{
    td_s32 ret = TD_SUCCESS;
    klad_handle_ctl_t klad_handle_ctl = {0};

    crypto_uapi_func_enter();
    crypto_chk_return(inner_km_is_init() != TD_TRUE, KM_COMPAT_ERRNO(ERROR_NOT_INIT), "KM not init\n");

    klad_handle_ctl.kapi_klad_handle = mpi_klad_handle;
    ret = crypto_km_ioctl(km_mgnt.dev_fd, CMD_KLAD_DESTROY_HANDLE, &klad_handle_ctl);
    crypto_chk_print(ret != TD_SUCCESS, "crypto_km_ioctl failed, ret is 0x%x\n", ret);

    crypto_uapi_func_exit();
    return ret;
}

td_s32 uapi_klad_attach(td_handle mpi_klad_handle, crypto_klad_dest klad_type,
    td_handle mpi_keyslot_handle)
{
    td_s32 ret = TD_SUCCESS;
    klad_attach_ctl_t klad_attach_ctl = {0};

    crypto_uapi_func_enter();
    crypto_chk_return(inner_km_is_init() != TD_TRUE, KM_COMPAT_ERRNO(ERROR_NOT_INIT), "KM not init\n");

    klad_attach_ctl.kapi_klad_handle = mpi_klad_handle;
    klad_attach_ctl.kapi_keyslot_handle =mpi_keyslot_handle;
    klad_attach_ctl.klad_type = klad_type;

    ret = crypto_km_ioctl(km_mgnt.dev_fd, CMD_KLAD_ATTACH, &klad_attach_ctl);
    crypto_chk_return(ret != TD_SUCCESS, ret, "crypto_km_ioctl failed, ret is 0x%x\n", ret);

    crypto_uapi_func_exit();
    return ret;
}

td_s32 uapi_klad_detach(td_handle mpi_klad_handle, crypto_klad_dest klad_type,
    td_handle mpi_keyslot_handle)
{
    td_s32 ret = TD_SUCCESS;

    klad_attach_ctl_t klad_attach_ctl = {0};

    crypto_uapi_func_enter();
    crypto_chk_return(inner_km_is_init() != TD_TRUE, KM_COMPAT_ERRNO(ERROR_NOT_INIT), "KM not init\n");

    klad_attach_ctl.kapi_klad_handle = mpi_klad_handle;
    klad_attach_ctl.kapi_keyslot_handle = mpi_keyslot_handle;
    klad_attach_ctl.klad_type = klad_type;

    ret = crypto_km_ioctl(km_mgnt.dev_fd, CMD_KLAD_DETACH, &klad_attach_ctl);
    crypto_chk_return(ret != TD_SUCCESS, ret, "crypto_km_ioctl failed, ret is 0x%x\n", ret);

    crypto_uapi_func_exit();
    return ret;
}

td_s32 uapi_klad_set_attr(td_handle mpi_klad_handle, const crypto_klad_attr *attr)
{
    td_s32 ret = TD_SUCCESS;
    klad_attr_ctl_t klad_attr_ctl = {0};

    crypto_uapi_func_enter();
    crypto_chk_return(inner_km_is_init() != TD_TRUE, KM_COMPAT_ERRNO(ERROR_NOT_INIT), "KM not init\n");
    km_null_ptr_chk(attr);

    klad_attr_ctl.kapi_klad_handle = mpi_klad_handle;
    (td_void)memcpy_s(&klad_attr_ctl.attr, sizeof(crypto_klad_attr), attr, sizeof(crypto_klad_attr));
    ret = crypto_km_ioctl(km_mgnt.dev_fd, CMD_KLAD_SET_ATTR, &klad_attr_ctl);
    crypto_chk_return(ret != TD_SUCCESS, ret, "crypto_km_ioctl failed, ret is 0x%x\n", ret);

    crypto_uapi_func_exit();
    return ret;
}

td_s32 uapi_klad_get_attr(td_handle mpi_klad_handle, crypto_klad_attr *attr)
{
    td_s32 ret = TD_SUCCESS;
    klad_attr_ctl_t klad_attr_ctl = {0};

    crypto_uapi_func_enter();
    crypto_chk_return(inner_km_is_init() != TD_TRUE, KM_COMPAT_ERRNO(ERROR_NOT_INIT), "KM not init\n");
    km_null_ptr_chk(attr);

    klad_attr_ctl.kapi_klad_handle = mpi_klad_handle;
    ret = crypto_km_ioctl(km_mgnt.dev_fd, CMD_KLAD_GET_ATTR, &klad_attr_ctl);
    crypto_chk_return(ret != TD_SUCCESS, ret, "crypto_km_ioctl failed, ret is 0x%x\n", ret);
    (td_void)memcpy_s(attr, sizeof(crypto_klad_attr), &klad_attr_ctl.attr, sizeof(crypto_klad_attr));

    crypto_uapi_func_exit();
    return ret;
}

#if defined(CONFIG_KM_SESSION_KEY_SUPPORT)
td_s32 uapi_klad_set_session_key(td_handle mpi_klad_handle, const crypto_klad_session_key *key)
{
    td_s32 ret = TD_SUCCESS;
    klad_set_session_key_ctl_t klad_set_session_key_ctl = {0};

    crypto_uapi_func_enter();
    crypto_chk_return(inner_km_is_init() != TD_TRUE, KM_COMPAT_ERRNO(ERROR_NOT_INIT), "KM not init\n");
    km_null_ptr_chk(key);
    km_null_ptr_chk(key->key);

    klad_set_session_key_ctl.kapi_klad_handle = mpi_klad_handle;
    klad_set_session_key_ctl.alg = key->alg;
    klad_set_session_key_ctl.level = key->level;
    klad_set_session_key_ctl.key_size = key->key_length;
    klad_set_session_key_ctl.key.p = (unsigned char *)key->key;
    ret = crypto_km_ioctl(km_mgnt.dev_fd, CMD_KLAD_SET_SESSION_KEY, &klad_set_session_key_ctl);
    crypto_chk_return(ret != TD_SUCCESS, ret, "crypto_km_ioctl failed, ret is 0x%x\n", ret);

    crypto_uapi_func_exit();
    return ret;
}
#else
td_s32 uapi_klad_set_session_key(td_handle mpi_klad_handle, const crypto_klad_session_key *key)
{
    crypto_unused(mpi_klad_handle);
    crypto_unused(key);
    return ERROR_UNSUPPORT;
}
#endif

#if defined(CONFIG_KM_CONTENT_KEY_SUPPORT)
td_s32 uapi_klad_set_content_key(td_handle mpi_klad_handle, const crypto_klad_content_key *key)
{
    td_s32 ret = TD_SUCCESS;
    klad_set_content_key_ctl_t klad_set_content_key_ctl = {0};

    crypto_uapi_func_enter();
    crypto_chk_return(inner_km_is_init() != TD_TRUE, KM_COMPAT_ERRNO(ERROR_NOT_INIT), "KM not init\n");
    km_null_ptr_chk(key);
    km_null_ptr_chk(key->key);

    klad_set_content_key_ctl.kapi_klad_handle = mpi_klad_handle;
    klad_set_content_key_ctl.alg = key->alg;
    klad_set_content_key_ctl.key_parity = key->key_parity;
    klad_set_content_key_ctl.key_size = key->key_length;
    klad_set_content_key_ctl.key.p = (unsigned char *)key->key;
    ret = crypto_km_ioctl(km_mgnt.dev_fd, CMD_KLAD_SET_CONTENT_KEY, &klad_set_content_key_ctl);
    crypto_chk_return(ret != TD_SUCCESS, ret, "crypto_km_ioctl failed, ret is 0x%x\n", ret);

    crypto_uapi_func_exit();
    return ret;
}
#else
td_s32 uapi_klad_set_content_key(td_handle mpi_klad_handle, const crypto_klad_content_key *key)
{
    crypto_unused(mpi_klad_handle);
    crypto_unused(key);
    return ERROR_UNSUPPORT;
}
#endif

#if defined(CONFIG_KM_CLEAR_KEY_SUPPORT)
td_s32 uapi_klad_set_clear_key(td_handle mpi_klad_handle, const crypto_klad_clear_key *key)
{
    td_s32 ret = TD_SUCCESS;
    klad_set_clear_key_ctl_t klad_set_clear_key_ctl = {0};

    crypto_uapi_func_enter();
    crypto_chk_return(inner_km_is_init() != TD_TRUE, KM_COMPAT_ERRNO(ERROR_NOT_INIT), "KM not init\n");
    km_null_ptr_chk(key);
    km_null_ptr_chk(key->key);

    klad_set_clear_key_ctl.kapi_klad_handle = mpi_klad_handle;
    klad_set_clear_key_ctl.hmac_type = key->hmac_type;
    klad_set_clear_key_ctl.key_parity = key->key_parity;
    klad_set_clear_key_ctl.key_size = key->key_length;
    klad_set_clear_key_ctl.key.p = (unsigned char *)key->key;
    ret = crypto_km_ioctl(km_mgnt.dev_fd, CMD_KLAD_SET_CLEAR_KEY, &klad_set_clear_key_ctl);
    crypto_chk_return(ret != TD_SUCCESS, ret, "crypto_km_ioctl failed, ret is 0x%x\n", ret);

    crypto_uapi_func_exit();
    return ret;
}
#else
td_s32 uapi_klad_set_clear_key(td_handle mpi_klad_handle, const crypto_klad_clear_key *key)
{
    crypto_unused(mpi_klad_handle);
    crypto_unused(key);
    return ERROR_UNSUPPORT;
}
#endif

#if defined(CONFIG_KM_EFFECTIVE_KEY_SUPPORT)
td_s32 uapi_klad_set_effective_key(td_handle mpi_klad_handle, const crypto_klad_effective_key *key)
{
    td_s32 ret = TD_SUCCESS;
    klad_set_effective_key_ctl_t klad_set_effective_key_ctl = {0};

    crypto_uapi_func_enter();
    crypto_chk_return(inner_km_is_init() != TD_TRUE, KM_COMPAT_ERRNO(ERROR_NOT_INIT), "KM not init\n");
    km_null_ptr_chk(key);
    km_null_ptr_chk(key->salt);

    klad_set_effective_key_ctl.kapi_klad_handle = mpi_klad_handle;
    klad_set_effective_key_ctl.kdf_hard_alg = key->kdf_hard_alg;
    klad_set_effective_key_ctl.key_parity = key->key_parity;
    klad_set_effective_key_ctl.key_size = key->key_size;
    klad_set_effective_key_ctl.oneway = key->oneway;
    klad_set_effective_key_ctl.salt_length = key->salt_length;
    klad_set_effective_key_ctl.salt = key->salt;

    ret = crypto_km_ioctl(km_mgnt.dev_fd, CMD_KLAD_SET_EFFECTVE_KEY, &klad_set_effective_key_ctl);
    crypto_chk_return(ret != TD_SUCCESS, ret, "crypto_km_ioctl failed, ret is 0x%x\n", ret);

    crypto_uapi_func_exit();
    return ret;
}
#else
td_s32 uapi_klad_set_effective_key(td_handle mpi_klad_handle, const crypto_klad_effective_key *key)
{
    crypto_unused(mpi_klad_handle);
    crypto_unused(key);
    return ERROR_UNSUPPORT;
}
#endif

#if defined(CONFIG_SYMC_HARDEN_INTERFACE_SUPPORT)
td_s32 uapi_km_create_key_impl(td_handle *keyslot_handle)
{
    td_s32 ret = TD_SUCCESS;
    km_create_key_impl_ctl_t km_key = {0};

    crypto_uapi_func_enter();
    km_null_ptr_chk(keyslot_handle);

    ret = uapi_km_init();
    crypto_chk_return(ret != TD_SUCCESS, ret, "uapi_km_init uapi_km_init, ret is 0x%x\n", ret);

    ret = crypto_km_ioctl(km_mgnt.dev_fd, CMD_KM_CREATE_KEY_IMPL, &km_key);
    crypto_chk_goto_with_ret(ret, ret != TD_SUCCESS, km_deinit, ret, "crypto_km_ioctl failed, ret is 0x%x\n", ret);

    *keyslot_handle = km_key.kapi_kslot_handle;
km_deinit:
    uapi_km_deinit();
    crypto_uapi_func_exit();
    return ret;
}

td_s32 uapi_km_set_key_impl(td_handle keyslot_handle, const td_u8 *key, td_u32 key_len, td_bool tee_open)
{
    td_s32 ret = TD_SUCCESS;
    km_set_impl_ctl_t km_key = {0};

    crypto_uapi_func_enter();
    km_null_ptr_chk(key);

    km_key.key.p = (void *)key;
    km_key.key_size = key_len;
    km_key.tee_open = tee_open;
    km_key.kapi_kslot_handle = keyslot_handle;

    ret = uapi_km_init();
    crypto_chk_return(ret != TD_SUCCESS, ret, "uapi_km_init uapi_km_init, ret is 0x%x\n", ret);

    ret = crypto_km_ioctl(km_mgnt.dev_fd, CMD_KM_SET_KEY_IMPL, &km_key);
    crypto_chk_goto_with_ret(ret, ret != TD_SUCCESS, km_deinit, ret, "crypto_km_ioctl failed, ret is 0x%x\n", ret);
km_deinit:
    uapi_km_deinit();
    crypto_uapi_func_exit();
    return ret;
}

td_s32 uapi_km_destroy_key_impl(td_handle keyslot_handle)
{
    td_s32 ret = TD_SUCCESS;
    km_destroy_key_impl_ctl_t km_handle = {0};

    crypto_uapi_func_enter();

    km_handle.kapi_keyslot_handle = keyslot_handle;

    ret = uapi_km_init();
    crypto_chk_return(ret != TD_SUCCESS, ret, "uapi_km_init uapi_km_init, ret is 0x%x\n", ret);

    ret = crypto_km_ioctl(km_mgnt.dev_fd, CMD_KM_DESTROY_KEY_IMPL, &km_handle);
    crypto_chk_goto_with_ret(ret, ret != TD_SUCCESS, km_deinit, ret, "crypto_km_ioctl failed, ret is 0x%x\n", ret);
km_deinit:
    uapi_km_deinit();
    crypto_uapi_func_exit();
    return ret;
}
#endif