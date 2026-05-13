/**
 * @FilePath     : stream_vi.cpp
 * @Author       : zhouzirui
 * @Date         : 2025-03-21 10:28:57
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2025-12-17 19:38:05
 * @Description  : VI 视频流采集输入
 */

#include "stream_vi.h"
#include "dlog.h"

HiVi_S *streamVi_init()
{
    HiVi_S *pHandle = (HiVi_S *)malloc(sizeof(HiVi_S));
    if (pHandle == NULL)
    {
        dlog_error("申请内存失败");
        return nullptr;
    }
    memset(pHandle, 0, sizeof(HiVi_S));

    HiViNeedParam_S stNeedParam;
    stNeedParam.nDevId = 0;
    stNeedParam.nChannel = 0;
    // stNeedParam.nWidth = PIXEL_WIDTH_2K;
    // stNeedParam.nHeight = PIXEL_HEIGHT_2K;
    stNeedParam.nWidth = PIXEL_WIDTH_2_5K;
    stNeedParam.nHeight = PIXEL_HEIGHT_2_5K;
    stNeedParam.enPixelFormat = OT_PIXEL_FORMAT_RGB_BAYER_12BPP;
    stNeedParam.enCompressMode = OT_COMPRESS_MODE_NONE;
    memcpy(stNeedParam.aEnityName, "/dev/video11", sizeof("/dev/video11"));
#ifdef SENSOR_SC500AI
    stNeedParam.sns_type = SC500AI_MIPI_5M_30FPS_10BIT;
#elif defined(SENSOR_SC533HAI)
    stNeedParam.sns_type = SC533HAI_MIPI_5M_30FPS_10BIT;
#endif

    pHandle = mppVi_alloc(stNeedParam);

    pHandle->stExParam.nDepth = 4;

    int nRet = OK;
    /*初始化vi*/
    nRet = pHandle->mppVi_init(pHandle);
    if (nRet != OK)
    {
        dlog_error("streamVi mppVi_init error");
        return nullptr;
    }

    dlog_info("Vi初始化成功");
    return pHandle;
}

int streamVi_uninit(HiVi_S *pHandle)
{
    if (pHandle == NULL)
    {
        dlog(LOG_ERROR, "句柄为空");
        return ERR_PTR_NULL;
    }
    int nRet = OK;
    /*反初始化vi*/
    nRet = pHandle->mppVi_uninit(pHandle);
    if (nRet < OK)
    {
        dlog_error("streamVi mppVi_uninit error");
        return nRet;
    }
    mppVi_release(pHandle);

    dlog_info("Vi去初始化成功");
    return nRet;
}

