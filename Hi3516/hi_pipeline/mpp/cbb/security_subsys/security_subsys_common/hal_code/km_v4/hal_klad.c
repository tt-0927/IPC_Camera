/**
  Copyright (c), 2001-2025, Shenshu Tech. Co., Ltd.
 */

#include "hal_klad.h"
#include "hal_klad_reg.h"

#include "crypto_drv_common.h"
#include "crypto_common_macro.h"

#define CRYPTO_U8_TO_U32_BIT_SHIFT(data) \
    ((td_u32)(data)[i]  | ((td_u32)(data)[i+1] << 8) | ((td_u32)(data)[i+2] << 16) | ((td_u32)(data)[i+3]<< 24))

#define HKL_COM_LOCK_STAT_TEE_LOCK      0xa5
#define HKL_COM_LOCK_STAT_REE_LOCK      0xaa

#define KM_COMPAT_ERRNO(err_code)     HAL_COMPAT_ERRNO(ERROR_MODULE_KM, err_code)

static td_s32 inner_klad_to_be_lock(td_void)
{
    td_u32 lock_stat_expected = 0;
    hkl_lock_ctrl lock_ctl = {0};
    hkl_com_lock_info lock_info = {0};
    hkl_com_lock_status lock_status = {0};
    crypto_cpu_type cpu_type = crypto_get_cpu_type();

    switch (cpu_type) {
        case CRYPTO_CPU_TYPE_ACPU:
            lock_stat_expected = HKL_COM_LOCK_STAT_REE_LOCK;
            break;
        case CRYPTO_CPU_TYPE_SCPU:
            lock_stat_expected = HKL_COM_LOCK_STAT_TEE_LOCK;
            break;
        default:
            crypto_log_err("invalid cpu_type\n");
            return KM_COMPAT_ERRNO(ERROR_INVALID_CPU_TYPE);
    }

    lock_ctl.bits.kl_lock = 0x1;
    km_reg_write(KL_LOCK_CTRL, lock_ctl.u32);

    lock_info.u32 = km_reg_read(KL_COM_LOCK_INFO);
    if (lock_info.bits.kl_com_lock_fail == 0x1) {
        return KM_COMPAT_ERRNO(ERROR_KLAD_LOCK);
    }

    lock_status.u32 = km_reg_read(KL_COM_LOCK_STATUS);
    if (lock_status.bits.kl_com_lock_stat != lock_stat_expected) {
        return KM_COMPAT_ERRNO(ERROR_KLAD_LOCK);
    }

    return TD_SUCCESS;
}

#define HKL_LOCK_TIMEOUT        10000
td_s32 hal_klad_lock(td_void)
{
    volatile td_s32 ret = TD_FAILURE;
    td_u32 i = 0;
    for (i = 0; i < HKL_LOCK_TIMEOUT; i++) {
        ret = inner_klad_to_be_lock();
        if (ret == TD_SUCCESS) {
            break;
        }

        crypto_udelay(1);
    }

    if (i >= HKL_LOCK_TIMEOUT) {
        crypto_log_err("klad is busy, lock failed\n");
        return KM_COMPAT_ERRNO(ERROR_KLAD_TIMEOUT);
    }

    return TD_SUCCESS;
}

td_s32 hal_klad_unlock(td_void)
{
    hkl_unlock_ctrl unlock_ctl = {0};
    hkl_com_lock_info lock_info = {0};
    hkl_com_lock_status lock_status = {0};
    td_u32 lock_stat_expected = 0;

    crypto_cpu_type cpu_type = crypto_get_cpu_type();
    switch (cpu_type) {
        case CRYPTO_CPU_TYPE_ACPU:
            lock_stat_expected = HKL_COM_LOCK_STAT_REE_LOCK;
            break;
        case CRYPTO_CPU_TYPE_SCPU:
            lock_stat_expected = HKL_COM_LOCK_STAT_TEE_LOCK;
            break;
        default:
            crypto_log_err("invalid cpu_type\n");
            return KM_COMPAT_ERRNO(ERROR_INVALID_CPU_TYPE);
    }

    lock_status.u32 = km_reg_read(KL_COM_LOCK_STATUS);
    if (lock_status.bits.kl_com_lock_stat != lock_stat_expected || lock_status.bits.kl_com_lock_stat == 0) {
        return TD_SUCCESS;
    }

    unlock_ctl.bits.kl_unlock = 0x1;
    km_reg_write(KL_UNLOCK_CTRL, unlock_ctl.u32);

    lock_info.u32 = km_reg_read(KL_COM_LOCK_INFO);
    if (lock_info.bits.kl_com_unlock_fail == 0x1) {
        return KM_COMPAT_ERRNO(ERROR_KLAD_LOCK);
    }

    return TD_SUCCESS;
}

#define HKL_ENGINE_AES          0x20
#define HKL_ENGINE_SM4          0x50
#define HKL_ENGINE_SHA1_HMAC    0xa0
#define HKL_ENGINE_SHA2_HMAC    0xa1
#define HKL_ENGINE_SM3_HMAC     0xa2

td_s32 hal_klad_set_key_crypto_cfg(td_bool encrypt_support, td_bool decrypt_support, crypto_klad_engine engine)
{
    hkl_key_cfg key_cfg = {0};
    td_u32 alg_engine_reg_value = 0;

    switch (engine) {
        case CRYPTO_KLAD_ENGINE_AES:
            alg_engine_reg_value = HKL_ENGINE_AES;
            break;
        case CRYPTO_KLAD_ENGINE_SM4:
            alg_engine_reg_value = HKL_ENGINE_SM4;
            break;
        case CRYPTO_KLAD_ENGINE_SHA1_HMAC:
            alg_engine_reg_value = HKL_ENGINE_SHA1_HMAC;
            break;
        case CRYPTO_KLAD_ENGINE_SHA2_HMAC:
            alg_engine_reg_value = HKL_ENGINE_SHA2_HMAC;
            break;
        case CRYPTO_KLAD_ENGINE_SM3_HMAC:
            alg_engine_reg_value = HKL_ENGINE_SM3_HMAC;
            break;
        default:
            crypto_log_err("invalid engine\n");
            return KM_COMPAT_ERRNO(ERROR_INVALID_PARAM);
    }

    key_cfg.u32 = km_reg_read(KL_KEY_CFG);
    key_cfg.bits.key_dec = decrypt_support;
    key_cfg.bits.key_enc = encrypt_support;
    key_cfg.bits.dsc_code = alg_engine_reg_value;

    km_reg_write(KL_KEY_CFG, key_cfg.u32);

    return TD_SUCCESS;
}

#define HKL_PORT_SEL_MCIPHER        0x1
#define HKL_PORT_SEL_HMAC           0x1 /* the sample as MCIPHER. */
#define HKL_PORT_SEL_NPU            0x5
#define HKL_PORT_SEL_FLASH          0x7

#define HKL_FLASH_SEL_REE_DEC       0

td_s32 hal_klad_set_key_dest_cfg(crypto_klad_dest dest, crypto_klad_flash_key_type flash_key_type)
{
    hkl_key_cfg key_cfg = {0};
    td_u32 port_sel_reg_val = 0;
    td_u32 flash_sel_reg_val = 0;

    switch (dest) {
        case CRYPTO_KLAD_DEST_MCIPHER:
            port_sel_reg_val = HKL_PORT_SEL_MCIPHER;
            break;
        case CRYPTO_KLAD_DEST_HMAC:
            port_sel_reg_val = HKL_PORT_SEL_HMAC;
            break;
        case CRYPTO_KLAD_DEST_FLASH:
            port_sel_reg_val = HKL_PORT_SEL_FLASH;
            break;
        case CRYPTO_KLAD_DEST_NPU:
            port_sel_reg_val = HKL_PORT_SEL_NPU;
            break;
        default:
            crypto_log_err("invalid dest\n");
            return KM_COMPAT_ERRNO(ERROR_INVALID_PARAM);
    }
    if (dest == CRYPTO_KLAD_DEST_FLASH) {
        switch (flash_key_type) {
            case CRYPTO_KLAD_FLASH_KEY_TYPE_REE_DEC:
                flash_sel_reg_val = HKL_FLASH_SEL_REE_DEC;
                break;
            default:
                crypto_log_err("invalid flash_key_type\n");
                return KM_COMPAT_ERRNO(ERROR_INVALID_PARAM);
        }
    }

    key_cfg.u32 = km_reg_read(KL_KEY_CFG);
    key_cfg.bits.port_sel = port_sel_reg_val;
    if (dest == CRYPTO_KLAD_DEST_FLASH) {
        key_cfg.bits.kl_flash_sel = flash_sel_reg_val;
    }
    km_reg_write(KL_KEY_CFG, key_cfg.u32);

    return TD_SUCCESS;
}

td_s32 hal_klad_set_key_secure_cfg(const crypto_klad_key_secure_config *secure_cfg)
{
    hkl_key_sec_cfg sec_cfg = {0};

    km_null_ptr_chk(secure_cfg);

    sec_cfg.u32 = km_reg_read(KL_KEY_SEC_CFG);
    sec_cfg.bits.master_only = secure_cfg->master_only_enable;
    sec_cfg.bits.dest_sec = secure_cfg->dest_buf_sec_support;
    sec_cfg.bits.dest_nsec = secure_cfg->dest_buf_non_sec_support;
    sec_cfg.bits.src_sec = secure_cfg->src_buf_sec_support;
    sec_cfg.bits.src_nsec = secure_cfg->src_buf_non_sec_support;
    sec_cfg.bits.key_sec = secure_cfg->key_sec;
    km_reg_write(KL_KEY_SEC_CFG, sec_cfg.u32);

    return TD_SUCCESS;
}

td_s32 hal_klad_set_key_addr(crypto_klad_dest klad_dest, td_u32 keyslot_chn)
{
    hkl_key_addr key_addr = {0};

    if (klad_dest == CRYPTO_KLAD_DEST_MCIPHER) {    /* symc */
        key_addr.u32 = km_reg_read(KL_KEY_ADDR);
        key_addr.bits.key_addr = (keyslot_chn << 1);
        km_reg_write(KL_KEY_ADDR, key_addr.u32);
    } else if (klad_dest == CRYPTO_KLAD_DEST_HMAC || klad_dest == CRYPTO_KLAD_DEST_NPU) { /* hmac & NPU */
        key_addr.u32 = km_reg_read(KL_KEY_ADDR);
        key_addr.bits.key_addr = keyslot_chn;
        km_reg_write(KL_KEY_ADDR, key_addr.u32);
    } else {
        crypto_log_err("invalid klad_dest\n");
        return KM_COMPAT_ERRNO(ERROR_INVALID_PARAM);
    }

    return TD_SUCCESS;
}

td_void hal_klad_clear_data(td_void)
{
    td_u32 i = 0;
    td_u32 clear_key_val = 0;

    for (i = 0; i < HKL_KEY_LEN / CRYPTO_WORD_WIDTH; i += CRYPTO_WORD_WIDTH) {
        km_reg_write(KL_DATA_IN_0 + i, clear_key_val);
    }
}

td_s32 hal_klad_set_data(const td_u8 *data, td_u32 data_length)
{
    td_u32 i = 0;
    td_u32 data_word = 0;
    if (data_length == 0) { /* do nothing. */
        return TD_SUCCESS;
    }

    km_null_ptr_chk(data);
    crypto_chk_return(data_length > HKL_KEY_LEN, KM_COMPAT_ERRNO(ERROR_INVALID_PARAM), "invalid data_length\n");
    crypto_chk_return((data_length % CRYPTO_WORD_WIDTH) != 0, KM_COMPAT_ERRNO(ERROR_INVALID_PARAM),
        "data_length must be aligned to 4-byte\n");
    for (i = 0; i < HKL_KEY_LEN; i += CRYPTO_WORD_WIDTH) {
        if (i < data_length) {
            data_word = CRYPTO_U8_TO_U32_BIT_SHIFT(data);
            km_reg_write(KL_DATA_IN_0 + i, data_word);
        } else {
            km_reg_write(KL_DATA_IN_0 + i, 0);      /* padding with zero. */
        }
    }
    return TD_SUCCESS;
}

td_void hal_klad_set_key_odd(td_bool odd)
{
    hkl_key_addr key_addr = {0};
    key_addr.u32 = km_reg_read(KL_KEY_ADDR);
    if (odd) {
        key_addr.u32 |= 0x1;    /* set bit0. */
    } else {
        key_addr.u32 &= ~(0x1); /* clear bit0. */
    }
    km_reg_write(KL_KEY_ADDR, key_addr.u32);
}

td_s32 inner_klad_error_check(td_void)
{
    td_u32 klad_err = 0;
    td_u32 kctrl_err = 0;

    klad_err = km_reg_read(KL_ERROR);
    if (klad_err) {
        crypto_log_err("klad_err is 0x%x\n", klad_err);
    }
    kctrl_err = km_reg_read(KC_ERROR);
    if (kctrl_err) {
        crypto_log_err("kctrl_err is 0x%x\n", kctrl_err);
    }
    if (klad_err | kctrl_err) {
        return KM_COMPAT_ERRNO(ERROR_KM_LOGIC);
    }
    return TD_SUCCESS;
}

#define KLAD_COM_ROUTE_TIMEOUT          1000000
td_s32 hal_klad_wait_com_route_done(td_void)
{
    volatile td_s32 ret = TD_FAILURE;
    kl_com_ctrl com_ctrl = { 0 };
    kl_int_raw int_raw = {0};
    kl_com_status com_status = {0};
    td_u32 i = 0;

    for (i = 0; i < KLAD_COM_ROUTE_TIMEOUT; ++i) {
        com_ctrl.u32 = km_reg_read(KL_COM_CTRL);
        if (com_ctrl.bits.kl_com_start == 0) {
            int_raw.u32 = km_reg_read(KL_INT_RAW);
            int_raw.bits.com_kl_int_raw = 0x1;
            km_reg_write(KL_INT_RAW, int_raw.u32);
            break;
        }
        crypto_udelay(1);
    }
    if (i < KLAD_COM_ROUTE_TIMEOUT) {
        ret = TD_SUCCESS;
    }

    com_status.u32 = km_reg_read(KL_COM_STATUS);
    if (com_status.bits.kl_com_rk_rdy == 0) {
        crypto_log_err("root key is not ready!\n");
    }
#if defined(CONFIG_KM_CONTENT_KEY_SUPPORT)
    if (com_status.bits.kl_com_lv1_rdy == 0) {
        crypto_log_err("level 1 is not ready!\n");
    }
#endif
    ret = inner_klad_error_check();
    crypto_chk_return(ret != TD_SUCCESS, ret, "inner_klad_error_check failed\n");

    return ret;
}

#define HKL_COM_KEY_SIZE_128            0x1
#define HKL_COM_KEY_SIZE_192            0x2
#define HKL_COM_KEY_SIZE_256            0x3
static const crypto_table_item g_klad_key_size_table[] = {
    {
        .index = CRYPTO_KLAD_KEY_SIZE_128BIT, .value = HKL_COM_KEY_SIZE_128
    },
#if defined(CONFIG_KM_KEY_LEN_192_SUPPORT)
    {
        .index = CRYPTO_KLAD_KEY_SIZE_192BIT, .value = HKL_COM_KEY_SIZE_192
    },
#endif
#if defined(CONFIG_KM_KEY_LEN_256_SUPPORT)
    {
        .index = CRYPTO_KLAD_KEY_SIZE_256BIT, .value = HKL_COM_KEY_SIZE_256
    },
#endif
};

#define HKL_COM_RK_CHOOSE_ODRK0         0x5
#define HKL_COM_RK_CHOOSE_OARK0         0x6
#define HKL_COM_RK_CHOOSE_ODRK1         0x7
#define HKL_COM_RK_CHOOSE_ABRK_REE      0x15
#define HKL_COM_RK_CHOOSE_RDRK_REE      0x17

#define HKL_COM_RK_CHOOSE_SBRK0         0x0
#define HKL_COM_RK_CHOOSE_SBRK1         0x1
#define HKL_COM_RK_CHOOSE_ABRK0         0x2
#define HKL_COM_RK_CHOOSE_ABRK1         0x3
#define HKL_COM_RK_CHOOSE_DRK0          0xc
#define HKL_COM_RK_CHOOSE_DRK1          0xd
#define HKL_COM_RK_CHOOSE_RDRK0         0xe
#define HKL_COM_RK_CHOOSE_RDRK1         0xf
#define HKL_COM_RK_CHOOSE_SBRK2         0x10
#define HKL_COM_RK_CHOOSE_ABRK2         0x11
#define HKL_COM_RK_CHOOSE_MDRK0         0x8
#define HKL_COM_RK_CHOOSE_MDRK1         0x9
#define HKL_COM_RK_CHOOSE_MDRK2         0xa
#define HKL_COM_RK_CHOOSE_MDRK3         0xb
#define HKL_COM_RK_CHOOSE_PSK           0x12
#define HKL_COM_RK_CHOOSE_ERK_TEE          6
#define HKL_COM_RK_CHOOSE_ERK_REE          7
#define HKL_COM_RK_CHOOSE_ERK1_REE          16
#define HKL_COM_RK_CHOOSE_ERK2_REE          17

static const crypto_table_item g_klad_rk_table[] = {
#if defined(CONFIG_KM_ODRK0_SUPPORT)
    {
        .index = CRYPTO_KDF_HARD_KEY_TYPE_ODRK0, .value = HKL_COM_RK_CHOOSE_ODRK0
    },
#endif
#if defined(CONFIG_KM_OARK0_SUPPORT)
    {
        .index = CRYPTO_KDF_HARD_KEY_TYPE_OARK0, .value = HKL_COM_RK_CHOOSE_OARK0
    },
#endif
#if defined(CONFIG_KM_ODRK1_SUPPORT)
    {
        .index = CRYPTO_KDF_HARD_KEY_TYPE_ODRK1, .value = HKL_COM_RK_CHOOSE_ODRK1
    },
#endif
#if defined(CONFIG_KM_ABRK_REE_SUPPORT)
    {
        .index = CRYPTO_KDF_HARD_KEY_TYPE_ABRK_REE, .value = HKL_COM_RK_CHOOSE_ABRK_REE
    },
#endif
#if defined(CONFIG_KM_RDRK_REE_SUPPORT)
    {
        .index = CRYPTO_KDF_HARD_KEY_TYPE_RDRK_REE, .value = HKL_COM_RK_CHOOSE_RDRK_REE
    },
#endif
#if defined(CONFIG_KM_ABRK0_SUPPORT)
    {
        .index = CRYPTO_KDF_HARD_KEY_TYPE_ABRK0, .value = HKL_COM_RK_CHOOSE_ABRK0
    },
#endif
#if defined(CONFIG_KM_ABRK1_SUPPORT)
    {
        .index = CRYPTO_KDF_HARD_KEY_TYPE_ABRK1, .value = HKL_COM_RK_CHOOSE_ABRK1
    },
#endif
#if defined(CONFIG_KM_ABRK2_SUPPORT)
    {
        .index = CRYPTO_KDF_HARD_KEY_TYPE_ABRK2, .value = HKL_COM_RK_CHOOSE_ABRK2
    },
#endif
#if defined(CONFIG_KM_SBRK0_SUPPORT)
    {
        .index = CRYPTO_KDF_HARD_KEY_TYPE_SBRK0, .value = HKL_COM_RK_CHOOSE_SBRK0
    },
#endif
#if defined(CONFIG_KM_SBRK1_SUPPORT)
    {
        .index = CRYPTO_KDF_HARD_KEY_TYPE_SBRK1, .value = HKL_COM_RK_CHOOSE_SBRK1
    },
#endif
#if defined(CONFIG_KM_SBRK2_SUPPORT)
    {
        .index = CRYPTO_KDF_HARD_KEY_TYPE_SBRK2, .value = HKL_COM_RK_CHOOSE_SBRK2
    },
#endif
#if defined(CONFIG_KM_DRK0_SUPPORT)
    {
        .index = CRYPTO_KDF_HARD_KEY_TYPE_DRK0, .value = HKL_COM_RK_CHOOSE_DRK0
    },
#endif
#if defined(CONFIG_KM_DRK1_SUPPORT)
    {
        .index = CRYPTO_KDF_HARD_KEY_TYPE_DRK1, .value = HKL_COM_RK_CHOOSE_DRK1
    },
#endif
#if defined(CONFIG_KM_RDRK0_SUPPORT)
    {
        .index = CRYPTO_KDF_HARD_KEY_TYPE_RDRK0, .value = HKL_COM_RK_CHOOSE_RDRK0
    },
#endif
#if defined(CONFIG_KM_RDRK1_SUPPORT)
    {
        .index = CRYPTO_KDF_HARD_KEY_TYPE_RDRK1, .value = HKL_COM_RK_CHOOSE_RDRK1
    },
#endif
#if defined(CONFIG_KM_MDRK0_SUPPORT)
    {
        .index = CRYPTO_KDF_HARD_KEY_TYPE_MDRK0, .value = HKL_COM_RK_CHOOSE_MDRK0
    },
#endif
#if defined(CONFIG_KM_MDRK1_SUPPORT)
    {
        .index = CRYPTO_KDF_HARD_KEY_TYPE_MDRK1, .value = HKL_COM_RK_CHOOSE_MDRK1
    },
#endif
#if defined(CONFIG_KM_MDRK2_SUPPORT)
    {
        .index = CRYPTO_KDF_HARD_KEY_TYPE_MDRK2, .value = HKL_COM_RK_CHOOSE_MDRK2
    },
#endif
#if defined(CONFIG_KM_MDRK3_SUPPORT)
    {
        .index = CRYPTO_KDF_HARD_KEY_TYPE_MDRK3, .value = HKL_COM_RK_CHOOSE_MDRK3
    },
#endif
#if defined(CONFIG_KM_PSK_SUPPORT)
    {
        .index = CRYPTO_KDF_HARD_KEY_TYPE_PSK, .value = HKL_COM_RK_CHOOSE_PSK
    },
#endif
#if defined(CONFIG_KM_ERK_TEE_SUPPORT)
    {
        .index = CRYPTO_KDF_HARD_KEY_TYPE_ERK_TEE, .value = HKL_COM_RK_CHOOSE_ERK_TEE
    },
#endif
#if defined(CONFIG_KM_ERK_REE_SUPPORT)
    {
        .index = CRYPTO_KDF_HARD_KEY_TYPE_ERK_REE, .value = HKL_COM_RK_CHOOSE_ERK_REE
    },
#endif
#if defined(CONFIG_KM_ERK1_REE_SUPPORT)
    {
        .index = CRYPTO_KDF_HARD_KEY_TYPE_ERK1_REE, .value = HKL_COM_RK_CHOOSE_ERK1_REE
    },
#endif
#if defined(CONFIG_KM_ERK2_REE_SUPPORT)
    {
        .index = CRYPTO_KDF_HARD_KEY_TYPE_ERK2_REE, .value = HKL_COM_RK_CHOOSE_ERK2_REE
    }
#endif
};

#define HKL_COM_ALG_TDES_SEL_VAL    (0x0)
#define HKL_COM_ALG_AES_SEL_VAL     (0x1)
#define HKL_COM_ALG_SM4_SEL_VAL     (0x2)
static const crypto_table_item g_klad_alg_sel_table[] = {
#if defined(CONFIG_KM_TDES_SUPPORT)
    {
        .index = CRYPTO_KLAD_ALG_SEL_TDES, .value = HKL_COM_ALG_TDES_SEL_VAL
    },
#endif
    {
        .index = CRYPTO_KLAD_ALG_SEL_AES, .value = HKL_COM_ALG_AES_SEL_VAL
    },
#if defined(CONFIG_KM_SM4_SUPPORT)
    {
        .index = CRYPTO_KLAD_ALG_SEL_SM4, .value = HKL_COM_ALG_SM4_SEL_VAL
    },
#endif
};

#if defined(CONFIG_KM_CONTENT_KEY_SUPPORT)
td_s32 hal_klad_com_start(crypto_klad_level_sel level, crypto_klad_alg_sel alg, crypto_klad_key_size key_size,
    crypto_kdf_hard_key_type rk_type)
{
    volatile td_s32 ret = TD_FAILURE;
    kl_com_ctrl com_ctrl = {0};
    td_u32 key_size_reg_val = 0;
    td_u32 alg_sel_reg_val = 0;
    td_u32 rk_choose_reg_val = 0;

    ret = crypto_get_value_by_index(g_klad_key_size_table, crypto_array_size(g_klad_key_size_table),
        key_size, &key_size_reg_val);
    crypto_chk_return(ret != TD_SUCCESS, KM_COMPAT_ERRNO(ERROR_INVALID_PARAM), "get key_size failed\n");

    ret = crypto_get_value_by_index(g_klad_alg_sel_table, crypto_array_size(g_klad_alg_sel_table),
        alg, &alg_sel_reg_val);
    crypto_chk_return(ret != TD_SUCCESS, KM_COMPAT_ERRNO(ERROR_INVALID_PARAM), "get alg_sel failed\n");

    ret = crypto_get_value_by_index(g_klad_rk_table, crypto_array_size(g_klad_rk_table),
        rk_type, &rk_choose_reg_val);
    crypto_chk_return(ret != TD_SUCCESS, KM_COMPAT_ERRNO(ERROR_INVALID_PARAM), "get rootkey failed\n");

    com_ctrl.u32 = km_reg_read(KL_COM_CTRL);
    com_ctrl.bits.kl_com_key_size = key_size_reg_val; /* size */
    com_ctrl.bits.kl_com_alg_sel = alg_sel_reg_val;       /* alg */
    com_ctrl.bits.kl_com_level_sel = level;   /* level */
    com_ctrl.bits.rk_choose = rk_choose_reg_val;        /* rk_choose. */
    com_ctrl.bits.kl_com_start = 1;   /* start calculation */
    km_reg_write(KL_COM_CTRL, com_ctrl.u32);

    return TD_SUCCESS;
}
#endif

td_s32 hal_klad_start_com_route(crypto_kdf_hard_key_type rk_type, const crypto_klad_effective_key *content_key,
    crypto_klad_dest klad_dest)
{
    volatile td_s32 ret = TD_FAILURE;
    td_bool key_parity = TD_FALSE;
    td_u32 key_size_reg_val = 0;
    td_u32 rk_choose_reg_val = 0;
    kl_com_ctrl com_ctrl = {0};

    crypto_chk_return(content_key == TD_NULL, TD_FAILURE, "content_key is NULL\n");
    crypto_chk_return(content_key->salt == TD_NULL, TD_FAILURE, "content_key->salt is NULL\n");

    ret = crypto_get_value_by_index(g_klad_rk_table, crypto_array_size(g_klad_rk_table),
        rk_type, &rk_choose_reg_val);
    crypto_chk_return(ret != TD_SUCCESS, KM_COMPAT_ERRNO(ERROR_INVALID_PARAM), "get rootkey failed\n");

    ret = crypto_get_value_by_index(g_klad_key_size_table, crypto_array_size(g_klad_key_size_table),
        content_key->key_size, &key_size_reg_val);
    crypto_chk_return(ret != TD_SUCCESS, KM_COMPAT_ERRNO(ERROR_INVALID_PARAM), "get key_size failed\n");
    /*
     * If key_length is 16, the key_parity is passed by caller;
     * If key_length is 24/32, the high 128bit's key_parity is even,
        the low 128bit's(padding 0 for 192bit) key_parity is odd.
     */
    if (content_key->key_size == CRYPTO_KLAD_KEY_SIZE_128BIT) {
        key_parity = TD_FALSE;
    }
    /* config the high 128bit  */
    if (klad_dest == CRYPTO_KLAD_DEST_MCIPHER) {
        hal_klad_set_key_odd(key_parity);
    }

    /* config com_ctrl */
    com_ctrl.bits.kl_com_key_size = key_size_reg_val; /* size */
    com_ctrl.bits.kl_com_start = 0x1;   /* start calculation */
    com_ctrl.bits.rk_choose = rk_choose_reg_val; /* klad config rootkey type */
    km_reg_write(KL_COM_CTRL, com_ctrl.u32);

    ret = hal_klad_wait_com_route_done();
    crypto_chk_return(ret != TD_SUCCESS, ret, "hal_klad_wait_com_route_done failed\n");

    /* config the low 64bit/128bit */
    if (content_key->key_size != CRYPTO_KLAD_KEY_SIZE_128BIT) {
        if (klad_dest == CRYPTO_KLAD_DEST_MCIPHER) {
            hal_klad_set_key_odd(TD_TRUE);
        }
        /* config com_ctrl */
        com_ctrl.bits.kl_com_key_size = key_size_reg_val; /* size */
        com_ctrl.bits.rk_choose = rk_choose_reg_val; /* klad config rootkey type */
        com_ctrl.bits.kl_com_start = 0x1;   /* start calculation */
        km_reg_write(KL_COM_CTRL, com_ctrl.u32);

        ret = hal_klad_wait_com_route_done();
        crypto_chk_return(ret != TD_SUCCESS, ret, "hal_klad_wait_com_route_done failed\n");
    }

    return ret;
}

#if defined(CONFIG_KLAD_DEBUG_ENABLE)
const reg_item_t g_hkl_reg_table[] = {
    { "KL_DATA_IN_0",       KL_DATA_IN_0 },
    { "KL_DATA_IN_1",       KL_DATA_IN_1 },
    { "KL_DATA_IN_2",       KL_DATA_IN_2 },
    { "KL_DATA_IN_3",       KL_DATA_IN_3 },
    { "KL_KEY_ADDR",       KL_KEY_ADDR },
    { "KL_KEY_CFG",       KL_KEY_CFG },
    { "KL_KEY_SEC_CFG",       KL_KEY_SEC_CFG },
    { "KL_STATE",       KL_STATE },
    { "KL_CRC",       KL_CRC },
    { "KL_ERROR",       KL_ERROR },
    { "KC_ERROR",       KC_ERROR },
    { "KL_INT_EN",       KL_INT_EN },
    { "KL_INT_RAW",       KL_INT_RAW },
    { "KL_INT",       KL_INT },
    { "SBRK_DISABLE",       SBRK_DISABLE },
    { "ABRK_DISABLE",       ABRK_DISABLE },
    { "KL_RK_GEN_STATUS",       KL_RK_GEN_STATUS },
    { "KL_LOCK_CTRL",       KL_LOCK_CTRL },
    { "KL_UNLOCK_CTRL",       KL_UNLOCK_CTRL },
    { "KL_COM_LOCK_INFO",       KL_COM_LOCK_INFO },
    { "KL_COM_LOCK_STATUS",       KL_COM_LOCK_STATUS },
    { "KL_COM_CTRL",       KL_COM_CTRL },
    { "KL_COM_STATUS",       KL_COM_STATUS },
    { "KL_CLR_CTRL",       KL_CLR_CTRL },
    { "KL_ALARM_INFO",       KL_ALARM_INFO },
};

static td_u32 inner_km_reg_reg(td_u32 offset)
{
    return km_reg_read(offset);
}

td_void hal_klad_debug(td_void)
{
    crypto_reg_item_dump("HKL", (const reg_item_t *)&g_hkl_reg_table,
        crypto_array_size(g_hkl_reg_table), inner_km_reg_reg);
}
#endif