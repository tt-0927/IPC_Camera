/**
  Copyright (c), 2001-2025, Shenshu Tech. Co., Ltd.
 */
#ifndef DRV_SYMC_INNER_H
#define DRV_SYMC_INNER_H

#include "crypto_symc_struct.h"

#define SYMC_COMPAT_ERRNO(err_code)     DRV_COMPAT_ERRNO(ERROR_MODULE_SYMC, err_code)
#if defined(CONFIG_DRV_SYMC_PARAM_TRACE_ENABLE)
#define drv_symc_param_trace            crypto_param_trace
#define drv_symc_param_dump_trace       crypto_param_dump_trace
#else
#define drv_symc_param_trace(...)
#define drv_symc_param_dump_trace(...)
#endif

typedef struct {
    td_u8 iv[CRYPTO_IV_LEN_IN_BYTES];
    td_u32 iv_length;
    td_u32 chn_num;
    crypto_symc_alg symc_alg;
    crypto_symc_work_mode work_mode;
    crypto_symc_key_length symc_key_length;
    crypto_symc_bit_width symc_bit_width;
    td_u32 last_pattern_len;
    td_handle keyslot_handle;
    td_u32 iv_change_flag;
#if defined(CONFIG_SYMC_CTR_NON_ALIGN_SUPPORT)
    td_u32 ctr_offset;
    td_u32 ctr_used;
    td_u8 ctr_last_block[CRYPTO_AES_BLOCK_SIZE_IN_BYTES];
#endif
#if defined(CONFIG_SYMC_CCM_SUPPORT) || defined(CONFIG_SYMC_GCM_SUPPORT)
    td_u32 data_len;        /* For CCM/GCM. */
    td_u32 processed_aad_len;
    td_u32 total_aad_len;
    td_u32 processed_len;   /* For CCM/GCM. */
#endif
#if defined(CONFIG_SYMC_GCM_SUPPORT)
    td_u8 ghash_tail_len;
    td_u8 ghash_tail[CRYPTO_AES_BLOCK_SIZE_IN_BYTES];
    td_u8 ghash[CRYPTO_AES_BLOCK_SIZE_IN_BYTES];
    td_u8 j0[CRYPTO_AES_BLOCK_SIZE_IN_BYTES];
#endif
#if defined(CONFIG_SYMC_CCM_SUPPORT)
    td_u32 tag_len;
    td_u8 ccm_cmac[CRYPTO_AES_BLOCK_SIZE_IN_BYTES];
    td_u8 ctr0[CRYPTO_AES_BLOCK_SIZE_IN_BYTES];
    /* CCM CMAC. */
    td_u8 cmac_tail_len;
    td_u8 cmac_tail[CRYPTO_AES_BLOCK_SIZE_IN_BYTES];
#endif
#if defined(CONFIG_SYMC_CBC_MAC_SUPPORT)
    td_u8 mac[CRYPTO_AES_BLOCK_SIZE_IN_BYTES];
    crypto_cmac_ctx cmac_ctx;
#endif
    td_u32 is_decrypt           : 1;
    td_u32 is_create_keyslot    : 1;
    td_u32 is_open              : 1;
    td_u32 is_config            : 1;
    td_u32 is_attached          : 1;
} drv_symc_context_t;

td_s32 inner_drv_symc_set_iv(td_handle symc_handle, const td_u8 *iv, td_u32 iv_len);

drv_symc_context_t *inner_get_symc_ctx(td_handle symc_handle);

td_s32 inner_drv_symc_crypto_chk(td_handle symc_handle, const crypto_buf_attr *src_buf,
    const crypto_buf_attr *dst_buf, td_u32 length);

td_s32 inner_symc_drv_handle_chk(td_handle symc_handle);

int32_t inner_drv_symc_common_process(drv_symc_context_t *symc_ctx, td_u64 src_phys_addr, td_u64 dst_phys_addr,
    td_u32 length, td_u32 node_type);

/* CCM. */
td_s32 drv_cipher_symc_ccm_set_config(drv_symc_context_t *symc_ctx, const crypto_symc_ctrl_t *symc_ctrl);

td_s32 inner_drv_cipher_symc_ccm_process(drv_symc_context_t *symc_ctx, td_ulong src_phys_addr,
    td_ulong dst_phys_addr, td_u32 length);

td_s32 inner_drv_cipher_symc_ccm_get_tag(drv_symc_context_t *symc_ctx, td_u8 *tag, td_u32 tag_length);

int32_t inner_drv_cipher_symc_ccm_cmac_start(drv_symc_context_t *symc_ctx);

int32_t inner_drv_cipher_symc_ccm_cmac_update(drv_symc_context_t *symc_ctx,
    uint64_t data_phys_addr, uint32_t data_len);

int32_t inner_drv_cipher_symc_ccm_cmac_finish(drv_symc_context_t *symc_ctx);

/* GCM. */
td_s32 drv_cipher_symc_gcm_set_config(drv_symc_context_t *symc_ctx, const crypto_symc_ctrl_t *symc_ctrl);

td_s32 inner_drv_cipher_symc_gcm_process(drv_symc_context_t *symc_ctx, td_ulong src_phys_addr,
    td_ulong dst_phys_addr, td_u32 length);

td_s32 inner_drv_cipher_symc_gcm_get_tag(drv_symc_context_t *symc_ctx, td_u8 *tag, td_u32 tag_length);

/* For CTR non align. */
td_s32 inner_drv_cipher_symc_ctr_process_non_align(drv_symc_context_t *symc_ctx,
    td_u64 src_phys_addr, td_u64 dst_phys_addr, td_u32 length);

/* Only for CENC. */
td_void cenc_ddr_unmap_input(td_u32 chn_num);

td_void cenc_ddr_unmap_output(td_u32 chn_num);

td_void cenc_ddr_release(td_void);
#endif