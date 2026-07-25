/*
 * @Author: lianghy lianghy@kfb.cn
 * @Date: 2026-01-09 10:47:39
 * @LastEditors: lianghy lianghy@kfb.cn
 * @LastEditTime: 2026-04-25 11:41:18
 * @FilePath: /1126/rv1126b_ipc/main_app/ai_app/detect_mode/group2_group4_detect/group2_group4_detect.hpp
 * @Description: 人、车非 事件相关
 */

#pragma once

#include <atomic>
#include <condition_variable>
#include <mutex>
#include <thread>
#include <sys/time.h>
#include "blocking_queue.hpp"
#include "algorithm.hpp"
#include <opencv2/opencv.hpp>
#include "Group2DetectV1_0.hpp"
#include "Group4DetectV1_0.hpp"
#include "LicensePlateCognitionV1_0.hpp"
#include "PresonAttributeV2_0.hpp"
#include "VehicleAttributeV2_0.hpp"
#include "NonMotorizedAttributeV2_0.hpp"

#ifdef ENABLE_GAT1400_SRC
#include "gat1400_utils.h"
#endif
class CGroup2_Group4Detect : public CAlgorithm {
  public:
    CGroup2_Group4Detect();
    ~CGroup2_Group4Detect();

    typedef struct PMNMDetectRuleInfo
    {
        // std::vector<Group2Detect_NS::DetectionTargetType_E> veDetectionTargets;
        std::vector<int>        veDetectionTargets; /* 0-人 1-机动车 2-非机动车 */
        Alarm::CrossDirection_E enCrossDirection = Alarm::CrossDirection_E::CROSS_DIRECTION_INVALID;
        Event::RuleInfo         stRuleInfo;
    } PMNMDetectRuleInfo_S;

    typedef struct TrafficRuleInfo
    {
        Alarm::CrossDirection_E enCrossDirection = Alarm::CrossDirection_E::CROSS_DIRECTION_INVALID;
        Event::RuleInfo         stRuleInfo;
    } TrafficRuleInfo_S;

    /**
     * @brief   : 接受媒体数据
     * @param    {MediaData_S} stMediaData：媒体数据
     */
    void recvMediaData(MediaData_S stMediaData) override;

    /**
     * @brief   : 更新算法配置参数
     * @param    {AlgorithmConfig} &stAlgoConfig：算法配置
     */
    void setAlgoEnCfg(const Event::AlgorithmConfig &stAlgoConfig) override;

    /**
     * @brief   : 更新越界侦测参数
     * @param    {BoundaryDetection_S} &stAlgoCfg：越界侦测
     */
    void setAlgoParamCfg(const Alarm::BoundaryDetection_S &stAlgoCfg, Event::Type_E enType = Event::Type_E::LINE_CROSSING);

    /**
     * @brief   : 更新区域入侵参数
     * @param    {FieldDetection_S} &stAlgoCfg：区域入侵
     */
    void setAlgoParamCfg(const Alarm::FieldDetection_S &stAlgoCfg, Event::Type_E enType = Event::Type_E::INTRUSION);

    /**
     * @brief   : 更新 进入区域参数
     * @param    {EntranceDetection_S} & 进入区域
     */
    void setAlgoParamCfg(const Alarm::EntranceDetection_S &stAlgoCfg, Event::Type_E enType = Event::Type_E::ENTER_REGION);

    /**
     * @brief   : 更新离开区域参数
     * @param    {ExitingDetection_S} &离开区域
     */
    void setAlgoParamCfg(const Alarm::ExitingDetection_S &stAlgoCfg, Event::Type_E enType = Event::Type_E::LEAVE_REGION);

    /**
     * @brief   : 更新徘徊侦测区域参数
     * @param    {LoiteringDetection_S} &徘徊侦测区域
     */
    void setAlgoParamCfg(const Alarm::LoiteringDetection_S &stAlgoCfg, Event::Type_E enType = Event::Type_E::LOITERING_DETECT);
    /**
     * @brief   : 更新翻越围栏参数
     * @param    {FenceClimbingDetection_S} &翻越围栏区域
     */
    void setAlgoParamCfg(const Alarm::FenceClimbingDetection_S &stAlgoCfg, Event::Type_E enType = Event::Type_E::FENCE_CLIMBING);
    /**
     * @brief   : 更新离岗识别参数
     * @param    {LeavePostDetection_S} &离岗区域
     */
    void setAlgoParamCfg(const Alarm::LeavePostDetection_S &stAlgoCfg, Event::Type_E enType = Event::Type_E::LEAVE_POST);
    /**
     * @brief   : 更新行人闯入参数
     * @param    {PedestrianIntrusionDetection_S} &行人闯入区域
     */
    void setAlgoParamCfg(const Alarm::PedestrianIntrusionDetection_S &stAlgoCfg, Event::Type_E enType = Event::Type_E::PEDESTRIAN_INTRUSION);
    /**
     * @brief   : 更新人员聚集参数
     * @param    {CrowdGathering_S} &人员聚集
     */
    void setAlgoParamCfg(const Alarm::CrowdGathering_S &stAlgoCfg, Event::Type_E enType = Event::Type_E::CROWD_GATHERING);
    /**
     * @brief   : 更新人员倒地参数
     * @param    {CrowdGathering_S} &人员聚集
     */
    void setAlgoParamCfg(const Alarm::PersonFallDownDetection_S &stAlgoCfg, Event::Type_E enType = Event::Type_E::PERSON_FALL_DOWN);

    /**
     * @brief 更新应急车道占用检测区域参数
     * @param stRuleInfo
     */
    void setAlgoParamCfg(const Alarm::EmergencyLaneOccupancyDetection_S &stAlgoCfg, Event::Type_E enType = Event::Type_E::EMERGENCY_LANE_OCCUPANCY);

    /**
     * @brief 更新非机动车闯入识别区域参数
     * @param stRuleInfo
     */
    void setAlgoParamCfg(const Alarm::NonMotorVehicleIntrusionDetection_S &stAlgoCfg, Event::Type_E enType = Event::Type_E::NON_MOTOR_VEHICLE_INTRUSION);

    /**
     * @brief 更新电瓶车进电梯识别参数
     * @param stRuleInfo
     */
    void setAlgoParamCfg(const Alarm::ElectricScooterDetection_S &stAlgoCfg, Event::Type_E enType = Event::Type_E::ELECTRIC_VEHICLE_IN_ELEVATOR);

    /**
     * @brief 更新吸烟识别参数
     * @param stRuleInfo
     */
    void setAlgoParamCfg(const Alarm::SmokingDection_S &stAlgoCfg, Event::Type_E enType = Event::Type_E::SMOKING);

    /**
     * @brief 更新睡觉识别参数
     * @param stRuleInfo
     */
    void setAlgoParamCfg(const Alarm::SleepOnDutyDetection_S &stAlgoCfg, Event::Type_E enType = Event::Type_E::SLEEP_ON_DUTY);

    /**
     * @brief 更新玩手机识别参数
     * @param stRuleInfo
     */
    void setAlgoParamCfg(const Alarm::PhoneUsageDetection_S &stAlgoCfg, Event::Type_E enType = Event::Type_E::PHONE_USAGE);

    /**
     * @brief 更新摔跤识别参数
     * @param stRuleInfo
     */
    void setAlgoParamCfg(const Alarm::TripDetection_S &stAlgoCfg, Event::Type_E enType = Event::Type_E::PERSON_TRIP);

    /**
     * @brief   : 更新逆行侦测参数
     * @param    {DrivingAgainstTrafficDetection_S} &逆行
     */
    void setAlgoParamCfg(const Alarm::DrivingAgainstTrafficDetection_S &stAlgoCfg, Event::Type_E enType = Event::Type_E::REVERSE_DIRECTION);

    /**
     * @brief   : 更新违规变道参数
     * @param    {IllegalLaneChangeDetection_S} &stAlgoCfg：违规变道
     */
    void setAlgoParamCfg(const Alarm::IllegalLaneChangeDetection_S &stAlgoCfg, Event::Type_E enType = Event::Type_E::ILLEGAL_LANE_CHANGE);

    /**
     * @brief   : 更新违停参数
     * @param    {IllegalLaneChangeDetection_S} &stAlgoCfg：违停
     */
    void setAlgoParamCfg(const Alarm::ParkingDetection_S &stAlgoCfg, Event::Type_E enType = Event::Type_E::PARKING_DETECT);

    /**
     * @brief   : 更新拥堵识别参数
     * @param    {CongestionDetection_S} &stAlgoCfg：拥堵识别
     */
    void setAlgoParamCfg(const Alarm::CongestionDetection_S &stAlgoCfg, Event::Type_E enType = Event::Type_E::CONGESTION);

    /**
     * @brief   : 更新车牌识别参数
     * @param    {HideAlarm_S} &stAlgoCfg：遮挡侦测
     */
    void setAlgoParamCfg(const Alarm::LicensePlateCognitionDetection_S &stAlgoCfg, Event::Type_E enType = Event::Type_E::CONSTRUCTION_OCCUPY_ROAD);

    /**
     * @brief 灵敏度转置信度（反向映射：灵敏度越高，置信度越低）
     * @param sensitivity 输入灵敏度（范围：1~100，超出会被 clamp 到该范围）
     * @param minConfidence 最低置信度（默认 0.1f，float 类型）
     * @param maxConfidence 最高置信度（默认 0.9f，float 类型）
     * @return float 置信度（范围：minConfidence ~ maxConfidence）
     */
    float sensitivityToConfidence(int sensitivity, float minConfidence = 0.1f, float maxConfidence = 0.9f);

    /**
     * @brief 将 1-100 的灵敏度转换为触发报警所需的连续帧数
     * @param sensitivity 灵敏度 (1-100)
     * @param minFrames 最高灵敏度 (100) 对应的帧数
     * @param maxFrames 最低灵敏度 (1) 对应的帧数
     * @return int 触发报警所需的连续帧数
     */
    int sensitivityToFrames(int sensitivity, int minFrames = 1, int maxFrames = 10);

    /**
     * @brief 将 1-100 的灵敏度转换为触发报警所需的检测时长
     * @param sensitivity 灵敏度 (1-100)
     * @param minFrames 最高灵敏度 (100) 对应的时长，单位s
     * @param maxFrames 最低灵敏度 (1) 对应的时长，单位s
     * @return int 触发报警所需的连续帧数
     */
    int sensitivityToDuration(int sensitivity, int minDuration = 0, int maxDuration = 5);

    /**
     * @brief 更新边界检测划线数据
     * @param stRuleInfo
     */
    void changeRuleInfos(const Event::RuleInfo &stRuleInfo, std::vector<int> veDetectionTargets, bool bNeedClear);

    /**
     * @brief 显示出检测区域
     * @param inMat
     * @param nChnId
     */
    void drawRulesToImage(cv::Mat &inMat);

    bool saveImage(const cv::Mat &image, const std::string &strOutputPath);

  private:
    /**
     * @brief 初始化模型组合2
     * @return [*]
     * @note
     */
    bool initGroup2();

    /**
     * @brief 反初始化模型组合2
     * @return [*]
     * @note
     */
    bool unInitGroup2();

    /**
     * @brief 初始化模型组合4
     * @return [*]
     * @note
     */
    bool initGroup4();

    /**
     * @brief 反初始化模型组合4
     * @return [*]
     * @note
     */
    bool unInitGroup4();

    /**
     * @brief 初始化车牌识别模型
     * @return [*]
     * @note
     */
    bool initLicensePlateCognitionDetect();

    /**
     * @brief 反初始化车牌识别模型
     * @return [*]
     * @note
     */
    bool unInitLicensePlateCognitionDetect();

    /**
     * @brief 初始化行人属性检测模型
     * @return [*]
     * @note
     */
    bool initPersonAttribute();

    /**
     * @brief 去初始化行人属性检测模型
     * @return [*]
     * @note
     */
    bool unInitPersonAttribute();

    /**
     * @brief 初始化机动车属性检测模型
     * @return [*]
     * @note
     */
    bool initMotorVehicleAttribute();

    /**
     * @brief 去初始化机动车属性检测模型
     * @return [*]
     * @note
     */
    bool unInitMotorVehicleAttribute();

    /**
     * @brief 初始化非机动车属性检测模型
     * @return [*]
     * @note
     */
    bool initNonMotorizedAttribute();

    /**
     * @brief 去初始化非机动车属性检测模型
     * @return [*]
     * @note
     */
    bool unInitNonMotorizedAttribute();

    /**
     * @brief 线程函数
     * @return [*]
     * @note
     */
    void run();

    /**
     * @brief 人、车、非机动车属性分析
     * @param srcData 图片数据
     * @param vecAllResult 人、车、非机动车识别结果
     * @return [*] 成功：0 失败：其他
     * @note
     */
    int pnmAttributeAnalysis(cv::Mat &srcData, const std::vector<Group2Detect_NS::Result_S> &vecAllResult);

    /**
     * @brief 行人属性分析
     * @param srcData 图片数据
     * @param vecAllResult 行人识别结果
     * @return [*] 成功：0 失败：其他
     * @note
     */
    int personAttributeAnalysis(cv::Mat &srcData, std::vector<Group2Detect_NS::Result_S> vstResult);

    /**
     * @brief 机动车属性分析
     * @param srcData 原图片数据
     * @param cropped 车辆图片数据
     * @param stActualResult 车辆车牌信息
     * @return [*] 成功：0 失败：其他
     * @note
     */
    int motorvehicleAttributeAnalysis(cv::Mat &srcData, cv::Mat &cropped, LicensePlateCognition_NS::Result_S stActualResult);

    /**
     * @brief 非机动车属性分析
     * @param srcData 图片数据
     * @param vecAllResult 非机动车识别结果
     * @return [*] 成功：0 失败：其他
     * @note
     */
    int nonMotorvehicleAttributeAnalysis(cv::Mat &srcData, std::vector<Group2Detect_NS::Result_S> vecNonMotorvehicleResult);

    /**
     * @brief 车牌识别
     * @param srcData 图片数据
     * @param vstResult 车辆识别结果
     * @param bMotorVehicle 是否开启车辆属性识别
     * @return [*] 成功：0 失败：其他
     * @note
     */
    int licensePlateDetectProcess(cv::Mat &srcData, const std::vector<Group2Detect_NS::Result_S> &vstResult, bool bMotorVehicleAttribute);

    /**
     * @brief 模型组合4识别
     * @param srcData 图片数据
     * @param vecPersonResult 行人识别结果
     * @return [*] 成功：0 失败：其他
     * @note
     */
    int group4DetectProcess(cv::Mat &srcData, const std::vector<Group2Detect_NS::Result_S> &vecPersonResult);

    /**
     * @brief 模型组合2识别后处理
     * @param stGroup2OutData 模型组合2识别结果
     * @return [*]
     * @note
     */
    void processGroup2Detect(const Group2Detect_NS::OutData_S &stGroup2OutData);

    /**
     * @brief 模型组合4识别后处理
     * @param stGroup2OutData 模型组合4识别结果
     * @return [*]
     * @note
     */
    void processGroup4Detect(const Group4Detect_NS::OutData_S &stGroup4OutData);

    /**
     * @brief 动态分析函数
     * @param vecAllResult 模型识别结果
     * @param bPersonDetect 是否开启人体检测
     * @param bMotorVehicle 是否开启机动车检测
     * @param bNonMotorVehicle 是否开启非机动车检测
     * @return int
     */
    int dynamicAnalysis(const std::vector<Group2Detect_NS::Result_S> &vecAllResult, bool bPersonDetect, bool bMotorVehicle, bool bNonMotorVehicle);

    /**
     * @brief 动态分析函数
     * @param vecAllResult 模型识别结果
     * @return int
     */
    int dynamicAnalysis(const std::vector<Group4Detect_NS::Result_S> &vecResult);

    /**
     * @brief 车牌动态分析函数
     * @param vecAllResult 模型识别结果
     * @return int
     */
    int dynamicAnalysis(const std::vector<LicensePlateCognition_NS::Result_S> &vecResult);

    /**
     * @brief   : 保存信息至抓图图片数据库
     * @note    : 保存了图片需要更新信息至数据库
     * @param    {string} &strFilename 图片名
     * @param    {Event::Type_E} &eEventType 事件类型
     * @return   {int} 0：成功 非零：失败
     */
    int saveToDatebase(const std::string &strFilename, Event::Type_E eEventType);

    /**
     * @brief 保存全景大图
     * @param image 图片数据
     * @param strPicType 图片类型
     * @return 全景大图路径
     */
    std::string saveFullImage(cv::Mat image, std::string strPicType);

    /**
     * @brief 保存目标小图
     * @param image 图片数据
     * @param stRect 目标坐标
     * @param strPicType 图片类型
     * @return 目标小图路径
     */
    std::string saveCropImage(cv::Mat image, Common::Rect_S stRect, std::string strPicType);

    /**
     * @brief 行人抓拍信息推送
     * @param strCurrentPicture 全景大图路径
     * @param strPersonPicture 目标小图路径
     * @param stResult 属性分析结果
     * @return
     */
    void pushPersonCaptureInfo(const std::string &strCurrentPicture, const std::string &strPersonPicture, const PresonAttribute_NS::Result_S &stResult);

    /**
     * @brief 机动车抓拍信息推送
     * @param strCurrentPicture 全景大图路径
     * @param strPersonPicture 目标小图路径
     * @param strLicensePlateNumber 车牌号码
     * @param stResult 属性分析结果
     * @return
     */
    void pushMotorvehicleCaptureInfo(const std::string &strCurrentPicture, const std::string &strTargetPicture, const std::string &strLicensePlateNumber, const VehicleAttribute_NS::Result_S &stResult);

    /**
     * @brief 非机动车抓拍信息推送
     * @param strCurrentPicture 全景大图路径
     * @param strTargetPicture 目标小图路径
     * @param stResult 属性分析结果
     * @return
     */
    void pushNonMotorvehicleCaptureInfo(const std::string &strCurrentPicture, const std::string &strTargetPicture, const NonMotorizedAttribute_NS::Result_S &stResult);

    /**
     * @brief   : 越界侦测转换区域坐标并判断是否使能算法
     */
    void convertBoundaryAndEnable(Alarm::BoundaryDetection_S &stConfig);

    /**
     * @brief 转换区域坐标并判断是否使能算法
     * @tparam T
     * @param stConfig
     */
    template <typename T>
    void convertAlertLineToZoneAndIsEnable(T &stConfig, Event::Type_E enType);

    /**
     * @brief 转换区域坐标并判断是否使能算法
     * @tparam T
     * @param stConfig
     */
    template <typename T>
    void convertResolutionAndEnable(T &stConfig, Event::Type enType);

    /**
     * @brief 转换区域坐标并判断是否使能算法
     * @tparam T
     * @param stConfig
     */
    template <typename T>
    void convertGuardAreaAndCheckAlgoEnable(T &stAlgoCfg, Event::Type_E enType);

    void convertCrowdGatherAndEnable(Alarm::CrowdGathering_S &stAlgoCfg, Event::Type_E enType);

#ifdef ENABLE_GAT1400_SRC
    void pushImageToGat1400(const cv::Mat &image, const std::vector<Group2Detect_NS::Result_S> &vecResult);
    /* 检查上传状态及类型 */
    int checkPushState(const Group2Detect_NS::OutData_S &stGroup2OutData);
#endif

  private:
    /* 句柄 */
    Group2Detect_NS::CGroup2DetectV1_0 *m_pGroup2Handle = nullptr;

    Group4Detect_NS::CGroup4DetectV1_0 *m_pGroup4Handle = nullptr;

    LicensePlateCognition_NS::CLicensePlateCognitionV1_0 *m_pLicensePlateHandle = nullptr;

    PresonAttribute_NS::CPresonAttributeV2_0 *m_pPersonAttributeHandle = nullptr;

    VehicleAttribute_NS::CVehicleAttributeV2_0 *m_pMotorVehicleAttributeHandle = nullptr;

    NonMotorizedAttribute_NS::CNonMotorizedAttributeV2_0 *m_pNonMotorizedAttributeHandle = nullptr;

    /* 队列 */
    BQ_NS::CBlockingQueue<MediaData_S> m_dateQueue;

    /* 用于控制线程的运行 */
    std::atomic<bool> m_bRunning;
    /* 互斥锁保护指针访问 */
    std::mutex m_mutex;
    /* 条件变量 */
    std::condition_variable m_condition;
    /* 数据获取线程 */
    std::thread m_thread;

    /* 是否开启人脸属性分析 */
    std::atomic<bool> m_bPedestrianAttribute;
    /* 是否开启机动车属性分析 */
    std::atomic<bool> m_bMotorVehicleAttribute;
    /* 是否开启非机动车属性分析 */
    std::atomic<bool> m_bNonMotorVehicleAttribute;

    /* 规则区域 */
    std::vector<PMNMDetectRuleInfo_S> m_vstRuleInfo;
    /* 检测频率控制 */
    EventManager m_RecvManager{350};
    // /* 事件管理器, 过滤重复事件 */
    // EventManager m_EventManager{20 * 1000};
    /* 框的置信度阈值 */
    float m_fBoxThreshold = 0.25;

    /* 越界侦测配置 */
    Alarm::BoundaryDetection_S m_stAlgoCrossCfg;
    /* 区域入侵配置 */
    Alarm::FieldDetection_S m_stAlgoIntruCfg;
    /* 进入区域配置 */
    Alarm::EntranceDetection_S m_stAlgoEntryCfg;
    /* 离开区域配置 */
    Alarm::ExitingDetection_S m_stAlgoExitCfg;

    /* 徘徊侦测相关配置 */
    Alarm::LoiteringDetection_S m_stLoiteringCfg;
    /* 翻越围栏配置 */
    Alarm::FenceClimbingDetection_S m_stFenceClimbingCfg;
    /* 离岗配置 */
    Alarm::LeavePostDetection_S m_stLeavePostCfg;
    /* 行人闯入配置 */
    Alarm::PedestrianIntrusionDetection_S m_stPedestrianIntrusionCfg;
    /* 人员聚集配置 */
    Alarm::CrowdGathering_S m_stCrowdGatheringDetCfg;
    /* 人员倒地配置 */
    Alarm::PersonFallDownDetection_S m_stPersonFallDownCfg;

    /* 应急车道占用配置 */
    Alarm::EmergencyLaneOccupancyDetection_S m_stAlgoEmergencyLaneOccupancyCfg;
    /* 非机动车闯入配置 */
    Alarm::NonMotorVehicleIntrusionDetection_S m_stAlgoNonMotorVehicleIntrusionCfg;
    /* 电瓶车识别相关配置 */
    Alarm::ElectricScooterDetection_S m_stAlgoElectricScooterCfg;

    /* 逆行侦测配置 */
    Alarm::DrivingAgainstTrafficDetection_S m_stDrivingAgainstTrafficDetectionCfg;
    /* 拥堵配置 */
    Alarm::CongestionDetection_S m_stAlgoCongestionDetectionCfg;
    /* 违规停车配置 */
    Alarm::ParkingDetection_S m_stAlgoParkingDetectionCfg;
    /* 违规变道配置 */
    Alarm::IllegalLaneChangeDetection_S m_stAlgoIllegalLaneChangeDetectionCfg;

    /* 抽烟识别相关配置 */
    Alarm::SmokingDection_S m_stAlgoSmokingCfg;
    /* 睡岗识别相关配置 */
    Alarm::SleepOnDutyDetection_S m_stAlgoSleepOnDutyCfg;
    /* 玩手机识别识别相关配置 */
    Alarm::PhoneUsageDetection_S m_stPhoneUsageCfg;
    /* 摔倒识别相关配置 */
    Alarm::TripDetection_S m_stAlgoTripCfg;

    /* 车牌识别配置 */
    Alarm::LicensePlateCognitionDetection_S m_stAlgoLicensePlateCognitionCfg;

    /* 规则配置 */
    std::vector<PMNMDetectRuleInfo_S> m_vstCrossRule;
    std::vector<PMNMDetectRuleInfo_S> m_vstIntruRule;
    std::vector<PMNMDetectRuleInfo_S> m_vstEntryRule;
    std::vector<PMNMDetectRuleInfo_S> m_vstExitRule;

    std::vector<Event::RuleInfo> m_vstLoiteringRule;
    std::vector<Event::RuleInfo> m_vstFenceClimbingRule;
    std::vector<Event::RuleInfo> m_vstLeavePostRule;
    std::vector<Event::RuleInfo> m_vstPedestrianIntrusionRule;
    std::vector<Event::RuleInfo> m_vstCrowdGatheringDetRule;

    std::vector<PMNMDetectRuleInfo_S> m_vstEmergencyLaneOccupancyRule;
    std::vector<PMNMDetectRuleInfo_S> m_vstNonMotorVehicleIntrusionRule;

    std::vector<TrafficRuleInfo_S> m_vstDrivingAgainstTrafficRule; /* 逆行规则 */
    std::vector<TrafficRuleInfo_S> m_vstIllegalLaneChangeRule;     /* 违规变道规则 */
    std::vector<Event::RuleInfo>   m_vstIllegalParkingRule;        /* 违规停车规则 */

    /* 报警状态管理 */
    CAlarmStateMachine m_CrossAlarmStateMachine;
    CAlarmStateMachine m_IntruAlarmStateMachine;
    CAlarmStateMachine m_EntryAlarmStateMachine;
    CAlarmStateMachine m_ExitAlarmStateMachine;

    CAlarmStateMachine m_LoiteringAlarmStateMachine;
    CAlarmStateMachine m_FenceClimbingAlarmStateMachine;
    CAlarmStateMachine m_LeavePostAlarmStateMachine;
    CAlarmStateMachine m_PedestrianIntrusionAlarmStateMachine;
    CAlarmStateMachine m_CrowdGatheringAlarmStateMachine;
    CAlarmStateMachine m_PersonFallDownStateMachine;

    CAlarmStateMachine m_ElectricScooterStateMachine; /* 电瓶车进电梯 */

    CAlarmStateMachine m_EmergencyLaneOccupancyAlarmStateMachine;
    CAlarmStateMachine m_NonMotorVehicleIntrusionAlarmStateMachine;

    CAlarmStateMachine m_DrivingAgainstTrafficStateMachine; /* 逆行 */
    CAlarmStateMachine m_IllegalLaneChangeStateMachine;     /* 违规变道 */
    CAlarmStateMachine m_IllegalParkingStateMachine;        /* 违规停车 */
    CAlarmStateMachine m_CongestionStateMachine;            /* 拥堵 */

    CAlarmStateMachine m_SmokingStateMachine;     /* 抽烟 */
    CAlarmStateMachine m_SleepOnDutyStateMachine; /* 睡觉 */
    CAlarmStateMachine m_PhoneUsageStateMachine;  /* 玩手机 */
    CAlarmStateMachine m_TripStateMachine;        /* 跌倒 */

    /* 报警状态管理 */
    CAlarmStateMachine m_LicensePlateStateMachine;

#ifdef ENABLE_GAT1400_SRC
    /* 上传gat1400平台状态管理 */
    CUploadStateMachine m_CrossUploadStateMachine;                    /* 越界侦测 */
    CUploadStateMachine m_IntruUploadStateMachine;                    /* 区域入侵 */
    CUploadStateMachine m_EntryUploadStateMachine;                    /* 进入区域 */
    CUploadStateMachine m_ExitUploadStateMachine;                     /* 离开区域 */
    CUploadStateMachine m_LoiteringUploadStateMachine;                /* 徘徊侦测 */
    CUploadStateMachine m_FenceClimbingUploadStateMachine;            /* 翻越围栏 */
    CUploadStateMachine m_LeavePostUploadStateMachine;                /* 离岗 */
    CUploadStateMachine m_PedestrianIntrusionUploadStateMachine;      /* 行人闯入 */
    CUploadStateMachine m_CrowdGatheringUploadStateMachine;           /* 人员聚集 */
    CUploadStateMachine m_ElectricScooterUploadStateMachine;          /* 电瓶车进电梯 */
    CUploadStateMachine m_EmergencyLaneOccupancyUploadStateMachine;   /* 应急车道占用 */
    CUploadStateMachine m_NonMotorVehicleIntrusionUploadStateMachine; /* 非机动车闯入 */
    CUploadStateMachine m_DrivingAgainstTrafficUploadStateMachine;    /* 逆行 */
    CUploadStateMachine m_IllegalLaneChangeUploadStateMachine;        /* 违规变道 */
    CUploadStateMachine m_IllegalParkingUploadStateMachine;           /* 违规停车 */
    CUploadStateMachine m_CongestionUploadStateMachine;               /* 拥堵 */
#endif

    /* 送到AI模块的帧的宽高 */
    int m_nAiChnWith = PIXEL_WIDTH_1280;
    int m_nAiChnHeigh = PIXEL_HEIGHT_720;

    /* group2、group4 算法默认分辨率 */
    int m_nWidth  = 640;
    int m_nHeight = 384;

    /* 车牌检测算法默认分辨率 */
    int m_nLicensePlateWidth  = 640;
    int m_nLicensePlateHeight = 640;

    /* 行人、非机动车属性识别 算法默认分辨率 */
    int m_nPersonAndNonMotorAttributeWidth  = 192;
    int m_nPersonAndNonMotorAttributeHeight = 256;

    /* 机动车属性识别 算法默认分辨率 */
    int m_nMotorVehicleAttributeWidth  = 192;
    int m_nMotorVehicleAttributeHeight = 256;
    
    /* 属性分析累计间隔了多少帧 */
    int m_nFrameCount = 0;
    /* 记录上一帧的行人目标框信息 */
    std::vector<Group2Detect_NS::Result_S> m_vecLastFramePersonResult;
    /* 记录上一帧的机动车目标框车牌号信息 */
    std::vector<std::string> m_vecLastFrameMotorvehicleResult;
    /* 记录上一帧的非机动车目标框信息 */
    std::vector<Group2Detect_NS::Result_S> m_vecLastFrameNonMotorvehicleResult;
};