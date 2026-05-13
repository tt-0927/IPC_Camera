/**
 * @FilePath     : mpp_sys.c
 * @Author       : zhouzirui
 * @Date         : 2025-03-21 16:29:38
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2025-07-17 14:33:59
 * @Description  : mpp系统控制模块接口
 */

#include "mpp_sys.h"

int mppSys_init()
{
    CHECK_API_RETURN(ss_mpi_sys_init());
    return TD_SUCCESS;
}

int mppSys_uninit()
{
    CHECK_API_RETURN(ss_mpi_sys_exit());
    return TD_SUCCESS;
}

int mppVb_set_cfg(ot_vb_cfg stVbCfg)
{
    // mpi_log("%d , stVbCfg%d, %d, %d,",stVbCfg.max_pool_cnt,stVbCfg.common_pool[0].)
    CHECK_API_RETURN(ss_mpi_vb_set_cfg(&stVbCfg));
    return TD_SUCCESS;
}

int mppVb_get_cfg(ot_vb_cfg *pVbCfg)
{
    CHECK_API_RETURN(ss_mpi_vb_get_cfg(pVbCfg));
    return TD_SUCCESS;
}

int mppVb_init()
{
    CHECK_API_RETURN(ss_mpi_vb_init());
    return TD_SUCCESS;
}

int mppVb_uninit()
{
    CHECK_API_RETURN(ss_mpi_vb_exit());
    return TD_SUCCESS;
}

int mppAudio_init()
{
    CHECK_API_RETURN(ss_mpi_audio_init());

    return TD_SUCCESS;
}

int mppAudio_uninit()
{
    CHECK_API_RETURN(ss_mpi_audio_exit());
    return TD_SUCCESS;
}