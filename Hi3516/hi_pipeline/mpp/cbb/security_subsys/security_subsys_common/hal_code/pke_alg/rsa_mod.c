/**
  Copyright (c), 2001-2025, Shenshu Tech. Co., Ltd.
 */
#include "hal_pke_alg.h"

#include "crypto_drv_common.h"
#include "hal_pke_v5.h"
#include "pke_alg_inner.h"
#include "drv_pke_inner.h"

#define MAX_OFFSET_OF_VALID_BIT 2

static void get_valid_bit(const td_u8 *n, const td_u32 key_size, td_u32 *valid_bit_index)
{
    for (*valid_bit_index = 0; *valid_bit_index < key_size; (*valid_bit_index)++) {
        if (n[*valid_bit_index] != 0x00) {
            break;
        }
    }
    return;
}

int inner_rsa_mod(const drv_pke_data *a, const drv_pke_data *p, const drv_pke_data *c)
{
    td_s32 ret = TD_FAILURE;
    td_u32 work_len = 0;
    td_u32 aligned_len = 0;
    td_u8 const_1[CRYPTO_WORD_WIDTH] = {0};
    td_u8 bit_2[CRYPTO_WORD_WIDTH] = {0};
    td_u32 a_valid_index = 0;
    td_u32 a_valid_length = 0;
    td_u32 p_valid_index = 0;
    td_u32 p_valid_length = 0;
    crypto_drv_func_enter();
    hal_crypto_pke_check_param(p->length > DRV_PKE_LEN_4096);

    const_1[CRYPTO_WORD_WIDTH - 1] = 0x1;   /* set const value. */

    /* the modulur couldn't be zero. */
    crypto_chk_return(inner_drv_is_zero(p->data, p->length) == TD_TRUE, ERROR_INVALID_PARAM, "p is zero\n");
    /* the modulur couldn't be even data. */
    hal_crypto_pke_check_param(p->data[p->length - 1] % 2 == 0);    /* 2: to check if even. */

    /* the parameter shouldn't be too large than the modulur. */
    get_valid_bit(a->data, a->length, &a_valid_index);
    get_valid_bit(p->data, p->length, &p_valid_index);
    a_valid_length = a->length - a_valid_index;
    p_valid_length = p->length - p_valid_index;
    hal_crypto_pke_check_param(a_valid_length > 2 * p_valid_length);    /* 2: double */

    /* Note: For 521 bit mode, the actual size should be 576 */
    ret = hal_pke_get_align_val(p_valid_length, &aligned_len);
    crypto_chk_func_return(hal_pke_get_align_val, ret);
    /* the time for calculate is 2^(valid_bit_index * 8) * 15 * 25(4,40), when valid_bit_index == 3, the time >= 10s;
        when valid_bit_index == 4, the time > 10000s, which will call timeout. */
    crypto_chk_return((aligned_len - p_valid_length > MAX_OFFSET_OF_VALID_BIT),
        PKE_COMPAT_ERRNO(ERROR_INVALID_PARAM), "the modulur is too small\n");
    /* the length should be enough to save the result. */
    hal_crypto_pke_check_param(c->length < aligned_len);

    (void)memset_s(bit_2, CRYPTO_WORD_WIDTH, 0x00, CRYPTO_WORD_WIDTH);
    /* Step 1. set data into DRAM. */
    hal_pke_set_ram(sec_arg_add_cs(rsa_addr_n, p->data + p_valid_index, p_valid_length, aligned_len));
    hal_pke_set_ram(sec_arg_add_cs(rsa_addr_const_1, const_1, CRYPTO_WORD_WIDTH, aligned_len));
    /* set high bit into rsa_addr_t1, set low bit into rsa_addr_t0. */
    if (a_valid_length < aligned_len) {
        hal_pke_set_ram(sec_arg_add_cs(rsa_addr_t0, a->data + a_valid_index, a_valid_length, aligned_len));
        hal_pke_set_ram(sec_arg_add_cs(rsa_addr_t1, bit_2, CRYPTO_WORD_WIDTH, aligned_len));
    } else {
        hal_pke_set_ram(sec_arg_add_cs(rsa_addr_t0, a->data + a->length - aligned_len, aligned_len, aligned_len));
        hal_pke_set_ram(sec_arg_add_cs(rsa_addr_t1, a->data + a_valid_index, a_valid_length - aligned_len,
            aligned_len));
    }
    /* set mont_param */
    ret = inner_update_rsa_modulus(p->data + p_valid_index, p_valid_length, aligned_len);
    crypto_chk_func_return(inner_update_rsa_modulus, ret);

    /* Step 2. start calculation. */
    work_len = aligned_len / ALIGNED_TO_WORK_LEN_IN_BYTE;
    ret = inner_batch_instr_process(&instr_rsa_mod, work_len);
    crypto_chk_func_return(inner_batch_instr_process, ret);

    /* Step 3. get result. */
    (void)memset_s(c->data, c->length, 0x00, c->length);
    hal_pke_get_ram(sec_arg_add_cs(rsa_addr_a, c->data + c->length - aligned_len, aligned_len));

    crypto_drv_func_exit();
    return TD_SUCCESS;
}

int hal_pke_alg_rsa_mod(const drv_pke_data *a, const drv_pke_data *p, const drv_pke_data *c)
{
    int32_t ret = CRYPTO_FAILURE;

    ret = inner_pke_alg_resume();
    crypto_chk_return(ret != CRYPTO_SUCCESS, ret, "inner_pke_alg_resume failed\n");

    ret = inner_rsa_mod(a, p, c);
    inner_pke_alg_suspend();

    return ret;
}