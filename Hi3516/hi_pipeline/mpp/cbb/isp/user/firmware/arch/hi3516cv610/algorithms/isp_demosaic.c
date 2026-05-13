/*
  Copyright (c), 2001-2025, Shenshu Tech. Co., Ltd.
 */
#include "isp_demosaic.h"
#include "isp_alg.h"
#include "isp_sensor.h"
#include "isp_config.h"
#include "isp_ext_config.h"
#include "isp_proc.h"
#include "isp_math_utils.h"
#include "isp_param_check.h"
#include "isp_ext_reg_access.h"

#define OT_DEMOSAIC_BITDEPTH    12
#define BITFIX_NUM             10

static const  td_u8  g_nddm_str[OT_ISP_AUTO_ISO_NUM] = {
    64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64
};
static const  td_u8  g_nddm_mf_detailehc_str[OT_ISP_AUTO_ISO_NUM] = {
    32, 24, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16
};
static const  td_u8  g_hf_detailehc_str[OT_ISP_AUTO_ISO_NUM] = {
    0, 0, 3, 5, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7
};
static const  td_u8  g_detail_smooth_range[OT_ISP_AUTO_ISO_NUM] = {
    2, 2, 2, 2, 2, 3, 3, 4, 4, 5, 5, 6, 6, 7, 7, 7
};
static const  td_u8  g_color_noise_thdf[OT_ISP_AUTO_ISO_NUM] = {
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};
static const  td_u8  g_color_noise_strf[OT_ISP_AUTO_ISO_NUM] = {
    8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8
};
static const  td_u8  g_color_noise_thdy[OT_ISP_AUTO_ISO_NUM] = {
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1
};
static const  td_u8  g_color_noise_stry[OT_ISP_AUTO_ISO_NUM] = {
    10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10
};

/* Linear parameters */
static const td_u8   g_lut_cc_hf_max_ratio_lin[OT_ISP_AUTO_ISO_NUM] = {
    3, 4, 5, 6, 6, 7, 7, 8, 8, 8, 8, 8, 8, 8, 8, 8
};
static const td_u8   g_lut_cc_hf_min_ratio_lin[OT_ISP_AUTO_ISO_NUM] = {
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};
static const td_u16  g_lut_desat_low_lin[OT_ISP_AUTO_ISO_NUM]       = {
    166, 166, 166, 166, 166, 166, 166, 166, 166, 166, 166, 166, 166, 166, 166, 166
};
static const td_u16  g_lut_desat_prot_th_lin[OT_ISP_AUTO_ISO_NUM]   = {
    128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128
};
static const td_s8   g_lut_nddm_eps_sft_lin[OT_ISP_AUTO_ISO_NUM] = {
    -5, -4, -3, -2, -1, 0, 1, 2, 3, 3, 3, 3, 3, 3, 3, 3
};
static const td_u16  g_lut_filter_blur_th_low_lin[OT_ISP_AUTO_ISO_NUM]   = {
    240, 240, 240, 240, 240, 240, 240, 240, 240, 240, 240, 240, 240, 240, 240, 240
};
static const td_u16  g_lut_filter_blur_th_hig_lin[OT_ISP_AUTO_ISO_NUM]   = {
    240, 240, 240, 240, 240, 240, 240, 240, 240, 240, 240, 240, 240, 240, 240, 240
};
static const td_u16  g_lut_var_thr_for_ahd_lin[OT_ISP_AUTO_ISO_NUM]   = {
    4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4
};
static const td_u16  g_lut_var_thr_for_cac_lin[OT_ISP_AUTO_ISO_NUM]   = {
    6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6
};

/* wdr parameters */
static const td_u8   g_lut_cc_hf_max_ratio_wdr[OT_ISP_AUTO_ISO_NUM] = {
    8, 8, 8, 8, 10, 10, 10, 12, 12, 14, 14, 16, 16, 16, 16, 16
};
static const td_u8   g_lut_cc_hf_min_ratio_wdr[OT_ISP_AUTO_ISO_NUM] = {
    0, 0, 0, 0, 0, 0, 2, 2, 4, 4, 4, 8, 10, 10, 12, 14
};
static const td_u16  g_lut_desat_low_wdr[OT_ISP_AUTO_ISO_NUM]       = {
    166, 166, 166, 166, 166, 166, 166, 166, 166, 166, 166, 166, 166, 166, 166, 166
};
static const td_u16  g_lut_desat_prot_th_wdr[OT_ISP_AUTO_ISO_NUM]   = {
    128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128
};
static const td_s8   g_lut_nddm_eps_sft_wdr[OT_ISP_AUTO_ISO_NUM] = {
    -5, -4, -3, -2, -1, 0, 1, 2, 3, 3, 3, 3, 3, 3, 3, 3
};
static const td_u16  g_lut_filter_blur_th_low_wdr[OT_ISP_AUTO_ISO_NUM]   = {
    240, 240, 240, 240, 240, 240, 240, 240, 240, 240, 240, 240, 240, 240, 240, 240
};
static const td_u16  g_lut_filter_blur_th_hig_wdr[OT_ISP_AUTO_ISO_NUM]   = {
    240, 240, 240, 240, 240, 240, 240, 240, 240, 240, 240, 240, 240, 240, 240, 240
};
static const td_u16  g_lut_var_thr_for_ahd_wdr[OT_ISP_AUTO_ISO_NUM]   = {
    4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4
};
static const td_u16  g_lut_var_thr_for_cac_wdr[OT_ISP_AUTO_ISO_NUM]   = {
    6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6
};

static isp_demosaic *g_demosaic_ctx[OT_ISP_MAX_PIPE_NUM] = {TD_NULL};

#define demosaic_get_ctx(pipe, ctx)   ((ctx) = g_demosaic_ctx[pipe])
#define demosaic_set_ctx(pipe, ctx)   (g_demosaic_ctx[pipe] = (ctx))
#define demosaic_reset_ctx(pipe)      (g_demosaic_ctx[pipe] = TD_NULL)

static td_s32 demosaic_ctx_init(ot_vi_pipe vi_pipe)
{
    isp_demosaic *demosaic_ctx = TD_NULL;

    demosaic_get_ctx(vi_pipe, demosaic_ctx);

    if (demosaic_ctx == TD_NULL) {
        demosaic_ctx = (isp_demosaic *)isp_malloc(sizeof(isp_demosaic));
        if (demosaic_ctx == TD_NULL) {
            isp_err_trace("isp[%d] demosaic_ctx malloc memory failed!\n", vi_pipe);
            return OT_ERR_ISP_NOMEM;
        }
    }

    (td_void)memset_s(demosaic_ctx, sizeof(isp_demosaic), 0, sizeof(isp_demosaic));

    demosaic_set_ctx(vi_pipe, demosaic_ctx);

    return TD_SUCCESS;
}

static td_void demosaic_ctx_exit(ot_vi_pipe vi_pipe)
{
    isp_demosaic *demosaic_ctx = TD_NULL;

    demosaic_get_ctx(vi_pipe, demosaic_ctx);
    isp_free(demosaic_ctx);
    demosaic_reset_ctx(vi_pipe);
}

static td_void demosaic_init_fw_linear(ot_vi_pipe vi_pipe, isp_demosaic *demosaic)
{
    ot_unused(vi_pipe);
    (td_void)memcpy_s(demosaic->lut_cc_hf_max_ratio, sizeof(td_u8) * OT_ISP_AUTO_ISO_NUM,
                      g_lut_cc_hf_max_ratio_lin,     sizeof(td_u8) * OT_ISP_AUTO_ISO_NUM);
    (td_void)memcpy_s(demosaic->lut_cc_hf_min_ratio, sizeof(td_u8) * OT_ISP_AUTO_ISO_NUM,
                      g_lut_cc_hf_min_ratio_lin,     sizeof(td_u8) * OT_ISP_AUTO_ISO_NUM);
    (td_void)memcpy_s(demosaic->lut_desat_low,       sizeof(td_u16) * OT_ISP_AUTO_ISO_NUM,
                      g_lut_desat_low_lin,           sizeof(td_u16) * OT_ISP_AUTO_ISO_NUM);
    (td_void)memcpy_s(demosaic->lut_desat_prot_th,   sizeof(td_u16) * OT_ISP_AUTO_ISO_NUM,
                      g_lut_desat_prot_th_lin,          sizeof(td_u16) * OT_ISP_AUTO_ISO_NUM);
    (td_void)memcpy_s(demosaic->lut_nddm_eps_sft,       sizeof(td_s8) * OT_ISP_AUTO_ISO_NUM,
                      g_lut_nddm_eps_sft_lin,           sizeof(td_s8) * OT_ISP_AUTO_ISO_NUM);
    (td_void)memcpy_s(demosaic->lut_filter_blur_th_low, sizeof(td_u16) * OT_ISP_AUTO_ISO_NUM,
                      g_lut_filter_blur_th_low_lin,     sizeof(td_u16) * OT_ISP_AUTO_ISO_NUM);
    (td_void)memcpy_s(demosaic->lut_filter_blur_th_hig, sizeof(td_u16) * OT_ISP_AUTO_ISO_NUM,
                      g_lut_filter_blur_th_hig_lin,     sizeof(td_u16) * OT_ISP_AUTO_ISO_NUM);
    (td_void)memcpy_s(demosaic->lut_var_thr_for_ahd,    sizeof(td_u16) * OT_ISP_AUTO_ISO_NUM,
                      g_lut_var_thr_for_ahd_lin,        sizeof(td_u16) * OT_ISP_AUTO_ISO_NUM);
    (td_void)memcpy_s(demosaic->lut_var_thr_for_cac,    sizeof(td_u16) * OT_ISP_AUTO_ISO_NUM,
                      g_lut_var_thr_for_cac_lin,        sizeof(td_u16) * OT_ISP_AUTO_ISO_NUM);
}

static td_void demosaic_init_fw_wdr(ot_vi_pipe vi_pipe, isp_demosaic *demosaic)
{
    ot_unused(vi_pipe);
    (td_void)memcpy_s(demosaic->lut_cc_hf_max_ratio, sizeof(td_u8) * OT_ISP_AUTO_ISO_NUM,
                      g_lut_cc_hf_max_ratio_wdr,     sizeof(td_u8) * OT_ISP_AUTO_ISO_NUM);
    (td_void)memcpy_s(demosaic->lut_cc_hf_min_ratio, sizeof(td_u8) * OT_ISP_AUTO_ISO_NUM,
                      g_lut_cc_hf_min_ratio_wdr,     sizeof(td_u8) * OT_ISP_AUTO_ISO_NUM);
    (td_void)memcpy_s(demosaic->lut_desat_low,       sizeof(td_u16) * OT_ISP_AUTO_ISO_NUM,
                      g_lut_desat_low_wdr,           sizeof(td_u16) * OT_ISP_AUTO_ISO_NUM);
    (td_void)memcpy_s(demosaic->lut_desat_prot_th,   sizeof(td_u16) * OT_ISP_AUTO_ISO_NUM,
                      g_lut_desat_prot_th_wdr,          sizeof(td_u16) * OT_ISP_AUTO_ISO_NUM);
    (td_void)memcpy_s(demosaic->lut_nddm_eps_sft,       sizeof(td_s8) * OT_ISP_AUTO_ISO_NUM,
                      g_lut_nddm_eps_sft_wdr,           sizeof(td_s8) * OT_ISP_AUTO_ISO_NUM);
    (td_void)memcpy_s(demosaic->lut_filter_blur_th_low, sizeof(td_u16) * OT_ISP_AUTO_ISO_NUM,
                      g_lut_filter_blur_th_low_wdr,     sizeof(td_u16) * OT_ISP_AUTO_ISO_NUM);
    (td_void)memcpy_s(demosaic->lut_filter_blur_th_hig, sizeof(td_u16) * OT_ISP_AUTO_ISO_NUM,
                      g_lut_filter_blur_th_hig_wdr,     sizeof(td_u16) * OT_ISP_AUTO_ISO_NUM);
    (td_void)memcpy_s(demosaic->lut_var_thr_for_ahd,    sizeof(td_u16) * OT_ISP_AUTO_ISO_NUM,
                      g_lut_var_thr_for_ahd_wdr,        sizeof(td_u16) * OT_ISP_AUTO_ISO_NUM);
    (td_void)memcpy_s(demosaic->lut_var_thr_for_cac,    sizeof(td_u16) * OT_ISP_AUTO_ISO_NUM,
                      g_lut_var_thr_for_cac_wdr,        sizeof(td_u16) * OT_ISP_AUTO_ISO_NUM);
}


static td_void demosaic_static_regs_initialize(ot_vi_pipe vi_pipe, isp_demosaic_static_cfg *static_reg_cfg, td_u32 i)
{
    ot_unused(vi_pipe);
    ot_unused(i);
    static_reg_cfg->de_sat_enable      = OT_ISP_DEMOSAIC_DESAT_ENABLE_DEFAULT;
    static_reg_cfg->g_clip_bit_sft     = OT_ISP_DEMOSAIC_G_CLIP_SFT_BIT_DEFAULT;
    static_reg_cfg->hv_blend_limit1    = OT_ISP_DEMOSAIC_BLENDLIMIT1_DEFAULT;
    static_reg_cfg->hv_blend_limit2    = OT_ISP_DEMOSAIC_BLENDLIMIT2_DEFAULT;
    static_reg_cfg->hv_grad_limit1     = OT_ISP_DEMOSAIC_GRADLIMIT1_DEFAULT;
    static_reg_cfg->hv_grad_limit2     = OT_ISP_DEMOSAIC_GRADLIMIT2_DEFAULT;
    static_reg_cfg->hv_color_ratio     = OT_ISP_DEMOSAIC_HV_COLOR_RATIO_DEFAULT;
    static_reg_cfg->hv_detg_ratio      = OT_ISP_DEMOSAIC_HV_DETG_RATIO_DEFAULT;
    static_reg_cfg->wgd_limit1         = OT_ISP_DEMOSAIC_WGDLIMIT1_DEFAULT;
    static_reg_cfg->wgd_limit2         = OT_ISP_DEMOSAIC_WGDLIMIT2_DEFAULT;
    static_reg_cfg->cx_var_max_rate    = OT_ISP_DEMOSAIC_CX_VAR_MAX_RATE_DEFAULT;
    static_reg_cfg->cx_var_min_rate    = OT_ISP_DEMOSAIC_CX_VAR_MIN_RATE_DEFAULT;
    static_reg_cfg->de_sat_thresh1     = OT_ISP_DEMOSAIC_DESAT_THRESH1_DEFAULT;
    static_reg_cfg->de_sat_thresh2     = OT_ISP_DEMOSAIC_DESAT_THRESH2_DEFAULT;
    static_reg_cfg->de_sat_hig         = OT_ISP_DEMOSAIC_DESAT_HIG_DEFAULT;
    static_reg_cfg->de_sat_prot_sl     = OT_ISP_DEMOSAIC_DESAT_PROTECT_SL_DEFAULT;
    static_reg_cfg->ahd_enable         = OT_ISP_DEMOSAIC_AHD_ENABLE_DEFAULT;
    static_reg_cfg->ahd_part1          = OT_ISP_DEMOSAIC_AHDPART1_DEFAULT;
    static_reg_cfg->ahd_part2          = OT_ISP_DEMOSAIC_AHDPART2_DEFAULT;

    static_reg_cfg->buf_id = 0;
    static_reg_cfg->resh = TD_TRUE;
}

static td_void demosaic_dyna_regs_initialize(isp_demosaic_dyna_cfg *dyna_reg_cfg)
{
    dyna_reg_cfg->cc_hf_max_ratio      = OT_ISP_DEMOSAIC_CC_HF_MAX_RATIO_DEFAULT;
    dyna_reg_cfg->cc_hf_min_ratio      = OT_ISP_DEMOSAIC_CC_HF_MIN_RATIO_DEFAULT;
    dyna_reg_cfg->lpff0                = OT_ISP_DEMOSAIC_LPF_F0_DEFAULT;
    dyna_reg_cfg->lpff3                = OT_ISP_DEMOSAIC_LPF_F3_DEFAULT;
    dyna_reg_cfg->nddmstr              = OT_ISP_DEMOSAIC_NDDM_NDDMSTR_DEFAULT;
    dyna_reg_cfg->ehc_gray             = OT_ISP_DEMOSAIC_NDDM_EHCGRAY_DEFAULT;
    dyna_reg_cfg->nddm_eps_sft         = OT_ISP_DEMOSAIC_NDDM_EPS_SFT_DEFAULT;
    dyna_reg_cfg->nddm_mode            = OT_ISP_DEMOSAIC_NDDM_MODE_DEFAULT;
    dyna_reg_cfg->de_sat_low           = OT_ISP_DEMOSAIC_DESAT_LOW_DEFAULT;
    dyna_reg_cfg->de_sat_ratio         = OT_ISP_DEMOSAIC_DESAT_RATIO_DEFAULT;
    dyna_reg_cfg->de_sat_prot_th       = OT_ISP_DEMOSAIC_DESAT_PROTECT_TH_DEFAULT;
    dyna_reg_cfg->filter_blur_th_low   = OT_ISP_DEMOSAIC_FILTER_BLUR_TH_LOW_DEFAULT;
    dyna_reg_cfg->filter_blur_th_hig   = OT_ISP_DEMOSAIC_FILTER_BLUR_TH_HIG_DEFAULT;
    dyna_reg_cfg->var_thr_for_ahd      = OT_ISP_DEMOSAIC_VAR_THR_FOR_AHD_DEFAULT;
    dyna_reg_cfg->var_thr_for_cac      = OT_ISP_DEMOSAIC_VAR_THR_FOR_CAC_DEFAULT;

    dyna_reg_cfg->update_gf = TD_TRUE;
    dyna_reg_cfg->resh      = TD_TRUE;
}

static td_void demosaic_regs_initialize(ot_vi_pipe vi_pipe, isp_reg_cfg  *reg_cfg, isp_demosaic *demosaic_ctx)
{
    td_u32 i;
    isp_demosaic_static_cfg *static_reg_cfg = TD_NULL;
    isp_demosaic_dyna_cfg   *dyna_reg_cfg   = TD_NULL;

    for (i = 0; i < reg_cfg->cfg_num; i++) {
        static_reg_cfg = &reg_cfg->alg_reg_cfg[i].dem_reg_cfg.static_reg_cfg;
        dyna_reg_cfg   = &reg_cfg->alg_reg_cfg[i].dem_reg_cfg.dyna_reg_cfg;
        reg_cfg->alg_reg_cfg[i].dem_reg_cfg.vhdm_enable = demosaic_ctx->mpi_cfg.enable;
        reg_cfg->alg_reg_cfg[i].dem_reg_cfg.nddm_enable = demosaic_ctx->mpi_cfg.enable;
        reg_cfg->alg_reg_cfg[i].dem_reg_cfg.inner_enable = demosaic_ctx->mpi_cfg.enable;
        demosaic_static_regs_initialize(vi_pipe, static_reg_cfg, i);
        demosaic_dyna_regs_initialize(dyna_reg_cfg);
    }

    reg_cfg->cfg_key.bit1_dem_cfg = 1;
}

static td_s32 demosaic_check_cmos_param(const ot_isp_demosaic_attr *cmos_demosaic)
{
    td_s32 ret;
    isp_check_bool_return(cmos_demosaic->enable);
    ret = isp_demosaic_attr_check("cmos", cmos_demosaic);
    if (ret != TD_SUCCESS) {
        return ret;
    }
    return TD_SUCCESS;
}

static td_s32 demosaic_update_from_sns(ot_isp_cmos_default *sns, isp_demosaic *demosaic)
{
    td_s32 ret;

    isp_check_pointer_return(sns->demosaic);
    ret = demosaic_check_cmos_param(sns->demosaic);
    if (ret != TD_SUCCESS) {
        return ret;
    }

    (td_void)memcpy_s(&demosaic->mpi_cfg, sizeof(ot_isp_demosaic_attr), sns->demosaic, sizeof(ot_isp_demosaic_attr));

    return TD_SUCCESS;
}

static td_void demosaic_update_from_default(isp_demosaic *demosaic)
{
    demosaic->mpi_cfg.enable  = TD_TRUE;
    demosaic->mpi_cfg.op_type = OT_OP_MODE_AUTO;
    demosaic->mpi_cfg.ai_detail_strength = OT_EXT_SYSTEM_DEMOSAIC_AI_DETAIL_STRENGTH_DEFAULT;

    if (is_wdr_mode(demosaic->wdr_mode)) {    /* Manual:WDR mode */
        demosaic->mpi_cfg.manual_attr.nddm_strength           = OT_EXT_SYSTEM_DEMOSAIC_MANUAL_NDDM_STR_WDRDFT;
        demosaic->mpi_cfg.manual_attr.nddm_mf_detail_strength = OT_EXT_SYSTEM_DEMOSAIC_MANUAL_NDDM_MF_EHNC_STR_WDRDFT;
        demosaic->mpi_cfg.manual_attr.hf_detail_strength      = OT_EXT_SYSTEM_DEMOSAIC_MANUAL_HF_EHNC_STR_WDRDFT;
        demosaic->mpi_cfg.manual_attr.detail_smooth_range     = OT_EXT_SYSTEM_DEMOSAIC_MANUAL_DTLSMOOTH_RANGE_WDRDFT;
    } else {   /* Manual:Linear Mode */
        demosaic->mpi_cfg.manual_attr.nddm_strength           = OT_EXT_SYSTEM_DEMOSAIC_MANUAL_NDDM_STR_LINDFT;
        demosaic->mpi_cfg.manual_attr.nddm_mf_detail_strength = OT_EXT_SYSTEM_DEMOSAIC_MANUAL_NDDM_MF_EHNC_STR_LINDFT;
        demosaic->mpi_cfg.manual_attr.hf_detail_strength      = OT_EXT_SYSTEM_DEMOSAIC_MANUAL_HF_EHNC_STR_LINDFT;
        demosaic->mpi_cfg.manual_attr.detail_smooth_range     = OT_EXT_SYSTEM_DEMOSAIC_MANUAL_DTLSMOOTH_RANGE_LINDFT;
    }

    demosaic->mpi_cfg.manual_attr.color_noise_f_threshold = OT_EXT_SYSTEM_DEMOSAIC_COLORNOISE_CTRL_THDF_DEFAULT;
    demosaic->mpi_cfg.manual_attr.color_noise_f_strength  = OT_EXT_SYSTEM_DEMOSAIC_COLORNOISE_CTRL_STRF_DEFAULT;
    demosaic->mpi_cfg.manual_attr.color_noise_y_threshold = OT_EXT_SYSTEM_DEMOSAIC_MANUAL_DESAT_DARK_RANGE_DEFAULT;
    demosaic->mpi_cfg.manual_attr.color_noise_y_strength  = OT_EXT_SYSTEM_DEMOSAIC_MANUAL_DESAT_DARK_STRENGTH_DEFAULT;

    (td_void)memcpy_s(demosaic->mpi_cfg.auto_attr.nddm_strength, OT_ISP_AUTO_ISO_NUM * sizeof(td_u8),
                      g_nddm_str, OT_ISP_AUTO_ISO_NUM * sizeof(td_u8));
    (td_void)memcpy_s(demosaic->mpi_cfg.auto_attr.nddm_mf_detail_strength, OT_ISP_AUTO_ISO_NUM * sizeof(td_u8),
                      g_nddm_mf_detailehc_str, OT_ISP_AUTO_ISO_NUM * sizeof(td_u8));
    (td_void)memcpy_s(demosaic->mpi_cfg.auto_attr.hf_detail_strength, OT_ISP_AUTO_ISO_NUM * sizeof(td_u8),
                      g_hf_detailehc_str, OT_ISP_AUTO_ISO_NUM * sizeof(td_u8));
    (td_void)memcpy_s(demosaic->mpi_cfg.auto_attr.detail_smooth_range, OT_ISP_AUTO_ISO_NUM * sizeof(td_u8),
                      g_detail_smooth_range, OT_ISP_AUTO_ISO_NUM * sizeof(td_u8));
    (td_void)memcpy_s(demosaic->mpi_cfg.auto_attr.color_noise_f_threshold, OT_ISP_AUTO_ISO_NUM * sizeof(td_u8),
                      g_color_noise_thdf, OT_ISP_AUTO_ISO_NUM * sizeof(td_u8));
    (td_void)memcpy_s(demosaic->mpi_cfg.auto_attr.color_noise_f_strength,  OT_ISP_AUTO_ISO_NUM * sizeof(td_u8),
                      g_color_noise_strf, OT_ISP_AUTO_ISO_NUM * sizeof(td_u8));
    (td_void)memcpy_s(demosaic->mpi_cfg.auto_attr.color_noise_y_threshold, OT_ISP_AUTO_ISO_NUM * sizeof(td_u8),
                      g_color_noise_thdy, OT_ISP_AUTO_ISO_NUM * sizeof(td_u8));
    (td_void)memcpy_s(demosaic->mpi_cfg.auto_attr.color_noise_y_strength, OT_ISP_AUTO_ISO_NUM * sizeof(td_u8),
                      g_color_noise_stry, OT_ISP_AUTO_ISO_NUM * sizeof(td_u8));
}


static td_s32 demosaic_update(ot_isp_cmos_default *sns_dft, isp_demosaic *demosaic)
{
    td_s32 ret;
    if (sns_dft->key.bit1_demosaic) {
        ret = demosaic_update_from_sns(sns_dft, demosaic);
        if (ret != TD_SUCCESS) {
            return ret;
        }
    } else {
        demosaic_update_from_default(demosaic);
    }

    return TD_SUCCESS;
}

static td_s32 isp_demosaic_ctx_initialize(ot_vi_pipe vi_pipe, isp_demosaic *demosaic)
{
    td_s32 ret;

    ot_isp_cmos_default *sns_dft = TD_NULL;
    isp_usr_ctx *isp_ctx = TD_NULL;

    isp_get_ctx(vi_pipe, isp_ctx);
    isp_sensor_get_default(vi_pipe, &sns_dft);

    isp_check_pointer_return(sns_dft);

    demosaic->wdr_mode = isp_ctx->sns_wdr_mode;
    demosaic->bit_depth_prc = OT_DEMOSAIC_BITDEPTH;

    if (demosaic->wdr_mode != 0) {
        demosaic->nddm_str = OT_EXT_SYSTEM_DEMOSAIC_MANUAL_NDDM_STR_WDRDFT;
        demosaic_init_fw_wdr(vi_pipe, demosaic);
    } else {
        demosaic->nddm_str = OT_EXT_SYSTEM_DEMOSAIC_MANUAL_NDDM_STR_LINDFT;
        demosaic_init_fw_linear(vi_pipe, demosaic);
    }
    /* ctx loading from sns */
    ret = demosaic_update(sns_dft, demosaic);
    if (ret != TD_SUCCESS) {
        return ret;
    }

    return TD_SUCCESS;
}

static td_void demosaic_ext_regs_initialize(ot_vi_pipe vi_pipe, isp_demosaic *demosaic)
{
    isp_demosaic_attr_write(vi_pipe, &demosaic->mpi_cfg);

    return;
}

td_void isp_demosaic_set_long_frame_mode(ot_vi_pipe vi_pipe, isp_demosaic *demosaic)
{
    isp_usr_ctx   *isp_ctx  = TD_NULL;

    isp_get_ctx(vi_pipe, isp_ctx);

    if (isp_ctx->linkage.fswdr_mode == isp_ctx->linkage.pre_fswdr_mode) {
        return;
    }

    if (is_linear_mode(isp_ctx->sns_wdr_mode) ||
        (isp_ctx->linkage.fswdr_mode == OT_ISP_FSWDR_LONG_FRAME_MODE) ||
        (isp_ctx->linkage.fswdr_mode == OT_ISP_FSWDR_AUTO_LONG_FRAME_MODE)) {
        demosaic_init_fw_linear(vi_pipe, demosaic);
    } else {
        demosaic_init_fw_wdr(vi_pipe, demosaic);
    }
}

td_void isp_demosaic_read_extregs(ot_vi_pipe vi_pipe, isp_demosaic *demosaic)
{
    demosaic->dem_attr_update = ot_ext_system_demosaic_attr_update_en_read(vi_pipe);
    if (demosaic->dem_attr_update) {
        ot_ext_system_demosaic_attr_update_en_write(vi_pipe, TD_FALSE);
        isp_demosaic_attr_read(vi_pipe, &demosaic->mpi_cfg);
    }
}

td_s32 demosaic_proc_write(ot_vi_pipe vi_pipe, ot_isp_ctrl_proc_write *proc, const isp_demosaic *demosaic)
{
    ot_unused(vi_pipe);
    ot_isp_ctrl_proc_write proc_tmp;

    if ((proc->proc_buff == TD_NULL) || (proc->buff_len == 0)) {
        return TD_FAILURE;
    }

    proc_tmp.proc_buff = proc->proc_buff;
    proc_tmp.buff_len  = proc->buff_len;

    isp_proc_print_title(&proc_tmp, &proc->write_len, "demosaic info");

    isp_proc_printf(&proc_tmp, proc->write_len,
                    "%16s" "%16s" "%16s" "%16s" "%16s\n",
                    "enable", "nondir_str", "nondir_mf_str", "hf_str", "de_smth_range");

    isp_proc_printf(&proc_tmp, proc->write_len,
                    "%16u" "%16u" "%16u" "%16u" "%16u\n",
                    demosaic->mpi_cfg.enable, demosaic->actual.nddm_strength, demosaic->actual.nddm_mf_detail_strength,
                    demosaic->actual.hf_detail_strength, demosaic->actual.detail_smooth_range);

    proc->write_len += 1;

    return TD_SUCCESS;
}

td_s32 isp_demosaic_param_init(ot_vi_pipe vi_pipe, isp_reg_cfg *reg_cfg, isp_demosaic *demosaic_ctx)
{
    td_s32 ret;
    demosaic_ctx->init = TD_FALSE;
    ret = isp_demosaic_ctx_initialize(vi_pipe, demosaic_ctx);
    if (ret != TD_SUCCESS) {
        return ret;
    }

    demosaic_regs_initialize(vi_pipe, reg_cfg, demosaic_ctx);
    demosaic_ext_regs_initialize(vi_pipe, demosaic_ctx);
    demosaic_ctx->init = TD_TRUE;
    ot_ext_system_isp_demosaic_init_status_write(vi_pipe, TD_TRUE);
    return TD_SUCCESS;
}

static td_s32 isp_demosaic_init(ot_vi_pipe vi_pipe, td_void *reg_cfg)
{
    td_s32 ret;
    isp_demosaic *demosaic = TD_NULL;

    ot_ext_system_isp_demosaic_init_status_write(vi_pipe, TD_FALSE);
    ret = demosaic_ctx_init(vi_pipe);
    if (ret != TD_SUCCESS) {
        return ret;
    }

    demosaic_get_ctx(vi_pipe, demosaic);
    isp_check_pointer_return(demosaic);

    return isp_demosaic_param_init(vi_pipe, (isp_reg_cfg *)reg_cfg, demosaic);
}

static td_s32 isp_demosaic_wdr_mode_set(ot_vi_pipe vi_pipe, td_void *reg_cfg_info)
{
    td_u8 i;
    isp_reg_cfg *reg_cfg = (isp_reg_cfg *)reg_cfg_info;

    for (i = 0; i < reg_cfg->cfg_num; i++) {
        reg_cfg->alg_reg_cfg[i].dem_reg_cfg.vhdm_enable = TD_FALSE;
        reg_cfg->alg_reg_cfg[i].dem_reg_cfg.nddm_enable = TD_FALSE;
    }
    reg_cfg->cfg_key.bit1_dem_cfg = 1;

    return isp_demosaic_init(vi_pipe, reg_cfg_info);
}

static td_void demosaic_get_iso_info(td_u32 *iso_low, td_u32 *iso_up, td_u8 *idx_up, td_u8 *idx_lo, td_u32 input_iso)
{
    td_u8 inner_idx[2]; /* inner_idx and inner_iso length is 2 */
    td_u32 inner_iso[2];

    inner_idx[0] = get_iso_index(input_iso);
    inner_idx[1] = MAX2((td_s8)inner_idx[0] - 1, 0);
    inner_iso[0] = get_iso(inner_idx[0]);
    inner_iso[1] = get_iso(inner_idx[1]);

    *idx_up  = inner_idx[0];
    *idx_lo  = inner_idx[1];
    *iso_up  = inner_iso[0];
    *iso_low = inner_iso[1];
}

static td_void demosaic_cfg(isp_demosaic_dyna_cfg *dm_cfg, const isp_demosaic_static_cfg *static_dm_cfg,
    isp_demosaic *demosaic, td_u32 iso)
{
    td_s16 desat_th1 = (td_s16)static_dm_cfg->de_sat_thresh1;
    td_s16 desat_th2 = (td_s16)static_dm_cfg->de_sat_thresh2;
    td_s16 desat_hig = (td_s16)static_dm_cfg->de_sat_hig;
    td_u16 nddm_str  = demosaic->nddm_str;
    td_u16 nddm_str_cfg;
    td_s8  nddm_eps_sft;

    td_u8  iso_index_upper;
    td_u8  iso_index_lower;
    td_s32 iso2;
    td_s32 iso1;

    /* Calculate iso_index_upper & iso_index_lower by using input iso */
    iso_index_upper = get_iso_index(iso);
    iso_index_lower = MAX2((td_s8)iso_index_upper - 1, 0);

    /* Calculate iso_index_upper & iso_index_lower by using input iso */
    iso1 = (td_s32)get_iso(iso_index_lower);
    iso2 = (td_s32)get_iso(iso_index_upper);

    dm_cfg->cc_hf_max_ratio     = (td_u8)linear_inter(iso, iso1, demosaic->lut_cc_hf_max_ratio[iso_index_lower],
                                                      iso2, demosaic->lut_cc_hf_max_ratio[iso_index_upper]);
    dm_cfg->cc_hf_min_ratio     = (td_u8)linear_inter(iso, iso1, demosaic->lut_cc_hf_min_ratio[iso_index_lower],
                                                      iso2, demosaic->lut_cc_hf_min_ratio[iso_index_upper]);
    dm_cfg->de_sat_low          = (td_u16)linear_inter(iso, iso1, demosaic->lut_desat_low[iso_index_lower],
                                                       iso2, demosaic->lut_desat_low[iso_index_upper]);
    dm_cfg->de_sat_prot_th      = (td_u16)linear_inter(iso, iso1, demosaic->lut_desat_prot_th[iso_index_lower],
                                                       iso2, demosaic->lut_desat_prot_th[iso_index_upper]);
    dm_cfg->filter_blur_th_low  = (td_u16)linear_inter(iso, iso1, demosaic->lut_filter_blur_th_low[iso_index_lower],
                                                       iso2, demosaic->lut_filter_blur_th_low[iso_index_upper]);
    dm_cfg->filter_blur_th_hig  = (td_u16)linear_inter(iso, iso1, demosaic->lut_filter_blur_th_hig[iso_index_lower],
                                                       iso2, demosaic->lut_filter_blur_th_hig[iso_index_upper]);
    dm_cfg->var_thr_for_ahd     = (td_u16)linear_inter(iso, iso1, demosaic->lut_var_thr_for_ahd[iso_index_lower],
                                                       iso2, demosaic->lut_var_thr_for_ahd[iso_index_upper]);
    dm_cfg->var_thr_for_cac     = (td_u16)linear_inter(iso, iso1, demosaic->lut_var_thr_for_cac[iso_index_lower],
                                                       iso2, demosaic->lut_var_thr_for_cac[iso_index_upper]);

    nddm_eps_sft      = (td_s8)linear_inter(iso, iso1, demosaic->lut_nddm_eps_sft[iso_index_lower],
                                            iso2, demosaic->lut_nddm_eps_sft[iso_index_upper]);
    if (nddm_eps_sft <= 0) {
        nddm_str_cfg = (td_u16)linear_inter(iso, iso1, nddm_str, iso2,
            nddm_str << (demosaic->lut_nddm_eps_sft[iso_index_upper] - demosaic->lut_nddm_eps_sft[iso_index_lower]));
        dm_cfg->nddmstr = nddm_str_cfg & ((1 << 12) - 1); // shift 12
        dm_cfg->nddm_eps_sft = (td_u8)(-nddm_eps_sft);
    } else {
        nddm_str_cfg = (td_u16)linear_inter(iso, iso1, nddm_str << demosaic->lut_nddm_eps_sft[iso_index_lower],
                                            iso2, nddm_str << demosaic->lut_nddm_eps_sft[iso_index_upper]);
        dm_cfg->nddmstr = nddm_str_cfg & ((1 << 12) - 1); // shift 12
        dm_cfg->nddm_eps_sft = 0;
    }

    if (nddm_str == OT_ISP_DEMOSAIC_NONDIR_STR_MAX) {
        dm_cfg->nddm_mode = 1;
    } else {
        dm_cfg->nddm_mode = 0;
    }

    if (desat_th1 == desat_th2) {
        dm_cfg->de_sat_ratio = 0;
    } else {
        /* 16 is a accurary norm factor for parameter de_sat_ratio */
        dm_cfg->de_sat_ratio = 16 * ((td_s16)dm_cfg->de_sat_low - desat_hig) /   /* 16 */
            div_0_to_1(desat_th2 - desat_th1);
    }
}

static td_void demosaic_get_dynareg_params(ot_vi_pipe vi_pipe, isp_demosaic_dyna_cfg *dm_cfg,
                                           isp_demosaic_static_cfg *static_dm_cfg, isp_demosaic *demosaic)
{
    td_u8            idx_up, idx_lo, filter_coef_index, nddm_str, mf_str, hf_str;
    td_u32           iso1, iso2, iso;
    isp_usr_ctx     *isp_ctx = TD_NULL;
    ot_isp_demosaic_auto_attr   *att_auto   = &demosaic->mpi_cfg.auto_attr;
    ot_isp_demosaic_manual_attr *att_manual = &demosaic->mpi_cfg.manual_attr;
    isp_get_ctx(vi_pipe, isp_ctx);
    iso = isp_ctx->linkage.iso;
    demosaic_get_iso_info(&iso1, &iso2, &idx_up, &idx_lo, iso);

    if (demosaic->mpi_cfg.op_type == OT_OP_MODE_AUTO) {
        nddm_str           = (td_u8)linear_inter(iso, iso1, att_auto->nddm_strength[idx_lo],
                                                 iso2, att_auto->nddm_strength[idx_up]);
        filter_coef_index  = (td_u8)linear_inter(iso, iso1, att_auto->detail_smooth_range[idx_lo],
                                                 iso2, att_auto->detail_smooth_range[idx_up]);
        mf_str             = (td_u8)linear_inter(iso, iso1, att_auto->nddm_mf_detail_strength[idx_lo],
                                                 iso2, att_auto->nddm_mf_detail_strength[idx_up]);
        hf_str             = (td_u8)linear_inter(iso, iso1, att_auto->hf_detail_strength[idx_lo],
                                                 iso2, att_auto->hf_detail_strength[idx_up]);
    } else {
        nddm_str                    = att_manual->nddm_strength;
        filter_coef_index           = att_manual->detail_smooth_range;
        mf_str                      = att_manual->nddm_mf_detail_strength;
        hf_str                      = att_manual->hf_detail_strength;
    }
    demosaic->nddm_str = nddm_str;
    demosaic_cfg(dm_cfg, static_dm_cfg, demosaic, iso);
    dm_cfg->ehc_gray = mf_str;
    dm_cfg->lpff0 = filter_coef_index;
    dm_cfg->lpff3 = hf_str;
    dm_cfg->resh  = TD_TRUE;
}

static td_void demosaic_get_actual_params(ot_vi_pipe vi_pipe, isp_demosaic_dyna_cfg *dm_cfg, isp_demosaic *demosaic)
{
    td_u8         filter_coef_index;
    td_u8         nddm_str;
    td_u8         idx_up, idx_lo;
    td_u32        iso1, iso2, iso;
    isp_usr_ctx  *isp_ctx = TD_NULL;
    ot_isp_demosaic_auto_attr *att_auto = &demosaic->mpi_cfg.auto_attr;

    isp_get_ctx(vi_pipe, isp_ctx);
    iso = isp_ctx->linkage.iso;
    demosaic_get_iso_info(&iso1, &iso2, &idx_up, &idx_lo, iso);

    if (demosaic->mpi_cfg.op_type == OT_OP_MODE_AUTO) {
        nddm_str          = (td_u8)linear_inter(iso, iso1, att_auto->nddm_strength[idx_lo],
                                                iso2, att_auto->nddm_strength[idx_up]);
        filter_coef_index = (td_u8)linear_inter(iso, iso1, att_auto->detail_smooth_range[idx_lo],
                                                iso2, att_auto->detail_smooth_range[idx_up]);
    } else {
        nddm_str          = demosaic->mpi_cfg.manual_attr.nddm_strength;
        filter_coef_index = demosaic->mpi_cfg.manual_attr.detail_smooth_range;
    }

    demosaic->actual.nddm_strength           = nddm_str;
    demosaic->actual.detail_smooth_range     = filter_coef_index;
    demosaic->actual.nddm_mf_detail_strength = dm_cfg->ehc_gray;
    demosaic->actual.hf_detail_strength      = dm_cfg->lpff3;
}

td_void isp_demosaic_reg_update(ot_vi_pipe vi_pipe, isp_reg_cfg *reg, isp_demosaic *demosaic)
{
    td_u8 i;
    isp_demosaic_dyna_cfg *dm_cfg  = TD_NULL;
    isp_demosaic_static_cfg *static_dm_cfg = TD_NULL;

    isp_usr_ctx  *isp_ctx = TD_NULL;
    isp_get_ctx(vi_pipe, isp_ctx);

    for (i = 0; i < isp_ctx->block_attr.block_num; i++) {
        dm_cfg = &reg->alg_reg_cfg[i].dem_reg_cfg.dyna_reg_cfg;
        static_dm_cfg = &reg->alg_reg_cfg[i].dem_reg_cfg.static_reg_cfg;
        demosaic_get_dynareg_params(vi_pipe, dm_cfg, static_dm_cfg, demosaic);
    }

    demosaic_get_actual_params(vi_pipe, &reg->alg_reg_cfg[0].dem_reg_cfg.dyna_reg_cfg, demosaic);
}

static td_bool check_demosaic_open(const isp_demosaic *demosaic)
{
    return (demosaic->mpi_cfg.enable == TD_TRUE);
}

static td_s32 isp_demosaic_run(ot_vi_pipe vi_pipe, const td_void *stat_info, td_void *reg_cfg, td_s32 rsv)
{
    td_u8  i;
    ot_isp_alg_mod alg_mod = OT_ISP_ALG_DEMOSAIC;
    isp_reg_cfg *reg = (isp_reg_cfg *)reg_cfg;
    isp_usr_ctx *isp_ctx = TD_NULL;
    isp_demosaic *demosaic = TD_NULL;
    ot_unused(stat_info);
    ot_unused(rsv);

    isp_get_ctx(vi_pipe, isp_ctx);
    demosaic_get_ctx(vi_pipe, demosaic);
    isp_check_pointer_return(demosaic);

    ot_ext_system_isp_demosaic_init_status_write(vi_pipe, demosaic->init);
    if (demosaic->init != TD_TRUE) {
        ioctl(isp_get_fd(vi_pipe), ISP_ALG_INIT_ERR_INFO_PRINT, &alg_mod);
        return TD_SUCCESS;
    }

    isp_demosaic_set_long_frame_mode(vi_pipe, demosaic);

    if ((isp_ctx->frame_cnt % 2 != 0) && (isp_ctx->linkage.snap_state != TD_TRUE)) { /* proc every 2 frames */
        return TD_SUCCESS;
    }

    demosaic->mpi_cfg.enable = ot_ext_system_demosaic_enable_read(vi_pipe);

    for (i = 0; i < reg->cfg_num; i++) {
        reg->alg_reg_cfg[i].dem_reg_cfg.vhdm_enable = TD_TRUE;
        reg->alg_reg_cfg[i].dem_reg_cfg.nddm_enable = TD_TRUE;
        reg->alg_reg_cfg[i].dem_reg_cfg.inner_enable = demosaic->mpi_cfg.enable;
    }

    reg->cfg_key.bit1_dem_cfg = 1;

    /* check hardware setting */
    if (!check_demosaic_open(demosaic)) {
        return TD_SUCCESS;
    }

    isp_demosaic_read_extregs(vi_pipe, demosaic);

    isp_demosaic_reg_update(vi_pipe, reg, demosaic);

    return TD_SUCCESS;
}

static td_s32 isp_demosaic_ctrl(ot_vi_pipe vi_pipe, td_u32 cmd, td_void *value)
{
    isp_demosaic *demosaic = TD_NULL;
    isp_reg_cfg_attr *local_reg_cfg = TD_NULL;

    switch (cmd) {
        case OT_ISP_WDR_MODE_SET:
            isp_regcfg_get_ctx(vi_pipe, local_reg_cfg);
            isp_check_pointer_return(local_reg_cfg);
            isp_demosaic_wdr_mode_set(vi_pipe, (td_void *)&local_reg_cfg->reg_cfg);
            break;

        case OT_ISP_PROC_WRITE:
            demosaic_get_ctx(vi_pipe, demosaic);
            isp_check_pointer_return(demosaic);
            demosaic_proc_write(vi_pipe, (ot_isp_ctrl_proc_write *)value, demosaic);
            break;

        default:
            break;
    }

    return TD_SUCCESS;
}

static td_s32 isp_demosaic_exit(ot_vi_pipe vi_pipe)
{
    td_u16 i;
    isp_reg_cfg_attr *local_reg_cfg = TD_NULL;

    isp_regcfg_get_ctx(vi_pipe, local_reg_cfg);
    ot_ext_system_isp_demosaic_init_status_write(vi_pipe, TD_FALSE);

    for (i = 0; i < local_reg_cfg->reg_cfg.cfg_num; i++) {
        local_reg_cfg->reg_cfg.alg_reg_cfg[i].dem_reg_cfg.nddm_enable = TD_FALSE;
        local_reg_cfg->reg_cfg.alg_reg_cfg[i].dem_reg_cfg.vhdm_enable = TD_FALSE;
    }

    local_reg_cfg->reg_cfg.cfg_key.bit1_dem_cfg = 1;

    demosaic_ctx_exit(vi_pipe);

    return TD_SUCCESS;
}

td_s32 isp_alg_register_demosaic(ot_vi_pipe vi_pipe)
{
    td_s32 ret;
    isp_usr_ctx *isp_ctx = TD_NULL;
    isp_alg_node *algs = TD_NULL;

    isp_get_ctx(vi_pipe, isp_ctx);
    isp_alg_check_return(isp_ctx->alg_key.bit1_demosaic);
    algs = isp_search_alg(isp_ctx->algs);
    isp_check_pointer_return(algs);
    ret = strncpy_s(algs->alg_name, sizeof(algs->alg_name), "demosaic", sizeof("demosaic"));
    isp_check_eok_return(ret, TD_FAILURE);
    algs->alg_type = OT_ISP_ALG_DEMOSAIC;
    algs->alg_func.pfn_alg_init = isp_demosaic_init;
    algs->alg_func.pfn_alg_run  = isp_demosaic_run;
    algs->alg_func.pfn_alg_ctrl = isp_demosaic_ctrl;
    algs->alg_func.pfn_alg_exit = isp_demosaic_exit;
    algs->used = TD_TRUE;

    return TD_SUCCESS;
}
