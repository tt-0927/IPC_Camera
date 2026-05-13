/**
  Copyright (c), 2001-2025, Shenshu Tech. Co., Ltd.
 */

#include "pke_alg_inner.h"

#include "hal_pke_v5.h"
#include "hal_pke_alg.h"
#include "crypto_drv_common.h"
#include "pke_alg_inner.h"
#include "drv_pke_inner.h"

td_s32 inner_pke_alg_resume(void)
{
    td_s32 ret = TD_FAILURE;
    crypto_drv_func_enter();

    ret = hal_pke_lock();
    crypto_chk_func_return(hal_pke_lock, ret);

    hal_pke_enable_noise();

    ret = hal_pke_pre_process();
    crypto_chk_goto((ret != TD_SUCCESS), __EXIT, "hal_pke_pre_process failed, ret = 0x%x", ret);

    crypto_drv_func_exit();
    return ret;
__EXIT:
    hal_pke_disable_noise();
    hal_pke_unlock();
    return ret;
}

void inner_pke_alg_suspend(void)
{
    crypto_drv_func_enter();
    (void)hal_pke_clean_ram();
    hal_pke_disable_noise();
    hal_pke_unlock();
    crypto_drv_func_exit();
}

td_s32 inner_batch_instr_process(const rom_lib *batch_instr, td_u32 work_len)
{
    td_s32 ret = TD_FAILURE;
    crypto_drv_func_enter();

    ret = hal_pke_set_mode(sec_arg_add_cs(PKE_BATCH_INSTR, 0, batch_instr, work_len));
    crypto_chk_func_return(hal_pke_set_mode, ret);

    ret = hal_pke_start(sec_arg_add_cs(PKE_BATCH_INSTR));
    crypto_chk_func_return(hal_pke_start, ret);

    ret = hal_pke_wait_done();
    crypto_chk_func_return(hal_pke_wait_done, ret);

    crypto_drv_func_exit();
    return ret;
}