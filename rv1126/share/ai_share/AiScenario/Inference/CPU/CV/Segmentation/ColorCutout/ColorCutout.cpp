/*
 * @FilePath     : ColorCutout.cpp
 * @Author       : 吴才朋 wucp@kfb.cn
 * @Date         : 2024-08-20 08:40:01
 * @LastEditors: 李辉 lihui@kfb.cn
 * @LastEditTime: 2024-10-31 16:27:11
 * @Description  : 人像抠图
 */

#include "ColorCutout.hpp"

using namespace ColorCutout_NS;

CColorCutout::CColorCutout()
{

}
CColorCutout::~CColorCutout()
{
}

/* 内联函数, 人像抠图公式：(R+G-2B+fThreshold1)*(1+fThreshold2) */
cv::Mat CColorCutout::cutoutFormula(cv::Mat aImage, AiScenario_NS::CutoutParam_S sCutoutParam) 
{
    cv::Mat aDiff;
    cv::Mat aMask;
    
    std::vector<cv::Mat> vChannels;
    cv::split(aImage, vChannels);
    cv::Mat R1,G1,B1;
    vChannels[0].convertTo(B1, CV_16S);
    vChannels[1].convertTo(G1, CV_16S);
    vChannels[2].convertTo(R1, CV_16S);
    
    /* 设置抠像的背景 */
    switch(sCutoutParam.eBgColor)
    {
    	case AiScenario_NS::BLUE:
    	{	
    		cv::subtract(G1 + R1, B1 + B1, aDiff);
    		break;
    	}
    	case AiScenario_NS::GREEN:
    	{
    		cv::subtract(B1 + R1, G1 + G1, aDiff);
    		break;
    	}
        default:
        {
            dlog(LOG_ERROR, "该人像抠图背景格式 [%d]格式有误", sCutoutParam.eBgColor);
            return cv::Mat();
        }
    }
    if(aDiff.empty())
    {
        dlog(LOG_ERROR, "抠像公式有误");
    	return cv::Mat();
    }

    cv::add(cv::Scalar(sCutoutParam.fColorThres), aDiff, aDiff);
    aDiff.convertTo(aMask, CV_16S, 1+sCutoutParam.fEdgeThres);
	
    return aMask;
}
/* 高效的图像抠图函数 */
int CColorCutout::inference(AiScenario_NS::CutoutParam_S& sCutoutParam) 
{
    if (sCutoutParam.aInputImage.empty() || sCutoutParam.aInputImage.type() != CV_8UC3) 
    {
        dlog(LOG_ERROR, "输入图片为空或者输入格式有误");
        return -1;
    }
    /* 获取输入图片的宽高 */
    int nInputW = sCutoutParam.aInputImage.cols;
    int nInputH = sCutoutParam.aInputImage.rows;
    int nReSizeW,nReSizeH;


    cv::Mat aResizeImg;
    /* 输出抠像RGBA图片的宽高 */
    if(nInputW<=960 && nInputH<=512)
    {
    	nReSizeW = nInputW;   
    	nReSizeH = nInputH;   
        aResizeImg = sCutoutParam.aInputImage;
    }
    else
    {
    	nReSizeW = 960;  
    	nReSizeH = 512;  
        /* 图像缩放 */
        cv::resize(sCutoutParam.aInputImage,aResizeImg,cv::Size(nReSizeW,nReSizeH));
    }


    /* 去掉颜色背景，生成Mask图 */
    cv::Mat aMask = cutoutFormula(aResizeImg, sCutoutParam);
	if(aMask.empty())
	{
        return -1;
	}

    /* 闭运算、膨胀、开运算、高斯模糊 */
    cv::Mat k = cv::getStructuringElement(cv::MORPH_RECT, cv::Size(3, 3));
    if(sCutoutParam.bCloseMorPh)
    {
    	cv::morphologyEx(aMask, aMask, cv::MORPH_CLOSE, k, cv::Point(1,1),1);    
    }
    if(sCutoutParam.bErodeMode)
    {
    	cv::erode(aMask, aMask, k); 
    }
    if(sCutoutParam.bOpenMorPh)
    {
    	cv::morphologyEx(aMask, aMask, cv::MORPH_OPEN, k, cv::Point(1,1),1);
    }    
    if(sCutoutParam.bGaussianBlur)
    {
    	cv::GaussianBlur(aMask, aMask, cv::Size(3, 3), 0);   
    }    
    
    /* 缩放回原尺寸 */
    if(nReSizeW != nInputW || nReSizeH != nInputH)
    {
        cv::resize(aMask,aMask,cv::Size(nInputW,nInputH));
    }
	aMask.convertTo(aMask, CV_8U);

    /* 转换为灰度图像 */
    if(sCutoutParam.bSharpEnabled)
    {
        cv::Mat aGrayFrame;
        cv::cvtColor(sCutoutParam.aInputImage, aGrayFrame, cv::COLOR_BGR2GRAY);

        /* 使用拉普拉斯算子进行边缘检测 */
        cv::Mat aLaplacian;
        cv::Laplacian(aGrayFrame, aLaplacian, CV_16S);

        /* 计算拉普拉斯的绝对值并转换为8位图像 */
        cv::Mat aLaplacianAbs;
        cv::convertScaleAbs(aLaplacian, aLaplacianAbs);
        // 将边缘图像转换为三通道
        cv::cvtColor(aLaplacianAbs, aLaplacianAbs, cv::COLOR_GRAY2BGR);

        /* 将边缘与原图像结合，以实现锐化效果 */
        cv::addWeighted(sCutoutParam.aInputImage, 1.0, aLaplacianAbs, sCutoutParam.fSharp, 0, sCutoutParam.aInputImage);
    }
	
    /* 合并通道为RGBA图像 */
    sCutoutParam.aOutputImage.create(nInputW,nInputH,CV_8UC4);
    std::vector<cv::Mat> channels1;
    cv::split(sCutoutParam.aInputImage, channels1);
    channels1.push_back(aMask);
    cv::merge(channels1, sCutoutParam.aOutputImage);
    return 0;
}


