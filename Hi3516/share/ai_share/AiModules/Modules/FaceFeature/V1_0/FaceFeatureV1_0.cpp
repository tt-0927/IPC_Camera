/**
 * @file FaceFeatureV1_0.cpp
 * @author wucp (wucp@kfb.cn)
 * @date 2024-11-01
 *
 * @brief
 */
#include "FaceFeatureV1_0.hpp"
#include "SaveImage.hpp"

#define FaceFeatureLen 128

FaceFeature_NS::CFaceFeatureV1_0::CFaceFeatureV1_0(InParam_S stInParam)
    : m_stInParam(stInParam)
{
}

FaceFeature_NS::CFaceFeatureV1_0::~CFaceFeatureV1_0()
{
    unInit();
}

/* 初始化 */
bool FaceFeature_NS::CFaceFeatureV1_0::init()
{
    bool bRet = false;
    
    m_pImageFeature = new Inference_NS::CImageFeature(m_stInParam.strModelPath);
    if (m_pImageFeature && m_pImageFeature->init())
    {
        m_pImageFeature->getSizeLimit(
                0,
                m_nLimitWidth,
                m_nLimitHeight,
                m_nLimitChannel);
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
bool FaceFeature_NS::CFaceFeatureV1_0::unInit()
{
    if (m_pImageFeature)
    {
        delete m_pImageFeature;
        m_pImageFeature = nullptr;
    }
    return true;
}

/* 处理数据 */
bool FaceFeature_NS::CFaceFeatureV1_0::process(
    InData_S            stInData,
    std::vector<float> &nResult,
    OutData_S*          stOutData)
{
    OutData_S defaultOutData;

    // 如果传入的指针为空，则使用默认对象
    if (stOutData == nullptr)
    {
        stOutData = &defaultOutData;
    }

    nResult.clear();

    if (stInData.inMat.empty())
    {
        printf("传入图片为空\n");
        return false;
    }

    if (!m_pImageFeature)
    {
        printf("未初始化算法类\n");
        return false;
    }

    bool bRet = true;

    /* 前处理 */
    if (stInData.inMat.cols != m_nLimitWidth || stInData.inMat.rows != m_nLimitHeight)
    {
        cv::resize(stInData.inMat, stInData.inMat, cv::Size(m_nLimitWidth,m_nLimitHeight));
        // resizeAndPadImage(stInData.inMat, stInData.inMat);
    }

    /* 推理+后处理 */
    Inference_NS::InputData_S stInputData;
    stInputData.pData = (float*)stInData.inMat.data;
    stInputData.nDataSize = static_cast<size_t>(stInData.inMat.total() * stInData.inMat.elemSize() * sizeof(float));
    
    std::vector<Inference_NS::ClsData_S> vClsDatas;
    bRet = m_pImageFeature->inference(stInputData, vClsDatas);
    if (!bRet)
    {
        printf("算法分析失败\n");
        return false;
    }

    int numFeatures = vClsDatas[0].vFeature.size();
    // printf("图片特征向量为[%d]:", numFeatures);
    // for (int i = 0; i < vClsDatas[0].vFeature.size(); i++)
    // {
    //     printf("%f ",vClsDatas[0].vFeature[i]);
    // }
    // printf("\n");

    // if (numFeatures != FaceFeatureLen)
    // {
    //     bRet = false;
    //     printf("算法推理结果异常\n");
    //     return false;
    // }
    // else
    {
        normalize(vClsDatas[0].vFeature); 
        nResult = vClsDatas[0].vFeature;
    }

    stOutData->nChnId = stInData.nChnId;
    
    stOutData->validResult = bRet;
    stOutData->nType = 5; //Type_E::TARGET_COMPARE;

    return true;
}

/* 向量归一化 */
void FaceFeature_NS::CFaceFeatureV1_0::normalize(std::vector<float> &vFeature)
{
    float fNorm = 0.0f;
    /* 计算向量的范数（L2 范数） */
    for (float fFeature : vFeature)
    {
        fNorm += fFeature * fFeature;
    }
    fNorm = std::sqrt(fNorm);

    /* 如果向量为很小的数，不做处理 */
    if (fNorm > 1e-10)
    {
        for (size_t i = 0; i < vFeature.size(); ++i)
        {
            vFeature[i] /= fNorm;
        }
    }
}

/* 处理数据 */
bool FaceFeature_NS::CFaceFeatureV1_0::resizeAndPadImage(cv::Mat inputImage, cv::Mat &outputImage)
{
    int imageWidth = inputImage.cols;
    int imageHeight = inputImage.rows;
    float fResizeScale = static_cast<float>(m_nLimitWidth) / std::max(imageWidth, imageHeight);

    int newWidth = static_cast<int>(imageWidth * fResizeScale);
    int newHeight = static_cast<int>(imageHeight * fResizeScale);

    // std::cout << "计算后的缩放：" << newWidth << "x" << newHeight << std::endl;

    cv::Mat resizedImage;
    cv::resize(inputImage, resizedImage, cv::Size(newWidth, newHeight));

    cv::Mat output = cv::Mat::zeros(m_nLimitWidth, m_nLimitHeight, inputImage.type());

    int nXOffset = static_cast<int>((m_nLimitWidth - newWidth) / 2);
    int nYOffset = static_cast<int>((m_nLimitHeight - newHeight) / 2);

    resizedImage.copyTo(output(cv::Rect(nXOffset, nYOffset, newWidth, newHeight)));
    outputImage = output;

    return true;
}