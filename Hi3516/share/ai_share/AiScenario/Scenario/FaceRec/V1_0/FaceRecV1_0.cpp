/*
 * @FilePath     : FaceRecV1_0.cpp
 * @Author       : lih lih@kfb.cn
 * @Date         : 2024-06-19 15:31:40
 * @LastEditors: 李辉 lihui@kfb.cn
 * @LastEditTime: 2024-10-31 16:30:14
 * @Description  :
 */

#include "FaceRecV1_0.hpp"

#include <numeric>

#include "dlog.h"
#include "FaceDetect.hpp"
#include "FaceFeature.hpp"
#include "JsonInterfase.h"


using namespace Scenario_NS;

/* 一组数据的大小 */
#define FACE_POS_DATA_GROUP_SIZE 4

Scenario_NS::CFaceRecV1_0::CFaceRecV1_0(AiScenario_NS::InParam_S stInParam)
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

Scenario_NS::CFaceRecV1_0::~CFaceRecV1_0()
{
    unInit();
}

/* 初始化 */
bool Scenario_NS::CFaceRecV1_0::init()
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

        m_pFaceFeatureInference = new InferenceV1_0_NS::CFaceFeature(
            m_stInParam.stNeedParam.vstrModelPath.at(1));
        if (m_pFaceFeatureInference)
        {
            if (m_pFaceFeatureInference->init())
            {
                bRet = m_pFaceFeatureInference->getSizeLimit(
                    0,
                    m_nFaceFeatureLimitWidth,
                    m_nFaceFeatureLimitHeight,
                    m_nFaceFeatureLimitChannel);

                if (m_nFaceFeatureLimitWidth <= 0 || m_nFaceFeatureLimitHeight <= 0)
                {
                    dlog(LOG_ERROR, "模型的目标尺寸[%d x %d]，获取异常",
                         m_nFaceFeatureLimitWidth,
                         m_nFaceFeatureLimitHeight);
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
bool Scenario_NS::CFaceRecV1_0::unInit()
{
    if (m_pFaceDetectInference)
    {
        delete m_pFaceDetectInference;
        m_pFaceDetectInference = nullptr;
        return true;
    }

    if (m_pFaceFeatureInference)
    {
        delete m_pFaceFeatureInference;
        m_pFaceFeatureInference = nullptr;
        return true;
    }
    return false;
}

/* 人脸矫正 */
void Scenario_NS::CFaceRecV1_0::faceAlignment(cv::Mat& aFace, float fX, float fY)
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
bool Scenario_NS::CFaceRecV1_0::process(
    AiScenario_NS::CVData_S stInData,
    char*&                  pchOutData,
    int&                    nDataSize)
{
    if (stInData.inMat.empty())
    {
        dlog(LOG_ERROR, "传入图片为空");
        return false;
    }

    if (!m_pFaceDetectInference || !m_pFaceFeatureInference)
    {
        dlog(LOG_ERROR, "未初始化算法类");
        return false;
    }

    bool               bRet = false;
    std::vector<float> vecFaceDetectPos;
    vecFaceDetectPos.clear();

    /* 人脸特征提取图片 */
    float                   aFacenet128Value[128];
    AiScenario_NS::CVData_S stFaceFeatureData;

    /* 框图数据及特征数据 */
    std::vector<float> vecBoxPos;
    vecBoxPos.clear();
    std::vector<float> vecFeaturePos;
    vecFeaturePos.clear();

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

    /* 人脸检测前处理 */
    if (stInData.inMat.channels() != m_nFaceDetectLimitChannel)
    {
        dlog(LOG_ERROR, "人脸检测模型需要的通道数和输入图片的通道数不一致 inMat[%d] != m_nFaceDetectLimitChannel[%d]", stInData.inMat.channels(), m_nFaceDetectLimitChannel);
        return false;
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
    bRet = m_pFaceDetectInference->inference(stInData, vecFaceDetectPos);
    if (!bRet)
    {
        dlog(LOG_ERROR, "算法分析失败");
        return false;
    }


    for (int i = 0; i < (vecFaceDetectPos.size() / 14); i++)
    {
        if (vecFaceDetectPos[i * 14 + 0] < 0)
        {
            vecFaceDetectPos[i * 14 + 0] = 0;
        }
        if (vecFaceDetectPos[i * 14 + 1] < 0)
        {
            vecFaceDetectPos[i * 14 + 1] = 0;
        }
    }
    /* 原图到模型输入缩放的倍数 */
    float fWMultiple = aImage.cols * 1.0 / stInData.inMat.cols;
    float fHMultiple = aImage.rows * 1.0 / stInData.inMat.rows;
    for (int i = 0; i < (vecFaceDetectPos.size() / 14); i++)
    {
        /* 人脸检测的结果，恢复到原来图片的大小 */
        vecFaceDetectPos[i * 14 + 0] *= fWMultiple;
        vecFaceDetectPos[i * 14 + 1] *= fHMultiple;
        vecFaceDetectPos[i * 14 + 2] *= fWMultiple;
        vecFaceDetectPos[i * 14 + 3] *= fHMultiple;

        vecFaceDetectPos[i * 14 + 5] *= fWMultiple;
        vecFaceDetectPos[i * 14 + 6] *= fHMultiple;

        vecFaceDetectPos[i * 14 + 7] *= fWMultiple;
        vecFaceDetectPos[i * 14 + 8] *= fHMultiple;

        vecFaceDetectPos[i * 14 + 9]  *= fWMultiple;
        vecFaceDetectPos[i * 14 + 10] *= fHMultiple;

        vecFaceDetectPos[i * 14 + 12] *= fWMultiple;
        vecFaceDetectPos[i * 14 + 13] *= fHMultiple;
        /* 截取人脸 */
        cv::Point topLeft(vecFaceDetectPos[i * 14 + 0], vecFaceDetectPos[i * 14 + 1]);
        cv::Point bottomRight(vecFaceDetectPos[i * 14 + 2], vecFaceDetectPos[i * 14 + 3]);
        stFaceFeatureData.inMat = aImage(cv::Rect(topLeft, bottomRight)).clone();
        if (stFaceFeatureData.inMat.empty())
        {
            return false;
        }
        try
        {
            cv::resize(stFaceFeatureData.inMat, stFaceFeatureData.inMat, cv::Size(160, 160));
        }
        catch (const cv::Exception& e)
        {
            dlog(LOG_ERROR, "cv::resize 失败: %s", e.what());
            return false;
        }
        /* 人脸矫正 */
        faceAlignment(stFaceFeatureData.inMat, vecFaceDetectPos[i * 14 + 4] - vecFaceDetectPos[i * 14 + 6], vecFaceDetectPos[i * 14 + 5] - vecFaceDetectPos[i * 14 + 7]);
        /* 人脸特征点提取 */
        m_pFaceFeatureInference->inference(stFaceFeatureData, vecFeaturePos);

        /* 定义一个存放结果的结构体 */
        for (int bindex = 0; bindex < FACE_POS_DATA_GROUP_SIZE; bindex++)
        {
            vecBoxPos.push_back(vecFaceDetectPos[i * 14 + bindex]);
        }
    }

    /* 后处理 */
    switch (m_stInParam.stNeedParam.enResultType)
    {
        case AiScenario_NS::JSON:
        {
            bRet = convertToJson(vecBoxPos, vecFeaturePos, &pchOutData, nDataSize);
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
bool Scenario_NS::CFaceRecV1_0::process(
    AiScenario_NS::CAData_S stInData,
    char*&                  pchOutData,
    int&                    nDataSize)
{
    dlog(LOG_ERROR, "该场景为视频场景, 调用处理数据失败");
    return false;
}

/* 释放处理结果 */
bool Scenario_NS::CFaceRecV1_0::releaseData(char*& pchOutData)
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

bool Scenario_NS::CFaceRecV1_0::convertToJson(
    std::vector<float> vecBoxPos,
    std::vector<float> vecFeaturePos,
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
        if (vecBoxPos.size() > ((nIndex * FACE_POS_DATA_GROUP_SIZE) + 3))
        {
            auto pArrayBox = Json::Array::init();
            auto pItem     = Json::init();
            Json::Array::add(pArrayBox, (int)vecBoxPos[nIndex * FACE_POS_DATA_GROUP_SIZE]);
            Json::Array::add(pArrayBox, (int)vecBoxPos[(nIndex * FACE_POS_DATA_GROUP_SIZE) + 1]);
            Json::Array::add(pArrayBox, (int)vecBoxPos[(nIndex * FACE_POS_DATA_GROUP_SIZE) + 2]);
            Json::Array::add(pArrayBox, (int)vecBoxPos[(nIndex * FACE_POS_DATA_GROUP_SIZE) + 3]);
            Json::add(pItem, "Box", pArrayBox);
            Json::Array::add(pArrayFaceJson, pItem);
        }
    }

    /* 人脸特征array */
    for (int nIndex = 0; nIndex < vecFeaturePos.size(); nIndex++)
    {
        Json::Array::add(pArrayFea, vecFeaturePos[nIndex]);
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
