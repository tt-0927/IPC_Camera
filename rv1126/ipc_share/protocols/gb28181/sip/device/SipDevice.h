/*
 * @Author       : EasonLu
 * @Date         : 2025-02-14 15:49:04
 * @LastEditors  : EasonLu
 * @LastEditTime : 2025-04-24 19:54:08
 * @FilePath     : SipDevice.h
 * @Description  : 设备信息及通道信息
 */
#pragma once
#include "GbDefine.h"
#include "MediaClient.hpp"
#include "MediaRtp.h"
#include "MediaSdp.h"
#include "MediaServer.hpp"
#include "SipType.h"
#include "SipChannel.h"
#include <map>
#include <memory>

namespace SIP
{

	/// @brief 设备信息
	class Device : public std::enable_shared_from_this<Device>
	{
	public:
		typedef std::shared_ptr<Device> Ptr;

		Device() = default;
		Device(const std::string &device_id, const std::string &ip, const std::string &port);

		void InsertChannel(const std::string &parent_id, const std::string &channel_id, Channel::Ptr channel);
		void DeleteChannel(const std::string &channel_id);

		Channel::Ptr GetChannel(const std::string &channel_id);
		Channel::Ptr GetChannelByIP(const std::string &strIP);
		Channel::Ptr GetChannelByCallID(const std::string &strCallID);
		Channel::Ptr GetChannelByIndex(const int &nIndex);
		Channel::Ptr GetChannelByType(SipChannelType_E enSipChannelType);
		std::vector<Channel::Ptr> GetAllChannels();
		void ClearChannel();

		std::string GetDeviceID() const;
		void SetDeviceID(const std::string &id);

		std::string GetName() const;
		void SetName(const std::string &name);

		std::string GetNickName() const;
		void SetNickName(const std::string &name);

		std::string GetIP() const;
		void SetIP(const std::string &ip);

		std::string GetPort() const;
		void SetPort(const std::string &port);

		std::string GetTransport() const;
		void SetTransport(const std::string &transport);

		void SetManufacturer(const std::string &manufacturer);
		std::string GetManufacturer() const;

		void SetModel(const std::string &model);
		std::string GetModel() const;

		int GetStatus() const;
		void SetStatus(int status);

		time_t GetRegistTime();
		void UpdateRegistTime(time_t t = time(nullptr));

		time_t GetLastTime();
		void UpdateLastTime(time_t t = time(nullptr));

		int GetChannelCount();
		void SetChannelCount(int count);

		std::string GetParentID() const;
		void SetParentID(const std::string &parent_id);

		std::string GetExternIP() const;
		void SetExternIP(const std::string &strExternIP);

		bool IsRegistered();
		void SetRegistered(bool flag);

		uint32_t GetExpires();
		void SetExpires(uint32_t nExpires);

		std::string GetUri() const;
		
		/**
		 * @brief	云台控制
		 * @param  [std::string] strChnID - 通道ID
		 * @param  [SipPtzType_E] enCmd - 云台控制指令
		 * @param  [int] nSpeed - 速度(默认128,区间为0-255)
		 * @return [*]
		 * @author EasonLu
		 * @note
		 */
		int PtzCtrl(const std::string &strChnID, SipPtzType_E enCmd, int nSpeed = 128);

		/**
		 * @brief  预置点控制
		 * @param  [string] &strChnID - 通道ID
		 * @param  [SipPresetType_E] enCmd - 预置点控制指令
		 * @param  [unsigned int] nPresetID - 预置点ID
		 * @return [*]
		 * @author EasonLu
		 * @note
		 */
		int PresetCtrl(const std::string &strChnID, SipPresetType_E enCmd, unsigned int nPresetID);

		/**
		 * @brief  报警订阅
		 * @param  [AlarmSubscribe_S] &stInfo - 报警订阅信息(默认不限时间订阅所有报警信息)
		 * @return [*]
		 * @author EasonLu
		 * @note   订阅后需要布防设备才能使设备上抛报警数据
		 */
		int AlarmSubscribe(const GB28181::AlarmSubscribe_S &stInfo = GB28181::_AlarmSubscribe_S_());

		/**
		 * @brief   布防设备
		 * @param  [bool] bSetGuard - true为布防，false为撤防
		 * @return [*]
		 * @author EasonLu
		 * @note   布防设备后，需要订阅报警信息才能使设备上抛报警数据
		 */
		int Guard(bool bSetGuard);

		/**
		 * @brief  上抛当前设备的报警信息
		 * @param  [AlarmCbData_S &] stCbInfo
		 * @return [*]
		 * @author EasonLu
		 * @note
		 */
		void AlarmCallback(const GB28181::AlarmCbData_S &stCbInfo);

		/**
		 * @brief  点播视频通道
		 * @return [*]
		 * @author EasonLu
		 * @note
		 */
		int PlayVideo(bool bStart = true);

		/*** 
		 * @description : 播放音频
		 * @author      : cyc
		 * @param        {bool} bStart
		 * @return       {*}
		 */		
		int PlayAudio(bool bStart = true);

		eXosip_t *exosip_context = nullptr;

	private:
		/* 对应设置GB设备ID */
		std::string _device_id;
		std::string _name;
		std::string _nickname;
		std::string _ip;
		std::string _port;
		std::string _transport = "UDP";
		std::string _manufacturer;
		std::string _model;
		/* 0：离线 1：在线 */
		int _status = 0;
		uint32_t _expires;
		/* 当前设备对外连接时的IP */
		std::string m_strExternIP;

		time_t _regist_time = 0;
		time_t _last_time = 0;

		int _channel_count = 0;
		std::string _parent_id;

		std::map<std::string, Channel::Ptr> _channels;

		bool _registered = false;
		std::mutex _mutex;

		SipDevInfoObserver m_fnStatusCb = nullptr;
		SipMediaCallBack m_fnMediaCb = nullptr;
		GB28181::AlarmObserver m_fnAlarmObserver = nullptr;
	};

}