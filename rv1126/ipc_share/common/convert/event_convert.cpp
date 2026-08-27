/***
 * @FilePath     : event_convert.cpp
 * @Author       : huangjunda
 * @Date         : 2025-04-28 19:24:52
 * @LastEditors  : huangjunda
 * @LastEditTime : 2025-04-29 11:40:48
 * @Description  :
 */

#include "event_convert.h"
#include "common_convert.h"

#include "convert.h" /* 这个要放在network_convert.h的后面 */

// /* 点坐标（无需修改）*/
// void Convert::deal(Json::Object *pRootJson, Event::IvaPoint &stInfo, bool bOutStruct)
// {
//     Convert::CConvert convert(bOutStruct);
//     convert.field(pRootJson, "x", stInfo.x);
//     convert.field(pRootJson, "y", stInfo.y);
// }

// /* 区域结构体（数组版本）*/
// void Convert::deal(Json::Object *pRootJson, Event::IvaArea &stInfo, bool bOutStruct)
// {
//     Convert::CConvert convert(bOutStruct);
//     convert.field(pRootJson, "PointNum", stInfo.pointNum);

//     if (bOutStruct)
//     {
//         // 反序列化：JSON数组 -> 结构体数组
//         Json::Object *pointsArray = Json::get(pRootJson, "Points");
//         if (pointsArray)
//         {
//             int actualPoints = std::min(
//                 Json::Array::size(pointsArray),
//                 static_cast<int>(IVA_AREA_POINT_NUM_MAX));
//             stInfo.pointNum = actualPoints;
//             for (int i = 0; i < actualPoints; ++i)
//             {
//                 Json::Object *pointJson = Json::Array::get(pointsArray, i);
//                 if (pointJson)
//                 {
//                     deal(pointJson, stInfo.points[i], bOutStruct);
//                 }
//             }
//         }
//     }
//     else
//     {
//         // 序列化：结构体数组 -> JSON数组
//         Json::Object *pointsArray = Json::Array::init();
//         for (uint32_t i = 0; i < stInfo.pointNum; ++i)
//         {
//             Json::Object *pointJson = Json::init();
//             deal(pointJson, stInfo.points[i], bOutStruct);
//             Json::Array::add(pointsArray, pointJson);
//         }
//         Json::add(pRootJson, "Points", pointsArray);
//     }
// }

// /* 多区域结构体（数组版本）*/
// void Convert::deal(Json::Object *pRootJson, Event::IvaAreas &stInfo, bool bOutStruct)
// {
//     Convert::CConvert convert(bOutStruct);
//     convert.field(pRootJson, "AreaNum", stInfo.areaNum);

//     if (bOutStruct)
//     {
//         // 反序列化：JSON数组 -> 结构体数组
//         Json::Object *areasArray = Json::get(pRootJson, "Areas");
//         if (areasArray)
//         {
//             int actualAreas = std::min(
//                 Json::Array::size(areasArray),
//                 static_cast<int>(IVA_AREA_NUM_MAX));
//             stInfo.areaNum = actualAreas;
//             for (int i = 0; i < actualAreas; ++i)
//             {
//                 Json::Object *areaJson = Json::Array::get(areasArray, i);
//                 if (areaJson)
//                 {
//                     deal(areaJson, stInfo.areas[i], bOutStruct);
//                 }
//             }
//         }
//     }
//     else
//     {
//         // 序列化：结构体数组 -> JSON数组
//         Json::Object *areasArray = Json::Array::init();
//         for (uint32_t i = 0; i < stInfo.areaNum; ++i)
//         {
//             Json::Object *areaJson = Json::init();
//             deal(areaJson, stInfo.areas[i], bOutStruct);
//             Json::Array::add(areasArray, areaJson);
//         }
//         Json::add(pRootJson, "Areas", areasArray);
//     }
// }


void Convert::deal(Json::Object* pRootJson, Event::ChnSmart_S& stInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }

    Convert::CConvert convert(bOutStruct);
    convert.field(pRootJson, "ChnId", stInfo.nChnId);
    convert.field(pRootJson, "EventTypes", stInfo.types);
}
void Convert::deal(Json::Object* pRootJson, std::set<Event::ChnSmart_S>& stInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }

    Convert::CConvert convert(bOutStruct);
    convert.structure(pRootJson, "ChnSmart", stInfo);
}
void Convert::deal(Json::Object* pRootJson, Event::NvrSmartInfo_S& stInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }

    Convert::CConvert convert(bOutStruct);
    convert.field(pRootJson, "MainType", stInfo.nMainType);
    convert.field(pRootJson, "MaxSupport", stInfo.nMaxSupport);
    convert.structure(pRootJson, stInfo.chnSmart);
}
void Convert::deal(Json::Object* pRootJson, std::set<Event::NvrSmartInfo_S>& stInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }

    Convert::CConvert convert(bOutStruct);
    convert.structure(pRootJson, "NvrSmartInfos", stInfo);
}
void Convert::deal(Json::Object* pRootJson, Event::AlgorithmConfig_S& stInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }

    Convert::CConvert convert(bOutStruct);
    convert.field(pRootJson, "EnMotionDetect", stInfo.nEnMotionDetect);
    convert.field(pRootJson, "EnOcclusionDetect", stInfo.nEnOcclusionDetect);
    convert.field(pRootJson, "EnAnomalyAlarm", stInfo.nEnAnomalyAlarm);
    convert.field(pRootJson, "EnAudioAlarm", stInfo.nEnAudioAlarm);
    convert.field(pRootJson, "EnAlarmInput", stInfo.nEnAlarmInput);
    convert.field(pRootJson, "EnAlarmOutput", stInfo.nEnAlarmOutput);
    convert.field(pRootJson, "EnFlashAlarm", stInfo.nEnFlashAlarm);
    convert.field(pRootJson, "EnPIRAlarm", stInfo.nEnPIRAlarm);
    convert.field(pRootJson, "EnLineCrossing", stInfo.nEnLineCrossing);
    convert.field(pRootJson, "EnIntrusion", stInfo.nEnIntrusion);
    convert.field(pRootJson, "EnEnterRegion", stInfo.nEnEnterRegion);
    convert.field(pRootJson, "EnLeaveRegion", stInfo.nEnLeaveRegion);
    convert.field(pRootJson, "EnAudioAnomaly", stInfo.nEnAudioAnomaly);
    convert.field(pRootJson, "EnSceneChange", stInfo.nEnSceneChange);
    convert.field(pRootJson, "EnFaceDetect", stInfo.nEnFaceDetect);
    convert.field(pRootJson, "EnLoiteringDetect", stInfo.nEnLoiteringDetect);
    convert.field(pRootJson, "EnCrowdGathering", stInfo.nEnCrowdGathering);
    convert.field(pRootJson, "EnParkingDetect", stInfo.nEnParkingDetect);
    convert.field(pRootJson, "EnUnattendedObject", stInfo.nEnUnattendedObject);
    convert.field(pRootJson, "EnObjectRemoval", stInfo.nEnObjectRemoval);
    convert.field(pRootJson, "EnPetRecognition", stInfo.nEnPetRecognition);
    convert.field(pRootJson, "EnFaceCapture", stInfo.nEnFaceCapture);
    convert.field(pRootJson, "nEnFaceCompare", stInfo.nEnFaceCompare);
    //convert.field(pRootJson, "EnFaceDet", stInfo.nEnFaceDet);
    convert.field(pRootJson, "EnFaceFea", stInfo.nEnFaceLib);
#ifdef SCENE_INTELLIGENT_ANALYSIS
    convert.field(pRootJson, "EnLLmInference", stInfo.nEnLLmInference);
    convert.field(pRootJson, "EnTextPreset", stInfo.nEnTextPreset);
    convert.field(pRootJson, "EnAISceneAnalysis", stInfo.nEnAISceneAnalysis);
#endif

#ifdef SCENE_INTELLIGENCE
    // ========== 行为监管 ==========
    convert.field(pRootJson, "EnSleepOnDuty", stInfo.nEnSleepOnDuty);
    convert.field(pRootJson, "EnLeavePost", stInfo.nEnLeavePost);
    convert.field(pRootJson, "EnElectricVehicleInElevator", stInfo.nEnElectricVehicleInElevator);
    convert.field(pRootJson, "EnPersonFallDown", stInfo.nEnPersonFallDown);
    convert.field(pRootJson, "EnFenceClimbing", stInfo.nEnFenceClimbing);
    convert.field(pRootJson, "EnTrip", stInfo.nEnTrip);
    convert.field(pRootJson, "EnSmoking", stInfo.nEnSmoking);
    convert.field(pRootJson, "EnPhoneUsage", stInfo.nEnPhoneUsage);
    convert.field(pRootJson, "EnSmokeFire", stInfo.nEnSmokeFire);
    convert.field(pRootJson, "EnOpenFlame", stInfo.nEnOpenFlame);
    convert.field(pRootJson, "EnManholeCoverAbnormal", stInfo.nEnManholeCoverAbnormal);
    convert.field(pRootJson, "EnBareSoil", stInfo.nEnBareSoil);
    convert.field(pRootJson, "EnHoleProtectionBar", stInfo.nEnHoleProtectionBar);
    convert.field(pRootJson, "EnPedestrianIntrusion", stInfo.nEnPedestrianIntrusion);

    // ========== 穿戴规范 ==========
    convert.field(pRootJson, "EnSafetyHelmet", stInfo.nEnSafetyHelmet);
    convert.field(pRootJson, "EnReflectiveClothing", stInfo.nEnReflectiveClothing);
    convert.field(pRootJson, "EnHighAltitudeSeatbelt", stInfo.nEnHighAltitudeSeatbelt);

    // ========== 交通行为监管 ==========
    convert.field(pRootJson, "EnConstructionOccupyRoad", stInfo.nEnConstructionOccupyRoad);
    convert.field(pRootJson, "EnEmergencyLaneOccupancy", stInfo.nEnEmergencyLaneOccupancy);
    convert.field(pRootJson, "EnReverseDirection", stInfo.nEnReverseDirection);
    convert.field(pRootJson, "EnNonMotorVehicleIntrusion", stInfo.nEnNonMotorVehicleIntrusion);
    convert.field(pRootJson, "EnRoadPonding", stInfo.nEnRoadPonding);
    convert.field(pRootJson, "EnCongestion", stInfo.nEnCongestion);
    convert.field(pRootJson, "EnIllegalParking", stInfo.nEnIllegalParking);
    convert.field(pRootJson, "EnIllegalLaneChange", stInfo.nEnIllegalLaneChange);
    
    // ========== 属性识别 ==========
    convert.field(pRootJson, "PlateNumber", stInfo.nPlateNumber);
#endif

#if defined(SCENE_INTELLIGENCE) || CAP_AI_GARBAGE_DETECT
    convert.field(pRootJson, "EnGarbageExposure", stInfo.nEnGarbageExposure);
    convert.field(pRootJson, "EnGarbageOverflow", stInfo.nEnGarbageOverflow);
#endif

#if CAP_AI_PEOPLE_STATISTICS
    convert.field(pRootJson, "EnPeopleFlowStatistics", stInfo.nEnPeopleFlowStatistics);
    convert.field(pRootJson, "EnPeopleDensityDetection", stInfo.nEnPeopleDensityDetection);
#endif
}

void Convert::deal(Json::Object* pRootJson, std::vector<Event::AlgorithmConfig_S>& stInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }

    Convert::CConvert convert(bOutStruct);
    convert.structure(pRootJson, "AlgorithmConfigs", stInfo);
}
void Convert::deal(Json::Object *pRootJson, Event::OrdinaryEventEnableStatus_S &stInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }

    Convert::CConvert convert(bOutStruct);
    convert.field(pRootJson, "MotionDetect", stInfo.bMotionDetect);
    convert.field(pRootJson, "OcclusionDetect", stInfo.bOcclusionDetect);
    convert.field(pRootJson, "AnomalyAlarm", stInfo.bAnomalyAlarm);
    convert.field(pRootJson, "AudioAlarm", stInfo.bAudioAlarm);
    convert.field(pRootJson, "AlarmInput", stInfo.bAlarmInput);
    convert.field(pRootJson, "AlarmOutput", stInfo.bAlarmOutput);
    convert.field(pRootJson, "FlashAlarm", stInfo.bFlashAlarm);
    convert.field(pRootJson, "PIRAlarm", stInfo.bPIRAlarm);
}
void Convert::deal(Json::Object* pRootJson, Event::SmartEventEnableStatus_S& stInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }

    Convert::CConvert convert(bOutStruct);
    convert.field(pRootJson, "LineCrossing", stInfo.bLineCrossing);
    convert.field(pRootJson, "Intrusion", stInfo.bIntrusion);
    convert.field(pRootJson, "EnterRegion", stInfo.bEnterRegion);
    convert.field(pRootJson, "LeaveRegion", stInfo.bLeaveRegion);
    convert.field(pRootJson, "LoiteringDetect", stInfo.bLoiteringDetect);
    convert.field(pRootJson, "CrowdGathering", stInfo.bCrowdGathering);
    convert.field(pRootJson, "ParkingDetect", stInfo.bParkingDetect);
    convert.field(pRootJson, "AudioAnomaly", stInfo.bAudioAnomaly);
    convert.field(pRootJson, "SceneChange", stInfo.bSceneChange);
    convert.field(pRootJson, "UnattendedObject", stInfo.bUnattendedObject);
    convert.field(pRootJson, "ObjectRemoval", stInfo.bObjectRemoval);
    convert.field(pRootJson, "FaceDetect", stInfo.bFaceDetect);
    convert.field(pRootJson, "PetRecognition", stInfo.bPetRecognition);
    convert.field(pRootJson, "FaceCapture", stInfo.bFaceCapture);
    convert.field(pRootJson, "FaceCompare", stInfo.bFaceCompare);
#ifdef SCENE_INTELLIGENCE
    // ========== 行为监管 ==========
    convert.field(pRootJson, "SleepOnDuty", stInfo.bSleepOnDuty);
    convert.field(pRootJson, "LeavePost", stInfo.bLeavePost);
    convert.field(pRootJson, "ElectricVehicleInElevator", stInfo.bElectricVehicleInElevator);
    convert.field(pRootJson, "PersonFallDown", stInfo.bPersonFallDown);
    convert.field(pRootJson, "FenceClimbing", stInfo.bFenceClimbing);
    convert.field(pRootJson, "Trip", stInfo.bTrip);
    convert.field(pRootJson, "Smoking", stInfo.bSmoking);
    convert.field(pRootJson, "PhoneUsage", stInfo.bPhoneUsage);
    convert.field(pRootJson, "SmokeFire", stInfo.bSmokeFire);
    convert.field(pRootJson, "OpenFlame", stInfo.bOpenFlame);
    convert.field(pRootJson, "ManholeCoverAbnormal", stInfo.bManholeCoverAbnormal);
    convert.field(pRootJson, "BareSoil", stInfo.bBareSoil);
    convert.field(pRootJson, "HoleProtectionBar", stInfo.bHoleProtectionBar);
    convert.field(pRootJson, "PedestrianIntrusion", stInfo.bPedestrianIntrusion);

    // ========== 穿戴规范 ==========
    convert.field(pRootJson, "SafetyHelmet", stInfo.bSafetyHelmet);
    convert.field(pRootJson, "ReflectiveClothing", stInfo.bReflectiveClothing);
    convert.field(pRootJson, "HighAltitudeSeatbelt", stInfo.bHighAltitudeSeatbelt);

    // ========== 交通行为监管 ==========
    convert.field(pRootJson, "ConstructionOccupyRoad", stInfo.bConstructionOccupyRoad);
    convert.field(pRootJson, "EmergencyLaneOccupancy", stInfo.bEmergencyLaneOccupancy);
    convert.field(pRootJson, "ReverseDirection", stInfo.bReverseDirection);
    convert.field(pRootJson, "NonMotorVehicleIntrusion", stInfo.bNonMotorVehicleIntrusion);
    convert.field(pRootJson, "RoadPonding", stInfo.bRoadPonding);
    convert.field(pRootJson, "Congestion", stInfo.bCongestion);
    convert.field(pRootJson, "IllegalParking", stInfo.bIllegalParking);
    convert.field(pRootJson, "IllegalLaneChange", stInfo.bIllegalLaneChange);

    // ========== 属性识别 ==========
    convert.field(pRootJson, "PlateNumber", stInfo.bPlateNumber);
#endif
#if defined(SCENE_INTELLIGENCE) || CAP_AI_GARBAGE_DETECT
    convert.field(pRootJson, "GarbageExposure", stInfo.bGarbageExposure);
    convert.field(pRootJson, "GarbageOverflow", stInfo.bGarbageOverflow);
#endif
#if CAP_AI_PEOPLE_STATISTICS
    convert.field(pRootJson, "PeopleFlowStatistics", stInfo.bPeopleFlowStatistics);
    convert.field(pRootJson, "PeopleDensityDetection", stInfo.bPeopleDensityDetection);
#endif
/*场景智能分析*/
#ifdef SCENE_INTELLIGENT_ANALYSIS
    // ========== 场景智能分析事件 ==========
    convert.field(pRootJson, "SceneAnalysis", stInfo.bSceneAnalysis);
#endif
}

void Convert::deal(Json::Object *pRootJson, std::vector<Event::Type_E> &stInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }

    Convert::CConvert convert(bOutStruct);
    std::vector<int> vstInfo;
    for (auto &type : stInfo)
    {
        vstInfo.push_back(static_cast<int>(type));
    }
    convert.field(pRootJson, "CanEventTypeArray", vstInfo);
}

void Convert::deal(Json::Object *pRootJson, Event::SmartResourceAlloc_S &stInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }

    Convert::CConvert convert(bOutStruct);
    convert.structure(pRootJson, "SmartEventEnableStatus", stInfo.stSmartEventEnableStatus);
    deal(pRootJson, stInfo.aCanEventTypeArray, bOutStruct);

    /* 智能事件启用情况列表 */
    std::vector<int> vstInfo;
    for (auto &type : stInfo.aSmartEventEnableStatusArray)
    {
        vstInfo.push_back(static_cast<int>(type));
    }
    convert.field(pRootJson, "SmartEventEnableStatusArray", vstInfo);
}

/* Metadata配置-Smart事件 */
void Convert::deal(Json::Object *pRootJson, Event::MetadataSmart_S &stInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }

    Convert::CConvert convert(bOutStruct);
    convert.field(pRootJson, "TargetId", stInfo.bTargetId);
    convert.field(pRootJson, "TargetCoord", stInfo.bTargetCoord);
    convert.field(pRootJson, "Time", stInfo.bTime);
}

/* Metadata配置-人脸抓拍 */
void Convert::deal(Json::Object *pRootJson, Event::MetadataFaceCapture_S &stInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }

    Convert::CConvert convert(bOutStruct);
    convert.field(pRootJson, "RegularRegionBox", stInfo.bRegularRegionBox);
    convert.field(pRootJson, "TargetId", stInfo.bTargetId);
    convert.field(pRootJson, "TargetCoord", stInfo.bTargetCoord);
    convert.field(pRootJson, "FaceScore", stInfo.bFaceScore);
    convert.field(pRootJson, "Time", stInfo.bTime);
}

/* Metadata配置 */
void Convert::deal(Json::Object *pRootJson, Event::MetadataConfig_S &stInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }

    Convert::CConvert convert(bOutStruct);
    convert.field(pRootJson, "Enable", stInfo.bEnable);
    convert.structure(pRootJson, "MetadataSmart", stInfo.stMetadataSmart);
    convert.structure(pRootJson, "MetadataFaceCapture", stInfo.stMetadataFaceCapture);
}

void Convert::deal(Json::Object* pRootJson, Event::BindVideo_S& stInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }

    Convert::CConvert convert(bOutStruct);
    convert.field(pRootJson, "ChnId", stInfo.nChnId);
    convert.field(pRootJson, "VideoPath", stInfo.strVideoPath);

}
void Convert::deal(Json::Object* pRootJson, std::vector<Event::BindVideo_S>& stInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }

    Convert::CConvert convert(bOutStruct);
    convert.structure(pRootJson, "BindVideos", stInfo);
}


/* 转换函数 */
void Convert::deal(Json::Object* pRootJson, Event::Info_S& stInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }

    Convert::CConvert convert(bOutStruct);

    convert.field(pRootJson, "Id", stInfo.nId);
    convert.field(pRootJson, "ChnId", stInfo.nChnId);
    convert.field(pRootJson, "Type", (int&)stInfo.enType);
    convert.field(pRootJson, "Date", stInfo.strDate);
    convert.field(pRootJson, "Time", stInfo.strTime);
    convert.field(pRootJson, "StartTime", stInfo.strStartTime);
    convert.field(pRootJson, "EndTime", stInfo.strEndTime);
    convert.field(pRootJson, "Timestamp", stInfo.lTimestamp);
    convert.field(pRootJson, "Lable", stInfo.strLabel);
    convert.field(pRootJson, "VideoPath", stInfo.strVideoPath);
    convert.field(pRootJson, "VideoSize", stInfo.nVideoSize);
    convert.structure(pRootJson, stInfo.bindVideos);

}


void Convert::deal(Json::Object* pRootJson, Event::CaptureInfo_S& stCaptureInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }
    
    deal(pRootJson, stCaptureInfo.stInfo, bOutStruct);
    Convert::CConvert convert(bOutStruct);

    std::string vfFeatrues;
    for(auto item : stCaptureInfo.vfFeatrue)
    {
        vfFeatrues += std::to_string(item);
    }
    convert.field(pRootJson, "Feature", vfFeatrues);
}


void Convert::deal(Json::Object* pRootJson, Event::CompareInfo_S& stCompareInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }

    deal(pRootJson, stCompareInfo.stInfo, bOutStruct);
    Convert::CConvert convert(bOutStruct);
    
    convert.field(pRootJson, "LibId", stCompareInfo.nLibId);
    convert.field(pRootJson, "MatchId", stCompareInfo.nMatchId);
}


void Convert::deal(Json::Object* pRootJson, Event::VehicleInfo_S& stVehicleInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }

    Convert::CConvert convert(bOutStruct);
    convert.structure(pRootJson, stVehicleInfo.stInfo);
    convert.field(pRootJson, "PlateRegion", (int&)stVehicleInfo.enPlateRegion);
    convert.field(pRootJson, "PlateSerial", stVehicleInfo.strPlateSerial);
}


void Convert::deal(Json::Object* pRootJson, std::vector<Event::Info_S> &Infos, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }
    
    Convert::CConvert convert(bOutStruct);
    convert.structure(pRootJson, "Infos", Infos);
}


void Convert::deal(Json::Object* pRootJson, std::vector<Event::VehicleInfo_S> &VehicleInfos, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }
    
    Convert::CConvert convert(bOutStruct);
    convert.structure(pRootJson, "VehicleInfos", VehicleInfos);
}


void Convert::deal(Json::Object* pRootJson, Event::RetrievalCond_S& stRetrievalCond, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }

    Convert::CConvert convert(bOutStruct);
    convert.field(pRootJson, "Id", stRetrievalCond.nId);
    convert.field(pRootJson, "ChnIds", stRetrievalCond.nChnIds);
    convert.field(pRootJson, "Type", (int&)stRetrievalCond.enType);
    convert.field(pRootJson, "GroupType", stRetrievalCond.groupType);
    convert.field(pRootJson, "StartDate", stRetrievalCond.strStartDate);
    convert.field(pRootJson, "EndDate", stRetrievalCond.strEndDate);
    convert.field(pRootJson, "Time", stRetrievalCond.strTime);
    convert.field(pRootJson, "StartTime", stRetrievalCond.strStartTime);
    convert.field(pRootJson, "EndTime", stRetrievalCond.strEndTime);

    convert.field(pRootJson, "IsLock", stRetrievalCond.nIsLock);
    convert.field(pRootJson, "Label", stRetrievalCond.strLabel);

    convert.field(pRootJson, "LibId", stRetrievalCond.nLibId);
    convert.field(pRootJson, "MatchId", stRetrievalCond.nMatchId);

    convert.field(pRootJson, "PlateRegion", (int&)stRetrievalCond.enPlateRegion);
    convert.field(pRootJson, "PlateSerial", stRetrievalCond.strPlateSerial);

    convert.field(pRootJson, "PicType", stRetrievalCond.nPicType);
    convert.field(pRootJson, "VideoType", stRetrievalCond.nVideoType);
    convert.field(pRootJson, "CompResult", stRetrievalCond.nCompResult);
    convert.field(pRootJson, "ImagePath", stRetrievalCond.strImagePath);
    convert.field(pRootJson, "CapFacePath", stRetrievalCond.strCapFacePath);
    convert.field(pRootJson, "Text", stRetrievalCond.strText);
    
    convert.field(pRootJson, "IsEnQuickEntry", stRetrievalCond.bEnQuickEntry);
}
void Convert::deal(Json::Object* pRootJson, Event::QuickEntry_S& stQuickEntry, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }

    Convert::CConvert convert(bOutStruct);
    convert.field(pRootJson, "EntryType", (int&)stQuickEntry.enType);
    convert.field(pRootJson, "EntryName", stQuickEntry.name);
    convert.field(pRootJson, "ActionCode", stQuickEntry.nActionCode);
    convert.field(pRootJson, "Message", stQuickEntry.message);
    convert.field(pRootJson, "Timestamp", stQuickEntry.nTimestamp);
}
void Convert::deal(Json::Object* pRootJson, std::set<Event::QuickEntry_S>& stQuickEntry, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }

    Convert::CConvert convert(bOutStruct);
    convert.structure(pRootJson, "QuickEntries", stQuickEntry);
}


void Convert::deal(Json::Object* pRootJson, Event::Point_S& stPoint, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }

    Convert::CConvert convert(bOutStruct);
    convert.field(pRootJson, "X", stPoint.nX);
    convert.field(pRootJson, "Y", stPoint.nY);
}

void Convert::deal(Json::Object* pRootJson, std::vector<Event::Point_S>& points, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }

    Convert::CConvert convert(bOutStruct);
    convert.structure(pRootJson, "Points", points);
}
void Convert::deal(Json::Object* pRootJson, Event::RuleInfo_S& stRuleInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }

    Convert::CConvert convert(bOutStruct);
    convert.field(pRootJson, "EventType", (int &)stRuleInfo.enType);
    convert.field(pRootJson, "ChnId", stRuleInfo.nChnId);
    convert.field(pRootJson, "Enable", stRuleInfo.bEnable);
    if (bOutStruct)
    {
            
        int nLineNum = 0;
        while (1)
        {
            nLineNum++;
            std::string key = "Line" + std::to_string(nLineNum);
            Json::Object* item = Json::get(pRootJson, key);
            if (item == nullptr)
            {
                break;
            }
            Event::Line line;
            for (int n = 0; n < Json::Array::size(item); n++)
            {
                Json::Object* point = Json::Array::get(item, n);
                if (point == nullptr)
                {
                    continue;;
                }
                Event::Point_S stPoint;
                convert.structure(point, stPoint);
                line.push_back(stPoint);
            }
            stRuleInfo.lines.push_back(line);
        }
            
        int nAreaNum = 0;
        while (1)
        {
            nAreaNum++;
            std::string key = "Area" + std::to_string(nAreaNum);
            Json::Object* item = Json::get(pRootJson, key);
            if (item == nullptr)
            {
                break;
            }
            Event::Area area;
            for (int n = 0; n < Json::Array::size(item); n++)
            {
                Json::Object* point = Json::Array::get(item, n);
                if (point == nullptr)
                {
                    continue;
                }
                Event::Point_S stPoint;
                convert.structure(point, stPoint);
                area.push_back(stPoint);
            }
            stRuleInfo.areas.push_back(area);
        }
    }
    else
    {
        int nLineNum = 0;
        for (auto &line : stRuleInfo.lines)
        {
            nLineNum++;
            std::string key = "Line" + std::to_string(nLineNum);
            convert.structure(pRootJson, key, line);
        }

        int nAreaNum = 0;
        for (auto &area : stRuleInfo.areas)
        {
            nAreaNum++;
            std::string key = "Area" + std::to_string(nAreaNum);
            convert.structure(pRootJson, key, area);
        }
    }
    
}

void Convert::deal(Json::Object* pRootJson, std::vector<Event::RuleInfo_S>& ruleInfos, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }

    Convert::CConvert convert(bOutStruct);
    convert.structure(pRootJson, "RuleInfos", ruleInfos);
}

void Convert::deal(Json::Object* pRootJson, Event::FaceLibInfo_S &stTargetLibInfo, bool bOutStruct)
{
	if (!pRootJson)
	{
		return;
	}

	Convert::CConvert convert(bOutStruct);
	convert.field(pRootJson, "LibId", stTargetLibInfo.strFaceLibName);
	convert.field(pRootJson, "TotalFace", stTargetLibInfo.nTotalFace);
	convert.field(pRootJson, "NormalNum", stTargetLibInfo.nNormalNum);
	convert.field(pRootJson, "AbnormalNum", stTargetLibInfo.nAbnormalNum);
}

void Convert::deal(Json::Object* pRootJson, std::vector<Event::FaceLibInfo_S> &targetLibInfos, bool bOutStruct)
{
	if (!pRootJson)
	{
		return;
	}

	Convert::CConvert convert(bOutStruct);
	convert.structure(pRootJson, "TargetLibInfos", targetLibInfos);
}

void Convert::deal(Json::Object* pRootJson, std::list<Event::FaceLibInfo_S> &targetLibInfos, bool bOutStruct)
{
	if (!pRootJson)
	{
		return;
	}

	Convert::CConvert convert(bOutStruct);
	convert.structure(pRootJson, "TargetLibInfos", targetLibInfos);
}

void Convert::deal(Json::Object* pRootJson, Event::AssociateLibInfo_S& stAssoFaceLibInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }

    deal(pRootJson, stAssoFaceLibInfo.faceLibInfo, bOutStruct);
    Convert::CConvert convert(bOutStruct);

    convert.field(pRootJson, "Similarity", stAssoFaceLibInfo.nSimilarity);
    convert.field(pRootJson, "Enable", stAssoFaceLibInfo.bEnable);
}

void Convert::deal(Json::Object* pRootJson, std::vector<Event::AssociateLibInfo_S>& stAssoFaceLibInfos, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }

    Convert::CConvert convert(bOutStruct);
    convert.structure(pRootJson, "TargetLibInfos", stAssoFaceLibInfos);
}

void Convert::deal(Json::Object* pRootJson, std::list<Event::AssociateLibInfo_S>& stAssoFaceLibInfos, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }

    Convert::CConvert convert(bOutStruct);
    convert.structure(pRootJson, "TargetLibInfos", stAssoFaceLibInfos);
}


void Convert::deal(Json::Object* pRootJson, Event::FaceFind_S &stFaceFind, bool bOutStruct)
{
	if (!pRootJson)
	{
		return;
	}

	Convert::CConvert convert(bOutStruct);
    convert.field(pRootJson, "LibId", stFaceFind.strFaceLibName);
	convert.field(pRootJson, "Name", stFaceFind.strName);
	convert.field(pRootJson, "PhoneNum", stFaceFind.strPhoneNum);
	convert.field(pRootJson, "ModelState", stFaceFind.nModelState);
	convert.field(pRootJson, "RatingLevel", stFaceFind.nRatingLevel);

}

void Convert::deal(Json::Object* pRootJson, Event::FaceIdInfo_S &stFaceIdInfo, bool bOutStruct)
{
	if (!pRootJson)
	{
		return;
	}

	Convert::CConvert convert(bOutStruct);
    convert.field(pRootJson, "Ids", stFaceIdInfo.ids);
}

void Convert::deal(Json::Object* pRootJson, Event::AddFaceInfoResult &stAddFaceInfo, bool bOutStruct)
{
    if (!pRootJson)
	{
		return;
	}

	Convert::CConvert convert(bOutStruct);
    convert.field(pRootJson, "nRet", stAddFaceInfo.nRet);
	convert.field(pRootJson, "hashId", stAddFaceInfo.hashId);
}
void Convert::deal(Json::Object* pRootJson, Event::FaceInfo_S &stFaceInfo, bool bOutStruct)
{
	if (!pRootJson)
	{
		return;
	}

	Convert::CConvert convert(bOutStruct);
    convert.field(pRootJson, "Id", stFaceInfo.nId);
	convert.field(pRootJson, "LibId", stFaceInfo.strFaceLibName);
	convert.field(pRootJson, "Name", stFaceInfo.strName);
	convert.field(pRootJson, "PhoneNum", stFaceInfo.strPhoneNum);
	convert.field(pRootJson, "PicPath", stFaceInfo.strPicPath);
	convert.field(pRootJson, "PicType", stFaceInfo.strPicType);
	convert.field(pRootJson, "PicSize", stFaceInfo.nPicSize);
	convert.field(pRootJson, "PicDate", stFaceInfo.strPicDate);
	convert.field(pRootJson, "ModelState", stFaceInfo.nModelState);
	convert.field(pRootJson, "RatingLevel", stFaceInfo.nRatingLevel);
    convert.field(pRootJson, "BinPath", stFaceInfo.BinPath);
}


void Convert::deal(Json::Object* pRootJson, std::vector<Event::FaceInfo_S> &faceInfos, bool bOutStruct)
{
	if (!pRootJson)
	{
		return;
	}

	Convert::CConvert convert(bOutStruct);
	convert.structure(pRootJson, "FaceInfos", faceInfos);
}

void Convert::deal(Json::Object* pRootJson, Event::FaceCompareInfo_S &stFaceCompareInfo, bool bOutStruct)
{
	if (!pRootJson)
	{
		return;
	}

	Convert::CConvert convert(bOutStruct);
	convert.field(pRootJson, "EventId", stFaceCompareInfo.nEventId);
	convert.field(pRootJson, "CompResult", stFaceCompareInfo.nCompResult);
	convert.field(pRootJson, "Similarity", stFaceCompareInfo.nSimilarity);
	convert.field(pRootJson, "FaceId", stFaceCompareInfo.nFaceId);
	convert.field(pRootJson, "FaceLibName", stFaceCompareInfo.strFaceLibName);
	convert.field(pRootJson, "FaceName", stFaceCompareInfo.strFaceName);
	convert.field(pRootJson, "LibFacePath", stFaceCompareInfo.strLibFacePath);
	convert.field(pRootJson, "CapFacePath", stFaceCompareInfo.strCapFacePath);
	convert.field(pRootJson, "CapImagePath", stFaceCompareInfo.strCapImagePath);
    convert.structure(pRootJson, stFaceCompareInfo.stInfo);
}


void Convert::deal(Json::Object* pRootJson, std::vector<Event::FaceCompareInfo_S> &faceCompareInfos, bool bOutStruct)
{
	if (!pRootJson)
	{
		return;
	}

	Convert::CConvert convert(bOutStruct);
	convert.structure(pRootJson, "FaceCompareInfos", faceCompareInfos);
}
void Convert::deal(Json::Object* pRootJson, Event::AlarmInfo_S &stAlarmInfo, bool bOutStruct)
{
	if (!pRootJson)
	{
		return;
	}
    
	Convert::CConvert convert(bOutStruct);
    convert.field(pRootJson, "ChnId", stAlarmInfo.nChnId);
    convert.field(pRootJson, "AlarmType", (int&)stAlarmInfo.nType);
    convert.field(pRootJson, "AlarmTime", stAlarmInfo.time);
    convert.field(pRootJson, "AlarmIp", stAlarmInfo.ip);
    convert.field(pRootJson, "AlarmCategory", (int&)stAlarmInfo.enCategory);
    convert.field(pRootJson, "VideoPath", stAlarmInfo.videoPath);
}

void Convert::deal(Json::Object* pRootJson, Event::TargetInfo_S &stTargetInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }
    
    Convert::CConvert convert(bOutStruct);

    convert.field(pRootJson, "ChnId", stTargetInfo.nChnId);
    convert.field(pRootJson, "Type", stTargetInfo.nType);
    convert.structure(pRootJson, "Rect", stTargetInfo.vecRect);
}

void Convert::deal(Json::Object* pRootJson, std::vector<Event::TargetInfo_S> &stTargetInfos, bool bOutStruct)
{
	if (!pRootJson)
	{
		return;
	}

	Convert::CConvert convert(bOutStruct);
	convert.structure(pRootJson, "TargetInfos", stTargetInfos);
}
