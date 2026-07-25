/*
 * @FilePath     : HeadCountPPV1_0.cpp
 * @Author       : 严泽辉 yanzeh@kfb.cn
 * @Date         : 2024-05-30 16:01:33
 * @LastEditors  : 严泽辉 yanzeh@kfb.cn
 * @LastEditTime : 2024-05-31 14:09:50
 * @Description  :
 */

#include "IIMPostProcess.hpp"

#include "dlog.h"

#include "opencv2/core/core.hpp"
#include "opencv2/imgcodecs.hpp"
#include "opencv2/imgproc.hpp"

/* 数据后处理 */
bool PostProcessV1_0_NS::cIIMPostProcess::postProcess(
    float*              pInput0,
    float*              pInput1,
    int                 nHeight,
    int                 nWidth,
    std::vector<float>& vPointsXY)
{
    if (nullptr == pInput0 ||
        nullptr == pInput1 ||
        nHeight <= 0 ||
        nWidth <= 0)
    {
        dlog(LOG_ERROR, "传入参数为空");
        return false;
    }

    /* 清空坐标点容器 */
    vPointsXY.clear();

    cv::Mat aImgNew, aLabels, aStats, aCentroids;
    aImgNew.create(nHeight, nWidth, CV_8UC1);
    aLabels.create(nHeight, nWidth, CV_8UC1);
    aStats.create(nHeight, nWidth, CV_8UC1);
    aCentroids.create(nHeight, nWidth, CV_8UC1);
    for (int h = 0; h < nHeight; h++)
    {
        for (int w = 0; w < nWidth; w++)
        {
            if (pInput1[h * nWidth + w] >= pInput0[h * nWidth + w])
            {
                aImgNew.at<uchar>(h * nWidth + w) = 255;
            }
            else
            {
                aImgNew.at<uchar>(h * nWidth + w) = 0;
            }
        }
    }

    /* 定义最小面积阈值 */
    int nArea;
    /* 进行连通组件分析 */
    int nNumLabels = 0;

    nNumLabels = cv::connectedComponentsWithStats(aImgNew, aLabels, aStats, aCentroids);

    /* 遍历每个连通组件的统计信息 */
    int nPepleNum = 0;

    for (int i = 1; i < nNumLabels; i++)
    {
        /* 获取连通组件的统计信息 */
        cv::Rect bbox(aStats.at<int>(i, cv::CC_STAT_LEFT),
                      aStats.at<int>(i, cv::CC_STAT_TOP),
                      aStats.at<int>(i, cv::CC_STAT_WIDTH),
                      aStats.at<int>(i, cv::CC_STAT_HEIGHT));
        nArea = aStats.at<int>(i, cv::CC_STAT_AREA);
        /* 判断连通组件的面积是否大于指定最小面积 */
        if (nArea >= 3)
        {
            /* 将连通组件的边界框和质心点添加到结果矩阵和质心点矩阵中 */
            nPepleNum++;

            vPointsXY.insert(vPointsXY.begin(), { aCentroids.at<double>(i, 0), aCentroids.at<double>(i, 1) });
        }
    }

    return true;
}
