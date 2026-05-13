/*
  Copyright (c), 2001-2025, Shenshu Tech. Co., Ltd.
 */

#include "ot_mpi_isp.h"
#include "isp_intf.h"
#include "isp_main.h"
#include "isp_debug.h"
#include "isp_param_check.h"
#include "isp_ext_config.h"
#include "mpi_isp.h"

#define isp_not_support_interface_return(vi_pipe,  val)                \
    do {                                                               \
        ot_unused(vi_pipe);                                            \
        ot_unused(val);                                                \
        isp_err_trace_not_support_interface();   \
        return OT_ERR_ISP_NOT_SUPPORT;                                 \
    } while (0)

static td_void isp_err_trace_not_support_interface(td_void)
{
    isp_err_trace("not support this interface\n");
}

td_s32 ot_mpi_isp_set_pub_attr(ot_vi_pipe vi_pipe, const ot_isp_pub_attr *pub_attr)
{
    td_s32  ret;
    mpi_isp_check_pipe_return(vi_pipe);
    mpi_isp_check_pointer_return(pub_attr);
    isp_mutex_lock(vi_pipe);
    ret = mpi_isp_set_pub_attr(vi_pipe, pub_attr);
    isp_mutex_unlock(vi_pipe);
    return ret;
}

td_s32 ot_mpi_isp_get_pub_attr(ot_vi_pipe vi_pipe, ot_isp_pub_attr *pub_attr)
{
    td_s32  ret;
    mpi_isp_check_pipe_return(vi_pipe);
    mpi_isp_check_pointer_return(pub_attr);
    isp_mutex_lock(vi_pipe);
    ret = mpi_isp_get_pub_attr(vi_pipe, pub_attr);
    isp_mutex_unlock(vi_pipe);
    return ret;
}

td_s32 ot_mpi_isp_set_pipe_differ_attr(ot_vi_pipe vi_pipe, const ot_isp_pipe_diff_attr *pipe_differ)
{
    td_s32  ret;
    mpi_isp_check_pipe_return(vi_pipe);
    mpi_isp_check_pointer_return(pipe_differ);
    isp_mutex_lock(vi_pipe);
    ret = isp_set_pipe_differ_attr(vi_pipe, pipe_differ);
    isp_mutex_unlock(vi_pipe);
    return ret;
}

td_s32 ot_mpi_isp_get_pipe_differ_attr(ot_vi_pipe vi_pipe, ot_isp_pipe_diff_attr *pipe_differ)
{
    td_s32  ret;
    mpi_isp_check_pipe_return(vi_pipe);
    mpi_isp_check_pointer_return(pipe_differ);
    isp_mutex_lock(vi_pipe);
    ret = isp_get_pipe_differ_attr(vi_pipe, pipe_differ);
    isp_mutex_unlock(vi_pipe);
    return ret;
}

td_s32 ot_mpi_isp_set_module_ctrl(ot_vi_pipe vi_pipe, const ot_isp_module_ctrl *mod_ctrl)
{
    td_s32 ret;
    mpi_isp_check_pipe_return(vi_pipe);
    mpi_isp_check_pointer_return(mod_ctrl);
    isp_mutex_lock(vi_pipe);
    ret = mpi_isp_set_module_ctrl(vi_pipe, mod_ctrl);
    isp_mutex_unlock(vi_pipe);
    return ret;
}

td_s32 ot_mpi_isp_get_module_ctrl(ot_vi_pipe vi_pipe, ot_isp_module_ctrl *mod_ctrl)
{
    td_s32 ret;
    mpi_isp_check_pipe_return(vi_pipe);
    mpi_isp_check_pointer_return(mod_ctrl);

    isp_mutex_lock(vi_pipe);
    ret = mpi_isp_get_module_ctrl(vi_pipe, mod_ctrl);
    isp_mutex_unlock(vi_pipe);
    return ret;
}

td_s32 ot_mpi_isp_set_fmw_state(ot_vi_pipe vi_pipe, const ot_isp_fmw_state state)
{
    td_s32 ret;
    mpi_isp_check_pipe_return(vi_pipe);
    isp_mutex_lock(vi_pipe);
    ret = mpi_isp_set_fmw_state(vi_pipe, state);
    isp_mutex_unlock(vi_pipe);
    return ret;
}

td_s32 ot_mpi_isp_get_fmw_state(ot_vi_pipe vi_pipe, ot_isp_fmw_state *state)
{
    td_s32 ret;
    mpi_isp_check_pipe_return(vi_pipe);
    mpi_isp_check_pointer_return(state);
    isp_mutex_lock(vi_pipe);
    ret = mpi_isp_get_fmw_state(vi_pipe, state);
    isp_mutex_unlock(vi_pipe);
    return ret;
}

td_s32 ot_mpi_isp_set_ldci_attr(ot_vi_pipe vi_pipe, const ot_isp_ldci_attr *ldci_attr)
{
    td_s32 ret;
    mpi_isp_check_pipe_return(vi_pipe);
    mpi_isp_check_pointer_return(ldci_attr);
    isp_mutex_lock(vi_pipe);
    ret = mpi_isp_set_ldci_attr(vi_pipe, ldci_attr);
    isp_mutex_unlock(vi_pipe);
    return ret;
}

td_s32 ot_mpi_isp_get_ldci_attr(ot_vi_pipe vi_pipe, ot_isp_ldci_attr *ldci_attr)
{
    td_s32 ret;
    mpi_isp_check_pipe_return(vi_pipe);
    mpi_isp_check_pointer_return(ldci_attr);
    isp_mutex_lock(vi_pipe);
    ret = mpi_isp_get_ldci_attr(vi_pipe, ldci_attr);
    isp_mutex_unlock(vi_pipe);
    return ret;
}

td_s32 ot_mpi_isp_set_drc_attr(ot_vi_pipe vi_pipe, const ot_isp_drc_attr *drc_attr)
{
    td_s32 ret;
    mpi_isp_check_pipe_return(vi_pipe);
    mpi_isp_check_pointer_return(drc_attr);
    isp_mutex_lock(vi_pipe);
    ret = mpi_isp_set_drc_attr(vi_pipe, drc_attr);
    isp_mutex_unlock(vi_pipe);
    return ret;
}

td_s32 ot_mpi_isp_get_drc_attr(ot_vi_pipe vi_pipe, ot_isp_drc_attr *drc_attr)
{
    td_s32 ret;
    mpi_isp_check_pipe_return(vi_pipe);
    mpi_isp_check_pointer_return(drc_attr);
    isp_mutex_lock(vi_pipe);
    ret = mpi_isp_get_drc_attr(vi_pipe, drc_attr);
    isp_mutex_unlock(vi_pipe);
    return ret;
}

td_s32 ot_mpi_isp_set_dehaze_attr(ot_vi_pipe vi_pipe, const ot_isp_dehaze_attr *dehaze_attr)
{
    td_s32 ret;

    mpi_isp_check_pipe_return(vi_pipe);
    mpi_isp_check_pointer_return(dehaze_attr);
    isp_mutex_lock(vi_pipe);
    ret = mpi_isp_set_dehaze_attr(vi_pipe, dehaze_attr);
    isp_mutex_unlock(vi_pipe);
    return ret;
}

td_s32 ot_mpi_isp_get_dehaze_attr(ot_vi_pipe vi_pipe, ot_isp_dehaze_attr *dehaze_attr)
{
    td_s32 ret;

    mpi_isp_check_pipe_return(vi_pipe);
    mpi_isp_check_pointer_return(dehaze_attr);
    isp_mutex_lock(vi_pipe);
    ret = mpi_isp_get_dehaze_attr(vi_pipe, dehaze_attr);
    isp_mutex_unlock(vi_pipe);
    return ret;
}

td_s32 ot_mpi_isp_set_fswdr_attr(ot_vi_pipe vi_pipe, const ot_isp_wdr_fs_attr *fswdr_attr)
{
    td_s32 ret;
    mpi_isp_check_pipe_return(vi_pipe);
    mpi_isp_check_pointer_return(fswdr_attr);
    isp_mutex_lock(vi_pipe);
    ret = mpi_isp_set_fswdr_attr(vi_pipe, fswdr_attr);
    isp_mutex_unlock(vi_pipe);
    return ret;
}

td_s32 ot_mpi_isp_get_fswdr_attr(ot_vi_pipe vi_pipe, ot_isp_wdr_fs_attr *fswdr_attr)
{
    td_s32 ret;
    mpi_isp_check_pipe_return(vi_pipe);
    mpi_isp_check_pointer_return(fswdr_attr);
    isp_mutex_lock(vi_pipe);
    ret = mpi_isp_get_fswdr_attr(vi_pipe, fswdr_attr);
    isp_mutex_unlock(vi_pipe);
    return ret;
}

td_s32 ot_mpi_isp_get_focus_stats(ot_vi_pipe vi_pipe, ot_isp_af_stats *af_stat)
{
#ifdef CONFIG_OT_ISP_AF_SUPPORT
    td_s32 ret;
    mpi_isp_check_pipe_return(vi_pipe);
    mpi_isp_check_pointer_return(af_stat);
    isp_mutex_lock(vi_pipe);
    ret = mpi_isp_get_focus_stats(vi_pipe, af_stat);
    isp_mutex_unlock(vi_pipe);
    return ret;
#else
    isp_not_support_interface_return(vi_pipe, af_stat);
#endif
}

td_s32 ot_mpi_isp_get_ae_stats(ot_vi_pipe vi_pipe, ot_isp_ae_stats *ae_stat)
{
    td_s32 ret;

    mpi_isp_check_pipe_return(vi_pipe);
    mpi_isp_check_pointer_return(ae_stat);
    isp_mutex_lock(vi_pipe);
    ret = mpi_isp_get_ae_stats(vi_pipe, ae_stat);
    isp_mutex_unlock(vi_pipe);
    return ret;
}

td_s32 ot_mpi_isp_get_ae_stitch_stats(ot_vi_pipe vi_pipe, ot_isp_ae_stitch_stats *stitch_stat)
{
#ifdef CONFIG_OT_VI_STITCH_GRP
    td_s32 ret;
    mpi_isp_check_pipe_return(vi_pipe);
    mpi_isp_check_pointer_return(stitch_stat);
    isp_mutex_lock(vi_pipe);
    ret = mpi_isp_get_ae_stitch_stats(vi_pipe, stitch_stat);
    isp_mutex_unlock(vi_pipe);
    return ret;
#else
    isp_not_support_interface_return(vi_pipe, stitch_stat);
#endif
}

td_s32 ot_mpi_isp_get_mg_stats(ot_vi_pipe vi_pipe, ot_isp_mg_stats *mg_stat)
{
#ifdef CONFIG_OT_ISP_MG_SUPPORT
    td_s32 ret;
    mpi_isp_check_pipe_return(vi_pipe);
    mpi_isp_check_pointer_return(mg_stat);
    isp_mutex_lock(vi_pipe);
    ret = isp_get_mg_stats(vi_pipe, mg_stat);
    isp_mutex_unlock(vi_pipe);
    return ret;
#else
    isp_not_support_interface_return(vi_pipe, mg_stat);
#endif
}

td_s32 ot_mpi_isp_get_wb_stitch_stats(ot_vi_pipe vi_pipe, ot_isp_wb_stitch_stats *stitch_wb_stat)
{
#ifdef CONFIG_OT_VI_STITCH_GRP
    td_s32 ret;
    mpi_isp_check_pipe_return(vi_pipe);
    mpi_isp_check_pointer_return(stitch_wb_stat);
    isp_mutex_lock(vi_pipe);
    ret = mpi_isp_get_wb_stitch_stats(vi_pipe, stitch_wb_stat);
    isp_mutex_unlock(vi_pipe);
    return ret;
#else
    isp_not_support_interface_return(vi_pipe, stitch_wb_stat);
#endif
}

td_s32 ot_mpi_isp_get_wb_stats(ot_vi_pipe vi_pipe, ot_isp_wb_stats *wb_stat)
{
    td_s32 ret;

    mpi_isp_check_pipe_return(vi_pipe);
    mpi_isp_check_pointer_return(wb_stat);
    isp_mutex_lock(vi_pipe);
    ret = mpi_isp_get_wb_stats(vi_pipe, wb_stat);
    isp_mutex_unlock(vi_pipe);
    return ret;
}

td_s32 ot_mpi_isp_set_fe_roi_cfg(ot_vi_pipe vi_pipe, const ot_isp_fe_roi_cfg            *fe_roi_cfg)
{
#ifdef CONFIG_OT_ISP_FE_ROI_SUPPORT
    td_s32 ret;

    mpi_isp_check_pipe_return(vi_pipe);
    mpi_isp_check_pointer_return(fe_roi_cfg);
    isp_mutex_lock(vi_pipe);
    ret = mpi_isp_set_fe_roi_cfg(vi_pipe, fe_roi_cfg);
    isp_mutex_unlock(vi_pipe);
    return ret;
#else
    isp_not_support_interface_return(vi_pipe, fe_roi_cfg);
#endif
}

td_s32 ot_mpi_isp_get_fe_roi_cfg(ot_vi_pipe vi_pipe, ot_isp_fe_roi_cfg *fe_roi_cfg)
{
#ifdef CONFIG_OT_ISP_FE_ROI_SUPPORT
    td_s32 ret;
    mpi_isp_check_pipe_return(vi_pipe);
    mpi_isp_check_pointer_return(fe_roi_cfg);

    isp_mutex_lock(vi_pipe);
    ret = mpi_isp_get_fe_roi_cfg(vi_pipe, fe_roi_cfg);
    isp_mutex_unlock(vi_pipe);
    return ret;
#else
    isp_not_support_interface_return(vi_pipe, fe_roi_cfg);
#endif
}

td_s32 ot_mpi_isp_set_stats_cfg(ot_vi_pipe vi_pipe, const ot_isp_stats_cfg *stat_cfg)
{
    td_s32 ret;
    mpi_isp_check_pipe_return(vi_pipe);
    mpi_isp_check_pointer_return(stat_cfg);
    isp_mutex_lock(vi_pipe);
    ret = mpi_isp_set_stats_cfg(vi_pipe, stat_cfg);
    isp_mutex_unlock(vi_pipe);
    if (ret != TD_SUCCESS) {
        return ret;
    }
    return isp_set_motion_stats_config(vi_pipe, &stat_cfg->motion_cfg);
}

td_s32 ot_mpi_isp_get_stats_cfg(ot_vi_pipe vi_pipe, ot_isp_stats_cfg *stat_cfg)
{
    td_s32 ret;
    mpi_isp_check_pipe_return(vi_pipe);
    mpi_isp_check_pointer_return(stat_cfg);
    isp_mutex_lock(vi_pipe);
    ret = mpi_isp_get_stats_cfg(vi_pipe, stat_cfg);
    isp_mutex_unlock(vi_pipe);
    if (ret != TD_SUCCESS) {
        return ret;
    }
    ret = isp_get_motion_stats_config(vi_pipe, &stat_cfg->motion_cfg);
    return ret;
}

td_s32 ot_mpi_isp_set_gamma_attr(ot_vi_pipe vi_pipe, const ot_isp_gamma_attr *gamma_attr)
{
    td_s32 ret;

    mpi_isp_check_pipe_return(vi_pipe);
    mpi_isp_check_pointer_return(gamma_attr);
    isp_mutex_lock(vi_pipe);
    ret = mpi_isp_set_gamma_attr(vi_pipe, gamma_attr);
    isp_mutex_unlock(vi_pipe);
    return ret;
}

td_s32 ot_mpi_isp_get_gamma_attr(ot_vi_pipe vi_pipe, ot_isp_gamma_attr *gamma_attr)
{
    td_s32 ret;

    mpi_isp_check_pipe_return(vi_pipe);
    mpi_isp_check_pointer_return(gamma_attr);
    isp_mutex_lock(vi_pipe);
    ret = mpi_isp_get_gamma_attr(vi_pipe, gamma_attr);
    isp_mutex_unlock(vi_pipe);
    return ret;
}

td_s32 ot_mpi_isp_set_pregamma_attr(ot_vi_pipe vi_pipe, const ot_isp_pregamma_attr *pregamma_attr)
{
#ifdef CONFIG_OT_ISP_PREGAMMA_SUPPORT
    td_s32 ret;
    mpi_isp_check_pipe_return(vi_pipe);
    mpi_isp_check_pointer_return(pregamma_attr);
    isp_mutex_lock(vi_pipe);
    ret = mpi_isp_set_pregamma_attr(vi_pipe, pregamma_attr);
    isp_mutex_unlock(vi_pipe);
    return ret;
#else
    isp_not_support_interface_return(vi_pipe, pregamma_attr);
#endif
}

td_s32 ot_mpi_isp_get_pregamma_attr(ot_vi_pipe vi_pipe, ot_isp_pregamma_attr *pregamma_attr)
{
#ifdef CONFIG_OT_ISP_PREGAMMA_SUPPORT
    td_s32 ret;

    mpi_isp_check_pipe_return(vi_pipe);
    mpi_isp_check_pointer_return(pregamma_attr);
    isp_mutex_lock(vi_pipe);
    ret = mpi_isp_get_pregamma_attr(vi_pipe, pregamma_attr);
    isp_mutex_unlock(vi_pipe);
    return ret;
#else
    isp_not_support_interface_return(vi_pipe, pregamma_attr);
#endif
}

td_s32 ot_mpi_isp_set_csc_attr(ot_vi_pipe vi_pipe, const ot_isp_csc_attr *csc_attr)
{
    td_s32 ret;
    mpi_isp_check_pipe_return(vi_pipe);
    mpi_isp_check_pointer_return(csc_attr);
    isp_mutex_lock(vi_pipe);
    ret = mpi_isp_set_csc_attr(vi_pipe, csc_attr);
    isp_mutex_unlock(vi_pipe);
    return ret;
}

td_s32 ot_mpi_isp_get_csc_attr(ot_vi_pipe vi_pipe, ot_isp_csc_attr *csc_attr)
{
    td_s32 ret;
    mpi_isp_check_pipe_return(vi_pipe);
    mpi_isp_check_pointer_return(csc_attr);
    isp_mutex_lock(vi_pipe);
    ret = mpi_isp_get_csc_attr(vi_pipe, csc_attr);
    isp_mutex_unlock(vi_pipe);
    return ret;
}
td_s32 ot_mpi_isp_set_dp_calibrate(ot_vi_pipe vi_pipe, const ot_isp_dp_static_calibrate *dp_calibrate)
{
#ifdef CONFIG_OT_ISP_DPC_STATIC_TABLE_SUPPORT
    td_s32 ret;
    mpi_isp_check_pipe_return(vi_pipe);
    mpi_isp_check_pointer_return(dp_calibrate);
    isp_mutex_lock(vi_pipe);
    ret = mpi_isp_set_dp_calibrate(vi_pipe, dp_calibrate);
    isp_mutex_unlock(vi_pipe);
    return ret;
#else
    isp_not_support_interface_return(vi_pipe, dp_calibrate);
#endif
}

td_s32 ot_mpi_isp_get_dp_calibrate(ot_vi_pipe vi_pipe, ot_isp_dp_static_calibrate *dp_calibrate)
{
#ifdef CONFIG_OT_ISP_DPC_STATIC_TABLE_SUPPORT
    td_s32 ret;
    mpi_isp_check_pipe_return(vi_pipe);
    mpi_isp_check_pointer_return(dp_calibrate);
    isp_mutex_lock(vi_pipe);
    ret = mpi_isp_get_dp_calibrate(vi_pipe, dp_calibrate);
    isp_mutex_unlock(vi_pipe);
    return ret;
#else
    isp_not_support_interface_return(vi_pipe, dp_calibrate);
#endif
}

td_s32 ot_mpi_isp_set_dp_static_attr(ot_vi_pipe vi_pipe, const ot_isp_dp_static_attr *dp_static_attr)
{
#ifdef CONFIG_OT_ISP_DPC_STATIC_TABLE_SUPPORT
    td_s32 ret;
    mpi_isp_check_pipe_return(vi_pipe);
    mpi_isp_check_pointer_return(dp_static_attr);
    isp_mutex_lock(vi_pipe);
    ret = mpi_isp_set_dp_static_attr(vi_pipe, dp_static_attr);
    isp_mutex_unlock(vi_pipe);
    return ret;
#else
    isp_not_support_interface_return(vi_pipe, dp_static_attr);
#endif
}

td_s32 ot_mpi_isp_get_dp_static_attr(ot_vi_pipe vi_pipe, ot_isp_dp_static_attr *dp_static_attr)
{
#ifdef CONFIG_OT_ISP_DPC_STATIC_TABLE_SUPPORT
    td_s32 ret;
    mpi_isp_check_pipe_return(vi_pipe);
    mpi_isp_check_pointer_return(dp_static_attr);
    isp_mutex_lock(vi_pipe);
    ret = mpi_isp_get_dp_static_attr(vi_pipe, dp_static_attr);
    isp_mutex_unlock(vi_pipe);
    return ret;
#else
    isp_not_support_interface_return(vi_pipe, dp_static_attr);
#endif
}

td_s32 ot_mpi_isp_set_dp_dynamic_attr(ot_vi_pipe vi_pipe, const ot_isp_dp_dynamic_attr *dp_dynamic_attr)
{
    td_s32 ret;
    mpi_isp_check_pipe_return(vi_pipe);
    mpi_isp_check_pointer_return(dp_dynamic_attr);
    isp_mutex_lock(vi_pipe);
    ret = mpi_isp_set_dp_dynamic_attr(vi_pipe, dp_dynamic_attr);
    isp_mutex_unlock(vi_pipe);
    return ret;
}

td_s32 ot_mpi_isp_get_dp_dynamic_attr(ot_vi_pipe vi_pipe, ot_isp_dp_dynamic_attr *dp_dynamic_attr)
{
    td_s32 ret;
    mpi_isp_check_pipe_return(vi_pipe);
    mpi_isp_check_pointer_return(dp_dynamic_attr);
    isp_mutex_lock(vi_pipe);
    ret = mpi_isp_get_dp_dynamic_attr(vi_pipe, dp_dynamic_attr);
    isp_mutex_unlock(vi_pipe);
    return ret;
}

td_s32 ot_mpi_isp_set_lblc_attr(ot_vi_pipe vi_pipe, const ot_isp_lblc_attr *lblc_attr)
{
#ifdef CONFIG_OT_ISP_LBLC_SUPPORT
    td_s32 ret;
    mpi_isp_check_pipe_return(vi_pipe);
    mpi_isp_check_pointer_return(lblc_attr);
    isp_mutex_lock(vi_pipe);
    ret = mpi_isp_set_lblc_attr(vi_pipe, lblc_attr);
    isp_mutex_unlock(vi_pipe);
    return ret;
#else
    isp_not_support_interface_return(vi_pipe, lblc_attr);
#endif
}

td_s32 ot_mpi_isp_get_lblc_attr(ot_vi_pipe vi_pipe, ot_isp_lblc_attr *lblc_attr)
{
#ifdef CONFIG_OT_ISP_LBLC_SUPPORT
    td_s32 ret;
    mpi_isp_check_pipe_return(vi_pipe);
    mpi_isp_check_pointer_return(lblc_attr);
    isp_mutex_lock(vi_pipe);
    ret = mpi_isp_get_lblc_attr(vi_pipe, lblc_attr);
    isp_mutex_unlock(vi_pipe);
    return ret;
#else
    isp_not_support_interface_return(vi_pipe, lblc_attr);
#endif
}

td_s32 ot_mpi_isp_set_lblc_lut_attr(ot_vi_pipe vi_pipe, const ot_isp_lblc_lut_attr *lblc_lut_attr)
{
#ifdef CONFIG_OT_ISP_LBLC_SUPPORT
    td_s32 ret;
    mpi_isp_check_pipe_return(vi_pipe);
    mpi_isp_check_pointer_return(lblc_lut_attr);
    isp_mutex_lock(vi_pipe);
    ret = mpi_isp_set_lblc_lut_attr(vi_pipe, lblc_lut_attr);
    isp_mutex_unlock(vi_pipe);
    return ret;
#else
    isp_not_support_interface_return(vi_pipe, lblc_lut_attr);
#endif
}

td_s32 ot_mpi_isp_get_lblc_lut_attr(ot_vi_pipe vi_pipe, ot_isp_lblc_lut_attr *lblc_lut_attr)
{
#ifdef CONFIG_OT_ISP_LBLC_SUPPORT
    td_s32 ret;
    mpi_isp_check_pipe_return(vi_pipe);
    mpi_isp_check_pointer_return(lblc_lut_attr);
    isp_mutex_lock(vi_pipe);
    ret = mpi_isp_get_lblc_lut_attr(vi_pipe, lblc_lut_attr);
    isp_mutex_unlock(vi_pipe);
    return ret;
#else
    isp_not_support_interface_return(vi_pipe, lblc_lut_attr);
#endif
}

td_s32 ot_mpi_isp_set_mesh_shading_attr(ot_vi_pipe vi_pipe, const ot_isp_shading_attr *shading_attr)
{
    td_s32 ret = TD_SUCCESS;

    mpi_isp_check_pipe_return(vi_pipe);
    mpi_isp_check_pointer_return(shading_attr);
    isp_mutex_lock(vi_pipe);
    ret = mpi_isp_usr_set_mesh_shading_attr(vi_pipe, shading_attr);
    isp_mutex_unlock(vi_pipe);
    return ret;
}

td_s32 ot_mpi_isp_get_mesh_shading_attr(ot_vi_pipe vi_pipe, ot_isp_shading_attr *shading_attr)
{
    td_s32 ret;
    mpi_isp_check_pipe_return(vi_pipe);
    mpi_isp_check_pointer_return(shading_attr);
    isp_mutex_lock(vi_pipe);
    ret = mpi_isp_usr_get_mesh_shading_attr(vi_pipe, shading_attr);
    isp_mutex_unlock(vi_pipe);
    return ret;
}

td_s32 ot_mpi_isp_set_mesh_shading_gain_lut_attr(ot_vi_pipe vi_pipe, const ot_isp_shading_lut_attr *shading_lut_attr)
{
    td_s32 ret = TD_SUCCESS;
    mpi_isp_check_pipe_return(vi_pipe);
    mpi_isp_check_pointer_return(shading_lut_attr);
    isp_mutex_lock(vi_pipe);
    ret = mpi_isp_usr_set_mesh_shading_lut_attr(vi_pipe, shading_lut_attr);
    isp_mutex_unlock(vi_pipe);
    return ret;
}

td_s32 ot_mpi_isp_get_mesh_shading_gain_lut_attr(ot_vi_pipe vi_pipe, ot_isp_shading_lut_attr *shading_lut_attr)
{
    td_s32 ret;
    mpi_isp_check_pipe_return(vi_pipe);
    mpi_isp_check_pointer_return(shading_lut_attr);
    isp_mutex_lock(vi_pipe);
    ret = mpi_isp_usr_get_mesh_shading_lut_attr(vi_pipe, shading_lut_attr);
    isp_mutex_unlock(vi_pipe);
    return ret;
}

td_s32 ot_mpi_isp_set_auto_color_shading_attr(ot_vi_pipe vi_pipe, const ot_isp_acs_attr *acs_attr)
{
    td_s32 ret;
    mpi_isp_check_pipe_return(vi_pipe);
    mpi_isp_check_pointer_return(acs_attr);
    isp_mutex_lock(vi_pipe);
    ret = mpi_isp_set_auto_color_shading_attr(vi_pipe, acs_attr);
    isp_mutex_unlock(vi_pipe);
    return ret;
}

td_s32 ot_mpi_isp_get_auto_color_shading_attr(ot_vi_pipe vi_pipe, ot_isp_acs_attr *acs_attr)
{
    td_s32 ret;
    mpi_isp_check_pipe_return(vi_pipe);
    mpi_isp_check_pointer_return(acs_attr);
    isp_mutex_lock(vi_pipe);
    ret = mpi_isp_get_auto_color_shading_attr(vi_pipe, acs_attr);
    isp_mutex_unlock(vi_pipe);
    return ret;
}

td_s32 ot_mpi_isp_get_lightbox_gain(ot_vi_pipe vi_pipe, ot_isp_awb_calibration_gain *awb_calibration_gain)
{
    mpi_isp_check_pipe_return(vi_pipe);
    mpi_isp_check_pointer_return(awb_calibration_gain);
    isp_check_open_return(vi_pipe);
    isp_check_mem_init_return(vi_pipe);
    isp_check_vreg_permission_return(vi_pipe);

    return isp_get_lightbox_gain(vi_pipe, awb_calibration_gain);
}

td_s32 ot_mpi_isp_mesh_shading_calibration(ot_vi_pipe vi_pipe, const td_u16 *src_raw,
                                           ot_isp_mlsc_calibration_cfg *mlsc_cali_cfg,
                                           ot_isp_mesh_shading_table *mlsc_table)
{
    td_s32 ret;
    mpi_isp_check_pipe_return(vi_pipe);
    mpi_isp_check_pointer_return(src_raw);
    mpi_isp_check_pointer_return(mlsc_cali_cfg);
    mpi_isp_check_pointer_return(mlsc_table);

    ret = isp_mesh_shading_calibration_cfg_check(mlsc_cali_cfg);
    if (ret != TD_SUCCESS) {
        return ret;
    }

    return isp_mesh_shading_calibration(src_raw, mlsc_cali_cfg, mlsc_table);
}

td_s32 ot_mpi_isp_set_cac_attr(ot_vi_pipe vi_pipe, const ot_isp_cac_attr *cac_attr)
{
    td_s32 ret;
    mpi_isp_check_pipe_return(vi_pipe);
    mpi_isp_check_pointer_return(cac_attr);
    isp_mutex_lock(vi_pipe);
    ret = mpi_isp_set_cac_attr(vi_pipe, cac_attr);
    isp_mutex_unlock(vi_pipe);
    return ret;
}

td_s32 ot_mpi_isp_get_cac_attr(ot_vi_pipe vi_pipe, ot_isp_cac_attr *cac_attr)
{
    td_s32 ret;
    mpi_isp_check_pipe_return(vi_pipe);
    mpi_isp_check_pointer_return(cac_attr);
    isp_mutex_lock(vi_pipe);
    ret = mpi_isp_get_cac_attr(vi_pipe, cac_attr);
    isp_mutex_unlock(vi_pipe);
    return ret;
}

td_s32 ot_mpi_isp_set_bayershp_attr(ot_vi_pipe vi_pipe, const ot_isp_bayershp_attr *bshp_attr)
{
#ifdef  CONFIG_OT_ISP_BAYER_SHP_SUPPORT
    td_s32 ret;
    mpi_isp_check_pipe_return(vi_pipe);
    mpi_isp_check_pointer_return(bshp_attr);
    isp_mutex_lock(vi_pipe);
    ret = mpi_isp_set_bayershp_attr(vi_pipe, bshp_attr);
    isp_mutex_unlock(vi_pipe);
    return ret;
#else
    isp_not_support_interface_return(vi_pipe, bshp_attr);
#endif
}

td_s32 ot_mpi_isp_get_bayershp_attr(ot_vi_pipe vi_pipe, ot_isp_bayershp_attr *bshp_attr)
{
#ifdef  CONFIG_OT_ISP_BAYER_SHP_SUPPORT
    td_s32 ret;
    mpi_isp_check_pipe_return(vi_pipe);
    mpi_isp_check_pointer_return(bshp_attr);
    isp_mutex_lock(vi_pipe);
    ret = mpi_isp_get_bayershp_attr(vi_pipe, bshp_attr);
    isp_mutex_unlock(vi_pipe);
    return ret;
#else
    isp_not_support_interface_return(vi_pipe, bshp_attr);
#endif
}

td_s32 ot_mpi_isp_set_rc_attr(ot_vi_pipe vi_pipe, const ot_isp_rc_attr *rc_attr)
{
#ifdef CONFIG_OT_ISP_RADIAL_CROP_SUPPORT
    td_s32 ret;
    mpi_isp_check_pipe_return(vi_pipe);
    mpi_isp_check_pointer_return(rc_attr);
    isp_mutex_lock(vi_pipe);
    ret = isp_set_rc_attr(vi_pipe, rc_attr);
    isp_mutex_unlock(vi_pipe);
    return ret;
#else
    isp_not_support_interface_return(vi_pipe, rc_attr);
#endif
}

td_s32 ot_mpi_isp_get_rc_attr(ot_vi_pipe vi_pipe, ot_isp_rc_attr *rc_attr)
{
#ifdef CONFIG_OT_ISP_RADIAL_CROP_SUPPORT
    td_s32 ret;
    mpi_isp_check_pipe_return(vi_pipe);
    mpi_isp_check_pointer_return(rc_attr);
    isp_mutex_lock(vi_pipe);
    ret = isp_get_rc_attr(vi_pipe, rc_attr);
    isp_mutex_unlock(vi_pipe);
    return ret;
#else
    isp_not_support_interface_return(vi_pipe, rc_attr);
#endif
}

td_s32 ot_mpi_isp_set_nr_attr(ot_vi_pipe vi_pipe, const ot_isp_nr_attr *nr_attr)
{
    td_s32 ret;
    mpi_isp_check_pipe_return(vi_pipe);
    mpi_isp_check_pointer_return(nr_attr);
    isp_mutex_lock(vi_pipe);
    ret = mpi_isp_set_nr_attr(vi_pipe, nr_attr);
    isp_mutex_unlock(vi_pipe);
    return ret;
}

td_s32 ot_mpi_isp_get_nr_attr(ot_vi_pipe vi_pipe, ot_isp_nr_attr *nr_attr)
{
    td_s32 ret;
    mpi_isp_check_pipe_return(vi_pipe);
    mpi_isp_check_pointer_return(nr_attr);
    isp_mutex_lock(vi_pipe);
    ret = mpi_isp_get_nr_attr(vi_pipe, nr_attr);
    isp_mutex_unlock(vi_pipe);
    return ret;
}

td_s32 ot_mpi_isp_set_rgbir_attr(ot_vi_pipe vi_pipe, const ot_isp_rgbir_attr *rgbir_attr)
{
#ifdef CONFIG_OT_ISP_RGBIR_SUPPORT
    td_s32 ret;

    mpi_isp_check_pipe_return(vi_pipe);
    mpi_isp_check_pointer_return(rgbir_attr);
    isp_mutex_lock(vi_pipe);
    ret = mpi_isp_set_rgbir_attr(vi_pipe, rgbir_attr);
    isp_mutex_unlock(vi_pipe);
    return ret;
#else
    isp_not_support_interface_return(vi_pipe, rgbir_attr);
#endif
}

td_s32 ot_mpi_isp_get_rgbir_attr(ot_vi_pipe vi_pipe, ot_isp_rgbir_attr *rgbir_attr)
{
#ifdef CONFIG_OT_ISP_RGBIR_SUPPORT
    td_s32 ret;
    mpi_isp_check_pipe_return(vi_pipe);
    mpi_isp_check_pointer_return(rgbir_attr);
    isp_mutex_lock(vi_pipe);
    ret = mpi_isp_get_rgbir_attr(vi_pipe, rgbir_attr);
    isp_mutex_unlock(vi_pipe);
    return ret;
#else
    isp_not_support_interface_return(vi_pipe, rgbir_attr);
#endif
}

td_s32 ot_mpi_isp_set_color_tone_attr(ot_vi_pipe vi_pipe, const ot_isp_color_tone_attr *ct_attr)
{
    td_s32 ret;
    mpi_isp_check_pipe_return(vi_pipe);
    mpi_isp_check_pointer_return(ct_attr);
    isp_mutex_lock(vi_pipe);
    ret = mpi_isp_set_color_tone_attr(vi_pipe, ct_attr);
    isp_mutex_unlock(vi_pipe);
    return ret;
}

td_s32 ot_mpi_isp_get_color_tone_attr(ot_vi_pipe vi_pipe, ot_isp_color_tone_attr *ct_attr)
{
    td_s32 ret;
    mpi_isp_check_pipe_return(vi_pipe);
    mpi_isp_check_pointer_return(ct_attr);
    isp_mutex_lock(vi_pipe);
    ret = mpi_isp_get_color_tone_attr(vi_pipe, ct_attr);
    isp_mutex_unlock(vi_pipe);
    return ret;
}

td_s32 ot_mpi_isp_set_sharpen_attr(ot_vi_pipe vi_pipe, const ot_isp_sharpen_attr *shp_attr)
{
    td_s32 ret;
    mpi_isp_check_pipe_return(vi_pipe);
    mpi_isp_check_pointer_return(shp_attr);
    isp_mutex_lock(vi_pipe);
    ret = mpi_isp_set_sharpen_attr(vi_pipe, shp_attr);
    isp_mutex_unlock(vi_pipe);
    return ret;
}

td_s32 ot_mpi_isp_get_sharpen_attr(ot_vi_pipe vi_pipe, ot_isp_sharpen_attr *shp_attr)
{
    td_s32 ret;
    mpi_isp_check_pipe_return(vi_pipe);
    mpi_isp_check_pointer_return(shp_attr);
    isp_mutex_lock(vi_pipe);
    ret = mpi_isp_get_sharpen_attr(vi_pipe, shp_attr);
    isp_mutex_unlock(vi_pipe);
    return ret;
}

/* Crosstalk Removal Strength */
td_s32 ot_mpi_isp_set_crosstalk_attr(ot_vi_pipe vi_pipe, const ot_isp_cr_attr *cr_attr)
{
#ifdef CONFIG_OT_ISP_CR_SUPPORT
    td_s32 ret;
    mpi_isp_check_pipe_return(vi_pipe);
    mpi_isp_check_pointer_return(cr_attr);
    isp_mutex_lock(vi_pipe);
    ret = mpi_isp_set_crosstalk_attr(vi_pipe, cr_attr);
    isp_mutex_unlock(vi_pipe);
    return ret;
#else
    isp_not_support_interface_return(vi_pipe, cr_attr);
#endif
}

td_s32 ot_mpi_isp_get_crosstalk_attr(ot_vi_pipe vi_pipe, ot_isp_cr_attr *cr_attr)
{
#ifdef CONFIG_OT_ISP_CR_SUPPORT
    td_s32 ret;
    mpi_isp_check_pipe_return(vi_pipe);
    mpi_isp_check_pointer_return(cr_attr);
    isp_mutex_lock(vi_pipe);
    ret = mpi_isp_get_crosstalk_attr(vi_pipe, cr_attr);
    isp_mutex_unlock(vi_pipe);
    return ret;
#else
    isp_not_support_interface_return(vi_pipe, cr_attr);
#endif
}

td_s32 ot_mpi_isp_set_anti_false_color_attr(ot_vi_pipe vi_pipe, const ot_isp_anti_false_color_attr *anti_false_color)
{
    td_s32 ret;
    mpi_isp_check_pipe_return(vi_pipe);
    mpi_isp_check_pointer_return(anti_false_color);
    isp_mutex_lock(vi_pipe);
    ret = mpi_isp_set_anti_false_color_attr(vi_pipe, anti_false_color);
    isp_mutex_unlock(vi_pipe);
    return ret;
}

td_s32 ot_mpi_isp_get_anti_false_color_attr(ot_vi_pipe vi_pipe, ot_isp_anti_false_color_attr *anti_false_color)
{
    td_s32 ret;
    mpi_isp_check_pipe_return(vi_pipe);
    mpi_isp_check_pointer_return(anti_false_color);
    isp_mutex_lock(vi_pipe);
    ret = mpi_isp_get_anti_false_color_attr(vi_pipe, anti_false_color);
    isp_mutex_unlock(vi_pipe);
    return ret;
}

td_s32 ot_mpi_isp_set_demosaic_attr(ot_vi_pipe vi_pipe, const ot_isp_demosaic_attr *demosaic_attr)
{
    td_s32 ret;
    mpi_isp_check_pipe_return(vi_pipe);
    mpi_isp_check_pointer_return(demosaic_attr);
    isp_mutex_lock(vi_pipe);
    ret = mpi_isp_set_demosaic_attr(vi_pipe, demosaic_attr);
    isp_mutex_unlock(vi_pipe);
    return ret;
}

td_s32 ot_mpi_isp_get_demosaic_attr(ot_vi_pipe vi_pipe, ot_isp_demosaic_attr *demosaic_attr)
{
    td_s32 ret;
    mpi_isp_check_pipe_return(vi_pipe);
    mpi_isp_check_pointer_return(demosaic_attr);
    isp_mutex_lock(vi_pipe);
    ret = mpi_isp_get_demosaic_attr(vi_pipe, demosaic_attr);
    isp_mutex_unlock(vi_pipe);
    return ret;
}

td_s32 ot_mpi_isp_set_ca_attr(ot_vi_pipe vi_pipe, const ot_isp_ca_attr *ca_attr)
{
#ifdef CONFIG_OT_ISP_CA_SUPPORT
    td_s32 ret;
    mpi_isp_check_pipe_return(vi_pipe);
    mpi_isp_check_pointer_return(ca_attr);

    isp_mutex_lock(vi_pipe);
    ret = mpi_isp_set_ca_attr(vi_pipe, ca_attr);
    isp_mutex_unlock(vi_pipe);
    return ret;
#else
    isp_not_support_interface_return(vi_pipe, ca_attr);
#endif
}

td_s32 ot_mpi_isp_get_ca_attr(ot_vi_pipe vi_pipe, ot_isp_ca_attr *ca_attr)
{
#ifdef CONFIG_OT_ISP_CA_SUPPORT
    td_s32 ret;
    mpi_isp_check_pipe_return(vi_pipe);
    mpi_isp_check_pointer_return(ca_attr);
    isp_mutex_lock(vi_pipe);
    ret = mpi_isp_get_ca_attr(vi_pipe, ca_attr);
    isp_mutex_unlock(vi_pipe);
    return ret;
#else
    isp_not_support_interface_return(vi_pipe, ca_attr);
#endif
}

td_s32 ot_mpi_isp_set_clut_coeff(ot_vi_pipe vi_pipe, const ot_isp_clut_lut *clut_lut)
{
#ifdef CONFIG_OT_ISP_CLUT_SUPPORT
    td_s32 ret;
    mpi_isp_check_pipe_return(vi_pipe);
    mpi_isp_check_pointer_return(clut_lut);
    isp_mutex_lock(vi_pipe);
    ret = isp_set_clut_coeff(vi_pipe, clut_lut);
    isp_mutex_unlock(vi_pipe);
    return ret;
#else
    isp_not_support_interface_return(vi_pipe, clut_lut);
#endif
}

td_s32 ot_mpi_isp_get_clut_coeff(ot_vi_pipe vi_pipe, ot_isp_clut_lut *clut_lut)
{
#ifdef CONFIG_OT_ISP_CLUT_SUPPORT
    td_s32 ret;
    mpi_isp_check_pipe_return(vi_pipe);
    mpi_isp_check_pointer_return(clut_lut);
    isp_mutex_lock(vi_pipe);
    ret = isp_get_clut_coeff(vi_pipe, clut_lut);
    isp_mutex_unlock(vi_pipe);
    return ret;
#else
    isp_not_support_interface_return(vi_pipe, clut_lut);
#endif
}

td_s32 ot_mpi_isp_set_clut_attr(ot_vi_pipe vi_pipe, const ot_isp_clut_attr *clut_attr)
{
#ifdef CONFIG_OT_ISP_CLUT_SUPPORT
    td_s32 ret;
    mpi_isp_check_pipe_return(vi_pipe);
    mpi_isp_check_pointer_return(clut_attr);
    isp_mutex_lock(vi_pipe);
    ret = isp_set_clut_attr(vi_pipe, clut_attr);
    isp_mutex_unlock(vi_pipe);
    return ret;
#else
    isp_not_support_interface_return(vi_pipe, clut_attr);
#endif
}

td_s32 ot_mpi_isp_get_clut_attr(ot_vi_pipe vi_pipe, ot_isp_clut_attr *clut_attr)
{
#ifdef CONFIG_OT_ISP_CLUT_SUPPORT
    td_s32 ret;
    mpi_isp_check_pipe_return(vi_pipe);
    mpi_isp_check_pointer_return(clut_attr);
    isp_mutex_lock(vi_pipe);
    ret = isp_get_clut_attr(vi_pipe, clut_attr);
    isp_mutex_unlock(vi_pipe);
    return ret;
#else
    isp_not_support_interface_return(vi_pipe, clut_attr);
#endif
}

td_s32 ot_mpi_isp_set_black_level_attr(ot_vi_pipe vi_pipe, const ot_isp_black_level_attr *black_level)
{
    td_s32 ret;
    mpi_isp_check_pipe_return(vi_pipe);
    mpi_isp_check_pointer_return(black_level);
    isp_mutex_lock(vi_pipe);
    ret = mpi_isp_set_black_level_attr(vi_pipe, black_level);
    isp_mutex_unlock(vi_pipe);
    return ret;
}

td_s32 ot_mpi_isp_get_black_level_attr(ot_vi_pipe vi_pipe, ot_isp_black_level_attr *black_level)
{
    td_s32 ret;
    mpi_isp_check_pipe_return(vi_pipe);
    mpi_isp_check_pointer_return(black_level);
    isp_mutex_lock(vi_pipe);
    ret = mpi_isp_get_black_level_attr(vi_pipe, black_level);
    isp_mutex_unlock(vi_pipe);
    return ret;
}

td_s32 ot_mpi_isp_fpn_calibrate(ot_vi_pipe vi_pipe, ot_isp_fpn_calibrate_attr *calibrate_attr)
{
#ifdef CONFIG_OT_VI_PIPE_FPN
    isp_check_phy_pipe_return(vi_pipe);
    mpi_isp_check_pointer_return(calibrate_attr);
    return mpi_isp_fpn_calibrate(vi_pipe, calibrate_attr);
#else
    isp_not_support_interface_return(vi_pipe, calibrate_attr);
#endif
}

td_s32 ot_mpi_isp_set_fpn_attr(ot_vi_pipe vi_pipe, const ot_isp_fpn_attr *fpn_attr)
{
#ifdef CONFIG_OT_VI_PIPE_FPN
    isp_check_phy_pipe_return(vi_pipe);
    mpi_isp_check_pointer_return(fpn_attr);
    return mpi_isp_set_fpn_attr(vi_pipe, fpn_attr);
#else
    isp_not_support_interface_return(vi_pipe, fpn_attr);
#endif
}

td_s32 ot_mpi_isp_get_fpn_attr(ot_vi_pipe vi_pipe, ot_isp_fpn_attr *fpn_attr)
{
#ifdef CONFIG_OT_VI_PIPE_FPN
    isp_check_phy_pipe_return(vi_pipe);
    mpi_isp_check_pointer_return(fpn_attr);
    isp_check_open_return(vi_pipe);
    isp_check_mem_init_return(vi_pipe);
    isp_check_vreg_permission_return(vi_pipe);

    return isp_get_correction_attr(vi_pipe, fpn_attr);
#else
    isp_not_support_interface_return(vi_pipe, fpn_attr);
#endif
}

td_s32 ot_mpi_isp_get_isp_reg_attr(ot_vi_pipe vi_pipe, ot_isp_reg_attr *isp_reg_attr)
{
    td_s32 ret;
    mpi_isp_check_pipe_return(vi_pipe);
    mpi_isp_check_pointer_return(isp_reg_attr);
    isp_mutex_lock(vi_pipe);
    ret = mpi_isp_get_reg_attr(vi_pipe, isp_reg_attr);
    isp_mutex_unlock(vi_pipe);
    return ret;
}

td_s32 ot_mpi_isp_set_debug(ot_vi_pipe vi_pipe, const ot_isp_debug_info *isp_debug)
{
    td_s32 ret;
    mpi_isp_check_pipe_return(vi_pipe);
    mpi_isp_check_pointer_return(isp_debug);
    isp_mutex_lock(vi_pipe);
    ret = isp_dbg_reg_set(vi_pipe, isp_debug);
    isp_mutex_unlock(vi_pipe);
    return ret;
}

td_s32 ot_mpi_isp_get_debug(ot_vi_pipe vi_pipe, ot_isp_debug_info *isp_debug)
{
    td_s32 ret;
    mpi_isp_check_pipe_return(vi_pipe);
    mpi_isp_check_pointer_return(isp_debug);

    isp_mutex_lock(vi_pipe);
    ret = isp_dbg_reg_get(vi_pipe, isp_debug);
    isp_mutex_unlock(vi_pipe);
    return ret;
}

td_s32 ot_mpi_isp_set_ctrl_param(ot_vi_pipe vi_pipe, const ot_isp_ctrl_param *isp_ctrl_param)
{
    mpi_isp_check_pipe_return(vi_pipe);
    mpi_isp_check_pointer_return(isp_ctrl_param);

    isp_check_open_return(vi_pipe);

    return ioctl(isp_get_fd(vi_pipe), ISP_SET_CTRL_PARAM, isp_ctrl_param);
}

td_s32 ot_mpi_isp_get_ctrl_param(ot_vi_pipe vi_pipe, ot_isp_ctrl_param *isp_ctrl_param)
{
    mpi_isp_check_pipe_return(vi_pipe);
    mpi_isp_check_pointer_return(isp_ctrl_param);
    isp_check_open_return(vi_pipe);

    return ioctl(isp_get_fd(vi_pipe), ISP_GET_CTRL_PARAM, isp_ctrl_param);
}

td_s32 ot_mpi_isp_query_inner_state_info(ot_vi_pipe vi_pipe, ot_isp_inner_state_info *inner_state_info)
{
    td_s32 ret;
    mpi_isp_check_pipe_return(vi_pipe);
    mpi_isp_check_pointer_return(inner_state_info);
    isp_mutex_lock(vi_pipe);
    ret = mpi_isp_query_inner_state_info(vi_pipe, inner_state_info);
    isp_mutex_unlock(vi_pipe);
    return ret;
}

td_s32 ot_mpi_isp_get_dng_image_static_info(ot_vi_pipe vi_pipe, ot_isp_dng_image_static_info *dng_image_static_info)
{
    td_s32 ret;
    mpi_isp_check_pipe_return(vi_pipe);
    mpi_isp_check_pointer_return(dng_image_static_info);
    isp_mutex_lock(vi_pipe);
    ret = mpi_isp_get_dng_image_static_info(vi_pipe, dng_image_static_info);
    isp_mutex_unlock(vi_pipe);
    return ret;
}

td_s32 ot_mpi_isp_set_dng_color_param(ot_vi_pipe vi_pipe, const ot_isp_dng_color_param *dng_color_param)
{
    td_s32 ret;
    mpi_isp_check_pipe_return(vi_pipe);
    mpi_isp_check_pointer_return(dng_color_param);
    isp_mutex_lock(vi_pipe);
    ret = mpi_isp_set_dng_color_param(vi_pipe, dng_color_param);
    isp_mutex_unlock(vi_pipe);
    return ret;
}

td_s32 ot_mpi_isp_get_dng_color_param(ot_vi_pipe vi_pipe, ot_isp_dng_color_param *dng_color_param)
{
    td_s32 ret;
    mpi_isp_check_pipe_return(vi_pipe);
    mpi_isp_check_pointer_return(dng_color_param);
    isp_mutex_lock(vi_pipe);
    ret = mpi_isp_get_dng_color_param(vi_pipe, dng_color_param);
    isp_mutex_unlock(vi_pipe);
    return ret;
}

td_s32 ot_mpi_isp_ir_auto(ot_vi_pipe vi_pipe, ot_isp_ir_auto_attr *ir_attr)
{
    td_s32 ret;
    td_u32 iso;
    ot_isp_wb_stats wb_stat = {0};
    mpi_isp_check_pipe_return(vi_pipe);
    mpi_isp_check_pointer_return(ir_attr);
    isp_mutex_lock(vi_pipe);
    isp_check_open_goto(vi_pipe, ret, exit);
    isp_check_mem_init_goto(vi_pipe, ret, exit);
    isp_check_vreg_permission_goto(vi_pipe, ret, exit);

    ret = isp_check_ir_auto_attr(vi_pipe, ir_attr);
    if (ret != TD_SUCCESS) {
        goto exit;
    }
    ret = mpi_isp_get_wb_stats(vi_pipe, &wb_stat);
    if (ret != TD_SUCCESS) {
        isp_err_trace("OT_MPI_ISP_GetWBStatistics failed\n");
        goto exit;
    }
    iso = ot_ext_system_sys_iso_read(vi_pipe);
    isp_mutex_unlock(vi_pipe); /* ir_auto do not access to vreg */
    return isp_ir_auto_run_once(vi_pipe, ir_attr, &wb_stat, iso);
exit:
    isp_mutex_unlock(vi_pipe);
    return ret;
}

td_s32 ot_mpi_isp_set_smart_info(ot_vi_pipe vi_pipe, const ot_isp_smart_info *smart_info)
{
    td_s32 ret;
    mpi_isp_check_pipe_return(vi_pipe);
    mpi_isp_check_pointer_return(smart_info);
    isp_mutex_lock(vi_pipe);
    ret = mpi_isp_set_smart_info(vi_pipe, smart_info);
    isp_mutex_unlock(vi_pipe);
    return ret;
}

td_s32 ot_mpi_isp_get_smart_info(ot_vi_pipe vi_pipe, ot_isp_smart_info *smart_info)
{
    td_s32 ret;
    mpi_isp_check_pipe_return(vi_pipe);
    mpi_isp_check_pointer_return(smart_info);
    isp_mutex_lock(vi_pipe);
    ret = mpi_isp_get_smart_info(vi_pipe, smart_info);
    isp_mutex_unlock(vi_pipe);
    return ret;
}

td_s32 ot_mpi_isp_calc_flicker_type(ot_vi_pipe vi_pipe, ot_isp_calc_flicker_input *input_param,
                                    ot_isp_calc_flicker_output *output_param, ot_video_frame_info frame[],
                                    td_u32 array_size)
{
    mpi_isp_check_pipe_return(vi_pipe);
    mpi_isp_check_pointer_return(input_param);
    mpi_isp_check_pointer_return(output_param);
    return mpi_isp_calc_flicker_type(vi_pipe, input_param, output_param, frame, array_size);
}

td_s32 ot_mpi_isp_set_expander_attr(ot_vi_pipe vi_pipe, const ot_isp_expander_attr *expander_attr)
{
    td_s32 ret;
    mpi_isp_check_pipe_return(vi_pipe);
    mpi_isp_check_pointer_return(expander_attr);
    isp_mutex_lock(vi_pipe);
    ret = mpi_isp_set_expander_attr(vi_pipe, expander_attr);
    isp_mutex_unlock(vi_pipe);
    return ret;
}

td_s32 ot_mpi_isp_get_expander_attr(ot_vi_pipe vi_pipe, ot_isp_expander_attr *expander_attr)
{
    td_s32 ret;
    mpi_isp_check_pipe_return(vi_pipe);
    mpi_isp_check_pointer_return(expander_attr);
    isp_mutex_lock(vi_pipe);
    ret = mpi_isp_get_expander_attr(vi_pipe, expander_attr);
    isp_mutex_unlock(vi_pipe);
    return ret;
}

td_s32 ot_mpi_isp_set_be_frame_attr(ot_vi_pipe vi_pipe, const ot_isp_be_frame_attr *be_frame_attr)
{
    td_s32 ret;
    mpi_isp_check_pipe_return(vi_pipe);
    mpi_isp_check_pointer_return(be_frame_attr);
    isp_mutex_lock(vi_pipe);
    ret = mpi_isp_set_be_frame_pos_attr(vi_pipe, be_frame_attr);
    isp_mutex_unlock(vi_pipe);
    return ret;
}

td_s32 ot_mpi_isp_get_be_frame_attr(ot_vi_pipe vi_pipe, ot_isp_be_frame_attr *be_frame_attr)
{
    td_s32 ret;
    mpi_isp_check_pipe_return(vi_pipe);
    mpi_isp_check_pointer_return(be_frame_attr);
    isp_mutex_lock(vi_pipe);
    ret = mpi_isp_get_be_frame_pos_attr(vi_pipe, be_frame_attr);
    isp_mutex_unlock(vi_pipe);
    return ret;
}

td_s32 ot_mpi_isp_set_noise_calibration(ot_vi_pipe vi_pipe,
    const ot_isp_noise_calibration *noise_calibration)
{
    td_s32 ret;
    mpi_isp_check_pipe_return(vi_pipe);
    mpi_isp_check_pointer_return(noise_calibration);
    isp_mutex_lock(vi_pipe);
    ret = isp_set_noise_calibration(vi_pipe, noise_calibration);
    isp_mutex_unlock(vi_pipe);
    return ret;
}

td_s32 ot_mpi_isp_get_noise_calibration(ot_vi_pipe vi_pipe, ot_isp_noise_calibration *noise_calibration)
{
    td_s32 ret;
    mpi_isp_check_pipe_return(vi_pipe);
    mpi_isp_check_pointer_return(noise_calibration);
    isp_mutex_lock(vi_pipe);
    ret = mpi_isp_get_noise_calibration(vi_pipe, noise_calibration);
    isp_mutex_unlock(vi_pipe);
    return ret;
}

td_s32 ot_mpi_isp_set_detail_stats_cfg(ot_vi_pipe vi_pipe,
    const ot_isp_detail_stats_cfg *detail_stats_cfg)
{
#ifdef CONFIG_OT_ISP_DETAIL_STATS_SUPPORT
    isp_check_fe_pipe_return(vi_pipe);
    mpi_isp_check_pointer_return(detail_stats_cfg);
    isp_check_open_return(vi_pipe);
    return ioctl(isp_get_fd(vi_pipe), ISP_SET_DETAIL_STATS_CFG, detail_stats_cfg);
#else
    isp_not_support_interface_return(vi_pipe, detail_stats_cfg);
#endif
}

td_s32 ot_mpi_isp_get_detail_stats_cfg(ot_vi_pipe vi_pipe, ot_isp_detail_stats_cfg *detail_stats_cfg)
{
#ifdef CONFIG_OT_ISP_DETAIL_STATS_SUPPORT
    isp_check_fe_pipe_return(vi_pipe);
    mpi_isp_check_pointer_return(detail_stats_cfg);
    isp_check_open_return(vi_pipe);

    return ioctl(isp_get_fd(vi_pipe), ISP_GET_DETAIL_STATS_CFG, detail_stats_cfg);
#else
    isp_not_support_interface_return(vi_pipe, detail_stats_cfg);
#endif
}

td_s32 ot_mpi_isp_get_detail_stats(ot_vi_pipe vi_pipe, ot_isp_detail_stats *detail_stats)
{
#ifdef CONFIG_OT_ISP_DETAIL_STATS_SUPPORT
    isp_detail_stats_user stats;
    isp_check_fe_pipe_return(vi_pipe);
    mpi_isp_check_pointer_return(detail_stats);
    isp_check_open_return(vi_pipe);

    stats.detail_stats = detail_stats;
    stats.ae_grid_status = detail_stats->ae_stats.ae_grid_info.status;
    stats.awb_grid_status = detail_stats->awb_stats.awb_grid_info.status;
    return ioctl(isp_get_fd(vi_pipe), ISP_GET_DETAIL_STATS, &stats);
#else
    isp_not_support_interface_return(vi_pipe, detail_stats);
#endif
}

td_s32 ot_mpi_isp_get_motion_stats(ot_vi_pipe vi_pipe, ot_isp_motion_stats *motion_stats)
{
    td_s32 ret = TD_SUCCESS;
    mpi_isp_check_pipe_return(vi_pipe);
    mpi_isp_check_pointer_return(motion_stats);
    isp_mutex_lock(vi_pipe);
    ret = isp_get_motion_stats(vi_pipe, motion_stats);
    if (ret != TD_SUCCESS) {
        isp_mutex_unlock(vi_pipe); /* if not support, return */
        return ret;
    }
    isp_mutex_unlock(vi_pipe);
    return mpi_vi_get_motion_stats(vi_pipe, motion_stats);
}
