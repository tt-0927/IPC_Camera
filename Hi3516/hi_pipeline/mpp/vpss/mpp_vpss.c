/**
 * @FilePath     : mpp_vpss.c
 * @Author       : zhouzirui
 * @Date         : 2025-03-20 10:56:57
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2025-12-30 11:00:17
 * @Description  : 海思vpss模块封装
 */

#include "mpp_vpss.h"
#include "mpi_common.h"

static int mppVpss_set_chnLowdelay(HiVpss_S *pHandle, int nChn, td_bool bEnable, td_bool bOneBufEn);

/**
 * @brief   : 获取传感器对应的full_lines_std值
 * @param    {td_u32} sensor_type：传感器型号
 * @return   {td_u32} 传感器对应的full_lines_std值
 */
static td_u32 get_sensor_full_lines_std(td_u32 sensor_type)
{
    switch (sensor_type)
    {
    case SC4336P_MIPI_4M_30FPS_10BIT:
    case OS04D10_MIPI_4M_30FPS_10BIT:
    case GC4023_MIPI_4M_30FPS_10BIT:
    case SC431HAI_MIPI_4M_30FPS_10BIT:
    case SC431HAI_MIPI_4M_30FPS_10BIT_WDR2TO1:
        return 1500;
    case SC450AI_MIPI_4M_30FPS_10BIT:
    case SC450AI_MIPI_4M_30FPS_10BIT_WDR2TO1:
        return 1585;
    case SC500AI_MIPI_5M_30FPS_10BIT:
    case SC500AI_MIPI_5M_30FPS_10BIT_WDR2TO1:
    case SC533HAI_MIPI_5M_30FPS_10BIT:
    case SC533HAI_MIPI_5M_30FPS_10BIT_WDR2TO1:
        return 1700;
    default:
        return 1500; // 默认值
    }
}

/**
 * @brief   : 配置VPSS卷绕参数
 * @param    {HiVpss_S} *pHandle：vpss句柄
 * @param    {td_s32} nGrp：group组号
 * @param    {td_s32} nChn：通道号
 * @param    {ot_vpss_chn_attr} *pChnAttr：vpss通道属性指针
 * @return   {td_s32}成功返回0,失败返回-1
 */
static td_s32 mppVpss_config_wrap(HiVpss_S *pHandle, td_s32 nGrp, td_s32 nChn, const ot_vpss_chn_attr *pChnAttr)
{
    ot_vpss_venc_wrap_param stWrapParam;
    memset_s(&stWrapParam, sizeof(ot_vpss_venc_wrap_param), 0, sizeof(ot_vpss_venc_wrap_param));
    td_u32 u32BufLine = 0;

    /*设置帧率*/
    stWrapParam.frame_rate = pHandle->astVpssChnAttr[nChn].nDstFrameRate;
    if (pChnAttr->frame_rate.src_frame_rate > 0 && pChnAttr->frame_rate.dst_frame_rate > 0)
    {
        stWrapParam.frame_rate = pChnAttr->frame_rate.dst_frame_rate * 30 / pChnAttr->frame_rate.src_frame_rate;
    }

    stWrapParam.all_online = TD_TRUE;
    stWrapParam.full_lines_std = get_sensor_full_lines_std(SENSOR0_TYPE);

    /*设置大码流尺寸*/
    stWrapParam.large_stream_size.width = pHandle->astVpssChnAttr[nChn].nWidth;
    stWrapParam.large_stream_size.height = pHandle->astVpssChnAttr[nChn].nHeight;

    /*存在多路小码流,设置为面积（宽x高）最大的那一路*/
    int nMaxSmallStreamSizeWidth = 0;
    int nMaxSmallStreamSizeHeight = 0;
    for (int i = 1; i < pHandle->nVpssChnSum; i++)
    {
        if(nMaxSmallStreamSizeWidth < pHandle->astVpssChnAttr[i].nWidth)
        {
            nMaxSmallStreamSizeWidth = pHandle->astVpssChnAttr[i].nWidth;
        }
        if(nMaxSmallStreamSizeHeight < pHandle->astVpssChnAttr[i].nHeight)
        {
            nMaxSmallStreamSizeHeight = pHandle->astVpssChnAttr[i].nHeight;
        }
    }
    /*设置小码流尺寸（取最大值或默认值）*/
    stWrapParam.small_stream_size.width = nMaxSmallStreamSizeWidth;
    stWrapParam.small_stream_size.height = nMaxSmallStreamSizeHeight;
    /* 因业务压力过大，手动放大小码流尺寸，使申请更大的buf_line及size，避免miss start、ring back、ring buf full中断丢帧 */
    if (pHandle->astVpssChnAttr[nChn].bSmallStreamSize)
    {
        stWrapParam.small_stream_size.width = pHandle->astVpssChnAttr[nChn].nSmallStreamWidth;
        stWrapParam.small_stream_size.height = pHandle->astVpssChnAttr[nChn].nSmallStreamHeight;
    }
    mpi_vpss_log("小码流尺寸: %dx%d", stWrapParam.small_stream_size.width, stWrapParam.small_stream_size.height);

    /*获取VPSS-VENC卷绕模式下的buffer像素行数*/
    CHECK_API_RETURN(ss_mpi_sys_get_vpss_venc_wrap_buf_line(&stWrapParam, &u32BufLine));

    /*配置buffer属性*/
    ot_pic_buf_attr stBufAttr = {
        .width = pHandle->astVpssChnAttr[nChn].nWidth,
        .height = pHandle->astVpssChnAttr[nChn].nHeight,
        .align = OT_DEFAULT_ALIGN,
        .bit_width = OT_DATA_BIT_WIDTH_8,
        .pixel_format = pHandle->astVpssChnAttr[nChn].enChnPixelFormat,
        .compress_mode = pHandle->astVpssChnAttr[nChn].enChnComMode,
        .video_format = OT_VIDEO_FORMAT_LINEAR};

    /*计算卷绕buffer大小*/
    td_u32 wrap_buf_size = ot_comm_get_vpss_venc_wrap_buf_size(&stBufAttr, u32BufLine);

    /*配置卷绕属性*/
    ot_vpss_chn_buf_wrap_attr stChnWrapAttr = {
        .enable = TD_TRUE,
        .buf_line = u32BufLine,
        .buf_size = wrap_buf_size};

    mpi_vpss_log("卷绕buffer行数:%d 大小:%d", u32BufLine, wrap_buf_size);

    /*设置VPSS物理通道低延时卷绕属性*/
    return ss_mpi_vpss_set_chn_buf_wrap(nGrp, nChn, &stChnWrapAttr);
}

/**
 * @brief   : 设置模块参数
 * @param    {HiVpss_S} *pHandle：vpss句柄
 * @return   {*}成功返回0,失败返回-1
 */
static int mppVpss_set_mod_param(HiVpss_S *pHandle)
{
    if (NULL == pHandle)
    {
        return TD_FAILURE;
    }

    /*通过接口获取并设置模块参数 仅执行一次*/
    static int nFirst = 1;
    if (nFirst)
    {
        ot_vpss_grp nGrp = pHandle->nVpssGrp;
        ot_vpss_mod_param stModParam;
        memset(&stModParam, 0, sizeof(ot_vpss_mod_param));
        CHECK_API_RETURN(ss_mpi_vpss_get_mod_param(&stModParam));
        stModParam.max_out_width = OT_VPSS_MAX_OUT_IMG_WIDTH;
        stModParam.max_out_height = OT_VPSS_MAX_OUT_IMG_HEIGHT;
        CHECK_API_RETURN(ss_mpi_vpss_set_mod_param(&stModParam));

        /*设置VPSS在线模式使用的帧中断属性*/
        ot_frame_interrupt_attr stFrameInterruptAttr = {
            .interrupt_type = OT_FRAME_INTERRUPT_EARLY_END,
            .early_line = pHandle->stVpssGrpAttr.nMaxH / 2, /* 2 half */
        };
        CHECK_API_RETURN(ss_mpi_vpss_set_grp_frame_interrupt_attr(nGrp, &stFrameInterruptAttr));
        nFirst = 0;
    }

    return TD_SUCCESS;
}

/**
 * @brief       : 将VPSS的通道绑定到用户私有视频缓存VB池中
 * @author      : zhouzirui
 * @param        {HiVpss_S} *pHandle：句柄
 * @param        {ot_vpss_chn} nChn：VPSS通道号
 * @return       {*}成功返回0,失败返回-1
 */
static int mppVpss_attach_chn_vb_pool(HiVpss_S *pHandle, ot_vpss_chn nChn)
{
    ot_vpss_grp nGrp = pHandle->nVpssGrp;
    ot_vb_pool_cfg vb_pool_cfg;
    ot_pic_buf_attr stBufAttr;
    stBufAttr.width = pHandle->astVpssChnAttr[nChn].nMaxWidth;
    stBufAttr.height = pHandle->astVpssChnAttr[nChn].nMaxHeight;
    stBufAttr.align = OT_DEFAULT_ALIGN;
    stBufAttr.bit_width = OT_DATA_BIT_WIDTH_8;
    stBufAttr.pixel_format = pHandle->astVpssChnAttr[nChn].enChnPixelFormat;
    stBufAttr.compress_mode = pHandle->astVpssChnAttr[nChn].enChnComMode;
    stBufAttr.video_format = OT_VIDEO_FORMAT_LINEAR;

    vb_pool_cfg.blk_size = ot_common_get_pic_buf_size(&stBufAttr);
    vb_pool_cfg.blk_cnt = pHandle->astVpssChnAttr[nChn].nDepth > 0 ? \
                            pHandle->astVpssChnAttr[nChn].nDepth + 2: 2;
    // if(nChn == 0)
    // {
    //     vb_pool_cfg.blk_cnt = 3;
    // }
    memcpy(vb_pool_cfg.mmz_name, MMZ_NAME, sizeof(MMZ_NAME));
    vb_pool_cfg.remap_mode = OT_VB_REMAP_MODE_NONE;

    mpi_vpss_log("vb_pool_cfg.blk_size:%lld,%d,%s,%d", vb_pool_cfg.blk_size, vb_pool_cfg.blk_cnt, vb_pool_cfg.mmz_name, vb_pool_cfg.remap_mode);
    pHandle->aVbPoolId[nChn] = ss_mpi_vb_create_pool(&vb_pool_cfg);
    // ot_vb_pool pHandle->aVbPoolId[nChn] = ss_mpi_vb_create_ext_pool(&vb_pool_cfg);
    if (pHandle->aVbPoolId[nChn] == OT_VB_INVALID_POOL_ID)
    {
        mpi_vpss_log("创建vpss grp:%d 通道:%d 用户视频缓冲池失败:%d", nGrp, nChn, pHandle->aVbPoolId[nChn]);
        return TD_FAILURE;
    }
    /*设置VPSS通道的VB来源*/
    CHECK_API_RETURN(ss_mpi_vpss_set_chn_vb_src(nGrp, nChn, OT_VB_SRC_USER));
    /*将VPSS的通道绑定到某个视频缓存VB池中*/
    CHECK_API_RETURN(ss_mpi_vpss_attach_chn_vb_pool(nGrp, nChn, pHandle->aVbPoolId[nChn]));

    return TD_SUCCESS;
}

/**
 * @brief       : vpss初始化
 * @author      : zhouzirui
 * @param        {HiVpss_S} *pHandle：vpss句柄
 * @return       {*}成功返回0,失败返回-1
 */
static int mppVpss_init(HiVpss_S *pHandle)
{
    if(NULL == pHandle)
    {
        return TD_FAILURE;
    }
    ot_vpss_grp nGrp = pHandle->nVpssGrp;
    ot_vpss_grp_attr stGrpAttr;
    ot_vpss_chn_attr stChnAttr;
    memset(&stGrpAttr, 0, sizeof(ot_vpss_grp_attr));
    memset(&stChnAttr, 0, sizeof(ot_vpss_chn_attr));

    /*水平翻转*/
    td_bool nMirror = TD_FALSE;
    /*垂直翻转*/
    td_bool nFlip = TD_FALSE;

    /*设置模块参数*/
    mppVpss_set_mod_param(pHandle);

    /*创建一个VPSS Group组*/
    stGrpAttr.ie_en = TD_FALSE;
    stGrpAttr.dci_en = TD_FALSE;
    stGrpAttr.buf_share_en = TD_FALSE;
    stGrpAttr.mcf_en = TD_FALSE;
    stGrpAttr.max_width = pHandle->stVpssGrpAttr.nMaxW;
    stGrpAttr.max_height = pHandle->stVpssGrpAttr.nMaxH;
    stGrpAttr.max_dei_width = pHandle->stVpssGrpAttr.nMaxW;
    stGrpAttr.max_dei_height = pHandle->stVpssGrpAttr.nMaxH;
    stGrpAttr.pixel_format = pHandle->stVpssGrpAttr.enGrpPixelFormat;
    stGrpAttr.dei_mode = OT_VPSS_DEI_MODE_AUTO;
    stGrpAttr.buf_share_chn = nGrp;
    stGrpAttr.frame_rate.src_frame_rate = pHandle->stVpssGrpAttr.nSrcFrameRate;
    stGrpAttr.frame_rate.dst_frame_rate = pHandle->stVpssGrpAttr.nDstFrameRate;
    stGrpAttr.dynamic_range = OT_DYNAMIC_RANGE_SDR8; // 保留参数
    CHECK_API_RETURN(ss_mpi_vpss_create_grp(nGrp, &stGrpAttr));

    /*启用VPSS Group*/
    CHECK_API_RETURN(ss_mpi_vpss_start_grp(nGrp));

    /*设置VPSS物理通道属性*/
    for (ot_vpss_chn nChn = 0; nChn < pHandle->nVpssChnSum; nChn++)
    {
        memset(&stChnAttr, 0, sizeof(ot_vpss_chn_attr));
        stChnAttr.mirror_en = nMirror;
        stChnAttr.flip_en = nFlip;
        stChnAttr.border_en = TD_FALSE;
        stChnAttr.width = pHandle->astVpssChnAttr[nChn].nWidth;
        stChnAttr.height = pHandle->astVpssChnAttr[nChn].nHeight;
        stChnAttr.depth = pHandle->astVpssChnAttr[nChn].nDepth;
        stChnAttr.chn_mode = pHandle->astVpssChnAttr[nChn].enVpssChnMode;
        stChnAttr.pixel_format = pHandle->astVpssChnAttr[nChn].enChnPixelFormat;
        stChnAttr.compress_mode = pHandle->astVpssChnAttr[nChn].enChnComMode;
        stChnAttr.frame_rate.src_frame_rate = pHandle->astVpssChnAttr[nChn].nSrcFrameRate;
        stChnAttr.frame_rate.dst_frame_rate = pHandle->astVpssChnAttr[nChn].nDstFrameRate;
        // stChnAttr.border_attr
        stChnAttr.aspect_ratio.mode = OT_ASPECT_RATIO_NONE; // 无幅型比 不支持
        stChnAttr.dynamic_range = OT_DYNAMIC_RANGE_SDR8;    // 保留参数
        stChnAttr.video_format = OT_VIDEO_FORMAT_LINEAR;    // 视频格式
        CHECK_API_RETURN(ss_mpi_vpss_set_chn_attr(nGrp, nChn, &stChnAttr));

        /* 配置VPSS卷绕参数,仅物理通道0可配置卷绕 */
        if (nChn == 0)
        {
            if (pHandle->astVpssChnAttr[nChn].bWrapEnable == TD_TRUE)
                CHECK_API_RETURN(mppVpss_config_wrap(pHandle, nGrp, nChn, &stChnAttr));
            else /*普通通道配置VB池*/
                CHECK_API_RETURN(mppVpss_attach_chn_vb_pool(pHandle, nChn));
        }

        /*启用VPSS通道*/
        CHECK_API_RETURN(ss_mpi_vpss_enable_chn(nGrp, nChn));

        if (nChn == 1)
        {
            CHECK_API_RETURN(mppVpss_attach_chn_vb_pool(pHandle, nChn));
        }

        /*通道号大于1的配置VB池*/
        if (nChn > 1)
        {
            CHECK_API_RETURN(mppVpss_attach_chn_vb_pool(pHandle, nChn));
        }

        if(pHandle->astVpssChnAttr[nChn].bLowDelay == TD_TRUE && pHandle->astVpssChnAttr[nChn].bWrapEnable == TD_FALSE)
        {
            /*设置VPSS通道低延时参数*/
            CHECK_API_RETURN(mppVpss_set_chnLowdelay(pHandle, nChn, TD_TRUE, pHandle->astVpssChnAttr[nChn].bOneBufEn));
        }
    }

    return TD_SUCCESS;
}

/**
 * @brief       : vpss反初始化
 * @author      : zhouzirui
 * @param        {HiVpss_S} *pHandle：vpss句柄
 * @return       {*}成功返回0,失败返回-1
 */
static int mppVpss_uninit(HiVpss_S *pHandle)
{
    if(NULL == pHandle)
    {
        return TD_FAILURE;
    }
    ot_vpss_grp nGrp = pHandle->nVpssGrp;
    /*禁用VPSS通道*/
    for (ot_vpss_chn nChn = 0; nChn < pHandle->nVpssChnSum; nChn++)
    {
        if(pHandle->astVpssChnAttr[nChn].bLowDelay == TD_TRUE && pHandle->astVpssChnAttr[nChn].bWrapEnable == TD_FALSE)
        {
            /*设置VPSS通道低延时参数*/
            CHECK_API_RETURN(mppVpss_set_chnLowdelay(pHandle, nChn, TD_FALSE, pHandle->astVpssChnAttr[nChn].bOneBufEn));
        }

        CHECK_API_RETURN(ss_mpi_vpss_disable_chn(nGrp, nChn));
        if (pHandle->aVbPoolId[nChn] != OT_VB_INVALID_POOL_ID)
        {
            /*将VPSS的通道从某个视频缓存VB池中解绑定*/
            CHECK_API_RETURN(ss_mpi_vpss_detach_chn_vb_pool(nGrp, nChn));
            /*销毁一个视频缓存池*/
            CHECK_API_RETURN(ss_mpi_vb_destroy_pool(pHandle->aVbPoolId[nChn]));
        }
    }
    /*禁用VPSS Group*/
    CHECK_API_RETURN(ss_mpi_vpss_stop_grp(nGrp));
    /*销毁一个VPSS组*/
    CHECK_API_RETURN(ss_mpi_vpss_destroy_grp(nGrp));
    
    return TD_SUCCESS;
}

/**
 * @brief       : 设置通道属性
 * @author      : zhouzirui
 * @param        {HiVpss_S} *pHandle：vpss句柄
 * @param        {HiVpssChnAttr_S} *pChnAttr：通道参数
 * @param        {int} nVpssChn：设置通道
 * @return       {*}成功返回0,失败返回-1
 */
static int mppVpss_set_chnAttr(HiVpss_S *pHandle, HiVpssChnAttr_S *pChnAttr, int nVpssChn)
{
    if(!pHandle || !pChnAttr)
    {
        mpi_vpss_log("指针为空");
        return TD_FAILURE;
    }

    if(nVpssChn >= pHandle->nVpssChnSum)
    {
        mpi_vpss_log("设置的通道数大于最大通道数");
        return TD_FAILURE;
    }

    /*组*/
    int nVpssGrp = pHandle->nVpssGrp;

    memcpy_s(&pHandle->astVpssChnAttr[nVpssChn], sizeof(HiVpssChnAttr_S), pChnAttr, sizeof(HiVpssChnAttr_S));

    /* 设置VPSS通道低延时 */
    if(pHandle->astVpssChnAttr[nVpssChn].bLowDelay == TD_TRUE)
    {
        if(pHandle->astVpssChnAttr[nVpssChn].bFlip)
        {
            mppVpss_set_chnLowdelay(pHandle,
                                    nVpssChn,
                                    TD_FALSE,
                                    pHandle->astVpssChnAttr[nVpssChn].bOneBufEn);
        }
    }

    ot_vpss_chn_attr stVpssChnAttr;
    memset(&stVpssChnAttr, 0, sizeof(ot_vpss_chn_attr));

    /*获取VPSS物理通道属性*/
    CHECK_API_RETURN(ss_mpi_vpss_get_chn_attr(nVpssGrp, nVpssChn, &stVpssChnAttr));

    stVpssChnAttr.mirror_en = pHandle->astVpssChnAttr[nVpssChn].bMirror;
    stVpssChnAttr.flip_en = pHandle->astVpssChnAttr[nVpssChn].bFlip;
    // stVpssChnAttr.border_en = TD_FALSE;
    stVpssChnAttr.width = pHandle->astVpssChnAttr[nVpssChn].nWidth;
    stVpssChnAttr.height = pHandle->astVpssChnAttr[nVpssChn].nHeight;
    stVpssChnAttr.depth = pHandle->astVpssChnAttr[nVpssChn].nDepth;
    stVpssChnAttr.chn_mode = pHandle->astVpssChnAttr[nVpssChn].enVpssChnMode;
    stVpssChnAttr.pixel_format = pHandle->astVpssChnAttr[nVpssChn].enChnPixelFormat;
    stVpssChnAttr.compress_mode = pHandle->astVpssChnAttr[nVpssChn].enChnComMode;
    stVpssChnAttr.frame_rate.src_frame_rate = pHandle->astVpssChnAttr[nVpssChn].nSrcFrameRate;
    stVpssChnAttr.frame_rate.dst_frame_rate = pHandle->astVpssChnAttr[nVpssChn].nDstFrameRate;
    // stVpssChnAttr.border_attr
    // stVpssChnAttr.aspect_ratio.mode = OT_ASPECT_RATIO_NONE; // 无幅型比
    // stVpssChnAttr.dynamic_range = OT_DYNAMIC_RANGE_SDR8;    // 保留参数
    // stVpssChnAttr.video_format = OT_VIDEO_FORMAT_LINEAR;    // 视频格式

    /*设置通道参数*/
    CHECK_API_RETURN(ss_mpi_vpss_set_chn_attr(nVpssGrp, nVpssChn, &stVpssChnAttr));

    /* 设置VPSS通道低延时 */
    if(pHandle->astVpssChnAttr[nVpssChn].bLowDelay == TD_TRUE)
    {
        if(!pHandle->astVpssChnAttr[nVpssChn].bFlip)
        {
            mppVpss_set_chnLowdelay(pHandle,
                                    nVpssChn,
                                    TD_TRUE,
                                    pHandle->astVpssChnAttr[nVpssChn].bOneBufEn);
        }
    }

    mpi_vpss_log("vpssSetChn : grp:%d, chn:%d, w:%d, h:%d", nVpssGrp, nVpssChn, pChnAttr->nWidth, pChnAttr->nHeight);

    return TD_SUCCESS;
}

/**
 * @brief       : 获取通道属性
 * @author      : zhouzirui
 * @param        {HiVpss_S} *pHandle vpss句柄
 * @param        {HiVpssChnAttr_S} *pChnAttr：通道参数
 * @param        {int} nVpssChn：设置通道
 * @return       {*}成功返回0,失败返回-1
 */
static int mppVpss_get_chnAttr(HiVpss_S *pHandle, HiVpssChnAttr_S *pChnAttr, int nVpssChn)
{
    if(!pHandle || !pChnAttr)
    {
        mpi_vpss_log("指针为空");
        return TD_FAILURE;
    }

    if(nVpssChn >= pHandle->nVpssChnSum)
    {
        mpi_vpss_log("获取的通道数大于最大通道数");
        return TD_FAILURE;
    }

    memcpy_s(pChnAttr, sizeof(HiVpssChnAttr_S), &pHandle->astVpssChnAttr[nVpssChn], sizeof(HiVpssChnAttr_S));

    return TD_SUCCESS;
}


/**
 * @brief   : 设置通道裁剪
 * @param    {HiVpss_S} *pHandle：vpss句柄
 * @param    {ot_vpss_crop_info} *pCropInfo：CROP参数
 * @param    {int} nVpssChn：设置通道
 * @return   {int}成功返回0,失败返回-1
 */
static int mppVpss_set_chnCrop(HiVpss_S *pHandle, ot_vpss_crop_info *pCropInfo, int nVpssChn)
{
    if(!pHandle || !pCropInfo)
    {
        mpi_vpss_log("指针为空");
        return TD_FAILURE;
    }

    if(nVpssChn >= pHandle->nVpssChnSum)
    {
        mpi_vpss_log("设置的通道数大于最大通道数");
        return TD_FAILURE;
    }

    if (pCropInfo->crop_rect.x >= pHandle->astVpssChnAttr[nVpssChn].nMaxWidth ||
        pCropInfo->crop_rect.y >= pHandle->astVpssChnAttr[nVpssChn].nMaxHeight ||
        pCropInfo->crop_rect.width > (td_u32) pHandle->astVpssChnAttr[nVpssChn].nMaxWidth ||
        pCropInfo->crop_rect.height > (td_u32) pHandle->astVpssChnAttr[nVpssChn].nMaxHeight ||
        pCropInfo->crop_rect.x + pCropInfo->crop_rect.width > (td_u32) pHandle->astVpssChnAttr[nVpssChn].nMaxWidth ||
        pCropInfo->crop_rect.y + pCropInfo->crop_rect.height > (td_u32) pHandle->astVpssChnAttr[nVpssChn].nMaxHeight)
    {
        mpi_vpss_log("设置的区域大小不合法");
        return TD_FAILURE;
    }

    /*组*/
    int nVpssGrp = pHandle->nVpssGrp;

    CHECK_API_RETURN(ss_mpi_vpss_set_chn_crop(nVpssGrp, nVpssChn, pCropInfo));

    mpi_vpss_log("vpssSetChnCrop : grp: %d, chn: %d, enable: %d, crop_mode :%d, crop_rect: [%d,%d][%d,%d]", nVpssGrp, nVpssChn, pCropInfo->enable, pCropInfo->crop_mode, pCropInfo->crop_rect.x, pCropInfo->crop_rect.y, pCropInfo->crop_rect.width, pCropInfo->crop_rect.height);

    return TD_SUCCESS;
}

/**
 * @brief   : 设置VPSS通道低延时
 * @param    {HiVpss_S} *pHandle vpss句柄
 * @param    {int} nChn 通道号
 * @param    {td_bool} bEnable 是否使能低延时
 * @param    {td_bool} bOneBufEn 是否使能单Buffer模式
 * @return   {int} 成功返回0,失败返回-1
 */
static int mppVpss_set_chnLowdelay(HiVpss_S *pHandle, int nChn, td_bool bEnable, td_bool bOneBufEn)
{
    if (!pHandle || nChn < 0 || nChn >= OT_VPSS_MAX_CHN_NUM)
    {
        mpi_vpss_log("参数错误");
        return TD_FAILURE;
    }

    ot_low_delay_info stLowDelayInfo = { .enable = bEnable,
                                         .line_cnt = 128,
                                         //  .line_cnt = pHandle->astVpssChnAttr[nChn].nHeight / 2,
                                         .one_buf_en = bOneBufEn };

    /*配置VPSS通道低延时*/
    CHECK_API_RETURN(ss_mpi_vpss_set_chn_low_delay(pHandle->nVpssGrp, nChn, &stLowDelayInfo));

    return TD_SUCCESS;
}

/**
 * @brief   : 重新设置卷绕
 * @param    {HiVpss_S} *pHandle vpss句柄
 * @param    {int} nWidth 图像宽
 * @param    {int} nHeight 图像高
 * @return   {int} 成功返回0,失败返回-1
 */
static int mppVpss_reset_wrap(HiVpss_S *pHandle, int nWidth, int nHeight)
{
    if (!pHandle || pHandle->astVpssChnAttr[0].bWrapEnable != TD_TRUE)
    {
        mpi_vpss_log("参数错误");
        return TD_FAILURE;
    }

    ot_vpss_grp nGrp = pHandle->nVpssGrp;
    ot_vpss_chn nChn = 0;
    /* 禁用VPSS通道 */
    CHECK_API_RETURN(ss_mpi_vpss_disable_chn(nGrp, nChn));

    /* 关闭卷绕,释放buf */
    ot_vpss_chn_buf_wrap_attr stChnWrapAttr;
    memset_s(&stChnWrapAttr, sizeof(ot_vpss_chn_buf_wrap_attr), 0, sizeof(ot_vpss_chn_buf_wrap_attr));
    CHECK_API_RETURN(ss_mpi_vpss_get_chn_buf_wrap(nGrp, nChn, &stChnWrapAttr));
    stChnWrapAttr.enable = TD_FALSE;
    CHECK_API_RETURN(ss_mpi_vpss_set_chn_buf_wrap(nGrp, nChn, &stChnWrapAttr));

    ot_vpss_chn_attr stChnAttr;
    memset_s(&stChnAttr, sizeof(ot_vpss_chn_attr), 0, sizeof(ot_vpss_chn_attr));
    CHECK_API_RETURN(ss_mpi_vpss_get_chn_attr(nGrp, nChn, &stChnAttr));
    pHandle->astVpssChnAttr[nChn].nWidth = nWidth;
    pHandle->astVpssChnAttr[nChn].nHeight = nHeight;

    stChnAttr.width = nWidth;
    stChnAttr.height = nHeight;
    CHECK_API_RETURN(ss_mpi_vpss_set_chn_attr(nGrp, nChn, &stChnAttr));

    /* 开启卷绕 */
    CHECK_API_RETURN(mppVpss_config_wrap(pHandle, nGrp, nChn, &stChnAttr));

    /*启用VPSS通道*/
    CHECK_API_RETURN(ss_mpi_vpss_enable_chn(nGrp, nChn));

    return TD_SUCCESS;
}

/**
 * @brief       : 获取通道一帧图像
 * @author      : zhouzirui
 * @param        {HiVpss_S} *pHandle：vpss句柄
 * @param        {int} nVpssChn：通道号
 * @param        {ot_video_frame_info} *pFrameInfo：图像帧
 * @param        {int} nTimeoutMs：超时时间
 * @return       {*}成功返回0,失败返回-1
 */
static int mppVpss_get_chnFrame(HiVpss_S *pHandle, int nVpssChn, ot_video_frame_info *pFrameInfo, int nTimeoutMs)
{
    if (!pHandle || !pFrameInfo || nVpssChn < 0 || nVpssChn >= OT_VPSS_MAX_CHN_NUM || nTimeoutMs < -1)
    {
        mpi_vpss_log("mppVpss_get_chnFrame 参数错误");
        return TD_FAILURE;
    }

    /*获取VPSS通道图像帧*/
    CHECK_API_RETURN(ss_mpi_vpss_get_chn_frame(pHandle->nVpssGrp, nVpssChn, pFrameInfo, nTimeoutMs));
    
    return TD_SUCCESS;
}

/**
 * @brief       : 释放通道一帧图像
 * @author      : zhouzirui
 * @param        {HiVpss_S} *pHandle：vpss句柄
 * @param        {int} nVpssChn
 * @param        {ot_video_frame_info} *pFrameInfo：图像帧
 * @return       {*}成功返回0,失败返回-1
 */
static int mppVpss_release_chnFrame(HiVpss_S *pHandle, int nVpssChn, ot_video_frame_info *pFrameInfo)
{
    if (!pHandle || !pFrameInfo || nVpssChn < 0 || nVpssChn >= OT_VPSS_MAX_CHN_NUM)
    {
        mpi_vpss_log("mppVpss_release_chnFrame 参数错误");
        return TD_FAILURE;
    }

    /*释放VPSS通道图像帧*/
    CHECK_API_RETURN(ss_mpi_vpss_release_chn_frame(pHandle->nVpssGrp, nVpssChn, pFrameInfo));

    return TD_SUCCESS;
}

/**
 * @brief   : 从Group获取一帧原始图像
 * @param    {HiVpss_S} *pHandle vpss句柄
 * @param    {ot_video_frame_info} *pFrameInfo 图像帧
 * @param    {int} nTimeoutMs 超时时间
 * @return   {int} 成功返回0,失败返回-1
 */
static int mppVpss_get_grpFrame(HiVpss_S *pHandle, ot_video_frame_info *pFrameInfo, int nTimeoutMs)
{
    if (!pHandle || !pFrameInfo || nTimeoutMs < -1)
    {
        mpi_vpss_log("mppVpss_get_grpFrame 参数错误");
        return TD_FAILURE;
    }

    /*获取VPSS通道图像帧*/
    CHECK_API_RETURN(ss_mpi_vpss_get_grp_frame(pHandle->nVpssGrp, pFrameInfo, nTimeoutMs));

    return TD_SUCCESS;
}

/**
 * @brief   : 释放一帧原始图像
 * @param    {HiVpss_S} *pHandle vpss句柄
 * @param    {ot_video_frame_info} *pFrameInfo 图像帧
 * @return   {int} 成功返回0,失败返回-1
 */
static int mppVpss_release_grpFrame(HiVpss_S *pHandle, ot_video_frame_info *pFrameInfo)
{
    if (!pHandle || !pFrameInfo)
    {
        mpi_vpss_log("mppVpss_release_grpFrame 参数错误");
        return TD_FAILURE;
    }

    /*释放VPSS通道图像帧*/
    CHECK_API_RETURN(ss_mpi_vpss_release_grp_frame(pHandle->nVpssGrp, pFrameInfo));

    return TD_SUCCESS;
}

HiVpss_S *mppVpss_alloc(HiVpssNeedParam_S stNeedParam)
{
    HiVpss_S *pHandle = (HiVpss_S*)malloc(sizeof(HiVpss_S));
    HiVpssChnAttr_S *pVpssChnAttr = NULL, *pVpssChnAttrSrc = NULL;
    memset(pHandle, 0, sizeof(HiVpss_S));

    //info /**********************必需参数***************************/
    pHandle->nVpssChnSum = stNeedParam.nVpssChnSum;
    pHandle->nVpssGrp = stNeedParam.nVpssGrp;
    pHandle->stVpssGrpAttr.enGrpPixelFormat = stNeedParam.stVpssGrpAttr.enGrpPixelFormat;
    pHandle->stVpssGrpAttr.enGrpComMode = stNeedParam.stVpssGrpAttr.enGrpComMode;
    pHandle->stVpssGrpAttr.nMaxW = stNeedParam.stVpssGrpAttr.nMaxW;
    pHandle->stVpssGrpAttr.nMaxH = stNeedParam.stVpssGrpAttr.nMaxH;
    pHandle->stVpssGrpAttr.nSrcFrameRate = stNeedParam.stVpssGrpAttr.nSrcFrameRate;
    pHandle->stVpssGrpAttr.nDstFrameRate = stNeedParam.stVpssGrpAttr.nDstFrameRate;

    for (int nVpssChn = 0; nVpssChn < OT_VPSS_MAX_CHN_NUM; nVpssChn++)
    {
        pVpssChnAttr = &pHandle->astVpssChnAttr[nVpssChn];
        pVpssChnAttrSrc = &stNeedParam.astVpssChnAttr[nVpssChn];
        pVpssChnAttr->enChnPixelFormat = pVpssChnAttrSrc->enChnPixelFormat;
        pVpssChnAttr->enVpssChnMode = pVpssChnAttrSrc->enVpssChnMode;
        pVpssChnAttr->enChnComMode = pVpssChnAttrSrc->enChnComMode;
        pVpssChnAttr->nWidth = pVpssChnAttrSrc->nWidth;
        pVpssChnAttr->nHeight = pVpssChnAttrSrc->nHeight;
        pVpssChnAttr->nMaxWidth = pVpssChnAttrSrc->nMaxWidth;
        pVpssChnAttr->nMaxHeight = pVpssChnAttrSrc->nMaxHeight;
        pVpssChnAttr->nDepth = pVpssChnAttrSrc->nDepth;
        pVpssChnAttr->nSrcFrameRate = pVpssChnAttrSrc->nSrcFrameRate;
        pVpssChnAttr->nDstFrameRate = pVpssChnAttrSrc->nDstFrameRate;
        pVpssChnAttr->bWrapEnable = pVpssChnAttrSrc->bWrapEnable;
        pVpssChnAttr->bLowDelay = pVpssChnAttrSrc->bLowDelay;
        pVpssChnAttr->bOneBufEn = pVpssChnAttrSrc->bOneBufEn;
        pVpssChnAttr->bSmallStreamSize = pVpssChnAttrSrc->bSmallStreamSize;
        pVpssChnAttr->nSmallStreamWidth = pVpssChnAttrSrc->nSmallStreamWidth;
        pVpssChnAttr->nSmallStreamHeight = pVpssChnAttrSrc->nSmallStreamHeight;
        pHandle->aVbPoolId[nVpssChn] = OT_VB_INVALID_POOL_ID;
    }

    // info /**********************函数列表***************************/
    pHandle->mppVpss_init               = mppVpss_init;
    pHandle->mppVpss_uninit             = mppVpss_uninit;
    pHandle->mppVpss_set_chnAttr        = mppVpss_set_chnAttr;
    pHandle->mppVpss_get_chnAttr        = mppVpss_get_chnAttr;
    pHandle->mppVpss_set_chnCrop        = mppVpss_set_chnCrop;
    pHandle->mppVpss_set_chnLowdelay    = mppVpss_set_chnLowdelay;
    pHandle->mppVpss_reset_wrap         = mppVpss_reset_wrap;
    pHandle->mppVpss_get_chnFrame       = mppVpss_get_chnFrame;
    pHandle->mppVpss_release_chnFrame   = mppVpss_release_chnFrame;
    pHandle->mppVpss_get_grpFrame       = mppVpss_get_grpFrame;
    pHandle->mppVpss_release_grpFrame   = mppVpss_release_grpFrame;

    return pHandle;
}

void mppVpss_release(HiVpss_S* pHandle)
{
    if(!pHandle)
    {
        free(pHandle);
        pHandle = NULL;
    }
}

