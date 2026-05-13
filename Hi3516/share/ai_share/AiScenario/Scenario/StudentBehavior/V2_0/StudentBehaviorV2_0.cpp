/*
 * @FilePath     : StudentBehaviorV2_0.cpp
 * @Author       : 吴才朋 wucp@kfb.cn
 * @Date         : 2024-08-01 16:29:52
 * @LastEditors: 李辉 lihui@kfb.cn
 * @LastEditTime: 2024-10-31 16:38:50
 * @Description  : 基于骨骼点检测的行为分析算法应用逻辑
 */

#include "StudentBehaviorV2_0.hpp"

#include <chrono>

#include "BehaviorProcess.hpp"
#include "dlog.h"
#include "Fastpose.hpp"
#include "HeadDetect.hpp"
#include "JsonInterfase.h"


/* 一组数据的大小 */
#define DATA_BOX_GROUP_SIZE 6
#define DATA_KEY_GROUP_SIZE 3
#define DATA_KEY_SIZE       26

using namespace Scenario_NS;

Scenario_NS::CStudentBehaviorV2_0::CStudentBehaviorV2_0(AiScenario_NS::InParam_S stInParam)
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

Scenario_NS::CStudentBehaviorV2_0::~CStudentBehaviorV2_0()
{
    unInit();
}

/* 初始化 */
bool Scenario_NS::CStudentBehaviorV2_0::init()
{
    bool bRet = false;

    if (m_stInParam.stNeedParam.vstrModelPath.size() > 1)
    {
        bRet = false;

        /* 人头检测算法 */
        m_pHInference = new InferenceV1_0_NS::CHeadDetect(
            m_stInParam.stNeedParam.vstrModelPath.at(0));
        if (m_pHInference)
        {
            if (m_pHInference->init())
            {
                bRet = m_pHInference->getSizeLimit(
                    0,
                    m_nHeadDetectLimitWidth,
                    m_nHeadDetectLimitHeight,
                    m_nHeadDetectLimitChannel);

                if (m_nHeadDetectLimitWidth <= 0 || m_nHeadDetectLimitHeight <= 0)
                {
                    dlog(LOG_ERROR, "模型的目标尺寸[%d x %d]，获取异常",
                         m_nHeadDetectLimitWidth,
                         m_nHeadDetectLimitHeight);
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

        /* 人体关键点提取算法 */
        m_pBInference = new InferenceV1_0_NS::CFastpose(
            m_stInParam.stNeedParam.vstrModelPath.at(1));
        if (m_pBInference)
        {
            if (m_pBInference->init())
            {
                bRet = m_pBInference->getSizeLimit(
                    0,
                    m_nFastPoseLimitWidth,
                    m_nFastPoseLimitHeight,
                    m_nFastPosetLimitChannel);

                if (m_nFastPoseLimitWidth <= 0 || m_nFastPoseLimitHeight <= 0)
                {
                    dlog(LOG_ERROR, "模型的目标尺寸[%d x %d]，获取异常",
                         m_nFastPoseLimitWidth,
                         m_nFastPoseLimitHeight);
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
bool Scenario_NS::CStudentBehaviorV2_0::unInit()
{
    if (m_pHInference)
    {
        delete m_pHInference;
        m_pHInference = nullptr;
        return true;
    }
    if (m_pBInference)
    {
        delete m_pBInference;
        m_pBInference = nullptr;
        return true;
    }
    return false;
}

/* 处理数据 */
bool Scenario_NS::CStudentBehaviorV2_0::process(
    AiScenario_NS::CVData_S stInData,
    char*&                  pchOutData,
    int&                    nDataSize)
{
    if (stInData.inMat.empty())
    {
        dlog(LOG_ERROR, "传入图片为空");
        return false;
    }

    if (!m_pHInference)
    {
        dlog(LOG_ERROR, "未初始化人头检测算法类");
        return false;
    }
    if (!m_pBInference)
    {
        dlog(LOG_ERROR, "未初始化人体特征提取算法类");
        return false;
    }

    bool bRet = true;


    std::vector<int>                vBehavior;
    std::vector<float>              vecBoxes;
    std::vector<float>              vecPoses;
    std::vector<std::vector<float>> vecKeyPoses;

    vBehavior.clear();
    vecBoxes.clear();
    vecPoses.clear();
    vecKeyPoses.clear();

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

    /* 人头检测前处理 */
    if (stInData.inMat.channels() != m_nHeadDetectLimitChannel)
    {
        dlog(LOG_ERROR, "模型需要的通道数和输入图片的通道数不一致 inMat[%d] != m_nHeadDetectLimitChannel[%d]", stInData.inMat.channels(), m_nHeadDetectLimitChannel);
        return false;
    }

    /* 拷贝一份用于绘制 */
    cv::Mat imageShow;
    if (m_stInParam.stExParam.bDebug)
    {
        imageShow = stInData.inMat.clone();
    }

    cv::Mat aImage = stInData.inMat.clone();
    if (stInData.inMat.cols != m_nHeadDetectLimitWidth || stInData.inMat.rows != m_nHeadDetectLimitHeight)
    {
        try
        {
            cv::resize(stInData.inMat, stInData.inMat, cv::Size(m_nHeadDetectLimitWidth, m_nHeadDetectLimitHeight));
        }
        catch (const cv::Exception& e)
        {
            dlog(LOG_ERROR, "cv::resize 失败: %s", e.what());
            return false;
        }
    }
    /* 人头检测推理+后处理 */
    bRet = m_pHInference->inference(stInData, vecBoxes);
    if (!bRet)
    {
        dlog(LOG_ERROR, "算法分析失败");
        return false;
    }

    if ((vecBoxes.size() % DATA_BOX_GROUP_SIZE) != 0)
    {
        dlog(LOG_ERROR, "算法推理结果异常");
        return false;
    }


    /* 原图到模型输入缩放的倍数 */
    float fWMultiple = aImage.cols * 1.0 / stInData.inMat.cols;
    float fHMultiple = aImage.rows * 1.0 / stInData.inMat.rows;

    int                     nHeadW      = 0;
    int                     nHeadH      = 0;
    int                     nWidthSize  = 2;
    int                     nHeightSize = 3;
    AiScenario_NS::CVData_S stPoseInData;


    for (int nBoxNum = 0; nBoxNum < vecBoxes.size() / DATA_BOX_GROUP_SIZE; nBoxNum++)
    {
        /* 人头检测的结果，恢复到原来图片的大小 */
        vecBoxes[DATA_BOX_GROUP_SIZE * nBoxNum + 0] *= fWMultiple;
        vecBoxes[DATA_BOX_GROUP_SIZE * nBoxNum + 1] *= fHMultiple;
        vecBoxes[DATA_BOX_GROUP_SIZE * nBoxNum + 2] *= fWMultiple;
        vecBoxes[DATA_BOX_GROUP_SIZE * nBoxNum + 3] *= fHMultiple;

        /* 人头坐标，扩充到全身 */
        nHeadW = vecBoxes[DATA_BOX_GROUP_SIZE * nBoxNum + 2] - vecBoxes[DATA_BOX_GROUP_SIZE * nBoxNum + 0];
        nHeadH = vecBoxes[DATA_BOX_GROUP_SIZE * nBoxNum + 3] - vecBoxes[DATA_BOX_GROUP_SIZE * nBoxNum + 1];

        vecBoxes[DATA_BOX_GROUP_SIZE * nBoxNum + 0] -= nHeadW * nWidthSize * 0.5;
        // vecBoxes[DATA_BOX_GROUP_SIZE*nBoxNum + 1] -= nHeightSize*0.0;
        vecBoxes[DATA_BOX_GROUP_SIZE * nBoxNum + 2] += nHeadW * nWidthSize * 0.5;
        vecBoxes[DATA_BOX_GROUP_SIZE * nBoxNum + 3] += nHeadH * nHeightSize;
        if (vecBoxes[DATA_BOX_GROUP_SIZE * nBoxNum + 0] < 0)
        {
            vecBoxes[DATA_BOX_GROUP_SIZE * nBoxNum + 0] = 0;
        }
        if (vecBoxes[DATA_BOX_GROUP_SIZE * nBoxNum + 2] > aImage.cols)
        {
            vecBoxes[DATA_BOX_GROUP_SIZE * nBoxNum + 2] = aImage.cols;
        }
        if (vecBoxes[DATA_BOX_GROUP_SIZE * nBoxNum + 3] > aImage.rows)
        {
            vecBoxes[DATA_BOX_GROUP_SIZE * nBoxNum + 3] = aImage.rows;
        }

        /* 人体裁剪 */
        cv::Rect cropRect(cv::Point(int(vecBoxes[DATA_BOX_GROUP_SIZE * nBoxNum + 0]), int(vecBoxes[DATA_BOX_GROUP_SIZE * nBoxNum + 1])), cv::Point(int(vecBoxes[DATA_BOX_GROUP_SIZE * nBoxNum + 2]), int(vecBoxes[DATA_BOX_GROUP_SIZE * nBoxNum + 3])));
        stPoseInData.inMat = aImage(cropRect).clone();
        /* 人体关键点提取前处理 */
        if (stPoseInData.inMat.channels() != m_nFastPosetLimitChannel)
        {
            dlog(LOG_ERROR, "模型需要的通道数和输入图片的通道数不一致 inMat[%d] != m_nHeadDetectLimitChannel[%d]", stPoseInData.inMat.channels(), m_nFastPosetLimitChannel);
            return false;
        }
        if (stPoseInData.inMat.cols != m_nFastPoseLimitWidth || stPoseInData.inMat.rows != m_nFastPoseLimitHeight)
        {
            try
            {
                cv::resize(stPoseInData.inMat, stPoseInData.inMat, cv::Size(m_nFastPoseLimitWidth, m_nFastPoseLimitHeight));
            }
            catch (const cv::Exception& e)
            {
                dlog(LOG_ERROR, "cv::resize 失败: %s", e.what());
                return false;
            }
        }

        /* 人体关键点提取推理+后处理 */
        vecPoses.clear();
        bRet = m_pBInference->inference(stPoseInData, vecPoses);
        if (!bRet)
        {
            dlog(LOG_ERROR, "算法分析失败");
            return false;
        }

        float fPWMultiple = (vecBoxes[DATA_BOX_GROUP_SIZE * nBoxNum + 2] - vecBoxes[DATA_BOX_GROUP_SIZE * nBoxNum + 0]) * 1.0 / m_nFastPoseLimitWidth;
        float fPHMultiple = (vecBoxes[DATA_BOX_GROUP_SIZE * nBoxNum + 3] - vecBoxes[DATA_BOX_GROUP_SIZE * nBoxNum + 1]) * 1.0 / m_nFastPoseLimitHeight;

        std::vector<CBehaviorProcess::KeyPosInfo_S> vKeyPosInfo;
        vKeyPosInfo.clear();

        for (int nP = 0; nP < vecPoses.size() / DATA_KEY_GROUP_SIZE; nP++)
        {
            vecPoses[nP * DATA_KEY_GROUP_SIZE + 0] = vecBoxes[DATA_BOX_GROUP_SIZE * nBoxNum + 0] + vecPoses[nP * DATA_KEY_GROUP_SIZE + 0] * fPWMultiple;
            vecPoses[nP * DATA_KEY_GROUP_SIZE + 1] = vecBoxes[DATA_BOX_GROUP_SIZE * nBoxNum + 1] + vecPoses[nP * DATA_KEY_GROUP_SIZE + 1] * fPHMultiple;


            CBehaviorProcess::KeyPosInfo_S stKeyPosInfo;
            stKeyPosInfo.dX     = vecPoses[nP * DATA_KEY_GROUP_SIZE + 0];
            stKeyPosInfo.dY     = vecPoses[nP * DATA_KEY_GROUP_SIZE + 1];
            stKeyPosInfo.dScore = vecPoses[nP * DATA_KEY_GROUP_SIZE + 2];
            vKeyPosInfo.push_back(stKeyPosInfo);
        }

        /* 计算行为类型 */
        CBehaviorProcess BehaviorProcess;
        int              nBehavioralType = BehaviorProcess.getBehavioralType(
            std::get<AiScenario_NS::BehaviorParam_S>(stInData.varParam),
            vKeyPosInfo);

        vBehavior.push_back(nBehavioralType);

        /* 保存特征点 */
        vecKeyPoses.push_back(vecPoses);

        if (m_stInParam.stExParam.bDebug)
        {
            cv::Scalar color;
            color = CScenarioBase::GetUniqueColor(nBehavioralType);

            /* 绘制图片 */
            cv::rectangle(
                imageShow,
                cv::Point(vecBoxes[DATA_BOX_GROUP_SIZE * nBoxNum + 0], vecBoxes[DATA_BOX_GROUP_SIZE * nBoxNum + 1]),
                cv::Point(vecBoxes[DATA_BOX_GROUP_SIZE * nBoxNum + 2], vecBoxes[DATA_BOX_GROUP_SIZE * nBoxNum + 3]),
                color,
                4);

            cv::putText(
                imageShow,
                std::to_string(nBehavioralType),
                cv::Point(vecBoxes[DATA_BOX_GROUP_SIZE * nBoxNum + 0], vecBoxes[DATA_BOX_GROUP_SIZE * nBoxNum + 1] - 10),
                cv::FONT_HERSHEY_SIMPLEX,
                0.9,
                color,
                4);

            for (int nP = 0; nP < vecPoses.size() / DATA_KEY_GROUP_SIZE; nP++)
            {
                /* 跳过置信度不高的点 */
                if (vecPoses[nP * DATA_KEY_GROUP_SIZE + 2] < 0.7)
                {
                    continue;
                }
                cv::circle(imageShow, cv::Point(vecPoses[nP * DATA_KEY_GROUP_SIZE + 0], vecPoses[nP * DATA_KEY_GROUP_SIZE + 1]), 2, cv::Scalar(0, 0, 255), -1);
            }
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

    switch (m_stInParam.stNeedParam.enResultType)
    {
        case AiScenario_NS::JSON:
        {
            bRet = convertToJson(vecBoxes, vBehavior, vecKeyPoses, &pchOutData, nDataSize);
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

EXIT:

    return bRet;
}

/* 处理数据 */
bool Scenario_NS::CStudentBehaviorV2_0::process(
    AiScenario_NS::CAData_S stInData,
    char*&                  pchOutData,
    int&                    nDataSize)
{
    dlog(LOG_ERROR, "该场景为视频场景, 调用处理数据失败");
    return false;
}

/* 释放处理结果 */
bool Scenario_NS::CStudentBehaviorV2_0::releaseData(char*& pchOutData)
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

bool Scenario_NS::CStudentBehaviorV2_0::convertToJson(
    const std::vector<float>&              vecBoxes,
    const std::vector<int>&                vecBehavior,
    const std::vector<std::vector<float>>& vecKeyPoses,
    char**                                 pchOutData,
    int&                                   nDataSize)
{
    if (!pchOutData)
    {
        dlog(LOG_ERROR, "传入参数指针为空");
        return false;
    }

    if (vecKeyPoses.size() != (vecBoxes.size() / DATA_BOX_GROUP_SIZE) ||
        vecKeyPoses.size() != vecBehavior.size())
    {
        dlog(LOG_ERROR, "数据大小异常");
        return false;
    }

    auto pRootJson      = Json::init();
    auto pArrayDataJson = Json::Array::init();

    for (int nBoxNum = 0; nBoxNum < vecBoxes.size() / DATA_BOX_GROUP_SIZE; nBoxNum++)
    {
        auto pArrayBox = Json::Array::init();
        auto pItem     = Json::init();
        Json::Array::add(pArrayBox, vecBoxes[(nBoxNum * DATA_BOX_GROUP_SIZE) + 0]);
        Json::Array::add(pArrayBox, vecBoxes[(nBoxNum * DATA_BOX_GROUP_SIZE) + 1]);
        Json::Array::add(pArrayBox, vecBoxes[(nBoxNum * DATA_BOX_GROUP_SIZE) + 2]);
        Json::Array::add(pArrayBox, vecBoxes[(nBoxNum * DATA_BOX_GROUP_SIZE) + 3]);
        Json::add(pItem, "Box", pArrayBox);

#if 0
        /* 组装骨骼点信息 */
        auto pKeyArray   = Json::Array::init();
        auto pScoreArray = Json::Array::init();
        for (int nP = 0; nP < vecKeyPoses[nBoxNum].size() / DATA_KEY_GROUP_SIZE; nP++)
        {
            auto pKeyItemArray   = Json::Array::init();
            auto pScoreItemArray = Json::Array::init();

            Json::Array::add(pKeyItemArray, vecKeyPoses[nBoxNum][(nP * DATA_KEY_GROUP_SIZE) + 0]);
            Json::Array::add(pKeyItemArray, vecKeyPoses[nBoxNum][(nP * DATA_KEY_GROUP_SIZE) + 1]);
            Json::Array::add(pKeyArray, pKeyItemArray);

            Json::Array::add(pScoreItemArray, vecKeyPoses[nBoxNum][nP * DATA_KEY_GROUP_SIZE + 2]);
            Json::Array::add(pScoreArray, pScoreItemArray);
        }
        Json::add(pItem, "Keypoints", pKeyArray);
        Json::add(pItem, "KpScore", pScoreArray);
#endif

        Json::add(pItem, "Confidence", vecBoxes[(nBoxNum * DATA_BOX_GROUP_SIZE) + 4]);
        Json::add(pItem, "Class", vecBehavior[nBoxNum]);
        Json::Array::add(pArrayDataJson, pItem);
    }
    Json::add(pRootJson, "BaseData", pArrayDataJson);

    *pchOutData = Json::print(pRootJson);

    nDataSize = strlen(Json::to_string(pRootJson).c_str());

    if (pRootJson)
    {
        /* 释放数据 */
        Json::deinit(pRootJson);
    }

    return true;
}
