/**
 * @FilePath     : tvsdk_convert.cpp
 * @Description  : IPC <-> TVSDK 结构体转换实现
 */

#include "tvsdk_convert.h"

#include <algorithm>
#include <cstdio>
#include <cstring>
#include <vector>



#include "dlog.h"
namespace TvSdkConvert
{
static constexpr size_t kOsdCustomSlotCount = 4;

/* 将 IPC 内部目标集合转换为 SDK 目标数组。
 * 全选使用单个 NET_TARGET_ALL 表示，部分选择逐项返回，便于后续扩展新的目标类型。
 */
static void FillDetectionTargets(const std::vector<int> &src, INT32 &nCount, INT32 *pTargets)
{
    nCount = 0;
    if (!pTargets)
    {
        return;
    }

    bool bHuman = false;
    bool bVehicle = false;
    bool bOther = false;
    for (const int nTarget : src)
    {
        bHuman = bHuman || nTarget == static_cast<int>(Alarm::HUMAN_DETECTION);
        bVehicle = bVehicle || nTarget == static_cast<int>(Alarm::CAR_DETECTION);
        bOther = bOther || nTarget == static_cast<int>(Alarm::OTHER_DETECTION);
    }

    if (bHuman && bVehicle && bOther)
    {
        pTargets[nCount++] = NET_TARGET_ALL;
    }
    else
    {
        if (bHuman)
        {
            pTargets[nCount++] = NET_TARGET_HUMAN;
        }
        if (bVehicle)
        {
            pTargets[nCount++] = NET_TARGET_VEHICLE;
        }
        if (bOther)
        {
            pTargets[nCount++] = NET_TARGET_OTHER;
        }
    }

    if (nCount == 0)
    {
        pTargets[nCount++] = NET_TARGET_ALL;
    }
}

/* 将 SDK 目标数组展开为 IPC 算法使用的目标集合。 */
static void ToDetectionTargets(const INT32 *pTargets, INT32 nCount, std::vector<int> &dst)
{
    dst.clear();
    if (!pTargets || nCount <= 0)
    {
        dst.push_back(static_cast<int>(Alarm::HUMAN_DETECTION));
        dst.push_back(static_cast<int>(Alarm::CAR_DETECTION));
        dst.push_back(static_cast<int>(Alarm::OTHER_DETECTION));
        return;
    }

    const auto addTarget = [&dst](int nTarget)
    {
        if (std::find(dst.begin(), dst.end(), nTarget) == dst.end())
        {
            dst.push_back(nTarget);
        }
    };

    for (INT32 nIndex = 0; nIndex < std::min(nCount, 8); ++nIndex)
    {
        switch (pTargets[nIndex])
        {
        case NET_TARGET_ALL:
            addTarget(static_cast<int>(Alarm::HUMAN_DETECTION));
            addTarget(static_cast<int>(Alarm::CAR_DETECTION));
            addTarget(static_cast<int>(Alarm::OTHER_DETECTION));
            break;
        case NET_TARGET_HUMAN:
            addTarget(static_cast<int>(Alarm::HUMAN_DETECTION));
            break;
        case NET_TARGET_VEHICLE:
            addTarget(static_cast<int>(Alarm::CAR_DETECTION));
            break;
        case NET_TARGET_OTHER:
            addTarget(static_cast<int>(Alarm::OTHER_DETECTION));
            break;
        default:
            break;
        }
    }
}

static bool HasCustomOsdPayload(const Osd::OsdInfo_S &info)
{
    return info.bEnable ||
           !info.strName.empty() ||
           info.stOsdAttr.nX != 0 ||
           info.stOsdAttr.nY != 0 ||
           info.stOsdAttr.nW != 0 ||
           info.stOsdAttr.nH != 0 ||
           info.stOsdAttr.enAttribute != Osd::OSD_ATTR_ALPHA_N_FLASH_N ||
           info.stOsdAttr.enFontSize != Osd::OSD_FONT_SIZE_ADAPTIVE ||
           info.stOsdAttr.enFontColor != Osd::OSD_COLOR_BLACK;
}

static size_t GetOsdCopyStart(const std::vector<Osd::OsdInfo_S> &infos)
{
    if (infos.size() <= kOsdCustomSlotCount)
    {
        return 0;
    }

    bool bFirstBlockHasPayload = false;
    for (size_t i = 0; i < kOsdCustomSlotCount && i < infos.size(); ++i)
    {
        bFirstBlockHasPayload = bFirstBlockHasPayload || HasCustomOsdPayload(infos[i]);
    }

    bool bSecondBlockHasPayload = false;
    const size_t nSecondEnd = std::min(infos.size(), kOsdCustomSlotCount * 2);
    for (size_t i = kOsdCustomSlotCount; i < nSecondEnd; ++i)
    {
        bSecondBlockHasPayload = bSecondBlockHasPayload || HasCustomOsdPayload(infos[i]);
    }

    return (!bFirstBlockHasPayload && bSecondBlockHasPayload) ? kOsdCustomSlotCount : 0;
}

static void FillSchedTime(const Common::SchedTime_S &src, NET_SchedTime_S &dst)
{
    std::memset(&dst, 0, sizeof(dst));
    dst.nStartHour   = (INT32)src.stStart.nHour;
    dst.nStartMinute = (INT32)src.stStart.nMinute;
    dst.nEndHour     = (INT32)src.stStop.nHour;
    dst.nEndMinute   = (INT32)src.stStop.nMinute;
    dlog_info("[TVSDK][Sched] FillSchedTime: %02d:%02d -> %02d:%02d",
              (int)dst.nStartHour, (int)dst.nStartMinute,
              (int)dst.nEndHour, (int)dst.nEndMinute);
}

static void ToSchedTime(const NET_SchedTime_S &src, Common::SchedTime_S &dst)
{
    dst.stStart.nHour   = (int)src.nStartHour;
    dst.stStart.nMinute = (int)src.nStartMinute;
    dst.stStart.nSecond = 0;

    dst.stStop.nHour   = (int)src.nEndHour;
    dst.stStop.nMinute = (int)src.nEndMinute;
    dst.stStop.nSecond = 0;
     dlog_info("[TVSDK][Sched] ToSchedTime: %02d:%02d -> %02d:%02d",
              dst.stStart.nHour, dst.stStart.nMinute,
              dst.stStop.nHour, dst.stStop.nMinute);
}

static void FillLinkageList(const Alarm::LinkageList_S &src, NET_LinkageList_S &dst)
{
    std::memset(&dst, 0, sizeof(dst));

    /* 报警输出。 */
    dst.uAlarmOutputCount = (INT32)std::min(src.alarmOutput.size(), (size_t)NET_MAX_ALARM_OUT_NUM);
    for (size_t i = 0; i < (size_t)dst.uAlarmOutputCount; ++i)
    {
        dst.auAlarmOutput[i] = (INT32)src.alarmOutput[i];
    }

    /* 录像通道。 */
    dst.uRecordChannelCount = (INT32)std::min(src.recordChn.size(), (size_t)NET_CHANNEL_MAX);
    for (size_t i = 0; i < (size_t)dst.uRecordChannelCount; ++i)
    {
        dst.auRecordChannel[i] = (INT32)src.recordChn[i];
    }

    /*
     * 新版 NET_LinkageList_S 只承载报警输出、录像和抓拍通道。
     * 历史 SDK 把常规联动类型复用到抓拍通道字段，既不符合新版语义，也会把类型值误当成通道号，
     * 因此这里不再写入该类数据。
     */
}

void ToLinkageList(const NET_LinkageList_S &src, Alarm::LinkageList_S &dst)
{
    dst.alarmOutput.clear();
    dst.recordChn.clear();
    dst.tradition.clear();

    /* 报警输出。 */
    for (INT32 i = 0; i < src.uAlarmOutputCount && i < NET_MAX_ALARM_OUT_NUM; ++i)
    {
        dst.alarmOutput.push_back((int)src.auAlarmOutput[i]);
    }

    /* 录像通道。 */
    for (INT32 i = 0; i < src.uRecordChannelCount && i < NET_CHANNEL_MAX; ++i)
    {
        dst.recordChn.push_back((int)src.auRecordChannel[i]);
    }

    /* 新版协议没有常规联动类型字段，不能从抓拍通道反推声音、邮件等动作。 */
}

/**
 * @brief 将内部字符串安全复制到 SDK 固定长度字符数组。
 * @author ITC
 * @param [in] strSource 待复制的内部字符串。
 * @param [out] pDestination 接收字符串的 SDK 字符数组。
 * @param [in] uDestinationSize 接收字符数组的总长度。
 * @return 无。
 */
static void copy_alarm_string(const std::string& strSource, CHAR* pDestination, size_t uDestinationSize)
{
    if (!pDestination || uDestinationSize == 0)
    {
        return;
    }

    const size_t uCopyLength = std::min(strSource.size(), uDestinationSize - 1U);
    std::memcpy(pDestination, strSource.data(), uCopyLength);
    pDestination[uCopyLength] = '\0';
}

/**
 * @brief 从 SDK 固定长度字符数组读取字符串，不依赖末尾空字符。
 * @author ITC
 * @param [in] pSource 待读取的 SDK 字符数组。
 * @param [in] uSourceSize 字符数组的总长度。
 * @return 读取到的字符串；输入为空时返回空字符串。
 */
static std::string read_alarm_string(const CHAR* pSource, size_t uSourceSize)
{
    if (!pSource)
    {
        return std::string();
    }

    size_t uLength = 0;
    while (uLength < uSourceSize && pSource[uLength] != '\0')
    {
        ++uLength;
    }
    return std::string(pSource, uLength);
}

/**
 * @brief 将 IPC 动态周布防时间表转换为 SDK 固定长度时间表。
 * @author ITC
 * @param [in] aSource IPC 周布防时间表。
 * @param [out] stDestination 转换后的 SDK 周布防时间表。
 * @return 无。
 */
static void fill_alarm_schedule(const std::vector<std::vector<Common::SchedTime_S>>& aSource,
                                NET_AlarmSchedule_S& stDestination)
{
    std::memset(&stDestination, 0, sizeof(stDestination));
    const size_t uDayCount = std::min(aSource.size(),
                                      static_cast<size_t>(NET_ALARM_SCHEDULE_DAY_COUNT));
    for (size_t uDay = 0; uDay < uDayCount; ++uDay)
    {
        const size_t uSectionCount = std::min(aSource[uDay].size(), static_cast<size_t>(NET_PLAN_SECTION_NUM));
        stDestination.uTimeSectionCount[uDay] = static_cast<INT32>(uSectionCount);
        for (size_t uSection = 0; uSection < uSectionCount; ++uSection)
        {
            FillSchedTime(aSource[uDay][uSection], stDestination.astTimeSection[uDay][uSection]);
        }
    }
}

/**
 * @brief 将 SDK 固定长度周布防时间表转换为 IPC 动态时间表。
 * @author ITC
 * @param [in] stSource SDK 周布防时间表。
 * @param [out] aDestination 转换后的 IPC 周布防时间表。
 * @return 无。
 */
static void to_alarm_schedule(const NET_AlarmSchedule_S& stSource,
                              std::vector<std::vector<Common::SchedTime_S>>& aDestination)
{
    aDestination.clear();
    aDestination.resize(NET_ALARM_SCHEDULE_DAY_COUNT);
    for (INT32 nDay = 0; nDay < NET_ALARM_SCHEDULE_DAY_COUNT; ++nDay)
    {
        const INT32 nSectionCount = std::max<INT32>(0,
            std::min<INT32>(stSource.uTimeSectionCount[nDay], NET_PLAN_SECTION_NUM));
        aDestination[nDay].resize(static_cast<size_t>(nSectionCount));
        for (INT32 nSection = 0; nSection < nSectionCount; ++nSection)
        {
            ToSchedTime(stSource.astTimeSection[nDay][nSection], aDestination[nDay][nSection]);
        }
    }
}

/**
 * @brief 将 IPC 动态复制到通道列表转换为 SDK 固定长度数组。
 * @author ITC
 * @param [in] aSource IPC 复制到通道列表。
 * @param [out] nDestinationCount 转换后列表中的元素数量。
 * @param [out] pDestination 接收通道号的 SDK 固定长度数组。
 * @param [in] nDestinationCapacity 接收数组的元素容量。
 * @return 无。
 */
static void fill_alarm_copy_to(const std::vector<int>& aSource,
                               INT32& nDestinationCount,
                               INT32* pDestination,
                               INT32 nDestinationCapacity)
{
    if (!pDestination || nDestinationCapacity <= 0)
    {
        nDestinationCount = 0;
        return;
    }

    const size_t uCount = std::min(aSource.size(), static_cast<size_t>(nDestinationCapacity));
    nDestinationCount = static_cast<INT32>(uCount);
    for (size_t uIndex = 0; uIndex < uCount; ++uIndex)
    {
        pDestination[uIndex] = static_cast<INT32>(aSource[uIndex]);
    }
}

/**
 * @brief 将 SDK 固定长度复制到通道数组转换为 IPC 动态列表。
 * @author ITC
 * @param [in] nSourceCount SDK 数组中的有效元素数量。
 * @param [in] pSource SDK 固定长度通道数组。
 * @param [in] nSourceCapacity SDK 固定长度数组的元素容量。
 * @param [out] aDestination 转换后的 IPC 复制到通道列表。
 * @return 无。
 */
static void to_alarm_copy_to(INT32 nSourceCount,
                             const INT32* pSource,
                             INT32 nSourceCapacity,
                             std::vector<int>& aDestination)
{
    aDestination.clear();
    if (!pSource || nSourceCapacity <= 0)
    {
        return;
    }

    const INT32 nCount = std::max<INT32>(0, std::min<INT32>(nSourceCount, nSourceCapacity));
    aDestination.reserve(static_cast<size_t>(nCount));
    for (INT32 nIndex = 0; nIndex < nCount; ++nIndex)
    {
        aDestination.push_back(static_cast<int>(pSource[nIndex]));
    }
}

/**
 * @brief 将 IPC 声音告警配置转换为 SDK 结构体。
 * @author ITC
 * @param [in] stSource IPC 声音告警配置。
 * @param [out] stDestination 转换后的 SDK 声音告警配置。
 * @return 无。
 */
void FillAudibleAlarmInfo(const Alarm::SoundOutputAlarm_S& stSource,
                                        NET_AudibleAlarmInfo_S& stDestination)
{
    std::memset(&stDestination, 0, sizeof(stDestination));
    stDestination.enSoundType = static_cast<INT32>(stSource.enSoundType);
    stDestination.enAlertSound = static_cast<INT32>(stSource.enAlertSound);
    stDestination.nTimes = stSource.nTimes;
    stDestination.nCustomAudioCount = static_cast<INT32>(std::min(stSource.aCustomAudio.size(),
        static_cast<size_t>(NET_AUDIBLE_ALARM_CUSTOM_AUDIO_MAX_NUM)));
    for (INT32 nIndex = 0; nIndex < stDestination.nCustomAudioCount; ++nIndex)
    {
        const Alarm::CustomAudio_S& stAudio = stSource.aCustomAudio[static_cast<size_t>(nIndex)];
        stDestination.astCustomAudios[nIndex].bSelected = stAudio.bChoose ? TRUE : FALSE;
        copy_alarm_string(stAudio.strCustomeName, stDestination.astCustomAudios[nIndex].strName,
                          sizeof(stDestination.astCustomAudios[nIndex].strName));
        copy_alarm_string(stAudio.strPath, stDestination.astCustomAudios[nIndex].strPath,
                          sizeof(stDestination.astCustomAudios[nIndex].strPath));
    }
    fill_alarm_schedule(stSource.aAlarmTime, stDestination.stAlarmSchedule);
}

/**
 * @brief 将 SDK 声音告警配置转换为 IPC 结构体。
 * @author ITC
 * @param [in] stSource SDK 声音告警配置。
 * @param [out] stDestination 转换后的 IPC 声音告警配置。
 * @return 无。
 */
void ToAudibleAlarm(const NET_AudibleAlarmInfo_S& stSource,
                                  Alarm::SoundOutputAlarm_S& stDestination)
{
    stDestination.enSoundType = static_cast<Alarm::SoundType_E>(stSource.enSoundType);
    stDestination.enAlertSound = static_cast<Alarm::AlertSoundType_E>(stSource.enAlertSound);
    stDestination.nTimes = stSource.nTimes;
    stDestination.aCustomAudio.clear();
    const INT32 nAudioCount = std::max<INT32>(0,
        std::min<INT32>(stSource.nCustomAudioCount, NET_AUDIBLE_ALARM_CUSTOM_AUDIO_MAX_NUM));
    stDestination.aCustomAudio.reserve(static_cast<size_t>(nAudioCount));
    for (INT32 nIndex = 0; nIndex < nAudioCount; ++nIndex)
    {
        Alarm::CustomAudio_S stAudio;
        stAudio.bChoose = (stSource.astCustomAudios[nIndex].bSelected == TRUE);
        stAudio.strCustomeName = read_alarm_string(stSource.astCustomAudios[nIndex].strName,
                                                   sizeof(stSource.astCustomAudios[nIndex].strName));
        stAudio.strPath = read_alarm_string(stSource.astCustomAudios[nIndex].strPath,
                                            sizeof(stSource.astCustomAudios[nIndex].strPath));
        stDestination.aCustomAudio.push_back(stAudio);
    }
    to_alarm_schedule(stSource.stAlarmSchedule, stDestination.aAlarmTime);
}

/**
 * @brief 将 IPC 一路报警输入配置转换为 SDK 结构体。
 * @author ITC
 * @param [in] stSource IPC 报警输入配置。
 * @param [out] stDestination 转换后的 SDK 报警输入配置。
 * @return 无。
 */
void FillAlarmInputInfo(const Alarm::IoInputInfo_S& stSource,
                                      NET_AlarmInputInfo_S& stDestination)
{
    std::memset(&stDestination, 0, sizeof(stDestination));
    stDestination.nAlarmNumber = stSource.nIoNumer;
    copy_alarm_string(stSource.ioAddr, stDestination.strAlarmAddress, sizeof(stDestination.strAlarmAddress));
    copy_alarm_string(stSource.ioName, stDestination.strAlarmName, sizeof(stDestination.strAlarmName));
    stDestination.bNormallyOpen = stSource.bNormallyOpen ? TRUE : FALSE;
    stDestination.nDealType = stSource.nDealType;
    fill_alarm_schedule(stSource.aAlarmTime, stDestination.stAlarmSchedule);
    FillLinkageList(stSource.stLinkageList, stDestination.stLinkageList);
    fill_alarm_copy_to(stSource.copyTo, stDestination.nCopyToCount, stDestination.anCopyTo,
                       NET_ALARM_COPY_TO_MAX_NUM);
}

/**
 * @brief 将 SDK 一路报警输入配置转换为 IPC 结构体。
 * @author ITC
 * @param [in] stSource SDK 报警输入配置。
 * @param [out] stDestination 转换后的 IPC 报警输入配置。
 * @return 无。
 */
void ToAlarmInputInfo(const NET_AlarmInputInfo_S& stSource,
                                    Alarm::IoInputInfo_S& stDestination)
{
    stDestination.nIoNumer = stSource.nAlarmNumber;
    stDestination.ioAddr = read_alarm_string(stSource.strAlarmAddress, sizeof(stSource.strAlarmAddress));
    stDestination.ioName = read_alarm_string(stSource.strAlarmName, sizeof(stSource.strAlarmName));
    stDestination.bNormallyOpen = (stSource.bNormallyOpen == TRUE);
    stDestination.nDealType = stSource.nDealType;
    to_alarm_schedule(stSource.stAlarmSchedule, stDestination.aAlarmTime);
    ToLinkageList(stSource.stLinkageList, stDestination.stLinkageList);
    to_alarm_copy_to(stSource.nCopyToCount, stSource.anCopyTo, NET_ALARM_COPY_TO_MAX_NUM,
                     stDestination.copyTo);
}

/**
 * @brief 将 IPC 报警输入配置集合转换为 SDK 结构体。
 * @author ITC
 * @param [in] stSource IPC 报警输入配置集合。
 * @param [out] stDestination 转换后的 SDK 报警输入配置集合。
 * @return 无。
 */
void FillAlarmInputInfoList(const std::set<Alarm::IoInputInfo_S>& stSource,
                                          NET_AlarmInputInfoList_S& stDestination)
{
    std::memset(&stDestination, 0, sizeof(stDestination));
    for (const Alarm::IoInputInfo_S& stInput : stSource)
    {
        if (stDestination.nAlarmInputCount >= NET_MAX_ALARM_IN_NUM)
        {
            break;
        }
        FillAlarmInputInfo(stInput, stDestination.astAlarmInputs[stDestination.nAlarmInputCount]);
        ++stDestination.nAlarmInputCount;
    }
}

/**
 * @brief 将 IPC 一路报警输出配置转换为 SDK 结构体。
 * @author ITC
 * @param [in] stSource IPC 报警输出配置。
 * @param [out] stDestination 转换后的 SDK 报警输出配置。
 * @return 无。
 */
void FillAlarmOutputInfo(const Alarm::IoOutputInfo_S& stSource,
                                       NET_AlarmOutputInfo_S& stDestination)
{
    std::memset(&stDestination, 0, sizeof(stDestination));
    stDestination.nAlarmNumber = stSource.nIoNumer;
    copy_alarm_string(stSource.ioAddr, stDestination.strAlarmAddress, sizeof(stDestination.strAlarmAddress));
    copy_alarm_string(stSource.ioName, stDestination.strAlarmName, sizeof(stDestination.strAlarmName));
    stDestination.nDelayTime = stSource.nDelayTime;
    stDestination.enState = static_cast<INT32>(stSource.enState);
    fill_alarm_schedule(stSource.aAlarmTime, stDestination.stAlarmSchedule);
    fill_alarm_copy_to(stSource.copyTo, stDestination.nCopyToCount, stDestination.anCopyTo,
                       NET_ALARM_COPY_TO_MAX_NUM);
}

/**
 * @brief 将 SDK 一路报警输出配置转换为 IPC 结构体。
 * @author ITC
 * @param [in] stSource SDK 报警输出配置。
 * @param [out] stDestination 转换后的 IPC 报警输出配置。
 * @return 无。
 */
void ToAlarmOutputInfo(const NET_AlarmOutputInfo_S& stSource,
                                     Alarm::IoOutputInfo_S& stDestination)
{
    stDestination.nIoNumer = stSource.nAlarmNumber;
    stDestination.ioAddr = read_alarm_string(stSource.strAlarmAddress, sizeof(stSource.strAlarmAddress));
    stDestination.ioName = read_alarm_string(stSource.strAlarmName, sizeof(stSource.strAlarmName));
    stDestination.nDelayTime = stSource.nDelayTime;
    stDestination.enState = static_cast<Alarm::IoOutputState_E>(stSource.enState);
    to_alarm_schedule(stSource.stAlarmSchedule, stDestination.aAlarmTime);
    to_alarm_copy_to(stSource.nCopyToCount, stSource.anCopyTo, NET_ALARM_COPY_TO_MAX_NUM,
                     stDestination.copyTo);
}

/**
 * @brief 将 IPC 报警输出配置集合转换为 SDK 结构体。
 * @author ITC
 * @param [in] stSource IPC 报警输出配置集合。
 * @param [out] stDestination 转换后的 SDK 报警输出配置集合。
 * @return 无。
 */
void FillAlarmOutputInfoList(const std::set<Alarm::IoOutputInfo_S>& stSource,
                                           NET_AlarmOutputInfoList_S& stDestination)
{
    std::memset(&stDestination, 0, sizeof(stDestination));
    for (const Alarm::IoOutputInfo_S& stOutput : stSource)
    {
        if (stDestination.nAlarmOutputCount >= NET_MAX_ALARM_OUT_NUM)
        {
            break;
        }
        FillAlarmOutputInfo(stOutput, stDestination.astAlarmOutputs[stDestination.nAlarmOutputCount]);
        ++stDestination.nAlarmOutputCount;
    }
}

/**
 * @brief 将 IPC 闪光灯告警配置转换为 SDK 结构体。
 * @author ITC
 * @param [in] stSource IPC 闪光灯告警配置。
 * @param [out] stDestination 转换后的 SDK 闪光灯告警配置。
 * @return 无。
 */
void FillFlashingLightAlarmInfo(const Alarm::FlashInfo_S& stSource,
                                              NET_FlashingLightAlarmInfo_S& stDestination)
{
    std::memset(&stDestination, 0, sizeof(stDestination));
    stDestination.nFlashTime = stSource.nFlashTime;
    stDestination.enFlashFrequency = static_cast<INT32>(stSource.enFalshFrequency);
    fill_alarm_schedule(stSource.aAlarmTime, stDestination.stAlarmSchedule);
    fill_alarm_copy_to(stSource.copyTo, stDestination.nCopyToCount, stDestination.anCopyTo,
                       NET_ALARM_COPY_TO_MAX_NUM);
}

/**
 * @brief 将 SDK 闪光灯告警配置转换为 IPC 结构体。
 * @author ITC
 * @param [in] stSource SDK 闪光灯告警配置。
 * @param [out] stDestination 转换后的 IPC 闪光灯告警配置。
 * @return 无。
 */
void ToFlashingLightAlarm(const NET_FlashingLightAlarmInfo_S& stSource,
                                        Alarm::FlashInfo_S& stDestination)
{
    stDestination.nFlashTime = stSource.nFlashTime;
    stDestination.enFalshFrequency = static_cast<Alarm::FlashFrequency_E>(stSource.enFlashFrequency);
    to_alarm_schedule(stSource.stAlarmSchedule, stDestination.aAlarmTime);
    to_alarm_copy_to(stSource.nCopyToCount, stSource.anCopyTo, NET_ALARM_COPY_TO_MAX_NUM,
                     stDestination.copyTo);
}

/**
 * @brief 将 IPC PIR 告警配置转换为 SDK 结构体。
 * @author ITC
 * @param [in] stSource IPC PIR 告警配置。
 * @param [out] stDestination 转换后的 SDK PIR 告警配置。
 * @return 无。
 */
void FillPirAlarmInfo(const Alarm::PirAlarmInfo_S& stSource,
                                    NET_PirAlarmInfo_S& stDestination)
{
    std::memset(&stDestination, 0, sizeof(stDestination));
    stDestination.bEnable = stSource.bEnable ? TRUE : FALSE;
    copy_alarm_string(stSource.AlarmName, stDestination.strAlarmName, sizeof(stDestination.strAlarmName));
    fill_alarm_schedule(stSource.aAlarmTime, stDestination.stAlarmSchedule);
    FillLinkageList(stSource.stLinkageList, stDestination.stLinkageList);
    fill_alarm_copy_to(stSource.copyTo, stDestination.nCopyToCount, stDestination.anCopyTo,
                       NET_ALARM_COPY_TO_MAX_NUM);
}

/**
 * @brief 将 SDK PIR 告警配置转换为 IPC 结构体。
 * @author ITC
 * @param [in] stSource SDK PIR 告警配置。29
 * @param [out] stDestination 转换后的 IPC PIR 告警配置。
 * @return 无。
 */
void ToPirAlarmInfo(const NET_PirAlarmInfo_S& stSource,
                                  Alarm::PirAlarmInfo_S& stDestination)
{
    stDestination.bEnable = (stSource.bEnable == TRUE);
    stDestination.AlarmName = read_alarm_string(stSource.strAlarmName, sizeof(stSource.strAlarmName));
    to_alarm_schedule(stSource.stAlarmSchedule, stDestination.aAlarmTime);
    ToLinkageList(stSource.stLinkageList, stDestination.stLinkageList);
    to_alarm_copy_to(stSource.nCopyToCount, stSource.anCopyTo, NET_ALARM_COPY_TO_MAX_NUM,
                     stDestination.copyTo);
}

void FillDeviceInfo(const ::System::DeviceInfo_S &src, NET_DeviceInfo_S &dst)
{
    memset(&dst, 0, sizeof(dst));
    dst.uDevType        = (INT32)NET_DTYPE_IPC;
    dst.uAlarmInPortNum  = (INT16)src.nAlarmInputCount;
    dst.uAlarmOutPortNum = (INT16)src.nAlarmOutputCount;
    dst.uChannelNum     = 1;
}

void FillDeviceBasicInfo(const ::System::DeviceInfo_S &src, NET_DeviceBasicInfo_S &dst)
{
    memset(&dst, 0, sizeof(dst));
    strncpy(dst.strDevModel, src.strUnitTpye.c_str(), sizeof(dst.strDevModel) - 1);
    strncpy(dst.strDeviceTypeV2, src.strUnitTpye.c_str(), sizeof(dst.strDeviceTypeV2) - 1);
    strncpy(dst.strSerialNum, src.serialNumber.c_str(), sizeof(dst.strSerialNum) - 1);
    strncpy(dst.strFirmwareVersion, src.systemVersion.c_str(), sizeof(dst.strFirmwareVersion) - 1);
    strncpy(dst.strDeviceName, src.deviceName.c_str(), sizeof(dst.strDeviceName) - 1);
    strncpy(dst.strManufacturer, src.strUnitTpye.c_str(), sizeof(dst.strManufacturer) - 1);
}

void ToDeviceInfo(const NET_DeviceBasicInfo_S &src, ::System::DeviceInfo_S &dst)
{
    dst.deviceName   = src.strDeviceName;
    dst.strUnitTpye  = src.strDevModel;
    dst.serialNumber = src.strSerialNum;
    dst.systemVersion = src.strFirmwareVersion;
}

void FillSystemNtpInfo(const ::System::TimeInfo_S &src, NET_SystemNtpInfo_S &dst)
{
    std::memset(&dst, 0, sizeof(dst));
    dst.enTimeZone = static_cast<INT32>(src.enTimeZone);
    dst.enDateFormat = static_cast<INT32>(src.enDateFormat);
    dst.bEnableNTPSync = src.bEnableNTPSync ? TRUE : FALSE;
    dst.bManualSync = src.bManualSync ? TRUE : FALSE;
    dst.bIsSyncWithComputer = src.bIsSyncWithComputer ? TRUE : FALSE;
    dst.nPort = src.stNTPInfo.nPort;
    dst.nSyncInterval = src.stNTPInfo.nSyncInterval;
    std::strncpy(dst.strDateTime, src.strDateTime.c_str(), sizeof(dst.strDateTime) - 1);
    std::strncpy(dst.strAddress, src.stNTPInfo.address.c_str(), sizeof(dst.strAddress) - 1);
}

void ToTimeInfo(const NET_SystemNtpInfo_S &src, ::System::TimeInfo_S &dst)
{
    dst.enTimeZone = static_cast<::System::TimeZone_E>(src.enTimeZone);
    dst.enDateFormat = static_cast<::System::DateFormat_E>(src.enDateFormat);
    dst.bEnableNTPSync = (src.bEnableNTPSync == TRUE);
    dst.bManualSync = (src.bManualSync == TRUE);
    dst.bIsSyncWithComputer = (src.bIsSyncWithComputer == TRUE);
    dst.strDateTime = src.strDateTime;
    dst.stNTPInfo.address = src.strAddress;
    dst.stNTPInfo.nPort = src.nPort;
    dst.stNTPInfo.nSyncInterval = src.nSyncInterval;
}

void FillNetworkCfg(const Network::Info_S &src, NET_NetworkCfg_S &dst)
{
    memset(&dst, 0, sizeof(dst));
    dst.uMTU = (INT32)src.stIp.nMtu;
    dst.bIPv4DHCP = src.stIp.bEnableDhcp ? TRUE : FALSE;
    strncpy(dst.szIpv4Address, src.stIp.ipv4Ip.c_str(), sizeof(dst.szIpv4Address) - 1);
    strncpy(dst.szIPv4GateWay, src.stIp.ipv4Gateway.c_str(), sizeof(dst.szIPv4GateWay) - 1);
    strncpy(dst.szIPv4SubnetMask, src.stIp.ipv4Mask.c_str(), sizeof(dst.szIPv4SubnetMask) - 1);
}

void ToNetworkInfo(const NET_NetworkCfg_S &src, Network::Info_S &dst)
{
    dst.stIp.bEnableDhcp = (src.bIPv4DHCP == TRUE);
    dst.stIp.ipv4Ip      = src.szIpv4Address;
    dst.stIp.ipv4Gateway = src.szIPv4GateWay;
    dst.stIp.ipv4Mask    = src.szIPv4SubnetMask;
    dst.stIp.nMtu        = (int)src.uMTU;
}

void FillWifiStaCfg(const Network::WifiStaInfo_S &src, NET_WifiStaCfg_S &dst)
{
    std::memset(&dst, 0, sizeof(dst));
    dst.bEnableWifi = src.bEnableWifi ? TRUE : FALSE;
    dst.bEnableBoost = src.bEnableBoost ? TRUE : FALSE;
}

void ToWifiStaInfo(const NET_WifiStaCfg_S &src, Network::WifiStaInfo_S &dst)
{
    dst.bEnableWifi = (src.bEnableWifi == TRUE);
    dst.bEnableBoost = (src.bEnableBoost == TRUE);
}

void FillWifiStaConnect(const Network::WifiStaConncet_S &src, NET_WifiStaConnect_S &dst)
{
    std::memset(&dst, 0, sizeof(dst));
    std::strncpy(dst.szSsid, src.ssid.c_str(), sizeof(dst.szSsid) - 1);
    dst.nSecurityMode = (INT32)src.mode;
    std::strncpy(dst.szIpAddress, src.ip_address.c_str(), sizeof(dst.szIpAddress) - 1);
    std::strncpy(dst.szPassword, src.password.c_str(), sizeof(dst.szPassword) - 1);
    std::strncpy(dst.szPairwise, src.pairwise.c_str(), sizeof(dst.szPairwise) - 1);
    dst.nWepKeyLen = src.wep_key_len;
    dst.bWepIsHex = src.wep_is_hex ? TRUE : FALSE;
    std::strncpy(dst.szAuthAlg, src.auth_alg.c_str(), sizeof(dst.szAuthAlg) - 1);

    const size_t nWepKeyCount = std::min<size_t>(src.wep_keys.size(), 4);
    dst.nWepKeyCount = (INT32)nWepKeyCount;
    for (size_t i = 0; i < nWepKeyCount; ++i)
    {
        dst.astWepKeys[i].nIndex = src.wep_keys[i].index;
        std::strncpy(dst.astWepKeys[i].szValue, src.wep_keys[i].value.c_str(), sizeof(dst.astWepKeys[i].szValue) - 1);
    }

    std::strncpy(dst.szEapIdentity, src.eap_identity.c_str(), sizeof(dst.szEapIdentity) - 1);
    std::strncpy(dst.szEapPassword, src.eap_password.c_str(), sizeof(dst.szEapPassword) - 1);
    std::strncpy(dst.szPeapVersion, src.peap_version.c_str(), sizeof(dst.szPeapVersion) - 1);
    std::strncpy(dst.szPhase2, src.phase2.c_str(), sizeof(dst.szPhase2) - 1);
    std::strncpy(dst.szAnonymousIdentity, src.anonymous_identity.c_str(), sizeof(dst.szAnonymousIdentity) - 1);
    std::strncpy(dst.szCaCertPath, src.ca_cert_path.c_str(), sizeof(dst.szCaCertPath) - 1);
    std::strncpy(dst.szPeapLabel, src.peap_label.c_str(), sizeof(dst.szPeapLabel) - 1);

    std::strncpy(dst.szTlsIdentity, src.tls_identity.c_str(), sizeof(dst.szTlsIdentity) - 1);
    std::strncpy(dst.szPrivateKeyPasswd, src.private_key_passwd.c_str(), sizeof(dst.szPrivateKeyPasswd) - 1);
    std::strncpy(dst.szEapolVersion, src.eapol_version.c_str(), sizeof(dst.szEapolVersion) - 1);
    std::strncpy(dst.szClientCertPath, src.client_cert_path.c_str(), sizeof(dst.szClientCertPath) - 1);
    std::strncpy(dst.szPrivateKeyPath, src.private_key_path.c_str(), sizeof(dst.szPrivateKeyPath) - 1);
    std::strncpy(dst.szCtrlInterface, src.ctrl_interface.c_str(), sizeof(dst.szCtrlInterface) - 1);
    std::strncpy(dst.szInterfaceName, src.interface_name.c_str(), sizeof(dst.szInterfaceName) - 1);
}

void ToWifiStaConnect(const NET_WifiStaConnect_S &src, Network::WifiStaConncet_S &dst)
{
    dst.ssid = src.szSsid;
    if (src.nSecurityMode >= (INT32)Network::WifiSecurityMode::WPA_PERSONAL &&
        src.nSecurityMode <= (INT32)Network::WifiSecurityMode::EAP_TLS)
    {
        dst.mode = (Network::WifiSecurityMode)src.nSecurityMode;
    }
    else
    {
        dst.mode = Network::WifiSecurityMode::OPEN;
    }

    dst.ip_address = src.szIpAddress;
    dst.password = src.szPassword;
    dst.pairwise = src.szPairwise;
    dst.wep_key_len = src.nWepKeyLen;
    dst.wep_is_hex = (src.bWepIsHex == TRUE);
    dst.auth_alg = src.szAuthAlg;

    dst.wep_keys.clear();
    const int nWepKeyCount = std::max(0, std::min(src.nWepKeyCount, 4));
    for (int i = 0; i < nWepKeyCount; ++i)
    {
        Network::WepKeyConfig stKey;
        stKey.index = src.astWepKeys[i].nIndex;
        stKey.value = src.astWepKeys[i].szValue;
        dst.wep_keys.push_back(stKey);
    }

    dst.eap_identity = src.szEapIdentity;
    dst.eap_password = src.szEapPassword;
    dst.peap_version = src.szPeapVersion;
    dst.phase2 = src.szPhase2;
    dst.anonymous_identity = src.szAnonymousIdentity;
    dst.ca_cert_path = src.szCaCertPath;
    dst.peap_label = src.szPeapLabel;

    dst.tls_identity = src.szTlsIdentity;
    dst.private_key_passwd = src.szPrivateKeyPasswd;
    dst.eapol_version = src.szEapolVersion;
    dst.client_cert_path = src.szClientCertPath;
    dst.private_key_path = src.szPrivateKeyPath;
    dst.ctrl_interface = src.szCtrlInterface;
    dst.interface_name = src.szInterfaceName;
}

void Fill4GInfo(const Network::Network_4G_Config_t &src, NET_4GInfo_S &dst)
{
    std::memset(&dst, 0, sizeof(dst));
    std::strncpy(dst.szApn, src.apn.c_str(), sizeof(dst.szApn) - 1);
    std::strncpy(dst.szUserName, src.username.c_str(), sizeof(dst.szUserName) - 1);
    std::strncpy(dst.szPassword, src.password.c_str(), sizeof(dst.szPassword) - 1);
    std::strncpy(dst.szCallNumber, src.call_number.c_str(), sizeof(dst.szCallNumber) - 1);
    dst.nMtu = src.mtu;
    dst.nAuthMode = src.auth_mode;
    dst.nNetworkMode = src.network_mode;
    dst.nDialMode = src.dial_mode;
}

void To4GConfig(const NET_4GInfo_S &src, Network::Network_4G_Config_t &dst)
{
    dst.apn = src.szApn;
    dst.username = src.szUserName;
    dst.password = src.szPassword;
    dst.call_number = src.szCallNumber;
    dst.mtu = src.nMtu;
    dst.auth_mode = src.nAuthMode;
    dst.network_mode = src.nNetworkMode;
    dst.dial_mode = src.nDialMode;
}

void FillHotspotInfo(const Network::HotspotConfig &src, NET_HotspotInfo_S &dst)
{
    std::memset(&dst, 0, sizeof(dst));
    dst.bEnabled = src.enabled ? TRUE : FALSE;
    std::strncpy(dst.szSsid, src.ssid.c_str(), sizeof(dst.szSsid) - 1);
    std::strncpy(dst.szSecurityMode, src.securityMode.c_str(), sizeof(dst.szSecurityMode) - 1);
    std::strncpy(dst.szEncryptionType, src.encryptionType.c_str(), sizeof(dst.szEncryptionType) - 1);
    std::strncpy(dst.szPassword, src.password.c_str(), sizeof(dst.szPassword) - 1);
    std::strncpy(dst.szConfirmPassword, src.confirmPassword.c_str(), sizeof(dst.szConfirmPassword) - 1);
}

void ToHotspotConfig(const NET_HotspotInfo_S &src, Network::HotspotConfig &dst)
{
    dst.enabled = (src.bEnabled == TRUE);
    dst.ssid = src.szSsid;
    dst.securityMode = src.szSecurityMode;
    dst.encryptionType = src.szEncryptionType;
    dst.password = src.szPassword;
    dst.confirmPassword = src.szConfirmPassword;
}

static INT32 ToSdkVideoCodec(Video_NS::VideoCodec_E src)
{
    switch (src)
    {
        case Video_NS::VideoCodec_E::H264:
            return NET_VIDEO_CODE_H264;
        case Video_NS::VideoCodec_E::H265:
            return NET_VIDEO_CODE_H265;
        case Video_NS::VideoCodec_E::JPEG:
            return NET_VIDEO_CODE_JPEG;
        case Video_NS::VideoCodec_E::MJPEG:
            return NET_VIDEO_CODE_MJPEG;
        case Video_NS::VideoCodec_E::SVAC3:
            return NET_VIDEO_CODE_SVAC3;
        case Video_NS::VideoCodec_E::MPEG4:
            return NET_VIDEO_CODE_MPEG4;
        default:
            return NET_VIDEO_CODE_INVALID;
    }
}

static Video_NS::VideoCodec_E ToIpcVideoCodec(INT32 src)
{
    switch (src)
    {
        case NET_VIDEO_CODE_H264:
            return Video_NS::VideoCodec_E::H264;
        case NET_VIDEO_CODE_H265:
            return Video_NS::VideoCodec_E::H265;
        case NET_VIDEO_CODE_JPEG:
            return Video_NS::VideoCodec_E::JPEG;
        case NET_VIDEO_CODE_MJPEG:
            return Video_NS::VideoCodec_E::MJPEG;
        case NET_VIDEO_CODE_SVAC3:
            return Video_NS::VideoCodec_E::SVAC3;
        case NET_VIDEO_CODE_MPEG4:
            return Video_NS::VideoCodec_E::MPEG4;
        default:
            return Video_NS::VideoCodec_E::H264;
    }
}

void FillVideoEncodeOption(const Video_NS::VideoConfig_S &src, NET_VideoEncodeOption_S &dst)
{
    std::memset(&dst, 0, sizeof(dst));
    dst.nId = (INT32)src.nId;
    dst.enVideoType = (INT32)src.enVideoType;
    dst.stVideoResolution.uWidth = (INT32)src.stVideoResolution.nWidth;
    dst.stVideoResolution.uHeight = (INT32)src.stVideoResolution.nHeight;
    dst.enBitrateType = (INT32)src.enBitrateType;
    dst.enImageQuality = (INT32)src.enImageQuality;
    dst.enFrameRate = (INT32)src.getFrameRateAsInt();
    dst.nBitrateUpperLimit = (INT32)src.nBitrateUpperLimit;
    dst.nAverageBitrate = (INT32)src.nAverageBitrate;
    dst.enVideoCodec = ToSdkVideoCodec(src.enVideoCodec);
    dst.bSmartEnable = src.bSmartEnable ? TRUE : FALSE;
    dst.enEncodingComplexity = (INT32)src.enEncodingComplexity;
    dst.nIFrameInterval = (INT32)src.nIFrameInterval;
    dst.enSvcEnable = (INT32)src.enSvcEnable;
    dst.nBitrateSmoothing = (INT32)src.nBitrateSmoothing;
}

void ToVideoConfig(const NET_VideoEncodeOption_S &src, Video_NS::VideoConfig_S &dst)
{
    dst.nId = (int)src.nId;
    dst.enVideoType = (Video_NS::VideoType_E)src.enVideoType;
    dst.stVideoResolution.nWidth = (int)src.stVideoResolution.uWidth;
    dst.stVideoResolution.nHeight = (int)src.stVideoResolution.uHeight;
    dst.enBitrateType = (Video_NS::BitrateType_E)src.enBitrateType;
    dst.enImageQuality = (Video_NS::ImageQuality_E)src.enImageQuality;
    dst.setFrameRate((int)src.enFrameRate);
    dst.nBitrateUpperLimit = (int)src.nBitrateUpperLimit;
    dst.nAverageBitrate = (int)src.nAverageBitrate;
    dst.enVideoCodec = ToIpcVideoCodec(src.enVideoCodec);
    dst.bSmartEnable = (src.bSmartEnable == TRUE);
    dst.enEncodingComplexity = (Video_NS::EncodingComplexity_E)src.enEncodingComplexity;
    dst.nIFrameInterval = (int)src.nIFrameInterval;
    dst.enSvcEnable = (Video_NS::SvcMode_E)src.enSvcEnable;
    dst.nBitrateSmoothing = (int)src.nBitrateSmoothing;
}

static void FillOsdAttribute(const Osd::OsdAttribute_S &src, OsdAttribute_S &dst)
{
    dst.nX = (INT32)src.nX;
    dst.nY = (INT32)src.nY;
    dst.nW = (INT32)src.nW;
    dst.nH = (INT32)src.nH;
    dst.enAttribute = (OSD_ATTRIBUTE_E)src.enAttribute;
    dst.enFontSize = (OSD_FONT_SIZE_E)src.enFontSize;
    dst.enFontColor = (OSD_COLOR_E)src.enFontColor;
    std::strncpy(dst.strFontColor, src.strFontColor.c_str(), sizeof(dst.strFontColor) - 1);
    std::strncpy(dst.strToken, src.strToken.c_str(), sizeof(dst.strToken) - 1);
}

static void ToOsdAttribute(const OsdAttribute_S &src, Osd::OsdAttribute_S &dst)
{
    dst.nX = (int)src.nX;
    dst.nY = (int)src.nY;
    dst.nW = (int)src.nW;
    dst.nH = (int)src.nH;
    dst.enAttribute = (Osd::OSD_ATTRIBUTE_E)src.enAttribute;
    dst.enFontSize = (Osd::OSD_FONT_SIZE_E)src.enFontSize;
    dst.enFontColor = (Osd::OSD_COLOR_E)src.enFontColor;
    dst.strFontColor = src.strFontColor;
    dst.strToken = src.strToken;
}

void FillOsdConfig(const Osd::OsdConfig_S &src, NET_VideoOsdCfg_S &dst)
{
    std::memset(&dst, 0, sizeof(dst));
    dst.enAlign = (OSD_ALIGN_E)src.enAlign;

    dst.stOsdNameInfo.bEnable = src.stOsdNameInfo.bEnable ? TRUE : FALSE;
    std::strncpy(dst.stOsdNameInfo.strName, src.stOsdNameInfo.strName.c_str(), sizeof(dst.stOsdNameInfo.strName) - 1);
    FillOsdAttribute(src.stOsdNameInfo.stOsdAttr, dst.stOsdNameInfo.stOsdAttr);

    dst.stOsdTimeInfo.bEnable = src.stOsdTimeInfo.bEnable ? TRUE : FALSE;
    dst.stOsdTimeInfo.bEnableWeek = src.stOsdTimeInfo.bEnableWeek ? TRUE : FALSE;
    dst.stOsdTimeInfo.enTimeFormat = (OSD_TIME_FORMAT_E)src.stOsdTimeInfo.enTimeFormat;
    dst.stOsdTimeInfo.enDateFormat = (OSD_DATE_FORMAT_E)src.stOsdTimeInfo.enDateFormat;
    FillOsdAttribute(src.stOsdTimeInfo.stOsdAttr, dst.stOsdTimeInfo.stOsdAttr);

    const size_t nStart = GetOsdCopyStart(src.vecOsdInfo);
    const size_t nCount = std::min(src.vecOsdInfo.size() - nStart, kOsdCustomSlotCount);
    for (size_t i = 0; i < nCount; ++i)
    {
        const Osd::OsdInfo_S &srcInfo = src.vecOsdInfo[nStart + i];
        dst.OsdInfo[i].nId = (INT32)srcInfo.nId;
        dst.OsdInfo[i].bEnable = srcInfo.bEnable ? TRUE : FALSE;
        std::strncpy(dst.OsdInfo[i].strName, srcInfo.strName.c_str(), sizeof(dst.OsdInfo[i].strName) - 1);
        FillOsdAttribute(srcInfo.stOsdAttr, dst.OsdInfo[i].stOsdAttr);
    }
}

void ToOsdConfig(const NET_VideoOsdCfg_S &src, Osd::OsdConfig_S &dst)
{
    dst.clear();
    dst.enAlign = (Osd::OSD_ALIGN_E)src.enAlign;

    dst.stOsdNameInfo.bEnable = (src.stOsdNameInfo.bEnable == TRUE);
    dst.stOsdNameInfo.strName = src.stOsdNameInfo.strName;
    ToOsdAttribute(src.stOsdNameInfo.stOsdAttr, dst.stOsdNameInfo.stOsdAttr);

    dst.stOsdTimeInfo.bEnable = (src.stOsdTimeInfo.bEnable == TRUE);
    dst.stOsdTimeInfo.bEnableWeek = (src.stOsdTimeInfo.bEnableWeek == TRUE);
    dst.stOsdTimeInfo.enTimeFormat = (Osd::OSD_TIME_FORMAT_E)src.stOsdTimeInfo.enTimeFormat;
    dst.stOsdTimeInfo.enDateFormat = (Osd::OSD_DATE_FORMAT_E)src.stOsdTimeInfo.enDateFormat;
    ToOsdAttribute(src.stOsdTimeInfo.stOsdAttr, dst.stOsdTimeInfo.stOsdAttr);

    const size_t nCount = std::min(dst.vecOsdInfo.size(), kOsdCustomSlotCount);
    for (size_t i = 0; i < nCount; ++i)
    {
        dst.vecOsdInfo[i].nId = (int)src.OsdInfo[i].nId;
        dst.vecOsdInfo[i].bEnable = (src.OsdInfo[i].bEnable == TRUE);
        dst.vecOsdInfo[i].strName = src.OsdInfo[i].strName;
        ToOsdAttribute(src.OsdInfo[i].stOsdAttr, dst.vecOsdInfo[i].stOsdAttr);
    }

    dst.init_token();
}

void FillPrivacyMaskCfg(const Osd::CoverConfig_S &src,std::size_t maxAreaCount, NET_PrivacyMaskCfg_S &dst)
{
    std::memset(&dst, 0, sizeof(dst));
    dst.bEnable = src.bEnable ? TRUE : FALSE;

    const size_t nSupportedCount = std::min(maxAreaCount, (size_t)NET_MAX_PRIVACY_MASK_AREA_NUM);
    const size_t nCount = std::min(src.vecCoverAttr.size(), nSupportedCount);
    dst.uAreaCount = (INT32)nCount;
    for (size_t i = 0; i < nCount; ++i)
    {
        const Osd::CoverAttribute_S &srcArea = src.vecCoverAttr[i];
        NET_PrivacyMaskArea_S &dstArea = dst.astArea[i];

        dstArea.nAreaID = (INT32)i;
        dstArea.bEnable = srcArea.bEnable ? TRUE : FALSE;
        dstArea.nRectLeft = (INT32)srcArea.nX;
        dstArea.nRectTop = (INT32)srcArea.nY;
        dstArea.nRectRight = (INT32)(srcArea.nX + srcArea.nWidth);
        dstArea.nRectBottom = (INT32)(srcArea.nY + srcArea.nHeight);
    }
}

bool ToPrivacyMaskCfg(const NET_PrivacyMaskCfg_S &src, std::size_t maxAreaCount, Osd::CoverConfig_S &dst)
{
    const size_t nSupportedCount = std::min(maxAreaCount, (size_t)NET_MAX_PRIVACY_MASK_AREA_NUM);
    if (src.uAreaCount < 0 || static_cast<size_t>(src.uAreaCount) > nSupportedCount)
    {
        return false;
    }

    dst.clear();
    dst.bEnable = (src.bEnable == TRUE);
    dst.vecCoverAttr.resize(nSupportedCount);

    const size_t nCount = static_cast<size_t>(src.uAreaCount);
    for (size_t i = 0; i < nCount; ++i)
    {
        const NET_PrivacyMaskArea_S &srcArea = src.astArea[i];
        Osd::CoverAttribute_S &dstArea = dst.vecCoverAttr[i];

        dstArea.nId = (int)i + 1;
        dstArea.bEnable = (srcArea.bEnable == TRUE);
        dstArea.strName = "遮挡区域" + std::to_string(i + 1);
        dstArea.enColor = Osd::OSD_COLOR_BLACK;
        dstArea.strColor = "#000000";
        dstArea.nX = (int)srcArea.nRectLeft;
        dstArea.nY = (int)srcArea.nRectTop;
        dstArea.nWidth = std::max(0, (int)(srcArea.nRectRight - srcArea.nRectLeft));
        dstArea.nHeight = std::max(0, (int)(srcArea.nRectBottom - srcArea.nRectTop));
    }
    
    return true;
}


void FillImageSetting(const ISP::ImageParam_S &src, NET_ImageSetting_S &dst)
{
    std::memset(&dst, 0, sizeof(dst));
    dst.nBrightness = (UINT32)src.nBrightness;
    dst.nContrast = (UINT32)src.nContrast;
    dst.nSaturation = (UINT32)src.nSaturation;
    dst.nSharpness = (UINT32)src.nSharpness;
}

void ToImageParam(const NET_ImageSetting_S &src, ISP::ImageParam_S &dst)
{
    dst.nBrightness = (unsigned int)src.nBrightness;
    dst.nContrast = (unsigned int)src.nContrast;
    dst.nSaturation = (unsigned int)src.nSaturation;
    dst.nSharpness = (unsigned int)src.nSharpness;
}

void FillPreviewInfo(const Preview::PreviewInfo_S &src, NET_PreviewInfo_S &dst)
{
    std::memset(&dst, 0, sizeof(dst));
    std::strncpy(dst.stRtspUrl.szRtspMainUrl, src.stRtspUrl.strRtspMainUrl.c_str(), sizeof(dst.stRtspUrl.szRtspMainUrl) - 1);
    std::strncpy(dst.stRtspUrl.szRtspSubUrl, src.stRtspUrl.strRtspSubUrl.c_str(), sizeof(dst.stRtspUrl.szRtspSubUrl) - 1);
    dst.stImageParam.nBrightness = src.stImageParam.nBrightness;
    dst.stImageParam.nContrast = src.stImageParam.nContrast;
    dst.stImageParam.nSaturation = src.stImageParam.nSaturation;
    dst.stImageParam.nSharpness = src.stImageParam.nSharpness;
}

void ToPreviewInfo(const NET_PreviewInfo_S &src, Preview::PreviewInfo_S &dst)
{
    dst.stRtspUrl.strRtspMainUrl = src.stRtspUrl.szRtspMainUrl;
    dst.stRtspUrl.strRtspSubUrl = src.stRtspUrl.szRtspSubUrl;
    dst.stImageParam.nBrightness = src.stImageParam.nBrightness;
    dst.stImageParam.nContrast = src.stImageParam.nContrast;
    dst.stImageParam.nSaturation = src.stImageParam.nSaturation;
    dst.stImageParam.nSharpness = src.stImageParam.nSharpness;
}

static void ParseResolutionName(const std::string &name, NET_VideoResolution_S &dst)
{
    int width = 0;
    int height = 0;
    if (std::sscanf(name.c_str(), "%d*%d", &width, &height) == 2 ||
        std::sscanf(name.c_str(), "%dx%d", &width, &height) == 2)
    {
        dst.uWidth = (INT32)width;
        dst.uHeight = (INT32)height;
    }
}

static void FillFrameRateList(FLOAT frameRateMin, FLOAT frameRateMax, NET_VideoResolution_S &dst)
{
    static const FLOAT kFrameRates[] = {
        1.0f / 16.0f, 1.0f / 8.0f, 1.0f / 4.0f, 1.0f / 2.0f,
        1.0f, 2.0f, 4.0f, 6.0f, 8.0f, 10.0f, 12.0f, 15.0f,
        16.0f, 18.0f, 20.0f, 22.0f, 25.0f, 30.0f
    };

    if (frameRateMin > frameRateMax)
    {
        const FLOAT tmp = frameRateMin;
        frameRateMin = frameRateMax;
        frameRateMax = tmp;
    }

    dst.uFrameRateNum = 0;
    for (size_t i = 0; i < sizeof(kFrameRates) / sizeof(kFrameRates[0]) &&
                       dst.uFrameRateNum < NET_VIDEO_FRAME_RATE_MAX_NUM; ++i)
    {
        if (kFrameRates[i] >= frameRateMin && kFrameRates[i] <= frameRateMax)
        {
            dst.afFrameRate[dst.uFrameRateNum++] = kFrameRates[i];
        }
    }
}

/**
 * @brief 将 IPC 侧 FrameRate_E 枚举值转换为 SDK 侧的浮点帧率值
 */
static FLOAT FrameRateEnumToFloat(Video_NS::FrameRate_E enFrameRate)
{
    switch (enFrameRate)
    {
        case Video_NS::FRAME_RATE_1_16: return 0.0625f;
        case Video_NS::FRAME_RATE_1_8:  return 0.125f;
        case Video_NS::FRAME_RATE_1_4:  return 0.25f;
        case Video_NS::FRAME_RATE_1_2:  return 0.5f;
        case Video_NS::FRAME_RATE_1:    return 1.0f;
        case Video_NS::FRAME_RATE_2:    return 2.0f;
        case Video_NS::FRAME_RATE_3:    return 3.0f;
        case Video_NS::FRAME_RATE_4:    return 4.0f;
        case Video_NS::FRAME_RATE_5:    return 5.0f;
        case Video_NS::FRAME_RATE_6:    return 6.0f;
        case Video_NS::FRAME_RATE_7:    return 7.0f;
        case Video_NS::FRAME_RATE_8:    return 8.0f;
        case Video_NS::FRAME_RATE_9:    return 9.0f;
        case Video_NS::FRAME_RATE_10:   return 10.0f;
        case Video_NS::FRAME_RATE_12:   return 12.0f;
        case Video_NS::FRAME_RATE_13:   return 13.0f;
        case Video_NS::FRAME_RATE_14:   return 14.0f;
        case Video_NS::FRAME_RATE_15:   return 15.0f;
        case Video_NS::FRAME_RATE_16:   return 16.0f;
        case Video_NS::FRAME_RATE_17:   return 17.0f;
        case Video_NS::FRAME_RATE_18:   return 18.0f;
        case Video_NS::FRAME_RATE_19:   return 19.0f;
        case Video_NS::FRAME_RATE_20:   return 20.0f;
        case Video_NS::FRAME_RATE_21:   return 21.0f;
        case Video_NS::FRAME_RATE_22:   return 22.0f;
        case Video_NS::FRAME_RATE_23:   return 23.0f;
        case Video_NS::FRAME_RATE_24:   return 24.0f;
        case Video_NS::FRAME_RATE_25:   return 25.0f;
        case Video_NS::FRAME_RATE_26:   return 26.0f;
        case Video_NS::FRAME_RATE_27:   return 27.0f;
        case Video_NS::FRAME_RATE_28:   return 28.0f;
        case Video_NS::FRAME_RATE_29:   return 29.0f;
        case Video_NS::FRAME_RATE_30:   return 30.0f;
        case Video_NS::FRAME_RATE_35:   return 35.0f;
        case Video_NS::FRAME_RATE_40:   return 40.0f;
        case Video_NS::FRAME_RATE_45:   return 45.0f;
        case Video_NS::FRAME_RATE_48:   return 48.0f;
        case Video_NS::FRAME_RATE_50:   return 50.0f;
        case Video_NS::FRAME_RATE_55:   return 55.0f;
        case Video_NS::FRAME_RATE_60:   return 60.0f;
        case Video_NS::FRAME_RATE_100:  return 100.0f;
        case Video_NS::FRAME_RATE_120:  return 120.0f;
        case Video_NS::FRAME_RATE_8_3:  return 8.3f;
        default:                        return 30.0f;
    }
}

/**
 * @brief 将 IPC 侧 Resolution_S 转换为 SDK 侧 NET_VideoResolution_S
 */
static void FillOneResolution(const Video_NS::Resolution_S &src, NET_VideoResolution_S &dst)
{
    std::memset(&dst, 0, sizeof(dst));
    std::strncpy(dst.szName, src.strName.c_str(), sizeof(dst.szName) - 1);
    ParseResolutionName(src.strName, dst);
    dst.fFrameRateMin = FrameRateEnumToFloat(src.enFrameRateMin);
    dst.fFrameRateMax = FrameRateEnumToFloat(src.enFrameRateMax);
    FillFrameRateList(dst.fFrameRateMin, dst.fFrameRateMax, dst);
    dst.uBitRateMin = (INT32)src.nBitRateMin;
    dst.uBitRateMax = (INT32)src.nBitRateMax;
}

static void FillOneEncodeAbility(const Video_NS::EncodeAbility_S &src, NET_VideoEncodeAbility_S &dst)
{
    std::memset(&dst, 0, sizeof(dst));
    std::strncpy(dst.szVideoCodec, src.strVideoCodec.c_str(), sizeof(dst.szVideoCodec) - 1);
    dst.enVideoCodec = ToSdkVideoCodec(Video_NS::string_toVideoCodec(src.strVideoCodec));
    dst.nSupportAdjustComplexity = (INT32)src.nSupportAdjustComplexity;
    dst.nEncodeComplexityNum = (INT32)std::min(src.vEncodeComplexity.size(), (size_t)NET_VIDEO_ENCODE_COMPLEXITY_MAX_NUM);
    for (INT32 i = 0; i < dst.nEncodeComplexityNum; ++i)
    {
        dst.anEncodeComplexity[i] = (INT32)src.vEncodeComplexity[(size_t)i];
    }
    dst.nDefaultComplexity = (UINT32)src.nDefaultComplexity;
    dst.bSupportSVC = (INT32)src.bSupportSVC;
    dst.bSupportStreamSmooth = (INT32)src.bSupportStreamSmooth;
}

static void FillOneEncodeOption(const Video_NS::VideoCapability_S &src,
                                const Video_NS::EncodeAbility_S *ability,
                                NET_VideoEncodeOption_S &dst,
                                INT32 streamType)
{
    std::memset(&dst, 0, sizeof(dst));
    dst.nId = streamType;
    dst.enVideoType = (INT32)Video_NS::VideoType_E::COMPOSITE_STREAM;
    dst.enBitrateType = (INT32)Video_NS::BitrateType_E::CBR;
    dst.enImageQuality = (INT32)Video_NS::ImageQuality_E::MEDIUM;

    if (!src.aResolution.empty())
    {
        ParseResolutionName(src.aResolution[0].strName, dst.stVideoResolution);
        dst.enFrameRate = (INT32)FrameRateEnumToFloat(src.aResolution[0].enFrameRateMax);
        dst.nAverageBitrate = (INT32)src.aResolution[0].nBitRateMin;
        dst.nBitrateUpperLimit = (INT32)src.aResolution[0].nBitRateMax;
    }

    if (ability)
    {
        dst.enVideoCodec = ToSdkVideoCodec(Video_NS::string_toVideoCodec(ability->strVideoCodec));
        dst.bSmartEnable = FALSE;
        dst.enEncodingComplexity = (INT32)ability->nDefaultComplexity;
        dst.enSvcEnable = ability->bSupportSVC ? Video_NS::SVC_MODE_ENABLE : Video_NS::SVC_MODE_DISABLE;
        dst.nBitrateSmoothing = ability->bSupportStreamSmooth ? src.nStreamSmoothMax : 0;
    }
    else
    {
        dst.enVideoCodec = NET_VIDEO_CODE_H264;
        dst.enEncodingComplexity = (INT32)Video_NS::EncodingComplexity_E::Main;
        dst.enSvcEnable = Video_NS::SVC_MODE_DISABLE;
    }

    dst.nIFrameInterval = src.nIFrameIntervalMax;
}

static void FillOneStreamCap(const Video_NS::VideoCapability_S &src, NET_VideoStreamCap_S &dst, INT32 streamType)
{
    memset(&dst, 0, sizeof(dst));
    dst.uStreamType = streamType;
    dst.bSupportMultiStream = (INT32)src.bSupportMultiStream;

    /* 编码能力列表 */
    dst.uEncodeTypeNum = src.nEncodeTypeNum;
    dst.uEncodeAbilityNum = (INT32)std::min(src.aEncodeAbility.size(), (size_t)NET_VIDEO_ENCODE_TYPE_MAX);
    if (dst.uEncodeTypeNum <= 0)
    {
        dst.uEncodeTypeNum = dst.uEncodeAbilityNum;
    }

    for (INT32 i = 0; i < dst.uEncodeAbilityNum; ++i)
    {
        FillOneEncodeAbility(src.aEncodeAbility[(size_t)i], dst.astEncodeAbility[i]);
    }

    dst.uEncodeCapSize = dst.uEncodeAbilityNum;
    if (dst.uEncodeCapSize == 0)
    {
        dst.uEncodeCapSize = 1;
        FillOneEncodeOption(src, nullptr, dst.astEncodeCap[0], streamType);
    }
    else
    {
        for (INT32 i = 0; i < dst.uEncodeCapSize; ++i)
        {
            FillOneEncodeOption(src, &src.aEncodeAbility[i], dst.astEncodeCap[i], streamType);
        }
    }

    /*
     * IPC 视频能力集只返回码流平滑范围，没有单独的图像质量能力范围。
     * 这里保留质量范围的兼容默认值，同时把真实的 StreamSmooth 范围填到对应字段。
     */
    dst.stQuality.uMin = 1;
    dst.stQuality.uMax = 100;
    dst.stStreamSmooth.uMin = src.nStreamSmoothMin;
    dst.stStreamSmooth.uMax = src.nStreamSmoothMax;
    dst.uIFrameIntervalMin = src.nIFrameIntervalMin;
    dst.uIFrameIntervalMax = src.nIFrameIntervalMax;

    /* 分辨率列表 */
    dst.uResolutionNum = (INT32)std::min(src.aResolution.size(), (size_t)NET_RESOLUTION_NUM_MAX);
    for (INT32 i = 0; i < dst.uResolutionNum; ++i)
    {
        FillOneResolution(src.aResolution[i], dst.astResolution[i]);
    }
}

void FillVideoEncodeCap(const Video_NS::VideoCapabilitySet_S &src, NET_VideoEncodeCap_S &dst)
{
    memset(&dst, 0, sizeof(dst));
    dst.uStreamCount = 0;
    FillOneStreamCap(src.stMain, dst.astStreamCap[0], NET_LIVE_STREAM_INDEX_MAIN);
    dst.uStreamCount++;
    FillOneStreamCap(src.stSub, dst.astStreamCap[1], NET_LIVE_STREAM_INDEX_AUX);
    dst.uStreamCount++;
}

static void FillOneAudioFormatCap(const Audio_NS::AudioFormatCapability_S &src,
                                  NET_AudioFormatCap_S &dst)
{
    memset(&dst, 0, sizeof(dst));

    dst.uFormat = (INT32)Audio_NS::string_toAudioFormat(src.strFormat);

    for (size_t i = 0; i < src.aSampleRates.size() && i < NET_AUDIO_SAMPRATE_MAX; ++i)
    {
        dst.auSampleRate[i] = (INT32)src.aSampleRates[i];
        dst.uSampleRateSize++;
    }

    for (size_t i = 0; i < src.aBitRates.size() && i < NET_AUDIO_BITRATE_MAX; ++i)
    {
        dst.auBitRate[i] = (INT32)src.aBitRates[i];
        dst.uBitRateSize++;
    }

    dst.stSampleRateRange.bEnable = src.stSampleRateRange.bEnable ? 1 : 0;
    dst.stSampleRateRange.uMin   = src.stSampleRateRange.nMin;
    dst.stSampleRateRange.uMax   = src.stSampleRateRange.nMax;
    dst.stSampleRateRange.uStep  = src.stSampleRateRange.nStep;

    dst.stBitRateRange.bEnable = src.stBitRateRange.bEnable ? 1 : 0;
    dst.stBitRateRange.uMin   = src.stBitRateRange.nMin;
    dst.stBitRateRange.uMax   = src.stBitRateRange.nMax;
    dst.stBitRateRange.uStep  = src.stBitRateRange.nStep;
}

void FillAudioEncodeCap(const Audio_NS::AudioCapabilitySet_S &src,
                        NET_AudioCap_S &dst)
{
    memset(&dst, 0, sizeof(dst));

    for (size_t i = 0; i < src.aInputTypes.size() && i < NET_AUDIO_INPUT_TYPE_MAX; ++i)
    {
        dst.auInputType[i] = (INT32)Audio_NS::string_toAudioInputType(src.aInputTypes[i]);
        dst.uInputTypeSize++;
    }

    for (size_t i = 0; i < src.aOutputTypes.size() && i < NET_AUDIO_OUTPUT_TYPE_MAX; ++i)
    {
        dst.auOutputType[i] = (INT32)Audio_NS::string_toAudioOutputType(src.aOutputTypes[i]);
        dst.uOutputTypeSize++;
    }

    for (size_t i = 0; i < src.aFormats.size() && i < NET_AUDIO_FORMAT_MAX; ++i)
    {
        dst.auFormat[i] = (INT32)Audio_NS::string_toAudioFormat(src.aFormats[i]);
        dst.uFormatSize++;
    }

    for (size_t i = 0; i < src.aFormatDetail.size() && i < NET_AUDIO_FORMAT_MAX; ++i)
    {
        FillOneAudioFormatCap(src.aFormatDetail[i], dst.astFormatDetail[i]);
        dst.uFormatDetailSize++;
    }
}

// --------- Motion (IPC MotionDetection_S <-> SDK NET_MotionAlarmInfo_S) ---------
void FillMotionAlarmInfo(const Alarm::MotionDetection_S &src, NET_MotionAlarmInfo_S &dst)
{
    memset(&dst, 0, sizeof(dst));
    dst.bEnable = src.bEnable ? TRUE : FALSE;
    dst.bDynamicAnalysisEnable = src.bDynamicAnalysisEnable ? TRUE : FALSE;
    dst.uMode = (INT32)src.enMode; // 0 normal, 1 expert

    // Normal mode
    dst.stNormalMode.nSensitivity = (INT32)src.stMotionNormalMode.nSensitivity;
    dst.stNormalMode.nRegionType  = (INT32)src.stMotionNormalMode.nRegionType;
    // NOTE:
    // 这里先做最小安全填充：不访问 IPC 侧 variant/grid 具体内容，避免在不同编译宏/ABI 组合下触发崩溃。
    // 若后续确认稳定，可再逐步恢复“矩形/网格区域”的精确映射。
    dst.stNormalMode.nRectLeft = 0;
    dst.stNormalMode.nRectTop = 0;
    dst.stNormalMode.nRectRight = 0;
    dst.stNormalMode.nRectBottom = 0;
    dst.stNormalMode.uGridWidth = 22;
    dst.stNormalMode.uGridHeight = 18;
    // 普通模式区域：筒型(Rect) 或 网格(abyGridArea)
    if (dst.stNormalMode.nRegionType == 0)
    {
        if (std::holds_alternative<Common::Rect_S>(src.stMotionNormalMode.varRegion))
        {
            const Common::Rect_S &r = std::get<Common::Rect_S>(src.stMotionNormalMode.varRegion);
            dst.stNormalMode.nRectLeft = r.nX;
            dst.stNormalMode.nRectTop = r.nY;
            dst.stNormalMode.nRectRight = r.nX + r.nWidth;
            dst.stNormalMode.nRectBottom = r.nY + r.nHeight;
        }
    }
    else if (dst.stNormalMode.nRegionType == 1)
    {
        // 默认全 0，只有网格中标记为 1 的宏块才置 1
        std::memset(dst.stNormalMode.abyGridArea, 0, sizeof(dst.stNormalMode.abyGridArea));

        if (std::holds_alternative<Alarm::MotionNormalMode_S::AreaGrid>(src.stMotionNormalMode.varRegion))
        {
            const auto &grid = std::get<Alarm::MotionNormalMode_S::AreaGrid>(src.stMotionNormalMode.varRegion);
            int h = (int)std::min<size_t>(grid.size(), 18);
            int w = 0;
            if (h > 0)
                w = (int)std::min<size_t>(grid[0].size(), 22);

            // 尝试按 IPC 网格实际尺寸填充；不足按 18x22 默认
            if (w > 0)
            {
                dst.stNormalMode.uGridHeight = h;
                dst.stNormalMode.uGridWidth = w;
                for (int y = 0; y < h; ++y)
                {
                    int rowW = (int)std::min<size_t>(grid[y].size(), (size_t)w);
                    for (int x = 0; x < rowW; ++x)
                    {
                        dst.stNormalMode.abyGridArea[y][x] = (grid[y][x] != 0) ? 1 : 0;
                    }
                }
            }
        }
    }

    // Expert mode: map first 16 regions
    dst.stExpertMode.nExpertDayNightCtrl = (INT32)src.stMotionExpertMode.nExpertDayNightCtrl;
    // stDayTime schedule: SDK uses NET_SchedTime_S, IPC uses Common::SchedTime_S, leave default
    if (!src.aAlarmTime.empty())
    {
        for (int day = 0; day < 7; day++)
        {
            if (day >= (int)src.aAlarmTime.size())
                break;
            const auto &vecDay = src.aAlarmTime[day];
            int cnt = (int)std::min<size_t>(vecDay.size(), NET_PLAN_SECTION_NUM);
            dst.stAlarmSchedule.uTimeSectionCount[day] = cnt;
            for (int seg = 0; seg < cnt; ++seg)
            {
                FillSchedTime(vecDay[seg], dst.stAlarmSchedule.astTimeSection[day][seg]);
            }
        }
    }
    dst.stExpertMode.uRegionCount = 0;
    for (size_t i = 0; i < src.stMotionExpertMode.vstMotionRegion.size() && i < 16; ++i)
    {
        const auto &reg = src.stMotionExpertMode.vstMotionRegion[i];
        auto &out = dst.stExpertMode.astRegion[i];
        memset(&out, 0, sizeof(out));
        out.nAreaNo = (INT32)reg.nAreaNo;
        out.nRectLeft   = reg.stRect.nX;
        out.nRectTop    = reg.stRect.nY;
        out.nRectRight  = reg.stRect.nX + reg.stRect.nWidth;
        out.nRectBottom = reg.stRect.nY + reg.stRect.nHeight;
        out.nCloseSensitivity   = (INT32)reg.nCloseSensitivity;
        out.nDaytimeSensitivity = (INT32)reg.nDaytimeSensitivity;
        out.nNightSensitivity   = (INT32)reg.nNightSensitivity;
        dst.stExpertMode.uRegionCount++;
    }
}

void ToMotionDetection(const NET_MotionAlarmInfo_S &src, Alarm::MotionDetection_S &dst)
{
    dst.bEnable = (src.bEnable == TRUE);
    dst.bDynamicAnalysisEnable = (src.bDynamicAnalysisEnable == TRUE);
    dst.enMode = (Alarm::MotionType_E)src.uMode;

    dst.stMotionNormalMode.nSensitivity = (unsigned int)src.stNormalMode.nSensitivity;
    dst.stMotionNormalMode.nRegionType  = (unsigned int)src.stNormalMode.nRegionType;

    // 只做最小安全回写：矩形区域可回写，网格区域暂不回写（保持 IPC 默认）
    if (dst.stMotionNormalMode.nRegionType == 0)
    {
        Common::Rect_S r;
        r.nX = src.stNormalMode.nRectLeft;
        r.nY = src.stNormalMode.nRectTop;
        r.nWidth  = src.stNormalMode.nRectRight - src.stNormalMode.nRectLeft;
        r.nHeight = src.stNormalMode.nRectBottom - src.stNormalMode.nRectTop;
        dst.stMotionNormalMode.varRegion = r;
    }
    else if (dst.stMotionNormalMode.nRegionType == 1)
    {
        // 网格区域：从 SDK abyGridArea 还原为 IPC AreaGrid（默认 18x22，按 dwGrid* 裁剪）
        int h = src.stNormalMode.uGridHeight;
        int w = src.stNormalMode.uGridWidth;
        if (h <= 0 || h > 18)
            h = 18;
        if (w <= 0 || w > 22)
            w = 22;

        Alarm::MotionNormalMode_S::AreaGrid grid((size_t)h, std::vector<unsigned int>((size_t)w, 0));
        for (int y = 0; y < h; ++y)
        {
            for (int x = 0; x < w; ++x)
            {
                grid[y][x] = (src.stNormalMode.abyGridArea[y][x] != 0) ? 1U : 0U;
            }
        }
        dst.stMotionNormalMode.varRegion = grid;
    }
    // 布防时间：用 SDK 一周×8 段回写 IPC 的 aAlarmTime（只填有值的时间段）
    dst.aAlarmTime.clear();
    dst.aAlarmTime.resize(7);
    for (int day = 0; day < 7; ++day)
    {
        int cnt = src.stAlarmSchedule.uTimeSectionCount[day];
        if (cnt <= 0)
            continue;
        cnt = std::min(cnt, NET_PLAN_SECTION_NUM);
        dst.aAlarmTime[day].resize(cnt);
        for (int seg = 0; seg < cnt; ++seg)
        {
            ToSchedTime(src.stAlarmSchedule.astTimeSection[day][seg], dst.aAlarmTime[day][seg]);
        }
    }
}

/* ---------- 安全服务与日志（465-472） ---------- */
void FillSecurityServicesInfo(const ::System::SecurityServices_S &src,
                              NET_SecurityServicesInfo_S &dst)
{
    std::memset(&dst, 0, sizeof(dst));
    dst.stLoginLock.bIllegalLoginEnable = src.stLoginLock.bIllegalLoginEnable ? TRUE : FALSE;
    dst.stLoginLock.nCheckInterval = src.stLoginLock.nCheckInterval;
    dst.stLoginLock.nMaxErrorTimes = src.stLoginLock.nMaxErrorTimes;
    dst.stLoginLock.nLockDuration = static_cast<INT32>(src.stLoginLock.nLockDuration);
    dst.stPwdPolicy.bPwdSecurityLevelEnable = src.stPwdPolicy.bPwdSecurityLevelEnable ? TRUE : FALSE;
    dst.stPwdPolicy.bAllowLowLevelPwdLogin = src.stPwdPolicy.bAllowLowLevelPwdLogin ? TRUE : FALSE;
    dst.stSshAdmin.bSshEnable = src.stSshAdmin.bSshEnable ? TRUE : FALSE;
    dst.stSshAdmin.nSshPort = src.stSshAdmin.nSshPort;
    copy_alarm_string(src.stSshAdmin.strSshStartTime, dst.stSshAdmin.szSshStartTime,
                      sizeof(dst.stSshAdmin.szSshStartTime));
    copy_alarm_string(src.stSshAdmin.strSshCountdown, dst.stSshAdmin.szSshCountdown,
                      sizeof(dst.stSshAdmin.szSshCountdown));
}

void ToSecurityServicesInfo(const NET_SecurityServicesInfo_S &src,
                            ::System::SecurityServices_S &dst)
{
    dst.stLoginLock.bIllegalLoginEnable = (src.stLoginLock.bIllegalLoginEnable == TRUE);
    dst.stLoginLock.nCheckInterval = src.stLoginLock.nCheckInterval;
    dst.stLoginLock.nMaxErrorTimes = src.stLoginLock.nMaxErrorTimes;
    dst.stLoginLock.nLockDuration = static_cast<::System::LockDuration_E>(src.stLoginLock.nLockDuration);
    dst.stPwdPolicy.bPwdSecurityLevelEnable = (src.stPwdPolicy.bPwdSecurityLevelEnable == TRUE);
    dst.stPwdPolicy.bAllowLowLevelPwdLogin = (src.stPwdPolicy.bAllowLowLevelPwdLogin == TRUE);
    dst.stSshAdmin.bSshEnable = (src.stSshAdmin.bSshEnable == TRUE);
    dst.stSshAdmin.nSshPort = src.stSshAdmin.nSshPort;
    dst.stSshAdmin.strSshStartTime = read_alarm_string(src.stSshAdmin.szSshStartTime,
                                                        sizeof(src.stSshAdmin.szSshStartTime));
    dst.stSshAdmin.strSshCountdown = read_alarm_string(src.stSshAdmin.szSshCountdown,
                                                        sizeof(src.stSshAdmin.szSshCountdown));
}

void FillSshCountdownInfo(const ::System::SshCountdown_S &src,
                          NET_SshCountdownInfo_S &dst)
{
    std::memset(&dst, 0, sizeof(dst));
    copy_alarm_string(src.strCountdown, dst.szCountdown, sizeof(dst.szCountdown));
}

void FillLogServerInfo(const ::System::LogServerInfo_S &src, NET_LogServerInfo_S &dst)
{
    std::memset(&dst, 0, sizeof(dst));
    dst.bEnable = src.bEnable ? TRUE : FALSE;
    dst.bEnSsl = src.bEnSsl ? TRUE : FALSE;
    dst.nPort = src.nPort;
    copy_alarm_string(src.strServerAddr, dst.szServerAddr, sizeof(dst.szServerAddr));
}

void ToLogServerInfo(const NET_LogServerInfo_S &src, ::System::LogServerInfo_S &dst)
{
    dst.bEnable = (src.bEnable == TRUE);
    dst.bEnSsl = (src.bEnSsl == TRUE);
    dst.nPort = src.nPort;
    dst.strServerAddr = read_alarm_string(src.szServerAddr, sizeof(src.szServerAddr));
}

void ToLogListRequest(const NET_LogList_S &src, Log::RetrievalCond_S &dstCond,
                      Common::PageInfo_S &dstPage)
{
    dstCond.enType = static_cast<Log::Type_E>(src.stCond.nType);
    dstCond.enAction = static_cast<Log::Action_E>(src.stCond.nAction);
    dstCond.startTime = read_alarm_string(src.stCond.szStartTime, sizeof(src.stCond.szStartTime));
    dstCond.endTime = read_alarm_string(src.stCond.szEndTime, sizeof(src.stCond.szEndTime));
    dstPage.nCurPage = src.stPage.nCurPage;
    dstPage.nPageSize = src.stPage.nPageSize;
}

void FillLogList(const Log::RetrievalCond_S &srcCond, const Common::PageInfo_S &srcPage,
                 const std::vector<Log::Info_S> &srcLogs, NET_LogList_S &dst)
{
    std::memset(&dst, 0, sizeof(dst));
    dst.stCond.nType = static_cast<INT32>(srcCond.enType);
    dst.stCond.nAction = static_cast<INT32>(srcCond.enAction);
    copy_alarm_string(srcCond.startTime, dst.stCond.szStartTime, sizeof(dst.stCond.szStartTime));
    copy_alarm_string(srcCond.endTime, dst.stCond.szEndTime, sizeof(dst.stCond.szEndTime));
    dst.stPage.nCurPage = srcPage.nCurPage;
    dst.stPage.nPageSize = srcPage.nPageSize;
    dst.stPage.nDataTotal = srcPage.nDataTotal;
    dst.stPage.nPageTotal = srcPage.nPageTotal;
    dst.nLogCount = static_cast<INT32>(std::min(srcLogs.size(), static_cast<size_t>(NET_LOG_QUERY_COND_NUM)));
    for (INT32 i = 0; i < dst.nLogCount; ++i)
    {
        const Log::Info_S &srcLog = srcLogs[static_cast<size_t>(i)];
        NET_LogInfo_S &dstLog = dst.astLogs[i];
        copy_alarm_string(srcLog.startTime, dstLog.szStartTime, sizeof(dstLog.szStartTime));
        dstLog.nType = srcLog.nType;
        dstLog.nAction = srcLog.nAction;
        copy_alarm_string(srcLog.chnName, dstLog.szChnName, sizeof(dstLog.szChnName));
        copy_alarm_string(srcLog.user, dstLog.szUser, sizeof(dstLog.szUser));
        copy_alarm_string(srcLog.host, dstLog.szHost, sizeof(dstLog.szHost));
        copy_alarm_string(srcLog.context, dstLog.szContext, sizeof(dstLog.szContext));
    }
}

/* ---------- 录像控制、计划、检索与下载（473-481） ---------- */
void ToRecordInfo(const NET_RecordInfo_S &src, Record_NS::Info_S &dst)
{
    dst.nChnId = src.nChnId;
    dst.nVideoStatus = src.nVideoStatus;
    dst.nAudioStatus = src.nAudioStatus;
    dst.nRecordStatus = src.nRecordStatus;
    dst.nRecordFormat = src.nRecordFormat;
    dst.nEventType = src.nEventType;
    dst.path = read_alarm_string(src.szPath, sizeof(src.szPath));
    dst.redunPath = read_alarm_string(src.szRedunPath, sizeof(src.szRedunPath));
    dst.strRecordName = read_alarm_string(src.szRecordName, sizeof(src.szRecordName));
    dst.strRecordTime = read_alarm_string(src.szRecordTime, sizeof(src.szRecordTime));
    dst.nStreamType = src.nStreamType;
}

void FillRecordStatusInfo(const Record_NS::RecordStatusInfo_S &src, NET_RecordStatusInfo_S &dst)
{
    std::memset(&dst, 0, sizeof(dst));
    dst.nStatus = static_cast<INT32>(src.enStatus);
}

void FillRecordSchedule(const Record_NS::Schedule_S &src, NET_RecordSchedule_S &dst)
{
    std::memset(&dst, 0, sizeof(dst));
    dst.bEnable = src.bEnable ? TRUE : FALSE;
    for (const Record_NS::DaySchedule_S &srcDay : src.daySchedules)
    {
        const INT32 nDay = static_cast<INT32>(srcDay.enDayOfWeek);
        if (nDay < 1 || nDay > NET_PLAN_DAY_NUM_AWEEK)
            continue;
        /* ABI 数组固定按周一至周日下标存放，不依赖 IPC vector 的原始排序。 */
        NET_RecordDaySchedule_S &dstDay = dst.astDaySchedules[nDay - 1];
        dstDay.nDayOfWeek = nDay;
        dstDay.nRecordTimeCount = static_cast<INT32>(std::min(srcDay.recordTimes.size(),
                                                               static_cast<size_t>(NET_TIME_DURATION_NUM)));
        for (INT32 i = 0; i < dstDay.nRecordTimeCount; ++i)
        {
            const Record_NS::RecordTime_S &srcTime = srcDay.recordTimes[static_cast<size_t>(i)];
            dstDay.astRecordTimes[i].nType = srcTime.nType;
            dstDay.astRecordTimes[i].nStartTime = srcTime.nStartTime;
            dstDay.astRecordTimes[i].nEndTime = srcTime.nEndTime;
        }
    }
    dst.nDayScheduleCount = static_cast<INT32>(std::min(src.daySchedules.size(),
                                                         static_cast<size_t>(NET_PLAN_DAY_NUM_AWEEK)));
}

void ToRecordSchedule(const NET_RecordSchedule_S &src, Record_NS::Schedule_S &dst)
{
    dst.bEnable = (src.bEnable == TRUE);
    dst.daySchedules.clear();
    const INT32 nDayCount = std::max<INT32>(0, std::min<INT32>(src.nDayScheduleCount, NET_PLAN_DAY_NUM_AWEEK));
    for (INT32 i = 0; i < nDayCount; ++i)
    {
        const NET_RecordDaySchedule_S &srcDay = src.astDaySchedules[i];
        INT32 nDay = srcDay.nDayOfWeek;
        /* 兼容旧客户端未填写 nDayOfWeek 的情况，按数组下标补齐星期。 */
        if (nDay < 1 || nDay > NET_PLAN_DAY_NUM_AWEEK)
            nDay = i + 1;
        Record_NS::DaySchedule_S dstDay;
        dstDay.enDayOfWeek = static_cast<Record_NS::DayOfWeek_E>(nDay);
        const INT32 nTimeCount = std::max<INT32>(0, std::min<INT32>(srcDay.nRecordTimeCount, NET_TIME_DURATION_NUM));
        dstDay.recordTimes.reserve(static_cast<size_t>(nTimeCount));
        for (INT32 j = 0; j < nTimeCount; ++j)
        {
            Record_NS::RecordTime_S dstTime;
            dstTime.nType = srcDay.astRecordTimes[j].nType;
            dstTime.nStartTime = srcDay.astRecordTimes[j].nStartTime;
            dstTime.nEndTime = srcDay.astRecordTimes[j].nEndTime;
            dstDay.recordTimes.push_back(dstTime);
        }
        dst.daySchedules.push_back(dstDay);
    }
}

void FillRecordAdvancedParam(const Record_NS::AdvancedParam_S &src, NET_RecordAdvancedParam_S &dst)
{
    std::memset(&dst, 0, sizeof(dst));
    dst.bLoopWrite = src.bLoopWrite ? TRUE : FALSE;
    dst.nPreTime = static_cast<INT32>(src.ePreTime);
    dst.nDelayTime = static_cast<INT32>(src.eDelayTime);
    dst.nStreamType = src.nStreamType;
}

void ToRecordAdvancedParam(const NET_RecordAdvancedParam_S &src, Record_NS::AdvancedParam_S &dst)
{
    dst.bLoopWrite = (src.bLoopWrite == TRUE);
    dst.ePreTime = static_cast<Record_NS::RecordPreTime_E>(src.nPreTime);
    dst.eDelayTime = static_cast<Record_NS::RecordDelayTime_E>(src.nDelayTime);
    dst.nStreamType = src.nStreamType;
}

void ToRecordFind(const NET_RecordFileList_S &src, Record_NS::Find_S &dst)
{
    dst.nChnId = src.stFind.nChnId;
    dst.nType = src.stFind.nType;
    dst.year = read_alarm_string(src.stFind.szYear, sizeof(src.stFind.szYear));
    dst.month = read_alarm_string(src.stFind.szMonth, sizeof(src.stFind.szMonth));
    dst.date = read_alarm_string(src.stFind.szDate, sizeof(src.stFind.szDate));
    dst.startTime = read_alarm_string(src.stFind.szStartTime, sizeof(src.stFind.szStartTime));
    dst.endTime = read_alarm_string(src.stFind.szEndTime, sizeof(src.stFind.szEndTime));
    dst.filename = read_alarm_string(src.stFind.szFilename, sizeof(src.stFind.szFilename));
}

void FillRecordFileList(const Record_NS::Find_S &srcFind,
                        const std::vector<Record_NS::FindResult_S> &srcResults,
                        NET_RecordFileList_S &dst)
{
    std::memset(&dst, 0, sizeof(dst));
    dst.stFind.nChnId = srcFind.nChnId;
    dst.stFind.nType = srcFind.nType;
    copy_alarm_string(srcFind.year, dst.stFind.szYear, sizeof(dst.stFind.szYear));
    copy_alarm_string(srcFind.month, dst.stFind.szMonth, sizeof(dst.stFind.szMonth));
    copy_alarm_string(srcFind.date, dst.stFind.szDate, sizeof(dst.stFind.szDate));
    copy_alarm_string(srcFind.startTime, dst.stFind.szStartTime, sizeof(dst.stFind.szStartTime));
    copy_alarm_string(srcFind.endTime, dst.stFind.szEndTime, sizeof(dst.stFind.szEndTime));
    copy_alarm_string(srcFind.filename, dst.stFind.szFilename, sizeof(dst.stFind.szFilename));
    /* SDK 公开数组容量有限，超过部分不返回，避免写越界。 */
    dst.nResultCount = static_cast<INT32>(std::min(srcResults.size(), static_cast<size_t>(NET_RECORD_FILE_MAX_NUM)));
    for (INT32 i = 0; i < dst.nResultCount; ++i)
    {
        const Record_NS::FindResult_S &srcResult = srcResults[static_cast<size_t>(i)];
        NET_RecordFindResult_S &dstResult = dst.astResults[i];
        dstResult.nChnId = srcResult.nChnId;
        dstResult.nDateCount = static_cast<INT32>(std::min(srcResult.dates.size(), static_cast<size_t>(NET_RECORD_DATE_MAX_NUM)));
        for (INT32 j = 0; j < dstResult.nDateCount; ++j)
            copy_alarm_string(srcResult.dates[static_cast<size_t>(j)], dstResult.aszDates[j], sizeof(dstResult.aszDates[j]));
        copy_alarm_string(srcResult.filename, dstResult.szFilename, sizeof(dstResult.szFilename));
        dstResult.nVideoTimeCount = static_cast<INT32>(std::min(srcResult.videoTimes.size(), static_cast<size_t>(NET_TIME_DURATION_NUM)));
        for (INT32 j = 0; j < dstResult.nVideoTimeCount; ++j)
        {
            dstResult.astVideoTimes[j].nStartTime = srcResult.videoTimes[static_cast<size_t>(j)].nStartTime;
            dstResult.astVideoTimes[j].nEndTime = srcResult.videoTimes[static_cast<size_t>(j)].nEndTime;
        }
    }
}

void ToRecordDownloadList(const NET_RecordDownloadList_S &src,
                          std::vector<Record_NS::DownloadInfo_S> &dst)
{
    dst.clear();
    /* 输入计数不可信，强制收敛到 ABI 数组边界。 */
    const INT32 nCount = std::max<INT32>(0, std::min<INT32>(src.nDownloadCount, NET_RECORD_DOWNLOAD_MAX_NUM));
    dst.reserve(static_cast<size_t>(nCount));
    for (INT32 i = 0; i < nCount; ++i)
    {
        const NET_RecordDownloadInfo_S &srcInfo = src.astDownloads[i];
        Record_NS::DownloadInfo_S dstInfo;
        dstInfo.nChnId = srcInfo.nChnId;
        dstInfo.path = read_alarm_string(srcInfo.szPath, sizeof(srcInfo.szPath));
        dstInfo.startTime = read_alarm_string(srcInfo.szStartTime, sizeof(srcInfo.szStartTime));
        dstInfo.endTime = read_alarm_string(srcInfo.szEndTime, sizeof(srcInfo.szEndTime));
        dst.push_back(dstInfo);
    }
}

void FillRecordDownloadProgress(const Record_NS::DownloadProgress_S &src,
                                NET_RecordDownloadProgress_S &dst)
{
    std::memset(&dst, 0, sizeof(dst));
    dst.nProgress = src.nProgress;
    copy_alarm_string(src.filename, dst.szFilename, sizeof(dst.szFilename));
}

/* ---------- 人脸比对、目标库与人员信息（482-490） ---------- */
void FillFaceCompareInfo(const Alarm::FaceCompare_S &src, NET_FaceCompareInfo_S &dst)
{
    std::memset(&dst, 0, sizeof(dst));
    dst.bEnable = src.bEnable ? TRUE : FALSE;
    fill_alarm_schedule(src.aAlarmTime, dst.stAlarmSchedule);
    FillLinkageList(src.stLinkageListSuccess, dst.stLinkageListSuccess);
    FillLinkageList(src.stLinkageListFail, dst.stLinkageListFail);
}

void ToFaceCompareInfo(const NET_FaceCompareInfo_S &src, Alarm::FaceCompare_S &dst)
{
    /* TargetLibInfos 不在公开 ABI 中，故意不触碰，调用方负责先加载现有配置。 */
    dst.bEnable = (src.bEnable == TRUE);
    to_alarm_schedule(src.stAlarmSchedule, dst.aAlarmTime);
    ToLinkageList(src.stLinkageListSuccess, dst.stLinkageListSuccess);
    ToLinkageList(src.stLinkageListFail, dst.stLinkageListFail);
}

void ToFaceLibInfo(const NET_FaceLibInfo_S &src, Event::FaceLibInfo_S &dst)
{
    dst.strFaceLibName = read_alarm_string(src.szFaceLibName, sizeof(src.szFaceLibName));
    dst.nTotalFace = src.nTotalFace;
    dst.nNormalNum = src.nNormalNum;
    dst.nAbnormalNum = src.nAbnormalNum;
}

void FillFaceLibList(const std::vector<Event::FaceLibInfo_S> &src, NET_FaceLibList_S &dst)
{
    std::memset(&dst, 0, sizeof(dst));
    dst.nTargetLibCount = static_cast<INT32>(std::min(src.size(), static_cast<size_t>(NET_FACE_LIB_MAX_NUM)));
    for (INT32 i = 0; i < dst.nTargetLibCount; ++i)
    {
        const Event::FaceLibInfo_S &srcInfo = src[static_cast<size_t>(i)];
        NET_FaceLibInfo_S &dstInfo = dst.astTargetLibInfos[i];
        copy_alarm_string(srcInfo.strFaceLibName, dstInfo.szFaceLibName, sizeof(dstInfo.szFaceLibName));
        dstInfo.nTotalFace = srcInfo.nTotalFace;
        dstInfo.nNormalNum = srcInfo.nNormalNum;
        dstInfo.nAbnormalNum = srcInfo.nAbnormalNum;
    }
}

void ToFaceIdInfo(const NET_FaceIdInfo_S &src, Event::FaceIdInfo_S &dst)
{
    const INT32 nCount = std::max<INT32>(0, std::min<INT32>(src.nIdCount, NET_FACE_ID_MAX_NUM));
    dst.ids.assign(src.anIds, src.anIds + nCount);
}

void ToFaceInfo(const NET_FaceInfo_S &src, Event::FaceInfo_S &dst)
{
    dst.nId = src.nId;
    dst.strFaceLibName = read_alarm_string(src.szFaceLibName, sizeof(src.szFaceLibName));
    dst.strName = read_alarm_string(src.szName, sizeof(src.szName));
    dst.strPhoneNum = read_alarm_string(src.szPhoneNum, sizeof(src.szPhoneNum));
    dst.strPicPath = read_alarm_string(src.szPicPath, sizeof(src.szPicPath));
    dst.BinPath = read_alarm_string(src.szBinPath, sizeof(src.szBinPath));
    dst.strPicType = read_alarm_string(src.szPicType, sizeof(src.szPicType));
    dst.nPicSize = src.nPicSize;
    dst.strPicDate = read_alarm_string(src.szPicDate, sizeof(src.szPicDate));
    dst.nModelState = src.nModelState;
    dst.nRatingLevel = src.nRatingLevel;
}

void FillFaceInfoList(const std::vector<Event::FaceInfo_S> &src, NET_FaceInfoList_S &dst)
{
    std::memset(&dst, 0, sizeof(dst));
    /* 人脸库可能大于单次 SDK 返回容量，只返回 ABI 可承载的前 N 项。 */
    dst.nFaceInfoCount = static_cast<INT32>(std::min(src.size(), static_cast<size_t>(NET_FACE_INFO_MAX_NUM)));
    for (INT32 i = 0; i < dst.nFaceInfoCount; ++i)
    {
        const Event::FaceInfo_S &srcInfo = src[static_cast<size_t>(i)];
        NET_FaceInfo_S &dstInfo = dst.astFaceInfos[i];
        dstInfo.nId = srcInfo.nId;
        copy_alarm_string(srcInfo.strFaceLibName, dstInfo.szFaceLibName, sizeof(dstInfo.szFaceLibName));
        copy_alarm_string(srcInfo.strName, dstInfo.szName, sizeof(dstInfo.szName));
        copy_alarm_string(srcInfo.strPhoneNum, dstInfo.szPhoneNum, sizeof(dstInfo.szPhoneNum));
        copy_alarm_string(srcInfo.strPicPath, dstInfo.szPicPath, sizeof(dstInfo.szPicPath));
        copy_alarm_string(srcInfo.BinPath, dstInfo.szBinPath, sizeof(dstInfo.szBinPath));
        copy_alarm_string(srcInfo.strPicType, dstInfo.szPicType, sizeof(dstInfo.szPicType));
        dstInfo.nPicSize = srcInfo.nPicSize;
        copy_alarm_string(srcInfo.strPicDate, dstInfo.szPicDate, sizeof(dstInfo.szPicDate));
        dstInfo.nModelState = srcInfo.nModelState;
        dstInfo.nRatingLevel = srcInfo.nRatingLevel;
    }
}

// --------- Tamper (IPC HideAlarm_S <-> SDK NET_TamperAlarmInfo_S) ---------
void FillTamperAlarmInfo(const Alarm::HideAlarm_S &src, NET_TamperAlarmInfo_S &dst)
{
     memset(&dst, 0, sizeof(dst));
    dst.bEnable = src.bEnable ? TRUE : FALSE;
    dst.uSensitivity = (INT32)src.nSensitivity;
    dst.nRectLeft   = src.stRect.nX;
    dst.nRectTop    = src.stRect.nY;
    dst.nRectRight  = src.stRect.nX + src.stRect.nWidth;
    dst.nRectBottom = src.stRect.nY + src.stRect.nHeight;

    // 布防时间：一周 × 最多 8 段
    if (!src.aAlarmTime.empty())
    {
        for (int day = 0; day < 7; day++)
        {
            if (day >= (int)src.aAlarmTime.size())
                break;
            const auto &vecDay = src.aAlarmTime[day];
            int cnt = (int)std::min<size_t>(vecDay.size(), NET_PLAN_SECTION_NUM);
            dst.stAlarmSchedule.uTimeSectionCount[day] = cnt;
            for (int seg = 0; seg < cnt; ++seg)
            {
                FillSchedTime(vecDay[seg], dst.stAlarmSchedule.astTimeSection[day][seg]);
            }
        }
    }
}

void ToHideAlarm(const NET_TamperAlarmInfo_S &src, Alarm::HideAlarm_S &dst)
{
    dst.bEnable = (src.bEnable == TRUE);
    dst.nSensitivity = (unsigned int)src.uSensitivity;
    dst.stRect.nX = src.nRectLeft;
    dst.stRect.nY = src.nRectTop;
    dst.stRect.nWidth  = src.nRectRight - src.nRectLeft;
    dst.stRect.nHeight = src.nRectBottom - src.nRectTop;

    // 布防时间回写
    dst.aAlarmTime.clear();
    dst.aAlarmTime.resize(7);
    for (int day = 0; day < 7; ++day)
    {
        int cnt = src.stAlarmSchedule.uTimeSectionCount[day];
        if (cnt <= 0)
            continue;
        cnt = std::min(cnt, NET_PLAN_SECTION_NUM);
        dst.aAlarmTime[day].resize(cnt);
        for (int seg = 0; seg < cnt; ++seg)
        {
            ToSchedTime(src.stAlarmSchedule.astTimeSection[day][seg], dst.aAlarmTime[day][seg]);
        }
    }
}

// --------- CrossLine (IPC BoundaryDetection_S <-> SDK NET_CrossLineAlarmInfo_S) ---------
void FillCrossLineAlarmInfo(const Alarm::BoundaryDetection_S &src, NET_CrossLineAlarmInfo_S &dst)
{
    memset(&dst, 0, sizeof(dst));
    dst.bEnable = src.bEnable ? TRUE : FALSE;
    dst.uRuleCount = 0;
    for (size_t i = 0; i < src.aRule.size() && i < 4; ++i)
    {
        const auto &r = src.aRule[i];
        auto &out = dst.stRule[i];
        memset(&out, 0, sizeof(out));
        out.bEnable = TRUE;
        out.fStartPosX = r.stStartPos.fX;
        out.fStartPosY = r.stStartPos.fY;
        out.fEndPosX   = r.stEndPos.fX;
        out.fEndPosY   = r.stEndPos.fY;
        out.enCrossDirection = (INT32)r.enCrossDirection;
        out.nSensitivity = (INT32)r.nSensitivity;
        FillDetectionTargets(r.aDetectionTarget, out.uDetectionTargetCount, out.auDetectionTarget);
        dst.uRuleCount++;
    }

     // 布防时间
    if (!src.aAlarmTime.empty())
    {
        for (int day = 0; day < 7; ++day)
        {
            if (day >= (int)src.aAlarmTime.size())
                break;
            const auto &vecDay = src.aAlarmTime[day];
            int cnt = (int)std::min<size_t>(vecDay.size(), NET_PLAN_SECTION_NUM);
            dst.stAlarmSchedule.uTimeSectionCount[day] = cnt;
            for (int seg = 0; seg < cnt; ++seg)
            {
                FillSchedTime(vecDay[seg], dst.stAlarmSchedule.astTimeSection[day][seg]);
            }
        }
    }
}

void ToBoundaryDetection(const NET_CrossLineAlarmInfo_S &src, Alarm::BoundaryDetection_S &dst)
{
    dst.bEnable = (src.bEnable == TRUE);
    dst.aRule.clear();
    for (int i = 0; i < src.uRuleCount && i < 4; ++i)
    {
        const auto &r = src.stRule[i];
        Alarm::BoundaryPlane_S out;
        out.stStartPos = { r.fStartPosX, r.fStartPosY };
        out.stEndPos   = { r.fEndPosX, r.fEndPosY };
        out.enCrossDirection = (Alarm::CrossDirection_E)r.enCrossDirection;
        out.nSensitivity = (unsigned int)r.nSensitivity;
        ToDetectionTargets(r.auDetectionTarget, r.uDetectionTargetCount, out.aDetectionTarget);
        dst.aRule.push_back(out);
    }

    // 布防时间
    dst.aAlarmTime.clear();
    dst.aAlarmTime.resize(7);
    for (int day = 0; day < 7; ++day)
    {
        int cnt = src.stAlarmSchedule.uTimeSectionCount[day];
        if (cnt <= 0)
            continue;
        cnt = std::min(cnt, NET_PLAN_SECTION_NUM);
        dst.aAlarmTime[day].resize(cnt);
        for (int seg = 0; seg < cnt; ++seg)
        {
            ToSchedTime(src.stAlarmSchedule.astTimeSection[day][seg], dst.aAlarmTime[day][seg]);
        }
    }
}

// --------- Intrusion (IPC FieldDetection_S <-> SDK NET_IntrusionAlarmInfo_S) ---------
void FillIntrusionAlarmInfo(const Alarm::FieldDetection_S &src, NET_IntrusionAlarmInfo_S &dst)
{
    memset(&dst, 0, sizeof(dst));
    dst.bEnable = src.bEnable ? TRUE : FALSE;
    dst.uRuleCount = 0;
    for (size_t i = 0; i < src.aRule.size() && i < 4; ++i)
    {
        const auto &r = src.aRule[i];
        auto &out = dst.stRule[i];
        memset(&out, 0, sizeof(out));
        out.bEnable = TRUE;
        out.uPointCount = (INT32)std::min<size_t>(r.stRegion.aPoint.size(), 32);
        for (int p = 0; p < out.uPointCount; ++p)
        {
            out.afPointX[p] = r.stRegion.aPoint[p].fX;
            out.afPointY[p] = r.stRegion.aPoint[p].fY;
        }
        out.nTimeThreshold = (INT32)r.nTimeThreshold;
        out.nSensitivity   = (INT32)r.nSensitivity;
        FillDetectionTargets(r.aDetectionTarget, out.uDetectionTargetCount, out.auDetectionTarget);
        dst.uRuleCount++;
    }

    // 布防时间
    if (!src.aAlarmTime.empty())
    {
        for (int day = 0; day < 7; ++day)
        {
            if (day >= (int)src.aAlarmTime.size())
                break;
            const auto &vecDay = src.aAlarmTime[day];
            int cnt = (int)std::min<size_t>(vecDay.size(), NET_PLAN_SECTION_NUM);
            dst.stAlarmSchedule.uTimeSectionCount[day] = cnt;
            for (int seg = 0; seg < cnt; ++seg)
            {
                FillSchedTime(vecDay[seg], dst.stAlarmSchedule.astTimeSection[day][seg]);
            }
        }
    }
}

void ToFieldDetection(const NET_IntrusionAlarmInfo_S &src, Alarm::FieldDetection_S &dst)
{
    dst.bEnable = (src.bEnable == TRUE);
    dst.aRule.clear();
    for (int i = 0; i < src.uRuleCount && i < 4; ++i)
    {
        const auto &r = src.stRule[i];
        Alarm::Intrusion_S out;
        out.nTimeThreshold = (unsigned int)r.nTimeThreshold;
        out.nSensitivity = (unsigned int)r.nSensitivity;
        out.stRegion.aPoint.clear();
        out.stRegion.nPointNum = r.uPointCount;
        for (int p = 0; p < r.uPointCount && p < 32; ++p)
        {
            out.stRegion.aPoint.push_back({ r.afPointX[p], r.afPointY[p] });
        }
        ToDetectionTargets(r.auDetectionTarget, r.uDetectionTargetCount, out.aDetectionTarget);
        dst.aRule.push_back(out);
    }

    // 布防时间
    dst.aAlarmTime.clear();
    dst.aAlarmTime.resize(7);
    for (int day = 0; day < 7; ++day)
    {
        int cnt = src.stAlarmSchedule.uTimeSectionCount[day];
        if (cnt <= 0)
            continue;
        cnt = std::min(cnt, NET_PLAN_SECTION_NUM);
        dst.aAlarmTime[day].resize(cnt);
        for (int seg = 0; seg < cnt; ++seg)
        {
            ToSchedTime(src.stAlarmSchedule.astTimeSection[day][seg], dst.aAlarmTime[day][seg]);
        }
    }
}

// --------- Loitering (IPC LoiteringDetection_S <-> SDK NET_LoiteringAlarmInfo_S) ---------
void FillLoiteringAlarmInfo(const Alarm::LoiteringDetection_S &src, NET_LoiteringAlarmInfo_S &dst)
{
    memset(&dst, 0, sizeof(dst));
    dst.bEnable = src.bEnable ? TRUE : FALSE;
    dst.uRuleCount = 0;
    for (size_t i = 0; i < src.aRule.size() && i < 4; ++i)
    {
        const auto &r = src.aRule[i];
        auto &out = dst.stRule[i];
        memset(&out, 0, sizeof(out));
        out.bEnable = TRUE;
        out.uPointCount = (INT32)std::min<size_t>(r.stRegion.aPoint.size(), 32);
        for (int p = 0; p < out.uPointCount; ++p)
        {
            out.afPointX[p] = r.stRegion.aPoint[p].fX;
            out.afPointY[p] = r.stRegion.aPoint[p].fY;
        }
        out.nTimeThreshold = (INT32)r.nTimeThreshold;
        out.nSensitivity   = (INT32)r.nSensitivity;
        FillDetectionTargets(r.aDetectionTarget, out.uDetectionTargetCount, out.auDetectionTarget);
        dst.uRuleCount++;
    }

    // 布防时间
    if (!src.aAlarmTime.empty())
    {
        for (int day = 0; day < 7; ++day)
        {
            if (day >= (int)src.aAlarmTime.size())
                break;
            const auto &vecDay = src.aAlarmTime[day];
            int cnt = (int)std::min<size_t>(vecDay.size(), NET_PLAN_SECTION_NUM);
            dst.stAlarmSchedule.uTimeSectionCount[day] = cnt;
            for (int seg = 0; seg < cnt; ++seg)
            {
                FillSchedTime(vecDay[seg], dst.stAlarmSchedule.astTimeSection[day][seg]);
            }
        }
    }
}

void ToLoiteringDetection(const NET_LoiteringAlarmInfo_S &src, Alarm::LoiteringDetection_S &dst)
{
    dst.bEnable = (src.bEnable == TRUE);
    dst.aRule.clear();
    for (int i = 0; i < src.uRuleCount && i < 4; ++i)
    {
        const auto &r = src.stRule[i];
        Alarm::LoiteringRule_S out;
        out.nTimeThreshold = (unsigned int)r.nTimeThreshold;
        out.nSensitivity = (unsigned int)r.nSensitivity;
        out.stRegion.aPoint.clear();
        out.stRegion.nPointNum = r.uPointCount;
        for (int p = 0; p < r.uPointCount && p < 32; ++p)
        {
            out.stRegion.aPoint.push_back({ r.afPointX[p], r.afPointY[p] });
        }
        ToDetectionTargets(r.auDetectionTarget, r.uDetectionTargetCount, out.aDetectionTarget);
        dst.aRule.push_back(out);
    }

    // 布防时间
    dst.aAlarmTime.clear();
    dst.aAlarmTime.resize(7);
    for (int day = 0; day < 7; ++day)
    {
        int cnt = src.stAlarmSchedule.uTimeSectionCount[day];
        if (cnt <= 0)
            continue;
        cnt = std::min(cnt, NET_PLAN_SECTION_NUM);
        dst.aAlarmTime[day].resize(cnt);
        for (int seg = 0; seg < cnt; ++seg)
        {
            ToSchedTime(src.stAlarmSchedule.astTimeSection[day][seg], dst.aAlarmTime[day][seg]);
        }
    }
}

static void FillPolygonPoints(const Alarm::Region_S &src, INT32 &pointCount, FLOAT pointX[32], FLOAT pointY[32])
{
    pointCount = (INT32)std::min<size_t>(src.aPoint.size(), 32);
    for (int p = 0; p < pointCount; ++p)
    {
        pointX[p] = src.aPoint[p].fX;
        pointY[p] = src.aPoint[p].fY;
    }
}

static void ToRegionFromPolygon(INT32 pointCount, const FLOAT pointX[32], const FLOAT pointY[32], Alarm::Region_S &dst)
{
    int cnt = pointCount;
    if (cnt < 0)
        cnt = 0;
    if (cnt > 32)
        cnt = 32;

    dst.aPoint.clear();
    dst.nPointNum = (unsigned int)cnt;
    for (int p = 0; p < cnt; ++p)
    {
        Common::PosF_S pt;
        pt.fX = pointX[p];
        pt.fY = pointY[p];
        dst.aPoint.push_back(pt);
    }
}

// --------- SceneChange (IPC SceneChange_S <-> SDK NET_SceneChangeAlarmInfo_S) ---------
void FillSceneChangeAlarmInfo(const Alarm::SceneChange_S &src, NET_SceneChangeAlarmInfo_S &dst)
{
    std::memset(&dst, 0, sizeof(dst));
    dst.bEnable = src.bEnable ? TRUE : FALSE;
    dst.nSensitivity = (INT32)src.nSensitivity;

    if (!src.aAlarmTime.empty())
    {
        for (int day = 0; day < 7; ++day)
        {
            if (day >= (int)src.aAlarmTime.size())
                break;
            const auto &vecDay = src.aAlarmTime[day];
            int cnt = (int)std::min<size_t>(vecDay.size(), NET_PLAN_SECTION_NUM);
            dst.stAlarmSchedule.uTimeSectionCount[day] = cnt;
            for (int seg = 0; seg < cnt; ++seg)
            {
                FillSchedTime(vecDay[seg], dst.stAlarmSchedule.astTimeSection[day][seg]);
            }
        }
    }
}

void ToSceneChange(const NET_SceneChangeAlarmInfo_S &src, Alarm::SceneChange_S &dst)
{
    dst.bEnable = (src.bEnable == TRUE);
    dst.nSensitivity = (unsigned int)src.nSensitivity;

    dst.aAlarmTime.clear();
    dst.aAlarmTime.resize(7);
    for (int day = 0; day < 7; ++day)
    {
        int cnt = src.stAlarmSchedule.uTimeSectionCount[day];
        if (cnt <= 0)
            continue;
        cnt = std::min(cnt, NET_PLAN_SECTION_NUM);
        dst.aAlarmTime[day].resize(cnt);
        for (int seg = 0; seg < cnt; ++seg)
        {
            ToSchedTime(src.stAlarmSchedule.astTimeSection[day][seg], dst.aAlarmTime[day][seg]);
        }
    }
}

// --------- CrowdGathering (IPC CrowdGathering_S <-> SDK NET_CrowdGatheringAlarmInfo_S) ---------
void FillCrowdGatheringAlarmInfo(const Alarm::CrowdGathering_S &src, NET_CrowdGatheringAlarmInfo_S &dst)
{
    std::memset(&dst, 0, sizeof(dst));
    dst.bEnable = src.bEnable ? TRUE : FALSE;
    dst.uRuleCount = 0;
    for (size_t i = 0; i < src.aRule.size() && i < 4; ++i)
    {
        const auto &r = src.aRule[i];
        auto &out = dst.astRule[i];
        std::memset(&out, 0, sizeof(out));
        out.bEnable = TRUE;
        FillPolygonPoints(r.stRegion, out.uPointCount, out.afPointX, out.afPointY);
        out.nObjectOccup = (INT32)r.nObjectOccup;
        dst.uRuleCount++;
    }

    if (!src.aAlarmTime.empty())
    {
        for (int day = 0; day < 7; ++day)
        {
            if (day >= (int)src.aAlarmTime.size())
                break;
            const auto &vecDay = src.aAlarmTime[day];
            int cnt = (int)std::min<size_t>(vecDay.size(), NET_PLAN_SECTION_NUM);
            dst.stAlarmSchedule.uTimeSectionCount[day] = cnt;
            for (int seg = 0; seg < cnt; ++seg)
            {
                FillSchedTime(vecDay[seg], dst.stAlarmSchedule.astTimeSection[day][seg]);
            }
        }
    }
}

void ToCrowdGathering(const NET_CrowdGatheringAlarmInfo_S &src, Alarm::CrowdGathering_S &dst)
{
    dst.bEnable = (src.bEnable == TRUE);
    dst.aRule.clear();
    for (int i = 0; i < src.uRuleCount && i < 4; ++i)
    {
        const auto &r = src.astRule[i];
        Alarm::CrowdGatheringRule_S out;
        ToRegionFromPolygon(r.uPointCount, r.afPointX, r.afPointY, out.stRegion);
        out.nObjectOccup = (unsigned int)r.nObjectOccup;
        dst.aRule.push_back(out);
    }

    dst.aAlarmTime.clear();
    dst.aAlarmTime.resize(7);
    for (int day = 0; day < 7; ++day)
    {
        int cnt = src.stAlarmSchedule.uTimeSectionCount[day];
        if (cnt <= 0)
            continue;
        cnt = std::min(cnt, NET_PLAN_SECTION_NUM);
        dst.aAlarmTime[day].resize(cnt);
        for (int seg = 0; seg < cnt; ++seg)
        {
            ToSchedTime(src.stAlarmSchedule.astTimeSection[day][seg], dst.aAlarmTime[day][seg]);
        }
    }
}

#if defined(SCENE_INTELLIGENCE) || CAP_AI_GARBAGE_DETECT

// --------- GarbageExposure (IPC Alarm::GarbageExposureDetection_S <-> SDK NET_GarbageExposureCfg_S) ---------
void FillGarbageExposureCfg(const Alarm::GarbageExposureDetection_S &src, NET_GarbageExposureCfg_S &dst)
{
    std::memset(&dst, 0, sizeof(dst));
    dst.bEnable = src.bEnable ? TRUE : FALSE;

    dst.stRule.nSensitivity = (INT32)src.stRule.nSensitivity;
    FillPolygonPoints(src.stRule.stRegion, dst.stRule.uPointCount, dst.stRule.afPointX, dst.stRule.afPointY);

    if (!src.aAlarmTime.empty())
    {
        for (int day = 0; day < 7; ++day)
        {
            if (day >= (int)src.aAlarmTime.size())
                break;
            const auto &vecDay = src.aAlarmTime[day];
            int cnt = (int)std::min<size_t>(vecDay.size(), NET_PLAN_SECTION_NUM);
            dst.stAlarmSchedule.uTimeSectionCount[day] = cnt;
            for (int seg = 0; seg < cnt; ++seg)
            {
                FillSchedTime(vecDay[seg], dst.stAlarmSchedule.astTimeSection[day][seg]);
            }
        }
    }

    FillLinkageList(src.stLinkageList, dst.stLinkageList);
}

void ToGarbageExposure(const NET_GarbageExposureCfg_S &src, Alarm::GarbageExposureDetection_S &dst)
{
    dst.bEnable = (src.bEnable == TRUE);
    dst.stRule.nSensitivity = (unsigned int)src.stRule.nSensitivity;
    ToRegionFromPolygon(src.stRule.uPointCount, src.stRule.afPointX, src.stRule.afPointY, dst.stRule.stRegion);

    dst.aAlarmTime.clear();
    dst.aAlarmTime.resize(7);
    for (int day = 0; day < 7; ++day)
    {
        int cnt = src.stAlarmSchedule.uTimeSectionCount[day];
        if (cnt <= 0)
            continue;
        cnt = std::min(cnt, NET_PLAN_SECTION_NUM);
        dst.aAlarmTime[day].resize(cnt);
        for (int seg = 0; seg < cnt; ++seg)
        {
            ToSchedTime(src.stAlarmSchedule.astTimeSection[day][seg], dst.aAlarmTime[day][seg]);
        }
    }

    ToLinkageList(src.stLinkageList, dst.stLinkageList);
}

// --------- GarbageOverflow (IPC Alarm::GarbageOverflowDetection_S <-> SDK NET_GarbageOverflowCfg_S) ---------
void FillGarbageOverflowCfg(const Alarm::GarbageOverflowDetection_S &src, NET_GarbageOverflowCfg_S &dst)
{
    std::memset(&dst, 0, sizeof(dst));
    dst.bEnable = src.bEnable ? TRUE : FALSE;

    dst.stRule.nSensitivity = (INT32)src.stRule.nSensitivity;
    FillPolygonPoints(src.stRule.stRegion, dst.stRule.uPointCount, dst.stRule.afPointX, dst.stRule.afPointY);

    if (!src.aAlarmTime.empty())
    {
        for (int day = 0; day < 7; ++day)
        {
            if (day >= (int)src.aAlarmTime.size())
                break;
            const auto &vecDay = src.aAlarmTime[day];
            int cnt = (int)std::min<size_t>(vecDay.size(), NET_PLAN_SECTION_NUM);
            dst.stAlarmSchedule.uTimeSectionCount[day] = cnt;
            for (int seg = 0; seg < cnt; ++seg)
            {
                FillSchedTime(vecDay[seg], dst.stAlarmSchedule.astTimeSection[day][seg]);
            }
        }
    }

    FillLinkageList(src.stLinkageList, dst.stLinkageList);
}

void ToGarbageOverflow(const NET_GarbageOverflowCfg_S &src, Alarm::GarbageOverflowDetection_S &dst)
{
    dst.bEnable = (src.bEnable == TRUE);
    dst.stRule.nSensitivity = (unsigned int)src.stRule.nSensitivity;
    ToRegionFromPolygon(src.stRule.uPointCount, src.stRule.afPointX, src.stRule.afPointY, dst.stRule.stRegion);

    dst.aAlarmTime.clear();
    dst.aAlarmTime.resize(7);
    for (int day = 0; day < 7; ++day)
    {
        int cnt = src.stAlarmSchedule.uTimeSectionCount[day];
        if (cnt <= 0)
            continue;
        cnt = std::min(cnt, NET_PLAN_SECTION_NUM);
        dst.aAlarmTime[day].resize(cnt);
        for (int seg = 0; seg < cnt; ++seg)
        {
            ToSchedTime(src.stAlarmSchedule.astTimeSection[day][seg], dst.aAlarmTime[day][seg]);
        }
    }

    ToLinkageList(src.stLinkageList, dst.stLinkageList);
}

#endif

static void FillSingleRuleAlarmSchedule(const Alarm::DefenseTime &src, NET_AlarmSchedule_S &dst)
{
    if (!src.empty())
    {
        for (int day = 0; day < 7; ++day)
        {
            if (day >= (int)src.size())
                break;

            const auto &vecDay = src[day];
            int cnt = (int)std::min<size_t>(vecDay.size(), NET_PLAN_SECTION_NUM);
            dst.uTimeSectionCount[day] = cnt;
            for (int seg = 0; seg < cnt; ++seg)
            {
                FillSchedTime(vecDay[seg], dst.astTimeSection[day][seg]);
            }
        }
    }
}

static void ToSingleRuleAlarmSchedule(const NET_AlarmSchedule_S &src, Alarm::DefenseTime &dst)
{
    dst.clear();
    dst.resize(7);
    for (int day = 0; day < 7; ++day)
    {
        int cnt = src.uTimeSectionCount[day];
        if (cnt <= 0)
            continue;

        cnt = std::min(cnt, NET_PLAN_SECTION_NUM);
        dst[day].resize(cnt);
        for (int seg = 0; seg < cnt; ++seg)
        {
            ToSchedTime(src.astTimeSection[day][seg], dst[day][seg]);
        }
    }
}

#ifdef SCENE_INTELLIGENCE
void FillManholeCoverAbnormalCfg(const Alarm::ManholeCoverAbnormalDetection_S &src, NET_ManholeCoverAbnormalCfg_S &dst)
{
    std::memset(&dst, 0, sizeof(dst));
    dst.bEnable = src.bEnable ? TRUE : FALSE;
    dst.stRule.nSensitivity = (INT32)src.stRule.nSensitivity;
    FillSingleRuleAlarmSchedule(src.aAlarmTime, dst.stAlarmSchedule);
    FillLinkageList(src.stLinkageList, dst.stLinkageList);
}

void ToManholeCoverAbnormal(const NET_ManholeCoverAbnormalCfg_S &src, Alarm::ManholeCoverAbnormalDetection_S &dst)
{
    dst.bEnable = (src.bEnable == TRUE);
    dst.stRule.nSensitivity = (unsigned int)src.stRule.nSensitivity;
    ToSingleRuleAlarmSchedule(src.stAlarmSchedule, dst.aAlarmTime);
    ToLinkageList(src.stLinkageList, dst.stLinkageList);
}

void FillSleepOnDutyCfg(const Alarm::SleepOnDutyDetection_S &src, NET_SleepOnDutyCfg_S &dst)
{
    std::memset(&dst, 0, sizeof(dst));
    dst.bEnable = src.bEnable ? TRUE : FALSE;
    dst.stRule.nSensitivity = (INT32)src.stRule.nSensitivity;
    FillSingleRuleAlarmSchedule(src.aAlarmTime, dst.stAlarmSchedule);
    FillLinkageList(src.stLinkageList, dst.stLinkageList);
}

void ToSleepOnDuty(const NET_SleepOnDutyCfg_S &src, Alarm::SleepOnDutyDetection_S &dst)
{
    dst.bEnable = (src.bEnable == TRUE);
    dst.stRule.nSensitivity = (unsigned int)src.stRule.nSensitivity;
    ToSingleRuleAlarmSchedule(src.stAlarmSchedule, dst.aAlarmTime);
    ToLinkageList(src.stLinkageList, dst.stLinkageList);
}

void FillElectricVehicleInElevatorCfg(const Alarm::ElectricScooterDetection_S &src, NET_ElectricVehicleInElevatorCfg_S &dst)
{
    std::memset(&dst, 0, sizeof(dst));
    dst.bEnable = src.bEnable ? TRUE : FALSE;
    dst.stRule.nSensitivity = (INT32)src.stRule.nSensitivity;
    FillSingleRuleAlarmSchedule(src.aAlarmTime, dst.stAlarmSchedule);
    FillLinkageList(src.stLinkageList, dst.stLinkageList);
}

void ToElectricVehicleInElevator(const NET_ElectricVehicleInElevatorCfg_S &src, Alarm::ElectricScooterDetection_S &dst)
{
    dst.bEnable = (src.bEnable == TRUE);
    dst.stRule.nSensitivity = (unsigned int)src.stRule.nSensitivity;
    ToSingleRuleAlarmSchedule(src.stAlarmSchedule, dst.aAlarmTime);
    ToLinkageList(src.stLinkageList, dst.stLinkageList);
}

void FillPersonFallDownCfg(const Alarm::PersonFallDownDetection_S &src, NET_PersonFallDownCfg_S &dst)
{
    std::memset(&dst, 0, sizeof(dst));
    dst.bEnable = src.bEnable ? TRUE : FALSE;
    dst.stRule.nSensitivity = (INT32)src.stRule.nSensitivity;
    FillSingleRuleAlarmSchedule(src.aAlarmTime, dst.stAlarmSchedule);
    FillLinkageList(src.stLinkageList, dst.stLinkageList);
}

void ToPersonFallDown(const NET_PersonFallDownCfg_S &src, Alarm::PersonFallDownDetection_S &dst)
{
    dst.bEnable = (src.bEnable == TRUE);
    dst.stRule.nSensitivity = (unsigned int)src.stRule.nSensitivity;
    ToSingleRuleAlarmSchedule(src.stAlarmSchedule, dst.aAlarmTime);
    ToLinkageList(src.stLinkageList, dst.stLinkageList);
}

void FillConstructionOccupyRoadCfg(const Alarm::ConstructionEncroachmentRoadDetection_S &src, NET_ConstructionOccupyRoadCfg_S &dst)
{
    std::memset(&dst, 0, sizeof(dst));
    dst.bEnable = src.bEnable ? TRUE : FALSE;
    dst.stRule.nSensitivity = (INT32)src.stRule.nSensitivity;
    FillSingleRuleAlarmSchedule(src.aAlarmTime, dst.stAlarmSchedule);
    FillLinkageList(src.stLinkageList, dst.stLinkageList);
}

void ToConstructionOccupyRoad(const NET_ConstructionOccupyRoadCfg_S &src, Alarm::ConstructionEncroachmentRoadDetection_S &dst)
{
    dst.bEnable = (src.bEnable == TRUE);
    dst.stRule.nSensitivity = (unsigned int)src.stRule.nSensitivity;
    ToSingleRuleAlarmSchedule(src.stAlarmSchedule, dst.aAlarmTime);
    ToLinkageList(src.stLinkageList, dst.stLinkageList);
}

void FillCongestionCfg(const Alarm::CongestionDetection_S &src, NET_CongestionCfg_S &dst)
{
    std::memset(&dst, 0, sizeof(dst));
    dst.bEnable = src.bEnable ? TRUE : FALSE;
    dst.stRule.nSensitivity = (INT32)src.stRule.nSensitivity;
    FillSingleRuleAlarmSchedule(src.aAlarmTime, dst.stAlarmSchedule);
    FillLinkageList(src.stLinkageList, dst.stLinkageList);
}

void ToCongestion(const NET_CongestionCfg_S &src, Alarm::CongestionDetection_S &dst)
{
    dst.bEnable = (src.bEnable == TRUE);
    dst.stRule.nSensitivity = (unsigned int)src.stRule.nSensitivity;
    ToSingleRuleAlarmSchedule(src.stAlarmSchedule, dst.aAlarmTime);
    ToLinkageList(src.stLinkageList, dst.stLinkageList);
}

void FillLicensePlateRecognitionCfg(const Alarm::LicensePlateCognitionDetection_S &src, NET_LicensePlateRecognitionCfg_S &dst)
{
    std::memset(&dst, 0, sizeof(dst));
    dst.bEnable = src.bEnable ? TRUE : FALSE;
    dst.stRule.nSensitivity = (INT32)src.stRule.nSensitivity;
    FillSingleRuleAlarmSchedule(src.aAlarmTime, dst.stAlarmSchedule);
    FillLinkageList(src.stLinkageList, dst.stLinkageList);
}

void ToLicensePlateRecognition(const NET_LicensePlateRecognitionCfg_S &src, Alarm::LicensePlateCognitionDetection_S &dst)
{
    dst.bEnable = (src.bEnable == TRUE);
    dst.stRule.nSensitivity = (unsigned int)src.stRule.nSensitivity;
    ToSingleRuleAlarmSchedule(src.stAlarmSchedule, dst.aAlarmTime);
    ToLinkageList(src.stLinkageList, dst.stLinkageList);
}

void FillHighAltitudeSeatbeltCfg(const Alarm::HighAltitudeSeatbeltDetection_S &src, NET_HighAltitudeSeatbeltCfg_S &dst)
{
    std::memset(&dst, 0, sizeof(dst));
    dst.bEnable = src.bEnable ? TRUE : FALSE;
    dst.stRule.nSensitivity = (INT32)src.stRule.nSensitivity;
    FillSingleRuleAlarmSchedule(src.aAlarmTime, dst.stAlarmSchedule);
    FillLinkageList(src.stLinkageList, dst.stLinkageList);
}

void ToHighAltitudeSeatbelt(const NET_HighAltitudeSeatbeltCfg_S &src, Alarm::HighAltitudeSeatbeltDetection_S &dst)
{
    dst.bEnable = (src.bEnable == TRUE);
    dst.stRule.nSensitivity = (unsigned int)src.stRule.nSensitivity;
    ToSingleRuleAlarmSchedule(src.stAlarmSchedule, dst.aAlarmTime);
    ToLinkageList(src.stLinkageList, dst.stLinkageList);
}

void FillSafetyHelmetCfg(const Alarm::SafetyHelmetDection_S &src, NET_SafetyHelmetCfg_S &dst)
{
    std::memset(&dst, 0, sizeof(dst));
    dst.bEnable = src.bEnable ? TRUE : FALSE;
    dst.stRule.nSensitivity = (INT32)src.stRule.nSensitivity;
    FillSingleRuleAlarmSchedule(src.aAlarmTime, dst.stAlarmSchedule);
    FillLinkageList(src.stLinkageList, dst.stLinkageList);
}

void ToSafetyHelmet(const NET_SafetyHelmetCfg_S &src, Alarm::SafetyHelmetDection_S &dst)
{
    dst.bEnable = (src.bEnable == TRUE);
    dst.stRule.nSensitivity = (unsigned int)src.stRule.nSensitivity;
    ToSingleRuleAlarmSchedule(src.stAlarmSchedule, dst.aAlarmTime);
    ToLinkageList(src.stLinkageList, dst.stLinkageList);
}

void FillPersonFallCfg(const Alarm::TripDetection_S &src, NET_PersonFallCfg_S &dst)
{
    std::memset(&dst, 0, sizeof(dst));
    dst.bEnable = src.bEnable ? TRUE : FALSE;
    dst.stRule.nSensitivity = (INT32)src.stRule.nSensitivity;
    FillSingleRuleAlarmSchedule(src.aAlarmTime, dst.stAlarmSchedule);
    FillLinkageList(src.stLinkageList, dst.stLinkageList);
}

void ToPersonFall(const NET_PersonFallCfg_S &src, Alarm::TripDetection_S &dst)
{
    dst.bEnable = (src.bEnable == TRUE);
    dst.stRule.nSensitivity = (unsigned int)src.stRule.nSensitivity;
    ToSingleRuleAlarmSchedule(src.stAlarmSchedule, dst.aAlarmTime);
    ToLinkageList(src.stLinkageList, dst.stLinkageList);
}

void FillPhoneUsageCfg(const Alarm::PhoneUsageDetection_S &src, NET_PhoneUsageCfg_S &dst)
{
    std::memset(&dst, 0, sizeof(dst));
    dst.bEnable = src.bEnable ? TRUE : FALSE;
    dst.stRule.nSensitivity = (INT32)src.stRule.nSensitivity;
    FillSingleRuleAlarmSchedule(src.aAlarmTime, dst.stAlarmSchedule);
    FillLinkageList(src.stLinkageList, dst.stLinkageList);
}

void ToPhoneUsage(const NET_PhoneUsageCfg_S &src, Alarm::PhoneUsageDetection_S &dst)
{
    dst.bEnable = (src.bEnable == TRUE);
    dst.stRule.nSensitivity = (unsigned int)src.stRule.nSensitivity;
    ToSingleRuleAlarmSchedule(src.stAlarmSchedule, dst.aAlarmTime);
    ToLinkageList(src.stLinkageList, dst.stLinkageList);
}

void FillSmokingCfg(const Alarm::SmokingDection_S &src, NET_SmokingCfg_S &dst)
{
    std::memset(&dst, 0, sizeof(dst));
    dst.bEnable = src.bEnable ? TRUE : FALSE;
    dst.stRule.nSensitivity = (INT32)src.stRule.nSensitivity;
    FillSingleRuleAlarmSchedule(src.aAlarmTime, dst.stAlarmSchedule);
    FillLinkageList(src.stLinkageList, dst.stLinkageList);
}

void ToSmoking(const NET_SmokingCfg_S &src, Alarm::SmokingDection_S &dst)
{
    dst.bEnable = (src.bEnable == TRUE);
    dst.stRule.nSensitivity = (unsigned int)src.stRule.nSensitivity;
    ToSingleRuleAlarmSchedule(src.stAlarmSchedule, dst.aAlarmTime);
    ToLinkageList(src.stLinkageList, dst.stLinkageList);
}

void FillOpenFlameCfg(const Alarm::OpenFlameDetection_S &src, NET_OpenFlameCfg_S &dst)
{
    std::memset(&dst, 0, sizeof(dst));
    dst.bEnable = src.bEnable ? TRUE : FALSE;
    dst.stRule.nSensitivity = (INT32)src.stRule.nSensitivity;
    FillSingleRuleAlarmSchedule(src.aAlarmTime, dst.stAlarmSchedule);
    FillLinkageList(src.stLinkageList, dst.stLinkageList);
}

void ToOpenFlame(const NET_OpenFlameCfg_S &src, Alarm::OpenFlameDetection_S &dst)
{
    dst.bEnable = (src.bEnable == TRUE);
    dst.stRule.nSensitivity = (unsigned int)src.stRule.nSensitivity;
    ToSingleRuleAlarmSchedule(src.stAlarmSchedule, dst.aAlarmTime);
    ToLinkageList(src.stLinkageList, dst.stLinkageList);
}

void FillBareSoilCfg(const Alarm::BareSoiletDection_S &src, NET_BareSoilCfg_S &dst)
{
    std::memset(&dst, 0, sizeof(dst));
    dst.bEnable = src.bEnable ? TRUE : FALSE;
    dst.stRule.nSensitivity = (INT32)src.stRule.nSensitivity;
    FillSingleRuleAlarmSchedule(src.aAlarmTime, dst.stAlarmSchedule);
    FillLinkageList(src.stLinkageList, dst.stLinkageList);
}

void ToBareSoil(const NET_BareSoilCfg_S &src, Alarm::BareSoiletDection_S &dst)
{
    dst.bEnable = (src.bEnable == TRUE);
    dst.stRule.nSensitivity = (unsigned int)src.stRule.nSensitivity;
    ToSingleRuleAlarmSchedule(src.stAlarmSchedule, dst.aAlarmTime);
    ToLinkageList(src.stLinkageList, dst.stLinkageList);
}

void FillHoleProtectionBarCfg(const Alarm::HoleProtectionBarDection_S &src, NET_HoleProtectionBarCfg_S &dst)
{
    std::memset(&dst, 0, sizeof(dst));
    dst.bEnable = src.bEnable ? TRUE : FALSE;
    dst.stRule.nSensitivity = (INT32)src.stRule.nSensitivity;
    FillSingleRuleAlarmSchedule(src.aAlarmTime, dst.stAlarmSchedule);
    FillLinkageList(src.stLinkageList, dst.stLinkageList);
}

void ToHoleProtectionBar(const NET_HoleProtectionBarCfg_S &src, Alarm::HoleProtectionBarDection_S &dst)
{
    dst.bEnable = (src.bEnable == TRUE);
    dst.stRule.nSensitivity = (unsigned int)src.stRule.nSensitivity;
    ToSingleRuleAlarmSchedule(src.stAlarmSchedule, dst.aAlarmTime);
    ToLinkageList(src.stLinkageList, dst.stLinkageList);
}

void FillReflectiveClothingCfg(const Alarm::ReflectiveClothingDection_S &src, NET_ReflectiveClothingCfg_S &dst)
{
    std::memset(&dst, 0, sizeof(dst));
    dst.bEnable = src.bEnable ? TRUE : FALSE;
    dst.stRule.nSensitivity = (INT32)src.stRule.nSensitivity;
    FillSingleRuleAlarmSchedule(src.aAlarmTime, dst.stAlarmSchedule);
    FillLinkageList(src.stLinkageList, dst.stLinkageList);
}

void ToReflectiveClothing(const NET_ReflectiveClothingCfg_S &src, Alarm::ReflectiveClothingDection_S &dst)
{
    dst.bEnable = (src.bEnable == TRUE);
    dst.stRule.nSensitivity = (unsigned int)src.stRule.nSensitivity;
    ToSingleRuleAlarmSchedule(src.stAlarmSchedule, dst.aAlarmTime);
    ToLinkageList(src.stLinkageList, dst.stLinkageList);
}
#endif

void FillPetRecognitionInfo(const Alarm::PetRecognition_S &src, NET_PetRecognitionInfo_S &dst)
{
    std::memset(&dst, 0, sizeof(dst));
    dst.bEnable = src.bEnable ? TRUE : FALSE;
    dst.bDynamicAnalysisEnable = src.bDynamicAnalysisEnable ? TRUE : FALSE;
    dst.nSensitivity = (INT32)src.nSensitivity;
    FillPolygonPoints(src.stRegion, dst.stRegion.uPointCount, dst.stRegion.afPointX, dst.stRegion.afPointY);
    FillSingleRuleAlarmSchedule(src.aAlarmTime, dst.stAlarmSchedule);
    FillLinkageList(src.stLinkageList, dst.stLinkageList);
}

void ToPetRecognition(const NET_PetRecognitionInfo_S &src, Alarm::PetRecognition_S &dst)
{
    dst.bEnable = (src.bEnable == TRUE);
    dst.bDynamicAnalysisEnable = (src.bDynamicAnalysisEnable == TRUE);
    dst.nSensitivity = (unsigned int)src.nSensitivity;
    ToRegionFromPolygon(src.stRegion.uPointCount, src.stRegion.afPointX, src.stRegion.afPointY, dst.stRegion);
    ToSingleRuleAlarmSchedule(src.stAlarmSchedule, dst.aAlarmTime);
    ToLinkageList(src.stLinkageList, dst.stLinkageList);
}

#ifdef SCENE_INTELLIGENCE
void FillClimbFenceInfo(const Alarm::FenceClimbingDetection_S &src, NET_ClimbFenceInfo_S &dst)
{
    std::memset(&dst, 0, sizeof(dst));
    dst.bEnable = src.bEnable ? TRUE : FALSE;
    dst.uRuleCount = 0;
    for (size_t i = 0; i < src.aRule.size() && i < 4; ++i)
    {
        const auto &r = src.aRule[i];
        NET_SmartRegionRule_S &out = dst.astRule[i];
        std::memset(&out, 0, sizeof(out));
        out.bEnable = TRUE;
        FillPolygonPoints(r.stRegion, out.uPointCount, out.afPointX, out.afPointY);
        out.nSensitivity = (INT32)r.nSensitivity;
        out.nTimeThreshold = (INT32)r.nTimeThreshold;
        FillDetectionTargets(r.aDetectionTarget, out.uDetectionTargetCount, out.auDetectionTarget);
        dst.uRuleCount++;
    }
    FillSingleRuleAlarmSchedule(src.aAlarmTime, dst.stAlarmSchedule);
    FillLinkageList(src.stLinkageList, dst.stLinkageList);
}

void ToClimbFence(const NET_ClimbFenceInfo_S &src, Alarm::FenceClimbingDetection_S &dst)
{
    dst.bEnable = (src.bEnable == TRUE);
    dst.aRule.clear();
    for (int i = 0; i < src.uRuleCount && i < 4; ++i)
    {
        const NET_SmartRegionRule_S &r = src.astRule[i];
        Alarm::FenceClimbingRule_S out;
        ToRegionFromPolygon(r.uPointCount, r.afPointX, r.afPointY, out.stRegion);
        out.nSensitivity = (unsigned int)r.nSensitivity;
        out.nTimeThreshold = (unsigned int)r.nTimeThreshold;
        ToDetectionTargets(r.auDetectionTarget, r.uDetectionTargetCount, out.aDetectionTarget);
        dst.aRule.push_back(out);
    }
    ToSingleRuleAlarmSchedule(src.stAlarmSchedule, dst.aAlarmTime);
    ToLinkageList(src.stLinkageList, dst.stLinkageList);
}

void FillDimissionInfo(const Alarm::LeavePostDetection_S &src, NET_DimissionInfo_S &dst)
{
    std::memset(&dst, 0, sizeof(dst));
    dst.bEnable = src.bEnable ? TRUE : FALSE;
    dst.uRuleCount = 0;
    for (size_t i = 0; i < src.aRule.size() && i < 4; ++i)
    {
        const auto &r = src.aRule[i];
        NET_SmartRegionRule_S &out = dst.astRule[i];
        std::memset(&out, 0, sizeof(out));
        out.bEnable = TRUE;
        FillPolygonPoints(r.stRegion, out.uPointCount, out.afPointX, out.afPointY);
        out.nSensitivity = (INT32)r.nSensitivity;
        out.nTimeThreshold = (INT32)r.nTimeThreshold;
        FillDetectionTargets(r.aDetectionTarget, out.uDetectionTargetCount, out.auDetectionTarget);
        dst.uRuleCount++;
    }
    FillSingleRuleAlarmSchedule(src.aAlarmTime, dst.stAlarmSchedule);
    FillLinkageList(src.stLinkageList, dst.stLinkageList);
}

void ToDimission(const NET_DimissionInfo_S &src, Alarm::LeavePostDetection_S &dst)
{
    dst.bEnable = (src.bEnable == TRUE);
    dst.aRule.clear();
    for (int i = 0; i < src.uRuleCount && i < 4; ++i)
    {
        const NET_SmartRegionRule_S &r = src.astRule[i];
        Alarm::LeavePostRule_S out;
        ToRegionFromPolygon(r.uPointCount, r.afPointX, r.afPointY, out.stRegion);
        out.nSensitivity = (unsigned int)r.nSensitivity;
        out.nTimeThreshold = (unsigned int)r.nTimeThreshold;
        ToDetectionTargets(r.auDetectionTarget, r.uDetectionTargetCount, out.aDetectionTarget);
        dst.aRule.push_back(out);
    }
    ToSingleRuleAlarmSchedule(src.stAlarmSchedule, dst.aAlarmTime);
    ToLinkageList(src.stLinkageList, dst.stLinkageList);
}

void FillIllegalLaneInfo(const Alarm::IllegalLaneChangeDetection_S &src, NET_IllegalLaneInfo_S &dst)
{
    std::memset(&dst, 0, sizeof(dst));
    dst.bEnable = src.bEnable ? TRUE : FALSE;
    dst.uRuleCount = 0;
    for (size_t i = 0; i < src.aRule.size() && i < 4; ++i)
    {
        const auto &r = src.aRule[i];
        NET_SmartLineRule_S &out = dst.astRule[i];
        std::memset(&out, 0, sizeof(out));
        out.bEnable = TRUE;
        out.fStartPosX = r.stStartPos.fX;
        out.fStartPosY = r.stStartPos.fY;
        out.fEndPosX = r.stEndPos.fX;
        out.fEndPosY = r.stEndPos.fY;
        out.enCrossDirection = (INT32)r.enCrossDirection;
        out.nSensitivity = (INT32)r.nSensitivity;
        dst.uRuleCount++;
    }
    FillSingleRuleAlarmSchedule(src.aAlarmTime, dst.stAlarmSchedule);
    FillLinkageList(src.stLinkageList, dst.stLinkageList);
}

void ToIllegalLane(const NET_IllegalLaneInfo_S &src, Alarm::IllegalLaneChangeDetection_S &dst)
{
    dst.bEnable = (src.bEnable == TRUE);
    dst.aRule.clear();
    for (int i = 0; i < src.uRuleCount && i < 4; ++i)
    {
        const NET_SmartLineRule_S &r = src.astRule[i];
        Alarm::IllegalLaneChangeRule_S out;
        out.stStartPos = {r.fStartPosX, r.fStartPosY};
        out.stEndPos = {r.fEndPosX, r.fEndPosY};
        out.enCrossDirection = (Alarm::CrossDirection_E)r.enCrossDirection;
        out.nSensitivity = (unsigned int)r.nSensitivity;
        dst.aRule.push_back(out);
    }
    ToSingleRuleAlarmSchedule(src.stAlarmSchedule, dst.aAlarmTime);
    ToLinkageList(src.stLinkageList, dst.stLinkageList);
}

void FillRetrogradeInfo(const Alarm::DrivingAgainstTrafficDetection_S &src, NET_RetrogradeInfo_S &dst)
{
    std::memset(&dst, 0, sizeof(dst));
    dst.bEnable = src.bEnable ? TRUE : FALSE;
    dst.uRuleCount = 0;
    for (size_t i = 0; i < src.aRule.size() && i < 4; ++i)
    {
        const auto &r = src.aRule[i];
        NET_SmartLineRule_S &out = dst.astRule[i];
        std::memset(&out, 0, sizeof(out));
        out.bEnable = TRUE;
        out.fStartPosX = r.stStartPos.fX;
        out.fStartPosY = r.stStartPos.fY;
        out.fEndPosX = r.stEndPos.fX;
        out.fEndPosY = r.stEndPos.fY;
        out.enCrossDirection = (INT32)r.enCrossDirection;
        out.nSensitivity = (INT32)r.nSensitivity;
        dst.uRuleCount++;
    }
    FillSingleRuleAlarmSchedule(src.aAlarmTime, dst.stAlarmSchedule);
    FillLinkageList(src.stLinkageList, dst.stLinkageList);
}

void ToRetrograde(const NET_RetrogradeInfo_S &src, Alarm::DrivingAgainstTrafficDetection_S &dst)
{
    dst.bEnable = (src.bEnable == TRUE);
    dst.aRule.clear();
    for (int i = 0; i < src.uRuleCount && i < 4; ++i)
    {
        const NET_SmartLineRule_S &r = src.astRule[i];
        Alarm::DrivingAgainstTrafficRule_S out;
        out.stStartPos = {r.fStartPosX, r.fStartPosY};
        out.stEndPos = {r.fEndPosX, r.fEndPosY};
        out.enCrossDirection = (Alarm::CrossDirection_E)r.enCrossDirection;
        out.nSensitivity = (unsigned int)r.nSensitivity;
        dst.aRule.push_back(out);
    }
    ToSingleRuleAlarmSchedule(src.stAlarmSchedule, dst.aAlarmTime);
    ToLinkageList(src.stLinkageList, dst.stLinkageList);
}

void FillNonmotorVehicleIntrusionInfo(const Alarm::NonMotorVehicleIntrusionDetection_S &src, NET_NonmotorVehicleIntrusionInfo_S &dst)
{
    std::memset(&dst, 0, sizeof(dst));
    dst.bEnable = src.bEnable ? TRUE : FALSE;
    dst.uRuleCount = 0;
    for (size_t i = 0; i < src.aRule.size() && i < 4; ++i)
    {
        const auto &r = src.aRule[i];
        NET_SmartRegionRule_S &out = dst.astRule[i];
        std::memset(&out, 0, sizeof(out));
        out.bEnable = TRUE;
        FillPolygonPoints(r.stRegion, out.uPointCount, out.afPointX, out.afPointY);
        out.nSensitivity = (INT32)r.nSensitivity;
        out.nTimeThreshold = (INT32)r.nTimeThreshold;
        FillDetectionTargets(r.aDetectionTarget, out.uDetectionTargetCount, out.auDetectionTarget);
        dst.uRuleCount++;
    }
    FillSingleRuleAlarmSchedule(src.aAlarmTime, dst.stAlarmSchedule);
    FillLinkageList(src.stLinkageList, dst.stLinkageList);
}

void ToNonmotorVehicleIntrusion(const NET_NonmotorVehicleIntrusionInfo_S &src, Alarm::NonMotorVehicleIntrusionDetection_S &dst)
{
    dst.bEnable = (src.bEnable == TRUE);
    dst.aRule.clear();
    for (int i = 0; i < src.uRuleCount && i < 4; ++i)
    {
        const NET_SmartRegionRule_S &r = src.astRule[i];
        Alarm::NonMotorVehicleIntrusionRule_S out;
        ToRegionFromPolygon(r.uPointCount, r.afPointX, r.afPointY, out.stRegion);
        out.nSensitivity = (unsigned int)r.nSensitivity;
        out.nTimeThreshold = (unsigned int)r.nTimeThreshold;
        ToDetectionTargets(r.auDetectionTarget, r.uDetectionTargetCount, out.aDetectionTarget);
        dst.aRule.push_back(out);
    }
    ToSingleRuleAlarmSchedule(src.stAlarmSchedule, dst.aAlarmTime);
    ToLinkageList(src.stLinkageList, dst.stLinkageList);
}

void FillOccupationEmergencyInfo(const Alarm::EmergencyLaneOccupancyDetection_S &src, NET_OccupationEmergencyInfo_S &dst)
{
    std::memset(&dst, 0, sizeof(dst));
    dst.bEnable = src.bEnable ? TRUE : FALSE;
    dst.uRuleCount = 0;
    for (size_t i = 0; i < src.aRule.size() && i < 4; ++i)
    {
        const auto &r = src.aRule[i];
        NET_SmartRegionRule_S &out = dst.astRule[i];
        std::memset(&out, 0, sizeof(out));
        out.bEnable = TRUE;
        FillPolygonPoints(r.stRegion, out.uPointCount, out.afPointX, out.afPointY);
        out.nSensitivity = (INT32)r.nSensitivity;
        out.nTimeThreshold = (INT32)r.nTimeThreshold;
        FillDetectionTargets(r.aDetectionTarget, out.uDetectionTargetCount, out.auDetectionTarget);
        dst.uRuleCount++;
    }
    FillSingleRuleAlarmSchedule(src.aAlarmTime, dst.stAlarmSchedule);
    FillLinkageList(src.stLinkageList, dst.stLinkageList);
}

void ToOccupationEmergency(const NET_OccupationEmergencyInfo_S &src, Alarm::EmergencyLaneOccupancyDetection_S &dst)
{
    dst.bEnable = (src.bEnable == TRUE);
    dst.aRule.clear();
    for (int i = 0; i < src.uRuleCount && i < 4; ++i)
    {
        const NET_SmartRegionRule_S &r = src.astRule[i];
        Alarm::EmergencyLaneOccupancyRule_S out;
        ToRegionFromPolygon(r.uPointCount, r.afPointX, r.afPointY, out.stRegion);
        out.nSensitivity = (unsigned int)r.nSensitivity;
        out.nTimeThreshold = (unsigned int)r.nTimeThreshold;
        ToDetectionTargets(r.auDetectionTarget, r.uDetectionTargetCount, out.aDetectionTarget);
        dst.aRule.push_back(out);
    }
    ToSingleRuleAlarmSchedule(src.stAlarmSchedule, dst.aAlarmTime);
    ToLinkageList(src.stLinkageList, dst.stLinkageList);
}

void FillPedestrianIntrusionInfo(const Alarm::PedestrianIntrusionDetection_S &src, NET_PedestrianIntrusionInfo_S &dst)
{
    std::memset(&dst, 0, sizeof(dst));
    dst.bEnable = src.bEnable ? TRUE : FALSE;
    dst.uRuleCount = 0;
    for (size_t i = 0; i < src.aRule.size() && i < 4; ++i)
    {
        const auto &r = src.aRule[i];
        NET_SmartRegionRule_S &out = dst.astRule[i];
        std::memset(&out, 0, sizeof(out));
        out.bEnable = TRUE;
        FillPolygonPoints(r.stRegion, out.uPointCount, out.afPointX, out.afPointY);
        out.nSensitivity = (INT32)r.nSensitivity;
        out.nTimeThreshold = (INT32)r.nTimeThreshold;
        FillDetectionTargets(r.aDetectionTarget, out.uDetectionTargetCount, out.auDetectionTarget);
        dst.uRuleCount++;
    }
    FillSingleRuleAlarmSchedule(src.aAlarmTime, dst.stAlarmSchedule);
    FillLinkageList(src.stLinkageList, dst.stLinkageList);
}

void ToPedestrianIntrusion(const NET_PedestrianIntrusionInfo_S &src, Alarm::PedestrianIntrusionDetection_S &dst)
{
    dst.bEnable = (src.bEnable == TRUE);
    dst.aRule.clear();
    for (int i = 0; i < src.uRuleCount && i < 4; ++i)
    {
        const NET_SmartRegionRule_S &r = src.astRule[i];
        Alarm::PedestrianIntrusionRule_S out;
        ToRegionFromPolygon(r.uPointCount, r.afPointX, r.afPointY, out.stRegion);
        out.nSensitivity = (unsigned int)r.nSensitivity;
        out.nTimeThreshold = (unsigned int)r.nTimeThreshold;
        ToDetectionTargets(r.auDetectionTarget, r.uDetectionTargetCount, out.aDetectionTarget);
        dst.aRule.push_back(out);
    }
    ToSingleRuleAlarmSchedule(src.stAlarmSchedule, dst.aAlarmTime);
    ToLinkageList(src.stLinkageList, dst.stLinkageList);
}

void FillSmokeFireCfg(const Alarm::SmokeFireDetection_S &src, NET_SmokeFireCfg_S &dst)
{
    std::memset(&dst, 0, sizeof(dst));
    dst.bEnable = src.bEnable ? TRUE : FALSE;
    dst.stRule.nSensitivity = (INT32)src.stRule.nSensitivity;
    FillSingleRuleAlarmSchedule(src.aAlarmTime, dst.stAlarmSchedule);
    FillLinkageList(src.stLinkageList, dst.stLinkageList);
}

void ToSmokeFire(const NET_SmokeFireCfg_S &src, Alarm::SmokeFireDetection_S &dst)
{
    dst.bEnable = (src.bEnable == TRUE);
    dst.stRule.nSensitivity = (unsigned int)src.stRule.nSensitivity;
    ToSingleRuleAlarmSchedule(src.stAlarmSchedule, dst.aAlarmTime);
    ToLinkageList(src.stLinkageList, dst.stLinkageList);
}

void FillRoadPondingCfg(const Alarm::RoadPondingDetection_S &src, NET_RoadPondingCfg_S &dst)
{
    std::memset(&dst, 0, sizeof(dst));
    dst.bEnable = src.bEnable ? TRUE : FALSE;
    dst.stRule.nSensitivity = (INT32)src.stRule.nSensitivity;
    FillSingleRuleAlarmSchedule(src.aAlarmTime, dst.stAlarmSchedule);
    FillLinkageList(src.stLinkageList, dst.stLinkageList);
}

void ToRoadPonding(const NET_RoadPondingCfg_S &src, Alarm::RoadPondingDetection_S &dst)
{
    dst.bEnable = (src.bEnable == TRUE);
    dst.stRule.nSensitivity = (unsigned int)src.stRule.nSensitivity;
    ToSingleRuleAlarmSchedule(src.stAlarmSchedule, dst.aAlarmTime);
    ToLinkageList(src.stLinkageList, dst.stLinkageList);
}

#endif

// --------- AudioAnomaly (IPC AudioAnomaly_S <-> SDK NET_AudioAnomalyAlarmInfo_S) ---------
void FillAudioAnomalyAlarmInfo(const Alarm::AudioAnomaly_S &src, NET_AudioAnomalyAlarmInfo_S &dst)
{
    std::memset(&dst, 0, sizeof(dst));
    dst.bEnable = src.bEnable ? TRUE : FALSE;
    dst.bAudioInputAnomaly = src.bAudioInputAnomaly ? TRUE : FALSE;
    dst.bUpEnable = src.bUpEnable ? TRUE : FALSE;
    dst.nUpSensitivity = (INT32)src.nUpSensitivity;
    dst.nUpThreshold = (INT32)src.nUpThreshold;
    dst.bDownEnable = src.bDownEnable ? TRUE : FALSE;
    dst.nDownSensitivity = (INT32)src.nDownSensitivity;

    if (!src.aAlarmTime.empty())
    {
        for (int day = 0; day < 7; ++day)
        {
            if (day >= (int)src.aAlarmTime.size())
                break;
            const auto &vecDay = src.aAlarmTime[day];
            int cnt = (int)std::min<size_t>(vecDay.size(), NET_PLAN_SECTION_NUM);
            dst.stAlarmSchedule.uTimeSectionCount[day] = cnt;
            for (int seg = 0; seg < cnt; ++seg)
            {
                FillSchedTime(vecDay[seg], dst.stAlarmSchedule.astTimeSection[day][seg]);
            }
        }
    }
}

void ToAudioAnomaly(const NET_AudioAnomalyAlarmInfo_S &src, Alarm::AudioAnomaly_S &dst)
{
    dst.bEnable = (src.bEnable == TRUE);
    dst.bAudioInputAnomaly = (src.bAudioInputAnomaly == TRUE);
    dst.bUpEnable = (src.bUpEnable == TRUE);
    dst.nUpSensitivity = (unsigned int)src.nUpSensitivity;
    dst.nUpThreshold = (unsigned int)src.nUpThreshold;
    dst.bDownEnable = (src.bDownEnable == TRUE);
    dst.nDownSensitivity = (unsigned int)src.nDownSensitivity;

    dst.aAlarmTime.clear();
    dst.aAlarmTime.resize(7);
    for (int day = 0; day < 7; ++day)
    {
        int cnt = src.stAlarmSchedule.uTimeSectionCount[day];
        if (cnt <= 0)
            continue;
        cnt = std::min(cnt, NET_PLAN_SECTION_NUM);
        dst.aAlarmTime[day].resize(cnt);
        for (int seg = 0; seg < cnt; ++seg)
        {
            ToSchedTime(src.stAlarmSchedule.astTimeSection[day][seg], dst.aAlarmTime[day][seg]);
        }
    }
}

#if CAP_AI_PEOPLE_STATISTICS
// --------- PeopleFlowStatistics (IPC Alarm::PeopleFlowStatistics_S <-> SDK NET_PeopleFlowStatisticsCfg_S) ---------
static void FillPeopleAlarmRule(const Alarm::PopulationAlarmRule_S &src, NET_PeopleAlarmRule_S &dst)
{
    std::memset(&dst, 0, sizeof(dst));
    dst.bEnable = src.bEnable ? TRUE : FALSE;
    dst.nThreshold = (INT32)src.nThreshold;
    FillLinkageList(src.stLinkageList, dst.stLinkageList);
}

static void ToPeopleAlarmRule(const NET_PeopleAlarmRule_S &src, Alarm::PopulationAlarmRule_S &dst)
{
    dst.bEnable = (src.bEnable == TRUE);
    dst.nThreshold = (unsigned int)src.nThreshold;
    ToLinkageList(src.stLinkageList, dst.stLinkageList);
}

void FillPeopleFlowStatisticsCfg(const Alarm::PeopleFlowStatistics_S &src, NET_PeopleFlowStatisticsCfg_S &dst)
{
    std::memset(&dst, 0, sizeof(dst));
    dst.bEnable = src.bEnable ? TRUE : FALSE;
    dst.nSensitivity = (INT32)src.nSensitivity;

    // 规则线
    dst.stRuleLine.fStartPointX = src.stRuleLine.stStartPos.fX;
    dst.stRuleLine.fStartPointY = src.stRuleLine.stStartPos.fY;
    dst.stRuleLine.fEndPointX = src.stRuleLine.stEndPos.fX;
    dst.stRuleLine.fEndPointY = src.stRuleLine.stEndPos.fY;
    dst.stRuleLine.nDirection = (INT32)src.stRuleLine.enDirection;

    // 检测区域
    FillPolygonPoints(src.stDetectRegion, dst.uPointCount, dst.afPointX, dst.afPointY);

    dst.nReportInterval = (INT32)src.nReportInterval;
    dst.enStatisticsType = (INT32)src.enStatisticsType;

    // 定时清零
    dst.stTimedReset.bEnable = src.stTimedReset.bEnable ? TRUE : FALSE;
    dst.stTimedReset.nHour = (INT32)src.stTimedReset.stExecuteTime.nHour;
    dst.stTimedReset.nMinute = (INT32)src.stTimedReset.stExecuteTime.nMinute;

    // 三级报警
    FillPeopleAlarmRule(src.stStayAlarm.stNormal, dst.stStayAlarm.stNormal);
    FillPeopleAlarmRule(src.stStayAlarm.stMedium, dst.stStayAlarm.stMedium);
    FillPeopleAlarmRule(src.stStayAlarm.stSevere, dst.stStayAlarm.stSevere);

    // 布防时间
    if (!src.aAlarmTime.empty())
    {
        for (int day = 0; day < 7; ++day)
        {
            if (day >= (int)src.aAlarmTime.size())
                break;
            const auto &vecDay = src.aAlarmTime[day];
            int cnt = (int)std::min<size_t>(vecDay.size(), NET_PLAN_SECTION_NUM);
            dst.stAlarmSchedule.uTimeSectionCount[day] = cnt;
            for (int seg = 0; seg < cnt; ++seg)
            {
                FillSchedTime(vecDay[seg], dst.stAlarmSchedule.astTimeSection[day][seg]);
            }
        }
    }
}

void ToPeopleFlowStatistics(const NET_PeopleFlowStatisticsCfg_S &src, Alarm::PeopleFlowStatistics_S &dst)
{
    dst.bEnable = (src.bEnable == TRUE);
    dst.nSensitivity = (unsigned int)src.nSensitivity;

    // 规则线
    dst.stRuleLine.stStartPos.fX = src.stRuleLine.fStartPointX;
    dst.stRuleLine.stStartPos.fY = src.stRuleLine.fStartPointY;
    dst.stRuleLine.stEndPos.fX = src.stRuleLine.fEndPointX;
    dst.stRuleLine.stEndPos.fY = src.stRuleLine.fEndPointY;
    dst.stRuleLine.enDirection = (Alarm::CrossDirection_E)src.stRuleLine.nDirection;

    // 检测区域
    ToRegionFromPolygon(src.uPointCount, src.afPointX, src.afPointY, dst.stDetectRegion);

    dst.nReportInterval = (unsigned int)src.nReportInterval;
    dst.enStatisticsType = (Alarm::PeopleFlowStatisticsType_E)src.enStatisticsType;

    // 定时清零
    dst.stTimedReset.bEnable = (src.stTimedReset.bEnable == TRUE);
    dst.stTimedReset.stExecuteTime.nHour = (int)src.stTimedReset.nHour;
    dst.stTimedReset.stExecuteTime.nMinute = (int)src.stTimedReset.nMinute;
    dst.stTimedReset.stExecuteTime.nSecond = 0;

    // 三级报警
    ToPeopleAlarmRule(src.stStayAlarm.stNormal, dst.stStayAlarm.stNormal);
    ToPeopleAlarmRule(src.stStayAlarm.stMedium, dst.stStayAlarm.stMedium);
    ToPeopleAlarmRule(src.stStayAlarm.stSevere, dst.stStayAlarm.stSevere);

    // 布防时间
    dst.aAlarmTime.clear();
    dst.aAlarmTime.resize(7);
    for (int day = 0; day < 7; ++day)
    {
        int cnt = src.stAlarmSchedule.uTimeSectionCount[day];
        if (cnt <= 0)
            continue;
        cnt = std::min(cnt, NET_PLAN_SECTION_NUM);
        dst.aAlarmTime[day].resize(cnt);
        for (int seg = 0; seg < cnt; ++seg)
        {
            ToSchedTime(src.stAlarmSchedule.astTimeSection[day][seg], dst.aAlarmTime[day][seg]);
        }
    }
}

// --------- PeopleDensityDetection (IPC Alarm::PeopleDensityDetection_S <-> SDK NET_PeopleDensityDetectionCfg_S) ---------
void FillPeopleDensityDetectionCfg(const Alarm::PeopleDensityDetection_S &src, NET_PeopleDensityDetectionCfg_S &dst)
{
    std::memset(&dst, 0, sizeof(dst));
    dst.bEnable = src.bEnable ? TRUE : FALSE;
    dst.nSensitivity = (INT32)src.nSensitivity;

    // 检测区域
    FillPolygonPoints(src.stDetectRegion, dst.uPointCount, dst.afPointX, dst.afPointY);

    dst.nReportInterval = (INT32)src.nReportInterval;

    // 三级报警
    FillPeopleAlarmRule(src.stDensityAlarm.stNormal, dst.stDensityAlarm.stNormal);
    FillPeopleAlarmRule(src.stDensityAlarm.stMedium, dst.stDensityAlarm.stMedium);
    FillPeopleAlarmRule(src.stDensityAlarm.stSevere, dst.stDensityAlarm.stSevere);

    // 布防时间
    if (!src.aAlarmTime.empty())
    {
        for (int day = 0; day < 7; ++day)
        {
            if (day >= (int)src.aAlarmTime.size())
                break;
            const auto &vecDay = src.aAlarmTime[day];
            int cnt = (int)std::min<size_t>(vecDay.size(), NET_PLAN_SECTION_NUM);
            dst.stAlarmSchedule.uTimeSectionCount[day] = cnt;
            for (int seg = 0; seg < cnt; ++seg)
            {
                FillSchedTime(vecDay[seg], dst.stAlarmSchedule.astTimeSection[day][seg]);
            }
        }
    }
}

void ToPeopleDensityDetection(const NET_PeopleDensityDetectionCfg_S &src, Alarm::PeopleDensityDetection_S &dst)
{
    dst.bEnable = (src.bEnable == TRUE);
    dst.nSensitivity = (unsigned int)src.nSensitivity;

    // 检测区域
    ToRegionFromPolygon(src.uPointCount, src.afPointX, src.afPointY, dst.stDetectRegion);

    dst.nReportInterval = (unsigned int)src.nReportInterval;

    // 三级报警
    ToPeopleAlarmRule(src.stDensityAlarm.stNormal, dst.stDensityAlarm.stNormal);
    ToPeopleAlarmRule(src.stDensityAlarm.stMedium, dst.stDensityAlarm.stMedium);
    ToPeopleAlarmRule(src.stDensityAlarm.stSevere, dst.stDensityAlarm.stSevere);

    // 布防时间
    dst.aAlarmTime.clear();
    dst.aAlarmTime.resize(7);
    for (int day = 0; day < 7; ++day)
    {
        int cnt = src.stAlarmSchedule.uTimeSectionCount[day];
        if (cnt <= 0)
            continue;
        cnt = std::min(cnt, NET_PLAN_SECTION_NUM);
        dst.aAlarmTime[day].resize(cnt);
        for (int seg = 0; seg < cnt; ++seg)
        {
            ToSchedTime(src.stAlarmSchedule.astTimeSection[day][seg], dst.aAlarmTime[day][seg]);
        }
    }
}
#endif

void ToUpgradeInfo(const NET_UpgradeInfo_S &src, ::System::UpgradeInfo_S &dst)
{
    dst.strUpgradePath = src.szUpgradePath;
}

void FillUpgradeStatus(const ::System::UpgradeStatus_S &src, NET_UpgradeStatus_S &dst)
{
    std::memset(&dst, 0, sizeof(dst));
    dst.nUpgradeStatus = (INT32)src.nUpgradeStatus;
}

void FillUpgradeVersion(const ::System::UpgradeVersion_S &src, NET_UpgradeVersion_S &dst)
{
    std::memset(&dst, 0, sizeof(dst));
    std::strncpy(dst.szVersion, src.strVersion.c_str(), sizeof(dst.szVersion) - 1);
    dst.szVersion[sizeof(dst.szVersion) - 1] = '\0';
}

static void FillOneCaptureConfig(const Capture_NS::CaptureConfig_S &src, NET_CaptureConfig_S &dst)
{
    std::memset(&dst, 0, sizeof(dst));
    dst.bEnable = src.bEnable ? TRUE : FALSE;
    dst.enPictureFormat = (INT32)src.enPictureFormat;
    dst.nWidth = (INT32)src.stVideoResolution.nWidth;
    dst.nHeight = (INT32)src.stVideoResolution.nHeight;
    dst.enImageQuality = (INT32)src.enImageQuality;
    dst.unInterval = src.stTimeInterval.unInterval;
    dst.enTimeUnit = (INT32)src.stTimeInterval.enTimeUnit;
    dst.unNumber = src.unNumber;
}

static void ToOneCaptureConfig(const NET_CaptureConfig_S &src, Capture_NS::CaptureConfig_S &dst)
{
    dst.bEnable = (src.bEnable == TRUE);
    dst.enPictureFormat = (Capture_NS::PictureFormat_E)src.enPictureFormat;
    dst.stVideoResolution.nWidth = src.nWidth;
    dst.stVideoResolution.nHeight = src.nHeight;
    dst.enImageQuality = (Capture_NS::ImageQuality_E)src.enImageQuality;
    dst.stTimeInterval.unInterval = src.unInterval;
    dst.stTimeInterval.enTimeUnit = (Capture_NS::TimeUnit_E)src.enTimeUnit;
    dst.unNumber = src.unNumber;
}

void FillCapturePlan(const Capture_NS::CapturePlan_S &src, NET_CapturePlanInfo_S &dst)
{
    std::memset(&dst, 0, sizeof(dst));
    for (size_t i = 0; i < NET_PLAN_DAY_NUM_AWEEK; ++i)
    {
        dst.astDaySchedules[i].nDayOfWeek = (INT32)(i + 1);
        dst.astDaySchedules[i].udwTimeCount = 1;
        dst.astDaySchedules[i].astTimes[0].nStartTime = 0;
        dst.astDaySchedules[i].astTimes[0].nEndTime = 24 * 60 * 60;
    }

    const size_t dayCount = src.vstDaySchedules.size();
    for (size_t i = 0; i < dayCount; ++i)
    {
        const Capture_NS::DaySchedule_S &day = src.vstDaySchedules[i];
        int nDayOfWeek = (int)day.enDayOfWeek;
        if (nDayOfWeek < 1 || nDayOfWeek > (int)NET_PLAN_DAY_NUM_AWEEK)
            continue;
        NET_CaptureDaySchedule_S &outDay = dst.astDaySchedules[(size_t)nDayOfWeek - 1];
        outDay.nDayOfWeek = (INT32)nDayOfWeek;

        const size_t timeCount = day.captureTimes.size();
        const size_t n = (timeCount < NET_PLAN_TIME_SECTION_NUM_ADAY) ? timeCount : NET_PLAN_TIME_SECTION_NUM_ADAY;
        outDay.udwTimeCount = (UINT32)n;
        if (n == 0)
            continue;

        for (size_t j = 0; j < n; ++j)
        {
            outDay.astTimes[j].nStartTime = (INT32)day.captureTimes[j].nStartTime;
            outDay.astTimes[j].nEndTime = (INT32)day.captureTimes[j].nEndTime;
        }
    }
}

void ToCapturePlan(const NET_CapturePlanInfo_S &src, Capture_NS::CapturePlan_S &dst)
{
    dst.init_weekSchedule();
    for (size_t i = 0; i < NET_PLAN_DAY_NUM_AWEEK; ++i)
    {
        const NET_CaptureDaySchedule_S &inDay = src.astDaySchedules[i];
        int nDayOfWeek = inDay.nDayOfWeek;
        if (nDayOfWeek < 1 || nDayOfWeek > 7)
            nDayOfWeek = (int)i + 1;
        Capture_NS::DaySchedule_S &outDay = dst.vstDaySchedules[(size_t)nDayOfWeek - 1];
        outDay.enDayOfWeek = (Capture_NS::DayOfWeek_E)nDayOfWeek;

        outDay.captureTimes.clear();
        size_t n = (size_t)inDay.udwTimeCount;
        if (n > NET_PLAN_TIME_SECTION_NUM_ADAY)
            n = NET_PLAN_TIME_SECTION_NUM_ADAY;

        if (n == 0)
        {
            Capture_NS::CaptureTime_S t;
            t.nStartTime = 0;
            t.nEndTime = 24 * 60 * 60;
            outDay.captureTimes.push_back(t);
            continue;
        }

        outDay.captureTimes.resize(n);
        for (size_t j = 0; j < n; ++j)
        {
            outDay.captureTimes[j].nStartTime = inDay.astTimes[j].nStartTime;
            outDay.captureTimes[j].nEndTime = inDay.astTimes[j].nEndTime;
        }
    }
}

void FillCaptureParam(const Capture_NS::CaptureParam_S &src, NET_CaptureParamInfo_S &dst)
{
    std::memset(&dst, 0, sizeof(dst));
    FillOneCaptureConfig(src.stCaptureTimingConfig, dst.stCaptureTimingConfig);
    FillOneCaptureConfig(src.stCaptureEventConfig, dst.stCaptureEventConfig);
}

void ToCaptureParam(const NET_CaptureParamInfo_S &src, Capture_NS::CaptureParam_S &dst)
{
    ToOneCaptureConfig(src.stCaptureTimingConfig, dst.stCaptureTimingConfig);
    ToOneCaptureConfig(src.stCaptureEventConfig, dst.stCaptureEventConfig);
}
} // namespace TvSdkConvert

void TvSdkConvert::FillExposureInfo(const ISP::ExposureAttr_S &src, NET_ExposureInfo_S &dst)
{
    std::memset(&dst, 0, sizeof(dst));
    dst.enExpTime = (INT32)src.enExpTime;
    dst.bAntiBanding = src.bAntiBanding ? TRUE : FALSE;
}

void TvSdkConvert::ToExposureAttr(const NET_ExposureInfo_S &src, ISP::ExposureAttr_S &dst)
{
    dst.enExpTime = (ISP::ExpTimeMode_E)src.enExpTime;
    dst.bAntiBanding = (src.bAntiBanding == TRUE);
}

void TvSdkConvert::FillDayNightInfo(const ISP::DayNightAttr_S &src, NET_DayNightInfo_S &dst)
{
    std::memset(&dst, 0, sizeof(dst));
    dst.enDayNightMode = (INT32)src.enDayNightMode;
    dst.nBeginHour = (INT32)src.stBeginTime.nHour;
    dst.nBeginMinute = (INT32)src.stBeginTime.nMinute;
    dst.nBeginSecond = (INT32)src.stBeginTime.nSecond;
    dst.nBeginMilliSec = (INT32)src.stBeginTime.nMilliSec;
    dst.nEndHour = (INT32)src.stEndTime.nHour;
    dst.nEndMinute = (INT32)src.stEndTime.nMinute;
    dst.nEndSecond = (INT32)src.stEndTime.nSecond;
    dst.nEndMilliSec = (INT32)src.stEndTime.nMilliSec;
    dst.nSensitivityLevel = src.nSensitivityLevel;
    dst.nFilterTime = src.nFilterTime;
    dst.bFillLightExp = src.bFillLightExp ? TRUE : FALSE;
    dst.enLightMode = (INT32)src.enLightMode;
    dst.enLightType = (INT32)src.stFillLight.enLightType;
    dst.bWhiteLightEnable = src.stFillLight.stWhiteAttr.bEnable ? TRUE : FALSE;
    dst.nWhiteLightLevel = src.stFillLight.stWhiteAttr.nLightLevel;
    dst.bRedLightEnable = src.stFillLight.stRedAttr.bEnable ? TRUE : FALSE;
    dst.nRedLightLevel = src.stFillLight.stRedAttr.nLightLevel;
}

void TvSdkConvert::ToDayNightAttr(const NET_DayNightInfo_S &src, ISP::DayNightAttr_S &dst)
{
    dst.enDayNightMode = (ISP::DayNightMode_E)src.enDayNightMode;
    dst.stBeginTime.nHour = (unsigned int)src.nBeginHour;
    dst.stBeginTime.nMinute = (unsigned int)src.nBeginMinute;
    dst.stBeginTime.nSecond = (unsigned int)src.nBeginSecond;
    dst.stBeginTime.nMilliSec = (unsigned int)src.nBeginMilliSec;
    dst.stEndTime.nHour = (unsigned int)src.nEndHour;
    dst.stEndTime.nMinute = (unsigned int)src.nEndMinute;
    dst.stEndTime.nSecond = (unsigned int)src.nEndSecond;
    dst.stEndTime.nMilliSec = (unsigned int)src.nEndMilliSec;
    dst.nSensitivityLevel = src.nSensitivityLevel;
    dst.nFilterTime = src.nFilterTime;
    dst.bFillLightExp = (src.bFillLightExp == TRUE);
    dst.enLightMode = (ISP::LightBrightMode_E)src.enLightMode;
    dst.stFillLight.enLightType = (ISP::LightType_E)src.enLightType;
    dst.stFillLight.stWhiteAttr.bEnable = (src.bWhiteLightEnable == TRUE);
    dst.stFillLight.stWhiteAttr.nLightLevel = src.nWhiteLightLevel;
    dst.stFillLight.stRedAttr.bEnable = (src.bRedLightEnable == TRUE);
    dst.stFillLight.stRedAttr.nLightLevel = src.nRedLightLevel;
}

void TvSdkConvert::FillBackLightInfo(const ISP::BackLightArrt_S &src, NET_BackLightInfo_S &dst)
{
    std::memset(&dst, 0, sizeof(dst));
    dst.enBackLightArea = (INT32)src.enBackLightArea;
    dst.bWdrEnable = src.stWdrAttr.bEnable ? TRUE : FALSE;
    dst.nWdrLevel = src.stWdrAttr.nWdrLevel;
    dst.bHlsEnable = src.stHlsAttr.bEnable ? TRUE : FALSE;
    dst.nHlsLevel = src.stHlsAttr.nHlsLevel;
}

void TvSdkConvert::ToBackLightAttr(const NET_BackLightInfo_S &src, ISP::BackLightArrt_S &dst)
{
    dst.enBackLightArea = (ISP::BackLightArea_E)src.enBackLightArea;
    dst.stWdrAttr.bEnable = (src.bWdrEnable == TRUE);
    dst.stWdrAttr.nWdrLevel = src.nWdrLevel;
    dst.stHlsAttr.bEnable = (src.bHlsEnable == TRUE);
    dst.stHlsAttr.nHlsLevel = src.nHlsLevel;
}

void TvSdkConvert::FillDenoiseInfo(const ISP::DnrAttr_S &src, NET_DenoiseInfo_S &dst)
{
    std::memset(&dst, 0, sizeof(dst));
    dst.enDnrMode = (INT32)src.enDnrMode;
    dst.nDnrLevel = src.nDnrLevel;
    dst.nSnrLevel = src.nSnrLevel;
    dst.nTnrLevel = src.nTnrLevel;
}

void TvSdkConvert::ToDnrAttr(const NET_DenoiseInfo_S &src, ISP::DnrAttr_S &dst)
{
    dst.enDnrMode = (ISP::DnrMode_E)src.enDnrMode;
    dst.nDnrLevel = src.nDnrLevel;
    dst.nSnrLevel = src.nSnrLevel;
    dst.nTnrLevel = src.nTnrLevel;
}

void TvSdkConvert::FillWhiteBalanceInfo(const ISP::AwbAttr_S &src, NET_WhiteBalanceInfo_S &dst)
{
    std::memset(&dst, 0, sizeof(dst));
    dst.enAwbMode = (INT32)src.enAwbMode;
    dst.nRGain = src.nRGain;
    dst.nBGain = src.nBGain;
}

void TvSdkConvert::ToAwbAttr(const NET_WhiteBalanceInfo_S &src, ISP::AwbAttr_S &dst)
{
    dst.enAwbMode = (ISP::AwbMode_E)src.enAwbMode;
    dst.nRGain = src.nRGain;
    dst.nBGain = src.nBGain;
}


void TvSdkConvert::FillTalkbackStateInfo(const Preview::IntercomInfo_S &src, NET_TalkbackStateInfo_S &dst)
{
    std::memset(&dst, 0, sizeof(dst));
    dst.bEnable = src.bEnable ? TRUE : FALSE;
    std::strncpy(dst.szSdp, src.strSdp.c_str(), sizeof(dst.szSdp) - 1);
    std::strncpy(dst.szUrl, src.strUrl.c_str(), sizeof(dst.szUrl) - 1);
    std::strncpy(dst.szLocalIP, src.strLocalIp.c_str(), sizeof(dst.szLocalIP) - 1);
}

void TvSdkConvert::ToIntercomInfo(const NET_TalkbackStateInfo_S &src, Preview::IntercomInfo_S &dst)
{
    dst.bEnable = (src.bEnable == TRUE);
    dst.strSdp = src.szSdp;
    dst.strUrl = src.szUrl;
    dst.strLocalIp = src.szLocalIP;
}

void TvSdkConvert::FillTalkbackStreamInfo(const Replay::Stream::Info_S &src, NET_TalkbackStreamInfo_S &dst)
{
    std::memset(&dst, 0, sizeof(dst));
    std::strncpy(dst.szHost, src.host.c_str(), sizeof(dst.szHost) - 1);
    dst.nPort = src.nPort;
    dst.nChnId = src.nChnId;
    dst.nUserID = src.nUserId;
    dst.bMainStream = src.bMainStream ? TRUE : FALSE;
    std::strncpy(dst.szProtocol, src.protocol.c_str(), sizeof(dst.szProtocol) - 1);
    std::strncpy(dst.szStartTime, src.startTime.c_str(), sizeof(dst.szStartTime) - 1);
    std::strncpy(dst.szEndTime, src.endTime.c_str(), sizeof(dst.szEndTime) - 1);
    std::strncpy(dst.szFileName, src.filename.c_str(), sizeof(dst.szFileName) - 1);
}

void TvSdkConvert::ToReplayStreamInfo(const NET_TalkbackStreamInfo_S &src, Replay::Stream::Info_S &dst)
{
    dst.host = src.szHost;
    dst.nPort = src.nPort;
    dst.nChnId = src.nChnId;
    dst.nUserId = src.nUserID;
    dst.bMainStream = (src.bMainStream == TRUE);
    dst.protocol = src.szProtocol;
    dst.startTime = src.szStartTime;
    dst.endTime = src.szEndTime;
    dst.filename = src.szFileName;
}

void TvSdkConvert::FillReplayTalkbackInfo(const Replay::Stream::ReplayRtpInfo_S &src, NET_ReplayTalkbackInfo_S &dst)
{
    std::memset(&dst, 0, sizeof(dst));
    std::strncpy(dst.szNvrIp, src.nvrIp.c_str(), sizeof(dst.szNvrIp) - 1);
    std::strncpy(dst.szRemoteIp, src.remoteIp.c_str(), sizeof(dst.szRemoteIp) - 1);
    FillTalkbackStreamInfo(src.ipcInfo, dst.stIPCInfo);
}

void TvSdkConvert::ToReplayRtpInfo(const NET_ReplayTalkbackInfo_S &src, Replay::Stream::ReplayRtpInfo_S &dst)
{
    dst.nvrIp = src.szNvrIp;
    dst.remoteIp = src.szRemoteIp;
    TvSdkConvert::ToReplayStreamInfo(src.stIPCInfo, dst.ipcInfo);
}

static void FillPolygonPoints(const Alarm::Region_S &src, INT32 &pointCount, FLOAT pointX[32], FLOAT pointY[32])
{
    pointCount = (INT32)std::min<size_t>(src.aPoint.size(), 32);
    for (int p = 0; p < pointCount; ++p)
    {
        pointX[p] = src.aPoint[p].fX;
        pointY[p] = src.aPoint[p].fY;
    }
}

// --------- ParkingDetect (IPC ParkingDetection_S <-> SDK NET_ParkingAlarmInfo_S) ---------

void TvSdkConvert::FillParkingDetectAlarmInfo(const Alarm::ParkingDetection_S &src, NET_ParkingAlarmInfo_S &dst)
{
    std::memset(&dst, 0, sizeof(dst));
    dst.bEnable = src.bEnable ? TRUE : FALSE;
    dst.uRuleCount = 0;
    for (size_t i = 0; i < src.aRule.size() && i < 4; ++i)
    {
        const auto &r = src.aRule[i];
        auto &out = dst.astRule[i];
        std::memset(&out, 0, sizeof(out));
        TvSdkConvert::FillPolygonPoints(r.stRegion, out.uPointCount, out.afPointX, out.afPointY);
        out.nSensitivity = (INT32)r.nSensitivity;
        out.nTimeThreshold = (INT32)r.nTimeThreshold;
        dst.uRuleCount++;
    }

    if (!src.aAlarmTime.empty())
    {
        for (int day = 0; day < 7; ++day)
        {
            if (day >= (int)src.aAlarmTime.size())
                break;
            const auto &vecDay = src.aAlarmTime[day];
            int cnt = (int)std::min<size_t>(vecDay.size(), NET_PLAN_SECTION_NUM);
            dst.stAlarmSchedule.uTimeSectionCount[day] = cnt;
            for (int seg = 0; seg < cnt; ++seg)
            {
                TvSdkConvert::FillSchedTime(vecDay[seg], dst.stAlarmSchedule.astTimeSection[day][seg]);
            }
        }
    }
}

void TvSdkConvert::ToParkingDetection(const NET_ParkingAlarmInfo_S &src, Alarm::ParkingDetection_S &dst)
{
    dst.bEnable = (src.bEnable == TRUE);
    dst.aRule.clear();
    for (int i = 0; i < src.uRuleCount && i < 4; ++i)
    {
        const auto &r = src.astRule[i];
        Alarm::ParkingRule_S out;
        TvSdkConvert::ToRegionFromPolygon(r.uPointCount, r.afPointX, r.afPointY, out.stRegion);
        out.nSensitivity = (unsigned int)r.nSensitivity;
        out.nTimeThreshold = (unsigned int)r.nTimeThreshold;
        dst.aRule.push_back(out);
    }

    dst.aAlarmTime.clear();
    dst.aAlarmTime.resize(7);
    for (int day = 0; day < 7; ++day)
    {
        int cnt = src.stAlarmSchedule.uTimeSectionCount[day];
        if (cnt <= 0)
            continue;
        cnt = std::min(cnt, NET_PLAN_SECTION_NUM);
        dst.aAlarmTime[day].resize(cnt);
        for (int seg = 0; seg < cnt; ++seg)
        {
            TvSdkConvert::ToSchedTime(src.stAlarmSchedule.astTimeSection[day][seg], dst.aAlarmTime[day][seg]);
        }
    }
}

// --------- UnattendedObject (IPC UnattendedObject_S <-> SDK NET_UnattendedObjectAlarmInfo_S) ---------
void TvSdkConvert::FillUnattendedObjectAlarmInfo(const Alarm::UnattendedObject_S &src, NET_UnattendedObjectAlarmInfo_S &dst)
{
    std::memset(&dst, 0, sizeof(dst));
    dst.bEnable = src.bEnable ? TRUE : FALSE;
    dst.uRuleCount = 0;
    for (size_t i = 0; i < src.aRule.size() && i < 4; ++i)
    {
        const auto &r = src.aRule[i];
        auto &out = dst.stRule[i];
        std::memset(&out, 0, sizeof(out));
        TvSdkConvert::FillPolygonPoints(r.stRegion, out.uPointCount, out.afPointX, out.afPointY);
        out.nSensitivity = (INT32)r.nSensitivity;
        out.nTimeThreshold = (INT32)r.nTimeThreshold;
        dst.uRuleCount++;
    }

    if (!src.aAlarmTime.empty())
    {
        for (int day = 0; day < 7; ++day)
        {
            if (day >= (int)src.aAlarmTime.size())
                break;
            const auto &vecDay = src.aAlarmTime[day];
            int cnt = (int)std::min<size_t>(vecDay.size(), NET_PLAN_SECTION_NUM);
            dst.stAlarmSchedule.uTimeSectionCount[day] = cnt;
            for (int seg = 0; seg < cnt; ++seg)
            {
               TvSdkConvert::FillSchedTime(vecDay[seg], dst.stAlarmSchedule.astTimeSection[day][seg]);
            }
        }
    }
}

void TvSdkConvert::ToUnattendedObject(const NET_UnattendedObjectAlarmInfo_S &src, Alarm::UnattendedObject_S &dst)
{
    dst.bEnable = (src.bEnable == TRUE);
    dst.aRule.clear();
    for (int i = 0; i < src.uRuleCount && i < 4; ++i)
    {
        const auto &r = src.stRule[i];
        Alarm::UnattendedObjectRule_S out;
        TvSdkConvert::ToRegionFromPolygon(r.uPointCount, r.afPointX, r.afPointY, out.stRegion);
        out.nSensitivity = (unsigned int)r.nSensitivity;
        out.nTimeThreshold = (unsigned int)r.nTimeThreshold;
        dst.aRule.push_back(out);
    }

    dst.aAlarmTime.clear();
    dst.aAlarmTime.resize(7);
    for (int day = 0; day < 7; ++day)
    {
        int cnt = src.stAlarmSchedule.uTimeSectionCount[day];
        if (cnt <= 0)
            continue;
        cnt = std::min(cnt, NET_PLAN_SECTION_NUM);
        dst.aAlarmTime[day].resize(cnt);
        for (int seg = 0; seg < cnt; ++seg)
        {
            TvSdkConvert::ToSchedTime(src.stAlarmSchedule.astTimeSection[day][seg], dst.aAlarmTime[day][seg]);
        }
    }
}

// --------- ObjectRemoval (IPC ObjectRemoval_S <-> SDK NET_ObjectRemovalAlarmInfo_S) ---------
void TvSdkConvert::FillObjectRemovalAlarmInfo(const Alarm::ObjectRemoval_S &src, NET_ObjectRemovalAlarmInfo_S &dst)
{
    std::memset(&dst, 0, sizeof(dst));
    dst.bEnable = src.bEnable ? TRUE : FALSE;
    dst.uRuleCount = 0;
    for (size_t i = 0; i < src.aRule.size() && i < 4; ++i)
    {
        const auto &r = src.aRule[i];
        auto &out = dst.stRule[i];
        std::memset(&out, 0, sizeof(out));
        TvSdkConvert::FillPolygonPoints(r.stRegion, out.uPointCount, out.afPointX, out.afPointY);
        out.nSensitivity = (INT32)r.nSensitivity;
        out.nTimeThreshold = (INT32)r.nTimeThreshold;
        dst.uRuleCount++;
    }

    if (!src.aAlarmTime.empty())
    {
        for (int day = 0; day < 7; ++day)
        {
            if (day >= (int)src.aAlarmTime.size())
                break;
            const auto &vecDay = src.aAlarmTime[day];
            int cnt = (int)std::min<size_t>(vecDay.size(), NET_PLAN_SECTION_NUM);
            dst.stAlarmSchedule.uTimeSectionCount[day] = cnt;
            for (int seg = 0; seg < cnt; ++seg)
            {
                TvSdkConvert::FillSchedTime(vecDay[seg], dst.stAlarmSchedule.astTimeSection[day][seg]);
            }
        }
    }
}

void TvSdkConvert::ToObjectRemoval(const NET_ObjectRemovalAlarmInfo_S &src, Alarm::ObjectRemoval_S &dst)
{
    dst.bEnable = (src.bEnable == TRUE);
    dst.aRule.clear();
    for (int i = 0; i < src.uRuleCount && i < 4; ++i)
    {
        const auto &r = src.stRule[i];
        Alarm::ObjectRemovalRule_S out;
        TvSdkConvert::ToRegionFromPolygon(r.uPointCount, r.afPointX, r.afPointY, out.stRegion);
        out.nSensitivity = (unsigned int)r.nSensitivity;
        out.nTimeThreshold = (unsigned int)r.nTimeThreshold;
        dst.aRule.push_back(out);
    }

    dst.aAlarmTime.clear();
    dst.aAlarmTime.resize(7);
    for (int day = 0; day < 7; ++day)
    {
        int cnt = src.stAlarmSchedule.uTimeSectionCount[day];
        if (cnt <= 0)
            continue;
        cnt = std::min(cnt, NET_PLAN_SECTION_NUM);
        dst.aAlarmTime[day].resize(cnt);
        for (int seg = 0; seg < cnt; ++seg)
        {
            TvSdkConvert::ToSchedTime(src.stAlarmSchedule.astTimeSection[day][seg], dst.aAlarmTime[day][seg]);
        }
    }
}

void TvSdkConvert::FillAudioCfg(const Audio_NS::AudioConfig_S &src, NET_AudioCfg_S &dst)
{
    std::memset(&dst, 0, sizeof(dst));
    dst.bAudioSwitch = src.bAudioSwitch ? TRUE : FALSE;
    dst.enInputType = (INT32)src.enInputType;
    dst.enFormat = (INT32)src.enFormat;
    dst.enSampRate = (INT32)src.enSampRate;
    dst.enBitRate = (INT32)src.enBitRate;
    dst.u32InputVolume = src.u32InputVolume;
    dst.bDenoise = src.bDenoise ? TRUE : FALSE;
    dst.enOutputType = (INT32)src.enOutputType;
    dst.u32OutputVolume = src.u32OutputVolume;
}

void TvSdkConvert::ToAudioConfig(const NET_AudioCfg_S &src, Audio_NS::AudioConfig_S &dst)
{
    dst.bAudioSwitch = (src.bAudioSwitch == TRUE);
    dst.enInputType = (Audio_NS::AudioInputType_E)src.enInputType;
    dst.enFormat = (Audio_NS::AudioFormat_E)src.enFormat;
    dst.enSampRate = (Audio_NS::AudioSamprate_E)src.enSampRate;
    dst.enBitRate = (Audio_NS::AudioBitrate_E)src.enBitRate;
    dst.u32InputVolume = src.u32InputVolume;
    dst.bDenoise = (src.bDenoise == TRUE);
    dst.enOutputType = (Audio_NS::AudioOutputType_E)src.enOutputType;
    dst.u32OutputVolume = src.u32OutputVolume;
}

// --------- EnterRegion (IPC EntranceDetection_S <-> SDK NET_EnterRegionAlarmInfo_S) ---------
void TvSdkConvert::FillEnterRegionAlarmInfo(const Alarm::EntranceDetection_S &src, NET_EnterRegionAlarmInfo_S &dst)
{
    std::memset(&dst, 0, sizeof(dst));
    dst.bEnable = src.bEnable ? TRUE : FALSE;
    dst.uRuleCount = 0;
    for (size_t i = 0; i < src.aRule.size() && i < 4; ++i)
    {
        const auto &r = src.aRule[i];
        auto &out = dst.stRule[i];
        std::memset(&out, 0, sizeof(out));
        out.bEnable = TRUE;
        out.uPointCount = (INT32)std::min<size_t>(r.stRegion.aPoint.size(), 32);
        for (int p = 0; p < out.uPointCount; ++p)
        {
            out.afPointX[p] = r.stRegion.aPoint[p].fX;
            out.afPointY[p] = r.stRegion.aPoint[p].fY;
        }
        out.nTimeThreshold = (INT32)r.nTimeThreshold;
        out.nSensitivity = (INT32)r.nSensitivity;
        FillDetectionTargets(r.aDetectionTarget, out.uDetectionTargetCount, out.auDetectionTarget);
        dst.uRuleCount++;
    }

    if (!src.aAlarmTime.empty())
    {
        for (int day = 0; day < 7; ++day)
        {
            if (day >= (int)src.aAlarmTime.size())
                break;
            const auto &vecDay = src.aAlarmTime[day];
            int cnt = (int)std::min<size_t>(vecDay.size(), NET_PLAN_SECTION_NUM);
            dst.stAlarmSchedule.uTimeSectionCount[day] = cnt;
            for (int seg = 0; seg < cnt; ++seg)
            {
                TvSdkConvert::FillSchedTime(vecDay[seg], dst.stAlarmSchedule.astTimeSection[day][seg]);
            }
        }
    }
}

void TvSdkConvert::ToEntranceDetection(const NET_EnterRegionAlarmInfo_S &src, Alarm::EntranceDetection_S &dst)
{
    dst.bEnable = (src.bEnable == TRUE);
    dst.aRule.clear();
    for (int i = 0; i < src.uRuleCount && i < 4; ++i)
    {
        const auto &r = src.stRule[i];
        Alarm::EnterExitIntrusion_S out;
        out.nTimeThreshold = (unsigned int)r.nTimeThreshold;
        out.nSensitivity = (unsigned int)r.nSensitivity;
        out.stRegion.aPoint.clear();
        out.stRegion.nPointNum = r.uPointCount;
        for (int p = 0; p < r.uPointCount && p < 32; ++p)
        {
            out.stRegion.aPoint.push_back({r.afPointX[p], r.afPointY[p]});
        }
        ToDetectionTargets(r.auDetectionTarget, r.uDetectionTargetCount, out.aDetectionTarget);
        dst.aRule.push_back(out);
    }

    dst.aAlarmTime.clear();
    dst.aAlarmTime.resize(7);
    for (int day = 0; day < 7; ++day)
    {
        int cnt = src.stAlarmSchedule.uTimeSectionCount[day];
        if (cnt <= 0)
            continue;
        cnt = std::min(cnt, NET_PLAN_SECTION_NUM);
        dst.aAlarmTime[day].resize(cnt);
        for (int seg = 0; seg < cnt; ++seg)
        {
            TvSdkConvert::ToSchedTime(src.stAlarmSchedule.astTimeSection[day][seg], dst.aAlarmTime[day][seg]);
        }
    }
}

// --------- LeaveRegion (IPC ExitingDetection_S <-> SDK NET_LeaveRegionAlarmInfo_S) ---------
void TvSdkConvert::FillLeaveRegionAlarmInfo(const Alarm::ExitingDetection_S &src, NET_LeaveRegionAlarmInfo_S &dst)
{
    std::memset(&dst, 0, sizeof(dst));
    dst.bEnable = src.bEnable ? TRUE : FALSE;
    dst.uRuleCount = 0;
    for (size_t i = 0; i < src.aRule.size() && i < 4; ++i)
    {
        const auto &r = src.aRule[i];
        auto &out = dst.stRule[i];
        std::memset(&out, 0, sizeof(out));
        out.bEnable = TRUE;
        out.uPointCount = (INT32)std::min<size_t>(r.stRegion.aPoint.size(), 32);
        for (int p = 0; p < out.uPointCount; ++p)
        {
            out.afPointX[p] = r.stRegion.aPoint[p].fX;
            out.afPointY[p] = r.stRegion.aPoint[p].fY;
        }
        out.nTimeThreshold = (INT32)r.nTimeThreshold;
        out.nSensitivity = (INT32)r.nSensitivity;
        FillDetectionTargets(r.aDetectionTarget, out.uDetectionTargetCount, out.auDetectionTarget);
        dst.uRuleCount++;
    }

    if (!src.aAlarmTime.empty())
    {
        for (int day = 0; day < 7; ++day)
        {
            if (day >= (int)src.aAlarmTime.size())
                break;
            const auto &vecDay = src.aAlarmTime[day];
            int cnt = (int)std::min<size_t>(vecDay.size(), NET_PLAN_SECTION_NUM);
            dst.stAlarmSchedule.uTimeSectionCount[day] = cnt;
            for (int seg = 0; seg < cnt; ++seg)
            {
                TvSdkConvert::FillSchedTime(vecDay[seg], dst.stAlarmSchedule.astTimeSection[day][seg]);
            }
        }
    }
}

void TvSdkConvert::ToExitingDetection(const NET_LeaveRegionAlarmInfo_S &src, Alarm::ExitingDetection_S &dst)
{
    dst.bEnable = (src.bEnable == TRUE);
    dst.aRule.clear();
    for (int i = 0; i < src.uRuleCount && i < 4; ++i)
    {
        const auto &r = src.stRule[i];
        Alarm::EnterExitIntrusion_S out;
        out.nTimeThreshold = (unsigned int)r.nTimeThreshold;
        out.nSensitivity = (unsigned int)r.nSensitivity;
        out.stRegion.aPoint.clear();
        out.stRegion.nPointNum = r.uPointCount;
        for (int p = 0; p < r.uPointCount && p < 32; ++p)
        {
            out.stRegion.aPoint.push_back({r.afPointX[p], r.afPointY[p]});
        }
        ToDetectionTargets(r.auDetectionTarget, r.uDetectionTargetCount, out.aDetectionTarget);
        dst.aRule.push_back(out);
    }

    dst.aAlarmTime.clear();
    dst.aAlarmTime.resize(7);
    for (int day = 0; day < 7; ++day)
    {
        int cnt = src.stAlarmSchedule.uTimeSectionCount[day];
        if (cnt <= 0)
            continue;
        cnt = std::min(cnt, NET_PLAN_SECTION_NUM);
        dst.aAlarmTime[day].resize(cnt);
        for (int seg = 0; seg < cnt; ++seg)
        {
            TvSdkConvert::ToSchedTime(src.stAlarmSchedule.astTimeSection[day][seg], dst.aAlarmTime[day][seg]);
        }
    }
}

// --------- FaceCapture (IPC FaceCapture_S <-> SDK NET_FaceCaptureInfo_S) ---------
void TvSdkConvert::FillFaceCaptureInfo(const Alarm::FaceCapture_S &src, NET_FaceCaptureInfo_S &dst)
{
    std::memset(&dst, 0, sizeof(dst));
    dst.bEnable = src.bEnable ? TRUE : FALSE;

    dst.stRule.nSensitivity = (INT32)src.stRule.nSensitivity;

    dst.stRule.stRegion.uPointCount = (INT32)std::min<size_t>(src.stRule.stRegion.aPoint.size(), 32);
    for (int p = 0; p < dst.stRule.stRegion.uPointCount; ++p)
    {
        dst.stRule.stRegion.afPointX[p] = src.stRule.stRegion.aPoint[p].fX;
        dst.stRule.stRegion.afPointY[p] = src.stRule.stRegion.aPoint[p].fY;
    }

    dst.stRule.uShieldRegionCount = (INT32)std::min<size_t>(src.stRule.vstShieldedRegion.size(), 4);
    for (int i = 0; i < dst.stRule.uShieldRegionCount; ++i)
    {
        const auto &reg = src.stRule.vstShieldedRegion[i];
        auto &out = dst.stRule.astShieldRegion[i];
        out.uPointCount = (INT32)std::min<size_t>(reg.aPoint.size(), 32);
        for (int p = 0; p < out.uPointCount; ++p)
        {
            out.afPointX[p] = reg.aPoint[p].fX;
            out.afPointY[p] = reg.aPoint[p].fY;
        }
    }

    dst.stRule.nMinIpdRectLeft = src.stRule.stMinIpdRect.nX;
    dst.stRule.nMinIpdRectTop = src.stRule.stMinIpdRect.nY;
    dst.stRule.nMinIpdRectRight = src.stRule.stMinIpdRect.nX + src.stRule.stMinIpdRect.nWidth;
    dst.stRule.nMinIpdRectBottom = src.stRule.stMinIpdRect.nY + src.stRule.stMinIpdRect.nHeight;
    dst.stRule.nMinWidth = src.stRule.nMinWidth;
    dst.stRule.nMinHeight = src.stRule.nMinHeight;
    dst.stRule.nMaxWidth = src.stRule.nMaxWidth;
    dst.stRule.nMaxHeight = src.stRule.nMaxHeight;
    dst.stRule.nInterval = src.stRule.nInterval;

    if (!src.aAlarmTime.empty())
    {
        for (int day = 0; day < 7; ++day)
        {
            if (day >= (int)src.aAlarmTime.size())
                break;
            const auto &vecDay = src.aAlarmTime[day];
            int cnt = (int)std::min<size_t>(vecDay.size(), NET_PLAN_SECTION_NUM);
            dst.stAlarmSchedule.uTimeSectionCount[day] = cnt;
            for (int seg = 0; seg < cnt; ++seg)
            {
                FillSchedTime(vecDay[seg], dst.stAlarmSchedule.astTimeSection[day][seg]);
            }
        }
    }
}

void TvSdkConvert::ToFaceCapture(const NET_FaceCaptureInfo_S &src, Alarm::FaceCapture_S &dst)
{
    dst.bEnable = (src.bEnable == TRUE);

    dst.stRule.nSensitivity = (unsigned int)src.stRule.nSensitivity;

    dst.stRule.stRegion.aPoint.clear();
    int pointCnt = std::max(0, std::min(src.stRule.stRegion.uPointCount, 32));
    dst.stRule.stRegion.nPointNum = (unsigned int)pointCnt;
    for (int p = 0; p < pointCnt; ++p)
    {
        Common::PosF_S pt;
        pt.fX = src.stRule.stRegion.afPointX[p];
        pt.fY = src.stRule.stRegion.afPointY[p];
        dst.stRule.stRegion.aPoint.push_back(pt);
    }

    dst.stRule.vstShieldedRegion.clear();
    int shieldCnt = std::max(0, std::min(src.stRule.uShieldRegionCount, 4));
    for (int i = 0; i < shieldCnt; ++i)
    {
        const auto &inReg = src.stRule.astShieldRegion[i];
        Alarm::Region_S outReg;
        outReg.aPoint.clear();
        int shieldPointCnt = std::max(0, std::min(inReg.uPointCount, 32));
        outReg.nPointNum = (unsigned int)shieldPointCnt;
        for (int p = 0; p < shieldPointCnt; ++p)
        {
            Common::PosF_S pt;
            pt.fX = inReg.afPointX[p];
            pt.fY = inReg.afPointY[p];
            outReg.aPoint.push_back(pt);
        }
        dst.stRule.vstShieldedRegion.push_back(outReg);
    }

    dst.stRule.stMinIpdRect.nX = src.stRule.nMinIpdRectLeft;
    dst.stRule.stMinIpdRect.nY = src.stRule.nMinIpdRectTop;
    dst.stRule.stMinIpdRect.nWidth = src.stRule.nMinIpdRectRight - src.stRule.nMinIpdRectLeft;
    dst.stRule.stMinIpdRect.nHeight = src.stRule.nMinIpdRectBottom - src.stRule.nMinIpdRectTop;
    dst.stRule.nMinWidth = src.stRule.nMinWidth;
    dst.stRule.nMinHeight = src.stRule.nMinHeight;
    dst.stRule.nMaxWidth = src.stRule.nMaxWidth;
    dst.stRule.nMaxHeight = src.stRule.nMaxHeight;
    dst.stRule.nInterval = src.stRule.nInterval;

    dst.aAlarmTime.clear();
    dst.aAlarmTime.resize(7);
    for (int day = 0; day < 7; ++day)
    {
        int cnt = src.stAlarmSchedule.uTimeSectionCount[day];
        if (cnt <= 0)
            continue;
        cnt = std::min(cnt, NET_PLAN_SECTION_NUM);
        dst.aAlarmTime[day].resize(cnt);
        for (int seg = 0; seg < cnt; ++seg)
        {
            ToSchedTime(src.stAlarmSchedule.astTimeSection[day][seg], dst.aAlarmTime[day][seg]);
        }
    }
}

void TvSdkConvert::FillFaceCaptureOverlayInfo(const Alarm::OverlayInfo_S &src,
                                              NET_FaceCaptureOverlayInfo_S &dst)
{
    std::memset(&dst, 0, sizeof(dst));
    dst.nDeviceID = src.nDeviceID;
    std::strncpy(dst.strMonitoryPointInfo,
                 src.strMonitoryPointInfo.c_str(),
                 sizeof(dst.strMonitoryPointInfo) - 1);
    dst.bOverlayDeviceID = src.bOverlayDeviceID ? TRUE : FALSE;
    dst.bOverlayCaptureTime = src.bOverlayCaptureTime ? TRUE : FALSE;
    dst.bOverlayMonitoryPointInfo = src.bOverlayMonitoryPointInfo ? TRUE : FALSE;
    dst.enFontColor = static_cast<NET_OSD_COLOR_E>(src.enFontColor);
    std::strncpy(dst.strFontColor,
                 src.strFontColor.c_str(),
                 sizeof(dst.strFontColor) - 1);
}

void TvSdkConvert::ToFaceCaptureOverlayInfo(const NET_FaceCaptureOverlayInfo_S &src,
                                            Alarm::OverlayInfo_S &dst)
{
    dst.nDeviceID = src.nDeviceID;
    dst.strMonitoryPointInfo.assign(src.strMonitoryPointInfo,
                                    std::find(src.strMonitoryPointInfo,
                                              src.strMonitoryPointInfo + sizeof(src.strMonitoryPointInfo),
                                              '\0'));
    dst.bOverlayDeviceID = (src.bOverlayDeviceID == TRUE);
    dst.bOverlayCaptureTime = (src.bOverlayCaptureTime == TRUE);
    dst.bOverlayMonitoryPointInfo = (src.bOverlayMonitoryPointInfo == TRUE);
    dst.enFontColor = static_cast<Osd::OSD_COLOR_E>(src.enFontColor);
    dst.strFontColor.assign(src.strFontColor,
                            std::find(src.strFontColor,
                                      src.strFontColor + sizeof(src.strFontColor),
                                      '\0'));
}
