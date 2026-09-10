#pragma once
#include "SipDevice.h"
#include "SipType.h"
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <mutex>
#include <thread>
#include <unordered_map>
namespace SIP
{
	class DeviceManage
	{
	public:
		/**
		 * @brief	设备管理单例
		 * @return [DeviceManage*]
		 * @author EasonLu
		 * @note   管理服务器端的设备信息，客户端的设备信息自行管理
		 */
		static DeviceManage *instance()
		{
			std::lock_guard<std::mutex> lock(m_mtx);
			if (m_pInstance == nullptr)
			{
				m_pInstance = new DeviceManage();
			}
			return m_pInstance;
		}

		void Init();
		void AddDevice(Device::Ptr device);
		Device::Ptr GetDevice(const std::string &device_id);
		Device::Ptr GetDevice(const std::string &ip, const std::string &port);
		void RemoveDevice(const std::string &device_id);

		std::vector<Device::Ptr> GetDeviceList();
		int GetDeviceList(std::vector<SipDeviceInfo_S> &vecDevInfo);
		int GetDeviceCount();
		void UpdateDeviceStatus(const std::string &device_id, int status);
		void UpdateDeviceLastTime(const std::string &device_id, time_t time = std::time(nullptr));
		void UpdateDeviceChannelCount(const std::string &device_id, int count);

		void Start();

		void Stop();

	private:
		DeviceManage() = default;
		~DeviceManage() = default;

		void CheckStatusThread();

	private:
		std::unordered_map<std::string, Device::Ptr> m_mapDevice;
		std::mutex m_mtxDevice;
		static DeviceManage *m_pInstance;
		static std::mutex m_mtx;
		std::shared_ptr<std::thread> m_thrCheck;
		std::atomic_bool m_bRunCheckThread;
	};
}