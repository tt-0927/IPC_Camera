/*
  Copyright (c), 2001-2025, Shenshu Tech. Co., Ltd.
 */

#ifndef KAPI_INNER_H
#define KAPI_INNER_H

#include "crypto_type.h"
#include "crypto_hash_struct.h"
#include "crypto_drv_common.h"

typedef struct {
    td_u32 tid;
    td_bool is_open;
    td_bool is_long_term;
    td_handle drv_hash_handle;
    crypto_hash_clone_ctx hash_clone_ctx;
    td_u8 *dma_buf;     /* tail(128) + dma_buf. */
    td_u8 tail_len;
    td_u32 dma_buf_len;
} crypto_kapi_hash_ctx;

typedef struct {
    td_u32 pid;
    crypto_owner owner;
    crypto_kapi_hash_ctx hash_ctx_list[CONFIG_HASH_VIRT_CHN_NUM];
    crypto_mutex hash_ctx_mutex[CONFIG_HASH_VIRT_CHN_NUM];
    td_u32 ctx_num;
    td_u32 init_counter;
    td_bool is_used;
} crypto_kapi_hash_process;

#if defined(CONFIG_KAPI_TRNG_SUPPORT)
td_s32 kapi_cipher_trng_env_init(td_void);
td_s32 kapi_cipher_trng_env_deinit(td_void);
#else
#define kapi_cipher_trng_env_init(...)        CRYPTO_SUCCESS
#define kapi_cipher_trng_env_deinit(...)        CRYPTO_SUCCESS
#endif

#if defined(CONFIG_KAPI_HASH_SUPPORT)
td_s32 kapi_cipher_hash_env_init(td_void);
td_s32 kapi_cipher_hash_env_deinit(td_void);
#else
#define kapi_cipher_hash_env_init(...)        CRYPTO_SUCCESS
#define kapi_cipher_hash_env_deinit(...)        CRYPTO_SUCCESS
#endif

#if defined(CONFIG_KAPI_SYMC_SUPPORT)
td_s32 kapi_cipher_symc_env_init(td_void);
td_s32 kapi_cipher_symc_env_deinit(td_void);
#else
#define kapi_cipher_symc_env_init(...)        CRYPTO_SUCCESS
#define kapi_cipher_symc_env_deinit(...)        CRYPTO_SUCCESS
#endif

#if defined(CONFIG_KAPI_PKE_SUPPORT)
td_s32 kapi_cipher_pke_env_init(td_void);
td_s32 kapi_cipher_pke_env_deinit(td_void);
#else
#define kapi_cipher_pke_env_init(...)        CRYPTO_SUCCESS
#define kapi_cipher_pke_env_deinit(...)        CRYPTO_SUCCESS
#endif

#if defined(CONFIG_KAPI_KM_SUPPORT)
td_s32 kapi_km_env_init(td_void);
td_s32 kapi_km_env_deinit(td_void);
#else
#define kapi_km_env_init(...)        CRYPTO_SUCCESS
#define kapi_km_env_deinit(...)        CRYPTO_SUCCESS
#endif

td_s32 kapi_env_init(td_void);

td_void kapi_env_deinit(td_void);

td_void inner_kapi_trng_lock(td_void);

td_void inner_kapi_trng_unlock(td_void);

#endif