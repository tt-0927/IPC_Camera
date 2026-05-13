/**
  Copyright (c), 2001-2025, Shenshu Tech. Co., Ltd.
 */

#include "hal_hash.h"

#include "crypto_drv_common.h"
#include "hal_spacc_reg.h"

#ifndef crypto_memory_barrier
#define crypto_memory_barrier()
#endif

#define HASH_COMPAT_ERRNO(err_code)         HAL_COMPAT_ERRNO(ERROR_MODULE_HASH, err_code)

typedef struct {
    td_u32 hash_first_node : 1;
    td_u32 hash_last_node : 1;
    td_u32 reserved_0 : 30;       // reserve
    td_u32 hash_alg_length;       // hash message length
    td_u32 hash_start_addr_l;     // hash message address
    td_u32 hash_start_addr_h : 4; // hash message address
} hash_entry_in;

typedef struct {
    hash_entry_in *entry_in;    /* ! spacc digest in entry struct */
    td_u32 idx_in;              /* !< current hash nodes to be used */
    td_u32 idx_cnt;             /* !< total hash nodes */
    td_bool is_last;
    td_bool is_locked;
    td_u32 block_size;
#if defined(CONFIG_HASH_WAIT_FUNCTION_SUPPORT)
    td_void *wait;
    crypto_wait_timeout_interruptible wait_func;
    td_bool done;
    td_bool is_wait;
    td_u32 timeout_ms;
#endif
} hal_hash_hard_chn_ctx;

typedef enum {
    HASH_CHN_ALG_SEL_SHA1   = 0xa,
    HASH_CHN_ALG_SEL_SHA2   = 0xb,
    HASH_CHN_ALG_SEL_SM3    = 0xc,
} hash_chn_alg_sel_e;

typedef enum {
    HASH_CHN_ALG_MODE_224   = 0x0,
    HASH_CHN_ALG_MODE_256   = 0x1,
    HASH_CHN_ALG_MODE_384   = 0x2,
    HASH_CHN_ALG_MODE_512   = 0x3,
} hash_chn_alg_mode_e;

#define CONFIG_HASH_NODE_DEPTH              2
#define HASH_NODE_SIZE              (sizeof(hash_entry_in) * CONFIG_HASH_NODE_DEPTH)

#if !defined(CONFIG_MMU_SUPPORT)
static td_u8 g_node_buffer[HASH_NODE_SIZE * CONFIG_HASH_HARD_CHN_CNT];
#else
static td_u8 *g_node_buffer = TD_NULL;
#endif

static hal_hash_hard_chn_ctx g_hash_hard_ctx[CONFIG_HASH_HARD_CHN_CNT];
static td_bool g_hal_hash_initialize = TD_FALSE;

static void inner_hash_irq_enable(td_u32 chn_num, td_bool enable)
{
#if defined(CONFIG_HASH_WAIT_FUNCTION_SUPPORT)
    td_u32 reg_val;

    reg_val = spacc_reg_read(HASH_CHANN_RAW_INT_EN);
    if (enable == TD_TRUE) {
        reg_val |= (1 << chn_num);
    } else {
        reg_val &= ~(1 << chn_num);
    }
    spacc_reg_write(HASH_CHANN_RAW_INT_EN, reg_val);
#endif
    crypto_unused(chn_num);
    crypto_unused(enable);
}

static td_s32 hal_hash_clear_channel(td_u32 chn_num)
{
    td_u32 i = 0;
    td_u32 clear_finish_reg = 0;
    td_u32 mask = 0x01 << chn_num;

    /* disable int_en default. */
    inner_hash_irq_enable(chn_num, TD_FALSE);

    spacc_reg_write(SPACC_HASH_CHN_CLEAR_REQ, mask);
    for (i = 0; i < CONFIG_HASH_CLEAR_CHN_TIMEOUT; i++) {
        clear_finish_reg = spacc_reg_read(SPACC_INT_RAW_HASH_CLEAR_FINISH);
        if ((mask & clear_finish_reg) != 0) {
            spacc_reg_write(SPACC_INT_RAW_HASH_CLEAR_FINISH, mask);
            break;
        }
        crypto_udelay(1);
    }

    if (i >= CONFIG_HASH_CLEAR_CHN_TIMEOUT) {
#if defined(CONFIG_HASH_TIMEOUT_DEBUG)
        hal_hash_debug();
        hal_hash_debug_chn(chn_num);
#endif
        crypto_log_err("hash clear channel timeout\n");
        return HASH_COMPAT_ERRNO(ERROR_HASH_CALC_TIMEOUT);
    }

    return TD_SUCCESS;
}

typedef struct {
    crypto_hash_type type;
    hash_chn_alg_mode_e mode;
    hash_chn_alg_sel_e alg;
    td_u32 state_size;
} hal_hash_cfg_item;

static hal_hash_cfg_item g_hash_cfg_item_list[] = {
#if defined(CONFIG_HASH_SHA1_SUPPORT)
    {
        .type = CRYPTO_HASH_TYPE_SHA1, .mode = 0, .alg = HASH_CHN_ALG_SEL_SHA1, .state_size = HASH_STATE_SIZE_160
    },
#endif
#if defined(CONFIG_HASH_SHA224_SUPPORT)
    {
        .type = CRYPTO_HASH_TYPE_SHA224, .mode = HASH_CHN_ALG_MODE_224,
        .alg = HASH_CHN_ALG_SEL_SHA2, .state_size = HASH_STATE_SIZE_256
    },
#endif
#if defined(CONFIG_HASH_SHA256_SUPPORT)
    {
        .type = CRYPTO_HASH_TYPE_SHA256, .mode = HASH_CHN_ALG_MODE_256,
        .alg = HASH_CHN_ALG_SEL_SHA2, .state_size = HASH_STATE_SIZE_256
    },
#endif
#if defined(CONFIG_HASH_SHA384_SUPPORT)
    {
        .type = CRYPTO_HASH_TYPE_SHA384, .mode = HASH_CHN_ALG_MODE_384,
        .alg = HASH_CHN_ALG_SEL_SHA2, .state_size = HASH_STATE_SIZE_512
    },
#endif
#if defined(CONFIG_HASH_SHA512_SUPPORT)
    {
        .type = CRYPTO_HASH_TYPE_SHA512, .mode = HASH_CHN_ALG_MODE_512,
        .alg = HASH_CHN_ALG_SEL_SHA2, .state_size = HASH_STATE_SIZE_512
    },
#endif
#if defined(CONFIG_HASH_SM3_SUPPORT)
    {
        .type = CRYPTO_HASH_TYPE_SM3, .mode = HASH_CHN_ALG_MODE_224,
        .alg = HASH_CHN_ALG_SEL_SM3, .state_size = HASH_STATE_SIZE_256
    },
#endif
};

static td_s32 priv_hal_hash_get_cfg(crypto_hash_type hash_type, hash_chn_alg_mode_e *mode, hash_chn_alg_sel_e *alg,
    td_u32 *state_size)
{
    td_u32 i;
    crypto_hash_type type = crypto_hash_remove_hmac_flag(hash_type);
    for (i = 0; i < crypto_array_size(g_hash_cfg_item_list); i++) {
        if (g_hash_cfg_item_list[i].type == type) {
            *mode = g_hash_cfg_item_list[i].mode;
            *alg = g_hash_cfg_item_list[i].alg;
            *state_size = g_hash_cfg_item_list[i].state_size;
            return TD_SUCCESS;
        }
    }
    crypto_log_err("Invalid Hash Mode!\n");
    return HASH_COMPAT_ERRNO(ERROR_UNSUPPORT);
}

td_s32 hal_cipher_hash_init(td_void)
{
    td_u32 i;
    td_s32 ret = TD_FAILURE;
    hal_hash_hard_chn_ctx *hard_ctx = TD_NULL;
    crypto_hal_func_enter();

    if (g_hal_hash_initialize == TD_TRUE) {
        return TD_SUCCESS;
    }
    (td_void)memset_s(g_hash_hard_ctx, sizeof(g_hash_hard_ctx), 0, sizeof(g_hash_hard_ctx));

#if !!defined(CONFIG_MMU_SUPPORT)
    g_node_buffer = crypto_malloc_mmz(HASH_NODE_SIZE * CONFIG_HASH_HARD_CHN_CNT, "crypto_hash_node_buffer");
    if (g_node_buffer == TD_NULL) {
        crypto_log_err("crypto_malloc_mmz failed\n");
        return TD_FAILURE;
    }
#endif
    for (i = 0; i < CONFIG_HASH_HARD_CHN_CNT; i++) {
        hard_ctx = &g_hash_hard_ctx[i];
        hard_ctx->entry_in = (hash_entry_in *)((uintptr_t)g_node_buffer + HASH_NODE_SIZE * i);
        hard_ctx->idx_cnt = CONFIG_HASH_NODE_DEPTH;
        hard_ctx->idx_in = 0;
    }

    g_hal_hash_initialize = TD_TRUE;

    ret = TD_SUCCESS;
    crypto_hal_func_exit();
    return ret;
}

td_s32 hal_cipher_hash_deinit(td_void)
{
    td_u32 i;
    td_s32 ret = TD_FAILURE;
    td_u32 node_size = HASH_NODE_SIZE * CONFIG_HASH_HARD_CHN_CNT;
    crypto_hal_func_enter();

    if (g_hal_hash_initialize == TD_FALSE) {
        return TD_SUCCESS;
    }

    for (i = 0; i < CONFIG_HASH_HARD_CHN_CNT; i++) {
        hal_hash_unlock(i);
    }
    (td_void)memset_s(g_node_buffer, node_size, 0, node_size);
#if !!defined(CONFIG_MMU_SUPPORT)
    crypto_free_coherent(g_node_buffer);
#endif
    g_hal_hash_initialize = TD_FALSE;
    ret = TD_SUCCESS;
    crypto_hal_func_exit();
    return ret;
}

#define HASH_SS_SECURE_VAL      0x5
#define HASH_SS_NONSECURE_VAL    0xa

td_s32 hal_hash_lock(td_u32 chn_num)
{
    td_s32 ret;
    td_u32 used = 0;
    td_u32 chnn_who_used = 0;
    spacc_cpu_mask cpu_mask = SPACC_CPU_IDLE;
    in_hash_chn_ctrl ctrl = { 0 };
    hal_hash_hard_chn_ctx *hard_ctx = TD_NULL;
    td_u64 entry_in_phys_addr = 0;

    crypto_hal_func_enter();
    crypto_param_require(g_hal_hash_initialize == TD_TRUE);

    if (chn_num >= CONFIG_HASH_HARD_CHN_CNT || chn_num == 0) {
        return TD_FAILURE;
    }

    hard_ctx = &g_hash_hard_ctx[chn_num];
    if (crypto_get_cpu_type() == CRYPTO_CPU_TYPE_SCPU) {
        cpu_mask = SPACC_CPU_TEE;
    } else {
        cpu_mask = SPACC_CPU_REE;
    }

    used = spacc_reg_read(SPACC_HASH_CHN_LOCK);

    /* try to use this channel. */
    chnn_who_used = CHN_WHO_USED_GET(used, chn_num);
    crypto_chk_return_only(chnn_who_used != SPACC_CPU_IDLE, HASH_COMPAT_ERRNO(ERROR_CHN_BUSY));

    CHN_WHO_USED_SET(used, chn_num, cpu_mask);
    spacc_reg_write(SPACC_HASH_CHN_LOCK, used);

    /* check if lock success. */
    used = spacc_reg_read(SPACC_HASH_CHN_LOCK);
    chnn_who_used = CHN_WHO_USED_GET(used, chn_num);
    crypto_chk_return_only(chnn_who_used != cpu_mask, HASH_COMPAT_ERRNO(ERROR_CHN_BUSY));

    /* clear hash channel. */
    ret = hal_hash_clear_channel(chn_num);
    crypto_chk_return(ret != TD_SUCCESS, ret, "hal_hash_clear_channel failed\n");
    /* set secure cfg. */
    ctrl.bits.hash_chn_en = 1;
    if (crypto_get_cpu_type() == CRYPTO_CPU_TYPE_SCPU) {
        ctrl.bits.hash_chn_ss = HASH_SS_SECURE_VAL;
    } else {
        ctrl.bits.hash_chn_ss = HASH_SS_NONSECURE_VAL;
    }
    spacc_reg_write(IN_HASH_CHN_CTRL(chn_num), ctrl.u32);

    /* set node start addr and node length. */
    entry_in_phys_addr = crypto_get_phys_addr(hard_ctx->entry_in);
    spacc_reg_write(IN_HASH_CHN_NODE_START_ADDR_L(chn_num), crypto_addr_l(entry_in_phys_addr));
    spacc_reg_write(IN_HASH_CHN_NODE_START_ADDR_H(chn_num), crypto_addr_h(entry_in_phys_addr));
    spacc_reg_write(IN_HASH_CHN_NODE_LENGTH(chn_num), CONFIG_HASH_NODE_DEPTH);

    hal_cipher_hash_done_try(chn_num);

    hard_ctx->idx_in = 0;
    hard_ctx->is_locked = TD_TRUE;
    (td_void)memset_s(hard_ctx->entry_in, HASH_NODE_SIZE, 0, HASH_NODE_SIZE);

    crypto_hal_func_exit();
    return TD_SUCCESS;
}

td_s32 hal_hash_unlock(td_u32 chn_num)
{
    td_u32 used;
    crypto_hal_func_enter();
    crypto_param_require(g_hal_hash_initialize == TD_TRUE);

    used = spacc_reg_read(SPACC_HASH_CHN_LOCK);
    CHN_WHO_USED_CLR(used, chn_num);
    spacc_reg_write(SPACC_HASH_CHN_LOCK, used);

    crypto_hal_func_exit();
    return TD_SUCCESS;
}

td_s32 hal_cipher_hash_config(td_u32 chn_num, crypto_hash_type hash_type, const td_u32 *state)
{
    td_s32 ret;
    td_u32 i;
    in_hash_chn_key_ctrl hash_key_ctrl = { 0 };
    hash_chn_alg_mode_e mode;
    hash_chn_alg_sel_e alg;
    td_u32 state_size;
    crypto_hal_func_enter();

    crypto_param_require(g_hal_hash_initialize == TD_TRUE);
    crypto_param_require(chn_num < CONFIG_HASH_HARD_CHN_CNT);
    crypto_param_require(state != TD_NULL);

    ret = priv_hal_hash_get_cfg(hash_type, &mode, &alg, &state_size);
    crypto_chk_return(ret != TD_SUCCESS, ret, "priv_hal_hash_get_cfg failed\n");

    hash_key_ctrl.u32 = spacc_reg_read(IN_HASH_CHN_KEY_CTRL(chn_num));
    hash_key_ctrl.bits.hash_chn_alg_mode = mode;
    hash_key_ctrl.bits.hash_chn_alg_sel = alg;
    spacc_reg_write(IN_HASH_CHN_KEY_CTRL(chn_num), hash_key_ctrl.u32);

    /* Write last state */
    for (i = 0; i < state_size / CRYPTO_WORD_WIDTH; i++) {
        spacc_reg_write(CHANN_HASH_STATE_VAL_ADDR(chn_num), i);
        spacc_reg_write(CHANN_HASH_STATE_VAL(chn_num), state[i]);
    }

    g_hash_hard_ctx[chn_num].block_size = crypto_hash_get_block_size(hash_type) / CRYPTO_BITS_IN_BYTE;
    crypto_hal_func_exit();
    return TD_SUCCESS;
}

td_s32 hal_cipher_hash_attach(td_u32 chn_num, td_u32 keyslot_chn_num)
{
    in_hash_chn_key_ctrl hash_key_ctrl = { 0 };
    crypto_hal_func_enter();
    crypto_param_require(g_hal_hash_initialize == TD_TRUE);

    hash_key_ctrl.u32 = spacc_reg_read(IN_HASH_CHN_KEY_CTRL(chn_num));
    hash_key_ctrl.bits.hash_key_chn_id = keyslot_chn_num;
    hash_key_ctrl.bits.hmac_vld = 1;
    spacc_reg_write(IN_HASH_CHN_KEY_CTRL(chn_num), hash_key_ctrl.u32);
    crypto_hal_func_exit();
    return CRYPTO_SUCCESS;
}

td_s32 hal_cipher_hash_add_in_node(td_u32 chn_num, td_u64 data_phys, td_u32 data_len,
    in_node_type_e in_node_type)
{
    td_u32 idx;
    hal_hash_hard_chn_ctx *hard_ctx = TD_NULL;
    hash_entry_in *entry_in = TD_NULL;
    crypto_hal_func_enter();
    crypto_param_require(g_hal_hash_initialize == TD_TRUE);

    crypto_chk_return(data_phys == 0, HASH_COMPAT_ERRNO(ERROR_INVALID_PHYS_ADDR), "data_phys is invalid\n");
    crypto_chk_return(data_phys % CONFIG_SPACC_ADDR_ALIGN_LEN != 0, HASH_COMPAT_ERRNO(ERROR_HASH_ADDR_NOT_ALIGNED),
        "data_phys must be aligned to %u-byte\n", CONFIG_SPACC_ADDR_ALIGN_LEN);
    hard_ctx = &g_hash_hard_ctx[chn_num];

    crypto_chk_return(hard_ctx->block_size == 0, HASH_COMPAT_ERRNO(ERROR_INVALID_PARAM),
        "block_size is zero!\n");
    crypto_chk_return(data_len % hard_ctx->block_size != 0, HASH_COMPAT_ERRNO(ERROR_HASH_LEN_NOT_ALIGNED),
        "data_len must be aligned to block size\n");

    idx = hard_ctx->idx_in++;
    hard_ctx->idx_in %= hard_ctx->idx_cnt;

    /* clear entry_in. */
    entry_in = &hard_ctx->entry_in[idx];
    (td_void)memset_s(entry_in, sizeof(hash_entry_in), 0, sizeof(hash_entry_in));

    /* set addr and length. */
    entry_in->hash_first_node = ((in_node_type & IN_NODE_TYPE_FIRST) == 0) ? 0 : 1;
    entry_in->hash_last_node = ((in_node_type & IN_NODE_TYPE_LAST) == 0) ? 0 : 1;
    entry_in->hash_start_addr_l = data_phys;
    entry_in->hash_start_addr_h = crypto_addr_h(data_phys);
    entry_in->hash_alg_length = data_len;

    if ((in_node_type & IN_NODE_TYPE_LAST) != 0) {
        hard_ctx->is_last = TD_TRUE;
    } else {
        hard_ctx->is_last = TD_FALSE;
    }

    crypto_hal_func_exit();
    return CRYPTO_SUCCESS;
}

td_s32 hal_cipher_hash_start(td_u32 chn_num, td_bool is_wait)
{
    td_u32 ptr;
    in_hash_chn_node_wr_point in_node_wr_ptr;
    hal_hash_hard_chn_ctx *hard_ctx = TD_NULL;
    crypto_hal_func_enter();
    crypto_param_require(g_hal_hash_initialize == TD_TRUE);

    crypto_unused(is_wait);
    hard_ctx = &g_hash_hard_ctx[chn_num];

#if defined(CONFIG_HASH_WAIT_FUNCTION_SUPPORT)
    if (hard_ctx->wait_func != TD_NULL && is_wait) {
        hard_ctx->done = TD_FALSE;
        hard_ctx->is_wait = TD_TRUE;
        inner_hash_irq_enable(chn_num, TD_TRUE);
    }
#endif

    /* configure in-node, only compute one nodes */
    in_node_wr_ptr.u32 = spacc_reg_read(IN_HASH_CHN_NODE_WR_POINT(chn_num));

    ptr = in_node_wr_ptr.bits.hash_chn_node_wr_point + 1;
    in_node_wr_ptr.bits.hash_chn_node_wr_point = ptr % hard_ctx->idx_cnt;

    /* make sure all the above explicit memory accesses and instructions are completed
     * before start the hardware.
     */
    crypto_memory_barrier();
    crypto_cache_flush((uintptr_t)hard_ctx->entry_in, HASH_NODE_SIZE);
    crypto_cache_all();
    /* Start */
    spacc_reg_write(IN_HASH_CHN_NODE_WR_POINT(chn_num), in_node_wr_ptr.u32);

    crypto_hal_func_exit();
    return TD_SUCCESS;
}

#if defined(CONFIG_HASH_WAIT_FUNCTION_SUPPORT)
static td_bool hal_hash_condition(const td_void *param)
{
    hal_hash_hard_chn_ctx *hard_ctx = TD_NULL;
    td_u32 chn_num = *(td_u32 *)param;

    hard_ctx = &g_hash_hard_ctx[chn_num];
    if (hard_ctx->done == TD_TRUE) {
        hard_ctx->done = TD_FALSE;
        return TD_TRUE;
    } else {
        return TD_FALSE;
    }
}
#endif

static td_s32 inner_hash_wait_chain_done(td_u32 chn_num, hal_hash_hard_chn_ctx *hard_ctx)
{
    td_s32 ret = TD_SUCCESS;
    td_u32 i;

    crypto_unused(hard_ctx);
#if defined(CONFIG_HASH_WAIT_FUNCTION_SUPPORT)
    if ((hard_ctx->is_wait == TD_TRUE) && (hard_ctx->wait_func != TD_NULL)) {
        ret = hard_ctx->wait_func(hard_ctx->wait, hal_hash_condition, (td_void *)(&chn_num), hard_ctx->timeout_ms);
        if (ret <= 0) {
            crypto_log_err("wait_func Timeout!\n");
            return HASH_COMPAT_ERRNO(ERROR_HASH_CALC_TIMEOUT);
        }
        return TD_SUCCESS;
    }
#endif

    for (i = 0; i < CONFIG_HASH_CALC_TIMEOUT; i++) {
        if (hal_cipher_hash_done_try(chn_num) != 0) {
            break;
        }
        crypto_udelay(1);
    }
    if (i >= CONFIG_HASH_CALC_TIMEOUT) {
        crypto_log_err("hash wait done timeout, chn=%d\n", chn_num);
        ret = HASH_COMPAT_ERRNO(ERROR_HASH_CALC_TIMEOUT);
    }

    return ret;
}

static td_s32 inner_hash_wait_node_done(td_u32 chn_num)
{
    td_u32 i;
    td_u32 unproc_data_len = 0;
    in_hash_chn_node_wr_point hash_wr_ptr = { 0 };
    in_hash_chn_node_rd_point hash_rd_ptr = { 0 };

    for (i = 0; i < CONFIG_HASH_CALC_TIMEOUT; i++) {
        hash_rd_ptr.u32 = spacc_reg_read(IN_HASH_CHN_NODE_RD_POINT(chn_num));
        hash_wr_ptr.u32 = spacc_reg_read(IN_HASH_CHN_NODE_WR_POINT(chn_num));
        if (hash_rd_ptr.u32 == hash_wr_ptr.u32) {
            break;
        }
        crypto_udelay(1);
    }

    if (i >= CONFIG_HASH_CALC_TIMEOUT) {
        return HASH_COMPAT_ERRNO(ERROR_HASH_CALC_TIMEOUT);
    }

    // After the value of the read pointer is increased by 1, wait a microsecond to ensure that the
    // value of IN_HASH_CHN_DATA_LEN is switched from 0 to the actual unprocessed data length.
    crypto_udelay(1);
    for (i = 0; i < CONFIG_HASH_CALC_TIMEOUT; i++) {
        unproc_data_len = spacc_reg_read(IN_HASH_CHN_DATA_LEN(chn_num));
        if (unproc_data_len == 0) {
            break;
        }
        crypto_udelay(1);
    }
    if (i >= CONFIG_HASH_CALC_TIMEOUT) {
        return HASH_COMPAT_ERRNO(ERROR_HASH_CALC_TIMEOUT);
    }

    return CRYPTO_SUCCESS;
}

td_s32 hal_cipher_hash_wait_done(td_u32 chn_num, td_u32 *state, td_u32 state_size)
{
    td_u32 i;
    td_s32 ret = TD_SUCCESS;
    hal_hash_hard_chn_ctx *hard_ctx = TD_NULL;
    crypto_hal_func_enter();
    crypto_param_require(g_hal_hash_initialize == TD_TRUE);

    hard_ctx = &g_hash_hard_ctx[chn_num];

    if (hard_ctx->is_last) {
        ret = inner_hash_wait_chain_done(chn_num, hard_ctx);
    } else {
        ret = inner_hash_wait_node_done(chn_num);
    }
    inner_hash_irq_enable(chn_num, TD_FALSE);
    if (ret == HASH_COMPAT_ERRNO(ERROR_HASH_CALC_TIMEOUT)) {
#if defined(CONFIG_HASH_TIMEOUT_DEBUG)
        hal_hash_debug();
        hal_hash_debug_chn(chn_num);
#endif
    } else {
        ret = CRYPTO_SUCCESS;
    }

    /* read hash result */
    if (state != TD_NULL) {
        for (i = 0; i < state_size / CRYPTO_WORD_WIDTH; i++) {
            spacc_reg_write(CHANN_HASH_STATE_VAL_ADDR(chn_num), i);
            state[i] = spacc_reg_read(CHANN_HASH_STATE_VAL(chn_num));
        }
    }

    crypto_hal_func_exit();
    return ret;
}

#if defined(CONFIG_HASH_WAIT_FUNCTION_SUPPORT)
td_s32 hal_cipher_hash_register_wait_func(td_u32 chn_num, td_void *wait,
    crypto_wait_timeout_interruptible wait_func, td_u32 timeout_ms)
{
    hal_hash_hard_chn_ctx *hard_ctx = TD_NULL;
    crypto_hal_func_enter();
    crypto_param_require(g_hal_hash_initialize == TD_TRUE);

    hard_ctx = &g_hash_hard_ctx[chn_num];
    hard_ctx->wait = wait;
    hard_ctx->wait_func = wait_func;
    hard_ctx->timeout_ms = timeout_ms;

    crypto_hal_func_exit();
    return TD_SUCCESS;
}

td_s32 hal_cipher_hash_done_notify(td_u32 chn_num)
{
    hal_hash_hard_chn_ctx *hard_ctx = TD_NULL;
    crypto_hal_func_enter();
    crypto_param_require(g_hal_hash_initialize == TD_TRUE);

    hard_ctx = &g_hash_hard_ctx[chn_num];
    hard_ctx->done = TD_TRUE;

    crypto_hal_func_exit();
    return TD_SUCCESS;
}
#endif

td_u32 hal_cipher_hash_done_try(td_u32 chn_num)
{
    hash_chann_raw_int hash_fin_int_raw = { 0 };
    crypto_param_require(g_hal_hash_initialize == TD_TRUE);

    hash_fin_int_raw.u32 = spacc_reg_read(HASH_CHANN_RAW_INT);
    hash_fin_int_raw.u32 &= (0x01 << chn_num);
    spacc_reg_write(HASH_CHANN_RAW_INT, hash_fin_int_raw.u32);

    return hash_fin_int_raw.u32;
}

#if defined(CONFIG_HASH_TIMEOUT_DEBUG)
td_void hal_hash_debug(td_void)
{
    td_u32 i;
    td_u32 chnn_who_used;
    td_u32 used;
    crypto_hal_func_enter();

    used = spacc_reg_read(SPACC_HASH_CHN_LOCK);

    crypto_print("Current Cpu Type is %s\n", crypto_get_cpu_type() == CRYPTO_CPU_TYPE_SCPU ? "SCPU" : "ACPU");
    crypto_print("The Status of Hash Hard Channel:\n");
    for (i = 0; i < CONFIG_HASH_HARD_CHN_CNT; i++) {
        chnn_who_used = CHN_WHO_USED_GET(used, i);
        if (chnn_who_used == SPACC_CPU_REE) {
            crypto_print("CHN %d is locked by REE CPU\n", i);
        } else if (chnn_who_used == SPACC_CPU_TEE) {
            crypto_print("CHN %d is locked by TEE CPU\n", i);
        } else {
            crypto_print("CHN %d is idle\n", i);
        }
    }
    crypto_print("HASH_CHANN_RAW_INT is 0x%x\n", spacc_reg_read(HASH_CHANN_RAW_INT));
    crypto_print("HASH_CHANN_RAW_INT_EN is 0x%x\n", spacc_reg_read(HASH_CHANN_RAW_INT_EN));

    crypto_print("TEE_HASH_CALC_CTRL_CHECK_ERR is 0x%x\n", spacc_reg_read(TEE_HASH_CALC_CTRL_CHECK_ERR));
    crypto_print("TEE_HASH_CALC_CTRL_CHECK_ERR_STATUS is 0x%x\n", spacc_reg_read(TEE_HASH_CALC_CTRL_CHECK_ERR_STATUS));

    crypto_print("REE_HASH_CALC_CTRL_CHECK_ERR is 0x%x\n", spacc_reg_read(REE_HASH_CALC_CTRL_CHECK_ERR));
    crypto_print("REE_HASH_CALC_CTRL_CHECK_ERR_STATUS is 0x%x\n", spacc_reg_read(REE_HASH_CALC_CTRL_CHECK_ERR_STATUS));

    crypto_hal_func_exit();
}

td_void hal_hash_debug_chn(td_u32 chn_num)
{
    td_u32 i;
    hal_hash_hard_chn_ctx *hard_ctx = TD_NULL;
    hash_entry_in *entry_in = TD_NULL;
    crypto_hal_func_enter();

    crypto_unused(entry_in);
    if (chn_num >= CONFIG_HASH_HARD_CHN_CNT) {
        crypto_log_err("Invalid chn_num!\n");
        return;
    }

    hard_ctx = &g_hash_hard_ctx[chn_num];

    crypto_print("The Status of Hash Hard Channel %d:\n", chn_num);
    crypto_print("IN_HASH_CHN_CTRL is 0x%x\n", spacc_reg_read(IN_HASH_CHN_CTRL(chn_num)));
    crypto_print("IN_HASH_CHN_KEY_CTRL is 0x%x\n", spacc_reg_read(IN_HASH_CHN_KEY_CTRL(chn_num)));
    crypto_print("IN_HASH_CHN_NODE_LENGTH: 0x%x\n", spacc_reg_read(IN_HASH_CHN_NODE_LENGTH(chn_num)));
    crypto_print("IN_HASH_CHN_NODE_START_ADDR_L: 0x%x\n", spacc_reg_read(IN_HASH_CHN_NODE_START_ADDR_L(chn_num)));
    crypto_print("IN_HASH_CHN_NODE_START_ADDR_H: 0x%x\n", spacc_reg_read(IN_HASH_CHN_NODE_START_ADDR_H(chn_num)));

    crypto_print("IN_HASH_CHN_NODE_WR_POINT: 0x%x\n", spacc_reg_read(IN_HASH_CHN_NODE_WR_POINT(chn_num)));
    crypto_print("IN_HASH_CHN_NODE_RD_POINT: 0x%x\n", spacc_reg_read(IN_HASH_CHN_NODE_RD_POINT(chn_num)));

    crypto_print("entry_in's virtual addr is %p, physical addr is 0x%lx\n", hard_ctx->entry_in,
        (td_ulong)crypto_get_phys_addr(hard_ctx->entry_in));
    crypto_print("Total Entry Count is %u, Current Entry IDX is %u\n", hard_ctx->idx_cnt, hard_ctx->idx_in);
#if defined(CONFIG_HASH_WAIT_FUNCTION_SUPPORT)
    if (hard_ctx->wait_func != TD_NULL) {
        crypto_print("Register Wait Func, timeout is %u ms, done flag is %s\n", hard_ctx->timeout_ms, hard_ctx->done ?
            "TD_TRUE" : "TD_FALSE");
    }
#endif
    /* Print Entry_In. */
    for (i = 0; i < CONFIG_HASH_NODE_DEPTH; i++) {
        crypto_print("Entry_IN[%u]:\n", i);
        entry_in = &hard_ctx->entry_in[i];
        crypto_print("hash_first_node is 0x%x\n", entry_in->hash_first_node);
        crypto_print("hash_last_node is 0x%x\n", entry_in->hash_last_node);
        crypto_print("hash_alg_length is 0x%x\n", entry_in->hash_alg_length);
        crypto_print("hash_start_addr_l is 0x%x\n", entry_in->hash_start_addr_l);
        crypto_print("hash_start_addr_h is 0x%x\n", entry_in->hash_start_addr_h);
    }

    /* Print last state */
    for (i = 0; i < CRYPTO_HASH_RESULT_SIZE_MAX_IN_WORD; i++) {
        spacc_reg_write(CHANN_HASH_STATE_VAL_ADDR(chn_num), i);
        crypto_print("CHANn_HASH_STATE_VAL[%d] is 0x%x\n", i, spacc_reg_read(CHANN_HASH_STATE_VAL(chn_num)));
    }
    crypto_hal_func_exit();
}
#endif