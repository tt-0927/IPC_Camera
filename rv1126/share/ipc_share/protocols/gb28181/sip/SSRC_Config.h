#pragma once
#include "SipType.h"
#include <atomic>
namespace SIP
{
	class SSRCInfo
	{
	public:
		typedef std::shared_ptr<SSRCInfo> Ptr;

		SSRCInfo(int port, const std::string &ssrc, const std::string &stream_id);

		int GetPort();
		void SetPort(int port);
		std::string GetSSRC() const;
		void SetSSRC(const std::string &ssrc);
		std::string GetStreamID() const;
		void SetStreamID(const std::string &id);

	private:
		int _port = 0;
		std::string _ssrc;
		std::string _stream_id;
	};

	class SSRCConfig
	{
	public:
		enum class Mode : int
		{
			Realtime = 0,
			Playback,
			Download,
		};

		typedef std::shared_ptr<SSRCConfig> Ptr;
		/**
		 * @brief  模块管理单例
		 * @return [*]
		 * @author EasonLu
		 * @note
		 */
		static SSRCConfig *instance()
		{
			std::lock_guard<std::mutex> lock(m_mtx);
			if (m_pInstance == nullptr)
			{
				m_pInstance = new SSRCConfig();
			}
			return m_pInstance;
		}

		std::string GenerateSSRC(Mode m = Mode::Realtime);
		void SetServerPrefix(const std::string &prefix)
		{
			m_strServerPrefix = prefix;
		}
	private:
		SSRCConfig() = default;
		static std::mutex m_mtx;
		static SSRCConfig *m_pInstance;
		uint16_t m_nSSRCindex = 0;
		std::string m_strServerPrefix;
	};
}