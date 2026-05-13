/***
 * @FilePath     : time_manage.h
 * @Author       : tianl (tianl@kfb.cn)
 * @Date         : 2024-11-01 13:54:54
 * @LastEditors  : huangjunda
 * @LastEditTime : 2025-03-28 13:55:40
 * @Description  : 时间管理类
 */

#pragma once

#include <cstring>
#include <string>
#include <iostream>
#include "ntp_client.h"
#include "IpcRet.h"
#include "dlog.h"

/* 默认字符串日期时间格式 */
#define DATE_TIME_FORMAT_YYYYMMDD_DEFAULT "%Y-%m-%d %H:%M:%S"
#define DATE_TIME_FORMAT_MMDDYYYY_DEFAULT "%m-%d-%Y %H:%M:%S"
#define DATE_TIME_FORMAT_DDMMYYYY_DEFAULT "%d-%m-%Y %H:%M:%S"

#define TIME_PERIOD_HOUR_12 12

class CTimeManage : public CSingleton<CTimeManage>
{
	CTimeManage();

public:
	virtual ~CTimeManage() = default;
	friend class CSingleton<CTimeManage>; // 允许 Singleton 访问私有构造函数
	/**
	 * @brief 初始化时间信息
	 * @return int
	 */
	int init_time_info(System::TimeInfo_S stNewTimeInfo);
	/**
	 * @brief 获取设备时区
	 */
	std::string get_current_zone(System::TimeZone_E enTimeZone);
	/**
	 * @brief 获取设备星期
	 */
	std::string get_current_week(System::Language_E enLanguage);
	/**
	 * @brief 获取设备时间
	 */
	std::string get_current_time(System::Language_E enLanguage, System::DateFormat_E enFormat);
	/**
	 * @brief 获取设备12小时制式时间
	 */
	std::string get_current_time12(System::Language_E enLanguage, System::DateFormat_E enFormat);
	/**
	 * @brief 设置时间信息
	 * @return int
	 */
	int set_time_info(System::TimeInfo_S stNewTimeInfo);

	/**
	 * @brief 获取时间信息
	 * @param stNewTimeInfo
	 */
	void get_time_info(System::TimeInfo_S &stTimeInfo);

	/**
	 * @brief 初始化时间配置
	 * @return IpcRet_E
	 */
	IpcRet_E init();
	/**
	 * @brief 去初始化时间配置
	 * @return IpcRet_E
	 */
	IpcRet_E deinit();

	/*** 获取设备当前时间，时间格式为2025-06-30T16:55:03
	 * @description : 
	 * @author      : cyc
	 * @return       string
	 */	
	std::string get_device_time();

	/**
	 * @brief ntp测试
	 * @return int 
	 */
	int ntp_test(System::TestNtp_S stTestNtp,std::function<void( int)> result);

private:
	/// @brief 时间信息
	System::TimeInfo_S stTimeInfo;
	/**
	 * @brief 配置文件
	 */
	std::string m_timeConfigFile;
	/**
	 * @brief 同步设备时间
	 * @param nTime 时间戳
	 * @return int
	 */
	int sync_time(int nTime);

	/**
	 * @brief 日期时间字符串转换为时间戳
	 * @param pDateTime 日期时间
	 * @return int
	 */
	int get_time_t(const char *pDateTime);
};