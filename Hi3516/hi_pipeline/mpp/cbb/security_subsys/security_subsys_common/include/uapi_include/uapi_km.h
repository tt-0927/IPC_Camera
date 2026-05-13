/*
  Copyright (c), 2001-2025, Shenshu Tech. Co., Ltd.
 */

#ifndef UAPI_KM_H
#define UAPI_KM_H

#include "crypto_km_struct.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif /* __cplusplus */
#endif /* __cplusplus */

td_s32 uapi_km_init(td_void);
td_s32 uapi_km_deinit(td_void);

/* Keyslot. */
td_s32 uapi_keyslot_create(td_handle *mpi_keyslot_handle, crypto_keyslot_type keyslot_type);
td_s32 uapi_keyslot_destroy(td_handle mpi_keyslot_handle);

/* Klad. */
td_s32 uapi_klad_create(td_handle *mpi_klad_handle);
td_s32 uapi_klad_destroy(td_handle mpi_klad_handle);

td_s32 uapi_klad_attach(td_handle mpi_klad_handle, crypto_klad_dest klad_type,
    td_handle mpi_keyslot_handle);
td_s32 uapi_klad_detach(td_handle mpi_klad_handle, crypto_klad_dest klad_type,
    td_handle mpi_keyslot_handle);

td_s32 uapi_klad_set_attr(td_handle mpi_klad_handle, const crypto_klad_attr *attr);
td_s32 uapi_klad_get_attr(td_handle mpi_klad_handle, crypto_klad_attr *attr);

td_s32 uapi_klad_set_session_key(td_handle mpi_klad_handle, const crypto_klad_session_key *key);
td_s32 uapi_klad_set_content_key(td_handle mpi_klad_handle, const crypto_klad_content_key *key);

td_s32 uapi_klad_set_clear_key(td_handle mpi_klad_handle, const crypto_klad_clear_key *key);
td_s32 uapi_klad_set_effective_key(td_handle mpi_klad_handle, const crypto_klad_effective_key *key);

td_s32 uapi_km_create_key_impl(td_handle *keyslot_handle);
td_s32 uapi_km_set_key_impl(td_handle keyslot_handle, const td_u8 *key, td_u32 key_len, td_bool tee_open);
td_s32 uapi_km_destroy_key_impl(td_handle keyslot_handle);

#ifdef __cplusplus
#if __cplusplus
}
#endif /* __cplusplus */
#endif /* __cplusplus */

#endif