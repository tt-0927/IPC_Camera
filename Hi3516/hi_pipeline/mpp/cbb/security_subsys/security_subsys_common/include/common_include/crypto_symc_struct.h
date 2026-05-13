/**
  Copyright (c), 2001-2025, Shenshu Tech. Co., Ltd.
 */
#ifndef CRYPTO_SYMC_STRUCT_H
#define CRYPTO_SYMC_STRUCT_H

#include "ot_common_crypto.h"
#include "crypto_common_struct.h"

typedef struct {
    td_u32 clear_header_len;
    td_u32 payload_len;
    td_u32 payload_pattern_encrypt_len;
    td_u32 payload_pattern_clear_len;
    td_u32 payload_pattern_offset_len;
    td_bool iv_change;
    td_u32 iv[CRYPTO_AES_IV_SIZE_IN_WORD];
} crypto_symc_cenc_subsample;

typedef struct {
    td_bool use_odd_key;
    td_u32 first_encrypt_offset;
    crypto_symc_cenc_subsample *subsample;
    td_u32 subsample_num;
    crypto_symc_alg alg;
    crypto_symc_work_mode work_mode;
    td_u32 iv[CRYPTO_AES_IV_SIZE_IN_WORD];
} crypto_symc_cenc_param;

typedef struct {
    td_u32 data_len;        /* For CCM/GCM. */
    td_u32 processed_aad_len;
    td_u32 total_aad_len;
    td_u32 tag_len;
    td_u8 ccm_cmac[CRYPTO_AES_BLOCK_SIZE_IN_BYTES];
    td_u8 ctr0[CRYPTO_AES_BLOCK_SIZE_IN_BYTES];
    /* CCM CMAC. */
    td_u8 cmac_tail_len;
    td_u8 cmac_tail[CRYPTO_AES_BLOCK_SIZE_IN_BYTES];
} crypto_symc_ccm_ctx;

typedef struct {
    td_u32 kslot_handle;
    crypto_symc_ccm_ctx store_ctx;
    td_u32 data_len;        /* Crypto Data Length In Bytes. */
    td_u32 tag_len;         /* Tag Length In Bytes. */
    td_u32 total_aad_len;
    td_u32 processed_aad_len;
    td_u32 processed_data_len;
    td_u8 iv[16];
    td_u32 iv_length;
    td_u8 key[32];
    td_u32 key_len;
    td_s32 is_decrypt; // flag of crypt
    td_u64 phys_addr;
    td_void *virt_addr;
} mbedtls_ccm_context;

typedef struct {
    td_u32 chn_id;                                         /* Channel number */
    td_u32 open;                                           /* Open or closed */
    td_u32 is_decrypt;                                     /* Decrypt or encrypt */
    td_u32 alg;                                            /* Algorithm */
    td_u32 mode;                                           /* Work mode */
    td_u32 key_len;                                        /* Key length */
    td_u32 key_source;                                     /* Hard or soft key */
    td_u32 int_raw;                                        /* Raw intertupt */
    td_u32 int_en;                                         /* Enable interrupt */
    td_u32 int_status;                                     /* Status interrupt */
    td_u32 owner;                                          /* Process PID of owner */
    td_u32 is_secure;                                      /* Secure channel or not */
    td_u32 smmu_enable;                                    /* Smmu enable */
    td_u32 in_node_head;                                   /* In node list head */
    td_u32 in_node_rptr;                                   /* In node list read index */
    td_u32 in_node_wptr;                                   /* In node list write index */
    td_u32 in_node_depth;                                  /* In node depth */
    td_u32 out_node_head;                                  /* Out node list head */
    td_u32 out_node_rptr;                                  /* Out node list read index */
    td_u32 out_node_wptr;                                  /* Out node list write index */
    td_u32 out_node_depth;                                 /* Out node depth */
    td_u8 iv[CRYPTO_AES_IV_SIZE + CRYPTO_AES_IV_SIZE + 1]; /* Out iv */
} crypto_symc_proc_info;

typedef struct {
    td_u32 allow_reset; /* allow reset CPU or not */
    td_u32 sec_cpu;     /* secure CPU or not */
    const char *name;   /* interrupt name */
    td_u32 int_num;     /* interrupt number */
    td_u32 int_en;      /* interrupt enable */
    td_u32 smmu_base;   /* smmu base address */
    td_u32 err_code;    /* error code */
} crypto_symc_module_info;

typedef struct {
    td_u8 key[32];
    td_bool is_odd;
    td_u32 klen;
} crypto_symc_clear_key;

typedef struct {
    uint8_t mac[CRYPTO_AES_IV_SIZE];
    uint32_t mac_len;
    uint8_t left[CRYPTO_AES_IV_SIZE];
    uint32_t left_len;
} crypto_cmac_ctx;

#endif
