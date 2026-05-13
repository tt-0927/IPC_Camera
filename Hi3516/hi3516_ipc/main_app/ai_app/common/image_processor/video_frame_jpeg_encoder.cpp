/**
 * @FilePath     : video_frame_jpeg_encoder.cpp
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2026-04-29 13:50:30
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-04-29 14:19:54
 * @Description  : 视频帧 JPEG 编码公共工具实现
 */

#include "video_frame_jpeg_encoder.hpp"

#include <cstdio>
#include <fstream>

#include "dlog.h"
#include "ffmpeg_image.h"
#include "IpcRet.h"

namespace AiAppCommon
{
int encode_video_frame_to_jpeg_file(ot_video_frame_info *pFrameInfo, const std::string &strFilename)
{
    if (pFrameInfo == nullptr || strFilename.empty())
    {
        dlog_error("视频帧 JPEG 编码参数无效");
        return ERR_PARAM_NULL;
    }

    /* 公共编码逻辑以人脸抓拍原保存功能为准，直接使用 MPP 视频帧首地址编码整帧 JPEG */
    CFfmpegImage image;
    if (!image.Open(strFilename, pFrameInfo->video_frame.width, pFrameInfo->video_frame.height))
    {
        return ERR;
    }

    /* 当前整帧 YUV 数据首地址 */
    char *pData = reinterpret_cast<char *>(pFrameInfo->video_frame.virt_addr[0]);
    /* 当前整帧 YUV420 数据长度 */
    const int nDataSize = pFrameInfo->video_frame.width * pFrameInfo->video_frame.height * 3 / 2;
    /* JPEG 编码结果，调用方根据返回值决定是否读取或上报图片 */
    const bool bEncodeOk = image.SendFrame(pData, nDataSize);
    image.Close();
    return bEncodeOk ? OK : ERR;
}

int encode_video_frame_to_jpeg_memory(ot_video_frame_info *pFrameInfo, EventTvSdkImage_S &stImage)
{
    stImage = EventTvSdkImage_S();
    if (pFrameInfo == nullptr)
    {
        dlog_error("视频帧 JPEG 内存编码参数无效");
        return ERR_PARAM_NULL;
    }

    /* 生成唯一临时文件路径，避免多线程冲突 */
    const std::string strTempPath = "/tmp/ai_event_frame_" + std::to_string(reinterpret_cast<std::uintptr_t>(pFrameInfo)) +
                                    "_" + std::to_string(pFrameInfo->video_frame.width) + "x" +
                                    std::to_string(pFrameInfo->video_frame.height) + ".jpg";

    if (encode_video_frame_to_jpeg_file(pFrameInfo, strTempPath) != OK)
    {
        std::remove(strTempPath.c_str());
        return ERR;
    }

    /* 读取临时文件到内存 */
    std::ifstream file(strTempPath, std::ios::binary);
    if (!file.is_open())
    {
        dlog_warn("读取 JPEG 临时文件失败[%s]", strTempPath.c_str());
        std::remove(strTempPath.c_str());
        return ERR;
    }

    file.seekg(0, std::ios::end);
    const std::streampos nFileSize = file.tellg();
    if (nFileSize <= 0)
    {
        std::remove(strTempPath.c_str());
        return ERR;
    }

    file.seekg(0, std::ios::beg);
    stImage.vecJpeg.resize(static_cast<size_t>(nFileSize));
    file.read(reinterpret_cast<char *>(stImage.vecJpeg.data()), static_cast<std::streamsize>(nFileSize));
    const bool bReadOk = file.gcount() == nFileSize;
    file.close();
    std::remove(strTempPath.c_str());

    if (!bReadOk)
    {
        stImage.vecJpeg.clear();
        return ERR;
    }

    stImage.nWidth = pFrameInfo->video_frame.width;
    stImage.nHeight = pFrameInfo->video_frame.height;
    stImage.strTag = "panorama";
    return OK;
}
} // namespace AiAppCommon
