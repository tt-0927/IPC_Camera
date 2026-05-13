/*
 * @FilePath     : LicensePlateCognitionV2_0.cpp
 * @Author       : 廖尔涛 liaoet@kfb.cn
 * @Date         : 2024-10-09 13:59:59
 * @LastEditors: lianghy lianghy@kfb.cn
 * @LastEditTime: 2026-02-06 17:02:27
 * @Description  : 人少场景
 */
#include "LicensePlateCognitionV1_0.hpp"

// #include "LicensePlateDetect.hpp"
// #include "LicensePlateRec.hpp"
#include "SaveImage.hpp"
#include <string>
#include <vector>

using namespace LicensePlateCognition_NS;

static const std::array<const char*, 76> gPlateToken = 
{
    "#", "京", "沪", "津", "渝", "冀", "晋", "蒙", "辽", "吉",
    "黑", "苏", "浙", "皖", "闽", "赣", "鲁", "豫", "鄂", "湘",
    "粤", "桂", "琼", "川", "贵", "云", "藏", "陕", "甘", "青",
    "宁", "新", "学", "警", "港", "澳", "挂", "使", "领", "民",
    "航", "危", "0", "1", "2", "3", "4", "5", "6", "7",
    "8", "9", "A", "B", "C", "D", "E", "F", "G", "H",
    "J", "K", "L", "M", "N", "P", "Q", "R", "S", "T",
    "U", "V", "W", "X", "Y", "Z"
};

static const std::array<const char*, 5> gPlateColor = 
{
    "黑色", "蓝色", "绿色", "白色", "黄色"
};

LicensePlateCognition_NS::CLicensePlateCognitionV1_0::CLicensePlateCognitionV1_0(InParam_S stInParam)
    : m_stInParam(stInParam)
{
}

LicensePlateCognition_NS::CLicensePlateCognitionV1_0::~CLicensePlateCognitionV1_0()
{
    unInit();
}

/* 初始化 */
bool LicensePlateCognition_NS::CLicensePlateCognitionV1_0::init()
{
    bool bRet = false;

    bRet = false;

    m_pLicensePlateDetect = new Inference_NS::CYoloUltralytics(m_stInParam.strModelPath1);
    if (m_pLicensePlateDetect)
    {
        if (m_pLicensePlateDetect->init())
        {
            bRet = m_pLicensePlateDetect->getSizeLimit(
                0,
                m_nLimitWidth,
                m_nLimitHeight,
                m_nLimitChannel);
        }
    }

    if (!bRet)
    {
        printf("模型初始化失败 [%s]\n",
                m_stInParam.strModelPath1.c_str());
        goto FAIL;
    }

    bRet = false;

    m_pLicensePlateRec = new Inference_NS::CLicensePlateRec(m_stInParam.strModelPath2);
    if (m_pLicensePlateRec)
    {
        if (m_pLicensePlateRec->init())
        {
            bRet = m_pLicensePlateRec->getSizeLimit(
                0,
                m_nRecLimitWidth,
                m_nRecLimitHeight,
                m_nRecLimitChannel);
        }
    }

    if (!bRet)
    {
        printf("模型初始化失败 [%s]\n",
                m_stInParam.strModelPath2.c_str());
        goto FAIL;
    }

    return bRet;

FAIL:

    unInit();

    return false;
}

/* 反初始化 */
bool LicensePlateCognition_NS::CLicensePlateCognitionV1_0::unInit()
{
    if (m_pLicensePlateDetect)
    {
        delete m_pLicensePlateDetect;
        m_pLicensePlateDetect = nullptr;
    }

    if (m_pLicensePlateRec)
    {
        delete m_pLicensePlateRec;
        m_pLicensePlateRec = nullptr;
    }
    
    return true;
}

/* 处理数据 */
bool LicensePlateCognition_NS::CLicensePlateCognitionV1_0::process(
    InData_S               stInData,
    std::vector<Result_S>& vecResult)
{
    vecResult.clear();

    if (stInData.inMat.empty())
    {
        printf("传入图片为空\n");
        return false;
    }

    if (!m_pLicensePlateDetect || !m_pLicensePlateRec)
    {
        printf("未初始化算法类\n");
        return false;
    }

    bool bRet = true;

    bRet = m_pLicensePlateDetect->setParam(stInData.stParam.fBoxThreshold, stInData.stParam.fNmsThreshold);
    if (!bRet)
    {
        printf("阈值参数设置错误，应该在0~1之间！！\n");
        return false;
    }

    std::vector<float> vecPos;
    vecPos.clear();

    /* 保存初始图片 */
    cv::Mat originalImage = stInData.inMat.clone();


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
        resizeAndPadImage(stInData.inMat, stInData.inMat);
    }

    Inference_NS::InputData_S stInputData;

    
    stInputData.pData = (float*)stInData.inMat.data;
    stInputData.nDataSize = static_cast<size_t>(stInData.inMat.total() * stInData.inMat.elemSize());
    stInputData.stBoxs.fConfidence = stInData.stParam.fBoxThreshold;
    stInputData.stBoxs.fNms = stInData.stParam.fNmsThreshold;

#if 0
    /* --------------------------读图-------------------------------- */
    cv::Mat img = cv::imread("/mnt/test.jpg", cv::IMREAD_COLOR);
    if (img.empty()) 
    {
        std::cerr << "imread failed\n";
        return -1;
    }

    std::cout << "width=" << img.cols << " height=" << img.rows << " channels=" << img.channels() << '\n';     // 3 通道

    cv::Mat dst;
    cv::resize(img, dst, cv::Size(640, 640));   // 缩放到 640×640

    stInputData.pData = (float*)dst.data;
    stInputData.nDataSize = static_cast<size_t>(dst.total() * dst.elemSize());
#endif

    std::vector<Inference_NS::BoxData_S> vBoxDatas;
    
    /* 推理+后处理 */
    bRet = m_pLicensePlateDetect->inference(stInputData, vBoxDatas);
    if (!bRet)
    {
        printf("车牌检测算法分析失败\n");
        return false;
    }

    for(unsigned int i = 0; i < vBoxDatas.size(); i++)
    {   
        // printf("===================>车牌识别  当前目标[%d] 类别class[%d]  置信度[%.2f] <===================\n",
        // i + 1,vBoxDatas[i].nLabel, vBoxDatas[i].fConfidence);
        // printf("[%s]:[%d] === [%f][%d] (%d, %d)(%d, %d)\n", __FILE__, __LINE__, vBoxDatas.at(i).fConfidence, vBoxDatas.at(i).nLabel, 
        // vBoxDatas.at(i).stBoxs.nX1, vBoxDatas.at(i).stBoxs.nY1, vBoxDatas.at(i).stBoxs.nX2, vBoxDatas.at(i).stBoxs.nY2);
    
        std::vector<cv::Point> rectPoints = {
                                        cv::Point(vBoxDatas[i].stBoxs.nX1,vBoxDatas[i].stBoxs.nY1), // 左上角
                                        cv::Point(vBoxDatas[i].stBoxs.nY2, vBoxDatas[i].stBoxs.nY1), // 右上角
                                        cv::Point(vBoxDatas[i].stBoxs.nY2, vBoxDatas[i].stBoxs.nX2), // 右下角
                                        cv::Point(vBoxDatas[i].stBoxs.nX1, vBoxDatas[i].stBoxs.nX2)  // 左下角
                                    };
        cv::polylines(stInData.inMat, 
                    rectPoints, 
                    true,
                    cv::Scalar(0, 255, 0), /* 边框颜色(绿色) */
                    2,
                    cv::LINE_AA);

        // Modules_NS::saveImage(stInData.inMat, "/mnt/chepai_test");

        cv::Mat licensePlateImage;

        float x1 = vBoxDatas[i].stBoxs.nX1;
        float y1 = vBoxDatas[i].stBoxs.nY1;
        float x2 = vBoxDatas[i].stBoxs.nX2;
        float y2 = vBoxDatas[i].stBoxs.nY2;

        int nClassId = vBoxDatas[i].nLabel;

        /* 车牌提取 */
        licensePlateExtraction(cv::Point(x1, y1), cv::Point(x2, y2), originalImage, licensePlateImage);

        /* 车牌类型判断 */
        if (nClassId == 1)
        {
            getSplitMerge(licensePlateImage, licensePlateImage);
        }
        cv::resize(licensePlateImage, licensePlateImage, cv::Size(m_nRecLimitWidth,m_nRecLimitHeight));

        Inference_NS::InputData_S stLicensePlateNumberInputData;
        std::vector<Inference_NS::ClsData_S> vClsDatas;

        stLicensePlateNumberInputData.pData = (float*)licensePlateImage.data;
        stLicensePlateNumberInputData.nDataSize = static_cast<size_t>(licensePlateImage.total() * licensePlateImage.elemSize());
        stLicensePlateNumberInputData.stBoxs.fConfidence = stInData.stParam.fBoxThreshold;
        stLicensePlateNumberInputData.stBoxs.fNms = stInData.stParam.fNmsThreshold;

        /* 车牌识别 */
        bRet = m_pLicensePlateRec->inference(stLicensePlateNumberInputData, vClsDatas);
        if (!bRet)
        {
            printf("车牌识别算法分析失败\n");
            return false;
        }

        int color_index = vClsDatas.at(0).stCls.nLabel;
        printf("车牌颜色 ===== %d %s 类别：%d, 置信度：%f\n", color_index, gPlateColor[color_index], vClsDatas.at(0).stCls.nLabel, vClsDatas.at(0).stCls.fConfidence);

        std::string strPlateNumber;
        for (const auto& c : vClsDatas[0].vCls) 
        {
            int idx = c.nLabel;
            if (idx == 0 || idx >= gPlateToken.size())
            {
                continue;
            } 

            std::string s = gPlateToken[idx];
            // printf("%d == %s\n", idx, s.c_str());
            strPlateNumber += s;
        }

        printf("车牌号 %s\n", strPlateNumber.c_str());

        Result_S stResult;

        stResult.fX = x1;
        stResult.fY = y1;
        stResult.fWidth = x2 - x1;
        stResult.fHeight = y2 - y1;
        stResult.licensePlateColor = gPlateColor[color_index];
        stResult.licensePlateType = nClassId;
        stResult.licensePlateNumber = strPlateNumber;

        vecResult.push_back(stResult);
    }

    if (m_stInParam.bDebug)
    {
        /* 保存分析后的图片 */
        if (!stInData.inMat.empty() && !m_stInParam.strAnalyzeDataPath.empty())
        {

            for (int nIndex = 0; nIndex < vecResult.size(); nIndex++)
            {
                // std::vector<float> vectlwh = vecStracks[nIndex].tlwh;
                // /* 框 */
                // cv::rectangle(
                //     stInData.inMat,
                //     cv::Rect(vectlwh[0], vectlwh[1], vectlwh[2], vectlwh[3]),
                //     cv::Scalar(0, 0, 255),
                //     4);

                if (!Modules_NS::saveImage(stInData.inMat, m_stInParam.strAnalyzeDataPath))
                {
                    printf("Debug-保存图片失败[%s]\n", m_stInParam.strAnalyzeDataPath.c_str());
                }
            }
        }
    }

    return true;
}

/* 处理数据 */
bool LicensePlateCognition_NS::CLicensePlateCognitionV1_0::resizeAndPadImage(cv::Mat inputImage, cv::Mat& outputImage)
{
    int imageWidth  = inputImage.cols;
    int imageHeight = inputImage.rows;
    m_fResizeScale  = static_cast<float>(m_nLimitWidth) / std::max(imageWidth, imageHeight);

    int newWidth  = static_cast<int>(imageWidth * m_fResizeScale);
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

/* 车牌提取 */
bool LicensePlateCognition_NS::CLicensePlateCognitionV1_0::licensePlateExtraction(
    cv::Point topLeft,
    cv::Point bottomRight,
    cv::Mat inImage,
    cv::Mat& outImage)
{
    cv::Rect rect(topLeft, bottomRight);
    outImage = inImage(rect);
    cv::cvtColor(outImage, outImage, cv::COLOR_BGR2RGB);
    return true;
}

/* 双层车牌变换 */
bool LicensePlateCognition_NS::CLicensePlateCognitionV1_0::getSplitMerge(
    cv::Mat inImage,
    cv::Mat& outImage)
{
    int h = inImage.rows;
    int w = inImage.cols;
    cv::Mat img_upper = inImage(cv::Range(0, 5 * h / 12), cv::Range(0, w)).clone();
    cv::Mat img_lower = inImage(cv::Range(h / 3, h), cv::Range(0, w)).clone();
    cv::resize(img_upper, img_upper, img_lower.size());
    cv::hconcat(img_upper, img_lower, outImage);
    return true;
}
