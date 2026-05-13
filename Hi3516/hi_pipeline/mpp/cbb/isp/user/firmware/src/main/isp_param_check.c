/*
  Copyright (c), 2001-2025, Shenshu Tech. Co., Ltd.
 */

#include "isp_param_check.h"
#include "ot_common_isp.h"
#include "ot_common_3a.h"
#include "ot_common_ae.h"
#include "ot_common_awb.h"
#include "isp_main.h"
#include "mkp_isp.h"
#include "isp_ext_config.h"
#include "isp_aiisp_ext_config.h"
#include "isp_param_check_hal.h"

#define ISP_AI_DETAIL_MAX_STRENGTH 511

static td_s32 isp_sharpen_auto_motion_attr_check(const char *src, const ot_isp_sharpen_auto_attr *auto_attr)
{
    td_u8 i, j;
    for (i = 0; i < OT_ISP_AUTO_ISO_NUM; i++) {
        for (j = 0; j < OT_ISP_SHARPEN_GAIN_NUM; j++) {
            if (auto_attr->motion_texture_strength[j][i] > 0xFFF) {
                isp_err_trace("Err %s auto motion_texture_strength[%u][%u]:%u! range:[0, 4095]\n", src, j, i,
                              auto_attr->motion_texture_strength[j][i]);
                return OT_ERR_ISP_ILLEGAL_PARAM;
            }

            if (auto_attr->motion_edge_strength[j][i] > 0xFFF) {
                isp_err_trace("Err %s auto motion_edge_strength[%u][%u]:%u! range:[0, 4095]\n", src, j, i,
                              auto_attr->motion_edge_strength[j][i]);
                return OT_ERR_ISP_ILLEGAL_PARAM;
            }
        }

        if (auto_attr->motion_texture_freq[i] > 0xFFF) {
            isp_err_trace("Err %s auto motion_texture_freq[%u]:%u! range:[0, 4095]\n", src, i,
                          auto_attr->motion_texture_freq[i]);
            return OT_ERR_ISP_ILLEGAL_PARAM;
        }
        if (auto_attr->motion_edge_freq[i] > 0xFFF) {
            isp_err_trace("Err %s auto motion_edge_freq[%u]:%u! range:[0, 4095]\n", src, i,
                          auto_attr->motion_edge_freq[i]);
            return OT_ERR_ISP_ILLEGAL_PARAM;
        }

        if (auto_attr->motion_over_shoot[i] > 0x7F) {
            isp_err_trace("Err %s auto motion_over_shoot[%u]:%u! range:[0, 127]\n", src, i,
                          auto_attr->motion_over_shoot[i]);
            return OT_ERR_ISP_ILLEGAL_PARAM;
        }
        if (auto_attr->motion_under_shoot[i] > 0x7F) {
            isp_err_trace("Err %s auto motion_under_shoot[%u]:%u! range:[0, 127]\n", src, i,
                          auto_attr->motion_under_shoot[i]);
            return OT_ERR_ISP_ILLEGAL_PARAM;
        }
    }

    return TD_SUCCESS;
}

static td_s32 isp_sharpen_auto_strength_attr_check(const char *src, const ot_isp_sharpen_auto_attr *auto_attr)
{
    td_u8 i, j;
    for (i = 0; i < OT_ISP_AUTO_ISO_NUM; i++) {
        for (j = 0; j < OT_ISP_SHARPEN_GAIN_NUM; j++) {
            if (auto_attr->texture_strength[j][i] > 0xFFF) {
                isp_err_trace("Err %s auto texture_strength[%u][%u]:%u! range:[0, 4095]\n", src, j, i,
                              auto_attr->texture_strength[j][i]);
                return OT_ERR_ISP_ILLEGAL_PARAM;
            }
            if (auto_attr->edge_strength[j][i] > 0xFFF) {
                isp_err_trace("Err %s auto edge_strength[%u][%u]:%u! range:[0, 4095]\n", src, j, i,
                              auto_attr->edge_strength[j][i]);
                return OT_ERR_ISP_ILLEGAL_PARAM;
            }
        }
        for (j = 0; j < OT_ISP_SHARPEN_LUMA_NUM; j++) {
            if (auto_attr->luma_wgt[j][i] > OT_ISP_SHARPEN_LUMAWGT_MAX) {
                isp_err_trace("Err %s auto luma_wgt[%u][%u]:%u! range:[0, %d]\n", src, j, i, auto_attr->luma_wgt[j][i],
                              OT_ISP_SHARPEN_LUMAWGT_MAX);
                return OT_ERR_ISP_ILLEGAL_PARAM;
            }
        }
        if (auto_attr->texture_freq[i] > 0xFFF) {
            isp_err_trace("Err %s auto texture_freq[%u]:%u! range:[0, 4095]\n", src, i, auto_attr->texture_freq[i]);
            return OT_ERR_ISP_ILLEGAL_PARAM;
        }
        if (auto_attr->edge_freq[i] > 0xFFF) {
            isp_err_trace("Err %s auto edge_freq[%u]:%u! range:[0, 4095]\n", src, i, auto_attr->edge_freq[i]);
            return OT_ERR_ISP_ILLEGAL_PARAM;
        }
    }

    return TD_SUCCESS;
}

static td_s32 isp_sharpen_auto_shoot_attr_check(const char *src, const ot_isp_sharpen_auto_attr *auto_attr)
{
    td_u8 i;
    for (i = 0; i < OT_ISP_AUTO_ISO_NUM; i++) {
        if (auto_attr->over_shoot[i] > 0x7F) {
            isp_err_trace("Err %s auto over_shoot[%u]:%u! range:[0, 127]\n", src, i, auto_attr->over_shoot[i]);
            return OT_ERR_ISP_ILLEGAL_PARAM;
        }
        if (auto_attr->under_shoot[i] > 0x7F) {
            isp_err_trace("Err %s auto under_shoot[%u]:%u! range:[0, 127]\n", src, i, auto_attr->under_shoot[i]);
            return OT_ERR_ISP_ILLEGAL_PARAM;
        }
        if (auto_attr->shoot_sup_adj[i] > 0xF) {
            isp_err_trace("Err %s auto shoot_sup_adj[%u]:%u! range:[0, 15]\n", src, i, auto_attr->shoot_sup_adj[i]);
            return OT_ERR_ISP_ILLEGAL_PARAM;
        }
        if (auto_attr->edge_filt_strength[i] > 0x3F) {
            isp_err_trace("Err %s auto edge_filt_strength[%u]:%u! range:[0, 63]\n", src, i,
                          auto_attr->edge_filt_strength[i]);
            return OT_ERR_ISP_ILLEGAL_PARAM;
        }

        if (auto_attr->edge_filt_max_cap[i] > OT_ISP_SHARPEN_EDGE_FILT_MAX_CAP_MAX) {
            isp_err_trace("Err %s auto edge_filt_max_cap[%u]:%u! range:[0, %u]\n", src, i,
                          auto_attr->edge_filt_max_cap[i], OT_ISP_SHARPEN_EDGE_FILT_MAX_CAP_MAX);
            return OT_ERR_ISP_ILLEGAL_PARAM;
        }

        if (auto_attr->r_gain[i] > OT_ISP_SHARPEN_RGAIN_MAX) {
            isp_err_trace("Err %s auto r_gain[%u]:%u! range:[0, %d]\n", src, i, auto_attr->r_gain[i],
                          OT_ISP_SHARPEN_RGAIN_MAX);
            return OT_ERR_ISP_ILLEGAL_PARAM;
        }

        if (auto_attr->b_gain[i] > OT_ISP_SHARPEN_BGAIN_MAX) {
            isp_err_trace("Err %s auto b_gain[%u]:%u! range:[0, %d]\n", src, i, auto_attr->b_gain[i],
                          OT_ISP_SHARPEN_BGAIN_MAX);
            return OT_ERR_ISP_ILLEGAL_PARAM;
        }

        if (auto_attr->skin_gain[i] > 0x1F) {
            isp_err_trace("Err %s auto skin_gain[%u]:%u! range:[0, 31]\n", src, i, auto_attr->skin_gain[i]);
            return OT_ERR_ISP_ILLEGAL_PARAM;
        }

        if (auto_attr->max_sharp_gain[i] > 0x7FF) {
            isp_err_trace("Err %s auto max_sharp_gain[%u]:%u! range:[0, 2047]\n", src, i, auto_attr->max_sharp_gain[i]);
            return OT_ERR_ISP_ILLEGAL_PARAM;
        }
    }

    return TD_SUCCESS;
}

static td_s32 isp_sharpen_manual_strength_attr_check(const char *src, const ot_isp_sharpen_manual_attr *manual_attr)
{
    td_u8 j;
    for (j = 0; j < OT_ISP_SHARPEN_GAIN_NUM; j++) {
        if (manual_attr->texture_strength[j] > 0xFFF) {
            isp_err_trace("Err %s manual texture_strength[%u]:%u! range:[0,4095]\n", src, j,
                manual_attr->texture_strength[j]);
            return OT_ERR_ISP_ILLEGAL_PARAM;
        }
        if (manual_attr->edge_strength[j] > 0xFFF) {
            isp_err_trace("Err %s manual edge_strength[%u]:%u! range:[0, 4095]\n", src, j,
                manual_attr->edge_strength[j]);
            return OT_ERR_ISP_ILLEGAL_PARAM;
        }
    }
    for (j = 0; j < OT_ISP_SHARPEN_LUMA_NUM; j++) {
        if (manual_attr->luma_wgt[j] > OT_ISP_SHARPEN_LUMAWGT_MAX) {
            isp_err_trace("Err %s manual luma_wgt[%u]:%u! range:[0, %d]\n", src, j, manual_attr->luma_wgt[j],
                OT_ISP_SHARPEN_LUMAWGT_MAX);
            return OT_ERR_ISP_ILLEGAL_PARAM;
        }
    }
    if (manual_attr->texture_freq > 0xFFF) {
        isp_err_trace("Err %s manual texture_freq:%u! range:[0, 4095]\n", src, manual_attr->texture_freq);
        return OT_ERR_ISP_ILLEGAL_PARAM;
    }
    if (manual_attr->edge_freq > 0xFFF) {
        isp_err_trace("Err %s manual edge_freq:%u! range:[0, 4095]\n", src, manual_attr->edge_freq);
        return OT_ERR_ISP_ILLEGAL_PARAM;
    }

    return TD_SUCCESS;
}

static td_s32 isp_sharpen_manual_shoot_attr_check(const char *src, const ot_isp_sharpen_manual_attr *manual_attr)
{
    if (manual_attr->over_shoot > 0x7F) {
        isp_err_trace("Err %s manual over_shoot:%u! range:[0, 127]\n", src, manual_attr->over_shoot);
        return OT_ERR_ISP_ILLEGAL_PARAM;
    }
    if (manual_attr->under_shoot > 0x7F) {
        isp_err_trace("Err %s manual under_shoot:%u! range:[0, 127]\n", src, manual_attr->under_shoot);
        return OT_ERR_ISP_ILLEGAL_PARAM;
    }
    if (manual_attr->shoot_sup_adj > 0xF) {
        isp_err_trace("Err %s manual shoot_sup_adj:%u! range:[0, 15]\n", src, manual_attr->shoot_sup_adj);
        return OT_ERR_ISP_ILLEGAL_PARAM;
    }
    if (manual_attr->edge_filt_strength > 0x3F) {
        isp_err_trace("Err %s manual edge_filt_strength:%u! range:[0, 63]\n", src, manual_attr->edge_filt_strength);
        return OT_ERR_ISP_ILLEGAL_PARAM;
    }
    if (manual_attr->edge_filt_max_cap > OT_ISP_SHARPEN_EDGE_FILT_MAX_CAP_MAX) {
        isp_err_trace("Err %s manual edge_filt_max_cap:%u! range:[0, %d]\n", src,
                      manual_attr->edge_filt_max_cap, OT_ISP_SHARPEN_EDGE_FILT_MAX_CAP_MAX);
        return OT_ERR_ISP_ILLEGAL_PARAM;
    }
    if (manual_attr->r_gain > OT_ISP_SHARPEN_RGAIN_MAX) {
        isp_err_trace("Err %s manual r_gain:%u! range:[0, %d]\n", src, manual_attr->r_gain, OT_ISP_SHARPEN_RGAIN_MAX);
        return OT_ERR_ISP_ILLEGAL_PARAM;
    }
    if (manual_attr->b_gain > OT_ISP_SHARPEN_BGAIN_MAX) {
        isp_err_trace("Err %s manual b_gain:%u! range:[0, %d]\n", src, manual_attr->b_gain, OT_ISP_SHARPEN_BGAIN_MAX);
        return OT_ERR_ISP_ILLEGAL_PARAM;
    }
    if (manual_attr->skin_gain > 0x1F) {
        isp_err_trace("Err %s manual skin_gain:%u! range:[0, 31]\n", src, manual_attr->skin_gain);
        return OT_ERR_ISP_ILLEGAL_PARAM;
    }
    if (manual_attr->max_sharp_gain > 0x7FF) {
        isp_err_trace("Err %s manual max_sharp_gain:%u! range:[0, 2047]\n", src, manual_attr->max_sharp_gain);
        return OT_ERR_ISP_ILLEGAL_PARAM;
    }

    return TD_SUCCESS;
}

static td_s32 isp_sharpen_manual_motion_attr_check(const char *src, const ot_isp_sharpen_manual_attr *manual_attr)
{
    td_u8 j;
    for (j = 0; j < OT_ISP_SHARPEN_GAIN_NUM; j++) {
        if (manual_attr->motion_texture_strength[j] > 0xFFF) {
            isp_err_trace("Err %s manual motion_texture_strength[%u]:%u! range:[0,4095]\n", src, j,
                          manual_attr->motion_texture_strength[j]);
            return OT_ERR_ISP_ILLEGAL_PARAM;
        }
        if (manual_attr->motion_edge_strength[j] > 0xFFF) {
            isp_err_trace("Err %s manual motion_edge_strength[%u]:%u! range:[0, 4095]\n", src, j,
                          manual_attr->motion_edge_strength[j]);
            return OT_ERR_ISP_ILLEGAL_PARAM;
        }
    }
    if (manual_attr->motion_texture_freq > 0xFFF) {
        isp_err_trace("Err %s manual motion_texture_freq:%u! range:[0, 4095]\n", src, manual_attr->motion_texture_freq);
        return OT_ERR_ISP_ILLEGAL_PARAM;
    }
    if (manual_attr->motion_edge_freq > 0xFFF) {
        isp_err_trace("Err %s manual motion_edge_freq:%u! range:[0, 4095]\n", src, manual_attr->motion_edge_freq);
        return OT_ERR_ISP_ILLEGAL_PARAM;
    }
    if (manual_attr->motion_over_shoot > 0x7F) {
        isp_err_trace("Err %s manual motion_over_shoot:%u! range:[0, 127]\n", src, manual_attr->motion_over_shoot);
        return OT_ERR_ISP_ILLEGAL_PARAM;
    }
    if (manual_attr->motion_under_shoot > 0x7F) {
        isp_err_trace("Err %s manual motion_under_shoot:%u! range:[0, 127]\n", src, manual_attr->motion_under_shoot);
        return OT_ERR_ISP_ILLEGAL_PARAM;
    }

    return TD_SUCCESS;
}

td_s32 isp_sharpen_auto_attr_check(const char *src, const ot_isp_sharpen_auto_attr *auto_attr)
{
    td_s32 ret;

    ret = isp_sharpen_auto_attr_arch_check(src, auto_attr);
    if (ret != TD_SUCCESS) {
        return ret;
    }

    ret = isp_sharpen_auto_strength_attr_check(src, auto_attr);
    if (ret != TD_SUCCESS) {
        return ret;
    }

    ret = isp_sharpen_auto_shoot_attr_check(src, auto_attr);
    if (ret != TD_SUCCESS) {
        return ret;
    }

    ret = isp_sharpen_auto_motion_attr_check(src, auto_attr);
    if (ret != TD_SUCCESS) {
        return ret;
    }

    return TD_SUCCESS;
}

td_s32 isp_sharpen_manual_attr_check(const char *src, const ot_isp_sharpen_manual_attr *manual_attr)
{
    td_s32 ret;

    ret = isp_sharpen_manual_attr_arch_check(src, manual_attr);
    if (ret != TD_SUCCESS) {
        return ret;
    }

    ret = isp_sharpen_manual_strength_attr_check(src, manual_attr);
    if (ret != TD_SUCCESS) {
        return ret;
    }

    ret = isp_sharpen_manual_shoot_attr_check(src, manual_attr);
    if (ret != TD_SUCCESS) {
        return ret;
    }

    ret = isp_sharpen_manual_motion_attr_check(src, manual_attr);
    if (ret != TD_SUCCESS) {
        return ret;
    }

    return TD_SUCCESS;
}

td_s32 isp_sharpen_comm_attr_check(const char *src, const ot_isp_sharpen_attr *shp_attr)
{
    td_s32 ret;
    isp_check_bool_return(shp_attr->enable);
    isp_check_bool_return(shp_attr->motion_en);

    ret = isp_sharpen_common_attr_arch_check(src, shp_attr);
    if (ret != TD_SUCCESS) {
        return OT_ERR_ISP_ILLEGAL_PARAM;
    }

    if (shp_attr->op_type >= OT_OP_MODE_BUTT) {
        isp_err_trace("Err sharpen type %d!\n", shp_attr->op_type);
        return OT_ERR_ISP_ILLEGAL_PARAM;
    }
    if (shp_attr->motion_threshold0 > 15) {   /* range:[0, 15] */
        isp_err_trace("Err %s motion_threshold0 %u! range:[0, 15]\n", src, shp_attr->motion_threshold0);
        return OT_ERR_ISP_ILLEGAL_PARAM;
    }
    if (shp_attr->motion_threshold1 > 15) {   /* range:[0, 15] */
        isp_err_trace("Err %s motion_threshold1 %u! range:[0, 15]\n", src, shp_attr->motion_threshold1);
        return OT_ERR_ISP_ILLEGAL_PARAM;
    }
    if (shp_attr->motion_threshold0 > shp_attr->motion_threshold1) {
        isp_err_trace("Err %s: motion_threshold0 should not larger than motion_threshold1!\n", src);
        return OT_ERR_ISP_ILLEGAL_PARAM;
    }
    if (shp_attr->motion_gain0 > 256) {   /* range:[0, 256] */
        isp_err_trace("Err %s motion_gain0 %u! range:[0, 256]\n", src, shp_attr->motion_gain0);
        return OT_ERR_ISP_ILLEGAL_PARAM;
    }
    if (shp_attr->motion_gain1 > 256) {   /* range:[0, 256] */
        isp_err_trace("Err %s motion_gain1 %u! range:[0, 256]\n", src, shp_attr->motion_gain1);
        return OT_ERR_ISP_ILLEGAL_PARAM;
    }
    if (shp_attr->skin_umin > shp_attr->skin_umax) {
        isp_err_trace("Err %s: skin_umin should not larger than skin_umax!\n", src);
        return OT_ERR_ISP_ILLEGAL_PARAM;
    }
    if (shp_attr->skin_vmin > shp_attr->skin_vmax) {
        isp_err_trace("Err %s: skin_vmin should not larger than skin_vmax!\n", src);
        return OT_ERR_ISP_ILLEGAL_PARAM;
    }

    return TD_SUCCESS;
}

td_s32 isp_drc_attr_check(const char *src, const ot_isp_drc_attr *drc_attr)
{
    td_s32 i;
    isp_check_bool_return(drc_attr->enable);
    isp_check_bool_return(drc_attr->shoot_reduction_en);

    if ((drc_attr->asymmetry_curve.asymmetry > 0x1E) || (drc_attr->asymmetry_curve.asymmetry < 0x1)) {
        isp_err_trace("Err %s asymmetry: %u, range: [%u, %u]!\n", src, drc_attr->asymmetry_curve.asymmetry,
            0x1, 0x1E);
        return OT_ERR_ISP_ILLEGAL_PARAM;
    }

    if ((drc_attr->asymmetry_curve.second_pole > 0xD2) || (drc_attr->asymmetry_curve.second_pole < 0x96)) {
        isp_err_trace("Err %s second_pole: %u, range: [%u, %u]!\n", src, drc_attr->asymmetry_curve.second_pole,
            0x96, 0xD2);
        return OT_ERR_ISP_ILLEGAL_PARAM;
    }

    if ((drc_attr->asymmetry_curve.stretch > 0x3C) || (drc_attr->asymmetry_curve.stretch < 0x1E)) {
        isp_err_trace("Err %s stretch: %u, range: [%u, %u]!\n", src, drc_attr->asymmetry_curve.stretch,
            0x1E, 0x3C);
        return OT_ERR_ISP_ILLEGAL_PARAM;
    }

    if ((drc_attr->asymmetry_curve.compress > 0xc8) || (drc_attr->asymmetry_curve.compress < 0x64)) {
        isp_err_trace("Err %s compress: %u, range: [%u, %u]!\n", src, drc_attr->asymmetry_curve.compress,
            0x64, 0xc8);
        return OT_ERR_ISP_ILLEGAL_PARAM;
    }

    if (drc_attr->op_type >= OT_OP_MODE_BUTT) {
        isp_err_trace("Err %s op_type: %d!\n", src, drc_attr->op_type);
        return OT_ERR_ISP_ILLEGAL_PARAM;
    }

    if (drc_attr->auto_attr.strength > OT_ISP_DRC_STRENGTH_MAX) {
        isp_err_trace("Err %s auto_attr.strength: %u, range: [%u, %u]!\n", src, drc_attr->auto_attr.strength,
            0, OT_ISP_DRC_STRENGTH_MAX);
        return OT_ERR_ISP_ILLEGAL_PARAM;
    }

    if (drc_attr->auto_attr.strength_max > OT_ISP_DRC_STRENGTH_MAX) {
        isp_err_trace("Err %s auto_attr.strength_max: %u, range: [%u, %u]!\n", src,
            drc_attr->auto_attr.strength_max, 0, OT_ISP_DRC_STRENGTH_MAX);
        return OT_ERR_ISP_ILLEGAL_PARAM;
    }

    if (drc_attr->auto_attr.strength_min > OT_ISP_DRC_STRENGTH_MAX) {
        isp_err_trace("Err %s auto_attr.strength_min: %u, range: [%u, %u]!\n", src,
            drc_attr->auto_attr.strength_min, 0, OT_ISP_DRC_STRENGTH_MAX);
        return OT_ERR_ISP_ILLEGAL_PARAM;
    }

    if (drc_attr->auto_attr.strength_max < drc_attr->auto_attr.strength_min) {
        isp_err_trace("Err %s auto_attr.strength_max (%u) must be greater than auto_attr.strength_min (%u)!\n",
            src, drc_attr->auto_attr.strength_max, drc_attr->auto_attr.strength_min);
        return OT_ERR_ISP_ILLEGAL_PARAM;
    }

    if (drc_attr->manual_attr.strength > OT_ISP_DRC_STRENGTH_MAX) {
        isp_err_trace("Err %s manual_attr.strength: %u, range: [%u, %u]!\n", src, drc_attr->manual_attr.strength,
            0, OT_ISP_DRC_STRENGTH_MAX);
        return OT_ERR_ISP_ILLEGAL_PARAM;
    }

    if (drc_attr->bright_gain_limit > 0xF) {
        isp_err_trace("Err %s bright_gain_limit: %u, range: [%u, %u]!\n", src, drc_attr->bright_gain_limit,
            0, 0xF);
        return OT_ERR_ISP_ILLEGAL_PARAM;
    }

    if (drc_attr->bright_gain_limit_step > 0xF) {
        isp_err_trace("Err %s bright_gain_limit_step: %u, range: [%u, %u]!\n", src,
            drc_attr->bright_gain_limit_step, 0, 0xF);
        return OT_ERR_ISP_ILLEGAL_PARAM;
    }

    if (drc_attr->dark_gain_limit_luma > 0x85) {
        isp_err_trace("Err %s dark_gain_limit_luma: %u, range: [%u, %u]!\n", src, drc_attr->dark_gain_limit_luma,
            0, 0x85);
        return OT_ERR_ISP_ILLEGAL_PARAM;
    }

    if (drc_attr->dark_gain_limit_chroma > 0x85) {
        isp_err_trace("Err %s dark_gain_limit_chroma: %u, range: [%u, %u]!\n", src,
            drc_attr->dark_gain_limit_chroma, 0, 0x85);
        return OT_ERR_ISP_ILLEGAL_PARAM;
    }

    if (drc_attr->purple_reduction_strength > 0x80) {
        isp_err_trace("Err %s purple_reduction_strength: %u, range: [%u, %u]!\n", src,
            drc_attr->purple_reduction_strength, 0, 0x80);
        return OT_ERR_ISP_ILLEGAL_PARAM;
    }

    if (drc_attr->contrast_ctrl > 0xF) {
        isp_err_trace("Err %s contrast_ctrl: %u, range: [%u, %u]!\n", src, drc_attr->contrast_ctrl,
            0, 0xF);
        return OT_ERR_ISP_ILLEGAL_PARAM;
    }

    if (drc_attr->spatial_filter_coef > OT_ISP_DRC_SPA_FLT_COEF_MAX) {
        isp_err_trace("Err %s spatial_filter_coef: %u, range: [%u, %u]!\n", src, drc_attr->spatial_filter_coef,
            0, OT_ISP_DRC_SPA_FLT_COEF_MAX);
        return OT_ERR_ISP_ILLEGAL_PARAM;
    }

    if (drc_attr->range_filter_coef > 0xA) {
        isp_err_trace("Err %s range_filter_coef: %u, range: [%u, %u]!\n", src, drc_attr->range_filter_coef,
            0, 0xA);
        return OT_ERR_ISP_ILLEGAL_PARAM;
    }

    if (drc_attr->rim_reduction_strength > 0x40) {
        isp_err_trace("Err %s rim_reduction_strength: %u, range: [%u, %u]!\n", src,
            drc_attr->rim_reduction_strength, 0, 0x40);
        return OT_ERR_ISP_ILLEGAL_PARAM;
    }

    if (drc_attr->rim_reduction_threshold > 0x80) {
        isp_err_trace("Err %s rim_reduction_threshold: %u, range: [%u, %u]!\n", src,
            drc_attr->rim_reduction_threshold, 0, 0x80);
        return OT_ERR_ISP_ILLEGAL_PARAM;
    }

    if (drc_attr->detail_adjust_coef > 0xF) {
        isp_err_trace("Err %s detail_adjust_coef: %u, range: [%u, %u]!\n", src, drc_attr->detail_adjust_coef,
            0, 0xF);
        return OT_ERR_ISP_ILLEGAL_PARAM;
    }

    if (drc_attr->high_saturation_color_ctrl > 0xF) {
        isp_err_trace("Err %s high_saturation_color_ctrl: %u, range: [%u, %u]!\n", src,
            drc_attr->high_saturation_color_ctrl, 0, 0xF);
        return OT_ERR_ISP_ILLEGAL_PARAM;
    }

    if (drc_attr->global_color_ctrl > 0xF) {
        isp_err_trace("Err %s global_color_ctrl: %u, range: [%u, %u]!\n", src, drc_attr->global_color_ctrl,
            0, 0xF);
        return OT_ERR_ISP_ILLEGAL_PARAM;
    }

    for (i = 0; i < OT_ISP_DRC_CC_NODE_NUM; i++) {
        if (drc_attr->color_correction_lut[i] > OT_ISP_DRC_CC_COEF_MAX) {
            isp_err_trace("Err %s color_correction_lut[%d]: %u, range: [%u, %u]!\n", src, i,
                drc_attr->color_correction_lut[i], 0, OT_ISP_DRC_CC_COEF_MAX);
            return OT_ERR_ISP_ILLEGAL_PARAM;
        }
    }

    if (isp_drc_attr_arch_check(src, drc_attr) != TD_SUCCESS) {
        return OT_ERR_ISP_ILLEGAL_PARAM;
    }

    return TD_SUCCESS;
}

#ifdef CONFIG_OT_ISP_PREGAMMA_SUPPORT
td_s32 isp_pregamma_attr_check(const char *src, const ot_isp_pregamma_attr *pregamma_attr)
{
    td_u32 i;
    isp_check_bool_return(pregamma_attr->enable);
    for (i = 0; i < OT_ISP_PREGAMMA_NODE_NUM; i++) {
        if (pregamma_attr->table[i] > OT_ISP_PREGAMMA_LUT_MAX) {
            isp_err_trace("Err %s pregamma table[%u]: %u, range: [%u, %u]!\n", src, i, pregamma_attr->table[i],
                0, OT_ISP_PREGAMMA_LUT_MAX);
            return OT_ERR_ISP_ILLEGAL_PARAM;
        }
    }
    return TD_SUCCESS;
}
#endif
static td_s32 isp_demosaic_auto_attr_check(const char *src, const ot_isp_demosaic_auto_attr *auto_attr)
{
    td_s32 i;
    for (i = 0; i < OT_ISP_AUTO_ISO_NUM; i++) {
        if (auto_attr->nddm_mf_detail_strength[i] > OT_ISP_DEMOSAIC_NONDIR_MFDETALEHC_STR_MAX) {
            isp_err_trace("Err %s nddm_mf_detail_strength[%d] %u, range: [%u, %u]!\n", src, i,
                auto_attr->nddm_mf_detail_strength[i], 0, OT_ISP_DEMOSAIC_NONDIR_MFDETALEHC_STR_MAX);
            return OT_ERR_ISP_ILLEGAL_PARAM;
        }
        if ((auto_attr->detail_smooth_range[i] > OT_ISP_DEMOSAIC_DETAIL_SMOOTH_RANGE_MAX) ||
            (auto_attr->detail_smooth_range[i] < OT_ISP_DEMOSAIC_DETAIL_SMOOTH_RANGE_MIN)) {
            isp_err_trace("Err %s detail_smooth_range[%d] %u, range: [%u, %u]!\n", src, i,
                auto_attr->detail_smooth_range[i], OT_ISP_DEMOSAIC_DETAIL_SMOOTH_RANGE_MIN,
                OT_ISP_DEMOSAIC_DETAIL_SMOOTH_RANGE_MAX);
            return OT_ERR_ISP_ILLEGAL_PARAM;
        }
        if (auto_attr->color_noise_f_strength[i] > 0x8) {
            isp_err_trace("Err %s color_noise_f_strength[%d] %u, range: [%u, %u]!\n", src, i,
                auto_attr->color_noise_f_strength[i], 0, 0x8);
            return OT_ERR_ISP_ILLEGAL_PARAM;
        }
        if (auto_attr->color_noise_y_threshold[i] > OT_ISP_DEMOSAIC_COLOR_NOISE_THDY_MAX) {
            isp_err_trace("Err %s color_noise_y_threshold[%d] %u, range: [%u, %u]!\n", src, i,
                auto_attr->color_noise_y_threshold[i], 0, OT_ISP_DEMOSAIC_COLOR_NOISE_THDY_MAX);
            return OT_ERR_ISP_ILLEGAL_PARAM;
        }
        if (auto_attr->color_noise_y_strength[i] > OT_ISP_DEMOSAIC_COLOR_NOISE_STRY_MAX) {
            isp_err_trace("Err %s color_noise_y_strength[%d] %u, range: [%u, %u]!\n", src, i,
                auto_attr->color_noise_y_strength[i], 0, OT_ISP_DEMOSAIC_COLOR_NOISE_STRY_MAX);
            return OT_ERR_ISP_ILLEGAL_PARAM;
        }
    }
    return isp_demosaic_auto_attr_arch_check(src, auto_attr);
}

static td_s32 isp_demosaic_manual_attr_check(const char *src, const ot_isp_demosaic_manual_attr *manual_attr)
{
    if (manual_attr->nddm_mf_detail_strength > OT_ISP_DEMOSAIC_NONDIR_MFDETALEHC_STR_MAX) {
        isp_err_trace("Err %s nddm_mf_detail_strength %u, range: [%u, %u]!\n", src,
            manual_attr->nddm_mf_detail_strength, 0, OT_ISP_DEMOSAIC_NONDIR_MFDETALEHC_STR_MAX);
        return OT_ERR_ISP_ILLEGAL_PARAM;
    }
    if ((manual_attr->detail_smooth_range > OT_ISP_DEMOSAIC_DETAIL_SMOOTH_RANGE_MAX) ||
        (manual_attr->detail_smooth_range < OT_ISP_DEMOSAIC_DETAIL_SMOOTH_RANGE_MIN)) {
        isp_err_trace("Err %s detail_smooth_range %u, range: [%u, %u]!\n", src, manual_attr->detail_smooth_range,
            OT_ISP_DEMOSAIC_DETAIL_SMOOTH_RANGE_MIN, OT_ISP_DEMOSAIC_DETAIL_SMOOTH_RANGE_MAX);
        return OT_ERR_ISP_ILLEGAL_PARAM;
    }
    if (manual_attr->color_noise_f_strength > 0x8) {
        isp_err_trace("Err %s color_noise_f_strength %u, range: [%u, %u]!\n", src,
            manual_attr->color_noise_f_strength, 0, 0x8);
        return OT_ERR_ISP_ILLEGAL_PARAM;
    }
    if (manual_attr->color_noise_y_threshold > OT_ISP_DEMOSAIC_COLOR_NOISE_THDY_MAX) {
        isp_err_trace("Err %s color_noise_y_threshold %u, range: [%u, %u]!\n", src,
            manual_attr->color_noise_y_threshold, 0, OT_ISP_DEMOSAIC_COLOR_NOISE_THDY_MAX);
        return OT_ERR_ISP_ILLEGAL_PARAM;
    }
    if (manual_attr->color_noise_y_strength > OT_ISP_DEMOSAIC_COLOR_NOISE_STRY_MAX) {
        isp_err_trace("Err %s color_noise_y_strength %u, range: [%u, %u]!\n", src,
            manual_attr->color_noise_y_strength, 0, OT_ISP_DEMOSAIC_COLOR_NOISE_STRY_MAX);
        return OT_ERR_ISP_ILLEGAL_PARAM;
    }
    return isp_demosaic_manual_attr_arch_check(src, manual_attr);
}

static td_s32 isp_demosaic_common_attr_check(const char *src, const ot_isp_demosaic_attr *dem_attr)
{
    if (dem_attr->ai_detail_strength > ISP_AI_DETAIL_MAX_STRENGTH) {
        isp_err_trace("Err %s ai_detail_strength %u, range: [0, %u]!\n", src, dem_attr->ai_detail_strength,
            ISP_AI_DETAIL_MAX_STRENGTH);
        return OT_ERR_ISP_ILLEGAL_PARAM;
    }
    return TD_SUCCESS;
}

td_s32 isp_demosaic_attr_check(const char *src, const ot_isp_demosaic_attr *dem_attr)
{
    td_s32 ret;
    if (dem_attr->op_type >= OT_OP_MODE_BUTT) {
        isp_err_trace("Err %s op_type %d!\n", src, dem_attr->op_type);
        return OT_ERR_ISP_ILLEGAL_PARAM;
    }

    ret = isp_demosaic_manual_attr_check(src, &dem_attr->manual_attr);
    if (ret != TD_SUCCESS) {
        return ret;
    }
    ret = isp_demosaic_auto_attr_check(src, &dem_attr->auto_attr);
    if (ret != TD_SUCCESS) {
        return ret;
    }
    ret = isp_demosaic_common_attr_check(src, dem_attr);
    if (ret != TD_SUCCESS) {
        return ret;
    }

    return TD_SUCCESS;
}

td_s32 isp_cac_comm_attr_check(const char *src, const ot_isp_cac_attr *cac_attr)
{
    if (cac_attr->op_type >= OT_OP_MODE_BUTT) {
        isp_err_trace("Err %s op_type!\n", src);
        return OT_ERR_ISP_ILLEGAL_PARAM;
    }
    if (cac_attr->detect_mode > 2) { /* range : [0:2] */
        isp_err_trace("Err %s detect_mode:%u! range:[0, 2]\n", src, cac_attr->detect_mode);
        return OT_ERR_ISP_ILLEGAL_PARAM;
    }
    if ((cac_attr->purple_upper_limit > OT_ISP_CAC_PURPLE_LIM_MAX) ||
        (cac_attr->purple_upper_limit < OT_ISP_CAC_PURPLE_LIM_MIN)) {
        isp_err_trace("Err %s purple_upper_limit:%d! range:[%d, %d]\n", src, cac_attr->purple_upper_limit,
            OT_ISP_CAC_PURPLE_LIM_MIN, OT_ISP_CAC_PURPLE_LIM_MAX);
        return OT_ERR_ISP_ILLEGAL_PARAM;
    }
    if ((cac_attr->purple_lower_limit > OT_ISP_CAC_PURPLE_LIM_MAX) ||
        (cac_attr->purple_lower_limit < OT_ISP_CAC_PURPLE_LIM_MIN)) {
        isp_err_trace("Err %s purple_lower_limit:%d! range:[%d, %d]\n", src, cac_attr->purple_lower_limit,
            OT_ISP_CAC_PURPLE_LIM_MIN, OT_ISP_CAC_PURPLE_LIM_MAX);
        return OT_ERR_ISP_ILLEGAL_PARAM;
    }
    if (cac_attr->purple_upper_limit <= cac_attr->purple_lower_limit) {
        isp_err_trace("Err %s purple_upper_limit:%d should be greater than purple_lower_limit:%d!\n",
            src, cac_attr->purple_upper_limit, cac_attr->purple_lower_limit);
        return OT_ERR_ISP_ILLEGAL_PARAM;
    }
    return TD_SUCCESS;
}

static td_s32 isp_cac_lcac_comm_attr_check(const char *src, const ot_isp_cac_lcac_attr *lcac_cfg)
{
    td_u8 i;
    if (lcac_cfg->var_threshold > 0xFFF) {
        isp_err_trace("Err %s var_threshold:%u! range:[0, 4095]\n", src, lcac_cfg->var_threshold);
        return OT_ERR_ISP_ILLEGAL_PARAM;
    }
    if (lcac_cfg->purple_detect_range > 410) { /* Range:[0, 410] */
        isp_err_trace("Err %s purple_detect_range:%u! range:[0, 410]\n", src, lcac_cfg->purple_detect_range);
        return OT_ERR_ISP_ILLEGAL_PARAM;
    }
    for (i = 0; i < OT_ISP_CAC_CURVE_NUM; i++) {
        if (lcac_cfg->r_detect_threshold[i] > 4095) { /* Range:[0, 4095] */
            isp_err_trace("Err %s r_detect_threshold[%u]:%u! range:[0, 4095]\n", src, i,
                lcac_cfg->r_detect_threshold[i]);
            return OT_ERR_ISP_ILLEGAL_PARAM;
        } else if (lcac_cfg->g_detect_threshold[i] > 4095) { /* Range:[0, 4095] */
            isp_err_trace("Err %s g_detect_threshold[%u]:%u! range:[0, 4095]\n", src, i,
                lcac_cfg->g_detect_threshold[i]);
            return OT_ERR_ISP_ILLEGAL_PARAM;
        } else if (lcac_cfg->b_detect_threshold[i] > 4095) { /* Range:[0, 4095] */
            isp_err_trace("Err %s b_detect_threshold[%u]:%u! range:[0, 4095]\n", src, i,
                lcac_cfg->b_detect_threshold[i]);
            return OT_ERR_ISP_ILLEGAL_PARAM;
        }
    }
    return TD_SUCCESS;
}

static td_s32 isp_cac_lcac_auto_attr_check(const char *src, const ot_isp_cac_lcac_auto_attr *lcac_auto)
{
    td_u8 i;
    for (i = 0; i < OT_ISP_CAC_EXP_RATIO_NUM; i++) {
        if (lcac_auto->de_purple_cr_strength[i] > 0x8) { /* max is 8 */
            isp_err_trace("Err %s depur_cr_strength[%u]:%u! range:[0, 8]\n", src, i,
                lcac_auto->de_purple_cr_strength[i]);
            return OT_ERR_ISP_ILLEGAL_PARAM;
        } else if (lcac_auto->de_purple_cb_strength[i] > 0x8) { /* max is 8 */
            isp_err_trace("Err %s depur_cb_strength[%u]:%u! range:[0, 8]\n", src, i,
                lcac_auto->de_purple_cb_strength[i]);
            return OT_ERR_ISP_ILLEGAL_PARAM;
        }
    }
    return TD_SUCCESS;
}

static td_s32 isp_cac_lcac_manual_attr_check(const char *src, const ot_isp_cac_lcac_manual_attr *lcac_manual)
{
    if (lcac_manual->de_purple_cr_strength > 0x8) { /* max is 8 */
        isp_err_trace("Err %s depur_cr_strength:%u! range:[0, 8]\n", src, lcac_manual->de_purple_cr_strength);
        return OT_ERR_ISP_ILLEGAL_PARAM;
    } else if (lcac_manual->de_purple_cb_strength > 0x8) { /* max is 8 */
        isp_err_trace("Err %s depur_cb_strength:%u! range:[0, 8]\n", src, lcac_manual->de_purple_cb_strength);
        return OT_ERR_ISP_ILLEGAL_PARAM;
    }
    return TD_SUCCESS;
}

static td_s32 isp_cac_acac_auto_attr_check(const char *src, const ot_isp_cac_acac_auto_attr *acac_auto)
{
    td_u8 i, j;
    for (i = 0; i < OT_ISP_AUTO_ISO_NUM; i++) {
        if (acac_auto->edge_gain[i] > OT_ISP_CAC_ACAC_EDGE_GAIN_MAX) {
            isp_err_trace("Err %s edge_gain[%u]:%u! range:[0, %u]\n", src, i, acac_auto->edge_gain[i],
                OT_ISP_CAC_ACAC_EDGE_GAIN_MAX);
            return OT_ERR_ISP_ILLEGAL_PARAM;
        } else if (acac_auto->cac_rb_strength[i] > OT_ISP_CAC_ACAC_RB_STR_MAX) {
            isp_err_trace("Err %s cac_rb_strength[%u]:%u! range:[0, %u]\n", src, i, acac_auto->cac_rb_strength[i],
                OT_ISP_CAC_ACAC_RB_STR_MAX);
            return OT_ERR_ISP_ILLEGAL_PARAM;
        } else if (acac_auto->purple_alpha[i] > 63) { /* max is 63 */
            isp_err_trace("Err %s purple_alpha[%u]:%u! range:[0, 63]\n", src, i, acac_auto->purple_alpha[i]);
            return OT_ERR_ISP_ILLEGAL_PARAM;
        } else if (acac_auto->edge_alpha[i] > 63) { /* max is 63 */
            isp_err_trace("Err %s edge_alpha[%u]:%u! range:[0, 63]\n", src, i, acac_auto->edge_alpha[i]);
            return OT_ERR_ISP_ILLEGAL_PARAM;
        } else if (acac_auto->satu_low_threshold[i] > OT_ISP_CAC_ACAC_SATU_THD_MAX) {
            isp_err_trace("Err %s satu_low_threshold[%u]:%u! range:[0, %d]\n", src, i,
                acac_auto->satu_low_threshold[i], OT_ISP_CAC_ACAC_SATU_THD_MAX);
            return OT_ERR_ISP_ILLEGAL_PARAM;
        }
        for (j = 0; j < OT_ISP_CAC_THR_NUM; j++) {
            if (acac_auto->edge_threshold[j][i] > 4095) { /* max is 4095 */
                isp_err_trace("Err %s edge_threshold[%u][%u]:%u! range:[0, 4095]\n", src, j, i,
                    acac_auto->edge_threshold[j][i]);
                return OT_ERR_ISP_ILLEGAL_PARAM;
            }
        }
        if (acac_auto->edge_threshold[0][i] >= acac_auto->edge_threshold[1][i]) {
            isp_err_trace("Err %s edge_threshold[0][%u]:%u can't be greater than edge_threshold[1][%u]:%u!\n",
                src, i, acac_auto->edge_threshold[0][i], i, acac_auto->edge_threshold[1][i]);
            return OT_ERR_ISP_ILLEGAL_PARAM;
        }
    }
    return isp_cac_acac_auto_attr_arch_check(src, acac_auto);
}

static td_s32 isp_cac_acac_manual_attr_check(const char *src, const ot_isp_cac_acac_manual_attr *acac_manual)
{
    td_u8 i;
    if (acac_manual->edge_gain > OT_ISP_CAC_ACAC_EDGE_GAIN_MAX) {
        isp_err_trace("Err %s edge_gain:%u! range:[0, %u]\n", src, acac_manual->edge_gain,
            OT_ISP_CAC_ACAC_EDGE_GAIN_MAX);
        return OT_ERR_ISP_ILLEGAL_PARAM;
    }
    if (acac_manual->cac_rb_strength > OT_ISP_CAC_ACAC_RB_STR_MAX) {
        isp_err_trace("Err %s cac_rb_strength:%u! range:[0, %u]\n", src, acac_manual->cac_rb_strength,
            OT_ISP_CAC_ACAC_RB_STR_MAX);
        return OT_ERR_ISP_ILLEGAL_PARAM;
    }
    if (acac_manual->purple_alpha > 63) { /* max is 63 */
        isp_err_trace("Err %s purple_alpha:%u! range:[0, 63]\n", src, acac_manual->purple_alpha);
        return OT_ERR_ISP_ILLEGAL_PARAM;
    }
    if (acac_manual->edge_alpha > 63) { /* max is 63 */
        isp_err_trace("Err %s edge_alpha:%u! range:[0, 63]\n", src, acac_manual->edge_alpha);
        return OT_ERR_ISP_ILLEGAL_PARAM;
    }
    if (acac_manual->satu_low_threshold > OT_ISP_CAC_ACAC_SATU_THD_MAX) {
        isp_err_trace("Err %s satu_low_threshold:%u! range:[0, %u]\n", src, acac_manual->satu_low_threshold,
            OT_ISP_CAC_ACAC_SATU_THD_MAX);
        return OT_ERR_ISP_ILLEGAL_PARAM;
    }
    for (i = 0; i < OT_ISP_CAC_THR_NUM; i++) {
        if (acac_manual->edge_threshold[i] > 4095) { /* max is 4095 */
            isp_err_trace("Err %s edge_threshold[%u]:%u! range:[0, 4095]\n", src, i, acac_manual->edge_threshold[i]);
            return OT_ERR_ISP_ILLEGAL_PARAM;
        }
    }
    if (acac_manual->edge_threshold[0] >= acac_manual->edge_threshold[1]) {
        isp_err_trace("Err %s edge_threshold[0]:%u can't be greater than edge_threshold[1]:%u!\n", src,
            acac_manual->edge_threshold[0], acac_manual->edge_threshold[1]);
        return OT_ERR_ISP_ILLEGAL_PARAM;
    }
    return isp_cac_acac_manual_attr_arch_check(src, acac_manual);
}

td_s32 isp_cac_acac_attr_check(const char *src, const ot_isp_cac_acac_attr *acac_cfg)
{
    td_s32 ret;
    ret = isp_cac_acac_auto_attr_check(src, &acac_cfg->acac_auto);
    if (ret != TD_SUCCESS) {
        return ret;
    }
    ret = isp_cac_acac_manual_attr_check(src, &acac_cfg->acac_manual);
    if (ret != TD_SUCCESS) {
        return ret;
    }
    return TD_SUCCESS;
}

td_s32 isp_cac_lcac_attr_check(const char *src, const ot_isp_cac_lcac_attr *lcac_cfg)
{
    td_s32 ret;
    ret = isp_cac_lcac_comm_attr_check(src, lcac_cfg);
    if (ret != TD_SUCCESS) {
        return ret;
    }
    ret = isp_cac_lcac_auto_attr_check(src, &lcac_cfg->lcac_auto);
    if (ret != TD_SUCCESS) {
        return ret;
    }
    ret = isp_cac_lcac_manual_attr_check(src, &lcac_cfg->lcac_manual);
    if (ret != TD_SUCCESS) {
        return ret;
    }
    return TD_SUCCESS;
}

#ifdef CONFIG_OT_ISP_BAYER_SHP_SUPPORT
td_s32 isp_bshp_comm_attr_check(const char *src, const ot_isp_bayershp_attr *bshp_attr)
{
    td_u16 j;
    isp_check_bool_return(bshp_attr->enable);

    if (bshp_attr->op_type >= OT_OP_MODE_BUTT) {
        isp_err_trace("Err %s invalid op_type %d!\n", src, bshp_attr->op_type);
        return OT_ERR_ISP_ILLEGAL_PARAM;
    }
    for (j = 0; j < OT_ISP_BSHP_THD_NUM; j++) {
        if (bshp_attr->dark_threshold[j] > 4095) { /* max is 4095 */
            isp_err_trace("Err %s dark_threshold[%u] = %u, range: [%u, %u]!\n", src, j,
                bshp_attr->dark_threshold[j], 0, 4095); /* max is 4095 */
            return OT_ERR_ISP_ILLEGAL_PARAM;
        } else if (bshp_attr->texture_threshold[j] > 4095) { /* max is 4095 */
            isp_err_trace("Err %s texture_threshold[%u] = %u, range: [%u, %u]!\n", src, j,
                bshp_attr->texture_threshold[j], 0, 4095); /* max is 4095 */
            return OT_ERR_ISP_ILLEGAL_PARAM;
        }
    }
    if (bshp_attr->dark_threshold[0] >= bshp_attr->dark_threshold[1]) {
        isp_err_trace("Err %s dark_threshold[1]:%u should be greater than dark_threshold[0]:%u!\n", src,
            bshp_attr->dark_threshold[1], bshp_attr->dark_threshold[0]);
        return OT_ERR_ISP_ILLEGAL_PARAM;
    }
    if (bshp_attr->texture_threshold[0] >= bshp_attr->texture_threshold[1]) {
        isp_err_trace("Err %s texture_threshold[1]:%u should be greater than texture_threshold[0]:%u!\n", src,
            bshp_attr->texture_threshold[1], bshp_attr->texture_threshold[0]);
        return OT_ERR_ISP_ILLEGAL_PARAM;
    }

    return TD_SUCCESS;
}

td_s32 isp_bshp_auto_attr_check(const char *src, const ot_isp_bayershp_auto_attr *auto_attr)
{
    td_u16 i, j;
    for (i = 0; i < OT_ISP_AUTO_ISO_NUM; i++) {
        if (auto_attr->mf_gain[i] > 15) { /* max is 15 */
            isp_err_trace("Err %s mf_gain[%u] = %u, range: [%u, %u]!\n", src, i, auto_attr->mf_gain[i],
                0, 15); /* max is 15 */
            return OT_ERR_ISP_ILLEGAL_PARAM;
        } else if (auto_attr->hf_gain[i] > 15) { /* max is 15 */
            isp_err_trace("Err %s hf_gain[%u] = %u, range: [%u, %u]!\n", src, i, auto_attr->hf_gain[i],
                0, 15); /* max is 15 */
            return OT_ERR_ISP_ILLEGAL_PARAM;
        } else if (auto_attr->dark_gain[i] > 15) { /* max is 15 */
            isp_err_trace("Err %s dark_gain[%u] = %u, range: [%u, %u]!\n", src, i, auto_attr->dark_gain[i],
                0, 15); /* max is 15 */
            return OT_ERR_ISP_ILLEGAL_PARAM;
        } else if (auto_attr->overshoot[i] > 255) { /* max is 255 */
            isp_err_trace("Err %s overshoot[%u] = %u, range: [%u, %u]!\n", src, i, auto_attr->overshoot[i],
                0, 255); /* max is 255 */
            return OT_ERR_ISP_ILLEGAL_PARAM;
        } else if (auto_attr->undershoot[i] > 255) { /* max is 255 */
            isp_err_trace("Err %s undershoot[%u] = %u, range: [%u, %u]!\n", src, i, auto_attr->undershoot[i],
                0, 255); /* max is 255 */
            return OT_ERR_ISP_ILLEGAL_PARAM;
        }
        for (j = 0; j < OT_ISP_BSHP_CURVE_NUM; j++) {
            if (auto_attr->mf_strength[j][i] > 63) { /* max is 63 */
                isp_err_trace("Err %s mf_strength[%u][%u] = %u, range: [%u, %u]!\n", src, j, i,
                    auto_attr->mf_strength[j][i], 0, 63); /* max is 63 */
                return OT_ERR_ISP_ILLEGAL_PARAM;
            } else if (auto_attr->hf_strength[j][i] > 63) { /* max is 63 */
                isp_err_trace("Err %s hf_strength[%u][%u] = %u, range: [%u, %u]!\n", src, j, i,
                    auto_attr->hf_strength[j][i], 0, 63); /* max is 63 */
                return OT_ERR_ISP_ILLEGAL_PARAM;
            } else if (auto_attr->dark_strength[j][i] > 63) { /* max is 63 */
                isp_err_trace("Err %s dark_strength[%u][%u] = %u, range: [%u, %u]!\n", src, j, i,
                    auto_attr->dark_strength[j][i], 0, 63); /* max is 63 */
                return OT_ERR_ISP_ILLEGAL_PARAM;
            }
        }
    }
    return TD_SUCCESS;
}

td_s32 isp_bshp_manual_attr_check(const char *src, const ot_isp_bayershp_manual_attr *manual_attr)
{
    td_s32 i;
    if (manual_attr->mf_gain > 15) { /* max is 15 */
        isp_err_trace("Err %s mf_gain %u, range: [%u, %u]!\n", src, manual_attr->mf_gain, 0, 15); /* max is 15 */
        return OT_ERR_ISP_ILLEGAL_PARAM;
    } else if (manual_attr->hf_gain > 15) { /* max is 15 */
        isp_err_trace("Err %s hf_gain %u, range: [%u, %u]!\n", src, manual_attr->hf_gain, 0, 15); /* max is 15 */
        return OT_ERR_ISP_ILLEGAL_PARAM;
    } else if (manual_attr->dark_gain > 15) { /* max is 15 */
        isp_err_trace("Err %s dark_gain %u, range: [%u, %u]!\n", src, manual_attr->dark_gain,
            0, 15); /* max is 15 */
        return OT_ERR_ISP_ILLEGAL_PARAM;
    } else if (manual_attr->overshoot > 255) { /* max is 255 */
        isp_err_trace("Err %s overshoot %u, range: [%u, %u]!\n", src, manual_attr->overshoot,
            0, 255); /* max is 255 */
        return OT_ERR_ISP_ILLEGAL_PARAM;
    } else if (manual_attr->undershoot > 255) { /* max is 255 */
        isp_err_trace("Err %s undershoot %u, range: [%u, %u]!\n", src, manual_attr->undershoot,
            0, 255); /* max is 255 */
        return OT_ERR_ISP_ILLEGAL_PARAM;
    }
    for (i = 0; i < OT_ISP_BSHP_CURVE_NUM; i++) {
        if (manual_attr->mf_strength[i] > 63) { /* max is 63 */
            isp_err_trace("Err %s mf_strength[%d] = %u, range: [%u, %u]!\n", src, i, manual_attr->mf_strength[i],
                0, 63); /* max is 63 */
            return OT_ERR_ISP_ILLEGAL_PARAM;
        } else if (manual_attr->hf_strength[i] > 63) { /* max is 63 */
            isp_err_trace("Err %s hf_strength[%d] = %u, range: [%u, %u]!\n", src, i, manual_attr->hf_strength[i],
                0, 63); /* max is 63 */
            return OT_ERR_ISP_ILLEGAL_PARAM;
        } else if (manual_attr->dark_strength[i] > 63) { /* max is 63 */
            isp_err_trace("Err %s dark_strength[%d] = %u, range: [%u, %u]!\n", src, i,
                manual_attr->dark_strength[i], 0, 63); /* max is 63 */
            return OT_ERR_ISP_ILLEGAL_PARAM;
        }
    }
    return TD_SUCCESS;
}
#endif

#ifdef CONFIG_OT_ISP_RGBIR_SUPPORT
static td_s32 isp_rgbir_cfg_check(const char *src, const ot_isp_rgbir_attr *rgbir_attr)
{
    td_u8 i;

    isp_check_bool_return(rgbir_attr->auto_gain_en);
    isp_check_bool_return(rgbir_attr->smooth_en);
    isp_check_bool_return(rgbir_attr->ir_rm_en);

    if (rgbir_attr->auto_gain > 1023) { /* range:[0, 1023] */
        isp_err_trace("Err %s auto_gain %u, range: [%u, %u]!\n", src, rgbir_attr->auto_gain,
            0, 1023); /* range:[0, 1023] */
        return OT_ERR_ISP_ILLEGAL_PARAM;
    }

    for (i = 0; i < OT_ISP_RGBIR_CTRL_NUM; i++) {
        if (rgbir_attr->exp_ctrl[i] > 2047) { /* range:[0, 2047] */
            isp_err_trace("Err %s exp_ctrl[%u] %u, range: [%u, %u]\n", src, i, rgbir_attr->exp_ctrl[i],
                0, 2047); /* range:[0, 2047] */
            return OT_ERR_ISP_ILLEGAL_PARAM;
        }

        if (rgbir_attr->exp_gain[i] > 511) { /* range:[0, 511] */
            isp_err_trace("Err %s exp_gain[%u] %u, range: [%u, %u]\n", src, i, rgbir_attr->exp_gain[i],
                0, 511); /* range:[0, 511] */
            return OT_ERR_ISP_ILLEGAL_PARAM;
        }

        if (rgbir_attr->wb_ctrl_strength[i] > 1024) { /* range:[0, 1024] */
            isp_err_trace("Err %s wb_ctrl_strength[%u] %u, range: [%u, %u]\n", src, i, rgbir_attr->wb_ctrl_strength[i],
                0, 1024); /* range:[0, 1024] */
            return OT_ERR_ISP_ILLEGAL_PARAM;
        }
    }

    for (i = 0; i < OT_ISP_RGBIR_CROSSTALK_NUM; i++) {
        if (rgbir_attr->ir_rm_ratio[i] > 1000) { /* range:[0, 1000] */
            isp_err_trace("Err %s ir_rm_ratio[%u] %u, range: [%u, %u]\n", src, i, rgbir_attr->ir_rm_ratio[i],
                0, 1000); /* range:[0, 1000] */
            return OT_ERR_ISP_ILLEGAL_PARAM;
        }
    }

    if (rgbir_attr->ir_cvtmat_mode >= OT_ISP_IR_CVTMAT_MODE_BUTT) {
        isp_err_trace("Err %s ir_cvtmat_mode %u, range: [%u, %u)!\n", src, rgbir_attr->ir_cvtmat_mode,
            OT_ISP_IR_CVTMAT_MODE_NORMAL, OT_ISP_IR_CVTMAT_MODE_BUTT);
        return OT_ERR_ISP_ILLEGAL_PARAM;
    }

    for (i = 0; i < OT_ISP_RGBIR_CVTMAT_NUM; i++) {
        if (rgbir_attr->cvt_matrix[i] < -16384 || rgbir_attr->cvt_matrix[i] > 16383) { /* [-16384, 16383] */
            isp_err_trace("Err %s cvt_matrix[%u] %d, range: [%d, %d]!\n", src, i, gbir_attr->cvt_matrix[i],
                -16384, 16383); /* [-16384, 16383] */
            return OT_ERR_ISP_ILLEGAL_PARAM;
        }
    }

    return TD_SUCCESS;
}

td_s32 isp_rgbir_attr_check(const char *src, ot_vi_pipe vi_pipe, const ot_isp_rgbir_attr *rgbir_attr)
{
    td_s32 ret;
    ot_wdr_mode wdr_mode;

    isp_check_bool_return(rgbir_attr->rgbir_en);
    isp_check_bool_return(rgbir_attr->rgbir_cfg.is_ir_upscale);

    if (rgbir_attr->rgbir_en == TD_TRUE && rgbir_attr->rgbir_cfg.mode == OT_ISP_RGBIR_MODE_NORMAL) {
        isp_err_trace("RGBIR can not support OT_ISP_RGBIR_MODE_NORMAL mode.\n");
        return OT_ERR_ISP_NOT_SUPPORT;
    }

    wdr_mode = ot_ext_system_sensor_wdr_mode_read(vi_pipe);
    if ((rgbir_attr->rgbir_en == TD_TRUE) && (is_wdr_mode(wdr_mode))) {
        isp_err_trace("RGBIR can not open in WDR mode\n");
        return OT_ERR_ISP_ILLEGAL_PARAM;
    }

    if (rgbir_attr->rgbir_cfg.mode >= OT_ISP_RGBIR_MODE_BUTT) {
        isp_err_trace("Err %s rgbir mode %d, range: [%u, %u)", src, rgbir_attr->rgbir_cfg.mode,
            OT_ISP_RGBIR_MODE_RGBIR4X4, OT_ISP_RGBIR_MODE_BUTT);
        return OT_ERR_ISP_ILLEGAL_PARAM;
    }

    if (rgbir_attr->rgbir_cfg.out_pattern >= OT_ISP_BAYER_BUTT) {
        isp_err_trace("Err %s out_pattern %u, range: [%u, %u)!\n", src, rgbir_attr->rgbir_cfg.out_pattern,
            OT_ISP_BAYER_RGGB, OT_ISP_BAYER_BUTT);
        return OT_ERR_ISP_ILLEGAL_PARAM;
    }

    if (rgbir_attr->rgbir_cfg.in_rgbir_pattern >= OT_ISP_IRBAYER_BUTT) {
        isp_err_trace("Err %s in_rgbir_pattern %u, range: [%u, %u)!\n", src, rgbir_attr->rgbir_cfg.in_rgbir_pattern,
            OT_ISP_IRBAYER_GRGBI, OT_ISP_IRBAYER_BUTT);
        return OT_ERR_ISP_ILLEGAL_PARAM;
    }
    if (rgbir_attr->rgbir_cfg.in_bayer_pattern >= OT_ISP_BAYER_BUTT) {
        isp_err_trace("Err %s in_bayer_pattern %u, range: [%u, %u)!\n", src, rgbir_attr->rgbir_cfg.in_bayer_pattern,
            OT_ISP_BAYER_RGGB, OT_ISP_BAYER_BUTT);
        return OT_ERR_ISP_ILLEGAL_PARAM;
    }

    ret = isp_rgbir_cfg_check(src, rgbir_attr);
    if (ret != TD_SUCCESS) {
        return ret;
    }

    return TD_SUCCESS;
}
#endif

static td_s32 isp_nr_snr_auto_attr_check(const char *src, const ot_isp_nr_snr_auto_attr *snr_auto)
{
    td_u8 i, j;

    for (i = 0; i < OT_ISP_AUTO_ISO_NUM; i++) {
        for (j = 0; j < OT_ISP_BAYER_CHN_NUM; j++) {
            if (snr_auto->sfm0_coarse_strength[j][i] > 864) { /* Range:[0, 864] */
                isp_err_trace("Err %s sfm0_coarse_strength[%u][%u] %u, range: [%u, %u]!\n", src, j, i,
                    snr_auto->sfm0_coarse_strength[j][i], 0, 864); /* Range:[0, 864] */
                return OT_ERR_ISP_ILLEGAL_PARAM;
            }
        }

        if (snr_auto->sfm0_detail_prot[i] > 31) { /* Range:[0, 31] */
            isp_err_trace("Err %s sfm0_detail_prot[%u] %u, range: [%u, %u]!\n", src, i,
                snr_auto->sfm0_detail_prot[i], 0, 31); /* Range:[0, 31] */
            return OT_ERR_ISP_ILLEGAL_PARAM;
        }

        if (snr_auto->fine_strength[i] > 128) { /* Range:[0, 128] */
            isp_err_trace("Err %s fine_strength[%u] %u, range: [%u, %u]!\n", src, i, snr_auto->fine_strength[i],
                0, 128); /* Range:[0, 128] */
            return OT_ERR_ISP_ILLEGAL_PARAM;
        }
    }

    return TD_SUCCESS;
}

static td_s32 isp_nr_snr_manual_attr_check(const char *src, const ot_isp_nr_snr_manual_attr *snr_manual)
{
    td_s32 j;

    for (j = 0; j < OT_ISP_BAYER_CHN_NUM; j++) {
        if (snr_manual->sfm0_coarse_strength[j] > 864) { /* Range:[0, 864] */
            isp_err_trace("Err %s sfm0_coarse_strength[%d] %u, range: [%u, %u]!\n", src, j,
                snr_manual->sfm0_coarse_strength[0], 0, 864); /* Range:[0, 864] */
            return OT_ERR_ISP_ILLEGAL_PARAM;
        }
    }

    if (snr_manual->sfm0_detail_prot > 31) { /* Range:[0, 31] */
        isp_err_trace("Err %s sfm0_detail_prot %d, range: [%u, %u]!\n", src, snr_manual->sfm0_detail_prot,
            0, 31); /* Range:[0, 31] */
        return OT_ERR_ISP_ILLEGAL_PARAM;
    }

    if (snr_manual->fine_strength > 128) { /* Range:[0, 128] */
        isp_err_trace("Err %s fine_strength %d, range: [%u, %u]!\n", src, snr_manual->fine_strength,
            0, 128); /* Range:[0, 128] */
        return OT_ERR_ISP_ILLEGAL_PARAM;
    }

    return TD_SUCCESS;
}

static td_s32 isp_nr_dering_auto_attr_check(const char *src, const ot_isp_nr_dering_auto_attr *dering_auto)
{
    td_s32 i;

    for (i = 0; i < OT_ISP_AUTO_ISO_NUM; i++) {
        if (dering_auto->dering_strength[i] > 1023) { /* Range:[0, 1023] */
            isp_err_trace("Err %s dering_strength[%d] %u, range: [%u, %u]!\n", src, i,
                dering_auto->dering_strength[i], 0, 1023); /* Range:[0, 1023] */
            return OT_ERR_ISP_ILLEGAL_PARAM;
        }

        if (dering_auto->dering_static_strength[i] > 32) { /* Range:[0, 32] */
            isp_err_trace("Err %s dering_static_strength[%d] %u, range: [%u, %u]!\n", src, i,
                dering_auto->dering_static_strength[i], 0, 32); /* Range:[0, 32] */
            return OT_ERR_ISP_ILLEGAL_PARAM;
        }

        if (dering_auto->dering_motion_strength[i] > 32) { /* Range:[0, 32] */
            isp_err_trace("Err %s dering_motion_strength[%d] %u, range: [%u, %u]!\n", src, i,
                dering_auto->dering_motion_strength[i], 0, 32); /* Range:[0, 32] */
            return OT_ERR_ISP_ILLEGAL_PARAM;
        }
    }

    return TD_SUCCESS;
}

static td_s32 isp_nr_dering_manual_attr_check(const char *src, const ot_isp_nr_dering_manual_attr *dering_manual)
{
    if (dering_manual->dering_strength > 1023) { /* Range:[0, 1023] */
        isp_err_trace("Err %s dering_strength %u, range: [%u, %u]!\n", src, dering_manual->dering_strength,
            0, 1023); /* Range:[0, 1023] */
        return OT_ERR_ISP_ILLEGAL_PARAM;
    }

    if (dering_manual->dering_static_strength > 32) { /* Range:[0, 32] */
        isp_err_trace("Err %s dering_static_strength %u, range: [%u, %u]!\n", src,
            dering_manual->dering_static_strength, 0, 32); /* Range:[0, 32] */
        return OT_ERR_ISP_ILLEGAL_PARAM;
    }

    if (dering_manual->dering_motion_strength > 32) { /* Range:[0, 32] */
        isp_err_trace("Err %s dering_motion_strength %u, range: [%u, %u]!\n", src,
            dering_manual->dering_motion_strength, 0, 32); /* Range:[0, 32] */
        return OT_ERR_ISP_ILLEGAL_PARAM;
    }

    return TD_SUCCESS;
}

td_s32 isp_nr_dering_attr_check(const char *src, const ot_isp_nr_dering_attr *dering_cfg)
{
    td_s32 ret;

    ret = isp_nr_dering_auto_attr_check(src, &dering_cfg->dering_auto);
    if (ret != TD_SUCCESS) {
        return ret;
    }
    ret = isp_nr_dering_manual_attr_check(src, &dering_cfg->dering_manual);
    if (ret != TD_SUCCESS) {
        return ret;
    }
    return TD_SUCCESS;
}

td_s32 isp_nr_snr_attr_check(const char *src, const ot_isp_nr_snr_attr *snr_cfg)
{
    td_s32 ret;
    ret = isp_nr_snr_attr_arch_check(src, snr_cfg);
    if (ret != TD_SUCCESS) {
        return ret;
    }
    ret = isp_nr_snr_auto_attr_check(src, &snr_cfg->snr_attr.snr_auto);
    if (ret != TD_SUCCESS) {
        return ret;
    }
    ret = isp_nr_snr_manual_attr_check(src, &snr_cfg->snr_attr.snr_manual);
    if (ret != TD_SUCCESS) {
        return ret;
    }
    return TD_SUCCESS;
}

td_s32 isp_nr_tnr_attr_check(const char *src, const ot_isp_nr_attr *nr_attr)
{
    td_s32 ret;
    ret = isp_nr_attr_arch_check(src, nr_attr);
    if (ret != TD_SUCCESS) {
        return ret;
    }
    return TD_SUCCESS;
}

td_s32 isp_nr_comm_attr_check(const char *src, ot_vi_pipe vi_pipe, const ot_isp_nr_attr *nr_attr)
{
    td_u8 i;
    td_bool online_ext_en;

    isp_check_bool_return(nr_attr->enable);
    isp_check_bool_return(nr_attr->tnr_en);
    isp_check_bool_return(nr_attr->lsc_nr_en);

    online_ext_en = ot_ext_system_online_ext_en_read(vi_pipe);
    if (online_ext_en == TD_TRUE && nr_attr->tnr_en == TD_TRUE) {
        isp_err_trace("Err %s not support tnr_en %d!\n", src, nr_attr->tnr_en);
        return OT_ERR_ISP_ILLEGAL_PARAM;
    }

    if (nr_attr->op_type >= OT_OP_MODE_BUTT) {
        isp_err_trace("Err %s NR type %d, range:[0,1]!\n", src, nr_attr->op_type);
        return OT_ERR_ISP_ILLEGAL_PARAM;
    }

    if (nr_attr->lsc_ratio1 > 15) { /* Range:[0, 15] */
        isp_err_trace("Err %s lsc_ratio1 %u, range: [%u, %u]!\n", src, nr_attr->lsc_ratio1,
            0, 15); /* Range:[0, 15] */
        return OT_ERR_ISP_ILLEGAL_PARAM;
    }

    for (i = 0; i < OT_ISP_BAYERNR_LUT_LENGTH; i++) {
        if (nr_attr->coring_ratio[i] > 0x3ff) {
            isp_err_trace("Err %s coring_ratio[%u] %u, range: [%u, %u]!\n", src, i, nr_attr->coring_ratio[i],
                0, 0x3ff);
            return OT_ERR_ISP_ILLEGAL_PARAM;
        }
    }

    for (i = 0; i < OT_ISP_BAYERNR_LUT_LENGTH1; i++) {
        if (nr_attr->mix_gain[i] > 0x80) {
            isp_err_trace("Err %s mix_gain[%d] %d, range: [%u, %u]!\n", src, i, nr_attr->mix_gain[i], 0, 0x80);
            return OT_ERR_ISP_ILLEGAL_PARAM;
        }
    }

    if (nr_attr->ref_mode >= OT_ISP_BNR_BUTT) { /* Range:[0, 2] */
        isp_err_trace("Err %s ref_mode %u, range: [%u, %u]!\n", src, nr_attr->ref_mode, 0, 2); /* Range:[0, 2] */
        return OT_ERR_ISP_ILLEGAL_PARAM;
    }

    if (nr_attr->load_ref_en > 1) { /* Range:[0, 1] */
        isp_err_trace("Err %s load_ref_en %u, range: [%u, %u]!\n", src, nr_attr->load_ref_en, 0, 1);
        return OT_ERR_ISP_ILLEGAL_PARAM;
    }
    return TD_SUCCESS;
}

td_s32 isp_nr_wdr_attr_check(const char *src, const ot_isp_nr_wdr_attr *wdr_cfg)
{
    td_s32 i;
    for (i = 0; i < OT_ISP_WDR_MAX_FRAME_NUM; i++) {
        if (wdr_cfg->snr_sfm0_wdr_strength[i] > 64) { /* Range:[0, 64] */
            isp_err_trace("Err %s snr_sfm0_wdr_strength[%d] %u, range: [%u, %u]!\n", src, i,
                wdr_cfg->snr_sfm0_wdr_strength[i], 0, 64); /* Range:[0, 64] */
            return OT_ERR_ISP_ILLEGAL_PARAM;
        }

        if (wdr_cfg->snr_sfm0_fusion_strength[i] > 64) { /* Range:[0, 64] */
            isp_err_trace("Err %s snr_sfm0_fusion_strength[%d] %u, range: [%u, %u]!\n", src, i,
                wdr_cfg->snr_sfm0_fusion_strength[i], 0, 64); /* Range:[0, 64] */
            return OT_ERR_ISP_ILLEGAL_PARAM;
        }

        if (wdr_cfg->snr_wdr_sfm6_strength[i] > 64) { /* Range:[0, 64] */
            isp_err_trace("Err %s snr_wdr_sfm6_strength[%d] %u, range: [%u, %u]!\n", src, i,
                wdr_cfg->snr_wdr_sfm6_strength[i], 0, 64); /* Range:[0, 64] */
            return OT_ERR_ISP_ILLEGAL_PARAM;
        }

        if (wdr_cfg->snr_fusion_sfm6_strength[i] > 64) { /* Range:[0, 64] */
            isp_err_trace("Err %s snr_fusion_sfm6_strength[%d] %u, range: [%u, %u]!\n", src, i,
                wdr_cfg->snr_fusion_sfm6_strength[i], 0, 64); /* Range:[0, 64] */
            return OT_ERR_ISP_ILLEGAL_PARAM;
        }

        if (wdr_cfg->snr_wdr_sfm7_strength[i] > 64) { /* Range:[0, 64] */
            isp_err_trace("Err %s snr_wdr_sfm7_strength[%d] %u, range: [%u, %u]!\n", src, i,
                wdr_cfg->snr_wdr_sfm7_strength[i], 0, 64); /* Range:[0, 64] */
            return OT_ERR_ISP_ILLEGAL_PARAM;
        }

        if (wdr_cfg->snr_fusion_sfm7_strength[i] > 64) { /* Range:[0, 64] */
            isp_err_trace("Err %s snr_fusion_sfm7_strength[%d] %u, range: [%u, %u]!\n", src, i,
                wdr_cfg->snr_fusion_sfm7_strength[i], 0, 64); /* Range:[0, 64] */
            return OT_ERR_ISP_ILLEGAL_PARAM;
        }

        if (wdr_cfg->md_wdr_strength[i] > 64) { /* Range:[0, 64] */
            isp_err_trace("Err %s md_wdr_strength[%d] %u, range: [%u, %u]!\n", src, i,
                wdr_cfg->md_wdr_strength[i], 0, 64); /* Range:[0, 64] */
            return OT_ERR_ISP_ILLEGAL_PARAM;
        }

        if (wdr_cfg->md_fusion_strength[i] > 64) { /* Range:[0, 64] */
            isp_err_trace("Err %s md_fusion_strength[%d] %u, range: [%u, %u]!\n", src, i,
                wdr_cfg->md_fusion_strength[i], 0, 64); /* Range:[0, 64] */
            return OT_ERR_ISP_ILLEGAL_PARAM;
        }
    }

    return TD_SUCCESS;
}

td_s32 isp_ldci_attr_check(const char *src, const ot_isp_ldci_attr *ldci_attr)
{
    isp_check_bool_return(ldci_attr->enable);

    if (ldci_attr->gauss_lpf_sigma < 0x1) {
        isp_err_trace("Err %s gauss_lpf_sigma:%u! range:[1,255]\n", src, ldci_attr->gauss_lpf_sigma);
        return OT_ERR_ISP_ILLEGAL_PARAM;
    }

    if (ldci_attr->op_type >= OT_OP_MODE_BUTT) {
        isp_err_trace("Err %s op_type:%d! range:[0,1]\n", src, ldci_attr->op_type);
        return OT_ERR_ISP_ILLEGAL_PARAM;
    }

    for (td_u8 i = 0; i < OT_ISP_AUTO_ISO_NUM; i++) {
        if (ldci_attr->auto_attr.he_wgt[i].he_pos_wgt.sigma < 0x1) {
            isp_err_trace("Err %s auto he_wgt.he_pos_wgt.sigma[%u]:%u! range:[1,255]\n",
                          src, i, ldci_attr->auto_attr.he_wgt[i].he_pos_wgt.sigma);
            return OT_ERR_ISP_ILLEGAL_PARAM;
        }

        if (ldci_attr->auto_attr.he_wgt[i].he_neg_wgt.sigma < 0x1) {
            isp_err_trace("Err %s auto he_wgt.he_neg_wgt.sigma[%u]:%u! range:[1,255]\n",
                          src, i, ldci_attr->auto_attr.he_wgt[i].he_neg_wgt.sigma);
            return OT_ERR_ISP_ILLEGAL_PARAM;
        }

        if (ldci_attr->auto_attr.blc_ctrl[i] > 0x1ff) {
            isp_err_trace("Err %s auto blc_ctrl[%u]:%u! range:[0,511]\n", src, i, ldci_attr->auto_attr.blc_ctrl[i]);
            return OT_ERR_ISP_ILLEGAL_PARAM;
        }
    }

    if (ldci_attr->manual_attr.he_wgt.he_pos_wgt.sigma < 0x1) {
        isp_err_trace("Err %s manual he_wgt.he_pos_wgt.sigma:%d! range:[1,255]\n",
                      src, ldci_attr->manual_attr.he_wgt.he_pos_wgt.sigma);
        return OT_ERR_ISP_ILLEGAL_PARAM;
    }

    if (ldci_attr->manual_attr.he_wgt.he_neg_wgt.sigma < 0x1) {
        isp_err_trace("Err %s manual he_wgt.he_neg_wgt.sigma:%d! range:[1,255]\n",
                      src, ldci_attr->manual_attr.he_wgt.he_neg_wgt.sigma);
        return OT_ERR_ISP_ILLEGAL_PARAM;
    }

    if (ldci_attr->manual_attr.blc_ctrl > 0x1ff) {
        isp_err_trace("Err %s manual blc_ctrl:%d! range:[0,511]\n", src, ldci_attr->manual_attr.blc_ctrl);
        return OT_ERR_ISP_ILLEGAL_PARAM;
    }

    if (ldci_attr->tpr_incr_coef > 0x100) {
        isp_err_trace("Err %s tpr_incr_coef:%d! range:[0,256]\n", src, ldci_attr->tpr_incr_coef);
        return OT_ERR_ISP_ILLEGAL_PARAM;
    }

    if (ldci_attr->tpr_decr_coef > 0x100) {
        isp_err_trace("Err %s tpr_decr_coef:%d! range:[0,256]\n", src, ldci_attr->tpr_decr_coef);
        return OT_ERR_ISP_ILLEGAL_PARAM;
    }

    return TD_SUCCESS;
}

static td_s32 isp_fswdr_common_combine_attr_check_inner(const char *src, const ot_isp_wdr_combine_attr *wdr_combine)
{
    if (wdr_combine->wdr_mdt.mdt_full_threshold > 0xFE) {
        isp_err_trace("Err %s mdt_full_thd %u > %d !\n", src, wdr_combine->wdr_mdt.mdt_full_threshold, 0xFE);
        return OT_ERR_ISP_ILLEGAL_PARAM;
    }
    if (wdr_combine->wdr_mdt.mdt_still_threshold > 0xFE) {
        isp_err_trace("Err %s mdt_still_thd %u > %d !\n", src, wdr_combine->wdr_mdt.mdt_still_threshold, 0xFE);
        return OT_ERR_ISP_ILLEGAL_PARAM;
    }
    if (wdr_combine->wdr_mdt.mdt_long_blend > 0xFE) {
        isp_err_trace("Err %s mdt_long_blend! %u > %d !\n", src, wdr_combine->wdr_mdt.mdt_long_blend, 0xFE);
        return OT_ERR_ISP_ILLEGAL_PARAM;
    }
    if (wdr_combine->wdr_mdt.short_check_threshold > 0xFFF) {
        isp_err_trace("Err %s short_check_threshold! %u > %d !\n",
            src, wdr_combine->wdr_mdt.short_check_threshold, 0xFFF);
        return OT_ERR_ISP_ILLEGAL_PARAM;
    }
    return TD_SUCCESS;
}

static td_s32 isp_fswdr_common_combine_attr_check(const char *src, const ot_isp_wdr_combine_attr *wdr_combine)
{
    td_s32 ret;
    if (wdr_combine->short_threshold > 0xFFF) {
        isp_err_trace("Err %s short_thr %u > %d !\n", src, wdr_combine->short_threshold, 0xFFF);
        return OT_ERR_ISP_ILLEGAL_PARAM;
    }
    if (wdr_combine->long_threshold > 0xFFF) {
        isp_err_trace("Err %s long_thr %u > %d !\n", src, wdr_combine->long_threshold, 0xFFF);
        return OT_ERR_ISP_ILLEGAL_PARAM;
    }
    if (wdr_combine->long_threshold > wdr_combine->short_threshold) {
        isp_err_trace("Err %s: long_thresh %u should NOT be larger than short_thresh %u !\n",
            src, wdr_combine->long_threshold, wdr_combine->short_threshold);
        return OT_ERR_ISP_ILLEGAL_PARAM;
    }

    if (wdr_combine->wdr_mdt.manual_attr.md_thr_low_gain > wdr_combine->wdr_mdt.manual_attr.md_thr_hig_gain) {
        isp_err_trace("Err %s: md_thr_low_gain %u should NOT be larger than md_thr_hig_gain %u !\n",
            src, wdr_combine->wdr_mdt.manual_attr.md_thr_low_gain,
            wdr_combine->wdr_mdt.manual_attr.md_thr_hig_gain);
        return OT_ERR_ISP_ILLEGAL_PARAM;
    }
    if (wdr_combine->force_long_hig_threshold > 0XFFF) {
        isp_err_trace("Err %s force_long_hig_thr %u > %d !\n",
            src, wdr_combine->force_long_hig_threshold, 0XFFF);
        return OT_ERR_ISP_ILLEGAL_PARAM;
    }
    if (wdr_combine->force_long_low_threshold > wdr_combine->force_long_hig_threshold) {
        isp_err_trace("Err %s: force_long_low_thr %u should NOT be larger than force_long_hig_thr %u !\n",
            src, wdr_combine->force_long_low_threshold, wdr_combine->force_long_hig_threshold);
        return OT_ERR_ISP_ILLEGAL_PARAM;
    }
    ret = isp_fswdr_common_combine_attr_check_inner(src, wdr_combine);
    if (ret != TD_SUCCESS) {
        return ret;
    }

    return TD_SUCCESS;
}

static td_s32 isp_fswdr_common_attr_check(const char *src, const ot_isp_wdr_fs_attr *fswdr_attr)
{
    td_u8 j;
    td_s32 ret;
    isp_check_bool_return(fswdr_attr->wdr_combine.motion_comp);
    isp_check_bool_return(fswdr_attr->wdr_combine.wdr_mdt.md_ref_flicker);
    isp_check_bool_return(fswdr_attr->wdr_combine.force_long);
    isp_check_bool_return(fswdr_attr->wdr_combine.wdr_mdt.short_expo_chk);
    isp_check_bool_return(fswdr_attr->fusion_attr.fusion_blend_en);
    isp_check_bool_return(fswdr_attr->fusion_attr.fusion_force_gray_en);

    ret = isp_fswdr_common_combine_attr_check(src, &fswdr_attr->wdr_combine);
    if (ret != TD_SUCCESS) {
        return ret;
    }
    if (fswdr_attr->wdr_merge_mode >= OT_ISP_MERGE_BUTT) {
        isp_err_trace("Err %s wdr_merge_mode %d >= %d!\n", src, fswdr_attr->wdr_merge_mode, OT_ISP_MERGE_BUTT);
        return OT_ERR_ISP_ILLEGAL_PARAM;
    }

    if (fswdr_attr->fusion_attr.fusion_blend_wgt > 0x10) { /* Range:[0, 16] */
        isp_err_trace("Err %s fusion_blend_wgt %u > %d !\n", src, fswdr_attr->fusion_attr.fusion_blend_wgt, 0x10);
        return OT_ERR_ISP_ILLEGAL_PARAM;
    }

    if (fswdr_attr->fusion_attr.fusion_force_blend_threshold > 0xF) { /* Range:[0, 15] */
            isp_err_trace("Err %s fusion_force_blend_threshold %u > %d !\n",
                src, fswdr_attr->fusion_attr.fusion_force_blend_threshold, 0xF);
            return OT_ERR_ISP_ILLEGAL_PARAM;
        }

    for (j = 0; j < OT_ISP_WDR_MAX_FRAME_NUM; j++) {
        if (fswdr_attr->fusion_attr.fusion_threshold[j] > OT_ISP_FUSION_THRESHOLD_MAX) {
            isp_err_trace("Err %s fusion_thr[%u] %u > %d !\n",
                src, j, fswdr_attr->fusion_attr.fusion_threshold[j], OT_ISP_FUSION_THRESHOLD_MAX);
            return OT_ERR_ISP_ILLEGAL_PARAM;
        }
    }

    return TD_SUCCESS;
}

static td_s32 isp_fswdr_auto_attr_check(const char *src, const ot_isp_wdr_fs_attr *fswdr_attr)
{
    td_u8 i, j;
    for (i = 0; i < OT_ISP_WDR_RATIO_NUM; i++) {
        for (j = 0; j < OT_ISP_AUTO_ISO_NUM; j++) {
            if (fswdr_attr->wdr_combine.wdr_mdt.auto_attr.md_thr_low_gain[i][j] >
                fswdr_attr->wdr_combine.wdr_mdt.auto_attr.md_thr_hig_gain[i][j]) {
                isp_err_trace("Err %s md_thr_low_gain[%u][%u] %u should NOT be larger than md_thr_hig_gain %u!\n",
                    src, i, j, fswdr_attr->wdr_combine.wdr_mdt.auto_attr.md_thr_low_gain[i][j],
                    fswdr_attr->wdr_combine.wdr_mdt.auto_attr.md_thr_hig_gain[i][j]);
                return OT_ERR_ISP_ILLEGAL_PARAM;
            }
        }
    }

    return TD_SUCCESS;
}

td_s32 isp_fswdr_attr_check(const char *src, const ot_isp_wdr_fs_attr *fswdr_attr)
{
    td_s32 ret;
    if (fswdr_attr->wdr_combine.wdr_mdt.op_type >= OT_OP_MODE_BUTT) {
        isp_err_trace("Err %s op_type! %d , which should be OT_OP_MODE_AUTO or OT_OP_MODE_MANUAL.\n",
            src, fswdr_attr->wdr_combine.wdr_mdt.op_type);
        return OT_ERR_ISP_ILLEGAL_PARAM;
    }

    ret = isp_fswdr_auto_attr_check(src, fswdr_attr);
    if (ret != TD_SUCCESS) {
        return ret;
    }
    ret = isp_fswdr_common_attr_check(src, fswdr_attr);
    if (ret != TD_SUCCESS) {
        return ret;
    }

    return TD_SUCCESS;
}


td_s32 isp_dehaze_attr_check(const char *src, const ot_isp_dehaze_attr *dehaze_attr)
{
    isp_check_bool_return(dehaze_attr->enable);
    isp_check_bool_return(dehaze_attr->user_lut_en);

    if (dehaze_attr->op_type >= OT_OP_MODE_BUTT) {
        isp_err_trace("Err %s op_type:%d! range:[0,1]\n", src, dehaze_attr->op_type);
        return OT_ERR_ISP_ILLEGAL_PARAM;
    }

    if (dehaze_attr->tmprflt_decr_coef > 0x80) {
        isp_err_trace("Err %s tmprflt_decr_coef:%u! range:[0,128]\n", src, dehaze_attr->tmprflt_decr_coef);
        return OT_ERR_ISP_ILLEGAL_PARAM;
    }

    if (dehaze_attr->tmprflt_incr_coef > 0x80) {
        isp_err_trace("Err %s tmprflt_incr_coef:%u! range:[0,128]\n", src, dehaze_attr->tmprflt_incr_coef);
        return OT_ERR_ISP_ILLEGAL_PARAM;
    }

    return TD_SUCCESS;
}

td_s32 isp_ca_attr_check(const char *src, const ot_isp_ca_attr *ca_attr)
{
    td_u16 i;
    isp_check_bool_return(ca_attr->enable);
    if (ca_attr->ca_cp_en >= OT_ISP_CA_BUTT) {
        isp_err_trace("Err ca type %d! range:[0,1]\n", ca_attr->ca_cp_en);
        return OT_ERR_ISP_ILLEGAL_PARAM;
    }
    for (i = 0; i < OT_ISP_CA_YRATIO_LUT_LENGTH; i++) {
        if (ca_attr->ca.y_ratio_lut[i] > OT_ISP_CA_RATIO_MAX) {
            isp_err_trace("Err %s y_ratio_lut[%u] %u! range:[0,%u]\n", src, i,
                ca_attr->ca.y_ratio_lut[i], OT_ISP_CA_RATIO_MAX);
            return OT_ERR_ISP_ILLEGAL_PARAM;
        }
        if (ca_attr->ca.y_sat_lut[i] > OT_ISP_CA_RATIO_MAX) {
            isp_err_trace("Err %s y_sat_lut[%u] %u! range:[0,%u]\n", src, i,
                ca_attr->ca.y_sat_lut[i], OT_ISP_CA_RATIO_MAX);
            return OT_ERR_ISP_ILLEGAL_PARAM;
        }
    }
    for (i = 0; i < OT_ISP_AUTO_ISO_NUM; i++) {
        if ((ca_attr->ca.iso_ratio[i] > OT_ISP_CA_RATIO_MAX) || (ca_attr->ca.iso_ratio[i] < 0)) {
            isp_err_trace("Err %s iso_ratio[%u] %d! range:[0,%u]\n", src, i,
                ca_attr->ca.iso_ratio[i], OT_ISP_CA_RATIO_MAX);
            return OT_ERR_ISP_ILLEGAL_PARAM;
        }
    }

    return isp_ca_attr_arch_check(src, ca_attr);
}

td_s32 isp_clut_attr_check(const char *src, const ot_isp_clut_attr *clut_attr)
{
    isp_check_bool_return(clut_attr->enable);
    if (clut_attr->gain_r > OT_ISP_CLUT_GAIN_MAX) {
        isp_err_trace("Err %s gain_r %u, range: [%u, %u]!\n", src, clut_attr->gain_r, 0, OT_ISP_CLUT_GAIN_MAX);
        return OT_ERR_ISP_ILLEGAL_PARAM;
    }
    if (clut_attr->gain_g > OT_ISP_CLUT_GAIN_MAX) {
        isp_err_trace("Err %s gain_g %u, range: [%u, %u]!\n", src, clut_attr->gain_g, 0, OT_ISP_CLUT_GAIN_MAX);
        return OT_ERR_ISP_ILLEGAL_PARAM;
    }
    if (clut_attr->gain_b > OT_ISP_CLUT_GAIN_MAX) {
        isp_err_trace("Err %s gain_b %u, range: [%u, %u]!\n", src, clut_attr->gain_b, 0, OT_ISP_CLUT_GAIN_MAX);
        return OT_ERR_ISP_ILLEGAL_PARAM;
    }
    return TD_SUCCESS;
}

td_s32 isp_anti_false_color_attr_check(const char *src, const ot_isp_anti_false_color_attr *anti_false_color)
{
    td_u32 i;
    isp_check_bool_return(anti_false_color->enable);
    if (anti_false_color->op_type >= OT_OP_MODE_BUTT) {
        isp_err_trace("Err %s op_type %d!\n", src, anti_false_color->op_type);
        return OT_ERR_ISP_ILLEGAL_PARAM;
    }

    for (i = 0; i < OT_ISP_AUTO_ISO_NUM; i++) {
        if (anti_false_color->auto_attr.threshold[i] > 0x20) {
            isp_err_trace("Err %s auto threshold[%u]: %u, range: [%u, %u]!\n", src, i,
                anti_false_color->auto_attr.threshold[i], 0, 0x20);
            return OT_ERR_ISP_ILLEGAL_PARAM;
        }
        if (anti_false_color->auto_attr.strength[i] > 0x1f) {
            isp_err_trace("Err %s auto strength[%u]: %u, range: [%u, %u]!\n", src, i,
                anti_false_color->auto_attr.strength[i], 0, 0x1f);
            return OT_ERR_ISP_ILLEGAL_PARAM;
        }
    }

    if (anti_false_color->manual_attr.threshold > 0x20) {
        isp_err_trace("Err %s manual threshold %d, range: [%u, %u]!\n", src,
            anti_false_color->manual_attr.threshold, 0, 0x20);
        return OT_ERR_ISP_ILLEGAL_PARAM;
    }
    if (anti_false_color->manual_attr.strength > 0x1f) {
        isp_err_trace("Err %s manual strength %d, range: [%u, %u]!\n", src,
            anti_false_color->manual_attr.strength, 0, 0x1f);
        return OT_ERR_ISP_ILLEGAL_PARAM;
    }

    return TD_SUCCESS;
}
#ifdef CONFIG_OT_ISP_DYNAMIC_BLC_SUPPORT
static td_s32 isp_dynamic_blc_attr_check_part2(const char *src, const ot_isp_black_level_dynamic_attr *dynamic_attr)
{
    td_u8 i;
    isp_check_bool_return(dynamic_attr->separate_en);

    for (i = 0; i < OT_ISP_AUTO_ISO_NUM; i++) {
        if (dynamic_attr->offset[i] > 0x3FFF || dynamic_attr->offset[i] < -0x3FFF) {
            isp_err_trace("Err %s dynamicblc offset %d, range: [%d, %d]!\n", src, dynamic_attr->offset[i],
                -0x3FFF, 0x3FFF);
            return OT_ERR_ISP_ILLEGAL_PARAM;
        }
    }

    for (i = 0; i < OT_ISP_AUTO_ISO_NUM; i++) {
        if (dynamic_attr->calibration_black_level[i] > 0x3FFF) {
            isp_err_trace("Err %s ag cali dynamicblc %d, range: [%u, %u]!\n", src,
                dynamic_attr->calibration_black_level[i], 0, 0x3FFF);
            return OT_ERR_ISP_ILLEGAL_PARAM;
        }
    }

    if (dynamic_attr->tolerance > 0x3FFF) {
        isp_err_trace("Err %s dynamicblc tolerance %d, range: [%u, %u]!\n", src, dynamic_attr->tolerance,
            0, 0x3FFF);
        return OT_ERR_ISP_ILLEGAL_PARAM;
    }
    if (dynamic_attr->filter_strength > 0x40) {
        isp_err_trace("Err %s dynamicblc filter_strength %d, range: [%u, %u]!\n", src,
            dynamic_attr->filter_strength, 0, 0x40);
        return OT_ERR_ISP_ILLEGAL_PARAM;
    }
    if (dynamic_attr->filter_thr > 0x3FFF) {
        isp_err_trace("Err %s dynamicblc filter_thr %d, range: [%u, %u]!\n", src, dynamic_attr->filter_thr,
            0, 0x3FFF);
        return OT_ERR_ISP_ILLEGAL_PARAM;
    }

    return TD_SUCCESS;
}

static td_s32 isp_dynamic_blc_ob_area_check(ot_vi_pipe vi_pipe, const char *src, const ot_rect *ob_area)
{
    td_u16 w, h;
    td_s32 ret;
    ot_size fe_in_size = { 0 };

    ret = ioctl(isp_get_fd(vi_pipe), ISP_GET_PIPE_SIZE, &fe_in_size);
    if (ret != TD_SUCCESS) {
        isp_err_trace("ISP[%d] get pipe size failed\n", vi_pipe);
        return ret;
    }

    w = fe_in_size.width;
    h = fe_in_size.height;

    if (ob_area->x % 2 != 0 || ob_area->x >= w || /* 2 is num */
        ob_area->x < 0) { /* 0 is num */
        isp_err_trace("Err %s x %d, range: [%u, %u) or x is not an even number!\n", src, ob_area->x, 0, w);
        return OT_ERR_ISP_ILLEGAL_PARAM;
    }

    if (ob_area->y % 2 != 0 || ob_area->y >= h || /* 2 is num */
        ob_area->y < 0) { /* 0 is num */
        isp_err_trace("Err %s y %d, range: [%u, %u) or y is not an even number!\n", src, ob_area->y, 0, h);
        return OT_ERR_ISP_ILLEGAL_PARAM;
    }

    if ((ob_area->y + ob_area->height) > h ||
        ob_area->height % 2 != 0x0) { /* 2 is num */
        isp_err_trace("Err %s y(%d) + height(%u) > %u or height is not an even number!\n", src,
            ob_area->y, ob_area->height, h);
        return OT_ERR_ISP_ILLEGAL_PARAM;
    }

    if ((ob_area->width + ob_area->x) > w ||
        ob_area->width % 2 != 0x0) { /* 2 is num */
        isp_err_trace("Err %s x(%d) + width(%u) > %u or width is not an even number!\n", src,
            ob_area->x, ob_area->width, w);
        return OT_ERR_ISP_ILLEGAL_PARAM;
    }

    return TD_SUCCESS;
}
#endif
td_s32 isp_dynamic_blc_attr_check(ot_vi_pipe vi_pipe, const char *src,
    const ot_isp_black_level_dynamic_attr *dynamic_attr, ot_isp_black_level_mode black_level_mode)
{
    td_u8  wdr_mode;
    td_s32 ret;
    wdr_mode  = ot_ext_system_sensor_wdr_mode_read(vi_pipe);

    if (black_level_mode != OT_ISP_BLACK_LEVEL_MODE_DYNAMIC) {
        return TD_SUCCESS;
    }
#ifdef CONFIG_OT_ISP_DYNAMIC_BLC_SUPPORT
    if (is_no_fe_pipe(vi_pipe)) {
        isp_err_trace("ISP[%d]: Not Support dynamic blc when no fe pipe!\n", vi_pipe);
        return OT_ERR_ISP_NOT_SUPPORT;
    }

    if (is_built_in_wdr_mode(wdr_mode)) {
        isp_err_trace("ISP[%d]: Not Support dynamic blc in sensor built-in mode!\n", vi_pipe);
        return OT_ERR_ISP_NOT_SUPPORT;
    }

    if (dynamic_attr->high_threshold > 0x3FFF) {
        isp_err_trace("Err %s high_threshold %u, range: [%u, %u]!\n", src, dynamic_attr->high_threshold,
            0, 0x3FFF);
        return OT_ERR_ISP_ILLEGAL_PARAM;
    }
    if (dynamic_attr->low_threshold >  0xFFF || dynamic_attr->low_threshold >= dynamic_attr->high_threshold) {
        isp_err_trace("Err %s low_threshold %u, range: [%u, %u] or low_threshold >= high_threshold!\n", src,
            dynamic_attr->low_threshold, 0, 0xFFF);
        return OT_ERR_ISP_ILLEGAL_PARAM;
    }
    ret = isp_dynamic_blc_ob_area_check(vi_pipe, src, &dynamic_attr->ob_area);
    if (ret != TD_SUCCESS) {
        return ret;
    }

    if (dynamic_attr->pattern >= OT_ISP_DYNAMIC_PATTERN_BUTT) {
        isp_err_trace("Err %s dynamicblc pattern %u >= %u!\n", src, dynamic_attr->pattern,
            OT_ISP_DYNAMIC_PATTERN_BUTT);
        return OT_ERR_ISP_ILLEGAL_PARAM;
    }

    ret = isp_dynamic_blc_attr_check_part2(src, dynamic_attr);
    if (ret != TD_SUCCESS) {
        return ret;
    }
    return TD_SUCCESS;
#else
    ot_unused(ret);
    ot_unused(wdr_mode);
    isp_err_trace("[isp%d]this chip not support dynamic blc\n", vi_pipe);
    return OT_ERR_ISP_NOT_SUPPORT;
#endif
}

td_s32 isp_black_level_value_check(const char *src, const td_u16 (*black_level)[OT_ISP_BAYER_CHN_NUM])
{
    td_u8 i, j;

    isp_check_pointer_return(black_level);
    for (i = 0; i < OT_ISP_WDR_MAX_FRAME_NUM; i++) {
        for (j = 0; j < OT_ISP_BAYER_CHN_NUM; j++) {
            if (black_level[i][j] > 0x3FFF) {
                isp_err_trace("Err %s black_level[%u][%u]:%u, range: [%u, %u]!\n", src, i, j, black_level[i][j],
                    0, 0x3FFF);
                return OT_ERR_ISP_ILLEGAL_PARAM;
            }
        }
    }

    return TD_SUCCESS;
}

td_s32 isp_user_black_level_en_check(ot_vi_pipe vi_pipe, const td_bool user_black_level_en)
{
    td_u8  wdr_mode;
    isp_check_bool_return(user_black_level_en);
    if (user_black_level_en == TD_FALSE) {
        return TD_SUCCESS;
    }
#ifdef CONFIG_OT_ISP_USER_BLC_SUPPORT
    if (is_no_fe_pipe(vi_pipe)) {
        isp_err_trace("ISP[%d]: Not Support user blc when no fe pipe!\n", vi_pipe);
        return OT_ERR_ISP_NOT_SUPPORT;
    }
    wdr_mode  = ot_ext_system_sensor_wdr_mode_read(vi_pipe);
    if (is_built_in_wdr_mode(wdr_mode)) {
        isp_err_trace("ISP[%d]: Not Support user blc in sensor built-in mode!\n", vi_pipe);
        return OT_ERR_ISP_NOT_SUPPORT;
    }
    return TD_SUCCESS;
#else
    ot_unused(wdr_mode);
    isp_err_trace("ISP[%d] this chip is not support user blc\n", vi_pipe);
    return OT_ERR_ISP_NOT_SUPPORT;
#endif
}

td_s32 isp_crosstalk_attr_check(const char *src, const ot_isp_cr_attr *cr_attr)
{
    td_u8 i;
    isp_check_bool_return(cr_attr->enable);
    if (cr_attr->slope > OT_ISP_CR_SLOPE_MAX) {
        isp_err_trace("Err %s slope %u, range: [%u, %u]!\n", src, cr_attr->slope, 0, OT_ISP_CR_SLOPE_MAX);
        return OT_ERR_ISP_ILLEGAL_PARAM;
    }

    if (cr_attr->sensi_slope > OT_ISP_CR_SLOPE_MAX) {
        isp_err_trace("Err %s sensi_slope %u, range: [%u, %u]!\n", src, cr_attr->sensi_slope,
            0, OT_ISP_CR_SLOPE_MAX);
        return OT_ERR_ISP_ILLEGAL_PARAM;
    }

    if (cr_attr->sensi_threshold > OT_ISP_CR_THR_MAX) {
        isp_err_trace("Err %s sensi_threshold %u, range: [%u, %u]!\n", src, cr_attr->sensi_threshold,
            0, OT_ISP_CR_THR_MAX);
        return OT_ERR_ISP_ILLEGAL_PARAM;
    }

    for (i = 0; i < OT_ISP_AUTO_ISO_NUM; i++) {
        if (cr_attr->strength[i] > 0x100) {
            isp_err_trace("Err %s strength[%u] %u, range: [%u, %u]!\n", src, i, cr_attr->strength[i], 0, 0x100);
            return OT_ERR_ISP_ILLEGAL_PARAM;
        }

        if (cr_attr->filter_mode[i] > 0x1F) {
            isp_err_trace("Err %s filter_mode[%u] %u, range: [%u, %u]!\n", src, i, cr_attr->filter_mode[i],
                0, 0x1F);
            return OT_ERR_ISP_ILLEGAL_PARAM;
        }

        if (cr_attr->np_offset[i] > OT_ISP_CR_NPOFFSET_MAX || cr_attr->np_offset[i] < OT_ISP_CR_NPOFFSET_MIN) {
            isp_err_trace("Err %s np_offset[%u] %u, range: [%u, %u]!\n", src, i, cr_attr->np_offset[i],
                OT_ISP_CR_NPOFFSET_MIN, OT_ISP_CR_NPOFFSET_MAX);
            return OT_ERR_ISP_ILLEGAL_PARAM;
        }

        if (cr_attr->threshold[i] > OT_ISP_CR_THR_MAX) {
            isp_err_trace("Err %s threshold[%u] %u, range: [%u, %u]!\n", src, i, cr_attr->threshold[i],
                0, OT_ISP_CR_THR_MAX);
            return OT_ERR_ISP_ILLEGAL_PARAM;
        }
    }
    return TD_SUCCESS;
}

static td_s32 isp_mesh_shading_grid_width_check(const char *src, ot_vi_pipe vi_pipe,
    const ot_isp_shading_lut_attr *lut_attr)
{
    td_u16 i;
    td_u32 width, height;
    td_u32 x_sum = 0;
    td_u32 y_sum = 0;

    width  = ot_ext_system_be_total_width_read(vi_pipe);
    height = ot_ext_system_be_total_height_read(vi_pipe);

    for (i = 0; i < OT_ISP_MLSC_X_HALF_GRID_NUM; i++) {
        x_sum += lut_attr->x_grid_width[i];
    }

    for (i = 0; i < OT_ISP_MLSC_Y_HALF_GRID_NUM; i++) {
        y_sum += lut_attr->y_grid_width[i];
    }

    if ((x_sum != (width / 4)) || (y_sum != (height / 4))) {  /* x_sum(y_sum)should be wdith/4(height/4) */
        isp_err_trace("Err %s block size x_sum %d != width / 4, y_sum %d != height / 4!\n", src, x_sum, y_sum);
        return OT_ERR_ISP_ILLEGAL_PARAM;
    }

    for (i = 0; i < OT_ISP_MLSC_X_HALF_GRID_NUM; i++) {
        if ((lut_attr->x_grid_width[i] > OT_ISP_LSC_XGRID_WIDTH_MAX) || (lut_attr->x_grid_width[i] < 0x4)) {
            isp_err_trace("Err %s x_grid_width[%d] %d, range: [%u, %u]!\n", src, i, lut_attr->x_grid_width[i],
                0x4, OT_ISP_LSC_XGRID_WIDTH_MAX);
            return OT_ERR_ISP_ILLEGAL_PARAM;
        }
    }

    for (i = 0; i < OT_ISP_MLSC_Y_HALF_GRID_NUM; i++) {
        if ((lut_attr->y_grid_width[i] > OT_ISP_LSC_YGRID_WIDTH_MAX) || (lut_attr->y_grid_width[i] < 0x4)) {
            isp_err_trace("Err %s y_grid_width[%d] %d, range: [%u, %u]!\n", src, i, lut_attr->y_grid_width[i],
                0x4, OT_ISP_LSC_YGRID_WIDTH_MAX);
            return OT_ERR_ISP_ILLEGAL_PARAM;
        }
    }

    return TD_SUCCESS;
}

td_s32 isp_mesh_shading_attr_check(const char *src, ot_vi_pipe vi_pipe,
    const ot_isp_shading_attr *shading_attr)
{
    ot_unused(vi_pipe);
    isp_check_bool_return(shading_attr->enable);
    if (shading_attr->mesh_strength > OT_ISP_LSC_MESHSTR_MAX) {
        isp_err_trace("Err %s mesh_strength:%u, range: [%u, %u]!\n", src, shading_attr->mesh_strength,
            0, OT_ISP_LSC_MESHSTR_MAX);
        return OT_ERR_ISP_ILLEGAL_PARAM;
    }

    if (shading_attr->blend_ratio > 0x100) {
        isp_err_trace("Err %s blend_ratio:%u, range: [%u, %u]!\n", src, shading_attr->blend_ratio, 0, 0x100);
        return OT_ERR_ISP_ILLEGAL_PARAM;
    }
    return TD_SUCCESS;
}

td_s32 isp_mesh_shading_gain_lut_attr_check(const char *src, ot_vi_pipe vi_pipe,
    const ot_isp_shading_lut_attr *lut_attr)
{
    td_u16 i;
    if (lut_attr->mesh_scale > (OT_ISP_LSC_MESHSCALE_NUM - 1)) {
        isp_err_trace("Err %s mesh_scale:%u, mesh_scale must be smaller than %u!\n", src, lut_attr->mesh_scale,
            OT_ISP_LSC_MESHSCALE_NUM);
        return OT_ERR_ISP_ILLEGAL_PARAM;
    }

    for (i = 0; i < OT_ISP_LSC_GRID_POINTS; i++) {
        if (lut_attr->lsc_gain_lut[0].r_gain[i] > OT_ISP_LSC_MAX_GAIN) {
            isp_err_trace("Err %s lsc_gain_lut[0].r_gain:%u, range: [%u, %u]!\n", src,
                lut_attr->lsc_gain_lut[0].r_gain[i], 0, OT_ISP_LSC_MAX_GAIN);
            return OT_ERR_ISP_ILLEGAL_PARAM;
        }
        if (lut_attr->lsc_gain_lut[0].gr_gain[i] > OT_ISP_LSC_MAX_GAIN) {
            isp_err_trace("Err %s lsc_gain_lut[0].gr_gain:%u, range: [%u, %u]!\n", src,
                lut_attr->lsc_gain_lut[0].gr_gain[i], 0, OT_ISP_LSC_MAX_GAIN);
            return OT_ERR_ISP_ILLEGAL_PARAM;
        }
        if (lut_attr->lsc_gain_lut[0].gb_gain[i] > OT_ISP_LSC_MAX_GAIN) {
            isp_err_trace("Err %s lsc_gain_lut[0].gb_gain:%u, range: [%u, %u]!\n", src,
                lut_attr->lsc_gain_lut[0].gb_gain[i], 0, OT_ISP_LSC_MAX_GAIN);
            return OT_ERR_ISP_ILLEGAL_PARAM;
        }
        if (lut_attr->lsc_gain_lut[0].b_gain[i] > OT_ISP_LSC_MAX_GAIN) {
            isp_err_trace("Err %s lsc_gain_lut[0].b_gain:%u, range: [%u, %u]!\n", src,
                lut_attr->lsc_gain_lut[0].b_gain[i], 0, OT_ISP_LSC_MAX_GAIN);
            return OT_ERR_ISP_ILLEGAL_PARAM;
        }
        if (lut_attr->lsc_gain_lut[1].r_gain[i] > OT_ISP_LSC_MAX_GAIN) {
            isp_err_trace("Err %s lsc_gain_lut[1].r_gain:%u, range: [%u, %u]!\n", src,
                lut_attr->lsc_gain_lut[1].r_gain[i], 0, OT_ISP_LSC_MAX_GAIN);
            return OT_ERR_ISP_ILLEGAL_PARAM;
        }
        if (lut_attr->lsc_gain_lut[1].gr_gain[i] > OT_ISP_LSC_MAX_GAIN) {
            isp_err_trace("Err %s lsc_gain_lut[1].gr_gain:%u, range: [%u, %u]!\n", src,
                lut_attr->lsc_gain_lut[1].gr_gain[i], 0, OT_ISP_LSC_MAX_GAIN);
            return OT_ERR_ISP_ILLEGAL_PARAM;
        }
        if (lut_attr->lsc_gain_lut[1].gb_gain[i] > OT_ISP_LSC_MAX_GAIN) {
            isp_err_trace("Err %s lsc_gain_lut[1].gb_gain:%u, range: [%u, %u]!\n", src,
                lut_attr->lsc_gain_lut[1].gb_gain[i], 0, OT_ISP_LSC_MAX_GAIN);
            return OT_ERR_ISP_ILLEGAL_PARAM;
        }
        if (lut_attr->lsc_gain_lut[1].b_gain[i] > OT_ISP_LSC_MAX_GAIN) {
            isp_err_trace("Err %s lsc_gain_lut[1].b_gain:%u, range: [%u, %u]!\n", src,
                lut_attr->lsc_gain_lut[1].b_gain[i], 0, OT_ISP_LSC_MAX_GAIN);
            return OT_ERR_ISP_ILLEGAL_PARAM;
        }
    }

    return isp_mesh_shading_grid_width_check(src, vi_pipe, lut_attr);
}

static td_s32 isp_mesh_shading_calibration_cfg_res_check(ot_isp_mlsc_calibration_cfg *cali_cfg)
{
    if (cali_cfg->img_width % OT_ISP_ALIGN_WIDTH != 0) {
        isp_err_trace("Err img_width:%u, width must be a muliple of 4!\n", cali_cfg->img_width);
        return OT_ERR_ISP_ILLEGAL_PARAM;
    }

    if (cali_cfg->img_height % OT_ISP_ALIGN_HEIGHT != 0) {
        isp_err_trace("Err img_height:%u, height must be a muliple of 4!\n", cali_cfg->img_height);
        return OT_ERR_ISP_ILLEGAL_PARAM;
    }

    if (cali_cfg->dst_img_width % OT_ISP_ALIGN_WIDTH != 0) {
        isp_err_trace("Err dst_img_width:%u, width must be a muliple of 4!\n", cali_cfg->dst_img_width);
        return OT_ERR_ISP_ILLEGAL_PARAM;
    }

    if (cali_cfg->dst_img_height % OT_ISP_ALIGN_HEIGHT != 0) {
        isp_err_trace("Err dst_img_height:%u, height must be a muliple of 4!\n", cali_cfg->dst_img_height);
        return OT_ERR_ISP_ILLEGAL_PARAM;
    }

    if ((cali_cfg->offset_x % 2) != 0) { /* offset must be a muliple of 2 */
        isp_err_trace("Err offset_x %u, offset_x must be a muliple of 2!\n", cali_cfg->offset_x);
        return OT_ERR_ISP_ILLEGAL_PARAM;
    }

    if ((cali_cfg->offset_y % 2) != 0) { /* offset must be a muliple of 2 */
        isp_err_trace("Err offset_y %u, offset_y must be a muliple of 2!\n", cali_cfg->offset_y);
        return OT_ERR_ISP_ILLEGAL_PARAM;
    }

    if (cali_cfg->img_width > OT_ISP_SENSOR_WIDTH_MAX) {
        isp_err_trace("Err img_width %u, img_width must be smaller than %u!\n", cali_cfg->img_width,
            OT_ISP_SENSOR_WIDTH_MAX);
        return OT_ERR_ISP_ILLEGAL_PARAM;
    }

    if (cali_cfg->img_height > OT_ISP_SENSOR_HEIGHT_MAX) {
        isp_err_trace("Err img_height %u, img_height must be smaller than %u!\n", cali_cfg->img_height,
            OT_ISP_SENSOR_HEIGHT_MAX);
        return OT_ERR_ISP_ILLEGAL_PARAM;
    }

    if (cali_cfg->dst_img_width + cali_cfg->offset_x > cali_cfg->img_width) {
        isp_err_trace("dst_img_width + offset_x should not be lager than img_width\n");
        return OT_ERR_ISP_ILLEGAL_PARAM;
    }

    if (cali_cfg->dst_img_height + cali_cfg->offset_y > cali_cfg->img_height) {
        isp_err_trace("dst_img_height + offset_y should not be lager than img_height\n");
        return OT_ERR_ISP_ILLEGAL_PARAM;
    }

    return TD_SUCCESS;
}

td_s32 isp_mesh_shading_calibration_cfg_check(ot_isp_mlsc_calibration_cfg *cali_cfg)
{
    td_s32 ret;

    ret = isp_mesh_shading_calibration_cfg_res_check(cali_cfg);
    if (ret != TD_SUCCESS) {
        return ret;
    }

    if (cali_cfg->mesh_scale > 0x7) {
        isp_err_trace("Err mesh_scale:%u, mesh_scale must be smaller than 8!\n", cali_cfg->mesh_scale);
        return OT_ERR_ISP_ILLEGAL_PARAM;
    }

    if ((cali_cfg->raw_bit >= OT_ISP_BAYER_RAW_BIT_BUTT) || (cali_cfg->raw_bit < OT_ISP_BAYER_RAW_BIT_8BIT) ||
        ((cali_cfg->raw_bit % 2) == 1)) {    /* raw_bit must be a muliple of 2 */
        isp_err_trace("Err raw_bit type:%d, range: [%u, %u) or is not an even number!\n", cali_cfg->raw_bit,
            OT_ISP_BAYER_RAW_BIT_8BIT, OT_ISP_BAYER_RAW_BIT_BUTT);
        return OT_ERR_ISP_ILLEGAL_PARAM;
    }

    if (cali_cfg->bayer >= OT_ISP_BAYER_BUTT) {
        isp_err_trace("Err bayer type:%d >= %u!\n", cali_cfg->bayer, OT_ISP_BAYER_BUTT);
        return OT_ERR_ISP_ILLEGAL_PARAM;
    }

    if (cali_cfg->blc_offset_b > 0xFFF) {
        isp_err_trace("Err blc(chn B) offset:%u, range: [%u, %u]!\n", cali_cfg->blc_offset_b, 0, 0xFFF);
        return OT_ERR_ISP_ILLEGAL_PARAM;
    }

    if (cali_cfg->blc_offset_gb > 0xFFF) {
        isp_err_trace("Err blc(chn Gb) offset:%u, range: [%u, %u]!\n", cali_cfg->blc_offset_gb, 0, 0xFFF);
        return OT_ERR_ISP_ILLEGAL_PARAM;
    }

    if (cali_cfg->blc_offset_gr > 0xFFF) {
        isp_err_trace("Err blc(chn Gr) offset:%u, range: [%u, %u]!\n", cali_cfg->blc_offset_gr, 0, 0xFFF);
        return OT_ERR_ISP_ILLEGAL_PARAM;
    }

    if (cali_cfg->blc_offset_r > 0xFFF) {
        isp_err_trace("Err blc(chn R) offset:%u, range: [%u, %u]!\n", cali_cfg->blc_offset_r, 0, 0xFFF);
        return OT_ERR_ISP_ILLEGAL_PARAM;
    }

    return TD_SUCCESS;
}

#ifdef CONFIG_OT_ISP_LBLC_SUPPORT
td_s32 isp_lblc_lut_attr_check(const char *src, ot_vi_pipe vi_pipe, const ot_isp_lblc_lut_attr *lut_attr)
{
    td_u16 i;
    ot_unused(vi_pipe);
    if ((lut_attr->offset_r > OT_ISP_LBLC_MAX_VALUE) ||
        (lut_attr->offset_gr > OT_ISP_LBLC_MAX_VALUE) ||
        (lut_attr->offset_gb > OT_ISP_LBLC_MAX_VALUE) ||
        (lut_attr->offset_b > OT_ISP_LBLC_MAX_VALUE)) {
        isp_err_trace("Err %s offset_r:%u offset_gr:%u offset_gb:%u offset_b:%u, range: [%u, %u]!\n", src,
            lut_attr->offset_r, lut_attr->offset_gr, lut_attr->offset_gb, lut_attr->offset_b, 0, OT_ISP_LBLC_MAX_VALUE);
        return OT_ERR_ISP_ILLEGAL_PARAM;
    }

    for (i = 0; i < OT_ISP_LBLC_GRID_POINTS; i++) {
        if (lut_attr->mesh_blc_r[i] > OT_ISP_LBLC_MAX_VALUE) {
            isp_err_trace("Err %s mesh_blc_r[%u]:%u, range: [%u, %u]!\n", src, i, lut_attr->mesh_blc_r[i],
                0, OT_ISP_LBLC_MAX_VALUE);
            return OT_ERR_ISP_ILLEGAL_PARAM;
        }
        if (lut_attr->mesh_blc_gr[i] > OT_ISP_LBLC_MAX_VALUE) {
            isp_err_trace("Err %s mesh_blc_gr[%u]:%u, range: [%u, %u]!\n", src, i, lut_attr->mesh_blc_gr[i],
                0, OT_ISP_LBLC_MAX_VALUE);
            return OT_ERR_ISP_ILLEGAL_PARAM;
        }
        if (lut_attr->mesh_blc_gb[i] > OT_ISP_LBLC_MAX_VALUE) {
            isp_err_trace("Err %s mesh_blc_gb[%u]:%u, range: [%u, %u]!\n", src, i, lut_attr->mesh_blc_gb[i],
                0, OT_ISP_LBLC_MAX_VALUE);
            return OT_ERR_ISP_ILLEGAL_PARAM;
        }
        if (lut_attr->mesh_blc_b[i] > OT_ISP_LBLC_MAX_VALUE) {
            isp_err_trace("Err %s mesh_blc_b[%u]:%u, range: [%u, %u]!\n", src, i, lut_attr->mesh_blc_b[i],
                0, OT_ISP_LBLC_MAX_VALUE);
            return OT_ERR_ISP_ILLEGAL_PARAM;
        }
    }

    return TD_SUCCESS;
}
#endif

td_s32 isp_expander_attr_check(const char *src, ot_vi_pipe vi_pipe, const ot_isp_expander_attr *expander_attr)
{
    td_u8  wdr_mode = ot_ext_system_sensor_wdr_mode_read(vi_pipe);
    td_s32 i;
    isp_check_pointer_return(expander_attr);
    isp_check_bool_return(expander_attr->enable);

    if ((expander_attr->enable == TD_TRUE) && (is_built_in_wdr_mode(wdr_mode) == TD_FALSE)) {
        isp_err_trace("Only Support enable expander in sensor built-in mode!\n");
        return OT_ERR_ISP_NOT_SUPPORT;
    }

    if ((expander_attr->bit_depth_in > 0x14) || (expander_attr->bit_depth_in < 0xC) ||
        (expander_attr->bit_depth_in % 2 != 0)) { /* bit_depth_in must be a multiple of 2 */
        isp_err_trace("Err %s bit_depth_in %u, range: [%u, %u] or is not an even number!\n", src,
            expander_attr->bit_depth_in, 0xC, 0x14);
        return OT_ERR_ISP_ILLEGAL_PARAM;
    }

    if ((expander_attr->knee_point_num > OT_ISP_EXPANDER_POINT_NUM_MAX) || (expander_attr->knee_point_num < 1)) {
        isp_err_trace("Err %s knee_point_num %u, range: [%u, %u]!\n", src, expander_attr->knee_point_num,
            1, (td_u32)OT_ISP_EXPANDER_POINT_NUM_MAX);
        return OT_ERR_ISP_ILLEGAL_PARAM;
    }

    for (i = 0; i < expander_attr->knee_point_num; i++) {
        if ((expander_attr->knee_point_coord[i].x > OT_ISP_EXPANDER_X_MAX) ||
            (expander_attr->knee_point_coord[i].x < 0)) {
            isp_err_trace("Err %s knee_point_coord[%d].x %d, range: [%u, %u]!\n", src, i,
                expander_attr->knee_point_coord[i].x, 0, OT_ISP_EXPANDER_X_MAX);
            return OT_ERR_ISP_ILLEGAL_PARAM;
        }

        if ((expander_attr->knee_point_coord[i].y > OT_ISP_EXPANDER_Y_MAX) ||
            (expander_attr->knee_point_coord[i].y < 0)) {
            isp_err_trace("Err %s knee_point_coord[%d].y %d, range: [%u, %u]!\n", src, i,
                expander_attr->knee_point_coord[i].y, 0, OT_ISP_EXPANDER_Y_MAX);
            return OT_ERR_ISP_ILLEGAL_PARAM;
        }
    }

    for (i = 1; i < expander_attr->knee_point_num; i++) {
        if ((expander_attr->knee_point_coord[i].x < expander_attr->knee_point_coord[i - 1].x) ||
            (expander_attr->knee_point_coord[i].y < expander_attr->knee_point_coord[i - 1].y)) {
            isp_err_trace("The knee_point_coord must be monotonically increasing\n");
            return OT_ERR_ISP_ILLEGAL_PARAM;
        }
    }

    return isp_expander_attr_arch_check(src, expander_attr);
}

td_s32 isp_dp_dynamic_attr_check(const char *src, const ot_isp_dp_dynamic_attr *dp_dynamic_attr)
{
    td_u8 i, j;

    isp_check_pointer_return(dp_dynamic_attr);
    isp_check_bool_return(dp_dynamic_attr->enable);

    for (i = 0; i < ISP_DPC_MAX_CHN_NUM; i++) {
        const ot_isp_dp_frame_dynamic_attr *frame_dynamic_attr = &(dp_dynamic_attr->frame_dynamic[i]);
        isp_check_bool_return(frame_dynamic_attr->sup_twinkle_en);

        if (frame_dynamic_attr->manual_attr.blend_ratio > 0x80) {
            isp_err_trace("Err %s channel %u manual blend_ratio %u, range: [%u, %u]!\n", src, i,
                          frame_dynamic_attr->manual_attr.blend_ratio, 0, 0x80);
            return OT_ERR_ISP_ILLEGAL_PARAM;
        }

        if (frame_dynamic_attr->soft_thr < 0x0) {
            isp_err_trace("Err %s channel %u soft_thr %d < 0!\n", src, i, frame_dynamic_attr->soft_thr);
            return OT_ERR_ISP_ILLEGAL_PARAM;
        }

        if (frame_dynamic_attr->op_type >= OT_OP_MODE_BUTT) {
            isp_err_trace("Err %s channel %u op_type %d >= %u!\n", src, i, frame_dynamic_attr->op_type,
                OT_OP_MODE_BUTT);
            return OT_ERR_ISP_ILLEGAL_PARAM;
        }

        for (j = 0; j < OT_ISP_AUTO_ISO_NUM; j++) {
            if (frame_dynamic_attr->auto_attr.blend_ratio[j] > 0x80) {
                isp_err_trace("Err %s channel %u auto blend_ratio[%u] %u, range: [%u, %u]!\n", src, i, j,
                    frame_dynamic_attr->auto_attr.blend_ratio[j], 0, 0x80);
                return OT_ERR_ISP_ILLEGAL_PARAM;
            }
        }

        if (frame_dynamic_attr->bright_strength >= 0x80) {
            isp_err_trace("Err %s channel %u bright strength %u, range: [%u, %u)!\n", src, i,
                frame_dynamic_attr->bright_strength, 0, 0x80);
            return OT_ERR_ISP_ILLEGAL_PARAM;
        }

        if (frame_dynamic_attr->dark_strength >= 0x80) {
            isp_err_trace("Err %s channel %u dark strength %u, range: [%u, %u)!\n", src, i,
                frame_dynamic_attr->dark_strength, 0, 0x80);
            return OT_ERR_ISP_ILLEGAL_PARAM;
        }
    }

    return TD_SUCCESS;
}

td_s32 isp_dcf_info_check(const char *src, const ot_isp_dcf_info *dcf_info)
{
    td_u8 light_source;

    isp_check_pointer_return(dcf_info);
    light_source = dcf_info->isp_dcf_const_info.light_source;
    if ((light_source > OT_ISP_DCF_LIGHT_SOURCE_FLASH && light_source < OT_ISP_DCF_LIGHT_SOURCE_CLOUDY) ||
        (light_source > OT_ISP_DCF_LIGHT_SOURCE_CLOUDY && light_source < OT_ISP_DCF_LIGHT_SOURCE_STANDARD_A) ||
        (light_source > OT_ISP_DCF_LIGHT_SOURCE_D75 && light_source < OT_ISP_DCF_LIGHT_SOURCE_OTHER)) {
        isp_err_trace("Err %s light_source %u!\n", src, light_source);
        return OT_ERR_ISP_ILLEGAL_PARAM;
    }

    if (dcf_info->isp_dcf_const_info.custom_rendered > OT_ISP_DCF_CUSTOM_RENDERED_MAX) {
        isp_err_trace("Err %s custom_rendered %u, range: [%u, %u]!\n", src,
            dcf_info->isp_dcf_const_info.custom_rendered, 0, OT_ISP_DCF_CUSTOM_RENDERED_MAX);
        return OT_ERR_ISP_ILLEGAL_PARAM;
    }

    if (dcf_info->isp_dcf_const_info.scene_capture_type > OT_ISP_DCF_CAPTRUE_TYPE_MAX) {
        isp_err_trace("Err %s scene_capture_type %u, range: [%u, %u]!\n", src,
            dcf_info->isp_dcf_const_info.scene_capture_type, 0, OT_ISP_DCF_CAPTRUE_TYPE_MAX);
        return OT_ERR_ISP_ILLEGAL_PARAM;
    }

    if (dcf_info->isp_dcf_const_info.gain_control > OT_ISP_DCF_GAIN_CONTROL_MAX) {
        isp_err_trace("Err %s gain_control %u, range: [%u, %u]!\n", src,
            dcf_info->isp_dcf_const_info.gain_control, 0, OT_ISP_DCF_GAIN_CONTROL_MAX);
        return OT_ERR_ISP_ILLEGAL_PARAM;
    }

    if (dcf_info->isp_dcf_const_info.saturation > OT_ISP_DCF_SATURATION_MAX) {
        isp_err_trace("Err %s saturation %u, range: [%u, %u]!\n", src, dcf_info->isp_dcf_const_info.saturation,
            0, OT_ISP_DCF_SATURATION_MAX);
        return OT_ERR_ISP_ILLEGAL_PARAM;
    }
    return TD_SUCCESS;
}

td_s32 isp_dng_param_check(const char *src, const ot_isp_dng_color_param *dng_color_param)
{
    isp_check_pointer_return(dng_color_param);
    if (dng_color_param->wb_gain1.b_gain > 0xFFF) {
        isp_err_trace("Err %s, wb_gain1.b_gain %u, range: [%u, %u]!\n", src, dng_color_param->wb_gain1.b_gain,
            0, 0xFFF);
        return OT_ERR_ISP_ILLEGAL_PARAM;
    }
    if (dng_color_param->wb_gain1.g_gain > 0xFFF) {
        isp_err_trace("Err %s, wb_gain1.g_gain %u, range: [%u, %u]!\n", src, dng_color_param->wb_gain1.g_gain,
            0, 0xFFF);
        return OT_ERR_ISP_ILLEGAL_PARAM;
    }
    if (dng_color_param->wb_gain1.r_gain > 0xFFF) {
        isp_err_trace("Err %s, wb_gain1.r_gain %u, range: [%u, %u]!\n", src, dng_color_param->wb_gain1.r_gain,
            0, 0xFFF);
        return OT_ERR_ISP_ILLEGAL_PARAM;
    }
    if (dng_color_param->wb_gain2.b_gain > 0xFFF) {
        isp_err_trace("Err %s, wb_gain2.b_gain %u, range: [%u, %u]!\n", src, dng_color_param->wb_gain2.b_gain,
            0, 0xFFF);
        return OT_ERR_ISP_ILLEGAL_PARAM;
    }
    if (dng_color_param->wb_gain2.g_gain > 0xFFF) {
        isp_err_trace("Err %s, wb_gain2.g_gain %u, range: [%u, %u]!\n", src, dng_color_param->wb_gain2.g_gain,
            0, 0xFFF);
        return OT_ERR_ISP_ILLEGAL_PARAM;
    }
    if (dng_color_param->wb_gain2.r_gain > 0xFFF) {
        isp_err_trace("Err %s, wb_gain2.r_gain %u, range: [%u, %u]!\n", src, dng_color_param->wb_gain2.r_gain,
            0, 0xFFF);
        return OT_ERR_ISP_ILLEGAL_PARAM;
    }

    if (dng_color_param->ccm_tab1.color_temp < 2000 || dng_color_param->ccm_tab1.color_temp > 10000) { /* 2000, 10000 */
        isp_err_trace("Err %s, ccm_tab1.color_temp %u, range: [%u, %u]!\n", src,
            dng_color_param->ccm_tab1.color_temp, 2000, 10000); /* 2000, 10000 */
        return OT_ERR_ISP_ILLEGAL_PARAM;
    }

    if (dng_color_param->ccm_tab2.color_temp < 2000 || dng_color_param->ccm_tab2.color_temp > 10000) { /* 2000, 10000 */
        isp_err_trace("Err %s, ccm_tab2.color_temp %u, range: [%u, %u]!\n", src,
            dng_color_param->ccm_tab2.color_temp, 2000, 10000); /* 2000, 10000 */
        return OT_ERR_ISP_ILLEGAL_PARAM;
    }

    if (dng_color_param->wb_gain1.r_gain > dng_color_param->wb_gain2.r_gain ||
        dng_color_param->wb_gain1.b_gain < dng_color_param->wb_gain2.b_gain) {
        isp_err_trace("Err %s, wb_gain1.r_gain %u > wb_gain2.r_gain %u or wb_gain1.b_gain %u < wb_gain2.b_gain %u!\n",
            src, dng_color_param->wb_gain1.r_gain, dng_color_param->wb_gain2.r_gain, dng_color_param->wb_gain1.b_gain,
            dng_color_param->wb_gain2.b_gain);
        return OT_ERR_ISP_ILLEGAL_PARAM;
    }

    if (dng_color_param->ccm_tab1.color_temp > dng_color_param->ccm_tab2.color_temp) {
        isp_err_trace("Err %s, ccm_tab1.color_temp %u > ccm_tab2.color_temp %u!\n", src,
            dng_color_param->ccm_tab1.color_temp, dng_color_param->ccm_tab2.color_temp);
        return OT_ERR_ISP_ILLEGAL_PARAM;
    }

    return TD_SUCCESS;
}

td_s32 isp_check_ir_auto_attr(ot_vi_pipe vi_pipe, ot_isp_ir_auto_attr *ir_attr)
{
    mpi_isp_check_bool_return(ir_attr->enable);

    if (ir_attr->rg_max > 0xFFF) {
        isp_err_trace("Invalid rg_max :%u, range: [%u, %u]!\n", ir_attr->rg_max, 0, 0xFFF);
        return OT_ERR_ISP_ILLEGAL_PARAM;
    }
    if (ir_attr->rg_min > 0xFFF) {
        isp_err_trace("Invalid rg_min :%u, range: [%u, %u]!\n", ir_attr->rg_min, 0, 0xFFF);
        return OT_ERR_ISP_ILLEGAL_PARAM;
    }
    if (ir_attr->bg_max > 0xFFF) {
        isp_err_trace("Invalid bg_max :%u, range: [%u, %u]!\n", ir_attr->bg_max, 0, 0xFFF);
        return OT_ERR_ISP_ILLEGAL_PARAM;
    }
    if (ir_attr->bg_min > 0xFFF) {
        isp_err_trace("Invalid bg_min :%u, range: [%u, %u]!\n", ir_attr->bg_min, 0, 0xFFF);
        return OT_ERR_ISP_ILLEGAL_PARAM;
    }
    if (ir_attr->rg_max < ir_attr->rg_min) {
        isp_err_trace("rg_max(%u) should not be less than rg_min(%u)!\n", ir_attr->rg_max, ir_attr->rg_min);
        return OT_ERR_ISP_ILLEGAL_PARAM;
    }
    if (ir_attr->bg_max < ir_attr->bg_min) {
        isp_err_trace("bg_max(%u) should not be less than bg_min(%u)!\n", ir_attr->bg_max, ir_attr->bg_min);
        return OT_ERR_ISP_ILLEGAL_PARAM;
    }
    if (ir_attr->ir_status >= OT_ISP_IR_BUTT) {
        isp_err_trace("Invalid ir_status :%u >= %u!\n", ir_attr->ir_status, OT_ISP_IR_BUTT);
        return OT_ERR_ISP_ILLEGAL_PARAM;
    }
    return TD_SUCCESS;
}

td_s32 isp_smart_info_param_check(ot_vi_pipe vi_pipe, const ot_isp_smart_info *smart_info)
{
    td_s32 i;
    td_u16 max_width, max_height;
    const td_u32 max_area_ratio = 10000; /* maximum value of max_area_ratio */
    const td_u32 max_exp_pref = 10000;   /* maximum value of tunnel_exp_perf */

    max_width = ot_ext_sync_total_width_read(vi_pipe);
    max_height = ot_ext_sync_total_height_read(vi_pipe);

    for (i = 0; i < OT_ISP_FACE_NUM; i++) {
        if ((smart_info->face_roi.face_rect[i].x < 0) || (smart_info->face_roi.face_rect[i].y < 0)) {
            isp_err_trace("face_rect[%d].x:%d or face_rect[%d].y:%d is less than zero!\n",
                i, smart_info->face_roi.face_rect[i].x, i, smart_info->face_roi.face_rect[i].y);
            return OT_ERR_ISP_ILLEGAL_PARAM;
        }

        if ((smart_info->face_roi.face_rect[i].x > max_width) || (smart_info->face_roi.face_rect[i].y > max_height)) {
            isp_err_trace("face_rect[%d].x:%d or face_rect[%d].y:%d exceeds the maximum,max_width:%u max_height:%u!\n",
                i, smart_info->face_roi.face_rect[i].x, i, smart_info->face_roi.face_rect[i].y,
                max_width, max_height);
            return OT_ERR_ISP_ILLEGAL_PARAM;
        }

        if (smart_info->face_roi.face_rect[i].x + smart_info->face_roi.face_rect[i].width > max_width) {
            isp_err_trace("The sum of face_rect[%d].x:%d and face_rect[%d].width:%u exceeds the maximum:%u!\n",
                i, smart_info->face_roi.face_rect[i].x, i, smart_info->face_roi.face_rect[i].width,
                max_width);
            return OT_ERR_ISP_ILLEGAL_PARAM;
        }

        if (smart_info->face_roi.face_rect[i].y + smart_info->face_roi.face_rect[i].height > max_height) {
            isp_err_trace("The sum of face_rect[%d].y:%d and face_rect[%d].height :%u exceeds the maximum:%u!\n",
                i, smart_info->face_roi.face_rect[i].y, i, smart_info->face_roi.face_rect[i].height,
                max_height);
            return OT_ERR_ISP_ILLEGAL_PARAM;
        }
    }

    for (i = 0; i < OT_ISP_TUNNEL_CLASS_MAX; i++) {
        if (smart_info->tunnel_roi[i].tunnel_area_ratio > max_area_ratio) {
            isp_err_trace("tunnel_roi[%d].tunnel_area_ratio:%d is more than maximum:%d!\n",
                i, smart_info->tunnel_roi[i].tunnel_area_ratio, max_area_ratio);
            return OT_ERR_ISP_ILLEGAL_PARAM;
        }

        if (smart_info->tunnel_roi[i].tunnel_exp_perf > max_exp_pref) {
            isp_err_trace("tunnel_roi[%d].tunnel_exp_perf:%d is more than maximum:%d!\n",
                i, smart_info->tunnel_roi[i].tunnel_exp_perf, max_exp_pref);
            return OT_ERR_ISP_ILLEGAL_PARAM;
        }
    }
    return TD_SUCCESS;
}

td_s32 isp_motion_stats_attr_check(ot_vi_pipe vi_pipe, const ot_isp_motion_stats_cfg *motion_cfg)
{
    return isp_motion_stats_attr_arch_check(vi_pipe, motion_cfg);
}
#ifdef CONFIG_OT_VI_PIPE_FPN
td_s32 isp_fpn_correction_attr_check(ot_vi_pipe vi_pipe, const ot_isp_fpn_attr *correction)
{
    td_u8 i;
    td_u8 wdr_mode = ot_ext_system_sensor_wdr_mode_read(vi_pipe);
    if (is_2to1_wdr_mode(wdr_mode)) {
        isp_err_trace("do not support FPN correction in 2to1 WDR mode!\n");
        return OT_ERR_ISP_NOT_SUPPORT;
    }

    if (correction->op_type >= OT_OP_MODE_BUTT) {
        isp_err_trace("invalid op_type %d, which should be OT_OP_MODE_AUTO or OT_OP_MODE_MANUAL.\n",
            correction->op_type);
        return OT_ERR_ISP_ILLEGAL_PARAM;
    }

    if (correction->fpn_type != OT_ISP_FPN_TYPE_FRAME) {
        isp_err_trace("invalid fpn_type %d.\n", correction->fpn_type);
        return OT_ERR_ISP_ILLEGAL_PARAM;
    }

    for (i = 0; i < OT_ISP_STRIPING_MAX_NUM; i++) {
        if (correction->fpn_frm_info.offset[i] > 0x3FFF) {
            isp_err_trace("invalid offset[%u], must be in [0, 0x3FFF].\n", i);
            return OT_ERR_ISP_ILLEGAL_PARAM;
        }
    }

    if (correction->op_type == OT_OP_MODE_MANUAL) {
        if (correction->manual_attr.strength > 0x3FF) {
            isp_err_trace("invalid fpn strength, must be in [0, 1023]!\n");
            return OT_ERR_ISP_ILLEGAL_PARAM;
        }
    }
    return isp_fpn_correction_attr_arch_check(vi_pipe, correction);
}

static td_bool isp_check_2_power(td_u32 value)
{
    td_bool ret = TD_TRUE;
    td_u32 temp_val = value;
    td_u32 num = 0;

    while (temp_val) {
        if (temp_val & 0x1) {
            num++;
            if (num > 1) {
                ret = TD_FALSE;
                break;
            }
        }

        temp_val >>= 1;
    }

    return ret;
}

td_s32 isp_fpn_calibrate_attr_check(ot_vi_pipe vi_pipe, const ot_isp_fpn_calibrate_attr *calibrate_attr)
{
    td_bool fpn_cor_enable;
    td_u8 wdr_mode = ot_ext_system_sensor_wdr_mode_read(vi_pipe);
    if (is_wdr_mode(wdr_mode)) {
        isp_err_trace("do not support FPN calibration in WDR mode!\n");
        return OT_ERR_ISP_NOT_SUPPORT;
    }

    fpn_cor_enable = isp_ext_system_fpn_cor_enable_read(vi_pipe);
    if (fpn_cor_enable) {
        isp_err_trace("cannot do FPN calibration when FPN correction is on!\n");
        return OT_ERR_ISP_NOT_SUPPORT;
    }

    if (calibrate_attr->fpn_type != OT_ISP_FPN_TYPE_FRAME) {
        isp_err_trace("invalid fpn_type %d, must be ISP_FPN_TYPE_FRAME.\n", calibrate_attr->fpn_type);
        return OT_ERR_ISP_NOT_SUPPORT;
    }

    if (calibrate_attr->fpn_mode >= OT_ISP_FPN_OUT_MODE_BUTT) {
        isp_err_trace("invalid fpn_mode %d.\n", calibrate_attr->fpn_mode);
        return OT_ERR_ISP_ILLEGAL_PARAM;
    }

    if ((calibrate_attr->frame_num < 1) || (calibrate_attr->frame_num > 16)) { /* [1, 16] */
        isp_err_trace("invalid frame_num %u, must be in [1, 16].\n", calibrate_attr->frame_num);
        return OT_ERR_ISP_ILLEGAL_PARAM;
    }
    if (isp_check_2_power(calibrate_attr->frame_num) == TD_FALSE) {
        isp_err_trace("invalid frame_num %u, must be 2^N.\n", calibrate_attr->frame_num);
        return OT_ERR_ISP_ILLEGAL_PARAM;
    }
    if ((calibrate_attr->threshold > 0xfff) || (calibrate_attr->threshold < 1)) {
        isp_err_trace("invalid threshold %u, must be in [1, 0xfff].\n", calibrate_attr->threshold);
        return OT_ERR_ISP_ILLEGAL_PARAM;
    }

    return TD_SUCCESS;
}
#endif
