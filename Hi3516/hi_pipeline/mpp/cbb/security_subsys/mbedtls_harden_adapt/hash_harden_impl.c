/*
  Copyright (c), 2001-2025, Shenshu Tech. Co., Ltd.
 */
#if defined(CONFIG_HASH_HARDEN_INTERFACE_SUPPORT)
#include "hash_harden_impl.h"
#include "uapi_hash.h"

int mbedtls_alt_hash_start_adapt(mbedtls_alt_hash_clone_ctx *clone_ctx, mbedtls_alt_hash_type hash_type)
{
    return unify_uapi_cipher_hash_start_impl((crypto_hash_clone_ctx *)clone_ctx, (crypto_hash_type)hash_type);
}

int mbedtls_alt_hash_update_adapt(mbedtls_alt_hash_clone_ctx *clone_ctx,
    const unsigned char *data, unsigned int data_len)
{
    return unify_uapi_cipher_hash_update_impl((crypto_hash_clone_ctx *)clone_ctx, (const td_u8 *)data, data_len);
}

int mbedtls_alt_hash_finish_adapt(mbedtls_alt_hash_clone_ctx *clone_ctx, unsigned char *out, unsigned int *out_len)
{
    return unify_uapi_cipher_hash_finish_impl((crypto_hash_clone_ctx *)clone_ctx, (td_u8 *)out, (td_u32 *)out_len);
}
#endif