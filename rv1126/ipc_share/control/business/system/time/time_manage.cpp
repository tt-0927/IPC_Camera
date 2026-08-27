/**
 * @FilePath     : time_manage.cpp
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2024-11-01 13:54:54
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-07-20 17:13:15
 * @Description  : 时间管理类
 */

#include <iostream>
#include <fstream>
#include <iomanip>

#include "time_manage.h"
#include "get_time.h"
#include "ntp_client.h"
#include "system_convert.h"
#include "system_manage.h"
#include "data_length.h"
#include "convert_interface.h"
#include "path_define.h"
#include "timezone_runtime.h"
#include "record_ctrl.h"
#include "record_file_manage.h"
#include "rtsp_server.h"

namespace
{
/**
 * @brief   : 获取系统时间变化来源的可读名称
 * @param    {SystemTimeChangeSource_E} enSource：时间变化来源
 * @return   {const char*} 用于日志的中文来源名称
 */
const char *get_time_change_source_name(SystemTimeChangeSource_E enSource)
{
    switch (enSource)
    {
    case SystemTimeChangeSource_E::MANUAL:
        return "手动校时";
    case SystemTimeChangeSource_E::NTP:
        return "NTP校时";
    case SystemTimeChangeSource_E::ONVIF:
        return "ONVIF校时";
    case SystemTimeChangeSource_E::TIMEZONE:
        return "时区变化";
    default:
        return "未知来源";
    }
}
} // namespace

CTimeManage::CTimeManage()
    : m_timeConfigFile(TIME_CONFIG_FILE)
{
}

IpcRet_E CTimeManage::init()
{
    System::TimeInfo_S stNewTimeInfo;
    if (Convert::read_file(m_timeConfigFile, stNewTimeInfo))
    {
        Convert::write_file(m_timeConfigFile, stNewTimeInfo);
    }
    stTimeInfo = stNewTimeInfo;

    /* time.json 是业务时区配置源，启动时同步到 shell 配置，保证其他进程读取同一份 POSIX 时区 */
    std::string strTimezone = TimezoneRuntime_NS::to_posix_timezone(static_cast<int>(stNewTimeInfo.enTimeZone));
    dlog_info("时间模块初始化时区, 枚举:%d, POSIX时区:%s", static_cast<int>(stNewTimeInfo.enTimeZone), strTimezone.c_str());
    TimezoneRuntime_NS::write_timezone_config(strTimezone);
    TimezoneRuntime_NS::reload_timezone("stream", "time_manage_init");

    /* NTP线程只上报时钟变化，具体业务消费者由控制组合根后续注册。 */
    CNtpClient::instance()->set_time_changed_callback(
        [this](std::time_t nOldTime, std::time_t nNewTime)
        {
            on_ntp_time_changed(nOldTime, nNewTime);
        });
    init_time_info(stNewTimeInfo);
    return OK;
}

IpcRet_E CTimeManage::deinit()
{
    /* 先解除NTP回调，防止停止过程中NTP线程继续访问时间管理对象。 */
    CNtpClient::instance()->clear_time_changed_callback();
    clear_time_change_callback();
    CNtpClient::instance()->deinit();
    return OK;
}

void CTimeManage::get_time_info(System::TimeInfo_S &stTimeInfo)
{
    Convert::read_file(m_timeConfigFile, stTimeInfo);
}

int CTimeManage::init_time_info(System::TimeInfo_S stNewTimeInfo)
{
    /*设置NTP校时*/
    if (stNewTimeInfo.bEnableNTPSync)
    {
        dlog_info("设置ntp校时");
        CNtpClient::instance()->init(stNewTimeInfo.stNTPInfo, stNewTimeInfo.enTimeZone);
    }
    /* 关闭NTP校时, 手动校时 */
    else if (!stNewTimeInfo.bEnableNTPSync)
    {
        dlog_info("关闭ntp校时");
        CNtpClient::instance()->stop();
    }
    return 0;
}

int CTimeManage::set_time_info(System::TimeInfo_S stNewTimeInfo)
{
    int nRet = 0;
    bool bTimezoneChanged = stNewTimeInfo.enTimeZone != stTimeInfo.enTimeZone;
    bool bManualTimeChanged = false;
    /* 校时前保留UTC时间戳，使日志可准确展示跳变幅度而不受时区切换影响。 */
    const std::time_t nOldTime = std::time(nullptr);

    if (bTimezoneChanged)
    {
        std::string strTimezone = TimezoneRuntime_NS::to_posix_timezone(static_cast<int>(stNewTimeInfo.enTimeZone));
        dlog_info("检测到时区配置变化, old:%d, new:%d, POSIX:%s",
                  static_cast<int>(stTimeInfo.enTimeZone),
                  static_cast<int>(stNewTimeInfo.enTimeZone),
                  strTimezone.c_str());
        if (TimezoneRuntime_NS::write_timezone_config(strTimezone) != OK)
        {
            dlog_error("写入时区配置失败, POSIX时区:%s", strTimezone.c_str());
            return ERR;
        }

        TimezoneRuntime_NS::reload_timezone("stream", "set_time_info");
    }

    stTimeInfo = stNewTimeInfo;

    /*设置NTP校时*/
    if (stNewTimeInfo.bEnableNTPSync)
    {
        dlog_info("设置ntp校时");
        CNtpClient::instance()->init(stNewTimeInfo.stNTPInfo, stNewTimeInfo.enTimeZone);
    }
    /* 关闭NTP校时, 手动校时 */
    else if (!stNewTimeInfo.bEnableNTPSync)
    {
        dlog_info("关闭ntp校时");
        CNtpClient::instance()->stop();
    }   
    
	/* 手动校时 */
	if(stNewTimeInfo.bManualSync)
	{ 
        dlog_info("手动校时");
        int nTimep;

        if (stNewTimeInfo.strDateTime.empty())
        {
            dlog_error("日期为空, 获取设备时间");
            stNewTimeInfo.strDateTime = get_current_time(System::Language_E::ENGLISH, stNewTimeInfo.enDateFormat);
        }

        if ((nTimep = get_time_t(stNewTimeInfo.strDateTime.c_str())) < 0)
        {
            dlog_error("转换时间戳失败");
            return ERR;
        }

        bool bSystemTimeUpdated = false;
        nRet = set_system_utc_time(static_cast<std::time_t>(nTimep),
                                   SystemTimeChangeSource_E::MANUAL,
                                   bTimezoneChanged,
                                   &bSystemTimeUpdated);
        bManualTimeChanged = bSystemTimeUpdated;
    }

    if (bTimezoneChanged && !bManualTimeChanged)
    {
        SystemTimeChangeInfo_S stChangeInfo;
        stChangeInfo.enSource = SystemTimeChangeSource_E::TIMEZONE;
        stChangeInfo.nOldTime = nOldTime;
        stChangeInfo.nNewTime = std::time(nullptr);
        stChangeInfo.bTimezoneChanged = true;
        const IpcRet_E enNotifyRet = notify_system_time_changed(stChangeInfo);
        if (nRet == OK)
        {
            nRet = enNotifyRet;
        }
    }

    stNewTimeInfo.strDateTime = get_current_time(System::Language_E::ENGLISH, stNewTimeInfo.enDateFormat);
    /* 时间设置信息保存到文件 */
    Convert::write_file(m_timeConfigFile, stNewTimeInfo);

    return nRet;
}

IpcRet_E CTimeManage::set_system_utc_time(std::time_t nNewTime,
                                          SystemTimeChangeSource_E enSource,
                                          bool bTimezoneChanged,
                                          bool *pbSystemTimeUpdated)
{
    if (pbSystemTimeUpdated != nullptr)
    {
        *pbSystemTimeUpdated = false;
    }

    SystemTimeChangeInfo_S stChangeInfo;
    stChangeInfo.enSource = enSource;
    stChangeInfo.nOldTime = std::time(nullptr);
    stChangeInfo.nNewTime = nNewTime;
    stChangeInfo.bTimezoneChanged = bTimezoneChanged;

    IpcRet_E enRet = sync_time(nNewTime);
    if (enRet != OK)
    {
        return enRet;
    }

    if (pbSystemTimeUpdated != nullptr)
    {
        *pbSystemTimeUpdated = true;
    }

    stChangeInfo.nNewTime = std::time(nullptr);
    return notify_system_time_changed(stChangeInfo);
}

IpcRet_E CTimeManage::sync_time(std::time_t nTime)
{
    struct timespec ts;

    /* 设置秒数，纳秒设为0 */
    ts.tv_sec = nTime;
    ts.tv_nsec = 0;

    /* 设置系统时间 */
    if (clock_settime(CLOCK_REALTIME, &ts) != OK)
    {
        dlog_error("手动设置时间失败: %s", strerror(errno));
        return ERR;
    }
    else
    {
        dlog_info("手动设置时间成功");
        /* 同步写到RTC（硬件时钟）中，RTC统一保存UTC时间 */
        int nSyncRtcRet = system("hwclock -w -u");
        if (nSyncRtcRet != OK)
        {
            dlog_error("同步UTC时间到RTC失败: %s", strerror(errno));
        }
        else
        {
            dlog_info("同步UTC时间到RTC成功");
        }
    }

    return OK;
}

void CTimeManage::set_time_change_callback(TimeChangeCallback fnCallback)
{
    std::lock_guard<std::mutex> stLock(m_mtxTimeChangeCallback);
    m_fnTimeChangeCallback = std::move(fnCallback);
}

void CTimeManage::clear_time_change_callback()
{
    /* lock: 与正在执行的NTP/手动校时通知串行，返回后旧外设对象不会再被访问。 */
    std::lock_guard<std::mutex> stLock(m_mtxTimeChangeCallback);
    m_fnTimeChangeCallback = TimeChangeCallback{};
}

bool CTimeManage::need_rebuild_recording(const SystemTimeChangeInfo_S &stChangeInfo) const
{
    constexpr std::time_t RECORD_TIME_JUMP_THRESHOLD_SEC = 10;
    const std::time_t nDeltaTime = stChangeInfo.nNewTime - stChangeInfo.nOldTime;
    return stChangeInfo.bTimezoneChanged || nDeltaTime > RECORD_TIME_JUMP_THRESHOLD_SEC ||
           nDeltaTime < -RECORD_TIME_JUMP_THRESHOLD_SEC;
}

void CTimeManage::rebuild_recording_after_time_change(const SystemTimeChangeInfo_S &stChangeInfo)
{
    /* memory: 录制文件管理器为单例，录制停止完成前不能清理其m3u8和索引文件。 */
    if (!CRecordCtrl::instance()->is_running())
    {
        dlog_info("录制模块未运行，跳过系统时间变化后的录制重建, source:%s", get_time_change_source_name(stChangeInfo.enSource));
        return;
    }

    dlog_info("系统时间变化重建录制, source:%s", get_time_change_source_name(stChangeInfo.enSource));
    CRecordCtrl::instance()->stop_record();
    sleep(1);
    RecordFileManage::instance()->dealTimeChange(stChangeInfo.nNewTime);
}

IpcRet_E CTimeManage::notify_system_time_changed(const SystemTimeChangeInfo_S &stChangeInfo)
{
    /* lock: 执行期间禁止注销回调，控制模块可将clear作为外设销毁前的屏障。 */
    std::lock_guard<std::mutex> stLock(m_mtxTimeChangeCallback);
    dlog_info("系统时间变化, source:%s, old_utc:%lld, new_utc:%lld, delta_sec:%lld",
              get_time_change_source_name(stChangeInfo.enSource),
              static_cast<long long>(stChangeInfo.nOldTime),
              static_cast<long long>(stChangeInfo.nNewTime),
              static_cast<long long>(stChangeInfo.nNewTime - stChangeInfo.nOldTime));

    if (!m_fnTimeChangeCallback)
    {
        dlog_warn("系统时间变化后未注册业务重算回调，跳过下游通知, source:%s",
                  get_time_change_source_name(stChangeInfo.enSource));
        return OK;
    }

    IpcRet_E enFinalRet = OK;
    if (stChangeInfo.bTimezoneChanged && TimezoneRuntime_NS::notify_timezone_reload("stream") != OK)
    {
        enFinalRet = ERR;
    }

    if (need_rebuild_recording(stChangeInfo))
    {
        rebuild_recording_after_time_change(stChangeInfo);
    }

    const int nRtspRet = CRtspServer::instance()->reset_lastIdrRequestTime();
    if (nRtspRet != OK)
    {
        dlog_error("系统时间变化后重置RTSP IDR请求时间失败, ret:%d", nRtspRet);
        enFinalRet = ERR;
    }

    const int nRet = m_fnTimeChangeCallback(stChangeInfo);
    if (nRet != OK)
    {
        dlog_error("系统时间变化后业务重算失败, source:%s, ret:%d", get_time_change_source_name(stChangeInfo.enSource), nRet);
        enFinalRet = ERR;
    }
    return enFinalRet;
}

void CTimeManage::on_ntp_time_changed(std::time_t nOldTime, std::time_t nNewTime)
{
    SystemTimeChangeInfo_S stChangeInfo;
    stChangeInfo.enSource = SystemTimeChangeSource_E::NTP;
    stChangeInfo.nOldTime = nOldTime;
    stChangeInfo.nNewTime = nNewTime;
    notify_system_time_changed(stChangeInfo);
}

int CTimeManage::get_time_t(const char *pDateTime)
{
    if (pDateTime == NULL)
    {
        return ERR;
    }

    /* 解析日期时间字符串*/
    struct tm tm = {0};
    if (strptime(pDateTime, to_string(System::Language_E::ENGLISH, stTimeInfo.enDateFormat), &tm) == NULL)
    {
        dlog_error("解析日期时间失败");
        return ERR;
    }

    /* 将tm结构转换为time_t类型的时间戳 */
    time_t timep = mktime(&tm);
    if (timep == ERR)
    {
        return ERR;
    }

    return (int)timep;
}

/* 获取当前时区 */
std::string CTimeManage::get_current_zone(System::TimeZone_E enTimeZone)
{
    time_t current_time;
    struct tm *pTime_info;
    char achZone[LENGTH16] = {0};

    /* 格式化时区为指定格式 */
    snprintf(achZone, sizeof(achZone), to_string(enTimeZone));

    return std::string(achZone);
}

/* 获取当前星期几 */
std::string CTimeManage::get_current_week(System::Language_E enLanguage)
{
    time_t current_time;
    struct tm *pTime_info;
    char achWeek[LENGTH10] = {0};

    /* 获取当前时间戳 */
    time(&current_time);

    /* 转换为本地时间 */
    pTime_info = localtime(&current_time);

    /* 格式化日期为指定格式 */
    snprintf(achWeek, sizeof(achWeek),  to_string(enLanguage, (System::Week_E)pTime_info->tm_wday));

    return std::string(achWeek);
}

/* 获取当前时间 */
std::string CTimeManage::get_current_time(System::Language_E enLanguage, System::DateFormat_E enFormat)
{
    time_t current_time;
    struct tm *pTime_info;
    char achTime[LENGTH64] = {0};

    /* 获取当前时间戳 */
    time(&current_time);

    /* 转换为本地时间 */
    pTime_info = localtime(&current_time);

    /* 格式化时间为指定格式 */
    strftime(achTime, sizeof(achTime), to_string(enLanguage, enFormat), pTime_info);

    return std::string(achTime);
}

/* 获取当前时间段 */
std::string CTimeManage::get_current_time12(System::Language_E enLanguage, System::DateFormat_E enFormat)
{
    time_t current_time;
    struct tm *pTime_info;
    char achTime[LENGTH64] = {0};
    char achPeriod[LENGTH8] = {0};
    std::string strTime;

    /* 获取当前时间戳 */
    time(&current_time);

    /* 转换为本地时间 */
    pTime_info = localtime(&current_time);
    if (TIME_PERIOD_HOUR_12 >= pTime_info->tm_hour)
    {
        /* 格式化时间为指定格式 */
        strftime(achTime, sizeof(achTime), to_string(enLanguage, enFormat), pTime_info);

        /* 格式化时间段为指定格式 */
        snprintf(achPeriod, sizeof(achPeriod), to_string(enLanguage, System::Period_E::AM));      
    }
    else if (TIME_PERIOD_HOUR_12 < pTime_info->tm_hour)
    {
        pTime_info->tm_hour -= 12;
        /* 格式化时间为指定格式 */
        strftime(achTime, sizeof(achTime), to_string(enLanguage, enFormat), pTime_info);

        /* 格式化时间段为指定格式 */
        snprintf(achPeriod, sizeof(achPeriod), to_string(enLanguage, System::Period_E::PM));
    }

    if (System::Language_E::SIMP_CHINESE == enLanguage)
    {
        int nspacePos = std::string(achTime).find(' ');
        strTime = std::string(achTime).insert(nspacePos + 1, achPeriod);
    }
    else if (System::Language_E::ENGLISH == enLanguage)
    {
        strTime = std::string(achTime) + " " + std::string(achPeriod);
    }

    return strTime;
}

/* 获取设备当前时间，时间格式为2025-06-30T16:55:03 */
std::string CTimeManage::get_device_time() 
{
    // 获取当前时间
    std::time_t now = std::time(nullptr);
    std::tm* localTime = std::localtime(&now);

    // 创建一个字符串流用于格式化输出
    std::ostringstream oss;
    // 设置输出格式
    oss << std::put_time(localTime, "%Y-%m-%dT%H:%M:%S");
    return oss.str();
}

int CTimeManage::ntp_test(System::TestNtp_S stTestNtp,std::function<void( int)> result)
{
    auto thrRun = [this](System::TestNtp_S stTestNtp,std::function<void(int)> result)
    {
        int nRet = CNtpClient::instance()->test_ntp(stTestNtp);
        result(nRet);
    };

    std::thread thr(thrRun,stTestNtp, result);
   	thr.detach();

    return 0;
}
