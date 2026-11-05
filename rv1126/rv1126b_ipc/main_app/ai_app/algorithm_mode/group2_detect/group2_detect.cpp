/*
 * @Author: lianghy lianghy@kfb.cn
 * @Date: 2026-01-09 11:34:24
 * @LastEditors: xushou
 * @LastEditTime: 2026-11-03 11:07:00
 * @FilePath: rv1126b_ipc/main_app/ai_app/algorithm_mode/group2_detect/group2_detect.cpp
 * @Description: 人群聚集检测算法组件（从Group2DetectV1_0迁移）
 */

#include "group2_detect.hpp"

#include "BYTETracker.h"
#include "SaveImage.hpp"
#include "StatisticsTimer.hpp"
#include <unistd.h>
#include <chrono>

CGroup2Detect::CGroup2Detect(InParam_S stInParam)
    : m_stInParam(stInParam)
{
}

CGroup2Detect::~CGroup2Detect()
{
    unInit();
}

/* 初始化 */
bool CGroup2Detect::init()
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
bool CGroup2Detect::unInit()
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

void CGroup2Detect::resetNonMotorVehicleIntrusionStatus()
{
    for (int i = 0; i < sizeof(m_stNonMotorVehicleIntrusion) / sizeof(NonMotorVehicleIntrusion_S); i++)
    {
        m_stNonMotorVehicleIntrusion[i].bNonMotorVehicleIntrusion = false;
    }
    return;
}

void CGroup2Detect::resetPedestrianIntrusionStatus()
{
    for (int i = 0; i < sizeof(m_stPedestrianIntrusion) / sizeof(Entry_S); i++)
    {
        m_stPedestrianIntrusion[i].bEntry = false;
    }
    return;
}

int CGroup2Detect::regularProcess(InData_S &stInData, const std::array<std::vector<DetectResult_S>, 3> &vecBoxs, OutData_S *pstOutData)
{
    const std::vector<DetectResult_S> &vstPersonBoxs          = vecBoxs[0];
    const std::vector<DetectResult_S> &vstMotorVehicleBoxs    = vecBoxs[1];
    const std::vector<DetectResult_S> &vstNonMotorVehicleBoxs = vecBoxs[2];

    /* ====================================== 非机动车闯入识别 ====================================== */
    bool bNonMotorVehicleIntrusionFlag = false;
    for (unsigned int i = 0; i < stInData.stParam.vstNonMotorVehicleIntrusionParam.size(); i++)
    {
        const auto& Param = stInData.stParam.vstNonMotorVehicleIntrusionParam[i];
        if (Param.bEnable)
        {
            int nRet  = 0;
            for (int j = 0; j < vecBoxs.size(); j++)
            {
                const auto& vecBox = vecBoxs[j];

                bool bJump = true;
                for(const auto& targetType : Param.veDetectionTargetTypes)
                {
                    if(targetType == j) //如果当前目标类型在参数中
                    {
                        bJump = false;
                    }
                }

                if(bJump)
                {
                    continue;
                }

                for(const auto& box : vecBox)
                {
                    if (box.fConfidence >= Param.fNonMotorVehicleIntrusionThreshold)
                    {
                        nRet = intrusionZoneDetection(
                            cv::Point(box.vfBox.x + box.vfBox.width / 2, box.vfBox.y + box.vfBox.height / 2),
                            Param.vecPoints);
                        if(nRet)
                        {
                            break;
                        }
                    }
                }

            }

            if (nRet)
            {
                /* 该侦测区域第一次出现目标 */
                if (!m_stNonMotorVehicleIntrusion[i].bNonMotorVehicleIntrusion)
                {
                    m_stNonMotorVehicleIntrusion[i].bNonMotorVehicleIntrusion = true;
                    m_stNonMotorVehicleIntrusion[i].nNonMotorVehicleIntrusionTimeStamp = time(NULL);
                }
                else
                {
                    if (time(NULL) - m_stNonMotorVehicleIntrusion[i].nNonMotorVehicleIntrusionTimeStamp >= Param.nNonMotorVehicleIntrusionTimeThreshold)
                    {
                        bNonMotorVehicleIntrusionFlag = true;
                        m_stNonMotorVehicleIntrusion[i].bNonMotorVehicleIntrusion = false;
                    }
                }
            }
            else
            {
                m_stNonMotorVehicleIntrusion[i].bNonMotorVehicleIntrusion = false;
            }
        }
        else
        {
            m_stNonMotorVehicleIntrusion[i].bNonMotorVehicleIntrusion = false;
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
    for (int i = 0; i < stInData.stParam.vstNonVehicleLaneDetParam.size(); i++)
    {
        const auto& Param = stInData.stParam.vstNonVehicleLaneDetParam[i];
        bool bNonVehicleLaneDetFlag = false;
        if (Param.bEnable)
        {
            int nRet  = 0;
            for (int j = 0; j < vecBoxs.size(); j++)
            {
                const auto& vecBox = vecBoxs[j];

                bool bJump = true;
                for(const auto& targetType : Param.veDetectionTargetTypes)
                {
                    if(targetType == j) //如果当前目标类型在参数中
                    {
                        bJump = false;
                    }
                }

                if(bJump)
                {
                    continue;
                }

                for(const auto& box : vecBox)
                {
                    if(box.fConfidence >= Param.fThreshold)
                    {
                        nRet = intrusionZoneDetection(
                            cv::Point(box.vfBox.x + box.vfBox.width / 2, box.vfBox.y + box.vfBox.height / 2),
                            Param.vecPoints);
                        if(nRet)
                        {
                            break;
                        }
                    }
                }
            }
            
            if(nRet)
            {
                // 累加检测到非机动车车道的帧数
                m_stNonMotorVehicleLane[i].nNonMotorVehicleLineFrameCount++;
                if(m_stNonMotorVehicleLane[i].nNonMotorVehicleLineFrameCount >= Param.nFrameRateThreshold)
                {
                    bNonVehicleLaneDetFlag = true;
                }
            }
            else
            {
                // 重置该区域的闯入状态
                m_stNonMotorVehicleLane[i].nNonMotorVehicleLineFrameCount = 0;
            }
        }
        else
        {
            m_stNonMotorVehicleLane[i].nNonMotorVehicleLineFrameCount = 0;
        }

        if (bNonVehicleLaneDetFlag)
        {
            pstOutData->bNonVehicleLaneDetFlag = true;
            break;
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
            /* 该侦测区域第一次出现目标 */
            if (m_nElectricScooterTime == 0)
            {
                m_nElectricScooterTime = time(NULL);
            }
            else
            {
                if (time(NULL) - m_nElectricScooterTime >= stInData.stParam.stElectricScooterParam.nTimeThreshold)
                {
                    pstOutData->bElectricScooter = true;
                    m_nElectricScooterTime = 0;
                }
            }
        }
        else
        {
            m_nElectricScooterTime = 0;
        }
    }
    else
    {
        m_nElectricScooterTime = 0;
    }
    /* ====================================== 电瓶车进电梯识别 ====================================== */

    /* ====================================== 人员聚集侦测 ====================================== */
    // bool bCrowdGatheringDetParamResult = false;
    int CrowdGatheringArea = 0;
    for (auto &Param : stInData.stParam.vstCrowdGatheringDetParam)
    {
        if (Param.bEnable)
        {
            int nProportionThreshold = 0;
            for (auto &vBoxData : vstPersonBoxs)
            {
                /* 中心点 */
                std::vector<cv::Point> rectPoints = {
                    cv::Point(vBoxData.vfBox.x,vBoxData.vfBox.y),
                    cv::Point(vBoxData.vfBox.x+vBoxData.vfBox.width,vBoxData.vfBox.y),
                    cv::Point(vBoxData.vfBox.x+vBoxData.vfBox.width,vBoxData.vfBox.y+vBoxData.vfBox.height),
                    cv::Point(vBoxData.vfBox.x,vBoxData.vfBox.y+vBoxData.vfBox.height)
                };
                
                bool      bCrowdGatheringDetFlag = Modules_NS::isIntersecting(rectPoints, Param.vecPoints);
                if (bCrowdGatheringDetFlag)
                {
                    nProportionThreshold++;
                }
            }

            if (nProportionThreshold >= Param.nProportionThreshold)
            {
                if(nCrowdGatheringDetTime[CrowdGatheringArea] == 0)
                {
                    nCrowdGatheringDetTime[CrowdGatheringArea] = time(NULL);
                }

                if(time(NULL) - nCrowdGatheringDetTime[CrowdGatheringArea] > 1)
                {
                    pstOutData->bCrowdGatheringDetParamFlag = true;
                    break;
                }
            }
            else
            {
                nCrowdGatheringDetTime[CrowdGatheringArea] = 0;
            }

        }
        CrowdGatheringArea++;
        if(pstOutData->bCrowdGatheringDetParamFlag)
        {
            std::fill(std::begin(nStairwellDetTime), std::end(nStairwellDetTime), 0);
            break;
        }
    }

    /* ====================================== 人员聚集侦测 ====================================== */
    
    /* 楼道拥挤 */
    int StairwellArea = 0;
    for (auto &Param : stInData.stParam.vstStairwellDetParam)
    {
        if (Param.bEnable)
        {
            int nProportionThreshold = 0;
            for (auto &vBoxData : vstPersonBoxs)
            {
                /* 中心点 */
                // cv::Point centerPoint(static_cast<int>((vBoxData.vfBox.x + vBoxData.vfBox.width / 2)),
                //                       static_cast<int>((vBoxData.vfBox.y + vBoxData.vfBox.height / 2)));
                std::vector<cv::Point> rectPoints = {
                    cv::Point(vBoxData.vfBox.x,vBoxData.vfBox.y),
                    cv::Point(vBoxData.vfBox.x+vBoxData.vfBox.width,vBoxData.vfBox.y),
                    cv::Point(vBoxData.vfBox.x+vBoxData.vfBox.width,vBoxData.vfBox.y+vBoxData.vfBox.height),
                    cv::Point(vBoxData.vfBox.x,vBoxData.vfBox.y+vBoxData.vfBox.height)
                };
                
                bool      bStairwellDetFlag = Modules_NS::isIntersecting(rectPoints, Param.vecPoints);
                if (bStairwellDetFlag)
                {
                    nProportionThreshold++;
                }
            }

            if (nProportionThreshold >= Param.nProportionThreshold)
            {
                if(nStairwellDetTime[StairwellArea] == 0)
                {
                    nStairwellDetTime[StairwellArea] = time(NULL);
                }
                if(time(NULL) - nStairwellDetTime[StairwellArea] > 1)
                {
                    pstOutData->bStairwellIntrusionFlag = true;
                    break;
                }
            }
            else
            {
                nStairwellDetTime[StairwellArea] = 0;
            }
        }
        StairwellArea++;
        if(pstOutData->bStairwellIntrusionFlag)
        {
            std::fill(std::begin(nStairwellDetTime), std::end(nStairwellDetTime), 0);
            break;
        }
    }
    /* 楼道拥挤 */

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
    int nLibraryRegionId = 0;
    if(m_llLibraryTime.size() < stInData.stParam.stLibraryVacanciesDetectParam.size())
    {
        m_llLibraryTime.resize(stInData.stParam.stLibraryVacanciesDetectParam.size());
    }

    for (auto &Param : stInData.stParam.stLibraryVacanciesDetectParam)
    {
        // 检查该区域是否启用检测
        if (Param.bEnable)
        {
            // ===== 先完整遍历所有人员，判断该区域内是否有人占位 =====
            // 每个区域代表一个座位位置
            bool bHasPersonInRegion = false;  // 标记该区域是否存在人

            if (vstPersonBoxs.size())
            {
                for (auto &vBoxData : vstPersonBoxs)
                {
                    // 只处理置信度高于阈值的目标
                    if (vBoxData.fConfidence < Param.fThreshold)
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
                        break; // 有人
                    }
                }
            }

            // ===== 根据区域内是否有人，分别处理 =====
            if (bHasPersonInRegion)
            {
                // 区域内有人员 → 重置座位计时器
                m_llLibraryTime[nLibraryRegionId] = 0;
            }
            else
            {
                // 区域内无人员 → 计时
                if (m_llLibraryTime[nLibraryRegionId] == 0)
                {
                    // 首次检测到无人，记录开始时间
                    m_llLibraryTime[nLibraryRegionId] = nCurrentTimeStamp;
                }

                // 时间是否超过阈值
                if (m_llLibraryTime[nLibraryRegionId] != 0
                    && (nCurrentTimeStamp - m_llLibraryTime[nLibraryRegionId] >= Param.nLeaveTimeThreshold))
                {
                    // 时间超过阈值，触发报警，空位
                    pstOutData->bLibraryVacanciesType = true;
                }
            }

            // 如果触发报警，退出循环（不再检查后续区域）
            if (pstOutData->bLibraryVacanciesType)
            {
                break;
            }
        }
        nLibraryRegionId++;
    }
    /* ====================================== 图书馆空位检测 ====================================== */

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
bool CGroup2Detect::process(
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
    
    Modules_NS::OffsetScaRestor(vBoxDatas,m_nLimitWidth,m_nLimitHeight,stInData.inMat.cols,stInData.inMat.rows);

    std::array<std::vector<DetectResult_S>, 3> vecBoxs; /* 0-人 1-机动车 2-非机动车 */
    for (const auto &box : vBoxDatas)
    {
        DetectResult_S vecBox;
        vecBox.nClassId    = box.nLabel;
        vecBox.fConfidence = box.fConfidence;
        vecBox.vfBox       = cv::Rect_<float>(box.stBoxs.nX1, box.stBoxs.nY1, box.stBoxs.nX2 - box.stBoxs.nX1, box.stBoxs.nY2 - box.stBoxs.nY1);

        vecBoxs[box.nLabel].push_back(vecBox); /* box.nLabel: 0-人 1-机动车 2-非机动车 */

    }

    regularProcess(stInData, vecBoxs, stOutData);

    /* 跟踪算法+区域分析 */
    {
        int64_t nCurrentTimeStamp = time(NULL);

        CStatisticsTimer runTime("跟踪+后处理耗时");

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

                std::vector<cv::Point> rectPoints = {
                    cv::Point(vectlwh[0],vectlwh[1]),
                    cv::Point(vectlwh[0]+vectlwh[2],vectlwh[1]),
                    cv::Point(vectlwh[0]+vectlwh[2],vectlwh[1]+vectlwh[3]),
                    cv::Point(vectlwh[0],vectlwh[1]+vectlwh[3])
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

                        m_FenceClimbStatus[nTargetId].isUsing = true;
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
                            m_TripLineStatus[nTargetId].init();
                        }

                        
                        if(stInData.stParam.stHeadCountParam.bEnable)
                        {
                            m_HeadCountStatus[nTargetId].init();
                        }

                        m_FenceClimbStatus[nTargetId].init();
                        
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
                            m_TripLineStatus[nTargetId].init();
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
                            m_TripLineStatus[nTargetId].init();
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
                vecResult.push_back(stResult);

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
                                                                                centerPoint,false);
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
                                                                                    centerPoint,false);
                                                
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
                                                        // std::cout << std::endl << "[越界][竖线] [person " << nTargetId << "][左-->右]" <<"last:" 
                                                        // << tripLineItem.lastlinePlace << "cur:" << cvPotPlace<< std::endl;
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
                        auto& hc = m_HeadCountStatus[nTargetId];

                        // 冷却期内递减跳过，防止穿越后边界振荡导致重复计数
                        if (hc.nCooldownFrames > 0)
                        {
                            hc.nCooldownFrames--;
                        }
                        else
                        {
                            // 首次出现：记录当前位置作为穿越起点
                            if (hc.isFirst)
                            {
                                
                                hc.lastlineType = headLineType;

                                hc.firstPlace = pointSideOfLine(
                                    stInData.stParam.stHeadCountParam.alertLine1,
                                    stInData.stParam.stHeadCountParam.alertLine2, centerPoint,
                                    false);

                                
                                hc.lastPlace = hc.firstPlace;
                                hc.isFirst   = false;
                                // if(headLineType == 0)
                                // {
                                //     if(hc.firstPlace == 1)
                                //     {
                                //         printf("[人流统计] person[%d] 首次初始化位置:上方 \n", nTargetId);
                                //     }
                                //     else if(hc.firstPlace == -1)
                                //     {
                                //         printf("[人流统计] person[%d] 首次初始化位置:下方 \n", nTargetId);
                                //     } 
                                // }
                                // else
                                // {
                                //     if(hc.firstPlace == 1)
                                //     {
                                //         printf("[人流统计] person[%d] 首次初始化位置:左方 \n", nTargetId);
                                //     }
                                //     else if(hc.firstPlace == -1)
                                //     {
                                //         printf("[人流统计] person[%d] 首次初始化位置:右方 \n", nTargetId);
                                //     }
                                // }
                            }
                            else
                            {
                                // 计算当前位置
                                int cvPotPlace = -2;
                                cvPotPlace = pointSideOfLine(
                                    stInData.stParam.stHeadCountParam.alertLine1,
                                    stInData.stParam.stHeadCountParam.alertLine2, centerPoint);

                                hc.lastPlace = cvPotPlace;

                                // if(headLineType == 0)
                                // {
                                //     if(hc.firstPlace == 1)
                                //     {
                                //         printf("[人流统计] person[%d] 首次出现位置:上方 ", nTargetId);
                                //     }
                                //     else if(hc.firstPlace == -1)
                                //     {
                                //         printf("[人流统计] person[%d] 首次出现位置:下方 ", nTargetId);
                                //     }

                                //     if(hc.lastPlace == 1)
                                //     {
                                //         printf("这次位置:上方\n");
                                //     }
                                //     else if(hc.lastPlace == -1)
                                //     {
                                //         printf("这次位置:下方\n");
                                //     }
                                //     else
                                //     {
                                //         printf("这次位置:无效\n");
                                //     }
                                // }
                                // else
                                // {
                                //     if(hc.firstPlace == 1)
                                //     {
                                //         printf("[人流统计] person[%d] 首次出现位置:左方 ", nTargetId);
                                //     }
                                //     else if(hc.firstPlace == -1)
                                //     {
                                //         printf("[人流统计] person[%d] 首次出现位置:右方 ", nTargetId);
                                //     }

                                //     if(hc.lastPlace == 1)
                                //     {
                                //         printf("这次位置:左方\n");
                                //     }
                                //     else if(hc.lastPlace == -1)
                                //     {
                                //         printf("这次位置:右方\n");
                                //     }
                                //     else
                                //     {
                                //         printf("这次位置:无效\n");
                                //     }
                                // }

                                
                                // 穿越判定：首次位置与当前位置异号 = 到达线的另一侧（1 × -1 = -1）
                                if (hc.firstPlace * hc.lastPlace == -1)
                                {
                                    if (hc.firstPlace == 1)
                                    {
                                        stOutData->nCountType.push_back((int)OVERFLOW_A_TO_B);
                                        std::cout << "[人流统计][" << (headLineType == 0 ? "横线" : "竖线")
                                                  << "] [person " << nTargetId << "]["
                                                  << (headLineType == 0 ? "上-->下" : "左-->右") << "]"
                                                  << std::endl;
                                    }
                                    else // hc.firstPlace == -1
                                    {
                                        stOutData->nCountType.push_back((int)OVERFLOW_B_TO_A);
                                        std::cout << "[人流统计][" << (headLineType == 0 ? "横线" : "竖线")
                                                  << "] [person " << nTargetId << "]["
                                                  << (headLineType == 0 ? "下-->上" : "右-->左") << "]"
                                                  << std::endl;
                                    }
                                    hc.init();               // 重置穿越状态
                                    hc.isFirst = true;
                                    hc.nCooldownFrames = 30;  // 冷却10帧，防止边界振荡重复计数（可调）

                                    if (vecResultOne)
                                    {
                                        vecResultOne->push_back(stResult);
                                    }
                                }
                            }
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
                                    pstTarget->stEmergencyLaneOccupancy.nEmergencyLaneOccupancyTimeStamp = time(NULL);
                                }
                                else
                                {
                                    if (time(NULL) - pstTarget->stEmergencyLaneOccupancy.nEmergencyLaneOccupancyTimeStamp >= Param.nEmergencyLaneOccupancyTimeThreshold)
                                    {
                                        stOutData->bEmergencyLaneOccupancyFlag = true;
                                        pstTarget->stEmergencyLaneOccupancy.bEmergencyLaneOccupancy          = false;
                                    }
                                }
                            }
                            else
                            {
                                pstTarget->stEmergencyLaneOccupancy.bEmergencyLaneOccupancy          = false;
                            }
                        }

                        if (stOutData->bEmergencyLaneOccupancyFlag)
                        {
                            break;
                        }
                    }

                    if (nClsIdx == PMNMClass::PERSON)
                    {
                        /* 徘徊检测 */
                        int nLoiterZoneIdx = 0;  // 徘徊区域索引，对应vsLoiteringParam，每个区域使用独立的计时状态
                        for (auto &Param : stInData.stParam.vsLoiteringParam)
                        {
                            if (Param.bEnable && fBoxConfidence >= Param.fLoiteringThreshold)
                            {
                                // 确保每个目标的徘徊状态槽位数量与区域参数数量一致
                                if (static_cast<int>(pstTarget->vstLoitering.size()) <= nLoiterZoneIdx)
                                {
                                    pstTarget->vstLoitering.resize(nLoiterZoneIdx + 1);
                                }
                                auto &stLoiter = pstTarget->vstLoitering[nLoiterZoneIdx];

                                if (!stLoiter.bLoiter)
                                {
                                    /* 是否徘徊 - 检测目标矩形区域是否与徘徊区域相交 */
                                    if (Modules_NS::isIntersecting(rectPoints, Param.vecPoints))
                                    {
                                        stLoiter.nLoiterTimeStamp = time(NULL); //getSteadyTimeStampMs();
                                        stLoiter.bLoiter          = true;
                                    }
                                }
                                else
                                {
                                    if (!Modules_NS::isIntersecting(rectPoints, Param.vecPoints))
                                    {
                                        stLoiter.bLoiter = false;
                                    }
                                    else
                                    {
                                        /* 时间阈值 - 判断是否超过设定的时间阈值 */
                                        if (time(NULL) - stLoiter.nLoiterTimeStamp >= Param.nTimeThreshold)
                                        {
                                            stOutData->bLoiteringFlag = true;
                                            stLoiter.bLoiter          = false;
                                        }
                                    }
                                }

                                if (stOutData->bLoiteringFlag)
                                {
                                    break;
                                }
                            }
                            nLoiterZoneIdx++;
                        }

                        /* 行人闯入 */
                        int nPerEntryZoneIdx = 0;  // 徘徊区域索引，对应vsLoiteringParam，每个区域使用独立的计时状态
                        for (auto &Param : stInData.stParam.vstPedestrianIntrusionParam)
                        {
                            if (Param.bEnable && fBoxConfidence >= Param.fPedestrianIntrusionThreshold)
                            {
                                // 确保每个目标的徘徊状态槽位数量与区域参数数量一致
                                if (static_cast<int>(pstTarget->vstPerEntrying.size()) <= nPerEntryZoneIdx)
                                {
                                    pstTarget->vstPerEntrying.resize(nPerEntryZoneIdx + 1);
                                }
                                // 获取当前区域对应的独立徘徊状态（每个区域独立计时，互不干扰）
                                auto &stPerEntry = pstTarget->vstPerEntrying[nPerEntryZoneIdx];

                                if (!stPerEntry.bEntry)
                                {
                                    /* 是否徘徊 - 检测目标矩形区域是否与徘徊区域相交 */
                                    if (Modules_NS::isIntersecting(rectPoints, Param.vecPoints))
                                    {
                                        stPerEntry.nEntryTimeStamp = time(NULL); //getSteadyTimeStampMs();
                                        stPerEntry.bEntry          = true;
                                    }
                                }
                                else
                                {
                                    if (!Modules_NS::isIntersecting(rectPoints, Param.vecPoints))
                                    {
                                        // 目标离开当前徘徊区域，取消该区域的计时
                                        stPerEntry.bEntry = false;
                                    }
                                    else
                                    {
                                        /* 时间阈值 - 判断是否超过设定的时间阈值 */
                                        if (time(NULL) - stPerEntry.nEntryTimeStamp >= Param.nTimeThreshold)
                                        {
                                            stOutData->bPedestrianIntrusionFlag = true;
                                            stPerEntry.bEntry          = false;
                                        }
                                    }
                                }

                                if (stOutData->bPedestrianIntrusionFlag)
                                {
                                    break;
                                }
                            }
                            nPerEntryZoneIdx++;
                        }
                        /* 行人闯入*/

                        /* 翻越围栏检测-note:连续检测多帧在区域内才触发 */
                        for (auto &Param : stInData.stParam.vstFenceClimbingParam)
                        {
                            if (Param.bEnable)
                            {
                                if(Param.vecPoints.size() < 4)
                                {
                                    continue;
                                }

                                cv::Point Line1[2];
                                cv::Point Line2[2];
                                if(calculateAngleWithVertical(Param.vecPoints[0],Param.vecPoints[1]) > 45 )
                                {
                                    Line1[0] = Param.vecPoints[0];
                                    Line1[1] = Param.vecPoints[1];
                                    Line2[0] = Param.vecPoints[2];
                                    Line2[1] = Param.vecPoints[3];
                                }
                                else
                                {
                                    Line1[0] = Param.vecPoints[0];
                                    Line1[1] = Param.vecPoints[3];
                                    Line2[0] = Param.vecPoints[1];
                                    Line2[1] = Param.vecPoints[2];
                                }


                                auto& fc = m_FenceClimbStatus[nTargetId];

                                if (fc.nCrossStage == 0)
                                {
                                    // 阶段0：未穿越任何线，检测第一条线穿越
                                    int nPos1 = pointSideOfLine(Line1[0], Line1[1], centerPoint);
                                    if (nPos1 != 0)
                                    {
                                        fc.nLine1First = nPos1;
                                        fc.nLine1Last = nPos1;
                                        fc.nCrossStage = 1;
                                    }
                                }
                                else if (fc.nCrossStage == 1)
                                {
                                    // 阶段1：已穿越第一条线，检测第二条线穿越
                                    fc.nLine1Last = pointSideOfLine(Line1[0], Line1[1], centerPoint);
                                    // 先验证第一条线确实已穿越（防止状态误判）
                                    if (fc.nLine1First * fc.nLine1Last == -1)
                                    {
                                        int nPos2 = pointSideOfLine(Line2[0], Line2[1], centerPoint);
                                        if (nPos2 != 0)
                                        {
                                            fc.nLine2First = nPos2;
                                            fc.nLine2Last = nPos2;
                                            fc.nCrossStage = 2;
                                        }
                                    }
                                }
                                else if (fc.nCrossStage == 2)
                                {
                                    // 阶段2：已穿越两条线，检测第二条线的最终穿越
                                    fc.nLine2Last = pointSideOfLine(Line2[0], Line2[1], centerPoint);
                                    if (fc.nLine2First * fc.nLine2Last == -1)
                                    {
                                        stOutData->bFenceClimbFlag = true;
                                        fc.init();
                                    }
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
                    
                        /* ====================================== 楼道拥挤侦测 ====================================== */
                        // 说明：由于楼道场景存在高低差视角问题，纯人数计数不可靠，
                        // 改用两种触发条件：1)人员静止不动超时  2)时间窗口内新增人员超阈值
                        int nStairwellZoneIdx = 0;  // 区域索引，对应vstStairwellDetParam和m_CrowdStatus
                        #if 0
                        for (auto &Param : stInData.stParam.vstStairwellDetParam)
                        {
                            if (!Param.bEnable)
                            {
                                nStairwellZoneIdx++;
                                continue;
                            }

                            auto &stCrowd              = m_CrowdStatus[nStairwellZoneIdx];
                            // 设置区域时间阈值，人数阈值
                            stCrowd.nStaticTimeThreshold = (20 - Param.nProportionThreshold)* 0.6 >0 ? (20 - Param.nProportionThreshold)* 0.6 : 3; // 区域过期时间 
                            stCrowd.nNewPersonThreshold = Param.nProportionThreshold;  // 时间阈值内多少个新增人员id

                            bool  bHasPersonInThisZone = false;  // 本帧区域内是否有人

                            // 重置本帧人员出现标记（用于后续清理离区人员的静止状态）
                            stCrowd.mapSeenCurFrame.clear();

                            
                            {
                                // 检测人员中心点是否在楼道检测区域内
                                if (!intrusionZoneDetection(centerPoint, Param.vecPoints))
                                {
                                    continue;  // 不在区域内，跳过
                                }

                                bHasPersonInThisZone = true;
                                int      nPersonId = nTargetId;

                                cv::Rect curBox(
                                    static_cast<int>(vectlwh[0]),
                                    static_cast<int>(vectlwh[1]),
                                    static_cast<int>(vectlwh[2]),
                                    static_cast<int>(vectlwh[3]));

                                // 标记该人员本帧出现在此区域内
                                stCrowd.mapSeenCurFrame[nPersonId] = true;

                                // ============================================================
                                // 条件1：人员静止不动检测
                                // 连续多帧 IOU >= fStaticIouThreshold → 视为静止
                                // 静止持续 >= nStaticTimeThreshold 秒 → 触发报警
                                // ============================================================
                                bool bHasPrevBox = (stCrowd.mapLastPersonBox.find(nPersonId) != stCrowd.mapLastPersonBox.end());

                                if (bHasPrevBox)
                                {
                                    cv::Rect &prevBox     = stCrowd.mapLastPersonBox[nPersonId];
                                    cv::Rect  interRect   = prevBox & curBox;                   // 交集
                                    double    interArea   = static_cast<double>(interRect.area());
                                    double    prevArea    = static_cast<double>(prevBox.area());
                                    double    curArea     = static_cast<double>(curBox.area());
                                    double    unionArea   = prevArea + curArea - interArea;
                                    double    dIou        = (unionArea > 0.0) ? (interArea / unionArea) : 0.0;

                                    if (dIou >= stCrowd.fStaticIouThreshold)
                                    {
                                        // IOU达标，人员趋于静止
                                        if (!stCrowd.mapStaticTracking[nPersonId])
                                        {
                                            // 首次达到静止条件 → 开始计时
                                            stCrowd.mapStaticTracking[nPersonId]   = true;
                                            stCrowd.mapStaticStartTime[nPersonId]  = time(NULL);
                                        }
                                        else
                                        {
                                            // 已在静止计时中 → 检查是否超过时间阈值
                                            if (time(NULL) - stCrowd.mapStaticStartTime[nPersonId] >= stCrowd.nStaticTimeThreshold)
                                            {
                                                std::cout << "ai_app IOU 触发... IOU: " << dIou << "停留时间: " << stCrowd.nStaticTimeThreshold <<std::endl;
                                                stOutData->bStairwellIntrusionFlag = true;
                                            }
                                        }
                                    }
                                    else
                                    {
                                        // IOU不足，人员有移动 → 重置静止状态
                                        stCrowd.mapStaticTracking[nPersonId] = false;
                                    }
                                }
                                // 无论是否有历史框，都更新上一帧框的记录
                                stCrowd.mapLastPersonBox[nPersonId] = curBox;

                                // ============================================================
                                // 条件2：新增人员数量检测
                                // 在 nWindowDuration 秒窗口内，新增人员ID数 >= nNewPersonThreshold → 触发报警
                                // ============================================================
                                // 窗口初始化
                                if (stCrowd.nWindowStartTime == 0)
                                {
                                    stCrowd.nWindowStartTime = time(NULL);
                                }

                                // 窗口过期 → 重置窗口，清空已计数人员
                                if (time(NULL) - stCrowd.nWindowStartTime > stCrowd.nWindowDuration)
                                {
                                    stCrowd.nWindowStartTime = time(NULL);
                                    stCrowd.mapCountedPerson.clear();
                                }

                                // 新增人员去重计数
                                if (stCrowd.mapCountedPerson.find(nPersonId) == stCrowd.mapCountedPerson.end())
                                {
                                    stCrowd.mapCountedPerson[nPersonId] = true;
                                    int nCount = static_cast<int>(stCrowd.mapCountedPerson.size());

                                    if (nCount >= stCrowd.nNewPersonThreshold)
                                    {
                                        std::cout << "ai_app 人数过多 触发... 人数: " << nCount << " 平均停留时间: "<< (float)nCount/stCrowd.nStaticTimeThreshold << std::endl;
                                        stOutData->bStairwellIntrusionFlag = true;
                                    }
                                }
                            }
                            

                            // ============================================================
                            // 清理不在本帧出现的离区人员的静止追溯状态
                            // ============================================================
                            for (auto it = stCrowd.mapStaticTracking.begin(); it != stCrowd.mapStaticTracking.end(); )
                            {
                                int nPersonId = it->first;
                                if (stCrowd.mapSeenCurFrame.find(nPersonId) == stCrowd.mapSeenCurFrame.end() && stCrowd.mapStaticStartTime[nPersonId]*1000 - getSteadyTimeStampMs() > (float)stCrowd.nStaticTimeThreshold*0.2f)
                                {
                                    // 该人员已不在区域内 → 清除所有相关状态
                                    stCrowd.mapLastPersonBox.erase(nPersonId);
                                    stCrowd.mapStaticStartTime.erase(nPersonId);
                                    it = stCrowd.mapStaticTracking.erase(it);
                                }
                                else
                                {
                                    ++it;
                                }
                            }

                            if (stOutData->bStairwellIntrusionFlag)
                            {
                                break;  // 已触发，不再检查后续区域
                            }
                            nStairwellZoneIdx++;
                        }
                        /* ====================================== 楼道拥挤侦测 ====================================== */
                        #endif
                    }

                    if (nClsIdx == PMNMClass::MOTOR_VEHICLE)
                    {
                        /* 违规变道检测区域 */
                        for (auto &Param : stInData.stParam.vstIllegalLaneChangeParam)
                        {
                            if (Param.bEnable)
                            {
                                if (fBoxConfidence < Param.fIllegalLaneChangeBoxThreshold)
                                {
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

                        /* 逆行检测区域 */
                        for (auto &Param : stInData.stParam.vstDrivingAgainstTrafficParam)
                        {
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
                                    break;
                                }
                            }
                        }

                        /* 停车检测区域 */
                        int ara = 0;
                        for (auto &Param : stInData.stParam.vstParkingParam)
                        {
                            if (Param.bEnable)
                            {
                                if (fBoxConfidence < Param.fParkingBoxThreshold)
                                {
                                    continue;
                                }
                                bool nRet = intrusionZoneDetection(m_mapVehicle[nTargetId].curPoint, Param.vecPoints);
                                if (nRet)
                                {
                                    if (!m_mapVehicle[nTargetId].stParking.bParking)
                                    {
                                        m_mapVehicle[nTargetId].stParking.bParking          = true;
                                        m_mapVehicle[nTargetId].stParking.nParkingTimeStamp = time(NULL);
                                    }
                                    else
                                    {
                                        if (time(NULL) - m_mapVehicle[nTargetId].stParking.nParkingTimeStamp >= Param.nParkingTimeThreshold)
                                        {
                                            stOutData->bParkingFlag = true;
                                            m_mapVehicle[nTargetId].stParking.bParking          = false;
                                        }
                                    }
                                }
                                else
                                {
                                    m_mapVehicle[nTargetId].stParking.bParking          = false;
                                }

                                if (stOutData->bParkingFlag)
                                {
                                    break;
                                }
                            }
                            ara++;
                        }
                    }
                }
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
                    itr = m_HeadCountStatus.erase(itr);  // erase返回下一个迭代器
                }
                else
                {
                    ++itr;  // 不删除时手动递增
                }
            }
        }

        for(auto itr  = m_FenceClimbStatus.begin(); itr != m_FenceClimbStatus.end();)
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
                    itr = m_FenceClimbStatus.erase(itr);  // erase返回下一个迭代器
                }
                else
                {
                    ++itr;  // 不删除时手动递增
                }
            }
        }

        stOutData->nChnId = stInData.nChnId;
    }
    return true;
}

/* 处理数据 */
bool CGroup2Detect::resizeAndPadImage(cv::Mat inputImage, cv::Mat &outputImage)
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

bool CGroup2Detect::resizeAndPadImage2(cv::Mat inputImage, cv::Mat &outputImage)
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
TripLineType_E CGroup2Detect::tripLineDetection(
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

bool CGroup2Detect::isBoundingBoxIntersecting(
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

int CGroup2Detect::crossProduct(
    const cv::Point &alertLineStart,
    const cv::Point &alertLineEnd,
    const cv::Point &testPoint)
{
    return (alertLineEnd.x - alertLineStart.x) * (testPoint.y - alertLineStart.y) -
           (alertLineEnd.y - alertLineStart.y) * (testPoint.x - alertLineStart.x);
}

/* 入侵检测：判断禁止入侵的区域有无点 */
bool CGroup2Detect::intrusionZoneDetection(
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
bool CGroup2Detect::entryZoneDetection(
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
bool CGroup2Detect::leaveZoneDetection(
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
bool CGroup2Detect::isIntersecting(
    std::vector<cv::Point> rectPolygon,
    std::vector<cv::Point> polygons)
{
    std::vector<cv::Point> intersectionPoints;
    cv::intersectConvexConvex(rectPolygon, polygons, intersectionPoints);
    return !intersectionPoints.empty();
}

/* 判断目标框与线段是否有交点 */
bool CGroup2Detect::isLineIntersectingRect(
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

bool CGroup2Detect::isTimeIntervalExceeded(int64_t nRecordTime, int nThresholdSec)
{
    int64_t nCurrentTime = getSteadyTimeStampMs();

    int64_t nTimeDiffMs = nCurrentTime - nRecordTime;
#if Group2Detect_DEBUG
    printf("====nTimeDiffMs[%ld]========\n", nTimeDiffMs);
#endif
    return (nTimeDiffMs > nThresholdSec * 1000);
}

/* 判断是否跨越线段 */
bool CGroup2Detect::doLinesIntersect(const cv::Point &p1,
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
/**
 * @brief 等比例缩放并填充图片至固定尺寸
 * @param inputImage  输入图像（任意尺寸）
 * @param outputImage 输出图像（固定尺寸 m_nLimitWidth × m_nLimitHeight）
 * @return true 始终返回 true
 * @details 先按比例缩放(支持放大/缩小)，使图像适应限制尺寸，然后居中填充黑色边框。
 *          缩放比例 = min(限制宽度/原宽度, 限制高度/原高度)，保证缩放后不超出限制尺寸。
 *          例：417×585 → scale=min(640/417, 384/585)=0.656, 缩放后 274×384, 居中偏移(183,0)
 */
bool CGroup2Detect::resizeAndPadImage3(cv::Mat inputImage, cv::Mat &outputImage)
{
    if (inputImage.empty()) {
        std::cerr << "Input image is empty!" << std::endl;
        return false;
    }
    
    int imageWidth  = inputImage.cols;
    int imageHeight = inputImage.rows;

    // 计算缩放比例：取宽高两个方向的较小值，保证缩放后两边都不超出限制尺寸
    // scale > 1 表示放大，scale < 1 表示缩小
    m_fResizeScale = std::min(
        static_cast<float>(m_nLimitWidth) / static_cast<float>(imageWidth),
        static_cast<float>(m_nLimitHeight) / static_cast<float>(imageHeight));

    // 缩放后的尺寸
    int newWidth  = static_cast<int>(imageWidth * m_fResizeScale);
    int newHeight = static_cast<int>(imageHeight * m_fResizeScale);

    // 执行缩放
    cv::Mat resizedImage;
    cv::resize(inputImage, resizedImage, cv::Size(newWidth, newHeight));

    // 居中偏移量（保证非负：newWidth/newHeight 由 min 保证 ≤ 限制尺寸）
    m_nXOffset = (m_nLimitWidth - newWidth) / 2;
    m_nYOffset = (m_nLimitHeight - newHeight) / 2;

    // 创建固定尺寸黑色背景，将缩放后的图像居中放置
    cv::Mat output = cv::Mat::zeros(m_nLimitHeight, m_nLimitWidth, inputImage.type());
    resizedImage.copyTo(output(cv::Rect(m_nXOffset, m_nYOffset, newWidth, newHeight)));

    outputImage = output;
    return true;
}


/* 判断人形区域与拌线是否有交集 */
TripLineType_E CGroup2Detect::tripLineDetection(
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
int CGroup2Detect::pointLeftOrRightOfLine(const cv::Point2f& linePt1, const cv::Point2f& linePt2, const cv::Point2f& point,bool bFlag) {
    
    cv::Point2f p1 = linePt1;
    cv::Point2f p2 = linePt2;

    if (p1.x > p2.x) std::swap(p1, p2);
    
    cv::Point2f lineVec = p2 - p1;
    cv::Point2f pointVec = point - p1;
    if(bFlag)
    {
        // 1. 先检查点是否在线段的"投影范围"内
        float projection = pointVec.dot(lineVec);  // 点积
        float lineLengthSq = lineVec.dot(lineVec); // 线段长度平方
        
        // 如果投影值 < 0，点在起点之前（延长线上）
        // 如果投影值 > lineLengthSq，点在终点之后（延长线上）
        if (projection < 0 || projection > lineLengthSq) {
            // 点在线段延长线上，不在线段范围内
            return 0;  // 或返回其他特殊值
        }
    }
    
    // 计算叉积 (lineVec × pointVec)
    float crossProduct = lineVec.x * pointVec.y - lineVec.y * pointVec.x;
    
    if (crossProduct > 1e-6) {
        return -1;  // 右侧
    } else if (crossProduct < -(1e-6)) {
        return 1;   // 左侧
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
int CGroup2Detect::pointAboveOrBelowLine(const cv::Point2f& linePt1, const cv::Point2f& linePt2, const cv::Point2f& point,bool bFlag) 
{
    cv::Point2f p1 = linePt1;
    cv::Point2f p2 = linePt2;
    
    if (p1.y > p2.y) std::swap(p1, p2);

    cv::Point2f lineVec = p2 - p1;
    cv::Point2f pointVec = point - p1;
    
    // 1. 先检查点是否在线段的"投影范围"内
    if(bFlag)
    {
        float projection = pointVec.dot(lineVec);  // 点积
        float lineLengthSq = lineVec.dot(lineVec); // 线段长度平方
        
        // 如果投影值 < 0，点在起点之前（延长线上）
        // 如果投影值 > lineLengthSq，点在终点之后（延长线上）
        if (projection < 0 || projection > lineLengthSq) {
            // 点在线段延长线上，不在线段范围内
            return 0;  // 或返回其他特殊值
        }
    }
    
    // 计算叉积 (lineVec × pointVec)
    float crossProduct = lineVec.x * pointVec.y - lineVec.y * pointVec.x;
    
    if (crossProduct > 1e-6) {
        return -1;   // 下方（图像坐标系）
    } else if (crossProduct < -(1e-6)) {
        return 1;  // 上方（图像坐标系）
    } else {
        return 0;   // 在线段上或共线
    }
}
int CGroup2Detect::pointSideOfLine(const cv::Point2f& linePt1, const cv::Point2f& linePt2, const cv::Point2f& point,bool bFlag)
{
    cv::Point2f p1 = linePt1, p2 = linePt2;
    float dx = std::abs(p2.x - p1.x);
    float dy = std::abs(p2.y - p1.y);

    // 按主方向排序
    if (dx >= dy) {  // 横线为主
        if (p1.x > p2.x) std::swap(p1, p2);
    } else {         // 竖线为主
        if (p1.y > p2.y) std::swap(p1, p2);
    }

    cv::Point2f lineVec = p2 - p1;
    cv::Point2f pointVec = point - p1;

    if(bFlag)
    {
        // 检查点是否在线段投影范围内（可选）
        float projection = pointVec.dot(lineVec);
        float lenSq = lineVec.dot(lineVec);
        if (projection < 0 || projection > lenSq) return 0; // 在延长线上
    }

    float cross = lineVec.x * pointVec.y - lineVec.y * pointVec.x;
    const float EPS = 1e-6f;

    if (cross > EPS) {
        // 横线 → 上方；竖线 → 左侧
        return 1;
    } else if (cross < -EPS) {
        // 横线（从左到右）→ 下方；竖线（从上到下）→ 右侧
        return -1;
    } else {
        return 0; // 共线
    }
}
/**
 * @brief 计算两条直线的夹角（锐角，0-90度）
 * @param linePt1 第一条直线的起点
 * @param linePt2 第一条直线的终点
 * @return 与垂直线的夹角（度）
 */
double CGroup2Detect::calculateAngleWithVertical(const cv::Point2f& linePt1, const cv::Point2f& linePt2) {
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

void CGroup2Detect::DrawPoint(cv::Mat& mat,cv::Point point,cv::Scalar sc, bool isWriteFile)
{
    cv::circle(mat, point, 5 , sc, cv::FILLED);
    if(isWriteFile)
    {
        cv::imwrite("/opt/"+std::to_string(getSteadyTimeStampMs())+".jpg",mat);
    }
}

void CGroup2Detect::DrawLine(cv::Mat& mat,cv::Point point1, cv::Point point2,cv::Scalar sc, bool isWriteFile)
{
    cv::line(mat, point1, point2, sc, 2);
    if(isWriteFile)
    {
        cv::imwrite("/opt/"+std::to_string(getSteadyTimeStampMs())+".jpg",mat);
    }
}

void CGroup2Detect::DrawArea(cv::Mat& mat,cv::Point point1, cv::Point point2,cv::Scalar sc, bool isWriteFile)
{
    cv::rectangle(mat, point1, point2, sc, 1);
    if(isWriteFile)
    {
        cv::imwrite("/opt/"+std::to_string(getSteadyTimeStampMs())+".jpg",mat);
    }
}

void CGroup2Detect::DrawArea(cv::Mat& mat,std::vector<cv::Point> points,cv::Scalar sc, bool isWriteFile)
{
    cv::polylines(mat, 
                points, 
                true,
                sc, /* 边框颜色（绿色） */
                1,
                cv::LINE_AA);
    if(isWriteFile)
    {
        cv::imwrite("/opt/"+std::to_string(getSteadyTimeStampMs())+".jpg",mat);
    }
}

void CGroup2Detect::DrawText(cv::Mat& mat,std::string text,cv::Point point,cv::Scalar sc, bool isWriteFile)
{
    cv::putText(
                mat,                                                 // 目标图像
                text,                                               // 文字内容（这里显示置信度）
                point,                                              // 文字位置（矩形框左上角偏上）
                cv::FONT_HERSHEY_SIMPLEX,                          // 字体类型
                0.6,                                               // 字体大小
                sc,                                                 // 颜色（B,G,R）绿色
                2,                                                 // 粗细
                cv::LINE_AA                                        // 抗锯齿
            );
    if(isWriteFile)
    {
        cv::imwrite("/opt/"+std::to_string(getSteadyTimeStampMs())+".jpg",mat);
    }
}