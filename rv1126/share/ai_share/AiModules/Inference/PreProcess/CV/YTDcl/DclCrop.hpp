#pragma once

#include "base_type.hpp"
#include "dcl_ive.h"
#include <unistd.h>
namespace dcl
{
    int dclCropResize(dcl::Mat &src, dcl::Mat &dst, dcl::Box &roi)
    {
        dclIvePicInfo picInfo;
        picInfo.picFormat = src.pixelFormat;
        picInfo.virAddr = (uint64_t)(src.data);
        picInfo.phyAddr = src.phyAddr;
        picInfo.picBufferSize = src.size();
        picInfo.picHeight = src.h();
        picInfo.picWidth = src.w();
        picInfo.picHeightStride = src.h();
        picInfo.picWidthStride = src.w();

        dclIveCropResizeInfo cropResizeInfo;
        cropResizeInfo.dstPic.picFormat = dst.pixelFormat;
        cropResizeInfo.dstPic.virAddr = (uint64_t)(dst.data);
        cropResizeInfo.dstPic.phyAddr = dst.phyAddr;
        cropResizeInfo.dstPic.picBufferSize = dst.size();
        cropResizeInfo.dstPic.picHeight = dst.h();
        cropResizeInfo.dstPic.picWidth = dst.w();
        cropResizeInfo.dstPic.picHeightStride = dst.h();
        cropResizeInfo.dstPic.picWidthStride = dst.w();
        cropResizeInfo.crop.roi.x = roi.x1;
        cropResizeInfo.crop.roi.y = roi.y1;
        cropResizeInfo.crop.roi.width = roi.w();
        cropResizeInfo.crop.roi.height = roi.h();
        cropResizeInfo.resize.height = dst.h();
        cropResizeInfo.resize.width = dst.w();
        cropResizeInfo.resize.interpolation = 0;

        uint32_t chn = 0;
        uint32_t count = 1;
        uint64_t taskId;
        int32_t milliSec = -1;
        int e = dcliveCropResize(chn, &picInfo, &cropResizeInfo, count, &taskId, milliSec);
        if (e != DCL_SUCCESS)
        {
            printf("dclmpiVpcCrop fail, error code: %d\n", e);
            return -1;
        }

        while (dcliveGetProcessResult(chn, taskId, milliSec) != DCL_SUCCESS)
        {
            usleep(1000000);
        }
        return 0;
    }
}