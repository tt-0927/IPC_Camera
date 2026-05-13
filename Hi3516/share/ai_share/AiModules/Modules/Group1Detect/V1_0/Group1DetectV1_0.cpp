/*
 * @Author: 梁浩尧 lianghaoyao@kfb.cn
 * @Date: 2025-12-24 16:23:34
 * @LastEditors: lianghy lianghy@kfb.cn
 * @LastEditTime: 2026-01-20 11:24:18
 * @FilePath: /1126/share/ai_share/AiModules/Modules/Group1Detect/V1_0/Group1DetectV1_0.hpp
 * @Description: notHelmet(未戴安全帽)、helmet(安全帽)、reflective(反光衣)、safetyRope(安全绳)、exposedSoil(泥土裸露)、person(人)
 */

#include "BYTETracker.h"
#include "Group1DetectV1_0.hpp"
#include "SaveImage.hpp"
#include "StatisticsTimer.hpp"

#include <unistd.h>

using namespace Group1Detect_NS;

Group1Detect_NS::CGroup1DetectV1_0::CGroup1DetectV1_0(InParam_S stInParam)
    : m_stInParam(stInParam)
{
}

Group1Detect_NS::CGroup1DetectV1_0::~CGroup1DetectV1_0()
{
    unInit();
}

/* 初始化 */
bool Group1Detect_NS::CGroup1DetectV1_0::init()
{
    CStatisticsTimer runTime("模型组1检测初始化耗时");
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
bool Group1Detect_NS::CGroup1DetectV1_0::unInit()
{
    CStatisticsTimer runTime("模型组1检测反初始化耗时");
    if (m_pYoloUltralytics)
    {
        delete m_pYoloUltralytics;
        m_pYoloUltralytics = nullptr;
    }

    return true;
}

/* 处理数据 */
bool Group1Detect_NS::CGroup1DetectV1_0::process(
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
        if (m_pYoloUltralytics)
        {
            bool bRet = m_pYoloUltralytics->inference(stInputData, vBoxDatas);
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

    if (access("/test_Group1Detect", F_OK) == 0)
    {
        for (int i = 0; i < vBoxDatas.size(); i++)
        {

            printf("===================> 模型组1  当前目标[%d] 类别class[%d]  置信度[%.2f] <===================\n",
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

            Modules_NS::saveImage(stInData.inMat, "/mnt/Group1Detect_test");
        }
    }

    /* 标记当前帧是否识别到了 安全帽 */
    bool bDetectSafetyHelmet = false;
    /* 标记当前帧是否识别到了 反光衣 */
    bool bDetectReflective = false;
    /* 标记当前帧是否识别到了 安全带 */
    bool bDetectSafetyRope = false;
    /* 标记当前帧是否识别到了 泥土裸露 */
    bool bDetectExposedSoil = false;

    for (const auto &box : vBoxDatas)
    {
        Result_S stResult;
        stResult.fBoxConfidence = box.fConfidence;
        stResult.fX1            = box.stBoxs.nX1;
        stResult.fX2            = box.stBoxs.nX2;
        stResult.fY1            = box.stBoxs.nY1;
        stResult.fY2            = box.stBoxs.nY2;
        stResult.nClassId       = box.nLabel;

        if (access("/group1Debug", F_OK) == 0)
        {
            printf("===================>模型组合1检测  类别class[%d]  置信度[%.2f]<===================\n", stResult.nClassId, stResult.fBoxConfidence);
        }

        if (stResult.nClassId == NOTHELMET)
        {
            if (stInData.stParam.stSafetyHelmetDetectParam.bEnable)
            {
                if (stResult.fBoxConfidence > stInData.stParam.stSafetyHelmetDetectParam.fConfidence)
                {
                    if (access("/group1Debug", F_OK) == 0)
                    {
                        printf("===================>安全帽检测  类别class[%d]  置信度[%.2f] 安全帽置信度阈值[%.2f]<===================\n",
                               stResult.nClassId,
                               stResult.fBoxConfidence,
                               stInData.stParam.stSafetyHelmetDetectParam.fConfidence);
                    }
                    bDetectSafetyHelmet = true;
                    vecResult.push_back(stResult);
                }
            }
            continue;
        }

        if (stResult.nClassId == REFLECTIVE)
        {
            if (stInData.stParam.stReflectiveClothingParam.bEnable)
            {
                if (stResult.fBoxConfidence > stInData.stParam.stReflectiveClothingParam.fConfidence)
                {
                    if (access("/group1Debug", F_OK) == 0)
                    {
                        printf("===================>反光衣检测  类别class[%d]  置信度[%.2f] 反光衣检测置信度阈值[%.2f]<===================\n",
                               stResult.nClassId,
                               stResult.fBoxConfidence,
                               stInData.stParam.stReflectiveClothingParam.fConfidence);
                    }
                    bDetectReflective = true;
                    vecResult.push_back(stResult);
                }
            }
            continue;
        }

        if (stResult.nClassId == SAFETYROPE)
        {
            if (stInData.stParam.stHighAltitudeSeatbeltParam.bEnable)
            {
                if (stResult.fBoxConfidence > stInData.stParam.stHighAltitudeSeatbeltParam.fConfidence)
                {
                    if (access("/group1Debug", F_OK) == 0)
                    {
                        printf("===================>安全绳检测  类别class[%d]  置信度[%.2f] 安全绳置信度阈值[%.2f]<===================\n",
                               stResult.nClassId,
                               stResult.fBoxConfidence,
                               stInData.stParam.stHighAltitudeSeatbeltParam.fConfidence);
                    }
                    bDetectSafetyRope = true;
                    vecResult.push_back(stResult);
                }
            }
            continue;
        }

        if (stResult.nClassId == EXPOSEDSOIL)
        {
            if (stInData.stParam.stBareSoiletParam.bEnable)
            {
                if (stResult.fBoxConfidence > stInData.stParam.stBareSoiletParam.fConfidence)
                {
                    if (access("/group1Debug", F_OK) == 0)
                    {
                        printf("===================>泥土裸露检测  类别class[%d]  置信度[%.2f] 泥土裸露置信度阈值[%.2f]<===================\n",
                               stResult.nClassId,
                               stResult.fBoxConfidence,
                               stInData.stParam.stBareSoiletParam.fConfidence);
                    }
                    bDetectExposedSoil = true;
                    vecResult.push_back(stResult);
                }
            }
            continue;
        }
    }

    if (bDetectSafetyHelmet)
    {
        m_nSafetyHelmetDetectFrameCount++;
        if (m_nSafetyHelmetDetectFrameCount >= stInData.stParam.stSafetyHelmetDetectParam.nDetectFrame)
        {
            printf("【报警】未识别到安全帽！ [%d]\n", m_nSafetyHelmetDetectFrameCount);
            stOutData->bSafetyHelmet = true;
            // m_nSafetyHelmetDetectFrameCount = 0;
        }
    }
    else
    {
        m_nSafetyHelmetDetectFrameCount = 0;
    }

    if (bDetectReflective)
    {
        m_nReflectiveClothingFrameCount++;
        if (m_nReflectiveClothingFrameCount >= stInData.stParam.stReflectiveClothingParam.nDetectFrame)
        {
            printf("【报警】识别到了反光衣！[%d]\n", m_nReflectiveClothingFrameCount);
            stOutData->bReflectiveClothing = true;
            // m_nReflectiveClothingFrameCount = 0;
        }
    }
    else
    {
        m_nReflectiveClothingFrameCount = 0;
    }

    if (bDetectSafetyRope)
    {
        m_nHighAltitudeSeatbeltFrameCount++;
        if (m_nHighAltitudeSeatbeltFrameCount >= stInData.stParam.stReflectiveClothingParam.nDetectFrame)
        {
            printf("【报警】识别到了高空安全带识别！[%d]\n", m_nHighAltitudeSeatbeltFrameCount);
            stOutData->bHighAltitudeSeatbelt = true;
            // m_nHighAltitudeSeatbeltFrameCount = 0;
        }
    }
    else
    {
        m_nHighAltitudeSeatbeltFrameCount = 0;
    }

    if (bDetectExposedSoil)
    {
        m_nBareSoiletFrameCount++;
        if (m_nBareSoiletFrameCount >= stInData.stParam.stReflectiveClothingParam.nDetectFrame)
        {
            printf("【报警】识别到了泥土裸露！[%d]\n", m_nBareSoiletFrameCount);
            stOutData->bBareSoilet = true;
            // m_nBareSoiletFrameCount = 0;
        }
    }
    else
    {
        m_nBareSoiletFrameCount = 0;
    }

    return true;
}
