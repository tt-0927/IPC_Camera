/**
 * @file VehiclePlateDetect.cpp
 * @author songww
 * @date 2025-10-30
 * 
 * @brief 车牌识别
 */
#include "VehiclePlateDetectV1_0.hpp"
#include "SaveImage.hpp"
#include "StatisticsTimer.hpp"

/* 一组数据的大小 */
using namespace VehiclePlateDetect_NS;

VehiclePlateDetect_NS::CVehiclePlateDetectV1_0::CVehiclePlateDetectV1_0(InParam_S stInParam)
    : m_stInParam(stInParam)
{
}

VehiclePlateDetect_NS::CVehiclePlateDetectV1_0::~CVehiclePlateDetectV1_0()
{
    unInit();
}

/* 初始化 */
bool VehiclePlateDetect_NS::CVehiclePlateDetectV1_0::init()
{
    CStatisticsTimer runTime("车牌检测初始化耗时");
    bool bRet = false;

    m_pLicensePlateRec = new Inference_NS::CLicensePlateRec(m_stInParam.strModelPath);
    if (m_pLicensePlateRec && m_pLicensePlateRec->init())
    {
        bRet = true;
    }

    if (!bRet)
    {
        printf("模型初始化失败 [%s]\n", m_stInParam.strModelPath.c_str());
        goto FAIL;
    }
    return bRet;

FAIL:
    unInit();
    return false;
}

/* 反初始化 */
bool VehiclePlateDetect_NS::CVehiclePlateDetectV1_0::unInit()
{
    CStatisticsTimer runTime("停车检测反初始化耗时");
    if (m_pLicensePlateRec)
    {
        delete m_pLicensePlateRec;
        m_pLicensePlateRec = nullptr;
    }
    return true;
}


/* 处理数据 */
bool VehiclePlateDetect_NS::CVehiclePlateDetectV1_0::process(
    InData_S& stInData,
    OutData_S& stOutData)
{

    if (stInData.inMat.empty())
    {
        printf("传入图片为空\n");
        return false;
    }

    if (!m_pLicensePlateRec)
    {
        printf("未初始化算法类\n");
        return false;
    }
    /* i420格式 */
    stInData.inMat;
    /* 维度转换 */
    Inference_NS::InputData_S stInputData;
    /* 获取图像大小 */
    #ifdef RK_3588
    stInputData.nDataSize = static_cast<size_t>(stInData.inMat.total() * stInData.inMat.elemSize() * sizeof(float));
    stInputData.pData = (float*)stInData.inMat.data;
    #else
    stInputData.nDataSize = static_cast<size_t>(stInData.inMat.total() * stInData.inMat.elemSize());
    stInputData.pData = (float*)stInData.inMat.data;
    #endif
    std::vector<Inference_NS::ClsData_S> vClsDatas;
    {

        bool result = m_pLicensePlateRec->inference(stInputData,vClsDatas);
        if(!result)
        {
            return false;
        }
    }
    stOutData.vClsDatas = vClsDatas;

    // printf("识别的车牌号的token下标为:");
    // for(int i=0;i<vClsDatas[0].vCls.size();i++)
    // {
    //     printf("%d ",vClsDatas[0].vCls[i].nLabel);
    // }
    // printf("\n");
    // printf("识别到车牌的颜色下标nLabel[%d]-fConfidence[%f]\n", vClsDatas[0].stCls.nLabel, vClsDatas[0].stCls.fConfidence);
    
    return true;
}

/* 处理数据 */
bool VehiclePlateDetect_NS::CVehiclePlateDetectV1_0::resizeAndPadImage(cv::Mat inputImage, cv::Mat &outputImage)
{
    int imageWidth = inputImage.cols;
    int imageHeight = inputImage.rows;
    m_fResizeScale = static_cast<float>(m_nLimitWidth) / std::max(imageWidth, imageHeight);

    // std::cout << "imageWidth：" << imageWidth << std::endl;
    // std::cout << "imageHeight：" << imageHeight << std::endl;
    // std::cout << "m_nLimitWidth：" << m_nLimitWidth << std::endl;
    // std::cout << "m_fResizeScale" << m_fResizeScale << std::endl;

    int newWidth = static_cast<int>(imageWidth * m_fResizeScale);
    int newHeight = static_cast<int>(imageHeight * m_fResizeScale);

    // std::cout << "计算后的缩放：" << newWidth << "x" << newHeight << std::endl;

    cv::Mat resizedImage;
    cv::resize(inputImage, resizedImage, cv::Size(newWidth, newHeight));

    cv::Mat output = cv::Mat::zeros(m_nLimitWidth, m_nLimitHeight, inputImage.type());

    m_nXOffset = static_cast<int>((m_nLimitWidth - newWidth) / 2);
    m_nYOffset = static_cast<int>((m_nLimitHeight - newHeight) / 2);

    resizedImage.copyTo(output(cv::Rect(m_nXOffset, m_nYOffset, newWidth, newHeight)));
    outputImage = output;

    return true;
}

bool VehiclePlateDetect_NS::CVehiclePlateDetectV1_0::resizeAndPadImage2(cv::Mat inputImage, cv::Mat &outputImage)
{
    int imageWidth = inputImage.cols;
    int imageHeight = inputImage.rows;
    m_fResizeScale = static_cast<float>(m_nLimitWidth) / std::max(imageWidth, imageHeight);

    int newWidth = static_cast<int>(imageWidth * m_fResizeScale);
    int newHeight = static_cast<int>(imageHeight * m_fResizeScale);

    cv::Mat resizedImage;
    cv::resize(inputImage, resizedImage, cv::Size(newWidth, newHeight));

    m_nXOffset = static_cast<int>((m_nLimitWidth - newWidth) / 2);
    m_nYOffset = static_cast<int>((m_nLimitHeight - newHeight) / 2);
    int nBottom = m_nLimitHeight - newHeight - m_nYOffset;
    int mRight = m_nLimitWidth - newWidth - m_nXOffset;

    cv::copyMakeBorder(resizedImage, outputImage,
                       m_nYOffset, nBottom, m_nXOffset, mRight, 
                       cv::BORDER_CONSTANT, cv::Scalar(0, 0, 0));
    return true;
}

/* 判断两个多边形是否有交点 */
bool VehiclePlateDetect_NS::CVehiclePlateDetectV1_0::isIntersecting(
    std::vector<cv::Point> rectPolygon,
    std::vector<cv::Point> polygons)
{
    std::vector<cv::Point> intersectionPoints;
    int result = cv::intersectConvexConvex(rectPolygon, polygons, intersectionPoints);
    if (intersectionPoints.empty())
    {
        return false;
    }
    else
    {
        return true;
    }
}
