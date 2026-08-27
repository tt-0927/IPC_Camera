/*
 * @Author       : EasonLu
 * @Date         : 2025-02-17 15:35:14
 * @LastEditors  : EasonLu
 * @LastEditTime : 2025-03-03 14:30:16
 * @FilePath     : RequestPool.cpp
 * @Description  : 等待响应的请求记录池
 */
#include "RequestPool.h"
#include "dlog.h"

/* 请求响应超时时间（单位：秒）参考GB28181-2022 5.3 */
#define CHECK_INTERVAL (4 + 2)
#define CHECK_TIMEOUT (CHECK_INTERVAL * 3)

using namespace SIP;

RequestPool *RequestPool::m_pInstance = nullptr;
std::mutex RequestPool::m_mtx;

void RequestPool::Start()
{
	m_nCheckInterval = CHECK_INTERVAL * 1000;
	m_pCheckThread = new BlockThread(
		std::bind(&RequestPool::CheckRequestTimeout, this),
		std::chrono::milliseconds(m_nCheckInterval));
	m_pCheckThread->start();

}

void SIP::RequestPool::Stop()
{
	if (m_pCheckThread != nullptr)
	{
		m_pCheckThread->stop();
		delete m_pCheckThread;
		m_pCheckThread = nullptr;
	}
	m_mapRequest.clear();
}

void RequestPool::AddRequest(const std::string &req_id, BaseRequest::Ptr request)
{
	std::scoped_lock<std::mutex> lk(m_mtxRequest);
	auto iter = m_mapRequest.find(req_id);
	if (iter == m_mapRequest.end())
	{
		request->SetRequestID(req_id);
		m_mapRequest[req_id] = request;
		dlog_info("添加请求记录: [%s]", req_id.c_str());
	}
	else
	{
		dlog_info("已存在请求记录: [%s]", req_id.c_str());
	}
	dlog_info("当前请求记录个数: [%d]", m_mapRequest.size());
}

void RequestPool::RemoveRequest(const std::string &req_id)
{
	std::scoped_lock<std::mutex> lk(m_mtxRequest);
	m_mapRequest.erase(req_id);
	dlog_info("请求已响应，移除请求记录: [%s]", req_id.c_str());
}

MessageRequest::Ptr RequestPool::GetMessageRequestBySN(const std::string &sn, REQUEST_MESSAGE_TYPE type)
{
	std::scoped_lock<std::mutex> lk(m_mtxRequest);
	for (auto &&req : m_mapRequest)
	{
		if (req.second->GetRequestType() != type)
		{
			continue;
		}

		MessageRequest::Ptr request = std::dynamic_pointer_cast<MessageRequest>(req.second);
		if (request->GetRequestSN() == sn)
		{
			return request;
		}
	}

	return nullptr;
}

int RequestPool::HandleMessageRequest(const std::string &req_id, int status_code)
{
	return HandleResponse(req_id, status_code);
}

void RequestPool::CheckRequestTimeout()
{
	dlog_info("检查请求记录是否响应超时");
	std::scoped_lock<std::mutex> lk(m_mtxRequest);
	auto now = std::time(nullptr);
	for (auto iter = m_mapRequest.begin(); iter != m_mapRequest.end();)
	{
		if (now - iter->second->GetRequestTime() > CHECK_TIMEOUT)
		{
			iter->second->HandleResponse(-1);
			iter->second->Finish();
			dlog_info("请求响应超时: [%s]", iter->first.c_str());
			iter = m_mapRequest.erase(iter);
		}
		else
		{
			++iter;
		}
	}

	return;
}

int RequestPool::HandleResponse(const std::string &req_id, int status_code)
{
	BaseRequest::Ptr request = nullptr;
	std::scoped_lock<std::mutex> lk(m_mtxRequest);
	auto iter = m_mapRequest.find(req_id);
	if (iter != m_mapRequest.end())
	{
		auto request = iter->second;
		// if (status_code != SIP_OK)
		/* NOTE 是否需要异步发送 */
		request->HandleResponse(status_code);
		request->Finish();
	}

	return 0;
}
