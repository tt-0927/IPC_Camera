/*
  Copyright (c), 2001-2025, Shenshu Tech. Co., Ltd.
 */
#ifndef ISP_SHARPEN_H
#define ISP_SHARPEN_H

#include "isp_config.h"
#include "ot_isp_debug.h"
#include "isp_ext_config.h"
#include "isp_math_utils.h"
#include "isp_alg.h"
#include "isp_sensor.h"
#include "isp_proc.h"
#include "isp_ext_reg_access.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif /* End of #ifdef __cplusplus */

typedef struct {
    td_u8   sft;
    td_u32  wgt_pre;
    td_u32  wgt_cur;
    td_s32  idx_cur;
    td_s32  idx_pre;
} isp_sharpen_inter_info;

typedef struct {
    td_bool init;
    td_bool sharpen_en;
    td_bool sharpen_mpi_update_en;
    td_u32  iso_last;
    td_u32  iso;
    td_u8   sft;
    td_u32  wgt_pre;
    td_u32  wgt_cur;
    td_u32  idx_cur;
    td_u32  idx_pre;

    /* sharpening yuv */
    /* tmp registers */
    td_u8   manual_sharpen_yuv_enabled;
    td_u8   gain_thd_sft_d;
    td_u8   dir_var_sft;

    td_u8   sel_pix_wgt;
    td_u8   rmf_gain_scale;
    td_u8   bmf_gain_scale;

    td_u8   gain_thd_sel_ud;
    td_u8   gain_thd_sft_ud;

    td_u8   sht_var_wgt0;
    td_u8   sht_var_diff_thd0;
    td_u8   sht_var_diff_thd1;
    td_u8   sht_var_diff_wgt1;

    /* MPI */
    td_u8   skin_umin;
    td_u8   skin_vmin;
    td_u8   skin_umax;
    td_u8   skin_vmax;
    td_u16  texture_str[OT_ISP_SHARPEN_GAIN_NUM];
    td_u16  edge_str[OT_ISP_SHARPEN_GAIN_NUM];
    td_u8   luma_wgt[OT_ISP_SHARPEN_LUMA_NUM];
    td_u16  texture_freq;
    td_u16  edge_freq;
    td_u8   over_shoot;
    td_u8   under_shoot;
    td_u8   shoot_sup_str;
    td_u8   shoot_sup_adj;
    td_u8   detail_ctrl;
    td_u8   detail_ctrl_thr;
    td_u8   edge_filt_str;
    td_u8   edge_filt_max_cap;
    td_u8   r_gain;
    td_u8   g_gain;
    td_u8   b_gain;
    td_u8   skin_gain;
    td_u16  max_sharp_gain;
//    td_u8   weak_detail_gain;

    td_u16  shoot_inner_threshold;
    td_u16  shoot_outer_threshold;
    td_u16  shoot_protect_threshold;

    td_u8   dir_var_thd;
    td_u8   mot_thd;
    td_u8   std_comb_mode;
    td_u8   std_comb_alpha;
    td_u8   mot_interp_mode;

    td_u16  edge_rly_fine_threshold;
    td_u16  edge_rly_fine_threshold_delta;
    td_u16  edge_rly_coarse_threshold;
    td_u16  edge_rly_coarse_threshold_delta;

    td_u8   edge_overshoot;
    td_u8   edge_undershoot;

    td_u8   edge_gain_by_rly[OT_ISP_SHARPEN_RLYWGT_NUM];
    td_u8   edge_rly_by_mot[OT_ISP_SHARPEN_STDGAIN_NUM];
    td_u8   edge_rly_by_luma[OT_ISP_SHARPEN_STDGAIN_NUM];
    td_u8   mf_gain_by_mot[OT_ISP_SHARPEN_MOT_NUM];
    td_u8   hf_gain_by_mot[OT_ISP_SHARPEN_MOT_NUM];
    td_u8   lmf_gain_by_mot[OT_ISP_SHARPEN_MOT_NUM];

    ot_isp_sharpen_auto_attr   auto_attr;
} isp_sharpen_ctx;

td_s32 isp_sharpen_param_init(ot_vi_pipe vi_pipe, isp_reg_cfg *reg_cfg, isp_sharpen_ctx *shp_ctx);

td_void isp_sharpen_read_extregs(ot_vi_pipe vi_pipe, isp_sharpen_ctx *shp_ctx);
td_void isp_sharpen_read_pro_mode(ot_vi_pipe vi_pipe, const isp_sharpen_ctx *shp_ctx);
td_void isp_sharpen_reg_update(ot_vi_pipe vi_pipe, isp_reg_cfg *reg_cfg, isp_sharpen_ctx *shp_ctx);
td_s32  sharpen_proc_write(ot_isp_ctrl_proc_write *proc, const isp_sharpen_ctx *shp_ctx);

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* End of #ifdef __cplusplus */

#endif
