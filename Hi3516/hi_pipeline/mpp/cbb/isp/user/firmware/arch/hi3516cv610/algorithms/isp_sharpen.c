/*
  Copyright (c), 2001-2025, Shenshu Tech. Co., Ltd.
 */
#include "isp_sharpen.h"
#include "isp_config.h"
#include "isp_ext_config.h"
#include "isp_math_utils.h"
#include "isp_alg.h"
#include "isp_sensor.h"
#include "isp_proc.h"
#include "isp_param_check.h"
#include "isp_ext_reg_access.h"
#define     SHRP_GAIN_LUT_SFT                   2
#define     SHRP_DIR_RANGE_MUL_PRECS            4
#define     SHRP_HF_EPS_MUL_PRECS               4
#define     SHRP_SHT_VAR_MUL_PRECS              4
#define     SHRP_SKIN_EDGE_MUL_PRECS            4
#define     SHRP_SKIN_ACCUM_MUL_PRECS           3
#define     SHRP_CHR_MUL_SFT                    4
#define     SHARPEN_EN                          1
#define     SHRP_DETAIL_SHT_MUL_PRECS           4
#define     SHRP_DETAIL_CTRL_THR_DELTA          16
#define     SHRP_EXLUMA_MUL_PRECS               8
#define  SHRP_ISO_NUM               OT_ISP_AUTO_ISO_NUM

static isp_sharpen_ctx *g_sharpen_ctx[OT_ISP_MAX_PIPE_NUM] = {TD_NULL};

#define sharpen_get_ctx(pipe, ctx)   ((ctx) = g_sharpen_ctx[pipe])
#define sharpen_set_ctx(pipe, ctx)   (g_sharpen_ctx[pipe] = (ctx))
#define sharpen_reset_ctx(pipe)      (g_sharpen_ctx[pipe] = TD_NULL)

static td_s32 shrp_blend(td_u8 sft, td_s32 wgt1, td_s32 v1, td_s32 wgt2, td_s32 v2)
{
    td_s32 res;
    res = (td_s32)signed_right_shift(((td_s64)v1 * wgt1) + ((td_s64)v2 * wgt2), sft);
    return res;
}

static td_s32 sharpen_ctx_init(ot_vi_pipe vi_pipe)
{
    isp_sharpen_ctx *sharpen_ctx = TD_NULL;

    sharpen_get_ctx(vi_pipe, sharpen_ctx);

    if (sharpen_ctx == TD_NULL) {
        sharpen_ctx = (isp_sharpen_ctx *)isp_malloc(sizeof(isp_sharpen_ctx));
        if (sharpen_ctx == TD_NULL) {
            isp_err_trace("isp[%d] sharpen_ctx malloc memory failed!\n", vi_pipe);
            return OT_ERR_ISP_NOMEM;
        }
    }

    (td_void)memset_s(sharpen_ctx, sizeof(isp_sharpen_ctx), 0, sizeof(isp_sharpen_ctx));

    sharpen_set_ctx(vi_pipe, sharpen_ctx);

    return TD_SUCCESS;
}

static td_void sharpen_ctx_exit(ot_vi_pipe vi_pipe)
{
    isp_sharpen_ctx *sharpen_ctx = TD_NULL;

    sharpen_get_ctx(vi_pipe, sharpen_ctx);
    isp_free(sharpen_ctx);
    sharpen_reset_ctx(vi_pipe);
}

static td_s32 sharpen_check_cmos_param(ot_vi_pipe vi_pipe, const ot_isp_sharpen_attr *sharpen)
{
    td_s32 ret;
    ot_unused(vi_pipe);

    ret = isp_sharpen_comm_attr_check("cmos", sharpen);
    if (ret != TD_SUCCESS) {
        return ret;
    }

    ret = isp_sharpen_auto_attr_check("cmos", &sharpen->auto_attr);
    if (ret != TD_SUCCESS) {
        return ret;
    }

    ret = isp_sharpen_manual_attr_check("cmos", &sharpen->manual_attr);
    if (ret != TD_SUCCESS) {
        return ret;
    }

    return TD_SUCCESS;
}

static td_void sharpen_ext_regs_cmos_auto_initialize(ot_vi_pipe vi_pipe, const ot_isp_sharpen_attr *sharpen)
{
    td_u8 i;
    td_u16 j, index;
    for (i = 0; i < SHRP_ISO_NUM; i++) {
        for (j = 0; j < OT_ISP_SHARPEN_GAIN_NUM; j++) {
            index = i + j * SHRP_ISO_NUM;
            ot_ext_system_sharpen_texture_str_write(vi_pipe, index, sharpen->auto_attr.texture_strength[j][i]);
            ot_ext_system_sharpen_edge_str_write(vi_pipe, index, sharpen->auto_attr.edge_strength[j][i]);
        }
        for (j = 0; j < OT_ISP_SHARPEN_LUMA_NUM; j++) {
            index = i + j * SHRP_ISO_NUM;
            ot_ext_system_sharpen_luma_wgt_write(vi_pipe, index, sharpen->auto_attr.luma_wgt[j][i]);
        }
        ot_ext_system_sharpen_texture_freq_write(vi_pipe, i, sharpen->auto_attr.texture_freq[i]);
        ot_ext_system_sharpen_edge_freq_write(vi_pipe, i, sharpen->auto_attr.edge_freq[i]);
        ot_ext_system_sharpen_over_shoot_write(vi_pipe, i, sharpen->auto_attr.over_shoot[i]);
        ot_ext_system_sharpen_under_shoot_write(vi_pipe, i, sharpen->auto_attr.under_shoot[i]);
        ot_ext_system_sharpen_shoot_sup_str_write(vi_pipe, i, sharpen->auto_attr.shoot_sup_strength[i]);
        ot_ext_system_sharpen_detailctrl_write(vi_pipe, i, sharpen->auto_attr.detail_ctrl[i]);
        ot_ext_system_sharpen_edge_filt_str_write(vi_pipe, i, sharpen->auto_attr.edge_filt_strength[i]);
        ot_ext_system_sharpen_edge_filt_max_cap_write(vi_pipe, i, sharpen->auto_attr.edge_filt_max_cap[i]);
        ot_ext_system_sharpen_r_gain_write(vi_pipe, i, sharpen->auto_attr.r_gain[i]);
        ot_ext_system_sharpen_g_gain_write(vi_pipe, i, sharpen->auto_attr.g_gain[i]);
        ot_ext_system_sharpen_b_gain_write(vi_pipe, i, sharpen->auto_attr.b_gain[i]);
        ot_ext_system_sharpen_skin_gain_write(vi_pipe, i, sharpen->auto_attr.skin_gain[i]);
        ot_ext_system_sharpen_shoot_sup_adj_write(vi_pipe, i, sharpen->auto_attr.shoot_sup_adj[i]);
        ot_ext_system_sharpen_max_sharp_gain_write(vi_pipe, i, sharpen->auto_attr.max_sharp_gain[i]);
        ot_ext_system_sharpen_detailctrl_thr_write(vi_pipe, i, sharpen->auto_attr.detail_ctrl_threshold[i]);

        ot_ext_system_sharpen_shoot_inner_threshold_write(vi_pipe, i,
            sharpen->auto_attr.shoot_threshold_attr.shoot_inner_threshold[i]);
        ot_ext_system_sharpen_shoot_outer_threshold_write(vi_pipe, i,
            sharpen->auto_attr.shoot_threshold_attr.shoot_outer_threshold[i]);
        ot_ext_system_sharpen_shoot_protect_threshold_write(vi_pipe, i,
            sharpen->auto_attr.shoot_threshold_attr.shoot_protect_threshold[i]);
        ot_ext_system_sharpen_edge_rly_fine_threshold_write(vi_pipe, i,
            sharpen->auto_attr.edge_rly_attr.edge_rly_fine_threshold[i]);
        ot_ext_system_sharpen_edge_rly_coarse_threshold_write(vi_pipe, i,
            sharpen->auto_attr.edge_rly_attr.edge_rly_coarse_threshold[i]);
        ot_ext_system_sharpen_edge_overshoot_write(vi_pipe, i, sharpen->auto_attr.edge_rly_attr.edge_overshoot[i]);
        ot_ext_system_sharpen_edge_undershoot_write(vi_pipe, i, sharpen->auto_attr.edge_rly_attr.edge_undershoot[i]);

        for (j = 0; j < OT_ISP_SHARPEN_RLYWGT_NUM; j++) {
            index = i + j * SHRP_ISO_NUM;
            ot_ext_system_sharpen_edge_gain_by_rly_write(vi_pipe, (td_u8)index,
                sharpen->auto_attr.edge_rly_attr.edge_gain_by_rly[j][i]);
        }

        for (j = 0; j < OT_ISP_SHARPEN_STDGAIN_NUM; j++) {
            index = i + j * SHRP_ISO_NUM;
            ot_ext_system_sharpen_edge_rly_by_mot_write(vi_pipe, (td_u8)index,
                sharpen->auto_attr.edge_rly_attr.edge_rly_by_mot[j][i]);
            ot_ext_system_sharpen_edge_rly_by_luma_write(vi_pipe, (td_u8)index,
                sharpen->auto_attr.edge_rly_attr.edge_rly_by_luma[j][i]);
        }

        for (j = 0; j < OT_ISP_SHARPEN_MOT_NUM; j++) {
            index = i + j * SHRP_ISO_NUM;
            ot_ext_system_sharpen_mf_gain_by_mot_write(vi_pipe, (td_u8)index,
                sharpen->auto_attr.gain_by_mot_attr.mf_gain_by_mot[j][i]);
            ot_ext_system_sharpen_hf_gain_by_mot_write(vi_pipe, (td_u8)index,
                sharpen->auto_attr.gain_by_mot_attr.hf_gain_by_mot[j][i]);
            ot_ext_system_sharpen_lmf_gain_by_mot_write(vi_pipe, (td_u8)index,
                sharpen->auto_attr.gain_by_mot_attr.lmf_gain_by_mot[j][i]);
        }
    }
}

static td_s32 sharpen_ext_regs_cmos_manual_initialize(ot_vi_pipe vi_pipe, const ot_isp_sharpen_attr *sharpen)
{
    td_u8 i;

    for (i = 0; i < OT_ISP_SHARPEN_GAIN_NUM; i++) {
        ot_ext_system_manual_sharpen_texture_str_write(vi_pipe, i, sharpen->manual_attr.texture_strength[i]);
        ot_ext_system_manual_sharpen_edge_str_write(vi_pipe, i, sharpen->manual_attr.edge_strength[i]);
    }
    for (i = 0; i < OT_ISP_SHARPEN_LUMA_NUM; i++) {
        ot_ext_system_manual_sharpen_luma_wgt_write(vi_pipe, i, sharpen->manual_attr.luma_wgt[i]);
    }

    ot_ext_system_manual_sharpen_texture_freq_write(vi_pipe, sharpen->manual_attr.texture_freq);
    ot_ext_system_manual_sharpen_edge_freq_write(vi_pipe, sharpen->manual_attr.edge_freq);
    ot_ext_system_manual_sharpen_over_shoot_write(vi_pipe, sharpen->manual_attr.over_shoot);
    ot_ext_system_manual_sharpen_under_shoot_write(vi_pipe, sharpen->manual_attr.under_shoot);
    ot_ext_system_manual_sharpen_shoot_sup_str_write(vi_pipe, sharpen->manual_attr.shoot_sup_strength);
    ot_ext_system_manual_sharpen_detailctrl_write(vi_pipe, sharpen->manual_attr.detail_ctrl);
    ot_ext_system_manual_sharpen_edge_filt_str_write(vi_pipe, sharpen->manual_attr.edge_filt_strength);
    ot_ext_system_manual_sharpen_edge_filt_max_cap_write(vi_pipe, sharpen->manual_attr.edge_filt_max_cap);
    ot_ext_system_manual_sharpen_r_gain_write(vi_pipe, sharpen->manual_attr.r_gain);
    ot_ext_system_manual_sharpen_g_gain_write(vi_pipe, sharpen->manual_attr.g_gain);
    ot_ext_system_manual_sharpen_b_gain_write(vi_pipe, sharpen->manual_attr.b_gain);
    ot_ext_system_manual_sharpen_skin_gain_write(vi_pipe, sharpen->manual_attr.skin_gain);
    ot_ext_system_manual_sharpen_shoot_sup_adj_write(vi_pipe, sharpen->manual_attr.shoot_sup_adj);
    ot_ext_system_manual_sharpen_max_sharp_gain_write(vi_pipe, sharpen->manual_attr.max_sharp_gain);
    ot_ext_system_manual_sharpen_detailctrl_thr_write(vi_pipe, sharpen->manual_attr.detail_ctrl_threshold);

    ot_ext_system_manual_sharpen_skin_umax_write(vi_pipe, sharpen->skin_umax);
    ot_ext_system_manual_sharpen_skin_umin_write(vi_pipe, sharpen->skin_umin);
    ot_ext_system_manual_sharpen_skin_vmax_write(vi_pipe, sharpen->skin_vmax);
    ot_ext_system_manual_sharpen_skin_vmin_write(vi_pipe, sharpen->skin_vmin);

    ot_ext_system_manual_sharpen_shoot_inner_threshold_write(vi_pipe,
        sharpen->manual_attr.shoot_threshold_attr.shoot_inner_threshold);
    ot_ext_system_manual_sharpen_shoot_outer_threshold_write(vi_pipe,
        sharpen->manual_attr.shoot_threshold_attr.shoot_outer_threshold);
    ot_ext_system_manual_sharpen_shoot_protect_threshold_write(vi_pipe,
        sharpen->manual_attr.shoot_threshold_attr.shoot_protect_threshold);

    ot_ext_system_actual_sharpen_overshoot_amt_write(vi_pipe, sharpen->manual_attr.over_shoot);
    ot_ext_system_actual_sharpen_undershoot_amt_write(vi_pipe, sharpen->manual_attr.under_shoot);
    ot_ext_system_actual_sharpen_shoot_sup_write(vi_pipe, sharpen->manual_attr.shoot_sup_strength);
    ot_ext_system_actual_sharpen_edge_frequence_write(vi_pipe, sharpen->manual_attr.edge_freq);
    ot_ext_system_actual_sharpen_texture_frequence_write(vi_pipe, sharpen->manual_attr.texture_freq);

    for (i = 0; i < OT_ISP_SHARPEN_GAIN_NUM; i++) {
        ot_ext_system_actual_sharpen_edge_str_write(vi_pipe, i, sharpen->manual_attr.edge_strength[i]);
        ot_ext_system_actual_sharpen_texture_str_write(vi_pipe, i, sharpen->manual_attr.texture_strength[i]);
    }

    ot_ext_system_manual_sharpen_shoot_inner_threshold_write(vi_pipe,
        sharpen->manual_attr.shoot_threshold_attr.shoot_inner_threshold);
    ot_ext_system_manual_sharpen_shoot_outer_threshold_write(vi_pipe,
        sharpen->manual_attr.shoot_threshold_attr.shoot_outer_threshold);
    ot_ext_system_manual_sharpen_shoot_protect_threshold_write(vi_pipe,
        sharpen->manual_attr.shoot_threshold_attr.shoot_protect_threshold);

    ot_ext_system_manual_sharpen_edge_rly_fine_threshold_write(vi_pipe,
        sharpen->manual_attr.edge_rly_attr.edge_rly_fine_threshold);
    ot_ext_system_manual_sharpen_edge_rly_coarse_threshold_write(vi_pipe,
        sharpen->manual_attr.edge_rly_attr.edge_rly_coarse_threshold);
    ot_ext_system_manual_sharpen_edge_overshoot_write(vi_pipe,
        sharpen->manual_attr.edge_rly_attr.edge_overshoot);
    ot_ext_system_manual_sharpen_edge_undershoot_write(vi_pipe,
        sharpen->manual_attr.edge_rly_attr.edge_undershoot);

    for (i = 0; i < OT_ISP_SHARPEN_RLYWGT_NUM; i++) {
        ot_ext_system_manual_sharpen_edge_gain_by_rly_write(vi_pipe, i,
            sharpen->manual_attr.edge_rly_attr.edge_gain_by_rly[i]);
    }

    for (i = 0; i < OT_ISP_SHARPEN_STDGAIN_NUM; i++) {
        ot_ext_system_manual_sharpen_edge_rly_by_mot_write(vi_pipe, i,
            sharpen->manual_attr.edge_rly_attr.edge_rly_by_mot[i]);
        ot_ext_system_manual_sharpen_edge_rly_by_luma_write(vi_pipe, i,
            sharpen->manual_attr.edge_rly_attr.edge_rly_by_luma[i]);
    }

    for (i = 0; i < OT_ISP_SHARPEN_MOT_NUM; i++) {
        ot_ext_system_manual_sharpen_mf_gain_by_mot_write(vi_pipe, i,
            sharpen->manual_attr.gain_by_mot_attr.mf_gain_by_mot[i]);
        ot_ext_system_manual_sharpen_hf_gain_by_mot_write(vi_pipe, i,
            sharpen->manual_attr.gain_by_mot_attr.hf_gain_by_mot[i]);
        ot_ext_system_manual_sharpen_lmf_gain_by_mot_write(vi_pipe, i,
            sharpen->manual_attr.gain_by_mot_attr.lmf_gain_by_mot[i]);
    }

    return TD_SUCCESS;
}

static td_s32 sharpen_ext_regs_default_auto_initialize(ot_vi_pipe vi_pipe)
{
    td_u8 i;
    td_u16 j, index;
    for (i = 0; i < SHRP_ISO_NUM; i++) {
        for (j = 0; j < OT_ISP_SHARPEN_GAIN_NUM; j++) {
            index = i + j * SHRP_ISO_NUM;
            ot_ext_system_sharpen_texture_str_write(vi_pipe, index, OT_EXT_SYSTEM_MANUAL_SHARPEN_TEXTURESTR_DEF);
            ot_ext_system_sharpen_edge_str_write(vi_pipe, index, OT_EXT_SYSTEM_MANUAL_SHARPEN_EDGESTR_DEF);
        }
        for (j = 0; j < OT_ISP_SHARPEN_LUMA_NUM; j++) {
            index = i + j * SHRP_ISO_NUM;
            ot_ext_system_sharpen_luma_wgt_write(vi_pipe, index, OT_EXT_SYSTEM_MANUAL_SHARPEN_LUMAWGT_DEF);
        }
        ot_ext_system_sharpen_texture_freq_write(vi_pipe, i, OT_EXT_SYSTEM_MANUAL_SHARPEN_TEXTUREFREQ_DEF);
        ot_ext_system_sharpen_edge_freq_write(vi_pipe, i, OT_EXT_SYSTEM_MANUAL_SHARPEN_EDGEFREQ_DEF);
        ot_ext_system_sharpen_over_shoot_write(vi_pipe, i, OT_EXT_SYSTEM_MANUAL_SHARPEN_OVERSHOOT_DEF);
        ot_ext_system_sharpen_under_shoot_write(vi_pipe, i, OT_EXT_SYSTEM_MANUAL_SHARPEN_UNDERSHOOT_DEF);
        ot_ext_system_sharpen_shoot_sup_str_write(vi_pipe, i, OT_EXT_SYSTEM_MANUAL_SHARPEN_SHOOTSUPSTR_DEF);
        ot_ext_system_sharpen_detailctrl_write(vi_pipe, i, OT_EXT_SYSTEM_MANUAL_SHARPEN_DETAILCTRL_DEF);
        ot_ext_system_sharpen_edge_filt_str_write(vi_pipe, i, OT_EXT_SYSTEM_MANUAL_SHARPEN_EDGEFILTSTR_DEF);
        ot_ext_system_sharpen_edge_filt_max_cap_write(vi_pipe, i, OT_EXT_SYSTEM_MANUAL_SHARPEN_EDGEFILTMAXCAP_DEF);
        ot_ext_system_sharpen_r_gain_write(vi_pipe, i, OT_EXT_SYSTEM_MANUAL_SHARPEN_RGAIN_DEF);
        ot_ext_system_sharpen_g_gain_write(vi_pipe, i, OT_EXT_SYSTEM_MANUAL_SHARPEN_GGAIN_DEF);
        ot_ext_system_sharpen_b_gain_write(vi_pipe, i, OT_EXT_SYSTEM_MANUAL_SHARPEN_BGAIN_DEF);
        ot_ext_system_sharpen_skin_gain_write(vi_pipe, i, OT_EXT_SYSTEM_MANUAL_SHARPEN_SKINGAIN_DEF);
        ot_ext_system_sharpen_shoot_sup_adj_write(vi_pipe, i, OT_EXT_SYSTEM_MANUAL_SHARPEN_SHOOTSUPADJ_DEF);
        ot_ext_system_sharpen_max_sharp_gain_write(vi_pipe, i, OT_EXT_SYSTEM_MANUAL_SHARPEN_MAXSHARPGAIN_DEF);
        ot_ext_system_sharpen_detailctrl_thr_write(vi_pipe, i, OT_EXT_SYSTEM_MANUAL_SHARPEN_DETAILCTRLTHR_DEF);
        ot_ext_system_sharpen_weak_detail_gain_write(vi_pipe, i, OT_EXT_SYSTEM_MANUAL_SHARPEN_WEAKDETAILGAIN_DEF);

        ot_ext_system_sharpen_shoot_inner_threshold_write(vi_pipe, i,
            OT_EXT_SYSTEM_MANUAL_SHARPEN_SHOOTINNERTHRESHOLD_DEF);
        ot_ext_system_sharpen_shoot_outer_threshold_write(vi_pipe, i,
            OT_EXT_SYSTEM_MANUAL_SHARPEN_SHOOTOUTERTHRESHOLD_DEF);
        ot_ext_system_sharpen_shoot_protect_threshold_write(vi_pipe, i,
            OT_EXT_SYSTEM_MANUAL_SHARPEN_SHOOTPROTECTTHRESHOLD_DEF);

        ot_ext_system_sharpen_edge_rly_fine_threshold_write(vi_pipe, i,
            OT_EXT_SYSTEM_MANUAL_SHARPEN_EDGERLYFINETHRESHOLD_DEF);
        ot_ext_system_sharpen_edge_rly_coarse_threshold_write(vi_pipe, i,
            OT_EXT_SYSTEM_MANUAL_SHARPEN_EDGERLYCOARSETHRESHOLD_DEF);
        ot_ext_system_sharpen_edge_overshoot_write(vi_pipe, i, OT_EXT_SYSTEM_MANUAL_SHARPEN_EDGEOVERSHOOT_DEF);
        ot_ext_system_sharpen_edge_undershoot_write(vi_pipe, i, OT_EXT_SYSTEM_MANUAL_SHARPEN_EDGEUNDERSHOOT_DEF);

        for (j = 0; j < OT_ISP_SHARPEN_RLYWGT_NUM; j++) {
            index = i + j * SHRP_ISO_NUM;
            ot_ext_system_sharpen_edge_gain_by_rly_write(vi_pipe, (td_u8)index,
                OT_EXT_SYSTEM_MANUAL_SHARPEN_EDGEGAINBYRLY_DEF);
        }

        for (j = 0; j < OT_ISP_SHARPEN_STDGAIN_NUM; j++) {
            index = i + j * SHRP_ISO_NUM;
            ot_ext_system_sharpen_edge_rly_by_mot_write(vi_pipe, (td_u8)index,
                OT_EXT_SYSTEM_MANUAL_SHARPEN_EDGERLYBYMOT_DEF);
            ot_ext_system_sharpen_edge_rly_by_luma_write(vi_pipe, (td_u8)index,
                OT_EXT_SYSTEM_MANUAL_SHARPEN_EDGERLYBYLUMA_DEF);
        }

        for (j = 0; j < OT_ISP_SHARPEN_MOT_NUM; j++) {
            index = i + j * SHRP_ISO_NUM;
            ot_ext_system_sharpen_mf_gain_by_mot_write(vi_pipe, (td_u8)index,
                OT_EXT_SYSTEM_MANUAL_SHARPEN_MFGAINBYMOT_DEF);
            ot_ext_system_sharpen_hf_gain_by_mot_write(vi_pipe, (td_u8)index,
                OT_EXT_SYSTEM_MANUAL_SHARPEN_HFGAINBYMOT_DEF);
            ot_ext_system_sharpen_lmf_gain_by_mot_write(vi_pipe, (td_u8)index,
                OT_EXT_SYSTEM_MANUAL_SHARPEN_LMFGAINBYMOT_DEF);
        }
    }

    return TD_SUCCESS;
}

static td_s32 sharpen_ext_regs_default_manual_initialize(ot_vi_pipe vi_pipe)
{
    td_u8 i;

    for (i = 0; i < OT_ISP_SHARPEN_GAIN_NUM; i++) {
        ot_ext_system_manual_sharpen_texture_str_write(vi_pipe, i, OT_EXT_SYSTEM_MANUAL_SHARPEN_TEXTURESTR_DEF);
        ot_ext_system_manual_sharpen_edge_str_write(vi_pipe, i, OT_EXT_SYSTEM_MANUAL_SHARPEN_EDGESTR_DEF);
    }
    for (i = 0; i < OT_ISP_SHARPEN_LUMA_NUM; i++) {
        ot_ext_system_manual_sharpen_luma_wgt_write(vi_pipe, i, OT_EXT_SYSTEM_MANUAL_SHARPEN_LUMAWGT_DEF);
    }

    ot_ext_system_manual_sharpen_texture_freq_write(vi_pipe, OT_EXT_SYSTEM_MANUAL_SHARPEN_TEXTUREFREQ_DEF);
    ot_ext_system_manual_sharpen_edge_freq_write(vi_pipe, OT_EXT_SYSTEM_MANUAL_SHARPEN_EDGEFREQ_DEF);
    ot_ext_system_manual_sharpen_over_shoot_write(vi_pipe, OT_EXT_SYSTEM_MANUAL_SHARPEN_OVERSHOOT_DEF);
    ot_ext_system_manual_sharpen_under_shoot_write(vi_pipe, OT_EXT_SYSTEM_MANUAL_SHARPEN_UNDERSHOOT_DEF);
    ot_ext_system_manual_sharpen_shoot_sup_str_write(vi_pipe, OT_EXT_SYSTEM_MANUAL_SHARPEN_SHOOTSUPSTR_DEF);
    ot_ext_system_manual_sharpen_detailctrl_write(vi_pipe, OT_EXT_SYSTEM_MANUAL_SHARPEN_DETAILCTRL_DEF);
    ot_ext_system_manual_sharpen_edge_filt_str_write(vi_pipe, OT_EXT_SYSTEM_MANUAL_SHARPEN_EDGEFILTSTR_DEF);
    ot_ext_system_manual_sharpen_edge_filt_max_cap_write(vi_pipe, OT_EXT_SYSTEM_MANUAL_SHARPEN_EDGEFILTMAXCAP_DEF);
    ot_ext_system_manual_sharpen_r_gain_write(vi_pipe, OT_EXT_SYSTEM_MANUAL_SHARPEN_RGAIN_DEF);
    ot_ext_system_manual_sharpen_g_gain_write(vi_pipe, OT_EXT_SYSTEM_MANUAL_SHARPEN_GGAIN_DEF);
    ot_ext_system_manual_sharpen_b_gain_write(vi_pipe, OT_EXT_SYSTEM_MANUAL_SHARPEN_BGAIN_DEF);
    ot_ext_system_manual_sharpen_skin_gain_write(vi_pipe, OT_EXT_SYSTEM_MANUAL_SHARPEN_SKINGAIN_DEF);
    ot_ext_system_manual_sharpen_shoot_sup_adj_write(vi_pipe, OT_EXT_SYSTEM_MANUAL_SHARPEN_SHOOTSUPADJ_DEF);
    ot_ext_system_manual_sharpen_max_sharp_gain_write(vi_pipe, OT_EXT_SYSTEM_MANUAL_SHARPEN_MAXSHARPGAIN_DEF);
    ot_ext_system_manual_sharpen_detailctrl_thr_write(vi_pipe, OT_EXT_SYSTEM_MANUAL_SHARPEN_DETAILCTRLTHR_DEF);
    ot_ext_system_manual_sharpen_weak_detail_gain_write(vi_pipe, OT_EXT_SYSTEM_MANUAL_SHARPEN_WEAKDETAILGAIN_DEF);
    ot_ext_system_manual_sharpen_skin_umax_write(vi_pipe, OT_EXT_SYSTEM_MANUAL_SHARPEN_SKINUMAX_DEF);
    ot_ext_system_manual_sharpen_skin_umin_write(vi_pipe, OT_EXT_SYSTEM_MANUAL_SHARPEN_SKINUMIN_DEF);
    ot_ext_system_manual_sharpen_skin_vmax_write(vi_pipe, OT_EXT_SYSTEM_MANUAL_SHARPEN_SKINVMAX_DEF);
    ot_ext_system_manual_sharpen_skin_vmin_write(vi_pipe, OT_EXT_SYSTEM_MANUAL_SHARPEN_SKINVMIN_DEF);

    ot_ext_system_manual_sharpen_shoot_inner_threshold_write(vi_pipe,
        OT_EXT_SYSTEM_MANUAL_SHARPEN_SHOOTINNERTHRESHOLD_DEF);
    ot_ext_system_manual_sharpen_shoot_outer_threshold_write(vi_pipe,
        OT_EXT_SYSTEM_MANUAL_SHARPEN_SHOOTOUTERTHRESHOLD_DEF);
    ot_ext_system_manual_sharpen_shoot_protect_threshold_write(vi_pipe,
        OT_EXT_SYSTEM_MANUAL_SHARPEN_SHOOTPROTECTTHRESHOLD_DEF);

    ot_ext_system_actual_sharpen_overshoot_amt_write(vi_pipe, OT_EXT_SYSTEM_MANUAL_SHARPEN_OVERSHOOT_DEF);
    ot_ext_system_actual_sharpen_undershoot_amt_write(vi_pipe, OT_EXT_SYSTEM_MANUAL_SHARPEN_UNDERSHOOT_DEF);
    ot_ext_system_actual_sharpen_shoot_sup_write(vi_pipe, OT_EXT_SYSTEM_MANUAL_SHARPEN_SHOOTSUPSTR_DEF);
    ot_ext_system_actual_sharpen_edge_frequence_write(vi_pipe, OT_EXT_SYSTEM_MANUAL_SHARPEN_EDGEFREQ_DEF);
    ot_ext_system_actual_sharpen_texture_frequence_write(vi_pipe, OT_EXT_SYSTEM_MANUAL_SHARPEN_TEXTUREFREQ_DEF);

    for (i = 0; i < OT_ISP_SHARPEN_GAIN_NUM; i++) {
        ot_ext_system_actual_sharpen_edge_str_write(vi_pipe, i, OT_EXT_SYSTEM_MANUAL_SHARPEN_EDGESTR_DEF);
        ot_ext_system_actual_sharpen_texture_str_write(vi_pipe, i, OT_EXT_SYSTEM_MANUAL_SHARPEN_TEXTURESTR_DEF);
    }

    ot_ext_system_manual_sharpen_shoot_inner_threshold_write(vi_pipe,
        OT_EXT_SYSTEM_MANUAL_SHARPEN_SHOOTINNERTHRESHOLD_DEF);
    ot_ext_system_manual_sharpen_shoot_outer_threshold_write(vi_pipe,
        OT_EXT_SYSTEM_MANUAL_SHARPEN_SHOOTOUTERTHRESHOLD_DEF);
    ot_ext_system_manual_sharpen_shoot_protect_threshold_write(vi_pipe,
        OT_EXT_SYSTEM_MANUAL_SHARPEN_SHOOTPROTECTTHRESHOLD_DEF);

    ot_ext_system_manual_sharpen_edge_rly_fine_threshold_write(vi_pipe,
        OT_EXT_SYSTEM_MANUAL_SHARPEN_EDGERLYFINETHRESHOLD_DEF);
    ot_ext_system_manual_sharpen_edge_rly_coarse_threshold_write(vi_pipe,
        OT_EXT_SYSTEM_MANUAL_SHARPEN_EDGERLYCOARSETHRESHOLD_DEF);
    ot_ext_system_manual_sharpen_edge_overshoot_write(vi_pipe,
        OT_EXT_SYSTEM_MANUAL_SHARPEN_EDGEOVERSHOOT_DEF);
    ot_ext_system_manual_sharpen_edge_undershoot_write(vi_pipe,
        OT_EXT_SYSTEM_MANUAL_SHARPEN_EDGEUNDERSHOOT_DEF);

    for (i = 0; i < OT_ISP_SHARPEN_RLYWGT_NUM; i++) {
        ot_ext_system_manual_sharpen_edge_gain_by_rly_write(vi_pipe, i,
            OT_EXT_SYSTEM_MANUAL_SHARPEN_EDGEGAINBYRLY_DEF);
    }

    for (i = 0; i < OT_ISP_SHARPEN_STDGAIN_NUM; i++) {
        ot_ext_system_manual_sharpen_edge_rly_by_mot_write(vi_pipe, i,
            OT_EXT_SYSTEM_MANUAL_SHARPEN_EDGERLYBYMOT_DEF);
        ot_ext_system_manual_sharpen_edge_rly_by_luma_write(vi_pipe, i,
            OT_EXT_SYSTEM_MANUAL_SHARPEN_EDGERLYBYLUMA_DEF);
    }

    for (i = 0; i < OT_ISP_SHARPEN_MOT_NUM; i++) {
        ot_ext_system_manual_sharpen_mf_gain_by_mot_write(vi_pipe, i,
            OT_EXT_SYSTEM_MANUAL_SHARPEN_MFGAINBYMOT_DEF);
        ot_ext_system_manual_sharpen_hf_gain_by_mot_write(vi_pipe, i,
            OT_EXT_SYSTEM_MANUAL_SHARPEN_HFGAINBYMOT_DEF);
        ot_ext_system_manual_sharpen_lmf_gain_by_mot_write(vi_pipe, i,
            OT_EXT_SYSTEM_MANUAL_SHARPEN_LMFGAINBYMOT_DEF);
    }

    return TD_SUCCESS;
}

static td_s32 sharpen_ext_regs_initialize(ot_vi_pipe vi_pipe)
{
    td_s32 ret;
    ot_isp_cmos_default *sns_dft = TD_NULL;

    isp_sensor_get_default(vi_pipe, &sns_dft);
    isp_check_pointer_return(sns_dft);

    ot_ext_system_sharpen_mpi_update_en_write(vi_pipe, TD_TRUE);

    /* auto ext_regs initial */
    if (sns_dft->key.bit1_sharpen) {
        isp_check_pointer_return(sns_dft->sharpen);

        ret = sharpen_check_cmos_param(vi_pipe, sns_dft->sharpen);
        if (ret != TD_SUCCESS) {
            return ret;
        }
        ot_ext_system_sharpen_manu_mode_write(vi_pipe, sns_dft->sharpen->op_type);
        ot_ext_system_manual_sharpen_en_write(vi_pipe, sns_dft->sharpen->enable);
        sharpen_ext_regs_cmos_auto_initialize(vi_pipe, sns_dft->sharpen);
        sharpen_ext_regs_cmos_manual_initialize(vi_pipe, sns_dft->sharpen);
    } else {
        ot_ext_system_sharpen_manu_mode_write(vi_pipe, OT_OP_MODE_AUTO);
        ot_ext_system_manual_sharpen_en_write(vi_pipe, TD_TRUE);
        sharpen_ext_regs_default_auto_initialize(vi_pipe);
        sharpen_ext_regs_default_manual_initialize(vi_pipe);
    }

    return TD_SUCCESS;
}

static td_void sharpen_static_shoot_reg_init(isp_sharpen_static_reg_cfg *static_cfg)
{
    td_u8 i;

    static_cfg->dither_mode       = 2;    /* dither mode  2 */
    static_cfg->static_resh       = TD_TRUE;
    static_cfg->gain_thd_sel_d    = 1;
    static_cfg->dir_var_scale     = 2;  /* var_scale 2 */
    static_cfg->dir_rly[0]        = 127;  /* dir rly0   127 */
    static_cfg->dir_rly[1]        = 0;
    static_cfg->max_var_clip_min  = 3;    /* max var clip 3 */
    static_cfg->o_max_chg         = 1000; /* o max chg 1000 */
    static_cfg->u_max_chg         = 1000; /* u max chg 1000 */
    static_cfg->sht_var_sft       = 0;

    for (i = 0; i < ISP_SHARPEN_FREQ_CORING_LENGTH; i++) {
        static_cfg->lmt_mf[i]     = 0;
        static_cfg->lmt_hf[i]     = 0;
    }

    static_cfg->sht_var_wgt1      = 127; /* sht var wgt1 127 */
    static_cfg->sht_var_diff_wgt0 = 127; /* sht var wgt0 127 */
    static_cfg->sht_var_thd0      = 0;
    static_cfg->en_shp8_dir       = 1;

    static_cfg->lf_gain_wgt       = 14;  /* lf gain wgt 14 */
    static_cfg->hf_gain_sft       = 5;   /* hf gain sft  5 */
    static_cfg->mf_gain_sft       = 5;   /* mf gain sft  5 */
    static_cfg->sht_var_sel       = 1;
    static_cfg->sht_var5x5_sft    = 1;
    static_cfg->detail_thd_sel    = 0;
    static_cfg->detail_thd_sft    = 1;

    static_cfg->en_var7x9_calc    = 1;
}

static td_void sharpen_static_chrome_reg_init(isp_sharpen_static_reg_cfg *static_cfg)
{
    static_cfg->chr_r_var_sft    = 7;   /* chr rvar sft  7 */
    static_cfg->chr_r_ori_cb     = 120; /* chr rori cb 120 */
    static_cfg->chr_r_ori_cr     = 220; /* chr rori cr 220 */
    static_cfg->chr_r_sft[0]     = 7;   /* chr red sft0  7 */
    static_cfg->chr_r_sft[1]     = 7;   /* chr red sft1  7 */
    static_cfg->chr_r_sft[2]     = 7;   /* chr red sft2  7 */
    static_cfg->chr_r_sft[3]     = 6;   /* chr red sft3  6 */
    static_cfg->chr_r_thd[0]     = 40;  /* chr red thd0 40 */
    static_cfg->chr_r_thd[1]     = 60;  /* chr red thd1 60 */

    static_cfg->chr_b_var_sft    = 2;   /* chr bvar sft  2 */
    static_cfg->chr_b_ori_cb     = 200; /* chr bori cb 200 */
    static_cfg->chr_b_ori_cr     = 64;  /* chr bori cr 64  */
    static_cfg->chr_b_sft[0]     = 7;   /* chr blu sft0 7  */
    static_cfg->chr_b_sft[1]     = 7;   /* chr blu sft1 7  */
    static_cfg->chr_b_sft[2]     = 7;   /* chr blu sft2 7  */
    static_cfg->chr_b_sft[3]     = 7;   /* chr blu sft3 7  */
    static_cfg->chr_b_thd[0]     = 50;  /* chr bl thd0 50  */
    static_cfg->chr_b_thd[1]     = 100; /* chr bl thd1 100 */

    static_cfg->chr_g_ori_cb     = 90;  /* chr gori cb  90 */
    static_cfg->chr_g_ori_cr     = 110; /* chr gori cr 110 */
    static_cfg->chr_g_sft[0]     = 4;   /* chr gre sft0 4  */
    static_cfg->chr_g_sft[1]     = 7;   /* chr gre sft1 7  */
    static_cfg->chr_g_sft[2]     = 4;   /* chr gre sft2 4  */
    static_cfg->chr_g_sft[3]     = 7;   /* chr gre sft3 7  */
    static_cfg->chr_g_thd[0]     = 20;  /* chr gre thd0 20 */
    static_cfg->chr_g_thd[1]     = 40;  /* chr gre thd1 40 */
}


static td_void sharpen_static_skin_reg_init(isp_sharpen_static_reg_cfg *static_cfg)
{
    static_cfg->skin_src_sel     = 0;   /* skin src sel   0 */
    static_cfg->skin_cnt_thd[0]  = 5;   /* skin cnt thd0  5 */
    static_cfg->skin_edge_thd[0] = 10;  /* skin edg thd0 10 */
    static_cfg->skin_cnt_thd[1]  = 8;   /* skin cnt thd1  8 */
    static_cfg->skin_edge_thd[1] = 30;  /* skin edg thd1 30 */
    static_cfg->skin_edge_sft    = 1;

    static_cfg->skin_cnt_mul = (td_u8)calc_mul_coef(static_cfg->skin_cnt_thd[0], 0,
                                                    static_cfg->skin_cnt_thd[1], 31, 0); /* Range: [0, 31] */
}

/* sharpen hardware regs that will change with MPI and ISO */
static td_void sharpen_mpi_dyna_reg_init(isp_sharpen_mpi_dyna_reg_cfg *mpi_dyna_cfg)
{
    td_u8 i;
    for (i = 0; i < SHRP_GAIN_LUT_SIZE; i++) {
        mpi_dyna_cfg->mf_gain_d[i]  = 200;  /* mf gain d  200 */
        mpi_dyna_cfg->mf_gain_ud[i] = 250;  /* mf gain ud 250 */
        mpi_dyna_cfg->hf_gain_d[i]  = 450;  /* hf gain d  450 */
        mpi_dyna_cfg->hf_gain_ud[i] = 400;  /* hf gain ud 400 */
    }

    mpi_dyna_cfg->osht_amt           = 55; /* over  shoot amt 55  */
    mpi_dyna_cfg->usht_amt           = 55; /* under shoot amt 55  */
    mpi_dyna_cfg->en_sht_ctrl_by_var = 1;   /* sht ctrl by var 1    */
    mpi_dyna_cfg->sht_bld_rt         = 8;   /* shoot blend rt  8    */
    mpi_dyna_cfg->sht_var_thd1       = 5;   /* oshoot var thd1 5    */

    mpi_dyna_cfg->en_chr_ctrl        = 1;   /* en chrome ctrl   1   */
    mpi_dyna_cfg->chr_r_gain         = 24;   /* chrome red gain 24   */
    mpi_dyna_cfg->chr_g_gain         = 32;  /* chrome gre gain  32  */
    mpi_dyna_cfg->chr_gmf_gain       = 32;  /* chrome gmf gain  32  */
    mpi_dyna_cfg->chr_b_gain         = 24;  /* chrome blue gain 24  */
    mpi_dyna_cfg->en_skin_ctrl       = 1;   /* en skin ctrl     1   */
    mpi_dyna_cfg->skin_edge_wgt[1]   = 6;  /* skin edge wgt1    6  */
    mpi_dyna_cfg->skin_edge_wgt[0]   = 12;  /* skin edge wgt0   12  */

    mpi_dyna_cfg->en_luma_ctrl       = 1;   /* en luma   ctrl   0   */
    mpi_dyna_cfg->en_detail_ctrl     = 0;   /* en detail ctrl   0   */
    mpi_dyna_cfg->detail_osht_amt    = 55; /* detail osht amt  55 */
    mpi_dyna_cfg->detail_usht_amt    = 55; /* detail usht amt  127 */
    mpi_dyna_cfg->dir_diff_sft       = 2;  /* dir diff asft    10  */
    mpi_dyna_cfg->dir_rt[0]          = 6;   /* direction rt0    6   */
    mpi_dyna_cfg->dir_rt[1]          = 18;  /* direction rt1    18  */
    mpi_dyna_cfg->skin_max_u         = 128; /* skin max u       128 */
    mpi_dyna_cfg->skin_min_u         = 100;  /* skin min u       100  */
    mpi_dyna_cfg->skin_max_v         = 150; /* skin max v       150 */
    mpi_dyna_cfg->skin_min_v         = 135; /* skin min v       135 */
    mpi_dyna_cfg->o_max_gain         = 40; /* oshoot max gain  40 */
    mpi_dyna_cfg->u_max_gain         = 40; /* ushoot max gain  40 */
    mpi_dyna_cfg->detail_osht_thr[0] = 160;  /* detail osht thr0 160  */
    mpi_dyna_cfg->detail_osht_thr[1] = 176;  /* detail osht thr1 176  */
    mpi_dyna_cfg->detail_usht_thr[0] = 160;  /* detail usht thr0 160  */
    mpi_dyna_cfg->detail_usht_thr[1] = 176;  /* detail usht thr1 176  */

    for (i = 0; i < OT_ISP_SHARPEN_LUMA_NUM; i++) {
        mpi_dyna_cfg->luma_wgt[i]  = 31;    /* luma weight      31  */
    }

    mpi_dyna_cfg->update_index     = 1;
    mpi_dyna_cfg->buf_id           = 0;
    mpi_dyna_cfg->resh             = TD_TRUE;
    mpi_dyna_cfg->pre_reg_new_en   = TD_FALSE;

    mpi_dyna_cfg->exluma_thd       = 0;
    mpi_dyna_cfg->exluma_out_thd   = 0;
    mpi_dyna_cfg->hard_luma_thd    = 0;

    mpi_dyna_cfg->en_mot_ctrl       = 0;
    mpi_dyna_cfg->en_std_adj_by_mot = 0;
    mpi_dyna_cfg->en_std_adj_by_y   = 0;

    mpi_dyna_cfg->var5_low_thd      = 0;
    mpi_dyna_cfg->var5_mid_thd      = 24;   /* mid thd 24 */
    mpi_dyna_cfg->var5_high_thd     = 49;   /* high thd 49 */
    mpi_dyna_cfg->var7x9_low_thd    = 0; /* low thd 6 */
    mpi_dyna_cfg->var7x9_high_thd   = 100; /* high thd 100 */

    mpi_dyna_cfg->edge_osht_amt     = 20;   /* edge_osht_amt 20 */
    mpi_dyna_cfg->edge_usht_amt     = 120;  /* edge_usht_amt 120 */

    for (i = 0; i < OT_ISP_SHARPEN_RLYWGT_NUM; i++) {
        mpi_dyna_cfg->rly_wgt[i] = 32; /* 32 */
    }

    for (i = 0; i < OT_ISP_SHARPEN_STDGAIN_NUM; i++) {
        mpi_dyna_cfg->std_gain_by_mot_lut[i] = 16;  /* std_gain_by_mot_lut 16 */
        mpi_dyna_cfg->std_gain_by_y_lut[i] = 16;    /* std_gain_by_y_lut 16 */
    }

    for (i = 0; i < OT_ISP_SHARPEN_STDGAIN_NUM; i++) {
        mpi_dyna_cfg->std_offset_by_mot_lut[i] = 0;
        mpi_dyna_cfg->std_offset_by_y_lut[i] = 0;
    }

    for (i = 0; i < OT_ISP_SHARPEN_MOT_NUM; i++) {
        mpi_dyna_cfg->mf_mot_dec[i] = 32;  /* mf_mot_dec 32 */
        mpi_dyna_cfg->hf_mot_dec[i] = 32;  /* hf_mot_dec 32 */
        mpi_dyna_cfg->lmf_mot_gain[i] = 16; /* lmf_mot_gain 16 */
    }
}

/* sharpen hardware regs that will change only with ISO */
static td_void sharpen_default_dyna_reg_init(isp_sharpen_default_dyna_reg_cfg *def_dyna_cfg)
{
    def_dyna_cfg->resh                = TD_TRUE;
    def_dyna_cfg->gain_thd_sft_d      = 0;   /* mf thd sft d       0  */
    def_dyna_cfg->gain_thd_sel_ud     = 1;   /* mf thd sel ud      1  */
    def_dyna_cfg->gain_thd_sft_ud     = 0;   /* mf thd sft ud      0  */
    def_dyna_cfg->dir_var_sft         = 10;  /* dir var sft        10 */
    def_dyna_cfg->sel_pix_wgt         = 31;  /* sel pix wgt        31 */
    def_dyna_cfg->sht_var_diff_thd[0] = 41;  /* sht var diff thd0  41 */
    def_dyna_cfg->sht_var_wgt0        = 25;  /* sht var wgt        25 */
    def_dyna_cfg->sht_var_diff_thd[1] = 56;  /* sht var diff thd1  56 */
    def_dyna_cfg->sht_var_diff_wgt1   = 5;   /* sht var diff wgt1  5  */
    def_dyna_cfg->rmf_gain_scale      = 6;  /* rmf gain scale     6 */
    def_dyna_cfg->bmf_gain_scale      = 4;  /* bmf gain scale     4 */
    def_dyna_cfg->dir_var_thd         = 0;   /* dir_var_thd        0 */
    def_dyna_cfg->mot_thd             = 9;   /* mot thd            9 */
    def_dyna_cfg->std_comb_mode       = 2;   /* std comb mode      2 */
    def_dyna_cfg->std_comb_alpha      = 4;   /* std comb alpha     4 */
    def_dyna_cfg->mot_interp_mode     = 1;   /* mot interp mode    1 */
}

static td_void sharpen_dyna_reg_init(isp_sharpen_reg_cfg *sharpen_reg_cfg)
{
    isp_sharpen_default_dyna_reg_cfg *def_dyna_cfg = TD_NULL;
    isp_sharpen_mpi_dyna_reg_cfg     *mpi_dyna_cfg = TD_NULL;
    isp_sharpen_static_reg_cfg       *static_cfg = TD_NULL;

    def_dyna_cfg = &(sharpen_reg_cfg->dyna_reg_cfg.default_dyna_reg_cfg);
    mpi_dyna_cfg     = &sharpen_reg_cfg->dyna_reg_cfg.mpi_dyna_reg_cfg;
    static_cfg      = &sharpen_reg_cfg->static_reg_cfg;

    sharpen_default_dyna_reg_init(def_dyna_cfg);
    sharpen_mpi_dyna_reg_init(mpi_dyna_cfg);

    /* calc all mul_coef */
    /* mpi */
    mpi_dyna_cfg->sht_var_mul  = (td_u16)calc_mul_coef(static_cfg->sht_var_thd0,  def_dyna_cfg->sht_var_wgt0,
                                                       mpi_dyna_cfg->sht_var_thd1, static_cfg->sht_var_wgt1,
                                                       SHRP_SHT_VAR_MUL_PRECS);

    mpi_dyna_cfg->chr_r_mul     = (td_s16)calc_mul_coef(static_cfg->chr_r_thd[0], mpi_dyna_cfg->chr_r_gain,
                                                        static_cfg->chr_r_thd[1], 0x20,
                                                        SHRP_CHR_MUL_SFT);
    mpi_dyna_cfg->chr_g_mul     = (td_s16)calc_mul_coef(static_cfg->chr_g_thd[0], mpi_dyna_cfg->chr_g_gain,
                                                        static_cfg->chr_g_thd[1], 0x20,
                                                        SHRP_CHR_MUL_SFT);
    mpi_dyna_cfg->chr_gmf_mul   = (td_s16)calc_mul_coef(static_cfg->chr_g_thd[0], mpi_dyna_cfg->chr_gmf_gain,
                                                        static_cfg->chr_g_thd[1], 0x20,
                                                        SHRP_CHR_MUL_SFT);
    mpi_dyna_cfg->chr_b_mul     = (td_s16)calc_mul_coef(static_cfg->chr_b_thd[0], mpi_dyna_cfg->chr_b_gain,
                                                        static_cfg->chr_b_thd[1], 0x20,
                                                        SHRP_CHR_MUL_SFT);

    mpi_dyna_cfg->skin_edge_mul = (td_s16)calc_mul_coef(static_cfg->skin_edge_thd[0], mpi_dyna_cfg->skin_edge_wgt[0],
                                                        static_cfg->skin_edge_thd[1], mpi_dyna_cfg->skin_edge_wgt[1],
                                                        SHRP_SKIN_EDGE_MUL_PRECS);

    mpi_dyna_cfg->detail_osht_mul =
        (td_s16)calc_mul_coef(mpi_dyna_cfg->detail_osht_thr[0], mpi_dyna_cfg->detail_osht_amt,
                              mpi_dyna_cfg->detail_osht_thr[1], mpi_dyna_cfg->osht_amt, SHRP_DETAIL_SHT_MUL_PRECS);

    mpi_dyna_cfg->detail_usht_mul =
        (td_s16)calc_mul_coef(mpi_dyna_cfg->detail_usht_thr[0], mpi_dyna_cfg->detail_usht_amt,
                              mpi_dyna_cfg->detail_usht_thr[1], mpi_dyna_cfg->usht_amt, SHRP_DETAIL_SHT_MUL_PRECS);
    if (mpi_dyna_cfg->exluma_thd + mpi_dyna_cfg->exluma_out_thd == 0) {
        mpi_dyna_cfg->exluma_mul = 0;
    } else {
        mpi_dyna_cfg->exluma_mul = (td_u8)((mpi_dyna_cfg->exluma_thd << SHRP_EXLUMA_MUL_PRECS) /
            (mpi_dyna_cfg->exluma_thd + mpi_dyna_cfg->exluma_out_thd));
    }
    /* defalut */
    def_dyna_cfg->sht_var_diff_mul =
        (td_s16)calc_mul_coef(def_dyna_cfg->sht_var_diff_thd[0], static_cfg->sht_var_diff_wgt0,
                              def_dyna_cfg->sht_var_diff_thd[1], def_dyna_cfg->sht_var_diff_wgt1,
                              SHRP_SHT_VAR_MUL_PRECS);
}

static td_void sharpen_regs_initialize(ot_vi_pipe vi_pipe, isp_reg_cfg *reg_cfg)
{
    td_u32 i;
    ot_unused(vi_pipe);
    for (i = 0; i < reg_cfg->cfg_num; i++) {
        reg_cfg->alg_reg_cfg[i].sharpen_reg_cfg.enable     = TD_TRUE;
        reg_cfg->alg_reg_cfg[i].sharpen_reg_cfg.lut2_stt_en = TD_TRUE;

        sharpen_static_shoot_reg_init(&(reg_cfg->alg_reg_cfg[i].sharpen_reg_cfg.static_reg_cfg));
        sharpen_static_chrome_reg_init(&(reg_cfg->alg_reg_cfg[i].sharpen_reg_cfg.static_reg_cfg));
        sharpen_static_skin_reg_init(&(reg_cfg->alg_reg_cfg[i].sharpen_reg_cfg.static_reg_cfg));

        sharpen_dyna_reg_init(&(reg_cfg->alg_reg_cfg[i].sharpen_reg_cfg));
    }

    reg_cfg->cfg_key.bit1_sharpen_cfg = 1;
}

static td_void sharpen_read_auto_extregs(ot_vi_pipe vi_pipe)
{
    td_u8 i;
    td_u16 j, index;
    isp_sharpen_ctx *sharpen = TD_NULL;
    sharpen_get_ctx(vi_pipe, sharpen);

    for (i = 0; i < SHRP_ISO_NUM; i++) {
        for (j = 0; j < OT_ISP_SHARPEN_GAIN_NUM; j++) {
            index = i + j * SHRP_ISO_NUM;
            sharpen->auto_attr.texture_strength[j][i] = ot_ext_system_sharpen_texture_str_read(vi_pipe, index);
            sharpen->auto_attr.edge_strength[j][i]    = ot_ext_system_sharpen_edge_str_read(vi_pipe, index);
        }
        for (j = 0; j < OT_ISP_SHARPEN_LUMA_NUM; j++) {
            index = i + j * SHRP_ISO_NUM;
            sharpen->auto_attr.luma_wgt[j][i]     = (td_u8)ot_ext_system_sharpen_luma_wgt_read(vi_pipe, index);
        }
        sharpen->auto_attr.texture_freq[i]   = ot_ext_system_sharpen_texture_freq_read(vi_pipe, i);
        sharpen->auto_attr.edge_freq[i]      = ot_ext_system_sharpen_edge_freq_read(vi_pipe, i);
        sharpen->auto_attr.over_shoot[i]     = ot_ext_system_sharpen_over_shoot_read(vi_pipe, i);
        sharpen->auto_attr.under_shoot[i]    = ot_ext_system_sharpen_under_shoot_read(vi_pipe, i);
        sharpen->auto_attr.shoot_sup_strength[i]  = ot_ext_system_sharpen_shoot_sup_str_read(vi_pipe, i);
        sharpen->auto_attr.detail_ctrl[i]    = ot_ext_system_sharpen_detailctrl_read(vi_pipe, i);
        sharpen->auto_attr.edge_filt_strength[i]  = ot_ext_system_sharpen_edge_filt_str_read(vi_pipe, i);
        sharpen->auto_attr.edge_filt_max_cap[i] = ot_ext_system_sharpen_edge_filt_max_cap_read(vi_pipe, i);
        sharpen->auto_attr.r_gain[i]          = ot_ext_system_sharpen_r_gain_read(vi_pipe, i);
        sharpen->auto_attr.g_gain[i]          = ot_ext_system_sharpen_g_gain_read(vi_pipe, i);
        sharpen->auto_attr.b_gain[i]          = ot_ext_system_sharpen_b_gain_read(vi_pipe, i);
        sharpen->auto_attr.skin_gain[i]       = ot_ext_system_sharpen_skin_gain_read(vi_pipe, i);
        sharpen->auto_attr.shoot_sup_adj[i]   = ot_ext_system_sharpen_shoot_sup_adj_read(vi_pipe, i);
        sharpen->auto_attr.max_sharp_gain[i]  = ot_ext_system_sharpen_max_sharp_gain_read(vi_pipe, i);
        sharpen->auto_attr.detail_ctrl_threshold[i]  = ot_ext_system_sharpen_detailctrl_thr_read(vi_pipe, i);

        sharpen->auto_attr.shoot_threshold_attr.shoot_inner_threshold[i] =
                ot_ext_system_sharpen_shoot_inner_threshold_read(vi_pipe, i);
        sharpen->auto_attr.shoot_threshold_attr.shoot_outer_threshold[i] =
                ot_ext_system_sharpen_shoot_outer_threshold_read(vi_pipe, i);
        sharpen->auto_attr.shoot_threshold_attr.shoot_protect_threshold[i] =
                ot_ext_system_sharpen_shoot_protect_threshold_read(vi_pipe, i);

        sharpen->auto_attr.edge_rly_attr.edge_rly_fine_threshold[i] =
                ot_ext_system_sharpen_edge_rly_fine_threshold_read(vi_pipe, i);
        sharpen->auto_attr.edge_rly_attr.edge_rly_coarse_threshold[i] =
                ot_ext_system_sharpen_edge_rly_coarse_threshold_read(vi_pipe, i);
        sharpen->auto_attr.edge_rly_attr.edge_overshoot[i] =
            (td_u8)ot_ext_system_sharpen_edge_overshoot_read(vi_pipe, i);
        sharpen->auto_attr.edge_rly_attr.edge_undershoot[i] =
            (td_u8)ot_ext_system_sharpen_edge_undershoot_read(vi_pipe, i);

        for (j = 0; j < OT_ISP_SHARPEN_RLYWGT_NUM; j++) {
            index = i + j * SHRP_ISO_NUM;
            sharpen->auto_attr.edge_rly_attr.edge_gain_by_rly[j][i] =
                (td_u8)ot_ext_system_sharpen_edge_gain_by_rly_read(vi_pipe, (td_u8)index);
        }

        for (j = 0; j < OT_ISP_SHARPEN_STDGAIN_NUM; j++) {
            index = i + j * SHRP_ISO_NUM;
            sharpen->auto_attr.edge_rly_attr.edge_rly_by_mot[j][i] =
                (td_u8)ot_ext_system_sharpen_edge_rly_by_mot_read(vi_pipe, (td_u8)index);
            sharpen->auto_attr.edge_rly_attr.edge_rly_by_luma[j][i] =
                (td_u8)ot_ext_system_sharpen_edge_rly_by_luma_read(vi_pipe, (td_u8)index);
        }

        for (j = 0; j < OT_ISP_SHARPEN_MOT_NUM; j++) {
            index = i + j * SHRP_ISO_NUM;
            sharpen->auto_attr.gain_by_mot_attr.mf_gain_by_mot[j][i] =
                (td_u8)ot_ext_system_sharpen_mf_gain_by_mot_read(vi_pipe, (td_u8)index);
            sharpen->auto_attr.gain_by_mot_attr.hf_gain_by_mot[j][i] =
                (td_u8)ot_ext_system_sharpen_hf_gain_by_mot_read(vi_pipe, (td_u8)index);
            sharpen->auto_attr.gain_by_mot_attr.lmf_gain_by_mot[j][i] =
                (td_u8)ot_ext_system_sharpen_lmf_gain_by_mot_read(vi_pipe, (td_u8)index);
        }
    }
}

static td_void sharpen_read_manual_extregs(ot_vi_pipe vi_pipe)
{
    td_u8 j;
    isp_sharpen_ctx *sharpen = TD_NULL;
    sharpen_get_ctx(vi_pipe, sharpen);

    for (j = 0; j < OT_ISP_SHARPEN_GAIN_NUM; j++) {
        sharpen->texture_str[j] = ot_ext_system_manual_sharpen_texture_str_read(vi_pipe, j);
        sharpen->edge_str[j]    = ot_ext_system_manual_sharpen_edge_str_read(vi_pipe, j);
    }
    for (j = 0; j < OT_ISP_SHARPEN_LUMA_NUM; j++) {
        sharpen->luma_wgt[j]     = ot_ext_system_manual_sharpen_luma_wgt_read(vi_pipe, j);
    }
    sharpen->texture_freq    = ot_ext_system_manual_sharpen_texture_freq_read(vi_pipe);
    sharpen->edge_freq       = ot_ext_system_manual_sharpen_edge_freq_read(vi_pipe);
    sharpen->over_shoot      = ot_ext_system_manual_sharpen_over_shoot_read(vi_pipe);
    sharpen->under_shoot     = ot_ext_system_manual_sharpen_under_shoot_read(vi_pipe);
    sharpen->shoot_sup_str   = ot_ext_system_manual_sharpen_shoot_sup_str_read(vi_pipe);
    sharpen->detail_ctrl     = ot_ext_system_manual_sharpen_detailctrl_read(vi_pipe);
    sharpen->edge_filt_str     = ot_ext_system_manual_sharpen_edge_filt_str_read(vi_pipe);
    sharpen->edge_filt_max_cap = ot_ext_system_manual_sharpen_edge_filt_max_cap_read(vi_pipe);
    sharpen->r_gain            = ot_ext_system_manual_sharpen_r_gain_read(vi_pipe);
    sharpen->g_gain            = ot_ext_system_manual_sharpen_g_gain_read(vi_pipe);
    sharpen->b_gain            = ot_ext_system_manual_sharpen_b_gain_read(vi_pipe);
    sharpen->skin_gain         = ot_ext_system_manual_sharpen_skin_gain_read(vi_pipe);
    sharpen->shoot_sup_adj     = ot_ext_system_manual_sharpen_shoot_sup_adj_read(vi_pipe);
    sharpen->detail_ctrl_thr   = ot_ext_system_manual_sharpen_detailctrl_thr_read(vi_pipe);
    sharpen->max_sharp_gain    = ot_ext_system_manual_sharpen_max_sharp_gain_read(vi_pipe);
    sharpen->shoot_inner_threshold    = ot_ext_system_manual_sharpen_shoot_inner_threshold_read(vi_pipe);
    sharpen->shoot_outer_threshold    = ot_ext_system_manual_sharpen_shoot_outer_threshold_read(vi_pipe);
    sharpen->shoot_protect_threshold  = ot_ext_system_manual_sharpen_shoot_protect_threshold_read(vi_pipe);

    sharpen->edge_rly_fine_threshold      = ot_ext_system_manual_sharpen_edge_rly_fine_threshold_read(vi_pipe);
    sharpen->edge_rly_coarse_threshold    = ot_ext_system_manual_sharpen_edge_rly_coarse_threshold_read(vi_pipe);
    sharpen->edge_overshoot    = (td_u8)ot_ext_system_manual_sharpen_edge_overshoot_read(vi_pipe);
    sharpen->edge_undershoot   = (td_u8)ot_ext_system_manual_sharpen_edge_undershoot_read(vi_pipe);

    for (j = 0; j < OT_ISP_SHARPEN_RLYWGT_NUM; j++) {
        sharpen->edge_gain_by_rly[j] = (td_u8)ot_ext_system_manual_sharpen_edge_gain_by_rly_read(vi_pipe, j);
    }

    for (j = 0; j < OT_ISP_SHARPEN_STDGAIN_NUM; j++) {
        sharpen->edge_rly_by_mot[j]  = (td_u8)ot_ext_system_manual_sharpen_edge_rly_by_mot_read(vi_pipe, j);
        sharpen->edge_rly_by_luma[j] = (td_u8)ot_ext_system_manual_sharpen_edge_rly_by_luma_read(vi_pipe, j);
    }

    for (j = 0; j < OT_ISP_SHARPEN_MOT_NUM; j++) {
        sharpen->mf_gain_by_mot[j]  = (td_u8)ot_ext_system_manual_sharpen_mf_gain_by_mot_read(vi_pipe, j);
        sharpen->hf_gain_by_mot[j]  = (td_u8)ot_ext_system_manual_sharpen_hf_gain_by_mot_read(vi_pipe, j);
        sharpen->lmf_gain_by_mot[j] = (td_u8)ot_ext_system_manual_sharpen_lmf_gain_by_mot_read(vi_pipe, j);
    }
}

static td_void sharpen_read_extregs(ot_vi_pipe vi_pipe)
{
    isp_sharpen_ctx *sharpen = TD_NULL;
    sharpen_get_ctx(vi_pipe, sharpen);

    sharpen->skin_umax   = ot_ext_system_manual_sharpen_skin_umax_read(vi_pipe);
    sharpen->skin_umin   = ot_ext_system_manual_sharpen_skin_umin_read(vi_pipe);
    sharpen->skin_vmax   = ot_ext_system_manual_sharpen_skin_vmax_read(vi_pipe);
    sharpen->skin_vmin   = ot_ext_system_manual_sharpen_skin_vmin_read(vi_pipe);
    sharpen->sharpen_mpi_update_en = ot_ext_system_sharpen_mpi_update_en_read(vi_pipe);
    if (sharpen->sharpen_mpi_update_en != TD_TRUE) {
        return;
    }

    ot_ext_system_sharpen_mpi_update_en_write(vi_pipe, TD_FALSE);

    sharpen->manual_sharpen_yuv_enabled = ot_ext_system_sharpen_manu_mode_read(vi_pipe);

    if (sharpen->manual_sharpen_yuv_enabled == OT_OP_MODE_MANUAL) {
        sharpen_read_manual_extregs(vi_pipe);
    } else {
        sharpen_read_auto_extregs(vi_pipe);
    }
}

static td_void sharpen_read_pro_mode(ot_vi_pipe vi_pipe)
{
    ot_unused(vi_pipe);
}

static td_void isp_sharpen_ctx_mpi_init(isp_sharpen_ctx *sharpen)
{
    td_u32 i, j;

    for (i = 0; i < OT_ISP_SHARPEN_GAIN_NUM; i++) {
        sharpen->texture_str[i] = OT_EXT_SYSTEM_MANUAL_SHARPEN_TEXTURESTR_DEF;
        sharpen->edge_str[i]    = OT_EXT_SYSTEM_MANUAL_SHARPEN_EDGESTR_DEF;
    }

    for (i = 0; i < OT_ISP_SHARPEN_LUMA_NUM; i++) {
        sharpen->luma_wgt[i]    = OT_EXT_SYSTEM_MANUAL_SHARPEN_LUMAWGT_DEF;
    }

    sharpen->texture_freq       = OT_EXT_SYSTEM_MANUAL_SHARPEN_TEXTUREFREQ_DEF;
    sharpen->edge_freq          = OT_EXT_SYSTEM_MANUAL_SHARPEN_EDGEFREQ_DEF;
    sharpen->over_shoot         = OT_EXT_SYSTEM_MANUAL_SHARPEN_OVERSHOOT_DEF;
    sharpen->under_shoot        = OT_EXT_SYSTEM_MANUAL_SHARPEN_UNDERSHOOT_DEF;
    sharpen->shoot_sup_str      = OT_EXT_SYSTEM_MANUAL_SHARPEN_SHOOTSUPSTR_DEF;
    sharpen->detail_ctrl        = OT_EXT_SYSTEM_MANUAL_SHARPEN_DETAILCTRL_DEF;
    sharpen->edge_filt_str      = OT_EXT_SYSTEM_MANUAL_SHARPEN_EDGEFILTSTR_DEF;
    sharpen->edge_filt_max_cap  = OT_EXT_SYSTEM_MANUAL_SHARPEN_EDGEFILTMAXCAP_DEF;
    sharpen->r_gain             = OT_EXT_SYSTEM_MANUAL_SHARPEN_RGAIN_DEF;
    sharpen->g_gain             = OT_EXT_SYSTEM_MANUAL_SHARPEN_GGAIN_DEF;
    sharpen->b_gain             = OT_EXT_SYSTEM_MANUAL_SHARPEN_BGAIN_DEF;
    sharpen->skin_gain          = OT_EXT_SYSTEM_MANUAL_SHARPEN_SKINGAIN_DEF;
    sharpen->shoot_sup_adj      = OT_EXT_SYSTEM_MANUAL_SHARPEN_SHOOTSUPADJ_DEF;
    sharpen->max_sharp_gain     = OT_EXT_SYSTEM_MANUAL_SHARPEN_MAXSHARPGAIN_DEF;
    sharpen->detail_ctrl_thr    = OT_EXT_SYSTEM_MANUAL_SHARPEN_DETAILCTRLTHR_DEF;

    sharpen->shoot_inner_threshold     = OT_EXT_SYSTEM_MANUAL_SHARPEN_SHOOTINNERTHRESHOLD_DEF;
    sharpen->shoot_outer_threshold     = OT_EXT_SYSTEM_MANUAL_SHARPEN_SHOOTOUTERTHRESHOLD_DEF;
    sharpen->shoot_protect_threshold   = OT_EXT_SYSTEM_MANUAL_SHARPEN_SHOOTPROTECTTHRESHOLD_DEF;

    sharpen->edge_rly_fine_threshold   = OT_EXT_SYSTEM_MANUAL_SHARPEN_EDGERLYFINETHRESHOLD_DEF;
    sharpen->edge_rly_coarse_threshold = OT_EXT_SYSTEM_MANUAL_SHARPEN_EDGERLYCOARSETHRESHOLD_DEF;
    sharpen->edge_overshoot            = OT_EXT_SYSTEM_MANUAL_SHARPEN_EDGEOVERSHOOT_DEF;
    sharpen->edge_undershoot           = OT_EXT_SYSTEM_MANUAL_SHARPEN_EDGEUNDERSHOOT_DEF;

    for (i = 0; i < OT_ISP_SHARPEN_RLYWGT_NUM; i++) {
        sharpen->edge_gain_by_rly[i]   = OT_EXT_SYSTEM_MANUAL_SHARPEN_EDGEGAINBYRLY_DEF;
    }

    for (i = 0; i < OT_ISP_SHARPEN_STDGAIN_NUM; i++) {
        sharpen->edge_rly_by_mot[i]    = OT_EXT_SYSTEM_MANUAL_SHARPEN_EDGERLYBYMOT_DEF;
        sharpen->edge_rly_by_luma[i]   = OT_EXT_SYSTEM_MANUAL_SHARPEN_EDGERLYBYLUMA_DEF;
    }

    for (i = 0; i < OT_ISP_SHARPEN_MOT_NUM; i++) {
        sharpen->mf_gain_by_mot[i]     = OT_EXT_SYSTEM_MANUAL_SHARPEN_MFGAINBYMOT_DEF;
        sharpen->hf_gain_by_mot[i]     = OT_EXT_SYSTEM_MANUAL_SHARPEN_HFGAINBYMOT_DEF;
        sharpen->lmf_gain_by_mot[i]    = OT_EXT_SYSTEM_MANUAL_SHARPEN_LMFGAINBYMOT_DEF;
    }

    for (i = 0; i < SHRP_ISO_NUM; i++) {
        for (j = 0; j < OT_ISP_SHARPEN_GAIN_NUM; j++) {
            sharpen->auto_attr.texture_strength[j][i] = OT_EXT_SYSTEM_MANUAL_SHARPEN_TEXTURESTR_DEF;
            sharpen->auto_attr.edge_strength[j][i]    = OT_EXT_SYSTEM_MANUAL_SHARPEN_EDGESTR_DEF;
        }
        for (j = 0; j < OT_ISP_SHARPEN_LUMA_NUM; j++) {
            sharpen->auto_attr.luma_wgt[j][i]    = OT_EXT_SYSTEM_MANUAL_SHARPEN_LUMAWGT_DEF;
        }
        sharpen->auto_attr.texture_freq[i]       = OT_EXT_SYSTEM_MANUAL_SHARPEN_TEXTUREFREQ_DEF;
        sharpen->auto_attr.edge_freq[i]          = OT_EXT_SYSTEM_MANUAL_SHARPEN_EDGEFREQ_DEF;
        sharpen->auto_attr.over_shoot[i]         = OT_EXT_SYSTEM_MANUAL_SHARPEN_OVERSHOOT_DEF;
        sharpen->auto_attr.under_shoot[i]        = OT_EXT_SYSTEM_MANUAL_SHARPEN_UNDERSHOOT_DEF;
        sharpen->auto_attr.shoot_sup_strength[i]      = OT_EXT_SYSTEM_MANUAL_SHARPEN_SHOOTSUPSTR_DEF;
        sharpen->auto_attr.detail_ctrl[i]        = OT_EXT_SYSTEM_MANUAL_SHARPEN_DETAILCTRL_DEF;
        sharpen->auto_attr.edge_filt_strength[i]      = OT_EXT_SYSTEM_MANUAL_SHARPEN_EDGEFILTSTR_DEF;
        sharpen->auto_attr.edge_filt_max_cap[i]  = OT_EXT_SYSTEM_MANUAL_SHARPEN_EDGEFILTMAXCAP_DEF;
        sharpen->auto_attr.r_gain[i]             = OT_EXT_SYSTEM_MANUAL_SHARPEN_RGAIN_DEF;
        sharpen->auto_attr.g_gain[i]             = OT_EXT_SYSTEM_MANUAL_SHARPEN_GGAIN_DEF;
        sharpen->auto_attr.b_gain[i]             = OT_EXT_SYSTEM_MANUAL_SHARPEN_BGAIN_DEF;
        sharpen->auto_attr.skin_gain[i]          = OT_EXT_SYSTEM_MANUAL_SHARPEN_SKINGAIN_DEF;
        sharpen->auto_attr.shoot_sup_adj[i]      = OT_EXT_SYSTEM_MANUAL_SHARPEN_SHOOTSUPADJ_DEF;
        sharpen->auto_attr.max_sharp_gain[i]     = OT_EXT_SYSTEM_MANUAL_SHARPEN_MAXSHARPGAIN_DEF;
        sharpen->auto_attr.detail_ctrl_threshold[i]    = OT_EXT_SYSTEM_MANUAL_SHARPEN_DETAILCTRLTHR_DEF;

        sharpen->auto_attr.shoot_threshold_attr.shoot_inner_threshold[i] =
                OT_EXT_SYSTEM_MANUAL_SHARPEN_SHOOTINNERTHRESHOLD_DEF;
        sharpen->auto_attr.shoot_threshold_attr.shoot_outer_threshold[i] =
                OT_EXT_SYSTEM_MANUAL_SHARPEN_SHOOTOUTERTHRESHOLD_DEF;
        sharpen->auto_attr.shoot_threshold_attr.shoot_protect_threshold[i] =
                OT_EXT_SYSTEM_MANUAL_SHARPEN_SHOOTPROTECTTHRESHOLD_DEF;

        sharpen->auto_attr.edge_rly_attr.edge_rly_fine_threshold[i] =
                OT_EXT_SYSTEM_MANUAL_SHARPEN_EDGERLYFINETHRESHOLD_DEF;
        sharpen->auto_attr.edge_rly_attr.edge_rly_coarse_threshold[i] =
                OT_EXT_SYSTEM_MANUAL_SHARPEN_EDGERLYCOARSETHRESHOLD_DEF;
        sharpen->auto_attr.edge_rly_attr.edge_overshoot[i] = OT_EXT_SYSTEM_MANUAL_SHARPEN_EDGEOVERSHOOT_DEF;
        sharpen->auto_attr.edge_rly_attr.edge_undershoot[i] = OT_EXT_SYSTEM_MANUAL_SHARPEN_EDGEUNDERSHOOT_DEF;

        for (j = 0; j < OT_ISP_SHARPEN_RLYWGT_NUM; j++) {
            sharpen->auto_attr.edge_rly_attr.edge_gain_by_rly[j][i]   = OT_EXT_SYSTEM_MANUAL_SHARPEN_EDGEGAINBYRLY_DEF;
        }

        for (j = 0; j < OT_ISP_SHARPEN_STDGAIN_NUM; j++) {
            sharpen->auto_attr.edge_rly_attr.edge_rly_by_mot[j][i]    = OT_EXT_SYSTEM_MANUAL_SHARPEN_EDGERLYBYMOT_DEF;
            sharpen->auto_attr.edge_rly_attr.edge_rly_by_luma[j][i]   = OT_EXT_SYSTEM_MANUAL_SHARPEN_EDGERLYBYLUMA_DEF;
        }

        for (j = 0; j < OT_ISP_SHARPEN_MOT_NUM; j++) {
            sharpen->auto_attr.gain_by_mot_attr.mf_gain_by_mot[j][i]  = OT_EXT_SYSTEM_MANUAL_SHARPEN_MFGAINBYMOT_DEF;
            sharpen->auto_attr.gain_by_mot_attr.hf_gain_by_mot[j][i]  = OT_EXT_SYSTEM_MANUAL_SHARPEN_HFGAINBYMOT_DEF;
            sharpen->auto_attr.gain_by_mot_attr.lmf_gain_by_mot[j][i] = OT_EXT_SYSTEM_MANUAL_SHARPEN_LMFGAINBYMOT_DEF;
        }
    }
}

static td_void isp_sharpen_ctx_init(isp_sharpen_ctx *sharpen)
{
    sharpen->sharpen_en                 = 1;
    sharpen->sharpen_mpi_update_en      = 1;
    sharpen->iso_last                   = 0;
    sharpen->manual_sharpen_yuv_enabled = 1;

    sharpen->gain_thd_sft_d    = 0;
    sharpen->dir_var_sft       = 10;  /* dir var sft        10 */
    sharpen->sel_pix_wgt       = 31;  /* sel pix wgt        31 */
    sharpen->rmf_gain_scale    = 2;   /* rmf gain scale      2 */
    sharpen->bmf_gain_scale    = 4;   /* bmf gain scale      4 */
    sharpen->gain_thd_sel_ud   = 2;   /* mf thd sel ud       2 */
    sharpen->gain_thd_sft_ud   = 0;   /* mf thd sft ud       0 */

    sharpen->sht_var_wgt0      = 10;  /* sht var wgt0       10 */
    sharpen->sht_var_diff_thd0 = 20;  /* sht var diff thd0  20 */
    sharpen->sht_var_diff_thd1 = 35;  /* sht var diff thd1  35 */
    sharpen->sht_var_diff_wgt1 = 27;  /* sht var diff wgt1  27 */

    sharpen->skin_umax         = OT_EXT_SYSTEM_MANUAL_SHARPEN_SKINUMAX_DEF;
    sharpen->skin_umin         = OT_EXT_SYSTEM_MANUAL_SHARPEN_SKINUMIN_DEF;
    sharpen->skin_vmin         = OT_EXT_SYSTEM_MANUAL_SHARPEN_SKINVMIN_DEF;
    sharpen->skin_vmax         = OT_EXT_SYSTEM_MANUAL_SHARPEN_SKINVMAX_DEF;

    sharpen->dir_var_thd       = 0;  /* 0 */
    sharpen->mot_thd           = 9;  /* 9 */
    sharpen->std_comb_mode     = 2;  /* 2 */
    sharpen->std_comb_alpha    = 4;  /* 4 */
    sharpen->mot_interp_mode   = 0;  /* 0 */

    isp_sharpen_ctx_mpi_init(sharpen);
}

static td_s32 isp_sharpen_init(ot_vi_pipe vi_pipe, td_void *reg_cfg)
{
    td_s32 ret;
    isp_sharpen_ctx *sharpen = TD_NULL;
    isp_usr_ctx     *isp_ctx  = TD_NULL;
    isp_get_ctx(vi_pipe, isp_ctx);
    isp_alg_check_return(isp_ctx->alg_key.bit1_sharpen);

    ot_ext_system_isp_sharpen_init_status_write(vi_pipe, TD_FALSE);
    ret = sharpen_ctx_init(vi_pipe);
    if (ret != TD_SUCCESS) {
        return ret;
    }

    sharpen_get_ctx(vi_pipe, sharpen);
    isp_check_pointer_return(sharpen);

    sharpen->manual_sharpen_yuv_enabled = 1;
    sharpen->sharpen_en                 = TD_TRUE;
    sharpen->iso_last                   = 0;

    isp_sharpen_ctx_init(sharpen);

    sharpen->init = TD_FALSE;
    sharpen_regs_initialize(vi_pipe, (isp_reg_cfg *)reg_cfg);
    ret = sharpen_ext_regs_initialize(vi_pipe);
    if (ret != TD_SUCCESS) {
        return ret;
    }

    ot_ext_system_isp_sharpen_init_status_write(vi_pipe, TD_TRUE);
    sharpen->init = TD_TRUE;

    return TD_SUCCESS;
}

static td_void isp_sharpen_wdr_mode_set(ot_vi_pipe vi_pipe, td_void *reg_cfg_info)
{
    td_u8  i;
    td_s32 ret;
    td_u32 update_idx[OT_ISP_STRIPING_MAX_NUM] = {0};
    isp_reg_cfg *reg_cfg = (isp_reg_cfg *)reg_cfg_info;

    for (i = 0; i < reg_cfg->cfg_num; i++) {
        update_idx[i] = reg_cfg->alg_reg_cfg[i].sharpen_reg_cfg.dyna_reg_cfg.mpi_dyna_reg_cfg.update_index;
    }

    ret = isp_sharpen_init(vi_pipe, reg_cfg);
    if (ret != TD_SUCCESS) {
        return;
    }

    for (i = 0; i < reg_cfg->cfg_num; i++) {
        reg_cfg->alg_reg_cfg[i].sharpen_reg_cfg.dyna_reg_cfg.mpi_dyna_reg_cfg.update_index = update_idx[i] + 1;
        reg_cfg->alg_reg_cfg[i].sharpen_reg_cfg.dyna_reg_cfg.mpi_dyna_reg_cfg.switch_mode    = TD_TRUE;
    }
}

static td_s32 isp_sharpen_get_linear_default_reg_cfg(isp_sharpen_ctx *sharpen_para)
{
    /* linear mode defalt regs */
    td_u8  gain_thd_sel_ud_linear[SHRP_ISO_NUM]    = {1,   1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1};
    td_u8  gain_thd_sft_ud_linear[SHRP_ISO_NUM]    = {0,   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0};
    td_u8  sht_var_wgt0_linear[SHRP_ISO_NUM]       = {20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20};
    td_u8  sht_var_diff_thd0_linear[SHRP_ISO_NUM]  = {27, 29, 31, 33, 37, 40, 42, 48, 49, 50, 53, 53, 53, 53, 53, 53};
    td_u8  sht_var_diff_thd1_linear[SHRP_ISO_NUM]  = {49, 50, 51, 52, 54, 56, 56, 61, 63, 64, 66, 67, 67, 67, 67, 67};
    td_u8  sht_var_diff_wgt1_linear[SHRP_ISO_NUM]  = {10, 10, 10, 10, 15, 15, 15, 18, 20, 20, 20, 20, 20, 20, 20, 20};

    td_u8  sft     = sharpen_para->sft;
    td_u16 wgt_pre = (td_u16)sharpen_para->wgt_pre;
    td_u16 wgt_cur = (td_u16)sharpen_para->wgt_cur;
    td_u32 idx_cur = sharpen_para->idx_cur;
    td_u32 idx_pre = sharpen_para->idx_pre;

    sharpen_para->gain_thd_sel_ud    =  (td_u8)shrp_blend(sft, wgt_pre, gain_thd_sel_ud_linear[idx_pre],
                                                          wgt_cur, gain_thd_sel_ud_linear[idx_cur]);
    sharpen_para->gain_thd_sft_ud    =  (td_u8)shrp_blend(sft, wgt_pre, gain_thd_sft_ud_linear[idx_pre],
                                                          wgt_cur, gain_thd_sft_ud_linear[idx_cur]);
    sharpen_para->sht_var_wgt0       =  (td_u8)shrp_blend(sft, wgt_pre, sht_var_wgt0_linear[idx_pre],
                                                          wgt_cur, sht_var_wgt0_linear[idx_cur]);
    sharpen_para->sht_var_diff_thd0  =  (td_u8)shrp_blend(sft, wgt_pre, sht_var_diff_thd0_linear[idx_pre],
                                                          wgt_cur, sht_var_diff_thd0_linear[idx_cur]);
    sharpen_para->sht_var_diff_thd1  =  (td_u8)shrp_blend(sft, wgt_pre, sht_var_diff_thd1_linear[idx_pre],
                                                          wgt_cur, sht_var_diff_thd1_linear[idx_cur]);
    sharpen_para->sht_var_diff_wgt1  =  (td_u8)shrp_blend(sft, wgt_pre, sht_var_diff_wgt1_linear[idx_pre],
                                                          wgt_cur, sht_var_diff_wgt1_linear[idx_cur]);

    return TD_SUCCESS;
}

static td_s32 isp_sharpen_get_wdr_default_reg_cfg(isp_sharpen_ctx *sharpen_para)
{
    /* WDR mode defalt regs */
    td_u8  gain_thd_sel_ud_wdr[SHRP_ISO_NUM]   = { 2,  2,  2,  2,  2,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1 };
    td_u8  gain_thd_sft_ud_wdr[SHRP_ISO_NUM]   = { 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0 };
    td_u8  sht_var_wgt0_wdr[SHRP_ISO_NUM]      = { 65, 65, 65, 65, 65, 55, 25, 25, 25, 25, 25, 20, 20, 20, 20, 20 };
    td_u8  sht_var_diff_thd0_wdr[SHRP_ISO_NUM] = { 27, 29, 31, 33, 37, 40, 42, 48, 49, 50, 53, 53, 53, 53, 53, 53 };
    td_u8  sht_var_diff_thd1_wdr[SHRP_ISO_NUM] = { 49, 50, 51, 52, 54, 56, 56, 61, 63, 64, 66, 67, 67, 67, 67, 67 };
    td_u8  sht_var_diff_wgt1_wdr[SHRP_ISO_NUM] = { 5,  5,  5,  5,  5,  5,  5,  5,  5,  5,  5,  5,  5,  5,  5,  5 };

    td_u8  sft     = sharpen_para->sft;
    td_u16 wgt_pre = (td_u16)sharpen_para->wgt_pre;
    td_u16 wgt_cur = (td_u16)sharpen_para->wgt_cur;
    td_u32 idx_cur = sharpen_para->idx_cur;
    td_u32 idx_pre = sharpen_para->idx_pre;

    sharpen_para->gain_thd_sel_ud    =  (td_u8)shrp_blend(sft, wgt_pre, gain_thd_sel_ud_wdr[idx_pre],
                                                          wgt_cur, gain_thd_sel_ud_wdr[idx_cur]);
    sharpen_para->gain_thd_sft_ud    =  (td_u8)shrp_blend(sft, wgt_pre, gain_thd_sft_ud_wdr[idx_pre],
                                                          wgt_cur, gain_thd_sft_ud_wdr[idx_cur]);
    sharpen_para->sht_var_wgt0       =  (td_u8)shrp_blend(sft, wgt_pre, sht_var_wgt0_wdr[idx_pre],
                                                          wgt_cur, sht_var_wgt0_wdr[idx_cur]);
    sharpen_para->sht_var_diff_thd0  =  (td_u8)shrp_blend(sft, wgt_pre, sht_var_diff_thd0_wdr[idx_pre],
                                                          wgt_cur, sht_var_diff_thd0_wdr[idx_cur]);
    sharpen_para->sht_var_diff_thd1  =  (td_u8)shrp_blend(sft, wgt_pre, sht_var_diff_thd1_wdr[idx_pre],
                                                          wgt_cur, sht_var_diff_thd1_wdr[idx_cur]);
    sharpen_para->sht_var_diff_wgt1  =  (td_u8)shrp_blend(sft, wgt_pre, sht_var_diff_wgt1_wdr[idx_pre],
                                                          wgt_cur, sht_var_diff_wgt1_wdr[idx_cur]);

    return TD_SUCCESS;
}

static td_void isp_sharpen_get_default_common_reg_cfg(isp_sharpen_ctx *sharpen_para)
{
    /* common regs */
    td_u8  gain_sft[SHRP_ISO_NUM]   = {0,   0,   0,  0,   0,    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0};
    td_u8  dir_sft[SHRP_ISO_NUM]    = {10,  10,  10, 10,  10,   10,  10,   8,   7,   6,   5,   4,   3,   3,   3,   3};
    td_u8  sel_wgt[SHRP_ISO_NUM]    = {31,  31,  31, 31,   31,  31,  31,  31,  31,  31,  31,  31,  31,  31,  31,  31};
    td_u16 rmf_scale[SHRP_ISO_NUM]  = {6,   6,   6,  6,    6,   6,   6,   6,   6,   4,   2,   2,   2,   2,   2,   2};
    td_u16 bmf_scale[SHRP_ISO_NUM]  = {6,   6,   6,  6,    4,   4,   4,   4,   4,   4,   4,   4,   4,   4,   4,   4};
    td_u8  mot_thd[SHRP_ISO_NUM]         = {9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9};
    td_u8  mot_interp_mode[SHRP_ISO_NUM] = {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1};
    td_u8  std_comb_mode[SHRP_ISO_NUM]   = {2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2};
    td_u8  std_comb_alpha[SHRP_ISO_NUM]  = {4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4};
    td_u16 dir_var_thd[SHRP_ISO_NUM]     = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

    td_u16 edge_rly_fine_threshold_delta[SHRP_ISO_NUM] = {0, 0, 0, 0, 0, 0, 50, 100, 150, 150, 150,
                                                          150, 150, 150, 150, 150};
    td_u16 edge_rly_coarse_threshold_delta[SHRP_ISO_NUM] = {100, 100, 100, 100, 100, 100, 100, 100,
                                                            100, 100, 100, 100, 100, 100, 100, 100};

    td_u8  sft     = sharpen_para->sft;
    td_u16 wgt_pre = (td_u16)sharpen_para->wgt_pre;
    td_u16 wgt_cur = (td_u16)sharpen_para->wgt_cur;
    td_u32 idx_cur = sharpen_para->idx_cur;
    td_u32 idx_pre = sharpen_para->idx_pre;

    sharpen_para->gain_thd_sft_d  = (td_u8)shrp_blend(sft, wgt_pre, gain_sft[idx_pre],  wgt_cur, gain_sft[idx_cur]);
    sharpen_para->dir_var_sft     = (td_u8)shrp_blend(sft, wgt_pre, dir_sft[idx_pre],   wgt_cur, dir_sft[idx_cur]);
    sharpen_para->sel_pix_wgt     = (td_u8)shrp_blend(sft, wgt_pre, sel_wgt[idx_pre],   wgt_cur, sel_wgt[idx_cur]);
    sharpen_para->rmf_gain_scale  = (td_u8)shrp_blend(sft, wgt_pre, rmf_scale[idx_pre], wgt_cur, rmf_scale[idx_cur]);
    sharpen_para->bmf_gain_scale  = (td_u8)shrp_blend(sft, wgt_pre, bmf_scale[idx_pre], wgt_cur, bmf_scale[idx_cur]);

    sharpen_para->mot_thd         = (td_u8)shrp_blend(sft, wgt_pre, mot_thd[idx_pre], wgt_cur, mot_thd[idx_cur]);
    sharpen_para->mot_interp_mode =
        (td_u8)shrp_blend(sft, wgt_pre, mot_interp_mode[idx_pre], wgt_cur, mot_interp_mode[idx_cur]);
    sharpen_para->std_comb_mode =
        (td_u8)shrp_blend(sft, wgt_pre, std_comb_mode[idx_pre], wgt_cur, std_comb_mode[idx_cur]);
    sharpen_para->std_comb_alpha =
        (td_u8)shrp_blend(sft, wgt_pre, std_comb_alpha[idx_pre], wgt_cur, std_comb_alpha[idx_cur]);
    sharpen_para->dir_var_thd = (td_u8)shrp_blend(sft, wgt_pre, dir_var_thd[idx_pre], wgt_cur, dir_var_thd[idx_cur]);

    sharpen_para->edge_rly_fine_threshold_delta = (td_u16)shrp_blend(sft, wgt_pre,
        edge_rly_fine_threshold_delta[idx_pre], wgt_cur, edge_rly_fine_threshold_delta[idx_cur]);
    sharpen_para->edge_rly_coarse_threshold_delta = (td_u16)shrp_blend(sft, wgt_pre,
        edge_rly_coarse_threshold_delta[idx_pre], wgt_cur, edge_rly_coarse_threshold_delta[idx_cur]);
}

static td_void isp_sharpen_get_default_reg_cfg(ot_vi_pipe vi_pipe)
{
    td_u8  wdr_mode;

    isp_usr_ctx *isp_ctx = TD_NULL;
    isp_sharpen_ctx *sharpen_para = TD_NULL;

    sharpen_get_ctx(vi_pipe, sharpen_para);
    isp_get_ctx(vi_pipe, isp_ctx);

    wdr_mode = isp_ctx->sns_wdr_mode;

    isp_sharpen_get_default_common_reg_cfg(sharpen_para);

    /* linear mode default regs */
    if (is_linear_mode(wdr_mode) || (isp_ctx->linkage.fswdr_mode == OT_ISP_FSWDR_LONG_FRAME_MODE) ||
        (isp_ctx->linkage.fswdr_mode == OT_ISP_FSWDR_AUTO_LONG_FRAME_MODE)) {
        isp_sharpen_get_linear_default_reg_cfg(sharpen_para);
    } else { /* WDR mode default regs */
        isp_sharpen_get_wdr_default_reg_cfg(sharpen_para);
    }
}

static td_void isp_sharpen_get_mpi_reg_cfg(isp_sharpen_ctx *sharpen_para)
{
    td_u8  i;
    td_u8  sft     = sharpen_para->sft;
    td_u16 wgt_pre = (td_u16)sharpen_para->wgt_pre;
    td_u16 wgt_cur = (td_u16)sharpen_para->wgt_cur;
    td_u32 idx_cur = sharpen_para->idx_cur;
    td_u32 idx_pre = sharpen_para->idx_pre;

    for (i = 0; i < OT_ISP_SHARPEN_GAIN_NUM; i++) {
        sharpen_para->texture_str[i] =
            (td_u16)shrp_blend(sft, wgt_pre, sharpen_para->auto_attr.texture_strength[i][idx_pre], wgt_cur,
            sharpen_para->auto_attr.texture_strength[i][idx_cur]);
        sharpen_para->edge_str[i] = (td_u16)shrp_blend(sft, wgt_pre, sharpen_para->auto_attr.edge_strength[i][idx_pre],
            wgt_cur, sharpen_para->auto_attr.edge_strength[i][idx_cur]);
    }

    for (i = 0; i < OT_ISP_SHARPEN_LUMA_NUM; i++) {
        sharpen_para->luma_wgt[i]    =  (td_u8)shrp_blend(sft, wgt_pre, sharpen_para->auto_attr.luma_wgt[i][idx_pre],
                                                          wgt_cur, sharpen_para->auto_attr.luma_wgt[i][idx_cur]);
    }

    sharpen_para->texture_freq       = (td_u16)shrp_blend(sft, wgt_pre, sharpen_para->auto_attr.texture_freq[idx_pre],
                                                          wgt_cur, sharpen_para->auto_attr.texture_freq[idx_cur]);
    sharpen_para->edge_freq          = (td_u16)shrp_blend(sft, wgt_pre, sharpen_para->auto_attr.edge_freq[idx_pre],
                                                          wgt_cur, sharpen_para->auto_attr.edge_freq[idx_cur]);
    sharpen_para->over_shoot         = (td_u8)shrp_blend(sft, wgt_pre, sharpen_para->auto_attr.over_shoot[idx_pre],
                                                         wgt_cur, sharpen_para->auto_attr.over_shoot[idx_cur]);
    sharpen_para->under_shoot        = (td_u8)shrp_blend(sft, wgt_pre, sharpen_para->auto_attr.under_shoot[idx_pre],
                                                         wgt_cur, sharpen_para->auto_attr.under_shoot[idx_cur]);
    sharpen_para->shoot_sup_str      = (td_u8)shrp_blend(sft, wgt_pre,
                                                         sharpen_para->auto_attr.shoot_sup_strength[idx_pre],
                                                         wgt_cur, sharpen_para->auto_attr.shoot_sup_strength[idx_cur]);
    sharpen_para->detail_ctrl        = (td_u8)shrp_blend(sft, wgt_pre, sharpen_para->auto_attr.detail_ctrl[idx_pre],
                                                         wgt_cur, sharpen_para->auto_attr.detail_ctrl[idx_cur]);
    sharpen_para->edge_filt_str      = (td_u8)shrp_blend(sft, wgt_pre,
                                                         sharpen_para->auto_attr.edge_filt_strength[idx_pre],
                                                         wgt_cur, sharpen_para->auto_attr.edge_filt_strength[idx_cur]);
    sharpen_para->edge_filt_max_cap  = (td_u8)shrp_blend(sft, wgt_pre,
                                                         sharpen_para->auto_attr.edge_filt_max_cap[idx_pre],
                                                         wgt_cur, sharpen_para->auto_attr.edge_filt_max_cap[idx_cur]);
    sharpen_para->r_gain             = (td_u8)shrp_blend(sft, wgt_pre, sharpen_para->auto_attr.r_gain[idx_pre],
                                                         wgt_cur, sharpen_para->auto_attr.r_gain[idx_cur]);
    sharpen_para->g_gain             = (td_u8)shrp_blend(sft, wgt_pre, sharpen_para->auto_attr.g_gain[idx_pre],
                                                         wgt_cur, sharpen_para->auto_attr.g_gain[idx_cur]);
    sharpen_para->b_gain             = (td_u8)shrp_blend(sft, wgt_pre, sharpen_para->auto_attr.b_gain[idx_pre],
                                                         wgt_cur, sharpen_para->auto_attr.b_gain[idx_cur]);
    sharpen_para->skin_gain          = (td_u8)shrp_blend(sft, wgt_pre, sharpen_para->auto_attr.skin_gain[idx_pre],
                                                         wgt_cur, sharpen_para->auto_attr.skin_gain[idx_cur]);
    sharpen_para->shoot_sup_adj      = (td_u8)shrp_blend(sft, wgt_pre, sharpen_para->auto_attr.shoot_sup_adj[idx_pre],
                                                         wgt_cur, sharpen_para->auto_attr.shoot_sup_adj[idx_cur]);
    sharpen_para->detail_ctrl_thr    = (td_u8)shrp_blend(sft, wgt_pre,
                                                         sharpen_para->auto_attr.detail_ctrl_threshold[idx_pre],
                                                         wgt_cur,
                                                         sharpen_para->auto_attr.detail_ctrl_threshold[idx_cur]);
    sharpen_para->max_sharp_gain     = (td_u16)shrp_blend(sft, wgt_pre, sharpen_para->auto_attr.max_sharp_gain[idx_pre],
                                                          wgt_cur, sharpen_para->auto_attr.max_sharp_gain[idx_cur]);

    sharpen_para->shoot_inner_threshold   = (td_u16)shrp_blend(sft, wgt_pre,
        sharpen_para->auto_attr.shoot_threshold_attr.shoot_inner_threshold[idx_pre],
        wgt_cur, sharpen_para->auto_attr.shoot_threshold_attr.shoot_inner_threshold[idx_cur]);
    sharpen_para->shoot_outer_threshold   = (td_u16)shrp_blend(sft, wgt_pre,
        sharpen_para->auto_attr.shoot_threshold_attr.shoot_outer_threshold[idx_pre],
        wgt_cur, sharpen_para->auto_attr.shoot_threshold_attr.shoot_outer_threshold[idx_cur]);
    sharpen_para->shoot_protect_threshold = (td_u16)shrp_blend(sft, wgt_pre,
        sharpen_para->auto_attr.shoot_threshold_attr.shoot_protect_threshold[idx_pre],
        wgt_cur, sharpen_para->auto_attr.shoot_threshold_attr.shoot_protect_threshold[idx_cur]);

    sharpen_para->edge_rly_fine_threshold   = (td_u16)shrp_blend(sft, wgt_pre,
        sharpen_para->auto_attr.edge_rly_attr.edge_rly_fine_threshold[idx_pre], wgt_cur,
        sharpen_para->auto_attr.edge_rly_attr.edge_rly_fine_threshold[idx_cur]);
    sharpen_para->edge_rly_coarse_threshold = (td_u16)shrp_blend(sft, wgt_pre,
        sharpen_para->auto_attr.edge_rly_attr.edge_rly_coarse_threshold[idx_pre],
        wgt_cur, sharpen_para->auto_attr.edge_rly_attr.edge_rly_coarse_threshold[idx_cur]);
    sharpen_para->edge_overshoot            = (td_u8)shrp_blend(sft, wgt_pre,
        sharpen_para->auto_attr.edge_rly_attr.edge_overshoot[idx_pre],
        wgt_cur, sharpen_para->auto_attr.edge_rly_attr.edge_overshoot[idx_cur]);
    sharpen_para->edge_undershoot = (td_u8)shrp_blend(sft, wgt_pre,
        sharpen_para->auto_attr.edge_rly_attr.edge_undershoot[idx_pre],
        wgt_cur, sharpen_para->auto_attr.edge_rly_attr.edge_undershoot[idx_cur]);

    for (i = 0; i < OT_ISP_SHARPEN_RLYWGT_NUM; i++) {
        sharpen_para->edge_gain_by_rly[i]    =  (td_u8)shrp_blend(sft, wgt_pre,
            sharpen_para->auto_attr.edge_rly_attr.edge_gain_by_rly[i][idx_pre], wgt_cur,
            sharpen_para->auto_attr.edge_rly_attr.edge_gain_by_rly[i][idx_cur]);
    }

    for (i = 0; i < OT_ISP_SHARPEN_STDGAIN_NUM; i++) {
        sharpen_para->edge_rly_by_mot[i]    =  (td_u8)shrp_blend(sft, wgt_pre,
            sharpen_para->auto_attr.edge_rly_attr.edge_rly_by_mot[i][idx_pre], wgt_cur,
            sharpen_para->auto_attr.edge_rly_attr.edge_rly_by_mot[i][idx_cur]);
        sharpen_para->edge_rly_by_luma[i]   =  (td_u8)shrp_blend(sft, wgt_pre,
            sharpen_para->auto_attr.edge_rly_attr.edge_rly_by_luma[i][idx_pre], wgt_cur,
            sharpen_para->auto_attr.edge_rly_attr.edge_rly_by_luma[i][idx_cur]);
    }

    for (i = 0; i < OT_ISP_SHARPEN_MOT_NUM; i++) {
        sharpen_para->mf_gain_by_mot[i]  =  (td_u8)shrp_blend(sft, wgt_pre,
            sharpen_para->auto_attr.gain_by_mot_attr.mf_gain_by_mot[i][idx_pre], wgt_cur,
            sharpen_para->auto_attr.gain_by_mot_attr.mf_gain_by_mot[i][idx_cur]);
        sharpen_para->hf_gain_by_mot[i]  =  (td_u8)shrp_blend(sft, wgt_pre,
            sharpen_para->auto_attr.gain_by_mot_attr.hf_gain_by_mot[i][idx_pre], wgt_cur,
            sharpen_para->auto_attr.gain_by_mot_attr.hf_gain_by_mot[i][idx_cur]);
        sharpen_para->lmf_gain_by_mot[i] =  (td_u8)shrp_blend(sft, wgt_pre,
            sharpen_para->auto_attr.gain_by_mot_attr.lmf_gain_by_mot[i][idx_pre], wgt_cur,
            sharpen_para->auto_attr.gain_by_mot_attr.lmf_gain_by_mot[i][idx_cur]);
    }
}

static void sharpen_mpi2reg_def(isp_sharpen_reg_cfg *sharpen_reg_cfg, isp_sharpen_ctx *sharpen)
{
    isp_sharpen_default_dyna_reg_cfg *def_cfg = TD_NULL;
    isp_sharpen_dyna_reg_cfg         *dyna_reg_cfg = TD_NULL;

    dyna_reg_cfg = &(sharpen_reg_cfg->dyna_reg_cfg);
    def_cfg = &(dyna_reg_cfg->default_dyna_reg_cfg);

    def_cfg->gain_thd_sft_d      = sharpen->gain_thd_sft_d;
    def_cfg->gain_thd_sel_ud     = sharpen->gain_thd_sel_ud;
    def_cfg->gain_thd_sft_ud     = sharpen->gain_thd_sft_ud;
    def_cfg->dir_var_sft         = sharpen->dir_var_sft;
    def_cfg->sel_pix_wgt         = sharpen->sel_pix_wgt;
    def_cfg->sht_var_diff_thd[0] = sharpen->sht_var_diff_thd0;
    def_cfg->sht_var_wgt0        = sharpen->sht_var_wgt0;
    def_cfg->sht_var_diff_thd[1] = sharpen->sht_var_diff_thd1;
    def_cfg->sht_var_diff_wgt1   = sharpen->sht_var_diff_wgt1;
    def_cfg->rmf_gain_scale      = sharpen->rmf_gain_scale;
    def_cfg->bmf_gain_scale      = sharpen->bmf_gain_scale;
    def_cfg->dir_var_thd         = sharpen->dir_var_thd;
    def_cfg->mot_thd             = sharpen->mot_thd;
    def_cfg->std_comb_mode       = sharpen->std_comb_mode;
    def_cfg->std_comb_alpha      = sharpen->std_comb_alpha;
    def_cfg->mot_interp_mode     = sharpen->mot_interp_mode;
}

static void sharpen_mpi2reg_mf_gain_d(isp_sharpen_reg_cfg *sharpen_reg_cfg, isp_sharpen_ctx *sharpen)
{
    td_u8  i, j;
    td_u32 tmp;
    td_u16 *eg_str = sharpen->edge_str;

    isp_sharpen_mpi_dyna_reg_cfg     *mpi_cfg = TD_NULL;
    isp_sharpen_dyna_reg_cfg         *dyna_reg_cfg = TD_NULL;

    dyna_reg_cfg = &(sharpen_reg_cfg->dyna_reg_cfg);
    mpi_cfg = &(dyna_reg_cfg->mpi_dyna_reg_cfg);

    for (i = 0; i < OT_ISP_SHARPEN_GAIN_NUM; i++) {
        j = i << 1;
        if (i < OT_ISP_SHARPEN_GAIN_NUM - 1) {
            mpi_cfg->mf_gain_d[j]      = MIN2(0xFFF, (0x20 + eg_str[i]));
            mpi_cfg->mf_gain_d[j + 1]  = MIN2(0xFFF, (0x20 + ((eg_str[i] + eg_str[i + 1]) >> 1)));
        } else { /* 31 */
            mpi_cfg->mf_gain_d[j]      = MIN2(0xFFF, (0x20 + eg_str[i]));
            mpi_cfg->mf_gain_d[j + 1]  = MIN2(0xFFF, (0x20 + eg_str[i]));
        }
        tmp = (td_u32)mpi_cfg->mf_gain_d[j] * sharpen->edge_freq;
        mpi_cfg->hf_gain_d[j]     = (td_u16)MIN2(0xFFF, (tmp >> 0x6));
        tmp = (td_u32)mpi_cfg->mf_gain_d[j + 1] * sharpen->edge_freq;
        mpi_cfg->hf_gain_d[j + 1] = (td_u16)MIN2(0xFFF, (tmp >> 0x6));
    }
}

static void sharpen_mpi2reg_mf_thd_sel_ud_enable(isp_sharpen_reg_cfg *sharpen_reg_cfg, isp_sharpen_ctx *sharpen)
{
    td_u8  i, j;
    td_u16 *tx_str = sharpen->texture_str;

    isp_sharpen_mpi_dyna_reg_cfg     *mpi_cfg = TD_NULL;
    isp_sharpen_dyna_reg_cfg         *dyna_reg_cfg = TD_NULL;

    dyna_reg_cfg = &(sharpen_reg_cfg->dyna_reg_cfg);
    mpi_cfg = &(dyna_reg_cfg->mpi_dyna_reg_cfg);

    j = 0;
    for (i = 0; i < 12; i++) {  /* 1st segment  [0,12)  */
        mpi_cfg->mf_gain_ud[j++] = MIN2(0xFFF, (0x20 + tx_str[i]));
    }
    for (i = 12; i < 20; i++) { /* 2nd segment  [12,20) */
        mpi_cfg->mf_gain_ud[j++] = MIN2(0xFFF, (0x20 + tx_str[i]));
        mpi_cfg->mf_gain_ud[j++] = MIN2(0xFFF, (0x20 + ((tx_str[i] + tx_str[i + 1]) >> 1)));
    }
    for (i = 20; i < 31; i++) { /* 3rd segment  [20,31) */
        mpi_cfg->mf_gain_ud[j++] = MIN2(0xFFF, (0x20 + tx_str[i]));
        mpi_cfg->mf_gain_ud[j++] = MIN2(0xFFF, (0x20 + ((tx_str[i]) * 2 + (tx_str[i + 1])) / 3)); /* (2+1)/3 */
        mpi_cfg->mf_gain_ud[j++] = MIN2(0xFFF, (0x20 + ((tx_str[i]) + (tx_str[i + 1]) * 2) / 3)); /* (1+2)/3 */
    }
    i = 31;                     /* last segment [31]    */
    mpi_cfg->mf_gain_ud[j++] = MIN2(0xFFF, (0x20 + tx_str[i]));
    mpi_cfg->mf_gain_ud[j++] = MIN2(0xFFF, (0x20 + tx_str[i]));
    mpi_cfg->mf_gain_ud[j++] = MIN2(0xFFF, (0x20 + tx_str[i]));
}

static void sharpen_mpi2reg_mf_thd_sel_ud_disable(isp_sharpen_reg_cfg *sharpen_reg_cfg, isp_sharpen_ctx *sharpen)
{
    td_u8  i, j;
    td_u16 *tx_str = sharpen->texture_str;

    isp_sharpen_mpi_dyna_reg_cfg     *mpi_cfg = TD_NULL;
    isp_sharpen_dyna_reg_cfg         *dyna_reg_cfg = TD_NULL;

    dyna_reg_cfg = &(sharpen_reg_cfg->dyna_reg_cfg);
    mpi_cfg = &(dyna_reg_cfg->mpi_dyna_reg_cfg);

    j = 0;
    for (i = 0; i < 16; i++) {  /* 1st segment [0,16)  */
        mpi_cfg->mf_gain_ud[j++] = MIN2(0xFFF, (0x20 + tx_str[i]));
    }
    for (i = 16; i < 24; i++) { /* 2nd segment [16,24) */
        mpi_cfg->mf_gain_ud[j++] = MIN2(0xFFF, (0x20 + tx_str[i]));
        mpi_cfg->mf_gain_ud[j++] = MIN2(0xFFF, (0x20 + ((tx_str[i] + tx_str[i + 1]) >> 1)));
    }
    for (i = 24; i < 28; i++) { /* 3rd segment [24,28)  */
        mpi_cfg->mf_gain_ud[j++] = MIN2(0xFFF, (0x20 + tx_str[i]));
        mpi_cfg->mf_gain_ud[j++] = MIN2(0xFFF, (0x20 + ((tx_str[i]) * 2 + (tx_str[i + 1])) / 3)); /* (2+1)/3 */
        mpi_cfg->mf_gain_ud[j++] = MIN2(0xFFF, (0x20 + ((tx_str[i]) + (tx_str[i + 1]) * 2) / 3)); /* (1+2)/3 */
    }
    for (i = 28; i < 31; i++) { /* 4th segment [28,31)  */
        mpi_cfg->mf_gain_ud[j++] = MIN2(0xFFF, (0x20 + tx_str[i]));
        mpi_cfg->mf_gain_ud[j++] = MIN2(0xFFF, (0x20 + ((tx_str[i]) * 4 + (tx_str[i + 1]))     / 5)); /* (4+1)/5 */
        mpi_cfg->mf_gain_ud[j++] = MIN2(0xFFF, (0x20 + ((tx_str[i]) * 3 + (tx_str[i + 1]) * 2) / 5)); /* (3+2)/5 */
        mpi_cfg->mf_gain_ud[j++] = MIN2(0xFFF, (0x20 + ((tx_str[i]) * 2 + (tx_str[i + 1]) * 3) / 5)); /* (2+3)/5 */
        mpi_cfg->mf_gain_ud[j++] = MIN2(0xFFF, (0x20 + ((tx_str[i])     + (tx_str[i + 1]) * 4) / 5)); /* (1+4)/5 */
    }
    i = 31;                     /* last segment [31]    */
    mpi_cfg->mf_gain_ud[j++] = MIN2(0xFFF, (0x20 + tx_str[i]));
    mpi_cfg->mf_gain_ud[j++] = MIN2(0xFFF, (0x20 + tx_str[i]));
    mpi_cfg->mf_gain_ud[j++] = MIN2(0xFFF, (0x20 + tx_str[i]));
    mpi_cfg->mf_gain_ud[j++] = MIN2(0xFFF, (0x20 + tx_str[i]));
    mpi_cfg->mf_gain_ud[j++] = MIN2(0xFFF, (0x20 + tx_str[i]));
}

static void sharpen_mpi2reg_mpi_chr(isp_sharpen_reg_cfg *sharpen_reg_cfg, isp_sharpen_ctx *sharpen)
{
    td_u8  i;
    td_u32 tmp;

    isp_sharpen_mpi_dyna_reg_cfg     *mpi_cfg = TD_NULL;
    isp_sharpen_dyna_reg_cfg         *dyna_reg_cfg = TD_NULL;

    dyna_reg_cfg = &(sharpen_reg_cfg->dyna_reg_cfg);
    mpi_cfg = &(dyna_reg_cfg->mpi_dyna_reg_cfg);

    for (i = 0; i < SHRP_GAIN_LUT_SIZE; i++) {
        tmp = (td_u32)mpi_cfg->mf_gain_ud[i] * sharpen->texture_freq;
        mpi_cfg->hf_gain_ud[i] = (td_u16)MIN2(0xFFF, (tmp >> 0x6));
    }

    mpi_cfg->osht_amt = sharpen->over_shoot;
    mpi_cfg->usht_amt = sharpen->under_shoot;
    /* skin ctrl */
    if (sharpen->skin_gain == 0x1F) {
        mpi_cfg->en_skin_ctrl = 0;
    } else {
        mpi_cfg->en_skin_ctrl = 1;
        mpi_cfg->skin_edge_wgt[1] = clip3((0x1F - sharpen->skin_gain), 0, 0x1F);
        mpi_cfg->skin_edge_wgt[0] = MIN2(0x1F, (mpi_cfg->skin_edge_wgt[1] << 1));
    }
    /* chr ctrl */
    mpi_cfg->en_chr_ctrl  = 1;
    mpi_cfg->chr_r_gain   = sharpen->r_gain;
    mpi_cfg->chr_g_gain   = sharpen->g_gain;
    mpi_cfg->chr_gmf_gain = sharpen->g_gain;
    mpi_cfg->chr_b_gain   = sharpen->b_gain;

    if (sharpen->detail_ctrl == 0x80) {
        mpi_cfg->en_detail_ctrl = 0;
    } else {
        mpi_cfg->en_detail_ctrl = 1;
    }
}

static void sharpen_mpi2reg_mpi_detail(isp_sharpen_reg_cfg *sharpen_reg_cfg, isp_sharpen_ctx *sharpen)
{
    td_u8  i;

    isp_sharpen_mpi_dyna_reg_cfg     *mpi_cfg = TD_NULL;
    isp_sharpen_dyna_reg_cfg         *dyna_reg_cfg = TD_NULL;

    dyna_reg_cfg = &(sharpen_reg_cfg->dyna_reg_cfg);
    mpi_cfg = &(dyna_reg_cfg->mpi_dyna_reg_cfg);

    mpi_cfg->detail_osht_amt = clip3((mpi_cfg->osht_amt) + (sharpen->detail_ctrl) - 0x80, 0, 0x7F);
    mpi_cfg->detail_usht_amt = clip3((mpi_cfg->usht_amt) + (sharpen->detail_ctrl) - 0x80, 0, 0x7F);

    /* dir */
    mpi_cfg->dir_diff_sft  = 0x3F - sharpen->edge_filt_str;

    if (sharpen->edge_filt_max_cap <= 12) { /* max cap thd0 12 */
        mpi_cfg->dir_rt[1] = sharpen->edge_filt_max_cap;
        mpi_cfg->dir_rt[0] = (sharpen->edge_filt_max_cap) >> 1;
    } else if (sharpen->edge_filt_max_cap <= 30) { /* max cap thd1 30 */
        mpi_cfg->dir_rt[1] = sharpen->edge_filt_max_cap;
        mpi_cfg->dir_rt[0] = 0x6;
    } else {
        mpi_cfg->dir_rt[1] = 0x1E;
        mpi_cfg->dir_rt[0] = sharpen->edge_filt_max_cap - 0x18;
    }

    mpi_cfg->en_sht_ctrl_by_var = 1;
    mpi_cfg->sht_bld_rt        = sharpen->shoot_sup_adj;
    mpi_cfg->sht_var_thd1     = sharpen->shoot_sup_str;

    mpi_cfg->o_max_gain = sharpen->max_sharp_gain;
    mpi_cfg->u_max_gain = sharpen->max_sharp_gain;
    mpi_cfg->skin_max_u = sharpen->skin_umax;
    mpi_cfg->skin_min_u = sharpen->skin_umin;
    mpi_cfg->skin_max_v = sharpen->skin_vmax;
    mpi_cfg->skin_min_v = sharpen->skin_vmin;
    mpi_cfg->detail_osht_thr[0]  = sharpen->detail_ctrl_thr;
    mpi_cfg->detail_osht_thr[1]  = MIN2(0xFF, sharpen->detail_ctrl_thr + SHRP_DETAIL_CTRL_THR_DELTA);
    mpi_cfg->detail_usht_thr[0]  = sharpen->detail_ctrl_thr;
    mpi_cfg->detail_usht_thr[1]  = MIN2(0xFF, sharpen->detail_ctrl_thr + SHRP_DETAIL_CTRL_THR_DELTA);
    mpi_cfg->en_luma_ctrl = 0;
    for (i = 0; i < OT_ISP_SHARPEN_LUMA_NUM; i++) {
        mpi_cfg->luma_wgt[i] = sharpen->luma_wgt[i];

        if (mpi_cfg->luma_wgt[i] < 0x1F) {
            mpi_cfg->en_luma_ctrl = 1;
        }
    }

    mpi_cfg->exluma_thd = sharpen->shoot_inner_threshold;
    mpi_cfg->exluma_out_thd = sharpen->shoot_outer_threshold;
    mpi_cfg->hard_luma_thd     = sharpen->shoot_protect_threshold;

    mpi_cfg->var5_low_thd      = sharpen->edge_rly_fine_threshold;
    mpi_cfg->var5_high_thd     = sharpen->edge_rly_fine_threshold + sharpen->edge_rly_fine_threshold_delta;
    mpi_cfg->var5_mid_thd      = (mpi_cfg->var5_low_thd + mpi_cfg->var5_high_thd) >> 1;
    mpi_cfg->var7x9_low_thd    = sharpen->edge_rly_coarse_threshold;
    mpi_cfg->var7x9_high_thd   = sharpen->edge_rly_coarse_threshold + sharpen->edge_rly_coarse_threshold_delta;
    mpi_cfg->edge_osht_amt     = sharpen->edge_overshoot;
    mpi_cfg->edge_usht_amt     = sharpen->edge_undershoot;

    for (i = 0; i < OT_ISP_SHARPEN_RLYWGT_NUM; i++) {
        mpi_cfg->rly_wgt[i] = sharpen->edge_gain_by_rly[i];
    }

    mpi_cfg->en_std_adj_by_mot = 0;
    mpi_cfg->en_std_adj_by_y = 0;
    for (i = 0; i < OT_ISP_SHARPEN_STDGAIN_NUM; i++) {
        mpi_cfg->std_gain_by_mot_lut[i] = sharpen->edge_rly_by_mot[i];
        mpi_cfg->std_gain_by_y_lut[i]   = sharpen->edge_rly_by_luma[i];

        if (mpi_cfg->std_gain_by_mot_lut[i] != 0x10) {
            mpi_cfg->en_std_adj_by_mot = 1;
        }

        if (mpi_cfg->std_gain_by_y_lut[i] != 0x10) {
            mpi_cfg->en_std_adj_by_y = 1;
        }
    }
    for (i = 0; i < OT_ISP_SHARPEN_STDGAIN_NUM; i++) {
        mpi_cfg->std_offset_by_mot_lut[i] = 0;
        mpi_cfg->std_offset_by_y_lut[i]   = 0;
    }

    mpi_cfg->en_mot_ctrl = 0;
    for (i = 0; i < OT_ISP_SHARPEN_MOT_NUM; i++) {
        mpi_cfg->mf_mot_dec[i] = sharpen->mf_gain_by_mot[i];
        mpi_cfg->hf_mot_dec[i] = sharpen->hf_gain_by_mot[i];
        mpi_cfg->lmf_mot_gain[i] = sharpen->lmf_gain_by_mot[i];

        if (mpi_cfg->mf_mot_dec[i] != 0x20 || mpi_cfg->hf_mot_dec[i] != 0x20 ||
            mpi_cfg->lmf_mot_gain[i] != 0x10) {
            mpi_cfg->en_mot_ctrl = 1;
        }
    }
}

static void sharpen_mpi2reg_mul_coef(isp_sharpen_reg_cfg *sharpen_reg_cfg, isp_sharpen_ctx *sharpen)
{
    isp_sharpen_default_dyna_reg_cfg *def_cfg = TD_NULL;
    isp_sharpen_mpi_dyna_reg_cfg     *mpi_cfg = TD_NULL;
    isp_sharpen_static_reg_cfg       *static_cfg   = TD_NULL;
    isp_sharpen_dyna_reg_cfg         *dyna_reg_cfg = TD_NULL;
    ot_unused(sharpen);
    dyna_reg_cfg = &(sharpen_reg_cfg->dyna_reg_cfg);
    def_cfg = &(dyna_reg_cfg->default_dyna_reg_cfg);
    mpi_cfg = &(dyna_reg_cfg->mpi_dyna_reg_cfg);
    static_cfg   = &(sharpen_reg_cfg->static_reg_cfg);

    def_cfg->sht_var_diff_mul = (td_s16)calc_mul_coef(def_cfg->sht_var_diff_thd[0], static_cfg->sht_var_diff_wgt0,
                                                      def_cfg->sht_var_diff_thd[1], def_cfg->sht_var_diff_wgt1,
                                                      SHRP_SHT_VAR_MUL_PRECS);

    mpi_cfg->sht_var_mul      = (td_u16)calc_mul_coef(static_cfg->sht_var_thd0,  def_cfg->sht_var_wgt0,
                                                      mpi_cfg->sht_var_thd1, static_cfg->sht_var_wgt1,
                                                      SHRP_SHT_VAR_MUL_PRECS);

    mpi_cfg->detail_osht_mul  = (td_s16)calc_mul_coef(mpi_cfg->detail_osht_thr[0], mpi_cfg->detail_osht_amt,
                                                      mpi_cfg->detail_osht_thr[1], mpi_cfg->osht_amt,
                                                      SHRP_DETAIL_SHT_MUL_PRECS);

    mpi_cfg->detail_usht_mul  = (td_s16)calc_mul_coef(mpi_cfg->detail_usht_thr[0], mpi_cfg->detail_usht_amt,
                                                      mpi_cfg->detail_usht_thr[1], mpi_cfg->usht_amt,
                                                      SHRP_DETAIL_SHT_MUL_PRECS);
    mpi_cfg->chr_r_mul        = (td_s16)calc_mul_coef(static_cfg->chr_r_thd[0], mpi_cfg->chr_r_gain,
                                                      static_cfg->chr_r_thd[1], 0x20,
                                                      SHRP_CHR_MUL_SFT);
    mpi_cfg->chr_g_mul        = (td_s16)calc_mul_coef(static_cfg->chr_g_thd[0], mpi_cfg->chr_g_gain,
                                                      static_cfg->chr_g_thd[1], 0x20,
                                                      SHRP_CHR_MUL_SFT);
    mpi_cfg->chr_gmf_mul      = (td_s16)calc_mul_coef(static_cfg->chr_g_thd[0], mpi_cfg->chr_gmf_gain,
                                                      static_cfg->chr_g_thd[1], 0x20,
                                                      SHRP_CHR_MUL_SFT);
    mpi_cfg->chr_b_mul        = (td_s16)calc_mul_coef(static_cfg->chr_b_thd[0], mpi_cfg->chr_b_gain,
                                                      static_cfg->chr_b_thd[1], 0x20,
                                                      SHRP_CHR_MUL_SFT);
    mpi_cfg->skin_edge_mul    = (td_s16)calc_mul_coef(static_cfg->skin_edge_thd[0], mpi_cfg->skin_edge_wgt[0],
                                                      static_cfg->skin_edge_thd[1], mpi_cfg->skin_edge_wgt[1],
                                                      SHRP_SKIN_EDGE_MUL_PRECS);
    if (mpi_cfg->exluma_thd + mpi_cfg->exluma_out_thd == 0) {
        mpi_cfg->exluma_mul = 0;
    } else if (mpi_cfg->exluma_out_thd == 0) {
        mpi_cfg->exluma_mul = 255; // 255 max
    } else {
        mpi_cfg->exluma_mul = (td_u8)((mpi_cfg->exluma_thd << SHRP_EXLUMA_MUL_PRECS) /
            (mpi_cfg->exluma_thd + mpi_cfg->exluma_out_thd));
    }
}

static td_void sharpen_mpi2reg(isp_sharpen_reg_cfg *sharpen_reg_cfg, isp_sharpen_ctx *sharpen)
{
    isp_sharpen_default_dyna_reg_cfg *def_cfg      = TD_NULL;
    isp_sharpen_mpi_dyna_reg_cfg     *mpi_cfg      = TD_NULL;
    isp_sharpen_dyna_reg_cfg         *dyna_reg_cfg = TD_NULL;

    dyna_reg_cfg = &(sharpen_reg_cfg->dyna_reg_cfg);
    mpi_cfg      = &(dyna_reg_cfg->mpi_dyna_reg_cfg);
    def_cfg      = &(dyna_reg_cfg->default_dyna_reg_cfg);

    if (def_cfg->resh) {
        sharpen_mpi2reg_def(sharpen_reg_cfg, sharpen);
    }

    if (mpi_cfg->resh) {
        sharpen_mpi2reg_mf_gain_d(sharpen_reg_cfg, sharpen);
        if (def_cfg->gain_thd_sel_ud == 1) {
            sharpen_mpi2reg_mf_thd_sel_ud_enable(sharpen_reg_cfg, sharpen);
        } else {
            sharpen_mpi2reg_mf_thd_sel_ud_disable(sharpen_reg_cfg, sharpen);
        }
        sharpen_mpi2reg_mpi_chr(sharpen_reg_cfg, sharpen);
        sharpen_mpi2reg_mpi_detail(sharpen_reg_cfg, sharpen);
    }

    sharpen_mpi2reg_mul_coef(sharpen_reg_cfg, sharpen);
}

__inline static td_bool check_sharpen_open(isp_sharpen_ctx *sharpen)
{
    return (sharpen->sharpen_en == TD_TRUE);
}

static td_void sharpen_actual_update(ot_vi_pipe vi_pipe, isp_sharpen_ctx *sharpen)
{
    td_u8 i;

    ot_ext_system_actual_sharpen_overshoot_amt_write(vi_pipe, sharpen->over_shoot);
    ot_ext_system_actual_sharpen_undershoot_amt_write(vi_pipe, sharpen->under_shoot);
    ot_ext_system_actual_sharpen_shoot_sup_write(vi_pipe, sharpen->shoot_sup_str);
    ot_ext_system_actual_sharpen_edge_frequence_write(vi_pipe, sharpen->edge_freq);
    ot_ext_system_actual_sharpen_texture_frequence_write(vi_pipe, sharpen->texture_freq);

    for (i = 0; i < OT_ISP_SHARPEN_GAIN_NUM; i++) {
        ot_ext_system_actual_sharpen_edge_str_write(vi_pipe, i, sharpen->edge_str[i]);
        ot_ext_system_actual_sharpen_texture_str_write(vi_pipe, i, sharpen->texture_str[i]);
    }
}

td_s32 sharpen_proc_write(ot_isp_ctrl_proc_write *proc, const isp_sharpen_ctx *shp_ctx)
{
    td_s32 i, index;
    ot_isp_ctrl_proc_write proc_tmp;

    if ((proc->proc_buff == TD_NULL) || (proc->buff_len == 0)) {
        return TD_FAILURE;
    }

    proc_tmp.proc_buff = proc->proc_buff;
    proc_tmp.buff_len = proc->buff_len;

    isp_proc_print_title(&proc_tmp, &proc->write_len, "sharpen info");
    isp_proc_printf(&proc_tmp, proc->write_len, "%16s\n", "sharpen_en");
    isp_proc_printf(&proc_tmp, proc->write_len, "%16u\n", (td_u16)shp_ctx->sharpen_en);

    for (i = 0; i < 0x2; i++) {
        index = i * 0x10;
        isp_proc_printf(&proc_tmp, proc->write_len, "%s"  "(%d"  "-"  "%d):\n", "luma_wgt", index, index + 0xf);
        isp_proc_printf(&proc_tmp, proc->write_len, "%7u"  "%7u"  "%7u"  "%7u"  "%7u"  "%7u"  "%7u"  "%7u"
            "%7u"  "%7u"  "%7u"  "%7u"  "%7u"  "%7u"  "%7u"  "%7u\n\n",
            (td_u16)shp_ctx->luma_wgt[index + 0x0], (td_u16)shp_ctx->luma_wgt[index + 0x1],
            (td_u16)shp_ctx->luma_wgt[index + 0x2], (td_u16)shp_ctx->luma_wgt[index + 0x3],
            (td_u16)shp_ctx->luma_wgt[index + 0x4], (td_u16)shp_ctx->luma_wgt[index + 0x5],
            (td_u16)shp_ctx->luma_wgt[index + 0x6], (td_u16)shp_ctx->luma_wgt[index + 0x7],
            (td_u16)shp_ctx->luma_wgt[index + 0x8], (td_u16)shp_ctx->luma_wgt[index + 0x9],
            (td_u16)shp_ctx->luma_wgt[index + 0xa], (td_u16)shp_ctx->luma_wgt[index + 0xb],
            (td_u16)shp_ctx->luma_wgt[index + 0xc], (td_u16)shp_ctx->luma_wgt[index + 0xd],
            (td_u16)shp_ctx->luma_wgt[index + 0xe], (td_u16)shp_ctx->luma_wgt[index + 0xf]);
    }
    for (i = 0; i < 0x2; i++) {
        index = i * 0x10;
        isp_proc_printf(&proc_tmp, proc->write_len, "%s"  "(%d"  "-"  "%d):\n", "texture_strength",
            index, index + 0xf);
        isp_proc_printf(&proc_tmp, proc->write_len, "%7u"  "%7u"  "%7u"  "%7u"  "%7u"  "%7u"  "%7u"  "%7u"
            "%7u"  "%7u"  "%7u"  "%7u"  "%7u"  "%7u"  "%7u"  "%7u\n\n",
            (td_u16)shp_ctx->texture_str[index + 0x0], (td_u16)shp_ctx->texture_str[index + 0x1],
            (td_u16)shp_ctx->texture_str[index + 0x2], (td_u16)shp_ctx->texture_str[index + 0x3],
            (td_u16)shp_ctx->texture_str[index + 0x4], (td_u16)shp_ctx->texture_str[index + 0x5],
            (td_u16)shp_ctx->texture_str[index + 0x6], (td_u16)shp_ctx->texture_str[index + 0x7],
            (td_u16)shp_ctx->texture_str[index + 0x8], (td_u16)shp_ctx->texture_str[index + 0x9],
            (td_u16)shp_ctx->texture_str[index + 0xa], (td_u16)shp_ctx->texture_str[index + 0xb],
            (td_u16)shp_ctx->texture_str[index + 0xc], (td_u16)shp_ctx->texture_str[index + 0xd],
            (td_u16)shp_ctx->texture_str[index + 0xe], (td_u16)shp_ctx->texture_str[index + 0xf]);
    }
    for (i = 0; i < 0x2; i++) {
        index = i * 0x10;
        isp_proc_printf(&proc_tmp, proc->write_len, "%s"  "(%d"  "-"  "%d):\n", "edge_strength", index, index + 0xf);
        isp_proc_printf(&proc_tmp, proc->write_len, "%7u"  "%7u"  "%7u"  "%7u"  "%7u"  "%7u"  "%7u"  "%7u"
            "%7u"  "%7u"  "%7u"  "%7u"  "%7u"  "%7u"  "%7u"  "%7u\n\n",
            (td_u16)shp_ctx->edge_str[index + 0x0], (td_u16)shp_ctx->edge_str[index + 0x1],
            (td_u16)shp_ctx->edge_str[index + 0x2], (td_u16)shp_ctx->edge_str[index + 0x3],
            (td_u16)shp_ctx->edge_str[index + 0x4], (td_u16)shp_ctx->edge_str[index + 0x5],
            (td_u16)shp_ctx->edge_str[index + 0x6], (td_u16)shp_ctx->edge_str[index + 0x7],
            (td_u16)shp_ctx->edge_str[index + 0x8], (td_u16)shp_ctx->edge_str[index + 0x9],
            (td_u16)shp_ctx->edge_str[index + 0xa], (td_u16)shp_ctx->edge_str[index + 0xb],
            (td_u16)shp_ctx->edge_str[index + 0xc], (td_u16)shp_ctx->edge_str[index + 0xd],
            (td_u16)shp_ctx->edge_str[index + 0xe], (td_u16)shp_ctx->edge_str[index + 0xf]);
    }

    isp_proc_printf(&proc_tmp, proc->write_len, "%s"  "(%d"  "-"  "%d):\n", "edge_gain_by_rly",
        0, 0xf);
    isp_proc_printf(&proc_tmp, proc->write_len, "%7u"  "%7u"  "%7u"  "%7u"  "%7u"  "%7u"  "%7u"  "%7u"
        "%7u"  "%7u"  "%7u"  "%7u"  "%7u"  "%7u"  "%7u"  "%7u\n\n",
        (td_u16)shp_ctx->edge_gain_by_rly[0x0], (td_u16)shp_ctx->edge_gain_by_rly[0x1],
        (td_u16)shp_ctx->edge_gain_by_rly[0x2], (td_u16)shp_ctx->edge_gain_by_rly[0x3],
        (td_u16)shp_ctx->edge_gain_by_rly[0x4], (td_u16)shp_ctx->edge_gain_by_rly[0x5],
        (td_u16)shp_ctx->edge_gain_by_rly[0x6], (td_u16)shp_ctx->edge_gain_by_rly[0x7],
        (td_u16)shp_ctx->edge_gain_by_rly[0x8], (td_u16)shp_ctx->edge_gain_by_rly[0x9],
        (td_u16)shp_ctx->edge_gain_by_rly[0xa], (td_u16)shp_ctx->edge_gain_by_rly[0xb],
        (td_u16)shp_ctx->edge_gain_by_rly[0xc], (td_u16)shp_ctx->edge_gain_by_rly[0xd],
        (td_u16)shp_ctx->edge_gain_by_rly[0xe], (td_u16)shp_ctx->edge_gain_by_rly[0xf]);

    isp_proc_printf(&proc_tmp, proc->write_len, "%s"  "(%d"  "-"  "%d):\n", "edge_rly_by_mot", 0, 0xf);
    isp_proc_printf(&proc_tmp, proc->write_len, "%7u"  "%7u"  "%7u"  "%7u"  "%7u"  "%7u"  "%7u"  "%7u"
        "%7u"  "%7u"  "%7u"  "%7u"  "%7u"  "%7u"  "%7u"  "%7u\n\n",
        (td_u16)shp_ctx->edge_rly_by_mot[0x0], (td_u16)shp_ctx->edge_rly_by_mot[0x1],
        (td_u16)shp_ctx->edge_rly_by_mot[0x2], (td_u16)shp_ctx->edge_rly_by_mot[0x3],
        (td_u16)shp_ctx->edge_rly_by_mot[0x4], (td_u16)shp_ctx->edge_rly_by_mot[0x5],
        (td_u16)shp_ctx->edge_rly_by_mot[0x6], (td_u16)shp_ctx->edge_rly_by_mot[0x7],
        (td_u16)shp_ctx->edge_rly_by_mot[0x8], (td_u16)shp_ctx->edge_rly_by_mot[0x9],
        (td_u16)shp_ctx->edge_rly_by_mot[0xa], (td_u16)shp_ctx->edge_rly_by_mot[0xb],
        (td_u16)shp_ctx->edge_rly_by_mot[0xc], (td_u16)shp_ctx->edge_rly_by_mot[0xd],
        (td_u16)shp_ctx->edge_rly_by_mot[0xe], (td_u16)shp_ctx->edge_rly_by_mot[0xf]);

    isp_proc_printf(&proc_tmp, proc->write_len, "%s"  "(%d"  "-"  "%d):\n", "edge_rly_by_luma", 0, 0xf);
    isp_proc_printf(&proc_tmp, proc->write_len, "%7u"  "%7u"  "%7u"  "%7u"  "%7u"  "%7u"  "%7u"  "%7u"
        "%7u"  "%7u"  "%7u"  "%7u"  "%7u"  "%7u"  "%7u"  "%7u\n\n",
        (td_u16)shp_ctx->edge_rly_by_luma[0x0], (td_u16)shp_ctx->edge_rly_by_luma[0x1],
        (td_u16)shp_ctx->edge_rly_by_luma[0x2], (td_u16)shp_ctx->edge_rly_by_luma[0x3],
        (td_u16)shp_ctx->edge_rly_by_luma[0x4], (td_u16)shp_ctx->edge_rly_by_luma[0x5],
        (td_u16)shp_ctx->edge_rly_by_luma[0x6], (td_u16)shp_ctx->edge_rly_by_luma[0x7],
        (td_u16)shp_ctx->edge_rly_by_luma[0x8], (td_u16)shp_ctx->edge_rly_by_luma[0x9],
        (td_u16)shp_ctx->edge_rly_by_luma[0xa], (td_u16)shp_ctx->edge_rly_by_luma[0xb],
        (td_u16)shp_ctx->edge_rly_by_luma[0xc], (td_u16)shp_ctx->edge_rly_by_luma[0xd],
        (td_u16)shp_ctx->edge_rly_by_luma[0xe], (td_u16)shp_ctx->edge_rly_by_luma[0xf]);

    isp_proc_printf(&proc_tmp, proc->write_len, "%s"  "(%d"  "-"  "%d):\n", "mf_gain_by_mot", 0, 0xf);
    isp_proc_printf(&proc_tmp, proc->write_len, "%7u"  "%7u"  "%7u"  "%7u"  "%7u"  "%7u"  "%7u"  "%7u"
        "%7u"  "%7u"  "%7u"  "%7u"  "%7u"  "%7u"  "%7u"  "%7u\n\n",
        (td_u16)shp_ctx->mf_gain_by_mot[0x0], (td_u16)shp_ctx->mf_gain_by_mot[0x1],
        (td_u16)shp_ctx->mf_gain_by_mot[0x2], (td_u16)shp_ctx->mf_gain_by_mot[0x3],
        (td_u16)shp_ctx->mf_gain_by_mot[0x4], (td_u16)shp_ctx->mf_gain_by_mot[0x5],
        (td_u16)shp_ctx->mf_gain_by_mot[0x6], (td_u16)shp_ctx->mf_gain_by_mot[0x7],
        (td_u16)shp_ctx->mf_gain_by_mot[0x8], (td_u16)shp_ctx->mf_gain_by_mot[0x9],
        (td_u16)shp_ctx->mf_gain_by_mot[0xa], (td_u16)shp_ctx->mf_gain_by_mot[0xb],
        (td_u16)shp_ctx->mf_gain_by_mot[0xc], (td_u16)shp_ctx->mf_gain_by_mot[0xd],
        (td_u16)shp_ctx->mf_gain_by_mot[0xe], (td_u16)shp_ctx->mf_gain_by_mot[0xf]);

    isp_proc_printf(&proc_tmp, proc->write_len, "%s"  "(%d"  "-"  "%d):\n", "hf_gain_by_mot", 0, 0xf);
    isp_proc_printf(&proc_tmp, proc->write_len, "%7u"  "%7u"  "%7u"  "%7u"  "%7u"  "%7u"  "%7u"  "%7u"
        "%7u"  "%7u"  "%7u"  "%7u"  "%7u"  "%7u"  "%7u"  "%7u\n\n",
        (td_u16)shp_ctx->hf_gain_by_mot[0x0], (td_u16)shp_ctx->hf_gain_by_mot[0x1],
        (td_u16)shp_ctx->hf_gain_by_mot[0x2], (td_u16)shp_ctx->hf_gain_by_mot[0x3],
        (td_u16)shp_ctx->hf_gain_by_mot[0x4], (td_u16)shp_ctx->hf_gain_by_mot[0x5],
        (td_u16)shp_ctx->hf_gain_by_mot[0x6], (td_u16)shp_ctx->hf_gain_by_mot[0x7],
        (td_u16)shp_ctx->hf_gain_by_mot[0x8], (td_u16)shp_ctx->hf_gain_by_mot[0x9],
        (td_u16)shp_ctx->hf_gain_by_mot[0xa], (td_u16)shp_ctx->hf_gain_by_mot[0xb],
        (td_u16)shp_ctx->hf_gain_by_mot[0xc], (td_u16)shp_ctx->hf_gain_by_mot[0xd],
        (td_u16)shp_ctx->hf_gain_by_mot[0xe], (td_u16)shp_ctx->hf_gain_by_mot[0xf]);

    isp_proc_printf(&proc_tmp, proc->write_len, "%s"  "(%d"  "-"  "%d):\n", "lmf_gain_by_mot", 0, 0xf);
    isp_proc_printf(&proc_tmp, proc->write_len, "%7u"  "%7u"  "%7u"  "%7u"  "%7u"  "%7u"  "%7u"  "%7u"
        "%7u"  "%7u"  "%7u"  "%7u"  "%7u"  "%7u"  "%7u"  "%7u\n\n",
        (td_u16)shp_ctx->lmf_gain_by_mot[0x0], (td_u16)shp_ctx->lmf_gain_by_mot[0x1],
        (td_u16)shp_ctx->lmf_gain_by_mot[0x2], (td_u16)shp_ctx->lmf_gain_by_mot[0x3],
        (td_u16)shp_ctx->lmf_gain_by_mot[0x4], (td_u16)shp_ctx->lmf_gain_by_mot[0x5],
        (td_u16)shp_ctx->lmf_gain_by_mot[0x6], (td_u16)shp_ctx->lmf_gain_by_mot[0x7],
        (td_u16)shp_ctx->lmf_gain_by_mot[0x8], (td_u16)shp_ctx->lmf_gain_by_mot[0x9],
        (td_u16)shp_ctx->lmf_gain_by_mot[0xa], (td_u16)shp_ctx->lmf_gain_by_mot[0xb],
        (td_u16)shp_ctx->lmf_gain_by_mot[0xc], (td_u16)shp_ctx->lmf_gain_by_mot[0xd],
        (td_u16)shp_ctx->lmf_gain_by_mot[0xe], (td_u16)shp_ctx->lmf_gain_by_mot[0xf]);

    isp_proc_printf(&proc_tmp, proc->write_len,
                    "%12s" "%12s" "%12s" "%12s" "%16s" "%12s \n",
                    "texture_freq", "edge_freq", "over_shoot", "under_shoot", "shoot_sup_str", "detail_ctrl");

    isp_proc_printf(&proc_tmp, proc->write_len,
                    "%12u"  "%12u"  "%12u"  "%12u"  "%16u"  "%12u\n\n",
                    (td_u16)shp_ctx->texture_freq, (td_u16)shp_ctx->edge_freq, (td_u16)shp_ctx->over_shoot,
                    (td_u16)shp_ctx->under_shoot, (td_u16)shp_ctx->shoot_sup_str, (td_u16)shp_ctx->detail_ctrl);

    isp_proc_printf(&proc_tmp, proc->write_len,
                    "%14s" "%20s" "%8s" "%8s" "%8s" "%12s\n",
                    "edge_filt_str", "edge_filt_max_cap", "r_gain", "g_gain", "b_gain", "skin_gain");

    isp_proc_printf(&proc_tmp, proc->write_len,
                    "%14u"  "%20u"  "%8u" "%8u"  "%8u" "%12u\n\n",
                    (td_u16)shp_ctx->edge_filt_str, (td_u16)shp_ctx->edge_filt_max_cap, (td_u16)shp_ctx->r_gain,
                    (td_u16)shp_ctx->g_gain, (td_u16)shp_ctx->b_gain, (td_u16)shp_ctx->skin_gain);

    isp_proc_printf(&proc_tmp, proc->write_len,
                    "%14s" "%18s" "%16s" "%12s" "%12s" "%12s"  "%12s\n",
                    "shoot_sup_adj", "detail_ctrl_thr", "max_sharp_gain", "skin_umax",
                    "skin_umin", "skin_vmax", "skin_vmin");

    isp_proc_printf(&proc_tmp, proc->write_len,
                    "%14u"  "%18u"  "%16u"  "%12u"  "%12u"  "%12u"  "%12u\n\n",
                    (td_u16)shp_ctx->shoot_sup_adj, (td_u16)shp_ctx->detail_ctrl_thr, (td_u16)shp_ctx->max_sharp_gain,
                    (td_u16)shp_ctx->skin_umax, (td_u16)shp_ctx->skin_umin, (td_u16)shp_ctx->skin_vmax,
                    (td_u16)shp_ctx->skin_vmin);
    isp_proc_printf(&proc_tmp, proc->write_len,
                    "%22s" "%22s" "%24s \n",
                    "shoot_inner_threshold", "shoot_outer_threshold", "shoot_protect_threshold");

    isp_proc_printf(&proc_tmp, proc->write_len, "%22u" "%22u" "%24u \n\n", (td_u16)shp_ctx->shoot_inner_threshold,
                    (td_u16)shp_ctx->shoot_outer_threshold, (td_u16)shp_ctx->shoot_protect_threshold);

    isp_proc_printf(&proc_tmp, proc->write_len, "%24s" "%26s" "%16s" "%18s \n", "edge_rly_fine_threshold",
                    "edge_rly_coarse_threshold", "edge_over_shoot", "edge_under_shoot");

    isp_proc_printf(&proc_tmp, proc->write_len, "%24u" "%26u" "%16u" "%18u \n\n",
                    (td_u16)shp_ctx->edge_rly_fine_threshold, (td_u16)shp_ctx->edge_rly_coarse_threshold,
                    (td_u16)shp_ctx->edge_overshoot, (td_u16)shp_ctx->edge_undershoot);

    proc->write_len += 1;

    return TD_SUCCESS;
}

static td_void sharpen_iso_update(ot_vi_pipe vi_pipe, isp_reg_cfg *reg_cfg)
{
    td_u8  i;

    isp_usr_ctx     *isp_ctx  = TD_NULL;
    isp_sharpen_ctx *sharpen = TD_NULL;

    isp_get_ctx(vi_pipe, isp_ctx);
    sharpen_get_ctx(vi_pipe, sharpen);

    if ((sharpen->iso != sharpen->iso_last) ||
        (isp_ctx->linkage.fswdr_mode != isp_ctx->linkage.pre_fswdr_mode)) { /* will not work if ISO is the same */
        isp_sharpen_get_default_reg_cfg(vi_pipe);
        for (i = 0; i < reg_cfg->cfg_num; i++) {
            reg_cfg->alg_reg_cfg[i].sharpen_reg_cfg.dyna_reg_cfg.default_dyna_reg_cfg.resh = TD_TRUE;
        }
    }

    if (sharpen->sharpen_mpi_update_en) {
        if (sharpen->manual_sharpen_yuv_enabled == OT_OP_MODE_AUTO) { /* auto mode */
            isp_sharpen_get_mpi_reg_cfg(sharpen);
        }
        for (i = 0; i < reg_cfg->cfg_num; i++) {
            reg_cfg->alg_reg_cfg[i].sharpen_reg_cfg.dyna_reg_cfg.mpi_dyna_reg_cfg.resh = TD_TRUE;
            reg_cfg->alg_reg_cfg[i].sharpen_reg_cfg.dyna_reg_cfg.mpi_dyna_reg_cfg.update_index += 1;
            sharpen_mpi2reg(&(reg_cfg->alg_reg_cfg[i].sharpen_reg_cfg), sharpen);
        }
    } else if (sharpen->iso != sharpen->iso_last) {
        if (sharpen->manual_sharpen_yuv_enabled == OT_OP_MODE_AUTO) { /* auto mode */
            isp_sharpen_get_mpi_reg_cfg(sharpen);
            for (i = 0; i < reg_cfg->cfg_num; i++) {
                reg_cfg->alg_reg_cfg[i].sharpen_reg_cfg.dyna_reg_cfg.mpi_dyna_reg_cfg.resh = TD_TRUE;
                reg_cfg->alg_reg_cfg[i].sharpen_reg_cfg.dyna_reg_cfg.mpi_dyna_reg_cfg.update_index += 1;
                sharpen_mpi2reg(&(reg_cfg->alg_reg_cfg[i].sharpen_reg_cfg), sharpen);
            }
        } else {
            for (i = 0; i < reg_cfg->cfg_num; i++) {
                sharpen_mpi2reg(&(reg_cfg->alg_reg_cfg[i].sharpen_reg_cfg), sharpen);
            }
        }
    }

    sharpen->iso_last = sharpen->iso;    /* will not work if ISO is the same */
}

static td_s32 isp_sharpen_run(ot_vi_pipe vi_pipe, const td_void *stat_info, td_void *reg_cfg_info, td_s32 rsv)
{
    td_u8  i;
    td_u8  sft = 0x8;
    td_u32 iso_lvl_cur, iso_lvl_pre;

    ot_isp_alg_mod alg_mod = OT_ISP_ALG_SHARPEN;
    isp_usr_ctx     *isp_ctx = TD_NULL;
    isp_sharpen_ctx *sharpen = TD_NULL;
    isp_reg_cfg *reg_cfg = (isp_reg_cfg *)reg_cfg_info;
    ot_unused(stat_info);
    ot_unused(rsv);

    isp_get_ctx(vi_pipe, isp_ctx);
    isp_alg_check_return(isp_ctx->alg_key.bit1_sharpen);
    sharpen_get_ctx(vi_pipe, sharpen);
    isp_check_pointer_return(sharpen);

    if (isp_ctx->linkage.defect_pixel) {
        return TD_SUCCESS;
    }

    ot_ext_system_isp_sharpen_init_status_write(vi_pipe, sharpen->init);
    if (sharpen->init != TD_TRUE) {
        ioctl(isp_get_fd(vi_pipe), ISP_ALG_INIT_ERR_INFO_PRINT, &alg_mod);
        return TD_SUCCESS;
    }

    sharpen->sharpen_en = ot_ext_system_manual_sharpen_en_read(vi_pipe);

    for (i = 0; i < reg_cfg->cfg_num; i++) {
        reg_cfg->alg_reg_cfg[i].sharpen_reg_cfg.enable = sharpen->sharpen_en;
    }

    reg_cfg->cfg_key.bit1_sharpen_cfg = 1;

    /* check hardware setting */
    if (!check_sharpen_open(sharpen)) {
        return TD_SUCCESS;
    }

    /* sharpen strength linkage with the iso calculated by ae */
    sharpen->sft = sft;
    sharpen->iso = isp_ctx->linkage.iso;
    sharpen->idx_cur = get_iso_index(sharpen->iso);
    sharpen->idx_pre = (sharpen->idx_cur == 0) ? 0 : MAX2(sharpen->idx_cur - 1, 0);

    iso_lvl_cur = get_iso((td_u8)sharpen->idx_cur);
    iso_lvl_pre = get_iso((td_u8)sharpen->idx_pre);
    if (sharpen->iso <= (td_u32)iso_lvl_pre) {
        sharpen->wgt_pre = (td_u32)signed_left_shift(1, sft);
    } else if (sharpen->iso >= (td_u32)iso_lvl_cur) {
        sharpen->wgt_pre = 0;
    } else {
        sharpen->wgt_pre = ((td_u32)signed_left_shift((iso_lvl_cur - sharpen->iso), sft) / (iso_lvl_cur - iso_lvl_pre));
    }
    sharpen->wgt_cur = (td_u32)signed_left_shift(1, sft) - sharpen->wgt_pre;

    sharpen_read_extregs(vi_pipe);
    sharpen_read_pro_mode(vi_pipe);
    sharpen_iso_update(vi_pipe, reg_cfg);
    sharpen_actual_update(vi_pipe, sharpen);

    return TD_SUCCESS;
}

static td_s32 isp_sharpen_ctrl(ot_vi_pipe vi_pipe, td_u32 cmd, td_void *value)
{
    isp_reg_cfg_attr  *reg_cfg = TD_NULL;
    isp_usr_ctx     *isp_ctx = TD_NULL;
    isp_get_ctx(vi_pipe, isp_ctx);
    isp_alg_check_return(isp_ctx->alg_key.bit1_sharpen);

    isp_sharpen_ctx *shp_ctx = TD_NULL;

    switch (cmd) {
        case OT_ISP_WDR_MODE_SET:
            isp_regcfg_get_ctx(vi_pipe, reg_cfg);
            isp_check_pointer_return(reg_cfg);
            isp_sharpen_wdr_mode_set(vi_pipe, (td_void *)&reg_cfg->reg_cfg);
            break;
        case OT_ISP_PROC_WRITE:
            sharpen_get_ctx(vi_pipe, shp_ctx);
            isp_check_pointer_return(shp_ctx);
            sharpen_proc_write((ot_isp_ctrl_proc_write *)value, shp_ctx);
            break;
        default:
            break;
    }

    return TD_SUCCESS;
}

static td_s32 isp_sharpen_exit(ot_vi_pipe vi_pipe)
{
    td_u8 i;
    isp_reg_cfg_attr *reg_cfg   = TD_NULL;
    isp_usr_ctx *isp_ctx = TD_NULL;

    isp_get_ctx(vi_pipe, isp_ctx);
    isp_alg_check_return(isp_ctx->alg_key.bit1_sharpen);

    isp_regcfg_get_ctx(vi_pipe, reg_cfg);

    ot_ext_system_isp_sharpen_init_status_write(vi_pipe, TD_FALSE);
    for (i = 0; i < reg_cfg->reg_cfg.cfg_num; i++) {
        reg_cfg->reg_cfg.alg_reg_cfg[i].sharpen_reg_cfg.enable = TD_FALSE;
    }
    reg_cfg->reg_cfg.cfg_key.bit1_sharpen_cfg = 1;

    ot_ext_system_sharpen_manu_mode_write(vi_pipe, TD_FALSE);

    sharpen_ctx_exit(vi_pipe);

    return TD_SUCCESS;
}

td_s32 isp_alg_register_sharpen(ot_vi_pipe vi_pipe)
{
    td_s32 ret;
    isp_usr_ctx *isp_ctx = TD_NULL;
    isp_alg_node *algs = TD_NULL;

    isp_get_ctx(vi_pipe, isp_ctx);
    isp_alg_check_return(isp_ctx->alg_key.bit1_sharpen);
    algs = isp_search_alg(isp_ctx->algs);
    isp_check_pointer_return(algs);
    ret = strncpy_s(algs->alg_name, sizeof(algs->alg_name), "sharpen", sizeof("sharpen"));
    isp_check_eok_return(ret, TD_FAILURE);
    algs->alg_type = OT_ISP_ALG_SHARPEN;
    algs->alg_func.pfn_alg_init = isp_sharpen_init;
    algs->alg_func.pfn_alg_run  = isp_sharpen_run;
    algs->alg_func.pfn_alg_ctrl = isp_sharpen_ctrl;
    algs->alg_func.pfn_alg_exit = isp_sharpen_exit;
    algs->used = TD_TRUE;

    return TD_SUCCESS;
}
