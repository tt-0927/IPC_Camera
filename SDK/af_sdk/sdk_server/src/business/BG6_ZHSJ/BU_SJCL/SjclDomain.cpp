/*
 * @FilePath     : sdk_new/sdk_server/src/business/BG6_ZHSJ/BU_SJCL/SjclDomain.cpp
 * @Author       : chenchl
 * @Date         : 2026-08-20
 * @LastEditors  : chenchl
 * @LastEditTime : 2026-08-20
 * @Description  : SJCL 配置域实现
 *                 注册 NVR/录播命令码→处理函数映射。
 */
#ifndef BU_SJCL_EXCLUDE

#include "SjclDomain.h"
#include "NvrBusiness.h"

#include <cmath>
#include <limits>
#include <memory>

/**
 * 功能：校验 JSON 数值为指定范围的整数，拒绝字符串、小数和溢出值。
 * param [in] pValue：待校验的 JSON 节点。
 * param [in] nMin：最小合法值。
 * param [in] nMax：最大合法值。
 * param [out]：无。
 * return：合法返回 true，否则返回 false。
 */
static bool sjcl_is_alarm_integer(const Json::Object* pValue, int nMin, int nMax)
{
    return cJSON_IsNumber(pValue) && std::isfinite(pValue->valuedouble) &&
           pValue->valuedouble >= nMin && pValue->valuedouble <= nMax &&
           std::floor(pValue->valuedouble) == pValue->valuedouble;
}

/**
 * 功能：按协议字段名精确查找节点，避免错误大小写被默认为空配置。
 * param [in] pObject：JSON 对象。
 * param [in] pKey：协议字段名。
 * param [out]：无。
 * return：字段节点；不存在时返回空指针，所有权仍属于原对象。
 */
static const Json::Object* sjcl_alarm_field(const Json::Object* pObject, const char* pKey)
{
    return cJSON_GetObjectItemCaseSensitive(pObject, pKey);
}

/**
 * 功能：校验字符串字段存在且可完整存入协议缓冲区，允许显式空字符串。
 * param [in] pObject：JSON 对象。
 * param [in] pKey：协议字段名。
 * param [in] nCapacity：包含字符串结束符的缓冲区容量。
 * param [out]：无。
 * return：合法返回 true，否则返回 false。
 */
static bool sjcl_is_alarm_string(const Json::Object* pObject, const char* pKey, size_t nCapacity)
{
    const Json::Object* pValue = sjcl_alarm_field(pObject, pKey);
    return cJSON_IsString(pValue) && pValue->valuestring &&
           std::strlen(pValue->valuestring) < nCapacity;
}

/**
 * 功能：校验七天布防时间的字段类型、数量与时间段内容，允许七天全部为空。
 * param [in] pSchedule：布防时间 JSON 对象。
 * param [out]：无。
 * return：合法返回 true，否则返回 false。
 */
static bool sjcl_is_alarm_schedule(const Json::Object* pSchedule)
{
    const Json::Object* pCounts = sjcl_alarm_field(pSchedule, "TimeSectionCount");
    const Json::Object* pSections = sjcl_alarm_field(pSchedule, "TimeSections");
    if (!cJSON_IsObject(pSchedule) || !cJSON_IsObject(pCounts) || !cJSON_IsObject(pSections) ||
        cJSON_GetArraySize(pCounts) != NET_ALARM_SCHEDULE_DAY_COUNT)
    {
        return false;
    }
    int nPresentDays = 0;
    for (int nDay = 0; nDay < NET_ALARM_SCHEDULE_DAY_COUNT; ++nDay)
    {
        const std::string strDay = std::to_string(nDay);
        const Json::Object* pCount = sjcl_alarm_field(pCounts, strDay.c_str());
        if (!sjcl_is_alarm_integer(pCount, 0, NET_PLAN_SECTION_NUM))
        {
            return false;
        }
        const int nCount = static_cast<int>(pCount->valuedouble);
        const Json::Object* pDay = sjcl_alarm_field(pSections, strDay.c_str());
        /* 数量为零时可省略当天对象，但不能携带未声明的时间段。 */
        if (!pDay && nCount == 0)
        {
            continue;
        }
        if (!cJSON_IsObject(pDay) || cJSON_GetArraySize(pDay) != nCount)
        {
            return false;
        }
        ++nPresentDays;
        for (int nSection = 0; nSection < nCount; ++nSection)
        {
            const std::string strSection = std::to_string(nSection);
            const Json::Object* pTime = sjcl_alarm_field(pDay, strSection.c_str());
            if (!cJSON_IsObject(pTime) ||
                !sjcl_is_alarm_integer(sjcl_alarm_field(pTime, "StartHour"), NET_ALARM_SCHEDULE_HOUR_MIN, NET_ALARM_SCHEDULE_HOUR_MAX) ||
                !sjcl_is_alarm_integer(sjcl_alarm_field(pTime, "EndHour"), NET_ALARM_SCHEDULE_HOUR_MIN, NET_ALARM_SCHEDULE_HOUR_MAX) ||
                !sjcl_is_alarm_integer(sjcl_alarm_field(pTime, "StartMinute"), NET_ALARM_SCHEDULE_MINUTE_MIN, NET_ALARM_SCHEDULE_MINUTE_MAX) ||
                !sjcl_is_alarm_integer(sjcl_alarm_field(pTime, "EndMinute"), NET_ALARM_SCHEDULE_MINUTE_MIN, NET_ALARM_SCHEDULE_MINUTE_MAX))
            {
                return false;
            }
        }
    }
    return cJSON_GetArraySize(pSections) == nPresentDays;
}

/**
 * 功能：校验数量与通道数组长度一致，拒绝非法元素及截断数组。
 * param [in] pObject：联动 JSON 对象。
 * param [in] pCountKey：数量字段名。
 * param [in] pArrayKey：数组字段名。
 * param [in] nCapacity：数组最大容量及通道编号上限。
 * param [out]：无。
 * return：合法返回 true，否则返回 false。
 */
static bool sjcl_is_alarm_channels(const Json::Object* pObject, const char* pCountKey,
                                   const char* pArrayKey, int nCapacity)
{
    const Json::Object* pCount = sjcl_alarm_field(pObject, pCountKey);
    const Json::Object* pArray = sjcl_alarm_field(pObject, pArrayKey);
    if (!sjcl_is_alarm_integer(pCount, 0, nCapacity) || !cJSON_IsArray(pArray) ||
        cJSON_GetArraySize(pArray) != static_cast<int>(pCount->valuedouble))
    {
        return false;
    }
    for (const Json::Object* pItem = pArray->child; pItem; pItem = pItem->next)
    {
        if (!sjcl_is_alarm_integer(pItem, 0, nCapacity - 1))
        {
            return false;
        }
    }
    return true;
}

/**
 * 功能：校验单路报警设置必需字段，拒绝 GET 列表包装和缺失字段导致的默认覆盖。
 * param [in] pRoot：请求根对象。
 * param [in] nCommand：497 报警输入或 499 报警输出命令。
 * param [out]：无。
 * return：合法返回 true，否则返回 false。
 */
static bool sjcl_is_alarm_io_request(const Json::Object* pRoot, INT32 nCommand)
{
    const bool bInput = nCommand == NET_SET_ALARM_INPUT_INFO;
    const int nCapacity = bInput ? NET_MAX_ALARM_IN_NUM : NET_MAX_ALARM_OUT_NUM;
    if ((nCommand != NET_SET_ALARM_INPUT_INFO && nCommand != NET_SET_ALARM_OUTPUT_INFO) ||
        !cJSON_IsObject(pRoot) || sjcl_alarm_field(pRoot, "AlarmInputs") ||
        sjcl_alarm_field(pRoot, "AlarmOutputs") || sjcl_alarm_field(pRoot, "AlarmInputCount") ||
        sjcl_alarm_field(pRoot, "AlarmOutputCount") ||
        !sjcl_is_alarm_integer(sjcl_alarm_field(pRoot, "AlarmNumber"), 0, nCapacity - 1) ||
        !sjcl_is_alarm_string(pRoot, "AlarmAddress", NET_ALARM_ADDRESS_LEN) ||
        !sjcl_is_alarm_string(pRoot, "AlarmName", NET_ALARM_NAME_LEN) ||
        !sjcl_is_alarm_schedule(sjcl_alarm_field(pRoot, "AlarmSchedule")))
    {
        return false;
    }
    const Json::Object* pCopyTo = sjcl_alarm_field(pRoot, "CopyTo");
    if (pCopyTo)
    {
        if (!cJSON_IsArray(pCopyTo) || cJSON_GetArraySize(pCopyTo) > NET_ALARM_COPY_TO_MAX_NUM)
        {
            return false;
        }
        for (const Json::Object* pItem = pCopyTo->child; pItem; pItem = pItem->next)
        {
            if (!sjcl_is_alarm_integer(pItem, 0, nCapacity - 1))
            {
                return false;
            }
        }
    }
    if (bInput)
    {
        const Json::Object* pLinkage = sjcl_alarm_field(pRoot, "LinkageList");
        return sjcl_is_alarm_integer(sjcl_alarm_field(pRoot, "NormallyOpen"), FALSE, TRUE) &&
               sjcl_is_alarm_integer(sjcl_alarm_field(pRoot, "DealType"), NET_ALARM_INPUT_DEAL_TYPE_DISABLED, NET_ALARM_INPUT_DEAL_TYPE_ENABLED) &&
               cJSON_IsObject(pLinkage) &&
               sjcl_is_alarm_channels(pLinkage, "AlarmOutputCount", "AlarmOutput", NET_MAX_ALARM_OUT_NUM) &&
               sjcl_is_alarm_channels(pLinkage, "RecordChannelCount", "RecordChannel", NET_CHANNEL_MAX) &&
               sjcl_is_alarm_channels(pLinkage, "SnapshotChannelCount", "SnapshotChannel", NET_CHANNEL_MAX);
    }
    return sjcl_is_alarm_integer(sjcl_alarm_field(pRoot, "DelayTime"), 0, (std::numeric_limits<INT32>::max)()) &&
           sjcl_is_alarm_integer(sjcl_alarm_field(pRoot, "State"), NET_ALARM_OUTPUT_STATE_OFF, NET_ALARM_OUTPUT_STATE_HUMAN_ON);
}

/**
 * 功能：校验并设置一路报警配置，任何 JSON 校验失败均不调用 IPC 回调。
 * param [in] nChannelId：设备通道标识。
 * param [in] nCommand：报警输入或输出设置命令。
 * param [in] strRequest：单路配置 JSON 请求体。
 * param [in] strUrlParam：URL 参数，当前不参与配置解析。
 * param [out]：无。
 * return：包含实际设置结果或无效参数错误的响应 JSON。
 */
template<typename TConfig>
static std::string sjcl_set_alarm_io(INT32 nChannelId, INT32 nCommand,
                                     const std::string& strRequest, const std::string& strUrlParam)
{
    (void)strUrlParam;
    /* JSON 对象由当前请求独占，自动释放；不持锁执行设备回调。 */
    std::unique_ptr<Json::Object, decltype(&cJSON_Delete)> pRoot(Json::init(strRequest), &cJSON_Delete);
    if (!sjcl_is_alarm_io_request(pRoot.get(), nCommand))
    {
        NETSDK_LOG_MESSAGE_WARN("报警设置 JSON 无效，要求完整单路配置: cmd=%d", nCommand);
        return SDKConvert::to_respString(NET_E_INVALID_PARAM, nCommand);
    }
    TConfig stConfig{};
    SDKConvert::deal(pRoot.get(), stConfig, true);
    const NET_COMMON_ECODE_E enResult = static_cast<NET_COMMON_ECODE_E>(
        executeSetDevConfigCb(nChannelId, nCommand, &stConfig));
    return SDKConvert::to_respString(enResult, nCommand);
}

CSjclDomain::CSjclDomain()
{
    /* ==================================================================
     * Get 命令注册
     * ================================================================== */

    /* ===== 录像参数 ===== */
    m_getTable[NET_GET_RECORD_STATUS]        = &CSjclDomain::TemplatedGet<NET_RecordStatusInfo_S>;
    m_getTable[NET_GET_RECORD_SCHEDULE]      = &CSjclDomain::TemplatedGet<NET_RecordSchedule_S>;
    m_getTable[NET_GET_RECORD_ADVANCED_PARAM] = &CSjclDomain::TemplatedGet<NET_RecordAdvancedParam_S>;

    /* ===== 抓拍 ===== */
    m_getTable[NET_GET_CAPTURE_PLAN_INFO]   = &CSjclDomain::TemplatedGet<NET_CapturePlanInfo_S>;
    m_getTable[NET_GET_CAPTURE_PARAM_INFO] = &CSjclDomain::TemplatedGet<NET_CaptureParamInfo_S>;

    /* ===== 传统报警 ===== */
    m_getTable[NET_GET_TAMPERALARM]            = &CSjclDomain::TemplatedGet<NET_TamperAlarmInfo_S>;
    m_getTable[NET_GET_MOTIONALARM]             = &CSjclDomain::TemplatedGet<NET_MotionAlarmInfo_S>;
    m_getTable[NET_GET_CROSSLINEALARM]          = &CSjclDomain::TemplatedGet<NET_CrossLineAlarmInfo_S>;
    m_getTable[NET_GET_INTRUSIONALARM]           = &CSjclDomain::TemplatedGet<NET_IntrusionAlarmInfo_S>;
    m_getTable[NET_GET_ENTERREGIONALARM]        = &CSjclDomain::TemplatedGet<NET_EnterRegionAlarmInfo_S>;
    m_getTable[NET_GET_LEAVEREGIONALARM]         = &CSjclDomain::TemplatedGet<NET_LeaveRegionAlarmInfo_S>;
    m_getTable[NET_GET_LOITERINGALARM]           = &CSjclDomain::TemplatedGet<NET_LoiteringAlarmInfo_S>;
    m_getTable[NET_GET_SCENECHANGEALARM]         = &CSjclDomain::TemplatedGet<NET_SceneChangeAlarmInfo_S>;
    m_getTable[NET_GET_CROWDGATHERINGALARM]      = &CSjclDomain::TemplatedGet<NET_CrowdGatheringAlarmInfo_S>;
    m_getTable[NET_GET_GARBAGE_EXPOSURE_CFG]     = &CSjclDomain::TemplatedGet<NET_GarbageExposureCfg_S>;
    m_getTable[NET_GET_GARBAGE_OVERFLOW_CFG]     = &CSjclDomain::TemplatedGet<NET_GarbageOverflowCfg_S>;
    m_getTable[NET_GET_PARKINGALARM]             = &CSjclDomain::TemplatedGet<NET_ParkingAlarmInfo_S>;
    m_getTable[NET_GET_UNATTENDEDOBJECTALARM]    = &CSjclDomain::TemplatedGet<NET_UnattendedObjectAlarmInfo_S>;
    m_getTable[NET_GET_OBJECTREMOVALALARM]       = &CSjclDomain::TemplatedGet<NET_ObjectRemovalAlarmInfo_S>;
    m_getTable[NET_GET_AUDIOANOMALYALARM]        = &CSjclDomain::TemplatedGet<NET_AudioAnomalyAlarmInfo_S>;
    m_getTable[NET_GET_AUDIO_ANOMALY_CURRENT_DB] = &CSjclDomain::TemplatedGet<NET_AudioAnomalyCurrentDb_S>;
    m_getTable[NET_GET_AUDIBLE_ALARM_INFO]      = &CSjclDomain::TemplatedGet<NET_AudibleAlarmInfo_S>;
    m_setTable[NET_SET_ALARM_INPUT_INFO]         = &sjcl_set_alarm_io<NET_AlarmInputInfo_S>;
    m_setTable[NET_SET_ALARM_OUTPUT_INFO]        = &sjcl_set_alarm_io<NET_AlarmOutputInfo_S>;
    m_getTable[NET_GET_ALARM_INPUT_INFO]         = &CSjclDomain::TemplatedGet<NET_AlarmInputInfoList_S>;
    m_getTable[NET_GET_ALARM_OUTPUT_INFO]        = &CSjclDomain::TemplatedGet<NET_AlarmOutputInfoList_S>;
    m_getTable[NET_GET_FLASHING_LIGHT_ALARM_INFO]= &CSjclDomain::TemplatedGet<NET_FlashingLightAlarmInfo_S>;
    m_getTable[NET_GET_PIR_ALARM_INFO]          = &CSjclDomain::TemplatedGet<NET_PirAlarmInfo_S>;

    
    /* ===== 预览/对讲 ===== */
    m_getTable[NET_FROM_STREAM_TALKBACK]      = &CSjclDomain::TemplatedGet<NET_TalkbackStreamInfo_S>;
    m_getTable[NET_GET_VOICECOM_AUDIO_CFG]   = &CSjclDomain::TemplatedGet<NET_VoiceComAudioCfg_S>;

    /* ===== 人脸库/人脸抓拍 ===== */
    m_getTable[NET_GET_TARGET_LIB]        = &CSjclDomain::TemplatedGet<NET_FaceLibList_S>;
    m_getTable[NET_GET_FACECAPTUREINFO]   = &CSjclDomain::TemplatedGet<NET_FaceCaptureInfo_S>;
    m_getTable[NET_GET_FACE_INFO]         = &CSjclDomain::TemplatedGet<NET_FaceInfoList_S>;

    /* ===== AI 分析配置 ===== */
    m_getTable[NET_GET_PEOPLE_FLOW_STATISTICS_CFG]        = &CSjclDomain::TemplatedGet<NET_PeopleFlowStatisticsCfg_S>;
    m_getTable[NET_GET_PEOPLE_DENSITY_DETECTION_CFG]      = &CSjclDomain::TemplatedGet<NET_PeopleDensityDetectionCfg_S>;
    m_getTable[NET_GET_MANHOLE_COVER_ABNORMAL_CFG]        = &CSjclDomain::TemplatedGet<NET_ManholeCoverAbnormalCfg_S>;
    m_getTable[NET_GET_SLEEP_ON_DUTY_CFG]                  = &CSjclDomain::TemplatedGet<NET_SleepOnDutyCfg_S>;
    m_getTable[NET_GET_ELECTRIC_VEHICLE_IN_ELEVATOR_CFG]  = &CSjclDomain::TemplatedGet<NET_ElectricVehicleInElevatorCfg_S>;
    m_getTable[NET_GET_PERSON_FALL_DOWN_CFG]              = &CSjclDomain::TemplatedGet<NET_PersonFallDownCfg_S>;
    m_getTable[NET_GET_CONSTRUCTION_OCCUPY_ROAD_CFG]      = &CSjclDomain::TemplatedGet<NET_ConstructionOccupyRoadCfg_S>;
    m_getTable[NET_GET_CONGESTION_CFG]                    = &CSjclDomain::TemplatedGet<NET_CongestionCfg_S>;
    m_getTable[NET_GET_LICENSE_PLATE_RECOGNITION_CFG]    = &CSjclDomain::TemplatedGet<NET_LicensePlateRecognitionCfg_S>;
    m_getTable[NET_GET_HIGH_ALTITUDE_SEATBELT_CFG]       = &CSjclDomain::TemplatedGet<NET_HighAltitudeSeatbeltCfg_S>;
    m_getTable[NET_GET_SAFETY_HELMET_CFG]                 = &CSjclDomain::TemplatedGet<NET_SafetyHelmetCfg_S>;
    m_getTable[NET_GET_PERSON_FALL_CFG]                   = &CSjclDomain::TemplatedGet<NET_PersonFallCfg_S>;
    m_getTable[NET_GET_PHONE_USAGE_CFG]                   = &CSjclDomain::TemplatedGet<NET_PhoneUsageCfg_S>;
    m_getTable[NET_GET_SMOKING_CFG]                       = &CSjclDomain::TemplatedGet<NET_SmokingCfg_S>;
    m_getTable[NET_GET_OPEN_FLAME_CFG]                    = &CSjclDomain::TemplatedGet<NET_OpenFlameCfg_S>;
    m_getTable[NET_GET_BARE_SOIL_CFG]                     = &CSjclDomain::TemplatedGet<NET_BareSoilCfg_S>;
    m_getTable[NET_GET_HOLE_PROTECTION_BAR_CFG]           = &CSjclDomain::TemplatedGet<NET_HoleProtectionBarCfg_S>;
    m_getTable[NET_GET_REFLECTIVE_CLOTHING_CFG]           = &CSjclDomain::TemplatedGet<NET_ReflectiveClothingCfg_S>;
    m_getTable[NET_GET_PET_RECOGNITION_INFO]              = &CSjclDomain::TemplatedGet<NET_PetRecognitionInfo_S>;
    m_getTable[NET_GET_CLIMB_FENCE_INFO]                  = &CSjclDomain::TemplatedGet<NET_ClimbFenceInfo_S>;
    m_getTable[NET_GET_DIMISSION_INFO]                     = &CSjclDomain::TemplatedGet<NET_DimissionInfo_S>;
    m_getTable[NET_GET_ILLEGAL_LANE_INFO]                  = &CSjclDomain::TemplatedGet<NET_IllegalLaneInfo_S>;
    m_getTable[NET_GET_RETROGRADE_INFO]                    = &CSjclDomain::TemplatedGet<NET_RetrogradeInfo_S>;
    m_getTable[NET_GET_NONMOTOR_VEHICLE_INTRUSION_INFO]   = &CSjclDomain::TemplatedGet<NET_NonmotorVehicleIntrusionInfo_S>;
    m_getTable[NET_GET_OCCUPATION_EMERGENCY_INFO]         = &CSjclDomain::TemplatedGet<NET_OccupationEmergencyInfo_S>;
    m_getTable[NET_GET_PEDESTRIAN_INTRUSION_INFO]         = &CSjclDomain::TemplatedGet<NET_PedestrianIntrusionInfo_S>;
    m_getTable[NET_GET_SMOKE_FIRE_CFG]                    = &CSjclDomain::TemplatedGet<NET_SmokeFireCfg_S>;
    m_getTable[NET_GET_ROAD_PONDING_CFG]                  = &CSjclDomain::TemplatedGet<NET_RoadPondingCfg_S>;

    /* ===== 设备状态/串口参数 ===== */
    m_getTable[NET_GET_DEVICE_STATUS]     = &CSjclDomain::TemplatedGet<NET_DeviceStatusInfo_S>;
    m_getTable[NET_GET_CHANNEL_NAME]      = &CSjclDomain::TemplatedGet<NET_ChannelNameInfo_S>;
    m_getTable[NET_GET_SERIAL_PORT_PARAM] = &CSjclDomain::TemplatedGet<NET_SerialPortParam_S>;

    /* ===== NVR 专属（委托 CNvrBusiness） ===== */
    m_getTable[NET_FIND_RECORD_FILE_INFO] = [](INT32 ch, INT32 cmd, const std::string&, const std::string& url) -> std::string {
        return CNvrBusiness::instance()->HandleGetRecordFileList(ch, cmd, url);
    };
    m_getTable[NET_GET_CHANNEL_INFO] = [](INT32 ch, INT32 cmd, const std::string&, const std::string&) -> std::string {
        return CNvrBusiness::instance()->HandleGetChannelInfo(ch, cmd);
    };
    m_getTable[NET_GET_RTSPURLCFG] = [](INT32 ch, INT32 cmd, const std::string& req_data, const std::string&) -> std::string {
        return CNvrBusiness::instance()->HandleGetRtspUrl(ch, cmd, req_data);
    };

    /* ===== 抓图 / 录像锁定 / 强制I帧（580~584，通用 Get 通道） ===== */
    m_getTable[NET_GET_CAPTURE_PICTURE]     = &CSjclDomain::TemplatedGet<NET_CapturePictureInfo_S>;
    m_getTable[NET_GET_RECORD_LOCK_STATUS]  = &CSjclDomain::TemplatedGet<NET_RecordLockInfo_S>;
    m_getTable[NET_FORCE_KEY_FRAME]         = &CSjclDomain::TemplatedGet<NET_ForceKeyFrameInfo_S>;

    /* ==================================================================
     * Set 命令注册
     * ================================================================== */

    /* ===== 录像控制/计划/高级参数/下载 ===== */
    m_setTable[NET_CONTROL_RECORD_INFO]       = &CSjclDomain::TemplatedSet<NET_RecordInfo_S>;
    m_setTable[NET_SET_RECORD_SCHEDULE]       = &CSjclDomain::TemplatedSet<NET_RecordSchedule_S>;
    m_setTable[NET_SET_RECORD_ADVANCED_PARAM] = &CSjclDomain::TemplatedSet<NET_RecordAdvancedParam_S>;
    m_setTable[NET_DOWNLOAD_RECORD_FILE]      = &CSjclDomain::TemplatedSet<NET_RecordDownloadList_S>;

    /* ===== 录像锁定/解锁（581/582，通用 Set 通道） ===== */
    m_setTable[NET_LOCK_RECORD_FILE]           = &CSjclDomain::TemplatedSet<NET_RecordLockInfo_S>;
    m_setTable[NET_UNLOCK_RECORD_FILE]         = &CSjclDomain::TemplatedSet<NET_RecordLockInfo_S>;

    /* ===== 传统报警 ===== */
    m_setTable[NET_SET_TAMPERALARM]              = &CSjclDomain::TemplatedSet<NET_TamperAlarmInfo_S>;
    m_setTable[NET_SET_MOTIONALARM]              = &CSjclDomain::TemplatedSet<NET_MotionAlarmInfo_S>;
    m_setTable[NET_SET_CROSSLINEALARM]           = &CSjclDomain::TemplatedSet<NET_CrossLineAlarmInfo_S>;
    m_setTable[NET_SET_INTRUSIONALARM]           = &CSjclDomain::TemplatedSet<NET_IntrusionAlarmInfo_S>;
    m_setTable[NET_SET_ENTERREGIONALARM]         = &CSjclDomain::TemplatedSet<NET_EnterRegionAlarmInfo_S>;
    m_setTable[NET_SET_LEAVEREGIONALARM]         = &CSjclDomain::TemplatedSet<NET_LeaveRegionAlarmInfo_S>;
    m_setTable[NET_SET_LOITERINGALARM]           = &CSjclDomain::TemplatedSet<NET_LoiteringAlarmInfo_S>;
    m_setTable[NET_SET_SCENECHANGEALARM]         = &CSjclDomain::TemplatedSet<NET_SceneChangeAlarmInfo_S>;
    m_setTable[NET_SET_CROWDGATHERINGALARM]      = &CSjclDomain::TemplatedSet<NET_CrowdGatheringAlarmInfo_S>;
    m_setTable[NET_SET_GARBAGE_EXPOSURE_CFG]     = &CSjclDomain::TemplatedSet<NET_GarbageExposureCfg_S>;
    m_setTable[NET_SET_GARBAGE_OVERFLOW_CFG]     = &CSjclDomain::TemplatedSet<NET_GarbageOverflowCfg_S>;
    m_setTable[NET_SET_PARKINGALARM]             = &CSjclDomain::TemplatedSet<NET_ParkingAlarmInfo_S>;
    m_setTable[NET_SET_UNATTENDEDOBJECTALARM]    = &CSjclDomain::TemplatedSet<NET_UnattendedObjectAlarmInfo_S>;
    m_setTable[NET_SET_OBJECTREMOVALALARM]       = &CSjclDomain::TemplatedSet<NET_ObjectRemovalAlarmInfo_S>;
    m_setTable[NET_SET_AUDIOANOMALYALARM]        = &CSjclDomain::TemplatedSet<NET_AudioAnomalyAlarmInfo_S>;
    m_setTable[NET_SET_AUDIBLE_ALARM_INFO]       = &CSjclDomain::TemplatedSet<NET_AudibleAlarmInfo_S>;
    m_setTable[NET_SET_ALARM_INPUT_INFO]         = &CSjclDomain::TemplatedSet<NET_AlarmInputInfo_S>;
    m_setTable[NET_SET_ALARM_OUTPUT_INFO]        = &CSjclDomain::TemplatedSet<NET_AlarmOutputInfo_S>;
    m_setTable[NET_SET_FLASHING_LIGHT_ALARM_INFO]= &CSjclDomain::TemplatedSet<NET_FlashingLightAlarmInfo_S>;
    m_setTable[NET_SET_PIR_ALARM_INFO]           = &CSjclDomain::TemplatedSet<NET_PirAlarmInfo_S>;

    /* ===== 预览/对讲 ===== */
    m_setTable[NET_SET_PREVIEW_INFO]         = &CSjclDomain::TemplatedSet<NET_PreviewInfo_S>;
    m_setTable[NET_SET_VOICECOM_AUDIO_CFG]   = &CSjclDomain::TemplatedSet<NET_VoiceComAudioCfg_S>;

    /* ===== 抓拍 ===== */
    m_setTable[NET_SET_CAPTURE_PLAN_INFO]   = &CSjclDomain::TemplatedSet<NET_CapturePlanInfo_S>;
    m_setTable[NET_SET_CAPTURE_PARAM_INFO]  = &CSjclDomain::TemplatedSet<NET_CaptureParamInfo_S>;

       /* ===== 对讲状态控制 ===== */
    m_setTable[NET_STATE_TALKBACK]      = &CSjclDomain::TemplatedSet<NET_TalkbackStateInfo_S>;
    m_setTable[NET_TO_STREAM_TALKBACK]  = &CSjclDomain::TemplatedSet<NET_TalkbackStreamInfo_S>;
    m_setTable[NET_REPLAY_TALKBACK]     = &CSjclDomain::TemplatedSet<NET_ReplayTalkbackInfo_S>;

    /* ===== 人脸库/人脸信息 ===== */
    m_setTable[NET_SET_FACECAPTUREINFO]   = &CSjclDomain::TemplatedSet<NET_FaceCaptureInfo_S>;
    m_setTable[NET_SET_FACE_COMPARE_INFO] = &CSjclDomain::TemplatedSet<NET_FaceCompareInfo_S>;
    m_setTable[NET_ADD_TARGET_LIB]        = &CSjclDomain::TemplatedSet<NET_FaceLibInfo_S>;
    m_setTable[NET_DEL_TARGET_LIB]        = &CSjclDomain::TemplatedSet<NET_FaceLibInfo_S>;
    m_setTable[NET_SET_TARGET_LIB]        = &CSjclDomain::TemplatedSet<NET_FaceLibInfo_S>;
    m_setTable[NET_ADD_FACE_INFO]         = &CSjclDomain::TemplatedSet<NET_FaceInfo_S>;
    m_setTable[NET_SET_FACE_INFO]         = &CSjclDomain::TemplatedSet<NET_FaceInfo_S>;
    m_setTable[NET_DEL_FACE_INFO]         = &CSjclDomain::TemplatedSet<NET_FaceIdInfo_S>;

    /* ===== AI 分析配置 ===== */
    m_setTable[NET_SET_PEOPLE_FLOW_STATISTICS_CFG]        = &CSjclDomain::TemplatedSet<NET_PeopleFlowStatisticsCfg_S>;
    m_setTable[NET_RESET_PEOPLE_FLOW_STATISTICS]          = &CSjclDomain::TemplatedSet<NET_PeopleFlowStatisticsCfg_S>;
    m_setTable[NET_SET_PEOPLE_DENSITY_DETECTION_CFG]      = &CSjclDomain::TemplatedSet<NET_PeopleDensityDetectionCfg_S>;
    m_setTable[NET_SET_MANHOLE_COVER_ABNORMAL_CFG]        = &CSjclDomain::TemplatedSet<NET_ManholeCoverAbnormalCfg_S>;
    m_setTable[NET_SET_SLEEP_ON_DUTY_CFG]                 = &CSjclDomain::TemplatedSet<NET_SleepOnDutyCfg_S>;
    m_setTable[NET_SET_ELECTRIC_VEHICLE_IN_ELEVATOR_CFG]  = &CSjclDomain::TemplatedSet<NET_ElectricVehicleInElevatorCfg_S>;
    m_setTable[NET_SET_PERSON_FALL_DOWN_CFG]              = &CSjclDomain::TemplatedSet<NET_PersonFallDownCfg_S>;
    m_setTable[NET_SET_CONSTRUCTION_OCCUPY_ROAD_CFG]      = &CSjclDomain::TemplatedSet<NET_ConstructionOccupyRoadCfg_S>;
    m_setTable[NET_SET_CONGESTION_CFG]                    = &CSjclDomain::TemplatedSet<NET_CongestionCfg_S>;
    m_setTable[NET_SET_LICENSE_PLATE_RECOGNITION_CFG]     = &CSjclDomain::TemplatedSet<NET_LicensePlateRecognitionCfg_S>;
    m_setTable[NET_SET_HIGH_ALTITUDE_SEATBELT_CFG]        = &CSjclDomain::TemplatedSet<NET_HighAltitudeSeatbeltCfg_S>;
    m_setTable[NET_SET_SAFETY_HELMET_CFG]                 = &CSjclDomain::TemplatedSet<NET_SafetyHelmetCfg_S>;
    m_setTable[NET_SET_PERSON_FALL_CFG]                   = &CSjclDomain::TemplatedSet<NET_PersonFallCfg_S>;
    m_setTable[NET_SET_PHONE_USAGE_CFG]                   = &CSjclDomain::TemplatedSet<NET_PhoneUsageCfg_S>;
    m_setTable[NET_SET_SMOKING_CFG]                       = &CSjclDomain::TemplatedSet<NET_SmokingCfg_S>;
    m_setTable[NET_SET_OPEN_FLAME_CFG]                    = &CSjclDomain::TemplatedSet<NET_OpenFlameCfg_S>;
    m_setTable[NET_SET_BARE_SOIL_CFG]                     = &CSjclDomain::TemplatedSet<NET_BareSoilCfg_S>;
    m_setTable[NET_SET_HOLE_PROTECTION_BAR_CFG]           = &CSjclDomain::TemplatedSet<NET_HoleProtectionBarCfg_S>;
    m_setTable[NET_SET_REFLECTIVE_CLOTHING_CFG]           = &CSjclDomain::TemplatedSet<NET_ReflectiveClothingCfg_S>;
    m_setTable[NET_SET_PET_RECOGNITION_INFO]              = &CSjclDomain::TemplatedSet<NET_PetRecognitionInfo_S>;
    m_setTable[NET_SET_CLIMB_FENCE_INFO]                  = &CSjclDomain::TemplatedSet<NET_ClimbFenceInfo_S>;
    m_setTable[NET_SET_DIMISSION_INFO]                    = &CSjclDomain::TemplatedSet<NET_DimissionInfo_S>;
    m_setTable[NET_SET_ILLEGAL_LANE_INFO]                 = &CSjclDomain::TemplatedSet<NET_IllegalLaneInfo_S>;
    m_setTable[NET_SET_RETROGRADE_INFO]                   = &CSjclDomain::TemplatedSet<NET_RetrogradeInfo_S>;
    m_setTable[NET_SET_NONMOTOR_VEHICLE_INTRUSION_INFO]   = &CSjclDomain::TemplatedSet<NET_NonmotorVehicleIntrusionInfo_S>;
    m_setTable[NET_SET_OCCUPATION_EMERGENCY_INFO]         = &CSjclDomain::TemplatedSet<NET_OccupationEmergencyInfo_S>;
    m_setTable[NET_SET_PEDESTRIAN_INTRUSION_INFO]         = &CSjclDomain::TemplatedSet<NET_PedestrianIntrusionInfo_S>;
    m_setTable[NET_SET_SMOKE_FIRE_CFG]                    = &CSjclDomain::TemplatedSet<NET_SmokeFireCfg_S>;
    m_setTable[NET_SET_ROAD_PONDING_CFG]                  = &CSjclDomain::TemplatedSet<NET_RoadPondingCfg_S>;

    /* ===== 通道名称 ===== */
    m_setTable[NET_SET_CHANNEL_NAME] = &CSjclDomain::TemplatedSet<NET_ChannelNameInfo_S>;

    /* ===== 透明通道 ===== */
    m_setTable[NET_OPEN_TRANSPARENT_CHANNEL]  = &CSjclDomain::TemplatedSet<NET_TransparentChannel_S>;
    m_setTable[NET_CLOSE_TRANSPARENT_CHANNEL] = &CSjclDomain::TemplatedSet<NET_TransparentChannel_S>;
    m_setTable[NET_SEND_TRANSPARENT_DATA]     = &CSjclDomain::TemplatedSet<NET_TransparentData_S>;

    /* ===== 串口通信 ===== */
    m_setTable[NET_SEND_SERIAL_DATA]        = &CSjclDomain::TemplatedSet<NET_SerialData_S>;
    m_setTable[NET_SEND_RS232_DATA]         = &CSjclDomain::TemplatedSet<NET_SerialData_S>;
    m_setTable[NET_SEND_SERIAL_DIRECT_DATA] = &CSjclDomain::TemplatedSet<NET_SerialData_S>;
}

#endif /* BU_SJCL_EXCLUDE */
