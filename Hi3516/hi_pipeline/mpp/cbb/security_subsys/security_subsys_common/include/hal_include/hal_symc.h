/**
  Copyright (c), 2001-2025, Shenshu Tech. Co., Ltd.
 */
#ifndef HAL_SYMC_H
#define HAL_SYMC_H

#include "crypto_symc_struct.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif /* __cplusplus */
#endif /* __cplusplus */

td_s32 hal_cipher_symc_init(td_void);

td_s32 hal_cipher_symc_deinit(td_void);

td_s32 hal_cipher_symc_low_power_set(td_bool flag);

td_s32 hal_cipher_symc_lock_chn(td_u32 chn_num);

td_s32 hal_cipher_symc_unlock_chn(td_u32 chn_num);

td_s32 hal_cipher_symc_attach(td_u32 symc_chn_num, td_u32 keyslot_chn_num);

td_s32 hal_cipher_symc_set_iv(td_u32 chn_num, const td_u8 *iv, td_u32 iv_len);

td_s32 hal_cipher_symc_get_iv(td_u32 chn_num, td_u8 *iv, td_u32 iv_len);

typedef struct {
    crypto_symc_alg symc_alg;
    crypto_symc_work_mode work_mode;
    crypto_symc_key_length symc_key_length;
    crypto_symc_bit_width symc_bit_width;
    td_bool is_decrypt;
    crypto_symc_iv_change_type iv_change_flag;
} hal_symc_config_t;

td_s32 hal_cipher_symc_set_flag(td_u32 chn_num, td_bool is_decrypt);

td_s32 hal_cipher_symc_config(td_u32 chn_num, const hal_symc_config_t *symc_config);

td_s32 hal_cipher_symc_get_module_info(crypto_symc_module_info *module_info);

td_s32 hal_cipher_symc_get_proc_info(td_u32 chn_num, crypto_symc_proc_info *proc_symc_info);

td_s32 hal_cipher_symc_get_config(td_u32 chn_num, hal_symc_config_t *symc_config);

td_s32 hal_cipher_symc_add_in_node(td_u32 chn_num, td_u64 data_phys_addr, td_u32 data_len,
    in_node_type_e in_node_type);

td_s32 hal_cipher_symc_add_out_node(td_u32 chn_num, td_u64 data_phys_addr, td_u32 data_len);

td_s32 hal_cipher_symc_enable_cenc_node(td_u32 chn_num, td_u32 is_head,
    td_u32 c_num, td_u32 e_num, td_u32 offset_len);

td_s32 hal_cipher_symc_get_tag(td_u32 chn_num, td_u8 *tag, td_u32 tag_len);

td_s32 hal_cipher_symc_start(td_u32 chn_num);

td_s32 hal_cipher_symc_wait_done(td_u32 chn_num, td_bool is_wait);

td_s32 hal_cipher_symc_done_try(td_u32 chn_num);

td_s32 hal_cipher_symc_done_notify(td_u32 chn_num);

td_s32 hal_cipher_symc_register_wait_func(td_u32 chn_num, td_void *wait,
    crypto_wait_timeout_interruptible wait_func, td_u32 timeout_ms);

td_s32 hal_cipher_symc_debug(td_void);

td_s32 hal_cipher_symc_debug_chn(td_u32 chn_num);

td_s32 hal_cipher_symc_set_key(td_u32 chn_num, td_u8 *key, td_u32 key_len);

td_void hal_cipher_set_chn_secure(td_u32 chn_num, td_bool dest_sec, td_bool src_sec);

td_s32 hal_cipher_symc_suspend(td_void);

td_s32 hal_cipher_symc_resume(td_void);

int32_t hal_cipher_symc_cmac_update(uint32_t chn_num, uint64_t data_phys_addr, uint32_t data_len, crypto_cmac_ctx *ctx);

int32_t hal_cipher_symc_cmac_finish(uint32_t chn_num, crypto_cmac_ctx *ctx);

int32_t hal_cipher_symc_cbc_mac_update(uint32_t chn_num, uint64_t data_phys_addr, uint32_t data_len,
    uint8_t *cbc_mac, uint32_t cbc_mac_len);

/* For CCM CMAC. */
int32_t hal_cipher_ccm_cmac_update(uint32_t chn_num, uint64_t data_phys_addr, uint32_t data_len,
    uint8_t *cmac, uint32_t cmac_len);

int32_t hal_cipher_ccm_compute_s0(uint32_t chn_num, const uint8_t *ctr0, uint32_t ctr0_len,
    uint8_t *s0, uint32_t s0_len);

/* For GCM ghash */
int32_t hal_cipher_gcm_compute_j0(uint32_t chn_num, const uint8_t *gcm_iv, uint32_t gcm_iv_len,
    uint8_t *j0, uint32_t j0_len);

int32_t hal_cipher_gcm_ghash_update(uint32_t chn_num, uint64_t data_phys_addr, uint32_t data_len,
    uint8_t *ghash, uint32_t ghash_len);

int32_t hal_cipher_gcm_compute_tag(uint32_t chn_num, const uint8_t *ghash, uint32_t ghash_len,
    const uint8_t *j0, uint32_t j0_len,
    uint8_t *tag, uint32_t tag_len);

#ifdef __cplusplus
#if __cplusplus
}
#endif /* __cplusplus */
#endif /* __cplusplus */

#endif
