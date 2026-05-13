/*
  Copyright (c), 2001-2025, Shenshu Tech. Co., Ltd.
 */

#include "isp_dfx.h"
#include "valg_plat.h"

static td_u32 g_usr_dfx_en = 1;

/* dfx: if want to use the the same precision of kernel time, use ot_mpi_sys_get_cur_pts() */
td_void isp_dfx_get_lock_time(isp_usr_ctx *isp_ctx)
{
    if (g_usr_dfx_en == 0) {
        return;
    }
    isp_ctx->dfx_info.lock_cur_time = get_sys_time_by_usec();
}
td_void isp_dfx_get_unlock_time(isp_usr_ctx *isp_ctx)
{
    if (g_usr_dfx_en == 0) {
        return;
    }

    isp_ctx->dfx_info.unlock_cur_time = get_sys_time_by_usec();
    isp_ctx->dfx_info.lock_time = (td_u32)(isp_ctx->dfx_info.unlock_cur_time - isp_ctx->dfx_info.lock_cur_time);
}

td_void isp_dfx_update_frame_edge_info(isp_usr_ctx *isp_ctx, isp_frame_edge_info *frame_edge_info)
{
    frame_edge_info->usr_dfx_en = (g_usr_dfx_en == 0) ? TD_FALSE : TD_TRUE;
    if (g_usr_dfx_en == 0) {
        return;
    }

    frame_edge_info->dfx_switch_time = (td_u32)(get_sys_time_by_usec() - isp_ctx->dfx_info.unlock_cur_time);
    frame_edge_info->dfx_lock_time = isp_ctx->dfx_info.lock_time;
    frame_edge_info->dfx_run_time = isp_ctx->dfx_info.run_time;
}

td_void isp_dfx_get_run_begin_time(isp_usr_ctx *isp_ctx)
{
    if (g_usr_dfx_en == 0) {
        return;
    }

    isp_ctx->dfx_info.run_begin_time = get_sys_time_by_usec();
}

td_void isp_dfx_get_run_end_time(isp_usr_ctx *isp_ctx)
{
    td_u64 end_time;
    if (g_usr_dfx_en == 0) {
        return;
    }

    end_time = get_sys_time_by_usec();
    isp_ctx->dfx_info.run_time = (td_u32)(end_time - isp_ctx->dfx_info.run_begin_time);
}
