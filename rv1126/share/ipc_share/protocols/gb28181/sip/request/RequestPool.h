/*
 * @Author       : EasonLu
 * @Date         : 2025-02-17 15:35:14
 * @LastEditors  : EasonLu
 * @LastEditTime : 2025-02-26 10:49:49
 * @FilePath     : RequestPool.h
 * @Description  : 等待响应的请求记录池
 */
#pragma once
#include "BaseRequest.h"
#include "BlockThread.hpp"
#include "MethodRequest.h"
#include "SipType.h"
#include <map>
#include <atomic>
#include <thread>
namespace SIP
{
	class RequestPool
	{
	public:
		/**
		 * @brief  模块管理单例
		 * @return [*]
		 * @author EasonLu
		 * @note
		 */
		static RequestPool *instance()
		{
			std::lock_guard<std::mutex> lock(m_mtx);
			if (m_pInstance == nullptr)
			{
				m_pInstance = new RequestPool();
			}
			return m_pInstance;
		}

		void Start();
		void Stop();
		void AddRequest(const std::string &req_id, BaseRequest::Ptr request);
		void RemoveRequest(const std::string &req_id);
		MessageRequest::Ptr GetMessageRequestBySN(const std::string &sn, REQUEST_MESSAGE_TYPE type);
		int HandleMessageRequest(const std::string &req_id, int status_code);

	private:
		RequestPool() = default;
		void CheckRequestTimeout();
		int HandleResponse(const std::string &req_id, int status_code);

	private:
		std::map<std::string, BaseRequest::Ptr> m_mapRequest;
		std::mutex m_mtxRequest;
		static RequestPool *m_pInstance;
		static std::mutex m_mtx;
		int m_nCheckInterval = -1;	/* 检查请求超时间隔 */
		BlockThread *m_pCheckThread = nullptr;
	};

}