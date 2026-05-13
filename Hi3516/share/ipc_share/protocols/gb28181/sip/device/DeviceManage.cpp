#include "DeviceManage.h"

using namespace SIP;

#ifndef DEVICE_CHECK_INTERVAL
/* 设备状态检查间隔（单位：秒） */
#define DEVICE_CHECK_INTERVAL 60
#endif

#ifndef DEVICE_OFFLINE_INTERVAL
/* 设备离线间隔（单位：秒） */
#define DEVICE_OFFLINE_INTERVAL 180
#endif

#define DEVICE_MANAGE_DEBUG 0

DeviceManage *DeviceManage::m_pInstance = nullptr;
std::mutex DeviceManage::m_mtx;
void DeviceManage::Init()
{
}

void DeviceManage::AddDevice(Device::Ptr device)
{
	std::scoped_lock<std::mutex> lk(m_mtxDevice);
	m_mapDevice[device->GetDeviceID()] = device;
}

Device::Ptr DeviceManage::GetDevice(const std::string &device_id)
{
	std::scoped_lock<std::mutex> lk(m_mtxDevice);
	auto iter = m_mapDevice.find(device_id);
	if (iter != m_mapDevice.end())
	{
		return iter->second;
	}
	return nullptr;
}

Device::Ptr DeviceManage::GetDevice(const std::string &ip, const std::string &port)
{
	std::scoped_lock<std::mutex> lk(m_mtxDevice);
	for (auto &&dev : m_mapDevice)
	{
		if (dev.second->GetIP() == ip && dev.second->GetPort() == port)
		{
			return dev.second;
		}
	}
	return nullptr;
}

void DeviceManage::RemoveDevice(const std::string &device_id)
{
	std::scoped_lock<std::mutex> lk(m_mtxDevice);
	m_mapDevice.erase(device_id);
}

std::vector<Device::Ptr> DeviceManage::GetDeviceList()
{
	std::scoped_lock<std::mutex> lk(m_mtxDevice);
	std::vector<Device::Ptr> devices;
	for (auto &&dev : m_mapDevice)
	{
		devices.push_back(dev.second);
	}
	return devices;
}

int DeviceManage::GetDeviceList(std::vector<SipDeviceInfo_S> &vecDevInfo)
{
	std::scoped_lock<std::mutex> lk(m_mtxDevice);
	vecDevInfo.clear();
	for (auto &&dev : m_mapDevice)
	{
		auto chnList = dev.second->GetAllChannels();
		for (auto &&chn : chnList)
		{
			SipDeviceInfo_S item;
			chn->ToSipDevice(item);
			vecDevInfo.push_back(item);
		}
	}
	return vecDevInfo.size();
}

int DeviceManage::GetDeviceCount()
{
	std::scoped_lock<std::mutex> lk(m_mtxDevice);
	return (int)m_mapDevice.size();
}

void DeviceManage::UpdateDeviceStatus(const std::string &device_id, int status)
{
	std::scoped_lock<std::mutex> lk(m_mtxDevice);
	auto iter = m_mapDevice.find(device_id);
	if (iter != m_mapDevice.end())
	{
		iter->second->SetStatus(status);
	}
}

void DeviceManage::UpdateDeviceLastTime(const std::string &device_id, time_t time)
{
	std::scoped_lock<std::mutex> lk(m_mtxDevice);
	auto iter = m_mapDevice.find(device_id);
	if (iter != m_mapDevice.end())
	{
		iter->second->UpdateLastTime(time);
	}
}

void DeviceManage::UpdateDeviceChannelCount(const std::string &device_id, int count)
{
	std::scoped_lock<std::mutex> lk(m_mtxDevice);
	auto iter = m_mapDevice.find(device_id);
	if (iter != m_mapDevice.end())
	{
		iter->second->SetChannelCount(count);
	}
}

void DeviceManage::Start()
{
	if (nullptr != m_thrCheck)
	{
		/* 已启动 */
		return;
	}
	m_bRunCheckThread = true;
	m_thrCheck = std::make_shared<std::thread>(std::bind(&DeviceManage::CheckStatusThread, this));
	m_thrCheck->detach();
}

void SIP::DeviceManage::Stop()
{
	m_bRunCheckThread = false;
	m_thrCheck = nullptr;
}

void SIP::DeviceManage::CheckStatusThread()
{
	pthread_setname_np(pthread_self(), "SIPDevStatus");

	MLOG_TRACE("开始设备状态检查线程");
	while (m_bRunCheckThread)
	{
		std::this_thread::sleep_for(std::chrono::seconds(DEVICE_CHECK_INTERVAL));
		std::unique_lock<std::mutex> lk(m_mtxDevice);
#if DEVICE_MANAGE_DEBUG
		MLOG_TRACE("===========> 开始检测设备状态");
#endif
		/* 获取当前时间 */
		auto now = std::time(nullptr);
		for (auto &&dev : m_mapDevice)
		{
#if DEVICE_MANAGE_DEBUG
			MLOG_TRACE("===========> 检测设备:%s", dev.second->GetDeviceID().c_str());
#endif
			/* 获取上次心跳时间 */
			auto lastTime = dev.second->GetLastTime();
			/* 判断是否已经注册 */
			if (dev.second->IsRegistered())
			{
				/* NOTE 操作时需要解锁，存在部分情况否则会导致死锁 */
				auto nGapTime = now - lastTime;
				/* 在线但是心跳超时 */
				if (dev.second->GetStatus() > 0 &&
					nGapTime > DEVICE_OFFLINE_INTERVAL)
				{
#if DEVICE_MANAGE_DEBUG
					MLOG_DEBUG("===========> 更新设备状态为离线:%s",
							   dev.second->GetDeviceID().c_str());
#endif
					lk.unlock();
					dev.second->SetStatus(0);
					lk.lock();
				}
				/* 超过注册有效时间 */
				if (nGapTime > dev.second->GetExpires())
				{
					/* NOTE 检查线程只需更新超过注册有效期的设备注册标记位，接收心跳信息会检测是否需要重新注册 */
					lk.unlock();
					dev.second->SetRegistered(false);
					lk.lock();
				}
			}
		}
	}
	MLOG_TRACE("设备状态检查线程退出");
}
