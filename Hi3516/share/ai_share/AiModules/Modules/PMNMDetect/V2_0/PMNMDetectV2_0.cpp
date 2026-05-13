/*
 * @FilePath     : PMNMDetectV2_0.cpp
 * @Author       : 廖尔涛 liaoet@kfb.cn
 * @Date         : 2024-09-23 20:19:15
 * @LastEditors: 梁浩尧 lianghaoyao@kfb.cn
 * @LastEditTime: 2025-12-24 15:44:23
 * @Description  : 人少场景
 */
#include "PMNMDetectV2_0.hpp"

#include "BYTETracker.h"
#include "SaveImage.hpp"
#include "StatisticsTimer.hpp"


using namespace PMNMDetect_NS;

PMNMDetect_NS::CPMNMDetectV2_0::CPMNMDetectV2_0(InParam_S stInParam)
    : m_stInParam(stInParam)
{
}

PMNMDetect_NS::CPMNMDetectV2_0::~CPMNMDetectV2_0()
{
    unInit();
}

/* 初始化 */
bool PMNMDetect_NS::CPMNMDetectV2_0::init()
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

    for (int i = 0; i < 3; ++i) 
    {
        bRet = false;
        m_pByteTracker[i] = new Inference_NS::cBYTETracker();
        if(m_pByteTracker[i])
        {
            m_pByteTracker[i]->setValue(m_fTrackThresh[i], m_fHighThresh[i], m_fMatchThresh[i], m_nFrameId[i], m_nMaxTimeLost);
            bRet = true;
        }
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
bool PMNMDetect_NS::CPMNMDetectV2_0::unInit()
{
    CStatisticsTimer runTime("边界检测反初始化耗时");
    if (m_pYoloUltralytics)
    {
        delete m_pYoloUltralytics;
        m_pYoloUltralytics = nullptr;
    }


    for (int i = 0; i < 3; ++i) 
    {
        if (m_pByteTracker[i])
        {
            delete m_pByteTracker[i];
            m_pByteTracker[i] = nullptr;
        }
    }

    return true;
}

void PMNMDetect_NS::CPMNMDetectV2_0::resetNonMotorVehicleIntrusionStatus()
{
    for(int i = 0; i < sizeof(m_stNonMotorVehicleIntrusion) / sizeof(NonMotorVehicleIntrusion_S); i++)
    {
        m_stNonMotorVehicleIntrusion[i].bNonMotorVehicleIntrusion = false;
    }
    return ;
}

/* 处理数据 */
bool PMNMDetect_NS::CPMNMDetectV2_0::process(
    InData_S stInData,
    std::vector<Result_S> &vecResult,
    // std::vector<Result_S> &vecResult1,
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

    for (int i = 0; i < 3; ++i) 
    {
        if (!m_pByteTracker[i])
        {
            printf("未初始化算法类\n");
            return false;
        }
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

    /* 跟踪算法 */
    std::vector<DetectResult_S> vecBoxs[3];   /* 0-人 1-机动车 2-非机动车 */
    for (const auto& box : vBoxDatas) 
    {
        DetectResult_S vecBox;
        vecBox.nClassId   = box.nLabel;
        vecBox.fConfidence= box.fConfidence;
        vecBox.vfBox      = cv::Rect_<float>(box.stBoxs.nX1, box.stBoxs.nY1,
                                        box.stBoxs.nX2 - box.stBoxs.nX1,
                                        box.stBoxs.nY2 - box.stBoxs.nY1);

        vecBoxs[box.nLabel].push_back(vecBox);  /* box.nLabel: 0-人 1-机动车 2-非机动车 */

        Result_S stResult;
        stResult.nId = 0;
        stResult.fX1 = (float)box.stBoxs.nX1;
        stResult.fY1 = (float)box.stBoxs.nY1;
        stResult.fX2 = (float)box.stBoxs.nX2;
        stResult.fY2 = (float)box.stBoxs.nY2;
        // vecResult1.push_back(stResult);
    }

    /* ====================================== 非机动车闯入识别 ====================================== */ 
    bool bNonMotorVehicleIntrusionFlag = false;
    std::vector<DetectResult_S> &vstNonMotorVehicleIntrusionBoxs = vecBoxs[2];

    for(unsigned int i = 0; i < stInData.stParam.vstNonMotorVehicleIntrusionParam.size(); i++)
    {
        int nRet = 0;
        NonMotorVehicleIntrusionParam_S Param = stInData.stParam.vstNonMotorVehicleIntrusionParam.at(i);
        if(Param.bEnable)
        {
            for(auto &vecBox : vstNonMotorVehicleIntrusionBoxs)
            {
                if(vecBox.fConfidence >= Param.fNonMotorVehicleIntrusionThreshold)
                {
                    nRet += intrusionZoneDetection(
                            cv::Point(vecBox.vfBox.x + vecBox.vfBox.width / 2, vecBox.vfBox.y + vecBox.vfBox.height),
                            Param.vecPoints);
                }
            }
            
            /* 判断当前侦测区域是否有进入禁止非机动车进入区域的目标 */
            if(nRet)
            {
                /* 该侦测区域第一次出现目标 */
                if(!m_stNonMotorVehicleIntrusion[i].bNonMotorVehicleIntrusion)
                {
                    // printf(" ============ 该侦测区域第一次出现目标 [%s]:[%d]============= \n", __FILE__, __LINE__);

                    m_stNonMotorVehicleIntrusion[i].bNonMotorVehicleIntrusion = true;
                    m_stNonMotorVehicleIntrusion[i].nNonMotorVehicleIntrusionTimeStamp = getSteadyTimeStampMs();
                    // stNonMotorVehicleIntrusion.nAreaCode = i;
                }
                else
                {
                    /* 判断从进入检测区域到当前持续时间是否超过设定阈值 */
                    if(getSteadyTimeStampMs() - m_stNonMotorVehicleIntrusion[i].nNonMotorVehicleIntrusionTimeStamp >= Param.nNonMotorVehicleIntrusionTimeThreshold)
                    {
                        bNonMotorVehicleIntrusionFlag = true;
                        // printf(" =====区域 %d 达到非机动车闯入的时长阈值 =======%d \n", i, Param.nNonMotorVehicleIntrusionTimeThreshold);
                    }   
                }
            }
            /* 检测到的目标全部都没在当前侦测区域 则重置该区域的状态 */
            else 
            {
                m_stNonMotorVehicleIntrusion[i].bNonMotorVehicleIntrusion = false;
                m_stNonMotorVehicleIntrusion[i].nNonMotorVehicleIntrusionTimeStamp = getSteadyTimeStampMs();
            }
        }
        else 
        {
            m_stNonMotorVehicleIntrusion[i].bNonMotorVehicleIntrusion = false;
            m_stNonMotorVehicleIntrusion[i].nNonMotorVehicleIntrusionTimeStamp = getSteadyTimeStampMs();
        }

        if(bNonMotorVehicleIntrusionFlag)
        {
            break;
        }
    }
    /* ====================================== 非机动车闯入识别 ====================================== */

    /* 跟踪算法+区域分析 */
    {
        CStatisticsTimer runTime("跟踪+后处理耗时");
        // std::vector<cSTrack> vecStracks = m_pByteTracker->update(vecBoxs);
        std::vector<cSTrack> vecStracks[3];
        for (int i = 0; i < 3; ++i) 
        {
            vecStracks[i] = m_pByteTracker[i]->update(vecBoxs[i]);
        }

        /* 分析结果 */
        /* 进行区域判断 */
        for (int nClsIdx = 0; nClsIdx < 3; ++nClsIdx) 
        {
            for (int nIndex = 0; nIndex < (int)vecStracks[nClsIdx].size(); nIndex++)
            {
                Target_S *pstTarget = nullptr;
                int nTargetId = vecStracks[nClsIdx][nIndex].track_id;
                std::vector<float> vectlwh = vecStracks[nClsIdx][nIndex].tlwh;
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
                if(nClsIdx == 0)
                {
                    /* 判断id在不在表里 */
                    if (m_mapPenson.count(nTargetId))
                    {
                        m_mapPenson[nTargetId].startPoint = m_mapPenson[nTargetId].curPoint;
                        m_mapPenson[nTargetId].curPoint = centerPoint;
                        m_mapPenson[nTargetId].bottomMidPoint = bottomMidPoint;
                        m_mapPenson[nTargetId].ndwellTime = 0;
                        m_mapPenson[nTargetId].isUsed = true;
                    }
                    else
                    {
                        Target_S newPenson;
                        newPenson.nId = nTargetId;
                        newPenson.startPoint = centerPoint;
                        newPenson.curPoint = centerPoint;
                        m_mapPenson[nTargetId].bottomMidPoint = bottomMidPoint;
                        newPenson.ndwellTime = 0;
                        newPenson.isUsed = true;

                        m_mapPenson[nTargetId] = newPenson;
                    }
                    
                    pstTarget = &m_mapPenson[nTargetId];
                }
                else if(nClsIdx == 1)
                {
                    if (m_mapVehicle.count(nTargetId))
                    {
                        m_mapVehicle[nTargetId].startPoint = m_mapVehicle[nTargetId].curPoint;
                        m_mapVehicle[nTargetId].curPoint = centerPoint;
                        m_mapVehicle[nTargetId].bottomMidPoint = bottomMidPoint;
                        m_mapVehicle[nTargetId].ndwellTime = 0;
                        m_mapVehicle[nTargetId].isUsed = true;
                    }
                    else
                    {
                        Target_S newVehicle;
                        newVehicle.nId = nTargetId;
                        newVehicle.startPoint = centerPoint;
                        newVehicle.curPoint = centerPoint;
                        m_mapVehicle[nTargetId].bottomMidPoint = bottomMidPoint;
                        newVehicle.ndwellTime = 0;
                        newVehicle.isUsed = true;

                        m_mapVehicle[nTargetId] = newVehicle;
                    }
                    
                    pstTarget = &m_mapVehicle[nTargetId];
                }
                else if(nClsIdx == 2)
                {
                    if (m_mapNonMotorVehicle.count(nTargetId))
                    {
                        m_mapNonMotorVehicle[nTargetId].startPoint = m_mapNonMotorVehicle[nTargetId].curPoint;
                        m_mapNonMotorVehicle[nTargetId].curPoint = centerPoint;
                        m_mapNonMotorVehicle[nTargetId].bottomMidPoint = bottomMidPoint;
                        m_mapNonMotorVehicle[nTargetId].ndwellTime = 0;
                        m_mapNonMotorVehicle[nTargetId].isUsed = true;
                    }
                    else
                    {
                        Target_S newNonMotorVehicle;
                        newNonMotorVehicle.nId = nTargetId;
                        newNonMotorVehicle.startPoint = centerPoint;
                        newNonMotorVehicle.curPoint = centerPoint;
                        m_mapNonMotorVehicle[nTargetId].bottomMidPoint = bottomMidPoint;
                        newNonMotorVehicle.ndwellTime = 0;
                        newNonMotorVehicle.isUsed = true;

                        m_mapNonMotorVehicle[nTargetId] = newNonMotorVehicle;
                    }
                    
                    pstTarget = &m_mapNonMotorVehicle[nTargetId];
                }
                /* 区域分析 */
                Result_S stResult;
                
                /* 是否启用多区域对比 */
                if(!stInData.stParam.bVecEnable)
                {
                    /* 越界检测 */
                    if (stInData.stParam.stTripLineParam.bEnable)
                    {
                        for(const auto& classId : stInData.stParam.stTripLineParam.veDetectionTargetTypes)
                        {
                            if(classId == nClsIdx)
                            {
                                stResult.enTripLineType = tripLineDetection(
                                pstTarget->startPoint,
                                centerPoint,
                                stInData.stParam.stTripLineParam.alertLine1,
                                stInData.stParam.stTripLineParam.alertLine2);
                                break;
                            }
                        }

                    }

                    /* 入侵检测 */
                    if (stInData.stParam.stIntrusionParam.bEnable)
                    {
                        for(const auto& classId : stInData.stParam.stIntrusionParam.veDetectionTargetTypes)
                        {
                            if(classId == nClsIdx)
                            {
                                stResult.bIntrusionFlag = intrusionZoneDetection(
                                centerPoint,
                                stInData.stParam.stIntrusionParam.vecPoints);
                                break;
                            }
                        }

                    }

                    /* 进入检测 */
                    if (stInData.stParam.stEntryParam.bEnable)
                    {
                        for(const auto& classId : stInData.stParam.stEntryParam.veDetectionTargetTypes)
                        {
                            if(classId == nClsIdx)
                            {
                                stResult.bEntryFlag = entryZoneDetection(
                                pstTarget->startPoint,
                                centerPoint,
                                stInData.stParam.stEntryParam.vecPoints);
                                break;
                            }
                        }

                    }

                    /* 离开检测 */
                    if (stInData.stParam.stLeaveParam.bEnable)
                    {
                        for(const auto& classId : stInData.stParam.stLeaveParam.veDetectionTargetTypes)
                        {
                            if(classId == nClsIdx)
                            {
                                stResult.bLeaveFlag = leaveZoneDetection(
                                pstTarget->startPoint,
                                centerPoint,
                                stInData.stParam.stLeaveParam.vecPoints);
                                break;
                            }
                        }
                    }
                }
                else
                {
                    /* 越界检测 */
                    for (auto &Param : stInData.stParam.vstTripLineParam)
                    {
                        if (Param.bEnable)
                        {
                            for(const auto& classId : Param.veDetectionTargetTypes)
                            {
                                if(classId == nClsIdx)
                                {
                                    stResult.enTripLineType = tripLineDetection(
                                    pstTarget->startPoint,
                                    centerPoint,
                                    Param.alertLine1,
                                    Param.alertLine2);
                                    break;
                                }
                            }
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
                            for(const auto& classId : Param.veDetectionTargetTypes)
                            {
                                if(classId == nClsIdx)
                                {
                                    stResult.bIntrusionFlag = intrusionZoneDetection(
                                    centerPoint,
                                    Param.vecPoints);
                                    break;
                                }
                            }
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
                            for(const auto& classId : Param.veDetectionTargetTypes)
                            {
                                if(classId == nClsIdx)
                                {
                                    stResult.bEntryFlag = entryZoneDetection(
                                    pstTarget->startPoint,
                                    centerPoint,
                                    Param.vecPoints);
                                    break;
                                }
                            }
                            
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
                            for(const auto& classId : Param.veDetectionTargetTypes)
                            {
                                if(classId == nClsIdx)
                                {
                                    stResult.bLeaveFlag = leaveZoneDetection(
                                    pstTarget->startPoint,
                                    centerPoint,
                                    Param.vecPoints);
                                    break;
                                }
                            }
                            
                            if (stResult.bLeaveFlag)
                            {
                                break;
                            }
                        }
                    }

                    // int i = 0;
                    /* 应急车道检测区域 */
                    for (auto &Param : stInData.stParam.vstEmergencyLaneOccupancyParam)
                    {
                        if (!Param.bEnable)
                        {
                            continue;
                        }

                        for(const auto& classId : Param.veDetectionTargetTypes)
                        {
                            if(classId != nClsIdx)
                            {
                                continue;
                            }
                            int nRet = intrusionZoneDetection(pstTarget->curPoint, Param.vecPoints);
                            if(nRet >= 0) /* 进入应急车道检测区域 */
                            {
                                if(!pstTarget->stEmergencyLaneOccupancy.bEmergencyLaneOccupancy) /* 记录第一次进入检测区域 */
                                {
                                    pstTarget->stEmergencyLaneOccupancy.bEmergencyLaneOccupancy = true;
                                    pstTarget->stEmergencyLaneOccupancy.nEmergencyLaneOccupancyTimeStamp = getSteadyTimeStampMs();
                                    // printf(" =====区域 %d 出现应急车道占用 =======%d \n", i, Param.nEmergencyLaneOccupancyTimeThreshold);
                                }

                                // if(pstTarget->stEmergencyLaneOccupancy.bEmergencyLaneOccupancy)
                                else
                                { 
                                    /* 判断从进入检测应急车道区域到当前持续时间是否超过设定阈值 */
                                    if(getSteadyTimeStampMs() - pstTarget->stEmergencyLaneOccupancy.nEmergencyLaneOccupancyTimeStamp >= Param.nEmergencyLaneOccupancyTimeThreshold)
                                    {
                                        stResult.bEmergencyLaneOccupancyFlag = true;
                                        // printf(" =====区域 %d 达到应急车道占用的时间阈值 ======= %lld - %lld = %lld %d \n", i, getSteadyTimeStampMs(), pstTarget->stEmergencyLaneOccupancy.nEmergencyLaneOccupancyTimeStamp, getSteadyTimeStampMs() - pstTarget->stEmergencyLaneOccupancy.nEmergencyLaneOccupancyTimeStamp, Param.nEmergencyLaneOccupancyTimeThreshold);
                                    }   
                                }
                            }
                            else /* 目标离开应急车道检测区域，重置 */
                            {
                                // printf(" =====区域 %d 离开应急车道 ======= \n", i);
                                pstTarget->stEmergencyLaneOccupancy.bEmergencyLaneOccupancy = false;
                                pstTarget->stEmergencyLaneOccupancy.nEmergencyLaneOccupancyTimeStamp = getSteadyTimeStampMs();
                            }
                            break;
                        }

                        if(stResult.bEmergencyLaneOccupancyFlag == true)
                        {
                            break;
                        }

                        // i++;
                    }

                }
                stResult.nId = nTargetId;
                stResult.nID = nClsIdx;
                stResult.fX1 = vectlwh[0];
                stResult.fY1 = vectlwh[1];
                stResult.fX2 = vectlwh[0] + vectlwh[2];
                stResult.fY2 = vectlwh[1] + vectlwh[3];

                vecResult.push_back(stResult);
            }
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
                pair = m_mapPenson.erase(pair);
            }
            else
            {
                ++pair;
            }
        }

        size_t total_elem_count = 0;
        for (const auto& vec : vecStracks) 
        {
            total_elem_count += vec.size();
        }

        int enType = 0;
        if (total_elem_count > 0)
        {
            const float scaleX = static_cast<float>(stInData.inMat.cols) / m_nLimitWidth;
            const float scaleY = static_cast<float>(stInData.inMat.rows * 2 / 3) / m_nLimitHeight;

            for (auto &result : vecResult)
            {
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

                if (result.bEmergencyLaneOccupancyFlag)
                {
                    enType |= 0x10; // Type_E::EMERGENCY_LANE_OCCUPANCY;
                }
   
                //0x10、0x20、0x40......

                result.fX1 *= scaleX;
                result.fY1 *= scaleY;
                result.fX2 *= scaleX;
                result.fY2 *= scaleY;
            }
        }
        
        if(bNonMotorVehicleIntrusionFlag)
        {
            enType |= 0x20;
        }

        if (enType != 0)
        {
            stOutData->validResult = true;
        }

        stOutData->nType = enType;
        stOutData->nChnId = stInData.nChnId;
        
    }
    return true;
}

/* 处理数据 */
bool PMNMDetect_NS::CPMNMDetectV2_0::resizeAndPadImage(cv::Mat inputImage, cv::Mat &outputImage)
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

bool PMNMDetect_NS::CPMNMDetectV2_0::resizeAndPadImage2(cv::Mat inputImage, cv::Mat &outputImage)
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
TripLineType_E PMNMDetect_NS::CPMNMDetectV2_0::tripLineDetection(
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

bool PMNMDetect_NS::CPMNMDetectV2_0::isBoundingBoxIntersecting(
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

int PMNMDetect_NS::CPMNMDetectV2_0::crossProduct(
    const cv::Point &alertLineStart,
    const cv::Point &alertLineEnd,
    const cv::Point &testPoint)
{
    return (alertLineEnd.x - alertLineStart.x) * (testPoint.y - alertLineStart.y) -
           (alertLineEnd.y - alertLineStart.y) * (testPoint.x - alertLineStart.x);
}

/* 入侵检测：判断禁止入侵的区域有无点 */
bool PMNMDetect_NS::CPMNMDetectV2_0::intrusionZoneDetection(
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
bool PMNMDetect_NS::CPMNMDetectV2_0::entryZoneDetection(
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
bool PMNMDetect_NS::CPMNMDetectV2_0::leaveZoneDetection(
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
bool PMNMDetect_NS::CPMNMDetectV2_0::isIntersecting(
    std::vector<cv::Point> rectPolygon,
    std::vector<cv::Point> polygons)
{
    std::vector<cv::Point> intersectionPoints;
    cv::intersectConvexConvex(rectPolygon, polygons, intersectionPoints);
    return !intersectionPoints.empty();
}

/* 判断目标框与线段是否有交点 */
bool PMNMDetect_NS::CPMNMDetectV2_0::isLineIntersectingRect(
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

bool PMNMDetect_NS::CPMNMDetectV2_0::isTimeIntervalExceeded(int64_t nRecordTime, int nThresholdSec) 
{
    int64_t nCurrentTime = getSteadyTimeStampMs();
    
    int64_t nTimeDiffMs = nCurrentTime - nRecordTime;
#if HumanAreaDetect_DEBUG
    printf("====nTimeDiffMs[%lld]========\n",nTimeDiffMs);
#endif
    return (nTimeDiffMs > nThresholdSec * 1000);
}