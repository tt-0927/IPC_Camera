/*
 * @FilePath     : HumanAreaDetectV3_0.cpp
 * @Author       : 廖尔涛 liaoet@kfb.cn
 * @Date         : 2024-09-23 20:19:15
 * @LastEditors: 梁浩尧 lianghaoyao@kfb.cn
 * @LastEditTime: 2025-12-11 14:34:27
 * @Description  : 人少场景
 */
#include "HumanAreaDetectV3_0.hpp"

#include "BYTETracker.h"
#include "SaveImage.hpp"
#include "StatisticsTimer.hpp"


using namespace HumanAreaDetect_NS;

HumanAreaDetect_NS::CHumanAreaDetectV3_0::CHumanAreaDetectV3_0(InParam_S stInParam)
    : m_stInParam(stInParam)
{
}

HumanAreaDetect_NS::CHumanAreaDetectV3_0::~CHumanAreaDetectV3_0()
{
    unInit();
}

/* 初始化 */
bool HumanAreaDetect_NS::CHumanAreaDetectV3_0::init()
{
    CStatisticsTimer runTime("边界检测初始化耗时");
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

    m_pByteTracker = new Inference_NS::cBYTETracker();
    if (m_pByteTracker)
    {
        m_pByteTracker->setValue(m_fTrackThresh, m_fHighThresh, m_fMatchThresh, m_nFrameId, m_nMaxTimeLost);
        bRet = true;
    }

    if (!bRet)
    {
        printf("跟踪算法初始化失败\n");
        goto FAIL;
    }

    return bRet;

FAIL:
    unInit();
    return false;
}

/* 反初始化 */
bool HumanAreaDetect_NS::CHumanAreaDetectV3_0::unInit()
{
    CStatisticsTimer runTime("边界检测反初始化耗时");
    if (m_pYoloUltralytics)
    {
        delete m_pYoloUltralytics;
        m_pYoloUltralytics = nullptr;
    }

    if (m_pByteTracker)
    {
        delete m_pByteTracker;
        m_pByteTracker = nullptr;
    }

    return true;
}

/* 处理数据 */
bool HumanAreaDetect_NS::CHumanAreaDetectV3_0::process(
    InData_S stInData,
    std::vector<Result_S> &vecResult,
    std::vector<Result_S>& vstModelDetectResult,
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

    if (!m_pYoloUltralytics || !m_pByteTracker)
    {
        printf("未初始化算法类\n");
        return false;
    }

    bool bRet = m_pYoloUltralytics->setParam(stInData.stParam.fBoxThreshold, stInData.stParam.fNmsThreshold);
    if (!bRet)
    {
        printf("阈值参数设置错误，应该在0~1之间！！\n");
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
        bRet = m_pYoloUltralytics->inference(stInputData, vBoxDatas);
        if (!bRet)
        {
            printf("算法分析失败\n");
            return false;
        }
    }

    /* ***************** 人员聚集侦测 ***************** */
    Result_S stCrowdGatheringDetResult;
    bool bCrowdGatheringDetParamResult = false;
    int nCount = 0;
    for (auto &Param : stInData.stParam.vstCrowdGatheringDetParam)
    {
        if(Param.bEnable)
        {
            int nProportionThreshold = 0;
            for(auto &vBoxData:vBoxDatas)
            {
                /* 中心点 */
                cv::Point centerPoint(static_cast<int>(((vBoxData.stBoxs.nX1 + vBoxData.stBoxs.nX2) / 2)),
                                    static_cast<int>(((vBoxData.stBoxs.nY1 + vBoxData.stBoxs.nY2) / 2)));
                bool bIntrusionFlag = intrusionZoneDetection(centerPoint, Param.vecPoints);
                if(bIntrusionFlag)
                {
                    stCrowdGatheringDetResult.nId = -1;
                    stCrowdGatheringDetResult.fX = vBoxData.stBoxs.nX1;
                    stCrowdGatheringDetResult.fY = vBoxData.stBoxs.nY1;
                    stCrowdGatheringDetResult.fWidth = vBoxData.stBoxs.nX2 - vBoxData.stBoxs.nX1;
                    stCrowdGatheringDetResult.fHeight = vBoxData.stBoxs.nY2 - vBoxData.stBoxs.nY1;
                    stCrowdGatheringDetResult.bCrowdGatheringDetParamFlag = true;
                    vecResult.push_back(stCrowdGatheringDetResult);
                    nProportionThreshold++;
                }
                if(nProportionThreshold >= Param.nProportionThreshold)
                {
                    printf("区域%d人员聚集阈值%d >= %d\n", nCount, nProportionThreshold, Param.nProportionThreshold);
                    bCrowdGatheringDetParamResult = true;
                    // break;
                }
                                    
            }
        }
        nCount++;
    }
    /* ***************** 人员聚集侦测 ***************** */


    /* 跟踪算法 */
    std::vector<DetectResult_S> vecBoxs;
    for (const auto& box : vBoxDatas)
    {
        DetectResult_S result;
        result.fConfidence = box.fConfidence;
        result.nClassId = box.nLabel;
        result.vfBox = cv::Rect_<float>(box.stBoxs.nX1, box.stBoxs.nY1, 
                                       box.stBoxs.nX2 - box.stBoxs.nX1, 
                                       box.stBoxs.nY2 - box.stBoxs.nY1);
        vecBoxs.push_back(result);
    
        Result_S stResult;
        stResult.nId = 0;
        stResult.fX = (float)box.stBoxs.nX1;
        stResult.fY = (float)box.stBoxs.nY1;
        stResult.fWidth = (float)box.stBoxs.nX2 - box.stBoxs.nX1;
        stResult.fHeight = (float)box.stBoxs.nY2 - box.stBoxs.nY1;
        vstModelDetectResult.push_back(stResult);
    }

    /* 跟踪算法+区域分析 */
    {
        CStatisticsTimer runTime("跟踪+后处理耗时");
        std::vector<cSTrack> vecStracks = m_pByteTracker->update(vecBoxs);

        /* 分析结果 */
        /* 进行区域判断 */
        for (int nIndex = 0; nIndex < (int)vecStracks.size(); nIndex++)
        {
            int nPersonId = vecStracks[nIndex].track_id;
            std::vector<float> vectlwh = vecStracks[nIndex].tlwh;
            if (vectlwh.size() < 4)
            {
                continue;
            }
            /* 中心点 */
            cv::Point centerPoint(static_cast<int>(vectlwh[0] + (vectlwh[2] / 2)),
                                static_cast<int>(vectlwh[1] + (vectlwh[3] / 2)));
            /* 底边中点 */                
            cv::Point bottomMidPoint(static_cast<int>(vectlwh[0] + (vectlwh[2] / 2)),
                                static_cast<int>((vectlwh[3])));
            /* 矩形区域 */
            std::vector<cv::Point> rectPoints = {
                                        cv::Point(vectlwh[0], vectlwh[1]), // 左上角
                                        cv::Point(vectlwh[2], vectlwh[1]), // 右上角
                                        cv::Point(vectlwh[2], vectlwh[3]), // 右下角
                                        cv::Point(vectlwh[0], vectlwh[3])  // 左下角
                                    };
            /* 长宽比 */
            float fAspectRatio = (vectlwh[3] - vectlwh[1]) / (vectlwh[2] - vectlwh[0]);

            /* 判断id在不在表里 */
            if (m_mapPenson.count(nPersonId))
            {
                m_mapPenson[nPersonId].startPoint = m_mapPenson[nPersonId].curPoint;
                m_mapPenson[nPersonId].curPoint = centerPoint;
                m_mapPenson[nPersonId].bottomMidPoint = bottomMidPoint;
                m_mapPenson[nPersonId].fAspectRatio = fAspectRatio;
                m_mapPenson[nPersonId].ndwellTime = 0;
                m_mapPenson[nPersonId].isUsed = true;
            }
            else
            {
                Penson_S newPenson;
                newPenson.nId = nPersonId;
                newPenson.startPoint = centerPoint;
                newPenson.curPoint = centerPoint;
                newPenson.fAspectRatio = fAspectRatio;
                m_mapPenson[nPersonId].bottomMidPoint = bottomMidPoint;
                newPenson.ndwellTime = 0;
                newPenson.isUsed = true;

                m_mapPenson[nPersonId] = newPenson;
            }

            /* 区域分析 */
            Result_S stResult;
            //printf("[人员倒地 DEBUG] 当前人员长宽比:%.2f\n",fAspectRatio);
            /* 人员倒地 */
            if(stInData.stParam.stPersonFallDownParam.bEnable)
            {
                stResult.bPersonFalldownFlag = (fAspectRatio < m_fFallDownThresh);
            }

            /* 是否启用多区域对比 */
            if(!stInData.stParam.bVecEnable)
            {
#if 0
                /* 越界检测 */
                if (stInData.stParam.stTripLineParam.bEnable)
                {
                    stResult.enTripLineType = tripLineDetection(
                        m_mapPenson[nPersonId].startPoint,
                        centerPoint,
                        stInData.stParam.stTripLineParam.alertLine1,
                        stInData.stParam.stTripLineParam.alertLine2);
                }

                /* 入侵检测 */
                if (stInData.stParam.stIntrusionParam.bEnable)
                {
                    stResult.bIntrusionFlag = intrusionZoneDetection(
                        centerPoint,
                        stInData.stParam.stIntrusionParam.vecPoints);
                }

                /* 进入检测 */
                if (stInData.stParam.stEntryParam.bEnable)
                {
                    stResult.bEntryFlag = entryZoneDetection(
                        m_mapPenson[nPersonId].startPoint,
                        centerPoint,
                        stInData.stParam.stEntryParam.vecPoints);
                }

                /* 离开检测 */
                if (stInData.stParam.stLeaveParam.bEnable)
                {
                    stResult.bLeaveFlag = leaveZoneDetection(
                        m_mapPenson[nPersonId].startPoint,
                        centerPoint,
                        stInData.stParam.stLeaveParam.vecPoints);
                }
#endif
            }
            else
            {
#if 0
                /* 越界检测 */
                for (auto &Param : stInData.stParam.vstTripLineParam)
                {
                    if (Param.bEnable)
                    {
                        stResult.enTripLineType = tripLineDetection(
                            m_mapPenson[nPersonId].startPoint,
                            centerPoint,
                            Param.alertLine1,
                            Param.alertLine2);
                        
                        if (stResult.enTripLineType != 0)
                        {
                            break;
                        }
                        
                    }
                }

                /* 入侵检测 */
                for (auto &Param : stInData.stParam.vstIntrusionParam)
                {
                    if (Param.bEnable)
                    {
                        stResult.bIntrusionFlag = intrusionZoneDetection(
                            centerPoint,
                            Param.vecPoints);
                        
                        if (stResult.bIntrusionFlag)
                        {
                            break;
                        }
                        
                    }
                }

                /* 进入检测 */
                for (auto &Param : stInData.stParam.vstEntryParam)
                {
                    if (Param.bEnable)
                    {
                        stResult.bEntryFlag = entryZoneDetection(
                            m_mapPenson[nPersonId].startPoint,
                            centerPoint,
                            Param.vecPoints);
                        
                        if (stResult.bEntryFlag)
                        {
                            break;
                        }
                        
                    }
                }

                /* 离开检测 */
                for (auto &Param : stInData.stParam.vstLeaveParam)
                {
                    if (Param.bEnable)
                    {
                        stResult.bLeaveFlag = leaveZoneDetection(
                            m_mapPenson[nPersonId].startPoint,
                            centerPoint,
                            Param.vecPoints);
                        
                        if (stResult.bLeaveFlag)
                        {
                            break;
                        }
                        
                    }
                }
#endif
                /* 徘徊检测 */
                for (auto &Param : stInData.stParam.vsLoiteringParam)
                {
                    if (Param.bEnable)
                    {
                        if(!m_mapPenson[nPersonId].stLoitering.bLoiter)
                        {
                             /* 是否徘徊 */
                            if(isIntersecting(rectPoints,Param.vecPoints))
                            {
                        #if HumanAreaDetect_DEBUG
                                printf("====personId[%d]===徘徊侦测 触发徘徊 获取当前时间戳=====\n",nPersonId);
                        #endif
                                m_mapPenson[nPersonId].stLoitering.nLoiterTimeStamp = getSteadyTimeStampMs();
                                m_mapPenson[nPersonId].stLoitering.bLoiter = true;
                            }
                        }
                        else
                        {
                            if(!isIntersecting(rectPoints,Param.vecPoints))
                            {
                        #if HumanAreaDetect_DEBUG
                                printf("====personId[%d]===徘徊侦测 未达到时间阈值=====\n",nPersonId);
                        #endif
                                m_mapPenson[nPersonId].stLoitering.bLoiter = false;
                            }
                            else
                            {
                        #if HumanAreaDetect_DEBUG
                                printf("====personId[%d]===徘徊侦测 判断是否达到徘徊侦测时间阈值 [%d]秒=====\n",nPersonId,Param.nTimeThreshold);
                        #endif
                                /* 时间阈值 */
                                if(isTimeIntervalExceeded(m_mapPenson[nPersonId].stLoitering.nLoiterTimeStamp,Param.nTimeThreshold))
                                {
                        #if HumanAreaDetect_DEBUG
                                printf("====personId[%d]===徘徊侦测 达到时间阈值=====\n",nPersonId);
                        #endif
                                    stResult.bLoiteringFlag = true;
                                    m_mapPenson[nPersonId].stLoitering.bLoiter = false;
                                }
                            }
                        }

                        if (stResult.bLoiteringFlag)
                        {
                            break;
                        }
                        
                    }
                }
                

                /* 翻越围栏检测-note:连续检测多帧在区域内才触发 */
                for (auto &Param : stInData.stParam.vstFenceClimbingParam)
                {
                    if (Param.bEnable)
                    {
                        if(intrusionZoneDetection(bottomMidPoint,Param.vecPoints))
                        {
                            m_mapPenson[nPersonId].stFenceClimbing.nFrameCount++;
                        }
                        if(m_mapPenson[nPersonId].stFenceClimbing.nFrameCount > Param.nDetectFrame)
                        {
                            stResult.bFenceClimbFlag = true;
                            m_mapPenson[nPersonId].stFenceClimbing.nFrameCount = 0;
                        }
                        if (stResult.bFenceClimbFlag)
                        {
                            break;
                        }
                    }
                }
                /* 离岗检测 */
                for (auto &Param : stInData.stParam.vstLeavePostParam)
                {
                    if (Param.bEnable)
                    {
                        if(m_mapPenson[nPersonId].stLeavePost.bEnLeave)
                        {
                            /* 判断是否在区域内 */
                            if(intrusionZoneDetection(centerPoint,Param.vecPoints))
                            {
                                m_mapPenson[nPersonId].stLeavePost.bEnLeave = false;
                            }
                            else
                            {
                        #if HumanAreaDetect_DEBUG
                                printf("====personId[%d]===离开岗位 判断是否达到离岗时间阈值 [%d]秒=====\n",nPersonId,Param.nTimeThreshold);
                        #endif
                                /* 时间阈值 */
                                if(isTimeIntervalExceeded(m_mapPenson[nPersonId].stLeavePost.nLeaveTimeStamp,Param.nTimeThreshold))
                                {
                        #if HumanAreaDetect_DEBUG
                                printf("====personId[%d]===离开岗位 达到时间阈值=====\n",nPersonId);
                        #endif
                                    stResult.bLeavePostFlag = true;
                                    m_mapPenson[nPersonId].stLeavePost.bEnLeave = false;
                                }
                            }
                        }
                        else
                        {
                            /* 判断是否离开区域 */
                            if(leaveZoneDetection(m_mapPenson[nPersonId].startPoint,centerPoint,Param.vecPoints))
                            {
                                m_mapPenson[nPersonId].stLeavePost.bEnLeave = true;
                                m_mapPenson[nPersonId].stLeavePost.nLeaveTimeStamp = getSteadyTimeStampMs();
                        #if HumanAreaDetect_DEBUG
                                printf("====personId[%d]===离开岗位 时间戳[%lld]=====\n",nPersonId,m_mapPenson[nPersonId].stLeavePost.nLeaveTimeStamp);
                        #endif
                            }
                        }
                        
                        if (stResult.bLeavePostFlag)
                        {
                            break;
                        }
                    }
                }
                /* 行人闯入识别 */
                for (auto &Param : stInData.stParam.vstPedestrianIntrusionParam)
                {
                    if (Param.bEnable)
                    {

                        if(m_mapPenson[nPersonId].stPedestrianIntrusion.bEntry)
                        {
                            /* 判断是否在区域外 */
                            if(!intrusionZoneDetection(m_mapPenson[nPersonId].bottomMidPoint,Param.vecPoints))
                            {
                                m_mapPenson[nPersonId].stPedestrianIntrusion.bEntry = false;
                            }
                            else
                            {
                        #if HumanAreaDetect_DEBUG
                                printf("=======行人闯入 判断是否达到闯入时间阈值[%d]秒=====\n",Param.nTimeThreshold);
                        #endif
                                /* 时间阈值 */
                                if(isTimeIntervalExceeded(m_mapPenson[nPersonId].stPedestrianIntrusion.nEntryTimeStamp,Param.nTimeThreshold))
                                {
                        #if HumanAreaDetect_DEBUG
                                printf("=======行人闯入 达到时间阈值=====\n");
                        #endif
                                    stResult.bPedestrianIntrusionFlag = true;
                                    m_mapPenson[nPersonId].stPedestrianIntrusion.bEntry = false;
                                }
                            }
                        }
                        else
                        {
                            /* 判断是否进入区域 */
                            if(intrusionZoneDetection(m_mapPenson[nPersonId].bottomMidPoint,Param.vecPoints))
                            {
                                m_mapPenson[nPersonId].stPedestrianIntrusion.bEntry = true;
                                m_mapPenson[nPersonId].stPedestrianIntrusion.nEntryTimeStamp = cv::getTickCount() / (cv::getTickFrequency() * 1000);
                            }
                        }
                        
                        if (stResult.bPedestrianIntrusionFlag)
                        {
                            break;
                        }
                    }
                }
            }
           
            stResult.nId = nPersonId;
            stResult.fX = vectlwh[0];
            stResult.fY = vectlwh[1];
            stResult.fWidth = vectlwh[2];
            stResult.fHeight = vectlwh[3];

            vecResult.push_back(stResult);
        }

        for (auto pair = m_mapPenson.begin(); pair != m_mapPenson.end();)
        {
            int id = pair->first;
           
            if (m_mapPenson[id].isUsed)
            {
                m_mapPenson[id].isUsed = false;
            }
            else
            {
                m_mapPenson[id].ndwellTime++;
            }

            if (m_mapPenson[id].ndwellTime >= m_nMaxTimeLost)
            {
                /* 离开区域 */
                if(m_mapPenson[id].stLeavePost.bEnLeave)
                {
                    Result_S stResult; 
                    stResult.bLeavePostFlag = true;
                    vecResult.push_back(stResult);
                }
            
                pair = m_mapPenson.erase(pair);
            }
            else
            {
                ++pair;
            }
        }
        
        int enType = 0;
        if (vecStracks.size() > 0)
        {
            const float scaleX = static_cast<float>(stInData.inMat.cols) / m_nLimitWidth;
            const float scaleY = static_cast<float>(stInData.inMat.rows * 2 / 3) / m_nLimitHeight;

            for (auto &result : vecResult)
            {
#if 0
                if (result.enTripLineType != 0)
                {
                    enType |= 0x01; // Type_E::OVERSHOOT;
                }

                if (result.bIntrusionFlag)
                {
                    enType |= 0x02; // Type_E::INTRUSION;
                }

                if (result.bEntryFlag)
                {
                    enType |= 0x04; // Type_E::ENTRY;
                }

                if (result.bLeaveFlag)
                {
                    enType |= 0x08; // Type_E::EXIT;
                }
#endif
                if (result.bFenceClimbFlag)
                {
                    enType |= 0x10; // Type_E::FENCE_CLIMBING;
                }
                if (result.bLeavePostFlag)
                {
                    enType |= 0x20; // Type_E::LEAVE_POST;
                }
                if (result.bPedestrianIntrusionFlag)
                {
                    enType |= 0x40; // Type_E::PEDESTRIAN_INTRUSION;
                }
                if (result.bLoiteringFlag)
                {
                    enType |= 0x80; // Type_E::LOITERING_DETECT;
                }
                if (result.bPersonFalldownFlag)
                {
                    enType |= 0x200; // Type_E::PERSON_FALL_DOWN;
                }
                //0x400......

                result.fX *= scaleX;
                result.fY *= scaleY;
                result.fWidth *= scaleX;
                result.fHeight *= scaleY;
            }

            stOutData->nChnId = stInData.nChnId;

            if (enType != 0)
            {
                stOutData->validResult = true;
                stOutData->nType = enType;
            }
        }
        
        /* 人员聚集 */
        if(bCrowdGatheringDetParamResult)
        {
            enType |= 0x100;
            stOutData->nChnId = stInData.nChnId;
            stOutData->validResult = true;
            stOutData->nType = enType;
        }
    }
    return true;
}

/* 处理数据 */
bool HumanAreaDetect_NS::CHumanAreaDetectV3_0::resizeAndPadImage(cv::Mat inputImage, cv::Mat &outputImage)
{
    int imageWidth = inputImage.cols;
    int imageHeight = inputImage.rows;
    m_fResizeScale = static_cast<float>(m_nLimitWidth) / std::max(imageWidth, imageHeight);

    // std::cout << "imageWidth：" << imageWidth << std::endl;
    // std::cout << "imageHeight：" << imageHeight << std::endl;
    // std::cout << "m_nLimitWidth：" << m_nLimitWidth << std::endl;
    // std::cout << "m_fResizeScale" << m_fResizeScale << std::endl;

    int newWidth = static_cast<int>(imageWidth * m_fResizeScale);
    int newHeight = static_cast<int>(imageHeight * m_fResizeScale);

    // std::cout << "计算后的缩放：" << newWidth << "x" << newHeight << std::endl;

    cv::Mat resizedImage;
    cv::resize(inputImage, resizedImage, cv::Size(newWidth, newHeight));

    cv::Mat output = cv::Mat::zeros(m_nLimitHeight, m_nLimitWidth, inputImage.type());

    m_nXOffset = static_cast<int>((m_nLimitWidth - newWidth) / 2);
    m_nYOffset = static_cast<int>((m_nLimitHeight - newHeight) / 2);

    resizedImage.copyTo(output(cv::Rect(m_nXOffset, m_nYOffset, newWidth, newHeight)));
    outputImage = output;

    return true;
}

bool HumanAreaDetect_NS::CHumanAreaDetectV3_0::resizeAndPadImage2(cv::Mat inputImage, cv::Mat &outputImage)
{
    int imageWidth = inputImage.cols;
    int imageHeight = inputImage.rows;
    m_fResizeScale = static_cast<float>(m_nLimitWidth) / std::max(imageWidth, imageHeight);

    int newWidth = static_cast<int>(imageWidth * m_fResizeScale);
    int newHeight = static_cast<int>(imageHeight * m_fResizeScale);

    cv::Mat resizedImage;
    cv::resize(inputImage, resizedImage, cv::Size(newWidth, newHeight));

    m_nXOffset = static_cast<int>((m_nLimitWidth - newWidth) / 2);
    m_nYOffset = static_cast<int>((m_nLimitHeight - newHeight) / 2);
    int nBottom = m_nLimitHeight - newHeight - m_nYOffset;
    int mRight = m_nLimitWidth - newWidth - m_nXOffset;

    cv::copyMakeBorder(resizedImage, outputImage,
                       m_nYOffset, nBottom, m_nXOffset, mRight, 
                       cv::BORDER_CONSTANT, cv::Scalar(0, 0, 0));
    return true;
}

/* 拌线检测：判断两条线段是否有交点 */
TripLineType_E HumanAreaDetect_NS::CHumanAreaDetectV3_0::tripLineDetection(
    const cv::Point &startPoint,
    const cv::Point &lastPoint,
    const cv::Point &alertLineFirst,
    const cv::Point &alertLineSecond)
{
    TripLineType_E enType = OVERFLOW_NONE;

    if (!isBoundingBoxIntersecting(startPoint, lastPoint, alertLineFirst, alertLineSecond))
    {
        return enType;
    }

    cv::Point lineStartPoint;
    cv::Point lineEndPoint;

    /* 判断线段的情况 */
    if (alertLineFirst.x == alertLineSecond.x)
    {
        if (alertLineFirst.y < alertLineSecond.y)
        {
            lineStartPoint = alertLineSecond;
            lineEndPoint = alertLineFirst;
        }
        else
        {
            lineStartPoint = alertLineFirst;
            lineEndPoint = alertLineSecond;
        }
    }
    else
    {
        if (alertLineFirst.x < alertLineSecond.x)
        {
            lineStartPoint = alertLineFirst;
            lineEndPoint = alertLineSecond;
        }
        else
        {
            lineStartPoint = alertLineSecond;
            lineEndPoint = alertLineFirst;
        }
    }

    /* 计算叉积 */
    int crossStartPoint = crossProduct(lineStartPoint, lineEndPoint, startPoint);
    int crossLastPoint = crossProduct(lineStartPoint, lineEndPoint, lastPoint);

    /* 判断A -> B */
    if (crossStartPoint < 0 && crossLastPoint > 0)
    {
        /* 拌线方向是A -> B */
        enType = OVERFLOW_A_TO_B;
        //std::cout << "拌线方向是A -> B" << std::endl;
    }

    /* 判断B -> A */
    if (crossStartPoint > 0 && crossLastPoint < 0)
    {
        /* 拌线方向是B -> A */
        enType = OVERFLOW_B_TO_A;
        //std::cout << "拌线方向是B -> A" << std::endl;
    }

    /* 判断A <-> B */
    if ((crossStartPoint * crossLastPoint) < 0)
    {
        /* 拌线方向是A <-> B的其中一种 */
        enType = OVERFLOW_A_B_BOTH;
        //std::cout << "拌线方向是A <-> B的其中一种" << std::endl;
    }

    return enType;
}

bool HumanAreaDetect_NS::CHumanAreaDetectV3_0::isBoundingBoxIntersecting(
    const cv::Point &lineA1,
    const cv::Point &lineA2,
    const cv::Point &lineB1,
    const cv::Point &lineB2)
{
    return std::max(lineA1.x, lineA2.x) >= std::min(lineB1.x, lineB2.x) &&
           std::max(lineB1.x, lineB2.x) >= std::min(lineA1.x, lineA2.x) &&
           std::max(lineA1.y, lineA2.y) >= std::min(lineB1.y, lineB2.y) &&
           std::max(lineB1.y, lineB2.y) >= std::min(lineA1.y, lineA2.y);
}

int HumanAreaDetect_NS::CHumanAreaDetectV3_0::crossProduct(
    const cv::Point &alertLineStart,
    const cv::Point &alertLineEnd,
    const cv::Point &testPoint)
{
    return (alertLineEnd.x - alertLineStart.x) * (testPoint.y - alertLineStart.y) -
           (alertLineEnd.y - alertLineStart.y) * (testPoint.x - alertLineStart.x);
}

/* 入侵检测：判断禁止入侵的区域有无点 */
bool HumanAreaDetect_NS::CHumanAreaDetectV3_0::intrusionZoneDetection(
    const cv::Point &lastPoint,
    std::vector<cv::Point> polygons)
{
    double intrusionResult = cv::pointPolygonTest(polygons, lastPoint, false);
#if HumanAreaDetect_DEBUG
    printf("区域入侵：lastPoint坐标 = (%d, %d), intrusionResult = %f, %s\n", 
       lastPoint.x, 
       lastPoint.y, 
       intrusionResult, 
       (intrusionResult > 0) ? "在区域内部" : ((intrusionResult == 0) ? "在区域边界上" : "在区域外部")
    ); 
#endif
    return intrusionResult >= 0;
}

/* 进入检测：根据起始点和当前点的关系，判断是否进入 */
bool HumanAreaDetect_NS::CHumanAreaDetectV3_0::entryZoneDetection(
    const cv::Point &startPoint,
    const cv::Point &lastPoint,
    std::vector<cv::Point> polygons)
{
    double StartResult = cv::pointPolygonTest(polygons, startPoint, false);
#if HumanAreaDetect_DEBUG
    printf("进入检测：startPoint坐标 = (%d, %d), StartResult = %f, %s\n", 
       startPoint.x, 
       startPoint.y, 
       StartResult, 
       (StartResult > 0) ? "在区域内部" : ((StartResult == 0) ? "在区域边界上" : "在区域外部")
    );  
#endif
    if (StartResult >= 0) return false;
    double LastResult = cv::pointPolygonTest(polygons, lastPoint, false);
#if HumanAreaDetect_DEBUG
     printf("进入检测：lastPointt坐标 = (%d, %d), LastResult = %f, %s\n", 
       lastPoint.x, 
       lastPoint.y, 
       LastResult, 
       (LastResult > 0) ? "在区域内部" : ((LastResult == 0) ? "在区域边界上" : "在区域外部")
    );
#endif
    return LastResult >= 0;
}

/* 离开检测：根据起始点和当前点的关系，判断是否离开 */
bool HumanAreaDetect_NS::CHumanAreaDetectV3_0::leaveZoneDetection(
    const cv::Point &startPoint,
    const cv::Point &lastPoint,
    std::vector<cv::Point> polygons)
{
    double StartResult = cv::pointPolygonTest(polygons, startPoint, false);
#if HumanAreaDetect_DEBUG
    printf("离开检测：startPoint坐标 = (%d, %d), StartResult = %f, %s\n", 
       startPoint.x, 
       startPoint.y, 
       StartResult, 
       (StartResult > 0) ? "在区域内部" : ((StartResult == 0) ? "在区域边界上" : "在区域外部")
    );  
#endif
    if (StartResult < 0) return false;
    double LastResult = cv::pointPolygonTest(polygons, lastPoint, false);
#if HumanAreaDetect_DEBUG
    printf("离开检测：lastPointt坐标 = (%d, %d), LastResult = %f, %s\n", 
       lastPoint.x, 
       lastPoint.y, 
       LastResult, 
       (LastResult > 0) ? "在区域内部" : ((LastResult == 0) ? "在区域边界上" : "在区域外部")
    );
#endif
    return LastResult < 0;
}

/* 判断两个多边形是否有交点 */
bool HumanAreaDetect_NS::CHumanAreaDetectV3_0::isIntersecting(
    std::vector<cv::Point> rectPolygon,
    std::vector<cv::Point> polygons)
{
    std::vector<cv::Point> intersectionPoints;
    cv::intersectConvexConvex(rectPolygon, polygons, intersectionPoints);
    return !intersectionPoints.empty();
}

/* 判断目标框与线段是否有交点 */
bool HumanAreaDetect_NS::CHumanAreaDetectV3_0::isLineIntersectingRect(
    const cv::Point &topLeft,
    const cv::Point &bottomRight, 
    const cv::Point &alertLineFirst, 
    const cv::Point &alertLineSecond)
{
    cv::Rect cBoxRect(topLeft, bottomRight);
    cv::Point clippedStart = alertLineFirst;
    cv::Point clippedEnd = alertLineSecond;
    return cv::clipLine(cBoxRect, clippedStart, clippedEnd);
}

bool HumanAreaDetect_NS::CHumanAreaDetectV3_0::isTimeIntervalExceeded(int64_t nRecordTime, int nThresholdSec) 
{
    int64_t nCurrentTime = getSteadyTimeStampMs();
    
    int64_t nTimeDiffMs = nCurrentTime - nRecordTime;
#if HumanAreaDetect_DEBUG
    printf("====nTimeDiffMs[%lld]========\n",nTimeDiffMs);
#endif
    return (nTimeDiffMs > nThresholdSec * 1000);
}