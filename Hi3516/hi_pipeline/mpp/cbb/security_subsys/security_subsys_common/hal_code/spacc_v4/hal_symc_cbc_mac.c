/**
  Copyright (c), 2001-2025, Shenshu Tech. Co., Ltd.
 */
#include "hal_symc.h"
#include "hal_spacc_reg.h"
#include "hal_spacc_inner.h"

#include "crypto_drv_common.h"

int32_t hal_cipher_symc_cbc_mac_update(uint32_t chn_num, uint64_t data_phys_addr, uint32_t data_len,
    uint8_t *cbc_mac, uint32_t cbc_mac_len)
{
    td_s32 ret;
    crypto_symc_hard_context *hard_ctx = TD_NULL;
    in_sym_chn_key_ctrl in_key_ctrl;

    crypto_hal_func_enter();

#if defined(CONFIG_SYMC_CBC_MAC_TRACE_ENABLE)
    crypto_dump_phys_addr("cbc mac update data", data_phys_addr, data_len);
    crypto_dump_data("cbc mac before", cbc_mac, cbc_mac_len);
#endif
    crypto_chk_return(data_len % CRYPTO_AES_BLOCK_SIZE_IN_BYTES != 0, SYMC_COMPAT_ERRNO(ERROR_SYMC_LEN_NOT_ALIGNED),
        "data_len must be aligned to 16-Byte\n");

    hard_ctx = inner_hal_get_symc_ctx(chn_num);
    crypto_chk_return(hard_ctx == TD_NULL, SYMC_COMPAT_ERRNO(ERROR_INVALID_HANDLE), "invalid chn_num");

    ret = hal_cipher_symc_set_iv(chn_num, cbc_mac, cbc_mac_len);
    crypto_chk_return(ret != TD_SUCCESS, ret, "hal_cipher_symc_set_iv failed\n");

    in_key_ctrl.u32 = spacc_reg_read(IN_SYM_CHN_KEY_CTRL(chn_num));
    in_key_ctrl.bits.sym_alg_sel = SYMC_ALG_AES_VAL;
    in_key_ctrl.bits.sym_alg_mode = SYMC_ALG_MODE_CBC_NOOUT_VAL;
    spacc_reg_write(IN_SYM_CHN_KEY_CTRL(chn_num), in_key_ctrl.u32);

    ret = hal_cipher_symc_add_in_node(chn_num, data_phys_addr, data_len, IN_NODE_TYPE_FIRST | IN_NODE_TYPE_LAST);
    crypto_chk_return(ret != TD_SUCCESS, ret, "inner_hal_priv_set_in_node failed\n");

    ret = hal_cipher_symc_start(chn_num);
    crypto_chk_return(ret != TD_SUCCESS, ret, "hal_cipher_symc_start failed\n");

    ret = hal_cipher_symc_wait_noout_done(chn_num);
    crypto_chk_return(ret != TD_SUCCESS, ret, "hal_cipher_symc_wait_noout_done failed\n");

    ret = hal_cipher_symc_get_iv(chn_num, cbc_mac, cbc_mac_len);
    crypto_chk_return(ret != TD_SUCCESS, ret, "hal_cipher_symc_get_iv failed\n");

#if defined(CONFIG_SYMC_CBC_MAC_TRACE_ENABLE)
    crypto_dump_data("cbc mac after", cbc_mac, cbc_mac_len);
#endif

    crypto_hal_func_exit();
    return TD_SUCCESS;
}
