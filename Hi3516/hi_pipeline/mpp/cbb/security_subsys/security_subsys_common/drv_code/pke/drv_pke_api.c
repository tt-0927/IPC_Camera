/**
  Copyright (c), 2001-2025, Shenshu Tech. Co., Ltd.
 */

#include "drv_pke.h"
#include "hal_pke_v5.h"
#include "crypto_drv_common.h"

td_s32 drv_cipher_pke_init(void)
{
    int ret;
    crypto_drv_func_enter();
    ret = hal_pke_init();
    crypto_drv_func_exit();
    return ret;
}

td_s32 drv_cipher_pke_deinit(void)
{
    int ret;
    crypto_drv_func_enter();
    ret = hal_pke_deinit();
    crypto_drv_func_exit();
    return ret;
}