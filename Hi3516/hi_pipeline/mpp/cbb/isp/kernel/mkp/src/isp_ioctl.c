/*
  Copyright (c), 2001-2025, Shenshu Tech. Co., Ltd.
 */
#include "isp_ioctl.h"

#include "isp.h"
#include "isp_drv.h"
#include "isp_drv_define.h"
#include "isp_drv_vreg.h"
#include "mm_ext.h"
#include "osal_ioctl.h"
#include "mkp_isp.h"
#include "isp_mem_share.h"
#include "isp_drv_dfx.h"
#include "isp_be_buf.h"
#include "isp_drv_stats.h"

static isp_version g_isp_lib_info = { { 0 } };

#define ISP_ALG_MOD_NAME_LENGTH  20

#ifdef CONFIG_OT_LOG_TRACE_SUPPORT
static td_char g_isp_alg_mod_name[OT_ISP_ALG_MOD_BUTT][ISP_ALG_MOD_NAME_LENGTH] = {
    "ae", "af", "awb", "blc", "dpc", "pregamma", "agamma", "drc", "demosaic", "antifalsecolor",
    "gamma", "crosstalk", "sharpen", "fswdr", "fpn", "dehaze", "local cac", "cac", "bayershp", "csc",
    "expander", "mcds", "auto color shading", "mesh shading", "rc", "rgbir", "hrs", "dgain", "bayer_nr", "flicker",
    "ldci", "ca", "clut", "ccm", "pq_ai", "hnr", "lblc"
};
#endif

td_s32 isp_drv_wait_condition_callback(const td_void *param)
{
    td_bool condition;

    condition = *(td_bool *)param;

    return (condition == TD_TRUE);
}

static td_void isp_get_last_run_time(ot_vi_pipe vi_pipe, isp_drv_ctx *drv_ctx,
    isp_frame_edge_info *frame_edge_info)
{
    td_u64 cur_run_time = 0;
    td_u32 last_run_time;
    if (ckfn_sys_entry() && ckfn_sys_get_time_stamp()) {
        cur_run_time = call_sys_get_time_stamp();
    }
    if (drv_ctx->drv_dbg_info.pre_run_time == 0) {
        drv_ctx->drv_dbg_info.pre_run_time = cur_run_time;
    }
    last_run_time = (td_u32)(cur_run_time - drv_ctx->drv_dbg_info.pre_run_time);
    drv_ctx->drv_dbg_info.last_run_time = last_run_time;
    drv_ctx->drv_dbg_info.run_cnt++;
    if (last_run_time > drv_ctx->drv_dbg_info.max_last_run_time && drv_ctx->drv_dbg_info.run_cnt > 3) { // 3:delay
        drv_ctx->drv_dbg_info.max_last_run_time = last_run_time;
    }
    if (drv_ctx->drv_dbg_info.run_cnt == 1000) { // 1000 frame reset
        drv_ctx->drv_dbg_info.run_cnt = 0;
        drv_ctx->drv_dbg_info.max_last_run_time = last_run_time;
    }
    drv_ctx->drv_dbg_info.usr_dfx_en = frame_edge_info->usr_dfx_en;
    if (drv_ctx->drv_dbg_info.last_cross_cnt != drv_ctx->drv_dbg_info.cross_frame_cnt) {
        if (frame_edge_info->usr_dfx_en == TD_TRUE) {
            drv_ctx->drv_dbg_info.cross_pre_run_t = drv_ctx->drv_dbg_info.record_run_t;
            drv_ctx->drv_dbg_info.cross_pre_swi_t = drv_ctx->drv_dbg_info.record_swi_t;
            drv_ctx->drv_dbg_info.cross_pre_lock_t = drv_ctx->drv_dbg_info.record_lock_t;

            drv_ctx->drv_dbg_info.cross_run_main_t = frame_edge_info->dfx_run_time;
            drv_ctx->drv_dbg_info.cross_switch_t = frame_edge_info->dfx_switch_time;
            drv_ctx->drv_dbg_info.cross_lock_t = frame_edge_info->dfx_lock_time;

            isp_warn_trace("ISP[%d] cross[%d(%d)] end_int_t:%u, usr_t:%u, "
                "(run, swi, lock): cur(%u, %u, %u), pre(%u, %u, %u)\n",
                vi_pipe, drv_ctx->drv_dbg_info.cross_frame_cnt, drv_ctx->drv_dbg_info.last_cross_cnt,
                drv_ctx->drv_dbg_info.cross_last_end_int_time, last_run_time, frame_edge_info->dfx_run_time,
                frame_edge_info->dfx_switch_time, frame_edge_info->dfx_lock_time,
                drv_ctx->drv_dbg_info.cross_pre_run_t, drv_ctx->drv_dbg_info.cross_pre_swi_t,
                drv_ctx->drv_dbg_info.cross_pre_lock_t);
        } else {
            isp_warn_trace("ISP[%d] cross[%d(%d)] end_int_t:%u, usr_t:%u\n",
                vi_pipe, drv_ctx->drv_dbg_info.cross_frame_cnt, drv_ctx->drv_dbg_info.last_cross_cnt,
                drv_ctx->drv_dbg_info.cross_last_end_int_time, last_run_time);
        }
        drv_ctx->drv_dbg_info.cross_last_run_time = last_run_time;
        drv_ctx->drv_dbg_info.last_cross_cnt = drv_ctx->drv_dbg_info.cross_frame_cnt;
    }
    if (frame_edge_info->usr_dfx_en == TD_TRUE) {
        drv_ctx->drv_dbg_info.record_run_t = frame_edge_info->dfx_run_time;
        drv_ctx->drv_dbg_info.record_swi_t = frame_edge_info->dfx_switch_time;
        drv_ctx->drv_dbg_info.record_lock_t = frame_edge_info->dfx_lock_time;
    }
    return;
}

static td_s32 isp_get_frame_edge(ot_vi_pipe vi_pipe, isp_frame_edge_info *frame_edge_info)
{
    unsigned long flags = 0;
    td_s32 ret;
    isp_drv_ctx *drv_ctx = TD_NULL;
    osal_spinlock *isp_spin_lock = TD_NULL;

    isp_check_pipe_return(vi_pipe);
    isp_check_pointer_return(frame_edge_info);

    drv_ctx = isp_drv_get_ctx(vi_pipe);
    isp_get_last_run_time(vi_pipe, drv_ctx, frame_edge_info);

    ret = osal_wait_timeout_interruptible(&drv_ctx->isp_wait, isp_drv_wait_condition_callback, &drv_ctx->edge,
        isp_drv_get_int_timeout(vi_pipe));
    if (ret <= 0) {
        frame_edge_info->int_status = 0;
        isp_warn_trace("Get Interrupt timeout failed!\n");
        return TD_FAILURE;
    }
    if (ckfn_sys_entry() && ckfn_sys_get_time_stamp()) {
        drv_ctx->drv_dbg_info.pre_run_time = call_sys_get_time_stamp();
    }
    isp_spin_lock = isp_drv_get_lock(vi_pipe);
    osal_spin_lock_irqsave(isp_spin_lock, &flags);
    drv_ctx->edge = TD_FALSE;
    frame_edge_info->int_status = drv_ctx->status;
    osal_spin_unlock_irqrestore(isp_spin_lock, &flags);

    return TD_SUCCESS;
}

static td_s32 isp_wait_event_interruptible(osal_wait *isp_wait,
    td_s32 (*func)(const td_void *), td_bool *flag, td_u32 milli_sec)
{
    td_s32 ret;

    if (milli_sec > 0) {
        ret = osal_wait_timeout_interruptible(isp_wait, func, flag, ((td_s32)milli_sec));
        if (ret == 0) {
            return OT_ERR_ISP_TIMEOUT;
        } else if (ret < 0) {
            return -ERESTARTSYS;
        }
    } else if (milli_sec == 0) {
        ret = osal_wait_interruptible(isp_wait, func, flag);
        if (ret) {
            return -ERESTARTSYS;
        }
    } else {
        return TD_SUCCESS;
    }
    return TD_SUCCESS;
}

static int isp_get_vd_start_time_out(ot_vi_pipe vi_pipe, td_u32 milli_sec, td_u32 *pu32status)
{
    unsigned long flags = 0;
    td_s32 ret;
    isp_drv_ctx *drv_ctx = TD_NULL;
    osal_spinlock *isp_spin_lock = TD_NULL;
    isp_check_pipe_return(vi_pipe);

    drv_ctx = isp_drv_get_ctx(vi_pipe);
    if (osal_sem_down(&drv_ctx->isp_sem)) {
        return -ERESTARTSYS;
    }
    isp_spin_lock = isp_drv_get_lock(vi_pipe);
    osal_spin_lock_irqsave(isp_spin_lock, &flags);
    drv_ctx->vd_start = TD_FALSE;
    osal_spin_unlock_irqrestore(isp_spin_lock, &flags);

    ret = isp_wait_event_interruptible(&drv_ctx->isp_wait_vd_start, isp_drv_wait_condition_callback,
        &drv_ctx->vd_start, milli_sec);
    if (ret != TD_SUCCESS) {
        osal_sem_up(&drv_ctx->isp_sem);
        return ret;
    }

    *pu32status = drv_ctx->status;

    osal_sem_up(&drv_ctx->isp_sem);

    return TD_SUCCESS;
}

static int isp_get_vd_end_time_out(ot_vi_pipe vi_pipe, td_u32 milli_sec, td_u32 *pu32status)
{
    unsigned long flags = 0;
    td_s32 ret;
    isp_drv_ctx *drv_ctx = TD_NULL;
    osal_spinlock *isp_spin_lock = TD_NULL;
    isp_check_pipe_return(vi_pipe);

    drv_ctx = isp_drv_get_ctx(vi_pipe);
    if (osal_sem_down(&drv_ctx->isp_sem_vd)) {
        return -ERESTARTSYS;
    }
    isp_spin_lock = isp_drv_get_lock(vi_pipe);
    osal_spin_lock_irqsave(isp_spin_lock, &flags);
    drv_ctx->vd_end = TD_FALSE;
    osal_spin_unlock_irqrestore(isp_spin_lock, &flags);

    ret = isp_wait_event_interruptible(&drv_ctx->isp_wait_vd_end, isp_drv_wait_condition_callback,
        &drv_ctx->vd_end, milli_sec);
    if (ret != TD_SUCCESS) {
        osal_sem_up(&drv_ctx->isp_sem_vd);
        return ret;
    }

    *pu32status = drv_ctx->status;

    osal_sem_up(&drv_ctx->isp_sem_vd);

    return TD_SUCCESS;
}

static int isp_get_vd_be_end_time_out(ot_vi_pipe vi_pipe, td_u32 milli_sec, td_u32 *pu32status)
{
    unsigned long flags = 0;
    td_s32 ret;
    isp_drv_ctx *drv_ctx = TD_NULL;
    osal_spinlock *isp_spin_lock = TD_NULL;
    isp_check_pipe_return(vi_pipe);

    drv_ctx = isp_drv_get_ctx(vi_pipe);
    if (osal_sem_down(&drv_ctx->isp_sem_be_vd)) {
        return -ERESTARTSYS;
    }
    isp_spin_lock = isp_drv_get_lock(vi_pipe);
    osal_spin_lock_irqsave(isp_spin_lock, &flags);
    if (drv_ctx->run_once_flag != TD_TRUE) {
        drv_ctx->vd_be_end = TD_FALSE;
    }
    osal_spin_unlock_irqrestore(isp_spin_lock, &flags);

    ret = isp_wait_event_interruptible(&drv_ctx->isp_wait_vd_be_end, isp_drv_wait_condition_callback,
        &drv_ctx->vd_be_end, milli_sec);
    if (ret != TD_SUCCESS) {
        osal_sem_up(&drv_ctx->isp_sem_be_vd);
        return ret;
    }

    *pu32status = 1;

    osal_sem_up(&drv_ctx->isp_sem_be_vd);

    return TD_SUCCESS;
}
#ifdef CONFIG_OT_SYS_KOL_SUPPORT
static int isp_get_vd_run_end_time_out(ot_vi_pipe vi_pipe, td_u32 milli_sec, td_u32 *pu32status)
{
    unsigned long flags = 0;
    td_s32 ret;
    isp_drv_ctx *drv_ctx = TD_NULL;
    osal_spinlock *isp_spin_lock = TD_NULL;
    isp_check_pipe_return(vi_pipe);

    drv_ctx = isp_drv_get_ctx(vi_pipe);
    if (osal_sem_down(&drv_ctx->isp_sem_run_end_vd)) {
        return -ERESTARTSYS;
    }
    isp_spin_lock = isp_drv_get_lock(vi_pipe);
    osal_spin_lock_irqsave(isp_spin_lock, &flags);
    drv_ctx->vd_run_end = TD_FALSE;
    osal_spin_unlock_irqrestore(isp_spin_lock, &flags);

    if (milli_sec != 0) {
        ret = osal_wait_timeout_interruptible(&drv_ctx->isp_wait_vd_run_end, isp_drv_wait_condition_callback,
            &drv_ctx->vd_run_end, (milli_sec));
        if (ret <= 0) {
            osal_sem_up(&drv_ctx->isp_sem_run_end_vd);
            return TD_FAILURE;
        }
    } else {
        ret = osal_wait_interruptible(&drv_ctx->isp_wait_vd_run_end, isp_drv_wait_condition_callback,
            &drv_ctx->vd_run_end);
        if (ret) {
            osal_sem_up(&drv_ctx->isp_sem_run_end_vd);
            return TD_FAILURE;
        }
    }

    *pu32status = 1;

    osal_sem_up(&drv_ctx->isp_sem_run_end_vd);

    return TD_SUCCESS;
}

static int isp_get_vd_standby_end_time_out(ot_vi_pipe vi_pipe, td_u32 milli_sec, td_u32 *pu32status)
{
    unsigned long flags = 0;
    td_s32 ret;
    isp_drv_ctx *drv_ctx = TD_NULL;
    osal_spinlock *isp_spin_lock = TD_NULL;
    isp_check_pipe_return(vi_pipe);

    drv_ctx = isp_drv_get_ctx(vi_pipe);
    if (osal_sem_down(&drv_ctx->isp_sem_standby_vd)) {
        return -ERESTARTSYS;
    }
    isp_spin_lock = isp_drv_get_lock(vi_pipe);
    osal_spin_lock_irqsave(isp_spin_lock, &flags);
    drv_ctx->vd_standby_end = TD_FALSE;
    osal_spin_unlock_irqrestore(isp_spin_lock, &flags);

    ret = isp_wait_event_interruptible(&drv_ctx->isp_wait_vd_standby_end, isp_drv_wait_condition_callback,
        &drv_ctx->vd_standby_end, milli_sec);
    if (ret != TD_SUCCESS) {
        osal_sem_up(&drv_ctx->isp_sem_standby_vd);
        return ret;
    }

    *pu32status = drv_ctx->status;

    osal_sem_up(&drv_ctx->isp_sem_standby_vd);

    return TD_SUCCESS;
}

#endif
static td_s32 isp_drv_set_isp_run_state(ot_vi_pipe vi_pipe, td_u64 *hand_signal)
{
    isp_drv_ctx *drv_ctx = TD_NULL;

    isp_check_pipe_return(vi_pipe);
    isp_check_pointer_return(hand_signal);

    drv_ctx = isp_drv_get_ctx(vi_pipe);

    if (*hand_signal == ISP_INIT_HAND_SIGNAL) {
        drv_ctx->isp_run_flag = TD_TRUE;
    } else if (*hand_signal == ISP_EXIT_HAND_SIGNAL) {
        drv_ctx->isp_run_flag = TD_FALSE;
        isp_drv_reset_fe_cfg(vi_pipe);
    } else {
        isp_err_trace("ISP[%d] set isp run state failed!\n", vi_pipe);
        return TD_FAILURE;
    }

    return TD_SUCCESS;
}

static td_s32 isp_drv_get_wdr_attr(ot_vi_pipe vi_pipe, vi_pipe_wdr_attr *wdr_attr)
{
    td_u32 i;
    td_u32 num;
    td_s32 ret;
    vi_pipe_wdr_attr wdr_attr_str = { 0 };
    vi_pipe_wdr_attr wdr_attr_str_tmp = { 0 };
    isp_drv_ctx *drv_ctx = TD_NULL;

    isp_check_pipe_return(vi_pipe);
    isp_check_pointer_return(wdr_attr);

    drv_ctx = isp_drv_get_ctx(vi_pipe);
    if (!ckfn_vi_get_pipe_wdr_attr()) {
        isp_err_trace("pipe[%d] ckfn_vi_get_pipe_wdr_attr is null\n", vi_pipe);
        return TD_FAILURE;
    }

    ret = call_vi_get_pipe_wdr_attr(vi_pipe, &wdr_attr_str);
    if (ret != TD_SUCCESS) {
        return ret;
    }
    /* Not WDR mode,BindPipe attr update */
    if (!is_fs_wdr_mode(wdr_attr_str.wdr_mode) && (wdr_attr_str.pipe_num != 1)) {
        wdr_attr_str.pipe_num = 1;
        wdr_attr_str.pipe_id[0] = vi_pipe;
    }

    num = wdr_attr_str.pipe_num;
    if ((num < 1) || (num > ISP_WDR_CHN_MAX)) {
        isp_err_trace("pipe[%d] Err wdr bind num %d!\n", vi_pipe, num);
        return OT_ERR_ISP_ILLEGAL_PARAM;
    }
    for (i = 0; i < num; i++) {
        isp_check_pipe_return(wdr_attr_str.pipe_id[i]);
    }
    if ((wdr_attr_str.wdr_mode > OT_WDR_MODE_BUTT)) {
        isp_err_trace("pipe[%d] Err wdr mode %d!\n", vi_pipe, wdr_attr_str.wdr_mode);
        return OT_ERR_ISP_ILLEGAL_PARAM;
    }

    if ((is_half_wdr_mode(wdr_attr_str.wdr_mode) == TD_TRUE) &&
        (isp_drv_get_run_wakeup_sel(vi_pipe) == OT_ISP_RUN_WAKEUP_BE_END)) {
        isp_err_trace("cur pipe%d run wakeup select = be_end, not support frame wdr!\n", vi_pipe);
        return OT_ERR_ISP_NOT_SUPPORT;
    }

    (td_void)memcpy_s(&wdr_attr_str_tmp, sizeof(vi_pipe_wdr_attr), &wdr_attr_str, sizeof(vi_pipe_wdr_attr));

    for (i = 0; i < num; i++) {
        wdr_attr_str_tmp.pipe_id[i] = wdr_attr_str.pipe_id[num - 1 - i];
    }
    (td_void)memcpy_s(&drv_ctx->wdr_attr, sizeof(vi_pipe_wdr_attr), &wdr_attr_str_tmp, sizeof(vi_pipe_wdr_attr));
    (td_void)memcpy_s(wdr_attr, sizeof(vi_pipe_wdr_attr), &wdr_attr_str_tmp, sizeof(vi_pipe_wdr_attr));

    return TD_SUCCESS;
}
#ifdef CONFIG_OT_VI_DISTRIBUTE_GRP
static td_s32 isp_drv_get_dist_attr(ot_vi_pipe vi_pipe, vi_distribute_attr *attr)
{
    td_s32 ret;
    vi_distribute_attr attr_tmp = { 0 };

    isp_drv_ctx *drv_ctx = TD_NULL;

    isp_check_pipe_return(vi_pipe);
    isp_check_pointer_return(attr);

    drv_ctx = isp_drv_get_ctx(vi_pipe);
    if (!ckfn_vi_get_distribute_attr()) {
        isp_err_trace("pipe[%d] ckfn_vi_get_dist_attr is null\n", vi_pipe);
        return TD_FAILURE;
    }

    ret = call_vi_get_distribute_attr(vi_pipe, &attr_tmp);
    if (ret != TD_SUCCESS) {
        isp_err_trace("pipe[%d] vi_get_dist_attr error\n", vi_pipe);
        return ret;
    }

    if (attr_tmp.distribute_en != TD_TRUE) {
        (td_void)memset_s(&drv_ctx->dist_attr, sizeof(vi_distribute_attr), 0, sizeof(vi_distribute_attr));
        (td_void)memset_s(attr, sizeof(vi_distribute_attr), 0, sizeof(vi_distribute_attr));

        return TD_SUCCESS;
    }

    isp_check_pipe_return(attr_tmp.pipe_id[0]);
    isp_check_pipe_return(attr_tmp.pipe_id[1]);
    // pipe_id must be [phy_pipe, virt_pipe]
    if (is_virt_pipe(attr_tmp.pipe_id[0]) || !is_virt_pipe(attr_tmp.pipe_id[1])) {
        isp_err_trace("pipe[%d] dist pipe_id[%d, %d] error\n", vi_pipe, attr_tmp.pipe_id[0], attr_tmp.pipe_id[1]);
        return OT_ERR_ISP_ILLEGAL_PARAM;
    }

    (td_void)memcpy_s(&drv_ctx->dist_attr, sizeof(vi_distribute_attr), &attr_tmp, sizeof(vi_distribute_attr));
    (td_void)memcpy_s(attr, sizeof(vi_distribute_attr), &attr_tmp, sizeof(vi_distribute_attr));

    return TD_SUCCESS;
}

static td_s32 isp_drv_get_dist_fps_status(ot_vi_pipe vi_pipe, vi_distribute_attr *dist_attr, void *arg)
{
    td_u8 i, j;
    isp_drv_ctx *tmp_drv_ctx = TD_NULL;
    isp_drv_ctx *drv_ctx = TD_NULL;
    ot_vi_pipe tmp_pipe;
    unsigned long flags = 0;

    tmp_drv_ctx = isp_drv_get_ctx(vi_pipe);

    if (dist_attr->grp_share_vs == TD_FALSE) {
        for (j = 0; j < OT_VI_MAX_DISTRIBUTE_PIPE_NUM; ++j) {
            tmp_pipe = tmp_drv_ctx->dist_attr.pipe_id[j];
            drv_ctx = isp_drv_get_ctx(tmp_pipe);

            osal_spin_lock_irqsave(isp_drv_get_lock(tmp_pipe), &flags);

            if (drv_ctx->is_chg_fps_done == TD_FALSE) {
                *(td_bool *)arg =  TD_FALSE;
                osal_spin_unlock_irqrestore(isp_drv_get_lock(tmp_pipe), &flags);
                return TD_SUCCESS;
            }
            if (j == OT_VI_MAX_DISTRIBUTE_PIPE_NUM - 1) {
                *(td_bool *)arg = TD_TRUE;
                drv_ctx->is_chg_fps_done = TD_FALSE;
                drv_ctx->chg_fps_cnt = 0;
            }
            osal_spin_unlock_irqrestore(isp_drv_get_lock(tmp_pipe), &flags);
        }

        return TD_SUCCESS;
    }

    for (i = 0; i < OT_ISP_MAX_PHY_PIPE_NUM; ++i) {
        tmp_drv_ctx = isp_drv_get_ctx(i);
        for (j = 0; j < OT_VI_MAX_DISTRIBUTE_PIPE_NUM; ++j) {
            tmp_pipe = tmp_drv_ctx->dist_attr.pipe_id[j];
            drv_ctx = isp_drv_get_ctx(tmp_pipe);

            osal_spin_lock_irqsave(isp_drv_get_lock(tmp_pipe), &flags);
            if (drv_ctx->is_chg_fps_done == TD_FALSE) {
                *(td_bool *)arg =  TD_FALSE;
                osal_spin_unlock_irqrestore(isp_drv_get_lock(tmp_pipe), &flags);
                return TD_SUCCESS;
            }
            if (j == OT_VI_MAX_DISTRIBUTE_PIPE_NUM - 1 && i == OT_ISP_MAX_PHY_PIPE_NUM - 1) {
                *(td_bool *)arg = TD_TRUE;
                drv_ctx->is_chg_fps_done = TD_FALSE;
                drv_ctx->chg_fps_cnt = 0;
            }
            osal_spin_unlock_irqrestore(isp_drv_get_lock(tmp_pipe), &flags);
        }
    }

    return TD_SUCCESS;
}
#endif

static td_s32 isp_drv_get_pipe_size(ot_vi_pipe vi_pipe, ot_size *pipe_size)
{
    td_s32 ret;
    ot_size pipe_size_str = { 0 };
    ot_vi_pipe phy_pipe = vi_pipe;

    isp_drv_dist_trans_pipe(&phy_pipe);
    isp_check_pipe_return(vi_pipe);
    isp_check_pointer_return(pipe_size);
    isp_check_vir_pipe_return(phy_pipe);

    if (!ckfn_vi_get_pipe_in_size()) {
        isp_err_trace("pipe[%d] ckfn_vi_get_pipe_bind_dev_size is null\n", vi_pipe);
        return TD_FAILURE;
    }

    ret = call_vi_get_pipe_in_size(phy_pipe, &pipe_size_str);
    if (ret != TD_SUCCESS) {
        isp_err_trace("pipe[%d] call_vi_get_pipe_bind_dev_size failed 0x%x!\n", vi_pipe, ret);
        return ret;
    }

    // input param check: check only the strict maximum and minimum resolutions.
    if ((pipe_size_str.width < OT_ISP_WIDTH_MIN) || (pipe_size_str.width > OT_ISP_SENSOR_WIDTH_MAX) ||
        (pipe_size_str.height < OT_ISP_HEIGHT_MIN) || (pipe_size_str.height > OT_ISP_SENSOR_HEIGHT_MAX)) {
        isp_err_trace("pipe[%d]: Image Width should between [%d, %d], Height should between[%d, %d]\n", vi_pipe,
            OT_ISP_WIDTH_MIN, OT_ISP_SENSOR_WIDTH_MAX, OT_ISP_HEIGHT_MIN, OT_ISP_SENSOR_HEIGHT_MAX);
        return TD_FAILURE;
    }

    (td_void)memcpy_s(pipe_size, sizeof(ot_size), &pipe_size_str, sizeof(ot_size));

    return TD_SUCCESS;
}

static td_s32 isp_drv_get_hdr_attr(ot_vi_pipe vi_pipe, vi_pipe_hdr_attr *hdr_attr)
{
    td_s32 ret;
    vi_pipe_hdr_attr hdr_attr_str = { 0 };

    isp_check_pipe_return(vi_pipe);
    isp_check_pointer_return(hdr_attr);

    if (!ckfn_vi_get_pipe_hdr_attr()) {
        isp_err_trace("pipe[%d] ckfn_vi_get_pipe_hdr_attr is null\n", vi_pipe);
        return TD_FAILURE;
    }

    ret = call_vi_get_pipe_hdr_attr(vi_pipe, &hdr_attr_str);
    if (ret != TD_SUCCESS) {
        isp_err_trace("pipe[%d] call_vi_get_pipe_hdr_attr failed 0x%x!\n", vi_pipe, ret);
        return ret;
    }

    (td_void)memcpy_s(hdr_attr, sizeof(vi_pipe_hdr_attr), &hdr_attr_str, sizeof(vi_pipe_hdr_attr));

    return TD_SUCCESS;
}

#ifdef CONFIG_OT_VI_STITCH_GRP
td_s32 isp_drv_get_stitch_attr(ot_vi_pipe vi_pipe, vi_stitch_attr *stitch_attr)
{
    td_u8 i;
    td_s32 ret;
    isp_drv_ctx *drv_ctx = TD_NULL;
    vi_stitch_attr stitch_attr_str = { 0 };
    isp_check_pipe_return(vi_pipe);
    isp_check_pointer_return(stitch_attr);
    drv_ctx = isp_drv_get_ctx(vi_pipe);
    if (!ckfn_vi_get_pipe_stitch_attr()) {
        isp_err_trace("pipe[%d] ckfn_vi_get_pipe_stitch_attr is null\n", vi_pipe);
        return TD_FAILURE;
    }

    ret = call_vi_get_pipe_stitch_attr(vi_pipe, &stitch_attr_str);
    if (ret != TD_SUCCESS) {
        isp_err_trace("pipe[%d] call_vi_get_pipe_stitch_attr failed 0x%x!\n", vi_pipe, ret);
        return ret;
    }

    if (stitch_attr_str.stitch_enable) {
        if ((stitch_attr_str.stitch_pipe_num < 1) || (stitch_attr_str.stitch_pipe_num > OT_ISP_MAX_STITCH_NUM)) {
            isp_err_trace("pipe[%d] err stitch num %d\n", vi_pipe, stitch_attr_str.stitch_pipe_num);
            return OT_ERR_ISP_ILLEGAL_PARAM;
        }
        for (i = 0; i < stitch_attr_str.stitch_pipe_num; i++) {
            isp_check_pipe_return(stitch_attr_str.stitch_bind_id[i]);
        }
    }
    (td_void)memcpy_s(&drv_ctx->stitch_attr, sizeof(vi_stitch_attr), &stitch_attr_str, sizeof(vi_stitch_attr));
    (td_void)memcpy_s(stitch_attr, sizeof(vi_stitch_attr), &stitch_attr_str, sizeof(vi_stitch_attr));
    return TD_SUCCESS;
}
#endif

#ifdef CONFIG_OT_ISP_DETAIL_STATS_SUPPORT
static td_s32 isp_drv_check_detail_stats_param(ot_vi_pipe vi_pipe, isp_drv_ctx *drv_ctx)
{
    if (drv_ctx->isp_init != TD_TRUE) {
        isp_err_trace("pipe:%d, isp is not inited:%d\n", vi_pipe, drv_ctx->isp_init);
        return OT_ERR_ISP_NOT_INIT;
    }
    if (drv_ctx->detail_stats_cfg.enable != TD_TRUE) {
        isp_err_trace("pipe:%d, detail_stats is not enable, not support get detail_stats\n", vi_pipe);
        return OT_ERR_ISP_NOT_SUPPORT;
    }
    if (drv_ctx->detail_stats_buf.init != TD_TRUE) {
        isp_err_trace("pipe:%d, detail_stats buf is not inited:%d\n", vi_pipe, drv_ctx->detail_stats_buf.init);
        return OT_ERR_ISP_NOT_INIT;
    }

    return TD_SUCCESS;
}
#endif
static td_s32 isp_drv_get_version(isp_version *version)
{
    isp_check_pointer_return(version);

    (td_void)memcpy_s(&g_isp_lib_info, sizeof(isp_version), version, sizeof(isp_version));

    g_isp_lib_info.magic = VERSION_MAGIC + ISP_MAGIC_OFFSET;
    (td_void)memcpy_s(version, sizeof(isp_version), &g_isp_lib_info, sizeof(isp_version));

    return TD_SUCCESS;
}

static td_s32 isp_drv_proc_init(ot_vi_pipe vi_pipe, isp_proc_mem *proc_mem)
{
#ifdef CONFIG_OT_PROC_SHOW_SUPPORT
    td_s32 ret;
    td_phys_addr_t phy_addr;
    td_u8 *vir_addr = TD_NULL;
    isp_drv_ctx *drv_ctx = TD_NULL;
    td_char ac_name[MAX_MMZ_NAME_LEN] = { 0 };
    mm_malloc_param malloc_param = {0};

    isp_check_pipe_return(vi_pipe);
    isp_check_pointer_return(proc_mem);

    if (isp_drv_get_proc_param(vi_pipe) == 0) {
        return TD_SUCCESS;
    }

    drv_ctx = isp_drv_get_ctx(vi_pipe);

    isp_check_buf_init_return(vi_pipe, drv_ctx->porc_mem.init);

    ret = snprintf_s(ac_name, sizeof(ac_name), sizeof(ac_name) - 1, "isp[%d].proc", vi_pipe);
    if (ret < 0) {
        return TD_FAILURE;
    }

    malloc_param.buf_name = ac_name;
    malloc_param.size = ISP_PROC_SIZE;
    malloc_param.kernel_only = TD_FALSE;
    ret = cmpi_mmz_malloc_cached(&malloc_param, &phy_addr, (td_void **)&vir_addr);
    if (ret != TD_SUCCESS) {
        isp_err_trace("alloc proc buf err\n");
        return OT_ERR_ISP_NOMEM;
    }

    ((td_char *)vir_addr)[0] = '\0';
    ((td_char *)vir_addr)[ISP_PROC_SIZE - 1] = '\0';

    if (osal_sem_down(&drv_ctx->proc_sem)) {
        if (phy_addr != 0) {
            cmpi_mmz_free(phy_addr, vir_addr);
        }
        return -ERESTARTSYS;
    }

    drv_ctx->porc_mem.init = TD_TRUE;
    drv_ctx->porc_mem.phy_addr = phy_addr;
    drv_ctx->porc_mem.size = ISP_PROC_SIZE;
    drv_ctx->porc_mem.virt_addr = (td_void *)vir_addr;

    (td_void)memcpy_s(proc_mem, sizeof(isp_proc_mem), &drv_ctx->porc_mem, sizeof(isp_proc_mem));

    osal_sem_up(&drv_ctx->proc_sem);
#endif
    return TD_SUCCESS;
}

static td_s32 isp_drv_proc_exit(ot_vi_pipe vi_pipe)
{
#ifdef CONFIG_OT_PROC_SHOW_SUPPORT
    isp_drv_ctx *drv_ctx = TD_NULL;
    td_phys_addr_t phy_addr;
    td_u8 *vir_addr = TD_NULL;

    isp_check_pipe_return(vi_pipe);

    if (isp_drv_get_proc_param(vi_pipe) == 0) {
        return TD_SUCCESS;
    }

    drv_ctx = isp_drv_get_ctx(vi_pipe);
    isp_check_exit_state_return(vi_pipe, drv_ctx->isp_run_flag);

    isp_check_buf_exit_return(vi_pipe, drv_ctx->porc_mem.init);

    phy_addr = drv_ctx->porc_mem.phy_addr;
    vir_addr = (td_u8 *)drv_ctx->porc_mem.virt_addr;

    if (osal_sem_down(&drv_ctx->proc_sem)) {
        return -ERESTARTSYS;
    }

    drv_ctx->porc_mem.init = TD_FALSE;
    drv_ctx->porc_mem.phy_addr = 0;
    drv_ctx->porc_mem.size = 0;
    drv_ctx->porc_mem.virt_addr = TD_NULL;
    osal_sem_up(&drv_ctx->proc_sem);

    if (phy_addr != 0) {
        cmpi_mmz_free(phy_addr, vir_addr);
    }
#endif
    return TD_SUCCESS;
}

static td_void isp_drv_trans_info_addr_init(isp_drv_ctx *drv_ctx, td_phys_addr_t phy_addr, td_u8 *vir_addr)
{
    td_u32 size_dng_info, size_frame_info, size_attach_info, size_color_gammut;
    size_dng_info = sizeof(ot_isp_dng_image_static_info);
    size_attach_info = sizeof(ot_isp_attach_info);
    size_color_gammut = sizeof(ot_isp_colorgammut_info);
    size_frame_info = sizeof(ot_isp_frame_info) * ISP_MAX_FRAMEINFO_BUF_NUM;

    drv_ctx->trans_info.init = TD_TRUE;
    drv_ctx->trans_info.dng_info.phy_addr = phy_addr;
    drv_ctx->trans_info.dng_info.vir_addr = (td_void *)vir_addr;

    drv_ctx->trans_info.atta_info.phy_addr = drv_ctx->trans_info.dng_info.phy_addr + size_dng_info;
    drv_ctx->trans_info.atta_info.vir_addr =
        (td_void *)((td_u8 *)drv_ctx->trans_info.dng_info.vir_addr + size_dng_info);

    drv_ctx->trans_info.color_gammut_info.phy_addr = drv_ctx->trans_info.atta_info.phy_addr + size_attach_info;
    drv_ctx->trans_info.color_gammut_info.vir_addr =
        (td_void *)((td_u8 *)drv_ctx->trans_info.atta_info.vir_addr + size_attach_info);

    drv_ctx->trans_info.frame_info.phy_addr = drv_ctx->trans_info.color_gammut_info.phy_addr + size_color_gammut;
    drv_ctx->trans_info.frame_info.vir_addr =
        (td_void *)((td_u8 *)drv_ctx->trans_info.color_gammut_info.vir_addr + size_color_gammut);

    drv_ctx->trans_info.update_info.phy_addr = drv_ctx->trans_info.frame_info.phy_addr + size_frame_info;
    drv_ctx->trans_info.update_info.vir_addr =
        (td_void *)((td_u8 *)drv_ctx->trans_info.frame_info.vir_addr + size_frame_info);
}

static td_s32 isp_drv_trans_info_buf_init(ot_vi_pipe vi_pipe, isp_trans_info_buf *trans_info)
{
    td_s32 ret;
    isp_drv_ctx *drv_ctx = TD_NULL;
    td_phys_addr_t phy_addr;
    td_u8 *vir_addr = TD_NULL;
    td_u32 size_dng_info, size_update_info, size_frame_info, size_attach_info, size_color_gammut;
    td_char ac_name[MAX_MMZ_NAME_LEN] = { 0 };
    unsigned long flags = 0;
    osal_spinlock *isp_spin_lock = TD_NULL;
    mm_malloc_param malloc_param = {0};

    isp_check_pipe_return(vi_pipe);
    isp_check_pointer_return(trans_info);

    drv_ctx = isp_drv_get_ctx(vi_pipe);

    isp_check_buf_init_return(vi_pipe, drv_ctx->trans_info.init);

    ret = snprintf_s(ac_name, sizeof(ac_name), sizeof(ac_name) - 1, "isp[%d].trans", vi_pipe);
    if (ret < 0) {
        return TD_FAILURE;
    }

    size_dng_info = sizeof(ot_isp_dng_image_static_info);
    size_attach_info = sizeof(ot_isp_attach_info);
    size_color_gammut = sizeof(ot_isp_colorgammut_info);
    size_frame_info = sizeof(ot_isp_frame_info) * ISP_MAX_FRAMEINFO_BUF_NUM;
    size_update_info = sizeof(ot_isp_dcf_update_info) * ISP_MAX_UPDATEINFO_BUF_NUM + sizeof(ot_isp_dcf_const_info);

    malloc_param.buf_name = ac_name;
    malloc_param.size = size_dng_info + size_attach_info + size_color_gammut + size_frame_info + size_update_info;
    malloc_param.kernel_only = TD_FALSE;
    ret = cmpi_mmz_malloc_nocache(&malloc_param, &phy_addr, (td_void **)&vir_addr);
    if (ret != TD_SUCCESS) {
        isp_err_trace("ISP[%d] alloc ISP Trans info buf err\n", vi_pipe);
        return OT_ERR_ISP_NOMEM;
    }

    (td_void)memset_s(vir_addr, malloc_param.size, 0, malloc_param.size);
    isp_spin_lock = isp_drv_get_lock(vi_pipe);
    osal_spin_lock_irqsave(isp_spin_lock, &flags);

    isp_drv_trans_info_addr_init(drv_ctx, phy_addr, vir_addr);
    (td_void)memcpy_s(trans_info, sizeof(isp_trans_info_buf), &drv_ctx->trans_info, sizeof(isp_trans_info_buf));

    osal_spin_unlock_irqrestore(isp_spin_lock, &flags);

    return TD_SUCCESS;
}

static td_s32 isp_drv_trans_info_buf_exit(ot_vi_pipe vi_pipe)
{
    isp_drv_ctx *drv_ctx = TD_NULL;
    td_phys_addr_t phy_addr;
    td_u8 *vir_addr = TD_NULL;
    unsigned long flags = 0;
    osal_spinlock *isp_spin_lock = TD_NULL;

    isp_check_pipe_return(vi_pipe);

    drv_ctx = isp_drv_get_ctx(vi_pipe);
    isp_check_exit_state_return(vi_pipe, drv_ctx->isp_run_flag);

    isp_check_buf_exit_return(vi_pipe, drv_ctx->trans_info.init);
    isp_spin_lock = isp_drv_get_lock(vi_pipe);
    osal_spin_lock_irqsave(isp_spin_lock, &flags);

    phy_addr = drv_ctx->trans_info.dng_info.phy_addr;
    vir_addr = (td_u8 *)drv_ctx->trans_info.dng_info.vir_addr;

    drv_ctx->trans_info.init = TD_FALSE;

    drv_ctx->trans_info.dng_info.phy_addr = 0;
    drv_ctx->trans_info.dng_info.vir_addr = TD_NULL;

    drv_ctx->trans_info.atta_info.phy_addr = 0;
    drv_ctx->trans_info.atta_info.vir_addr = TD_NULL;

    drv_ctx->trans_info.color_gammut_info.phy_addr = 0;
    drv_ctx->trans_info.color_gammut_info.vir_addr = TD_NULL;

    drv_ctx->trans_info.frame_info.phy_addr = 0;
    drv_ctx->trans_info.frame_info.vir_addr = TD_NULL;

    drv_ctx->trans_info.update_info.phy_addr = 0;
    drv_ctx->trans_info.update_info.vir_addr = TD_NULL;

    osal_spin_unlock_irqrestore(isp_spin_lock, &flags);

    if (phy_addr != 0) {
        cmpi_mmz_free(phy_addr, vir_addr);
    }

    return TD_SUCCESS;
}

static td_s32 isp_drv_update_ldci_normal_online_attr(ot_vi_pipe vi_pipe)
{
    td_u8 write_buf_idx;
    isp_drv_ctx *drv_ctx = TD_NULL;

    isp_check_pipe_return(vi_pipe);

    drv_ctx = isp_drv_get_ctx(vi_pipe);

    write_buf_idx = drv_ctx->ldci_write_buf_attr.buf_idx;

    drv_ctx->ldci_read_buf_attr.buf_idx = write_buf_idx;
    drv_ctx->ldci_write_buf_attr.buf_idx = (write_buf_idx + 1) % div_0_to_1(drv_ctx->ldci_write_buf_attr.buf_num);

    return TD_SUCCESS;
}

static td_s32 isp_drv_update_ldci_tpr_online_attr(ot_vi_pipe vi_pipe, isp_stat *stat)
{
    td_u8 cur_read_buf_idx;
    isp_drv_ctx *drv_ctx = TD_NULL;
    isp_ldci_stat *ldci_stat = TD_NULL;

    isp_check_pipe_return(vi_pipe);
    isp_check_pointer_return(stat);

    drv_ctx = isp_drv_get_ctx(vi_pipe);

    cur_read_buf_idx = drv_ctx->ldci_write_buf_attr.buf_idx;

    /* Get LDCI Statistics from WriteBuffer(copy statistics to stat), then update WriteSttAddr */
    ldci_stat = (isp_ldci_stat *)drv_ctx->ldci_write_buf_attr.ldci_buf[cur_read_buf_idx].vir_addr;

    if (ldci_stat != TD_NULL) {
        (td_void)memcpy_s(&stat->ldci_stat, sizeof(isp_ldci_stat), ldci_stat, sizeof(isp_ldci_stat));
    }
    drv_ctx->ldci_write_buf_attr.buf_idx = (cur_read_buf_idx + 1) % div_0_to_1(drv_ctx->ldci_write_buf_attr.buf_num);

    return TD_SUCCESS;
}

td_s32 isp_drv_ldci_online_attr_update(ot_vi_pipe vi_pipe, isp_stat *stat)
{
    isp_check_pipe_return(vi_pipe);

    if (isp_drv_get_ldci_tpr_flt_en(vi_pipe) == TD_TRUE) {
        /* Copy LDCI statistics information to stat, then update LDCI WriteSttAddr */
        isp_drv_update_ldci_tpr_online_attr(vi_pipe, stat);
    } else {
        /* Only update LDCI ReadSttAddr and WriteSttAddr */
        isp_drv_update_ldci_normal_online_attr(vi_pipe);
    }
    return TD_SUCCESS;
}

td_s32 isp_drv_switch_be_offline_stt_info(ot_vi_pipe vi_pipe, td_u32 viproc_id,
    td_u32 stat_valid, td_s32 work_id)
{
    isp_drv_ctx *drv_ctx = TD_NULL;
    isp_check_pipe_return(vi_pipe);
    drv_ctx = isp_drv_get_ctx(vi_pipe);
    isp_check_viproc_id_return(drv_ctx->work_mode.running_mode, viproc_id);
    isp_check_bool_return(work_id); /* work_id only 0, 1 */
    if (drv_ctx->isp_init != TD_TRUE) {
        return TD_FAILURE;
    }
    if (is_online_mode(drv_ctx->work_mode.running_mode)) {
        return TD_SUCCESS;
    }
    drv_ctx->be_off_stt_buf.last_ready_be_stt_idx[viproc_id] = work_id;
    isp_drv_switch_be_offline_stt_buf_index(vi_pipe, viproc_id, drv_ctx, stat_valid);

#ifdef CONFIG_OT_VI_STITCH_GRP
    return isp_drv_stitch_sync_be_stt_info(vi_pipe, viproc_id, drv_ctx);
#else
    return TD_SUCCESS;
#endif
}

td_s32 isp_drv_get_be_offline_stt_addr(ot_vi_pipe vi_pipe, td_u32 viproc_id, td_u64 pts,
                                       isp_be_offline_stt_buf *be_offline_stt_addr, td_s32 *work_id)
{
    isp_drv_ctx *drv_ctx = TD_NULL;

    isp_check_pipe_return(vi_pipe);
    isp_check_pointer_return(be_offline_stt_addr);
    isp_check_pointer_return(work_id);
    drv_ctx = isp_drv_get_ctx(vi_pipe);

    isp_check_viproc_id_return(drv_ctx->work_mode.running_mode, viproc_id);
    if (drv_ctx->isp_init != TD_TRUE) {
        return TD_FAILURE;
    }
    if (is_online_mode(drv_ctx->work_mode.running_mode)) {
        return TD_FAILURE;
    }

    isp_drv_be_offline_addr_info_update(vi_pipe, viproc_id, pts, be_offline_stt_addr);

    *work_id = drv_ctx->be_off_stt_buf.working_be_stt_idx[viproc_id];
    drv_ctx->be_off_stt_buf.working_be_stt_idx[viproc_id] = 1 - *work_id;

    return TD_SUCCESS;
}

static td_s32 isp_drv_drc_buf_init(ot_vi_pipe vi_pipe)
{
    td_s32 ret;
    unsigned long flags = 0;
    td_phys_addr_t phy_addr;
    td_u8 *vir_addr = TD_NULL;
    isp_drv_ctx *drv_ctx = TD_NULL;
    isp_viproc_reg_type *post_viproc[OT_ISP_STRIPING_MAX_NUM] = { TD_NULL };
    td_char ac_name[MAX_MMZ_NAME_LEN] = { 0 };
    osal_spinlock *isp_spin_lock = TD_NULL;
    mm_malloc_param malloc_param = {0};
    td_bool is_alloc = TD_FALSE;
    isp_check_pipe_return(vi_pipe);
    drv_ctx = isp_drv_get_ctx(vi_pipe);

    is_alloc = isp_drv_get_drc_alloc_flag(drv_ctx->work_mode.running_mode);
    if (is_alloc == TD_FALSE) {
        return TD_SUCCESS;
    }
    isp_check_buf_init_return(vi_pipe, drv_ctx->drc_buf_attr.init);

    if (drv_ctx->work_mode.running_mode == ISP_MODE_RUNNING_ONLINE) {
        if (isp_drv_get_post_viproc_reg_virt_addr(vi_pipe, post_viproc) != TD_SUCCESS) {
            isp_err_trace("ISP[%d] get viproc reg fail!\n", vi_pipe);
            return TD_FAILURE;
        }
    }

    ret = snprintf_s(ac_name, sizeof(ac_name), sizeof(ac_name) - 1, "isp[%d].drc", vi_pipe);
    if (ret < 0) {
        return TD_FAILURE;
    }

    malloc_param.buf_name = ac_name;
    malloc_param.size = ISP_DRC_BUF_SIZE;
    malloc_param.kernel_only = TD_TRUE;
    ret = cmpi_mmz_malloc_nocache(&malloc_param, &phy_addr, (td_void **)&vir_addr);
    if (ret != TD_SUCCESS) {
        isp_err_trace("ISP[%d] MmzMalloc drc buffer Failure!\n", vi_pipe);
        return OT_ERR_ISP_NOMEM;
    }

    (td_void)memset_s(vir_addr, malloc_param.size, 0, malloc_param.size);

    isp_spin_lock = isp_drv_get_lock(vi_pipe);
    osal_spin_lock_irqsave(isp_spin_lock, &flags);
    drv_ctx->drc_buf_attr.init = TD_TRUE;
    drv_ctx->drc_buf_attr.drc_buf.size = malloc_param.size;
    drv_ctx->drc_buf_attr.drc_buf.phy_addr = phy_addr;
    drv_ctx->drc_buf_attr.drc_buf.vir_addr = (td_void *)vir_addr;
    osal_spin_unlock_irqrestore(isp_spin_lock, &flags);

    if (drv_ctx->work_mode.running_mode == ISP_MODE_RUNNING_ONLINE && post_viproc[0] != TD_NULL) {
        post_viproc[0]->viproc_para_drc_addr_low.u32 = phy_addr;
        post_viproc[0]->viproc_out_para_drc_addr_low.u32 = phy_addr;
    }

    return TD_SUCCESS;
}

static td_s32 isp_drv_drc_buf_exit(ot_vi_pipe vi_pipe)
{
    td_s32 ret;
    td_phys_addr_t phy_addr;
    td_void *vir_addr = TD_NULL;
    isp_drv_ctx *drv_ctx = TD_NULL;
    osal_spinlock *isp_spin_lock = TD_NULL;
    unsigned long flags;
    td_bool is_alloc = TD_FALSE;
    isp_viproc_reg_type *post_viproc[OT_ISP_STRIPING_MAX_NUM] = { TD_NULL };

    isp_check_pipe_return(vi_pipe);

    drv_ctx = isp_drv_get_ctx(vi_pipe);
    is_alloc = isp_drv_get_drc_alloc_flag(drv_ctx->work_mode.running_mode);
    if (is_alloc == TD_FALSE) {
        return TD_SUCCESS;
    }
    isp_check_exit_state_return(vi_pipe, drv_ctx->isp_run_flag);
    isp_check_buf_exit_return(vi_pipe, drv_ctx->drc_buf_attr.init);

    isp_spin_lock = isp_drv_get_lock(vi_pipe);
    osal_spin_lock_irqsave(isp_spin_lock, &flags);
    phy_addr = drv_ctx->drc_buf_attr.drc_buf.phy_addr;
    vir_addr = drv_ctx->drc_buf_attr.drc_buf.vir_addr;

    drv_ctx->drc_buf_attr.init = TD_FALSE;
    drv_ctx->drc_buf_attr.drc_buf.size = 0;
    drv_ctx->drc_buf_attr.drc_buf.phy_addr = 0;
    drv_ctx->drc_buf_attr.drc_buf.vir_addr = TD_NULL;
    if (drv_ctx->work_mode.running_mode == ISP_MODE_RUNNING_ONLINE) {
        ret = isp_drv_get_post_viproc_reg_virt_addr(vi_pipe, post_viproc);
        if (ret == TD_SUCCESS && post_viproc[0] != TD_NULL) {
            post_viproc[0]->viproc_para_drc_addr_low.u32 = 0;
            post_viproc[0]->viproc_out_para_drc_addr_low.u32 = 0;
        } else {
            isp_err_trace("ISP[%d] get viproc reg fail!\n", vi_pipe);
        }
    }
    osal_spin_unlock_irqrestore(isp_spin_lock, &flags);

    if (phy_addr != 0) {
        cmpi_mmz_free(phy_addr, vir_addr);
    }

    return TD_SUCCESS;
}

static td_void isp_drv_ldci_buf_attr_reset(ot_vi_pipe vi_pipe, isp_drv_ctx *drv_ctx)
{
    td_u8 i, be_buf_num;

    drv_ctx->ldci_read_buf_attr.init = TD_FALSE;
    drv_ctx->ldci_write_buf_attr.init = TD_FALSE;
    drv_ctx->ldci_read_buf_attr.buf_num = 0;
    drv_ctx->ldci_write_buf_attr.buf_num = 0;
    be_buf_num = isp_drv_get_be_buf_num(vi_pipe);
    for (i = 0; i < be_buf_num; i++) {
        drv_ctx->ldci_write_buf_attr.ldci_buf[i].phy_addr = 0;
        drv_ctx->ldci_write_buf_attr.ldci_buf[i].vir_addr = TD_NULL;
        drv_ctx->ldci_write_buf_attr.ldci_buf[i].size = 0;
    }

    for (i = 0; i < be_buf_num; i++) {
        drv_ctx->ldci_read_buf_attr.ldci_buf[i].phy_addr = 0;
        drv_ctx->ldci_read_buf_attr.ldci_buf[i].vir_addr = TD_NULL;
        drv_ctx->ldci_read_buf_attr.ldci_buf[i].size = 0;
    }
}

static td_void isp_drv_ldci_tpr_buf_attr_init(isp_drv_ctx *drv_ctx, td_u8 wr_rd_buf_num, td_u8 tmp_buf_num,
    td_phys_addr_t phy_addr, td_u8 *vir_addr)
{
    td_u8 i, j, write_buf_num, read_buf_num;
    td_u64 size = sizeof(isp_ldci_stat);

    write_buf_num = wr_rd_buf_num / 2; // 2 one half
    read_buf_num = wr_rd_buf_num / 2; // 2 one half

    drv_ctx->ldci_write_buf_attr.init    = TD_TRUE;
    drv_ctx->ldci_write_buf_attr.buf_num = write_buf_num;
    drv_ctx->ldci_write_buf_attr.buf_idx = 0;

    drv_ctx->ldci_read_buf_attr.init = TD_TRUE;
    drv_ctx->ldci_read_buf_attr.buf_num = read_buf_num;
    drv_ctx->ldci_read_buf_attr.buf_idx = 0;

    for (i = 0; i < write_buf_num; i++) {
        drv_ctx->ldci_write_buf_attr.ldci_buf[i].phy_addr = phy_addr + i * size;
        drv_ctx->ldci_write_buf_attr.ldci_buf[i].vir_addr = (td_void *)(vir_addr + i * size);
        drv_ctx->ldci_write_buf_attr.ldci_buf[i].size = size;
    }

    for (i = 0; i < read_buf_num; i++) {
        j = i + write_buf_num;
        drv_ctx->ldci_read_buf_attr.ldci_buf[i].phy_addr = phy_addr + j * size;
        drv_ctx->ldci_read_buf_attr.ldci_buf[i].vir_addr = (td_void *)(vir_addr + j * size);
        drv_ctx->ldci_read_buf_attr.ldci_buf[i].size = size;
    }

    if (tmp_buf_num == 1) {
        drv_ctx->ldci_tmp_buf_attr.init = TD_TRUE;
        drv_ctx->ldci_tmp_buf_attr.buf_num = 1;
        drv_ctx->ldci_tmp_buf_attr.buf_idx = 0;

        j = read_buf_num + write_buf_num;
        drv_ctx->ldci_tmp_buf_attr.ldci_buf[0].phy_addr = phy_addr + j * size;
        drv_ctx->ldci_tmp_buf_attr.ldci_buf[0].vir_addr = (td_void *)(vir_addr + j * size);
        drv_ctx->ldci_tmp_buf_attr.ldci_buf[0].size = size;
    }
}

static td_s32 isp_drv_ldci_tpr_buf_malloc(ot_vi_pipe vi_pipe, td_phys_addr_t *ldci_phy_addr, td_u8 **ldci_vir_addr)
{
    td_u8 wr_rd_buf_num, tmp_buf_num, buf_num_all;
    td_u8 *vir_addr = TD_NULL;
    td_s32 ret;
    td_phys_addr_t phy_addr;
    td_u64 size;
    unsigned long flags = 0;
    isp_drv_ctx *drv_ctx = TD_NULL;
    td_char ac_name[MAX_MMZ_NAME_LEN] = { 0 };
    osal_spinlock *isp_spin_lock = TD_NULL;
    mm_malloc_param malloc_param = {0};

    isp_check_pipe_return(vi_pipe);
    drv_ctx = isp_drv_get_ctx(vi_pipe);

    ret = snprintf_s(ac_name, sizeof(ac_name), sizeof(ac_name) - 1, "isp[%d].ldci", vi_pipe);
    if (ret < 0) {
        return TD_FAILURE;
    }

    size = sizeof(isp_ldci_stat);

    tmp_buf_num = 0;
    if (is_online_mode(drv_ctx->work_mode.running_mode)) {
        wr_rd_buf_num = ISP_ONLINE_LDCI_TPR_BUF_NUM + ISP_ONLINE_LDCI_TPR_BUF_NUM;
    } else {
        wr_rd_buf_num = 2 * isp_drv_get_be_buf_num(vi_pipe); // 2: write and read
        if (isp_drv_is_ldci_stt_all_load() == TD_TRUE && drv_ctx->work_mode.block_num == 2) {
            tmp_buf_num = 1;
        }
    }

    buf_num_all = wr_rd_buf_num + tmp_buf_num;

    malloc_param.buf_name = ac_name;
    malloc_param.size = size * buf_num_all;
    malloc_param.kernel_only = TD_FALSE;
    ret = cmpi_mmz_malloc_nocache(&malloc_param, &phy_addr, (td_void **)&vir_addr);
    if (ret != TD_SUCCESS) {
        isp_err_trace("ISP[%d] MmzMalloc Ldci buffer Failure!\n", vi_pipe);
        return OT_ERR_ISP_NOMEM;
    }

    (td_void)memset_s(vir_addr, malloc_param.size, 0, malloc_param.size);
    isp_spin_lock = isp_drv_get_lock(vi_pipe);
    osal_spin_lock_irqsave(isp_spin_lock, &flags);
    isp_drv_ldci_tpr_buf_attr_init(drv_ctx, wr_rd_buf_num, tmp_buf_num, phy_addr, vir_addr);
    osal_spin_unlock_irqrestore(isp_spin_lock, &flags);

    *ldci_phy_addr = phy_addr;
    *ldci_vir_addr = vir_addr;

    return TD_SUCCESS;
}

static td_s32 isp_drv_ldci_tpr_buf_init(ot_vi_pipe vi_pipe)
{
    td_u8 *vir_addr = TD_NULL;
    td_s32 ret;
    td_phys_addr_t phy_addr;
    isp_drv_ctx *drv_ctx = TD_NULL;

    isp_check_pipe_return(vi_pipe);

    drv_ctx = isp_drv_get_ctx(vi_pipe);
    isp_check_buf_init_return(vi_pipe, drv_ctx->ldci_read_buf_attr.init);
    isp_check_buf_init_return(vi_pipe, drv_ctx->ldci_write_buf_attr.init);

    ret = isp_drv_ldci_tpr_buf_malloc(vi_pipe, &phy_addr, &vir_addr);
    isp_check_return(vi_pipe, ret, "isp_drv_ldci_tpr_buf_malloc");

    if (is_online_mode(drv_ctx->work_mode.running_mode)) {
        ret = isp_drv_set_ldci_stt_addr(vi_pipe, drv_ctx->ldci_read_buf_attr.ldci_buf[0].phy_addr,
            drv_ctx->ldci_write_buf_attr.ldci_buf[0].phy_addr);
        if (ret != TD_SUCCESS) {
            isp_err_trace("ISP[%d] Set Ldci Param/OutParam addr Err!\n", vi_pipe);
            goto fail0;
        }

        /* update Write Index */
        drv_ctx->ldci_write_buf_attr.buf_idx =
            (drv_ctx->ldci_write_buf_attr.buf_idx + 1) % div_0_to_1(drv_ctx->ldci_write_buf_attr.buf_num);
    }

    return TD_SUCCESS;

fail0:
    isp_drv_ldci_buf_attr_reset(vi_pipe, drv_ctx);
    if (phy_addr != 0) {
        cmpi_mmz_free(phy_addr, (td_void *)vir_addr);
    }
    return TD_FAILURE;
}

static td_s32 isp_drv_ldci_tpr_buf_exit(ot_vi_pipe vi_pipe)
{
    td_phys_addr_t phy_addr;
    td_void *vir_addr = TD_NULL;
    unsigned long flags = 0;
    isp_drv_ctx *drv_ctx = TD_NULL;
    osal_spinlock *isp_spin_lock = TD_NULL;
    isp_check_pipe_return(vi_pipe);

    drv_ctx = isp_drv_get_ctx(vi_pipe);

    isp_check_buf_exit_return(vi_pipe, drv_ctx->ldci_read_buf_attr.init);
    isp_check_buf_exit_return(vi_pipe, drv_ctx->ldci_write_buf_attr.init);

    isp_spin_lock = isp_drv_get_lock(vi_pipe);
    osal_spin_lock_irqsave(isp_spin_lock, &flags);
    phy_addr = drv_ctx->ldci_write_buf_attr.ldci_buf[0].phy_addr;
    vir_addr = drv_ctx->ldci_write_buf_attr.ldci_buf[0].vir_addr;
    isp_drv_ldci_buf_attr_reset(vi_pipe, drv_ctx);
    osal_spin_unlock_irqrestore(isp_spin_lock, &flags);

    if (phy_addr != 0) {
        cmpi_mmz_free(phy_addr, vir_addr);
    }

    return TD_SUCCESS;
}

static td_void isp_drv_ldci_normal_buf_attr_init(isp_drv_ctx *drv_ctx, td_u8 buf_num,
    td_phys_addr_t phy_addr, td_u64 size)
{
    td_u8 i;
    drv_ctx->ldci_read_buf_attr.init = TD_TRUE;
    drv_ctx->ldci_read_buf_attr.buf_num = buf_num;
    drv_ctx->ldci_read_buf_attr.buf_idx = 0;

    drv_ctx->ldci_write_buf_attr.init = TD_TRUE;
    drv_ctx->ldci_write_buf_attr.buf_num = buf_num;
    drv_ctx->ldci_write_buf_attr.buf_idx = MIN2(buf_num - 1, drv_ctx->ldci_read_buf_attr.buf_idx + 1);

    for (i = 0; i < buf_num; i++) {
        drv_ctx->ldci_read_buf_attr.ldci_buf[i].phy_addr = phy_addr + i * size;
        drv_ctx->ldci_write_buf_attr.ldci_buf[i].phy_addr = phy_addr + i * size;
    }
}

static td_s32 isp_drv_ldci_normal_buf_init(ot_vi_pipe vi_pipe)
{
    td_u8 buf_num = 1;
    td_s32 ret;
    td_phys_addr_t phy_addr;
    td_u64 size;
    unsigned long flags = 0;
    isp_drv_ctx *drv_ctx = TD_NULL;
    td_char ac_name[MAX_MMZ_NAME_LEN] = { 0 };
    osal_spinlock *isp_spin_lock = TD_NULL;
    mm_malloc_param malloc_param = {0};
    isp_check_pipe_return(vi_pipe);

    drv_ctx = isp_drv_get_ctx(vi_pipe);

    isp_check_buf_init_return(vi_pipe, drv_ctx->ldci_read_buf_attr.init);
    isp_check_buf_init_return(vi_pipe, drv_ctx->ldci_write_buf_attr.init);

    ret = snprintf_s(ac_name, sizeof(ac_name), sizeof(ac_name) - 1, "isp[%d].ldci", vi_pipe);
    if (ret < 0) {
        return TD_FAILURE;
    }
    size = sizeof(isp_ldci_stat);

    if (is_striping_mode(drv_ctx->work_mode.running_mode) && isp_drv_is_ldci_stt_all_load() == TD_FALSE) {
        buf_num = ISP_STRIPING_LDCI_NORMAL_BUF_NUM;
    }

    malloc_param.buf_name = ac_name;
    malloc_param.size = size * buf_num;
    malloc_param.kernel_only = TD_FALSE;
    phy_addr = cmpi_mmz_malloc(&malloc_param);
    if (phy_addr == 0) {
        isp_err_trace("alloc ISP[%d] Ldci buf err\n", vi_pipe);
        return OT_ERR_ISP_NOMEM;
    }
    isp_spin_lock = isp_drv_get_lock(vi_pipe);
    osal_spin_lock_irqsave(isp_spin_lock, &flags);
    isp_drv_ldci_normal_buf_attr_init(drv_ctx, buf_num, phy_addr, size);
    osal_spin_unlock_irqrestore(isp_spin_lock, &flags);

    return TD_SUCCESS;
}

static td_s32 isp_drv_ldci_normal_buf_exit(ot_vi_pipe vi_pipe)
{
    td_phys_addr_t phy_addr;
    isp_drv_ctx *drv_ctx = TD_NULL;
    osal_spinlock *isp_spin_lock = TD_NULL;
    unsigned long flags = 0;

    isp_check_pipe_return(vi_pipe);
    drv_ctx = isp_drv_get_ctx(vi_pipe);
    isp_check_buf_exit_return(vi_pipe, drv_ctx->ldci_read_buf_attr.init);
    isp_check_buf_exit_return(vi_pipe, drv_ctx->ldci_write_buf_attr.init);
    isp_spin_lock = isp_drv_get_lock(vi_pipe);
    osal_spin_lock_irqsave(isp_spin_lock, &flags);
    phy_addr = drv_ctx->ldci_write_buf_attr.ldci_buf[0].phy_addr;
    isp_drv_ldci_buf_attr_reset(vi_pipe, drv_ctx);
    osal_spin_unlock_irqrestore(isp_spin_lock, &flags);

    if (phy_addr != 0) {
        cmpi_mmz_free(phy_addr, TD_NULL);
    }

    return TD_SUCCESS;
}

static td_s32 isp_drv_ldci_buf_init(ot_vi_pipe vi_pipe)
{
    isp_check_pipe_return(vi_pipe);

    if (isp_drv_get_ldci_tpr_flt_en(vi_pipe) == TD_TRUE) {
        return isp_drv_ldci_tpr_buf_init(vi_pipe);
    } else {
        return isp_drv_ldci_normal_buf_init(vi_pipe);
    }
}

static td_s32 isp_drv_ldci_buf_exit(ot_vi_pipe vi_pipe)
{
    isp_drv_ctx *drv_ctx = TD_NULL;

    isp_check_pipe_return(vi_pipe);

    drv_ctx = isp_drv_get_ctx(vi_pipe);
    isp_check_exit_state_return(vi_pipe, drv_ctx->isp_run_flag);

    if (isp_drv_get_ldci_tpr_flt_en(vi_pipe) == TD_TRUE) {
        return isp_drv_ldci_tpr_buf_exit(vi_pipe);
    } else {
        return isp_drv_ldci_normal_buf_exit(vi_pipe);
    }

    return TD_FAILURE;
}

td_s32 isp_drv_ldci_read_stt_buf_get(ot_vi_pipe vi_pipe, isp_ldci_read_stt_buf *ldci_read_stt_buf)
{
    unsigned long flags = 0;
    isp_drv_ctx *drv_ctx = TD_NULL;
    td_u64 size;
    osal_spinlock *isp_spin_lock = TD_NULL;

    isp_check_pipe_return(vi_pipe);
    isp_check_pointer_return(ldci_read_stt_buf);

    drv_ctx = isp_drv_get_ctx(vi_pipe);
    isp_spin_lock = isp_drv_get_lock(vi_pipe);
    osal_spin_lock_irqsave(isp_spin_lock, &flags);
    ldci_read_stt_buf->buf_num = drv_ctx->ldci_read_buf_attr.buf_num;
    ldci_read_stt_buf->buf_idx = 0;
    ldci_read_stt_buf->head_phy_addr = drv_ctx->ldci_read_buf_attr.ldci_buf[0].phy_addr;
    size = drv_ctx->ldci_read_buf_attr.ldci_buf[0].size;
    osal_spin_unlock_irqrestore(isp_spin_lock, &flags);

    if (ot_mmz_check_phys_addr(ldci_read_stt_buf->head_phy_addr, size)) {
        (td_void)memset_s(ldci_read_stt_buf, sizeof(isp_ldci_read_stt_buf), 0, sizeof(isp_ldci_read_stt_buf));
        isp_err_trace("check ldci buff error\n");
        return TD_FAILURE;
    }

    return TD_SUCCESS;
}

#ifdef CONFIG_OT_ISP_CLUT_SUPPORT
td_s32 isp_drv_clut_buf_init(ot_vi_pipe vi_pipe)
{
    td_s32 ret;
    td_phys_addr_t phy_addr;
    td_u8 *vir_addr = TD_NULL;
    isp_drv_ctx *drv_ctx = TD_NULL;
    td_char ac_name[MAX_MMZ_NAME_LEN] = { 0 };
    mm_malloc_param malloc_param = {0};

    isp_check_pipe_return(vi_pipe);
    drv_ctx = isp_drv_get_ctx(vi_pipe);

    isp_check_buf_init_return(vi_pipe, drv_ctx->clut_buf_attr.init);

    ret = snprintf_s(ac_name, sizeof(ac_name), sizeof(ac_name) - 1, "isp[%d].clut", vi_pipe);
    if (ret < 0) {
        return TD_FAILURE;
    }

    malloc_param.buf_name = ac_name;
    malloc_param.size = OT_ISP_CLUT_LUT_LENGTH * sizeof(td_u32);
    malloc_param.kernel_only = TD_FALSE;
    ret = cmpi_mmz_malloc_nocache(&malloc_param, &phy_addr, (td_void **)&vir_addr);
    if (ret != TD_SUCCESS) {
        isp_err_trace("ISP[%d] MmzMalloc Clut buffer Failure!\n", vi_pipe);
        return OT_ERR_ISP_NOMEM;
    }

    (td_void)memset_s(vir_addr, malloc_param.size, 0, malloc_param.size);

    drv_ctx->clut_buf_attr.init = TD_TRUE;
    drv_ctx->clut_buf_attr.clut_buf.size = malloc_param.size;
    drv_ctx->clut_buf_attr.clut_buf.phy_addr = phy_addr;
    drv_ctx->clut_buf_attr.clut_buf.vir_addr = (td_void *)vir_addr;

    return TD_SUCCESS;
}

td_s32 isp_drv_clut_buf_exit(ot_vi_pipe vi_pipe)
{
    td_phys_addr_t phy_addr;
    td_void *vir_addr = TD_NULL;
    isp_drv_ctx *drv_ctx = TD_NULL;
    unsigned long flags;
    osal_spinlock *isp_spin_lock = TD_NULL;

    isp_check_pipe_return(vi_pipe);

    drv_ctx = isp_drv_get_ctx(vi_pipe);
    isp_check_exit_state_return(vi_pipe, drv_ctx->isp_run_flag);

    isp_check_buf_exit_return(vi_pipe, drv_ctx->clut_buf_attr.init);

    isp_spin_lock = isp_drv_get_lock(vi_pipe);
    osal_spin_lock_irqsave(isp_spin_lock, &flags);
    phy_addr = drv_ctx->clut_buf_attr.clut_buf.phy_addr;
    vir_addr = drv_ctx->clut_buf_attr.clut_buf.vir_addr;

    drv_ctx->clut_buf_attr.init = TD_FALSE;
    drv_ctx->clut_buf_attr.clut_buf.size = 0;
    drv_ctx->clut_buf_attr.clut_buf.phy_addr = 0;
    drv_ctx->clut_buf_attr.clut_buf.vir_addr = TD_NULL;
    osal_spin_unlock_irqrestore(isp_spin_lock, &flags);

    if (phy_addr != 0) {
        cmpi_mmz_free(phy_addr, vir_addr);
    }

    return TD_SUCCESS;
}
#endif
#ifdef CONFIG_OT_ISP_LBLC_SUPPORT
static td_s32 isp_drv_fe_lut_stt_buf_get(unsigned int cmd, void *arg, void *private_data)
{
    ot_vi_pipe vi_pipe = isp_get_pipe(private_data);
    isp_drv_ctx *drv_ctx = TD_NULL;
    ot_unused(cmd);
    // need to check pipe is valid or not first, then to check it is no fe pipe or not
    isp_check_pipe_return(vi_pipe);
    isp_check_no_fe_pipe_return(vi_pipe);
    isp_check_pointer_return(arg);

    drv_ctx = isp_drv_get_ctx(vi_pipe);
    if (drv_ctx->fe_lut2stt_attr.init != TD_TRUE) {
        return OT_ERR_ISP_NOMEM;
    }

    if (ot_mmz_check_phys_addr(drv_ctx->fe_lut2stt_attr.lut_stt_buf[0].phy_addr,
        drv_ctx->fe_lut2stt_attr.lut_stt_buf[0].size) != TD_SUCCESS) {
        isp_err_trace("check fe lut error \n");
        return OT_ERR_ISP_NOT_SUPPORT;
    }

    *(td_phys_addr_t *)arg = drv_ctx->fe_lut2stt_attr.lut_stt_buf[0].phy_addr;
    return TD_SUCCESS;
}
#endif

static td_s32 isp_drv_set_reg_kernel_cfgs(ot_vi_pipe vi_pipe, isp_kernel_reg_cfg *reg_kernel_cfg)
{
    td_u32 flag;
    unsigned long flags = 0;
    isp_drv_ctx *drv_ctx = TD_NULL;
    osal_spinlock *isp_spin_lock = TD_NULL;

    isp_check_pipe_return(vi_pipe);
    isp_check_pointer_return(reg_kernel_cfg);

    drv_ctx = isp_drv_get_ctx(vi_pipe);
    if ((drv_ctx->reg_cfg_info_flag != 0) && (drv_ctx->reg_cfg_info_flag != 1)) {
        isp_err_trace("Pipe[%d] Err reg_cfg_info_flag != 0/1 !!!\n", vi_pipe);
        return TD_FAILURE;
    }

    flag = 1 - drv_ctx->reg_cfg_info_flag;
    (td_void)memcpy_s(&drv_ctx->kernel_cfg[flag], sizeof(isp_kernel_reg_cfg), reg_kernel_cfg,
        sizeof(isp_kernel_reg_cfg));
    isp_spin_lock = isp_drv_get_lock(vi_pipe);
    osal_spin_lock_irqsave(isp_spin_lock, &flags);
    drv_ctx->reg_cfg_info_flag = flag;
    osal_spin_unlock_irqrestore(isp_spin_lock, &flags);

    return TD_SUCCESS;
}

static td_s32 isp_drv_chn_select_write(ot_vi_pipe vi_pipe, td_u32 channel_sel)
{
    td_u8 i;
    td_u32 chn_switch[ISP_CHN_SWITCH_NUM] = { 0 };
    isp_chn_sel chn_sel;
    isp_drv_ctx *drv_ctx = TD_NULL;

    isp_check_pipe_return(vi_pipe);
    drv_ctx = isp_drv_get_ctx(vi_pipe);

    chn_switch[4] = drv_ctx->yuv_mode ? 1 : 0; /* register[4] value 1 means yuv mode */

    for (i = 0; i < ISP_CHN_SWITCH_NUM - 1; i++) {
        chn_switch[i] = drv_ctx->chn_sel_attr[0].wdr_chn_sel[i];
    }

    chn_sel = (isp_chn_sel)(channel_sel & 0x3);
    switch (chn_sel) {
        case ISP_CHN_SWITCH_NORMAL:
            break;

        case ISP_CHN_SWITCH_2LANE:
            chn_switch[1] = drv_ctx->chn_sel_attr[0].wdr_chn_sel[0];
            chn_switch[0] = drv_ctx->chn_sel_attr[0].wdr_chn_sel[1];
            break;

        case ISP_CHN_SWITCH_3LANE:
            chn_switch[2] = drv_ctx->chn_sel_attr[0].wdr_chn_sel[0]; /* array index 2 */
            chn_switch[1] = drv_ctx->chn_sel_attr[0].wdr_chn_sel[1];
            chn_switch[0] = drv_ctx->chn_sel_attr[0].wdr_chn_sel[2]; /* array index 2 */
            break;

        case ISP_CHN_SWITCH_4LANE:
            chn_switch[3] = drv_ctx->chn_sel_attr[0].wdr_chn_sel[0]; /* array index 3 */
            chn_switch[2] = drv_ctx->chn_sel_attr[0].wdr_chn_sel[1]; /* array index 2 */
            chn_switch[1] = drv_ctx->chn_sel_attr[0].wdr_chn_sel[2]; /* array index 2 */
            chn_switch[0] = drv_ctx->chn_sel_attr[0].wdr_chn_sel[3]; /* array index 3 */
            break;

        default:
            break;
    }

    if (is_fs_wdr_mode(drv_ctx->sync_cfg.wdr_mode)) {
        isp_drv_set_input_sel(vi_pipe, &chn_switch[0], sizeof(chn_switch));
    }

    return TD_SUCCESS;
}

static td_s32 isp_drv_chn_select_cfg(ot_vi_pipe vi_pipe, td_u32 chn_sel)
{
    td_u32 i;
    td_s32 ret = TD_SUCCESS;
    isp_drv_ctx *drv_ctx = TD_NULL;
    unsigned long flags = 0;
    osal_spinlock *isp_spin_lock = TD_NULL;

    isp_check_pipe_return(vi_pipe);

    drv_ctx = isp_drv_get_ctx(vi_pipe);
    isp_spin_lock = isp_drv_get_lock(vi_pipe);
    osal_spin_lock_irqsave(isp_spin_lock, &flags);

    for (i = 0; i < OT_ISP_STRIPING_MAX_NUM; i++) {
        drv_ctx->chn_sel_attr[i].channel_sel = chn_sel;
    }

    if (is_online_mode(drv_ctx->work_mode.running_mode)) {
        ret = isp_drv_chn_select_write(vi_pipe, chn_sel);
        if (ret != TD_SUCCESS) {
            osal_spin_unlock_irqrestore(isp_spin_lock, &flags);
            isp_err_trace("isp[%d] ChnSelect Write err!\n", vi_pipe);
            return ret;
        }
    }

    osal_spin_unlock_irqrestore(isp_spin_lock, &flags);

    return ret;
}

#ifdef CONFIG_OT_ISP_MCF_SUPPORT
static td_s32 isp_drv_get_nr_mode(ot_vi_pipe vi_pipe, td_bool *nr_en)
{
    if (!ckfn_vi_is_ia_nr_en()) {
        return TD_FAILURE;
    }

    *nr_en = call_vi_is_ia_nr_en(vi_pipe);

    return TD_SUCCESS;
}
#endif

static td_s32 isp_drv_bnr_temporal_filt_set(ot_vi_pipe vi_pipe, isp_bnr_temporal_filt *tpr_filt)
{
    td_s32 ret;
    unsigned long flags = 0;
    isp_drv_ctx *drv_ctx = TD_NULL;
    osal_spinlock *isp_spin_lock = TD_NULL;
    vi_isp_bnr_cfg bnr_cfg = {0};
    isp_check_pipe_return(vi_pipe);
    isp_check_pointer_return(tpr_filt);
    isp_check_bool_return(tpr_filt->nr_en);
    isp_check_bool_return(tpr_filt->tnr_en);
    drv_ctx = isp_drv_get_ctx(vi_pipe);
    isp_spin_lock = isp_drv_get_lock(vi_pipe);
    osal_spin_lock_irqsave(isp_spin_lock, &flags);
    (td_void)memcpy_s(&drv_ctx->bnr_tpr_filt.cur, sizeof(isp_bnr_temporal_filt),
        tpr_filt, sizeof(isp_bnr_temporal_filt));

    osal_spin_unlock_irqrestore(isp_spin_lock, &flags);

    if (ckfn_vi_set_pipe_bnr_cfg() != TD_NULL) {
        bnr_cfg.bnr_en = tpr_filt->nr_en && tpr_filt->tnr_en;
        bnr_cfg.bnr_ctrl_en = tpr_filt->nr_en;
#ifdef CONFIG_OT_VI_PIPE_BNR_PURE_MODE
        bnr_cfg.bnr_ref_mode = tpr_filt->ref_mode;
        bnr_cfg.bnr_load_ref_en = tpr_filt->load_ref_en;
#endif
        ret = call_vi_set_pipe_bnr_cfg(vi_pipe, &bnr_cfg);
        if (ret != TD_SUCCESS) {
            isp_err_trace("ISP[%d] call_vi_set_pipe_bnr_en Failed!\n", vi_pipe);
            return ret;
        }
    } else {
        isp_err_trace("ISP[%d] vi_set_pipe_bnr_en is TD_NULL\n", vi_pipe);
        return TD_FAILURE;
    }

    return TD_SUCCESS;
}

#ifdef IQ_DEBUG
static td_s32 isp_drv_bnr_mot_bitw_set(ot_vi_pipe vi_pipe, td_u8 *bitw)
{
    td_s32 ret;
    isp_check_pipe_return(vi_pipe);
    if (ckfn_vi_set_pipe_bnr_mot_bitw() != TD_NULL) {
        ret = call_vi_set_pipe_bnr_mot_bitw(vi_pipe, *bitw);
        if (ret != TD_SUCCESS) {
            isp_err_trace("ISP[%d] call_vi_set_pipe_bnr_mot_bitw Failed!\n", vi_pipe);
            return ret;
        }
    } else {
        isp_err_trace("ISP[%d] ckfn_vi_set_pipe_bnr_mot_bitw is TD_NULL\n", vi_pipe);
        return TD_FAILURE;
    }
    return TD_SUCCESS;
}
#endif

static td_s32 isp_drv_sharpen_mot_set(ot_vi_pipe vi_pipe, td_bool *mot_en)
{
    unsigned long flags = 0;
    isp_drv_ctx *drv_ctx = TD_NULL;
    osal_spinlock *isp_spin_lock = TD_NULL;
    isp_check_pipe_return(vi_pipe);
    isp_check_pointer_return(mot_en);

    drv_ctx = isp_drv_get_ctx(vi_pipe);
    isp_spin_lock = isp_drv_get_lock(vi_pipe);
    osal_spin_lock_irqsave(isp_spin_lock, &flags);
    drv_ctx->bnr_tpr_filt.sharpen_mot_en = *mot_en;
    osal_spin_unlock_irqrestore(isp_spin_lock, &flags);

    return TD_SUCCESS;
}

static td_s32 isp_drv_sync_cfg_set(ot_vi_pipe vi_pipe, isp_sync_cfg_buf_node *sync_cfg_buf_node)
{
    isp_drv_ctx *drv_ctx = TD_NULL;
    isp_sync_cfg_buf *sync_cfg_buf = TD_NULL;
    isp_sync_cfg_buf_node *cur_node = TD_NULL;
    unsigned long flags = 0;
    osal_spinlock *isp_spin_lock = TD_NULL;

    isp_check_pipe_return(vi_pipe);
    isp_check_pointer_return(sync_cfg_buf_node);

    drv_ctx = isp_drv_get_ctx(vi_pipe);
    isp_spin_lock = isp_drv_get_lock(vi_pipe);
    osal_spin_lock_irqsave(isp_spin_lock, &flags);
    sync_cfg_buf = &drv_ctx->sync_cfg.sync_cfg_buf;

    if (isp_sync_buf_is_full(sync_cfg_buf)) {
        isp_err_trace("Pipe[%d] isp sync buffer is full\n", vi_pipe);
        osal_spin_unlock_irqrestore(isp_spin_lock, &flags);
        return TD_FAILURE;
    }

    if ((sync_cfg_buf_node->sns_regs_info.sns_type >= OT_ISP_SNS_TYPE_BUTT) ||
        (sync_cfg_buf_node->ae_reg_cfg.fs_wdr_mode >= OT_ISP_FSWDR_MODE_BUTT) ||
        (sync_cfg_buf_node->sns_regs_info.cfg2_valid_delay_max > CFG2VLD_DLY_LIMIT) ||
        (sync_cfg_buf_node->sns_regs_info.cfg2_valid_delay_max < 1) ||
        (sync_cfg_buf_node->sns_regs_info.slv_sync.delay_frame_num > CFG2VLD_DLY_LIMIT) ||
        (sync_cfg_buf_node->sns_regs_info.reg_num > OT_ISP_MAX_SNS_REGS)) {
        osal_spin_unlock_irqrestore(isp_spin_lock, &flags);
        isp_err_trace("Pipe[%d] Invalid sns_regs_info!(sns_type:%d, wdr:%d, delay_max:%u, slv_delay:%u, reg_num:%u)\n",
            vi_pipe, sync_cfg_buf_node->sns_regs_info.sns_type,
            sync_cfg_buf_node->ae_reg_cfg.fs_wdr_mode,
            sync_cfg_buf_node->sns_regs_info.cfg2_valid_delay_max,
            sync_cfg_buf_node->sns_regs_info.slv_sync.delay_frame_num,
            sync_cfg_buf_node->sns_regs_info.reg_num);
        return TD_FAILURE;
    }

    cur_node = &sync_cfg_buf->sync_cfg_buf_node[sync_cfg_buf->buf_wr_flag];
    osal_spin_unlock_irqrestore(isp_spin_lock, &flags);

    (td_void)memcpy_s(cur_node, sizeof(isp_sync_cfg_buf_node), sync_cfg_buf_node, sizeof(isp_sync_cfg_buf_node));

    osal_spin_lock_irqsave(isp_spin_lock, &flags);
    sync_cfg_buf->buf_wr_flag = (sync_cfg_buf->buf_wr_flag + 1) % ISP_SYNC_BUF_NODE_NUM;
    osal_spin_unlock_irqrestore(isp_spin_lock, &flags);

    return TD_SUCCESS;
}

#ifdef CONFIG_OT_SYS_KOL_SUPPORT
static td_s32 isp_drv_standby_cfg_init(ot_vi_pipe vi_pipe, const ot_isp_sns_regs_info* standby_cfg)
{
    isp_drv_ctx *drv_ctx = TD_NULL;
    isp_check_pipe_return(vi_pipe);
    isp_check_pointer_return(standby_cfg);

    drv_ctx = isp_drv_get_ctx(vi_pipe);
    if (standby_cfg->reg_num > OT_ISP_MAX_SNS_REGS || standby_cfg->sns_type >= OT_ISP_SNS_TYPE_BUTT) {
        isp_err_trace("pipe %d invalid standby cfg! reg num %u sns_type %u \n", vi_pipe,
            standby_cfg->reg_num, standby_cfg->sns_type);
        return OT_ERR_ISP_ILLEGAL_PARAM;
    }
    (td_void)memcpy_s(&drv_ctx->standby_cfg, sizeof(ot_isp_sns_regs_info), standby_cfg, sizeof(ot_isp_sns_regs_info));

    return TD_SUCCESS;
}
#endif


#ifdef CONFIG_OT_VI_STITCH_GRP
static td_void isp_drv_be_stitch_sync_param_init(ot_vi_pipe vi_pipe, const isp_be_sync_para *be_sync_param,
    isp_drv_ctx *drv_ctx)
{
    td_u8 i;
    ot_vi_pipe stitch_pipe;
    isp_drv_ctx *drv_ctx_stitch_pipe = TD_NULL;
    if (drv_ctx->stitch_attr.stitch_enable != TD_TRUE) {
        return;
    }
    if (drv_ctx->stitch_attr.main_pipe != TD_TRUE) {
        return;
    }
    for (i = 0; i < drv_ctx->stitch_attr.stitch_pipe_num; i++) {
        (td_void)memcpy_s(&drv_ctx->be_sync_para_stitch[i], sizeof(isp_be_sync_para), be_sync_param,
            sizeof(isp_be_sync_para));
        (td_void)memcpy_s(&drv_ctx->be_pre_sync_para_stitch[i], sizeof(isp_be_sync_para), be_sync_param,
            sizeof(isp_be_sync_para));
    }

    for (i = 1; i < drv_ctx->stitch_attr.stitch_pipe_num; i++) {
        stitch_pipe = drv_ctx->stitch_attr.stitch_bind_id[i];
        drv_ctx_stitch_pipe = isp_drv_get_ctx(stitch_pipe);
        (td_void)memcpy_s(&drv_ctx_stitch_pipe->be_sync_para_stitch[0], sizeof(isp_be_sync_para) * OT_ISP_MAX_PIPE_NUM,
            &drv_ctx->be_sync_para_stitch[0], sizeof(isp_be_sync_para) * OT_ISP_MAX_PIPE_NUM);
        (td_void)memcpy_s(&drv_ctx_stitch_pipe->be_pre_sync_para_stitch[0],
            sizeof(isp_be_sync_para) * OT_ISP_MAX_PIPE_NUM, &drv_ctx->be_pre_sync_para_stitch[0],
            sizeof(isp_be_sync_para) * OT_ISP_MAX_PIPE_NUM);
    }
}
#endif
static td_s32 isp_drv_be_sync_param_init(ot_vi_pipe vi_pipe, const isp_be_sync_para *be_sync_param)
{
    isp_drv_ctx *drv_ctx = TD_NULL;

    isp_check_pipe_return(vi_pipe);
    isp_check_pointer_return(be_sync_param);

    drv_ctx = isp_drv_get_ctx(vi_pipe);
    (td_void)memcpy_s(&drv_ctx->be_sync_para, sizeof(isp_be_sync_para), be_sync_param, sizeof(isp_be_sync_para));
    (td_void)memcpy_s(&drv_ctx->be_pre_sync_para, sizeof(isp_be_sync_para), be_sync_param, sizeof(isp_be_sync_para));
    drv_ctx->dyna_blc_info.pre_black_level_mode = OT_ISP_BLACK_LEVEL_MODE_BUTT;
#ifdef CONFIG_OT_VI_STITCH_GRP
    isp_drv_be_stitch_sync_param_init(vi_pipe, be_sync_param, drv_ctx);
#endif
    return TD_SUCCESS;
}

static td_void isp_drv_switch_mode_cfg_vc(isp_sync_cfg *sync_cfg)
{
    sync_cfg->vc_num = 0;
    sync_cfg->vc_cfg_num = 0;
    sync_cfg->cfg2_vld_dly_max = 1;

    /* get N (N to 1 WDR) */
    switch (sync_cfg->wdr_mode) {
        case OT_WDR_MODE_2To1_FRAME:
            sync_cfg->vc_num_max = 1;
            break;

        case OT_WDR_MODE_3To1_FRAME:
            sync_cfg->vc_num_max = 2; /* reg config is 2 */
            break;

        case OT_WDR_MODE_4To1_FRAME:
            sync_cfg->vc_num_max = 3; /* reg config is 3 */
            break;
        default:
            sync_cfg->vc_num_max = 0;
            break;
    }
}

static td_void isp_drv_switch_mode_cfg_chn_switch(ot_vi_pipe vi_pipe, isp_drv_ctx *drv_ctx)
{
    td_u8 i;
    td_u32 chn_switch[ISP_CHN_SWITCH_NUM] = { 0 };

    for (i = 0; i < ISP_CHN_SWITCH_NUM - 1; i++) {
        chn_switch[i] = i;
    }

    if (is_half_wdr_mode(drv_ctx->sync_cfg.wdr_mode)) {
        chn_switch[0] = 1 % div_0_to_1(drv_ctx->sync_cfg.vc_num_max + 1);
        chn_switch[1] = (chn_switch[0] + 1) % div_0_to_1(drv_ctx->sync_cfg.vc_num_max + 1);
        chn_switch[2] = (chn_switch[0] + 2) % div_0_to_1(drv_ctx->sync_cfg.vc_num_max + 1); /* array index 2 */
        chn_switch[3] = (chn_switch[0] + 3) % div_0_to_1(drv_ctx->sync_cfg.vc_num_max + 1); /* array index 3 */
    }

    chn_switch[4] = drv_ctx->yuv_mode ? 1 : 0; /* array index 4 */

    isp_drv_set_input_sel(vi_pipe, &chn_switch[0], sizeof(chn_switch));
    for (i = 0; i < drv_ctx->work_mode.block_num; i++) {
        (td_void)memcpy_s(drv_ctx->chn_sel_attr[i].wdr_chn_sel, sizeof(chn_switch), chn_switch, sizeof(chn_switch));
    }
}


static td_s32 isp_drv_switch_mode(ot_vi_pipe vi_pipe, isp_drv_ctx *drv_ctx)
{
    td_u8 i, j;
    td_s32 vi_dev;

    isp_sync_cfg *sync_cfg = TD_NULL;

    isp_check_pipe_return(vi_pipe);
    isp_check_pointer_return(drv_ctx);
    vi_dev = drv_ctx->wdr_attr.vi_dev;

    sync_cfg = &drv_ctx->sync_cfg;
    sync_cfg->wdr_mode = drv_ctx->wdr_cfg.wdr_mode;

    for (j = 0; j < OT_ISP_EXP_RATIO_NUM; j++) {
        for (i = 0; i < CFG2VLD_DLY_LIMIT; i++) {
            sync_cfg->exp_ratio[j][i] = drv_ctx->wdr_cfg.exp_ratio[j];
        }
    }

    /* init cfg when modes change */
    (td_void)memset_s(&drv_ctx->sync_cfg.sync_cfg_buf, sizeof(isp_sync_cfg_buf), 0, sizeof(isp_sync_cfg_buf));
    (td_void)memset_s(&drv_ctx->sync_cfg.node, sizeof(drv_ctx->sync_cfg.node), 0, sizeof(drv_ctx->sync_cfg.node));
    (td_void)memset_s(&drv_ctx->isp_int_info, sizeof(isp_interrupt_info), 0, sizeof(isp_interrupt_info));

    sync_cfg->cur_sync_id = -1;

    isp_drv_switch_mode_cfg_vc(sync_cfg);

    /* Channel Switch config */
    isp_drv_switch_mode_cfg_chn_switch(vi_pipe, drv_ctx);

    /* pt_int_mask */
    if ((is_full_wdr_mode(sync_cfg->wdr_mode)) || (is_half_wdr_mode(sync_cfg->wdr_mode))) {
        io_rw_pt_address(vi_pt_base(vi_dev) + VI_PT_INT_MASK) |= VI_PT_INT_FSTART;
    } else {
        io_rw_pt_address(vi_pt_base(vi_dev) + VI_PT_INT_MASK) &= ~(VI_PT_INT_FSTART);
    }

    sync_cfg->pre_wdr_mode = sync_cfg->wdr_mode;

    return TD_SUCCESS;
}


static td_s32 isp_drv_set_wdr_cfg(ot_vi_pipe vi_pipe, isp_wdr_cfg *wdr_cfg)
{
    td_s32 ret;
    isp_drv_ctx *drv_ctx = TD_NULL;
    unsigned long flags = 0;
    osal_spinlock *isp_spin_lock = TD_NULL;

    isp_check_pipe_return(vi_pipe);
    isp_check_pointer_return(wdr_cfg);

    drv_ctx = isp_drv_get_ctx(vi_pipe);

    if (wdr_cfg->wdr_mode >= OT_WDR_MODE_BUTT) {
        isp_err_trace("Pipe[%d] Invalid WDR mode %d!\n", vi_pipe, wdr_cfg->wdr_mode);
        return TD_FAILURE;
    }
    isp_spin_lock = isp_drv_get_lock(vi_pipe);
    osal_spin_lock_irqsave(isp_spin_lock, &flags);
    (td_void)memcpy_s(&drv_ctx->wdr_cfg, sizeof(isp_wdr_cfg), wdr_cfg, sizeof(isp_wdr_cfg));
    ret = isp_drv_switch_mode(vi_pipe, drv_ctx);
    if (ret != TD_SUCCESS) {
        osal_spin_unlock_irqrestore(isp_spin_lock, &flags);
        isp_err_trace("Pipe[%d] isp_drv_switch_mode err 0x%x!\n", vi_pipe, ret);
        return ret;
    }
    osal_spin_unlock_irqrestore(isp_spin_lock, &flags);

    return TD_SUCCESS;
}

static td_s32 isp_drv_alg_init_err_info_print(ot_vi_pipe vi_pipe, ot_isp_alg_mod alg_mod)
{
    isp_check_pipe_return(vi_pipe);
    if (alg_mod >= OT_ISP_ALG_MOD_BUTT) {
        isp_err_trace("Not Support this alg module:%d!!\n", alg_mod);
        return TD_FAILURE;
    }

    isp_err_trace("ISP[%d] %s NOT init. Please check cmos parameters or mem malloc!\n",
                  vi_pipe, g_isp_alg_mod_name[alg_mod]);

    return TD_SUCCESS;
}

td_s32 isp_set_pub_attr_info(ot_vi_pipe vi_pipe, ot_isp_pub_attr *pub_attr)
{
    isp_drv_ctx *drv_ctx = TD_NULL;
    unsigned long flags = 0;
    osal_spinlock *isp_spin_lock = TD_NULL;

    isp_check_pipe_return(vi_pipe);
    isp_check_pointer_return(pub_attr);

    drv_ctx = isp_drv_get_ctx(vi_pipe);
    isp_spin_lock = isp_drv_get_lock(vi_pipe);
    osal_spin_lock_irqsave(isp_spin_lock, &flags);
    (td_void)memcpy_s(&drv_ctx->proc_pub_info, sizeof(ot_isp_pub_attr), pub_attr, sizeof(ot_isp_pub_attr));
    osal_spin_unlock_irqrestore(isp_spin_lock, &flags);

    drv_ctx->pub_attr_ok = TD_TRUE;

    return TD_SUCCESS;
}
#ifdef CONFIG_OT_ISP_DYNAMIC_BLC_SUPPORT
static td_s32 isp_drv_get_dynamic_actual_info(ot_vi_pipe vi_pipe, isp_blc_actual_info *actual_info)
{
    isp_drv_ctx *drv_ctx = TD_NULL;
    isp_check_no_fe_pipe_return(vi_pipe);
    isp_check_pointer_return(actual_info);

    drv_ctx = isp_drv_get_ctx(vi_pipe);
    (td_void)memcpy_s(actual_info, sizeof(isp_blc_actual_info),
                      &drv_ctx->dyna_blc_info.actual_info, sizeof(isp_blc_actual_info));
    return TD_SUCCESS;
}
#endif
static td_s32 isp_drv_set_fpn_work_mode_set(ot_vi_pipe vi_pipe, const td_u8 *work_mode)
{
    isp_drv_ctx *drv_ctx = TD_NULL;
    unsigned long flags = 0;
    osal_spinlock *isp_spin_lock = TD_NULL;

    isp_check_pipe_return(vi_pipe);

    if ((*work_mode != FPN_MODE_CORRECTION) && (*work_mode != FPN_MODE_CALIBRATE) &&
        (*work_mode != FPN_MODE_NONE)) {
        return TD_FAILURE;
    }

    drv_ctx = isp_drv_get_ctx(vi_pipe);
    isp_spin_lock = isp_drv_get_lock(vi_pipe);
    osal_spin_lock_irqsave(isp_spin_lock, &flags);
    drv_ctx->fpn_work_mode = *work_mode;

    osal_spin_unlock_irqrestore(isp_spin_lock, &flags);

    return TD_SUCCESS;
}
#ifdef CONFIG_OT_ISP_DYNAMIC_BLC_SUPPORT
static td_s32 isp_get_bas_crop_attr(ot_vi_pipe vi_pipe, vi_blc_crop_info *bas_crop_attr)
{
    isp_check_pipe_return(vi_pipe);
    isp_check_pointer_return(bas_crop_attr);

    if (!ckfn_vi_get_blc_crop_info()) {
        return TD_FAILURE;
    }

    return call_vi_get_blc_crop_info(vi_pipe, bas_crop_attr);
}
#endif
static td_s32 isp_get_stagger_attr(ot_vi_pipe vi_pipe, isp_stagger_attr *stagger_attr)
{
    td_s32 ret;
    vi_out_mode_info out_mode_info;

    isp_check_pipe_return(vi_pipe);
    isp_check_pointer_return(stagger_attr);
    if (is_no_fe_pipe(vi_pipe)) {
        stagger_attr->stagger_en       = TD_FALSE;
        stagger_attr->merge_frame_num  = 1;
        stagger_attr->crop_info.enable = TD_FALSE;
        return TD_SUCCESS;
    }

    if (!ckfn_vi_get_out_mode()) {
        isp_err_trace("pipe[%d] ckfn_vi_get_out_mode is null\n", vi_pipe);
        return TD_FAILURE;
    }

    ret = call_vi_get_out_mode(vi_pipe, &out_mode_info);
    if (ret != TD_SUCCESS) {
        isp_err_trace("pipe[%d] call_vi_get_out_mode failed 0x%x!\n", vi_pipe, ret);
        return ret;
    }

    if (out_mode_info.out_mode == OT_VI_OUT_MODE_2F1_STAGGER) {
        stagger_attr->stagger_en      = TD_TRUE;
        stagger_attr->merge_frame_num = 2; /* 2F1_STAGGER */
    } else if (out_mode_info.out_mode == OT_VI_OUT_MODE_3F1_STAGGER) {
        stagger_attr->stagger_en      = TD_TRUE;
        stagger_attr->merge_frame_num = 3; /* 3F1_STAGGER */
    } else if (out_mode_info.out_mode == OT_VI_OUT_MODE_4F1_STAGGER) {
        stagger_attr->stagger_en      = TD_TRUE;
        stagger_attr->merge_frame_num = 4; /* 4F1_STAGGER */
    } else {
        stagger_attr->stagger_en      = TD_FALSE;
        stagger_attr->merge_frame_num = 1;
    }
    (td_void)memcpy_s(&stagger_attr->crop_info, sizeof(ot_crop_info),
                      &out_mode_info.satgger_crop_info, sizeof(ot_crop_info));

    return TD_SUCCESS;
}

static td_s32 isp_drv_get_split_attr(ot_vi_pipe vi_pipe, vi_pipe_split_attr *pipe_split_attr)
{
    if (!ckfn_vi_get_split_attr()) {
        return TD_FAILURE;
    }

    return call_vi_get_split_attr(vi_pipe, pipe_split_attr);
}

static td_s32 isp_drv_get_running_mode(const vi_pipe_split_attr *pipe_split_attr, isp_running_mode *running_mode)
{
    isp_check_pointer_return(running_mode);
    isp_check_pointer_return(pipe_split_attr);
    switch (pipe_split_attr->vi_vpss_mode) {
        case OT_VI_ONLINE_VPSS_OFFLINE:
        case OT_VI_ONLINE_VPSS_ONLINE:
            if (pipe_split_attr->aiisp_mode == OT_VI_AIISP_MODE_DEFAULT) {
                *running_mode = ISP_MODE_RUNNING_ONLINE;
            } else {
#ifdef CONFIG_SD3403V100
                *running_mode = ISP_MODE_RUNNING_PRE_ONLINE_POST_OFFLINE;
#else
                *running_mode = ISP_MODE_RUNNING_ONLINE;
#endif
            }
            break;
        case OT_VI_OFFLINE_VPSS_OFFLINE:
        case OT_VI_OFFLINE_VPSS_ONLINE:
            if (pipe_split_attr->split_num == 1) {
                *running_mode = ISP_MODE_RUNNING_OFFLINE;
            } else {
                *running_mode = ISP_MODE_RUNNING_STRIPING;
            }
            break;
        default:
            *running_mode = ISP_MODE_RUNNING_BUTT;
            return TD_FAILURE;
    }
    return TD_SUCCESS;
}

static td_void isp_drv_get_yuv_mode(const vi_pipe_split_attr *pipe_split_attr, td_bool *yuv_mode,
                                    isp_data_input_mode *data_input_mode)
{
    if ((pipe_split_attr->pixel_format == OT_PIXEL_FORMAT_YUV_SEMIPLANAR_420) ||
        (pipe_split_attr->pixel_format == OT_PIXEL_FORMAT_YVU_SEMIPLANAR_420) ||
        (pipe_split_attr->pixel_format == OT_PIXEL_FORMAT_YUV_SEMIPLANAR_422) ||
        (pipe_split_attr->pixel_format == OT_PIXEL_FORMAT_YVU_SEMIPLANAR_422) ||
        (pipe_split_attr->pixel_format == OT_PIXEL_FORMAT_YUV_SEMIPLANAR_444) ||
        (pipe_split_attr->pixel_format == OT_PIXEL_FORMAT_YVU_SEMIPLANAR_444) ||
        (pipe_split_attr->pixel_format == OT_PIXEL_FORMAT_YUV_400)) {
        *yuv_mode = TD_TRUE;
        *data_input_mode = ISP_MODE_BT1120_YUV;
    } else {
        *yuv_mode = TD_FALSE;
        *data_input_mode = ISP_MODE_RAW;
    }

    return;
}

static td_s32 isp_drv_work_mode_update(ot_vi_pipe vi_pipe, isp_working_mode *work_mode,
    vi_pipe_split_attr *pipe_split_attr, isp_data_input_mode data_input_mode, isp_running_mode running_mode)
{
    work_mode->data_input_mode = data_input_mode;

    work_mode->block_num = pipe_split_attr->split_num;
    work_mode->block_dev = (td_u8)isp_drv_get_block_id(vi_pipe, running_mode);
    work_mode->running_mode = running_mode;
    work_mode->aiisp_mode = pipe_split_attr->aiisp_mode;
    work_mode->over_lap = pipe_split_attr->overlap;

    work_mode->frame_rect.width = pipe_split_attr->wch_out_rect.width;
    work_mode->frame_rect.height = pipe_split_attr->wch_out_rect.height;

    (td_void)memcpy_s(&work_mode->max_size, sizeof(ot_size), &pipe_split_attr->max_size, sizeof(ot_size));
    (td_void)memcpy_s(&work_mode->min_size, sizeof(ot_size), &pipe_split_attr->min_size, sizeof(ot_size));
    (td_void)memcpy_s(work_mode->block_rect, sizeof(ot_rect) * OT_ISP_STRIPING_MAX_NUM, pipe_split_attr->rect,
        sizeof(ot_rect) * OT_ISP_STRIPING_MAX_NUM);

    return TD_SUCCESS;
}

static td_void isp_drv_update_blk_attr(isp_drv_ctx *drv_ctx, isp_block_attr *blk_attr,
    vi_pipe_split_attr *pipe_split_attr, isp_running_mode running_mode)
{
    blk_attr->block_num = pipe_split_attr->split_num;
    blk_attr->over_lap = pipe_split_attr->overlap;
    blk_attr->running_mode = running_mode;
    blk_attr->online_ex_en = pipe_split_attr->online_ex_en;

    (td_void)memcpy_s(&blk_attr->frame_rect, sizeof(ot_size), &drv_ctx->work_mode.frame_rect, sizeof(ot_size));
    (td_void)memcpy_s(blk_attr->block_rect, sizeof(ot_rect) * OT_ISP_STRIPING_MAX_NUM, pipe_split_attr->rect,
        sizeof(ot_rect) * OT_ISP_STRIPING_MAX_NUM);
}

static td_s32 isp_drv_work_mode_init(ot_vi_pipe vi_pipe, isp_block_attr *blk_attr)
{
    td_s32 ret;
#ifdef CONFIG_OT_ISP_MCF_SUPPORT
    td_bool nr_en = TD_FALSE;
#endif
    unsigned long flags = 0;
    isp_running_mode running_mode = ISP_MODE_RUNNING_OFFLINE;
    vi_pipe_split_attr pipe_split_attr = { 0 };
    isp_drv_ctx *drv_ctx = TD_NULL;
    ot_frame_interrupt_attr frame_int_attr = { 0 };
    td_bool yuv_mode = TD_FALSE;
    isp_data_input_mode data_input_mode = ISP_MODE_RAW;
    osal_spinlock *isp_spin_lock = TD_NULL;

    isp_check_pipe_return(vi_pipe);
    isp_check_pointer_return(blk_attr);

    drv_ctx = isp_drv_get_ctx(vi_pipe);
    drv_ctx->work_mode.pre_block_num = drv_ctx->work_mode.block_num;

    ret = isp_drv_get_split_attr(vi_pipe, &pipe_split_attr);
    isp_check_return(vi_pipe, ret, "isp_drv_get_split_attr");

#ifdef CONFIG_OT_ISP_MCF_SUPPORT
    ret = isp_drv_get_nr_mode(vi_pipe, &nr_en);
    isp_check_return(vi_pipe, ret, "isp_drv_get_nr_mode");
    drv_ctx->work_mode.is_ia_nr_enable = nr_en;
#endif

    isp_check_block_num_return(pipe_split_attr.split_num);

    ret = isp_drv_get_running_mode(&pipe_split_attr, &running_mode);
    isp_check_return(vi_pipe, ret, "isp_drv_get_running_mode");
    /* online mode not support */
    if ((isp_drv_get_run_wakeup_sel(vi_pipe) == OT_ISP_RUN_WAKEUP_BE_END) && is_online_mode(running_mode)) {
        isp_err_trace("ISP[%d] run_be not support for online!\n", vi_pipe);
        return OT_ERR_ISP_NOT_SUPPORT;
    }
    isp_drv_get_yuv_mode(&pipe_split_attr, &yuv_mode, &data_input_mode);

    ret = isp_drv_get_frame_interrupt_attr(vi_pipe, &frame_int_attr);
    isp_check_return(vi_pipe, ret, "isp_drv_get_frame_interrupt_attr");

    isp_spin_lock = isp_drv_get_lock(vi_pipe);
    osal_spin_lock_irqsave(isp_spin_lock, &flags);

    drv_ctx->yuv_mode = yuv_mode;
    ret = isp_drv_work_mode_update(vi_pipe, &drv_ctx->work_mode, &pipe_split_attr, data_input_mode, running_mode);
    isp_check_ret_goto(ret, ret, out, "vi_pipe[%d] isp_drv_work_mode_update fail!\n", vi_pipe);

    isp_drv_update_blk_attr(drv_ctx, blk_attr, &pipe_split_attr, running_mode);
    drv_ctx->frame_int_attr = frame_int_attr;

out:
    osal_spin_unlock_irqrestore(isp_spin_lock, &flags);

    return ret;
}

static td_s32 isp_drv_work_mode_exit(ot_vi_pipe vi_pipe)
{
    isp_drv_ctx *drv_ctx = TD_NULL;
    isp_check_pipe_return(vi_pipe);

    drv_ctx = isp_drv_get_ctx(vi_pipe);
    isp_check_exit_state_return(vi_pipe, drv_ctx->isp_run_flag);

    drv_ctx->work_mode.running_mode = ISP_MODE_RUNNING_OFFLINE;

    return TD_SUCCESS;
}
static td_s32 isp_drv_get_dng_info(ot_vi_pipe vi_pipe, ot_isp_dng_image_static_info *dng_info)
{
    unsigned long flags = 0;
    isp_drv_ctx *drv_ctx = TD_NULL;
    osal_spinlock *isp_spin_lock = TD_NULL;
    isp_check_pipe_return(vi_pipe);
    isp_check_pointer_return(dng_info);

    drv_ctx = isp_drv_get_ctx(vi_pipe);
    isp_check_tranbuf_init_return(vi_pipe, drv_ctx->trans_info.init);

    isp_spin_lock = isp_drv_get_lock(vi_pipe);
    osal_spin_lock_irqsave(isp_spin_lock, &flags);

    if ((dng_info != TD_NULL) && (drv_ctx->trans_info.dng_info.vir_addr != TD_NULL)) {
        (td_void)memcpy_s(dng_info, sizeof(ot_isp_dng_image_static_info),
            (ot_isp_dng_image_static_info *)drv_ctx->trans_info.dng_info.vir_addr,
            sizeof(ot_isp_dng_image_static_info));
    }

    osal_spin_unlock_irqrestore(isp_spin_lock, &flags);

    return TD_SUCCESS;
}

static td_s32 isp_drv_set_dng_info(ot_vi_pipe vi_pipe, ot_dng_image_dynamic_info *dng_img_dyn_info)
{
    isp_drv_ctx *drv_ctx = TD_NULL;

    isp_check_pipe_return(vi_pipe);
    isp_check_pointer_return(dng_img_dyn_info);

    drv_ctx = isp_drv_get_ctx(vi_pipe);

    (td_void)memcpy_s(&drv_ctx->dng_image_dynamic_info[1], sizeof(ot_dng_image_dynamic_info),
        &drv_ctx->dng_image_dynamic_info[0], sizeof(ot_dng_image_dynamic_info));
    (td_void)memcpy_s(&drv_ctx->dng_image_dynamic_info[0], sizeof(ot_dng_image_dynamic_info), dng_img_dyn_info,
        sizeof(ot_dng_image_dynamic_info));

    return TD_SUCCESS;
}

#ifdef CONFIG_OT_SNAP_SUPPORT
/* vi send Proenable */
static td_s32 isp_set_pro_enable(ot_vi_pipe vi_pipe)
{
    isp_drv_ctx *drv_ctx = TD_NULL;
    unsigned long flags = 0;
    osal_spinlock *isp_spin_lock = TD_NULL;
    isp_check_pipe_return(vi_pipe);

    drv_ctx = isp_drv_get_ctx(vi_pipe);
    isp_spin_lock = isp_drv_get_lock(vi_pipe);
    osal_spin_lock_irqsave(isp_spin_lock, &flags);
    drv_ctx->pro_enable = TD_TRUE;
    drv_ctx->pro_start = TD_FALSE;
    drv_ctx->pro_frm_num = 0;
    drv_ctx->start_snap_num = 0;
    osal_spin_unlock_irqrestore(isp_spin_lock, &flags);

    return TD_SUCCESS;
}

td_s32 isp_set_snap_attr(ot_vi_pipe vi_pipe, const isp_snap_attr *snap_attr)
{
    isp_drv_ctx *drv_ctx = TD_NULL;
    unsigned long flags = 0;
    osal_spinlock *isp_spin_lock = TD_NULL;
    td_u8 i;

    isp_check_pipe_return(vi_pipe);
    isp_check_pointer_return(snap_attr);

    for (i = 0; i < OT_ISP_MAX_PIPE_NUM; i++) {
        drv_ctx = isp_drv_get_ctx(i);
        if (!drv_ctx->mem_init) {
            continue;
        }

        if ((i == snap_attr->picture_pipe_id) || (i == snap_attr->preview_pipe_id)) {
            isp_spin_lock = isp_drv_get_lock(i);
            osal_spin_lock_irqsave(isp_spin_lock, &flags);
            (td_void)memcpy_s(&drv_ctx->snap_attr, sizeof(isp_snap_attr), snap_attr, sizeof(isp_snap_attr));
            osal_spin_unlock_irqrestore(isp_spin_lock, &flags);
            isp_set_pro_enable(i);
        }
    }
    return TD_SUCCESS;
}
#endif

#ifdef CONFIG_OT_SNAP_SUPPORT
static td_s32 isp_drv_set_config_info(ot_vi_pipe vi_pipe, ot_isp_config_info *isp_config_info)
{
    td_u32 i;
    isp_drv_ctx *drv_ctx = TD_NULL;

    isp_check_pipe_return(vi_pipe);
    isp_check_pointer_return(isp_config_info);

    drv_ctx = isp_drv_get_ctx(vi_pipe);

    for (i = ISP_SAVEINFO_MAX - 1; i >= 1; i--) {
        (td_void)memcpy_s(&drv_ctx->snap_info_save[i], sizeof(ot_isp_config_info), &drv_ctx->snap_info_save[i - 1],
            sizeof(ot_isp_config_info));
    }

    (td_void)memcpy_s(&drv_ctx->snap_info_save[0], sizeof(ot_isp_config_info), isp_config_info,
        sizeof(ot_isp_config_info));

    return TD_SUCCESS;
}
#endif

static td_s32 isp_ioctl_set_int_enable(unsigned int cmd, void *arg, void *private_data)
{
    ot_vi_pipe vi_pipe = isp_get_pipe(private_data);
    td_bool en;
    isp_check_pointer_return(arg);
    isp_check_pipe_return(vi_pipe);

    ot_unused(cmd);

    en = *(td_bool *)arg;
    return isp_drv_set_int_enable(vi_pipe, en);
}

static td_s32 isp_ioctl_set_mem_info(unsigned int cmd, void *arg, void *private_data)
{
    ot_vi_pipe vi_pipe = isp_get_pipe(private_data);
    isp_drv_ctx *drv_ctx = TD_NULL;
    td_s32 ret;
    isp_check_pipe_return(vi_pipe);
    isp_check_pointer_return(arg);
    isp_check_bool_return(*(td_bool *)arg);

    ot_unused(cmd);
    ret = isp_drv_mem_share_owner(vi_pipe);
    if (ret != TD_SUCCESS) {
        return ret;
    }
    drv_ctx = isp_drv_get_ctx(vi_pipe);
    drv_ctx->mem_init = TD_TRUE;

    return TD_SUCCESS;
}

static td_s32 isp_ioctl_get_mem_info(unsigned int cmd, void *arg, void *private_data)
{
    ot_vi_pipe vi_pipe = isp_get_pipe(private_data);
    isp_drv_ctx *drv_ctx = TD_NULL;
    isp_check_pipe_return(vi_pipe);
    isp_check_pointer_return(arg);

    ot_unused(cmd);
    drv_ctx = isp_drv_get_ctx(vi_pipe);
    *(td_bool *)arg = drv_ctx->mem_init;

    return TD_SUCCESS;
}

static td_s32 isp_ioctl_set_init_info(unsigned int cmd, void *arg, void *private_data)
{
    ot_vi_pipe vi_pipe = isp_get_pipe(private_data);
    isp_drv_ctx *drv_ctx = TD_NULL;
    isp_check_pipe_return(vi_pipe);
    isp_check_pointer_return(arg);
    ot_unused(cmd);

    isp_check_bool_return(*(td_bool *)arg);
    drv_ctx = isp_drv_get_ctx(vi_pipe);
    if (osal_sem_down(&drv_ctx->isp_sem_buf)) {
        return -ERESTARTSYS;
    }

    isp_dfx_ctx_init(vi_pipe);
    osal_sem_up(&drv_ctx->isp_sem_buf);
    drv_ctx->isp_init = *(td_bool *)arg;

    return TD_SUCCESS;
}

static td_s32 isp_ioctl_get_init_info(unsigned int cmd, void *arg, void *private_data)
{
    ot_vi_pipe vi_pipe = isp_get_pipe(private_data);
    isp_drv_ctx *drv_ctx = TD_NULL;
    isp_check_pipe_return(vi_pipe);
    isp_check_pointer_return(arg);
    ot_unused(cmd);

    drv_ctx = isp_drv_get_ctx(vi_pipe);
    *(td_bool *)arg = drv_ctx->isp_init;

    return TD_SUCCESS;
}

static td_s32 isp_ioctl_set_run_state(unsigned int cmd, void *arg, void *private_data)
{
    ot_vi_pipe vi_pipe = isp_get_pipe(private_data);
    td_u64 *hand_signal = TD_NULL;
    isp_check_pipe_return(vi_pipe);
    ot_unused(cmd);

    hand_signal = (td_u64 *)arg;
    return isp_drv_set_isp_run_state(vi_pipe, hand_signal);
}

static td_s32 isp_ioctl_get_wdr_attr(unsigned int cmd, void *arg, void *private_data)
{
    ot_vi_pipe vi_pipe = isp_get_pipe(private_data);
    vi_pipe_wdr_attr *wdr_attr = TD_NULL;
    isp_check_pipe_return(vi_pipe);
    ot_unused(cmd);

    wdr_attr = (vi_pipe_wdr_attr *)arg;
    return isp_drv_get_wdr_attr(vi_pipe, wdr_attr);
}

#ifdef CONFIG_OT_VI_DISTRIBUTE_GRP
static td_s32 isp_ioctl_get_dist_attr(unsigned int cmd, void *arg, void *private_data)
{
    ot_vi_pipe vi_pipe = isp_get_pipe(private_data);
    vi_distribute_attr *attr = TD_NULL;
    isp_check_pipe_return(vi_pipe);
    ot_unused(cmd);

    attr = (vi_distribute_attr *)arg;
    return isp_drv_get_dist_attr(vi_pipe, attr);
}

static td_s32 isp_ioctl_get_dist_fps_status(unsigned int cmd, void *arg, void *private_data)
{
    ot_vi_pipe vi_pipe = isp_get_pipe(private_data);
    isp_drv_ctx *tmp_drv_ctx = TD_NULL;
    vi_distribute_attr dist_attr = {0};
    isp_check_pipe_return(vi_pipe);
    isp_check_pointer_return(arg);
    ot_unused(cmd);

    tmp_drv_ctx = isp_drv_get_ctx(vi_pipe);
    if (tmp_drv_ctx->isp_init != TD_TRUE || tmp_drv_ctx->dist_attr.distribute_en == TD_FALSE) {
        return TD_FAILURE;
    }

    if (!ckfn_vi_get_distribute_attr() || call_vi_get_distribute_attr(vi_pipe, &dist_attr) != TD_SUCCESS) {
        isp_err_trace("pipe %d get vi distribute attr error\n", vi_pipe);
        return TD_FAILURE;
    }

    return isp_drv_get_dist_fps_status(vi_pipe, &dist_attr, arg);
}
#endif

static td_s32 isp_ioctl_update_pre_blk_num(unsigned int cmd, void *arg, void *private_data)
{
    ot_vi_pipe vi_pipe = isp_get_pipe(private_data);
    isp_drv_ctx *drv_ctx = TD_NULL;
    isp_check_pipe_return(vi_pipe);
    isp_check_pointer_return(arg);
    ot_unused(cmd);

    if ((*(td_u8 *)arg == 0) || (*(td_u8 *)arg > OT_ISP_STRIPING_MAX_NUM)) {
        isp_err_trace("pipe[%d] pre_block_num:%d is not in [1, %d]\n",
            vi_pipe, *(td_u8 *)arg, OT_ISP_STRIPING_MAX_NUM);
        return -OT_ERR_ISP_ILLEGAL_PARAM;
    }

    drv_ctx = isp_drv_get_ctx(vi_pipe);
    drv_ctx->work_mode.pre_block_num = *(td_u8 *)arg;

    return TD_SUCCESS;
}

static td_s32 isp_ioctl_get_pipe_size(unsigned int cmd, void *arg, void *private_data)
{
    ot_vi_pipe vi_pipe = isp_get_pipe(private_data);
    ot_size *pipe_size = TD_NULL;
    isp_check_pipe_return(vi_pipe);
    ot_unused(cmd);

    pipe_size = (ot_size *)arg;
    return isp_drv_get_pipe_size(vi_pipe, pipe_size);
}

static td_s32 isp_ioctl_get_hdr_attr(unsigned int cmd, void *arg, void *private_data)
{
    ot_vi_pipe vi_pipe = isp_get_pipe(private_data);
    vi_pipe_hdr_attr *hdr_attr = TD_NULL;
    isp_check_pipe_return(vi_pipe);
    ot_unused(cmd);

    hdr_attr = (vi_pipe_hdr_attr *)arg;
    return isp_drv_get_hdr_attr(vi_pipe, hdr_attr);
}

static td_s32 isp_ioctl_get_p2_en_info(unsigned int cmd, void *arg, void *private_data)
{
    ot_vi_pipe vi_pipe = isp_get_pipe(private_data);
    td_bool *p2_en = TD_NULL;
    isp_check_pipe_return(vi_pipe);
    ot_unused(cmd);

    p2_en = (td_bool *)arg;
    return isp_drv_get_p2_en_info(vi_pipe, p2_en);
}
#ifdef CONFIG_OT_VI_STITCH_GRP
static td_s32 isp_ioctl_get_stitch_attr(unsigned int cmd, void *arg, void *private_data)
{
    ot_vi_pipe vi_pipe = isp_get_pipe(private_data);
    vi_stitch_attr *stitch_attr = TD_NULL;
    isp_check_pipe_return(vi_pipe);
    ot_unused(cmd);

    stitch_attr = (vi_stitch_attr *)arg;
    return isp_drv_get_stitch_attr(vi_pipe, stitch_attr);
}
#endif

static td_s32 isp_ioctl_get_version(unsigned int cmd, void *arg, void *private_data)
{
    ot_vi_pipe vi_pipe = isp_get_pipe(private_data);
    isp_version *version = TD_NULL;
    isp_check_pipe_return(vi_pipe);
    ot_unused(cmd);

    version = (isp_version *)arg;
    return isp_drv_get_version(version);
}

static td_s32 isp_ioctl_init_trans_info(unsigned int cmd, void *arg, void *private_data)
{
    ot_vi_pipe vi_pipe = isp_get_pipe(private_data);
    td_s32 ret;
    isp_drv_ctx *drv_ctx = TD_NULL;
    isp_trans_info_buf *trans_info = TD_NULL;
    isp_check_pipe_return(vi_pipe);
    ot_unused(cmd);

    trans_info = (isp_trans_info_buf *)arg;
    drv_ctx = isp_drv_get_ctx(vi_pipe);
    if (osal_sem_down(&drv_ctx->isp_sem_buf)) {
        return -ERESTARTSYS;
    }
    ret = isp_drv_trans_info_buf_init(vi_pipe, trans_info);
    osal_sem_up(&drv_ctx->isp_sem_buf);
    return ret;
}

static td_s32 isp_ioctl_init_stat_buf(unsigned int cmd, void *arg, void *private_data)
{
    ot_vi_pipe vi_pipe = isp_get_pipe(private_data);
    td_s32 ret;
    isp_drv_ctx *drv_ctx = TD_NULL;
    td_phys_addr_t *stat_phy_addr = TD_NULL;
    isp_check_pipe_return(vi_pipe);
    ot_unused(cmd);

    stat_phy_addr = (td_phys_addr_t *)arg;
    drv_ctx = isp_drv_get_ctx(vi_pipe);
    if (osal_sem_down(&drv_ctx->isp_sem_buf)) {
        return -ERESTARTSYS;
    }
    ret = isp_drv_stats_buf_init(vi_pipe, stat_phy_addr);
    osal_sem_up(&drv_ctx->isp_sem_buf);
    return ret;
}

static td_s32 isp_ioctl_init_be_cfg_buf(unsigned int cmd, void *arg, void *private_data)
{
    ot_vi_pipe vi_pipe = isp_get_pipe(private_data);
    td_s32 ret;
    isp_drv_ctx *drv_ctx = TD_NULL;
    isp_mmz_buf_ex *be_cfg_buf = TD_NULL;
    isp_check_pipe_return(vi_pipe);
    ot_unused(cmd);

    be_cfg_buf = (isp_mmz_buf_ex *)arg;
    drv_ctx = isp_drv_get_ctx(vi_pipe);
    if (osal_sem_down(&drv_ctx->isp_sem_buf)) {
        return -ERESTARTSYS;
    }
    ret = isp_drv_be_buf_init(vi_pipe, be_cfg_buf);
    osal_sem_up(&drv_ctx->isp_sem_buf);
    return ret;
}

static td_s32 isp_ioctl_init_be_all_buf(unsigned int cmd, void *arg, void *private_data)
{
    td_s32 ret;
    isp_drv_ctx *drv_ctx = TD_NULL;
    ot_vi_pipe vi_pipe = isp_get_pipe(private_data);
    ot_unused(cmd);
    ot_unused(arg);

    isp_check_pipe_return(vi_pipe);
    drv_ctx = isp_drv_get_ctx(vi_pipe);
    if (osal_sem_down(&drv_ctx->isp_sem_buf)) {
        return -ERESTARTSYS;
    }
    ret = isp_drv_all_be_buf_init(vi_pipe);
    osal_sem_up(&drv_ctx->isp_sem_buf);
    return ret;
}

#ifdef CONFIG_OT_ISP_CLUT_SUPPORT
static td_s32 isp_ioctl_init_clut_buf(unsigned int cmd, void *arg, void *private_data)
{
    td_s32 ret = TD_FAILURE;
    isp_drv_ctx *drv_ctx = TD_NULL;
    ot_vi_pipe vi_pipe = isp_get_pipe(private_data);
    ot_unused(arg);
    ot_unused(cmd);
    isp_check_pipe_return(vi_pipe);

    drv_ctx = isp_drv_get_ctx(vi_pipe);
    if (osal_sem_down(&drv_ctx->isp_sem_buf)) {
        return -ERESTARTSYS;
    }
    ret = isp_drv_clut_buf_init(vi_pipe);
    osal_sem_up(&drv_ctx->isp_sem_buf);
    return ret;
}
#endif

static td_s32 isp_ioctl_init_drc_buf(unsigned int cmd, void *arg, void *private_data)
{
    td_s32 ret;
    ot_vi_pipe vi_pipe = isp_get_pipe(private_data);
    isp_drv_ctx *drv_ctx = TD_NULL;
    isp_check_pipe_return(vi_pipe);
    ot_unused(arg);
    ot_unused(cmd);

    drv_ctx = isp_drv_get_ctx(vi_pipe);
    if (osal_sem_down(&drv_ctx->isp_sem_buf)) {
        return -ERESTARTSYS;
    }
    ret = isp_drv_drc_buf_init(vi_pipe);
    osal_sem_up(&drv_ctx->isp_sem_buf);
    return ret;
}

static td_s32 isp_ioctl_init_ldci_buf(unsigned int cmd, void *arg, void *private_data)
{
    ot_vi_pipe vi_pipe = isp_get_pipe(private_data);
    td_s32 ret;
    isp_drv_ctx *drv_ctx = TD_NULL;
    isp_check_pipe_return(vi_pipe);

    drv_ctx = isp_drv_get_ctx(vi_pipe);
    if (osal_sem_down(&drv_ctx->isp_sem_buf)) {
        return -ERESTARTSYS;
    }
    ret = isp_drv_ldci_buf_init(vi_pipe);
    osal_sem_up(&drv_ctx->isp_sem_buf);
    ot_unused(cmd);
    ot_unused(arg);
    return ret;
}

static td_s32 isp_ioctl_init_stt_buf(unsigned int cmd, void *arg, void *private_data)
{
    td_s32 ret;
    ot_vi_pipe vi_pipe = isp_get_pipe(private_data);
    isp_drv_ctx *drv_ctx = TD_NULL;
    isp_check_pipe_return(vi_pipe);

    drv_ctx = isp_drv_get_ctx(vi_pipe);
    if (osal_sem_down(&drv_ctx->isp_sem_buf)) {
        return -ERESTARTSYS;
    }
    ret = isp_drv_stt_buf_init(vi_pipe);
    osal_sem_up(&drv_ctx->isp_sem_buf);
    ot_unused(cmd);
    ot_unused(arg);

    return ret;
}

static td_s32 isp_ioctl_get_ldci_read_stt_buf(unsigned int cmd, void *arg, void *private_data)
{
    ot_vi_pipe vi_pipe = isp_get_pipe(private_data);
    isp_check_pipe_return(vi_pipe);
    ot_unused(cmd);

    return isp_drv_ldci_read_stt_buf_get(vi_pipe, (isp_ldci_read_stt_buf *)arg);
}

static td_s32 isp_ioctl_init_stt_addr(unsigned int cmd, void *arg, void *private_data)
{
    ot_vi_pipe vi_pipe = isp_get_pipe(private_data);
    isp_drv_ctx *drv_ctx = TD_NULL;

    isp_check_pipe_return(vi_pipe);
    isp_check_no_fe_pipe_return(vi_pipe);
    ot_unused(cmd);
    ot_unused(arg);

    drv_ctx = isp_drv_get_ctx(vi_pipe);
    if (drv_ctx->stitch_attr.stitch_enable == TD_FALSE) {
        return isp_drv_fe_stt_addr_init(vi_pipe);
    } else {
#ifdef CONFIG_OT_VI_STITCH_GRP
        return isp_drv_fe_stitch_stt_addr_init(vi_pipe);
#else
        return TD_SUCCESS;
#endif
    }
}

static td_s32 isp_ioctl_get_be_lut_stt_buf(unsigned int cmd, void *arg, void *private_data)
{
    ot_vi_pipe vi_pipe = isp_get_pipe(private_data);
    isp_drv_ctx *drv_ctx = TD_NULL;
    td_phys_addr_t phy_addr;
    isp_check_pipe_return(vi_pipe);
    isp_check_pointer_return(arg);
    ot_unused(cmd);
    drv_ctx = isp_drv_get_ctx(vi_pipe);
    if (drv_ctx->be_lut2stt_attr.init == TD_FALSE) {
        return OT_ERR_ISP_NOMEM;
    }
    phy_addr = drv_ctx->be_lut2stt_attr.be_lut_stt_buf[0].lut_stt_buf[0].phy_addr;

    if (ot_mmz_check_phys_addr(phy_addr, drv_ctx->be_lut2stt_attr.be_lut_stt_buf[0].lut_stt_buf[0].size) !=
        TD_SUCCESS) {
        isp_err_trace("be lut check error\n");
        *(td_phys_addr_t *)arg = 0;
        return OT_ERR_ISP_NOT_SUPPORT;
    }
    *(td_phys_addr_t *)arg = phy_addr;

    return TD_SUCCESS;
}

#ifdef CONFIG_OT_VI_PREONLINE_POSTOFFLINE
static td_s32 isp_ioctl_get_be_pre_lut_stt_buf(unsigned int cmd, void *arg, void *private_data)
{
    ot_vi_pipe vi_pipe = isp_get_pipe(private_data);
    td_phys_addr_t phy_addr;
    isp_drv_ctx *drv_ctx = TD_NULL;
    ot_unused(cmd);
    isp_check_pipe_return(vi_pipe);
    isp_check_pointer_return(arg);
    drv_ctx = isp_drv_get_ctx(vi_pipe);
    if (drv_ctx->be_pre_on_post_off_lut2stt_attr.init == TD_FALSE) {
        return OT_ERR_ISP_NOMEM;
    }
    phy_addr = drv_ctx->be_pre_on_post_off_lut2stt_attr.be_lut_stt_buf[0].lut_stt_buf[0].phy_addr;
    if (ot_mmz_check_phys_addr(phy_addr,
        drv_ctx->be_pre_on_post_off_lut2stt_attr.be_lut_stt_buf[0].lut_stt_buf[0].size) != TD_SUCCESS) {
        *(td_phys_addr_t *)arg = 0;
        isp_err_trace("be pre lut check error\n");
        return OT_ERR_ISP_NOT_SUPPORT;
    }
    *(td_phys_addr_t *)arg = phy_addr;
    return TD_SUCCESS;
}
#endif

static td_s32 isp_ioctl_set_dev_fd(unsigned int cmd, void *arg, void *private_data)
{
    ot_unused(cmd);
    isp_check_pointer_return(arg);
    isp_check_pointer_return(private_data);

    *((td_u32 *)private_data) = *(td_u32 *)arg;

    return TD_SUCCESS;
}

static td_s32 isp_ioctl_get_pwm_num(unsigned int cmd, void *arg, void *private_data)
{
    ot_vi_pipe vi_pipe = isp_get_pipe(private_data);
    isp_check_pipe_return(vi_pipe);
    isp_check_pointer_return(arg);
    ot_unused(cmd);

    *(td_u32 *)arg = isp_drv_get_pwm_number(vi_pipe);

    return TD_SUCCESS;
}

static td_s32 isp_ioctl_opt_run_once_info(unsigned int cmd, void *arg, void *private_data)
{
    ot_vi_pipe vi_pipe = isp_get_pipe(private_data);
    isp_check_pipe_return(vi_pipe);
    ot_unused(cmd);

    return isp_drv_opt_run_once_info(vi_pipe, (td_bool *)arg);
}

static td_s32 isp_ioctl_yuv_run_once_info(unsigned int cmd, void *arg, void *private_data)
{
    ot_vi_pipe vi_pipe = isp_get_pipe(private_data);
    isp_check_pipe_return(vi_pipe);
    ot_unused(cmd);

    return isp_drv_yuv_run_once_info(vi_pipe, (td_bool *)arg);
}

static td_s32 isp_ioctl_run_once_process(unsigned int cmd, void *arg, void *private_data)
{
    ot_vi_pipe vi_pipe = isp_get_pipe(private_data);
    isp_check_pipe_return(vi_pipe);
    ot_unused(cmd);
    ot_unused(arg);

    return isp_drv_run_once_process(vi_pipe);
}

static td_s32 isp_ioctl_yuv_run_once_process(unsigned int cmd, void *arg, void *private_data)
{
    ot_vi_pipe vi_pipe = isp_get_pipe(private_data);
    isp_check_pipe_return(vi_pipe);
    ot_unused(cmd);
    ot_unused(arg);

    return isp_drv_yuv_run_once_process(vi_pipe);
}

static td_s32 isp_ioctl_run_trigger_process(unsigned int cmd, void *arg, void *private_data)
{
    ot_vi_pipe vi_pipe = isp_get_pipe(private_data);
    isp_check_pipe_return(vi_pipe);
    ot_unused(cmd);
    ot_unused(arg);

    return isp_drv_run_trigger_process(vi_pipe);
}

static td_s32 isp_ioctl_set_reg_cfg(unsigned int cmd, void *arg, void *private_data)
{
    ot_vi_pipe vi_pipe = isp_get_pipe(private_data);
    isp_kernel_reg_cfg *reg_kernel_cfg = TD_NULL;
    isp_check_pipe_return(vi_pipe);
    ot_unused(cmd);

    reg_kernel_cfg = (isp_kernel_reg_cfg *)arg;

    return isp_drv_set_reg_kernel_cfgs(vi_pipe, reg_kernel_cfg);
}

static td_s32 isp_ioctl_chn_select_cfg(unsigned int cmd, void *arg, void *private_data)
{
    ot_vi_pipe vi_pipe = isp_get_pipe(private_data);
    td_u32 channel_sel;
    isp_check_pipe_return(vi_pipe);
    isp_check_pointer_return(arg);
    ot_unused(cmd);

    channel_sel = *(td_u32 *)arg;

    return isp_drv_chn_select_cfg(vi_pipe, channel_sel);
}

#ifdef CONFIG_OT_ISP_MCF_SUPPORT
static td_s32 isp_ioctl_get_mcf_en(unsigned int cmd, void *arg, void *private_data)
{
    ot_vi_pipe vi_pipe = isp_get_pipe(private_data);
    isp_drv_ctx *drv_ctx = TD_NULL;
    td_bool *nr_en = TD_NULL;
    isp_check_pipe_return(vi_pipe);
    isp_check_pointer_return(arg);
    ot_unused(cmd);

    drv_ctx = isp_drv_get_ctx(vi_pipe);
    nr_en = (td_bool *)arg;
    isp_drv_get_nr_mode(vi_pipe, nr_en);
    drv_ctx->work_mode.is_ia_nr_enable = *nr_en;
    return TD_SUCCESS;
}
#endif

static td_s32 isp_ioctl_set_bnr_temporal_filt(unsigned int cmd, void *arg, void *private_data)
{
    ot_vi_pipe vi_pipe = isp_get_pipe(private_data);
    isp_check_pipe_return(vi_pipe);
    ot_unused(cmd);

    return isp_drv_bnr_temporal_filt_set(vi_pipe, (isp_bnr_temporal_filt *)arg);
}

#ifdef IQ_DEBUG
static td_s32 isp_ioctl_set_bnr_mot_bitw(unsigned int cmd, void *arg, void *private_data)
{
    ot_vi_pipe vi_pipe = isp_get_pipe(private_data);
    isp_check_pipe_return(vi_pipe);
    isp_check_pointer_return(arg);
    ot_unused(cmd);

    return isp_drv_bnr_mot_bitw_set(vi_pipe, (td_u8 *)arg);
}
#endif

static td_s32 isp_ioctl_disable_bnr_wmot(unsigned int cmd, void *arg, void *private_data)
{
    ot_vi_pipe vi_pipe = isp_get_pipe(private_data);
    isp_check_pipe_return(vi_pipe);
    ot_unused(cmd);

    return isp_drv_disable_bnr_wmot(vi_pipe);
}

static td_s32 isp_ioctl_set_mot_cfg(unsigned int cmd, void *arg, void *private_data)
{
    ot_vi_pipe vi_pipe = isp_get_pipe(private_data);
    isp_check_pipe_return(vi_pipe);
    ot_unused(cmd);

    return isp_drv_sharpen_mot_set(vi_pipe, (td_bool *)arg);
}

#ifdef CONFIG_OT_ISP_CLUT_SUPPORT
static td_s32 isp_ioctl_get_clut_buf(unsigned int cmd, void *arg, void *private_data)
{
    ot_vi_pipe vi_pipe = isp_get_pipe(private_data);
    isp_drv_ctx *drv_ctx = TD_NULL;
    isp_check_pipe_return(vi_pipe);
    isp_check_pointer_return(arg);
    ot_unused(cmd);
    drv_ctx = isp_drv_get_ctx(vi_pipe);
    if (drv_ctx->clut_buf_attr.init == TD_FALSE) {
        return OT_ERR_ISP_NOMEM;
    }

    if (ot_mmz_check_phys_addr(drv_ctx->clut_buf_attr.clut_buf.phy_addr,
        drv_ctx->clut_buf_attr.clut_buf.size) != TD_SUCCESS) {
        isp_err_trace("check clut buf addr error\n");
        return OT_ERR_ISP_NOT_SUPPORT;
    }
    *(td_phys_addr_t *)arg = drv_ctx->clut_buf_attr.clut_buf.phy_addr;

    return TD_SUCCESS;
}
#endif

static td_s32 isp_ioctl_set_sync_cfg(unsigned int cmd, void *arg, void *private_data)
{
    ot_vi_pipe vi_pipe = isp_get_pipe(private_data);
    isp_sync_cfg_buf_node *sync_cfg_buf = TD_NULL;
    isp_check_pipe_return(vi_pipe);
    ot_unused(cmd);

    sync_cfg_buf = (isp_sync_cfg_buf_node *)arg;

    return isp_drv_sync_cfg_set(vi_pipe, sync_cfg_buf);
}

#ifdef CONFIG_OT_SYS_KOL_SUPPORT
static td_s32 isp_ioctl_init_standby_cfg(unsigned int cmd, void *arg, void *private_data)
{
    ot_vi_pipe vi_pipe = isp_get_pipe(private_data);
    isp_check_pipe_return(vi_pipe);
    ot_unused(cmd);

    return isp_drv_standby_cfg_init(vi_pipe, (ot_isp_sns_regs_info *)arg);
}
#endif

static td_s32 isp_ioctl_init_be_sync_param(unsigned int cmd, void *arg, void *private_data)
{
    ot_vi_pipe vi_pipe = isp_get_pipe(private_data);
    isp_check_pipe_return(vi_pipe);
    ot_unused(cmd);

    return isp_drv_be_sync_param_init(vi_pipe, (isp_be_sync_para *)arg);
}

static td_s32 isp_ioctl_init_stt_sync(unsigned int cmd, void *arg, void *private_data)
{
    ot_vi_pipe vi_pipe = isp_get_pipe(private_data);
    isp_drv_ctx *drv_ctx = TD_NULL;
    isp_check_pipe_return(vi_pipe);
    isp_check_pointer_return(arg);
    isp_check_bool_return(*(td_bool *)arg);
    ot_unused(cmd);

    drv_ctx = isp_drv_get_ctx(vi_pipe);

    drv_ctx->stitch_sync = TD_FALSE;
#ifdef CONFIG_OT_VI_STITCH_GRP
    if (drv_ctx->stitch_attr.stitch_enable == TD_TRUE) {
        drv_ctx->stitch_sync = *(td_bool *)arg;
    }
#endif
    return TD_SUCCESS;
}

static td_s32 isp_switch_ctx_reset(ot_vi_pipe vi_pipe)
{
    isp_drv_ctx *drv_ctx = TD_NULL;
    unsigned long flags = 0;
    osal_spinlock *isp_spin_lock = TD_NULL;

    drv_ctx = isp_drv_get_ctx(vi_pipe);
    isp_spin_lock = isp_drv_get_spin_lock(vi_pipe);
    osal_spin_lock_irqsave(isp_spin_lock, &flags);
    drv_ctx->edge = TD_FALSE;
    osal_spin_unlock_irqrestore(isp_spin_lock, &flags);

    return TD_SUCCESS;
}

static td_s32 isp_ioctl_set_res_switch(unsigned int cmd, void *arg, void *private_data)
{
    ot_vi_pipe vi_pipe = isp_get_pipe(private_data);
    isp_check_pipe_return(vi_pipe);
    ot_unused(cmd);
    ot_unused(arg);

    if (isp_switch_ctx_reset(vi_pipe) != TD_SUCCESS) {
        return TD_FAILURE;
    }

    return isp_drv_be_buf_switch_state(vi_pipe);
}

static td_s32 isp_ioctl_set_be_buf_switch_finish(unsigned int cmd, void *arg, void *private_data)
{
    ot_vi_pipe vi_pipe = isp_get_pipe(private_data);
    isp_check_pipe_return(vi_pipe);
    ot_unused(cmd);
    ot_unused(arg);

    return isp_drv_be_buf_switch_finish_state(vi_pipe);
}

static td_s32 isp_ioctl_set_wdr_cfg(unsigned int cmd, void *arg, void *private_data)
{
    ot_vi_pipe vi_pipe = isp_get_pipe(private_data);
    isp_check_pipe_return(vi_pipe);
    ot_unused(cmd);

    return isp_drv_set_wdr_cfg(vi_pipe, (isp_wdr_cfg *)arg);
}

#ifdef CONFIG_OT_VI_STITCH_GRP
static td_s32 isp_ioctl_init_stitch_sync_param(unsigned int cmd, void *arg, void *private_data)
{
    ot_vi_pipe vi_pipe = isp_get_pipe(private_data);
    isp_check_pipe_return(vi_pipe);
    ot_unused(cmd);
    ot_unused(arg);

    return isp_drv_stitch_sync_ctrl_init(vi_pipe);
}
#endif

static td_s32 isp_ioctl_set_mod_param(unsigned int cmd, void *arg, void *private_data)
{
    ot_vi_pipe vi_pipe = isp_get_pipe(private_data);
    isp_check_pipe_return(vi_pipe);
    ot_unused(cmd);

    return isp_set_mod_param((ot_isp_mod_param *)arg);
}

static td_s32 isp_ioctl_get_mod_param(unsigned int cmd, void *arg, void *private_data)
{
    ot_vi_pipe vi_pipe = isp_get_pipe(private_data);
    isp_check_pipe_return(vi_pipe);
    ot_unused(cmd);

    return isp_get_mod_param((ot_isp_mod_param *)arg);
}

static td_s32 isp_ioctl_set_ctrl_param(unsigned int cmd, void *arg, void *private_data)
{
    ot_vi_pipe vi_pipe = isp_get_pipe(private_data);
    isp_check_pipe_return(vi_pipe);
    ot_unused(cmd);

    return isp_set_ctrl_param(vi_pipe, (ot_isp_ctrl_param *)arg);
}

static td_s32 isp_ioctl_get_ctrl_param(unsigned int cmd, void *arg, void *private_data)
{
    ot_vi_pipe vi_pipe = isp_get_pipe(private_data);
    isp_check_pipe_return(vi_pipe);
    ot_unused(cmd);

    return isp_get_ctrl_param(vi_pipe, (ot_isp_ctrl_param *)arg);
}
#ifdef CONFIG_OT_ISP_DETAIL_STATS_SUPPORT
static td_s32 isp_ioctl_set_detail_stats_cfg(unsigned int cmd, void *arg, void *private_data)
{
    ot_vi_pipe vi_pipe = isp_get_pipe(private_data);
    isp_check_fe_pipe_return(vi_pipe);
    ot_unused(cmd);

    return isp_set_detail_stats_cfg(vi_pipe, (ot_isp_detail_stats_cfg *)arg);
}

static td_s32 isp_ioctl_get_detail_stats_cfg(unsigned int cmd, void *arg, void *private_data)
{
    ot_vi_pipe vi_pipe = isp_get_pipe(private_data);
    isp_check_fe_pipe_return(vi_pipe);
    ot_unused(cmd);

    return isp_get_detail_stats_cfg(vi_pipe, (ot_isp_detail_stats_cfg *)arg);
}

static td_s32 isp_ioctl_get_detail_blk_size(unsigned int cmd, void *arg, void *private_data)
{
    ot_vi_pipe vi_pipe = isp_get_pipe(private_data);
    isp_detail_size *detail_size = TD_NULL;
    isp_check_pipe_return(vi_pipe);
    ot_unused(cmd);

    detail_size = (isp_detail_size *)arg;
    return isp_drv_get_detail_blk_size(vi_pipe, detail_size);
}

static td_s32 isp_ioctl_init_detail_stats_buf(unsigned int cmd, void *arg, void *private_data)
{
    td_s32 ret, ret2;
    isp_drv_ctx *drv_ctx = TD_NULL;
    isp_detail_stats_info *detail_stats_info = TD_NULL;
    ot_vi_pipe vi_pipe = isp_get_pipe(private_data);
    isp_check_pipe_return(vi_pipe);
    isp_check_pointer_return(arg);
    drv_ctx = isp_drv_get_ctx(vi_pipe);
    detail_stats_info = (isp_detail_stats_info *)arg;

    ot_unused(cmd);
    if (osal_sem_down(&drv_ctx->isp_sem_buf)) {
        return -ERESTARTSYS;
    }

    if (drv_ctx->detail_stats_cfg.enable == TD_FALSE) {
        osal_sem_up(&drv_ctx->isp_sem_buf);
        return TD_SUCCESS;
    }
    isp_check_fe_pipe_return(vi_pipe);
    if (is_offline_mode(drv_ctx->work_mode.running_mode) == TD_FALSE) {
        osal_sem_up(&drv_ctx->isp_sem_buf);
        isp_err_trace("pipe:%d only support offline mode:%d\n", vi_pipe, (drv_ctx->work_mode.running_mode));
        return OT_ERR_ISP_NOT_SUPPORT;
    }

    ret = isp_drv_detail_stats_buf_init(vi_pipe);
    if (ret != TD_SUCCESS) {
        osal_sem_up(&drv_ctx->isp_sem_buf);
        isp_err_trace("pipe:%d, init detail stats error:%x\n", vi_pipe, ret);
        return ret;
    }

    ret = isp_drv_get_detail_blk_size(vi_pipe, &detail_stats_info->detail_size);
    if (ret != TD_SUCCESS) {
        isp_err_trace("pipe:%d, get_detail_blk_size error:%x\n", vi_pipe, ret);
        goto fail_ret;
    }

    ret = memcpy_s(&detail_stats_info->stats_cfg, sizeof(ot_isp_detail_stats_cfg),
        &drv_ctx->detail_stats_cfg, sizeof(ot_isp_detail_stats_cfg));
    if (ret != TD_SUCCESS) {
        isp_err_trace("pipe:%d, memcpy_s error ret:%x\n", vi_pipe, ret);
        goto fail_ret;
    }

    osal_sem_up(&drv_ctx->isp_sem_buf);
    return TD_SUCCESS;
fail_ret:
    ret2 = isp_drv_detail_stats_buf_exit(vi_pipe);
    if (ret2 != TD_SUCCESS) {
        isp_err_trace("pipe:%d, detail_stats_buf_exit error:%x\n", vi_pipe, ret2);
    }
    osal_sem_up(&drv_ctx->isp_sem_buf);

    return ret;
}

static td_s32 isp_ioctl_detail_stats_buf_exit(unsigned int cmd, void *arg, void *private_data)
{
    td_s32 ret;
    isp_drv_ctx *drv_ctx = TD_NULL;
    ot_vi_pipe vi_pipe = isp_get_pipe(private_data);
    isp_check_pipe_return(vi_pipe);
    drv_ctx = isp_drv_get_ctx(vi_pipe);

    ot_unused(cmd);
    ot_unused(arg);
    if (osal_sem_down(&drv_ctx->isp_sem_buf)) {
        return -ERESTARTSYS;
    }

    ret = isp_drv_detail_stats_buf_exit(vi_pipe);
    osal_sem_up(&drv_ctx->isp_sem_buf);
    return ret;
}

static td_s32 isp_ioctl_get_detail_stats(unsigned int cmd, void *arg, void *private_data)
{
    td_s32 ret;
    ot_vi_pipe vi_pipe;
    isp_detail_stats_user *detail_stats_to_user = TD_NULL;
    ot_isp_detail_stats *detail_stats = TD_NULL;
    ot_isp_detail_stats *detail_stats_dest = TD_NULL;
    isp_drv_ctx *drv_ctx = TD_NULL;
    td_void *vir_addr = TD_NULL;

    vi_pipe = isp_get_pipe(private_data);
    isp_check_fe_pipe_return(vi_pipe);
    drv_ctx = isp_drv_get_ctx(vi_pipe);
    ot_unused(cmd);

    detail_stats_to_user = (isp_detail_stats_user *)arg;
    isp_check_pointer_return(detail_stats_to_user);
    detail_stats_dest = (ot_isp_detail_stats *)detail_stats_to_user->detail_stats;
    isp_check_pointer_return(detail_stats_dest);

    if (osal_sem_down(&drv_ctx->isp_sem_buf)) {
        return -ERESTARTSYS;
    }

    ret = isp_drv_check_detail_stats_param(vi_pipe, drv_ctx);
    if (ret != TD_SUCCESS) {
        osal_sem_up(&drv_ctx->isp_sem_buf);
        return ret;
    }

    vir_addr = drv_ctx->detail_stats_buf.detail_buf.vir_addr;
    detail_stats = (ot_isp_detail_stats *)vir_addr;
    if (detail_stats == TD_NULL) {
        osal_sem_up(&drv_ctx->isp_sem_buf);
        isp_err_trace("isp%d get detail stats vir addr is null\n", vi_pipe);
        return OT_ERR_ISP_NULL_PTR;
    }

    ret = isp_drv_get_detail_stats(vi_pipe, detail_stats, detail_stats_to_user);
    if (ret != TD_SUCCESS) {
        osal_sem_up(&drv_ctx->isp_sem_buf);
        isp_err_trace("pipe:%d, isp get detail stats failed! ret:0x%x\n", vi_pipe, ret);
        return ret;
    }
    ret = osal_copy_to_user(detail_stats_dest, detail_stats, sizeof(ot_isp_detail_stats));
    if (ret != 0) {
        osal_sem_up(&drv_ctx->isp_sem_buf);
        isp_err_trace("pipe:%d, isp copy detail stats to user failed, ret:0x%x!\n", vi_pipe, ret);
        return TD_FAILURE;
    }
    osal_sem_up(&drv_ctx->isp_sem_buf);
    return TD_SUCCESS;
}
#endif

static td_s32 isp_ioctl_print_alg_err_info(unsigned int cmd, void *arg, void *private_data)
{
    ot_vi_pipe vi_pipe = isp_get_pipe(private_data);
    isp_check_pipe_return(vi_pipe);
    isp_check_pointer_return(arg);
    ot_unused(cmd);
    return isp_drv_alg_init_err_info_print(vi_pipe, *(ot_isp_alg_mod *)arg);
}

static td_s32 isp_ioctl_get_be_buf_num(unsigned int cmd, void *arg, void *private_data)
{
    ot_vi_pipe vi_pipe = isp_get_pipe(private_data);
    isp_check_pipe_return(vi_pipe);
    isp_check_pointer_return(arg);
    ot_unused(cmd);

    *(td_u8 *)arg = isp_drv_get_be_buf_num(vi_pipe);
    return TD_SUCCESS;
}

static td_s32 isp_ioctl_get_be_buf_first(unsigned int cmd, void *arg, void *private_data)
{
    ot_vi_pipe vi_pipe = isp_get_pipe(private_data);
    isp_check_pipe_return(vi_pipe);
    ot_unused(cmd);

    return isp_drv_get_be_buf_first(vi_pipe, (td_phys_addr_t *)arg);
}

static td_s32 isp_ioctl_get_be_free_buf(unsigned int cmd, void *arg, void *private_data)
{
    ot_vi_pipe vi_pipe = isp_get_pipe(private_data);
    isp_check_pipe_return(vi_pipe);
    ot_unused(cmd);

    return isp_drv_query_be_free_buf(vi_pipe, (isp_be_wo_cfg_buf *)arg);
}

static td_s32 isp_ioctl_get_be_last_buf(unsigned int cmd, void *arg, void *private_data)
{
    ot_vi_pipe vi_pipe = isp_get_pipe(private_data);
    isp_check_pipe_return(vi_pipe);
    ot_unused(cmd);

    return isp_drv_get_be_last_buf(vi_pipe, (td_phys_addr_t *)arg);
}
#ifdef CONFIG_OT_VI_STITCH_GRP
static td_s32 isp_ioctl_be_buf_run_state(unsigned int cmd, void *arg, void *private_data)
{
    ot_vi_pipe vi_pipe = isp_get_pipe(private_data);
    isp_check_pipe_return(vi_pipe);
    ot_unused(cmd);
    ot_unused(arg);

    return isp_drv_be_buf_run_state(vi_pipe);
}
#endif
static td_s32 isp_ioctl_be_buf_ctl(unsigned int cmd, void *arg, void *private_data)
{
    ot_vi_pipe vi_pipe = isp_get_pipe(private_data);
    isp_check_pipe_return(vi_pipe);
    ot_unused(cmd);
    ot_unused(arg);

    return isp_drv_be_buf_ctl(vi_pipe);
}

static td_s32 isp_ioctl_trans_info_buf_exit(unsigned int cmd, void *arg, void *private_data)
{
    ot_vi_pipe vi_pipe = isp_get_pipe(private_data);
    isp_check_pipe_return(vi_pipe);
    ot_unused(cmd);
    ot_unused(arg);

    return isp_drv_trans_info_buf_exit(vi_pipe);
}

static td_s32 isp_ioctl_stat_buf_exit(unsigned int cmd, void *arg, void *private_data)
{
    ot_vi_pipe vi_pipe = isp_get_pipe(private_data);
    isp_check_pipe_return(vi_pipe);
    ot_unused(cmd);
    ot_unused(arg);

    return isp_drv_stats_buf_exit(vi_pipe);
}

static td_s32 isp_ioctl_be_buf_exit(unsigned int cmd, void *arg, void *private_data)
{
    ot_vi_pipe vi_pipe = isp_get_pipe(private_data);
    isp_check_pipe_return(vi_pipe);
    ot_unused(cmd);
    ot_unused(arg);

    return isp_drv_be_buf_exit(vi_pipe);
}

#ifdef CONFIG_OT_ISP_CLUT_SUPPORT
static td_s32 isp_ioctl_clut_buf_exit(unsigned int cmd, void *arg, void *private_data)
{
    ot_vi_pipe vi_pipe = isp_get_pipe(private_data);
    isp_check_pipe_return(vi_pipe);
    ot_unused(cmd);
    ot_unused(arg);

    return isp_drv_clut_buf_exit(vi_pipe);
}
#endif

static td_s32 isp_ioctl_drc_buf_exit(unsigned int cmd, void *arg, void *private_data)
{
    ot_vi_pipe vi_pipe = isp_get_pipe(private_data);
    isp_check_pipe_return(vi_pipe);
    ot_unused(cmd);
    ot_unused(arg);

    return isp_drv_drc_buf_exit(vi_pipe);
}

static td_s32 isp_ioctl_ldci_buf_exit(unsigned int cmd, void *arg, void *private_data)
{
    ot_vi_pipe vi_pipe = isp_get_pipe(private_data);
    isp_check_pipe_return(vi_pipe);
    ot_unused(cmd);
    ot_unused(arg);

    return isp_drv_ldci_buf_exit(vi_pipe);
}

static td_s32 isp_ioctl_stt_buf_exit(unsigned int cmd, void *arg, void *private_data)
{
    td_s32 ret;
    isp_drv_ctx *drv_ctx = TD_NULL;
    ot_vi_pipe vi_pipe = isp_get_pipe(private_data);
    isp_check_pipe_return(vi_pipe);
    ot_unused(cmd);
    ot_unused(arg);
    drv_ctx = isp_drv_get_ctx(vi_pipe);
    if (osal_sem_down(&drv_ctx->isp_sem_buf)) {
        return -ERESTARTSYS;
    }
    ret = isp_drv_stt_buf_exit(vi_pipe);
    osal_sem_up(&drv_ctx->isp_sem_buf);
    return ret;
}

#ifdef CONFIG_OT_SNAP_SUPPORT
static td_s32 isp_ioctl_get_pro_trigger(unsigned int cmd, void *arg, void *private_data)
{
    ot_vi_pipe vi_pipe = isp_get_pipe(private_data);
    isp_drv_ctx *drv_ctx = TD_NULL;

    isp_check_pipe_return(vi_pipe);
    isp_check_pointer_return(arg);
    ot_unused(cmd);

    drv_ctx = isp_drv_get_ctx(vi_pipe);
    *(td_bool *)arg = drv_ctx->pro_enable;

    if (drv_ctx->pro_enable == TD_TRUE) {
        drv_ctx->pro_enable = TD_FALSE;
        drv_ctx->pro_trig_flag = 1;
    }

    return TD_SUCCESS;
}

static td_s32 isp_ioctl_get_snap_attr(unsigned int cmd, void *arg, void *private_data)
{
    ot_vi_pipe vi_pipe = isp_get_pipe(private_data);
    isp_drv_ctx *drv_ctx = TD_NULL;
    isp_snap_attr *snap_attr = TD_NULL;

    isp_check_pipe_return(vi_pipe);
    isp_check_pointer_return(arg);
    ot_unused(cmd);

    snap_attr = (isp_snap_attr *)arg;
    drv_ctx = isp_drv_get_ctx(vi_pipe);
    (td_void)memcpy_s(snap_attr, sizeof(isp_snap_attr), &drv_ctx->snap_attr, sizeof(isp_snap_attr));

    return TD_SUCCESS;
}

static td_s32 isp_ioctl_set_config_info(unsigned int cmd, void *arg, void *private_data)
{
    ot_vi_pipe vi_pipe = isp_get_pipe(private_data);
    ot_isp_config_info *isp_config_info = TD_NULL;

    isp_check_pipe_return(vi_pipe);
    ot_unused(cmd);

    isp_config_info = (ot_isp_config_info *)arg;

    return isp_drv_set_config_info(vi_pipe, isp_config_info);
}

static td_s32 isp_ioctl_set_snap_attr(unsigned int cmd, void *arg, void *private_data)
{
    ot_vi_pipe vi_pipe = isp_get_pipe(private_data);

    isp_check_pipe_return(vi_pipe);
    ot_unused(cmd);

    return isp_set_snap_attr(vi_pipe, (isp_snap_attr*)arg);
}

static td_s32 isp_ioctl_set_procalcdone(unsigned int cmd, void *arg, void *private_data)
{
    ot_vi_pipe vi_pipe = isp_get_pipe(private_data);
    isp_drv_ctx *drv_ctx = TD_NULL;
    isp_check_pipe_return(vi_pipe);
    ot_unused(cmd);
    ot_unused(arg);
    drv_ctx = isp_drv_get_ctx(vi_pipe);
    drv_ctx->pro_start = TD_TRUE;

    return TD_SUCCESS;
}
#endif

#ifdef CONFIG_OT_ISP_PQ_FOR_AI_SUPPORT
static td_s32 isp_ioctl_set_pq_ai_attr(unsigned int cmd, void *arg, void *private_data)
{
    ot_vi_pipe vi_pipe = isp_get_pipe(private_data);

    isp_check_pipe_return(vi_pipe);
    ot_unused(cmd);

    return isp_set_pq_ai_attr((pq_ai_attr *)arg);
}

static td_s32 isp_ioctl_get_pq_ai_attr(unsigned int cmd, void *arg, void *private_data)
{
    ot_vi_pipe vi_pipe = isp_get_pipe(private_data);

    isp_check_pipe_return(vi_pipe);
    ot_unused(cmd);

    return isp_get_pq_ai_attr(vi_pipe, (pq_ai_attr *)arg);
}

static td_s32 isp_ioctl_set_pq_ai_post_nr_attr(unsigned int cmd, void *arg, void *private_data)
{
    ot_vi_pipe vi_pipe = isp_get_pipe(private_data);

    isp_check_pipe_return(vi_pipe);
    ot_unused(cmd);

    return isp_set_pq_ai_post_nr_attr(vi_pipe, (ot_pq_ai_noiseness_post_attr *)arg);
}

static td_s32 isp_ioctl_get_pq_ai_post_nr_attr(unsigned int cmd, void *arg, void *private_data)
{
    ot_vi_pipe vi_pipe = isp_get_pipe(private_data);

    isp_check_pipe_return(vi_pipe);
    isp_check_pointer_return(arg);
    ot_unused(cmd);

    return isp_get_pq_ai_post_nr_attr(vi_pipe, (ot_pq_ai_noiseness_post_attr *)arg);
}
#endif

#ifdef CONFIG_OT_ISP_RGBIR_SUPPORT
static td_s32 isp_ioctl_set_rgbir_format(unsigned int cmd, void *arg, void *private_data)
{
    ot_vi_pipe vi_pipe = isp_get_pipe(private_data);
    isp_drv_ctx *drv_ctx = TD_NULL;
    isp_check_pipe_return(vi_pipe);
    isp_check_pointer_return(arg);
    ot_unused(cmd);
    drv_ctx = isp_drv_get_ctx(vi_pipe);
    drv_ctx->isp_rgbir_format = *(td_u32 *)arg;
    return TD_SUCCESS;
}
#endif

static td_s32 isp_ioctl_get_update_pos(unsigned int cmd, void *arg, void *private_data)
{
    ot_vi_pipe vi_pipe = isp_get_pipe(private_data);

    isp_check_pipe_return(vi_pipe);
    isp_check_pointer_return(arg);
    ot_unused(cmd);

    *(td_u32 *)arg = isp_drv_get_update_pos(vi_pipe);

    return TD_SUCCESS;
}

static td_s32 isp_ioctl_get_frm_cnt(unsigned int cmd, void *arg, void *private_data)
{
    ot_vi_pipe vi_pipe = isp_get_pipe(private_data);
    isp_drv_ctx *drv_ctx = TD_NULL;

    isp_check_pipe_return(vi_pipe);
    isp_check_pointer_return(arg);
    ot_unused(cmd);

    drv_ctx = isp_drv_get_ctx(vi_pipe);
    *(td_u32 *)arg = drv_ctx->frame_cnt;

    return TD_SUCCESS;
}

static td_s32 isp_ioctl_set_pub_attr(unsigned int cmd, void *arg, void *private_data)
{
    ot_vi_pipe vi_pipe = isp_get_pipe(private_data);

    isp_check_pipe_return(vi_pipe);
    ot_unused(cmd);

    return isp_set_pub_attr_info(vi_pipe, (ot_isp_pub_attr *)arg);
}

static td_s32 isp_ioctl_reset_ctx(unsigned int cmd, void *arg, void *private_data)
{
    td_s32 ret;
    isp_drv_ctx *drv_ctx = TD_NULL;
    ot_vi_pipe vi_pipe = isp_get_pipe(private_data);

    isp_check_pipe_return(vi_pipe);
    ot_unused(cmd);
    ret = isp_drv_reset_ctx(vi_pipe);
    if (ret != TD_SUCCESS) {
        return ret;
    }
    drv_ctx = isp_drv_get_ctx(vi_pipe);
    if (osal_sem_down(&drv_ctx->isp_sem_buf)) {
        return -ERESTARTSYS;
    }
    isp_dfx_ctx_deinit(vi_pipe);
    osal_sem_up(&drv_ctx->isp_sem_buf);
    return ret;
}
#ifdef CONFIG_OT_ISP_DYNAMIC_BLC_SUPPORT
static td_s32 isp_ioctl_get_dynamic_actual_info(unsigned int cmd, void *arg, void *private_data)
{
    ot_vi_pipe vi_pipe = isp_get_pipe(private_data);

    isp_check_pipe_return(vi_pipe);
    ot_unused(cmd);

    return isp_drv_get_dynamic_actual_info(vi_pipe, (isp_blc_actual_info *)arg);
}
#endif
static td_s32 isp_ioctl_set_fpn_work_mode(unsigned int cmd, void *arg, void *private_data)
{
    ot_vi_pipe vi_pipe = isp_get_pipe(private_data);

    isp_check_pipe_return(vi_pipe);
    isp_check_pointer_return(arg);
    ot_unused(cmd);

    return isp_drv_set_fpn_work_mode_set(vi_pipe, (td_u8 *)arg);
}
#ifdef CONFIG_OT_ISP_DYNAMIC_BLC_SUPPORT
static td_s32 isp_ioctl_get_bas_crop_attr(unsigned int cmd, void *arg, void *private_data)
{
    ot_vi_pipe vi_pipe = isp_get_pipe(private_data);

    isp_check_pipe_return(vi_pipe);
    ot_unused(cmd);

    return isp_get_bas_crop_attr(vi_pipe, (vi_blc_crop_info *)arg);
}
#endif
static td_s32 isp_ioctl_get_stagger_attr(unsigned int cmd, void *arg, void *private_data)
{
    ot_vi_pipe vi_pipe = isp_get_pipe(private_data);

    isp_check_pipe_return(vi_pipe);
    ot_unused(cmd);

    return isp_get_stagger_attr(vi_pipe, (isp_stagger_attr *)arg);
}

static td_s32 isp_ioctl_init_work_mode(unsigned int cmd, void *arg, void *private_data)
{
    ot_vi_pipe vi_pipe = isp_get_pipe(private_data);

    isp_check_pipe_return(vi_pipe);
    ot_unused(cmd);

    return isp_drv_work_mode_init(vi_pipe, (isp_block_attr *)arg);
}

static td_s32 isp_ioctl_work_mode_exit(unsigned int cmd, void *arg, void *private_data)
{
    ot_vi_pipe vi_pipe = isp_get_pipe(private_data);

    isp_check_pipe_return(vi_pipe);
    ot_unused(cmd);
    ot_unused(arg);

    return isp_drv_work_mode_exit(vi_pipe);
}

static td_s32 isp_ioctl_get_work_mode(unsigned int cmd, void *arg, void *private_data)
{
    ot_vi_pipe vi_pipe = isp_get_pipe(private_data);
    isp_drv_ctx *drv_ctx = TD_NULL;
    isp_working_mode *isp_work_mode = TD_NULL;

    isp_check_pipe_return(vi_pipe);
    isp_check_pointer_return(arg);
    ot_unused(cmd);
    isp_work_mode = (isp_working_mode *)arg;
    drv_ctx = isp_drv_get_ctx(vi_pipe);
    (td_void)memcpy_s(isp_work_mode, sizeof(isp_working_mode), &drv_ctx->work_mode, sizeof(isp_working_mode));

    return TD_SUCCESS;
}

static td_s32 isp_ioctl_get_stat_buf(unsigned int cmd, void *arg, void *private_data)
{
    ot_vi_pipe vi_pipe = isp_get_pipe(private_data);

    isp_check_pipe_return(vi_pipe);
    ot_unused(cmd);
    return isp_drv_stats_buf_user_get(vi_pipe, (isp_stat_info *)arg);
}

static td_s32 isp_ioctl_put_stat_buf(unsigned int cmd, void *arg, void *private_data)
{
    ot_vi_pipe vi_pipe = isp_get_pipe(private_data);

    isp_check_pipe_return(vi_pipe);
    ot_unused(cmd);
    return isp_drv_stats_buf_user_put(vi_pipe, (isp_stat_info *)arg);
}

static td_s32 isp_ioctl_get_stat_act(unsigned int cmd, void *arg, void *private_data)
{
    ot_vi_pipe vi_pipe = isp_get_pipe(private_data);

    isp_check_pipe_return(vi_pipe);
    ot_unused(cmd);
    return isp_drv_stats_query_active(vi_pipe, (isp_stat_info *)arg);
}

static td_s32 isp_ioctl_set_update_info(unsigned int cmd, void *arg, void *private_data)
{
    ot_vi_pipe vi_pipe = isp_get_pipe(private_data);
    isp_drv_ctx *drv_ctx = TD_NULL;
    ot_isp_dcf_update_info *isp_update_info = TD_NULL;

    isp_check_pipe_return(vi_pipe);
    isp_check_pointer_return(arg);
    ot_unused(cmd);
    drv_ctx = isp_drv_get_ctx(vi_pipe);
    isp_update_info = (ot_isp_dcf_update_info *)arg;
    (td_void)memcpy_s(&drv_ctx->update_info, sizeof(ot_isp_dcf_update_info),
                      isp_update_info, sizeof(ot_isp_dcf_update_info));
    return TD_SUCCESS;
}

static td_s32 isp_ioctl_set_frame_info(unsigned int cmd, void *arg, void *private_data)
{
    ot_vi_pipe vi_pipe = isp_get_pipe(private_data);
    isp_drv_ctx *drv_ctx = TD_NULL;
    ot_isp_frame_info *isp_frame_info = TD_NULL;

    isp_check_pipe_return(vi_pipe);
    isp_check_pointer_return(arg);
    ot_unused(cmd);

    drv_ctx = isp_drv_get_ctx(vi_pipe);
    isp_frame_info = (ot_isp_frame_info *)arg;
    (td_void)memcpy_s(&drv_ctx->frame_info, sizeof(ot_isp_frame_info),
                      isp_frame_info, sizeof(ot_isp_frame_info));
    return TD_SUCCESS;
}

static td_s32 isp_ioctl_get_frame_info(unsigned int cmd, void *arg, void *private_data)
{
    ot_vi_pipe vi_pipe = isp_get_pipe(private_data);
    ot_isp_frame_info *isp_frame_info = TD_NULL;

    isp_check_pipe_return(vi_pipe);
    ot_unused(cmd);

    isp_frame_info = (ot_isp_frame_info *)arg;
    return isp_get_frame_info(vi_pipe, isp_frame_info);
}

static td_s32 isp_ioctl_set_dng_info(unsigned int cmd, void *arg, void *private_data)
{
    ot_vi_pipe vi_pipe = isp_get_pipe(private_data);

    isp_check_pipe_return(vi_pipe);
    ot_unused(cmd);

    return isp_drv_set_dng_info(vi_pipe, (ot_dng_image_dynamic_info *)arg);
}

static td_s32 isp_ioctl_get_dng_info(unsigned int cmd, void *arg, void *private_data)
{
    ot_vi_pipe vi_pipe = isp_get_pipe(private_data);

    isp_check_pipe_return(vi_pipe);
    ot_unused(cmd);

    return isp_drv_get_dng_info(vi_pipe, (ot_isp_dng_image_static_info *)arg);
}

static td_s32 isp_ioctl_get_frame_edge(unsigned int cmd, void *arg, void *private_data)
{
    ot_vi_pipe vi_pipe = isp_get_pipe(private_data);

    isp_check_pipe_return(vi_pipe);
    isp_check_pointer_return(arg);
    ot_unused(cmd);
    return isp_get_frame_edge(vi_pipe, (isp_frame_edge_info *)arg);
}

static td_s32 isp_ioctl_get_vd_timeout(unsigned int cmd, void *arg, void *private_data)
{
    ot_vi_pipe vi_pipe = isp_get_pipe(private_data);
    isp_vd_timeout isp_vd_time_out;
    isp_vd_timeout *vd_time_out = TD_NULL;
    td_s32 ret;

    isp_check_pipe_return(vi_pipe);
    isp_check_pointer_return(arg);
    ot_unused(cmd);

    vd_time_out = (isp_vd_timeout *)arg;
    (td_void)memcpy_s(&isp_vd_time_out, sizeof(isp_vd_timeout), vd_time_out, sizeof(isp_vd_timeout));
    ret = isp_get_vd_start_time_out(vi_pipe, isp_vd_time_out.milli_sec, &isp_vd_time_out.int_status);
    (td_void)memcpy_s(vd_time_out, sizeof(isp_vd_timeout), &isp_vd_time_out, sizeof(isp_vd_timeout));

    return ret;
}

static td_s32 isp_ioctl_get_vd_end_timeout(unsigned int cmd, void *arg, void *private_data)
{
    ot_vi_pipe vi_pipe = isp_get_pipe(private_data);
    isp_vd_timeout isp_vd_time_out;
    isp_vd_timeout *vd_time_out = TD_NULL;
    td_s32 ret;

    isp_check_pipe_return(vi_pipe);
    isp_check_pointer_return(arg);
    ot_unused(cmd);

    vd_time_out = (isp_vd_timeout *)arg;
    (td_void)memcpy_s(&isp_vd_time_out, sizeof(isp_vd_timeout), vd_time_out, sizeof(isp_vd_timeout));
    ret = isp_get_vd_end_time_out(vi_pipe, isp_vd_time_out.milli_sec, &isp_vd_time_out.int_status);
    (td_void)memcpy_s(vd_time_out, sizeof(isp_vd_timeout), &isp_vd_time_out, sizeof(isp_vd_timeout));

    return ret;
}

static td_s32 isp_ioctl_get_be_end_timeout(unsigned int cmd, void *arg, void *private_data)
{
    ot_vi_pipe vi_pipe = isp_get_pipe(private_data);
    isp_vd_timeout isp_vd_time_out;
    isp_vd_timeout *vd_time_out = TD_NULL;
    td_s32 ret;

    isp_check_pipe_return(vi_pipe);
    isp_check_pointer_return(arg);
    ot_unused(cmd);

    vd_time_out = (isp_vd_timeout *)arg;
    (td_void)memcpy_s(&isp_vd_time_out, sizeof(isp_vd_timeout), vd_time_out, sizeof(isp_vd_timeout));
    ret = isp_get_vd_be_end_time_out(vi_pipe, isp_vd_time_out.milli_sec, &isp_vd_time_out.int_status);
    (td_void)memcpy_s(vd_time_out, sizeof(isp_vd_timeout), &isp_vd_time_out, sizeof(isp_vd_timeout));

    return ret;
}
#ifdef CONFIG_OT_SYS_KOL_SUPPORT
static td_s32 isp_ioctl_get_run_end_timeout(unsigned int cmd, void *arg, void *private_data)
{
    td_s32 ret;
    ot_vi_pipe vi_pipe = isp_get_pipe(private_data);
    isp_vd_timeout isp_vd_time_out;
    isp_vd_timeout *vd_time_out = TD_NULL;

    isp_check_pipe_return(vi_pipe);
    isp_check_pointer_return(arg);
    ot_unused(cmd);

    vd_time_out = (isp_vd_timeout *)arg;
    (td_void)memcpy_s(&isp_vd_time_out, sizeof(isp_vd_timeout), vd_time_out, sizeof(isp_vd_timeout));
    ret = isp_get_vd_run_end_time_out(vi_pipe, isp_vd_time_out.milli_sec, &isp_vd_time_out.int_status);
    (td_void)memcpy_s(vd_time_out, sizeof(isp_vd_timeout), &isp_vd_time_out, sizeof(isp_vd_timeout));

    return ret;
}

static td_s32 isp_ioctl_get_vd_standby_end_timeout(unsigned int cmd, void *arg, void *private_data)
{
    ot_vi_pipe vi_pipe = isp_get_pipe(private_data);
    isp_vd_timeout isp_vd_time_out;
    isp_vd_timeout *vd_time_out = TD_NULL;
    td_s32 ret;

    isp_check_pipe_return(vi_pipe);
    isp_check_pointer_return(arg);
    ot_unused(cmd);

    vd_time_out = (isp_vd_timeout *)arg;
    (td_void)memcpy_s(&isp_vd_time_out, sizeof(isp_vd_timeout), vd_time_out, sizeof(isp_vd_timeout));
    ret = isp_get_vd_standby_end_time_out(vi_pipe, isp_vd_time_out.milli_sec, &isp_vd_time_out.int_status);
    (td_void)memcpy_s(vd_time_out, sizeof(isp_vd_timeout), &isp_vd_time_out, sizeof(isp_vd_timeout));

    return ret;
}

#endif
static td_s32 isp_ioctl_init_proc(unsigned int cmd, void *arg, void *private_data)
{
    ot_vi_pipe vi_pipe = isp_get_pipe(private_data);

    isp_check_pipe_return(vi_pipe);
    ot_unused(cmd);

    return isp_drv_proc_init(vi_pipe, (isp_proc_mem *)arg);
}

static td_s32 isp_ioctl_writing_proc(unsigned int cmd, void *arg, void *private_data)
{
    ot_vi_pipe vi_pipe = isp_get_pipe(private_data);
    isp_drv_ctx *drv_ctx = TD_NULL;
    isp_check_pipe_return(vi_pipe);
    ot_unused(cmd);
    ot_unused(arg);
    drv_ctx = isp_drv_get_ctx(vi_pipe);
    if (osal_sem_down_interruptible(&drv_ctx->proc_sem)) {
        return -ERESTARTSYS;
    }
    drv_ctx->proc_updating = TD_TRUE;

    osal_sem_up(&drv_ctx->proc_sem);
    return TD_SUCCESS;
}

static td_s32 isp_ioctl_write_proc_ok(unsigned int cmd, void *arg, void *private_data)
{
    ot_vi_pipe vi_pipe = isp_get_pipe(private_data);
    isp_drv_ctx *drv_ctx = TD_NULL;
    isp_check_pipe_return(vi_pipe);
    ot_unused(cmd);
    ot_unused(arg);
    drv_ctx = isp_drv_get_ctx(vi_pipe);
    if (osal_sem_down_interruptible(&drv_ctx->proc_sem)) {
        return -ERESTARTSYS;
    }
    drv_ctx->proc_updating = TD_FALSE;

    osal_sem_up(&drv_ctx->proc_sem);
    return TD_SUCCESS;
}

static td_s32 isp_ioctl_proc_exit(unsigned int cmd, void *arg, void *private_data)
{
    ot_vi_pipe vi_pipe = isp_get_pipe(private_data);

    isp_check_pipe_return(vi_pipe);
    ot_unused(cmd);
    ot_unused(arg);

    return isp_drv_proc_exit(vi_pipe);
}

static td_s32 isp_ioctl_get_proc_param(unsigned int cmd, void *arg, void *private_data)
{
    ot_vi_pipe vi_pipe = isp_get_pipe(private_data);

    isp_check_pipe_return(vi_pipe);
    isp_check_pointer_return(arg);
    ot_unused(cmd);

    *(td_u32 *)arg = isp_drv_get_proc_param(vi_pipe);

    return TD_SUCCESS;
}

static td_s32 isp_ioctl_mem_share_pid(unsigned int cmd, void *arg, void *private_data)
{
    ot_vi_pipe vi_pipe = isp_get_pipe(private_data);
    td_s32 pid;

    isp_check_pipe_return(vi_pipe);
    isp_check_pointer_return(arg);
    ot_unused(cmd);
    pid = *(td_s32 *)arg;
    return isp_drv_mem_share_pid(vi_pipe, pid);
}

static td_s32 isp_ioctl_mem_unshare_pid(unsigned int cmd, void *arg, void *private_data)
{
    ot_vi_pipe vi_pipe = isp_get_pipe(private_data);
    td_s32 pid;

    isp_check_pipe_return(vi_pipe);
    isp_check_pointer_return(arg);
    ot_unused(cmd);
    pid = *(td_s32 *)arg;
    return isp_drv_mem_unshare_pid(vi_pipe, pid);
}

static td_s32 isp_ioctl_mem_share_all(unsigned int cmd, void *arg, void *private_data)
{
    ot_vi_pipe vi_pipe = isp_get_pipe(private_data);

    isp_check_pipe_return(vi_pipe);
    ot_unused(cmd);
    ot_unused(arg);
    return isp_drv_mem_share_all(vi_pipe);
}

static td_s32 isp_ioctl_mem_unshare_all(unsigned int cmd, void *arg, void *private_data)
{
    ot_vi_pipe vi_pipe = isp_get_pipe(private_data);

    isp_check_pipe_return(vi_pipe);
    ot_unused(cmd);
    ot_unused(arg);
    return isp_drv_mem_unshare_all(vi_pipe);
}

static td_s32 isp_ioctl_mem_verify_pid(unsigned int cmd, void *arg, void *private_data)
{
    ot_vi_pipe vi_pipe = isp_get_pipe(private_data);
    td_s32 pid, ret;

    isp_check_pipe_return(vi_pipe);
    ot_unused(cmd);
    ot_unused(arg);
    pid = osal_get_current_tgid();
    ret = isp_drv_mem_verify_pid(vi_pipe, pid);
    if (ret != TD_SUCCESS) {
        isp_err_trace("pipe %d pid%d verify pid fail \n", vi_pipe, pid);
    }

    return ret;
}

static osal_ioctl_cmd g_isp_ioctl_cmd_list[] = {
    { ISP_DEV_SET_FD, isp_ioctl_set_dev_fd },
    { VREG_DRV_FD, isp_ioctl_set_vreg_fd },
    { VREG_DRV_GETADDR, isp_ioctl_get_vreg_addr },
    { VREG_DRV_CHECK_PERMISSION, isp_ioctl_check_permission},
    { ISP_MEM_INFO_GET,  isp_ioctl_get_mem_info },
    { ISP_GET_FRAME_EDGE, isp_ioctl_get_frame_edge },
    { ISP_GET_VD_TIMEOUT, isp_ioctl_get_vd_timeout },
    { ISP_GET_VD_END_TIMEOUT, isp_ioctl_get_vd_end_timeout },
    { ISP_GET_VD_BEEND_TIMEOUT, isp_ioctl_get_be_end_timeout },
#ifdef CONFIG_OT_SYS_KOL_SUPPORT
    { ISP_GET_VD_RUNEND_TIMEOUT, isp_ioctl_get_run_end_timeout },
    { ISP_GET_VD_STANDBY_END_TIMEOUT, isp_ioctl_get_vd_standby_end_timeout },
#endif
#ifdef CONFIG_OT_ISP_MCF_SUPPORT
    { ISP_MCF_EN_GET, isp_ioctl_get_mcf_en },
#endif
    { ISP_ALG_INIT_ERR_INFO_PRINT, isp_ioctl_print_alg_err_info },
    { ISP_BE_FREE_BUF_GET, isp_ioctl_get_be_free_buf },
    { ISP_BE_CFG_BUF_CTL, isp_ioctl_be_buf_ctl },
#ifdef CONFIG_OT_VI_STITCH_GRP
    { ISP_BE_CFG_BUF_RUNNING, isp_ioctl_be_buf_run_state },
#endif
    { ISP_STAT_BUF_GET, isp_ioctl_get_stat_buf },
    { ISP_STAT_ACT_GET, isp_ioctl_get_stat_act },
    { ISP_STAT_BUF_PUT, isp_ioctl_put_stat_buf },
    { ISP_REG_CFG_SET, isp_ioctl_set_reg_cfg },
    { ISP_UPDATE_POS_GET, isp_ioctl_get_update_pos },
#ifdef CONFIG_OT_ISP_DYNAMIC_BLC_SUPPORT
    { ISP_DYNAMIC_ACTUAL_INFO_GET, isp_ioctl_get_dynamic_actual_info },
#endif
    { ISP_BNR_TEMPORAL_FILT_CFG_SET, isp_ioctl_set_bnr_temporal_filt },
#ifdef IQ_DEBUG
    { ISP_BNR_MOT_BITW_SET, isp_ioctl_set_bnr_mot_bitw },
#endif
    { ISP_DISABLE_BNR_WMOT, isp_ioctl_disable_bnr_wmot},
    { ISP_DNG_INFO_SET, isp_ioctl_set_dng_info },
    { ISP_WDR_CFG_SET, isp_ioctl_set_wdr_cfg },
    { ISP_FRAME_CNT_GET, isp_ioctl_get_frm_cnt },
    { ISP_PROC_PARAM_GET, isp_ioctl_get_proc_param },
    { ISP_PROC_WRITE_ING, isp_ioctl_writing_proc },
    { ISP_PROC_WRITE_OK, isp_ioctl_write_proc_ok },
    { ISP_SYNC_CFG_SET, isp_ioctl_set_sync_cfg },
    { ISP_OPT_RUNONCE_INFO, isp_ioctl_opt_run_once_info },
    { ISP_YUV_RUNONCE_INFO, isp_ioctl_yuv_run_once_info },
    { ISP_KERNEL_RUNONCE, isp_ioctl_run_once_process },
    { ISP_KERNEL_YUV_RUNONCE, isp_ioctl_yuv_run_once_process },
    { ISP_KERNEL_RUN_TRIGGER, isp_ioctl_run_trigger_process },

    { ISP_WORK_MODE_GET, isp_ioctl_get_work_mode },
    { ISP_GET_WDR_ATTR, isp_ioctl_get_wdr_attr },
#ifdef CONFIG_OT_VI_DISTRIBUTE_GRP
    { ISP_GET_DIST_ATTR, isp_ioctl_get_dist_attr },
    { ISP_GET_DIST_FPS_STATUS, isp_ioctl_get_dist_fps_status },
#endif
#ifdef CONFIG_OT_ISP_CLUT_SUPPORT
    { ISP_CLUT_BUF_GET, isp_ioctl_get_clut_buf },
#endif
#ifdef CONFIG_OT_VI_STITCH_GRP
    { ISP_GET_STITCH_ATTR, isp_ioctl_get_stitch_attr },
#endif
    { ISP_MOT_CFG_SET, isp_ioctl_set_mot_cfg },
    { ISP_PUB_ATTR_INFO, isp_ioctl_set_pub_attr },
    { ISP_CHN_SELECT_CFG, isp_ioctl_chn_select_cfg },
    { ISP_PWM_NUM_GET, isp_ioctl_get_pwm_num },

#ifdef CONFIG_OT_SNAP_SUPPORT
    { ISP_PRO_TRIGGER_GET, isp_ioctl_get_pro_trigger },
    { ISP_SNAP_ATTR_GET, isp_ioctl_get_snap_attr },
    { ISP_CONFIG_INFO_SET, isp_ioctl_set_config_info },
    { ISP_SNAP_ATTR_SET, isp_ioctl_set_snap_attr },
    { ISP_SET_PROCALCDONE, isp_ioctl_set_procalcdone },
#endif
    { ISP_SET_CTRL_PARAM, isp_ioctl_set_ctrl_param },
    { ISP_GET_CTRL_PARAM, isp_ioctl_get_ctrl_param },
#ifdef CONFIG_OT_ISP_RGBIR_SUPPORT
    { ISP_SET_RGBIR_FORMAT, isp_ioctl_set_rgbir_format },
#endif
    { ISP_DNG_INFO_GET, isp_ioctl_get_dng_info },
    { ISP_UPDATE_INFO_SET, isp_ioctl_set_update_info },
    { ISP_FRAME_INFO_SET, isp_ioctl_set_frame_info },
    { ISP_FRAME_INFO_GET, isp_ioctl_get_frame_info },
    { ISP_FPN_WORK_MODE_SET, isp_ioctl_set_fpn_work_mode },
    { ISP_SET_MOD_PARAM, isp_ioctl_set_mod_param },
    { ISP_GET_MOD_PARAM, isp_ioctl_get_mod_param },
#ifdef CONFIG_OT_ISP_DETAIL_STATS_SUPPORT
    { ISP_SET_DETAIL_STATS_CFG, isp_ioctl_set_detail_stats_cfg },
    { ISP_GET_DETAIL_STATS_CFG, isp_ioctl_get_detail_stats_cfg },
    { ISP_GET_DETAIL_STATS, isp_ioctl_get_detail_stats },
    { ISP_GET_DETAIL_BLK_SIZE, isp_ioctl_get_detail_blk_size },
#endif
    /* init & switch */
    { ISP_GET_HDR_ATTR, isp_ioctl_get_hdr_attr },
    { ISP_GET_PIPE_SIZE, isp_ioctl_get_pipe_size },
    { ISP_P2EN_INFO_GET, isp_ioctl_get_p2_en_info },
    { ISP_STT_ADDR_INIT, isp_ioctl_init_stt_addr },
    { ISP_WORK_MODE_INIT, isp_ioctl_init_work_mode },
    { ISP_SYNC_INIT_SET, isp_ioctl_init_stt_sync },
#ifdef CONFIG_OT_VI_STITCH_GRP
    { ISP_SYNC_STITCH_PARAM_INIT, isp_ioctl_init_stitch_sync_param },
#endif
    { ISP_RES_SWITCH_SET, isp_ioctl_set_res_switch },
    { ISP_BE_LAST_BUF_GET, isp_ioctl_get_be_last_buf },
    { ISP_BE_SYNC_PARAM_INIT, isp_ioctl_init_be_sync_param },
    { ISP_BE_SWITCH_FINISH_STATE_SET, isp_ioctl_set_be_buf_switch_finish },
#ifdef CONFIG_OT_ISP_DYNAMIC_BLC_SUPPORT
    { ISP_GET_BAS_CROP_ATTR, isp_ioctl_get_bas_crop_attr },
#endif

    /* mem init */
    { VREG_DRV_INIT, isp_ioctl_init_vreg },
    { ISP_PRE_BLK_NUM_UPDATE, isp_ioctl_update_pre_blk_num },
    { ISP_STAGGER_ATTR_GET, isp_ioctl_get_stagger_attr },
    { ISP_BE_BUF_NUM_GET, isp_ioctl_get_be_buf_num },
    { ISP_GET_VERSION, isp_ioctl_get_version },
    { ISP_MEM_INFO_SET,   isp_ioctl_set_mem_info },
    /* init */
    { ISP_LDCI_BUF_INIT, isp_ioctl_init_ldci_buf },
    { ISP_LDCI_READ_STT_BUF_GET, isp_ioctl_get_ldci_read_stt_buf },
    { ISP_DRC_BUF_INIT, isp_ioctl_init_drc_buf },
    { ISP_BE_CFG_BUF_INIT, isp_ioctl_init_be_cfg_buf },
    { ISP_GET_BE_BUF_FIRST, isp_ioctl_get_be_buf_first },
    { ISP_STT_BUF_INIT, isp_ioctl_init_stt_buf },
#ifdef CONFIG_OT_SYS_KOL_SUPPORT
    { ISP_STANDBY_CFG_INIT, isp_ioctl_init_standby_cfg },
#endif
#ifdef CONFIG_OT_VI_PREONLINE_POSTOFFLINE
    { ISP_BE_PRE_LUT_STT_BUF_GET, isp_ioctl_get_be_pre_lut_stt_buf },
#endif
    { ISP_BE_LUT_STT_BUF_GET, isp_ioctl_get_be_lut_stt_buf },
#ifdef CONFIG_OT_ISP_LBLC_SUPPORT
    { ISP_FE_LUT_STT_BUF_GET, isp_drv_fe_lut_stt_buf_get },
#endif
#ifdef CONFIG_OT_ISP_CLUT_SUPPORT
    { ISP_CLUT_BUF_INIT, isp_ioctl_init_clut_buf },
#endif
    { ISP_STAT_BUF_INIT, isp_ioctl_init_stat_buf },
    { ISP_PROC_INIT, isp_ioctl_init_proc },
    { ISP_TRANS_BUF_INIT, isp_ioctl_init_trans_info },
    { ISP_BE_ALL_BUF_INIT, isp_ioctl_init_be_all_buf },
    { ISP_INIT_INFO_SET, isp_ioctl_set_init_info },
    { ISP_INIT_INFO_GET, isp_ioctl_get_init_info },
    { ISP_RUN_STATE_SET, isp_ioctl_set_run_state },
    { ISP_SET_INT_ENABLE, isp_ioctl_set_int_enable },
#ifdef CONFIG_OT_ISP_DETAIL_STATS_SUPPORT
    { ISP_DETAIL_STATS_BUF_INIT, isp_ioctl_init_detail_stats_buf },
#endif
    /* exit */
    { ISP_TRANS_BUF_EXIT, isp_ioctl_trans_info_buf_exit },
    { ISP_PROC_EXIT, isp_ioctl_proc_exit },
    { ISP_STAT_BUF_EXIT, isp_ioctl_stat_buf_exit },
#ifdef CONFIG_OT_ISP_CLUT_SUPPORT
    { ISP_CLUT_BUF_EXIT, isp_ioctl_clut_buf_exit },
#endif
    { ISP_BE_CFG_BUF_EXIT, isp_ioctl_be_buf_exit },
    { ISP_STT_BUF_EXIT, isp_ioctl_stt_buf_exit },
    { ISP_DRC_BUF_EXIT, isp_ioctl_drc_buf_exit },
    { ISP_LDCI_BUF_EXIT, isp_ioctl_ldci_buf_exit },
    { ISP_WORK_MODE_EXIT, isp_ioctl_work_mode_exit },
    { ISP_RESET_CTX, isp_ioctl_reset_ctx },
    { VREG_DRV_EXIT, isp_ioctl_vreg_exit },
    { VREG_DRV_RELEASE_ALL, isp_ioctl_release_vreg },
#ifdef CONFIG_OT_ISP_DETAIL_STATS_SUPPORT
    { ISP_DETAIL_STATS_BUF_EXIT, isp_ioctl_detail_stats_buf_exit },
#endif
    /* mem share */
    { ISP_SHARE_PID, isp_ioctl_mem_share_pid },
    { ISP_UNSHARE_PID, isp_ioctl_mem_unshare_pid },
    { ISP_SHARE_ALL, isp_ioctl_mem_share_all },
    { ISP_UNSHARE_ALL, isp_ioctl_mem_unshare_all },
    { ISP_VERIFY_PID, isp_ioctl_mem_verify_pid },
#ifdef CONFIG_OT_ISP_PQ_FOR_AI_SUPPORT
    { ISP_PQ_AI_GROUP_ATTR_SET, isp_ioctl_set_pq_ai_attr },
    { ISP_PQ_AI_GROUP_ATTR_GET, isp_ioctl_get_pq_ai_attr },
    { ISP_PQ_AI_POST_NR_ATTR_SET, isp_ioctl_set_pq_ai_post_nr_attr },
    { ISP_PQ_AI_POST_NR_ATTR_GET, isp_ioctl_get_pq_ai_post_nr_attr },
#endif
};

td_void isp_set_ioctl_cmd_list(osal_fileops *isp_fops)
{
    isp_fops->cmd_list = g_isp_ioctl_cmd_list;
    isp_fops->cmd_cnt = sizeof(g_isp_ioctl_cmd_list) / sizeof(g_isp_ioctl_cmd_list[0]);
}

td_s32 isp_init_ioctl_info_check(td_void)
{
    td_s32 i;
    td_u32 cmd_nr;
    td_s32 cmd_num;

    cmd_num = sizeof(g_isp_ioctl_cmd_list) / sizeof(g_isp_ioctl_cmd_list[0]);
    if (cmd_num >= IOC_NR_ISP_BUTT) {
        isp_err_trace("Ioctl cmd num is too large!\n");
        return TD_FAILURE;
    }

    for (i = 0; i < cmd_num - 1; i++) {
        cmd_nr = _IOC_NR(g_isp_ioctl_cmd_list[i].cmd);
        if (cmd_nr >= IOC_NR_ISP_BUTT) {
            isp_err_trace("Ioctl cmd is out of range!\n");
            return TD_FAILURE;
        }
    }
    return TD_SUCCESS;
}
