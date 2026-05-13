/**
  Copyright (c), 2001-2025, Shenshu Tech. Co., Ltd.
 */

#ifndef DRV_KLAD_INNER_H
#define DRV_KLAD_INNER_H

#include "drv_klad.h"
#include "hal_klad.h"
#include "hal_rkp.h"

#define KLAD_VALID_HANDLE       0x2D3C4B5A

typedef struct {
    crypto_kdf_hard_key_type hard_key_type;
    crypto_klad_dest klad_dest;
    crypto_klad_attr klad_attr;
    td_handle keyslot_handle;
    td_u32 rkp_sw_cfg;
    td_bool is_open;
    td_bool is_attached;
    td_bool is_set_attr;
    td_bool is_set_key;
    td_bool is_set_session_key;
    crypto_klad_session_key session_key;
} drv_klad_context;

#define KM_COMPAT_ERRNO(err_code)     DRV_COMPAT_ERRNO(ERROR_MODULE_KM, err_code)

drv_klad_context *inner_klad_get_ctx(void);

#endif