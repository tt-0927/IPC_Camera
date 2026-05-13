#include "CameraObstruction.hpp"

Inference_NS::CComplexityDetect::CComplexityDetect()
{
}
Inference_NS::CComplexityDetect::~CComplexityDetect()
{
}

/* 摄像头画面模糊检测 */
bool Inference_NS::CComplexityDetect::inference(cv::Mat aImage, double &dResult)
{
    cv::Mat aResized, aChannel;
    std::vector<cv::Mat> aHsvChannels;
    cv::resize(aImage, aResized, aResizeSet);
    cv::cvtColor(aResized, aResized, cv::COLOR_BGR2HSV);
    cv::split(aResized, aHsvChannels);
    cv::GaussianBlur(aHsvChannels[2], aChannel, cv::Size{5, 5}, 0);

    cv::Mat aGradX, aGradY, aGrad;
    cv::Mat aAbsGradX, aAbsGradY;
    cv::Sobel(aChannel, aGradX, CV_16S, 1, 0, 3, 1, 0, cv::BORDER_DEFAULT);
    cv::convertScaleAbs(aGradX, aAbsGradX);
    cv::Sobel(aChannel, aGradY, CV_16S, 0, 1, 3, 1, 0, cv::BORDER_DEFAULT);
    cv::convertScaleAbs(aGradY, aAbsGradY);
    cv::addWeighted(aAbsGradX, 0.5, aAbsGradY, 0.5, 0, aGrad);
    cv::threshold(aGrad, aGrad, 50, 1, cv::THRESH_BINARY);

    float fWNumPixel = float(aResizeSet.width) / float(aSuperpixelSet.width);
    float fHNumPixel = float(aResizeSet.height) / float(aSuperpixelSet.height);
    cv::Mat aScoreMat(aSuperpixelSet.height, aSuperpixelSet.width, CV_32F, cv::Scalar(0));
    for (int w = 0; w < aSuperpixelSet.width; w++)
    {
        for (int h = 0; h < aSuperpixelSet.height; h++)
        {
            cv::Rect rect(int(fWNumPixel * w), int(fHNumPixel * h), int(fWNumPixel), int(fHNumPixel));
            cv::Scalar score_tmp = cv::mean(aGrad(rect));
            float *rowptr = aScoreMat.ptr<float>(h);
            rowptr[w] = score_tmp[0];
        }
    }
    cv::Mat means, stddev;
    cv::meanStdDev(aScoreMat, means, stddev);

    double dDivider = means.at<double>(0) * 2;
    double dScore = (dDivider > 0) ? stddev.at<double>(0) / dDivider : 1.0;
    dResult = dScore <= 1.0 ? dScore : 1.0;

    return true;
}