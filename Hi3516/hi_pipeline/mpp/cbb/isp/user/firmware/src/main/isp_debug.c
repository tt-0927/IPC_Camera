/*
  Copyright (c), 2001-2025, Shenshu Tech. Co., Ltd.
 */


#include "ot_isp_debug.h"
#include "isp_ext_reg_access.h"
#include "isp_main.h"
#include "isp_ext_config.h"
#include "ot_mpi_sys_mem.h"
#include "isp_debug.h"

static td_s32 isp_dbg_check(ot_vi_pipe vi_pipe, const ot_isp_debug_info *dbg_info)
{
    td_bool debug_en = TD_FALSE;
    switch (dbg_info->debug_type) {
        case OT_ISP_DEBUG_BLC:
            debug_en = ot_ext_system_sys_debug_enable_read(vi_pipe);
            break;
        case OT_ISP_DEBUG_AE:
            debug_en = ot_ext_system_sys_ae_debug_enable_read(vi_pipe);
            break;
        case OT_ISP_DEBUG_AWB:
            debug_en = ot_ext_system_sys_awb_debug_enable_read(vi_pipe);
            break;
        case OT_ISP_DEBUG_DEHAZE:
            debug_en = ot_ext_system_sys_dehaze_debug_enable_read(vi_pipe);
            break;
        default:
            isp_err_trace("invalid debug_type.\n");
            return OT_ERR_ISP_ILLEGAL_PARAM;
    }

    if (debug_en && dbg_info->debug_en) {
        isp_err_trace("isp debug type %d has enabled debug info!\n", dbg_info->debug_type);
        return OT_ERR_ISP_ILLEGAL_PARAM;
    }

    if (dbg_info->debug_en) {
        if (dbg_info->depth == 0) {
            isp_err_trace("isp debug depth should greater than 0\n");
            return OT_ERR_ISP_ILLEGAL_PARAM;
        }

        if (dbg_info->phys_addr == TD_NULL) {
            isp_err_trace("isp debug phys_addr is NULL\n");
            return OT_ERR_ISP_ILLEGAL_PARAM;
        }
        if (ioctl(isp_get_fd(vi_pipe), ISP_VERIFY_PID) != TD_SUCCESS) {
            isp_err_trace("pipe %d  verify pid fail \n", vi_pipe);
            return TD_FAILURE;
        }
    }
    return TD_SUCCESS;
}

td_s32 isp_dbg_reg_set(ot_vi_pipe vi_pipe, const ot_isp_debug_info *dbg_info)
{
    td_s32 ret;
    isp_check_open_return(vi_pipe);
    isp_check_mem_init_return(vi_pipe);
    isp_check_vreg_permission_return(vi_pipe);
    mpi_isp_check_bool_return(dbg_info->debug_en);
    ret = isp_dbg_check(vi_pipe, dbg_info);
    if (ret != TD_SUCCESS) {
        return TD_FAILURE;
    }

    switch (dbg_info->debug_type) {
        case OT_ISP_DEBUG_BLC:
            ot_ext_system_sys_debug_enable_write(vi_pipe, dbg_info->debug_en);
            ot_ext_addr_write(ot_ext_system_sys_debug_high_addr_write,
                ot_ext_system_sys_debug_low_addr_write, vi_pipe, dbg_info->phys_addr);
            ot_ext_system_sys_debug_depth_write(vi_pipe, dbg_info->depth);
            break;
        case OT_ISP_DEBUG_AE:
            ot_ext_system_sys_ae_debug_enable_write(vi_pipe, dbg_info->debug_en);
            ot_ext_addr_write(ot_ext_system_sys_ae_debug_high_addr_write,
                ot_ext_system_sys_ae_debug_low_addr_write, vi_pipe, dbg_info->phys_addr);
            ot_ext_system_sys_ae_debug_depth_write(vi_pipe, dbg_info->depth);
            break;
        case OT_ISP_DEBUG_AWB:
            ot_ext_system_sys_awb_debug_enable_write(vi_pipe, dbg_info->debug_en);
            ot_ext_addr_write(ot_ext_system_sys_awb_debug_high_addr_write,
                ot_ext_system_sys_awb_debug_low_addr_write, vi_pipe, dbg_info->phys_addr);
            ot_ext_system_sys_awb_debug_depth_write(vi_pipe, dbg_info->depth);
            break;
        case OT_ISP_DEBUG_DEHAZE:
            ot_ext_system_sys_dehaze_debug_enable_write(vi_pipe, dbg_info->debug_en);
            ot_ext_addr_write(ot_ext_system_sys_dehaze_debug_high_addr_write,
                ot_ext_system_sys_dehaze_debug_low_addr_write, vi_pipe, dbg_info->phys_addr);
            ot_ext_system_sys_dehaze_debug_depth_write(vi_pipe, dbg_info->depth);
            break;
        default:
            isp_err_trace("invalid debug_type.\n");
            return TD_FAILURE;
    }
    ot_ext_system_sys_debug_type_write(vi_pipe, dbg_info->debug_type);
    ot_ext_system_sys_debug_update_write(vi_pipe, TD_TRUE);

    return TD_SUCCESS;
}

td_s32 isp_dbg_reg_get(ot_vi_pipe vi_pipe, ot_isp_debug_info *dbg_info)
{
    td_phys_addr_t phy_addr_temp;
    isp_check_pointer_return(dbg_info);
    isp_check_open_return(vi_pipe);
    isp_check_mem_init_return(vi_pipe);
    isp_check_vreg_permission_return(vi_pipe);

    switch (dbg_info->debug_type) {
        case OT_ISP_DEBUG_BLC:
            phy_addr_temp = ot_ext_addr_read(ot_ext_system_sys_debug_high_addr_read,
                ot_ext_system_sys_debug_low_addr_read, vi_pipe);
            dbg_info->phys_addr = phy_addr_temp;
            dbg_info->debug_en  = ot_ext_system_sys_debug_enable_read(vi_pipe);
            dbg_info->depth     = ot_ext_system_sys_debug_depth_read(vi_pipe);
            break;
        case OT_ISP_DEBUG_AE:
            phy_addr_temp = ot_ext_addr_read(ot_ext_system_sys_ae_debug_high_addr_read,
                ot_ext_system_sys_ae_debug_low_addr_read, vi_pipe);
            dbg_info->phys_addr = phy_addr_temp;
            dbg_info->debug_en  = ot_ext_system_sys_ae_debug_enable_read(vi_pipe);
            dbg_info->depth     = ot_ext_system_sys_ae_debug_depth_read(vi_pipe);
            break;
        case OT_ISP_DEBUG_AWB:
            phy_addr_temp = ot_ext_addr_read(ot_ext_system_sys_awb_debug_high_addr_read,
                ot_ext_system_sys_awb_debug_low_addr_read, vi_pipe);
            dbg_info->phys_addr = phy_addr_temp;
            dbg_info->debug_en  = ot_ext_system_sys_awb_debug_enable_read(vi_pipe);
            dbg_info->depth     = ot_ext_system_sys_awb_debug_depth_read(vi_pipe);
            break;
        case OT_ISP_DEBUG_DEHAZE:
            phy_addr_temp = ot_ext_addr_read(ot_ext_system_sys_dehaze_debug_high_addr_read,
                ot_ext_system_sys_dehaze_debug_low_addr_read, vi_pipe);
            dbg_info->phys_addr = phy_addr_temp;
            dbg_info->debug_en  = ot_ext_system_sys_dehaze_debug_enable_read(vi_pipe);
            dbg_info->depth     = ot_ext_system_sys_dehaze_debug_depth_read(vi_pipe);
            break;
        default:
            isp_err_trace("invalid debug_type.\n");
            return OT_ERR_ISP_ILLEGAL_PARAM;
    }

    return TD_SUCCESS;
}


td_s32 isp_blc_dbg_set(ot_vi_pipe vi_pipe, const ot_isp_debug_info *dbg_info)
{
    if (dbg_info->debug_en) {
        if (dbg_info->phys_addr == 0) {
            isp_err_trace("isp lib's debug phys_addr is 0!\n");
            return TD_FAILURE;
        }

        if (dbg_info->depth == 0) {
            isp_err_trace("ae lib's debug depth is 0!\n");
            return TD_FAILURE;
        }
        size_t size;
        size = sizeof(ot_isp_debug_attr) + sizeof(ot_isp_debug_status) * dbg_info->depth;
        ot_ext_system_sys_debug_enable_write(vi_pipe, dbg_info->debug_en);
        ot_ext_addr_write(ot_ext_system_sys_debug_high_addr_write,
            ot_ext_system_sys_debug_low_addr_write, vi_pipe, dbg_info->phys_addr);
        ot_ext_system_sys_debug_depth_write(vi_pipe, dbg_info->depth);
        ot_ext_system_sys_debug_size_write(vi_pipe, size);
    } else {
        ot_ext_system_sys_debug_enable_write(vi_pipe, dbg_info->debug_en);
    }
    return TD_SUCCESS;
}

td_s32 isp_blc_dbg_get(ot_vi_pipe vi_pipe, ot_isp_debug_info *dbg_info)
{
    td_phys_addr_t phy_addr_temp;

    phy_addr_temp = ot_ext_addr_read(ot_ext_system_sys_debug_high_addr_read,
        ot_ext_system_sys_debug_low_addr_read, vi_pipe);
    dbg_info->phys_addr = phy_addr_temp;
    dbg_info->debug_en = ot_ext_system_sys_debug_enable_read(vi_pipe);
    dbg_info->depth    = ot_ext_system_sys_debug_depth_read(vi_pipe);

    return TD_SUCCESS;
}

td_s32 isp_blc_dbg_run_bgn(isp_dbg_ctrl *dbg, td_u32 frm_cnt)
{
    ot_isp_debug_status  *dbg_status = TD_NULL;

    if (!dbg->debug_en) {
        if (dbg->dbg_attr != TD_NULL) {
            ot_mpi_sys_munmap(dbg->dbg_attr, dbg->size);
            dbg->dbg_attr = TD_NULL;
            dbg->dbg_status = TD_NULL;
        }
        return TD_SUCCESS;
    }

    if ((dbg->debug_en) && (dbg->dbg_attr == TD_NULL)) {
        dbg->dbg_attr = (ot_isp_debug_attr *)ot_mpi_sys_mmap(dbg->phy_addr, dbg->size);
        if (dbg->dbg_attr == TD_NULL) {
            isp_err_trace("isp map debug buf failed!\n");
            return TD_FAILURE;
        }
        dbg->dbg_status = (ot_isp_debug_status *)(dbg->dbg_attr + 1);
    }

    dbg_status = dbg->dbg_status + (frm_cnt % div_0_to_1(dbg->depth));

    dbg_status->frame_num_begain = frm_cnt;

    return TD_SUCCESS;
}

td_s32 isp_blc_dbg_run_end(ot_vi_pipe vi_pipe, isp_dbg_ctrl *dbg, td_u32 frm_cnt)
{
    ot_isp_debug_status *dbg_status = TD_NULL;

    if ((!dbg->debug_en) || (dbg->dbg_status == TD_NULL)) {
        return TD_SUCCESS;
    }
    dbg_status = dbg->dbg_status + (frm_cnt % div_0_to_1(dbg->depth));
    /* record status */
    dbg_status->frame_num_end = frm_cnt;

    isp_black_level_actual_value_read(vi_pipe, &dbg_status->black_level_actual[0]);
#ifdef CONFIG_OT_ISP_DYNAMIC_BLC_SUPPORT
    ori_black_level_actual_value_read(vi_pipe, &dbg_status->black_level_original[0]);
#endif
    return TD_SUCCESS;
}

td_s32 isp_blc_dbg_exit(ot_vi_pipe vi_pipe, isp_dbg_ctrl *dbg)
{
    if (dbg->dbg_attr != TD_NULL) {
        ot_mpi_sys_munmap(dbg->dbg_attr, dbg->size);
        dbg->dbg_attr = TD_NULL;
        dbg->dbg_status = TD_NULL;
    }
    return TD_SUCCESS;
}

td_s32 isp_ae_dbg_set(ot_vi_pipe vi_pipe, ot_isp_debug_info *dbg_info)
{
    isp_usr_ctx *isp_ctx = TD_NULL;
    td_u32 i;

    isp_get_ctx(vi_pipe, isp_ctx);
    for (i = 0; i < ISP_MAX_ALGS_NUM; i++) {
        if (isp_ctx->algs[i].used == TD_TRUE && isp_ctx->algs[i].alg_type == OT_ISP_ALG_AE) {
            if (isp_ctx->algs[i].alg_func.pfn_alg_ctrl != TD_NULL) {
                isp_ctx->algs[i].alg_func.pfn_alg_ctrl(vi_pipe, OT_ISP_AE_DEBUG_ATTR_SET, dbg_info);
                return TD_SUCCESS;
            }
        }
    }
    return TD_FAILURE;
}

td_s32 isp_awb_dbg_set(ot_vi_pipe vi_pipe, ot_isp_debug_info *dbg_info)
{
    isp_usr_ctx *isp_ctx = TD_NULL;
    td_u32 i;

    isp_get_ctx(vi_pipe, isp_ctx);
    for (i = 0; i < ISP_MAX_ALGS_NUM; i++) {
        if (isp_ctx->algs[i].used == TD_TRUE && isp_ctx->algs[i].alg_type == OT_ISP_ALG_AWB) {
            if (isp_ctx->algs[i].alg_func.pfn_alg_ctrl != TD_NULL) {
                isp_ctx->algs[i].alg_func.pfn_alg_ctrl(vi_pipe, OT_ISP_AWB_DEBUG_ATTR_SET, dbg_info);
                return TD_SUCCESS;
            }
        }
    }
    return TD_FAILURE;
}

td_s32 isp_dehaze_dbg_set(ot_vi_pipe vi_pipe, ot_isp_debug_info *dbg_info)
{
    isp_usr_ctx *isp_ctx = TD_NULL;
    td_u32 i;

    isp_get_ctx(vi_pipe, isp_ctx);
    for (i = 0; i < ISP_MAX_ALGS_NUM; i++) {
        if (isp_ctx->algs[i].used == TD_TRUE && isp_ctx->algs[i].alg_type == OT_ISP_ALG_DEHAZE) {
            if (isp_ctx->algs[i].alg_func.pfn_alg_ctrl != TD_NULL) {
                isp_ctx->algs[i].alg_func.pfn_alg_ctrl(vi_pipe, OT_ISP_DEHAZE_DEBUG_ATTR_SET, dbg_info);
                return TD_SUCCESS;
            }
        }
    }
    return TD_FAILURE;
}

