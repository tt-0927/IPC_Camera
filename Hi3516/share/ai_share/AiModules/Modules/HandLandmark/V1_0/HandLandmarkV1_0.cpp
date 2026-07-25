/**
 * @file HandLandmarkV1_0.cpp
 * @author wucp (wucp@kfb.cn)
 * @date 2025-05-21
 *
 * @brief
 */
#include "HandLandmarkV1_0.hpp"

#include <algorithm>

#include "SaveImage.hpp"

HandLandmark_NS::CHandLandmarkV1_0::CHandLandmarkV1_0(InParam_S stInParam)
    : m_stInParam(stInParam)
{
}

HandLandmark_NS::CHandLandmarkV1_0::~CHandLandmarkV1_0()
{
    unInit();
}

/* 初始化 */
bool HandLandmark_NS::CHandLandmarkV1_0::init()
{
    bool bRet = false;

    /* 人脸检测模型初始化 */
    m_pHandDetect = new Inference_NS::CYolov5(m_stInParam.strDetectModelPath);
    if (m_pHandDetect)
    {
        if (m_pHandDetect->init())
        {
            bRet = m_pHandDetect->getSizeLimit(
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

    bRet            = false;
    /* 人脸特征提取模型初始化 */
    m_pHandLandmark = new Inference_NS::CHandLandmark(m_stInParam.strLandmarkModelPath);
    if (m_pHandLandmark)
    {
        if (m_pHandLandmark->init())
        {
            bRet = m_pHandLandmark->getSizeLimit(
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
bool HandLandmark_NS::CHandLandmarkV1_0::unInit()
{
    if (m_pHandDetect)
    {
        delete m_pHandDetect;
        m_pHandDetect = nullptr;
    }
    if (m_pHandLandmark)
    {
        delete m_pHandLandmark;
        m_pHandLandmark = nullptr;
    }
    return true;
}

/* 处理数据 */
bool HandLandmark_NS::CHandLandmarkV1_0::process(
    InData_S                stInData,
    std::vector<OutData_S>& stOutData)
{
    stOutData.clear();

    std::vector<float> vOutData;
    vOutData.clear();

    if (stInData.inMat.empty())
    {
        printf("传入图片为空\n");
        return false;
    }

    if (!m_pHandLandmark || !m_pHandDetect)
    {
        printf("未初始化算法类\n");
        return false;
    }

    bool bRet = true;
    /* 设置参数 */
    bRet      = m_pHandDetect->setParam(stInData.stParam.fBoxThreshold, stInData.stParam.fNmsThreshold);

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
        bRet = resizeAndPadImage(stInData.inMat,
                                 m_nDetectLimitWidth,
                                 m_nDetectLimitHeight,
                                 m_nDetectXOffset,
                                 m_nDetectYOffset,
                                 m_fDetectResizeScale,
                                 stInData.inMat);
        if (!bRet)
        {
            printf("算法分析失败\n");
            return false;
        }
    }
    /* 推理+后处理 */
    Inference_NS::InputData_S stInputData;
    stInputData.pData     = (float*)stInData.inMat.data;
    stInputData.nDataSize = static_cast<int>(stInData.inMat.total() * stInData.inMat.elemSize() * sizeof(float));
    std::vector<Inference_NS::BoxData_S> vBoxDatas;
    bRet = m_pHandDetect->inference(stInputData, vBoxDatas);
    if (!bRet)
    {
        printf("算法分析失败\n");
        return false;
    }

    int nImgW = aOriginImg.cols;
    int nImgH = aOriginImg.rows;

    /* 将坐标扩回原来的大小 */
    for (int nIndex = 0; nIndex < vBoxDatas.size(); nIndex++)
    {
        float fX1            = (vBoxDatas[nIndex].stBoxs.nX1 - m_nDetectXOffset) / m_fDetectResizeScale;
        float fY1            = (vBoxDatas[nIndex].stBoxs.nY1 - m_nDetectYOffset) / m_fDetectResizeScale;
        float fX2            = (vBoxDatas[nIndex].stBoxs.nX2 - m_nDetectXOffset) / m_fDetectResizeScale;
        float fY2            = (vBoxDatas[nIndex].stBoxs.nY2 - m_nDetectYOffset) / m_fDetectResizeScale;
        float fBoxConfidence = vBoxDatas[nIndex].fConfidence;

        /* 手部扩充、裁剪、缩放 */
        float fW = fX2 - fX1;
        float fH = fY2 - fY1;
        if (fW <= 0 || fH <= 0)
        {
            continue;
        }

        fX1 -= fW * 0.2;
        fY1 -= fH * 0.2;
        fX2 += fW * 0.2;
        fY2 += fH * 0.2;

        fX1           = std::max(0.0f, std::min(nImgW * 1.0f, fX1));
        fY1           = std::max(0.0f, std::min(nImgH * 1.0f, fY1));
        fX2           = std::max(0.0f, std::min(nImgW * 1.0f, fX2));
        fY2           = std::max(0.0f, std::min(nImgH * 1.0f, fY2));
        cv::Mat aHand = aOriginImg.clone()(cv::Rect(fX1, fY1, fX2 - fX1, fY2 - fY1));
#if 0
        // 1. 获取当前时间戳（秒级）
        std::time_t        t = std::time(nullptr);
        std::ostringstream oss;
        oss << t;    // 例如 "1724736183"

        // 2. 拼接文件名
        std::string filename = "aHand_" + oss.str() + ".png";

        // 3. 保存
        if (cv::imwrite(filename, aHand))
        {
            std::cout << "保存成功: " << filename << std::endl;
        }
        else
        {
            std::cerr << "保存失败！" << std::endl;
        }
#endif
        int nPFw = fX2 - fX1;
        int nPFh = fY2 - fY1;
        bRet     = resizeAndPadImage(
            aHand,
            m_nLandmarkLimitWidth,
            m_nLandmarkLimitHeight,
            m_nLandmarkXOffset,
            m_nLandmarkYOffset,
            m_fLandmarkResizeScale,
            aHand);
        if (!bRet)
        {
            printf("算法分析失败\n");
            return false;
        }
        /* 推理+后处理 */
        Inference_NS::InputData_S stPoseInputData;
        stPoseInputData.pData     = (float*)aHand.data;
        stPoseInputData.nDataSize = static_cast<int>(aHand.total() * aHand.elemSize()) * sizeof(float);
        std::vector<Inference_NS::PointData_S> vPointDatas;
        bRet = m_pHandLandmark->inference(stPoseInputData, vPointDatas);
        if (!bRet)
        {
            printf("算法分析失败\n");
            return false;
        }

        OutData_S stOneOutData;
        stOneOutData.fX1         = fX1;
        stOneOutData.fY1         = fY1;
        stOneOutData.fX2         = fX2;
        stOneOutData.fY2         = fY2;
        stOneOutData.nLabel      = vPointDatas[0].nLabel;
        stOneOutData.fConfidence = vPointDatas[0].fConfidence;
        for (int nL = 0; nL < vPointDatas[0].vPoints.size(); nL++)
        {
            float fPx = fX1 + (vPointDatas[0].vPoints[nL].nX - m_nLandmarkXOffset) / m_fLandmarkResizeScale;
            float fPY = fY1 + (vPointDatas[0].vPoints[nL].nY - m_nLandmarkYOffset) / m_fLandmarkResizeScale;

            std::vector<float> vvLandmark;
            vvLandmark.push_back(fPx);
            vvLandmark.push_back(fPY);
            stOneOutData.vvLandmarks.push_back(vvLandmark);
        }

        stOutData.push_back(stOneOutData);
    }

    return true;
}

/* 处理数据 */
bool HandLandmark_NS::CHandLandmarkV1_0::resizeAndPadImage(
    cv::Mat  inputImage,
    int      nLimitWidth,
    int      nLimitHeight,
    int&     nXOffset,
    int&     nYOffset,
    float&   fResizeScale,
    cv::Mat& outputImage)
{
    if (inputImage.empty())
    {
        std::cerr << "输入图像为空，无法resize" << std::endl;
        return false;
    }

    if (nLimitWidth <= 0 ||
        nLimitHeight <= 0)
    {
        std::cerr << "模型初始化失败" << std::endl;
        return false;
    }

    int imageWidth  = inputImage.cols;
    int imageHeight = inputImage.rows;

    if (imageWidth <= 0 ||
        imageHeight <= 0)
    {
        std::cerr << "输入图像, 宽高异常" << std::endl;
        return false;
    }

    fResizeScale = static_cast<float>(nLimitWidth) / std::max(imageWidth, imageHeight);

    int     newWidth  = int(imageWidth * fResizeScale);
    int     newHeight = int(imageHeight * fResizeScale);
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