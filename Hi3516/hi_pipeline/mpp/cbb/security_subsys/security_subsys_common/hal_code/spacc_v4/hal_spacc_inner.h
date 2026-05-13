/**
  Copyright (c), 2001-2025, Shenshu Tech. Co., Ltd.
 */
#ifndef HAL_SPACC_INNER_H
#define HAL_SPACC_INNER_H

#include "crypto_type.h"

/* current cpu */
#define SPACC_CPU_CUR ((crypto_get_cpu_type() == CRYPTO_CPU_TYPE_SCPU) ? SPACC_CPU_TEE : SPACC_CPU_REE)

#define SYMC_COMPAT_ERRNO(err_code)         HAL_COMPAT_ERRNO(ERROR_MODULE_SYMC, err_code)

/* ! \Numbers of nodes list */
#define CRYPTO_SYMC_MAX_LIST_NUM            2

/* ! spacc symc int entry struct which is defined by hardware, you can't change it */
typedef struct {
    td_u32 sym_first_node : 1;         /* !<  Indicates whether the node is the first node */
    td_u32 sym_last_node : 1;          /* !<  Indicates whether the node is the last node */
    td_u32 rev1 : 7;                   /* !<  reserve */
    td_u32 odd_even : 1;               /* !<  Indicates whether the key is an odd key or even key */
    td_u32 rev2 : 22;                  /* !<  reserve */
    td_u32 sym_alg_length;             /* !<  symc data length */
    td_u32 sym_start_addr;             /* !<  symc start low addr */
    td_u32 sym_start_high;             /* !<  symc start high addr */
    td_u32 iv[CRYPTO_AES_IV_SIZE_IN_WORD]; /* !<  symc IV */
} crypto_symc_entry_in;

/* ! spacc symc out entry struct which is defined by hardware, you can't change it */
typedef struct {
    td_u32 rev1;           /* !<  reserve */
    td_u32 sym_alg_length; /* !<  syma data length */
    td_u32 sym_start_addr; /* !<  syma start low addr */
    td_u32 sym_start_high; /* !<  syma start high addr */
} crypto_symc_entry_out;

typedef struct {
    crypto_symc_entry_in *entry_in;
    crypto_symc_entry_out *entry_out;
    td_u32 idx_in;
    td_u32 idx_out;
    td_u32 cnt_in;
    td_u32 cnt_out;
    td_void *wait;
    crypto_wait_timeout_interruptible wait_func;
    td_bool done;
    td_bool is_wait;
    td_u32 timeout_ms;
    hal_symc_config_t symc_config;
} crypto_symc_hard_context;

crypto_symc_hard_context *inner_hal_get_symc_ctx(td_u32 chn_num);

td_s32 hal_cipher_symc_wait_noout_done(td_u32 chn_num);

#endif