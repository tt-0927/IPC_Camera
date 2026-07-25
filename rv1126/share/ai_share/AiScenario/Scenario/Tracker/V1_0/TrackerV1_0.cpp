/*
 * @FilePath     : TrackerV1_0.cpp
 * @Author       : lih lih@kfb.cn
 * @Date         : 2024-05-31 15:31:40
 * @LastEditors: weigl 308241951@qq.com
 * @LastEditTime: 2024-12-18 19:15:51
 * @Description  :
 */

#include "TrackerV1_0.hpp"

#include <numeric>

#include "BodyFeature.hpp"
#include "dlog.h"
#include "HeadDetect.hpp"
#include "JsonInterfase.h"


/* 一组数据的大小 */
#define TRACK_POS_DATA_GROUP_SIZE 4
#define POS_DATA_GROUP_SIZE       6

using namespace Scenario_NS;

Scenario_NS::CTrakcerV1_0::CTrakcerV1_0(AiScenario_NS::InParam_S stInParam)
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

Scenario_NS::CTrakcerV1_0::~CTrakcerV1_0()
{
    unInit();
}

/* 初始化 */
bool Scenario_NS::CTrakcerV1_0::init()
{
    bool bRet = false;

    std::string strModel;
    if (m_stInParam.stNeedParam.vstrModelPath.size() > 1)
    {
        bRet = false;

        m_pHCInference = new InferenceV1_0_NS::CHeadDetect(
            m_stInParam.stNeedParam.vstrModelPath.at(0));
        if (m_pHCInference)
        {
            if (m_pHCInference->init())
            {
                bRet = m_pHCInference->getSizeLimit(
                    0,
                    m_nHeadDetectLimitWidth,
                    m_nHeadDetecttLimitHeight,
                    m_nHeadDetectLimitChannel);

                if (m_nHeadDetectLimitWidth <= 0 || m_nHeadDetecttLimitHeight <= 0)
                {
                    dlog(LOG_ERROR, "模型的目标尺寸[%d x %d]，获取异常",
                         m_nHeadDetectLimitWidth,
                         m_nHeadDetecttLimitHeight);
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

        m_pTInference = new InferenceV1_0_NS::CBodyFeature(
            m_stInParam.stNeedParam.vstrModelPath.at(1));
        if (m_pTInference)
        {
            if (m_pTInference->init())
            {
                bRet = m_pTInference->getSizeLimit(
                    0,
                    m_nBodyFeatureLimitWidth,
                    m_nBodyFeatureLimitHeight,
                    m_nBodyFeatureLimitChannel);

                if (m_nBodyFeatureLimitWidth <= 0 || m_nBodyFeatureLimitHeight <= 0)
                {
                    dlog(LOG_ERROR, "模型的目标尺寸[%d x %d]，获取异常",
                         m_nBodyFeatureLimitWidth,
                         m_nBodyFeatureLimitHeight);
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
bool Scenario_NS::CTrakcerV1_0::unInit()
{
    if (m_pHCInference)
    {
        delete m_pHCInference;
        m_pHCInference = nullptr;
        return true;
    }

    if (m_pTInference)
    {
        delete m_pTInference;
        m_pTInference = nullptr;
        return true;
    }

    return false;
}

/* 判断框选坐标是否存在人 */
bool Scenario_NS::CTrakcerV1_0::peopleChoose(std::vector<float> vResultPoints, std::vector<float> vSelectPoints)
{
    for (int i = 0; i < vSelectPoints.size() / 6; i++)
    {
        float w     = fmax(0.f, fmin(vResultPoints[2], vResultPoints[2 + i * 6]) - fmax(vResultPoints[0], vResultPoints[0 + i * 6]) + 1.0);
        float h     = fmax(0.f, fmin(vResultPoints[3], vResultPoints[3 + i * 6]) - fmax(vResultPoints[1], vResultPoints[1 + i * 6]) + 1.0);
        float fArea = w * h;
        float u     = (vResultPoints[2] - vResultPoints[0] + 1.0) * (vResultPoints[3] - vResultPoints[1] + 1.0) + (vResultPoints[2 + i * 6] - vResultPoints[0 + i * 6] + 1.0) * (vResultPoints[3 + i * 6] - vResultPoints[1 + i * 6] + 1.0) - fArea;
        if (u > 0.2)
        {
            return true;
        }
    }
    return false;
}

/* 清空保存的特征点 */
void Scenario_NS::CTrakcerV1_0::clearFeatures()
{
    for (auto item : m_vFeatures)
    {
        delete[] item;
    }
    m_vFeatures.clear();
}

/* 更新滑动窗口的信息 */
void Scenario_NS::CTrakcerV1_0::nextImg(cv::Mat aFrame, std::vector<float> vPoints, std::vector<float>& vNewPoints)
{
    /* 匹配算法 */
    float fFeatureThes = 0.85;
    /* 扩充特征提取区域 */
    float fX1;
    float fY1;
    float fX2;
    float fY2;
    /* 获取特征 */
    float pFeatures[512]     = { 0 };
    /*缓存特征*/
    float pSaveFeatures[512] = { 0 };
    /*计算余弦相似度，拿最大的框*/
    int   nMaxIndex          = -1;
    float fSimilarity        = 0;
    float fCs                = 0;

    for (int i = 0; i < vPoints.size() / 6; i++)
    {
        fX1 = vPoints[6 * i];
        fY1 = vPoints[6 * i + 1];
        fX2 = vPoints[6 * i + 2];
        fY2 = vPoints[6 * i + 3];

        AiScenario_NS::CVData_S aRoiImg;
        memset(&aRoiImg, 0, sizeof(AiScenario_NS::CVData_S));
        std::vector<float> vecTPos;
        vecTPos.clear();
        /* 获取当前人物的特征 */
        float ww, hh;
        ww       = fX2 - fX1;
        hh       = fY2 - fY1;
        float x1 = fX1 - ww / 1.5;
        float x2 = fX2 + ww / 1.5;
        float y1 = fY1;
        float y2 = fY2 + hh * 1.5;
        if (x1 < 0)
        {
            x1 = 0;
        }
        if (x2 > aFrame.cols)
        {
            x2 = aFrame.cols;
        }
        if (y2 > aFrame.rows)
        {
            y2 = aFrame.rows;
        }
        if (!aFrame.empty())
        {
            cv::Rect aRc  = cv::Rect(int(x1), int(y1), int(x2 - x1), int(y2 - y1));
            aRoiImg.inMat = aFrame.clone()(aRc);
            try
            {
                cv::resize(aRoiImg.inMat, aRoiImg.inMat, cv::Size(m_nBodyFeatureLimitWidth, m_nBodyFeatureLimitHeight));
            }
            catch (const cv::Exception& e)
            {
                dlog(LOG_ERROR, "cv::resize 失败: %s", e.what());
                return;
            }
            m_pTInference->inference(aRoiImg, vecTPos);
            for (size_t i = 0; i < vecTPos.size(); ++i)
            {
                pFeatures[i] = vecTPos[i];
            }
        }

        if (m_bFlag && (fX1 > 0 && fY1 > 0 && fX2 < aFrame.cols && fY2 < aFrame.rows))
        {
            // 复制m_vFeatures[0]的内容到pFeatures
            for (int ii = 0; ii < 512; ii++)
            {
                m_pMyFeatures[ii] = pFeatures[ii];
            }
            printf("特征获取成功\n");
            m_bFlag = false;
        }

        fCs = cosineSimilarity(pFeatures, m_pMyFeatures, 512);

        if (fCs > fSimilarity)
        {
            nMaxIndex   = i;
            fSimilarity = fCs;
            /* 临时保存最大的特征-缓存 */
            for (int ii = 0; ii < 512; ii++)
            {
                pSaveFeatures[ii] = pFeatures[ii];
            }
        }
    }

    /* 如果特征队列ls存在数据，并且当前帧存在特征大于阈值 */
    float fFeatureScores = 0;
    /*需要更新坐标的位置，即和特征列表余弦相识度最大的下面*/
    int   nMaxFeature    = -1;
    float nNCs           = 0;
    fCs                  = 0;
    if (nMaxIndex != -1 && fSimilarity > m_fSimilarityThreshold)
    {
        /* 如果特征列表没有保存满，则直接插入，否则更新特征最像的那个 */
        if (m_vFeatures.size() < m_nNumFeatures)
        {
            float* aNF = new float[512];
            for (int ii = 0; ii < 512; ii++)
            {
                /*赋值给最新的特征*/
                aNF[ii] = pSaveFeatures[ii];
            }
            m_vFeatures.push_back(aNF);
        }
        for (int jj = 0; jj < m_vFeatures.size(); jj++)
        {
            nNCs            = cosineSimilarity(m_vFeatures[jj], pSaveFeatures, 512);
            // printf("余弦相识度：%f\n",nNCs);
            fFeatureScores += nNCs;
            /*获取更大的相似度下标*/
            if (nNCs > fCs)
            {
                fCs         = nNCs;
                nMaxFeature = jj;
            }
        }
        if (m_vFeatures.size() >= m_nNumFeatures && nMaxFeature != -1)
        {
            /*替换特征*/
            for (int ii = 0; ii < 512; ii++)
            {
                /*赋值给最新的特征*/
                m_vFeatures[nMaxFeature][ii] = pSaveFeatures[ii];
            }
        }
        /* 判断分数是否符合要求 */
        fFeatureScores = fFeatureScores * 1.0 / m_vFeatures.size();
        // printf("综合得分为：%f,%ld\n",fFeatureScores,m_vFeatures.size());
        if (fFeatureScores > fFeatureThes)
        {
            vNewPoints.insert(vNewPoints.end(), { vPoints[6 * nMaxIndex], vPoints[6 * nMaxIndex + 1], vPoints[6 * nMaxIndex + 2], vPoints[6 * nMaxIndex + 3] });
            /*复制m_vFeatures[0]的内容到pFeatures*/
            for (int ii = 0; ii < 512; ii++)
            {
                m_pMyFeatures[ii] = pSaveFeatures[ii];
            }
        }
    }
}

/* 余弦相识度 */
float Scenario_NS::CTrakcerV1_0::cosineSimilarity(const float* vec1, const float* vec2, int size)
{
    float fDotProduct = std::inner_product(vec1, vec1 + size, vec2, 0.0f);

    float norm1 = std::sqrt(std::inner_product(vec1, vec1 + size, vec1, 0.0f));
    float norm2 = std::sqrt(std::inner_product(vec2, vec2 + size, vec2, 0.0f));

    float similarity = fDotProduct / (norm1 * norm2);
    return similarity;
}

void Scenario_NS::CTrakcerV1_0::resizeAndPad(cv::Mat& image, int nTargetWidth, int nTargetHeight)
{
    int imageWidth  = image.cols;
    int imageHeight = image.rows;

    // 计算图像的缩放比例
    double scale = std::min((double)nTargetWidth / imageWidth, (double)nTargetHeight / imageHeight);

    // 计算等比缩放后的目标尺寸
    int resizedWidth  = std::round(imageWidth * scale);
    int resizedHeight = std::round(imageHeight * scale);

    // 使用cv::resize函数进行等比缩放
    try
    {
        cv::resize(image, image, cv::Size(resizedWidth, resizedHeight));
    }
    catch (const cv::Exception& e)
    {
        dlog(LOG_ERROR, "cv::resize 失败: %s", e.what());
        return;
    }

    // 计算填充的边界大小
    int padWidth  = nTargetWidth - resizedWidth;
    int padHeight = nTargetHeight - resizedHeight;

    // 使用cv::copyMakeBorder函数进行填充
    cv::copyMakeBorder(image, image, 0, padHeight, 0, padWidth, cv::BORDER_CONSTANT, cv::Scalar(128, 128, 128));
}

/* 处理数据 */
bool Scenario_NS::CTrakcerV1_0::process(
    AiScenario_NS::CVData_S stInData,
    char*&                  pchOutData,
    int&                    nDataSize)
{
    if (stInData.inMat.empty() || !std::holds_alternative<AiScenario_NS::TrackerParam_S>(stInData.varParam))
    {
        dlog(LOG_ERROR, "传入图片或框选信息为空");
        return false;
    }

    if (!m_pHCInference || !m_pTInference)
    {
        dlog(LOG_ERROR, "未初始化算法类");
        return false;
    }

    bool               bRet = true;
    std::vector<float> vecHCPos;
    vecHCPos.clear();
    m_vTrackerPoints = std::get<AiScenario_NS::TrackerParam_S>(stInData.varParam).vBoxPoints;

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

    /* 前处理 */
    if (stInData.inMat.channels() != m_nHeadDetectLimitChannel)
    {
        dlog(LOG_ERROR, "模型需要的通道数和输入图片的通道数不一致 inMat[%d] != m_nHeadDetectLimitChannel[%d]", stInData.inMat.channels(), m_nHeadDetectLimitChannel);
        return bRet;
    }
    if (stInData.inMat.cols != m_nHeadDetectLimitWidth || stInData.inMat.rows != m_nHeadDetecttLimitHeight)
    {
        try
        {
            cv::resize(stInData.inMat, stInData.inMat, cv::Size(m_nHeadDetectLimitWidth, m_nHeadDetecttLimitHeight));
        }
        catch (const cv::Exception& e)
        {
            dlog(LOG_ERROR, "cv::resize 失败: %s", e.what());
            return false;
        }
    }

    /* 人头检测推理+后处理 */
    bRet = m_pHCInference->inference(stInData, vecHCPos);
    if (!bRet)
    {
        dlog(LOG_ERROR, "算法分析失败");
        return bRet;
    }

    /* 去掉画面下面四分之一的人框 */
    std::vector<float> vDeleteBox;
    for (int nSIndex = 0; nSIndex < vecHCPos.size() / 6; nSIndex++)
    {
        if (vecHCPos[nSIndex * 6 + 3] > (float)stInData.inMat.rows * 4.0 / 5)
        {
            vDeleteBox.push_back(nSIndex);
        }
    }
    /* 抹去画面下面四分之一的人框 */
    int vDeleteIdex = 0;
    for (int i = vDeleteBox.size() - 1; i >= 0; i--)
    {
        vDeleteIdex = vDeleteBox[i];
        for (int j = 5; j >= 0; j--)
        {
            vecHCPos.erase(vecHCPos.begin() + vDeleteIdex * 6 + j);    // 删除指定位置的元素
        }
    }

    /* 推理 */
    std::vector<float> vTargetPos;
    vTargetPos.clear();

    /* 跟踪检测推理 */
    /* 判断是否有框选 */
    if (m_vTrackerPoints.size() > 0)
    {
        /* 判断框选坐标是否存在人 */
        if (peopleChoose(m_vTrackerPoints, vecHCPos))
        {
            /* 框选功能启动 */
            bChoose = true;
            AiScenario_NS::CVData_S aRoiImg;
            memset(&aRoiImg, 0, sizeof(AiScenario_NS::CVData_S));
            /* 优先级人体特征插入 */
            PRIORITYFEATUES pOnePriorityData;
            pOnePriorityData.vPriorityFeatures = new float[512];
            /* 获取当前人物的特征 */
            float ww, hh;
            ww       = m_vTrackerPoints[2] - m_vTrackerPoints[0];
            hh       = m_vTrackerPoints[3] - m_vTrackerPoints[1];
            float x1 = m_vTrackerPoints[0] - ww / 1.5;
            float x2 = m_vTrackerPoints[2] + ww / 1.5;
            float y1 = m_vTrackerPoints[1];
            float y2 = m_vTrackerPoints[3] + hh * 1.5;
            if (x1 < 0)
            {
                x1 = 0;
            }
            if (x2 > stInData.inMat.cols)
            {
                x2 = stInData.inMat.cols;
            }
            if (y2 > stInData.inMat.rows)
            {
                y2 = stInData.inMat.rows;
            }
            if (!stInData.inMat.empty())
            {
                cv::Rect aRc  = cv::Rect(int(x1), int(y1), int(x2 - x1), int(y2 - y1));
                aRoiImg.inMat = stInData.inMat.clone()(aRc);
                resizeAndPad(aRoiImg.inMat, m_nBodyFeatureLimitWidth, m_nBodyFeatureLimitHeight);
                m_pTInference->inference(aRoiImg, vTargetPos);
            }
            pOnePriorityData.Priority = 0;
            if (m_vPriorityDatas.size() == 0)
            {
                m_vPriorityDatas.push_back(pOnePriorityData);
            }
            else
            {
                delete[] m_vPriorityDatas[0].vPriorityFeatures;
                m_vPriorityDatas[0] = pOnePriorityData;
            }
        }
        else
        {
            return -1;
        }
    }

    /*满足优先级的标志位*/
    int                nFlg = 0;
    std::vector<float> vecT1Pos;
    vecT1Pos.clear();

    m_vTrackerPoints.clear();
    /* 优先级人物的匹配 */
    if (vecHCPos.size() / 6 > 0 && m_vPriorityDatas.size() >= 1)
    {
        AiScenario_NS::CVData_S aRoiImg;
        memset(&aRoiImg, 0, sizeof(AiScenario_NS::CVData_S));
        float* detectFeatrue  = new float[512];
        float  fMaxSimilarity = 0.0f;
        float  sPointData[7];
        for (int i = 0; i < vecHCPos.size() / 6; i++)
        {
            /* 获取当前人物的特征 */
            float ww, hh;
            ww       = vecHCPos[i * 6 + 2] - vecHCPos[i * 6 + 0];
            hh       = vecHCPos[i * 6 + 3] - vecHCPos[i * 6 + 1];
            float x1 = vecHCPos[i * 6 + 0] - ww / 1.5;
            float x2 = vecHCPos[i * 6 + 2] + ww / 1.5;
            float y1 = vecHCPos[i * 6 + 1];
            float y2 = vecHCPos[i * 6 + 3] + hh * 1.5;
            if (x1 < 0)
            {
                x1 = 0;
            }
            if (x2 > stInData.inMat.cols)
            {
                x2 = stInData.inMat.cols;
            }
            if (y2 > stInData.inMat.rows)
            {
                y2 = stInData.inMat.rows;
            }
            if (!stInData.inMat.empty())
            {
                cv::Rect aRc  = cv::Rect(int(x1), int(y1), int(x2 - x1), int(y2 - y1));
                aRoiImg.inMat = stInData.inMat.clone()(aRc);
                resizeAndPad(aRoiImg.inMat, m_nBodyFeatureLimitWidth, m_nBodyFeatureLimitHeight);
                m_pTInference->inference(aRoiImg, vecT1Pos);
                for (size_t i = 0; i < vecT1Pos.size(); ++i)
                {
                    detectFeatrue[i] = vecT1Pos[i];
                }
            }

            float fSimilarity = cosineSimilarity(detectFeatrue, m_vPriorityDatas[0].vPriorityFeatures, 512);

            if (fSimilarity > 0.98)    // PStrack.fSimilarityThreshold)
            {
                if (fMaxSimilarity <= fSimilarity)
                {
                    fMaxSimilarity = fSimilarity;
                    printf("======满足======%f\n", fSimilarity);
                    nFlg = 1;
                    /* 将框选的坐标特征初始化为第一个特征 */
                    for (int nPIndex = 0; nPIndex < 6; nPIndex++)
                    {
                        sPointData[nPIndex] = vecHCPos[i * 6 + nPIndex];
                        printf("%f ", sPointData[nPIndex]);
                    }
                    printf("\n");
                }
            }
        }
        if (nFlg != 0)
        {
            /* 被跟踪人特征清除算法 */
            clearFeatures();
            vTargetPos.clear();
            /* 将框选的坐标特征初始化为第一个特征 */
            for (int nPIndex = 0; nPIndex < 6; nPIndex++)
            {
                vTargetPos.push_back(sPointData[nPIndex]);
            }
        }
        delete[] detectFeatrue;
    }

    /* 后处理 */
    /*没有人的时候，回全景*/
    if (0 == vecHCPos.size() / 6)
    {
        m_nChangeNum++;
    }
    /*一个人的时候，直接返回该人的位置*/
    else if (1 == vecHCPos.size() / 6 && nFlg == 0)
    {
        /* 将框选的坐标特征初始化为第一个特征 */
        for (int nPIndex = 0; nPIndex < 4; nPIndex++)
        {
            m_vTrackerPoints.push_back(vecHCPos[nPIndex]);
        }
        m_nOneFlg = 1;
    }
    /*多人且没有优先级的时候，就使用上一个人的特征列表进行匹配*/
    else if (vecHCPos.size() / 6 > 1 && vTargetPos.size() == 0)
    {
        m_nNumFeatures = 20;
        /*特征匹配得到下一帧被跟踪人头的位置*/
        nextImg(stInData.inMat, vecHCPos, m_vTrackerPoints);
        if (1 == m_nOneFlg)
        {
            /* 被跟踪人特征清除算法 */
            clearFeatures();
            vTargetPos.clear();
            m_nOneFlg = 0;
        }
    }
    /*多人且满足优先级的人的时候，直接返回优先级的位置*/
    else if (nFlg != 0)
    {
        m_nNumFeatures = 20;
        /*特征匹配得到下一帧被跟踪人头的位置*/
        nextImg(stInData.inMat, vTargetPos, m_vTrackerPoints);
    }

    switch (m_stInParam.stNeedParam.enResultType)
    {
        case AiScenario_NS::JSON:
        {
            bRet = convertToJson(m_vTrackerPoints, vecHCPos, &pchOutData, nDataSize);
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
bool Scenario_NS::CTrakcerV1_0::process(
    AiScenario_NS::CAData_S stInData,
    char*&                  pchOutData,
    int&                    nDataSize)
{
    dlog(LOG_ERROR, "该场景为视频场景, 调用处理数据失败");
    return false;
}

/* 释放处理结果 */
bool Scenario_NS::CTrakcerV1_0::releaseData(char*& pchOutData)
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

bool Scenario_NS::CTrakcerV1_0::convertToJson(
    std::vector<float> vTargetXY,
    std::vector<float> vAllXY,
    char**             pchOutData,
    int&               nDataSize)
{
    auto pRootJson         = Json::init();
    auto pDataJson         = Json::init();
    auto pArrayTrackerJson = Json::Array::init();
    auto pArrayAllJson     = Json::Array::init();

    for (int nIndex = 0; nIndex < vTargetXY.size(); nIndex++)
    {
        if (vTargetXY.size() > ((nIndex * TRACK_POS_DATA_GROUP_SIZE) + 3))
        {
            auto pArrayBox = Json::Array::init();
            auto pItem     = Json::init();
            Json::Array::add(pArrayBox, (int)vTargetXY[nIndex * TRACK_POS_DATA_GROUP_SIZE]);
            Json::Array::add(pArrayBox, (int)vTargetXY[(nIndex * TRACK_POS_DATA_GROUP_SIZE) + 1]);
            Json::Array::add(pArrayBox, (int)vTargetXY[(nIndex * TRACK_POS_DATA_GROUP_SIZE) + 2]);
            Json::Array::add(pArrayBox, (int)vTargetXY[(nIndex * TRACK_POS_DATA_GROUP_SIZE) + 3]);
            Json::add(pItem, "Box", pArrayBox);
            Json::Array::add(pArrayTrackerJson, pItem);
        }
    }
    for (int nIndex = 0; nIndex < vAllXY.size(); nIndex++)
    {
        if (vAllXY.size() > ((nIndex * POS_DATA_GROUP_SIZE) + 3))
        {
            auto pArrayBox = Json::Array::init();
            auto pItem     = Json::init();
            Json::Array::add(pArrayBox, vAllXY[nIndex * POS_DATA_GROUP_SIZE]);
            Json::Array::add(pArrayBox, vAllXY[(nIndex * POS_DATA_GROUP_SIZE) + 1]);
            Json::Array::add(pArrayBox, vAllXY[(nIndex * POS_DATA_GROUP_SIZE) + 2]);
            Json::Array::add(pArrayBox, vAllXY[(nIndex * POS_DATA_GROUP_SIZE) + 3]);
            Json::add(pItem, "Box", pArrayBox);
            Json::Array::add(pArrayAllJson, pItem);
        }
    }
    Json::add(pDataJson, "Tracker", pArrayTrackerJson);
    Json::add(pDataJson, "AllPos", pArrayAllJson);
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
