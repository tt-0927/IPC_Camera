/**
 * @file RubishDetectV1_0.cpp
 * @author tianl (tianl@kfb.cn)
 * @date 2025-11-13
 * 
 * @brief 垃圾检测
 */
#include "RubishDetectV1_0.hpp"

#include "BYTETracker.h"
#include "SaveImage.hpp"
#include "StatisticsTimer.hpp"
#include <unistd.h>

using namespace RubishDetect_NS;

RubishDetect_NS::CRubishDetectV1_0::CRubishDetectV1_0(InParam_S stInParam)
    : m_stInParam(stInParam)
{
}

RubishDetect_NS::CRubishDetectV1_0::~CRubishDetectV1_0()
{
    unInit();
}

/* 初始化 */
bool RubishDetect_NS::CRubishDetectV1_0::init()
{
    CStatisticsTimer runTime("垃圾检测初始化耗时");
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
bool RubishDetect_NS::CRubishDetectV1_0::unInit()
{
    CStatisticsTimer runTime("垃圾检测反初始化耗时");
    if (m_pYoloUltralytics)
    {
        delete m_pYoloUltralytics;
        m_pYoloUltralytics = nullptr;
    }

    return true;
}

/* 处理数据 */
bool RubishDetect_NS::CRubishDetectV1_0::process(
    InData_S stInData,
    std::vector<Result_S> &vecResult,
    OutData_S *stOutData)
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
	
    if (access("/test_rubishdetect", F_OK) == 0)
	{
        for (int i = 0; i < vBoxDatas.size(); i++)
        {
		
            printf("===================>垃圾检测  当前目标[%d] 类别class[%d]  置信度[%.2f] <===================\n",
            i + 1,vBoxDatas[i].nLabel, vBoxDatas[i].fConfidence);
        
            // std::vector<cv::Point> rectPoints = {
            //                                 cv::Point(vBoxDatas[i].stBoxs.nX1,vBoxDatas[i].stBoxs.nY1), // 左上角
            //                                 cv::Point(vBoxDatas[i].stBoxs.nY2, vBoxDatas[i].stBoxs.nY1), // 右上角
            //                                 cv::Point(vBoxDatas[i].stBoxs.nY2, vBoxDatas[i].stBoxs.nX2), // 右下角
            //                                 cv::Point(vBoxDatas[i].stBoxs.nX1, vBoxDatas[i].stBoxs.nX2)  // 左下角
            //                             };
            // cv::polylines(stInData.inMat, 
            //             rectPoints, 
            //             true,
            //             cv::Scalar(0, 255, 0), /* 边框颜色(绿色) */
            //             2,
            //             cv::LINE_AA);

            // Modules_NS::saveImage(stInData.inMat, "/mnt/stree_test");
		
	    }	
    }
    
	// 标记当前帧是否检测到垃圾暴露或垃圾溢出
	bool bDetectOverflow = false;
	bool bDetectExposure = false;
    for (const auto& box : vBoxDatas)
    {
        DetectResult_S result;
        result.fConfidence = box.fConfidence;
        result.nClassId = box.nLabel;
         //printf("===================>垃圾检测  类别class[%d]  置信度[%.2f] 垃圾暴露置信度阈值[%.2f] 垃圾满溢置信度阈值[%.2f]<===================\n",
         //result.nClassId,result.fConfidence,stInData.stParam.stGarbageOverflowParam.fConfidence,stInData.stParam.stGarbageOverflowParam.fConfidence);
		if (result.nClassId == OVERFLOW && result.fConfidence > stInData.stParam.stGarbageOverflowParam.fConfidence) 
		{
			bDetectOverflow = true; 
		} 
		else if (result.nClassId == EXPOSURE && result.fConfidence > stInData.stParam.stGarbageExposureParam.fConfidence) 
		{
			bDetectExposure = true;
		}

    }
	int enType = 0;
	if(stInData.stParam.stGarbageOverflowParam.bEnable)
	{
		if (bDetectOverflow) 
		{
            Result_S stResult;
            stResult.bGarbageOverflow = true;
            vecResult.push_back(stResult);

            //printf("【报警】检测到垃圾满溢！\n");
            enType |= 0x02;
			
		} 
		
	}
	
	if(stInData.stParam.stGarbageExposureParam.bEnable)
	{
		if (bDetectExposure) 
		{
			
            Result_S stResult;
            stResult.bGarbageExposure = true;
            vecResult.push_back(stResult);

            //printf("【报警】检测到垃圾暴露！\n");
            enType |= 0x04;
			
		} 
	}
	
	if(!vecResult.empty())
	{
		stOutData->validResult = true;
		stOutData->nType = enType;
	}
	
    return true;
}

