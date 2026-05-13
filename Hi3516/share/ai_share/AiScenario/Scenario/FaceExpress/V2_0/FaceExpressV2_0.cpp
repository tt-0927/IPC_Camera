/*
 * @FilePath     : FaceExpressV2_0.cpp
 * @Author       : 吴才朋 wucp@kfb.cn
 * @Date         : 2024-08-01 16:29:52
 * @LastEditors: 李辉 lihui@kfb.cn
 * @LastEditTime: 2024-10-31 16:30:02
 * @Description  : 表情识别
 */

#include "FaceExpressV2_0.hpp"

#include <algorithm>
#include <numeric>

#include "dlog.h"
#include "Expression.hpp"
#include "FaceDetect.hpp"
#include "JsonInterfase.h"


using namespace Scenario_NS;

/* 一组数据的大小 */
#define DATA_GROUP_SIZE 14

Scenario_NS::CFaceExpressV2_0::CFaceExpressV2_0(AiScenario_NS::InParam_S stInParam)
    : CScenarioBase(stInParam)
{
    if (stInParam.stExParam.fBoxThreshold != 0.0f)
    {
        m_fBoxThreshold = stInParam.stExParam.fBoxThreshold;
    }

    if (stInParam.stExParam.fNmsThreshold != 0.0f)
    {
        m_fNmsThreshold = stInParam.stExParam.fNmsThreshold;
    }
}

Scenario_NS::CFaceExpressV2_0::~CFaceExpressV2_0()
{
    unInit();
}

/* 初始化 */
bool Scenario_NS::CFaceExpressV2_0::init()
{
    bool        bRet = false;
    std::string strModel;
    if (m_stInParam.stNeedParam.vstrModelPath.size() > 1)
    {
        bRet = false;

        m_pFaceDetectInference = new InferenceV1_0_NS::CFaceDetect(
            m_stInParam.stNeedParam.vstrModelPath.at(0));
        if (m_pFaceDetectInference)
        {
            if (m_pFaceDetectInference->init())
            {
                bRet = m_pFaceDetectInference->getSizeLimit(
                    0,
                    m_nFaceDetectLimitWidth,
                    m_nFaceDetectLimitHeight,
                    m_nFaceDetectLimitChannel);

                if (m_nFaceDetectLimitWidth <= 0 || m_nFaceDetectLimitHeight <= 0)
                {
                    dlog(LOG_ERROR, "模型的目标尺寸[%d x %d]，获取异常",
                         m_nFaceDetectLimitWidth,
                         m_nFaceDetectLimitHeight);
                    bRet = false;
                }
            }
        }

        if (!bRet)
        {
            dlog(LOG_ERROR, "模型初始化失败 [%s]",
                 m_stInParam.stNeedParam.vstrModelPath.at(0).c_str());
            goto FAIL;
        }

        bRet = false;

        m_pFaceExpressInference = new InferenceV1_0_NS::CExpression(
            m_stInParam.stNeedParam.vstrModelPath.at(1));
        if (m_pFaceExpressInference)
        {
            if (m_pFaceExpressInference->init())
            {
                bRet = m_pFaceExpressInference->getSizeLimit(
                    0,
                    m_nFaceExpressLimitWidth,
                    m_nFaceExpressLimitHeight,
                    m_nFaceExpressLimitChannel);

                if (m_nFaceExpressLimitHeight <= 0 || m_nFaceExpressLimitWidth <= 0)
                {
                    dlog(LOG_ERROR, "模型的目标尺寸[%d x %d]，获取异常",
                         m_nFaceExpressLimitWidth,
                         m_nFaceExpressLimitHeight);
                    bRet = false;
                }
            }
        }

        if (!bRet)
        {
            dlog(LOG_ERROR, "模型初始化失败 [%s]",
                 m_stInParam.stNeedParam.vstrModelPath.at(1).c_str());
            goto FAIL;
        }
    }

    return bRet;

FAIL:

    unInit();

    return false;
}

/* 反初始化 */
bool Scenario_NS::CFaceExpressV2_0::unInit()
{
    if (m_pFaceDetectInference)
    {
        delete m_pFaceDetectInference;
        m_pFaceDetectInference = nullptr;
        return true;
    }

    if (m_pFaceExpressInference)
    {
        delete m_pFaceExpressInference;
        m_pFaceExpressInference = nullptr;
        return true;
    }
    return false;
}

/* 人脸矫正 */
void Scenario_NS::CFaceExpressV2_0::faceAlignment(cv::Mat& aFace, float fX, float fY)
{
    // Calculate the angle of the eye line with respect to the horizontal line
    float fAngle;
    if (fX == 0)
    {
        fAngle = 0;
    }
    else
    {
        fAngle = std::atan(fY / fX) * 180 / M_PI;
    }

    cv::Point2f aCenter(aFace.cols / 2.0f, aFace.rows / 2.0f);
    cv::Mat     aRotationMatrix = cv::getRotationMatrix2D(aCenter, fAngle, 1.0);

    // Apply affine transformation to the image
    cv::warpAffine(aFace, aFace, aRotationMatrix, aFace.size());
}

/* 处理数据 */
bool Scenario_NS::CFaceExpressV2_0::process(
    AiScenario_NS::CVData_S stInData,
    char*&                  pchOutData,
    int&                    nDataSize)
{
    if (stInData.inMat.empty())
    {
        dlog(LOG_ERROR, "传入图片为空");
        return false;
    }

    if (!m_pFaceDetectInference || !m_pFaceExpressInference)
    {
        dlog(LOG_ERROR, "未初始化算法类");
        return false;
    }
    bool bRet = false;


    /* 框图数据及特征数据 */
    std::vector<float> vecBoxPos;
    vecBoxPos.clear();
    /* 人脸表情识别 */
    AiScenario_NS::CVData_S stFaceExpressData;
    std::vector<float>      vecExpressPos;
    vecExpressPos.clear();

    if (m_stInParam.stExParam.bDebug)
    {
        /* 保存图片 */
        if (!stInData.inMat.empty() && !m_stInParam.stExParam.strOriginalDataPath.empty())
        {
            if (!saveImage(stInData.inMat, m_stInParam.stExParam.strOriginalDataPath))
            {
                dlog(LOG_ERROR, "Debug-保存图片失败[%s]", m_stInParam.stExParam.strOriginalDataPath.c_str());
            }
        }
    }

    /* YOLO人脸检测前处理 */
    if (stInData.inMat.channels() != m_nFaceDetectLimitChannel)
    {
        dlog(LOG_ERROR, "人脸检测模型需要的通道数和输入图片的通道数不一致 inMat[%d] != m_nFaceDetectLimitChannel[%d]", stInData.inMat.channels(), m_nFaceDetectLimitChannel);
        return false;
    }

    /* 拷贝一份用于绘制 */
    cv::Mat imageShow;
    if (m_stInParam.stExParam.bDebug)
    {
        imageShow = stInData.inMat.clone();
    }

    cv::Mat aImage = stInData.inMat.clone();
    if (stInData.inMat.cols != m_nFaceDetectLimitWidth || stInData.inMat.rows != m_nFaceDetectLimitHeight)
    {
        try
        {
            cv::resize(stInData.inMat, stInData.inMat, cv::Size(m_nFaceDetectLimitWidth, m_nFaceDetectLimitHeight));
        }
        catch (const cv::Exception& e)
        {
            dlog(LOG_ERROR, "cv::resize 失败: %s", e.what());
            return false;
        }
    }

    /* 人脸检测 */
    bRet = m_pFaceDetectInference->inference(stInData, vecBoxPos);
    if (!bRet)
    {
        dlog(LOG_ERROR, "算法分析失败");
        return false;
    }

    /* 原图到模型输入缩放的倍数 */
    float fWMultiple = aImage.cols * 1.0 / stInData.inMat.cols;
    float fHMultiple = aImage.rows * 1.0 / stInData.inMat.rows;

    for (int nBoxNum = 0; nBoxNum < vecBoxPos.size() / DATA_GROUP_SIZE; nBoxNum++)
    {
        if (vecBoxPos[DATA_GROUP_SIZE * nBoxNum + 0] < 0)
        {
            vecBoxPos[DATA_GROUP_SIZE * nBoxNum + 0] = 0;
        }
        if (vecBoxPos[DATA_GROUP_SIZE * nBoxNum + 1] < 0)
        {
            vecBoxPos[DATA_GROUP_SIZE * nBoxNum + 1] = 0;
        }
        /* 人脸检测的结果，恢复到原来图片的大小 */
        vecBoxPos[DATA_GROUP_SIZE * nBoxNum + 0] *= fWMultiple;
        vecBoxPos[DATA_GROUP_SIZE * nBoxNum + 1] *= fHMultiple;
        vecBoxPos[DATA_GROUP_SIZE * nBoxNum + 2] *= fWMultiple;
        vecBoxPos[DATA_GROUP_SIZE * nBoxNum + 3] *= fHMultiple;

        vecBoxPos[DATA_GROUP_SIZE * nBoxNum + 5]  *= fWMultiple;
        vecBoxPos[DATA_GROUP_SIZE * nBoxNum + 6]  *= fHMultiple;
        vecBoxPos[DATA_GROUP_SIZE * nBoxNum + 7]  *= fWMultiple;
        vecBoxPos[DATA_GROUP_SIZE * nBoxNum + 8]  *= fHMultiple;
        vecBoxPos[DATA_GROUP_SIZE * nBoxNum + 9]  *= fWMultiple;
        vecBoxPos[DATA_GROUP_SIZE * nBoxNum + 10] *= fHMultiple;
        vecBoxPos[DATA_GROUP_SIZE * nBoxNum + 12] *= fWMultiple;
        vecBoxPos[DATA_GROUP_SIZE * nBoxNum + 13] *= fHMultiple;

        /* 人脸裁剪 */
        cv::Rect cropRect(cv::Point(int(vecBoxPos[DATA_GROUP_SIZE * nBoxNum + 0]), int(vecBoxPos[DATA_GROUP_SIZE * nBoxNum + 1])), cv::Point(int(vecBoxPos[DATA_GROUP_SIZE * nBoxNum + 2]), int(vecBoxPos[DATA_GROUP_SIZE * nBoxNum + 3])));
        stFaceExpressData.inMat = aImage(cropRect).clone();
        /* 人脸矫正 */
        faceAlignment(stFaceExpressData.inMat, vecBoxPos[DATA_GROUP_SIZE * nBoxNum + 4] - vecBoxPos[DATA_GROUP_SIZE * nBoxNum + 6], vecBoxPos[DATA_GROUP_SIZE * nBoxNum + 5] - vecBoxPos[DATA_GROUP_SIZE * nBoxNum + 7]);
        /* 人脸表情识别前处理 */
        if (stFaceExpressData.inMat.channels() != m_nFaceExpressLimitChannel)
        {
            dlog(LOG_ERROR, "人脸表情识别模型需要的通道数和输入图片的通道数不一致 inMat[%d] != m_nFaceDetectLimitChannel[%d]", stFaceExpressData.inMat.channels(), m_nFaceExpressLimitChannel);
            return false;
        }
        if (stFaceExpressData.inMat.cols != m_nFaceExpressLimitWidth || stFaceExpressData.inMat.rows != m_nFaceExpressLimitHeight)
        {
            try
            {
                cv::resize(stFaceExpressData.inMat, stFaceExpressData.inMat, cv::Size(m_nFaceExpressLimitWidth, m_nFaceExpressLimitHeight));
            }
            catch (const cv::Exception& e)
            {
                dlog(LOG_ERROR, "cv::resize 失败: %s", e.what());
                return false;
            }
        }
        /* 人脸表情识别推理 */
        bRet = m_pFaceExpressInference->inference(stFaceExpressData, vecExpressPos);
        if (!bRet)
        {
            dlog(LOG_ERROR, "算法分析失败");
            return false;
        }
        /* 后处理 - 获取概率最大的表情类别 */
        size_t nMaxIndex = -1;
        auto   maxIt     = std::max_element(vecExpressPos.begin(), vecExpressPos.end());
        if (maxIt != vecExpressPos.end())
        {
            /* 计算最大元素的下标（索引） */
            nMaxIndex = std::distance(vecExpressPos.begin(), maxIt);
            std::cout << "最大值的下标是：" << nMaxIndex << std::endl;
        }
        else
        {
            std::cout << "容器为空或未找到最大元素" << std::endl;
        }

        if (m_stInParam.stExParam.bDebug)
        {
            /* 绘制人脸 */
            cv::rectangle(imageShow, cv::Point(vecBoxPos[DATA_GROUP_SIZE * nBoxNum + 0], vecBoxPos[DATA_GROUP_SIZE * nBoxNum + 1]), cv::Point(vecBoxPos[DATA_GROUP_SIZE * nBoxNum + 2], vecBoxPos[DATA_GROUP_SIZE * nBoxNum + 3]), cv::Scalar(255, 0, 0), 2);
            /* 将文本绘制到图像上 */
            cv::putText(imageShow, std::to_string(nMaxIndex), cv::Point(vecBoxPos[DATA_GROUP_SIZE * nBoxNum + 0], vecBoxPos[DATA_GROUP_SIZE * nBoxNum + 1] - 10), cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(255, 0, 255), 1);
        }
    }

    if (m_stInParam.stExParam.bDebug)
    {
        /* 保存图片 */
        if (!imageShow.empty() && !m_stInParam.stExParam.strAnalyzeDataPath.empty())
        {
            if (!saveImage(imageShow, m_stInParam.stExParam.strAnalyzeDataPath))
            {
                dlog(LOG_ERROR, "Debug-保存图片失败[%s]", m_stInParam.stExParam.strAnalyzeDataPath.c_str());
            }
        }
    }

    /* 数据处理 */
    switch (m_stInParam.stNeedParam.enResultType)
    {
        case AiScenario_NS::JSON:
        {
            bRet = convertToJson(vecBoxPos, vecExpressPos, &pchOutData, nDataSize);
            break;
        }
        case AiScenario_NS::XML:
        {
            bRet = false;
            dlog(LOG_ERROR, "该算法场景为实现 XML格式返回");
            break;
        }
        case AiScenario_NS::PIC:
        {
            bRet = false;
            dlog(LOG_ERROR, "该算法场景为实现 PIC格式返回");
            break;
        }
        default:
        {
            bRet = false;
            dlog(LOG_ERROR, "该算法场景为实现 [%d]格式返回", m_stInParam.stNeedParam.enResultType);

            break;
        }
    }

    return bRet;
}

/* 处理数据 */
bool Scenario_NS::CFaceExpressV2_0::process(
    AiScenario_NS::CAData_S stInData,
    char*&                  pchOutData,
    int&                    nDataSize)
{
    dlog(LOG_ERROR, "该场景为视频场景, 调用处理数据失败");
    return false;
}

/* 释放处理结果 */
bool Scenario_NS::CFaceExpressV2_0::releaseData(char*& pchOutData)
{
    if (!pchOutData)
    {
        return false;
    }

    int bRet = true;

    switch (m_stInParam.stNeedParam.enResultType)
    {
        case AiScenario_NS::JSON:
        {
            Json::release(pchOutData);
            break;
        }
        case AiScenario_NS::XML:
        {
            bRet = false;
            dlog(LOG_ERROR, "该算法场景为实现 XML格式返回");
            break;
        }
        case AiScenario_NS::PIC:
        {
            bRet = false;
            dlog(LOG_ERROR, "该算法场景为实现 PIC格式返回");
            break;
        }
        default:
        {
            bRet = false;
            dlog(LOG_ERROR, "该算法场景为实现 [%d]格式返回", m_stInParam.stNeedParam.enResultType);

            break;
        }
    }

    return bRet;
}

bool Scenario_NS::CFaceExpressV2_0::convertToJson(
    std::vector<float> vecBoxPos,
    std::vector<float> vecExpressPos,
    char**             pchOutData,
    int&               nDataSize)
{
    auto pRootJson      = Json::init();
    auto pDataJson      = Json::init();
    auto pArrayFaceJson = Json::Array::init();
    auto pArrayFea      = Json::Array::init();

    /* 人脸BOX */
    for (int nIndex = 0; nIndex < vecBoxPos.size(); nIndex++)
    {
        if (vecBoxPos.size() > ((nIndex * DATA_GROUP_SIZE) + 3))
        {
            auto pArrayBox = Json::Array::init();
            auto pItem     = Json::init();
            Json::Array::add(pArrayBox, (int)vecBoxPos[nIndex * DATA_GROUP_SIZE]);
            Json::Array::add(pArrayBox, (int)vecBoxPos[(nIndex * DATA_GROUP_SIZE) + 1]);
            Json::Array::add(pArrayBox, (int)vecBoxPos[(nIndex * DATA_GROUP_SIZE) + 2]);
            Json::Array::add(pArrayBox, (int)vecBoxPos[(nIndex * DATA_GROUP_SIZE) + 3]);
            Json::add(pItem, "Box", pArrayBox);
            Json::Array::add(pArrayFaceJson, pItem);
        }
    }

    /* 人脸特征array */
    for (int nIndex = 0; nIndex < vecExpressPos.size(); nIndex++)
    {
        Json::Array::add(pArrayFea, vecExpressPos[nIndex]);
    }

    Json::add(pDataJson, "Face", pArrayFaceJson);
    Json::add(pDataJson, "Feature", pArrayFea);
    Json::add(pRootJson, "BaseData", pDataJson);

    *pchOutData = Json::print(pRootJson);

    nDataSize = strlen(Json::to_string(pRootJson).c_str());

    if (pRootJson)
    {
        /* 释放数据 */
        Json::deinit(pRootJson);
    }

    return true;
}
