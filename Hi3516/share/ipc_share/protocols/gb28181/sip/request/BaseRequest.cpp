/*
 * @Author       : EasonLu
 * @Date         : 2025-02-14 16:47:02
 * @LastEditors  : EasonLu
 * @LastEditTime : 2025-02-14 16:57:50
 * @FilePath     : BaseRequest.cpp
 * @Description  : 请求基类
 */
#include "BaseRequest.h"
using namespace SIP;
BaseRequest::BaseRequest(eXosip_t *ctx, Device::Ptr device, REQUEST_MESSAGE_TYPE type)
    : m_pContext(ctx), _request_type(type)
{
    _device = device;
    _request_time = std::time(nullptr);
}

BaseRequest::~BaseRequest()
{
}

int BaseRequest::HandleResponse(int status_code)
{
    return 0;
}

void BaseRequest::SetWait(bool bwait)
{
    _b_wait = bwait;
}

void BaseRequest::WaitResult()
{
    if (!_b_wait)
        return;
    std::unique_lock<std::mutex> lk(_mutex);
    _cv.wait(lk);
}

bool BaseRequest::IsFinished()
{
    return _b_finished;
}

void BaseRequest::SetRequestID(const std::string &id)
{
    _request_id = id;
}

REQUEST_MESSAGE_TYPE BaseRequest::GetRequestType()
{
    return _request_type;
}

void BaseRequest::OnRequestFinished()
{
    _b_finished = true;
    if (_b_wait)
    {
        Finish();
    }
}

time_t BaseRequest::GetRequestTime()
{
    return _request_time;
}

void BaseRequest::Finish()
{
    if (!_b_wait)
    {
        return;
    }
    std::unique_lock<std::mutex> lk(_mutex);
    lk.unlock();
    _cv.notify_one();
    _b_wait = false;
}

Device::Ptr BaseRequest::GetDevice()
{
    return _device;
}

const char *BaseRequest::_get_request_id_from_request(osip_message_t *msg)
{
    osip_generic_param_t *tag = nullptr;
    // osip_to_get_tag(msg->from, &tag);
    osip_uri_param_get_byname(&msg->from->gen_params, (char *)"tag", &tag);

    if (tag == nullptr || tag->gvalue == nullptr)
    {
        return nullptr;
    }
    return tag->gvalue;
}
