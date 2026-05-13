#include "share_define.h"
#include "task.h"
#include "dlog.h"

/* 命令相关 */
using namespace Task;

std::string CTask::get_data(std::string jsonData)
{
    Json::Object *pJsonRoot = Json::init(jsonData);
    if (!pJsonRoot)
    {
        return std::string();
    }

    Json::Object *pJsonData = Json::get(pJsonRoot, "Data");
    std::string data = Json::to_string(pJsonData);
    Json::deinit(pJsonRoot);
    return data;
}

void CTask::deal_result(std::function<void(std::string)> fnDealFunc)
{
    m_fnDealResults.push(fnDealFunc);
}

void CTask::deal_result(std::string data)
{
    if (m_fnDealResults.empty())
    {
        return;
    }
    std::function<void(std::string)> fnDealResult = m_fnDealResults.front();
    m_fnDealResults.pop();
    if (fnDealResult)
    {
        fnDealResult(data);
        fnDealResult = nullptr;

        if (m_nActionCode != 3105 /* AC_GET_REPLAY_MEDIA_INFO */)
        {
            dlog_debug("result:%s", data.c_str());
        }
    }
}
int CTask::result(std::string res, int nRes)
{
    std::unique_lock<std::mutex> mtx(m_mtx);
    for (auto &stInfo : m_infos)
    {
        std::string data = fill_common(res, nRes, stInfo);
        if (stInfo.fnResultCallbacks)
        {
            stInfo.fnResultCallbacks(static_cast<const void *>(data.c_str()), data.size(), m_nActionCode, stInfo.pHandler);
            if (m_nActionCode != 3105 /* AC_GET_REPLAY_MEDIA_INFO */)
            {
                dlog_debug("result:%s", data.c_str());
            }
        }
    }
    m_taskData.clear();
    m_infos.clear();
    return 0;
}

void CTask::result(int nRet)
{

    std::unique_lock<std::mutex> mtx(m_mtx);
    for (auto &stInfo : m_infos)
    {
        std::string res;
        std::string data = fill_common(res, nRet, stInfo);
        if (stInfo.fnResultCallbacks)
        {
            stInfo.fnResultCallbacks(static_cast<const void *>(data.c_str()), data.size(), m_nActionCode, stInfo.pHandler);
            if (m_nActionCode != 3105 /* AC_GET_REPLAY_MEDIA_INFO */)
            {
                dlog_debug("result:%s", data.c_str());
            }
        }
    }
    m_taskData.clear();
    m_infos.clear();
}

void CTask::publish(std::string message)
{
    Info_S stInfo;
    stInfo.user = "admin";
    message = fill_common(message, 0, stInfo);
    publish(static_cast<const void *>(message.c_str()), message.length());
}

void CTask::publish(const void *pData, int nLen)
{
    for (auto &callback : m_subscribe)
    {
        callback(pData, nLen, m_nActionCode, nullptr);
    }
}

bool CTask::is_result(std::string jsonData)
{
    int nRet = 0;
    return Json::get(jsonData.c_str(), "Return", nRet);
}
void CTask::set_info(int nActionCode, Info_S stInfo)
{
    std::unique_lock<std::mutex> mtx(m_mtx);
    m_nActionCode = nActionCode;
    m_data = stInfo.data;
    m_taskData = get_data(stInfo.data);
    Json::get(stInfo.data.c_str(), "UserName", stInfo.user);
    Json::get(stInfo.data.c_str(), "UniqueId", stInfo.nUniqueId);
    m_infos.insert(stInfo);

    if (m_nActionCode != 3105 /* AC_GET_REPLAY_MEDIA_INFO */)
    {
        dlog_debug("m_taskData:%s\n", m_taskData.c_str());
    }
}

void CTask::set_actionCode(int nActionCode)
{
    m_nActionCode = nActionCode;
}

int CTask::get_actionCode()
{
    return m_nActionCode;
}

void CTask::set_user(std::string user)
{
    m_user = user;
}

void CTask::set_userIp(std::string strUserIp)
{
    m_strUserIp = strUserIp;
}

void CTask::fill_head(std::string &data, int nActionCode)
{
    Json::Object *pJsonRoot = Json::init();
    Json::add(pJsonRoot, "ActionCode", nActionCode);
    Json::add(pJsonRoot, "DeviceName", "");
    Json::add(pJsonRoot, "UserName", m_user);

    Json::Object *pJsonData = Json::init(data);
    if (pJsonData)
    {
        Json::add(pJsonRoot, "Data", pJsonData);
    }

    data = Json::to_string(pJsonRoot);
    Json::deinit(pJsonRoot);
    return;
}

void Task::CTask::fill_returnHead(std::string &data, int nActionCode)
{
    Json::Object *pJsonRoot = Json::init();
    Json::add(pJsonRoot, "ActionCode", nActionCode);
    Json::add(pJsonRoot, "DeviceName", "");
    Json::add(pJsonRoot, "UserName", m_user);
    Json::add(pJsonRoot, "Return", 0);

    Json::Object *pJsonData = Json::init(data);
    if (pJsonData)
    {
        Json::add(pJsonRoot, "Data", pJsonData);
    }

    data = Json::to_string(pJsonRoot);
    Json::deinit(pJsonRoot);
    return;
}

bool CTask::verify_requester(Common::Requester_E enRequester)
{
    std::unique_lock<std::mutex> mtx(m_mtx);
    for (auto &stInfo : m_infos)
    {
        if (stInfo.enRequester == enRequester)
        {
            return true;
        }
    }
    return false;
}
void CTask::register_subscribe(ResultCallback fnResultCallback)
{
    m_subscribe.insert(fnResultCallback);
}
std::string CTask::fill_common(std::string data, int nRet, const Info_S &stInfo)
{
    Json::Object *pJsonRoot = Json::init();
    Json::add(pJsonRoot, "ActionCode", m_nActionCode);
    Json::add(pJsonRoot, "DeviceName", DEVICE_CODE);
    Json::add(pJsonRoot, "UserName", stInfo.user);
    if (stInfo.nUniqueId != 0)
    {
        Json::add(pJsonRoot, "UniqueId", stInfo.nUniqueId);
    }
    Json::add(pJsonRoot, "Return", nRet);

    Json::Object *pJsonData = Json::init(data);
    if (pJsonData)
    {
        Json::add(pJsonRoot, "Data", pJsonData);
    }

    data = Json::to_string(pJsonRoot);
    Json::deinit(pJsonRoot);
    return std::move(data);
}
