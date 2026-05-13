/**
  Copyright (c), 2001-2025, Shenshu Tech. Co., Ltd.
 */
#include "pke_alg_inner.h"

#include "hal_pke_v5.h"
#include "hal_pke_alg.h"
#include "crypto_drv_common.h"
#include "pke_alg_inner.h"
#include "drv_pke_inner.h"

static void inner_arry_right_shift_value(td_u8 *k, td_u32 k_len, td_u32 shift_bit)
{
    td_u32 i = 0;
    td_u8 left_mask = ((1 << shift_bit) - 1) & 0xFF;
    td_u8 right_mask = ~((1 << shift_bit) - 1) & 0xFF;
    td_u32 left_shift_bit = CRYPTO_BITS_IN_BYTE - shift_bit;

    for (i = k_len - 1; i > 0; i--) {
        k[i] = (((k[i] & right_mask) >> shift_bit) & (0xFF >> shift_bit)) |
            (((k[i - 1] & left_mask) << left_shift_bit) & (0xFF << left_shift_bit));
    }
    k[0] = ((k[0] & right_mask) >> shift_bit) & (0xFF >> shift_bit);

    return;
}

CRYPTO_STATIC void inner_bn_add_one(td_u8 *data, td_u32 len)
{
    td_u32 idx = len - 1;
    td_s32 carry = 1;
    td_u32 value;
    while (carry == 1 && idx >= 1) {
        value = data[idx] + carry;
        if (value > 0xFF) {
            carry = 1;
        } else {
            carry = 0;
        }
        data[idx] = value % (0x100);
        idx--;
    }
    data[idx] += carry;
}

CRYPTO_STATIC void inner_bn_sub_one(td_u8 *data, td_u32 len)
{
    td_s32 idx = len - 1;
    td_s32 carry = 1;
    while (carry == 1 && idx >= 0) {
        if (data[idx] == 0) {
            carry = 1;
            data[idx] = 0xFF;
        } else {
            carry = 0;
            data[idx]--;
        }
        if (idx == 0) {
            return;
        }
        idx--;
    }
}

static void inner_array_add_plus_minus_one(td_u8 *k, td_u32 k_len, td_s32 value)
{
    if (value == 0) {
        return;
    }

    if (value == 1) {
        inner_bn_add_one(k, k_len);
    } else if (value == -1) {
        inner_bn_sub_one(k, k_len);
    }

    return;
}

static td_s32 inner_convert_normal_scalar_to_naf(const td_u8 *k, const td_u32 k_len, td_s32 *k_naf,
    td_u32 *k_naf_len)
{
    td_s32 ret = TD_FAILURE;
    td_u32 cnt = 0;

    td_u8 *tmp = crypto_malloc(k_len);
    crypto_chk_return((tmp == TD_NULL), ret, "malloc failed!\n");

    ret = memcpy_s(tmp, k_len, k, k_len);
    crypto_chk_goto_with_ret(ret, ret != EOK, __EXIT, ERROR_MEMCPY_S, "copy data failed!\n");

    while (inner_drv_is_zero(tmp, k_len) != TD_TRUE) {
        if (tmp[k_len - 1] % 2 == 1) {  /* 2: to check whether the byte is odd or even. */
            /* the last byte is odd, then the whole data is odd. */
            k_naf[cnt] = (int) (2 - tmp[k_len - 1] % CRYPTO_WORD_WIDTH);    /* 2: for normal scalar to naf trans. */
            inner_array_add_plus_minus_one(tmp, k_len, -k_naf[cnt]);
        } else {
            k_naf[cnt] = 0;
        }
        inner_arry_right_shift_value(tmp, k_len, 1);  /* 1: right shift one. */
        cnt++;
    }

    *k_naf_len = cnt;
__EXIT:
    if (tmp != TD_NULL) {
        crypto_free(tmp);
        tmp = TD_NULL;
    }
    return ret;
}

static td_s32 inner_point_mul_naf(const rom_lib *batch_instr, const td_u32 batch_instr_num,
    const drv_pke_data *k, td_u32 work_len)
{
    td_s32 ret = TD_FAILURE;
    td_s32 i = 0;

    /* Step 1: convert k to non-adjacent-form. */
    td_u32 k_naf_len = k->length * CRYPTO_BITS_IN_BYTE + 1;
    td_s32 *k_naf = crypto_malloc(k_naf_len * sizeof(td_u32));
    crypto_chk_return((k_naf == TD_NULL), ERROR_MALLOC, "malloc failed!\n");
    ret = inner_convert_normal_scalar_to_naf(k->data, k->length, k_naf, &k_naf_len);
    crypto_chk_goto((ret != TD_SUCCESS), __EXIT, "convert data to naf failed!\n");

    crypto_unused(batch_instr_num);
    /* before calculate naf_point_mul, you need to set mont_a to ecc_addr_mont_a first.
       And keep ecc->p in the modulur. */
    /* Step 2: calculate point multiplication by double point and point plus. k_naf couldn't be infinity point. */
    /* instr_ecfp_mul_p_22_18[0,40], instr_ecfp_mul_g_22_18[40,40], instr_ecfp_mul_c_double_22[0,22] */
    for (i = k_naf_len - 2; i >= 0; i--) {  /* 2: to get the start calculate index. */
        if (k_naf[i] == 1) {
            ret = hal_pke_set_mode(sec_arg_add_cs(PKE_BATCH_INSTR, 0, &batch_instr[0], work_len));
        } else if (k_naf[i] == -1) {
            ret = hal_pke_set_mode(sec_arg_add_cs(PKE_BATCH_INSTR, 0, &batch_instr[1], work_len));
        } else {
            ret = hal_pke_set_mode(sec_arg_add_cs(PKE_BATCH_INSTR, 0, &batch_instr[2], work_len));  /* 2: instr index */
        }
        crypto_chk_goto((ret != TD_SUCCESS), __EXIT, "hal_pke_set_mode failed, ret = 0x%x", ret);
        ret = hal_pke_start(sec_arg_add_cs(PKE_BATCH_INSTR));
        crypto_chk_goto((ret != TD_SUCCESS), __EXIT, "hal_pke_start failed, ret = 0x%x", ret);
        ret = hal_pke_wait_done();
        crypto_chk_goto((ret != TD_SUCCESS), __EXIT, "hal_pke_wait_done failed, ret = 0x%x", ret);
    }

__EXIT:
    if (k_naf != TD_NULL) {
        crypto_free(k_naf);
        k_naf = TD_NULL;
    }

    return ret;
}

#define POINT_NAF_INSTR_NUM     3
td_s32 inner_ecfp_mul_naf_cal(td_u32 work_len, const drv_pke_data *k)
{
    volatile td_s32 ret = TD_FAILURE;
    rom_lib batch_instr_block[POINT_NAF_INSTR_NUM] = {instr_ecfp_mul_p_22_18, instr_ecfp_mul_g_22_18,
        instr_ecfp_mul_c_double_22};

    crypto_drv_func_enter();

    /* WARN: from here, there should be no other process which is not ecc, for the result in DRAM wasn't got out, other
    process will destroy data in the DRAM. */
    /* 3. copy negative point data */
    ret = inner_batch_instr_process(&instr_ecfp_cpy_np2g_2, work_len);
    crypto_chk_func_return(inner_batch_instr_process, ret);

    /* 4. NAF point multiplication. */
    ret = inner_point_mul_naf(batch_instr_block, POINT_NAF_INSTR_NUM, k, work_len);
    crypto_chk_func_return(inner_point_mul_naf, ret);

    crypto_drv_func_exit();
    return ret;
}