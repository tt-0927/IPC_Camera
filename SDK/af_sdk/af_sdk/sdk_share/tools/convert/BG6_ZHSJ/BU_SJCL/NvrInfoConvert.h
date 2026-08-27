/*
 * @FilePath     : sdk_new/sdk_share/tools/convert/BG6_ZHSJ/BU_SJCL/NvrInfoConvert.h
 * @Author       : ITC
 * @Date         : 2026-08-21
 * @LastEditors  : ITC
 * @LastEditTime : 2026-08-21
 * @Description  : NVR 侧杂项转换
 *                 收口 抓拍/人脸/对讲/人流/AI 分析配置等 NVR 侧非纯报警结构体。
 */
#pragma once

#include <string>
#include <vector>
#include <set>

#include "Json.h"

/* 库通用头文件 */
#ifdef NET_SDK_SERVER_API
    #include "NetTVSDKServerInterface.h"
#elif defined(NET_SDK_CLIENT_API)
    #include "NetTVSDKClientInterface.h"
#else
    #include "NetTVSDKCommon.h"
#endif

namespace SDKConvert
{
    void deal(Json::Object *pRootJson, NET_CaptureTime_S &stInfo, bool bOutStruct);
    void deal(Json::Object *pRootJson, NET_CaptureDaySchedule_S &stInfo, bool bOutStruct);
    void deal(Json::Object *pRootJson, NET_CapturePlanInfo_S &stInfo, bool bOutStruct);
    void deal(Json::Object *pRootJson, NET_CaptureConfig_S &stInfo, bool bOutStruct);
    void deal(Json::Object *pRootJson, NET_CaptureParamInfo_S &stInfo, bool bOutStruct);

    void deal(Json::Object* pRootJson, NET_FaceCaptureRegion_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_FaceCaptureRule_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_FaceCaptureInfo_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_FaceCompareInfo_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_FaceLibInfo_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_FaceLibList_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_FaceIdInfo_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_FaceInfo_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_FaceInfoList_S& stInfo, bool bOutStruct);

    void deal(Json::Object* pRootJson, NET_TalkbackStateInfo_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_TalkbackStreamInfo_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_ReplayTalkbackInfo_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_VoiceComAudioCfg_S& stInfo, bool bOutStruct);

    void deal(Json::Object* pRootJson, NET_PeopleFlowRuleLine_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_PeopleAlarmRule_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_PeopleAlarmConfig_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_StatisticsResetConfig_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_PeopleFlowStatisticsCfg_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_PeopleDensityDetectionCfg_S& stInfo, bool bOutStruct);

    void deal(Json::Object* pRootJson, NET_ManholeCoverAbnormalCfg_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_SleepOnDutyCfg_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_ElectricVehicleInElevatorCfg_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_PersonFallDownCfg_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_ConstructionOccupyRoadCfg_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_CongestionCfg_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_LicensePlateRecognitionCfg_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_HighAltitudeSeatbeltCfg_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_SafetyHelmetCfg_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_PersonFallCfg_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_PhoneUsageCfg_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_SmokingCfg_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_OpenFlameCfg_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_BareSoilCfg_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_HoleProtectionBarCfg_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_ReflectiveClothingCfg_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_PetRecognitionInfo_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_ClimbFenceInfo_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_DimissionInfo_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_IllegalLaneInfo_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_RetrogradeInfo_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_NonmotorVehicleIntrusionInfo_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_OccupationEmergencyInfo_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_PedestrianIntrusionInfo_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_SmokeFireCfg_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_RoadPondingCfg_S& stInfo, bool bOutStruct);
}
