/**
 * @FilePath     : stream_vi.cpp
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2025-12-02 09:50:53
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2025-12-04 15:39:04
 * @Description  : VI 视频采集输入
 */

#include "stream_vi.h"
#include "IpcRet.h"
#include "video_define.h"

#define RK_COLOR_YUV_Y_PLANE 0
#define RK_COLOR_YUV_UV_PLANE 1
#define RK_MAX_COLOR_COMPONENT 2

int save_yuv_to_file(FILE *file, VIDEO_FRAME_S *frame)
{
    // 计算 YUV 数据大小
    size_t y_size = frame->u32Width * frame->u32Height;        // Y 分量的大小
    size_t uv_size = (frame->u32Width * frame->u32Height) / 2; // UV 分量的大小

    // 写入 Y 数据
    size_t written_y = fwrite(frame->pVirAddr[RK_COLOR_YUV_Y_PLANE], 1, y_size, file);
    if (written_y != y_size)
    {
        perror("写入 Y 数据失败");
        fclose(file);
        return -1; // 返回错误
    }

    // 写入 UV 数据（假设 UV 数据存储在同一个平面中，即 NV12 格式）
    size_t written_uv = fwrite(frame->pVirAddr[RK_COLOR_YUV_UV_PLANE], 1, uv_size, file);
    if (written_uv != uv_size)
    {
        perror("写入 UV 数据失败");
        fclose(file);
        return -1; // 返回错误
    }

    return 0; // 返回成功
}

void printVideoFrame(const VIDEO_FRAME_S *frame)
{
    if (frame == NULL)
    {
        dlog(LOG_DEBUG, "frame is NULL");
        return;
    }

    dlog(LOG_DEBUG, "VIDEO_FRAME_S:");
    dlog(LOG_DEBUG, "  Width: %u", frame->u32Width);            // 图像实际宽度
    dlog(LOG_DEBUG, "  Height: %u", frame->u32Height);          // 图像实际⾼度
    dlog(LOG_DEBUG, "  VirWidth: %u", frame->u32VirWidth);      // 图像虚宽
    dlog(LOG_DEBUG, "  VirHeight: %u", frame->u32VirHeight);    // 图像虚⾼
    dlog(LOG_DEBUG, "  Field: %d", frame->enField);             // 帧场模式
    dlog(LOG_DEBUG, "  PixelFormat: %d", frame->enPixelFormat); // ⽬标图像像素格式
    // dlog(LOG_DEBUG, "  VideoFormat: %d", frame->enVideoFormat); // 不⽀持
    dlog(LOG_DEBUG, "  CompressMode: %d", frame->enCompressMode); // ⽬标图像压缩模式
    // dlog(LOG_DEBUG, "  DynamicRange: %d", frame->enDynamicRange); // 不⽀持
    // dlog(LOG_DEBUG, "  ColorGamut: %d", frame->enColorGamut); // 不⽀持

    // 打印虚拟地址
    // 图像Y和UV的地址
    // 取值范围:
    // Y地址:pVirAddr[RK_COLOR_YUV_Y_PLANE]
    // uv地址:pVirAddr[RK_COLOR_YUV_UV_PLANE
    for (int i = 0; i < RK_MAX_COLOR_COMPONENT; i++)
    {
        dlog(LOG_DEBUG, "  pVirAddr[%d]: %p", i, frame->pVirAddr[i]);
    }

    // dlog(LOG_DEBUG, "  TimeRef: %u", frame->u32TimeRef);    //不⽀持
    dlog(LOG_DEBUG, "  PTS: %llu", frame->u64PTS); // 图像时间戳
    // dlog(LOG_DEBUG, "  PrivateData: %lu", frame->u64PrivateData);   //不⽀持
    dlog(LOG_DEBUG,
         "  FrameFlag: %u",
         frame->u32FrameFlag); // 当前帧的标记，使⽤FRAME_FLAG_E⾥⾯的值标记，可以按位或操作
}

RkVi_S *streamVi_init()
{
    RkVi_S *pHandle = (RkVi_S *) malloc(sizeof(RkVi_S));
    if (pHandle == NULL)
    {
        dlog_error("申请内存失败");
        return nullptr;
    }
    memset(pHandle, 0, sizeof(RkVi_S));

    RkViNeedParam_S stNeedParam;
    memset(&stNeedParam, 0, sizeof(RkViNeedParam_S));
    /* vi配置初始化 */
    stNeedParam.nDevId = 0;
    stNeedParam.nChannel = 0;
    memcpy(stNeedParam.aEnityName, VI_CHN_VIDEO_DEV, sizeof(VI_CHN_VIDEO_DEV));
    stNeedParam.nWidth = PIXEL_WIDTH_4K;
    stNeedParam.nHeight = PIXEL_HEIGHT_4K;
    stNeedParam.enPixelFormat = RK_FMT_YUV420SP;
    stNeedParam.enCompressMode = COMPRESS_MODE_NONE;
    stNeedParam.enMemType = VI_V4L2_MEMORY_TYPE_DMABUF; // 经过isp 推荐设置为4 不推荐设置为1
    memcpy(stNeedParam.sGdcFecFile, VI_GDCFILE_PATH, sizeof(VI_GDCFILE_PATH));

    pHandle = rockitVi_alloc(stNeedParam);
    int nRet = OK;
    /*初始化vi*/
    nRet = pHandle->rockitVi_init(pHandle);
    if (nRet != OK)
    {
        dlog_error("初始化Vi失败");
        return nullptr;
    }

    dlog_info("Vi初始化成功");
    return pHandle;
}

int streamVi_uninit(RkVi_S *pHandle)
{
    if (pHandle == NULL)
    {
        dlog(LOG_ERROR, "句柄为空");
        return ERR_PTR_NULL;
    }
    int nRet = OK;
    /*反初始化vi*/
    nRet = pHandle->rockitVi_uninit(pHandle);
    if (nRet < OK)
    {
        dlog_error("去初始化Vi失败");
        return nRet;
    }
    rockitVi_release(pHandle);

    dlog_info("Vi去初始化成功");
    return nRet;
}
