/**
  Copyright (c), 2001-2025, Shenshu Tech. Co., Ltd.
*/

#include "hal_pke_v5.h"
#include "hal_pke_reg_v5.h"
#include "crypto_errno.h"
#include "crypto_drv_common.h"
#include "crypto_common_macro.h"
#include "rom_lib.h"

#if defined(CONFIG_PKE_RAM_MASK_SUPPORT)
#include "hal_trng.h"
#endif

/************************************************** global define start************************************/
#define PKE_COMPAT_ERRNO(err_code)      HAL_COMPAT_ERRNO(ERROR_MODULE_PKE, err_code)
#if (defined(CRYPTO_LOG_LEVEL) && (CRYPTO_LOG_LEVEL == 2 || CRYPTO_LOG_LEVEL == 3 || CRYPTO_LOG_LEVEL == 4))
#define crypto_dump_buffer(buffer_name, buffer, len) crypto_dump_data(buffer_name, buffer, len)
#else
#define crypto_dump_buffer(fmt, args...)
#endif

td_u32 g_pke_initialize;
static td_u32 g_lock_code = CPU_ID_SCPU;


/************************************************** global define end************************************/

/************************************************** hal inner API start************************************/
/* function define */
td_bool pke_lock_condition(const td_void *param __attribute__((unused)));
td_s32 hal_pke_check_robust_warn(void);
td_s32 hal_pke_error_code(void);
td_s32 check_instr_rdy(pke_mode mode);
void hal_pke_start_pre_process(void);
td_s32 get_lock_code(td_u32 *lock_code);

/**
 * @brief start PKE calculate in single instruction0 mode
 */
void hal_pke_start0(void);

/**
 * @brief start PKE calculate in sigle instruction1 mode
 */
void hal_pke_start1(void);

/**
 * @brief start PKE calculate in batch processing mode
 */
void hal_pke_batch_start(void);

/**
 * @brief PKE wait for free in loop
 *
 * @return td_s32
 */
td_s32 hal_pke_wait_free(void);

td_s32 hal_pke_check_robust_warn(void)
{
    pke_alarm_status result = {.u32 = PKE_NON_SPECIAL_VAL};

    result.u32 = pke_reg_read(PKE_ALARM_STATUS);
    if (result.bits.alarm_int == PKE_ALARM_STATUS_EFFECTIVE_CODE) {
        result.u32 = PKE_ALARM_STATUS_CLEAN_CODE; // clear warn interrupt
        pke_reg_write(PKE_ALARM_STATUS, result.u32);
        return TD_SUCCESS;
    }
    return TD_FAILURE;
}

td_s32 hal_pke_error_code(void)
{
    pke_failure_flag result = {.u32 = PKE_NON_SPECIAL_VAL};

    result.u32 = pke_reg_read(PKE_FAILURE_FLAG);
    if (result.u32 == 0) {
        return TD_SUCCESS;
    }

    crypto_log_err("Hardware Error Code: 0x%x\n", result.u32);
    return PKE_COMPAT_ERRNO(ERROR_PKE_LOGIC);
}

td_s32 check_instr_rdy(pke_mode mode)
{
    pke_instr_rdy instr_rdy;
    td_u32 mode_rdy_location;
    td_u32 i;

    switch (mode) {
        case PKE_SINGLE_INSTR0: {
            mode_rdy_location = 0x1; /* single_instr0: 0x1 */
            break;
        }
        case PKE_SINGLE_INSTR1: {
            mode_rdy_location = 0x2; /* single_instr0: 0x2 */
            break;
        }
        case PKE_BATCH_INSTR: {
            mode_rdy_location = 0x4; /* batch_instr: 0x4 */
            break;
        }
        default:
            crypto_log_err("error, pke_instr_start_mode invaild!\n");
            return TD_FAILURE;
    }

    /* wait ready */
    for (i = 0; i < CONFIG_PKE_TIMEOUT_IN_US; i++) {
        instr_rdy.u32  = pke_reg_read(PKE_INSTR_RDY);
        if ((instr_rdy.u32 & mode_rdy_location) == mode_rdy_location) {
            break;
        }
        crypto_udelay(1); /* 1us */
    }

    if (i >= CONFIG_PKE_TIMEOUT_IN_US) {
        return PKE_COMPAT_ERRNO(ERROR_PKE_WAIT_DONE_TIMEOUT);
    }

    return TD_SUCCESS;
}

void hal_pke_start_pre_process(void)
{
    /* 1.clear interrupt */
    pke_int_no_mask_status result = {.u32 = PKE_NON_SPECIAL_VAL};
    result.u32 = pke_reg_read(PKE_INT_NOMASK_STATUS);
    result.bits.finish_int_nomask = 1;
    pke_reg_write(PKE_INT_NOMASK_STATUS, result.u32);

    /* call back for secure enhancement */
    result.u32 = pke_reg_read(PKE_INT_NOMASK_STATUS);
    val_enhance_chk(result.bits.finish_int_nomask, 1);
}

#if defined(CONFIG_PKE_SINGLE_INSTR_SUPPORT)
void hal_pke_start0(void)
{
    pke_start start = {0};
    hal_pke_start_pre_process();

    start = (pke_start) {.u32 = PKE_NON_SPECIAL_VAL};
    start.u32 = PKE_START0_CODE;
    pke_reg_write(PKE_START, start.u32);

    /* call back for secure enhancement */
    reg_callback_chk(PKE_START, PKE_START0_CODE);

    return;
}

void hal_pke_start1(void)
{
    pke_start start = {0};
    hal_pke_start_pre_process();

    start = (pke_start) {.u32 = PKE_NON_SPECIAL_VAL};
    start.u32 = PKE_START1_CODE;
    pke_reg_write(PKE_START, start.u32);

    /* call back for secure enhancement */
    reg_callback_chk(PKE_START, PKE_START1_CODE);

    return;
}
#endif

void hal_pke_batch_start(void)
{
    pke_start start = {0};
    hal_pke_start_pre_process();

    start = (pke_start) {.u32 = PKE_NON_SPECIAL_VAL};
    start.u32 = PKE_BATCH_START_CODE;
    pke_reg_write(PKE_START, start.u32);

    /* call back for secure enhancement */
    reg_callback_chk(PKE_START, PKE_BATCH_START_CODE);

    return;
}

td_s32 hal_pke_wait_free(void)
{
    td_u32 i = 0;
    td_s32 ret = TD_FAILURE;
    pke_busy busy = {.u32 = PKE_NON_SPECIAL_VAL};
    pke_int_no_mask_status int_status = {0};

    /* wait ready */
    for (i = 0; i < CONFIG_PKE_TIMEOUT_IN_US; i++) {
        busy.u32 = pke_reg_read(PKE_BUSY);
        if (busy.bits.pke_busy == 0) {
            break;
        }
        crypto_udelay(1); /* 1us */
    }

    /* get interrupt status */
    int_status.u32 = PKE_NON_SPECIAL_VAL;
    int_status.u32 = pke_reg_read(PKE_INT_NOMASK_STATUS);
    if (i < CONFIG_PKE_TIMEOUT_IN_US && int_status.bits.finish_int_nomask == PKE_INT_NOMASK_FINISH_EFFECTIVE_CODE) {
        /* clear interrupt */
        int_status.bits.finish_int_nomask = 1;
        pke_reg_write(PKE_INT_NOMASK_STATUS, int_status.u32);
        ret = TD_SUCCESS;
    } else {
        ret = PKE_COMPAT_ERRNO(ERROR_PKE_WAIT_DONE_TIMEOUT);
    }
    crypto_chk_print((ret != TD_SUCCESS), "error, pke wait free timeout\n");

    return ret;
}

/************************************************** hal inner API end************************************/

/************************************************** hal outter API start************************************/

td_s32 hal_pke_init(void)
{
    if (g_pke_initialize == TD_TRUE) {
        return TD_SUCCESS;
    }
    get_lock_code(&g_lock_code);
    g_pke_initialize = TD_TRUE;
    return TD_SUCCESS;
}

td_s32 hal_pke_deinit(void)
{
    if (g_pke_initialize == TD_TRUE) {
        g_pke_initialize = TD_FALSE;
        return TD_SUCCESS;
    }

    return PKE_COMPAT_ERRNO(ERROR_NOT_INIT);
}

td_s32 get_lock_code(td_u32 *lock_code)
{
    crypto_cpu_type cpu_type = crypto_get_cpu_type();
    switch (cpu_type) {
        case CRYPTO_CPU_TYPE_SCPU:
            *lock_code = CPU_ID_SCPU;
            break;
        case CRYPTO_CPU_TYPE_ACPU:
            *lock_code = CPU_ID_ACPU;
            break;
        case CRYPTO_CPU_TYPE_PCPU:
            *lock_code = CPU_ID_PCPU;
            break;
        case CRYPTO_CPU_TYPE_AIDSP:
            *lock_code = CPU_ID_AIDSP;
            break;
        default:
            *lock_code = CPU_ID_ACPU;
            return TD_FAILURE;
    }

    return TD_SUCCESS;
}

td_s32 hal_pke_lock(void)
{
    td_u32 i = 0;
    td_s32 ret = TD_FAILURE;
    pke_lock_ctrl lock_ctrl = {.u32 = PKE_NON_SPECIAL_VAL};
    pke_lock_status lock_status = {.u32 = PKE_NON_SPECIAL_VAL};

    /* lock pke */
    lock_ctrl.u32 = pke_reg_read(PKE_LOCK_CTRL);
    lock_ctrl.bits.pke_lock_type = 0; /* lock command */
    lock_ctrl.bits.pke_lock = 1;
    pke_reg_write(PKE_LOCK_CTRL, lock_ctrl.u32);

    for (i = 0; i < CONFIG_PKE_TIMEOUT_IN_US; i++) {
        /* check lock result */
        lock_status.u32 = pke_reg_read(PKE_LOCK_STATUS);
        if (lock_status.bits.pke_lock_stat == g_lock_code) {
            break;
        }
        crypto_udelay(1); // 1 us is empirical value of register lock read
    }
    if (i < CONFIG_PKE_TIMEOUT_IN_US) {
        ret = TD_SUCCESS;
    } else {
        ret = PKE_COMPAT_ERRNO(ERROR_PKE_LOCK_TIMEOUT);
    }

    crypto_chk_print((ret != TD_SUCCESS), "pke lock timeout\n");
    return ret;
}

void hal_pke_unlock(void)
{
    pke_lock_ctrl lock_ctrl = {.u32 = PKE_NON_SPECIAL_VAL};

    /* unlock pke */
    lock_ctrl.u32 = pke_reg_read(PKE_LOCK_CTRL);
    lock_ctrl.bits.pke_lock_type = 1; /* unlock command */
    lock_ctrl.bits.pke_lock = 1;
    pke_reg_write(PKE_LOCK_CTRL, lock_ctrl.u32);
}

void hal_pke_enable_noise(void)
{
    pke_noise_en noise = {
        .u32 = PKE_NON_SPECIAL_VAL
    };

    /* enable noise */
    noise.u32 = pke_reg_read(PKE_NOISE_EN);
    noise.bits.noise_en = 1;
    pke_reg_write(PKE_NOISE_EN, noise.u32);
}

void hal_pke_disable_noise(void)
{
    pke_noise_en noise = {.u32 = PKE_NON_SPECIAL_VAL};

    /* disable noise */
    noise.u32 = pke_reg_read(PKE_NOISE_EN);
    noise.bits.noise_en = 0;
    pke_reg_write(PKE_NOISE_EN, noise.u32);
}

td_s32 hal_pke_pre_process(void)
{
    td_s32 ret = TD_FAILURE;
    td_u32 random_num = DEFAULT_MASK_CODE;
    /* 2. set claculate will use random number for mask */
    pke_mask_rng_cfg mask_rng = {.u32 = PKE_NON_SPECIAL_VAL};
    mask_rng.u32 = pke_reg_read(PKE_MASK_RNG_CFG);
    mask_rng.bits.mask_rng_cfg = 1;
    pke_reg_write(PKE_MASK_RNG_CFG, mask_rng.u32);
    /* call back for secure enhancement */
    reg_callback_chk(PKE_MASK_RNG_CFG, mask_rng.u32);

    crypto_unused(ret);
    crypto_unused(random_num);
    /* 3. set mask random number */
#if defined(CONFIG_PKE_RAM_MASK_SUPPORT)
    random_num = pke_reg_read(PKE_DRAM_MASK);
    ret = hal_cipher_trng_get_random(&random_num);
    crypto_chk_func_return(hal_cipher_trng_get_random, ret);
    pke_reg_write(PKE_DRAM_MASK, random_num);
    /* call back for secure enhancement */
    reg_callback_chk(PKE_DRAM_MASK, random_num);
#endif
    return CRYPTO_SUCCESS;
}

td_s32 hal_pke_check_free(void)
{
    td_s32 ret = TD_FAILURE;
    td_u32 i = 0;
    pke_busy busy = {.u32 = PKE_NON_SPECIAL_VAL};

    /* wait ready */
    for (i = 0; i < CONFIG_PKE_TIMEOUT_IN_US; i++) {
        busy.u32 = pke_reg_read(PKE_BUSY);
        if (busy.bits.pke_busy == 0) {
            break;
        }
        crypto_udelay(1); /* 1us */
    }

    if (i < CONFIG_PKE_TIMEOUT_IN_US) {
        ret = TD_SUCCESS;
    } else {
        ret = PKE_COMPAT_ERRNO(ERROR_PKE_WAIT_DONE_TIMEOUT);
    }
    crypto_chk_print((ret != TD_SUCCESS), "error, pke wait free timeout\n");

    return ret;
}

td_void hal_pke_set_ram_const_1(td_u32 ram_section, td_u32 data_len)
{
    int32_t i;
    td_u32 random_num = DEFAULT_MASK_CODE;
    td_u32 start_dram_addr = PKE_DRAM_BASE + ram_section * PKE_DRAM_BLOCK_LENGTH;
    /* 1. get mask number */
    random_num = pke_reg_read(PKE_DRAM_MASK);

    for (i = data_len; i >= 0; i -= CRYPTO_WORD_WIDTH) {
        if (i == (int32_t)data_len) {
            pke_reg_write(start_dram_addr, 0x1 ^ random_num);
        } else {
            pke_reg_write(start_dram_addr, 0 ^ random_num);
        }
        start_dram_addr += CRYPTO_WORD_WIDTH;
    }
}

td_void hal_pke_set_ram(td_u32 ram_section, const td_u8 *data, td_u32 data_len, td_u32 aligned_len CIPHER_CHECK_WORD)
{
    td_u32 i = 0;
    td_u32 j = 0;
    td_u32 val = 0;
    td_u32 random_num = DEFAULT_MASK_CODE;
    td_u32 start_dram_addr = PKE_DRAM_BASE + ram_section * PKE_DRAM_BLOCK_LENGTH;
    td_u32 write_len = (data_len + CRYPTO_WORD_WIDTH - 1) / CRYPTO_WORD_WIDTH * CRYPTO_WORD_WIDTH;

    crypto_hal_func_enter();
    if (data == TD_NULL) {
        crypto_log_err("\r\n%s:%d data is NULL\n", __func__, __LINE__);
        return;
    }

    /* secure enhancement */
    check_sum_assert(ram_section, data, data_len, aligned_len);
    crypto_dump_buffer("data", data, data_len);

    /* 1. get mask number */
    random_num = pke_reg_read(PKE_DRAM_MASK);
    crypto_log_dbg("random_num: 0x%x\r\n", random_num);

    /* 2. set data into DRAM */
    /* Input the data which is aligned with 8 bytes;
       For little-endian system, on reading one word from ram to val, byte sequence should be adjusted as -
       in ram: Byte1 | Byte2 | Byte3 | Byte4 (section low <--> section high)
       to val: Byte4 | Byte3 | Byte2 | Byte1 (MSB <--> LSB)
    */
    /* not use dynamic micro in rom code, otherwise it won't be able to be changed after chip delivery. */
    for (i = data_len; i >= CRYPTO_WORD_WIDTH; i -= CRYPTO_WORD_WIDTH) {
        val = 0;
        val = data[i - 4] << 24; /* i - 4 index shift 24 bits */
        val |= data[i - 3] << 16; /* i - 3 index shift 16 bits */
        val |= data[i - 2] << 8; /* i - 2 index shift 8 bits */
        val |= data[i - 1];
        val ^= random_num;
        pke_reg_write(start_dram_addr, val);
        start_dram_addr += CRYPTO_WORD_WIDTH;
    }
    /* secure enhancement */
    val_enhance_chk(i < CRYPTO_WORD_WIDTH, TD_TRUE);

    /* Input the data which is not aligned with 4 bytes */
    if (i != 0) {
        val = 0;
        for (j = 0; j < i; j++) {
            val |= data[j] << (CRYPTO_BITS_IN_BYTE * (i - j - 1));
        }
        /* secure enhancement */
        val_enhance_chk(j, i);
        val ^= random_num;
        pke_reg_write(start_dram_addr, val);
        start_dram_addr += CRYPTO_WORD_WIDTH;
    }
    /* secure enhancement */
    val_enhance_chk(start_dram_addr, PKE_DRAM_BASE + ram_section * PKE_DRAM_BLOCK_LENGTH + write_len);

    /* 3. padding with 0x00 for not aligned with 64bits data */
    for (; write_len < aligned_len; write_len += CRYPTO_WORD_WIDTH) {
        val = 0x0;
        val ^= random_num;
        pke_reg_write(start_dram_addr, val);
        start_dram_addr += CRYPTO_WORD_WIDTH;
    }
    /* secure enhancement */
    val_enhance_chk(start_dram_addr, PKE_DRAM_BASE + ram_section * PKE_DRAM_BLOCK_LENGTH + aligned_len);

    crypto_hal_func_exit();
    return;
}

td_void hal_pke_get_ram(td_u32 ram_section, td_u8 *data, td_u32 data_len CIPHER_CHECK_WORD)
{
    td_u32 val = 0;
    td_u32 i = 0;
    td_u32 count = 0;
    td_u32 mask_random = DEFAULT_MASK_CODE;
    td_u32 count_len = data_len;
    td_u32 start_dram_addr = PKE_DRAM_BASE + ram_section * PKE_DRAM_BLOCK_LENGTH;

    crypto_hal_func_enter();
    if (data == TD_NULL) {
        crypto_log_err("data is NULL\n");
        return;
    }

    /* secure enhancement */
    check_sum_assert(ram_section, data, data_len);

    /* 1. get the mask random number */
    mask_random = pke_reg_read(PKE_DRAM_MASK);

    if ((count_len % CRYPTO_WORD_WIDTH) != 0) {
        val = pke_reg_read(start_dram_addr + (count_len - (count_len % CRYPTO_WORD_WIDTH)));
        val ^= mask_random;
        for (count = 0; count < (count_len % CRYPTO_WORD_WIDTH); count++) {
            *(data + count) = val >> (CRYPTO_BITS_IN_BYTE * ((count_len % CRYPTO_WORD_WIDTH) - 1 - count)) & 0xFF;
        }
        val_enhance_chk(count, count_len % CRYPTO_WORD_WIDTH);
        count_len = count_len - (count_len % CRYPTO_WORD_WIDTH);
    }
    for (i = count_len; i >= CRYPTO_WORD_WIDTH; i -= CRYPTO_WORD_WIDTH) {
        val = pke_reg_read(start_dram_addr + i - CRYPTO_WORD_WIDTH);
        val ^= mask_random;
        *(data + count + 0) = (val >> 24) & 0xFF; /* shift 24 bits */
        *(data + count + 1) = (val >> 16) & 0xFF; /* shift 16 bits */
        *(data + count + 2) = (val >> 8) & 0xFF;  /* offset 2, shift 8 bits */
        *(data + count + 3) = (val) & 0xFF;       /* offset 3 */
        count += CRYPTO_WORD_WIDTH;
    }
    val_enhance_chk(count, count_len);

#ifdef SEC_ENHANCE
    /* secure enhancement */
    count = 0;
    count_len = data_len;
    if ((count_len % CRYPTO_WORD_WIDTH) != 0) {
        val = 0;
        for (count = 0; count < (count_len % CRYPTO_WORD_WIDTH); count++) {
            val |= *(data + count) << (CRYPTO_BITS_IN_BYTE * (CRYPTO_WORD_WIDTH - 1 - count));
        }
        val ^= mask_random;
        val_enhance_chk(val, pke_reg_read(start_dram_addr + (count_len - (count_len % CRYPTO_WORD_WIDTH))));
        count_len = count_len - (count_len % CRYPTO_WORD_WIDTH);
    }

    for (i = count_len; i >= CRYPTO_WORD_WIDTH; i -= CRYPTO_WORD_WIDTH) {
        val = (*(data + count + 0) << 24) + // 24 bits left-shift according to algorithmn
            (*(data + count + 1) << 16) + // 16 bits left-shift according to algorithmn
            (*(data + count + 2) << CRYPTO_BITS_IN_BYTE) + // 2 is byte index: 0, 1, 2, 3
            *(data + count + 3); // 3 is byte index: 0, 1, 2, 3
        val ^= mask_random;
        val_enhance_chk(pke_reg_read(start_dram_addr + i - CRYPTO_WORD_WIDTH), val);
        count += CRYPTO_WORD_WIDTH;
    }
    val_enhance_chk(count, count_len);
#endif

    crypto_dump_buffer("data", data, data_len);
    crypto_hal_func_exit();
    return;
}

td_s32 hal_pke_clean_ram(void)
{
    td_s32 ret = TD_FAILURE;
    pke_dram_clr dram_clr = {0};
    crypto_hal_func_enter();

    /* 1. wait for PKE free, no need to wait. */
    /* Fotr that, if wait_done success, here will also success, if wait_done failed, here will also failed, but will
    cost a long time to wait failure result.  */

    dram_clr.u32 = PKE_NON_SPECIAL_VAL;
    dram_clr.u32 = pke_reg_read(PKE_DRAM_CLR);
    dram_clr.bits.dram_clr = 0x1;
    pke_reg_write(PKE_DRAM_CLR, dram_clr.u32);

    /* 2. wait for PKE free */
    ret = hal_pke_check_free();
    crypto_hal_func_exit();
    return ret;
}

td_s32 hal_pke_set_mode(pke_mode mode, td_u32 single_instr, const rom_lib *batch_instr,
    pke_data_work_len work_len CIPHER_CHECK_WORD)
{
    td_s32 ret = TD_FAILURE;
    td_u32 batch_instr_addr = 0;
    td_u32 batch_instr_num = 0;
    crypto_hal_func_enter();
    /* secure enhancement */
    check_sum_assert(mode, single_instr, batch_instr, work_len);

    crypto_chk_return((mode == PKE_SINGLE_INSTR0 || mode == PKE_SINGLE_INSTR1) && single_instr == 0,
        PKE_COMPAT_ERRNO(ERROR_INVALID_PARAM), "mode is invalid\n");
    crypto_chk_return(mode == PKE_BATCH_INSTR && batch_instr == TD_NULL,
        PKE_COMPAT_ERRNO(ERROR_INVALID_PARAM), "mode is invalid\n");

    if (batch_instr != TD_NULL) {
        /* avoid dynamic MICRO to be used in rom code. */
        batch_instr_addr = batch_instr->instr_addr;
        batch_instr_num = batch_instr->instr_num * CRYPTO_WORD_WIDTH;

        crypto_log_dbg("batch_instr_addr = %d\r\n",
            (batch_instr_addr - CONFIG_PKE_ROM_LIB_START_ADDR) / CRYPTO_WORD_WIDTH);
        crypto_log_dbg("batch_instr_len = %d\r\n", batch_instr->instr_num);
        crypto_log_dbg("batch_instr_0 = 0x%x\r\n", crypto_reg_read(batch_instr_addr));
    }

    /* 1. set work_len, work_len = celing(width/64bit) */
    pke_reg_write(PKE_WORK_LEN, work_len);
    /* call back for secure enhancement */
    reg_callback_chk(PKE_WORK_LEN, work_len);

    /* 2. check pke instr ready */
    ret = check_instr_rdy(mode);
    crypto_chk_func_return(check_instr_rdy, ret);

    /* 3. set calculate instruction */
    if (mode == PKE_SINGLE_INSTR0) {
        pke_reg_write(PKE_INSTR0, single_instr);
        /* call back for secure enhancement */
        reg_callback_chk(PKE_INSTR0, single_instr);
    } else if (mode == PKE_SINGLE_INSTR1) {
        pke_reg_write(PKE_INSTR1, single_instr);
        /* call back for secure enhancement */
        reg_callback_chk(PKE_INSTR1, single_instr);
    } else if (mode == PKE_BATCH_INSTR) {
        pke_reg_write(PKE_INSTR_ADDR_LOW, batch_instr_addr);
        /* call back for secure enhancement */
        reg_callback_chk(PKE_INSTR_ADDR_LOW, batch_instr_addr);
        /* WARN: need to set. */
        pke_reg_write(PKE_INSTR_ADDR_HIG, 0);
        /* call back for secure enhancement */
        reg_callback_chk(PKE_INSTR_ADDR_HIG, 0);
        /* here the PKE_INSTR_LEN should be set in byte, and should be aligned to 4 bytes. And eatch instr
        will occupy 4 bytes. */
        pke_reg_write(PKE_INSTR_LEN, batch_instr_num);
        /* call back for secure enhancement */
        reg_callback_chk(PKE_INSTR_LEN, batch_instr_num);
    }

    crypto_hal_func_exit();
    return TD_SUCCESS;
}

td_s32 hal_pke_start(pke_mode mode CIPHER_CHECK_WORD)
{
    crypto_hal_func_enter();
    /* secure enhancement */
    check_sum_assert(mode);

    switch (mode) {
#if defined(CONFIG_PKE_SINGLE_INSTR_SUPPORT)
        case PKE_SINGLE_INSTR0: {
            hal_pke_start0();
            break;
        }
        case PKE_SINGLE_INSTR1: {
            hal_pke_start1();
            break;
        }
#endif
        case PKE_BATCH_INSTR: {
            hal_pke_batch_start();
            break;
        }
        default:
            crypto_log_err("error, pke_instr_start_mode invaild!\n");
            return TD_FAILURE;
    }
    crypto_hal_func_exit();
    return TD_SUCCESS;
}

td_s32 hal_pke_wait_done(void)
{
    td_s32 ret = TD_FAILURE;
    crypto_hal_func_enter();

    /* wait ready */
    ret = hal_pke_wait_free();
    if (ret != TD_SUCCESS) {
        return ret;
    }

    if (hal_pke_check_robust_warn() == TD_SUCCESS) {
        return PKE_COMPAT_ERRNO(ERROR_PKE_ROBUST_WARNING);
    }

    ret = hal_pke_error_code();
    crypto_chk_func_return(hal_pke_error_code, ret);

    crypto_hal_func_exit();
    return TD_SUCCESS;
}

td_s32 hal_pke_get_align_val(const td_u32 len, td_u32 *aligned_len)
{
    unsigned int i;
    td_u32 aligned_len_list[] = {
        DRV_PKE_LEN_192, DRV_PKE_LEN_256, DRV_PKE_LEN_384, DRV_PKE_LEN_512, DRV_PKE_LEN_576,
        DRV_PKE_LEN_1024, DRV_PKE_LEN_1536, DRV_PKE_LEN_2048, DRV_PKE_LEN_3072, DRV_PKE_LEN_4096
    };
    crypto_hal_func_enter();
    crypto_chk_return(aligned_len == TD_NULL, PKE_COMPAT_ERRNO(ERROR_PARAM_IS_NULL), "aligned_len is NULL\n");
    for (i = 0; i < crypto_array_size(aligned_len_list); i++) {
        if (len <= aligned_len_list[i]) {
            *aligned_len = aligned_len_list[i];
            crypto_hal_func_exit();
            return TD_SUCCESS;
        }
    }

    crypto_hal_func_exit();
    return TD_FAILURE;
}

td_s32 hal_pke_set_mont_para(td_u32 low_bit, td_u32 high_bit CIPHER_CHECK_WORD)
{
    crypto_hal_func_enter();
    check_sum_inspect(PKE_COMPAT_ERRNO(ERROR_INVALID_PARAM), low_bit, high_bit);

    /* set low 32bit data */
    pke_reg_write(PKE_MONT_PARA0, low_bit);
    /* call back for secure enhancement */
    reg_callback_chk(PKE_MONT_PARA0, low_bit);

    /* set high 32bit data */
    pke_reg_write(PKE_MONT_PARA1, high_bit);
    /* call back for secure enhancement */
    reg_callback_chk(PKE_MONT_PARA1, high_bit);

    crypto_hal_func_exit();
    return TD_SUCCESS;
}

#ifdef CONFIG_PKE_SUPPORT_CURVE
td_s32 hal_pke_set_curve_param(const pke_ecc_init_param *init_param, const drv_pke_ecc_curve *ecc_curve)
{
    td_s32 ret = TD_FAILURE;
    td_u32 aligned_len = ecc_curve->ksize;
    crypto_hal_func_enter();
    crypto_chk_return(ecc_curve->ecc_type != DRV_PKE_ECC_TYPE_RFC7748 &&
        ecc_curve->ecc_type != DRV_PKE_ECC_TYPE_RFC7748_448,
        PKE_COMPAT_ERRNO(ERROR_INVALID_PARAM), "ecc_type is invalid\n");

    if (ecc_curve->ksize == DRV_PKE_LEN_448) {
        aligned_len = DRV_PKE_LEN_448;
    } else {
        ret = hal_pke_get_align_val(ecc_curve->ksize, &aligned_len);
        crypto_chk_return((ret != TD_SUCCESS), ret, "hal_pke_get_align_val failed!\n");
    }
    hal_pke_set_ram(sec_arg_add_cs(curve_addr_n, ecc_curve->n, ecc_curve->ksize, aligned_len));
    hal_pke_set_ram(sec_arg_add_cs(curve_addr_p, ecc_curve->p, ecc_curve->ksize, aligned_len));
    hal_pke_set_ram(sec_arg_add_cs(curve_addr_rrn, init_param->rrn, ecc_curve->ksize, aligned_len));
    hal_pke_set_ram(sec_arg_add_cs(curve_addr_rrp, init_param->rrp, ecc_curve->ksize, aligned_len));
    hal_pke_set_ram(sec_arg_add_cs(curve_addr_const_0, init_param->const_0, ecc_curve->ksize, aligned_len));
    hal_pke_set_ram(sec_arg_add_cs(curve_addr_const_1, init_param->const_1, ecc_curve->ksize, aligned_len));
    hal_pke_set_ram(sec_arg_add_cs(curve_addr_mont_a24, init_param->mont_a, ecc_curve->ksize, aligned_len));
    hal_pke_set_ram(sec_arg_add_cs(curve_addr_mont_1_p, init_param->mont_1_p, ecc_curve->ksize, aligned_len));
    hal_pke_set_ram(sec_arg_add_cs(curve_addr_mont_1_n, init_param->mont_1_n, ecc_curve->ksize, aligned_len));

    crypto_hal_func_exit();
    return TD_SUCCESS;
}
#endif

#ifdef CONFIG_PKE_SUPPORT_EDWARD
td_s32 hal_pke_set_ed_param(const pke_ecc_init_param *init_param, const drv_pke_ecc_curve *ecc_curve)
{
    td_s32 ret = TD_FAILURE;
    td_u32 aligned_len = ecc_curve->ksize;
    crypto_chk_return(ecc_curve->ecc_type != DRV_PKE_ECC_TYPE_RFC8032, PKE_COMPAT_ERRNO(ERROR_INVALID_PARAM),
        "ecc_type is invalid\n");

    ret = hal_pke_get_align_val(ecc_curve->ksize, &aligned_len);
    crypto_chk_return((ret != TD_SUCCESS), ret, "hal_pke_get_align_val failed!\n");
    hal_pke_set_ram(sec_arg_add_cs(ed_addr_n, ecc_curve->n, ecc_curve->ksize, aligned_len));
    hal_pke_set_ram(sec_arg_add_cs(ed_addr_p, ecc_curve->p, ecc_curve->ksize, aligned_len));
    hal_pke_set_ram(sec_arg_add_cs(ed_addr_rrn, init_param->rrn, ecc_curve->ksize, aligned_len));
    hal_pke_set_ram(sec_arg_add_cs(ed_addr_rrp, init_param->rrp, ecc_curve->ksize, aligned_len));
    hal_pke_set_ram(sec_arg_add_cs(ed_addr_const_0, init_param->const_0, ecc_curve->ksize, aligned_len));
    hal_pke_set_ram(sec_arg_add_cs(ed_addr_const_1, init_param->const_1, ecc_curve->ksize, aligned_len));
    hal_pke_set_ram(sec_arg_add_cs(ed_addr_mont_d, init_param->mont_a, ecc_curve->ksize, aligned_len));
    hal_pke_set_ram(sec_arg_add_cs(ed_addr_sqrt_m1, init_param->mont_b, ecc_curve->ksize, aligned_len));
    hal_pke_set_ram(sec_arg_add_cs(ed_addr_mont_1_p, init_param->mont_1_p, ecc_curve->ksize, aligned_len));
    hal_pke_set_ram(sec_arg_add_cs(ed_addr_mont_1_n, init_param->mont_1_n, ecc_curve->ksize, aligned_len));
    return TD_SUCCESS;
}
#endif

#ifdef CONFIG_PKE_ECC_SUPPORT
td_s32 hal_pke_set_ecc_param(const pke_ecc_init_param *init_param, const drv_pke_ecc_curve *ecc_curve)
{
    td_s32 ret = TD_FAILURE;
    td_u32 aligned_len = ecc_curve->ksize;
    crypto_hal_func_enter();

    ret = hal_pke_get_align_val(ecc_curve->ksize, &aligned_len);
    crypto_chk_return((ret != TD_SUCCESS), ret, "hal_pke_get_align_val failed!\n");
    hal_pke_set_ram(sec_arg_add_cs(ecc_addr_n, ecc_curve->n, ecc_curve->ksize, aligned_len));
    hal_pke_set_ram(sec_arg_add_cs(ecc_addr_p, ecc_curve->p, ecc_curve->ksize, aligned_len));
    hal_pke_set_ram(sec_arg_add_cs(ecc_addr_rrn, init_param->rrn, ecc_curve->ksize, aligned_len));
    hal_pke_set_ram(sec_arg_add_cs(ecc_addr_rrp, init_param->rrp, ecc_curve->ksize, aligned_len));
    hal_pke_set_ram(sec_arg_add_cs(ecc_addr_const_0, init_param->const_0, ecc_curve->ksize, aligned_len));
    hal_pke_set_ram(sec_arg_add_cs(ecc_addr_const_1, init_param->const_1, ecc_curve->ksize, aligned_len));
    hal_pke_set_ram(sec_arg_add_cs(ecc_addr_mont_a, init_param->mont_a, ecc_curve->ksize, aligned_len));
    hal_pke_set_ram(sec_arg_add_cs(ecc_addr_mont_b, init_param->mont_b, ecc_curve->ksize, aligned_len));
    hal_pke_set_ram(sec_arg_add_cs(ecc_addr_mont_1_p, init_param->mont_1_p, ecc_curve->ksize, aligned_len));
    hal_pke_set_ram(sec_arg_add_cs(ecc_addr_mont_1_n, init_param->mont_1_n, ecc_curve->ksize, aligned_len));
    crypto_hal_func_exit();
    return TD_SUCCESS;
}
#endif

td_s32 hal_pke_set_init_param(const pke_ecc_init_param *init_param, const drv_pke_ecc_curve *ecc_curve)
{
    td_s32 ret = TD_FAILURE;
    crypto_hal_func_enter();

    crypto_unused(init_param);

    switch (ecc_curve->ecc_type) {
#ifdef CONFIG_PKE_SUPPORT_CURVE
        case DRV_PKE_ECC_TYPE_RFC7748:
        case DRV_PKE_ECC_TYPE_RFC7748_448:
            ret = hal_pke_set_curve_param(init_param, ecc_curve);
            break;
#endif
#ifdef CONFIG_PKE_SUPPORT_EDWARD
        case DRV_PKE_ECC_TYPE_RFC8032:
            ret = hal_pke_set_ed_param(init_param, ecc_curve);
            break;
#endif
#ifdef CONFIG_PKE_ECC_SUPPORT
        case DRV_PKE_ECC_TYPE_RFC5639_P256:
        case DRV_PKE_ECC_TYPE_RFC5639_P384:
        case DRV_PKE_ECC_TYPE_RFC5639_P512:
        case DRV_PKE_ECC_TYPE_FIPS_P192R:
        case DRV_PKE_ECC_TYPE_FIPS_P224R:
        case DRV_PKE_ECC_TYPE_FIPS_P256R:
        case DRV_PKE_ECC_TYPE_FIPS_P384R:
        case DRV_PKE_ECC_TYPE_FIPS_P521R:
        case DRV_PKE_ECC_TYPE_SM2:
            ret = hal_pke_set_ecc_param(init_param, ecc_curve);
            break;
#endif
        default:
            crypto_log_err("invalid ecc_type!\n");
            ret = PKE_COMPAT_ERRNO(ERROR_INVALID_PARAM);
            break;
    }

    crypto_hal_func_exit();
    return ret;
}