/**
 * @FilePath     : time_manage.cpp
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2024-11-01 13:54:54
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-06-05 15:53:09
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

    init_time_info(stNewTimeInfo);
    return OK;
}

IpcRet_E CTimeManage::deinit()
{
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
        TimezoneRuntime_NS::notify_timezone_reload("stream");
         /* 时区切换后重启录制，清理时区切换导致的"未来"录制文件并重启录制 */
        dlog_info("时区切换, 重启录制");
        CRecordCtrl::instance()->stop_record();
        sleep(1);
        time_t nCurTime = time(NULL);
        RecordFileManage::instance()->dealTimeChange(nCurTime);
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

        nRet = sync_time(nTimep);

        /* 重置RtspServer上次请求IDR帧时间，防止无法请求出IDR帧 */
        CRtspServer::instance()->reset_lastIdrRequestTime();
    }

    stNewTimeInfo.strDateTime = get_current_time(System::Language_E::ENGLISH, stNewTimeInfo.enDateFormat);
    /* 时间设置信息保存到文件 */
    Convert::write_file(m_timeConfigFile, stNewTimeInfo);

    return nRet;
}

int CTimeManage::sync_time(int nTime)
{
    int nRet = ERR;
    struct timespec ts;

    /* 设置秒数，纳秒设为0 */
    ts.tv_sec = nTime;
    ts.tv_nsec = 0;

    /* 设置系统时间 */
    nRet = clock_settime(CLOCK_REALTIME, &ts);
    if (nRet != 0)
    {
        dlog_error("手动设置时间失败: %s", strerror(errno));
        return nRet;
    }
    else
    {
        dlog_info("手动设置时间成功");
        /* 同步写到RTC（硬件时钟）中，RTC统一保存UTC时间 */
        int nRet = system("hwclock -w -u");
        if (nRet != OK)
        {
            dlog_error("同步UTC时间到RTC失败: %s", strerror(errno));
        }
        else
        {
            dlog_info("同步UTC时间到RTC成功");
        }
    }

    return nRet;
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
