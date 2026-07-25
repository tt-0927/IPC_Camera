/**
 * @file DclResize.hpp
 * @author wucp (wucp@kfb.cn)
 * @date 2025-10-14
 *
 * @brief
 */

#pragma once
#include "dcl_ive.h"
#include "dcl_memory.h"
#include "dcl_mpi.h"
#include <cassert>
#include <unistd.h>
#include "color.hpp"

namespace dcl
{
    typedef enum
    {
        NONE = 0,
        LEFT_TOP,
        CENTER
    } paddingType_t;

    static int resizeCvtPaddingOp(const unsigned char *in_data, int channels,
                                  int in_h, int in_w, unsigned char *out_data,
                                  int out_h, int out_w, colorSpace_t colorSpace,
                                  paddingType_t paddingType = NONE,
                                  unsigned char paddingValue = 114)
    {
        float scale_x = float(in_w) / out_w;
        float scale_y = float(in_h) / out_h;
        float scale = scale_x > scale_y ? scale_x : scale_y;
        int offset_w, offset_h;
        int target_w, target_h;
        if (NONE == paddingType)
        {
            target_w = out_w;
            target_h = out_h;
            scale_x = float(in_w) / target_w;
            scale_y = float(in_h) / target_h;
            offset_w = 0;
            offset_h = 0;
        }
        else if (LEFT_TOP == paddingType)
        {
            if (scale_x > scale_y)
            {
                target_w = out_w;
                target_h = int(in_h / scale);
            }
            else
            {
                target_h = out_h;
                target_w = int(in_w / scale);
            }
            offset_w = 0;
            offset_h = 0;
            memset(out_data, paddingValue, out_w * out_h * channels);
        }
        else if (CENTER == paddingType)
        {
            if (scale_x > scale_y)
            {
                target_w = out_w;
                target_h = int(in_h / scale);
                offset_w = 0;
                offset_h = (out_h - target_h) / 2;
            }
            else
            {
                target_h = out_h;
                target_w = int(in_w / scale);
                offset_w = (out_w - target_w) / 2;
                offset_h = 0;
            }
            memset(out_data, paddingValue, out_w * out_h * channels);
        }
        else
        {
            DCL_APP_LOG(DCL_ERROR, "Not support padding type: %d", paddingType);
            return -1;
        }

        for (int dc = 0; dc < channels; ++dc)
        {
            for (int dh = 0; dh < target_h; ++dh)
            {
                for (int dw = 0; dw < target_w; ++dw)
                {
                    float fx = (dw + 0.5f) * scale_x - 0.5f;
                    float fy = (dh + 0.5f) * scale_y - 0.5f;
                    int sx = int(floor(fx));
                    int sy = int(floor(fy));
                    fx -= sx;
                    fy -= sy;

                    if (sx < 0)
                    {
                        fx = 0;
                        sx = 0;
                    }

                    if (sx >= in_w - 1)
                    {
                        fx = 1;
                        sx = in_w - 2;
                    }

                    if (sy < 0)
                    {
                        fy = 0;
                        sy = 0;
                    }

                    if (sy >= in_h - 1)
                    {
                        fy = 1;
                        sy = in_h - 2;
                    }

                    float cbufx_x = 1.0f - fx;
                    float cbufx_y = fx;

                    float cbufy_x = 1.0f - fy;
                    float cbufy_y = fy;

                    float v00 = in_data[(sy + 0) * in_w * channels + (sx + 0) * channels + dc];
                    float v01 = in_data[(sy + 1) * in_w * channels + (sx + 0) * channels + dc];
                    float v10 = in_data[(sy + 0) * in_w * channels + (sx + 1) * channels + dc];
                    float v11 = in_data[(sy + 1) * in_w * channels + (sx + 1) * channels + dc];

                    float val = cbufx_x * cbufy_x * v00 + cbufx_x * cbufy_y * v01 + cbufx_y * cbufy_x * v10 + cbufx_y * cbufy_y * v11;

                    if (IMAGE_COLOR_BGR888_TO_RGB888_PLANAR == colorSpace)
                    {
                        out_data[(2 - dc) * out_h * out_w + (dh + offset_h) * out_w + (dw + offset_w)] = round(val);
                    }
                    else if (IMAGE_COLOR_BGR888_TO_BGR888_PLANAR == colorSpace)
                    {
                        out_data[dc * out_h * out_w + (dh + offset_h) * out_w + (dw + offset_w)] = round(val);
                    }
                    else if (IMAGE_COLOR_BGR888_TO_BGR888 == colorSpace)
                    {
                        out_data[(dh + offset_h) * out_w * channels + (dw + offset_w) * channels + dc] = round(val);
                    }
                    else if (IMAGE_COLOR_BGR888_TO_RGB888 == colorSpace)
                    {
                        out_data[(dh + offset_h) * out_w * channels + (dw + offset_w) * channels + (2 - dc)] = round(val);
                    }
                    else
                    {
                        printf("Not support color space: %d\n", colorSpace);
                        return -1;
                    }
                }
            }
        }
        return 0;
    }

    static int dclResizeCvtPaddingOp(const dcl::Mat &src, dcl::Mat &dst,
                                     paddingType_t paddingType = NONE,
                                     unsigned char paddingValue = 114)
    {
        float scale_x = float(src.w()) / dst.w();
        float scale_y = float(src.h()) / dst.h();
        float scale = scale_x > scale_y ? scale_x : scale_y;
        int padding_left, padding_top, padding_right, padding_bottom;
        int target_w, target_h;
        if (NONE == paddingType)
        {
            target_w = dst.w();
            target_h = dst.h();
            padding_left = 0;
            padding_top = 0;
            padding_right = 0;
            padding_bottom = 0;
        }
        else if (LEFT_TOP == paddingType)
        {
            padding_left = 0;
            padding_top = 0;
            if (scale_x > scale_y)
            {
                target_w = dst.w();
                target_h = int(src.h() / scale);
                padding_right = 0;
                padding_bottom = dst.h() - target_h;
            }
            else
            {
                target_h = dst.h();
                target_w = int(src.w() / scale);
                padding_right = dst.w() - target_w;
                padding_bottom = 0;
            }
        }
        else if (CENTER == paddingType)
        {
            if (scale_x > scale_y)
            {
                target_w = dst.w();
                target_h = int(src.h() / scale);
                padding_left = 0;
                padding_top = (dst.h() - target_h) / 2;
                padding_right = 0;
                padding_bottom = dst.h() - target_h - padding_top;
            }
            else
            {
                target_h = dst.h();
                target_w = int(src.w() / scale);
                padding_left = (dst.w() - target_w) / 2;
                padding_top = 0;
                padding_right = dst.w() - target_w - padding_left;
                padding_bottom = 0;
            }
        }
        else
        {
            printf("Not support padding type: %d\n", paddingType);
            return -1;
        }

        dclIveCropResizeMakeBorderInfo info;
        info.crop.roi.x = 0;
        info.crop.roi.y = 0;
        info.crop.roi.width = src.w();
        info.crop.roi.height = src.h();
        info.resize.width = target_w;
        info.resize.height = target_h;
        info.resize.interpolation = 0;
        info.border.leftPadSize = padding_left;
        info.border.topPadSize = padding_top;
        info.border.rightPadSize = padding_right;
        info.border.bottomPadSize = padding_bottom;
        info.border.value[0] = paddingValue;
        info.border.value[1] = paddingValue;
        info.border.value[2] = paddingValue;
        info.border.mode = 0;

        dclIvePicInfo picIn;
        picIn.picWidth = src.w();
        picIn.picHeight = src.h();
        picIn.picFormat = src.pixelFormat;
        picIn.picWidthStride = src.w();
        picIn.picHeightStride = src.h();
        picIn.phyAddr = src.phyAddr;
        picIn.virAddr = (uint64_t)(src.data);
        picIn.picBufferSize = src.size();

        info.dstPic.picWidth = dst.w();
        info.dstPic.picHeight = dst.h();
        info.dstPic.picFormat = dst.pixelFormat;
        info.dstPic.picWidthStride = dst.w();
        info.dstPic.picHeightStride = dst.h();
        info.dstPic.phyAddr = dst.phyAddr;
        info.dstPic.virAddr = (uint64_t)(dst.data);
        info.dstPic.picBufferSize = dst.size();

        uint64_t taskId;
        dclError ret = dcliveCropCvtColorResizeMakeBorder(0, &picIn, &info, 1, &taskId, -1);
        if (ret != DCL_SUCCESS)
        {
            printf("dclmpiVpcCropCvtColorResizeMakeBorder fail, error code: %d\n", ret);
            return -1;
        }

        while (dcliveGetProcessResult(0, taskId, -1) != DCL_SUCCESS)
        {
            usleep(1000000);
        }
        return 0;
    }
} // namespace dcl
