/**
  Copyright (c), 2001-2025, Shenshu Tech. Co., Ltd.
 */
#if defined(CONFIG_SYMC_PROC_SUPPORT)

#include "hal_symc.h"
#include "hal_spacc_reg.h"
#include "hal_spacc_inner.h"

#include "crypto_drv_common.h"

td_s32 hal_cipher_symc_get_proc_info(td_u32 chn_num, crypto_symc_proc_info *proc_symc_info)
{
    td_s32 ret;
    hal_symc_config_t symc_config;

    in_sym_chn_key_ctrl in_key_ctrl;
 
    out_sym_chan_raw_int out_raw_int;
    out_sym_chan_raw_int_en out_sym_int_en;
    out_sym_chn_status out_sym_int_status;
 
    in_sym_chn_node_wr_point in_node_wr_ptr;
    in_sym_chn_node_rd_point in_node_rd_ptr;
    in_sym_chn_node_length in_node_depth;
 
    out_sym_chn_node_wr_point out_node_wr_ptr;
    out_sym_chn_node_rd_point out_node_rd_ptr;
    out_sym_chn_node_length out_node_depth;
 
    crypto_hal_func_enter();
 
    crypto_chk_return(proc_symc_info == TD_NULL, SYMC_COMPAT_ERRNO(ERROR_PARAM_IS_NULL), "proc_symc_info is NULL\n");
 
    /* Get Key Ctrl. */
    in_key_ctrl.u32 = spacc_reg_read(IN_SYM_CHN_KEY_CTRL(chn_num));
 
    ret = hal_cipher_symc_get_config(chn_num, &symc_config);
    crypto_chk_return(ret != TD_SUCCESS, SYMC_COMPAT_ERRNO(ERROR_INVALID_PARAM), "hal_cipher_symc_get_config failed\n");

    proc_symc_info->is_decrypt = symc_config.is_decrypt;

    /* Alg. */
    proc_symc_info->alg = symc_config.symc_alg;
 
    /* Alg Mode. */
    proc_symc_info->mode = symc_config.work_mode;
 
    /* Key Length. */
    proc_symc_info->key_len = symc_config.symc_key_length;
 
    /* Key Source. */
    proc_symc_info->key_source = in_key_ctrl.bits.sym_key_chn_id;
 
    /* int_raw. */
    out_raw_int.u32 = spacc_reg_read(OUT_SYM_CHAN_RAW_LAST_NODE_INT);
    proc_symc_info->int_raw = (out_raw_int.bits.out_sym_chan_raw_int >> chn_num) & 0x1;
 
    /* int_en. */
    out_sym_int_en.u32 = spacc_reg_read(OUT_SYM_CHAN_RAW_LAST_NODE_INT_EN);
    proc_symc_info->int_en = (out_sym_int_en.bits.out_sym_chan_int_en >> chn_num) & 0x1;
 
    /* int status. */
    out_sym_int_status.u32 = spacc_reg_read(OUT_SYM_CHAN_LAST_NODE_INT);
    proc_symc_info->int_status = (out_sym_int_status.bits.out_sym_chn_int_status >> chn_num) & 0x1;
 
    /* is_secure. */
    proc_symc_info->is_secure = (crypto_get_cpu_type() == CRYPTO_CPU_TYPE_SCPU) ? 1 : 0;
 
    /* in node: head(r/w/d) */
    in_node_wr_ptr.u32 = spacc_reg_read(IN_SYM_CHN_NODE_WR_POINT(chn_num));
    in_node_rd_ptr.u32 = spacc_reg_read(IN_SYM_CHN_NODE_RD_POINT(chn_num));
    in_node_depth.u32 = spacc_reg_read(IN_SYM_CHN_NODE_LENGTH(chn_num));
    proc_symc_info->in_node_head = spacc_reg_read(IN_SYM_CHN_NODE_START_ADDR_L(chn_num));
    proc_symc_info->in_node_rptr = in_node_rd_ptr.bits.sym_chn_node_rd_point;
    proc_symc_info->in_node_wptr = in_node_wr_ptr.bits.sym_chn_node_wr_point;
    proc_symc_info->in_node_depth = in_node_depth.bits.sym_chn_node_length;
 
    /* out node: head(r/w/d) */
    out_node_wr_ptr.u32 = spacc_reg_read(OUT_SYM_CHN_NODE_WR_POINT(chn_num));
    out_node_rd_ptr.u32 = spacc_reg_read(OUT_SYM_CHN_NODE_RD_POINT(chn_num));
    out_node_depth.u32 = spacc_reg_read(OUT_SYM_CHN_NODE_LENGTH(chn_num));
    proc_symc_info->out_node_head = spacc_reg_read(OUT_SYM_CHN_NODE_START_ADDR_L(chn_num));
    proc_symc_info->out_node_rptr = out_node_rd_ptr.bits.sym_chn_node_rd_point;
    proc_symc_info->out_node_wptr = out_node_wr_ptr.bits.sym_chn_node_wr_point;
    proc_symc_info->out_node_depth = out_node_depth.bits.sym_chn_node_length;
 
    crypto_hal_func_exit();
 
    return TD_SUCCESS;
}
#endif