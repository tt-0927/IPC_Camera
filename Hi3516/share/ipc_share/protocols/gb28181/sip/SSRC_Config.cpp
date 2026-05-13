#include "SSRC_Config.h"
#include "SipUtils.h"
#include <iomanip>
using namespace SIP;
SSRCInfo::SSRCInfo(int port, const std::string &ssrc, const std::string &stream_id)
	: _port(port), _ssrc(ssrc), _stream_id(stream_id)
{
}

int SSRCInfo::GetPort()
{
	return _port;
}

void SSRCInfo::SetPort(int port)
{
	_port = port;
}

std::string SSRCInfo::GetSSRC() const
{
	return _ssrc;
}

void SSRCInfo::SetSSRC(const std::string &ssrc)
{
	_ssrc = ssrc;
}

std::string SSRCInfo::GetStreamID() const
{
	return _stream_id;
}

void SSRCInfo::SetStreamID(const std::string &id)
{
	_stream_id = id;
}

SSRCConfig *SSRCConfig::m_pInstance = nullptr;
std::mutex SSRCConfig::m_mtx;

std::string SSRCConfig::GenerateSSRC(SSRCConfig::Mode mode)
{
	std::string strSSRC;
	std::string strMode = std::to_string(static_cast<int>(mode));
	if (++m_nSSRCindex > 9999)
	{
		m_nSSRCindex = 0;
	}
	std::stringstream ss;
	ss << std::setw(4) << std::setfill('0') << m_nSSRCindex;
	/* NOTE 参考GBT+28181-2022 附录G p130 */
	/* 第一位为实时或回放，第二到第六位取监控域ID的4~8位，第七到第十位为不重复的四位十进制整数 */
	strSSRC = strMode + m_strServerPrefix + ss.str();
	return strSSRC;
}