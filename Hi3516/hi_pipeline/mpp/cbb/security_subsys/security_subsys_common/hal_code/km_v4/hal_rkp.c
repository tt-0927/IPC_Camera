/**
  Copyright (c), 2001-2025, Shenshu Tech. Co., Ltd.
 */

#include "hal_rkp.h"
#include "hal_rkp_reg.h"
#include "crypto_drv_common.h"
#include "crypto_common_macro.h"

#define CRYPTO_U8_TO_U32_BIT_SHIFT(data, i) \
    ((td_u32)(data)[(i)*4]  | ((td_u32)(data)[(i)*4+1] << 8) | ((td_u32)(data)[(i)*4+2] << 16) | \
    ((td_u32)(data)[(i)*4+3]<< 24))

#define KM_COMPAT_ERRNO(err_code)     HAL_COMPAT_ERRNO(ERROR_MODULE_KM, err_code)

#define RKP_LOCK_TIMEOUT_IN_US      1000000
td_s32 hal_rkp_lock(td_void)
{
    td_u32 i = 0;
    rkp_lock lock_val = {0};
    td_u32 rkp_config_val = RKP_LOCK_CPU_REE;
    crypto_cpu_type cpu_type = crypto_get_cpu_type();
    if (cpu_type == CRYPTO_CPU_TYPE_SCPU) {
        rkp_config_val = RKP_LOCK_CPU_TEE;
    }

    for (i = 0; i < RKP_LOCK_TIMEOUT_IN_US; i++) {
        km_reg_write(RKP_LOCK, rkp_config_val);
        lock_val.u32 = km_reg_read(RKP_LOCK);
        if (lock_val.bits.km_lock_status == rkp_config_val) {
            break;
        }
        crypto_udelay(1);
    }

    if (i >= RKP_LOCK_TIMEOUT_IN_US) {
        crypto_log_err("drv_rkp_lock busy.\n");
        return KM_COMPAT_ERRNO(ERROR_RKP_LOCK_TIMEOUT);
    }

    return TD_SUCCESS;
}

td_s32 hal_rkp_unlock(void)
{
    km_reg_write(RKP_LOCK, RKP_LOCK_CPU_IDLE);
    return TD_SUCCESS;
}

#define DEOB_UPDATE_KEY_SEL_MRK1        0
#define DEOB_UPDATE_KEY_SEL_USK         1
#define DEOB_UPDATE_KEY_SEL_RUSK        2

#define DEOB_UPDATE_ALG_SEL_AES         0
#define DEOB_UPDATE_ALG_SEL_SM4         1

#define RKP_DEOB_UPDATE_TIMEOUT_IN_US      1000000
#define KDF_MAX_VAL_LENGTH      64
#define KDF_PADDING_VAL_LEN         64
#define KDF_PADDING_SALT_LEN         128
#define KDF_PADDING_KEY_LEN         128

/*
    pbkdf2_key_config
*/
#define KDF_SW_GEN              3

#define PBKDF2_ALG_SEL_SHA1     1
#define PBKDF2_ALG_SEL_SHA256   0
#define PBKDF2_ALG_SEL_SHA384   3
#define PBKDF2_ALG_SEL_SHA512   4
#define PBKDF2_ALG_SEL_SM3      5

#define RKP_WAIT_TIMEOUT_IN_US          1000000
td_s32 hal_rkp_kdf_wait_done(td_void)
{
    td_u32 i;
    volatile td_s32 ret = TD_FAILURE;
    td_u32 kdf_err = 0;
    rkp_cmd_cfg cmd_cfg = { 0 };

    for (i = 0; i < RKP_WAIT_TIMEOUT_IN_US; ++i) {
        cmd_cfg.u32 = km_reg_read(RKP_CMD_CFG);
        if (cmd_cfg.bits.sw_calc_req == 0x0) {
            km_reg_write(RKP_RAW_INT, 0x1);
            break;
        }
        crypto_udelay(1);
    }
    if (i >= RKP_WAIT_TIMEOUT_IN_US) {
        ret = KM_COMPAT_ERRNO(ERROR_RKP_CALC_TIMEOUT);
    } else {
        ret = TD_SUCCESS;
    }

    /* check kdf err. */
    kdf_err = km_reg_read(KDF_ERROR);
    if (kdf_err != 0) {
        crypto_log_err("kdf_err is 0x%x\n", kdf_err);
        ret = KM_COMPAT_ERRNO(ERROR_KM_LOGIC);
    }

    return ret;
}

#define PBKDF2_KEY_SEL_ABRK_REE     20
#define PBKDF2_KEY_SEL_RDRK_REE     22

#define PBKDF2_KEY_SEL_SBRK1        5
#define PBKDF2_KEY_SEL_SBRK2        16
#define PBKDF2_KEY_SEL_ABRK1        6
#define PBKDF2_KEY_SEL_ABRK2        17
#define PBKDF2_KEY_SEL_ODRK0        7
#define PBKDF2_KEY_SEL_ODRK1        8
#define PBKDF2_KEY_SEL_RDRK         9
#define PBKDF2_KEY_SEL_MDRK0        10
#define PBKDF2_KEY_SEL_FDRK         11
#define PBKDF2_KEY_SEL_MDRK1        12
#define PBKDF2_KEY_SEL_DRK          13
#define PBKDF2_KEY_SEL_MDRK2        14
#define PBKDF2_KEY_SEL_MDRK3        15
#define PBKDF2_KEY_SEL_SBRK2        16
#define PBKDF2_KEY_SEL_ABRK2        17
#define PBKDF2_KEY_SEL_PSK          18

#define PBKDF2_KEY_ERK_TEE          6
#define PBKDF2_KEY_ERK_REE          7
#define PBKDF2_KEY_ERK1_REE          16
#define PBKDF2_KEY_ERK2_REE          17
static const crypto_table_item g_rkp_key_sel_table[] = {
#if defined(CONFIG_KM_ODRK0_SUPPORT)
    {
        .index = CRYPTO_KDF_HARD_KEY_TYPE_ODRK0, .value = PBKDF2_KEY_SEL_ODRK0
    },
#endif
#if defined(CONFIG_KM_OARK0_SUPPORT)
    {
        .index = CRYPTO_KDF_HARD_KEY_TYPE_OARK0, .value = PBKDF2_KEY_SEL_ODRK0
    },
#endif
#if defined(CONFIG_KM_ODRK1_SUPPORT)
    {
        .index = CRYPTO_KDF_HARD_KEY_TYPE_ODRK1, .value = PBKDF2_KEY_SEL_ODRK1
    },
#endif
#if defined(CONFIG_KM_ABRK_REE_SUPPORT)
    {
        .index = CRYPTO_KDF_HARD_KEY_TYPE_ABRK_REE, .value = PBKDF2_KEY_SEL_ABRK_REE
    },
#endif
#if defined(CONFIG_KM_RDRK_REE_SUPPORT)
    {
        .index = CRYPTO_KDF_HARD_KEY_TYPE_RDRK_REE, .value = PBKDF2_KEY_SEL_RDRK_REE
    },
#endif
#if defined(CONFIG_KM_SBRK0_SUPPORT)
    {
        .index = CRYPTO_KDF_HARD_KEY_TYPE_SBRK0, .value = PBKDF2_KEY_SEL_SBRK1
    },
#endif
#if defined(CONFIG_KM_SBRK1_SUPPORT)
    {
        .index = CRYPTO_KDF_HARD_KEY_TYPE_SBRK1, .value = PBKDF2_KEY_SEL_SBRK1
    },
#endif
#if defined(CONFIG_KM_SBRK2_SUPPORT)
    {
        .index = CRYPTO_KDF_HARD_KEY_TYPE_SBRK2, .value = PBKDF2_KEY_SEL_SBRK2
    },
#endif
#if defined(CONFIG_KM_ABRK0_SUPPORT)
    {
        .index = CRYPTO_KDF_HARD_KEY_TYPE_ABRK0, .value = PBKDF2_KEY_SEL_ABRK1
    },
#endif
#if defined(CONFIG_KM_ABRK1_SUPPORT)
    {
        .index = CRYPTO_KDF_HARD_KEY_TYPE_ABRK1, .value = PBKDF2_KEY_SEL_ABRK1
    },
#endif
#if defined(CONFIG_KM_ABRK2_SUPPORT)
    {
        .index = CRYPTO_KDF_HARD_KEY_TYPE_ABRK2, .value = PBKDF2_KEY_SEL_ABRK2
    },
#endif
#if defined(CONFIG_KM_DRK0_SUPPORT)
    {
        .index = CRYPTO_KDF_HARD_KEY_TYPE_DRK0, .value = PBKDF2_KEY_SEL_DRK
    },
#endif
#if defined(CONFIG_KM_DRK1_SUPPORT)
    {
        .index = CRYPTO_KDF_HARD_KEY_TYPE_DRK1, .value = PBKDF2_KEY_SEL_DRK
    },
#endif
#if defined(CONFIG_KM_RDRK0_SUPPORT)
    {
        .index = CRYPTO_KDF_HARD_KEY_TYPE_RDRK0, .value = PBKDF2_KEY_SEL_RDRK
    },
#endif
#if defined(CONFIG_KM_RDRK1_SUPPORT)
    {
        .index = CRYPTO_KDF_HARD_KEY_TYPE_RDRK1, .value = PBKDF2_KEY_SEL_RDRK
    },
#endif
#if defined(CONFIG_KM_MDRK0_SUPPORT)
    {
        .index = CRYPTO_KDF_HARD_KEY_TYPE_MDRK0, .value = PBKDF2_KEY_SEL_MDRK0
    },
#endif
#if defined(CONFIG_KM_MDRK1_SUPPORT)
    {
        .index = CRYPTO_KDF_HARD_KEY_TYPE_MDRK1, .value = PBKDF2_KEY_SEL_MDRK1
    },
#endif
#if defined(CONFIG_KM_MDRK2_SUPPORT)
    {
        .index = CRYPTO_KDF_HARD_KEY_TYPE_MDRK2, .value = PBKDF2_KEY_SEL_MDRK2
    },
#endif
#if defined(CONFIG_KM_MDRK3_SUPPORT)
    {
        .index = CRYPTO_KDF_HARD_KEY_TYPE_MDRK3, .value = PBKDF2_KEY_SEL_MDRK3
    },
#endif
#if defined(CONFIG_KM_PSK_SUPPORT)
    {
        .index = CRYPTO_KDF_HARD_KEY_TYPE_PSK, .value = PBKDF2_KEY_SEL_PSK
    },
#endif
#if defined(CONFIG_KM_ERK_TEE_SUPPORT)
    {
        .index = CRYPTO_KDF_HARD_KEY_TYPE_ERK_TEE, .value = PBKDF2_KEY_ERK_TEE
    },
#endif
#if defined(CONFIG_KM_ERK_REE_SUPPORT)
    {
        .index = CRYPTO_KDF_HARD_KEY_TYPE_ERK_REE, .value = PBKDF2_KEY_ERK_REE
    },
#endif
#if defined(CONFIG_KM_ERK1_REE_SUPPORT)
    {
        .index = CRYPTO_KDF_HARD_KEY_TYPE_ERK1_REE, .value = PBKDF2_KEY_ERK1_REE
    },
#endif
#if defined(CONFIG_KM_ERK2_REE_SUPPORT)
    {
        .index = CRYPTO_KDF_HARD_KEY_TYPE_ERK2_REE, .value = PBKDF2_KEY_ERK2_REE
    }
#endif
};

#define PBKDF2_KEY_LEN_192BIT       0
#define PBKDF2_KEY_LEN_128BIT       1
#define PBKDF2_KEY_LEN_256BIT       2
static const crypto_table_item g_rkp_key_len_table[] = {
    {
        .index = CRYPTO_KDF_HARD_KEY_SIZE_128BIT, .value = PBKDF2_KEY_LEN_128BIT
    },
#if defined(CONFIG_KM_KEY_LEN_192_SUPPORT)
    {
        .index = CRYPTO_KDF_HARD_KEY_SIZE_192BIT, .value = PBKDF2_KEY_LEN_192BIT
    },
#endif
#if defined(CONFIG_KM_KEY_LEN_256_SUPPORT)
    {
        .index = CRYPTO_KDF_HARD_KEY_SIZE_256BIT, .value = PBKDF2_KEY_LEN_256BIT
    },
#endif
};

#define PBKDF2_RDRK_REE_ONEWAY_OFFSET   0
#define PBKDF2_ABRK_REE_ONEWAY_OFFSET   1
#define PBKDF2_ODRK1_ONEWAY_OFFSET      2
#define PBKDF2_ERK_REE_ONEWAY_OFFSET    0
#define PBKDF2_ERK_TEE_ONEWAY_OFFSET    1
#define PBKDF2_ERK1_REE_ONEWAY_OFFSET    0
#define PBKDF2_ERK2_REE_ONEWAY_OFFSET    0

#if defined(CRYPTO_IOT_V200)
static const crypto_table_item g_rkp_oneway_offset_table[] = {
#if defined(CONFIG_KM_RDRK_REE_SUPPORT)
    {
        .index = CRYPTO_KDF_HARD_KEY_TYPE_RDRK_REE, .value = PBKDF2_RDRK_REE_ONEWAY_OFFSET
    },
#endif
#if defined(CONFIG_KM_ABRK_REE_SUPPORT)
    {
        .index = CRYPTO_KDF_HARD_KEY_TYPE_ABRK_REE, .value = PBKDF2_ABRK_REE_ONEWAY_OFFSET
    },
#endif
#if defined(CONFIG_KM_ODRK1_SUPPORT)
    {
        .index = CRYPTO_KDF_HARD_KEY_TYPE_ODRK1, .value = PBKDF2_ODRK1_ONEWAY_OFFSET
    },
#endif
#if defined(CONFIG_KM_ERK_REE_SUPPORT)
    {
        .index = CRYPTO_KDF_HARD_KEY_TYPE_ERK_REE, .value = PBKDF2_ERK_REE_ONEWAY_OFFSET
    },
#endif
#if defined(CONFIG_KM_ERK_TEE_SUPPORT)
    {
        .index = CRYPTO_KDF_HARD_KEY_TYPE_ERK_TEE, .value = PBKDF2_ERK_TEE_ONEWAY_OFFSET
    },
#endif
#if defined(CONFIG_KM_ERK1_REE_SUPPORT)
    {
        .index = CRYPTO_KDF_HARD_KEY_TYPE_ERK1_REE, .value = PBKDF2_ERK1_REE_ONEWAY_OFFSET
    },
#endif
#if defined(CONFIG_KM_ERK2_REE_SUPPORT)
    {
        .index = CRYPTO_KDF_HARD_KEY_TYPE_ERK2_REE, .value = PBKDF2_ERK2_REE_ONEWAY_OFFSET
    }
#endif
};
#endif

static const crypto_table_item g_rkp_hard_alg_sel_table[] = {
    {
        .index = CRYPTO_KDF_HARD_ALG_SHA256, .value = PBKDF2_ALG_SEL_SHA256
    },
#if defined(CONFIG_KM_KDF_SM3_SUPPORT)
    {
        .index = CRYPTO_KDF_HARD_ALG_SM3, .value = PBKDF2_ALG_SEL_SM3
    },
#endif
};

#define KDF_HARD_SALT_LEN           28
td_s32 hal_rkp_kdf_hard_calculation(const crypto_kdf_hard_calc_param *param)
{
    volatile td_s32 ret = TD_FAILURE;
    td_u32 i;
    td_u32 key_sel_reg_val = 0;
    td_u32 alg_reg_val = 0;
    td_u32 key_len_reg_val = 0;
#if defined(CRYPTO_IOT_V200)
    rkp_oneway_ree onewayval = {0};
    td_u32 oneway_offset = 0;
#endif
    rkp_cmd_cfg cfgval = {0};
    td_u32 salt_word = 0;

    km_null_ptr_chk(param);
    if (param->salt != TD_NULL) {
        crypto_chk_return(param->salt_length != KDF_HARD_SALT_LEN, KM_COMPAT_ERRNO(ERROR_INVALID_PARAM),
            "invalid param->salt_length\n");
    }
    crypto_chk_return(param->hard_alg != CRYPTO_KDF_HARD_ALG_SHA256 && param->hard_alg != CRYPTO_KDF_HARD_ALG_SM3,
        KM_COMPAT_ERRNO(ERROR_INVALID_PARAM), "invalid param->hard->alg\n");

    /* get key_sel. */
    ret = crypto_get_value_by_index(g_rkp_key_sel_table, crypto_array_size(g_rkp_key_sel_table),
        param->hard_key_type, &key_sel_reg_val);
    crypto_chk_return(ret != TD_SUCCESS, KM_COMPAT_ERRNO(ERROR_INVALID_PARAM), "get key_sel failed\n");

    /* get alg_sel. */
    ret = crypto_get_value_by_index(g_rkp_hard_alg_sel_table, crypto_array_size(g_rkp_hard_alg_sel_table),
        param->hard_alg, &alg_reg_val);
    crypto_chk_return(ret != TD_SUCCESS, KM_COMPAT_ERRNO(ERROR_INVALID_PARAM), "get alg_sel failed\n");

    /* get key_len. */
    ret = crypto_get_value_by_index(g_rkp_key_len_table, crypto_array_size(g_rkp_key_len_table),
        param->hard_key_size, &key_len_reg_val);
    crypto_chk_return(ret != TD_SUCCESS, KM_COMPAT_ERRNO(ERROR_INVALID_PARAM), "get key_len failed\n");

#if defined(CRYPTO_IOT_V200)
    /* get oneway_offset. */
    ret = crypto_get_value_by_index(g_rkp_oneway_offset_table, crypto_array_size(g_rkp_oneway_offset_table),
        param->hard_key_type, &oneway_offset);
    crypto_chk_return(ret != TD_SUCCESS, KM_COMPAT_ERRNO(ERROR_INVALID_PARAM), "get oneway_offset failed\n");

    /* config oneway. */
    if (param->hard_key_type == CRYPTO_KDF_HARD_KEY_TYPE_ERK1_REE) {
        onewayval.u32 |= (param->is_oneway << oneway_offset);
        km_reg_write(RKP_ERK1_ONEWAY, onewayval.u32);
    } else if (param->hard_key_type == CRYPTO_KDF_HARD_KEY_TYPE_ERK2_REE) {
        onewayval.u32 |= (param->is_oneway << oneway_offset);
        km_reg_write(RKP_ERK2_ONEWAY, onewayval.u32);
    } else {
        onewayval.u32 |= (param->is_oneway << oneway_offset);
        km_reg_write(RKP_ONEWAY, onewayval.u32);
    }
#endif
    /* config salt. */
    if (param->salt != TD_NULL) {
        for (i = 0; i < param->salt_length / CRYPTO_WORD_WIDTH; i++) {
            salt_word = CRYPTO_U8_TO_U32_BIT_SHIFT(param->salt, i);
#if defined(CONFIG_KM_CONTENT_KEY_SUPPORT)
            km_reg_write(RKP_SALT(i), salt_word);
#else
            km_reg_write(RKP_PBKDF2_DATA(i), salt_word);
#endif
        }
    }
    /* config sw_cfg for MDRK0 MDRK1 MDRK2 MDRK3 */
    if (param->hard_key_type == CRYPTO_KDF_HARD_KEY_TYPE_MDRK0 ||
        param->hard_key_type == CRYPTO_KDF_HARD_KEY_TYPE_MDRK1 ||
        param->hard_key_type == CRYPTO_KDF_HARD_KEY_TYPE_MDRK2 ||
        param->hard_key_type == CRYPTO_KDF_HARD_KEY_TYPE_MDRK3) {
            km_reg_write(SW_CFG, param->rkp_sw_cfg);
        }
    /* config rkp_cmd_cfg. */
    cfgval.u32 = km_reg_read(RKP_CMD_CFG);
    cfgval.bits.pbkdf2_key_len = key_len_reg_val;
    cfgval.bits.pbkdf2_alg_sel_cfg = alg_reg_val;
    cfgval.bits.pbkdf2_key_sel_cfg = key_sel_reg_val;
    cfgval.bits.master_key_sel = 0x2;

    cfgval.bits.sw_calc_req = 0x1;         /* start kdf calculation */
    km_reg_write(RKP_CMD_CFG, cfgval.u32);

    ret = hal_rkp_kdf_wait_done();
    crypto_chk_return(ret != TD_SUCCESS, ret, "hal_rkp_kdf_wait_done failed\n");

    return ret;
}

#if defined(CONFIG_RKP_DEBUG_ENABLE)
td_void hal_rkp_debug(td_void)
{
    td_u32 i;
    td_u32 reg_value = 0;
    rkp_cmd_cfg cmd_cfg;

    crypto_unused(cmd_cfg);
    /* RKP_LOCK. */
    reg_value = km_reg_read(RKP_LOCK);
    if (reg_value == RKP_LOCK_CPU_REE) {
        crypto_print("RKP locked by REE CPU!\r\n");
    } else if (reg_value == RKP_LOCK_CPU_TEE) {
        crypto_print("RKP locked by TEE CPU!\r\n");
    }

    /* RKP_CMD_CFG. */
    cmd_cfg.u32 = km_reg_read(RKP_CMD_CFG);
    crypto_print("RKP_CMD_CFG: sw_calc_req is 0x%x\r\n", cmd_cfg.bits.sw_calc_req);
    crypto_print("RKP_CMD_CFG: pbkdf2_alg_sel_cfg is 0x%x\r\n", cmd_cfg.bits.pbkdf2_alg_sel_cfg);
    crypto_print("RKP_CMD_CFG: pbkdf2_key_sel_cfg is 0x%x\r\n", cmd_cfg.bits.pbkdf2_key_sel_cfg);
    crypto_print("RKP_CMD_CFG: pbkdf2_key_len is 0x%x\r\n", cmd_cfg.bits.pbkdf2_key_len);
    crypto_print("RKP_CMD_CFG: rkp_pbkdf_calc_time is 0x%x\r\n", cmd_cfg.bits.rkp_pbkdf_calc_time);

    /* KDF_ERROR. */
    crypto_print("KDF_ERROR is 0x%x\r\n", km_reg_read(KDF_ERROR));
    /* RKP_RAW_INT */
    crypto_print("RKP_RAW_INT is 0x%x\r\n", km_reg_read(RKP_RAW_INT));
    /* RKP_INT_ENABLE */
    crypto_print("RKP_INT_ENABLE is 0x%x\r\n", km_reg_read(RKP_INT_ENABLE));
    /* RKP_INT */
    crypto_print("RKP_INT is 0x%x\r\n", km_reg_read(RKP_INT));
    /* RKP_DEOB_CFG */
    crypto_print("RKP_DEOB_CFG is 0x%x\r\n", km_reg_read(RKP_DEOB_CFG));
    /* DEOB_ERROR */
    crypto_print("DEOB_ERROR is 0x%x\r\n", km_reg_read(DEOB_ERROR));
    /* RK_RDY */
    crypto_print("RK_RDY is 0x%x\r\n", km_reg_read(RK_RDY));
    /* RKP_SALT */
    for (i = 0; i < KDF_HARD_SALT_LEN / CRYPTO_WORD_WIDTH; i++) {
        crypto_print("RKP_PBKDF2_DATA is 0x%x\r\n", km_reg_read(RKP_PBKDF2_DATA(i)));
    }
    /* RKP_ONEWAY */
    crypto_print("RKP_ONEWAY is 0x%x\r\n", km_reg_read(RKP_ONEWAY));
    crypto_print("RKP_ERK1_ONEWAY is 0x%x\r\n", km_reg_read(RKP_ERK1_ONEWAY));
    crypto_print("RKP_ERK2_ONEWAY is 0x%x\r\n", km_reg_read(RKP_ERK2_ONEWAY));

    crypto_print("RKP_PBKDF2_RSLT_CRC is 0x%x\n", km_reg_read(RKP_PBKDF2_RSLT_CRC));
}
#endif