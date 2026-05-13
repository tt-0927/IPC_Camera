/**
 * @file SmokingDetectV1_0.cpp
 * @author tianl (tianl@kfb.cn)
 * @date 2025-11-13
 * 
 * @brief 抽烟识别
 */
#include "SmokingDetectV1_0.hpp"

#include "BYTETracker.h"
#include "SaveImage.hpp"
#include "StatisticsTimer.hpp"
#include <unistd.h>

using namespace SmokingDetect_NS;

SmokingDetect_NS::CSmokingDetectV1_0::CSmokingDetectV1_0(InParam_S stInParam)
    : m_stInParam(stInParam)
{
}

SmokingDetect_NS::CSmokingDetectV1_0::~CSmokingDetectV1_0()
{
    unInit();
}

/* 初始化 */
bool SmokingDetect_NS::CSmokingDetectV1_0::init()
{
    CStatisticsTimer runTime("抽烟检测初始化耗时");
    bool bRet = false;

    m_pYolov5 = new Inference_NS::CYolov5(m_stInParam.strModelPath);
    if (m_pYolov5 && m_pYolov5->init())
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
bool SmokingDetect_NS::CSmokingDetectV1_0::unInit()
{
    CStatisticsTimer runTime("抽烟检测反初始化耗时");
    if (m_pYolov5)
    {
        delete m_pYolov5;
        m_pYolov5 = nullptr;
    }

    return true;
}

/* 处理数据 */
bool SmokingDetect_NS::CSmokingDetectV1_0::process(
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

    if (!m_pYolov5)
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
    stInputData.nDataSize = static_cast<size_t>(stInData.inMat.total() * stInData.inMat.elemSize());
    stInputData.stBoxs.fConfidence = stInData.stParam.fBoxThreshold;
    stInputData.stBoxs.fNms = stInData.stParam.fNmsThreshold;

    /* 推理+后处理 */
    std::vector<Inference_NS::BoxData_S> vBoxDatas;
    {
        CStatisticsTimer runTime("推理耗时");
        bool bRet = m_pYolov5->inference(stInputData, vBoxDatas);
        if (!bRet)
        {
            printf("算法分析失败\n");
            return false;
        }
    }
	
    if (access("/test_SmokingDetect", F_OK) == 0)
	{
        for (int i = 0; i < vBoxDatas.size(); i++)
        {
		
            printf("===================>抽烟识别  当前目标[%d] 类别class[%d]  置信度[%.2f] <===================\n",
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

            // Modules_NS::saveImage(stInData.inMat, "/mnt/test_SmokingDetect");
		
	    }	
    }
    
	// 标记当前帧是否识别到了抽烟
	bool bDetectSmoking = false;
    for (const auto& box : vBoxDatas)
    {
        DetectResult_S result;
        result.fConfidence = box.fConfidence;
        result.nClassId = box.nLabel;
        // printf("===================>抽烟检测  类别class[%d]  置信度[%.2f] 抽烟置信度阈值[%.2f]<===================\n",
        // result.nClassId,result.fConfidence,stInData.stParam.stSmokingParam.fConfidence);
		if (result.fConfidence > stInData.stParam.stSmokingParam.fConfidence) 
		{
			bDetectSmoking = true; 
		} 
    }
	int enType = 0;
	if(stInData.stParam.stSmokingParam.bEnable)
	{
		if (bDetectSmoking) 
		{
            m_nSmokingFrameCount++;
            if (m_nSmokingFrameCount >= stInData.stParam.stSmokingParam.nDetectFrame) 
            {
                Result_S stResult;
                stResult.bSmoking = true;
                vecResult.push_back(stResult);

                printf("【报警】识别到了抽烟！\n");
                enType |= 0x02;
                m_nSmokingFrameCount = 0;
            }
		} 
		
	}

	if(!vecResult.empty())
	{
		stOutData->validResult = true;
		stOutData->nType = enType;
	}
	
    return true;
}

