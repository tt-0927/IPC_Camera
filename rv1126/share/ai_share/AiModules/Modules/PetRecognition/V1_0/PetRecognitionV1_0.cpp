/*
 * @Author: 梁浩尧 lianghaoyao@kfb.cn
 * @Date: 2025-11-24 14:39:18
 * @LastEditors: lianghy lianghy@kfb.cn
 * @LastEditTime: 2026-04-07 14:37:06
 * @FilePath: /1126/share/ai_share/AiModules/Modules/PetRecognition/V1_0/PetRecognitionV1_0.cpp
 * @Description: 宠物识别
 */

#include "PetRecognitionV1_0.hpp"
#include "SaveImage.hpp"
#include "StatisticsTimer.hpp"
#include <unistd.h>

using namespace PetRecognition_NS;

PetRecognition_NS::CPetRecognitionV1_0::CPetRecognitionV1_0(InParam_S stInParam)
    : m_stInParam(stInParam)
{
}

PetRecognition_NS::CPetRecognitionV1_0::~CPetRecognitionV1_0()
{
    unInit();
}

/* 初始化 */
bool PetRecognition_NS::CPetRecognitionV1_0::init()
{
    CStatisticsTimer runTime("宠物识别检测初始化耗时");
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
bool PetRecognition_NS::CPetRecognitionV1_0::unInit()
{
    CStatisticsTimer runTime("宠物识别检测反初始化耗时");
    if (m_pYoloUltralytics)
    {
        delete m_pYoloUltralytics;
        m_pYoloUltralytics = nullptr;
    }

    return true;
}

/* 处理数据 */
bool PetRecognition_NS::CPetRecognitionV1_0::process(InData_S stInData, std::vector<Result_S> &vecResult)
{
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
    stInputData.stBoxs.fNms = stInData.stParam.fNmsThreshold;

    /* 推理+后处理 */
    std::vector<Inference_NS::BoxData_S> vBoxDatas;
    {
        CStatisticsTimer runTime("推理耗时");
        bool bRet = m_pYoloUltralytics->inference(stInputData, vBoxDatas);
        if (!bRet)
        {
            printf("宠物识别算法分析失败\n");
            return false;
        }
    }
	
    if (access("/test_PetRecognition", F_OK) == 0)
	{
        for (int i = 0; i < vBoxDatas.size(); i++)
        {
		
            printf("===================>宠物识别检测  当前目标[%d] 类别class[%d]  置信度[%.2f] <===================\n",
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

            Modules_NS::saveImage(stInData.inMat, "/mnt/PetRecognition_test");
		
	    }	
    }
    
    for (const auto& box : vBoxDatas)
    {
        printf("===================> 宠物识别检测  类别class[%d]  置信度[%.2f] 宠物识别置信度阈值[%.2f] <===================\n",
        box.nLabel, box.fConfidence, stInData.stParam.stPetRecognitionParam.fConfidence);
		if (box.fConfidence > stInData.stParam.stPetRecognitionParam.fConfidence)
		{
            Result_S result;
            result.nClassId = box.nLabel;
            result.fX1 = box.stBoxs.nX1;
            result.fY1 = box.stBoxs.nY1;
            result.fX2 = box.stBoxs.nX2;
            result.fY2 = box.stBoxs.nY2;
            // result.fConfidence = box.fConfidence;
			vecResult.push_back(result);
		} 
    }

    return true;
}

/* 处理数据 */
bool PetRecognition_NS::CPetRecognitionV1_0::resizeAndPadImage(cv::Mat inputImage, cv::Mat &outputImage)
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