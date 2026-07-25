/*
 * @Author: 梁浩尧 lianghaoyao@kfb.cn
 * @Date: 2025-11-17 10:07:33
 * @LastEditors: lianghy lianghy@kfb.cn
 * @LastEditTime: 2026-04-07 14:32:57
 * @FilePath: /1126/share/ai_share/AiModules/Modules/ConstructionEncroachmentRoadDetect/V1_0/ConstructionEncroachmentRoadDetectV1_0.cpp
 * @Description: 施工占道检测
 */

#include "ConstructionEncroachmentRoadDetectV1_0.hpp"

#include "SaveImage.hpp"
#include "StatisticsTimer.hpp"
#include <unistd.h>

using namespace ConstructionEncroachmentRoadDetect_NS;

#define DETECT_TIMEOUT 10

ConstructionEncroachmentRoadDetect_NS::ConstructionEncroachmentRoadDetectV1_0::ConstructionEncroachmentRoadDetectV1_0(InParam_S stInParam)
    : m_stInParam(stInParam)
{
}

ConstructionEncroachmentRoadDetect_NS::ConstructionEncroachmentRoadDetectV1_0::~ConstructionEncroachmentRoadDetectV1_0()
{
    unInit();
}

/* 初始化 */
bool ConstructionEncroachmentRoadDetect_NS::ConstructionEncroachmentRoadDetectV1_0::init()
{
    CStatisticsTimer runTime("施工占道检测初始化耗时");
    bool bRet = false;

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
bool ConstructionEncroachmentRoadDetect_NS::ConstructionEncroachmentRoadDetectV1_0::unInit()
{
    CStatisticsTimer runTime("施工占道,检测反初始化耗时");
    if (m_pYoloUltralytics)
    {
        delete m_pYoloUltralytics;
        m_pYoloUltralytics = nullptr;
    }

    return true;
}

/* 处理数据 */
bool ConstructionEncroachmentRoadDetect_NS::ConstructionEncroachmentRoadDetectV1_0::process(
    InData_S stInData,
    std::vector<Result_S> &vecResult,
    OutData_S *pstOutData)
{
    OutData_S defaultOutData;

    // 如果传入的指针为空，则使用默认对象
    if (pstOutData == nullptr)
    {
        pstOutData = &defaultOutData;
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
    stInputData.pData = (float*)stInData.inMat.data;
    stInputData.nDataSize = static_cast<size_t>(stInData.inMat.total() * stInData.inMat.elemSize() * sizeof(float));
    stInputData.stBoxs.fConfidence = stInData.stParam.fBoxThreshold;
    stInputData.stBoxs.fNms = stInData.stParam.fNmsThreshold;

    /* 推理+后处理 */
    std::vector<Inference_NS::BoxData_S> vBoxDatas;
    {
        CStatisticsTimer runTime("推理耗时");
        bool bRet = m_pYoloUltralytics->inference(stInputData, vBoxDatas);
        if (!bRet)
        {
            printf("算法分析失败\n");
            return false;
        }
    }
	
	// 标记当前帧是否检测到施工占道
	bool bConstructionEncroachmentRoadDetect = false;

    if (access("/mnt/test_ConstructionEncroachmentRoadDetect", F_OK) == 0)
	{
        for (int i = 0; i < vBoxDatas.size(); i++)
        {
		
			printf("===================>施工占道  当前目标[%d] 类别class[%d]  置信度[%.2f] <===================\n",
			i + 1,vBoxDatas[i].nLabel, vBoxDatas[i].fConfidence);
	
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

            Modules_NS::saveImage(stInData.inMat, "/mnt/ConstructionEncroachmentRoad_test");
		
	    }	
    }

    for (const auto& box : vBoxDatas)
    {
		if ((box.nLabel == CONICAL_BARREL || box.nLabel == CRASH_BARREL) && box.fConfidence > stInData.stParam.stConstructionEncroachmentRoadParam.fConfidence) 
		{
            time_t nTime = time(NULL);
            /* 距离上一次成功检测到目标超过10s，则把检测到的次数清零 */
            if(nTime - m_nLastDetectTime > DETECT_TIMEOUT)
            {
                printf(" ====== 检测超时 (%ld s) == %d 帧\n", nTime - m_nLastDetectTime, m_nConstructionEncroachmentRoadFrameCount );
                m_nConstructionEncroachmentRoadFrameCount = 0;
            }
            m_nLastDetectTime = time(NULL);
            bConstructionEncroachmentRoadDetect = true;
            Result_S stResult;
            stResult.nType   = box.nLabel;
            stResult.fX      = box.stBoxs.nX1;
            stResult.fY      = box.stBoxs.nY1;
            stResult.fWidth  = box.stBoxs.nX2 - box.stBoxs.nX1;
            stResult.fHeight = box.stBoxs.nY2 - box.stBoxs.nY1;

            vecResult.push_back(stResult);
		}
    }

	if(stInData.stParam.stConstructionEncroachmentRoadParam.bEnable)
	{
		if (bConstructionEncroachmentRoadDetect) 
		{
            // printf(" ======= 检测到目标个数 %d ======= \n", m_nConstructionEncroachmentRoadFrameCount);
			m_nConstructionEncroachmentRoadFrameCount++; 
			if (m_nConstructionEncroachmentRoadFrameCount >= stInData.stParam.stConstructionEncroachmentRoadParam.nDetectFrame) 
			{
                pstOutData->bConstructionEncroachmentRoad = true;
                printf("【报警】连续%d帧检测到施工占道！\n", m_nConstructionEncroachmentRoadFrameCount);
                m_nConstructionEncroachmentRoadFrameCount = 0;
			}
		}
		
	}
	
    return true;
}

