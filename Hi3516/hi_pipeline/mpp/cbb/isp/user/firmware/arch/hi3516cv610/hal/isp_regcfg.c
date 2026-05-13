/*
  Copyright (c), 2001-2025, Shenshu Tech. Co., Ltd.
 */

#include <stdio.h>
#include <string.h>
#include <sys/ioctl.h>
#include "mkp_isp.h"
#include "isp_config.h"
#include "isp_lut_config.h"
#include "isp_ext_config.h"
#include "isp_main.h"
#include "isp_math_utils.h"
#include "ot_mpi_sys_mem.h"
#include "ot_math.h"
#include "valg_plat.h"
#include "ot_isp_define.h"
#include "isp_regcfg.h"
#ifdef CONFIG_OT_ISP_DUMP_DEBUG_SUPPORT
#include "isp_dump_dbg.h"
#endif

isp_be_buf            g_be_buf_ctx[OT_ISP_MAX_PIPE_NUM] = {{ 0 }};
isp_reg_cfg_attr     *g_reg_cfg_ctx[OT_ISP_MAX_PIPE_NUM] = { TD_NULL };
isp_be_lut_buf        g_be_lut_buf_ctx[OT_ISP_MAX_PIPE_NUM] = { 0 };
isp_ldci_read_stt_buf g_ldci_read_stt_buf_ctx[OT_ISP_MAX_PIPE_NUM] = { 0 };
isp_fe_lut2stt_attr   g_fe_lut_stt_buf_ctx[OT_ISP_MAX_FE_PIPE_NUM] = { 0 };

#define isp_regcfg_set_ctx(pipe, ctx) (g_reg_cfg_ctx[pipe] = (ctx))
#define isp_regcfg_reset_ctx(pipe) (g_reg_cfg_ctx[pipe] = TD_NULL)
#define be_reg_get_ctx(pipe, ctx) ctx = &g_be_buf_ctx[pipe]
#define be_lut_buf_get_ctx(pipe, ctx) ctx = &g_be_lut_buf_ctx[pipe]
#define fe_lut_stt_buf_get_ctx(pipe, ctx) ctx = &g_fe_lut_stt_buf_ctx[pipe]
#define ldci_buf_get_ctx(pipe, ctx) ctx = &g_ldci_read_stt_buf_ctx[pipe]
#define STT_LUT_CONFIG_TIMES 3

isp_reg_cfg_attr *isp_get_regcfg_ctx(ot_vi_pipe vi_pipe)
{
    return g_reg_cfg_ctx[vi_pipe];
}

td_s32 isp_ctrl_param_get(ot_isp_ctrl_param *ctrl_param)
{
    isp_check_pointer_return(ctrl_param);

    ctrl_param->quick_start_en = TD_FALSE;

    return TD_SUCCESS;
}

td_s32 isp_clut_buf_init(ot_vi_pipe vi_pipe)
{
    return TD_SUCCESS;
}

td_void isp_clut_buf_exit(ot_vi_pipe vi_pipe)
{
    return;
}

static td_s32 isp_update_be_lut_stt_buf_ctx(ot_vi_pipe vi_pipe, td_phys_addr_t phy_addr)
{
    td_u8 i;
    size_t size;
    td_void *virt_addr = TD_NULL;
    isp_usr_ctx *isp_ctx = TD_NULL;
    isp_be_lut_buf *be_lut_buf = TD_NULL;

    isp_get_ctx(vi_pipe, isp_ctx);

    if (is_offline_mode(isp_ctx->block_attr.running_mode) || is_striping_mode(isp_ctx->block_attr.running_mode) ||
        is_pre_online_post_offline(isp_ctx->block_attr.running_mode)) {
        return TD_FAILURE;
    }

    be_lut_buf_get_ctx(vi_pipe, be_lut_buf);

    size = sizeof(isp_be_lut_wstt_type);

    virt_addr = ot_mpi_sys_mmap(phy_addr, size * 2 * ISP_MAX_BE_NUM); /* lut_stt_buf number 2  */
    if (virt_addr == TD_NULL) {
        isp_err_trace("Pipe:%d get be lut stt bufs address failed!\n", vi_pipe);
        return TD_FAILURE;
    }

    for (i = 0; i < ISP_MAX_BE_NUM; i++) {
        be_lut_buf->lut_stt_buf[i].phy_addr = phy_addr + 2 * i * size;                        /* phy_addr index 2  */
        be_lut_buf->lut_stt_buf[i].vir_addr = (td_void *)((td_u8 *)virt_addr + 2 * i * size); /* lut_stt_buf 2  */
        be_lut_buf->lut_stt_buf[i].size = size;
    }

    return TD_SUCCESS;
}

static td_s32 isp_be_lut_buf_addr_init(ot_vi_pipe vi_pipe)
{
    td_s32 ret;
    td_phys_addr_t phy_addr;
    isp_usr_ctx *isp_ctx = TD_NULL;

    isp_get_ctx(vi_pipe, isp_ctx);

    if (is_offline_mode(isp_ctx->block_attr.running_mode) || is_striping_mode(isp_ctx->block_attr.running_mode)) {
        return TD_SUCCESS;
    }

#ifdef CONFIG_OT_VI_PREONLINE_POSTOFFLINE
    if (is_pre_online_post_offline(isp_ctx->block_attr.running_mode)) {
        return TD_SUCCESS;
    }
#endif
    ret = ioctl(isp_get_fd(vi_pipe), ISP_BE_LUT_STT_BUF_GET, &phy_addr);
    if (ret != TD_SUCCESS) {
        isp_err_trace("Pipe:%d get be lut2stt bufs address failed%x!\n", vi_pipe, ret);
        return ret;
    }

    ot_ext_addr_write(ot_ext_system_be_lut_stt_buffer_high_addr_write,
        ot_ext_system_be_lut_stt_buffer_low_addr_write, vi_pipe, phy_addr);

    ret = isp_update_be_lut_stt_buf_ctx(vi_pipe, phy_addr);
    if (ret != TD_SUCCESS) {
        return ret;
    }

    return TD_SUCCESS;
}


static td_s32 isp_be_lut_buf_addr_exit(ot_vi_pipe vi_pipe)
{
    td_u8 i;
    isp_usr_ctx *isp_ctx = TD_NULL;
    isp_be_lut_buf *be_lut_buf = TD_NULL;

    isp_get_ctx(vi_pipe, isp_ctx);
    be_lut_buf_get_ctx(vi_pipe, be_lut_buf);

    if (is_offline_mode(isp_ctx->block_attr.running_mode) || is_striping_mode(isp_ctx->block_attr.running_mode)) {
        return TD_SUCCESS;
    }
#ifdef CONFIG_OT_VI_PREONLINE_POSTOFFLINE
    if (is_pre_online_post_offline(isp_ctx->block_attr.running_mode)) {
        for (i = 0; i < ISP_MAX_BE_NUM; i++) {
            be_lut_buf->lut_stt_buf[i].vir_addr = TD_NULL;
        }
        return TD_SUCCESS;
    }
#endif

    if (be_lut_buf->lut_stt_buf[0].vir_addr != TD_NULL) {
        ot_mpi_sys_munmap(be_lut_buf->lut_stt_buf[0].vir_addr,
            sizeof(isp_be_lut_wstt_type) * 2 * ISP_MAX_BE_NUM); /* lut buf number is 2 */
        for (i = 0; i < ISP_MAX_BE_NUM; i++) {
            be_lut_buf->lut_stt_buf[i].vir_addr = TD_NULL;
        }
    }

    return TD_SUCCESS;
}

td_s32 isp_stt_buf_init(ot_vi_pipe vi_pipe)
{
    td_s32 ret;

    if (ioctl(isp_get_fd(vi_pipe), ISP_STT_BUF_INIT) != TD_SUCCESS) {
        isp_err_trace("ISP[%d] stt buffer init failed\n", vi_pipe);
        return TD_FAILURE;
    }

    ret = isp_be_lut_buf_addr_init(vi_pipe);
    if (ret != TD_SUCCESS) {
        isp_err_trace("ISP[%d] be lut2stt buffer address init failed\n", vi_pipe);
        goto fail0;
    }

    return TD_SUCCESS;

fail0:
    isp_stt_buf_exit(vi_pipe);
    return TD_FAILURE;
}

td_void isp_stt_buf_exit(ot_vi_pipe vi_pipe)
{
    td_s32 ret;

    ret = isp_be_lut_buf_addr_exit(vi_pipe);
    if (ret != TD_SUCCESS) {
        isp_err_trace("ISP[%d] be lut stt buffer exit failed\n", vi_pipe);
    }

    if (ioctl(isp_get_fd(vi_pipe), ISP_STT_BUF_EXIT) != TD_SUCCESS) {
        isp_err_trace("exit stt bufs failed\n");
        return;
    }
}

td_s32 isp_stt_addr_init(ot_vi_pipe vi_pipe)
{
    if (ioctl(isp_get_fd(vi_pipe), ISP_STT_ADDR_INIT) != TD_SUCCESS) {
        isp_err_trace("ISP[%d] stt address init failed\n", vi_pipe);
        return TD_FAILURE;
    }

    return TD_SUCCESS;
}

static td_s32 isp_update_ldci_read_stt_buf_ctx(ot_vi_pipe vi_pipe, td_phys_addr_t phy_addr)
{
    td_u8 i;
    size_t size;
    td_void *virt_addr = TD_NULL;
    isp_usr_ctx *isp_ctx = TD_NULL;
    isp_ldci_read_stt_buf *ldci_read_stt_buf = TD_NULL;

    isp_get_ctx(vi_pipe, isp_ctx);

    if (isp_ctx->ldci_tpr_flt_en == TD_FALSE) {
        return TD_SUCCESS;
    }

    ldci_buf_get_ctx(vi_pipe, ldci_read_stt_buf);

    size = sizeof(isp_ldci_stat);
    virt_addr = ot_mpi_sys_mmap(phy_addr, size * ldci_read_stt_buf->buf_num);
    if (virt_addr == TD_NULL) {
        isp_err_trace("ISP[%d]:map ldci read stt buffer failed\n", vi_pipe);
        return TD_FAILURE;
    }

    for (i = 0; i < ldci_read_stt_buf->buf_num; i++) {
        ldci_read_stt_buf->read_buf[i].phy_addr = phy_addr + i * size;
        ldci_read_stt_buf->read_buf[i].vir_addr = (td_void *)((td_u8 *)virt_addr + i * size);
        ldci_read_stt_buf->read_buf[i].size = size;
    }

    return TD_SUCCESS;
}

static td_s32 isp_ldci_read_stt_buf_addr_init(ot_vi_pipe vi_pipe)
{
    td_s32 ret;
    td_phys_addr_t phy_addr;
    isp_usr_ctx *isp_ctx = TD_NULL;
    isp_ldci_read_stt_buf *ldci_read_stt_buf = TD_NULL;

    isp_get_ctx(vi_pipe, isp_ctx);

    if (isp_ctx->ldci_tpr_flt_en == TD_FALSE) {
        return TD_SUCCESS;
    }

    ldci_buf_get_ctx(vi_pipe, ldci_read_stt_buf);
    ret = ioctl(isp_get_fd(vi_pipe), ISP_LDCI_READ_STT_BUF_GET, ldci_read_stt_buf);
    if (ret != TD_SUCCESS) {
        isp_err_trace("ISP[%d]:Get ldci read stt buffer address failed\n", vi_pipe);
        return ret;
    }

    phy_addr = ldci_read_stt_buf->head_phy_addr;
    ot_ext_addr_write(ot_ext_system_ldci_read_stt_buffer_high_addr_write,
        ot_ext_system_ldci_read_stt_buffer_low_addr_write, vi_pipe, phy_addr);

    ret = isp_update_ldci_read_stt_buf_ctx(vi_pipe, phy_addr);
    if (ret != TD_SUCCESS) {
        return ret;
    }

    return TD_SUCCESS;
}

static td_s32 isp_ldci_read_stt_buf_addr_exit(ot_vi_pipe vi_pipe)
{
    td_u8 i;
    isp_usr_ctx *isp_ctx = TD_NULL;
    isp_ldci_read_stt_buf *ldci_read_stt_buf = TD_NULL;

    isp_get_ctx(vi_pipe, isp_ctx);

    if (isp_ctx->ldci_tpr_flt_en == TD_FALSE) {
        return TD_SUCCESS;
    }

    ldci_buf_get_ctx(vi_pipe, ldci_read_stt_buf);

    if (ldci_read_stt_buf->read_buf[0].vir_addr != TD_NULL) {
        ot_mpi_sys_munmap(ldci_read_stt_buf->read_buf[0].vir_addr, sizeof(isp_ldci_stat) * ldci_read_stt_buf->buf_num);

        for (i = 0; i < OT_ISP_BE_BUF_NUM_MAX; i++) {
            ldci_read_stt_buf->read_buf[i].vir_addr = TD_NULL;
        }
    }

    return TD_SUCCESS;
}

td_s32 isp_ldci_buf_init(ot_vi_pipe vi_pipe)
{
    td_s32 ret;
    ot_isp_ctrl_param isp_ctrl_param = { 0 };
    isp_usr_ctx *isp_ctx = TD_NULL;

    isp_get_ctx(vi_pipe, isp_ctx);

    ret = ioctl(isp_get_fd(vi_pipe), ISP_GET_CTRL_PARAM, &isp_ctrl_param);
    if (ret != TD_SUCCESS) {
        isp_err_trace("ISP[%d] get ctrlparam failed\n", vi_pipe);
        return ret;
    }

    isp_ctx->ldci_tpr_flt_en = isp_ctrl_param.ldci_tpr_flt_en;

    if ((isp_ctx->ldci_tpr_flt_en == TD_FALSE) && (is_online_mode(isp_ctx->block_attr.running_mode))) {
        return TD_SUCCESS;
    }

    ret = ioctl(isp_get_fd(vi_pipe), ISP_LDCI_BUF_INIT);
    if (ret != TD_SUCCESS) {
        isp_err_trace("ISP[%d] ldci buffer init failed\n", vi_pipe);
        return TD_FAILURE;
    }

    if (isp_ctx->ldci_tpr_flt_en == TD_TRUE) {
        ret = isp_ldci_read_stt_buf_addr_init(vi_pipe);
        if (ret != TD_SUCCESS) {
            isp_err_trace("ISP[%d] ldci read stt buffer address init failed\n", vi_pipe);
            isp_ldci_buf_exit(vi_pipe);
            return TD_FAILURE;
        }
    }

    return TD_SUCCESS;
}

td_void isp_ldci_buf_exit(ot_vi_pipe vi_pipe)
{
    td_s32 ret;
    isp_usr_ctx *isp_ctx = TD_NULL;

    isp_get_ctx(vi_pipe, isp_ctx);
    if ((isp_ctx->ldci_tpr_flt_en == TD_FALSE) && is_online_mode(isp_ctx->block_attr.running_mode)) {
        return;
    }

    if (isp_ctx->ldci_tpr_flt_en == TD_TRUE) {
        ret = isp_ldci_read_stt_buf_addr_exit(vi_pipe);
        if (ret != TD_SUCCESS) {
            isp_err_trace("ISP[%d] exit readstt bufaddr failed\n", vi_pipe);
        }
    }

    if (ioctl(isp_get_fd(vi_pipe), ISP_LDCI_BUF_EXIT) != TD_SUCCESS) {
        isp_err_trace("ISP[%d] exit ldci bufs failed\n", vi_pipe);
        return;
    }
}

static td_s32 isp_drc_buf_init(ot_vi_pipe vi_pipe)
{
    td_s32 ret;

    ret = ioctl(isp_get_fd(vi_pipe), ISP_DRC_BUF_INIT);
    if (ret != TD_SUCCESS) {
        isp_err_trace("ISP[%d] drc buffer init failed\n", vi_pipe);
        return TD_FAILURE;
    }

    return TD_SUCCESS;
}

static td_void isp_drc_buf_exit(ot_vi_pipe vi_pipe)
{
    td_s32 ret;

    ret = ioctl(isp_get_fd(vi_pipe), ISP_DRC_BUF_EXIT);
    if (ret != TD_SUCCESS) {
        isp_err_trace("ISP[%d] exit drc bufs failed\n", vi_pipe);
        return;
    }
}

td_s32 isp_alg_stats_buf_init(ot_vi_pipe vi_pipe)
{
    td_s32 ret;

    ret = isp_ldci_buf_init(vi_pipe);
    if (ret != TD_SUCCESS) {
        goto fail00;
    }

    ret = isp_drc_buf_init(vi_pipe);
    if (ret != TD_SUCCESS) {
        goto fail01;
    }

    return TD_SUCCESS;

fail01:
    isp_ldci_buf_exit(vi_pipe);
fail00:
    return OT_ERR_ISP_NOT_INIT;
}

td_void isp_alg_stats_buf_exit(ot_vi_pipe vi_pipe)
{
    isp_drc_buf_exit(vi_pipe);
    isp_ldci_buf_exit(vi_pipe);
}

td_s32 isp_cfg_be_buf_init(ot_vi_pipe vi_pipe)
{
    td_s32 ret;
    size_t be_buf_size;
    isp_mmz_buf_ex be_buf_info;
    isp_be_buf *be_buf = TD_NULL;

    isp_check_offline_mode_return(vi_pipe);
    be_reg_get_ctx(vi_pipe, be_buf);

    ret = ioctl(isp_get_fd(vi_pipe), ISP_BE_CFG_BUF_INIT, &be_buf_info);
    if (ret != TD_SUCCESS) {
        isp_err_trace("Pipe:%d init be config bufs failed %x!\n", vi_pipe, ret);
        return ret;
    }
    be_buf->be_phy_addr = be_buf_info.phy_addr;
    be_buf->be_buf_size = be_buf_info.size; // all be_buf_size: size * be_buf_num
    be_buf->be_virt_addr = ot_mpi_sys_mmap_cached(be_buf->be_phy_addr, (td_u32)be_buf->be_buf_size);

    if (be_buf->be_virt_addr == TD_NULL) {
        isp_err_trace("Pipe:%d init be config bufs failed!\n", vi_pipe);
        ret = ioctl(isp_get_fd(vi_pipe), ISP_BE_CFG_BUF_EXIT);
        if (ret != TD_SUCCESS) {
            isp_err_trace("Pipe:%d exit be config bufs failed %x!\n", vi_pipe, ret);
            return ret;
        }

        return OT_ERR_ISP_NOMEM;
    }

    be_buf->be_wo_cfg_buf.phy_addr = be_buf->be_phy_addr;

    /* Get be buffer start address & size */
    be_buf_size = (size_t)be_buf_info.size;

    ot_ext_addr_write(ot_ext_system_be_buffer_address_high_write,
        ot_ext_system_be_buffer_address_low_write, vi_pipe, be_buf->be_phy_addr);
    ot_ext_system_be_buffer_size_write(vi_pipe, be_buf_size);

    return TD_SUCCESS;
}


static td_s8 isp_get_block_id_by_pipe(ot_vi_pipe vi_pipe)
{
    td_s8 block_id = 0;
    ot_unused(vi_pipe);
    return block_id;
}

static td_s32 isp_update_be_buf_addr(ot_vi_pipe vi_pipe, td_void *virt_addr)
{
    td_u16 i;
    size_t buf_size;
    td_u8 *head_virt = TD_NULL;
    isp_running_mode isp_runing_mode;
    isp_usr_ctx *isp_ctx = TD_NULL;
    isp_get_ctx(vi_pipe, isp_ctx);

    isp_runing_mode = isp_ctx->block_attr.running_mode;
    buf_size = sizeof(isp_be_wo_reg_cfg) / OT_ISP_STRIPING_MAX_NUM;

    switch (isp_runing_mode) {
        case ISP_MODE_RUNNING_STRIPING:
            for (i = 0; i < OT_ISP_STRIPING_MAX_NUM; i++) {
                head_virt = (td_u8 *)virt_addr + i * buf_size;
                isp_ctx->isp_pre_be_virt_addr[i] = (td_void *)head_virt;
                isp_ctx->isp_post_be_virt_addr[i] = (td_void *)(head_virt + BE_OFFLINE_OFFSET);
                isp_ctx->pre_viproc_virt_addr[i] = (td_void *)(head_virt + VIPROC_OFFLINE_OFFSET);
                isp_ctx->post_viproc_virt_addr[i] = (td_void *)(head_virt + VIPROC_OFFLINE_OFFSET);
            }
            break;

        case ISP_MODE_RUNNING_OFFLINE:
            for (i = 0; i < OT_ISP_STRIPING_MAX_NUM; i++) {
                if (i == 0) {
                    isp_ctx->isp_pre_be_virt_addr[i] = virt_addr;
                    isp_ctx->isp_post_be_virt_addr[i] = (td_void *)((td_u8 *)virt_addr + BE_OFFLINE_OFFSET);
                    isp_ctx->pre_viproc_virt_addr[i] = (td_void *)((td_u8 *)virt_addr + VIPROC_OFFLINE_OFFSET);
                    isp_ctx->post_viproc_virt_addr[i] = (td_void *)((td_u8 *)virt_addr + VIPROC_OFFLINE_OFFSET);
                } else {
                    isp_ctx->isp_pre_be_virt_addr[i] = TD_NULL;
                    isp_ctx->isp_post_be_virt_addr[i] = TD_NULL;
                    isp_ctx->pre_viproc_virt_addr[i] = TD_NULL;
                    isp_ctx->post_viproc_virt_addr[i] = TD_NULL;
                }
            }
            break;
#ifdef CONFIG_OT_VI_PREONLINE_POSTOFFLINE
        case ISP_MODE_RUNNING_PRE_ONLINE_POST_OFFLINE:
            for (i = 0; i < OT_ISP_STRIPING_MAX_NUM; i++) {
                if (i == 0) {
                    isp_ctx->isp_pre_be_virt_addr[i] = isp_get_be_reg_virt_addr_base(isp_be_reg_base(i));
                    isp_ctx->isp_post_be_virt_addr[i] = (td_void *)((td_u8 *)virt_addr);
                    isp_ctx->pre_viproc_virt_addr[i] = isp_get_viproc_reg_virt_addr_base(isp_viproc_reg_base(i));
                    isp_ctx->post_viproc_virt_addr[i] = (td_void *)((td_u8 *)virt_addr + VIPROC_OFFLINE_OFFSET);
                } else {
                    isp_ctx->isp_pre_be_virt_addr[i] = TD_NULL;
                    isp_ctx->isp_post_be_virt_addr[i] = TD_NULL;
                    isp_ctx->pre_viproc_virt_addr[i] = TD_NULL;
                    isp_ctx->post_viproc_virt_addr[i] = TD_NULL;
                }
            }
            break;
#endif
        default:
            return TD_FAILURE;
    }

    return TD_SUCCESS;
}

static td_s32 isp_cfg_be_buf_mmap(ot_vi_pipe vi_pipe)
{
    td_s32 ret;
    td_phys_addr_t be_phy_addr;
    isp_be_buf *be_buf = TD_NULL;

    be_reg_get_ctx(vi_pipe, be_buf);

    be_phy_addr = be_buf->be_wo_cfg_buf.phy_addr;
    ot_ext_addr_write(ot_ext_system_be_free_buffer_high_addr_write,
        ot_ext_system_be_free_buffer_low_addr_write, vi_pipe, be_phy_addr);

    if (be_buf->be_virt_addr != TD_NULL) {
        be_buf->be_wo_cfg_buf.vir_addr =
            (td_void *)((td_u8 *)be_buf->be_virt_addr + (be_buf->be_wo_cfg_buf.phy_addr - be_buf->be_phy_addr));
    } else {
        be_buf->be_wo_cfg_buf.vir_addr = TD_NULL;
    }

    if (be_buf->be_wo_cfg_buf.vir_addr == TD_NULL) {
        return TD_FAILURE;
    }

    ret = isp_update_be_buf_addr(vi_pipe, be_buf->be_wo_cfg_buf.vir_addr);
    if (ret != TD_SUCCESS) {
        isp_err_trace("Pipe:%d isp update BE bufs failed %x!\n", vi_pipe, ret);
        return TD_FAILURE;
    }

    return TD_SUCCESS;
}

td_s32 isp_get_be_buf_first(ot_vi_pipe vi_pipe)
{
    td_s32 ret;
    isp_be_buf *be_buf = TD_NULL;

    isp_check_offline_mode_return(vi_pipe);
    be_reg_get_ctx(vi_pipe, be_buf);

    ret = ioctl(isp_get_fd(vi_pipe), ISP_GET_BE_BUF_FIRST, &be_buf->be_wo_cfg_buf.phy_addr);
    if (ret != TD_SUCCESS) {
        isp_err_trace("Pipe:%d Get be free bufs failed %x!\n", vi_pipe, ret);
        return ret;
    }

    ret = isp_cfg_be_buf_mmap(vi_pipe);
    if (ret != TD_SUCCESS) {
        isp_err_trace("Pipe:%d isp_cfg_be_buf_mmap failed %x!\n", vi_pipe, ret);
        return TD_FAILURE;
    }

    return TD_SUCCESS;
}

td_s32 isp_get_be_free_buf(ot_vi_pipe vi_pipe)
{
    td_s32 ret;
    isp_be_buf *be_buf = TD_NULL;

    isp_check_offline_mode_return(vi_pipe);
    be_reg_get_ctx(vi_pipe, be_buf);

    ret = ioctl(isp_get_fd(vi_pipe), ISP_BE_FREE_BUF_GET, &be_buf->be_wo_cfg_buf);
    if (ret != TD_SUCCESS) {
        return ret;
    }

    ret = isp_cfg_be_buf_mmap(vi_pipe);
    if (ret != TD_SUCCESS) {
        isp_err_trace("Pipe:%d isp_cfg_be_buf_mmap failed %x!\n", vi_pipe, ret);
        return TD_FAILURE;
    }

    return TD_SUCCESS;
}

td_s32 isp_get_be_last_buf(ot_vi_pipe vi_pipe)
{
    td_s32 ret;
    isp_be_buf *be_buf = TD_NULL;

    isp_check_offline_mode_return(vi_pipe);
    be_reg_get_ctx(vi_pipe, be_buf);

    ret = ioctl(isp_get_fd(vi_pipe), ISP_BE_LAST_BUF_GET, &be_buf->be_wo_cfg_buf.phy_addr);
    if (ret != TD_SUCCESS) {
        isp_err_trace("Pipe:%d Get be busy bufs failed %x!\n", vi_pipe, ret);
        return ret;
    }

    ret = isp_cfg_be_buf_mmap(vi_pipe);
    if (ret != TD_SUCCESS) {
        isp_err_trace("Pipe:%d isp_cfg_be_buf_mmap failed %x!\n", vi_pipe, ret);
        return TD_FAILURE;
    }

    return TD_SUCCESS;
}

td_void isp_cfg_be_buf_exit(ot_vi_pipe vi_pipe)
{
    td_s32 ret;
    isp_usr_ctx *isp_ctx = TD_NULL;
    isp_be_buf *be_buf = TD_NULL;

    isp_get_ctx(vi_pipe, isp_ctx);

    if (is_online_mode(isp_ctx->block_attr.running_mode)) {
        return;
    }
    be_reg_get_ctx(vi_pipe, be_buf);

    if (be_buf->be_virt_addr != TD_NULL) {
        ot_mpi_sys_munmap(be_buf->be_virt_addr, (td_u32)be_buf->be_buf_size);
        be_buf->be_virt_addr = TD_NULL;
    }

    ret = ioctl(isp_get_fd(vi_pipe), ISP_BE_CFG_BUF_EXIT);
    if (ret != TD_SUCCESS) {
        isp_err_trace("Pipe:%d exit be config bufs failed %x!\n", vi_pipe, ret);
        return;
    }
}

static td_s32 isp_cfg_be_buf_ctl(ot_vi_pipe vi_pipe)
{
    td_s32 ret;
    isp_be_buf *be_buf = TD_NULL;

    isp_check_offline_mode_return(vi_pipe);
    be_reg_get_ctx(vi_pipe, be_buf);

    ret = ioctl(isp_get_fd(vi_pipe), ISP_BE_CFG_BUF_CTL, &be_buf->be_wo_cfg_buf);
    if (ret != TD_SUCCESS) {
        return ret;
    }

    return TD_SUCCESS;
}

#ifdef CONFIG_OT_VI_STITCH_GRP
td_s32 isp_set_cfg_be_buf_state(ot_vi_pipe vi_pipe)
{
    td_s32 ret;
    isp_usr_ctx *isp_ctx   = TD_NULL;
    isp_check_pipe_return(vi_pipe);
    isp_get_ctx(vi_pipe, isp_ctx);

    isp_check_offline_mode_return(vi_pipe);
    if (isp_ctx->stitch_attr.stitch_enable != TD_TRUE) {
        return TD_SUCCESS;
    }

    ret = ioctl(isp_get_fd(vi_pipe), ISP_BE_CFG_BUF_RUNNING);
    if (ret != TD_SUCCESS) {
        return ret;
    }

    return TD_SUCCESS;
}
#endif

/* init isp be cfgs all buffer */
td_s32 isp_all_cfgs_be_buf_init(ot_vi_pipe vi_pipe)
{
    td_s32 ret;

    isp_check_offline_mode_return(vi_pipe);

    ret = ioctl(isp_get_fd(vi_pipe), ISP_BE_ALL_BUF_INIT);
    if (ret != TD_SUCCESS) {
        isp_err_trace("ISP[%d] init be all bufs Failed with ec %#x!\n", vi_pipe, ret);
        return ret;
    }

    return TD_SUCCESS;
}

static td_void *isp_vreg_cfg_buf_addr(ot_vi_pipe vi_pipe, ot_blk_dev blk_dev)
{
    size_t size, map_size;
    td_phys_addr_t phy_addr_temp;

    isp_be_buf *be_buf = TD_NULL;
    isp_usr_ctx *isp_ctx = TD_NULL;

    isp_get_ctx(vi_pipe, isp_ctx);
    be_reg_get_ctx(vi_pipe, be_buf);

    size = sizeof(isp_be_wo_reg_cfg) / OT_ISP_STRIPING_MAX_NUM;

    if (be_buf->be_wo_cfg_buf.vir_addr != TD_NULL) {
        return ((td_u8 *)be_buf->be_wo_cfg_buf.vir_addr + ((td_u32)blk_dev) * size);
    }

    phy_addr_temp = ot_ext_addr_read(ot_ext_system_be_free_buffer_high_addr_read,
        ot_ext_system_be_free_buffer_low_addr_read, vi_pipe);

    map_size = (size_t)(be_buf->be_buf_size / isp_ctx->be_buf_num); // one be_buf size

    be_buf->be_wo_cfg_buf.phy_addr = phy_addr_temp;
    be_buf->be_wo_cfg_buf.vir_addr = ot_mpi_sys_mmap_cached(be_buf->be_wo_cfg_buf.phy_addr, map_size);
    if (be_buf->be_wo_cfg_buf.vir_addr == TD_NULL) {
        isp_err_trace("mmap fail\n");
        return TD_NULL;
    }

    return ((td_u8 *)be_buf->be_wo_cfg_buf.vir_addr + ((td_u32)blk_dev) * size);
}

static td_s32 isp_get_reg_cfg_virt_addr_online(ot_vi_pipe vi_pipe, td_void *pre_be_virt_addr[],
    td_void *post_be_virt_addr[], td_void *pre_viproc_virt_addr[], td_void *post_viproc_virt_addr[])
{
    td_u8 k;
    td_s8 blk_dev;
    td_u8 block_id;

    blk_dev = isp_get_block_id_by_pipe(vi_pipe);
    if (blk_dev == -1) {
        isp_err_trace("ISP[%d] Online Mode Pipe Err!\n", vi_pipe);
        return TD_FAILURE;
    }

    block_id = (td_u8)blk_dev;
    for (k = 0; k < OT_ISP_STRIPING_MAX_NUM; k++) {
        if (k == 0) {
            pre_be_virt_addr[k] = isp_get_be_reg_virt_addr_base(isp_be_reg_base(block_id));
            post_be_virt_addr[k] = pre_be_virt_addr[k];
            pre_viproc_virt_addr[k] = isp_get_viproc_reg_virt_addr_base(isp_viproc_reg_base(block_id));
            post_viproc_virt_addr[k] = pre_viproc_virt_addr[k];
        } else {
            pre_be_virt_addr[k] = TD_NULL;
            post_be_virt_addr[k] = TD_NULL;
            pre_viproc_virt_addr[k] = TD_NULL;
            post_viproc_virt_addr[k] = TD_NULL;
        }
    }

    return TD_SUCCESS;
}

static td_void isp_get_reg_cfg_virt_addr_offline(ot_vi_pipe vi_pipe, td_void *pre_be_virt_addr[],
    td_void *post_be_virt_addr[], td_void *pre_viproc_virt_addr[], td_void *post_viproc_virt_addr[])
{
    td_u8 k;
    td_void *head_virt = TD_NULL;

    for (k = 0; k < OT_ISP_STRIPING_MAX_NUM; k++) {
        if (k == 0) {
            head_virt = (td_u8 *)isp_vreg_cfg_buf_addr(vi_pipe, (ot_blk_dev)k);
            pre_be_virt_addr[k] = head_virt;
            post_be_virt_addr[k] = pre_be_virt_addr[k];
            pre_viproc_virt_addr[k] = (td_void *)((td_u8 *)head_virt + VIPROC_OFFLINE_OFFSET);
            post_viproc_virt_addr[k] = pre_viproc_virt_addr[k];
        } else {
            pre_be_virt_addr[k] = TD_NULL;
            post_be_virt_addr[k] = TD_NULL;
            pre_viproc_virt_addr[k] = TD_NULL;
            post_viproc_virt_addr[k] = TD_NULL;
        }
    }
}

#ifdef CONFIG_OT_VI_PREONLINE_POSTOFFLINE
static td_s32 isp_get_reg_cfg_virt_addr_pre_online_post_offline(ot_vi_pipe vi_pipe, td_void *pre_be_virt_addr[],
    td_void *post_be_virt_addr[], td_void *pre_viproc_virt_addr[], td_void *post_viproc_virt_addr[])
{
    td_u8 k;
    td_void *head_virt = TD_NULL;

    for (k = 0; k < OT_ISP_STRIPING_MAX_NUM; k++) {
        if (k == 0) {
            head_virt = (td_u8 *)isp_vreg_cfg_buf_addr(vi_pipe, (ot_blk_dev)k);
            pre_be_virt_addr[k] = isp_get_be_reg_virt_addr_base(isp_be_reg_base(k));
            pre_viproc_virt_addr[k] = isp_get_viproc_reg_virt_addr_base(isp_viproc_reg_base(k));
            post_be_virt_addr[k] = (td_void *)((td_u8 *)head_virt + BE_OFFLINE_OFFSET);
            post_viproc_virt_addr[k] = (td_void *)((td_u8 *)head_virt + VIPROC_OFFLINE_OFFSET);
        } else {
            pre_be_virt_addr[k] = TD_NULL;
            post_be_virt_addr[k] = TD_NULL;
            pre_viproc_virt_addr[k] = TD_NULL;
            post_viproc_virt_addr[k] = TD_NULL;
        }
    }

    return TD_SUCCESS;
}
#endif

static td_void isp_get_reg_cfg_virt_addr_striping(ot_vi_pipe vi_pipe, td_void *pre_be_virt_addr[],
    td_void *post_be_virt_addr[], td_void *pre_viproc_virt_addr[], td_void *post_viproc_virt_addr[])
{
    td_u8 k;
    td_u8 *head_virt = TD_NULL;

    for (k = 0; k < OT_ISP_STRIPING_MAX_NUM; k++) {
        head_virt = (td_u8 *)isp_vreg_cfg_buf_addr(vi_pipe, (ot_blk_dev)k);

        pre_be_virt_addr[k] = (td_void *)head_virt;
        post_be_virt_addr[k] = pre_be_virt_addr[k];
        pre_viproc_virt_addr[k] = (td_void *)(head_virt + VIPROC_OFFLINE_OFFSET);
        post_viproc_virt_addr[k] = pre_viproc_virt_addr[k];
    }
}

static td_s32 isp_get_reg_cfg_virt_addr(ot_vi_pipe vi_pipe, td_void *pre_be_virt_addr[], td_void *post_be_virt_addr[],
    td_void *pre_viproc_virt_addr[], td_void *post_viproc_virt_addr[])
{
    td_s32 ret;
    isp_working_mode isp_work_mode;

    ret = ioctl(isp_get_fd(vi_pipe), ISP_WORK_MODE_GET, &isp_work_mode);
    if (ret != TD_SUCCESS) {
        isp_err_trace("get isp work mode failed!\n");
        return ret;
    }

    switch (isp_work_mode.running_mode) {
        case ISP_MODE_RUNNING_ONLINE:
            ret = isp_get_reg_cfg_virt_addr_online(vi_pipe, pre_be_virt_addr, post_be_virt_addr, pre_viproc_virt_addr,
                post_viproc_virt_addr);
            if (ret != TD_SUCCESS) {
                return ret;
            }
            break;
        case ISP_MODE_RUNNING_OFFLINE:
            isp_get_reg_cfg_virt_addr_offline(vi_pipe, pre_be_virt_addr, post_be_virt_addr, pre_viproc_virt_addr,
                post_viproc_virt_addr);
            break;
        case ISP_MODE_RUNNING_STRIPING:
            isp_get_reg_cfg_virt_addr_striping(vi_pipe, pre_be_virt_addr, post_be_virt_addr, pre_viproc_virt_addr,
                post_viproc_virt_addr);
            break;
#ifdef CONFIG_OT_VI_PREONLINE_POSTOFFLINE
        case ISP_MODE_RUNNING_PRE_ONLINE_POST_OFFLINE:
            isp_get_reg_cfg_virt_addr_pre_online_post_offline(vi_pipe, pre_be_virt_addr, post_be_virt_addr,
                pre_viproc_virt_addr, post_viproc_virt_addr);
            break;
#endif
        default:
            isp_err_trace("ISP[%d] GetBe Running Mode Err!\n", vi_pipe);
            return TD_FAILURE;
    }

    return TD_SUCCESS;
}

td_s32 isp_be_vreg_addr_init(ot_vi_pipe vi_pipe)
{
    isp_usr_ctx *isp_ctx = TD_NULL;

    isp_get_ctx(vi_pipe, isp_ctx);
    return isp_get_reg_cfg_virt_addr(vi_pipe, isp_ctx->isp_pre_be_virt_addr, isp_ctx->isp_post_be_virt_addr,
        isp_ctx->pre_viproc_virt_addr, isp_ctx->post_viproc_virt_addr);
}

static td_void *isp_get_ldci_read_stt_vir_addr(ot_vi_pipe vi_pipe, td_u8 buf_idx)
{
    td_s32 ret;
    td_phys_addr_t phy_addr_tmp;
    isp_usr_ctx *isp_ctx = TD_NULL;
    isp_ldci_read_stt_buf *ldci_read_stt_buf = TD_NULL;

    isp_get_ctx(vi_pipe, isp_ctx);

    if (isp_ctx->ldci_tpr_flt_en == TD_FALSE) {
        return TD_NULL;
    }

    ldci_buf_get_ctx(vi_pipe, ldci_read_stt_buf);

    if (ldci_read_stt_buf->read_buf[buf_idx].vir_addr != TD_NULL) {
        return ldci_read_stt_buf->read_buf[buf_idx].vir_addr;
    }

    phy_addr_tmp = ot_ext_addr_read(ot_ext_system_ldci_read_stt_buffer_high_addr_read,
        ot_ext_system_ldci_read_stt_buffer_low_addr_read, vi_pipe);
    ret = isp_update_ldci_read_stt_buf_ctx(vi_pipe, phy_addr_tmp);
    if (ret != TD_SUCCESS) {
        return TD_NULL;
    }

    return ldci_read_stt_buf->read_buf[buf_idx].vir_addr;
}

td_void *isp_get_fe_vir_addr(ot_vi_pipe vi_pipe)
{
    isp_check_fe_pipe_return_null(vi_pipe);

    return isp_get_fe_reg_virt_addr_base(isp_fe_reg_base(vi_pipe));
}

static td_void *isp_get_be_lut2stt_vir_addr(ot_vi_pipe vi_pipe, ot_blk_dev blk_dev, td_u8 buf_id)
{
    td_s32 ret;
    size_t size;
    td_phys_addr_t phy_addr_temp;
    isp_be_lut_buf *be_lut_buf = TD_NULL;
    isp_usr_ctx *isp_ctx = TD_NULL;

    isp_check_pipe_valid_return(vi_pipe);
    isp_check_be_dev_return(blk_dev);
    isp_get_ctx(vi_pipe, isp_ctx);
    be_lut_buf_get_ctx(vi_pipe, be_lut_buf);

    if (is_offline_mode(isp_ctx->block_attr.running_mode) || is_striping_mode(isp_ctx->block_attr.running_mode) ||
        is_pre_online_post_offline(isp_ctx->block_attr.running_mode) || blk_dev >= ISP_MAX_BE_NUM) {
        return TD_NULL;
    }

    size = sizeof(isp_be_lut_wstt_type);

    if (be_lut_buf->lut_stt_buf[blk_dev].vir_addr != TD_NULL) {
        return (td_void *)((td_u8 *)be_lut_buf->lut_stt_buf[blk_dev].vir_addr + size * buf_id);
    }

    phy_addr_temp = ot_ext_addr_read(ot_ext_system_be_lut_stt_buffer_high_addr_read,
        ot_ext_system_be_lut_stt_buffer_low_addr_read, vi_pipe);
    ret = isp_update_be_lut_stt_buf_ctx(vi_pipe, phy_addr_temp);
    if (ret != TD_SUCCESS) {
        return TD_NULL;
    }

    return (td_void *)((td_u8 *)be_lut_buf->lut_stt_buf[blk_dev].vir_addr + size * buf_id);
}

static td_void *isp_get_post_be_vir_addr(ot_vi_pipe vi_pipe, ot_blk_dev blk_dev)
{
    td_s32 ret;
    isp_usr_ctx *isp_ctx = TD_NULL;
    td_void *pre_be_virt_addr[OT_ISP_STRIPING_MAX_NUM] = { TD_NULL };
    td_void *post_be_virt_addr[OT_ISP_STRIPING_MAX_NUM] = { TD_NULL };
    td_void *pre_viproc_virt_addr[OT_ISP_STRIPING_MAX_NUM] = { TD_NULL };
    td_void *post_viproc_virt_addr[OT_ISP_STRIPING_MAX_NUM] = { TD_NULL };

    isp_check_pipe_valid_return(vi_pipe);
    isp_check_be_dev_return(blk_dev);
    isp_get_ctx(vi_pipe, isp_ctx);

    if (isp_ctx->isp_post_be_virt_addr[blk_dev] != TD_NULL) {
        return isp_ctx->isp_post_be_virt_addr[blk_dev];
    }

    ret = isp_get_reg_cfg_virt_addr(vi_pipe, pre_be_virt_addr, post_be_virt_addr, pre_viproc_virt_addr,
        post_viproc_virt_addr);
    if (ret != TD_SUCCESS) {
        isp_err_trace("ISP[%d] Get Be CfgAddr Failed!\n", vi_pipe);
        return TD_NULL;
    }

    isp_check_reg_nullptr_return(post_be_virt_addr[blk_dev]);

    return post_be_virt_addr[blk_dev];
}

td_void *isp_get_pre_be_vir_addr(ot_vi_pipe vi_pipe, ot_blk_dev blk_dev)
{
    td_s32 ret;
    isp_usr_ctx *isp_ctx = TD_NULL;
    td_void *pre_be_virt_addr[OT_ISP_STRIPING_MAX_NUM] = { TD_NULL };
    td_void *post_be_virt_addr[OT_ISP_STRIPING_MAX_NUM] = { TD_NULL };
    td_void *pre_viproc_virt_addr[OT_ISP_STRIPING_MAX_NUM] = { TD_NULL };
    td_void *post_viproc_virt_addr[OT_ISP_STRIPING_MAX_NUM] = { TD_NULL };

    isp_check_pipe_valid_return(vi_pipe);
    isp_check_be_dev_return(blk_dev);
    isp_get_ctx(vi_pipe, isp_ctx);

    if (isp_ctx->isp_pre_be_virt_addr[blk_dev] != TD_NULL) {
        return isp_ctx->isp_pre_be_virt_addr[blk_dev];
    }

    ret = isp_get_reg_cfg_virt_addr(vi_pipe, pre_be_virt_addr, post_be_virt_addr, pre_viproc_virt_addr,
        post_viproc_virt_addr);
    if (ret != TD_SUCCESS) {
        isp_err_trace("ISP[%d] Get Be CfgAddr Failed!\n", vi_pipe);
        return TD_NULL;
    }

    isp_check_reg_nullptr_return(pre_be_virt_addr[blk_dev]);

    return pre_be_virt_addr[blk_dev];
}

static td_void *isp_get_pre_vi_proc_vir_addr(ot_vi_pipe vi_pipe, ot_blk_dev blk_dev)
{
    td_s32 ret;
    isp_usr_ctx *isp_ctx = TD_NULL;
    td_void *pre_be_virt_addr[OT_ISP_STRIPING_MAX_NUM] = { TD_NULL };
    td_void *post_be_virt_addr[OT_ISP_STRIPING_MAX_NUM] = { TD_NULL };
    td_void *pre_viproc_virt_addr[OT_ISP_STRIPING_MAX_NUM] = { TD_NULL };
    td_void *post_viproc_virt_addr[OT_ISP_STRIPING_MAX_NUM] = { TD_NULL };

    isp_check_pipe_valid_return(vi_pipe);
    isp_check_be_dev_return(blk_dev);
    isp_get_ctx(vi_pipe, isp_ctx);

    if (isp_ctx->pre_viproc_virt_addr[blk_dev] != TD_NULL) {
        return isp_ctx->pre_viproc_virt_addr[blk_dev];
    }

    ret = isp_get_reg_cfg_virt_addr(vi_pipe, pre_be_virt_addr, post_be_virt_addr, pre_viproc_virt_addr,
        post_viproc_virt_addr);
    if (ret != TD_SUCCESS) {
        isp_err_trace("ISP[%d] Get viproc CfgAddr Failed!\n", vi_pipe);
        return TD_NULL;
    }

    isp_check_reg_nullptr_return(pre_viproc_virt_addr[blk_dev]);

    return pre_viproc_virt_addr[blk_dev];
}

static td_void *isp_get_post_vi_proc_vir_addr(ot_vi_pipe vi_pipe, ot_blk_dev blk_dev)
{
    td_s32 ret;
    isp_usr_ctx *isp_ctx = TD_NULL;
    td_void *pre_be_virt_addr[OT_ISP_STRIPING_MAX_NUM] = { TD_NULL };
    td_void *post_be_virt_addr[OT_ISP_STRIPING_MAX_NUM] = { TD_NULL };
    td_void *pre_viproc_virt_addr[OT_ISP_STRIPING_MAX_NUM] = { TD_NULL };
    td_void *post_viproc_virt_addr[OT_ISP_STRIPING_MAX_NUM] = { TD_NULL };

    isp_check_pipe_valid_return(vi_pipe);
    isp_check_be_dev_return(blk_dev);
    isp_get_ctx(vi_pipe, isp_ctx);

    if (isp_ctx->post_viproc_virt_addr[blk_dev] != TD_NULL) {
        return isp_ctx->post_viproc_virt_addr[blk_dev];
    }

    ret = isp_get_reg_cfg_virt_addr(vi_pipe, pre_be_virt_addr, post_be_virt_addr, pre_viproc_virt_addr,
        post_viproc_virt_addr);
    if (ret != TD_SUCCESS) {
        isp_err_trace("ISP[%d] Get viproc CfgAddr Failed!\n", vi_pipe);
        return TD_NULL;
    }

    isp_check_reg_nullptr_return(post_viproc_virt_addr[blk_dev]);

    return post_viproc_virt_addr[blk_dev];
}

#ifdef CONFIG_OT_ISP_DETAIL_STATS_SUPPORT
static td_void *isp_get_be_ae_vir_addr(ot_vi_pipe vi_pipe)
{
    td_s32 ret;
    isp_usr_ctx *isp_ctx = TD_NULL;
    td_void *pre_be_virt_addr[OT_ISP_STRIPING_MAX_NUM] = { TD_NULL };
    td_void *post_be_virt_addr[OT_ISP_STRIPING_MAX_NUM] = { TD_NULL };
    td_void *pre_viproc_virt_addr[OT_ISP_STRIPING_MAX_NUM] = { TD_NULL };
    td_void *post_viproc_virt_addr[OT_ISP_STRIPING_MAX_NUM] = { TD_NULL };

    isp_check_fe_pipe_return_null(vi_pipe);
    isp_get_ctx(vi_pipe, isp_ctx);

    if (isp_ctx->isp_be_ae_virt_addr != TD_NULL) {
        return isp_ctx->isp_be_ae_virt_addr;
    }

    ret = isp_get_reg_cfg_virt_addr(vi_pipe, pre_be_virt_addr, post_be_virt_addr, pre_viproc_virt_addr,
        post_viproc_virt_addr);
    if (ret != TD_SUCCESS) {
        isp_err_trace("ISP[%d] Get Be CfgAddr Failed!\n", vi_pipe);
        return TD_NULL;
    }

    isp_check_reg_nullptr_return(isp_ctx->isp_be_ae_virt_addr);

    return isp_ctx->isp_be_ae_virt_addr;
}

static td_void *isp_get_be_awb_vir_addr(ot_vi_pipe vi_pipe)
{
    td_s32 ret;
    isp_usr_ctx *isp_ctx = TD_NULL;
    td_void *pre_be_virt_addr[OT_ISP_STRIPING_MAX_NUM] = { TD_NULL };
    td_void *post_be_virt_addr[OT_ISP_STRIPING_MAX_NUM] = { TD_NULL };
    td_void *pre_viproc_virt_addr[OT_ISP_STRIPING_MAX_NUM] = { TD_NULL };
    td_void *post_viproc_virt_addr[OT_ISP_STRIPING_MAX_NUM] = { TD_NULL };

    isp_check_fe_pipe_return_null(vi_pipe);
    isp_get_ctx(vi_pipe, isp_ctx);

    if (isp_ctx->isp_be_awb_virt_addr != TD_NULL) {
        return isp_ctx->isp_be_awb_virt_addr;
    }

    ret = isp_get_reg_cfg_virt_addr(vi_pipe, pre_be_virt_addr, post_be_virt_addr, pre_viproc_virt_addr,
        post_viproc_virt_addr);
    if (ret != TD_SUCCESS) {
        isp_err_trace("ISP[%d] Get Be CfgAddr Failed!\n", vi_pipe);
        return TD_NULL;
    }

    isp_check_reg_nullptr_return(isp_ctx->isp_be_awb_virt_addr);

    return isp_ctx->isp_be_awb_virt_addr;
}
#endif
static td_s32 isp_bnr_lut_wstt_addr_write(ot_vi_pipe vi_pipe, td_u8 i, td_u8 buf_id, isp_viproc_reg_type *vi_proc_reg)
{
    td_phys_addr_t phy_addr;
    size_t size, addr_offset;
    isp_be_lut_buf *be_lut_buf = TD_NULL;

    be_lut_buf_get_ctx(vi_pipe, be_lut_buf);

    phy_addr = be_lut_buf->lut_stt_buf[i].phy_addr;

    if (phy_addr == 0) {
        return TD_FAILURE;
    }

    size = sizeof(isp_be_lut_wstt_type);
    addr_offset = ISP_BNR_LUT_WSTT_OFFSET + buf_id * size;
    isp_bnr_lut_addr_low_write(vi_proc_reg, get_low_addr(phy_addr + addr_offset));

    return TD_SUCCESS;
}

static td_s32 isp_lsc_lut_wstt_addr_write(ot_vi_pipe vi_pipe, td_u8 i, td_u8 buf_id, isp_viproc_reg_type *vi_proc_reg)
{
    td_phys_addr_t phy_addr;
    size_t size;
    isp_be_lut_buf *be_lut_buf = TD_NULL;

    be_lut_buf_get_ctx(vi_pipe, be_lut_buf);

    phy_addr = be_lut_buf->lut_stt_buf[i].phy_addr;

    if (phy_addr == 0) {
        return TD_FAILURE;
    }

    size = sizeof(isp_be_lut_wstt_type);
    isp_lsc_lut_addr_low_write(vi_proc_reg, get_low_addr(phy_addr + ISP_MLSC_LUT_WSTT_OFFSET + buf_id * size));

    return TD_SUCCESS;
}

static td_s32 isp_gamma_lut_wstt_addr_write(ot_vi_pipe vi_pipe, td_u8 i, td_u8 buf_id, isp_viproc_reg_type *vi_proc_reg)
{
    td_phys_addr_t phy_addr;
    size_t size, addr_offset;
    isp_be_lut_buf *be_lut_buf = TD_NULL;

    be_lut_buf_get_ctx(vi_pipe, be_lut_buf);

    phy_addr = be_lut_buf->lut_stt_buf[i].phy_addr;

    if (phy_addr == 0) {
        return TD_FAILURE;
    }

    size = sizeof(isp_be_lut_wstt_type);
    addr_offset = POST_BE_WSTT_OFFSET + ISP_GAMMA_LUT_WSTT_OFFSET + buf_id * size;
    isp_gamma_lut_addr_low_write(vi_proc_reg, get_low_addr(phy_addr + addr_offset));

    return TD_SUCCESS;
}
static td_s32 isp_dehaze_lut_wstt_addr_write(ot_vi_pipe vi_pipe, td_u8 i, td_u8 buf_id,
    isp_viproc_reg_type *vi_proc_reg)
{
    td_phys_addr_t phy_addr;
    size_t size, addr_offset;
    isp_be_lut_buf *be_lut_buf = TD_NULL;

    be_lut_buf_get_ctx(vi_pipe, be_lut_buf);

    phy_addr = be_lut_buf->lut_stt_buf[i].phy_addr;

    if (phy_addr == 0) {
        return TD_FAILURE;
    }

    size = sizeof(isp_be_lut_wstt_type);
    addr_offset = POST_BE_WSTT_OFFSET + ISP_DEHAZE_LUT_WSTT_OFFSET + buf_id * size;
    isp_dehaze_lut_addr_low_write(vi_proc_reg, get_low_addr(phy_addr + addr_offset));

    return TD_SUCCESS;
}

static td_s32 isp_sharpen_lut_wstt_addr_write(ot_vi_pipe vi_pipe, td_u8 i, td_u8 buf_id,
    isp_viproc_reg_type *vi_proc_reg)
{
    td_phys_addr_t phy_addr;
    size_t size, addr_offset;
    isp_be_lut_buf *be_lut_buf = TD_NULL;

    be_lut_buf_get_ctx(vi_pipe, be_lut_buf);

    phy_addr = be_lut_buf->lut_stt_buf[i].phy_addr;

    if (phy_addr == 0) {
        return TD_FAILURE;
    }

    size = sizeof(isp_be_lut_wstt_type);
    addr_offset = POST_BE_WSTT_OFFSET + ISP_SHARPEN_LUT_WSTT_OFFSET + buf_id * size;
    isp_sharpen_lut_addr_low_write(vi_proc_reg, get_low_addr(phy_addr + addr_offset));

    return TD_SUCCESS;
}

static td_s32 isp_ldci_lut_wstt_addr_write(ot_vi_pipe vi_pipe, td_u8 i, td_u8 buf_id, isp_viproc_reg_type *vi_proc_reg)
{
    td_phys_addr_t phy_addr;
    size_t size, addr_offset;
    isp_be_lut_buf *be_lut_buf = TD_NULL;

    be_lut_buf_get_ctx(vi_pipe, be_lut_buf);

    phy_addr = be_lut_buf->lut_stt_buf[i].phy_addr;

    if (phy_addr == 0) {
        return TD_FAILURE;
    }

    size = sizeof(isp_be_lut_wstt_type);
    addr_offset = POST_BE_WSTT_OFFSET + ISP_LDCI_LUT_WSTT_OFFSET + buf_id * size;
    isp_ldci_lut_addr_low_write(vi_proc_reg, get_low_addr(phy_addr + addr_offset));

    return TD_SUCCESS;
}

static td_s32 isp_ca_lut_wstt_addr_write(ot_vi_pipe vi_pipe, td_u8 i, td_u8 buf_id, isp_viproc_reg_type *vi_proc_reg)
{
    td_phys_addr_t phy_addr;
    size_t size, offset;
    isp_be_lut_buf *be_lut_buf = TD_NULL;

    be_lut_buf_get_ctx(vi_pipe, be_lut_buf);

    phy_addr = be_lut_buf->lut_stt_buf[i].phy_addr;
    if (phy_addr == 0) {
        return TD_FAILURE;
    }

    size = sizeof(isp_be_lut_wstt_type);
    offset = POST_BE_WSTT_OFFSET + ISP_CA_LUT_WSTT_OFFSET;
    isp_ca_lut_addr_low_write(vi_proc_reg, get_low_addr(phy_addr + offset + buf_id * size));
    isp_ca_lut_width_word_write(vi_proc_reg, OT_ISP_CA_LUT_WIDTH_WORD_DEFAULT);

    return TD_SUCCESS;
}

static td_s32 isp_cc_lut_wstt_addr_write(ot_vi_pipe vi_pipe, td_u8 i, td_u8 buf_id, isp_viproc_reg_type *vi_proc_reg)
{
    td_phys_addr_t phy_addr;
    size_t size, addr_offset;
    isp_be_lut_buf *be_lut_buf = TD_NULL;

    be_lut_buf_get_ctx(vi_pipe, be_lut_buf);

    phy_addr = be_lut_buf->lut_stt_buf[i].phy_addr;

    if (phy_addr == 0) {
        return TD_FAILURE;
    }

    size = sizeof(isp_be_lut_wstt_type);
    addr_offset = POST_BE_WSTT_OFFSET + ISP_CC_LUT_WSTT_OFFSET + buf_id * size;
    isp_cc_lut_addr_low_write(vi_proc_reg, get_low_addr(phy_addr + addr_offset));

    return TD_SUCCESS;
}

static td_s32 isp_fe_dg_reg_config(ot_vi_pipe vi_pipe, isp_reg_cfg *reg_cfg_info)
{
    td_u16 i;
    ot_vi_pipe vi_pipe_bind;
    isp_fe_dg_dyna_cfg *dg2_dyna_reg_cfg = TD_NULL;
    isp_fe_dg_dyna_cfg *dg_dyna_reg_cfg = TD_NULL;
    isp_usr_ctx *isp_ctx = TD_NULL;
    isp_fe_reg_type *fe_reg = TD_NULL;

    isp_check_pipe_return(vi_pipe);
    isp_get_ctx(vi_pipe, isp_ctx);

    dg2_dyna_reg_cfg = &reg_cfg_info->alg_reg_cfg[0].fe_dg2_reg_cfg.dyna_reg_cfg;
    dg_dyna_reg_cfg = &reg_cfg_info->alg_reg_cfg[0].fe_dg_reg_cfg.dyna_reg_cfg;

    if (isp_ctx->wdr_attr.is_mast_pipe != TD_TRUE) {
        return TD_SUCCESS;
    }

    for (i = 0; i < isp_ctx->wdr_attr.pipe_num; i++) {
        vi_pipe_bind = isp_ctx->wdr_attr.pipe_id[i];
        isp_check_no_fe_pipe_return(vi_pipe_bind);
        isp_dist_trans_pipe(&vi_pipe_bind);

        fe_reg = (isp_fe_reg_type *)isp_get_fe_vir_addr(vi_pipe_bind);
        isp_check_pointer_return(fe_reg);

        if (reg_cfg_info->cfg_key.bit1_fe_dg_cfg) {
            isp_fe_dg2_en_write(fe_reg, reg_cfg_info->alg_reg_cfg[0].fe_dg2_reg_cfg.dg_en);

            if (dg2_dyna_reg_cfg->resh) {
                isp_fe_dg2_rgain_write(fe_reg, dg2_dyna_reg_cfg->gain_r[i]);
                isp_fe_dg2_grgain_write(fe_reg, dg2_dyna_reg_cfg->gain_gr[i]);
                isp_fe_dg2_gbgain_write(fe_reg, dg2_dyna_reg_cfg->gain_gb[i]);
                isp_fe_dg2_bgain_write(fe_reg, dg2_dyna_reg_cfg->gain_b[i]);
                isp_fe_dg2_clip_value_write(fe_reg, dg2_dyna_reg_cfg->clip_value);
            }
#ifdef CONFIG_OT_AIBNR_SUPPORT
            isp_fe_dg_en_write(fe_reg, reg_cfg_info->alg_reg_cfg[0].fe_dg_reg_cfg.dg_en);
            if (dg_dyna_reg_cfg->resh) {
                isp_fe_dg_rgain_write(fe_reg, dg_dyna_reg_cfg->gain_r[i]);
                isp_fe_dg_grgain_write(fe_reg, dg_dyna_reg_cfg->gain_gr[i]);
                isp_fe_dg_gbgain_write(fe_reg, dg_dyna_reg_cfg->gain_gb[i]);
                isp_fe_dg_bgain_write(fe_reg, dg_dyna_reg_cfg->gain_b[i]);
                isp_fe_dg_clip_value_write(fe_reg, dg_dyna_reg_cfg->clip_value);
            }
#endif
        }
    }

    dg2_dyna_reg_cfg->resh = TD_FALSE;
    dg_dyna_reg_cfg->resh = TD_FALSE;
    reg_cfg_info->cfg_key.bit1_fe_dg_cfg = 0;

    return TD_SUCCESS;
}
#ifdef CONFIG_OT_AIBNR_SUPPORT
static td_s32 isp_agamma_lut_reg_config(ot_vi_pipe vi_pipe, isp_fe_reg_type *fe_reg,
    isp_agamma_dyna_cfg *dyna_reg_cfg)
{
    td_u16 j;
    isp_fe_gamma_lut_waddr_write(fe_reg, 0);

    for (j = 0; j < ISP_AGAMMA_NODE_NUM; j++) {
        isp_fe_gamma_lut_wdata_write(fe_reg, dyna_reg_cfg->agamma_lut[j]);
    }

    return TD_SUCCESS;
}

static td_s32 isp_fe_agamma_reg_config(ot_vi_pipe vi_pipe, isp_reg_cfg *reg_cfg_info)
{
    td_u32 i;
    ot_vi_pipe vi_pipe_bind;
    isp_agamma_dyna_cfg *dyna_reg_cfg = TD_NULL;
    isp_agamma_static_cfg *static_reg_cfg = TD_NULL;
    isp_usr_ctx *isp_ctx = TD_NULL;
    isp_fe_reg_type *fe_reg = TD_NULL;
    isp_get_ctx(vi_pipe, isp_ctx);

    isp_check_pipe_return(vi_pipe);
    isp_get_ctx(vi_pipe, isp_ctx);

    dyna_reg_cfg = &reg_cfg_info->alg_reg_cfg[0].agamma_reg_cfg.dyna_reg_cfg;
    static_reg_cfg = &reg_cfg_info->alg_reg_cfg[0].agamma_reg_cfg.static_reg_cfg;

    if (isp_ctx->wdr_attr.is_mast_pipe != TD_TRUE || vi_pipe != 0) {
        return TD_SUCCESS;
    }

    for (i = 0; i < isp_ctx->wdr_attr.pipe_num; i++) {
        vi_pipe_bind = isp_ctx->wdr_attr.pipe_id[i];
        isp_check_no_fe_pipe_return(vi_pipe_bind);
        fe_reg = (isp_fe_reg_type *)isp_get_fe_vir_addr(vi_pipe_bind);
        isp_check_pointer_return(fe_reg);

        if (reg_cfg_info->cfg_key.bit1_agamma_cfg) {
            isp_fe_gamma_en_write(fe_reg, reg_cfg_info->alg_reg_cfg[0].agamma_reg_cfg.enable);

            if (static_reg_cfg->resh) {
                isp_fe_gamma_bitw_write(fe_reg, static_reg_cfg->bitw);
                static_reg_cfg->resh = TD_FALSE;
            }

            if (dyna_reg_cfg->resh) {
                isp_agamma_lut_reg_config(vi_pipe, fe_reg, dyna_reg_cfg);
            }
        }
    }

    dyna_reg_cfg->resh = TD_FALSE;
    reg_cfg_info->cfg_key.bit1_agamma_cfg = 0;

    return TD_SUCCESS;
}
#endif
static td_void isp_fe_blc_static_reg_config(ot_vi_pipe vi_pipe_bind, isp_fe_reg_type *fe_reg,
                                            const isp_fe_blc_static_cfg *static_cfg)
{
    ot_unused(vi_pipe_bind);
    if (static_cfg->resh_static) {
        /* Fe Dg */
        isp_fe_dg2_blc_en_write(fe_reg, static_cfg->fe_dg_blc.blc_in, static_cfg->fe_dg_blc.blc_out);
        isp_fe_dg_blc_en_write(fe_reg, static_cfg->fe_dg_blc.blc_in, static_cfg->fe_dg_blc.blc_out);
        isp_fe_wb_blc_en_write(fe_reg, static_cfg->fe_wb_blc.blc_in, static_cfg->fe_wb_blc.blc_out);

        /* Fe Ae */
        isp_fe_ae_blc_en_write(fe_reg, static_cfg->fe_ae_blc.blc_in);
        /* Fe RC */
        isp_fe_rc_blc_in_en_write(fe_reg, static_cfg->rc_blc.blc_in);
        isp_fe_rc_blc_out_en_write(fe_reg, static_cfg->rc_blc.blc_out);
        /* FE BLC */
        isp_fe_blc1_en_write(fe_reg, static_cfg->fe_blc.blc_in);
    }
}

static td_void isp_fe_blc_dyna_reg_config(ot_vi_pipe vi_pipe_bind, td_u8 wdr_idx, isp_fe_reg_type *fe_reg,
                                          const isp_fe_blc_dyna_cfg *dyna_cfg)
{
    isp_fe_blc1_en_write(fe_reg, dyna_cfg->fe_blc1_en);
    isp_fe_blc1_offset_write(fe_reg, dyna_cfg->fe_blc[wdr_idx].blc);

    isp_fe_dg2_offset_write(fe_reg, dyna_cfg->fe_dg_blc[wdr_idx].blc);
    isp_fe_dg_offset_write(fe_reg, dyna_cfg->fe_dg_blc[wdr_idx].blc);
    isp_fe_wb_offset_write(fe_reg, dyna_cfg->fe_wb_blc[wdr_idx].blc);
    isp_fe_ae_offset_write(fe_reg, dyna_cfg->fe_ae_blc[wdr_idx].blc);

    isp_fe_rc_blc_r_write(fe_reg, dyna_cfg->rc_blc.blc[0]); /* array index 0 */
    isp_fe_rc_blc_gr_write(fe_reg, dyna_cfg->rc_blc.blc[1]); /* array index 1 */
    isp_fe_rc_blc_gb_write(fe_reg, dyna_cfg->rc_blc.blc[2]); /* array index 2 */
    isp_fe_rc_blc_b_write(fe_reg, dyna_cfg->rc_blc.blc[3]); /* array index 3 */
}

static td_s32 isp_fe_blc_reg_config(ot_vi_pipe vi_pipe, isp_reg_cfg *reg_cfg_info)
{
    td_u8 i;
    ot_vi_pipe vi_pipe_bind;
    isp_fe_blc_static_cfg *static_cfg = TD_NULL;
    isp_fe_blc_dyna_cfg *dyna_cfg = TD_NULL;
    isp_usr_ctx *isp_ctx = TD_NULL;
    isp_fe_reg_type *fe_reg = TD_NULL;

    isp_get_ctx(vi_pipe, isp_ctx);

    static_cfg = &reg_cfg_info->alg_reg_cfg[0].fe_blc_cfg.static_blc;
    dyna_cfg = &reg_cfg_info->alg_reg_cfg[0].fe_blc_cfg.dyna_blc;

    if (isp_ctx->wdr_attr.is_mast_pipe != TD_TRUE) {
        return TD_SUCCESS;
    }

    for (i = 0; i < isp_ctx->wdr_attr.pipe_num; i++) {
        vi_pipe_bind = isp_ctx->wdr_attr.pipe_id[i];
        isp_check_no_fe_pipe_return(vi_pipe_bind);
        isp_dist_trans_pipe(&vi_pipe_bind);

        fe_reg = (isp_fe_reg_type *)isp_get_fe_vir_addr(vi_pipe_bind);
        isp_check_pointer_return(fe_reg);

        if (reg_cfg_info->cfg_key.bit1_fe_blc_cfg) {
            isp_fe_blc_static_reg_config(vi_pipe_bind, fe_reg, static_cfg);
            if (reg_cfg_info->alg_reg_cfg[0].fe_blc_cfg.resh_dyna_init == TD_TRUE) {
                isp_fe_blc_dyna_reg_config(vi_pipe_bind, i, fe_reg, dyna_cfg);
            }
        }
    }

    static_cfg->resh_static = TD_FALSE;
    reg_cfg_info->alg_reg_cfg[0].fe_blc_cfg.resh_dyna_init = TD_FALSE;
    reg_cfg_info->cfg_key.bit1_fe_blc_cfg = 0;

    return TD_SUCCESS;
}

static td_void isp_fe_ae_weight_config(isp_fe_reg_type *fe_reg, isp_ae_dyna_cfg *dyna_reg_cfg)
{
    td_u16 j, k;
    td_u32 table_weight_tmp;
    td_u32 combin_weight = 0;
    td_u32 combin_weight_num = 0;

    isp_fe_ae_wei_waddr_write(fe_reg, 0);

    for (j = 0; j < OT_ISP_AE_ZONE_ROW; j++) {
        for (k = 0; k < OT_ISP_AE_ZONE_COLUMN; k++) {
            table_weight_tmp = (td_u32)dyna_reg_cfg->fe_weight_table[j][k];
            combin_weight |= (table_weight_tmp << (8 * combin_weight_num)); /* weightTmp shift left 8 */
            combin_weight_num++;

            if (combin_weight_num == OT_ISP_AE_WEI_COMBIN_COUNT) {
                isp_fe_ae_wei_wdata_write(fe_reg, combin_weight);
                combin_weight_num = 0;
                combin_weight = 0;
            }
        }
    }

    if ((combin_weight_num != OT_ISP_AE_WEI_COMBIN_COUNT) && (combin_weight_num != 0)) {
        isp_fe_ae_wei_wdata_write(fe_reg, combin_weight);
    }
}

static td_s32 isp_fe_ae_reg_config(ot_vi_pipe vi_pipe, isp_reg_cfg *reg_cfg_info)
{
    td_bool lut_update_en = TD_FALSE;
    td_u16 i, crop_width, crop_x;
    ot_vi_pipe vi_pipe_bind;
    isp_ae_static_cfg *static_reg = TD_NULL;
    isp_ae_dyna_cfg *dyna_reg = TD_NULL;
    isp_fe_reg_type *fe_reg = TD_NULL;
    isp_usr_ctx *isp_ctx = TD_NULL;

    isp_get_ctx(vi_pipe, isp_ctx);

    if (isp_ctx->wdr_attr.is_mast_pipe != TD_TRUE) {
        return TD_SUCCESS;
    }

    if (reg_cfg_info->cfg_key.bit1_ae_cfg1) {
        for (i = 0; i < isp_ctx->wdr_attr.pipe_num; i++) {
            vi_pipe_bind = isp_ctx->wdr_attr.pipe_id[i];
            isp_check_no_fe_pipe_return(vi_pipe_bind);
            isp_dist_trans_pipe(&vi_pipe_bind);

            fe_reg = (isp_fe_reg_type *)isp_get_fe_vir_addr(vi_pipe_bind);
            isp_check_pointer_return(fe_reg);
            /* ae fe static */
            static_reg = &reg_cfg_info->alg_reg_cfg[0].ae_reg_cfg.static_reg_cfg;
            dyna_reg = &reg_cfg_info->alg_reg_cfg[0].ae_reg_cfg.dyna_reg_cfg;

            isp_fe_ae1_en_write(fe_reg, static_reg->fe_enable);
            crop_x = is_hrs_on(vi_pipe_bind) ? (static_reg->fe_crop_pos_x >> 1) : static_reg->fe_crop_pos_x;
            isp_fe_ae_crop_pos_write(fe_reg, crop_x, static_reg->fe_crop_pos_y);
            crop_width = is_hrs_on(vi_pipe_bind) ? (static_reg->fe_crop_out_width >> 1) : static_reg->fe_crop_out_width;
            isp_fe_ae_crop_outsize_write(fe_reg, crop_width - 1, static_reg->fe_crop_out_height - 1);

            /* ae fe dynamic */
            isp_fe_ae_zone_write(fe_reg, dyna_reg->fe_weight_table_width, dyna_reg->fe_weight_table_height);
            isp_fe_ae_skip_crg_write(fe_reg, dyna_reg->fe_hist_skip_x, dyna_reg->fe_hist_offset_x,
                dyna_reg->fe_hist_skip_y, dyna_reg->fe_hist_offset_y);
            isp_fe_ae_bitmove_write(fe_reg, dyna_reg->fe_bit_move);

            isp_fe_ae_weight_config(fe_reg, dyna_reg);
            lut_update_en = dyna_reg->fe_weight_table_update;
        }
    }

    reg_cfg_info->alg_reg_cfg[0].fe_lut_update_cfg.ae1_lut_update = lut_update_en;
    return TD_SUCCESS;
}

static td_void isp_combine_ae_wgt_calc(isp_ae_dyna_cfg *dyna_reg_be_cfg)
{
    td_u16 j, k, m;
    td_u32 combin_weight, combin_weight_num;

    m = 0;
    combin_weight = 0;
    combin_weight_num = 0;

    for (j = 0; j < dyna_reg_be_cfg->be_weight_table_height; j++) {
        for (k = 0; k < dyna_reg_be_cfg->be_weight_table_width; k++) {
            combin_weight |= ((td_u32)dyna_reg_be_cfg->be_weight_table[j][k] << (8 * combin_weight_num)); /* 8 */
            combin_weight_num++;
            if (combin_weight_num == OT_ISP_AE_WEI_COMBIN_COUNT) {
                dyna_reg_be_cfg->be_combine_wgt[m++] = combin_weight;
                combin_weight_num = 0;
                combin_weight = 0;
            }
        }
    }

    if ((combin_weight_num != OT_ISP_AE_WEI_COMBIN_COUNT) && (combin_weight_num != 0)) {
        dyna_reg_be_cfg->be_combine_wgt[m++] = combin_weight;
    }
}

static td_void isp_ae_online_switch_write(isp_viproc_reg_type *viproc_reg,
    isp_be_reg_type *be_reg, isp_ae_reg_cfg *ae_reg_cfg)
{
    isp_ae_static_cfg *static_reg_cfg = &ae_reg_cfg->static_reg_cfg;
    isp_ae_dyna_cfg *dyna_reg_cfg = &ae_reg_cfg->dyna_reg_cfg;
    if (dyna_reg_cfg->be_ae_sel != dyna_reg_cfg->pre_be_ae_sel) {
        isp_ae_en_write(viproc_reg, TD_FALSE);
    } else {
        isp_ae_en_write(viproc_reg, static_reg_cfg->be_enable);
        isp_ae_sel_write(be_reg, dyna_reg_cfg->be_ae_sel);
    }
    dyna_reg_cfg->pre_be_ae_sel = dyna_reg_cfg->be_ae_sel;
}

static td_void isp_ae_reg_write(isp_viproc_reg_type *viproc_reg, isp_be_reg_type *be_reg, isp_ae_reg_cfg *ae_reg_cfg)
{
    td_u16 m;
    isp_ae_static_cfg *static_reg_cfg = &ae_reg_cfg->static_reg_cfg;
    isp_ae_dyna_cfg *dyna_reg_cfg = &ae_reg_cfg->dyna_reg_cfg;
    td_u16 ae_be_weight_size = (OT_ISP_BE_AE_ZONE_ROW * OT_ISP_BE_AE_ZONE_COLUMN) / OT_ISP_AE_WEI_COMBIN_COUNT;
    isp_combine_ae_wgt_calc(dyna_reg_cfg);

    isp_ae_crop_pos_x_write(be_reg, static_reg_cfg->be_crop_pos_x);
    isp_ae_crop_pos_y_write(be_reg, static_reg_cfg->be_crop_pos_y);
    isp_ae_crop_out_width_write(be_reg, static_reg_cfg->be_crop_out_width - 1);
    isp_ae_crop_out_height_write(be_reg, static_reg_cfg->be_crop_out_height - 1);
    /* ae be dynamic */
    isp_ae_hnum_write(be_reg, dyna_reg_cfg->be_weight_table_width);
    isp_ae_vnum_write(be_reg, dyna_reg_cfg->be_weight_table_height);
    isp_ae_skip_x_write(be_reg, dyna_reg_cfg->be_hist_skip_x);
    isp_ae_offset_x_write(be_reg, dyna_reg_cfg->be_hist_offset_x);
    isp_ae_skip_y_write(be_reg, dyna_reg_cfg->be_hist_skip_y);
    isp_ae_offset_y_write(be_reg, dyna_reg_cfg->be_hist_offset_y);
    isp_ae_bitmove_write(be_reg, dyna_reg_cfg->be_bit_move);
    isp_ae_hist_gamma_mode_write(be_reg, dyna_reg_cfg->be_hist_gamma_mode);
    isp_ae_aver_gamma_mode_write(be_reg, dyna_reg_cfg->be_aver_gamma_mode);
    isp_ae_fourplanemode_write(be_reg, dyna_reg_cfg->be_four_plane_mode);
    if (dyna_reg_cfg->is_online == TD_TRUE) {
        isp_ae_online_switch_write(viproc_reg, be_reg, ae_reg_cfg);
        isp_ae_wei_waddr_write(be_reg, 0);
        for (m = 0; m < ae_be_weight_size; m++) {
            isp_ae_wei_wdata_write(be_reg, dyna_reg_cfg->be_combine_wgt[m]);
        }
    } else {
        isp_ae_en_write(viproc_reg, static_reg_cfg->be_enable);
        isp_ae_sel_write(be_reg, dyna_reg_cfg->be_ae_sel);
        isp_ae_weight_write(&be_reg->be_lut.be_apb_lut, dyna_reg_cfg->be_combine_wgt,
            ae_be_weight_size);
    }
}

static td_s32 isp_ae_reg_config(ot_vi_pipe vi_pipe, isp_reg_cfg *reg_cfg_info, td_u8 i)
{
    td_bool lut_update_en = TD_FALSE;
    td_bool offline_mode;
    isp_ae_dyna_cfg *dyna_reg_cfg = TD_NULL;
    isp_viproc_reg_type *viproc_reg = TD_NULL;
    isp_be_reg_type *be_reg = TD_NULL;

    dyna_reg_cfg = &reg_cfg_info->alg_reg_cfg[i].ae_reg_cfg.dyna_reg_cfg;
    offline_mode = (dyna_reg_cfg->is_online == TD_FALSE);
    if (reg_cfg_info->cfg_key.bit1_ae_cfg1) {
        if (dyna_reg_cfg->is_in_pre_be == TD_FALSE) {
            viproc_reg = (isp_viproc_reg_type *)isp_get_post_vi_proc_vir_addr(vi_pipe, i);
            be_reg = (isp_be_reg_type *)isp_get_post_be_vir_addr(vi_pipe, i);
        } else {
            viproc_reg = (isp_viproc_reg_type *)isp_get_pre_vi_proc_vir_addr(vi_pipe, i);
            be_reg = (isp_be_reg_type *)isp_get_pre_be_vir_addr(vi_pipe, i);
        }
        isp_check_pointer_return(viproc_reg);
        isp_check_pointer_return(be_reg);

        isp_ae_reg_write(viproc_reg, be_reg, &reg_cfg_info->alg_reg_cfg[i].ae_reg_cfg);

        isp_check_pointer_return(viproc_reg);
        isp_check_pointer_return(be_reg);

        lut_update_en = reg_cfg_info->alg_reg_cfg[i].ae_reg_cfg.dyna_reg_cfg.be_weight_table_update;
    }

    reg_cfg_info->alg_reg_cfg[i].be_lut_update_cfg.ae_lut_update = lut_update_en || offline_mode;

    return TD_SUCCESS;
}

#ifdef CONFIG_OT_ISP_AF_SUPPORT
static td_void isp_be_af_iir_gain_write(isp_post_be_reg_type *be_reg, isp_af_reg_cfg *af_reg_be_cfg)
{
    isp_af_iirg0_0_write(be_reg, af_reg_be_cfg->iir_gain0_group0);
    isp_af_iirg0_1_write(be_reg, af_reg_be_cfg->iir_gain0_group1);
    isp_af_iirg1_0_write(be_reg, af_reg_be_cfg->iir_gain1_group0);
    isp_af_iirg1_1_write(be_reg, af_reg_be_cfg->iir_gain1_group1);
    isp_af_iirg2_0_write(be_reg, af_reg_be_cfg->iir_gain2_group0);
    isp_af_iirg2_1_write(be_reg, af_reg_be_cfg->iir_gain2_group1);
    isp_af_iirg3_0_write(be_reg, af_reg_be_cfg->iir_gain3_group0);
    isp_af_iirg3_1_write(be_reg, af_reg_be_cfg->iir_gain3_group1);
    isp_af_iirg4_0_write(be_reg, af_reg_be_cfg->iir_gain4_group0);
    isp_af_iirg4_1_write(be_reg, af_reg_be_cfg->iir_gain4_group1);
    isp_af_iirg5_0_write(be_reg, af_reg_be_cfg->iir_gain5_group0);
    isp_af_iirg5_1_write(be_reg, af_reg_be_cfg->iir_gain5_group1);
    isp_af_iirg6_0_write(be_reg, af_reg_be_cfg->iir_gain6_group0);
    isp_af_iirg6_1_write(be_reg, af_reg_be_cfg->iir_gain6_group1);
}

static td_void isp_be_af_iir_shift_write(isp_post_be_reg_type *be_reg, isp_af_reg_cfg *af_reg_be_cfg)
{
    isp_af_iirshift0_0_write(be_reg, af_reg_be_cfg->iir0_shift_group0);
    isp_af_iirshift0_1_write(be_reg, af_reg_be_cfg->iir1_shift_group0);
    isp_af_iirshift0_2_write(be_reg, af_reg_be_cfg->iir2_shift_group0);
    isp_af_iirshift0_3_write(be_reg, af_reg_be_cfg->iir3_shift_group0);
    isp_af_iirshift1_0_write(be_reg, af_reg_be_cfg->iir0_shift_group1);
    isp_af_iirshift1_1_write(be_reg, af_reg_be_cfg->iir1_shift_group1);
    isp_af_iirshift1_2_write(be_reg, af_reg_be_cfg->iir2_shift_group1);
    isp_af_iirshift1_3_write(be_reg, af_reg_be_cfg->iir3_shift_group1);
}

static td_void isp_be_af_fir_gain_write(isp_post_be_reg_type *be_reg, isp_af_reg_cfg *af_reg_be_cfg)
{
    isp_af_firh0_0_write(be_reg, af_reg_be_cfg->fir_h_gain0_group0);
    isp_af_firh0_1_write(be_reg, af_reg_be_cfg->fir_h_gain0_group1);

    isp_af_firh1_0_write(be_reg, af_reg_be_cfg->fir_h_gain1_group0);
    isp_af_firh1_1_write(be_reg, af_reg_be_cfg->fir_h_gain1_group1);

    isp_af_firh2_0_write(be_reg, af_reg_be_cfg->fir_h_gain2_group0);
    isp_af_firh2_1_write(be_reg, af_reg_be_cfg->fir_h_gain2_group1);

    isp_af_firh3_0_write(be_reg, af_reg_be_cfg->fir_h_gain3_group0);
    isp_af_firh3_1_write(be_reg, af_reg_be_cfg->fir_h_gain3_group1);

    isp_af_firh4_0_write(be_reg, af_reg_be_cfg->fir_h_gain4_group0);
    isp_af_firh4_1_write(be_reg, af_reg_be_cfg->fir_h_gain4_group1);
}

static td_void isp_be_af_crop_write(isp_post_be_reg_type *be_reg, isp_af_reg_cfg *af_reg_be_cfg)
{
    isp_af_crop_en_write(be_reg, af_reg_be_cfg->crop_enable);
    if (af_reg_be_cfg->crop_enable) {
        isp_af_pos_x_write(be_reg, af_reg_be_cfg->crop_pos_x);
        isp_af_pos_y_write(be_reg, af_reg_be_cfg->crop_pos_y);
        isp_af_crop_hsize_write(be_reg, af_reg_be_cfg->crop_h_size - 1);
        isp_af_crop_vsize_write(be_reg, af_reg_be_cfg->crop_v_size - 1);
    }
}

static td_void isp_be_af_raw_cfg_write(isp_post_be_reg_type *be_reg, isp_af_reg_cfg *af_reg_be_cfg)
{
    isp_af_raw_mode_write(be_reg, af_reg_be_cfg->raw_mode);
    isp_af_gamma_write(be_reg, af_reg_be_cfg->gamma);
    isp_af_bayer_mode_write(be_reg, af_reg_be_cfg->bayer_mode);
}

static td_void isp_be_af_level_depend_gain_cfg_write(isp_post_be_reg_type *be_reg, isp_af_reg_cfg *af_reg_be_cfg)
{
    isp_af_iir0_ldg_en_write(be_reg, af_reg_be_cfg->iir0_ldg_enable);
    isp_af_iir_thre0_l_write(be_reg, af_reg_be_cfg->iir_thre0_low);
    isp_af_iir_thre0_h_write(be_reg, af_reg_be_cfg->iir_thre0_high);
    isp_af_iir_slope0_l_write(be_reg, af_reg_be_cfg->iir_slope0_low);
    isp_af_iir_slope0_h_write(be_reg, af_reg_be_cfg->iir_slope0_high);
    isp_af_iir_gain0_l_write(be_reg, af_reg_be_cfg->iir_gain0_low);
    isp_af_iir_gain0_h_write(be_reg, af_reg_be_cfg->iir_gain0_high);

    isp_af_iir1_ldg_en_write(be_reg, af_reg_be_cfg->iir1_ldg_enable);
    isp_af_iir_thre1_l_write(be_reg, af_reg_be_cfg->iir_thre1_low);
    isp_af_iir_thre1_h_write(be_reg, af_reg_be_cfg->iir_thre1_high);
    isp_af_iir_slope1_l_write(be_reg, af_reg_be_cfg->iir_slope1_low);
    isp_af_iir_slope1_h_write(be_reg, af_reg_be_cfg->iir_slope1_high);
    isp_af_iir_gain1_l_write(be_reg, af_reg_be_cfg->iir_gain1_low);
    isp_af_iir_gain1_h_write(be_reg, af_reg_be_cfg->iir_gain1_high);

    isp_af_fir0_ldg_en_write(be_reg, af_reg_be_cfg->fir0_ldg_enable);
    isp_af_fir_thre0_l_write(be_reg, af_reg_be_cfg->fir_thre0_low);
    isp_af_fir_thre0_h_write(be_reg, af_reg_be_cfg->fir_thre0_high);
    isp_af_fir_slope0_l_write(be_reg, af_reg_be_cfg->fir_slope0_low);
    isp_af_fir_slope0_h_write(be_reg, af_reg_be_cfg->fir_slope0_high);
    isp_af_fir_gain0_l_write(be_reg, af_reg_be_cfg->fir_gain0_low);
    isp_af_fir_gain0_h_write(be_reg, af_reg_be_cfg->fir_gain0_high);

    isp_af_fir1_ldg_en_write(be_reg, af_reg_be_cfg->fir1_ldg_enable);
    isp_af_fir_thre1_l_write(be_reg, af_reg_be_cfg->fir_thre1_low);
    isp_af_fir_thre1_h_write(be_reg, af_reg_be_cfg->fir_thre1_high);
    isp_af_fir_slope1_l_write(be_reg, af_reg_be_cfg->fir_slope1_low);
    isp_af_fir_slope1_h_write(be_reg, af_reg_be_cfg->fir_slope1_high);
    isp_af_fir_gain1_l_write(be_reg, af_reg_be_cfg->fir_gain1_low);
    isp_af_fir_gain1_h_write(be_reg, af_reg_be_cfg->fir_gain1_high);
}

static td_void isp_be_af_coring_cfg_write(isp_post_be_reg_type *be_reg, isp_af_reg_cfg *af_reg_be_cfg)
{
    isp_af_iir_thre0_c_write(be_reg, af_reg_be_cfg->iir_thre0_coring);
    isp_af_iir_slope0_c_write(be_reg, af_reg_be_cfg->iir_slope0_coring);
    isp_af_iir_peak0_c_write(be_reg, af_reg_be_cfg->iir_peak0_coring);

    isp_af_iir_thre1_c_write(be_reg, af_reg_be_cfg->iir_thre1_coring);
    isp_af_iir_slope1_c_write(be_reg, af_reg_be_cfg->iir_slope1_coring);
    isp_af_iir_peak1_c_write(be_reg, af_reg_be_cfg->iir_peak1_coring);

    isp_af_fir_thre0_c_write(be_reg, af_reg_be_cfg->fir_thre0_coring);
    isp_af_fir_slope0_c_write(be_reg, af_reg_be_cfg->fir_slope0_coring);
    isp_af_fir_peak0_c_write(be_reg, af_reg_be_cfg->fir_peak0_coring);

    isp_af_fir_thre1_c_write(be_reg, af_reg_be_cfg->fir_thre1_coring);
    isp_af_fir_slope1_c_write(be_reg, af_reg_be_cfg->fir_slope1_coring);
    isp_af_fir_peak1_c_write(be_reg, af_reg_be_cfg->fir_peak1_coring);
}

static td_void isp_be_af_output_shift_cfg_write(isp_post_be_reg_type *be_reg, isp_af_reg_cfg *af_reg_be_cfg)
{
    isp_af_acc_shift0_h_write(be_reg, af_reg_be_cfg->acc_shift0_h);
    isp_af_acc_shift1_h_write(be_reg, af_reg_be_cfg->acc_shift1_h);
    isp_af_acc_shift0_v_write(be_reg, af_reg_be_cfg->acc_shift0_v);
    isp_af_acc_shift1_v_write(be_reg, af_reg_be_cfg->acc_shift1_v);
    isp_af_acc_shift_y_write(be_reg, af_reg_be_cfg->acc_shift_y);
    isp_af_cnt_shift_y_write(be_reg, af_reg_be_cfg->shift_count_y);
    isp_af_cnt_shift0_v_write(be_reg, ISP_AF_CNT_SHIFT0_V_DEFAULT);
    isp_af_cnt_shift0_h_write(be_reg, 0x0);
    isp_af_cnt_shift1_h_write(be_reg, 0x0);
    isp_af_cnt_shift1_v_write(be_reg, 0x0);
}

static td_void isp_be_af_reg_write(isp_post_be_reg_type *be_reg, isp_af_reg_cfg *af_reg_be_cfg)
{
    isp_af_lpf_en_write(be_reg, af_reg_be_cfg->lpf_enable);
    isp_af_fir0_lpf_en_write(be_reg, af_reg_be_cfg->fir0_lpf_enable);
    isp_af_fir1_lpf_en_write(be_reg, af_reg_be_cfg->fir1_lpf_enable);
    isp_af_iir0_ds_en_write(be_reg, af_reg_be_cfg->iir0_ds_enable);
    isp_af_iir1_ds_en_write(be_reg, af_reg_be_cfg->iir1_ds_enable);
    isp_af_iir_dilate0_write(be_reg, af_reg_be_cfg->iir_dilate0);
    isp_af_iir_dilate1_write(be_reg, af_reg_be_cfg->iir_dilate1);
    isp_af_iirplg_0_write(be_reg, af_reg_be_cfg->iir_plg_group0);
    isp_af_iirpls_0_write(be_reg, af_reg_be_cfg->iir_pls_group0);
    isp_af_iirplg_1_write(be_reg, af_reg_be_cfg->iir_plg_group1);
    isp_af_iirpls_1_write(be_reg, af_reg_be_cfg->iir_pls_group1);

    isp_af_iir0_en0_write(be_reg, af_reg_be_cfg->iir0_enable0);
    isp_af_iir0_en1_write(be_reg, af_reg_be_cfg->iir0_enable1);
    isp_af_iir0_en2_write(be_reg, af_reg_be_cfg->iir0_enable2);
    isp_af_iir1_en0_write(be_reg, af_reg_be_cfg->iir1_enable0);
    isp_af_iir1_en1_write(be_reg, af_reg_be_cfg->iir1_enable1);
    isp_af_iir1_en2_write(be_reg, af_reg_be_cfg->iir1_enable2);
    isp_af_peak_mode_write(be_reg, af_reg_be_cfg->peak_mode);
    isp_af_squ_mode_write(be_reg, af_reg_be_cfg->squ_mode);
    isp_af_hnum_write(be_reg, af_reg_be_cfg->window_hnum);
    isp_af_vnum_write(be_reg, af_reg_be_cfg->window_vnum);

    isp_be_af_iir_gain_write(be_reg, af_reg_be_cfg);
    isp_be_af_iir_shift_write(be_reg, af_reg_be_cfg);
    isp_be_af_fir_gain_write(be_reg, af_reg_be_cfg);

    isp_be_af_crop_write(be_reg, af_reg_be_cfg);
    isp_be_af_raw_cfg_write(be_reg, af_reg_be_cfg);

    /* AF BE pre median filter */
    isp_af_mean_en_write(be_reg, af_reg_be_cfg->mean_enable);
    isp_af_mean_thres_write(be_reg, 0xFFFF - af_reg_be_cfg->mean_thres);
    isp_be_af_level_depend_gain_cfg_write(be_reg, af_reg_be_cfg); /* level depend gain */
    isp_be_af_coring_cfg_write(be_reg, af_reg_be_cfg);            /* AF BE coring */

    /* high luma counter */
    isp_af_highlight_write(be_reg, af_reg_be_cfg->highlight_thre);

    isp_be_af_output_shift_cfg_write(be_reg, af_reg_be_cfg);
}
#endif

static td_s32 isp_af_online_switch_config(ot_vi_pipe vi_pipe, isp_reg_cfg *reg_cfg_info, td_u8 i)
{
    isp_af_reg_cfg *af_reg_be_cfg = TD_NULL;
    isp_post_be_reg_type *be_reg = TD_NULL;
    isp_viproc_reg_type *viproc_reg = TD_NULL;

    af_reg_be_cfg = &reg_cfg_info->alg_reg_cfg[i].be_af_reg_cfg;
    if (af_reg_be_cfg->is_in_pre_be == TD_TRUE) {
        be_reg = (isp_be_reg_type *)isp_get_pre_be_vir_addr(vi_pipe, i);
        viproc_reg = (isp_viproc_reg_type *)isp_get_pre_vi_proc_vir_addr(vi_pipe, i);
    } else {
        be_reg = (isp_be_reg_type *)isp_get_post_be_vir_addr(vi_pipe, i);
        viproc_reg = (isp_viproc_reg_type *)isp_get_post_vi_proc_vir_addr(vi_pipe, i);
    }
    isp_check_pointer_return(be_reg);
    isp_check_pointer_return(viproc_reg);

    if (af_reg_be_cfg->af_pos_sel != af_reg_be_cfg->pre_af_pos_sel) {
        isp_af_en_write(viproc_reg, TD_FALSE);
    } else {
        isp_af_en_write(viproc_reg, af_reg_be_cfg->af_enable);
        isp_af_sel_write(be_reg, af_reg_be_cfg->af_pos_sel);
    }
    af_reg_be_cfg->pre_af_pos_sel = af_reg_be_cfg->af_pos_sel;
    return TD_SUCCESS;
}

static td_s32 isp_af_reg_config(ot_vi_pipe vi_pipe, isp_reg_cfg *reg_cfg_info, td_u8 i)
{
#ifdef CONFIG_OT_ISP_AF_SUPPORT
    td_bool offline_mode;
    td_bool usr_resh;
    td_bool idx_resh;
    td_u8 blk_num = reg_cfg_info->cfg_num;
    isp_af_reg_cfg *af_reg_be_cfg = TD_NULL;
    isp_post_be_reg_type *be_reg = TD_NULL;
    isp_viproc_reg_type *viproc_reg = TD_NULL;

    af_reg_be_cfg = &reg_cfg_info->alg_reg_cfg[i].be_af_reg_cfg;
    offline_mode = (af_reg_be_cfg->be_is_online == TD_TRUE) ? (TD_FALSE) : (TD_TRUE);

    if (af_reg_be_cfg->is_in_pre_be == TD_TRUE) {
        be_reg = (isp_be_reg_type *)isp_get_pre_be_vir_addr(vi_pipe, i);
        viproc_reg = (isp_viproc_reg_type *)isp_get_pre_vi_proc_vir_addr(vi_pipe, i);
    } else {
        be_reg = (isp_be_reg_type *)isp_get_post_be_vir_addr(vi_pipe, i);
        viproc_reg = (isp_viproc_reg_type *)isp_get_post_vi_proc_vir_addr(vi_pipe, i);
    }

    isp_check_pointer_return(be_reg);
    isp_check_pointer_return(viproc_reg);

    if (offline_mode == TD_TRUE) {
        isp_af_en_write(viproc_reg, af_reg_be_cfg->af_enable);
        isp_af_sel_write(be_reg, af_reg_be_cfg->af_pos_sel);
    } else {
        isp_af_online_switch_config(vi_pipe, reg_cfg_info, i);
    }

    idx_resh = (isp_af_update_index_read(be_reg) != af_reg_be_cfg->update_index);
    usr_resh =
        (offline_mode) ? (reg_cfg_info->cfg_key.bit1_af_be_cfg && idx_resh) : (reg_cfg_info->cfg_key.bit1_af_be_cfg);

    if (usr_resh) {
        isp_af_update_index_write(be_reg, af_reg_be_cfg->update_index);

        isp_be_af_reg_write(be_reg, af_reg_be_cfg);
        reg_cfg_info->cfg_key.bit1_af_be_cfg = offline_mode ? 1 : ((blk_num <= (i + 1)) ? 0 : 1);
    }
#endif

    return TD_SUCCESS;
}

static td_s32 isp_fe_awb_reg_config(ot_vi_pipe vi_pipe, isp_reg_cfg *reg_cfg_info)
{
    td_u32 i;
    ot_vi_pipe vi_pipe_bind;
    isp_awb_reg_dyn_cfg *awb_reg_dyn_cfg = TD_NULL;
    isp_awb_reg_sta_cfg *awb_reg_sta_cfg = TD_NULL;
    isp_fe_reg_type *fe_reg = TD_NULL;
    isp_usr_ctx *isp_ctx = TD_NULL;

    isp_get_ctx(vi_pipe, isp_ctx);

    if (isp_ctx->wdr_attr.is_mast_pipe != TD_TRUE) {
        return TD_SUCCESS;
    }

    for (i = 0; i < isp_ctx->wdr_attr.pipe_num; i++) {
        vi_pipe_bind = isp_ctx->wdr_attr.pipe_id[i];
        isp_check_no_fe_pipe_return(vi_pipe_bind);
        isp_dist_trans_pipe(&vi_pipe_bind);

        fe_reg = (isp_fe_reg_type *)isp_get_fe_vir_addr(vi_pipe_bind);
        isp_check_pointer_return(fe_reg);
        if (reg_cfg_info->cfg_key.bit1_awb_dyn_cfg) {
            awb_reg_dyn_cfg = &reg_cfg_info->alg_reg_cfg[0].awb_reg_cfg.awb_reg_dyn_cfg;
            isp_fe_wb_gain_write(fe_reg, awb_reg_dyn_cfg->fe_white_balance_gain);
            isp_fe_wb_en_write(fe_reg, awb_reg_dyn_cfg->fe_wb_work_en);
        }

        awb_reg_sta_cfg = &reg_cfg_info->alg_reg_cfg[0].awb_reg_cfg.awb_reg_sta_cfg;

        if (awb_reg_sta_cfg->fe_awb_sta_cfg) {
            awb_reg_sta_cfg = &reg_cfg_info->alg_reg_cfg[0].awb_reg_cfg.awb_reg_sta_cfg;
            isp_fe_wb1_clip_value_write(fe_reg, awb_reg_sta_cfg->fe_clip_value);
        }
    }

    return TD_SUCCESS;
}

static td_void isp_awb_cc_set0(isp_post_be_reg_type *be_reg,
    td_u16 be_cc[OT_ISP_COLOR_SECTOR_NUM][OT_ISP_CCM_MATRIX_SIZE])
{
    td_u8 i;
    for (i = 0; i < OT_ISP_CCM_MATRIX_SIZE; i++) {
        if ((be_cc[0][i] >> 12) < 0x8) {          /* valid bit 12, sign bit 0x8 */
            be_cc[0][i] = MIN2(be_cc[0][i], 0xFFF);  /* max positive value 0xFFF */
        } else {
            be_cc[0][i] = MIN2(be_cc[0][i], 0x8FFF); /* max negative value 0x8FFF */
        }
    }
    isp_cc_coef000_write(be_reg, ccm_convert(ccm_convert_pre(be_cc[0][0]))); /* array index 0 */
    isp_cc_coef001_write(be_reg, ccm_convert(ccm_convert_pre(be_cc[0][1]))); /* array index 1 */
    isp_cc_coef002_write(be_reg, ccm_convert(ccm_convert_pre(be_cc[0][2]))); /* array index 2 */
    isp_cc_coef010_write(be_reg, ccm_convert(ccm_convert_pre(be_cc[0][3]))); /* array index 3 */
    isp_cc_coef011_write(be_reg, ccm_convert(ccm_convert_pre(be_cc[0][4]))); /* array index 4 */
    isp_cc_coef012_write(be_reg, ccm_convert(ccm_convert_pre(be_cc[0][5]))); /* array index 5 */
    isp_cc_coef020_write(be_reg, ccm_convert(ccm_convert_pre(be_cc[0][6]))); /* array index 6 */
    isp_cc_coef021_write(be_reg, ccm_convert(ccm_convert_pre(be_cc[0][7]))); /* array index 7 */
    isp_cc_coef022_write(be_reg, ccm_convert(ccm_convert_pre(be_cc[0][8]))); /* array index 8 */
    isp_cc_coef100_write(be_reg, ccm_convert(ccm_convert_pre(be_cc[1][0]))); /* array index 0 */
    isp_cc_coef101_write(be_reg, ccm_convert(ccm_convert_pre(be_cc[1][1]))); /* array index 1 */
    isp_cc_coef102_write(be_reg, ccm_convert(ccm_convert_pre(be_cc[1][2]))); /* array index 2 */
    isp_cc_coef110_write(be_reg, ccm_convert(ccm_convert_pre(be_cc[1][3]))); /* array index 3 */
    isp_cc_coef111_write(be_reg, ccm_convert(ccm_convert_pre(be_cc[1][4]))); /* array index 4 */
    isp_cc_coef112_write(be_reg, ccm_convert(ccm_convert_pre(be_cc[1][5]))); /* array index 5 */
    isp_cc_coef120_write(be_reg, ccm_convert(ccm_convert_pre(be_cc[1][6]))); /* array index 6 */
    isp_cc_coef121_write(be_reg, ccm_convert(ccm_convert_pre(be_cc[1][7]))); /* array index 7 */
    isp_cc_coef122_write(be_reg, ccm_convert(ccm_convert_pre(be_cc[1][8]))); /* array index 8 */
    isp_cc_coef200_write(be_reg, ccm_convert(ccm_convert_pre(be_cc[2][0]))); /* array index 2, 0 */
    isp_cc_coef201_write(be_reg, ccm_convert(ccm_convert_pre(be_cc[2][1]))); /* array index 2, 1 */
    isp_cc_coef202_write(be_reg, ccm_convert(ccm_convert_pre(be_cc[2][2]))); /* array index 2, 2 */
    isp_cc_coef210_write(be_reg, ccm_convert(ccm_convert_pre(be_cc[2][3]))); /* array index 2, 3 */
    isp_cc_coef211_write(be_reg, ccm_convert(ccm_convert_pre(be_cc[2][4]))); /* array index 2, 4 */
    isp_cc_coef212_write(be_reg, ccm_convert(ccm_convert_pre(be_cc[2][5]))); /* array index 2, 5 */
    isp_cc_coef220_write(be_reg, ccm_convert(ccm_convert_pre(be_cc[2][6]))); /* array index 2, 6 */
    isp_cc_coef221_write(be_reg, ccm_convert(ccm_convert_pre(be_cc[2][7]))); /* array index 2, 7 */
    isp_cc_coef222_write(be_reg, ccm_convert(ccm_convert_pre(be_cc[2][8]))); /* array index 2, 8 */
    isp_cc_coef300_write(be_reg, ccm_convert(ccm_convert_pre(be_cc[3][0]))); /* array index 3, 0 */
    isp_cc_coef301_write(be_reg, ccm_convert(ccm_convert_pre(be_cc[3][1]))); /* array index 3, 1 */
    isp_cc_coef302_write(be_reg, ccm_convert(ccm_convert_pre(be_cc[3][2]))); /* array index 3, 2 */
    isp_cc_coef310_write(be_reg, ccm_convert(ccm_convert_pre(be_cc[3][3]))); /* array index 3, 3 */
    isp_cc_coef311_write(be_reg, ccm_convert(ccm_convert_pre(be_cc[3][4]))); /* array index 3, 4 */
    isp_cc_coef312_write(be_reg, ccm_convert(ccm_convert_pre(be_cc[3][5]))); /* array index 3, 5 */
    isp_cc_coef320_write(be_reg, ccm_convert(ccm_convert_pre(be_cc[3][6]))); /* array index 3, 6 */
    isp_cc_coef321_write(be_reg, ccm_convert(ccm_convert_pre(be_cc[3][7]))); /* array index 3, 7 */
    isp_cc_coef322_write(be_reg, ccm_convert(ccm_convert_pre(be_cc[3][8]))); /* array index 3, 8 */
}

static td_void isp_awb_cc_set1(isp_post_be_reg_type *be_reg,
    td_u16 be_cc[OT_ISP_COLOR_SECTOR_NUM][OT_ISP_CCM_MATRIX_SIZE])
{
    isp_cc_coef400_write(be_reg, ccm_convert(ccm_convert_pre(be_cc[4][0]))); /* array index 4, 0 */
    isp_cc_coef401_write(be_reg, ccm_convert(ccm_convert_pre(be_cc[4][1]))); /* array index 4, 1 */
    isp_cc_coef402_write(be_reg, ccm_convert(ccm_convert_pre(be_cc[4][2]))); /* array index 4, 2 */
    isp_cc_coef410_write(be_reg, ccm_convert(ccm_convert_pre(be_cc[4][3]))); /* array index 4, 3 */
    isp_cc_coef411_write(be_reg, ccm_convert(ccm_convert_pre(be_cc[4][4]))); /* array index 4, 4 */
    isp_cc_coef412_write(be_reg, ccm_convert(ccm_convert_pre(be_cc[4][5]))); /* array index 4, 5 */
    isp_cc_coef420_write(be_reg, ccm_convert(ccm_convert_pre(be_cc[4][6]))); /* array index 4, 6 */
    isp_cc_coef421_write(be_reg, ccm_convert(ccm_convert_pre(be_cc[4][7]))); /* array index 4, 7 */
    isp_cc_coef422_write(be_reg, ccm_convert(ccm_convert_pre(be_cc[4][8]))); /* array index 4, 8 */
    isp_cc_coef500_write(be_reg, ccm_convert(ccm_convert_pre(be_cc[5][0]))); /* array index 5, 0 */
    isp_cc_coef501_write(be_reg, ccm_convert(ccm_convert_pre(be_cc[5][1]))); /* array index 5, 1 */
    isp_cc_coef502_write(be_reg, ccm_convert(ccm_convert_pre(be_cc[5][2]))); /* array index 5, 2 */
    isp_cc_coef510_write(be_reg, ccm_convert(ccm_convert_pre(be_cc[5][3]))); /* array index 5, 3 */
    isp_cc_coef511_write(be_reg, ccm_convert(ccm_convert_pre(be_cc[5][4]))); /* array index 5, 4 */
    isp_cc_coef512_write(be_reg, ccm_convert(ccm_convert_pre(be_cc[5][5]))); /* array index 5, 5 */
    isp_cc_coef520_write(be_reg, ccm_convert(ccm_convert_pre(be_cc[5][6]))); /* array index 5, 6 */
    isp_cc_coef521_write(be_reg, ccm_convert(ccm_convert_pre(be_cc[5][7]))); /* array index 5, 7 */
    isp_cc_coef522_write(be_reg, ccm_convert(ccm_convert_pre(be_cc[5][8]))); /* array index 5, 8 */
}


static td_void isp_awb_gain_set(isp_post_be_reg_type *be_reg, td_u32 *be_wb_gain)
{
    isp_wb_rgain_write(be_reg, be_wb_gain[0]);  /* array index 0 */
    isp_wb_grgain_write(be_reg, be_wb_gain[1]); /* array index 1 */
    isp_wb_gbgain_write(be_reg, be_wb_gain[2]); /* array index 2 */
    isp_wb_bgain_write(be_reg, be_wb_gain[3]);  /* array index 3 */
}

static td_void isp_awb_dyn_reg_config(isp_post_be_reg_type *be_reg, isp_awb_reg_dyn_cfg *awb_reg_dyn_cfg)
{
    isp_awb_threshold_max_write(be_reg, awb_reg_dyn_cfg->be_metering_white_level_awb);
    isp_awb_threshold_min_write(be_reg, awb_reg_dyn_cfg->be_metering_black_level_awb);
    isp_awb_cr_ref_max_write(be_reg, awb_reg_dyn_cfg->be_metering_cr_ref_max_awb);
    isp_awb_cr_ref_min_write(be_reg, awb_reg_dyn_cfg->be_metering_cr_ref_min_awb);
    isp_awb_cb_ref_max_write(be_reg, awb_reg_dyn_cfg->be_metering_cb_ref_max_awb);
    isp_awb_cb_ref_min_write(be_reg, awb_reg_dyn_cfg->be_metering_cb_ref_min_awb);

    isp_cc_r_gain_write(be_reg, awb_reg_dyn_cfg->be_cc_r_gain);
    isp_cc_g_gain_write(be_reg, awb_reg_dyn_cfg->be_cc_g_gain);
    isp_cc_b_gain_write(be_reg, awb_reg_dyn_cfg->be_cc_b_gain);

    isp_awb_crop_pos_x_write(be_reg, awb_reg_dyn_cfg->be_crop_pos_x);
    isp_awb_crop_pos_y_write(be_reg, awb_reg_dyn_cfg->be_crop_pos_y);
    isp_awb_crop_out_width_write(be_reg, awb_reg_dyn_cfg->be_crop_out_width - 1);
    isp_awb_crop_out_height_write(be_reg, awb_reg_dyn_cfg->be_crop_out_height - 1);
}

static td_void isp_awb_sta_reg_config(isp_post_be_reg_type *be_reg, isp_awb_reg_sta_cfg *awb_reg_sta_cfg)
{
    isp_awb_bitmove_write(be_reg, awb_reg_sta_cfg->be_awb_bitmove);
    isp_awb_stat_raddr_write(be_reg, awb_reg_sta_cfg->be_awb_stat_raddr);
    isp_cc_recover_en_write(be_reg, 0);

    isp_cc_in_dc0_write(be_reg, awb_reg_sta_cfg->be_cc_in_dc0);
    isp_cc_in_dc1_write(be_reg, awb_reg_sta_cfg->be_cc_in_dc1);
    isp_cc_in_dc2_write(be_reg, awb_reg_sta_cfg->be_cc_in_dc2);
    isp_cc_out_dc0_write(be_reg, awb_reg_sta_cfg->be_cc_out_dc0);
    isp_cc_out_dc1_write(be_reg, awb_reg_sta_cfg->be_cc_out_dc1);
    isp_cc_out_dc2_write(be_reg, awb_reg_sta_cfg->be_cc_out_dc2);
    isp_wb_clip_value_write(be_reg, awb_reg_sta_cfg->be_wb_clip_value);
    isp_awb_offset_comp_write(be_reg, awb_reg_sta_cfg->be_awb_offset_comp);

    isp_cc_hue0_write(be_reg, awb_reg_sta_cfg->be_cc_sector[0]);
    isp_cc_hue1_write(be_reg, awb_reg_sta_cfg->be_cc_sector[1]);
    isp_cc_hue2_write(be_reg, awb_reg_sta_cfg->be_cc_sector[2]); /* 2 index */
    isp_cc_hue3_write(be_reg, awb_reg_sta_cfg->be_cc_sector[3]); /* 3 index */
    isp_cc_hue4_write(be_reg, awb_reg_sta_cfg->be_cc_sector[4]); /* 4 index */
    isp_cc_hue5_write(be_reg, awb_reg_sta_cfg->be_cc_sector[5]); /* 5 index */
    isp_cc_matrix_gray_en_write(be_reg, awb_reg_sta_cfg->be_cc_gray_en);
}

static td_void isp_awb_usr_reg_config(isp_post_be_reg_type *be_reg, isp_awb_reg_usr_cfg *awb_reg_usr_cfg)
{
    isp_awb_update_index_write(be_reg, awb_reg_usr_cfg->update_index);
    isp_awb_hnum_write(be_reg, awb_reg_usr_cfg->be_zone_col);
    isp_awb_vnum_write(be_reg, awb_reg_usr_cfg->be_zone_row);
}

static td_void isp_awb_cc_and_gain_reg_config(ot_vi_pipe vi_pipe, isp_awb_reg_dyn_cfg *awb_reg_dyn_cfg,
    isp_post_be_reg_type *be_reg)
{
    isp_usr_ctx *isp_ctx = TD_NULL;
    isp_get_ctx(vi_pipe, isp_ctx);

    /* offline mode, cfg ccm/awb at sync param in vi module. */
    if (isp_ctx->stitch_attr.stitch_enable != TD_TRUE) {
        isp_cc_hue_en_write(be_reg, awb_reg_dyn_cfg->be_cc_sector_en);
        isp_awb_cc_set0(be_reg, awb_reg_dyn_cfg->be_color_matrix);
        isp_awb_cc_set1(be_reg, awb_reg_dyn_cfg->be_color_matrix);
        isp_awb_gain_set(be_reg, awb_reg_dyn_cfg->be_white_balance_gain);
    }
}

static td_void isp_awb_reg_config_write(ot_vi_pipe vi_pipe, isp_be_reg_type *be_reg, isp_viproc_reg_type *viproc_reg,
    isp_awb_reg_cfg *awb_reg_cfg)
{
    td_bool offline_mode;
    td_bool idx_resh, usr_resh;
    isp_awb_reg_sta_cfg *awb_reg_sta_cfg = &awb_reg_cfg->awb_reg_sta_cfg;
    isp_awb_reg_dyn_cfg *awb_reg_dyn_cfg = &awb_reg_cfg->awb_reg_dyn_cfg;
    isp_awb_reg_usr_cfg *awb_reg_usr_cfg = &awb_reg_cfg->awb_reg_usr_cfg;

    isp_awb_cc_and_gain_reg_config(vi_pipe, awb_reg_dyn_cfg, be_reg);

    isp_awb_dyn_reg_config(be_reg, awb_reg_dyn_cfg);

    if (awb_reg_sta_cfg->be_awb_sta_cfg) {
        isp_awb_sta_reg_config(be_reg, awb_reg_sta_cfg);
        awb_reg_sta_cfg->be_awb_sta_cfg = 0;
    }

    offline_mode = (awb_reg_dyn_cfg->is_online == TD_TRUE) ? (TD_FALSE) : (TD_TRUE);
    idx_resh = (isp_awb_update_index_read(be_reg) != awb_reg_usr_cfg->update_index);
    usr_resh = (offline_mode) ? (awb_reg_usr_cfg->resh && idx_resh) : (awb_reg_usr_cfg->resh);

    if (usr_resh) {
        isp_awb_usr_reg_config(be_reg, awb_reg_usr_cfg);
        /* if online mode, resh=0; if offline mode, resh=1; but only index != will resh */
        awb_reg_usr_cfg->resh = offline_mode;
    }

    awb_reg_usr_cfg->valid_awb_switch = awb_reg_usr_cfg->be_awb_switch;
    if (awb_reg_dyn_cfg->is_online == TD_TRUE) {
        if (awb_reg_usr_cfg->be_awb_switch != awb_reg_usr_cfg->pre_awb_switch &&
            awb_reg_usr_cfg->switch_started == TD_FALSE) {
            isp_awb_en_write(viproc_reg, TD_FALSE);
            awb_reg_usr_cfg->valid_awb_switch = awb_reg_usr_cfg->pre_awb_switch;
            awb_reg_usr_cfg->pre_awb_switch = awb_reg_usr_cfg->be_awb_switch;
            awb_reg_usr_cfg->switch_started = TD_TRUE;
        } else {
            isp_awb_en_write(viproc_reg, awb_reg_sta_cfg->be_awb_work_en);
            isp_awb_sel_write(be_reg, awb_reg_usr_cfg->be_awb_switch);
            awb_reg_usr_cfg->valid_awb_switch = awb_reg_usr_cfg->be_awb_switch;
            awb_reg_usr_cfg->switch_started = TD_FALSE;
            awb_reg_usr_cfg->pre_awb_switch = awb_reg_usr_cfg->valid_awb_switch;
        }
    } else {
        isp_awb_en_write(viproc_reg, awb_reg_sta_cfg->be_awb_work_en);
        isp_awb_sel_write(be_reg, awb_reg_usr_cfg->be_awb_switch);
    }
}

static td_s32 isp_awb_reg_config(ot_vi_pipe vi_pipe, isp_reg_cfg *reg_cfg_info, td_u8 i)
{
    isp_awb_reg_dyn_cfg *awb_reg_dyn_cfg = TD_NULL;
    isp_viproc_reg_type *viproc_reg = TD_NULL;
    isp_be_reg_type *be_reg = TD_NULL;

    awb_reg_dyn_cfg = &reg_cfg_info->alg_reg_cfg[i].awb_reg_cfg.awb_reg_dyn_cfg;

    if (awb_reg_dyn_cfg->is_in_pre_be == TD_TRUE) {
        be_reg = (isp_be_reg_type *)isp_get_pre_be_vir_addr(vi_pipe, i);
        viproc_reg = (isp_viproc_reg_type *)isp_get_pre_vi_proc_vir_addr(vi_pipe, i);
    } else {
        be_reg = (isp_be_reg_type *)isp_get_post_be_vir_addr(vi_pipe, i);
        viproc_reg = (isp_viproc_reg_type *)isp_get_post_vi_proc_vir_addr(vi_pipe, i);
    }
    isp_check_pointer_return(be_reg);
    isp_check_pointer_return(viproc_reg);

    isp_awb_reg_config_write(vi_pipe, be_reg, viproc_reg, &reg_cfg_info->alg_reg_cfg[i].awb_reg_cfg);

    return TD_SUCCESS;
}

static td_void isp_sharpen_lumawgt_write(isp_post_be_reg_type *be_reg, td_u8 *luma_wgt)
{
    isp_sharpen_lumawgt0_write(be_reg, luma_wgt[0]);   /* array index 0 */
    isp_sharpen_lumawgt1_write(be_reg, luma_wgt[1]);   /* array index 1 */
    isp_sharpen_lumawgt2_write(be_reg, luma_wgt[2]);   /* array index 2 */
    isp_sharpen_lumawgt3_write(be_reg, luma_wgt[3]);   /* array index 3 */
    isp_sharpen_lumawgt4_write(be_reg, luma_wgt[4]);   /* array index 4 */
    isp_sharpen_lumawgt5_write(be_reg, luma_wgt[5]);   /* array index 5 */
    isp_sharpen_lumawgt6_write(be_reg, luma_wgt[6]);   /* array index 6 */
    isp_sharpen_lumawgt7_write(be_reg, luma_wgt[7]);   /* array index 7 */
    isp_sharpen_lumawgt8_write(be_reg, luma_wgt[8]);   /* array index 8 */
    isp_sharpen_lumawgt9_write(be_reg, luma_wgt[9]);   /* array index 9 */
    isp_sharpen_lumawgt10_write(be_reg, luma_wgt[10]); /* array index 10 */
    isp_sharpen_lumawgt11_write(be_reg, luma_wgt[11]); /* array index 11 */
    isp_sharpen_lumawgt12_write(be_reg, luma_wgt[12]); /* array index 12 */
    isp_sharpen_lumawgt13_write(be_reg, luma_wgt[13]); /* array index 13 */
    isp_sharpen_lumawgt14_write(be_reg, luma_wgt[14]); /* array index 14 */
    isp_sharpen_lumawgt15_write(be_reg, luma_wgt[15]); /* array index 15 */
    isp_sharpen_lumawgt16_write(be_reg, luma_wgt[16]); /* array index 16 */
    isp_sharpen_lumawgt17_write(be_reg, luma_wgt[17]); /* array index 17 */
    isp_sharpen_lumawgt18_write(be_reg, luma_wgt[18]); /* array index 18 */
    isp_sharpen_lumawgt19_write(be_reg, luma_wgt[19]); /* array index 19 */
    isp_sharpen_lumawgt20_write(be_reg, luma_wgt[20]); /* array index 20 */
    isp_sharpen_lumawgt21_write(be_reg, luma_wgt[21]); /* array index 21 */
    isp_sharpen_lumawgt22_write(be_reg, luma_wgt[22]); /* array index 22 */
    isp_sharpen_lumawgt23_write(be_reg, luma_wgt[23]); /* array index 23 */
    isp_sharpen_lumawgt24_write(be_reg, luma_wgt[24]); /* array index 24 */
    isp_sharpen_lumawgt25_write(be_reg, luma_wgt[25]); /* array index 25 */
    isp_sharpen_lumawgt26_write(be_reg, luma_wgt[26]); /* array index 26 */
    isp_sharpen_lumawgt27_write(be_reg, luma_wgt[27]); /* array index 27 */
    isp_sharpen_lumawgt28_write(be_reg, luma_wgt[28]); /* array index 28 */
    isp_sharpen_lumawgt29_write(be_reg, luma_wgt[29]); /* array index 29 */
    isp_sharpen_lumawgt30_write(be_reg, luma_wgt[30]); /* array index 30 */
    isp_sharpen_lumawgt31_write(be_reg, luma_wgt[31]); /* array index 31 */
}

static td_void isp_sharpen_rlywgt_write(isp_post_be_reg_type *be_reg, td_u8 *rly_wgt)
{
    isp_sharpen_rly_wgt0_write(be_reg, rly_wgt[0]);
    isp_sharpen_rly_wgt1_write(be_reg, rly_wgt[1]);
    isp_sharpen_rly_wgt2_write(be_reg, rly_wgt[2]);       /* idx 2 */
    isp_sharpen_rly_wgt3_write(be_reg, rly_wgt[3]);       /* idx 3 */
    isp_sharpen_rly_wgt4_write(be_reg, rly_wgt[4]);       /* idx 4 */
    isp_sharpen_rly_wgt5_write(be_reg, rly_wgt[5]);       /* idx 5 */
    isp_sharpen_rly_wgt6_write(be_reg, rly_wgt[6]);       /* idx 6 */
    isp_sharpen_rly_wgt7_write(be_reg, rly_wgt[7]);       /* idx 7 */
    isp_sharpen_rly_wgt8_write(be_reg, rly_wgt[8]);       /* idx 8 */
    isp_sharpen_rly_wgt9_write(be_reg, rly_wgt[9]);       /* idx 9 */
    isp_sharpen_rly_wgt10_write(be_reg, rly_wgt[10]);     /* idx 10 */
    isp_sharpen_rly_wgt11_write(be_reg, rly_wgt[11]);     /* idx 11 */
    isp_sharpen_rly_wgt12_write(be_reg, rly_wgt[12]);     /* idx 12 */
    isp_sharpen_rly_wgt13_write(be_reg, rly_wgt[13]);     /* idx 13 */
    isp_sharpen_rly_wgt14_write(be_reg, rly_wgt[14]);     /* idx 14 */
    isp_sharpen_rly_wgt15_write(be_reg, rly_wgt[15]);     /* idx 15 */
}

static td_void isp_sharpen_stdgainbymotlut_write(isp_post_be_reg_type *be_reg, td_u8 *std_gain_by_mot_lut)
{
    isp_sharpen_std_gain_bymot0_write(be_reg, std_gain_by_mot_lut[0]);
    isp_sharpen_std_gain_bymot1_write(be_reg, std_gain_by_mot_lut[1]);
    isp_sharpen_std_gain_bymot2_write(be_reg, std_gain_by_mot_lut[2]);      /* idx 2 */
    isp_sharpen_std_gain_bymot3_write(be_reg, std_gain_by_mot_lut[3]);      /* idx 3 */
    isp_sharpen_std_gain_bymot4_write(be_reg, std_gain_by_mot_lut[4]);      /* idx 4 */
    isp_sharpen_std_gain_bymot5_write(be_reg, std_gain_by_mot_lut[5]);      /* idx 5 */
    isp_sharpen_std_gain_bymot6_write(be_reg, std_gain_by_mot_lut[6]);      /* idx 6 */
    isp_sharpen_std_gain_bymot7_write(be_reg, std_gain_by_mot_lut[7]);      /* idx 7 */
    isp_sharpen_std_gain_bymot8_write(be_reg, std_gain_by_mot_lut[8]);      /* idx 8 */
    isp_sharpen_std_gain_bymot9_write(be_reg, std_gain_by_mot_lut[9]);      /* idx 9 */
    isp_sharpen_std_gain_bymot10_write(be_reg, std_gain_by_mot_lut[10]);    /* idx 10 */
    isp_sharpen_std_gain_bymot11_write(be_reg, std_gain_by_mot_lut[11]);    /* idx 11 */
    isp_sharpen_std_gain_bymot12_write(be_reg, std_gain_by_mot_lut[12]);    /* idx 12 */
    isp_sharpen_std_gain_bymot13_write(be_reg, std_gain_by_mot_lut[13]);    /* idx 13 */
    isp_sharpen_std_gain_bymot14_write(be_reg, std_gain_by_mot_lut[14]);    /* idx 14 */
    isp_sharpen_std_gain_bymot15_write(be_reg, std_gain_by_mot_lut[15]);    /* idx 15 */
}

static td_void isp_sharpen_stdoffsetbymotlut_write(isp_post_be_reg_type *be_reg, td_u8 *std_offset_by_mot_lut)
{
    isp_sharpen_std_offset_bymot0_write(be_reg, std_offset_by_mot_lut[0]);
    isp_sharpen_std_offset_bymot1_write(be_reg, std_offset_by_mot_lut[1]);
    isp_sharpen_std_offset_bymot2_write(be_reg, std_offset_by_mot_lut[2]);      /* idx 2 */
    isp_sharpen_std_offset_bymot3_write(be_reg, std_offset_by_mot_lut[3]);      /* idx 3 */
    isp_sharpen_std_offset_bymot4_write(be_reg, std_offset_by_mot_lut[4]);      /* idx 4 */
    isp_sharpen_std_offset_bymot5_write(be_reg, std_offset_by_mot_lut[5]);      /* idx 5 */
    isp_sharpen_std_offset_bymot6_write(be_reg, std_offset_by_mot_lut[6]);      /* idx 6 */
    isp_sharpen_std_offset_bymot7_write(be_reg, std_offset_by_mot_lut[7]);      /* idx 7 */
    isp_sharpen_std_offset_bymot8_write(be_reg, std_offset_by_mot_lut[8]);      /* idx 8 */
    isp_sharpen_std_offset_bymot9_write(be_reg, std_offset_by_mot_lut[9]);      /* idx 9 */
    isp_sharpen_std_offset_bymot10_write(be_reg, std_offset_by_mot_lut[10]);    /* idx 10 */
    isp_sharpen_std_offset_bymot11_write(be_reg, std_offset_by_mot_lut[11]);    /* idx 11 */
    isp_sharpen_std_offset_bymot12_write(be_reg, std_offset_by_mot_lut[12]);    /* idx 12 */
    isp_sharpen_std_offset_bymot13_write(be_reg, std_offset_by_mot_lut[13]);    /* idx 13 */
    isp_sharpen_std_offset_bymot14_write(be_reg, std_offset_by_mot_lut[14]);    /* idx 14 */
    isp_sharpen_std_offset_bymot15_write(be_reg, std_offset_by_mot_lut[15]);    /* idx 15 */
}

static td_void isp_sharpen_stdgainbyylut_write(isp_post_be_reg_type *be_reg, td_u8 *std_gain_by_y_lut)
{
    isp_sharpen_std_gain_byy0_write(be_reg, std_gain_by_y_lut[0]);
    isp_sharpen_std_gain_byy1_write(be_reg, std_gain_by_y_lut[1]);
    isp_sharpen_std_gain_byy2_write(be_reg, std_gain_by_y_lut[2]);      /* idx 2 */
    isp_sharpen_std_gain_byy3_write(be_reg, std_gain_by_y_lut[3]);      /* idx 3 */
    isp_sharpen_std_gain_byy4_write(be_reg, std_gain_by_y_lut[4]);      /* idx 4 */
    isp_sharpen_std_gain_byy5_write(be_reg, std_gain_by_y_lut[5]);      /* idx 5 */
    isp_sharpen_std_gain_byy6_write(be_reg, std_gain_by_y_lut[6]);      /* idx 6 */
    isp_sharpen_std_gain_byy7_write(be_reg, std_gain_by_y_lut[7]);      /* idx 7 */
    isp_sharpen_std_gain_byy8_write(be_reg, std_gain_by_y_lut[8]);      /* idx 8 */
    isp_sharpen_std_gain_byy9_write(be_reg, std_gain_by_y_lut[9]);      /* idx 9 */
    isp_sharpen_std_gain_byy10_write(be_reg, std_gain_by_y_lut[10]);    /* idx 10 */
    isp_sharpen_std_gain_byy11_write(be_reg, std_gain_by_y_lut[11]);    /* idx 11 */
    isp_sharpen_std_gain_byy12_write(be_reg, std_gain_by_y_lut[12]);    /* idx 12 */
    isp_sharpen_std_gain_byy13_write(be_reg, std_gain_by_y_lut[13]);    /* idx 13 */
    isp_sharpen_std_gain_byy14_write(be_reg, std_gain_by_y_lut[14]);    /* idx 14 */
    isp_sharpen_std_gain_byy15_write(be_reg, std_gain_by_y_lut[15]);    /* idx 15 */
}

static td_void isp_sharpen_stdoffsetbyylut_write(isp_post_be_reg_type *be_reg, td_u8 *std_offset_by_y_lut)
{
    isp_sharpen_std_offset_byy1_write(be_reg, std_offset_by_y_lut[1]);
    isp_sharpen_std_offset_byy2_write(be_reg, std_offset_by_y_lut[2]);   /* idx 2 */
    isp_sharpen_std_offset_byy3_write(be_reg, std_offset_by_y_lut[3]);   /* idx 3 */
    isp_sharpen_std_offset_byy4_write(be_reg, std_offset_by_y_lut[4]);   /* idx 4 */
    isp_sharpen_std_offset_byy5_write(be_reg, std_offset_by_y_lut[5]);   /* idx 5 */
    isp_sharpen_std_offset_byy6_write(be_reg, std_offset_by_y_lut[6]);   /* idx 6 */
    isp_sharpen_std_offset_byy7_write(be_reg, std_offset_by_y_lut[7]);   /* idx 7 */
    isp_sharpen_std_offset_byy8_write(be_reg, std_offset_by_y_lut[8]);   /* idx 8 */
    isp_sharpen_std_offset_byy9_write(be_reg, std_offset_by_y_lut[9]);   /* idx 9 */
    isp_sharpen_std_offset_byy10_write(be_reg, std_offset_by_y_lut[10]); /* idx 10 */
    isp_sharpen_std_offset_byy11_write(be_reg, std_offset_by_y_lut[11]); /* idx 11 */
    isp_sharpen_std_offset_byy12_write(be_reg, std_offset_by_y_lut[12]); /* idx 12 */
    isp_sharpen_std_offset_byy13_write(be_reg, std_offset_by_y_lut[13]); /* idx 13 */
    isp_sharpen_std_offset_byy14_write(be_reg, std_offset_by_y_lut[14]); /* idx 14 */
    isp_sharpen_std_offset_byy15_write(be_reg, std_offset_by_y_lut[15]); /* idx 15 */
}

static td_void isp_sharpen_mfmotdec_write(isp_post_be_reg_type *be_reg, td_u8 *mf_mot_dec)
{
    isp_sharpen_mf_mot_dec0_write(be_reg, mf_mot_dec[0]);
    isp_sharpen_mf_mot_dec1_write(be_reg, mf_mot_dec[1]);
    isp_sharpen_mf_mot_dec2_write(be_reg, mf_mot_dec[2]);    /* idx 2 */
    isp_sharpen_mf_mot_dec3_write(be_reg, mf_mot_dec[3]);    /* idx 3 */
    isp_sharpen_mf_mot_dec4_write(be_reg, mf_mot_dec[4]);    /* idx 4 */
    isp_sharpen_mf_mot_dec5_write(be_reg, mf_mot_dec[5]);    /* idx 5 */
    isp_sharpen_mf_mot_dec6_write(be_reg, mf_mot_dec[6]);    /* idx 6 */
    isp_sharpen_mf_mot_dec7_write(be_reg, mf_mot_dec[7]);    /* idx 7 */
    isp_sharpen_mf_mot_dec8_write(be_reg, mf_mot_dec[8]);    /* idx 8 */
    isp_sharpen_mf_mot_dec9_write(be_reg, mf_mot_dec[9]);    /* idx 9 */
    isp_sharpen_mf_mot_dec10_write(be_reg, mf_mot_dec[10]);  /* idx 10 */
    isp_sharpen_mf_mot_dec11_write(be_reg, mf_mot_dec[11]);  /* idx 11 */
    isp_sharpen_mf_mot_dec12_write(be_reg, mf_mot_dec[12]);  /* idx 12 */
    isp_sharpen_mf_mot_dec13_write(be_reg, mf_mot_dec[13]);  /* idx 13 */
    isp_sharpen_mf_mot_dec14_write(be_reg, mf_mot_dec[14]);  /* idx 14 */
    isp_sharpen_mf_mot_dec15_write(be_reg, mf_mot_dec[15]);  /* idx 15 */
}

static td_void isp_sharpen_hfmotdec_write(isp_post_be_reg_type *be_reg, td_u8 *hf_mot_dec)
{
    isp_sharpen_hf_mot_dec0_write(be_reg, hf_mot_dec[0]);
    isp_sharpen_hf_mot_dec1_write(be_reg, hf_mot_dec[1]);
    isp_sharpen_hf_mot_dec2_write(be_reg, hf_mot_dec[2]);   /* idx 2 */
    isp_sharpen_hf_mot_dec3_write(be_reg, hf_mot_dec[3]);   /* idx 3 */
    isp_sharpen_hf_mot_dec4_write(be_reg, hf_mot_dec[4]);   /* idx 4 */
    isp_sharpen_hf_mot_dec5_write(be_reg, hf_mot_dec[5]);   /* idx 5 */
    isp_sharpen_hf_mot_dec6_write(be_reg, hf_mot_dec[6]);   /* idx 6 */
    isp_sharpen_hf_mot_dec7_write(be_reg, hf_mot_dec[7]);   /* idx 7 */
    isp_sharpen_hf_mot_dec8_write(be_reg, hf_mot_dec[8]);   /* idx 8 */
    isp_sharpen_hf_mot_dec9_write(be_reg, hf_mot_dec[9]);   /* idx 9 */
    isp_sharpen_hf_mot_dec10_write(be_reg, hf_mot_dec[10]); /* idx 10 */
    isp_sharpen_hf_mot_dec11_write(be_reg, hf_mot_dec[11]); /* idx 11 */
    isp_sharpen_hf_mot_dec12_write(be_reg, hf_mot_dec[12]); /* idx 12 */
    isp_sharpen_hf_mot_dec13_write(be_reg, hf_mot_dec[13]); /* idx 13 */
    isp_sharpen_hf_mot_dec14_write(be_reg, hf_mot_dec[14]); /* idx 14 */
    isp_sharpen_hf_mot_dec15_write(be_reg, hf_mot_dec[15]); /* idx 15 */
}

static td_void isp_sharpen_lmfmotgain_write(isp_post_be_reg_type *be_reg, td_u8 *lmf_mot_gain)
{
    isp_sharpen_lmf_mot_gain0_write(be_reg, lmf_mot_gain[0]);
    isp_sharpen_lmf_mot_gain1_write(be_reg, lmf_mot_gain[1]);
    isp_sharpen_lmf_mot_gain2_write(be_reg, lmf_mot_gain[2]);   /* idx 2 */
    isp_sharpen_lmf_mot_gain3_write(be_reg, lmf_mot_gain[3]);   /* idx 3 */
    isp_sharpen_lmf_mot_gain4_write(be_reg, lmf_mot_gain[4]);   /* idx 4 */
    isp_sharpen_lmf_mot_gain5_write(be_reg, lmf_mot_gain[5]);   /* idx 5 */
    isp_sharpen_lmf_mot_gain6_write(be_reg, lmf_mot_gain[6]);   /* idx 6 */
    isp_sharpen_lmf_mot_gain7_write(be_reg, lmf_mot_gain[7]);   /* idx 7 */
    isp_sharpen_lmf_mot_gain8_write(be_reg, lmf_mot_gain[8]);   /* idx 8 */
    isp_sharpen_lmf_mot_gain9_write(be_reg, lmf_mot_gain[9]);   /* idx 9 */
    isp_sharpen_lmf_mot_gain10_write(be_reg, lmf_mot_gain[10]); /* idx 10 */
    isp_sharpen_lmf_mot_gain11_write(be_reg, lmf_mot_gain[11]); /* idx 11 */
    isp_sharpen_lmf_mot_gain12_write(be_reg, lmf_mot_gain[12]); /* idx 12 */
    isp_sharpen_lmf_mot_gain13_write(be_reg, lmf_mot_gain[13]); /* idx 13 */
    isp_sharpen_lmf_mot_gain14_write(be_reg, lmf_mot_gain[14]); /* idx 14 */
    isp_sharpen_lmf_mot_gain15_write(be_reg, lmf_mot_gain[15]); /* idx 15 */
}

static td_void isp_sharpen_mpi_dyna_reg_config(isp_post_be_reg_type *be_reg,
    isp_sharpen_mpi_dyna_reg_cfg *mpi_dyna_reg_cfg)
{
    isp_sharpen_detl_oshtmul_write(be_reg, mpi_dyna_reg_cfg->detail_osht_mul);
    isp_sharpen_detl_ushtmul_write(be_reg, mpi_dyna_reg_cfg->detail_usht_mul);
    isp_sharpen_omaxgain_write(be_reg, mpi_dyna_reg_cfg->o_max_gain);
    isp_sharpen_umaxgain_write(be_reg, mpi_dyna_reg_cfg->u_max_gain);
    isp_sharpen_skinmaxu_write(be_reg, mpi_dyna_reg_cfg->skin_max_u);
    isp_sharpen_skinminu_write(be_reg, mpi_dyna_reg_cfg->skin_min_u);
    isp_sharpen_skinmaxv_write(be_reg, mpi_dyna_reg_cfg->skin_max_v);
    isp_sharpen_skinminv_write(be_reg, mpi_dyna_reg_cfg->skin_min_v);
    isp_sharpen_chrgmfmul_write(be_reg, mpi_dyna_reg_cfg->chr_gmf_mul);
    isp_sharpen_chrgmul_write(be_reg, mpi_dyna_reg_cfg->chr_g_mul);
    isp_sharpen_chrggain_write(be_reg, mpi_dyna_reg_cfg->chr_g_gain);
    isp_sharpen_chrgmfgain_write(be_reg, mpi_dyna_reg_cfg->chr_gmf_gain);

    isp_sharpen_chrbgain_write(be_reg, mpi_dyna_reg_cfg->chr_b_gain);
    isp_sharpen_chrbmul_write(be_reg, mpi_dyna_reg_cfg->chr_b_mul);
    isp_sharpen_chrrgain_write(be_reg, mpi_dyna_reg_cfg->chr_r_gain);
    isp_sharpen_chrrmul_write(be_reg, mpi_dyna_reg_cfg->chr_r_mul);
    isp_sharpen_benchrctrl_write(be_reg, mpi_dyna_reg_cfg->en_chr_ctrl);
    isp_sharpen_bendetailctrl_write(be_reg, mpi_dyna_reg_cfg->en_detail_ctrl);
    isp_sharpen_benlumactrl_write(be_reg, mpi_dyna_reg_cfg->en_luma_ctrl);
    isp_sharpen_benshtctrlbyvar_write(be_reg, mpi_dyna_reg_cfg->en_sht_ctrl_by_var);
    isp_sharpen_benskinctrl_write(be_reg, mpi_dyna_reg_cfg->en_skin_ctrl);
    isp_sharpen_dirdiffsft_write(be_reg, mpi_dyna_reg_cfg->dir_diff_sft);
    isp_sharpen_dirrt0_write(be_reg, mpi_dyna_reg_cfg->dir_rt[0]); /* array index 0 */
    isp_sharpen_dirrt1_write(be_reg, mpi_dyna_reg_cfg->dir_rt[1]); /* array index 1 */
    isp_sharpen_lumawgt_write(be_reg, mpi_dyna_reg_cfg->luma_wgt);

    isp_sharpen_osht_dtl_thd0_write(be_reg, mpi_dyna_reg_cfg->detail_osht_thr[0]); /* array index 0 */
    isp_sharpen_osht_dtl_thd1_write(be_reg, mpi_dyna_reg_cfg->detail_osht_thr[1]); /* array index 1 */
    isp_sharpen_osht_dtl_wgt_write(be_reg, mpi_dyna_reg_cfg->detail_osht_amt);
    isp_sharpen_shtvarthd1_write(be_reg, mpi_dyna_reg_cfg->sht_var_thd1);
    isp_sharpen_oshtamt_write(be_reg, mpi_dyna_reg_cfg->osht_amt);
    isp_sharpen_ushtamt_write(be_reg, mpi_dyna_reg_cfg->usht_amt);
    isp_sharpen_shtbldrt_write(be_reg, mpi_dyna_reg_cfg->sht_bld_rt);
    isp_sharpen_shtvarmul_write(be_reg, mpi_dyna_reg_cfg->sht_var_mul);
    isp_sharpen_skinedgemul_write(be_reg, mpi_dyna_reg_cfg->skin_edge_mul);
    isp_sharpen_skinedgewgt0_write(be_reg, mpi_dyna_reg_cfg->skin_edge_wgt[0]);
    isp_sharpen_skinedgewgt1_write(be_reg, mpi_dyna_reg_cfg->skin_edge_wgt[1]);
    isp_sharpen_usht_dtl_thd0_write(be_reg, mpi_dyna_reg_cfg->detail_usht_thr[0]);
    isp_sharpen_usht_dtl_thd1_write(be_reg, mpi_dyna_reg_cfg->detail_usht_thr[1]);
    isp_sharpen_usht_dtl_wgt_write(be_reg, mpi_dyna_reg_cfg->detail_usht_amt);

    isp_sharpen_exluma_thd_write(be_reg, mpi_dyna_reg_cfg->exluma_thd);
    isp_sharpen_exluma_out_thd_write(be_reg, mpi_dyna_reg_cfg->exluma_out_thd);
    isp_sharpen_exluma_mul_write(be_reg, mpi_dyna_reg_cfg->exluma_mul);
    isp_sharpen_hard_luma_thd_write(be_reg, mpi_dyna_reg_cfg->hard_luma_thd);

    isp_sharpen_mot_ctrl_en_write(be_reg, mpi_dyna_reg_cfg->en_mot_ctrl);
    isp_sharpen_std_adj_bymot_en_write(be_reg, mpi_dyna_reg_cfg->en_std_adj_by_mot);
    isp_sharpen_std_adj_byy_en_write(be_reg, mpi_dyna_reg_cfg->en_std_adj_by_y);
    isp_sharpen_var5_low_thd_write(be_reg, mpi_dyna_reg_cfg->var5_low_thd);
    isp_sharpen_var5_mid_thd_write(be_reg, mpi_dyna_reg_cfg->var5_mid_thd);
    isp_sharpen_var5_high_thd_write(be_reg, mpi_dyna_reg_cfg->var5_high_thd);
    isp_sharpen_var7_low_thd_write(be_reg, mpi_dyna_reg_cfg->var7x9_low_thd);
    isp_sharpen_var7_high_thd_write(be_reg, mpi_dyna_reg_cfg->var7x9_high_thd);
    isp_sharpen_edge_osht_amt_write(be_reg, mpi_dyna_reg_cfg->edge_osht_amt);
    isp_sharpen_edge_usht_amt_write(be_reg, mpi_dyna_reg_cfg->edge_usht_amt);
    isp_sharpen_rlywgt_write(be_reg, mpi_dyna_reg_cfg->rly_wgt);
    isp_sharpen_stdgainbymotlut_write(be_reg, mpi_dyna_reg_cfg->std_gain_by_mot_lut);
    isp_sharpen_stdoffsetbymotlut_write(be_reg, mpi_dyna_reg_cfg->std_offset_by_mot_lut);
    isp_sharpen_stdgainbyylut_write(be_reg, mpi_dyna_reg_cfg->std_gain_by_y_lut);
    isp_sharpen_stdoffsetbyylut_write(be_reg, mpi_dyna_reg_cfg->std_offset_by_y_lut);
    isp_sharpen_mfmotdec_write(be_reg, mpi_dyna_reg_cfg->mf_mot_dec);
    isp_sharpen_hfmotdec_write(be_reg, mpi_dyna_reg_cfg->hf_mot_dec);
    isp_sharpen_lmfmotgain_write(be_reg, mpi_dyna_reg_cfg->lmf_mot_gain);
}

static td_void isp_sharpen_def_dyna_reg_config(isp_post_be_reg_type *be_reg,
    isp_sharpen_default_dyna_reg_cfg *def_dyna_reg_cfg)
{
    /* sharpen default iso */
    isp_sharpen_mhfthdsftd_write(be_reg, def_dyna_reg_cfg->gain_thd_sft_d);
    isp_sharpen_mhfthdselud_write(be_reg, def_dyna_reg_cfg->gain_thd_sel_ud);
    isp_sharpen_mhfthdsftud_write(be_reg, def_dyna_reg_cfg->gain_thd_sft_ud);
    isp_sharpen_dirvarsft_write(be_reg, def_dyna_reg_cfg->dir_var_sft);
    isp_sharpen_shtvarwgt0_write(be_reg, def_dyna_reg_cfg->sht_var_wgt0);
    isp_sharpen_shtvardiffthd0_write(be_reg, def_dyna_reg_cfg->sht_var_diff_thd[0]);
    isp_sharpen_selpixwgt_write(be_reg, def_dyna_reg_cfg->sel_pix_wgt);
    isp_sharpen_shtvardiffthd1_write(be_reg, def_dyna_reg_cfg->sht_var_diff_thd[1]);
    isp_sharpen_shtvardiffwgt1_write(be_reg, def_dyna_reg_cfg->sht_var_diff_wgt1);
    isp_sharpen_shtvardiffmul_write(be_reg, def_dyna_reg_cfg->sht_var_diff_mul);
    isp_sharpen_rmfscale_write(be_reg, def_dyna_reg_cfg->rmf_gain_scale);
    isp_sharpen_bmfscale_write(be_reg, def_dyna_reg_cfg->bmf_gain_scale);

    isp_sharpen_dirvar_thd_write(be_reg, def_dyna_reg_cfg->dir_var_thd);
    isp_sharpen_mot_thd_write(be_reg, def_dyna_reg_cfg->mot_thd);
    isp_sharpen_mot_interp_mode_write(be_reg, def_dyna_reg_cfg->mot_interp_mode);
    isp_sharpen_std_comb_mode_write(be_reg, def_dyna_reg_cfg->std_comb_mode);
    isp_sharpen_std_comb_alpha_write(be_reg, def_dyna_reg_cfg->std_comb_alpha);
}

static td_void isp_sharpen_dyna_reg_config(isp_post_be_reg_type *be_reg, isp_sharpen_mpi_dyna_reg_cfg *mpi_dyna_reg_cfg,
    isp_sharpen_default_dyna_reg_cfg *def_dyna_reg_cfg)
{
    isp_sharpen_mpi_dyna_reg_config(be_reg, mpi_dyna_reg_cfg);
    isp_sharpen_def_dyna_reg_config(be_reg, def_dyna_reg_cfg);
}

static td_void isp_sharpen_lmt_hf_write(isp_post_be_reg_type *be_reg, td_u8 *lmt_hf)
{
    isp_sharpen_lmthf0_write(be_reg, lmt_hf[0]); /* array index 0 */
    isp_sharpen_lmthf1_write(be_reg, lmt_hf[1]); /* array index 1 */
    isp_sharpen_lmthf2_write(be_reg, lmt_hf[2]); /* array index 2 */
    isp_sharpen_lmthf3_write(be_reg, lmt_hf[3]); /* array index 3 */
    isp_sharpen_lmthf4_write(be_reg, lmt_hf[4]); /* array index 4 */
    isp_sharpen_lmthf5_write(be_reg, lmt_hf[5]); /* array index 5 */
    isp_sharpen_lmthf6_write(be_reg, lmt_hf[6]); /* array index 6 */
    isp_sharpen_lmthf7_write(be_reg, lmt_hf[7]); /* array index 7 */
}

static td_void isp_sharpen_lmt_mf_write(isp_post_be_reg_type *be_reg, td_u8 *lmt_mf)
{
    isp_sharpen_lmtmf0_write(be_reg, lmt_mf[0]); /* array index 0 */
    isp_sharpen_lmtmf1_write(be_reg, lmt_mf[1]); /* array index 1 */
    isp_sharpen_lmtmf2_write(be_reg, lmt_mf[2]); /* array index 2 */
    isp_sharpen_lmtmf3_write(be_reg, lmt_mf[3]); /* array index 3 */
    isp_sharpen_lmtmf4_write(be_reg, lmt_mf[4]); /* array index 4 */
    isp_sharpen_lmtmf5_write(be_reg, lmt_mf[5]); /* array index 5 */
    isp_sharpen_lmtmf6_write(be_reg, lmt_mf[6]); /* array index 6 */
    isp_sharpen_lmtmf7_write(be_reg, lmt_mf[7]); /* array index 7 */
}

static td_void isp_sharpen_chr_write(isp_post_be_reg_type *be_reg, isp_sharpen_static_reg_cfg *static_reg_cfg)
{
    isp_sharpen_chrrsft0_write(be_reg, static_reg_cfg->chr_r_sft[0]); /* array index 0 */
    isp_sharpen_chrrsft1_write(be_reg, static_reg_cfg->chr_r_sft[1]); /* array index 1 */
    isp_sharpen_chrrsft2_write(be_reg, static_reg_cfg->chr_r_sft[2]); /* array index 2 */
    isp_sharpen_chrrsft3_write(be_reg, static_reg_cfg->chr_r_sft[3]); /* array index 3 */
    isp_sharpen_chrrvarshift_write(be_reg, static_reg_cfg->chr_r_var_sft);

    isp_sharpen_chrbsft0_write(be_reg, static_reg_cfg->chr_b_sft[0]); /* array index 0 */
    isp_sharpen_chrbsft1_write(be_reg, static_reg_cfg->chr_b_sft[1]); /* array index 1 */
    isp_sharpen_chrbsft2_write(be_reg, static_reg_cfg->chr_b_sft[2]); /* array index 2 */
    isp_sharpen_chrbsft3_write(be_reg, static_reg_cfg->chr_b_sft[3]); /* array index 3 */
    isp_sharpen_chrbvarshift_write(be_reg, static_reg_cfg->chr_b_var_sft);
    isp_sharpen_chrgsft0_write(be_reg, static_reg_cfg->chr_g_sft[0]); /* array index 0 */
    isp_sharpen_chrgsft1_write(be_reg, static_reg_cfg->chr_g_sft[1]); /* array index 1 */
    isp_sharpen_chrgsft2_write(be_reg, static_reg_cfg->chr_g_sft[2]); /* array index 2 */
    isp_sharpen_chrgsft3_write(be_reg, static_reg_cfg->chr_g_sft[3]); /* array index 3 */

    isp_sharpen_chrgori0_write(be_reg, static_reg_cfg->chr_g_ori_cb);
    isp_sharpen_chrgori1_write(be_reg, static_reg_cfg->chr_g_ori_cr);
    isp_sharpen_chrgthd0_write(be_reg, static_reg_cfg->chr_g_thd[0]); /* array index 0 */
    isp_sharpen_chrgthd1_write(be_reg, static_reg_cfg->chr_g_thd[1]); /* array index 1 */
    isp_sharpen_chrrori0_write(be_reg, static_reg_cfg->chr_r_ori_cb);
    isp_sharpen_chrrori1_write(be_reg, static_reg_cfg->chr_r_ori_cr);
    isp_sharpen_chrrthd0_write(be_reg, static_reg_cfg->chr_r_thd[0]); /* array index 0 */
    isp_sharpen_chrrthd1_write(be_reg, static_reg_cfg->chr_r_thd[1]); /* array index 1 */

    isp_sharpen_chrbori0_write(be_reg, static_reg_cfg->chr_b_ori_cb);
    isp_sharpen_chrbori1_write(be_reg, static_reg_cfg->chr_b_ori_cr);
    isp_sharpen_chrbthd0_write(be_reg, static_reg_cfg->chr_b_thd[0]); /* array index 0 */
    isp_sharpen_chrbthd1_write(be_reg, static_reg_cfg->chr_b_thd[1]); /* array index 1 */
}

static td_void isp_sharpen_static_reg_config(isp_post_be_reg_type *be_reg, isp_sharpen_static_reg_cfg *static_reg_cfg)
{
    isp_sharpen_skincntthd0_write(be_reg, static_reg_cfg->skin_cnt_thd[0]); /* array index 0 */
    isp_sharpen_skincntthd1_write(be_reg, static_reg_cfg->skin_cnt_thd[1]); /* array index 1 */
    isp_sharpen_skincntmul_write(be_reg, static_reg_cfg->skin_cnt_mul);

    isp_sharpen_hfgain_sft_write(be_reg, static_reg_cfg->hf_gain_sft);
    isp_sharpen_mfgain_sft_write(be_reg, static_reg_cfg->mf_gain_sft);
    isp_sharpen_lfgainwgt_write(be_reg, static_reg_cfg->lf_gain_wgt);

    isp_sharpen_lmt_hf_write(be_reg, static_reg_cfg->lmt_hf);
    isp_sharpen_lmt_mf_write(be_reg, static_reg_cfg->lmt_mf);

    isp_sharpen_omaxchg_write(be_reg, static_reg_cfg->o_max_chg);
    isp_sharpen_umaxchg_write(be_reg, static_reg_cfg->u_max_chg);
    isp_sharpen_chr_write(be_reg, static_reg_cfg);

    isp_sharpen_ben8dir_sel_write(be_reg, static_reg_cfg->en_shp8_dir);
    isp_sharpen_benshtvar_sel_write(be_reg, static_reg_cfg->sht_var_sel);
    isp_sharpen_detailthd_sel_write(be_reg, static_reg_cfg->detail_thd_sel);
    isp_sharpen_dirrly0_write(be_reg, static_reg_cfg->dir_rly[0]); /* array index 0 */
    isp_sharpen_dirrly1_write(be_reg, static_reg_cfg->dir_rly[1]); /* array index 1 */
    isp_sharpen_dirvarscale_write(be_reg, static_reg_cfg->dir_var_scale);
    isp_sharpen_mhfthdseld_write(be_reg, static_reg_cfg->gain_thd_sel_d);
    isp_sharpen_max_var_clip_write(be_reg, static_reg_cfg->max_var_clip_min);
    isp_sharpen_shtvarthd0_write(be_reg, static_reg_cfg->sht_var_thd0);
    isp_sharpen_shtvarwgt1_write(be_reg, static_reg_cfg->sht_var_wgt1);
    isp_sharpen_shtvardiffwgt0_write(be_reg, static_reg_cfg->sht_var_diff_wgt0);
    isp_sharpen_shtvar5x5_sft_write(be_reg, static_reg_cfg->sht_var5x5_sft);
    isp_sharpen_shtvarsft_write(be_reg, static_reg_cfg->sht_var_sft);
    isp_sharpen_skinedgesft_write(be_reg, static_reg_cfg->skin_edge_sft);
    isp_sharpen_skinedgethd0_write(be_reg, static_reg_cfg->skin_edge_thd[0]); /* array index 0 */
    isp_sharpen_skinedgethd1_write(be_reg, static_reg_cfg->skin_edge_thd[1]); /* array index 1 */
    isp_sharpen_dtl_thdsft_write(be_reg, static_reg_cfg->detail_thd_sft);
    isp_sharpen_var7x9_calc_en_write(be_reg, static_reg_cfg->en_var7x9_calc);
}

static td_s32 isp_sharpen_usr_reg_config(ot_vi_pipe vi_pipe, td_u8 i, isp_post_be_reg_type *be_reg,
    isp_sharpen_mpi_dyna_reg_cfg *mpi_dyna_reg_cfg)
{
    td_u8 buf_id;
    td_s32 ret;
    isp_be_lut_wstt_type *be_lut_stt_reg = TD_NULL;
    isp_usr_ctx *isp_ctx = TD_NULL;
    isp_viproc_reg_type *post_viproc = TD_NULL;

    isp_get_ctx(vi_pipe, isp_ctx);

    post_viproc = (isp_viproc_reg_type *)isp_get_post_vi_proc_vir_addr(vi_pipe, i);
    isp_check_pointer_return(post_viproc);

    if (is_online_mode(isp_ctx->block_attr.running_mode)) {
        if (mpi_dyna_reg_cfg->switch_mode != TD_TRUE) {
            /* online lut2stt regconfig */
            buf_id = mpi_dyna_reg_cfg->buf_id;
            be_lut_stt_reg = (isp_be_lut_wstt_type *)isp_get_be_lut2stt_vir_addr(vi_pipe, i, buf_id);
            isp_check_pointer_return(be_lut_stt_reg);

            isp_sharpen_lut_wstt_write(be_lut_stt_reg, mpi_dyna_reg_cfg->mf_gain_d,
                mpi_dyna_reg_cfg->mf_gain_ud, mpi_dyna_reg_cfg->hf_gain_d, mpi_dyna_reg_cfg->hf_gain_ud);

            ret = isp_sharpen_lut_wstt_addr_write(vi_pipe, i, buf_id, post_viproc);
            if (ret != TD_SUCCESS) {
                isp_err_trace("ISP[%d] isp_sharpen_lut_wstt_addr_write failed\n", vi_pipe);
                return ret;
            }

            isp_sharpen_stt2lut_en_write(be_reg, TD_TRUE);
            isp_sharpen_stt2lut_regnew_write(be_reg, TD_TRUE);

            mpi_dyna_reg_cfg->buf_id = 1 - buf_id;
        }
    } else {
        isp_sharpen_lut_wstt_write(&be_reg->be_lut.be_lut2stt, mpi_dyna_reg_cfg->mf_gain_d,
            mpi_dyna_reg_cfg->mf_gain_ud, mpi_dyna_reg_cfg->hf_gain_d, mpi_dyna_reg_cfg->hf_gain_ud);

        isp_sharpen_stt2lut_en_write(be_reg, TD_TRUE);
        isp_sharpen_stt2lut_regnew_write(be_reg, TD_TRUE);
    }

    return TD_SUCCESS;
}

static td_void isp_sharpen_online_lut2stt_info_check(td_bool offline_mode, isp_post_be_reg_type *be_reg)
{
    td_u16 lut2stt_info;
    if (offline_mode == TD_TRUE) {
        return;
    }

    lut2stt_info = isp_sharpen_stt2lut_info_read(be_reg);
    if (lut2stt_info != 0) {
        isp_sharpen_stt2lut_regnew_write(be_reg, TD_TRUE);
        isp_sharpen_stt2lut_clr_write(be_reg, 1);
    }
}
static td_s32 isp_sharpen_reg_config(ot_vi_pipe vi_pipe, isp_reg_cfg *reg_cfg_info, td_u8 i)
{
    td_bool offline_mode;
    td_bool usr_resh;
    td_bool idx_resh;
    td_u8 blk_num = reg_cfg_info->cfg_num;
    td_s32 ret;
    isp_sharpen_mpi_dyna_reg_cfg *mpi_dyna_reg_cfg = TD_NULL;
    isp_sharpen_static_reg_cfg *static_reg_cfg = TD_NULL;
    isp_viproc_reg_type *post_viproc = TD_NULL;
    isp_post_be_reg_type *be_reg = TD_NULL;
    isp_usr_ctx *isp_ctx = TD_NULL;

    isp_get_ctx(vi_pipe, isp_ctx);

    offline_mode =
        (is_offline_mode(isp_ctx->block_attr.running_mode) || is_striping_mode(isp_ctx->block_attr.running_mode) ||
        is_pre_online_post_offline(isp_ctx->block_attr.running_mode));

    if (reg_cfg_info->cfg_key.bit1_sharpen_cfg) {
        be_reg = (isp_post_be_reg_type *)isp_get_post_be_vir_addr(vi_pipe, i);
        isp_check_pointer_return(be_reg);
        post_viproc = (isp_viproc_reg_type *)isp_get_post_vi_proc_vir_addr(vi_pipe, i);
        isp_check_pointer_return(post_viproc);

        static_reg_cfg = &reg_cfg_info->alg_reg_cfg[i].sharpen_reg_cfg.static_reg_cfg;
        mpi_dyna_reg_cfg = &reg_cfg_info->alg_reg_cfg[i].sharpen_reg_cfg.dyna_reg_cfg.mpi_dyna_reg_cfg;

        if (static_reg_cfg->static_resh) {
            isp_sharpen_lut_width_word_write(post_viproc, OT_ISP_SHARPEN_LUT_WIDTH_WORD_DEFAULT);

            isp_sharpen_static_reg_config(be_reg, static_reg_cfg);
            static_reg_cfg->static_resh = TD_FALSE;
        }

        idx_resh = (isp_sharpen_update_index_read(be_reg) != mpi_dyna_reg_cfg->update_index);
        usr_resh = (offline_mode) ? (mpi_dyna_reg_cfg->resh && idx_resh) : (mpi_dyna_reg_cfg->resh);

        if (usr_resh) {
            isp_sharpen_update_index_write(be_reg, mpi_dyna_reg_cfg->update_index);

            ret = isp_sharpen_usr_reg_config(vi_pipe, i, be_reg, mpi_dyna_reg_cfg);
            if (ret != TD_SUCCESS) {
                return ret;
            }

            mpi_dyna_reg_cfg->resh = offline_mode;
        }
        isp_sharpen_online_lut2stt_info_check(offline_mode, be_reg);
        reg_cfg_info->cfg_key.bit1_sharpen_cfg = offline_mode ? 1 : ((blk_num <= (i + 1)) ? 0 : 1);
    }

    return TD_SUCCESS;
}

static td_void isp_demosaic_static_reg_write(isp_post_be_reg_type *be_reg, isp_demosaic_static_cfg *static_reg_cfg)
{
    isp_demosaic_desat_enable_write(be_reg, static_reg_cfg->de_sat_enable);
    isp_demosaic_bld_limit1_write(be_reg, static_reg_cfg->hv_blend_limit1);
    isp_demosaic_bld_limit2_write(be_reg, static_reg_cfg->hv_blend_limit2);
    isp_demosaic_grad_limit1_write(be_reg, static_reg_cfg->hv_grad_limit1);
    isp_demosaic_grad_limit2_write(be_reg, static_reg_cfg->hv_grad_limit2);
    isp_demosaic_hv_color_ratio_write(be_reg, static_reg_cfg->hv_color_ratio);
    isp_demosaic_hv_detg_ratio_write(be_reg, static_reg_cfg->hv_detg_ratio);
    isp_demosaic_wgd_limit1_write(be_reg, static_reg_cfg->wgd_limit1);
    isp_demosaic_wgd_limit2_write(be_reg, static_reg_cfg->wgd_limit2);
    isp_demosaic_g_clip_sft_bit_write(be_reg, static_reg_cfg->g_clip_bit_sft);
    isp_demosaic_cx_var_max_rate_write(be_reg, static_reg_cfg->cx_var_max_rate);
    isp_demosaic_cx_var_min_rate_write(be_reg, static_reg_cfg->cx_var_min_rate);
    isp_demosaic_desat_thresh1_write(be_reg, static_reg_cfg->de_sat_thresh1);
    isp_demosaic_desat_thresh2_write(be_reg, static_reg_cfg->de_sat_thresh2);
    isp_demosaic_desat_hig_write(be_reg, static_reg_cfg->de_sat_hig);
    isp_demosaic_desat_protect_sl_write(be_reg, static_reg_cfg->de_sat_prot_sl);
    isp_demosaic_ahd_en_write(be_reg, static_reg_cfg->ahd_enable);
    isp_demosaic_ahd_par1_write(be_reg, static_reg_cfg->ahd_part1);
    isp_demosaic_ahd_par2_write(be_reg, static_reg_cfg->ahd_part2);
}

static td_void isp_demosaic_dyna_reg_write(isp_post_be_reg_type *be_reg, isp_demosaic_dyna_cfg *dyna_reg_cfg)
{
    isp_demosaic_cc_hf_max_ratio_write(be_reg, dyna_reg_cfg->cc_hf_max_ratio);
    isp_demosaic_cc_hf_min_ratio_write(be_reg, dyna_reg_cfg->cc_hf_min_ratio);
    isp_demosaic_filter_f0_write(be_reg, dyna_reg_cfg->lpff0);
    isp_demosaic_filter_f3_write(be_reg, dyna_reg_cfg->lpff3);
    isp_demosaic_nddm_str_write(be_reg, dyna_reg_cfg->nddmstr);
    isp_demosaic_nddm_ehcgray_write(be_reg, dyna_reg_cfg->ehc_gray);
    isp_demosaic_nddm_eps_sft_write(be_reg, dyna_reg_cfg->nddm_eps_sft);
    isp_demosaic_nddm_mode_write(be_reg, dyna_reg_cfg->nddm_mode);
    isp_demosaic_desat_low_write(be_reg, dyna_reg_cfg->de_sat_low);
    isp_demosaic_desat_ratio_write(be_reg, dyna_reg_cfg->de_sat_ratio);
    isp_demosaic_desat_protect_th_write(be_reg, dyna_reg_cfg->de_sat_prot_th);
    isp_demosaic_filter_blur1_write(be_reg, dyna_reg_cfg->filter_blur_th_low);
    isp_demosaic_filter_blur2_write(be_reg, dyna_reg_cfg->filter_blur_th_hig);

    isp_demosaic_var_thr_for_ahd_write(be_reg, dyna_reg_cfg->var_thr_for_ahd);
    isp_demosaic_var_thr_for_cac_write(be_reg, dyna_reg_cfg->var_thr_for_cac);
}

static td_s32 isp_demosaic_reg_config(ot_vi_pipe vi_pipe, isp_reg_cfg *reg_cfg_info, td_u8 i)
{
    td_bool offline_mode;
    td_bool gf_lut_update = TD_FALSE;
    td_u8 blk_num = reg_cfg_info->cfg_num;
    td_bool  stt2_lut_regnew = TD_FALSE;
    td_bool usr_resh = TD_FALSE;
    td_u8 idx_resh;
    isp_demosaic_static_cfg *static_reg_cfg = TD_NULL;
    isp_demosaic_dyna_cfg *dyna_reg_cfg = TD_NULL;
    isp_demosaic_usr_cfg *usr_reg_cfg = TD_NULL;
    isp_post_be_reg_type *be_reg = TD_NULL;
    isp_usr_ctx *isp_ctx = TD_NULL;

    isp_get_ctx(vi_pipe, isp_ctx);
    offline_mode =
        (is_offline_mode(isp_ctx->block_attr.running_mode) || is_striping_mode(isp_ctx->block_attr.running_mode) ||
        is_pre_online_post_offline(isp_ctx->block_attr.running_mode));
    be_reg = (isp_post_be_reg_type *)isp_get_post_be_vir_addr(vi_pipe, i);
    isp_check_pointer_return(be_reg);

    if (reg_cfg_info->cfg_key.bit1_dem_cfg) {
        static_reg_cfg = &reg_cfg_info->alg_reg_cfg[i].dem_reg_cfg.static_reg_cfg;
        dyna_reg_cfg = &reg_cfg_info->alg_reg_cfg[i].dem_reg_cfg.dyna_reg_cfg;

        if (static_reg_cfg->resh) { /* static */
            isp_demosaic_static_reg_write(be_reg, static_reg_cfg);
            reg_cfg_info->alg_reg_cfg[i].dem_reg_cfg.static_reg_cfg.resh = TD_FALSE;
        }
        if (dyna_reg_cfg->resh) { /* dynamic */
            isp_demosaic_dyna_reg_write(be_reg, dyna_reg_cfg);
            gf_lut_update = dyna_reg_cfg->update_gf;
            dyna_reg_cfg->resh = offline_mode;
        }

        /* usr */
        usr_reg_cfg = &reg_cfg_info->alg_reg_cfg[i].dem_reg_cfg.usr_reg_cfg;
        idx_resh = (isp_demosaic_update_index_read(be_reg) != usr_reg_cfg->update_index);
        usr_resh = (offline_mode) ? ((td_u8)usr_reg_cfg->resh & idx_resh) : (usr_reg_cfg->resh);
        if (usr_resh) {
            isp_demosaic_update_index_write(be_reg, usr_reg_cfg->update_index);
            usr_reg_cfg->resh = offline_mode;
        }

        reg_cfg_info->cfg_key.bit1_dem_cfg = offline_mode ? 1 : ((blk_num <= (i + 1)) ? 0 : 1);
    }
    reg_cfg_info->alg_reg_cfg[i].be_lut_update_cfg.nddm_gf_lut_update = gf_lut_update || offline_mode;
    reg_cfg_info->alg_reg_cfg[i].stt2lut_regnew_cfg.dmnr_stt2lut_regnew = stt2_lut_regnew;

    return TD_SUCCESS;
}

static td_s32 isp_fpn_reg_config(ot_vi_pipe vi_pipe, isp_reg_cfg *reg_cfg_info, td_u8 i)
{
#ifdef CONFIG_OT_VI_PIPE_FPN
    td_bool offline_mode;
    td_u8 blk_num = reg_cfg_info->cfg_num;

    isp_fpn_dyna_cfg *dyna_reg_cfg = TD_NULL;
    isp_pre_be_reg_type *be_reg = TD_NULL;
    isp_usr_ctx *isp_ctx = TD_NULL;

    isp_get_ctx(vi_pipe, isp_ctx);
    offline_mode =
        (is_offline_mode(isp_ctx->block_attr.running_mode) || is_striping_mode(isp_ctx->block_attr.running_mode) ||
        is_pre_online_post_offline(isp_ctx->block_attr.running_mode));

    if (reg_cfg_info->cfg_key.bit1_fpn_cfg) {
        be_reg = (isp_pre_be_reg_type *)isp_get_pre_be_vir_addr(vi_pipe, i);
        isp_check_pointer_return(be_reg);

        dyna_reg_cfg = &reg_cfg_info->alg_reg_cfg[i].fpn_reg_cfg.dyna_reg_cfg;

        isp_fpn_overflowthr_write(be_reg, dyna_reg_cfg->isp_fpn_overflow_thr);
        isp_fpn_strength0_write(be_reg, dyna_reg_cfg->isp_fpn_strength[0]);

        reg_cfg_info->cfg_key.bit1_fpn_cfg = offline_mode ? 1 : ((blk_num <= (i + 1)) ? 0 : 1);
    }
#else
    ot_unused(vi_pipe);
    ot_unused(reg_cfg_info);
    ot_unused(i);
#endif
    return TD_SUCCESS;
}

static td_s32 isp_ldci_read_stt_addr_write(isp_viproc_reg_type *post_viproc, td_phys_addr_t phy_addr)
{
    if (phy_addr == 0) {
        return TD_FAILURE;
    }

    viproc_para_dci_addr_low_write(post_viproc, get_low_addr(phy_addr));

    return TD_SUCCESS;
}

static td_s32 isp_ldci_static_lut_reg_config(ot_vi_pipe vi_pipe, td_u8 i, isp_post_be_reg_type *be_reg,
    isp_ldci_dyna_cfg *dyna_reg_cfg)
{
    td_u8 k;
    isp_usr_ctx *isp_ctx = TD_NULL;
    isp_be_lut_wstt_type *be_lut_stt_reg = TD_NULL;

    isp_get_ctx(vi_pipe, isp_ctx);

    if (is_online_mode(isp_ctx->block_attr.running_mode)) {
        for (k = 0; k < 2; k++) { /* config all 2 lut2stt buffer for static lut */
            be_lut_stt_reg = (isp_be_lut_wstt_type *)isp_get_be_lut2stt_vir_addr(vi_pipe, i, k);
            isp_check_pointer_return(be_lut_stt_reg);
            isp_ldci_drc_cgain_lut_wstt_write(be_lut_stt_reg, dyna_reg_cfg->color_gain_lut);
        }
    } else {
        isp_ldci_drc_cgain_lut_wstt_write(&be_reg->be_lut.be_lut2stt, dyna_reg_cfg->color_gain_lut);
    }

    return TD_SUCCESS;
}

static td_s32 isp_ldci_tpr_flt_attr_config(ot_vi_pipe vi_pipe, td_u8 i, isp_post_be_reg_type *be_reg,
    isp_viproc_reg_type *post_viproc, isp_ldci_dyna_cfg *dyna_reg_cfg)
{
    td_bool rdstat_en;
    td_u8 read_buf_idx;
    td_u8 blk_num;
    td_s32 ret;
    isp_usr_ctx *isp_ctx = TD_NULL;
    isp_ldci_read_stt_buf *ldci_read_stt_buf = TD_NULL;
    isp_ldci_stat *read_stt_buf = TD_NULL;

    isp_get_ctx(vi_pipe, isp_ctx);

    blk_num = isp_ctx->block_attr.block_num;

    if (isp_ctx->ldci_tpr_flt_en == TD_TRUE) {
        ldci_buf_get_ctx(vi_pipe, ldci_read_stt_buf);
        read_buf_idx = ldci_read_stt_buf->buf_idx;

        if (i == 0) {
            read_stt_buf = (isp_ldci_stat *)isp_get_ldci_read_stt_vir_addr(vi_pipe, read_buf_idx);
            isp_check_pointer_return(read_stt_buf);
            (td_void)memcpy_s(read_stt_buf, sizeof(isp_ldci_stat), &dyna_reg_cfg->tpr_stat, sizeof(isp_ldci_stat));
        }

        if ((i + 1) == blk_num) {
            ldci_read_stt_buf->buf_idx = (read_buf_idx + 1) % div_0_to_1(ldci_read_stt_buf->buf_num);
        }

        /* Set ReadStt Addr */
        ret = isp_ldci_read_stt_addr_write(post_viproc, ldci_read_stt_buf->read_buf[read_buf_idx].phy_addr);
        rdstat_en = (ret == TD_SUCCESS) ? (dyna_reg_cfg->rdstat_en) : (TD_FALSE);
        isp_ldci_rdstat_en_write(be_reg, rdstat_en);
    } else {
        isp_ldci_rdstat_en_write(be_reg, dyna_reg_cfg->rdstat_en);
    }

    return TD_SUCCESS;
}

static td_s32 isp_ldci_dyna_lut_reg_config(ot_vi_pipe vi_pipe, td_u8 i, isp_post_be_reg_type *be_reg,
    isp_viproc_reg_type *post_viproc, isp_ldci_dyna_cfg *dyna_reg_cfg)
{
    td_u8 buf_id;
    td_s32 ret;
    isp_usr_ctx *isp_ctx = TD_NULL;
    isp_be_lut_wstt_type *be_lut_stt_reg = TD_NULL;

    isp_get_ctx(vi_pipe, isp_ctx);

    if (is_online_mode(isp_ctx->block_attr.running_mode)) {
        /* online Lut2stt regconfig */
        buf_id = dyna_reg_cfg->buf_id;

        be_lut_stt_reg = (isp_be_lut_wstt_type *)isp_get_be_lut2stt_vir_addr(vi_pipe, i, buf_id);
        isp_check_pointer_return(be_lut_stt_reg);

        isp_ldci_he_lut_wstt_write(be_lut_stt_reg,
            dyna_reg_cfg->he_pos_lut, dyna_reg_cfg->he_neg_lut);
        ret = isp_ldci_lut_wstt_addr_write(vi_pipe, i, buf_id, post_viproc);
        if (ret != TD_SUCCESS) {
            isp_err_trace("isp[%d]_ldci_lut_wstt_addr_write failed\n", vi_pipe);
            return ret;
        }
        dyna_reg_cfg->buf_id = 1 - buf_id;
    } else {
        isp_ldci_he_lut_wstt_write(&be_reg->be_lut.be_lut2stt,
            dyna_reg_cfg->he_pos_lut, dyna_reg_cfg->he_neg_lut);
    }

    isp_ldci_lut_width_word_write(post_viproc, OT_ISP_LDCI_LUT_WIDTH_WORD_DEFAULT);

    isp_ldci_stt2lut_en_write(be_reg, TD_TRUE);
    isp_ldci_stt2lut_regnew_write(be_reg, TD_TRUE);

    return TD_SUCCESS;
}

static td_s32 isp_ldci_static_reg_config(ot_vi_pipe vi_pipe, isp_reg_cfg *reg_cfg_info, td_u8 i,
    isp_post_be_reg_type *be_reg)
{
    td_s32 ret;
    isp_ldci_static_cfg *static_reg_cfg = TD_NULL;
    isp_ldci_dyna_cfg *dyna_reg_cfg = TD_NULL;

    static_reg_cfg = &reg_cfg_info->alg_reg_cfg[i].ldci_reg_cfg.static_reg_cfg;
    dyna_reg_cfg = &reg_cfg_info->alg_reg_cfg[i].ldci_reg_cfg.dyna_reg_cfg;

    isp_ldci_stat_evratio_write(be_reg, 0x1000);
    isp_ldci_luma_sel_write(be_reg, static_reg_cfg->calc_luma_sel);
    isp_ldci_lpfsft_write(be_reg, static_reg_cfg->lpf_sft);
    isp_ldci_chrposdamp_write(be_reg, static_reg_cfg->chr_pos_damp);
    isp_ldci_chrnegdamp_write(be_reg, static_reg_cfg->chr_neg_damp);

    ret = isp_ldci_static_lut_reg_config(vi_pipe, i, be_reg, dyna_reg_cfg);
    if (ret != TD_SUCCESS) {
        return ret;
    }

    return TD_SUCCESS;
}

static td_s32 isp_ldci_dyna_reg_config(ot_vi_pipe vi_pipe, td_u8 i, isp_post_be_reg_type *be_reg,
    isp_viproc_reg_type *post_viproc, isp_ldci_dyna_cfg *dyna_reg_cfg)
{
    td_s32 ret;
    ret = isp_ldci_tpr_flt_attr_config(vi_pipe, i, be_reg, post_viproc, dyna_reg_cfg);
    if (ret != TD_SUCCESS) {
        return ret;
    }

    isp_ldci_wrstat_en_write(be_reg, dyna_reg_cfg->wrstat_en);
    isp_ldci_calc_en_write(be_reg, dyna_reg_cfg->calc_enable);
    isp_ldci_calc_map_offsetx_write(be_reg, dyna_reg_cfg->calc_map_offset_x);
    isp_ldci_smlmapstride_write(be_reg, dyna_reg_cfg->calc_sml_map_stride);
    isp_ldci_smlmapheight_write(be_reg, dyna_reg_cfg->calc_sml_map_height);
    isp_ldci_total_zone_write(be_reg, dyna_reg_cfg->calc_total_zone);
    isp_ldci_scalex_write(be_reg, dyna_reg_cfg->calc_scale_x);
    isp_ldci_scaley_write(be_reg, dyna_reg_cfg->calc_scale_y);
    isp_ldci_stat_smlmapwidth_write(be_reg, dyna_reg_cfg->stat_sml_map_width);
    isp_ldci_stat_smlmapheight_write(be_reg, dyna_reg_cfg->stat_sml_map_height);
    isp_ldci_stat_total_zone_write(be_reg, dyna_reg_cfg->stat_total_zone);
    isp_ldci_blk_smlmapwidth0_write(be_reg, dyna_reg_cfg->blk_sml_map_width[0]); /* array index 0 */
    isp_ldci_blk_smlmapwidth1_write(be_reg, dyna_reg_cfg->blk_sml_map_width[1]); /* array index 1 */
    isp_ldci_blk_smlmapwidth2_write(be_reg, dyna_reg_cfg->blk_sml_map_width[2]); /* array index 2 */
    isp_ldci_hstart_write(be_reg, dyna_reg_cfg->stat_h_start);
    isp_ldci_hend_write(be_reg, dyna_reg_cfg->stat_h_end);
    isp_ldci_vstart_write(be_reg, dyna_reg_cfg->stat_v_start);
    isp_ldci_vend_write(be_reg, dyna_reg_cfg->stat_v_end);
    isp_ldci_lpf_en_write(be_reg, dyna_reg_cfg->lpf_enable);
    isp_ldci_stat_offset_write(be_reg, dyna_reg_cfg->stat_offset);

    ret = isp_ldci_dyna_lut_reg_config(vi_pipe, i, be_reg, post_viproc, dyna_reg_cfg);
    if (ret != TD_SUCCESS) {
        return ret;
    }

    return TD_SUCCESS;
}

static td_s32 isp_ldci_reg_config(ot_vi_pipe vi_pipe, isp_reg_cfg *reg_cfg_info, td_u8 i)
{
    td_bool offline_mode, cur_ldci_calc_lut_reg_new;
    td_bool ldci_drc_lut_update = TD_FALSE;
    td_bool cur_ldci_drc_lut_reg_new = TD_FALSE;
    td_bool ldci_calc_lut_update = TD_FALSE;
    td_u8 blk_num = reg_cfg_info->cfg_num;
    td_s32 ret;
    isp_ldci_static_cfg *static_reg_cfg = TD_NULL;
    isp_ldci_dyna_cfg *dyna_reg_cfg = TD_NULL;
    isp_viproc_reg_type *post_viproc = TD_NULL;
    isp_post_be_reg_type *be_reg = TD_NULL;
    isp_usr_ctx *isp_ctx = TD_NULL;

    isp_get_ctx(vi_pipe, isp_ctx);

    offline_mode =
        (is_offline_mode(isp_ctx->block_attr.running_mode) || is_striping_mode(isp_ctx->block_attr.running_mode) ||
        is_pre_online_post_offline(isp_ctx->block_attr.running_mode));

    if (reg_cfg_info->cfg_key.bit1_ldci_cfg) {
        be_reg = (isp_post_be_reg_type *)isp_get_post_be_vir_addr(vi_pipe, i);
        isp_check_pointer_return(be_reg);
        post_viproc = (isp_viproc_reg_type *)isp_get_post_vi_proc_vir_addr(vi_pipe, i);
        isp_check_pointer_return(post_viproc);

        static_reg_cfg = &reg_cfg_info->alg_reg_cfg[i].ldci_reg_cfg.static_reg_cfg;
        dyna_reg_cfg = &reg_cfg_info->alg_reg_cfg[i].ldci_reg_cfg.dyna_reg_cfg;

        if (static_reg_cfg->static_resh) {
            ret = isp_ldci_static_reg_config(vi_pipe, reg_cfg_info, i, be_reg);
            if (ret != TD_SUCCESS) {
                return ret;
            }

            cur_ldci_drc_lut_reg_new = TD_TRUE;
            static_reg_cfg->static_resh = TD_FALSE;
        }

        ldci_drc_lut_update = static_reg_cfg->pre_drc_lut_update;
        static_reg_cfg->pre_drc_lut_update = cur_ldci_drc_lut_reg_new;

        /* dynamic */
        isp_ldci_en_write(post_viproc, dyna_reg_cfg->enable);
        ret = isp_ldci_dyna_reg_config(vi_pipe, i, be_reg, post_viproc, dyna_reg_cfg);
        if (ret != TD_SUCCESS) {
            return ret;
        }

        cur_ldci_calc_lut_reg_new = TD_TRUE;
        ldci_calc_lut_update = dyna_reg_cfg->pre_calc_lut_reg_new;
        dyna_reg_cfg->pre_calc_lut_reg_new = cur_ldci_calc_lut_reg_new;

        reg_cfg_info->cfg_key.bit1_ldci_cfg = offline_mode ? 1 : ((blk_num <= (i + 1)) ? 0 : 1);
    }

    reg_cfg_info->alg_reg_cfg[i].be_lut_update_cfg.ldci_calc_lut_update = ldci_calc_lut_update || offline_mode;
    reg_cfg_info->alg_reg_cfg[i].be_lut_update_cfg.ldci_drc_lut_update = ldci_drc_lut_update || offline_mode;

    return TD_SUCCESS;
}

static td_s32 isp_fcr_reg_config(ot_vi_pipe vi_pipe, isp_reg_cfg *reg_cfg_info, td_u8 i)
{
    td_u8 blk_num = reg_cfg_info->cfg_num;
    td_bool offline_mode;
    isp_antifalsecolor_dyna_cfg *dyna_reg_cfg = TD_NULL;
    isp_antifalsecolor_static_cfg *static_reg_cfg = TD_NULL;
    isp_post_be_reg_type *be_reg = TD_NULL;
    isp_usr_ctx *isp_ctx = TD_NULL;

    isp_get_ctx(vi_pipe, isp_ctx);

    offline_mode =
        (is_offline_mode(isp_ctx->block_attr.running_mode) || is_striping_mode(isp_ctx->block_attr.running_mode) ||
        is_pre_online_post_offline(isp_ctx->block_attr.running_mode));

    if (reg_cfg_info->cfg_key.bit1_fcr_cfg) {
        be_reg = (isp_post_be_reg_type *)isp_get_post_be_vir_addr(vi_pipe, i);
        isp_check_pointer_return(be_reg);

        static_reg_cfg = &reg_cfg_info->alg_reg_cfg[i].anti_false_color_reg_cfg.static_reg_cfg;
        dyna_reg_cfg = &reg_cfg_info->alg_reg_cfg[i].anti_false_color_reg_cfg.dyna_reg_cfg;

        /* static */
        if (static_reg_cfg->resh) {
            isp_demosaic_fcr_limit1_write(be_reg, static_reg_cfg->fcr_limit1);
            isp_demosaic_fcr_limit2_write(be_reg, static_reg_cfg->fcr_limit2);
            isp_demosaic_fcr_remove_write(be_reg, static_reg_cfg->fcr_thr);
            static_reg_cfg->resh = TD_FALSE;
        }

        /* dynamic */
        if (dyna_reg_cfg->resh) {
            isp_demosaic_fcr_gain_write(be_reg, dyna_reg_cfg->fcr_gain);
            isp_demosaic_fcr_ratio_write(be_reg, dyna_reg_cfg->fcr_ratio);
            isp_demosaic_fcr_gray_ratio_write(be_reg, dyna_reg_cfg->fcr_gray_ratio);
            isp_demosaic_fcr_hfthresh1_write(be_reg, dyna_reg_cfg->fcr_hf_thresh_low);
            isp_demosaic_fcr_hfthresh2_write(be_reg, dyna_reg_cfg->fcr_hf_thresh_hig);
            dyna_reg_cfg->resh = offline_mode;
        }

        reg_cfg_info->cfg_key.bit1_fcr_cfg = offline_mode ? 1 : ((blk_num <= (i + 1)) ? 0 : 1);
    }

    return TD_SUCCESS;
}

static td_void isp_cac_static_reg_config(isp_be_reg_type *be_reg, isp_cac_static_cfg *static_reg_cfg)
{
    isp_demosaic_local_cac_en_write(be_reg, static_reg_cfg->local_cac_en);
    isp_demosaic_r_counter_thr_write(be_reg, static_reg_cfg->r_counter_thr);
    isp_demosaic_g_counter_thr_write(be_reg, static_reg_cfg->g_counter_thr);
    isp_demosaic_b_counter_thr_write(be_reg, static_reg_cfg->b_counter_thr);
    isp_demosaic_lcac_ratio_thd_write(be_reg, static_reg_cfg->lcac_ratio_thd);
    isp_demosaic_lcac_ratio_sft_write(be_reg, static_reg_cfg->lcac_ratio_sft);
    isp_demosaic_lcac_g_avg_thd_write(be_reg, static_reg_cfg->lcac_g_avg_thd);
    isp_demosaic_lcac_g_avg_sft_write(be_reg, static_reg_cfg->lcac_g_avg_sft);
    isp_demosaic_over_exp_thd_write(be_reg, static_reg_cfg->over_exp_thd);
    isp_demosaic_b_diff_sft_write(be_reg, static_reg_cfg->b_diff_sft);
    isp_demosaic_gcac_rb_diff_thd_write(be_reg, static_reg_cfg->diff_thd);
}
static td_void isp_cac_usr_reg_config(isp_be_reg_type *be_reg, isp_cac_usr_cfg *usr_reg_cfg)
{
    isp_gcac_update_index_write(be_reg, usr_reg_cfg->update_index);
    isp_demosaic_gcac_edgemode_write(be_reg, usr_reg_cfg->cac_det_mode);
    isp_demosaic_cbcr_ratio_high_limit_write(be_reg, usr_reg_cfg->cr_cb_ratio_high_limit);
    isp_demosaic_cbcr_ratio_low_limit_write(be_reg, usr_reg_cfg->cr_cb_ratio_low_limit);
    isp_demosaic_r_luma_thr_write(be_reg, usr_reg_cfg->r_luma_thr);
    isp_demosaic_g_luma_thr_write(be_reg, usr_reg_cfg->g_luma_thr);
    isp_demosaic_b_luma_thr_write(be_reg, usr_reg_cfg->b_luma_thr);
    isp_demosaic_plat_thd_write(be_reg, usr_reg_cfg->plat_thd);
    isp_demosaic_g_var_thr_for_purple_write(be_reg, usr_reg_cfg->var_thr);
}

static td_void isp_cac_dyna_reg_config(isp_be_reg_type *be_reg, isp_cac_dyna_cfg *dyna_reg_cfg)
{
    isp_demosaic_gcac_globalstr_write(be_reg, dyna_reg_cfg->cac_edge_str);
    isp_demosaic_gcac_lamdathd1_write(be_reg, dyna_reg_cfg->lamda_thd0);
    isp_demosaic_gcac_lamdathd2_write(be_reg, dyna_reg_cfg->lamda_thd1);
    isp_demosaic_gcac_lamdamul_write(be_reg, dyna_reg_cfg->lamda_mul);
    isp_demosaic_gcac_thrbthd1_write(be_reg, dyna_reg_cfg->edge_thd0);
    isp_demosaic_gcac_thrbthd2_write(be_reg, dyna_reg_cfg->edge_thd1);
    isp_demosaic_gcac_thrbmul_write(be_reg, dyna_reg_cfg->edge_thd_mul);
    isp_demosaic_gcac_tao_write(be_reg, dyna_reg_cfg->cac_tao);
    isp_demosaic_gcac_purplealpha_write(be_reg, dyna_reg_cfg->purple_alpha);
    isp_demosaic_gcac_edge_alpha_write(be_reg, dyna_reg_cfg->edge_alpha);
    isp_demosaic_satu_thr_write(be_reg, dyna_reg_cfg->det_satu_thr);
    isp_demosaic_depurplectrcr_write(be_reg, dyna_reg_cfg->de_purple_ctr_cr);
    isp_demosaic_depurplectrcb_write(be_reg, dyna_reg_cfg->de_purple_ctr_cb);
}

static td_s32 isp_cac_reg_config(ot_vi_pipe vi_pipe, isp_reg_cfg *reg_cfg_info, td_u8 i)
{
    td_bool offline_mode;
    td_u8 blk_num = reg_cfg_info->cfg_num;
    td_bool usr_resh = TD_FALSE;
    td_u8 idx_resh;
    isp_cac_usr_cfg *usr_reg_cfg = TD_NULL;
    isp_post_be_reg_type *be_reg = TD_NULL;
    isp_cac_static_cfg *static_reg_cfg = TD_NULL;
    isp_cac_dyna_cfg *dyna_reg_cfg = TD_NULL;
    isp_usr_ctx *isp_ctx = TD_NULL;
    isp_get_ctx(vi_pipe, isp_ctx);
    offline_mode =
        (is_offline_mode(isp_ctx->block_attr.running_mode) || is_striping_mode(isp_ctx->block_attr.running_mode) ||
        is_pre_online_post_offline(isp_ctx->block_attr.running_mode));
    if (reg_cfg_info->cfg_key.bit1_cac_cfg) {
        be_reg = (isp_post_be_reg_type *)isp_get_post_be_vir_addr(vi_pipe, i);
        isp_check_pointer_return(be_reg);
        /* static */
        static_reg_cfg = &reg_cfg_info->alg_reg_cfg[i].cac_reg_cfg.static_reg_cfg;
        if (static_reg_cfg->static_resh) {
            isp_cac_static_reg_config(be_reg, static_reg_cfg);
            static_reg_cfg->static_resh = TD_FALSE;
        }
        /* usr */
        usr_reg_cfg = &reg_cfg_info->alg_reg_cfg[i].cac_reg_cfg.usr_reg_cfg;
        idx_resh = (isp_gcac_update_index_read(be_reg) != usr_reg_cfg->update_index);
        usr_resh = (offline_mode) ? ((td_u8)usr_reg_cfg->usr_resh & idx_resh) : (usr_reg_cfg->usr_resh);
        if (usr_resh) {
            isp_cac_usr_reg_config(be_reg, usr_reg_cfg);
            usr_reg_cfg->usr_resh = offline_mode;
        }
        /* dyna */
        dyna_reg_cfg = &reg_cfg_info->alg_reg_cfg[i].cac_reg_cfg.dyna_reg_cfg;
        if (dyna_reg_cfg->dyna_resh) {
            isp_cac_dyna_reg_config(be_reg, dyna_reg_cfg);
            dyna_reg_cfg->dyna_resh = TD_FALSE;
        }
        reg_cfg_info->cfg_key.bit1_cac_cfg = offline_mode ? 1 : ((blk_num <= (i + 1)) ? 0 : 1);
    }

    return TD_SUCCESS;
}

static td_s32 isp_dpc_usr_reg_config(ot_vi_pipe vi_pipe, isp_reg_cfg *reg_cfg_info, td_u8 i,
    isp_pre_be_reg_type *be_reg, td_bool offline_mode)
{
    isp_dpc_usr_cfg *usr_reg_cfg = TD_NULL;
    td_bool usr_resh, idx_resh;
    td_bool stt2_lut_regnew = TD_FALSE;

    usr_reg_cfg = &reg_cfg_info->alg_reg_cfg[i].dp_reg_cfg.usr_reg_cfg;

    if (usr_reg_cfg->usr_dyna_cor_reg_cfg.resh) {
        isp_dpc_ex_hard_thr_en_write(be_reg, usr_reg_cfg->usr_dyna_cor_reg_cfg.hard_thr_en[0]);
        isp_dpc_ex_rake_ratio_write(be_reg, usr_reg_cfg->usr_dyna_cor_reg_cfg.rake_ratio[0]);
        isp_dpc_soft_thr_max_write(be_reg, usr_reg_cfg->usr_dyna_cor_reg_cfg.sup_twinkle_thr_max[0]);
        isp_dpc_soft_thr_min_write(be_reg, usr_reg_cfg->usr_dyna_cor_reg_cfg.sup_twinkle_thr_min[0]);

        usr_reg_cfg->usr_dyna_cor_reg_cfg.resh = offline_mode;
    }

    idx_resh = (isp_dpc_update_index_read(be_reg) != usr_reg_cfg->usr_sta_cor_reg_cfg.update_index);
    usr_resh =
        (offline_mode) ? (usr_reg_cfg->usr_sta_cor_reg_cfg.resh && idx_resh) : (usr_reg_cfg->usr_sta_cor_reg_cfg.resh);

    if (usr_resh) {
        isp_dpc_update_index_write(be_reg, usr_reg_cfg->usr_sta_cor_reg_cfg.update_index);

        usr_reg_cfg->usr_sta_cor_reg_cfg.resh = offline_mode;
    }

    reg_cfg_info->alg_reg_cfg[i].stt2lut_regnew_cfg.dpc_stt2lut_regnew = stt2_lut_regnew;

    return TD_SUCCESS;
}

static td_void isp_dpc_dyn_reg_config(isp_pre_be_reg_type *be_reg, isp_dpc_dyna_cfg *dyna_reg_cfg)
{
    isp_dpc_blend_write(be_reg, dyna_reg_cfg->dpcc_alpha[0]);
    isp_dpc_mode_write(be_reg, dyna_reg_cfg->dpcc_mode[0]);
    isp_dpc_set_use_write(be_reg, dyna_reg_cfg->dpcc_set_use[0]);
    isp_dpc_methods_set_1_write(be_reg, dyna_reg_cfg->dpcc_methods_set1[0]);
    isp_dpc_line_thresh_1_write(be_reg, dyna_reg_cfg->dpcc_line_thr[0]);
    isp_dpc_line_mad_fac_1_write(be_reg, dyna_reg_cfg->dpcc_line_mad_fac[0]);
    isp_dpc_pg_fac_1_write(be_reg, dyna_reg_cfg->dpcc_pg_fac[0]);
    isp_dpc_rg_fac_1_write(be_reg, dyna_reg_cfg->dpcc_rg_fac[0]);
    isp_dpc_ro_limits_write(be_reg, dyna_reg_cfg->dpcc_ro_limits[0]);

    isp_dpc_line_kerdiff_fac_write(be_reg, dyna_reg_cfg->dpcc_line_kerdiff_fac[0]);
    isp_dpc_amp_coef_k_write(be_reg, dyna_reg_cfg->dpcc_amp_coef_k[0]);
    isp_dpc_amp_coef_min_write(be_reg, dyna_reg_cfg->dpcc_amp_coef_min[0]);

    isp_dpc_line_std_thr_1_write(be_reg, dyna_reg_cfg->dpcc_line_std_thr[0]);
    isp_dpc_line_diff_thr_1_write(be_reg, dyna_reg_cfg->dpcc_line_diff_thr[0]);
    isp_dpc_line_aver_fac_1_write(be_reg, dyna_reg_cfg->dpcc_line_aver_fac[0]);
    isp_dpc_rg_fac_1_mtp_write(be_reg, dyna_reg_cfg->dpcc_rg_fac_mtp[0]);
}

static td_s32 isp_dpc_reg_config(ot_vi_pipe vi_pipe, isp_reg_cfg *reg_cfg_info, td_u8 i)
{
    td_bool offline_mode;

    td_u8 blk_num = reg_cfg_info->cfg_num;
    td_s32 ret;
    isp_dpc_static_cfg *static_reg_cfg = TD_NULL;
    isp_dpc_dyna_cfg *dyna_reg_cfg = TD_NULL;

    isp_pre_be_reg_type *be_reg = TD_NULL;
    isp_viproc_reg_type *pre_viproc = TD_NULL;
    isp_usr_ctx *isp_ctx = TD_NULL;

    isp_get_ctx(vi_pipe, isp_ctx);
    be_reg = (isp_pre_be_reg_type *)isp_get_pre_be_vir_addr(vi_pipe, i);
    pre_viproc = (isp_viproc_reg_type *)isp_get_pre_vi_proc_vir_addr(vi_pipe, i);
    isp_check_pointer_return(be_reg);
    isp_check_pointer_return(pre_viproc);
    offline_mode =
        (is_offline_mode(isp_ctx->block_attr.running_mode) || is_striping_mode(isp_ctx->block_attr.running_mode));

    if (is_wdr_mode(isp_ctx->sns_wdr_mode)) {
        isp_dcg_sel_write(be_reg, 0); // dcg before demosaic
    } else {
        isp_dcg_sel_write(be_reg, 1); // dcg before wdr
    }

    if (reg_cfg_info->cfg_key.bit1_dp_cfg) {
        /* static */
        static_reg_cfg = &reg_cfg_info->alg_reg_cfg[i].dp_reg_cfg.static_reg_cfg;

        if (static_reg_cfg->static_resh) {
            static_reg_cfg->static_resh = TD_FALSE;
        }

        /* usr */
        ret = isp_dpc_usr_reg_config(vi_pipe, reg_cfg_info, i, be_reg, offline_mode);
        if (ret != TD_SUCCESS) {
            return ret;
        }

        /* dynamic */
        dyna_reg_cfg = &reg_cfg_info->alg_reg_cfg[i].dp_reg_cfg.dyna_reg_cfg;

        if (dyna_reg_cfg->resh) {
            isp_dpc_stat_en_write(pre_viproc, dyna_reg_cfg->dpc_stat_en);
            isp_dpc_dyn_reg_config(be_reg, dyna_reg_cfg);
            dyna_reg_cfg->resh = offline_mode;
        }

        reg_cfg_info->cfg_key.bit1_dp_cfg = offline_mode ? 1 : ((blk_num <= (i + 1)) ? 0 : 1);
    }

    return TD_SUCCESS;
}

#ifdef CONFIG_OT_ISP_CR_SUPPORT
static td_void isp_ge_static_reg_config(isp_pre_be_reg_type *be_reg, isp_ge_static_cfg *static_reg_cfg)
{
    if (static_reg_cfg->static_resh) {
        static_reg_cfg->static_resh = TD_FALSE;
    }
}

static td_void isp_ge_usr_reg_config(isp_pre_be_reg_type *be_reg, isp_ge_usr_cfg *usr_reg_cfg)
{
    isp_ge_ge0_ct_th2_write(be_reg, usr_reg_cfg->ge_ct_th2[0]);

    isp_ge_ge0_ct_slope1_write(be_reg, usr_reg_cfg->ge_ct_slope1[0]);

    isp_ge_ge0_ct_slope2_write(be_reg, usr_reg_cfg->ge_ct_slope2[0]);
}

static td_void isp_ge_dyna_reg_config(isp_pre_be_reg_type *be_reg, isp_ge_dyna_cfg *dyna_reg_cfg)
{
    isp_ge_ge0_ct_th1_write(be_reg, dyna_reg_cfg->ge_ct_th1[0]);

    isp_ge_ge0_ct_th3_write(be_reg, dyna_reg_cfg->ge_ct_th3[0]);

    isp_ge_strength_write(be_reg, dyna_reg_cfg->ge_strength);
}
#endif
static td_s32 isp_ge_reg_config(ot_vi_pipe vi_pipe, isp_reg_cfg *reg_cfg_info, td_u8 i)
{
#ifdef CONFIG_OT_ISP_CR_SUPPORT
    td_u8 blk_num = reg_cfg_info->cfg_num;
    td_bool offline_mode;
    isp_ge_dyna_cfg *dyna_reg_cfg = TD_NULL;
    isp_ge_usr_cfg *usr_reg_cfg = TD_NULL;
    isp_pre_be_reg_type *be_reg = TD_NULL;
    isp_usr_ctx *isp_ctx = TD_NULL;

    isp_get_ctx(vi_pipe, isp_ctx);
    offline_mode =
        (is_offline_mode(isp_ctx->block_attr.running_mode) || is_striping_mode(isp_ctx->block_attr.running_mode));

    if (reg_cfg_info->cfg_key.bit1_ge_cfg) {
        be_reg = (isp_pre_be_reg_type *)isp_get_pre_be_vir_addr(vi_pipe, i);
        isp_check_pointer_return(be_reg);

        isp_ge_static_reg_config(be_reg, &reg_cfg_info->alg_reg_cfg[i].ge_reg_cfg.static_reg_cfg);

        /* usr */
        usr_reg_cfg = &reg_cfg_info->alg_reg_cfg[i].ge_reg_cfg.usr_reg_cfg;
        if (usr_reg_cfg->resh) {
            isp_ge_usr_reg_config(be_reg, usr_reg_cfg);

            usr_reg_cfg->resh = offline_mode;
        }

        /* dynamic */
        dyna_reg_cfg = &reg_cfg_info->alg_reg_cfg[i].ge_reg_cfg.dyna_reg_cfg;
        if (dyna_reg_cfg->resh) {
            isp_ge_dyna_reg_config(be_reg, dyna_reg_cfg);
            dyna_reg_cfg->resh = offline_mode;
        }

        reg_cfg_info->cfg_key.bit1_ge_cfg = offline_mode ? 1 : ((blk_num <= (i + 1)) ? 0 : 1);
    }
#endif

    return TD_SUCCESS;
}

static td_void isp_lsc_win_x_info_write(isp_be_reg_type *be_reg, isp_lsc_usr_cfg *usr_reg_cfg)
{
    isp_mlsc_x0_write(be_reg, usr_reg_cfg->delta_x[0], usr_reg_cfg->inv_x[0]); /* delta_x[0] */
    isp_mlsc_x1_write(be_reg, usr_reg_cfg->delta_x[1], usr_reg_cfg->inv_x[1]); /* delta_x[1] */
    isp_mlsc_x2_write(be_reg, usr_reg_cfg->delta_x[2], usr_reg_cfg->inv_x[2]); /* delta_x[2] */
    isp_mlsc_x3_write(be_reg, usr_reg_cfg->delta_x[3], usr_reg_cfg->inv_x[3]); /* delta_x[3] */
    isp_mlsc_x4_write(be_reg, usr_reg_cfg->delta_x[4], usr_reg_cfg->inv_x[4]); /* delta_x[4] */
    isp_mlsc_x5_write(be_reg, usr_reg_cfg->delta_x[5], usr_reg_cfg->inv_x[5]); /* delta_x[5] */
    isp_mlsc_x6_write(be_reg, usr_reg_cfg->delta_x[6], usr_reg_cfg->inv_x[6]); /* delta_x[6] */
    isp_mlsc_x7_write(be_reg, usr_reg_cfg->delta_x[7], usr_reg_cfg->inv_x[7]); /* delta_x[7] */
    isp_mlsc_x8_write(be_reg, usr_reg_cfg->delta_x[8], usr_reg_cfg->inv_x[8]); /* delta_x[8] */
    isp_mlsc_x9_write(be_reg, usr_reg_cfg->delta_x[9], usr_reg_cfg->inv_x[9]); /* delta_x[9] */
    isp_mlsc_x10_write(be_reg, usr_reg_cfg->delta_x[10], usr_reg_cfg->inv_x[10]); /* delta_x[10] */
    isp_mlsc_x11_write(be_reg, usr_reg_cfg->delta_x[11], usr_reg_cfg->inv_x[11]); /* delta_x[11] */
    isp_mlsc_x12_write(be_reg, usr_reg_cfg->delta_x[12], usr_reg_cfg->inv_x[12]); /* delta_x[12] */
    isp_mlsc_x13_write(be_reg, usr_reg_cfg->delta_x[13], usr_reg_cfg->inv_x[13]); /* delta_x[13] */
    isp_mlsc_x14_write(be_reg, usr_reg_cfg->delta_x[14], usr_reg_cfg->inv_x[14]); /* delta_x[14] */
    isp_mlsc_x15_write(be_reg, usr_reg_cfg->delta_x[15], usr_reg_cfg->inv_x[15]); /* delta_x[15] */
    isp_mlsc_x16_write(be_reg, usr_reg_cfg->delta_x[16], usr_reg_cfg->inv_x[16]); /* delta_x[16] */
    isp_mlsc_x17_write(be_reg, usr_reg_cfg->delta_x[17], usr_reg_cfg->inv_x[17]); /* delta_x[17] */
    isp_mlsc_x18_write(be_reg, usr_reg_cfg->delta_x[18], usr_reg_cfg->inv_x[18]); /* delta_x[18] */
    isp_mlsc_x19_write(be_reg, usr_reg_cfg->delta_x[19], usr_reg_cfg->inv_x[19]); /* delta_x[19] */
    isp_mlsc_x20_write(be_reg, usr_reg_cfg->delta_x[20], usr_reg_cfg->inv_x[20]); /* delta_x[20] */
    isp_mlsc_x21_write(be_reg, usr_reg_cfg->delta_x[21], usr_reg_cfg->inv_x[21]); /* delta_x[21] */
    isp_mlsc_x22_write(be_reg, usr_reg_cfg->delta_x[22], usr_reg_cfg->inv_x[22]); /* delta_x[22] */
    isp_mlsc_x23_write(be_reg, usr_reg_cfg->delta_x[23], usr_reg_cfg->inv_x[23]); /* delta_x[23] */
    isp_mlsc_x24_write(be_reg, usr_reg_cfg->delta_x[24], usr_reg_cfg->inv_x[24]); /* delta_x[24] */
    isp_mlsc_x25_write(be_reg, usr_reg_cfg->delta_x[25], usr_reg_cfg->inv_x[25]); /* delta_x[25] */
    isp_mlsc_x26_write(be_reg, usr_reg_cfg->delta_x[26], usr_reg_cfg->inv_x[26]); /* delta_x[26] */
    isp_mlsc_x27_write(be_reg, usr_reg_cfg->delta_x[27], usr_reg_cfg->inv_x[27]); /* delta_x[27] */
    isp_mlsc_x28_write(be_reg, usr_reg_cfg->delta_x[28], usr_reg_cfg->inv_x[28]); /* delta_x[28] */
    isp_mlsc_x29_write(be_reg, usr_reg_cfg->delta_x[29], usr_reg_cfg->inv_x[29]); /* delta_x[29] */
    isp_mlsc_x30_write(be_reg, usr_reg_cfg->delta_x[30], usr_reg_cfg->inv_x[30]); /* delta_x[30] */
    isp_mlsc_x31_write(be_reg, usr_reg_cfg->delta_x[31], usr_reg_cfg->inv_x[31]); /* delta_x[31] */
}

static td_void isp_lsc_win_y_info_write(isp_be_reg_type *be_reg, isp_lsc_usr_cfg *usr_reg_cfg)
{
    isp_mlsc_deltay0_write(be_reg, usr_reg_cfg->delta_y[0]); /* delta_y[0] */
    isp_mlsc_deltay1_write(be_reg, usr_reg_cfg->delta_y[1]); /* delta_y[1] */
    isp_mlsc_deltay2_write(be_reg, usr_reg_cfg->delta_y[2]); /* delta_y[2] */
    isp_mlsc_deltay3_write(be_reg, usr_reg_cfg->delta_y[3]); /* delta_y[3] */
    isp_mlsc_deltay4_write(be_reg, usr_reg_cfg->delta_y[4]); /* delta_y[4] */
    isp_mlsc_deltay5_write(be_reg, usr_reg_cfg->delta_y[5]); /* delta_y[5] */
    isp_mlsc_deltay6_write(be_reg, usr_reg_cfg->delta_y[6]); /* delta_y[6] */
    isp_mlsc_deltay7_write(be_reg, usr_reg_cfg->delta_y[7]); /* delta_y[7] */
    isp_mlsc_deltay8_write(be_reg, usr_reg_cfg->delta_y[8]); /* delta_y[8] */

    isp_mlsc_invy0_write(be_reg, usr_reg_cfg->inv_y[0]); /* inv_y[0] */
    isp_mlsc_invy1_write(be_reg, usr_reg_cfg->inv_y[1]); /* inv_y[1] */
    isp_mlsc_invy2_write(be_reg, usr_reg_cfg->inv_y[2]); /* inv_y[2] */
    isp_mlsc_invy3_write(be_reg, usr_reg_cfg->inv_y[3]); /* inv_y[3] */
    isp_mlsc_invy4_write(be_reg, usr_reg_cfg->inv_y[4]); /* inv_y[4] */
    isp_mlsc_invy5_write(be_reg, usr_reg_cfg->inv_y[5]); /* inv_y[5] */
    isp_mlsc_invy6_write(be_reg, usr_reg_cfg->inv_y[6]); /* inv_y[6] */
    isp_mlsc_invy7_write(be_reg, usr_reg_cfg->inv_y[7]); /* inv_y[7] */
    isp_mlsc_invy8_write(be_reg, usr_reg_cfg->inv_y[8]); /* inv_y[8] */
}

static td_s32 isp_lsc_usr_reg_config(td_bool *stt2_lut_regnew, ot_vi_pipe vi_pipe, const isp_reg_cfg *reg_cfg_info,
    td_u8 i, isp_lsc_usr_cfg *usr_reg_cfg)
{
    td_u8 buf_id;
    td_s32 ret;
    isp_usr_ctx *isp_ctx = TD_NULL;
    isp_be_lut_wstt_type *be_lut_stt_reg = TD_NULL;
    isp_viproc_reg_type *post_viproc = TD_NULL;
    isp_be_reg_type *be_reg = TD_NULL;

    isp_get_ctx(vi_pipe, isp_ctx);

    be_reg = (isp_be_reg_type *)isp_get_post_be_vir_addr(vi_pipe, i);
    post_viproc = (isp_viproc_reg_type *)isp_get_post_vi_proc_vir_addr(vi_pipe, i);
    isp_check_pointer_return(post_viproc);
    isp_check_pointer_return(be_reg);

    isp_mlsc_width_offset_write(be_reg, usr_reg_cfg->width_offset);

    isp_lsc_win_y_info_write(be_reg, usr_reg_cfg);
    isp_lsc_win_x_info_write(be_reg, usr_reg_cfg);

    if (is_online_mode(isp_ctx->block_attr.running_mode)) {
        if (reg_cfg_info->alg_reg_cfg[i].lsc_reg_cfg.lut2_stt_en == TD_TRUE) { /* online lut2stt regconfig */
            buf_id = usr_reg_cfg->buf_id;

            be_lut_stt_reg = (isp_be_lut_wstt_type *)isp_get_be_lut2stt_vir_addr(vi_pipe, i, buf_id);
            isp_check_pointer_return(be_lut_stt_reg);

            isp_mlsc_lut_wstt_write(be_lut_stt_reg, usr_reg_cfg->r_gain, usr_reg_cfg->gr_gain,
                usr_reg_cfg->gb_gain, usr_reg_cfg->b_gain);
            ret = isp_lsc_lut_wstt_addr_write(vi_pipe, i, buf_id, post_viproc);
            if (ret != TD_SUCCESS) {
                isp_err_trace("isp[%d]_lsc_lut_wstt_addr_write failed\n", vi_pipe);
                return ret;
            }

            isp_mlsc_stt2lut_en_write(be_reg, TD_TRUE);

            usr_reg_cfg->buf_id = 1 - buf_id;
            *stt2_lut_regnew = TD_TRUE;
        }
    } else {
        isp_mlsc_lut_wstt_write(&be_reg->be_lut.be_lut2stt, usr_reg_cfg->r_gain, usr_reg_cfg->gr_gain,
            usr_reg_cfg->gb_gain, usr_reg_cfg->b_gain);
        isp_mlsc_stt2lut_en_write(be_reg, TD_TRUE);
        isp_mlsc_stt2lut_regnew_write(be_reg, TD_TRUE);
    }

    return TD_SUCCESS;
}

static td_void isp_lsc_static_reg_config(isp_post_be_reg_type *be_reg, isp_viproc_reg_type *post_viproc,
    isp_lsc_static_cfg *static_reg_cfg)
{
    if (static_reg_cfg->static_resh) {
        isp_mlsc_numh_write(be_reg, static_reg_cfg->win_num_h);
        isp_mlsc_numv_write(be_reg, static_reg_cfg->win_num_v);
        isp_lsc_lut_width_word_write(post_viproc, OT_ISP_LSC_LUT_WIDTH_WORD_DEFAULT);
        static_reg_cfg->static_resh = TD_FALSE;
    }
}

static td_void isp_lsc_usr_coef_reg_config(td_bool offline_mode, isp_post_be_reg_type *be_reg,
    isp_lsc_usr_cfg *usr_reg_cfg)
{
    if (usr_reg_cfg->coef_update) {
        isp_lsc_mesh_str_write(be_reg, usr_reg_cfg->mesh_str);
        isp_mlsc_mesh_str_write(be_reg, usr_reg_cfg->mesh_str);
        usr_reg_cfg->coef_update = offline_mode;
    }
}

static td_void isp_lsc_online_lut2stt_info_check(td_bool offline_mode, td_bool *stt2_lut_regnew,
    isp_post_be_reg_type *be_reg)
{
    td_u16 lut2stt_info;
    if (offline_mode == TD_TRUE) {
        return;
    }

    lut2stt_info = isp_mlsc_stt2lut_info_read(be_reg);
    if (lut2stt_info != 0) {
        *stt2_lut_regnew = TD_TRUE;
        isp_mlsc_stt2lut_clr_write(be_reg, 1);
    }
}

static td_s32 isp_lsc_reg_config(ot_vi_pipe vi_pipe, isp_reg_cfg *reg_cfg_info, td_u8 i)
{
    td_bool offline_mode, usr_resh, idx_resh;
    td_bool lut_update = TD_FALSE;
    td_bool stt2_lut_regnew = TD_FALSE;
    td_u8 blk_num = reg_cfg_info->cfg_num;
    td_s32 ret;
    isp_lsc_usr_cfg *usr_reg_cfg = TD_NULL;
    isp_viproc_reg_type *post_viproc = TD_NULL;
    isp_post_be_reg_type *be_reg = TD_NULL;
    isp_usr_ctx *isp_ctx = TD_NULL;

    isp_get_ctx(vi_pipe, isp_ctx);
    offline_mode =
        (is_offline_mode(isp_ctx->block_attr.running_mode) || is_striping_mode(isp_ctx->block_attr.running_mode));

    if (reg_cfg_info->cfg_key.bit1_lsc_cfg) {
        be_reg = (isp_post_be_reg_type *)isp_get_post_be_vir_addr(vi_pipe, i);
        post_viproc = (isp_viproc_reg_type *)isp_get_post_vi_proc_vir_addr(vi_pipe, i);
        isp_check_pointer_return(be_reg);
        isp_check_pointer_return(post_viproc);
        usr_reg_cfg = &reg_cfg_info->alg_reg_cfg[i].lsc_reg_cfg.usr_reg_cfg;

        isp_lsc_static_reg_config(be_reg, post_viproc, &reg_cfg_info->alg_reg_cfg[i].lsc_reg_cfg.static_reg_cfg);
        isp_lsc_usr_coef_reg_config(offline_mode, be_reg, usr_reg_cfg);

        idx_resh = (isp_lsc_update_index_read(be_reg) != usr_reg_cfg->update_index);
        usr_resh = (offline_mode) ? (usr_reg_cfg->lut_update && idx_resh) : (usr_reg_cfg->lut_update);

        if (usr_resh) {
            isp_lsc_update_index_write(be_reg, usr_reg_cfg->update_index);

            ret = isp_lsc_usr_reg_config(&stt2_lut_regnew, vi_pipe, reg_cfg_info, i, usr_reg_cfg);
            if (ret != TD_SUCCESS) {
                return ret;
            }

            lut_update = TD_TRUE;

            usr_reg_cfg->lut_update = offline_mode;
        }
        isp_lsc_online_lut2stt_info_check(offline_mode, &stt2_lut_regnew, be_reg);
        reg_cfg_info->cfg_key.bit1_lsc_cfg = offline_mode ? 1 : ((blk_num <= (i + 1)) ? 0 : 1);
    }

    reg_cfg_info->alg_reg_cfg[i].be_lut_update_cfg.lsc_lut_update = lut_update;
    reg_cfg_info->alg_reg_cfg[i].stt2lut_regnew_cfg.lsc_stt2lut_regnew = stt2_lut_regnew;

    if (reg_cfg_info->alg_reg_cfg[i].lsc_reg_cfg.usr_reg_cfg.switch_lut2_stt_reg_new == TD_TRUE) {
        if (reg_cfg_info->alg_reg_cfg[i].lsc_reg_cfg.usr_reg_cfg.switch_reg_new_cnt < 3) { /* lsc reg config conunt 3 */
            reg_cfg_info->alg_reg_cfg[i].stt2lut_regnew_cfg.lsc_stt2lut_regnew = TD_TRUE;
            reg_cfg_info->alg_reg_cfg[i].lsc_reg_cfg.usr_reg_cfg.switch_reg_new_cnt++;
        } else {
            reg_cfg_info->alg_reg_cfg[i].lsc_reg_cfg.usr_reg_cfg.switch_lut2_stt_reg_new = TD_FALSE;
            reg_cfg_info->alg_reg_cfg[i].lsc_reg_cfg.usr_reg_cfg.switch_reg_new_cnt = 0;
        }
    }

    return TD_SUCCESS;
}

static td_s32 isp_gamma_lut_reg_config(td_bool *stt2_lut_regnew, ot_vi_pipe vi_pipe, td_u8 i,
    isp_post_be_reg_type *be_reg, isp_gamma_usr_cfg *usr_reg_cfg)
{
    td_u8 buf_id;
    td_s32 ret;
    isp_be_lut_wstt_type *be_lut_stt_reg = TD_NULL;
    isp_usr_ctx *isp_ctx = TD_NULL;
    isp_viproc_reg_type *post_viproc = TD_NULL;

    isp_get_ctx(vi_pipe, isp_ctx);

    post_viproc = (isp_viproc_reg_type *)isp_get_post_vi_proc_vir_addr(vi_pipe, i);
    isp_check_pointer_return(post_viproc);

    isp_gamma_lut_width_word_write(post_viproc, OT_ISP_GAMMA_LUT_WIDTH_WORD_DEFAULT);

    if (is_online_mode(isp_ctx->block_attr.running_mode)) {
        if (usr_reg_cfg->switch_mode != TD_TRUE) {
            buf_id = usr_reg_cfg->buf_id;

            be_lut_stt_reg = (isp_be_lut_wstt_type *)isp_get_be_lut2stt_vir_addr(vi_pipe, i, buf_id);
            isp_check_pointer_return(be_lut_stt_reg);

            isp_gamma_lut_wstt_write(be_lut_stt_reg, usr_reg_cfg->gamma_lut);
            ret = isp_gamma_lut_wstt_addr_write(vi_pipe, i, buf_id, post_viproc);
            if (ret != TD_SUCCESS) {
                isp_err_trace("isp[%d]_gamma_lut_wstt_addr_write failed\n", vi_pipe);
                return ret;
            }
            isp_gamma_stt2lut_en_write(be_reg, TD_TRUE);
            usr_reg_cfg->buf_id = 1 - buf_id;
            *stt2_lut_regnew = TD_TRUE;
        }
    } else {
        isp_gamma_lut_wstt_write(&be_reg->be_lut.be_lut2stt, usr_reg_cfg->gamma_lut);
        isp_gamma_stt2lut_en_write(be_reg, TD_TRUE);
        isp_gamma_stt2lut_regnew_write(be_reg, TD_TRUE);
    }

    return TD_SUCCESS;
}

static td_void isp_gamma_inseg_write(isp_post_be_reg_type *be_reg, td_u16 *gamma_in_seg)
{
    isp_gamma_inseg_0_write(be_reg, gamma_in_seg[0]); /* index[0] */
    isp_gamma_inseg_1_write(be_reg, gamma_in_seg[1]); /* index[1] */
    isp_gamma_inseg_2_write(be_reg, gamma_in_seg[2]); /* index[2] */
    isp_gamma_inseg_3_write(be_reg, gamma_in_seg[3]); /* index[3] */
    isp_gamma_inseg_4_write(be_reg, gamma_in_seg[4]); /* index[4] */
    isp_gamma_inseg_5_write(be_reg, gamma_in_seg[5]); /* index[5] */
    isp_gamma_inseg_6_write(be_reg, gamma_in_seg[6]); /* index[6] */
    isp_gamma_inseg_7_write(be_reg, gamma_in_seg[7]); /* index[7] */
}

static td_void isp_gamma_pos_write(isp_post_be_reg_type *be_reg, td_u16 *gamma_pos)
{
    isp_gamma_pos_0_write(be_reg, gamma_pos[0]); /* index[0] */
    isp_gamma_pos_1_write(be_reg, gamma_pos[1]); /* index[1] */
    isp_gamma_pos_2_write(be_reg, gamma_pos[2]); /* index[2] */
    isp_gamma_pos_3_write(be_reg, gamma_pos[3]); /* index[3] */
    isp_gamma_pos_4_write(be_reg, gamma_pos[4]); /* index[4] */
    isp_gamma_pos_5_write(be_reg, gamma_pos[5]); /* index[5] */
    isp_gamma_pos_6_write(be_reg, gamma_pos[6]); /* index[6] */
    isp_gamma_pos_7_write(be_reg, gamma_pos[7]); /* index[7] */
}

static td_void isp_gamma_step_write(isp_post_be_reg_type *be_reg, td_u8 *gamma_step)
{
    isp_gamma_step0_write(be_reg, gamma_step[0]); /* index[0] */
    isp_gamma_step1_write(be_reg, gamma_step[1]); /* index[1] */
    isp_gamma_step2_write(be_reg, gamma_step[2]); /* index[2] */
    isp_gamma_step3_write(be_reg, gamma_step[3]); /* index[3] */
    isp_gamma_step4_write(be_reg, gamma_step[4]); /* index[4] */
    isp_gamma_step5_write(be_reg, gamma_step[5]); /* index[5] */
    isp_gamma_step6_write(be_reg, gamma_step[6]); /* index[6] */
    isp_gamma_step7_write(be_reg, gamma_step[7]); /* index[7] */
}

static td_void isp_gamma_online_lut2stt_info_check(td_bool offline_mode, td_bool *stt2_lut_regnew,
    isp_post_be_reg_type *be_reg)
{
    td_u16 lut2stt_info;
    if (offline_mode == TD_TRUE) {
        return;
    }

    lut2stt_info = isp_gamma_stt2lut_info_read(be_reg);
    if (lut2stt_info != 0) {
        *stt2_lut_regnew = TD_TRUE;
        isp_gamma_stt2lut_clr_write(be_reg, 1);
    }
}

static td_s32 isp_gamma_reg_config(ot_vi_pipe vi_pipe, isp_reg_cfg *reg_cfg_info, td_u8 i)
{
    td_bool offline_mode, usr_resh, idx_resh;
    td_bool stt2_lut_regnew = TD_FALSE;
    td_u8 blk_num = reg_cfg_info->cfg_num;
    td_s32 ret;
    isp_post_be_reg_type *be_reg = TD_NULL;
    isp_gamma_usr_cfg *usr_reg_cfg = TD_NULL;
    isp_usr_ctx *isp_ctx = TD_NULL;

    isp_get_ctx(vi_pipe, isp_ctx);

    offline_mode =
        (is_offline_mode(isp_ctx->block_attr.running_mode) || is_striping_mode(isp_ctx->block_attr.running_mode) ||
        is_pre_online_post_offline(isp_ctx->block_attr.running_mode));

    usr_reg_cfg = &reg_cfg_info->alg_reg_cfg[i].gamma_cfg.usr_reg_cfg;

    if (reg_cfg_info->cfg_key.bit1_gamma_cfg) {
        be_reg = (isp_post_be_reg_type *)isp_get_post_be_vir_addr(vi_pipe, i);
        isp_check_pointer_return(be_reg);

        idx_resh = (isp_gamma_update_index_read(be_reg) != usr_reg_cfg->update_index);
        usr_resh = (offline_mode) ? (usr_reg_cfg->gamma_lut_update_en && idx_resh) : (usr_reg_cfg->gamma_lut_update_en);

        if (usr_resh) {
            isp_gamma_update_index_write(be_reg, usr_reg_cfg->update_index);

            ret = isp_gamma_lut_reg_config(&stt2_lut_regnew, vi_pipe, i, be_reg, usr_reg_cfg);
            if (ret != TD_SUCCESS) {
                return ret;
            }

            isp_gamma_inseg_write(be_reg, usr_reg_cfg->gamma_in_seg);
            isp_gamma_pos_write(be_reg, usr_reg_cfg->gamma_pos);
            isp_gamma_step_write(be_reg, usr_reg_cfg->gamma_step);

            usr_reg_cfg->gamma_lut_update_en = offline_mode;
            usr_reg_cfg->switch_mode = TD_FALSE;
        }
        isp_gamma_online_lut2stt_info_check(offline_mode, &stt2_lut_regnew, be_reg);
        reg_cfg_info->cfg_key.bit1_gamma_cfg = offline_mode ? 1 : ((blk_num <= (i + 1)) ? 0 : 1);
    }

    reg_cfg_info->alg_reg_cfg[i].stt2lut_regnew_cfg.gamma_stt2lut_regnew = stt2_lut_regnew;

    return TD_SUCCESS;
}
#ifdef CONFIG_OT_AIBNR_SUPPORT
static td_s32 isp_adgamma_lut_reg_config(const isp_usr_ctx *isp_ctx, isp_pre_be_reg_type *be_reg,
    isp_adgamma_dyna_cfg *dyna_reg_cfg)
{
    td_u16 j;

    if (is_online_mode(isp_ctx->block_attr.running_mode) ||
        is_pre_online_post_offline(isp_ctx->block_attr.running_mode)) {
        isp_expander_lut_waddr_write(be_reg, 0);

        for (j = 0; j < ISP_AGAMMA_NODE_NUM; j++) {
            isp_expander_lut_wdata_write(be_reg, dyna_reg_cfg->adgamma_lut[j]);
        }
    } else {
        isp_adgamma_lut_write(&be_reg->be_lut.be_apb_lut, dyna_reg_cfg->adgamma_lut);
    }

    return TD_SUCCESS;
}


static td_s32 isp_adgamma_reg_config(ot_vi_pipe vi_pipe, isp_reg_cfg *reg_cfg_info, td_u8 i)
{
    td_u8 blk_num = reg_cfg_info->cfg_num;
    isp_pre_be_reg_type *be_reg = TD_NULL;
    isp_adgamma_dyna_cfg *dyna_reg_cfg = TD_NULL;
    isp_adgamma_static_cfg *static_reg_cfg = TD_NULL;
    isp_usr_ctx *isp_ctx = TD_NULL;
    isp_get_ctx(vi_pipe, isp_ctx);
    td_bool offline_mode;

    offline_mode = (is_offline_mode(isp_ctx->block_attr.running_mode) ||
                    is_striping_mode(isp_ctx->block_attr.running_mode));

    if (reg_cfg_info->cfg_key.bit1_adgamma_cfg) {
        be_reg = (isp_pre_be_reg_type *)isp_get_pre_be_vir_addr(vi_pipe, i);
        isp_check_pointer_return(be_reg);

        /* adgamma enable updated by kernel for sync */
        reg_cfg_info->alg_reg_cfg[i].expander_cfg.enable = reg_cfg_info->alg_reg_cfg[i].adgamma_reg_cfg.enable;

        dyna_reg_cfg = &reg_cfg_info->alg_reg_cfg[i].adgamma_reg_cfg.dyna_reg_cfg;
        static_reg_cfg = &reg_cfg_info->alg_reg_cfg[i].adgamma_reg_cfg.static_reg_cfg;

        if (static_reg_cfg->resh) {
            isp_expander_bitw_out_write(be_reg, static_reg_cfg->bitw);
            isp_expander_bitw_in_write(be_reg, static_reg_cfg->bitw);
            static_reg_cfg->resh = TD_FALSE;
        }

        if (dyna_reg_cfg->resh) {
            isp_adgamma_lut_reg_config(isp_ctx, be_reg, dyna_reg_cfg);
            dyna_reg_cfg->resh = offline_mode;
        }

        reg_cfg_info->cfg_key.bit1_adgamma_cfg = offline_mode ? 1 : ((blk_num <= (i + 1)) ? 0 : 1);
    }

    return TD_SUCCESS;
}
#endif
static td_void isp_csp_lut_apb_config(isp_be_reg_type *be_reg, isp_drc_usr_cfg *usr_reg_cfg)
{
    td_s32 i;

    isp_cc_dgain_lut_waddr_write(be_reg, 0);
    isp_cc_gamma_lut_waddr_write(be_reg, 0);

    for (i = 0; i < DRC_CSP_LUT_NODE_NUM; i++) {
        isp_cc_dgain_lut_wdata_write(be_reg, usr_reg_cfg->csp_lutb_lut[i] & 0x3FFF);
        isp_cc_gamma_lut_wdata_write(be_reg, usr_reg_cfg->csp_lutc_lut[i] & 0xFFF);
    }
}

static td_s32 isp_csp_lut_reg_config(td_bool *stt2_lut_regnew, ot_vi_pipe vi_pipe, td_u8 i,
    isp_post_be_reg_type *be_reg, isp_drc_usr_cfg *usr_reg_cfg)
{
    td_u8 buf_id;
    td_s32 ret;
    isp_be_lut_wstt_type *be_lut_stt_reg = TD_NULL;
    isp_usr_ctx *isp_ctx = TD_NULL;
    isp_viproc_reg_type *post_viproc = TD_NULL;

    isp_get_ctx(vi_pipe, isp_ctx);

    post_viproc = (isp_viproc_reg_type *)isp_get_post_vi_proc_vir_addr(vi_pipe, i);
    isp_check_pointer_return(post_viproc);

    isp_cc_lut_width_word_write(post_viproc, OT_ISP_CC_LUT_WIDTH_WORD_DEFAULT);

    if (is_online_mode(isp_ctx->block_attr.running_mode)) {
        buf_id = usr_reg_cfg->buf_id;

        be_lut_stt_reg = (isp_be_lut_wstt_type *)isp_get_be_lut2stt_vir_addr(vi_pipe, i, buf_id);
        isp_check_pointer_return(be_lut_stt_reg);

        isp_cc_lut_wstt_write(be_lut_stt_reg, usr_reg_cfg->csp_lutb_lut,
                              usr_reg_cfg->csp_lutc_lut);
        ret = isp_cc_lut_wstt_addr_write(vi_pipe, i, buf_id, post_viproc);
        if (ret != TD_SUCCESS) {
            isp_err_trace("isp[%d]_csp_lut_wstt_addr_write failed\n", vi_pipe);
            return ret;
        }
        isp_cc_stt2lut_en_write(be_reg, TD_TRUE);
        usr_reg_cfg->buf_id = 1 - buf_id;
        *stt2_lut_regnew = TD_TRUE;
    } else {
        isp_cc_lut_wstt_write(&be_reg->be_lut.be_lut2stt, usr_reg_cfg->csp_lutb_lut,
                              usr_reg_cfg->csp_lutc_lut);
        isp_cc_stt2lut_en_write(be_reg, TD_TRUE);
        isp_cc_stt2lut_regnew_write(be_reg, TD_TRUE);
    }

    return TD_SUCCESS;
}

static td_void isp_csp_lutb_inseg_write(isp_post_be_reg_type *be_reg, td_u16 *csp_in_seg)
{
    isp_cc_dgain_inseg_0_write(be_reg, csp_in_seg[0]); /* index[0] */
    isp_cc_dgain_inseg_1_write(be_reg, csp_in_seg[1]); /* index[1] */
    isp_cc_dgain_inseg_2_write(be_reg, csp_in_seg[2]); /* index[2] */
    isp_cc_dgain_inseg_3_write(be_reg, csp_in_seg[3]); /* index[3] */
    isp_cc_dgain_inseg_4_write(be_reg, csp_in_seg[4]); /* index[4] */
    isp_cc_dgain_inseg_5_write(be_reg, csp_in_seg[5]); /* index[5] */
    isp_cc_dgain_inseg_6_write(be_reg, csp_in_seg[6]); /* index[6] */
    isp_cc_dgain_inseg_7_write(be_reg, csp_in_seg[7]); /* index[7] */
}

static td_void isp_csp_lutb_pos_write(isp_post_be_reg_type *be_reg, td_u16 *csp_pos)
{
    isp_cc_dgain_pos_0_write(be_reg, csp_pos[0]); /* index[0] */
    isp_cc_dgain_pos_1_write(be_reg, csp_pos[1]); /* index[1] */
    isp_cc_dgain_pos_2_write(be_reg, csp_pos[2]); /* index[2] */
    isp_cc_dgain_pos_3_write(be_reg, csp_pos[3]); /* index[3] */
    isp_cc_dgain_pos_4_write(be_reg, csp_pos[4]); /* index[4] */
    isp_cc_dgain_pos_5_write(be_reg, csp_pos[5]); /* index[5] */
    isp_cc_dgain_pos_6_write(be_reg, csp_pos[6]); /* index[6] */
    isp_cc_dgain_pos_7_write(be_reg, csp_pos[7]); /* index[7] */
}

static td_void isp_csp_lutb_step_write(isp_post_be_reg_type *be_reg, td_u8 *csp_step)
{
    isp_cc_dgain_step0_write(be_reg, csp_step[0]); /* index[0] */
    isp_cc_dgain_step1_write(be_reg, csp_step[1]); /* index[1] */
    isp_cc_dgain_step2_write(be_reg, csp_step[2]); /* index[2] */
    isp_cc_dgain_step3_write(be_reg, csp_step[3]); /* index[3] */
    isp_cc_dgain_step4_write(be_reg, csp_step[4]); /* index[4] */
    isp_cc_dgain_step5_write(be_reg, csp_step[5]); /* index[5] */
    isp_cc_dgain_step6_write(be_reg, csp_step[6]); /* index[6] */
    isp_cc_dgain_step7_write(be_reg, csp_step[7]); /* index[7] */
}

static td_void isp_csp_lutc_inseg_write(isp_post_be_reg_type *be_reg, td_u16 *csp_in_seg)
{
    isp_cc_gamma_inseg_0_write(be_reg, csp_in_seg[0]); /* index[0] */
    isp_cc_gamma_inseg_1_write(be_reg, csp_in_seg[1]); /* index[1] */
    isp_cc_gamma_inseg_2_write(be_reg, csp_in_seg[2]); /* index[2] */
    isp_cc_gamma_inseg_3_write(be_reg, csp_in_seg[3]); /* index[3] */
    isp_cc_gamma_inseg_4_write(be_reg, csp_in_seg[4]); /* index[4] */
    isp_cc_gamma_inseg_5_write(be_reg, csp_in_seg[5]); /* index[5] */
    isp_cc_gamma_inseg_6_write(be_reg, csp_in_seg[6]); /* index[6] */
    isp_cc_gamma_inseg_7_write(be_reg, csp_in_seg[7]); /* index[7] */
}

static td_void isp_csp_lutc_pos_write(isp_post_be_reg_type *be_reg, td_u16 *csp_pos)
{
    isp_cc_gamma_pos_0_write(be_reg, csp_pos[0]); /* index[0] */
    isp_cc_gamma_pos_1_write(be_reg, csp_pos[1]); /* index[1] */
    isp_cc_gamma_pos_2_write(be_reg, csp_pos[2]); /* index[2] */
    isp_cc_gamma_pos_3_write(be_reg, csp_pos[3]); /* index[3] */
    isp_cc_gamma_pos_4_write(be_reg, csp_pos[4]); /* index[4] */
    isp_cc_gamma_pos_5_write(be_reg, csp_pos[5]); /* index[5] */
    isp_cc_gamma_pos_6_write(be_reg, csp_pos[6]); /* index[6] */
    isp_cc_gamma_pos_7_write(be_reg, csp_pos[7]); /* index[7] */
}

static td_void isp_csp_lutc_step_write(isp_post_be_reg_type *be_reg, td_u8 *csp_step)
{
    isp_cc_gamma_step0_write(be_reg, csp_step[0]); /* index[0] */
    isp_cc_gamma_step1_write(be_reg, csp_step[1]); /* index[1] */
    isp_cc_gamma_step2_write(be_reg, csp_step[2]); /* index[2] */
    isp_cc_gamma_step3_write(be_reg, csp_step[3]); /* index[3] */
    isp_cc_gamma_step4_write(be_reg, csp_step[4]); /* index[4] */
    isp_cc_gamma_step5_write(be_reg, csp_step[5]); /* index[5] */
    isp_cc_gamma_step6_write(be_reg, csp_step[6]); /* index[6] */
    isp_cc_gamma_step7_write(be_reg, csp_step[7]); /* index[7] */
}

static td_void isp_csp_online_lut2stt_info_check(td_bool offline_mode, td_bool *stt2_lut_regnew,
    isp_post_be_reg_type *be_reg)
{
    td_u16 lut2stt_info;
    if (offline_mode == TD_TRUE) {
        return;
    }

    lut2stt_info = isp_cc_stt2lut_info_read(be_reg);
    if (lut2stt_info != 0) {
        *stt2_lut_regnew = TD_TRUE;
        isp_cc_stt2lut_clr_write(be_reg, 1);
    }
}
static td_s32 isp_csp_reg_config(ot_vi_pipe vi_pipe, isp_reg_cfg *reg_cfg_info, td_u8 i)
{
    td_bool offline_mode;
    td_bool usr_resh;
    td_bool idx_resh;
    td_bool stt2_lut_regnew = TD_FALSE;
    td_u8 blk_num = reg_cfg_info->cfg_num;
    td_s32 ret;
    isp_post_be_reg_type *be_reg = TD_NULL;
    isp_drc_usr_cfg *usr_reg_cfg = TD_NULL;
    isp_usr_ctx *isp_ctx = TD_NULL;

    isp_get_ctx(vi_pipe, isp_ctx);

    offline_mode =
        (is_offline_mode(isp_ctx->block_attr.running_mode) || is_striping_mode(isp_ctx->block_attr.running_mode) ||
        is_pre_online_post_offline(isp_ctx->block_attr.running_mode));

    usr_reg_cfg = &reg_cfg_info->alg_reg_cfg[i].drc_reg_cfg.usr_reg_cfg;
    be_reg = (isp_post_be_reg_type *)isp_get_post_be_vir_addr(vi_pipe, i);
    isp_check_pointer_return(be_reg);

    if (reg_cfg_info->cfg_key.bit1_csp_cfg) {
        idx_resh = (isp_csp_update_index_read(be_reg) != usr_reg_cfg->update_index);
        usr_resh = (offline_mode) ? (usr_reg_cfg->csp_lut_update_en && idx_resh) : (usr_reg_cfg->csp_lut_update_en);

        if (usr_resh) {
            isp_csp_update_index_write(be_reg, usr_reg_cfg->update_index);

            ret = isp_csp_lut_reg_config(&stt2_lut_regnew, vi_pipe, i, be_reg, usr_reg_cfg);
            if (ret != TD_SUCCESS) {
                return ret;
            }
            isp_cc_luma_wdr_coefr_write(be_reg, usr_reg_cfg->csp_coefr);
            isp_cc_luma_wdr_coefb_write(be_reg, usr_reg_cfg->csp_coefb);
            isp_csp_lutb_inseg_write(be_reg, usr_reg_cfg->csp_lutb_in_seg);
            isp_csp_lutb_pos_write(be_reg, usr_reg_cfg->csp_lutb_pos);
            isp_csp_lutb_step_write(be_reg, usr_reg_cfg->csp_lutb_step);
            isp_csp_lutc_inseg_write(be_reg, usr_reg_cfg->csp_lutc_in_seg);
            isp_csp_lutc_pos_write(be_reg, usr_reg_cfg->csp_lutc_pos);
            isp_csp_lutc_step_write(be_reg, usr_reg_cfg->csp_lutc_step);
            usr_reg_cfg->csp_lut_update_en = offline_mode;
            usr_reg_cfg->switch_mode = TD_FALSE;
        }

        reg_cfg_info->cfg_key.bit1_csp_cfg = offline_mode ? 1 : ((blk_num <= (i + 1)) ? 0 : 1);
    }
    isp_csp_online_lut2stt_info_check(offline_mode, &stt2_lut_regnew, be_reg);
    reg_cfg_info->alg_reg_cfg[i].stt2lut_regnew_cfg.csp_stt2lut_regnew = stt2_lut_regnew;

    return TD_SUCCESS;
}

static td_s32 isp_csc_reg_config(ot_vi_pipe vi_pipe, isp_reg_cfg *reg_cfg_info, td_u8 i)
{
    td_u8 blk_num = reg_cfg_info->cfg_num;
    td_bool offline_mode;
    isp_csc_dyna_cfg *dyna_reg_cfg = TD_NULL;
    isp_post_be_reg_type *be_reg = TD_NULL;
    isp_usr_ctx *isp_ctx = TD_NULL;

    isp_get_ctx(vi_pipe, isp_ctx);
    offline_mode =
        (is_offline_mode(isp_ctx->block_attr.running_mode) || is_striping_mode(isp_ctx->block_attr.running_mode) ||
        is_pre_online_post_offline(isp_ctx->block_attr.running_mode));

    if (reg_cfg_info->cfg_key.bit1_csc_cfg) {
        be_reg = (isp_post_be_reg_type *)isp_get_post_be_vir_addr(vi_pipe, i);
        isp_check_pointer_return(be_reg);
        dyna_reg_cfg = &reg_cfg_info->alg_reg_cfg[i].csc_cfg.dyna_reg_cfg;

        /* dynamic */
        if (dyna_reg_cfg->resh) {
            isp_csc_coef00_write(be_reg, dyna_reg_cfg->csc_coef[0]);  /* coef0 */
            isp_csc_coef01_write(be_reg, dyna_reg_cfg->csc_coef[1]);  /* coef1 */
            isp_csc_coef02_write(be_reg, dyna_reg_cfg->csc_coef[2]);  /* coef2 */
            isp_csc_coef10_write(be_reg, dyna_reg_cfg->csc_coef[3]);  /* coef3 */
            isp_csc_coef11_write(be_reg, dyna_reg_cfg->csc_coef[4]);  /* coef4 */
            isp_csc_coef12_write(be_reg, dyna_reg_cfg->csc_coef[5]);  /* coef5 */
            isp_csc_coef20_write(be_reg, dyna_reg_cfg->csc_coef[6]);  /* coef6 */
            isp_csc_coef21_write(be_reg, dyna_reg_cfg->csc_coef[7]);  /* coef7 */
            isp_csc_coef22_write(be_reg, dyna_reg_cfg->csc_coef[8]);  /* coef8 */
            isp_csc_in_dc0_write(be_reg, dyna_reg_cfg->csc_in_dc[0]); /* in_dc0 */
            isp_csc_in_dc1_write(be_reg, dyna_reg_cfg->csc_in_dc[1]); /* in_dc1 */
            isp_csc_in_dc2_write(be_reg, dyna_reg_cfg->csc_in_dc[2]); /* in_dc2 */

            isp_csc_out_dc0_write(be_reg, dyna_reg_cfg->csc_out_dc[0]); /* out_dc0 */
            isp_csc_out_dc1_write(be_reg, dyna_reg_cfg->csc_out_dc[1]); /* out_dc1 */
            isp_csc_out_dc2_write(be_reg, dyna_reg_cfg->csc_out_dc[2]); /* out_dc2 */
            dyna_reg_cfg->resh = offline_mode;
        }

        reg_cfg_info->cfg_key.bit1_csc_cfg = offline_mode ? 1 : ((blk_num <= (i + 1)) ? 0 : 1);
    }

    return TD_SUCCESS;
}

#ifdef CONFIG_OT_ISP_CA_SUPPORT
static td_s32 isp_ca_lut_reg_config(td_bool *stt2_lut_regnew, ot_vi_pipe vi_pipe, td_u8 i,
    isp_post_be_reg_type *be_reg, isp_ca_usr_cfg *usr_reg_cfg)
{
    td_u8  buf_id;
    td_s32 ret;
    isp_usr_ctx *isp_ctx = TD_NULL;
    isp_viproc_reg_type *post_viproc = TD_NULL;
    isp_be_lut_wstt_type *be_lut_stt_reg = TD_NULL;

    isp_get_ctx(vi_pipe, isp_ctx);

    post_viproc = (isp_viproc_reg_type *)isp_get_post_vi_proc_vir_addr(vi_pipe, i);
    isp_check_pointer_return(post_viproc);
    isp_ca_lut_width_word_write(post_viproc, OT_ISP_CA_LUT_WIDTH_WORD_DEFAULT);
    if (is_online_mode(isp_ctx->block_attr.running_mode)) {
        buf_id = usr_reg_cfg->buf_id;
        be_lut_stt_reg = (isp_be_lut_wstt_type *)isp_get_be_lut2stt_vir_addr(vi_pipe, i, buf_id);
        isp_check_pointer_return(be_lut_stt_reg);

        isp_ca_y_ratio_lut_wstt_write(be_lut_stt_reg, usr_reg_cfg->y_ratio_lut);
        isp_ca_y_sat_lut_wstt_write(be_lut_stt_reg, usr_reg_cfg->y_sat_lut);
        isp_ca_y_ch1_lut_wstt_write(be_lut_stt_reg, usr_reg_cfg->y_ch1_lut);
        isp_ca_y_ch2_lut_wstt_write(be_lut_stt_reg, usr_reg_cfg->y_ch2_lut);
        isp_ca_y_slu_lut_wstt_write(be_lut_stt_reg, usr_reg_cfg->y_slu_lut);
        isp_ca_y_slv_lut_wstt_write(be_lut_stt_reg, usr_reg_cfg->y_slv_lut);

        ret = isp_ca_lut_wstt_addr_write(vi_pipe, i, buf_id, post_viproc);
        if (ret != TD_SUCCESS) {
            isp_err_trace("isp[%d]_ca_lut_wstt_addr_write failed\n", vi_pipe);
            return ret;
        }
        isp_ca_stt2lut_en_write(be_reg, TD_TRUE);

        usr_reg_cfg->buf_id = 1 - buf_id;
        *stt2_lut_regnew = TD_TRUE;
    } else {
        isp_ca_y_ratio_lut_wstt_write(&be_reg->be_lut.be_lut2stt, usr_reg_cfg->y_ratio_lut);
        isp_ca_y_sat_lut_wstt_write(&be_reg->be_lut.be_lut2stt, usr_reg_cfg->y_sat_lut);
        isp_ca_y_ch1_lut_wstt_write(&be_reg->be_lut.be_lut2stt, usr_reg_cfg->y_ch1_lut);
        isp_ca_y_ch2_lut_wstt_write(&be_reg->be_lut.be_lut2stt, usr_reg_cfg->y_ch2_lut);
        isp_ca_y_slu_lut_wstt_write(&be_reg->be_lut.be_lut2stt, usr_reg_cfg->y_slu_lut);
        isp_ca_y_slv_lut_wstt_write(&be_reg->be_lut.be_lut2stt, usr_reg_cfg->y_slv_lut);
        isp_ca_stt2lut_en_write(be_reg, TD_TRUE);
        isp_ca_stt2lut_regnew_write(be_reg, TD_TRUE);
    }

    return TD_SUCCESS;
}

static td_void isp_ca_static_reg_config(isp_post_be_reg_type *be_reg, isp_ca_static_cfg *static_reg_cfg)
{
    if (static_reg_cfg->static_resh) {
        isp_ca_llhcproc_en_write(be_reg, static_reg_cfg->ca_llhc_proc_en);
        isp_ca_skinproc_en_write(be_reg, static_reg_cfg->ca_skin_proc_en);
        isp_ca_satvssat_en_write(be_reg, static_reg_cfg->ca_satvssat_en);
        isp_ca_satadj_en_write(be_reg, static_reg_cfg->ca_satu_adj_en);
        isp_ca_des_point_write(be_reg, static_reg_cfg->ca_des_point);
        isp_ca_des_mix_write(be_reg, static_reg_cfg->ca_des_mix);
        isp_ca_sat_c2_write(be_reg, static_reg_cfg->ca_sat_c2);
        isp_ca_sat_c1_write(be_reg, static_reg_cfg->ca_sat_c1);
        static_reg_cfg->static_resh = TD_FALSE;
    }
}
static td_void isp_ca_online_lut2stt_info_check(td_bool offline_mode, td_bool *stt2_lut_regnew,
    isp_post_be_reg_type *be_reg)
{
    td_u16 lut2stt_info;
    if (offline_mode == TD_TRUE) {
        return;
    }

    lut2stt_info = isp_ca_stt2lut_info_read(be_reg);
    if (lut2stt_info != 0) {
        *stt2_lut_regnew = TD_TRUE;
        isp_ca_stt2lut_clr_write(be_reg, 1);
    }
}
#endif

static td_s32 isp_ca_reg_config(ot_vi_pipe vi_pipe, isp_reg_cfg *reg_cfg_info, td_u8 i)
{
#ifdef CONFIG_OT_ISP_CA_SUPPORT
    td_bool offline_mode, usr_resh, idx_resh;
    td_bool  stt2_lut_regnew = TD_FALSE;
    td_u8 blk_num = reg_cfg_info->cfg_num;
    td_s32 ret;
    isp_ca_usr_cfg *usr_reg_cfg = TD_NULL;
    isp_post_be_reg_type *be_reg = TD_NULL;
    isp_usr_ctx *isp_ctx = TD_NULL;
    isp_viproc_reg_type *post_viproc = TD_NULL;

    isp_get_ctx(vi_pipe, isp_ctx);

    offline_mode =
        (is_offline_mode(isp_ctx->block_attr.running_mode) || is_striping_mode(isp_ctx->block_attr.running_mode) ||
        is_pre_online_post_offline(isp_ctx->block_attr.running_mode));
    be_reg = (isp_post_be_reg_type *)isp_get_post_be_vir_addr(vi_pipe, i);
    isp_check_pointer_return(be_reg);
    if (reg_cfg_info->cfg_key.bit1_ca_cfg) {
        post_viproc = (isp_viproc_reg_type *)isp_get_post_vi_proc_vir_addr(vi_pipe, i);
        isp_check_pointer_return(post_viproc);

        /* usr */
        usr_reg_cfg = &reg_cfg_info->alg_reg_cfg[i].ca_reg_cfg.usr_reg_cfg;
        idx_resh = (isp_ca_update_index_read(be_reg) != usr_reg_cfg->update_index);
        usr_resh = (offline_mode) ? (usr_reg_cfg->resh && idx_resh) : (usr_reg_cfg->resh);

        if (usr_resh) {
            isp_ca_update_index_write(be_reg, usr_reg_cfg->update_index);
            ret = isp_ca_lut_reg_config(&stt2_lut_regnew, vi_pipe, i, be_reg, usr_reg_cfg);
            if (ret != TD_SUCCESS) {
                return ret;
            }
            usr_reg_cfg->resh = offline_mode;
        }

        /* static */
        isp_ca_static_reg_config(be_reg, &reg_cfg_info->alg_reg_cfg[i].ca_reg_cfg.static_reg_cfg);

        reg_cfg_info->cfg_key.bit1_ca_cfg = offline_mode ? 1 : ((blk_num <= (i + 1)) ? 0 : 1);
    }

    isp_ca_online_lut2stt_info_check(offline_mode, &stt2_lut_regnew, be_reg);

    reg_cfg_info->alg_reg_cfg[i].stt2lut_regnew_cfg.ca_stt2lut_regnew = stt2_lut_regnew;
#endif

    return TD_SUCCESS;
}

static td_s32 isp_mcds_reg_config(ot_vi_pipe vi_pipe, isp_reg_cfg *reg_cfg_info, td_u8 i)
{
    td_u8 blk_num = reg_cfg_info->cfg_num;
    td_bool offline_mode;
    isp_mcds_static_reg_cfg *static_reg_cfg = TD_NULL;
    isp_viproc_reg_type *post_viproc = TD_NULL;
    isp_post_be_reg_type *be_reg = TD_NULL;
    isp_usr_ctx *isp_ctx = TD_NULL;

    isp_get_ctx(vi_pipe, isp_ctx);
    offline_mode =
        (is_offline_mode(isp_ctx->block_attr.running_mode) || is_striping_mode(isp_ctx->block_attr.running_mode) ||
        is_pre_online_post_offline(isp_ctx->block_attr.running_mode));

    if (reg_cfg_info->cfg_key.bit1_mcds_cfg) {
        be_reg = (isp_post_be_reg_type *)isp_get_post_be_vir_addr(vi_pipe, i);
        post_viproc = (isp_viproc_reg_type *)isp_get_post_vi_proc_vir_addr(vi_pipe, i);
        isp_check_pointer_return(be_reg);
        isp_check_pointer_return(post_viproc);

        isp_hcds_en_write(post_viproc, reg_cfg_info->alg_reg_cfg[i].mcds_reg_cfg.hcds_en);

        static_reg_cfg = &reg_cfg_info->alg_reg_cfg[i].mcds_reg_cfg.static_reg_cfg;

        if (static_reg_cfg->static_resh) {
            isp_hcds_coefh0_write(be_reg, static_reg_cfg->h_coef[0]); /* coefh0 */
            isp_hcds_coefh1_write(be_reg, static_reg_cfg->h_coef[1]); /* coefh1 */
            isp_hcds_coefh2_write(be_reg, static_reg_cfg->h_coef[2]); /* coefh2 */
            isp_hcds_coefh3_write(be_reg, static_reg_cfg->h_coef[3]); /* coefh3 */
            isp_hcds_coefh4_write(be_reg, static_reg_cfg->h_coef[4]); /* coefh4 */
            isp_hcds_coefh5_write(be_reg, static_reg_cfg->h_coef[5]); /* coefh5 */
            isp_hcds_coefh6_write(be_reg, static_reg_cfg->h_coef[6]); /* coefh6 */
            isp_hcds_coefh7_write(be_reg, static_reg_cfg->h_coef[7]); /* coefh7 */
            isp_sharpen_vcds_filterv_write(be_reg, TD_TRUE);
            static_reg_cfg->static_resh = TD_FALSE;
        }

        reg_cfg_info->cfg_key.bit1_mcds_cfg = offline_mode ? 1 : ((blk_num <= (i + 1)) ? 0 : 1);
    }

    return TD_SUCCESS;
}

static td_void isp_wdr_static_reg_config_bcom(isp_pre_be_reg_type *be_reg, isp_fswdr_static_cfg *static_cfg)
{
    isp_bcom_lut_inseg0_write(be_reg, static_cfg->gamma_bcom_non_unit_seg_lut[0]); /* 0 */
    isp_bcom_lut_inseg1_write(be_reg, static_cfg->gamma_bcom_non_unit_seg_lut[1]); /* 1 */
    isp_bcom_lut_inseg2_write(be_reg, static_cfg->gamma_bcom_non_unit_seg_lut[2]); /* 2 */
    isp_bcom_lut_inseg3_write(be_reg, static_cfg->gamma_bcom_non_unit_seg_lut[3]); /* 3 */
    isp_bcom_lut_inseg4_write(be_reg, static_cfg->gamma_bcom_non_unit_seg_lut[4]); /* 4 */
    isp_bcom_lut_inseg5_write(be_reg, static_cfg->gamma_bcom_non_unit_seg_lut[5]); /* 5 */
    isp_bcom_lut_inseg6_write(be_reg, static_cfg->gamma_bcom_non_unit_seg_lut[6]); /* 6 */
    isp_bcom_lut_inseg7_write(be_reg, static_cfg->gamma_bcom_non_unit_seg_lut[7]); /* 7 */

    isp_bcom_lut_pos0_write(be_reg, static_cfg->gamma_bcom_non_unit_pos_lut[0]); /* 0 */
    isp_bcom_lut_pos1_write(be_reg, static_cfg->gamma_bcom_non_unit_pos_lut[1]); /* 1 */
    isp_bcom_lut_pos2_write(be_reg, static_cfg->gamma_bcom_non_unit_pos_lut[2]); /* 2 */
    isp_bcom_lut_pos3_write(be_reg, static_cfg->gamma_bcom_non_unit_pos_lut[3]); /* 3 */
    isp_bcom_lut_pos4_write(be_reg, static_cfg->gamma_bcom_non_unit_pos_lut[4]); /* 4 */
    isp_bcom_lut_pos5_write(be_reg, static_cfg->gamma_bcom_non_unit_pos_lut[5]); /* 5 */
    isp_bcom_lut_pos6_write(be_reg, static_cfg->gamma_bcom_non_unit_pos_lut[6]); /* 6 */
    isp_bcom_lut_pos7_write(be_reg, static_cfg->gamma_bcom_non_unit_pos_lut[7]); /* 7 */

    isp_bcom_lut_step0_write(be_reg, static_cfg->gamma_bcom_non_unit_step_lut[0]); /* 0 */
    isp_bcom_lut_step1_write(be_reg, static_cfg->gamma_bcom_non_unit_step_lut[1]); /* 1 */
    isp_bcom_lut_step2_write(be_reg, static_cfg->gamma_bcom_non_unit_step_lut[2]); /* 2 */
    isp_bcom_lut_step3_write(be_reg, static_cfg->gamma_bcom_non_unit_step_lut[3]); /* 3 */
    isp_bcom_lut_step4_write(be_reg, static_cfg->gamma_bcom_non_unit_step_lut[4]); /* 4 */
    isp_bcom_lut_step5_write(be_reg, static_cfg->gamma_bcom_non_unit_step_lut[5]); /* 5 */
    isp_bcom_lut_step6_write(be_reg, static_cfg->gamma_bcom_non_unit_step_lut[6]); /* 6 */
    isp_bcom_lut_step7_write(be_reg, static_cfg->gamma_bcom_non_unit_step_lut[7]); /* 7 */
}

static td_void isp_wdr_static_reg_config_bdec(isp_pre_be_reg_type *be_reg, isp_fswdr_static_cfg *static_reg_cfg)
{
    isp_bdec_lut_inseg0_write(be_reg, static_reg_cfg->gamma_bdec_non_unit_seg_lut[0]); /* 0 */
    isp_bdec_lut_inseg1_write(be_reg, static_reg_cfg->gamma_bdec_non_unit_seg_lut[1]); /* 1 */
    isp_bdec_lut_inseg2_write(be_reg, static_reg_cfg->gamma_bdec_non_unit_seg_lut[2]); /* 2 */
    isp_bdec_lut_inseg3_write(be_reg, static_reg_cfg->gamma_bdec_non_unit_seg_lut[3]); /* 3 */
    isp_bdec_lut_inseg4_write(be_reg, static_reg_cfg->gamma_bdec_non_unit_seg_lut[4]); /* 4 */
    isp_bdec_lut_inseg5_write(be_reg, static_reg_cfg->gamma_bdec_non_unit_seg_lut[5]); /* 5 */
    isp_bdec_lut_inseg6_write(be_reg, static_reg_cfg->gamma_bdec_non_unit_seg_lut[6]); /* 6 */
    isp_bdec_lut_inseg7_write(be_reg, static_reg_cfg->gamma_bdec_non_unit_seg_lut[7]); /* 7 */

    isp_bdec_lut_pos0_write(be_reg, static_reg_cfg->gamma_bdec_non_unit_pos_lut[0]); /* 0 */
    isp_bdec_lut_pos1_write(be_reg, static_reg_cfg->gamma_bdec_non_unit_pos_lut[1]); /* 1 */
    isp_bdec_lut_pos2_write(be_reg, static_reg_cfg->gamma_bdec_non_unit_pos_lut[2]); /* 2 */
    isp_bdec_lut_pos3_write(be_reg, static_reg_cfg->gamma_bdec_non_unit_pos_lut[3]); /* 3 */
    isp_bdec_lut_pos4_write(be_reg, static_reg_cfg->gamma_bdec_non_unit_pos_lut[4]); /* 4 */
    isp_bdec_lut_pos5_write(be_reg, static_reg_cfg->gamma_bdec_non_unit_pos_lut[5]); /* 5 */
    isp_bdec_lut_pos6_write(be_reg, static_reg_cfg->gamma_bdec_non_unit_pos_lut[6]); /* 6 */
    isp_bdec_lut_pos7_write(be_reg, static_reg_cfg->gamma_bdec_non_unit_pos_lut[7]); /* 7 */

    isp_bdec_lut_step0_write(be_reg, static_reg_cfg->gamma_bdec_non_unit_step_lut[0]); /* 0 */
    isp_bdec_lut_step1_write(be_reg, static_reg_cfg->gamma_bdec_non_unit_step_lut[1]); /* 1 */
    isp_bdec_lut_step2_write(be_reg, static_reg_cfg->gamma_bdec_non_unit_step_lut[2]); /* 2 */
    isp_bdec_lut_step3_write(be_reg, static_reg_cfg->gamma_bdec_non_unit_step_lut[3]); /* 3 */
    isp_bdec_lut_step4_write(be_reg, static_reg_cfg->gamma_bdec_non_unit_step_lut[4]); /* 4 */
    isp_bdec_lut_step5_write(be_reg, static_reg_cfg->gamma_bdec_non_unit_step_lut[5]); /* 5 */
    isp_bdec_lut_step6_write(be_reg, static_reg_cfg->gamma_bdec_non_unit_step_lut[6]); /* 6 */
    isp_bdec_lut_step7_write(be_reg, static_reg_cfg->gamma_bdec_non_unit_step_lut[7]); /* 7 */
}

static td_void isp_bcom_gamma_lut_apb_reg_config(isp_post_be_reg_type *be_reg,
    isp_fswdr_static_cfg *static_reg_cfg)
{
    td_u16 j;

    isp_bcom_gamma_lut_waddr_write(be_reg, 0);

    for (j = 0; j < BCOM_BDEC_NODE_NUMBER; j++) {
        isp_bcom_gamma_lut_wdata_write(be_reg, static_reg_cfg->gamma_bcom_non_unit_lut[j]);
    }
}

static td_void isp_bdec_gamma_lut_apb_reg_config(isp_post_be_reg_type *be_reg,
    isp_fswdr_static_cfg *static_reg_cfg)
{
    td_u16 j;

    isp_bdec_gamma_lut_waddr_write(be_reg, 0);

    for (j = 0; j < BCOM_BDEC_NODE_NUMBER; j++) {
        isp_bdec_gamma_lut_wdata_write(be_reg, static_reg_cfg->gamma_bdec_non_unit_lut[j]);
    }
}

static td_void isp_wdr_static_reg_config_first_frame(isp_pre_be_reg_type *be_reg, isp_fswdr_static_cfg *static_reg_cfg,
    isp_fswdr_dyna_cfg *dyna_reg_cfg, isp_fswdr_usr_cfg *usr_reg_cfg, td_bool offline_mode)
{
    td_u32 expo_value[4]; /* 4 means the number of  max frame for wdr mode */

    expo_value[0] = static_reg_cfg->expo_value_lut[0];                      /* array index 0  assignment */
    expo_value[1] = static_reg_cfg->expo_value_lut[1];                      /* array index 1  assignment */
    expo_value[2] = static_reg_cfg->expo_value_lut[2];                      /* array index 2  assignment */
    expo_value[3] = static_reg_cfg->expo_value_lut[3];                      /* array index 3  assignment */
    isp_wdr_expovalue0_write(be_reg, expo_value[0]);                        /* expovalue 0 */
    isp_wdr_expovalue1_write(be_reg, expo_value[1]);                        /* expovalue 1 */
    isp_wdr_exporratio0_write(be_reg, static_reg_cfg->expo_r_ratio_lut[0]); /* expo_r_ratio 0 */
    isp_wdr_mdt_en_write(be_reg, dyna_reg_cfg->wdr_mdt_en);
    isp_wdr_fusionmode_write(be_reg, usr_reg_cfg->fusion_mode);
    isp_wdr_maxratio_write(be_reg, static_reg_cfg->max_ratio);
    isp_wdr_short_thr_write(be_reg, dyna_reg_cfg->short_thr);
    isp_wdr_long_thr_write(be_reg, dyna_reg_cfg->long_thr);
    isp_wdr_wgt_slope_write(be_reg, dyna_reg_cfg->wgt_slope);

    isp_wdr_static_reg_config_bcom(be_reg, static_reg_cfg);
    isp_wdr_static_reg_config_bdec(be_reg, static_reg_cfg);

    if (offline_mode == TD_TRUE) {
        isp_bcom_gamma_lut_write(&be_reg->be_lut.be_apb_lut, static_reg_cfg->gamma_bcom_non_unit_lut);
        isp_bdec_gamma_lut_write(&be_reg->be_lut.be_apb_lut, static_reg_cfg->gamma_bdec_non_unit_lut);
    } else {
        /* bcom */
        isp_bcom_gamma_lut_apb_reg_config(be_reg, static_reg_cfg);

        /* bdec */
        isp_bdec_gamma_lut_apb_reg_config(be_reg, static_reg_cfg);
    }

    static_reg_cfg->first_frame = TD_FALSE;
}

static td_void isp_wdr_static_reg_config(isp_pre_be_reg_type *be_reg, isp_fswdr_static_cfg *static_reg_cfg)
{
    isp_wdr_expovalue0_write(be_reg, static_reg_cfg->expo_value_lut[0]);                        /* expovalue 0 */
    isp_wdr_expovalue1_write(be_reg, static_reg_cfg->expo_value_lut[1]);                        /* expovalue 1 */
    isp_wdr_maxratio_write(be_reg, static_reg_cfg->max_ratio);
    isp_wdr_exporratio0_write(be_reg, static_reg_cfg->expo_r_ratio_lut[0]);                     /* expo_r_ratio 0 */
    isp_wdr_grayscale_mode_write(be_reg, static_reg_cfg->gray_scale_mode);
    isp_wdr_bsaveblc_write(be_reg, static_reg_cfg->save_blc);
    isp_wdr_mask_similar_thr_write(be_reg, static_reg_cfg->mask_similar_thr);
    isp_wdr_mask_similar_cnt_write(be_reg, static_reg_cfg->mask_similar_cnt);
    isp_wdr_dftwgt_fl_write(be_reg, static_reg_cfg->dft_wgt_fl);
    isp_wdr_bldrlhfidx_write(be_reg, static_reg_cfg->bldr_lhf_idx);
    isp_wdr_forcelong_smooth_en_write(be_reg, static_reg_cfg->force_long_smooth_en);

    static_reg_cfg->resh = TD_FALSE;
}

static td_void isp_wdr_dyna_reg_config_first(isp_pre_be_reg_type *be_reg, isp_fswdr_dyna_cfg *dyna_reg_cfg)
{
    isp_wdr_f0_still_thr_write(be_reg, dyna_reg_cfg->still_thr_lut[0]);

    isp_wdr_erosion_en_write(be_reg, dyna_reg_cfg->erosion_en);
}

static td_void isp_wdr_dyna_reg_config_second(isp_pre_be_reg_type *be_reg, isp_fswdr_dyna_cfg *dyna_reg_cfg,
    isp_post_be_reg_type *post_be_reg)
{
    isp_wdr_forcelong_en_write(be_reg, dyna_reg_cfg->force_long);
    isp_wdr_forcelong_low_thd_write(be_reg, dyna_reg_cfg->force_long_low_thr);
    isp_wdr_forcelong_high_thd_write(be_reg, dyna_reg_cfg->force_long_hig_thr);

    isp_wdr_shortchk_thd_write(be_reg, dyna_reg_cfg->short_check_thd);
    isp_wdr_fusion_data_mode_write(be_reg, dyna_reg_cfg->fusion_data_mode);
}

static td_void isp_wdr_user_reg_config(isp_pre_be_reg_type *be_reg, isp_fswdr_usr_cfg *usr_reg_cfg,
    td_bool offline_mode)
{
    td_bool usr_resh;
    td_bool idx_resh;

    idx_resh = (isp_wdr_update_index_read(be_reg) != usr_reg_cfg->update_index);
    usr_resh = (offline_mode) ? (usr_reg_cfg->resh && idx_resh) : (usr_reg_cfg->resh);
    if (usr_resh) {
        isp_wdr_update_index_write(be_reg, usr_reg_cfg->update_index);
        isp_wdr_shortexpo_chk_write(be_reg, usr_reg_cfg->short_expo_chk);
        isp_wdr_mdtlbld_write(be_reg, usr_reg_cfg->mdt_l_bld);
        isp_wdr_mdt_full_thr_write(be_reg, usr_reg_cfg->mdt_full_thr);
        isp_wdr_mdt_still_thr_write(be_reg, usr_reg_cfg->mdt_still_thr);
        isp_wdr_pixel_avg_max_diff_write(be_reg, usr_reg_cfg->pixel_avg_max_diff);
        usr_reg_cfg->resh = offline_mode;
    }
}

static td_void isp_wdr_module_en_write(ot_vi_pipe vi_pipe, isp_reg_cfg *reg_cfg_info, td_u8 i)
{
    isp_viproc_reg_type *pre_viproc = TD_NULL;
    isp_viproc_reg_type *post_viproc = TD_NULL;
    pre_viproc = (isp_viproc_reg_type *)isp_get_pre_vi_proc_vir_addr(vi_pipe, i);
    post_viproc = (isp_viproc_reg_type *)isp_get_post_vi_proc_vir_addr(vi_pipe, i);
    isp_check_pointer_void_return(pre_viproc);
    isp_check_pointer_void_return(post_viproc);

    isp_wdr_en_write(pre_viproc, reg_cfg_info->alg_reg_cfg[i].wdr_reg_cfg.wdr_en);
    isp_bcom_en_write(pre_viproc, reg_cfg_info->alg_reg_cfg[i].wdr_reg_cfg.dyna_reg_cfg.bcom_en);
    isp_bdec_en_write(post_viproc, reg_cfg_info->alg_reg_cfg[i].wdr_reg_cfg.dyna_reg_cfg.bdec_en);
}

static td_s32 isp_wdr_reg_config(ot_vi_pipe vi_pipe, isp_reg_cfg *reg_cfg_info, td_u8 i)
{
    td_u16   offset0;
    td_u32   blc_comp0;
    td_bool offline_mode;
    td_u8 blk_num = reg_cfg_info->cfg_num;
    isp_fswdr_static_cfg *static_reg_cfg = TD_NULL;
    isp_fswdr_dyna_cfg *dyna_reg_cfg = TD_NULL;
    isp_fswdr_usr_cfg *usr_reg_cfg = TD_NULL;
    isp_pre_be_reg_type *be_reg = TD_NULL;
    isp_post_be_reg_type *post_be_reg = TD_NULL;
    isp_usr_ctx *isp_ctx = TD_NULL;
    isp_get_ctx(vi_pipe, isp_ctx);
    offline_mode =
        (is_offline_mode(isp_ctx->block_attr.running_mode) || is_striping_mode(isp_ctx->block_attr.running_mode));
    if (reg_cfg_info->cfg_key.bit1_fs_wdr_cfg) {
        be_reg = (isp_pre_be_reg_type *)isp_get_pre_be_vir_addr(vi_pipe, i);
        post_be_reg = (isp_post_be_reg_type *)isp_get_post_be_vir_addr(vi_pipe, i);
        isp_check_pointer_return(be_reg);
        isp_check_pointer_return(post_be_reg);
        isp_wdr_module_en_write(vi_pipe, reg_cfg_info, i);

        isp_bnr_wdr_enable_write(post_be_reg, reg_cfg_info->alg_reg_cfg[i].wdr_reg_cfg.wdr_en);
        static_reg_cfg = &reg_cfg_info->alg_reg_cfg[i].wdr_reg_cfg.static_reg_cfg;
        usr_reg_cfg = &reg_cfg_info->alg_reg_cfg[i].wdr_reg_cfg.usr_reg_cfg;
        dyna_reg_cfg = &reg_cfg_info->alg_reg_cfg[i].wdr_reg_cfg.dyna_reg_cfg;

        if (static_reg_cfg->first_frame == TD_TRUE) {
            isp_wdr_static_reg_config_first_frame(be_reg, static_reg_cfg, dyna_reg_cfg, usr_reg_cfg, offline_mode);

            offset0  = reg_cfg_info->alg_reg_cfg[i].wdr_reg_cfg.sync_reg_cfg.offset0;
            blc_comp0 = static_reg_cfg->blc_comp_lut[0] * offset0;
            static_reg_cfg->blc_comp_lut[0] = blc_comp0;
            isp_wdr_blc_comp0_write(be_reg, blc_comp0);
            isp_wdr_wgt_slope_write(be_reg, dyna_reg_cfg->wgt_slope);
        }
        if (static_reg_cfg->resh) {
            isp_wdr_static_reg_config(be_reg, static_reg_cfg);
            isp_wdr_blc_comp0_write(be_reg, static_reg_cfg->blc_comp_lut[0]);
            isp_wdr_saturate_thr_write(be_reg, static_reg_cfg->saturate_thr);
            isp_wdr_fusion_saturate_thd_write(be_reg, static_reg_cfg->fusion_saturate_thd);
        }

        isp_wdr_user_reg_config(be_reg, usr_reg_cfg, offline_mode);

        if (dyna_reg_cfg->resh) {
            isp_wdr_dyna_reg_config_first(be_reg, dyna_reg_cfg);
            isp_wdr_dyna_reg_config_second(be_reg, dyna_reg_cfg, post_be_reg);

            isp_wdr_mdt_nosf_hig_thr_write(be_reg, dyna_reg_cfg->mdt_nf_high_thr);
            isp_wdr_mdt_nosf_low_thr_write(be_reg, dyna_reg_cfg->mdt_nf_low_thr);
            isp_wdr_gain_sum_hig_thr_write(be_reg, dyna_reg_cfg->gain_sum_high_thr);
            isp_wdr_gain_sum_low_thr_write(be_reg, dyna_reg_cfg->gain_sum_low_thr);
            isp_wdr_forcelong_slope_write(be_reg, dyna_reg_cfg->force_long_slope);
            isp_wdr_fusion_f0_thr_r_write(be_reg, dyna_reg_cfg->fusion_thr_r_lut[0]);
            isp_wdr_fusion_f1_thr_r_write(be_reg, dyna_reg_cfg->fusion_thr_r_lut[1]);
            isp_wdr_fusion_f0_thr_g_write(be_reg, dyna_reg_cfg->fusion_thr_g_lut[0]);
            isp_wdr_fusion_f1_thr_g_write(be_reg, dyna_reg_cfg->fusion_thr_g_lut[1]);
            isp_wdr_fusion_f0_thr_b_write(be_reg, dyna_reg_cfg->fusion_thr_b_lut[0]);
            isp_wdr_fusion_f1_thr_b_write(be_reg, dyna_reg_cfg->fusion_thr_b_lut[1]);
            isp_wdr_fusion_data_mode_write(be_reg, dyna_reg_cfg->fusion_data_mode);

            dyna_reg_cfg->resh = offline_mode;
        }

        reg_cfg_info->cfg_key.bit1_fs_wdr_cfg = offline_mode ? 1 : ((blk_num <= (i + 1)) ? 0 : 1);
    }
    return TD_SUCCESS;
}

static td_void isp_drc_cclut_reg_config(td_bool offline_mode, isp_post_be_reg_type *be_reg,
    isp_drc_usr_cfg *usr_reg_cfg)
{
    td_u16 j;

    if (offline_mode == TD_FALSE) {
        isp_drc_cclut_waddr_write(be_reg, 0);

        for (j = 0; j < OT_ISP_DRC_CC_NODE_NUM; j++) {
            isp_drc_cclut_wdata_write(be_reg, usr_reg_cfg->cc_lut[j] << 2); /* shift 2 */
        }
    } else {
        isp_drc_cclut_write(&be_reg->be_lut.be_apb_lut, usr_reg_cfg->cc_lut);
    }
}

static td_void isp_drc_delut_reg_config(td_bool offline_mode, isp_be_reg_type *be_reg,
                                        isp_drc_usr_cfg *usr_reg_cfg)
{
    td_u16   j;

    if (offline_mode == TD_FALSE) {
        isp_drc_delut_waddr_write(be_reg, 0);
        for (j = 0; j < OT_ISP_DRC_BCNR_NODE_NUM; j++) {
            isp_drc_delut_wdata_write(be_reg, usr_reg_cfg->bcnr_detail_restore_lut[j]);
        }
    } else {
        isp_drc_delut_write(&be_reg->be_lut.be_apb_lut, usr_reg_cfg->bcnr_detail_restore_lut);
    }
    isp_drc_lut_update2_write(be_reg, 1);
}

static td_void isp_drc_coefa_reg_config(td_bool offline_mode, isp_be_reg_type *be_reg,
                                        isp_drc_usr_cfg *usr_reg_cfg)
{
    td_u16 j;

    if (offline_mode == TD_FALSE) {
        isp_drc_coefalut_waddr_write(be_reg, 0);
        for (j = 0; j < OT_ISP_DRC_BCNR_STRENGTH_NODE_NUM; j++) {
            isp_drc_coefalut_wdata_write(be_reg, usr_reg_cfg->bcnr_filter_strength[j]);
        }
    } else {
        isp_drc_coefalut_write(&be_reg->be_lut.be_apb_lut, usr_reg_cfg->bcnr_filter_strength);
    }
    isp_drc_lut_update1_write(be_reg, 1);
}
static td_void isp_drc_tmlut_reg_config(td_bool *tm_lut_update, const isp_usr_ctx *isp_ctx,
    isp_post_be_reg_type *be_reg, isp_drc_dyna_cfg *dyna_reg_cfg)
{
    td_u16 j;

    if (is_online_mode(isp_ctx->block_attr.running_mode)) {
        if (dyna_reg_cfg->lut_update) {
            isp_drc_tmlut0_waddr_write(be_reg, 0);
            for (j = 0; j < OT_ISP_DRC_TM_NODE_NUM; j++) {
                 /* 16 bit  2 shift */
                isp_drc_tmlut0_wdata_write(be_reg, (((td_u32)(dyna_reg_cfg->tone_mapping_value[j]) << 16)) |
                ((td_u32)(dyna_reg_cfg->tone_mapping_diff[j]) << 2)); // 2: bit
            }
            *tm_lut_update = TD_TRUE;
        }
    } else {
        isp_drc_tmlut0_value_write(&be_reg->be_lut.be_apb_lut, dyna_reg_cfg->tone_mapping_value);
        isp_drc_tmlut0_diff_write(&be_reg->be_lut.be_apb_lut, dyna_reg_cfg->tone_mapping_diff);
    }
}

static td_void isp_drc_static_comm_reg_config(isp_post_be_reg_type *be_reg, isp_drc_static_cfg *static_reg_cfg)
{
    isp_drc_wrstat_en_write(be_reg, static_reg_cfg->wrstat_en);
    isp_drc_rdstat_en_write(be_reg, static_reg_cfg->rdstat_en);

    isp_drc_bin_write(be_reg, static_reg_cfg->bin_num_z);

    isp_drc_mono_chroma_en_write(be_reg, static_reg_cfg->monochrome_mode);

    isp_drc_wgt_box_tri_sel_write(be_reg, static_reg_cfg->wgt_box_tri_sel);

    isp_drc_dp_det_thrmin_write(be_reg, static_reg_cfg->dp_det_thr_min);
    isp_drc_dp_det_thrslo_write(be_reg, static_reg_cfg->dp_det_thr_slo);
    isp_drc_dp_det_g2rb_write(be_reg, static_reg_cfg->dp_det_g2rb);
    isp_drc_dp_det_rb2rb_write(be_reg, static_reg_cfg->dp_det_rb2rb);
    isp_drc_dp_det_replctr_write(be_reg, static_reg_cfg->dp_det_repl_ctr);
    isp_drc_dp_det_rngrto_write(be_reg, static_reg_cfg->dp_det_rng_ratio);
}

static td_void isp_drc_seg_reg_config(isp_post_be_reg_type *be_reg, isp_drc_static_cfg *static_reg_cfg)
{
    isp_drc_idxbase0_write(be_reg, static_reg_cfg->seg_idx_base[0]); /* idxbase[0] */
    isp_drc_idxbase1_write(be_reg, static_reg_cfg->seg_idx_base[1]); /* idxbase[1] */
    isp_drc_idxbase2_write(be_reg, static_reg_cfg->seg_idx_base[2]); /* idxbase[2] */
    isp_drc_idxbase3_write(be_reg, static_reg_cfg->seg_idx_base[3]); /* idxbase[3] */
    isp_drc_idxbase4_write(be_reg, static_reg_cfg->seg_idx_base[4]); /* idxbase[4] */
    isp_drc_idxbase5_write(be_reg, static_reg_cfg->seg_idx_base[5]); /* idxbase[5] */
    isp_drc_idxbase6_write(be_reg, static_reg_cfg->seg_idx_base[6]); /* idxbase[6] */
    isp_drc_idxbase7_write(be_reg, static_reg_cfg->seg_idx_base[7]); /* idxbase[7] */

    isp_drc_maxval0_write(be_reg, static_reg_cfg->seg_max_val[0]); /* maxval[0] */
    isp_drc_maxval1_write(be_reg, static_reg_cfg->seg_max_val[1]); /* maxval[1] */
    isp_drc_maxval2_write(be_reg, static_reg_cfg->seg_max_val[2]); /* maxval[2] */
    isp_drc_maxval3_write(be_reg, static_reg_cfg->seg_max_val[3]); /* maxval[3] */
    isp_drc_maxval4_write(be_reg, static_reg_cfg->seg_max_val[4]); /* maxval[4] */
    isp_drc_maxval5_write(be_reg, static_reg_cfg->seg_max_val[5]); /* maxval[5] */
    isp_drc_maxval6_write(be_reg, static_reg_cfg->seg_max_val[6]); /* maxval[6] */
    isp_drc_maxval7_write(be_reg, static_reg_cfg->seg_max_val[7]); /* maxval[7] */
}

static td_void isp_drc_prev_luma_reg_config(isp_post_be_reg_type *be_reg, isp_drc_static_cfg *static_reg_cfg)
{
    if (static_reg_cfg->first_frame) {
        isp_drc_shp_log_write(be_reg, static_reg_cfg->shp_log);
        isp_drc_shp_exp_write(be_reg, static_reg_cfg->shp_exp);
        isp_drc_div_denom_log_write(be_reg, static_reg_cfg->div_denom_log);
        isp_drc_denom_exp_write(be_reg, static_reg_cfg->denom_exp);
        isp_drc_prev_luma_0_write(be_reg, static_reg_cfg->prev_luma[0]); /* prev_luma[0] */
        isp_drc_prev_luma_1_write(be_reg, static_reg_cfg->prev_luma[1]); /* prev_luma[1] */
        isp_drc_prev_luma_2_write(be_reg, static_reg_cfg->prev_luma[2]); /* prev_luma[2] */
        isp_drc_prev_luma_3_write(be_reg, static_reg_cfg->prev_luma[3]); /* prev_luma[3] */
        isp_drc_prev_luma_4_write(be_reg, static_reg_cfg->prev_luma[4]); /* prev_luma[4] */
        isp_drc_prev_luma_5_write(be_reg, static_reg_cfg->prev_luma[5]); /* prev_luma[5] */
        isp_drc_prev_luma_6_write(be_reg, static_reg_cfg->prev_luma[6]); /* prev_luma[6] */
        isp_drc_prev_luma_7_write(be_reg, static_reg_cfg->prev_luma[7]); /* prev_luma[7] */

        static_reg_cfg->first_frame = TD_FALSE;
    }
}

static td_void isp_drc_stat_reg_config(isp_post_be_reg_type *be_reg, isp_drc_dyna_cfg *dyna_reg_cfg)
{
    isp_drc_vbiflt_en_write(be_reg, dyna_reg_cfg->vbiflt_en);

    isp_drc_big_x_init_write(be_reg, dyna_reg_cfg->big_x_init);
    isp_drc_idx_x_init_write(be_reg, dyna_reg_cfg->idx_x_init);
    isp_drc_cnt_x_init_write(be_reg, dyna_reg_cfg->cnt_x_init);
    isp_drc_acc_x_init_write(be_reg, dyna_reg_cfg->acc_x_init);
    isp_drc_blk_wgt_init_write(be_reg, dyna_reg_cfg->wgt_x_init);
    isp_drc_total_width_write(be_reg, dyna_reg_cfg->total_width - 1);
    isp_drc_stat_width_write(be_reg, dyna_reg_cfg->stat_width - 1);

    isp_drc_hnum_write(be_reg, dyna_reg_cfg->block_h_num);
    isp_drc_vnum_write(be_reg, dyna_reg_cfg->block_v_num);

    isp_drc_zone_hsize_write(be_reg, dyna_reg_cfg->block_h_size - 1);
    isp_drc_zone_vsize_write(be_reg, dyna_reg_cfg->block_v_size - 1);
    isp_drc_chk_x_write(be_reg, dyna_reg_cfg->block_chk_x);
    isp_drc_chk_y_write(be_reg, dyna_reg_cfg->block_chk_y);

    isp_drc_div_x0_write(be_reg, dyna_reg_cfg->div_x0);
    isp_drc_div_x1_write(be_reg, dyna_reg_cfg->div_x1);
    isp_drc_div_y0_write(be_reg, dyna_reg_cfg->div_y0);
    isp_drc_div_y1_write(be_reg, dyna_reg_cfg->div_y1);

    isp_drc_bin_scale_write(be_reg, dyna_reg_cfg->bin_scale);
    isp_drc_strp_en_write(be_reg, dyna_reg_cfg->strp_en);
}

static td_void isp_drc_static_reg_config(isp_post_be_reg_type *be_reg, isp_drc_static_cfg *static_reg_cfg)
{
    if (static_reg_cfg->resh) {
        isp_drc_static_comm_reg_config(be_reg, static_reg_cfg);
        isp_drc_seg_reg_config(be_reg, static_reg_cfg);
        isp_drc_prev_luma_reg_config(be_reg, static_reg_cfg);

        static_reg_cfg->resh = TD_FALSE;
    }
}

static td_void isp_drc_usr_comm_reg_config(isp_post_be_reg_type *be_reg, isp_drc_usr_cfg *usr_reg_cfg)
{
    isp_drc_gain_clip_knee_write(be_reg, usr_reg_cfg->gain_clip_knee);
    isp_drc_gain_clip_step_write(be_reg, usr_reg_cfg->gain_clip_step);
    isp_drc_r_wgt_write(be_reg, usr_reg_cfg->r_wgt);
    isp_drc_g_wgt_write(be_reg, usr_reg_cfg->g_wgt);
    isp_drc_b_wgt_write(be_reg, usr_reg_cfg->b_wgt);

    isp_drc_line_detect_en_write(be_reg, usr_reg_cfg->bcnr_line_detect_en);
    isp_drc_inadpt_en_write(be_reg, usr_reg_cfg->inadpt_en);

    isp_drc_val1_y_write(be_reg, usr_reg_cfg->yval1);
    isp_drc_sft1_y_write(be_reg, usr_reg_cfg->ysft1);
    isp_drc_val2_y_write(be_reg, usr_reg_cfg->yval2);
    isp_drc_sft2_y_write(be_reg, usr_reg_cfg->ysft2);
    isp_drc_val1_c_write(be_reg, usr_reg_cfg->cval1);
    isp_drc_sft1_c_write(be_reg, usr_reg_cfg->csft1);
    isp_drc_val2_c_write(be_reg, usr_reg_cfg->cval2);
    isp_drc_sft2_c_write(be_reg, usr_reg_cfg->csft2);

    isp_drc_val_write(be_reg, usr_reg_cfg->val);
    isp_drc_sft_write(be_reg, usr_reg_cfg->sft);

    isp_drc_cc_lut_ctrl_write(be_reg, usr_reg_cfg->cclut_ctrl);

    isp_drc_cc_global_corr_write(be_reg, 1024); // 1024: alg num
    isp_drc_colorgain_slope_write(be_reg, 1);
    isp_drc_colorgain_thr_write(be_reg, 0);
    isp_drc_detailgain_slope_write(be_reg, 4); // 4 alg num
    isp_drc_flt_en_write(be_reg, 1);
}

static td_void isp_drc_flt_reg_config(isp_post_be_reg_type *be_reg, isp_drc_usr_cfg *usr_reg_cfg, td_bool offline_mode)
{
    isp_drc_bright_max_write(be_reg, usr_reg_cfg->local_mixing_bright_max);
    isp_drc_bright_min_write(be_reg, usr_reg_cfg->local_mixing_bright_min);
    isp_drc_bright_thr_write(be_reg, usr_reg_cfg->local_mixing_bright_thr);
    isp_drc_bright_slo_write(be_reg, usr_reg_cfg->local_mixing_bright_slo);
    isp_drc_dark_max_write(be_reg, usr_reg_cfg->local_mixing_dark_max);
    isp_drc_dark_min_write(be_reg, usr_reg_cfg->local_mixing_dark_min);
    isp_drc_dark_thr_write(be_reg, usr_reg_cfg->local_mixing_dark_thr);
    isp_drc_dark_slo_write(be_reg, usr_reg_cfg->local_mixing_dark_slo);

    isp_drc_detail_sub_factor_write(be_reg, usr_reg_cfg->detail_adjust_coef);

    isp_drc_suppress_bright_max_write(be_reg, usr_reg_cfg->suppress_bright_max);
    isp_drc_suppress_bright_min_write(be_reg, usr_reg_cfg->suppress_bright_min);
    isp_drc_suppress_bright_thr_write(be_reg, usr_reg_cfg->suppress_bright_thr);
    isp_drc_suppress_bright_slo_write(be_reg, usr_reg_cfg->suppress_bright_slo);
    isp_drc_suppress_dark_max_write(be_reg, usr_reg_cfg->suppress_dark_max);
    isp_drc_suppress_dark_min_write(be_reg, usr_reg_cfg->suppress_dark_min);
    isp_drc_suppress_dark_thr_write(be_reg, usr_reg_cfg->suppress_dark_thr);
    isp_drc_suppress_dark_slo_write(be_reg, usr_reg_cfg->suppress_dark_slo);

    isp_drc_grad_rev_thres_write(be_reg, usr_reg_cfg->grad_rev_thr);
    isp_drc_grad_rev_max_write(be_reg, usr_reg_cfg->grad_rev_max);
    isp_drc_grad_rev_slope_write(be_reg, usr_reg_cfg->grad_rev_slope);
    isp_drc_grad_rev_shift_write(be_reg, usr_reg_cfg->grad_rev_shift);

    isp_drc_flt_spa_fine_write(be_reg, usr_reg_cfg->flt_spa_fine);
    isp_drc_flt_rng_fine_write(be_reg, usr_reg_cfg->flt_rng_fine);

#ifdef CONFIG_OT_ISP_DRC_SWITCH_SUPPORT
    if (offline_mode) {
        isp_drc_var_rng_fine_write(be_reg, usr_reg_cfg->var_rng_fine);
    }
#else
    isp_drc_var_rng_fine_write(be_reg, usr_reg_cfg->var_rng_fine);
#endif
    isp_drc_mixing_coring_write(be_reg, usr_reg_cfg->local_mixing_coring);
}

static td_void isp_drc_usr_reg_config(isp_post_be_reg_type *be_reg, isp_drc_usr_cfg *usr_reg_cfg, td_bool offline_mode)
{
    td_bool usr_resh, idx_resh;
    /* user */
    idx_resh = (isp_drc_update_index_read(be_reg) != usr_reg_cfg->update_index);
    usr_resh = (offline_mode) ? (usr_reg_cfg->resh && idx_resh) : (usr_reg_cfg->resh);

    if (usr_resh) {
        isp_drc_update_index_write(be_reg, usr_reg_cfg->update_index);
        isp_drc_usr_comm_reg_config(be_reg, usr_reg_cfg);
        isp_drc_flt_reg_config(be_reg, usr_reg_cfg, offline_mode);
        isp_drc_cclut_reg_config(offline_mode, be_reg, usr_reg_cfg);
        isp_drc_delut_reg_config(offline_mode, be_reg, usr_reg_cfg);
        isp_drc_coefa_reg_config(offline_mode, be_reg, usr_reg_cfg);

        usr_reg_cfg->resh = offline_mode;
    }
}

static td_s32 isp_drc_get_bcnr_en(ot_vi_pipe vi_pipe, td_bool *bcnr_en)
{
    isp_usr_ctx *isp_ctx = TD_NULL;
    isp_get_ctx(vi_pipe, isp_ctx);
    td_u8 wdr_mode = isp_ctx->sns_wdr_mode;

    if (is_linear_mode(wdr_mode)) {
        *bcnr_en = TD_TRUE;
        if (isp_ctx->block_attr.online_ex_en == TD_TRUE) {
            *bcnr_en = TD_FALSE;
        }
    } else {
        *bcnr_en = TD_FALSE;
    }

    return TD_SUCCESS;
}

static td_s32 isp_drc_nr_config(ot_vi_pipe vi_pipe, isp_reg_cfg *reg_cfg_info, td_u8 i, isp_usr_ctx *isp_ctx)
{
    td_bool bcnr_en = TD_FALSE;
    td_bool bypass = TD_FALSE;
    isp_viproc_reg_type *post_viproc = TD_NULL;
    isp_post_be_reg_type *be_reg = TD_NULL;

    be_reg = (isp_post_be_reg_type *)isp_get_post_be_vir_addr(vi_pipe, i);
    post_viproc = (isp_viproc_reg_type *)isp_get_post_vi_proc_vir_addr(vi_pipe, i);

    isp_check_pointer_return(be_reg);
    isp_check_pointer_return(post_viproc);

    isp_drc_en_write(post_viproc, TD_TRUE);

#ifdef CONFIG_OT_ISP_DRC_SWITCH_SUPPORT
    if (!is_online_mode(isp_ctx->block_attr.running_mode) || isp_ctx->isp_yuv_mode == TD_TRUE) {
        bypass = (reg_cfg_info->alg_reg_cfg[i].drc_reg_cfg.enable == TD_TRUE) ? TD_FALSE : TD_TRUE;
        isp_drc_inner_bypass_en_write(be_reg, bypass);
    }
#else
    bypass = (reg_cfg_info->alg_reg_cfg[i].drc_reg_cfg.enable == TD_TRUE) ? TD_FALSE : TD_TRUE;
    isp_drc_inner_bypass_en_write(be_reg, bypass);
#endif

    isp_drc_get_bcnr_en(vi_pipe, &bcnr_en);
    isp_drc_nr_en_write(be_reg, bcnr_en);

    return TD_SUCCESS;
}

static td_s32 isp_drc_reg_config(ot_vi_pipe vi_pipe, isp_reg_cfg *reg_cfg_info, td_u8 i)
{
    td_bool offline_mode;
    td_bool tm_lut_update = TD_FALSE;
    td_u8 blk_num = reg_cfg_info->cfg_num;
    isp_drc_dyna_cfg *dyna_reg_cfg = TD_NULL;
    isp_viproc_reg_type *post_viproc = TD_NULL;
    isp_post_be_reg_type *be_reg = TD_NULL;
    isp_usr_ctx *isp_ctx = TD_NULL;

    be_reg = (isp_post_be_reg_type *)isp_get_post_be_vir_addr(vi_pipe, i);
    post_viproc = (isp_viproc_reg_type *)isp_get_post_vi_proc_vir_addr(vi_pipe, i);

    isp_check_pointer_return(be_reg);
    isp_check_pointer_return(post_viproc);

    isp_get_ctx(vi_pipe, isp_ctx);
    offline_mode =
        (is_offline_mode(isp_ctx->block_attr.running_mode) || is_striping_mode(isp_ctx->block_attr.running_mode) ||
        is_pre_online_post_offline(isp_ctx->block_attr.running_mode));

    isp_drc_nr_config(vi_pipe, reg_cfg_info, i, isp_ctx);

    if (reg_cfg_info->cfg_key.bit1_drc_cfg) {
        isp_drc_static_reg_config(be_reg, &reg_cfg_info->alg_reg_cfg[i].drc_reg_cfg.static_reg_cfg);
        isp_drc_usr_reg_config(be_reg, &reg_cfg_info->alg_reg_cfg[i].drc_reg_cfg.usr_reg_cfg, offline_mode);

        /* dynamic */
        dyna_reg_cfg = &reg_cfg_info->alg_reg_cfg[i].drc_reg_cfg.dyna_reg_cfg;

        if (dyna_reg_cfg->resh) {
            isp_drc_color_corr_en_write(be_reg, dyna_reg_cfg->cc_en);
            isp_drc_tmlut_reg_config(&tm_lut_update, isp_ctx, be_reg, dyna_reg_cfg);

            if (dyna_reg_cfg->img_size_changed) {
                isp_drc_stat_reg_config(be_reg, dyna_reg_cfg);
                dyna_reg_cfg->img_size_changed = offline_mode;
            }

            isp_drc_strength_write(be_reg, dyna_reg_cfg->strength);

            dyna_reg_cfg->resh = offline_mode;
        }

        reg_cfg_info->cfg_key.bit1_drc_cfg = offline_mode ? 1 : ((blk_num <= (i + 1)) ? 0 : 1);
    }

    reg_cfg_info->alg_reg_cfg[i].be_lut_update_cfg.drc_tm_lut_update = tm_lut_update || offline_mode;

    return TD_SUCCESS;
}

static td_void isp_dehaze_static_reg_config(isp_post_be_reg_type *be_reg, isp_dehaze_static_cfg *static_reg_cfg)
{
    /* static registers */
    if (static_reg_cfg->resh) {
        isp_dehaze_blthld_write(be_reg, static_reg_cfg->dehaze_blthld);
        isp_dehaze_block_sum_write(be_reg, static_reg_cfg->block_sum);
        isp_dehaze_dc_numh_write(be_reg, static_reg_cfg->dchnum);
        isp_dehaze_dc_numv_write(be_reg, static_reg_cfg->dcvnum);

        static_reg_cfg->resh = TD_FALSE;
    }
}

static td_s32 isp_dehaze_dyna_lut_reg_config(ot_vi_pipe vi_pipe, isp_reg_cfg *reg_cfg_info, td_u8 i,
    isp_post_be_reg_type *be_reg, isp_viproc_reg_type *post_viproc)
{
    td_u8 buf_id;
    td_u16 blk_num;
    td_s32 ret;
    isp_usr_ctx *isp_ctx = TD_NULL;
    isp_dehaze_static_cfg *static_reg_cfg = TD_NULL;
    isp_dehaze_dyna_cfg *dyna_reg_cfg = TD_NULL;
    isp_be_lut_wstt_type *be_lut_stt_reg = TD_NULL;
    isp_get_ctx(vi_pipe, isp_ctx);

    static_reg_cfg = &reg_cfg_info->alg_reg_cfg[i].dehaze_reg_cfg.static_reg_cfg;
    dyna_reg_cfg = &reg_cfg_info->alg_reg_cfg[i].dehaze_reg_cfg.dyna_reg_cfg;

    blk_num = ((static_reg_cfg->dchnum + 1) * (static_reg_cfg->dcvnum + 1) + 1) / 2; /* blk calculation 2 */
    if (is_online_mode(isp_ctx->block_attr.running_mode)) {
        /* online lut2stt regconfig */
        buf_id = dyna_reg_cfg->buf_id;

        be_lut_stt_reg = (isp_be_lut_wstt_type *)isp_get_be_lut2stt_vir_addr(vi_pipe, i, buf_id);
        isp_check_pointer_return(be_lut_stt_reg);

        isp_dehaze_lut_wstt_write(be_lut_stt_reg, blk_num, dyna_reg_cfg->prestat, dyna_reg_cfg->lut);
        ret = isp_dehaze_lut_wstt_addr_write(vi_pipe, i, buf_id, post_viproc);
        if (ret != TD_SUCCESS) {
            isp_err_trace("isp[%d]_dehaze_lut_wstt_addr_write failed\n", vi_pipe);
            return ret;
        }

        dyna_reg_cfg->buf_id = 1 - buf_id;
    } else {
        isp_dehaze_lut_wstt_write(&be_reg->be_lut.be_lut2stt, blk_num, dyna_reg_cfg->prestat,
            dyna_reg_cfg->lut);
    }

    isp_dehaze_stt2lut_en_write(be_reg, TD_TRUE);
    isp_dehaze_stt2lut_regnew_write(be_reg, TD_TRUE);

    return TD_SUCCESS;
}

static td_s32 isp_dehaze_reg_config(ot_vi_pipe vi_pipe, isp_reg_cfg *reg_cfg_info, td_u8 i)
{
    td_bool offline_mode;
    td_u8 sys_blk_num = reg_cfg_info->cfg_num;
    td_s32 ret;
    isp_dehaze_static_cfg *static_reg_cfg = TD_NULL;
    isp_dehaze_dyna_cfg *dyna_reg_cfg = TD_NULL;
    isp_viproc_reg_type *post_viproc = TD_NULL;
    isp_post_be_reg_type *be_reg = TD_NULL;
    isp_usr_ctx *isp_ctx = TD_NULL;

    isp_get_ctx(vi_pipe, isp_ctx);
    offline_mode =
        (is_offline_mode(isp_ctx->block_attr.running_mode) || is_striping_mode(isp_ctx->block_attr.running_mode) ||
        is_pre_online_post_offline(isp_ctx->block_attr.running_mode));

    if (reg_cfg_info->cfg_key.bit1_dehaze_cfg) {
        be_reg = (isp_post_be_reg_type *)isp_get_post_be_vir_addr(vi_pipe, i);
        post_viproc = (isp_viproc_reg_type *)isp_get_post_vi_proc_vir_addr(vi_pipe, i);
        isp_check_pointer_return(be_reg);
        isp_check_pointer_return(post_viproc);
        static_reg_cfg = &reg_cfg_info->alg_reg_cfg[i].dehaze_reg_cfg.static_reg_cfg;
        dyna_reg_cfg = &reg_cfg_info->alg_reg_cfg[i].dehaze_reg_cfg.dyna_reg_cfg;

        isp_dehaze_static_reg_config(be_reg, static_reg_cfg);
        isp_dehaze_lut_width_word_write(post_viproc, OT_ISP_DEHAZE_LUT_WIDTH_WORD_DEFAULT);

        ret = isp_dehaze_dyna_lut_reg_config(vi_pipe, reg_cfg_info, i, be_reg, post_viproc);
        if (ret != TD_SUCCESS) {
            return ret;
        }

        isp_dehaze_block_sizeh_write(be_reg, dyna_reg_cfg->blockhsize);
        isp_dehaze_block_sizev_write(be_reg, dyna_reg_cfg->blockvsize);
        isp_dehaze_phase_x_write(be_reg, dyna_reg_cfg->phasex);
        isp_dehaze_phase_y_write(be_reg, dyna_reg_cfg->phasey);

        isp_dehaze_smlmapoffset_write(be_reg, dyna_reg_cfg->sml_map_offset);
        isp_dehaze_statstartx_write(be_reg, dyna_reg_cfg->stat_start_x);
        isp_dehaze_statendx_write(be_reg, dyna_reg_cfg->stat_end_x);

        isp_dehaze_stat_numv_write(be_reg, dyna_reg_cfg->statnum_v);
        isp_dehaze_stat_numh_write(be_reg, dyna_reg_cfg->statnum_h);

        isp_dehaze_thld_tr_write(be_reg, dyna_reg_cfg->dehaze_thld_r);
        isp_dehaze_thld_tg_write(be_reg, dyna_reg_cfg->dehaze_thld_g);
        isp_dehaze_thld_tb_write(be_reg, dyna_reg_cfg->dehaze_thld_b);

        reg_cfg_info->cfg_key.bit1_dehaze_cfg = offline_mode ? 1 : ((sys_blk_num <= (i + 1)) ? 0 : 1);
    }

    return TD_SUCCESS;
}

static td_void isp_bnr_lut_wstt_write(isp_be_lut_wstt_type *be_lut2stt, isp_bayernr_dyna_cfg *dyna_reg_cfg)
{
    isp_bnr_fbratiotable_lut_wstt_write(be_lut2stt, dyna_reg_cfg->u8fb_ratio_table);
    isp_bnr_noised_lut_wstt_write(be_lut2stt, dyna_reg_cfg->u16noise_sd_lut);
    isp_bnr_noiseds_lut_wstt_write(be_lut2stt, dyna_reg_cfg->u16noise_sd_lut_s);
    isp_bnr_noisedinv_lut_wstt_write(be_lut2stt, dyna_reg_cfg->u32noise_inv_lut);
    isp_bnr_noisedinv_mag_lut_wstt_write(be_lut2stt, dyna_reg_cfg->u8noise_inv_magidx);
    isp_bnr_coring_low_lut_wstt_write(be_lut2stt, dyna_reg_cfg->coring_low_lut);
    isp_bnr_fbratio_mot2yuv_table_lut_wstt_write(be_lut2stt, dyna_reg_cfg->u8fb_ratio_mot2yuv_table);
}

static td_void isp_bnr_lut_reg_config(ot_vi_pipe vi_pipe, isp_post_be_reg_type *be_reg,
    isp_bayernr_dyna_cfg *dyna_reg_cfg, td_u8 i)
{
    td_u8 buf_id = 0;
    isp_be_lut_wstt_type *be_lut_stt_reg = TD_NULL;
    isp_viproc_reg_type *viproc_reg = TD_NULL;

    isp_usr_ctx *isp_ctx = TD_NULL;
    isp_get_ctx(vi_pipe, isp_ctx);

    viproc_reg = (isp_viproc_reg_type *)isp_get_pre_vi_proc_vir_addr(vi_pipe, i);
    isp_check_pointer_void_return(viproc_reg);

    if (is_online_mode(isp_ctx->block_attr.running_mode)) {
        buf_id = dyna_reg_cfg->buf_id;

        be_lut_stt_reg = (isp_be_lut_wstt_type *)isp_get_be_lut2stt_vir_addr(vi_pipe, i, buf_id);
        isp_check_pointer_void_return(be_lut_stt_reg);

        isp_bnr_lut_wstt_write(be_lut_stt_reg, dyna_reg_cfg);
        isp_bnr_lut_wstt_addr_write(vi_pipe, i, buf_id, viproc_reg);

        dyna_reg_cfg->buf_id = 1 - buf_id;
    } else {
        isp_bnr_lut_wstt_write(&be_reg->be_lut.be_lut2stt, dyna_reg_cfg);
    }
    isp_bnr_stt2lut_en_write(be_reg, TD_TRUE);
    isp_bnr_stt2lut_regnew_write(be_reg, TD_TRUE);
    isp_bnr_lut_width_word_write(viproc_reg, OT_ISP_BNR_LUT_WIDTH_WORD_DEFAULT);
}

static td_void isp_bnr_dyna_reg_config(isp_post_be_reg_type *be_reg, isp_bayernr_dyna_cfg *dyna_reg_cfg)
{
    isp_bnr_ensptnr_write(be_reg, dyna_reg_cfg->snr_enable);
    isp_bnr_bltev500_n_limit_gain_sad_d_write(be_reg, dyna_reg_cfg->isp_bnr_bltev500_n_limit_gain_sad_d);
    isp_bnr_bltev500_pnr_write(be_reg, dyna_reg_cfg->isp_bnr_bltev500_pnr);
    isp_bnr_bltev500_en_imp_nr_write(be_reg, dyna_reg_cfg->isp_bnr_bltev500_en_imp_nr);
    isp_bnr_bltev500_imp_nr_str_write(be_reg, dyna_reg_cfg->isp_bnr_bltev500_imp_nr_str);
    isp_bnr_bltev500_wt_ccoef_write(be_reg, dyna_reg_cfg->isp_bnr_bltev500_wt_ccoef);
    isp_bnr_bltev500_wt_cmax_write(be_reg, dyna_reg_cfg->isp_bnr_bltev500_wt_cmax);
    isp_bnr_jnlm_lmt_gain0_write(be_reg, dyna_reg_cfg->jnlm_limit_mult_gain[0]); /* index 0 */
    isp_bnr_jnlm_lmt_gain1_write(be_reg, dyna_reg_cfg->jnlm_limit_mult_gain[1]); /* index 1 */
    isp_bnr_jnlm_lmt_gain2_write(be_reg, dyna_reg_cfg->jnlm_limit_mult_gain[2]); /* index 2 */
    isp_bnr_jnlm_lmt_gain3_write(be_reg, dyna_reg_cfg->jnlm_limit_mult_gain[3]); /* index 3 */
    isp_bnr_jnlm_lmt_gain4_write(be_reg, dyna_reg_cfg->jnlm_limit_mult_gain_s[0]); /* index 0 */
    isp_bnr_jnlm_lmt_gain5_write(be_reg, dyna_reg_cfg->jnlm_limit_mult_gain_s[1]); /* index 1 */
    isp_bnr_jnlm_lmt_gain6_write(be_reg, dyna_reg_cfg->jnlm_limit_mult_gain_s[2]); /* index 2 */
    isp_bnr_jnlm_lmt_gain7_write(be_reg, dyna_reg_cfg->jnlm_limit_mult_gain_s[3]); /* index 3 */
    isp_bnr_mdet_size_write(be_reg, dyna_reg_cfg->isp_bnr_mdet_size);
    isp_bnr_mdet_cor_level_write(be_reg, dyna_reg_cfg->isp_bnr_mdet_cor_level);
    isp_bnr_rnt_th_write(be_reg, dyna_reg_cfg->isp_bnr_rnt_th);
    isp_bnr_rnt_wid_write(be_reg, dyna_reg_cfg->isp_bnr_rnt_wid);
    isp_bnr_nr_strength_st_int_write(be_reg, dyna_reg_cfg->isp_bnr_nr_strength_st_int);
    isp_bnr_nr_strength_mv_int_write(be_reg, dyna_reg_cfg->isp_bnr_nr_strength_mv_int);
    isp_bnr_nr_strength_slope_write(be_reg, dyna_reg_cfg->isp_bnr_nr_strength_slope);

    isp_bnr_rnt_th_mot2yuv_write(be_reg, dyna_reg_cfg->isp_bnr_rnt_th_mot2yuv);
    isp_bnr_tss_write(be_reg, dyna_reg_cfg->isp_bnr_tss);
    isp_bnr_freqreconst_en_write(be_reg, dyna_reg_cfg->isp_bnr_freqreconst_en);

    isp_bnr_target_noise_ratio_write(be_reg, dyna_reg_cfg->isp_bnr_target_noise_ratio);
    isp_bnr_mix_gain_0_r_write(be_reg, dyna_reg_cfg->isp_bnr_mix_gain_0_r);
    isp_bnr_mix_gain_0_b_write(be_reg, dyna_reg_cfg->isp_bnr_mix_gain_0_b);
    isp_bnr_wb_gain_r_write(be_reg, dyna_reg_cfg->isp_bnr_wb_gain_r);
    isp_bnr_wb_gain_b_write(be_reg, dyna_reg_cfg->isp_bnr_wb_gain_b);
    isp_bnr_wb_gain_inv_r_write(be_reg, dyna_reg_cfg->isp_bnr_wb_gain_inv_r);
    isp_bnr_wb_gain_inv_b_write(be_reg, dyna_reg_cfg->isp_bnr_wb_gain_inv_b);
    isp_bnr_bnrlscen_write(be_reg, dyna_reg_cfg->bnr_lsc_en);
    isp_bnr_bnrlscratio_write(be_reg, dyna_reg_cfg->bnr_lsc_ratio);
    isp_bnr_wdr_en_fusion_write(be_reg, dyna_reg_cfg->wdr_en_fusion);
    isp_bnr_wdr_map_flt_mod_write(be_reg, dyna_reg_cfg->wdr_map_flt_mod);
    isp_bnr_wdr_map_gain_write(be_reg, dyna_reg_cfg->wdr_map_gain);
    isp_bnr_en_2nd_tmp_nr_write(be_reg, 1);

    isp_bnr_alphamax_st_write(be_reg, dyna_reg_cfg->alpha_max_st);
    isp_bnr_maxv_mode_write(be_reg, dyna_reg_cfg->maxv_mode);
    isp_bnr_coring_hig_write(be_reg, dyna_reg_cfg->coring_hig);
    isp_bnr_coring_enable_write(be_reg, dyna_reg_cfg->coring_enable);
    isp_bnr_jnlm_gain_write(be_reg, dyna_reg_cfg->fine_str);
    isp_bnr_mixgain_sprs_en_write(be_reg, dyna_reg_cfg->en_mix_gain_sprs);
    isp_bnr_inputraw_ratio_write(be_reg, dyna_reg_cfg->input_raw_ratio);

    isp_bnr_mdet_mixratio_write(be_reg, dyna_reg_cfg->isp_bnr_mdet_mix_ratio);
    isp_bnr_ynet_alpha_write(be_reg, dyna_reg_cfg->ynet_alpha);
    isp_bnr_sad_bit_reduction_write(be_reg, dyna_reg_cfg->isp_bnr_sad_bit_reduction);
    isp_bnr_blt_enable_symm_sad_write(be_reg, dyna_reg_cfg->isp_bnr_blt_enable_symm_sad);
}

static td_void isp_bnr_online_lut2stt_info_check(td_bool offline_mode, isp_post_be_reg_type *be_reg)
{
    td_u16 lut2stt_info;
    if (offline_mode == TD_TRUE) {
        return;
    }

    lut2stt_info = isp_bnr_stt2lut_info_read(be_reg);
    if (lut2stt_info != 0) {
        isp_bnr_stt2lut_regnew_write(be_reg, TD_TRUE);
        isp_bnr_stt2lut_clr_write(be_reg, 1);
    }
}

static td_s32 isp_bayer_nr_reg_config(ot_vi_pipe vi_pipe, isp_reg_cfg *reg_cfg_info, td_u8 i)
{
    td_u8 blk_num = reg_cfg_info->cfg_num;
    td_bool offline_mode, mot_en;
    isp_bayernr_static_cfg *static_reg_cfg = TD_NULL;
    isp_bayernr_dyna_cfg *dyna_reg_cfg = TD_NULL;
    isp_post_be_reg_type *be_reg = TD_NULL;
    isp_usr_ctx *isp_ctx = TD_NULL;
    isp_viproc_reg_type *viproc_reg = TD_NULL;

    isp_get_ctx(vi_pipe, isp_ctx);

    offline_mode =
        (is_offline_mode(isp_ctx->block_attr.running_mode) || is_striping_mode(isp_ctx->block_attr.running_mode));

    if (reg_cfg_info->cfg_key.bit1_bayernr_cfg) {
        be_reg = (isp_post_be_reg_type *)isp_get_pre_be_vir_addr(vi_pipe, i);
        isp_check_pointer_return(be_reg);
        viproc_reg = (isp_viproc_reg_type *)isp_get_pre_vi_proc_vir_addr(vi_pipe, i);
        isp_check_pointer_return(viproc_reg);

        static_reg_cfg = &reg_cfg_info->alg_reg_cfg[i].bnr_reg_cfg.static_reg_cfg;
        dyna_reg_cfg = &reg_cfg_info->alg_reg_cfg[i].bnr_reg_cfg.dyna_reg_cfg;
        mot_en = dyna_reg_cfg->tnr_enable && reg_cfg_info->alg_reg_cfg[i].bnr_reg_cfg.bnr_enable;
        if (static_reg_cfg->resh) { /* static */
            isp_bnr_frm_phase_write(be_reg, 1);
            isp_bnr_en_area_alt_write(be_reg, 0);
            isp_bnr_b_deform_md_wim_write(be_reg, 0);
            isp_bnr_rb_diff_en_write(be_reg, 0);
            isp_bnr_en_write(viproc_reg, reg_cfg_info->alg_reg_cfg[i].bnr_reg_cfg.bnr_enable);
            isp_bnr_entmpnr_write(be_reg, dyna_reg_cfg->tnr_enable);
            isp_sharpen_mot_en_write(be_reg, mot_en);
            isp_bnr_isinitial_write(be_reg, TD_TRUE);
            isp_bnr_jnlm_lum_sel_write(be_reg, static_reg_cfg->isp_bnr_jnlm_lum_sel);
            isp_bnr_bltev500_win_size_write(be_reg, static_reg_cfg->isp_bnr_bltev500_win_size);
            isp_bnr_en_mixing_write(be_reg, static_reg_cfg->mix_enable);
            isp_bnr2d_en_write(be_reg, static_reg_cfg->isp_bnr2d_en);
            static_reg_cfg->resh = TD_FALSE;
        }

        if (dyna_reg_cfg->resh) {
            isp_bnr_dyna_reg_config(be_reg, dyna_reg_cfg);
            isp_bnr_lut_reg_config(vi_pipe, be_reg, dyna_reg_cfg, i);
            dyna_reg_cfg->resh = offline_mode;
        }

        isp_bnr_online_lut2stt_info_check(offline_mode, be_reg);

        reg_cfg_info->cfg_key.bit1_bayernr_cfg = offline_mode ? 1 : ((blk_num <= (i + 1)) ? 0 : 1);
    }

    return TD_SUCCESS;
}

static td_s32 isp_dg_reg_config(ot_vi_pipe vi_pipe, isp_reg_cfg *reg_cfg_info, td_u8 i)
{
    td_u8 blk_num = reg_cfg_info->cfg_num;
    td_bool offline_mode;
    isp_dg_static_cfg *static_reg_cfg = TD_NULL;
    isp_dg_dyna_cfg *dyna_reg_cfg = TD_NULL;
    isp_viproc_reg_type *viproc_reg = TD_NULL;
    isp_post_be_reg_type *be_reg = TD_NULL;
    isp_usr_ctx *isp_ctx = TD_NULL;

    isp_get_ctx(vi_pipe, isp_ctx);
    offline_mode =
        (is_offline_mode(isp_ctx->block_attr.running_mode) || is_striping_mode(isp_ctx->block_attr.running_mode));

    if (reg_cfg_info->cfg_key.bit1_dg_cfg) {
        be_reg = (isp_post_be_reg_type *)isp_get_pre_be_vir_addr(vi_pipe, i);
        viproc_reg = (isp_viproc_reg_type *)isp_get_pre_vi_proc_vir_addr(vi_pipe, i);
        isp_check_pointer_return(be_reg);
        isp_check_pointer_return(viproc_reg);
        isp_dg_en_write(viproc_reg, reg_cfg_info->alg_reg_cfg[i].dg_reg_cfg.dg_en);

        static_reg_cfg = &reg_cfg_info->alg_reg_cfg[i].dg_reg_cfg.static_reg_cfg;
        dyna_reg_cfg = &reg_cfg_info->alg_reg_cfg[i].dg_reg_cfg.dyna_reg_cfg;

        if (static_reg_cfg->resh) {
            isp_dg_rgain_write(be_reg, dyna_reg_cfg->gain_r);
            isp_dg_grgain_write(be_reg, dyna_reg_cfg->gain_gr);
            isp_dg_gbgain_write(be_reg, dyna_reg_cfg->gain_gb);
            isp_dg_bgain_write(be_reg, dyna_reg_cfg->gain_b);

            static_reg_cfg->resh = TD_FALSE;
        }

        if (dyna_reg_cfg->resh) {
            isp_dg_clip_value_write(be_reg, dyna_reg_cfg->clip_value);
            dyna_reg_cfg->resh = offline_mode;
        }

        reg_cfg_info->cfg_key.bit1_dg_cfg = offline_mode ? 1 : ((blk_num <= (i + 1)) ? 0 : 1);
    }

    return TD_SUCCESS;
}

static td_s32 isp_4dg_reg_config(ot_vi_pipe vi_pipe, isp_reg_cfg *reg_cfg_info, td_u8 i)
{
    td_u8 blk_num = reg_cfg_info->cfg_num;
    td_bool offline_mode;
    isp_4dg_static_cfg *static_reg_cfg = TD_NULL;
    isp_4dg_dyna_cfg *dyna_reg_cfg = TD_NULL;
    isp_usr_ctx *isp_ctx = TD_NULL;
    isp_pre_be_reg_type *be_reg = TD_NULL;
    isp_viproc_reg_type *pre_viproc = TD_NULL;

    isp_get_ctx(vi_pipe, isp_ctx);
    offline_mode =
        (is_offline_mode(isp_ctx->block_attr.running_mode) || is_striping_mode(isp_ctx->block_attr.running_mode));

    if (reg_cfg_info->cfg_key.bit1_wdr_dg_cfg) {
        pre_viproc = (isp_viproc_reg_type *)isp_get_pre_vi_proc_vir_addr(vi_pipe, i);
        be_reg = (isp_pre_be_reg_type *)isp_get_pre_be_vir_addr(vi_pipe, i);
        isp_check_pointer_return(pre_viproc);
        isp_check_pointer_return(be_reg);

        if (is_linear_mode(isp_ctx->sns_wdr_mode)) {
            isp_4dg_en_write(pre_viproc, reg_cfg_info->alg_reg_cfg[i].dg_reg_cfg.dg_en);
        } else {
            isp_4dg_en_write(pre_viproc, TD_TRUE);
        }

        /* static */
        static_reg_cfg = &reg_cfg_info->alg_reg_cfg[i].four_dg_reg_cfg.static_reg_cfg;

        if (static_reg_cfg->resh) {
            isp_4dg0_rgain_write(be_reg, static_reg_cfg->gain_r0);
            isp_4dg0_grgain_write(be_reg, static_reg_cfg->gain_gr0);
            isp_4dg0_gbgain_write(be_reg, static_reg_cfg->gain_gb0);
            isp_4dg0_bgain_write(be_reg, static_reg_cfg->gain_b0);
            isp_4dg1_rgain_write(be_reg, static_reg_cfg->gain_r1);
            isp_4dg1_grgain_write(be_reg, static_reg_cfg->gain_gr1);
            isp_4dg1_gbgain_write(be_reg, static_reg_cfg->gain_gb1);
            isp_4dg1_bgain_write(be_reg, static_reg_cfg->gain_b1);
            static_reg_cfg->resh = TD_FALSE;
        }

        /* dynamic */
        dyna_reg_cfg = &reg_cfg_info->alg_reg_cfg[i].four_dg_reg_cfg.dyna_reg_cfg;

        if (dyna_reg_cfg->resh) {
            isp_4dg0_clip_value_write(be_reg, dyna_reg_cfg->clip_value0);
            isp_4dg1_clip_value_write(be_reg, dyna_reg_cfg->clip_value1);

            dyna_reg_cfg->resh = offline_mode;
        }

        reg_cfg_info->cfg_key.bit1_wdr_dg_cfg = offline_mode ? 1 : ((blk_num <= (i + 1)) ? 0 : 1);
    }

    return TD_SUCCESS;
}

static td_void isp_pre_be_blc_static_reg_config(ot_vi_pipe vi_pipe, td_u8 i, isp_be_blc_static_cfg *static_cfg)
{
    isp_pre_be_reg_type *be_reg = TD_NULL;

    be_reg = (isp_pre_be_reg_type *)isp_get_pre_be_vir_addr(vi_pipe, i);
    isp_check_pointer_void_return(be_reg);

    /* 4Dg */
    isp_4dg_en_in_write(be_reg, static_cfg->wdr_dg_blc[0].blc_in);
    isp_4dg_en_out_write(be_reg, static_cfg->wdr_dg_blc[0].blc_out);

    /* ge */
    isp_ge0_blc_offset_en_write(be_reg, static_cfg->wdr_dg_blc[0].blc_in);

    /* WDR */
    isp_wdr_bsaveblc_write(be_reg, static_cfg->wdr_blc[0].blc_out);
}

static td_void isp_post_be_blc_static_reg_config(ot_vi_pipe vi_pipe, td_u8 i, isp_be_blc_static_cfg *static_cfg)
{
    isp_post_be_reg_type *be_reg = TD_NULL;

    be_reg = (isp_post_be_reg_type *)isp_get_post_be_vir_addr(vi_pipe, i);
    isp_check_pointer_void_return(be_reg);

    /* lsc */
    isp_lsc_blc_in_en_write(be_reg, static_cfg->lsc_blc.blc_in);
    isp_lsc_blc_out_en_write(be_reg, static_cfg->lsc_blc.blc_out);

    /* Dg */
    isp_dg_en_in_write(be_reg, static_cfg->dg_blc.blc_in);
    isp_dg_en_out_write(be_reg, static_cfg->dg_blc.blc_out);

    /* AE */
    isp_ae_blc_en_write(be_reg, static_cfg->ae_blc.blc_in);
    isp_ae_offset_r_write(be_reg, 0);
    isp_ae_offset_gr_write(be_reg, 0);
    isp_ae_offset_gb_write(be_reg, 0);
    isp_ae_offset_b_write(be_reg, 0);

    /* MG */
    isp_la_blc_en_write(be_reg, static_cfg->mg_blc.blc_in);
    isp_la_offset_r_write(be_reg, 0);
    isp_la_offset_gr_write(be_reg, 0);
    isp_la_offset_gb_write(be_reg, 0);
    isp_la_offset_b_write(be_reg, 0);

    /* WB */
    isp_wb_en_in_write(be_reg, static_cfg->wb_blc.blc_in);
    isp_wb_en_out_write(be_reg, static_cfg->wb_blc.blc_out);
    /* AF */
    isp_af_offset_en_write(be_reg, static_cfg->af_blc.blc_in);
    isp_af_offset_gr_write(be_reg, 0);
    isp_af_offset_gb_write(be_reg, 0);
}

static td_void isp_4dg_blc_dyna_reg_config(isp_pre_be_reg_type *be_reg, isp_be_blc_dyna_cfg *dyna_cfg)
{
    isp_4dg0_ofsr_write(be_reg, dyna_cfg->wdr_dg_blc[0].blc[0]);  /* index 0 ,0 */
    isp_4dg0_ofsgr_write(be_reg, dyna_cfg->wdr_dg_blc[0].blc[1]); /* index 0 ,1 */
    isp_4dg0_ofsgb_write(be_reg, dyna_cfg->wdr_dg_blc[0].blc[2]); /* index 0 ,2 */
    isp_4dg0_ofsb_write(be_reg, dyna_cfg->wdr_dg_blc[0].blc[3]);  /* index 0 ,3 */

    isp_4dg1_ofsr_write(be_reg, dyna_cfg->wdr_dg_blc[1].blc[0]);  /* index 1 ,0 */
    isp_4dg1_ofsgr_write(be_reg, dyna_cfg->wdr_dg_blc[1].blc[1]); /* index 1 ,1 */
    isp_4dg1_ofsgb_write(be_reg, dyna_cfg->wdr_dg_blc[1].blc[2]); /* index 1 ,2 */
    isp_4dg1_ofsb_write(be_reg, dyna_cfg->wdr_dg_blc[1].blc[3]);  /* index 1 ,3 */
}

static td_void isp_ge_blc_dyna_reg_config(isp_pre_be_reg_type *be_reg, isp_be_blc_dyna_cfg *dyna_cfg)
{
    isp_ge0_blc_offset_gr_write(be_reg, dyna_cfg->ge_blc.blc[1]);     /* index 0 ,1 */
    isp_ge0_blc_offset_gb_write(be_reg, dyna_cfg->ge_blc.blc[2]);     /* index 0 ,2 */
}

static td_void isp_wdr_blc_dyna_reg_config(isp_pre_be_reg_type *be_reg, isp_be_blc_dyna_cfg *dyna_cfg)
{
    isp_wdr_f0_inblc_r_write(be_reg, dyna_cfg->wdr_blc[0].blc[0]);  /* index 0 ,0 */
    isp_wdr_f0_inblc_gr_write(be_reg, dyna_cfg->wdr_blc[0].blc[1]); /* index 0 ,1 */
    isp_wdr_f0_inblc_gb_write(be_reg, dyna_cfg->wdr_blc[0].blc[2]); /* index 0 ,2 */
    isp_wdr_f0_inblc_b_write(be_reg, dyna_cfg->wdr_blc[0].blc[3]);  /* index 0 ,3 */

    isp_wdr_f1_inblc_r_write(be_reg, dyna_cfg->wdr_blc[1].blc[0]);  /* index 1 ,0 */
    isp_wdr_f1_inblc_gr_write(be_reg, dyna_cfg->wdr_blc[1].blc[1]); /* index 1 ,1 */
    isp_wdr_f1_inblc_gb_write(be_reg, dyna_cfg->wdr_blc[1].blc[2]); /* index 1 ,2 */
    isp_wdr_f1_inblc_b_write(be_reg, dyna_cfg->wdr_blc[1].blc[3]);  /* index 1 ,3 */

    isp_wdr_outblc_write(be_reg, dyna_cfg->wdr_blc[0].out_blc);  /* index 0 */
}

static td_void isp_pre_be_blc_dyna_reg_config(ot_vi_pipe vi_pipe, td_u8 i, isp_be_blc_dyna_cfg *dyna_cfg)
{
    isp_pre_be_reg_type *be_reg = TD_NULL;

    be_reg = (isp_pre_be_reg_type *)isp_get_pre_be_vir_addr(vi_pipe, i);
    isp_check_pointer_void_return(be_reg);

    isp_4dg_blc_dyna_reg_config(be_reg, dyna_cfg);     /* 4Dg */
    isp_ge_blc_dyna_reg_config(be_reg, dyna_cfg);      /* ge */
    isp_wdr_blc_dyna_reg_config(be_reg, dyna_cfg);     /* WDR */
}

static td_void isp_post_be_blc_dyna_reg_config(ot_vi_pipe vi_pipe, td_u8 i, isp_be_blc_dyna_cfg *dyna_cfg)
{
    isp_post_be_reg_type *be_reg = TD_NULL;

    be_reg = (isp_post_be_reg_type *)isp_get_post_be_vir_addr(vi_pipe, i);
    isp_check_pointer_void_return(be_reg);

    /* lsc */
    isp_lsc_blc_r_write(be_reg, dyna_cfg->lsc_blc.blc[0]);  /* array index 0 */
    isp_lsc_blc_gr_write(be_reg, dyna_cfg->lsc_blc.blc[1]); /* array index 1 */
    isp_lsc_blc_gb_write(be_reg, dyna_cfg->lsc_blc.blc[2]); /* array index 2 */
    isp_lsc_blc_b_write(be_reg, dyna_cfg->lsc_blc.blc[3]);  /* array index 3 */

    /* Dg */
    isp_dg_ofsr_write(be_reg, dyna_cfg->dg_blc.blc[0]);  /* array index 0 */
    isp_dg_ofsgr_write(be_reg, dyna_cfg->dg_blc.blc[1]); /* array index 1 */
    isp_dg_ofsgb_write(be_reg, dyna_cfg->dg_blc.blc[2]); /* array index 2 */
    isp_dg_ofsb_write(be_reg, dyna_cfg->dg_blc.blc[3]);  /* array index 3 */

    /* WB */
    isp_wb_ofsr_write(be_reg, dyna_cfg->wb_blc.blc[0]);  /* array index 0 */
    isp_wb_ofsgr_write(be_reg, dyna_cfg->wb_blc.blc[1]); /* array index 1 */
    isp_wb_ofsgb_write(be_reg, dyna_cfg->wb_blc.blc[2]); /* array index 2 */
    isp_wb_ofsb_write(be_reg, dyna_cfg->wb_blc.blc[3]);  /* array index 3 */
}

static td_void isp_be_blc_bit_shift_reg_config(ot_vi_pipe vi_pipe, isp_reg_cfg *reg_cfg_info, td_u8 i)
{
    ot_unused(reg_cfg_info);
    td_bool wdr_en = reg_cfg_info->alg_reg_cfg[i].wdr_reg_cfg.wdr_en;
    isp_pre_be_reg_type *be_reg = TD_NULL;

    be_reg = (isp_pre_be_reg_type *)isp_get_pre_be_vir_addr(vi_pipe, i);
    isp_check_pointer_void_return(be_reg);

    if (wdr_en == TD_TRUE) {
        isp_dg_blc_offset_shift_write(be_reg, ISP_BLACK_LEVEL_RIGHT_SHIFT_BIT_WDR);
        isp_lsc_blc_right_shift_write(be_reg, ISP_BLACK_LEVEL_RIGHT_SHIFT_BIT_WDR);
    } else {
        isp_dg_blc_offset_shift_write(be_reg, 0);
        isp_lsc_blc_right_shift_write(be_reg, 0);
    }
}

static td_s32 isp_be_blc_reg_config(ot_vi_pipe vi_pipe, isp_reg_cfg *reg_cfg_info, td_u8 i)
{
    td_bool offline_mode;
    td_u8 blk_num = reg_cfg_info->cfg_num;
    isp_be_blc_static_cfg *static_cfg = TD_NULL;
    isp_be_blc_dyna_cfg *dyna_cfg = TD_NULL;
    isp_usr_ctx *isp_ctx = TD_NULL;

    isp_get_ctx(vi_pipe, isp_ctx);
    offline_mode =
        (is_offline_mode(isp_ctx->block_attr.running_mode) || is_striping_mode(isp_ctx->block_attr.running_mode) ||
        is_pre_online_post_offline(isp_ctx->block_attr.running_mode));

    if (reg_cfg_info->cfg_key.bit1_be_blc_cfg) {
        static_cfg = &reg_cfg_info->alg_reg_cfg[i].be_blc_cfg.static_blc;
        dyna_cfg = &reg_cfg_info->alg_reg_cfg[i].be_blc_cfg.dyna_blc;

        if (static_cfg->resh_static) {
            isp_pre_be_blc_static_reg_config(vi_pipe, i, static_cfg);
            isp_post_be_blc_static_reg_config(vi_pipe, i, static_cfg);
            static_cfg->resh_static = offline_mode;
        }

        isp_be_blc_bit_shift_reg_config(vi_pipe, reg_cfg_info, i);

        if (reg_cfg_info->alg_reg_cfg[i].be_blc_cfg.resh_dyna_init == TD_TRUE) {
            isp_pre_be_blc_dyna_reg_config(vi_pipe, i, dyna_cfg);
            isp_post_be_blc_dyna_reg_config(vi_pipe, i, dyna_cfg);
            reg_cfg_info->alg_reg_cfg[i].be_blc_cfg.resh_dyna_init = TD_FALSE;
        }

        reg_cfg_info->cfg_key.bit1_be_blc_cfg = offline_mode ? 1 : ((blk_num <= (i + 1)) ? 0 : 1);
    }

    return TD_SUCCESS;
}

static td_void isp_expander_lut_reg_config(const isp_usr_ctx *isp_ctx, isp_pre_be_reg_type *be_reg,
    isp_expander_usr_cfg *usr_reg_cfg)
{
    td_u16 j;

    if (is_online_mode(isp_ctx->block_attr.running_mode) ||
        is_pre_online_post_offline(isp_ctx->block_attr.running_mode)) {
        isp_expander_lut_waddr_write(be_reg, 0);

        for (j = 0; j < OT_ISP_EXPANDER_REG_NODE_NUM; j++) {
            isp_expander_lut_wdata_write(be_reg, usr_reg_cfg->lut[j]);
        }
    } else {
        isp_expander_lut_write(&be_reg->be_lut.be_apb_lut, usr_reg_cfg->lut);
    }
}

static td_s32 isp_expander_reg_config(ot_vi_pipe vi_pipe, isp_reg_cfg *reg_cfg_info, td_u8 i)
{
    td_bool offline_mode;
    td_bool usr_resh, idx_resh;
    td_u8 blk_num = reg_cfg_info->cfg_num;

    isp_expander_usr_cfg *usr_reg_cfg = TD_NULL;
    isp_pre_be_reg_type *be_reg = TD_NULL;
    isp_usr_ctx *isp_ctx = TD_NULL;

    isp_get_ctx(vi_pipe, isp_ctx);

    offline_mode = (is_offline_mode(isp_ctx->block_attr.running_mode) ||
                    is_striping_mode(isp_ctx->block_attr.running_mode));

    if (reg_cfg_info->cfg_key.bit1_expander_cfg) {
        be_reg = (isp_pre_be_reg_type *)isp_get_pre_be_vir_addr(vi_pipe, i);
        isp_check_pointer_return(be_reg);

        usr_reg_cfg = &reg_cfg_info->alg_reg_cfg[i].expander_cfg.usr_cfg;

        idx_resh = (isp_expander_update_index_read(be_reg) != usr_reg_cfg->update_index);
        usr_resh = (offline_mode) ? (usr_reg_cfg->resh && idx_resh) : (usr_reg_cfg->resh);

        if (usr_resh) {
            isp_expander_update_index_write(be_reg, usr_reg_cfg->update_index);
            isp_expander_bitw_out_write(be_reg, usr_reg_cfg->bit_depth_out);
            isp_expander_bitw_in_write(be_reg, usr_reg_cfg->bit_depth_in);

            isp_expander_lut_reg_config(isp_ctx, be_reg, usr_reg_cfg);

            usr_reg_cfg->resh = offline_mode;
        }

        reg_cfg_info->cfg_key.bit1_expander_cfg = offline_mode ? 1 : ((blk_num <= (i + 1)) ? 0 : 1);
    }

    return TD_SUCCESS;
}

static td_s32 isp_fe_update_reg_config(ot_vi_pipe vi_pipe, isp_reg_cfg *reg_cfg_info)
{
    td_u32 i;
    ot_vi_pipe vi_pipe_bind;
    isp_usr_ctx *isp_ctx = TD_NULL;
    isp_fe_reg_type *fe_reg = TD_NULL;

    isp_get_ctx(vi_pipe, isp_ctx);

    for (i = 0; i < isp_ctx->wdr_attr.pipe_num; i++) {
        vi_pipe_bind = isp_ctx->wdr_attr.pipe_id[i];
        isp_check_no_fe_pipe_return(vi_pipe_bind);
        isp_dist_trans_pipe(&vi_pipe_bind);

        fe_reg = (isp_fe_reg_type *)isp_get_fe_vir_addr(vi_pipe_bind);
        isp_check_pointer_return(fe_reg);

        isp_fe_update_mode_write(fe_reg, TD_FALSE);
        isp_fe_update_write(fe_reg, TD_TRUE);

        if (reg_cfg_info->alg_reg_cfg[0].fe_lut_update_cfg.ae1_lut_update) {
            isp_fe_ae_lut_update_write(fe_reg, reg_cfg_info->alg_reg_cfg[0].fe_lut_update_cfg.ae1_lut_update);
        }
    }

    return TD_SUCCESS;
}

static td_void isp_fe_size_reg_config(const isp_usr_ctx *isp_ctx, isp_fe_reg_type *fe_reg)
{
    td_bool isp_crop_en;
    td_s32 x, y;
    td_u32 width, height, pipe_w, pipe_h;
    td_u32 merge_frame;

    x = isp_ctx->sys_rect.x;
    y = isp_ctx->sys_rect.y;
    width = isp_ctx->sys_rect.width;
    height = isp_ctx->sys_rect.height;
    pipe_w = isp_ctx->pipe_size.width;
    pipe_h = isp_ctx->pipe_size.height;

    /* ISP crop low-power process */
    if ((x == 0) && (y == 0) && (width == pipe_w) && (height == pipe_h)) {
        isp_crop_en = TD_FALSE;
    } else {
        isp_crop_en = TD_TRUE;
    }

    isp_fe_crop_pos_write(fe_reg, x, y);
    isp_fe_crop_size_out_write(fe_reg, width - 1, height - 1);

    if (isp_ctx->stagger_attr.stagger_en == TD_TRUE) {
        if (isp_ctx->stagger_attr.crop_info.enable == TD_TRUE) {
            isp_fe_crop_en_write(fe_reg, TD_TRUE);
            isp_fe_crop_pos_write(fe_reg, isp_ctx->stagger_attr.crop_info.rect.x,
                                  isp_ctx->stagger_attr.crop_info.rect.y);
            isp_fe_crop_size_out_write(fe_reg, isp_ctx->stagger_attr.crop_info.rect.width - 1,
                                       isp_ctx->stagger_attr.crop_info.rect.height - 1);
        } else {
            isp_fe_crop_en_write(fe_reg, TD_FALSE);
        }
        merge_frame = div_0_to_1(isp_ctx->stagger_attr.merge_frame_num);
        isp_mul_u32_limit(width, pipe_w, merge_frame);
        height = (pipe_h + (merge_frame - 1)) / div_0_to_1(merge_frame);
        isp_fe_size_write(fe_reg, width - 1, height - 1);
        isp_fe_blk_size_write(fe_reg, width - 1, height - 1);
    } else {
        isp_fe_crop_en_write(fe_reg, isp_crop_en);
        isp_fe_size_write(fe_reg, pipe_w - 1, pipe_h - 1);
        isp_fe_blk_size_write(fe_reg, pipe_w - 1, pipe_h - 1);
    }

    isp_fe_delay_write(fe_reg, height >> 1); /* set fe delay interrup trigger threshold */
}

static td_s32 isp_fe_system_reg_config(ot_vi_pipe vi_pipe)
{
    td_u8 rggb_cfg;
    td_u32 i;
    ot_vi_pipe vi_pipe_bind;
    isp_usr_ctx *isp_ctx = TD_NULL;
    isp_fe_reg_type *fe_reg = TD_NULL;

    isp_get_ctx(vi_pipe, isp_ctx);

    rggb_cfg = ot_ext_system_rggb_cfg_read(vi_pipe);

    for (i = 0; i < isp_ctx->wdr_attr.pipe_num; i++) {
        vi_pipe_bind = isp_ctx->wdr_attr.pipe_id[i];
        isp_check_no_fe_pipe_return(vi_pipe_bind);
        isp_dist_trans_pipe(&vi_pipe_bind);
        fe_reg = (isp_fe_reg_type *)isp_get_fe_vir_addr(vi_pipe_bind);
        isp_check_pointer_return(fe_reg);

        /* ISP FE/BE Set Offline Mode */
        /* isp regs uptate mode:   0: update; 1:frame */
        isp_fe_rggb_cfg_write(fe_reg, rggb_cfg);
        isp_fe_fix_timing_write(fe_reg, OT_ISP_FE_FIX_TIMING_STAT);
        isp_fe_blk_f_hblank_write(fe_reg, 0);
        isp_fe_hsync_mode_write(fe_reg, 0);
        isp_fe_vsync_mode_write(fe_reg, 0);

        isp_fe_size_reg_config(isp_ctx, fe_reg);
    }

    return TD_SUCCESS;
}

static td_s32 isp_reg_default(ot_vi_pipe vi_pipe, const isp_reg_cfg *reg_cfg_info, td_u8 i)
{
    isp_post_be_reg_type *post_be_reg = TD_NULL;
    isp_pre_be_reg_type *pre_be_reg = TD_NULL;
    isp_viproc_reg_type *post_viproc = TD_NULL;

    ot_unused(reg_cfg_info);

    post_be_reg = (isp_post_be_reg_type *)isp_get_post_be_vir_addr(vi_pipe, i);
    pre_be_reg = (isp_pre_be_reg_type *)isp_get_pre_be_vir_addr(vi_pipe, i);
    post_viproc = (isp_viproc_reg_type *)isp_get_post_vi_proc_vir_addr(vi_pipe, i);
    isp_check_pointer_return(post_be_reg);
    isp_check_pointer_return(pre_be_reg);
    isp_check_pointer_return(post_viproc);

    /* pre be */
    isp_clip_y_min_write(pre_be_reg, ISP_CLIP_Y_MIN_DEFAULT);
    isp_clip_y_max_write(pre_be_reg, ISP_CLIP_Y_MAX_DEFAULT);
    isp_clip_c_min_write(pre_be_reg, ISP_CLIP_C_MIN_DEFAULT);
    isp_clip_c_max_write(pre_be_reg, ISP_CLIP_C_MAX_DEFAULT);
    isp_blk_f_hblank_write(pre_be_reg, OT_ISP_BLK_F_HBLANK_DEFAULT);
    isp_blk_f_vblank_write(pre_be_reg, OT_ISP_BLK_F_VBLANK_DEFAULT);
    isp_blk_b_hblank_write(pre_be_reg, OT_ISP_BLK_B_HBLANK_DEFAULT);
    isp_blk_b_vblank_write(pre_be_reg, OT_ISP_BLK_B_VBLANK_DEFAULT);

    /* post be */
    isp_clip_y_min_write(post_be_reg, ISP_CLIP_Y_MIN_DEFAULT);
    isp_clip_y_max_write(post_be_reg, ISP_CLIP_Y_MAX_DEFAULT);
    isp_clip_c_min_write(post_be_reg, ISP_CLIP_C_MIN_DEFAULT);
    isp_clip_c_max_write(post_be_reg, ISP_CLIP_C_MAX_DEFAULT);
    isp_blk_f_hblank_write(post_be_reg, OT_ISP_BLK_F_HBLANK_DEFAULT);
    isp_blk_f_vblank_write(post_be_reg, OT_ISP_BLK_F_VBLANK_DEFAULT);
    isp_blk_b_hblank_write(post_be_reg, OT_ISP_BLK_B_HBLANK_DEFAULT);
    isp_blk_b_vblank_write(post_be_reg, OT_ISP_BLK_B_VBLANK_DEFAULT);

    isp_blc_en_write(post_viproc, TD_FALSE);
    isp_split_en_write(post_viproc, TD_FALSE);

    return TD_SUCCESS;
}

static td_s32 isp_system_reg_config(ot_vi_pipe vi_pipe, td_u8 i)
{
    td_u32 rggb_cfg;
    isp_usr_ctx *isp_ctx = TD_NULL;
    isp_viproc_reg_type *post_viproc = TD_NULL;
    isp_viproc_reg_type *pre_viproc = TD_NULL;
    isp_post_be_reg_type *post_be_reg = TD_NULL;
    isp_pre_be_reg_type *pre_be_reg = TD_NULL;

    isp_get_ctx(vi_pipe, isp_ctx);

    rggb_cfg = ot_ext_system_rggb_cfg_read(vi_pipe);

    post_be_reg = (isp_post_be_reg_type *)isp_get_post_be_vir_addr(vi_pipe, i);
    pre_be_reg = (isp_pre_be_reg_type *)isp_get_pre_be_vir_addr(vi_pipe, i);
    post_viproc = (isp_viproc_reg_type *)isp_get_post_vi_proc_vir_addr(vi_pipe, i);
    pre_viproc = (isp_viproc_reg_type *)isp_get_pre_vi_proc_vir_addr(vi_pipe, i);
    isp_check_pointer_return(post_be_reg);
    isp_check_pointer_return(pre_be_reg);
    isp_check_pointer_return(post_viproc);
    isp_check_pointer_return(pre_viproc);

    isp_be_rggb_cfg_write(post_viproc, rggb_cfg);
    isp_be_rggb_cfg_write(pre_viproc, rggb_cfg);

    if ((is_offline_mode(isp_ctx->block_attr.running_mode)) || (is_striping_mode(isp_ctx->block_attr.running_mode))) {
        isp_be_stt_en_write(post_be_reg, TD_TRUE);
        isp_be_stt_en_write(pre_be_reg, TD_TRUE);
#ifdef CONFIG_OT_VI_PREONLINE_POSTOFFLINE
    } else if (is_pre_online_post_offline(isp_ctx->block_attr.running_mode)) {
        isp_be_stt_en_write(post_be_reg, TD_TRUE);
        isp_be_stt_en_write(pre_be_reg, TD_FALSE); // todo pre_online_post_offline
#endif
    } else {
        isp_be_stt_en_write(post_be_reg, TD_FALSE);
        isp_be_stt_en_write(pre_be_reg, TD_FALSE);
    }

    isp_sumy_en_write(pre_viproc, TD_TRUE);

    isp_sumy_en_write(post_viproc, TD_TRUE);

    isp_yuv422_sum_en_write(post_be_reg, TD_TRUE);
    isp_yuv422_sum_en_write(pre_be_reg, TD_TRUE);

    isp_mem_cfg_write(pre_be_reg, isp_ctx->block_attr.online_ex_en);
    isp_mem_cfg_write(post_be_reg, isp_ctx->block_attr.online_ex_en);

    return TD_SUCCESS;
}

static td_s32 isp_dither_reg_config(ot_vi_pipe vi_pipe, const isp_reg_cfg *reg_cfg_info, td_u8 i)
{
    isp_usr_ctx *isp_ctx = TD_NULL;
    isp_post_be_reg_type *be_reg = TD_NULL;

    isp_get_ctx(vi_pipe, isp_ctx);
    be_reg = (isp_post_be_reg_type *)isp_get_post_be_vir_addr(vi_pipe, i);
    isp_check_pointer_return(be_reg);
#ifdef ISP_BINARY_CONSISTENCY
    ot_unused(reg_cfg_info);
    ot_unused(isp_ctx);
    isp_drc_dither_en_write(be_reg, TD_FALSE);
    isp_dmnr_dither_en_write(be_reg, TD_FALSE);
    isp_sharpen_dither_en_write(be_reg, TD_FALSE);
#else
    /* after drc module */
    if (reg_cfg_info->alg_reg_cfg[i].drc_reg_cfg.enable == TD_TRUE) {
        isp_drc_dither_en_write(be_reg, TD_FALSE);
    } else {
        isp_drc_dither_en_write(be_reg, !(isp_ctx->hdr_attr.dynamic_range == OT_DYNAMIC_RANGE_XDR));
    }
    isp_drc_dither_out_bits_write(be_reg, ISP_DRC_DITHER_OUT_BITS_DEFAULT);
    isp_drc_dither_round_write(be_reg, ISP_DRC_DITHER_ROUND_DEFAULT);
    isp_drc_dither_spatial_mode_write(be_reg, ISP_DRC_DITHER_SPATIAL_MODE_DEFAULT);

    /* after gamma module */
    isp_dmnr_dither_en_write(be_reg, TD_TRUE);
    isp_dmnr_dither_out_bits_write(be_reg, ISP_DMNR_DITHER_OUT_BITS_DEFAULT);
    isp_dmnr_dither_round_write(be_reg, ISP_DMNR_DITHER_ROUND_DEFAULT);
    isp_dmnr_dither_spatial_mode_write(be_reg, ISP_DMNR_DITHER_SPATIAL_MODE_DEFAULT);

    isp_sharpen_ldci_dither_round_write(be_reg, ISP_SHARPEN_LDCI_DITHER_ROUND_DEFAULT);
#endif

    return TD_SUCCESS;
}

static td_s32 isp_fe_stt_reg_config(ot_vi_pipe vi_pipe, isp_reg_cfg *reg_cfg_info)
{
    td_u16 num_h, num_v;
    td_u32 k;
    td_bool stt_enable = TD_TRUE;
    ot_vi_pipe vi_pipe_bind;
    isp_usr_ctx *isp_ctx = TD_NULL;
    isp_fe_reg_type *fe_reg = TD_NULL;

    isp_get_ctx(vi_pipe, isp_ctx);

    for (k = 0; k < isp_ctx->wdr_attr.pipe_num; k++) {
        vi_pipe_bind = isp_ctx->wdr_attr.pipe_id[k];
        isp_check_no_fe_pipe_return(vi_pipe_bind);
        isp_dist_trans_pipe(&vi_pipe_bind);

        fe_reg = (isp_fe_reg_type *)isp_get_fe_vir_addr(vi_pipe_bind);
        isp_check_pointer_return(fe_reg);

        if (isp_ctx->special_opt.fe_stt_update) {
            isp_fe_ae_stt_en_write(fe_reg, stt_enable);
            isp_fe_ae_stt_bst_write(fe_reg, 0xF);
        }

        /* ae */
        num_h = reg_cfg_info->alg_reg_cfg[0].ae_reg_cfg.dyna_reg_cfg.fe_weight_table_width;
        num_v = reg_cfg_info->alg_reg_cfg[0].ae_reg_cfg.dyna_reg_cfg.fe_weight_table_height;
        isp_fe_ae_stt_size_write(fe_reg, (num_h * num_v + 3) / 4); /* plus 3 divide 4 is byte align */
    }

    isp_ctx->special_opt.fe_stt_update = TD_FALSE;

    return TD_SUCCESS;
}

static td_void isp_ae_stt_reg_config(ot_vi_pipe vi_pipe, isp_reg_cfg *reg_cfg_info, td_u8 i)
{
    td_u16 num_h, num_v;
    isp_ae_dyna_cfg *dyna_reg_cfg = &reg_cfg_info->alg_reg_cfg[i].ae_reg_cfg.dyna_reg_cfg;
    isp_be_reg_type *be_reg = TD_NULL;

    if (dyna_reg_cfg->is_in_pre_be == TD_TRUE) {
        be_reg = (isp_be_reg_type *)isp_get_pre_be_vir_addr(vi_pipe, i);
    } else {
        be_reg = (isp_be_reg_type *)isp_get_post_be_vir_addr(vi_pipe, i);
    }

    isp_check_pointer_void_return(be_reg);

    isp_ae_stt_en_write(be_reg, TD_TRUE);
    isp_ae_stt_bst_write(be_reg, 0xF);
    num_h = dyna_reg_cfg->be_weight_table_width;
    num_v = dyna_reg_cfg->be_weight_table_height;
    isp_ae_stt_size_write(be_reg, (num_h * num_v + 1) / 2); /* plus 1 divide 2 is byte align */
}

static td_void isp_awb_stt_reg_config(ot_vi_pipe vi_pipe, isp_reg_cfg *reg_cfg_info, td_u8 i)
{
    td_u16 num_h, num_v;
    isp_be_reg_type *be_reg = TD_NULL;

    if (reg_cfg_info->alg_reg_cfg[i].awb_reg_cfg.awb_reg_dyn_cfg.is_in_pre_be == TD_TRUE) {
        be_reg = (isp_be_reg_type *)isp_get_pre_be_vir_addr(vi_pipe, i);
    } else {
        be_reg = (isp_be_reg_type *)isp_get_post_be_vir_addr(vi_pipe, i);
    }

    isp_check_pointer_void_return(be_reg);

    isp_awb_stt_en_write(be_reg, TD_TRUE);
    isp_awb_stt_bst_write(be_reg, 0xF);
    num_h = reg_cfg_info->alg_reg_cfg[i].awb_reg_cfg.awb_reg_usr_cfg.be_zone_col;
    num_v = reg_cfg_info->alg_reg_cfg[i].awb_reg_cfg.awb_reg_usr_cfg.be_zone_row;
    isp_awb_stt_size_write(be_reg, (num_h * num_v * 2 + 1) / 2); /* 2 plus 1 divide 2 is byte align */
}

static td_void isp_af_stt_reg_config(ot_vi_pipe vi_pipe, isp_reg_cfg *reg_cfg_info, td_u8 i)
{
    td_u16 num_h, num_v;
    isp_be_reg_type *be_reg = TD_NULL;

    if (reg_cfg_info->alg_reg_cfg[i].be_af_reg_cfg.is_in_pre_be == TD_TRUE) {
        be_reg = (isp_be_reg_type *)isp_get_pre_be_vir_addr(vi_pipe, i);
    } else {
        be_reg = (isp_be_reg_type *)isp_get_post_be_vir_addr(vi_pipe, i);
    }

    isp_check_pointer_void_return(be_reg);

    isp_af_stt_en_write(be_reg, TD_TRUE);
    isp_af_stt_bst_write(be_reg, 0xF);
    num_h = reg_cfg_info->alg_reg_cfg[i].be_af_reg_cfg.window_hnum;
    num_v = reg_cfg_info->alg_reg_cfg[i].be_af_reg_cfg.window_vnum;
    isp_af_stt_size_write(be_reg, (num_h * num_v * 2)); /* plus 1 divide 2 is byte align */
}

static td_void isp_stt_enable_reg_config(isp_post_be_reg_type *post_be_reg)
{
    td_bool stt_enable = TD_TRUE;

    isp_dehaze_stt_en_write(post_be_reg, stt_enable);
    isp_ldci_lpfstt_en_write(post_be_reg, stt_enable);

    isp_dehaze_stt_bst_write(post_be_reg, 0xF);
    isp_ldci_lpfstt_bst_write(post_be_reg, 0xF);
}

static td_s32 isp_stt_reg_config(ot_vi_pipe vi_pipe, isp_reg_cfg *reg_cfg_info, td_u8 i)
{
    td_u16 num_h, num_v;
    isp_usr_ctx *isp_ctx = TD_NULL;
    isp_post_be_reg_type *post_be_reg = TD_NULL;

    isp_get_ctx(vi_pipe, isp_ctx);

    isp_ae_stt_reg_config(vi_pipe, reg_cfg_info, i);
    isp_awb_stt_reg_config(vi_pipe, reg_cfg_info, i);
    isp_af_stt_reg_config(vi_pipe, reg_cfg_info, i);

    post_be_reg = (isp_post_be_reg_type *)isp_get_post_be_vir_addr(vi_pipe, i);
    isp_check_pointer_return(post_be_reg);

    if (isp_ctx->special_opt.be_on_stt_update[i]) {
        isp_stt_enable_reg_config(post_be_reg);

        isp_ctx->special_opt.be_on_stt_update[i] = TD_FALSE;
    }

    /* dehaze */
    num_h = reg_cfg_info->alg_reg_cfg[i].dehaze_reg_cfg.dyna_reg_cfg.statnum_h;
    num_v = reg_cfg_info->alg_reg_cfg[i].dehaze_reg_cfg.dyna_reg_cfg.statnum_v;
    isp_dehaze_stt_size_write(post_be_reg, (((num_h + 1) * (num_v + 1)) + 1) / 2); /* plus 1 divide 2 is byte align */

    /* Ldci */
    num_h = (td_u16)reg_cfg_info->alg_reg_cfg[i].ldci_reg_cfg.dyna_reg_cfg.calc_sml_map_stride;
    num_v = (td_u16)reg_cfg_info->alg_reg_cfg[i].ldci_reg_cfg.dyna_reg_cfg.calc_sml_map_height;
    isp_ldci_lpfstt_size_write(post_be_reg, (num_h * num_v + 1) / 2); /* plus 1 divide 2 is byte align */

    return TD_SUCCESS;
}

static td_s32 isp_be_alg_lut2stt_reg_new_reg_config(ot_vi_pipe vi_pipe, isp_reg_cfg *reg_cfg_info, td_u8 i)
{
    isp_be_stt2lut_regnew_reg_cfg *be_stt2lut_regnew_cfg = &reg_cfg_info->alg_reg_cfg[i].stt2lut_regnew_cfg;
    isp_post_be_reg_type *post_be_reg = TD_NULL;
    isp_pre_be_reg_type *pre_be_reg = TD_NULL;
    isp_usr_ctx *isp_ctx = TD_NULL;
    isp_get_ctx(vi_pipe, isp_ctx);

    post_be_reg = (isp_post_be_reg_type *)isp_get_post_be_vir_addr(vi_pipe, i);
    pre_be_reg = (isp_pre_be_reg_type *)isp_get_pre_be_vir_addr(vi_pipe, i);
    isp_check_pointer_return(post_be_reg);
    isp_check_pointer_return(pre_be_reg);

    if (be_stt2lut_regnew_cfg->gamma_stt2lut_regnew) {
        isp_gamma_stt2lut_regnew_write(post_be_reg, TD_TRUE);
    }

    if ((isp_ctx->frame_cnt < STT_LUT_CONFIG_TIMES) || be_stt2lut_regnew_cfg->csp_stt2lut_regnew) {
        isp_cc_stt2lut_regnew_write(post_be_reg, TD_TRUE);
    }

    if ((isp_ctx->frame_cnt < STT_LUT_CONFIG_TIMES) || be_stt2lut_regnew_cfg->lsc_stt2lut_regnew) {
        isp_mlsc_stt2lut_regnew_write(post_be_reg, TD_TRUE);
    }

    if ((isp_ctx->frame_cnt < STT_LUT_CONFIG_TIMES) || be_stt2lut_regnew_cfg->ca_stt2lut_regnew) {
        isp_ca_stt2lut_regnew_write(post_be_reg, TD_TRUE);
    }

    return TD_SUCCESS;
}

static td_s32 isp_be_alg_lut_update_reg_config(ot_vi_pipe vi_pipe, isp_reg_cfg *reg_cfg_info, td_u8 i)
{
    isp_be_lut_update_reg_cfg *be_lut_update_cfg = &reg_cfg_info->alg_reg_cfg[i].be_lut_update_cfg;
    isp_post_be_reg_type *post_be_reg = TD_NULL;

    post_be_reg = (isp_post_be_reg_type *)isp_get_post_be_vir_addr(vi_pipe, i);
    isp_check_pointer_return(post_be_reg);

    if (be_lut_update_cfg->ae_lut_update) {
        isp_ae_lut_update_write(post_be_reg, be_lut_update_cfg->ae_lut_update);
    }

    if (be_lut_update_cfg->drc_tm_lut_update) {
        isp_drc_lut_update0_write(post_be_reg, be_lut_update_cfg->drc_tm_lut_update);
    }

    return TD_SUCCESS;
}

static td_void isp_be_cur_enable_reg_config(ot_vi_pipe vi_pipe, isp_alg_reg_cfg *alg_reg_cfg, td_u8 i)
{
    isp_viproc_reg_type *post_viproc = TD_NULL;
    isp_viproc_reg_type *pre_viproc = TD_NULL;
    isp_post_be_reg_type *post_be_reg = TD_NULL;
    isp_pre_be_reg_type *pre_be_reg = TD_NULL;
    post_viproc = (isp_viproc_reg_type *)isp_get_post_vi_proc_vir_addr(vi_pipe, i);
    pre_viproc = (isp_viproc_reg_type *)isp_get_pre_vi_proc_vir_addr(vi_pipe, i);
    post_be_reg = (isp_post_be_reg_type *)isp_get_post_be_vir_addr(vi_pipe, i);
    pre_be_reg = (isp_pre_be_reg_type *)isp_get_pre_be_vir_addr(vi_pipe, i);
    if ((post_viproc == TD_NULL) || (pre_viproc == TD_NULL) || (post_be_reg == TD_NULL) || (pre_be_reg == TD_NULL)) {
        return;
    }

    /* pre be */
    isp_dpc_en_write(pre_viproc, TD_TRUE);
    if (alg_reg_cfg->dp_reg_cfg.dpc_en[0] == TD_FALSE) {
        isp_dpc_mode_write(pre_be_reg, alg_reg_cfg->dp_reg_cfg.dyna_reg_cfg.dpcc_mode[0] & 0xfffc);
    }
    isp_ge_en_write(pre_viproc, alg_reg_cfg->ge_reg_cfg.ge_en[0]);         /* array index 0 */
    isp_expander_en_write(pre_viproc, alg_reg_cfg->expander_cfg.enable);
    isp_wb_en_write(pre_viproc, alg_reg_cfg->awb_reg_cfg.awb_reg_dyn_cfg.be_wb_work_en);
    isp_lsc_en_write(pre_viproc, alg_reg_cfg->lsc_reg_cfg.lsc_en);

    /* post be */
    /* viproc part */
    isp_cc_en_write(post_viproc, alg_reg_cfg->awb_reg_cfg.awb_reg_dyn_cfg.be_cc_en);
    isp_sharpen_en_write(post_viproc, alg_reg_cfg->sharpen_reg_cfg.enable);
    isp_dmnr_vhdm_en_write(post_viproc, TD_TRUE);
    isp_dmnr_nddm_en_write(post_viproc, TD_TRUE);
    isp_gamma_en_write(post_viproc, alg_reg_cfg->gamma_cfg.gamma_en);
    isp_csc_en_write(post_viproc, alg_reg_cfg->csc_cfg.enable);
    isp_ca_en_write(post_viproc, alg_reg_cfg->ca_reg_cfg.ca_en);
    isp_dehaze_en_write(post_viproc, alg_reg_cfg->dehaze_reg_cfg.dehaze_en);

    /* be part */
    isp_cc_colortone_en_write(post_be_reg, alg_reg_cfg->awb_reg_cfg.awb_reg_dyn_cfg.be_cc_colortone_en);
    isp_cc_lutb_en_write(post_be_reg, alg_reg_cfg->drc_reg_cfg.usr_reg_cfg.csp_en);
    isp_demosaic_gcac_en_write(post_be_reg, alg_reg_cfg->cac_reg_cfg.cac_en);
    isp_demosaic_fcr_en_write(post_be_reg, alg_reg_cfg->anti_false_color_reg_cfg.fcr_enable);
    isp_demosaic_inner_bypass_en_write(post_be_reg,
        (alg_reg_cfg->dem_reg_cfg.inner_enable == TD_TRUE ? TD_FALSE : TD_TRUE));
}

static td_s32 isp_be_cur_reg_config(ot_vi_pipe vi_pipe, isp_reg_cfg *reg_cfg_info, td_u8 i)
{
    isp_alg_reg_cfg *alg_reg_cfg = &reg_cfg_info->alg_reg_cfg[i];
    isp_post_be_reg_type *post_be_reg = TD_NULL;

    post_be_reg = (isp_post_be_reg_type *)isp_get_post_be_vir_addr(vi_pipe, i);

    isp_check_pointer_return(post_be_reg);

    /* module enable */
    isp_be_cur_enable_reg_config(vi_pipe, alg_reg_cfg, i);

    /* ldci */
    isp_ldci_blc_ctrl_write(post_be_reg, alg_reg_cfg->ldci_reg_cfg.dyna_reg_cfg.calc_blc_ctrl);
    isp_ldci_lpfcoef0_write(post_be_reg, alg_reg_cfg->ldci_reg_cfg.dyna_reg_cfg.lpf_coef[0]); /* array index 0 */
    isp_ldci_lpfcoef1_write(post_be_reg, alg_reg_cfg->ldci_reg_cfg.dyna_reg_cfg.lpf_coef[1]); /* array index 1 */
    isp_ldci_lpfcoef2_write(post_be_reg, alg_reg_cfg->ldci_reg_cfg.dyna_reg_cfg.lpf_coef[2]); /* array index 2 */
    isp_ldci_lpfcoef3_write(post_be_reg, alg_reg_cfg->ldci_reg_cfg.dyna_reg_cfg.lpf_coef[3]); /* array index 3 */
    isp_ldci_lpfcoef4_write(post_be_reg, alg_reg_cfg->ldci_reg_cfg.dyna_reg_cfg.lpf_coef[4]); /* array index 4 */

    /* lsc */
    if (alg_reg_cfg->lsc_reg_cfg.usr_reg_cfg.lut_update) {
        isp_lsc_mesh_scale_write(post_be_reg, alg_reg_cfg->lsc_reg_cfg.usr_reg_cfg.mesh_scale);
        isp_mlsc_mesh_scale_write(post_be_reg, alg_reg_cfg->lsc_reg_cfg.usr_reg_cfg.mesh_scale);
    }

    /* ca */
    isp_ca_isoratio_write(post_be_reg, alg_reg_cfg->ca_reg_cfg.dyna_reg_cfg.ca_iso_ratio);

    /* dehaze */
    isp_dehaze_air_r_write(post_be_reg, alg_reg_cfg->dehaze_reg_cfg.dyna_reg_cfg.air_r);
    isp_dehaze_air_g_write(post_be_reg, alg_reg_cfg->dehaze_reg_cfg.dyna_reg_cfg.air_g);
    isp_dehaze_air_b_write(post_be_reg, alg_reg_cfg->dehaze_reg_cfg.dyna_reg_cfg.air_b);
    isp_dehaze_gstrth_write(post_be_reg, alg_reg_cfg->dehaze_reg_cfg.dyna_reg_cfg.strength);

    /* sharpen */
    isp_sharpen_dyna_reg_config(post_be_reg, &alg_reg_cfg->sharpen_reg_cfg.dyna_reg_cfg.mpi_dyna_reg_cfg,
        &alg_reg_cfg->sharpen_reg_cfg.dyna_reg_cfg.default_dyna_reg_cfg);

    return TD_SUCCESS;
}

static td_void isp_pre_be_last_reg_config_enable(isp_viproc_reg_type *pre_viproc, isp_pre_be_reg_type *pre_be_reg,
    isp_lut2stt_sync_reg_cfg *lut2stt_sync_cfg)
{
    isp_dpc_en_write(pre_viproc, TD_TRUE);       /* array index 0 */
    isp_ge_en_write(pre_viproc, lut2stt_sync_cfg->ge_en[0]);         /* array index 0 */
    isp_expander_en_write(pre_viproc, lut2stt_sync_cfg->expander_en);
    isp_wb_en_write(pre_viproc, lut2stt_sync_cfg->wb_en);
}

static td_void isp_be_last_reg_config_enable(ot_vi_pipe vi_pipe, isp_lut2stt_sync_reg_cfg *lut2stt_sync_cfg, td_u8 i)
{
    isp_post_be_reg_type *post_be_reg = TD_NULL;
    isp_pre_be_reg_type *pre_be_reg = TD_NULL;
    isp_viproc_reg_type *post_viproc = TD_NULL;
    isp_viproc_reg_type *pre_viproc = TD_NULL;
    post_be_reg = (isp_post_be_reg_type *)isp_get_post_be_vir_addr(vi_pipe, i);
    pre_be_reg = (isp_pre_be_reg_type *)isp_get_pre_be_vir_addr(vi_pipe, i);
    post_viproc = (isp_viproc_reg_type *)isp_get_post_vi_proc_vir_addr(vi_pipe, i);
    pre_viproc = (isp_viproc_reg_type *)isp_get_pre_vi_proc_vir_addr(vi_pipe, i);
    isp_check_pointer_void_return(post_be_reg);
    isp_check_pointer_void_return(pre_be_reg);
    isp_check_pointer_void_return(post_viproc);
    isp_check_pointer_void_return(pre_viproc);

    /* pre be */
    isp_pre_be_last_reg_config_enable(pre_viproc, pre_be_reg, lut2stt_sync_cfg);

    /* post be */
    /* viproc part */
    isp_cc_en_write(post_viproc, lut2stt_sync_cfg->ccm_en);
    isp_sharpen_en_write(post_viproc, lut2stt_sync_cfg->sharpen_en);
    isp_dmnr_vhdm_en_write(post_viproc, TD_TRUE);
    isp_dmnr_nddm_en_write(post_viproc, TD_TRUE);
    isp_lsc_en_write(post_viproc, lut2stt_sync_cfg->lsc_en);
    isp_gamma_en_write(post_viproc, lut2stt_sync_cfg->gamma_en);
    isp_csc_en_write(post_viproc, lut2stt_sync_cfg->csc_en);
    isp_ca_en_write(post_viproc, lut2stt_sync_cfg->ca_en);
    isp_dehaze_en_write(post_viproc, lut2stt_sync_cfg->dehaze_en);
    isp_clut_en_write(post_viproc, lut2stt_sync_cfg->clut_en);
    /* be part */
    isp_cc_colortone_en_write(post_be_reg, lut2stt_sync_cfg->ccm_color_tone_en);
    isp_cc_lutb_en_write(post_be_reg, lut2stt_sync_cfg->csp_en);
    isp_demosaic_gcac_en_write(post_be_reg, lut2stt_sync_cfg->cac_en);
    isp_demosaic_fcr_en_write(post_be_reg, lut2stt_sync_cfg->fcr_en);
    isp_demosaic_inner_bypass_en_write(post_be_reg,
        (lut2stt_sync_cfg->dm_inner_enable == TD_TRUE ? TD_FALSE : TD_TRUE));
}

static td_s32 isp_be_last_reg_config(ot_vi_pipe vi_pipe, isp_reg_cfg *reg_cfg_info, td_u8 i)
{
    isp_lut2stt_sync_reg_cfg *lut2stt_sync_cfg = &reg_cfg_info->alg_reg_cfg[i].lut2stt_sync_cfg[0];
    isp_post_be_reg_type *post_be_reg = TD_NULL;

    post_be_reg = (isp_post_be_reg_type *)isp_get_post_be_vir_addr(vi_pipe, i);
    isp_check_pointer_return(post_be_reg);

    /* module enable */
    isp_be_last_reg_config_enable(vi_pipe, lut2stt_sync_cfg, i);

    if (lut2stt_sync_cfg->dpc_en[0] == TD_FALSE) {
        isp_dpc_mode_write(post_be_reg, reg_cfg_info->alg_reg_cfg->dp_reg_cfg.dyna_reg_cfg.dpcc_mode[0] & 0xfffc);
    }

    /* ldci */
    isp_ldci_blc_ctrl_write(post_be_reg, lut2stt_sync_cfg->ldci_sync_cfg.calc_blc_ctrl);
    isp_ldci_lpfcoef0_write(post_be_reg, lut2stt_sync_cfg->ldci_sync_cfg.lpf_coef[0]); /* array index 0 */
    isp_ldci_lpfcoef1_write(post_be_reg, lut2stt_sync_cfg->ldci_sync_cfg.lpf_coef[1]); /* array index 1 */
    isp_ldci_lpfcoef2_write(post_be_reg, lut2stt_sync_cfg->ldci_sync_cfg.lpf_coef[2]); /* array index 2 */
    isp_ldci_lpfcoef3_write(post_be_reg, lut2stt_sync_cfg->ldci_sync_cfg.lpf_coef[3]); /* array index 3 */
    isp_ldci_lpfcoef4_write(post_be_reg, lut2stt_sync_cfg->ldci_sync_cfg.lpf_coef[4]); /* array index 4 */

    /* lsc */
    if (lut2stt_sync_cfg->lsc_sync_cfg.resh) {
        isp_lsc_mesh_scale_write(post_be_reg, lut2stt_sync_cfg->lsc_sync_cfg.mesh_scale);
        isp_mlsc_mesh_scale_write(post_be_reg, lut2stt_sync_cfg->lsc_sync_cfg.mesh_scale);
    }
    /* ca */
    isp_ca_isoratio_write(post_be_reg, lut2stt_sync_cfg->ca_sync_cfg.ca_iso_ratio);
    /* dehaze */
    if (reg_cfg_info->alg_reg_cfg[i].dehaze_reg_cfg.lut2_stt_en == TD_TRUE) {
        isp_dehaze_air_r_write(post_be_reg, lut2stt_sync_cfg->dehaze_sync_cfg.air_r);
        isp_dehaze_air_g_write(post_be_reg, lut2stt_sync_cfg->dehaze_sync_cfg.air_g);
        isp_dehaze_air_b_write(post_be_reg, lut2stt_sync_cfg->dehaze_sync_cfg.air_b);
        isp_dehaze_gstrth_write(post_be_reg, lut2stt_sync_cfg->dehaze_sync_cfg.strength);
    }
    /* sharpen */
    isp_sharpen_dyna_reg_config(post_be_reg, &lut2stt_sync_cfg->sharpen_sync_cfg.mpi_dyna_reg_cfg,
        &lut2stt_sync_cfg->sharpen_sync_cfg.default_dyna_reg_cfg);

    return TD_SUCCESS;
}

static td_s32 isp_be_alg_sync_reg_config(ot_vi_pipe vi_pipe, isp_reg_cfg *reg_cfg_info, td_u8 i)
{
    isp_usr_ctx *isp_ctx = TD_NULL;
    td_s32 ret;
    isp_get_ctx(vi_pipe, isp_ctx);

    if (is_offline_mode(isp_ctx->block_attr.running_mode) || is_striping_mode(isp_ctx->block_attr.running_mode)) {
        ret = isp_be_cur_reg_config(vi_pipe, reg_cfg_info, i);
        if (ret != TD_SUCCESS) {
            isp_err_trace("Pipe:%d isp_be_cur_reg_config failed!\n", vi_pipe);
        }
    } else {
        ret = isp_be_last_reg_config(vi_pipe, reg_cfg_info, i);
        if (ret != TD_SUCCESS) {
            isp_err_trace("Pipe:%d isp_be_last_reg_config failed!\n", vi_pipe);
        }
    }

    return TD_SUCCESS;
}

static td_void isp_save_be_sync_enable_reg(isp_lut2stt_sync_reg_cfg *lut2stt_sync_cfg, isp_alg_reg_cfg *alg_reg_cfg)
{
    td_u8 j;
    lut2stt_sync_cfg->ae_en = alg_reg_cfg->ae_reg_cfg.static_reg_cfg.be_enable;
    lut2stt_sync_cfg->awb_en = alg_reg_cfg->awb_reg_cfg.awb_reg_sta_cfg.be_awb_work_en;
    lut2stt_sync_cfg->wb_en = alg_reg_cfg->awb_reg_cfg.awb_reg_dyn_cfg.be_wb_work_en;
    lut2stt_sync_cfg->awblsc_en = alg_reg_cfg->awblsc_reg_cfg.awblsc_reg_sta_cfg.awb_work_en;
    lut2stt_sync_cfg->ccm_en = alg_reg_cfg->awb_reg_cfg.awb_reg_dyn_cfg.be_cc_en;
    lut2stt_sync_cfg->ccm_color_tone_en = alg_reg_cfg->awb_reg_cfg.awb_reg_dyn_cfg.be_cc_colortone_en;
    lut2stt_sync_cfg->af_en = alg_reg_cfg->be_af_reg_cfg.af_enable;
    lut2stt_sync_cfg->sharpen_en = alg_reg_cfg->sharpen_reg_cfg.enable;
    lut2stt_sync_cfg->print_sel = 0; // alg_reg_cfg->sharpen_reg_cfg.dyna_reg_cfg.mpi_dyna_reg_cfg.print_sel;
    lut2stt_sync_cfg->vhdm_en = alg_reg_cfg->dem_reg_cfg.vhdm_enable;
    lut2stt_sync_cfg->nddm_en = alg_reg_cfg->dem_reg_cfg.nddm_enable;
    lut2stt_sync_cfg->dm_inner_enable = alg_reg_cfg->dem_reg_cfg.inner_enable;
    lut2stt_sync_cfg->ldci_en = alg_reg_cfg->ldci_reg_cfg.dyna_reg_cfg.enable;
    lut2stt_sync_cfg->cac_en = alg_reg_cfg->cac_reg_cfg.cac_en;
    lut2stt_sync_cfg->bshp_en = 0; // alg_reg_cfg->bshp_reg_cfg.bshp_enable;
    lut2stt_sync_cfg->fcr_en = alg_reg_cfg->anti_false_color_reg_cfg.fcr_enable;
    lut2stt_sync_cfg->lsc_en = alg_reg_cfg->lsc_reg_cfg.lsc_en;
    lut2stt_sync_cfg->gamma_en = alg_reg_cfg->gamma_cfg.gamma_en;
    lut2stt_sync_cfg->csc_en = alg_reg_cfg->csc_cfg.enable;
    lut2stt_sync_cfg->ca_en = alg_reg_cfg->ca_reg_cfg.ca_en;
    lut2stt_sync_cfg->ca_cp_en = alg_reg_cfg->ca_reg_cfg.usr_reg_cfg.ca_cp_en;
    lut2stt_sync_cfg->ca_sync_cfg.ca_iso_ratio      = alg_reg_cfg->ca_reg_cfg.dyna_reg_cfg.ca_iso_ratio;
    lut2stt_sync_cfg->wdr_en = alg_reg_cfg->wdr_reg_cfg.wdr_en;
    lut2stt_sync_cfg->drc_en = alg_reg_cfg->drc_reg_cfg.enable;
    lut2stt_sync_cfg->dehaze_en = alg_reg_cfg->dehaze_reg_cfg.dehaze_en;
    lut2stt_sync_cfg->bnr_lsc_en = 0; // alg_reg_cfg->bnr_reg_cfg.dyna_reg_cfg.bnr_lsc_en;
    lut2stt_sync_cfg->dg_en = alg_reg_cfg->dg_reg_cfg.dg_en;
    lut2stt_sync_cfg->four_dg_en = alg_reg_cfg->four_dg_reg_cfg.enable;
    lut2stt_sync_cfg->pregamma_en = 0; // alg_reg_cfg->pregamma_reg_cfg.enable;
    lut2stt_sync_cfg->clut_en = 0; // alg_reg_cfg->clut_cfg.enable;
    lut2stt_sync_cfg->expander_en = alg_reg_cfg->expander_cfg.enable;
    lut2stt_sync_cfg->csp_en = alg_reg_cfg->drc_reg_cfg.usr_reg_cfg.csp_en;

    for (j = 0; j < ISP_DPC_MAX_CHN_NUM; j++) { /*  dpc channel number is 2 */
        lut2stt_sync_cfg->dpc_en[j] = alg_reg_cfg->dp_reg_cfg.dpc_en[j];
    }

    for (j = 0; j < 4; j++) { /*  ge channel number is 4 */
        lut2stt_sync_cfg->ge_en[j] = alg_reg_cfg->ge_reg_cfg.ge_en[j];
    }
}

static td_s32 isp_save_be_sync_reg(ot_vi_pipe vi_pipe, isp_reg_cfg *reg_cfg_info, td_u8 i)
{
    isp_usr_ctx *isp_ctx = TD_NULL;
    isp_lut2stt_sync_reg_cfg *lut2stt_sync_cfg = &reg_cfg_info->alg_reg_cfg[i].lut2stt_sync_cfg[0];
    isp_alg_reg_cfg *alg_reg_cfg = &reg_cfg_info->alg_reg_cfg[i];

    isp_get_ctx(vi_pipe, isp_ctx);

    if (is_offline_mode(isp_ctx->block_attr.running_mode) || is_striping_mode(isp_ctx->block_attr.running_mode)) {
        return TD_SUCCESS;
    }

    isp_save_be_sync_enable_reg(lut2stt_sync_cfg, alg_reg_cfg);

    lut2stt_sync_cfg->lsc_sync_cfg.resh = alg_reg_cfg->be_lut_update_cfg.lsc_lut_update;
    lut2stt_sync_cfg->lsc_sync_cfg.mesh_scale = alg_reg_cfg->lsc_reg_cfg.usr_reg_cfg.mesh_scale;

    lut2stt_sync_cfg->dehaze_sync_cfg.air_r = alg_reg_cfg->dehaze_reg_cfg.dyna_reg_cfg.air_r;
    lut2stt_sync_cfg->dehaze_sync_cfg.air_g = alg_reg_cfg->dehaze_reg_cfg.dyna_reg_cfg.air_g;
    lut2stt_sync_cfg->dehaze_sync_cfg.air_b = alg_reg_cfg->dehaze_reg_cfg.dyna_reg_cfg.air_b;
    lut2stt_sync_cfg->dehaze_sync_cfg.strength = alg_reg_cfg->dehaze_reg_cfg.dyna_reg_cfg.strength;

    lut2stt_sync_cfg->ldci_sync_cfg.calc_blc_ctrl = alg_reg_cfg->ldci_reg_cfg.dyna_reg_cfg.calc_blc_ctrl;
    (td_void)memcpy_s(lut2stt_sync_cfg->ldci_sync_cfg.lpf_coef, sizeof(td_u32) * OT_ISP_LDCI_LPF_LUT_SIZE,
        alg_reg_cfg->ldci_reg_cfg.dyna_reg_cfg.lpf_coef, sizeof(td_u32) * OT_ISP_LDCI_LPF_LUT_SIZE);

    (td_void)memcpy_s(&lut2stt_sync_cfg->sharpen_sync_cfg.mpi_dyna_reg_cfg, sizeof(isp_sharpen_mpi_dyna_reg_cfg),
        &alg_reg_cfg->sharpen_reg_cfg.dyna_reg_cfg.mpi_dyna_reg_cfg, sizeof(isp_sharpen_mpi_dyna_reg_cfg));
    (td_void)memcpy_s(
        &lut2stt_sync_cfg->sharpen_sync_cfg.default_dyna_reg_cfg, sizeof(isp_sharpen_default_dyna_reg_cfg),
        &alg_reg_cfg->sharpen_reg_cfg.dyna_reg_cfg.default_dyna_reg_cfg, sizeof(isp_sharpen_default_dyna_reg_cfg));

    return TD_SUCCESS;
}

static td_void isp_be_reah_cfg_raw_domain1(isp_alg_reg_cfg *alg_reg_cfg)
{
    alg_reg_cfg->dp_reg_cfg.static_reg_cfg.static_resh = TD_TRUE;
    alg_reg_cfg->dp_reg_cfg.usr_reg_cfg.usr_dyna_cor_reg_cfg.resh = TD_TRUE;
    alg_reg_cfg->dp_reg_cfg.usr_reg_cfg.usr_sta_cor_reg_cfg.resh = TD_TRUE;
    alg_reg_cfg->dp_reg_cfg.dyna_reg_cfg.resh = TD_TRUE;

    alg_reg_cfg->dp_reg_cfg.static_reg_cfg.static_resh1 = TD_TRUE;
    alg_reg_cfg->dp_reg_cfg.usr_reg_cfg.usr_dyna_cor_reg_cfg.resh1 = TD_TRUE;
    alg_reg_cfg->dp_reg_cfg.usr_reg_cfg.usr_sta_cor_reg_cfg.resh1 = TD_TRUE;
    alg_reg_cfg->dp_reg_cfg.dyna_reg_cfg.resh1 = TD_TRUE;

    alg_reg_cfg->ge_reg_cfg.static_reg_cfg.static_resh = TD_TRUE;
    alg_reg_cfg->ge_reg_cfg.usr_reg_cfg.resh = TD_TRUE;
    alg_reg_cfg->ge_reg_cfg.dyna_reg_cfg.resh = TD_TRUE;

    alg_reg_cfg->four_dg_reg_cfg.static_reg_cfg.resh = TD_TRUE;
    alg_reg_cfg->four_dg_reg_cfg.dyna_reg_cfg.resh = TD_TRUE;

    alg_reg_cfg->wdr_reg_cfg.static_reg_cfg.resh = TD_TRUE;
    alg_reg_cfg->wdr_reg_cfg.usr_reg_cfg.resh = TD_TRUE;
    alg_reg_cfg->wdr_reg_cfg.dyna_reg_cfg.resh = TD_TRUE;
}

static td_void isp_be_reah_cfg_raw_domain2(isp_alg_reg_cfg *alg_reg_cfg)
{
    alg_reg_cfg->expander_cfg.usr_cfg.resh = TD_TRUE;
    alg_reg_cfg->bnr_reg_cfg.static_reg_cfg.resh = TD_TRUE;
    alg_reg_cfg->bnr_reg_cfg.dyna_reg_cfg.resh = TD_TRUE;
    alg_reg_cfg->bnr_reg_cfg.usr_reg_cfg.resh = TD_TRUE;
    alg_reg_cfg->lsc_reg_cfg.static_reg_cfg.static_resh = TD_TRUE;
    alg_reg_cfg->lsc_reg_cfg.usr_reg_cfg.coef_update = TD_TRUE;
    alg_reg_cfg->lsc_reg_cfg.usr_reg_cfg.lut_update = TD_TRUE;

    alg_reg_cfg->dg_reg_cfg.static_reg_cfg.resh = TD_TRUE;
    alg_reg_cfg->dg_reg_cfg.dyna_reg_cfg.resh = TD_TRUE;

    alg_reg_cfg->be_blc_cfg.static_blc.resh_static = TD_TRUE;
    alg_reg_cfg->be_blc_cfg.resh_dyna_init = TD_TRUE;

    alg_reg_cfg->awb_reg_cfg.awb_reg_sta_cfg.be_awb_sta_cfg = TD_TRUE;
    alg_reg_cfg->awb_reg_cfg.awb_reg_usr_cfg.resh = TD_TRUE;

    alg_reg_cfg->drc_reg_cfg.static_reg_cfg.resh = TD_TRUE;
    alg_reg_cfg->drc_reg_cfg.usr_reg_cfg.resh = TD_TRUE;
    alg_reg_cfg->drc_reg_cfg.dyna_reg_cfg.resh = TD_TRUE;

    alg_reg_cfg->dem_reg_cfg.static_reg_cfg.resh = TD_TRUE;
    alg_reg_cfg->dem_reg_cfg.dyna_reg_cfg.resh = TD_TRUE;

    alg_reg_cfg->cac_reg_cfg.static_reg_cfg.static_resh = TD_TRUE;
    alg_reg_cfg->cac_reg_cfg.usr_reg_cfg.usr_resh = TD_TRUE;
    alg_reg_cfg->cac_reg_cfg.dyna_reg_cfg.dyna_resh = TD_TRUE;

    alg_reg_cfg->anti_false_color_reg_cfg.static_reg_cfg.resh = TD_TRUE;
    alg_reg_cfg->anti_false_color_reg_cfg.dyna_reg_cfg.resh = TD_TRUE;
}

static td_void isp_be_reah_cfg_rgb_domain(isp_alg_reg_cfg *alg_reg_cfg)
{
    alg_reg_cfg->gamma_cfg.usr_reg_cfg.gamma_lut_update_en = TD_TRUE;
    alg_reg_cfg->csc_cfg.dyna_reg_cfg.resh = TD_TRUE;
    alg_reg_cfg->dehaze_reg_cfg.static_reg_cfg.resh = TD_TRUE;
    alg_reg_cfg->dehaze_reg_cfg.dyna_reg_cfg.lut_update = 1;
}

static td_void isp_be_reah_cfg_yuv_domain(isp_alg_reg_cfg *alg_reg_cfg)
{
    alg_reg_cfg->ldci_reg_cfg.static_reg_cfg.static_resh = TD_TRUE;
    alg_reg_cfg->ca_reg_cfg.static_reg_cfg.static_resh = TD_TRUE;
    alg_reg_cfg->ca_reg_cfg.dyna_reg_cfg.resh = TD_TRUE;
    alg_reg_cfg->ca_reg_cfg.usr_reg_cfg.resh = TD_TRUE;
    alg_reg_cfg->ca_reg_cfg.usr_reg_cfg.ca_lut_update_en = TD_TRUE;

    alg_reg_cfg->sharpen_reg_cfg.static_reg_cfg.static_resh = TD_TRUE;
    alg_reg_cfg->sharpen_reg_cfg.dyna_reg_cfg.default_dyna_reg_cfg.resh = TD_TRUE;
    alg_reg_cfg->sharpen_reg_cfg.dyna_reg_cfg.mpi_dyna_reg_cfg.resh = TD_TRUE;

    alg_reg_cfg->mcds_reg_cfg.static_reg_cfg.static_resh = TD_TRUE;
    alg_reg_cfg->mcds_reg_cfg.dyna_reg_cfg.dyna_resh = TD_TRUE;
}

static td_void isp_be_resh_cfg(isp_alg_reg_cfg *alg_reg_cfg)
{
    isp_be_reah_cfg_raw_domain1(alg_reg_cfg);

    isp_be_reah_cfg_raw_domain2(alg_reg_cfg);

    isp_be_reah_cfg_rgb_domain(alg_reg_cfg);

    isp_be_reah_cfg_yuv_domain(alg_reg_cfg);
}

td_s32 isp_reset_fe_stt_en(ot_vi_pipe vi_pipe)
{
    td_u8 i;
    ot_vi_pipe vi_pipe_bind;
    isp_usr_ctx *isp_ctx = TD_NULL;
    isp_fe_reg_type *fe_reg = TD_NULL;

    isp_get_ctx(vi_pipe, isp_ctx);

    if (isp_ctx->wdr_attr.is_mast_pipe) {
        for (i = 0; i < isp_ctx->wdr_attr.pipe_num; i++) {
            vi_pipe_bind = isp_ctx->wdr_attr.pipe_id[i];
            isp_check_no_fe_pipe_return(vi_pipe_bind);
            isp_dist_trans_pipe(&vi_pipe_bind);

            fe_reg = (isp_fe_reg_type *)isp_get_fe_vir_addr(vi_pipe_bind);
            isp_check_pointer_return(fe_reg);

            isp_fe_ae_stt_en_write(fe_reg, TD_FALSE);
            isp_fe_update_write(fe_reg, TD_TRUE);
        }
    }

    return TD_SUCCESS;
}

static td_void isp_fe_alg_en_exit(isp_usr_ctx *isp_ctx)
{
    td_u8 i;
    ot_vi_pipe vi_pipe_bind;
    isp_fe_reg_type *fe_reg = TD_NULL;

    if (isp_ctx->wdr_attr.is_mast_pipe != TD_TRUE) {
        return;
    }

    for (i = 0; i < isp_ctx->wdr_attr.pipe_num; i++) {
        vi_pipe_bind = isp_ctx->wdr_attr.pipe_id[i];

        if ((vi_pipe_bind < 0) || (vi_pipe_bind >= OT_ISP_MAX_FE_PIPE_NUM)) {
            continue;
        }

        fe_reg = (isp_fe_reg_type *)isp_get_fe_vir_addr(vi_pipe_bind);
        isp_check_pointer_void_return(fe_reg);

        isp_fe_ae1_en_write(fe_reg, TD_FALSE);
        isp_fe_wb_en_write(fe_reg, TD_FALSE);
        isp_fe_dg2_en_write(fe_reg, TD_FALSE);
        isp_fe_af1_en_write(fe_reg, TD_FALSE);
        isp_fe_ae_stt_en_write(fe_reg, TD_FALSE);
        isp_fe_update_write(fe_reg, TD_TRUE);
    }
}

static td_void isp_disable_tnr_reg_write(isp_post_be_reg_type *post_be_reg, isp_viproc_reg_type *post_viproc)
{
    isp_sharpen_en_write(post_viproc, TD_FALSE);
    isp_sharpen_mot_en_write(post_be_reg, TD_FALSE);

    isp_bnr_en_write(post_viproc, TD_FALSE);
    isp_bnr_ensptnr_write(post_be_reg, TD_FALSE);
}

static td_void isp_disable_bnr_tnr(ot_vi_pipe vi_pipe)
{
    td_s32 ret;
    isp_bnr_temporal_filt tpr_filt;
    tpr_filt.nr_en = TD_FALSE;
    tpr_filt.tnr_en = TD_FALSE;
    ret = ioctl(isp_get_fd(vi_pipe), ISP_DISABLE_BNR_WMOT);
    if (ret != TD_SUCCESS) {
        isp_err_trace("set bnr mot off failure!\n");
    }

    ret = ioctl(isp_get_fd(vi_pipe), ISP_BNR_TEMPORAL_FILT_CFG_SET, &tpr_filt);
    if (ret != TD_SUCCESS) {
        isp_err_trace("ISP[%d] set bnr temporal filer failed %#x!\n", vi_pipe, ret);
    }
}

static td_void isp_be_alg_en_exit(ot_vi_pipe vi_pipe, const isp_usr_ctx *isp_ctx)
{
    td_u8 i;
    isp_post_be_reg_type *post_be_reg = TD_NULL;
    isp_pre_be_reg_type *pre_be_reg = TD_NULL;
    isp_viproc_reg_type *post_viproc = TD_NULL;
    isp_viproc_reg_type *pre_viproc = TD_NULL;

    if (is_offline_mode(isp_ctx->block_attr.running_mode) || is_striping_mode(isp_ctx->block_attr.running_mode)) {
        return;
    }

    for (i = 0; i < isp_ctx->block_attr.block_num; i++) {
        post_be_reg = (isp_post_be_reg_type *)isp_get_post_be_vir_addr(vi_pipe, i);
        pre_be_reg = (isp_pre_be_reg_type *)isp_get_pre_be_vir_addr(vi_pipe, i);
        post_viproc = (isp_viproc_reg_type *)isp_get_post_vi_proc_vir_addr(vi_pipe, i);
        pre_viproc = (isp_viproc_reg_type *)isp_get_pre_vi_proc_vir_addr(vi_pipe, i);
        if ((post_be_reg == TD_NULL) || (pre_be_reg == TD_NULL) || (post_viproc == TD_NULL) ||
            (pre_viproc == TD_NULL)) {
            return;
        }

        /* stat to ddr:disable module_en */
        isp_ae_en_write(post_viproc, TD_FALSE);
        isp_la_en_write(post_viproc, TD_FALSE);
        isp_awb_en_write(post_viproc, TD_FALSE);
        isp_af_en_write(post_viproc, TD_FALSE);
        isp_dehaze_en_write(post_viproc, TD_FALSE);
        isp_ldci_en_write(post_viproc, TD_FALSE);
        isp_flicker_en_write(pre_viproc, TD_FALSE);
        isp_ldci_wrstat_en_write(post_be_reg, TD_FALSE);
        isp_ldci_rdstat_en_write(post_be_reg, TD_FALSE);

        /* lut2stt: disable stt2lut_en  */
        isp_sharpen_stt2lut_en_write(post_be_reg, TD_FALSE);
        isp_ldci_stt2lut_en_write(post_be_reg, TD_FALSE);
        isp_dehaze_stt2lut_en_write(post_be_reg, TD_FALSE);
        isp_gamma_stt2lut_en_write(post_be_reg, TD_FALSE);
        isp_mlsc_stt2lut_en_write(post_be_reg, TD_FALSE);
        isp_bnr_stt2lut_en_write(post_be_reg, TD_FALSE);

        isp_disable_bnr_tnr(vi_pipe);

        isp_disable_tnr_reg_write(post_be_reg, post_viproc);
    }
}

#define ms_to_us(milli_sec) (1000 * (milli_sec))

static td_void isp_alg_en_exit_wait(ot_vi_pipe vi_pipe, const isp_usr_ctx *isp_ctx)
{
    td_u32 milli_sec;
    td_u32 fe_cnt_base, fe_cnt;
    td_u64 time_begin;
    isp_fe_reg_type *fe_reg = TD_NULL;

    if ((isp_ctx->wdr_attr.is_mast_pipe != TD_TRUE) || is_virt_pipe(vi_pipe) || is_no_fe_phy_pipe(vi_pipe)) {
        return;
    }

    fe_reg = (isp_fe_reg_type *)isp_get_fe_vir_addr(vi_pipe);
    isp_check_pointer_void_return(fe_reg);

    milli_sec = (td_u32)(2000 / div_0_to_1_float(isp_ctx->sns_image_mode.fps)); /* 2000:2 * 1000, wait 2 frame */

    fe_cnt_base = fe_reg->isp_fe_startup.bits.isp_fe_fcnt;
    fe_cnt      = fe_cnt_base;
    time_begin  = get_sys_time_by_usec();
    while ((fe_cnt - fe_cnt_base) < 2) { /* 2 for a full frame process */
        fe_cnt = fe_reg->isp_fe_startup.bits.isp_fe_fcnt;
        if ((get_sys_time_by_usec() - time_begin) >= ms_to_us(milli_sec)) {
            return;
        }
#ifdef __LITEOS__
        usleep(1); /* msleep 10 */
#else
        usleep(ms_to_us(10)); /* msleep 10 */
#endif
    }
}

td_s32 isp_alg_en_exit(ot_vi_pipe vi_pipe)
{
    isp_usr_ctx *isp_ctx = TD_NULL;

    isp_get_ctx(vi_pipe, isp_ctx);
    isp_check_pointer_return(isp_ctx);

    if (isp_ctx->para_rec.init == TD_FALSE) {
        return TD_SUCCESS;
    }

    /* FE */
    isp_fe_alg_en_exit(isp_ctx);

    /* BE */
    isp_be_alg_en_exit(vi_pipe, isp_ctx);

    /* wait */
    isp_alg_en_exit_wait(vi_pipe, isp_ctx);

    return TD_SUCCESS;
}

static td_s32 isp_fe_regs_config(ot_vi_pipe vi_pipe, isp_reg_cfg *reg_cfg_info)
{
    td_s64 ret = TD_SUCCESS;
    isp_usr_ctx *isp_ctx = TD_NULL;

    isp_check_no_fe_pipe_return(vi_pipe);
    isp_get_ctx(vi_pipe, isp_ctx);

    if (isp_ctx->wdr_attr.is_mast_pipe) {
        /* FE alg cfgs setting to register */
        ret += isp_fe_ae_reg_config(vi_pipe, reg_cfg_info);  /* Ae */
        ret += isp_fe_awb_reg_config(vi_pipe, reg_cfg_info); /* awb */
        ret += isp_fe_dg_reg_config(vi_pipe, reg_cfg_info);  /* DG */
        ret += isp_fe_blc_reg_config(vi_pipe, reg_cfg_info); /* fe blc */
#ifdef CONFIG_OT_AIBNR_SUPPORT
        ret += isp_fe_agamma_reg_config(vi_pipe, reg_cfg_info);   /* agamma */
#endif
        ret += isp_fe_system_reg_config(vi_pipe);
        ret += isp_fe_stt_reg_config(vi_pipe, reg_cfg_info);

        ret += isp_fe_update_reg_config(vi_pipe, reg_cfg_info);
        if (ret != TD_SUCCESS) {
            isp_err_trace("ISP[%d] isp_fe_regs_config failed!\n", vi_pipe);
            return TD_FAILURE;
        }
    }

    return TD_SUCCESS;
}

static td_void isp_be_drc_param_init(isp_be_drc_sync_param *drc_sync_param, isp_drc_reg_cfg *drc_reg_cfg)
{
    td_u8 j;
    drc_sync_param->shp_log = drc_reg_cfg->static_reg_cfg.shp_log;
    drc_sync_param->div_denom_log = drc_reg_cfg->static_reg_cfg.div_denom_log;
    drc_sync_param->denom_exp = drc_reg_cfg->static_reg_cfg.denom_exp;
#ifdef CONFIG_OT_ISP_DRC_SWITCH_SUPPORT
    drc_sync_param->inner_bypass_en = drc_reg_cfg->sync_reg_cfg.inner_bypass_en;
#endif
    for (j = 0; j < OT_ISP_DRC_EXP_COMP_SAMPLE_NUM; j++) {
        drc_sync_param->prev_luma[j] = drc_reg_cfg->static_reg_cfg.prev_luma[j];
    }
}

static td_void isp_be_wdr_param_init(isp_be_wdr_sync_param *be_sync_param, isp_wdr_reg_cfg *wdr_reg_cfg)
{
    td_u8 j;

    be_sync_param->wdr_exp_ratio = wdr_reg_cfg->static_reg_cfg.expo_r_ratio_lut[0];
    be_sync_param->flick_exp_ratio = wdr_reg_cfg->static_reg_cfg.flick_exp_ratio;

    for (j = 0; j < OT_ISP_WDR_MAX_FRAME_NUM; j++) {
        be_sync_param->wdr_exp_val[j] = wdr_reg_cfg->static_reg_cfg.expo_value_lut[j];
    }

    be_sync_param->wdr_mdt_en = wdr_reg_cfg->dyna_reg_cfg.wdr_mdt_en;
    be_sync_param->fusion_mode = wdr_reg_cfg->usr_reg_cfg.fusion_mode;

    be_sync_param->short_thr = wdr_reg_cfg->dyna_reg_cfg.short_thr;
    be_sync_param->long_thr = wdr_reg_cfg->dyna_reg_cfg.long_thr;
    be_sync_param->wgt_slope = wdr_reg_cfg->dyna_reg_cfg.wgt_slope;

    be_sync_param->wdr_max_ratio = wdr_reg_cfg->static_reg_cfg.max_ratio;
    be_sync_param->fusion_max_ratio = wdr_reg_cfg->static_reg_cfg.max_ratio;
}

static td_s32 isp_be_sync_param_init(ot_vi_pipe vi_pipe, isp_reg_cfg *reg_cfg_info)
{
    td_u8 j;
    td_s32 ret;
    isp_be_sync_para be_sync_param = { 0 };
    isp_alg_reg_cfg *alg_reg_cfg = &reg_cfg_info->alg_reg_cfg[0];

    /* DG */
    be_sync_param.isp_dgain[0] = alg_reg_cfg->dg_reg_cfg.dyna_reg_cfg.gain_r;  /* array 0 assignment */
    be_sync_param.isp_dgain[1] = alg_reg_cfg->dg_reg_cfg.dyna_reg_cfg.gain_gr; /* array 1 assignment */
    be_sync_param.isp_dgain[2] = alg_reg_cfg->dg_reg_cfg.dyna_reg_cfg.gain_gb; /* array 2 assignment */
    be_sync_param.isp_dgain[3] = alg_reg_cfg->dg_reg_cfg.dyna_reg_cfg.gain_b;  /* array 3 assignment */
    for (j = 0; j < OT_ISP_WDR_MAX_FRAME_NUM; j++) {
        be_sync_param.wdr_gain[j] = 0x100;
    }

    /* LDCI */
    be_sync_param.ldci.ldci_comp = 0x1000;

    /* DRC */
    isp_be_drc_param_init(&be_sync_param.drc, &alg_reg_cfg->drc_reg_cfg);

    /* WDR */
    isp_be_wdr_param_init(&be_sync_param.wdr, &alg_reg_cfg->wdr_reg_cfg);

    /* AWB */
    for (j = 0; j < OT_ISP_BAYER_CHN_NUM; j++) {
        be_sync_param.wb_gain[j] = alg_reg_cfg->awb_reg_cfg.awb_reg_dyn_cfg.be_white_balance_gain[j];
    }

    /* blc */
    (td_void)memcpy_s(&be_sync_param.be_blc, sizeof(isp_be_blc_dyna_cfg), &alg_reg_cfg->be_blc_cfg.dyna_blc,
        sizeof(isp_be_blc_dyna_cfg));

    ret = ioctl(isp_get_fd(vi_pipe), ISP_BE_SYNC_PARAM_INIT, &be_sync_param);
    if (ret != TD_SUCCESS) {
        isp_err_trace("ISP[%d] Init BE Sync Param Failed with ec %#x!\n", vi_pipe, ret);
        return ret;
    }

    return TD_SUCCESS;
}

static td_s32 isp_set_be_dump_raw_pre_be(ot_vi_pipe vi_pipe, const isp_usr_ctx *isp_ctx, td_u8 i)
{
    isp_viproc_reg_type *pre_viproc = TD_NULL;
    isp_pre_be_reg_type *pre_be_reg = TD_NULL;

    if ((isp_ctx->be_frame_attr.frame_pos == OT_ISP_DUMP_FRAME_POS_NORMAL)
#ifdef CONFIG_OT_ISP_DUMP_DEBUG_SUPPORT
        && isp_dump_dbg_need_return_pre_be(isp_ctx->be_pos_attr)
#endif
        ) {
        return TD_SUCCESS;
    }

    pre_be_reg = (isp_pre_be_reg_type *)isp_get_pre_be_vir_addr(vi_pipe, i);
    pre_viproc = (isp_viproc_reg_type *)isp_get_pre_vi_proc_vir_addr(vi_pipe, i);

    isp_check_pointer_return(pre_be_reg);
    isp_check_pointer_return(pre_viproc);

#ifdef CONFIG_OT_ISP_DUMP_DEBUG_SUPPORT
    isp_dump_dbg_process_pre_be(vi_pipe, pre_viproc);
#endif
    isp_expander_en_write(pre_viproc, TD_FALSE);
    isp_bcom_en_write(pre_viproc, TD_FALSE);
    isp_bnr_en_write(pre_viproc, TD_FALSE);
    isp_bdec_en_write(pre_viproc, TD_FALSE);
    isp_lsc_en_write(pre_viproc, TD_FALSE);
    isp_dg_en_write(pre_viproc, TD_FALSE);
    isp_ae_en_write(pre_viproc, TD_FALSE);
    isp_awb_en_write(pre_viproc, TD_FALSE);
    isp_af_en_write(pre_viproc, TD_FALSE);
    isp_wb_en_write(pre_viproc, TD_FALSE);

    return TD_SUCCESS;
}

static td_s32 isp_set_be_dump_raw_post_be(ot_vi_pipe vi_pipe, td_u8 i)
{
    isp_viproc_reg_type *post_viproc = TD_NULL;
    isp_post_be_reg_type *post_be_reg = TD_NULL;
    isp_usr_ctx *isp_ctx = TD_NULL;
    isp_get_ctx(vi_pipe, isp_ctx);

    post_be_reg = (isp_post_be_reg_type *)isp_get_post_be_vir_addr(vi_pipe, i);
    post_viproc = (isp_viproc_reg_type *)isp_get_post_vi_proc_vir_addr(vi_pipe, i);

    isp_check_pointer_return(post_be_reg);
    isp_check_pointer_return(post_viproc);
    if (isp_ctx->be_frame_attr.frame_pos == OT_ISP_DUMP_FRAME_POS_AFTER_WDR) {
        isp_la_en_write(post_viproc, TD_FALSE);
        isp_drc_en_write(post_viproc, TD_FALSE);
        isp_drc_dither_en_write(post_be_reg, TD_FALSE);
    }
#ifdef CONFIG_OT_ISP_DUMP_DEBUG_SUPPORT
    isp_dump_dbg_process_post_be(vi_pipe, post_be_reg, post_viproc);
#endif
    isp_dmnr_vhdm_en_write(post_viproc, TD_FALSE);
    isp_dmnr_nddm_en_write(post_viproc, TD_FALSE);
    isp_demosaic_local_cac_en_write(post_be_reg, TD_FALSE);
    isp_demosaic_fcr_en_write(post_be_reg, TD_FALSE);

    return TD_SUCCESS;
}

static td_s32 isp_set_be_dump_raw_post_be_after_demosaic(ot_vi_pipe vi_pipe, td_u8 i)
{
    isp_viproc_reg_type *post_viproc = TD_NULL;
    isp_post_be_reg_type *post_be_reg = TD_NULL;

    post_be_reg = (isp_post_be_reg_type *)isp_get_post_be_vir_addr(vi_pipe, i);
    post_viproc = (isp_viproc_reg_type *)isp_get_post_vi_proc_vir_addr(vi_pipe, i);

    isp_check_pointer_return(post_be_reg);
    isp_check_pointer_return(post_viproc);

    isp_cc_en_write(post_viproc, TD_FALSE);
    isp_cc_colortone_en_write(post_be_reg, TD_FALSE);
    isp_cc_lutb_en_write(post_be_reg, TD_FALSE);
    isp_clut_en_write(post_viproc, TD_FALSE);
    isp_gamma_en_write(post_viproc, TD_FALSE);
    isp_dehaze_en_write(post_viproc, TD_FALSE);
    isp_csc_en_write(post_viproc, TD_FALSE);
    isp_ldci_en_write(post_viproc, TD_FALSE);
    isp_ca_en_write(post_viproc, TD_FALSE);
    isp_hcds_en_write(post_viproc, TD_FALSE);
    isp_sharpen_en_write(post_viproc, TD_FALSE);

    /* dither */
    isp_dmnr_dither_en_write(post_be_reg, TD_FALSE);

    return TD_SUCCESS;
}

static td_s32 isp_set_be_dump_raw(ot_vi_pipe vi_pipe, td_u8 i)
{
    td_s32 ret;
    isp_usr_ctx *isp_ctx = TD_NULL;
    isp_get_ctx(vi_pipe, isp_ctx);

    if (isp_ctx->be_frame_attr.frame_pos == OT_ISP_DUMP_FRAME_POS_NORMAL
#ifdef CONFIG_OT_ISP_DUMP_DEBUG_SUPPORT
        && isp_dump_dbg_is_pos_normal(&isp_ctx->be_pos_attr)
#endif
    ) {
        return TD_SUCCESS;
    }

    ret = isp_set_be_dump_raw_pre_be(vi_pipe, isp_ctx, i);
    if (ret != TD_SUCCESS) {
        return ret;
    }

    ret = isp_set_be_dump_raw_post_be(vi_pipe, i);
    if (ret != TD_SUCCESS) {
        return ret;
    }

    ret = isp_set_be_dump_raw_post_be_after_demosaic(vi_pipe, i);
    if (ret != TD_SUCCESS) {
        return ret;
    }

    return TD_SUCCESS;
}
#ifdef CONFIG_OT_ISP_DUMP_DEBUG_SUPPORT
static td_s32 isp_set_be_dump_dbg_send(ot_vi_pipe vi_pipe, td_u8 i)
{
    td_s32 ret;

    isp_viproc_reg_type *pre_viproc = TD_NULL;
    isp_pre_be_reg_type *pre_be_reg = TD_NULL;
    isp_viproc_reg_type *post_viproc = TD_NULL;
    isp_post_be_reg_type *post_be_reg = TD_NULL;

    pre_be_reg = (isp_pre_be_reg_type *)isp_get_pre_be_vir_addr(vi_pipe, i);
    pre_viproc = (isp_viproc_reg_type *)isp_get_pre_vi_proc_vir_addr(vi_pipe, i);
    post_be_reg = (isp_post_be_reg_type *)isp_get_post_be_vir_addr(vi_pipe, i);
    post_viproc = (isp_viproc_reg_type *)isp_get_post_vi_proc_vir_addr(vi_pipe, i);

    isp_check_pointer_return(pre_be_reg);
    isp_check_pointer_return(pre_viproc);
    isp_check_pointer_return(post_be_reg);
    isp_check_pointer_return(post_viproc);

    ret = isp_dump_dbg_be_send_config(vi_pipe, pre_be_reg, pre_viproc, post_be_reg, post_viproc);

    return ret;
}
#endif

static td_s32 isp_be_alg_reg_config(ot_vi_pipe vi_pipe, isp_reg_cfg *reg_cfg_info, td_u8 i)
{
    td_s64 ret = TD_SUCCESS;

    ret += isp_ae_reg_config(vi_pipe, reg_cfg_info, i);       /* ae, pre or post */
    ret += isp_awb_reg_config(vi_pipe, reg_cfg_info, i);      /* awb, pre or post  */
    ret += isp_af_reg_config(vi_pipe, reg_cfg_info, i);       /* AF, pre or post  */
    ret += isp_sharpen_reg_config(vi_pipe, reg_cfg_info, i);  /* sharpen, post */
    ret += isp_demosaic_reg_config(vi_pipe, reg_cfg_info, i); /* demosaic, post */
    ret += isp_fpn_reg_config(vi_pipe, reg_cfg_info, i);      /* FPN, pre */
    ret += isp_ldci_reg_config(vi_pipe, reg_cfg_info, i);     /* ldci, post */
    ret += isp_cac_reg_config(vi_pipe, reg_cfg_info, i);      /* cac, post */
    ret += isp_fcr_reg_config(vi_pipe, reg_cfg_info, i);      /* FCR, post */
    ret += isp_dpc_reg_config(vi_pipe, reg_cfg_info, i);      /* dpc, pre */
    ret += isp_ge_reg_config(vi_pipe, reg_cfg_info, i);       /* ge, pre */
    ret += isp_lsc_reg_config(vi_pipe, reg_cfg_info, i);      /* BE LSC, pre */
    ret += isp_gamma_reg_config(vi_pipe, reg_cfg_info, i);    /* gamma, post */
    ret += isp_csc_reg_config(vi_pipe, reg_cfg_info, i);      /* csc, post */
    ret += isp_ca_reg_config(vi_pipe, reg_cfg_info, i);       /* ca, post */
    ret += isp_mcds_reg_config(vi_pipe, reg_cfg_info, i);     /* mcds, post */
    ret += isp_wdr_reg_config(vi_pipe, reg_cfg_info, i);      /* wdr, pre */
    ret += isp_drc_reg_config(vi_pipe, reg_cfg_info, i);      /* drc, pre or post */
    ret += isp_dehaze_reg_config(vi_pipe, reg_cfg_info, i);   /* dehaze, post */
    ret += isp_bayer_nr_reg_config(vi_pipe, reg_cfg_info, i); /* BayerNR, pre */
    ret += isp_dg_reg_config(vi_pipe, reg_cfg_info, i);       /* DG, pre */
    ret += isp_4dg_reg_config(vi_pipe, reg_cfg_info, i);      /* 4DG, pre */
    ret += isp_be_blc_reg_config(vi_pipe, reg_cfg_info, i);   /* be blc, most pre */
    ret += isp_expander_reg_config(vi_pipe, reg_cfg_info, i); /* expander, pre */
    ret += isp_csp_reg_config(vi_pipe, reg_cfg_info, i);      /* csp, post */
#ifdef CONFIG_OT_AIBNR_SUPPORT
    ret += isp_adgamma_reg_config(vi_pipe, reg_cfg_info, i);  /* adgamma */
#endif
    if (ret != TD_SUCCESS) {
        return TD_FAILURE;
    }

    return TD_SUCCESS;
}

static td_void isp_be_reg_up_config(ot_vi_pipe vi_pipe, td_u8 i)
{
    isp_viproc_reg_type *pre_viproc = TD_NULL;
    isp_viproc_reg_type *post_viproc = TD_NULL;

    pre_viproc = (isp_viproc_reg_type *)isp_get_pre_vi_proc_vir_addr(vi_pipe, i);
    post_viproc = (isp_viproc_reg_type *)isp_get_post_vi_proc_vir_addr(vi_pipe, i);
    isp_check_pointer_void_return(pre_viproc);
    isp_check_pointer_void_return(post_viproc);

    isp_be_reg_up_write(pre_viproc, TD_TRUE);
    isp_be_reg_up_write(post_viproc, TD_TRUE);
}

static td_s64 isp_be_regs_config_update(ot_vi_pipe vi_pipe, isp_reg_cfg *reg_cfg_info)
{
    td_u8 i;
    td_s64 ret = TD_SUCCESS;

    for (i = 0; i < reg_cfg_info->cfg_num; i++) {
        ret += isp_system_reg_config(vi_pipe, i);     /* sys */
        ret += isp_dither_reg_config(vi_pipe, reg_cfg_info, i);     /* dither */
        ret += isp_stt_reg_config(vi_pipe, reg_cfg_info, i); /* online stt */

        /* Be alg cfgs setting to register */
        ret += isp_be_alg_reg_config(vi_pipe, reg_cfg_info, i);
    }

    for (i = 0; i < reg_cfg_info->cfg_num; i++) {
        ret += isp_be_alg_sync_reg_config(vi_pipe, reg_cfg_info, i);
        ret += isp_be_alg_lut2stt_reg_new_reg_config(vi_pipe, reg_cfg_info, i);
        ret += isp_be_alg_lut_update_reg_config(vi_pipe, reg_cfg_info, i);
    }

    for (i = 0; i < reg_cfg_info->cfg_num; i++) {
        ret += isp_set_be_dump_raw(vi_pipe, i);
#ifdef CONFIG_OT_ISP_DUMP_DEBUG_SUPPORT
        ret += isp_set_be_dump_dbg_send(vi_pipe, i);
#endif
    }

    for (i = 0; i < reg_cfg_info->cfg_num; i++) {
        ret += isp_save_be_sync_reg(vi_pipe, reg_cfg_info, i);
    }

    return ret;
}

static td_s32 isp_be_regs_config(ot_vi_pipe vi_pipe, isp_reg_cfg *reg_cfg_info)
{
    td_s64 ret;
    td_s32 ret1;
    isp_usr_ctx *isp_ctx = TD_NULL;

    isp_get_ctx(vi_pipe, isp_ctx);

    if (isp_ctx->alg_run_select != OT_ISP_ALG_RUN_FE_ONLY) {
        ret = isp_be_regs_config_update(vi_pipe, reg_cfg_info);
        if (ret != TD_SUCCESS) {
            isp_err_trace("ISP[%d] isp_be_regs_config_update failed!\n", vi_pipe);
            return TD_FAILURE;
        }
    }

    if ((is_offline_mode(isp_ctx->block_attr.running_mode)) ||
        (is_striping_mode(isp_ctx->block_attr.running_mode)) ||
        is_pre_online_post_offline(isp_ctx->block_attr.running_mode)) {
        ret1 = isp_cfg_be_buf_ctl(vi_pipe);
        if (ret1 != TD_SUCCESS) {
            isp_err_trace("ISP[%d] Be config bufs ctl failed %x!\n", vi_pipe, ret1);
            return ret1;
        }
    }

    return TD_SUCCESS;
}

static td_s32 isp_be_regs_config_init(ot_vi_pipe vi_pipe, isp_reg_cfg *reg_cfg_info)
{
    td_u8 i;
    td_s64 ret = TD_SUCCESS;

    for (i = 0; i < reg_cfg_info->cfg_num; i++) {
        ret += isp_save_be_sync_reg(vi_pipe, reg_cfg_info, i);
    }

    for (i = 0; i < reg_cfg_info->cfg_num; i++) {
        ret += isp_reg_default(vi_pipe, reg_cfg_info, i);
        ret += isp_system_reg_config(vi_pipe, i); /* sys */
        ret += isp_dither_reg_config(vi_pipe, reg_cfg_info, i); /* dither */
        ret += isp_stt_reg_config(vi_pipe, reg_cfg_info, i);
        /* Be alg cfgs setting to register */
        ret += isp_be_alg_reg_config(vi_pipe, reg_cfg_info, i);
    }

    for (i = 0; i < reg_cfg_info->cfg_num; i++) {
        ret += isp_be_alg_sync_reg_config(vi_pipe, reg_cfg_info, i);
        ret += isp_be_alg_lut2stt_reg_new_reg_config(vi_pipe, reg_cfg_info, i);
        isp_be_reg_up_config(vi_pipe, i);
        ret += isp_be_alg_lut_update_reg_config(vi_pipe, reg_cfg_info, i);
    }

    ret += isp_be_sync_param_init(vi_pipe, reg_cfg_info);
    if (ret != TD_SUCCESS) {
        isp_err_trace("ISP[%d] isp_be_regs_config_init failed!\n", vi_pipe);
        return TD_FAILURE;
    }

    return TD_SUCCESS;
}

td_s32 isp_reg_cfg_init(ot_vi_pipe vi_pipe)
{
    isp_reg_cfg_attr *reg_cfg_ctx = TD_NULL;

    isp_regcfg_get_ctx(vi_pipe, reg_cfg_ctx);

    if (reg_cfg_ctx == TD_NULL) {
        reg_cfg_ctx = (isp_reg_cfg_attr *)isp_malloc(sizeof(isp_reg_cfg_attr));
        if (reg_cfg_ctx == TD_NULL) {
            isp_err_trace("Isp[%d] RegCfgCtx malloc memory failed!\n", vi_pipe);
            return OT_ERR_ISP_NOMEM;
        }
    }

    (td_void)memset_s(reg_cfg_ctx, sizeof(isp_reg_cfg_attr), 0, sizeof(isp_reg_cfg_attr));

    isp_regcfg_set_ctx(vi_pipe, reg_cfg_ctx);

    return TD_SUCCESS;
}

td_void isp_reg_cfg_exit(ot_vi_pipe vi_pipe)
{
    isp_reg_cfg_attr *reg_cfg_ctx = TD_NULL;

    isp_regcfg_get_ctx(vi_pipe, reg_cfg_ctx);
    isp_free(reg_cfg_ctx);
    isp_regcfg_reset_ctx(vi_pipe);
}

td_s32 isp_get_reg_cfg_ctx(ot_vi_pipe vi_pipe, td_void **reg_cfg_info)
{
    isp_usr_ctx *isp_ctx = TD_NULL;
    isp_reg_cfg_attr *reg_cfg = TD_NULL;

    isp_get_ctx(vi_pipe, isp_ctx);
    isp_regcfg_get_ctx(vi_pipe, reg_cfg);
    isp_check_pointer_return(reg_cfg);

    if (!reg_cfg->init) {
        reg_cfg->reg_cfg.cfg_key.key = 0;

        reg_cfg->init = TD_TRUE;
    }

    reg_cfg->reg_cfg.cfg_num = isp_ctx->block_attr.block_num;

    *reg_cfg_info = &reg_cfg->reg_cfg;

    return TD_SUCCESS;
}

td_s32 isp_reg_cfg_info_init(ot_vi_pipe vi_pipe)
{
    td_s32 ret;
    isp_reg_cfg_attr *reg_cfg = TD_NULL;
    isp_usr_ctx *isp_ctx = TD_NULL;

    isp_get_ctx(vi_pipe, isp_ctx);
    isp_regcfg_get_ctx(vi_pipe, reg_cfg);
    isp_check_pointer_return(reg_cfg);

    ret = isp_fe_regs_config(vi_pipe, &reg_cfg->reg_cfg);
    if (ret != TD_SUCCESS) {
        isp_err_trace("Pipe:%d isp_fe_regs_config failed!\n", vi_pipe);
    }

    if (isp_ctx->alg_run_select != OT_ISP_ALG_RUN_FE_ONLY) {
        ret = isp_be_regs_config_init(vi_pipe, &reg_cfg->reg_cfg);
        if (ret != TD_SUCCESS) {
            isp_err_trace("Pipe:%d isp_be_regs_config_init failed!\n", vi_pipe);
        }
    }
    return TD_SUCCESS;
}

td_s32 isp_reg_cfg_info_set(ot_vi_pipe vi_pipe)
{
    td_s32 ret;
    isp_reg_cfg_attr *reg_cfg = TD_NULL;

    isp_regcfg_get_ctx(vi_pipe, reg_cfg);
    isp_check_pointer_return(reg_cfg);

    ret = isp_fe_regs_config(vi_pipe, &reg_cfg->reg_cfg);
    if (ret != TD_SUCCESS) {
        isp_err_trace("Pipe:%d isp_fe_regs_config failed!\n", vi_pipe);
    }

    ret = isp_be_regs_config(vi_pipe, &reg_cfg->reg_cfg);
    if (ret != TD_SUCCESS) {
        isp_err_trace("Pipe:%d isp_be_regs_config failed!\n", vi_pipe);
    }

    if (reg_cfg->reg_cfg.kernel_reg_cfg.cfg_key.key) {
        ret = ioctl(isp_get_fd(vi_pipe), ISP_REG_CFG_SET, &reg_cfg->reg_cfg.kernel_reg_cfg);
        if (ret != TD_SUCCESS) {
            isp_err_trace("Config ISP register Failed with ec %#x!\n", ret);
            return ret;
        }
    }

    return TD_SUCCESS;
}

static td_void isp_sns_regs_info_check(ot_vi_pipe vi_pipe, const ot_isp_sns_regs_info *sns_regs_info)
{
    ot_unused(vi_pipe);

    if ((sns_regs_info->sns_type >= OT_ISP_SNS_TYPE_BUTT)) {
        isp_err_trace("senor's regs info invalid, sns_type %d\n", sns_regs_info->sns_type);
        return;
    }

    if (sns_regs_info->reg_num > OT_ISP_MAX_SNS_REGS) {
        isp_err_trace("senor's regs info invalid, reg_num %u\n", sns_regs_info->reg_num);
        return;
    }

    return;
}

static td_void isp_get_start_and_end_vipipe(ot_vi_pipe vi_pipe, isp_usr_ctx *isp_ctx, td_s32 *pipe_st, td_s32 *pipe_ed)
{
    if (isp_ctx->stitch_attr.stitch_enable == TD_TRUE) {
#ifdef CONFIG_OT_VI_STITCH_GRP
        td_s8 stitch_main_pipe = isp_ctx->stitch_attr.stitch_bind_id[0];

        if (is_stitch_main_pipe(vi_pipe, stitch_main_pipe)) {
            *pipe_st = 0;
            *pipe_ed = isp_ctx->stitch_attr.stitch_pipe_num - 1;
        } else {
            *pipe_st = vi_pipe;
            *pipe_ed = vi_pipe - 1;
        }
#endif
    } else {
        *pipe_st = vi_pipe;
        *pipe_ed = vi_pipe;
    }
}

static td_void isp_normal_sync_cfg_get(ot_vi_pipe vi_pipe, ot_isp_sns_regs_info *sns_regs_info,
    isp_alg_reg_cfg *alg_reg_cfg, isp_sync_cfg_buf_node *sync_cfg_node)
{
    (td_void)memcpy_s(&sync_cfg_node->sns_regs_info, sizeof(ot_isp_sns_regs_info), sns_regs_info,
        sizeof(ot_isp_sns_regs_info));
    isp_sns_regs_info_check(vi_pipe, &sync_cfg_node->sns_regs_info);
    (td_void)memcpy_s(&sync_cfg_node->ae_reg_cfg, sizeof(isp_ae_reg_cfg_2), &alg_reg_cfg->ae_reg_cfg2,
        sizeof(isp_ae_reg_cfg_2));
    (td_void)memcpy_s(&sync_cfg_node->awb_reg_cfg.be_white_balance_gain[0], sizeof(td_u32) * OT_ISP_BAYER_CHN_NUM,
        &alg_reg_cfg->awb_reg_cfg.awb_reg_dyn_cfg.be_white_balance_gain[0], sizeof(td_u32) * OT_ISP_BAYER_CHN_NUM);
    (td_void)memcpy_s(&sync_cfg_node->awb_reg_cfg.color_matrix[0], sizeof(td_u16) * OT_ISP_CCM_MATRIX_SIZE,
        &alg_reg_cfg->awb_reg_cfg.awb_reg_dyn_cfg.be_color_matrix[0], sizeof(td_u16) * OT_ISP_CCM_MATRIX_SIZE);
    (td_void)memcpy_s(&sync_cfg_node->drc_reg_cfg, sizeof(isp_drc_sync_cfg), &alg_reg_cfg->drc_reg_cfg.sync_reg_cfg,
        sizeof(isp_drc_sync_cfg));
    (td_void)memcpy_s(&sync_cfg_node->wdr_reg_cfg, sizeof(isp_fswdr_sync_cfg), &alg_reg_cfg->wdr_reg_cfg.sync_reg_cfg,
        sizeof(isp_fswdr_sync_cfg));
    (td_void)memcpy_s(&sync_cfg_node->be_blc_reg_cfg, sizeof(isp_be_blc_dyna_cfg), &alg_reg_cfg->be_blc_cfg.dyna_blc,
        sizeof(isp_be_blc_dyna_cfg));
    (td_void)memcpy_s(&sync_cfg_node->fe_blc_reg_cfg, sizeof(isp_fe_blc_dyna_cfg), &alg_reg_cfg->fe_blc_cfg.dyna_blc,
        sizeof(isp_fe_blc_dyna_cfg));
#ifdef CONFIG_OT_ISP_DYNAMIC_BLC_SUPPORT
    (td_void)memcpy_s(&sync_cfg_node->dynamic_blc_cfg, sizeof(isp_dynamic_blc_sync_cfg),
        &alg_reg_cfg->dynamic_blc_reg_cfg.sync_cfg, sizeof(isp_dynamic_blc_sync_cfg));
#endif
    (td_void)memcpy_s(&sync_cfg_node->fpn_cfg, sizeof(isp_fpn_sync_cfg), &alg_reg_cfg->fpn_reg_cfg.sync_cfg,
        sizeof(isp_fpn_sync_cfg));
    sync_cfg_node->awb_reg_cfg.be_awb_switch = alg_reg_cfg->awb_reg_cfg.awb_reg_usr_cfg.valid_awb_switch;
}

#ifdef CONFIG_OT_VI_STITCH_GRP
static td_void isp_main_pipe_stitch_sync_cfg_get(const isp_usr_ctx *isp_ctx, isp_sync_cfg_buf_node *sync_cfg_node)
{
    td_u8 i, stitch_idx;
    ot_vi_pipe stitch_pipe;
    isp_reg_cfg_attr *reg_cfg = TD_NULL;
    isp_awb_reg_dyn_cfg *awb_reg = TD_NULL;

    for (stitch_idx = 0; stitch_idx < isp_ctx->stitch_attr.stitch_pipe_num; stitch_idx++) {
        stitch_pipe = isp_ctx->stitch_attr.stitch_bind_id[stitch_idx];
        isp_regcfg_get_ctx(stitch_pipe, reg_cfg);
        if (reg_cfg == TD_NULL) {
            continue;
        }
        awb_reg = &reg_cfg->reg_cfg.alg_reg_cfg[0].awb_reg_cfg.awb_reg_dyn_cfg;
        for (i = 0; i < OT_ISP_BAYER_CHN_NUM; i++) {
            sync_cfg_node->awb_reg_cfg_stitch[stitch_idx].be_white_balance_gain[i] = awb_reg->be_white_balance_gain[i];
        }

        for (i = 0; i < OT_ISP_CCM_MATRIX_SIZE; i++) {
            sync_cfg_node->awb_reg_cfg_stitch[stitch_idx].color_matrix[i] = awb_reg->be_color_matrix[0][i];
        }

        (td_void)memcpy_s(&sync_cfg_node->fe_blc_reg_cfg_stitch[stitch_idx], sizeof(isp_fe_blc_dyna_cfg),
            &reg_cfg->reg_cfg.alg_reg_cfg[0].fe_blc_cfg.dyna_blc, sizeof(isp_fe_blc_dyna_cfg));
        (td_void)memcpy_s(&sync_cfg_node->be_blc_reg_cfg_stitch[stitch_idx], sizeof(isp_be_blc_dyna_cfg),
            &reg_cfg->reg_cfg.alg_reg_cfg[0].be_blc_cfg.dyna_blc, sizeof(isp_be_blc_dyna_cfg));
    }
}

static td_void isp_stitch_sync_cfg_get(ot_vi_pipe vi_pipe, ot_isp_sns_regs_info *sns_regs_info,
    isp_alg_reg_cfg *alg_reg_cfg, isp_sync_cfg_buf_node *sync_cfg_node)
{
    td_s8 stitch_main_pipe;
    isp_usr_ctx *isp_ctx = TD_NULL;
    isp_reg_cfg_attr *reg_cfg_s = TD_NULL;

    isp_get_ctx(vi_pipe, isp_ctx);

    if (isp_ctx->stitch_attr.stitch_enable != TD_TRUE) {
        return;
    }

    stitch_main_pipe = isp_ctx->stitch_attr.stitch_bind_id[0];
    if (is_stitch_main_pipe(vi_pipe, stitch_main_pipe)) {
        isp_main_pipe_stitch_sync_cfg_get(isp_ctx, sync_cfg_node);
        return;
    }

    isp_regcfg_get_ctx(stitch_main_pipe, reg_cfg_s);
    if (reg_cfg_s == TD_NULL) {
        return;
    }

    (td_void)memcpy_s(&sync_cfg_node->sns_regs_info, sizeof(ot_isp_sns_regs_info),
        &reg_cfg_s->sync_cfg_node.sns_regs_info, sizeof(ot_isp_sns_regs_info));
    (td_void)memcpy_s(&sync_cfg_node->sns_regs_info.com_bus, sizeof(ot_isp_sns_commbus), &sns_regs_info->com_bus,
        sizeof(ot_isp_sns_commbus));
    (td_void)memcpy_s(&sync_cfg_node->sns_regs_info.slv_sync.slave_bind_dev, sizeof(td_u32),
        &sns_regs_info->slv_sync.slave_bind_dev, sizeof(td_u32));
    (td_void)memcpy_s(&sync_cfg_node->ae_reg_cfg, sizeof(isp_ae_reg_cfg_2),
        &reg_cfg_s->reg_cfg.alg_reg_cfg[0].ae_reg_cfg2, sizeof(isp_ae_reg_cfg_2));
    (td_void)memcpy_s(&sync_cfg_node->awb_reg_cfg.color_matrix[0], sizeof(td_u16) * OT_ISP_CCM_MATRIX_SIZE,
        &alg_reg_cfg->awb_reg_cfg.awb_reg_dyn_cfg.be_color_matrix[0], sizeof(td_u16) * OT_ISP_CCM_MATRIX_SIZE);
    (td_void)memcpy_s(&sync_cfg_node->awb_reg_cfg.be_white_balance_gain[0], sizeof(td_u32) * OT_ISP_BAYER_CHN_NUM,
        &alg_reg_cfg->awb_reg_cfg.awb_reg_dyn_cfg.be_white_balance_gain[0], sizeof(td_u32) * OT_ISP_BAYER_CHN_NUM);
    sync_cfg_node->awb_reg_cfg.be_awb_switch = alg_reg_cfg->awb_reg_cfg.awb_reg_usr_cfg.valid_awb_switch;
}
#endif

td_s32 isp_sync_cfg_set(ot_vi_pipe vi_pipe)
{
    td_s32 ret;
    td_s32 pipe_st = 0;
    td_s32 pipe_ed = 0;
    ot_vi_pipe pipe_tmp = vi_pipe;

    isp_usr_ctx *isp_ctx = TD_NULL;
    isp_reg_cfg_attr *reg_cfg = TD_NULL;

    ot_isp_sns_regs_info *sns_regs_info = TD_NULL;

    isp_get_ctx(pipe_tmp, isp_ctx);

    isp_get_start_and_end_vipipe(pipe_tmp, isp_ctx, &pipe_st, &pipe_ed);

    while (pipe_st <= pipe_ed) {
        if (isp_ctx->stitch_attr.stitch_enable == TD_TRUE) {
            pipe_tmp = isp_ctx->stitch_attr.stitch_bind_id[pipe_st];
        } else {
            pipe_tmp = pipe_st;
        }

        isp_get_ctx(pipe_tmp, isp_ctx);
        isp_regcfg_get_ctx(pipe_tmp, reg_cfg);
        isp_check_pointer_return(reg_cfg);
        isp_check_open_return(pipe_tmp);

        if (isp_sensor_update_sns_reg(pipe_tmp) != TD_SUCCESS) {
            /* If Users need to config AE sync info themselves, they can set pfn_cmos_get_sns_reg_info
             * to TD_NULL in cmos.c
             */
            /* Then there will be NO AE sync configs in kernel of firmware */
            return TD_SUCCESS;
        }

        isp_sensor_get_sns_reg(pipe_tmp, &sns_regs_info);

        isp_normal_sync_cfg_get(pipe_tmp, sns_regs_info, &reg_cfg->reg_cfg.alg_reg_cfg[0], &reg_cfg->sync_cfg_node);
#ifdef CONFIG_OT_VI_STITCH_GRP
        isp_stitch_sync_cfg_get(pipe_tmp, sns_regs_info, &reg_cfg->reg_cfg.alg_reg_cfg[0], &reg_cfg->sync_cfg_node);
#endif
        reg_cfg->sync_cfg_node.valid = TD_TRUE;
        ret = ioctl(isp_get_fd(pipe_tmp), ISP_SYNC_CFG_SET, &reg_cfg->sync_cfg_node);
        if (ret != TD_SUCCESS) {
            return ret;
        }

        sns_regs_info->config = TD_TRUE;

        pipe_st++;
    }

    return TD_SUCCESS;
}
#ifdef CONFIG_OT_SNAP_SUPPORT
td_bool isp_pro_trigger_get(ot_vi_pipe vi_pipe)
{
    td_s32 ret;
    td_bool enable;

    ret = ioctl(isp_get_fd(vi_pipe), ISP_PRO_TRIGGER_GET, &enable);
    if (ret != TD_SUCCESS) {
        return TD_FALSE;
    }

    return enable;
}
#endif
td_s32 isp_reg_cfg_ctrl(ot_vi_pipe vi_pipe)
{
    td_u8 i;
    isp_usr_ctx *isp_ctx = TD_NULL;
    isp_reg_cfg_attr *reg_cfg = TD_NULL;
    isp_get_ctx(vi_pipe, isp_ctx);
    isp_regcfg_get_ctx(vi_pipe, reg_cfg);
    isp_check_pointer_return(reg_cfg);
    reg_cfg->reg_cfg.cfg_key.key = 0xFFFFFFFFFFFFFFFF;
    for (i = isp_ctx->block_attr.pre_block_num; i < isp_ctx->block_attr.block_num; i++) {
        (td_void)memcpy_s(&reg_cfg->reg_cfg.alg_reg_cfg[i], sizeof(isp_alg_reg_cfg), &reg_cfg->reg_cfg.alg_reg_cfg[0],
            sizeof(isp_alg_reg_cfg));
    }

    if (is_online_mode(isp_ctx->block_attr.running_mode)) {
        for (i = 0; i < MIN2(OT_ISP_STRIPING_MAX_NUM, isp_ctx->block_attr.block_num); i++) {
            isp_ctx->special_opt.be_on_stt_update[i] = TD_TRUE;
        }

        for (i = isp_ctx->block_attr.pre_block_num; i < isp_ctx->block_attr.block_num; i++) {
            isp_be_resh_cfg(&reg_cfg->reg_cfg.alg_reg_cfg[i]);
        }
    }

    reg_cfg->reg_cfg.cfg_num = isp_ctx->block_attr.block_num;

    return TD_SUCCESS;
}

static td_void isp_gamma_lut_apb_reg_config(isp_post_be_reg_type *be_reg, isp_gamma_usr_cfg *usr_reg_cfg)
{
    td_u16 j;
    isp_gamma_lut_waddr_write(be_reg, 0);

    for (j = 0; j < GAMMA_REG_NODE_NUM; j++) {
        isp_gamma_lut_wdata_write(be_reg, usr_reg_cfg->gamma_lut[j]);
    }
}

static td_void isp_sharpen_lut_apb_reg_config(isp_post_be_reg_type *be_reg, isp_sharpen_mpi_dyna_reg_cfg *dyna)
{
    td_u16 j;
    isp_sharpen_mhfgaind_waddr_write(be_reg, 0);
    isp_sharpen_mhfgainud_waddr_write(be_reg, 0);

    for (j = 0; j < SHRP_GAIN_LUT_SIZE; j++) {
        isp_sharpen_mhfgaind_wdata_write(be_reg, (dyna->hf_gain_d[j] << 12) + dyna->mf_gain_d[j]);         /* 12 */
        isp_sharpen_mhfgainud_wdata_write(be_reg, (dyna->hf_gain_ud[j] << 12) + dyna->mf_gain_ud[j]);      /* 12 */
    }
}

static td_void isp_ldci_lut_apb_reg_config(isp_post_be_reg_type *be_reg, isp_ldci_dyna_cfg *dyna_reg_cfg)
{
    td_u16 j;
    isp_ldci_cgain_waddr_write(be_reg, 0);
    isp_ldci_he_waddr_write(be_reg, 0);

    for (j = 0; j < LDCI_COLOR_GAIN_LUT_SIZE; j++) {
        isp_ldci_cgain_wdata_write(be_reg, dyna_reg_cfg->color_gain_lut[j]);
    }

    for (j = 0; j < LDCI_HE_LUT_SIZE; j++) {
        isp_ldci_he_wdata_write(be_reg, dyna_reg_cfg->he_pos_lut[j], dyna_reg_cfg->he_neg_lut[j]);
    }
}

static td_s32 isp_lut_apb_reg_config(ot_vi_pipe vi_pipe, isp_reg_cfg_attr *reg_cfg)
{
    td_u8 i;
    isp_post_be_reg_type *be_reg = TD_NULL;
    isp_alg_reg_cfg *alg_reg_cfg = TD_NULL;

    for (i = 0; i < reg_cfg->reg_cfg.cfg_num; i++) {
        alg_reg_cfg = &reg_cfg->reg_cfg.alg_reg_cfg[i];
        be_reg = (isp_post_be_reg_type *)isp_get_post_be_vir_addr(vi_pipe, i);

        isp_check_pointer_return(be_reg);

        /* gamma */
        isp_gamma_lut_apb_reg_config(be_reg, &alg_reg_cfg->gamma_cfg.usr_reg_cfg);

        /* sharpen */
        isp_sharpen_lut_apb_reg_config(be_reg, &alg_reg_cfg->sharpen_reg_cfg.dyna_reg_cfg.mpi_dyna_reg_cfg);

        /* LDCI */
        isp_ldci_lut_apb_reg_config(be_reg, &alg_reg_cfg->ldci_reg_cfg.dyna_reg_cfg);

        /* csp */
        if (reg_cfg->reg_cfg.cfg_key.bit1_csp_cfg && alg_reg_cfg->drc_reg_cfg.usr_reg_cfg.csp_lut_update_en &&
            alg_reg_cfg->drc_reg_cfg.usr_reg_cfg.csp_en) {
            isp_csp_lut_apb_config(be_reg, &alg_reg_cfg->drc_reg_cfg.usr_reg_cfg);
        }
    }

    return TD_SUCCESS;
}


td_s32 isp_switch_reg_set(ot_vi_pipe vi_pipe)
{
    td_s32 ret;
    isp_usr_ctx *isp_ctx = TD_NULL;
    isp_reg_cfg_attr *reg_cfg = TD_NULL;

    isp_get_ctx(vi_pipe, isp_ctx);

    if (is_online_mode(isp_ctx->block_attr.running_mode)) {
        isp_regcfg_get_ctx(vi_pipe, reg_cfg);
        isp_check_pointer_return(reg_cfg);
        isp_lut_apb_reg_config(vi_pipe, reg_cfg);
        ret = isp_reg_cfg_info_init(vi_pipe);
        if (ret != TD_SUCCESS) {
            isp_err_trace("vi_pipe %d isp_reg_cfg_info_init failed \n", vi_pipe);
        }
        if (reg_cfg->reg_cfg.kernel_reg_cfg.cfg_key.key) {
            ret = ioctl(isp_get_fd(vi_pipe), ISP_REG_CFG_SET, &reg_cfg->reg_cfg.kernel_reg_cfg);
            if (ret != TD_SUCCESS) {
                isp_err_trace("Config ISP register Failed with ec %#x!\n", ret);
                return ret;
            }
        }

        isp_ctx->block_attr.pre_block_num = isp_ctx->block_attr.block_num;

        return TD_SUCCESS;
    }

    /* record the register config information to fhy and kernel,and be valid in next frame. */
    ret = isp_reg_cfg_info_init(vi_pipe);
    if (ret != TD_SUCCESS) {
        return ret;
    }

    isp_ctx->para_rec.stitch_sync = TD_TRUE;
    ret = ioctl(isp_get_fd(vi_pipe), ISP_SYNC_INIT_SET, &isp_ctx->para_rec.stitch_sync);
    if (ret != TD_SUCCESS) {
        isp_ctx->para_rec.stitch_sync = TD_FALSE;
        isp_err_trace("ISP[%d] set isp stitch sync failed!\n", vi_pipe);
    }

    ret = isp_all_cfgs_be_buf_init(vi_pipe);
    if (ret != TD_SUCCESS) {
        isp_err_trace("pipe:%d init all be bufs failed \n", vi_pipe);
        return ret;
    }

    isp_ctx->block_attr.pre_block_num = isp_ctx->block_attr.block_num;

    return TD_SUCCESS;
}

td_s32 isp_lut2stt_apb_reg(ot_vi_pipe vi_pipe)
{
    td_s32 ret;
    isp_usr_ctx *isp_ctx = TD_NULL;
    isp_reg_cfg_attr *reg_cfg = TD_NULL;

    isp_regcfg_get_ctx(vi_pipe, reg_cfg);
    isp_check_pointer_return(reg_cfg);
    isp_get_ctx(vi_pipe, isp_ctx);

    /* config lut need before sensor init */
    if (is_online_mode(isp_ctx->block_attr.running_mode)) {
        ret = isp_lut_apb_reg_config(vi_pipe, reg_cfg);
        if (ret != TD_SUCCESS) {
            isp_err_trace("pipe:%d init all be bufs failed \n", vi_pipe);
            return ret;
        }
    }

    return TD_SUCCESS;
}

td_s32 isp_switch_state_set(ot_vi_pipe vi_pipe)
{
    td_s32 ret;

    ret = ioctl(isp_get_fd(vi_pipe), ISP_BE_SWITCH_FINISH_STATE_SET);
    if (ret != TD_SUCCESS) {
        return ret;
    }

    return TD_SUCCESS;
}
