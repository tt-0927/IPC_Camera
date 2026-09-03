/**
 * @FilePath     : algo_stream_deal.cpp
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2025-06-06 16:02:10
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2025-08-15 16:58:55
 * @Description  : 负责取流传给算法检测
 */

#include "algo_stream_deal.h"
#include "motion_detect.hpp"
#include "hide_detect.hpp"
#include "object_detect.hpp"
#include "face_detect.hpp"
#include "scene_change_detect.hpp"
#include "pet_recognition.hpp"
#include "audio_detect.hpp"
#include "group1_detect.hpp"
#include "group2_group4_detect.hpp"
#include "group3_detect.hpp"
#include "group5_detect.hpp"
// #include "sleep_detect.hpp"
// #include "trip_detect.hpp"
// #include "phoneUsage_detect.hpp"
// #include "person_detect.hpp"
// #include "fire_detect.hpp"
// #include "stree_detect.hpp"
// #include "rubish_detect.hpp"
// #include "constructionEncroachmentRoad_detect.hpp"
// #include "highSafyBelt_Soil_detect.hpp"
// #include "helmet_detect.hpp"
// #include "reflective_detect.hpp"
// #include "smoking_detect.hpp"
// #include "fence_detect.hpp"
// #include "pmnm_detect.hpp"
// #include "electricScooter_detect.hpp"
// #include "vehicle_detect.hpp"
// #include "licensePlateCognition_detect.hpp"

int CAlgoStreamDeal::init()
{
    /*初始化算法句柄*/
    runOnce();
    m_StreamHandler = std::make_shared<CStreamHandler>();
    dlog_debug("AI_APP: 初始化算法音视频流数据处理");
    return 0;
}

void CAlgoStreamDeal::deinit()
{
    /* 取消所有绑定 */
    unbindVideoSig(m_StreamHandler.get());

    /* 统一存储需要释放的句柄 */
    std::vector<std::shared_ptr<CAlgorithm>> algos;
#ifdef DEVICE_TV_3882TI
    algos.emplace_back(std::move(m_pVisionTextAlgo));
#endif
    algos.emplace_back(std::move(m_pMotionAlgo));
    algos.emplace_back(std::move(m_pHideAlgo));

    algos.emplace_back(std::move(m_pGroup1Algo));
    algos.emplace_back(std::move(m_pGroup2AndGroup4Algo));
    algos.emplace_back(std::move(m_pGroup3Algo));
    algos.emplace_back(std::move(m_pGroup5Algo));

    algos.emplace_back(std::move(m_pObjectDetectAlgo));
    // algos.emplace_back(std::move(m_pLicensePlateCognitionDetectAlgo));
    algos.emplace_back(std::move(m_pSceneChangeAlgo));
    algos.emplace_back(std::move(m_pPetAlgo));
    algos.emplace_back(std::move(m_pAudioAlgo));
    algos.emplace_back(std::move(m_pFaceDetectAlgo));

#if 0
     algos.emplace_back(std::move(m_pPersonDetectAlgo));
     algos.emplace_back(std::move(m_pFireDetectAlgo));
     algos.emplace_back(std::move(m_pStreeDetectAlgo));
     algos.emplace_back(std::move(m_pRubishDetectAlgo));
     algos.emplace_back(std::move(m_pSleepDetectAlgo));
     algos.emplace_back(std::move(m_pTripDetectAlgo));
     // algos.emplace_back(std::move(m_pPhoneUsageAlgo));
     algos.emplace_back(std::move(m_pConstructionEncroachmentRoadDetectAlgo));
     algos.emplace_back(std::move(m_pHighSafyBeltSoilDetecAlgo));
     algos.emplace_back(std::move(m_pHelmetAlgo));
     algos.emplace_back(std::move(m_pReflectiveAlgo));
     algos.emplace_back(std::move(m_pSmokingAlgo));
     algos.emplace_back(std::move(m_pFenceAlgo));
     algos.emplace_back(std::move(m_pPersonMotorNomotorAlgo));
     algos.emplace_back(std::move(m_pElectricScooterDetectAlgo));
     algos.emplace_back(std::move(m_pVehicleAlgo));
#endif

    /* 释放算法实例 */
    for (auto &pAlgo : algos)
    {
        pAlgo.reset();
        dlog_debug("AI_APP: 释放算法实例 引用计数为[%d]", pAlgo.use_count());
    }

    /* 释放 StreamHandler */
    m_StreamHandler.reset();
}

void CAlgoStreamDeal::deal_message(const void *pData, int nLength, int nH, int nW)
{
    if (pData != nullptr)
    {
        if (m_StreamHandler)
        {
            m_StreamHandler.get()->recvDataProcess(pData, nLength, nH, nW);
        }
        else
        {
            dlog_error("m_StreamHandler is null");
        }
    }
    else
    {
        dlog_error("收到无效消息或空数据");
    }
}

void CAlgoStreamDeal::deal_audioStreamData(const void *pData, int nLength)
{
    if (pData != nullptr && nLength > 0)
    {
        if (m_StreamHandler)
        {
            m_StreamHandler->recvDataProcess(pData, nLength);
        }
        else
        {
            dlog_error("m_StreamHandler is null");
        }
    }
    else
    {
        dlog_error("收到无效消息或空数据");
    }
}

void CAlgoStreamDeal::bindRecvFunc(Event::AlgorithmConfig &stAlgoConfig)
{
    if (!m_StreamHandler)
    {
        dlog_error("绑定回调失败， m_StreamHandler未初始化");
        return;
    }

    
    Alarm::AttributeDetectSwitch_S stAttributeDetectSwitch;

    CEventConfigure::instance()->get_configure(stAttributeDetectSwitch);

    /* 配置项和算法对象的映射 */
    std::vector<std::pair<bool, std::shared_ptr<CAlgorithm>>> algoBindings =
        {
#ifdef DEVICE_TV_3882TI
            /*AI智能场景分析*/
            {stAlgoConfig.nEnLLmInference || stAlgoConfig.nEnTextPreset || stAlgoConfig.nEnAISceneAnalysis, m_pVisionTextAlgo},
#endif
            /* 移动侦测 */
            {stAlgoConfig.nEnMotionDetect, m_pMotionAlgo},
            /* 遮挡侦测 */
            {stAlgoConfig.nEnOcclusionDetect, m_pHideAlgo},
            /* notHelmet(未戴安全帽)、helmet(安全帽)、reflective(反光衣)、safetyRope(安全绳)、exposedSoil(泥土裸露)、person(人) */
            {stAlgoConfig.nEnSafetyHelmet || stAlgoConfig.nEnReflectiveClothing || stAlgoConfig.nEnHighAltitudeSeatbelt || stAlgoConfig.nEnBareSoil, m_pGroup1Algo},
            /* penson(人)、Car(机动车)、NonCar(非机动车) */
            {stAlgoConfig.nEnLineCrossing || stAlgoConfig.nEnIntrusion || stAlgoConfig.nEnEnterRegion || stAlgoConfig.nEnLeaveRegion ||
                 stAlgoConfig.nEnLoiteringDetect || stAlgoConfig.nEnFenceClimbing || stAlgoConfig.nEnLeavePost || stAlgoConfig.nEnPedestrianIntrusion || stAlgoConfig.nEnCrowdGathering || stAlgoConfig.nEnPersonFallDown ||
                 stAlgoConfig.nEnEmergencyLaneOccupancy || stAlgoConfig.nEnNonMotorVehicleIntrusion || stAlgoConfig.nEnElectricVehicleInElevator ||
                 stAlgoConfig.nEnReverseDirection || stAlgoConfig.nEnCongestion || stAlgoConfig.nEnIllegalParking || stAlgoConfig.nEnParkingDetect || stAlgoConfig.nEnIllegalLaneChange ||
                 stAlgoConfig.nEnSmoking || stAlgoConfig.nEnSleepOnDuty || stAlgoConfig.nEnPhoneUsage || stAlgoConfig.nEnTrip || stAlgoConfig.nPlateNumber || 
                 stAttributeDetectSwitch.bPedestrianAttribute || stAttributeDetectSwitch.bMotorVehicleAttribute || stAttributeDetectSwitch.bNonMotorVehicleAttribute,
             m_pGroup2AndGroup4Algo},
            /* smoke(烟雾)、fire(火焰)、Overflow(垃圾满溢)、expose(垃圾暴露)、Complete(井盖完好)、Damaged(井盖破损)、Lost(井盖丢失)、Uncovered(未盖井盖)、BreakoutOfOuterEdge(井盖外边沿破损)、WaterAccumulation(道路积水) */
            {stAlgoConfig.nEnSmokeFire || stAlgoConfig.nEnOpenFlame || stAlgoConfig.nEnGarbageExposure || stAlgoConfig.nEnGarbageOverflow || stAlgoConfig.nEnManholeCoverAbnormal || stAlgoConfig.nEnRoadPonding, m_pGroup3Algo},
            // /* cigarette(香烟)、sleep(睡觉)、phone(玩手机)、fall(摔跤)、falling(摔跤中)
            // {stAlgoConfig.nEnSmoking || stAlgoConfig.nEnSleepOnDuty || stAlgoConfig.nEnPhoneUsage || stAlgoConfig.nEnTrip, m_pGroup4Algo},
            /* metalFence(金属栅栏)、ConeTank(锥形桶)、CrashBarrels(防撞桶)、fence(防护栏) */
            {stAlgoConfig.nEnHoleProtectionBar || stAlgoConfig.nEnConstructionOccupyRoad, m_pGroup5Algo},
            /* 物品检测 */
            {stAlgoConfig.nEnUnattendedObject || stAlgoConfig.nEnObjectRemoval, m_pObjectDetectAlgo},
            // /* 车牌识别检测 */
            // {stAlgoConfig.nPlateNumber, m_pLicensePlateCognitionDetectAlgo},
            /* 场景变更识别检测 */
            {stAlgoConfig.nEnSceneChange, m_pSceneChangeAlgo},
            /* 宠物识别检测 */
            {stAlgoConfig.nEnPetRecognition, m_pPetAlgo},
            /* 人脸检测 */
            {stAlgoConfig.nEnFaceCapture || stAlgoConfig.nEnFaceDetect || stAttributeDetectSwitch.bFaceAttribute, m_pFaceDetectAlgo},

#if 0
         /* 行人检测相关 */
         {stAlgoConfig.nEnLeavePost || stAlgoConfig.nEnPersonFallDown || stAlgoConfig.nEnFenceClimbing || stAlgoConfig.nEnPhoneUsage ||
         stAlgoConfig.nEnLoiteringDetect || stAlgoConfig.nEnPedestrianIntrusion || stAlgoConfig.nEnCrowdGathering, m_pPersonDetectAlgo},
         /* 火焰检测 */
         { stAlgoConfig.nEnSmokeFire || stAlgoConfig.nEnOpenFlame ,m_pFireDetectAlgo},
          /* 街道检测 */
         { stAlgoConfig.nEnManholeCoverAbnormal || stAlgoConfig.nEnRoadPonding ,m_pStreeDetectAlgo},
         /* 垃圾检测 */
         { stAlgoConfig.nEnGarbageExposure || stAlgoConfig.nEnGarbageExposure ,m_pRubishDetectAlgo},
         /* 睡岗识别 */
         { stAlgoConfig.nEnSleepOnDuty ,m_pSleepDetectAlgo},
         /* 摔倒识别 */
         { stAlgoConfig.nEnTrip ,m_pTripDetectAlgo},
         // /* 玩手机识别 */
         // { stAlgoConfig.nEnPhoneUsage ,m_pPhoneUsageAlgo},
         /* 施工占道检测 */
         { stAlgoConfig.nEnConstructionOccupyRoad, m_pConstructionEncroachmentRoadDetectAlgo},
         /*高空安全带黄土裸露 */
         { stAlgoConfig.nEnHighAltitudeSeatbelt || stAlgoConfig.nEnBareSoil ,m_pHighSafyBeltSoilDetecAlgo},
         /* 安全帽检测 */ 
         { stAlgoConfig.nEnSafetyHelmet ,m_pHelmetAlgo},
         /* 反光衣检测 */ 
         { stAlgoConfig.nEnReflectiveClothing ,m_pReflectiveAlgo},
         /* 抽烟检测 */ 
         { stAlgoConfig.nEnSmoking ,m_pSmokingAlgo},
         /* 防护栏检测 */ 
         { stAlgoConfig.nEnHoleProtectionBar ,m_pFenceAlgo},
         /* 机动车、行人、非机动车侦测 */ 
         {stAlgoConfig.nEnLineCrossing || stAlgoConfig.nEnIntrusion || stAlgoConfig.nEnEnterRegion || stAlgoConfig.nEnLeaveRegion || stAlgoConfig.nEnEmergencyLaneOccupancy || stAlgoConfig.nEnNonMotorVehicleIntrusion, m_pPersonMotorNomotorAlgo},
         /* 电瓶车检测 */ 
         { stAlgoConfig.nEnElectricVehicleInElevator, m_pElectricScooterDetectAlgo},
 
         { stAlgoConfig.nEnReverseDirection || stAlgoConfig.nEnCongestion || stAlgoConfig.nEnIllegalParking || stAlgoConfig.nEnParkingDetect || stAlgoConfig.nEnIllegalLaneChange, m_pVehicleAlgo},
#endif
        };

    for (const auto &binding : algoBindings)
    {
        if (binding.first && binding.second)
        {
            dlog_debug("AI_APP: 绑定视频回调成功");
            bindVideoSlot<CAlgorithm>(m_StreamHandler.get(), binding.second.get(), &CAlgorithm::recvMediaData);
        }
    }

    /* 音频 配置项和算法对象的映射 */
    std::vector<std::pair<bool, std::shared_ptr<CAlgorithm>>> audioAlgoBindings =
        {
            /* 音频异常识别检测 */
            {stAlgoConfig.nEnAudioAnomaly, m_pAudioAlgo},
        };

    for (const auto &audioBinding : audioAlgoBindings)
    {
        if (audioBinding.first && audioBinding.second)
        {
            dlog_debug("AI_APP: 绑定音频回调成功");
            bindAudioSlot<CAlgorithm>(m_StreamHandler.get(), audioBinding.second.get(), &CAlgorithm::recvMediaData);
        }
    }
}

void CAlgoStreamDeal::manageAlgorithmInstances(const Event::AlgorithmConfig &stAlgoConfig)
{
    int bIsNew = 0;

    Alarm::AttributeDetectSwitch_S stAttributeDetectSwitch;
    /* 获取属性识别开关信息 */
    CEventConfigure::instance()->get_configure(stAttributeDetectSwitch);

#ifdef DEVICE_TV_3882TI
    /* 场景智能分析算法 */
    bIsNew += manageSingleAlgorithm(m_pVisionTextAlgo, stAlgoConfig.nEnLLmInference || stAlgoConfig.nEnTextPreset || stAlgoConfig.nEnAISceneAnalysis, []() -> std::shared_ptr<CAlgorithm> {
        return std::static_pointer_cast<CAlgorithm>(std::make_shared<CVisionTextManager>());
    });
#endif
    /* 移动侦测算法 */
    bIsNew += manageSingleAlgorithm(m_pMotionAlgo, stAlgoConfig.nEnMotionDetect, []() -> std::shared_ptr<CAlgorithm> {
        return std::static_pointer_cast<CAlgorithm>(std::make_shared<CMotionDetect>());
    });
    /* 遮挡侦测算法 */
    bIsNew += manageSingleAlgorithm(m_pHideAlgo, stAlgoConfig.nEnOcclusionDetect, []() {
        return std::static_pointer_cast<CAlgorithm>(std::make_shared<CHideDetect>());
    });

    /* group1：notHelmet(未戴安全帽)、helmet(安全帽)、reflective(反光衣)、safetyRope(安全绳)、exposedSoil(泥土裸露)、person(人) */
    bIsNew += manageSingleAlgorithm(m_pGroup1Algo,
                                    stAlgoConfig.nEnSafetyHelmet || stAlgoConfig.nEnReflectiveClothing || stAlgoConfig.nEnHighAltitudeSeatbelt || stAlgoConfig.nEnBareSoil,
                                    [this]() { return std::static_pointer_cast<CAlgorithm>(std::make_shared<CGroup1Detect>()); });

    /* group2：penson(人)、Car(机动车)、NonCar(非机动车)   group4：cigarette(香烟)、sleep(睡觉)、phone(玩手机)、fall(摔跤)、falling(摔跤中)*/
    bIsNew += manageSingleAlgorithm(m_pGroup2AndGroup4Algo,
                                    stAlgoConfig.nEnLineCrossing || stAlgoConfig.nEnIntrusion || stAlgoConfig.nEnEnterRegion || stAlgoConfig.nEnLeaveRegion ||
                                        stAlgoConfig.nEnLoiteringDetect || stAlgoConfig.nEnFenceClimbing || stAlgoConfig.nEnLeavePost || stAlgoConfig.nEnPedestrianIntrusion ||
                                        stAlgoConfig.nEnCrowdGathering || stAlgoConfig.nEnPersonFallDown || stAlgoConfig.nEnEmergencyLaneOccupancy ||
                                        stAlgoConfig.nEnNonMotorVehicleIntrusion || stAlgoConfig.nEnElectricVehicleInElevator || stAlgoConfig.nEnReverseDirection ||
                                        stAlgoConfig.nEnCongestion || stAlgoConfig.nEnIllegalParking || stAlgoConfig.nEnParkingDetect || stAlgoConfig.nEnIllegalLaneChange ||
                                        stAlgoConfig.nEnSmoking || stAlgoConfig.nEnSleepOnDuty || stAlgoConfig.nEnPhoneUsage || stAlgoConfig.nEnTrip || stAlgoConfig.nPlateNumber || 
                                        stAttributeDetectSwitch.bPedestrianAttribute || stAttributeDetectSwitch.bMotorVehicleAttribute || stAttributeDetectSwitch.bNonMotorVehicleAttribute, 
                                    [this]() { return std::static_pointer_cast<CAlgorithm>(std::make_shared<CGroup2_Group4Detect>()); });

    /* group3：smoke(烟雾)、fire(火焰)、Overflow(垃圾满溢)、expose(垃圾暴露)、井盖异常、WaterAccumulation(道路积水) */
    bIsNew += manageSingleAlgorithm(m_pGroup3Algo,
                                    stAlgoConfig.nEnSmokeFire || stAlgoConfig.nEnOpenFlame || stAlgoConfig.nEnGarbageExposure || stAlgoConfig.nEnGarbageOverflow || stAlgoConfig.nEnManholeCoverAbnormal || stAlgoConfig.nEnRoadPonding,
                                    [this]() { return std::static_pointer_cast<CAlgorithm>(std::make_shared<CGroup3Detect>()); });

    /* group5：metalFence(金属栅栏)、ConeTank(锥形桶)、CrashBarrels(防撞桶)、fence(防护栏) */
    bIsNew += manageSingleAlgorithm(m_pGroup5Algo,
                                    stAlgoConfig.nEnHoleProtectionBar || stAlgoConfig.nEnConstructionOccupyRoad,
                                    [this]() { return std::static_pointer_cast<CAlgorithm>(std::make_shared<CGroup5Detect>()); });

    /* 人脸检测算法 */
    bIsNew += manageSingleAlgorithm(m_pFaceDetectAlgo,
                                    stAlgoConfig.nEnFaceCapture || stAlgoConfig.nEnFaceDetect || stAttributeDetectSwitch.bFaceAttribute,
                                    [this]() { return std::static_pointer_cast<CAlgorithm>(std::make_shared<CFaceDetect>()); });

    // /* 车牌识别检测算法 */
    // bIsNew += manageSingleAlgorithm(m_pLicensePlateCognitionDetectAlgo, stAlgoConfig.nPlateNumber, []() {
    //     return std::static_pointer_cast<CAlgorithm>(std::make_shared<CLicensePlateCognitionDetect>());
    // });

    /* 场景变更检测算法 */
    bIsNew += manageSingleAlgorithm(m_pSceneChangeAlgo, stAlgoConfig.nEnSceneChange, []() {
        return std::static_pointer_cast<CAlgorithm>(std::make_shared<SceneChangeDetect>());
    });

    bIsNew += manageSingleAlgorithm(m_pObjectDetectAlgo, 
        stAlgoConfig.nEnUnattendedObject || stAlgoConfig.nEnObjectRemoval,
        [this]() { return std::static_pointer_cast<CAlgorithm>(std::make_shared<CObjectDetect>()); });

    /* 宠物识别检测算法 */
    bIsNew += manageSingleAlgorithm(m_pPetAlgo, stAlgoConfig.nEnPetRecognition, []() {
        return std::static_pointer_cast<CAlgorithm>(std::make_shared<CPetRecognition>());
    });

    /* 音频异常识别检测算法 */
    bIsNew += manageSingleAlgorithm(m_pAudioAlgo, stAlgoConfig.nEnAudioAnomaly, []() {
        return std::static_pointer_cast<CAlgorithm>(std::make_shared<CAudioDetect>());
    });

#if 0
     /* 行人区域检测算法 */
     bIsNew += manageSingleAlgorithm(m_pPersonDetectAlgo, 
                 stAlgoConfig.nEnLeavePost || stAlgoConfig.nEnPersonFallDown || stAlgoConfig.nEnFenceClimbing || stAlgoConfig.nEnPhoneUsage
                          || stAlgoConfig.nEnLoiteringDetect || stAlgoConfig.nEnPedestrianIntrusion || stAlgoConfig.nEnCrowdGathering,
                          [this]() { return std::static_pointer_cast<CAlgorithm>(std::make_shared<CPersonDetect>()); });

     /* 火焰检测算法 */
     bIsNew += manageSingleAlgorithm(m_pFireDetectAlgo, 
                         stAlgoConfig.nEnSmokeFire || stAlgoConfig.nEnOpenFlame,
                         [this]() { return std::static_pointer_cast<CAlgorithm>(std::make_shared<CFireDetect>()); });
     /* 街道检测算法 */
     bIsNew += manageSingleAlgorithm(m_pStreeDetectAlgo, 
                         stAlgoConfig.nEnManholeCoverAbnormal || stAlgoConfig.nEnRoadPonding,
                         [this]() { return std::static_pointer_cast<CAlgorithm>(std::make_shared<CStreeDetect>()); });
     /* 垃圾检测算法 */
     bIsNew += manageSingleAlgorithm(m_pRubishDetectAlgo, 
                         stAlgoConfig.nEnGarbageExposure || stAlgoConfig.nEnGarbageExposure,
                         [this]() { return std::static_pointer_cast<CAlgorithm>(std::make_shared<CRubishDetect>()); });
     /* 睡岗识别算法 */
     bIsNew += manageSingleAlgorithm(m_pSleepDetectAlgo, 
                         stAlgoConfig.nEnSleepOnDuty,
                         [this]() { return std::static_pointer_cast<CAlgorithm>(std::make_shared<CSleepDetect>()); });
      /* 摔倒识别算法 */
     bIsNew += manageSingleAlgorithm(m_pTripDetectAlgo, 
                         stAlgoConfig.nEnTrip,
                         [this]() { return std::static_pointer_cast<CAlgorithm>(std::make_shared<CTripDetect>()); });
     //  /* 玩手机识别算法 */
     // bIsNew += manageSingleAlgorithm(m_pPhoneUsageAlgo, 
     //                     stAlgoConfig.nEnPhoneUsage,
     //                     [this]() { return std::static_pointer_cast<CAlgorithm>(std::make_shared<CPhoneUsageDetect>()); });
     /* 施工占道检测算法 */
     bIsNew += manageSingleAlgorithm(m_pConstructionEncroachmentRoadDetectAlgo, stAlgoConfig.nEnConstructionOccupyRoad,
                                     []()
                                     {
                                         return std::static_pointer_cast<CAlgorithm>(std::make_shared<CConstructionEncroachmentRoadDetect>());
                                     });
     
     /* 高空安全带黄土裸露检测算法 */
     bIsNew += manageSingleAlgorithm(m_pHighSafyBeltSoilDetecAlgo, 
                         stAlgoConfig.nEnHighAltitudeSeatbelt || stAlgoConfig.nEnBareSoil,
                         [this]() { return std::static_pointer_cast<CAlgorithm>(std::make_shared<CHighSafyBeltSoilDetect>()); });
     /* 安全帽识别算法 */
     bIsNew += manageSingleAlgorithm(m_pHelmetAlgo, 
                         stAlgoConfig.nEnSafetyHelmet,
                         [this]() { return std::static_pointer_cast<CAlgorithm>(std::make_shared<CHelmetDetect>()); });
      /* 反光衣识别算法 */
     bIsNew += manageSingleAlgorithm(m_pReflectiveAlgo, 
                         stAlgoConfig.nEnReflectiveClothing,
                         [this]() { return std::static_pointer_cast<CAlgorithm>(std::make_shared<CReflectiveDetect>()); });
     /* 抽烟识别算法 */
     bIsNew += manageSingleAlgorithm(m_pSmokingAlgo, 
                         stAlgoConfig.nEnSmoking,
                         [this]() { return std::static_pointer_cast<CAlgorithm>(std::make_shared<CSmokingDetect>()); });
     /* 防护栏识别算法 */
     bIsNew += manageSingleAlgorithm(m_pFenceAlgo, 
                         stAlgoConfig.nEnHoleProtectionBar,
                         [this]() { return std::static_pointer_cast<CAlgorithm>(std::make_shared<CFenceDetect>()); });
     
     /* 机动车、行人、非机动车检测算法 */
     bIsNew += manageSingleAlgorithm(m_pPersonMotorNomotorAlgo,stAlgoConfig.nEnLineCrossing || stAlgoConfig.nEnIntrusion || stAlgoConfig.nEnEnterRegion 
                                 || stAlgoConfig.nEnLeaveRegion || stAlgoConfig.nEnEmergencyLaneOccupancy || stAlgoConfig.nEnNonMotorVehicleIntrusion,
                                 []()
                                 {
                                     return std::static_pointer_cast<CAlgorithm>(std::make_shared<CPMNMDetect>());
                                 });
 
     /* 电瓶车检测算法 */
     bIsNew += manageSingleAlgorithm(m_pElectricScooterDetectAlgo, stAlgoConfig.nEnElectricVehicleInElevator,
                                     []()
                                     {
                                         return std::static_pointer_cast<CAlgorithm>(std::make_shared<CElectricScooterDetect>());
                                     });
     
     /* 车辆识别检测算法 */
     bIsNew += manageSingleAlgorithm(m_pVehicleAlgo, stAlgoConfig.nEnReverseDirection || stAlgoConfig.nEnCongestion ||
                                     stAlgoConfig.nEnParkingDetect || stAlgoConfig.nEnIllegalParking || stAlgoConfig.nEnIllegalLaneChange,
                                     []()
                                     {
                                         return std::static_pointer_cast<CAlgorithm>(std::make_shared<CVehicleDetect>());
                                     });
#endif
    if (bIsNew)
    {
        /* 统一获取区域配置 */
        dlog_debug("AI_APP: 统一获取区域配置 bIsNew[%d]", bIsNew);
    }
}

bool CAlgoStreamDeal::manageSingleAlgorithm(std::shared_ptr<CAlgorithm> &algo, bool bEnabled, std::function<std::shared_ptr<CAlgorithm>()> creator)
{
    if (bEnabled)
    {
        if (!algo)
        {
            algo = creator();
            dlog_debug("AI_APP: 初始化算法实例");
            return true;
        }
    }
    else
    {
        if (algo && (algo != m_pVisionTextAlgo))
        {
            algo.reset();
            dlog_debug("AI_APP: 释放算法实例 引用计数为[%d]", algo.use_count());
            return false;
        }
    }
    return false;
}

void CAlgoStreamDeal::set_Algo_EnConfig(Event::AlgorithmConfig &stAlgoConfig)
{
    printAlgoCfg(stAlgoConfig);

    /* 取消所有绑定 */
    unbindVideoSig(m_StreamHandler.get());
    unbindAudioSig(m_StreamHandler.get());

    /* 根据配置管理算法实例 */
    manageAlgorithmInstances(stAlgoConfig);

    /* 重新绑定回调函数 */
    bindRecvFunc(stAlgoConfig);

    /* 通知 Algorithm 更新参数 */
    std::vector<std::shared_ptr<CAlgorithm>> algos = {
#ifdef DEVICE_TV_3882TI
        m_pVisionTextAlgo,
#endif
        m_pMotionAlgo,
        m_pHideAlgo,
        m_pGroup1Algo,
        m_pGroup2AndGroup4Algo,
        m_pGroup3Algo,
        m_pGroup5Algo,
        m_pFaceDetectAlgo,
        // m_pLicensePlateCognitionDetectAlgo,
        m_pSceneChangeAlgo,
        m_pPetAlgo,
        m_pObjectDetectAlgo,
        m_pAudioAlgo

    };

    for (auto &pAlgo : algos)
    {
        if (pAlgo)
        {
            pAlgo->setAlgoEnCfg(stAlgoConfig);
        }
    }
}

// void CAlgoStreamDeal::set_Algo_ParamConfig(Alarm::TargetDetection_S &stAlgoCfg)
// {
//     if (m_pFaceAlgo)
//     {
//         m_pFaceAlgo->setAlgoParamCfg(stAlgoCfg);
//     }
// }

float CAlgoStreamDeal::getCurrentDb() const
{
    if (!m_pAudioAlgo)
    {
        return 0.0f;
    }
    return m_pAudioAlgo.get()->getCurrentDb();
}

void CAlgoStreamDeal::runOnce()
{
    /* 使用静态局部变量来跟踪函数是否已执行过 */
    static bool s_bHasRun = false;

    /* 如果已执行过，直接返回 */
    if (s_bHasRun)
    {
        return;
    }

    s_bHasRun = true;
    // if (!m_pFaceAlgo)
    // {
    //     m_pFaceAlgo = std::static_pointer_cast<CAlgorithm>(std::make_shared<CFaceDetect>());
    // }
}
