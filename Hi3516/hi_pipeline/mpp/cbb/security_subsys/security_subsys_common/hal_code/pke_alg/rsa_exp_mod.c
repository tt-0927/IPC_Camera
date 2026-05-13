/**
  Copyright (c), 2001-2025, Shenshu Tech. Co., Ltd.
 */
#if defined(CONFIG_PKE_RSA_SUPPORT)
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

static td_s32 rsa_exp_mod(const drv_pke_data *n, const drv_pke_data *k, const drv_pke_data *in,
    const drv_pke_data *out)
{
    td_s32 ret = TD_FAILURE;
    td_u32 i = 0;
    td_s32 j = 0;
    td_u32 aligned_len = 0;
    td_u32 work_len = 0;
    const rom_lib *rom_lib_list[4] = {
        &instr_rsa_exp_00, &instr_rsa_exp_01, &instr_rsa_exp_10, &instr_rsa_exp_11
    };
    td_u8 bit_2 = 0;
    td_bool start_flag = TD_FALSE;
    td_u32 n_valid_index = 0;
    td_u32 k_valid_index = 0;
    td_u32 in_valid_index = 0;

    crypto_drv_func_enter();

    /* the parameter shouldn't be too large than the modulur. */
    get_valid_bit(n->data, n->length, &n_valid_index);
    get_valid_bit(k->data, k->length, &k_valid_index);
    get_valid_bit(in->data, in->length, &in_valid_index);
    aligned_len = crypto_max(crypto_max(k->length - k_valid_index, in->length - in_valid_index),
        n->length - n_valid_index);
    ret = hal_pke_get_align_val(aligned_len, &aligned_len);
    crypto_chk_func_return(hal_pke_get_align_val, ret);
    /* the time for calculate is 2^(valid_bit_index * 8) * 15 * 25(4,40), when valid_bit_index == 3, the time >= 10s;
        when valid_bit_index == 4, the time > 10000s, which will call timeout. */
    crypto_chk_return((aligned_len - (n->length - n_valid_index) > MAX_OFFSET_OF_VALID_BIT),
        PKE_COMPAT_ERRNO(ERROR_INVALID_PARAM), "the modulur is too small\n");
    /* the length should be enough to save the result. */
    hal_crypto_pke_check_param(out->length < aligned_len);

    ret = inner_update_rsa_modulus(sec_arg_add_cs(n->data + n_valid_index, n->length - n_valid_index, aligned_len));
    crypto_chk_func_return(inner_update_rsa_modulus, ret);

    /* step 1: data montgomery and copy. */
    /* 1. set data into DRAM. */
    work_len = aligned_len / ALIGNED_TO_WORK_LEN_IN_BYTE;
    hal_pke_set_ram(sec_arg_add_cs(rsa_addr_n, n->data + n_valid_index, n->length - n_valid_index, aligned_len));
    hal_pke_set_ram(sec_arg_add_cs(rsa_addr_a, in->data + in_valid_index, in->length - in_valid_index, aligned_len));

    hal_pke_set_ram_const_1(rsa_addr_const_1, aligned_len);
    /* 2. start calculate. */
    ret = inner_batch_instr_process(&instr_rsa_exp_pre_6, work_len);
    crypto_chk_func_return(inner_batch_instr_process, ret);

    /*
     * 1) k->data is one byte array, k->length is the byte number.
     * 2) For each byte in k->data, get bit2 = byte[7:6], byte[5:4], byte[3:2], byte[1:0] in order.
     * 3) If there is one bit2 is not zero, then all the next bit2(contain this time) will call
        inner_batch_instr_process(), the first param depends on bit2's value.
     */
    for (i = 0; i < k->length; i++) {
        for (j = 3; j >= 0; j--) {                  // 3: get byte[7:6], byte[5:4], byte[3:2], byte[1:0] in order.
            bit_2 = (k->data[i] >> (j * 2)) & 0x3;   // 2, 0x3: get bit2
            if ((start_flag == TD_FALSE) && (bit_2 != 0)) {
                start_flag = TD_TRUE;
                ret = inner_batch_instr_process(rom_lib_list[bit_2], work_len);
                crypto_chk_return(ret != TD_SUCCESS, ret, "inner_batch_instr_process failed\n");
            } else if (start_flag == TD_TRUE) {
                ret = inner_batch_instr_process(rom_lib_list[bit_2], work_len);
                crypto_chk_return(ret != TD_SUCCESS, ret, "inner_batch_instr_process failed\n");
            }
        }
    }

    /* step 3: data demontgomery and twice modulus reduction. */
    ret = inner_batch_instr_process(&instr_rsa_exp_post_3, work_len);
    crypto_chk_func_return(inner_batch_instr_process, ret);

    /* step 4: get data out. */
    (void)memset_s(out->data, out->length, 0x00, out->length);
    hal_pke_get_ram(sec_arg_add_cs(rsa_addr_s, out->data + out->length - aligned_len, aligned_len));

    crypto_drv_func_exit();
    return ret;
}

int32_t hal_pke_alg_rsa_exp_mod(const drv_pke_data *n, const drv_pke_data *k, const drv_pke_data *in,
    const drv_pke_data *out)
{
    int32_t ret = CRYPTO_FAILURE;

    ret = inner_pke_alg_resume();
    crypto_chk_return(ret != CRYPTO_SUCCESS, ret, "inner_pke_alg_resume failed\n");

    ret = rsa_exp_mod(n, k, in, out);
    inner_pke_alg_suspend();

    return ret;
}
#endif