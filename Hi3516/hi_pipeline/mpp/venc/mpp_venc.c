/**
 * @FilePath     : mpp_venc.c
 * @Author       : zhouzirui
 * @Date         : 2025-03-20 15:50:55
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-01-21 08:51:59
 * @Description  : 海思venc模块封装
 */

#include "mpp_venc.h"
#include "mpi_common.h"

/*帧buffer比例最大值*/
#define MPP_FRAME_BUF_RATIO_MAX 100
/*帧buffer比例最小值*/
#define MPP_FRAME_BUF_RATIO_MIN 70
/*帧buffer比例默认值*/
#define MPP_FRAME_BUF_RATIO_DEFAULT 75

/* 全局静态变量-是否设置了单包模式 */
static td_bool gs_bIsSetOneStreamBuf = TD_FALSE;

/**
 * @brief       : H.264编码通道码率控制器属性填充
 * @author      : zhouzirui
 * @param        {HiVenc_S} *pHandle：句柄
 * @param        {ot_venc_chn_attr} *pChnAttr：编码通道属性结构体指针
 * @return       {*}成功返回0,失败返回-1
 */
static int mppVenc_h264_attr_fill(HiVenc_S *pHandle, ot_venc_chn_attr *pChnAttr)
{
    pChnAttr->venc_attr.h264_attr.frame_buf_ratio = MPP_FRAME_BUF_RATIO_DEFAULT; //帧buffer比例
    pChnAttr->venc_attr.h264_attr.rcn_ref_share_buf_en = pHandle->stNeedParam.bWrapEnable; //是否使能帧节省模式

    td_u32 u32StatsTime = VENC_RT_STATS_TIME_DEFAULT;
    if(pChnAttr->gop_attr.gop_mode == OT_VENC_GOP_MODE_SMART_P)
    {
        u32StatsTime = pChnAttr->gop_attr.smart_p.bg_interval / pHandle->stNeedParam.nGop;
    }
    switch (pHandle->stExParam.enRcMode)
    {
    case OT_VENC_RC_MODE_H264_ABR:
        pChnAttr->rc_attr.h264_abr = (ot_venc_h264_abr){
            .gop = (td_u32)pHandle->stNeedParam.nGop,
            .stats_time = u32StatsTime,
            .src_frame_rate = (td_u32)pHandle->stNeedParam.nInFrameRate,
            .dst_frame_rate = (td_u32)pHandle->stNeedParam.nOutFrameRate,
            .bit_rate = (td_u32)pHandle->stExParam.nBitRate,
            .vbv_buf_delay = VENC_RT_VBV_BUF_DELAY};
        break;
    case OT_VENC_RC_MODE_H264_CBR:
        pChnAttr->rc_attr.h264_cbr = (ot_venc_h264_cbr){
            .gop = (td_u32)pHandle->stNeedParam.nGop,
            .stats_time = u32StatsTime,
            .src_frame_rate = (td_u32)pHandle->stNeedParam.nInFrameRate,
            .dst_frame_rate = (td_u32)pHandle->stNeedParam.nOutFrameRate,
            .bit_rate = (td_u32)pHandle->stExParam.nBitRate};
        break;
    case OT_VENC_RC_MODE_H264_VBR:
        pChnAttr->rc_attr.h264_vbr = (ot_venc_h264_vbr){
            .gop = (td_u32)pHandle->stNeedParam.nGop,
            .stats_time = u32StatsTime,
            .src_frame_rate = (td_u32)pHandle->stNeedParam.nInFrameRate,
            .dst_frame_rate = (td_u32)pHandle->stNeedParam.nOutFrameRate,
            .max_bit_rate = (td_u32)pHandle->stExParam.nMaxBitRate};
        break;
    case OT_VENC_RC_MODE_H264_AVBR:
        pChnAttr->rc_attr.h264_avbr = (ot_venc_h264_avbr){
            .gop = (td_u32)pHandle->stNeedParam.nGop,
            .stats_time = u32StatsTime,
            .src_frame_rate = (td_u32)pHandle->stNeedParam.nInFrameRate,
            .dst_frame_rate = (td_u32)pHandle->stNeedParam.nOutFrameRate,
            .max_bit_rate = (td_u32)pHandle->stExParam.nMaxBitRate};
        mpi_venc_log("FrameRate: %d,%d",pHandle->stNeedParam.nInFrameRate,pHandle->stNeedParam.nOutFrameRate);
        break;
    case OT_VENC_RC_MODE_H264_CVBR:
        pChnAttr->rc_attr.h264_cvbr = (ot_venc_h264_cvbr){
            .gop = (td_u32) pHandle->stNeedParam.nGop,
            .stats_time = u32StatsTime,
            .src_frame_rate = (td_u32) pHandle->stNeedParam.nInFrameRate,
            .dst_frame_rate = (td_u32) pHandle->stNeedParam.nOutFrameRate,
            .max_bit_rate = (td_u32) pHandle->stExParam.nBitRate,
            .short_term_stats_time = u32StatsTime,
            .long_term_stats_time = 1,
            .long_term_max_bit_rate = (td_u32) pHandle->stExParam.nMaxBitRate,
            .long_term_min_bit_rate = (td_u32) pHandle->stExParam.nMinBitRate};
        break;
    case OT_VENC_RC_MODE_H264_FIXQP:
        pChnAttr->rc_attr.h264_fixqp = (ot_venc_h264_fixqp){
            .gop = (td_u32)pHandle->stNeedParam.nGop,
            .src_frame_rate = (td_u32)pHandle->stNeedParam.nInFrameRate,
            .dst_frame_rate = (td_u32)pHandle->stNeedParam.nOutFrameRate,
            .i_qp = pHandle->stExParam.i_qp,
            .p_qp = pHandle->stExParam.p_qp,
            .b_qp = pHandle->stExParam.b_qp};
        break;
    default:
        mpi_venc_log("Unsupported H264 rc_mode: %d", pHandle->stExParam.enRcMode);
        return TD_FAILURE;
    }
    return TD_SUCCESS;
}

/**
 * @brief       : H.265编码通道码率控制器属性填充
 * @author      : zhouzirui
 * @param        {HiVenc_S} *pHandle：句柄
 * @param        {ot_venc_chn_attr} *pChnAttr：编码通道属性结构体指针
 * @return       {*}成功返回0,失败返回-1
 */
static int mppVenc_h265_attr_fill(HiVenc_S *pHandle, ot_venc_chn_attr *pChnAttr)
{
    pChnAttr->venc_attr.h265_attr.frame_buf_ratio = MPP_FRAME_BUF_RATIO_DEFAULT; //帧buffer比例
    pChnAttr->venc_attr.h265_attr.rcn_ref_share_buf_en = pHandle->stNeedParam.bWrapEnable; //是否使能帧节省模式

    td_u32 u32StatsTime = VENC_RT_STATS_TIME_DEFAULT;
    if(pChnAttr->gop_attr.gop_mode == OT_VENC_GOP_MODE_SMART_P)
    {
        u32StatsTime = pChnAttr->gop_attr.smart_p.bg_interval / pHandle->stNeedParam.nGop;
    }
    switch (pHandle->stExParam.enRcMode)
    {
    case OT_VENC_RC_MODE_H265_ABR:
        pChnAttr->rc_attr.h265_abr = (ot_venc_h265_abr){
            .gop = (td_u32)pHandle->stNeedParam.nGop,
            .stats_time = u32StatsTime,
            .src_frame_rate = (td_u32)pHandle->stNeedParam.nInFrameRate,
            .dst_frame_rate = (td_u32)pHandle->stNeedParam.nOutFrameRate,
            .bit_rate = (td_u32)pHandle->stExParam.nBitRate,
            .vbv_buf_delay = VENC_RT_VBV_BUF_DELAY};
        break;
    case OT_VENC_RC_MODE_H265_CBR:
        pChnAttr->rc_attr.h265_cbr = (ot_venc_h265_cbr){
            .gop = (td_u32)pHandle->stNeedParam.nGop,
            .stats_time = u32StatsTime,
            .src_frame_rate = (td_u32)pHandle->stNeedParam.nInFrameRate,
            .dst_frame_rate = (td_u32)pHandle->stNeedParam.nOutFrameRate,
            .bit_rate = (td_u32)pHandle->stExParam.nBitRate};
        break;
    case OT_VENC_RC_MODE_H265_VBR:
        pChnAttr->rc_attr.h265_vbr = (ot_venc_h265_vbr){
            .gop = (td_u32)pHandle->stNeedParam.nGop,
            .stats_time = u32StatsTime,
            .src_frame_rate = (td_u32)pHandle->stNeedParam.nInFrameRate,
            .dst_frame_rate = (td_u32)pHandle->stNeedParam.nOutFrameRate,
            .max_bit_rate = (td_u32)pHandle->stExParam.nMaxBitRate};
        break;
    case OT_VENC_RC_MODE_H265_AVBR:
        pChnAttr->rc_attr.h265_avbr = (ot_venc_h265_avbr){
            .gop = (td_u32)pHandle->stNeedParam.nGop,
            .stats_time = u32StatsTime,
            .src_frame_rate = (td_u32)pHandle->stNeedParam.nInFrameRate,
            .dst_frame_rate = (td_u32)pHandle->stNeedParam.nOutFrameRate,
            .max_bit_rate = (td_u32)pHandle->stExParam.nMaxBitRate};
        break;
    case OT_VENC_RC_MODE_H265_CVBR:
        pChnAttr->rc_attr.h265_cvbr = (ot_venc_h265_cvbr){
            .gop = (td_u32) pHandle->stNeedParam.nGop,
            .stats_time = u32StatsTime,
            .src_frame_rate = (td_u32) pHandle->stNeedParam.nInFrameRate,
            .dst_frame_rate = (td_u32) pHandle->stNeedParam.nOutFrameRate,
            .max_bit_rate = (td_u32) pHandle->stExParam.nBitRate,
            .short_term_stats_time = u32StatsTime,
            .long_term_stats_time = 1,
            .long_term_max_bit_rate = (td_u32) pHandle->stExParam.nMaxBitRate,
            .long_term_min_bit_rate = (td_u32) pHandle->stExParam.nMinBitRate};
        break;
    case OT_VENC_RC_MODE_H265_FIXQP:
        pChnAttr->rc_attr.h265_fixqp = (ot_venc_h265_fixqp){
            .gop = (td_u32)pHandle->stNeedParam.nGop,
            .src_frame_rate = (td_u32)pHandle->stNeedParam.nInFrameRate,
            .dst_frame_rate = (td_u32)pHandle->stNeedParam.nOutFrameRate,
            .i_qp = pHandle->stExParam.i_qp,
            .p_qp = pHandle->stExParam.p_qp,
            .b_qp = pHandle->stExParam.b_qp};
        break;
    default:
        mpi_venc_log("Unsupported H265 rc_mode: %d", pHandle->stExParam.enRcMode);
        return TD_FAILURE;
    }
    return TD_SUCCESS;
}

/**
 * @brief       : SVAC3编码通道码率控制器属性填充
 * @author      : zhouzirui
 * @param        {HiVenc_S} *pHandle：句柄
 * @param        {ot_venc_chn_attr} *pChnAttr：编码通道属性结构体指针
 * @return       {*}成功返回0,失败返回-1
 */
static int mppVenc_svac3_attr_fill(HiVenc_S *pHandle, ot_venc_chn_attr *pChnAttr)
{
    pChnAttr->venc_attr.svac3_attr.frame_buf_ratio = MPP_FRAME_BUF_RATIO_DEFAULT; //帧buffer比例
    pChnAttr->venc_attr.svac3_attr.rcn_ref_share_buf_en = pHandle->stNeedParam.bWrapEnable; //是否使能帧节省模式

    td_u32 u32StatsTime = VENC_RT_STATS_TIME_DEFAULT;
    if(pChnAttr->gop_attr.gop_mode == OT_VENC_GOP_MODE_SMART_P)
    {
        u32StatsTime = pChnAttr->gop_attr.smart_p.bg_interval / pHandle->stNeedParam.nGop;
    }
    switch (pHandle->stExParam.enRcMode)
    {
    case OT_VENC_RC_MODE_SVAC3_ABR:
        pChnAttr->rc_attr.svac3_abr = (ot_venc_svac3_abr){
            .gop =  (td_u32)pHandle->stNeedParam.nGop,
            .stats_time = u32StatsTime,
            .src_frame_rate = (td_u32)pHandle->stNeedParam.nInFrameRate,
            .dst_frame_rate = (td_u32)pHandle->stNeedParam.nOutFrameRate,
            .bit_rate = (td_u32)pHandle->stExParam.nBitRate,
            .vbv_buf_delay = VENC_RT_VBV_BUF_DELAY};
        break;
    case OT_VENC_RC_MODE_SVAC3_CBR:
        pChnAttr->rc_attr.svac3_cbr = (ot_venc_svac3_cbr){
            .gop = (td_u32)pHandle->stNeedParam.nGop,
            .stats_time = u32StatsTime,
            .src_frame_rate = (td_u32)pHandle->stNeedParam.nInFrameRate,
            .dst_frame_rate = (td_u32)pHandle->stNeedParam.nOutFrameRate,
            .bit_rate = (td_u32)pHandle->stExParam.nBitRate};
        break;
    case OT_VENC_RC_MODE_SVAC3_VBR:
        pChnAttr->rc_attr.svac3_vbr = (ot_venc_svac3_vbr){
            .gop = (td_u32)pHandle->stNeedParam.nGop,
            .stats_time = u32StatsTime,
            .src_frame_rate = (td_u32)pHandle->stNeedParam.nInFrameRate,
            .dst_frame_rate = (td_u32)pHandle->stNeedParam.nOutFrameRate,
            .max_bit_rate = (td_u32)pHandle->stExParam.nMaxBitRate};
        break;
    case OT_VENC_RC_MODE_SVAC3_AVBR:
        pChnAttr->rc_attr.svac3_avbr = (ot_venc_svac3_avbr){
            .gop = (td_u32)pHandle->stNeedParam.nGop,
            .stats_time = u32StatsTime,
            .src_frame_rate = (td_u32)pHandle->stNeedParam.nInFrameRate,
            .dst_frame_rate = (td_u32)pHandle->stNeedParam.nOutFrameRate,
            .max_bit_rate = (td_u32)pHandle->stExParam.nMaxBitRate};
        break;
    case OT_VENC_RC_MODE_SVAC3_CVBR:
        pChnAttr->rc_attr.svac3_cvbr = (ot_venc_svac3_cvbr){
            .gop = (td_u32) pHandle->stNeedParam.nGop,
            .stats_time = u32StatsTime,
            .src_frame_rate = (td_u32) pHandle->stNeedParam.nInFrameRate,
            .dst_frame_rate = (td_u32) pHandle->stNeedParam.nOutFrameRate,
            .max_bit_rate = (td_u32) pHandle->stExParam.nBitRate,
            .short_term_stats_time = u32StatsTime,
            .long_term_stats_time = 1,
            .long_term_max_bit_rate = (td_u32) pHandle->stExParam.nMaxBitRate,
            .long_term_min_bit_rate = (td_u32) pHandle->stExParam.nMinBitRate};
        break;
    case OT_VENC_RC_MODE_SVAC3_FIXQP:
        pChnAttr->rc_attr.svac3_fixqp = (ot_venc_svac3_fixqp){
            .gop = (td_u32)pHandle->stNeedParam.nGop,
            .src_frame_rate = (td_u32)pHandle->stNeedParam.nInFrameRate,
            .dst_frame_rate = (td_u32)pHandle->stNeedParam.nOutFrameRate,
            .i_qp = pHandle->stExParam.i_qp,
            .p_qp = pHandle->stExParam.p_qp,
            .b_qp = pHandle->stExParam.b_qp};
        break;
    default:
        mpi_venc_log("Unsupported SVAC3 rc_mode: %d", pHandle->stExParam.enRcMode);
        return TD_FAILURE;
    }
    return TD_SUCCESS;
}

/**
 * @brief       : jpeg 编码通道码率控制器属性填充
 * @param        {HiVenc_S} *pHandle：句柄
 * @param        {ot_venc_chn_attr} *pChnAttr：编码通道属性结构体指针
 * @return       {int}成功返回0,失败返回-1
 */
static int mppVenc_jpeg_attr_fill(HiVenc_S *pHandle, ot_venc_chn_attr *pChnAttr)
{
    /* 是否使能DCF（Design rule for Camera File system） DCF信息包含拍照基本信息和缩略图。 */
    pChnAttr->venc_attr.jpeg_attr.dcf_en = TD_FALSE;
    /* 编码mpf缩略图的个数 Hi3516CV610 不支持MPF*/
    pChnAttr->venc_attr.jpeg_attr.mpf_cfg.large_thumbnail_num = 0;
    /* 当前编码通道只允许从一个源接收图 */
    pChnAttr->venc_attr.jpeg_attr.recv_mode = OT_VENC_PIC_RECV_SINGLE;

    return TD_SUCCESS;
}

/**
 * @brief       : mjpeg 编码通道码率控制器属性填充
 * @param        {HiVenc_S} *pHandle：句柄
 * @param        {ot_venc_chn_attr} *pChnAttr：编码通道属性结构体指针
 * @return       {int}成功返回0,失败返回-1
 */
static int mppVenc_mjpeg_attr_fill(HiVenc_S *pHandle, ot_venc_chn_attr *pChnAttr)
{
    switch (pHandle->stExParam.enRcMode)
    {
    case OT_VENC_RC_MODE_MJPEG_CBR:
        pChnAttr->rc_attr.mjpeg_cbr = (ot_venc_mjpeg_cbr){
            .stats_time = VENC_RT_STATS_TIME_DEFAULT,
            .src_frame_rate = (td_u32)pHandle->stNeedParam.nInFrameRate,
            .dst_frame_rate = (td_u32)pHandle->stNeedParam.nOutFrameRate,
            .bit_rate = (td_u32)pHandle->stExParam.nBitRate};
        break;
    case OT_VENC_RC_MODE_MJPEG_VBR:
        pChnAttr->rc_attr.mjpeg_vbr = (ot_venc_mjpeg_vbr){
            .stats_time = VENC_RT_STATS_TIME_DEFAULT,
            .src_frame_rate = (td_u32)pHandle->stNeedParam.nInFrameRate,
            .dst_frame_rate = (td_u32)pHandle->stNeedParam.nOutFrameRate,
            .max_bit_rate = (td_u32)pHandle->stExParam.nMaxBitRate};
        break;
    default:
        mpi_venc_log("Unsupported MJPEG rc_mode: %d", pHandle->stExParam.enRcMode);
        return TD_FAILURE;
    }
    return TD_SUCCESS;
}

/**
 * @brief       : 编码器GOP属性填充
 * @author      : zhouzirui
 * @param        {HiVenc_S} *pHandle：句柄
 * @param        {ot_venc_gop_attr} *pGopAttr：编码器GOP属性结构体指针
 * @return       {*}成功返回0,失败返回-1
 */
static int mppVenc_gop_attr_fill(HiVenc_S *pHandle, ot_venc_gop_attr *pGopAttr)
{
    /*根据码流平滑值设置qp差值*/
    int smoothing = pHandle->stExParam.nBitrateSmoothing;
    if (smoothing < 1) smoothing = 1;
    if (smoothing > 100) smoothing = 100;

    float smoothFactor = smoothing / 100.0f;
    int ip_qp_delta = (int)(2 + smoothFactor * 8); // 基础值2，最大到10
    // int sp_qp_delta = (int)(2 + smoothFactor * 6); // SP帧可设置得更小些
    int bg_qp_delta = (int)(4 + smoothFactor * 6);
    int vi_qp_delta = (int)(2 + smoothFactor * 5);

    switch (pHandle->stNeedParam.enGopMode)
    {
    case OT_VENC_GOP_MODE_NORMAL_P:
        pGopAttr->normal_p.ip_qp_delta = ip_qp_delta; //I帧相对P帧的QP差值。取值范围：[-10, 30]。默认为：2
        break;
    case OT_VENC_GOP_MODE_DUAL_P:
        pGopAttr->dual_p.sp_interval = 3; //Special P帧的间隔。取值范围：[0, 1)∪(1, gop-1]，gop是I帧间隔。
        pGopAttr->dual_p.sp_qp_delta = 2; //Special P帧相对普通P帧的QP差值。取值范围：[-10, 30]
        pGopAttr->dual_p.ip_qp_delta = 4; //I帧相对普通P帧的QP差值。取值范围：[-10, 30]
        break;
    case OT_VENC_GOP_MODE_SMART_P:
        pGopAttr->smart_p.bg_interval = pHandle->stNeedParam.nGop * 6; //长期参考帧的间隔。取值范围：[gop, 65536]，且必须是gop的整数倍。
        pGopAttr->smart_p.bg_qp_delta = bg_qp_delta; //长期参考帧和P帧的QP差值。取值范围：[-10, 30]。备注：在调试信息中，该参数等同其他码控模式的ip_qp_delta。默认为：4
        pGopAttr->smart_p.vi_qp_delta = vi_qp_delta; //虚拟I帧相对于普通P帧的QP差值。取值范围：[-10, 30]。默认为：2
        break;
    case OT_VENC_GOP_MODE_SMART_CRR:
        pGopAttr->smart_crr.bg_interval = 300; //长期参考帧的间隔。取值范围：[gop, 65536]，且必须是gop的整数倍。
        pGopAttr->smart_crr.bg_qp_delta = 10; //长期参考帧和P帧的QP差值。取值范围：[-10, 30]备注：在调试信息中，该参数等同其他码控模式的ip_qp_delta。
        pGopAttr->smart_crr.vi_qp_delta = 3; //虚拟I帧相对于普通P帧的QP差值。取值范围：[-10, 30]
        pGopAttr->smart_crr.crr_split_num = 4; //非显示知识图像切片个数。取值范围：[1, gop）
        pGopAttr->smart_crr.crr_delay_num = 0; //CRR切片传输延迟个数。（crr_split_num-1）*（crr_delay_num+1）+ 2 小于GOP大小
        pGopAttr->smart_crr.strategy = OT_VENC_CRR_RECODE_DISABLE; //CRR帧对应位置显示帧编码策略。
        break;
    default:
        mpi_venc_log("Unsupported gop mode: %d", pHandle->stNeedParam.enGopMode);
        return TD_FAILURE;
    }

    return TD_SUCCESS;
}

/**
 * @brief   : 编码通道码率控制器的高级参数填充
 * @param    {HiVenc_S} *pHandle：句柄
 * @param    {ot_venc_rc_param} *pRcParam：编码通道码率控制器的高级参数
 * @return   {*}成功返回0,失败返回-1
 */
static int mppVenc_rc_param_fill(HiVenc_S *pHandle, ot_venc_rc_param *pRcParam)
{
    if (!pHandle || !pRcParam)
    {
        return TD_FAILURE;
    }

    /* 关闭重编功能，以优化通路延时，如影响编码效果，则进行回退 */
    if(pHandle->stExParam.enRcMode == OT_VENC_RC_MODE_H264_CVBR)
    {
        pRcParam->h264_cvbr_param.max_reencode_times = 0;
    }
    else if(pHandle->stExParam.enRcMode == OT_VENC_RC_MODE_H264_AVBR)
    {
        pRcParam->h264_avbr_param.max_reencode_times = 0;
    }
    else if(pHandle->stExParam.enRcMode == OT_VENC_RC_MODE_H265_CVBR)
    {
        pRcParam->h265_cvbr_param.max_reencode_times = 0;
    }
    else if(pHandle->stExParam.enRcMode == OT_VENC_RC_MODE_H265_AVBR)
    {
        pRcParam->h265_avbr_param.max_reencode_times = 0;
    }

    /*根据码率大小设置 row_qp_delta（0~5）控制宏块行QP变化范围*/
    int row_qp_delta  = 0;
    int bitrate = pHandle->stExParam.nBitRate;
    if (bitrate <= 512)
        row_qp_delta = 5;
    else if (bitrate <= 1024)
        row_qp_delta = 4;
    else if (bitrate <= 2048)
        row_qp_delta = 3;
    else if (bitrate <= 4096)
        row_qp_delta = 2;
    else if (bitrate <= 8192)
        row_qp_delta = 1;
    else
        row_qp_delta = 0;

    // mpi_venc_log("row_qp_delta:%d->%d", pRcParam->row_qp_delta, row_qp_delta);
    /*在宏块级码率控制时，每一行宏块的起始Qp相对于帧起始Qp的波动幅度值。取值范围：[0, 10]*/
    pRcParam->row_qp_delta = row_qp_delta;

    /*简单的threshold设置，越低表示QP变化更激进（更清晰但不平滑）*/
    // int smoothing = pHandle->stExParam.nBitrateSmoothing; // 1~100，越大越平滑
    // uint8_t base_threshold[OT_VENC_TEXTURE_THRESHOLD_SIZE] = {0, 0, 0, 0, 3, 3, 5, 5, 8, 8, 8, 15, 15, 20, 25, 25};
    // int scale = (100 - smoothing) / 10; // 越模糊，越平滑，阈值越大

    /*宏块级码率控制的mad门限*/
    // for (int i = 0; i < OT_VENC_TEXTURE_THRESHOLD_SIZE; ++i)
    // {
    //     int val = base_threshold[i] + scale;
    //     if (val > 255)
    //         val = 255;
    //     pRcParam->threshold_i[i] = val;
    //     pRcParam->threshold_p[i] = val;
    //     pRcParam->threshold_b[i] = val;
    // }

    // note MJPEG 格式不进行判断
    /*若为变码率模式，根据图像质量设置对应的 QP 范围限制*/
    ot_venc_rc_mode rcMode = pHandle->stExParam.enRcMode;
    td_bool is_vbr_mode = (rcMode == OT_VENC_RC_MODE_H264_AVBR || rcMode == OT_VENC_RC_MODE_H264_VBR ||
                           rcMode == OT_VENC_RC_MODE_H265_AVBR || rcMode == OT_VENC_RC_MODE_H265_VBR ||
                           rcMode == OT_VENC_RC_MODE_SVAC3_AVBR || rcMode == OT_VENC_RC_MODE_SVAC3_VBR ||
                           rcMode == OT_VENC_RC_MODE_MJPEG_VBR)
                              ? TD_TRUE
                              : TD_FALSE;

    /*是否为变码率*/
    if (is_vbr_mode) 
    {
        int min_qp, max_qp;
        int chg_pos, min_qfactor, max_qfactor;

        /*图像质量对QP范围的映射*/
        switch (pHandle->stExParam.nImageQuality)
        {
            case 1:  min_qp = 38; max_qp = 51; chg_pos = 50; min_qfactor = 90; max_qfactor = 100; break; // 最低质量
            case 20: min_qp = 34; max_qp = 48; chg_pos = 60; min_qfactor = 80; max_qfactor = 98; break;
            case 40: min_qp = 30; max_qp = 44; chg_pos = 70; min_qfactor = 70; max_qfactor = 90; break;
            case 60: min_qp = 26; max_qp = 40; chg_pos = 80; min_qfactor = 55; max_qfactor = 80; break; // 中质量
            case 80: min_qp = 22; max_qp = 36; chg_pos = 88; min_qfactor = 40; max_qfactor = 65; break;
            case 100:min_qp = 18; max_qp = 32; chg_pos = 95; min_qfactor = 25; max_qfactor = 50; break; // 最高质量
            default: min_qp = 26; max_qp = 40; chg_pos = 80; min_qfactor = 55; max_qfactor = 80; break;
        }
        switch (rcMode)
        {
        case OT_VENC_RC_MODE_H264_VBR:
            pRcParam->h264_vbr_param.min_qp = min_qp;
            pRcParam->h264_vbr_param.max_qp = max_qp;
            pRcParam->h264_vbr_param.min_i_qp = min_qp;
            pRcParam->h264_vbr_param.max_i_qp = max_qp;
            pRcParam->h264_vbr_param.min_vi_qp = min_qp;
            pRcParam->h264_vbr_param.max_vi_qp = max_qp;
            mpi_venc_log("max_reencode_times:%d [%d,%d]", pRcParam->h264_vbr_param.max_reencode_times, min_qp, max_qp);
            break;
        case OT_VENC_RC_MODE_H264_AVBR:
            pRcParam->h264_avbr_param.min_qp = min_qp;
            pRcParam->h264_avbr_param.max_qp = max_qp;
            pRcParam->h264_avbr_param.min_i_qp = min_qp;
            pRcParam->h264_avbr_param.max_i_qp = max_qp;
            pRcParam->h264_avbr_param.min_vi_qp = min_qp;
            pRcParam->h264_avbr_param.max_vi_qp = max_qp;
            mpi_venc_log("max_reencode_times:%d [%d,%d]", pRcParam->h264_avbr_param.max_reencode_times, min_qp, max_qp);
            break;
        case OT_VENC_RC_MODE_H265_VBR:
            pRcParam->h265_vbr_param.min_qp = min_qp;
            pRcParam->h265_vbr_param.max_qp = max_qp;
            pRcParam->h265_vbr_param.min_i_qp = min_qp;
            pRcParam->h265_vbr_param.max_i_qp = max_qp;
            pRcParam->h265_vbr_param.min_vi_qp = min_qp;
            pRcParam->h265_vbr_param.max_vi_qp = max_qp;
            mpi_venc_log("max_reencode_times:%d [%d,%d]", pRcParam->h265_vbr_param.max_reencode_times, min_qp, max_qp);
            break;
        case OT_VENC_RC_MODE_H265_AVBR:
            pRcParam->h265_avbr_param.min_qp = min_qp;
            pRcParam->h265_avbr_param.max_qp = max_qp;
            pRcParam->h265_avbr_param.min_i_qp = min_qp;
            pRcParam->h265_avbr_param.max_i_qp = max_qp;
            pRcParam->h265_avbr_param.min_vi_qp = min_qp;
            pRcParam->h265_avbr_param.max_vi_qp = max_qp;
            mpi_venc_log("max_reencode_times:%d [%d,%d]", pRcParam->h265_avbr_param.max_reencode_times, min_qp, max_qp);
            break;
        case OT_VENC_RC_MODE_SVAC3_VBR:
            pRcParam->svac3_vbr_param.min_qp = min_qp;
            pRcParam->svac3_vbr_param.max_qp = max_qp;
            pRcParam->svac3_vbr_param.min_i_qp = min_qp;
            pRcParam->svac3_vbr_param.max_i_qp = max_qp;
            pRcParam->svac3_vbr_param.min_vi_qp = min_qp;
            pRcParam->svac3_vbr_param.max_vi_qp = max_qp;
            mpi_venc_log("max_reencode_times:%d [%d,%d]", pRcParam->svac3_vbr_param.max_reencode_times, min_qp, max_qp);
            break;
        case OT_VENC_RC_MODE_SVAC3_AVBR:
            pRcParam->svac3_avbr_param.min_qp = min_qp;
            pRcParam->svac3_avbr_param.max_qp = max_qp;
            pRcParam->svac3_avbr_param.min_i_qp = min_qp;
            pRcParam->svac3_avbr_param.max_i_qp = max_qp;
            pRcParam->svac3_avbr_param.min_vi_qp = min_qp;
            pRcParam->svac3_avbr_param.max_vi_qp = max_qp;
            mpi_venc_log("max_reencode_times:%d [%d,%d]", pRcParam->svac3_avbr_param.max_reencode_times, min_qp, max_qp);
            break;
        case OT_VENC_RC_MODE_MJPEG_VBR:
            pRcParam->mjpeg_vbr_param.chg_pos = chg_pos;
            pRcParam->mjpeg_vbr_param.min_qfactor = min_qfactor;
            pRcParam->mjpeg_vbr_param.max_qfactor = max_qfactor;
            break;
        default:
            mpi_venc_log("图像质量映射qp 设置失败");
            break;
        }
    }

    return TD_SUCCESS;
}

/**
 * @brief       : 设置编码通道配置
 * @author      : zhouzirui
 * @param        {HiVenc_S} *pHandle：句柄
 * @return       {*}成功返回0,失败返回-1
 */
static int mppVenc_set_chn_config(HiVenc_S *pHandle)
{
    if (NULL == pHandle)
    {
        return TD_FAILURE;
    }
    ot_venc_chn_config stChnConfig;
    /*获取编码通道配置*/
    CHECK_API_RETURN(ss_mpi_venc_get_chn_config(pHandle->stNeedParam.nChn, &stChnConfig));
    stChnConfig.svc_version = OT_VENC_SVC_V2;
    /*设置编码通道配置*/
    CHECK_API_RETURN(ss_mpi_venc_set_chn_config(pHandle->stNeedParam.nChn, &stChnConfig));

    return TD_SUCCESS;
}

/**
 * @brief       : 设置SVC智能编码相关参数：句柄
 * @author      : zhouzirui
 * @param        {HiVenc_S} *pHandle：句柄
 * @return       {*}成功返回0,失败返回-1
 */
static int mppVenc_set_svc_param(HiVenc_S *pHandle)
{
    if (NULL == pHandle)
    {
        return TD_FAILURE;
    }

    // td_u32 qp_i[SAMPLE_VENC_NUM] = {2, 62, 94, 1, 0};
    // td_u32 qp_p[SAMPLE_VENC_NUM] = {4, 58, 94, 2, 0};
    ot_venc_svc_param_ex stSvcParamEx;
    /*获取智能编码相关参数*/
    CHECK_API_RETURN(ss_mpi_venc_get_svc_param_ex(pHandle->stNeedParam.nChn, &stSvcParamEx));
    if(stSvcParamEx.svc_version == OT_VENC_SVC_V1) //智能编码1.0
    {
        //qpmap_value_i: I帧的QpMap值。取值范围：[0,255]。默认值为0。
        //qpmap_value_p: P帧的QpMap值。取值范围：[0,255]。默认值为0。
        //skipmap_value: P帧前景SkipMap值。使用更强的Skip倾向性可以降低码率，但会牺牲质量取值范围：[0,255]。默认值为0。
        /*对面积较大的前景物体少量升高QP，避免质量过好，以减少对码率的消耗。取值范围：[0,1]。默认值为1。*/
        stSvcParamEx.svc_param_v1.fg_protect_adaptive_en = TD_TRUE;
        /*可能有目标区域的QpMap和SkipMap值*/
        stSvcParamEx.svc_param_v1.activity_region.qpmap_value_i = 0;
        stSvcParamEx.svc_param_v1.activity_region.qpmap_value_p = 0;
        stSvcParamEx.svc_param_v1.activity_region.skipmap_value = 0;
        /*背景区域的QpMap和SkipMap值*/
        stSvcParamEx.svc_param_v1.bg_region.qpmap_value_i = 2;
        stSvcParamEx.svc_param_v1.bg_region.qpmap_value_p = 6;
        stSvcParamEx.svc_param_v1.bg_region.skipmap_value = 0;
        for (td_s32 j = 0; j < SVC_RECT_TYPE_BUTT; j++)
        {
            /*前景区域的QpMap和SkipMap值*/
            stSvcParamEx.svc_param_v1.fg_region[j].qpmap_value_i = 0; //qp_i[j]
            stSvcParamEx.svc_param_v1.fg_region[j].qpmap_value_p = 0; //qp_p[j]
            stSvcParamEx.svc_param_v1.fg_region[j].skipmap_value = 0;
        }
    }else if(stSvcParamEx.svc_version == OT_VENC_SVC_V2) //智能编码2.0
    {
        stSvcParamEx.svc_param_v2.max_ref_num = 3; //长期参考个数，范围[2,4]。
        stSvcParamEx.svc_param_v2.refresh_interval = 11; //参考刷新间隔，范围[1,21]。
        for (td_s32 j = 0; j < SVC_RECT_TYPE_BUTT; j++)
        {
            stSvcParamEx.svc_param_v2.qp_delta[j] = 0; // 前景和背景qp调节值，范围[-16，15]。
        }
        stSvcParamEx.svc_param_v2.roi_type = OT_VENC_SVC_ROI_TYPE_RECT; //内部接收检测目标方式。接收检测目标框/接收检测目标mask
    }
    /*设置智能编码相关参数*/
    CHECK_API_RETURN(ss_mpi_venc_set_svc_param_ex(pHandle->stNeedParam.nChn, &stSvcParamEx));

    return TD_SUCCESS;
}

/**
 * @brief       : 设置编码jpeg相关配置
 * @param        {HiVenc_S} *pHandle：句柄
 * @return       {int}成功返回0,失败返回-1
 */
static int mppVenc_set_jpeg_config(HiVenc_S *pHandle)
{
    if (NULL == pHandle)
    {
        return TD_FAILURE;
    }

    if(pHandle->stNeedParam.enCodec == OT_PT_JPEG)
    {
        ot_venc_jpeg_param stJpegParam;
        // ot_venc_jpeg_enc_mode enEncMode;
    
        /*获取JPEG协议编码通道的高级参数*/
        CHECK_API_RETURN(ss_mpi_venc_get_jpeg_param(pHandle->stNeedParam.nChn, &stJpegParam));
        /* 品质因数 */
        stJpegParam.qfactor = pHandle->stExParam.uQFactor;
        /*设置JPEG协议编码通道的高级参数*/
        CHECK_API_RETURN(ss_mpi_venc_set_jpeg_param(pHandle->stNeedParam.nChn, &stJpegParam));
    
        /* Hi3516CV610 不支持抓拍 */
        /* 获取JPEG编码通道的抓拍模式 */
        // CHECK_API_RETURN(ss_mpi_venc_get_jpeg_enc_mode(pHandle->stNeedParam.nChn, &enEncMode));
        /* 设置JPEG抓拍通道的抓拍模式 */
        // CHECK_API_RETURN(ss_mpi_venc_set_jpeg_enc_mode(pHandle->stNeedParam.nChn, enEncMode));
    }
    return TD_SUCCESS;
}

/**
 * @brief   : 设置编码mjpeg相关配置
 * @param    {HiVenc_S} *pHandle 句柄
 * @return   {int} 成功返回0,失败返回-1
 */
static int mppVenc_set_mjpeg_config(HiVenc_S *pHandle)
{
    if (NULL == pHandle)
    {
        return TD_FAILURE;
    }

    if (pHandle->stNeedParam.enCodec == OT_PT_MJPEG)
    {
        ot_venc_mjpeg_param stMjpegParam;

        /*获取MJPEG协议编码通道的高级参数*/
        CHECK_API_RETURN(ss_mpi_venc_get_mjpeg_param(pHandle->stNeedParam.nChn, &stMjpegParam));

        /*设置MJPEG协议编码通道的高级参数*/
        CHECK_API_RETURN(ss_mpi_venc_set_mjpeg_param(pHandle->stNeedParam.nChn, &stMjpegParam));
    }
    return TD_SUCCESS;
}


/**
 * @brief   : 根据编码格式设置该编码为单包还是多包模式
 * @param    {HiVenc_S} *pHandle
 * @param    {ot_payload_type} enType 编码格式
 * @param    {td_u32} u32OneStreamBuf 编码模式
 * @return   {int} 成功返回0,失败返回-1
 */
static int mppVenc_set_one_stream_buf(HiVenc_S *pHandle, ot_payload_type enType, td_u32 u32OneStreamBuf)
{
    /* 设置编码输出为单包模式，I帧中组合SPS、PPS、SEI等 */
    /* 设置编码输出为多包模式，I帧中不组合SPS、PPS、SEI等 */
    ot_venc_mod_param stModParam;
    switch (enType)
    {
    case OT_PT_H264:
        stModParam.mod_type = OT_VENC_MOD_H264;
        break;
    case OT_PT_H265:
        stModParam.mod_type = OT_VENC_MOD_H265;
        break;
    case OT_PT_SVAC3:
        stModParam.mod_type = OT_VENC_MOD_SVAC3;
        break;
    case OT_PT_MJPEG:
    case OT_PT_JPEG:
        stModParam.mod_type = OT_VENC_MOD_JPEG;
        break;
    default:
        return TD_FAILURE;
    }
    /* 获取编码相关的模块参数 */
    CHECK_API_RETURN(ss_mpi_venc_get_mod_param(&stModParam));
    switch (enType)
    {
    case OT_PT_H264:
        stModParam.h264_mod_param.one_stream_buf = u32OneStreamBuf;
        break;
    case OT_PT_H265:
        stModParam.h265_mod_param.one_stream_buf = u32OneStreamBuf;
        break;
    case OT_PT_SVAC3:
        stModParam.svac3_mod_param.one_stream_buf = u32OneStreamBuf;
        break;
    case OT_PT_MJPEG:
    case OT_PT_JPEG:
        stModParam.jpeg_mod_param.one_stream_buf = u32OneStreamBuf;
        break;
    default:
        return TD_FAILURE;
    }
    /* 设置编码相关的模块参数 */
    CHECK_API_RETURN(ss_mpi_venc_set_mod_param(&stModParam));

    return TD_SUCCESS;
}
 
/**
 * @brief       : 请求IDR帧
 * @author      : zhouzirui
 * @param        {HiVenc_S} *pHandle：句柄
 * @param        {td_bool} bInstant：是否使能立即编码IDR帧
 * @return       {*}成功返回0,失败返回-1 
 */
static int mppVenc_request_idr(HiVenc_S *pHandle, td_bool bInstant)
{
    if (NULL == pHandle)
    {
        return TD_FAILURE;
    }
    /*请求IDR帧*/
    CHECK_API_RETURN(ss_mpi_venc_request_idr(pHandle->stNeedParam.nChn, bInstant));

    return TD_SUCCESS;
}

/**
 * @brief   : 发送原始图像进行编码
 * @param    {HiVenc_S} *pHandle：句柄
 * @param    {ot_video_frame_info} *pFrameInfo：视频帧信息类型指针
 * @param    {int} nTimeoutMs：超时时间
 * @return   {int} 成功返回0,失败返回-1
 */
static int mppVenc_send_stream(HiVenc_S *pHandle, const ot_video_frame_info *pFrameInfo, int nTimeoutMs)
{
    if (NULL == pHandle || NULL == pFrameInfo || nTimeoutMs < -1)
    {
        return TD_FAILURE;
    }

    CHECK_API_RETURN(ss_mpi_venc_send_frame(pHandle->stNeedParam.nChn, pFrameInfo, nTimeoutMs));

    return TD_SUCCESS;
}

/**
 * @brief       : 获取编码码流
 * @author      : zhouzirui
 * @param        {HiVenc_S} *pHandle：句柄
 * @param        {ot_venc_stream} *pStream：帧码流类型指针
 * @param        {int} nTimeoutMs
 * @return       {*}成功返回0,失败返回-1
 */
static int mppVenc_get_stream(HiVenc_S *pHandle, ot_venc_stream *pStream, int nTimeoutMs)
{
    if(NULL == pHandle || NULL == pStream || nTimeoutMs < -1)
    {
        return TD_FAILURE;    
    }
    fd_set read_fds;
    struct timeval timeout_val;
    FD_ZERO(&read_fds);
    /*获取编码通道对应的设备文件句柄*/
    pHandle->nVencFd = ss_mpi_venc_get_fd(pHandle->stNeedParam.nChn);
    if (pHandle->nVencFd < 0)
    {
        mpi_venc_log("ss_mpi_venc_get_fd failed with %#x!", pHandle->nVencFd);
        /* 出现异常，20ms后再重试 */
        usleep(20*1000);
        return TD_FAILURE;
    }
    FD_SET(pHandle->nVencFd, &read_fds);
    timeout_val.tv_sec  = nTimeoutMs / 1000;
    timeout_val.tv_usec = (nTimeoutMs % 1000) * 1000;
    int ret = select(pHandle->nVencFd + 1, &read_fds, NULL, NULL, &timeout_val);
    if (ret < 0)
    {
        mpi_venc_log("select failed! errno:%d, %s", errno, strerror(errno));
        /* 出现异常，20ms后再重试 */
        usleep(20*1000);
        return TD_FAILURE;
    }
    else if (ret == 0)
    {
        // mpi_venc_log("get venc stream time out, exit thread");
        return TD_FAILURE;
    }
    else
    {
        if (FD_ISSET(pHandle->nVencFd, &read_fds))
        {
            ot_venc_chn_status stStatus;
            CHECK_API_RETURN(ss_mpi_venc_query_status(pHandle->stNeedParam.nChn,&stStatus));
            // mpi_venc_log("stStatus.cur_packs:%d stStatus.left_stream_frames:%d", stStatus.cur_packs, stStatus.left_stream_frames);
            if(stStatus.cur_packs > 0 && stStatus.left_stream_frames > 0)
            {
                pStream->pack = (ot_venc_pack *)malloc(sizeof(ot_venc_pack) * stStatus.cur_packs);
                if (pStream->pack == NULL)
                {
                    mpi_venc_log("malloc memory failed!");
                    return TD_FAILURE;
                }
                pStream->pack_cnt = stStatus.cur_packs;
                CHECK_API_RETURN(ss_mpi_venc_get_stream(pHandle->stNeedParam.nChn, pStream, nTimeoutMs));
            }else{
                return TD_FAILURE;
            }
        }
        else
        {
            return TD_FAILURE;
        }
    }
    return TD_SUCCESS;
}

/**
 * @brief       : 释放码流缓存
 * @author      : zhouzirui
 * @param        {HiVenc_S} *pHandle：句柄
 * @param        {ot_venc_stream} *pStream：帧码流类型指针
 * @return       {*}成功返回0,失败返回-1
 */
static int mppVenc_release_stream(HiVenc_S *pHandle, ot_venc_stream *pStream)
{
    if(NULL == pHandle || NULL == pStream)
    {
        return TD_FAILURE;
    }
    CHECK_API_RETURN(ss_mpi_venc_release_stream(pHandle->stNeedParam.nChn, pStream));
    if(pStream->pack != NULL)
    {
        free(pStream->pack);
        pStream->pack = NULL;
    }

    return TD_SUCCESS;
}

/**
 * @brief   : 设置感兴趣编码区域属性
 * @param    {HiVenc_S} *pHandle：句柄
 * @param    {ot_venc_roi_attr} *pRoiAttrc：编码感兴趣区域信息指针
 * @return   {*}成功返回0,失败返回-1
 */
static int mppVenc_set_roi_attr(HiVenc_S *pHandle, ot_venc_roi_attr *pRoiAttr)
{
    if (NULL == pHandle || pRoiAttr->idx < 0 || pRoiAttr->idx > 7)
    {
        mpi_venc_log("设置感兴趣编码区域属性失败:参数错误");
        return TD_FAILURE;
    }

    if(pRoiAttr->enable == TD_TRUE && pHandle->stExParam.bSvcEnable == TD_TRUE)
    {
        mpi_venc_log("设置感兴趣编码区域属性失败:SVC与ROI不能同时使用");
        return TD_FAILURE;
    }

    /*编码通道*/
    ot_venc_chn nChn = pHandle->stNeedParam.nChn;
    ot_venc_roi_attr stRoiAttr;
    /*获取H.264/H.265/SVAC3通道的ROI属性*/
    CHECK_API_RETURN(ss_mpi_venc_get_roi_attr(nChn, pRoiAttr->idx, &stRoiAttr));
    stRoiAttr.idx = pRoiAttr->idx;
    stRoiAttr.enable = pRoiAttr->enable;
    stRoiAttr.is_abs_qp = pRoiAttr->is_abs_qp;
    stRoiAttr.qp = pRoiAttr->qp;
    stRoiAttr.rect.x = pRoiAttr->rect.x;
    stRoiAttr.rect.y = pRoiAttr->rect.y;
    stRoiAttr.rect.width = pRoiAttr->rect.width;
    stRoiAttr.rect.height = pRoiAttr->rect.height;
    /*设置H.264/H.265/SVAC3通道的ROI属性*/
    CHECK_API_RETURN(ss_mpi_venc_set_roi_attr(nChn, &stRoiAttr));

    return TD_SUCCESS;
}

/**
 * @brief   : 获取感兴趣编码区域属性
 * @param    {HiVenc_S} *pHandle：句柄
 * @param    {ot_venc_roi_attr} *pRoiAttr：编码感兴趣区域信息指针
 * @return   {*}成功返回0,失败返回-1
 */
static int mppVenc_get_roi_attr(HiVenc_S *pHandle, ot_venc_roi_attr *pRoiAttr)
{
    if (NULL == pHandle || pRoiAttr->idx < 0 || pRoiAttr->idx > 7)
    {
        return TD_FAILURE;
    }

    /*编码通道*/
    ot_venc_chn nChn = pHandle->stNeedParam.nChn;
    CHECK_API_RETURN(ss_mpi_venc_get_roi_attr(nChn, pRoiAttr->idx, &pRoiAttr));
    mpi_venc_log("ROI idx:%d", pRoiAttr->idx);

    return TD_SUCCESS;
}

/**
 * @brief   : 设置编码通道码率控制器的高级参数
 * @param    {HiVenc_S} *pHandle：句柄
 * @return   {*}成功返回0,失败返回-1
 */
static int mppVenc_set_rc_param(HiVenc_S *pHandle)
{
    if (NULL == pHandle)
    {
        return TD_FAILURE;
    }

    if(pHandle->stNeedParam.enCodec == OT_PT_JPEG)
    {
        return TD_SUCCESS;
    }

    /*编码通道*/
    ot_venc_chn nChn = pHandle->stNeedParam.nChn;
    /*设置编码通道码率控制器的高级参数*/
    ot_venc_rc_param stRcParam;
    CHECK_API_RETURN(ss_mpi_venc_get_rc_param(nChn, &stRcParam));
    mppVenc_rc_param_fill(pHandle, &stRcParam);
    CHECK_API_RETURN(ss_mpi_venc_set_rc_param(nChn, &stRcParam));

    return TD_SUCCESS;
}

/**
 * @brief   : 设置H.264/H.265协议编码通道的类型属性
 * @param    {HiVenc_S} *pHandle：句柄
 * @return   {int}成功返回0,失败返回-1
 */
static int mppVenc_set_codec_type_attr(HiVenc_S *pHandle)
{
    if (!pHandle)
    {
        return TD_FAILURE;
    }

    if(pHandle->stNeedParam.enCodec == OT_PT_JPEG || pHandle->stNeedParam.enCodec == OT_PT_MJPEG)
    {
        return TD_SUCCESS;
    }

    /*编码通道*/
    ot_venc_chn nChn = pHandle->stNeedParam.nChn;
    /*编码协议类型*/
    ot_payload_type enCodec = pHandle->stNeedParam.enCodec;

    /* 解析分数帧率：低16位 = 分子(N)，高16位 = 分母(D) */
    int nFrameRate = pHandle->stNeedParam.nOutFrameRate;
    td_u32 u32Numerator = nFrameRate & 0xFFFF;         // 分子 (低16位)
    td_u32 u32Denominator = (nFrameRate >> 16) & 0xFFFF; // 分母 (高16位)

    /* 兼容处理：如果高16位为0，则认为是普通整数帧率，分母为1 */
    if (u32Denominator == 0)
    {
        u32Denominator = 1;
    }

    // 基础时钟倍率，保持精度。通常设为1000或90000。
    // 这里使用1000作为基数，方便计算。
    // 最终的 num_units_in_tick = 分母 * 1000
    // 最终的 time_scale 由分子决定
    const td_u32 u32ClockBase = 1000;
    /* 每个时钟周期的单位数 */
    td_u32 u32Num_units_in_tick = u32Denominator * u32ClockBase;

    if(enCodec == OT_PT_H264)
    {
        ot_venc_h264_poc stH264Poc;
        /*获取H.264协议编码通道的POC类型*/
        CHECK_API_RETURN(ss_mpi_venc_get_h264_poc(nChn, &stH264Poc));
        stH264Poc.pic_order_cnt_type = 0; // POC 计算类型（0/1/2）
        /*设置H.264协议编码通道的POC类型*/
        CHECK_API_RETURN(ss_mpi_venc_set_h264_poc(nChn, &stH264Poc));

        ot_venc_h264_vui stH264Vui;
        /*获取H.264协议编码通道的Vui配置*/
        CHECK_API_RETURN(ss_mpi_venc_get_h264_vui(nChn, &stH264Vui));
        stH264Vui.vui_time_info.timing_info_present_flag = 1; // 是否包含时间信息（1bit）
        stH264Vui.vui_time_info.fixed_frame_rate_flag = 1;    // 是否为固定帧率（1bit）
        // H.264 公式: FPS = time_scale / (2 * num_units_in_tick)
        // 目标 FPS = Num / Den
        // 代入: Num / Den = (Num * 2 * Base) / (2 * (Den * Base)) -> 成立
        stH264Vui.vui_time_info.num_units_in_tick = u32Num_units_in_tick;     // 每个时钟周期的单位数
        stH264Vui.vui_time_info.time_scale = u32Numerator * 2 * u32ClockBase; // 时间刻度
        /*设置H.264协议编码通道的vui参数*/
        CHECK_API_RETURN(ss_mpi_venc_set_h264_vui(nChn, &stH264Vui));
    }
    else if (enCodec == OT_PT_H265)
    {
        ot_venc_h265_vui stH265Vui;
        /*获取H.265协议编码通道的Vui配置*/
        CHECK_API_RETURN(ss_mpi_venc_get_h265_vui(nChn, &stH265Vui));
        stH265Vui.vui_time_info.timing_info_present_flag = 1; // 标志位，是否包含时间信息
        // H.265 公式: FPS = time_scale / num_units_in_tick
        // 目标 FPS = Num / Den
        // 代入: Num / Den = (Num * Base) / (Den * Base) -> 成立
        stH265Vui.vui_time_info.num_units_in_tick = u32Num_units_in_tick; // 每个时钟周期的单位数
        stH265Vui.vui_time_info.time_scale = u32Numerator * u32ClockBase; // 时间刻度
        stH265Vui.vui_time_info.num_ticks_poc_diff_one_minus1 = 1;        // POC 差值对应的 ticks 数减1
        /*设置H.265协议编码通道的vui参数*/
        CHECK_API_RETURN(ss_mpi_venc_set_h265_vui(nChn, &stH265Vui));
    }

    return TD_SUCCESS;
}

/**
 * @brief   : 获取编码通道属性
 * @param    {HiVenc_S} *pHandle：句柄
 * @param    {ot_venc_chn_attr} *pChnAttr：编码通道属性结构体指针
 * @return   {int}成功返回0,失败返回-1
 */
static int mppVenc_get_chn_attr(HiVenc_S *pHandle, ot_venc_chn_attr *pChnAttr)
{
    if (!pHandle || !pChnAttr)
    {
        return TD_FAILURE;
    }

    /*编码通道*/
    ot_venc_chn nChn = pHandle->stNeedParam.nChn;
    /*获取编码通道属性*/
    CHECK_API_RETURN(ss_mpi_venc_get_chn_attr(nChn, pChnAttr));

    return TD_SUCCESS;
}

/**
 * @brief   : 设置编码通道属性
 * @param    {HiVenc_S} *pHandle：句柄
 * @param    {ot_venc_chn_attr} *pChnAttr：编码通道属性结构体指针
 * @return   {int}成功返回0,失败返回-1
 */
static int mppVenc_set_chn_attr(HiVenc_S *pHandle, ot_venc_chn_attr *pChnAttr)
{
    if (!pHandle || !pChnAttr)
    {
        return TD_FAILURE;
    }

    /*编码通道*/
    ot_venc_chn nChn = pHandle->stNeedParam.nChn;
    /*设置编码通道属性*/
    CHECK_API_RETURN(ss_mpi_venc_set_chn_attr(nChn, pChnAttr));

    return TD_SUCCESS;
}

/**
 * @brief   : 重新设置编码通道属性
 * @param    {HiVenc_S} *pHandle：句柄
 * @return   {int}成功返回0,失败返回-1
 */
static int mppVenc_reset_chn_attr(HiVenc_S *pHandle)
{
    if (!pHandle)
    {
        return TD_FAILURE;
    }

    /*编码通道*/
    ot_venc_chn nChn = pHandle->stNeedParam.nChn;
    /*编码通道属性*/
    ot_venc_chn_attr stChnAttr;
    /*获取编码通道属性*/
    CHECK_API_RETURN(ss_mpi_venc_get_chn_attr(nChn, &stChnAttr));
    stChnAttr.rc_attr.rc_mode = pHandle->stExParam.enRcMode;
    if (pHandle->stNeedParam.enCodec == OT_PT_H264)
    {
        mppVenc_h264_attr_fill(pHandle, &stChnAttr);
    }else if (pHandle->stNeedParam.enCodec == OT_PT_H265)
    {
        mppVenc_h265_attr_fill(pHandle, &stChnAttr);
    }else if (pHandle->stNeedParam.enCodec == OT_PT_SVAC3)
    {
        mppVenc_svac3_attr_fill(pHandle, &stChnAttr);
    }else if (pHandle->stNeedParam.enCodec == OT_PT_JPEG)
    {
        mppVenc_jpeg_attr_fill(pHandle, &stChnAttr);
    }else if (pHandle->stNeedParam.enCodec == OT_PT_MJPEG)
    {
        mppVenc_mjpeg_attr_fill(pHandle, &stChnAttr);
    }

    if (stChnAttr.venc_attr.type == OT_PT_MJPEG || stChnAttr.venc_attr.type == OT_PT_JPEG)
    {
        stChnAttr.gop_attr.gop_mode = OT_VENC_GOP_MODE_NORMAL_P;
        stChnAttr.gop_attr.normal_p.ip_qp_delta = 0;
    }else{
        stChnAttr.gop_attr.gop_mode = pHandle->stNeedParam.enGopMode;
        mppVenc_gop_attr_fill(pHandle, &stChnAttr.gop_attr);
    }

    /*设置编码通道属性*/
    CHECK_API_RETURN(ss_mpi_venc_set_chn_attr(nChn, &stChnAttr));

    return TD_SUCCESS;
}

/**
 * @brief   : 重新设置编码动态属性 
 * @param    {HiVenc_S} *pHandle：句柄
 * @return   {int}成功返回0,失败返回-1
 */
static int mppVenc_reset_attr(HiVenc_S *pHandle)
{
    if (!pHandle)
    {
        return TD_FAILURE;
    }

    /*编码通道*/
    ot_venc_chn nChn = pHandle->stNeedParam.nChn;
    /*重新设置编码通道属性*/
    CHECK_API_RETURN(mppVenc_reset_chn_attr(pHandle));

    /*设置编码通道码率控制器的高级参数*/
    CHECK_API_RETURN(mppVenc_set_rc_param(pHandle));

    /*设置H.264/H.265协议编码通道的类型属性*/
    CHECK_API_RETURN(mppVenc_set_codec_type_attr(pHandle));

    /* 设置JPEG协议编码通道的高级参数 */
    CHECK_API_RETURN(mppVenc_set_jpeg_config(pHandle));

    /* 设置MJPEG协议编码通道的高级参数 */
    // CHECK_API_RETURN(mppVenc_set_mjpeg_config(pHandle));

    /*开启智能编码SVC*/
    if(pHandle->stExParam.bSvcEnable == TD_TRUE)
    {
        CHECK_API_RETURN(ss_mpi_venc_enable_svc(nChn, TD_TRUE));
        CHECK_API_RETURN(mppVenc_set_svc_param(pHandle));
    }

    /*ROI*/
    for (int i = 0; i < OT_VENC_MAX_ROI_NUM; i++)
    {
        if (pHandle->stExParam.astRoiAttr[i].enable == TD_TRUE &&
            pHandle->stExParam.bSvcEnable == TD_FALSE) // 不能与SVC同时打开
        {
            if (pHandle->stExParam.astRoiAttr[i].rect.width == 0 ||
                pHandle->stExParam.astRoiAttr[i].rect.height == 0)
            {
                continue;
            }
            /*设置H.264/H.265/SVAC3通道的ROI属性*/
            CHECK_API_RETURN(ss_mpi_venc_set_roi_attr(nChn, &pHandle->stExParam.astRoiAttr[i]));
        }
    }

    return TD_SUCCESS;
}

static int mppVenc_set_chn_crop(HiVenc_S *pHandle, ot_crop_info stCropInfo)
{
    if (!pHandle)
    {
        return TD_FAILURE;
    }

    /*编码通道*/
    ot_venc_chn nChn = pHandle->stNeedParam.nChn;
    ot_venc_chn_param stChnParam;
    memset_s(&stChnParam, sizeof(ot_venc_chn_param), 0, sizeof(ot_venc_chn_param));

    /*获取通道参数*/
    CHECK_API_RETURN(ss_mpi_venc_get_chn_param(nChn, &stChnParam));
    memcpy_s(&stChnParam.crop_info, sizeof(ot_venc_chn_param), &stCropInfo, sizeof(ot_venc_chn_param));
    /*设置通道参数*/
    CHECK_API_RETURN(ss_mpi_venc_set_chn_param(nChn, &stChnParam));

    return TD_SUCCESS;
}

/**
 * @brief       : venc初始化
 * @author      : zhouzirui
 * @param        {HiVenc_S} *pHandle：句柄
 * @return       {*}成功返回0,失败返回-1
 */
static int mppVenc_init(HiVenc_S *pHandle)
{
    if (NULL == pHandle)
    {
        return TD_FAILURE;
    }
    /*编码通道*/
    ot_venc_chn nChn = pHandle->stNeedParam.nChn;
    /*编码通道属性*/
    ot_venc_chn_attr stChnAttr;
    /*编码通道连续接收并编码的帧数*/
    ot_venc_start_param stRecvParam;
    memset(&stChnAttr, 0, sizeof(ot_venc_chn_attr));
    memset(&stRecvParam, 0, sizeof(ot_venc_start_param));
    stChnAttr.venc_attr.type = pHandle->stNeedParam.enCodec;
    stChnAttr.venc_attr.max_pic_width = pHandle->stNeedParam.unWidth;
    stChnAttr.venc_attr.max_pic_height = pHandle->stNeedParam.unHeight;
    if (stChnAttr.venc_attr.type == OT_PT_MJPEG || stChnAttr.venc_attr.type == OT_PT_JPEG) {
        stChnAttr.venc_attr.buf_size =
            MPI_ALIGN_UP(pHandle->stNeedParam.unWidth, 16) * MPI_ALIGN_UP(pHandle->stNeedParam.unHeight, 16) * 4; /* 16 4 is a number */
    } else {
        stChnAttr.venc_attr.buf_size =
            MPI_ALIGN_UP(pHandle->stNeedParam.unWidth * pHandle->stNeedParam.unHeight * 3 / 4, 64); /*  3  4 64 is a number */
    }
    stChnAttr.venc_attr.profile = pHandle->stExParam.nProfile;
    stChnAttr.venc_attr.is_by_frame = TD_TRUE;
    stChnAttr.venc_attr.pic_width = pHandle->stNeedParam.unWidth;
    stChnAttr.venc_attr.pic_height = pHandle->stNeedParam.unHeight;
    stChnAttr.rc_attr.rc_mode = pHandle->stExParam.enRcMode;
    if (pHandle->stNeedParam.enCodec == OT_PT_H264)
    {
        mppVenc_h264_attr_fill(pHandle, &stChnAttr);
    }else if (pHandle->stNeedParam.enCodec == OT_PT_H265)
    {
        mppVenc_h265_attr_fill(pHandle, &stChnAttr);
    }else if (pHandle->stNeedParam.enCodec == OT_PT_SVAC3)
    {
        mppVenc_svac3_attr_fill(pHandle, &stChnAttr);
    }else if (pHandle->stNeedParam.enCodec == OT_PT_JPEG)
    {
        mppVenc_jpeg_attr_fill(pHandle, &stChnAttr);
    }else if (pHandle->stNeedParam.enCodec == OT_PT_MJPEG)
    {
        mppVenc_mjpeg_attr_fill(pHandle, &stChnAttr);
    }
    
    if (stChnAttr.venc_attr.type == OT_PT_MJPEG || stChnAttr.venc_attr.type == OT_PT_JPEG)
    {
        stChnAttr.gop_attr.gop_mode = OT_VENC_GOP_MODE_NORMAL_P;
        stChnAttr.gop_attr.normal_p.ip_qp_delta = 0;
    }else{
        stChnAttr.gop_attr.gop_mode = pHandle->stNeedParam.enGopMode;
        mppVenc_gop_attr_fill(pHandle, &stChnAttr.gop_attr);
    }

    /*开启智能编码SVC 编码通道配置*/
    if(pHandle->stExParam.bSvcEnable == TD_TRUE)
    {
        mppVenc_set_chn_config(pHandle);
    }

    /* 设置编码输出为单包模式，I帧中组合SPS、PPS、SEI */
    if(!gs_bIsSetOneStreamBuf)
    {
        CHECK_API_RETURN(mppVenc_set_one_stream_buf(pHandle, OT_PT_H264, 1));
        CHECK_API_RETURN(mppVenc_set_one_stream_buf(pHandle, OT_PT_H265, 1));
        CHECK_API_RETURN(mppVenc_set_one_stream_buf(pHandle, OT_PT_SVAC3, 1));
        CHECK_API_RETURN(mppVenc_set_one_stream_buf(pHandle, OT_PT_MJPEG, 1)); // MJPEG和JPEG设置一个即可
        gs_bIsSetOneStreamBuf = TD_TRUE;
    }

    /*创建编码通道*/
    CHECK_API_RETURN(ss_mpi_venc_create_chn(nChn, &stChnAttr));

    ot_venc_chn_param stChnParam;
    /*获取通道参数*/
    CHECK_API_RETURN(ss_mpi_venc_get_chn_param(nChn, &stChnParam));
    if(pHandle->stNeedParam.enCodec == OT_PT_JPEG)
    {
        stChnParam.frame_rate.src_frame_rate = pHandle->stNeedParam.nInFrameRate;
        stChnParam.frame_rate.dst_frame_rate = pHandle->stNeedParam.nOutFrameRate;
    }
    /* 编码通道优先级 */
    stChnParam.priority = nChn;
    /* 编码通道优先级参数,取值范围：[0, 2) */
    if (nChn > 0)
    {
        stChnParam.priority = 1;
    }
    /*设置通道参数*/
    CHECK_API_RETURN(ss_mpi_venc_set_chn_param(nChn, &stChnParam));

    /*设置编码通道码率控制器的高级参数*/
    CHECK_API_RETURN(mppVenc_set_rc_param(pHandle));

    /*设置H.264/H.265协议编码通道的类型属性*/
    CHECK_API_RETURN(mppVenc_set_codec_type_attr(pHandle));

    /* 设置JPEG协议编码通道的高级参数 */
    CHECK_API_RETURN(mppVenc_set_jpeg_config(pHandle));

    /* 设置MJPEG协议编码通道的高级参数 */
    // CHECK_API_RETURN(mppVenc_set_mjpeg_config(pHandle));

    /*开启智能编码SVC*/
    if(pHandle->stExParam.bSvcEnable == TD_TRUE)
    {
        CHECK_API_RETURN(ss_mpi_venc_enable_svc(nChn, TD_TRUE));
        CHECK_API_RETURN(mppVenc_set_svc_param(pHandle));
    }

    /*ROI*/
    for (int i = 0; i < OT_VENC_MAX_ROI_NUM; i++)
    {
        if (pHandle->stExParam.astRoiAttr[i].enable == TD_TRUE &&
            pHandle->stExParam.bSvcEnable == TD_FALSE) // 不能与SVC同时打开
        {
            if (pHandle->stExParam.astRoiAttr[i].rect.width == 0 ||
                pHandle->stExParam.astRoiAttr[i].rect.height == 0)
            {
                continue;
            }
            /*设置H.264/H.265/SVAC3通道的ROI属性*/
            CHECK_API_RETURN(ss_mpi_venc_set_roi_attr(nChn, &pHandle->stExParam.astRoiAttr[i]));
        }
    }

    /*开启编码通道接收输入图像，允许指定接收帧数，超出指定的帧数后自动停止接收图像*/
    stRecvParam.recv_pic_num = -1;
    CHECK_API_RETURN(ss_mpi_venc_start_chn(nChn, &stRecvParam));

    /*获取编码通道对应的设备文件句柄*/
    pHandle->nVencFd = ss_mpi_venc_get_fd(nChn);
    mpi_venc_log("pHandle->nVencFd:%d", pHandle->nVencFd);
    if (pHandle->nVencFd < 0)
    {
        mpi_venc_log("ss_mpi_venc_get_fd failed with %#x!", pHandle->nVencFd);
        return TD_FAILURE;
    }
    return TD_SUCCESS;
}

/**
 * @brief       : venc去初始化
 * @author      : zhouzirui
 * @param        {HiVenc_S} *pHandle：句柄
 * @return       {*}成功返回0,失败返回-1
 */
static int mppVenc_unInit(HiVenc_S *pHandle)
{
    if (NULL == pHandle)
    {
        return TD_FAILURE;
    }
    /*编码通道*/
    ot_venc_chn nChn = pHandle->stNeedParam.nChn;
    /*停止编码通道接收输入图像*/
    CHECK_API_RETURN(ss_mpi_venc_stop_chn(nChn));
    /*销毁编码通道*/
    CHECK_API_RETURN(ss_mpi_venc_destroy_chn(nChn));

    return TD_SUCCESS;
}

HiVenc_S *mppVenc_alloc(HiVencNeedParam_S stParam)
{
    HiVenc_S *pHandle = (HiVenc_S *)malloc(sizeof(HiVenc_S));
    memset(pHandle, 0, sizeof(HiVenc_S));

    //info /**********************必需参数***************************/
    /*分辨率*/
    pHandle->stNeedParam.unWidth = stParam.unWidth;
    pHandle->stNeedParam.unHeight = stParam.unHeight;
    pHandle->stNeedParam.unVirWidth = stParam.unVirWidth;
    pHandle->stNeedParam.unVirHeight = stParam.unVirHeight;
    /*编码类型*/
    pHandle->stNeedParam.enCodec = stParam.enCodec;
    /*输入图像格式*/
    pHandle->stNeedParam.enPixFormat = stParam.enPixFormat;
    /*gop*/
    pHandle->stNeedParam.nGop = stParam.nGop;
    pHandle->stNeedParam.enGopMode = stParam.enGopMode;
    /*编码通道*/
    pHandle->stNeedParam.nChn = stParam.nChn;
    /*输入的帧率*/
    pHandle->stNeedParam.nInFrameRate = stParam.nInFrameRate;
    /*输出的帧率*/
    pHandle->stNeedParam.nOutFrameRate = stParam.nOutFrameRate;
    /*压缩模式*/
    // pHandle->stNeedParam.enCompressMode = stParam.enCompressMode;
    /* 是否开启卷绕 */
    pHandle->stNeedParam.bWrapEnable = stParam.bWrapEnable;

    //info /**********************功能参数***************************/
    /*码流大小*/
    pHandle->stExParam.nBitRate = 4096;
    pHandle->stExParam.nMaxBitRate = 4096;
    pHandle->stExParam.nMinBitRate = 3072;
    pHandle->stExParam.nAverageBitrate = 2048;
    pHandle->stExParam.nBitrateSmoothing = 50;
    pHandle->stExParam.nImageQuality = 1;

    pHandle->stExParam.nProfile = 0;
    pHandle->stExParam.bSvcEnable = TD_FALSE;
    pHandle->stExParam.enRcMode = OT_VENC_RC_MODE_H264_CBR;
    pHandle->stExParam.i_qp = 25;
    pHandle->stExParam.p_qp = 30;
    pHandle->stExParam.b_qp = 32;
    memset(pHandle->stExParam.astRoiAttr, 0, sizeof(ot_venc_roi_attr) * OT_VENC_MAX_ROI_NUM);
    pHandle->nVencFd = 0;
    pHandle->stExParam.uQFactor = 75;
    //info /**********************函数列表***************************/
    pHandle->mppVenc_init                   = mppVenc_init;
    pHandle->mppVenc_unInit                 = mppVenc_unInit;
    pHandle->mppVenc_send_stream            = mppVenc_send_stream;
    pHandle->mppVenc_get_stream             = mppVenc_get_stream;
    pHandle->mppVenc_release_stream         = mppVenc_release_stream;
    pHandle->mppVenc_request_idr            = mppVenc_request_idr;
    pHandle->mppVenc_set_roi_attr           = mppVenc_set_roi_attr;
    pHandle->mppVenc_get_roi_attr           = mppVenc_get_roi_attr;
    pHandle->mppVenc_set_rc_param           = mppVenc_set_rc_param;
    pHandle->mppVenc_get_chn_attr           = mppVenc_get_chn_attr;
    pHandle->mppVenc_set_chn_attr           = mppVenc_set_chn_attr;
    pHandle->mppVenc_reset_chn_attr         = mppVenc_reset_chn_attr;
    pHandle->mppVenc_reset_attr             = mppVenc_reset_attr;
    pHandle->mppVenc_set_chn_crop           = mppVenc_set_chn_crop;

    return pHandle;
}

void mppVenc_release(HiVenc_S *pHandle)
{
    if (!pHandle)
    {
        free(pHandle);
        pHandle = NULL;
    }
}
