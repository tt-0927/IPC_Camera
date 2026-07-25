#pragma once

#include "dcl_ive.h"
#include "base_type.hpp"
#include <unistd.h>
namespace dcl
{
    /**
     * @brief 旋转图片
     * @param stSrc 输入图片数据
     * @param stDst 输出图片数据
     * @param stCenter 旋转中心
     * @param fRotateAngle 旋转角度（正数为逆时针）
     * @param stRotationmatrix 旋转矩阵
     * @return int
     */
    int dclRotation(
        dcl::Mat &stSrc,
        dcl::Mat &stDst,
        dclPoint2f stCenter,
        float fRotateAngle,
        dclIveAffineMatrix &stRotationmatrix)
    {
        dclIvePicInfo picInfo;
        picInfo.picFormat = stSrc.pixelFormat;
        picInfo.virAddr = (uint64_t)(stSrc.data);
        picInfo.phyAddr = stSrc.phyAddr;
        picInfo.picBufferSize = stSrc.size();
        picInfo.picHeight = stSrc.h();
        picInfo.picWidth = stSrc.w();
        picInfo.picHeightStride = stSrc.h();
        picInfo.picWidthStride = stSrc.w();

        dclIvePicInfo dstInfo;
        dstInfo.picFormat = stDst.pixelFormat;
        dstInfo.virAddr = (uint64_t)(stDst.data);
        dstInfo.phyAddr = stDst.phyAddr;
        dstInfo.picBufferSize = stDst.size();
        dstInfo.picHeight = stDst.h();
        dstInfo.picWidth = stDst.w();
        dstInfo.picHeightStride = stDst.h();
        dstInfo.picWidthStride = stDst.w();

        /* 获取旋转矩阵 */
        dclError ret = dcliveGetRotationMatrix2D(stCenter, fRotateAngle, 1.0, &stRotationmatrix);
        if (ret != DCL_ERROR_NONE)
        {
            return ret;
        }

        /* 设置变换参数 */
        dclIveWarpParam warp_param;
        warp_param.pad = E_WARP_PAD_CONST_ZERO; /* 使用常量0填充边界 */
        warp_param.inter = E_WARP_INTER_LINEAR; /* 使用线性插值 */

        /* 应用旋转 */
        return dcliveWarpAffine(1, &picInfo, &stRotationmatrix, &dstInfo, &warp_param);
    }

    /**
     * @brief 获取旋转后的坐标
     * @param stBox 旋转前的坐标，得到的结果也会赋值给该变量
     * @param nIimageWidth 图片的宽
     * @param nImageHeight 图片的高
     * @param stCenter 旋转中心
     * @param stRotationmatrix 旋转矩阵
     * @return true
     * @return false
     */
    bool rotatedBox(Box &stBox,
                    int nIimageWidth,
                    int nImageHeight,
                    dclPoint2f stCenter,
                    dclIveAffineMatrix stRotationmatrix)
    {
        // 将点转换为以旋转中心为原点的坐标系
        std::vector<Point2f> corners = {
            {stBox.x1 - stCenter.x, stBox.y1 - stCenter.y},
            {stBox.x2 - stCenter.x, stBox.y1 - stCenter.y},
            {stBox.x2 - stCenter.x, stBox.y2 - stCenter.y},
            {stBox.x1 - stCenter.x, stBox.y2 - stCenter.y}};

        std::vector<Point2f> rotatedCorners(4);
        /* 应用旋转矩阵（注意：这里假设旋转矩阵是在以旋转中心为原点的坐标系中计算的）*/
        for (int i = 0; i < 4; i++)
        {
            rotatedCorners[i].x = stRotationmatrix.aa[0][0] * corners[i].x +
                                  stRotationmatrix.aa[0][1] * corners[i].y +
                                  stCenter.x; /* 转换回图像坐标系 */
            rotatedCorners[i].y = stRotationmatrix.aa[1][0] * corners[i].x +
                                  stRotationmatrix.aa[1][1] * corners[i].y +
                                  stCenter.y; /* 转换回图像坐标系 */
        }

        /* 计算边界框 */
        float min_x = rotatedCorners[0].x;
        float max_x = rotatedCorners[0].x;
        float min_y = rotatedCorners[0].y;
        float max_y = rotatedCorners[0].y;
        for (int i = 1; i < 4; i++)
        {
            min_x = std::min(min_x, rotatedCorners[i].x);
            max_x = std::max(max_x, rotatedCorners[i].x);
            min_y = std::min(min_y, rotatedCorners[i].y);
            max_y = std::max(max_y, rotatedCorners[i].y);
        }

        /* 确保在图像范围内 */
        stBox.x1 = std::max(0.0f, min_x);
        stBox.y1 = std::max(0.0f, min_y);
        stBox.x2 = std::min(static_cast<float>(nIimageWidth - 1), max_x);
        stBox.y2 = std::min(static_cast<float>(nImageHeight - 1), max_y);

        return true;
    }
}