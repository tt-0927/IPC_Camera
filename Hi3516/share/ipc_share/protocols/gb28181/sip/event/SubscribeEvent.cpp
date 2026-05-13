/*
 * @Author       : EasonLu
 * @Date         : 2025-04-21 09:11:48
 * @LastEditors  : EasonLu
 * @LastEditTime : 2025-04-21 09:15:21
 * @FilePath     : SubscribeEvent.cpp
 * @Description  : 订阅事件
 */
#include "SubscribeEvent.h"
#include "SipClient.h"
#include "SipDevice.h"
#include "SipUtils.h"
#include "XmlParser.h"
#include <sstream>
#include <sys/time.h>
#include <thread>
#include <vector>

#define SUBSCRIBE_EVENT_DEBUG 1
/* 使用Notify消息发送 */
#define SUBSCRIBE_SEND_BY_NOTIFY 1

/* 通知的消息间隔（单位：毫秒） */
#ifndef SUBSCRIBE_NOTIFY_GAP
#define SUBSCRIBE_NOTIFY_GAP 250 /* 使用Notity方法的发送间隔 */
#endif
/* 启动发送线程的等待时间（单位：毫秒） */
#define SUBSCRIBE_SEND_WAIT_TIME 1000
/* 最大报警消息缓存数——存在没有网络的情况 */
#define SUBSCRIBE_ALARM_MAX 500
/* 报警消息重发时间间隔（单位：毫秒） */
#define SUBSCRIBE_ALARM_RESEND_TIME 1000

using namespace SIP;
SIP::SubscribeEvent::SubscribeEvent()
    : m_queAlarm(SUBSCRIBE_ALARM_MAX)
{
}
SIP::SubscribeEvent::~SubscribeEvent()
{
    /* 先清理目录通知线程 */ 
    if (m_pThrCatalog != nullptr)
    {
        MLOG_INFO("正在停止目录通知线程");
        m_pThrCatalog->stop();
        
        /* 等待线程完全停止 */ 
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        
        delete m_pThrCatalog;
        m_pThrCatalog = nullptr;
    }

    if (m_bThrRun.load())
    {
        m_bThrRun.store(false);
        m_queAlarm.exit();
        if (m_pThrAlarm && m_pThrAlarm->joinable())
        {
            m_pThrAlarm->join();
            delete m_pThrAlarm;
            m_pThrAlarm = nullptr;
        }
    }
}
bool SIP::SubscribeEvent::Handle(const SipEvent::Ptr &e)
{
    int nExpires = GetExpires(e);
    MLOG_DEBUG("收到订阅请求，Expires: %d 秒", nExpires);
    pugi::xml_document doc;
    if (ParseHeader(e) < 0)
    {
        return false;
    }
#if SUBSCRIBE_EVENT_DEBUG
    MLOG_DEBUG("订阅一级事件[%d]二级事件[%d]时长[%d]CmdType[%s],nSubscribeID[%d]",
               m_header.cmd_category,
               m_header.cmd_type,
               nExpires,
               m_header.strCmdType.c_str(),e->m_pEvent->did);
    MLOG_DEBUG("SN[%s]", m_header.strSN.c_str());
    MLOG_DEBUG("DevID[%s]", m_header.strDevID.c_str());
#endif

    /* TODO 建立订阅列表，记录订阅的操作 */
    bool bIsSupport = false;
    switch (m_header.cmd_type)
    {
    case MANSCDP_QUERY_CMD_CATALOG:
        bIsSupport = true;
        break;
    case MANSCDP_QUERY_CMD_ALARM:
        bIsSupport = true;
        break;
    default:
        break;
    }

    /* 更新订阅请求的记录 */
    Data_S stSubData;
    stSubData.enCmdType = m_header.cmd_type;
    stSubData.nExpires = nExpires;
    stSubData.nStartTime = time(nullptr);
    stSubData.nLeftTime = nExpires;
    stSubData.nSubscribeID = e->m_pEvent->did;
    stSubData.strSN = m_header.strSN;
    stSubData.strDevID = m_header.strDevID;
    stSubData.strCmdType = m_header.strCmdType;
    stSubData.pClient = dynamic_cast<SipClient *>(e->m_pNetBase);
    GetRequestURI(e, stSubData.strFromUri, stSubData.strToUri);

    std::unique_lock<std::mutex> lock(m_mutexMap);
    m_mapSubscribe[stSubData.enCmdType] = stSubData;
    /* 回复200 + 结果描述 */
    HandleResponse(e, m_header, bIsSupport);
    if (bIsSupport)
    {
        if (MANSCDP_QUERY_CMD_CATALOG == stSubData.enCmdType)
        {
            NotifyCatalog();
        }
    }
    return true;
}

int SIP::SubscribeEvent::NotifyCatalog()
{
    /* 异步操作 */
#if 0
    std::thread thr([this]()
                    { thrNotifyCatalog(); });
    thr.detach();
#else
    std::lock_guard<std::mutex> lock(m_mutexCatalog);
    /* 创建一条延时发送的线程，后续继续有发送，则重新创建，确保延时期间只发送一次 */
    if (m_pThrCatalog != nullptr)
    {
        MLOG_WARN("已存在等待发送的目录通知线程，将重新创建");
        m_pThrCatalog->stop();
        delete m_pThrCatalog;
        m_pThrCatalog = nullptr;
    }
    m_pThrCatalog = new BlockThread(
        std::bind(&SubscribeEvent::thrNotifyCatalog, this),
        std::chrono::milliseconds(SUBSCRIBE_SEND_WAIT_TIME),
        true);
    m_pThrCatalog->start();
    MLOG_INFO("创建目录通知线程成功，将在[%d]毫秒后进行发送",
              SUBSCRIBE_SEND_WAIT_TIME);
#endif
    return 0;
}

int SIP::SubscribeEvent::NotifyAlarm(GB28181::AlarmInfo_S &stInfo)
{
    if (0 == stInfo.nTime)
    {
        /* 获取当前时间（包括毫秒） */
        struct timeval tv;
        gettimeofday(&tv, nullptr);

        /* 记录当前时间的秒数 */
        stInfo.nTime = tv.tv_sec;
    }
#if 1
    if (!m_bThrRun.load())
    {
        MLOG_DEBUG("启动报警通知线程");
        /* 线程没跑则先开启线程 */
        m_bThrRun.store(true);
        m_pThrAlarm = new std::thread(std::bind(&SubscribeEvent::thrNotifyAlarm, this));
    }
    /* 存入队列发送*/
    m_queAlarm.push(stInfo);
#else
    /* 直接发送消息 */
    SendAlarmMsg(stInfo);
#endif
    return 0;
}

int SIP::SubscribeEvent::Clear()
{
    /* 添加线程清理 */ 
    {
        std::lock_guard<std::mutex> lock(m_mutexCatalog);
        if (m_pThrCatalog != nullptr)
        {
            MLOG_INFO("清理目录通知线程");
            m_pThrCatalog->stop();
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            delete m_pThrCatalog;
            m_pThrCatalog = nullptr;
        }
    }
    
    /* 清理报警线程 */ 
    if (m_bThrRun.load())
    {
        m_bThrRun.store(false);
        m_queAlarm.exit();
        
        if (m_pThrAlarm && m_pThrAlarm->joinable())
        {
            m_pThrAlarm->join();
            delete m_pThrAlarm;
            m_pThrAlarm = nullptr;
        }
    }

    std::unique_lock<std::mutex> lock(m_mutexMap);
    m_mapSubscribe.clear();
    return 0;
}

int SIP::SubscribeEvent::SendNotify(
    const Data_S &stSubData,
    const std::string &strBody)
{
    if (nullptr == stSubData.pClient)
    {
        MLOG_WARN("订阅客户端为空");
        return -1;
    }
    if (!stSubData.pClient->GetRegister())
    {
        MLOG_WARN("订阅客户端未注册或已取消注册");
        return -2;
    }
    auto pContext = stSubData.pClient->GetSipContext();
    /* FIXME 偶尔会构建Notify消息失败，返回-3或-6，状态不对或找不到订阅消息 */
#if SUBSCRIBE_SEND_BY_NOTIFY
    osip_message_t *notify = nullptr;
    int nRet = eXosip_insubscription_build_notify(
        pContext,
        stSubData.nSubscribeID,
        EXOSIP_SUBCRSTATE_ACTIVE,
        0,
        &notify);
    if (notify != nullptr)
    {
        /* 数据体 */
        osip_message_set_content_type(notify, "Application/MANSCDP+xml");
        osip_message_set_body(notify, strBody.c_str(), strBody.length());
        eXosip_lock(pContext);
        nRet = eXosip_insubscription_send_request(pContext, stSubData.nSubscribeID, notify);
        eXosip_unlock(pContext);
    }
    else
    {
        MLOG_WARN("构建Notify消息失败[%d]", nRet);
    }

#else
    /* 直接发送Message方法 */
    osip_message_t *message = nullptr;
    int nRet = eXosip_message_build_request(
        pContext,
        &message,
        "Message",
        stSubData.strFromUri.c_str(),
        stSubData.strToUri.c_str(),
        nullptr);
    if (message != nullptr)
    {
        osip_message_set_content_type(message, "Application/MANSCDP+xml");
        osip_message_set_body(message, strBody.c_str(), strBody.length());
        eXosip_lock(pContext);
        nRet = eXosip_message_send_request(pContext, message);
        eXosip_unlock(pContext);
    }
    else
    {
        MLOG_WARN("构建Message消息失败[%d]", nRet);
    }

#endif
    return nRet;
}

void SIP::SubscribeEvent::thrNotifyCatalog()
{
    Data_S stSubData;
    if (!GetSubData(MANSCDP_QUERY_CMD_CATALOG, stSubData))
    {
        /* 没有相关的订阅记录或订阅过期了 */
        return;
    }
    { /* 拼装消息，逐条发送 */
        std::vector<Channel::Ptr> vecChnList;
        stSubData.pClient->GetChnInfo(vecChnList);
        auto nChnTotal = vecChnList.size();
        MLOG_DEBUG("获取客户端的目录数量:[%d]", nChnTotal);
        pugi::xml_document stNewDoc;

        auto declaration = stNewDoc.append_child(pugi::node_declaration);
        auto attrVersion = declaration.append_attribute("version");
        attrVersion.set_value("1.0");
        auto attrEncoding = declaration.append_attribute("encoding");
        attrEncoding.set_value("GB18030");

        auto root = stNewDoc.append_child("Notify");

        auto nodeCmdType = root.append_child("CmdType");
        nodeCmdType.text().set("Catalog");

        auto nodeSN = root.append_child("SN");
        nodeSN.text().set(stSubData.strSN);

        auto nodeDeviceID = root.append_child("DeviceID");
        nodeDeviceID.text().set(stSubData.strDevID);

        auto nodeSumNum = root.append_child("SumNum");
        nodeSumNum.text().set(std::to_string(nChnTotal));

        /* 每次发送一个通道信息 */
        auto nodeDevList = root.append_child("DeviceList");
        nodeDevList.append_attribute("Num").set_value(std::to_string(1));

        auto nodeItem = nodeDevList.append_child("Item");
        nodeItem.append_child("DeviceID");
        nodeItem.append_child("Name");
        nodeItem.append_child("Manufacturer");
        nodeItem.append_child("Model");
        nodeItem.append_child("Owner");
        nodeItem.append_child("CivilCode");
        nodeItem.append_child("Address");
        nodeItem.append_child("Parental");
        nodeItem.append_child("ParentID");
        nodeItem.append_child("RegisterWay");
        nodeItem.append_child("Secrecy");
        nodeItem.append_child("Status");

        for (auto &item : vecChnList)
        {
            nodeItem.child("DeviceID").text().set(item->strChannelID);
            nodeItem.child("Name").text().set(item->strName);
            nodeItem.child("Manufacturer").text().set(item->strManufacturer);
            nodeItem.child("Model").text().set(item->strModel);
            nodeItem.child("Owner").text().set(item->strOwner);
            nodeItem.child("CivilCode").text().set(item->strCivilCode);
            nodeItem.child("Address").text().set(item->strAddress);
            nodeItem.child("Parental").text().set(item->strParental);
            nodeItem.child("ParentID").text().set(item->strParentID);
            nodeItem.child("RegisterWay").text().set(item->strRegisterWay);
            nodeItem.child("Secrecy").text().set(item->strSecrecy);
            nodeItem.child("Status").text().set(item->strStatus);
            std::ostringstream os;
            stNewDoc.save(os);
            auto strGB18030 = ::ToMbcsString(os.str());
            /* 发送Notify消息 */
            SendNotify(stSubData, strGB18030);
#if SUBSCRIBE_SEND_BY_NOTIFY
            /* 使用Notify时，需间隔一定时间发送消息 */
            std::this_thread::sleep_for(std::chrono::milliseconds(SUBSCRIBE_NOTIFY_GAP));
#endif
        }
    }
    return;
}

void SIP::SubscribeEvent::thrNotifyAlarm()
{
	pthread_setname_np(pthread_self(), "SIPNotifyAlarm");

    MLOG_INFO("开始处理上抛报警事件");
    while (m_bThrRun.load())
    {
        GB28181::AlarmInfo_S stInfo;
        if (!m_queAlarm.peek(stInfo))
        {
            continue;
        }

        if (SendAlarmMsg(stInfo) < 0)
        {
            /* 发送失败则等待一定时间后再发送 */
            std::this_thread::sleep_for(std::chrono::milliseconds(SUBSCRIBE_ALARM_RESEND_TIME));
        }
        else
        {
            /* 发送成功则删除队列中的数据 */
            m_queAlarm.pop(stInfo);
        }
    }
    MLOG_INFO("结束处理上抛报警事件");
}

bool SIP::SubscribeEvent::GetSubData(int enCmdType, Data_S &stSubData)
{
    /* 固定类型值 */
    auto pFind = m_mapSubscribe.find(enCmdType);
    if (pFind == m_mapSubscribe.end())
    {
        /* 没有相关订阅记录则直接返回 */
        return false;
    }
    /* 计算是否过了订阅有效期 */
    auto nCurrentTime = time(nullptr);
    auto nElapsedTime = nCurrentTime - pFind->second.nStartTime;  // 已经过去的时间
    auto nLeftTime = pFind->second.nExpires - nElapsedTime;       // 剩余时间
    
    MLOG_DEBUG("订阅类型[%d] 当前时间[%ld] 开始时间[%ld] 已过去[%ld]秒 订阅时长[%d]秒 剩余[%ld]秒", 
               enCmdType, nCurrentTime, pFind->second.nStartTime, nElapsedTime, pFind->second.nExpires, nLeftTime);
    
    if (nLeftTime <= 0)
    {
        MLOG_INFO("订阅类型[%d]已过期，剩余时间[%ld]秒", enCmdType, nLeftTime);
        m_mapSubscribe.erase(pFind);
        return false;
    }
    /* 更新剩余时间 */
    pFind->second.nLeftTime = nLeftTime;
    if (pFind->second.pClient == nullptr)
    {
        MLOG_WARN("订阅记录中的客户端实例为空，无法发送订阅通知");
        return false;
    }
    /* 订阅未过期，更新数据 */
    MLOG_INFO("订阅类型[%d][%s]的Notify通知，剩余有效期[%ld]秒",
              enCmdType,
              pFind->second.strCmdType.c_str(),
              nLeftTime);
    stSubData = pFind->second;
    return true;
}

int SIP::SubscribeEvent::SendAlarmMsg(GB28181::AlarmInfo_S &stInfo)
{
    Data_S stSubData;
    if (!GetSubData(MANSCDP_QUERY_CMD_ALARM, stSubData))
    {
        /* 没有相关的订阅记录或订阅过期了 */
        MLOG_WARN("没有相关的订阅记录或订阅过期了");
        return -1;
    }
    std::string strXml;
    { /* 拼接消息 */
        pugi::xml_document stNewDoc;

        auto declaration = stNewDoc.append_child(pugi::node_declaration);
        auto attrVersion = declaration.append_attribute("version");
        attrVersion.set_value("1.0");
        auto attrEncoding = declaration.append_attribute("encoding");
        attrEncoding.set_value("GB18030");

        auto root = stNewDoc.append_child("Notify");

        auto nodeCmdType = root.append_child("CmdType");
        nodeCmdType.text().set("Alarm");

        auto nodeSN = root.append_child("SN");
        nodeSN.text().set(stSubData.strSN);

        auto nodeDeviceID = root.append_child("DeviceID");
        nodeDeviceID.text().set(m_header.strDevID);

        auto nodeAlarmPriority = root.append_child("AlarmPriority");
        nodeAlarmPriority.text().set(stInfo.enPriority);

        auto nodeAlarmMethod = root.append_child("AlarmMethod");
        nodeAlarmMethod.text().set(stInfo.enMethod);

        auto nodeAlarmTime = root.append_child("AlarmTime");
        nodeAlarmTime.text().set(::TimeTToISO8601(stInfo.nTime));

        { /* Info节点 */
            auto info = root.append_child("Info");

            auto nodeAlarmType = info.append_child("AlarmType");
            nodeAlarmType.text().set(stInfo.enType);

            if (stInfo.enTypeParam > 0)
            {
                auto nodeAlarmTypeParam = info.append_child("AlarmTypeParam");
                auto nodeEventType = nodeAlarmTypeParam.append_child("EventType");
                nodeEventType.text().set(stInfo.enTypeParam);
            }
        }

        std::ostringstream os;
        stNewDoc.save(os);
        strXml = ::ToMbcsString(os.str());
    }
    return SendNotify(stSubData, strXml);
}

int SIP::SubscribeEvent::HandleResponse(
    const SipEvent::Ptr &e,
    const manscdp_msgbody_header_t &header,
    bool bIsSupport)
{
    /* 构建并发送响应 */
    osip_message_t *answer = nullptr;
    /* 返回的消息值均为200 */
    int status = bIsSupport ? SIP_OK : SIP_FORBIDDEN;
    eXosip_insubscription_build_answer(
        e->m_pContext, e->m_pEvent->tid, status, &answer);

    if (answer != nullptr)
    {
        { /* 数据体 */
            pugi::xml_document stNewDoc;

            auto declaration = stNewDoc.append_child(pugi::node_declaration);
            auto attrVersion = declaration.append_attribute("version");
            attrVersion.set_value("1.0");
            auto attrEncoding = declaration.append_attribute("encoding");
            attrEncoding.set_value("GB18030");

            auto root = stNewDoc.append_child("Response");

            auto nodeCmdType = root.append_child("CmdType");
            nodeCmdType.text().set(m_header.strCmdType);

            auto nodeSN = root.append_child("SN");
            nodeSN.text().set(m_header.strSN);

            auto nodeDeviceID = root.append_child("DeviceID");
            nodeDeviceID.text().set(m_header.strDevID);

            auto nodeResult = root.append_child("Result");
            nodeResult.text().set(bIsSupport ? "OK" : "ERROR");

            std::ostringstream os;
            stNewDoc.save(os);
            auto strBody = os.str();

            osip_message_set_content_type(answer, "Application/MANSCDP+xml");
            osip_message_set_body(answer, strBody.c_str(), strBody.length());
        }
        eXosip_lock(e->m_pContext);
        eXosip_insubscription_send_answer(
            e->m_pContext, e->m_pEvent->tid, status, answer);
        eXosip_unlock(e->m_pContext);
    }
    return 0;
}
