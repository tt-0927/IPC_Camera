/*
  Copyright (c), 2001-2025, Shenshu Tech. Co., Ltd.
 */

#ifndef DRV_SYMC_OUTER_H
#define DRV_SYMC_OUTER_H

#include "crypto_hash_struct.h"
#include "crypto_symc_struct.h"

typedef struct {
    td_u8 mac[CRYPTO_IV_LEN_IN_BYTES];
    td_u32 mac_length;
    td_u8 tail[CRYPTO_IV_LEN_IN_BYTES];
    td_u32 tail_length;
} crypto_symc_mac_ctx;

td_s32 inner_symc_drv_handle_chk(td_handle symc_handle);

td_s32 inner_drv_get_mac_ctx(td_handle symc_handle, crypto_symc_mac_ctx *mac_ctx);

td_s32 inner_drv_set_mac_ctx(td_handle symc_handle, const crypto_symc_mac_ctx *mac_ctx);

td_s32 inner_symc_cfg_param_check(const crypto_symc_ctrl_t *symc_ctrl);

typedef struct {
    td_u8 *iv0;
    td_u32 iv0_length;
    td_u8 *iv_mac;
    td_u32 iv_mac_length;
    td_u32 data_length;
    td_u32 processed_length;
    td_u32 aad_len;
} drv_symc_ex_context_t;

td_s32 inner_drv_symc_get_iv0(td_handle symc_handle, td_u8 *iv0, td_u32 iv0_length);

td_s32 inner_drv_symc_get_iv_mac(td_handle symc_handle, td_u8 *iv_mac, td_u32 iv_mac_length);

td_s32 inner_drv_symc_ex_restore(td_handle symc_handle, const drv_symc_ex_context_t *symc_ex_ctx);

td_s32 inner_drv_symc_set_ctr_block(td_handle symc_handle, const td_u8 *block, td_u32 block_size, td_u32 ctr_offset);

td_s32 inner_drv_symc_get_ctr_block(td_handle symc_handle, td_u8 *block, td_u32 block_size, td_u32 *ctr_offset);

td_s32 inner_drv_symc_set_iv(td_handle symc_handle, const td_u8 *iv, td_u32 iv_len);

td_s32 inner_drv_symc_get_iv(td_handle symc_handle, td_u8 *iv, td_u32 iv_len);

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
} drv_symc_ccm_ctx;

typedef struct {
    td_u32 data_len;        /* For CCM/GCM. */
    td_u32 processed_aad_len;
    td_u32 total_aad_len;
    td_u32 processed_len;   /* For CCM/GCM. */
    td_u8 ghash_tail_len;
    td_u8 ghash_tail[CRYPTO_AES_BLOCK_SIZE_IN_BYTES];
    td_u8 ghash[CRYPTO_AES_BLOCK_SIZE_IN_BYTES];
    td_u8 j0[CRYPTO_AES_BLOCK_SIZE_IN_BYTES];
} drv_symc_gcm_ctx;

td_s32 inner_drv_symc_set_iv(td_handle symc_handle, const td_u8 *iv, td_u32 iv_len);

td_s32 inner_drv_symc_get_iv(td_handle symc_handle, td_u8 *iv, td_u32 iv_len);

/* CCM. */
td_s32 inner_drv_symc_ccm_set_ctx(td_handle symc_handle, const drv_symc_ccm_ctx *ccm_ctx);

td_s32 inner_drv_symc_ccm_get_ctx(td_handle symc_handle, drv_symc_ccm_ctx *ccm_ctx);

/* GCM. */
td_s32 inner_drv_symc_gcm_set_ctx(td_handle symc_handle, const drv_symc_gcm_ctx *gcm_ctx);

td_s32 inner_drv_symc_gcm_get_ctx(td_handle symc_handle, drv_symc_gcm_ctx *gcm_ctx);

/* CMAC&CBC_MAC. */
int32_t inner_drv_mac_set_ctx(td_handle symc_handle, const crypto_cmac_ctx *mac_ctx);

int32_t inner_drv_mac_get_ctx(td_handle symc_handle, crypto_cmac_ctx *mac_ctx);

/* CBC_MAC. */
td_s32 inner_drv_symc_cbc_mac_set_ctx(td_handle symc_handle, const td_u8 *mac, uint32_t mac_len);
#endif