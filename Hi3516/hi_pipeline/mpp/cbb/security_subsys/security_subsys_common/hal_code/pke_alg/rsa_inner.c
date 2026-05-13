/**
  Copyright (c), 2001-2025, Shenshu Tech. Co., Ltd.
 */
#if defined(CONFIG_PKE_RSA_SUPPORT)
#include "pke_alg_inner.h"

#include "drv_pke_inner.h"
#include "hal_pke_v5.h"
#include "crypto_drv_common.h"

#define PKE_MONT_PARAM_LEN  2
#define PKE_MONT_BIT_LEN    64
#define PKE_MONT_LEN_IN_BYTE    8
#define HALF_BYTE_VALUE 128
#define BYTE_VALUE 256
#define ALIGNED_TO_WORK_LEN_IN_BYTE 8

typedef struct {
    td_u32 aligned_len;
    const rom_lib *batch_instr;
    td_u32 loop_number;
} rrn_loop_table;

#define DRV_PKE_CAL_LEN_1536    192
#define DRV_PKE_CAL_LEN_320     40
#define LOOP_NUMBER_4096    8
#define LOOP_NUMBER_3072    10
#define LOOP_NUMBER_2048    7
#define LOOP_NUMBER_1536    9
#define LOOP_NUMBER_1024    6
#define LOOP_NUMBER_192     6
#define LOOP_NUMBER_256     4
#define LOOP_NUMBER_320     6
#define LOOP_NUMBER_384     7
#define LOOP_NUMBER_512     5
#define MAX_RRN_SUPPORT_TYPE    10
#if defined(CONFIG_ROMABLE_API_SUPPORT)
static const rrn_loop_table g_rrn_loop_table[MAX_RRN_SUPPORT_TYPE] = {
    {DRV_PKE_LEN_3072, &instr_rsa_rrn_add_3072, LOOP_NUMBER_3072},
};
#else
static const rrn_loop_table g_rrn_loop_table[MAX_RRN_SUPPORT_TYPE] = {
    {DRV_PKE_LEN_3072, &instr_rsa_rrn_add_3072, LOOP_NUMBER_3072},
    {DRV_PKE_CAL_LEN_1536, &instr_rsa_rrn_add_3072, LOOP_NUMBER_1536},
    {DRV_PKE_LEN_192, &instr_rsa_rrn_add_3072, LOOP_NUMBER_192},
    {DRV_PKE_LEN_256, &instr_rsa_rrn_add, LOOP_NUMBER_256},
    {DRV_PKE_CAL_LEN_320, &instr_rsa_rrn_add_320, LOOP_NUMBER_320},
    {DRV_PKE_LEN_384, &instr_rsa_rrn_add_3072, LOOP_NUMBER_384},
    {DRV_PKE_LEN_512, &instr_rsa_rrn_add, LOOP_NUMBER_512},
    {DRV_PKE_LEN_1024, &instr_rsa_rrn_add, LOOP_NUMBER_1024},
    {DRV_PKE_LEN_2048, &instr_rsa_rrn_add, LOOP_NUMBER_2048},
    {DRV_PKE_LEN_4096, &instr_rsa_rrn_add, LOOP_NUMBER_4096},
};
#endif

static td_s32 inner_get_loop_number(td_u32 aligned_len, td_u32 *loop_number)
{
    td_s32 ret = TD_FAILURE;
    td_u32 i = 0;
    td_u32 work_len = aligned_len / ALIGNED_TO_WORK_LEN_IN_BYTE;

    /* Step3: prepare data for loop calculate. */
    for (; i < MAX_RRN_SUPPORT_TYPE; i++) {
        if (aligned_len == g_rrn_loop_table[i].aligned_len) {
            ret = inner_batch_instr_process(g_rrn_loop_table[i].batch_instr, work_len);
            crypto_chk_func_return(inner_batch_instr_process, ret);
            *loop_number = g_rrn_loop_table[i].loop_number;
            return TD_SUCCESS;
        }
    }
    crypto_log_err("invalid modulur\n");
    return PKE_COMPAT_ERRNO(ERROR_INVALID_PARAM);
}

static td_s32 inner_rsa_rrn(const td_u8 *n, const td_u32 key_size, const td_u32 aligned_len, td_u8 *rrn)
{
    td_s32 ret = TD_FAILURE;
    td_u32 cnt = 0;
    td_u32 i = 0;
    /* WARN: the aligned_len maybe larger than key_size. */
    td_u32 work_len = aligned_len / ALIGNED_TO_WORK_LEN_IN_BYTE;
    td_u8 bit_2[DRV_PKE_LEN_4096];

    /* Step 1: get data initial data rr = 2^(bit_len - 1). */
    (void)memset_s(bit_2, DRV_PKE_LEN_4096, 0x00, DRV_PKE_LEN_4096);
    bit_2[0] = HALF_BYTE_VALUE;

    /* Step 2: set data to PKE DRAM. */
    hal_pke_set_ram(sec_arg_add_cs(rsa_addr_n, n, key_size, aligned_len));
    hal_pke_set_ram(sec_arg_add_cs(rsa_addr_rr, bit_2, aligned_len, aligned_len));

    /* Step3: prepare data for loop calculate. */
    ret = inner_get_loop_number(aligned_len, &cnt);
    crypto_chk_func_return(inner_get_loop_number, ret);

    /* Step 4.2: start calculate square multiplication modulur. */
    for (i = 0; i < cnt; i++) {
        ret = inner_batch_instr_process(&instr_rsa_rrn_mul, work_len);
        crypto_chk_func_return(inner_batch_instr_process, ret);
    }

    /* Step 4: if rrn is not null, get data out from DRAM. */
    if (rrn != TD_NULL) {
        hal_pke_get_ram(sec_arg_add_cs(rsa_addr_rr, rrn, aligned_len));
    }

    return ret;
}

static td_s32 inner_byte_stream_to_int_array(const td_u8 *byte_stream, const td_u32 stream_length,
    td_u32 *int_array, td_u32 array_length)
{
    td_u32 i = 0;
    crypto_drv_func_enter();

    hal_crypto_pke_check_param(stream_length % sizeof(td_u32) != 0);

    (void)memset_s(int_array, array_length, 0x00, array_length);

    for (i = 0; i < stream_length; i += CRYPTO_WORD_WIDTH) {
        int_array[ i / sizeof(td_u32)] |= byte_stream[i] << 24;     /* 24: get [31:24] */
        int_array[ i / sizeof(td_u32)] |= byte_stream[i + 1] << 16; /* 16: get [23:16] */
        int_array[ i / sizeof(td_u32)] |= byte_stream[i + 2] << 8;  /* 2,8: get [15:8] */
        int_array[ i / sizeof(td_u32)] |= byte_stream[i + 3];       /* 3: get[7:0] */
    }

    crypto_drv_func_exit();
    return TD_SUCCESS;
}

/* int this condition b will no more than a, and the most b = a - 1, so that c->length = a.length */
static td_s32 normal_sub(const drv_pke_data *a, const drv_pke_data *b, const drv_pke_data *c)
{
    td_s32 ret = TD_FAILURE;
    td_s32 i = 0;
    td_s32 j = 0;
    td_s32 carry = 0;

    hal_crypto_pke_check_param(a == TD_NULL);
    hal_crypto_pke_check_param(a->data == TD_NULL);
    hal_crypto_pke_check_param(b == TD_NULL);
    hal_crypto_pke_check_param(b->data == TD_NULL);
    hal_crypto_pke_check_param(c == TD_NULL);
    hal_crypto_pke_check_param(c->data == TD_NULL);
    hal_crypto_pke_check_param(a->length != c->length);

    (void)memset_s(c->data, c->length, 0x00, c->length);
    ret = memcpy_s(c->data, c->length, a->data, a->length);
    crypto_chk_return(ret != EOK, ret, "memcpy_s failed\n");

    for (i = a->length - 1, j = b->length - 1; i >= 1 && j >= 0; i--, j--) {
        if (a->data[i] < b->data[j] || a->data[i] - b->data[j] < carry) {
            /* need to call one */
            c->data[i] = c->data[i] + (BYTE_VALUE - b->data[j] - carry);
            carry = 1;  /* note next byte */
        } else {
            c->data[i] -= (b->data[j] + carry);
            carry = 0;  /* note next byte */
        }
    }
    if (j == 0) {
        /* which means the b->length == a->length */
        c->data[i] -= (b->data[j] + carry);
    }

    return TD_SUCCESS;
}

/* left shift const one offset bits, while offset is less than 64. */
static td_void left_shift_const_one(const drv_pke_data *a, const td_u32 offset)
{
    td_u32 i = offset / CRYPTO_BITS_IN_BYTE;
    td_u32 e = offset % CRYPTO_BITS_IN_BYTE;
    crypto_drv_func_enter();

    (void)memset_s(a->data, a->length, 0x00, a->length);
    a->data[a->length - i - 1] = 1 << e;

    crypto_drv_func_exit();
}

/* left is high address, right is low address. And c->length is larger one than the max(a->length, b->length) */
static td_void inner_normal_add(const drv_pke_data *a, const drv_pke_data *b, const drv_pke_data *c)
{
    td_s32 i = 0;
    td_s32 j = 0;
    td_s32 k = 0;
    td_s32 carry = 0;
    crypto_drv_func_enter();

    (void)memset_s(c->data, c->length, 0x00, c->length);
    for (i = a->length - 1, j = b->length - 1, k = c->length - 1; i >= 0 && j >= 0 && k >= 1; i--, j--, k--) {
        carry = 0;
        if (a->data[i] >= (BYTE_VALUE - b->data[j])) {
            carry = b->data[j] - (BYTE_VALUE - a->data[i]); /* most value is 0xff + 0xff = 0x1fe, will cause move. */
            c->data[k - 1] += 1;
        } else {
            carry = a->data[i] + b->data[j]; /* most value is 0x80 + 0x7f = 0xff. */
        }
        if ((carry == BYTE_VALUE - 1 && c->data[k] == 1) ||
            (carry == BYTE_VALUE - 2 && c->data[k] == 2)) { // 2: carry in binary.
            /* in this condition, the add process will cause carry. */
            c->data[k] = 0;
            c->data[k - 1] += 1;
        } else {
            c->data[k] += carry;
        }
    }

    /* when out the for loop, i == 0 or j == 0 */
    for (; j >= 0 && k >= 1; j--, k--) {
        if ((b->data[j] == BYTE_VALUE - 1 && c->data[k] == 1) ||
            (b->data[j] == BYTE_VALUE - 2 && c->data[k] == 2)) {  /* 2: carry in binary. */
            /* in this condition, the add process will cause carry. */
            c->data[k] = 0;
            c->data[k - 1] += 1;
        } else {
            c->data[k] += b->data[j];
        }
    }
    for (; i >= 0 && k >= 1; i--, k--) {
        if ((a->data[i] == BYTE_VALUE - 1 && c->data[k] == 1) ||
            (a->data[i] == BYTE_VALUE - 2 && c->data[k] == 2)) {  /* 2: carry in binary. */
            /* in this condition, the add process will cause carry. */
            c->data[k] = 0;
            c->data[k - 1] += 1;
        } else {
            c->data[k] += a->data[i];
        }
    }

    crypto_drv_func_exit();
}

static void inner_arry_right_shift_value(td_u8 *k, td_u32 k_len, td_u32 shift_bit)
{
    td_u32 i = 0;
    td_u8 left_mask = ((1 << shift_bit) - 1) & 0xFF;
    td_u8 right_mask = ~((1 << shift_bit) - 1) & 0xFF;

    for (i = k_len - 1; i > 0; i--) {
        k[i] = ((k[i] & right_mask) >> shift_bit) | ((k[i - 1] & left_mask) << (CRYPTO_BITS_IN_BYTE - shift_bit));
    }
    k[0] = (k[0] & right_mask) >> shift_bit;

    return;
}

static td_s32 inner_set_mont_param(const drv_pke_data *p)
{
    td_s32 ret = TD_FAILURE;
    td_s32 i = 0;
    td_u32 s_len = PKE_MONT_LEN_IN_BYTE;
    td_u8 s[PKE_MONT_LEN_IN_BYTE];
    td_u8 p_cal[PKE_MONT_LEN_IN_BYTE];
    td_u32 x_len = PKE_MONT_LEN_IN_BYTE + 1;
    td_u8 x[PKE_MONT_LEN_IN_BYTE + 1];
    td_u8 bit_2[PKE_MONT_LEN_IN_BYTE + 1];
    td_u8 tmp[PKE_MONT_LEN_IN_BYTE + 1];
    td_u8 tmp_2[PKE_MONT_LEN_IN_BYTE + 1];
    drv_pke_data aa = { .length = s_len, .data = s };
    drv_pke_data bb = { .length = s_len, .data = p_cal };
    drv_pke_data cc = { .length = x_len, .data = tmp };
    drv_pke_data dd = { .length = x_len, .data = x };
    drv_pke_data ff = { .length = x_len, .data = tmp_2 };
    td_u32 mont_param[PKE_MONT_PARAM_LEN] = {0};
    crypto_drv_func_enter();

    (void)memset_s(x, x_len, 0, x_len);
    x[PKE_MONT_LEN_IN_BYTE] = 1;

    /* montgomery_data_config */
    (void)memset_s(bit_2, x_len, 0, x_len);
    bit_2[0] = 1;   /* 2^(2*bit_len), so it need to move left one bit, which means the highest one byte value is 1. */
    (void)memset_s(tmp, x_len, 0, x_len);

    /* Step 1: only 64bit participate the calculation. */
    ret = memcpy_s(p_cal, s_len, (p->data + p->length - s_len), s_len);
    crypto_chk_return(ret != EOK, ret, "memcpy_s failed\n");

    ret = memcpy_s(s, s_len, p_cal, s_len);
    crypto_chk_return(ret != EOK, ret, "memcpy_s failed\n");

    /* Step 2: right shift 1 to get s = (p - 1) >> 1 */
    inner_arry_right_shift_value(s, s_len, 1);

    /* x = x + (xi << i), xi << i may largest is 64 bit, so x largest is 64 bit 1, and smallest is 1. */
    for (i = 1; i < PKE_MONT_BIT_LEN; i++) {
        td_s32 xi = s[s_len - 1] & 1;   /* xi = s & 1; */
        crypto_log_dbg("xi[%d] = %d\r\n", i, xi);
        if (xi == 1) {
            inner_normal_add(&aa, &bb, &cc);  /* s = s + xi * p; */
            inner_arry_right_shift_value(tmp, x_len, 1);  /* right shift 1. s = s >> 1. */
            ret = memcpy_s(s, s_len, tmp + 1, s_len); /* copy the result s for next calculation. */
            crypto_chk_return(ret != EOK, ret, "memcpy_s failed\n");

            left_shift_const_one(&cc, i);   /* xi << i */
            inner_normal_add(&dd, &cc, &ff);  /* x + (xi << i) */
            ret = memcpy_s(x, x_len, tmp_2, x_len);   /* x = x + (xi << i) */
            crypto_chk_return(ret != EOK, ret, "memcpy_s failed\n");
        } else {
            inner_arry_right_shift_value(s, s_len, 1);    /* right shift 1. */
        }
    }
    cc.data = bit_2;
    ret = normal_sub(&cc, &dd, &ff);
    crypto_chk_func_return(normal_sub, ret);

    /* set mont_param into register. */
    ret = inner_byte_stream_to_int_array(tmp_2 + 1, s_len, mont_param, PKE_MONT_PARAM_LEN);
    crypto_chk_func_return(inner_byte_stream_to_int_array, ret);

    crypto_log_dbg("mont_param[0] = 0x%x, mont_param[1] = 0x%x\r\n", mont_param[0], mont_param[1]);
    ret = hal_pke_set_mont_para(sec_arg_add_cs(mont_param[1], mont_param[0]));
    crypto_chk_func_return(hal_pke_set_mont_para, ret);

    crypto_drv_func_exit();
    return TD_SUCCESS;
}

td_s32 inner_update_rsa_modulus(const td_u8 *n, const td_u32 n_len, const td_u32 aligned_len)
{
    td_s32 ret = TD_FAILURE;
    td_u8 rrn[DRV_PKE_LEN_4096] = {0};
    drv_pke_data mod_n = {0};
    crypto_drv_func_enter();
    check_sum_inspect(PKE_COMPAT_ERRNO(ERROR_INVALID_PARAM), n, n_len);

    mod_n = (drv_pke_data) {.data = (td_u8 *)n, .length = n_len};
    /* 1. get new montgomery parameters and set it into register. */
    ret = inner_set_mont_param(&mod_n);
    crypto_chk_func_return(inner_set_mont_param, ret);
    /* 2. calculate to get rrn value. */
    ret = inner_rsa_rrn(n, n_len, aligned_len, rrn);
    crypto_chk_func_return(inner_rsa_rrn, ret);
    /* 3. set rrn into PKE DRAM rsa_addr_rr. */
    hal_pke_set_ram(sec_arg_add_cs(rsa_addr_rr, rrn, aligned_len, aligned_len));

    crypto_drv_func_exit();
    return TD_SUCCESS;
}
#endif