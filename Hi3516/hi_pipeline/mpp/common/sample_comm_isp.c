/*
  Copyright (c), 2001-2025, Shenshu Tech. Co., Ltd.
 */
#include <sched.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#ifdef __LITEOS__
#include <sys/select.h>
#endif
#include <fcntl.h>
#include <pthread.h>
#include <sys/time.h>
#include <sys/mman.h>
#include <signal.h>
#include <math.h>
#include <sys/prctl.h>

#include "ss_mpi_awb.h"
#include "ss_mpi_ae.h"
#ifdef CONFIG_OT_ISP_THERMO_SUPPORT
#include "ss_mpi_thermo.h"
#endif
#include "ot_sns_ctrl.h"
#include "ss_mpi_isp.h"
#include "ot_common_isp.h"
#include "ot_common_video.h"
#include "sample_comm.h"

static sample_sns_type g_sns_type[OT_VI_MAX_PIPE_NUM] = {SNS_TYPE_BUTT};
static pthread_t g_isp_pid[OT_VI_MAX_PIPE_NUM] = {0};

/* IspPub attr */

static ot_isp_pub_attr g_isp_pub_attr_sc4336p_mipi_4m_30fps = {
    { 0, 0, 2560, 1440 },
    { 2560, 1440 },
    30,
    OT_ISP_BAYER_BGGR,
    OT_WDR_MODE_NONE,
    0,
    1,
    0,
    {
        0,
        { 0, 0, 2560, 1440 },
    },
};

static ot_isp_pub_attr g_isp_pub_attr_sc4336p_mipi_3m_30fps = {
    { 0, 0, 2304, 1296 },
    { 2304, 1296 },
    30,
    OT_ISP_BAYER_BGGR,
    OT_WDR_MODE_NONE,
    0,
    1,
    0,
    {
        0,
        { 0, 0, 2304, 1296 },
    },
};

static ot_isp_pub_attr g_isp_pub_attr_sc4336p_mipi_2m_30fps = {
    { 0, 0, 1920, 1080 },
    { 1920, 1080 },
    30,
    OT_ISP_BAYER_BGGR,
    OT_WDR_MODE_NONE,
    0,
    0,
    0,
    {
        0,
        { 0, 0, 1920, 1080 },
    },
};

static ot_isp_pub_attr g_isp_pub_attr_os04d10_mipi_4m_30fps = {
    { 0, 0, 2560, 1440 },
    { 2560, 1440 },
    30,
    OT_ISP_BAYER_BGGR,
    OT_WDR_MODE_NONE,
    0,
    0,
    0,
    {
        0,
        { 0, 0, 2560, 1440 },
    },
};

static ot_isp_pub_attr g_isp_pub_attr_sc431hai_mipi_4m_30fps = {
    {0, 0, 2560, 1440},
    { 2560, 1440 },
    30,
    OT_ISP_BAYER_RGGB,
    OT_WDR_MODE_NONE,
    0,
    0,
    0,
    {
        0,
        {0, 0, 2560, 1440},
    },
};

static ot_isp_pub_attr g_isp_pub_attr_sc431hai_mipi_4m_30fps_wdr2to1 = {
    {0, 0, 2560, 1440},
    { 2560, 1440 },
    30,
    OT_ISP_BAYER_RGGB,
    OT_WDR_MODE_2To1_LINE,
    0,
    0,
    0,
    {
        0,
        {0, 0, 2560, 1440},
    },
};

static ot_isp_pub_attr g_isp_pub_attr_sc450ai_mipi_4m_30fps = {
    {0, 0, 2688, 1520},
    { 2688, 1520 },
    30,
    OT_ISP_BAYER_RGGB,
    OT_WDR_MODE_NONE,
    0,
    0,
    0,
    {
        0,
        {0, 0, 2688, 1520},
    },
};

static ot_isp_pub_attr g_isp_pub_attr_sc450ai_mipi_4m_30fps_wdr2to1 = {
    {0, 0, 2688, 1520},
    { 2688, 1520 },
    30,
    OT_ISP_BAYER_RGGB,
    OT_WDR_MODE_2To1_LINE,
    0,
    0,
    0,
    {
        0,
        {0, 0, 2688, 1520},
    },
};

static ot_isp_pub_attr g_isp_pub_attr_sc500ai_mipi_5m_30fps = {
    {0, 0, 2880, 1620},
    { 2880, 1620 },
    30,
    OT_ISP_BAYER_BGGR,
    OT_WDR_MODE_NONE,
    0,
    0,
    0,
    {
        0,
        {0, 0, 2880, 1620},
    },
};

static ot_isp_pub_attr g_isp_pub_attr_sc500ai_mipi_5m_30fps_wdr2to1 = {
    {0, 0, 2880, 1620},
    { 2880, 1620 },
    30,
    OT_ISP_BAYER_BGGR,
    OT_WDR_MODE_2To1_LINE,
    0,
    0,
    0,
    {
        0,
        {0, 0, 2880, 1620},
    },
};

static ot_isp_pub_attr g_isp_pub_attr_sc533hai_mipi_5m_30fps = {
    {0, 0, 2880, 1620},
    { 2880, 1620 },
    30,
    OT_ISP_BAYER_BGGR,
    OT_WDR_MODE_NONE,
    0,
    0,
    0,
    {
        0,
        {0, 0, 2880, 1620},
    },
};

static ot_isp_pub_attr g_isp_pub_attr_sc533hai_mipi_5m_30fps_wdr2to1 = {
    {0, 0, 2880, 1620},
    { 2880, 1620 },
    30,
    OT_ISP_BAYER_BGGR,
    OT_WDR_MODE_2To1_LINE,
    0,
    0,
    0,
    {
        0,
        {0, 0, 2880, 1620},
    },
};

static ot_isp_pub_attr g_isp_pub_attr_gc4023_mipi_4m_30fps = {
    { 0, 0, 2560, 1440 },
    { 2560, 1440 },
    30,
    OT_ISP_BAYER_RGGB,
    OT_WDR_MODE_NONE,
    0,
    0,
    0,
    {
        0,
        { 0, 0, 2560, 1440 },
    },
};

td_void sample_comm_isp_get_pub_attr_by_sns_part1(sample_sns_type sns_type, ot_isp_pub_attr *pub_attr)
{
    switch (sns_type) {
        case SC500AI_MIPI_5M_30FPS_10BIT:
            (td_void)memcpy_s(pub_attr, sizeof(ot_isp_pub_attr),
                &g_isp_pub_attr_sc500ai_mipi_5m_30fps, sizeof(ot_isp_pub_attr));
            break;
        case SC500AI_MIPI_5M_30FPS_10BIT_WDR2TO1:
            (td_void)memcpy_s(pub_attr, sizeof(ot_isp_pub_attr),
                &g_isp_pub_attr_sc500ai_mipi_5m_30fps_wdr2to1, sizeof(ot_isp_pub_attr));
            break;
        case SC533HAI_MIPI_5M_30FPS_10BIT:
            (td_void)memcpy_s(pub_attr, sizeof(ot_isp_pub_attr),
                &g_isp_pub_attr_sc533hai_mipi_5m_30fps, sizeof(ot_isp_pub_attr));
            break;
        case SC533HAI_MIPI_5M_30FPS_10BIT_WDR2TO1:
            (td_void)memcpy_s(pub_attr, sizeof(ot_isp_pub_attr),
                &g_isp_pub_attr_sc533hai_mipi_5m_30fps_wdr2to1, sizeof(ot_isp_pub_attr));
            break;
        case OS04D10_MIPI_4M_30FPS_10BIT:
            (td_void)memcpy_s(pub_attr, sizeof(ot_isp_pub_attr),
                &g_isp_pub_attr_os04d10_mipi_4m_30fps, sizeof(ot_isp_pub_attr));
            break;

        default:
            break;
    }
}

td_s32 sample_comm_isp_get_pub_attr_by_sns(sample_sns_type sns_type, ot_isp_pub_attr *pub_attr)
{
    switch (sns_type) {
        case SC4336P_MIPI_4M_30FPS_10BIT:
            (td_void)memcpy_s(pub_attr, sizeof(ot_isp_pub_attr),
                &g_isp_pub_attr_sc4336p_mipi_4m_30fps, sizeof(ot_isp_pub_attr));
            break;
        case SC4336P_MIPI_3M_30FPS_10BIT:
            (td_void)memcpy_s(pub_attr, sizeof(ot_isp_pub_attr),
                &g_isp_pub_attr_sc4336p_mipi_3m_30fps, sizeof(ot_isp_pub_attr));
            break;
        case SC4336P_MIPI_2M_30FPS_10BIT:
            (td_void)memcpy_s(pub_attr, sizeof(ot_isp_pub_attr),
                &g_isp_pub_attr_sc4336p_mipi_2m_30fps, sizeof(ot_isp_pub_attr));
            break;
        case GC4023_MIPI_4M_30FPS_10BIT:
            (td_void)memcpy_s(pub_attr, sizeof(ot_isp_pub_attr),
                &g_isp_pub_attr_gc4023_mipi_4m_30fps, sizeof(ot_isp_pub_attr));
            break;
        case SC431HAI_MIPI_4M_30FPS_10BIT:
            (td_void)memcpy_s(pub_attr, sizeof(ot_isp_pub_attr),
                &g_isp_pub_attr_sc431hai_mipi_4m_30fps, sizeof(ot_isp_pub_attr));
            break;
        case SC431HAI_MIPI_4M_30FPS_10BIT_WDR2TO1:
            (td_void)memcpy_s(pub_attr, sizeof(ot_isp_pub_attr),
                &g_isp_pub_attr_sc431hai_mipi_4m_30fps_wdr2to1, sizeof(ot_isp_pub_attr));
            break;
        case SC450AI_MIPI_4M_30FPS_10BIT:
            (td_void)memcpy_s(pub_attr, sizeof(ot_isp_pub_attr),
                &g_isp_pub_attr_sc450ai_mipi_4m_30fps, sizeof(ot_isp_pub_attr));
            break;
        case SC450AI_MIPI_4M_30FPS_10BIT_WDR2TO1:
            (td_void)memcpy_s(pub_attr, sizeof(ot_isp_pub_attr),
                &g_isp_pub_attr_sc450ai_mipi_4m_30fps_wdr2to1, sizeof(ot_isp_pub_attr));
            break;

        default:
            (td_void)memcpy_s(pub_attr, sizeof(ot_isp_pub_attr),
                &g_isp_pub_attr_sc4336p_mipi_4m_30fps, sizeof(ot_isp_pub_attr));
            break;
    }
    sample_comm_isp_get_pub_attr_by_sns_part1(sns_type, pub_attr);
    return TD_SUCCESS;
}

ot_isp_sns_obj *sample_comm_isp_get_sns_obj(sample_sns_type sns_type)
{
    switch (sns_type) {
#ifdef SC4336P_MIPI_4M_30FPS_10BIT_SELECT
        case SC4336P_MIPI_4M_30FPS_10BIT:
            return &g_sns_sc4336p_obj;
#endif
#ifdef SC4336P_MIPI_3M_30FPS_10BIT_SELECT
        case SC4336P_MIPI_3M_30FPS_10BIT:
            return &g_sns_sc4336p_obj;
#endif
#ifdef SC4336P_MIPI_2M_30FPS_10BIT_SELECT
        case SC4336P_MIPI_2M_30FPS_10BIT:
            return &g_sns_sc4336p_obj;
#endif
#ifdef GC4023_MIPI_4M_30FPS_10BIT_SELECT
        case GC4023_MIPI_4M_30FPS_10BIT:
            return &g_sns_gc4023_obj;
#endif
#ifdef OS04D10_MIPI_4M_30FPS_10BIT_SELECT
        case OS04D10_MIPI_4M_30FPS_10BIT:
            return &g_sns_os04d10_obj;
#endif
#ifdef SC431HAI_MIPI_4M_30FPS_10BIT_SELECT
        case SC431HAI_MIPI_4M_30FPS_10BIT:
        case SC431HAI_MIPI_4M_30FPS_10BIT_WDR2TO1:
            return &g_sns_sc431hai_obj;
#endif
#ifdef SC450AI_MIPI_4M_30FPS_10BIT_SELECT
        case SC450AI_MIPI_4M_30FPS_10BIT:
        case SC450AI_MIPI_4M_30FPS_10BIT_WDR2TO1:
            return &g_sns_sc450ai_obj;
#endif
#ifdef SC500AI_MIPI_5M_30FPS_10BIT_SELECT
        case SC500AI_MIPI_5M_30FPS_10BIT:
        case SC500AI_MIPI_5M_30FPS_10BIT_WDR2TO1:
            return &g_sns_sc500ai_obj;
#endif
#ifdef SC533HAI_MIPI_5M_30FPS_10BIT_SELECT
        case SC533HAI_MIPI_5M_30FPS_10BIT:
        case SC533HAI_MIPI_5M_30FPS_10BIT_WDR2TO1:
            return &g_sns_sc533hai_obj;
#endif
        default:
            return TD_NULL;
    }
}

ot_isp_sns_type sample_comm_get_sns_bus_type(sample_sns_type sns_type)
{
    ot_unused(sns_type);
    return OT_ISP_SNS_TYPE_I2C;
}

/******************************************************************************
* funciton : ISP init
******************************************************************************/
td_s32 sample_comm_isp_sensor_regiter_callback(ot_isp_dev isp_dev, sample_sns_type sns_type)
{
    td_s32 ret;
    ot_isp_3a_alg_lib ae_lib;
    ot_isp_3a_alg_lib thermo_lib;
    ot_isp_3a_alg_lib awb_lib;
    ot_isp_sns_obj *sns_obj;

    sns_obj = sample_comm_isp_get_sns_obj(sns_type);
    if (sns_obj == TD_NULL) {
        printf("sensor %d not exist!\n", sns_type);
        return TD_FAILURE;
    }

    ae_lib.id = isp_dev;
    thermo_lib.id = isp_dev;
    awb_lib.id = isp_dev;
    ret = strncpy_s(ae_lib.lib_name, sizeof(ae_lib.lib_name), OT_AE_LIB_NAME, sizeof(OT_AE_LIB_NAME));
    sample_check_eok_return(ret, TD_FAILURE);
    ret = strncpy_s(thermo_lib.lib_name, sizeof(thermo_lib.lib_name), OT_THERMO_LIB_NAME, sizeof(OT_THERMO_LIB_NAME));
    sample_check_eok_return(ret, TD_FAILURE);
    strncpy_s(awb_lib.lib_name, sizeof(awb_lib.lib_name), OT_AWB_LIB_NAME, sizeof(OT_AWB_LIB_NAME));
    sample_check_eok_return(ret, TD_FAILURE);
    if (sns_obj->pfn_register_callback != TD_NULL) {
        ret = sns_obj->pfn_register_callback(isp_dev, &ae_lib, &awb_lib);
        if (ret != TD_SUCCESS) {
            printf("sensor_register_callback failed with %#x!\n", ret);
            return ret;
        }
    } else {
        printf("sensor_register_callback failed with TD_NULL!\n");
    }

    g_sns_type[isp_dev] = sns_type;

    return TD_SUCCESS;
}

td_s32 sample_comm_isp_sensor_unregiter_callback(ot_isp_dev isp_dev)
{
    ot_isp_3a_alg_lib ae_lib;
#ifdef CONFIG_OT_ISP_THERMO_SUPPORT
    ot_isp_3a_alg_lib thermo_lib;
#endif
    ot_isp_3a_alg_lib awb_lib;
    ot_isp_sns_obj *sns_obj;
    td_s32 ret;

    sns_obj = sample_comm_isp_get_sns_obj(g_sns_type[isp_dev]);
    if (sns_obj == TD_NULL) {
        printf("sensor %d not exist!\n", g_sns_type[isp_dev]);
        return TD_FAILURE;
    }

    ae_lib.id = isp_dev;
#ifdef CONFIG_OT_ISP_THERMO_SUPPORT
    thermo_lib.id = isp_dev;
#endif
    awb_lib.id = isp_dev;
    ret = strncpy_s(ae_lib.lib_name, sizeof(ae_lib.lib_name), OT_AE_LIB_NAME, sizeof(OT_AE_LIB_NAME));
    sample_check_eok_return(ret, TD_FAILURE);
#ifdef CONFIG_OT_ISP_THERMO_SUPPORT
    ret = strncpy_s(thermo_lib.lib_name, sizeof(thermo_lib.lib_name), OT_THERMO_LIB_NAME, sizeof(OT_THERMO_LIB_NAME));
    sample_check_eok_return(ret, TD_FAILURE);
#endif
    ret = strncpy_s(awb_lib.lib_name, sizeof(awb_lib.lib_name), OT_AWB_LIB_NAME, sizeof(OT_AWB_LIB_NAME));
    sample_check_eok_return(ret, TD_FAILURE);
    if (sns_obj->pfn_un_register_callback != TD_NULL) {
        ret = sns_obj->pfn_un_register_callback(isp_dev, &ae_lib, &awb_lib);
        if (ret != TD_SUCCESS) {
            printf("sensor_unregister_callback failed with %#x!\n", ret);
            return ret;
        }
    } else {
        printf("sensor_unregister_callback failed with TD_NULL!\n");
    }

    g_sns_type[isp_dev] = SNS_TYPE_BUTT;

    return TD_SUCCESS;
}


td_s32 sample_comm_isp_bind_sns(ot_isp_dev isp_dev, sample_sns_type sns_type, td_s8 sns_dev)
{
    ot_isp_sns_commbus sns_bus_info;
    ot_isp_sns_type    bus_type;
    ot_isp_sns_obj    *sns_obj;
    td_s32 ret;

    sns_obj = sample_comm_isp_get_sns_obj(sns_type);
    if (sns_obj == TD_NULL) {
        printf("sensor %d not exist!\n", sns_type);
        return TD_FAILURE;
    }

    bus_type = sample_comm_get_sns_bus_type(sns_type);
    if (bus_type == OT_ISP_SNS_TYPE_I2C) {
        sns_bus_info.i2c_dev = sns_dev;
    } else {
        sns_bus_info.ssp_dev.bit4_ssp_dev = sns_dev;
        sns_bus_info.ssp_dev.bit4_ssp_cs  = 0;
    }

    if (sns_obj->pfn_set_bus_info != TD_NULL) {
        ret = sns_obj->pfn_set_bus_info(isp_dev, sns_bus_info);
        if (ret != TD_SUCCESS) {
            printf("set sensor bus info failed with %#x!\n", ret);
            return ret;
        }
    } else {
        printf("not support set sensor bus info!\n");
        return TD_FAILURE;
    }

    return TD_SUCCESS;
}

td_s32 sample_comm_isp_thermo_lib_uncallback(ot_isp_dev isp_dev)
{
#ifdef CONFIG_OT_ISP_THERMO_SUPPORT
    td_s32 ret;
    ot_isp_3a_alg_lib thermo_lib;

    thermo_lib.id = isp_dev;
    ret = strncpy_s(thermo_lib.lib_name, sizeof(thermo_lib.lib_name), OT_THERMO_LIB_NAME, sizeof(OT_THERMO_LIB_NAME));
    sample_check_eok_return(ret, TD_FAILURE);
    ret = ss_mpi_thermo_unregister(isp_dev, &thermo_lib);
    if (ret != TD_SUCCESS) {
        printf("ss_mpi_thermo_unregister failed with %#x!\n", ret);
        return ret;
    }
#endif
    return TD_SUCCESS;
}

td_s32 sample_comm_isp_ae_lib_callback(ot_isp_dev isp_dev)
{
    td_s32 ret;
    ot_isp_3a_alg_lib ae_lib;

    ae_lib.id = isp_dev;
    ret = strncpy_s(ae_lib.lib_name, sizeof(ae_lib.lib_name), OT_AE_LIB_NAME, sizeof(OT_AE_LIB_NAME));
    sample_check_eok_return(ret, TD_FAILURE);
    ret = ss_mpi_ae_register(isp_dev, &ae_lib);
    if (ret != TD_SUCCESS) {
        printf("ss_mpi_ae_register failed with %#x!\n", ret);
        return ret;
    }

    return TD_SUCCESS;
}

td_s32 sample_comm_isp_ae_lib_uncallback(ot_isp_dev isp_dev)
{
    td_s32 ret;
    ot_isp_3a_alg_lib ae_lib;

    ae_lib.id = isp_dev;
    ret = strncpy_s(ae_lib.lib_name, sizeof(ae_lib.lib_name), OT_AE_LIB_NAME, sizeof(OT_AE_LIB_NAME));
    sample_check_eok_return(ret, TD_FAILURE);
    ret = ss_mpi_ae_unregister(isp_dev, &ae_lib);
    if (ret != TD_SUCCESS) {
        printf("ss_mpi_ae_unregister failed with %#x!\n", ret);
        return ret;
    }

    return TD_SUCCESS;
}

td_s32 sample_comm_isp_awb_lib_callback(ot_isp_dev isp_dev)
{
    td_s32 ret;
    ot_isp_3a_alg_lib awb_lib;

    awb_lib.id = isp_dev;
    ret = strncpy_s(awb_lib.lib_name, sizeof(awb_lib.lib_name), OT_AWB_LIB_NAME, sizeof(OT_AWB_LIB_NAME));
    sample_check_eok_return(ret, TD_FAILURE);
    ret = ss_mpi_awb_register(isp_dev, &awb_lib);
    if (ret != TD_SUCCESS) {
        printf("ss_mpi_awb_register failed with %#x!\n", ret);
        return ret;
    }

    return TD_SUCCESS;
}

td_s32 sample_comm_isp_awb_lib_uncallback(ot_isp_dev isp_dev)
{
    td_s32 ret;
    ot_isp_3a_alg_lib awb_lib;

    awb_lib.id = isp_dev;
    ret = strncpy_s(awb_lib.lib_name, sizeof(awb_lib.lib_name), OT_AWB_LIB_NAME, sizeof(OT_AWB_LIB_NAME));
    sample_check_eok_return(ret, TD_FAILURE);
    ret = ss_mpi_awb_unregister(isp_dev, &awb_lib);
    if (ret != TD_SUCCESS) {
        printf("ss_mpi_awb_unregister failed with %#x!\n", ret);
        return ret;
    }

    return TD_SUCCESS;
}

td_void sample_common_isp_switch_set_exposure(const sample_vi_cfg *vi_cfg)
{
    td_u32 i;
    ot_vi_pipe vi_pipe;
    td_u32 exposure;
    ot_isp_exp_info exp_info = {0};
    ot_isp_sns_obj *sns_obj;
    ot_isp_init_attr init_attr = {0};

    for (i = 0; i < vi_cfg->grp_info.grp_num; i++) {
        vi_pipe = vi_cfg->grp_info.fusion_grp_attr[i].pipe_id[0];
        ss_mpi_isp_query_exposure_info(vi_pipe, &exp_info);
        exposure = exp_info.exposure;
        sns_obj = sample_comm_isp_get_sns_obj(vi_cfg->sns_info.sns_type);
        if (sns_obj == TD_NULL) {
            printf("sensor %d not exist!\n", vi_cfg->sns_info.sns_type);
            return;
        }
        init_attr.exposure = exposure;
        sns_obj->pfn_set_init(vi_pipe, &init_attr);
    }
}

/******************************************************************************
* funciton : ISP Run
******************************************************************************/
static void *sample_comm_isp_thread(td_void *param)
{
    td_s32 ret;
    ot_isp_dev isp_dev;
    td_char thread_name[20];
    isp_dev = (ot_isp_dev)(td_uintptr_t)param;
    errno_t err;

    err = snprintf_s(thread_name, sizeof(thread_name), sizeof(thread_name) - 1, "ISP%d_RUN", isp_dev); /* 20,19 chars */
    if (err < 0) {
        return NULL;
    }
    prctl(PR_SET_NAME, thread_name, 0, 0, 0);

    printf("ISP Dev %d running !\n", isp_dev);
    ret = ss_mpi_isp_run(isp_dev);
    if (ret != TD_SUCCESS) {
        printf("OT_MPI_ISP_Run failed with %#x!\n", ret);
        return NULL;
    }

    return NULL;
}

td_s32 sample_comm_isp_run(ot_isp_dev isp_dev)
{
    td_s32 ret;
    pthread_attr_t *thread_attr = NULL;

    ret = pthread_create(&g_isp_pid[isp_dev], thread_attr, sample_comm_isp_thread, (td_void*)(td_uintptr_t)isp_dev);
    if (ret != 0) {
        printf("create isp running thread failed!, error: %d\r\n", ret);
    }

    if (thread_attr != TD_NULL) {
        pthread_attr_destroy(thread_attr);
    }
    return ret;
}

td_void sample_comm_isp_stop(ot_isp_dev isp_dev)
{
    if (g_isp_pid[isp_dev]) {
        pthread_join(g_isp_pid[isp_dev], NULL);
        g_isp_pid[isp_dev] = 0;
    }
    return;
}


td_void sample_comm_all_isp_stop(td_void)
{
    ot_isp_dev isp_dev;

    for (isp_dev = 0; isp_dev < OT_VI_MAX_PIPE_NUM; isp_dev++) {
        sample_comm_isp_stop(isp_dev);
    }
}

/**
 * @brief   : 设置镜像、倒置翻转
 * @param    {sample_sns_type} sns_type sensor类型
 * @param    {ot_vi_pipe} vi_pipe VI 物理PIPE号
 * @param    {ot_isp_sns_mirrorflip_type} sns_mirror_flip sensor mirror-flip 枚举
 * @return   {td_s32} 0：成功 -1：失败
 */
td_s32 sample_comm_isp_set_mirror_flip(sample_sns_type sns_type,
                                   ot_vi_pipe vi_pipe,
                                   ot_isp_sns_mirrorflip_type sns_mirror_flip)
{
    ot_isp_sns_obj *sns_obj;

    sns_obj = sample_comm_isp_get_sns_obj(sns_type);
    if (sns_obj == TD_NULL)
    {
        printf("sensor %d not exist!\n", sns_type);
        return TD_FAILURE;
    }

    if (sns_obj->pfn_mirror_flip != TD_NULL)
    {
        sns_obj->pfn_mirror_flip(vi_pipe, sns_mirror_flip);
    }
    else
    {
        printf("sensor_mirror_flip failed with TD_NULL!\n");
        return TD_FAILURE;
    }

    return TD_SUCCESS;
}
