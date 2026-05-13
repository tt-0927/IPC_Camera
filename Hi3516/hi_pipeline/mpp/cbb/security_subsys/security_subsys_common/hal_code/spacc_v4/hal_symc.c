/**
  Copyright (c), 2001-2025, Shenshu Tech. Co., Ltd.
 */
#include "hal_symc.h"
#include "hal_spacc_reg.h"
#include "hal_spacc_inner.h"

#include "crypto_drv_common.h"

#ifndef crypto_memory_barrier
#define crypto_memory_barrier()
#endif

#define CRYPTO_SYMC_IN_NODE_SIZE            (sizeof(crypto_symc_entry_in) * CRYPTO_SYMC_MAX_LIST_NUM)
#define CRYPTO_SYMC_OUT_NODE_SIZE           (sizeof(crypto_symc_entry_out) * CRYPTO_SYMC_MAX_LIST_NUM)
#define CRYPTO_SYMC_NODE_SIZE               (CRYPTO_SYMC_IN_NODE_SIZE + CRYPTO_SYMC_OUT_NODE_SIZE)
#define CRYPTO_SYMC_NODE_LIST_SIZE          (CRYPTO_SYMC_NODE_SIZE * CONFIG_SYMC_HARD_CHN_CNT)

#if !defined(CONFIG_MMU_SUPPORT)
static td_u8 g_symc_node_buffer[CRYPTO_SYMC_NODE_LIST_SIZE];
#else
static td_u8 *g_symc_node_buffer = NULL;
#endif

static crypto_symc_hard_context g_symc_hard_context[CONFIG_SYMC_HARD_CHN_CNT];
static td_bool g_hal_symc_initialize = TD_FALSE;


static td_s32 hal_cipher_symc_out_node_done_try(td_u32 chn_num);

static void inner_hal_symc_irq_enable(td_u8 chn_num, td_bool enable)
{
    td_u32 reg_val;

    reg_val = spacc_reg_read(OUT_SYM_CHAN_RAW_LAST_NODE_INT_EN);
    if (enable == TD_TRUE) {
        reg_val |= (1 << chn_num);
    } else {
        reg_val &= ~(1 << chn_num);
    }
    spacc_reg_write(OUT_SYM_CHAN_RAW_LAST_NODE_INT_EN, reg_val);
}

static void inner_symc_int_raw_enable(td_u8 chn_num, td_bool enable)
{
    td_u32 reg_val;

    reg_val = spacc_reg_read(SYM_CHANN_RAW_INT_EN);
    if (enable == TD_TRUE) {
        reg_val |= (1 << chn_num);
    } else {
        reg_val &= ~(1 << chn_num);
    }
    spacc_reg_write(SYM_CHANN_RAW_INT_EN, reg_val);
}

static td_s32 hal_cipher_symc_clear_channel(td_u32 chn_num)
{
    td_s32 ret = TD_SUCCESS;
    td_u32 i;
    td_u32 chn_mask;
    spacc_sym_chn_clear_req symc_chn_clear = {0};
    spacc_int_raw_sym_clear_finish symc_chn_clear_finish = {0};

    chn_mask = 0x1 << chn_num;
    symc_chn_clear.u32 = spacc_reg_read(SPACC_SYM_CHN_CLEAR_REQ);
    symc_chn_clear.bits.sym_chn_clear_req |= chn_mask;
    spacc_reg_write(SPACC_SYM_CHN_CLEAR_REQ, symc_chn_clear.u32);

    for (i = 0; i < CONFIG_SYMC_CLEAR_TIMEOUT_IN_US; i++) {
        symc_chn_clear_finish.u32 = spacc_reg_read(SPACC_INT_RAW_SYM_CLEAR_FINISH);
        symc_chn_clear_finish.bits.int_raw_sym_clear_finish &= chn_mask;
        if (symc_chn_clear_finish.bits.int_raw_sym_clear_finish != 0) {
            spacc_reg_write(SPACC_INT_RAW_SYM_CLEAR_FINISH, symc_chn_clear.u32);
            break;
        }
        crypto_udelay(1);
    }
    if (i >= CONFIG_SYMC_CLEAR_TIMEOUT_IN_US) {
        crypto_log_err("SYMC CHN %u Clear Channel Timeout\n", chn_num);
        return TD_FAILURE;
    }
    inner_symc_int_raw_enable(chn_num, TD_FALSE);

    return ret;
}

#define CRYPTO_SYMC_DFA_ENABLE_VAL          0x1
#define CRYPTO_SYMC_DFA_DISABLE_VAL         0xa
static td_void hal_cipher_symc_dfa_config(td_u32 chn_num, td_bool enable)
{
#if defined(CONFIG_SPACC_DFA_SUPPORT)
    symc_chann_dfa_en dfa_en = { 0 };

    dfa_en.u32 = spacc_reg_read(CHANN_CIPHER_DFA_EN(chn_num));
    if (enable == TD_TRUE) {
        dfa_en.bits.chann_dfa_en = CRYPTO_SYMC_DFA_ENABLE_VAL;
    } else {
        dfa_en.bits.chann_dfa_en = CRYPTO_SYMC_DFA_DISABLE_VAL;
    }
    spacc_reg_write(CHANN_CIPHER_DFA_EN(chn_num), dfa_en.u32);
#else
    crypto_unused(chn_num);
    crypto_unused(enable);
    return;
#endif
}

static td_void hal_cipher_symc_set_entry_node(td_u32 chn_num, crypto_symc_hard_context *hard_ctx)
{
    in_sym_chn_node_length symc_in_node_length;
    out_sym_chn_node_length symc_out_node_length;
    td_u64 entry_in_phy = crypto_get_phys_addr((td_void *)hard_ctx->entry_in);
    td_u64 entry_out_phy = crypto_get_phys_addr((td_void *)hard_ctx->entry_out);

    /* set start addr for cipher in node and node length. */
    spacc_reg_write(IN_SYM_CHN_NODE_START_ADDR_L(chn_num), entry_in_phy);
    spacc_reg_write(IN_SYM_CHN_NODE_START_ADDR_H(chn_num), 0);

    symc_in_node_length.u32 = spacc_reg_read(IN_SYM_CHN_NODE_LENGTH(chn_num));
    symc_in_node_length.bits.sym_chn_node_length = CRYPTO_SYMC_MAX_LIST_NUM;
    spacc_reg_write(IN_SYM_CHN_NODE_LENGTH(chn_num), symc_in_node_length.u32);

    /* set start addr for cipher out node and node length. */
    spacc_reg_write(OUT_SYM_CHN_NODE_START_ADDR_L(chn_num), entry_out_phy);
    spacc_reg_write(OUT_SYM_CHN_NODE_START_ADDR_H(chn_num), 0);

    symc_out_node_length.u32 = spacc_reg_read(OUT_SYM_CHN_NODE_LENGTH(chn_num));
    symc_out_node_length.bits.sym_chn_node_length = CRYPTO_SYMC_MAX_LIST_NUM;
    spacc_reg_write(OUT_SYM_CHN_NODE_LENGTH(chn_num), symc_out_node_length.u32);

    hard_ctx->idx_in = 0;
    hard_ctx->idx_out = 0;
    hard_ctx->cnt_in = 0;
    hard_ctx->cnt_out = 0;
    (td_void)memset_s(hard_ctx->entry_in, CRYPTO_SYMC_IN_NODE_SIZE, 0, CRYPTO_SYMC_IN_NODE_SIZE);
    (td_void)memset_s(hard_ctx->entry_out, CRYPTO_SYMC_OUT_NODE_SIZE, 0, CRYPTO_SYMC_OUT_NODE_SIZE);
}

td_s32 hal_cipher_symc_wait_noout_done(td_u32 chn_num)
{
    td_u32 i = 0;
    td_s32 ret;
    crypto_hal_func_enter();

    for (i = 0; i < CONFIG_SYMC_WAIT_TIMEOUT_IN_US; i++) {
        ret = hal_cipher_symc_out_node_done_try(chn_num);
        if (ret == TD_SUCCESS) {
            break;
        }
        crypto_udelay(1);
    }

    if (i >= CONFIG_SYMC_WAIT_TIMEOUT_IN_US) {
        ret = SYMC_COMPAT_ERRNO(ERROR_SYMC_CALC_TIMEOUT);
    }
    if (ret == SYMC_COMPAT_ERRNO(ERROR_SYMC_CALC_TIMEOUT)) {
        crypto_log_err("symc wait done timeout, chn is %u\n", chn_num);
#if defined(CONFIG_SYMC_HAL_DEBUG_ENABLE)
        hal_cipher_symc_debug();
        hal_cipher_symc_debug_chn(chn_num);
#endif
        return ret;
    }

    crypto_hal_func_exit();
    return TD_SUCCESS;
}

static td_void inner_spacc_interrupt_enable(td_bool enable)
{
    spacc_ie cfg_val = { 0 };
    cfg_val.u32 = spacc_reg_read(SPACC_IE);
    if (crypto_get_cpu_type() == CRYPTO_CPU_TYPE_SCPU) {
        cfg_val.bits.spacc_ie_tee = enable;
    } else {
        cfg_val.bits.spacc_ie_ree = enable;
    }

    spacc_reg_write(SPACC_IE, cfg_val.u32);
}

crypto_symc_hard_context *inner_hal_get_symc_ctx(td_u32 chn_num)
{
    if (chn_num >= CONFIG_SYMC_HARD_CHN_CNT) {
        return TD_NULL;
    }
    return &g_symc_hard_context[chn_num];
}

td_s32 hal_cipher_symc_init(td_void)
{
    td_s32 ret = TD_SUCCESS;
    td_u32 i;
    td_u8 *entry_buffer = TD_NULL;
    crypto_symc_hard_context *hard_ctx = TD_NULL;
    crypto_hal_func_enter();

    if (g_hal_symc_initialize == TD_TRUE) {
        return TD_SUCCESS;
    }
    /* Node Buffer must be MMZ. */
#if defined(CONFIG_MMU_SUPPORT)
    g_symc_node_buffer = crypto_malloc_mmz(CRYPTO_SYMC_NODE_LIST_SIZE, "crypto_symc_node_buffer");
    if (g_symc_node_buffer == TD_NULL) {
        crypto_log_err("crypto_malloc_mmz failed\n");
        return TD_FAILURE;
    }
#endif
    entry_buffer = g_symc_node_buffer;
    (td_void)memset_s(g_symc_hard_context, sizeof(g_symc_hard_context), 0, sizeof(g_symc_hard_context));
    for (i = 0; i < CONFIG_SYMC_HARD_CHN_CNT; i++) {
        hard_ctx = &g_symc_hard_context[i];
        hard_ctx->entry_in = (void *)(entry_buffer + i * CRYPTO_SYMC_NODE_SIZE);
        hard_ctx->entry_out = (void *)(entry_buffer + i * CRYPTO_SYMC_NODE_SIZE +
            CRYPTO_SYMC_IN_NODE_SIZE);
    }

    inner_spacc_interrupt_enable(TD_FALSE);

#if defined(CRYPTO_ERROR_ENV)
    if (CRYPTO_ERROR_ENV != ERROR_ENV_NOOS) {
        /* release all previously locked channels */
        for (i = 0; i < CONFIG_SYMC_HARD_CHN_CNT; i++) {
            (td_void)hal_cipher_symc_unlock_chn(i);
        }
    }
#endif

    g_hal_symc_initialize = TD_TRUE;
    crypto_hal_func_exit();
    return ret;
}

td_s32 hal_cipher_symc_deinit(td_void)
{
    td_u8 *entry_buffer = TD_NULL;
    td_u32 i;
    crypto_hal_func_enter();

    if (g_hal_symc_initialize == TD_FALSE) {
        return TD_SUCCESS;
    }
    entry_buffer = g_symc_node_buffer;
    (td_void)memset_s(entry_buffer, CRYPTO_SYMC_NODE_LIST_SIZE, 0, CRYPTO_SYMC_NODE_LIST_SIZE);
#if defined(CONFIG_MMU_SUPPORT)
    crypto_free_coherent(entry_buffer);
#endif
    (td_void)memset_s(g_symc_hard_context, sizeof(g_symc_hard_context), 0, sizeof(g_symc_hard_context));
    for (i = 0; i < CONFIG_SYMC_HARD_CHN_CNT; i++) {
        hal_cipher_symc_unlock_chn(i);
    }
    g_hal_symc_initialize = TD_FALSE;
    crypto_hal_func_exit();
    return TD_SUCCESS;
}

td_s32 hal_cipher_symc_lock_chn(td_u32 chn_num)
{
    td_s32 ret = TD_SUCCESS;
    td_u32 used;
    td_u32 chnn_who_used = 0;
    spacc_cpu_mask cpu_mask = SPACC_CPU_IDLE;
    crypto_symc_hard_context *hard_ctx = TD_NULL;
    in_sym_chn_ctrl in_ctrl;
    crypto_hal_func_enter();

    if (chn_num == 0) {
        return CRYPTO_FAILURE;
    }

    cpu_mask = SPACC_CPU_CUR;

    used = spacc_reg_read(SPACC_SYM_CHN_LOCK);
    chnn_who_used = CHN_WHO_USED_GET(used, chn_num);
    if (chnn_who_used != SPACC_CPU_IDLE) {
        return TD_FAILURE;
    }
    CHN_WHO_USED_SET(used, chn_num, cpu_mask);
    spacc_reg_write(SPACC_SYM_CHN_LOCK, used);

    /* Read Back. */
    used = spacc_reg_read(SPACC_SYM_CHN_LOCK);
    chnn_who_used = CHN_WHO_USED_GET(used, chn_num);
    crypto_chk_return(chnn_who_used != cpu_mask, TD_FAILURE, "Lock SYMC CHN %u Failed\n", chn_num);

    ret = hal_cipher_symc_clear_channel(chn_num);
    if (ret != TD_SUCCESS) {
        crypto_log_err("hal_cipher_symc_clear_channel failed\n");
        hal_cipher_symc_unlock_chn(chn_num);
        return ret;
    }
    hard_ctx = &g_symc_hard_context[chn_num];
    hal_cipher_symc_set_entry_node(chn_num, hard_ctx);

    /* enable channel. */
    in_ctrl.u32 = spacc_reg_read(IN_SYM_CHN_CTRL(chn_num));
    in_ctrl.bits.sym_chn_en = 1;
    if (crypto_get_cpu_type() == CRYPTO_CPU_TYPE_SCPU) {
        in_ctrl.bits.sym_chn_ss = SYMC_CFG_SECURE;
        in_ctrl.bits.sym_chn_ds = SYMC_CFG_SECURE;
    } else {
        in_ctrl.bits.sym_chn_ss = SYMC_CFG_NON_SECURE;
        in_ctrl.bits.sym_chn_ds = SYMC_CFG_NON_SECURE;
    }
    spacc_reg_write(IN_SYM_CHN_CTRL(chn_num), in_ctrl.u32);

    /* Check whether there are residual interrupts. If yes, clear the interrupts. */
    hal_cipher_symc_done_try(chn_num);
    hal_cipher_symc_out_node_done_try(chn_num);

    /* DFA Enable(Default). */
    hal_cipher_symc_dfa_config(chn_num, TD_TRUE);

    crypto_hal_func_exit();
    return ret;
}

td_s32 hal_cipher_symc_unlock_chn(td_u32 chn_num)
{
    td_s32 ret = TD_SUCCESS;
    td_u32 used;
    td_u32 chnn_who_used = 0;
    spacc_cpu_mask cpu_mask = SPACC_CPU_IDLE;
    crypto_hal_func_enter();

    cpu_mask = SPACC_CPU_CUR;

    used = spacc_reg_read(SPACC_SYM_CHN_LOCK);
    chnn_who_used = CHN_WHO_USED_GET(used, chn_num);
    if (chnn_who_used != cpu_mask) {
        return TD_SUCCESS;
    }

    ret = hal_cipher_symc_clear_channel(chn_num);
    if (ret != TD_SUCCESS) {
        crypto_log_err("hal_cipher_symc_clear_channel failed\n");
    }

    used = spacc_reg_read(SPACC_SYM_CHN_LOCK);
    CHN_WHO_USED_CLR(used, chn_num);
    spacc_reg_write(SPACC_SYM_CHN_LOCK, used);

    crypto_hal_func_exit();
    return ret;
}

td_s32 hal_cipher_symc_attach(td_u32 symc_chn_num, td_u32 keyslot_chn_num)
{
    in_sym_chn_key_ctrl in_key_ctrl;
    crypto_hal_func_enter();

    in_key_ctrl.u32 = spacc_reg_read(IN_SYM_CHN_KEY_CTRL(symc_chn_num));
    /* Keyslot CHN Num. */
    in_key_ctrl.bits.sym_key_chn_id = keyslot_chn_num;
    spacc_reg_write(IN_SYM_CHN_KEY_CTRL(symc_chn_num), in_key_ctrl.u32);

    crypto_hal_func_exit();
    return TD_SUCCESS;
}

td_s32 hal_cipher_symc_set_iv(td_u32 chn_num, const td_u8 *iv, td_u32 iv_len)
{
    td_s32 ret;
    crypto_symc_hard_context *hard_ctx = TD_NULL;
    crypto_symc_entry_in *entry_in = TD_NULL;
    td_u32 idx;
    crypto_hal_func_enter();

    crypto_assert_eq(chn_num < CONFIG_SYMC_HARD_CHN_CNT, TD_TRUE);
    crypto_assert_neq(iv, TD_NULL);
    crypto_assert_eq(iv_len, sizeof(entry_in->iv));

    hard_ctx = &g_symc_hard_context[chn_num];
    idx = hard_ctx->idx_in;
    entry_in = &hard_ctx->entry_in[idx];

    ret = memcpy_s(entry_in->iv, sizeof(entry_in->iv), iv, iv_len);
    if (ret != TD_SUCCESS) {
        crypto_log_err("memcpy_s failed, ret is 0x%x\n", ret);
        return ret;
    }

    crypto_hal_func_exit();
    return TD_SUCCESS;
}

td_s32 hal_cipher_symc_get_iv(td_u32 chn_num, td_u8 *iv, td_u32 iv_len)
{
    td_u32 i;
    td_u32 iv_get[CRYPTO_AES_IV_SIZE_IN_WORD] = {0};
    crypto_hal_func_enter();
    crypto_unused(iv_len);

    crypto_assert_eq(chn_num < CONFIG_SYMC_HARD_CHN_CNT, TD_TRUE);
    crypto_assert_neq(iv, TD_NULL);
    crypto_assert_eq(iv_len, CRYPTO_IV_LEN_IN_BYTES);

    for (i = 0; i < CRYPTO_AES_IV_SIZE_IN_WORD; i++) {
        iv_get[i] = spacc_reg_read(CHANN_CIPHER_IVOUT(chn_num) + i * CRYPTO_WORD_WIDTH);
    }

    (td_void)memcpy_s(iv, CRYPTO_IV_LEN_IN_BYTES, iv_get, CRYPTO_IV_LEN_IN_BYTES);

    crypto_hal_func_exit();
    return TD_SUCCESS;
}

/* 1. Key Length. */
#define SYMC_KEY_64BIT_VAL      0
#define SYMC_KEY_128BIT_VAL     1
#define SYMC_KEY_192BIT_VAL     2
#define SYMC_KEY_256BIT_VAL     3

static const crypto_table_item g_symc_key_length_table[] = {
#if defined(CONFIG_SYMC_64_SUPPORT)
    {
        .index = CRYPTO_SYMC_KEY_64BIT, .value = SYMC_KEY_64BIT_VAL
    },
#endif
    {
        .index = CRYPTO_SYMC_KEY_128BIT, .value = SYMC_KEY_128BIT_VAL
    },
#if defined(CONFIG_SYMC_192_SUPPORT)
    {
        .index = CRYPTO_SYMC_KEY_192BIT, .value = SYMC_KEY_192BIT_VAL
    },
#endif
#if defined(CONFIG_SYMC_256_SUPPORT)
    {
        .index = CRYPTO_SYMC_KEY_256BIT, .value = SYMC_KEY_256BIT_VAL
    },
#endif
};

/* 2. ALg Mode. */
static const crypto_table_item g_symc_alg_mode_table[] = {
#if defined(CONFIG_SYMC_MODE_ECB_SUPPORT)
    {
        .index = CRYPTO_SYMC_WORK_MODE_ECB, .value = SYMC_ALG_MODE_ECB_VAL
    },
#endif
    {
        .index = CRYPTO_SYMC_WORK_MODE_CBC, .value = SYMC_ALG_MODE_CBC_VAL
    },
#if defined(CONFIG_SYMC_MODE_CTR_SUPPORT)
    {
        .index = CRYPTO_SYMC_WORK_MODE_CTR, .value = SYMC_ALG_MODE_CTR_VAL
    },
#endif
#if defined(CONFIG_SYMC_MODE_OFB_SUPPORT)
    {
        .index = CRYPTO_SYMC_WORK_MODE_OFB, .value = SYMC_ALG_MODE_OFB_VAL
    },
#endif
#if defined(CONFIG_SYMC_MODE_CFB_SUPPORT)
    {
        .index = CRYPTO_SYMC_WORK_MODE_CFB, .value = SYMC_ALG_MODE_CFB_VAL
    },
#endif
#if defined(CONFIG_SYMC_MODE_CCM_SUPPORT)
    {
        .index = CRYPTO_SYMC_WORK_MODE_CCM, .value = SYMC_ALG_MODE_CTR_VAL
    },
#endif
#if defined(CONFIG_SYMC_MODE_GCM_SUPPORT)
    {
        .index = CRYPTO_SYMC_WORK_MODE_GCM, .value = SYMC_ALG_MODE_GCTR_VAL
    },
#endif
#if defined(CONFIG_SYMC_MODE_CBC_MAC_SUPPORT)
    {
        .index = CRYPTO_SYMC_WORK_MODE_CBC_MAC, .value = SYMC_ALG_MODE_CBC_NOOUT_VAL
    },
#endif
#if defined(CONFIG_SYMC_MODE_CMAC_SUPPORT)
    {
        .index = CRYPTO_SYMC_WORK_MODE_CMAC, .value = SYMC_ALG_MODE_CMAC_VAL
    }
#endif
};

/* 3. Alg. */
static const crypto_table_item g_symc_alg_table[] = {
    {
        .index = CRYPTO_SYMC_ALG_AES, .value = SYMC_ALG_AES_VAL
    },
#if defined(CONFIG_SYMC_ALG_SM4_SUPPORT)
    {
        .index = CRYPTO_SYMC_ALG_SM4, .value = SYMC_ALG_SM4_VAL
    },
#endif
};

/* 4. Bit Width. */
#define SYMC_ALG_BIT_WIDTH_128BIT   0
#define SYMC_ALG_BIT_WIDTH_64BIT    1
#define SYMC_ALG_BIT_WIDTH_8BIT     1
#define SYMC_ALG_BIT_WIDTH_1BIT     2

static const crypto_table_item g_symc_bit_width_table[] = {
    {
        .index = CRYPTO_SYMC_BIT_WIDTH_128BIT, .value = SYMC_ALG_BIT_WIDTH_128BIT
    },
#if defined(CONFIG_SYMC_BIT_WIDTH_64_SUPPORT)
    {
        .index = CRYPTO_SYMC_BIT_WIDTH_64BIT, .value = SYMC_ALG_BIT_WIDTH_64BIT
    },
#endif
#if defined(CONFIG_SYMC_BIT_WIDTH_8_SUPPORT)
    {
        .index = CRYPTO_SYMC_BIT_WIDTH_8BIT, .value = SYMC_ALG_BIT_WIDTH_8BIT
    },
#endif
#if defined(CONFIG_SYMC_BIT_WIDTH_1_SUPPORT)
    {
        .index = CRYPTO_SYMC_BIT_WIDTH_1BIT, .value = SYMC_ALG_BIT_WIDTH_1BIT
    },
#endif
};

static td_s32 inner_symc_config_key_ctrl(td_u32 chn_num, const hal_symc_config_t *symc_config)
{
    volatile td_s32 ret = TD_FAILURE;
    td_u32 reg_value = 0;
    in_sym_chn_key_ctrl in_key_ctrl = {0};
    td_bool is_decrypt = symc_config->is_decrypt;
    crypto_symc_alg symc_alg = symc_config->symc_alg;
    crypto_symc_work_mode work_mode = symc_config->work_mode;
    crypto_symc_key_length symc_key_length = symc_config->symc_key_length;
    crypto_symc_bit_width symc_bit_width = symc_config->symc_bit_width;

    in_key_ctrl.u32 = spacc_reg_read(IN_SYM_CHN_KEY_CTRL(chn_num));

    /* alg_decrypt. */
    in_key_ctrl.bits.sym_alg_decrypt = is_decrypt;

    /* alg_sel. */
    ret = crypto_get_value_by_index(g_symc_alg_table, crypto_array_size(g_symc_alg_table),
        symc_alg, &reg_value);
    crypto_chk_return(ret != TD_SUCCESS, SYMC_COMPAT_ERRNO(ERROR_INVALID_PARAM), "Invalid Alg!\n");
    in_key_ctrl.bits.sym_alg_sel = reg_value;

    /* alg_mode. */
    ret = crypto_get_value_by_index(g_symc_alg_mode_table, crypto_array_size(g_symc_alg_mode_table),
        work_mode, &reg_value);
    crypto_chk_return(ret != TD_SUCCESS, SYMC_COMPAT_ERRNO(ERROR_INVALID_PARAM), "Invalid Alg_Mode!\n");
    in_key_ctrl.bits.sym_alg_mode = reg_value;

    /* alg_key_len. */
    ret = crypto_get_value_by_index(g_symc_key_length_table, crypto_array_size(g_symc_key_length_table),
        symc_key_length, &reg_value);
    crypto_chk_return(ret != TD_SUCCESS, SYMC_COMPAT_ERRNO(ERROR_INVALID_PARAM), "Invalid key_len!\n");
    in_key_ctrl.bits.sym_alg_key_len = reg_value;

    /* alg_data_width. */
    ret = crypto_get_value_by_index(g_symc_bit_width_table, crypto_array_size(g_symc_bit_width_table),
        symc_bit_width, &reg_value);
    crypto_chk_return(ret != TD_SUCCESS, SYMC_COMPAT_ERRNO(ERROR_INVALID_PARAM), "Invalid alg_data_width!\n");
    in_key_ctrl.bits.sym_alg_data_width = reg_value;

    spacc_reg_write(IN_SYM_CHN_KEY_CTRL(chn_num), in_key_ctrl.u32);
    return TD_SUCCESS;
}

td_s32 hal_cipher_symc_set_flag(td_u32 chn_num, td_bool is_decrypt)
{
    in_sym_chn_key_ctrl in_key_ctrl = {0};
    crypto_hal_func_enter();

    crypto_assert_eq(chn_num < CONFIG_SYMC_HARD_CHN_CNT, TD_TRUE);

    in_key_ctrl.u32 = spacc_reg_read(IN_SYM_CHN_KEY_CTRL(chn_num));

    in_key_ctrl.bits.sym_alg_decrypt = is_decrypt;

    spacc_reg_write(IN_SYM_CHN_KEY_CTRL(chn_num), in_key_ctrl.u32);
    
    crypto_hal_func_exit();
    return CRYPTO_SUCCESS;
}

td_s32 hal_cipher_symc_config(td_u32 chn_num, const hal_symc_config_t *symc_config)
{
    td_s32 ret;
    in_sym_out_ctrl cipher_dma_ctrl = { 0 };
    crypto_symc_alg symc_alg = symc_config->symc_alg;
    crypto_symc_hard_context *hard_ctx = TD_NULL;

    crypto_hal_func_enter();
    crypto_assert_eq(chn_num < CONFIG_SYMC_HARD_CHN_CNT, TD_TRUE);
    crypto_assert_neq(symc_config, TD_NULL);

    /* dma enable. */
    cipher_dma_ctrl.u32 = spacc_reg_read(IN_SYM_OUT_CTRL(chn_num));
    if (symc_alg == CRYPTO_SYMC_ALG_DMA) {
        cipher_dma_ctrl.bits.sym_dma_copy = TD_TRUE;
        spacc_reg_write(IN_SYM_OUT_CTRL(chn_num), cipher_dma_ctrl.u32);
        return TD_SUCCESS;
    }
    cipher_dma_ctrl.bits.sym_dma_copy = TD_FALSE;
    spacc_reg_write(IN_SYM_OUT_CTRL(chn_num), cipher_dma_ctrl.u32);

    ret = inner_symc_config_key_ctrl(chn_num, symc_config);
    crypto_chk_return(ret != TD_SUCCESS, SYMC_COMPAT_ERRNO(ERROR_INVALID_PARAM), "inner_symc_config_key_ctrl failed\n");

    hard_ctx = &g_symc_hard_context[chn_num];

    (td_void)memcpy_s(&(hard_ctx->symc_config), sizeof(hal_symc_config_t), symc_config, sizeof(hal_symc_config_t));

    crypto_hal_func_exit();

    return ret;
}

td_s32 hal_cipher_symc_get_config(td_u32 chn_num, hal_symc_config_t *symc_config)
{
    td_s32 ret;
    crypto_symc_hard_context *hard_ctx = TD_NULL;

    crypto_hal_func_enter();
    crypto_assert_eq(chn_num < CONFIG_SYMC_HARD_CHN_CNT, TD_TRUE);
    crypto_assert_neq(symc_config, TD_NULL);

    hard_ctx = &g_symc_hard_context[chn_num];

    ret = memcpy_s(symc_config, sizeof(hal_symc_config_t), &hard_ctx->symc_config, sizeof(hal_symc_config_t));
    crypto_chk_return(ret != TD_SUCCESS, ret, "memcpy_s failed\n");

    crypto_hal_func_exit();
    return TD_SUCCESS;
}

td_s32 hal_cipher_symc_start(td_u32 chn_num)
{
    in_sym_chn_node_wr_point in_node_wr_ptr;
    out_sym_chn_node_wr_point out_node_wr_ptr;
    crypto_symc_hard_context *hard_ctx = TD_NULL;
    td_u32 ptr;

    crypto_hal_func_enter();
    crypto_assert_eq(chn_num < CONFIG_SYMC_HARD_CHN_CNT, TD_TRUE);

    hard_ctx = &g_symc_hard_context[chn_num];

    /* if wait_func registered, enable interrupt */
    if (hard_ctx->wait_func != TD_NULL) {
        hard_ctx->done = TD_FALSE;
        hard_ctx->is_wait = TD_TRUE;
        inner_hal_symc_irq_enable(chn_num, TD_TRUE);
    } else {
        hard_ctx->is_wait = TD_FALSE;
        inner_hal_symc_irq_enable(chn_num, TD_FALSE);
    }

    /* configure out nodes. */
    out_node_wr_ptr.u32 = spacc_reg_read(OUT_SYM_CHN_NODE_WR_POINT(chn_num));
    ptr = out_node_wr_ptr.bits.sym_chn_node_wr_point + hard_ctx->cnt_out;
    out_node_wr_ptr.bits.sym_chn_node_wr_point = ptr % CRYPTO_SYMC_MAX_LIST_NUM;

    /* make sure all the above explicit memory accesses and instructions are completed
     * before start the hardware.
     */
    crypto_memory_barrier();

    spacc_reg_write(OUT_SYM_CHN_NODE_WR_POINT(chn_num), out_node_wr_ptr.u32);

    /* configure in nodes. */
    in_node_wr_ptr.u32 = spacc_reg_read(IN_SYM_CHN_NODE_WR_POINT(chn_num));

    ptr = in_node_wr_ptr.bits.sym_chn_node_wr_point + hard_ctx->cnt_in;
    in_node_wr_ptr.bits.sym_chn_node_wr_point = ptr % CRYPTO_SYMC_MAX_LIST_NUM;

    /* make sure all the above explicit memory accesses and instructions are completed
     * before start the hardware.
     */
    crypto_memory_barrier();

    crypto_cache_flush((uintptr_t)hard_ctx->entry_in, CRYPTO_SYMC_IN_NODE_SIZE);
    crypto_cache_flush((uintptr_t)hard_ctx->entry_out, CRYPTO_SYMC_OUT_NODE_SIZE);

    crypto_cache_all();
    spacc_reg_write(IN_SYM_CHN_NODE_WR_POINT(chn_num), in_node_wr_ptr.u32);

    hard_ctx->cnt_in = 0;
    hard_ctx->cnt_out = 0;

    crypto_hal_func_exit();
    return TD_SUCCESS;
}

td_s32 hal_cipher_symc_done_try(td_u32 chn_num)
{
    out_sym_chan_raw_int last_raw;

    td_u32 mask;

    last_raw.u32 = spacc_reg_read(OUT_SYM_CHAN_RAW_LAST_NODE_INT);
    last_raw.bits.out_sym_chan_raw_int &= (0x01 << chn_num);
    spacc_reg_write(OUT_SYM_CHAN_RAW_LAST_NODE_INT, last_raw.u32);

    mask = last_raw.bits.out_sym_chan_raw_int;
    if (mask == 0) {
        return CRYPTO_FAILURE;
    }

    return CRYPTO_SUCCESS;
}

#if defined(CONFIG_SPACC_IRQ_ENABLE)
td_s32 hal_cipher_symc_done_notify(td_u32 chn_num)
{
    crypto_symc_hard_context *hard_ctx = TD_NULL;
    crypto_hal_func_enter();
    crypto_assert_eq(chn_num < CONFIG_SYMC_HARD_CHN_CNT, TD_TRUE);

    hard_ctx = &g_symc_hard_context[chn_num];
    hard_ctx->done = TD_TRUE;

    crypto_hal_func_exit();
    return TD_SUCCESS;
}
#endif

static td_s32 hal_cipher_symc_out_node_done_try(td_u32 chn_num)
{
    sym_chann_raw_int raw_int;
    td_u32 mask;
    crypto_hal_func_enter();

    raw_int.u32 = spacc_reg_read(SYM_CHANN_RAW_INT);
    raw_int.bits.sym_chann_raw_int &= (0x01 << chn_num);
    spacc_reg_write(SYM_CHANN_RAW_INT, raw_int.u32);

    mask = raw_int.bits.sym_chann_raw_int;
    if (mask == 0) {
        return TD_FAILURE;
    }

    crypto_hal_func_exit();
    return TD_SUCCESS;
}

#if defined(CONFIG_SPACC_IRQ_ENABLE)
static td_bool hal_symc_condition(const td_void *param)
{
    crypto_symc_hard_context *hard_ctx = TD_NULL;
    td_u32 chn_num = *(td_u32 *)param;

    hard_ctx = &g_symc_hard_context[chn_num];
    if (hard_ctx->done == TD_TRUE) {
        hard_ctx->done = TD_FALSE;
        return TD_TRUE;
    } else {
        return TD_FALSE;
    }
}

td_s32 hal_wait_in_node_done(td_u32 chn_num)
{
    td_u32 i;

    in_sym_chn_node_wr_point in_node_wr_ptr = {0};
    in_sym_chn_node_rd_point in_node_rd_ptr = {0};

    for (i = 0; i < CONFIG_SYMC_WAIT_TIMEOUT_IN_US; i++) {
        in_node_wr_ptr.u32 = spacc_reg_read(IN_SYM_CHN_NODE_WR_POINT(chn_num));
        in_node_rd_ptr.u32 = spacc_reg_read(IN_SYM_CHN_NODE_RD_POINT(chn_num));
        if (in_node_rd_ptr.bits.sym_chn_node_rd_point == in_node_wr_ptr.bits.sym_chn_node_wr_point) {
            break;
        }
        crypto_udelay(1);
    }

    if (i >= CONFIG_SYMC_WAIT_TIMEOUT_IN_US) {
        crypto_log_err("hal_wait_in_node_done Timeout!\n");
        return TD_FAILURE;
    }

    return TD_SUCCESS;
}
#endif

static td_s32 symc_wait_done(td_u32 chn_num, td_bool is_wait, crypto_symc_hard_context *hard_ctx)
{
    td_s32 ret = CRYPTO_SUCCESS;
    td_u32 i = 0;
    (void)(is_wait);
    if ((hard_ctx->is_wait == TD_TRUE) && (hard_ctx->wait_func != TD_NULL)) {
#if defined(CONFIG_SPACC_IRQ_ENABLE)
        ret = hard_ctx->wait_func(hard_ctx->wait, hal_symc_condition, (td_void *)(&chn_num), hard_ctx->timeout_ms);
        if (ret <= 0) {
            crypto_log_err("wait_func Timeout!\n");
            ret = SYMC_COMPAT_ERRNO(ERROR_SYMC_CALC_TIMEOUT);
        }
#endif
    } else {
        for (i = 0; i < CONFIG_SYMC_WAIT_TIMEOUT_IN_US; i++) {
            ret = hal_cipher_symc_done_try(chn_num);
            if (ret == TD_SUCCESS) {
                break;
            }
            crypto_udelay(1);
        }
        if (i >= CONFIG_SYMC_WAIT_TIMEOUT_IN_US) {
            ret = SYMC_COMPAT_ERRNO(ERROR_SYMC_CALC_TIMEOUT);
        }
    }

    return ret;
}

td_s32 hal_cipher_symc_wait_done(td_u32 chn_num, td_bool is_wait)
{
    td_s32 ret;
    crypto_symc_hard_context *hard_ctx = TD_NULL;
    crypto_hal_func_enter();

    hard_ctx = &g_symc_hard_context[chn_num];

    ret = symc_wait_done(chn_num, is_wait, hard_ctx);
    inner_hal_symc_irq_enable(chn_num, TD_FALSE);
    if (ret == SYMC_COMPAT_ERRNO(ERROR_SYMC_CALC_TIMEOUT)) {
        crypto_log_err("symc wait done timeout, chn is %u\n", chn_num);
#if defined(CONFIG_SYMC_HAL_DEBUG_ENABLE)
        hal_cipher_symc_debug();
        hal_cipher_symc_debug_chn(chn_num);
#endif
        return ret;
    }

    crypto_hal_func_exit();
    return TD_SUCCESS;
}

td_s32 hal_cipher_symc_add_in_node(td_u32 chn_num, td_u64 data_phys_addr, td_u32 data_len,
    in_node_type_e in_node_type)
{
    crypto_symc_hard_context *hard_ctx = TD_NULL;
    crypto_symc_entry_in *entry_in = TD_NULL;

    crypto_hal_func_enter();
    crypto_chk_return(data_phys_addr == 0, SYMC_COMPAT_ERRNO(ERROR_INVALID_PHYS_ADDR), "data_phys_addr is invalid\n");
    crypto_chk_return(data_phys_addr % CONFIG_SPACC_ADDR_ALIGN_LEN != 0, SYMC_COMPAT_ERRNO(ERROR_SYMC_ADDR_NOT_ALIGNED),
        "phys_addr should align to %u-Byte\n", CONFIG_SPACC_ADDR_ALIGN_LEN);

    hard_ctx = &g_symc_hard_context[chn_num];
    entry_in = &hard_ctx->entry_in[hard_ctx->idx_in];

    crypto_cache_flush(data_phys_addr, data_len);
    entry_in->sym_first_node = (in_node_type & IN_NODE_TYPE_FIRST) ? 0x1 : 0;
    entry_in->sym_last_node = (in_node_type & IN_NODE_TYPE_LAST) ? 0x1 : 0;
    entry_in->sym_alg_length = data_len;
    entry_in->sym_start_addr = data_phys_addr;
    entry_in->sym_start_high = 0;

    hard_ctx->idx_in++;
    hard_ctx->cnt_in++;
    hard_ctx->idx_in %= CRYPTO_SYMC_MAX_LIST_NUM;

    crypto_hal_func_exit();
    return TD_SUCCESS;
}

td_s32 hal_cipher_symc_add_out_node(td_u32 chn_num, td_u64 data_phys_addr, td_u32 data_len)
{
    crypto_symc_hard_context *hard_ctx = TD_NULL;
    crypto_symc_entry_out *entry_out = TD_NULL;
    td_u32 idx_out;
    crypto_hal_func_enter();
    crypto_chk_return(data_phys_addr == 0, SYMC_COMPAT_ERRNO(ERROR_INVALID_PHYS_ADDR), "data_phys_addr is invalid\n");
    crypto_chk_return(data_phys_addr % CONFIG_SPACC_ADDR_ALIGN_LEN != 0, SYMC_COMPAT_ERRNO(ERROR_SYMC_ADDR_NOT_ALIGNED),
        "phys_addr should align to %u-Byte\n", CONFIG_SPACC_ADDR_ALIGN_LEN);

    hard_ctx = &g_symc_hard_context[chn_num];
    idx_out = hard_ctx->idx_out;
    if (hard_ctx->entry_out == TD_NULL) {
        crypto_log_err("hard_ctx->entry_out is NULL\n");
        return TD_FAILURE;
    }
    entry_out = &hard_ctx->entry_out[idx_out];

    crypto_cache_flush(data_phys_addr, data_len);
    entry_out->sym_alg_length = data_len;
    entry_out->sym_start_addr = data_phys_addr;
    entry_out->sym_start_high = 0;

    hard_ctx->idx_out++;
    hard_ctx->cnt_out++;
    hard_ctx->idx_out %= CRYPTO_SYMC_MAX_LIST_NUM;

    crypto_hal_func_exit();
    return TD_SUCCESS;
}

#if defined(CONFIG_SPACC_IRQ_ENABLE)
td_s32 hal_cipher_symc_register_wait_func(td_u32 chn_num, td_void *wait,
    crypto_wait_timeout_interruptible wait_func, td_u32 timeout_ms)
{
    crypto_symc_hard_context *hard_ctx = TD_NULL;
    crypto_hal_func_enter();

    hard_ctx = &g_symc_hard_context[chn_num];
    hard_ctx->wait = wait;
    hard_ctx->wait_func = wait_func;
    hard_ctx->timeout_ms = timeout_ms;

    if (wait_func != TD_NULL) {
        inner_spacc_interrupt_enable(TD_TRUE);
    }
    crypto_hal_func_exit();
    return TD_SUCCESS;
}
#endif

#if defined(CONFIG_SYMC_HAL_DEBUG_ENABLE)
td_s32 hal_cipher_symc_debug(td_void)
{
    crypto_print("CRYPTO_SYMC_IN_NODE_SIZE is 0x%x\n", (unsigned int)CRYPTO_SYMC_IN_NODE_SIZE);
    crypto_print("CRYPTO_SYMC_OUT_NODE_SIZE is 0x%x\n", (unsigned int)CRYPTO_SYMC_OUT_NODE_SIZE);
    crypto_print("CRYPTO_SYMC_NODE_SIZE is 0x%x\n", (unsigned int)CRYPTO_SYMC_NODE_SIZE);
    crypto_print("CRYPTO_SYMC_NODE_LIST_SIZE is 0x%x\n", (unsigned int)CRYPTO_SYMC_NODE_LIST_SIZE);
    crypto_print("SPACC_IE is 0x%x\n", spacc_reg_read(SPACC_IE));
    crypto_print("SPACC_SYMC_CHN_LOCK is 0x%x\n", spacc_reg_read(SPACC_SYM_CHN_LOCK));
    crypto_print("OUT_SYM_CHAN_RAW_LAST_NODE_INT is 0x%x\n", spacc_reg_read(OUT_SYM_CHAN_RAW_LAST_NODE_INT));
    crypto_print("OUT_SYM_CHAN_RAW_LAST_NODE_INT_EN is 0x%x\n", spacc_reg_read(OUT_SYM_CHAN_RAW_LAST_NODE_INT_EN));
    crypto_print("OUT_SYM_CHAN_RAW_LEVEL_INT is 0x%x\n", spacc_reg_read(OUT_SYM_CHAN_RAW_LEVEL_INT));
    crypto_print("SYM_CHANN_RAW_INT is 0x%x\n", spacc_reg_read(SYM_CHANN_RAW_INT));
    crypto_print("SYM_CHANN_RAW_INT_EN is 0x%x\n", spacc_reg_read(SYM_CHANN_RAW_INT_EN));
    crypto_print("TEE_SYM_CALC_CTRL_CHECK_ERR is 0x%x\n", spacc_reg_read(TEE_SYM_CALC_CTRL_CHECK_ERR));
    crypto_print("TEE_SYM_CALC_CTRL_CHECK_ERR_STATUS is 0x%x\n", spacc_reg_read(TEE_SYM_CALC_CTRL_CHECK_ERR_STATUS));
    crypto_print("REE_SYM_CALC_CTRL_CHECK_ERR is 0x%x\n", spacc_reg_read(REE_SYM_CALC_CTRL_CHECK_ERR));
    crypto_print("REE_SYM_CALC_CTRL_CHECK_ERR_STATUS is 0x%x\n", spacc_reg_read(REE_SYM_CALC_CTRL_CHECK_ERR_STATUS));

    return TD_SUCCESS;
}

static td_void hal_cipher_symc_print_entry_in(const crypto_symc_entry_in *entry_in)
{
    crypto_unused(entry_in);
    crypto_print("The Details of Entry In:\n");
    crypto_print("sym_first_node is 0x%x\n", entry_in->sym_first_node);
    crypto_print("sym_last_node is 0x%x\n", entry_in->sym_last_node);
    crypto_print("odd_even is 0x%x\n", entry_in->odd_even);
    crypto_print("sym_alg_length is 0x%x\n", entry_in->sym_alg_length);
    crypto_print("sym_start_addr is 0x%x\n", entry_in->sym_start_addr);
    crypto_print("sym_start_high is 0x%x\n", entry_in->sym_start_high);
    crypto_dump_data("iv", (td_u8 *)entry_in->iv, sizeof(entry_in->iv));
}

static td_void hal_cipher_symc_print_entry_out(const crypto_symc_entry_out *entry_in)
{
    crypto_unused(entry_in);
    crypto_print("The Details of Entry Out:\n");
    crypto_print("sym_alg_length is 0x%x\n", entry_in->sym_alg_length);
    crypto_print("sym_start_addr is 0x%x\n", entry_in->sym_start_addr);
    crypto_print("sym_start_high is 0x%x\n", entry_in->sym_start_high);
}

td_s32 hal_cipher_symc_debug_chn(td_u32 chn_num)
{
    crypto_symc_hard_context *hard_ctx = TD_NULL;
    crypto_symc_entry_in *entry_in = TD_NULL;
    crypto_symc_entry_out *entry_out = TD_NULL;
    td_u32 i;
    td_u8 iv[CRYPTO_IV_LEN_IN_BYTES] = {0};

    hard_ctx = &g_symc_hard_context[chn_num];
    entry_in = hard_ctx->entry_in;
    entry_out = hard_ctx->entry_out;

    crypto_print("The Status of SYMC CHN %u:\n", chn_num);
    crypto_print("IN_SYM_CHN_NODE_START_ADDR_L is 0x%x\n", spacc_reg_read(IN_SYM_CHN_NODE_START_ADDR_L(chn_num)));
    crypto_print("IN_SYM_CHN_NODE_START_ADDR_H is 0x%x\n", spacc_reg_read(IN_SYM_CHN_NODE_START_ADDR_H(chn_num)));
    crypto_print("IN_SYM_CHN_NODE_LENGTH is 0x%x\n", spacc_reg_read(IN_SYM_CHN_NODE_LENGTH(chn_num)));

    crypto_print("OUT_SYM_CHN_NODE_START_ADDR_L is 0x%x\n", spacc_reg_read(OUT_SYM_CHN_NODE_START_ADDR_L(chn_num)));
    crypto_print("OUT_SYM_CHN_NODE_START_ADDR_H is 0x%x\n", spacc_reg_read(OUT_SYM_CHN_NODE_START_ADDR_H(chn_num)));
    crypto_print("OUT_SYM_CHN_NODE_LENGTH is 0x%x\n", spacc_reg_read(OUT_SYM_CHN_NODE_LENGTH(chn_num)));

    crypto_print("IN_SYM_CHN_CTRL is 0x%x\n", spacc_reg_read(IN_SYM_CHN_CTRL(chn_num)));
    crypto_print("IN_SYM_OUT_CTRL is 0x%x\n", spacc_reg_read(IN_SYM_OUT_CTRL(chn_num)));
    crypto_print("IN_SYM_CHN_KEY_CTRL is 0x%x\n", spacc_reg_read(IN_SYM_CHN_KEY_CTRL(chn_num)));

    crypto_print("OUT_SYM_CHN_NODE_WR_POINT is 0x%x\n", spacc_reg_read(OUT_SYM_CHN_NODE_WR_POINT(chn_num)));
    crypto_print("OUT_SYM_CHN_NODE_RD_POINT is 0x%x\n", spacc_reg_read(OUT_SYM_CHN_NODE_RD_POINT(chn_num)));

    crypto_print("IN_SYM_CHN_NODE_WR_POINT is 0x%x\n", spacc_reg_read(IN_SYM_CHN_NODE_WR_POINT(chn_num)));
    crypto_print("IN_SYM_CHN_NODE_RD_POINT is 0x%x\n", spacc_reg_read(IN_SYM_CHN_NODE_RD_POINT(chn_num)));

    for (i = 0; i < CRYPTO_SYMC_MAX_LIST_NUM; i++) {
        crypto_print("IDX_IN %u:\n", i);
        hal_cipher_symc_print_entry_in(&entry_in[i]);
    }
    for (i = 0; i < CRYPTO_SYMC_MAX_LIST_NUM; i++) {
        crypto_print("IDX_OUT %u:\n", i);
        hal_cipher_symc_print_entry_out(&entry_out[i]);
    }

    hal_cipher_symc_get_iv(chn_num, iv, sizeof(iv));
    crypto_dump_data("Current IV", iv, sizeof(iv));

    return TD_SUCCESS;
}
#endif


td_void hal_cipher_set_chn_secure(td_u32 chn_num, td_bool dest_sec, td_bool src_sec)
{
    crypto_unused(chn_num);
    crypto_unused(dest_sec);
    crypto_unused(src_sec);

    return;
}

#if defined(CONFIG_SYMC_SUSPEND_SUPPORT)
td_s32 hal_cipher_symc_resume(td_void)
{
    return TD_SUCCESS;
}
 
td_s32 hal_cipher_symc_suspend(td_void)
{
    return TD_SUCCESS;
}
#endif