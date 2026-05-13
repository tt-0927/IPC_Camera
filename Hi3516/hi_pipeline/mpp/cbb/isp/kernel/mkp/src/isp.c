/*
  Copyright (c), 2001-2025, Shenshu Tech. Co., Ltd.
 */
#include "isp.h"
#include "dev_ext.h"
#include "ot_common.h"
#include "ot_common_isp.h"
#include "ot_common_sys.h"
#ifdef CONFIG_OT_SYS_KOL_SUPPORT
#include "ot_common_syskol.h"
#endif
#include "ot_i2c.h"
#include "ot_spi.h"
#include "ot_osal.h"
#include "isp_drv.h"
#include "isp_drv_define.h"
#ifdef CONFIG_OT_PROC_SHOW_SUPPORT
#include "isp_drv_proc.h"
#endif
#include "isp_drv_vreg.h"
#include "isp_drv_stats.h"
#include "isp_ext.h"
#include "isp_be_buf.h"
#include "isp_ioctl.h"
#include "mkp_isp.h"
#include "mm_ext.h"
#include "mod_ext.h"
#include "proc_ext.h"
#include "securec.h"
#include "sys_ext.h"
#include "isp_drv_dfx.h"
#include "isp_mem_share.h"
#include "ot_math.h"
#define CALC_SNS_AVE_CNT 1000
#define CALC_ISP_INT_AVG_CNT 1000

/* MACRO DEFINITION */
td_void isp_irq_route(ot_vi_pipe vi_pipe, td_u32 int_num);
int isp_drv_int_bottom_half(int irq);

#define UPDATE_POS_DEFAULT      0    /* 0: frame start; 1: frame end */
#define INT_TIME_OUT_DEFAULT    200  /* inter timeout 200 */
#define STAT_INTVL_DEFAULT      1
#define PROC_PARAM_DEFAULT      30   /* proc update interval num 30 */
#define PORT_INY_DELAY_DEFAULT  0    /* Port intertupt delay value */
#ifdef CONFIG_OT_AIBNR_SUPPORT
#define BE_BUF_NUM_DEFAULT  8    /* buf num 8 */
#else
#define BE_BUF_NUM_DEFAULT  4    /* buf num 4 */
#endif

#ifdef CONFIG_OT_VI_DISTRIBUTE_GRP
#define DIST_FPS_WAIT_CNT  20
#endif

/*  GLOBAL VARIABLES */
static isp_drv_ctx           g_isp_drv_ctx[OT_ISP_MAX_PIPE_NUM] = {{0}};

static osal_spinlock g_isp_lock[OT_ISP_MAX_PIPE_NUM];
static osal_spinlock g_isp_sync_lock[OT_ISP_MAX_PIPE_NUM];
#ifdef CONFIG_OT_ISP_DRC_SWITCH_SUPPORT
static osal_spinlock g_isp_drc_lock[OT_ISP_MAX_PIPE_NUM];
#endif
/* ldci temporal filter enable */
static td_bool               g_ldci_tpr_flt_en[OT_ISP_MAX_PIPE_NUM] = { [0 ...(OT_ISP_MAX_PIPE_NUM - 1)] = TD_FALSE };

/* ISP ModParam info */
td_u32 g_pwm_number[OT_ISP_MAX_PIPE_NUM] = { [0 ...(OT_ISP_MAX_PIPE_NUM - 1)] = PWM_CHN_IDX_DEFAULT };
td_u32 g_update_pos[OT_ISP_MAX_PIPE_NUM] = { [0 ...(OT_ISP_MAX_PIPE_NUM - 1)] = UPDATE_POS_DEFAULT };

/* The time(unit:ms) of interrupt timeout */
td_u32  g_int_timeout[OT_ISP_MAX_PIPE_NUM] = { [0 ...(OT_ISP_MAX_PIPE_NUM - 1)] = INT_TIME_OUT_DEFAULT };

/* update isp statistic information per stat-intval frame, purpose to reduce CPU load */
td_u32  g_stat_intvl[OT_ISP_MAX_PIPE_NUM] = { [0 ...(OT_ISP_MAX_PIPE_NUM - 1)] = STAT_INTVL_DEFAULT };

/* 0: close proc; n: write proc info's interval int num */
td_u32  g_proc_param[OT_ISP_MAX_PIPE_NUM] = { [0 ...(OT_ISP_MAX_PIPE_NUM - 1)] = PROC_PARAM_DEFAULT };
td_u32  g_port_int_delay[OT_ISP_MAX_PIPE_NUM] = { PORT_INY_DELAY_DEFAULT };
td_u8   g_be_buf_num[OT_ISP_MAX_PIPE_NUM] = { [0 ...(OT_ISP_MAX_PIPE_NUM - 1)] = BE_BUF_NUM_DEFAULT };
ot_isp_ob_stats_update_pos g_ob_stats_update_pos[OT_ISP_MAX_PIPE_NUM] = { [0 ...(OT_ISP_MAX_PIPE_NUM - 1)] = 0 };
ot_isp_alg_run_select g_isp_alg_run_select[OT_ISP_MAX_PIPE_NUM] =  { [0 ...(OT_ISP_MAX_PIPE_NUM - 1)] = 0 };
ot_isp_run_wakeup_select g_isp_run_wakeup_sel[OT_ISP_MAX_PIPE_NUM] = { [0 ...(OT_ISP_MAX_PIPE_NUM - 1)] = 0 };

td_bool g_int_bottom_half = TD_FALSE; /* 1 to enable interrupt processing at bottom half */

/* 1 to enable interrupt processing at bottom half */
td_bool g_quick_start[OT_ISP_MAX_PIPE_NUM] = { [0 ...(OT_ISP_MAX_PIPE_NUM - 1)] = TD_FALSE };

/* 1 : enable long frame  pipe interrupt */
td_bool g_long_frm_int_en[OT_ISP_MAX_PIPE_NUM] = { [0 ...(OT_ISP_MAX_PIPE_NUM - 1)] = TD_FALSE };

td_bool g_use_bottom_half = TD_FALSE; /* 1 to use interrupt processing at bottom half */

td_bool g_isp_bottom_half_support = TD_FALSE;

static td_bool g_mask_is_open[OT_ISP_MAX_PIPE_NUM] = { [0 ...(OT_ISP_MAX_PIPE_NUM - 1)] = TD_FALSE };
#ifdef CONFIG_OT_ISP_LITEOS_BOTTOM_HALF_SUPPORT
isp_work_queue_ctx g_isp_work_queue_ctx;
#endif

td_bool isp_drv_check_no_fe_pipe(ot_vi_pipe pipe)
{
#ifdef CONFIG_OT_VI_DISTRIBUTE_GRP
    isp_drv_ctx *ctx = TD_NULL;
    if (pipe < OT_ISP_MAX_PIPE_NUM && pipe >= OT_ISP_MAX_FE_PIPE_NUM) {
        ctx = isp_drv_get_ctx(pipe);
        return ctx->dist_attr.distribute_en == TD_TRUE ? TD_FALSE : TD_TRUE;
    }
#else
    if (pipe < OT_ISP_MAX_PIPE_NUM && pipe >= OT_ISP_MAX_FE_PIPE_NUM) {
        return TD_TRUE;
    }
#endif

    return TD_FALSE;
}

td_void isp_drv_dist_trans_pipe(ot_vi_pipe *pipe)
{
#ifdef CONFIG_OT_VI_DISTRIBUTE_GRP
    isp_drv_ctx *ctx = TD_NULL;
    isp_check_pointer_void_return(pipe);

    ctx = isp_drv_get_ctx(*pipe);
    if (ctx->dist_attr.distribute_en == TD_TRUE) {
        *pipe = ctx->dist_attr.pipe_id[0];
    }

#else
    ot_unused(pipe);
#endif
}


td_s32 isp_drv_check_pipe(ot_vi_pipe vi_pipe, const char *func, const td_s32 line)
{
    if ((vi_pipe < 0) || (vi_pipe >= OT_ISP_MAX_PIPE_NUM)) {
        isp_err_trace_ext(func, line, "Err isp pipe %d!\n", vi_pipe);
        return OT_ERR_ISP_ILLEGAL_PARAM;
    }

    return TD_SUCCESS;
}

td_s32 isp_drv_check_pointer(td_void *ptr, const char *func, const td_s32 line)
{
    if (ptr == TD_NULL) {
        isp_err_trace_ext(func, line, "Null Pointer!\n");
        return OT_ERR_ISP_NULL_PTR;
    }

    return TD_SUCCESS;
}

td_s32 isp_drv_check_bool(td_bool value, const char *func, const td_s32 line)
{
    if ((value != TD_TRUE) && (value != TD_FALSE)) {
        isp_err_trace_ext(func, line, "Invalid ISP Bool Type %d!\n", value);
        return OT_ERR_ISP_ILLEGAL_PARAM;
    }

    return TD_SUCCESS;
}

static td_bool isp_is_online_mode(isp_running_mode running_mode)
{
    /* online mode not support */
    if ((is_online_mode(running_mode)) || is_pre_online_post_offline(running_mode)) {
        return TD_TRUE;
    }

    return TD_FALSE;
}

static __inline td_bool isp_bottom_half_is_enable(td_void)
{
    if (g_isp_bottom_half_support && g_use_bottom_half) {
        return TD_TRUE;
    }
    return TD_FALSE;
}

isp_drv_ctx *isp_drv_get_ctx(ot_vi_pipe vi_pipe)
{
    return &g_isp_drv_ctx[vi_pipe];
}

ot_vi_pipe isp_drv_get_pipe_id(const isp_drv_ctx *ctx)
{
    ot_vi_pipe id;

    for (id = 0; id < OT_ISP_MAX_PIPE_NUM; id++) {
        if (&g_isp_drv_ctx[id] == ctx) {
            return id;
        }
    }

    return id = MIN2(id, OT_ISP_MAX_PIPE_NUM - 1);
}

osal_spinlock *isp_drv_get_lock(ot_vi_pipe vi_pipe)
{
    return &g_isp_lock[vi_pipe];
}
#ifdef CONFIG_OT_VI_STITCH_GRP
osal_spinlock *isp_drv_get_sync_lock(ot_vi_pipe vi_pipe)
{
    return &g_isp_sync_lock[vi_pipe];
}
#endif

#ifdef CONFIG_OT_ISP_DRC_SWITCH_SUPPORT
osal_spinlock *isp_drv_get_drc_lock(ot_vi_pipe vi_pipe)
{
    return &g_isp_drc_lock[vi_pipe];
}
#endif
td_bool isp_drv_get_ldci_tpr_flt_en(ot_vi_pipe vi_pipe)
{
    return g_ldci_tpr_flt_en[vi_pipe];
}

td_u32 isp_drv_get_update_pos(ot_vi_pipe vi_pipe)
{
    return g_update_pos[vi_pipe];
}
#ifdef CONFIG_OT_ISP_DYNAMIC_BLC_SUPPORT
ot_isp_ob_stats_update_pos isp_drv_get_ob_stats_update_pos(ot_vi_pipe vi_pipe)
{
    return g_ob_stats_update_pos[vi_pipe];
}
#endif
ot_isp_alg_run_select isp_drv_get_alg_run_select(ot_vi_pipe vi_pipe)
{
    return g_isp_alg_run_select[vi_pipe];
}

ot_isp_run_wakeup_select isp_drv_get_run_wakeup_sel(ot_vi_pipe vi_pipe)
{
    return g_isp_run_wakeup_sel[vi_pipe];
}

td_u8 isp_drv_get_be_buf_num(ot_vi_pipe vi_pipe)
{
    return g_be_buf_num[vi_pipe];
}

td_u32 isp_drv_get_int_timeout(ot_vi_pipe vi_pipe)
{
    return g_int_timeout[vi_pipe];
}

td_u32 isp_drv_get_proc_param(ot_vi_pipe vi_pipe)
{
    return g_proc_param[vi_pipe];
}

td_u32 isp_drv_get_pwm_number(ot_vi_pipe vi_pipe)
{
    return g_pwm_number[vi_pipe];
}

static td_void isp_drv_slave_pipe_int_enable(ot_vi_pipe vi_pipe, td_bool en)
{
    isp_drv_ctx *drv_ctx = TD_NULL;
    td_u8 k;
    td_u8 chn_num_max;
    ot_vi_pipe vi_pipe_bind;
    drv_ctx = isp_drv_get_ctx(vi_pipe);
    if (g_long_frm_int_en[vi_pipe] == TD_FALSE) {
        return;
    }
    if (is_fs_wdr_mode(drv_ctx->sync_cfg.wdr_mode) == TD_FALSE) {
        return;
    }

    chn_num_max = clip3(drv_ctx->wdr_attr.pipe_num, 1, ISP_WDR_CHN_MAX);
    if (en) {
        for (k = 0; k < chn_num_max; k++) {
            vi_pipe_bind = drv_ctx->wdr_attr.pipe_id[k];
            if (vi_pipe_bind == vi_pipe) {
                continue;
            }
            if (((vi_pipe_bind) < 0) || ((vi_pipe_bind) >= OT_ISP_MAX_FE_PIPE_NUM)) {
                continue;
            }
            io_rw_fe_address(vi_pipe_bind, ISP_INT_FE) = 0xff;
            io_rw_fe_address(vi_pipe_bind, ISP_INT_FE_MASK) |= ISP_INT_FE_FSTART;
            io_rw_fe_address(vi_pipe_bind, ISP_INT_FE_MASK) |= ISP_INT_FE_FEND;
            io_rw_fe_address(vi_pipe_bind, ISP_INT_FE_MASK) |= ISP_INT_FE_DYNABLC_END;
        }
    } else {
        for (k = 0; k < chn_num_max; k++) {
            vi_pipe_bind = drv_ctx->wdr_attr.pipe_id[k];
            if (vi_pipe_bind == vi_pipe) {
                continue;
            }
            if (((vi_pipe_bind) < 0) || ((vi_pipe_bind) >= OT_ISP_MAX_FE_PIPE_NUM)) {
                continue;
            }
            io_rw_fe_address(vi_pipe_bind, ISP_INT_FE) = 0xff;
            io_rw_fe_address(vi_pipe_bind, ISP_INT_FE_MASK) &= ~ISP_INT_FE_FSTART;
            io_rw_fe_address(vi_pipe_bind, ISP_INT_FE_MASK) &= ~ISP_INT_FE_FEND;
            io_rw_fe_address(vi_pipe_bind, ISP_INT_FE_MASK) &= ~ISP_INT_FE_DYNABLC_END;
        }
    }
}

static td_void isp_drv_set_frame_start_int_mask(ot_vi_pipe vi_pipe, td_bool en)
{
    isp_drv_ctx *drv_ctx = isp_drv_get_ctx(vi_pipe);

    if (drv_ctx->work_mode.running_mode == ISP_MODE_RUNNING_PRE_ONLINE_POST_OFFLINE) {
#ifdef CONFIG_OT_VI_PREONLINE_POSTOFFLINE
        if (en) {
            isp_drv_set_pre_proc_fstart_int_mask();
        } else {
            isp_drv_clear_pre_proc_fstart_int_mask();
        }
#endif
    } else {
        if (is_no_fe_phy_pipe(vi_pipe) == TD_FALSE) {
            io_rw_fe_address(vi_pipe, ISP_INT_FE) = 0xFF;  /* clear interrupt */
        }
    }
}

td_s32 isp_drv_set_int_enable(ot_vi_pipe vi_pipe, td_bool en)
{
    td_s32 vi_dev;
    td_bool int0_mask_en = TD_TRUE;
    isp_drv_ctx *drv_ctx = TD_NULL;

    isp_check_pipe_return(vi_pipe);
    isp_check_bool_return(en);
    drv_ctx = isp_drv_get_ctx(vi_pipe);
    vi_dev = drv_ctx->wdr_attr.vi_dev;

    isp_check_vir_pipe_return(vi_pipe);

    if (en) {
        if (ckfn_vi_pipe_get_dev_irq_affinity()) {
            call_vi_pipe_get_dev_irq_affinity(vi_pipe, &int0_mask_en);
        }
        g_mask_is_open[vi_pipe] = TD_TRUE;
        if (is_full_wdr_mode(drv_ctx->wdr_cfg.wdr_mode) || is_half_wdr_mode(drv_ctx->wdr_cfg.wdr_mode)) {
            io_rw_pt_address(vi_pt_base(vi_dev) + VI_PT_INT_MASK) |= VI_PT_INT_FSTART;
            io_rw_pt_address(vi_pt_base(vi_dev) + VI_PT_FSTART_DLY) = g_port_int_delay[vi_pipe];
        }
        io_rw_pt_address(vi_pt_base(vi_dev) + VI_PT_INT_MASK) |= VI_PT_INT_ERR;
        if (is_no_fe_phy_pipe(vi_pipe) == TD_FALSE) {
            io_rw_fe_address(vi_pipe, ISP_INT_FE_MASK) |= ISP_INT_FE_FEND;
            io_rw_fe_address(vi_pipe, ISP_INT_FE_MASK) |= ISP_INT_FE_DYNABLC_END;
            io_rw_fe_address(vi_pipe, ISP_INT_FE_MASK) |= ISP_INT_FE_DELAY;
        }
        isp_drv_enbale_vicap_int_mask(vi_dev, vi_pipe, int0_mask_en);
    } else {
        if (is_full_wdr_mode(drv_ctx->wdr_cfg.wdr_mode) || is_half_wdr_mode(drv_ctx->wdr_cfg.wdr_mode)) {
            io_rw_pt_address(vi_pt_base(vi_dev) + VI_PT_INT_MASK) &= ~(VI_PT_INT_FSTART);
            io_rw_pt_address(vi_pt_base(vi_dev) + VI_PT_FSTART_DLY) = 0;
        }
        io_rw_pt_address(vi_pt_base(vi_dev) + VI_PT_INT) = 0xF;
        io_rw_pt_address(vi_pt_base(vi_dev) + VI_PT_INT_MASK) &= ~(VI_PT_INT_ERR);

        if (is_no_fe_phy_pipe(vi_pipe) == TD_FALSE) {
            io_rw_fe_address(vi_pipe, ISP_INT_FE_MASK) &= ~(ISP_INT_FE_FEND);
            io_rw_fe_address(vi_pipe, ISP_INT_FE_MASK) &= ~(ISP_INT_FE_DYNABLC_END);
            io_rw_fe_address(vi_pipe, ISP_INT_FE_MASK) &= ~(ISP_INT_FE_DELAY);
        }
        isp_drv_disable_vicap_int_mask(vi_dev, vi_pipe);

        g_mask_is_open[vi_pipe] = TD_FALSE;
    }
    isp_drv_set_frame_start_int_mask(vi_pipe, en);
    isp_drv_slave_pipe_int_enable(vi_pipe, en);

    return TD_SUCCESS;
}

td_s32 isp_drv_set_vicap_int_mask(ot_vi_pipe vi_pipe, td_bool int0_mask_en)
{
    isp_check_pipe_return(vi_pipe);
    isp_check_vir_pipe_return(vi_pipe);
    isp_check_bool_return(int0_mask_en);
    // when g_mask_is_open=TD_FALSE, isp_drv_set_int_enable to set mask
    if (g_mask_is_open[vi_pipe] == TD_TRUE) {
        isp_drv_change_vicap_int_mask(vi_pipe, int0_mask_en);
    }

    return TD_SUCCESS;
}

td_s32 isp_drv_get_frame_interrupt_attr(ot_vi_pipe vi_pipe, ot_frame_interrupt_attr *frame_int_attr)
{
    ot_vi_pipe fe_phy_pipe = vi_pipe;
    isp_drv_dist_trans_pipe(&fe_phy_pipe);

    if (!ckfn_vi_get_vi_frame_interrupt_attr()) {
        return TD_FAILURE;
    }

    return call_vi_get_vi_frame_interrupt_attr(fe_phy_pipe, frame_int_attr);
}

td_s32 isp_drv_get_pipe_low_delay_en(ot_vi_pipe vi_pipe, td_bool *pipe_low_delay_en)
{
    ot_vi_pipe pipe = vi_pipe;
    isp_drv_dist_trans_pipe(&pipe);
    *pipe_low_delay_en = TD_FALSE;

    if (is_virt_pipe(pipe)) {
        return TD_SUCCESS;
    }

    if (!ckfn_vi_get_vi_pipe_low_delay_en()) {
        return TD_FAILURE;
    }

    return call_vi_get_vi_pipe_low_delay_en(pipe, pipe_low_delay_en);
}

static td_bool isp_drv_stitch_timing_err_correction(ot_vi_pipe cur_pipe, const isp_drv_ctx *drv_ctx)
{
#ifdef CONFIG_OT_VI_STITCH_GRP
    td_u64 cur_time;
    td_u64 time_diff;
    isp_drv_ctx *drv_ctx_main = TD_NULL;
    td_bool pipe_low_delay_en = TD_FALSE;
    isp_sync_cfg_buf_node *cur_node = TD_NULL;

    if (drv_ctx->stitch_attr.stitch_enable == TD_FALSE) {
        return TD_FALSE;
    }

    if (drv_ctx->stitch_attr.main_pipe == TD_TRUE) {
        return TD_FALSE;
    }

    if (drv_ctx->frame_int_attr.interrupt_type != OT_FRAME_INTERRUPT_START) {
        return TD_FALSE;
    }

    if (isp_drv_get_pipe_low_delay_en(cur_pipe, &pipe_low_delay_en) != TD_SUCCESS) {
        isp_warn_trace("ISP[%d] Get pipe_low_delay_en_attr failed !!!", cur_pipe);
    }

    if (pipe_low_delay_en) {
        return TD_FALSE;
    }

    drv_ctx_main = isp_drv_get_ctx(drv_ctx->stitch_attr.stitch_bind_id[0]);

    cur_time = call_sys_get_time_stamp();
    time_diff = abs_diff(cur_time, drv_ctx_main->sync_cfg.last_update_time);
    if (time_diff < ISP_STITCH_MAX_GAP) {
        // This branch represents that the main pipe of the current group of spliced images has been processed,
        // and then the secondary pipe is processed. There is no timing exception,
        // and fault tolerance correction is not required.
        return TD_FALSE;
    }

    // The following represents that the splicing scenario at this time has the risk of timing disorder.
    // Get the sync_id of BE from PIPE first.
    // The synchronization node parameters for the new set of frames are not configured at this time,
    // and it is expected that this node will be configured later when the main PIPE is processed,
    // and the stay_cnt of this node needs to be corrected to the original value +1.
    cur_node = drv_ctx_main->sync_cfg.node[0];
    if (cur_node == TD_NULL) {
        isp_err_trace("isp[%d] drv_ctx_main:%d cur_node is null\n", cur_pipe, drv_ctx->stitch_attr.stitch_bind_id[0]);
        return TD_FALSE;
    }
    isp_info_trace("vi_pipe %d, cur time %llu, cfg sns last update time %llu, force to change stay_cnt frome %d->%d\n",
        cur_pipe, cur_time, drv_ctx_main->sync_cfg.last_update_time, cur_node->stay_cnt, cur_node->stay_cnt+1);

    return TD_TRUE;
#else
    return TD_FALSE;
#endif
}

static td_bool isp_drv_exp_stitch_timing_err_correction(ot_vi_pipe cur_pipe, isp_drv_ctx *drv_ctx)
{
#ifdef CONFIG_OT_VI_STITCH_GRP
    td_u64 cur_time;
    td_u64 time_diff;
    isp_drv_ctx *drv_ctx_main = TD_NULL;
    isp_sync_cfg_buf_node *cur_node = TD_NULL;

    if (drv_ctx->stitch_attr.stitch_enable == TD_FALSE) {
        return TD_FALSE;
    }

    if (drv_ctx->stitch_attr.main_pipe == TD_TRUE) {
        return TD_FALSE;
    }

    if (drv_ctx->isp_sync_ctrl.first_flag == TD_FALSE) {
        return TD_FALSE;
    }
    drv_ctx->isp_sync_ctrl.first_flag = TD_FALSE;

    drv_ctx_main = isp_drv_get_ctx(drv_ctx->stitch_attr.stitch_bind_id[0]);

    cur_time = call_sys_get_time_stamp();
    time_diff = abs_diff(cur_time, drv_ctx_main->sync_cfg.last_update_time);
    if (time_diff < ISP_STITCH_MAX_GAP) {
        // This branch represents that the main pipe of the current group of spliced images has been processed,
        // and then the secondary pipe is processed. There is no timing exception,
        // and fault tolerance correction is not required.
        return TD_FALSE;
    }

    // The following represents the timing disorder risk in the splicing scenario at this time.
    // The sync_id of be is obtained from pipe first,
    // and the synchronization node parameters of the new set of frames are not configured.
    // It is expected that the node will be configured later when the main pipe processes,
    // and the stay_cnt of this node needs to be corrected to the original value +1.
    cur_node = drv_ctx_main->sync_cfg.node[0];
    if (cur_node == TD_NULL) {
        isp_err_trace("isp[%d] drv_ctx_main:%d cur_node is null\n", cur_pipe, drv_ctx->stitch_attr.stitch_bind_id[0]);
        return TD_FALSE;
    }
    isp_info_trace("vi_pipe %d, cur time %llu, cfg sns last update time %llu, force to change stay_cnt frome %d->%d\n",
        cur_pipe, cur_time, drv_ctx_main->sync_cfg.last_update_time, cur_node->stay_cnt, cur_node->stay_cnt+1);

    return TD_TRUE;
#else
    return TD_FALSE;
#endif
}

td_u32 isp_drv_get_dynamic_blc_sync_id(ot_vi_pipe vi_pipe)
{
    td_u32 blc_sync_id = 0;

    isp_drv_ctx *drv_ctx = isp_drv_get_ctx(vi_pipe);

    blc_sync_id = drv_ctx->dyna_blc_info.be_blc_sync[0].blc_id;

    return blc_sync_id;
}

#ifdef CONFIG_OT_SYS_KOL_SUPPORT
td_void isp_drv_get_single_mode(ot_vi_pipe vi_pipe, td_bool *single_mode)
{
    ot_syskol_param kol_param;

    if (ckfn_sys_query_kol_param()) {
        call_sys_query_kol_param(&kol_param);
    } else {
        *single_mode = TD_FALSE;
        return;
    }

    if (kol_param.capture_mode == OT_SYSKOL_CAPTURE_MODE_SINGLE) {
        *single_mode = TD_TRUE;
    } else {
        *single_mode = TD_FALSE;
    }

    return;
}
#endif

td_u32 isp_drv_cal_target_cfg_num(ot_vi_pipe vi_pipe, isp_drv_ctx *drv_ctx_main,
    td_bool current_frm, td_bool before_sns_cfg)
{
    td_u32 target_cfg_num = 0;
    td_bool distribute_en, pipe_low_delay_en, single_mode;
    td_s32 ret, delay_max;

    single_mode = TD_FALSE;
    delay_max = drv_ctx_main->sync_cfg.cfg2_vld_dly_max;
#ifdef CONFIG_OT_SYS_KOL_SUPPORT
    isp_drv_get_single_mode(vi_pipe, &single_mode);
#endif
    if (single_mode == TD_TRUE) {
        delay_max = 1;
    }

    distribute_en = TD_FALSE;
    pipe_low_delay_en = TD_FALSE;
#ifdef CONFIG_OT_VI_DISTRIBUTE_GRP
    distribute_en = drv_ctx_main->dist_attr.distribute_en;
#endif

    if (current_frm) {
        if (before_sns_cfg) {
            /* The sensor is configured one frame late */
            target_cfg_num = delay_max;
        } else {
            /* Send current frm, so plus 1 */
            target_cfg_num = delay_max + 1;
        }

        if (isp_bottom_half_is_enable() == TD_TRUE) {
            target_cfg_num += 1;
        }
    } else {
        /* The sensor is configured one frame in advance & vi send frame delay one frame, so plus 2 */
        target_cfg_num = delay_max + 2;
    }

    ret = isp_drv_get_pipe_low_delay_en(vi_pipe, &pipe_low_delay_en);
    if (ret != TD_SUCCESS) {
        isp_warn_trace("ISP[%d] Get pipe_low_delay_en_attr failed !!!", vi_pipe);
    }
    if (distribute_en == TD_TRUE &&
        (pipe_low_delay_en == TD_FALSE || drv_ctx_main->frame_int_attr.interrupt_type == OT_FRAME_INTERRUPT_START)) {
        target_cfg_num--;
    }

    return target_cfg_num;
}

td_s32 isp_drv_get_be_sync_id(ot_vi_pipe vi_pipe, td_bool current_frm, td_bool before_sns_cfg)
{
    td_s32 sync_id = -1;
    td_u32 target_cfg_num;
    td_u32 total_cfg_num = 0;
    td_u32 stay_cnt;
    unsigned long flags = 0;
    td_u32 idx = 0;
    isp_drv_ctx *drv_ctx = isp_drv_get_ctx(vi_pipe);
    isp_drv_ctx *drv_ctx_main = drv_ctx;
    ot_vi_pipe vi_pipe_main = vi_pipe;
    isp_sync_cfg_buf_node *cur_node = TD_NULL;

    if (drv_ctx->isp_init == TD_FALSE) {
        return sync_id;
    }

    if ((drv_ctx->stitch_attr.stitch_enable == TD_TRUE) && (drv_ctx->stitch_attr.main_pipe == TD_FALSE)) {
#ifdef CONFIG_OT_VI_STITCH_GRP
        vi_pipe_main = drv_ctx->stitch_attr.stitch_bind_id[0];
        drv_ctx_main = isp_drv_get_ctx(vi_pipe_main);
#endif
    }

    osal_spin_lock_irqsave(&g_isp_sync_lock[vi_pipe_main], &flags);
    target_cfg_num = isp_drv_cal_target_cfg_num(vi_pipe_main, drv_ctx_main, current_frm, before_sns_cfg);

    while (idx <= CFG2VLD_DLY_LIMIT) {
        cur_node = drv_ctx_main->sync_cfg.node[idx];
        if (cur_node == TD_NULL) {
            goto exit;
        }
        if ((idx == 0) && (isp_drv_stitch_timing_err_correction(vi_pipe, drv_ctx) == TD_TRUE)) {
            stay_cnt = cur_node->stay_cnt + 1;
        } else {
            stay_cnt = cur_node->stay_cnt;
        }

        total_cfg_num += stay_cnt;
        if (total_cfg_num >= target_cfg_num) {
            sync_id = cur_node->unique_id;
            goto exit;
        }

        idx++;
    }

exit:
    osal_spin_unlock_irqrestore(&g_isp_sync_lock[vi_pipe_main], &flags);

    isp_info_trace("vi_pipe = %d, get be sync node[%d] unique id = %d\n", vi_pipe, idx, sync_id);

    return sync_id;
}

td_u8 isp_drv_get_block_num(ot_vi_pipe vi_pipe)
{
    return g_isp_drv_ctx[vi_pipe].work_mode.block_num;
}

static td_void isp_drv_stat_buf_fe_insert(ot_vi_pipe vi_pipe, isp_drv_ctx *drv_ctx)
{
    drv_ctx->frame_cnt++;

    if (drv_ctx->frame_cnt % div_0_to_1(g_stat_intvl[vi_pipe]) == 0) {
        isp_drv_fe_stats_buf_busy_put(vi_pipe);
    }
}

static td_void isp_drv_stat_buf_summary_insert(ot_vi_pipe vi_pipe, isp_drv_ctx *drv_ctx)
{
    if (drv_ctx->frame_cnt % div_0_to_1(g_stat_intvl[vi_pipe]) == 0) {
        isp_drv_stats_buf_busy_put(vi_pipe);
    }
}

osal_spinlock *isp_drv_get_spin_lock(ot_vi_pipe vi_pipe)
{
    ot_vi_pipe main_pipe;
    isp_drv_ctx *drv_ctx = TD_NULL;

    drv_ctx = isp_drv_get_ctx(vi_pipe);
    if (drv_ctx->stitch_attr.stitch_enable != TD_TRUE) {
        return &g_isp_lock[vi_pipe];
    } else {
        main_pipe = drv_ctx->stitch_attr.stitch_bind_id[0];
        return &g_isp_sync_lock[main_pipe];
    }
}

static td_s32 isp_drv_get_sync_ctrl_info(ot_vi_pipe vi_pipe, isp_sync_cfg *sync_cfg)
{
    td_s32 vi_dev;
    isp_drv_ctx *drv_ctx = TD_NULL;
    isp_sync_cfg_buf_node *cur_node = TD_NULL;

    isp_check_pipe_return(vi_pipe);
    isp_check_pointer_return(sync_cfg);

    drv_ctx = isp_drv_get_ctx(vi_pipe);
    vi_dev = drv_ctx->wdr_attr.vi_dev;

    sync_cfg->vc_num = (io_rw_pt_address(vi_pt_base(vi_dev) + VI_PT0_ID) & 0x30) >> 4; /* right shift 4 */

    if (sync_cfg->vc_num_max == 0) {
        sync_cfg->vc_num = 0;
    }

    if (sync_cfg->vc_num > sync_cfg->vc_num_max) {
        isp_err_trace("err VC number(%d), can't be large than VC total(%d)!\n", sync_cfg->vc_num, sync_cfg->vc_num_max);
    }

    /* get Cfg2VldDlyMAX */
    if (!isp_sync_buf_is_empty(&sync_cfg->sync_cfg_buf)) {
        cur_node = &sync_cfg->sync_cfg_buf.sync_cfg_buf_node[sync_cfg->sync_cfg_buf.buf_rd_flag];

        if (cur_node != TD_NULL) {
            if (cur_node->valid) {
                sync_cfg->cfg2_vld_dly_max = cur_node->sns_regs_info.cfg2_valid_delay_max;
            }
        }
    }

    if ((sync_cfg->cfg2_vld_dly_max > CFG2VLD_DLY_LIMIT) || (sync_cfg->cfg2_vld_dly_max < 1)) {
        isp_warn_trace("Delay of config to valid is:0x%x\n", sync_cfg->cfg2_vld_dly_max);
        sync_cfg->cfg2_vld_dly_max = 1;
    }

    sync_cfg->vc_cfg_num =
        (sync_cfg->vc_num + sync_cfg->vc_num_max * sync_cfg->cfg2_vld_dly_max) % div_0_to_1(sync_cfg->vc_num_max + 1);

    return TD_SUCCESS;
}

static td_void isp_drv_update_exp_ratio(isp_sync_cfg *sync_cfg, isp_sync_cfg_buf_node *cur_node, td_u64 *exp,
    td_u32 len)
{
    td_u32 i, j;
    td_u64 tmp;
    td_u64 ratio;

    for (j = 0; j < OT_ISP_EXP_RATIO_NUM && j + 1 < len; j++) {
        for (i = CFG2VLD_DLY_LIMIT - 1; i >= 1; i--) {
            sync_cfg->exp_ratio[j][i] = sync_cfg->exp_ratio[j][i - 1];
        }

        ratio = exp[j + 1];
        tmp = exp[j];
        tmp = div_0_to_1(tmp);

        while (ratio > (0x1LL << 25) || tmp > (0x1LL << 25)) { /* left shift 25 */
            tmp >>= 1;
            ratio >>= 1;
        }

        ratio = (ratio * cur_node->ae_reg_cfg.wdr_gain[j + 1]) << WDR_EXP_RATIO_SHIFT;
        tmp = (tmp * cur_node->ae_reg_cfg.wdr_gain[j]);

        while (tmp > (0x1LL << 31)) { /* left shift 31 */
            tmp >>= 1;
            ratio >>= 1;
        }

        ratio = osal_div64_u64(ratio, div_0_to_1(tmp));
        sync_cfg->exp_ratio[j][0] = (td_u32)ratio;
    }

    for (i = CFG2VLD_DLY_LIMIT - 1; i >= 1; i--) {
        sync_cfg->lf_mode[i] = sync_cfg->lf_mode[i - 1];
    }

    if ((is_line_wdr_mode(sync_cfg->wdr_mode)) && (cur_node->ae_reg_cfg.fs_wdr_mode == OT_ISP_FSWDR_LONG_FRAME_MODE)) {
        for (j = 0; j < OT_ISP_EXP_RATIO_NUM; j++) {
            sync_cfg->exp_ratio[j][0] = 0x40;
        }

        sync_cfg->lf_mode[0] = OT_ISP_FSWDR_LONG_FRAME_MODE;
    } else if ((is_line_wdr_mode(sync_cfg->wdr_mode)) &&
        (cur_node->ae_reg_cfg.fs_wdr_mode == OT_ISP_FSWDR_AUTO_LONG_FRAME_MODE)) {
        for (j = 0; j < OT_ISP_EXP_RATIO_NUM; j++) {
            sync_cfg->exp_ratio[j][0] = (sync_cfg->exp_ratio[j][0] < 0x45) ? 0x40 : sync_cfg->exp_ratio[j][0];
        }

        sync_cfg->lf_mode[0] = OT_ISP_FSWDR_AUTO_LONG_FRAME_MODE;
    } else {
        sync_cfg->lf_mode[0] = OT_ISP_FSWDR_NORMAL_MODE;
    }
}

static td_void isp_drv_sync_wdr_gain(isp_sync_cfg *sync_cfg, isp_sync_cfg_buf_node *cur_node)
{
    td_u32 i, j;

    for (j = 0; j < OT_ISP_EXP_RATIO_NUM; j++) {
        for (i = CFG2VLD_DLY_LIMIT - 1; i >= 1; i--) {
            sync_cfg->wdr_gain[j][i] = sync_cfg->wdr_gain[j][i - 1];
        }
        sync_cfg->wdr_gain[j][0] = cur_node->ae_reg_cfg.wdr_gain[j];
    }

    for (i = CFG2VLD_DLY_LIMIT - 1; i >= 1; i--) {
        sync_cfg->wdr_gain[3][i] = sync_cfg->wdr_gain[3][i - 1]; /* array index 3 */
    }
    sync_cfg->wdr_gain[3][0] = 0x100; /* array index 3 */
}

static td_void isp_drv_calc_exp_ratio(isp_sync_cfg *sync_cfg, isp_sync_cfg_buf_node *cur_node,
    const isp_sync_cfg_buf_node *pre_node, const isp_sns_gain *sns_gain)
{
    td_u32 i;
    td_u64 exp[OT_ISP_WDR_MAX_FRAME_NUM] = { 0 };
    isp_vc_num vc_cfg_num;

    vc_cfg_num = (isp_vc_num)sync_cfg->vc_cfg_num;

    for (i = 0; i < OT_ISP_WDR_MAX_FRAME_NUM; i++) {
        exp[i] = cur_node->ae_reg_cfg.int_time[i] * sns_gain->cur_sns_gain[0];
    }

    if (is_full_wdr_mode(sync_cfg->wdr_mode)) {
        switch (vc_cfg_num) {
            case ISP_VC_NUM_VS:
                exp[1] = pre_node->ae_reg_cfg.int_time[1] * sns_gain->pre_sns_gain[0];
                exp[2] = pre_node->ae_reg_cfg.int_time[2] * sns_gain->pre_sns_gain[0]; /* array index 2 */
                exp[3] = pre_node->ae_reg_cfg.int_time[3] * sns_gain->pre_sns_gain[0]; /* array index 3 */
                break;

            case ISP_VC_NUM_S:
                exp[2] = pre_node->ae_reg_cfg.int_time[2] * sns_gain->pre_sns_gain[0]; /* array index 2 */
                exp[3] = pre_node->ae_reg_cfg.int_time[3] * sns_gain->pre_sns_gain[0]; /* array index 3 */
                break;

            case ISP_VC_NUM_M:
                exp[3] = pre_node->ae_reg_cfg.int_time[3] * sns_gain->pre_sns_gain[0]; /* array index 3 */
                break;

            default:
            case ISP_VC_NUM_L:
                break;
        }
    } else if (is_2to1_wdr_mode(sync_cfg->wdr_mode)) {
        exp[1] = cur_node->ae_reg_cfg.int_time[1] * sns_gain->cur_sns_gain[1];
    }

    isp_drv_update_exp_ratio(sync_cfg, cur_node, exp, OT_ISP_WDR_MAX_FRAME_NUM);
}

static td_u32 isp_drv_calc_drc_comp(isp_sync_cfg *sync_cfg, const isp_sync_cfg_buf_node *cur_node,
    const isp_sync_cfg_buf_node *pre_node)
{
    td_u64 tmp, cur_exp, pre_exp;

    if (is_2to1_wdr_mode(sync_cfg->wdr_mode)) {
        cur_exp = cur_node->ae_reg_cfg.exposure_sf * 0x100;
        pre_exp = pre_node->ae_reg_cfg.exposure_sf * 0x100;
    } else {
        cur_exp = cur_node->ae_reg_cfg.exposure;
        pre_exp = pre_node->ae_reg_cfg.exposure;
        cur_exp = cur_exp * cur_node->ae_reg_cfg.wdr_gain[0];
        pre_exp = pre_exp * pre_node->ae_reg_cfg.wdr_gain[0];
    }
    while (cur_exp > (0x1LL << 31) || pre_exp > (0x1LL << 31)) { /* left shift 31 */
        cur_exp >>= 1;
        pre_exp >>= 1;
    }

    cur_exp = cur_exp << DRC_COMP_SHIFT;
    tmp = div_0_to_1(pre_exp);

    while (tmp > (0x1LL << 31)) { /* left shift 31 */
        tmp >>= 1;
        cur_exp >>= 1;
    }

    cur_exp = osal_div64_u64(cur_exp, tmp);

    return (td_u32)cur_exp;
}

static td_void isp_drv_sync_drc_comp(isp_sync_cfg *sync_cfg, const isp_sync_cfg_buf_node *cur_node,
    const isp_sync_cfg_buf_node *pre_node)
{
    td_u32 i;

    for (i = CFG2VLD_DLY_LIMIT - 1; i >= 1; i--) {
        sync_cfg->drc_comp[i] = sync_cfg->drc_comp[i - 1];
    }

    sync_cfg->drc_comp[0] = isp_drv_calc_drc_comp(sync_cfg, cur_node, pre_node);

    return;
}

static td_void isp_dfx_update_sync_cfg_gap(isp_drv_ctx *drv_ctx)
{
    isp_drv_dbg_info *dbg_info = &drv_ctx->drv_dbg_info;
    td_u64 cur_time = 0;
    td_u64 gap = 0;

    if (ckfn_sys_entry() && ckfn_sys_get_time_stamp()) {
        cur_time = call_sys_get_time_stamp();
    }

    if (dbg_info->sync_cfg_last_time) {
        gap = abs_diff(cur_time, dbg_info->sync_cfg_last_time);

        dbg_info->sync_cfg_gap = (td_u32)gap;
        dbg_info->sync_cfg_gap_max = max(dbg_info->sync_cfg_gap, dbg_info->sync_cfg_gap_max);
        dbg_info->sync_cfg_gap_min = min(dbg_info->sync_cfg_gap, dbg_info->sync_cfg_gap_min);
    } else {
        dbg_info->sync_cfg_gap_max = 0;
        dbg_info->sync_cfg_gap_min = 0xffffffff;
    }

    dbg_info->sync_cfg_last_time = cur_time;
}

static td_void isp_drv_sync_node_init(ot_vi_pipe vi_pipe, isp_sync_cfg *sync_cfg, isp_sync_cfg_buf_node *node,
    td_bool old_node)
{
    isp_drv_ctx *drv_ctx = TD_NULL;

    if (node == TD_NULL) {
        return;
    }

    drv_ctx = isp_drv_get_ctx(vi_pipe);
    isp_dfx_update_sync_cfg_gap(drv_ctx);

    if (old_node == TD_TRUE) {
        return;
    }

    node->stay_cnt = 0;

    if ((drv_ctx->stitch_attr.stitch_enable == TD_TRUE) && (drv_ctx->stitch_attr.main_pipe == TD_FALSE)) {
#ifdef CONFIG_OT_VI_STITCH_GRP
        /* concatenating a non-primary pipe directly uses the unique-id
           of the frame corresponding to the primary pipe */
        isp_drv_ctx *drv_ctx_main = isp_drv_get_ctx(drv_ctx->stitch_attr.stitch_bind_id[0]);
        isp_sync_cfg_buf_node *node0 = drv_ctx_main->sync_cfg.node[0];
        node->unique_id = (node0 != TD_NULL) ? node0->unique_id : -1;
#endif
    } else {
        node->unique_id = sync_cfg->total_cnt;
        sync_cfg->total_cnt++;
        if (sync_cfg->total_cnt > RUN_BE_SYNC_ID_MAX) {
            sync_cfg->total_cnt = RUN_BE_SYNC_ID_MIN;
        }
    }
}

static td_void isp_drv_sensor_regs_force_to_update(isp_sync_cfg_buf_node *cur_node)
{
    td_u32 i;

    if (cur_node->sns_regs_info.sns_type == OT_ISP_SNS_TYPE_I2C) {
        for (i = 0; i < cur_node->sns_regs_info.reg_num; i++) {
            cur_node->sns_regs_info.i2c_data[i].update = TD_TRUE;
        }
    } else if (cur_node->sns_regs_info.sns_type == OT_ISP_SNS_TYPE_SSP) {
        for (i = 0; i < cur_node->sns_regs_info.reg_num; i++) {
            cur_node->sns_regs_info.ssp_data[i].update = TD_TRUE;
        }
    }

    cur_node->sns_regs_info.slv_sync.update = TD_TRUE;
}

static td_s32 isp_drv_get_sync_cfg_node(ot_vi_pipe vi_pipe, isp_sync_cfg *sync_cfg,
    isp_sync_cfg_buf_node **cur_node_point, isp_sync_cfg_buf_node **pre_node_point)
{
    td_bool err = TD_FALSE;
    td_bool empty = TD_FALSE;
    td_u32 i;
    isp_sync_cfg_buf_node *cur_node = TD_NULL;
    isp_sync_cfg_buf_node *pre_node = TD_NULL;
    isp_sync_cfg_buf_node *node1 = TD_NULL;

    /* update node when VCCfgNum is 0 */
    if (sync_cfg->vc_cfg_num == 0) {
        for (i = CFG2VLD_DLY_LIMIT; i >= 1; i--) {
            sync_cfg->node[i] = sync_cfg->node[i - 1];
        }

        /* avoid skip effective AE results */
        if (isp_sync_buf_is_err(&sync_cfg->sync_cfg_buf)) {
            err = TD_TRUE;
        } else if (isp_sync_buf_is_empty(&sync_cfg->sync_cfg_buf) == TD_TRUE) {
            empty = TD_TRUE;
        }

        /* read the newest information */
        isp_sync_buf_read2(&sync_cfg->sync_cfg_buf, &sync_cfg->node[0]);

        isp_drv_sync_node_init(vi_pipe, sync_cfg, sync_cfg->node[0], empty);
    }

    cur_node = sync_cfg->node[0];
    if (cur_node == TD_NULL) {
        return TD_FAILURE;
    }
    if (cur_node->valid == TD_FALSE) {
        return TD_FAILURE;
    }

    if (err == TD_TRUE) {
        isp_drv_sensor_regs_force_to_update(cur_node);
    }

    pre_node = cur_node;
    node1 = sync_cfg->node[1];
    if (node1 != TD_NULL) {
        if (node1->valid == TD_TRUE) {
            pre_node = node1;
        }
    } else {
        (td_void)memcpy_s(&sync_cfg->node_pre, sizeof(sync_cfg->node_pre),
            cur_node, sizeof(isp_sync_cfg_buf_node));
    }

    *cur_node_point = cur_node;
    *pre_node_point = pre_node;

    return TD_SUCCESS;
}

static td_void isp_drv_calc_sns_gain(isp_sync_cfg *sync_cfg, isp_sync_cfg_buf_node *cur_node,
    isp_sync_cfg_buf_node *pre_node, isp_sns_gain *sns_gain)
{
    td_u64 isp_total_gain;
    td_u64 *cur_gain = sns_gain->cur_sns_gain;
    td_u64 *pre_gain = sns_gain->pre_sns_gain;

    if (is_2to1_wdr_mode(sync_cfg->wdr_mode)) {
        if (sync_cfg->vc_cfg_num == 0) {
            isp_total_gain = (td_u64)pre_node->ae_reg_cfg.isp_dgain * pre_node->ae_reg_cfg.wdr_gain[0];
            isp_total_gain = div_0_to_1(isp_total_gain >> ISP_GAIN_SHIFT);
            pre_gain[0] = pre_node->ae_reg_cfg.exposure_sf;
            pre_gain[0] = osal_div_u64(pre_gain[0], div_0_to_1(pre_node->ae_reg_cfg.int_time[0]));
            pre_gain[0] = osal_div_u64(pre_gain[0] << ISP_GAIN_SHIFT, isp_total_gain);

            isp_total_gain = (td_u64)cur_node->ae_reg_cfg.isp_dgain * cur_node->ae_reg_cfg.wdr_gain[0];
            isp_total_gain = div_0_to_1(isp_total_gain >> ISP_GAIN_SHIFT);
            cur_gain[0] = cur_node->ae_reg_cfg.exposure_sf;
            cur_gain[0] = osal_div_u64(cur_gain[0], div_0_to_1(cur_node->ae_reg_cfg.int_time[0]));
            cur_gain[0] = osal_div_u64(cur_gain[0] << ISP_GAIN_SHIFT, isp_total_gain);

            isp_total_gain = (td_u64)pre_node->ae_reg_cfg.isp_dgain * pre_node->ae_reg_cfg.wdr_gain[1];
            isp_total_gain = div_0_to_1(isp_total_gain >> ISP_GAIN_SHIFT);
            pre_gain[1] = pre_node->ae_reg_cfg.exposure;
            pre_gain[1] = osal_div_u64(pre_gain[1], div_0_to_1(pre_node->ae_reg_cfg.int_time[1]));
            pre_gain[1] = osal_div_u64(pre_gain[1] << ISP_GAIN_SHIFT, isp_total_gain);

            isp_total_gain = (td_u64)cur_node->ae_reg_cfg.isp_dgain * cur_node->ae_reg_cfg.wdr_gain[1];
            isp_total_gain = div_0_to_1(isp_total_gain >> ISP_GAIN_SHIFT);
            cur_gain[1] = cur_node->ae_reg_cfg.exposure;
            cur_gain[1] = osal_div_u64(cur_gain[1], div_0_to_1(cur_node->ae_reg_cfg.int_time[1]));
            cur_gain[1] = osal_div_u64(cur_gain[1] << ISP_GAIN_SHIFT, isp_total_gain);

            sync_cfg->pre_sns_gain_sf = pre_gain[0];
            sync_cfg->cur_sns_gain_sf = cur_gain[0];
            sync_cfg->pre_sns_gain = pre_gain[1];
            sync_cfg->cur_sns_gain = cur_gain[1];
        }

        pre_gain[0] = sync_cfg->pre_sns_gain_sf;
        cur_gain[0] = sync_cfg->cur_sns_gain_sf;
        pre_gain[1] = sync_cfg->pre_sns_gain;
        cur_gain[1] = sync_cfg->cur_sns_gain;
    } else {
        if (sync_cfg->vc_cfg_num == 0) {
            pre_gain[0] = pre_node->ae_reg_cfg.exposure;
            pre_gain[0] = osal_div_u64(pre_gain[0], div_0_to_1(pre_node->ae_reg_cfg.int_time[0]));
            pre_gain[0] = osal_div_u64(pre_gain[0] << ISP_GAIN_SHIFT, div_0_to_1(pre_node->ae_reg_cfg.isp_dgain));

            cur_gain[0] = cur_node->ae_reg_cfg.exposure;
            cur_gain[0] = osal_div_u64(cur_gain[0], div_0_to_1(cur_node->ae_reg_cfg.int_time[0]));
            cur_gain[0] = osal_div_u64(cur_gain[0] << ISP_GAIN_SHIFT, div_0_to_1(cur_node->ae_reg_cfg.isp_dgain));

            sync_cfg->pre_sns_gain = pre_gain[0];
            sync_cfg->cur_sns_gain = cur_gain[0];
        }

        pre_gain[0] = sync_cfg->pre_sns_gain;
        cur_gain[0] = sync_cfg->cur_sns_gain;
    }
}

static td_s32 isp_drv_calc_sync_cfg(ot_vi_pipe vi_pipe, isp_sync_cfg *sync_cfg)
{
    td_s32 ret;
    td_u32 i;
    isp_sync_cfg_buf_node *cur_node = TD_NULL;
    isp_sync_cfg_buf_node *pre_node = TD_NULL;
    isp_sns_gain sns_gain = {0};
    isp_check_pointer_return(sync_cfg);

    ret = isp_drv_get_sync_cfg_node(vi_pipe, sync_cfg, &cur_node, &pre_node);
    if (ret != TD_SUCCESS) {
        return TD_SUCCESS;
    }

    isp_drv_sync_wdr_gain(sync_cfg, cur_node);

    if (g_isp_run_wakeup_sel[vi_pipe] == OT_ISP_RUN_WAKEUP_BE_END) {
        return TD_SUCCESS;
    }

    isp_drv_calc_sns_gain(sync_cfg, cur_node, pre_node, &sns_gain);

    /* calculate exposure ratio */
    isp_drv_calc_exp_ratio(sync_cfg, cur_node, pre_node, &sns_gain);

    /* calculate AlgProc */
    if (is_line_wdr_mode(sync_cfg->wdr_mode)) {
        for (i = CFG2VLD_DLY_LIMIT - 1; i >= 1; i--) {
            sync_cfg->alg_proc[i] = sync_cfg->alg_proc[i - 1];
        }
        sync_cfg->alg_proc[0] = cur_node->wdr_reg_cfg.wdr_mdt_en;
    }

    /* calculate DRC compensation */
    if (sync_cfg->vc_cfg_num == 0) {
        isp_drv_sync_drc_comp(sync_cfg, cur_node, pre_node);
    }

    return TD_SUCCESS;
}

static td_u8 isp_drv_get_fe_sync_index_typical(ot_vi_pipe vi_pipe, const isp_drv_ctx *drv_ctx)
{
    return (drv_ctx->sync_cfg.cfg2_vld_dly_max - 1) / div_0_to_1(drv_ctx->sync_cfg.vc_num_max + 1);
}

static td_u8 isp_drv_get_fe_sync_index_specify(ot_vi_pipe vi_pipe, const isp_drv_ctx *drv_ctx, td_bool is_first)
{
    td_u32 target_cfg_num;
    td_u32 total_cfg_num = 0;
    td_u8 delay;
    td_u8 idx = 0;
    isp_sync_cfg_buf_node *cur_node = TD_NULL;
    td_u8 index = isp_drv_get_fe_sync_index_typical(vi_pipe, drv_ctx);

    delay = (is_first == TD_TRUE) ? 1 : 0;
    target_cfg_num = drv_ctx->sync_cfg.cfg2_vld_dly_max - delay;

    while (idx <= CFG2VLD_DLY_LIMIT) {
        cur_node = drv_ctx->sync_cfg.node[idx];
        if (cur_node == TD_NULL) {
            goto exit;
        }
        total_cfg_num += cur_node->stay_cnt;
        if (total_cfg_num >= target_cfg_num) {
            index = idx;
            goto exit;
        }
        idx++;
    }

exit:
    return index;
}

static td_u8 isp_drv_get_exp_dist_sync_index_specify(ot_vi_pipe vi_pipe, isp_drv_ctx *drv_ctx, td_bool is_first)
{
    td_u32 target_cfg_num;
    td_u32 total_cfg_num = 0;
    td_u8 delay;
    td_u8 idx = 0;
    td_u32 stay_cnt;
    td_u8 index = isp_drv_get_fe_sync_index_typical(vi_pipe, drv_ctx);
    isp_sync_cfg_buf_node *cur_node = TD_NULL;

    delay = (is_first == TD_TRUE) ? 1 : 0;
    target_cfg_num = drv_ctx->sync_cfg.cfg2_vld_dly_max - delay;
    while (idx <= CFG2VLD_DLY_LIMIT) {
        cur_node = drv_ctx->sync_cfg.node[idx];
        if (cur_node == TD_NULL) {
            goto exit;
        }
        if ((idx == 0) && (isp_drv_exp_stitch_timing_err_correction(vi_pipe, drv_ctx) == TD_TRUE)) {
            stay_cnt = cur_node->stay_cnt + 1;
        } else {
            stay_cnt = cur_node->stay_cnt;
        }

        total_cfg_num += stay_cnt;
        if (total_cfg_num >= target_cfg_num) {
            index = idx;
            goto exit;
        }
        idx++;
    }

exit:
    return index;
}

td_u8 isp_drv_get_fe_sync_index(ot_vi_pipe vi_pipe, const isp_drv_ctx *drv_ctx, td_bool is_first)
{
    td_u8 index;

    if (g_isp_run_wakeup_sel[vi_pipe] == OT_ISP_RUN_WAKEUP_BE_END) {
        index = isp_drv_get_fe_sync_index_specify(vi_pipe, drv_ctx, is_first);
    } else {
        index = isp_drv_get_fe_sync_index_typical(vi_pipe, drv_ctx);
    }

    return index;
}

static td_void isp_drv_init_index(const isp_drv_ctx *drv_ctx, td_u8 *index)
{
    isp_sync_cfg_buf_node *node = drv_ctx->sync_cfg.node[0];
    if (node != TD_NULL) {
        if (((node->sns_regs_info.sns_type == OT_ISP_SNS_TYPE_I2C) &&
            (node->sns_regs_info.com_bus.i2c_dev == -1)) ||
            ((node->sns_regs_info.sns_type == OT_ISP_SNS_TYPE_SSP) &&
            (node->sns_regs_info.com_bus.ssp_dev.bit4_ssp_dev == -1))) {
            *index = 0;
        }
    }
}

static td_u8 isp_drv_get_index(ot_vi_pipe vi_pipe, const isp_drv_ctx *drv_ctx, const td_bool low_delay_en)
{
    td_u8 index, delay_max;
    ot_frame_interrupt_type interrupt_type = drv_ctx->frame_int_attr.interrupt_type;
    td_bool single_mode;

    single_mode = TD_FALSE;
    delay_max = drv_ctx->sync_cfg.cfg2_vld_dly_max;
#ifdef CONFIG_OT_SYS_KOL_SUPPORT
    isp_drv_get_single_mode(vi_pipe, &single_mode);
#endif

    if (single_mode == TD_TRUE) {
        delay_max = 1;
    }

    if (((interrupt_type == OT_FRAME_INTERRUPT_START) || (interrupt_type == OT_FRAME_INTERRUPT_EARLY)) &&
        (low_delay_en == TD_FALSE)) {
        if (is_half_wdr_mode(drv_ctx->sync_cfg.wdr_mode)) {
            index = delay_max;
        } else {
            index = delay_max + 1;
        }
    } else {
        if (is_half_wdr_mode(drv_ctx->sync_cfg.wdr_mode)) {
            index = delay_max - 1;
        } else {
            index = delay_max;
        }
    }

    if (isp_bottom_half_is_enable() && drv_ctx->run_once_flag != TD_TRUE) {
        if (index == 0) {
            index = CFG2VLD_DLY_LIMIT - 1;
        } else {
            index = index - 1;
        }
    }

    if (((low_delay_en == TD_TRUE) && (isp_bottom_half_is_enable() == TD_TRUE)) ||
        ((low_delay_en == TD_FALSE) && (isp_bottom_half_is_enable() == TD_TRUE) &&
        (interrupt_type == OT_FRAME_INTERRUPT_EARLY_END))) {
        index = index + 1;
    }

    isp_drv_init_index(drv_ctx, &index);

    return index;
}

static td_u8 isp_drv_get_be_sync_index_typical(ot_vi_pipe vi_pipe, const isp_drv_ctx *drv_ctx)
{
    td_u8 index;
    td_s32 ret;
    td_bool pipe_low_delay_en = TD_FALSE;
    td_bool distribute_en = TD_FALSE;

#ifdef CONFIG_OT_VI_DISTRIBUTE_GRP
    distribute_en = drv_ctx->dist_attr.distribute_en;
#endif

    if (is_online_mode(drv_ctx->work_mode.running_mode)) {
        index = (drv_ctx->sync_cfg.cfg2_vld_dly_max - 1) / div_0_to_1(drv_ctx->sync_cfg.vc_num_max + 1);
    } else {
        ret = isp_drv_get_pipe_low_delay_en(vi_pipe, &pipe_low_delay_en);
        if (ret != TD_SUCCESS) {
            isp_warn_trace("ISP[%d] Get pipe_low_delay_en_attr failed !!!", vi_pipe);
        }
        index = isp_drv_get_index(vi_pipe, drv_ctx, pipe_low_delay_en);
    }
    if (distribute_en == TD_TRUE) {
        index = index - 1;
    }

    return index;
}

static td_void isp_dfx_sync_index_show(ot_vi_pipe vi_pipe, const isp_drv_ctx *drv_ctx)
{
    td_u8 i;
    isp_sync_cfg_buf_node *node = TD_NULL;
    for (i = 0; i < CFG2VLD_DLY_LIMIT; i++) {
        node = drv_ctx->sync_cfg.node[i];
        if (node != TD_NULL) {
            isp_err_trace("vi_pipe = %d i = %d unique_id = %d, stay_cnt = %d\n",
                vi_pipe, i, node->unique_id, node->stay_cnt);
        }
    }
}

static td_u8 isp_drv_get_be_sync_index_specify(ot_vi_pipe vi_pipe, const isp_drv_ctx *drv_ctx)
{
    td_u8 i;
    td_s32 sync_unique_id = drv_ctx->sync_cfg.cur_sync_id;
    isp_sync_cfg_buf_node *cur_node = TD_NULL;
    td_bool distribute_en = TD_FALSE;
#ifdef CONFIG_OT_LOG_TRACE_SUPPORT
    td_u64 cur_time = -1ULL;
#endif

#ifdef CONFIG_OT_VI_DISTRIBUTE_GRP
    distribute_en = drv_ctx->dist_attr.distribute_en;
#endif
    if (is_virt_pipe(vi_pipe) == TD_TRUE && distribute_en == TD_FALSE) {
        return 0;
    }

    if (sync_unique_id == -1) {
        isp_warn_trace("vi_pipe = %d, sync_unique_id = %d, force get sync index typical!\n", vi_pipe, sync_unique_id);
        goto err;
    }

#ifdef CONFIG_OT_LOG_TRACE_SUPPORT
    if (ckfn_sys_entry() && ckfn_sys_get_time_stamp()) {
        cur_time = call_sys_get_time_stamp();
    }
#endif

    for (i = 0; i <= CFG2VLD_DLY_LIMIT; i++) {
        cur_node = drv_ctx->sync_cfg.node[i];
        if (cur_node != TD_NULL && cur_node->unique_id == sync_unique_id) {
            isp_info_trace("vi_pipe = %d, target sync unique_id = %d, find match index = %d, pts = %llu, "
                "cur time = %llu, pts diff = %llu us\n",
                vi_pipe, sync_unique_id, i, drv_ctx->sync_cfg.cur_pts, cur_time,
                cur_time - drv_ctx->sync_cfg.cur_pts);
            return i;
        }
    }

    if (i == CFG2VLD_DLY_LIMIT + 1) {
        isp_err_trace("vi_pipe = %d, target sync unique_id = %d, can't find match index! pts = %llu, "
            "cur time = %llu, pts diff = %llu us\n",
            vi_pipe, sync_unique_id, drv_ctx->sync_cfg.cur_pts, cur_time,
            cur_time - drv_ctx->sync_cfg.cur_pts);
        isp_dfx_sync_index_show(vi_pipe, drv_ctx);
    }

err:
    return isp_drv_get_be_sync_index_typical(vi_pipe, drv_ctx);
}

td_u8 isp_drv_get_be_sync_index(ot_vi_pipe vi_pipe, const isp_drv_ctx *drv_ctx)
{
    td_u8 index;

    if (g_isp_run_wakeup_sel[vi_pipe] == OT_ISP_RUN_WAKEUP_BE_END) {
        index = isp_drv_get_be_sync_index_specify(vi_pipe, drv_ctx);
    } else {
        index = isp_drv_get_be_sync_index_typical(vi_pipe, drv_ctx);
    }

    return clip_max(index, CFG2VLD_DLY_LIMIT - 1);
}

td_u8 isp_drv_get_pre_be_sync_index(ot_vi_pipe vi_pipe, const isp_drv_ctx *drv_ctx)
{
    td_u8 index;
    if (drv_ctx == TD_NULL) {
        return 0;
    }
    if (is_pre_online_post_offline(drv_ctx->work_mode.running_mode)) {
        index = isp_drv_get_fe_sync_index(vi_pipe, drv_ctx, TD_FALSE);
    } else {
        index = isp_drv_get_be_sync_index(vi_pipe, drv_ctx);
    }

    return index;
}

td_u8 isp_get_drc_comp_sync_index(ot_vi_pipe vi_pipe, const isp_drv_ctx *drv_ctx)
{
    td_u8 index;

    if (g_isp_run_wakeup_sel[vi_pipe] == OT_ISP_RUN_WAKEUP_BE_END) {
        index = 0;
    } else {
        index = isp_drv_get_be_sync_index_typical(vi_pipe, drv_ctx);
    }

    return clip_max(index, CFG2VLD_DLY_LIMIT - 1);
}

static td_u8 isp_get_exp_ratio_sync_index(ot_vi_pipe vi_pipe)
{
    td_u8 sync_index;
    isp_drv_ctx *drv_ctx = TD_NULL;
    drv_ctx = isp_drv_get_ctx(vi_pipe);

    sync_index = isp_drv_get_pre_be_sync_index(vi_pipe, drv_ctx);
    sync_index = MIN2(sync_index, CFG2VLD_DLY_LIMIT - 1);

    if (is_half_wdr_mode(drv_ctx->sync_cfg.wdr_mode)) {
        if (is_offline_mode(drv_ctx->work_mode.running_mode) || is_striping_mode(drv_ctx->work_mode.running_mode)) {
            sync_index += 0x2;
            sync_index = MIN2(sync_index, CFG2VLD_DLY_LIMIT - 1);
        }
    }

    return sync_index;
}

static td_u32 isp_get_sensor_cfg_index(ot_vi_pipe vi_pipe, const isp_drv_ctx *drv_ctx)
{
    td_u8 i, index;
    isp_sync_cfg_buf_node *cur_node = TD_NULL;
    isp_sync_cfg_buf_node *next_node = TD_NULL;
    cur_node = drv_ctx->sync_cfg.node[0];
    if (g_isp_run_wakeup_sel[vi_pipe] == OT_ISP_RUN_WAKEUP_FE_START ||
        (cur_node == TD_NULL) || (cur_node->stay_cnt != 0)) {
        return 0;
    }

    for (i = 0; i < CFG2VLD_DLY_LIMIT - 1; i++) {
        cur_node = drv_ctx->sync_cfg.node[i];
        next_node = drv_ctx->sync_cfg.node[i + 1];
        if ((cur_node != TD_NULL) && (next_node == TD_NULL)) {
            break;
        } else if ((cur_node != TD_NULL) && (cur_node->stay_cnt == 0) &&
            (next_node != TD_NULL) && (next_node->stay_cnt != 0)) {
            break;
        }
    }

    index = i;
    if (index == CFG2VLD_DLY_LIMIT - 1) {
        isp_warn_trace("sensor config id(%d) is too large, may be something wrong!\n", index);
    }

    return clip_max(index, CFG2VLD_DLY_LIMIT - 1);
}

static td_void isp_drv_get_exp_ratio(ot_vi_pipe vi_pipe, const isp_drv_ctx *drv_ctx,
    td_u32 *ratio, td_u32 length)
{
    td_u8 exp_ratio_index;
    td_u32 i;
    isp_sync_cfg_buf_node *node0 = TD_NULL;

    if (g_isp_run_wakeup_sel[vi_pipe] == OT_ISP_RUN_WAKEUP_FE_START) {
        exp_ratio_index = isp_get_exp_ratio_sync_index(vi_pipe);
    } else {
        exp_ratio_index = 0;
    }
    if (is_wdr_mode(drv_ctx->sync_cfg.wdr_mode)) {
        node0 = drv_ctx->sync_cfg.node[0];
        if (node0 != TD_NULL) {
            if (((node0->sns_regs_info.sns_type == OT_ISP_SNS_TYPE_I2C) &&
                (node0->sns_regs_info.com_bus.i2c_dev == -1)) ||
                ((node0->sns_regs_info.sns_type == OT_ISP_SNS_TYPE_SSP) &&
                (node0->sns_regs_info.com_bus.ssp_dev.bit4_ssp_dev == -1))) {
                exp_ratio_index = 0;
            }
        }

        for (i = 0; i < length; i++) {
            ratio[i] = drv_ctx->sync_cfg.exp_ratio[i][exp_ratio_index];
        }
    } else {
    }

    /* when the data of sensor built-in WDR after decompand is 16bit, the ratio value is as follow. */
    if (is_built_in_wdr_mode(drv_ctx->sync_cfg.wdr_mode)) {
        ratio[0] = BUILT_IN_WDR_RATIO_VS_S;
        ratio[1] = BUILT_IN_WDR_RATIO_S_M;
        ratio[2] = BUILT_IN_WDR_RATIO_M_L; /* array index 2 */
    }

    for (i = 0; i < length; i++) {
        ratio[i] = clip3(ratio[i], EXP_RATIO_MIN, EXP_RATIO_MAX);
    }
}

static td_u8 isp_drv_get_cfg_node_vc(const isp_drv_ctx *drv_ctx)
{
    td_u8 cfg_node_vc;

    if (is_wdr_mode(drv_ctx->sync_cfg.wdr_mode)) {
        if (is_online_mode(drv_ctx->work_mode.running_mode)) {
            cfg_node_vc = (drv_ctx->sync_cfg.cfg2_vld_dly_max - 1) % div_0_to_1(drv_ctx->sync_cfg.vc_num_max + 1);
        } else {
            cfg_node_vc = (drv_ctx->sync_cfg.cfg2_vld_dly_max + 1) % div_0_to_1(drv_ctx->sync_cfg.vc_num_max + 1);
        }
    } else {
        cfg_node_vc = 0;
    }

    return cfg_node_vc;
}
#ifdef CONFIG_OT_SNAP_SUPPORT
static td_bool isp_drv_snap_get_pictrue_pipe(isp_drv_ctx *drv_ctx)
{
    ot_vi_pipe picture_pipe;
    isp_drv_ctx *drv_ctx_pic = TD_NULL;
    isp_running_mode picture_running_mode;
    td_bool online_have_pictrue_pipe = TD_FALSE;

    isp_check_pointer_return(drv_ctx);
    if (drv_ctx->snap_attr.picture_pipe_id >= 0) {
        picture_pipe = drv_ctx->snap_attr.picture_pipe_id;
        drv_ctx_pic = isp_drv_get_ctx(picture_pipe);
        picture_running_mode = drv_ctx_pic->work_mode.running_mode;

        if (is_online_mode(picture_running_mode)) {
            if (picture_pipe != drv_ctx->snap_attr.preview_pipe_id) {
                online_have_pictrue_pipe = TD_TRUE;
            }
        }
    }

    return online_have_pictrue_pipe;
}
#endif

static td_void isp_drv_reg_config_be_dgain(ot_vi_pipe vi_pipe, isp_drv_ctx *drv_ctx, isp_ae_reg_cfg_2 *ae_reg_cfg)
{
    td_u32 isp_dgain;
    td_s32 i;

    isp_dgain = isp_drv_isp_dgain_blc_compensation(ae_reg_cfg->isp_dgain, drv_ctx->be_sync_para.be_blc.dg_blc.blc[1]);
    isp_dgain = clip3(isp_dgain, ISP_DIGITAL_GAIN_MIN, ISP_DIGITAL_GAIN_MAX);

    for (i = 0; i < OT_ISP_BAYER_CHN_NUM; i++) {
        drv_ctx->be_sync_para.isp_dgain[i] = isp_dgain;
    }
}

static td_void isp_drv_reg_config_sync_4dgain(ot_vi_pipe vi_pipe, isp_drv_ctx *drv_ctx)
{
    td_s32 i;
    td_u8 sync_index;
    isp_sync_4dgain_cfg sync_4dgain_cfg;
    td_u32  wdr_gain[OT_ISP_WDR_MAX_FRAME_NUM] = { 0x100, 0x100, 0x100, 0x100 };
    sync_index = isp_get_exp_ratio_sync_index(vi_pipe);
    sync_index = MIN2(sync_index, CFG2VLD_DLY_LIMIT - 1);

    for (i = 0; i < OT_ISP_WDR_MAX_FRAME_NUM; i++) {
        wdr_gain[i] = drv_ctx->sync_cfg.wdr_gain[i][sync_index];
        wdr_gain[i] = clip3(wdr_gain[i], ISP_DIGITAL_GAIN_MIN, ISP_DIGITAL_GAIN_MAX);
        sync_4dgain_cfg.wdr_gain[i] = wdr_gain[i];
    }

    isp_drv_reg_config_4dgain(vi_pipe, drv_ctx, &sync_4dgain_cfg);
}

static td_void isp_drv_reg_config_sync_piris(ot_vi_pipe vi_pipe, isp_drv_ctx *drv_ctx, isp_ae_reg_cfg_2 *ae_reg_cfg)
{
    /* config Piris */
    if (ae_reg_cfg != TD_NULL) {
        if (ae_reg_cfg->piris_valid == TD_TRUE) {
            if (drv_ctx->piris_cb.pfn_piris_gpio_update != TD_NULL) {
                drv_ctx->piris_cb.pfn_piris_gpio_update(vi_pipe, &ae_reg_cfg->piris_pos);
            }
        }
    }
}

static td_s32 isp_drv_reg_config_sync(ot_vi_pipe vi_pipe, td_u8 cfg_node_idx, td_u8 cfg_node_vc)
{
    isp_ae_reg_cfg_2 *ae_reg_cfg = TD_NULL;
    isp_drv_ctx *drv_ctx = TD_NULL;
    isp_sync_cfg_buf_node *cfg_node = TD_NULL;

    isp_check_pipe_return(vi_pipe);

    drv_ctx = isp_drv_get_ctx(vi_pipe);
    if ((drv_ctx->stitch_attr.stitch_enable == TD_TRUE) && (drv_ctx->stitch_attr.main_pipe == TD_FALSE)) {
#ifdef CONFIG_OT_VI_STITCH_GRP
        ot_vi_pipe main_pipe = drv_ctx->stitch_attr.stitch_bind_id[0];
        isp_drv_ctx *drv_ctx_s = isp_drv_get_ctx(main_pipe);
        cfg_node = drv_ctx_s->sync_cfg.node[cfg_node_idx];
#endif
    } else {
        cfg_node = drv_ctx->sync_cfg.node[cfg_node_idx];
    }

    isp_check_pointer_return(cfg_node);

    isp_drv_reg_config_sync_awb_ccm(vi_pipe, drv_ctx, cfg_node_idx, cfg_node_vc);

    if (drv_ctx->sync_cfg.vc_cfg_num == cfg_node_vc) {
        ae_reg_cfg = &cfg_node->ae_reg_cfg;

        isp_drv_reg_config_be_dgain(vi_pipe, drv_ctx, ae_reg_cfg);

        isp_drv_reg_config_sync_4dgain(vi_pipe, drv_ctx);

        isp_drv_reg_config_sync_piris(vi_pipe, drv_ctx, ae_reg_cfg);
    }

    return TD_SUCCESS;
}

static td_void isp_drv_reg_config_isp_fe(ot_vi_pipe vi_pipe)
{
    isp_drv_ctx *drv_ctx = isp_drv_get_ctx(vi_pipe);

    isp_drv_reg_config_fe_blc(vi_pipe, drv_ctx);

    isp_drv_reg_config_fe_dgain(vi_pipe, drv_ctx);
}

static td_void isp_drv_sync_cfg_update_by_be_end(ot_vi_pipe vi_pipe, isp_drv_ctx *drv_ctx,
    td_u8 cur_idx, isp_sync_cfg_buf_node *cur_node)
{
    td_s32 i;
    td_u8 next_idx;
    isp_sns_gain sns_gain = {0};
    isp_sync_cfg_buf_node *pre_node = TD_NULL;
    isp_sync_cfg_buf_node *next_node = TD_NULL;
    isp_check_pointer_void_return(cur_node);

    pre_node = &drv_ctx->sync_cfg.node_pre;
    next_idx = clip3(cur_idx - 1, 0, CFG2VLD_DLY_LIMIT - 1);
    next_node = drv_ctx->sync_cfg.node[next_idx];
    next_node = (next_node == TD_NULL) ? cur_node : next_node;

    isp_drv_calc_sns_gain(&drv_ctx->sync_cfg, cur_node, pre_node, &sns_gain);

    /* calculate exposure ratio */
    isp_drv_calc_exp_ratio(&drv_ctx->sync_cfg, cur_node, pre_node, &sns_gain);

    /* calculate AlgProc */
    if (is_line_wdr_mode(drv_ctx->sync_cfg.wdr_mode)) {
        for (i = CFG2VLD_DLY_LIMIT - 1; i >= 1; i--) {
            drv_ctx->sync_cfg.alg_proc[i] = drv_ctx->sync_cfg.alg_proc[i - 1];
        }
        drv_ctx->sync_cfg.alg_proc[0] = cur_node->wdr_reg_cfg.wdr_mdt_en;
    }

    isp_drv_sync_drc_comp(&drv_ctx->sync_cfg, cur_node, pre_node);

    if (drv_ctx->sync_cfg.drc_comp_next != 0 && drv_ctx->sync_cfg.drc_comp_next != drv_ctx->sync_cfg.drc_comp[0]) {
        /* dfx: last ldci use drc comp is err! proc debug count plus 1 */
        drv_ctx->drv_dbg_info.ldci_comp_err_cnt++;
    }

    drv_ctx->sync_cfg.drc_comp_next = isp_drv_calc_drc_comp(&drv_ctx->sync_cfg, next_node, cur_node);

    (td_void)memcpy_s(pre_node, sizeof(isp_sync_cfg_buf_node), cur_node, sizeof(isp_sync_cfg_buf_node));
}

td_s32 isp_drv_reg_config_isp_be(ot_vi_pipe vi_pipe)
{
    td_s32 ret;
    td_u8 cfg_node_idx, cfg_node_idx_pre_be, cfg_node_vc;
    td_u32 ratio[OT_ISP_EXP_RATIO_NUM] = { 0x40 };
    isp_sync_cfg_buf_node *cfg_node = TD_NULL;
    isp_sync_cfg_buf_node *pre_be_cfg_node = TD_NULL;
    isp_drv_ctx *drv_ctx = isp_drv_get_ctx(vi_pipe);

    cfg_node_idx = MIN2(isp_drv_get_be_sync_index(vi_pipe, drv_ctx), CFG2VLD_DLY_LIMIT - 1);
    cfg_node_idx_pre_be = MIN2(isp_drv_get_pre_be_sync_index(vi_pipe, drv_ctx), CFG2VLD_DLY_LIMIT - 1);

    cfg_node_vc = isp_drv_get_cfg_node_vc(drv_ctx);

    if (is_wdr_mode(drv_ctx->sync_cfg.wdr_mode)) {
        /* Channel Switch */
        isp_drv_reg_config_chn_sel(vi_pipe, drv_ctx);
    }
#ifdef CONFIG_OT_ISP_DRC_SWITCH_SUPPORT
    isp_drv_update_drc_bypass(drv_ctx);
#endif
    cfg_node = drv_ctx->sync_cfg.node[cfg_node_idx];
    isp_check_pointer_success_return(cfg_node);

    pre_be_cfg_node = drv_ctx->sync_cfg.node[cfg_node_idx_pre_be];
    isp_check_pointer_success_return(pre_be_cfg_node);

    if (drv_ctx->sync_cfg.vc_cfg_num == cfg_node_vc) {
        if (g_isp_run_wakeup_sel[vi_pipe] == OT_ISP_RUN_WAKEUP_BE_END) {
            isp_drv_sync_cfg_update_by_be_end(vi_pipe, drv_ctx, cfg_node_idx, cfg_node);
        }
        isp_drv_get_exp_ratio(vi_pipe, drv_ctx, ratio, OT_ISP_EXP_RATIO_NUM);

        isp_drv_reg_config_be_blc(vi_pipe, drv_ctx, cfg_node, pre_be_cfg_node);

        isp_drv_reg_config_wdr(vi_pipe, &pre_be_cfg_node->wdr_reg_cfg, ratio, OT_ISP_EXP_RATIO_NUM);

        /* config Ldci compensation */
        isp_drv_reg_config_ldci(vi_pipe, drv_ctx);

        /* config drc strength */
        isp_drv_reg_config_drc(vi_pipe, &cfg_node->drc_reg_cfg);
    }
#ifdef CONFIG_OT_SNAP_SUPPORT
    /* online mode double pipe snap, when vipipe == preview_pipe_id, config picture_pipe */
    if ((isp_drv_snap_get_pictrue_pipe(drv_ctx) == TD_TRUE) && (vi_pipe == drv_ctx->snap_attr.picture_pipe_id)) {
        return TD_SUCCESS;
    }
#endif

    ret = isp_drv_reg_config_sync(vi_pipe, cfg_node_idx, cfg_node_vc);

    return ret;
}

td_s32 isp_drv_reg_config_isp(ot_vi_pipe vi_pipe, isp_drv_ctx *drv_ctx)
{
    isp_check_pipe_return(vi_pipe);

    isp_drv_reg_config_isp_fe(vi_pipe);

    return isp_drv_reg_config_isp_be(vi_pipe);
}

static td_bool check_sync_id_valid(ot_vi_pipe vi_pipe, const ot_isp_frame_info *isp_frame_info)
{
    isp_drv_ctx *drv_ctx = isp_drv_get_ctx(vi_pipe);

    if (isp_frame_info != TD_NULL) {
        if (isp_frame_info->sync_id >= RUN_BE_SYNC_ID_MIN && isp_frame_info->sync_id <= RUN_BE_SYNC_ID_MAX) {
            return TD_TRUE;
        } else {
            isp_warn_trace("vi_pipe = %d sync_id(%d) invalid!", vi_pipe, isp_frame_info->sync_id);
        }
    }
    isp_warn_trace("vi_pipe = %d isp_info_virt_addr is null", vi_pipe);

    drv_ctx->drv_dbg_info.sync_id_err_cnt++;

    return TD_FALSE;
}

td_void isp_drv_update_be_sync_para_by_id(ot_vi_pipe vi_pipe, const ot_video_frame_info *frame_info)
{
    td_s32 ret;
    isp_drv_ctx *drv_ctx = TD_NULL;
    ot_isp_frame_info *isp_frame_info = TD_NULL;
    td_s32 sync_id = -1;
    td_u32 blc_sync_id = 0;

    isp_check_pointer_void_return(frame_info);
    isp_check_pipe_void_return(vi_pipe);

    drv_ctx = isp_drv_get_ctx(vi_pipe);

    isp_frame_info = (ot_isp_frame_info *)frame_info->video_frame.supplement.isp_info_virt_addr;

    if (check_sync_id_valid(vi_pipe, isp_frame_info) == TD_TRUE) {
        sync_id = isp_frame_info->sync_id;
        blc_sync_id = isp_frame_info->blc_sync_id;
    }

    drv_ctx->sync_cfg.cur_sync_id = sync_id;
    drv_ctx->sync_cfg.cur_pts = frame_info->video_frame.pts;
    drv_ctx->dyna_blc_info.blc_sync_id = blc_sync_id;

    ret = isp_drv_reg_config_isp_be(vi_pipe);
#ifdef CONFIG_OT_ISP_DYNAMIC_BLC_SUPPORT
    if (drv_ctx->dyna_blc_info.sync_cfg.black_level_mode == OT_ISP_BLACK_LEVEL_MODE_DYNAMIC) {
        // according to blc_sync_id is valid or not to decide config be dynamic blc
        if (blc_sync_id >= RUN_BE_DYN_BLC_ID_MIN && blc_sync_id <= RUN_BE_DYN_BLC_ID_MAX) {
            isp_drv_reg_config_be_dynamic_blc_single_pipe(vi_pipe, drv_ctx);
        }
    }
#else
    ot_unused(blc_sync_id);
#endif
    isp_check_ret_continue(vi_pipe, ret, "isp_drv_reg_config_isp");
}

td_s32 isp_drv_get_be_sync_para(ot_vi_pipe vi_pipe, const ot_video_frame_info *frame_info,
    isp_be_sync_para *be_sync_para)
{
    isp_drv_ctx *drv_ctx = TD_NULL;
    unsigned long flags = 0;
    osal_spinlock *isp_spin_lock = TD_NULL;
    isp_check_pipe_return(vi_pipe);
    isp_check_pointer_return(be_sync_para);

    drv_ctx = isp_drv_get_ctx(vi_pipe);
    isp_spin_lock = isp_drv_get_lock(vi_pipe);

    if (drv_ctx->stitch_attr.stitch_enable == TD_TRUE) {
#ifdef CONFIG_OT_VI_STITCH_GRP
        if (g_isp_run_wakeup_sel[vi_pipe] == OT_ISP_RUN_WAKEUP_BE_END && frame_info != TD_NULL) {
            if (isp_drv_get_stitch_be_sync_para_specify(vi_pipe, be_sync_para, frame_info) != TD_SUCCESS) {
                return TD_FAILURE;
            }
        } else {
            if (isp_drv_get_stitch_be_sync_para(vi_pipe, be_sync_para) != TD_SUCCESS) {
                return TD_FAILURE;
            }
        }
#endif
    } else {
        if (g_isp_run_wakeup_sel[vi_pipe] == OT_ISP_RUN_WAKEUP_BE_END && frame_info != TD_NULL) {
            isp_drv_update_be_sync_para_by_id(vi_pipe, frame_info);
        }
        osal_spin_lock_irqsave(isp_spin_lock, &flags);
        (td_void)memcpy_s(be_sync_para, sizeof(isp_be_sync_para), &drv_ctx->be_sync_para, sizeof(isp_be_sync_para));
        osal_spin_unlock_irqrestore(isp_spin_lock, &flags);
    }
    return TD_SUCCESS;
}

isp_sync_cfg_buf_node *isp_drv_get_sns_cfg_node(const isp_drv_ctx *drv_ctx, td_u8 cur_node_idx, td_u8 delay_frm_num)
{
    td_u8 wdr_mode, cfg_node_idx, cfg_node_vc;
    isp_sync_cfg_buf_node *cfg_node = TD_NULL;
    isp_sync_cfg_buf_node *pre_cfg_node = TD_NULL;

    wdr_mode = drv_ctx->wdr_cfg.wdr_mode;
    cfg_node_idx = delay_frm_num / div_0_to_1(drv_ctx->sync_cfg.vc_num_max + 1);
    cfg_node_vc = delay_frm_num % div_0_to_1(drv_ctx->sync_cfg.vc_num_max + 1);
    if (drv_ctx->sync_cfg.vc_cfg_num == cfg_node_vc) {
        if (cur_node_idx + cfg_node_idx > CFG2VLD_DLY_LIMIT - 1) {
            return TD_NULL;
        }

        cfg_node = drv_ctx->sync_cfg.node[cur_node_idx + cfg_node_idx];
        pre_cfg_node = drv_ctx->sync_cfg.node[cur_node_idx + cfg_node_idx + 1];

        if (cfg_node == TD_NULL) {
            return TD_NULL;
        }

        /* not config sensor when cur == pre */
        if ((pre_cfg_node != TD_NULL) && (cfg_node == pre_cfg_node)) {
            if ((is_linear_mode(wdr_mode)) || (is_built_in_wdr_mode(wdr_mode))) {
                return TD_NULL;
            }
        }
    }

    return cfg_node;
}

td_bool is_sensor_data_has_been_config(isp_sync_cfg_buf_node *cfg_node, td_u32 idx, td_u8 stay_cnt)
{
    if (idx >= OT_ISP_MAX_SNS_REGS) {
        isp_err_trace("sync cfg node unique_id = %d, data_idx %d >= MAX_SNS_REGS(%d) invalid!\n",
            cfg_node->unique_id, idx, OT_ISP_MAX_SNS_REGS);
        return TD_TRUE;
    }

    if (stay_cnt > cfg_node->sns_regs_info.i2c_data[idx].delay_frame_num) {
        isp_warn_trace("cur node unique_id %d stay_cnt %d, config data[%d].delay_frame_num %d, already configed!\n",
            cfg_node->unique_id, stay_cnt, idx, cfg_node->sns_regs_info.i2c_data[idx].delay_frame_num);
        return TD_TRUE;
    }

    return TD_FALSE;
}

static td_s32 isp_drv_regs_cfg_sensor_check(isp_sync_cfg_buf_node *cur_node)
{
    if (cur_node == TD_NULL) {
        isp_warn_trace("NULL pointer!\n");
        return TD_FAILURE;
    }
    if (cur_node->valid == TD_FALSE) {
        isp_warn_trace("Invalid Node!\n");
        return TD_FAILURE;
    }

    if (cur_node->sns_regs_info.reg_num == 0) {
        isp_warn_trace("Err reg_num!\n");
        return TD_FAILURE;
    }

    return TD_SUCCESS;
}

#ifndef ISP_WRITE_I2C_THROUGH_MUL_REG
static td_void isp_drv_write_i2c_one_reg(isp_drv_ctx *drv_ctx, td_u8 cur_node_idx, td_u32 update_pos,
    td_s8 i2c_dev, td_u8 stay_cnt)
{
    td_u32 i;
    td_u8 cfg_node_id;
    isp_sync_cfg_buf_node *cur_node = drv_ctx->sync_cfg.node[cur_node_idx];
    isp_sync_cfg_buf_node *cfg_node = TD_NULL;
    ot_isp_i2c_data *i2c_data = TD_NULL;
    isp_check_pointer_void_return(cur_node);

    for (i = 0; i < cur_node->sns_regs_info.reg_num; i++) {
        if (is_sensor_data_has_been_config(cur_node, i, stay_cnt) == TD_TRUE) {
            continue;
        }

        cfg_node_id = cur_node->sns_regs_info.i2c_data[i].delay_frame_num - stay_cnt;
        cfg_node = isp_drv_get_sns_cfg_node(drv_ctx, cur_node_idx, cfg_node_id);
        if (cfg_node == TD_NULL) {
            continue;
        }

        i2c_data = &cfg_node->sns_regs_info.i2c_data[i];
        if (((i2c_data->update == TD_TRUE) && (update_pos == i2c_data->interrupt_pos)) ||
            drv_ctx->bottom_half_cross_frame == TD_TRUE) {
            drv_ctx->bus_cb.pfn_isp_write_i2c_data(i2c_dev, i2c_data->dev_addr, i2c_data->reg_addr,
                i2c_data->addr_byte_num, i2c_data->data, i2c_data->data_byte_num);
        }
    }
    drv_ctx->bottom_half_cross_frame = TD_FALSE;

    isp_dfx_sns_sync_cfg_show(drv_ctx, cur_node_idx, stay_cnt, update_pos);

    return;
}
#else
static td_void isp_drv_write_i2c_data_buf(ot_isp_i2c_data *i2c_data, td_s8 *tmp_buf, td_u32 array_size,
    td_u8 *index)
{
    td_u8 idx = *index;

    if (i2c_data->addr_byte_num == 1) { /* reg_addr config */
        if (idx >= array_size) {
            isp_err_trace("idx%d is larger than arry_size:%d\n", idx, array_size);
            goto exit;
        }
        tmp_buf[idx++] = i2c_data->reg_addr & 0xff;
    } else {
        if (idx >= array_size || (idx + 1) >= array_size) {
            isp_err_trace("idx:%d, idx or idx+1 is larger than arry_size:%d\n", idx, array_size);
            goto exit;
        }
        tmp_buf[idx++] = (i2c_data->reg_addr >> 8) & 0xff; /* reg-addr shift by 8 */
        tmp_buf[idx++] = i2c_data->reg_addr & 0xff;
    }
    if (i2c_data->data_byte_num == 1) { /* data config */
        if (idx >= array_size) {
            isp_err_trace("idx%d is larger than arry_size:%d\n", idx, array_size);
            goto exit;
        }
        tmp_buf[idx++] = i2c_data->data & 0xff;
    } else {
        if (idx >= array_size || (idx + 1) >= array_size) {
            isp_err_trace("idx:%d, idx or idx+1 is larger than arry_size:%d\n", idx, array_size);
            goto exit;
        }
        tmp_buf[idx++] = (i2c_data->data >> 8) & 0xff; /* reg addr shift by 8 */
        tmp_buf[idx++] = i2c_data->data & 0xff;
    }
exit:
    *index = idx;
}

static td_void isp_drv_write_i2c_mul_reg(isp_drv_ctx *drv_ctx, td_u8 cur_node_idx, td_u32 update_pos,
    td_s8 i2c_dev, td_u8 stay_cnt)
{
    td_u32 i;
    td_s8 tmp_buf[ISP_WRITE_I2C_MAX_REG_NUM];
    td_u8 idx = 0;
    td_u8 cfg_node_id, dev_addr, addr_byte_num, data_byte_num;
    isp_sync_cfg_buf_node *cfg_node = TD_NULL;
    isp_sync_cfg_buf_node *cur_node = drv_ctx->sync_cfg.node[cur_node_idx];
    ot_isp_i2c_data *i2c_data = TD_NULL;
    isp_check_pointer_void_return(cur_node);
    for (i = 0; i < cur_node->sns_regs_info.reg_num; i++) {
        if (is_sensor_data_has_been_config(cur_node, i, stay_cnt) == TD_TRUE) {
            continue;
        }

        cfg_node_id = cur_node->sns_regs_info.i2c_data[i].delay_frame_num - stay_cnt;
        cfg_node = isp_drv_get_sns_cfg_node(drv_ctx, cur_node_idx, cfg_node_id);
        if (cfg_node == TD_NULL) {
            continue;
        }
        i2c_data = &cfg_node->sns_regs_info.i2c_data[i];
        if (((i2c_data->update == TD_TRUE) && (update_pos == i2c_data->interrupt_pos)) ||
            drv_ctx->bottom_half_cross_frame == TD_TRUE) {
            dev_addr = i2c_data->dev_addr;
            addr_byte_num = i2c_data->addr_byte_num;
            data_byte_num = i2c_data->data_byte_num;
            if ((idx + addr_byte_num + data_byte_num) >= ISP_WRITE_I2C_MAX_REG_NUM) {
                drv_ctx->bus_cb.pfn_isp_write_i2c_data(i2c_dev, dev_addr, tmp_buf, sizeof(tmp_buf), idx, addr_byte_num,
                    data_byte_num);
                idx = 0;
            }
            isp_drv_write_i2c_data_buf(i2c_data, tmp_buf, ISP_WRITE_I2C_MAX_REG_NUM, &idx);
        }
    }
    drv_ctx->bottom_half_cross_frame = TD_FALSE;
    if (idx != 0) {
        drv_ctx->bus_cb.pfn_isp_write_i2c_data(i2c_dev, dev_addr, tmp_buf, sizeof(tmp_buf), idx, addr_byte_num,
            data_byte_num);
    }

    isp_dfx_sns_sync_cfg_show(drv_ctx, cur_node_idx, stay_cnt, update_pos);

    return;
}
#endif

td_s32 isp_drv_write_i2c_data(isp_drv_ctx *drv_ctx, td_u8 cur_node_idx, td_u32 update_pos, td_s8 i2c_dev,
    td_u8 stay_cnt)
{
    if (i2c_dev == -1) {
        return TD_SUCCESS;
    }
    if (drv_ctx->bus_cb.pfn_isp_write_i2c_data == TD_NULL) {
        isp_warn_trace("pfn_isp_write_i2c_data is TD_NULL point!\n");
        return TD_FAILURE;
    }

#ifndef ISP_WRITE_I2C_THROUGH_MUL_REG
    isp_drv_write_i2c_one_reg(drv_ctx, cur_node_idx, update_pos, i2c_dev, stay_cnt);
#else
    isp_drv_write_i2c_mul_reg(drv_ctx, cur_node_idx, update_pos, i2c_dev, stay_cnt);
#endif

    return TD_SUCCESS;
}

static td_s32 isp_drv_write_ssp_data(isp_drv_ctx *drv_ctx, td_u8 cur_node_idx, td_u32 update_pos,
    isp_sync_cfg_buf_node* input_cur_node, td_u8 stay_cnt)
{
    td_u32 i;
    td_u8 cfg_node_id;
    td_s8 ssp_dev, ssp_cs;
    isp_sync_cfg_buf_node *cur_node = drv_ctx->sync_cfg.node[cur_node_idx];
    isp_sync_cfg_buf_node *cfg_node = TD_NULL;
    ot_isp_ssp_data *ssp_data = TD_NULL;

    ssp_dev = input_cur_node->sns_regs_info.com_bus.ssp_dev.bit4_ssp_dev;
    ssp_cs = input_cur_node->sns_regs_info.com_bus.ssp_dev.bit4_ssp_cs;
    if (ssp_dev == -1) {
        return TD_SUCCESS;
    }

    if (drv_ctx->bus_cb.pfn_isp_write_ssp_data == TD_NULL) {
        isp_warn_trace("pfn_isp_write_ssp_data is TD_NULL point!\n");
        return TD_FAILURE;
    }

    for (i = 0; i < cur_node->sns_regs_info.reg_num; i++) {
        if (is_sensor_data_has_been_config(cur_node, i, stay_cnt) == TD_TRUE) {
            continue;
        }

        cfg_node_id = cur_node->sns_regs_info.ssp_data[i].delay_frame_num - stay_cnt;
        cfg_node = isp_drv_get_sns_cfg_node(drv_ctx, cur_node_idx, cfg_node_id);
        if (cfg_node == TD_NULL) {
            continue;
        }

        ssp_data = &cfg_node->sns_regs_info.ssp_data[i];
        if (((ssp_data->update == TD_TRUE) && (update_pos == ssp_data->interrupt_pos)) ||
            drv_ctx->bottom_half_cross_frame == TD_TRUE) {
            drv_ctx->bus_cb.pfn_isp_write_ssp_data(ssp_dev, ssp_cs, ssp_data->dev_addr, ssp_data->dev_addr_byte_num,
                ssp_data->reg_addr, ssp_data->reg_addr_byte_num, ssp_data->data, ssp_data->data_byte_num);
        }
    }
    drv_ctx->bottom_half_cross_frame = TD_FALSE;

    isp_dfx_sns_sync_cfg_show(drv_ctx, cur_node_idx, stay_cnt, update_pos);

    return TD_SUCCESS;
}

#ifdef CONFIG_OT_SYS_KOL_SUPPORT
#ifndef ISP_WRITE_I2C_THROUGH_MUL_REG
static td_void isp_drv_standby_i2c_one_reg(isp_drv_ctx *drv_ctx, ot_isp_sns_regs_info *standby_cfg)
{
    td_u32 i;
    ot_isp_i2c_data *i2c_data = TD_NULL;
    isp_check_pointer_void_return(standby_cfg);

    for (i = 0; i < standby_cfg->reg_num; i++) {
        i2c_data = &standby_cfg->i2c_data[i];
        drv_ctx->bus_cb.pfn_isp_write_i2c_data(standby_cfg->com_bus.i2c_dev, i2c_data->dev_addr, i2c_data->reg_addr,
            i2c_data->addr_byte_num, i2c_data->data, i2c_data->data_byte_num);
    }

    return;
}
#else
static td_void isp_drv_standby_i2c_mul_reg(isp_drv_ctx *drv_ctx, ot_isp_sns_regs_info *standby_cfg)
{
    td_u32 i;
    td_s8 tmp_buf[ISP_WRITE_I2C_MAX_REG_NUM];
    td_u8 idx = 0;
    td_u8 dev_addr, addr_byte_num, data_byte_num;
    ot_isp_i2c_data *i2c_data = TD_NULL;
    isp_check_pointer_void_return(standby_cfg);
    for (i = 0; i < standby_cfg->reg_num; i++) {
        i2c_data = &standby_cfg->i2c_data[i];
        dev_addr = i2c_data->dev_addr;
        addr_byte_num = i2c_data->addr_byte_num;
        data_byte_num = i2c_data->data_byte_num;
        if ((idx + addr_byte_num + data_byte_num) >= ISP_WRITE_I2C_MAX_REG_NUM) {
            drv_ctx->bus_cb.pfn_isp_write_i2c_data(standby_cfg->com_bus.i2c_dev, dev_addr,
                tmp_buf, sizeof(tmp_buf), idx, addr_byte_num, data_byte_num);
            idx = 0;
        }
        isp_drv_write_i2c_data_buf(i2c_data, tmp_buf, ISP_WRITE_I2C_MAX_REG_NUM, &idx);
    }
    if (idx != 0) {
        drv_ctx->bus_cb.pfn_isp_write_i2c_data(standby_cfg->com_bus.i2c_dev, dev_addr,
            tmp_buf, sizeof(tmp_buf), idx, addr_byte_num, data_byte_num);
    }

    return;
}
#endif

td_s32 isp_drv_standby_i2c_data(isp_drv_ctx *drv_ctx, ot_isp_sns_regs_info *standby_cfg)
{
    if (standby_cfg->com_bus.i2c_dev == -1) {
        return TD_SUCCESS;
    }
    if (drv_ctx->bus_cb.pfn_isp_write_i2c_data == TD_NULL) {
        isp_warn_trace("pfn_isp_write_i2c_data is TD_NULL point!\n");
        return TD_FAILURE;
    }

#ifndef ISP_WRITE_I2C_THROUGH_MUL_REG
    isp_drv_standby_i2c_one_reg(drv_ctx, standby_cfg);
#else
    isp_drv_standby_i2c_mul_reg(drv_ctx, standby_cfg);
#endif

    return TD_SUCCESS;
}

static td_s32 isp_drv_standby_ssp_data(isp_drv_ctx *drv_ctx, ot_isp_sns_regs_info *standby_cfg)
{
    td_u32 i;
    td_s8 ssp_dev, ssp_cs;
    ot_isp_ssp_data *ssp_data = TD_NULL;

    ssp_dev = standby_cfg->com_bus.ssp_dev.bit4_ssp_dev;
    ssp_cs = standby_cfg->com_bus.ssp_dev.bit4_ssp_cs;
    if (ssp_dev == -1) {
        return TD_SUCCESS;
    }

    if (drv_ctx->bus_cb.pfn_isp_write_ssp_data == TD_NULL) {
        isp_warn_trace("pfn_isp_write_ssp_data is TD_NULL point!\n");
        return TD_FAILURE;
    }

    for (i = 0; i < standby_cfg->reg_num; i++) {
        ssp_data = &standby_cfg->ssp_data[i];
        drv_ctx->bus_cb.pfn_isp_write_ssp_data(ssp_dev, ssp_cs, ssp_data->dev_addr, ssp_data->dev_addr_byte_num,
            ssp_data->reg_addr, ssp_data->reg_addr_byte_num, ssp_data->data, ssp_data->data_byte_num);
    }

    return TD_SUCCESS;
}

#endif

static td_void isp_drv_cfg_wrap_slave(ot_vi_pipe vi_pipe, td_u32 slave_dev, isp_sync_cfg_buf_node *cfg_node)
{
#ifdef CONFIG_OT_SYS_VI_WRAP_SUPPORT
    td_bool slave_wrap = TD_FALSE;
    td_s32 ret;

    isp_check_no_phy_pipe_void_return(vi_pipe);

    if (ckfn_vi_get_vi_pipe_wrap_en()) {
        ret = call_vi_get_vi_pipe_wrap_en(vi_pipe, &slave_wrap);
        if (ret != TD_SUCCESS) {
            isp_err_trace("pipe %d get wrap en error \n", vi_pipe);
            return;
        }
    }
    if (slave_wrap == TD_TRUE) {
        io_rw_pt_address(vicap_slave_hstime(slave_dev)) = cfg_node->sns_regs_info.slv_sync.slave_hs_time;
        io_rw_pt_address(vicap_slave_vscycle(slave_dev)) = cfg_node->sns_regs_info.slv_sync.slave_vs_cycle;
        io_rw_pt_address(vicap_slave_hscycle(slave_dev)) = cfg_node->sns_regs_info.slv_sync.slave_hs_cycle;
        io_rw_pt_address(vicap_slave_hsdlycle(slave_dev)) = cfg_node->sns_regs_info.slv_sync.slave_hs_dly_cyc;
    }
#else
    ot_unused(vi_pipe);
    ot_unused(slave_dev);
    ot_unused(cfg_node);
#endif
}

static td_bool dist_is_time_cfg_slave(ot_vi_pipe vi_pipe)
{
#ifdef CONFIG_OT_VI_DISTRIBUTE_GRP
    td_u8 i, j;
    isp_drv_ctx *drv_ctx;
    isp_drv_ctx *tmp_drv_ctx;
    vi_distribute_attr dist_attr = {0};

    tmp_drv_ctx = isp_drv_get_ctx(vi_pipe);
    if (tmp_drv_ctx->dist_attr.distribute_en == TD_FALSE) {
        return TD_TRUE;
    }

    if (!ckfn_vi_get_distribute_attr() || call_vi_get_distribute_attr(vi_pipe, &dist_attr) != TD_SUCCESS) {
        isp_err_trace("pipe %d get vi distribute attr error\n", vi_pipe);
        return TD_FALSE;
    }

    if (dist_attr.grp_share_vs == TD_FALSE) {
        for (j = 0; j < OT_VI_MAX_DISTRIBUTE_PIPE_NUM; ++j) {
            drv_ctx = isp_drv_get_ctx(tmp_drv_ctx->dist_attr.pipe_id[j]);
            if (drv_ctx->is_chg_fps_cfg == TD_FALSE) {
                return TD_FALSE;
            }
        }

        return TD_TRUE;
    }

    for (i = 0; i < OT_ISP_MAX_PHY_PIPE_NUM; ++i) {
        tmp_drv_ctx = isp_drv_get_ctx(i);
        for (j = 0; j < OT_VI_MAX_DISTRIBUTE_PIPE_NUM; ++j) {
            drv_ctx = isp_drv_get_ctx(tmp_drv_ctx->dist_attr.pipe_id[j]);
            if (drv_ctx->is_chg_fps_cfg == TD_FALSE) {
                return TD_FALSE;
            }
        }
    }

    return TD_TRUE;
#else
    return TD_TRUE;
#endif
}

#ifdef CONFIG_OT_VI_DISTRIBUTE_GRP
td_void isp_dist_reset_all_pipe_cfg(ot_vi_pipe vi_pipe)
{
    td_u8 i, j;
    isp_drv_ctx *drv_ctx;
    isp_drv_ctx *tmp_drv_ctx;
    vi_distribute_attr dist_attr = {0};
    unsigned long flags = 0;
    ot_vi_pipe tmp_pipe;

    tmp_drv_ctx = isp_drv_get_ctx(vi_pipe);
    if (tmp_drv_ctx->dist_attr.distribute_en == TD_FALSE) {
        return;
    }

    if (!ckfn_vi_get_distribute_attr() || call_vi_get_distribute_attr(vi_pipe, &dist_attr) != TD_SUCCESS) {
        isp_err_trace("pipe %d get vi distribute attr error\n", vi_pipe);
        return;
    }

    if (dist_attr.grp_share_vs == TD_FALSE) {
        for (j = 0; j < OT_VI_MAX_DISTRIBUTE_PIPE_NUM; ++j) {
            tmp_pipe = tmp_drv_ctx->dist_attr.pipe_id[j];
            drv_ctx = isp_drv_get_ctx(tmp_pipe);
            osal_spin_lock_irqsave(&g_isp_lock[tmp_pipe], &flags);
            drv_ctx->is_chg_fps_cfg = TD_FALSE;
            drv_ctx->is_chg_fps_done = TD_TRUE;
            drv_ctx->chg_fps_cnt = 0;
            osal_spin_unlock_irqrestore(&g_isp_lock[tmp_pipe], &flags);
        }
    } else {
        for (i = 0; i < OT_ISP_MAX_PHY_PIPE_NUM; ++i) {
            tmp_drv_ctx = isp_drv_get_ctx(i);
            for (j = 0; j < OT_VI_MAX_DISTRIBUTE_PIPE_NUM; ++j) {
                tmp_pipe = tmp_drv_ctx->dist_attr.pipe_id[j];
                drv_ctx = isp_drv_get_ctx(tmp_pipe);
                osal_spin_lock_irqsave(&g_isp_lock[tmp_pipe], &flags);
                drv_ctx->is_chg_fps_cfg = TD_FALSE;
                drv_ctx->is_chg_fps_done = TD_TRUE;
                drv_ctx->chg_fps_cnt = 0;
                osal_spin_unlock_irqrestore(&g_isp_lock[tmp_pipe], &flags);
            }
        }
    }

    return;
}

static td_void dist_sync_slave(ot_vi_pipe vi_pipe)
{
    vi_distribute_attr dist_attr = {0};

    if (!ckfn_vi_get_distribute_attr() || call_vi_get_distribute_attr(vi_pipe, &dist_attr) != TD_SUCCESS) {
        isp_err_trace("pipe %d get vi distribute attr error\n", vi_pipe);
        return ;
    }
    // sync slave
    io_rw_pt_address(vicap_slave_sync) = (1 << dist_attr.slave_mode_time);
    io_rw_pt_address(vicap_slave_sync) = 0;
}
#endif

static td_void dist_set_pipe_cfg_status(ot_vi_pipe vi_pipe)
{
#ifdef CONFIG_OT_VI_DISTRIBUTE_GRP
    unsigned long flags = 0;
    isp_drv_ctx *drv_ctx;
    drv_ctx = isp_drv_get_ctx(vi_pipe);

    osal_spin_lock_irqsave(&g_isp_lock[vi_pipe], &flags);
    if (drv_ctx->dist_attr.distribute_en == TD_TRUE) {
        drv_ctx->is_chg_fps_cfg = TD_TRUE;
    }

    osal_spin_unlock_irqrestore(&g_isp_lock[vi_pipe], &flags);
#else
    ot_unused(vi_pipe);
#endif
}

static td_void isp_drv_cfg_dist_slave(ot_vi_pipe vi_pipe, td_u32 slave_dev, isp_sync_cfg_buf_node *cfg_node)
{
#ifdef CONFIG_OT_VI_DISTRIBUTE_GRP
    isp_drv_ctx *drv_ctx;

    drv_ctx = isp_drv_get_ctx(vi_pipe);
    if (drv_ctx->dist_attr.distribute_en == TD_FALSE) {
        return;
    }

    if (!ckfn_vi_set_distribute_stop()) {
        isp_err_trace("pipe %d set distribute error \n", vi_pipe);
        return;
    }

    if (call_vi_set_distribute_stop(vi_pipe) != TD_SUCCESS) {
        isp_err_trace("pipe %d set distribute stop error \n", vi_pipe);
        return;
    }
    if (dist_is_time_cfg_slave(vi_pipe) == TD_TRUE) {
        io_rw_pt_address(vicap_slave_hstime(slave_dev)) = cfg_node->sns_regs_info.slv_sync.slave_hs_time;
        io_rw_pt_address(vicap_slave_vscycle(slave_dev)) = cfg_node->sns_regs_info.slv_sync.slave_vs_cycle;
        io_rw_pt_address(vicap_slave_hscycle(slave_dev)) = cfg_node->sns_regs_info.slv_sync.slave_hs_cycle;
        io_rw_pt_address(vicap_slave_hsdlycle(slave_dev)) = cfg_node->sns_regs_info.slv_sync.slave_hs_dly_cyc;

        isp_info_trace("pipe %d vs cycle %u  slave_hs_time %u  cycle %u \n ",
            vi_pipe, cfg_node->sns_regs_info.slv_sync.slave_vs_cycle,
            cfg_node->sns_regs_info.slv_sync.slave_hs_time, cfg_node->sns_regs_info.slv_sync.slave_hs_cycle);

        dist_sync_slave(vi_pipe);
        isp_dist_reset_all_pipe_cfg(vi_pipe);
    }
    cfg_node->sns_regs_info.slv_sync.update = TD_FALSE;
#else
    ot_unused(vi_pipe);
    ot_unused(slave_dev);
    ot_unused(cfg_node);
#endif
}

static td_void isp_drv_regs_cfg_sensor_slave(ot_vi_pipe vi_pipe, td_u32 slave_dev, isp_drv_ctx *drv_ctx,
    td_u8 cur_node_idx, td_u8 stay_cnt)
{
    td_u8 cfg_node_id;
    isp_sync_cfg_buf_node *cur_node = drv_ctx->sync_cfg.node[cur_node_idx];
    isp_sync_cfg_buf_node *cfg_node = TD_NULL;
    isp_check_pointer_void_return(cur_node);
    if (stay_cnt > cur_node->sns_regs_info.slv_sync.delay_frame_num) {
        isp_warn_trace("cur node unique_id %d stay_cnt %d, config slv_sync.delay_frame_num %d, already configed!\n",
            cur_node->unique_id, stay_cnt, cur_node->sns_regs_info.slv_sync.delay_frame_num);
        return;
    }

    cfg_node_id = cur_node->sns_regs_info.slv_sync.delay_frame_num - stay_cnt;
    cfg_node = isp_drv_get_sns_cfg_node(drv_ctx, cur_node_idx, cfg_node_id);
    if (cfg_node == TD_NULL || stay_cnt > cur_node->sns_regs_info.slv_sync.delay_frame_num) {
        return;
    }

    if ((cfg_node->valid == TD_TRUE) && (cfg_node->sns_regs_info.slv_sync.update == TD_TRUE) &&
        (slave_dev < CAP_SLAVE_MAX_NUM)) {
        /* adjust the relationship between slavedev and vipipe */
        dist_set_pipe_cfg_status(vi_pipe);

        if (dist_is_time_cfg_slave(vi_pipe) == TD_TRUE) {
            io_rw_pt_address(vicap_slave_vstime(slave_dev)) = cfg_node->sns_regs_info.slv_sync.slave_vs_time;
            isp_info_trace("pipe %d vs time %u \n ", vi_pipe, cfg_node->sns_regs_info.slv_sync.slave_vs_time);
        }
        isp_drv_cfg_dist_slave(vi_pipe, slave_dev, cfg_node);
        isp_drv_cfg_wrap_slave(vi_pipe, slave_dev, cfg_node);
    }

    return;
}

td_s32 isp_drv_reg_config_sensor(ot_vi_pipe vi_pipe, isp_drv_ctx *drv_ctx)
{
    td_s32 ret;
    td_u32 slave_dev;
    td_s8 i2c_dev;
    td_u8 stay_cnt;
    td_u8 cfg_node_idx;
    isp_drv_ctx *drv_ctx_main = TD_NULL;
    isp_sync_cfg_buf_node *cur_node = TD_NULL;

    isp_check_pipe_return(vi_pipe);
    isp_check_pointer_return(drv_ctx);

    cfg_node_idx = isp_get_sensor_cfg_index(vi_pipe, drv_ctx);

    drv_ctx->sync_cfg.pipe_cfg_node_idx = cfg_node_idx;

    cur_node = drv_ctx->sync_cfg.node[cfg_node_idx];
    if (isp_drv_regs_cfg_sensor_check(cur_node) != TD_SUCCESS) {
        return TD_FAILURE;
    }

    stay_cnt = (g_isp_run_wakeup_sel[vi_pipe] == OT_ISP_RUN_WAKEUP_BE_END) ? cur_node->stay_cnt : 0;
    if (stay_cnt >= drv_ctx->sync_cfg.cfg2_vld_dly_max) {
        cur_node->stay_cnt++;
        isp_warn_trace("current sensor register has been configured, skip it directly!\n");
        return TD_FAILURE;
    }

    if (drv_ctx->stitch_attr.stitch_enable == TD_TRUE && drv_ctx->stitch_attr.main_pipe == TD_FALSE) {
#ifdef CONFIG_OT_VI_STITCH_GRP
        drv_ctx_main = isp_drv_get_ctx(drv_ctx->stitch_attr.stitch_bind_id[0]);
        cfg_node_idx = drv_ctx_main->sync_cfg.pipe_cfg_node_idx;
        if (drv_ctx_main->sync_cfg.node[cfg_node_idx] == TD_NULL) {
            isp_warn_trace("cur_node NULL pointer Stitch!\n");
            return TD_FAILURE;
        }
#endif
    } else {
        drv_ctx_main = drv_ctx;
    }

    if (cur_node->sns_regs_info.sns_type == OT_ISP_SNS_TYPE_I2C) {
        i2c_dev = cur_node->sns_regs_info.com_bus.i2c_dev;

        ret = isp_drv_write_i2c_data(drv_ctx_main, cfg_node_idx, drv_ctx->int_pos, i2c_dev, stay_cnt);
        isp_check_return(vi_pipe, ret, "isp_drv_write_i2c_data");
    } else if (cur_node->sns_regs_info.sns_type == OT_ISP_SNS_TYPE_SSP) {
        ret = isp_drv_write_ssp_data(drv_ctx_main, cfg_node_idx, drv_ctx->int_pos, cur_node, stay_cnt);
        isp_check_return(vi_pipe, ret, "isp_drv_write_ssp_data");
    }

    /* write slave sns vmax sync */
    slave_dev = cur_node->sns_regs_info.slv_sync.slave_bind_dev;
    isp_drv_regs_cfg_sensor_slave(vi_pipe, slave_dev, drv_ctx_main, cfg_node_idx, stay_cnt);

    cur_node->stay_cnt++;
    isp_info_trace("vi pipe = %d unique_id = %d stay_cnt = %d\n", vi_pipe, cur_node->unique_id, cur_node->stay_cnt);

    return TD_SUCCESS;
}

#ifdef CONFIG_OT_SYS_KOL_SUPPORT
td_s32 isp_drv_reg_config_sensor_standby(ot_vi_pipe vi_pipe, isp_drv_ctx *drv_ctx)
{
    td_s32 ret;
    ot_isp_sns_regs_info *standby_cfg;
    isp_check_pipe_return(vi_pipe);
    isp_check_pointer_return(drv_ctx);

    standby_cfg = &drv_ctx->standby_cfg;

    if (standby_cfg->sns_type == OT_ISP_SNS_TYPE_I2C) {
        ret = isp_drv_standby_i2c_data(drv_ctx, standby_cfg);
        isp_check_return(vi_pipe, ret, "isp_drv_standby_i2c_data");
    } else if (standby_cfg->sns_type == OT_ISP_SNS_TYPE_SSP) {
        ret = isp_drv_standby_ssp_data(drv_ctx, standby_cfg);
        isp_check_return(vi_pipe, ret, "isp_drv_standby_ssp_data");
    }
    return TD_SUCCESS;
}
#endif

static td_s32 isp_drv_prepare_be_buf(ot_vi_pipe vi_pipe, isp_drv_ctx *drv_ctx)
{
    if (drv_ctx->use_node != TD_NULL) {
        if (g_isp_run_wakeup_sel[vi_pipe] == OT_ISP_RUN_WAKEUP_BE_END) {
            isp_warn_trace("Pipe[%d] isp is running!\r\n", vi_pipe);
        } else {
            isp_err_trace("Pipe[%d] isp is running!\r\n", vi_pipe);
        }
        drv_ctx->drv_dbg_info.cross_frame_cnt++;
        return TD_FAILURE;
    }

    if (isp_drv_get_free_be_buf(vi_pipe) != TD_SUCCESS) {
        isp_err_trace("Pipe[%d] get FreeBeBuf is fail!, busy:%d, free:%d, use:%d\r\n",
            vi_pipe, drv_ctx->be_buf_queue.busy_num, drv_ctx->be_buf_queue.free_num, drv_ctx->be_buf_info.use_cnt);

        return TD_FAILURE;
    }

    return TD_SUCCESS;
}

td_void isp_drv_wake_up_thread(ot_vi_pipe vi_pipe)
{
    td_bool wake_up_tread = TD_TRUE;
    isp_drv_ctx *drv_ctx = TD_NULL;

    drv_ctx = isp_drv_get_ctx(vi_pipe);
    if (is_offline_mode(drv_ctx->work_mode.running_mode) || is_striping_mode(drv_ctx->work_mode.running_mode) ||
        is_pre_online_post_offline(drv_ctx->work_mode.running_mode)) {
#ifdef CONFIG_OT_VI_STITCH_GRP
        if (drv_ctx->stitch_attr.stitch_enable == TD_TRUE) {
            if (drv_ctx->running_state != ISP_BE_BUF_STATE_INIT) {
                wake_up_tread = TD_FALSE;
            }
        }
#endif
    }

    if (wake_up_tread == TD_TRUE) {
        drv_ctx->edge = TD_TRUE;
        drv_ctx->vd_start = TD_TRUE;
        osal_wait_wakeup(&drv_ctx->isp_wait);
        osal_wait_wakeup(&drv_ctx->isp_wait_vd_start);
    }

    return;
}

#ifdef CONFIG_OT_VI_STITCH_GRP
static td_bool isp_is_stitch_pipe_be_all_done(ot_vi_pipe vi_pipe)
{
    td_u64 gap;
    ot_vi_pipe vi_pipe_main;
    isp_drv_ctx *drv_ctx = TD_NULL;
    isp_drv_ctx *drv_ctx_main = TD_NULL;

    drv_ctx = isp_drv_get_ctx(vi_pipe);

    vi_pipe_main = drv_ctx->stitch_attr.stitch_bind_id[0];
    drv_ctx_main = isp_drv_get_ctx(vi_pipe_main);

    gap = abs_diff(drv_ctx->frame_pts, drv_ctx_main->isp_sync_ctrl.first_done_pts);
    if (gap > ISP_STITCH_MAX_GAP) {
        if (drv_ctx_main->isp_sync_ctrl.stitch_done_cnt < drv_ctx->stitch_attr.stitch_pipe_num) {
            isp_warn_trace("vi_pipe = %d, stitch pipe be process may be discard frame.\n", vi_pipe);
        }
        drv_ctx_main->isp_sync_ctrl.stitch_done_cnt = 1;
        drv_ctx_main->isp_sync_ctrl.first_done_pts = drv_ctx->frame_pts;
    } else {
        drv_ctx_main->isp_sync_ctrl.stitch_done_cnt++;
    }

    return (drv_ctx_main->isp_sync_ctrl.stitch_done_cnt == drv_ctx->stitch_attr.stitch_pipe_num) ? TD_TRUE : TD_FALSE;
}

static td_void isp_drv_run_trigger_prepare_stitch(ot_vi_pipe vi_pipe, isp_drv_ctx *drv_ctx)
{
    td_s32 i;
    unsigned long flags = 0;
    ot_vi_pipe vi_pipe_s;
    isp_drv_ctx *drv_ctx_s = TD_NULL;

    if (isp_is_stitch_pipe_be_all_done(vi_pipe) == TD_TRUE) {
        /* Process all the pipe splicing tasks in turn, then wake up in turn */
        for (i = 0; i < drv_ctx->stitch_attr.stitch_pipe_num; i++) {
            vi_pipe_s = drv_ctx->stitch_attr.stitch_bind_id[i];
            drv_ctx_s = isp_drv_get_ctx(vi_pipe_s);
            osal_spin_lock_irqsave(&g_isp_lock[vi_pipe_s], &flags);
            if (isp_drv_prepare_be_buf(vi_pipe_s, drv_ctx_s) != TD_SUCCESS) {
                isp_warn_trace("Pipe[%d] force to exit\n", vi_pipe);
                osal_spin_unlock_irqrestore(&g_isp_lock[vi_pipe_s], &flags);
                return;
            }

            osal_spin_unlock_irqrestore(&g_isp_lock[vi_pipe_s], &flags);

            isp_drv_stat_buf_summary_insert(vi_pipe_s, drv_ctx_s);

            drv_ctx_s->status = ISP_INT_BE_END;

            isp_drv_wake_up_thread(vi_pipe_s);
        }
    }
}
#endif

static td_void isp_drv_run_trigger_prepare_normal(ot_vi_pipe vi_pipe, isp_drv_ctx *drv_ctx)
{
    unsigned long flags = 0;
    osal_spin_lock_irqsave(&g_isp_lock[vi_pipe], &flags);
    if (isp_drv_prepare_be_buf(vi_pipe, drv_ctx) != TD_SUCCESS) {
        osal_spin_unlock_irqrestore(&g_isp_lock[vi_pipe], &flags);
        isp_warn_trace("Pipe[%d] force to exit\n", vi_pipe);
        return;
    }

    osal_spin_unlock_irqrestore(&g_isp_lock[vi_pipe], &flags);

    isp_drv_stat_buf_summary_insert(vi_pipe, drv_ctx);

    drv_ctx->status = ISP_INT_BE_END;
    isp_drv_wake_up_thread(vi_pipe);
}

static td_void isp_drv_proc_calc_be_end_int(ot_vi_pipe vi_pipe, isp_drv_ctx *drv_ctx,
    td_u64 be_end_int_time1)
{
    td_u64 be_end_int_time2 = 0;

    if (ckfn_sys_entry() && ckfn_sys_get_time_stamp()) {
        be_end_int_time2 = call_sys_get_time_stamp();
    }
    drv_ctx->drv_dbg_info.isp_int_be_end_time = be_end_int_time2 - be_end_int_time1;

    if (drv_ctx->drv_dbg_info.isp_int_be_end_time > drv_ctx->drv_dbg_info.isp_int_be_end_time_max) {
        drv_ctx->drv_dbg_info.isp_int_be_end_time_max = drv_ctx->drv_dbg_info.isp_int_be_end_time;
    }
    if (g_isp_run_wakeup_sel[vi_pipe] == OT_ISP_RUN_WAKEUP_BE_END) {
        if (drv_ctx->drv_dbg_info.cross_frame_cnt != drv_ctx->drv_dbg_info.last_cross_cnt) {
            drv_ctx->drv_dbg_info.cross_last_end_int_time =
                (td_u32)(be_end_int_time1 - drv_ctx->drv_dbg_info.pre_end_int_time);
            isp_warn_trace("be_end_int gap_time:%u\n", drv_ctx->drv_dbg_info.cross_last_end_int_time);
        }
        drv_ctx->drv_dbg_info.pre_end_int_time = be_end_int_time1;
    }
}

td_void isp_drv_run_trigger_prepare(ot_vi_pipe vi_pipe, isp_drv_ctx *drv_ctx)
{
    if (drv_ctx->mem_init == TD_FALSE) {
        return;
    }

    if (drv_ctx->stitch_attr.stitch_enable == TD_TRUE) {
#ifdef CONFIG_OT_VI_STITCH_GRP
        isp_drv_run_trigger_prepare_stitch(vi_pipe, drv_ctx);
#endif
    } else {
        isp_drv_run_trigger_prepare_normal(vi_pipe, drv_ctx);
    }
}

td_s32 isp_drv_be_end_int_proc(ot_vi_pipe vi_pipe)
{
    isp_drv_ctx *drv_ctx = TD_NULL;
    unsigned long flags = 0;
    td_u64 be_end_int_time1 = 0;

    isp_check_pipe_return(vi_pipe);

    drv_ctx = isp_drv_get_ctx(vi_pipe);

    if (ckfn_sys_entry() && ckfn_sys_get_time_stamp()) {
        be_end_int_time1 = call_sys_get_time_stamp();
    }

    if (drv_ctx->run_once_ok == TD_TRUE) {
        isp_drv_fe_stats_buf_busy_put(vi_pipe);
        isp_drv_stats_buf_busy_put(vi_pipe);
        drv_ctx->run_once_ok = TD_FALSE;
    }

    if (drv_ctx->yuv_run_once_ok == TD_TRUE) {
        isp_drv_stats_buf_busy_put(vi_pipe);
        drv_ctx->yuv_run_once_ok = TD_FALSE;
    }

    osal_spin_lock_irqsave(&g_isp_lock[vi_pipe], &flags);
    drv_ctx->vd_be_end = TD_TRUE;
    osal_wait_wakeup(&drv_ctx->isp_wait_vd_be_end);
    osal_spin_unlock_irqrestore(&g_isp_lock[vi_pipe], &flags);

    if ((drv_ctx->run_once_ok == TD_TRUE) || (drv_ctx->yuv_run_once_ok == TD_TRUE)) {
        return TD_SUCCESS;
    }

    if (g_isp_run_wakeup_sel[vi_pipe] == OT_ISP_RUN_WAKEUP_BE_END) {
        isp_drv_run_trigger_prepare(vi_pipe, drv_ctx); /* runbe already read af statistic */
    } else {
        isp_drv_stats_be_af_offline_end_int_read(vi_pipe);
    }

    isp_drv_proc_calc_be_end_int(vi_pipe, drv_ctx, be_end_int_time1);

    return TD_SUCCESS;
}

static td_s32 isp_update_info_sync(ot_vi_pipe vi_pipe, ot_isp_dcf_update_info *isp_update_info)
{
    isp_drv_ctx *drv_ctx = TD_NULL;
    unsigned long flags = 0;
    td_s32 i;
    ot_isp_dcf_update_info *update_info_vir_addr = TD_NULL;
    isp_check_pipe_return(vi_pipe);
    drv_ctx = isp_drv_get_ctx(vi_pipe);

    osal_spin_lock_irqsave(&g_isp_lock[vi_pipe], &flags);

    if (drv_ctx->trans_info.update_info.vir_addr == TD_NULL) {
        isp_warn_trace("UpdateInfo buf don't init ok!\n");
        osal_spin_unlock_irqrestore(&g_isp_lock[vi_pipe], &flags);
        return OT_ERR_ISP_NOT_INIT;
    }

    update_info_vir_addr = (ot_isp_dcf_update_info *)drv_ctx->trans_info.update_info.vir_addr;

    for (i = ISP_MAX_UPDATEINFO_BUF_NUM - 1; i >= 1; i--) {
        (td_void)memcpy_s(update_info_vir_addr + i, sizeof(ot_isp_dcf_update_info), update_info_vir_addr + i - 1,
            sizeof(ot_isp_dcf_update_info));
    }
    (td_void)memcpy_s(update_info_vir_addr, sizeof(ot_isp_dcf_update_info), isp_update_info,
        sizeof(ot_isp_dcf_update_info));
    osal_spin_unlock_irqrestore(&g_isp_lock[vi_pipe], &flags);

    return TD_SUCCESS;
}

static td_s32 isp_frame_info_sync(ot_vi_pipe vi_pipe, ot_isp_frame_info *ispframe_info)
{
    isp_drv_ctx *drv_ctx = TD_NULL;
    unsigned long flags = 0;
    td_s32 i;
    ot_isp_frame_info *pframe_info_vir_addr = TD_NULL;
    isp_check_pipe_return(vi_pipe);
    drv_ctx = isp_drv_get_ctx(vi_pipe);

    osal_spin_lock_irqsave(&g_isp_lock[vi_pipe], &flags);

    if (drv_ctx->trans_info.frame_info.vir_addr == TD_NULL) {
        isp_warn_trace("UpdateInfo buf don't init ok!\n");
        osal_spin_unlock_irqrestore(&g_isp_lock[vi_pipe], &flags);
        return OT_ERR_ISP_NOT_INIT;
    }

    pframe_info_vir_addr = (ot_isp_frame_info *)drv_ctx->trans_info.frame_info.vir_addr;

    for (i = ISP_MAX_FRAMEINFO_BUF_NUM - 1; i >= 1; i--) {
        (td_void)memcpy_s(pframe_info_vir_addr + i, sizeof(ot_isp_frame_info), pframe_info_vir_addr + i - 1,
            sizeof(ot_isp_frame_info));
    }
    (td_void)memcpy_s(pframe_info_vir_addr, sizeof(ot_isp_frame_info), ispframe_info, sizeof(ot_isp_frame_info));

    if (drv_ctx->trans_info.debug_get_frame_flag == TD_TRUE) {
        drv_ctx->trans_info.debug_get_frame_flag = TD_FALSE;
    } else {
        isp_debug_trace("The frame info is updated twice without getting frame info.\n");
    }
    osal_spin_unlock_irqrestore(&g_isp_lock[vi_pipe], &flags);

    return TD_SUCCESS;
}

static td_u8 isp_cal_sync_info_index_typical(ot_vi_pipe vi_pipe, const isp_drv_ctx *drv_ctx)
{
    td_u8 cfg_dly_max;
    td_u8 index;

    cfg_dly_max = MAX2(drv_ctx->sync_cfg.cfg2_vld_dly_max, 2); /* cfg_dly_max max 2] */
    if (g_update_pos[vi_pipe] == 0) {
        index = cfg_dly_max - 1;
    } else {
        index = cfg_dly_max - 2; /* index [cfg_dly_max - 2] */
    }

    return index;
}

td_void isp_cal_sync_info_index(ot_vi_pipe vi_pipe, td_s32 *index)
{
    isp_drv_ctx *drv_ctx = TD_NULL;
    td_u8 idx;
    drv_ctx = isp_drv_get_ctx(vi_pipe);

    if (g_isp_run_wakeup_sel[vi_pipe] == OT_ISP_RUN_WAKEUP_BE_END) {
        idx = isp_drv_get_exp_dist_sync_index_specify(vi_pipe, drv_ctx, TD_FALSE);
    } else {
        idx = isp_cal_sync_info_index_typical(vi_pipe, drv_ctx);
    }

    *index = clip_max(idx, CFG2VLD_DLY_LIMIT - 1);
}

#ifdef CONFIG_OT_SNAP_SUPPORT
td_s32 isp_get_preview_dcf_info(ot_vi_pipe vi_pipe, ot_isp_dcf_info *isp_dcf)
{
    isp_drv_ctx *drv_ctx = TD_NULL;
    ot_isp_dcf_update_info *isp_update_info = TD_NULL;
    ot_isp_dcf_const_info *isp_dcf_const_info = TD_NULL;
    unsigned long flags = 0;
    td_s32 index = 0;
    ot_isp_dcf_update_info *update_info_vir_addr = TD_NULL;

    drv_ctx = isp_drv_get_ctx(vi_pipe);
    isp_check_tranbuf_init_return(vi_pipe, drv_ctx->trans_info.init);

    osal_spin_lock_irqsave(&g_isp_lock[vi_pipe], &flags);

    if (drv_ctx->trans_info.update_info.vir_addr == TD_NULL) {
        isp_warn_trace("UpdateInfo buf don't init ok!\n");
        osal_spin_unlock_irqrestore(&g_isp_lock[vi_pipe], &flags);
        return OT_ERR_ISP_NOT_INIT;
    }

    update_info_vir_addr = (ot_isp_dcf_update_info *)drv_ctx->trans_info.update_info.vir_addr;
    isp_cal_sync_info_index(vi_pipe, &index);

    isp_update_info = update_info_vir_addr + index;

    isp_dcf_const_info = (ot_isp_dcf_const_info *)(update_info_vir_addr + ISP_MAX_UPDATEINFO_BUF_NUM);

    (td_void)memcpy_s(&isp_dcf->isp_dcf_const_info, sizeof(ot_isp_dcf_const_info), isp_dcf_const_info,
        sizeof(ot_isp_dcf_const_info));
    (td_void)memcpy_s(&isp_dcf->isp_dcf_update_info, sizeof(ot_isp_dcf_update_info), isp_update_info,
        sizeof(ot_isp_dcf_update_info));
    osal_spin_unlock_irqrestore(&g_isp_lock[vi_pipe], &flags);

    return TD_SUCCESS;
}
#endif

static td_void isp_copy_actual_frame_info(ot_vi_pipe vi_pipe, ot_isp_frame_info *isp_frame, isp_drv_ctx *drv_ctx)
{
    td_s32 index = 0;
    ot_isp_frame_info *frame_info_vir_addr = TD_NULL;

    if ((isp_frame != TD_NULL) && (drv_ctx->trans_info.frame_info.vir_addr != TD_NULL)) {
        frame_info_vir_addr = (ot_isp_frame_info *)drv_ctx->trans_info.frame_info.vir_addr;
        isp_cal_sync_info_index(vi_pipe, &index);
        (td_void)memcpy_s(isp_frame, sizeof(ot_isp_frame_info), frame_info_vir_addr + index,
            sizeof(ot_isp_frame_info));
    }
}

#ifdef CONFIG_OT_SNAP_SUPPORT
static td_s32 isp_get_actual_preview_frame_info(ot_vi_pipe vi_pipe, ot_isp_frame_info *isp_frame)
{
    isp_drv_ctx *drv_ctx = TD_NULL;
    unsigned long flags = 0;
    td_s32 ret = TD_SUCCESS;
    isp_check_pipe_return(vi_pipe);

    drv_ctx = isp_drv_get_ctx(vi_pipe);

    osal_spin_lock_irqsave(&g_isp_lock[vi_pipe], &flags);
    isp_check_tranbuf_init_goto(vi_pipe, drv_ctx->trans_info.init, ret, exit);

    isp_copy_actual_frame_info(vi_pipe, isp_frame, drv_ctx);

    drv_ctx->trans_info.debug_get_frame_flag = TD_TRUE;
exit:
    osal_spin_unlock_irqrestore(&g_isp_lock[vi_pipe], &flags);

    return ret;
}
#endif

static td_void isp_get_actual_exp_distance(ot_vi_pipe vi_pipe, ot_isp_frame_info *isp_frame, isp_drv_ctx *drv_ctx)
{
    td_u8 i, fe_cfg_node_idx;
    isp_sync_cfg_buf_node *cfg_node = TD_NULL;

    fe_cfg_node_idx = isp_drv_get_fe_sync_index(vi_pipe, drv_ctx, TD_FALSE);
    fe_cfg_node_idx = MIN2(fe_cfg_node_idx, CFG2VLD_DLY_LIMIT - 1);
    cfg_node = drv_ctx->sync_cfg.node[fe_cfg_node_idx];
    if (cfg_node == TD_NULL) {
        return;
    }

    for (i = 0; i < (OT_ISP_WDR_MAX_FRAME_NUM - 1); i++) {
        isp_frame->exposure_distance[i] = cfg_node->sns_regs_info.exp_distance[i];
    }
}

static td_s32 isp_get_actual_frame_info(ot_vi_pipe vi_pipe, ot_isp_frame_info *isp_frame)
{
    isp_drv_ctx *drv_ctx = TD_NULL;
    unsigned long flags = 0;
    td_s32 ret = TD_SUCCESS;

    isp_check_pipe_return(vi_pipe);
    isp_check_pointer_return(isp_frame);

    drv_ctx = isp_drv_get_ctx(vi_pipe);
    if (drv_ctx->mem_init == TD_FALSE) {
        return OT_ERR_ISP_MEM_NOT_INIT;
    }
#ifdef CONFIG_OT_SNAP_SUPPORT
    if (vi_pipe == drv_ctx->snap_attr.picture_pipe_id) {
        ret = isp_get_actual_preview_frame_info(drv_ctx->snap_attr.preview_pipe_id, isp_frame);
    } else {
#endif
        osal_spin_lock_irqsave(&g_isp_lock[vi_pipe], &flags);
        isp_check_tranbuf_init_goto(vi_pipe, drv_ctx->trans_info.init, ret, exit);

        isp_copy_actual_frame_info(vi_pipe, isp_frame, drv_ctx);

        isp_get_actual_exp_distance(vi_pipe, isp_frame, drv_ctx);
        drv_ctx->trans_info.debug_get_frame_flag = TD_TRUE;
exit:
        osal_spin_unlock_irqrestore(&g_isp_lock[vi_pipe], &flags);
#ifdef CONFIG_OT_SNAP_SUPPORT
    }
#endif
    return ret;
}

td_s32 isp_set_mod_param(ot_isp_mod_param *mod_param)
{
    ot_vi_pipe vi_pipe;
    isp_drv_ctx *drv_ctx = TD_NULL;

    isp_check_pointer_return(mod_param);

    for (vi_pipe = 0; vi_pipe < OT_ISP_MAX_PIPE_NUM; vi_pipe++) {
        drv_ctx = isp_drv_get_ctx(vi_pipe);
        if (drv_ctx->mem_init == TD_TRUE) {
            isp_err_trace("Does not support changed after isp init!\n");
            return OT_ERR_ISP_NOT_SUPPORT;
        }
    }

    if ((mod_param->interrupt_bottom_half != 0) && (mod_param->interrupt_bottom_half != 1)) {
        isp_err_trace("u32IntBotHalf must be 0 or 1.\n");
        return OT_ERR_ISP_ILLEGAL_PARAM;
    }

    g_int_bottom_half = mod_param->interrupt_bottom_half;

#ifndef __LITEOS__
    if (g_int_bottom_half) {
        g_use_bottom_half = TD_TRUE;
    }
#else
#ifdef CONFIG_OT_ISP_LITEOS_BOTTOM_HALF_SUPPORT
    if (g_int_bottom_half) {
        g_use_bottom_half = TD_TRUE;
    }
#endif
#endif

    return TD_SUCCESS;
}

td_s32 isp_get_mod_param(ot_isp_mod_param *mod_param)
{
    isp_check_pointer_return(mod_param);

    mod_param->interrupt_bottom_half = g_int_bottom_half;
    return TD_SUCCESS;
}

static td_void isp_drv_update_ctrl_param(ot_vi_pipe vi_pipe, ot_isp_ctrl_param *isp_ctrl_param)
{
    g_proc_param[vi_pipe] = isp_ctrl_param->proc_param;
    g_stat_intvl[vi_pipe] = isp_ctrl_param->stat_interval;
    g_update_pos[vi_pipe] = isp_ctrl_param->update_pos;
    g_int_timeout[vi_pipe] = isp_ctrl_param->interrupt_time_out;
    g_pwm_number[vi_pipe] = isp_ctrl_param->pwm_num;
    g_port_int_delay[vi_pipe] = isp_ctrl_param->port_interrupt_delay;
    g_ldci_tpr_flt_en[vi_pipe] = isp_ctrl_param->ldci_tpr_flt_en;
    g_be_buf_num[vi_pipe] = isp_ctrl_param->be_buf_num;
    g_ob_stats_update_pos[vi_pipe] = isp_ctrl_param->ob_stats_update_pos;
    g_isp_alg_run_select[vi_pipe] = isp_ctrl_param->alg_run_select;
    g_quick_start[vi_pipe] = isp_ctrl_param->quick_start_en;
    g_long_frm_int_en[vi_pipe] = isp_ctrl_param->long_frame_interrupt_en;
    g_isp_run_wakeup_sel[vi_pipe]  = isp_ctrl_param->isp_run_wakeup_select;
}

static td_s32 isp_drv_ctrl_interrupt_param_check(ot_vi_pipe vi_pipe, const ot_isp_ctrl_param *isp_ctrl_param,
    const isp_drv_ctx *drv_ctx)
{
    if ((isp_ctrl_param->update_pos != 0) && (isp_ctrl_param->update_pos != 1)) {
        isp_err_trace("vi_pipe:%d update_pos must be 0 or 1.\n", vi_pipe);
        return OT_ERR_ISP_ILLEGAL_PARAM;
    }

    if ((g_update_pos[vi_pipe] != isp_ctrl_param->update_pos) && (drv_ctx->mem_init == TD_TRUE)) {
        isp_err_trace("vi_pipe:%d Does not support changed after isp init (update_pos)!\n", vi_pipe);
        return OT_ERR_ISP_NOT_SUPPORT;
    }

    if ((g_port_int_delay[vi_pipe] != isp_ctrl_param->port_interrupt_delay) && (drv_ctx->mem_init == TD_TRUE)) {
        isp_err_trace("vi_pipe:%d Does not support changed after isp init (port_interrupt_delay)!\n", vi_pipe);
        return OT_ERR_ISP_NOT_SUPPORT;
    }

    if (isp_ctrl_param->ob_stats_update_pos >= OT_ISP_UPDATE_OB_STATS_BUTT) {
        isp_err_trace("vi_pipe:%d err ob_stats_update_pos\n", vi_pipe);
        return OT_ERR_ISP_ILLEGAL_PARAM;
    }

    if ((g_ob_stats_update_pos[vi_pipe] != isp_ctrl_param->ob_stats_update_pos) && (drv_ctx->mem_init == TD_TRUE)) {
        isp_err_trace("vi_pipe:%d Does not support changed after isp init (ob_stats_update_pos)!\n", vi_pipe);
        return OT_ERR_ISP_NOT_SUPPORT;
    }

    return TD_SUCCESS;
}

static td_s32 isp_drv_ctrl_alg_sel_check(ot_vi_pipe vi_pipe, const ot_isp_ctrl_param *isp_ctrl_param,
    const isp_drv_ctx *drv_ctx)
{
    if (isp_ctrl_param->alg_run_select >= OT_ISP_ALG_RUN_BUTT) {
        isp_err_trace("vi_pipe:%d err alg_run_select\n", vi_pipe);
        return OT_ERR_ISP_ILLEGAL_PARAM;
    }

    if ((g_isp_alg_run_select[vi_pipe] != isp_ctrl_param->alg_run_select) && (drv_ctx->mem_init == TD_TRUE)) {
        isp_err_trace("vi_pipe:%d Does not support changed after isp init (alg_run_select)!\n", vi_pipe);
        return OT_ERR_ISP_NOT_SUPPORT;
    }

    if (is_no_fe_pipe(vi_pipe) && (isp_ctrl_param->alg_run_select == OT_ISP_ALG_RUN_FE_ONLY)) {
        isp_err_trace("vi_pipe:%d Does not support set alg_run_select to %d when no fe pipe!\n",
            vi_pipe, isp_ctrl_param->alg_run_select);
        return OT_ERR_ISP_NOT_SUPPORT;
    }

    return TD_SUCCESS;
}

td_s32 isp_set_ctrl_param(ot_vi_pipe vi_pipe, ot_isp_ctrl_param *isp_ctrl_param)
{
    td_s32 ret;
    isp_drv_ctx *drv_ctx = TD_NULL;

    isp_check_pipe_return(vi_pipe);
    isp_check_pointer_return(isp_ctrl_param);
    isp_check_bool_return(isp_ctrl_param->ldci_tpr_flt_en);

    drv_ctx = isp_drv_get_ctx(vi_pipe);
    /* isp proc can be closed before mem init */
    /* if isp proc is opened, cannot change proc_param to 0 after mem init */
    if ((g_proc_param[vi_pipe] != 0) && (isp_ctrl_param->proc_param == 0) && (drv_ctx->mem_init == TD_TRUE)) {
        isp_err_trace("Vipipe:%d proc_param do not support to change %d to 0.\n", vi_pipe, g_proc_param[vi_pipe]);
        return OT_ERR_ISP_NOT_SUPPORT;
    }

    /* if isp proc is closed, cannot change proc_param to non-0 after mem init */
    if ((g_proc_param[vi_pipe] == 0) && (isp_ctrl_param->proc_param != 0) && (drv_ctx->mem_init == TD_TRUE)) {
        isp_err_trace("Vipipe:%d proc_param do not support to change %d to Non-0.\n", vi_pipe, g_proc_param[vi_pipe]);
        return OT_ERR_ISP_NOT_SUPPORT;
    }

    if (!isp_ctrl_param->stat_interval) {
        isp_err_trace("Vipipe:%d stat_interval must be larger than 0.\n", vi_pipe);
        return OT_ERR_ISP_ILLEGAL_PARAM;
    }

    ret = isp_drv_ctrl_interrupt_param_check(vi_pipe, isp_ctrl_param, drv_ctx);
    if (ret != TD_SUCCESS) {
        return ret;
    }

    ret = isp_drv_ctrl_alg_sel_check(vi_pipe, isp_ctrl_param, drv_ctx);
    if (ret != TD_SUCCESS) {
        return ret;
    }

    if ((g_pwm_number[vi_pipe] != isp_ctrl_param->pwm_num) && (drv_ctx->mem_init == TD_TRUE)) {
        isp_err_trace("Vipipe:%d Does not support changed after isp init (pwm_num)!\n", vi_pipe);
        return OT_ERR_ISP_NOT_SUPPORT;
    }

    if ((g_ldci_tpr_flt_en[vi_pipe] != isp_ctrl_param->ldci_tpr_flt_en) && (drv_ctx->mem_init == TD_TRUE)) {
        isp_err_trace("Vipipe:%d Does not support changed after isp init (ldci_tpr_flt_en)!\n", vi_pipe);
        return OT_ERR_ISP_NOT_SUPPORT;
    }

    if (isp_ctrl_param->be_buf_num < OT_ISP_BE_BUF_NUM_MIN || isp_ctrl_param->be_buf_num > OT_ISP_BE_BUF_NUM_MAX) {
        isp_err_trace("err be_buf_num, range:[%d, %d]\n", OT_ISP_BE_BUF_NUM_MIN, OT_ISP_BE_BUF_NUM_MAX);
        return OT_ERR_ISP_ILLEGAL_PARAM;
    }

    if ((g_be_buf_num[vi_pipe] != isp_ctrl_param->be_buf_num) && (drv_ctx->mem_init == TD_TRUE)) {
        isp_err_trace("Vipipe:%d Does not support changed after isp init (be_buf_num)!\n", vi_pipe);
        return OT_ERR_ISP_NOT_SUPPORT;
    }

    if (isp_ctrl_param->isp_run_wakeup_select >= OT_ISP_RUN_WAKEUP_BUTT) {
        isp_err_trace("isp_run_wakeup_select, range:[%d, %d)n", 0, OT_ISP_RUN_WAKEUP_BUTT);
        return OT_ERR_ISP_ILLEGAL_PARAM;
    }

    if ((isp_ctrl_param->isp_run_wakeup_select != g_isp_run_wakeup_sel[vi_pipe]) && (drv_ctx->mem_init == TD_TRUE)) {
        isp_err_trace("Vipipe:%d Does not support changed after isp init (isp_run_wakeup_select)!\n", vi_pipe);
        return OT_ERR_ISP_NOT_SUPPORT;
    }

    if ((isp_ctrl_param->quick_start_en != 0) && (isp_ctrl_param->quick_start_en != 1)) {
        isp_err_trace("u32QuickStart must be 0 or 1.\n");
        return OT_ERR_ISP_ILLEGAL_PARAM;
    }

    if ((isp_ctrl_param->long_frame_interrupt_en != 0) && (isp_ctrl_param->long_frame_interrupt_en != 1)) {
        isp_err_trace("long_frame_interrupt_en must be 0 or 1.\n");
        return OT_ERR_ISP_ILLEGAL_PARAM;
    }

    isp_drv_update_ctrl_param(vi_pipe, isp_ctrl_param);

    return TD_SUCCESS;
}

td_s32 isp_get_ctrl_param(ot_vi_pipe vi_pipe, ot_isp_ctrl_param *isp_ctrl_param)
{
    isp_check_pipe_return(vi_pipe);
    isp_check_pointer_return(isp_ctrl_param);

    isp_ctrl_param->proc_param = g_proc_param[vi_pipe];
    isp_ctrl_param->stat_interval = g_stat_intvl[vi_pipe];
    isp_ctrl_param->update_pos = g_update_pos[vi_pipe];
    isp_ctrl_param->interrupt_time_out = g_int_timeout[vi_pipe];
    isp_ctrl_param->pwm_num = g_pwm_number[vi_pipe];
    isp_ctrl_param->port_interrupt_delay = g_port_int_delay[vi_pipe];
    isp_ctrl_param->ldci_tpr_flt_en = g_ldci_tpr_flt_en[vi_pipe];
    isp_ctrl_param->be_buf_num = g_be_buf_num[vi_pipe];
    isp_ctrl_param->ob_stats_update_pos = g_ob_stats_update_pos[vi_pipe];
    isp_ctrl_param->alg_run_select      = g_isp_alg_run_select[vi_pipe];
    isp_ctrl_param->isp_run_wakeup_select = g_isp_run_wakeup_sel[vi_pipe];
    isp_ctrl_param->quick_start_en = g_quick_start[vi_pipe];
    isp_ctrl_param->long_frame_interrupt_en = g_long_frm_int_en[vi_pipe];
    return TD_SUCCESS;
}
#ifdef CONFIG_OT_ISP_DETAIL_STATS_SUPPORT
static td_s32 isp_check_detail_stats_cfg(ot_vi_pipe vi_pipe, const ot_isp_detail_stats_cfg *detail_stats_cfg)
{
    isp_drv_ctx *drv_ctx = TD_NULL;
    drv_ctx = isp_drv_get_ctx(vi_pipe);
    if (ckfn_vi_is_pipe_existed()) {
        if (call_vi_is_pipe_existed(vi_pipe) == TD_TRUE) {
            isp_err_trace("VI pipe %d is created! need set before vi_pipe created\r\n", vi_pipe);
            return OT_ERR_ISP_NOT_SUPPORT;
        }
    }
    if (drv_ctx->mem_init == TD_TRUE) {
        isp_err_trace("VI pipe %d need set before isp mem init\r\n", vi_pipe);
        return OT_ERR_ISP_NOT_SUPPORT;
    }

    isp_check_bool_return(detail_stats_cfg->enable);
    if (detail_stats_cfg->enable == TD_TRUE && detail_stats_cfg->ctrl.bit1_ae == 0 &&
        detail_stats_cfg->ctrl.bit1_awb == 0) {
        isp_err_trace("pipe:%d, detail_stats is enable, but ctrl:%d is all zero, need to check\n",
            vi_pipe, detail_stats_cfg->ctrl.key);
        return OT_ERR_ISP_ILLEGAL_PARAM;
    }

    if (detail_stats_cfg->col > OT_ISP_DETAIL_STATS_MAX_COLUMN || detail_stats_cfg->col < 1 ||
        detail_stats_cfg->row > OT_ISP_DETAIL_STATS_MAX_ROW || detail_stats_cfg->row < 1) {
        isp_err_trace("vi_pipe:%d, detail_stats_cfg->col:%d need be [1, %d], row:%d need be [1, %d]\n",
            vi_pipe, detail_stats_cfg->col, OT_ISP_DETAIL_STATS_MAX_COLUMN,
            detail_stats_cfg->row, OT_ISP_DETAIL_STATS_MAX_ROW);
        return OT_ERR_ISP_ILLEGAL_PARAM;
    }

    if (detail_stats_cfg->enable == TD_TRUE && detail_stats_cfg->col == 1 && detail_stats_cfg->row == 1) {
        isp_err_trace("vi_pipe:%d, detail_stats_cfg->col:%d row:%d no need set detail stats cfg\n",
            vi_pipe, detail_stats_cfg->col, detail_stats_cfg->row);
        return OT_ERR_ISP_ILLEGAL_PARAM;
    }

    if (detail_stats_cfg->interval < 1) {
        isp_err_trace("Vipipe:%d interval must be larger than 0.\n", vi_pipe);
        return OT_ERR_ISP_ILLEGAL_PARAM;
    }
    return TD_SUCCESS;
}

td_s32 isp_set_detail_stats_cfg(ot_vi_pipe vi_pipe, ot_isp_detail_stats_cfg *detail_stats_cfg)
{
    td_s32 ret;
    isp_drv_ctx *drv_ctx = TD_NULL;

    isp_check_fe_pipe_return(vi_pipe);
    isp_check_pointer_return(detail_stats_cfg);

    drv_ctx = isp_drv_get_ctx(vi_pipe);

    ret = isp_check_detail_stats_cfg(vi_pipe, detail_stats_cfg);
    if (ret != TD_SUCCESS) {
        return ret;
    }

    (td_void)memcpy_s(&drv_ctx->detail_stats_cfg, sizeof(ot_isp_detail_stats_cfg),
        detail_stats_cfg, sizeof(ot_isp_detail_stats_cfg));

    return TD_SUCCESS;
}

td_s32 isp_get_detail_stats_cfg(ot_vi_pipe vi_pipe, ot_isp_detail_stats_cfg *detail_stats_cfg)
{
    isp_drv_ctx *drv_ctx = TD_NULL;

    isp_check_fe_pipe_return(vi_pipe);
    isp_check_pointer_return(detail_stats_cfg);
    drv_ctx = isp_drv_get_ctx(vi_pipe);

    (td_void)memcpy_s(detail_stats_cfg, sizeof(ot_isp_detail_stats_cfg),
        &drv_ctx->detail_stats_cfg, sizeof(ot_isp_detail_stats_cfg));

    return TD_SUCCESS;
}
#define ISP_PIPE_SPLIT_ALIGN 4
static td_void isp_drv_calc_split_detail_size(td_u32 input, td_u32 detail_size[], td_u32 split_num)
{
    td_u32 total_size = input;
    td_u32 i;
    for (i = split_num; i > 0; i--) {
        detail_size[split_num - i] = OT_ALIGN_DOWN(total_size / i, ISP_PIPE_SPLIT_ALIGN);
        total_size -= detail_size[split_num - i];
    }
}

td_s32 isp_drv_get_detail_blk_size(ot_vi_pipe vi_pipe, isp_detail_size *detail_blk_size)
{
    isp_drv_ctx *drv_ctx = TD_NULL;
    ot_size size;
    isp_check_pointer_return(detail_blk_size);
    drv_ctx = isp_drv_get_ctx(vi_pipe);
    if (drv_ctx->detail_stats_cfg.enable == TD_FALSE) {
        return TD_SUCCESS;
    }
    isp_check_fe_pipe_return(vi_pipe);
    if (is_offline_mode(drv_ctx->work_mode.running_mode) == TD_FALSE) {
        isp_err_trace("isp[%d] detail_stats only support offline mode:%d\n", vi_pipe, drv_ctx->work_mode.running_mode);
        return OT_ERR_ISP_NOT_SUPPORT;
    }

    size.width = drv_ctx->work_mode.frame_rect.width;
    size.height = drv_ctx->work_mode.frame_rect.height;
    isp_drv_calc_split_detail_size(size.width, drv_ctx->detail_size.split_width, drv_ctx->detail_stats_cfg.col);
    isp_drv_calc_split_detail_size(size.height, drv_ctx->detail_size.split_height, drv_ctx->detail_stats_cfg.row);

    (td_void)memcpy_s(detail_blk_size, sizeof(isp_detail_size),
        &drv_ctx->detail_size, sizeof(isp_detail_size));

    return TD_SUCCESS;
}
static td_void isp_drv_calc_grid_pos(td_u32 width, td_u32 start, td_u32 blk_num, td_u16 *grid, td_u8 start_idx)
{
    td_u32 i, remainder;
    grid[start_idx] = start;
    for (i = 1; i < blk_num; i++) {
        remainder = (width * i) % blk_num;
        if (remainder == 0) {
            grid[start_idx + i] = start + (width * i) / blk_num;
        } else {
            grid[start_idx + i] = start + (width * i) / blk_num + 1;
        }
    }
}

static td_void isp_drv_calc_x_grid_info(isp_drv_ctx *drv_ctx, td_bool ae_grid_update,
    td_bool awb_grid_update, ot_isp_detail_ae_grid_info *ae_grid_info, ot_isp_detail_awb_grid_info *awb_grid_info)
{
    td_u8 i, col;
    td_u32 w, start_x;
    col = drv_ctx->detail_stats_cfg.col;
    start_x = 0;
    for (i = 0; i < col; i++) {
        w = drv_ctx->detail_size.split_width[i];
        if (ae_grid_update) {
            isp_drv_calc_grid_pos(w, start_x, OT_ISP_AE_ZONE_COLUMN,
                ae_grid_info->grid_x_pos, (i * OT_ISP_AE_ZONE_COLUMN));
        }
        if (awb_grid_update) {
            isp_drv_calc_grid_pos(w, start_x, OT_ISP_AWB_ZONE_ORIG_COLUMN,
                awb_grid_info->grid_x_pos, (i * OT_ISP_AWB_ZONE_ORIG_COLUMN));
        }
        start_x += w;
    }
    if (ae_grid_update) {
        ae_grid_info->grid_x_pos[OT_ISP_AE_ZONE_COLUMN * col] = start_x - 1;
    }
    if (awb_grid_update) {
        awb_grid_info->grid_x_pos[OT_ISP_AWB_ZONE_ORIG_COLUMN * col] = start_x - 1;
    }
}

static td_void isp_drv_calc_y_grid_info(isp_drv_ctx *drv_ctx, td_bool ae_grid_update,
    td_bool awb_grid_update, ot_isp_detail_ae_grid_info *ae_grid_info, ot_isp_detail_awb_grid_info *awb_grid_info)
{
    td_u8 i, row;
    td_u32 h, start_y;
    row = drv_ctx->detail_stats_cfg.row;
    start_y = 0;
    for (i = 0; i < row; i++) {
        h = drv_ctx->detail_size.split_height[i];
        if (ae_grid_update) {
            isp_drv_calc_grid_pos(h, start_y, OT_ISP_AE_ZONE_ROW,
                ae_grid_info->grid_y_pos, (i * OT_ISP_AE_ZONE_ROW));
        }
        if (awb_grid_update) {
            isp_drv_calc_grid_pos(h, start_y, OT_ISP_AWB_ZONE_ORIG_ROW,
                awb_grid_info->grid_y_pos, (i * OT_ISP_AWB_ZONE_ORIG_ROW));
        }
        start_y += h;
    }
    if (ae_grid_update) {
        ae_grid_info->grid_y_pos[OT_ISP_AE_ZONE_ROW * row] = start_y - 1;
        ae_grid_info->status = 1;
    }
    if (awb_grid_update) {
        awb_grid_info->grid_y_pos[OT_ISP_AWB_ZONE_ORIG_COLUMN * row] = start_y - 1;
        awb_grid_info->status = 1;
    }
}

static td_void isp_drv_calc_grid_info(ot_vi_pipe vi_pipe, isp_drv_ctx *drv_ctx,
    isp_detail_stats_user *detail_stats_to_user, ot_isp_detail_stats *detail_stats)
{
    td_bool ae_grid_update, awb_grid_update;
    ot_isp_detail_ae_grid_info *ae_grid_info = TD_NULL;
    ot_isp_detail_awb_grid_info *awb_grid_info = TD_NULL;

    if (detail_stats_to_user->ae_grid_status == 1 && detail_stats_to_user->awb_grid_status == 1) {
        isp_info_trace("pipe:%d, this buf gets grid_info before, no need to get again\n", vi_pipe);
        return;
    }
    ae_grid_info = &detail_stats->ae_stats.ae_grid_info;
    awb_grid_info = &detail_stats->awb_stats.awb_grid_info;
    ae_grid_update = (drv_ctx->detail_stats_cfg.ctrl.bit1_ae && detail_stats_to_user->ae_grid_status == 0);
    awb_grid_update = (drv_ctx->detail_stats_cfg.ctrl.bit1_awb && detail_stats_to_user->awb_grid_status == 0);

    isp_drv_calc_x_grid_info(drv_ctx, ae_grid_update, awb_grid_update, ae_grid_info, awb_grid_info);
    isp_drv_calc_y_grid_info(drv_ctx, ae_grid_update, awb_grid_update, ae_grid_info, awb_grid_info);

    return;
}

td_s32 isp_drv_get_detail_stats(ot_vi_pipe vi_pipe, ot_isp_detail_stats *detail_stats,
    isp_detail_stats_user *detail_stats_to_user)
{
    td_s32 ret, ret1;
    isp_drv_ctx *drv_ctx = TD_NULL;
    isp_be_offline_detail_stat *stt_addr = TD_NULL;
    vi_stt_buf_info stt_info = {0};

    isp_check_fe_pipe_return(vi_pipe);
    isp_check_pointer_return(detail_stats);
    drv_ctx = isp_drv_get_ctx(vi_pipe);

    if (ckfn_vi_get_pipe_stt_info()) {
        ret = call_vi_get_pipe_stt_info(vi_pipe, &stt_info);
        if (ret == OT_ERR_VI_BUF_EMPTY) {
            isp_info_trace("vi pipe[%d] stt_buf is empty, please wait\n", vi_pipe);
            return TD_SUCCESS;
        } else if (ret != TD_SUCCESS) {
            isp_err_trace("vi get pipe:%d stt_info failed:0x%x\n", vi_pipe, ret);
            return TD_FAILURE;
        }
    }

    stt_addr = (isp_be_offline_detail_stat *)stt_info.stt_virt_addr;
    if (stt_addr == TD_NULL) {
        isp_err_trace("vi get pipe:%d stt_info stt_addr is null\n", vi_pipe);
        ret = TD_FAILURE;
        goto release_ret;
    }

    isp_drv_read_be_detail_stats(drv_ctx, stt_addr, detail_stats);
    detail_stats->pts = stt_info.stt_pts;
    ret = TD_SUCCESS;

release_ret:
    if (ckfn_vi_release_pipe_stt_info()) {
        ret1 = call_vi_release_pipe_stt_info(vi_pipe, &stt_info);
        if (ret1 != TD_SUCCESS) {
            isp_err_trace("vi release pipe:%d stt_info failed:0x%x\n", vi_pipe, ret);
            return TD_FAILURE;
        }
    }

    isp_drv_calc_grid_info(vi_pipe, drv_ctx, detail_stats_to_user, detail_stats);

    return ret;
}
#endif
#ifdef CONFIG_OT_VI_STITCH_GRP
td_s32 isp_drv_stitch_sync(ot_vi_pipe vi_pipe)
{
    td_u8 k;
    ot_vi_pipe vi_pipe_id;
    isp_drv_ctx *drv_ctx = TD_NULL;
    isp_drv_ctx *drv_ctx_s = TD_NULL;

    drv_ctx = isp_drv_get_ctx(vi_pipe);

    for (k = 0; k < drv_ctx->stitch_attr.stitch_pipe_num; k++) {
        vi_pipe_id = drv_ctx->stitch_attr.stitch_bind_id[k];
        drv_ctx_s = isp_drv_get_ctx(vi_pipe_id);
        if (drv_ctx_s->isp_init != TD_TRUE) {
            return TD_FAILURE;
        }
    }

    return TD_SUCCESS;
}
#endif

static inline td_bool is_wdr_include_pipe(vi_pipe_wdr_attr *wdr_attr, ot_vi_pipe vi_pipe)
{
    td_s32 i;
    for (i = 0; i < wdr_attr->pipe_num; i++) {
        if (vi_pipe == wdr_attr->pipe_id[i]) {
            return TD_TRUE;
        }
    }
    return TD_FALSE;
}

td_s32 isp_get_frame_info(ot_vi_pipe vi_pipe, ot_isp_frame_info *isp_frame)
{
    isp_drv_ctx *drv_ctx = TD_NULL;
    isp_drv_ctx *drv_ctx_s = TD_NULL;
    ot_vi_pipe base_pipe = vi_pipe;
    ot_vi_pipe vi_pipes;

    isp_check_pipe_return(vi_pipe);
    isp_check_pointer_return(isp_frame);

    drv_ctx = isp_drv_get_ctx(vi_pipe);
    if (drv_ctx->wdr_attr.is_mast_pipe == TD_FALSE) {
        for (vi_pipes = 0; vi_pipes < OT_ISP_MAX_PHY_PIPE_NUM; vi_pipes++) {
            drv_ctx_s = isp_drv_get_ctx(vi_pipes);
            if (drv_ctx_s->wdr_attr.is_mast_pipe == TD_TRUE && is_wdr_include_pipe(&drv_ctx_s->wdr_attr, vi_pipe)) {
                base_pipe = vi_pipes;
                break;
            }
        }
    }

    return isp_get_actual_frame_info(base_pipe, isp_frame);
}

td_s32 isp_drv_run_once_process(ot_vi_pipe vi_pipe)
{
    td_s32 ret;
    unsigned long flags = 0;
    isp_drv_ctx *drv_ctx = TD_NULL;
    isp_sync_cfg *sync_cfg = TD_NULL;
    osal_spinlock *isp_spin_lock = TD_NULL;

    isp_check_pipe_return(vi_pipe);

    drv_ctx = isp_drv_get_ctx(vi_pipe);
    if (drv_ctx->mem_init == TD_FALSE) {
        return OT_ERR_ISP_MEM_NOT_INIT;
    }

    /* online mode not support */
    if (isp_is_online_mode(drv_ctx->work_mode.running_mode) == TD_TRUE) {
        isp_err_trace("ISP[%d] run_once not support for online!\n", vi_pipe);
        return OT_ERR_ISP_NOT_SUPPORT;
    }

    sync_cfg = &drv_ctx->sync_cfg;

    ret = isp_drv_get_sync_ctrl_info(vi_pipe, sync_cfg);
    isp_check_ret_continue(vi_pipe, ret, "isp_drv_get_sync_ctrl_info");
    ret = isp_drv_calc_sync_cfg(vi_pipe, sync_cfg);
    isp_check_ret_continue(vi_pipe, ret, "isp_drv_calc_sync_cfg");
    ret = isp_drv_reg_config_isp(vi_pipe, drv_ctx);
    isp_check_ret_continue(vi_pipe, ret, "isp_drv_reg_config_isp");
    ret = isp_drv_reg_config_sensor(vi_pipe, drv_ctx);
    isp_check_ret_continue(vi_pipe, ret, "isp_drv_reg_config_sensor");
    ret = isp_update_info_sync(vi_pipe, &drv_ctx->update_info);
    isp_check_ret_continue(vi_pipe, ret, "isp_update_info_sync");
    ret = isp_frame_info_sync(vi_pipe, &drv_ctx->frame_info);
    isp_check_ret_continue(vi_pipe, ret, "isp_frame_info_sync");
#ifdef CONFIG_OT_ISP_DYNAMIC_BLC_SUPPORT
    isp_drv_reg_config_dynamic_blc(vi_pipe, drv_ctx);
#endif
    isp_spin_lock = isp_drv_get_spin_lock(vi_pipe);
    osal_spin_lock_irqsave(isp_spin_lock, &flags);
    if (drv_ctx->run_once_flag == TD_TRUE) {
        isp_drv_be_buf_queue_put_busy(vi_pipe);
    }
    osal_spin_unlock_irqrestore(isp_spin_lock, &flags);

    drv_ctx->run_once_ok = TD_TRUE;

    return TD_SUCCESS;
}

td_s32 isp_drv_yuv_run_once_process(ot_vi_pipe vi_pipe)
{
    unsigned long flags = 0;
    isp_drv_ctx *drv_ctx = TD_NULL;
    osal_spinlock *isp_spin_lock = TD_NULL;

    isp_check_pipe_return(vi_pipe);

    drv_ctx = isp_drv_get_ctx(vi_pipe);
    if (drv_ctx->mem_init == TD_FALSE) {
        return OT_ERR_ISP_MEM_NOT_INIT;
    }

    /* online mode not support */
    if (isp_is_online_mode(drv_ctx->work_mode.running_mode) == TD_TRUE) {
        isp_err_trace("ISP[%d] run_once not support for online!\n", vi_pipe);
        return OT_ERR_ISP_NOT_SUPPORT;
    }

    isp_spin_lock = isp_drv_get_spin_lock(vi_pipe);
    osal_spin_lock_irqsave(isp_spin_lock, &flags);
    if (drv_ctx->yuv_run_once_flag == TD_TRUE) {
        isp_drv_be_buf_queue_put_busy(vi_pipe);
    }
    osal_spin_unlock_irqrestore(isp_spin_lock, &flags);

    drv_ctx->yuv_run_once_ok = TD_TRUE;

    return TD_SUCCESS;
}

static td_s32 isp_run_trigger_process_single(ot_vi_pipe vi_pipe)
{
    td_s32 ret;
    isp_sync_cfg *sync_cfg = TD_NULL;
    isp_drv_ctx *drv_ctx = TD_NULL;

    drv_ctx = isp_drv_get_ctx(vi_pipe);
    if (drv_ctx->mem_init == TD_FALSE) {
        return OT_ERR_ISP_MEM_NOT_INIT;
    }

    sync_cfg = &drv_ctx->sync_cfg;

    ret = isp_drv_get_sync_ctrl_info(vi_pipe, sync_cfg);
    isp_check_ret_continue(vi_pipe, ret, "isp_drv_get_sync_controlnfo");

    ret = isp_drv_calc_sync_cfg(vi_pipe, sync_cfg);
    isp_check_ret_continue(vi_pipe, ret, "isp_drv_calc_sync_cfg");

    ret = isp_update_info_sync(vi_pipe, &drv_ctx->update_info);
    isp_check_ret_continue(vi_pipe, ret, "isp_update_info_sync");
    ret = isp_frame_info_sync(vi_pipe, &drv_ctx->frame_info);
    isp_check_ret_continue(vi_pipe, ret, "isp_frame_info_sync");

    return ret;
}

td_s32 isp_drv_run_trigger_process(ot_vi_pipe vi_pipe)
{
    td_s32 ret = TD_FAILURE;
    isp_drv_ctx *drv_ctx = TD_NULL;
#ifdef CONFIG_OT_SYS_KOL_SUPPORT
    unsigned long flags_tmp = 0;
#endif
    isp_check_pipe_return(vi_pipe);

    drv_ctx = isp_drv_get_ctx(vi_pipe);
    if (drv_ctx->mem_init == TD_FALSE) {
        return OT_ERR_ISP_MEM_NOT_INIT;
    }

    if (drv_ctx->stitch_attr.stitch_enable == TD_TRUE) {
#ifdef CONFIG_OT_VI_STITCH_GRP
        td_s32 i;
        unsigned long flags = 0;

        if (drv_ctx->stitch_attr.main_pipe != TD_TRUE) {
            isp_err_trace("stich pipes should call run trigger process just in main pipe(%d), current pipe(%d).\n",
                drv_ctx->stitch_attr.stitch_bind_id[0], vi_pipe);
            return TD_FAILURE;
        }

        osal_spin_lock_irqsave(&g_isp_sync_lock[vi_pipe], &flags);
        for (i = 0; i < drv_ctx->stitch_attr.stitch_pipe_num; i++) {
            ret = isp_run_trigger_process_single(drv_ctx->stitch_attr.stitch_bind_id[i]);
        }
        osal_spin_unlock_irqrestore(&g_isp_sync_lock[vi_pipe], &flags);
#endif
    } else {
        ret = isp_run_trigger_process_single(vi_pipe);
    }
#ifdef CONFIG_OT_SYS_KOL_SUPPORT
    osal_spin_lock_irqsave(&g_isp_lock[vi_pipe], &flags_tmp);
    drv_ctx->vd_run_end = TD_TRUE;
    osal_wait_wakeup(&drv_ctx->isp_wait_vd_run_end);
    osal_spin_unlock_irqrestore(&g_isp_lock[vi_pipe], &flags_tmp);
#endif
    return ret;
}

td_s32 isp_drv_opt_run_once_info(ot_vi_pipe vi_pipe, td_bool *run_once)
{
    td_bool run_once_flag;
    isp_drv_ctx *drv_ctx = TD_NULL;
    unsigned long flags = 0;

    isp_check_pipe_return(vi_pipe);
    isp_check_pointer_return(run_once);

    drv_ctx = isp_drv_get_ctx(vi_pipe);
    /* online mode not support */
    if (isp_is_online_mode(drv_ctx->work_mode.running_mode) == TD_TRUE) {
        isp_err_trace("ISP[%d] run_once not support for online!\n", vi_pipe);
        return OT_ERR_ISP_NOT_SUPPORT;
    }

    osal_spin_lock_irqsave(&g_isp_lock[vi_pipe], &flags);

    run_once_flag = *run_once;

    if (run_once_flag == TD_TRUE) {
        if (isp_drv_prepare_be_buf(vi_pipe, drv_ctx) != TD_SUCCESS) {
            osal_spin_unlock_irqrestore(&g_isp_lock[vi_pipe], &flags);
            return TD_FAILURE;
        }
    }

    drv_ctx->run_once_flag = run_once_flag;
    /* runonce, set vd_be_end here to prevent that be_end is earlier than get_vd_timeout in case of Low resolution */
    /* then in this case, may wait for get_vd_timeout timeout */
    drv_ctx->vd_be_end = TD_FALSE;

    osal_spin_unlock_irqrestore(&g_isp_lock[vi_pipe], &flags);

    return TD_SUCCESS;
}

td_s32 isp_drv_yuv_run_once_info(ot_vi_pipe vi_pipe, td_bool *run_once)
{
    td_bool yuv_run_once_flag;
    isp_drv_ctx *drv_ctx = TD_NULL;
    unsigned long flags = 0;

    isp_check_pipe_return(vi_pipe);
    isp_check_pointer_return(run_once);

    drv_ctx = isp_drv_get_ctx(vi_pipe);
    /* online mode not support */
    if (isp_is_online_mode(drv_ctx->work_mode.running_mode) == TD_TRUE) {
        isp_err_trace("ISP[%d] run_once not support for online!\n", vi_pipe);
        return OT_ERR_ISP_NOT_SUPPORT;
    }

    osal_spin_lock_irqsave(&g_isp_lock[vi_pipe], &flags);

    yuv_run_once_flag = *run_once;

    if (yuv_run_once_flag == TD_TRUE) {
        if (drv_ctx->use_node != TD_NULL) {
            osal_spin_unlock_irqrestore(&g_isp_lock[vi_pipe], &flags);
            isp_err_trace("Pipe[%d] isp is running!\r\n", vi_pipe);
            return TD_FAILURE;
        }

        if (isp_drv_get_free_be_buf(vi_pipe) != TD_SUCCESS) {
            osal_spin_unlock_irqrestore(&g_isp_lock[vi_pipe], &flags);
            isp_err_trace("Pipe[%d] get FreeBeBuf is fail!, busy:%d, free:%d, use:%d\r\n",
                vi_pipe, drv_ctx->be_buf_queue.busy_num, drv_ctx->be_buf_queue.free_num,
                drv_ctx->be_buf_info.use_cnt);
            return TD_FAILURE;
        }
    }

    drv_ctx->yuv_run_once_flag = yuv_run_once_flag;

    osal_spin_unlock_irqrestore(&g_isp_lock[vi_pipe], &flags);

    return TD_SUCCESS;
}

#ifdef CONFIG_OT_AIBNR_SUPPORT
td_s32 isp_drv_update_aibnr_be_cfg(ot_vi_pipe vi_pipe, td_void *be_node, isp_aibnr_cfg *aibnr_cfg)
{
    isp_drv_ctx *drv_ctx = TD_NULL;

    isp_check_pipe_return(vi_pipe);
    isp_check_pointer_return(be_node);
    isp_check_pointer_return(aibnr_cfg);
    drv_ctx = isp_drv_get_ctx(vi_pipe);
    if ((isp_drv_get_alg_run_select(vi_pipe) == OT_ISP_ALG_RUN_FE_ONLY) && (drv_ctx->yuv_mode != TD_TRUE)) {
        return TD_SUCCESS;
    }

    if (drv_ctx->isp_init != TD_TRUE) {
        return TD_FAILURE;
    }
    drv_ctx->aibnr_info.aibnr_en = aibnr_cfg->aibnr_en;
    drv_ctx->aibnr_info.blend_en = aibnr_cfg->blend_en;
    isp_drv_set_aibnr_bnr_cfg((isp_be_wo_reg_cfg *)be_node, drv_ctx, aibnr_cfg);

    return TD_SUCCESS;
}
#endif

#ifdef CONFIG_OT_AIDRC_SUPPORT
td_s32 isp_drv_update_aidrc_be_cfg(ot_vi_pipe vi_pipe, td_void *be_node, isp_aidrc_cfg *cfg)
{
    td_u8 cfg_node_idx;
    isp_sync_cfg_buf_node *cfg_node = TD_NULL;
    isp_drv_ctx *drv_ctx = TD_NULL;
    isp_check_pipe_return(vi_pipe);
    drv_ctx = isp_drv_get_ctx(vi_pipe);

    cfg_node_idx = MIN2(isp_drv_get_be_sync_index(vi_pipe, drv_ctx), CFG2VLD_DLY_LIMIT - 1);
    cfg_node = drv_ctx->sync_cfg.node[cfg_node_idx];
    isp_check_pointer_success_return(cfg_node);
    isp_check_pipe_return(vi_pipe);
    isp_check_pointer_return(be_node);
    isp_check_pointer_return(cfg);

    drv_ctx = isp_drv_get_ctx(vi_pipe);
    if (drv_ctx->isp_init != TD_TRUE) {
        return TD_FAILURE;
    }
    drv_ctx->aidrc_info.aidrc_en = cfg->aidrc_en;
    drv_ctx->aidrc_info.mode = cfg->mode;
    drv_ctx->aidrc_info.pending_idx = cfg->pending_idx;
    drv_ctx->aidrc_info.pending_stat = cfg->pending_stat;
    isp_drv_set_aidrc_drc_cfg((isp_be_wo_reg_cfg *)be_node, drv_ctx, &cfg_node->drc_reg_cfg);

    return TD_SUCCESS;
}
#endif

static td_void isp_drv_reset_stitch_ctx(isp_drv_ctx *drv_ctx)
{
#ifdef CONFIG_OT_VI_STITCH_GRP
    td_u8 i;
    ot_vi_pipe stitch_pipe_id;
    isp_drv_ctx *stitch_drv_ctx = TD_NULL;

    if (drv_ctx->stitch_attr.stitch_enable != TD_TRUE) {
        return;
    }
    if ((drv_ctx->stitch_attr.main_pipe != TD_TRUE)) {
        drv_ctx->stitch_attr.stitch_enable = TD_FALSE;
        return;
    }

    for (i = 0; i < drv_ctx->stitch_attr.stitch_pipe_num; i++) {
        stitch_pipe_id = drv_ctx->stitch_attr.stitch_bind_id[i];
        if ((stitch_pipe_id < 0) || (stitch_pipe_id >= OT_ISP_MAX_PIPE_NUM)) {
            return;
        }
        stitch_drv_ctx = isp_drv_get_ctx(stitch_pipe_id);
        stitch_drv_ctx->stitch_sync = TD_FALSE;
    }

    drv_ctx->stitch_attr.stitch_enable = TD_FALSE;
#endif
    return;
}

static td_void isp_drv_reset_snap_ctx(isp_drv_ctx *drv_ctx)
{
#ifdef CONFIG_OT_SNAP_SUPPORT
    drv_ctx->snap_attr.snap_type = OT_SNAP_TYPE_NORM;
    drv_ctx->snap_attr.picture_pipe_id = -1;
    drv_ctx->snap_attr.preview_pipe_id = -1;
    drv_ctx->snap_attr.load_ccm = TD_TRUE;
    drv_ctx->snap_attr.pro_param.operation_mode = OT_OP_MODE_AUTO;
#else
    ot_unused(drv_ctx);
#endif
}

static td_void isp_drv_reset_param(ot_vi_pipe vi_pipe)
{
    g_int_bottom_half = TD_FALSE;
    g_use_bottom_half = TD_FALSE;
    g_proc_param[vi_pipe] = PROC_PARAM_DEFAULT;
    g_stat_intvl[vi_pipe] = STAT_INTVL_DEFAULT;
    g_update_pos[vi_pipe] = UPDATE_POS_DEFAULT;
    g_int_timeout[vi_pipe] = INT_TIME_OUT_DEFAULT;
    g_pwm_number[vi_pipe] = PWM_CHN_IDX_DEFAULT;
    g_port_int_delay[vi_pipe] = PORT_INY_DELAY_DEFAULT;
    g_ldci_tpr_flt_en[vi_pipe] = TD_FALSE;
    g_be_buf_num[vi_pipe] = BE_BUF_NUM_DEFAULT;
    g_ob_stats_update_pos[vi_pipe] = OT_ISP_UPDATE_OB_STATS_FE_FRAME_END; // VALUE 0
    g_isp_alg_run_select[vi_pipe] = OT_ISP_ALG_RUN_NORM;
    g_quick_start[vi_pipe] = TD_FALSE;
    g_long_frm_int_en[vi_pipe] = TD_FALSE;
    g_isp_run_wakeup_sel[vi_pipe]  = OT_ISP_RUN_WAKEUP_FE_START;
}

static td_void isp_drv_reset_aibnr_info(isp_drv_ctx *drv_ctx)
{
#if defined(CONFIG_OT_AIISP_SUPPORT) && defined(CONFIG_OT_AIBNR_SUPPORT)
    drv_ctx->aibnr_info.aibnr_en = TD_FALSE;
    drv_ctx->aibnr_info.pre_aibnr_en = TD_FALSE;
    drv_ctx->aibnr_info.pre_isp_dgain = 0;
    drv_ctx->aibnr_info.switch_aibnr_cnt = 0;
    drv_ctx->aibnr_info.off_switch_cnt = 0;
    drv_ctx->aibnr_info.blend_en = TD_FALSE;
#endif
}

static td_void isp_drv_reset_vd(isp_drv_ctx *drv_ctx)
{
    drv_ctx->edge = TD_FALSE;
    drv_ctx->vd_start = TD_FALSE;
    drv_ctx->vd_end = TD_FALSE;
    drv_ctx->vd_be_end = TD_FALSE;
    drv_ctx->vd_run_end = TD_FALSE;
    drv_ctx->vd_standby_end = TD_TRUE;
}
#ifdef CONFIG_OT_ISP_DRC_SWITCH_SUPPORT
static td_void isp_drv_reset_drc(isp_drv_ctx *drv_ctx)
{
    drv_ctx->drc_freeze = TD_FALSE;
}
#endif

static td_void isp_drv_reset_flag(isp_drv_ctx *drv_ctx)
{
    drv_ctx->frame_cnt = 0;
    drv_ctx->mem_init = TD_FALSE;
    drv_ctx->isp_init = TD_FALSE;
    drv_ctx->pub_attr_ok = TD_FALSE;
    drv_ctx->run_once_ok = TD_FALSE;
    drv_ctx->run_once_flag = TD_FALSE;
    drv_ctx->yuv_run_once_ok = TD_FALSE;
    drv_ctx->yuv_run_once_flag = TD_FALSE;
    drv_ctx->int_pos = 0;
    drv_ctx->fpn_work_mode = FPN_MODE_NONE;
    drv_ctx->pre_fpn_cor_en = TD_FALSE;
    drv_ctx->sync_cfg.total_cnt = RUN_BE_SYNC_ID_MIN;
}
td_s32 isp_drv_reset_ctx(ot_vi_pipe vi_pipe)
{
    td_u8 i;
    td_s32 ret;
    unsigned long flags = 0;
    isp_drv_ctx *drv_ctx = TD_NULL;

    isp_check_pipe_return(vi_pipe);
    drv_ctx = isp_drv_get_ctx(vi_pipe);
    if (drv_ctx->isp_run_flag == TD_TRUE) {
        isp_err_trace("ISP[%d] Should set isp_run_flag to TD_FALSE first!!\n", vi_pipe);
        return TD_FAILURE;
    }

    osal_spin_lock_irqsave(&g_isp_lock[vi_pipe], &flags);

    isp_drv_reset_flag(drv_ctx);

    isp_drv_reset_aibnr_info(drv_ctx);
    isp_drv_reset_stitch_ctx(drv_ctx);

    for (i = 0; i < OT_ISP_STRIPING_MAX_NUM; i++) {
        drv_ctx->chn_sel_attr[i].channel_sel = 0;
    }

    isp_drv_reset_snap_ctx(drv_ctx);

    ret = isp_drv_mem_share_exit(vi_pipe);
    if (ret != TD_SUCCESS) {
        isp_err_trace("mem_share exit failed!\n");
    }

    (td_void)memset_s(&drv_ctx->bnr_tpr_filt, sizeof(isp_kernel_tpr_filt_reg), 0, sizeof(isp_kernel_tpr_filt_reg));
    (td_void)memset_s(&drv_ctx->dyna_blc_info, sizeof(isp_dynamic_blc_info), 0, sizeof(isp_dynamic_blc_info));
    (td_void)memset_s(&drv_ctx->proc_frame_info, sizeof(isp_proc_frame_info), 0, sizeof(isp_proc_frame_info));
    (td_void)memset_s(&drv_ctx->wdr_attr, sizeof(vi_pipe_wdr_attr), 0, sizeof(vi_pipe_wdr_attr));
#ifdef CONFIG_OT_ISP_DETAIL_STATS_SUPPORT
    isp_drv_reset_detail_stats_cfg(vi_pipe);
#endif
    isp_drv_reset_vd(drv_ctx);
#ifdef CONFIG_OT_ISP_DRC_SWITCH_SUPPORT
    isp_drv_reset_drc(drv_ctx);
#endif
#ifdef CONFIG_OT_VI_DISTRIBUTE_GRP
    drv_ctx->is_chg_fps_cfg = TD_FALSE;
    drv_ctx->is_chg_fps_done = TD_TRUE;
    drv_ctx->chg_fps_cnt = 0;
#endif

    isp_drv_reset_param(vi_pipe);
    osal_spin_unlock_irqrestore(&g_isp_lock[vi_pipe], &flags);

    return TD_SUCCESS;
}

static int isp_open(void *data)
{
    ot_unused(data);
    return 0;
}

static int isp_close(void *data)
{
    ot_unused(data);
    return 0;
}

static td_s32 isp_check_reg_addr(td_phys_addr_t phy_addr, td_s32 size)
{
    if (size <= 0) {
        return TD_FAILURE;
    }
    if (phy_addr + size < phy_addr) { // overflow
        return TD_FAILURE;
    }

    /* check vicap reg addr */
    if ((phy_addr >= VICAP_REG_BASE) && ((phy_addr + size) <= (VICAP_REG_BASE + VICAP_REG_SIZE_ALIGN))) {
        return TD_SUCCESS;
    }

    /* check viproc reg addr */
    if ((phy_addr >= VIPROC_REG_BASE) && ((phy_addr + size) <= (VIPROC_REG_BASE +
        VIPROC_REG_SIZE_ALIGN * ISP_MAX_BE_NUM))) { // VIPROC_REG_SIZE_ALIGN is size of 1 viproc
        return TD_SUCCESS;
    }

    return TD_FAILURE;
}

static td_s32 isp_check_mmap_pid(td_void)
{
    ot_vi_pipe i;

    for (i = 0; i < OT_ISP_MAX_PIPE_NUM; ++i) {
        if (isp_drv_mem_verify_pid(i, osal_get_current_tgid()) == TD_SUCCESS) {
            return TD_SUCCESS;
        }
    }

    return TD_FAILURE;
}

static int isp_mmap(osal_vm *vm, unsigned long start, unsigned long end,
    unsigned long vm_pgoff, td_void *private_data)
{
    td_s32 size = end - start;
    td_phys_addr_t phy_addr;

    ot_unused(private_data);

    phy_addr = (td_phys_addr_t)(vm_pgoff << SYS_PAGE_SHIFT);

    if (isp_check_reg_addr(phy_addr, size) != TD_SUCCESS) {
        isp_err_trace("invalid phyaddr, addr:%#lx, size:%d!\n", (td_ulong)phy_addr, size);
        return TD_FAILURE;
    }

    if (isp_check_mmap_pid() != TD_SUCCESS) {
        isp_err_trace("check mmap pid%d fail \n", osal_get_current_tgid());
        return TD_FAILURE;
    }

    osal_pgprot_noncached(vm);
    if (osal_remap_pfn_range(vm, start, vm_pgoff, size)) {
        isp_err_trace("remap error \n");
        return TD_FAILURE;
    }

    return TD_SUCCESS;
}

static osal_fileops g_isp_file_ops = {
    .open = isp_open,
    .release = isp_close,
    .mmap = isp_mmap,
};

static osal_dev *g_isp_device = TD_NULL;

#ifdef CONFIG_OT_ISP_LITEOS_BOTTOM_HALF_SUPPORT
static void isp_drv_work_queue_handler(osal_workqueue *worker)
{
    td_s32 ret;
    isp_work_queue_ctx *isp_work_queue = osal_container_of((void *)worker, isp_work_queue_ctx, worker);
    if (osal_sem_down(&isp_work_queue->sem)) {
        return;
    }
    ret = isp_drv_int_bottom_half(0);
    if (ret != OSAL_IRQ_HANDLED) {
    }
    osal_sem_up(&isp_work_queue->sem);

    return;
}

void isp_drv_work_queue_run(ot_vi_pipe vi_pipe)
{
    osal_workqueue_schedule(&g_isp_work_queue_ctx.worker);
}

td_s32 isp_drv_work_queue_init(void)
{
    if (osal_sem_init(&g_isp_work_queue_ctx.sem, 1) != TD_SUCCESS) {
        isp_err_trace("sem init error\n");
        return TD_FAILURE;
    }

    if (osal_workqueue_init(&g_isp_work_queue_ctx.worker, isp_drv_work_queue_handler) != TD_SUCCESS) {
        osal_sem_destroy(&g_isp_work_queue_ctx.sem);
        isp_err_trace("workqueue init error\n");
        return TD_FAILURE;
    }

    return TD_SUCCESS;
}

void isp_drv_work_queue_exit(void)
{
    osal_sem_destroy(&g_isp_work_queue_ctx.sem);
    osal_workqueue_destroy(&g_isp_work_queue_ctx.worker);
}
#endif

static td_void isp_update_interrupt_info(ot_vi_pipe vi_pipe, isp_drv_ctx *drv_ctx, isp_interrupt_sch *int_sch)
{
    td_u8 write_flag;
    unsigned long flags = 0;

    osal_spin_lock_irqsave(&g_isp_lock[vi_pipe], &flags);
    if (isp_interrupt_buf_is_full(&drv_ctx->isp_int_info)) {
        isp_warn_trace("ISP[%d] interrupts buf is full!\n", vi_pipe);
        (td_void)memset_s(&drv_ctx->isp_int_info, sizeof(isp_interrupt_info), 0, sizeof(isp_interrupt_info));
    }

    (td_void)memset_s(&drv_ctx->isp_int_info.int_sch[drv_ctx->isp_int_info.write_flag], sizeof(isp_interrupt_sch), 0,
        sizeof(isp_interrupt_sch));

    if (int_sch->isp_int_status || int_sch->port_int_status ||
        ((int_sch->wch_int_status != 0) && (drv_ctx->yuv_mode == TD_TRUE))) {
        write_flag = drv_ctx->isp_int_info.write_flag;
        (td_void)memcpy_s(&drv_ctx->isp_int_info.int_sch[write_flag], sizeof(isp_interrupt_sch), int_sch,
            sizeof(isp_interrupt_sch));
        drv_ctx->isp_int_info.write_flag = (write_flag + 1) % ISP_INTERRUPTS_SAVEINFO_MAX;
    }

    osal_spin_unlock_irqrestore(&g_isp_lock[vi_pipe], &flags);
}

static td_void isp_bottom_half_cross_frame(ot_vi_pipe vi_pipe, isp_drv_ctx *drv_ctx)
{
    unsigned long flags = 0;
    td_s8 w_flag_num = 0;
    td_s8 w_flag_end = 0;
    td_u8 r_flag_num = 0;
    td_u8 count = 0;
    td_s8 cur_write_flag = 0;

    osal_spin_lock_irqsave(&g_isp_lock[vi_pipe], &flags);

    cur_write_flag = drv_ctx->isp_int_info.write_flag == 0 ? ISP_INTERRUPTS_SAVEINFO_MAX - 1
        : drv_ctx->isp_int_info.write_flag;

    r_flag_num = drv_ctx->isp_int_info.read_flag;

    if (r_flag_num == drv_ctx->isp_int_info.write_flag) {
        osal_spin_unlock_irqrestore(&g_isp_lock[vi_pipe], &flags);
        return;
    } else {
        while (r_flag_num != cur_write_flag && count < ISP_INTERRUPTS_SAVEINFO_MAX) {
            count++;
            r_flag_num = (r_flag_num + 1) % ISP_INTERRUPTS_SAVEINFO_MAX;
        }
    }

    if (count >= CROSS_FRAME_FLAG) {
        w_flag_num = cur_write_flag;

        if ((drv_ctx->isp_int_info.int_sch[w_flag_num].isp_int_status & FE_INT_FSTART) == 1) {
            drv_ctx->bottom_half_cross_frame = TD_TRUE;
        } else {
            while (((drv_ctx->isp_int_info.int_sch[w_flag_num].isp_int_status & FE_INT_FSTART) != 1) &&
                (w_flag_end < ISP_INTERRUPTS_SAVEINFO_MAX)) {
                w_flag_num = w_flag_num - 1 < 0 ? ISP_INTERRUPTS_SAVEINFO_MAX - 1 : w_flag_num - 1;
                w_flag_end++;
            }
        }
        drv_ctx->isp_int_info.read_flag = (td_u8)w_flag_num;
    }
    osal_spin_unlock_irqrestore(&g_isp_lock[vi_pipe], &flags);
}

static td_s32 isp_get_interrupt_info(ot_vi_pipe vi_pipe, isp_drv_ctx *drv_ctx, td_u32 *int_read_num)
{
    td_u8 read_flag;
    unsigned long flags = 0;
    isp_interrupt_sch *int_sch = TD_NULL;

    isp_bottom_half_cross_frame(vi_pipe, drv_ctx);

    while (drv_ctx->isp_int_info.read_flag != drv_ctx->isp_int_info.write_flag) {
        osal_spin_lock_irqsave(&g_isp_lock[vi_pipe], &flags);
        read_flag = drv_ctx->isp_int_info.read_flag;
        int_sch = &drv_ctx->isp_int_info.int_sch[read_flag];

        if ((!int_sch->isp_int_status) && (!int_sch->port_int_status) &&
            (!(int_sch->wch_int_status && (drv_ctx->yuv_mode == TD_TRUE)))) {
            osal_spin_unlock_irqrestore(&g_isp_lock[vi_pipe], &flags);
            return 0;
        }

        if (isp_interrupt_buf_is_empty(&drv_ctx->isp_int_info)) {
            isp_warn_trace("ISP[%d] interrupts buf is empty\n", vi_pipe);
            osal_spin_unlock_irqrestore(&g_isp_lock[vi_pipe], &flags);
            return 0;
        }

        (td_void)memcpy_s(&drv_ctx->int_sch[*int_read_num], sizeof(isp_interrupt_sch),
            &drv_ctx->isp_int_info.int_sch[read_flag], sizeof(isp_interrupt_sch));
        (td_void)memset_s(&drv_ctx->isp_int_info.int_sch[read_flag], sizeof(isp_interrupt_sch), 0,
            sizeof(isp_interrupt_sch));
        drv_ctx->isp_int_info.read_flag = (read_flag + 1) % ISP_INTERRUPTS_SAVEINFO_MAX;
        (*int_read_num)++;
        osal_spin_unlock_irqrestore(&g_isp_lock[vi_pipe], &flags);
    }

    return 1;
}

td_s32 isp_drv_int_clear(ot_vi_pipe vi_pipe, td_s32 vi_dev, td_u32 port_int, td_u32 isp_raw_int)
{
    if (!port_int && !isp_raw_int) {
        return TD_FAILURE;
    }

    if (port_int) {
        io_rw_pt_address(vi_pt_base(vi_dev) + VI_PT_INT) = port_int;
    }

    if (isp_raw_int) {
        io_rw_fe_address(vi_pipe, ISP_INT_FE) = isp_raw_int;
    }

    return TD_SUCCESS;
}

#ifdef CONFIG_OT_SYS_KOL_SUPPORT
static td_void isp_drv_single_mode_update_int(isp_drv_ctx *drv_ctx, isp_interrupt_sch *int_sch, td_u32 wch_int_mask,
    td_u32 wch_int_status)
{
    td_u32 wch_lowdelay_status, isp_int_status, port_int_status;

    /* VI_WCH_INT_FSTART or VI_WCH_INT_DELAY0 */
    if (drv_ctx->kol_int_info.kol_int_num[ISP_INT_IDX_CHN_START] == 1) {
        int_sch->wch_int_status &= ~(wch_int_mask);
        int_sch->isp_int_status &= ~(wch_int_mask);
    } else if (wch_int_status > 0) {
        drv_ctx->kol_int_info.kol_int_num[ISP_INT_IDX_CHN_START]++;
    }
    /* VI_WCH_INT_LOW_DELAY */
    wch_lowdelay_status = (int_sch->wch_int_status & VI_WCH_INT_LOW_DELAY);
    if (drv_ctx->kol_int_info.kol_int_num[ISP_INT_IDX_CHN_LOWDELAY] == 1) {
        int_sch->wch_int_status &= ~(VI_WCH_INT_LOW_DELAY);
        int_sch->isp_int_status &= ~(VI_WCH_INT_LOW_DELAY);
    } else if (wch_lowdelay_status > 0) {
        drv_ctx->kol_int_info.kol_int_num[ISP_INT_IDX_CHN_LOWDELAY]++;
    }
    /* ISP_INT_FE_DYNABLC_END */
    isp_int_status = (int_sch->isp_int_status & ISP_INT_FE_DYNABLC_END);
    if (drv_ctx->kol_int_info.kol_int_num[ISP_INT_IDX_FE_DYNBLC_END] == 1) {
        int_sch->isp_int_status &= ~(ISP_INT_FE_DYNABLC_END);
    } else if (isp_int_status > 0) {
        drv_ctx->kol_int_info.kol_int_num[ISP_INT_IDX_FE_DYNBLC_END]++;
    }
    /* ISP_INT_FE_DELAY */
    isp_int_status = (int_sch->isp_int_status & ISP_INT_FE_DELAY);
    if (drv_ctx->kol_int_info.kol_int_num[ISP_INT_IDX_FE_DELAY] == 1) {
        int_sch->isp_int_status &= ~(ISP_INT_FE_DELAY);
    } else if (isp_int_status > 0) {
        drv_ctx->kol_int_info.kol_int_num[ISP_INT_IDX_FE_DELAY]++;
    }
    /* FE_INT_FEND */
    isp_int_status = (int_sch->isp_int_status & FE_INT_FEND);
    if (drv_ctx->kol_int_info.kol_int_num[ISP_INT_IDX_FE_END] == 1) {
        int_sch->isp_int_status &= ~(FE_INT_FEND);
    } else if (isp_int_status > 0) {
        drv_ctx->kol_int_info.kol_int_num[ISP_INT_IDX_FE_END]++;
    }
    /* VI_PT_INT_FSTART */
    port_int_status = (int_sch->port_int_status & VI_PT_INT_FSTART);
    if (drv_ctx->kol_int_info.kol_int_num[ISP_INT_IDX_PORT] == 1) {
        int_sch->port_int_status &= ~(VI_PT_INT_FSTART);
    } else if (port_int_status > 0) {
        drv_ctx->kol_int_info.kol_int_num[ISP_INT_IDX_PORT]++;
    }
}

static td_void isp_drv_single_mode_update_int_info(ot_vi_pipe vi_pipe, isp_drv_ctx *drv_ctx,
    isp_interrupt_sch *int_sch)
{
    td_u32 wch_int_mask, wch_int_status;
    ot_syskol_param param = {0};

    if (!ckfn_sys_query_kol_param() || (call_sys_query_kol_param(&param) != TD_SUCCESS)) {
        isp_err_trace("isp[%d] sys is not exist\n", vi_pipe);
    }
    drv_ctx->kol_int_info.mode = param.capture_mode;
    if (param.capture_mode != OT_SYSKOL_CAPTURE_MODE_SINGLE) {
        return;
    }

    wch_int_status = 0;
    wch_int_mask = (g_isp_run_wakeup_sel[vi_pipe] == OT_ISP_RUN_WAKEUP_FE_START ||
        drv_ctx->frame_int_attr.interrupt_type == OT_FRAME_INTERRUPT_START) ? VI_WCH_INT_FSTART : VI_WCH_INT_DELAY0;
    wch_int_status = (int_sch->wch_int_status & wch_int_mask);

    isp_drv_single_mode_update_int(drv_ctx, int_sch, wch_int_mask, wch_int_status);

    isp_drv_dfx_check_kol_int_num(vi_pipe, drv_ctx, wch_int_status, int_sch);

    return;
}
#endif
static td_bool isp_is_port_int_valid(td_s32 vi_dev, td_u32 vicap_int_status)
{
    if (((vicap_int_status) & (ISP_VICAP_INT_PORT0 << (vi_dev))) > 0) {
        return TD_TRUE;
    }
    return TD_FALSE;
}

static td_bool isp_is_fe_int_valid(ot_vi_pipe vi_pipe, td_u32 vicap_int_status)
{
    if (((vicap_int_status) & (ISP_VICAP_INT_ISP0 << (vi_pipe))) > 0) {
        return TD_TRUE;
    }
    return TD_FALSE;
}

static td_void isp_drv_get_fe_int_status(ot_vi_pipe vi_pipe, isp_drv_ctx *drv_ctx, td_u32 *isp_raw_int,
    td_u32 *isp_int_status)
{
    td_u32 raw_int;
    raw_int = io_rw_fe_address(vi_pipe, ISP_INT_FE);
    if (is_online_mode(drv_ctx->work_mode.running_mode) && ((raw_int & ISP_INT_FE_CFG_LOSS) > 0)) {
        drv_ctx->drv_dbg_info.cross_frame_cnt++;
        isp_warn_trace("cross frame, loss cfg, raw_int:%d\n", raw_int);
    }
    *isp_raw_int = raw_int;
    *isp_int_status = raw_int & io_rw_fe_address(vi_pipe, ISP_INT_FE_MASK);
}

td_void isp_update_pre_on_post_off_int_status(isp_interrupt_sch *int_sch)
{
#ifdef CONFIG_OT_VI_PREONLINE_POSTOFFLINE
    td_u32 be_pre_int = isp_drv_get_pre_proc_fstart_status();
    int_sch->isp_int_status |= be_pre_int; /* when in this mode, only use be_pre frame start interrupt */
                                          /* replace fe frame start interrupt */
    if (be_pre_int) {
        isp_drv_clear_pre_proc_fstart_int();
    }
#endif
}

td_s32 isp_drv_int_status_process(ot_vi_pipe phy_pipe,  td_s32 vi_dev, isp_drv_ctx *drv_ctx, td_u32 vicap_int_status)
{
    td_u32 port_int, port_int_f_start, port_int_err, isp_raw_int, isp_int_status, wch_int, wch_int_valid;
    td_u32 wch_int_mask;
    isp_interrupt_sch int_sch = { 0 };
#ifdef CONFIG_OT_VI_DISTRIBUTE_GRP
    ot_vi_pipe pipe = drv_ctx->dist_proc_pipe;
#else
    ot_vi_pipe pipe = phy_pipe;
#endif

    /* read interrupt status */
    port_int = 0;
    /* two int, in order to Prevent reentry, need to according to vicap_int_status to check it's port int or not. */
    /* When re-entering, it is impossible to determine whether to clear the interrupt based on the sub-interrupt, */
    /* which may result in the incorrect clearing of the interrupt on int0 while int1 is active, */
    /* causing int0 to not respond. */
    if (isp_is_port_int_valid(vi_dev, vicap_int_status)) {
        port_int = io_rw_pt_address(vi_pt_base(vi_dev) + VI_PT_INT);
    }
    port_int &= io_rw_pt_address(vi_pt_base(vi_dev) + VI_PT_INT_MASK);
    port_int_f_start = port_int & VI_PT_INT_FSTART;
    port_int_err = port_int & VI_PT_INT_ERR;

    if ((g_mask_is_open[phy_pipe] == TD_TRUE) && isp_is_chn_int(phy_pipe, vicap_int_status)) {
        wch_int = io_rw_ch_address(phy_pipe, VI_WCH_INT);
        wch_int_mask = (g_isp_run_wakeup_sel[pipe] == OT_ISP_RUN_WAKEUP_FE_START ||
            drv_ctx->frame_int_attr.interrupt_type == OT_FRAME_INTERRUPT_START) ? VI_WCH_INT_FSTART : VI_WCH_INT_DELAY0;
        wch_int_mask |= VI_WCH_INT_LOW_DELAY;
        wch_int_valid = wch_int & wch_int_mask;
    } else {
        wch_int_valid = 0;
    }

    // same to port int, need to check isp_is_fe_int_valid
    if (is_no_fe_phy_pipe(phy_pipe) == TD_FALSE && isp_is_fe_int_valid(phy_pipe, vicap_int_status)) {
        isp_drv_get_fe_int_status(phy_pipe, drv_ctx, &isp_raw_int, &isp_int_status);
    } else {
        isp_raw_int = 0;
        isp_int_status = 0;
    }

    int_sch.isp_int_status = isp_int_status;

    if (drv_ctx->work_mode.running_mode == ISP_MODE_RUNNING_PRE_ONLINE_POST_OFFLINE) {
        isp_update_pre_on_post_off_int_status(&int_sch);
    } else {
        int_sch.isp_int_status |= wch_int_valid;
    }

    int_sch.port_int_status = port_int_f_start;
    int_sch.port_int_err = port_int_err;
    int_sch.wch_int_status = wch_int_valid;
#ifdef CONFIG_OT_SYS_KOL_SUPPORT
    isp_drv_single_mode_update_int_info(pipe, drv_ctx, &int_sch);
#endif

    isp_update_interrupt_info(pipe, drv_ctx, &int_sch);

    /* clear interrupt */
    if (isp_drv_int_clear(phy_pipe, vi_dev, port_int, isp_raw_int) != TD_SUCCESS) {
        return TD_FAILURE;
    }

    if (port_int_err) {
        drv_ctx->drv_dbg_info.isp_reset_cnt++;
        return TD_FAILURE;
    }

    return TD_SUCCESS;
}

#ifdef CONFIG_OT_VI_DISTRIBUTE_GRP
static td_void isp_dist_update_fps_status(isp_drv_ctx *drv_ctx, ot_vi_pipe stat_pipe)
{
    unsigned long flags = 0;

    osal_spin_lock_irqsave(isp_drv_get_lock(stat_pipe), &flags);
    if (drv_ctx->is_chg_fps_done == TD_FALSE) {
        drv_ctx->chg_fps_cnt++;
    }
    osal_spin_unlock_irqrestore(isp_drv_get_lock(stat_pipe), &flags);

    if (drv_ctx->chg_fps_cnt > DIST_FPS_WAIT_CNT) {
        isp_dist_reset_all_pipe_cfg(stat_pipe);
    }
}
#endif

static td_void isp_dist_trans_ctx(ot_vi_pipe phy_pipe, td_u32 vicap_int_status, isp_drv_ctx **drv_ctx)
{
#ifdef CONFIG_OT_VI_DISTRIBUTE_GRP
    td_u32 raw_int = 0;
    ot_vi_pipe stat_pipe;
    isp_drv_ctx *phy_drv_ctx = TD_NULL;
    vi_distribute_attr dist_attr = {0};
    ot_vi_pipe working_pipe;

    phy_drv_ctx = isp_drv_get_ctx(phy_pipe);
    if (phy_drv_ctx->dist_attr.distribute_en == TD_FALSE) {
        return;
    }

    if ((g_mask_is_open[phy_pipe] == TD_TRUE) && isp_is_chn_int(phy_pipe, vicap_int_status)) {
        raw_int = io_rw_ch_address(phy_pipe, VI_WCH_INT);
    }

    if ((raw_int & ISP_INT_FE_FSTART) != 0) {
        if (!ckfn_vi_get_distribute_working_id()) {
            isp_err_trace("get vi distribute attr error\n");
            return;
        }

        if (!ckfn_vi_get_distribute_attr()) {
            isp_err_trace("get vi distribute attr error\n");
            return;
        }

        working_pipe = call_vi_get_distribute_working_id(phy_pipe, TD_FALSE);
        if (call_vi_get_distribute_attr(phy_pipe, &dist_attr) != TD_SUCCESS) {
            isp_err_trace("pipe %d get vi distribute attr error\n", phy_pipe);
            return;
        }

        if (is_virt_pipe(dist_attr.pipe_id[0]) || !is_virt_pipe(dist_attr.pipe_id[1]) ||
            (working_pipe != dist_attr.pipe_id[0] && working_pipe != dist_attr.pipe_id[1])) {
            isp_err_trace("pipe[%d] dist pipe_id[%d, %d] work pipe %d error\n", phy_pipe,
                dist_attr.pipe_id[0], dist_attr.pipe_id[1], working_pipe);
            return;
        }

        stat_pipe = (working_pipe == dist_attr.pipe_id[0] ? dist_attr.pipe_id[0] : dist_attr.pipe_id[1]);

        phy_drv_ctx->dist_proc_pipe = stat_pipe ;
        *drv_ctx = isp_drv_get_ctx(phy_drv_ctx->dist_proc_pipe);
        (*drv_ctx)->dist_proc_pipe = stat_pipe;

        isp_dist_update_fps_status((*drv_ctx), stat_pipe);
    }

    *drv_ctx = isp_drv_get_ctx(phy_drv_ctx->dist_proc_pipe);
#endif
}

td_void isp_dist_clear_int(ot_vi_pipe phy_pipe, td_u32 vicap_int_status)
{
#ifdef CONFIG_OT_VI_DISTRIBUTE_GRP
    td_u32 raw_int = 0;
    td_u32 port_int = 0;
    isp_drv_ctx *phy_drv_ctx = TD_NULL;
    phy_drv_ctx = isp_drv_get_ctx(phy_pipe);
    if (phy_drv_ctx->dist_attr.distribute_en == TD_FALSE) {
        return;
    }

    if (isp_is_fe_int_valid(phy_pipe, vicap_int_status)) {
        raw_int = io_rw_fe_address(phy_pipe, ISP_INT_FE);
    }

    if (raw_int) {
        io_rw_fe_address(phy_pipe, ISP_INT_FE) = raw_int;
    }

    if (isp_is_port_int_valid(phy_drv_ctx->wdr_attr.vi_dev, vicap_int_status)) {
        port_int = io_rw_pt_address(vi_pt_base(phy_drv_ctx->wdr_attr.vi_dev) + VI_PT_INT);
    }

    if (port_int) {
        io_rw_pt_address(vi_pt_base(phy_drv_ctx->wdr_attr.vi_dev) + VI_PT_INT) = port_int;
    }

    return;
#endif
}

int isp_isr(int irq, void *id, td_u32 vicap_int_status)
{
    td_u32 i;
    td_s32 vi_dev, ret;
    ot_vi_pipe vi_pipe;
    isp_drv_ctx *drv_ctx = TD_NULL;
    ot_unused(id);

    /* Isp FE Interrupt Process Begin */
    for (i = 0; i < OT_ISP_MAX_PHY_PIPE_NUM; i++) {
        vi_pipe = i;

        drv_ctx = isp_drv_get_ctx(vi_pipe);
        vi_dev = drv_ctx->wdr_attr.vi_dev;

        isp_dist_trans_ctx(i, vicap_int_status, &drv_ctx);

        if (drv_ctx->mem_init == TD_FALSE) {
            isp_dist_clear_int(i, vicap_int_status);
            continue;
        }

        ret = isp_drv_int_status_process(vi_pipe, vi_dev, drv_ctx, vicap_int_status);
        if (ret != TD_SUCCESS) {
            continue;
        }
    }

    if (isp_bottom_half_is_enable() == TD_FALSE) {
        isp_drv_int_bottom_half(irq);
    } else {
#ifdef CONFIG_OT_ISP_LITEOS_BOTTOM_HALF_SUPPORT
#ifdef __LITEOS__
        isp_drv_work_queue_run(vi_pipe);
#endif
#endif
    }

    return OSAL_IRQ_WAKE_THREAD;
}

int isp_int_bottom_half(int irq, void *id)
{
    ot_unused(id);
    if (isp_bottom_half_is_enable() == TD_TRUE) {
        return isp_drv_int_bottom_half(irq);
    } else {
        return OSAL_IRQ_HANDLED;
    }
}

td_s32 isp_drv_get_use_node_yuv_mode(ot_vi_pipe vi_pipe, td_u32 isp_int_status, td_u32 wch_int_status)
{
    td_s32 ret;
    isp_drv_ctx *drv_ctx = TD_NULL;
    isp_sync_cfg *sync_cfg = TD_NULL;
    unsigned long flags = 0;

    isp_check_pipe_return(vi_pipe);

    drv_ctx = isp_drv_get_ctx(vi_pipe);
    sync_cfg = &drv_ctx->sync_cfg;

    if ((drv_ctx->yuv_mode == TD_TRUE) && wch_int_status && (drv_ctx->isp_run_flag == TD_TRUE)) {
        if (drv_ctx->use_node) {
            osal_spin_lock_irqsave(&g_isp_lock[vi_pipe], &flags);

            ret = isp_drv_get_sync_ctrl_info(vi_pipe, sync_cfg);
            isp_check_ret_continue(vi_pipe, ret, "isp_drv_get_sync_ctrl_info");
            ret = isp_drv_calc_sync_cfg(vi_pipe, sync_cfg);
            isp_check_ret_continue(vi_pipe, ret, "isp_drv_calc_sync_cfg");
            ret = isp_drv_reg_config_isp(vi_pipe, drv_ctx);
            isp_check_ret_continue(vi_pipe, ret, "isp_drv_reg_config_isp");

            drv_ctx->status = isp_int_status;

            osal_spin_unlock_irqrestore(&g_isp_lock[vi_pipe], &flags);

            isp_drv_stat_buf_fe_insert(vi_pipe, drv_ctx);
            isp_drv_stat_buf_summary_insert(vi_pipe, drv_ctx);

            osal_spin_lock_irqsave(&g_isp_lock[vi_pipe], &flags);
            isp_drv_wake_up_thread(vi_pipe);
            osal_spin_unlock_irqrestore(&g_isp_lock[vi_pipe], &flags);
            isp_sync_task_process(vi_pipe);

            return TD_FAILURE;
        }

        osal_spin_lock_irqsave(&g_isp_lock[vi_pipe], &flags);
        if (isp_drv_get_free_be_buf(vi_pipe) != TD_SUCCESS) {
            osal_spin_unlock_irqrestore(&g_isp_lock[vi_pipe], &flags);
            isp_err_trace("Pipe[%d] get FreeBeBuf is fail!, busy:%d, free:%d, use:%d\r\n",
                vi_pipe, drv_ctx->be_buf_queue.busy_num, drv_ctx->be_buf_queue.free_num,
                drv_ctx->be_buf_info.use_cnt);
            return TD_FAILURE;
        }

        osal_spin_unlock_irqrestore(&g_isp_lock[vi_pipe], &flags);
    }

    return TD_SUCCESS;
}

td_s32 isp_drv_get_use_node_raw_mode(ot_vi_pipe vi_pipe, td_u32 isp_int_status)
{
    td_s32 ret;
    isp_drv_ctx *drv_ctx = TD_NULL;
    isp_sync_cfg *sync_cfg = TD_NULL;
    unsigned long flags = 0;

    isp_check_pipe_return(vi_pipe);

    drv_ctx = isp_drv_get_ctx(vi_pipe);
    sync_cfg = &drv_ctx->sync_cfg;

    isp_check_bebuf_init_return(vi_pipe, drv_ctx->be_buf_info.init);

    if (isp_int_status & FE_INT_FSTART) {
        if (drv_ctx->use_node != TD_NULL) {
            drv_ctx->drv_dbg_info.cross_frame_cnt++;
            isp_warn_trace("ISP[%d] cross frame:%d\n", vi_pipe, drv_ctx->drv_dbg_info.cross_frame_cnt);
            /* Need to configure the sensor registers and get statistics for AE/AWB. */
            osal_spin_lock_irqsave(&g_isp_lock[vi_pipe], &flags);
            ret = isp_drv_get_sync_ctrl_info(vi_pipe, sync_cfg);
            isp_check_ret_continue(vi_pipe, ret, "isp_drv_get_sync_ctrl_info");
            ret = isp_drv_calc_sync_cfg(vi_pipe, sync_cfg);
            isp_check_ret_continue(vi_pipe, ret, "isp_drv_calc_sync_cfg");
            ret = isp_drv_reg_config_isp(vi_pipe, drv_ctx);
            isp_check_ret_continue(vi_pipe, ret, "isp_drv_reg_config_isp");
            ret = isp_drv_reg_config_sensor(vi_pipe, drv_ctx);
            isp_check_ret_continue(vi_pipe, ret, "isp_drv_reg_config_sensor");

            drv_ctx->status = isp_int_status;

            osal_spin_unlock_irqrestore(&g_isp_lock[vi_pipe], &flags);

            isp_drv_stat_buf_fe_insert(vi_pipe, drv_ctx);
            isp_drv_stat_buf_summary_insert(vi_pipe, drv_ctx);

            osal_spin_lock_irqsave(&g_isp_lock[vi_pipe], &flags);
            if (g_isp_run_wakeup_sel[vi_pipe] == OT_ISP_RUN_WAKEUP_FE_START) {
                isp_drv_wake_up_thread(vi_pipe);
            }

            osal_spin_unlock_irqrestore(&g_isp_lock[vi_pipe], &flags);
            isp_sync_task_process(vi_pipe);

            return TD_FAILURE;
        }

        osal_spin_lock_irqsave(&g_isp_lock[vi_pipe], &flags);

        if (isp_drv_get_free_be_buf(vi_pipe) != TD_SUCCESS) {
            /* Need to configure the sensor registers. */
            ret = isp_drv_reg_config_sensor(vi_pipe, drv_ctx);
            isp_check_ret_continue(vi_pipe, ret, "isp_drv_reg_config_sensor");
            osal_spin_unlock_irqrestore(&g_isp_lock[vi_pipe], &flags);
            isp_err_trace("Pipe[%d] get FreeBeBuf is fail!, busy_num:%d, free_num:%d, use_cnt:%d\r\n",
                vi_pipe, drv_ctx->be_buf_queue.busy_num, drv_ctx->be_buf_queue.free_num, drv_ctx->be_buf_info.use_cnt);

            return TD_FAILURE;
        }

        osal_spin_unlock_irqrestore(&g_isp_lock[vi_pipe], &flags);
    }

    return TD_SUCCESS;
}

td_s32 isp_drv_get_use_node(ot_vi_pipe vi_pipe, td_u32 isp_int_status, td_u32 wch_int_status)
{
    td_s32 ret;

    isp_check_pipe_return(vi_pipe);

    ret = isp_drv_get_use_node_raw_mode(vi_pipe, isp_int_status);
    if (ret != TD_SUCCESS) {
        return ret;
    }

    ret = isp_drv_get_use_node_yuv_mode(vi_pipe, isp_int_status, wch_int_status);
    if (ret != TD_SUCCESS) {
        return ret;
    }

    return TD_SUCCESS;
}

#ifdef CONFIG_OT_VI_STITCH_GRP
td_s32 isp_drv_stitch_get_use_node(ot_vi_pipe vi_pipe, td_u32 isp_int_status)
{
    td_s32 ret;
    td_u32 busy_num, free_num, use_cnt;
    ot_vi_pipe main_pipe;
    isp_drv_ctx *drv_ctx = TD_NULL;
    isp_sync_cfg *sync_cfg = TD_NULL;
    unsigned long flags = 0;

    isp_check_pipe_return(vi_pipe);

    drv_ctx = isp_drv_get_ctx(vi_pipe);
    sync_cfg = &drv_ctx->sync_cfg;
    main_pipe = drv_ctx->stitch_attr.stitch_bind_id[0];

    isp_check_bebuf_init_return(vi_pipe, drv_ctx->be_buf_info.init);

    if (isp_int_status & FE_INT_FSTART) {
        osal_spin_lock_irqsave(&g_isp_sync_lock[main_pipe], &flags);

        if (drv_ctx->use_node != TD_NULL) {
            /* Need to configure the sensor registers and get statistics for AE/AWB. */
            ret = isp_drv_get_sync_ctrl_info(vi_pipe, sync_cfg);
            isp_check_ret_continue(vi_pipe, ret, "isp_drv_get_sync_ctrl_info");
            ret = isp_drv_calc_sync_cfg(vi_pipe, sync_cfg);
            isp_check_ret_continue(vi_pipe, ret, "isp_drv_calc_sync_cfg");
            ret = isp_drv_reg_config_isp(vi_pipe, drv_ctx);
            isp_check_ret_continue(vi_pipe, ret, "isp_drv_reg_config_isp");
            ret = isp_drv_reg_config_sensor(vi_pipe, drv_ctx);
            isp_check_ret_continue(vi_pipe, ret, "isp_drv_reg_config_sensor");

            drv_ctx->status = isp_int_status;

            isp_drv_stat_buf_fe_insert(vi_pipe, drv_ctx);
            isp_drv_stat_buf_summary_insert(vi_pipe, drv_ctx);

            if (g_isp_run_wakeup_sel[vi_pipe] == OT_ISP_RUN_WAKEUP_FE_START) {
                isp_drv_wake_up_thread(vi_pipe);
            }

            osal_spin_unlock_irqrestore(&g_isp_sync_lock[main_pipe], &flags);
            isp_sync_task_process(vi_pipe);

            return TD_FAILURE;
        }

        if (isp_drv_get_free_be_buf(vi_pipe) != TD_SUCCESS) {
            /* Need to configure the sensor registers. */
            ret = isp_drv_reg_config_sensor(vi_pipe, drv_ctx);
            isp_check_ret_continue(vi_pipe, ret, "isp_drv_reg_config_sensor");
            busy_num = drv_ctx->be_buf_queue.busy_num;
            free_num = drv_ctx->be_buf_queue.free_num;
            use_cnt = drv_ctx->be_buf_info.use_cnt;
            osal_spin_unlock_irqrestore(&g_isp_sync_lock[main_pipe], &flags);
            isp_err_trace("Pipe[%d] get FreeBeBuf is fail!, busy:%d, free:%d, use:%d\r\n",
                vi_pipe, busy_num, free_num, use_cnt);
            return TD_FAILURE;
        }

        osal_spin_unlock_irqrestore(&g_isp_sync_lock[main_pipe], &flags);
    }

    return TD_SUCCESS;
}
#endif

td_s32 isp_drv_get_be_buf_use_node(ot_vi_pipe vi_pipe, td_u32 isp_int_status, td_u32 int_num)
{
    isp_drv_ctx *drv_ctx = TD_NULL;
    td_u32 wch_int_f_start;
    isp_check_pipe_return(vi_pipe);

    drv_ctx = isp_drv_get_ctx(vi_pipe);
    if (is_online_mode(drv_ctx->work_mode.running_mode) || is_sidebyside_mode(drv_ctx->work_mode.running_mode)) {
        return TD_SUCCESS;
    }

    wch_int_f_start = drv_ctx->int_sch[int_num].wch_int_status;
    if (drv_ctx->stitch_attr.stitch_enable != TD_TRUE) {
        return isp_drv_get_use_node(vi_pipe, isp_int_status, wch_int_f_start);
    } else {
#ifdef CONFIG_OT_VI_STITCH_GRP
        return isp_drv_stitch_get_use_node(vi_pipe, isp_int_status);
#else
        return TD_FAILURE;
#endif
    }
}

td_void isp_drv_proc_get_port_int_time1(isp_drv_ctx *drv_ctx, td_u32 port_int_f_start, td_u64 *pt_time1)
{
    if (port_int_f_start) { /* port int proc */
        drv_ctx->drv_dbg_info.pt_int_cnt++;
        if (ckfn_sys_entry() && ckfn_sys_get_time_stamp()) {
            *pt_time1 = call_sys_get_time_stamp();
        }

        if (drv_ctx->drv_dbg_info.pt_last_int_time) {
            drv_ctx->drv_dbg_info.pt_int_gap_time = *pt_time1 - drv_ctx->drv_dbg_info.pt_last_int_time;

            if (drv_ctx->drv_dbg_info.pt_int_gap_time > drv_ctx->drv_dbg_info.pt_int_gap_time_max) {
                drv_ctx->drv_dbg_info.pt_int_gap_time_max = drv_ctx->drv_dbg_info.pt_int_gap_time;
            }
        }

        drv_ctx->drv_dbg_info.pt_last_int_time = *pt_time1;
    }
}

td_void isp_drv_proc_get_isp_int_time1(isp_drv_ctx *drv_ctx, td_u32 isp_int_status, td_u64 *isp_time1)
{
    if (isp_int_status & FE_INT_FSTART) { /* isp int proc */
        drv_ctx->drv_dbg_info.isp_int_cnt++;
        if (ckfn_sys_entry() && ckfn_sys_get_time_stamp()) {
            *isp_time1 = call_sys_get_time_stamp();
        }

        if (drv_ctx->drv_dbg_info.isp_last_int_time) {
            drv_ctx->drv_dbg_info.isp_int_gap_time = *isp_time1 - drv_ctx->drv_dbg_info.isp_last_int_time;

            if (drv_ctx->drv_dbg_info.isp_int_gap_time > drv_ctx->drv_dbg_info.isp_int_gap_time_max) {
                drv_ctx->drv_dbg_info.isp_int_gap_time_max = drv_ctx->drv_dbg_info.isp_int_gap_time;
            }
        }

        drv_ctx->drv_dbg_info.isp_last_int_time = *isp_time1;
    }
}

td_void isp_drv_proc_calc_port_int(isp_drv_ctx *drv_ctx, td_u32 port_int_f_start, td_u64 pt_time1)
{
    td_u64 pt_time2 = 0;

    if (port_int_f_start) { /* port int proc */
        if (ckfn_sys_entry() && ckfn_sys_get_time_stamp()) {
            pt_time2 = call_sys_get_time_stamp();
        }
        drv_ctx->drv_dbg_info.pt_int_time = pt_time2 - pt_time1;

        if (drv_ctx->drv_dbg_info.pt_int_time > drv_ctx->drv_dbg_info.pt_int_time_max) {
            drv_ctx->drv_dbg_info.pt_int_time_max = drv_ctx->drv_dbg_info.pt_int_time;
        }

        if ((pt_time2 - drv_ctx->drv_dbg_info.pt_last_rate_time) >= 1000000UL) {
            drv_ctx->drv_dbg_info.pt_last_rate_time = pt_time2;
            drv_ctx->drv_dbg_info.pt_rate = drv_ctx->drv_dbg_info.pt_rate_int_cnt;
            drv_ctx->drv_dbg_info.pt_rate_int_cnt = 0;
        }

        drv_ctx->drv_dbg_info.pt_rate_int_cnt++;
    }
}

td_void isp_drv_proc_calc_isp_int(isp_drv_ctx *drv_ctx, td_u32 isp_int_status, td_u64 isp_time1)
{
    td_u64 isp_time2 = 0;

    if (isp_int_status & FE_INT_FSTART) { /* isp int proc */
        if (ckfn_sys_entry() && ckfn_sys_get_time_stamp()) {
            isp_time2 = call_sys_get_time_stamp();
        }
        drv_ctx->drv_dbg_info.isp_int_time = isp_time2 - isp_time1;

        if (drv_ctx->drv_dbg_info.isp_int_time > drv_ctx->drv_dbg_info.isp_int_time_max) {
            drv_ctx->drv_dbg_info.isp_int_time_max = drv_ctx->drv_dbg_info.isp_int_time;
        }

        if ((isp_time2 - drv_ctx->drv_dbg_info.isp_last_rate_time) >= 10000000UL) {
            drv_ctx->drv_dbg_info.isp_last_rate_time = isp_time2;
            drv_ctx->drv_dbg_info.isp_rate = drv_ctx->drv_dbg_info.isp_rate_int_cnt / 10; /* 10s */
            drv_ctx->drv_dbg_info.isp_rate_int_cnt = 0;
        }

        drv_ctx->drv_dbg_info.isp_rate_int_cnt++;
    }
}

td_void isp_drv_proc_calc_avg_isp_int(isp_drv_ctx *drv_ctx, td_u32 isp_int_status)
{
    isp_drv_dbg_info *dbg_info = &drv_ctx->drv_dbg_info;
    td_u32 isp_int_cnt = dbg_info->isp_int_cnt % CALC_ISP_INT_AVG_CNT;

    if (isp_int_status & FE_INT_FSTART) { /* isp int proc */
        if (isp_int_cnt == 0) {
            dbg_info->isp_int_total_time = 0;
        } else {
            dbg_info->isp_int_total_time += dbg_info->isp_int_time;
            dbg_info->isp_int_avg_time = dbg_info->isp_int_total_time / div_0_to_1(isp_int_cnt);
        }
    }
}

td_void isp_drv_proc_calc_sensor_cfg_time(isp_drv_ctx *drv_ctx)
{
    isp_drv_dbg_info *dbg_info = &drv_ctx->drv_dbg_info;
    if (dbg_info->sensor_cfg_time > dbg_info->sensor_cfg_time_max) {
        dbg_info->sensor_cfg_time_max = dbg_info->sensor_cfg_time;
    }
    dbg_info->sensor_cfg_time_total += dbg_info->sensor_cfg_time;
    dbg_info->sensor_cfg_cnt++;
    if (dbg_info->sensor_cfg_cnt > CALC_SNS_AVE_CNT) {
        dbg_info->sensor_cfg_cnt = 0;
        dbg_info->sensor_cfg_time_total = 0;
    } else {
        dbg_info->sensor_cfg_time_avg = dbg_info->sensor_cfg_time_total / div_0_to_1(dbg_info->sensor_cfg_cnt);
    }
}

static td_void isp_drv_config_sensor(ot_vi_pipe vi_pipe, isp_drv_ctx *drv_ctx)
{
    td_s32 ret;
    td_u64 sensor_cfg_time1 = 0;
    td_u64 sensor_cfg_time2 = 0;

    if (ckfn_sys_entry() && ckfn_sys_get_time_stamp()) {
        sensor_cfg_time1 = call_sys_get_time_stamp();
    }
    ret = isp_drv_reg_config_sensor(vi_pipe, drv_ctx);
    isp_check_ret_continue(vi_pipe, ret, "isp_drv_reg_config_sensor");
    if (ckfn_sys_entry() && ckfn_sys_get_time_stamp()) {
        sensor_cfg_time2 = call_sys_get_time_stamp();
    }
    drv_ctx->drv_dbg_info.sensor_cfg_time = sensor_cfg_time2 - sensor_cfg_time1;
}

td_s32 isp_drv_sync_process(ot_vi_pipe vi_pipe, isp_drv_ctx *drv_ctx)
{
    td_s32 ret;
    isp_sync_cfg *sync_cfg = &drv_ctx->sync_cfg;

    ret = isp_drv_get_sync_ctrl_info(vi_pipe, sync_cfg);
    isp_check_ret_continue(vi_pipe, ret, "isp_drv_get_sync_ctrl_info");

    ret = isp_drv_calc_sync_cfg(vi_pipe, sync_cfg);
    isp_check_ret_continue(vi_pipe, ret, "isp_drv_calc_sync_cfg");

    ret = isp_drv_reg_config_isp(vi_pipe, drv_ctx);
    isp_check_ret_continue(vi_pipe, ret, "isp_drv_reg_config_isp");

    isp_drv_config_sensor(vi_pipe, drv_ctx);

    return TD_SUCCESS;
}

td_void isp_get_slave_pipe_int_status(ot_vi_pipe vi_pipe, td_bool *slave_pipe_int_active)
{
    td_u8 chn_num_max;
    td_u8 k;
    td_u8 int_status_index;
    isp_drv_ctx *drv_ctx = TD_NULL;
    td_u32 isp_int_status = 0;
    td_u32 isp_raw_int = 0;
    ot_vi_pipe vi_pipe_bind;
    if (g_long_frm_int_en[vi_pipe] == TD_FALSE) {
        *slave_pipe_int_active = TD_FALSE;
        return;
    }

    drv_ctx = isp_drv_get_ctx(vi_pipe);
    if ((drv_ctx->wdr_attr.is_mast_pipe == TD_FALSE) || (is_fs_wdr_mode(drv_ctx->wdr_attr.wdr_mode) == TD_FALSE)) {
        (td_void)memset_s(&(drv_ctx->slave_pipe_int_status), sizeof(isp_slave_pipe_int_status) * (ISP_WDR_CHN_MAX - 1),
            0, sizeof(isp_slave_pipe_int_status) * (ISP_WDR_CHN_MAX - 1));
        *slave_pipe_int_active = TD_FALSE;
        return;
    }

    chn_num_max = clip3(drv_ctx->wdr_attr.pipe_num, 1, ISP_WDR_CHN_MAX);
    int_status_index = 0;
    for (k = 0; k < chn_num_max; k++) {
        vi_pipe_bind = drv_ctx->wdr_attr.pipe_id[k];
        if (vi_pipe == vi_pipe_bind) {
            continue;
        } else {
            if (vi_pipe_bind >= OT_ISP_MAX_FE_PIPE_NUM) {
                continue;
            }
            /* read fe interrupt status */
            isp_raw_int = io_rw_fe_address(vi_pipe_bind, ISP_INT_FE);
            isp_int_status = isp_raw_int & io_rw_fe_address(vi_pipe_bind, ISP_INT_FE_MASK);
            if (isp_raw_int) {  /* clear interrupts */
                io_rw_fe_address(vi_pipe_bind, ISP_INT_FE) = isp_raw_int;
            }

            drv_ctx->slave_pipe_int_status[int_status_index].vi_pipe_id = vi_pipe_bind;
            drv_ctx->slave_pipe_int_status[int_status_index].isp_int_status = isp_int_status;
            int_status_index++;
            if (isp_int_status != 0) {
                *slave_pipe_int_active = TD_TRUE;
            }
        }
    }

    return;
}

static isp_cfg_sensor_time isp_drv_get_sns_cfg_int_pos_wdr_2to1(const isp_drv_ctx *drv_ctx, ot_vi_pipe vi_pipe_bind)
{
    ot_unused(drv_ctx);
    ot_unused(vi_pipe_bind);

    return ISP_CFG_SNS_SHORT_FRAME;
}

static isp_cfg_sensor_time isp_drv_get_sns_cfg_int_pos_wdr_3to1(const isp_drv_ctx *drv_ctx, ot_vi_pipe vi_pipe_bind)
{
    ot_vi_pipe wdr_short_frame_pipe_id;
    ot_vi_pipe status0_pipe_id = drv_ctx->slave_pipe_int_status[0].vi_pipe_id;
    ot_vi_pipe status1_pipe_id = drv_ctx->slave_pipe_int_status[1].vi_pipe_id; /* 1 */

    if (status0_pipe_id < status1_pipe_id) {
        wdr_short_frame_pipe_id = status0_pipe_id;
    } else {
        wdr_short_frame_pipe_id = status1_pipe_id;
    }
    if (vi_pipe_bind == wdr_short_frame_pipe_id) {
        return ISP_CFG_SNS_SHORT_FRAME;
    } else {
        return ISP_CFG_SNS_MIDDLE_FRAME;
    }
}

static isp_cfg_sensor_time isp_drv_get_sns_cfg_int_pos_wdr_4to1(const isp_drv_ctx *drv_ctx, ot_vi_pipe vi_pipe_bind)
{
    ot_vi_pipe wdr_short_frame_pipe_id;
    ot_vi_pipe wdr_long_frame_pipe_id;
    ot_vi_pipe status0_pipe_id = drv_ctx->slave_pipe_int_status[0].vi_pipe_id;
    ot_vi_pipe status1_pipe_id = drv_ctx->slave_pipe_int_status[1].vi_pipe_id; /* 1 */
    ot_vi_pipe status2_pipe_id = drv_ctx->slave_pipe_int_status[2].vi_pipe_id; /* 2 */

    if ((status0_pipe_id < status1_pipe_id) && (status0_pipe_id < status2_pipe_id)) {
        wdr_short_frame_pipe_id = status0_pipe_id;
    } else if ((status1_pipe_id < status0_pipe_id) && (status1_pipe_id < status2_pipe_id)) {
        wdr_short_frame_pipe_id = status1_pipe_id;
    } else {
        wdr_short_frame_pipe_id = status2_pipe_id;
    }
    if ((status0_pipe_id > status1_pipe_id) && (status0_pipe_id > status2_pipe_id)) {
        wdr_long_frame_pipe_id = status0_pipe_id;
    } else if ((status1_pipe_id > status0_pipe_id) && (status1_pipe_id > status2_pipe_id)) {
        wdr_long_frame_pipe_id = status1_pipe_id;
    } else {
        wdr_long_frame_pipe_id = status2_pipe_id;
    }
    if (vi_pipe_bind == wdr_short_frame_pipe_id) {
        return ISP_CFG_SNS_SHORT_FRAME;
    } else if (vi_pipe_bind == wdr_long_frame_pipe_id) {
        return ISP_CFG_SNS_LONG_FRAME;
    } else {
        return ISP_CFG_SNS_MIDDLE_FRAME;
    }
}

isp_cfg_sensor_time isp_drv_get_sns_cfg_int_pos(const isp_drv_ctx *drv_ctx, ot_vi_pipe vi_pipe_bind)
{
    switch (drv_ctx->wdr_attr.wdr_mode) {
        case OT_WDR_MODE_2To1_LINE:
            return isp_drv_get_sns_cfg_int_pos_wdr_2to1(drv_ctx, vi_pipe_bind);

        case OT_WDR_MODE_3To1_LINE:
            return isp_drv_get_sns_cfg_int_pos_wdr_3to1(drv_ctx, vi_pipe_bind);

        case OT_WDR_MODE_4To1_LINE:
            return isp_drv_get_sns_cfg_int_pos_wdr_4to1(drv_ctx, vi_pipe_bind);

        default:
            return ISP_CFG_SNS_SHORT_FRAME;
    }
}
td_void isp_long_frm_cfg_sensor(ot_vi_pipe vi_pipe)
{
    isp_drv_ctx *drv_ctx = TD_NULL;
    td_u32 bind_pipe_int_status;
    td_u32 pipe_int_pos_save;
    td_u32 pipe_int_status_save;
    td_u32 k;
    ot_vi_pipe vi_pipe_bind;
    isp_cfg_sensor_time cfg_sns_int_pos;
    if (g_long_frm_int_en[vi_pipe] == TD_FALSE) {
        return;
    }
    drv_ctx = isp_drv_get_ctx(vi_pipe);
    if (is_fs_wdr_mode(drv_ctx->wdr_attr.wdr_mode) == TD_FALSE) {
        return;
    }

    pipe_int_status_save = drv_ctx->status;
    pipe_int_pos_save = drv_ctx->int_pos;

    for (k = 0; k < ISP_WDR_CHN_MAX - 1; k++) {
        bind_pipe_int_status = drv_ctx->slave_pipe_int_status[k].isp_int_status;
        vi_pipe_bind = drv_ctx->slave_pipe_int_status[k].vi_pipe_id;
        cfg_sns_int_pos = isp_drv_get_sns_cfg_int_pos(drv_ctx, vi_pipe_bind);
        if (bind_pipe_int_status & FE_INT_FSTART) {
            drv_ctx->int_pos = cfg_sns_int_pos + 0; /* 0:frame start */
            drv_ctx->status = bind_pipe_int_status;
            isp_drv_reg_config_sensor(vi_pipe, drv_ctx);
        }
        if (bind_pipe_int_status & FE_INT_FEND) {
            drv_ctx->int_pos = cfg_sns_int_pos + 1; /* 1:frame end */
            drv_ctx->status = bind_pipe_int_status;
            isp_drv_reg_config_sensor(vi_pipe, drv_ctx);
        }
    }
    drv_ctx->status = pipe_int_status_save;
    drv_ctx->int_pos = pipe_int_pos_save;
    return;
}

static td_s32 isp_drv_dynamic_blc_process(ot_vi_pipe vi_pipe, isp_drv_ctx *drv_ctx, td_u32 int_num)
{
#ifdef CONFIG_OT_ISP_DYNAMIC_BLC_SUPPORT
    td_u32 isp_int_status = 0;

    isp_check_no_fe_pipe_return(vi_pipe);

    if (g_ob_stats_update_pos[vi_pipe] == OT_ISP_UPDATE_OB_STATS_FE_FRAME_END) {
        isp_int_status = drv_ctx->int_sch[int_num].isp_int_status & ISP_INT_FE_DYNABLC_END;
    } else if (g_ob_stats_update_pos[vi_pipe] == OT_ISP_UPDATE_OB_STATS_FE_FRAME_START) {
        isp_int_status = drv_ctx->int_sch[int_num].isp_int_status & ISP_INT_FE_FSTART;
    }

    if (isp_int_status) {
        isp_drv_reg_config_dynamic_blc(vi_pipe, drv_ctx);
    }
#else
    ot_unused(vi_pipe);
    ot_unused(drv_ctx);
    ot_unused(int_num);
#endif

    return TD_SUCCESS;
}

static td_u32 isp_be_cfg_int_time_get(ot_vi_pipe vi_pipe, const isp_drv_ctx *drv_ctx,
    td_u32 isp_calc_sync_timing, td_u32 int_num)
{
    td_u32 isp_int_status;

    isp_int_status = drv_ctx->int_sch[int_num].isp_int_status;
    if (is_half_wdr_mode(drv_ctx->sync_cfg.wdr_mode)) {
        return (isp_int_status & ISP_INT_FE_DELAY); /* fe_int_int_delay */
    }

    if (is_line_wdr_mode(drv_ctx->sync_cfg.wdr_mode)) {
        return (isp_int_status & ISP_INT_FE_DELAY); /* fe_int_int_delay */
    }

    if (g_update_pos[vi_pipe] == 0) { /* frame start */
        return (isp_int_status & ISP_INT_FE_DELAY); /* fe_int_int_delay */
    } else {
        return isp_calc_sync_timing;
    }
}

/* because FE frame start time is early than BE frame start when ONLINE mode, */
/* sometime sync register configure at FE frame start will be valid at BE frame start. this means configure register */
/* and register active at the same frame, this make an error occurred. */
/* for this case, we use frame delay interrupt at ONLINE mode, */
/* when frame delay interrupt triggered, configure register */

static td_void isp_drv_reg_config_isp_be_online(ot_vi_pipe vi_pipe, isp_drv_ctx *drv_ctx,
    td_u32 isp_calc_sync_timing, td_u32 int_num)
{
    td_u32 be_cfg_timing;
    if (is_offline_mode(drv_ctx->work_mode.running_mode) ||
        is_striping_mode(drv_ctx->work_mode.running_mode)) {
        return;
    }

    be_cfg_timing = isp_be_cfg_int_time_get(vi_pipe, drv_ctx, isp_calc_sync_timing, int_num);
    if (be_cfg_timing) {
        isp_drv_set_isp_be_sync_param_online(vi_pipe, drv_ctx);
    }
    return;
}

#ifdef CONFIG_OT_SNAP_SUPPORT
td_s32 isp_irq_snap_process(ot_vi_pipe vi_pipe, isp_drv_ctx *drv_ctx, td_u32 int_num)
{
    td_u32 isp_int_status;
    td_u8 cfg_dly_max;
    td_u32 isp_snap_int_mask;
    td_bool pipe_low_delay_en;

    isp_check_pointer_return(drv_ctx);
    isp_int_status = drv_ctx->int_sch[int_num].isp_int_status;
    isp_snap_int_mask = FE_INT_FEND;

    isp_drv_get_pipe_low_delay_en(vi_pipe, &pipe_low_delay_en);
    if (pipe_low_delay_en == TD_TRUE) {
        isp_snap_int_mask = VI_WCH_INT_LOW_DELAY;
    }

    if (isp_int_status & isp_snap_int_mask) {
        if (drv_ctx->pro_trig_flag == 1) {
            drv_ctx->pro_trig_flag++;
            drv_ctx->start_snap_num = drv_ctx->frame_cnt;
        }

        if (drv_ctx->pro_trig_flag > 1 && drv_ctx->start_snap_num > 0) {
            cfg_dly_max = MAX2(drv_ctx->sync_cfg.cfg2_vld_dly_max, 1);
            if (ckfn_vi_set_pro_frame_flag() &&
                (drv_ctx->frame_cnt - drv_ctx->start_snap_num > (td_u32)cfg_dly_max) &&
                (drv_ctx->pro_frm_num < drv_ctx->snap_attr.pro_param.pro_frame_num)) {
                call_vi_set_pro_frame_flag(vi_pipe);
                drv_ctx->pro_frm_num++;
            }

            if (drv_ctx->pro_frm_num >= drv_ctx->snap_attr.pro_param.pro_frame_num) {
                drv_ctx->pro_trig_flag = 0;
            }
        }
    }

    return TD_SUCCESS;
}
#endif

td_s32 isp_irq_full_wdr_process(ot_vi_pipe vi_pipe, isp_drv_ctx *drv_ctx, td_u32 int_num)
{
    td_s32 ret;
    td_bool single_mode;
    td_u32 isp_int_status;

    isp_int_status = drv_ctx->int_sch[int_num].isp_int_status;

    drv_ctx->int_pos = 0;
    if (isp_int_status & FE_INT_FSTART) {
        if (is_full_wdr_mode(drv_ctx->sync_cfg.wdr_mode)) {
            ret = isp_drv_sync_process(vi_pipe, drv_ctx);
            isp_check_return(vi_pipe, ret, "isp_drv_sync_process");
        }
    }

    if (isp_int_status & FE_INT_FEND) {
        drv_ctx->int_pos = 1;

#ifdef CONFIG_OT_SYS_KOL_SUPPORT
        isp_drv_get_single_mode(vi_pipe, &single_mode);
        if (single_mode || drv_ctx->vd_standby_end == TD_FALSE) {
            ret = isp_drv_reg_config_sensor_standby(vi_pipe, drv_ctx);
            isp_check_ret_continue(vi_pipe, ret, "isp_drv_reg_config_sensor_standby");
        }
#else
        single_mode = TD_FALSE;
        ot_unused(single_mode);
#endif
        ret = isp_drv_reg_config_sensor(vi_pipe, drv_ctx);
        isp_check_ret_continue(vi_pipe, ret, "isp_drv_reg_config_sensor");

        drv_ctx->vd_end = TD_TRUE;
        drv_ctx->vd_standby_end = TD_TRUE;
        isp_update_info_sync(vi_pipe, &drv_ctx->update_info);
        isp_frame_info_sync(vi_pipe, &drv_ctx->frame_info);
        osal_wait_wakeup(&drv_ctx->isp_wait_vd_end);
        osal_wait_wakeup(&drv_ctx->isp_wait_vd_standby_end);
    }

    if (isp_int_status & FE_INT_FSTART) {
        /* N to 1 fullrate frame WDR mode, get statistics only in the last frame(N-1) */
        if (is_full_wdr_mode(drv_ctx->sync_cfg.wdr_mode)) {
            if (drv_ctx->sync_cfg.vc_num != drv_ctx->sync_cfg.vc_num_max) {
                return TD_SUCCESS;
            }
        }

        drv_ctx->status = isp_int_status;

        isp_drv_stat_buf_fe_insert(vi_pipe, drv_ctx);
        isp_drv_stat_buf_summary_insert(vi_pipe, drv_ctx);

        if (g_isp_run_wakeup_sel[vi_pipe] == OT_ISP_RUN_WAKEUP_FE_START) {
            isp_drv_wake_up_thread(vi_pipe);
        }

        /* Sync  task AF statistics */
        isp_sync_task_process(vi_pipe);
    }
    isp_long_frm_cfg_sensor(vi_pipe);
    return TD_SUCCESS;
}

td_s32 isp_irq_half_wdr_process(ot_vi_pipe vi_pipe, isp_drv_ctx *drv_ctx, td_u32 int_num)
{
    td_s32 ret;
    td_u32 port_int_f_start, isp_int_status;
    td_bool single_mode;

    port_int_f_start = drv_ctx->int_sch[int_num].port_int_status;
    isp_int_status = drv_ctx->int_sch[int_num].isp_int_status;

    drv_ctx->int_pos = 0;
    if (port_int_f_start) {
        if (is_half_wdr_mode(drv_ctx->sync_cfg.wdr_mode)) {
            ret = isp_drv_sync_process(vi_pipe, drv_ctx);
            isp_check_return(vi_pipe, ret, "isp_drv_sync_process");
        }
    }
    isp_drv_dynamic_blc_process(vi_pipe, drv_ctx, int_num);
    isp_drv_reg_config_isp_be_online(vi_pipe, drv_ctx, port_int_f_start, int_num);

    if (isp_int_status & FE_INT_FEND) {
        drv_ctx->int_pos = 1;

#ifdef CONFIG_OT_SYS_KOL_SUPPORT
        isp_drv_get_single_mode(vi_pipe, &single_mode);
        if (single_mode || drv_ctx->vd_standby_end == TD_FALSE) {
            ret = isp_drv_reg_config_sensor_standby(vi_pipe, drv_ctx);
            isp_check_ret_continue(vi_pipe, ret, "isp_drv_reg_config_sensor_standby");
        }
#else
        single_mode = TD_FALSE;
        ot_unused(single_mode);
#endif

        ret = isp_drv_reg_config_sensor(vi_pipe, drv_ctx);
        isp_check_ret_continue(vi_pipe, ret, "isp_drv_reg_config_sensor");

        drv_ctx->vd_end = TD_TRUE;
        drv_ctx->vd_standby_end = TD_TRUE;
        isp_update_info_sync(vi_pipe, &drv_ctx->update_info);
        isp_frame_info_sync(vi_pipe, &drv_ctx->frame_info);
        osal_wait_wakeup(&drv_ctx->isp_wait_vd_end);
        osal_wait_wakeup(&drv_ctx->isp_wait_vd_standby_end);
    }

    if (isp_int_status & FE_INT_FSTART) {
        drv_ctx->status = isp_int_status;

        isp_drv_stat_buf_fe_insert(vi_pipe, drv_ctx);
        isp_drv_stat_buf_summary_insert(vi_pipe, drv_ctx);

        if (g_isp_run_wakeup_sel[vi_pipe] == OT_ISP_RUN_WAKEUP_FE_START) {
            isp_drv_wake_up_thread(vi_pipe);
        }

        /* Sync  task AF statistics */
        isp_sync_task_process(vi_pipe);
    }
    isp_long_frm_cfg_sensor(vi_pipe);
    return TD_SUCCESS;
}

td_s32 isp_irq_line_wdr_process(ot_vi_pipe vi_pipe, isp_drv_ctx *drv_ctx, td_u32 int_num)
{
    td_s32 ret;
    td_u32 isp_int_status;
    td_bool single_mode;

    isp_int_status = drv_ctx->int_sch[int_num].isp_int_status;

    drv_ctx->int_pos = 0;
    if (isp_int_status & FE_INT_FSTART) {
        if (is_line_wdr_mode(drv_ctx->sync_cfg.wdr_mode)) {
            ret = isp_drv_sync_process(vi_pipe, drv_ctx);
            isp_check_return(vi_pipe, ret, "isp_drv_sync_process");
        }
    }
    isp_drv_dynamic_blc_process(vi_pipe, drv_ctx, int_num);
    isp_drv_reg_config_isp_be_online(vi_pipe, drv_ctx, isp_int_status & FE_INT_FSTART, int_num);

    if (isp_int_status & FE_INT_FEND) {
        drv_ctx->int_pos = 1;

#ifdef CONFIG_OT_SYS_KOL_SUPPORT
        isp_drv_get_single_mode(vi_pipe, &single_mode);
        if (single_mode || drv_ctx->vd_standby_end == TD_FALSE) {
            ret = isp_drv_reg_config_sensor_standby(vi_pipe, drv_ctx);
            isp_check_ret_continue(vi_pipe, ret, "isp_drv_reg_config_sensor_standby");
        }
#else
        single_mode = TD_FALSE;
        ot_unused(single_mode);
#endif

        ret = isp_drv_reg_config_sensor(vi_pipe, drv_ctx);
        isp_check_ret_continue(vi_pipe, ret, "isp_drv_reg_config_sensor");

        drv_ctx->vd_end = TD_TRUE;
        drv_ctx->vd_standby_end = TD_TRUE;
        isp_update_info_sync(vi_pipe, &drv_ctx->update_info);
        isp_frame_info_sync(vi_pipe, &drv_ctx->frame_info);
        osal_wait_wakeup(&drv_ctx->isp_wait_vd_end);
        osal_wait_wakeup(&drv_ctx->isp_wait_vd_standby_end);
    }

    if (isp_int_status & FE_INT_FSTART) {
        drv_ctx->status = isp_int_status;

        isp_drv_stat_buf_fe_insert(vi_pipe, drv_ctx);
        isp_drv_stat_buf_summary_insert(vi_pipe, drv_ctx);

        if (g_isp_run_wakeup_sel[vi_pipe] == OT_ISP_RUN_WAKEUP_FE_START) {
            isp_drv_wake_up_thread(vi_pipe);
        }

        /* Sync  task AF statistics */
        isp_sync_task_process(vi_pipe);
    }
    isp_long_frm_cfg_sensor(vi_pipe);
    return TD_SUCCESS;
}

td_s32 isp_irq_linear_process(ot_vi_pipe vi_pipe, isp_drv_ctx *drv_ctx, td_u32 int_num)
{
    td_s32 ret;
    td_u32 isp_int_status, sensor_cfg_int;
    td_bool single_mode;

    isp_int_status = drv_ctx->int_sch[int_num].isp_int_status;

    drv_ctx->int_pos = 0;

    sensor_cfg_int = (g_update_pos[vi_pipe] == 0) ? (isp_int_status & FE_INT_FSTART) : (isp_int_status & FE_INT_FEND);
    if (sensor_cfg_int) {
        ret = isp_drv_sync_process(vi_pipe, drv_ctx);
        isp_check_return(vi_pipe, ret, "isp_drv_sync_process");
    }

    isp_drv_dynamic_blc_process(vi_pipe, drv_ctx, int_num);
    isp_drv_reg_config_isp_be_online(vi_pipe, drv_ctx, sensor_cfg_int, int_num);

    if (isp_int_status & FE_INT_FEND) {
        drv_ctx->int_pos = 1;

#ifdef CONFIG_OT_SYS_KOL_SUPPORT
        isp_drv_get_single_mode(vi_pipe, &single_mode);
        if (single_mode || drv_ctx->vd_standby_end == TD_FALSE) {
            ret = isp_drv_reg_config_sensor_standby(vi_pipe, drv_ctx);
            isp_check_ret_continue(vi_pipe, ret, "isp_drv_reg_config_sensor_standby");
        }
#else
        single_mode = TD_FALSE;
        ot_unused(single_mode);
#endif

        ret = isp_drv_reg_config_sensor(vi_pipe, drv_ctx);
        isp_check_ret_continue(vi_pipe, ret, "isp_drv_reg_config_sensor");

        drv_ctx->vd_end = TD_TRUE;
        drv_ctx->vd_standby_end = TD_TRUE;
        isp_update_info_sync(vi_pipe, &drv_ctx->update_info);
        isp_frame_info_sync(vi_pipe, &drv_ctx->frame_info);
        osal_wait_wakeup(&drv_ctx->isp_wait_vd_end);
        osal_wait_wakeup(&drv_ctx->isp_wait_vd_standby_end);
    }

    if (isp_int_status & FE_INT_FSTART) {
        drv_ctx->status = isp_int_status;

        isp_drv_stat_buf_fe_insert(vi_pipe, drv_ctx);
        isp_drv_stat_buf_summary_insert(vi_pipe, drv_ctx);

        if (g_isp_run_wakeup_sel[vi_pipe] == OT_ISP_RUN_WAKEUP_FE_START) {
            isp_drv_wake_up_thread(vi_pipe);
        }

        /* Sync  task AF statistics */
        isp_sync_task_process(vi_pipe);
    }

    return TD_SUCCESS;
}

static td_void isp_set_be_regup_online_yuv(ot_vi_pipe vi_pipe)
{
    td_s32  ret;
    td_u8   i, block_dev;
    isp_viproc_reg_type *post_viproc[OT_ISP_STRIPING_MAX_NUM] = { TD_NULL };
    isp_drv_ctx *drv_ctx;

    drv_ctx = isp_drv_get_ctx(vi_pipe);
    if (!is_online_mode(drv_ctx->work_mode.running_mode)) {
        return;
    }
    ret = isp_drv_get_post_viproc_reg_virt_addr(vi_pipe, post_viproc);
    if (ret != TD_SUCCESS) {
        return;
    }

    block_dev = drv_ctx->work_mode.block_dev;
    for (i = 0; i < drv_ctx->work_mode.block_num; i++) {
        isp_drv_set_regup_online_yuv(post_viproc[i + block_dev], TD_TRUE);
    }
}

td_s32 isp_irq_yuv_process(ot_vi_pipe vi_pipe, isp_drv_ctx *drv_ctx, td_u32 int_num)
{
    td_u32 wch_int_f_start;

    wch_int_f_start = drv_ctx->int_sch[int_num].wch_int_status;

    if (wch_int_f_start && (drv_ctx->yuv_mode == TD_TRUE) && (drv_ctx->isp_run_flag == TD_TRUE)) { /* WCH int */
        drv_ctx->status = wch_int_f_start;

        isp_drv_stat_buf_fe_insert(vi_pipe, drv_ctx);
        isp_drv_stat_buf_summary_insert(vi_pipe, drv_ctx);

        isp_set_be_regup_online_yuv(vi_pipe);

        isp_drv_wake_up_thread(vi_pipe);
        /* Sync  task AF statistics */
        isp_sync_task_process(vi_pipe);
    }

    return TD_SUCCESS;
}

static td_void isp_irq_proc_by_wdr_mode(ot_vi_pipe vi_pipe, isp_drv_ctx *drv_ctx, td_u32 int_num)
{
    if (is_linear_mode(drv_ctx->sync_cfg.wdr_mode) || is_built_in_wdr_mode(drv_ctx->sync_cfg.wdr_mode)) {
        isp_irq_linear_process(vi_pipe, drv_ctx, int_num);
    }

    if (is_line_wdr_mode(drv_ctx->sync_cfg.wdr_mode)) {
        isp_irq_line_wdr_process(vi_pipe, drv_ctx, int_num);
    }

    if (is_half_wdr_mode(drv_ctx->sync_cfg.wdr_mode)) {
        isp_irq_half_wdr_process(vi_pipe, drv_ctx, int_num);
    }

    if (is_full_wdr_mode(drv_ctx->sync_cfg.wdr_mode)) {
        isp_irq_full_wdr_process(vi_pipe, drv_ctx, int_num);
    }
}

#ifdef CONFIG_OT_ISP_DYNAMIC_BLC_SUPPORT
static td_s32 isp_drv_reg_config_runbe_fe_dynamic_blc(ot_vi_pipe vi_pipe, isp_drv_ctx *drv_ctx, td_u32 int_num)
{
    td_u32 isp_blc_int_status = 0;
    isp_be_blc_dyna_cfg *dyna_blc = TD_NULL;

    isp_check_no_fe_pipe_return(vi_pipe);
    isp_check_pointer_return(drv_ctx);
    dyna_blc = &drv_ctx->dyna_blc_info.be_blc_sync[0];

    if (g_ob_stats_update_pos[vi_pipe] >= OT_ISP_UPDATE_OB_STATS_BUTT) {
        return TD_SUCCESS;
    }
    if (g_ob_stats_update_pos[vi_pipe] == OT_ISP_UPDATE_OB_STATS_FE_FRAME_END) {
        isp_blc_int_status = drv_ctx->int_sch[int_num].isp_int_status & ISP_INT_FE_DYNABLC_END;
    } else if (g_ob_stats_update_pos[vi_pipe] == OT_ISP_UPDATE_OB_STATS_FE_FRAME_START) {
        isp_blc_int_status = drv_ctx->int_sch[int_num].isp_int_status & ISP_INT_FE_FSTART;
    }

    if (isp_blc_int_status) {
        if (drv_ctx->dyna_blc_info.sync_cfg.black_level_mode == OT_ISP_BLACK_LEVEL_MODE_DYNAMIC) {
            // according to ob_stats_read_en to decide to config fe dynamic blc
            if (drv_ctx->dyna_blc_info.ob_stats_read_en != TD_TRUE) {
                dyna_blc->blc_id = 0;
                return TD_SUCCESS;
            }
            isp_drv_reg_config_fe_dynamic_blc_single_pipe(vi_pipe, drv_ctx);
            drv_ctx->dyna_blc_info.ob_stats_read_en = TD_FALSE;
        }
    }

    return TD_SUCCESS;
}
#endif
td_void isp_irq_route_run_plus(ot_vi_pipe vi_pipe, td_u32 int_num)
{
    td_s32 ret;
    isp_drv_ctx *drv_ctx = TD_NULL;
    isp_drv_ctx *drv_ctx_dist = TD_NULL;
    td_u32 port_int_f_start, isp_int_status;
    td_u32 mask;
    td_u64 pt_time1 = 0;
    td_u64 isp_time1 = 0;
    td_bool single_mode = TD_FALSE;
#if defined(CONFIG_OT_VI_DISTRIBUTE_GRP) || defined(CONFIG_OT_VI_DISTRIBUTE_GRP)
    ot_vi_pipe vi_pipe_dist = vi_pipe;
#endif

    isp_check_pipe_void_return(vi_pipe);

    drv_ctx = isp_drv_get_ctx(vi_pipe);
    drv_ctx_dist = drv_ctx;

    port_int_f_start = drv_ctx->int_sch[int_num].port_int_status;
    isp_int_status = drv_ctx->int_sch[int_num].isp_int_status;

    isp_drv_proc_get_port_int_time1(drv_ctx, port_int_f_start, &pt_time1);
    isp_drv_proc_get_isp_int_time1(drv_ctx, isp_int_status, &isp_time1);

    if (isp_int_status & FE_INT_FEND) {
#ifdef CONFIG_OT_VI_DISTRIBUTE_GRP
    if (drv_ctx->dist_attr.distribute_en == TD_TRUE && ckfn_vi_get_distribute_working_id()) {
        vi_pipe_dist = call_vi_get_distribute_working_id(vi_pipe, TD_TRUE);
        isp_check_pipe_void_return(vi_pipe_dist);
        drv_ctx_dist = isp_drv_get_ctx(vi_pipe_dist);
    }
#endif
#ifdef CONFIG_OT_SYS_KOL_SUPPORT
        isp_drv_get_single_mode(vi_pipe_dist, &single_mode);
        if (single_mode || drv_ctx_dist->vd_standby_end == TD_FALSE) {
            ret = isp_drv_reg_config_sensor_standby(vi_pipe_dist, drv_ctx_dist);
            isp_check_ret_continue(vi_pipe_dist, ret, "isp_drv_reg_config_sensor_standby");
        }
#else
        ot_unused(single_mode);
        ot_unused(ret);
#endif
        drv_ctx->vd_end = TD_TRUE;
        drv_ctx_dist->vd_standby_end = TD_TRUE;
        osal_wait_wakeup(&drv_ctx->isp_wait_vd_end);
        osal_wait_wakeup(&drv_ctx_dist->isp_wait_vd_standby_end);
    }

    drv_ctx->int_pos = 0;

    if (isp_drv_get_frame_interrupt_attr(vi_pipe, &drv_ctx->frame_int_attr) != TD_SUCCESS) {
        drv_ctx->frame_int_attr.interrupt_type = OT_FRAME_INTERRUPT_START;
    }

    mask = (drv_ctx->frame_int_attr.interrupt_type == OT_FRAME_INTERRUPT_START) ? VI_WCH_INT_FSTART : VI_WCH_INT_DELAY0;

    if (isp_int_status & mask) {
        isp_drv_config_sensor(vi_pipe, drv_ctx);

        isp_drv_reg_config_isp_fe(vi_pipe);

        isp_drv_stat_buf_fe_insert(vi_pipe, drv_ctx);

        if (ckfn_sys_entry() && ckfn_sys_get_time_stamp()) {
            drv_ctx->sync_cfg.last_update_time = call_sys_get_time_stamp();
        }
    }
#ifdef CONFIG_OT_ISP_DYNAMIC_BLC_SUPPORT
    isp_drv_reg_config_runbe_fe_dynamic_blc(vi_pipe, drv_ctx, int_num);
#endif

    isp_drv_proc_calc_sensor_cfg_time(drv_ctx);
    isp_drv_proc_calc_port_int(drv_ctx, port_int_f_start, pt_time1);
    isp_drv_proc_calc_isp_int(drv_ctx, isp_int_status, isp_time1);
    isp_drv_proc_calc_avg_isp_int(drv_ctx, isp_int_status);

    return;
}

td_void isp_irq_route_run(ot_vi_pipe vi_pipe, td_u32 int_num)
{
    td_s32 ret;
    isp_drv_ctx *drv_ctx = TD_NULL;
    td_u32 port_int_f_start, isp_int_status;
    td_u64 pt_time1 = 0;
    td_u64 isp_time1 = 0;

    isp_check_pipe_void_return(vi_pipe);

    drv_ctx = isp_drv_get_ctx(vi_pipe);

    port_int_f_start = drv_ctx->int_sch[int_num].port_int_status;
    isp_int_status = drv_ctx->int_sch[int_num].isp_int_status;

    isp_drv_proc_get_port_int_time1(drv_ctx, port_int_f_start, &pt_time1);
    isp_drv_proc_get_isp_int_time1(drv_ctx, isp_int_status, &isp_time1);

    drv_ctx->int_pos = 0;

    ret = isp_drv_get_be_buf_use_node(vi_pipe, isp_int_status, int_num);
    if (ret != TD_SUCCESS) {
        return;
    }

    if (drv_ctx->yuv_mode == TD_FALSE) {
#ifdef CONFIG_OT_SNAP_SUPPORT
        if ((vi_pipe != drv_ctx->snap_attr.picture_pipe_id) ||
            ((drv_ctx->snap_attr.picture_pipe_id != -1 &&
            (drv_ctx->snap_attr.picture_pipe_id == drv_ctx->snap_attr.preview_pipe_id)))) {
            isp_irq_proc_by_wdr_mode(vi_pipe, drv_ctx, int_num);

            if (drv_ctx->snap_attr.snap_type == OT_SNAP_TYPE_PRO) {
                isp_irq_snap_process(vi_pipe, drv_ctx, int_num);
            }
        }
#else
        isp_irq_proc_by_wdr_mode(vi_pipe, drv_ctx, int_num);
#endif
    } else {
        isp_irq_yuv_process(vi_pipe, drv_ctx, int_num);
    }

    isp_drv_proc_calc_sensor_cfg_time(drv_ctx);
    isp_drv_proc_calc_port_int(drv_ctx, port_int_f_start, pt_time1);
    isp_drv_proc_calc_isp_int(drv_ctx, isp_int_status, isp_time1);
    isp_drv_proc_calc_avg_isp_int(drv_ctx, isp_int_status);

    return;
}

td_void isp_irq_route(ot_vi_pipe vi_pipe, td_u32 int_num)
{
    isp_check_pipe_void_return(vi_pipe);

    if (g_isp_run_wakeup_sel[vi_pipe] == OT_ISP_RUN_WAKEUP_BE_END) {
        isp_irq_route_run_plus(vi_pipe, int_num);
    } else {
        isp_irq_route_run(vi_pipe, int_num);
    }

    return;
}

#ifdef CONFIG_OT_VI_STITCH_GRP
static td_void isp_stitch_irq_route(ot_vi_pipe vi_pipe, td_u32 isp_int_status,
    td_u32 port_int_status, td_u32 int_num)
{
    td_s32 j;
    td_s32 ret;
    ot_vi_pipe vi_pipes;
    isp_drv_ctx *drv_ctx = TD_NULL;
    isp_drv_ctx *drv_ctx_s = TD_NULL;
    td_u32 mask;
    unsigned long flags = 0;

    drv_ctx = isp_drv_get_ctx(vi_pipe);
    mask = (drv_ctx->frame_int_attr.interrupt_type == OT_FRAME_INTERRUPT_START) ? VI_WCH_INT_FSTART : VI_WCH_INT_DELAY0;
    if (isp_int_status & mask) {
        drv_ctx->isp_sync_ctrl.first_flag = TD_TRUE;
    }
    if (drv_ctx->stitch_attr.main_pipe == TD_FALSE) {
        return;
    }
    ret = isp_drv_stitch_sync(vi_pipe);
    if (ret != TD_SUCCESS) {
        return;
    }

    if (g_isp_run_wakeup_sel[vi_pipe] == OT_ISP_RUN_WAKEUP_BE_END) {
        osal_spin_lock_irqsave(&g_isp_sync_lock[vi_pipe], &flags);
        for (j = 0; j < drv_ctx->stitch_attr.stitch_pipe_num; j++) {
            vi_pipes = drv_ctx->stitch_attr.stitch_bind_id[j];
            drv_ctx_s = isp_drv_get_ctx(vi_pipes);

        drv_ctx_s->int_sch[int_num].isp_int_status = isp_int_status;
        drv_ctx_s->int_sch[int_num].port_int_status = port_int_status;
        drv_ctx_s->int_sch[int_num].port_int_err = drv_ctx->int_sch[int_num].port_int_err;

            isp_irq_route_run_plus(vi_pipes, int_num);
        }
        osal_spin_unlock_irqrestore(&g_isp_sync_lock[vi_pipe], &flags);
    } else {
        for (j = 0; j < drv_ctx->stitch_attr.stitch_pipe_num; j++) {
            vi_pipes = drv_ctx->stitch_attr.stitch_bind_id[j];
            drv_ctx_s = isp_drv_get_ctx(vi_pipes);

            drv_ctx_s->int_sch[int_num].isp_int_status = isp_int_status;
            drv_ctx_s->int_sch[int_num].port_int_status = port_int_status;
            drv_ctx_s->int_sch[int_num].port_int_err = drv_ctx->int_sch[int_num].port_int_err;

            isp_irq_route_run(vi_pipes, int_num);
        }
    }
}
#endif

static td_void isp_drv_dist_trans_int_process(ot_vi_pipe phy_pipe, ot_vi_pipe *vi_pipe, isp_drv_ctx **drv_ctx)
{
#ifdef CONFIG_OT_VI_DISTRIBUTE_GRP
    isp_drv_ctx *phy_drv_ctx = TD_NULL;

    phy_drv_ctx = isp_drv_get_ctx(phy_pipe);
    if (phy_drv_ctx->dist_attr.distribute_en == TD_FALSE) {
        return;
    }

    *drv_ctx = isp_drv_get_ctx(phy_drv_ctx->dist_proc_pipe);
    *vi_pipe = phy_drv_ctx->dist_proc_pipe;
#else
    ot_unused(phy_pipe);
    ot_unused(vi_pipe);
    ot_unused(drv_ctx);

#endif
}

int isp_drv_int_bottom_half(int irq)
{
    ot_vi_pipe vi_pipe;
    td_u32 i;
    td_u32 j;
    td_u32 isp_int_read_num = 0;

    isp_drv_ctx *drv_ctx = TD_NULL;
    td_u32 port_int_status;
    td_u32 isp_int_status;
    td_u32 wch_int_f_start;
    td_bool slave_pipe_int_active;
    ot_unused(irq);

    for (i = 0; i < OT_ISP_MAX_PHY_PIPE_NUM; i++) {
        vi_pipe = i;
        isp_int_read_num = 0;
        drv_ctx = isp_drv_get_ctx(vi_pipe);
        isp_drv_dist_trans_int_process(i, &vi_pipe, &drv_ctx);
        if (isp_get_interrupt_info(vi_pipe, drv_ctx, &isp_int_read_num) == 0) {
            continue;
        }

        for (j = 0; j < isp_int_read_num; j++) {
            isp_int_status = drv_ctx->int_sch[j].isp_int_status;
            port_int_status = drv_ctx->int_sch[j].port_int_status;
            wch_int_f_start = drv_ctx->int_sch[j].wch_int_status;
            isp_get_slave_pipe_int_status(vi_pipe, &slave_pipe_int_active);
            if (!port_int_status && !isp_int_status && !wch_int_f_start && (slave_pipe_int_active == TD_FALSE)) {
                continue;
            }

            if (!drv_ctx->mem_init) {
                continue;
            }

            if (drv_ctx->stitch_attr.stitch_enable == TD_TRUE) {
#ifdef CONFIG_OT_VI_STITCH_GRP
                isp_stitch_irq_route(vi_pipe, isp_int_status, port_int_status, j);
#endif
            } else {
                isp_irq_route(vi_pipe, j);
            }
        }
    }

    return OSAL_IRQ_HANDLED;
}

static int isp_drv_init(void)
{
    td_s32 ret;

    ret = isp_drv_be_remap();
    if (ret == TD_FAILURE) {
        return ret;
    }

    ret = isp_drv_vicap_remap();
    if (ret == TD_FAILURE) {
        goto OUT2;
    }

    ret = isp_drv_fe_remap();
    if (ret == TD_FAILURE) {
        goto OUT1;
    }
#ifdef CONFIG_OT_ISP_LITEOS_BOTTOM_HALF_SUPPORT
    if (isp_drv_work_queue_init() != TD_SUCCESS) {
        goto OUT0;
    }
#endif
    return 0;

#ifdef CONFIG_OT_ISP_LITEOS_BOTTOM_HALF_SUPPORT
OUT0:
    isp_drv_fe_unmap();
#endif
OUT1:
    isp_drv_vicap_unmap();
OUT2:
    isp_drv_be_unmap();

    return ret;
}

static int isp_drv_exit(void)
{
    isp_drv_be_unmap();

    isp_drv_vicap_unmap();

    isp_drv_fe_unmap();
#ifdef CONFIG_OT_ISP_LITEOS_BOTTOM_HALF_SUPPORT
    isp_drv_work_queue_exit();
#endif

    return 0;
}

#ifdef CONFIG_OT_SYS_KOL_SUPPORT
isp_fe_reg_type g_fe_reg_mem[OT_ISP_MAX_FE_PIPE_NUM];
td_u32 g_fe_ae_weight[ISP_FE_WEIGHT_NUM];

#define RECORD_REG_PARTS  8
td_u32 g_need_record_reg[RECORD_REG_PARTS][2] = {{0x100, 0x1F0}, {0x820, 0x8A4}, /* 8 parts reg addr */
    {0x110C, 0x1114}, {0x1d04, 0x1d20}, {0x1f04, 0x1f20}, {0x280c, 0x2814}, {0x2840, 0x2878}, {0xb10c, 0xb114}};

#define SALVE_MODE_REG_NUM  8
static td_u32 g_slave_mode_reg[SALVE_MODE_REG_NUM] = {0x0, 0x4, 0x8, 0xc, 0x18, 0x1c, 0x20, 0x2c};
static td_u32 g_slave_mode_reg_mem[CAP_SLAVE_MAX_NUM][SALVE_MODE_REG_NUM];
static td_u32 g_slave_mode_time_sel;

td_void isp_clear_kol_int_num(ot_vi_pipe vi_pipe)
{
    td_s32 ret;
#ifdef CONFIG_OT_VI_DISTRIBUTE_GRP
    vi_distribute_attr attr_tmp = { 0 };
    ot_vi_pipe slave_pipe;
#endif
    isp_drv_ctx *drv_ctx = TD_NULL;
    isp_check_pipe_void_return(vi_pipe);
    drv_ctx = isp_drv_get_ctx(vi_pipe);
    ret = memset_s(drv_ctx->kol_int_info.kol_int_num, sizeof(drv_ctx->kol_int_info.kol_int_num), 0,
        sizeof(drv_ctx->kol_int_info.kol_int_num));
    isp_check_eok_void(ret);
    drv_ctx->kol_int_info.kol_frame_num = 0;
#ifdef CONFIG_OT_VI_DISTRIBUTE_GRP
    if (ckfn_vi_get_distribute_attr()) {
        ret = call_vi_get_distribute_attr(vi_pipe, &attr_tmp);
        if (ret != TD_SUCCESS) {
            isp_err_trace("pipe[%d] vi_get_dist_attr error\n", vi_pipe);
            return;
        }
        if (attr_tmp.distribute_en) {
            slave_pipe = attr_tmp.pipe_id[1];
            isp_check_pipe_void_return(slave_pipe);
            drv_ctx = isp_drv_get_ctx(slave_pipe);
            ret = memset_s(drv_ctx->kol_int_info.kol_int_num, sizeof(drv_ctx->kol_int_info.kol_int_num), 0,
                sizeof(drv_ctx->kol_int_info.kol_int_num));
            isp_check_eok_void(ret);
            drv_ctx->kol_int_info.kol_frame_num = 0;
        }
    }
#endif
}

td_s32 isp_suspend(void *private_data)
{
    td_s32 vi_pipe, slave_id, i, j;
    isp_drv_ctx *drv_ctx = TD_NULL;
    td_u8 *src_reg_addr = TD_NULL;
    td_u8 *dst_reg_addr = TD_NULL;
    td_u8 *slv_reg_addr = TD_NULL;
    ot_unused(private_data);

    for (vi_pipe = 0; vi_pipe < OT_ISP_MAX_FE_PIPE_NUM; vi_pipe++) {
        drv_ctx = isp_drv_get_ctx(vi_pipe);
        if (drv_ctx->mem_init != TD_TRUE) {
            continue;
        }
        dst_reg_addr = (td_u8 *)&g_fe_reg_mem[vi_pipe];
        src_reg_addr = (td_u8 *)isp_drv_get_ispfe_base_va(vi_pipe);
        isp_check_pointer_return(src_reg_addr);
        for (j = 0; j < RECORD_REG_PARTS; j++) {
            for (i = g_need_record_reg[j][0]; i <= g_need_record_reg[j][1]; i += 4) { /* 4 byte */
                *(td_u32 *)(dst_reg_addr + i) = *(td_u32 *)(src_reg_addr + i);
            }
        }

        isp_drv_get_fe_ae_weight(vi_pipe, g_fe_ae_weight, ISP_FE_WEIGHT_NUM);
        isp_drv_set_int_enable(vi_pipe, TD_FALSE);
    }

    src_reg_addr = (td_u8 *)isp_drv_get_reg_vicap_base_va();
    isp_check_pointer_return(src_reg_addr);
    g_slave_mode_time_sel = *(td_u32 *)(src_reg_addr + (SLAVE_MODE_REG_BASE - CAP_REG_BASE));
    for (slave_id = 0; slave_id < CAP_SLAVE_MAX_NUM; slave_id++) {
        slv_reg_addr = src_reg_addr + (cap_slave_reg_base(slave_id) - CAP_REG_BASE);
        for (i = 0; i < SALVE_MODE_REG_NUM; i++) {
            g_slave_mode_reg_mem[slave_id][i] = *(td_u32 *)(slv_reg_addr + g_slave_mode_reg[i]);
        }
    }
    return TD_SUCCESS;
}

td_s32 isp_resume(void *private_data)
{
    td_s32 vi_pipe, slave_id, i, j;
    isp_drv_ctx *drv_ctx = TD_NULL;
    volatile td_u8 *src_reg_addr = TD_NULL;
    volatile td_u8 *dst_reg_addr = TD_NULL;
    volatile td_u8 *slv_reg_addr = TD_NULL;

    ot_unused(private_data);

    dst_reg_addr = (td_u8 *)isp_drv_get_reg_vicap_base_va();
    isp_check_pointer_return(dst_reg_addr);
    for (slave_id = 0; slave_id < CAP_SLAVE_MAX_NUM; slave_id++) {
        slv_reg_addr = dst_reg_addr + (cap_slave_reg_base(slave_id) - CAP_REG_BASE);
        for (i = 0; i < SALVE_MODE_REG_NUM; i++) {
            *(td_u32 *)(slv_reg_addr + g_slave_mode_reg[i]) = g_slave_mode_reg_mem[slave_id][i];
        }
    }
    *(td_u32 *)(dst_reg_addr + (SLAVE_MODE_REG_BASE - CAP_REG_BASE)) = g_slave_mode_time_sel;
    *(td_u32 *)(dst_reg_addr + (SLAVE_MODE_REG_SYNC - CAP_REG_BASE)) = (0x1 << CAP_SLAVE_MAX_NUM) - 1;
    *(td_u32 *)(dst_reg_addr + (SLAVE_MODE_REG_SYNC - CAP_REG_BASE)) = 0x0;

    for (vi_pipe = 0; vi_pipe < OT_ISP_MAX_FE_PIPE_NUM; vi_pipe++) {
        drv_ctx = isp_drv_get_ctx(vi_pipe);
        if (drv_ctx->mem_init != TD_TRUE) {
            continue;
        }
        src_reg_addr = (td_u8 *)&g_fe_reg_mem[vi_pipe];
        dst_reg_addr = (td_u8 *)isp_drv_get_ispfe_base_va(vi_pipe);
        isp_check_pointer_return(dst_reg_addr);
        for (j = 0; j < RECORD_REG_PARTS; j++) {
            for (i = g_need_record_reg[j][0]; i <= g_need_record_reg[j][1]; i += 4) { /* 4 byte */
                *(td_u32 *)(dst_reg_addr + i) = *(td_u32 *)(src_reg_addr + i);
            }
        }
        isp_drv_set_fe_ae_weight(vi_pipe, g_fe_ae_weight, ISP_FE_WEIGHT_NUM);
        isp_clear_kol_int_num(vi_pipe);
        isp_drv_set_int_enable(vi_pipe, TD_TRUE);
    }
    return TD_SUCCESS;
}

static osal_pmops g_isp_pm_ops = {
    .pm_suspend = isp_suspend,
    .pm_resume = isp_resume,
};
#endif

td_s32 isp_kern_init(void *p)
{
    td_u32 vi_pipe;
    ot_unused(p);

    if (check_func_entry(OT_ID_VI) != TD_TRUE || check_func_entry(OT_ID_SYS) != TD_TRUE) {
        OT_PRINT("vi.ko or sys.ko not ready\n");
        return TD_FAILURE;
    }

    for (vi_pipe = 0; vi_pipe < OT_ISP_MAX_PIPE_NUM; vi_pipe++) {
        (td_void)memset_s(&g_isp_drv_ctx[vi_pipe].drv_dbg_info, sizeof(isp_drv_dbg_info), 0, sizeof(isp_drv_dbg_info));
    }

    return TD_SUCCESS;
}

td_void isp_kern_exit(void)
{
    td_u32 vi_pipe;
    td_void *reg_vicap_base_va = TD_NULL;
    td_void  *reg_ispfe_base_va[OT_ISP_MAX_PIPE_NUM] = {TD_NULL};

    reg_vicap_base_va = isp_drv_get_reg_vicap_base_va();
    for (vi_pipe = 0; vi_pipe < OT_ISP_MAX_PIPE_NUM; vi_pipe++) {
        reg_ispfe_base_va[vi_pipe] = isp_drv_get_ispfe_base_va(vi_pipe);
        if ((reg_vicap_base_va != TD_NULL) && (reg_ispfe_base_va[vi_pipe] != TD_NULL)) {
            isp_drv_set_int_enable(vi_pipe, TD_FALSE);
        }
        (td_void)memset_s(&g_isp_drv_ctx[vi_pipe].drv_dbg_info, sizeof(isp_drv_dbg_info), 0, sizeof(isp_drv_dbg_info));
#ifdef CONFIG_OT_ISP_DETAIL_STATS_SUPPORT
        isp_drv_reset_detail_stats_cfg(vi_pipe);
#endif
    }

    return;
}

static td_u32 isp_get_ver_magic(td_void)
{
    return VERSION_MAGIC;
}

static umap_module g_isp_module = {
    .mod_id = OT_ID_ISP,
    .mod_name = "isp",
    .pfn_init = isp_kern_init,
    .pfn_exit = isp_kern_exit,
    .pfn_ver_checker = isp_get_ver_magic,
    .export_funcs = TD_NULL,
    .data = TD_NULL,
};

static td_s32 isp_creat_device(td_void)
{
    g_isp_device = osal_dev_create("isp_dev");
    if (g_isp_device == TD_NULL) {
        OT_PRINT("isp create dev failed\n");
        return TD_FAILURE;
    }

    if (isp_init_ioctl_info_check() != TD_SUCCESS) {
        OT_PRINT("isp init ioctl info failed\n");
        goto destroy_dev;
    }
    isp_set_ioctl_cmd_list(&g_isp_file_ops);
    g_isp_device->fops = &g_isp_file_ops;
    g_isp_device->minor = UMAP_ISP_MINOR_BASE;
#ifdef CONFIG_OT_SYS_KOL_SUPPORT
    g_isp_device->pmops = &g_isp_pm_ops;
#endif
    if (osal_dev_register(g_isp_device) < 0) {
        OT_PRINT("Kernel: Could not register isp devices\n");
        goto destroy_dev;
    }
    return TD_SUCCESS;

destroy_dev:
    osal_dev_destroy(g_isp_device);
    g_isp_device = TD_NULL;
    return TD_FAILURE;
}

static td_void isp_destory_device(td_void)
{
    osal_dev_unregister(g_isp_device);
    osal_dev_destroy(g_isp_device);
    g_isp_device = TD_NULL;
}

static td_s32 isp_module_init_spinlock(td_void)
{
    td_s32 ret;
    td_u32 vi_pipe;
    td_u32 i;

    for (vi_pipe = 0; vi_pipe < OT_ISP_MAX_PIPE_NUM; vi_pipe++) {
        ret = osal_spin_lock_init(&g_isp_lock[vi_pipe]);
        if (ret != OSAL_SUCCESS) {
            isp_err_trace("osal_spin_lock_init failed with %#x!\n", ret);
            goto out;
        }

        ret = osal_spin_lock_init(&g_isp_sync_lock[vi_pipe]);
        if (ret != OSAL_SUCCESS) {
            osal_spin_lock_destroy(&g_isp_lock[vi_pipe]);
            isp_err_trace("osal_spin_lock_init failed with %#x!\n", ret);
            goto out;
        }
#ifdef CONFIG_OT_ISP_DRC_SWITCH_SUPPORT
        ret = osal_spin_lock_init(&g_isp_drc_lock[vi_pipe]);
        if (ret != OSAL_SUCCESS) {
            osal_spin_lock_destroy(&g_isp_lock[vi_pipe]);
            osal_spin_lock_destroy(&g_isp_sync_lock[vi_pipe]);
            isp_err_trace("osal_spin_lock_init failed with %#x!\n", ret);
            goto out;
        }
#endif
    }

    return TD_SUCCESS;

out:
    for (i = 0; i < vi_pipe; i++) {
        osal_spin_lock_destroy(&g_isp_lock[i]);
        osal_spin_lock_destroy(&g_isp_sync_lock[i]);
#ifdef CONFIG_OT_ISP_DRC_SWITCH_SUPPORT
        osal_spin_lock_destroy(&g_isp_drc_lock[i]);
#endif
    }

    return TD_FAILURE;
}

int isp_module_init(void)
{
    td_u32 vi_pipe;
#ifdef CONFIG_OT_PROC_SHOW_SUPPORT
    osal_proc_entry *proc = TD_NULL;
#endif
    if (isp_creat_device() != TD_SUCCESS) {
        return TD_FAILURE;
    }

#ifdef CONFIG_OT_PROC_SHOW_SUPPORT
    proc = osal_create_proc_entry(PROC_ENTRY_ISP, TD_NULL);
    if (proc == TD_NULL) {
        OT_PRINT("Kernel: Register isp proc failed!\n");
        goto OUT3;
    }

    proc->read = isp_proc_show;
#endif
    g_isp_module.export_funcs = isp_get_export_func();
    if (cmpi_register_module(&g_isp_module)) {
        goto OUT2;
    }

    if (isp_module_init_spinlock() != TD_SUCCESS) {
        cmpi_unregister_module(OT_ID_ISP);
        goto OUT2;
    }

    if (isp_drv_init() != 0) {
        OT_PRINT("isp init failed\n");
        goto OUT1;
    }

    isp_dfx_global_enable_init();

    OT_PRINT("load isp.ko ....OK!\n");
    return TD_SUCCESS;

OUT1:
    for (vi_pipe = 0; vi_pipe < OT_ISP_MAX_PIPE_NUM; vi_pipe++) {
        osal_spin_lock_destroy(&g_isp_lock[vi_pipe]);
        osal_spin_lock_destroy(&g_isp_sync_lock[vi_pipe]);
#ifdef CONFIG_OT_ISP_DRC_SWITCH_SUPPORT
        osal_spin_lock_destroy(&g_isp_drc_lock[vi_pipe]);
#endif
    }
    cmpi_unregister_module(OT_ID_ISP);
OUT2:
#ifdef CONFIG_OT_PROC_SHOW_SUPPORT
    osal_remove_proc_entry(PROC_ENTRY_ISP, TD_NULL);
OUT3:
#endif
    isp_destory_device();
    OT_PRINT("isp mod init failed!\n");
    return TD_FAILURE;
}

void isp_module_exit(void)
{
    int i;

    isp_drv_exit();

    for (i = 0; i < OT_ISP_MAX_PIPE_NUM; i++) {
        osal_spin_lock_destroy(&g_isp_lock[i]);
        osal_spin_lock_destroy(&g_isp_sync_lock[i]);
#ifdef CONFIG_OT_ISP_DRC_SWITCH_SUPPORT
        osal_spin_lock_destroy(&g_isp_drc_lock[i]);
#endif
    }

    cmpi_unregister_module(OT_ID_ISP);

#ifdef CONFIG_OT_PROC_SHOW_SUPPORT
    osal_remove_proc_entry(PROC_ENTRY_ISP, TD_NULL);
#endif
    isp_destory_device();

    OT_PRINT("unload isp.ko ....OK!\n");
}
