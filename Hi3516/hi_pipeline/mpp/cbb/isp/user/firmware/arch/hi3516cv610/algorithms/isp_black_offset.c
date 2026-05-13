/*
  Copyright (c), 2001-2025, Shenshu Tech. Co., Ltd.
 */

#include "isp_alg.h"
#include "isp_sensor.h"
#include "isp_config.h"
#include "isp_ext_config.h"
#include "isp_math_utils.h"
#include "isp_proc.h"
#include "isp_param_check.h"
#include "isp_ext_reg_access.h"
#include "securec.h"

#define BLACK_OFFSET_BIT_WIDTH 14
#define EXPANDER_X_BIT_WIDTH   7
#define EXPANDER_Y_BIT_WIDTH   17

typedef struct {
    td_bool init;
    td_bool pre_defect_pixel;
    td_u8   black_level_change;
    td_u8   merge_frame;
    td_u8   wdr_mode_state;
    td_u16  black_level[OT_ISP_WDR_MAX_FRAME_NUM][OT_ISP_BAYER_CHN_NUM];
    td_u16  isp_black_level[OT_ISP_WDR_MAX_FRAME_NUM][OT_ISP_BAYER_CHN_NUM];
    td_bool user_black_level_en;
    td_u16  user_black_level[OT_ISP_WDR_MAX_FRAME_NUM][OT_ISP_BAYER_CHN_NUM];
    isp_blc_actual_info actual;
    ot_isp_black_level_mode op_mode;
    td_bool expander_update;
    ot_isp_expander_attr expander_attr; // only support offline to change cfg
} isp_blacklevel_ctx;

typedef struct {
    td_u8 shift;
    td_u16 x0;
    td_u16 x1;
    td_s32 y0;
    td_s32 y1;
} blc_linear_interp_cfg;

isp_blacklevel_ctx g_black_level_ctx[OT_ISP_MAX_PIPE_NUM] = { { 0 } };
#define blacklevel_get_ctx(pipe, blc_ctx) blc_ctx = &g_black_level_ctx[pipe]

static td_void blc_get_merge_frame(td_u8 wdr_mode, isp_blacklevel_ctx *blc_ctx)
{
    if (is_linear_mode(wdr_mode) || is_built_in_wdr_mode(wdr_mode)) {
        blc_ctx->merge_frame = 1;
    } else if (is_2to1_wdr_mode(wdr_mode)) {
        blc_ctx->merge_frame = 2;     /* 2to1 wdr */
    } else if (is_3to1_wdr_mode(wdr_mode)) {
        blc_ctx->merge_frame = 3;    /* 3to1 wdr */
    } else {
        blc_ctx->merge_frame = 1;
    }
}

static td_s32 isp_blc_check_cmos_param(ot_vi_pipe vi_pipe, const ot_isp_cmos_black_level *sns_black_level)
{
    td_s32 ret;
    isp_check_pointer_return(sns_black_level);
    isp_check_bool_return(sns_black_level->auto_attr.update);

    if (sns_black_level->black_level_mode >= OT_ISP_BLACK_LEVEL_MODE_DYNAMIC) {
        isp_err_trace("Err cmos black_level_mode %d!\n", sns_black_level->black_level_mode);
        return OT_ERR_ISP_ILLEGAL_PARAM;
    }

    ret = isp_black_level_value_check("cmos manual", sns_black_level->manual_attr.black_level);
    if (ret != TD_SUCCESS) {
        return ret;
    }

    ret = isp_black_level_value_check("cmos auto", sns_black_level->auto_attr.black_level);
    if (ret != TD_SUCCESS) {
        return ret;
    }

    ret = isp_black_level_value_check("cmos user", sns_black_level->user_black_level);
    if (ret != TD_SUCCESS) {
        return ret;
    }

    ret = isp_user_black_level_en_check(vi_pipe, sns_black_level->user_black_level_en);
    if (ret != TD_SUCCESS) {
        return ret;
    }

    ret = isp_dynamic_blc_attr_check(vi_pipe, "cmos", &sns_black_level->dynamic_attr,
                                     sns_black_level->black_level_mode);
    if (ret != TD_SUCCESS) {
        return ret;
    }

    return TD_SUCCESS;
}

static td_s32 blc_initialize(ot_vi_pipe vi_pipe)
{
    td_u8  wdr_mode;
    td_s32 ret;
    size_t blc_size;
    isp_blacklevel_ctx      *blc_ctx = TD_NULL;
    ot_isp_cmos_black_level *sns_black_level = TD_NULL;
    isp_usr_ctx             *isp_ctx         = TD_NULL;

    blacklevel_get_ctx(vi_pipe, blc_ctx);
    isp_sensor_get_blc(vi_pipe, &sns_black_level);
    isp_get_ctx(vi_pipe, isp_ctx);

    wdr_mode = isp_ctx->sns_wdr_mode;

    ret = isp_blc_check_cmos_param(vi_pipe, sns_black_level);
    if (ret != TD_SUCCESS) {
        isp_err_trace("blc cmos check error\n");
        return ret;
    }

    blc_ctx->op_mode  = sns_black_level->black_level_mode;
    blc_size = OT_ISP_WDR_MAX_FRAME_NUM * OT_ISP_BAYER_CHN_NUM * sizeof(td_u16);
    if (blc_ctx->op_mode == OT_ISP_BLACK_LEVEL_MODE_AUTO) {
        (td_void)memcpy_s(blc_ctx->black_level, blc_size, sns_black_level->auto_attr.black_level, blc_size);
    } else {
        (td_void)memcpy_s(blc_ctx->black_level, blc_size, sns_black_level->manual_attr.black_level, blc_size);
    }

    blc_ctx->user_black_level_en = sns_black_level->user_black_level_en;
    (td_void)memcpy_s(blc_ctx->user_black_level, blc_size, sns_black_level->user_black_level, blc_size);

    if (is_linear_mode(wdr_mode) || is_built_in_wdr_mode(wdr_mode)) {
        blc_ctx->wdr_mode_state = TD_FALSE;
    } else {
        blc_ctx->wdr_mode_state = TD_TRUE;
    }

    isp_expander_attr_read(vi_pipe, &blc_ctx->expander_attr);

    blc_get_merge_frame(wdr_mode, blc_ctx);

    blc_ctx->pre_defect_pixel = TD_FALSE;

    return TD_SUCCESS;
}

static td_void blc_ext_regs_initialize(ot_vi_pipe vi_pipe)
{
    ot_isp_black_level_manual_attr manual = {0};
    isp_blacklevel_ctx *blc_ctx = TD_NULL;
    size_t blc_size;

    blacklevel_get_ctx(vi_pipe, blc_ctx);

    ot_ext_system_black_level_mode_write(vi_pipe, blc_ctx->op_mode);
    ot_ext_system_black_level_change_write(vi_pipe, OT_EXT_SYSTEM_BLACK_LEVEL_CHANGE_DEFAULT);

    blc_size = OT_ISP_WDR_MAX_FRAME_NUM * OT_ISP_BAYER_CHN_NUM * sizeof(td_u16);
    (td_void)memcpy_s(&manual, blc_size, blc_ctx->black_level, blc_size);
    isp_black_level_manual_attr_write(vi_pipe, &manual);
    ot_ext_system_isp_user_blc_en_write(vi_pipe, blc_ctx->user_black_level_en);
    isp_user_black_level_write(vi_pipe, blc_ctx->user_black_level);

    black_level_actual_value_write(vi_pipe, &blc_ctx->actual);
}

static td_void get_isp_black_level_value(isp_blacklevel_ctx *blc_ctx)
{
    size_t blc_size = OT_ISP_WDR_MAX_FRAME_NUM * OT_ISP_BAYER_CHN_NUM * sizeof(td_u16);

    if (blc_ctx->user_black_level_en == TD_TRUE) {
        (td_void)memcpy_s(blc_ctx->isp_black_level, blc_size, blc_ctx->user_black_level, blc_size);
    } else {
        (td_void)memcpy_s(blc_ctx->isp_black_level, blc_size, blc_ctx->black_level, blc_size);
    }
}

static td_s32 blc_linear_interpol(td_u16 xm, blc_linear_interp_cfg *intp_cfg)
{
    td_s32 ym;
    td_s32 y0 = intp_cfg->y0;
    td_s32 y1 = intp_cfg->y1;
    td_s64 tmp;

    if (xm <= (intp_cfg->x0 << intp_cfg->shift)) {
        return y0;
    }
    if (xm >= (intp_cfg->x1 << intp_cfg->shift)) {
        return y1;
    }

    tmp = (y1 - y0) * (xm - (intp_cfg->x0 << intp_cfg->shift)) / div_0_to_1(intp_cfg->x1 - intp_cfg->x0);
    ym = (td_s32)signed_right_shift(tmp, intp_cfg->shift) + y0;

    return ym;
}

static td_u32 get_index(td_u32 x, td_u16 *x_lut, td_u16 point_num)
{
    td_u32 index;

    for (index = 0; index < point_num; index++) {
        if (x <= x_lut[index]) {
            break;
        }
    }

    return index;
}

static td_void get_built_in_expander_blc(const ot_isp_expander_attr *expander,
    td_u16 (*sensor_blc)[OT_ISP_BAYER_CHN_NUM], td_u16 *expander_blc, td_u8 array_length)
{
    td_u8  i, j;
    const td_u8 x_shift = BLACK_OFFSET_BIT_WIDTH - EXPANDER_X_BIT_WIDTH;
    const td_u8 y_shift = EXPANDER_Y_BIT_WIDTH - BLACK_OFFSET_BIT_WIDTH;
    td_u8  idx_up, idx_low;
    td_u16 expander_point_num;
    td_u16 x[OT_ISP_EXPANDER_POINT_NUM_MAX + 1] = {0};
    td_u32 y[OT_ISP_EXPANDER_POINT_NUM_MAX + 1] = {0};
    blc_linear_interp_cfg  intp_cfg;

    expander_point_num = expander->knee_point_num;

    for (i = 1; i < expander_point_num + 1; i++) {
        x[i] = (td_u16)(expander->knee_point_coord[i - 1].x);
        y[i] = (td_u32)(expander->knee_point_coord[i - 1].y);
    }

    for (j = 0; j < array_length; j++) {
        idx_up  = (td_u8)get_index(sensor_blc[0][j] >> x_shift, x, expander_point_num);
        idx_low = (td_u8)MAX2((td_s8)idx_up - 1, 0);

        /* update blc_linear_interp_cfg */
        intp_cfg.x0 = x[idx_low];
        intp_cfg.x1 = x[idx_up];
        intp_cfg.y0 = (td_s32)y[idx_low];
        intp_cfg.y1 = (td_s32)y[idx_up];
        intp_cfg.shift = x_shift;
        expander_blc[j] = (td_u16)(((td_u32)blc_linear_interpol(sensor_blc[0][j], &intp_cfg)) >> y_shift);
    }
}

static td_void update_actual_blc_normal(isp_blacklevel_ctx *blc_ctx)
{
    td_u8 i, j;
    for (i = 0; i < blc_ctx->merge_frame; i++) {
        for (j = 0; j < OT_ISP_BAYER_CHN_NUM; j++) {
            blc_ctx->actual.sns_black_level[i][j] = blc_ctx->black_level[i][j];
            blc_ctx->actual.isp_black_level[i][j] = blc_ctx->isp_black_level[i][j];
        }
    }

    for (i = blc_ctx->merge_frame; i < OT_ISP_WDR_MAX_FRAME_NUM; i++) {
        for (j = 0; j < OT_ISP_BAYER_CHN_NUM; j++) {
            blc_ctx->actual.sns_black_level[i][j] = 0;
            blc_ctx->actual.isp_black_level[i][j] = 0;
        }
    }
}

static td_void be_blc_dyna_regs_linear(ot_vi_pipe vi_pipe, isp_be_blc_dyna_cfg *dyna_blc, isp_blacklevel_ctx *blc_ctx)
{
    td_u8 i, j;
    ot_unused(vi_pipe);
    for (j = 0; j < OT_ISP_BAYER_CHN_NUM; j++) {
        for (i = 0; i < ISP_WDR_CHN_MAX; i++) {
            dyna_blc->wdr_dg_blc[i].blc[j]  = blc_ctx->isp_black_level[i][j]; /* 4DG, shift by 2 */
            dyna_blc->wdr_blc[i].blc[j]    = blc_ctx->isp_black_level[i][j] >> 2;      /* WDR */
            dyna_blc->wdr_blc[i].out_blc   = 0;                     /* WDR */
        }

        dyna_blc->lsc_blc.blc[j]   = blc_ctx->isp_black_level[0][j]; /* lsc, shift by 2 */
        dyna_blc->dg_blc.blc[j]    = blc_ctx->isp_black_level[0][j]; /* Dg, shift by 2 */
        dyna_blc->ae_blc.blc[j]    = blc_ctx->isp_black_level[0][j]; /* AE, shift by 2 */
        dyna_blc->mg_blc.blc[j]    = blc_ctx->isp_black_level[0][j]; /* MG, shift by 2 */
        dyna_blc->wb_blc.blc[j]    = blc_ctx->isp_black_level[0][j]; /* WB, shift by 2 */
        dyna_blc->rgbir_blc.blc[j] = blc_ctx->isp_black_level[0][j] >> 2; /* rgbir */
        dyna_blc->ge_blc.blc[j] = blc_ctx->isp_black_level[0][j] >> 2; /* ge 12bit blc */
    }

    /* bnr */
    dyna_blc->bnr_blc.blc[0]  = blc_ctx->isp_black_level[0][0] >> 2; /* 12bits */

    update_actual_blc_normal(blc_ctx);
}

static td_void be_blc_dyna_regs_wdr(ot_vi_pipe vi_pipe, isp_be_blc_dyna_cfg *dyna_blc, isp_blacklevel_ctx *blc_ctx)
{
    td_u8  i, j;
    td_u8  wdr_mode_state = blc_ctx->wdr_mode_state;
    td_u16 wdr_out_blc;

    if (wdr_mode_state == TD_FALSE) { /* reg value same as linear mode */
        be_blc_dyna_regs_linear(vi_pipe, dyna_blc, blc_ctx);
    } else if (wdr_mode_state == TD_TRUE) {
        wdr_out_blc = blc_ctx->isp_black_level[0][0]; /* WDR outblc, 14bit's blc */
        for (j = 0; j < OT_ISP_BAYER_CHN_NUM; j++) {
            for (i = 0; i < ISP_WDR_CHN_MAX; i++) {
                dyna_blc->wdr_dg_blc[i].blc[j]  = blc_ctx->isp_black_level[i][j]; /* 4DG, shift 2 */
                dyna_blc->wdr_blc[i].blc[j]      = blc_ctx->isp_black_level[i][j] >> 2; /* WDR, logic bit is 12 */
                dyna_blc->wdr_blc[i].out_blc     = wdr_out_blc >> 3; /* WDR, 17 bit wdr by 3, 11 bit's blc */
            }

            dyna_blc->lsc_blc.blc[j]      = wdr_out_blc; /* lsc,shift by 6 */
            dyna_blc->dg_blc.blc[j]       = wdr_out_blc; /* Dg,shift by 6 */
            dyna_blc->ae_blc.blc[j]       = wdr_out_blc; /* AE,shift by 6 */
            dyna_blc->mg_blc.blc[j]       = wdr_out_blc; /* MG,shift by 6 */
            dyna_blc->wb_blc.blc[j]       = wdr_out_blc; /* WB,shift by 6 */
            dyna_blc->ge_blc.blc[j] = blc_ctx->isp_black_level[0][j] >> 2; /* ge 12bit blc */
            dyna_blc->rgbir_blc.blc[j]    = 0;
        }

        /* bnr */
        dyna_blc->bnr_blc.blc[0]  = wdr_out_blc >> 2; /* 12bits, shift by 8 bits */

        update_actual_blc_normal(blc_ctx);
    }
}

static td_s32 be_blc_dyna_regs_built_in(ot_vi_pipe vi_pipe, isp_be_blc_dyna_cfg *dyna_blc, isp_blacklevel_ctx *blc_ctx)
{
    td_u8  i, j;
    td_u16 actual_black_level[OT_ISP_BAYER_CHN_NUM] = {0};

    if (blc_ctx->expander_attr.enable == TD_TRUE) {
        get_built_in_expander_blc(&blc_ctx->expander_attr, blc_ctx->black_level, actual_black_level,
            OT_ISP_BAYER_CHN_NUM);
    } else {
        for (i = 0; i < OT_ISP_BAYER_CHN_NUM; i++) {
            actual_black_level[i] = blc_ctx->black_level[0][i];      /* 14bits */
        }
    }

    for (j = 0; j < OT_ISP_BAYER_CHN_NUM; j++) {
        for (i = 0; i < ISP_WDR_CHN_MAX; i++) {
            dyna_blc->wdr_dg_blc[i].blc[j]  = 0; /* 4DG */
            dyna_blc->wdr_blc[i].blc[j]     = 0; /* WDR */
            dyna_blc->wdr_blc[i].out_blc    = 0; /* WDR */
        }

        dyna_blc->lsc_blc.blc[j]      = actual_black_level[j]; /* lsc */
        dyna_blc->dg_blc.blc[j]       = actual_black_level[j]; /* Dg */
        dyna_blc->ae_blc.blc[j]       = actual_black_level[j]; /* AE */
        dyna_blc->mg_blc.blc[j]       = actual_black_level[j]; /* MG */
        dyna_blc->wb_blc.blc[j]       = actual_black_level[j]; /* WB */
        dyna_blc->rgbir_blc.blc[j]    = 0;
        dyna_blc->ge_blc.blc[j] = actual_black_level[j] >> 2; /* ge 12bit blc */
    }

    /* bnr */
    dyna_blc->bnr_blc.blc[0] = actual_black_level[0] >> 2; /* 12bits */

    for (i = 0; i < OT_ISP_BAYER_CHN_NUM; i++) {
        blc_ctx->actual.isp_black_level[0][i] = actual_black_level[i]; /* Notice: Actual Blc is 14bits */
        blc_ctx->actual.sns_black_level[0][i] = actual_black_level[i];
    }

    for (i = 1; i < OT_ISP_WDR_MAX_FRAME_NUM; i++) {
        for (j = 0; j < OT_ISP_BAYER_CHN_NUM; j++) {
            blc_ctx->actual.isp_black_level[i][j] = 0;
            blc_ctx->actual.sns_black_level[i][j] = 0;
        }
    }

    return TD_SUCCESS;
}

static td_void be_blc_dyna_regs(ot_vi_pipe vi_pipe, td_u8 wdr_mode, isp_be_blc_dyna_cfg *dyna_blc,
                                isp_blacklevel_ctx *blc_ctx)
{
    blc_ctx->wdr_mode_state = ot_ext_system_wdr_en_read(vi_pipe);

    if (is_linear_mode(wdr_mode)) {
        be_blc_dyna_regs_linear(vi_pipe, dyna_blc, blc_ctx);
    } else if (is_2to1_wdr_mode(wdr_mode)) {
        be_blc_dyna_regs_wdr(vi_pipe, dyna_blc, blc_ctx);
    } else if (is_built_in_wdr_mode(wdr_mode)) {
        be_blc_dyna_regs_built_in(vi_pipe, dyna_blc, blc_ctx);
    }

    dyna_blc->resh_dyna = TD_TRUE;
}

static td_void be_blc_static_regs(td_u8 wdr_mode, isp_be_blc_static_cfg *static_blc)
{
    td_u8 i;
    ot_unused(wdr_mode);
    /* 4DG */
    for (i = 0; i < ISP_WDR_CHN_MAX; i++) {
        static_blc->wdr_dg_blc[i].blc_in  = TD_TRUE;
        static_blc->wdr_dg_blc[i].blc_out = TD_TRUE;
    }

    /* WDR */
    static_blc->wdr_blc[0].blc_out = TD_TRUE;
    /* lsc */
    static_blc->lsc_blc.blc_in     = TD_TRUE;
    static_blc->lsc_blc.blc_out    = TD_TRUE;
    /* Dg */
    static_blc->dg_blc.blc_in      = TD_TRUE;
    static_blc->dg_blc.blc_out     = TD_FALSE;
    /* AE */
    static_blc->ae_blc.blc_in      = TD_FALSE;
    /* MG */
    static_blc->mg_blc.blc_in      = TD_FALSE;
    /* WB */
    static_blc->wb_blc.blc_in      = TD_FALSE;
    static_blc->wb_blc.blc_out     = TD_FALSE;

    static_blc->resh_static = TD_TRUE;
}

static td_void fe_blc_dyna_regs(isp_fe_blc_dyna_cfg *dyna_blc, isp_blacklevel_ctx *blc_ctx)
{
    td_u8 i, j;
    td_s16 diff[OT_ISP_WDR_MAX_FRAME_NUM][OT_ISP_BAYER_CHN_NUM] = { 0 };
    dyna_blc->fe_blc1_en = blc_ctx->user_black_level_en;
    if (blc_ctx->user_black_level_en == TD_TRUE) {
        for (i = 0; i < OT_ISP_WDR_MAX_FRAME_NUM; i++) {
            for (j = 0; j < OT_ISP_BAYER_CHN_NUM; j++) {
                diff[i][j] = (td_s16)blc_ctx->black_level[i][j] - (td_s16)blc_ctx->user_black_level[i][j];
                diff[i][j] = clip3(diff[i][j], -16383, 16383); /* range[-16383, 16383] */
            }
        }
    }

    for (i = 0; i < OT_ISP_WDR_MAX_FRAME_NUM; i++) {
        for (j = 0; j < OT_ISP_BAYER_CHN_NUM; j++) {
            dyna_blc->fe_blc[i].blc[j] = (td_u16)diff[i][j];
            dyna_blc->fe_dg_blc[i].blc[j]  = blc_ctx->isp_black_level[i][j]; /* Fe Dg */
            dyna_blc->fe_wb_blc[i].blc[j]  = blc_ctx->isp_black_level[i][j]; /* Fe WB */
            dyna_blc->fe_ae_blc[i].blc[j]  = blc_ctx->isp_black_level[i][j]; /* Fe AE */
            dyna_blc->rc_blc.blc[j]     = blc_ctx->isp_black_level[i][j]; /* RC */
        }
    }

    dyna_blc->resh_dyna = TD_TRUE;
}

static td_void fe_blc_static_regs(isp_fe_blc_static_cfg *static_blc)
{
    /* Fe Dg */
    static_blc->fe_dg_blc.blc_in  = TD_TRUE;
    static_blc->fe_dg_blc.blc_out = TD_TRUE;
    /* Fe WB */
    static_blc->fe_wb_blc.blc_in  = TD_TRUE;
    static_blc->fe_wb_blc.blc_out = TD_TRUE;
    /* Fe AE */
    static_blc->fe_ae_blc.blc_in  = TD_FALSE;
    /* RC */
    static_blc->rc_blc.blc_in    = TD_FALSE;
    static_blc->rc_blc.blc_out   = TD_FALSE;
    /* FE BLC */
    static_blc->fe_blc.blc_in    = TD_TRUE;

    static_blc->resh_static = TD_TRUE;
}

static td_void update_wdr_sync_offset(isp_fswdr_sync_cfg *sync_reg_cfg, isp_blc_dyna_cfg *wdr_blc)
{
    sync_reg_cfg->offset0 = wdr_blc[0].blc[1];
}


static td_void blc_regs_initialize(ot_vi_pipe vi_pipe, isp_reg_cfg *reg_cfg)
{
    td_u8  i, wdr_mode;
    isp_blacklevel_ctx *blc_ctx = TD_NULL;
    isp_usr_ctx        *isp_ctx = TD_NULL;

    isp_get_ctx(vi_pipe, isp_ctx);
    blacklevel_get_ctx(vi_pipe, blc_ctx);

    wdr_mode = isp_ctx->sns_wdr_mode;

    get_isp_black_level_value(blc_ctx);

    /* BE */
    for (i = 0; i < reg_cfg->cfg_num; i++) {
        be_blc_dyna_regs(vi_pipe, wdr_mode, &reg_cfg->alg_reg_cfg[i].be_blc_cfg.dyna_blc, blc_ctx);
        be_blc_static_regs(wdr_mode, &reg_cfg->alg_reg_cfg[i].be_blc_cfg.static_blc);
        update_wdr_sync_offset(&reg_cfg->alg_reg_cfg[i].wdr_reg_cfg.sync_reg_cfg,
                               reg_cfg->alg_reg_cfg[i].be_blc_cfg.dyna_blc.wdr_blc);
        reg_cfg->alg_reg_cfg[i].be_blc_cfg.resh_dyna_init = TD_TRUE;
    }
    reg_cfg->cfg_key.bit1_be_blc_cfg = 1;

    /* FE */
    fe_blc_dyna_regs(&reg_cfg->alg_reg_cfg[0].fe_blc_cfg.dyna_blc, blc_ctx);
    fe_blc_static_regs(&reg_cfg->alg_reg_cfg[0].fe_blc_cfg.static_blc);
    reg_cfg->alg_reg_cfg[0].fe_blc_cfg.resh_dyna_init = TD_TRUE;
    reg_cfg->cfg_key.bit1_fe_blc_cfg                  = 1;
}

static td_void blc_read_extregs(ot_vi_pipe vi_pipe)
{
    isp_blacklevel_ctx *blc_ctx = TD_NULL;
    ot_isp_black_level_manual_attr manual = {0};
    size_t size = OT_ISP_WDR_MAX_FRAME_NUM * OT_ISP_BAYER_CHN_NUM * sizeof(td_u16);

    blacklevel_get_ctx(vi_pipe, blc_ctx);

    blc_ctx->black_level_change = ot_ext_system_black_level_change_read(vi_pipe);

    if (blc_ctx->black_level_change) {
        ot_ext_system_black_level_change_write(vi_pipe, TD_FALSE);
        blc_ctx->op_mode         = ot_ext_system_black_level_mode_read(vi_pipe);

        isp_black_level_manual_attr_read(vi_pipe, &manual);
        (td_void)memcpy_s(&blc_ctx->black_level[0][0], size, &manual, size);
    }

    blc_ctx->expander_update = ot_ext_system_expander_blc_param_update_read(vi_pipe);
    if (blc_ctx->expander_update) {
        ot_ext_system_expander_blc_param_update_write(vi_pipe, TD_FALSE);
        isp_expander_attr_read(vi_pipe, &blc_ctx->expander_attr);
    }

    blc_ctx->user_black_level_en = ot_ext_system_isp_user_blc_en_read(vi_pipe);
    if (blc_ctx->user_black_level_en == TD_TRUE) {
        isp_user_black_level_read(vi_pipe, blc_ctx->user_black_level);
    }
}

static td_s32 blc_proc_mode_check(const isp_blacklevel_ctx *blc_ctx, td_char mode_name[], td_u8 mode_name_size)
{
    td_s32 ret;
    if (blc_ctx->op_mode == OT_ISP_BLACK_LEVEL_MODE_AUTO) {
        ret = snprintf_s(mode_name, mode_name_size, mode_name_size - 1, "auto");
        if (ret < 0) {
            return TD_FAILURE;
        }
    } else if (blc_ctx->op_mode == OT_ISP_BLACK_LEVEL_MODE_MANUAL) {
        ret = snprintf_s(mode_name, mode_name_size, mode_name_size - 1, "manual");
        if (ret < 0) {
            return TD_FAILURE;
        }
#ifdef CONFIG_OT_ISP_DYNAMIC_BLC_SUPPORT
    } else if (blc_ctx->op_mode == OT_ISP_BLACK_LEVEL_MODE_DYNAMIC) {
        ret = snprintf_s(mode_name, mode_name_size, mode_name_size - 1, "dynamic");
        if (ret < 0) {
            return TD_FAILURE;
        }
#endif
    } else {
        ret = snprintf_s(mode_name, mode_name_size, mode_name_size - 1, "BUTT");
        if (ret < 0) {
            return TD_FAILURE;
        }
    }
    return TD_SUCCESS;
}

static td_s32 isp_blc_proc_write(ot_vi_pipe vi_pipe, ot_isp_ctrl_proc_write *proc)
{
    ot_isp_ctrl_proc_write proc_tmp;
    isp_blacklevel_ctx    *blc_ctx = TD_NULL;
    td_s32 ret;
    td_char mode_name[MAX_MMZ_NAME_LEN] = { 0 };
    size_t mode_name_size;
    td_u8 i;

    blacklevel_get_ctx(vi_pipe, blc_ctx);

    if ((proc->proc_buff == TD_NULL) || (proc->buff_len == 0)) {
        return TD_FAILURE;
    }

    proc_tmp.proc_buff = proc->proc_buff;
    proc_tmp.buff_len  = proc->buff_len;

    mode_name_size = sizeof(mode_name);
    ret = blc_proc_mode_check(blc_ctx, mode_name, (td_u8)mode_name_size);
    if (ret != TD_SUCCESS) {
        return TD_FAILURE;
    }

    isp_proc_print_title(&proc_tmp, &proc->write_len, "black level actual info");

    isp_proc_printf(&proc_tmp, proc->write_len,
        "%10s" "%10s" "%11s" "%11s" "%11s" "%11s" "%11s" "%11s" "%11s" "%11s\n",
        "mode", "user_en", "isp_blc_r", "isp_blc_gr", "isp_blc_gb", "isp_blc_b",
        "sns_blc_r", "sns_blc_gr", "sns_blc_gb", "sns_blc_b");

    for (i = 0; i < blc_ctx->merge_frame; ++i) {
        isp_proc_printf(&proc_tmp, proc->write_len,
            "%10s" "%10u" "%11u" "%11u" "%11u" "%11u" "%11u" "%11u" "%11u" "%11u\n",
            mode_name, blc_ctx->user_black_level_en,
            blc_ctx->actual.isp_black_level[i][OT_ISP_CHN_R], blc_ctx->actual.isp_black_level[i][OT_ISP_CHN_GR],
            blc_ctx->actual.isp_black_level[i][OT_ISP_CHN_GB], blc_ctx->actual.isp_black_level[i][OT_ISP_CHN_B],
            blc_ctx->actual.sns_black_level[i][OT_ISP_CHN_R], blc_ctx->actual.sns_black_level[i][OT_ISP_CHN_GR],
            blc_ctx->actual.sns_black_level[i][OT_ISP_CHN_GB], blc_ctx->actual.sns_black_level[i][OT_ISP_CHN_B]);
    }

    proc->write_len += 1;

    return TD_SUCCESS;
}

static td_bool check_wdr_state(ot_vi_pipe vi_pipe, isp_usr_ctx *isp_ctx, isp_blacklevel_ctx *blc_ctx)
{
    td_u8   wdr_en;
    td_bool wdr_state_change;

    wdr_en = ot_ext_system_wdr_en_read(vi_pipe);

    if (is_2to1_wdr_mode(isp_ctx->sns_wdr_mode)) {
        wdr_state_change = (blc_ctx->wdr_mode_state == wdr_en) ? TD_FALSE : TD_TRUE;
    } else {
        wdr_state_change = TD_FALSE;
    }

    blc_ctx->wdr_mode_state = wdr_en;

    return wdr_state_change;
}

static td_s32 isp_blc_init(ot_vi_pipe vi_pipe, td_void *reg_cfg_info)
{
    td_s32 ret;
    isp_reg_cfg *reg_cfg = (isp_reg_cfg *)reg_cfg_info;
    isp_blacklevel_ctx *blc_ctx = TD_NULL;
    blacklevel_get_ctx(vi_pipe, blc_ctx);

    blc_ctx->init = TD_FALSE;
    ot_ext_system_isp_blc_init_status_write(vi_pipe, blc_ctx->init);

    ret = blc_initialize(vi_pipe);
    if (ret != TD_SUCCESS) {
        return ret;
    }
    blc_regs_initialize(vi_pipe, reg_cfg);
    blc_ext_regs_initialize(vi_pipe);

    blc_ctx->init = TD_TRUE;
    ot_ext_system_isp_blc_init_status_write(vi_pipe, blc_ctx->init);

    return TD_SUCCESS;
}

static td_void dp_calib_mode_blc_cfg(isp_usr_ctx *isp_ctx, isp_blacklevel_ctx *blc_ctx, isp_reg_cfg *reg_cfg)
{
    td_u8 i;

    if (isp_ctx->linkage.defect_pixel) {
        if (blc_ctx->pre_defect_pixel == TD_FALSE) {
            for (i = 0; i < reg_cfg->cfg_num; i++) {
                reg_cfg->alg_reg_cfg[i].be_blc_cfg.static_blc.wb_blc.blc_in = TD_TRUE;
                reg_cfg->alg_reg_cfg[i].be_blc_cfg.static_blc.resh_static   = TD_TRUE;
            }
        }
    } else if (blc_ctx->pre_defect_pixel) {
        for (i = 0; i < reg_cfg->cfg_num; i++) {
            reg_cfg->alg_reg_cfg[i].be_blc_cfg.static_blc.wb_blc.blc_in = TD_FALSE;
            reg_cfg->alg_reg_cfg[i].be_blc_cfg.static_blc.resh_static   = TD_TRUE;
        }
    }

    blc_ctx->pre_defect_pixel = isp_ctx->linkage.defect_pixel;
}

static td_void blc_regs_run(ot_vi_pipe vi_pipe, isp_usr_ctx *isp_ctx, isp_blacklevel_ctx *blc_ctx, isp_reg_cfg *reg_cfg)
{
    td_u8 i;

    get_isp_black_level_value(blc_ctx);
    fe_blc_dyna_regs(&reg_cfg->alg_reg_cfg[0].fe_blc_cfg.dyna_blc, blc_ctx);

    for (i = 0; i < reg_cfg->cfg_num; i++) {
        be_blc_dyna_regs(vi_pipe, isp_ctx->sns_wdr_mode, &reg_cfg->alg_reg_cfg[i].be_blc_cfg.dyna_blc, blc_ctx);
        update_wdr_sync_offset(&reg_cfg->alg_reg_cfg[i].wdr_reg_cfg.sync_reg_cfg,
                               reg_cfg->alg_reg_cfg[i].be_blc_cfg.dyna_blc.wdr_blc);
    }

    black_level_actual_value_write(vi_pipe, &blc_ctx->actual);
}

static td_s32 isp_blc_run(ot_vi_pipe vi_pipe, const td_void *stat_info, td_void *reg_cfg_info, td_s32 rsv)
{
    td_bool wdr_state_change = TD_FALSE;
    ot_isp_alg_mod alg_mod = OT_ISP_ALG_BLC;
    isp_usr_ctx             *isp_ctx         = TD_NULL;
    ot_isp_cmos_black_level *sns_black_level = TD_NULL;
    isp_blacklevel_ctx      *blc_ctx         = TD_NULL;
    isp_reg_cfg             *reg_cfg         = (isp_reg_cfg *)reg_cfg_info;
    size_t blc_size = OT_ISP_WDR_MAX_FRAME_NUM * OT_ISP_BAYER_CHN_NUM * sizeof(td_u16);
    ot_unused(stat_info);
    ot_unused(rsv);

    isp_get_ctx(vi_pipe, isp_ctx);
    blacklevel_get_ctx(vi_pipe, blc_ctx);

    ot_ext_system_isp_blc_init_status_write(vi_pipe, blc_ctx->init);
    if (blc_ctx->init != TD_TRUE) {
        ioctl(isp_get_fd(vi_pipe), ISP_ALG_INIT_ERR_INFO_PRINT, &alg_mod);
        return TD_SUCCESS;
    }

    wdr_state_change = check_wdr_state(vi_pipe, isp_ctx, blc_ctx);

    if (ot_ext_system_dpc_static_defect_type_read(vi_pipe) == 0) { /* hot pixel */
        dp_calib_mode_blc_cfg(isp_ctx, blc_ctx, reg_cfg);
    }

    blc_read_extregs(vi_pipe);

    reg_cfg->cfg_key.bit1_fe_blc_cfg = 1;
    reg_cfg->cfg_key.bit1_be_blc_cfg = 1;

    /* mannual mode update */
    if (blc_ctx->op_mode == OT_ISP_BLACK_LEVEL_MODE_MANUAL) {
        blc_regs_run(vi_pipe, isp_ctx, blc_ctx, reg_cfg);
        return TD_SUCCESS;
    }

    /* some sensors's blacklevel is changed with iso. */
    isp_sensor_get_blc(vi_pipe, &sns_black_level);
    isp_check_pointer_return(sns_black_level);

    /* sensors's blacklevel is changed by cmos. */
    if (sns_black_level->auto_attr.update == TD_TRUE) {
#ifdef CONFIG_OT_SNAP_SUPPORT
        if (isp_ctx->linkage.snap_pipe_mode != ISP_SNAP_NONE) {
            ot_vi_pipe main_pipe;
            if (vi_pipe == isp_ctx->linkage.picture_pipe_id) {
                main_pipe = isp_ctx->linkage.preview_pipe_id;
                isp_check_pipe_return(main_pipe);
                isp_sensor_get_blc(main_pipe, &sns_black_level);
            } else {
                isp_sensor_get_blc(vi_pipe, &sns_black_level);
            }

            (td_void)memcpy_s(&blc_ctx->black_level[0][0], blc_size,
                              &sns_black_level->auto_attr.black_level[0][0], blc_size);
        }
#endif
        isp_sensor_update_blc(vi_pipe);
        (td_void)memcpy_s(&blc_ctx->black_level[0][0], blc_size,
                          &sns_black_level->auto_attr.black_level[0][0], blc_size);

        blc_regs_run(vi_pipe, isp_ctx, blc_ctx, reg_cfg);

        return TD_SUCCESS;
    }

    /* sensors's blacklevel is changed by mpi. */
    if (((blc_ctx->black_level_change == TD_TRUE) && (blc_ctx->op_mode == OT_ISP_BLACK_LEVEL_MODE_AUTO)) ||
        (wdr_state_change == TD_TRUE)) {
        isp_sensor_update_blc(vi_pipe);
        (td_void)memcpy_s(&blc_ctx->black_level[0][0], blc_size,
                          &sns_black_level->auto_attr.black_level[0][0], blc_size);

        blc_regs_run(vi_pipe, isp_ctx, blc_ctx, reg_cfg);
    }

    return TD_SUCCESS;
}

static td_s32 isp_blc_ctrl(ot_vi_pipe vi_pipe, td_u32 cmd, td_void *value)
{
    isp_reg_cfg_attr *reg_cfg_attr = TD_NULL;

    switch (cmd) {
        case OT_ISP_CHANGE_IMAGE_MODE_SET:
        case OT_ISP_WDR_MODE_SET:
            isp_regcfg_get_ctx(vi_pipe, reg_cfg_attr);
            isp_check_pointer_return(reg_cfg_attr);
            isp_blc_init(vi_pipe, (td_void *)&reg_cfg_attr->reg_cfg);
            break;
        case OT_ISP_PROC_WRITE:
            isp_blc_proc_write(vi_pipe, (ot_isp_ctrl_proc_write *)value);
            break;
        default:
            break;
    }
    return TD_SUCCESS;
}

static td_s32 isp_blc_exit(ot_vi_pipe vi_pipe)
{
    ot_ext_system_isp_blc_init_status_write(vi_pipe, TD_FALSE);
    return TD_SUCCESS;
}

td_s32 isp_alg_register_blc(ot_vi_pipe vi_pipe)
{
    td_s32 ret;
    isp_usr_ctx *isp_ctx = TD_NULL;
    isp_alg_node *algs = TD_NULL;

    isp_get_ctx(vi_pipe, isp_ctx);
    isp_alg_check_return(isp_ctx->alg_key.bit1_blc);
    algs = isp_search_alg(isp_ctx->algs);
    isp_check_pointer_return(algs);
    ret = strncpy_s(algs->alg_name, sizeof(algs->alg_name), "blc", sizeof("blc"));
    isp_check_eok_return(ret, TD_FAILURE);
    algs->alg_type = OT_ISP_ALG_BLC;
    algs->alg_func.pfn_alg_init = isp_blc_init;
    algs->alg_func.pfn_alg_run  = isp_blc_run;
    algs->alg_func.pfn_alg_ctrl = isp_blc_ctrl;
    algs->alg_func.pfn_alg_exit = isp_blc_exit;
    algs->used = TD_TRUE;

    return TD_SUCCESS;
}

