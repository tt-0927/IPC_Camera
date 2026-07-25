/*
 * @Author: 梁浩尧 lianghaoyao@kfb.cn
 * @Date: 2025-12-24 16:23:34
 * @LastEditors: lianghy lianghy@kfb.cn
 * @LastEditTime: 2026-04-07 14:36:05
 * @FilePath: /1126/share/ai_share/AiModules/Modules/Group5Detect/V1_0/Group5DetectV1_0.hpp
 * @Description: metalFence(金属栅栏)、ConeTank(锥形桶)、CrashBarrels(防撞桶)、fence(防护栏)
 */

#include "BYTETracker.h"
#include "Group5DetectV1_0.hpp"
#include "SaveImage.hpp"
#include "StatisticsTimer.hpp"

#include <unistd.h>

using namespace Group5Detect_NS;

Group5Detect_NS::CGroup5DetectV1_0::CGroup5DetectV1_0(InParam_S stInParam)
    : m_stInParam(stInParam)
{
}

Group5Detect_NS::CGroup5DetectV1_0::~CGroup5DetectV1_0()
{
    unInit();
}

/* 初始化 */
bool Group5Detect_NS::CGroup5DetectV1_0::init()
{
    CStatisticsTimer runTime("模型组合5检测初始化耗时");
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
bool Group5Detect_NS::CGroup5DetectV1_0::unInit()
{
    CStatisticsTimer runTime("模型组合5检测反初始化耗时");
    if (m_pYoloUltralytics)
    {
        delete m_pYoloUltralytics;
        m_pYoloUltralytics = nullptr;
    }

    return true;
}

/* 处理数据 */
bool Group5Detect_NS::CGroup5DetectV1_0::process(
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

    if (access("/test_Group5Detect", F_OK) == 0)
    {
        for (int i = 0; i < vBoxDatas.size(); i++)
        {

            printf("===================> 模型组5  当前目标[%d] 类别class[%d]  置信度[%.2f] <===================\n",
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

            Modules_NS::saveImage(stInData.inMat, "/mnt/Group5Detect_test");
        }
    }

    /* 标记当前帧是否识别到了防护栏 */
    bool bDetectHoleProtectionBar = false;
    /* 标记当前帧是否识别到了防撞桶、锥形桶 */
    bool bDetectConstructionEncroachmentRoad = false;

    for (const auto &box : vBoxDatas)
    {
        Result_S stResult;
        stResult.fBoxConfidence = box.fConfidence;
        stResult.fX1            = box.stBoxs.nX1;
        stResult.fX2            = box.stBoxs.nX2;
        stResult.fY1            = box.stBoxs.nY1;
        stResult.fY2            = box.stBoxs.nY2;
        stResult.nClassId       = box.nLabel;

        if (access("/group5Debug", F_OK) == 0)
        {
            printf("===================>模型组合1检测  类别class[%d]  置信度[%.2f]<===================\n", stResult.nClassId, stResult.fBoxConfidence);
        }

        if ((stResult.nClassId == METALFENCE || stResult.nClassId == FENCE))
        {
            if (stInData.stParam.stHoleProtectionBarDetectParam.bEnable)
            {
                if (stResult.fBoxConfidence >= stInData.stParam.stHoleProtectionBarDetectParam.fConfidence)
                {
                    if (access("/group5Debug", F_OK) == 0)
                    {
                        printf("===================>防护栏检测  类别class[%d]  置信度[%.2f] 防护栏置信度阈值[%.2f]<===================\n",
                               stResult.nClassId,
                               stResult.fBoxConfidence,
                               stInData.stParam.stHoleProtectionBarDetectParam.fConfidence);
                    }
                    bDetectHoleProtectionBar = true;
                    vecResult.push_back(stResult);
                }
            }
            continue;
        }

        if (stResult.nClassId == CONETANK || stResult.nClassId == CRASHBARRELS)
        {
            if (stInData.stParam.stConstructionEncroachmentRoad.bEnable)
            {
                if (stResult.fBoxConfidence >= stInData.stParam.stConstructionEncroachmentRoad.fConfidence)
                {
                    if (access("/group5Debug", F_OK) == 0)
                    {
                        printf("===================>施工占道检测  类别class[%d]  置信度[%.2f] 施工占道置信度阈值[%.2f]<===================\n",
                               stResult.nClassId,
                               stResult.fBoxConfidence,
                               stInData.stParam.stConstructionEncroachmentRoad.fConfidence);
                    }
                    bDetectConstructionEncroachmentRoad = true;
                    vecResult.push_back(stResult);
                }
            }
            continue;
        }
    }

    if (bDetectHoleProtectionBar)
    {
        m_nHoleProtectionBarDetectFrameCount++;
        if (m_nHoleProtectionBarDetectFrameCount >= stInData.stParam.stHoleProtectionBarDetectParam.nDetectFrame)
        {
            printf("【报警】识别到了防护栏！\n");
            stOutData->bHoleProtectionBar        = true;
            // m_nHoleProtectionBarDetectFrameCount = 0;
        }
    }
    else
    {
        m_nHoleProtectionBarDetectFrameCount = 0;
    }

    if (bDetectConstructionEncroachmentRoad)
    {
        m_nConstructionEncroachmentRoadFrameCount++;
        if (m_nConstructionEncroachmentRoadFrameCount >= stInData.stParam.stHoleProtectionBarDetectParam.nDetectFrame)
        {
            printf("【报警】识别到了施工占道！\n");
            stOutData->bConstructionEncroachmentRoad  = true;
            // m_nConstructionEncroachmentRoadFrameCount = 0;
        }
    }
    else
    {
        m_nConstructionEncroachmentRoadFrameCount = 0;
    }

    return true;
}

/* 处理数据 */
bool Group5Detect_NS::CGroup5DetectV1_0::resizeAndPadImage(cv::Mat inputImage, cv::Mat &outputImage)
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