/*
  Copyright (c), 2001-2025, Shenshu Tech. Co., Ltd.
 */

#include "isp_intf.h"
#include "ot_mpi_sys_mem.h"
#include "ot_common_vi.h"
#include "ot_common_isp.h"
#include "ot_common_3a.h"
#include "ot_common_ae.h"
#include "ot_common_awb.h"
#include "isp_main.h"
#include "isp_ext_reg_access.h"
#include "isp_ext_config.h"
#include "isp_math_utils.h"
#include "ot_math.h"
#include "vi_ext.h"
static td_void isp_calc_grid_info(td_u16 width, td_u16 start_pox, td_u16 block_num, td_u16 *grid_info)
{
    td_u16 i;

    grid_info[0] = start_pox;
    for (i = 1; i < block_num; i++) {
        grid_info[i] = start_pox + (i * width + block_num - 1) / div_0_to_1(block_num);
    }

    return;
}
static td_u16 isp_get_striping_active_img_start(td_u8 block_index, isp_working_mode *isp_work_mode)
{
    td_u32 over_lap;
    td_u32 block_start;

    over_lap = isp_work_mode->over_lap;
    if (block_index == 0) {
        block_start = (td_u32)isp_work_mode->block_rect[block_index].x;
    } else {
        block_start = (td_u32)isp_work_mode->block_rect[block_index].x + over_lap;
    }

    return (td_u16)block_start;
}

static td_u16 isp_get_striping_active_img_width(td_u8 block_index, isp_working_mode *isp_work_mode)
{
    td_u32 block_width;
    td_u32 over_lap;
    td_u8  block_num;

    over_lap    = isp_work_mode->over_lap;
    block_width = isp_work_mode->block_rect[block_index].width;
    block_num   = isp_work_mode->block_num;

    if ((block_index == 0) || (block_index == (block_num - 1))) { /* first block and last block */
        block_width = block_width - over_lap;
    } else {
        block_width = block_width - over_lap * 2; /*  overlap * 2 */
    }
    return (td_u16)block_width;
}

static td_u32 isp_get_striping_grid_x_info(td_u16 *grid_pos, td_u16 grid_num, isp_working_mode *isp_work_mode)
{
    td_u8  i;
    td_u16 start;
    td_u16 width;
    td_u16 div_num;
    td_u16 index = 0;

    for (i = 0; i < isp_work_mode->block_num; i++) {
        start = isp_get_striping_active_img_start(i, isp_work_mode);
        width = isp_get_striping_active_img_width(i, isp_work_mode);

        if (i < grid_num % div_0_to_1(isp_work_mode->block_num)) {
            div_num = grid_num / div_0_to_1(isp_work_mode->block_num) + 1;
        } else {
            div_num = grid_num / div_0_to_1(isp_work_mode->block_num);
        }

        isp_calc_grid_info(width, start, div_num, &(grid_pos[index]));
        index = index + div_num;
    }
    return TD_SUCCESS;
}

static td_u32 ae_get_striping_grid_x_info(ot_vi_pipe vi_pipe, td_u16 *grid_pos, isp_working_mode *isp_work_mode)
{
    td_u8  i;
    td_u16 start;
    td_u16 width;
    td_u16 div_num;
    td_u16 index = 0;

    for (i = 0; i < isp_work_mode->block_num; i++) {
        div_num = ot_ext_system_ae_zone_hnum_read(vi_pipe, i);
        start = ot_ext_system_ae_split_x_read(vi_pipe, i);
        width = ot_ext_system_ae_split_width_read(vi_pipe, i);
        isp_calc_grid_info(width, start, div_num, &(grid_pos[index]));
        index = index + div_num;
    }
    return TD_SUCCESS;
}

static td_void isp_calc_ae_fe_grid_info(ot_vi_pipe vi_pipe, ot_isp_ae_grid_info *fe_grid_info)
{
    td_bool crop_en;
    td_u16  img_total_width, img_total_height;
    td_u16  img_start_x, img_start_y;

    crop_en = ot_ext_system_ae_fe_crop_en_read(vi_pipe);
    if (crop_en == TD_TRUE) {
        img_start_x      = ot_ext_system_ae_fe_crop_x_read(vi_pipe);
        img_start_y      = ot_ext_system_ae_fe_crop_y_read(vi_pipe);
        img_total_width  = ot_ext_system_ae_fe_crop_width_read(vi_pipe);
        img_total_height = ot_ext_system_ae_fe_crop_height_read(vi_pipe);
    } else {
        img_start_x      = 0;
        img_start_y      = 0;
        img_total_width  = ot_ext_sync_total_width_read(vi_pipe);
        img_total_height = ot_ext_sync_total_height_read(vi_pipe);
    }

    isp_calc_grid_info(img_total_width,  img_start_x, OT_ISP_AE_ZONE_COLUMN, fe_grid_info->grid_x_pos);
    isp_calc_grid_info(img_total_height, img_start_y, OT_ISP_AE_ZONE_ROW, fe_grid_info->grid_y_pos);

    fe_grid_info->grid_x_pos[OT_ISP_AE_ZONE_COLUMN] = img_start_x + img_total_width - 1;
    fe_grid_info->grid_y_pos[OT_ISP_AE_ZONE_ROW]    = img_start_y + img_total_height - 1;
    fe_grid_info->status = 1;
}

static td_s32 isp_calc_ae_be_grid_info(ot_vi_pipe vi_pipe, ot_isp_be_ae_grid_info *be_grid_info)
{
    td_bool crop_en;
    td_u8 be_zone_hnum, be_zone_vnum;
    td_u16 be_width, be_height;
    td_u16 be_start_x = 0;
    td_u16 be_start_y;
    isp_working_mode isp_work_mode;

    if (ioctl(isp_get_fd(vi_pipe), ISP_WORK_MODE_GET, &isp_work_mode) != TD_SUCCESS) {
        isp_err_trace("get work mode error!\n");
        return TD_FAILURE;
    }

    crop_en = ot_ext_system_ae_crop_en_read(vi_pipe);
    if (is_striping_mode(isp_work_mode.running_mode)) {
        ae_get_striping_grid_x_info(vi_pipe, be_grid_info->grid_x_pos, &isp_work_mode);
        be_start_y = (td_u16)isp_work_mode.block_rect[0].y;
        be_height = (td_u16)isp_work_mode.frame_rect.height;
        be_zone_vnum = ot_ext_system_ae_zone_vnum_read(vi_pipe, 0);
        isp_calc_grid_info(be_height, be_start_y, be_zone_vnum, be_grid_info->grid_y_pos);
        be_width = (td_u16)isp_work_mode.frame_rect.width;
        be_grid_info->grid_x_pos[OT_ISP_BE_AE_ZONE_COLUMN] = be_start_x + be_width - 1;  /* last position */
        be_grid_info->grid_y_pos[be_zone_vnum] = be_start_y + be_height - 1;  /* last position */
    } else {
        if (crop_en == TD_TRUE) {
            be_start_x = ot_ext_system_ae_crop_x_read(vi_pipe);
            be_start_y = ot_ext_system_ae_crop_y_read(vi_pipe);
            be_width = ot_ext_system_ae_crop_width_read(vi_pipe);
            be_height = ot_ext_system_ae_crop_height_read(vi_pipe);
        } else {
            be_start_x = 0;
            be_start_y = 0;
            be_width = (td_u16)isp_work_mode.frame_rect.width;
            be_height = (td_u16)isp_work_mode.frame_rect.height;
        }

        be_zone_hnum = ot_ext_system_ae_zone_hnum_read(vi_pipe, 0);
        be_zone_vnum = ot_ext_system_ae_zone_vnum_read(vi_pipe, 0);

        isp_calc_grid_info(be_width, be_start_x, be_zone_hnum, be_grid_info->grid_x_pos);
        isp_calc_grid_info(be_height, be_start_y, be_zone_vnum, be_grid_info->grid_y_pos);

        be_grid_info->grid_x_pos[be_zone_hnum] = be_start_x + be_width - 1;  /* last position */
        be_grid_info->grid_y_pos[be_zone_vnum] = be_start_y + be_height - 1;  /* last position */
    }
    be_grid_info->status = 1;

    return TD_SUCCESS;
}

td_s32 isp_get_ae_grid_info(ot_vi_pipe vi_pipe, ot_isp_ae_grid_info *fe_grid_info,
    ot_isp_be_ae_grid_info *be_grid_info)
{
    td_s32 ret;
    (td_void)memset_s(fe_grid_info, sizeof(ot_isp_ae_grid_info), 0, sizeof(ot_isp_ae_grid_info));
    (td_void)memset_s(be_grid_info, sizeof(ot_isp_be_ae_grid_info), 0, sizeof(ot_isp_be_ae_grid_info));

    isp_calc_ae_fe_grid_info(vi_pipe, fe_grid_info);

    ret = isp_calc_ae_be_grid_info(vi_pipe, be_grid_info);

    return ret;
}

static td_void isp_get_af_fe_grid_info(ot_vi_pipe vi_pipe, ot_isp_focus_grid_info *fe_grid_info)
{
    td_bool crop_en;
    td_u16 img_total_height;
    td_u16 img_total_width;
    td_u16 img_start_y;
    td_u16 img_start_x;
    td_u16 af_x_grid_num, af_y_grid_num;

    crop_en = ot_ext_af_fe_crop_enable_read(vi_pipe);
    af_y_grid_num = ot_ext_af_window_vnum_read(vi_pipe);
    af_x_grid_num = ot_ext_af_window_hnum_read(vi_pipe);

    if (crop_en == TD_TRUE) {
        img_start_x = ot_ext_af_fe_crop_pos_x_read(vi_pipe);
        img_start_y = ot_ext_af_fe_crop_pos_y_read(vi_pipe);
        img_total_width  = ot_ext_af_fe_crop_h_size_read(vi_pipe);
        img_total_height = ot_ext_af_fe_crop_v_size_read(vi_pipe);
    } else {
        img_start_x = 0;
        img_start_y = 0;
        img_total_width = ot_ext_sync_total_width_read(vi_pipe);
        img_total_height = ot_ext_sync_total_height_read(vi_pipe);
    }

    isp_calc_grid_info(img_total_width, img_start_x, af_x_grid_num, fe_grid_info->grid_x_pos);
    isp_calc_grid_info(img_total_height, img_start_y, af_y_grid_num, fe_grid_info->grid_y_pos);

    fe_grid_info->grid_x_pos[af_x_grid_num] = img_start_x + img_total_width - 1;
    fe_grid_info->grid_y_pos[af_y_grid_num] = img_start_y + img_total_height - 1;
    fe_grid_info->status = 1;
}

static td_s32 isp_get_af_be_grid_info(ot_vi_pipe vi_pipe, ot_isp_focus_grid_info *be_grid_info)
{
    td_bool crop_en;
    td_u16  be_width, be_height;
    td_u16  be_start_x = 0;
    td_u16  be_start_y;
    td_u16  af_x_grid_num, af_y_grid_num;
    isp_working_mode isp_work_mode;

    if (ioctl(isp_get_fd(vi_pipe), ISP_WORK_MODE_GET, &isp_work_mode) != TD_SUCCESS) {
        isp_err_trace("get work mode error!\n");
        return TD_FAILURE;
    }

    crop_en = ot_ext_af_crop_enable_read(vi_pipe);
    af_y_grid_num = ot_ext_af_window_vnum_read(vi_pipe);
    af_x_grid_num = ot_ext_af_window_hnum_read(vi_pipe);

    if (is_striping_mode(isp_work_mode.running_mode)) {
        isp_get_striping_grid_x_info(be_grid_info->grid_x_pos, af_x_grid_num, &isp_work_mode);
        be_start_y = (td_u16)isp_work_mode.block_rect[0].y;
        be_height = (td_u16)isp_work_mode.frame_rect.height;
        isp_calc_grid_info(be_height, be_start_y, af_y_grid_num, be_grid_info->grid_y_pos);
        be_width  = (td_u16)isp_work_mode.frame_rect.width;
    } else {
        if (crop_en == TD_TRUE) {
            be_start_x = ot_ext_af_crop_pos_x_read(vi_pipe);
            be_start_y = ot_ext_af_crop_pos_y_read(vi_pipe);
            be_width  = ot_ext_af_crop_h_size_read(vi_pipe);
            be_height = ot_ext_af_crop_v_size_read(vi_pipe);
        } else {
            be_start_x = 0;
            be_start_y = 0;
            be_width  = (td_u16)isp_work_mode.frame_rect.width;
            be_height = (td_u16)isp_work_mode.frame_rect.height;
        }

        isp_calc_grid_info(be_width,  be_start_x, af_x_grid_num, be_grid_info->grid_x_pos);
        isp_calc_grid_info(be_height, be_start_y, af_y_grid_num,    be_grid_info->grid_y_pos);
    }

    be_grid_info->grid_x_pos[af_x_grid_num] = be_start_x + be_width  - 1; /* last position */
    be_grid_info->grid_y_pos[af_y_grid_num] = be_start_y + be_height - 1; /* last position */
    be_grid_info->status = 1;

    return TD_SUCCESS;
}

td_s32 isp_get_af_grid_info(ot_vi_pipe vi_pipe, ot_isp_focus_grid_info *fe_grid_info,
                            ot_isp_focus_grid_info *be_grid_info)
{
    (td_void)memset_s(fe_grid_info, sizeof(ot_isp_focus_grid_info), 0, sizeof(ot_isp_focus_grid_info));
    (td_void)memset_s(be_grid_info, sizeof(ot_isp_focus_grid_info), 0, sizeof(ot_isp_focus_grid_info));

    isp_get_af_fe_grid_info(vi_pipe, fe_grid_info);
    return isp_get_af_be_grid_info(vi_pipe, be_grid_info);
}

td_s32 isp_get_wb_grid_info(ot_vi_pipe vi_pipe, ot_isp_awb_grid_info *grid_info)
{
    td_bool crop_en;
    td_u16  be_width, be_height;
    td_u16  be_start_x = 0;
    td_u16  be_start_y;
    td_u16  u16awb_x_grid_num, u16awb_y_grid_num;
    isp_working_mode isp_work_mode;

    (td_void)memset_s(grid_info, sizeof(ot_isp_awb_grid_info), 0, sizeof(ot_isp_awb_grid_info));

    if (ioctl(isp_get_fd(vi_pipe), ISP_WORK_MODE_GET, &isp_work_mode) != TD_SUCCESS) {
        isp_err_trace("get work mode error!\n");
        return TD_FAILURE;
    }

    u16awb_y_grid_num = ot_ext_system_awb_vnum_read(vi_pipe);
    u16awb_x_grid_num = ot_ext_system_awb_hnum_read(vi_pipe);
    crop_en           = ot_ext_system_awb_crop_en_read(vi_pipe);

    if (is_striping_mode(isp_work_mode.running_mode)) {
        isp_get_striping_grid_x_info(grid_info->grid_x_pos, u16awb_x_grid_num, &isp_work_mode);
        be_start_y = (td_u16)isp_work_mode.block_rect[0].y;
        be_height = (td_u16)isp_work_mode.frame_rect.height;
        isp_calc_grid_info(be_height, be_start_y, u16awb_y_grid_num, grid_info->grid_y_pos);
        be_width    = (td_u16)isp_work_mode.frame_rect.width;
    } else {
        if (crop_en == TD_TRUE) {
            be_start_x = ot_ext_system_awb_crop_x_read(vi_pipe);
            be_start_y = ot_ext_system_awb_crop_y_read(vi_pipe);
            be_width   = ot_ext_system_awb_crop_width_read(vi_pipe);
            be_height  = ot_ext_system_awb_crop_height_read(vi_pipe);
        } else {
            be_start_x = 0;
            be_start_y = 0;
            be_width   = (td_u16)isp_work_mode.frame_rect.width;
            be_height  = (td_u16)isp_work_mode.frame_rect.height;
        }

        isp_calc_grid_info(be_width, be_start_x, u16awb_x_grid_num, grid_info->grid_x_pos);
        isp_calc_grid_info(be_height, be_start_y, u16awb_y_grid_num, grid_info->grid_y_pos);
    }

    grid_info->grid_x_pos[u16awb_x_grid_num] = be_start_x + be_width  - 1; /* last position */
    grid_info->grid_y_pos[u16awb_y_grid_num] = be_start_y + be_height - 1; /* last position */
    grid_info->status                     = 1;

    return TD_SUCCESS;
}

td_s32 isp_get_fe_focus_stats(ot_vi_pipe vi_pipe, ot_isp_fe_focus_stats *fe_af_stat,
                              isp_stat *isp_act_stat, td_u8 wdr_chn)
{
#ifdef CONFIG_OT_ISP_FE_AF_STAT_SUPPORT
    td_u8 i, j;
    td_u8 col, row;
    td_u8 k = 0;

    col = clip_max(ot_ext_af_window_hnum_read(vi_pipe), OT_ISP_AF_ZONE_COLUMN);
    row = clip_max(ot_ext_af_window_vnum_read(vi_pipe), OT_ISP_AF_ZONE_ROW);

    for (; k < wdr_chn; k++) {
        for (i = 0; i < row; i++) {
            for (j = 0; j < col; j++) {
                fe_af_stat->zone_metrics[k][i][j].v1 = isp_act_stat->fe_af_stat.zone_metrics[k][i][j].v1;
                fe_af_stat->zone_metrics[k][i][j].h1 = isp_act_stat->fe_af_stat.zone_metrics[k][i][j].h1;
                fe_af_stat->zone_metrics[k][i][j].v2 = isp_act_stat->fe_af_stat.zone_metrics[k][i][j].v2;
                fe_af_stat->zone_metrics[k][i][j].h2 = isp_act_stat->fe_af_stat.zone_metrics[k][i][j].h2;
                fe_af_stat->zone_metrics[k][i][j].y  = isp_act_stat->fe_af_stat.zone_metrics[k][i][j].y;
                fe_af_stat->zone_metrics[k][i][j].hl_cnt = isp_act_stat->fe_af_stat.zone_metrics[k][i][j].hl_cnt;
            }
        }
    }

    for (; k < OT_ISP_WDR_MAX_FRAME_NUM; k++) {
        for (i = 0; i < row; i++) {
            for (j = 0; j < col; j++) {
                fe_af_stat->zone_metrics[k][i][j].v1 = 0;
                fe_af_stat->zone_metrics[k][i][j].h1 = 0;
                fe_af_stat->zone_metrics[k][i][j].v2 = 0;
                fe_af_stat->zone_metrics[k][i][j].h2 = 0;
                fe_af_stat->zone_metrics[k][i][j].y = 0;
                fe_af_stat->zone_metrics[k][i][j].hl_cnt = 0;
            }
        }
    }
#endif
    return TD_SUCCESS;
}

td_s32 isp_set_pipe_differ_attr(ot_vi_pipe vi_pipe, const ot_isp_pipe_diff_attr *pipe_differ)
{
    isp_err_trace("not support pipe differ\n");
    return OT_ERR_ISP_NOT_SUPPORT;
}

td_s32 isp_get_pipe_differ_attr(ot_vi_pipe vi_pipe, ot_isp_pipe_diff_attr *pipe_differ)
{
    isp_err_trace("not support pipe differ\n");
    return OT_ERR_ISP_NOT_SUPPORT;
}

td_s32 isp_get_check_sum_stats(ot_vi_pipe vi_pipe, isp_check_sum_stat *sum_stat)
{
    td_s32 ret;
    isp_stat_info stat_info;
    isp_stat *isp_act_stat = TD_NULL;

    if (ioctl(isp_get_fd(vi_pipe), ISP_STAT_ACT_GET, &stat_info) != TD_SUCCESS) {
        isp_err_trace("ISP[%d] get active stat buffer failed\n", vi_pipe);
        return OT_ERR_ISP_NOMEM;
    }

    stat_info.virt_addr = ot_mpi_sys_mmap_cached(stat_info.phy_addr, sizeof(isp_stat));
    if (stat_info.virt_addr == TD_NULL) {
        isp_err_trace("mmap stat_info.phy_addr failed!\n");
        return OT_ERR_ISP_NULL_PTR;
    }
    isp_act_stat = (isp_stat *)stat_info.virt_addr;

    ret = memcpy_s(sum_stat->sum_y, sizeof(sum_stat->sum_y), isp_act_stat->sum_stat.sum_y,
        sizeof(isp_act_stat->sum_stat.sum_y));
    if ((ret) != EOK) {
        isp_err_trace("memcpy_s failed!\n");
        ot_mpi_sys_munmap((td_void *)isp_act_stat, sizeof(isp_stat));
        return TD_FAILURE;
    }

    sum_stat->pts = isp_act_stat->frame_pts;
    ot_mpi_sys_munmap((td_void *)isp_act_stat, sizeof(isp_stat));

    return TD_SUCCESS;
}

td_s32 isp_ae_stats_weight_config_check(const ot_isp_ae_stats_cfg *ae_cfg)
{
    td_s32 i, j;
    for (i = 0; i < OT_ISP_AE_ZONE_ROW; i++) {
        for (j = 0; j < OT_ISP_AE_ZONE_COLUMN; j++) {
            if (ae_cfg->weight[i][j] > 0xF) {
                isp_err_trace("weight[%d][%d] should not be larger than 0xF!\n", i, j);
                return OT_ERR_ISP_ILLEGAL_PARAM;
            }
        }
    }

    for (i = 0; i < OT_ISP_BE_AE_ZONE_ROW; i++) {
        for (j = 0; j < OT_ISP_BE_AE_ZONE_COLUMN; j++) {
            if (ae_cfg->be_weight[i][j] > 0xF) {
                isp_err_trace("weight[%d][%d] should not be larger than 0xF!\n", i, j);
                return OT_ERR_ISP_ILLEGAL_PARAM;
            }
        }
    }
    return TD_SUCCESS;
}

td_void isp_drc_arch_attr_write(ot_vi_pipe vi_pipe, const ot_isp_drc_attr *drc_attr)
{
    td_u8 i;
    ot_ext_system_drc_local_mixing_bright_max_write(vi_pipe, drc_attr->local_mixing_bright_param.max);
    ot_ext_system_drc_local_mixing_bright_min_write(vi_pipe, drc_attr->local_mixing_bright_param.min);
    ot_ext_system_drc_local_mixing_bright_threshold_write(vi_pipe, drc_attr->local_mixing_bright_param.threshold);
    ot_ext_system_drc_local_mixing_bright_slope_write(vi_pipe, (td_u8)drc_attr->local_mixing_bright_param.slope);

    ot_ext_system_drc_local_mixing_dark_max_write(vi_pipe, drc_attr->local_mixing_dark_param.max);
    ot_ext_system_drc_local_mixing_dark_min_write(vi_pipe, drc_attr->local_mixing_dark_param.min);
    ot_ext_system_drc_local_mixing_dark_threshold_write(vi_pipe, drc_attr->local_mixing_dark_param.threshold);
    ot_ext_system_drc_local_mixing_dark_slope_write(vi_pipe, (td_u8)drc_attr->local_mixing_dark_param.slope);

    ot_ext_system_drc_auto_curve_brightness_write(vi_pipe, drc_attr->auto_curve.brightness);
    ot_ext_system_drc_auto_curve_contrast_write(vi_pipe, drc_attr->auto_curve.contrast);
    ot_ext_system_drc_auto_curve_tolerance_write(vi_pipe, drc_attr->auto_curve.tolerance);

    for (i = 0; i < OT_ISP_DRC_BCNR_NODE_NUM; i++) {
        ot_ext_system_drc_bcnr_lut_write(vi_pipe, i, drc_attr->bcnr_attr.detail_restore_lut[i]);
    }

    ot_ext_system_drc_bcnr_en_write(vi_pipe, drc_attr->bcnr_attr.enable);
    ot_ext_system_drc_bcnr_edge_strength_write(vi_pipe, drc_attr->bcnr_attr.strength);
}

td_void isp_drc_arch_attr_read(ot_vi_pipe vi_pipe, ot_isp_drc_attr *drc_attr)
{
    td_u8 i;
    drc_attr->local_mixing_bright_param.max = ot_ext_system_drc_local_mixing_bright_max_read(vi_pipe);
    drc_attr->local_mixing_bright_param.min = ot_ext_system_drc_local_mixing_bright_min_read(vi_pipe);
    drc_attr->local_mixing_bright_param.threshold = ot_ext_system_drc_local_mixing_bright_threshold_read(vi_pipe);
    drc_attr->local_mixing_bright_param.slope = (td_s8)ot_ext_system_drc_local_mixing_bright_slope_read(vi_pipe);

    drc_attr->local_mixing_dark_param.max = ot_ext_system_drc_local_mixing_dark_max_read(vi_pipe);
    drc_attr->local_mixing_dark_param.min = ot_ext_system_drc_local_mixing_dark_min_read(vi_pipe);
    drc_attr->local_mixing_dark_param.threshold = ot_ext_system_drc_local_mixing_dark_threshold_read(vi_pipe);
    drc_attr->local_mixing_dark_param.slope = (td_s8)ot_ext_system_drc_local_mixing_dark_slope_read(vi_pipe);

    drc_attr->auto_curve.brightness = ot_ext_system_drc_auto_curve_brightness_read(vi_pipe);
    drc_attr->auto_curve.contrast   = ot_ext_system_drc_auto_curve_contrast_read(vi_pipe);
    drc_attr->auto_curve.tolerance  = ot_ext_system_drc_auto_curve_tolerance_read(vi_pipe);

    for (i = 0; i < OT_ISP_DRC_BCNR_NODE_NUM; i++) {
        drc_attr->bcnr_attr.detail_restore_lut[i] = ot_ext_system_drc_bcnr_lut_read(vi_pipe, i);
    }
    drc_attr->bcnr_attr.enable = ot_ext_system_drc_bcnr_en_read(vi_pipe);
    drc_attr->bcnr_attr.strength = ot_ext_system_drc_bcnr_edge_strength_read(vi_pipe);
}

td_void isp_nr_comm_intf_attr_write(ot_vi_pipe vi_pipe, const ot_isp_nr_attr *nr_attr)
{
    ot_ext_system_bayernr_tnr_enable_write(vi_pipe, nr_attr->md_en);
}

td_void isp_nr_arch_intf_attr_write(ot_vi_pipe vi_pipe, const ot_isp_nr_attr *nr_attr)
{
    td_u8 i;
    const ot_isp_nr_md_auto_attr *md_auto = &nr_attr->md_cfg.md_auto;
    for (i = 0; i < OT_ISP_AUTO_ISO_NUM; i++) {
        ot_ext_system_bayernr_auto_md_mode_lut_write(vi_pipe, i, md_auto->md_mode[i]);
        ot_ext_system_bayernr_auto_md_size_ratio_lut_write(vi_pipe, i, md_auto->md_size_ratio[i]);
        ot_ext_system_bayernr_auto_md_anti_flicker_strength_lut_write(vi_pipe, i, md_auto->md_anti_flicker_strength[i]);
        ot_ext_system_bayernr_auto_md_static_ratio_lut_write(vi_pipe, i, md_auto->md_static_ratio[i]);
        ot_ext_system_bayernr_auto_md_motion_ratio_lut_write(vi_pipe, i, md_auto->md_motion_ratio[i]);
        ot_ext_system_bayernr_auto_md_static_fine_strength_lut_write(vi_pipe, i, md_auto->md_static_fine_strength[i]);
        ot_ext_system_bayernr_auto_tfs_lut_write(vi_pipe, i, md_auto->tfs[i]);
        ot_ext_system_bayernr_auto_user_define_md_lut_write(vi_pipe, i, md_auto->user_define_md[i]);
        ot_ext_system_bayernr_auto_user_define_bri_thresh_lut_write(vi_pipe, i, md_auto->user_define_slope[i]);
        ot_ext_system_bayernr_auto_user_define_dark_thresh_lut_write(vi_pipe, i, md_auto->user_define_dark_thresh[i]);
        ot_ext_system_bayernr_auto_user_define_color_thresh_lut_write(vi_pipe, i, md_auto->user_define_color_thresh[i]);
        ot_ext_system_bayernr_auto_sfr_r_lut_write(vi_pipe, i, md_auto->sfr_r[i]);
        ot_ext_system_bayernr_auto_sfr_g_lut_write(vi_pipe, i, md_auto->sfr_g[i]);
        ot_ext_system_bayernr_auto_sfr_b_lut_write(vi_pipe, i, md_auto->sfr_b[i]);
    }

    const ot_isp_nr_md_manual_attr *md_manual = &nr_attr->md_cfg.md_manual;
    ot_ext_system_bayernr_manual_md_mode_write(vi_pipe, md_manual->md_mode);
    ot_ext_system_bayernr_manual_md_size_ratio_write(vi_pipe, md_manual->md_size_ratio);
    ot_ext_system_bayernr_manual_md_anti_flicker_strength_write(vi_pipe, md_manual->md_anti_flicker_strength);
    ot_ext_system_bayernr_manual_md_static_ratio_write(vi_pipe, md_manual->md_static_ratio);
    ot_ext_system_bayernr_manual_md_motion_ratio_write(vi_pipe, md_manual->md_motion_ratio);
    ot_ext_system_bayernr_manual_md_static_fine_strength_write(vi_pipe, md_manual->md_static_fine_strength);
    ot_ext_system_bayernr_manual_tfs_write(vi_pipe, md_manual->tfs);
    ot_ext_system_bayernr_manual_user_define_md_write(vi_pipe, md_manual->user_define_md);
    ot_ext_system_bayernr_manual_user_define_bri_thresh_write(vi_pipe, md_manual->user_define_slope);
    ot_ext_system_bayernr_manual_user_define_dark_thresh_write(vi_pipe, md_manual->user_define_dark_thresh);
    ot_ext_system_bayernr_manual_user_define_color_thresh_write(vi_pipe, md_manual->user_define_color_thresh);
    ot_ext_system_bayernr_manual_sfr_r_write(vi_pipe, md_manual->sfr_r);
    ot_ext_system_bayernr_manual_sfr_g_write(vi_pipe, md_manual->sfr_g);
    ot_ext_system_bayernr_manual_sfr_b_write(vi_pipe, md_manual->sfr_b);
}

td_void isp_nr_comm_intf_attr_read(ot_vi_pipe vi_pipe, ot_isp_nr_attr *nr_attr)
{
    nr_attr->md_en     = ot_ext_system_bayernr_tnr_enable_read(vi_pipe);
}

td_void isp_nr_arch_intf_attr_read(ot_vi_pipe vi_pipe, ot_isp_nr_attr *nr_attr)
{
    td_u8 i;
    ot_isp_nr_md_auto_attr *md_auto = &nr_attr->md_cfg.md_auto;
    for (i = 0; i < OT_ISP_AUTO_ISO_NUM; i++) {
        md_auto->md_mode[i] = ot_ext_system_bayernr_auto_md_mode_lut_read(vi_pipe, i);
        md_auto->md_size_ratio[i] = ot_ext_system_bayernr_auto_md_size_ratio_lut_read(vi_pipe, i);
        md_auto->md_anti_flicker_strength[i] = ot_ext_system_bayernr_auto_md_anti_flicker_strength_lut_read(vi_pipe, i);
        md_auto->md_static_ratio[i] = ot_ext_system_bayernr_auto_md_static_ratio_lut_read(vi_pipe, i);
        md_auto->md_motion_ratio[i] = ot_ext_system_bayernr_auto_md_motion_ratio_lut_read(vi_pipe, i);
        md_auto->md_static_fine_strength[i] = ot_ext_system_bayernr_auto_md_static_fine_strength_lut_read(vi_pipe, i);
        md_auto->tfs[i] = ot_ext_system_bayernr_auto_tfs_lut_read(vi_pipe, i);
        md_auto->user_define_md[i]   = ot_ext_system_bayernr_auto_user_define_md_lut_read(vi_pipe, i);
        md_auto->user_define_slope[i]   = ot_ext_system_bayernr_auto_user_define_bri_thresh_lut_read(vi_pipe, i);
        md_auto->user_define_dark_thresh[i]  = ot_ext_system_bayernr_auto_user_define_dark_thresh_lut_read(vi_pipe, i);
        md_auto->user_define_color_thresh[i] = ot_ext_system_bayernr_auto_user_define_color_thresh_lut_read(vi_pipe, i);
        md_auto->sfr_r[i]   = ot_ext_system_bayernr_auto_sfr_r_lut_read(vi_pipe, i);
        md_auto->sfr_g[i]   = ot_ext_system_bayernr_auto_sfr_g_lut_read(vi_pipe, i);
        md_auto->sfr_b[i]   = ot_ext_system_bayernr_auto_sfr_b_lut_read(vi_pipe, i);
    }

    ot_isp_nr_md_manual_attr *md_manual = &nr_attr->md_cfg.md_manual;
    md_manual->md_mode = ot_ext_system_bayernr_manual_md_mode_read(vi_pipe);
    md_manual->md_size_ratio = ot_ext_system_bayernr_manual_md_size_ratio_read(vi_pipe);
    md_manual->md_anti_flicker_strength = ot_ext_system_bayernr_manual_md_anti_flicker_strength_read(vi_pipe);
    md_manual->md_static_ratio = ot_ext_system_bayernr_manual_md_static_ratio_read(vi_pipe);
    md_manual->md_motion_ratio = ot_ext_system_bayernr_manual_md_motion_ratio_read(vi_pipe);
    md_manual->md_static_fine_strength = ot_ext_system_bayernr_manual_md_static_fine_strength_read(vi_pipe);
    md_manual->tfs = ot_ext_system_bayernr_manual_tfs_read(vi_pipe);
    md_manual->user_define_md           = ot_ext_system_bayernr_manual_user_define_md_read(vi_pipe);
    md_manual->user_define_slope = ot_ext_system_bayernr_manual_user_define_bri_thresh_read(vi_pipe);
    md_manual->user_define_dark_thresh  = ot_ext_system_bayernr_manual_user_define_dark_thresh_read(vi_pipe);
    md_manual->user_define_color_thresh = ot_ext_system_bayernr_manual_user_define_color_thresh_read(vi_pipe);
    md_manual->sfr_r   = ot_ext_system_bayernr_manual_sfr_r_read(vi_pipe);
    md_manual->sfr_g   = ot_ext_system_bayernr_manual_sfr_g_read(vi_pipe);
    md_manual->sfr_b   = ot_ext_system_bayernr_manual_sfr_b_read(vi_pipe);
}

td_void isp_demosaic_arch_attr_write(ot_vi_pipe vi_pipe, const ot_isp_demosaic_attr *dm_attr)
{
    td_u8 i;
    for (i = 0; i < OT_ISP_AUTO_ISO_NUM; i++) {
        ot_ext_system_demosaic_auto_hfehnc_str_write(vi_pipe, i, dm_attr->auto_attr.hf_detail_strength[i]);
    }
    ot_ext_system_demosaic_manual_hfehnc_str_write(vi_pipe, dm_attr->manual_attr.hf_detail_strength);
}

td_void isp_demosaic_arch_attr_read(ot_vi_pipe vi_pipe, ot_isp_demosaic_attr *dm_attr)
{
    td_u8 i;
    for (i = 0; i < OT_ISP_AUTO_ISO_NUM; i++) {
        dm_attr->auto_attr.hf_detail_strength[i] = ot_ext_system_demosaic_auto_hfehnc_str_read(vi_pipe, i);
    }
    dm_attr->manual_attr.hf_detail_strength = ot_ext_system_demosaic_manual_hfehnc_str_read(vi_pipe);
}

td_s32 isp_expander_attr_update(ot_vi_pipe vi_pipe, const ot_isp_expander_attr *expander_attr)
{
    td_s32 isp_fd;
    td_s32 ret;
    isp_working_mode work_mode;

    isp_fd = isp_get_fd(vi_pipe);
    ret = ioctl(isp_fd, ISP_WORK_MODE_GET, &work_mode);
    if (ret) {
        isp_err_trace("get work mode error!\n");
        return ret;
    }
    if (is_offline_mode(work_mode.running_mode) || is_striping_mode(work_mode.running_mode)) {
        isp_expander_attr_write(vi_pipe, expander_attr);
    } else {
        ot_ext_system_expander_en_write(vi_pipe, expander_attr->enable);
    }

    ot_ext_system_expander_param_update_write(vi_pipe, TD_TRUE);
    ot_ext_system_expander_blc_param_update_write(vi_pipe, TD_TRUE);

    return TD_SUCCESS;
}

td_s32 isp_set_noise_calibration(ot_vi_pipe vi_pipe, const ot_isp_noise_calibration *noise_calibration)
{
    td_u32 i;
    isp_check_open_return(vi_pipe);
    isp_check_mem_init_return(vi_pipe);
    isp_check_vreg_permission_return(vi_pipe);

    for (i = 0; i < OT_BAYER_CALIBRATION_PARA_NUM_NEW; i++) {
        ot_ext_system_sns_noise_calibration_lut_write(vi_pipe, i, noise_calibration->calibration_coef[i]);
    }

    return TD_SUCCESS;
}

td_s32 isp_set_algs_update(ot_vi_pipe vi_pipe)
{
    ot_ext_system_tunnel_update_write(vi_pipe, TD_TRUE);
    ot_ext_system_smart_update_write(vi_pipe, TD_TRUE);
    ot_ext_af_set_flag_write(vi_pipe, OT_EXT_AF_SET_FLAG_ENABLE);
    ot_ext_system_wb_statistics_mpi_update_en_write(vi_pipe, TD_TRUE);
    ot_ext_system_black_level_change_write(vi_pipe, TD_TRUE);
    ot_ext_system_ca_coef_update_en_write(vi_pipe, TD_TRUE);
    ot_ext_system_cac_coef_update_write(vi_pipe, TD_TRUE);
    ot_ext_system_csc_attr_update_write(vi_pipe, TD_TRUE);
    ot_ext_system_dpc_static_attr_update_write(vi_pipe, TD_TRUE);
    ot_ext_system_dpc_dynamic_attr_update_write(vi_pipe, TD_TRUE);
    ot_ext_system_demosaic_attr_update_en_write(vi_pipe, TD_TRUE);
    ot_ext_system_wdr_coef_update_en_write(vi_pipe, TD_TRUE);
    ot_ext_system_gamma_lut_update_write(vi_pipe, TD_TRUE);
    ot_ext_system_ge_coef_update_en_write(vi_pipe, TD_TRUE);
    ot_ext_system_isp_mesh_shading_attr_updata_write(vi_pipe, TD_TRUE);
    ot_ext_system_isp_mesh_shading_lut_attr_updata_write(vi_pipe, TD_TRUE);
    ot_ext_system_sharpen_mpi_update_en_write(vi_pipe, TD_TRUE);
    ot_ext_system_user_dehaze_lut_update_write(vi_pipe, TD_TRUE);
    ot_ext_system_drc_param_updated_write(vi_pipe, TD_TRUE);
    ot_ext_system_expander_param_update_write(vi_pipe, TD_TRUE);
    ot_ext_system_isp_acs_attr_updata_write(vi_pipe, TD_TRUE);
    ot_ext_system_bayernr_attr_update_write(vi_pipe, TD_TRUE);
    return TD_SUCCESS;
}

td_s32 isp_set_motion_stats_config(ot_vi_pipe vi_pipe, const ot_isp_motion_stats_cfg *motion_cfg)
{
    return mpi_vi_set_motion_stats_cfg(vi_pipe, motion_cfg);
}

td_s32 isp_get_motion_stats_config(ot_vi_pipe vi_pipe, ot_isp_motion_stats_cfg *motion_cfg)
{
    return mpi_vi_get_motion_stats_cfg(vi_pipe, motion_cfg);
}

td_s32 isp_get_motion_stats(ot_vi_pipe vi_pipe, ot_isp_motion_stats *motion_stats)
{
    td_bool bnr_en, md_en;
    isp_check_open_return(vi_pipe);
    isp_check_mem_init_return(vi_pipe);
    isp_check_vreg_permission_return(vi_pipe);

    bnr_en = ot_ext_system_bayernr_enable_read(vi_pipe);
    md_en = ot_ext_system_bayernr_tnr_enable_read(vi_pipe);
    if ((md_en && bnr_en) == TD_FALSE) {
        isp_err_trace("ISP[%d] get motion stats not support for md disable(bnr:%d, md:%d)\n", vi_pipe, bnr_en, md_en);
        return OT_ERR_ISP_NOT_SUPPORT;
    }

    return TD_SUCCESS;
}
