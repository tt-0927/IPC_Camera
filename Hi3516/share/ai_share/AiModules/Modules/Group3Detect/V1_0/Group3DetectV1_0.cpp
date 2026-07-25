/*
 * @Author: lianghy lianghy@kfb.cn
 * @Date: 2026-01-08 16:28:43
 * @LastEditors: lianghy lianghy@kfb.cn
 * @LastEditTime: 2026-04-07 14:35:55
 * @FilePath: /1126/share/ai_share/AiModules/Modules/Group3Detect/V1_0/Group3DetectV1_0.hpp
 * @Description: smoke(烟雾)、fire(火焰)、Overflow(垃圾满溢)、expose(垃圾暴露)、Complete(井盖完好)、Damaged(井盖破损)、Lost(井盖丢失)、Uncovered(未盖井盖)、BreakoutOfOuterEdge(井盖外边沿破损)、WaterAccumulation(道路积水)
 */

#include "BYTETracker.h"
#include "Group3DetectV1_0.hpp"
#include "SaveImage.hpp"
#include "StatisticsTimer.hpp"

#include <unistd.h>

using namespace Group3Detect_NS;

Group3Detect_NS::CGroup3DetectV1_0::CGroup3DetectV1_0(InParam_S stInParam)
    : m_stInParam(stInParam)
{
}

Group3Detect_NS::CGroup3DetectV1_0::~CGroup3DetectV1_0()
{
    unInit();
}

/* 初始化 */
bool Group3Detect_NS::CGroup3DetectV1_0::init()
{
    CStatisticsTimer runTime("模型组3检测初始化耗时");
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
bool Group3Detect_NS::CGroup3DetectV1_0::unInit()
{
    CStatisticsTimer runTime("模型组3检测反初始化耗时");
    if (m_pYoloUltralytics)
    {
        delete m_pYoloUltralytics;
        m_pYoloUltralytics = nullptr;
    }

    return true;
}

/* 处理数据 */
bool Group3Detect_NS::CGroup3DetectV1_0::process(
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

    if(stInData.inMat.type() == CV_8UC3)
    {
        if(stInData.inMat.cols != m_nLimitWidth || stInData.inMat.rows != m_nLimitHeight)
        {
            cv::Mat reMat;
            resizeAndPadImage(stInData.inMat,reMat);
            stInputData.pData              = (float *)reMat.data;
            stInputData.nDataSize          = static_cast<size_t>(reMat.total() * reMat.elemSize() * sizeof(float));
        }
        else
        {
            stInputData.pData              = (float *)stInData.inMat.data;
            stInputData.nDataSize          = static_cast<size_t>(stInData.inMat.total() * stInData.inMat.elemSize() * sizeof(float));
        }
    }
    else
    {
        stInputData.pData              = (float *)stInData.inMat.data;
        stInputData.nDataSize          = static_cast<size_t>(stInData.inMat.total() * stInData.inMat.elemSize() * sizeof(float));
    }
    
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

    if (access("/test_Group3Detect", F_OK) == 0)
    {
        for (int i = 0; i < vBoxDatas.size(); i++)
        {

            printf("===================> 模型组3  当前目标[%d] 类别class[%d]  置信度[%.2f] <===================\n",
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

            Modules_NS::saveImage(stInData.inMat, "/mnt/Group3Detect_test");
        }
    }

    /* 标记当前帧是否识别到了 烟雾 */
    bool bDetectSmoke = false;
    /* 标记当前帧是否识别到了 火焰 */
    bool bDetectFire = false;
    /* 标记当前帧是否识别到了 垃圾暴露 */
    bool bDetectGarbageExposure = false;
    /* 标记当前帧是否识别到了 垃圾满溢 */
    bool bDetectGarbageOver = false;
    /* 标记当前帧是否识别到了 井盖异常 */
    bool bDetectManholeCoverAbnormal = false;
    /* 标记当前帧是否识别到了 道路积水 */
    bool bDetectRoadPonding = false;

    for (const auto &box : vBoxDatas)
    {
        Result_S stResult;
        stResult.fBoxConfidence = box.fConfidence;
        stResult.fX1            = box.stBoxs.nX1;
        stResult.fX2            = box.stBoxs.nX2;
        stResult.fY1            = box.stBoxs.nY1;
        stResult.fY2            = box.stBoxs.nY2;
        stResult.nClassId       = box.nLabel;

        if (access("/group3Debug", F_OK) == 0)
        {
            printf("===================>模型组合3检测  类别class[%d]  置信度[%.2f]<===================\n", stResult.nClassId, stResult.fBoxConfidence);
        }

        if (stResult.nClassId == SMOKE)
        {
            if (stInData.stParam.stSmokeFireDetectParam.bEnable)
            {
                if (stResult.fBoxConfidence > stInData.stParam.stSmokeFireDetectParam.fConfidence)
                {
                    if (access("/group3Debug", F_OK) == 0)
                    {
                        printf("===================>烟雾检测  类别class[%d]  置信度[%.2f] 烟雾置信度阈值[%.2f]<===================\n",
                               stResult.nClassId,
                               stResult.fBoxConfidence,
                               stInData.stParam.stSmokeFireDetectParam.fConfidence);
                    }
                    bDetectSmoke = true;
                    vecResult.push_back(stResult);
                }
            }
            continue;
        }

        if (stResult.nClassId == FIRE)
        {
            if (stInData.stParam.stOpenFlameParam.bEnable)
            {
                if (stResult.fBoxConfidence > stInData.stParam.stOpenFlameParam.fConfidence)
                {
                    if (access("/group3Debug", F_OK) == 0)
                    {
                        printf("===================>火焰检测  类别class[%d]  置信度[%.2f] 火焰检测置信度阈值[%.2f]<===================\n",
                               stResult.nClassId,
                               stResult.fBoxConfidence,
                               stInData.stParam.stOpenFlameParam.fConfidence);
                    }
                    bDetectFire = true;
                    vecResult.push_back(stResult);
                }
            }
            continue;
        }

        if (stResult.nClassId == GARBAGEEXPOSURE)
        {
            if (stInData.stParam.stGarbageExposureParam.bEnable)
            {
                if (stResult.fBoxConfidence > stInData.stParam.stGarbageExposureParam.fConfidence)
                {
                    if (access("/group3Debug", F_OK) == 0)
                    {
                        printf("===================>垃圾暴露检测  类别class[%d]  置信度[%.2f] 垃圾暴露置信度阈值[%.2f]<===================\n",
                               stResult.nClassId,
                               stResult.fBoxConfidence,
                               stInData.stParam.stGarbageExposureParam.fConfidence);
                    }
                    bDetectGarbageExposure = true;
                    vecResult.push_back(stResult);
                }
            }
            continue;
        }

        if (stResult.nClassId == GARBAGEOVER)
        {
            if (stInData.stParam.stGarbageOverParam.bEnable)
            {
                if (stResult.fBoxConfidence > stInData.stParam.stGarbageOverParam.fConfidence)
                {
                    if (access("/group3Debug", F_OK) == 0)
                    {
                        printf("===================>垃圾满溢检测  类别class[%d]  置信度[%.2f] 垃圾满溢置信度阈值[%.2f]<===================\n",
                               stResult.nClassId,
                               stResult.fBoxConfidence,
                               stInData.stParam.stGarbageOverParam.fConfidence);
                    }
                    bDetectGarbageOver = true;
                    vecResult.push_back(stResult);
                }
            }
            continue;
        }

        if (stResult.nClassId == DAMAGED || stResult.nClassId == LOST || stResult.nClassId == UNCOVERED || stResult.nClassId == BREAKOUTOFOUTEREDGE)
        {
            if (stInData.stParam.stManholeCoverAbnormalParam.bEnable)
            {
                if (stResult.fBoxConfidence > stInData.stParam.stManholeCoverAbnormalParam.fConfidence)
                {
                    if (access("/group3Debug", F_OK) == 0)
                    {
                        printf("===================>井盖异常检测  类别class[%d]  置信度[%.2f] 井盖异常置信度阈值[%.2f]<===================\n",
                               stResult.nClassId,
                               stResult.fBoxConfidence,
                               stInData.stParam.stManholeCoverAbnormalParam.fConfidence);
                    }
                    bDetectManholeCoverAbnormal = true;
                    vecResult.push_back(stResult);
                }
            }
            continue;
        }

        if (stResult.nClassId == WATERACCUMULATION)
        {
            if (stInData.stParam.stRoadPondingParam.bEnable)
            {
                if (stResult.fBoxConfidence > stInData.stParam.stRoadPondingParam.fConfidence)
                {
                    if (access("/group3Debug", F_OK) == 0)
                    {
                        printf("===================>道路积水检测  类别class[%d]  置信度[%.2f] 道路积水置信度阈值[%.2f]<===================\n",
                               stResult.nClassId,
                               stResult.fBoxConfidence,
                               stInData.stParam.stRoadPondingParam.fConfidence);
                    }
                    bDetectRoadPonding = true;
                    vecResult.push_back(stResult);
                }
            }
            continue;
        }
    }

    if (bDetectSmoke)
    {
        m_nSmokeFrameCount++;
        if (m_nSmokeFrameCount >= stInData.stParam.stSmokeFireDetectParam.nDetectFrame)
        {
            printf("【报警】识别到烟雾！ [%d]\n", m_nSmokeFrameCount);
            stOutData->bSmoke        = true;
            // m_nSmokeFrameCount = 0;
        }
    }
    else
    {
        m_nSmokeFrameCount = 0;
    }

    if (bDetectFire)
    {
        m_nFireFrameCount++;
        if (m_nFireFrameCount >= stInData.stParam.stOpenFlameParam.nDetectFrame)
        {
            printf("【报警】识别到了火焰！[%d]\n", m_nFireFrameCount);
            stOutData->bOpenFire  = true;
            // m_nFireFrameCount = 0;
        }
    }
    else
    {
        m_nFireFrameCount = 0;
    }

    if (bDetectGarbageExposure)
    {
        m_nGarbageExposureFrameCount++;
        if (m_nGarbageExposureFrameCount >= stInData.stParam.stGarbageExposureParam.nDetectFrame)
        {
            printf("【报警】识别到了垃圾暴露！[%d]\n", m_nGarbageExposureFrameCount);
            stOutData->bGarbageExposure  = true;
            // m_nGarbageExposureFrameCount = 0;
        }
    }
    else
    {
        m_nGarbageExposureFrameCount = 0;
    }

    if (bDetectGarbageOver)
    {
        m_nGarbageOverFrameCount++;
        if (m_nGarbageOverFrameCount >= stInData.stParam.stGarbageOverParam.nDetectFrame)
        {
            printf("【报警】识别到垃圾满溢！[%d]\n", m_nGarbageOverFrameCount);
            stOutData->bGarbageOver  = true;
            // m_nGarbageOverFrameCount = 0;
        }
    }
    else
    {
        m_nGarbageOverFrameCount = 0;
    }

    if (bDetectManholeCoverAbnormal)
    {
        m_nManholeCoverAbnormalFrameCount++;
        if (m_nManholeCoverAbnormalFrameCount >= stInData.stParam.stManholeCoverAbnormalParam.nDetectFrame)
        {
            printf("【报警】识别到了井盖异常！[%d]\n", m_nManholeCoverAbnormalFrameCount);
            stOutData->bManholeCoverAbnormal  = true;
            // m_nManholeCoverAbnormalFrameCount = 0;
        }
    }
    else
    {
        m_nManholeCoverAbnormalFrameCount = 0;
    }

    if (bDetectRoadPonding)
    {
        m_nRoadPondingFrameCount++;
        if (m_nRoadPondingFrameCount >= stInData.stParam.stRoadPondingParam.nDetectFrame)
        {
            printf("【报警】识别到了道路积水！[%d]\n", m_nRoadPondingFrameCount);
            stOutData->bRoadPonding  = true;
            // m_nRoadPondingFrameCount = 0;
        }
    }
    else
    {
        m_nRoadPondingFrameCount = 0;
    }

    return true;
}

/* 处理数据 */
bool Group3Detect_NS::CGroup3DetectV1_0::resizeAndPadImage(cv::Mat inputImage, cv::Mat &outputImage)
{
    int imageWidth = inputImage.cols;
    int imageHeight = inputImage.rows;

    int newWidth = 0;
    int newHeight = 0;
    
    cv::Mat resizedImage;
    if(imageWidth > m_nLimitWidth || imageHeight > m_nLimitHeight)
    {
        m_fResizeScale = static_cast<float>(m_nLimitWidth) / std::max(imageWidth, imageHeight);
        
        newWidth = static_cast<int>(imageWidth * m_fResizeScale);
        newHeight = static_cast<int>(imageHeight * m_fResizeScale);
        
        cv::resize(inputImage, resizedImage, cv::Size(newWidth, newHeight));

        m_nXOffset = static_cast<int>((m_nLimitWidth - newWidth) / 2);
        m_nYOffset = static_cast<int>((m_nLimitHeight - newHeight) / 2);
    }
    else
    {
        m_nXOffset = 0;
        m_nYOffset = 0;
        m_fResizeScale = 1.0;
        
        newWidth = imageWidth;
        newHeight = imageHeight;
        
        resizedImage = inputImage;
    }

    cv::Mat output = cv::Mat::zeros(cv::Size(m_nLimitWidth, m_nLimitHeight), inputImage.type());
    resizedImage.copyTo(output(cv::Rect(m_nXOffset, m_nYOffset, newWidth, newHeight)));
    
    outputImage = output;

    return true;
}