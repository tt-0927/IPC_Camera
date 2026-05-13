/**
 * @FilePath     : stream_sys.cpp
 * @Author       : zhouzirui
 * @Date         : 2025-05-19 16:09:50
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2025-12-22 09:08:37
 * @Description  : MPI系统/视频缓存池
 */

#include "stream_sys.h"
#include "av_configure.h"
#include "dlog.h"

/**
 * @brief   : 计算Vi离线所需vb
 * @param    {vector<Video_NS::VideoConfig_S>} &vstVideoConfig 视频配置
 * @param    {td_u32} *pBufSize VB大小
 * @return   {int} 0：成功，非0：失败
 */
int streamSys_compute_viOffLine(const std::vector<Video_NS::VideoConfig_S> &vstVideoConfig, td_u32 *pBufSize)
{
    if(pBufSize == nullptr || vstVideoConfig.empty())
    {
        return ERR;
    }

    ot_pic_buf_attr stBufAttr;
    memset_s(&stBufAttr, sizeof(ot_pic_buf_attr), 0, sizeof(ot_pic_buf_attr));
    stBufAttr.width = PIXEL_WIDTH_2_5K;
    stBufAttr.height = PIXEL_HEIGHT_2_5K;
    stBufAttr.align = OT_DEFAULT_ALIGN;
    stBufAttr.bit_width = OT_DATA_BIT_WIDTH_8;
    stBufAttr.pixel_format = OT_PIXEL_FORMAT_RGB_BAYER_12BPP;
    stBufAttr.compress_mode = OT_COMPRESS_MODE_NONE;
    stBufAttr.video_format = OT_VIDEO_FORMAT_LINEAR;

    *pBufSize = ot_common_get_pic_buf_size(&stBufAttr);
    dlog_debug("Vi离线所需vb :%d", *pBufSize);

    return OK;
}

/**
 * @brief   : 计算VPSS-VENC卷绕buffer大小common vb
 * @param    {vector<Video_NS::VideoConfig_S>} &vstVideoConfig 视频配置
 * @param    {td_u32} *pBufSize VB大小
 * @return   {int} 0：成功，非0：失败
 */
int streamSys_compute_vpssVencWrap(const std::vector<Video_NS::VideoConfig_S> &vstVideoConfig, td_u32 *pBufSize)
{
    if(pBufSize == nullptr || vstVideoConfig.empty())
    {
        return ERR;
    }

    /*VPSS-VENC卷绕参数结构体*/
    ot_vpss_venc_wrap_param stWrapParam;
    memset(&stWrapParam, 0, sizeof(ot_vpss_venc_wrap_param));
    /*buffer像素行数*/
    td_u32 u32BufLine = 0;

    stWrapParam.frame_rate = 30;
    stWrapParam.all_online = TD_TRUE;
    if (SENSOR0_TYPE == SC4336P_MIPI_4M_30FPS_10BIT || SENSOR0_TYPE == OS04D10_MIPI_4M_30FPS_10BIT ||
        SENSOR0_TYPE == GC4023_MIPI_4M_30FPS_10BIT || SENSOR0_TYPE == SC431HAI_MIPI_4M_30FPS_10BIT ||
        SENSOR0_TYPE == SC431HAI_MIPI_4M_30FPS_10BIT_WDR2TO1)
    {
        stWrapParam.full_lines_std = 1500; /* full_lines_std: 1500 */
    }
    else if (SENSOR0_TYPE == SC450AI_MIPI_4M_30FPS_10BIT || SENSOR0_TYPE == SC450AI_MIPI_4M_30FPS_10BIT_WDR2TO1)
    {
        stWrapParam.full_lines_std = 1585; /* full_lines_std: 1585 */
    }
    else if (SENSOR0_TYPE == SC500AI_MIPI_5M_30FPS_10BIT || SENSOR0_TYPE == SC500AI_MIPI_5M_30FPS_10BIT_WDR2TO1
             || SENSOR0_TYPE == SC533HAI_MIPI_5M_30FPS_10BIT || SENSOR0_TYPE == SC533HAI_MIPI_5M_30FPS_10BIT_WDR2TO1)
    {
        stWrapParam.full_lines_std = 1700; /* full_lines_std: 1700 */
    }
    // stWrapParam.large_stream_size.width = vstVideoConfig[0].stVideoResolution.nWidth;
    // stWrapParam.large_stream_size.height = vstVideoConfig[0].stVideoResolution.nHeight;
    stWrapParam.large_stream_size.width = PIXEL_WIDTH_2_5K;
    stWrapParam.large_stream_size.height = PIXEL_HEIGHT_2_5K;
    // stWrapParam.small_stream_size.width = 0; // 无小码流可以将分辨率设置成0
    // stWrapParam.small_stream_size.height = 0;
    /*存在多路小码流,设置为面积（宽x高）最大的那一路*/
    // stWrapParam.small_stream_size.width = PIXEL_WIDTH_1024;
    // stWrapParam.small_stream_size.height = PIXEL_HEIGHT_576;
    /* 因业务压力过大，手动放大小码流尺寸，使申请更大的buf_line及size，避免miss start、ring back、ring buf full中断丢帧 */
    stWrapParam.small_stream_size.width = PIXEL_WIDTH_1680;
    stWrapParam.small_stream_size.height = PIXEL_HEIGHT_954;

    dlog_debug("小码流尺寸: %dx%d", stWrapParam.small_stream_size.width, stWrapParam.small_stream_size.height);
    /*获取VPSS-VENC卷绕模式下的buffer像素行数*/
    ss_mpi_sys_get_vpss_venc_wrap_buf_line(&stWrapParam, &u32BufLine);

    ot_pic_buf_attr stBufAttr;
    memset_s(&stBufAttr, sizeof(ot_pic_buf_attr), 0, sizeof(ot_pic_buf_attr));
    stBufAttr.width = PIXEL_WIDTH_2_5K;
    stBufAttr.height = PIXEL_HEIGHT_2_5K;
    stBufAttr.align = OT_DEFAULT_ALIGN;
    stBufAttr.bit_width = OT_DATA_BIT_WIDTH_8;
    stBufAttr.pixel_format = OT_PIXEL_FORMAT_YVU_SEMIPLANAR_420;
    stBufAttr.compress_mode = OT_COMPRESS_MODE_SEG_COMPACT;
    stBufAttr.video_format = OT_VIDEO_FORMAT_LINEAR;
    // note：CPU不能每帧都立即响应VI/VPSS/VENC中断，存在中断延迟等情况，适当增大buf_line
    //  u32BufLine = u32BufLine + 128;

    /*计算卷绕buffer大小*/
    *pBufSize = ot_comm_get_vpss_venc_wrap_buf_size(&stBufAttr, u32BufLine);
    dlog_debug("卷绕 wrap_buf_line: %d wrap_buf_size: %d", u32BufLine, *pBufSize);

    return OK;
}

/**
 * @brief   : 计算小码流VENC所需vb
 * @param    {vector<Video_NS::VideoConfig_S>} &vstVideoConfig 视频配置
 * @param    {td_u32} *pBufSize VB大小
 * @return   {int} 0：成功，非0：失败
 */
int streamSys_compute_vencLowBitrateStream(const std::vector<Video_NS::VideoConfig_S> &vstVideoConfig, td_u32 *pBufSize)
{
    if(pBufSize == nullptr || vstVideoConfig.empty())
    {
        return ERR;
    }

    ot_pic_buf_attr stBufAttr;
    memset_s(&stBufAttr, sizeof(ot_pic_buf_attr), 0, sizeof(ot_pic_buf_attr));
    Video_NS::VideoCapabilitySet_S stCapabilitySet;
    Convert::read_file(VIDEO_CAPABILITY_SET_FILE, stCapabilitySet);
    Video_NS::VideoResolution_S stVideoResolution;
    stVideoResolution.parse_string(stCapabilitySet.stSub.aResolution.begin()->strName);
    stBufAttr.width = stVideoResolution.nWidth;
    stBufAttr.height = stVideoResolution.nHeight;
    stBufAttr.align = OT_DEFAULT_ALIGN;
    stBufAttr.bit_width = OT_DATA_BIT_WIDTH_8;
    stBufAttr.pixel_format = OT_PIXEL_FORMAT_RGB_BAYER_12BPP;
    stBufAttr.compress_mode = OT_COMPRESS_MODE_NONE;
    stBufAttr.video_format = OT_VIDEO_FORMAT_LINEAR;

    *pBufSize = ot_common_get_pic_buf_size(&stBufAttr);
    dlog_debug("小码流VENC所需vb :%d", *pBufSize);

    return OK;
}

int streamSys_init(const std::vector<Video_NS::VideoConfig_S> &vstVideoConfig)
{
    int nRet = OK;
    /*去初始化海思MPI系统*/
    nRet = mppSys_uninit();
    if (nRet != OK)
    {
        dlog_error("去初始化海思MPI系统失败");
        return ERR;
    }
    /*去初始化MPP视频缓存池*/
    nRet = mppVb_uninit();
    if (nRet != OK)
    {
        dlog_error("去初始化MPP视频缓存池失败");
        return ERR;
    }
    ot_vb_cfg stVbCfg;
    memset(&stVbCfg, 0, sizeof(ot_vb_cfg));
    stVbCfg.max_pool_cnt = OT_VB_MAX_POOLS;

    /* 计算VPSS-VENC卷绕buffer大小common vb */
    td_u32 wrap_buf_size = 0;
    nRet = streamSys_compute_vpssVencWrap(vstVideoConfig, &wrap_buf_size);
    if (nRet != OK)
    {
        dlog_error("计算blk_size失败");
        return ERR;
    }

    stVbCfg.common_pool[0].blk_size = wrap_buf_size;
    stVbCfg.common_pool[0].blk_cnt = 1;
    stVbCfg.common_pool[0].remap_mode = OT_VB_REMAP_MODE_NONE;

    /*设置MPP视频缓存池属性*/
    nRet = mppVb_set_cfg(stVbCfg);
    if (nRet != OK)
    {
        dlog_error("设置MPP视频缓存池属性失败");
        return ERR;
    }

    /* 设置VB内存的附加信息 */
    // td_u32 supplement_config = OT_VB_SUPPLEMENT_JPEG_MASK | OT_VB_SUPPLEMENT_BNR_MOT_MASK;
    // ot_vb_supplement_cfg supplement_conf = {0};
    // supplement_conf.supplement_cfg = supplement_config;
    // ss_mpi_vb_set_supplement_cfg(&supplement_conf);

    /*初始化MPP视频缓存池*/
    nRet = mppVb_init();
    if (nRet != OK)
    {
        dlog_error("初始化MPP视频缓存池失败");
        return ERR;
    }

    /*初始化海思MPI系统*/
    nRet = mppSys_init();
    if (nRet != OK)
    {
        dlog_error("MPI系统初始化失败 错误码为:%x",nRet);
        return ERR;
    }

    dlog_info("MPI系统/视频缓存池初始化成功");
    return OK;
}

int streamSys_deinit()
{
    int nRet = OK;
    /*去初始化海思MPI系统*/
    nRet = mppSys_uninit();
    if (nRet != OK)
    {
        dlog_error("MPI系统去初始化失败");
        return ERR;
    }
    /*去初始化MPP视频缓存池*/
    nRet = mppVb_uninit();
    if (nRet != OK)
    {
        dlog_error("去初始化MPP视频缓存池失败");
        return ERR;
    }

    dlog_info("MPI系统/视频缓存池去初始化成功");
    return OK;
}

