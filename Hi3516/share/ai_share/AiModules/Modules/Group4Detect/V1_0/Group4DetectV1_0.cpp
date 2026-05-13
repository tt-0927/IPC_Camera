/*
 * @Author: lianghy lianghy@kfb.cn
 * @Date: 2026-01-08 16:28:43
 * @LastEditors: lianghy lianghy@kfb.cn
 * @LastEditTime: 2026-01-20 11:29:53
 * @FilePath: /1126/share/ai_share/AiModules/Modules/Group4Detect/V1_0/Group4DetectV1_0.hpp
 * @Description: cigarette(香烟)、sleep(睡觉)、phone(玩手机)、fall(摔跤)、falling(摔跤中)
 */

#include "BYTETracker.h"
#include "Group4DetectV1_0.hpp"
#include "SaveImage.hpp"
#include "StatisticsTimer.hpp"

#include <unistd.h>

using namespace Group4Detect_NS;

Group4Detect_NS::CGroup4DetectV1_0::CGroup4DetectV1_0(InParam_S stInParam)
    : m_stInParam(stInParam)
{
}

Group4Detect_NS::CGroup4DetectV1_0::~CGroup4DetectV1_0()
{
    unInit();
}

/* 初始化 */
bool Group4Detect_NS::CGroup4DetectV1_0::init()
{
    CStatisticsTimer runTime("模型组4检测初始化耗时");
    bool             bRet = false;

    m_pYoloUltralytics = new Inference_NS::CYoloUltralytics(m_stInParam.strModelPath);
    if (m_pYoloUltralytics && m_pYoloUltralytics->init())
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
bool Group4Detect_NS::CGroup4DetectV1_0::unInit()
{
    CStatisticsTimer runTime("模型组4检测反初始化耗时");
    if (m_pYoloUltralytics)
    {
        delete m_pYoloUltralytics;
        m_pYoloUltralytics = nullptr;
    }

    return true;
}

/* 处理数据 */
bool Group4Detect_NS::CGroup4DetectV1_0::process(
    bool                   isDetectPerson,
    InData_S               stInData,
    std::vector<Result_S> &vecResult,
    OutData_S             *stOutData)
{
    OutData_S defaultOutData;

    // 如果传入的指针为空，则使用默认对象
    if (stOutData == nullptr)
    {
        stOutData = &defaultOutData;
    }

    vecResult.clear();

    if (stInData.inMat.empty())
    {
        printf("传入图片为空\n");
        return false;
    }

    if (!m_pYoloUltralytics)
    {
        printf("未初始化算法类\n");
        return false;
    }

    if (m_stInParam.bDebug && !stInData.inMat.empty() && !m_stInParam.strOriginalDataPath.empty())
    {

        if (!Modules_NS::saveImage(stInData.inMat, m_stInParam.strOriginalDataPath))
        {
            printf("Debug-保存图片失败[%s]\n", m_stInParam.strOriginalDataPath.c_str());
        }
    }

    /* 推理+后处理 */
    Inference_NS::InputData_S stInputData;
    stInputData.pData              = (float *)stInData.inMat.data;
    stInputData.nDataSize          = static_cast<size_t>(stInData.inMat.total() * stInData.inMat.elemSize());
    stInputData.stBoxs.fConfidence = stInData.stParam.fBoxThreshold;
    stInputData.stBoxs.fNms        = stInData.stParam.fNmsThreshold;

    /* 推理+后处理 */
    std::vector<Inference_NS::BoxData_S> vBoxDatas;
    {
        CStatisticsTimer runTime("推理耗时");

        bool bRet = false;
        if (m_pYoloUltralytics)
        {
            bRet = m_pYoloUltralytics->inference(stInputData, vBoxDatas);
            if (!bRet)
            {
                printf("算法分析失败\n");
                return false;
            }
        }
        else
        {
            printf("m_pYoloUltralytics 为空，算法分析失败\n");
            return false;
        }
    }

    if (access("/test_Group4Detect", F_OK) == 0)
    {
        for (int i = 0; i < vBoxDatas.size(); i++)
        {

            printf("===================> 模型组4  当前目标[%d] 类别class[%d]  置信度[%.2f] <===================\n",
                   i + 1,
                   vBoxDatas[i].nLabel,
                   vBoxDatas[i].fConfidence);

            std::vector<cv::Point> rectPoints = {
                cv::Point(vBoxDatas[i].stBoxs.nX1, vBoxDatas[i].stBoxs.nY1),  // 左上角
                cv::Point(vBoxDatas[i].stBoxs.nY2, vBoxDatas[i].stBoxs.nY1),  // 右上角
                cv::Point(vBoxDatas[i].stBoxs.nY2, vBoxDatas[i].stBoxs.nX2),  // 右下角
                cv::Point(vBoxDatas[i].stBoxs.nX1, vBoxDatas[i].stBoxs.nX2)   // 左下角
            };
            cv::polylines(stInData.inMat,
                          rectPoints,
                          true,
                          cv::Scalar(0, 255, 0), /* 边框颜色(绿色) */
                          2,
                          cv::LINE_AA);

            Modules_NS::saveImage(stInData.inMat, "/mnt/Group4Detect_test");
        }
    }

    /* 标记当前帧是否识别到了 香烟 */
    bool bDetectCigarette = false;
    /* 标记当前帧是否识别到了 睡觉 */
    bool bDetectSleep = false;
    /* 标记当前帧是否识别到了 玩手机 */
    bool bDetectPhone = false;
    /* 标记当前帧是否识别到了 摔跤 */
    bool bDetectFall = false;

    for (const auto &box : vBoxDatas)
    {
        Result_S stResult;
        stResult.fBoxConfidence = box.fConfidence;
        stResult.fX1            = box.stBoxs.nX1;
        stResult.fX2            = box.stBoxs.nX2;
        stResult.fY1            = box.stBoxs.nY1;
        stResult.fY2            = box.stBoxs.nY2;
        stResult.nClassId       = box.nLabel;

        if (access("/group4Debug", F_OK) == 0)
        {
            printf("===================>模型组合4检测  类别class[%d]  置信度[%.2f]<===================\n", stResult.nClassId, stResult.fBoxConfidence);
        }

        if (isDetectPerson)
        {
            if (stResult.nClassId == CIGARETTE)
            {
                if (stInData.stParam.stCigaretteDetectParam.bEnable)
                {
                    if (stResult.fBoxConfidence > stInData.stParam.stCigaretteDetectParam.fConfidence)
                    {
                        if (access("/group4Debug", F_OK) == 0)
                        {
                            printf("===================>香烟检测  类别class[%d]  置信度[%.2f] 香烟置信度阈值[%.2f]<===================\n",
                                   stResult.nClassId,
                                   stResult.fBoxConfidence,
                                   stInData.stParam.stCigaretteDetectParam.fConfidence);
                        }
                        bDetectCigarette = true;
                        vecResult.push_back(stResult);
                    }
                }
                continue;
            }

            if (stResult.nClassId == PHONE)
            {
                if (stInData.stParam.stPhoneParam.bEnable)
                {
                    if (stResult.fBoxConfidence > stInData.stParam.stPhoneParam.fConfidence)
                    {
                        if (access("/group4Debug", F_OK) == 0)
                        {
                            printf("===================>玩手机检测  类别class[%d]  置信度[%.2f] 玩手机置信度阈值[%.2f]<===================\n",
                                   stResult.nClassId,
                                   stResult.fBoxConfidence,
                                   stInData.stParam.stPhoneParam.fConfidence);
                        }
                        bDetectPhone = true;
                        vecResult.push_back(stResult);
                    }
                }
                continue;
            }
        }
        else
        {
            if (stResult.nClassId == SLEEP)
            {
                if (stInData.stParam.stSleepParam.bEnable)
                {
                    if (stResult.fBoxConfidence > stInData.stParam.stSleepParam.fConfidence)
                    {
                        if (access("/group4Debug", F_OK) == 0)
                        {
                            printf("===================>睡觉  类别class[%d]  置信度[%.2f] 睡觉检测置信度阈值[%.2f]<===================\n",
                                   stResult.nClassId,
                                   stResult.fBoxConfidence,
                                   stInData.stParam.stSleepParam.fConfidence);
                        }
                        bDetectSleep = true;
                        vecResult.push_back(stResult);
                    }
                }
                continue;
            }

            if (stResult.nClassId == FALL || stResult.nClassId == FALLING)
            {
                if (stInData.stParam.stFallParam.bEnable)
                {
                    if (stResult.fBoxConfidence > stInData.stParam.stFallParam.fConfidence)
                    {
                        if (access("/group4Debug", F_OK) == 0)
                        {
                            printf("===================>摔跤检测  类别class[%d]  置信度[%.2f] 摔跤置信度阈值[%.2f]<===================\n",
                                   stResult.nClassId,
                                   stResult.fBoxConfidence,
                                   stInData.stParam.stFallParam.fConfidence);
                        }
                        bDetectFall = true;
                        vecResult.push_back(stResult);
                    }
                }
                continue;
            }
        }
    }

    int64_t llCurTimestamp = time(NULL);
    if (isDetectPerson)
    {
        /* 检测到吸烟 */
        if (bDetectCigarette)
        {
            /* 已经开始计时，并且在时间窗口内达标 */
            if (m_llCigaretteTimestamp != 0 && llCurTimestamp - m_llCigaretteTimestamp >= stInData.stParam.stCigaretteDetectParam.nDetectDuration)
            {
                stOutData->bCigarette = true;
                printf("【报警】识别到了吸烟！[%lld]\n", llCurTimestamp - m_llCigaretteTimestamp);
            }

            /* 第一次检测到吸烟，记录时间 */
            if (m_llCigaretteTimestamp == 0)
            {
                m_llCigaretteTimestamp = llCurTimestamp;
            }
        }
        else
        {
            /* 已经开始计时，但在整个时间窗口内都没再检测到吸烟 */
            if (m_llCigaretteTimestamp != 0 && (llCurTimestamp - m_llCigaretteTimestamp > stInData.stParam.stCigaretteDetectParam.nDetectDuration))
            {
                m_llCigaretteTimestamp = 0;
            }
        }

        if (bDetectPhone)
        {
            /* 已经开始计时，并且在时间窗口内达标 */
            if (m_llPhoneTimestamp != 0 && llCurTimestamp - m_llPhoneTimestamp >= stInData.stParam.stPhoneParam.nDetectDuration)
            {
                stOutData->bPhone = true;
                printf("【报警】识别到了玩手机！[%lld]\n", llCurTimestamp - m_llPhoneTimestamp);
            }

            /* 第一次检测到玩手机，记录时间 */
            if (m_llPhoneTimestamp == 0)
            {
                m_llPhoneTimestamp = llCurTimestamp;
            }
        }
        else
        {
            /* 已经开始计时，但在整个时间窗口内都没再检测到玩手机 */
            if (m_llPhoneTimestamp != 0 && (llCurTimestamp - m_llPhoneTimestamp > stInData.stParam.stPhoneParam.nDetectDuration))
            {
                m_llPhoneTimestamp = 0;
            }
        }
    }
    else
    {
        if (bDetectSleep)
        {
            m_nSleepFrameCount++;
            if (m_nSleepFrameCount >= stInData.stParam.stSleepParam.nDetectFrame)
            {
                printf("【报警】识别到了睡觉！[%d]\n", m_nSleepFrameCount);
                stOutData->bSleep  = true;
                // m_nSleepFrameCount = 0; 
            }
        }
        else
        {
            m_nSleepFrameCount = 0;
        }

        if (bDetectFall)
        {
            m_nFallFrameCount++;
            if (m_nFallFrameCount >= stInData.stParam.stFallParam.nDetectFrame)
            {
                printf("【报警】识别到摔跤！[%d]\n", m_nFallFrameCount);
                stOutData->bFall  = true;
                // m_nFallFrameCount = 0;
            }
        }
        else
        {
            m_nFallFrameCount = 0;
        }
    }

    return true;
}
