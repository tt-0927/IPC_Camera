/**
  Copyright (c), 2001-2025, Shenshu Tech. Co., Ltd.
 */

#ifndef KAPI_SYMC_INNER_H
#define KAPI_SYMC_INNER_H

#include "drv_symc_outer.h"
#include "crypto_symc_struct.h"
#include "crypto_drv_common.h"

typedef struct {
    td_bool is_open;
    td_bool is_mac;
    td_bool is_multi_pack;
    td_u32 tid;
    td_handle drv_symc_handle;
    crypto_symc_attr symc_attr;
    crypto_symc_work_mode work_mode;
    td_handle keyslot_handle;
    td_bool is_attached;
    td_bool is_set_config;
    td_bool is_long_term;
    td_u8 *aad_buf;
    td_u32 aad_buf_size;
    crypto_symc_ctrl_t symc_ctrl;
    crypto_symc_config_aes_ccm_gcm ccm_gcm_config;
    td_u8 tag[CRYPTO_AES_MAX_TAG_SIZE];
    /* The length should be processed. */
    td_u32 data_length;
    /* The length has been processd. */
    td_u32 processed_length;
    /* iv_mac for ccm, iv_ghash for gcm. */
    td_u8 iv_mac[16];
    /* s0 for ccm, j0 for gcm. */
    td_u8 iv0[16];
#if defined(CONFIG_SYMC_CTR_NON_ALIGN_SUPPORT)
    td_u32 ctr_offset;
    td_u8 ctr_last_block[CRYPTO_AES_BLOCK_SIZE_IN_BYTES];
#endif
    drv_symc_ccm_ctx ccm_ctx;
    drv_symc_gcm_ctx gcm_ctx;
    crypto_symc_mac_attr mac_attr;
    crypto_symc_mac_ctx mac_ctx;
    td_u8 *dma_buf;
    td_u32 dma_buf_len;
    crypto_cmac_ctx cmac_ctx;
} crypto_kapi_symc_ctx;

typedef struct {
    crypto_owner owner;
    crypto_kapi_symc_ctx symc_ctx_list[CONFIG_SYMC_VIRT_CHN_NUM];
    crypto_mutex symc_ctx_mutex[CONFIG_SYMC_VIRT_CHN_NUM];
    td_u32 ctx_num;
    td_bool is_used;
    td_u32 init_counter;
    td_u8 *dma_addr;
} crypto_kapi_symc_process;

#define SYMC_COMPAT_ERRNO(err_code)     KAPI_COMPAT_ERRNO(ERROR_MODULE_SYMC, err_code)

/* Common. */
crypto_kapi_symc_ctx *inner_kapi_get_symc_ctx(td_handle symc_handle, td_u32 *idx);

td_s32 inner_get_symc_mutex(crypto_mutex *symc_mutex);

td_s32 inner_get_symc_handle(td_handle *symc_handle);

td_s32 inner_kapi_drv_symc_create_lock(crypto_kapi_symc_ctx *symc_ctx, const crypto_symc_attr *symc_attr);

td_void inner_kapi_drv_symc_destroy_lock(td_handle drv_symc_handle);

void kapi_symc_lock(void);

void kapi_symc_unlock(void);

crypto_kapi_symc_process *inner_get_current_symc_channel(td_void);

crypto_kapi_symc_ctx *priv_occupy_symc_soft_chn(crypto_kapi_symc_process *symc_channel, td_u32 *idx);

td_void priv_release_symc_soft_chn(crypto_kapi_symc_ctx *symc_ctx);

/* Long Term. */
td_s32 inner_kapi_symc_create_long_term(crypto_kapi_symc_ctx *symc_ctx, const crypto_symc_attr *symc_attr);

td_s32 inner_kapi_symc_set_config_long_term(crypto_kapi_symc_ctx *symc_ctx, const crypto_symc_ctrl_t *symc_ctrl);

td_s32 inner_kapi_symc_get_config_long_term(crypto_kapi_symc_ctx *symc_ctx, crypto_symc_ctrl_t *symc_ctrl);

td_s32 inner_kapi_symc_attach_long_term(crypto_kapi_symc_ctx *symc_ctx, td_handle keyslot_handle);

td_s32 inner_kapi_symc_crypto_long_term(crypto_kapi_symc_ctx *symc_ctx, const crypto_buf_attr *src_buf,
    const crypto_buf_attr *dst_buf, td_u32 length, td_u32 crypto_type);

td_s32 inner_kapi_symc_get_tag_long_term(crypto_kapi_symc_ctx *symc_ctx, td_u8 *tag, td_u32 tag_length);

/* Short Term. */
td_s32 inner_kapi_symc_create_short_term(crypto_kapi_symc_ctx *symc_ctx, const crypto_symc_attr *symc_attr);

td_s32 inner_kapi_symc_set_config_short_term(crypto_kapi_symc_ctx *symc_ctx, const crypto_symc_ctrl_t *symc_ctrl);

td_s32 inner_kapi_symc_get_config_short_term(crypto_kapi_symc_ctx *symc_ctx, crypto_symc_ctrl_t *symc_ctrl);

td_s32 inner_kapi_symc_attach_short_term(crypto_kapi_symc_ctx *symc_ctx, td_handle keyslot_handle);

td_s32 inner_kapi_symc_crypto_short_term(crypto_kapi_symc_ctx *symc_ctx,
    const crypto_buf_attr *src_buf, const crypto_buf_attr *dst_buf, td_u32 length, td_u32 crypto_type);

td_s32 inner_kapi_symc_get_tag_short_term(crypto_kapi_symc_ctx *symc_ctx, td_u8 *tag, td_u32 tag_length);

#endif