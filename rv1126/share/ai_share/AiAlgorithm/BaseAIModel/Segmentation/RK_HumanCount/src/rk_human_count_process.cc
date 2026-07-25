/*
 *  File Name: postprocess.cc
 *  Created on: 2023年7月13日
 *  Author: wcp
 *  description : 对模型输出的数据处理，获得具体的人数和在图片绘制关键点和人数
 *  Modify date: 2023年7月18日
 */

#include "rk_human_count_process.h"


/* 图像预处理方法 */
int ctrlNetoutputProcess(float* pInput0, float* pInput1, int nHeight, int nWidth, TunableParam_S stTunableParam, int& nPepleNum, std::vector<float>& vPointsXY)
{
    cv::Mat aImgNew(nHeight, nWidth, CV_8UC1);
    cv::Mat aLabels(nHeight, nWidth, CV_8UC1);
    cv::Mat aStats(nHeight, nWidth, CV_8UC1);
    cv::Mat aCentroids(nHeight, nWidth, CV_8UC1);

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
    int     nArea;
    /* 进行连通组件分析 */
    int     num_labels = 0;
    num_labels = cv::connectedComponentsWithStats(aImgNew, aLabels, aStats, aCentroids);
    /* 遍历每个连通组件的统计信息 */
    nPepleNum  = 0;
    /* 清空坐标点容器 */
    vPointsXY.clear();
    /* 在图像上绘制一个点 */
    cv::Scalar color(stTunableParam.stPointParam.nB, stTunableParam.stPointParam.nG, stTunableParam.stPointParam.nR);
    for (int i = 1; i < num_labels; i++)
    {
        /* 获取连通组件的统计信息 */
        cv::Rect bbox(aStats.at<int>(i, cv::CC_STAT_LEFT),
                      aStats.at<int>(i, cv::CC_STAT_TOP),
                      aStats.at<int>(i, cv::CC_STAT_WIDTH),
                      aStats.at<int>(i, cv::CC_STAT_HEIGHT));
        nArea = aStats.at<int>(i, cv::CC_STAT_AREA);
        /* 判断连通组件的面积是否大于指定最小面积 */
        if (nArea >= stTunableParam.nMinArea)
        {
            /* 将连通组件的边界框和质心点添加到结果矩阵和质心点矩阵中 */
            nPepleNum++;

            vPointsXY.insert(vPointsXY.begin(), { aCentroids.at<double>(i, 0), aCentroids.at<double>(i, 1) });
        }
    }

    std::string strContextTxt = strlen(stTunableParam.stTextParam.strContextTxt) == 0 ? "total target numbers = " + std::to_string(nPepleNum) : stTunableParam.stTextParam.strContextTxt;

    return 1;
}
