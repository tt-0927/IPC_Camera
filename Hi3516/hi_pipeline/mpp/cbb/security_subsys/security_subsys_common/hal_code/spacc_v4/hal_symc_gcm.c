/**
  Copyright (c), 2001-2025, Shenshu Tech. Co., Ltd.
 */
#if defined(CONFIG_SYMC_GCM_SUPPORT)

#include "hal_symc.h"
#include "hal_spacc_reg.h"
#include "hal_spacc_inner.h"

#include "crypto_drv_common.h"

int32_t hal_cipher_gcm_ghash_update(uint32_t chn_num, uint64_t data_phys_addr, uint32_t data_len,
    uint8_t *ghash, uint32_t ghash_len)
{
    int32_t ret;
    in_sym_chn_key_ctrl in_key_ctrl = {0};
    crypto_symc_hard_context *hard_ctx = TD_NULL;

    crypto_hal_func_enter();

    hard_ctx = inner_hal_get_symc_ctx(chn_num);
    crypto_chk_return(hard_ctx == NULL, CRYPTO_FAILURE, "inner_hal_get_symc_ctx failed\n");

    /* To gcm_mac_update, you should choose ALG_GHASH. */
    in_key_ctrl.u32 = spacc_reg_read(IN_SYM_CHN_KEY_CTRL(chn_num));
    in_key_ctrl.bits.sym_alg_sel = SYMC_ALG_GHASH_VAL;
    /* symc_alg_mode must be 0 for GHASH, otherwise the calculation will timeout. */
    in_key_ctrl.bits.sym_alg_mode = 0;
    in_key_ctrl.bits.sym_alg_decrypt = TD_FALSE;
    spacc_reg_write(IN_SYM_CHN_KEY_CTRL(chn_num), in_key_ctrl.u32);

    /**
    * You should set iv before mac_update.
    **/
    ret = hal_cipher_symc_set_iv(chn_num, ghash, ghash_len);
    crypto_chk_return(ret != TD_SUCCESS, ret, "hal_cipher_symc_set_iv failed\n");

    ret = hal_cipher_symc_add_in_node(chn_num, data_phys_addr, data_len, IN_NODE_TYPE_FIRST | IN_NODE_TYPE_LAST);
    crypto_chk_return(ret != TD_SUCCESS, ret, "hal_cipher_symc_add_in_node failed\n");

    hal_cipher_symc_start(chn_num);

    /**
     * For last node, you need to wait until the interrupt(SYM_CHANN_RAW_INT) occurs;
     **/
    ret = hal_cipher_symc_wait_noout_done(chn_num);
    crypto_chk_return(ret != TD_SUCCESS, ret, "hal_cipher_symc_wait_noout_done failed\n");

#if defined(CONFIG_SYMC_GCM_TRACE_ENABLE)
    crypto_dump_data("gcm ghash before", ghash, ghash_len);
#endif

    ret = hal_cipher_symc_get_iv(chn_num, ghash, ghash_len);
    crypto_chk_return(ret != TD_SUCCESS, ret, "hal_cipher_symc_get_iv failed\n");

#if defined(CONFIG_SYMC_GCM_TRACE_ENABLE)
    crypto_dump_data("gcm ghash after", ghash, ghash_len);
#endif
    return TD_SUCCESS;
}

/**
 * tag = AES-CTR(iv=j0, data=ghash)
 */
int32_t hal_cipher_gcm_compute_tag(uint32_t chn_num, const uint8_t *ghash, uint32_t ghash_len,
    const uint8_t *j0, uint32_t j0_len,
    uint8_t *tag, uint32_t tag_len)
{
    int ret;
    in_sym_chn_key_ctrl in_key_ctrl = {0};
    crypto_symc_hard_context *hard_ctx = NULL;
    uint8_t *data_buf = NULL;

    crypto_chk_return(tag_len > CRYPTO_AES_BLOCK_SIZE_IN_BYTES, ERROR_INVALID_PARAM, "tag_len is too long\n");

    data_buf = crypto_malloc_coherent(CRYPTO_AES_BLOCK_SIZE_IN_BYTES, "gcm_tag_buf");
    crypto_chk_return(data_buf == NULL, ERROR_MALLOC, "crypto_malloc_coherent failed\n");

    hard_ctx = inner_hal_get_symc_ctx(chn_num);
    crypto_chk_goto_with_ret(ret, hard_ctx == NULL, exit_free, CRYPTO_FAILURE, "inner_hal_get_symc_ctx failed\n");

    in_key_ctrl.u32 = spacc_reg_read(IN_SYM_CHN_KEY_CTRL(chn_num));
    in_key_ctrl.bits.sym_alg_sel = SYMC_ALG_AES_VAL;
    in_key_ctrl.bits.sym_alg_mode = SYMC_ALG_MODE_CTR_VAL;
    in_key_ctrl.bits.sym_alg_decrypt = TD_FALSE;
    spacc_reg_write(IN_SYM_CHN_KEY_CTRL(chn_num), in_key_ctrl.u32);

    ret = memcpy_s(data_buf, CRYPTO_AES_BLOCK_SIZE_IN_BYTES, ghash, ghash_len);
    crypto_chk_goto_with_ret(ret, ret != EOK, exit_free, ERROR_MEMCPY_S, "memcpy_s failed\n");

    ret = hal_cipher_symc_set_iv(chn_num, j0, j0_len);
    crypto_chk_goto(ret != TD_SUCCESS, exit_free, "hal_cipher_symc_set_iv failed\n");

    ret = hal_cipher_symc_add_in_node(chn_num, crypto_get_phys_addr(data_buf), CRYPTO_AES_BLOCK_SIZE_IN_BYTES,
        IN_NODE_TYPE_FIRST | IN_NODE_TYPE_LAST);
    crypto_chk_goto(ret != CRYPTO_SUCCESS, exit_free, "hal_cipher_symc_add_in_node failed\n");

    ret = hal_cipher_symc_add_out_node(chn_num, crypto_get_phys_addr(data_buf), CRYPTO_AES_BLOCK_SIZE_IN_BYTES);
    crypto_chk_goto(ret != CRYPTO_SUCCESS, exit_free, "hal_cipher_symc_add_out_node failed\n");

    hal_cipher_symc_start(chn_num);

    ret = hal_cipher_symc_wait_done(chn_num, TD_FALSE);
    crypto_chk_goto(ret != TD_SUCCESS, exit_free, "hal_cipher_symc_wait_done failed\n");

    ret = memcpy_s(tag, tag_len, data_buf, tag_len);
    crypto_chk_goto_with_ret(ret, ret != EOK, exit_free, ERROR_MEMCPY_S, "memcpy_s failed\n");

    ret = CRYPTO_SUCCESS;
exit_free:
    crypto_free_coherent(data_buf);
    return ret;
}
#endif