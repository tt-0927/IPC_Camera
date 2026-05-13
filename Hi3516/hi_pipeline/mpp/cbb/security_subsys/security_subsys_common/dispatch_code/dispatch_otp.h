/*
  Copyright (c), 2001-2025, Shenshu Tech. Co., Ltd.
 */

#ifndef DISPATCH_OTP_H
#define DISPATCH_OTP_H

#include "ioctl_otp.h"
#include "crypto_drv_common.h"

crypto_ioctl_cmd *get_otp_func_list(td_void);

td_u32 get_otp_func_num(td_void);

#endif