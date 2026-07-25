/**
 * @file FaceLandmarkV1_0.cpp
 * @author wucp (wucp@kfb.cn)
 * @date 2024-11-01
 *
 * @brief
 */
#include <algorithm>
#include "FaceLandmarkV1_0.hpp"
#include "SaveImage.hpp"

#define nResultNum 16
#define nLandmarkNum 1946

FaceLandmark_NS::CFaceLandmarkV1_0::CFaceLandmarkV1_0(InParam_S stInParam)
    : m_stInParam(stInParam)
{
}

FaceLandmark_NS::CFaceLandmarkV1_0::~CFaceLandmarkV1_0()
{
    unInit();
}

/* 初始化 */
bool FaceLandmark_NS::CFaceLandmarkV1_0::init()
{
    bool bRet = false;

    /* 人脸检测模型初始化 */
    m_pFaceDetect = new Inference_NS::CYolov5Face(m_stInParam.strDetectModelPath);
    if (m_pFaceDetect)
    {
        if (m_pFaceDetect->init())
        {
            bRet = m_pFaceDetect->getSizeLimit(
                0,
                m_nDetectLimitWidth,
                m_nDetectLimitHeight,
                m_nDetectLimitChannel);
        }
    }
    if (!bRet)
    {
        printf("模型初始化失败 [%s]\n",
               m_stInParam.strDetectModelPath.c_str());
        goto FAIL;
    }
    /* 人脸特征提取模型初始化 */
    m_pFaceLandmark = new Inference_NS::CFaceLandmark1000(m_stInParam.strLandmarkModelPath);
    if (m_pFaceLandmark)
    {
        if (m_pFaceLandmark->init())
        {
            bRet = m_pFaceLandmark->getSizeLimit(
                0,
                m_nLandmarkLimitWidth,
                m_nLandmarkLimitHeight,
                m_nLandmarkLimitChannel);
        }
    }

    if (!bRet)
    {
        printf("模型初始化失败 [%s]\n",
               m_stInParam.strLandmarkModelPath.c_str());
        goto FAIL;
    }
    return bRet;

FAIL:

    unInit();

    return false;
}

/* 反初始化 */
bool FaceLandmark_NS::CFaceLandmarkV1_0::unInit()
{
    if (m_pFaceDetect)
    {
        delete m_pFaceDetect;
        m_pFaceDetect = nullptr;
    }
    if (m_pFaceLandmark)
    {
        delete m_pFaceLandmark;
        m_pFaceLandmark = nullptr;
    }
    return true;
}

/* 处理数据 */
bool FaceLandmark_NS::CFaceLandmarkV1_0::process(
    InData_S stInData,
    std::vector<OutData_S> &stOutData)
{
    stOutData.clear();

    std::vector<float> vOutData;
    vOutData.clear();

    if (stInData.inMat.empty())
    {
        printf("传入图片为空\n");
        return false;
    }

    if (!m_pFaceLandmark || !m_pFaceDetect)
    {
        printf("未初始化算法类\n");
        return false;
    }

    bool bRet = true;
    /* 设置参数 */
    bRet = m_pFaceDetect->setParam(stInData.stParam.fBoxThreshold, stInData.stParam.fNmsThreshold);

    if (m_stInParam.bDebug)
    {
        /* 保存图片 */
        if (!stInData.inMat.empty() && !m_stInParam.strAnalyzeDataPath.empty())
        {
            // if (!Modules_NS::saveImage(stInData.inMat, m_stInParam.strAnalyzeDataPath, stInData.nChnId, 0, stOutData.savedFileName))
            // {
            //     printf("Debug-保存图片失败[%s]\n", m_stInParam.strAnalyzeDataPath.c_str());
            // }
        }
    }

    /* 保留一份输入的原图 */
    cv::Mat aOriginImg = stInData.inMat.clone();
    /* 人脸检测前处理 */
    if (stInData.inMat.channels() != m_nDetectLimitChannel)
    {
        printf("模型需要的通道数和输入图片的通道数不一致 inMat[%d] != m_nLimitChannel[%d]\n",
               stInData.inMat.channels(),
               m_nDetectLimitChannel);
        return false;
    }
    if (stInData.inMat.cols != m_nDetectLimitWidth || stInData.inMat.rows != m_nDetectLimitHeight)
    {
        // cv::resize(stInData.inMat, stInData.inMat, cv::Size(m_nLimitWidth,m_nLimitHeight));
        resizeAndPadImage(stInData.inMat,
                          m_nDetectLimitWidth,
                          m_nDetectLimitHeight,
                          m_nDetectXOffset,
                          m_nDetectYOffset,
                          m_fDetectResizeScale,
                          stInData.inMat);
    }
    /* 推理+后处理 */
    bRet = m_pFaceDetect->inference(stInData.inMat, vOutData);
    if (!bRet)
    {
        printf("算法分析失败\n");
        return false;
    }
    if (vOutData.size() % nResultNum != 0)
    {
        printf("算法推理结果异常\n");
        return false;
    }

    int nImgW = aOriginImg.cols;
    int nImgH = aOriginImg.rows;

    /* 将坐标扩回原来的大小 */
    for (int nIndex = 0; nIndex < (int)vOutData.size() / nResultNum; nIndex++)
    {
        float fX1 = (vOutData[nIndex * nResultNum + 0] - m_nDetectXOffset) / m_fDetectResizeScale;
        float fY1 = (vOutData[nIndex * nResultNum + 1] - m_nDetectYOffset) / m_fDetectResizeScale;
        float fX2 = (vOutData[nIndex * nResultNum + 2] - m_nDetectXOffset) / m_fDetectResizeScale;
        float fY2 = (vOutData[nIndex * nResultNum + 3] - m_nDetectYOffset) / m_fDetectResizeScale;
        float fBoxConfidence = vOutData[nIndex * nResultNum + 4];

        /* 人脸扩充、裁剪、缩放 */
        float fW = fX2 - fX1;
        float fH = fY2 - fY1;
        if (fW <= 0 || fH <= 0)
        {
            continue;
        }

        fX1 -= fW * 0.2;
        fY1 -= fH * 0.1;
        fX2 += fW * 0.2;
        fY2 += fH * 0.1;

        fX1 = std::max(0.0f, std::min(nImgW * 1.0f, fX1));
        fY1 = std::max(0.0f, std::min(nImgH * 1.0f, fY1));
        fX2 = std::max(0.0f, std::min(nImgW * 1.0f, fX2));
        fY2 = std::max(0.0f, std::min(nImgH * 1.0f, fY2));
        cv::Mat aFace = aOriginImg.clone()(cv::Rect(fX1, fY1, fX2 - fX1, fY2 - fY1));
        int nPFw = fX2 - fX1;
        int nPFh = fY2 - fY1;
        resizeAndPadImage(
            aFace,
            m_nLandmarkLimitWidth,
            m_nLandmarkLimitHeight,
            m_nLandmarkXOffset,
            m_nLandmarkYOffset,
            m_fLandmarkResizeScale,
            aFace);
        cv::cvtColor(aFace, aFace, cv::COLOR_BGR2GRAY);
        /* 推理+后处理 */
        std::vector<float> vLandmarkOutData;
        bRet = m_pFaceLandmark->inference(aFace, vLandmarkOutData);
        if (!bRet)
        {
            printf("算法分析失败\n");
            return false;
        }
        if (vLandmarkOutData.size() < nLandmarkNum)
        {
            printf("算法推理结果异常\n");
            return false;
        }

        OutData_S stOneOutData;
        stOneOutData.fX1 = fX1;
        stOneOutData.fY1 = fY1;
        stOneOutData.fX2 = fX2;
        stOneOutData.fY2 = fY2;
        for (int nL = 0; nL < nLandmarkNum / 2; nL++)
        {
            float fPx = fX1 + (vLandmarkOutData[nL * 2 + 0] * m_nLandmarkLimitWidth - m_nLandmarkXOffset) / m_fLandmarkResizeScale;
            float fPY = fY1 + (vLandmarkOutData[nL * 2 + 1] * m_nLandmarkLimitHeight - m_nLandmarkYOffset) / m_fLandmarkResizeScale;

            std::vector<float> vvLandmark;
            vvLandmark.push_back(fPx);
            vvLandmark.push_back(fPY);
            stOneOutData.vvLandmarks.push_back(vvLandmark);
        }
        printf("\n");
        stOutData.push_back(stOneOutData);
    }

    return true;
}

/* 处理数据 */
bool FaceLandmark_NS::CFaceLandmarkV1_0::resizeAndPadImage(
    cv::Mat inputImage,
    int nLimitWidth,
    int nLimitHeight,
    int &nXOffset,
    int &nYOffset,
    float &fResizeScale,
    cv::Mat &outputImage)
{
    int imageWidth = inputImage.cols;
    int imageHeight = inputImage.rows;
    fResizeScale = static_cast<float>(nLimitWidth) / std::max(imageWidth, imageHeight);

    int newWidth = static_cast<int>(imageWidth * fResizeScale);
    int newHeight = static_cast<int>(imageHeight * fResizeScale);
    cv::Mat resizedImage;
    cv::resize(inputImage, resizedImage, cv::Size(newWidth, newHeight));

    cv::Mat output = cv::Mat::zeros(nLimitWidth, nLimitHeight, inputImage.type());
    output.setTo(cv::Scalar(127, 127, 127));

    nXOffset = static_cast<int>((nLimitWidth - newWidth) / 2);
    nYOffset = static_cast<int>((nLimitHeight - newHeight) / 2);

    resizedImage.copyTo(output(cv::Rect(nXOffset, nYOffset, newWidth, newHeight)));
    outputImage = output;
    return true;
}