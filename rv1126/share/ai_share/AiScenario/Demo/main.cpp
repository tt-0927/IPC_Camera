/*
 * @FilePath     : main.cpp
 * @Author       : 严泽辉 yanzeh@kfb.cn
 * @Date         : 2024-05-21 19:16:25
 * @LastEditors: 李辉 lihui@kfb.cn
 * @LastEditTime: 2024-10-31 16:22:31
 * @Description  :
 */

#include <errno.h>
#include <fcntl.h>     // 包含open函数所需的头文件
#include <unistd.h>    // 包含lseek、read和close函数所需的头文件
#include <chrono>
#include "BlError.h"
#include "dlog.h"
#include "Scenario.hpp"

int main(int argc, char const* argv[])
{

    char* pchOutData = nullptr;
    int   nDataSize  = 0;
	/* ------------------------------------------------ 模型初始化 ---------------------------------------------------------- */
    AiScenario_NS::InParam_S stInParam;
    stInParam.clear();
    
	/* ===================================== 表情识别算法V1_0测试 ======================================================== */
    // stInParam.stNeedParam.enType       = AiScenario_NS::FACE_EXPRESS;
    // stInParam.stNeedParam.enResultType = AiScenario_NS::JSON;

    // stInParam.stNeedParam.enVersions = AiScenario_NS::V1_0;
    // stInParam.stNeedParam.vstrModelPath.clear();
    // stInParam.stNeedParam.vstrModelPath.push_back("./weights/FaceDetect.rknn");
    // stInParam.stNeedParam.vstrModelPath.push_back("./weights/Expression.rknn");

    // Scenario_NS::CScenarioBase* StudentExpressV1 = AiScenario_NS::CScenario::create(stInParam);
    // if (!StudentExpressV1)
    // {
    // 	std::cerr << "构建推理对象失败"<< std::endl;
    //     return -1;
    // }
    // bool bInit = StudentExpressV1->init();
	// if (!bInit)
    // {
    // 	std::cerr << "模型初始化失败"<< std::endl;
    //     return -1;
    // }
    // /* ================================================== 表情识别算法V2_0测试 ======================================================== */
    // stInParam.stNeedParam.enType       = AiScenario_NS::FACE_EXPRESS;
    // stInParam.stNeedParam.enResultType = AiScenario_NS::JSON;

    // stInParam.stNeedParam.enVersions = AiScenario_NS::V2_0;
    // stInParam.stNeedParam.vstrModelPath.clear();
    // stInParam.stNeedParam.vstrModelPath.push_back("./weights/MB_FaceDetector1920x1024.rknn");
    // stInParam.stNeedParam.vstrModelPath.push_back("./weights/Expression.rknn");

    // Scenario_NS::CScenarioBase* StudentExpressV2 = AiScenario_NS::CScenario::create(stInParam);
    // if (!StudentExpressV2)
    // {
    // 	std::cerr << "构建推理对象失败"<< std::endl;
    //     return -1;
    // }
    // bInit = StudentExpressV2->init();
	// if (!bInit)
    // {
    // 	std::cerr << "模型初始化失败"<< std::endl;
    //     return -1;
    // }
    
    // /* ================================================== 学生行为分析算法V1_0测试 ======================================================== */
    // stInParam.stNeedParam.enType     = AiScenario_NS::STUDENT_BEHAVIOR;
    // stInParam.stNeedParam.enVersions = AiScenario_NS::V1_0;
    // stInParam.stNeedParam.vstrModelPath.clear();
    // stInParam.stNeedParam.vstrModelPath.push_back("./weights/StudentBehaviorV2_1.rknn");

    // Scenario_NS::CScenarioBase* StudentBehaviorV1 = AiScenario_NS::CScenario::create(stInParam);
    // if (!StudentBehaviorV1)
    // {
    //     return -1;
    // }

    // StudentBehaviorV1->init();
    
    // /* ================================================== 学生行为分析算法V2_0测试 ======================================================== */
    // stInParam.stNeedParam.enType       = AiScenario_NS::STUDENT_BEHAVIOR;
    // stInParam.stNeedParam.enResultType = AiScenario_NS::JSON;

    // stInParam.stNeedParam.enVersions = AiScenario_NS::V2_0;
    // stInParam.stNeedParam.vstrModelPath.clear();
    // stInParam.stNeedParam.vstrModelPath.push_back("./weights/HeadDetect.rknn");
    // stInParam.stNeedParam.vstrModelPath.push_back("./weights/Fastpose.rknn");

    // Scenario_NS::CScenarioBase* StudentBehaviorV2 = AiScenario_NS::CScenario::create(stInParam); 
    // if (!StudentBehaviorV2)
    // {
    // 	std::cerr << "构建推理对象失败"<< std::endl;
    //     return -1;
    // }
    // bInit = StudentBehaviorV2->init();
	// if (!bInit)
    // {
    // 	std::cerr << "模型初始化失败"<< std::endl;
    //     return -1;
    // }
    
    // /* ================================================== 人数统计V1_0测试 ======================================================== */
    // stInParam.stNeedParam.enType       = AiScenario_NS::HUMAN_CUTOUT;
    // stInParam.stNeedParam.enResultType = AiScenario_NS::JSON;

    // stInParam.stNeedParam.enVersions = AiScenario_NS::V1_0;
    // stInParam.stNeedParam.vstrModelPath.clear();
    // stInParam.stNeedParam.vstrModelPath.push_back("./weights/HR_human768x640_NWPU.rknn");  

    // Scenario_NS::CScenarioBase* pHumanCount1 = AiScenario_NS::CScenario::create(stInParam);
    // if (!pHumanCount1)
    // {
    //     return -1;
    // }

    // pHumanCount1->init();

	// /* ================================================== 人数统计V2_0测试 ======================================================== */
    // stInParam.stNeedParam.enType       = AiScenario_NS::HUMAN_CUTOUT;
    // stInParam.stNeedParam.enResultType = AiScenario_NS::JSON;
    // stInParam.stNeedParam.enVersions   = AiScenario_NS::V2_0;
    // stInParam.stNeedParam.vstrModelPath.clear();
    // stInParam.stNeedParam.vstrModelPath.push_back("./HeadDetect.rknn");

    // Scenario_NS::CScenarioBase* pHumanCount2 = AiScenario_NS::CScenario::create(stInParam);
    // if (!pHumanCount2)
    // {
    //     return -1;
    // }

    // pHumanCount2->init();

	/* ================================================== 跟踪算法V1_0测试 ======================================================== */
    // stInParam.stNeedParam.enType       = AiScenario_NS::TRACKER;
    // stInParam.stNeedParam.enResultType = AiScenario_NS::JSON;

    // stInParam.stNeedParam.enVersions = AiScenario_NS::V1_0;
    // stInParam.stNeedParam.vstrModelPath.clear();
    // stInParam.stNeedParam.vstrModelPath.push_back("./weights/HeadDetect.rknn");
    // stInParam.stNeedParam.vstrModelPath.push_back("./weights/Deepsort_facenet.rknn");

    // Scenario_NS::CScenarioBase* pTracker = AiScenario_NS::CScenario::create(stInParam);  
    // if (!pTracker)
    // {
    //     return -1;
    // }

    // pTracker->init();
    
    // /* ================================================== 人脸识别V1_0测试 ======================================================== */
    // stInParam.stNeedParam.enType       = AiScenario_NS::FACR_RECT;
    // stInParam.stNeedParam.enResultType = AiScenario_NS::JSON;

    // stInParam.stNeedParam.enVersions = AiScenario_NS::V1_0;
    // stInParam.stNeedParam.vstrModelPath.clear();
    // stInParam.stNeedParam.vstrModelPath.push_back("./weights/MB_FaceDetector1920x1024.rknn");
    // stInParam.stNeedParam.vstrModelPath.push_back("./weights/MB_facenet160x160.rknn");

    // Scenario_NS::CScenarioBase* pFace = AiScenario_NS::CScenario::create(stInParam);
    // if (!pFace)
    // {
    //     return -1;
    // }

    // pFace->init();
    
    // /* ================================================== 黑底白数字识别V1_0测试 ======================================================== */
    // stInParam.stNeedParam.enType       = AiScenario_NS::NUMBER_OCR;
    // stInParam.stNeedParam.enResultType = AiScenario_NS::JSON;

    // stInParam.stNeedParam.enVersions = AiScenario_NS::V1_0;
    // stInParam.stNeedParam.vstrModelPath.clear();
    // stInParam.stNeedParam.vstrModelPath.push_back("./weights/NumberOcr.rknn");

    // Scenario_NS::CScenarioBase* pNumberOcr = AiScenario_NS::CScenario::create(stInParam);
    // if (!pFace)
    // {
    //     return -1;
    // }

    // pNumberOcr->init();    

    /* ================================================== 行人区域检测测试 ======================================================== */
    stInParam.stNeedParam.enType       = AiScenario_NS::HUMAN_AREA;
    stInParam.stNeedParam.enResultType = AiScenario_NS::JSON;
    stInParam.stNeedParam.enVersions   = AiScenario_NS::V1_0;
    stInParam.stNeedParam.vstrModelPath.clear();
    stInParam.stNeedParam.vstrModelPath.push_back("./HumanDetect.rknn");

    Scenario_NS::CScenarioBase* pHumanArea = AiScenario_NS::CScenario::create(stInParam);
    if (!pHumanArea)
    {
        return -1;
    }

    pHumanArea->init();
    
    /* ------------------------------------------------ 模型初始化结束 ---------------------------------------------------------- */

    /* 使用cv::imread函数打开图像 */
    cv::Mat image = cv::imread("./1.png", cv::IMREAD_COLOR);

    /* 检查图像是否成功加载 */
    if (image.empty())
    {
        std::cerr << "Could not read the image: " << "./1.png" << std::endl;
        return 1;
    }
    AiScenario_NS::CVData_S stInData;
    stInData.inMat = image;
    
    /* ------------------------------------------------ AI推理 ---------------------------------------------------------- */
     std::chrono::high_resolution_clock::time_point start, end;
     std::chrono::milliseconds duration;

	// /* 学生表情识别V1模型推理以及计算程序运行时间 */
    // start = std::chrono::high_resolution_clock::now();
    // StudentExpressV1->process(stInData, pchOutData, nDataSize);
	// end = std::chrono::high_resolution_clock::now();
	// duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
	// std::cout << "学生表情识别V1总共推理耗时: " << duration.count() << " ms" << std::endl;
    // dlog(LOG_INFO, "StudentExpressV1 = %s", pchOutData);
    // StudentExpressV1->releaseData(pchOutData);

	// /* 模型推理以及计算程序运行时间 */
    // start = std::chrono::high_resolution_clock::now();
    // StudentExpressV2->process(stInData, pchOutData, nDataSize);
	// end = std::chrono::high_resolution_clock::now();
	// duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
	// std::cout << "学生表情识别V2总共推理耗时: " << duration.count() << " ms" << std::endl;
    // dlog(LOG_INFO, "StudentExpressV2 = %s", pchOutData);
    // StudentExpressV2->releaseData(pchOutData);	

	/* 学生行为分析V1模型推理以及计算程序运行时间 */
    // start = std::chrono::high_resolution_clock::now();
    // StudentBehaviorV1->process(stInData, pchOutData, nDataSize);
	// end = std::chrono::high_resolution_clock::now();
	// /* 学生行为分析V2模型推理以及计算程序运行时间 */
    // start = std::chrono::high_resolution_clock::now();
    // StudentBehaviorV2->process(stInData, pchOutData);
	// end = std::chrono::high_resolution_clock::now();
	// duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
	// std::cout << "学生行为分析V2总共推理耗时: " << duration.count() << " ms" << std::endl;
    // dlog(LOG_INFO, "StudentBehaviorV2 = %s", pchOutData);
    // StudentBehaviorV2->releaseData(pchOutData);    
    
	// /* 人数统计V1模型推理以及计算程序运行时间 */
    // start = std::chrono::high_resolution_clock::now();
    // pHumanCount1->process(stInData, pchOutData);
	// end = std::chrono::high_resolution_clock::now();
	// duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
	// std::cout << "人数统计V1总共推理耗时: " << duration.count() << " ms" << std::endl;
    // dlog(LOG_INFO, "pHumanCount1 = %s", pchOutData);
    // pHumanCount1->releaseData(pchOutData);    
    
	// /* 人数统计V2模型推理以及计算程序运行时间 */
    // start = std::chrono::high_resolution_clock::now();
    // pHumanCount2->process(stInData, pchOutData);
	// end = std::chrono::high_resolution_clock::now();
	// duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
	// std::cout << "人数统计V2总共推理耗时: " << duration.count() << " ms" << std::endl;
    // dlog(LOG_INFO, "pHumanCount2 = %s", pchOutData);
    // pHumanCount2->releaseData(pchOutData);    
    
	// /* 摄像头跟踪模型推理以及计算程序运行时间 */
    // start = std::chrono::high_resolution_clock::now();
    // pTracker->process(stInData, pchOutData);
	// end = std::chrono::high_resolution_clock::now();
	// duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
	// std::cout << "摄像头跟踪总共推理耗时: " << duration.count() << " ms" << std::endl;
    // dlog(LOG_INFO, "pTracker = %s", pchOutData);
    // pTracker->releaseData(pchOutData);
    
	// /* 人脸识别模型推理以及计算程序运行时间 */
    // start = std::chrono::high_resolution_clock::now();
    // pFace->process(stInData, pchOutData);
	// end = std::chrono::high_resolution_clock::now();
	// duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
	// std::cout << "人脸识别总共推理耗时: " << duration.count() << " ms" << std::endl;
    // dlog(LOG_INFO, "pFace = %s", pchOutData);
    // pFace->releaseData(pchOutData);    
    
	// /* 黑底白数字识别模型推理以及计算程序运行时间 */
    // start = std::chrono::high_resolution_clock::now();
    // pNumberOcr->process(stInData, pchOutData);
	// end = std::chrono::high_resolution_clock::now();
	// duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
	// std::cout << "黑底白数字识别总共推理耗时: " << duration.count() << " ms" << std::endl;
    // dlog(LOG_INFO, "pNumberOcr = %s", pchOutData);
    // pFace->releaseData(pchOutData); 

    /* 行人区域检测模型推理以及计算程序运行时间 */
    start = std::chrono::high_resolution_clock::now();
    pHumanArea->process(stInData, pchOutData);
	end = std::chrono::high_resolution_clock::now();
	duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
	std::cout << "行人区域检测总共推理耗时: " << duration.count() << " ms" << std::endl;
    dlog(LOG_INFO, "pHumanArea = %s", pchOutData);
    pHumanArea->releaseData(pchOutData);    
    
  
EXIT:
    // if (StudentExpressV1)
    // {
    //     AiScenario_NS::CScenario::release(StudentExpressV1);
    // }
    // if (StudentExpressV2)
    // {
    //     AiScenario_NS::CScenario::release(StudentExpressV2);
    // }    
    // if (StudentBehaviorV1)
    // {
    //     AiScenario_NS::CScenario::release(StudentBehaviorV1);
    // }    
    // if (StudentBehaviorV2)
    // {
    //     AiScenario_NS::CScenario::release(StudentBehaviorV2);
    // }    
    // if (pHumanCount1)
    // {
    //     AiScenario_NS::CScenario::release(pHumanCount1);
    // }    
    // if (pHumanCount2)
    // {
    //     AiScenario_NS::CScenario::release(pHumanCount2);
    // }    
    // if (pTracker)
    // {
    //     AiScenario_NS::CScenario::release(pTracker);
    // }     
    // if (pFace)
    // {
    //     AiScenario_NS::CScenario::release(pFace);
 	// }
 	// if (pNumberOcr)
    // {
    //     AiScenario_NS::CScenario::release(pNumberOcr);
 	// }
    if (pHumanArea)
    {
        AiScenario_NS::CScenario::release(pHumanArea);
    }   
    
    return 0;
}
