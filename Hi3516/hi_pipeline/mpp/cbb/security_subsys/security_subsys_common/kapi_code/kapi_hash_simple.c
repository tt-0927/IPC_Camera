/**
  Copyright (c), 2001-2025, Shenshu Tech. Co., Ltd.
*/

#include <securec.h>
#include "kapi_hash.h"
#include "kapi_inner.h"

#include "drv_hash.h"
#include "crypto_common_macro.h"
#include "crypto_common_def.h"
#include "crypto_errno.h"
#include "crypto_hash_common.h"

#define INVALID_HANDLE      0xFFFFFFFF

#define HASH_COMPAT_ERRNO(err_code)     KAPI_COMPAT_ERRNO(ERROR_MODULE_HASH, err_code)

#define HASH_DMA_ADD_LEN 128

typedef struct {
    crypto_hash_clone_ctx clone_ctx;
    crypto_owner owner;
    td_handle drv_hash_handle;
    unsigned char *dma_buf;
    unsigned int dma_buf_len;
    unsigned char tail_len; /* not large than 128. */
    unsigned int is_keyslot         : 1;
    unsigned int is_long_term       : 1;
} kapi_hash_context;

static kapi_hash_context *g_hash_ctx[CONFIG_HASH_VIRT_CHN_NUM];

#define kapi_hash_lock()
#define kapi_hash_unlock()

#define kapi_hash_common_lock() do {                   \
    hash_common_lock();           \
} while (0)
#define kapi_hash_common_unlock() do {                 \
    hash_common_unlock();         \
} while (0)

static td_s32 inner_kapi_hash_handle_check(td_handle kapi_hash_handle)
{
    crypto_chk_return(kapi_get_module_id(kapi_hash_handle) != KAPI_HASH_MODULE_ID,
        HASH_COMPAT_ERRNO(ERROR_INVALID_HANDLE), "Invalid Hash Handle! 0x%x\n", kapi_hash_handle);
    crypto_chk_return(kapi_get_ctx_idx(kapi_hash_handle) >= CONFIG_HASH_VIRT_CHN_NUM,
        HASH_COMPAT_ERRNO(ERROR_INVALID_HANDLE), "Invalid Hash Handle! 0x%x\n", kapi_hash_handle);
    return TD_SUCCESS;
}

static kapi_hash_context *inner_kapi_get_hash_ctx(td_handle hash_handle)
{
    unsigned int idx = kapi_get_ctx_idx(hash_handle);
    crypto_owner owner = {0};

    if (inner_kapi_hash_handle_check(hash_handle) != TD_SUCCESS) {
        return NULL;
    }
    if (g_hash_ctx[idx] == NULL) {
        return NULL;
    }
    crypto_get_owner(&owner);
    if (memcmp(&g_hash_ctx[idx]->owner, &owner, sizeof(crypto_owner)) != 0) {
        return NULL;
    }
    return g_hash_ctx[idx];
}

static int inner_kapi_alloc_hash_ctx(td_handle *hash_handle, kapi_hash_context **hash_ctx)
{
    int ret = TD_FAILURE;
    unsigned int i;
    kapi_hash_lock();
    for (i = 0; i < CONFIG_HASH_VIRT_CHN_NUM; i++) {
        if (g_hash_ctx[i] == NULL) {
            break;
        }
    }
    if (i >= CONFIG_HASH_VIRT_CHN_NUM) {
        crypto_log_err("All hash contexts are busy!\n");
        ret = HASH_COMPAT_ERRNO(ERROR_CHN_BUSY);
        goto exit_unlock;
    }

    *hash_ctx = crypto_malloc(sizeof(kapi_hash_context));
    crypto_chk_goto_with_ret(ret, *hash_ctx == NULL, exit_unlock,
        HASH_COMPAT_ERRNO(ERROR_MALLOC), "crypto_malloc failed\n");
    (void)memset_s(*hash_ctx, sizeof(kapi_hash_context), 0, sizeof(kapi_hash_context));

    (*hash_ctx)->dma_buf = crypto_malloc_mmz(CONFIG_HASH_KAPI_DMA_BUF_LEN + HASH_DMA_ADD_LEN, "hash_dma_buf");
    if ((*hash_ctx)->dma_buf == NULL) {
        crypto_free(*hash_ctx);
        crypto_log_err("crypto_malloc failed\n");
        ret = HASH_COMPAT_ERRNO(ERROR_MALLOC);
        goto exit_unlock;
    }

    (*hash_ctx)->dma_buf_len = CONFIG_HASH_KAPI_DMA_BUF_LEN;

    g_hash_ctx[i] = *hash_ctx;
    crypto_get_owner(&(g_hash_ctx[i]->owner));
    *hash_handle = synthesize_kapi_handle(KAPI_HASH_MODULE_ID, i);
    ret = CRYPTO_SUCCESS;
exit_unlock:
    kapi_hash_unlock();
    return ret;
}

static int inner_kapi_free_hash_ctx(td_handle hash_handle)
{
    kapi_hash_context *hash_ctx = NULL;
    unsigned int idx;

    if (inner_kapi_hash_handle_check(hash_handle) != TD_SUCCESS) {
        return HASH_COMPAT_ERRNO(ERROR_INVALID_HANDLE);
    }
    idx = kapi_get_ctx_idx(hash_handle);
    hash_ctx = inner_kapi_get_hash_ctx(hash_handle);
    crypto_chk_return(hash_ctx == NULL, HASH_COMPAT_ERRNO(ERROR_INVALID_HANDLE), "inner_kapi_get_hash_ctx failed\n");
    kapi_hash_lock();
    if (hash_ctx->dma_buf != NULL) {
        crypto_free_coherent(hash_ctx->dma_buf);
        hash_ctx->dma_buf = NULL;
    }
    crypto_free(hash_ctx);
    g_hash_ctx[idx] = NULL;
    hash_ctx = NULL;
    kapi_hash_unlock();
    return CRYPTO_SUCCESS;
}

static td_s32 inner_drv_lock_start(kapi_hash_context *hash_ctx, const crypto_hash_attr *hash_attr)
{
    td_s32 ret = TD_SUCCESS;
    kapi_hash_lock();
    ret = drv_cipher_hash_start(&hash_ctx->drv_hash_handle, hash_attr);
    kapi_hash_unlock();
    return ret;
}

td_s32 kapi_cipher_hash_env_init(td_void)
{
    td_s32 ret = TD_SUCCESS;
    crypto_kapi_func_enter();

    ret = drv_cipher_hash_init();
    if (ret != TD_SUCCESS) {
        crypto_log_err("drv_cipher_hash_init failed\n");
        return ret;
    }

    ret = hash_mutex_init();
    if (ret != TD_SUCCESS) {
        crypto_log_err("crypto_mutex_init failed\n");
        ret = HASH_COMPAT_ERRNO(ERROR_MUTEX_INIT);
        goto error_hash_deinit;
    }

    crypto_kapi_func_exit();
    return ret;

error_hash_deinit:
    drv_cipher_hash_deinit();
    return ret;
}

td_s32 kapi_cipher_hash_env_deinit(td_void)
{
    unsigned int i;
    td_s32 ret = TD_SUCCESS;
    crypto_kapi_func_enter();
    for (i = 0; i < CONFIG_HASH_VIRT_CHN_NUM; i++) {
        if (g_hash_ctx[i] == NULL) {
            continue;
        }
        if (g_hash_ctx[i]->dma_buf != NULL) {
            crypto_free_coherent(g_hash_ctx[i]->dma_buf);
            g_hash_ctx[i]->dma_buf = NULL;
        }
        crypto_free(g_hash_ctx[i]);
        g_hash_ctx[i] = NULL;
    }
    hash_mutex_destroy();
    drv_cipher_hash_deinit();
    crypto_kapi_func_exit();
    return ret;
}

td_void kapi_cipher_hash_process_release(td_void)
{
    unsigned int i;
    crypto_owner owner = {0};

    crypto_get_owner(&owner);
    kapi_hash_lock();
    for (i = 0; i < CONFIG_HASH_VIRT_CHN_NUM; i++) {
        if (g_hash_ctx[i] == NULL) {
            continue;
        }
        if (memcmp(&g_hash_ctx[i]->owner, &owner, sizeof(crypto_owner)) != 0) {
            continue;
        }
        if (g_hash_ctx[i]->is_long_term) {
            (void)drv_cipher_hash_destroy(g_hash_ctx[i]->drv_hash_handle);
        }
        if (g_hash_ctx[i]->dma_buf != NULL) {
            crypto_free_coherent(g_hash_ctx[i]->dma_buf);
            g_hash_ctx[i]->dma_buf = NULL;
        }
        crypto_free(g_hash_ctx[i]);
        g_hash_ctx[i] = NULL;
    }
    kapi_hash_unlock();
}

td_s32 kapi_cipher_hash_init(td_void)
{
    td_s32 ret = TD_SUCCESS;
    crypto_kapi_func_enter();

    crypto_kapi_func_exit();
    return ret;
}
CRYPTO_EXPORT_SYMBOL(kapi_cipher_hash_init);

td_s32 kapi_cipher_hash_deinit(td_void)
{
    crypto_kapi_func_enter();
    crypto_kapi_func_exit();
    return TD_SUCCESS;
}
CRYPTO_EXPORT_SYMBOL(kapi_cipher_hash_deinit);

static td_s32 inner_kapi_cipher_hash_start(td_handle *kapi_hash_handle, const crypto_hash_attr *hash_attr)
{
    int ret = TD_FAILURE;
    kapi_hash_context *hash_ctx = NULL;
    crypto_kapi_func_enter();

    hash_null_ptr_chk(kapi_hash_handle);
    hash_null_ptr_chk(hash_attr);

    ret = inner_kapi_alloc_hash_ctx(kapi_hash_handle, &hash_ctx);
    crypto_chk_return(ret != CRYPTO_SUCCESS, ret, "inner_kapi_alloc_hash_ctx failed\n");

    if (crypto_hash_is_hmac(hash_attr->hash_type) == TD_TRUE) {
        ret = inner_drv_lock_start(hash_ctx, hash_attr);
        crypto_chk_goto(ret != TD_SUCCESS, error_free_ctx, "inner_drv_lock_start failed\n");
        hash_ctx->is_keyslot = hash_attr->is_keyslot;
        hash_ctx->is_long_term = TD_TRUE;
        crypto_kapi_func_exit();
        return CRYPTO_SUCCESS;
    }

    (void)memset_s(&hash_ctx->clone_ctx, sizeof(hash_ctx->clone_ctx), 0, sizeof(hash_ctx->clone_ctx));

    ret = drv_hash_get_state_iv(hash_attr->hash_type, TD_NULL,
        hash_ctx->clone_ctx.state, sizeof(hash_ctx->clone_ctx.state));
    crypto_chk_goto(ret != CRYPTO_SUCCESS, error_free_ctx, "drv_hash_get_state_iv failed\n");

    hash_ctx->clone_ctx.hash_type = hash_attr->hash_type;
    crypto_kapi_func_exit();

    return 0;
error_free_ctx:
    inner_kapi_free_hash_ctx(*kapi_hash_handle);
    return ret;
}

td_s32 kapi_cipher_hash_start(td_handle *kapi_hash_handle, const crypto_hash_attr *hash_attr)
{
    td_s32 ret;
    kapi_hash_common_lock();
    ret = inner_kapi_cipher_hash_start(kapi_hash_handle, hash_attr);
    kapi_hash_common_unlock();

    return ret;
}
CRYPTO_EXPORT_SYMBOL(kapi_cipher_hash_start);


static td_s32 inner_kapi_cipher_hash_update(td_handle kapi_hash_handle,  const crypto_buf_attr *src_buf,
    const td_u32 len)
{
    int ret;
    td_handle hash_handle = INVALID_HANDLE;
    crypto_hash_attr hash_attr = {0};
    kapi_hash_context *hash_ctx = NULL;
    crypto_kapi_func_enter();

    hash_ctx = inner_kapi_get_hash_ctx(kapi_hash_handle);
    crypto_chk_return(hash_ctx == NULL, HASH_COMPAT_ERRNO(ERROR_INVALID_HANDLE), "inner_kapi_get_hash_ctx failed\n");

    if (hash_ctx->is_long_term == TD_TRUE) {
        ret = drv_cipher_hash_update(hash_ctx->drv_hash_handle, src_buf, len);
        crypto_chk_return(ret != 0, ret, "drv_cipher_hash_update failed\n");

        return CRYPTO_SUCCESS;
    }

    kapi_hash_lock();
    hash_attr.hash_type = hash_ctx->clone_ctx.hash_type;
    hash_attr.is_keyslot = hash_ctx->is_keyslot;
    ret = drv_cipher_hash_start(&hash_handle, &hash_attr);
    crypto_chk_goto(ret != 0, exit_mutex_unlock, "drv_cipher_hash_set failed\n");

    ret = drv_cipher_hash_set(hash_handle, &hash_ctx->clone_ctx);
    crypto_chk_goto(ret != 0, exit_hash_destroy, "drv_cipher_hash_set failed\n");

    ret = drv_cipher_hash_update(hash_handle, src_buf, len);
    crypto_chk_goto(ret != 0, exit_hash_destroy, "drv_cipher_hash_update failed\n");

    ret = drv_cipher_hash_get(hash_handle, &hash_ctx->clone_ctx);
    crypto_chk_goto(ret != 0, exit_hash_destroy, "drv_cipher_hash_get failed\n");

exit_hash_destroy:
    (void)drv_cipher_hash_destroy(hash_handle);
exit_mutex_unlock:
    kapi_hash_unlock();
    crypto_kapi_func_exit();
    return ret;
}
#define HASH_MMZ_LENGTH     (64 * 1024)
static td_s32 inner_kapi_cipher_hash_update_common(td_handle kapi_hash_handle,  const crypto_buf_attr *src_buf,
    const td_u32 len)
{
    int ret;
    crypto_buf_attr new_buf;
    unsigned char *mmz_buf = NULL;
    unsigned int left = len;
    unsigned int processing_len = 0;
    unsigned int processed_len = 0;

    hash_null_ptr_chk(src_buf);
    if (crypto_get_phys_addr(src_buf->virt_addr) != 0) {
        return inner_kapi_cipher_hash_update(kapi_hash_handle, src_buf, len);
    }

    mmz_buf = crypto_malloc_coherent(HASH_MMZ_LENGTH, "kapi_hash_buf");
    crypto_chk_return(mmz_buf == NULL, HASH_COMPAT_ERRNO(ERROR_MALLOC), "crypto_malloc_coherent failed\n");

    new_buf.virt_addr = mmz_buf;
    while (left > 0) {
        processing_len = crypto_min(left, HASH_MMZ_LENGTH);
        ret = memcpy_s(mmz_buf, HASH_MMZ_LENGTH, src_buf->virt_addr + processed_len, processing_len);
        crypto_chk_goto_with_ret(ret, ret != EOK, exit_free, HASH_COMPAT_ERRNO(ERROR_MEMCPY_S), "memcpy_s failed\n");

        ret = inner_kapi_cipher_hash_update(kapi_hash_handle, &new_buf, processing_len);
        crypto_chk_goto(ret != CRYPTO_SUCCESS, exit_free, "inner_kapi_cipher_hash_update failed\n");

        left -= processing_len;
        processed_len += processing_len;
    }

    ret = CRYPTO_SUCCESS;
exit_free:
    crypto_free_coherent(mmz_buf);
    return ret;
}

td_s32 kapi_cipher_hash_update(td_handle kapi_hash_handle,  const crypto_buf_attr *src_buf, const td_u32 len)
{
    td_s32 ret;
    kapi_hash_common_lock();
    ret = inner_kapi_cipher_hash_update_common(kapi_hash_handle, src_buf, len);
    kapi_hash_common_unlock();

    return ret;
}
CRYPTO_EXPORT_SYMBOL(kapi_cipher_hash_update);

static td_s32 inner_kapi_cipher_hash_finish(td_handle kapi_hash_handle, td_u8 *out, td_u32 *out_len)
{
    int ret;
    td_handle hash_handle = INVALID_HANDLE;
    crypto_hash_attr hash_attr;
    kapi_hash_context *hash_ctx = NULL;
    crypto_buf_attr src_buf;
    crypto_kapi_func_enter();

    hash_ctx = inner_kapi_get_hash_ctx(kapi_hash_handle);
    crypto_chk_return(hash_ctx == NULL, HASH_COMPAT_ERRNO(ERROR_INVALID_HANDLE), "inner_kapi_get_hash_ctx failed\n");

    kapi_hash_lock();

    if (hash_ctx->is_long_term == TD_TRUE) {
        if (hash_ctx->tail_len != 0) {
            src_buf.virt_addr = hash_ctx->dma_buf;
            ret = drv_cipher_hash_update(hash_ctx->drv_hash_handle, &src_buf, hash_ctx->tail_len);
            crypto_chk_goto(ret != 0, exit_hash_destroy, "drv_cipher_hash_update failed\n");
        }
        ret = drv_cipher_hash_finish(hash_ctx->drv_hash_handle, out, out_len);
        if (ret != TD_SUCCESS) {
            crypto_log_err("drv_cipher_hash_finish failed\n");
            goto exit_hash_destroy;
        }
        ret = CRYPTO_SUCCESS;
        goto exit_mutex_unlock;
    }

    hash_attr.hash_type = hash_ctx->clone_ctx.hash_type;
    hash_attr.is_keyslot = hash_ctx->is_keyslot;
    ret = drv_cipher_hash_start(&hash_handle, &hash_attr);
    crypto_chk_goto(ret != 0, exit_mutex_unlock, "drv_cipher_hash_start failed\n");

    ret = drv_cipher_hash_set(hash_handle, &hash_ctx->clone_ctx);
    crypto_chk_goto(ret != 0, exit_hash_destroy, "drv_cipher_hash_set failed\n");

    ret = drv_cipher_hash_finish(hash_handle, out, out_len);
    crypto_chk_goto(ret != 0, exit_hash_destroy, "drv_cipher_hash_finish failed\n");
    hash_handle = INVALID_HANDLE;

exit_hash_destroy:
    if (hash_handle != INVALID_HANDLE) {
        (void)drv_cipher_hash_destroy(hash_handle);
    }
exit_mutex_unlock:
    kapi_hash_unlock();
    inner_kapi_free_hash_ctx(kapi_hash_handle);
    crypto_kapi_func_exit();
    return ret;
}

td_s32 kapi_cipher_hash_finish(td_handle kapi_hash_handle, td_u8 *out, td_u32 *out_len)
{
    td_s32 ret;
    kapi_hash_common_lock();
    ret = inner_kapi_cipher_hash_finish(kapi_hash_handle, out, out_len);
    kapi_hash_common_unlock();

    return ret;
}
CRYPTO_EXPORT_SYMBOL(kapi_cipher_hash_finish);

static td_s32 inner_kapi_cipher_hash_get(td_handle kapi_hash_handle, crypto_hash_clone_ctx *hash_clone_ctx)
{
    int ret;
    kapi_hash_context *hash_ctx = NULL;
    crypto_kapi_func_enter();

    hash_ctx = inner_kapi_get_hash_ctx(kapi_hash_handle);
    crypto_chk_return(hash_ctx == NULL, HASH_COMPAT_ERRNO(ERROR_INVALID_HANDLE), "inner_kapi_get_hash_ctx failed\n");

    if (hash_ctx->is_long_term == TD_TRUE) {
        ret = drv_cipher_hash_get(hash_ctx->drv_hash_handle, hash_clone_ctx);
        crypto_chk_return(ret != CRYPTO_SUCCESS, ret, "drv_cipher_hash_get failed\n");
    } else {
        ret = memcpy_s(hash_clone_ctx, sizeof(crypto_hash_clone_ctx),
            &hash_ctx->clone_ctx, sizeof(crypto_hash_clone_ctx));
        crypto_chk_return(ret != EOK, HASH_COMPAT_ERRNO(ERROR_MEMCPY_S), "memcpy_s failed\n");
    }

    crypto_kapi_func_exit();
    return 0;
}

td_s32 kapi_cipher_hash_get(td_handle kapi_hash_handle, crypto_hash_clone_ctx *hash_clone_ctx)
{
    td_s32 ret;
    kapi_hash_common_lock();
    ret = inner_kapi_cipher_hash_get(kapi_hash_handle, hash_clone_ctx);
    kapi_hash_common_unlock();

    return ret;
}
CRYPTO_EXPORT_SYMBOL(kapi_cipher_hash_get);

static td_s32 inner_kapi_cipher_hash_set(td_handle kapi_hash_handle, const crypto_hash_clone_ctx *hash_clone_ctx)
{
    int ret;
    kapi_hash_context *hash_ctx = NULL;
    crypto_kapi_func_enter();

    hash_ctx = inner_kapi_get_hash_ctx(kapi_hash_handle);
    crypto_chk_return(hash_ctx == NULL, HASH_COMPAT_ERRNO(ERROR_INVALID_HANDLE), "inner_kapi_get_hash_ctx failed\n");

    if (hash_ctx->is_long_term == TD_TRUE) {
        ret = drv_cipher_hash_set(hash_ctx->drv_hash_handle, hash_clone_ctx);
        crypto_chk_return(ret != CRYPTO_SUCCESS, ret, "drv_cipher_hash_get failed\n");
    } else {
        ret = memcpy_s(&hash_ctx->clone_ctx, sizeof(crypto_hash_clone_ctx),
            hash_clone_ctx, sizeof(crypto_hash_clone_ctx));
        crypto_chk_return(ret != EOK, HASH_COMPAT_ERRNO(ERROR_MEMCPY_S), "memcpy_s failed\n");
    }

    crypto_kapi_func_exit();
    return 0;
}

td_s32 kapi_cipher_hash_set(td_handle kapi_hash_handle, const crypto_hash_clone_ctx *hash_clone_ctx)
{
    td_s32 ret;
    kapi_hash_common_lock();
    ret = inner_kapi_cipher_hash_set(kapi_hash_handle, hash_clone_ctx);
    kapi_hash_common_unlock();

    return ret;
}
CRYPTO_EXPORT_SYMBOL(kapi_cipher_hash_set);

static td_s32 inner_kapi_cipher_hash_destroy(td_handle kapi_hash_handle)
{
    int ret;
    kapi_hash_context *hash_ctx = NULL;
    crypto_kapi_func_enter();

    hash_ctx = inner_kapi_get_hash_ctx(kapi_hash_handle);
    crypto_chk_return(hash_ctx == NULL, HASH_COMPAT_ERRNO(ERROR_INVALID_HANDLE), "inner_kapi_get_hash_ctx failed\n");

    if (hash_ctx->is_long_term == TD_TRUE) {
        (void)drv_cipher_hash_destroy(hash_ctx->drv_hash_handle);
    }
    ret = inner_kapi_free_hash_ctx(kapi_hash_handle);
    crypto_kapi_func_exit();
    return ret;
}

td_s32 kapi_cipher_hash_destroy(td_handle kapi_hash_handle)
{
    td_s32 ret;
    kapi_hash_common_lock();
    ret = inner_kapi_cipher_hash_destroy(kapi_hash_handle);
    kapi_hash_common_unlock();

    return ret;
}
CRYPTO_EXPORT_SYMBOL(kapi_cipher_hash_destroy);

static td_s32 inner_kapi_cipher_pbkdf2(const crypto_kdf_pbkdf2_param *param, td_u8 *out, const td_u32 out_len)
{
    td_s32 ret = TD_SUCCESS;
    crypto_kapi_func_enter();

    kapi_hash_lock();
    ret = drv_cipher_pbkdf2(param, out, out_len);
    kapi_hash_unlock();
    crypto_kapi_func_exit();
    return ret;
}

td_s32 kapi_cipher_pbkdf2(const crypto_kdf_pbkdf2_param *param, td_u8 *out, const td_u32 out_len)
{
    td_s32 ret;
    kapi_hash_common_lock();
    ret = inner_kapi_cipher_pbkdf2(param, out, out_len);
    kapi_hash_common_unlock();

    return ret;
}
CRYPTO_EXPORT_SYMBOL(kapi_cipher_pbkdf2);