/*
  Copyright (c), 2001-2025, Shenshu Tech. Co., Ltd.
 */

#include "isp_drv_stats.h"
#include "mkp_isp.h"
#include "ot_osal.h"
#include "isp.h"
#include "isp_drv_define.h"
#include "isp_drv_dfx.h"
#include "isp_drv.h"

static td_s32 isp_drv_fe_stat_free_list_prepare(ot_vi_pipe vi_pipe, isp_drv_ctx *drv_ctx)
{
    struct osal_list_head *plist = TD_NULL;

    if (isp_drv_get_run_wakeup_sel(vi_pipe) == OT_ISP_RUN_WAKEUP_BE_END) {
        /* don't clear history busy list statistics, unless free list is empty. */
        if (osal_list_empty(&drv_ctx->fe_statistics_buf.free_list)) {
            plist = drv_ctx->fe_statistics_buf.busy_list.next;
            if (plist == TD_NULL) {
                isp_err_trace("vi_pipe = %d fe statistics info discard, because busy list's node illegal\n", vi_pipe);
                return TD_FAILURE;
            }

            osal_list_del(plist);
            drv_ctx->fe_statistics_buf.busy_num--;

            osal_list_add_tail(plist, &drv_ctx->fe_statistics_buf.free_list);
            drv_ctx->fe_statistics_buf.free_num++;
        }
    } else {
        /* There should be one frame of the newest statistics info in busy list. */
        while (!osal_list_empty(&drv_ctx->fe_statistics_buf.busy_list)) {
            plist = drv_ctx->fe_statistics_buf.busy_list.next;
            if (plist == TD_NULL) {
                return TD_FAILURE;
            }
            osal_list_del(plist);
            drv_ctx->fe_statistics_buf.busy_num--;

            osal_list_add_tail(plist, &drv_ctx->fe_statistics_buf.free_list);
            drv_ctx->fe_statistics_buf.free_num++;
        }
    }

    return TD_SUCCESS;
}

static td_s32 isp_drv_fe_all_statistics_read(ot_vi_pipe vi_pipe, const isp_drv_ctx *drv_ctx, isp_stat_info *stat_info)
{
    td_s32 ret;
    if (drv_ctx->stitch_attr.stitch_enable == TD_TRUE) {
#ifdef CONFIG_OT_VI_STITCH_GRP
        if (drv_ctx->stitch_attr.main_pipe == TD_TRUE) {
            isp_drv_fe_stitch_statistics_read(vi_pipe, stat_info);
        }

        isp_drv_fe_stitch_non_statistics_read(vi_pipe, stat_info);
#endif
    } else {
        ret = isp_drv_fe_statistics_read(vi_pipe, stat_info);
        if (ret != TD_SUCCESS) {
            isp_err_trace("isp_drv_fe_statistics_read failed!\n");
            return TD_FAILURE;
        }
    }

    isp_dfx_sns_sync_verify_show(drv_ctx, stat_info);

    return TD_SUCCESS;
}

static td_void isp_drv_calc_fe_stat_time(isp_drv_ctx *drv_ctx, td_u64 fe_stat_time1)
{
    td_u64 fe_stat_time2 = 0;

    if (ckfn_sys_entry() && ckfn_sys_get_time_stamp()) {
        fe_stat_time2 = call_sys_get_time_stamp();
    }
    drv_ctx->drv_dbg_info.isp_fe_stat_time = fe_stat_time2 - fe_stat_time1;

    if (drv_ctx->drv_dbg_info.isp_fe_stat_time > drv_ctx->drv_dbg_info.isp_fe_stat_time_max) {
        drv_ctx->drv_dbg_info.isp_fe_stat_time_max = drv_ctx->drv_dbg_info.isp_fe_stat_time;
    }
}

static td_s32 isp_drv_fe_int_statistics_read(ot_vi_pipe vi_pipe, isp_stat_info *stat_info)
{
    td_s32 ret;
    isp_drv_ctx *drv_ctx = TD_NULL;
    td_u64 fe_stat_time1 = 0;

    isp_check_pipe_return(vi_pipe);
    isp_check_pointer_return(stat_info);
    drv_ctx = isp_drv_get_ctx(vi_pipe);
    /* online snap, AE and AWB params set by the preview pipe.
      In order to get picture as fast as, dehaze don't used. */
#ifdef CONFIG_OT_SNAP_SUPPORT
    if (is_online_mode(drv_ctx->work_mode.running_mode)) {
        if ((drv_ctx->snap_attr.picture_pipe_id == vi_pipe) &&
            (drv_ctx->snap_attr.picture_pipe_id != drv_ctx->snap_attr.preview_pipe_id)) {
            return TD_SUCCESS;
        }
    }
#endif
    if (ckfn_sys_entry() && ckfn_sys_get_time_stamp()) {
        fe_stat_time1 = call_sys_get_time_stamp();
    }
    ret = isp_drv_fe_all_statistics_read(vi_pipe, drv_ctx, stat_info);
    if (ret != TD_SUCCESS) {
        return ret;
    }
    isp_drv_calc_fe_stat_time(drv_ctx, fe_stat_time1);

    return TD_SUCCESS;
}

td_s32 isp_drv_fe_stats_buf_busy_put(ot_vi_pipe vi_pipe)
{
    td_s32 ret;
    isp_drv_ctx *drv_ctx = TD_NULL;
    struct osal_list_head *plist = TD_NULL;
    isp_stat_node *node = TD_NULL;
    unsigned long flags = 0;

    isp_check_pipe_return(vi_pipe);

    drv_ctx = isp_drv_get_ctx(vi_pipe);
    isp_check_stabuf_init_return(vi_pipe, drv_ctx->fe_statistics_buf.init);

    osal_spin_lock_irqsave(isp_drv_get_lock(vi_pipe), &flags);

    ret = isp_drv_fe_stat_free_list_prepare(vi_pipe, drv_ctx);
    if (ret != TD_SUCCESS) {
        osal_spin_unlock_irqrestore(isp_drv_get_lock(vi_pipe), &flags);
        return TD_FAILURE;
    }

    if (osal_list_empty(&drv_ctx->fe_statistics_buf.free_list)) {
        isp_err_trace("vi_pipe = %d fe statistics info discard!!\n", vi_pipe);
        osal_spin_unlock_irqrestore(isp_drv_get_lock(vi_pipe), &flags);
        return TD_FAILURE;
    }

    /* get free */
    plist = drv_ctx->fe_statistics_buf.free_list.next;
    if (plist == TD_NULL) {
        isp_warn_trace("free list empty\n");
        osal_spin_unlock_irqrestore(isp_drv_get_lock(vi_pipe), &flags);
        return TD_FAILURE;
    }
    osal_list_del(plist);
    drv_ctx->fe_statistics_buf.free_num--;

    /* read statistics */
    node = osal_list_entry(plist, isp_stat_node, list);

    ret = isp_drv_fe_int_statistics_read(vi_pipe, &node->stat_info);

    /* put busy */
    osal_list_add_tail(plist, &drv_ctx->fe_statistics_buf.busy_list);
    drv_ctx->fe_statistics_buf.busy_num++;

    osal_spin_unlock_irqrestore(isp_drv_get_lock(vi_pipe), &flags);

    return ret;
}


static td_s32 isp_drv_delete_busy_put_free(isp_drv_ctx *drv_ctx)
{
    struct osal_list_head *plist = TD_NULL;
    /* There should be one frame of the newest statistics info in busy list. */
    while (!osal_list_empty(&drv_ctx->statistics_buf.busy_list)) {
        plist = drv_ctx->statistics_buf.busy_list.next;
        if (plist == TD_NULL) {
            return TD_FAILURE;
        }
        osal_list_del(plist);
        drv_ctx->statistics_buf.busy_num--;

        osal_list_add_tail(plist, &drv_ctx->statistics_buf.free_list);
        drv_ctx->statistics_buf.free_num++;
    }
    return TD_SUCCESS;
}

static td_s32 isp_drv_switch_be_online_stt_addr(ot_vi_pipe vi_pipe)
{
    td_u8 read_buf_idx;
    td_u8 cur_read_buf_idx;
    td_u8 write_buf_idx;
    td_s32 ret;
    td_u32 cur_read_flag;
    isp_drv_ctx *drv_ctx = TD_NULL;

    isp_check_pipe_return(vi_pipe);
    drv_ctx = isp_drv_get_ctx(vi_pipe);

    cur_read_flag = 1 - drv_ctx->be_online_stt_buf.cur_write_flag;

    ret = isp_drv_set_online_stt_addr(vi_pipe, drv_ctx->be_online_stt_buf.be_stt_buf[cur_read_flag].phy_addr);
    if (ret != TD_SUCCESS) {
        isp_err_trace("ISP[%d] Set ISP online stt addr Err!\n", vi_pipe);
    }

    if (isp_drv_get_ldci_tpr_flt_en(vi_pipe) == TD_TRUE) {
        cur_read_buf_idx = drv_ctx->ldci_write_buf_attr.buf_idx;
        ret = isp_drv_set_ldci_stt_addr(vi_pipe, drv_ctx->ldci_read_buf_attr.ldci_buf[0].phy_addr,
            drv_ctx->ldci_write_buf_attr.ldci_buf[cur_read_buf_idx].phy_addr);
    } else {
        read_buf_idx = drv_ctx->ldci_read_buf_attr.buf_idx;
        write_buf_idx = drv_ctx->ldci_write_buf_attr.buf_idx;

        ret = isp_drv_set_ldci_stt_addr(vi_pipe, drv_ctx->ldci_read_buf_attr.ldci_buf[read_buf_idx].phy_addr,
            drv_ctx->ldci_write_buf_attr.ldci_buf[write_buf_idx].phy_addr);
    }

    return TD_SUCCESS;
}

/* ISP BE read sta from FHY, online mode */
static td_s32 isp_drv_be_online_statistics_read(ot_vi_pipe vi_pipe, isp_stat_info *stat_info)
{
    isp_stat *stat = TD_NULL;
    isp_stat_key stat_key;

    isp_check_pipe_return(vi_pipe);
    isp_check_pointer_return(stat_info);

    stat = (isp_stat *)stat_info->virt_addr;
    if (stat == TD_NULL) {
        return TD_FAILURE;
    }

    stat_key.key = stat_info->stat_key.key;
    stat->be_update = TD_TRUE;

    isp_drv_be_apb_statistics_read(vi_pipe, stat, stat_key);
    isp_drv_be_stt_statistics_read(vi_pipe, stat, stat_key);

    return isp_drv_switch_be_online_stt_addr(vi_pipe); /* for debug */
}

static td_s32 isp_drv_be_all_statistics_read(ot_vi_pipe vi_pipe, const isp_drv_ctx *drv_ctx, isp_stat_info *stat_info)
{
    td_s32 ret;
    if (is_online_mode(drv_ctx->work_mode.running_mode)) {
        /* BE statistics for online */
        ret = isp_drv_be_online_statistics_read(vi_pipe, stat_info);
        if (ret != TD_SUCCESS) {
            isp_err_trace("isp_drv_be_online_statistics_read failed!\n");
            return TD_FAILURE;
        }
    } else if (is_offline_mode(drv_ctx->work_mode.running_mode) || is_striping_mode(drv_ctx->work_mode.running_mode) ||
        is_pre_online_post_offline(drv_ctx->work_mode.running_mode)) {
        /* BE statistics for offline */
        ret = isp_drv_be_offline_statistics_read(vi_pipe, stat_info);
        if (ret) {
            isp_err_trace("isp_drv_be_offline_statistics_read failed!\n");
            return TD_FAILURE;
        }

        isp_drv_be_offline_stitch_statistics_read(vi_pipe, stat_info);
    } else {
        isp_err_trace("running_mode err 0x%x!\n", drv_ctx->work_mode.running_mode);
        return TD_FAILURE;
    }

    return TD_SUCCESS;
}

static td_void isp_drv_calc_be_stat_time(isp_drv_ctx *drv_ctx, td_u64 be_stat_time1)
{
    td_u64 be_stat_time2 = 0;

    if (ckfn_sys_entry() && ckfn_sys_get_time_stamp()) {
        be_stat_time2 = call_sys_get_time_stamp();
    }
    drv_ctx->drv_dbg_info.isp_be_stat_time = be_stat_time2 - be_stat_time1;

    if (drv_ctx->drv_dbg_info.isp_be_stat_time > drv_ctx->drv_dbg_info.isp_be_stat_time_max) {
        drv_ctx->drv_dbg_info.isp_be_stat_time_max = drv_ctx->drv_dbg_info.isp_be_stat_time;
    }
}

static td_s32 isp_drv_be_statistics_read(ot_vi_pipe vi_pipe, isp_stat_info *stat_info)
{
    td_s32 ret;
    isp_drv_ctx *drv_ctx = TD_NULL;
    td_u64 be_stat_time1 = 0;

    isp_check_pipe_return(vi_pipe);
    isp_check_pointer_return(stat_info);

    drv_ctx = isp_drv_get_ctx(vi_pipe);
    /* online snap, AE and AWB params set by the preview pipe.
      In order to get picture as fast as, dehaze don't used. */
#ifdef CONFIG_OT_SNAP_SUPPORT
    if (is_online_mode(drv_ctx->work_mode.running_mode)) {
        if ((drv_ctx->snap_attr.picture_pipe_id == vi_pipe) &&
            (drv_ctx->snap_attr.picture_pipe_id != drv_ctx->snap_attr.preview_pipe_id)) {
            return TD_SUCCESS;
        }
    }
#endif
    if ((isp_drv_get_alg_run_select(vi_pipe) == OT_ISP_ALG_RUN_FE_ONLY) && (drv_ctx->yuv_mode != TD_TRUE)) {
        return TD_SUCCESS;
    }
    if (ckfn_sys_entry() && ckfn_sys_get_time_stamp()) {
        be_stat_time1 = call_sys_get_time_stamp();
    }
    ret = isp_drv_be_all_statistics_read(vi_pipe, drv_ctx, stat_info);
    if (ret != TD_SUCCESS) {
        return ret;
    }
    isp_drv_calc_be_stat_time(drv_ctx, be_stat_time1);

    return TD_SUCCESS;
}

static td_void isp_drv_calc_cp_fe_stat_time(isp_drv_ctx *drv_ctx, td_u64 cp_fe_stat_time1)
{
    td_u64 cp_fe_stat_time2 = 0;

    if (ckfn_sys_entry() && ckfn_sys_get_time_stamp()) {
        cp_fe_stat_time2 = call_sys_get_time_stamp();
    }
    drv_ctx->drv_dbg_info.isp_cp_fe_stat_time = cp_fe_stat_time2 - cp_fe_stat_time1;

    if (drv_ctx->drv_dbg_info.isp_cp_fe_stat_time > drv_ctx->drv_dbg_info.isp_cp_fe_stat_time_max) {
        drv_ctx->drv_dbg_info.isp_cp_fe_stat_time_max = drv_ctx->drv_dbg_info.isp_cp_fe_stat_time;
    }
}

static td_void isp_drv_fe_stat_mem_mov(isp_stat *stat, const isp_fe_stat *fe_stat, td_u64 stat_key)
{
    isp_stat_key un_statkey;

    un_statkey.key = stat_key;

    (td_void)memcpy_s(&stat->be_stats_calc_info, sizeof(isp_be_stats_calc_info),
        &fe_stat->be_stats_calc_info, sizeof(isp_be_stats_calc_info));

    if (un_statkey.bit1_fe_ae_global_stat) {
        (td_void)memcpy_s(&stat->fe_ae_stat1, sizeof(ot_isp_fe_ae_stat_1),
            &fe_stat->fe_ae_stat1, sizeof(ot_isp_fe_ae_stat_1));
    }
#ifdef CONFIG_OT_ISP_FE_AE_GLOBAL_STAT_SUPPORT
    (td_void)memcpy_s(&stat->fe_ae_stat2, sizeof(ot_isp_fe_ae_stat_2),
        &fe_stat->fe_ae_stat2, sizeof(ot_isp_fe_ae_stat_2));
#endif
#ifdef CONFIG_OT_ISP_FE_AE_ZONE_STAT_SUPPORT
    if (un_statkey.bit1_fe_ae_local_stat) {
        (td_void)memcpy_s(&stat->fe_ae_stat3, sizeof(ot_isp_fe_ae_stat_3),
            &fe_stat->fe_ae_stat3, sizeof(ot_isp_fe_ae_stat_3));
    }
#endif
#ifdef CONFIG_OT_ISP_FE_AF_STAT_SUPPORT
    if (un_statkey.bit1_fe_af_stat) {
        (td_void)memcpy_s(&stat->fe_af_stat, sizeof(ot_isp_fe_af_stat),
            &fe_stat->fe_af_stat, sizeof(ot_isp_fe_af_stat));
    }
#endif
#ifdef CONFIG_OT_VI_STITCH_GRP
    if (un_statkey.bit1_fe_ae_stitch_global_stat) {
        (td_void)memcpy_s(&stat->stitch_stat.fe_ae_stat1, sizeof(ot_isp_fe_ae_stat_1),
            &fe_stat->stitch_stat.fe_ae_stat1, sizeof(ot_isp_fe_ae_stat_1));
    }

    (td_void)memcpy_s(&stat->stitch_stat.fe_ae_stat2, sizeof(ot_isp_fe_ae_stat_2),
        &fe_stat->stitch_stat.fe_ae_stat2, sizeof(ot_isp_fe_ae_stat_2));

    if (un_statkey.bit1_fe_ae_stitch_local_stat) {
        (td_void)memcpy_s(&stat->stitch_stat.fe_ae_stat3, sizeof(ot_isp_fe_ae_stitch_stat_3),
            &fe_stat->stitch_stat.fe_ae_stat3, sizeof(ot_isp_fe_ae_stitch_stat_3));
    }
#endif
}
/* (1) runbe + lowdelay mode, user can get frame before fstart, be_end is before next frame start */
/* the stat of cur frame is not updated(update at fstart), this function may can't get the match be stat */
/* so get the nearest fe stat firstly(fn-1), then try to get the match one, to prevent not find the match one */
/* in this case, the stat may be fe(fn-1) + be(fn) */
/* (2) if be is far behind of fe, can't find the match one, then use the original stat, AE can't be converging */
/* then use the oldest one of six buffers, to avoid getting the original stat(too old) */
/* choose the oldest not the newest to aviod the concussion of fe_stat */
static td_s32 isp_drv_get_fe_stat_for_tolerance(ot_vi_pipe vi_pipe, isp_stat *stat,
    isp_stat_key stat_key)
{
    td_s32 ret;
    td_bool pipe_low_delay_en = TD_FALSE;
    isp_fe_stat *fe_stat = TD_NULL;
    isp_stat_node *node = TD_NULL;
    struct osal_list_head *list_tmp = TD_NULL;
    struct osal_list_head *list_node = TD_NULL;
    isp_drv_ctx *drv_ctx = TD_NULL;

    drv_ctx = isp_drv_get_ctx(vi_pipe);
    ret = isp_drv_get_pipe_low_delay_en(vi_pipe, &pipe_low_delay_en);
    if (ret != TD_SUCCESS) {
        isp_warn_trace("ISP[%d] Get pipe_low_delay_en_attr failed !!!", vi_pipe);
    }
    if (pipe_low_delay_en == TD_TRUE) {
        osal_list_for_each_prev_safe(list_node, list_tmp, &drv_ctx->fe_statistics_buf.busy_list) {
            node = osal_list_entry(list_node, isp_stat_node, list);
            fe_stat = (isp_fe_stat *)node->stat_info.virt_addr;
            if (fe_stat == TD_NULL) {
                isp_warn_trace("vi_pipe %d, isp_fe_stat is null!\n", vi_pipe);
                return TD_FAILURE;
            }
            isp_drv_fe_stat_mem_mov(stat, fe_stat, stat_key.key); // find the nearest one, return
            return TD_SUCCESS;
        }
    } else {
        osal_list_for_each_safe(list_node, list_tmp, &drv_ctx->fe_statistics_buf.busy_list) {
            node = osal_list_entry(list_node, isp_stat_node, list);
            fe_stat = (isp_fe_stat *)node->stat_info.virt_addr;
            if (fe_stat == TD_NULL) {
                isp_warn_trace("vi_pipe %d, isp_fe_stat is null!\n", vi_pipe);
                return TD_FAILURE;
            }
            isp_drv_fe_stat_mem_mov(stat, fe_stat, stat_key.key); // find the oldest one of 6 buffers, return
            return TD_SUCCESS;
        }
    }
    return TD_SUCCESS;
}

static td_s32 isp_drv_fe_stat_buf_busy_get(ot_vi_pipe vi_pipe, td_u64 target_pts,
    isp_stat *stat, isp_stat_key stat_key)
{
    td_s32 ret = TD_FALSE;
    isp_drv_ctx *drv_ctx = TD_NULL;
    isp_stat_node *node = TD_NULL;
    isp_fe_stat *fe_stat = TD_NULL;
    struct osal_list_head *list_tmp = TD_NULL;
    struct osal_list_head *list_node = TD_NULL;
    td_u64 cp_fe_stat_time1 = 0;

    drv_ctx = isp_drv_get_ctx(vi_pipe);
    if (ckfn_sys_entry() && ckfn_sys_get_time_stamp()) {
        cp_fe_stat_time1 = call_sys_get_time_stamp();
    }
    if (isp_drv_get_run_wakeup_sel(vi_pipe) == OT_ISP_RUN_WAKEUP_BE_END) {
        ret = isp_drv_get_fe_stat_for_tolerance(vi_pipe, stat, stat_key);
        if (ret != TD_SUCCESS) {
            isp_warn_trace("vi_pipe %d, get fe stat buf failed!\n", vi_pipe);
        }
    }

    osal_list_for_each_safe(list_node, list_tmp, &drv_ctx->fe_statistics_buf.busy_list) {
        node = osal_list_entry(list_node, isp_stat_node, list);
        fe_stat = (isp_fe_stat *)node->stat_info.virt_addr;
        if (fe_stat == TD_NULL) {
            isp_warn_trace("vi_pipe %d, isp_fe_stat is null!\n", vi_pipe);
            return TD_FAILURE;
        }

        if ((isp_drv_get_run_wakeup_sel(vi_pipe) == OT_ISP_RUN_WAKEUP_FE_START) ||
            (fe_stat->fe_frame_pts == target_pts)) {
            isp_drv_fe_stat_mem_mov(stat, fe_stat, stat_key.key);

            osal_list_del(list_node);
            drv_ctx->fe_statistics_buf.busy_num--;

            osal_list_add_tail(list_node, &drv_ctx->fe_statistics_buf.free_list);
            drv_ctx->fe_statistics_buf.free_num++;
            isp_drv_calc_cp_fe_stat_time(drv_ctx, cp_fe_stat_time1);
            return TD_SUCCESS;
        } else if (fe_stat->fe_frame_pts < target_pts) {
            /* fe frame pts < target be frame pts, it means this fe raw discard so this fe stat info discard together */
            osal_list_del(list_node);
            drv_ctx->fe_statistics_buf.busy_num--;

            osal_list_add_tail(list_node, &drv_ctx->fe_statistics_buf.free_list);
            drv_ctx->fe_statistics_buf.free_num++;
        } else {
           /* fe frame pts > target be frame pts, it means that the following nodes will also not match */
            isp_warn_trace("vi pipe %d, be pts(%llu) not found!! this statistics info may be something wrong!\n",
                vi_pipe, target_pts);
            return TD_FAILURE;
        }
    }

    isp_warn_trace("vi pipe %d, be pts(%llu) not found!! this statistics info may be something wrong!\n",
        vi_pipe, target_pts);

    return TD_FAILURE;
}

td_s32 isp_drv_stats_buf_busy_put(ot_vi_pipe vi_pipe)
{
    td_s32 ret = TD_FAILURE;
    isp_drv_ctx *drv_ctx = TD_NULL;
    struct osal_list_head *plist = TD_NULL;
    isp_stat_node *node = TD_NULL;
    unsigned long flags = 0;
    isp_stat *stat = TD_NULL;

    isp_check_pipe_return(vi_pipe);

    drv_ctx = isp_drv_get_ctx(vi_pipe);
    isp_check_stabuf_init_return(vi_pipe, drv_ctx->statistics_buf.init);

    osal_spin_lock_irqsave(isp_drv_get_lock(vi_pipe), &flags);
    if (drv_ctx->be_off_stt_buf.be_broken != TD_TRUE) {
        ret = isp_drv_delete_busy_put_free(drv_ctx);
        if (ret != TD_SUCCESS) {
            goto exit;
        }
    }
    if (osal_list_empty(&drv_ctx->statistics_buf.free_list)) {
        isp_warn_trace("free list empty\n");
        goto exit;
    }

    /* get free */
    plist = drv_ctx->statistics_buf.free_list.next;
    if (plist == TD_NULL) {
        isp_warn_trace("free list empty\n");
        goto exit;
    }
    osal_list_del(plist);
    drv_ctx->statistics_buf.free_num--;
    /* read statistics */
    node = osal_list_entry(plist, isp_stat_node, list);

    drv_ctx->statistics_buf.act_stat = &node->stat_info;

    stat = (isp_stat *)node->stat_info.virt_addr;
    if (stat == TD_NULL) {
        osal_list_add_tail(plist, &drv_ctx->statistics_buf.free_list);
        drv_ctx->statistics_buf.free_num++;
        goto exit;
    }

    if (drv_ctx->be_off_stt_buf.be_broken != TD_TRUE) {
        isp_drv_be_statistics_read(vi_pipe, &node->stat_info);
    }
    isp_drv_fe_stat_buf_busy_get(vi_pipe, drv_ctx->frame_pts, stat, node->stat_info.stat_key);

    stat->frame_pts = drv_ctx->frame_pts;

    /* put busy */
    osal_list_add_tail(plist, &drv_ctx->statistics_buf.busy_list);
    drv_ctx->statistics_buf.busy_num++;
    isp_info_trace("pipe:%d, be_broken:%d\n", vi_pipe, drv_ctx->be_off_stt_buf.be_broken);

exit:
    osal_spin_unlock_irqrestore(isp_drv_get_lock(vi_pipe), &flags);

    return ret;
}

static td_void isp_drv_be_af_stat_read(ot_vi_pipe vi_pipe, isp_stat_info *stat_info)
{
    td_s32 ret;
    isp_be_stat_valid stat_valid;
    isp_stat_key stat_key;
    isp_drv_ctx *drv_ctx = TD_NULL;
    isp_stat *stat_dst = TD_NULL;
    isp_stat *stat_src = TD_NULL;
    isp_stat_info *act_stat_info = TD_NULL;
    drv_ctx = isp_drv_get_ctx(vi_pipe);
    act_stat_info = drv_ctx->statistics_buf.act_stat;
    if (act_stat_info == TD_NULL) {
        return;
    }
    stat_dst = (isp_stat *)act_stat_info->virt_addr;
    stat_src = (isp_stat *)stat_info->virt_addr;
    if (stat_dst == TD_NULL || stat_src == TD_NULL) {
        return;
    }
    stat_valid.key = drv_ctx->be_off_stt_buf.stat_valid.key;
    stat_key.key = act_stat_info->stat_key.key;
    if (stat_key.bit1_be_af_stat && stat_valid.bits.bit_be_af_stat) {
        ret = memcpy_s(&(stat_dst->be_af_stat), sizeof(stat_dst->be_af_stat), &(stat_src->be_af_stat),
            sizeof(stat_src->be_af_stat));
        if (ret != EOK) {
            isp_err_trace("memcpy_s af stat err\n");
            return;
        }
    }
}

static td_s32 isp_drv_be_stat_buf_read(ot_vi_pipe vi_pipe)
{
    isp_stat_node *node = TD_NULL;
    isp_stat *stat = TD_NULL;
    td_s32 ret = TD_FAILURE;
    isp_drv_ctx *drv_ctx = TD_NULL;
    struct osal_list_head *plist = TD_NULL;

    isp_check_pipe_return(vi_pipe);
    drv_ctx = isp_drv_get_ctx(vi_pipe);
    isp_check_stabuf_init_return(vi_pipe, drv_ctx->statistics_buf.init);
    /* There should be one frame of the newest statistics info in busy list. */
    ret = isp_drv_delete_busy_put_free(drv_ctx);
    if (ret != TD_SUCCESS) {
        return ret;
    }

    if (osal_list_empty(&drv_ctx->statistics_buf.free_list)) {
        isp_warn_trace("free list empty\n");
        return ret;
    }

    /* get free */
    plist = drv_ctx->statistics_buf.free_list.next;
    if (plist == TD_NULL) {
        isp_warn_trace("free list empty\n");
        return ret;
    }

    /* read statistics */
    node = osal_list_entry(plist, isp_stat_node, list);
    stat = (isp_stat *)node->stat_info.virt_addr;
    if (stat == TD_NULL) {
        osal_list_add_tail(plist, &drv_ctx->statistics_buf.free_list);
        drv_ctx->statistics_buf.free_num++;
        return ret;
    }

    isp_drv_be_statistics_read(vi_pipe, &node->stat_info);
    isp_drv_be_af_stat_read(vi_pipe, &node->stat_info);

    return TD_SUCCESS;
}


td_void isp_drv_stats_be_end_read(ot_vi_pipe vi_pipe)
{
    isp_drv_ctx *drv_ctx = TD_NULL;
    isp_check_pipe_void_return(vi_pipe);
    drv_ctx = isp_drv_get_ctx(vi_pipe);
    if (drv_ctx->isp_run_flag == TD_FALSE) {
        return;
    }
    /* defalut or ai3dnr mode, stat_valid=0xffffffff, */
    /* aidrc or aidm, stat_valid may be 0xfffffffff too, but pre and post are all worked, don't read stats in advance */
    drv_ctx->be_off_stt_buf.be_broken = TD_FALSE; // means be is not broken
    /* read statistics when offline mode at be end(be_pre finished or be_post finished) proc interrupt */
    if (drv_ctx->be_off_stt_buf.stat_valid.key == 0xffffffff) { // means defalut or ai3dnr, not aidrc or aidm
        return;
    }
    drv_ctx->be_off_stt_buf.be_broken = TD_TRUE; // means aidrc or aidm
    isp_drv_be_stat_buf_read(vi_pipe);
}

td_void isp_drv_stats_be_af_offline_end_int_read(ot_vi_pipe vi_pipe)
{
    td_u8 blk_num;
    unsigned long flags = 0;
    isp_drv_ctx *drv_ctx = TD_NULL;
    isp_stat *stat = TD_NULL;
    isp_stat_info *stat_info = TD_NULL;
    isp_stat_key stat_key;

    if ((vi_pipe < 0) || (vi_pipe >= OT_ISP_MAX_PIPE_NUM)) {
        return;
    }

    drv_ctx = isp_drv_get_ctx(vi_pipe);
    if (drv_ctx->isp_run_flag == TD_FALSE) {
        return;
    }
    if (is_online_mode(drv_ctx->work_mode.running_mode)) {
        return;
    }

    /* read af statistics when offline mode at be end proc interrupt */
    blk_num = isp_drv_get_block_num(vi_pipe);
    blk_num = div_0_to_1(blk_num);

    osal_spin_lock_irqsave(isp_drv_get_lock(vi_pipe), &flags);
    if (drv_ctx->be_off_stt_buf.be_broken == TD_TRUE) { // aidrc or aidm, be is broken, already read af stats
        goto exit;
    }

    if (drv_ctx->statistics_buf.init == TD_FALSE) {
        goto exit;
    }

    stat_info = drv_ctx->statistics_buf.act_stat;
    if (stat_info == TD_NULL) {
        goto exit;
    }
    stat = (isp_stat *)stat_info->virt_addr;
    if (stat == TD_NULL) {
        goto exit;
    }

    stat_key.key = stat_info->stat_key.key;
    isp_drv_read_af_offline_stats_end_int(vi_pipe, drv_ctx, blk_num, stat_info, stat_key);

exit:
    osal_spin_unlock_irqrestore(isp_drv_get_lock(vi_pipe), &flags);
    return;
}


static td_s32 isp_drv_fe_stat_buf_init(ot_vi_pipe vi_pipe)
{
    td_s32 ret, i;
    isp_drv_ctx *drv_ctx = TD_NULL;
    td_phys_addr_t phy_addr;
    td_u8 *vir_addr = TD_NULL;
    td_char ac_name[MAX_MMZ_NAME_LEN] = {0};
    unsigned long flags = 0;
    osal_spinlock *isp_spin_lock = TD_NULL;
    mm_malloc_param malloc_param = {0};
    td_u32 buf_num;

    drv_ctx = isp_drv_get_ctx(vi_pipe);

    buf_num = (isp_drv_get_run_wakeup_sel(vi_pipe) == OT_ISP_RUN_WAKEUP_FE_START ?
        1 : MAX_ISP_FE_STAT_BUF_NUM);

    isp_check_buf_init_return(vi_pipe, drv_ctx->fe_statistics_buf.init);

    ret = snprintf_s(ac_name, sizeof(ac_name), sizeof(ac_name) - 1, "isp[%d].fe_stat", vi_pipe);
    if (ret < 0) {
        return TD_FAILURE;
    }

    malloc_param.buf_name = ac_name;
    malloc_param.size = sizeof(isp_fe_stat) * buf_num;
    malloc_param.kernel_only = TD_TRUE;
    ret = cmpi_mmz_malloc_cached(&malloc_param, &phy_addr, (td_void **)&vir_addr);
    if (ret != TD_SUCCESS) {
        isp_err_trace("alloc ISP statistics buf err\n");
        return OT_ERR_ISP_NOMEM;
    }

    (td_void)memset_s(vir_addr, malloc_param.size, 0, malloc_param.size);
    isp_spin_lock = isp_drv_get_lock(vi_pipe);

    osal_spin_lock_irqsave(isp_spin_lock, &flags);
    drv_ctx->fe_statistics_buf.phy_addr = phy_addr;
    drv_ctx->fe_statistics_buf.vir_addr = (td_void *)vir_addr;
    drv_ctx->fe_statistics_buf.size = malloc_param.size;

    OSAL_INIT_LIST_HEAD(&drv_ctx->fe_statistics_buf.free_list);
    OSAL_INIT_LIST_HEAD(&drv_ctx->fe_statistics_buf.busy_list);

    for (i = 0; i < buf_num; i++) {
        drv_ctx->fe_statistics_buf.node[i].stat_info.phy_addr = phy_addr + i * sizeof(isp_fe_stat);
        drv_ctx->fe_statistics_buf.node[i].stat_info.virt_addr = (td_void *)(vir_addr + i * sizeof(isp_fe_stat));

        drv_ctx->fe_statistics_buf.node[i].stat_info.stat_key.key = ISP_STATISTICS_KEY;

        osal_list_add_tail(&drv_ctx->fe_statistics_buf.node[i].list, &drv_ctx->fe_statistics_buf.free_list);
    }

    drv_ctx->fe_statistics_buf.init = TD_TRUE;
    drv_ctx->fe_statistics_buf.busy_num = 0;
    drv_ctx->fe_statistics_buf.free_num = buf_num;

    osal_spin_unlock_irqrestore(isp_spin_lock, &flags);

    return TD_SUCCESS;
}

td_s32 isp_drv_stats_buf_exit(ot_vi_pipe vi_pipe)
{
    isp_drv_ctx *drv_ctx = TD_NULL;
    td_phys_addr_t phy_addr;
    td_u8 *vir_addr = TD_NULL;

    td_phys_addr_t fe_phy_addr;
    td_u8 *fe_vir_addr = TD_NULL;

    unsigned long flags = 0;
    osal_spinlock *isp_spin_lock = TD_NULL;

    isp_check_pipe_return(vi_pipe);

    drv_ctx = isp_drv_get_ctx(vi_pipe);
    isp_check_exit_state_return(vi_pipe, drv_ctx->isp_run_flag);

    if ((drv_ctx->statistics_buf.init == TD_FALSE) && (drv_ctx->fe_statistics_buf.init == TD_FALSE)) {
        return TD_SUCCESS;
    }

    isp_spin_lock = isp_drv_get_lock(vi_pipe);
    osal_spin_lock_irqsave(isp_spin_lock, &flags);

    phy_addr = drv_ctx->statistics_buf.phy_addr;
    vir_addr = (td_u8 *)drv_ctx->statistics_buf.vir_addr;

    fe_phy_addr = drv_ctx->fe_statistics_buf.phy_addr;
    fe_vir_addr = (td_u8 *)drv_ctx->fe_statistics_buf.vir_addr;

    drv_ctx->statistics_buf.vir_addr = TD_NULL;
    drv_ctx->statistics_buf.node[0].stat_info.virt_addr = TD_NULL;
    drv_ctx->statistics_buf.node[1].stat_info.virt_addr = TD_NULL;
    drv_ctx->statistics_buf.phy_addr = 0;
    drv_ctx->statistics_buf.node[0].stat_info.phy_addr = 0;
    drv_ctx->statistics_buf.node[1].stat_info.phy_addr = 0;
    drv_ctx->statistics_buf.init = TD_FALSE;
    drv_ctx->statistics_buf.act_stat = TD_NULL;

    drv_ctx->fe_statistics_buf.vir_addr = TD_NULL;
    drv_ctx->fe_statistics_buf.node[0].stat_info.virt_addr = TD_NULL;
    drv_ctx->fe_statistics_buf.node[1].stat_info.virt_addr = TD_NULL;
    drv_ctx->fe_statistics_buf.phy_addr = 0;
    drv_ctx->fe_statistics_buf.node[0].stat_info.phy_addr = 0;
    drv_ctx->fe_statistics_buf.node[1].stat_info.phy_addr = 0;
    drv_ctx->fe_statistics_buf.init = TD_FALSE;
    drv_ctx->fe_statistics_buf.act_stat = TD_NULL;

    OSAL_INIT_LIST_HEAD(&drv_ctx->statistics_buf.free_list);
    OSAL_INIT_LIST_HEAD(&drv_ctx->statistics_buf.busy_list);
    OSAL_INIT_LIST_HEAD(&drv_ctx->statistics_buf.user_list);

    OSAL_INIT_LIST_HEAD(&drv_ctx->fe_statistics_buf.free_list);
    OSAL_INIT_LIST_HEAD(&drv_ctx->fe_statistics_buf.busy_list);

    osal_spin_unlock_irqrestore(isp_spin_lock, &flags);

    if (phy_addr != 0) {
        cmpi_mmz_free(phy_addr, vir_addr);
    }

    if (fe_phy_addr != 0) {
        cmpi_mmz_free(fe_phy_addr, fe_vir_addr);
    }

    return TD_SUCCESS;
}

static td_void isp_drv_stat_buf_default(ot_vi_pipe vi_pipe)
{
    isp_drv_ctx *drv_ctx = TD_NULL;
    drv_ctx = isp_drv_get_ctx(vi_pipe);

    drv_ctx->statistics_buf.init = TD_TRUE;
    drv_ctx->statistics_buf.busy_num = 0;
    drv_ctx->statistics_buf.user_num = 0;
    drv_ctx->statistics_buf.free_num = MAX_ISP_STAT_BUF_NUM;
    drv_ctx->statistics_buf.act_stat = &drv_ctx->statistics_buf.node[0].stat_info;
}

td_s32 isp_drv_stats_buf_init(ot_vi_pipe vi_pipe, td_phys_addr_t *point_phy_addr)
{
    td_s32 ret, i;
    isp_drv_ctx *drv_ctx = TD_NULL;
    td_phys_addr_t phy_addr;
    td_u8 *vir_addr = TD_NULL;
    td_char ac_name[MAX_MMZ_NAME_LEN] = {0};
    unsigned long flags = 0;
    osal_spinlock *isp_spin_lock = TD_NULL;
    mm_malloc_param malloc_param = {0};

    isp_check_pipe_return(vi_pipe);
    isp_check_pointer_return(point_phy_addr);

    drv_ctx = isp_drv_get_ctx(vi_pipe);

    isp_check_buf_init_return(vi_pipe, drv_ctx->statistics_buf.init);

    if (snprintf_s(ac_name, sizeof(ac_name), sizeof(ac_name) - 1, "isp[%d].stat", vi_pipe) < 0) {
        return TD_FAILURE;
    }

    malloc_param.buf_name = ac_name;
    malloc_param.size = sizeof(isp_stat) * MAX_ISP_STAT_BUF_NUM;
    malloc_param.kernel_only = TD_FALSE;
    ret = cmpi_mmz_malloc_cached(&malloc_param, &phy_addr, (td_void **)&vir_addr);
    if (ret != TD_SUCCESS) {
        isp_err_trace("alloc ISP statistics buf err\n");
        return OT_ERR_ISP_NOMEM;
    }

    (td_void)memset_s(vir_addr, malloc_param.size, 0, malloc_param.size);
    isp_spin_lock = isp_drv_get_lock(vi_pipe);
    osal_spin_lock_irqsave(isp_spin_lock, &flags);
    drv_ctx->statistics_buf.phy_addr = phy_addr;
    drv_ctx->statistics_buf.vir_addr = (td_void *)vir_addr;
    drv_ctx->statistics_buf.size = malloc_param.size;

    OSAL_INIT_LIST_HEAD(&drv_ctx->statistics_buf.free_list);
    OSAL_INIT_LIST_HEAD(&drv_ctx->statistics_buf.busy_list);
    OSAL_INIT_LIST_HEAD(&drv_ctx->statistics_buf.user_list);

    for (i = 0; i < MAX_ISP_STAT_BUF_NUM; i++) {
        drv_ctx->statistics_buf.node[i].stat_info.phy_addr = phy_addr + i * sizeof(isp_stat);
        drv_ctx->statistics_buf.node[i].stat_info.virt_addr = (td_void *)(vir_addr + i * sizeof(isp_stat));

        drv_ctx->statistics_buf.node[i].stat_info.stat_key.key = ISP_STATISTICS_KEY;

        osal_list_add_tail(&drv_ctx->statistics_buf.node[i].list, &drv_ctx->statistics_buf.free_list);
    }

    isp_drv_stat_buf_default(vi_pipe);

    *point_phy_addr = drv_ctx->statistics_buf.phy_addr;

    osal_spin_unlock_irqrestore(isp_spin_lock, &flags);

    if (isp_drv_fe_stat_buf_init(vi_pipe) != TD_SUCCESS) {
        isp_err_trace("alloc ISP statistics buf err\n");
        isp_drv_stats_buf_exit(vi_pipe);
        return TD_FAILURE;
    }

    return TD_SUCCESS;
}

static td_s32 isp_drv_stat_buf_user_get(ot_vi_pipe vi_pipe, isp_stat_info **stat_info)
{
    isp_drv_ctx *drv_ctx = TD_NULL;
    struct osal_list_head *plist = TD_NULL;
    isp_stat_node *node = TD_NULL;
    unsigned long flags = 0;
    osal_spinlock *isp_spin_lock = TD_NULL;

    isp_check_pipe_return(vi_pipe);
    isp_check_pointer_return(stat_info);

    drv_ctx = isp_drv_get_ctx(vi_pipe);
    isp_check_stabuf_init_return(vi_pipe, drv_ctx->statistics_buf.init);
    isp_spin_lock = isp_drv_get_lock(vi_pipe);
    osal_spin_lock_irqsave(isp_spin_lock, &flags);

    if (osal_list_empty(&drv_ctx->statistics_buf.busy_list)) {
        isp_warn_trace("busy list empty\n");
        *stat_info = TD_NULL;
        osal_spin_unlock_irqrestore(isp_spin_lock, &flags);
        return TD_FAILURE;
    }

    /* get busy */
    plist = drv_ctx->statistics_buf.busy_list.next;
    if (plist == TD_NULL) {
        isp_warn_trace("busy list empty\n");
        *stat_info = TD_NULL;
        osal_spin_unlock_irqrestore(isp_spin_lock, &flags);
        return TD_FAILURE;
    }
    osal_list_del(plist);
    drv_ctx->statistics_buf.busy_num--;

    /* return info */
    node = osal_list_entry(plist, isp_stat_node, list);
    *stat_info = &node->stat_info;

    /* put user */
    osal_list_add_tail(plist, &drv_ctx->statistics_buf.user_list);
    drv_ctx->statistics_buf.user_num++;
    osal_spin_unlock_irqrestore(isp_spin_lock, &flags);

    return TD_SUCCESS;
}

td_s32 isp_drv_stats_buf_user_put(ot_vi_pipe vi_pipe, isp_stat_info *stat_info)
{
    isp_drv_ctx *drv_ctx = TD_NULL;
    struct osal_list_head *plist = TD_NULL;
    isp_stat_node *node = TD_NULL;
    td_bool valid = TD_FALSE;
    unsigned long flags = 0;
    osal_spinlock *isp_spin_lock = TD_NULL;
    td_s32 ret;

    isp_check_pipe_return(vi_pipe);
    isp_check_pointer_return(stat_info);

    ret = ot_mmz_check_phys_addr(stat_info->phy_addr, sizeof(isp_stat));
    if (ret != TD_SUCCESS) {
        isp_err_trace("stat addr check error\n");
        return TD_FAILURE;
    }

    drv_ctx = isp_drv_get_ctx(vi_pipe);
    isp_check_stabuf_init_return(vi_pipe, drv_ctx->statistics_buf.init);
    isp_spin_lock = isp_drv_get_lock(vi_pipe);
    osal_spin_lock_irqsave(isp_spin_lock, &flags);
    osal_list_for_each(plist, &drv_ctx->statistics_buf.user_list)
    {
        node = osal_list_entry(plist, isp_stat_node, list);
        if (node == TD_NULL) {
            isp_err_trace("node  null pointer\n");
            break;
        }

        if (node->stat_info.phy_addr == stat_info->phy_addr) {
            valid = TD_TRUE;
            node->stat_info.stat_key.key = stat_info->stat_key.key;
            break;
        }
    }

    if (!valid) {
        isp_err_trace("invalid stat info, please check it\n");
        osal_spin_unlock_irqrestore(isp_spin_lock, &flags);
        return TD_FAILURE;
    }

    /* get user */
    if (plist == TD_NULL) {
        isp_err_trace("user list empty\n");
        osal_spin_unlock_irqrestore(isp_spin_lock, &flags);
        return TD_FAILURE;
    }
    osal_list_del(plist);
    drv_ctx->statistics_buf.user_num--;

    /* put free */
    osal_list_add_tail(plist, &drv_ctx->statistics_buf.free_list);
    drv_ctx->statistics_buf.free_num++;
    osal_spin_unlock_irqrestore(isp_spin_lock, &flags);

    return TD_SUCCESS;
}

td_s32 isp_drv_stats_buf_user_get(ot_vi_pipe vi_pipe, isp_stat_info *stat)
{
    td_s32 ret;
    isp_stat_info *stat_info = TD_NULL;

    isp_check_pointer_return(stat);

    ret = isp_drv_stat_buf_user_get(vi_pipe, &stat_info);
    if (ret != TD_SUCCESS) {
        return TD_FAILURE;
    }

    if (stat_info == TD_NULL) {
        return TD_FAILURE;
    }

    ret = ot_mmz_check_phys_addr(stat_info->phy_addr, sizeof(isp_stat));
    if (ret != TD_SUCCESS) {
        isp_err_trace("stat addr check error\n");
        return TD_FAILURE;
    }

    (td_void)memcpy_s(stat, sizeof(isp_stat_info), stat_info, sizeof(isp_stat_info));

    return TD_SUCCESS;
}

td_s32 isp_drv_stats_query_active(ot_vi_pipe vi_pipe, isp_stat_info *stat_info)
{
    td_s32 ret;
    isp_drv_ctx *drv_ctx = TD_NULL;
    isp_stat_info act_stat_info;
    unsigned long flags = 0;

    osal_spinlock *isp_spin_lock = TD_NULL;

    isp_check_pipe_return(vi_pipe);
    isp_check_pointer_return(stat_info);

    drv_ctx = isp_drv_get_ctx(vi_pipe);
    isp_spin_lock = isp_drv_get_lock(vi_pipe);
    osal_spin_lock_irqsave(isp_spin_lock, &flags);
    if (drv_ctx->statistics_buf.act_stat == TD_NULL) {
        osal_spin_unlock_irqrestore(isp_spin_lock, &flags);
        isp_warn_trace("Pipe[%d] get statistic active buffer err, stat not ready!\n", vi_pipe);
        return TD_FAILURE;
    }

    (td_void)memcpy_s(&act_stat_info, sizeof(isp_stat_info), drv_ctx->statistics_buf.act_stat, sizeof(isp_stat_info));
    osal_spin_unlock_irqrestore(isp_spin_lock, &flags);
    ret = ot_mmz_check_phys_addr(act_stat_info.phy_addr, sizeof(isp_stat));
    if (ret != TD_SUCCESS) {
        isp_err_trace("stat addr check error\n");
        return TD_FAILURE;
    }
    (td_void)memcpy_s(stat_info, sizeof(isp_stat_info), &act_stat_info, sizeof(isp_stat_info));

    return TD_SUCCESS;
}

