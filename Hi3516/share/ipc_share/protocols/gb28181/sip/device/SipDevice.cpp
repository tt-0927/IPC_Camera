#include "SipDevice.h"
#include "CallSession.h"
#include "MessageRequest.h"
#include "MethodRequest.h"
#include "ModuleLog.h"
#include "SipModule.h"
#include "SipUtils.h"
#include "StreamManager.h"
using namespace SIP;

Device::Device(const std::string &device_id, const std::string &ip, const std::string &port)
{
	_device_id = device_id;
	_ip = ip;
	_port = port;
}

void Device::InsertChannel(const std::string &parent_id, const std::string &channel_id, Channel::Ptr channel)
{
	std::lock_guard<std::mutex> lk(_mutex);
	if(parent_id.empty() || channel_id.empty() || channel == nullptr)
	{
		MLOG_INFO("插入通道失败 通道信息为空");
		return;
	}
	MLOG_INFO("=========添加或更新通道 设备ID[%s] 父ID[%s] 通道ID[%s]=====",
	       _device_id.c_str(),
	       parent_id.c_str(),
			channel_id.c_str());
	if (_channels.find(channel_id) == _channels.end())
	{
		_channels[channel_id] = channel;
	}
	/* 同步信息给通道 */
	channel->SetParentID(parent_id);
	channel->SetStreamID(_device_id + "_" + channel_id);
	channel->SetCivilCode(_device_id.substr(0, 10));
	channel->SetExternIP(_ip);
	channel->nPort = std::stoi(_port);
	channel->SetSipStatus(SIP_ONLINE);
	channel->SetChannelTypeFromId(channel_id);
	_channels[channel_id] = channel;
	_channel_count++;
	
}

void Device::DeleteChannel(const std::string &channel_id)
{
	std::lock_guard<std::mutex> lk(_mutex);
	auto iter = _channels.find(channel_id);
	if (iter != _channels.end())
	{
		_channels.erase(iter);
		_channel_count--;
	}
}

Channel::Ptr Device::GetChannel(const std::string &channel_id)
{
	std::lock_guard<std::mutex> lk(_mutex);
	auto iter = _channels.find(channel_id);
	if (iter != _channels.end())
	{
		return iter->second;
	}
	return nullptr;
}

std::vector<Channel::Ptr> Device::GetAllChannels()
{
	std::lock_guard<std::mutex> lk(_mutex);
	std::vector<Channel::Ptr> channels;
	for (auto &&ch : _channels)
	{
		channels.push_back(ch.second);
	}
	return channels;
}

Channel::Ptr SIP::Device::GetChannelByIP(const std::string &strIP)
{
	std::lock_guard<std::mutex> lk(_mutex);
	for (auto &&chn : _channels)
	{
		if (chn.second->GetExternIP() == strIP)
		{
			return chn.second;
		}
	}
	return nullptr;
}

Channel::Ptr SIP::Device::GetChannelByCallID(const std::string &strCallID)
{
	if (strCallID.empty())
	{
		return nullptr;
	}
	std::lock_guard<std::mutex> lk(_mutex);
	for (auto &&chn : _channels)
	{
		if (chn.second->GetCallID() == strCallID)
		{
			return chn.second;
		}
	}
	return nullptr;
}

Channel::Ptr SIP::Device::GetChannelByIndex(const int &nIndex)
{
	if (nIndex < 0)
	{
		return nullptr;
	}
	std::lock_guard<std::mutex> lk(_mutex);
	for (auto &&chn : _channels)
	{
		if (chn.second->nIndex == nIndex)
		{
			return chn.second;
		}
	}
	return nullptr;
}

Channel::Ptr SIP::Device::GetChannelByType(SipChannelType_E enSipChannelType)
{
	if (enSipChannelType == SipChannelType_E::CHANNELTYPE_UNKNOWN)
	{
		return nullptr;
	}
	std::lock_guard<std::mutex> lk(_mutex);
	for (auto &&chn : _channels)
	{
		if (chn.second->enChannelType == enSipChannelType)
		{
			return chn.second;
		}
	}
	return nullptr;
}


void SIP::Device::ClearChannel()
{
	std::lock_guard<std::mutex> lk(_mutex);
	_channels.clear();
}

std::string Device::GetDeviceID() const
{
	return _device_id;
}

void Device::SetDeviceID(const std::string &id)
{
	_device_id = id;
}

std::string Device::GetName() const
{
	return _name;
}

void Device::SetName(const std::string &name)
{
	_name = ToMbcsString(name);
}

std::string Device::GetNickName() const
{
	return _nickname.empty() ? _name : _nickname;
}

void Device::SetNickName(const std::string &name)
{
	_nickname = ToMbcsString(name);
}

std::string Device::GetIP() const
{
	return _ip;
}

void Device::SetIP(const std::string &ip)
{
	_ip = ip;
}

std::string Device::GetPort() const
{
	return _port;
}

void Device::SetPort(const std::string &port)
{
	_port = port;
}

std::string Device::GetTransport() const
{
	return _transport;
}

void Device::SetTransport(const std::string &transport)
{
	_transport = transport;
}

void Device::SetManufacturer(const std::string &manufacturer)
{
	_manufacturer = ToMbcsString(manufacturer);
}

std::string Device::GetManufacturer() const
{
	return _manufacturer;
}

void Device::SetModel(const std::string &model)
{
	_model = model;
}

std::string Device::GetModel() const
{
	return _model;
}

int Device::GetStatus() const
{
	return _status;
}

void Device::SetStatus(int status)
{
	if (_status == status)
	{
		/* 状态相同不必更新 */
		return;
	}
	_status = status;
	if (status == 0)
	{
		_registered = false;
		/* 父设备已注销，同步离线状态到所有子通道 */
		for (auto &&ch : _channels)
		{
			ch.second->SetSipStatus(SIP_OFFLINE);
			ch.second->UploadInfo();
			/* 关闭或移除点播通道 */
			PlayVideo(false);
		}
	}
}

time_t Device::GetRegistTime()
{
	return _regist_time;
}

void Device::UpdateRegistTime(time_t t)
{
	_regist_time = t;
}

time_t Device::GetLastTime()
{
	return _last_time;
}

void Device::UpdateLastTime(time_t t)
{
	_last_time = t;
}

int Device::GetChannelCount()
{
	return _channel_count;
}

void Device::SetChannelCount(int count)
{
	_channel_count = count;
}

std::string Device::GetParentID() const
{
	return _parent_id;
}

void Device::SetParentID(const std::string &parent_id)
{
	_parent_id = parent_id;
}

std::string Device::GetExternIP() const
{
	return m_strExternIP;
}

void Device::SetExternIP(const std::string &strExternIP)
{
	m_strExternIP = strExternIP;
}

bool Device::IsRegistered()
{
	return _registered;
}

void Device::SetRegistered(bool flag)
{
	_registered = flag;
	/* TODO 注册状态变更是否要上抛？ */
}

uint32_t Device::GetExpires()
{
	return _expires;
}

void Device::SetExpires(uint32_t nExpires)
{
	_expires = nExpires;
}

std::string Device::GetUri() const
{
	return std::string("sip:") + GetDeviceID() + "@" + GetIP() + ":" + GetPort();
}

int Device::PtzCtrl(const std::string &strChnID, SipPtzType_E enCmd, int nSpeed)
{
	if (!IsRegistered() || GetStatus() == 0)
	{
		MLOG_WARN("设备[%s]未注册或离线，无法控制", _device_id.c_str());
		return -1;
	}

	if (nullptr == GetChannel(strChnID))
	{
		MLOG_WARN("通道[%d]不存在，无法控制", strChnID);
		return -1;
	}

	int nRet = 0;
	std::shared_ptr<MessageRequest> request = nullptr;
	std::shared_ptr<Device> device = shared_from_this();
	switch (enCmd)
	{
	case SipPtzType_E::UP:
		request = std::make_shared<PtzCtlRequest>(exosip_context, device, strChnID, PtzCommand_E::NONE, PtzCommand_E::UP, PtzCommand_E::NONE, nSpeed, 0);
		break;
	case SipPtzType_E::DOWN:
		request = std::make_shared<PtzCtlRequest>(exosip_context, device, strChnID, PtzCommand_E::NONE, PtzCommand_E::DOWN, PtzCommand_E::NONE, nSpeed, 0);
		break;
	case SipPtzType_E::LEFT:
		request = std::make_shared<PtzCtlRequest>(exosip_context, device, strChnID, PtzCommand_E::LEFT, PtzCommand_E::NONE, PtzCommand_E::NONE, nSpeed, 0);
		break;
	case SipPtzType_E::RIGHT:
		request = std::make_shared<PtzCtlRequest>(exosip_context, device, strChnID, PtzCommand_E::RIGHT, PtzCommand_E::NONE, PtzCommand_E::NONE, nSpeed, 0);
		break;
	case SipPtzType_E::TOP_LEFT:
		request = std::make_shared<PtzCtlRequest>(exosip_context, device, strChnID, PtzCommand_E::LEFT, PtzCommand_E::UP, PtzCommand_E::NONE, nSpeed, 0);
		break;
	case SipPtzType_E::TOP_RIGHT:
		request = std::make_shared<PtzCtlRequest>(exosip_context, device, strChnID, PtzCommand_E::RIGHT, PtzCommand_E::UP, PtzCommand_E::NONE, nSpeed, 0);
		break;
	case SipPtzType_E::LOWER_LEFT:
		request = std::make_shared<PtzCtlRequest>(exosip_context, device, strChnID, PtzCommand_E::LEFT, PtzCommand_E::DOWN, PtzCommand_E::NONE, nSpeed, 0);
		break;
	case SipPtzType_E::LOWER_RIGHT:
		request = std::make_shared<PtzCtlRequest>(exosip_context, device, strChnID, PtzCommand_E::RIGHT, PtzCommand_E::DOWN, PtzCommand_E::NONE, nSpeed, 0);
		break;
	case SipPtzType_E::RESET:
		request = std::make_shared<PtzCtlRequest>(exosip_context, device, strChnID, PtzCommand_E::NONE, PtzCommand_E::NONE, PtzCommand_E::NONE, 0, 0);
		break;
	case SipPtzType_E::STOP:
		request = std::make_shared<PtzCtlRequest>(exosip_context, device, strChnID, PtzCommand_E::NONE, PtzCommand_E::NONE, PtzCommand_E::NONE, 0, 0);
		break;
	case SipPtzType_E::ZOOM_UP:
		request = std::make_shared<PtzCtlRequest>(exosip_context, device, strChnID, PtzCommand_E::NONE, PtzCommand_E::NONE, PtzCommand_E::ZOOMIN, 0, nSpeed);
		break;
	case SipPtzType_E::ZOOM_DOWN:
		request = std::make_shared<PtzCtlRequest>(exosip_context, device, strChnID, PtzCommand_E::NONE, PtzCommand_E::NONE, PtzCommand_E::ZOOMOUT, 0, nSpeed);
		break;
	case SipPtzType_E::FOCUS_UP:
		request = std::make_shared<LensCtlRequest>(exosip_context, device, strChnID, PtzCommand_E::NONE, PtzCommand_E::FOCUS_NEAR, 0, nSpeed);
		break;
	case SipPtzType_E::FOCUS_DOWN:
		request = std::make_shared<LensCtlRequest>(exosip_context, device, strChnID, PtzCommand_E::NONE, PtzCommand_E::FOCUS_FAR, 0, nSpeed);
		break;
	case SipPtzType_E::APERTUR_UP:
		request = std::make_shared<LensCtlRequest>(exosip_context, device, strChnID, PtzCommand_E::IFIS_AMPLIFICATION, PtzCommand_E::NONE, nSpeed, 0);
		break;
	case SipPtzType_E::APERTUR_DOWN:
		request = std::make_shared<LensCtlRequest>(exosip_context, device, strChnID, PtzCommand_E::IFIS_SHRINK, PtzCommand_E::NONE, nSpeed, 0);
		break;
	default:
		MLOG_ERROR("Unkown ptz type:%d", enCmd);
		nRet = -1;
		break;
	}
	request->SendMessage();
	return nRet;
}

int SIP::Device::PresetCtrl(const std::string &strChnID, SipPresetType_E enCmd, unsigned int nPresetID)
{
	if (!IsRegistered() || GetStatus() == 0)
	{
		MLOG_WARN("设备[%s]未注册或离线，无法控制预置位", _device_id.c_str());
		return -1;
	}

	/* TODO 判断预置位ID，还有支持的最大值 */
	if (nPresetID < 1)
	{
		MLOG_WARN("设备[%s]无法控制预置位[%d]", _device_id.c_str(), nPresetID);
		return -1;
	}

	auto request = std::make_shared<PresetCtlRequest>(exosip_context, shared_from_this(), strChnID, static_cast<int>(enCmd), 0, static_cast<int>(nPresetID), 0);
	request->SendMessage();
	return 0;
}

int SIP::Device::AlarmSubscribe(const GB28181::AlarmSubscribe_S &stInfo)
{
	if (!IsRegistered() || GetStatus() == 0)
	{
		MLOG_WARN("设备[%s]未注册或离线，无法订阅报警信息", _device_id.c_str());
		return -1;
	}

	/* 更新回调函数 */
	m_fnAlarmObserver = stInfo.fnObserver;
	/* 发送订阅报警信息请求 */
	auto request = std::make_shared<AlarmRequest>(exosip_context, shared_from_this(), stInfo);
	request->SendMessage();

	return 0;
}

int SIP::Device::Guard(bool bSetGuard)
{
	if (!IsRegistered() || GetStatus() == 0)
	{
		MLOG_WARN("设备[%s]未注册或离线，无法布防设备", _device_id.c_str());
		return -1;
	}

	auto request = std::make_shared<GuardRequest>(exosip_context, shared_from_this(), bSetGuard);
	request->SendMessage();

	return 0;
}

void SIP::Device::AlarmCallback(const GB28181::AlarmCbData_S &stCbInfo)
{
	GB28181::AlarmCbData_S stInfo = stCbInfo;
	if (m_fnAlarmObserver)
	{
		stInfo.strIP = _ip;
		m_fnAlarmObserver(stInfo);
	}
}

int SIP::Device::PlayVideo(bool bStart)
{
	auto pChn = GetChannelByType(SipChannelType_E::NETWORK_CAMERA_IPC_ENCODE);
	if (nullptr == pChn)
	{
		/* 无指定通道ID */
		return -1;
	}
	auto strStreamID = pChn->GetStreamID();
	/* 校验当前通道是否已开启点播 */
	auto stream = StreamManager::instance()->GetStream(strStreamID);
	if (nullptr != stream)
	{
		auto session = std::dynamic_pointer_cast<CallSession>(stream);
		if (session && session->IsConnected())
		{
			if (!bStart)
			{
				MLOG_INFO("关闭设备[%s]通道点播", _device_id.c_str());
				/* 停止点播 */
				session->SendBye();
				/* 关闭服务器 */
				pChn->DestroyPlayNet();
				/* 移除记录的ID */
				StreamManager::instance()->RemoveStream(strStreamID);
			}
			else
			{
				MLOG_INFO("设备[%s]通道已经开始点播", _device_id.c_str());
			}
			return 0;
		}
	}

	/* 开启通道的数据接收 */
	pChn->CreateServer();
	pChn->CreatePlayRtp(true, pChn->GetDefaultSSRC());
	InviteRequest::InviteParam stInvParam;
	/* 请求端的Uri */
	stInvParam.strFromUri = SipModule::instance()->GetSipServerInfo(exosip_context).GetUri();
	/* 被请求端的Uri */
	stInvParam.strToUri = "sip:" + pChn->GetChannelID() + "@" + GetIP() + ":" + GetPort();
	/* SubjectID */
	stInvParam.strSubject = pChn->GetChannelID() + ":" + pChn->GetDefaultSSRC() + "," + pChn->GetChannelID() + ":0";
	stInvParam.strChnID = pChn->GetChannelID();
	stInvParam.strLocalIP = m_strExternIP;
	stInvParam.nLocalPort = pChn->GetPlayPort();
	stInvParam.strSSRC = pChn->GetDefaultSSRC();
	stInvParam.strStreamID = pChn->GetStreamID();
	/* 点播的请求 */
	stInvParam.enType = InviteRequest::InviteType::Play;
	/* DEBUG */
	// stInvParam.bUsingTcp = false;

	/* 发送INVITE请求 */
	auto request = std::make_shared<InviteRequest>(exosip_context, stInvParam);
	request->SendCall();
	return 0;
}

int SIP::Device::PlayAudio(bool bStart)
{
	auto pChn = GetChannelByType(SipChannelType_E::NETWORK_CAMERA_IPC_ENCODE);
	if (nullptr == pChn)
	{
		MLOG_ERROR("pChn is NULL");
		/* 无指定通道ID */
		return -1;
	}
	auto strStreamID = pChn->GetStreamID();
	/* 校验当前通道是否已开启点播 */
	auto stream = StreamManager::instance()->GetStream(strStreamID);
	if (nullptr != stream)
	{
		auto session = std::dynamic_pointer_cast<CallSession>(stream);
		if (session && session->IsConnected())
		{
			if (!bStart)
			{
				MLOG_INFO("关闭设备[%s]通道点播", _device_id.c_str());
				/* 停止点播 */
				session->SendBye();
				/* 关闭服务器 */
				pChn->DestroyPlayNet();
				/* 移除记录的ID */
				StreamManager::instance()->RemoveStream(strStreamID);
			}
			else
			{
				MLOG_INFO("设备[%s]通道已经开始点播", _device_id.c_str());
			}
			return 0;
		}
	}

	/* 开启通道的数据接收 */
	pChn->CreateServer();
	pChn->CreatePlayRtp(true, pChn->GetDefaultSSRC());
	InviteAudioRequest::InviteParam stInvParam;
	/* 请求端的Uri */
	stInvParam.strFromUri = SipModule::instance()->GetSipServerInfo(exosip_context).GetUri();
	/* 被请求端的Uri */
	stInvParam.strToUri = "sip:" + pChn->GetChannelID() + "@" + GetIP() + ":" + GetPort();
	/* SubjectID */
	stInvParam.strSubject = pChn->GetChannelID() + ":" + pChn->GetDefaultSSRC() + "," + pChn->GetChannelID() + ":0";
	stInvParam.strChnID = pChn->GetChannelID();
	stInvParam.strLocalIP = m_strExternIP;
	stInvParam.nLocalPort = pChn->GetPlayPort();
	stInvParam.strSSRC = pChn->GetDefaultSSRC();
	stInvParam.strStreamID = pChn->GetStreamID();
	/* 点播的请求 */
	stInvParam.enType = InviteAudioRequest::InviteType::Play;
	/* DEBUG */
	// stInvParam.bUsingTcp = false;

	/* 发送INVITE请求 */
	auto request = std::make_shared<InviteAudioRequest>(exosip_context, stInvParam);
	request->SendCall();
	return 0;
}

