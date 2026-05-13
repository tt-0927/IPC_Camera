/*
  Copyright (c), 2001-2025, Shenshu Tech. Co., Ltd.
 */

#include "ot_mpi_cipher.h"
#include "ot_mpi_utils.h"

td_s32 ot_mpi_cipher_hash_init(td_void)
{
    return UNIFY_MPI_HASH_INIT();
}

td_s32 ot_mpi_cipher_hash_deinit(td_void)
{
    return UNIFY_MPI_HASH_DEINIT();
}

td_s32 ot_mpi_cipher_hash_create(td_handle *mpi_hash_handle, const crypto_hash_attr *hash_attr)
{
    return UNIFY_MPI_HASH_START(mpi_hash_handle, hash_attr);
}

td_s32 ot_mpi_cipher_hash_update(td_handle mpi_hash_handle, const crypto_buf_attr *src_buf, const td_u32 len)
{
    return UNIFY_MPI_HASH_UPDATE(mpi_hash_handle, src_buf, len);
}

td_s32 ot_mpi_cipher_hash_finish(td_handle mpi_hash_handle, td_u8 *virt_addr, td_u32 buffer_len, td_u32 *result_len)
{
    td_s32 ret;

    crypto_chk_return_only(result_len == NULL, UAPI_COMPAT_ERRNO(ERROR_MODULE_HASH, ERROR_PARAM_IS_NULL));
    *result_len = buffer_len;
    ret = UNIFY_MPI_HASH_FINISH(mpi_hash_handle, virt_addr, result_len);
    return ret;
}

td_s32 ot_mpi_cipher_hash_get(td_handle mpi_hash_handle, crypto_hash_clone_ctx *hash_clone_ctx)
{
    return UNIFY_MPI_HASH_GET(mpi_hash_handle, hash_clone_ctx);
}

td_s32 ot_mpi_cipher_hash_set(td_handle mpi_hash_handle, const crypto_hash_clone_ctx *hash_clone_ctx)
{
    return UNIFY_MPI_HASH_SET(mpi_hash_handle, hash_clone_ctx);
}

td_s32 ot_mpi_cipher_hash_destroy(td_handle mpi_hash_handle)
{
    return UNIFY_MPI_HASH_DESTROY(mpi_hash_handle);
}
