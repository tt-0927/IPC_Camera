/*
  Copyright (c), 2001-2025, Shenshu Tech. Co., Ltd.
 */

#ifndef KAPI_OTP_H
#define KAPI_OTP_H

#include "crypto_type.h"

td_s32 kapi_otp_init(td_void);

td_s32 kapi_otp_deinit(td_void);

td_s32 kapi_otp_read_word(td_u32 addr, td_u32 *data);

td_s32 kapi_otp_read_byte(td_u32 addr, td_u8 *data);

td_s32 kapi_otp_write_byte(td_u32 addr, td_u8 data);

#endif