/**
 * @file PMNMDetectV1_0.cpp
 * @author wucp (wucp@kfb.cn)
 * @date 2024-10-22
 *
 * @brief
 */

#include "PMNMDetectV1_0.hpp"
#include "SaveImage.hpp"

#define nResultNum 6

PMNMDetect_NS::CPMNMDetectV1_0::CPMNMDetectV1_0(InParam_S stInParam)
    : m_stInParam(stInParam)
{
}

PMNMDetect_NS::CPMNMDetectV1_0::~CPMNMDetectV1_0()
{
    unInit();
}

/* 初始化 */
bool PMNMDetect_NS::CPMNMDetectV1_0::init()
{
    bool bRet = false;

    m_pPMNMDetect = new Inference_NS::CYoloUltralytics(m_stInParam.strModelPath);
    if (m_pPMNMDetect)
    {
        if (m_pPMNMDetect->init())
        {
            bRet = m_pPMNMDetect->getSizeLimit(
                0,
                m_nLimitWidth,
                m_nLimitHeight,
                m_nLimitChannel);
        }
    }

    if (!bRet)
    {
        printf("模型初始化失败 [%s]\n",
               m_stInParam.strModelPath.c_str());
        goto FAIL;
    }

    return bRet;

FAIL:

    unInit();

    return false;
}

/* 反初始化 */
bool PMNMDetect_NS::CPMNMDetectV1_0::unInit()
{
    if (m_pPMNMDetect)
    {
        delete m_pPMNMDetect;
        m_pPMNMDetect = nullptr;
    }
    return true;
}

/* 处理数据 */
bool PMNMDetect_NS::CPMNMDetectV1_0::process(
    InData_S stInData,
    std::vector<Result_S> &vResults)
{
    vResults.clear();
    // std::vector<float> vOutData;

    if (stInData.inMat.empty())
    {
        printf("传入图片为空\n");
        return false;
    }

    if (!m_pPMNMDetect)
    {
        printf("未初始化算法类\n");
        return false;
    }

    bool bRet = true;

    bRet = m_pPMNMDetect->setParam(stInData.stParam.fBoxThreshold, stInData.stParam.fNmsThreshold);
    if (!bRet)
    {
        printf("阈值参数设置错误，应该在0~1之间！！\n");
        return false;
    }

    if (m_stInParam.bDebug)
    {
        /* 保存图片 */
        if (!stInData.inMat.empty() && !m_stInParam.strOriginalDataPath.empty())
        {
            if (!Modules_NS::saveImage(stInData.inMat, m_stInParam.strOriginalDataPath))
            {
                printf("Debug-保存图片失败[%s]\n", m_stInParam.strOriginalDataPath.c_str());
            }
        }
    }

    /* 前处理 */
    if (stInData.inMat.channels() != m_nLimitChannel)
    {
        printf("模型需要的通道数和输入图片的通道数不一致 inMat[%d] != m_nLimitChannel[%d]\n",
               stInData.inMat.channels(),
               m_nLimitChannel);
        return false;
    }
    if (stInData.inMat.cols != m_nLimitWidth || stInData.inMat.rows != m_nLimitHeight)
    {
        // cv::resize(stInData.inMat, stInData.inMat, cv::Size(m_nLimitWidth, m_nLimitHeight));
        resizeAndPadImage(stInData.inMat, stInData.inMat);
    }

    Inference_NS::InputData_S stInputData;
    stInputData.pData = (float*)stInData.inMat.data;
    stInputData.nDataSize = static_cast<size_t>(stInData.inMat.total() * stInData.inMat.elemSize() * sizeof(float));
    stInputData.stBoxs.fConfidence = stInData.stParam.fBoxThreshold;
    stInputData.stBoxs.fNms = stInData.stParam.fNmsThreshold;

    std::vector<Inference_NS::BoxData_S> vBoxDatas;

    /* 推理+后处理 */
    bRet = m_pPMNMDetect->inference(stInputData, vBoxDatas);
    if (!bRet)
    {
        printf("算法分析失败\n");
        return false;
    }

    // if (vOutData.size() % nResultNum != 0)
    // {
    //     bRet = false;
    //     printf("算法推理结果异常\n");
    //     return false;
    // }

    // /* 将坐标扩回原来的大小 */
    // for (int nIndex = 0; nIndex < vOutData.size() / nResultNum; nIndex++)
    // {
    //     Result_S stResult;
    //     stResult.fX1 = (vOutData[nIndex * nResultNum + 0] - m_nXOffset) / m_fResizeScale;
    //     stResult.fY1 = (vOutData[nIndex * nResultNum + 1] - m_nYOffset) / m_fResizeScale;
    //     stResult.fX2 = (vOutData[nIndex * nResultNum + 2] - m_nXOffset) / m_fResizeScale;
    //     stResult.fY2 = (vOutData[nIndex * nResultNum + 3] - m_nYOffset) / m_fResizeScale;
    //     stResult.fBoxConfidence = vOutData[nIndex * nResultNum + 4];
    //     stResult.nID = int(vOutData[nIndex * nResultNum + 5]);
    //     if (stResult.nID < vClassNames.size())
    //     {
    //         stResult.sClassName = vClassNames[int(vOutData[nIndex * nResultNum + 5])];
    //     }
    //     vResults.push_back(stResult);
    // }

    for (int nIndex = 0; nIndex < vBoxDatas.size(); nIndex++)
    {
        Inference_NS::BoxData_S box = vBoxDatas.at(nIndex);
        Result_S stResult;
        stResult.fX1 = (float)(box.stBoxs.nX1 - m_nXOffset) / m_fResizeScale;
        stResult.fY1 = (float)(box.stBoxs.nY1 - m_nYOffset) / m_fResizeScale;
        stResult.fX2 = (float)(box.stBoxs.nX2 - m_nXOffset) / m_fResizeScale;
        stResult.fY2 = (float)(box.stBoxs.nY2 - m_nYOffset) / m_fResizeScale;
        stResult.fBoxConfidence = box.fConfidence;
        stResult.nID = box.nLabel;
        vResults.push_back(stResult);
    }

    return true;
}

/* 处理数据 */
bool PMNMDetect_NS::CPMNMDetectV1_0::resizeAndPadImage(cv::Mat inputImage, cv::Mat &outputImage)
{
    int imageWidth = inputImage.cols;
    int imageHeight = inputImage.rows;
    m_fResizeScale = static_cast<float>(m_nLimitWidth) / std::max(imageWidth, imageHeight);

    int newWidth = static_cast<int>(imageWidth * m_fResizeScale);
    int newHeight = static_cast<int>(imageHeight * m_fResizeScale);

    cv::Mat resizedImage;
    cv::resize(inputImage, resizedImage, cv::Size(newWidth, newHeight));

    cv::Mat output = cv::Mat::zeros(m_nLimitWidth, m_nLimitHeight, inputImage.type());

    m_nXOffset = static_cast<int>((m_nLimitWidth - newWidth) / 2);
    m_nYOffset = static_cast<int>((m_nLimitHeight - newHeight) / 2);

    resizedImage.copyTo(output(cv::Rect(m_nXOffset, m_nYOffset, newWidth, newHeight)));
    outputImage = output;

    return true;
}
