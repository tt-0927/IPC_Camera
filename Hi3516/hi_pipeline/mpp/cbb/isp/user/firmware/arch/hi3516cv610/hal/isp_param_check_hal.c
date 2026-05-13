/*
  Copyright (c), 2001-2025, Shenshu Tech. Co., Ltd.
 */

#include "isp_param_check_hal.h"
#include "isp_main.h"
#include "isp_ext_config.h"
td_s32 isp_sharpen_common_attr_arch_check(const char *src, const ot_isp_sharpen_attr *sharpen_attr)
{
    if (sharpen_attr->detail_map != OT_ISP_SHARPEN_NORMAL) {
        isp_err_trace("Err %s detail_map %d!\n", src, sharpen_attr->detail_map);
        return OT_ERR_ISP_ILLEGAL_PARAM;
    }
    return TD_SUCCESS;
}


static td_s32 isp_sharpen_manual_shoot_threshold_attr_check(const char *src,
    const ot_isp_sharpen_manual_attr *manual_attr)
{
    if (manual_attr->shoot_threshold_attr.shoot_inner_threshold > 0x1FF) {
        isp_err_trace("Err %s manual shoot_inner_threshold:%u! range:[0, 511]\n", src,
            manual_attr->shoot_threshold_attr.shoot_inner_threshold);
        return OT_ERR_ISP_ILLEGAL_PARAM;
    }

    if (manual_attr->shoot_threshold_attr.shoot_outer_threshold > 0x1FF) {
        isp_err_trace("Err %s manual shoot_outer_threshold:%u! range:[0, 511]\n", src,
            manual_attr->shoot_threshold_attr.shoot_outer_threshold);
        return OT_ERR_ISP_ILLEGAL_PARAM;
    }

    if (manual_attr->shoot_threshold_attr.shoot_protect_threshold > 0x1FF) {
        isp_err_trace("Err %s manual shoot_protect_threshold:%u! range:[0, 511]\n", src,
            manual_attr->shoot_threshold_attr.shoot_protect_threshold);
        return OT_ERR_ISP_ILLEGAL_PARAM;
    }
    return TD_SUCCESS;
}

static td_s32 isp_sharpen_manual_edge_rly_attr_check(const char *src,
    const ot_isp_sharpen_manual_attr *manual_attr)
{
    td_u8 j;

    if (manual_attr->edge_rly_attr.edge_rly_fine_threshold > 0x3FF) {
        isp_err_trace("Err %s manual edge_rly_fine_threshold:%u! range:[0, 1023]\n", src,
            manual_attr->edge_rly_attr.edge_rly_fine_threshold);
        return OT_ERR_ISP_ILLEGAL_PARAM;
    }

    if (manual_attr->edge_rly_attr.edge_rly_coarse_threshold > 0x3FF) {
        isp_err_trace("Err %s manual edge_rly_coarse_threshold:%u! range:[0, 1023]\n", src,
            manual_attr->edge_rly_attr.edge_rly_coarse_threshold);
        return OT_ERR_ISP_ILLEGAL_PARAM;
    }

    if (manual_attr->edge_rly_attr.edge_overshoot > 0x7F) {
        isp_err_trace("Err %s manual edge_overshoot:%u! range:[0, 127]\n", src,
            manual_attr->edge_rly_attr.edge_overshoot);
        return OT_ERR_ISP_ILLEGAL_PARAM;
    }

    if (manual_attr->edge_rly_attr.edge_undershoot > 0x7F) {
        isp_err_trace("Err %s manual edge_undershoot:%u! range:[0, 127]\n", src,
            manual_attr->edge_rly_attr.edge_undershoot);
        return OT_ERR_ISP_ILLEGAL_PARAM;
    }

    for (j = 0; j < OT_ISP_SHARPEN_STDGAIN_NUM; j++) {
        if (manual_attr->edge_rly_attr.edge_rly_by_mot[j] > 0x3F) {
            isp_err_trace("Err %s manual edge_rly_by_mot[%u]:%u! range:[0, 63]\n", src, j,
                manual_attr->edge_rly_attr.edge_rly_by_mot[j]);
            return OT_ERR_ISP_ILLEGAL_PARAM;
        }
        if (manual_attr->edge_rly_attr.edge_rly_by_luma[j] > 0x3F) {
            isp_err_trace("Err %s manual edge_rly_by_luma[%u]:%u! range:[0, 63]\n", src, j,
                manual_attr->edge_rly_attr.edge_rly_by_luma[j]);
            return OT_ERR_ISP_ILLEGAL_PARAM;
        }
    }
    return TD_SUCCESS;
}

static td_s32 isp_sharpen_manual_gain_by_mot_attr_check(const char *src,
    const ot_isp_sharpen_manual_attr *manual_attr)
{
    td_u8 j;
    for (j = 0; j < OT_ISP_SHARPEN_MOT_NUM; j++) {
        if (manual_attr->gain_by_mot_attr.mf_gain_by_mot[j] > 0x20) {
            isp_err_trace("Err %s manual mf_gain_by_mot[%u]:%u! range:[0, 32]\n", src, j,
                manual_attr->gain_by_mot_attr.mf_gain_by_mot[j]);
            return OT_ERR_ISP_ILLEGAL_PARAM;
        }
        if (manual_attr->gain_by_mot_attr.hf_gain_by_mot[j] > 0x20) {
            isp_err_trace("Err %s manual hf_gain_by_mot[%u]:%u! range:[0, 32]\n", src, j,
                manual_attr->gain_by_mot_attr.hf_gain_by_mot[j]);
            return OT_ERR_ISP_ILLEGAL_PARAM;
        }
    }
    return TD_SUCCESS;
}

td_s32 isp_sharpen_manual_attr_arch_check(const char *src, const ot_isp_sharpen_manual_attr *manual_attr)
{
    td_s32 ret;
    ret = isp_sharpen_manual_shoot_threshold_attr_check(src, manual_attr);
    if (ret != TD_SUCCESS) {
        return ret;
    }

    ret = isp_sharpen_manual_edge_rly_attr_check(src, manual_attr);
    if (ret != TD_SUCCESS) {
        return ret;
    }

    ret = isp_sharpen_manual_gain_by_mot_attr_check(src, manual_attr);
    if (ret != TD_SUCCESS) {
        return ret;
    }

    return TD_SUCCESS;
}

static td_s32 isp_sharpen_auto_shoot_threshold_attr_check(const char *src,
    const ot_isp_sharpen_auto_attr *auto_attr)
{
    td_u8 i;

    for (i = 0; i < OT_ISP_AUTO_ISO_NUM; i++) {
        if (auto_attr->shoot_threshold_attr.shoot_inner_threshold[i] > 0x1FF) {
            isp_err_trace("Err %s auto shoot_inner_threshold[%u]:%u! range:[0, 511]\n", src, i,
                auto_attr->shoot_threshold_attr.shoot_inner_threshold[i]);
            return OT_ERR_ISP_ILLEGAL_PARAM;
        }

        if (auto_attr->shoot_threshold_attr.shoot_outer_threshold[i] > 0x1FF) {
            isp_err_trace("Err %s auto shoot_outer_threshold[%u]:%u! range:[0, 511]\n", src, i,
                auto_attr->shoot_threshold_attr.shoot_outer_threshold[i]);
            return OT_ERR_ISP_ILLEGAL_PARAM;
        }

        if (auto_attr->shoot_threshold_attr.shoot_protect_threshold[i] > 0x1FF) {
            isp_err_trace("Err %s auto shoot_protect_threshold[%u]:%u! range:[0, 511]\n", src, i,
                auto_attr->shoot_threshold_attr.shoot_protect_threshold[i]);
            return OT_ERR_ISP_ILLEGAL_PARAM;
        }
    }
    return TD_SUCCESS;
}

static td_s32 isp_sharpen_auto_edge_rly_attr_check(const char *src,
    const ot_isp_sharpen_auto_attr *auto_attr)
{
    td_u8 i, j;

    for (i = 0; i < OT_ISP_AUTO_ISO_NUM; i++) {
        if (auto_attr->edge_rly_attr.edge_rly_fine_threshold[i] > 0x3FF) {
            isp_err_trace("Err %s auto edge_rly_fine_threshold[%u]:%u! range:[0, 1023]\n", src, i,
                auto_attr->edge_rly_attr.edge_rly_fine_threshold[i]);
            return OT_ERR_ISP_ILLEGAL_PARAM;
        }

        if (auto_attr->edge_rly_attr.edge_rly_coarse_threshold[i] > 0x3FF) {
            isp_err_trace("Err %s auto edge_rly_coarse_threshold[%u]:%u! range:[0, 1023]\n", src, i,
                auto_attr->edge_rly_attr.edge_rly_coarse_threshold[i]);
            return OT_ERR_ISP_ILLEGAL_PARAM;
        }

        if (auto_attr->edge_rly_attr.edge_overshoot[i] > 0x7F) {
            isp_err_trace("Err %s auto_edge_overshoot[%u]:%u! range:[0, 127]\n", src, i,
                auto_attr->edge_rly_attr.edge_overshoot[i]);
            return OT_ERR_ISP_ILLEGAL_PARAM;
        }

        if (auto_attr->edge_rly_attr.edge_undershoot[i] > 0x7F) {
            isp_err_trace("Err %s auto_edge_undershoot[%u]:%u! range:[0, 127]\n", src, i,
                auto_attr->edge_rly_attr.edge_undershoot[i]);
            return OT_ERR_ISP_ILLEGAL_PARAM;
        }

        for (j = 0; j < OT_ISP_SHARPEN_STDGAIN_NUM; j++) {
            if (auto_attr->edge_rly_attr.edge_rly_by_mot[j][i] > 0x3F) {
                isp_err_trace("Err %s auto edge_rly_by_mot[%u][%u]:%u! range:[0, 63]\n", src, j, i,
                    auto_attr->edge_rly_attr.edge_rly_by_mot[j][i]);
                return OT_ERR_ISP_ILLEGAL_PARAM;
            }
            if (auto_attr->edge_rly_attr.edge_rly_by_luma[j][i] > 0x3F) {
                isp_err_trace("Err %s auto edge_rly_by_luma[%u][%u]:%u! range:[0, 63]\n", src, j, i,
                    auto_attr->edge_rly_attr.edge_rly_by_luma[j][i]);
                return OT_ERR_ISP_ILLEGAL_PARAM;
            }
        }
    }
    return TD_SUCCESS;
}

static td_s32 isp_sharpen_auto_gain_by_mot_attr_check(const char *src,
    const ot_isp_sharpen_auto_attr *auto_attr)
{
    td_u8 i, j;

    for (i = 0; i < OT_ISP_AUTO_ISO_NUM; i++) {
        for (j = 0; j < OT_ISP_SHARPEN_MOT_NUM; j++) {
            if (auto_attr->gain_by_mot_attr.mf_gain_by_mot[j][i] > 0x20) {
                isp_err_trace("Err %s auto mf_gain_by_mot[%u][%u]:%u! range:[0, 32]\n", src, j, i,
                    auto_attr->gain_by_mot_attr.mf_gain_by_mot[j][i]);
                return OT_ERR_ISP_ILLEGAL_PARAM;
            }
            if (auto_attr->gain_by_mot_attr.hf_gain_by_mot[j][i] > 0x20) {
                isp_err_trace("Err %s auto hf_gain_by_mot[%u][%u]:%u! range:[0, 32]\n", src, j, i,
                    auto_attr->gain_by_mot_attr.hf_gain_by_mot[j][i]);
                return OT_ERR_ISP_ILLEGAL_PARAM;
            }
        }
    }
    return TD_SUCCESS;
}

td_s32 isp_sharpen_auto_attr_arch_check(const char *src, const ot_isp_sharpen_auto_attr *auto_attr)
{
    td_s32 ret;
    ret = isp_sharpen_auto_shoot_threshold_attr_check(src, auto_attr);
    if (ret != TD_SUCCESS) {
        return ret;
    }

    ret = isp_sharpen_auto_edge_rly_attr_check(src, auto_attr);
    if (ret != TD_SUCCESS) {
        return ret;
    }

    ret = isp_sharpen_auto_gain_by_mot_attr_check(src, auto_attr);
    if (ret != TD_SUCCESS) {
        return ret;
    }

    return TD_SUCCESS;
}

static td_s32 isp_drc_bcnr_attr_check(const char *src, const ot_isp_drc_attr *drc_attr)
{
    isp_check_bool_return(drc_attr->bcnr_attr.enable);

    if (drc_attr->bcnr_attr.strength > 8) { /* val 8 */
        isp_err_trace("Err %s strength: %u!\n", src, drc_attr->bcnr_attr.strength);
        return OT_ERR_ISP_ILLEGAL_PARAM;
    }
    return TD_SUCCESS;
}

static td_s32 isp_drc_auto_curve_attr_check(const char *src, const ot_isp_drc_attr *drc_attr)
{
    if (drc_attr->auto_curve.brightness > 0xF) {
        isp_err_trace("Err %s auto_curve.brightness: %u!\n", src, drc_attr->auto_curve.brightness);
        return OT_ERR_ISP_ILLEGAL_PARAM;
    }

    if (drc_attr->auto_curve.contrast > 0xF) {
        isp_err_trace("Err %s auto_curve.contrast: %u!\n", src, drc_attr->auto_curve.contrast);
        return OT_ERR_ISP_ILLEGAL_PARAM;
    }

    if (drc_attr->auto_curve.tolerance > 0xF) {
        isp_err_trace("Err %s auto_curve.tolerance: %u!\n", src, drc_attr->auto_curve.tolerance);
        return OT_ERR_ISP_ILLEGAL_PARAM;
    }

    return TD_SUCCESS;
}

static td_s32 isp_drc_local_mixing_param_check(const char *src, const ot_isp_drc_attr *drc_attr)
{
    if (drc_attr->local_mixing_bright_param.max > 0x80) {
        isp_err_trace("Err %s local_mixing_bright_param.max: %u!\n", src, drc_attr->local_mixing_bright_param.max);
        return OT_ERR_ISP_ILLEGAL_PARAM;
    }

    if (drc_attr->local_mixing_bright_param.min > 0x80) {
        isp_err_trace("Err %s local_mixing_bright_param.min: %u!\n", src, drc_attr->local_mixing_bright_param.min);
        return OT_ERR_ISP_ILLEGAL_PARAM;
    }

    if (drc_attr->local_mixing_bright_param.slope > 7 || drc_attr->local_mixing_bright_param.slope < -7) { /* val 7 */
        isp_err_trace("Err %s local_mixing_bright_param.slope: %d!\n", src, drc_attr->local_mixing_bright_param.slope);
        return OT_ERR_ISP_ILLEGAL_PARAM;
    }

    if (drc_attr->local_mixing_bright_param.max < drc_attr->local_mixing_bright_param.min) {
        isp_err_trace(
            "Err %s local_mixing_bright_param.max (%u) must be greater than local_mixing_bright_param.min (%u)!\n",
            src, drc_attr->local_mixing_bright_param.max, drc_attr->local_mixing_bright_param.min);
        return OT_ERR_ISP_ILLEGAL_PARAM;
    }

    if (drc_attr->local_mixing_dark_param.max > 0x80) {
        isp_err_trace("Err %s local_mixing_dark_param.max: %u!\n", src, drc_attr->local_mixing_dark_param.max);
        return OT_ERR_ISP_ILLEGAL_PARAM;
    }

    if (drc_attr->local_mixing_dark_param.min > 0x80) {
        isp_err_trace("Err %s local_mixing_dark_param.min: %u!\n", src, drc_attr->local_mixing_dark_param.min);
        return OT_ERR_ISP_ILLEGAL_PARAM;
    }

    if (drc_attr->local_mixing_dark_param.slope > 7 || drc_attr->local_mixing_dark_param.slope < -7) { /* val 7 */
        isp_err_trace("Err %s local_mixing_dark_param.slope: %d!\n", src, drc_attr->local_mixing_dark_param.slope);
        return OT_ERR_ISP_ILLEGAL_PARAM;
    }

    if (drc_attr->local_mixing_dark_param.max < drc_attr->local_mixing_dark_param.min) {
        isp_err_trace(
            "Err %s local_mixing_dark_param.max (%u) must be greater than local_mixing_dark_param.min (%u)!\n",
            src, drc_attr->local_mixing_dark_param.max, drc_attr->local_mixing_dark_param.min);
        return OT_ERR_ISP_ILLEGAL_PARAM;
    }

    return TD_SUCCESS;
}

td_s32 isp_drc_attr_arch_check(const char *src, const ot_isp_drc_attr *drc_attr)
{
    if (isp_drc_local_mixing_param_check(src, drc_attr) == OT_ERR_ISP_ILLEGAL_PARAM) {
        return OT_ERR_ISP_ILLEGAL_PARAM;
    }

    if (isp_drc_auto_curve_attr_check(src, drc_attr) == OT_ERR_ISP_ILLEGAL_PARAM) {
        return OT_ERR_ISP_ILLEGAL_PARAM;
    }

    if (isp_drc_bcnr_attr_check(src, drc_attr) == OT_ERR_ISP_ILLEGAL_PARAM) {
        return OT_ERR_ISP_ILLEGAL_PARAM;
    }

    if ((drc_attr->curve_select >= OT_ISP_DRC_CURVE_BUTT)) {
        isp_err_trace("Err %s curve_select: %d!\n", src, drc_attr->curve_select);
        return OT_ERR_ISP_ILLEGAL_PARAM;
    }

    return TD_SUCCESS;
}

td_s32 isp_ca_attr_arch_check(const char *src, const ot_isp_ca_attr *ca_attr)
{
    if (ca_attr->ca_cp_en == OT_ISP_CP_ENABLE) {
        isp_err_trace("Err %s not support ca type %d CP mode!\n", src, ca_attr->ca_cp_en);
        return OT_ERR_ISP_NOT_SUPPORT;
    }
    return TD_SUCCESS;
}

static td_s32 isp_nr_snr_auto_attr_arch_check(const char *src, const ot_isp_nr_snr_auto_attr *snr_auto)
{
    td_u8 i;

    for (i = 0; i < OT_ISP_AUTO_ISO_NUM; i++) {
        if (snr_auto->tss[i] > 128) { /* Range:[0, 128] */
            isp_err_trace("Err %s tss[%u] %u!\n", src, i, snr_auto->tss[i]);
            return OT_ERR_ISP_ILLEGAL_PARAM;
        }
    }

    return TD_SUCCESS;
}

static td_s32 isp_nr_snr_manual_attr_arch_check(const char *src, const ot_isp_nr_snr_manual_attr *snr_manual)
{
    if (snr_manual->tss > 128) { /* Range:[0, 128] */
        isp_err_trace("Err %s tss %u!\n", src, snr_manual->tss);
        return OT_ERR_ISP_ILLEGAL_PARAM;
    }

    return TD_SUCCESS;
}

td_s32 isp_nr_snr_attr_arch_check(const char *src, const ot_isp_nr_snr_attr *snr_cfg)
{
    td_s32 ret;

    ret = isp_nr_snr_auto_attr_arch_check(src, &snr_cfg->snr_attr.snr_auto);
    if (ret != TD_SUCCESS) {
        return ret;
    }
    ret = isp_nr_snr_manual_attr_arch_check(src, &snr_cfg->snr_attr.snr_manual);
    if (ret != TD_SUCCESS) {
        return ret;
    }
    return TD_SUCCESS;
}

static td_s32 isp_nr_auto_attr_arch_check(const char *src, const ot_isp_nr_attr *nr_attr)
{
    td_u8 i;
    const ot_isp_nr_md_auto_attr *md_auto = &nr_attr->md_cfg.md_auto;

    for (i = 0; i < OT_ISP_AUTO_ISO_NUM; i++) {
        if (md_auto->md_mode[i] > 2) { /* Range:[0, 2] */
            isp_err_trace("Err %s md_mode[%u] %u!\n", src, i, md_auto->md_mode[i]);
            return OT_ERR_ISP_ILLEGAL_PARAM;
        }

        if (md_auto->md_size_ratio[i] > 32) { /* Range:[0, 32] */
            isp_err_trace("Err %s md_size_ratio[%u] %u!\n", src, i, md_auto->md_size_ratio[i]);
            return OT_ERR_ISP_ILLEGAL_PARAM;
        }

        if (md_auto->md_anti_flicker_strength[i] > 64) { /* Range:[0, 64] */
            isp_err_trace("Err %s md_anti_flicker_strength[%u] %u!\n", src, i, md_auto->md_anti_flicker_strength[i]);
            return OT_ERR_ISP_ILLEGAL_PARAM;
        }

        if (md_auto->md_static_ratio[i] > 64) { /* Range:[0, 64] */
            isp_err_trace("Err %s md_static_ratio[%u] %u!\n", src, i, md_auto->md_static_ratio[i]);
            return OT_ERR_ISP_ILLEGAL_PARAM;
        }

        if (md_auto->md_motion_ratio[i] > 64) { /* Range:[0, 64] */
            isp_err_trace("Err %s md_motion_ratio[%u] %u!\n", src, i, md_auto->md_motion_ratio[i]);
            return OT_ERR_ISP_ILLEGAL_PARAM;
        }

        if (md_auto->user_define_md[i] > 2) { /* Range:[0, 2] */
            isp_err_trace("Err %s user_define_md[%u] %u!\n", src, i, md_auto->user_define_md[i]);
            return OT_ERR_ISP_ILLEGAL_PARAM;
        }

        if (md_auto->user_define_color_thresh[i] > 64) { /* Range:[0, 64] */
            isp_err_trace("Err %s user_define_color_thresh[%u] %u!\n", src, i, md_auto->user_define_color_thresh[i]);
            return OT_ERR_ISP_ILLEGAL_PARAM;
        }

        if (md_auto->sfr_r[i] > 128) { /* Range:[0, 128] */
            isp_err_trace("Err %s sfr_r[%u] %u!\n", src, i, md_auto->sfr_r[i]);
            return OT_ERR_ISP_ILLEGAL_PARAM;
        }

        if (md_auto->sfr_g[i] > 128) { /* Range:[0, 128] */
            isp_err_trace("Err %s sfr_g[%u] %u!\n", src, i, md_auto->sfr_g[i]);
            return OT_ERR_ISP_ILLEGAL_PARAM;
        }

        if (md_auto->sfr_b[i] > 128) { /* Range:[0, 128] */
            isp_err_trace("Err %s sfr_b[%u] %u!\n", src, i, md_auto->sfr_b[i]);
            return OT_ERR_ISP_ILLEGAL_PARAM;
        }
    }

    return TD_SUCCESS;
}

static td_s32 isp_nr_manual_attr_arch_check(const char *src, const ot_isp_nr_attr *nr_attr)
{
    const ot_isp_nr_md_manual_attr *md_manual = &nr_attr->md_cfg.md_manual;

    if (md_manual->md_mode > 2) { /* Range:[0, 2] */
        isp_err_trace("Err %s md_mode %u!\n", src, md_manual->md_mode);
        return OT_ERR_ISP_ILLEGAL_PARAM;
    }

    if (md_manual->md_size_ratio > 32) { /* Range:[0, 32] */
        isp_err_trace("Err %s md_size_ratio %u!\n", src, md_manual->md_size_ratio);
        return OT_ERR_ISP_ILLEGAL_PARAM;
    }

    if (md_manual->md_anti_flicker_strength > 64) { /* Range:[0, 64] */
        isp_err_trace("Err %s md_anti_flicker_strength %u!\n", src, md_manual->md_anti_flicker_strength);
        return OT_ERR_ISP_ILLEGAL_PARAM;
    }

    if (md_manual->md_static_ratio > 64) { /* Range:[0, 64] */
        isp_err_trace("Err %s md_static_ratio %u!\n", src, md_manual->md_static_ratio);
        return OT_ERR_ISP_ILLEGAL_PARAM;
    }

    if (md_manual->md_motion_ratio > 64) { /* Range:[0, 64] */
        isp_err_trace("Err %s md_motion_ratio %u!\n", src, md_manual->md_motion_ratio);
        return OT_ERR_ISP_ILLEGAL_PARAM;
    }

    if (md_manual->user_define_md > 2) { /* Range:[0, 2] */
        isp_err_trace("Err %s user_define_md %u!\n", src, md_manual->user_define_md);
        return OT_ERR_ISP_ILLEGAL_PARAM;
    }

    if (md_manual->user_define_color_thresh > 64) { /* Range:[0, 64] */
        isp_err_trace("Err %s user_define_color_thresh %u!\n", src, md_manual->user_define_color_thresh);
        return OT_ERR_ISP_ILLEGAL_PARAM;
    }

    if (md_manual->sfr_r > 128) { /* Range:[0, 128] */
        isp_err_trace("Err %s sfr_r %u!\n", src, md_manual->sfr_r);
        return OT_ERR_ISP_ILLEGAL_PARAM;
    }

    if (md_manual->sfr_g > 128) { /* Range:[0, 128] */
        isp_err_trace("Err %s sfr_g %u!\n", src, md_manual->sfr_g);
        return OT_ERR_ISP_ILLEGAL_PARAM;
    }

    if (md_manual->sfr_b > 128) { /* Range:[0, 128] */
        isp_err_trace("Err %s sfr_b %u!\n", src, md_manual->sfr_b);
        return OT_ERR_ISP_ILLEGAL_PARAM;
    }

    return TD_SUCCESS;
}

td_s32 isp_nr_attr_arch_check(const char *src, const ot_isp_nr_attr *nr_attr)
{
    td_s32 ret;

    ret = isp_nr_auto_attr_arch_check(src, nr_attr);
    if (ret != TD_SUCCESS) {
        return ret;
    }
    ret = isp_nr_manual_attr_arch_check(src, nr_attr);
    if (ret != TD_SUCCESS) {
        return ret;
    }
    return TD_SUCCESS;
}

td_s32 isp_demosaic_manual_attr_arch_check(const char *src, const ot_isp_demosaic_manual_attr *manual_attr)
{
    if (manual_attr->hf_detail_strength > OT_ISP_DEMOSAIC_HFDETALEHC_STR_MAX) {
        isp_err_trace("Err %s hf_detail_strength %u!\n", src, manual_attr->hf_detail_strength);
        return OT_ERR_ISP_ILLEGAL_PARAM;
    }
    return TD_SUCCESS;
}

td_s32 isp_demosaic_auto_attr_arch_check(const char *src, const ot_isp_demosaic_auto_attr *auto_attr)
{
    td_s32 i;
    for (i = 0; i < OT_ISP_AUTO_ISO_NUM; i++) {
        if (auto_attr->hf_detail_strength[i] > OT_ISP_DEMOSAIC_HFDETALEHC_STR_MAX) {
            isp_err_trace("Err %s hf_detail_strength[%d] %u!\n", src, i, auto_attr->hf_detail_strength[i]);
            return OT_ERR_ISP_ILLEGAL_PARAM;
        }
    }
    return TD_SUCCESS;
}

td_s32 isp_cac_acac_manual_attr_arch_check(const char *src, const ot_isp_cac_acac_manual_attr *acac_attr)
{
    ot_unused(src);
    ot_unused(acac_attr);
    return TD_SUCCESS;
}

td_s32 isp_cac_acac_auto_attr_arch_check(const char *src, const ot_isp_cac_acac_auto_attr *acac_attr)
{
    ot_unused(src);
    ot_unused(acac_attr);
    return TD_SUCCESS;
}

td_s32 isp_expander_attr_arch_check(const char *src, const ot_isp_expander_attr *expander_attr)
{
    if ((expander_attr->bit_depth_out > 0x14) || (expander_attr->bit_depth_out < 0xC)) {
        isp_err_trace("Err %s bit_depth_out!\n", src);
        return OT_ERR_ISP_ILLEGAL_PARAM;
    }
    return TD_SUCCESS;
}

td_s32 isp_motion_stats_attr_arch_check(ot_vi_pipe vi_pipe, const ot_isp_motion_stats_cfg *motion_cfg)
{
    td_bool bnr_en, md_en;
    isp_check_bool_return(motion_cfg->motion_stats_en);

    bnr_en = ot_ext_system_bayernr_enable_read(vi_pipe);
    md_en = ot_ext_system_bayernr_tnr_enable_read(vi_pipe);
    if (((md_en && bnr_en) == TD_FALSE) && motion_cfg->motion_stats_en == TD_TRUE) {
        isp_err_trace("ISP[%d] motion_stats_en not support for md disable(bnr:%d, md:%d)\n", vi_pipe, bnr_en, md_en);
        return OT_ERR_ISP_NOT_SUPPORT;
    }
    if (motion_cfg->motion_thr > 0x10) { /* 4bit, [0, 0x10] */
        isp_err_trace("ISP[%d] motion_thr:%u not support, the scope is [0, 0x10]\n", vi_pipe, motion_cfg->motion_thr);
        return OT_ERR_ISP_ILLEGAL_PARAM;
    }
    return TD_SUCCESS;
}
#ifdef CONFIG_OT_VI_PIPE_FPN
td_s32 isp_fpn_correction_attr_arch_check(ot_vi_pipe vi_pipe, const ot_isp_fpn_attr *correction)
{
    return TD_SUCCESS;
}
#endif
