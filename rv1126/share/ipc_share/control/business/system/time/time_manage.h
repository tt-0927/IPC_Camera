/**
 * @FilePath     : time_manage.h
 * @Author       : tianl (tianl@kfb.cn)
 * @Date         : 2024-11-01 13:54:54
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-07-20 17:13:15
 * @Description  : 时间管理类
 */

#pragma once

#include <cstring>
#include <ctime>
#include <functional>
#include <string>
#include <iostream>
#include <mutex>
#include <utility>
#include "ntp_client.h"
#include "IpcRet.h"
#include "dlog.h"

/* 默认字符串日期时间格式 */
#define DATE_TIME_FORMAT_YYYYMMDD_DEFAULT "%Y-%m-%d %H:%M:%S"
#define DATE_TIME_FORMAT_MMDDYYYY_DEFAULT "%m-%d-%Y %H:%M:%S"
#define DATE_TIME_FORMAT_DDMMYYYY_DEFAULT "%d-%m-%Y %H:%M:%S"

#define TIME_PERIOD_HOUR_12 12

/**
 * @brief 系统墙钟变化的来源。
 */
enum class SystemTimeChangeSource_E
{
    MANUAL,   /* 网页手动校时。 */
    NTP,      /* NTP线程校时。 */
    ONVIF,    /* ONVIF协议校时。 */
    TIMEZONE, /* 时区配置改变，本地时间解释发生变化。 */
};

/**
 * @brief   : 系统时间变化上下文
 * @note    : nOldTime和nNewTime始终为UTC时间戳；时区变化不改变UTC秒数，
 *            通过bTimezoneChanged标识本地时间解释已变更。
 */
struct SystemTimeChangeInfo_S
{
    /* 变化来源。 */
    SystemTimeChangeSource_E enSource = SystemTimeChangeSource_E::MANUAL;
    /* 变化前UTC时间戳。 */
    std::time_t nOldTime = 0;
    /* 变化后UTC时间戳。 */
    std::time_t nNewTime = 0;
    /* 本地时区是否发生变化。 */
    bool bTimezoneChanged = false;
};

class CTimeManage : public CSingleton<CTimeManage>
{
	CTimeManage();

public:
    /* 系统时间变化消费者；返回值用于让时间模块记录下游重算是否成功。 */
    using TimeChangeCallback = std::function<int(const SystemTimeChangeInfo_S &stChangeInfo)>;

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
     * @brief   : 设置系统UTC时间并统一通知所有时间敏感模块
     * @param    {time_t} nNewTime：待设置的UTC时间戳
     * @param    {SystemTimeChangeSource_E} enSource：校时来源
     * @param    {bool} bTimezoneChanged：本地时区是否同时发生变化
     * @param    {bool*} pbSystemTimeUpdated：可选输出，物理系统时间是否已设置成功
     * @return   {IpcRet_E} OK：设置和通知成功，非OK：设置或通知失败
     * @note    : 外部校时入口必须调用本接口，禁止直接调用clock_settime或settimeofday。
     */
    IpcRet_E set_system_utc_time(std::time_t nNewTime,
                                 SystemTimeChangeSource_E enSource,
                                 bool bTimezoneChanged = false,
                                 bool *pbSystemTimeUpdated = nullptr);

    /**
     * @brief   : 分发已经生效的系统时间变化
     * @param    {SystemTimeChangeInfo_S} stChangeInfo：时间变化上下文
     * @return   {IpcRet_E} OK：所有可返回状态的消费者执行成功，非OK：存在失败
     * @note    : 本接口是时间跳变后的唯一通知入口，按固定顺序处理时区广播、录制、
     *            RTSP与外设等消费者；NTP等已自行设置墙钟的路径调用本接口。
     */
    IpcRet_E notify_system_time_changed(const SystemTimeChangeInfo_S &stChangeInfo);

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

    /**
     * @brief   : 设置系统时间变化回调
     * @param    {TimeChangeCallback} fnCallback：校时或时区变化后执行的下游重算回调
     * @return   {void}
     * @note    : 回调串行执行，clear_time_change_callback返回后不再访问旧回调。
     */
    void set_time_change_callback(TimeChangeCallback fnCallback);

    /**
     * @brief   : 清除系统时间变化回调
     * @return   {void}
     * @note    : 控制模块销毁外设前必须调用，防止NTP线程访问已释放对象。
     */
    void clear_time_change_callback();

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
	 * @return IpcRet_E
	 */
	IpcRet_E sync_time(std::time_t nTime);

	/**
	 * @brief 日期时间字符串转换为时间戳
	 * @param pDateTime 日期时间
	 * @return int
	 */
	int get_time_t(const char *pDateTime);

    /**
     * @brief   : 接收NTP线程完成校时后的时间变化通知
     * @param    {time_t} nOldTime：校时前UTC时间戳
     * @param    {time_t} nNewTime：校时后UTC时间戳
     * @return   {void}
     */
    void on_ntp_time_changed(std::time_t nOldTime, std::time_t nNewTime);

    /**
     * @brief   : 判断时间变化是否需要重建录制索引和文件
     * @param    {SystemTimeChangeInfo_S} stChangeInfo：时间变化上下文
     * @return   {bool} true：需要重建，false：无需重建
     * @note    : 沿用录制模块既有10秒跳变阈值，时区变化始终重建。
     */
    bool need_rebuild_recording(const SystemTimeChangeInfo_S &stChangeInfo) const;

    /**
     * @brief   : 重建受时间变化影响的录制状态
     * @param    {SystemTimeChangeInfo_S} stChangeInfo：时间变化上下文
     * @return   {void}
     * @note    : 必须等待录制进程完成m3u8收尾，避免清理过程与写入并发。
     */
    void rebuild_recording_after_time_change(const SystemTimeChangeInfo_S &stChangeInfo);

    /* lock: 串行时间变化通知与注销，保护外部非拥有回调生命周期。 */
    std::mutex m_mtxTimeChangeCallback;
    /* 控制组合根注册的时间变化消费者。 */
    TimeChangeCallback m_fnTimeChangeCallback;
};
