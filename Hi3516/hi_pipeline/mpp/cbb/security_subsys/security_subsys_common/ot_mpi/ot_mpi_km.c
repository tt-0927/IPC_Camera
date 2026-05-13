/*
  Copyright (c), 2001-2025, Shenshu Tech. Co., Ltd.
 */

#include "ot_mpi_km.h"
#include "ot_mpi_utils.h"
#include "crypto_km_struct.h"
#include "securec.h"

/* Init */
td_s32 ot_mpi_km_init(td_void)
{
    return UNIFY_MPI_KM_INIT();
}

td_s32 ot_mpi_km_deinit(td_void)
{
    return UNIFY_MPI_KM_DEINIT();
}

/* Keyslot. */
td_s32 ot_mpi_keyslot_create(td_handle *mpi_keyslot_handle, km_keyslot_type keyslot_type)
{
    return UNIFY_MPI_KEYSLOT_CREATE(mpi_keyslot_handle, (crypto_keyslot_type)keyslot_type);
}

td_s32 ot_mpi_keyslot_destroy(td_handle mpi_keyslot_handle)
{
    return UNIFY_MPI_KEYSLOT_DESTROY(mpi_keyslot_handle);
}

/* Klad. */
td_s32 ot_mpi_klad_create(td_handle *mpi_klad_handle)
{
    return UNIFY_MPI_KLAD_CREATE(mpi_klad_handle);
}

td_s32 ot_mpi_klad_destroy(td_handle mpi_klad_handle)
{
    return UNIFY_MPI_KLAD_DESTROY(mpi_klad_handle);
}

td_s32 ot_mpi_klad_attach(td_handle mpi_klad_handle, km_klad_dest_type klad_type,
    td_handle mpi_keyslot_handle)
{
    return UNIFY_MPI_KLAD_ATTACH(mpi_klad_handle, (crypto_klad_dest)klad_type, mpi_keyslot_handle);
}

td_s32 ot_mpi_klad_detach(td_handle mpi_klad_handle, km_klad_dest_type klad_type,
    td_handle mpi_keyslot_handle)
{
    return UNIFY_MPI_KLAD_DETACH(mpi_klad_handle, (crypto_klad_dest)klad_type, mpi_keyslot_handle);
}

td_s32 ot_mpi_klad_set_attr(td_handle mpi_klad_handle, const km_klad_attr *attr)
{
    return UNIFY_MPI_SET_ATTR(mpi_klad_handle, (const crypto_klad_attr *)attr);
}

td_s32 ot_mpi_klad_get_attr(td_handle mpi_klad_handle, km_klad_attr *attr)
{
    return UNIFY_MPI_GET_ATTR(mpi_klad_handle, (crypto_klad_attr *)attr);
}

td_s32 ot_mpi_klad_set_session_key(td_handle mpi_klad_handle, const km_klad_session_key *key)
{
#if defined(CONFIG_KM_SESSION_KEY_SUPPORT)
    int ret;
    crypto_klad_session_key tmp_key;
    if (key == NULL || key->key == NULL) {
        return ERROR_PARAM_IS_NULL;
    }
    tmp_key.level = (crypto_klad_level_sel)key->level;
    tmp_key.alg = (crypto_klad_alg_sel)key->alg;
    tmp_key.key_length = key->key_size;
    ret = memcpy_s(tmp_key.key, sizeof(tmp_key.key), key->key, key->key_size);
    crypto_chk_return(ret != EOK, ERROR_MEMCPY_S, "memcpy_s failed\n");

    ret = UNIFY_MPI_SESSION_KEY(mpi_klad_handle, &tmp_key);
    (void)memset_s(&tmp_key, sizeof(crypto_klad_session_key), 0, sizeof(crypto_klad_session_key));

    return ret;
#else
    return ERROR_UNSUPPORT;
#endif
}

td_s32 ot_mpi_klad_set_content_key(td_handle mpi_klad_handle, const km_klad_content_key *key)
{
#if defined(CONFIG_KM_CONTENT_KEY_SUPPORT)
    int ret;
    crypto_klad_content_key tmp_key;
    if (key == NULL || key->key == NULL) {
        return ERROR_PARAM_IS_NULL;
    }
    tmp_key.alg = (crypto_klad_alg_sel)key->alg;
    tmp_key.key_length = key->key_size;
    tmp_key.key_parity = (td_bool)key->key_parity;
    ret = memcpy_s(tmp_key.key, sizeof(tmp_key.key), key->key, key->key_size);
    crypto_chk_return(ret != EOK, ERROR_MEMCPY_S, "memcpy_s failed\n");

    ret = UNIFY_MPI_CONTENT_KEY(mpi_klad_handle, &tmp_key);
    (void)memset_s(&tmp_key, sizeof(crypto_klad_content_key), 0, sizeof(crypto_klad_content_key));

    return ret;
#else
    return ERROR_UNSUPPORT;
#endif
}

td_s32 ot_mpi_klad_set_clear_key(td_handle mpi_klad_handle, const km_klad_clear_key *key)
{
    return UNIFY_MPI_CLEAR_KEY(mpi_klad_handle, (const crypto_klad_clear_key *)key);
}

td_s32 ot_mpi_klad_set_effective_key(td_handle mpi_klad_handle, const km_klad_effective_key *key)
{
    return UNIFY_MPI_EFFECTIVE_KEY(mpi_klad_handle, (const crypto_klad_effective_key *)key);
}