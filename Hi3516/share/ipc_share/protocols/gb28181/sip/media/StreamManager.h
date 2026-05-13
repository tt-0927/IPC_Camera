/*
 * @Author       : EasonLu
 * @Date         : 2025-02-27 15:43:45
 * @LastEditors  : EasonLu
 * @LastEditTime : 2025-02-27 15:43:53
 * @FilePath     : StreamManager.h
 * @Description  : 媒体流管理
 */
#pragma once
#include "MediaStream.hpp"
#include <map>
#include <mutex>
#include <vector>
namespace SIP
{

    class StreamManager
    {
    public:
		/**
		 * @brief  模块管理单例
		 * @return [*]
		 * @author EasonLu
		 * @note
		 */
		static StreamManager *instance()
		{
			std::lock_guard<std::mutex> lock(m_mtx);
			if (m_pInstance == nullptr)
			{
				m_pInstance = new StreamManager();
			}
			return m_pInstance;
		}

        void AddStream(MediaStream::Ptr stream);
        void RemoveStream(const std::string &id);

        MediaStream::Ptr GetStream(const std::string &id);
        MediaStream::Ptr GetStreamByCallID(int id);
        std::vector<MediaStream::Ptr> GetAllStream();
        std::vector<MediaStream::Ptr> GetStreamByType(STREAM_TYPE type);
        MediaStream::Ptr MakeStream(const std::string &stream_id, const std::string &app, STREAM_TYPE type);
        void ClearStreams();

    private:
        StreamManager() = default;
        ~StreamManager() = default;
    private:
        static StreamManager *m_pInstance;
        static std::mutex m_mtx;
        std::mutex _mutex;
        /* Key值为DeviceID_ChannelID */
        std::map<std::string, MediaStream::Ptr> _streams;
    };
}
