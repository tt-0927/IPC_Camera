/**
 * @FilePath     : rockit_venc.c
 * @Author       : luoyk
 * @Date         : 2022年05月11日 星期三 16时16分04秒
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2025-12-08 09:48:25
 * @Description  : RK VENC 视频编码
 */

#include "rockit_venc.h"

/* 全局静态变量-是否设置了单包模式 */
static RK_BOOL gs_bIsSetOneStreamBuf = RK_FALSE;

// info /**********************内部接口***************************/
/**
 * @brief   : H.264编码通道码率控制器属性填充
 * @param    {RkVenc_S} *pHandle 句柄
 * @param    {VENC_CHN_ATTR_S} *pChnAttr 编码通道属性结构体指针
 * @return   {int} 成功返回0,失败返回-1
 */
static int rockitVenc_h264_attr_fill(RkVenc_S *pHandle, VENC_CHN_ATTR_S *pChnAttr)
{
    if (NULL == pChnAttr || NULL == pHandle)
    {
        return RK_FAILURE;
    }

    switch (pChnAttr->stRcAttr.enRcMode)
    {
    case VENC_RC_MODE_H264CBR:
        pChnAttr->stRcAttr.stH264Cbr = (VENC_H264_CBR_S){ .u32Gop = pHandle->stNeedParam.nGop,
                                                .u32SrcFrameRateNum = pHandle->stNeedParam.nInFrameRate,
                                                .u32SrcFrameRateDen = 1,
                                                .fr32DstFrameRateNum = pHandle->stNeedParam.nOutFrameRate,
                                                .fr32DstFrameRateDen = 1,
                                                .u32BitRate = pHandle->stExParam.nBitRate,
                                                .u32StatTime = VENC_RT_STAT_TIME_DEFAULT };
        break;
    case VENC_RC_MODE_H264VBR:
        pChnAttr->stRcAttr.stH264Vbr = (VENC_H264_VBR_S){ .u32Gop = pHandle->stNeedParam.nGop,
                                                .u32SrcFrameRateNum = pHandle->stNeedParam.nInFrameRate,
                                                .u32SrcFrameRateDen = 1,
                                                .fr32DstFrameRateNum = pHandle->stNeedParam.nOutFrameRate,
                                                .fr32DstFrameRateDen = 1,
                                                .u32BitRate = pHandle->stExParam.nBitRate,
                                                .u32MaxBitRate = pHandle->stExParam.nMaxBitRate,
                                                .u32MinBitRate = pHandle->stExParam.nMinBitRate,
                                                .u32StatTime = VENC_RT_STAT_TIME_DEFAULT };
        break;
    case VENC_RC_MODE_H264AVBR:
        pChnAttr->stRcAttr.stH264Avbr = (VENC_H264_AVBR_S){ .u32Gop = pHandle->stNeedParam.nGop,
                                                  .u32SrcFrameRateNum = pHandle->stNeedParam.nInFrameRate,
                                                  .u32SrcFrameRateDen = 1,
                                                  .fr32DstFrameRateNum = pHandle->stNeedParam.nOutFrameRate,
                                                  .fr32DstFrameRateDen = 1,
                                                  .u32BitRate = pHandle->stExParam.nBitRate,
                                                  .u32MaxBitRate = pHandle->stExParam.nMaxBitRate,
                                                  .u32MinBitRate = pHandle->stExParam.nMinBitRate,
                                                  .u32StatTime = VENC_RT_STAT_TIME_DEFAULT };
        break;
    default:
        mpi_venc_log("不支持的 H264 rc_mode: %d", pChnAttr->stRcAttr.enRcMode);
        return RK_FAILURE;
    }
    return RK_SUCCESS;
}

/**
 * @brief   : H.265编码通道码率控制器属性填充
 * @param    {RkVenc_S} *pHandle 句柄
 * @param    {VENC_CHN_ATTR_S} *pChnAttr 编码通道属性结构体指针
 * @return   {int} 成功返回0,失败返回-1
 */
static int rockitVenc_h265_attr_fill(RkVenc_S *pHandle, VENC_CHN_ATTR_S *pChnAttr)
{
    if (NULL == pChnAttr || NULL == pHandle)
    {
        return RK_FAILURE;
    }

    switch (pChnAttr->stRcAttr.enRcMode)
    {
    case VENC_RC_MODE_H265CBR:
        pChnAttr->stRcAttr.stH264Cbr = (VENC_H265_CBR_S){ .u32Gop = pHandle->stNeedParam.nGop,
                                                .u32SrcFrameRateNum = pHandle->stNeedParam.nInFrameRate,
                                                .u32SrcFrameRateDen = 1,
                                                .fr32DstFrameRateNum = pHandle->stNeedParam.nOutFrameRate,
                                                .fr32DstFrameRateDen = 1,
                                                .u32BitRate = pHandle->stExParam.nBitRate,
                                                .u32StatTime = VENC_RT_STAT_TIME_DEFAULT };
        break;
    case VENC_RC_MODE_H265VBR:
        pChnAttr->stRcAttr.stH264Vbr = (VENC_H265_VBR_S){ .u32Gop = pHandle->stNeedParam.nGop,
                                                .u32SrcFrameRateNum = pHandle->stNeedParam.nInFrameRate,
                                                .u32SrcFrameRateDen = 1,
                                                .fr32DstFrameRateNum = pHandle->stNeedParam.nOutFrameRate,
                                                .fr32DstFrameRateDen = 1,
                                                .u32BitRate = pHandle->stExParam.nBitRate,
                                                .u32MaxBitRate = pHandle->stExParam.nMaxBitRate,
                                                .u32MinBitRate = pHandle->stExParam.nMinBitRate,
                                                .u32StatTime = VENC_RT_STAT_TIME_DEFAULT };
        break;
    case VENC_RC_MODE_H265AVBR:
        pChnAttr->stRcAttr.stH264Avbr = (VENC_H265_AVBR_S){ .u32Gop = pHandle->stNeedParam.nGop,
                                                  .u32SrcFrameRateNum = pHandle->stNeedParam.nInFrameRate,
                                                  .u32SrcFrameRateDen = 1,
                                                  .fr32DstFrameRateNum = pHandle->stNeedParam.nOutFrameRate,
                                                  .fr32DstFrameRateDen = 1,
                                                  .u32BitRate = pHandle->stExParam.nBitRate,
                                                  .u32MaxBitRate = pHandle->stExParam.nMaxBitRate,
                                                  .u32MinBitRate = pHandle->stExParam.nMinBitRate,
                                                  .u32StatTime = VENC_RT_STAT_TIME_DEFAULT };
        break;
    default:
        mpi_venc_log("不支持的 H265 rc_mode: %d", pChnAttr->stRcAttr.enRcMode);
        return RK_FAILURE;
    }
    return RK_SUCCESS;
}

/**
 * @brief   : JPEG编码通道码率控制器属性填充
 * @param    {RkVenc_S} *pHandle 句柄
 * @param    {VENC_CHN_ATTR_S} *pChnAttr 编码通道属性结构体指针
 * @return   {int} 成功返回0,失败返回-1
 */
static int rockitVenc_jpeg_attr_fill(RkVenc_S *pHandle, VENC_CHN_ATTR_S *pChnAttr)
{
    if (NULL == pChnAttr || NULL == pHandle)
    {
        return RK_FAILURE;
    }

    // note rv1126b芯片暂不支持
    /* 是否支持 DCF */
    pChnAttr->stVencAttr.stAttrJpege.bSupportDCF = RK_FALSE;
    /* 多图片配置 */
    pChnAttr->stVencAttr.stAttrJpege.stMPFCfg.u8LargeThumbNailNum = 0;
    /* 图像接收模式 */
    pChnAttr->stVencAttr.stAttrJpege.enReceiveMode = VENC_PIC_RECEIVE_SINGLE;
    return RK_SUCCESS;
}

/**
 * @brief   : MJPEG编码通道码率控制器属性填充
 * @param    {RkVenc_S} *pHandle 句柄
 * @param    {VENC_CHN_ATTR_S} *pChnAttr 编码通道属性结构体指针
 * @return   {int} 成功返回0,失败返回-1
 */
static int rockitVenc_mjpeg_attr_fill(RkVenc_S *pHandle, VENC_CHN_ATTR_S *pChnAttr)
{
    if (NULL == pChnAttr || NULL == pHandle)
    {
        return RK_FAILURE;
    }

    switch (pChnAttr->stRcAttr.enRcMode)
    {
    case VENC_RC_MODE_MJPEGCBR:
        pChnAttr->stRcAttr.stMjpegCbr = (VENC_MJPEG_CBR_S){ .u32SrcFrameRateNum = pHandle->stNeedParam.nInFrameRate,
                                                            .u32SrcFrameRateDen = 1,
                                                            .fr32DstFrameRateNum = pHandle->stNeedParam.nOutFrameRate,
                                                            .fr32DstFrameRateDen = 1,
                                                            .u32BitRate = pHandle->stExParam.nBitRate,
                                                            .u32StatTime = VENC_RT_STAT_TIME_DEFAULT };
        break;
    case VENC_RC_MODE_MJPEGVBR:
        pChnAttr->stRcAttr.stMjpegVbr = (VENC_MJPEG_VBR_S){ .u32SrcFrameRateNum = pHandle->stNeedParam.nInFrameRate,
                                                            .u32SrcFrameRateDen = 1,
                                                            .fr32DstFrameRateNum = pHandle->stNeedParam.nOutFrameRate,
                                                            .fr32DstFrameRateDen = 1,
                                                            .u32BitRate = pHandle->stExParam.nBitRate,
                                                            .u32MaxBitRate = pHandle->stExParam.nMaxBitRate,
                                                            .u32MinBitRate = pHandle->stExParam.nMinBitRate,
                                                            .u32StatTime = VENC_RT_STAT_TIME_DEFAULT };
        break;
    default:
        mpi_venc_log("不支持的 MJPEG rc_mode: %d", pChnAttr->stRcAttr.enRcMode);
        return RK_FAILURE;
    }
    return RK_SUCCESS;
}

/**
 * @brief   : 设置H.264/H.265协议编码通道的类型属性
 * @param    {RkVenc_S} *pHandle：句柄
 * @return   {int} 成功返回0,失败返回-1
 */
static int rockitVenc_set_codec_type_attr(RkVenc_S *pHandle)
{
    if (!pHandle)
    {
        return RK_FAILURE;
    }

    if (pHandle->stNeedParam.enCodec == RK_VIDEO_ID_JPEG || pHandle->stNeedParam.enCodec == RK_VIDEO_ID_MJPEG)
    {
        return RK_SUCCESS;
    }

    /* 编码通道 */
    VENC_CHN nChn = pHandle->stNeedParam.nChn;
    /*编码协议类型*/
    RK_CODEC_ID_E enCodec = pHandle->stNeedParam.enCodec;

    // info 帧率=time_scale/ (2* num_units_in_tick)
    /*目标帧率*/
    int nFrameRate = pHandle->stNeedParam.nOutFrameRate;
    /*时钟 ticks 的单位数（分子）*/
    RK_U32 u32Num_units_in_tick = 1000;
    if (enCodec == RK_VIDEO_ID_AVC)
    {
        VENC_H264_VUI_S stH264Vui;
        /* 获取H.264协议编码通道的Vui参数 */
        CHECK_API_RETURN(RK_MPI_VENC_GetH264Vui(nChn, &stH264Vui));
        stH264Vui.stVuiTimeInfo.timing_info_present_flag = 1;             // 是否包含时间信息（1bit）
        stH264Vui.stVuiTimeInfo.fixed_frame_rate_flag = 1;                // 是否为固定帧率（1bit）
        stH264Vui.stVuiTimeInfo.num_units_in_tick = u32Num_units_in_tick; // 时钟 ticks 的单位数（分子）
        stH264Vui.stVuiTimeInfo.time_scale = nFrameRate * 2 * u32Num_units_in_tick; // 时间刻度（分母，单位：Hz）
        // stH264Vui.stVuiVideoSignal.video_full_range_flag = 0;
        /* 设置H.264协议编码通道的Vui参数 */
        CHECK_API_RETURN(RK_MPI_VENC_SetH264Vui(nChn, &stH264Vui));
    }
    else if (enCodec == RK_VIDEO_ID_HEVC)
    {
        VENC_H265_VUI_S stH265Vui;
        /* 获取H.265协议编码通道的Vui参数 */
        CHECK_API_RETURN(RK_MPI_VENC_GetH265Vui(nChn, &stH265Vui));
        stH265Vui.stVuiTimeInfo.timing_info_present_flag = 1;                   // 标志位，是否包含时间信息
        stH265Vui.stVuiTimeInfo.num_units_in_tick = u32Num_units_in_tick;       // 时钟 ticks 的单位数（分子）
        stH265Vui.stVuiTimeInfo.time_scale = nFrameRate * u32Num_units_in_tick; // 时间刻度（分母，单位：Hz）
        stH265Vui.stVuiTimeInfo.num_ticks_poc_diff_one_minus1 = 1;              // POC 差值对应的 ticks 数减1
        /* 设置H.265协议编码通道的Vui参数 */
        CHECK_API_RETURN(RK_MPI_VENC_SetH265Vui(nChn, &stH265Vui));
    }

    /* 使能参考帧共享属性可以节省内存，但是⽆法⽀持超⼤帧重编和去呼吸效应 */
    // VENC_CHN_REF_BUF_SHARE_S stVencChnRefBufShare;
    // stVencChnRefBufShare.bEnable = RK_TRUE;
    // CHECK_API_RETURN(RK_MPI_VENC_SetChnRefBufShareAttr(nChn, &stVencChnRefBufShare));

    return RK_SUCCESS;
}

/**
 * @brief       : 设置编码jpeg相关配置
 * @param        {RkVenc_S} *pHandle：句柄
 * @return       {int} 成功返回0,失败返回-1
 */
static int rockitVenc_set_jpeg_config(RkVenc_S *pHandle)
{
    if (NULL == pHandle)
    {
        return RK_FAILURE;
    }

    /* 编码通道 */
    VENC_CHN nChn = pHandle->stNeedParam.nChn;

    if(pHandle->stNeedParam.enCodec == RK_VIDEO_ID_JPEG)
    {
        VENC_JPEG_PARAM_S stJpegParam;
        /* 获取 JPEG 编码的参数集合 */
        CHECK_API_RETURN(RK_MPI_VENC_GetJpegParam(nChn, &stJpegParam));
        /* 品质因数 */
        stJpegParam.u32Qfactor = pHandle->stExParam.u32Qfactor;
        /* 设置 JPEG 编码的参数集合 */
        CHECK_API_RETURN(RK_MPI_VENC_SetJpegParam(nChn, &stJpegParam));
    }
    return RK_SUCCESS;
}

/**
 * @brief   : 根据编码格式设置该编码为单包还是多包模式
 * @param    {RkVenc_S} *pHandle 句柄
 * @param    {RK_CODEC_ID_E} enType 编码格式
 * @param    {RK_U32} u32OneStreamBuf 编码模式
 * @return   {int} 成功返回0,失败返回-1
 */
static int rockitVenc_set_one_stream_buf(RkVenc_S *pHandle, RK_CODEC_ID_E enType, RK_U32 u32OneStreamBuf)
{
    /* 设置编码输出为单包模式，I帧中组合SPS、PPS、SEI等 */
    /* 设置编码输出为多包模式，I帧中不组合SPS、PPS、SEI等 */
    VENC_PARAM_MOD_S stModParam;
    switch (enType)
    {
    case RK_VIDEO_ID_AVC:
        stModParam.enVencModType = MODTYPE_H264E;
        break;
    case RK_VIDEO_ID_HEVC:
        stModParam.enVencModType = MODTYPE_H265E;
        break;
    case RK_VIDEO_ID_JPEG:
    case RK_VIDEO_ID_MJPEG:
        stModParam.enVencModType = MODTYPE_JPEGE;
        break;
    default:
        return RK_FAILURE;
    }
    /* 获取编码相关的模块参数 */
    CHECK_API_RETURN(RK_MPI_VENC_GetModParam(&stModParam));
    switch (enType)
    {
    case RK_VIDEO_ID_AVC:
        stModParam.stH264eModParam.u32OneStreamBuffer = u32OneStreamBuf;
        break;
    case RK_VIDEO_ID_HEVC:
        stModParam.stH265eModParam.u32OneStreamBuffer = u32OneStreamBuf;
        break;
    case RK_VIDEO_ID_JPEG:
    case RK_VIDEO_ID_MJPEG:
        stModParam.stJpegeModParam.u32OneStreamBuffer = u32OneStreamBuf;
        break;
    default:
        return RK_FAILURE;
    }
    /* 设置编码相关的模块参数 */
    CHECK_API_RETURN(RK_MPI_VENC_SetModParam(&stModParam));

    return RK_SUCCESS;
}

/**
 * @brief   : 编码通道码率控制器的高级参数填充
 * @param    {RkVenc_S} *pHandle：句柄
 * @param    {VENC_RC_PARAM_S} *pRcParam：编码通道码率控制器的高级参数
 * @return   {int} 成功返回0,失败返回-1
 */
static int rockitVenc_rc_param_fill(RkVenc_S *pHandle, VENC_RC_PARAM_S *pRcParam)
{
    if (!pHandle || !pRcParam)
    {
        return RK_FAILURE;
    }

    /* 根据码率大小和码流平滑值设置 row_qp_delta（0~10）控制宏块行QP变化范围 */
    int row_qp_delta = 0;
    int bitrate = pHandle->stExParam.nBitRate;
    int smoothLevel = pHandle->stExParam.nBitrateSmoothing; /* 码流平滑值: 0-100 [清晰->平滑] */

    /**
     * 码流平滑策略:
     * smoothLevel=0: 关闭平滑,使用基础QP步进(根据码率)
     * smoothLevel=1-30 (清晰优先): 小幅降低QP步进,允许较大质量波动
     * smoothLevel=31-70 (平衡模式): 中等QP步进,平衡清晰度与平滑度
     * smoothLevel=71-100 (平滑优先): 最小QP步进,追求极致平滑
     */
    if (smoothLevel <= 0)
    {
        /* 关闭平滑,使用基础QP步进(根据码率自适应) */
        if (bitrate <= 512)
            row_qp_delta = 7;
        else if (bitrate <= 1024)
            row_qp_delta = 6;
        else if (bitrate <= 2048)
            row_qp_delta = 5;
        else if (bitrate <= 4096)
            row_qp_delta = 4;
        else if (bitrate <= 8192)
            row_qp_delta = 3;
        else
            row_qp_delta = 2;
    }
    else
    {
        /* 启用码流平滑,平滑值越高,QP步进越小,码流越平稳 */
        int base_delta;

        /* 先根据码率确定基础步进 */
        if (bitrate <= 512)
            base_delta = 7;
        else if (bitrate <= 1024)
            base_delta = 6;
        else if (bitrate <= 2048)
            base_delta = 5;
        else if (bitrate <= 4096)
            base_delta = 4;
        else if (bitrate <= 8192)
            base_delta = 3;
        else
            base_delta = 2;

        /* 根据平滑级别调整QP步进 */
        if (smoothLevel <= 30)
        {
            /* 清晰优先: 保持80%基础步进 */
            row_qp_delta = (base_delta * 80) / 100;
        }
        else if (smoothLevel <= 70)
        {
            /* 平衡模式: 保持50%基础步进 */
            row_qp_delta = (base_delta * 50) / 100;
        }
        else
        {
            /* 平滑优先: 保持20%基础步进 */
            row_qp_delta = (base_delta * 20) / 100;
        }

        /* 确保最小值为0(完全平滑),最大值为10 */
        if (row_qp_delta > 10)
            row_qp_delta = 10;
    }

    /* 判断是否为变码率模式 */
    RK_CODEC_ID_E enCodec = pHandle->stNeedParam.enCodec;
    VENC_RC_MODE_E rcMode = pHandle->stExParam.enRcMode;
    RK_BOOL is_vbr_mode =RK_FALSE;
    switch (rcMode)
    {
    case VENC_RC_MODE_H264VBR:
    case VENC_RC_MODE_H264AVBR:
    case VENC_RC_MODE_H265VBR:
    case VENC_RC_MODE_H265AVBR:
    case VENC_RC_MODE_MJPEGVBR:
        is_vbr_mode = RK_TRUE;
        break;
    default:
        is_vbr_mode = RK_FALSE;
        break;
    }

    /* QP参数配置 */
    int min_qp, max_qp, min_i_qp, max_i_qp, delta_ip_qp;
    int qfactor, min_qfactor, max_qfactor;
    int image_quality;

    /* 如果是变码率,使用用户设置的图像质量;如果是定码率,默认使用中等质量(60) */
    if (is_vbr_mode)
    {
        image_quality = pHandle->stExParam.nImageQuality;
    }
    else
    {
        image_quality = 60; /* 定码率默认中等质量 */
    }

    /* 根据图像质量映射QP参数 */
    switch (image_quality)
    {
    case 1: /* 最低质量 - 追求最小码流 */
        min_qp = 38;
        max_qp = 51;
        min_i_qp = 36;
        max_i_qp = 48;
        delta_ip_qp = 3; /* I帧质量略高于P帧 */
        qfactor = 95;
        min_qfactor = 90;
        max_qfactor = 99;
        break;
    case 20: /* 较低质量 - 较小码流 */
        min_qp = 34;
        max_qp = 48;
        min_i_qp = 32;
        max_i_qp = 45;
        delta_ip_qp = 2;
        qfactor = 90;
        min_qfactor = 85;
        max_qfactor = 95;
        break;
    case 40: /* 低质量 - 平衡偏省流 */
        min_qp = 30;
        max_qp = 44;
        min_i_qp = 28;
        max_i_qp = 42;
        delta_ip_qp = 2;
        qfactor = 80;
        min_qfactor = 70;
        max_qfactor = 88;
        break;
    case 60: /* 中等质量 - 推荐默认值 */
        min_qp = 26;
        max_qp = 41;
        min_i_qp = 24;
        max_i_qp = 40;
        delta_ip_qp = 2;
        qfactor = 70;
        min_qfactor = 60;
        max_qfactor = 80;
        break;
    case 80: /* 较高质量 - 较大码流 */
        min_qp = 20;
        max_qp = 39;
        min_i_qp = 18;
        max_i_qp = 38;
        delta_ip_qp = 2;
        qfactor = 50;
        min_qfactor = 40;
        max_qfactor = 65;
        break;
    case 100: /* 最高质量 - 追求最佳画质 */
        min_qp = 15;
        max_qp = 38;
        min_i_qp = 15;
        max_i_qp = 37;
        delta_ip_qp = 2; /* I帧和P帧质量接近 */
        qfactor = 30;
        min_qfactor = 20;
        max_qfactor = 50;
        break;
    default: /* 容错处理,默认中等质量 */
        min_qp = 26;
        max_qp = 41;
        min_i_qp = 24;
        max_i_qp = 40;
        delta_ip_qp = 2;
        qfactor = 70;
        min_qfactor = 60;
        max_qfactor = 80;
        mpi_venc_log("警告: 图像质量参数异常(%d), 使用默认中等质量", image_quality);
        break;
    }

    /* 根据编码格式设置对应的QP参数 */
    switch (enCodec)
    {
    case RK_VIDEO_ID_AVC:
        /* P帧QP参数 */
        pRcParam->stParamH264.u32StepQp = row_qp_delta; /* QP步进(控制宏块行QP变化) */ // qp最大步进参数设置
        pRcParam->stParamH264.u32MaxQp = max_qp; // P帧最大qp设置。取值范围：[1, 51]。默认51
        pRcParam->stParamH264.u32MinQp = min_qp; // P帧最小qp设置。取值范围：[1, u32MaxQp]。默认10。
        /* I帧QP参数 */
        pRcParam->stParamH264.u32MaxIQp = max_i_qp; // I帧最大qp设置。取值范围：[1, 51]。默认46。
        pRcParam->stParamH264.u32MinIQp = min_i_qp; // I帧最小qp设置。取值范围：[1, u32MaxIQp]。默认24。
        /* I帧前几帧P帧平均QP与I帧的差值，即s32DeltIpQp=average(PPPP)-I；
        该值越大，I帧质量相对越好，呼吸效应越明显；
        该值越小，I帧质量相对越差，呼吸效应相对较弱，但是也会影响后续的P帧质量。
        取值范围：[-7, 7]。默认2。 */
        pRcParam->stParamH264.s32DeltIpQp = delta_ip_qp; 
        /* 帧级QP参数(更宽松的范围,允许码率控制器有更大调整空间) */
        pRcParam->stParamH264.u32FrmMaxQp = (max_qp < 46) ? (max_qp + 5) : 51; // P帧最大帧级qp设置。取值范围：[1, 51]。
        pRcParam->stParamH264.u32FrmMinQp = (min_qp > 5) ? (min_qp - 5) : 1; // P帧最小帧级qp设置。取值范围：[1, 51]。
        pRcParam->stParamH264.u32FrmMaxIQp = (max_i_qp < 46) ? (max_i_qp + 5) : 51; // I帧最大帧级qp设置。取值范围：[1, 51]。
        pRcParam->stParamH264.u32FrmMinIQp = (min_i_qp > 5) ? (min_i_qp - 5) : 1; // I帧最小帧级qp设置。取值范围：[1, 51]。
        /* 重编码和场景切换参数 */
        pRcParam->stParamH264.s32MaxReEncodeTimes = 3; // 最大重编次数：[0, 3]。默认1。
        pRcParam->stParamH264.u32MotionStaticSwitchFrmQp = max_qp; // 动静切换帧级qp设置。取值范围：[1, 51]。
        break;
    case RK_VIDEO_ID_HEVC:
        /* P帧QP参数 */
        pRcParam->stParamH265.u32StepQp = row_qp_delta; /* QP步进(控制宏块行QP变化) */ // qp最大步进参数设置
        pRcParam->stParamH265.u32MaxQp = max_qp; // P帧最大qp设置。取值范围：[1, 51]。默认51
        pRcParam->stParamH265.u32MinQp = min_qp; // P帧最小qp设置。取值范围：[1, u32MaxQp]。默认10。
        /* I帧QP参数 */
        pRcParam->stParamH265.u32MaxIQp = max_i_qp; // I帧最大qp设置。取值范围：[1, 51]。默认46。
        pRcParam->stParamH265.u32MinIQp = min_i_qp; // I帧最小qp设置。取值范围：[1, u32MaxIQp]。默认24。
        /* I帧前几帧P帧平均QP与I帧的差值，即s32DeltIpQp=average(PPPP)-I；
        该值越大，I帧质量相对越好，呼吸效应越明显；
        该值越小，I帧质量相对越差，呼吸效应相对较弱，但是也会影响后续的P帧质量。
        取值范围：[-7, 7]。默认2。 */
        pRcParam->stParamH265.s32DeltIpQp = delta_ip_qp; 
        /* 帧级QP参数(更宽松的范围,允许码率控制器有更大调整空间) */
        pRcParam->stParamH265.u32FrmMaxQp = (max_qp < 46) ? (max_qp + 5) : 51; // P帧最大帧级qp设置。取值范围：[1, 51]。
        pRcParam->stParamH265.u32FrmMinQp = (min_qp > 5) ? (min_qp - 5) : 1; // P帧最小帧级qp设置。取值范围：[1, 51]。
        pRcParam->stParamH265.u32FrmMaxIQp = (max_i_qp < 46) ? (max_i_qp + 5) : 51; // I帧最大帧级qp设置。取值范围：[1, 51]。
        pRcParam->stParamH265.u32FrmMinIQp = (min_i_qp > 5) ? (min_i_qp - 5) : 1; // I帧最小帧级qp设置。取值范围：[1, 51]。
        /* 重编码和场景切换参数 */
        pRcParam->stParamH265.s32MaxReEncodeTimes = 3; // 最大重编次数：[0, 3]。默认1。
        pRcParam->stParamH265.u32MotionStaticSwitchFrmQp = max_qp; // 动静切换帧级qp设置。取值范围：[1, 51]。
        break;
    case RK_VIDEO_ID_MJPEG:
        /* MJPEG使用Qfactor参数(值越小质量越高) */
        pRcParam->stParamMjpeg.u32Qfactor = qfactor; // 目标质量因子 取值范围： [1, 99]。默认值：70。
        pRcParam->stParamMjpeg.u32MaxQfactor = max_qfactor; // 最大Qfactor(最低质量) 取值范围： [u32Qfactor, 99]。默认值：99。
        pRcParam->stParamMjpeg.u32MinQfactor = min_qfactor; // 最小Qfactor(最高质量) 取值范围： [1, u32Qfactor]。默认值：30。
        break;
    default:
        mpi_venc_log("不支持的编码格式: %d", enCodec);
        break;
    }

    /* 日志输出当前配置 */
    if (enCodec == RK_VIDEO_ID_AVC || enCodec == RK_VIDEO_ID_HEVC)
    {
        mpi_venc_log("[RC参数] 模式:%s, 质量级别:%d, P帧QP[%d-%d], I帧QP[%d-%d], ΔIP:%d, 码率:%dkbps",
                     is_vbr_mode ? "VBR" : "CBR",
                     image_quality,
                     min_qp,
                     max_qp,
                     min_i_qp,
                     max_i_qp,
                     delta_ip_qp,
                     bitrate);
    }
    else if (enCodec == RK_VIDEO_ID_MJPEG)
    {
        mpi_venc_log("[RC参数] MJPEG 质量级别:%d, Qfactor[%d-%d-%d], 码率:%dkbps",
                     image_quality,
                     min_qfactor,
                     qfactor,
                     max_qfactor,
                     bitrate);
    }

    return RK_SUCCESS;
}

/**
 * @brief   : 设置通道码率控制高级参数
 * @param    {RkVenc_S*} pHandle 句柄
 * @return   {int} 成功返回0,失败返回-1
 */
static int rockitVenc_set_rcParam(RkVenc_S *pHandle)
{
    if (NULL == pHandle)
    {
        return RK_FAILURE;
    }

    /*编码通道*/
    VENC_CHN nChn = pHandle->stNeedParam.nChn;

    VENC_RC_PARAM_S stRcParam;
    /* 获取通道码率控制⾼级参数 */
    CHECK_API_RETURN(RK_MPI_VENC_GetRcParam(nChn, &stRcParam));
    rockitVenc_rc_param_fill(pHandle, &stRcParam);
    /* 设置通道码率控制⾼级参数 */
    CHECK_API_RETURN(RK_MPI_VENC_SetRcParam(nChn, &stRcParam));

    return RK_SUCCESS;
}

/**
 * @brief   : 设置编码码流平滑-P帧刷Islice
 * @param    {RkVenc_S} *pHandle 句柄
 * @return   {int} 成功返回0,失败返回-1
 */
static int rockitVenc_set_intraRefresh(RkVenc_S *pHandle)
{
    if (NULL == pHandle)
    {
        return RK_FAILURE;
    }

    /* h264和h265有效 */
    if (pHandle->stNeedParam.enCodec != RK_VIDEO_ID_AVC && pHandle->stNeedParam.enCodec != RK_VIDEO_ID_HEVC)
    {
        return RK_SUCCESS;
    }

    /* 编码通道 */
    VENC_CHN nChn = pHandle->stNeedParam.nChn;

    /* 刷 I slice 的参数 */
    VENC_INTRA_REFRESH_S stIntraRefresh;
    /* 获取 P 帧刷 I slice 的参数 */
    CHECK_API_RETURN(RK_MPI_VENC_GetIntraRefresh(nChn, &stIntraRefresh));
    
    /* 码流平滑值 */
    int smoothLevel = pHandle->stExParam.nBitrateSmoothing;
    if (smoothLevel <= 0)
    {
        /* 关闭P 帧刷 Islice */
        stIntraRefresh.bRefreshEnable = RK_FALSE;
        /* 设置 P 帧刷 I slice 的参数 */
        CHECK_API_RETURN(RK_MPI_VENC_SetIntraRefresh(nChn, &stIntraRefresh));
        mpi_venc_log("关闭码流平滑-P帧刷Islice");
    }
    else
    {
        /* 计算宏块总数 (16x16对齐) */
        RK_U32 mbW = (pHandle->stNeedParam.unWidth + 15) / 16;
        RK_U32 mbH = (pHandle->stNeedParam.unHeight + 15) / 16;
        // RK_U32 totalMB = mbW * mbH;

        /*
        * 平滑策略映射:
        * smoothLevel=1-30   (清晰优先): 快速刷新, 1-2秒完成全屏
        * smoothLevel=31-70  (平衡模式): 中速刷新, 2-4秒完成全屏
        * smoothLevel=71-100 (平滑优先): 慢速刷新, 4-8秒完成全屏
        */
        float refreshCycleSec;
        if (smoothLevel <= 30)
        {
            /* 清晰优先: 1.0s - 2.0s */
            refreshCycleSec = 1.0f + (smoothLevel / 30.0f) * 1.0f;
        }
        else if (smoothLevel <= 70)
        {
            /* 平衡模式: 2.0s - 4.0s */
            refreshCycleSec = 2.0f + ((smoothLevel - 30) / 40.0f) * 2.0f;
        }
        else
        {
            /* 平滑优先: 4.0s - 8.0s */
            refreshCycleSec = 4.0f + ((smoothLevel - 70) / 30.0f) * 4.0f;
        }

        /* 计算刷新周期帧数 */
        RK_U32 refreshFrames = (RK_U32) (pHandle->stNeedParam.nOutFrameRate * refreshCycleSec);
        if (refreshFrames < 1)
        {
            refreshFrames = 1;
        }

        /* 计算每帧刷新的宏块行数/列数 */
        RK_U32 refreshNum;
        stIntraRefresh.enIntraRefreshMode = INTRA_REFRESH_ROW; /* 按行刷新 */
        if (stIntraRefresh.enIntraRefreshMode == INTRA_REFRESH_ROW)
        {
            /* 按行刷新: 计算每帧刷新多少行 */
            refreshNum = (mbH + refreshFrames - 1) / refreshFrames;
            /* 限制范围: 至少1行, 最多不超过总行数 */
            if (refreshNum < 1)
                refreshNum = 1;
            if (refreshNum > mbH)
                refreshNum = mbH;
        }
        else
        {
            /* 按列刷新: 计算每帧刷新多少列 */
            refreshNum = (mbW + refreshFrames - 1) / refreshFrames;
            /* 限制范围: 至少1列, 最多不超过总列数 */
            if (refreshNum < 1)
                refreshNum = 1;
            if (refreshNum > mbW)
                refreshNum = mbW;
        }

        /* 配置帧内刷新参数 */
        stIntraRefresh.bRefreshEnable = RK_TRUE;
        stIntraRefresh.u32RefreshNum = refreshNum;
        stIntraRefresh.u32ReqIQp = 0; /* 文档说明暂不支持,设为0 */

        /* 应用帧内刷新 */
        CHECK_API_RETURN(RK_MPI_VENC_SetIntraRefresh(pHandle->stNeedParam.nChn, &stIntraRefresh));

        mpi_venc_log("[码流平滑] 平滑值:%d, 刷新周期:%.2fs(%d帧), 每帧刷新:%d%s, 总宏块:%dx%d",
                    smoothLevel,
                    refreshCycleSec,
                    refreshFrames,
                    refreshNum,
                    (stIntraRefresh.enIntraRefreshMode == INTRA_REFRESH_ROW) ? "行" : "列",
                    mbW,
                    mbH);
    }

    return RK_SUCCESS;
}

// info /**********************外部回调接口***************************/

/**
 * @brief   : 发送编码数据
 * @note    : 未正确实现
 * @param    {RkVenc_S} *pHandle 句柄
 * @param    {void} *pParam
 * @param    {int} nSize
 * @param    {void} *(*send_pic)(void *pData, void *pParam, int nSize)
 * @return   {*}
 */
static int rockitVenc_send_frame(RkVenc_S *pHandle, void *pParam, int nSize, void *(*send_pic)(void *pData, void *pParam, int nSize))
{
    return RK_SUCCESS;
    if (NULL == pHandle)
    {
        return RK_FAILURE;
    }

    if (NULL == pParam)
    {
        return RK_FAILURE;
    }

    int nRet = 0;
    VIDEO_FRAME_INFO_S stFrame;

    memset(&stFrame, 0, sizeof(VIDEO_FRAME_INFO_S));

    MB_BLK blk = RK_MPI_MB_GetMB(pHandle->vencPoolInput, nSize, RK_TRUE);
    int8_t *pVirAddr = (int8_t *) RK_MPI_MB_Handle2VirAddr(blk);

    /*填充数据*/
    if (send_pic)
    {
        send_pic(pVirAddr, pParam, nSize);
    }
    else
    {
        memcpy(pVirAddr, pParam, nSize);
    }

    RK_MPI_SYS_MmzFlushCache(blk, RK_FALSE);
    stFrame.stVFrame.pMbBlk = blk;
    stFrame.stVFrame.u32Width = pHandle->stNeedParam.unWidth;
    stFrame.stVFrame.u32Height = pHandle->stNeedParam.unHeight;
    stFrame.stVFrame.u32VirWidth = pHandle->stNeedParam.unVirWidth;
    stFrame.stVFrame.u32VirHeight = pHandle->stNeedParam.unVirHeight;
    stFrame.stVFrame.enPixelFormat = pHandle->stNeedParam.enPixFormat;
    stFrame.stVFrame.u32FrameFlag |= 0;
    stFrame.stVFrame.enCompressMode = pHandle->stNeedParam.enCompressMode;
    nRet = RK_MPI_VENC_SendFrame(pHandle->stNeedParam.nChn, &stFrame, -1);

    if (nRet != RK_SUCCESS)
    {
        mpi_venc_log("ven chn %d send fail error=%x", pHandle->stNeedParam.nChn, nRet);
    }
    RK_MPI_MB_ReleaseMB(blk);
    return nRet;
}

/**
 * @brief   : 发送原始图像进行编码
 * @param    {RkVenc_S} *pHandle 句柄
 * @param    {VIDEO_FRAME_INFO_S} *pVFrame 视频图像帧信息结构体指针
 * @param    {int} nTimeOutMs 超时时间 -1：阻塞接⼝；0：⾮阻塞接⼝；⼤于0：超时等待时间。单位为毫秒（ms）
 * @return   {int} 成功返回0,失败返回-1
 */
static int rockitVenc_send_VFrame(RkVenc_S *pHandle, VIDEO_FRAME_INFO_S *pVFrame, int nTimeOutMs)
{
    /* 用户发送原始图像进⾏编码 */
    CHECK_API_RETURN(RK_MPI_VENC_SendFrame(pHandle->stNeedParam.nChn, pVFrame, nTimeOutMs));
    return RK_SUCCESS;
}

/**
 * @brief   : 获取编码的码流
 * @param    {RkVenc_S} *pHandle 句柄
 * @param    {VENC_STREAM_S} *pFrame 码流结构体指针
 * @param    {VENC_PACK_S} *pPackArray 帧码流包结构指针
 * @param    {int} nPackCount 获取包数量
 * @param    {int} nTimeOutMs 超时时间 -1：阻塞接⼝；0：⾮阻塞接⼝；⼤于0：超时等待时间。单位为毫秒（ms）
 * @return   {int} 成功返回0,失败返回-1
 */
static int rockitVenc_get_stream(RkVenc_S *pHandle, VENC_STREAM_S *pFrame, VENC_PACK_S *pPackArray, uint32_t nPackCount, int nTimeOutMs)
{
    if (NULL == pHandle || NULL == pFrame || NULL == pPackArray)
    {
        return RK_FAILURE;
    }

    /*允许获取多个包（SPS, PPS, IDR帧等） */
    pFrame->u32PackCount = nPackCount; 
    pFrame->pstPack = pPackArray;

    /* 获取编码码流 */
    int ret = RK_MPI_VENC_GetStream(pHandle->stNeedParam.nChn, pFrame, nTimeOutMs);
    return ret;
}

/**
 * @brief   : 获取码流的虚拟地址
 * @note    : 获取一个内存缓存池中的缓存块的用户态虚拟地址
 * @param    {VENC_PACK_S*} pPack 帧码流包结构指针
 * @return   {uint8_t *} 码流的虚拟地址:成功, NULL:失败
 */
static uint8_t *rockitVenc_get_streamVirdata(VENC_PACK_S *pPack)
{
    if (NULL == pPack)
    {
        return NULL;
    }

    /* 获取一个内存缓存池中的缓存块的用户态虚拟地址 */
    return RK_MPI_MB_Handle2VirAddr(pPack->pMbBlk);
}

/**
 * @brief   : 释放码流缓存
 * @param    {RkVenc_S} *pHandle 句柄
 * @param    {VENC_STREAM_S} *pFrame 码流结构体指针
 * @return   {int} 成功返回0,失败返回-1
 */
static int rockitVenc_release_stream(RkVenc_S *pHandle, VENC_STREAM_S *pFrame)
{
    if (NULL == pHandle)
    {
        return RK_FAILURE;
    }

    /* 释放码流缓存 */
    CHECK_API_RETURN(RK_MPI_VENC_ReleaseStream(pHandle->stNeedParam.nChn, pFrame));
    return RK_SUCCESS;
}

/**
 * @brief   : 获取编码通道GOP
 * @param    {RkVenc_S*} pHandle 句柄
 * @param    {unsigned int} *uGop 获取返回的GOP指针
 * @return   {int} 成功返回0,失败返回-1
 */
static int rockitVenc_get_gop(RkVenc_S* pHandle,unsigned int *uGop)
{
    if (NULL == pHandle)
    {
        return RK_FAILURE;
    }

    VENC_CHN_ATTR_S stChnAttr;
    /* 获取编码通道的编码属性 */
    CHECK_API_RETURN(RK_MPI_VENC_GetChnAttr(pHandle->stNeedParam.nChn, &stChnAttr));
    switch (stChnAttr.stRcAttr.enRcMode)
    {
    case VENC_RC_MODE_H264CBR:
        *uGop = stChnAttr.stRcAttr.stH264Cbr.u32Gop;
        break;
    case VENC_RC_MODE_H265CBR:
        *uGop = stChnAttr.stRcAttr.stH265Cbr.u32Gop;
        break;
    case VENC_RC_MODE_H264VBR:
        *uGop = stChnAttr.stRcAttr.stH264Vbr.u32Gop;
        break;
    case VENC_RC_MODE_H265VBR:
        *uGop = stChnAttr.stRcAttr.stH265Vbr.u32Gop;
        break;
    case VENC_RC_MODE_H264AVBR:
        *uGop = stChnAttr.stRcAttr.stH264Avbr.u32Gop;
        break;
    case VENC_RC_MODE_H265AVBR:
        *uGop = stChnAttr.stRcAttr.stH265Avbr.u32Gop;
        break;
    default:
        return RK_FAILURE;
    }

    return RK_SUCCESS;
}

/**
* @brief   : 设置编码通道GOP
* @param    {RkVenc_S} *pHandle 句柄
* @param    {unsigned int} uGop 设置的GOP
* @return   {int} 成功返回0,失败返回-1
*/
static int rockitVenc_set_gop(RkVenc_S *pHandle, unsigned int uGop)
{
    if (NULL == pHandle)
    {
        return RK_FAILURE;
    }

    VENC_CHN_ATTR_S stChnAttr;
    /* 获取编码通道的编码属性 */
    CHECK_API_RETURN(RK_MPI_VENC_GetChnAttr(pHandle->stNeedParam.nChn, &stChnAttr));

    switch (stChnAttr.stRcAttr.enRcMode)
    {
    case VENC_RC_MODE_H264CBR:
        stChnAttr.stRcAttr.stH264Cbr.u32Gop = uGop;
        break;
    case VENC_RC_MODE_H265CBR:
        stChnAttr.stRcAttr.stH265Cbr.u32Gop = uGop;
        break;
    case VENC_RC_MODE_H264VBR:
        stChnAttr.stRcAttr.stH264Vbr.u32Gop = uGop;
        break;
    case VENC_RC_MODE_H265VBR:
        stChnAttr.stRcAttr.stH265Vbr.u32Gop = uGop;
        break;
    case VENC_RC_MODE_H264AVBR:
        stChnAttr.stRcAttr.stH264Avbr.u32Gop = uGop;
        break;
    case VENC_RC_MODE_H265AVBR:
        stChnAttr.stRcAttr.stH265Avbr.u32Gop = uGop;
        break;
    default:
        return RK_FAILURE;
    }

    /* 设置编码通道的编码属性 */
    CHECK_API_RETURN(RK_MPI_VENC_SetChnAttr(pHandle->stNeedParam.nChn, &stChnAttr));

    return RK_SUCCESS;
}
 
/**
 * @brief   : 设置h264/h265编码通道的感兴趣区域编码配置
 * @param    {RkVenc_S*} pHandle 句柄
 * @param    {VENC_ROI_ATTR_S} *pRoiAttr 感兴趣区域编码配置
 * @return   {int} 成功返回0,失败返回-1
 */
static int rockitVenc_set_roiAttr(RkVenc_S* pHandle, VENC_ROI_ATTR_S *pRoiAttr)
{
    if (NULL == pHandle)
    {
        return RK_FAILURE;
    }

    /* 设置 H.264/H.265 通道的 ROI 配置⾼级属性 */
    CHECK_API_RETURN(RK_MPI_VENC_SetRoiAttr(pHandle->stNeedParam.nChn, pRoiAttr));
    return RK_SUCCESS;
}

/**
 * @brief   : 设置通道编码裁剪缩放或者裁剪
 * @note    : x y w h 要2字节对齐，否则不成功
 * @param    {RkVenc_S} *pHandle 句柄
 * @param    {VENC_CROP_TYPE_E} enCropType 裁剪方式
 * @param    {RECT_S} *pstCorp 裁剪的区域
 * @param    {VENC_SCALE_RECT_S} *pstScale 裁剪缩放控制
 * @return   {*}
 */
static int rockitVenc_set_corpOrScale(RkVenc_S *pHandle, VENC_CROP_TYPE_E enCropType, RECT_S *pstCorp, VENC_SCALE_RECT_S *pstScale)
{
    if (NULL == pHandle)
    {
        return RK_FAILURE;
    }

    VENC_CHN_PARAM_S stParam;
    /* 获取通道参数 */
    CHECK_API_RETURN(RK_MPI_VENC_GetChnParam(pHandle->stNeedParam.nChn, &stParam));
    /*裁剪*/
    if (enCropType == VENC_CROP_ONLY)
    {
        if (NULL == pstCorp)
        {
            return RK_FAILURE;
        }
        memcpy(&stParam.stCropCfg.stCropRect, pstCorp, sizeof(RECT_S));
    }
    /*裁剪缩放*/
    else if (enCropType == VENC_CROP_SCALE)
    {
        if (NULL == pstScale)
        {
            return RK_FAILURE;
        }
        memcpy(&stParam.stCropCfg.stScaleRect, pstScale, sizeof(VENC_SCALE_RECT_S));
    }
    stParam.stCropCfg.enCropType = enCropType;
    /* 设置通道参数 */
    CHECK_API_RETURN(RK_MPI_VENC_SetChnParam(pHandle->stNeedParam.nChn, &stParam));
	return RK_SUCCESS;
}

/**
 * @brief   : 请求 IDR 帧
 * @param    {RkVenc_S} *pHandle 句柄
 * @return   {int} 成功返回0,失败返回-1
 */
static int rockitVenc_request_idr(RkVenc_S *pHandle)
{
    if (NULL == pHandle)
    {
        return RK_FAILURE;
    }

    /* 请求 IDR 帧 */
    CHECK_API_RETURN(RK_MPI_VENC_RequestIDR(pHandle->stNeedParam.nChn, RK_FALSE));
    return RK_SUCCESS;
}

/**
 * @brief   : venc初始化
 * @param    {RkVenc_S} *pHandle 句柄
 * @return   {int} 成功返回0,失败返回-1
 */
static int rockitVenc_init(RkVenc_S *pHandle)
{
    if (NULL == pHandle)
    {
        return RK_FAILURE;
    }

    /*编码通道*/
    VENC_CHN nChn = pHandle->stNeedParam.nChn;

    mpi_venc_log("编码通道[%d]开始初始化", nChn);

    /* 计算图像大小 */
    if (pHandle->stExParam.unBufferSize == 0)
    {
        mpi_venc_log("计算图像大小");
        PIC_BUF_ATTR_S stPicBufAttr;
        MB_PIC_CAL_S stMbPicCalResult;
        stPicBufAttr.u32Width = pHandle->stNeedParam.unWidth;
        stPicBufAttr.u32Height = pHandle->stNeedParam.unHeight;
        stPicBufAttr.enPixelFormat = pHandle->stNeedParam.enPixFormat;
        stPicBufAttr.enCompMode = pHandle->stNeedParam.enCompressMode;
        /* 计算输入图像的数据大小 */
        CHECK_API_RETURN(RK_MPI_CAL_COMM_GetPicBufferSize(&stPicBufAttr, &stMbPicCalResult));
        pHandle->stExParam.unBufferSize = stMbPicCalResult.u32MBSize;
        /*增加子码流编码缓冲区，防止录像写入卡顿，子码流编码缓冲区内存踩踏，导致文件异常*/
        if(RK_VENC_CHN_SUB == nChn)
        {    
            pHandle->stExParam.unBufferSize *= 4;
        }
        else if(RK_VENC_CHN_MAIN == nChn)
        {
            pHandle->stExParam.unBufferSize *= 2;
        }
        mpi_venc_log("计算图像大小完成,Size:[%d]",stMbPicCalResult.u32MBSize);
    }

    /* 输入内存池使用用户模式 */
    if (pHandle->stExParam.bAttachPool)
    {
        MB_POOL_CONFIG_S stMbPoolCfg;
        memset(&stMbPoolCfg, 0, sizeof(MB_POOL_CONFIG_S));
        /* 创建输入内存池 */
        stMbPoolCfg.u64MBSize = pHandle->stExParam.unBufferSize;
        stMbPoolCfg.u32MBCnt = pHandle->stExParam.unMbCnt;
        stMbPoolCfg.enAllocType = MB_ALLOC_TYPE_DMA;
        stMbPoolCfg.bPreAlloc = RK_TRUE;
        pHandle->vencPoolInput = RK_MPI_MB_CreatePool(&stMbPoolCfg);
        if (pHandle->vencPoolInput == MB_INVALID_POOLID)
        {
            mpi_venc_log("编码通道[%d],创建输入内存缓存池失败!", nChn);
            return RK_FAILURE;
        }
        mpi_venc_log("输入内存池创建完成");
    }

    /*创建编码通道*/
    VENC_CHN_ATTR_S stChnAttr;
    memset(&stChnAttr, 0, sizeof(VENC_CHN_ATTR_S));
    stChnAttr.stVencAttr.enType = pHandle->stNeedParam.enCodec;
    stChnAttr.stVencAttr.u32Profile = pHandle->stExParam.nProfile;
    stChnAttr.stVencAttr.enPixelFormat = pHandle->stNeedParam.enPixFormat;
    stChnAttr.stVencAttr.u32PicWidth = pHandle->stNeedParam.unWidth;
    stChnAttr.stVencAttr.u32PicHeight = pHandle->stNeedParam.unHeight;
    stChnAttr.stVencAttr.u32MaxPicWidth = pHandle->stNeedParam.unWidth;
    stChnAttr.stVencAttr.u32MaxPicHeight = pHandle->stNeedParam.unHeight;
    if (pHandle->stNeedParam.unVirWidth <= 0)
    {
        pHandle->stNeedParam.unVirWidth = pHandle->stNeedParam.unWidth;
    }
    if (pHandle->stNeedParam.unVirHeight <= 0)
    {
        pHandle->stNeedParam.unVirHeight = pHandle->stNeedParam.unHeight;
    }
    stChnAttr.stVencAttr.u32VirWidth = pHandle->stNeedParam.unVirWidth;
    stChnAttr.stVencAttr.u32VirHeight = pHandle->stNeedParam.unVirHeight;

    /*内存块数*/
    stChnAttr.stVencAttr.u32StreamBufCnt = pHandle->stExParam.unMbCnt;
    /*输入帧的大小*/
    stChnAttr.stVencAttr.u32BufSize = pHandle->stExParam.unBufferSize;

    stChnAttr.stRcAttr.enRcMode = pHandle->stExParam.enRcMode;
    if (stChnAttr.stVencAttr.enType == RK_VIDEO_ID_JPEG || stChnAttr.stVencAttr.enType == RK_VIDEO_ID_MJPEG)
    {
        stChnAttr.stGopAttr.enGopMode = VENC_GOPMODE_NORMALP;
        /* normalp和其他gop mode需要动态切换时，设置为1；如果仅有normalp，设置为0  */
        stChnAttr.stGopAttr.u32MaxLtrCount = 0;
        /* smartp和tsvc mode需要动态切换时，设置为1；如果仅有smartp，设置为0 */
        stChnAttr.stGopAttr.u32TsvcPreload = 0;
    }
    else
    {
        if(stChnAttr.stGopAttr.enGopMode == VENC_GOPMODE_SMARTP)
        {
            stChnAttr.stGopAttr.s32VirIdrLen = pHandle->stNeedParam.nGop / 2;
        }
        /* normalp和其他gop mode需要动态切换时，设置为1；如果仅有normalp，设置为0  */
        stChnAttr.stGopAttr.u32MaxLtrCount = 1;
        /* smartp和tsvc mode需要动态切换时，设置为1；如果仅有smartp，设置为0 */
        stChnAttr.stGopAttr.u32TsvcPreload = 0;
        stChnAttr.stGopAttr.enGopMode = pHandle->stNeedParam.enGopMode;
    }

    /* 码率控制器属性 */
    if (stChnAttr.stVencAttr.enType == RK_VIDEO_ID_AVC)
    {
        CHECK_API_RETURN(rockitVenc_h264_attr_fill(pHandle, &stChnAttr));
    }
    else if (stChnAttr.stVencAttr.enType == RK_VIDEO_ID_HEVC)
    {
        CHECK_API_RETURN(rockitVenc_h265_attr_fill(pHandle, &stChnAttr));
    }
    else if (stChnAttr.stVencAttr.enType == RK_VIDEO_ID_JPEG)
    {
        CHECK_API_RETURN(rockitVenc_jpeg_attr_fill(pHandle, &stChnAttr));
    }
    else if (stChnAttr.stVencAttr.enType == RK_VIDEO_ID_MJPEG)
    {
        CHECK_API_RETURN(rockitVenc_mjpeg_attr_fill(pHandle, &stChnAttr));
    }

    /* 设置编码输出为单包模式，I帧中组合SPS、PPS、SEI */
    if(!gs_bIsSetOneStreamBuf)
    {
        CHECK_API_RETURN(rockitVenc_set_one_stream_buf(pHandle, RK_VIDEO_ID_AVC, 1));
        CHECK_API_RETURN(rockitVenc_set_one_stream_buf(pHandle, RK_VIDEO_ID_HEVC, 1));
        CHECK_API_RETURN(rockitVenc_set_one_stream_buf(pHandle, RK_VIDEO_ID_MJPEG, 1)); // MJPEG和JPEG设置一个即可
        gs_bIsSetOneStreamBuf = RK_TRUE;
    }

    /* 创建编码通道 */
    CHECK_API_RETURN(RK_MPI_VENC_CreateChn(nChn, &stChnAttr));

    /*设置编码通道码率控制器的高级参数*/
    CHECK_API_RETURN(rockitVenc_set_rcParam(pHandle));
    
    /*设置H.264/H.265协议编码通道的类型属性*/
    CHECK_API_RETURN(rockitVenc_set_codec_type_attr(pHandle));
    
    /*设置编码通道码率控制器的高级参数*/
    CHECK_API_RETURN(rockitVenc_set_jpeg_config(pHandle));

    // note 不使用帧内刷新，会导致概率第一帧I帧未正确解码，后续又无I帧，导致无法播放
    /* 设置帧内刷新 */
    //CHECK_API_RETURN(rockitVenc_set_intraRefresh(pHandle));

    /*开启智能编码SVC*/
    if (pHandle->stExParam.bSvcEnable == RK_TRUE)
    {
        CHECK_API_RETURN(RK_MPI_VENC_EnableSvc(nChn, pHandle->stExParam.bSvcEnable));
    }

    /*ROI*/
    if (pHandle->stExParam.bRoiEnable == RK_TRUE && pHandle->stExParam.bSvcEnable == RK_FALSE) // 不能与SVC同时打开
    {
        for (int i = 0; i < VENC_MAX_ROI_NUM; i++)
        {
            if (pHandle->stExParam.astRoiAttr[i].stRect.u32Width == 0
                || pHandle->stExParam.astRoiAttr[i].stRect.u32Height == 0)
            {
                continue;
            }
            /*设置H.264/H.265通道的ROI属性*/
            CHECK_API_RETURN(rockitVenc_set_roiAttr(pHandle, &pHandle->stExParam.astRoiAttr[i]));
        }
    }

    /*设置slice分割*/
    //RK_MPI_VENC_SetSliceSplit(pHandle->stNeedParam.nChn, &pHandle->stExParam.stSliceSplit);

    /* 输出内存池使用用户模式 */
    if (pHandle->stExParam.bAttachPool)
    {
        MB_POOL_CONFIG_S stMbPoolCfg;
        memset(&stMbPoolCfg, 0, sizeof(MB_POOL_CONFIG_S));
        /* 创建输出内存池 */
        memset(&stMbPoolCfg, 0, sizeof(MB_POOL_CONFIG_S));
        stMbPoolCfg.u64MBSize = pHandle->stExParam.unBufferSize;
        stMbPoolCfg.u32MBCnt = pHandle->stExParam.unMbCnt;
        stMbPoolCfg.enAllocType = MB_ALLOC_TYPE_DMA;
        /* 是否在缓存池创建时申请好缓存块。这必须预先分配，以便进行连接、加密和输出操作 */
        stMbPoolCfg.bPreAlloc = RK_TRUE;
        pHandle->vencPoolOutput = RK_MPI_MB_CreatePool(&stMbPoolCfg);
        if (pHandle->vencPoolOutput == MB_INVALID_POOLID)
        {
            mpi_venc_log("编码通道[%d],创建输出内存缓存池失败!", nChn);
            return RK_FAILURE;
        }

        CHECK_API_RETURN(RK_MPI_VENC_AttachMbPool(nChn, pHandle->vencPoolOutput));
        mpi_venc_log("输出内存池创建成功");
    }

    /* 修改 JPEG编码 通道的帧率*/
    if (pHandle->stNeedParam.enCodec == RK_VIDEO_ID_JPEG || pHandle->stNeedParam.enCodec == RK_VIDEO_ID_MJPEG) 
    {
        VENC_CHN_PARAM_S stChnParamUpdate;
        CHECK_API_RETURN(RK_MPI_VENC_GetChnParam(nChn, &stChnParamUpdate));
        
        stChnParamUpdate.stFrameRate.bEnable = RK_TRUE;
        stChnParamUpdate.stFrameRate.s32SrcFrmRateNum = pHandle->stNeedParam.nInFrameRate;
        stChnParamUpdate.stFrameRate.s32SrcFrmRateDen = pHandle->stNeedParam.nOutFrameRate;
        stChnParamUpdate.stFrameRate.s32DstFrmRateNum = pHandle->stNeedParam.nOutFrameRate; 
        stChnParamUpdate.stFrameRate.s32DstFrmRateDen = pHandle->stNeedParam.nOutFrameRate;
        CHECK_API_RETURN(RK_MPI_VENC_SetChnParam(nChn, &stChnParamUpdate));
    }

    /*开始接受编码码流*/
    VENC_RECV_PIC_PARAM_S stRecvParam;
    stRecvParam.s32RecvPicNum = pHandle->stExParam.nSnapPicCount;
    CHECK_API_RETURN(RK_MPI_VENC_StartRecvFrame(nChn, &stRecvParam));

    mpi_venc_log("编码通道[%d]初始化完成",nChn);
    return RK_SUCCESS;
}

/**
 * @brief   : venc去初始化
 * @param    {RkVenc_S} *pHandle 句柄
 * @return   {int} 成功返回0,失败返回-1
 */
static int rockitVenc_unInit(RkVenc_S *pHandle)
{
    if (NULL == pHandle)
    {
        mpi_venc_log("句柄为空");
        return RK_FAILURE;
    }

    /* 停⽌编码通道接收输⼊图像 */
    CHECK_API_RETURN(RK_MPI_VENC_StopRecvFrame(pHandle->stNeedParam.nChn));

    /* 内存池使用用户模式 */
    if (pHandle->stExParam.bAttachPool)
    {
        /* 将编码通道从视频缓存 MB 池中解绑定 */
        CHECK_API_RETURN(RK_MPI_VENC_DetachMbPool(pHandle->stNeedParam.nChn));
    }

    /* 复位编码通道 */
    RK_MPI_VENC_ResetChn(pHandle->stNeedParam.nChn);
    /* 销毁编码通道 */
    RK_MPI_VENC_DestroyChn(pHandle->stNeedParam.nChn);

    if (pHandle->stExParam.bAttachPool)
    {
        /* 将编码通道从视频缓存 MB 池中解绑定 */
        CHECK_API_RETURN(RK_MPI_VENC_DetachMbPool(pHandle->stNeedParam.nChn));
        /* 销毁输出内存缓存池 */
        CHECK_API_RETURN(RK_MPI_MB_DestroyPool(pHandle->vencPoolOutput));
        /* 销毁输入内存缓存池 */
        CHECK_API_RETURN(RK_MPI_MB_DestroyPool(pHandle->vencPoolInput));
    }

    return RK_SUCCESS;
}

RkVenc_S *rockitVenc_alloc(RkVencNeedParam_S stParam)
{
    RkVenc_S *pHandle = (RkVenc_S *) malloc(sizeof(RkVenc_S));

    memset(pHandle, 0, sizeof(RkVenc_S));

    // info /**********************必需参数***************************/
    pHandle->stNeedParam = stParam;

    // info /**********************功能参数***************************/

    /*内存块数*/
    pHandle->stExParam.unMbCnt = 8;

    /*编码等级和码流设置*/
    if (RK_VIDEO_ID_AVC == stParam.enCodec)
    {
        pHandle->stExParam.nProfile = 77;
        pHandle->stExParam.enRcMode = VENC_RC_MODE_H264CBR;
    }
    else if (RK_VIDEO_ID_HEVC == stParam.enCodec)
    {
        pHandle->stExParam.nProfile = 0;
        pHandle->stExParam.enRcMode = VENC_RC_MODE_H265CBR;
    }
    else if (RK_VIDEO_ID_JPEG == stParam.enCodec || RK_VIDEO_ID_MJPEG == stParam.enCodec)
    {
        pHandle->stExParam.nProfile = 0;
        pHandle->stExParam.enRcMode = VENC_RC_MODE_MJPEGCBR;
    }

    /*智能编码使能*/
    pHandle->stExParam.bSvcEnable = RK_FALSE;
    /*图像质量*/
    pHandle->stExParam.nImageQuality = 1;
    /*码流平滑 [ 清晰<->平滑 ]*/
    pHandle->stExParam.nBitrateSmoothing = 50;
    /*编码帧数*/
    pHandle->stExParam.nSnapPicCount = -1;
    /* 码流大小 */
    pHandle->stExParam.nBitRate = 4096;
    pHandle->stExParam.nMaxBitRate = 4096;
    pHandle->stExParam.nMinBitRate = 3072;
    pHandle->stExParam.nAverageBitrate = 2048;
    // pHandle->stExParam.nBitRate = pHandle->stNeedParam.unWidth * pHandle->stNeedParam.unHeight / 64
    //                               * (pHandle->stNeedParam.nOutFrameRate / pHandle->stNeedParam.nInFrameRate);
    /* 高级码率控制 */
    pHandle->stExParam.nStepQp = 8;
    pHandle->stExParam.nMaxQp = 51;
    pHandle->stExParam.nMinQp = 10;
    pHandle->stExParam.nMaxIQp = 46;
    pHandle->stExParam.nMinIQp = 24;
    pHandle->stExParam.nDeltIpQp = 2;

    /* JPEG/MJPEG格式 品质因数 */
    pHandle->stExParam.u32Qfactor = 70;
    /* MJPEG格式 最大品质因数 */
    pHandle->stExParam.u32MaxQfactor = 100;
    /* MJPEG格式 最小品质因数 */
    pHandle->stExParam.u32MinQfactor = 50;
    /* SLICE分割 */
    pHandle->stExParam.stSliceSplit.bSplitEnable = RK_FALSE;
    pHandle->stExParam.stSliceSplit.u32SplitMode = 0;
    pHandle->stExParam.stSliceSplit.u32SplitSize = 400;
    /* 图像大小 */
    pHandle->stExParam.unBufferSize = 0;
    /* 输出缓冲区模式 */
    pHandle->stExParam.bAttachPool = RK_FALSE;
    // info /**********************函数列表***************************/
    pHandle->rockitVenc_init                    = rockitVenc_init;
    pHandle->rockitVenc_unInit                  = rockitVenc_unInit;
    pHandle->rockitVenc_send_frame              = rockitVenc_send_frame;
    pHandle->rockitVenc_send_VFrame             = rockitVenc_send_VFrame;
    pHandle->rockitVenc_get_stream              = rockitVenc_get_stream;
    pHandle->rockitVenc_get_streamVirdata       = rockitVenc_get_streamVirdata;
    pHandle->rockitVenc_release_stream          = rockitVenc_release_stream;
    pHandle->rockitVenc_get_gop                 = rockitVenc_get_gop;
    pHandle->rockitVenc_set_gop                 = rockitVenc_set_gop;
    pHandle->rockitVenc_set_roiAttr             = rockitVenc_set_roiAttr;
    pHandle->rockitVenc_set_corpOrScale         = rockitVenc_set_corpOrScale;
    pHandle->rockitVenc_request_idr             = rockitVenc_request_idr;

    return pHandle;
}

void rockitVenc_release(RkVenc_S* pHandle)
{
    if (!pHandle)
    {
        free(pHandle);
        pHandle = NULL;
    }
}
