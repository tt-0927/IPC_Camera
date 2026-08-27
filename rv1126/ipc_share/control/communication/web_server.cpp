/**
 * @FilePath     : web_server.cpp
 * @Author       : zhangjc (zhangjc@kfb.cn)
 * @Date         : 2024-10-08 13:46:32
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-01-29 10:59:33
 * @Description  : WebSocket服务器类
 */

#include "web_server.h"
#include "dlog.h"
#include "common_define.h"
#include "convert_interface.h"
#include "action_code.h"
#include "user_manage.h"
#include "libwebsockets.h"

namespace
{
/**
 * @brief   : 从登录结果中提取在线用户ID
 * @param    {std::string} result：登录返回JSON
 * @param    {int} &userId：解析出的在线用户ID
 * @return   {bool} true：解析成功，false：解析失败
 */
bool get_online_user_id_from_login_result(const std::string &result, int &userId)
{
    userId = 0;
    if (result.empty())
    {
        return false;
    }

    /* 保存登录结果根节点 */
    Json::Object *pJsonRoot = Json::init(result);
    if (!pJsonRoot)
    {
        return false;
    }

    /* 保存登录结果中的Data节点 */
    Json::Object *pJsonData = Json::get(pJsonRoot, "Data");
    if (!pJsonData)
    {
        Json::deinit(pJsonRoot);
        return false;
    }

    /* 保存Data节点序列化字符串 */
    std::string strData = Json::to_string(pJsonData);
    Json::get(strData.c_str(), "OnlineUserId", userId);
    Json::deinit(pJsonRoot);
    return userId > 0;
}
}

IpcRet_E CWebServer::init()
{
    Net::Param_S stParam;
    /* 设置心跳码、状态码 */
    stParam.stInitParam.nHearbeatCode = AC_HEARTBIT;
    stParam.stInitParam.nStatusCode = AC_STATUS;
    /* 设置命令码回调函数 */
    using namespace std::placeholders;
    stParam.stInitParam.fnDefaultCallback = std::bind(&CWebServer::deal_message, this, _1, _2);
    stParam.stInitParam.callbackMap[stParam.stInitParam.nHearbeatCode] = std::bind(&CWebServer::deal_heartbeat, this, _1, _2);
    stParam.stInitParam.callbackMap[stParam.stInitParam.nStatusCode] = std::bind(&CWebServer::deal_status, this, _1, _2);
    /* 设置ip、端口号 */
    stParam.stInitParam.ip = std::string("127.0.0.1");
    stParam.stInitParam.nPort = IN_WEB_CONTROL_PROT;
    /* 创建客户端 */
    m_pHandler = std::make_shared<Net::WebSocketServer>(stParam);

    /* 启动清理连接信息线程 */
    startCleanupThread();

    return OK;
}

IpcRet_E CWebServer::deinit()
{
    /* 停止清理连接信息线程 */
    stopCleanupThread();

    /* 检查指针是否有效，避免在为空时调用 disconnect */
    if (m_pHandler)
    {
        m_pHandler.reset();
    }
    
    return OK;
}

void CWebServer::set_taskManage(std::shared_ptr<CTaskManage> pTaskManage)
{
    m_pTaskManage = pTaskManage;
    
    /* 订阅设置 */
    std::vector<int> actionCode;
    // actionCode.push_back(AC_FACE_DETECT_EVENT);
    // actionCode.push_back(AC_NOTICE_IPC_CONNECT_STATUS);
    // actionCode.push_back(AC_NOTICE_IPC_ALARM_INFO);
    // actionCode.push_back(AC_GET_STATUS_ID_INFO);
#ifdef SCENE_INTELLIGENT_ANALYSIS   
    actionCode.push_back(AC_GET_REAL_ALARM_PUSH_INFO); 
    actionCode.push_back(AC_RETURN_IMAGE_ANALYSIS_RESULT);
#endif

#ifdef SCENE_INTELLIGENCE
    actionCode.push_back(AC_PUSH_FACE_CAPTURE_INFO);
    actionCode.push_back(AC_PUSH_PERSON_CAPTURE_INFO);
    actionCode.push_back(AC_PUSH_MOTORVEHICLE_CAPTURE_INFO);
    actionCode.push_back(AC_PUSH_NONMOTORVEHICLE_CAPTURE_INFO);
#endif

    actionCode.push_back(AC_DEL_AND_EXIT_USER);
    actionCode.push_back(AC_UPDATE_AND_EXIT_USER);

#ifdef ENABLE_GAT1400_SRC
    actionCode.push_back(AC_GET_GAT1400_INFO);
#endif

#ifdef ENABLE_AI_STUDENT
    actionCode.push_back((AC_GET_CLASS_INFO));
    actionCode.push_back((AC_SET_CLASS_INFO));
    actionCode.push_back(AC_GET_ATTENDANCE_INFO);
    actionCode.push_back(AC_GET_STUDENT_BEHAVIOR_INFO);
    actionCode.push_back(AC_GET_STUDENT_PERFORMANCE_INFO);
#endif

    auto fnResultCallbacks = std::bind(&CWebServer::send, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4);
    m_pTaskManage->register_subscribe(actionCode, fnResultCallbacks);
}
int CWebServer::send(const void *pData, int nDataLen, int nActionCode, void *pHandle)
{
    if (!m_pHandler)
    {
        return -1;
    }
    Net::Param_S stParam;
    Net::Message_S stMessage;
    stMessage.pHandle = pHandle;
    stMessage.nActionCode = nActionCode;
    stMessage.pData = pData;
    stMessage.nDataLength = nDataLen;
    return m_pHandler->send(stMessage);
}

Task::ResultCallback CWebServer::buildResultCallback(int nActionCode)
{
    if (nActionCode == AC_LOGIN)
    {
        return std::bind(&CWebServer::sendWithLoginBind,
                         this,
                         std::placeholders::_1,
                         std::placeholders::_2,
                         std::placeholders::_3,
                         std::placeholders::_4);
    }

    return std::bind(&CWebServer::send,
                     this,
                     std::placeholders::_1,
                     std::placeholders::_2,
                     std::placeholders::_3,
                     std::placeholders::_4);
}

int CWebServer::sendWithLoginBind(const void *pData, int nDataLen, int nActionCode, void *pHandle)
{
    if (pData && nDataLen > 0 && nActionCode == AC_LOGIN)
    {
        /* 保存登录返回中的在线用户ID */
        int userId = 0;
        /* 保存完整登录返回报文 */
        std::string result(static_cast<const char *>(pData), nDataLen);
        if (get_online_user_id_from_login_result(result, userId) && !setUserIdForConnection(pHandle, userId))
        {
            dlog_warn("登录成功但连接已断开，立即清理在线用户 userId:%d", userId);
            CUserManage::instance()->delete_online_user(userId);
        }
    }

    return send(pData, nDataLen, nActionCode, pHandle);
}

void CWebServer::deal_heartbeat(Net::Message_S& stMessage, Net::UserParam_S &stUserParam)
{
    // dlog_debug("接收到心跳消息：%s", stMessage.pData);
}

void CWebServer::deal_status(Net::Message_S& stMessage, Net::UserParam_S &stUserParam)
{
    int nStatus = *(const int *)stMessage.pData;
    if (nStatus == Net::STATUS_SUCCESS)
    {
        dlog_info("客户端已接入 ip: %s, handle: %p", stMessage.ip.c_str(), stMessage.pHandle);

        /* 添加连接到管理器 */
        addConnection(stMessage.pHandle, stMessage.ip);

        if (m_statusObserver)
        {
            m_statusObserver(Common::REQUESTER_WEB, true);
        }
    }
    else
    {
        dlog_info("客户端已断开 ip: %s, handle: %p", stMessage.ip.c_str(), stMessage.pHandle);
        
        if (m_statusObserver)
        {
            m_statusObserver(Common::REQUESTER_WEB, false);
        }

        /* 移除连接 */
        removeConnection(stMessage.pHandle);
    }
}

void CWebServer::deal_message(Net::Message_S& stMessage, Net::UserParam_S &stUserParam)
{
    if (m_pTaskManage == nullptr)
    {
        dlog_error("无绑定任务");
        return;
    }
    Json::get(static_cast<const char *>(stMessage.pData), "ActionCode", (int &)stMessage.nActionCode);
    if (stMessage.nActionCode <= 0)
    {
        return;
    }

    /* 获取登录设备的ip */
    if (stMessage.nActionCode  == AC_LOGIN)
    {
        dlog_info("开始获取登录设备的IP");
       
        if(!stMessage.ip.empty())
        {
            dlog_info("登录的设备ip：[%s]", stMessage.ip.c_str());
        }
        else
        {
            dlog_error("获取登录设备ip失败");
        }
        m_LoginDeviceIp = stMessage.ip;
    }
    else if (stMessage.nActionCode == AC_LOCAL_ONLINE_USER || stMessage.nActionCode == AC_DELETE_ONLINE_USERS)
    {
        dlog_debug("收到 AC_LOCAL_ONLINE_USER/AC_DELETE_ONLINE_USERS: %s", stMessage.pData);
        Json::Object *pJsonRoot = Json::init(static_cast<const char*>(stMessage.pData));
        if (pJsonRoot)
        {
            Json::Object *pJsonData = Json::get(pJsonRoot, "Data");
            if(!pJsonData)
            {
                Json::deinit(pJsonRoot);
                return;
            }
            std::string data = Json::to_string(pJsonData);
            Json::deinit(pJsonRoot);
            /* 用户数据处理 */
            User::UserInfo_S stUserInfo;
            Convert::to_struct(data, stUserInfo);
            if (stMessage.nActionCode == AC_LOCAL_ONLINE_USER)
            {
                /* 更新当前句柄对应的在线用户ID */
                setUserIdForConnection(stMessage.pHandle, stUserInfo.stAccountInfo.stOnlineUser.nOnlineUserId);
                return;
            }
            else if (stMessage.nActionCode == AC_DELETE_ONLINE_USERS)
            {
                /* 移除当前句柄对应的在线用户ID */
                deleteUserIdForConnection(stMessage.pHandle, stUserInfo.stAccountInfo.stOnlineUser.nOnlineUserId);
                /* 关闭对讲/广播 */
                closeAudio(stMessage.ip);
            }
        }
    }

    dlog_debug("接收到[%s]的[%d]消息：%s", stMessage.ip.c_str(), stMessage.nActionCode, stMessage.pData);
    Task::Info_S stInfo;
    stInfo.pHandler = stMessage.pHandle;
    stInfo.data = static_cast<const char*>(stMessage.pData);
    stInfo.enRequester = Common::REQUESTER_WEB;
    /* 修正字符串实际长度 */
    stMessage.ip.resize(strlen(stMessage.ip.c_str()));
    stInfo.strIp = stMessage.ip;
    m_UserDeviceIp = stMessage.ip;
    stInfo.fnResultCallbacks = buildResultCallback(stMessage.nActionCode);
    int ret = m_pTaskManage->execute(stMessage.nActionCode, stInfo);
    if (ret < 0)
    {
        dlog_error("未绑定任务");
        return;
    }
}

std::string CWebServer::get_loginclient_ip()
{
    return m_LoginDeviceIp;
}

std::string CWebServer::get_userclient_ip()
{
    return m_UserDeviceIp;
}

void CWebServer::set_statusObserver(Common::StatusCallback observer)
{
    m_statusObserver = observer;
}

void CWebServer::startCleanupThread()
{
    m_bStopCleanup = false;
    m_cleanupThread = std::thread(&CWebServer::cleanupThreadFunc, this);
}

void CWebServer::stopCleanupThread()
{
    m_bStopCleanup = true;
    if (m_cleanupThread.joinable())
    {
        m_cleanupThread.join();
    }
}

void CWebServer::cleanupThreadFunc()
{
    pthread_setname_np(pthread_self(), "WebServerClean");
    while (!m_bStopCleanup)
    {
        std::this_thread::sleep_for(std::chrono::seconds(CLEANUP_CHECK_INTERVAL));
        std::lock_guard<std::mutex> lock(m_connectionMutex);
        time_t currentTime = time(nullptr);

        auto it = m_vecPendingCleanup.begin();
        while (it != m_vecPendingCleanup.end())
        {
            /* 检查是否超过宽限期 */
            if (currentTime - it->disconnectTime >= RECONNECT_GRACE_PERIOD)
            {
                /* 宽限期已过，用户没有重新连接，执行真正的下线处理 */
                dlog_info("用户 %d (IP:%s) 的重连宽限期已过，执行下线", it->userId, it->ip.c_str());
                /* 处理用户下线 */
                handleUserOffline(it->userId, it->ip);
                /* 关闭对讲/广播 */
                closeAudio(it->ip);
                /* 从待清理列表中移除 */
                it = m_vecPendingCleanup.erase(it);
            }
            else
            {
                ++it;
            }
        }
    }
}

void CWebServer::addConnection(void *pHandle, const std::string &ip)
{
    std::lock_guard<std::mutex> lock(m_connectionMutex);

    ConnectionInfo_S info;
    info.pHandle = pHandle;
    info.ip = ip.c_str(); // .c_str() 避免内部string length异常的问题
    info.userId = 0;      // 刚连接时，用户ID为0，代表未关联

    m_mapConnections[pHandle] = info;

    dlog_info("添加连接 handle:%p, ip:%s", pHandle, ip.c_str());
}

void CWebServer::removeConnection(void* pHandle)
{
    std::lock_guard<std::mutex> lock(m_connectionMutex);
    
    auto it = m_mapConnections.find(pHandle);
    if (it == m_mapConnections.end())
    {
        dlog_warn("尝试移除一个不存在的连接 handle:%p", pHandle);
        return;
    }

    ConnectionInfo_S disconnectedConn = it->second;
    /* 从活动连接映射中移除此连接 */
    m_mapConnections.erase(it);
    dlog_info("移除连接 handle:%p, ip:%s, userId:%d", pHandle, disconnectedConn.ip.c_str(), disconnectedConn.userId);

    /* 如果此连接未关联任何用户 (userId为0)，则无需任何操作 */
    if (disconnectedConn.userId == 0)
    {
        dlog_debug("未登录的连接断开，无需处理");
        /* 强制关闭对讲/广播，不管当前IP是否开启，后续对讲/广播 task 中进行判断ip */
        closeAudio(disconnectedConn.ip);
        return;
    }

    /* 检查该用户是否还有其他活动的连接 */
    if (hasOtherConnections(disconnectedConn.userId))
    {
        dlog_info("用户 %d (IP:%s) 关闭了一个连接，但仍有其他活动连接，不执行下线。", disconnectedConn.userId, disconnectedConn.ip.c_str());
    }
    else
    {
        /* 这是该用户的最后一个连接，将其加入待清理列表，启动下线倒计时 */
        dlog_info("用户 %d (IP:%s) 的最后一个连接已断开，加入待清理列表 (宽限期: %d 秒)。", disconnectedConn.userId, disconnectedConn.ip.c_str(), RECONNECT_GRACE_PERIOD);
        PendingCleanup_S cleanup;
        cleanup.ip = disconnectedConn.ip;
        cleanup.userId = disconnectedConn.userId;
        cleanup.disconnectTime = time(nullptr);
        m_vecPendingCleanup.push_back(cleanup);
    }
}

bool CWebServer::setUserIdForConnection(void* pHandle, const int userId)
{
    std::lock_guard<std::mutex> lock(m_connectionMutex);

    if (userId == 0)
    {
        dlog_warn("尝试为一个连接设置无效的用户ID(0)");
        return false;
    }

    auto it = m_mapConnections.find(pHandle);
    if (it != m_mapConnections.end())
    {
        /* 更新当前连接句柄的用户ID */
        it->second.userId = userId;
        dlog_info("关联成功: handle:%p -> userId:%d", pHandle, userId);

        /* 检查该用户是否在待清理列表中，如果是，则移除 */
        auto cleanupIt = m_vecPendingCleanup.begin();
        while (cleanupIt != m_vecPendingCleanup.end())
        {
            if (cleanupIt->userId == userId)
            {
                dlog_info("用户 %d 在宽限期内重新连接，取消待处理的下线。", userId);
                cleanupIt = m_vecPendingCleanup.erase(cleanupIt);
                /* 因为一个userId只会有一个待下线记录，找到后即可退出循环 */
                break;
            }
            else
            {
                ++cleanupIt;
            }
        }
        return true;
    }

    dlog_error("尝试为不存在的连接 handle:%p 设置用户ID:%d", pHandle, userId);
    return false;
}

void CWebServer::deleteUserIdForConnection(void *pHandle, const int userId)
{
    std::lock_guard<std::mutex> lock(m_connectionMutex);
    
    if (userId == 0)
    {
        dlog_warn("尝试为一个连接设置无效的用户ID(0)");
        return;
    }

    auto it = m_mapConnections.find(pHandle);
    if (it != m_mapConnections.end())
    {
        if (it->second.userId != userId)
        {
            return;
        }

        /* 删除当前连接句柄的用户ID */
        it->second.userId = 0;
        dlog_info("删除关联成功: handle:%p -> userId:%d -> userId:0", pHandle, userId);
    }
    else
    {
        dlog_error("尝试为不存在的连接 handle:%p 删除用户ID:%d", pHandle, userId);
    }
}

bool CWebServer::hasOtherConnections(const int userId)
{
    /* 调用此函数时，必须已持有 m_connectionMutex 锁 */
    for (const auto& pair : m_mapConnections)
    {
        if (pair.second.userId == userId)
        {
            return true; /* 找到另一个属于该用户的连接 */
        }
    }
    return false; /* 未找到其他连接 */
}

bool CWebServer::hasActiveLoggedInConnectionByIp(const std::string &ip)
{
    /* 保护连接映射读取 */
    std::lock_guard<std::mutex> lock(m_connectionMutex);
    for (const auto &pair : m_mapConnections)
    {
        if (pair.second.ip == ip && pair.second.userId != 0)
        {
            return true;
        }
    }
    return false;
}

void CWebServer::clearPendingCleanupByIp(const std::string &ip)
{
    /* 保护待下线列表写入 */
    std::lock_guard<std::mutex> lock(m_connectionMutex);

    /* 保存待清理列表迭代器 */
    auto it = m_vecPendingCleanup.begin();
    while (it != m_vecPendingCleanup.end())
    {
        if (it->ip == ip)
        {
            dlog_info("移除IP[%s]的待下线记录 userId:%d", ip.c_str(), it->userId);
            it = m_vecPendingCleanup.erase(it);
        }
        else
        {
            ++it;
        }
    }
}

void CWebServer::handleUserOffline(const int userId, const std::string& ip)
{
    Json::Object *pJsonRoot = Json::init();
    Json::add(pJsonRoot, "ActionCode", AC_DELETE_ONLINE_USERS);
    Json::Object *pJsonData = Json::init();
    if (pJsonData)
    {
        Json::add(pJsonData, "OnlineUserId", userId);
        Json::add(pJsonData, "IpAddress", ip.c_str());
        Json::add(pJsonRoot, "Data", pJsonData);
    }
    auto data = Json::to_string(pJsonRoot);
    Json::deinit(pJsonRoot);

    /* 生成删除在线用户任务 */
    Task::Info_S stInfo;
    stInfo.data = data;
    stInfo.enRequester = Common::REQUESTER_WEB;
    stInfo.fnResultCallbacks = std::bind(&CWebServer::send, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4);
    int ret = m_pTaskManage->execute(AC_DELETE_ONLINE_USERS, stInfo);
    if (ret < 0)
    {
        dlog_error("执行用户下线任务失败 userId:%d", userId);
        return;
    }

    dlog_info("已触发用户下线任务 userId:%d, ip:%s", userId, ip.c_str());
}

void CWebServer::closeAudio(const std::string &ip)
{
    /* 关闭对讲 */
    Json::Object *pJsonRoot = Json::init();
    Json::add(pJsonRoot, "ActionCode", AC_SET_INTERCOM_INFO);
    Json::Object *pJsonData = Json::init();
    if (pJsonData)
    {
        Json::add(pJsonData, "Enable", false);
        Json::add(pJsonData, "LocalIp", ip.c_str());
        Json::add(pJsonRoot, "Data", pJsonData);
    }
    auto data = Json::to_string(pJsonRoot);
    Json::deinit(pJsonRoot);

    /* 生成关闭对讲任务 */
    Task::Info_S stInfo;
    stInfo.data = data;
    stInfo.enRequester = Common::REQUESTER_WEB;
    stInfo.strIp = ip;
    stInfo.fnResultCallbacks = std::bind(&CWebServer::send, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4);
    int ret = m_pTaskManage->execute(AC_SET_INTERCOM_INFO, stInfo);
    if (ret < 0)
    {
        dlog_error("执行关闭对讲任务失败 ip:%d", ip.c_str());
    }

    /* 关闭广播 */
    pJsonRoot = Json::init();
    Json::add(pJsonRoot, "ActionCode", AC_SET_BROADCAST_INFO);
    pJsonData = Json::init();
    if (pJsonData)
    {
        Json::add(pJsonData, "Enable", false);
        Json::add(pJsonData, "LocalIp", ip.c_str());
        Json::add(pJsonRoot, "Data", pJsonData);
    }
    data = Json::to_string(pJsonRoot);
    Json::deinit(pJsonRoot);

    /* 生成关闭广播任务 */
    stInfo.data = data;
    stInfo.enRequester = Common::REQUESTER_WEB;
    stInfo.strIp = ip;
    stInfo.fnResultCallbacks = std::bind(&CWebServer::send, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4);
    ret = m_pTaskManage->execute(AC_SET_BROADCAST_INFO, stInfo);
    if (ret < 0)
    {
        dlog_error("执行关闭广播任务失败 ip:%d", ip.c_str());
    }

    dlog_info("已触发关闭对讲/广播任务 ip:%s", ip.c_str());
}
