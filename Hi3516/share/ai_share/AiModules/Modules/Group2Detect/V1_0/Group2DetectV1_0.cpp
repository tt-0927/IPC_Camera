/*
 * @Author: lianghy lianghy@kfb.cn
 * @Date: 2026-01-09 11:34:24
 * @LastEditors: lianghy lianghy@kfb.cn
 * @LastEditTime: 2026-04-07 14:35:48
 * @FilePath: /1126/share/ai_share/AiModules/Modules/Group2Detect/V1_0/Group2DetectV1_0.hpp
 * @Description: 人、车、非相关事件检测
 */

#include "Group2DetectV1_0.hpp"

#include "BYTETracker.h"
#include "SaveImage.hpp"
#include "StatisticsTimer.hpp"
#include <unistd.h>
#include <chrono>
using namespace Group2Detect_NS;

Group2Detect_NS::CGroup2DetectV1_0::CGroup2DetectV1_0(InParam_S stInParam)
    : m_stInParam(stInParam)
{
}

Group2Detect_NS::CGroup2DetectV1_0::~CGroup2DetectV1_0()
{
    unInit();
}

/* 初始化 */
bool Group2Detect_NS::CGroup2DetectV1_0::init()
{
    CStatisticsTimer runTime("边界检测初始化耗时");
    bool             bRet = false;

    m_pYoloUltralytics = new Inference_NS::CYoloUltralytics(m_stInParam.strModelPath);
    if (m_pYoloUltralytics && m_pYoloUltralytics->init())
    {
        m_pYoloUltralytics->getSizeLimit(
                0,
                m_nLimitWidth,
                m_nLimitHeight,
                m_nLimitChannel);
        bRet = true;
    }

    if (!bRet)
    {
        printf("模型初始化失败 [%s]\n", m_stInParam.strModelPath.c_str());
        goto FAIL;
    }

    for (int i = 0; i < 3; ++i)
    {
        bRet              = false;
        m_pByteTracker[i] = new Inference_NS::cBYTETracker();
        if (m_pByteTracker[i])
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
bool Group2Detect_NS::CGroup2DetectV1_0::unInit()
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

void Group2Detect_NS::CGroup2DetectV1_0::resetNonMotorVehicleIntrusionStatus()
{
    for (int i = 0; i < sizeof(m_stNonMotorVehicleIntrusion) / sizeof(NonMotorVehicleIntrusion_S); i++)
    {
        m_stNonMotorVehicleIntrusion[i].bNonMotorVehicleIntrusion = false;
    }
    return;
}

void Group2Detect_NS::CGroup2DetectV1_0::resetPedestrianIntrusionStatus()
{
    for (int i = 0; i < sizeof(m_stPedestrianIntrusion) / sizeof(Entry_S); i++)
    {
        m_stPedestrianIntrusion[i].bEntry = false;
    }
    return;
}

int Group2Detect_NS::CGroup2DetectV1_0::regularProcess(InData_S &stInData, const std::array<std::vector<DetectResult_S>, 3> &vecBoxs, OutData_S *pstOutData)
{
    const std::vector<DetectResult_S> &vstPersonBoxs          = vecBoxs[0];
    const std::vector<DetectResult_S> &vstMotorVehicleBoxs    = vecBoxs[1];
    const std::vector<DetectResult_S> &vstNonMotorVehicleBoxs = vecBoxs[2];

    /* ====================================== 非机动车闯入识别 ====================================== */
    bool bNonMotorVehicleIntrusionFlag = false;
    for (unsigned int i = 0; i < stInData.stParam.vstNonMotorVehicleIntrusionParam.size(); i++)
    {
        int                             nRet  = 0;
        NonMotorVehicleIntrusionParam_S Param = stInData.stParam.vstNonMotorVehicleIntrusionParam.at(i);
        if (Param.bEnable)
        {
            for (auto &vecBox : vstNonMotorVehicleBoxs)
            {
                if (vecBox.fConfidence >= Param.fNonMotorVehicleIntrusionThreshold)
                {
                    nRet += intrusionZoneDetection(
                        cv::Point(vecBox.vfBox.x + vecBox.vfBox.width / 2, vecBox.vfBox.y + vecBox.vfBox.height / 2),
                        Param.vecPoints);
                }
            }

            /* 判断当前侦测区域是否有进入禁止非机动车进入区域的目标 */
            if (nRet)
            {
                /* 该侦测区域第一次出现目标 */
                if (!m_stNonMotorVehicleIntrusion[i].bNonMotorVehicleIntrusion)
                {
                    // printf(" ============ 该侦测区域第一次出现目标 [%s]:[%d]============= \n", __FILE__, __LINE__);

                    m_stNonMotorVehicleIntrusion[i].bNonMotorVehicleIntrusion          = true;
                    m_stNonMotorVehicleIntrusion[i].nNonMotorVehicleIntrusionTimeStamp = getSteadyTimeStampMs();
                    // stNonMotorVehicleIntrusion.nAreaCode = i;
                }
                else
                {
                    /* 判断从进入检测区域到当前持续时间是否超过设定阈值 */
                    if (getSteadyTimeStampMs() - m_stNonMotorVehicleIntrusion[i].nNonMotorVehicleIntrusionTimeStamp >= Param.nNonMotorVehicleIntrusionTimeThreshold)
                    {
                        bNonMotorVehicleIntrusionFlag = true;
                        // printf(" =====区域 %d 达到非机动车闯入的时长阈值 =======%d \n", i, Param.nNonMotorVehicleIntrusionTimeThreshold);
                    }
                }
            }
            /* 检测到的目标全部都没在当前侦测区域 则重置该区域的状态 */
            else
            {
                m_stNonMotorVehicleIntrusion[i].bNonMotorVehicleIntrusion          = false;
                m_stNonMotorVehicleIntrusion[i].nNonMotorVehicleIntrusionTimeStamp = getSteadyTimeStampMs();
            }
        }
        else
        {
            m_stNonMotorVehicleIntrusion[i].bNonMotorVehicleIntrusion          = false;
            m_stNonMotorVehicleIntrusion[i].nNonMotorVehicleIntrusionTimeStamp = getSteadyTimeStampMs();
        }

        if (bNonMotorVehicleIntrusionFlag)
        {
            pstOutData->bNonMotorVehicleIntrusionFlag = true;
            break;
        }
    }
    /* ====================================== 非机动车闯入识别 ====================================== */
    
    /* ====================================== 非机动车车道检测 ====================================== */
    // 暂不清楚检测什么，只检测非机动车
    bool bNonVehicleLaneDetFlag = false;
    for (auto &Param : stInData.stParam.vstNonVehicleLaneDetParam)
    {
        for (auto &vecBox : vstNonMotorVehicleBoxs)
        {
            if (vecBox.fConfidence >= Param.fThreshold)
            {
                bNonVehicleLaneDetFlag = intrusionZoneDetection(
                        cv::Point(vecBox.vfBox.x + vecBox.vfBox.width / 2, vecBox.vfBox.y + vecBox.vfBox.height / 2),
                        Param.vecPoints);
                if(bNonVehicleLaneDetFlag)
                {
                    printf("【报警】达到识别到了非机动车车道的条件！\n");
                    pstOutData->bNonVehicleLaneDetFlag = true;
                    break;
                }
            }
        }
    }
    /* ====================================== 非机动车车道检测 ====================================== */

    /* ====================================== 电瓶车进电梯识别 ====================================== */
    // 标记当前帧是否识别到了电瓶车
    bool bDetectElectricScooter = false;
    if (stInData.stParam.stElectricScooterParam.bEnable)
    {
        for (auto &vecBox : vstNonMotorVehicleBoxs)
        {
            if (vecBox.fConfidence >= stInData.stParam.stElectricScooterParam.fConfidence)
            {
                bDetectElectricScooter = true;
                break;
            }
        }

        if (bDetectElectricScooter)
        {
            m_nElectricScooterFrameCount++;
            if (m_nElectricScooterFrameCount >= stInData.stParam.stElectricScooterParam.nDetectFrame)
            {
                printf("【报警】达到识别到了电瓶车的条件！\n");
                pstOutData->bElectricScooter = true;
                // m_nElectricScooterFrameCount = 0;
            }
        }
        /* 当前帧未识别到符合要求的非机动车，将检测到的帧重新置为0 */
        else
        {
            m_nElectricScooterFrameCount = 0;
        }
    }
    else
    {
        m_nElectricScooterFrameCount = 0;
    }
    /* ====================================== 电瓶车进电梯识别 ====================================== */

    /* ====================================== 人员聚集侦测 ====================================== */
    // bool bCrowdGatheringDetParamResult = false;
    int i = 0;
    for (auto &Param : stInData.stParam.vstCrowdGatheringDetParam)
    {
        // std::cout << "[算法]【聚集】enable:"<< Param.bEnable << "Threshold:" << Param.nProportionThreshold <<std::endl;
        if (Param.bEnable)
        {
            int nProportionThreshold = 0;
            for (auto &vBoxData : vstPersonBoxs)
            {
                /* 中心点 */
                cv::Point centerPoint(static_cast<int>((vBoxData.vfBox.x + vBoxData.vfBox.width / 2)),
                                      static_cast<int>((vBoxData.vfBox.y + vBoxData.vfBox.height / 2)));
                bool      bGatheringPeopleIntrusionFlag = intrusionZoneDetection(centerPoint, Param.vecPoints);
                if (bGatheringPeopleIntrusionFlag)
                {
                    nProportionThreshold++;
                }
                if (nProportionThreshold >= Param.nProportionThreshold)
                {
                    printf("区域%d人员聚集阈值%d >= %d\n", i, nProportionThreshold, Param.nProportionThreshold);
                    pstOutData->bCrowdGatheringDetParamFlag = true;
                    break;
                }
            }
        }
        i++;
    }
    /* ====================================== 人员聚集侦测 ====================================== */

    /* ====================================== 楼道拥挤侦测 ====================================== */
    for (auto &Param : stInData.stParam.vstStairwellDetParam)
    {
        // std::cout << "[算法]【楼道拥挤】enable:"<< Param.bEnable << "Threshold:" << Param.nProportionThreshold <<std::endl;
        if (Param.bEnable)
        {
            int nProportionThreshold = 0;
            for (auto &vBoxData : vstPersonBoxs)
            {
                /* 中心点 */
                cv::Point centerPoint(static_cast<int>((vBoxData.vfBox.x + vBoxData.vfBox.width / 2)),
                                      static_cast<int>((vBoxData.vfBox.y + vBoxData.vfBox.height / 2)));
                bool      bStairwellIntrusionFlag = intrusionZoneDetection(centerPoint, Param.vecPoints);
                if (bStairwellIntrusionFlag)
                {
                    nProportionThreshold++;
                }
                if (nProportionThreshold >= Param.nProportionThreshold)
                {
                    pstOutData->bStairwellIntrusionFlag = true;
                    break;
                }
            }
        }
    }
    /* ====================================== 楼道拥挤侦测 ====================================== */
    

    /* ====================================== 离岗检测 ====================================== */
    int nLeavePostDetectRegionId = 0;
    int64_t nCurrentTimeStamp = time(NULL);
    for (auto &Param : stInData.stParam.vstLeavePostParam)
    {
        // 检查该区域是否启用检测
        if (Param.bEnable)
        {
            // ===== 先完整遍历所有人员，判断该区域内是否有任意一人在岗 =====
            // 每个区域代表一个岗位位置，只要有任意人员在区域内即视为在岗
            bool bHasPersonInRegion = false;  // 标记该区域是否存在人员

            if (vstPersonBoxs.size())
            {
                for (auto &vBoxData : vstPersonBoxs)
                {
                    // 只处理置信度高于阈值的目标
                    if (vBoxData.fConfidence < Param.fLeavePostThreshold)
                    {
                        continue;
                    }
                    // 计算人员中心点
                    cv::Point centerPoint(
                        static_cast<int>((vBoxData.vfBox.x + vBoxData.vfBox.width / 2)),
                        static_cast<int>((vBoxData.vfBox.y + vBoxData.vfBox.height / 2)));
                    // 检测人员中心点是否在岗位区域内
                    if (intrusionZoneDetection(centerPoint, Param.vecPoints))
                    {
                        bHasPersonInRegion = true;
                        break;  // 已确认区域有人，无需继续检查其他人员
                    }
                }
            }

            // ===== 根据区域内是否有人，分别处理 =====
            if (bHasPersonInRegion)
            {
                // 区域内有人员在岗 → 重置离岗计时器
                m_llLeavePostStartTime[nLeavePostDetectRegionId] = 0;
            }
            else
            {
                // 区域内无人员 → 离岗计时
                if (m_llLeavePostStartTime[nLeavePostDetectRegionId] == 0)
                {
                    // 首次检测到离岗，记录开始时间
                    m_llLeavePostStartTime[nLeavePostDetectRegionId] = nCurrentTimeStamp;
                }

                // 离岗时间是否超过阈值
                if (m_llLeavePostStartTime[nLeavePostDetectRegionId] != 0
                    && (nCurrentTimeStamp - m_llLeavePostStartTime[nLeavePostDetectRegionId] >= Param.nTimeThreshold))
                {
                    // 离岗时间超过阈值，触发报警
                    pstOutData->bLeavePostFlag = true;
                }
            }

            // 如果触发报警，退出循环（不再检查后续区域）
            if (pstOutData->bLeavePostFlag)
            {
                break;
            }
        }
        nLeavePostDetectRegionId++;
    }
    /* ====================================== 离岗检测 ====================================== */

    /* ====================================== 图书馆空位检测 ====================================== */
    if(stInData.stParam.stLibraryVacanciesDetectParam.bEnable)
    {
        auto& param = stInData.stParam.stLibraryVacanciesDetectParam;
        if(vstPersonBoxs.size() > 0) // 有人
        {
            for (auto &vBoxData : vstPersonBoxs)
            {
                if(vBoxData.fConfidence < stInData.stParam.stLibraryVacanciesDetectParam.fThreshold)
                {
                    continue;
                }

                cv::Point centerPoint(static_cast<int>((vBoxData.vfBox.x + vBoxData.vfBox.width / 2)),
                                          static_cast<int>((vBoxData.vfBox.y + vBoxData.vfBox.height / 2)));

                bool bFlag = intrusionZoneDetection(centerPoint, param.vecPoints);
                if(bFlag)
                {
                    m_llLibraryTime[LEAVE_SEAT] = 0;
                    
                    if(m_llLibraryTime[ENTRY_SEAT] == 0)
                    {
                        m_llLibraryTime[ENTRY_SEAT] = nCurrentTimeStamp;
                    }
                    else
                    {
                        if(nCurrentTimeStamp - m_llLibraryTime[ENTRY_SEAT] > param.nEntryTimeThreshold)
                            pstOutData->nLibraryVacanciesType = ENTRY_SEAT;
                    }
                }
                else
                {
                    m_llLibraryTime[ENTRY_SEAT] = 0;

                    if(m_llLibraryTime[LEAVE_SEAT] == 0)
                    {
                        m_llLibraryTime[LEAVE_SEAT] = nCurrentTimeStamp;
                    }
                    else
                    {
                        if(nCurrentTimeStamp - m_llLibraryTime[LEAVE_SEAT] > param.nEntryTimeThreshold)
                            pstOutData->nLibraryVacanciesType = LEAVE_SEAT;
                    }
                }
            }
            
        }
        else
        {
            if(m_llLibraryTime[LEAVE_SEAT] == 0)
                m_llLibraryTime[LEAVE_SEAT] = nCurrentTimeStamp;
            
            if(nCurrentTimeStamp - m_llLibraryTime[LEAVE_SEAT] > param.nEntryTimeThreshold)
                pstOutData->nLibraryVacanciesType = LEAVE_SEAT;
        }
    }
    /* ====================================== 图书馆空位检测 ====================================== */


    /* ====================================== 行人闯入识别 ====================================== */
    bool bPedestrianIntrusionFlag = false;
    for (unsigned int i = 0; i < stInData.stParam.vstPedestrianIntrusionParam.size(); i++)
    {
        int                        nRet  = 0;
        PedestrianIntrusionParam_S Param = stInData.stParam.vstPedestrianIntrusionParam.at(i);
        if (Param.bEnable)
        {
            for (auto &vecBox : vstPersonBoxs)
            {
                if (vecBox.fConfidence >= Param.fPedestrianIntrusionThreshold)
                {
                    nRet += intrusionZoneDetection(
                        cv::Point(vecBox.vfBox.x + vecBox.vfBox.width / 2, vecBox.vfBox.y + vecBox.vfBox.height / 2),
                        Param.vecPoints);
                }
            }
            /* 判断当前侦测区域是否有进入禁止行人进入区域的目标 */
            if (nRet)
            {
                /* 该侦测区域第一次出现目标 */
                if (!m_stPedestrianIntrusion[i].bEntry)
                {
                    printf(" ============ 该侦测区域第一次出现目标 [%s]:[%d]============= \n", __FILE__, __LINE__);

                    m_stPedestrianIntrusion[i].bEntry          = true;
                    m_stPedestrianIntrusion[i].nEntryTimeStamp = getSteadyTimeStampMs();
                    // stPedestrianIntrusion.nAreaCode = i;
                }
                else
                {
                    /* 判断从进入检测区域到当前持续时间是否超过设定阈值 */
                    if (getSteadyTimeStampMs() - m_stPedestrianIntrusion[i].nEntryTimeStamp >= Param.nTimeThreshold)
                    {
                        bPedestrianIntrusionFlag = true;
                        printf(" =====区域 %d 达到行人闯入的时长阈值 =======%d \n", i, Param.nTimeThreshold);
                    }
                }
            }
            /* 检测到的目标全部都没在当前侦测区域 则重置该区域的状态 */
            else
            {
                m_stPedestrianIntrusion[i].bEntry          = false;
                m_stPedestrianIntrusion[i].nEntryTimeStamp = getSteadyTimeStampMs();
            }
        }
        else
        {
            m_stPedestrianIntrusion[i].bEntry          = false;
            m_stPedestrianIntrusion[i].nEntryTimeStamp = getSteadyTimeStampMs();
        }

        if (bPedestrianIntrusionFlag)
        {
            pstOutData->bPedestrianIntrusionFlag = true;
            break;
        }
    }
    /* ====================================== 行人闯入识别 ====================================== */

    /* ====================================== 拥堵识别检测 ====================================== */
    int nVehicleCount = 0;
    if(stInData.stParam.stCongestionParam.bEnable)
    {
        for (auto &BoxData : vstMotorVehicleBoxs)
        {
            if (BoxData.fConfidence >= stInData.stParam.stCongestionParam.fCongestionBoxThreshold)
            {
                nVehicleCount++;
            }
        }
        if (nVehicleCount >= stInData.stParam.stCongestionParam.nCongestionThreshold)
        {
            pstOutData->bCongestionFlag = true;
        }
    }
    /* ====================================== 拥堵识别检测 ====================================== */

    return 0;
}

/* 处理数据 */
bool Group2Detect_NS::CGroup2DetectV1_0::process(
    InData_S               stInData,
    std::vector<Result_S> &vecResult,
    std::vector<Result_S> &vecAllResult,
    OutData_S             *stOutData,
    std::vector<Result_S>* vecResultOne)
{
    OutData_S defaultOutData;

    // 如果传入的指针为空，则使用默认对象
    if (stOutData == nullptr)
    {
        stOutData = &defaultOutData;
    }

    vecResult.clear();
    if(vecResultOne)
    {
        vecResultOne->clear();
    }
    
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

    int tripLineType = -1;// 0横线 1竖线
    if (stInData.stParam.stTripLineParam.bEnable)
    {
        calculateAngleWithVertical(stInData.stParam.stHeadCountParam.alertLine1,stInData.stParam.stHeadCountParam.alertLine2) > 45 ?tripLineType = 0:tripLineType = 1;
    }
    int headLineType = -1;
    if (stInData.stParam.stHeadCountParam.bEnable)
    {
        calculateAngleWithVertical(stInData.stParam.stHeadCountParam.alertLine1,stInData.stParam.stHeadCountParam.alertLine2) > 45 ?headLineType = 0:headLineType = 1;
    }

    int tripAllowedType = 0;
    for(const auto& itr : stInData.stParam.stTripLineParam.veDetectionTargetTypes)
    {
        tripAllowedType |= 1 << itr;
    }


    Inference_NS::InputData_S stInputData;
    cv::Mat reMat;
    if(stInData.inMat.type() == CV_8UC3)
    {
        if(stInData.inMat.cols != m_nLimitWidth || stInData.inMat.rows != m_nLimitHeight)
        {
            resizeAndPadImage3(stInData.inMat,reMat);
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
            bRet = m_pYoloUltralytics->inference(stInputData, vBoxDatas);
            if (!bRet)
            {
                printf("算法分析失败\n");
                return false;
            }
        }
        else
        {
            printf("算法分析失败\n");
            return false;
        }
    }

    // std::vector<DetectResult_S> vecBoxs[3]; /* 0-人 1-机动车 2-非机动车 */
    std::array<std::vector<DetectResult_S>, 3> vecBoxs; /* 0-人 1-机动车 2-非机动车 */
    for (const auto &box : vBoxDatas)
    {
        DetectResult_S vecBox;
        vecBox.nClassId    = box.nLabel;
        vecBox.fConfidence = box.fConfidence;
        vecBox.vfBox       = cv::Rect_<float>(box.stBoxs.nX1, box.stBoxs.nY1, box.stBoxs.nX2 - box.stBoxs.nX1, box.stBoxs.nY2 - box.stBoxs.nY1);

        vecBoxs[box.nLabel].push_back(vecBox); /* box.nLabel: 0-人 1-机动车 2-非机动车 */

        Result_S stResult;

        stResult.nId            = 0;
        stResult.nID            = vecBox.nClassId;
        stResult.fX1            = (float)box.stBoxs.nX1;
        stResult.fY1            = (float)box.stBoxs.nY1;
        stResult.fX2            = (float)box.stBoxs.nX2;
        stResult.fY2            = (float)box.stBoxs.nY2;
        stResult.fBoxConfidence = vecBox.fConfidence;

        vecAllResult.push_back(stResult);
    }

    regularProcess(stInData, vecBoxs, stOutData);

    /* 跟踪算法+区域分析 */
    {
        int64_t nCurrentTimeStamp = time(NULL);

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
                Target_S          *pstTarget      = nullptr;
                int                nTargetId      = vecStracks[nClsIdx][nIndex].track_id;
                float              fBoxConfidence = vecStracks[nClsIdx][nIndex].score;
                std::vector<float> vectlwh        = vecStracks[nClsIdx][nIndex].tlwh;
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
                    cv::Point(vectlwh[0], vectlwh[1]),  // 左上角
                    cv::Point(vectlwh[2], vectlwh[1]),  // 右上角
                    cv::Point(vectlwh[2], vectlwh[3]),  // 右下角
                    cv::Point(vectlwh[0], vectlwh[3])   // 左下角
                };

                /* 框的长宽比 */
                float fAspectRatio = (vectlwh[3] - vectlwh[1]) / (vectlwh[2] - vectlwh[0]);
                if (nClsIdx == 0)
                {
                    /* 判断id在不在表里 */
                    if (m_mapPenson.count(nTargetId))
                    {
                        m_mapPenson[nTargetId].startPoint     = m_mapPenson[nTargetId].curPoint;
                        m_mapPenson[nTargetId].curPoint       = centerPoint;
                        m_mapPenson[nTargetId].bottomMidPoint = bottomMidPoint;
                        m_mapPenson[nTargetId].ndwellTime     = 0;
                        m_mapPenson[nTargetId].isUsed         = true;
                        if(tripAllowedType & (1 << nClsIdx))
                        {
                            m_TripLineStatus[nTargetId].isUsing = true;
                        }

                        if(stInData.stParam.stHeadCountParam.bEnable)
                        {
                            m_HeadCountStatus[nTargetId].isUsing = true;
                        }
                    }
                    else
                    {
                        Target_S newPenson;
                        newPenson.nId            = nTargetId;
                        newPenson.startPoint     = centerPoint;
                        newPenson.curPoint       = centerPoint;
                        newPenson.bottomMidPoint = bottomMidPoint;
                        newPenson.ndwellTime     = 0;
                        newPenson.isUsed         = true;
                        newPenson.fAspectRatio   = fAspectRatio;

                        m_mapPenson[nTargetId] = newPenson;

                        if(tripAllowedType & (1 << nClsIdx))
                        {
                            m_TripLineStatus[nTargetId].lastStatus = OVERFLOW_NONE;
                            m_TripLineStatus[nTargetId].lastlineType = -1;
                            m_TripLineStatus[nTargetId].lastlinePlace = -2;
                            m_TripLineStatus[nTargetId].lostFrameCount = 0;
                            m_TripLineStatus[nTargetId].isUsing = true;
                        }

                        
                        if(stInData.stParam.stHeadCountParam.bEnable)
                        {
                            m_HeadCountStatus[nTargetId].lastStatus = OVERFLOW_NONE;
                            m_HeadCountStatus[nTargetId].lastlineType = -1;
                            m_HeadCountStatus[nTargetId].lastlinePlace = -2;
                            m_HeadCountStatus[nTargetId].lostFrameCount = 0;
                            m_HeadCountStatus[nTargetId].isUsing = true;
                            m_HeadCountStatus[nTargetId].firstPlace = 0;
                            m_HeadCountStatus[nTargetId].lastPlace = 0;
                        }
                    }

                    pstTarget = &m_mapPenson[nTargetId];
                }
                else if (nClsIdx == 1)
                {
                    if (m_mapVehicle.count(nTargetId))
                    {
                        m_mapVehicle[nTargetId].startPoint     = m_mapVehicle[nTargetId].curPoint;
                        m_mapVehicle[nTargetId].curPoint       = centerPoint;
                        m_mapVehicle[nTargetId].bottomMidPoint = bottomMidPoint;
                        m_mapVehicle[nTargetId].ndwellTime     = 0;
                        m_mapVehicle[nTargetId].isUsed         = true;
                        if(tripAllowedType & (1 << nClsIdx))
                        {
                            m_TripLineStatus[nTargetId].isUsing = true;
                        }
                    }
                    else
                    {
                        Target_S newVehicle;
                        newVehicle.nId            = nTargetId;
                        newVehicle.startPoint     = centerPoint;
                        newVehicle.curPoint       = centerPoint;
                        newVehicle.bottomMidPoint = bottomMidPoint;
                        newVehicle.ndwellTime     = 0;
                        newVehicle.isUsed         = true;

                        m_mapVehicle[nTargetId] = newVehicle;

                        if(tripAllowedType & (1 << nClsIdx))
                        {
                            m_TripLineStatus[nTargetId].lastStatus = OVERFLOW_NONE;
                            m_TripLineStatus[nTargetId].lastlineType = -1;
                            m_TripLineStatus[nTargetId].lastlinePlace = -2;
                            m_TripLineStatus[nTargetId].lostFrameCount = 0;
                            m_TripLineStatus[nTargetId].isUsing = true;
                        }
                    }

                    pstTarget = &m_mapVehicle[nTargetId];
                }
                else if (nClsIdx == 2)
                {
                    if (m_mapNonMotorVehicle.count(nTargetId))
                    {
                        m_mapNonMotorVehicle[nTargetId].startPoint     = m_mapNonMotorVehicle[nTargetId].curPoint;
                        m_mapNonMotorVehicle[nTargetId].curPoint       = centerPoint;
                        m_mapNonMotorVehicle[nTargetId].bottomMidPoint = bottomMidPoint;
                        m_mapNonMotorVehicle[nTargetId].ndwellTime     = 0;
                        m_mapNonMotorVehicle[nTargetId].isUsed         = true;

                        if(tripAllowedType & (1 << nClsIdx))
                        {
                            m_TripLineStatus[nTargetId].isUsing = true;
                        }
                    }
                    else
                    {
                        Target_S newNonMotorVehicle;
                        newNonMotorVehicle.nId            = nTargetId;
                        newNonMotorVehicle.startPoint     = centerPoint;
                        newNonMotorVehicle.curPoint       = centerPoint;
                        newNonMotorVehicle.bottomMidPoint = bottomMidPoint;
                        newNonMotorVehicle.ndwellTime     = 0;
                        newNonMotorVehicle.isUsed         = true;

                        m_mapNonMotorVehicle[nTargetId] = newNonMotorVehicle;

                        if(tripAllowedType & (1 << nClsIdx))
                        {
                            m_TripLineStatus[nTargetId].lastStatus = OVERFLOW_NONE;
                            m_TripLineStatus[nTargetId].lastlineType = -1;
                            m_TripLineStatus[nTargetId].lastlinePlace = -2;
                            m_TripLineStatus[nTargetId].lostFrameCount = 0;
                            m_TripLineStatus[nTargetId].isUsing = true;
                        }

                    }

                    pstTarget = &m_mapNonMotorVehicle[nTargetId];
                }
                /* 区域分析 */
                Result_S stResult;

                stResult.nId = nTargetId;
                stResult.nID = nClsIdx;
                stResult.fX1 = vectlwh[0];
                stResult.fY1 = vectlwh[1];
                stResult.fX2 = vectlwh[0] + vectlwh[2];
                stResult.fY2 = vectlwh[1] + vectlwh[3];
                stResult.fBoxConfidence = fBoxConfidence;

                Result_S stHeadCountArea;
                stHeadCountArea.fX1 = vectlwh[0];
                stHeadCountArea.fY1 = vectlwh[1];
                stHeadCountArea.fX2 = vectlwh[0] + vectlwh[2];
                stHeadCountArea.fY2 = vectlwh[1] + vectlwh[3];
                /* 是否启用多区域对比 */
                if (!stInData.stParam.bVecEnable)
                {
                    /* 越界检测 */
                    if (stInData.stParam.stTripLineParam.bEnable && fBoxConfidence >= stInData.stParam.stTripLineParam.fTripLineThreshold)
                    {
                        for (const auto &classId : stInData.stParam.stTripLineParam.veDetectionTargetTypes)
                        {
                            if (classId == nClsIdx)
                            {
                                auto isIntersect = tripLineDetection(stHeadCountArea,
                                                            stInData.stParam.stTripLineParam.alertLine1,
                                                            stInData.stParam.stTripLineParam.alertLine2);
                                auto& tripLineItem = m_TripLineStatus[nTargetId];
                        
                                if(tripLineItem.lastStatus != isIntersect)
                                {
                                    // std::cout << std::endl << "[越界] personId:" << nTargetId << " 上次状态:" << tripLineItem.lastStatus << " 这次状态:" << isIntersect <<
                                    // std::endl;
                                    do{
                                        // std::cout << std::endl << "[越界] personId:" << nTargetId << " 上次line:" << tripLineItem.lastlineType << " 这次line:" << headLineType <<
                                        // std::endl;
                                        if(tripLineItem.lastlineType == -1)//初始化忽略不同
                                        {
                                            tripLineItem.lastlineType = tripLineType;
                                        }
                                        if(tripLineItem.lastlineType == tripLineType)
                                        {
                                            int cvPotPlace = -2;
                                            if(tripLineType == 0)//横线
                                            {
                                                cvPotPlace = pointAboveOrBelowLine(stInData.stParam.stTripLineParam.alertLine1,
                                                                                stInData.stParam.stTripLineParam.alertLine2,
                                                                                centerPoint);
                                                if(cvPotPlace > 0)
                                                {
                                                    std::cout<<std::endl<<"[越界][横线] [person " << nTargetId << "][cur][线 上方]" << std::endl;
                                                }
                                                else if(cvPotPlace < 0)
                                                {
                                                    std::cout<<std::endl<<"[越界][横线] [person " << nTargetId << "][cur][线 下方]" << std::endl;
                                                }

                                                if(tripLineItem.lastlinePlace == -2)//初始化忽略不同
                                                {
                                                    tripLineItem.lastlinePlace = cvPotPlace;
                                                    // std::cout << "[越界][横线] personId:" << nTargetId << " 首次初始化值 上次位置:" << tripLineItem.lastlinePlace 
                                                    // << " 这次位置:" << cvPotPlace <<std::endl;
                                                }
                                                else
                                                {
                                                    /**
                                                     * -1--1 = 0 同边未越界
                                                     * -1-0 = -1 压线越界 下-->上
                                                     * -1-1 = -2 不同边越界 下-->上
                                                     * 0--1 = 1 踩线越界 上-->下
                                                     * 0-0 = 0 压线未越界
                                                     * 0-1 = -1 不同边越界 下-->上
                                                     * 1--1 = 2 不同边越界 上-->下
                                                     * 1-0 = 1 压线越界 上-->下
                                                     * 1-1 = 0 同边未越界
                                                     * 
                                                     * >0 上-->下   <0 下-->上  ==0 未越界
                                                     */
                                                    if(tripLineItem.lastlinePlace - cvPotPlace > 0)
                                                    {
                                                        // std::cout << std::endl << "[越界][横线] [person " << nTargetId << "][上-->下]"<<"last:" 
                                                        // << tripLineItem.lastlinePlace << "cur:" << cvPotPlace << std::endl;
                                                        stOutData->bTripLineType = (int) OVERFLOW_A_TO_B;
                                                        tripLineItem.lastlinePlace = cvPotPlace;
                                                        if(vecResultOne)
                                                        {
                                                            vecResultOne->push_back(stResult);
                                                        }
                                                        break;
                                                    }
                                                    else if(tripLineItem.lastlinePlace - cvPotPlace < 0)
                                                    {
                                                        // std::cout << std::endl << "[越界][横线] [person " << nTargetId << "][下-->上]"<<"last:" 
                                                        // << tripLineItem.lastlinePlace << "cur:" << cvPotPlace << std::endl;
                                                        stOutData->bTripLineType = (int) OVERFLOW_B_TO_A;
                                                        tripLineItem.lastlinePlace = cvPotPlace;
                                                        if(vecResultOne)
                                                        {
                                                            vecResultOne->push_back(stResult);
                                                        }
                                                        break;
                                                    }

                                                    if(tripLineItem.lastlinePlace - cvPotPlace != 0)
                                                    {
                                                        stOutData->bTripLineType = (int) OVERFLOW_A_B_BOTH;
                                                        tripLineItem.lastlinePlace = cvPotPlace;
                                                        if(vecResultOne)
                                                        {
                                                            vecResultOne->push_back(stResult);
                                                        }
                                                        break;
                                                    }
                                                }

                                                
                                            }
                                            else if(tripLineType == 1)//竖线
                                            {
                                                cvPotPlace = pointLeftOrRightOfLine(stInData.stParam.stTripLineParam.alertLine1,
                                                                                    stInData.stParam.stTripLineParam.alertLine2,
                                                                                    centerPoint);
                                                
                                                if(cvPotPlace > 0)
                                                {
                                                    // std::cout<<std::endl<<"[越界][竖线] [person " << nTargetId << "][cur][线 右方]" << std::endl;
                                                }
                                                else if(cvPotPlace < 0)
                                                {
                                                    // std::cout<<std::endl<<"[越界][竖线] [person " << nTargetId << "][cur][线 左方]" << std::endl;
                                                }

                                                if(tripLineItem.lastlinePlace == -2)//初始化忽略不同
                                                {
                                                    tripLineItem.lastlinePlace = cvPotPlace;
                                                    // std::cout << "[越界][竖线] personId:" << nTargetId << " 首次初始化值 上次位置:" << tripLineItem.lastlinePlace 
                                                    // << " 这次位置:" << cvPotPlace <<std::endl;
                                                }
                                                else
                                                {
                                                    /**
                                                     * -1--1 = 0 同边未越界
                                                     * -1-0 = -1 压线越界 左-->右
                                                     * -1-1 = -2 不同边越界 左-->右
                                                     * 0--1 = 1 踩线越界 右-->左
                                                     * 0-0 = 0 压线未越界
                                                     * 0-1 = -1 踩线越界 左-->右
                                                     * 1--1 = 2 不同边越界 右-->左
                                                     * 1-0 = 1 压线越界 右-->左
                                                     * 1-1 = 0 同边未越界
                                                     * 
                                                     * >0 右-->左   <0 左-->右  ==0 未越界
                                                     */
                                                    if(tripLineItem.lastlinePlace - cvPotPlace > 0)
                                                    {
                                                        // std::cout << std::endl << "[越界][竖线] [person " << nTargetId << "][右-->左]" <<"last:" 
                                                        // << tripLineItem.lastlinePlace << "cur:" << cvPotPlace<< std::endl;
                                                        stOutData->bTripLineType = (int) OVERFLOW_B_TO_A;
                                                        tripLineItem.lastlinePlace = cvPotPlace;
                                                        if(vecResultOne)
                                                        {
                                                            vecResultOne->push_back(stResult);
                                                        }
                                                        break;
                                                    }
                                                    else if(tripLineItem.lastlinePlace - cvPotPlace < 0)
                                                    {
                                                        // std::cout << std::endl << "[越界][竖线] [person " << nTargetId << "][左-->右]" <<"last:" 
                                                        // << tripLineItem.lastlinePlace << "cur:" << cvPotPlace<< std::endl;
                                                        stOutData->bTripLineType = (int) OVERFLOW_A_TO_B;
                                                        tripLineItem.lastlinePlace = cvPotPlace;
                                                        if(vecResultOne)
                                                        {
                                                            vecResultOne->push_back(stResult);
                                                        }
                                                        break;
                                                    }

                                                    if(tripLineItem.lastlinePlace - cvPotPlace != 0)
                                                    {
                                                        stOutData->bTripLineType = (int) OVERFLOW_A_B_BOTH;
                                                        tripLineItem.lastlinePlace = cvPotPlace;
                                                        if(vecResultOne)
                                                        {
                                                            vecResultOne->push_back(stResult);
                                                        }
                                                        break;
                                                    }
                                                }
                                                
                                            }
                                        }
                                        else
                                        {
                                            tripLineItem.lastlineType = tripLineType;
                                            tripLineItem.lastlinePlace = -2;
                                        }
                                    }while(false);

                                    tripLineItem.lastStatus = isIntersect;
                                }

                                break;
                            }
                        }
                    }

                    /* 入侵检测 */
                    if (stInData.stParam.stIntrusionParam.bEnable && fBoxConfidence >= stInData.stParam.stIntrusionParam.fIntrusionThreshold)
                    {
                        bool bIntrusionFlag = false;
                        for (const auto &classId : stInData.stParam.stIntrusionParam.veDetectionTargetTypes)
                        {
                            if (classId == nClsIdx)
                            {
                                bIntrusionFlag = intrusionZoneDetection(
                                    centerPoint,
                                    stInData.stParam.stIntrusionParam.vecPoints);
                                break;
                            }
                        }

                        /* 检测到区域入侵 */
                        if(bIntrusionFlag)
                        {
                            /* 目标再次触发区域入侵 */
                            if(pstTarget->stIntrusion.bIntrusion)
                            {
                                /* 判断是否达到用户设置的区域入侵时间阈值 */
                                if((nCurrentTimeStamp - pstTarget->stIntrusion.nIntrusionTimeStamp) >= stInData.stParam.stIntrusionParam.nIntrusionTimeThreshold)
                                {
                                    stOutData->bIntrusionFlag = true;
                                    if(vecResultOne)
                                    {
                                        vecResultOne->push_back(stResult);
                                    }
                                    break;
                                }
                            }
                            /* 该目标第一次触发入侵检测，开始计时 */
                            else 
                            {
                                pstTarget->stIntrusion.bIntrusion = true;
                                pstTarget->stIntrusion.nIntrusionTimeStamp = nCurrentTimeStamp;
                            }
                        }
                        else 
                        {
                            /* 超过用户设定的时间不触发，重置状态 */
                            if(pstTarget->stIntrusion.nIntrusionTimeStamp != 0 && (nCurrentTimeStamp - pstTarget->stIntrusion.nIntrusionTimeStamp) >= stInData.stParam.stIntrusionParam.nIntrusionTimeThreshold)
                            {
                                pstTarget->stIntrusion.bIntrusion = false;
                                pstTarget->stIntrusion.nIntrusionTimeStamp = 0;
                            }
                        }
                    }

                    /* 进入检测 */
                    if (stInData.stParam.stEntryParam.bEnable && fBoxConfidence >= stInData.stParam.stEntryParam.fEntryThreshold)
                    {
                        for (const auto &classId : stInData.stParam.stEntryParam.veDetectionTargetTypes)
                        {
                            if (classId == nClsIdx && !stOutData->bEntryFlag)
                            {
                                stOutData->bEntryFlag |= entryZoneDetection(
                                    pstTarget->startPoint,
                                    centerPoint,
                                    stInData.stParam.stEntryParam.vecPoints);
                                if(stOutData->bEntryFlag)
                                {
                                    if(vecResultOne)
                                    {
                                        vecResultOne->push_back(stResult);
                                    }
                                }
                                break;
                            }
                        }
                    }

                    /* 离开检测 */
                    if (stInData.stParam.stLeaveParam.bEnable && fBoxConfidence >= stInData.stParam.stLeaveParam.fLeaveThreshold)
                    {
                        for (const auto &classId : stInData.stParam.stLeaveParam.veDetectionTargetTypes)
                        {
                            if (classId == nClsIdx && !stOutData->bLeaveFlag)
                            {
                                stOutData->bLeaveFlag |= leaveZoneDetection(
                                    pstTarget->startPoint,
                                    centerPoint,
                                    stInData.stParam.stLeaveParam.vecPoints);
                                if(stOutData->bLeaveFlag)
                                {
                                    if(vecResultOne)
                                    {
                                        vecResultOne->push_back(stResult);
                                    }
                                }
                                break;
                            }
                        }
                    }

                    /* 人流统计 */
                    if (stInData.stParam.stHeadCountParam.bEnable && fBoxConfidence >= stInData.stParam.stHeadCountParam.fThreshold)
                    {
                        //区域是否与线相交
                        auto isIntersect = tripLineDetection(stHeadCountArea,
                                                            stInData.stParam.stHeadCountParam.alertLine1,
                                                            stInData.stParam.stHeadCountParam.alertLine2);
                        auto& headCountItem = m_HeadCountStatus[nTargetId];

                        // std::cout << std::endl <<"[人流统计] 当前 personId:"  << nTargetId <<std::endl;
                        // if(headCountItem.lastStatus != isIntersect)
                        {
                            // std::cout <<  "[人流统计] personId:" << nTargetId << " 上次状态:" << headCountItem.lastStatus << " 这次状态:" << isIntersect <<
                            // std::endl;
                            do{
                                if(headCountItem.lastlineType == -1)//初始化忽略不同
                                {
                                    headCountItem.lastlineType = headLineType;
                                }
                                if(headCountItem.lastlineType == headLineType)
                                {
                                    int cvPotPlace = -2;
                                    // printf("remat(%d,%d) line1(%d,%d) line2(%d,%d) ",reMat.rows,reMat.cols,
                                    //                                                 stInData.stParam.stHeadCountParam.alertLine1.x,stInData.stParam.stHeadCountParam.alertLine1.y,
                                    //                                                 stInData.stParam.stHeadCountParam.alertLine2.x,stInData.stParam.stHeadCountParam.alertLine2.y);
                                    std::cout<<std::endl;

                                    auto now = std::chrono::system_clock::now();
                                    auto curtime = std::chrono::duration_cast<std::chrono::microseconds>(
                                                    now.time_since_epoch()
                                                ).count();
                                    if(headLineType == 0)//横线
                                    {
                                        cvPotPlace = pointAboveOrBelowLine(stInData.stParam.stHeadCountParam.alertLine1,
                                                                        stInData.stParam.stHeadCountParam.alertLine2,
                                                                        centerPoint);

                                        headCountItem.lastPlace = cvPotPlace;
                                        if(cvPotPlace > 0)
                                        {
                                            std::cout<<"[人流统计][横线] [person " << nTargetId << "][cur][线 上方]" << std::endl;
                                        }
                                        else if(cvPotPlace < 0)
                                        {
                                            std::cout<<"[人流统计][横线] [person " << nTargetId << "][cur][线 下方]" << std::endl;
                                        }
                                        
                                        if(headCountItem.lastlinePlace == -2)//初始化忽略不同
                                        {
                                            headCountItem.lastlinePlace = cvPotPlace;
                                            headCountItem.firstPlace = cvPotPlace;
                                            // cv::circle(reMat, centerPoint, 5 , cv::Scalar(0, 0, 255), cv::FILLED);
                                            std::cout << "[人流统计][横线] personId:" << nTargetId << " 首次初始化值 上次位置:" << headCountItem.lastlinePlace 
                                            << " 这次位置:" << cvPotPlace <<std::endl;
                                        }
                                        else
                                        {
                                            cv::circle(reMat, centerPoint, 5 , cv::Scalar(255, 0, 0), cv::FILLED);
                                            if(cvPotPlace!=0)
                                                std::cout << "[人流统计][横线] [person " << nTargetId << "] 首次位置:" << headCountItem.firstPlace << " 这次位置:" << cvPotPlace << std::endl;
                                            if(headCountItem.firstPlace - headCountItem.lastPlace > 0)
                                            {
                                                std::cout << "[人流统计][横线] [person " << nTargetId << "][上-->下]"<< std::endl;
                                                stOutData->nCountType.push_back((int)OVERFLOW_A_TO_B);
                                                headCountItem.lastlinePlace = cvPotPlace;
                                                headCountItem.init();
                                                if(vecResultOne)
                                                {
                                                    vecResultOne->push_back(stResult);
                                                }
                                            }
                                            else if(headCountItem.firstPlace - headCountItem.lastPlace < 0)
                                            {
                                                std::cout << "[人流统计][横线] [person " << nTargetId << "][下-->上]"<< std::endl;
                                                stOutData->nCountType.push_back((int)OVERFLOW_B_TO_A);
                                                headCountItem.lastlinePlace = cvPotPlace;
                                                headCountItem.init();
                                                if(vecResultOne)
                                                {
                                                    vecResultOne->push_back(stResult);
                                                }
                                            }
                                        }
                                    }
                                    else if(headLineType == 1)//竖线
                                    {
                                        cvPotPlace = pointLeftOrRightOfLine(stInData.stParam.stHeadCountParam.alertLine1,
                                                                            stInData.stParam.stHeadCountParam.alertLine2,
                                                                            centerPoint);
                                        headCountItem.lastPlace = cvPotPlace;
                                        if(cvPotPlace > 0)
                                        {
                                            std::cout<<"[人流统计][竖线] [person " << nTargetId << "][cur][线 右方]" << std::endl;
                                        }
                                        else if(cvPotPlace < 0)
                                        {
                                            std::cout<<"[人流统计][竖线] [person " << nTargetId << "][cur][线 左方]" << std::endl;
                                        }
                                        if(headCountItem.lastlinePlace == -2)//初始化忽略不同
                                        {
                                            headCountItem.lastlinePlace = cvPotPlace;
                                            headCountItem.firstPlace = cvPotPlace;
                                            // cv::circle(reMat, centerPoint, 5 , cv::Scalar(0, 0, 255), cv::FILLED);
                                            std::cout << "[人流统计][竖线] personId:" << nTargetId << " 首次初始化值 上次位置:" << headCountItem.lastlinePlace 
                                            << " 这次位置:" << cvPotPlace <<std::endl;
                                            
                                        }
                                        else
                                        {
                                            cv::circle(reMat, centerPoint, 5 , cv::Scalar(255, 0, 0), cv::FILLED);
                                            if(cvPotPlace!=0)
                                                std::cout << "[人流统计][竖线] [person " << nTargetId << "] 首次位置:" << headCountItem.firstPlace << " 这次位置:" << cvPotPlace << std::endl;
                                            if(headCountItem.firstPlace - headCountItem.lastPlace > 0)
                                            {
                                                std::cout << "[人流统计][竖线] [person " << nTargetId << "][右-->左]" << std::endl;
                                                stOutData->nCountType.push_back((int)OVERFLOW_B_TO_A);
                                                headCountItem.lastlinePlace = cvPotPlace;
                                                headCountItem.init();
                                                if(vecResultOne)
                                                {
                                                    vecResultOne->push_back(stResult);
                                                }
                                            }
                                            else if(headCountItem.firstPlace - headCountItem.lastPlace < 0)
                                            {
                                                std::cout << "[人流统计][竖线] [person " << nTargetId << "][左-->右]" << std::endl;
                                                stOutData->nCountType.push_back((int)OVERFLOW_A_TO_B);
                                                headCountItem.lastlinePlace = cvPotPlace;
                                                headCountItem.init();
                                                if(vecResultOne)
                                                {
                                                    vecResultOne->push_back(stResult);
                                                }
                                            }
                                        }
                                    }
                                    // if(access("/draw_poins",F_OK)==0 && cvPotPlace!=0 && cvPotPlace != -2)
                                    // {
                                    //     cv::Rect rect(stHeadCountArea.fX1,stHeadCountArea.fY1,stHeadCountArea.fX2 - stHeadCountArea.fX1,stHeadCountArea.fY2 - stHeadCountArea.fY1);
                                    //     cv::rectangle(reMat, rect, cv::Scalar(0, 255, 0), 1);
                                    //     cv::line(reMat, stInData.stParam.stHeadCountParam.alertLine1, stInData.stParam.stHeadCountParam.alertLine2, cv::Scalar(0, 0, 255), 2);
                                    //     cv::imwrite("/opt/tarId"+ std::to_string(nTargetId) +std::string("_") +std::to_string(cvPotPlace) + std::string("_") + std::to_string(curtime) +".jpg",reMat);
                                    // }
                                }
                                else
                                {
                                    headCountItem.lastlineType = headLineType;
                                    headCountItem.lastlinePlace = -2;
                                }
                            }while(false);

                            headCountItem.lastStatus = isIntersect;
                        }
                    }
                }
                else
                {
                    /* 越界检测 */
                    for (auto &Param : stInData.stParam.vstTripLineParam)
                    {
                        if (Param.bEnable && fBoxConfidence >= Param.fTripLineThreshold)
                        {
                            for (const auto &classId : Param.veDetectionTargetTypes)
                            {
                                if (classId == nClsIdx)
                                {
                                    bool bBidirectional = false;
                                    if (Param.eTripLineType == OVERFLOW_A_B_BOTH)
                                    {
                                        bBidirectional = true;
                                    }
                                    TripLineType_E enTripLineType = OVERFLOW_NONE;
                                    enTripLineType                = tripLineDetection(
                                        pstTarget->startPoint,
                                        centerPoint,
                                        Param.alertLine1,
                                        Param.alertLine2,
                                        bBidirectional);
                                    if (enTripLineType != OVERFLOW_NONE && enTripLineType == Param.eTripLineType)
                                    {
                                        stOutData->bTripLineType = true;
                                    }
                                    break;
                                }
                            }
                            if (stOutData->bTripLineType)
                            {
                                break;
                            }
                        }
                    }

                    /* 入侵检测 */
                    for (auto &Param : stInData.stParam.vstIntrusionParam)
                    {
                        /* 当前帧已经检测到触发入侵事件，不再进行检测 */
                        if (stOutData->bIntrusionFlag)
                        {
                            break;
                        }
                        bool bIntrusionFlag = false;
                        if (Param.bEnable && fBoxConfidence >= Param.fIntrusionThreshold)
                        {
                            for (const auto &classId : Param.veDetectionTargetTypes)
                            {
                                if (classId == nClsIdx)
                                {
                                    bIntrusionFlag = intrusionZoneDetection(
                                        centerPoint,
                                        Param.vecPoints);
                                    break;
                                }
                            }

                            /* 检测到区域入侵 */
                            if(bIntrusionFlag)
                            {
                                /* 目标再次触发区域入侵 */
                                if(pstTarget->stIntrusion.bIntrusion)
                                {
                                    /* 判断是否达到用户设置的区域入侵时间阈值 */
                                    if((nCurrentTimeStamp - pstTarget->stIntrusion.nIntrusionTimeStamp) >= Param.nIntrusionTimeThreshold)
                                    {
                                        stOutData->bIntrusionFlag = true;
                                    }
                                }
                                /* 该目标第一次触发入侵检测，开始计时 */
                                else 
                                {
                                    pstTarget->stIntrusion.bIntrusion = true;
                                    pstTarget->stIntrusion.nIntrusionTimeStamp = nCurrentTimeStamp;
                                }
                            }
                            else 
                            {
                                /* 超过用户设定的时间不触发，重置状态 */
                                if(pstTarget->stIntrusion.nIntrusionTimeStamp != 0 && (nCurrentTimeStamp - pstTarget->stIntrusion.nIntrusionTimeStamp) >= Param.nIntrusionTimeThreshold)
                                {
                                    pstTarget->stIntrusion.bIntrusion = false;
                                    pstTarget->stIntrusion.nIntrusionTimeStamp = 0;
                                }
                            }


                        }
                    }

                    /* 进入检测 */
                    for (auto &Param : stInData.stParam.vstEntryParam)
                    {
                        if (Param.bEnable && fBoxConfidence >= Param.fEntryThreshold)
                        {
                            for (const auto &classId : Param.veDetectionTargetTypes)
                            {
                                if (classId == nClsIdx)
                                {
                                    stOutData->bEntryFlag |= entryZoneDetection(
                                        pstTarget->startPoint,
                                        centerPoint,
                                        Param.vecPoints);
                                    break;
                                }
                            }

                            if (stOutData->bEntryFlag)
                            {
                                break;
                            }
                        }
                    }
                    /* 离开检测 */
                    for (auto &Param : stInData.stParam.vstLeaveParam)
                    {
                        if (Param.bEnable && fBoxConfidence >= Param.fLeaveThreshold)
                        {
                            for (const auto &classId : Param.veDetectionTargetTypes)
                            {
                                if (classId == nClsIdx)
                                {
                                    stOutData->bLeaveFlag |= leaveZoneDetection(
                                        pstTarget->startPoint,
                                        centerPoint,
                                        Param.vecPoints);
                                    break;
                                }
                            }

                            if (stOutData->bLeaveFlag)
                            {
                                break;
                            }
                        }
                    }

                    // int i = 0;
                    /* 应急车道检测区域 */
                    for (auto &Param : stInData.stParam.vstEmergencyLaneOccupancyParam)
                    {
                        if (!Param.bEnable || fBoxConfidence < Param.fEmergencyLaneOccupancyThreshold)
                        {
                            continue;
                        }

                        for (const auto &classId : Param.veDetectionTargetTypes)
                        {
                            if (classId != nClsIdx)
                            {
                                continue;
                            }
                            int nRet = intrusionZoneDetection(pstTarget->curPoint, Param.vecPoints);
                            if (nRet >= 0) /* 进入应急车道检测区域 */
                            {
                                if (!pstTarget->stEmergencyLaneOccupancy.bEmergencyLaneOccupancy) /* 记录第一次进入检测区域 */
                                {
                                    pstTarget->stEmergencyLaneOccupancy.bEmergencyLaneOccupancy          = true;
                                    pstTarget->stEmergencyLaneOccupancy.nEmergencyLaneOccupancyTimeStamp = getSteadyTimeStampMs();
                                    // printf(" =====区域 %d 出现应急车道占用 =======%d \n", i, Param.nEmergencyLaneOccupancyTimeThreshold);
                                }

                                // if(pstTarget->stEmergencyLaneOccupancy.bEmergencyLaneOccupancy)
                                else
                                {
                                    /* 判断从进入检测应急车道区域到当前持续时间是否超过设定阈值 */
                                    if (getSteadyTimeStampMs() - pstTarget->stEmergencyLaneOccupancy.nEmergencyLaneOccupancyTimeStamp >= Param.nEmergencyLaneOccupancyTimeThreshold)
                                    {
                                        stOutData->bEmergencyLaneOccupancyFlag = true;
                                        // printf(" =====区域 %d 达到应急车道占用的时间阈值 ======= %lld - %lld = %lld %d \n", i, getSteadyTimeStampMs(), pstTarget->stEmergencyLaneOccupancy.nEmergencyLaneOccupancyTimeStamp, getSteadyTimeStampMs() - pstTarget->stEmergencyLaneOccupancy.nEmergencyLaneOccupancyTimeStamp, Param.nEmergencyLaneOccupancyTimeThreshold);
                                    }
                                }
                            }
                            else /* 目标离开应急车道检测区域，重置 */
                            {
                                // printf(" =====区域 %d 离开应急车道 ======= \n", i);
                                pstTarget->stEmergencyLaneOccupancy.bEmergencyLaneOccupancy          = false;
                                pstTarget->stEmergencyLaneOccupancy.nEmergencyLaneOccupancyTimeStamp = getSteadyTimeStampMs();
                            }
                            break;
                        }

                        if (stOutData->bEmergencyLaneOccupancyFlag)
                        {
                            break;
                        }

                        // i++;
                    }

                    if (nClsIdx == PMNMClass::PERSON)
                    {
                        // std::cout << "[算法]【徘徊】vec.size:" << stInData.stParam.vsLoiteringParam.size();
                        /* 徘徊检测 */
                        for (auto &Param : stInData.stParam.vsLoiteringParam)
                        {
                            // std::cout << "[算法]【徘徊】enable:"<< Param.bEnable << "Threshold:" << Param.fLoiteringThreshold << "fBoxConfidence:" <<fBoxConfidence <<std::endl;
                            if (Param.bEnable && fBoxConfidence >= Param.fLoiteringThreshold)
                            {
                                if (!pstTarget->stLoitering.bLoiter)
                                {
                                    /* 是否徘徊 */
                                    if (isIntersecting(rectPoints, Param.vecPoints))
                                    {
#if Group2Detect_DEBUG
                                        printf("====personId[%d]===徘徊侦测 触发徘徊 获取当前时间戳=====\n", nTargetId);
#endif
                                        pstTarget->stLoitering.nLoiterTimeStamp = getSteadyTimeStampMs();
                                        pstTarget->stLoitering.bLoiter          = true;
                                        if(vecResultOne)
                                        {
                                            vecResultOne->push_back(stResult);
                                        }
                                    }
                                }
                                else
                                {
                                    if(vecResultOne)
                                    {
                                        vecResultOne->push_back(stResult);
                                    }
                                    if (!isIntersecting(rectPoints, Param.vecPoints) && getSteadyTimeStampMs() - pstTarget->stLoitering.nLoiterTimeStamp > ((float)Param.nTimeThreshold/4))
                                    {
#if Group2Detect_DEBUG
                                        printf("====personId[%d]===徘徊侦测 未达到时间阈值=====\n", nTargetId);
#endif
                                        pstTarget->stLoitering.bLoiter = false;
                                    }
                                    else
                                    {
#if Group2Detect_DEBUG
                                        printf("====personId[%d]===徘徊侦测 判断是否达到徘徊侦测时间阈值 [%d]秒=====\n", nTargetId, Param.nTimeThreshold);
#endif
                                        /* 时间阈值 */
                                        if (isTimeIntervalExceeded(pstTarget->stLoitering.nLoiterTimeStamp, Param.nTimeThreshold))
                                        {
#if Group2Detect_DEBUG
                                            printf("====personId[%d]===徘徊侦测 达到时间阈值=====\n", nTargetId);
#endif
                                            stOutData->bLoiteringFlag      = true;
                                            pstTarget->stLoitering.bLoiter = false;
                                        }
                                    }
                                }

                                if (stOutData->bLoiteringFlag)
                                {
                                    break;
                                }
                            }
                        }

                        /* 翻越围栏检测-note:连续检测多帧在区域内才触发 */
                        for (auto &Param : stInData.stParam.vstFenceClimbingParam)
                        {
                            if (Param.bEnable && fBoxConfidence >= Param.fFenceClimbingThreshold)
                            {
                                if (intrusionZoneDetection(bottomMidPoint, Param.vecPoints))
                                {
                                    pstTarget->stFenceClimbing.nFrameCount++;
                                }
                                else
                                {
                                    /* 是否需要连续检测到？？ */
                                    // pstTarget->stFenceClimbing.nFrameCount = 0;
                                }

                                if (pstTarget->stFenceClimbing.nFrameCount > Param.nDetectFrame)
                                {
                                    stOutData->bFenceClimbFlag             = true;
                                    pstTarget->stFenceClimbing.nFrameCount = 0;
                                }

                                if (stOutData->bFenceClimbFlag)
                                {
                                    break;
                                }
                            }
                        }

                        /* 人员倒地检测 */
                        if (stInData.stParam.stPersonFallDownParam.bEnable)
                        {
                            // printf(" ======= 框的置信度 = %f < 置信度阈值 = %f ======= %d \n", fBoxConfidence, stInData.stParam.stPersonFallDownParam.fPersonFallDownThreshold, __LINE__);
                            if (fBoxConfidence >= stInData.stParam.stPersonFallDownParam.fPersonFallDownThreshold)
                            {
                                int64_t llCurTimestamp = time(NULL);
                                if (pstTarget->fAspectRatio > 0 && fAspectRatio > 0)
                                {
                                    float fRatio = pstTarget->fAspectRatio / fAspectRatio;

                                    if (fRatio > 2)
                                    {
                                        if (m_llPersonFalldownTimestamp != 0 && llCurTimestamp - m_llPersonFalldownTimestamp >= stInData.stParam.stPersonFallDownParam.nTimeThreshold)
                                        {
                                            stOutData->bPersonFalldownFlag = true;
                                        }

                                        if (!m_llPersonFalldownTimestamp)
                                        {
                                            m_llPersonFalldownTimestamp = llCurTimestamp;
                                        }
                                    }
                                    else
                                    {
                                        if (m_llPersonFalldownTimestamp != 0 && llCurTimestamp - m_llPersonFalldownTimestamp > stInData.stParam.stPersonFallDownParam.nTimeThreshold)
                                        {
                                            m_llPersonFalldownTimestamp = 0;
                                        }
                                    }
                                }
                                pstTarget->fAspectRatio = fAspectRatio;
                            }
                        }
                    }

                    if (nClsIdx == PMNMClass::MOTOR_VEHICLE)
                    {
                        /* 违规变道检测区域 */
                        // int i = 0;
                        for (auto &Param : stInData.stParam.vstIllegalLaneChangeParam)
                        {
                            // i++;
                            if (Param.bEnable)
                            {
                                if (fBoxConfidence < Param.fIllegalLaneChangeBoxThreshold)
                                {
                                    // printf(" ======= 框的置信度 = %f < 置信度阈值 = %f ======= %d \n", fBoxConfidence, Param.fIllegalLaneChangeBoxThreshold, __LINE__);
                                    continue;
                                }
                                stOutData->bIllegalLaneChangeFlag |= doLinesIntersect(
                                    m_mapVehicle[nTargetId].startPoint, centerPoint, Param.alertLine1, Param.alertLine2);

                                if (stOutData->bIllegalLaneChangeFlag)
                                {
                                    // printf("区域 %d 触发违规变道\n", i);
                                    break;
                                }
                            }
                        }

                        // i = 0;
                        /* 逆行检测区域 */
                        for (auto &Param : stInData.stParam.vstDrivingAgainstTrafficParam)
                        {
                            // i++;
                            if (Param.bEnable)
                            {
                                if (fBoxConfidence < Param.fDrivingAgainstTrafficBoxThreshold)
                                {
                                    continue;
                                }
                                TripLineType_E enTripLineType = OVERFLOW_NONE;

                                enTripLineType = tripLineDetection(m_mapVehicle[nTargetId].startPoint, centerPoint, Param.alertLine1, Param.alertLine2, false);

                                /* 检测结果不为空，且不符合预设方向则判定为逆行 */
                                if (enTripLineType != OVERFLOW_NONE && enTripLineType != Param.eTripLineType)
                                {
                                    stOutData->bDrivingAgainstTrafficFlag = true;
                                    // printf(" 区域 %d 触发逆行 当前帧车辆框中点(%d, %d)\n", i, m_mapVehicle[nTargetId].curPoint.x, m_mapVehicle[nTargetId].curPoint.y);
                                    if (m_stInParam.bDebug)
                                    {
                                        /* 框 */
                                        cv::rectangle(
                                            stInData.inMat,
                                            cv::Rect(vectlwh[0], vectlwh[1], vectlwh[2], vectlwh[3]),
                                            cv::Scalar(0, 0, 255),
                                            4);

                                        if (!Modules_NS::saveImage(stInData.inMat, m_stInParam.strAnalyzeDataPath))
                                        {
                                            printf("Debug-保存图片失败[%s]\n", m_stInParam.strAnalyzeDataPath.c_str());
                                        }
                                    }

                                    break;
                                }
                            }
                        }

                        /* 停车检测区域 */
                        // i = 0;
                        for (auto &Param : stInData.stParam.vstParkingParam)
                        {
                            // std::cout << "[算法]【停车】enable:"<< Param.bEnable << "Threshold:" << Param.fParkingBoxThreshold << "fBoxConfidence:" <<fBoxConfidence <<std::endl;
                            // i++;
                            if (Param.bEnable)
                            {
                                if (fBoxConfidence < Param.fParkingBoxThreshold)
                                {
                                    // printf(" ===== 框的置信度 = %f < 置信度阈值 = %f===== %d \n", fBoxConfidence, Param.fParkingBoxThreshold, __LINE__);
                                    continue;
                                }
                                bool nRet = intrusionZoneDetection(m_mapVehicle[nTargetId].curPoint, Param.vecPoints);
                                if (nRet) /* 进入检测区域 */
                                {
                                    if (!m_mapVehicle[nTargetId].stParking.bParking) /* 记录第一次进入检测区域 */
                                    {
                                        // printf(" =====区域 %d 出现违停 =======%d \n", i, Param.nParkingTimeThreshold);
                                        m_mapVehicle[nTargetId].stParking.bParking          = true;
                                        m_mapVehicle[nTargetId].stParking.nParkingTimeStamp = getSteadyTimeStampMs();
                                    }
                                    else
                                    {
                                        /* 判断从进入检测停车区域到当前持续时间是否超过设定阈值 */
                                        if (getSteadyTimeStampMs() - m_mapVehicle[nTargetId].stParking.nParkingTimeStamp >= Param.nParkingTimeThreshold)
                                        {
                                            stOutData->bParkingFlag = true;
                                            // printf(" =====区域 %d 达到违停的阈值 =======%d \n", i, Param.nParkingTimeThreshold);
                                        }
                                    }
                                }
                                else /* 目标离开检测区域，重置 */
                                {
                                    m_mapVehicle[nTargetId].stParking.bParking          = false;
                                    m_mapVehicle[nTargetId].stParking.nParkingTimeStamp = getSteadyTimeStampMs();
                                }

                                if (stOutData->bParkingFlag)
                                {
                                    break;
                                }
                            }
                        }
                    }
                }
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

        for (auto pair = m_mapVehicle.begin(); pair != m_mapVehicle.end();)
        {
            int id = pair->first;
            if (m_mapVehicle[id].isUsed)
            {
                m_mapVehicle[id].isUsed = false;
            }
            else
            {
                m_mapVehicle[id].ndwellTime++;
            }

            if (m_mapVehicle[id].ndwellTime >= m_nMaxTimeLost)
            {
                pair = m_mapVehicle.erase(pair);
            }
            else
            {
                ++pair;
            }
        }

        for (auto pair = m_mapNonMotorVehicle.begin(); pair != m_mapNonMotorVehicle.end();)
        {
            int id = pair->first;
            if (m_mapNonMotorVehicle[id].isUsed)
            {
                m_mapNonMotorVehicle[id].isUsed = false;
            }
            else
            {
                m_mapNonMotorVehicle[id].ndwellTime++;
            }

            if (m_mapNonMotorVehicle[id].ndwellTime >= m_nMaxTimeLost)
            {
                pair = m_mapNonMotorVehicle.erase(pair);
            }
            else
            {
                ++pair;
            }
        }

        for(auto itr  = m_TripLineStatus.begin(); itr != m_TripLineStatus.end();)
        {
            if(itr->second.isUsing == true)
            {
                itr->second.isUsing = false;
                itr->second.lostFrameCount = 0;
            }
            else
            {
                itr->second.lostFrameCount++;
                
                if(itr->second.lostFrameCount >= m_nMaxTimeLost)
                {
                    itr = m_TripLineStatus.erase(itr);  // erase返回下一个迭代器
                }
                else
                {
                    ++itr;  // 不删除时手动递增
                }
            }
        }

        for(auto itr  = m_HeadCountStatus.begin(); itr != m_HeadCountStatus.end();)
        {
            if(itr->second.isUsing == true)
            {
                itr->second.isUsing = false;
                itr->second.lostFrameCount = 0;
            }
            else
            {
                itr->second.lostFrameCount++;
                
                if(itr->second.lostFrameCount >= m_nMaxTimeLost)
                {
                    if(itr->second.firstPlace!=0 && itr->second.firstPlace - itr->second.lastPlace > 0)
                    {
                        stOutData->nCountType.push_back((int)OVERFLOW_A_TO_B);
                    }
                    else if(itr->second.firstPlace!=0 && itr->second.firstPlace - itr->second.lastPlace < 0)
                    {
                        stOutData->nCountType.push_back((int)OVERFLOW_B_TO_A);
                    }
                    itr = m_HeadCountStatus.erase(itr);  // erase返回下一个迭代器
                }
                else
                {
                    ++itr;  // 不删除时手动递增
                }
            }
        }

        size_t total_elem_count = 0;
        for (const auto &vec : vecStracks)
        {
            total_elem_count += vec.size();
        }

        // int enType = 0;
        if (total_elem_count > 0)
        {
            const float scaleX = static_cast<float>(stInData.inMat.cols) / m_nLimitWidth;
            float scaleY = 1;
            if(stInData.inMat.type() == CV_8UC3)
            {
                scaleY = static_cast<float>(stInData.inMat.rows) / m_nLimitHeight;
            }
            else
            {
                scaleY = static_cast<float>(stInData.inMat.rows * 2 / 3) / m_nLimitHeight;
            }
            for (auto &result : vecResult)
            {
                // if (result.enTripLineType != 0)
                // {
                //     enType |= 0x01;  // Type_E::OVERSHOOT;
                // }

                // if (result.bIntrusionFlag)
                // {
                //     enType |= 0x02;  // Type_E::INTRUSION;
                // }

                // if (result.bEntryFlag)
                // {
                //     enType |= 0x04;  // Type_E::ENTRY;
                // }

                // if (result.bLeaveFlag)
                // {
                //     enType |= 0x08;  // Type_E::EXIT;
                // }

                // if (result.bEmergencyLaneOccupancyFlag)
                // {
                //     enType |= 0x10;  // Type_E::EMERGENCY_LANE_OCCUPANCY;
                // }

                // 0x10、0x20、0x40......
                if(stInData.inMat.type() == CV_8UC3)
                {
                    result.fX1 = static_cast<float>(std::max(0,((int)result.fX1 - m_nXOffset)) / m_fResizeScale);
                    result.fY1 = static_cast<float>(std::max(0,((int)result.fY1 - m_nYOffset)) / m_fResizeScale);
                    result.fX2 = static_cast<float>(std::max(0,((int)result.fX2 - m_nXOffset)) / m_fResizeScale);
                    result.fY2 = static_cast<float>(std::max(0,((int)result.fY2 - m_nYOffset)) / m_fResizeScale);
                }
                else
                {
                    result.fX1 *= scaleX;
                    result.fY1 *= scaleY;
                    result.fX2 *= scaleX;
                    result.fY2 *= scaleY;
                }
            }
            if(vecResultOne)
            {
                for (auto &result : *vecResultOne)
                {
                    if(stInData.inMat.type() == CV_8UC3)
                    {
                        result.fX1 = static_cast<float>(std::max(0,((int)result.fX1 - m_nXOffset)) / m_fResizeScale);
                        result.fY1 = static_cast<float>(std::max(0,((int)result.fY1 - m_nYOffset)) / m_fResizeScale);
                        result.fX2 = static_cast<float>(std::max(0,((int)result.fX2 - m_nXOffset)) / m_fResizeScale);
                        result.fY2 = static_cast<float>(std::max(0,((int)result.fY2 - m_nYOffset)) / m_fResizeScale);
                    }
                    else
                    {
                        result.fX1 *= scaleX;
                        result.fY1 *= scaleY;
                        result.fX2 *= scaleX;
                        result.fY2 *= scaleY;
                    }
                }
            }
        }

        // if (bNonMotorVehicleIntrusionFlag)
        // {
        //     enType |= 0x20;
        // }

        // if (enType != 0)
        // {
        //     // stOutData->validResult = true;
        // }

        // if(bTripLineFlag)
        // {

        // }

        // stOutData->nType  = enType;
        stOutData->nChnId = stInData.nChnId;
    }
    return true;
}

/* 处理数据 */
bool Group2Detect_NS::CGroup2DetectV1_0::resizeAndPadImage(cv::Mat inputImage, cv::Mat &outputImage)
{
    int imageWidth  = inputImage.cols;
    int imageHeight = inputImage.rows;
    m_fResizeScale  = static_cast<float>(m_nLimitWidth) / std::max(imageWidth, imageHeight);

    // std::cout << "imageWidth：" << imageWidth << std::endl;
    // std::cout << "imageHeight：" << imageHeight << std::endl;
    // std::cout << "m_nLimitWidth：" << m_nLimitWidth << std::endl;
    // std::cout << "m_fResizeScale" << m_fResizeScale << std::endl;

    int newWidth  = static_cast<int>(imageWidth * m_fResizeScale);
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

bool Group2Detect_NS::CGroup2DetectV1_0::resizeAndPadImage2(cv::Mat inputImage, cv::Mat &outputImage)
{
    int imageWidth  = inputImage.cols;
    int imageHeight = inputImage.rows;
    m_fResizeScale  = static_cast<float>(m_nLimitWidth) / std::max(imageWidth, imageHeight);

    int newWidth  = static_cast<int>(imageWidth * m_fResizeScale);
    int newHeight = static_cast<int>(imageHeight * m_fResizeScale);

    cv::Mat resizedImage;
    cv::resize(inputImage, resizedImage, cv::Size(newWidth, newHeight));

    m_nXOffset  = static_cast<int>((m_nLimitWidth - newWidth) / 2);
    m_nYOffset  = static_cast<int>((m_nLimitHeight - newHeight) / 2);
    int nBottom = m_nLimitHeight - newHeight - m_nYOffset;
    int mRight  = m_nLimitWidth - newWidth - m_nXOffset;

    cv::copyMakeBorder(resizedImage, outputImage, m_nYOffset, nBottom, m_nXOffset, mRight, cv::BORDER_CONSTANT, cv::Scalar(0, 0, 0));
    return true;
}

/* 拌线检测：判断两条线段是否有交点 */
TripLineType_E Group2Detect_NS::CGroup2DetectV1_0::tripLineDetection(
    const cv::Point &startPoint,
    const cv::Point &lastPoint,
    const cv::Point &alertLineFirst,
    const cv::Point &alertLineSecond,
    bool             bBidirectional

)
{
    TripLineType_E enType = OVERFLOW_NONE;

    if (startPoint.x == lastPoint.x && startPoint.y == lastPoint.y)
    {
        return enType;
    }

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
            lineEndPoint   = alertLineFirst;
        }
        else
        {
            lineStartPoint = alertLineFirst;
            lineEndPoint   = alertLineSecond;
        }
    }
    else
    {
        if (alertLineFirst.x < alertLineSecond.x)
        {
            lineStartPoint = alertLineFirst;
            lineEndPoint   = alertLineSecond;
        }
        else
        {
            lineStartPoint = alertLineSecond;
            lineEndPoint   = alertLineFirst;
        }
    }

    /* 计算叉积 */
    int crossStartPoint = crossProduct(lineStartPoint, lineEndPoint, startPoint);
    int crossLastPoint  = crossProduct(lineStartPoint, lineEndPoint, lastPoint);

    /* 判断A -> B */
    if (crossStartPoint < 0 && crossLastPoint > 0)
    {
        /* 拌线方向是A -> B */
        enType = OVERFLOW_A_TO_B;

        // std::cout << "拌线方向是A -> B :" << crossStartPoint << " " << crossLastPoint << std::endl;
    }

    /* 判断B -> A */
    if (crossStartPoint > 0 && crossLastPoint < 0)
    {
        /* 拌线方向是B -> A */
        enType = OVERFLOW_B_TO_A;
        // std::cout << "拌线方向是B -> A " << crossStartPoint << " " << crossLastPoint << std::endl;
    }

    if (bBidirectional)
    {
        /* 判断A <-> B */
        if ((crossStartPoint * crossLastPoint) < 0)
        {
            /* 拌线方向是A <-> B的其中一种 */
            enType = OVERFLOW_A_B_BOTH;
            // std::cout << "拌线方向是A <-> B的其中一种" << std::endl;
        }
    }

    return enType;
}

bool Group2Detect_NS::CGroup2DetectV1_0::isBoundingBoxIntersecting(
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

int Group2Detect_NS::CGroup2DetectV1_0::crossProduct(
    const cv::Point &alertLineStart,
    const cv::Point &alertLineEnd,
    const cv::Point &testPoint)
{
    return (alertLineEnd.x - alertLineStart.x) * (testPoint.y - alertLineStart.y) -
           (alertLineEnd.y - alertLineStart.y) * (testPoint.x - alertLineStart.x);
}

/* 入侵检测：判断禁止入侵的区域有无点 */
bool Group2Detect_NS::CGroup2DetectV1_0::intrusionZoneDetection(
    const cv::Point       &lastPoint,
    std::vector<cv::Point> polygons)
{
    double intrusionResult = cv::pointPolygonTest(polygons, lastPoint, false);
#if Group2Detect_DEBUG
    printf("区域入侵：lastPoint坐标 = (%d, %d), intrusionResult = %f, %s\n",
           lastPoint.x,
           lastPoint.y,
           intrusionResult,
           (intrusionResult > 0) ? "在区域内部" : ((intrusionResult == 0) ? "在区域边界上" : "在区域外部"));
#endif
    return intrusionResult >= 0;
}

/* 进入检测：根据起始点和当前点的关系，判断是否进入 */
bool Group2Detect_NS::CGroup2DetectV1_0::entryZoneDetection(
    const cv::Point       &startPoint,
    const cv::Point       &lastPoint,
    std::vector<cv::Point> polygons)
{
    double StartResult = cv::pointPolygonTest(polygons, startPoint, false);
#if Group2Detect_DEBUG
    printf("进入检测：startPoint坐标 = (%d, %d), StartResult = %f, %s\n",
           startPoint.x,
           startPoint.y,
           StartResult,
           (StartResult > 0) ? "在区域内部" : ((StartResult == 0) ? "在区域边界上" : "在区域外部"));
#endif
    if (StartResult >= 0)
        return false;
    double LastResult = cv::pointPolygonTest(polygons, lastPoint, false);
#if Group2Detect_DEBUG
    printf("进入检测：lastPointt坐标 = (%d, %d), LastResult = %f, %s\n",
           lastPoint.x,
           lastPoint.y,
           LastResult,
           (LastResult > 0) ? "在区域内部" : ((LastResult == 0) ? "在区域边界上" : "在区域外部"));
#endif
    return LastResult >= 0;
}

/* 离开检测：根据起始点和当前点的关系，判断是否离开 */
bool Group2Detect_NS::CGroup2DetectV1_0::leaveZoneDetection(
    const cv::Point       &startPoint,
    const cv::Point       &lastPoint,
    std::vector<cv::Point> polygons)
{
    double StartResult = cv::pointPolygonTest(polygons, startPoint, false);
#if Group2Detect_DEBUG
    printf("离开检测：startPoint坐标 = (%d, %d), StartResult = %f, %s\n",
           startPoint.x,
           startPoint.y,
           StartResult,
           (StartResult > 0) ? "在区域内部" : ((StartResult == 0) ? "在区域边界上" : "在区域外部"));
#endif
    if (StartResult < 0)
        return false;
    double LastResult = cv::pointPolygonTest(polygons, lastPoint, false);
#if Group2Detect_DEBUG
    printf("离开检测：lastPointt坐标 = (%d, %d), LastResult = %f, %s\n",
           lastPoint.x,
           lastPoint.y,
           LastResult,
           (LastResult > 0) ? "在区域内部" : ((LastResult == 0) ? "在区域边界上" : "在区域外部"));
#endif
    return LastResult < 0;
}

/* 判断两个多边形是否有交点 */
bool Group2Detect_NS::CGroup2DetectV1_0::isIntersecting(
    std::vector<cv::Point> rectPolygon,
    std::vector<cv::Point> polygons)
{
    std::vector<cv::Point> intersectionPoints;
    cv::intersectConvexConvex(rectPolygon, polygons, intersectionPoints);
    return !intersectionPoints.empty();
}

/* 判断目标框与线段是否有交点 */
bool Group2Detect_NS::CGroup2DetectV1_0::isLineIntersectingRect(
    const cv::Point &topLeft,
    const cv::Point &bottomRight,
    const cv::Point &alertLineFirst,
    const cv::Point &alertLineSecond)
{
    cv::Rect  cBoxRect(topLeft, bottomRight);
    cv::Point clippedStart = alertLineFirst;
    cv::Point clippedEnd   = alertLineSecond;
    return cv::clipLine(cBoxRect, clippedStart, clippedEnd);
}

bool Group2Detect_NS::CGroup2DetectV1_0::isTimeIntervalExceeded(int64_t nRecordTime, int nThresholdSec)
{
    int64_t nCurrentTime = getSteadyTimeStampMs();

    int64_t nTimeDiffMs = nCurrentTime - nRecordTime;
#if Group2Detect_DEBUG
    printf("====nTimeDiffMs[%ld]========\n", nTimeDiffMs);
#endif
    return (nTimeDiffMs > nThresholdSec * 1000);
}

/* 判断是否跨越线段 */
bool Group2Detect_NS::CGroup2DetectV1_0::doLinesIntersect(const cv::Point &p1,
                                                          const cv::Point &q1,
                                                          const cv::Point &p2,
                                                          const cv::Point &q2)
{
    int d1 = crossProduct(p2, q2, p1);
    int d2 = crossProduct(p2, q2, q1);
    int d3 = crossProduct(p1, q1, p2);
    int d4 = crossProduct(p1, q1, q2);

    /* 如果两条线段相交 */
    if (((d1 > 0 && d2 < 0) || (d1 < 0 && d2 > 0)) && ((d3 > 0 && d4 < 0) || (d3 < 0 && d4 > 0)))
        return true;

    /* 处理共线的特殊情况 */
    if (d1 == 0 && d2 == 0 && d3 == 0 && d4 == 0)
    {
        return isBoundingBoxIntersecting(p1, q1, p2, q2);
    }

    return false;
}

/* 处理数据 */
bool Group2Detect_NS::CGroup2DetectV1_0::resizeAndPadImage3(cv::Mat inputImage, cv::Mat &outputImage)
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


/* 判断人形区域与拌线是否有交集 */
TripLineType_E Group2Detect_NS::CGroup2DetectV1_0::tripLineDetection(
    Result_S& stResult,
    const cv::Point &alertLineFirst,
    const cv::Point &alertLineSecond)
{
    cv::Rect rect(stResult.fX1,stResult.fY1,stResult.fX2 - stResult.fX1,stResult.fY2 - stResult.fY1);
    if (rect.contains(alertLineFirst) || rect.contains(alertLineSecond)) 
    {
        return OVERFLOW_A_B_BOTH;
    }

    cv::Point clippedStart = alertLineFirst;
    cv::Point clippedEnd = alertLineSecond;
    bool bRes = cv::clipLine(rect, clippedStart, clippedEnd);
    return bRes?OVERFLOW_A_B_BOTH : OVERFLOW_NONE;
}


/**
 * @brief 判断点在线段的左侧还是右侧（适用于非垂直的线段）
 * @param linePt1 线段起点
 * @param linePt2 线段终点
 * @param point 待判断的点
 * @return -1: 左侧, 1: 右侧, 0: 在线段上或线段垂直
 */
int Group2Detect_NS::CGroup2DetectV1_0::pointLeftOrRightOfLine(const cv::Point2f& linePt1, const cv::Point2f& linePt2, const cv::Point2f& point) {
    cv::Point2f lineVec = linePt2 - linePt1;
    cv::Point2f pointVec = point - linePt1;
    
    // 1. 先检查点是否在线段的"投影范围"内
    float projection = pointVec.dot(lineVec);  // 点积
    float lineLengthSq = lineVec.dot(lineVec); // 线段长度平方
    
    // 如果投影值 < 0，点在起点之前（延长线上）
    // 如果投影值 > lineLengthSq，点在终点之后（延长线上）
    if (projection < 0 || projection > lineLengthSq) {
        // 点在线段延长线上，不在线段范围内
        return 0;  // 或返回其他特殊值
    }

    float dx = linePt2.x - linePt1.x; 
    float dy = linePt2.y - linePt1.y; 

    if (std::abs(dx) < 1e-6) {
        std::cout << std::endl <<" 线段近乎水平 " << std::endl;
        return 0;
    }
    
    // 计算叉积 (lineVec × pointVec)
    float crossProduct = lineVec.x * pointVec.y - lineVec.y * pointVec.x;
    
    if (crossProduct > 1e-6) {
        return -1;  // 左侧
    } else if (crossProduct < -1e-6) {
        return 1;   // 右侧
    } else {
        return 0;   // 在线段上或共线
    }
}

/**
 * @brief 判断点在线段的上方还是下方（适用于非水平的线段）
 * @param linePt1 线段起点
 * @param linePt2 线段终点
 * @param point 待判断的点
 * @return 1: 上方, -1: 下方, 0: 在线段上或线段水平
 */
int Group2Detect_NS::CGroup2DetectV1_0::pointAboveOrBelowLine(const cv::Point2f& linePt1, const cv::Point2f& linePt2, const cv::Point2f& point) 
{
    cv::Point2f lineVec = linePt2 - linePt1;
    cv::Point2f pointVec = point - linePt1;
    
    // 1. 先检查点是否在线段的"投影范围"内
    float projection = pointVec.dot(lineVec);  // 点积
    float lineLengthSq = lineVec.dot(lineVec); // 线段长度平方
    
    // 如果投影值 < 0，点在起点之前（延长线上）
    // 如果投影值 > lineLengthSq，点在终点之后（延长线上）
    if (projection < 0 || projection > lineLengthSq) {
        // 点在线段延长线上，不在线段范围内
        return 0;  // 或返回其他特殊值
    }

    float dx = linePt2.x - linePt1.x; 
    float dy = linePt2.y - linePt1.y; 

    if (std::abs(dy) < 1e-6) {
        std::cout << std::endl <<" 线段近乎水平 " << std::endl;
        return 0;
    }
    
    // 计算叉积 (lineVec × pointVec)
    float crossProduct = lineVec.x * pointVec.y - lineVec.y * pointVec.x;
    
    if (crossProduct > 1e-6) {
        return -1;   // 下方（图像坐标系）
    } else if (crossProduct < -1e-6) {
        return 1;  // 上方（图像坐标系）
    } else {
        return 0;   // 在线段上或共线
    }
}

/**
 * @brief 计算两条直线的夹角（锐角，0-90度）
 * @param linePt1 第一条直线的起点
 * @param linePt2 第一条直线的终点
 * @return 与垂直线的夹角（度）
 */
double Group2Detect_NS::CGroup2DetectV1_0::calculateAngleWithVertical(const cv::Point2f& linePt1, const cv::Point2f& linePt2) {
    // 计算线段方向向量
    cv::Point2f lineVec = linePt2 - linePt1;
    
    // 垂直线方向向量
    cv::Point2f verticalVec(0, 1);
    
    // 计算点积
    float dotProduct = lineVec.x * verticalVec.x + lineVec.y * verticalVec.y;
    
    // 计算模长
    float lineLength = sqrt(lineVec.x * lineVec.x + lineVec.y * lineVec.y);
    float verticalLength = 1.0f;  // 垂直向量长度为1
    
    // 计算夹角余弦值
    float cosAngle = dotProduct / (lineLength * verticalLength);
    
    // 确保在[-1, 1]范围内
    cosAngle = std::max(-1.0f, std::min(1.0f, cosAngle));
    
    // 计算夹角
    float angleRad = acos(cosAngle);
    
    // 确保是锐角
    float angleDeg = angleRad * 180.0f / CV_PI;
    if (angleDeg > 90.0f) {
        angleDeg = 180.0f - angleDeg;
    }
    
    return angleDeg;
}