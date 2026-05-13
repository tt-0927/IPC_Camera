/*
  Copyright (c), 2001-2025, Shenshu Tech. Co., Ltd.
 */

#include "isp_be_buf.h"
#include "isp_drv_define.h"
#include "isp_drv.h"
#include "isp_list.h"

static td_u32 g_isp_exit_timeout = 2000;     /* The time(unit:ms) of exit be buffer timeout */

#ifdef CONFIG_OT_ISP_DETAIL_STATS_SUPPORT
static td_u64 isp_drv_get_extend_cfg_size(isp_drv_ctx *drv_ctx)
{
    td_u8 blk_num;
    td_u64 extend_cfg_size = 0;

    blk_num = drv_ctx->detail_stats_cfg.col * drv_ctx->detail_stats_cfg.row;
    if (drv_ctx->detail_stats_cfg.ctrl.bit1_ae) {
        extend_cfg_size += ISP_DETAIL_STATS_AE_CFG_SIZE;
    }
    if (drv_ctx->detail_stats_cfg.ctrl.bit1_awb) {
        extend_cfg_size += ISP_DETAIL_STATS_AWB_CFG_SIZE;
    }
    extend_cfg_size = extend_cfg_size * blk_num;
    return extend_cfg_size;
}
#endif

static td_s32 isp_drv_be_buf_malloc(ot_vi_pipe vi_pipe, isp_mmz_buf_ex *be_buf_temp, td_u64 *extend_size)
{
    td_u8 be_buf_num;
    td_s32 ret;
    isp_drv_ctx *drv_ctx = TD_NULL;
    td_phys_addr_t phy_addr;
    td_u64 size, extend_cfg_size;
    td_u8 *vir_addr = TD_NULL;
    td_char ac_name[MAX_MMZ_NAME_LEN] = {0};
    unsigned long flags = 0;
    osal_spinlock *isp_spin_lock = TD_NULL;
    mm_malloc_param malloc_param = {0};

    isp_check_pipe_return(vi_pipe);
    drv_ctx = isp_drv_get_ctx(vi_pipe);

    ret = snprintf_s(ac_name, sizeof(ac_name), sizeof(ac_name) - 1, "isp[%d].be_cfg", vi_pipe);
    if (ret < 0) {
        return TD_FAILURE;
    }

    size = sizeof(isp_be_wo_reg_cfg);

    extend_cfg_size = 0;
#ifdef CONFIG_OT_ISP_DETAIL_STATS_SUPPORT
    if (drv_ctx->detail_stats_cfg.enable) {
        extend_cfg_size = isp_drv_get_extend_cfg_size(drv_ctx);
    }
    size += extend_cfg_size;
#endif
    be_buf_num = isp_drv_get_be_buf_num(vi_pipe);

    malloc_param.buf_name = ac_name;
    malloc_param.size = size * be_buf_num;
    malloc_param.kernel_only = TD_FALSE;
    ret = cmpi_mmz_malloc_cached(&malloc_param, &phy_addr, (td_void **)&vir_addr);
    if (ret != TD_SUCCESS) {
        isp_err_trace("Pipe[%d] alloc ISP BeCfgBuf err!\n", vi_pipe);
        return OT_ERR_ISP_NOMEM;
    }

    (td_void)memset_s(vir_addr, malloc_param.size, 0, malloc_param.size);
    isp_spin_lock = isp_drv_get_lock(vi_pipe);
    osal_spin_lock_irqsave(isp_spin_lock, &flags);

    drv_ctx->be_buf_info.init = TD_TRUE;
    drv_ctx->be_buf_info.be_buf_haddr.phy_addr = phy_addr;
    drv_ctx->be_buf_info.be_buf_haddr.vir_addr = (td_void *)vir_addr;
    drv_ctx->be_buf_info.be_buf_haddr.size = malloc_param.size; // all_size : size * be_buf_num

    osal_spin_unlock_irqrestore(isp_spin_lock, &flags);

    be_buf_temp->phy_addr = phy_addr;
    be_buf_temp->vir_addr = (td_void *)vir_addr;
    be_buf_temp->size = size;
    *extend_size = extend_cfg_size;
    return TD_SUCCESS;
}

static td_void isp_drv_be_buf_info_reset(isp_drv_ctx *drv_ctx)
{
    drv_ctx->be_buf_info.init = TD_FALSE;
    drv_ctx->be_buf_info.be_buf_haddr.phy_addr = 0;
    drv_ctx->be_buf_info.be_buf_haddr.vir_addr = TD_NULL;
    drv_ctx->be_buf_info.be_buf_haddr.size = 0;
    return;
}

td_s32 isp_drv_be_buf_init(ot_vi_pipe vi_pipe, isp_mmz_buf_ex *be_cfg_buf_info)
{
    td_u8 be_buf_num;
    td_s32 ret, i;
    isp_drv_ctx *drv_ctx = TD_NULL;
    isp_be_buf_node *node = TD_NULL;
    td_u64 extend_size;
    isp_mmz_buf_ex be_buf_temp = {0};
    unsigned long flags = 0;
    osal_spinlock *isp_spin_lock = TD_NULL;

    isp_check_pipe_return(vi_pipe);
    isp_check_pointer_return(be_cfg_buf_info);

    drv_ctx = isp_drv_get_ctx(vi_pipe);
    isp_check_online_mode_return(vi_pipe, drv_ctx->work_mode.running_mode);

    isp_check_buf_init_return(vi_pipe, drv_ctx->be_buf_info.init);

    ret = isp_drv_be_buf_malloc(vi_pipe, &be_buf_temp, &extend_size);
    isp_check_return(vi_pipe, ret, "isp_drv_be_buf_malloc");
    be_buf_num = isp_drv_get_be_buf_num(vi_pipe);
    ret = isp_creat_be_buf_queue(&drv_ctx->be_buf_queue, be_buf_num);
    isp_check_ret_goto(ret, ret, fail0, "vi_pipe[%d] creat be buf queue fail!\n", vi_pipe);

    isp_spin_lock = isp_drv_get_lock(vi_pipe);
    osal_spin_lock_irqsave(isp_spin_lock, &flags);

    for (i = 0; i < be_buf_num; i++) {
        node = isp_queue_get_free_be_buf(&drv_ctx->be_buf_queue);
        if (node == TD_NULL) {
            isp_err_trace("vi_pipe[%d] queue get free be buf fail!\r\n", vi_pipe);
            osal_spin_unlock_irqrestore(isp_spin_lock, &flags);
            goto fail1;
        }
        isp_drv_be_cfg_buf_addr_init(vi_pipe, node, i, &be_buf_temp, extend_size);
        isp_queue_put_free_be_buf(&drv_ctx->be_buf_queue, node);
    }

    drv_ctx->use_node = TD_NULL;
    drv_ctx->running_state = ISP_BE_BUF_STATE_INIT;
    drv_ctx->exit_state = ISP_BE_BUF_READY;

    be_cfg_buf_info->phy_addr = drv_ctx->be_buf_info.be_buf_haddr.phy_addr;
    be_cfg_buf_info->size = drv_ctx->be_buf_info.be_buf_haddr.size; // all_size : size * be_buf_num
    be_cfg_buf_info->vir_addr = TD_NULL;
    osal_spin_unlock_irqrestore(isp_spin_lock, &flags);

    return TD_SUCCESS;

fail1:
    isp_destroy_be_buf_queue(&drv_ctx->be_buf_queue);

fail0:
    isp_drv_be_buf_info_reset(drv_ctx);

    if (be_buf_temp.phy_addr != 0) {
        cmpi_mmz_free(be_buf_temp.phy_addr, (td_void *)be_buf_temp.vir_addr);
    }

    return TD_FAILURE;
}

static td_s32 isp_drv_wait_exit_callback(const td_void *param)
{
    td_s32 condition;

    condition = *(td_s32 *)param;

    return (condition == 0);
}

td_s32 isp_drv_be_buf_exit(ot_vi_pipe vi_pipe)
{
    td_s32 ret;
    td_phys_addr_t phy_addr;
    td_void *vir_addr = TD_NULL;
    isp_drv_ctx *drv_ctx = TD_NULL;
    unsigned long flags = 0;
    osal_spinlock *isp_spin_lock = TD_NULL;

    isp_check_pipe_return(vi_pipe);

    drv_ctx = isp_drv_get_ctx(vi_pipe);
    isp_check_online_mode_return(vi_pipe, drv_ctx->work_mode.running_mode);
    isp_check_exit_state_return(vi_pipe, drv_ctx->isp_run_flag);

    isp_check_buf_exit_return(vi_pipe, drv_ctx->be_buf_info.init);

    isp_spin_lock = isp_drv_get_lock(vi_pipe);
    osal_spin_lock_irqsave(isp_spin_lock, &flags);
    drv_ctx->exit_state = ISP_BE_BUF_WAITING;
    osal_spin_unlock_irqrestore(isp_spin_lock, &flags);
    if (check_func_entry(OT_ID_VI) && ckfn_vi_update_vi_vpss_mode()) {
        /* Note: this function cannot be placed in the ISP lock, otherwise it will be deadlocked. */
        call_vi_isp_clear_input_queue(vi_pipe);
    }

    ret = osal_wait_timeout_uninterruptible(&drv_ctx->isp_exit_wait, isp_drv_wait_exit_callback,
        &drv_ctx->be_buf_info.use_cnt, g_isp_exit_timeout);
    if (ret <= 0) {
        isp_err_trace("Pipe:%d isp exit wait failed:ret:%d!\n", vi_pipe, ret);
        return TD_FAILURE;
    }

    osal_spin_lock_irqsave(isp_spin_lock, &flags);
    phy_addr = drv_ctx->be_buf_info.be_buf_haddr.phy_addr;
    vir_addr = drv_ctx->be_buf_info.be_buf_haddr.vir_addr;
    isp_destroy_be_buf_queue(&drv_ctx->be_buf_queue);
    isp_drv_be_buf_info_reset(drv_ctx);
    drv_ctx->exit_state = ISP_BE_BUF_EXIT;

    osal_spin_unlock_irqrestore(isp_spin_lock, &flags);

    if (phy_addr != 0) {
        cmpi_mmz_free(phy_addr, vir_addr);
    }

    return TD_SUCCESS;
}

static td_s32 isp_drv_write_all_ldci_stt_addr(ot_vi_pipe vi_pipe)
{
    td_u8 k, write_buf_idx, free_num, write_buf_num;
    td_phys_addr_t write_stt_head_addr;
    isp_be_wo_reg_cfg *be_reg_cfg = TD_NULL;
    isp_be_buf_node *node = TD_NULL;
    isp_drv_ctx *drv_ctx = TD_NULL;

    isp_check_pipe_return(vi_pipe);
    drv_ctx = isp_drv_get_ctx(vi_pipe);

    write_buf_num = drv_ctx->ldci_write_buf_attr.buf_num;
    write_buf_idx = drv_ctx->ldci_write_buf_attr.buf_idx;
    write_stt_head_addr = drv_ctx->ldci_write_buf_attr.ldci_buf[write_buf_idx].phy_addr;

    be_reg_cfg = (isp_be_wo_reg_cfg *)drv_ctx->use_node->be_cfg_buf.vir_addr;

    isp_drv_set_ldci_blk_write_addr(drv_ctx, be_reg_cfg, write_stt_head_addr);
    drv_ctx->ldci_write_buf_attr.buf_idx = (write_buf_idx + 1) % div_0_to_1(write_buf_num);

    free_num = isp_queue_get_free_num(&drv_ctx->be_buf_queue);

    for (k = 0; k < free_num; k++) {
        node = isp_queue_get_free_be_buf(&drv_ctx->be_buf_queue);
        if (node == TD_NULL) {
            isp_err_trace("ISP[%d] Get QueueGetFreeBeBuf fail!\r\n", vi_pipe);
            return TD_FAILURE;
        }

        be_reg_cfg = (isp_be_wo_reg_cfg *)node->be_cfg_buf.vir_addr;
        write_buf_idx = drv_ctx->ldci_write_buf_attr.buf_idx;
        write_stt_head_addr = drv_ctx->ldci_write_buf_attr.ldci_buf[write_buf_idx].phy_addr;

        isp_drv_set_ldci_blk_write_addr(drv_ctx, be_reg_cfg, write_stt_head_addr);
        drv_ctx->ldci_write_buf_attr.buf_idx = (write_buf_idx + 1) % div_0_to_1(write_buf_num);
        isp_queue_put_free_be_buf(&drv_ctx->be_buf_queue, node);
    }

    return TD_SUCCESS;
}
#ifdef CONFIG_OT_ISP_DETAIL_STATS_SUPPORT
static td_void isp_drv_copy_extend_to_free_buf(isp_drv_ctx *drv_ctx, isp_be_buf_node *node)
{
    td_s32 ret;
    if (drv_ctx->detail_stats_cfg.enable) {
        ret = memcpy_s((td_u8 *)node->be_cfg_buf.extend_vir_addr, node->be_cfg_buf.extend_size,
            (td_u8 *)drv_ctx->use_node->be_cfg_buf.extend_vir_addr, drv_ctx->use_node->be_cfg_buf.extend_size);
        isp_check_eok_void(ret);
    }
}
#endif
static td_s32 isp_drv_write_be_free_buf(ot_vi_pipe vi_pipe)
{
    td_s32 i, free_num, ret;
    isp_running_mode running_mode;
    isp_be_buf_node *node = TD_NULL;
    isp_be_wo_reg_cfg *be_reg_cfg_src = TD_NULL;
    isp_be_wo_reg_cfg *be_reg_cfg_dst = TD_NULL;
    isp_drv_ctx *drv_ctx = TD_NULL;
    td_phys_addr_t phy_addr;
    td_u64 size;
    td_void *vir_addr = TD_NULL;

    isp_check_pipe_return(vi_pipe);

    drv_ctx = isp_drv_get_ctx(vi_pipe);
    isp_check_bebuf_init_return(vi_pipe, drv_ctx->be_buf_info.init);

    if (drv_ctx->use_node == TD_NULL) {
        isp_err_trace("Pipe[%d] pstCurNode is null for init!\r\n", vi_pipe);
        return TD_FAILURE;
    }

    be_reg_cfg_src = drv_ctx->use_node->be_cfg_buf.vir_addr;
    running_mode = drv_ctx->work_mode.running_mode;

    free_num = isp_queue_get_free_num(&drv_ctx->be_buf_queue);

    for (i = 0; i < free_num; i++) {
        node = isp_queue_get_free_be_buf(&drv_ctx->be_buf_queue);
        if (node == TD_NULL) {
            isp_err_trace("Pipe[%d] Get QueueGetFreeBeBuf fail!\r\n", vi_pipe);
            return TD_FAILURE;
        }

        be_reg_cfg_dst = (isp_be_wo_reg_cfg *)node->be_cfg_buf.vir_addr;

        if ((running_mode == ISP_MODE_RUNNING_SIDEBYSIDE) || (running_mode == ISP_MODE_RUNNING_STRIPING)) {
            (td_void)memcpy_s(be_reg_cfg_dst, sizeof(isp_be_wo_reg_cfg), be_reg_cfg_src, sizeof(isp_be_wo_reg_cfg));
        } else {
            (td_void)memcpy_s(&be_reg_cfg_dst->be_reg_cfg[0], sizeof(isp_be_all_reg_type),
                &be_reg_cfg_src->be_reg_cfg[0], sizeof(isp_be_all_reg_type));
        }
#ifdef CONFIG_OT_ISP_DETAIL_STATS_SUPPORT
        isp_drv_copy_extend_to_free_buf(drv_ctx, node);
#endif
        phy_addr = drv_ctx->use_node->be_cfg_buf.phy_addr;
        vir_addr = drv_ctx->use_node->be_cfg_buf.vir_addr;
        size = drv_ctx->use_node->be_cfg_buf.size;

        cmpi_dcache_region_wb(vir_addr, phy_addr, size);

        isp_queue_put_free_be_buf(&drv_ctx->be_buf_queue, node);
    }

    ret = isp_drv_write_all_ldci_stt_addr(vi_pipe);
    if (ret != TD_SUCCESS) {
        return TD_FAILURE;
    }

    return TD_SUCCESS;
}

#ifdef CONFIG_OT_VI_STITCH_GRP
static td_s32 isp_drv_stitch_sync_ex(ot_vi_pipe vi_pipe)
{
    td_u8 k;
    ot_vi_pipe vi_pipe_id;
    isp_drv_ctx *drv_ctx = TD_NULL;
    isp_drv_ctx *drv_ctx_s = TD_NULL;

    drv_ctx = isp_drv_get_ctx(vi_pipe);

    for (k = 0; k < drv_ctx->stitch_attr.stitch_pipe_num; k++) {
        vi_pipe_id = drv_ctx->stitch_attr.stitch_bind_id[k];
        drv_ctx_s = isp_drv_get_ctx(vi_pipe_id);
        if (drv_ctx_s->stitch_sync != TD_TRUE) {
            return TD_FAILURE;
        }
    }

    return TD_SUCCESS;
}

static td_s32 isp_drv_stitch_write_be_buf_all(ot_vi_pipe vi_pipe)
{
    td_s32 i, ret;
    ot_vi_pipe vi_pipes, main_pipe;
    isp_drv_ctx *drv_ctx = TD_NULL;
    isp_drv_ctx *drv_ctx_s = TD_NULL;
    unsigned long flags = 0;
    osal_spinlock *isp_sync_lock = TD_NULL;

    isp_check_pipe_return(vi_pipe);

    drv_ctx = isp_drv_get_ctx(vi_pipe);
    main_pipe = drv_ctx->stitch_attr.stitch_bind_id[0];

    ret = isp_drv_write_be_free_buf(vi_pipe);
    isp_check_return(vi_pipe, ret, "isp_drv_write_be_free_buf");
    isp_sync_lock = isp_drv_get_sync_lock(main_pipe);
    osal_spin_lock_irqsave(isp_sync_lock, &flags);

    ret = isp_drv_stitch_sync_ex(vi_pipe);
    if (ret != TD_SUCCESS) {
        osal_spin_unlock_irqrestore(isp_sync_lock, &flags);

        return TD_SUCCESS;
    }

    if (drv_ctx->running_state == ISP_BE_BUF_STATE_SWITCH_START) {
        drv_ctx->running_state = ISP_BE_BUF_STATE_SWITCH;
        for (i = 0; i < drv_ctx->stitch_attr.stitch_pipe_num; i++) {
            vi_pipes = drv_ctx->stitch_attr.stitch_bind_id[i];
            drv_ctx_s = isp_drv_get_ctx(vi_pipes);
            if (drv_ctx_s->running_state != ISP_BE_BUF_STATE_SWITCH) {
                osal_spin_unlock_irqrestore(isp_sync_lock, &flags);
                drv_ctx->running_state = ISP_BE_BUF_STATE_SWITCH_START;
                return TD_SUCCESS;
            }
        }
        drv_ctx->running_state = ISP_BE_BUF_STATE_SWITCH_START;
    }

    for (i = 0; i < drv_ctx->stitch_attr.stitch_pipe_num; i++) {
        vi_pipes = drv_ctx->stitch_attr.stitch_bind_id[i];
        drv_ctx_s = isp_drv_get_ctx(vi_pipes);
        if ((drv_ctx_s->be_buf_info.init != TD_TRUE) || (drv_ctx_s->use_node == TD_NULL)) {
            osal_spin_unlock_irqrestore(isp_sync_lock, &flags);
            isp_err_trace("Pipe[%d] BeBuf (bInit != TRUE) or use_node is TD_NULL!\n", vi_pipe);
            return TD_FAILURE;
        }

        isp_queue_put_busy_be_buf(&drv_ctx_s->be_buf_queue, drv_ctx_s->use_node);
        drv_ctx_s->use_node = TD_NULL;
        drv_ctx_s->running_state = ISP_BE_BUF_STATE_INIT;
    }

    osal_spin_unlock_irqrestore(isp_sync_lock, &flags);

    return TD_SUCCESS;
}
#endif

td_s32 isp_drv_all_be_buf_init(ot_vi_pipe vi_pipe)
{
    td_s32 ret;
    isp_drv_ctx *drv_ctx = TD_NULL;
    unsigned long flags = 0;
    osal_spinlock *isp_spin_lock = TD_NULL;

    isp_check_pipe_return(vi_pipe);

    drv_ctx = isp_drv_get_ctx(vi_pipe);
    isp_check_online_mode_return(vi_pipe, drv_ctx->work_mode.running_mode);

    if (drv_ctx->stitch_attr.stitch_enable != TD_TRUE) {
        if (drv_ctx->be_buf_info.init != TD_TRUE) {
            isp_err_trace("Pipe[%d] BeBuf (bInit != TRUE) !\n", vi_pipe);
            return TD_FAILURE;
        }
        isp_spin_lock = isp_drv_get_lock(vi_pipe);
        osal_spin_lock_irqsave(isp_spin_lock, &flags);

        ret = isp_drv_write_be_free_buf(vi_pipe);
        if (ret != TD_SUCCESS) {
            osal_spin_unlock_irqrestore(isp_spin_lock, &flags);

            isp_err_trace("Pipe[%d] ISP_DRV_WriteBeFreeBuf fail!\n", vi_pipe);
            return ret;
        }

        isp_queue_put_busy_be_buf(&drv_ctx->be_buf_queue, drv_ctx->use_node);
        drv_ctx->use_node = TD_NULL;

        osal_spin_unlock_irqrestore(isp_spin_lock, &flags);
    } else {
#ifdef CONFIG_OT_VI_STITCH_GRP
        ret = isp_drv_stitch_write_be_buf_all(vi_pipe);
        if (ret != TD_SUCCESS) {
            isp_err_trace("Pipe[%d] ISP_DRV_StitchWriteBeBufAll fail!\n", vi_pipe);
            return ret;
        }
#endif
    }

    return TD_SUCCESS;
}

td_s32 isp_drv_get_be_buf_first(ot_vi_pipe vi_pipe, td_phys_addr_t *point_phy_addr)
{
    unsigned long flags = 0;
    isp_drv_ctx *drv_ctx = TD_NULL;
    isp_be_buf_node *node = TD_NULL;
    osal_spinlock *isp_spin_lock = TD_NULL;

    isp_check_pipe_return(vi_pipe);
    isp_check_pointer_return(point_phy_addr);

    drv_ctx = isp_drv_get_ctx(vi_pipe);
    isp_check_online_mode_return(vi_pipe, drv_ctx->work_mode.running_mode);
    isp_check_bebuf_init_return(vi_pipe, drv_ctx->be_buf_info.init);
    isp_spin_lock = isp_drv_get_lock(vi_pipe);
    osal_spin_lock_irqsave(isp_spin_lock, &flags);

    node = isp_queue_get_free_be_buf(&drv_ctx->be_buf_queue);
    if (node == TD_NULL) {
        osal_spin_unlock_irqrestore(isp_spin_lock, &flags);

        isp_err_trace("Pipe[%d] Get FreeBeBuf to user fail!\r\n", vi_pipe);
        return TD_FAILURE;
    }
    osal_spin_unlock_irqrestore(isp_spin_lock, &flags);

    if (ot_mmz_check_phys_addr(node->be_cfg_buf.phy_addr, sizeof(isp_be_wo_reg_cfg)) != TD_SUCCESS) {
        isp_err_trace("be buf addr check error\n");
        osal_spin_lock_irqsave(isp_spin_lock, &flags);
        isp_queue_put_free_be_buf(&drv_ctx->be_buf_queue, node);
        osal_spin_unlock_irqrestore(isp_spin_lock, &flags);

        return TD_FAILURE;
    }

    osal_spin_lock_irqsave(isp_spin_lock, &flags);
    drv_ctx->use_node = node;
    *point_phy_addr = drv_ctx->use_node->be_cfg_buf.phy_addr;
    osal_spin_unlock_irqrestore(isp_spin_lock, &flags);

    return TD_SUCCESS;
}

td_s32 isp_drv_query_be_free_buf(ot_vi_pipe vi_pipe, isp_be_wo_cfg_buf *be_wo_cfg_buf)
{
    osal_spinlock *isp_spin_lock = TD_NULL;
    unsigned long flags = 0;
    isp_drv_ctx *drv_ctx = TD_NULL;
    isp_be_wo_cfg_buf *cur_node_buf = TD_NULL;

    isp_check_pipe_return(vi_pipe);
    isp_check_pointer_return(be_wo_cfg_buf);

    drv_ctx = isp_drv_get_ctx(vi_pipe);
    isp_check_online_mode_return(vi_pipe, drv_ctx->work_mode.running_mode);
    isp_check_bebuf_init_return(vi_pipe, drv_ctx->be_buf_info.init);

    isp_spin_lock = isp_drv_get_spin_lock(vi_pipe);

    osal_spin_lock_irqsave(isp_spin_lock, &flags);

    if (drv_ctx->use_node == TD_NULL) {
        osal_spin_unlock_irqrestore(isp_spin_lock, &flags);
        return TD_FAILURE;
    }

    cur_node_buf = &drv_ctx->use_node->be_cfg_buf;
    (td_void)memcpy_s(be_wo_cfg_buf, sizeof(isp_be_wo_cfg_buf), cur_node_buf, sizeof(isp_be_wo_cfg_buf));
    osal_spin_unlock_irqrestore(isp_spin_lock, &flags);

    if (ot_mmz_check_phys_addr(cur_node_buf->phy_addr, cur_node_buf->size) != TD_SUCCESS) {
        isp_err_trace("ISP[%d] be buf check error\n", vi_pipe);
        (td_void)memset_s(be_wo_cfg_buf, sizeof(isp_be_wo_cfg_buf), 0, sizeof(isp_be_wo_cfg_buf));
        return TD_FAILURE;
    }

    return TD_SUCCESS;
}

td_s32 isp_drv_get_free_be_buf(ot_vi_pipe vi_pipe)
{
    isp_drv_ctx *drv_ctx = isp_drv_get_ctx(vi_pipe);
    isp_check_pointer_return(drv_ctx);

    drv_ctx->use_node = isp_queue_get_free_be_buf(&drv_ctx->be_buf_queue);
    if (drv_ctx->use_node == TD_NULL) {
        return TD_FAILURE;
    }

    return TD_SUCCESS;
}

td_s32 isp_drv_get_be_last_buf(ot_vi_pipe vi_pipe, td_phys_addr_t *point_phy_addr)
{
    td_u8 i;
    isp_drv_ctx *drv_ctx = TD_NULL;
    isp_be_buf_node *node = TD_NULL;
    isp_be_wo_reg_cfg *be_reg_cfg_dst = TD_NULL;
    struct osal_list_head *list_tmp = TD_NULL;
    struct osal_list_head *list_node = TD_NULL;
    unsigned long flags = 0;
    osal_spinlock *isp_spin_lock = TD_NULL;

    isp_check_pipe_return(vi_pipe);
    isp_check_pointer_return(point_phy_addr);

    drv_ctx = isp_drv_get_ctx(vi_pipe);
    isp_check_online_mode_return(vi_pipe, drv_ctx->work_mode.running_mode);
    isp_check_bebuf_init_return(vi_pipe, drv_ctx->be_buf_info.init);

    isp_spin_lock = isp_drv_get_spin_lock(vi_pipe);

    osal_spin_lock_irqsave(isp_spin_lock, &flags);

    osal_list_for_each_safe(list_node, list_tmp, &drv_ctx->be_buf_queue.busy_list)
    {
        node = osal_list_entry(list_node, isp_be_buf_node, list);

        node->hold_cnt = 0;

        isp_queue_del_busy_be_buf(&drv_ctx->be_buf_queue, node);
        isp_queue_put_free_be_buf(&drv_ctx->be_buf_queue, node);
    }

    if (drv_ctx->use_node == TD_NULL) {
        node = isp_queue_get_free_be_buf_tail(&drv_ctx->be_buf_queue);
        if (node == TD_NULL) {
            osal_spin_unlock_irqrestore(isp_spin_lock, &flags);
            isp_err_trace("Pipe[%d] Get LastBeBuf fail!\r\n", vi_pipe);
            return TD_FAILURE;
        }

        drv_ctx->use_node = node;
    }

    be_reg_cfg_dst = (isp_be_wo_reg_cfg *)drv_ctx->use_node->be_cfg_buf.vir_addr;

    for (i = drv_ctx->work_mode.pre_block_num; i < drv_ctx->work_mode.block_num; i++) {
        (td_void)memcpy_s(&be_reg_cfg_dst->be_reg_cfg[i], sizeof(isp_be_all_reg_type), &be_reg_cfg_dst->be_reg_cfg[0],
            sizeof(isp_be_all_reg_type));
    }
    *point_phy_addr = drv_ctx->use_node->be_cfg_buf.phy_addr;
    osal_spin_unlock_irqrestore(isp_spin_lock, &flags);

    if (ot_mmz_check_phys_addr(drv_ctx->use_node->be_cfg_buf.phy_addr,
        drv_ctx->use_node->be_cfg_buf.size) != TD_SUCCESS) {
        *point_phy_addr = TD_NULL;
        isp_err_trace("be buf check error \n");
        return TD_FAILURE;
    }

    return TD_SUCCESS;
}

#ifdef CONFIG_OT_VI_STITCH_GRP
td_s32 isp_drv_be_buf_run_state(ot_vi_pipe vi_pipe)
{
    isp_drv_ctx *drv_ctx = TD_NULL;
    unsigned long flags = 0;
    osal_spinlock *isp_spin_lock = TD_NULL;
    isp_check_pipe_return(vi_pipe);

    drv_ctx = isp_drv_get_ctx(vi_pipe);
    isp_check_online_mode_return(vi_pipe, drv_ctx->work_mode.running_mode);
    isp_check_bebuf_init_return(vi_pipe, drv_ctx->be_buf_info.init);

    isp_spin_lock = isp_drv_get_spin_lock(vi_pipe);

    osal_spin_lock_irqsave(isp_spin_lock, &flags);

    if (drv_ctx->stitch_attr.stitch_enable == TD_TRUE) {
        if (drv_ctx->running_state != ISP_BE_BUF_STATE_INIT) {
            osal_spin_unlock_irqrestore(isp_spin_lock, &flags);

            isp_warn_trace("Pipe[%d] isp isn't init state!\n", vi_pipe);
            return TD_FAILURE;
        }

        drv_ctx->running_state = ISP_BE_BUF_STATE_RUNNING;
    }

    osal_spin_unlock_irqrestore(isp_spin_lock, &flags);

    return TD_SUCCESS;
}
#endif

td_s32 isp_drv_be_buf_switch_state(ot_vi_pipe vi_pipe)
{
#ifdef CONFIG_OT_VI_STITCH_GRP
    isp_drv_ctx *drv_ctx = TD_NULL;
    unsigned long flags = 0;
    osal_spinlock *isp_spin_lock = TD_NULL;

    drv_ctx = isp_drv_get_ctx(vi_pipe);

    isp_spin_lock = isp_drv_get_spin_lock(vi_pipe);

    osal_spin_lock_irqsave(isp_spin_lock, &flags);

    if (drv_ctx->stitch_attr.stitch_enable == TD_TRUE) {
        drv_ctx->running_state = ISP_BE_BUF_STATE_SWITCH_START;
    }

    osal_spin_unlock_irqrestore(isp_spin_lock, &flags);
#endif
    return TD_SUCCESS;
}

td_s32 isp_drv_be_buf_switch_finish_state(ot_vi_pipe vi_pipe)
{
#ifdef CONFIG_OT_VI_STITCH_GRP
    td_s32 i;
    ot_vi_pipe vi_pipes;
    isp_drv_ctx *drv_ctx_s = TD_NULL;
    isp_drv_ctx *drv_ctx = TD_NULL;
    unsigned long flags = 0;
    osal_spinlock *isp_spin_lock = TD_NULL;

    isp_check_pipe_return(vi_pipe);

    drv_ctx = isp_drv_get_ctx(vi_pipe);

    isp_spin_lock = isp_drv_get_spin_lock(vi_pipe);
    osal_spin_lock_irqsave(isp_spin_lock, &flags);

    if (drv_ctx->stitch_attr.stitch_enable == TD_TRUE) {
        if (drv_ctx->running_state != ISP_BE_BUF_STATE_SWITCH_START) {
            osal_spin_unlock_irqrestore(isp_spin_lock, &flags);
            isp_warn_trace("Pipe[%d] isp isn't init state!\n", vi_pipe);
            return TD_FAILURE;
        }

        drv_ctx->running_state = ISP_BE_BUF_STATE_SWITCH;

        for (i = 0; i < drv_ctx->stitch_attr.stitch_pipe_num; i++) {
            vi_pipes = drv_ctx->stitch_attr.stitch_bind_id[i];
            drv_ctx_s = isp_drv_get_ctx(vi_pipes);
            if (drv_ctx_s->running_state != ISP_BE_BUF_STATE_SWITCH) {
                osal_spin_unlock_irqrestore(isp_spin_lock, &flags);
                isp_warn_trace("Pipe[%d] isp isn't  finish state!\n", vi_pipe);
                return TD_FAILURE;
            }
        }

        for (i = 0; i < drv_ctx->stitch_attr.stitch_pipe_num; i++) {
            vi_pipes = drv_ctx->stitch_attr.stitch_bind_id[i];
            drv_ctx_s = isp_drv_get_ctx(vi_pipes);
            drv_ctx_s->running_state = ISP_BE_BUF_STATE_INIT;
        }
    }

    osal_spin_unlock_irqrestore(isp_spin_lock, &flags);
#endif
    return TD_SUCCESS;
}

#ifdef CONFIG_OT_VI_STITCH_GRP
static td_void isp_drv_stitch_be_buf_ctl(ot_vi_pipe vi_pipe)
{
    td_s32 i;
    td_s32 ret;
    ot_vi_pipe vi_pipes;
    ot_vi_pipe main_pipe;
    isp_drv_ctx *drv_ctx = TD_NULL;
    isp_drv_ctx *drv_ctx_s = TD_NULL;
    unsigned long flags = 0;
    osal_spinlock *isp_sync_lock = TD_NULL;

    drv_ctx = isp_drv_get_ctx(vi_pipe);

    main_pipe = drv_ctx->stitch_attr.stitch_bind_id[0];
    isp_sync_lock = isp_drv_get_sync_lock(main_pipe);
    osal_spin_lock_irqsave(isp_sync_lock, &flags);

    if (drv_ctx->running_state != ISP_BE_BUF_STATE_RUNNING) {
        osal_spin_unlock_irqrestore(isp_sync_lock, &flags);
        return;
    }

    drv_ctx->running_state = ISP_BE_BUF_STATE_FINISH;

    ret = isp_drv_stitch_sync(vi_pipe);
    if (ret != TD_SUCCESS) {
        osal_spin_unlock_irqrestore(isp_sync_lock, &flags);
        return;
    }

    for (i = 0; i < drv_ctx->stitch_attr.stitch_pipe_num; i++) {
        vi_pipes = drv_ctx->stitch_attr.stitch_bind_id[i];
        drv_ctx_s = isp_drv_get_ctx(vi_pipes);
        if (drv_ctx_s->running_state != ISP_BE_BUF_STATE_FINISH) {
            osal_spin_unlock_irqrestore(isp_sync_lock, &flags);
            return;
        }
    }

    for (i = 0; i < drv_ctx->stitch_attr.stitch_pipe_num; i++) {
        vi_pipes = drv_ctx->stitch_attr.stitch_bind_id[i];
        drv_ctx_s = isp_drv_get_ctx(vi_pipes);
        if (drv_ctx_s->be_buf_info.init != TD_TRUE) {
            isp_err_trace("Pipe[%d] BeBuf (bInit != TRUE) !\n", vi_pipe);
            osal_spin_unlock_irqrestore(isp_sync_lock, &flags);
            return;
        }

        if (drv_ctx_s->run_once_flag != TD_TRUE) {
            isp_drv_be_buf_queue_put_busy(vi_pipes);
        }
        drv_ctx_s->running_state = ISP_BE_BUF_STATE_INIT;
    }

    osal_spin_unlock_irqrestore(isp_sync_lock, &flags);

    return;
}
#endif

td_void isp_drv_be_buf_queue_put_busy(ot_vi_pipe vi_pipe)
{
    td_phys_addr_t phy_addr;
    td_u64 size;
    td_void *vir_addr = TD_NULL;
    isp_be_buf_node *node = TD_NULL;
    struct osal_list_head *list_tmp = TD_NULL;
    struct osal_list_head *list_node = TD_NULL;
    isp_drv_ctx *drv_ctx = TD_NULL;

    drv_ctx = isp_drv_get_ctx(vi_pipe);
    if (drv_ctx->use_node == TD_NULL) {
        return;
    }

    osal_list_for_each_safe(list_node, list_tmp, &drv_ctx->be_buf_queue.busy_list)
    {
        node = osal_list_entry(list_node, isp_be_buf_node, list);
        if (node->hold_cnt == 0) {
            isp_queue_del_busy_be_buf(&drv_ctx->be_buf_queue, node);
            isp_queue_put_free_be_buf(&drv_ctx->be_buf_queue, node);
        }
    }

    phy_addr = drv_ctx->use_node->be_cfg_buf.phy_addr;
    vir_addr = drv_ctx->use_node->be_cfg_buf.vir_addr;
    size = drv_ctx->use_node->be_cfg_buf.size;

    cmpi_dcache_region_wb(vir_addr, phy_addr, size);

    isp_queue_put_busy_be_buf(&drv_ctx->be_buf_queue, drv_ctx->use_node);

    drv_ctx->use_node = TD_NULL;

    return;
}

td_s32 isp_drv_be_buf_ctl(ot_vi_pipe vi_pipe)
{
    isp_drv_ctx *drv_ctx = TD_NULL;
    unsigned long flags = 0;
    osal_spinlock *isp_spin_lock = TD_NULL;

    isp_check_pipe_return(vi_pipe);

    drv_ctx = isp_drv_get_ctx(vi_pipe);
    isp_check_online_mode_return(vi_pipe, drv_ctx->work_mode.running_mode);

    if (drv_ctx->stitch_attr.stitch_enable != TD_TRUE) {
        if (drv_ctx->be_buf_info.init != TD_TRUE) {
            isp_err_trace("Pipe[%d] BeBuf (bInit != TRUE) !\n", vi_pipe);
            return TD_FAILURE;
        }
        isp_spin_lock = isp_drv_get_lock(vi_pipe);
        osal_spin_lock_irqsave(isp_spin_lock, &flags);
        if (drv_ctx->run_once_flag != TD_TRUE) {
            isp_drv_be_buf_queue_put_busy(vi_pipe);
        }
        osal_spin_unlock_irqrestore(isp_spin_lock, &flags);
    } else {
#ifdef CONFIG_OT_VI_STITCH_GRP
        isp_drv_stitch_be_buf_ctl(vi_pipe);
#endif
    }

    return TD_SUCCESS;
}

static td_void isp_drv_get_be_buf_dbg_info(isp_drv_ctx *drv_ctx)
{
    td_u32 count = 0;
    isp_be_buf_node *node = TD_NULL;

    struct osal_list_head *list_tmp = TD_NULL;
    struct osal_list_head *list_node = TD_NULL;
    osal_list_for_each_safe(list_node, list_tmp, &drv_ctx->be_buf_queue.busy_list)
    {
        node = osal_list_entry(list_node, isp_be_buf_node, list);
        if (node->hold_cnt != 0) {
            count++;
        }
    }
    drv_ctx->be_buf_queue.hold_num_max = MAX2(drv_ctx->be_buf_queue.hold_num_max, count);
    return;
}
#ifdef CONFIG_OT_VI_STITCH_GRP
static td_s32 isp_drv_stitch_all_pipe_init(ot_vi_pipe vi_pipe)
{
    isp_drv_ctx *drv_ctx = TD_NULL;
    td_s32 ret;

    isp_check_pipe_return(vi_pipe);
    drv_ctx = isp_drv_get_ctx(vi_pipe);
    if (drv_ctx->stitch_attr.stitch_enable == TD_TRUE) {
        ret = isp_drv_stitch_sync(vi_pipe);
        if (ret != TD_SUCCESS) {
            return ret;
        }
    } else {
        if (drv_ctx->isp_init != TD_TRUE) {
            return TD_FAILURE;
        }
    }

    return TD_SUCCESS;
}
#endif

td_s32 isp_drv_get_ready_be_buf(ot_vi_pipe vi_pipe, isp_be_wo_cfg_buf *be_cfg_buf)
{
    osal_spinlock *isp_spin_lock = TD_NULL;
    td_s32 ret;
    unsigned long flags = 0;
    isp_be_buf_node *node = TD_NULL;
    isp_drv_ctx *drv_ctx = TD_NULL;

    isp_check_pipe_return(vi_pipe);
    isp_check_pointer_return(be_cfg_buf);

    drv_ctx = isp_drv_get_ctx(vi_pipe);
#ifdef CONFIG_OT_VI_STITCH_GRP
    ret = isp_drv_stitch_all_pipe_init(vi_pipe);
    if (ret != TD_SUCCESS) {
        return ret;
    }
#else
    ot_unused(ret);
    if (drv_ctx->isp_init != TD_TRUE) {
        return TD_FAILURE;
    }
#endif

    isp_check_online_mode_return(vi_pipe, drv_ctx->work_mode.running_mode);

    isp_spin_lock = isp_drv_get_spin_lock(vi_pipe);
    osal_spin_lock_irqsave(isp_spin_lock, &flags);

    if ((drv_ctx->exit_state == ISP_BE_BUF_EXIT) || (drv_ctx->exit_state == ISP_BE_BUF_WAITING)) {
        isp_err_trace("ViPipe[%d] ISP BE Buf not existed!!!\n", vi_pipe);
        osal_spin_unlock_irqrestore(isp_spin_lock, &flags);
        return TD_FAILURE;
    }

    node = isp_queue_query_busy_be_buf(&drv_ctx->be_buf_queue);
    if (node == TD_NULL) {
        isp_err_trace("ViPipe[%d] QueueQueryBusyBeBuf pstNode is null!\n", vi_pipe);
        osal_spin_unlock_irqrestore(isp_spin_lock, &flags);
        return TD_FAILURE;
    }

    node->hold_cnt++;
    drv_ctx->be_buf_info.use_cnt++;
    isp_drv_get_be_buf_dbg_info(drv_ctx);
    (td_void)memcpy_s(be_cfg_buf, sizeof(isp_be_wo_cfg_buf), &node->be_cfg_buf, sizeof(isp_be_wo_cfg_buf));

    if (node->hold_cnt == 1) {
        isp_drv_reg_config_bnr_offline((isp_be_wo_reg_cfg *)be_cfg_buf->vir_addr, drv_ctx);
        isp_drv_reg_config_vi_fpn_offline((isp_be_wo_reg_cfg *)be_cfg_buf->vir_addr, drv_ctx);
    }
    cmpi_dcache_region_wb(be_cfg_buf->vir_addr, be_cfg_buf->phy_addr, be_cfg_buf->size);

    isp_drv_update_be_offline_addr_info(vi_pipe, be_cfg_buf);

    drv_ctx->exit_state = ISP_BE_BUF_READY;

    osal_spin_unlock_irqrestore(isp_spin_lock, &flags);

    return TD_SUCCESS;
}

static td_void isp_drv_put_busy_to_free(ot_vi_pipe vi_pipe, isp_be_wo_cfg_buf *be_cfg_buf)
{
    isp_be_buf_node *node = TD_NULL;
    struct osal_list_head *list_tmp = TD_NULL;
    struct osal_list_head *list_node = TD_NULL;
    isp_drv_ctx *drv_ctx = TD_NULL;

    drv_ctx = isp_drv_get_ctx(vi_pipe);

    osal_list_for_each_safe(list_node, list_tmp, &drv_ctx->be_buf_queue.busy_list)
    {
        node = osal_list_entry(list_node, isp_be_buf_node, list);
        if (node->be_cfg_buf.phy_addr == be_cfg_buf->phy_addr) {
            if (node->hold_cnt > 0) {
                node->hold_cnt--;
            }

            if ((node->hold_cnt == 0) && (isp_queue_get_busy_num(&drv_ctx->be_buf_queue) > 1)) {
                isp_queue_del_busy_be_buf(&drv_ctx->be_buf_queue, node);
                isp_queue_put_free_be_buf(&drv_ctx->be_buf_queue, node);
            }
        }
    }

    return;
}

td_s32 isp_drv_put_free_be_buf(ot_vi_pipe vi_pipe, isp_be_wo_cfg_buf *be_cfg_buf)
{
    osal_spinlock *isp_spin_lock = TD_NULL;
    isp_drv_ctx *drv_ctx = TD_NULL;
    unsigned long flags = 0;

    isp_check_pipe_return(vi_pipe);
    isp_check_pointer_return(be_cfg_buf);

    drv_ctx = isp_drv_get_ctx(vi_pipe);
    isp_check_online_mode_return(vi_pipe, drv_ctx->work_mode.running_mode);

    isp_spin_lock = isp_drv_get_spin_lock(vi_pipe);
    osal_spin_lock_irqsave(isp_spin_lock, &flags);

    if (drv_ctx->exit_state == ISP_BE_BUF_EXIT) {
        isp_err_trace("ViPipe[%d] ISP BE Buf not existed!!!\n", vi_pipe);
        osal_spin_unlock_irqrestore(isp_spin_lock, &flags);
        return TD_FAILURE;
    }

    isp_drv_put_busy_to_free(vi_pipe, be_cfg_buf);

    if (drv_ctx->be_buf_info.use_cnt > 0) {
        drv_ctx->be_buf_info.use_cnt--;
    }
    if (isp_drv_get_ldci_tpr_flt_en(vi_pipe) == TD_TRUE) {
        isp_drv_update_ldci_tpr_offline_stat(vi_pipe, (isp_be_wo_reg_cfg *)be_cfg_buf->vir_addr);
    }

    osal_spin_unlock_irqrestore(isp_spin_lock, &flags);

    osal_wait_wakeup(&drv_ctx->isp_exit_wait);

    return TD_SUCCESS;
}


td_s32 isp_drv_hold_busy_be_buf(ot_vi_pipe vi_pipe, isp_be_wo_cfg_buf *be_cfg_buf)
{
    osal_spinlock *isp_spin_lock = TD_NULL;
    isp_be_buf_node *node = TD_NULL;
    struct osal_list_head *list_tmp = TD_NULL;
    struct osal_list_head *list_node = TD_NULL;
    isp_drv_ctx *drv_ctx = TD_NULL;
    unsigned long flags = 0;

    isp_check_pipe_return(vi_pipe);
    isp_check_pointer_return(be_cfg_buf);

    drv_ctx = isp_drv_get_ctx(vi_pipe);
    isp_check_online_mode_return(vi_pipe, drv_ctx->work_mode.running_mode);

    isp_spin_lock = isp_drv_get_spin_lock(vi_pipe);
    osal_spin_lock_irqsave(isp_spin_lock, &flags);

    if (drv_ctx->exit_state == ISP_BE_BUF_EXIT) {
        isp_err_trace("ViPipe[%d] ISP BE Buf not existed!!!\n", vi_pipe);
        osal_spin_unlock_irqrestore(isp_spin_lock, &flags);
        return TD_FAILURE;
    }

    osal_list_for_each_safe(list_node, list_tmp, &drv_ctx->be_buf_queue.busy_list)
    {
        node = osal_list_entry(list_node, isp_be_buf_node, list);
        if (node->be_cfg_buf.phy_addr == be_cfg_buf->phy_addr) {
            node->hold_cnt++;
            drv_ctx->be_buf_info.use_cnt++;
        }
    }
    isp_drv_get_be_buf_dbg_info(drv_ctx);

    if (drv_ctx->exit_state != ISP_BE_BUF_WAITING) {
        drv_ctx->exit_state = ISP_BE_BUF_READY;
    }

    osal_spin_unlock_irqrestore(isp_spin_lock, &flags);

    return TD_SUCCESS;
}

