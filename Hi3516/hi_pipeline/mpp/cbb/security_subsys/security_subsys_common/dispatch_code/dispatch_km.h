/*
  Copyright (c), 2001-2025, Shenshu Tech. Co., Ltd.
 */

#ifndef DISPATCH_KM_H
#define DISPATCH_KM_H

#include "ioctl_km.h"
#include "crypto_drv_common.h"

crypto_ioctl_cmd *get_km_func_list(td_void);

td_u32 get_km_func_num(td_void);

#endif