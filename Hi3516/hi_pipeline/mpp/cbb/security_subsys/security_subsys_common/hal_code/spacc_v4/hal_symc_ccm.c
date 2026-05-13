/**
  Copyright (c), 2001-2025, Shenshu Tech. Co., Ltd.
 */
#if defined(CONFIG_SYMC_CCM_SUPPORT)

#include "hal_symc.h"
#include "hal_spacc_reg.h"
#include "hal_spacc_inner.h"

#include "crypto_drv_common.h"

int32_t hal_cipher_ccm_cmac_update(uint32_t chn_num, uint64_t data_phys_addr, uint32_t data_len,
    uint8_t *cmac, uint32_t cmac_len)
{
    int ret;
    in_sym_chn_key_ctrl in_key_ctrl = {0};
    crypto_symc_hard_context *hard_ctx = NULL;

    hard_ctx = inner_hal_get_symc_ctx(chn_num);
    crypto_chk_return(hard_ctx == NULL, CRYPTO_FAILURE, "inner_hal_get_symc_ctx failed\n");

    /* To mac_update, you should choose MODE_CBC_NOOUT. */
    in_key_ctrl.u32 = spacc_reg_read(IN_SYM_CHN_KEY_CTRL(chn_num));
    in_key_ctrl.bits.sym_alg_mode = SYMC_ALG_MODE_CBC_NOOUT_VAL;
    in_key_ctrl.bits.sym_alg_decrypt = TD_FALSE;
    spacc_reg_write(IN_SYM_CHN_KEY_CTRL(chn_num), in_key_ctrl.u32);

    /* You should set iv_mac before mac_update. */
    ret = hal_cipher_symc_set_iv(chn_num, cmac, cmac_len);
    crypto_chk_return(ret != TD_SUCCESS, ret, "hal_cipher_symc_set_iv failed\n");

    ret = hal_cipher_symc_add_in_node(chn_num, data_phys_addr, data_len, IN_NODE_TYPE_FIRST | IN_NODE_TYPE_LAST);
    crypto_chk_return(ret != CRYPTO_SUCCESS, ret, "hal_cipher_symc_add_in_node failed\n");

    hal_cipher_symc_start(chn_num);

    ret = hal_cipher_symc_wait_noout_done(chn_num);
    crypto_chk_return(ret != TD_SUCCESS, ret, "hal_cipher_symc_wait_noout_done failed\n");

    /* You should update iv_mac after mac_update. */
    ret = hal_cipher_symc_get_iv(chn_num, cmac, cmac_len);
    crypto_chk_return(ret != TD_SUCCESS, ret, "hal_cipher_symc_get_iv failed\n");

    return CRYPTO_SUCCESS;
}

int32_t hal_cipher_ccm_compute_s0(uint32_t chn_num, const uint8_t *ctr0, uint32_t ctr0_len,
    uint8_t *s0, uint32_t s0_len)
{
    int ret;
    in_sym_chn_key_ctrl in_key_ctrl = {0};
    crypto_symc_hard_context *hard_ctx = NULL;
    uint8_t *data_buf = NULL;

    data_buf = crypto_malloc_coherent(CRYPTO_AES_BLOCK_SIZE_IN_BYTES, "ccm_s0_buf");
    crypto_chk_return(data_buf == NULL, ERROR_MALLOC, "crypto_malloc_coherent failed\n");

    (void)memset_s(data_buf, CRYPTO_AES_BLOCK_SIZE_IN_BYTES, 0, CRYPTO_AES_BLOCK_SIZE_IN_BYTES);

    hard_ctx = inner_hal_get_symc_ctx(chn_num);
    crypto_chk_goto_with_ret(ret, hard_ctx == NULL, exit_free, CRYPTO_FAILURE, "inner_hal_get_symc_ctx failed\n");

    in_key_ctrl.u32 = spacc_reg_read(IN_SYM_CHN_KEY_CTRL(chn_num));
    in_key_ctrl.bits.sym_alg_mode = SYMC_ALG_MODE_CTR_VAL;
    in_key_ctrl.bits.sym_alg_decrypt = TD_FALSE;
    spacc_reg_write(IN_SYM_CHN_KEY_CTRL(chn_num), in_key_ctrl.u32);

    ret = hal_cipher_symc_set_iv(chn_num, ctr0, ctr0_len);
    crypto_chk_goto(ret != TD_SUCCESS, exit_free, "hal_cipher_symc_set_iv failed\n");

    ret = hal_cipher_symc_add_in_node(chn_num, crypto_get_phys_addr(data_buf), CRYPTO_AES_BLOCK_SIZE_IN_BYTES,
        IN_NODE_TYPE_FIRST | IN_NODE_TYPE_LAST);
    crypto_chk_goto(ret != CRYPTO_SUCCESS, exit_free, "hal_cipher_symc_add_in_node failed\n");

    ret = hal_cipher_symc_add_out_node(chn_num, crypto_get_phys_addr(data_buf), CRYPTO_AES_BLOCK_SIZE_IN_BYTES);
    crypto_chk_goto(ret != CRYPTO_SUCCESS, exit_free, "hal_cipher_symc_add_out_node failed\n");

    hal_cipher_symc_start(chn_num);

    ret = hal_cipher_symc_wait_done(chn_num, TD_FALSE);
    crypto_chk_goto(ret != TD_SUCCESS, exit_free, "hal_cipher_symc_wait_done failed\n");

    ret = memcpy_s(s0, s0_len, data_buf, CRYPTO_AES_BLOCK_SIZE_IN_BYTES);
    crypto_chk_goto_with_ret(ret, ret != EOK, exit_free, ERROR_MEMCPY_S, "memcpy_s failed\n");

    ret = CRYPTO_SUCCESS;
exit_free:
    crypto_free_coherent(data_buf);
    return ret;
}
#endif