/*
  Copyright (c), 2001-2025, Shenshu Tech. Co., Ltd.
 */

#include "template_sns_cfg.h"

__attribute__((unused)) static void template_sns_default_reg_init(cis_info *cis)
{
    td_u32 i;
    td_s32 ret = TD_SUCCESS;
    ot_isp_sns_state *past_sensor = TD_NULL;

    past_sensor = cis->sns_state;
    for (i = 0; i < past_sensor->regs_info[0].reg_num; i++) {
        ret += cis_write_reg(&cis->i2c,
            past_sensor->regs_info[0].i2c_data[i].reg_addr,
            past_sensor->regs_info[0].i2c_data[i].data);
    }

    if (ret != TD_SUCCESS) {
        isp_err_trace("write register failed!\n");
    }

    return;
}

static td_s32 template_sns_reg_init(cis_info *cis, cis_reg_cfg *cfg, td_u32 len)
{
    td_u32 i;

    sns_check_return(cis_write_reg(&cis->i2c, 0x0103, 0x01));
    cis_delay_ms(1); /* 1ms */

    for (i = 0; i < len; i++) {
        sns_check_return(cis_write_reg(&cis->i2c, cfg->addr, cfg->data));
        cfg++;
    }

    template_sns_default_reg_init(cis);

    sns_check_return(cis_write_reg(&cis->i2c, 0x0100, 0x01));

    return TD_SUCCESS;
}

td_s32 template_sns_4m_30fps_12bit_linear_init(cis_info *cis)
{
    td_s32 ret;
    td_u32 len;
    cis_reg_cfg *cfg = template_sns_linear_5m30_10bit;

    sns_check_pointer_return(cis);

    len = (td_u32)(sizeof(template_sns_linear_5m30_10bit) / sizeof(template_sns_linear_5m30_10bit[0]));
    ret = template_sns_reg_init(cis, cfg, len);
    if (ret != TD_SUCCESS) {
        isp_err_trace("template_sns_reg_init failed!\n");
        return ret;
    }

    printf("===================================================================================\n");
    printf("vi_pipe:%d, (MIPI) TEMPLATE_SNS_4M_30FPS_12BIT_LINEAR_MODE init success!\n", cis->pipe);
    printf("===================================================================================\n");

    return TD_SUCCESS;
}

td_s32 template_sns_4m_30fps_12bit_2to1_line_wdr_init(cis_info *cis)
{
    td_s32 ret;
    td_u32 len;
    cis_reg_cfg *cfg = template_sns_wdr_2t1_4m30_12bit;

    sns_check_pointer_return(cis);

    len = (td_u32)(sizeof(template_sns_wdr_2t1_4m30_12bit) / sizeof(template_sns_wdr_2t1_4m30_12bit[0]));
    ret = template_sns_reg_init(cis, cfg, len);
    if (ret != TD_SUCCESS) {
        isp_err_trace("template_sns_reg_init failed!\n");
        return ret;
    }

    printf("===========================================================================================\n");
    printf("vi_pipe:%d, (MIPI) TEMPLATE_SNS_4M_30FPS_12BIT_2TO1_LINE_WDR_MODE init success!\n", cis->pipe);
    printf("============================================================================================\n");

    return TD_SUCCESS;
}
