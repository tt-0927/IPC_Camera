/*
 * @Author       : EasonLu
 * @Date         : 2025-02-13 14:58:43
 * @LastEditors: error: error: git config user.name & please set dead value or install git && error: git config user.email & please set dead value or install git & please set dead value or install git
 * @LastEditTime: 2025-05-12 09:25:59
 * @FilePath     : EventBusiness.cpp
 * @Description  : 各类事件的业务处理
 */
#include "EventBusiness.h"
#include "dlog.h"
#include "CallSession.h"
#include "DeviceManage.h"
#include "HttpDigest.h"
#include "MediaRtp.h"
#include "MediaSdp.h"
#include "MethodRequest.h"
#include "PtzCmd.h"
#include "QueryEvent.h"
#include "RequestPool.h"
#include "SSRC_Config.h"
#include "SipClient.h"
#include "SipModule.h"
#include "SipNetBase.h"
#include "SipServer.h"
#include "SipType.h"
#include "SipUtils.h"
#include "StreamManager.h"
#include "XmlParser.h"
#include <sstream>
#include <string>
#include <thread>
#include "ConfigEvent.h"
#include <sys/time.h>

using namespace SIP;

#define EVENT_BUSINESS_DEBUG 0

/* 图像传输完成消息重发时间间隔（单位：毫秒） */
#define UPLOADSNAPSHOT_FINISH_RESEND_TIME 1000

int RegisterEvent::HandleIncomingRequest(const SipEvent::Ptr &e)
{
    auto pServer = dynamic_cast<SipServer *>(e->m_pNetBase);
    if (pServer == nullptr)
    {
        /* 非服务器的请求直接返回 */
        dlog_debug("非服务器请求，服务器实例为空");
        return 0;
    }
    osip_authorization_t *authorization = nullptr;
    osip_message_get_authorization(e->m_pEvent->request, 0, &authorization);

    /* NOTE 注册流程，有鉴权字段则默认鉴权，没有则默认允许注册 */

    /* 有鉴权字段则默认鉴权 */
    if (authorization && authorization->username)
    {
        char *method = nullptr;
        char *username = nullptr;
        char *uri = nullptr;
        char *response = nullptr;

        method = e->m_pEvent->request->sip_method;
        auto stSipServerInfo = pServer->GetSipServerInfo();
        /* 校验请求的服务器ID是否为本地服务器ID */
        auto sip_id = e->m_pEvent->request->req_uri->username;
        if (strcmp(sip_id, stSipServerInfo.strID.c_str()) != 0)
        {
            SendResponse(e, SIP_BAD_REQUEST);
            dlog_warn("Device Regist Failed, sip id doesn't match");
            dlog_warn("sip id = %s, server id = %s", sip_id, stSipServerInfo.strID.c_str());
            return -1;
        }

#define SIP_STRDUP(field)     \
    if (authorization->field) \
        (field) = osip_strdup_without_quote(authorization->field);

        SIP_STRDUP(uri);
        SIP_STRDUP(username);
        SIP_STRDUP(response);
        dlog_debug("URI[%s] Username[%s] Response[%s]", uri, username, response);
        HASHHEX HA1, calc_response;
        DigestCalcHA1("REGISTER", username, stSipServerInfo.strRealm.c_str(),
                      stSipServerInfo.strPassword.c_str(),
                      stSipServerInfo.strNonce.c_str(), NULL, HA1);
        DigestCalcResponse(HA1, stSipServerInfo.strNonce.c_str(),
                           NULL, NULL, NULL, 0, method, uri, NULL,
                           calc_response);
        dlog_info("MD5:[%s] Response:[%s]", calc_response, response);

        auto client_device_id = username;
        /* 鉴权成功后判断是注册还是注销 */
        if (0 == memcmp(calc_response, response, HASHHEXLEN))
        {
            /* 获取注册有效期 */
            int nRegisterExpires = GetExpires(e);
            std::string strRegIP;
            uint16_t nRegPort = 0;
            SendResponseAndGetAddress(e->m_pContext, e->m_pEvent->tid, SIP_OK,
                                      strRegIP, nRegPort);
            dlog_info("Device Authentication Success");
            if (nRegisterExpires > 0)
            { /* 设备数据相关更新 */
                std::string client_port = std::to_string(nRegPort);

                auto device = DeviceManage::instance()->GetDevice(client_device_id);
                if (device == nullptr)
                {
                    device = std::make_shared<Device>(client_device_id, strRegIP, client_port);
                    device->SetExternIP(stSipServerInfo.strExternIP);
                    DeviceManage::instance()->AddDevice(device);
                }
                else
                {
                    /* TODO 设备ID存在，是否拒绝请求？ */
                    // 如果设备已经存在的话，就只更新在线状态和注册时间
                    if (device->GetIP() != strRegIP || device->GetPort() != client_port)
                    {
                        dlog_warn("设备地址变化:ID[%s]  IP:[%s] => IP:[%s]",
                                  client_device_id, device->GetIP().c_str(), strRegIP);
                        device->SetIP(strRegIP);
                        device->SetPort(client_port);
                    }
                }
                /* 把设备状态回调函数设置给每一个设备，当设备状态变更时，自行调用 */
                device->SetRegistered(true);
                device->SetStatus(1);
                device->UpdateRegistTime();
                device->UpdateLastTime();
                device->SetExpires(nRegisterExpires);
                // 将设备和sip context关联，用于后期判断处理方便。
                device->exosip_context = e->m_pContext;

                { /* 获取设备信息 */
                    auto request = std::make_shared<DeviceInfoRequest>(e->m_pContext, device);
                    request->SendMessage();
                }

                { /* 获取设备目录才能获取到通道信息 */
                    auto request = std::make_shared<CatalogRequest>(e->m_pContext, device);
                    request->SendMessage();
                }
                dlog_info("设备注册成功ID:[%s]", client_device_id);
            }
            else if (nRegisterExpires == 0)
            {
                auto device = DeviceManage::instance()->GetDevice(client_device_id);
                if (device != nullptr)
                {
                    device->SetRegistered(false);
                    device->SetStatus(0);
                    device->UpdateRegistTime();
                    device->UpdateLastTime();
                    device->SetExpires(nRegisterExpires);
                    dlog_info("Device Unregistered Success ID:[%s]", client_device_id);
                }
            }
            else
            {
                dlog_warn("Device Unknow Registration Expires [%d] ID:[%s]",
                          nRegisterExpires, client_device_id);
            }
        }
        else
        {
            SendResponse(e, SIP_UNAUTHORIZED);
            dlog_info("Device authentication registration failed~~, ID:[%s]", client_device_id);

            DeviceManage::instance()->RemoveDevice(client_device_id);
        }
        osip_free(uri);
        osip_free(username);
        osip_free(response);
    }
    else
    {
        /* 无鉴权字段则通知客户端需要进行鉴权 */
        _response_register_401unauthorized(e);
    }

    return 0;
}

void RegisterEvent::_response_register_401unauthorized(const SipEvent::Ptr &e)
{
    auto pServer = dynamic_cast<SipServer *>(e->m_pNetBase);
    if (nullptr == pServer)
    {
        /* 不是来自服务器的不处理 */
        return;
    }
    osip_www_authenticate_t *www_authenticate_header = nullptr;
    osip_www_authenticate_init(&www_authenticate_header);

    char *dest = nullptr;
    osip_message_t *response = nullptr;

    auto stSipServerInfo = pServer->GetSipServerInfo();
    osip_www_authenticate_set_auth_type(www_authenticate_header, osip_strdup("Digest"));
    osip_www_authenticate_set_realm(www_authenticate_header, osip_enquote(stSipServerInfo.strRealm.c_str()));
    osip_www_authenticate_set_nonce(www_authenticate_header, osip_enquote(stSipServerInfo.strNonce.c_str()));
    osip_www_authenticate_to_str(www_authenticate_header, &dest);
    int ret = eXosip_message_build_answer(e->m_pContext, e->m_pEvent->tid, SIP_UNAUTHORIZED, &response);
    if (ret == 0 && response != nullptr)
    {
        osip_message_set_www_authenticate(response, dest);
        osip_message_set_content_type(response, "Application/MANSCDP+xml");
        eXosip_lock(e->m_pContext);
        eXosip_message_send_answer(e->m_pContext, e->m_pEvent->tid, SIP_UNAUTHORIZED, response);
        eXosip_unlock(e->m_pContext);
        dlog_info("发送401响应tid[%d]", e->m_pEvent->tid);
    }
    else
    {
        dlog_error("response_register_401unauthorized error");
    }

    osip_www_authenticate_free(www_authenticate_header);
    osip_free(dest);
}

void RegisterEvent::_response_register_302moveTemporarily(const SipEvent::Ptr &e)
{
    auto pServer = dynamic_cast<SipServer *>(e->m_pNetBase);
    if (nullptr == pServer)
    {
        /* 不是来自服务器的不处理 */
        return;
    }

    char *dest = nullptr;
    osip_message_t *response = nullptr;

    auto stSipServerInfo = pServer->GetSipServerInfo();
    int ret = eXosip_message_build_answer(e->m_pContext, e->m_pEvent->tid, 302, &response);
    if (ret == 0 && response != nullptr)
    {
        osip_message_set_content_type(response, "Application/MANSCDP+xml");
        osip_message_set_contact(response, "sip:34020000002000000001@172.16.25.8:15060");
        eXosip_lock(e->m_pContext);
        eXosip_message_send_answer(e->m_pContext, e->m_pEvent->tid, 302, response);
        eXosip_unlock(e->m_pContext);
        dlog_info("发送302响应tid[%d]", e->m_pEvent->tid);
    }
    else
    {
        dlog_error("response_register_302moveTemporarily error");
    }
}

MessageEvent::MessageEvent()
  :m_queSnapShot(500)
{

}

MessageEvent::~MessageEvent()
{
    if (m_bThrRun.load())
    {
        m_bThrRun.store(false);
        m_queSnapShot.exit();
        if (m_pThrSnapShot && m_pThrSnapShot->joinable())
        {
            m_pThrSnapShot->join();
            delete m_pThrSnapShot;
            m_pThrSnapShot = nullptr;
        }
    }
}
int MessageEvent::HandleIncomingRequest(const SipEvent::Ptr &e)
{
    if (ParseHeader(e) < 0)
    {
        return -1;
    }

    /* NOTE SipServer中已打印SIP消息,不再打印 */
    dlog_trace("cmd_category[%d] cmd_type[%d]", m_header.cmd_category, m_header.cmd_type);

    switch (m_header.cmd_category)
    {
    case MANSCDP_CMD_CATEGORY_CONTROL:
    {
        DeviceCtrlEvent h;
        h.Handle(e);
        break;
    }
    case MANSCDP_CMD_CATEGORY_QUERY:
    {
        QueryEvent h;
        h.SetSnapShotMap(m_mapSnapShotFini);
        h.Handle(e);
        break;
    }
    case MANSCDP_CMD_CATEGORY_NOTIFY:
    {
        if (m_header.cmd_type == MANSCDP_NOTIFY_CMD_KEEPALIVE)
        {
            HeartbeatEvent h;
            h.Handle(e);
        }
        else if (m_header.cmd_type == MANSCDP_NOTIFY_CMD_ALARM)
        {
            /* 订阅的报警信息处理 */
            AlarmEvent h;
            h.Handle(e);
        }

        else if(m_header.cmd_type == MANSCDP_NOTIFY_CMD_BROADCASE)
        {
            BroadcastEvent h;
            h.HandleRequest(e);
        }
        else if(m_header.cmd_type == MANSCDP_NOTIFY_CMD_UPLOADSNAPSHOTFINISHED)
        {
            UploadSnapShotFinishEvent h;
            h.Handle(e);
        }


    }
    case MANSCDP_CMD_CATEGORY_RESPONSE:
    {
        if (m_header.cmd_type == MANSCDP_QUERY_CMD_CATALOG)
        {
            CatalogEvent h;
            h.Handle(e);
        }
        else if (m_header.cmd_type == MANSCDP_QUERY_CMD_DEVICE_INFO)
        {
            DeviceInfoEvent h;
            h.Handle(e);
        }
        else if (m_header.cmd_type == MANSCDP_QUERY_CMD_PRESET_QUERY)
        {
            PresetQueryEvent h;
            h.Handle(e);
        }
        else if(m_header.cmd_type == MANSCDP_QUERY_CMD_CONFIG_DOWNLOAD)
        {
            ConfigEvent h;
            h.Handle(e);
        }
        else if(m_header.cmd_type == MANSCDP_QUERY_CMD_CRUISETRACKQUERY)
        {
            CruiseTrackEvent h;
            h.Handle(e);
        }
        else if(m_header.cmd_type == MANSCDP_QUERY_CMD_CRUISETRACKLISTQUERY)
        {
            CruiseTrackListEvent h;
            h.Handle(e);
        }
        else if(m_header.cmd_type == MANSCDP_NOTIFY_CMD_BROADCASE)
        {
            BroadcastEvent h;
            h.HandleResponse(e);
        }
        else if(m_header.cmd_type == MANSCDP_CONTROL_CMD_DEVICE_CONTROL)
        {
            DeviceCtrlEvent h;
            h.HandleRespone(e);
        }
        else if(m_header.cmd_type == MANSCDP_QUERY_CMD_HOMEPOSITIONQUERY)
        {
            HomePositionQueryEvent h;
            h.Handle(e);
        }
        else if(m_header.cmd_type == MANSCDP_QUERY_CMD_PTZPOSITION)
        {
            PTZPositionEvent h;
            h.Handle(e);
        }

        break;
    }
    default:
        break;
    }

    return 0;
}

int MessageEvent::HandleResponseSuccess(const SipEvent::Ptr &e)
{
    // int code = GetStatusCodeFromResponse(e->m_pEvent->response);
    auto id = GetMsgIDFromRequest(e->m_pEvent->request);
    /* 请求消息有响应，移除请求记录池 */
    RequestPool::instance()->RemoveRequest(id);

    return 0;
}

int MessageEvent::HandleResponseFailure(const SipEvent::Ptr &e)
{
    int code = GetStatusCodeFromResponse(e->m_pEvent->response);
    auto id = GetMsgIDFromRequest(e->m_pEvent->request);
    /* TODO 响应请求消息失败 */
    RequestPool::instance()->HandleMessageRequest(id, code);
    return 0;
}

int MessageEvent::NotifySnapShotFinish(GB28181::UploadSnapShotFiniInfo_S &stInfo)
{
#if 1
    if (!m_bThrRun.load())
    {
        /* 线程没跑则先开启线程 */
        m_bThrRun.store(true);
        m_pThrSnapShot = new std::thread(std::bind(&MessageEvent::thrSendSnapShot, this));
    }
    /* 存入队列发送*/
    m_queSnapShot.push(stInfo);
#else
    /* 直接发送消息 */
    SendUploadSnapShotFinish(stInfo);
#endif
    return 0;
}

void MessageEvent::thrSendSnapShot()
{
	pthread_setname_np(pthread_self(), "SendSnapShot");

    dlog_info("开始处理上抛图像传输完成通知");
    while (m_bThrRun.load())
    {
        GB28181::UploadSnapShotFiniInfo_S stInfo;
        if (!m_queSnapShot.peek(stInfo))
        {
            continue;
        }

        if (SendUploadSnapShotFinish(stInfo) < 0)
        {
            /* 发送失败则等待一定时间后再发送 */
            std::this_thread::sleep_for(std::chrono::milliseconds(UPLOADSNAPSHOT_FINISH_RESEND_TIME));
        }
        else
        {
            /*删除m_mapSnapShotFini中的记录*/
            m_mapSnapShotFini.erase(stInfo.strSessionID);
            dlog_info("删除记录 SessionID:%s", stInfo.strSessionID);
            /* 发送成功则删除队列中的数据 */
            m_queSnapShot.pop(stInfo);
        }

        //检测m_mapSnapShotFini是否有超时未处理的事件
        if(m_mapSnapShotFini.size() > 0)
        {
            for(auto it = m_mapSnapShotFini.begin(); it != m_mapSnapShotFini.end();)
            {
                 /* 获取当前时间（包括毫秒） */
                struct timeval tv;
                gettimeofday(&tv, nullptr);
                if ((tv.tv_sec - it->second.nTime) > 120){
                    it = m_mapSnapShotFini.erase(it);  // 返回下一个有效迭代器
                }
                else {
                    ++it;
                }
            }
        }
    }
    dlog_info("结束处理上抛图像传输完成通知");
}

int MessageEvent::SendUploadSnapShotFinish(GB28181::UploadSnapShotFiniInfo_S &Info)
{
    /* 拼接返回报文 */
    pugi::xml_document stNewDoc;

    auto declaration = stNewDoc.append_child(pugi::node_declaration);
    auto attrVersion = declaration.append_attribute("version");
    attrVersion.set_value("1.0");
    auto attrEncoding = declaration.append_attribute("encoding");
    attrEncoding.set_value("GB18030");

    auto rootRet = stNewDoc.append_child("Response");

    auto nodeCmdType = rootRet.append_child("CmdType");
    nodeCmdType.text().set("UploadSnapShotFinished");

    auto nodeSN = rootRet.append_child("SN");
    nodeSN.text().set(Info.nSN);

    auto nodeDeviceID = rootRet.append_child("DeviceID");
    nodeDeviceID.text().set(Info.strID);

    auto nodeSessionID = rootRet.append_child("SessionID");
    nodeSessionID.text().set(Info.strSessionID);

    auto nodeSnapShotList = rootRet.append_child("SnapShotList");

    int Ret = 0;

    auto it = m_mapSnapShotFini.find(Info.strSessionID);

    if(Info.vecSnapShotList.size() > 0)
    {
        for (auto Item : Info.vecSnapShotList)
        {
            nodeSnapShotList.child("SnapShotFileID").text().set(Item.strSnapShotFileID);
            std::ostringstream os;
            stNewDoc.save(os);
            auto strGB18030 = ::ToMbcsString(os.str());
            Ret = SendMessageWithCallID(it->second.e, strGB18030);
        }
    }
    else
    {
        std::ostringstream os;
        stNewDoc.save(os);
        auto strGB18030 = ::ToMbcsString(os.str());
        Ret = SendMessageWithCallID(it->second.e, strGB18030);
    }

    return Ret;
}

bool HeartbeatEvent::Handle(const SipEvent::Ptr &e)
{
    ParseHeader(e);
    auto device = DeviceManage::instance()->GetDevice(m_header.strDevID);
    if (device && device->IsRegistered())
    {
        device->UpdateLastTime();
        device->SetStatus(1);
        SendResponse(e, SIP_OK);
        return true;
    }

    /* 需要让设备重新注册 */
    dlog_warn("Heartbeat SIP_UNAUTHORIZED");
    /* 返回Message的401不会让设备主动重新注册 */
    SendResponse(e, SIP_UNAUTHORIZED);
    /* FIXME 需要构建一个注册方法的401 */
    // RegisterEvent reg;
    // reg._response_register_401unauthorized(e);
    return false;
}

bool CatalogEvent::Handle(const SipEvent::Ptr &e)
{
    dlog_warn("================设备目录查询响应====================");
    ParseHeader(e);
    auto root = m_doc.first_child();

    std::string device_id;
    if(!root.child("DeviceID").empty())
    {
        device_id = root.child("DeviceID").text().as_string();
    }
    auto device = DeviceManage::instance()->GetDevice(device_id);
    if (device == nullptr)
    {
        SendResponse(e, SIP_BAD_REQUEST);
        return false;
    }

    if (nullptr == e->m_pNetBase)
    {
        SendResponse(e, SIP_BAD_REQUEST);
        dlog_warn("事件回调网络实例为空");
        return false;
    }

    auto node = root.child("DeviceList");
    if (node.empty())
    {
        return false;
    }

    auto children = node.children("Item");
    for (auto &&child : children)
    {
        std::string channel_id;
        if(!child.child("DeviceID").empty())
        {
            channel_id = child.child("DeviceID").text().as_string();
        }

        /* 只创建视频通道 */

        std::shared_ptr<Channel> channel = nullptr;
        channel = device->GetChannel(channel_id);
        if (channel == nullptr)
        {
            /* 创建服务器的通道类型 */
            channel = std::make_shared<Channel>(e->m_pNetBase->GetCbInfo(), true);
            device->InsertChannel(device_id, channel_id, channel);
        }
        channel->SetDefaultSSRC(SSRCConfig::instance()->GenerateSSRC());
        if(!child.child("DeviceID").empty())
        {
            channel->SetChannelID(child.child("DeviceID").text().as_string());
        }
        if(!child.child("Name").empty())
        {
            channel->SetName(child.child("Name").text().as_string());
        }
        if(!child.child("Manufacturer").empty())
        {
            channel->SetManufacturer(child.child("Manufacturer").text().as_string());
        }
        if(!child.child("Model").empty())
        {
            channel->SetModel(child.child("Model").text().as_string());
        }
        if(!child.child("Owner").empty())
        {
            channel->SetOwner(child.child("Owner").text().as_string());
        }
        if(!child.child("CivilCode").empty())
        {
            channel->SetCivilCode(child.child("CivilCode").text().as_string());
        }
        if(!child.child("Address").empty())
        {
            channel->SetAddress(child.child("Address").text().as_string());
        }
        if(!child.child("Parental").empty())
        {
            channel->SetParental(child.child("Parental").text().as_string());
        }
        if(!child.child("RegisterWay").empty())
        {
            channel->SetRegisterWay(child.child("RegisterWay").text().as_string());
        }
        if(!child.child("Secrecy").empty())
        {
            channel->SetSecrecy(child.child("Secrecy").text().as_string());
        }
        if(!child.child("StreamNum").empty())
        {
            channel->SetStreamNum(child.child("StreamNum").text().as_string());
        }
        /* 通道IP默认跟随父设备 */
        // channel->SetIpAddress(child.child("IPAddress").text().as_string());
        /* 部分父设备的ID与通道的ID不一致，统一采用一级的DeviceID */
        // channel->SetParentID(child.child("ParentID").text().as_string());
        channel->SetParentID(device_id);
        if(!child.child("Status").empty())
        {
            channel->SetStatus(child.child("Status").text().as_string());
        }
        channel->SetSipStatus(SIP_ONLINE);

        auto info = child.child("Info");
        if (info && info.child("PTZType"))
        {
            channel->SetPtzType(info.child("PTZType").text().as_string());
        }
        if (info && info.child("DownloadSpeed"))
        {
            channel->SetDownloadSpeed(info.child("DownloadSpeed").text().as_string());
        }
        channel->UploadInfo();
        /* 接收到通道信息才能上抛数据 */
        dlog_info("接收到设备[%s]的通道信息[%s]，上抛设备数据",
                  device->GetDeviceID().c_str(),
                  channel_id);
    }

    SendResponse(e, SIP_OK);
    return true;
}

bool DeviceInfoEvent::Handle(const SipEvent::Ptr &e)
{
    ParseHeader(e);
    auto root = m_doc.first_child();

    std::string device_id; 
    if(!root.child("DeviceID").empty())
    {
        device_id = root.child("DeviceID").text().as_string();
    }
    auto device = DeviceManage::instance()->GetDevice(device_id);
    if (device)
    {
        if(!root.child("DeviceName").empty())
        {
            device->SetName(root.child("DeviceName").text().as_string());
        }
        if(!root.child("Manufacturer").empty())
        {
            device->SetManufacturer(root.child("Manufacturer").text().as_string());
        }
        if(!root.child("Model").empty())
        {
            device->SetModel(root.child("Model").text().as_string());
        }
       
        // dlog_info("%s", device->toString().c_str());
        // DbManager::instance()->AddOrUpdateDevice(device, true);
        SendResponse(e, SIP_OK);
        return true;
    }
    return false;
}

bool PresetQueryEvent::Handle(const SipEvent::Ptr &e)
{
#if 0 /* TODO 需要配合请求池 */
    ParseHeader(e);
    auto root = m_doc.first_child();
    auto sn = root.child("SN").text().as_string();
    auto device_id = root.child("DeviceID").text().as_string();

    auto req = RequestPool::instance()->GetMessageRequestBySN(sn, REQUEST_MESSAGE_TYPE::DEVICE_QUERY_PRESET);
    if (req == nullptr)
    {
        dlog_error("PresetQueryEvent can not find request by sn: [%s]", sn);
        SendResponse(e, SIP_INTERNAL_SERVER_ERROR);
        return false;
    }

    auto node = root.child("PresetList");
    auto num = node.attribute("Num").as_int();
    dlog_info("DeviceID: [%s] Preset:[%d]", device_id, num);

    auto request = std::dynamic_pointer_cast<PresetRequest>(req);
    auto children = node.children("Item");
    for (auto &&child : children)
    {
        auto id = child.child("PresetID").text().as_string();
        auto name = child.child("PresetName").text().as_string();

        request->InsertPreset(id, name);
    }
    request->OnRequestFinished();

    SendResponse(e, SIP_OK);
#endif
    
    ParseHeader(e);

    /* 先响应200，说明已接收到请求 */
    SendResponse(e, SIP_OK);

    GB28181::PresetQueryInfo_S Preset;

    auto root = m_doc.first_child();
    /*序列号*/
    Preset.nSN = root.child("SN").text().as_int();
    /* 设备Id */
    std::string strID; 
    if(!root.child("DeviceID").empty())
    {
        strID = root.child("DeviceID").text().as_string();
    }
    Preset.strID = strID;

    /*通道标号*/
    Preset.nIndex = -1;  //通道下标，下级默认-1
    /*true--设备配置设置，false--设备配置获取*/
    Preset.bIsSet = false;

    Preset.nSumNum = root.child("SumNum").text().as_int();/*以实际收到的数量为准，还是以报文的数量为准？*/

    /*预置位列表*/
    auto node = root.child("PresetList");
    auto num = node.attribute("Num").as_int();
    dlog_info("DeviceID: [%s] Preset:[%d]", Preset.strID, num);

    /*预置位记录项*/
    auto children = node.children("Item");
    for (auto &&child : children)
    {
        GB28181::PresetListItem  Item;
        /*预置位编码*/
        if(!child.child("PresetID").empty())
        {
            Item.strPresetID = child.child("PresetID").text().as_string();
        }
        /*预置位名称*/
        if(!child.child("PresetName").empty())
        {
            Item.strPresetName = child.child("PresetName").text().as_string();
        }
        Preset.vecPresetList.push_back(Item);
    }

    /*TODO 巡航轨迹回调上抛*/
    { /* 配合上层实现业务功能 */
        SipCbResult_S stResult;
        auto fnCb = ::SipModule::instance()->GetPresetQueryCb();
        if (fnCb)
        {
            fnCb(Preset, stResult);
        }
    }
    return true;
}

bool AlarmEvent::Handle(const SipEvent::Ptr &e)
{
    ParseHeader(e);
    /* 上抛报警信息即可，后续作为中间层，则需要考虑转发 */
    GB28181::AlarmCbData_S stInfo;
    auto root = m_doc.first_child();
    auto info = root.child("Info");

    /* 报警序号 */
    stInfo.nSN = root.child("SN").text().as_int();
    /* 设备Id */
    std::string strID; 
    if(!root.child("DeviceID").empty())
    {
        strID = root.child("DeviceID").text().as_string();
    }
    stInfo.strID = strID;
    /* 报警等级 */
    stInfo.enPriority = static_cast<GB28181::AlarmPriority_E>(root.child("AlarmPriority").text().as_int());
    /* 报警方法 */
    stInfo.enMethod = static_cast<GB28181::AlarmMethod_E>(root.child("AlarmMethod").text().as_int());
    /* 报警时间 */
    if(!root.child("AlarmTime").empty())
    {
        stInfo.strTime = root.child("AlarmTime").text().as_string();
    }
    stInfo.nTime = ::ISO8601ToTimeT(stInfo.strTime);
    /* 报警类型 */
    stInfo.enType = info.child("AlarmType").text().as_int();

    /* 额外可选字段 */
    auto typeParam = info.child("AlarmTypeParam");
    stInfo.enTypeParam = typeParam.child("EventType").text().as_int();

    /* 通过设备中的报警回调上抛报警信息 */
    auto device = DeviceManage::instance()->GetDevice(stInfo.strID);
    if (device)
    {
        device->AlarmCallback(stInfo);
    }
    /* 级联时上层进行转发操作 */
    return true;
}

bool DeviceCtrlEvent::HandleGuardCmd(const SipEvent::Ptr &e)
{
    ParseHeader(e);
    auto root = m_doc.first_child();
    /*序列号*/
    uint64_t nSN = root.child("SN").text().as_int();
    /* 设备Id */
    std::string strID; 
    if(!root.child("DeviceID").empty())
    {
        strID = root.child("DeviceID").text().as_string();
    }
    /*布防类型*/
    std::string strGuardType = root.child("GuardCmd").text().as_string();

    bool bGuard = false;
    if (strGuardType.compare("SetGuard") == 0)
    {
        bGuard = true;
    }
    else if (strGuardType.compare("ResetGuard") == 0)
    {
        bGuard = false;
    }
    /* 更新布防设置 */
    auto pClient = dynamic_cast<SipClient *>(e->m_pNetBase);
    if (pClient)
    {
        pClient->SetGuard(bGuard);
    }

    /* 拼接返回报文 */
    pugi::xml_document stNewDoc;

    auto declaration = stNewDoc.append_child(pugi::node_declaration);
    auto attrVersion = declaration.append_attribute("version");
    attrVersion.set_value("1.0");
    auto attrEncoding = declaration.append_attribute("encoding");
    attrEncoding.set_value("GB18030");

    auto rootRet = stNewDoc.append_child("Response");

    auto nodeCmdType = rootRet.append_child("CmdType");
    nodeCmdType.text().set(m_header.strCmdType);

    auto nodeSN = rootRet.append_child("SN");
    nodeSN.text().set(nSN);

    auto nodeDeviceID = rootRet.append_child("DeviceID");
    nodeDeviceID.text().set(strID);

    auto nodeSumNum = rootRet.append_child("Result");
    nodeSumNum.text().set(std::to_string(0));

    std::ostringstream os;
    stNewDoc.save(os);
    auto strGB18030 = ::ToMbcsString(os.str());
    SendMessageWithCallID(e, strGB18030);

    return true;
}

bool DeviceCtrlEvent::HandleTeleBoot(const SipEvent::Ptr &e)
{
    ParseHeader(e);
    auto root = m_doc.first_child();
    /*序列号*/
    uint64_t nSN = root.child("SN").text().as_int();
    /* 设备Id */
    std::string strID; 
    if(!root.child("DeviceID").empty())
    {
        strID = root.child("DeviceID").text().as_string();
    }
    /*远程启动类型*/
    std::string strTeleType = root.child("TeleBoot").text().as_string();
    std::string strResult;
     /*TODO 设备远程启动上抛*/
    { /* 配合上层实现业务功能 */
        SipCbResult_S stResult;
        auto fnCb = ::SipModule::instance()->GetNVRTeleBootCb();
        if (fnCb)
        {
            fnCb(strTeleType, stResult);
        }

        if(stResult.nResult != 0)
        {
            dlog_warn("回调操作执行失败[%d]", stResult.nResult);
            strResult = "ERROR";
        }
        else
        {
            strResult = "OK";
        }
    }
    //拼接返回报文
    ResponeControlResult(e, nSN, m_header.strCmdType, strID, strResult);
    return true;
}

bool DeviceCtrlEvent::HandleIFrameCmd(const SipEvent::Ptr &e)
{
    ParseHeader(e);
    GB28181::IFrame_S tIFrame;
    auto root = m_doc.first_child();
    /*序列号*/
    uint64_t nSN = root.child("SN").text().as_int();
    /* 设备Id */
    std::string strID; 
    if(!root.child("DeviceID").empty())
    {
        strID = root.child("DeviceID").text().as_string();
    }
    tIFrame.nSN = nSN;
    tIFrame.strID = strID;
    auto pClient = dynamic_cast<SipClient *>(e->m_pNetBase);
    if (pClient == nullptr)
    {
        dlog_warn("客户端实例为空，无法获取客户端的通道信息");
        return false;
    }
    auto pChn = pClient->GetChannelByID(strID);
    if (nullptr == pChn)
    {
        dlog_warn("没有找到通道[%s]", m_header.strDevID.c_str());
        return false;
    }

    tIFrame.nIndex = pChn->nIndex;

    std::string IFrameCmd;
    if(!root.child("IFrameCmd").empty())
    {
        IFrameCmd = root.child("IFrameCmd").text().as_string();
    }
    tIFrame.IFrameCmd = IFrameCmd;
    std::string strResult;
    /*TODO 回调上抛*/
    { /* 配合上层实现业务功能 */
        SipCbResult_S stResult;
        auto fnCb = ::SipModule::instance()->GetIFrameCmdCb();
        if (fnCb)
        {
            fnCb(tIFrame, stResult);
        }

        if(stResult.nResult != 0)
        {
            dlog_warn("回调操作执行失败[%d]", stResult.nResult);
            strResult = "ERROR";
        }
        else
        {
            strResult = "OK";
        }

    }
     //拼接返回报文
    ResponeControlResult(e, nSN, m_header.strCmdType, strID, strResult);
    return true;
}

bool DeviceCtrlEvent::HandleDragZoomIn(const SipEvent::Ptr &e)
{
    ParseHeader(e);

    GB28181::DragZoomInfo_S tDragZoomInfo;

    auto root = m_doc.first_child();
    /*序列号*/
    uint64_t nSN = root.child("SN").text().as_int();
    /* 设备Id */
    std::string strID; 
    if(!root.child("DeviceID").empty())
    {
        strID = root.child("DeviceID").text().as_string();
    }    
    tDragZoomInfo.strID = strID;
    /*拉框放大*/
    tDragZoomInfo.bIsZooIn = true;

    auto pClient = dynamic_cast<SipClient *>(e->m_pNetBase);
    if (pClient == nullptr)
    {
        dlog_warn("客户端实例为空，无法获取客户端的通道信息");
        return false;
    }
    auto pChn = pClient->GetChannelByID(strID);
    if (nullptr == pChn)
    {
        dlog_warn("没有找到通道[%s]", m_header.strDevID.c_str());
        return false;
    }

    auto nodeDragZoomIn = root.child("DragZoomIn");

    tDragZoomInfo.nLength = nodeDragZoomIn.child("Length").text().as_int();
    tDragZoomInfo.nWidth = nodeDragZoomIn.child("Width").text().as_int();
    tDragZoomInfo.nMidPointX = nodeDragZoomIn.child("MidPointX").text().as_int();
    tDragZoomInfo.nMidPointY = nodeDragZoomIn.child("MidPointY").text().as_int();
    tDragZoomInfo.nLengthX = nodeDragZoomIn.child("LengthX").text().as_int();
    tDragZoomInfo.nLengthY = nodeDragZoomIn.child("LengthY").text().as_int();

    /*TODO 回调上抛*/
    { /* 配合上层实现业务功能 */
        SipCbResult_S stResult;
        auto fnCb = ::SipModule::instance()->GetDragZoomInOutCb();
        if (fnCb)
        {
            fnCb(tDragZoomInfo, stResult);
        }

        if(stResult.nResult != 0)
        {
            dlog_warn("回调操作执行失败[%d]", stResult.nResult);
        }
    }

    return true;
}

bool DeviceCtrlEvent::HandleDragZoomOut(const SipEvent::Ptr &e)
{
    ParseHeader(e);

    GB28181::DragZoomInfo_S tDragZoomInfo;

    auto root = m_doc.first_child();
    /*序列号*/
    uint64_t nSN = root.child("SN").text().as_int();
    /* 设备Id */
    std::string strID; 
    if(!root.child("DeviceID").empty())
    {
        strID = root.child("DeviceID").text().as_string();
    }    
    tDragZoomInfo.strID = strID;
    /*拉框缩小*/
    tDragZoomInfo.bIsZooIn = false; 

    auto pClient = dynamic_cast<SipClient *>(e->m_pNetBase);
    if (pClient == nullptr)
    {
        dlog_warn("客户端实例为空，无法获取客户端的通道信息");
        return false;
    }
    auto pChn = pClient->GetChannelByID(strID);
    if (nullptr == pChn)
    {
        dlog_warn("没有找到通道[%s]", m_header.strDevID.c_str());
        return false;
    }

    auto nodeDragZoomIn = root.child("DragZoomOut");

    tDragZoomInfo.nLength = nodeDragZoomIn.child("Length").text().as_int();
    tDragZoomInfo.nWidth = nodeDragZoomIn.child("Width").text().as_int();
    tDragZoomInfo.nMidPointX = nodeDragZoomIn.child("MidPointX").text().as_int();
    tDragZoomInfo.nMidPointY = nodeDragZoomIn.child("MidPointY").text().as_int();
    tDragZoomInfo.nLengthX = nodeDragZoomIn.child("LengthX").text().as_int();
    tDragZoomInfo.nLengthY = nodeDragZoomIn.child("LengthY").text().as_int();

    /*TODO 回调上抛*/
    { /* 配合上层实现业务功能 */
        SipCbResult_S stResult;
        auto fnCb = ::SipModule::instance()->GetDragZoomInOutCb();
        if (fnCb)
        {
            fnCb(tDragZoomInfo, stResult);
        }

        if(stResult.nResult != 0)
        {
            dlog_warn("回调操作执行失败[%d]", stResult.nResult);
        }
    }

    return true;
}

bool DeviceCtrlEvent::HandlePTZ(const SipEvent::Ptr &e)
{
    ParseHeader(e);
    auto root = m_doc.first_child();
    std::string strPtzCmd;
    if(!root.child("PTZCmd").empty())
    {
        strPtzCmd = root.child("PTZCmd").text().as_string();
    }
    control_cmd_t stCmd;
    PtzParser ptzParser;
    ptzParser.ParseControlCmd(stCmd, strPtzCmd);
    dlog_info("设备控制PtzCmd[%d]", stCmd.ctrltype);
    SipPtzInfoCb_S stPtzCb;
    SipPresetInfoCb_S stPresetCb;
    SipCbResult_S stResult;
    /* 默认返回成功 */
    int nRet = SIP_OK;
    /* 先找通道 */
    auto pClient = dynamic_cast<SipClient *>(e->m_pNetBase);
    if (nullptr == pClient)
    {
        dlog_error("无法转换为客户端实例")
        nRet = SIP_INTERNAL_SERVER_ERROR;
        goto EXIT;
    }
    if (CTRL_CMD_UNKNOWN == stCmd.ctrltype)
    {
        nRet = SIP_INTERNAL_SERVER_ERROR;
        goto EXIT;
    }

    /* 转换为对外的PtzCmd类型进行上抛 */
    if (PRESET_TYPE == stCmd.ctrltype)
    {
        stPresetCb.nIndex = pClient->GetIndexByChnID(m_header.strDevID);
        stPresetCb.nPresetId = stCmd.preset.index;
        switch (stCmd.preset.cmdtype)
        {
        case preset_cmd_t::PRESET_SET:
            stPresetCb.enType = SipPresetType_E::SET;
            break;
        case preset_cmd_t::PRESET_CALL:
            stPresetCb.enType = SipPresetType_E::CALL;
            break;
        case preset_cmd_t::PRESET_DELE:
            stPresetCb.enType = SipPresetType_E::DEL;
            break;
        default:
            stPresetCb.enType = SipPresetType_E::UNKNOWN;
            break;
        }
        /* 回调上抛 */
        if (pClient->GetCbInfo().fnPreset)
        {
            pClient->GetCbInfo().fnPreset(stPresetCb, stResult);
        }
    }
    else
    {
        stPtzCb.nIndex = pClient->GetIndexByChnID(m_header.strDevID);
        switch (stCmd.ctrltype)
        {
        case PTZ_TYPE:
        {
            /* TODO 存在组合指令的问题——复合指令暂时以左右不为0的速度为主 */
            if (stCmd.ptz_pan.cmdtype == ptz_cmd_pan_t::PAN_LEFT)
            {
                stPtzCb.enType = SipPtzType_E::LEFT;
                stPtzCb.nSpeed = stCmd.ptz_pan.speed;
            }
            else if (stCmd.ptz_pan.cmdtype == ptz_cmd_pan_t::PAN_RIGHT)
            {
                stPtzCb.enType = SipPtzType_E::RIGHT;
                stPtzCb.nSpeed = stCmd.ptz_pan.speed;
            }

            /* 存在复合操作，需要结合左右进行判断 */
            if (stCmd.ptz_tilt.cmdtype == ptz_cmd_tilt_t::TILT_UP)
            {
                if (stCmd.ptz_pan.cmdtype == ptz_cmd_pan_t::PAN_LEFT)
                {
                    stPtzCb.enType = SipPtzType_E::TOP_LEFT;
                }
                else if (stCmd.ptz_pan.cmdtype == ptz_cmd_pan_t::PAN_RIGHT)
                {
                    stPtzCb.enType = SipPtzType_E::TOP_RIGHT;
                }
                else
                {
                    stPtzCb.enType = SipPtzType_E::UP;
                }

                if (stPtzCb.nSpeed == 0)
                {
                    stPtzCb.nSpeed = stCmd.ptz_tilt.speed;
                }
            }
            else if (stCmd.ptz_tilt.cmdtype == ptz_cmd_tilt_t::TILT_DOWN)
            {
                if (stCmd.ptz_pan.cmdtype == ptz_cmd_pan_t::PAN_LEFT)
                {
                    stPtzCb.enType = SipPtzType_E::LOWER_LEFT;
                }
                else if (stCmd.ptz_pan.cmdtype == ptz_cmd_pan_t::PAN_RIGHT)
                {
                    stPtzCb.enType = SipPtzType_E::LOWER_RIGHT;
                }
                else
                {
                    stPtzCb.enType = SipPtzType_E::DOWN;
                }
                if (stPtzCb.nSpeed == 0)
                {
                    stPtzCb.nSpeed = stCmd.ptz_tilt.speed;
                }
            }

            if (stCmd.ptz_zoom.cmdtype == ptz_cmd_zoom_t::ZOOM_IN)
            {
                stPtzCb.enType = SipPtzType_E::ZOOM_UP;
                stPtzCb.nSpeed = stCmd.ptz_zoom.speed;
            }
            else if (stCmd.ptz_zoom.cmdtype == ptz_cmd_zoom_t::ZOOM_OUT)
            {
                stPtzCb.enType = SipPtzType_E::ZOOM_DOWN;
                stPtzCb.nSpeed = stCmd.ptz_zoom.speed;
            }
            break;
        }
        case FI_TYPE:
        {
            if (stCmd.fi_focus.cmdtype == fi_cmd_focus_t::FOCUS_FAR)
            {
                /* 聚焦远 */
                stPtzCb.enType = SipPtzType_E::FOCUS_DOWN;
                stPtzCb.nSpeed = stCmd.fi_focus.speed;
            }
            else if (stCmd.fi_focus.cmdtype == fi_cmd_focus_t::FOCUS_NEAR)
            {
                /* 聚焦近 */
                stPtzCb.enType = SipPtzType_E::FOCUS_UP;
                stPtzCb.nSpeed = stCmd.fi_focus.speed;
            }
            else if (stCmd.fi_iris.cmdtype == fi_cmd_iris_t::IFIS_AMPLIFICATION)
            {
                /* 光圈放大 */
                stPtzCb.enType = SipPtzType_E::APERTUR_UP;
                stPtzCb.nSpeed = stCmd.fi_iris.speed;
            }
            else if (stCmd.fi_iris.cmdtype == fi_cmd_iris_t::IFIS_SHRINK)
            {
                /* 光圈缩小 */
                stPtzCb.enType = SipPtzType_E::APERTUR_DOWN;
                stPtzCb.nSpeed = stCmd.fi_iris.speed;
            }
            break;
        }
        case PATROL_TYPE:
        {

            break;
        }
        case SCAN_TYPE:
        {

            break;
        }
        case AUX_TYPE:
        {
            break;
        }
        case CONTROL_STOP:
        {
            stPtzCb.enType = SipPtzType_E::STOP;
            break;
        }
        default:
            break;
        }

        /* 回调上抛 */
        if (pClient->GetCbInfo().fnPtzCmd)
        {
            pClient->GetCbInfo().fnPtzCmd(stPtzCb, stResult);
        }
    }

EXIT:
    SendResponse(e, nRet);
    return true;
}

bool DeviceCtrlEvent::Handle(const SipEvent::Ptr &e)
{
    ParseHeader(e);
    /* 先响应200，说明已接收到请求 */
    SendResponse(e, SIP_OK);

    switch (m_header.cmd_type)
    {
    case MANSCDP_CONTROL_CMD_DEVICE_CONTROL:
        {
            return HandleDeviceControl(e);
        }
    case MANSCDP_CONTROL_CMD_DEVICE_CONFIG:
        {
            return HandleDeviceConfig(e);
        }
    default:
        break;
    }

    return true;
}

bool DeviceCtrlEvent::HandleRespone(const SipEvent::Ptr &e)
{
    ParseHeader(e);
    /* 先响应200，说明已接收到请求 */
    SendResponse(e, SIP_OK);

    switch (m_header.cmd_type)
    {
    case MANSCDP_CONTROL_CMD_DEVICE_CONTROL:
        {
            return HandleControlRespone(e);
        }
    case MANSCDP_CONTROL_CMD_DEVICE_CONFIG:
        {
            return HandleConfigRespone(e);
        }
    default:
        break;
    }

    return true;
}

bool DeviceCtrlEvent::HandleControlRespone(const SipEvent::Ptr &e)
{
    ParseHeader(e);

    GB28181::ControlResults_S tResults;

    auto root = m_doc.first_child();
    /*序列号*/
    tResults.nSN = root.child("SN").text().as_int();
    /* 设备Id */
    std::string strID; 
    if(!root.child("DeviceID").empty())
    {
        strID = root.child("DeviceID").text().as_string();
    }    
    tResults.strID = strID;
    /*查询结果*/
    tResults.strResult = root.child("Result").text().as_string();
   /*1-设备控制类型应答*/
    tResults.nCmdType = 1;

    /*TODO 基本参数配置回调上抛*/
    { /* 配合上层实现业务功能 */
        SipCbResult_S stResult;
        auto fnCb = ::SipModule::instance()->GetControlResultsCb();
        if (fnCb)
        {
            fnCb(tResults, stResult);
        }

        if(stResult.nResult != 0)
        {
            dlog_warn("回调操作执行失败[%d]", stResult.nResult);
        }
    }

    return true;
}

bool DeviceCtrlEvent::HandleConfigRespone(const SipEvent::Ptr &e)
{
    ParseHeader(e);

    GB28181::ControlResults_S tResults;

    auto root = m_doc.first_child();
    /*序列号*/
    tResults.nSN = root.child("SN").text().as_int();
    /*设备Id*/
    std::string strID;
    if(!root.child("DeviceID").empty())
    {
        strID = root.child("DeviceID").text().as_string();
    }
    tResults.strID = strID;
    /*查询结果*/
    if(!root.child("Result").empty())
    {
        tResults.strResult = root.child("Result").text().as_string();
    }
   /*2-设备配置控制类型应答*/
    tResults.nCmdType = 2;

    /*TODO 基本参数配置回调上抛*/
    { /* 配合上层实现业务功能 */
        SipCbResult_S stResult;
        auto fnCb = ::SipModule::instance()->GetControlResultsCb();
        if (fnCb)
        {
            fnCb(tResults, stResult);
        }

        if(stResult.nResult != 0)
        {
            dlog_warn("回调操作执行失败[%d]", stResult.nResult);
        }
    }

    return true;
}

bool DeviceCtrlEvent::HandleDeviceControl(const SipEvent::Ptr &e)
{
    ParseHeader(e);

    auto root = m_doc.first_child();
    /* 根据控制指令的细分进行判断 */
    if (!root.child("GuardCmd").empty())
    {
        /* 布防/撤防 */
        return HandleGuardCmd(e);
    }
    else if(!root.child("PTZCmd").empty())
    {
        /* 云台基础控制/预置位 */
        return HandlePTZ(e);
    }
    else if(!root.child("TeleBoot").empty())
    {
        /*设备远程启动*/
        return HandleTeleBoot(e);
    }
    else if(!root.child("IFrameCmd").empty())
    {
        /*强制关键帧*/
        return HandleIFrameCmd(e);
    }
    else if(!root.child("DragZoomIn").empty())
    {
        /*拉框放大*/
        return HandleDragZoomIn(e);
    }
    else if(!root.child("DragZoomOut").empty())
    {
        /*拉框缩小*/
        return HandleDragZoomOut(e);
    }
    else if(!root.child("PTZPreciseCtrl").empty())
    {
        /*PTZ精准控制*/
        return HandleDevicePTZCtrl(e);
    }
    else if(!root.child("RecordCmd").empty())
    {
        /*录像控制命令*/
        return HandleRecordCmd(e);
    }
    else if(!root.child("AlarmCmd").empty())
    {
        /*报警复位控制命令*/
        return HandleAlarmCmd(e);
    }
    else if(!root.child("HomePosition").empty())
    {
        /*看守位控制命令*/
        return HandleHomePosition(e);
    }
    else if(!root.child("DeviceUpgrade").empty())
    {
        /*设备软件升级控制命令*/
        return HandleDeviceUpgrade(e);
    }
    else if(!root.child("TargetTrack").empty())
    {
        /*目标跟踪*/
        return HandleTargetTrack(e);
    }

    return true;
}

bool DeviceCtrlEvent::HandleDeviceConfig(const SipEvent::Ptr &e)
{
    ParseHeader(e);

    auto root = m_doc.first_child();

    /* 根据控制指令的细分进行判断 */
    if (!root.child("BasicParam").empty())
    {
        /* 设备配置-基本参数配置 */
        return HandleBasicControl(e);
    }
    else if(!root.child("VideoParamOpt").empty())
    {
        /* 设备配置-视频参数范围配置类型*/
        return HandleVideoOptControl(e);
    }
    else if(!root.child("SVACEncodeConfig").empty())
    {
        /* 设备配置-SVAC编码配置*/
        return HandleSVACEncodeControl(e);
    }
    else if(!root.child("SVACDncodeConfig").empty())
    {
        /* 设备配置-SVAC解码配置*/
        return HandleSVACDncodeControl(e);
    }
    else if(!root.child("VideoParamAttribute").empty())
    {
        /* 设备配置-视频参数属性配置*/
        return HandleVideoAttriControl(e);
    }
    else if(!root.child("VideoRecordPlan").empty())
    {
        /* 设备配置-录像计划配置*/
        return HandleRecordPlanControl(e);
    }
    else if(!root.child("VideoAlarmRecord").empty())
    {
        /* 设备配置-报警录像配置*/
        return HandleAlarmRecordControl(e);
    }
    else if(!root.child("PictureMask").empty())
    {
         /* 设备配置-视频画面遮挡配置*/
        return HandlePictureMaskControl(e);
    }
    else if(!root.child("FrameMirror").empty())
    {
        /* 设备配置-视频画面翻转配置*/
        return  HandleFrameMirrorControl(e);
    }
    else if(!root.child("AlarmReport").empty())
    {
        /* 设备配置-报警上报开关配置*/
        return HandleAlarmReportControl(e);
    }
    else if(!root.child("OSDConfig").empty())
    {
        /* 设备配置-OSD配置*/
        return HandleOSDConfigControl(e);
    }
    else if(!root.child("SnapShotConfig").empty())
    {
        /* 设备配置-图像抓拍配置*/
        return HandleSnapShotControl(e);
    }

    return true;
}

bool DeviceCtrlEvent::HandleDevicePTZCtrl(const SipEvent::Ptr &e)
{
    ParseHeader(e);

    GB28181::PTZPreciseCtrl_S tPTZInfo;

   auto root = m_doc.first_child();
   /*序列号*/
   uint64_t nSN = root.child("SN").text().as_int();
   /* 设备Id */
   std::string strID;
   if(!root.child("DeviceID").empty())
   {
        strID = root.child("DeviceID").text().as_string();
   }
   tPTZInfo.strID = strID;

   auto pClient = dynamic_cast<SipClient *>(e->m_pNetBase);
   if (pClient == nullptr)
   {
       dlog_warn("客户端实例为空，无法获取客户端的通道信息");
       ResponeControlResult(e, nSN, m_header.strCmdType, strID, "ERROR");
   }
   auto pChn = pClient->GetChannelByID(strID);
   if (nullptr == pChn)
   {
       dlog_warn("没有找到通道[%s]", m_header.strDevID.c_str());
       ResponeControlResult(e, nSN, m_header.strCmdType, strID, "ERROR");
   }
    tPTZInfo.nIndex = pChn->nIndex;
   auto nodePTZCtrl = root.child("PTZPreciseCtrl");

   tPTZInfo.dPan = nodePTZCtrl.child("Pan").text().as_double();
   tPTZInfo.dTilt = nodePTZCtrl.child("Tilt").text().as_double();
   tPTZInfo.dZoom = nodePTZCtrl.child("Zoom").text().as_double();

   /*TODO 回调上抛*/
   { /* 配合上层实现业务功能 */
       SipCbResult_S stResult;
       auto fnCb = ::SipModule::instance()->GetPTZPreciseCtrlCb();
       if (fnCb)
       {
           fnCb(tPTZInfo, stResult);
       }

       if(stResult.nResult != 0)
       {
           dlog_warn("回调操作执行失败[%d]", stResult.nResult);
       }
   }

   //拼接返回报文
   ResponeControlResult(e, nSN, m_header.strCmdType, strID, tPTZInfo.strResult);

   return true;
}

bool DeviceCtrlEvent::HandleRecordCmd(const SipEvent::Ptr &e)
{
    ParseHeader(e);

    GB28181::RecordCmd_S tRecord;

   auto root = m_doc.first_child();
   /*序列号*/
   uint64_t nSN = root.child("SN").text().as_int();
   /* 设备Id */
   std::string strID;
   if(!root.child("DeviceID").empty())
   {
        strID = root.child("DeviceID").text().as_string();
   }
   tRecord.strID = strID;

   auto pClient = dynamic_cast<SipClient *>(e->m_pNetBase);
   if (pClient == nullptr)
   {
       ResponeControlResult(e, nSN, m_header.strCmdType, strID, "ERROR");
       dlog_warn("客户端实例为空，无法获取客户端的通道信息");
       return false;
   }
   auto pChn = pClient->GetChannelByID(strID);
   if (nullptr == pChn)
   {
       ResponeControlResult(e, nSN, m_header.strCmdType, strID, "ERROR");
       dlog_warn("没有找到通道[%s]", m_header.strDevID.c_str());
       return false;
   }
   tRecord.nIndex = pChn->nIndex;
   /*码流类型*/
    if(!root.child("StreamNumber").empty())
    {
        tRecord.nStreamNumber = root.child("StreamNumber").text().as_int();
    }
    else
    {
        tRecord.nStreamNumber = 0;
    }

   /*录像控制命令*/
   std::string RecordCmd;
   auto nodeRecordCmd = root.child("RecordCmd");
   if(!nodeRecordCmd.empty())
   {
        RecordCmd = nodeRecordCmd.text().as_string();
   }
   if(RecordCmd.compare("Record") == 0)
   {    
        tRecord.bIsRecord = true;
   }
   else if(RecordCmd.compare("StopRecord") == 0)
   {
        tRecord.bIsRecord = false;
   }
   tRecord.strResult = "";

   /*TODO 回调上抛*/
   { /* 配合上层实现业务功能 */
       SipCbResult_S stResult;
       auto fnCb = ::SipModule::instance()->GetRecordCmdCb();
       if (fnCb)
       {
           fnCb(tRecord, stResult);
       }

       if(stResult.nResult != 0)
       {
           tRecord.strResult  = "ERROR";
           dlog_warn("回调操作执行失败[%d]", stResult.nResult);
       }
   }

   //拼接返回报文
   ResponeControlResult(e, nSN, m_header.strCmdType, strID, tRecord.strResult);

   return true;
}

bool DeviceCtrlEvent::HandleAlarmCmd(const SipEvent::Ptr &e)
{
    ParseHeader(e);

    GB28181::AlarmCmd_S tRAlarm;

   auto root = m_doc.first_child();
   /*序列号*/
   uint64_t nSN = root.child("SN").text().as_int();
   /* 设备Id */
   std::string strID; 
   if(!root.child("DeviceID").empty())
   {
       strID = root.child("DeviceID").text().as_string();
   }
   tRAlarm.strID = strID;

   auto pClient = dynamic_cast<SipClient *>(e->m_pNetBase);
   if (pClient == nullptr)
   {
       ResponeControlResult(e, nSN, m_header.strCmdType, strID, "ERROR");
       dlog_warn("客户端实例为空，无法获取客户端的通道信息");
       return false;
   }
   auto pChn = pClient->GetChannelByID(strID);
   if (nullptr == pChn)
   {
       ResponeControlResult(e, nSN, m_header.strCmdType, strID, "ERROR");
       dlog_warn("没有找到通道[%s]", m_header.strDevID.c_str());
       return false;
   }

   /*报警控制命令*/
   std::string RecordCmd;
   if(!root.child("AlarmCmd").empty())
   {
        RecordCmd = root.child("AlarmCmd").text().as_string();
   }
   auto nodeInfo = root.child("Info");
   if(!nodeInfo.child("AlarmMethod").empty())
   {
        tRAlarm.strAlarmMethod = nodeInfo.child("AlarmMethod").text().as_string();
   }
   if(!nodeInfo.child("AlarmType").empty())
   {
        tRAlarm.strAlarmType = nodeInfo.child("AlarmType").text().as_string();
   }
   tRAlarm.strResult = "";

   /*TODO 回调上抛*/
   { /* 配合上层实现业务功能 */
       SipCbResult_S stResult;
       auto fnCb = ::SipModule::instance()->GetAlarmCmdCb();
       if (fnCb)
       {
           fnCb(tRAlarm, stResult);
       }

       if(stResult.nResult != 0)
       {
           tRAlarm.strResult  = "ERROR";
           dlog_warn("回调操作执行失败[%d]", stResult.nResult);
       }
   }

   //拼接返回报文
   ResponeControlResult(e, nSN, m_header.strCmdType, strID, tRAlarm.strResult);

   return true;
}

bool DeviceCtrlEvent::HandleHomePosition(const SipEvent::Ptr &e)
{
    ParseHeader(e);

    /*设备看守位控制设置*/ 
    GB28181::HomePositionInfo_S tHomePosition;

   auto root = m_doc.first_child();
   /*序列号*/
   uint64_t nSN = root.child("SN").text().as_int();
   /* 设备Id */
   std::string strID; 
   if(!root.child("DeviceID").empty())
   {
       strID = root.child("DeviceID").text().as_string();
   }
   tHomePosition.strID = strID;

   auto pClient = dynamic_cast<SipClient *>(e->m_pNetBase);
   if (pClient == nullptr)
   {
       ResponeControlResult(e, nSN, m_header.strCmdType, strID, "ERROR");
       dlog_warn("客户端实例为空，无法获取客户端的通道信息");
       return false;
   }
   auto pChn = pClient->GetChannelByID(strID);
   if (nullptr == pChn)
   {
       ResponeControlResult(e, nSN, m_header.strCmdType, strID, "ERROR");
       dlog_warn("没有找到通道[%s]", m_header.strDevID.c_str());
       return false;
   }
   /*上级根据通道号赋值，下级设置成-1*/
   tHomePosition.nIndex = pChn->nIndex;
   /*true-设备配置设置，false-设备配置获取*/
   tHomePosition.bIsSet = true;

   auto nodeHomePosition = root.child("HomePosition");

   tHomePosition.nEnabled = nodeHomePosition.child("Enabled").text().as_int();
   tHomePosition.nResetTime = nodeHomePosition.child("ResetTime").text().as_int();
   tHomePosition.nPresetIndex = nodeHomePosition.child("PresetIndex").text().as_int();


   /*TODO 回调上抛*/
   { /* 配合上层实现业务功能 */
       SipCbResult_S stResult;
       auto fnCb = ::SipModule::instance()->GetHomePositionCb();
       if (fnCb)
       {
           fnCb(tHomePosition, stResult);
       }

       if(stResult.nResult != 0)
       {
          tHomePosition.strResult = "ERROR";
           dlog_warn("回调操作执行失败[%d]", stResult.nResult);
       }
   }

   //拼接返回报文
   ResponeControlResult(e, nSN, m_header.strCmdType, strID, tHomePosition.strResult);

   return true;
}

bool DeviceCtrlEvent::HandleDeviceUpgrade(const SipEvent::Ptr &e)
{
    ParseHeader(e);

    /*设备软件升级控制设置*/ 
    GB28181::DeviceUpgradeInfo_S tUpgrade;

   auto root = m_doc.first_child();
   /*序列号*/
   uint64_t nSN = root.child("SN").text().as_int();
   /* 设备Id */
   std::string strID; 
   if(!root.child("DeviceID").empty())
   {
       strID = root.child("DeviceID").text().as_string();
   }
   tUpgrade.strID = strID;

   auto pClient = dynamic_cast<SipClient *>(e->m_pNetBase);
   if (pClient == nullptr)
   {
       ResponeControlResult(e, nSN, m_header.strCmdType, strID, "ERROR");
       dlog_warn("客户端实例为空，无法获取客户端的通道信息");
       return false;
   }

#if  0   /*找NVR*/
   auto pChn = pClient->GetChannelByID(strID);
   if (nullptr == pChn)
   {
       ResponeControlResult(e, nSN, m_header.strCmdType, strID, "ERROR");
       dlog_warn("没有找到通道[%s]", m_header.strDevID.c_str());
       return false;
   }
#endif


   auto nodeDeviceUpgrade = root.child("DeviceUpgrade");

   if(!nodeDeviceUpgrade.child("Firmware").empty())
   {
        tUpgrade.strFirmware = nodeDeviceUpgrade.child("Firmware").text().as_string();
   }
   if(!nodeDeviceUpgrade.child("FileURL").empty())
   {
        tUpgrade.strFileURL = nodeDeviceUpgrade.child("FileURL").text().as_string();
   }
   if(!nodeDeviceUpgrade.child("Manufacturer").empty())
   {
        tUpgrade.strManufacturer = nodeDeviceUpgrade.child("Manufacturer").text().as_string();
   }
   if(nodeDeviceUpgrade.child("SessionID").empty())
   {
        tUpgrade.strSessionID = nodeDeviceUpgrade.child("SessionID").text().as_string();
   }
   tUpgrade.strResult = "";


   /*TODO 回调上抛*/
   { /* 配合上层实现业务功能 */
       SipCbResult_S stResult;
       auto fnCb = ::SipModule::instance()->GetDeviceUpgradeCb();
       if (fnCb)
       {
           fnCb(tUpgrade, stResult);
       }

       if(stResult.nResult != 0)
       {
           tUpgrade.strResult = "ERROR";
           dlog_warn("回调操作执行失败[%d]", stResult.nResult);
       }
   }

   //拼接返回报文
   ResponeControlResult(e, nSN, m_header.strCmdType, strID, tUpgrade.strResult);

   return true;
};

bool DeviceCtrlEvent::HandleTargetTrack(const SipEvent::Ptr &e)
{
    ParseHeader(e);

    GB28181::TargetTrackInfo_S tTargetTrack;

   auto root = m_doc.first_child();
   /*序列号*/
   uint64_t nSN = root.child("SN").text().as_int();
   /* 设备Id */
   std::string strID; 
   if(!root.child("DeviceID").empty())
   {
       strID = root.child("DeviceID").text().as_string();
   }
   tTargetTrack.strID = strID;

   auto pClient = dynamic_cast<SipClient *>(e->m_pNetBase);
   if (pClient == nullptr)
   {
       dlog_warn("客户端实例为空，无法获取客户端的通道信息");
       return false;
   }
   auto pChn = pClient->GetChannelByID(strID);
   if (nullptr == pChn)
   {
       dlog_warn("没有找到通道[%s]", m_header.strDevID.c_str());
       return false;
   }

   std::string argetTrack;
   auto nodeTargetTrack = root.child("TargetTrack");
   if(!nodeTargetTrack.empty())
   {
        argetTrack = nodeTargetTrack.text().as_string();
   }
   if(argetTrack.compare("Auto") == 0)
   {
        tTargetTrack.TrackType = GB28181::TargetTrackInfo_S::TYPE::AUTO;
   }
   else if(argetTrack.compare("Manual") == 0)
   {
        tTargetTrack.TrackType = GB28181::TargetTrackInfo_S::TYPE::MANUAL;
   }
   else if(argetTrack.compare("Stop") == 0)
   {
        tTargetTrack.TrackType = GB28181::TargetTrackInfo_S::TYPE::STOP;
   }

   if(root.child("DeviceID2").empty())
   {
        tTargetTrack.strDeviceID2 = root.child("DeviceID2").text().as_string();
   }
   
   auto nodeTargetArea = root.child("TargetArea");

   tTargetTrack.nLength = nodeTargetArea.child("Length").text().as_int();
   tTargetTrack.nWidth = nodeTargetArea.child("Width").text().as_int();
   tTargetTrack.nMidPointX = nodeTargetArea.child("MidPointX").text().as_int();
   tTargetTrack.nMidPointY = nodeTargetArea.child("MidPointY").text().as_int();
   tTargetTrack.nLengthX = nodeTargetArea.child("LengthX").text().as_int();
   tTargetTrack.nLengthY = nodeTargetArea.child("LengthY").text().as_int();

   /*TODO 回调上抛*/
   { /* 配合上层实现业务功能 */
       SipCbResult_S stResult;
       auto fnCb = ::SipModule::instance()->GetTargetTrackCb();
       if (fnCb)
       {
           fnCb(tTargetTrack, stResult);
       }

       if(stResult.nResult != 0)
       {
           dlog_warn("回调操作执行失败[%d]", stResult.nResult);
       }
   }

   return true;
}

bool DeviceCtrlEvent::ResponeControlResult(const SipEvent::Ptr &e, uint64_t nSN, std::string strCmdType, std::string strDeviceID, std::string strResult)
{
    //拼接返回报文
    pugi::xml_document stNewDoc;

    auto declaration = stNewDoc.append_child(pugi::node_declaration);
    auto attrVersion = declaration.append_attribute("version");
    attrVersion.set_value("1.0");
    auto attrEncoding = declaration.append_attribute("encoding");
    attrEncoding.set_value("GB18030");

    auto rootRet = stNewDoc.append_child("Response");

    auto nodeCmdType = rootRet.append_child("CmdType");
    nodeCmdType.text().set(strCmdType);

    auto nodeSN = rootRet.append_child("SN");
    nodeSN.text().set(nSN);

    auto nodeDeviceID = rootRet.append_child("DeviceID");
    nodeDeviceID.text().set(strDeviceID);

    auto nodeResult = rootRet.append_child("Result");
    nodeResult.text().set(strResult);

    std::ostringstream os;
    stNewDoc.save(os);
    auto strGB18030 = ::ToMbcsString(os.str());
    SendMessageWithCallID(e, strGB18030);

    return true;
}

bool DeviceCtrlEvent::HandleBasicControl(const SipEvent::Ptr &e)
{
    ParseHeader(e);

     /*设备BasicParam配置控制设置*/ 
     GB28181::BasicParamInfo_S tBasicParamInfo;

    auto root = m_doc.first_child();
    /*序列号*/
    uint64_t nSN = root.child("SN").text().as_int();
    /* 设备Id */
    std::string strID; 
    if(!root.child("DeviceID").empty())
    {
        strID = root.child("DeviceID").text().as_string();
    }
    tBasicParamInfo.strID = strID;
    /* 只获设置当前设备----注释掉通道 */
    //auto pClient = dynamic_cast<SipClient *>(e->m_pNetBase);
    //if (pClient == nullptr)
    //{
    //    ResponeControlResult(e, nSN, m_header.strCmdType, strID, "ERROR");
    //    dlog_warn("客户端实例为空，无法获取客户端的通道信息");
    //    return false;
    //}
    //auto pChn = pClient->GetChannelByID(strID);
    //if (nullptr == pChn)
    //{
    //    ResponeControlResult(e, nSN, m_header.strCmdType, strID, "ERROR");
    //    dlog_warn("没有找到通道[%s]", m_header.strDevID.c_str());
    //    return false;
    //}
    /*上级根据通道号赋值，下级设置成-1*/
    tBasicParamInfo.nIndex = -1;
    /*true-设备配置设置，false-设备配置获取*/
    tBasicParamInfo.bIsSet = true;

    auto nodeBasicParam = root.child("BasicParam");

    if(!nodeBasicParam.child("Name").empty())
    {
        tBasicParamInfo.m_szName = nodeBasicParam.child("Name").text().as_string();
    }
    tBasicParamInfo.m_nExpiration = nodeBasicParam.child("Expiration").text().as_int();
    tBasicParamInfo.m_nHeartBeatInterval = nodeBasicParam.child("HeartBeatInterval").text().as_int();
    tBasicParamInfo.m_nHeartBeatCount = nodeBasicParam.child("HeartBeatCount").text().as_int();

    /*TODO 回调上抛*/
    { /* 配合上层实现业务功能 */
        SipCbResult_S stResult;
        auto fnCb = ::SipModule::instance()->GetBasicParamCb();
        if (fnCb)
        {
            fnCb(tBasicParamInfo, stResult);
        }

        if(stResult.nResult != 0)
        {
            tBasicParamInfo.strResult = "ERROR";
            dlog_warn("回调操作执行失败[%d]", stResult.nResult);
        }
        else
        {
            tBasicParamInfo.strResult = "OK";
            dlog_warn("回调操作执行成功[%d]", stResult.nResult);
        }
    }

    //拼接返回报文
    ResponeControlResult(e, nSN, m_header.strCmdType, strID, tBasicParamInfo.strResult);

    return true;

}

bool DeviceCtrlEvent::HandleVideoOptControl(const SipEvent::Ptr &e)
{
    ParseHeader(e);

     /*设备BasicParam配置控制设置*/ 
     GB28181::VideoParamOptInfo_S tParamOpt;

    auto root = m_doc.first_child();
    /*序列号*/
    uint64_t nSN = root.child("SN").text().as_int();
    /* 设备Id */
    std::string strID; 
    if(!root.child("DeviceID").empty())
    {
        strID = root.child("DeviceID").text().as_string();
    }
    tParamOpt.strID = strID;

    auto pClient = dynamic_cast<SipClient *>(e->m_pNetBase);
    if (pClient == nullptr)
    {
        ResponeControlResult(e, nSN, m_header.strCmdType, strID, "ERROR");
        dlog_warn("客户端实例为空，无法获取客户端的通道信息");
        return false;
    }
    auto pChn = pClient->GetChannelByID(strID);
    if (nullptr == pChn)
    {
        ResponeControlResult(e, nSN, m_header.strCmdType, strID, "ERROR");
        dlog_warn("没有找到通道[%s]", m_header.strDevID.c_str());
        return false;
    }
    /*上级根据通道号赋值，下级设置成-1*/
    tParamOpt.nIndex = pChn->nIndex;
    /*true-设备配置设置，false-设备配置获取*/
    tParamOpt.bIsSet = true;

    auto nodeParamOpt = root.child("VideoParamOpt");

    //下载倍数
    if(!nodeParamOpt.child("DownloadSpeed").empty())
    {
        tParamOpt.strDownloadSpeed = nodeParamOpt.child("DownloadSpeed").text().as_string();
    }
    //分辨率
    if(!nodeParamOpt.child("Resolution").empty())
    {
        tParamOpt.strResolution = nodeParamOpt.child("Resolution").text().as_string();
    }
    
    /*TODO 回调上抛*/
    { /* 配合上层实现业务功能 */
        SipCbResult_S stResult;
        auto fnCb = ::SipModule::instance()->GetVideoParamOptCb();
        if (fnCb)
        {
            fnCb(tParamOpt, stResult);
        }

        if(stResult.nResult != 0)
        {
            tParamOpt.strResult = "ERROR";
            dlog_warn("回调操作执行失败[%d]", stResult.nResult);
        }
    }

    //拼接返回报文
    ResponeControlResult(e, nSN, m_header.strCmdType, strID, tParamOpt.strResult);
    
    return true;
}

bool DeviceCtrlEvent::HandleSVACEncodeControl(const SipEvent::Ptr &e)
{
    ParseHeader(e);

    /*设备BasicParam配置控制设置*/ 
    GB28181::SVACEncodeInfo_S tSVACEncode;

    auto root = m_doc.first_child();
    /*序列号*/
    uint64_t nSN = root.child("SN").text().as_int();
    /* 设备Id */
    std::string strID; 
    if(!root.child("DeviceID").empty())
    {
        strID = root.child("DeviceID").text().as_string();
    }
    tSVACEncode.strID = strID;

    auto pClient = dynamic_cast<SipClient *>(e->m_pNetBase);
    if (pClient == nullptr)
    {
        ResponeControlResult(e, nSN, m_header.strCmdType, strID, "ERROR");
        dlog_warn("客户端实例为空，无法获取客户端的通道信息");
        return false;
    }
    auto pChn = pClient->GetChannelByID(strID);
    if (nullptr == pChn)
    {
        ResponeControlResult(e, nSN, m_header.strCmdType, strID, "ERROR");
        dlog_warn("没有找到通道[%s]", m_header.strDevID.c_str());
        return false;
    }
    /*上级根据通道号赋值，下级设置成-1*/
    tSVACEncode.nIndex = pChn->nIndex;
    /*true-设备配置设置，false-设备配置获取*/
    tSVACEncode.bIsSet = true;

    auto node = root.child("SVACEncodeConfig");

    //感兴趣区域参数
    {
        auto nodeROI = node.child("ROIParam");
        if (!nodeROI.empty())
        {
            //区域开关
            tSVACEncode.tROIParam.nROIFlag = nodeROI.child("ROIFlag").text().as_int();
            //区域数量
            tSVACEncode.tROIParam.nROINumber = nodeROI.child("ROINumber").text().as_int();
            auto children = nodeROI.children("Item");
            for (auto &&child : children)
            {
                GB28181::ROIParamItem Item;
                Item.nROISeq = child.child("ROISeq").text().as_int();
                Item.nTopLeft = child.child("TopLeft").text().as_int();
                Item.nBottomRight  = child.child("BottomRight").text().as_int();
                Item.nROIQP  = child.child("ROIQP").text().as_int();
                tSVACEncode.tROIParam.vecROIParamItem.push_back(Item);
            }
        }
    }
    
    //SVC参数
    {
        auto nodeSVC = node.child("SVCParam");
        if (!nodeSVC.empty())
        {
            //空域编码方式
            if(!nodeSVC.child("SVCSpaceDomainMode").empty())
            {
                tSVACEncode.tSVCParam.nSVCSpaceDomainMode = nodeSVC.child("SVCSpaceDomainMode").text().as_int();
            }
            //时域编码方式
            if(!nodeSVC.child("SVCTimeDomainMode").empty())
            {
                tSVACEncode.tSVCParam.nSVCTimeDomainMode = nodeSVC.child("SVCTimeDomainMode").text().as_int();
            }
            //比例值
            if(!nodeSVC.child("SSVCRatioValue").empty())
            {
                tSVACEncode.tSVCParam.strSSVCRatioValue = nodeSVC.child("SSVCRatioValue").text().as_string();
            }
            //空域编码能力
            if(!nodeSVC.child("SVCSpaceSupportMode").empty())
            {
                tSVACEncode.tSVCParam.nSVCSpaceSupportMode = nodeSVC.child("SVCSpaceSupportMode").text().as_int();
            }
             //时域编码能力
            if(!nodeSVC.child("SVCTimeSupportMode").empty())
            {
                tSVACEncode.tSVCParam.nSVCTimeSupportMode = nodeSVC.child("SVCTimeSupportMode").text().as_int();
            }
            //比例能力
            if(!nodeSVC.child("SSVCRatioSupportList").empty())
            {
                tSVACEncode.tSVCParam.strSSVCRatioSupportList = nodeSVC.child("SSVCRatioSupportList").text().as_string();
            }
        }
    }
    //监控专用信息参数
    {
        auto nodeSurveillance = node.child("SurveillanceParam");
        if (!nodeSurveillance.empty())
        {
            //时间信息开关
            tSVACEncode.tSurveillanceParam.nTimeFlag = nodeSurveillance.child("TimeFlag").text().as_int();
            tSVACEncode.tSurveillanceParam.nOSDFlag  = nodeSurveillance.child("OSDFlag").text().as_int();
            tSVACEncode.tSurveillanceParam.nAIFlag   = nodeSurveillance.child("AIFlag").text().as_int();
            tSVACEncode.tSurveillanceParam.nGISFlag   = nodeSurveillance.child("GISFlag").text().as_int();
        }
    }
    //音频参数
    {
        auto nodeAudio = node.child("AudioParam");
        if (!nodeAudio.empty())
        {
            //时间信息开关
            tSVACEncode.tAudioParam.nAudioRecognitionFlag = nodeAudio.child("AudioRecognitionFlag").text().as_int();
        }
    }

    /*TODO 回调上抛*/
    { /* 配合上层实现业务功能 */
        SipCbResult_S stResult;
        auto fnCb = ::SipModule::instance()->GetSVACEncodeCb();
        if (fnCb)
        {
            fnCb(tSVACEncode, stResult);
        }

        if(stResult.nResult != 0)
        {
            tSVACEncode.strResult = "ERROR";
            dlog_warn("回调操作执行失败[%d]", stResult.nResult);
        }
    }

    //拼接返回报文
    ResponeControlResult(e, nSN, m_header.strCmdType, strID, tSVACEncode.strResult);
    
    return true;
}

bool DeviceCtrlEvent::HandleSVACDncodeControl(const SipEvent::Ptr &e)
{
    ParseHeader(e);

    GB28181::SVACDecodeInfo_S tSVACDecode;

    auto root = m_doc.first_child();
    /*序列号*/
    uint64_t nSN = root.child("SN").text().as_int();
    /* 设备Id */
    std::string strID; 
    if(!root.child("DeviceID").empty())
    {
        strID = root.child("DeviceID").text().as_string();
    }
    tSVACDecode.strID = strID;

    auto pClient = dynamic_cast<SipClient *>(e->m_pNetBase);
    if (pClient == nullptr)
    {
        ResponeControlResult(e, nSN, m_header.strCmdType, strID, "ERROR");
        dlog_warn("客户端实例为空，无法获取客户端的通道信息");
        return false;
    }
    auto pChn = pClient->GetChannelByID(strID);
    if (nullptr == pChn)
    {
        ResponeControlResult(e, nSN, m_header.strCmdType, strID, "ERROR");
        dlog_warn("没有找到通道[%s]", m_header.strDevID.c_str());
        return false;
    }
    /*上级根据通道号赋值，下级设置成-1*/
    tSVACDecode.nIndex = pChn->nIndex;
    /*true-设备配置设置，false-设备配置获取*/
    tSVACDecode.bIsSet = true;

    auto node = root.child("SVACDecodeConfig");

    //SVC参数
    {
        auto nodeSVC = node.child("SVCParam");
        if (!nodeSVC.empty())
        {
            //码流显示模式
            tSVACDecode.tSVCParam.nSVCSTMMode = nodeSVC.child("SVCSTMMode").text().as_int();
            //空域编码能力
            tSVACDecode.tSVCParam.nSVCSpaceSupportMode = nodeSVC.child("SVCSpaceSupportMode").text().as_int();
             //时域编码能力
            tSVACDecode.tSVCParam.nSVCTimeSupportMode = nodeSVC.child("SVCTimeSupportMode").text().as_int();
        }
    }
    //监控专用信息参数
    {
        auto nodeSurveillance = node.child("SurveillanceParam");
        if (!nodeSurveillance.empty())
        {
            //时间信息开关
            tSVACDecode.tSurveillanceParam.nTimeShowFlag  = nodeSurveillance.child("TimeShowFlag").text().as_int();
            tSVACDecode.tSurveillanceParam.nOSDShowFlag   = nodeSurveillance.child("OSDShowFlag").text().as_int();
            tSVACDecode.tSurveillanceParam.nAIShowFlag    = nodeSurveillance.child("AIShowFlag").text().as_int();
            tSVACDecode.tSurveillanceParam.nGISShowFlag   = nodeSurveillance.child("GISShowFlag").text().as_int();
        }
    }

    /*TODO 回调上抛*/
    { /* 配合上层实现业务功能 */
        SipCbResult_S stResult;
        auto fnCb = ::SipModule::instance()->GetSVACDncodeCb();
        if (fnCb)
        {
            fnCb(tSVACDecode, stResult);
        }

        if(stResult.nResult != 0)
        {
            tSVACDecode.strResult = "ERROR";
            dlog_warn("回调操作执行失败[%d]", stResult.nResult);
        }
    }

    //拼接返回报文
    ResponeControlResult(e, nSN, m_header.strCmdType, strID, tSVACDecode.strResult);
    
    return true;
}

bool DeviceCtrlEvent::HandleVideoAttriControl(const SipEvent::Ptr &e)
{
    ParseHeader(e);

    GB28181::VideoParamAttributeInfo_S tAttribute;

    auto root = m_doc.first_child();
    /*序列号*/
    uint64_t nSN = root.child("SN").text().as_int();
    /* 设备Id */
    std::string strID; 
    if(!root.child("DeviceID").empty())
    {
        strID = root.child("DeviceID").text().as_string();
    }
    tAttribute.strID = strID;

    auto pClient = dynamic_cast<SipClient *>(e->m_pNetBase);
    if (pClient == nullptr)
    {
        ResponeControlResult(e, nSN, m_header.strCmdType, strID, "ERROR");
        dlog_warn("客户端实例为空，无法获取客户端的通道信息");
        return false;
    }
    auto pChn = pClient->GetChannelByID(strID);
    if (nullptr == pChn)
    {
        ResponeControlResult(e, nSN, m_header.strCmdType, strID, "ERROR");
        dlog_warn("没有找到通道[%s]", m_header.strDevID.c_str());
        return false;
    }
    /*上级根据通道号赋值，下级设置成-1*/
    tAttribute.nIndex = pChn->nIndex;
    /*true-设备配置设置，false-设备配置获取*/
    tAttribute.bIsSet = true;

    auto node = root.child("VideoParamAttribute");

    auto children = node.children("Item");
    for (auto &&child : children)
    {
        GB28181::VideoParamAttributeItem Item;
        //码流编号
        Item.nStreamNumber = child.child("StreamNumber").text().as_int();
        //视频编码格式
        if(!child.child("VideoFormat").empty())
        {
            Item.enVideoFormat = (GB28181::GbVideoType_E)child.child("VideoFormat").text().as_int();
        }
        //分辨率
        if(!child.child("Resolution").empty())
        {
            Item.enResolution  = (GB28181::GbResolutionType_E)child.child("Resolution").text().as_int();
        }
        //帧率
        if(!child.child("FrameRate").empty())
        {
            Item.strFrameRate  = child.child("FrameRate").text().as_string();
        }
        //码率类型
        if(!child.child("BitRateType").empty())
        {
            Item.enBitRateType = (GB28181::GbBitRateType_E)child.child("BitRateType").text().as_int();
        }
        //视频码率
        if(!child.child("VideoBitRate").empty())
        {
            Item.strVideoBitRate = child.child("VideoBitRate").text().as_string();
        }
        tAttribute.vecVideoParAttrItem.push_back(Item);
    }

    /*TODO 回调上抛*/
    { /* 配合上层实现业务功能 */
        SipCbResult_S stResult;
        auto fnCb = ::SipModule::instance()->GetVideoAttributeCb();
        if (fnCb)
        {
            fnCb(tAttribute, stResult);
        }

        if(stResult.nResult != 0)
        {
            tAttribute.strResult = "ERROR";
            dlog_warn("回调操作执行失败[%d]", stResult.nResult);
        }
    }

    //拼接返回报文
    ResponeControlResult(e, nSN, m_header.strCmdType, strID, tAttribute.strResult);
    
    return true;
}

bool DeviceCtrlEvent::HandleRecordPlanControl(const SipEvent::Ptr &e)
{
    ParseHeader(e);

    GB28181::VideoRecordPlanInfo_S tRecordPlan;

    auto root = m_doc.first_child();
    /*序列号*/
    uint64_t nSN = root.child("SN").text().as_int();
    /* 设备Id */
    std::string strID; 
    if(!root.child("DeviceID").empty())
    {
        strID = root.child("DeviceID").text().as_string();
    }
    tRecordPlan.strID = strID;

    auto pClient = dynamic_cast<SipClient *>(e->m_pNetBase);
    if (pClient == nullptr)
    {
        ResponeControlResult(e, nSN, m_header.strCmdType, strID, "ERROR");
        dlog_warn("客户端实例为空，无法获取客户端的通道信息");
        return false;
    }
    auto pChn = pClient->GetChannelByID(strID);
    if (nullptr == pChn)
    {
        ResponeControlResult(e, nSN, m_header.strCmdType, strID, "ERROR");
        dlog_warn("没有找到通道[%s]", m_header.strDevID.c_str());
        return false;
    }
    /*上级根据通道号赋值，下级设置成-1*/
    tRecordPlan.nIndex = pChn->nIndex;
    /*true-设备配置设置，false-设备配置获取*/
    tRecordPlan.bIsSet = true;

    auto node = root.child("VideoRecordPlan");

    /*录像启用,0-否，1-是*/
    tRecordPlan.nRecordEnable = node.child("RecordEnable").text().as_int();
     /*录像计划总天数*/
    tRecordPlan.nRecordScheduleSumNum = node.child("RecordScheduleSumNum").text().as_int();
    /*码流类型*/
    tRecordPlan.nStreamNumber = node.child("StreamNumber").text().as_int();  
    
    auto nodeSchedulechil = node.children("RecordSchedule");
    for (auto &&childS : nodeSchedulechil)
    {
        //每一天的计划
        GB28181::RecordScheduleItem ScheduleItem;

        //周几
        ScheduleItem.nWeekDayNum = childS.child("WeekDayNum").text().as_int();  
        //每天录像计划时间段总数
        ScheduleItem.nTimeSegmentSumNum = childS.child("TimeSegmentSumNum").text().as_int(); 
        auto childTime = childS.children("TimeSegment");
        for (auto &&childT : childTime)
        {
            GB28181::TimeSegmentItem  SegmentItem;
            /*开始时间时*/
            SegmentItem.nStartHour = childT.child("StartHour").text().as_int();
            /*开始时间分*/
            SegmentItem.nStartMin = childT.child("StartMin").text().as_int();
            /*开始时间秒*/
            SegmentItem.nStartSec = childT.child("StartSec").text().as_int();
            /*结束时间时*/
            SegmentItem.nStopHour = childT.child("StopHour").text().as_int();
             /*结束时间分*/
            SegmentItem.nStopMin = childT.child("nStopMin").text().as_int();
            /*结束时间秒*/
            SegmentItem.nStopSec = childT.child("StopSec").text().as_int();
            ScheduleItem.vecTimeSegment.push_back(SegmentItem);
        }
        tRecordPlan.vecRecordSchedule.insert(std::pair<int,  GB28181::RecordScheduleItem>(ScheduleItem.nWeekDayNum, ScheduleItem));
    }

    /*TODO 回调上抛*/
    { /* 配合上层实现业务功能 */
        SipCbResult_S stResult;
        auto fnCb = ::SipModule::instance()->GetRecordPlanCb();
        if (fnCb)
        {
            fnCb(tRecordPlan, stResult);
        }

        if(stResult.nResult != 0)
        {
            tRecordPlan.strResult = "ERROR";
            dlog_warn("回调操作执行失败[%d]", stResult.nResult);
        }
    }

    //拼接返回报文
    ResponeControlResult(e, nSN, m_header.strCmdType, strID, tRecordPlan.strResult);
    
    return true;
}

bool DeviceCtrlEvent::HandleAlarmRecordControl(const SipEvent::Ptr &e)
{
    ParseHeader(e);

    GB28181::VideoAlarmRecordInfo_S tAlarmRecord;

    auto root = m_doc.first_child();
    /*序列号*/
    uint64_t nSN = root.child("SN").text().as_int();
    /* 设备Id */
    std::string strID; 
    if(!root.child("DeviceID").empty())
    {
        strID = root.child("DeviceID").text().as_string();
    }
    tAlarmRecord.strID = strID;

    auto pClient = dynamic_cast<SipClient *>(e->m_pNetBase);
    if (pClient == nullptr)
    {
        ResponeControlResult(e, nSN, m_header.strCmdType, strID, "ERROR");
        dlog_warn("客户端实例为空，无法获取客户端的通道信息");
        return false;
    }
    auto pChn = pClient->GetChannelByID(strID);
    if (nullptr == pChn)
    {
        ResponeControlResult(e, nSN, m_header.strCmdType, strID, "ERROR");
        dlog_warn("没有找到通道[%s]", m_header.strDevID.c_str());
        return false;
    }
    /*上级根据通道号赋值，下级设置成-1*/
    tAlarmRecord.nIndex = pChn->nIndex;
    /*true-设备配置设置，false-设备配置获取*/
    tAlarmRecord.bIsSet = true;

    auto node = root.child("VideoRecordPlan");

    tAlarmRecord.nRecordEnable = node.child("RecordEnable").text().as_int();
    tAlarmRecord.nRecordTime = node.child("RecordTime").text().as_int();
    tAlarmRecord.nPreRecordTime = node.child("PreRecordTime").text().as_int();
    tAlarmRecord.nStreamNumber = node.child("StreamNumber").text().as_int();

    /*TODO 回调上抛*/
    { /* 配合上层实现业务功能 */
        SipCbResult_S stResult;
        auto fnCb = ::SipModule::instance()->GetAlarmRecordCb();
        if (fnCb)
        {
            fnCb(tAlarmRecord, stResult);
        }

        if(stResult.nResult != 0)
        {
            tAlarmRecord.strResult = "ERROR";
            dlog_warn("回调操作执行失败[%d]", stResult.nResult);
        }
    }

    //拼接返回报文
    ResponeControlResult(e, nSN, m_header.strCmdType, strID, tAlarmRecord.strResult);

    return true;
}

bool DeviceCtrlEvent::HandlePictureMaskControl(const SipEvent::Ptr &e)
{
    ParseHeader(e);

    GB28181::PictureMaskInfo_S tPictureMask;

    auto root = m_doc.first_child();
    /*序列号*/
    uint64_t nSN = root.child("SN").text().as_int();
    /* 设备Id */
    std::string strID; 
    if(!root.child("DeviceID").empty())
    {
        strID = root.child("DeviceID").text().as_string();
    }
    tPictureMask.strID = strID;

    auto pClient = dynamic_cast<SipClient *>(e->m_pNetBase);
    if (pClient == nullptr)
    {
        ResponeControlResult(e, nSN, m_header.strCmdType, strID, "ERROR");
        dlog_warn("客户端实例为空，无法获取客户端的通道信息");
        return false;
    }
    auto pChn = pClient->GetChannelByID(strID);
    if (nullptr == pChn)
    {
        ResponeControlResult(e, nSN, m_header.strCmdType, strID, "ERROR");
        dlog_warn("没有找到通道[%s]", m_header.strDevID.c_str());
        return false;
    }
    /*上级根据通道号赋值，下级设置成-1*/
    tPictureMask.nIndex = pChn->nIndex;
    /*true-设备配置设置，false-设备配置获取*/
    tPictureMask.bIsSet = true;

    auto node = root.child("PictureMask");

    tPictureMask.On = node.child("On").text().as_int();
    tPictureMask.SumNum = node.child("SumNum").text().as_int();

    auto nodeRegionList = node.child("RegionList");

    //区域列表
    auto children = nodeRegionList.children("Item");
    for (auto &&child : children)
    {
        GB28181::RegionListItem Item;
        //区域编号
        Item.Seq = child.child("Seq").text().as_int();
        //区域左上角
        std::string piont;
        if(!child.child("Piont").empty())
        {
            piont = child.child("Piont").text().as_string();
        }
        std::istringstream iss(piont);
        std::string token;
        std::vector<int> ids;
        while (std::getline(iss, token, ',')) {
            int id = std::stoi(token);
            ids.push_back(id); 
        }
        Item.nlx = ids[0];
        Item.nly = ids[1];
        Item.nrx = ids[2];
        Item.nry = ids[3];
        
        tPictureMask.vecRegionList.push_back(Item);
    }

    /*TODO 回调上抛*/
    { /* 配合上层实现业务功能 */
        SipCbResult_S stResult;
        auto fnCb = ::SipModule::instance()->GetPictureMaskCb();
        if (fnCb)
        {
            fnCb(tPictureMask, stResult);
        }

        if(stResult.nResult != 0)
        {
            tPictureMask.strResult = "ERROR";
            dlog_warn("回调操作执行失败[%d]", stResult.nResult);
        }
    }

    //拼接返回报文
    ResponeControlResult(e, nSN, m_header.strCmdType, strID, tPictureMask.strResult);

    return true;

}

bool DeviceCtrlEvent::HandleFrameMirrorControl(const SipEvent::Ptr &e)
{
    ParseHeader(e);

    GB28181::FrameMirrorInfo_S Mirror;

    auto root = m_doc.first_child();
    /*序列号*/
    uint64_t nSN = root.child("SN").text().as_int();
    /* 设备Id */
    std::string strID; 
    if(!root.child("DeviceID").empty())
    {
        strID = root.child("DeviceID").text().as_string();
    }
    Mirror.strID = strID;

    auto pClient = dynamic_cast<SipClient *>(e->m_pNetBase);
    if (pClient == nullptr)
    {
        ResponeControlResult(e, nSN, m_header.strCmdType, strID, "ERROR");
        dlog_warn("客户端实例为空，无法获取客户端的通道信息");
        return false;
    }
    auto pChn = pClient->GetChannelByID(strID);
    if (nullptr == pChn)
    {
        ResponeControlResult(e, nSN, m_header.strCmdType, strID, "ERROR");
        dlog_warn("没有找到通道[%s]", m_header.strDevID.c_str());
        return false;
    }
    /*上级根据通道号赋值，下级设置成-1*/
    Mirror.nIndex = pChn->nIndex;
    /*true-设备配置设置，false-设备配置获取*/
    Mirror.bIsSet = true;

    auto node = root.child("PictureMask");

    /*0-不启用镜像，1-水平镜像，2-上下镜像，3-中心镜像*/
    Mirror.nFrameMirror = root.child("FrameMirror").text().as_int();

    /*TODO 回调上抛*/
    { /* 配合上层实现业务功能 */
        SipCbResult_S stResult;
        auto fnCb = ::SipModule::instance()->GetFrameMirrorCb();
        if (fnCb)
        {
            fnCb(Mirror, stResult);
        }

        if(stResult.nResult != 0)
        {
            Mirror.strResult = "ERROR";
            dlog_warn("回调操作执行失败[%d]", stResult.nResult);
        }
    }

   //拼接返回报文
   ResponeControlResult(e, nSN, m_header.strCmdType, strID, Mirror.strResult);


    return true;
}

bool DeviceCtrlEvent::HandleAlarmReportControl(const SipEvent::Ptr &e)
{
    ParseHeader(e);

    GB28181::AlarmReportInfo_S AlarmRepor;

    auto root = m_doc.first_child();
    /*序列号*/
    uint64_t nSN = root.child("SN").text().as_int();
    /* 设备Id */
    std::string strID; 
    if(!root.child("DeviceID").empty())
    {
        strID = root.child("DeviceID").text().as_string();
    }
    AlarmRepor.strID = strID;

    auto pClient = dynamic_cast<SipClient *>(e->m_pNetBase);
    if (pClient == nullptr)
    {
        ResponeControlResult(e, nSN, m_header.strCmdType, strID, "ERROR");
        dlog_warn("客户端实例为空，无法获取客户端的通道信息");
        return false;
    }
    auto pChn = pClient->GetChannelByID(strID);
    if (nullptr == pChn)
    {
        ResponeControlResult(e, nSN, m_header.strCmdType, strID, "ERROR");
        dlog_warn("没有找到通道[%s]", m_header.strDevID.c_str());
        return false;
    }
    /*上级根据通道号赋值，下级设置成-1*/
    AlarmRepor.nIndex = pChn->nIndex;
    /*true-设备配置设置，false-设备配置获取*/
    AlarmRepor.bIsSet = true;

    auto node = root.child("AlarmReport");

    /*移动侦测事件上报开关，0-关闭，1-打开*/
    AlarmRepor.nMotionDetection = node.child("MotionDetection").text().as_int();
    AlarmRepor.nFieldDetection = node.child("FieldDetection").text().as_int();

    /*TODO 回调上抛*/
    { /* 配合上层实现业务功能 */
        SipCbResult_S stResult;
        auto fnCb = ::SipModule::instance()->GetAlarmReportCb();
        if (fnCb)
        {
            fnCb(AlarmRepor, stResult);
        }

        if(stResult.nResult != 0)
        {
            AlarmRepor.strResult = "ERROR";
            dlog_warn("回调操作执行失败[%d]", stResult.nResult);
        }
    }

    //拼接返回报文
    ResponeControlResult(e, nSN, m_header.strCmdType, strID, AlarmRepor.strResult);

    return true;
}

bool DeviceCtrlEvent::HandleOSDConfigControl(const SipEvent::Ptr &e)
{
    ParseHeader(e);

    GB28181::OSDConfig_S OSD;

    auto root = m_doc.first_child();
    /*序列号*/
    uint64_t nSN = root.child("SN").text().as_int();
    /* 设备Id */
    std::string strID;
    auto nodeDevice = root.child("DeviceID");
    if(!nodeDevice.empty())
    {
        strID = nodeDevice.text().as_string();
    }
    OSD.strID = strID;

    auto pClient = dynamic_cast<SipClient *>(e->m_pNetBase);
    if (pClient == nullptr)
    {
        ResponeControlResult(e, nSN, m_header.strCmdType, strID, "ERROR");
        dlog_warn("客户端实例为空，无法获取客户端的通道信息");
        return false;
    }
    auto pChn = pClient->GetChannelByID(strID);
    if (nullptr == pChn)
    {
        ResponeControlResult(e, nSN, m_header.strCmdType, strID, "ERROR");
        dlog_warn("没有找到通道[%s]", m_header.strDevID.c_str());
        return false;
    }
    /*上级根据通道号赋值，下级设置成-1*/
    OSD.nIndex = pChn->nIndex;
    /*true-设备配置设置，false-设备配置获取*/
    OSD.bIsSet = true;

    auto node = root.child("OSDConfig");

    OSD.m_nLength = node.child("Length").text().as_int();
    OSD.m_nWidth = node.child("Width").text().as_int();
    OSD.m_nTimeX = node.child("TimeX").text().as_int();
    OSD.m_nTimeY = node.child("TimeY").text().as_int();
    OSD.m_SumNum = node.child("SumNum").text().as_int();

    auto children = node.children("Item");
    for (auto &&child : children)
    {
        GB28181::OSDCItem  Item;
        if(child.child("Text").empty())
        {
            Item.Text = child.child("Text").text().as_string();
        }
        Item.X = child.child("X").text().as_int();
        Item.Y = child.child("Y").text().as_int();
        OSD.m_vecItme.push_back(Item);
    }

    /*TODO 回调上抛*/
    { /* 配合上层实现业务功能 */
        SipCbResult_S stResult;
        auto fnCb = ::SipModule::instance()->GetOSDConfigCb();
        if (fnCb)
        {
            fnCb(OSD, stResult);
        }

        if(stResult.nResult != 0)
        {
            OSD.strResult = "ERROR";
            dlog_warn("回调操作执行失败[%d]", stResult.nResult);
        }
    }

    //拼接返回报文
    ResponeControlResult(e, nSN, m_header.strCmdType, strID, OSD.strResult);
 
    return true;
}

bool DeviceCtrlEvent::HandleSnapShotControl(const SipEvent::Ptr &e)
{
    ParseHeader(e);

    GB28181::SnapShotConfigInfo_S Snap;

    auto root = m_doc.first_child();
    /*序列号*/
    uint64_t nSN = root.child("SN").text().as_int();
    /* 设备Id */
    std::string strID;
    auto nodeDevice = root.child("DeviceID");
    if(!nodeDevice.empty())
    {
        strID = nodeDevice.text().as_string();
    }
    Snap.strID = strID;

    auto pClient = dynamic_cast<SipClient *>(e->m_pNetBase);
    if (pClient == nullptr)
    {
        ResponeControlResult(e, nSN, m_header.strCmdType, strID, "ERROR");
        dlog_warn("客户端实例为空，无法获取客户端的通道信息");
        return false;
    }
    auto pChn = pClient->GetChannelByID(strID);
    if (nullptr == pChn)
    {
        ResponeControlResult(e, nSN, m_header.strCmdType, strID, "ERROR");
        dlog_warn("没有找到通道[%s]", m_header.strDevID.c_str());
        return false;
    }
    /*上级根据通道号赋值，下级设置成-1*/
    Snap.nIndex = pChn->nIndex;
    /*true-设备配置设置，false-设备配置获取*/
    Snap.bIsSet = true;

    auto node = root.child("SnapShotConfig");

    Snap.nSnapNum = node.child("SnapNum").text().as_int();
    Snap.nInterval = node.child("Interval").text().as_int();
    if(!node.child("UploadURL").empty())
    {
        Snap.strUploadURL  = node.child("UploadURL").text().as_string();
    }
    if(!node.child("SessionID").empty())
    {
        Snap.strSessionID  = node.child("SessionID").text().as_string();
    }  

    /*TODO 回调上抛*/
    { /* 配合上层实现业务功能 */
        SipCbResult_S stResult;
        auto fnCb = ::SipModule::instance()->GetSnapShotCb();
        if (fnCb)
        {
            fnCb(Snap, stResult);
        }

        if(stResult.nResult != 0)
        {
            Snap.strResult = "ERROR";
            dlog_warn("回调操作执行失败[%d]", stResult.nResult);
        }
    }

    //拼接返回报文
    ResponeControlResult(e, nSN, m_header.strCmdType, strID, Snap.strResult);

    return true;
}

bool CruiseTrackEvent::Handle(const SipEvent::Ptr &e)
{
    ParseHeader(e);

    /* 先响应200，说明已接收到请求 */
    SendResponse(e, SIP_OK);

    GB28181::CruiseTrackQueryInfo_S tTrack;

    auto root = m_doc.first_child();
    /*序列号*/
    tTrack.nSN = root.child("SN").text().as_int();
    /* 设备Id */
    std::string strID;
    auto nodeDevice = root.child("DeviceID");
    if(!nodeDevice.empty())
    {
        strID = nodeDevice.text().as_string();
    }
    tTrack.strID = strID;

    /*通道标号*/
    tTrack.nIndex = -1;  //通道下标，下级默认-1
    /*true--设备配置设置，false--设备配置获取*/
    tTrack.bIsSet = false;

    tTrack.nNumber = root.child("Number").text().as_int();
    if(!root.child("Name").empty())
    {
        tTrack.strName = root.child("Name").text().as_string();
    }
    tTrack.nSumNum = root.child("SumNum").text().as_int();/*以实际收到的数量为准，还是以报文的数量为准？*/

    auto node = root.child("CruisePointList");

    auto children = node.children("CruisePoint");
    for (auto &&child : children)
    {
        GB28181::CruisePointItem  Item;
        Item.nPresetIndex = child.child("PresetIndex").text().as_int();
        Item.nStayTime = child.child("StayTime").text().as_int();
        Item.nSpeed = child.child("Speed").text().as_int();
        tTrack.vecCruisePointList.push_back(Item);
    }

    /*TODO 基本参数配置回调上抛*/
    { /* 配合上层实现业务功能 */
        SipCbResult_S stResult;
        auto fnCb = ::SipModule::instance()->GetCruiseTrackQueryCb();
        if (fnCb)
        {
            fnCb(tTrack, stResult);
        }
    }

    return true;
}

bool CruiseTrackListEvent::Handle(const SipEvent::Ptr &e)
{
    ParseHeader(e);

    /* 先响应200，说明已接收到请求 */
    SendResponse(e, SIP_OK);

    GB28181::CruiseTrackListQueryInfo_S tTrack;

    auto root = m_doc.first_child();
    /*序列号*/
    tTrack.nSN = root.child("SN").text().as_int();
    /* 设备Id */
    std::string strID;
    auto nodeDevice = root.child("DeviceID");
    if(!nodeDevice.empty())
    {
        strID = nodeDevice.text().as_string();
    }
    tTrack.strID = strID;

    /*通道标号*/
    tTrack.nIndex = -1;  //通道下标，下级默认-1
    /*true--设备配置设置，false--设备配置获取*/
    tTrack.bIsSet = false;

    tTrack.nSumNum = root.child("SumNum").text().as_int();/*以实际收到的数量为准，还是以报文的数量为准？*/

    /*巡航轨迹信息*/
    auto node = root.child("CruiseTrackList");

    /*轨迹信息*/
    auto children = node.children("CruiseTrack");
    for (auto &&child : children)
    {
        GB28181::CruiseTrackItem  Item;
        /*轨迹编号*/
        Item.nNumber = child.child("Number").text().as_int();
        /*轨迹名称*/
        if(!child.child("Name").empty())
        {
            Item.strName = child.child("Name").text().as_string();
        }
        tTrack.vecCruiseTracktList.push_back(Item);
    }

    /*TODO 巡航轨迹回调上抛*/
    { /* 配合上层实现业务功能 */
        SipCbResult_S stResult;
        auto fnCb = ::SipModule::instance()->GetCruiseTrackListQueryCb();
        if (fnCb)
        {
            fnCb(tTrack, stResult);
        }
    }

    return true;
}

bool HomePositionQueryEvent::Handle(const SipEvent::Ptr &e)
{
    ParseHeader(e);

    /* 先响应200，说明已接收到请求 */
    SendResponse(e, SIP_OK);

    GB28181::HomePositionInfo_S tHomePosition;

    auto root = m_doc.first_child();
    /*序列号*/
    tHomePosition.nSN = root.child("SN").text().as_int();
    /* 设备Id */
    std::string strID;
    auto nodeDevice = root.child("DeviceID");
    if(!nodeDevice.empty())
    {
        strID = nodeDevice.text().as_string();
    }
    tHomePosition.strID = strID;

    /*通道标号*/
    tHomePosition.nIndex = -1;  //通道下标，下级默认-1
    /*true--设备配置设置，false--设备配置获取*/
    tHomePosition.bIsSet = false;

    auto node= root.child("HomePosition");
  
    tHomePosition.nEnabled = node.child("Enabled").text().as_int();
    tHomePosition.nResetTime = node.child("ResetTime").text().as_int();
    tHomePosition.nPresetIndex = node.child("PresetIndex").text().as_int();

    /*TODO 巡航轨迹回调上抛*/
    { /* 配合上层实现业务功能 */
        SipCbResult_S stResult;
        auto fnCb = ::SipModule::instance()->GetHomePositionCb();
        if (fnCb)
        {
            fnCb(tHomePosition, stResult);
        }
    }

    return true;
}

bool PTZPositionEvent::Handle(const SipEvent::Ptr &e)
{
    ParseHeader(e);

    /* 先响应200，说明已接收到请求 */
    SendResponse(e, SIP_OK);

    GB28181::PTZPositionInfo_S tPTZPosition;

    auto root = m_doc.first_child();
    /*序列号*/
    tPTZPosition.nSN = root.child("SN").text().as_int();
    /* 设备Id */
    std::string strID;
    auto nodeDevice = root.child("DeviceID");
    if(!nodeDevice.empty())
    {
        strID = nodeDevice.text().as_string();
    }
    tPTZPosition.strID = strID;

    /*通道标号*/
    tPTZPosition.nIndex = -1;  //通道下标，下级默认-1

    /*水平角度*/
    tPTZPosition.dPan= root.child("Pan").text().as_double();
     /* 云台垂直角度 */
    tPTZPosition.dTilt = root.child("Tilt").text().as_double();
     /*变焦倍数*/
    tPTZPosition.dZoom = root.child("Zoom").text().as_double();
    /*摄像机水平视场角*/
    tPTZPosition.HorizontalFieldAngle = root.child("HorizontalFieldAngle").text().as_double();
    /*摄像机垂直视场角*/
    tPTZPosition.dVerticalFieldAngle = root.child("VerticalFieldAngle").text().as_double();
    /*摄像机可视距离*/
    tPTZPosition.dMaxViewDistance = root.child("MaxViewDistance").text().as_double();

    /*TODO 巡航轨迹回调上抛*/
    { /* 配合上层实现业务功能 */
        SipCbResult_S stResult;
        auto fnCb = ::SipModule::instance()->GetPTZPositionCb();
        if (fnCb)
        {
            fnCb(tPTZPosition, stResult);
        }
    }

    return true;
}

bool UploadSnapShotFinishEvent::Handle(const SipEvent::Ptr &e)
{
    ParseHeader(e);

    /* 先响应200，说明已接收到请求 */
    SendResponse(e, SIP_OK);

    GB28181::UploadSnapShotFiniInfo_S tUploadFini;

    auto root = m_doc.first_child();
    /*序列号*/
    tUploadFini.nSN = root.child("SN").text().as_int();
    /* 设备Id */
    std::string strID;
    auto nodeDevice = root.child("DeviceID");
    if(!nodeDevice.empty())
    {
        strID = nodeDevice.text().as_string();
    }
    tUploadFini.strID = strID;
    /*会话ID*/
    auto nodeSessionID = root.child("SessionID");
    if(!nodeSessionID.empty())
    {
        tUploadFini.strSessionID =  nodeSessionID.text().as_string();
    }

    auto children = root.children("SnapShotList");
    for (auto &&child : children)
    {
        GB28181::SnapShotItem  Item;
        /*图像唯一标识*/
        Item.strSnapShotFileID = child.child("SnapShotFileID").text().as_int();
        tUploadFini.vecSnapShotList.push_back(Item);
    }


    /*TODO 巡航轨迹回调上抛*/
    { /* 配合上层实现业务功能 */
        SipCbResult_S stResult;
        auto fnCb = ::SipModule::instance()->GetUploadSnapShotFiniCb();
        if (fnCb)
        {
            fnCb(tUploadFini, stResult);
        }
    }

    return true;
}

bool BroadcastEvent::HandleResponse(const SipEvent::Ptr &e)
{
    ParseHeader(e);

    /* 先响应200，说明已接收到请求 */
    SendResponse(e, SIP_OK);

    GB28181::BroadcastInfo_S tBroadcast;

    auto root = m_doc.first_child();
    /*序列号*/
    tBroadcast.nSN = root.child("SN").text().as_int();
    /* 设备Id */
    std::string strID;
    auto nodeDevice = root.child("DeviceID");
    if(!nodeDevice.empty())
    {
        strID = nodeDevice.text().as_string();
    }
    tBroadcast.strID = strID;

    auto nodeResult = root.child("Result");
    if(!nodeResult.empty())
    {
        tBroadcast.strResult = nodeResult.text().as_string();
    }

    /*通道标号*/
    tBroadcast.nIndex = -1;  //通道下标，下级默认-1

    /*TODO 巡航轨迹回调上抛*/
    { /* 配合上层实现业务功能 */
        SipCbResult_S stResult;
        auto fnCb = ::SipModule::instance()->GetBroadcastCb();
        if (fnCb)
        {
            fnCb(tBroadcast, stResult);
        }
    }

    return true;
}

bool BroadcastEvent::HandleRequest(const SipEvent::Ptr &e)
{
    ParseHeader(e);

    /* 先响应200，说明已接收到请求 */
    SendResponse(e, SIP_OK);

     /*设备BasicParam配置控制设置*/ 
     GB28181::BroadcastInfo_S tBroadcast;

    auto root = m_doc.first_child();
    /*序列号*/
    uint64_t nSN = root.child("SN").text().as_int();

    std::string strSourceID;
    auto nodeSourceID = root.child("SourceID");
    if(!nodeSourceID.empty())
    {
        strSourceID = nodeSourceID.text().as_string();
    }

    tBroadcast.strSourceID= strSourceID;

    std::string strTargetID;
    auto nodeTargetID = root.child("TargetID");
    if(!nodeTargetID.empty())
    {
        strTargetID = nodeTargetID.text().as_string();
    }
    tBroadcast.strTargetID= strTargetID;

    //拼接返回报文
    pugi::xml_document stNewDoc;

    auto declaration = stNewDoc.append_child(pugi::node_declaration);
    auto attrVersion = declaration.append_attribute("version");
    attrVersion.set_value("1.0");
    auto attrEncoding = declaration.append_attribute("encoding");
    attrEncoding.set_value("GB18030");

    auto rootRet = stNewDoc.append_child("Response");

    auto nodeCmdType = rootRet.append_child("CmdType");
    nodeCmdType.text().set(m_header.strCmdType);

    auto nodeSN = rootRet.append_child("SN");
    nodeSN.text().set(nSN);

    auto nodeDeviceID = rootRet.append_child("DeviceID");
    nodeDeviceID.text().set(strTargetID);

    auto nodeResult = rootRet.append_child("Result");

    tBroadcast.strResult = "OK"; //默认OK
   
    auto pClient = dynamic_cast<SipClient *>(e->m_pNetBase);
    if (pClient == nullptr)
    {
        nodeResult.text().set("ERROR");
        std::ostringstream os;
        stNewDoc.save(os);
        auto strGB18030 = ::ToMbcsString(os.str());
        SendMessageWithCallID(e, strGB18030);

        dlog_warn("客户端实例为空，无法获取客户端的通道信息");
        return false;
    }
    pClient->setBroadcastTid(e->m_pEvent->tid);
    auto pChn = pClient->GetChannelByID(strTargetID);
    if (nullptr == pChn)
    {
        nodeResult.text().set("ERROR");
        std::ostringstream os;
        stNewDoc.save(os);
        auto strGB18030 = ::ToMbcsString(os.str());
        SendMessageWithCallID(e, strGB18030);

        dlog_warn("没有找到通道[%s]", m_header.strDevID.c_str());
        return false;
    }
    /*上级根据通道号赋值，下级设置成-1*/
    tBroadcast.nIndex = pChn->nIndex;


    /*TODO 回调上抛*/
    { /* 配合上层实现业务功能 */
        SipCbResult_S stResult;
        stResult.nResult = 0;
        auto fnCb = ::SipModule::instance()->GetBroadcastCb();
        if (fnCb)
        {
            fnCb(tBroadcast, stResult);
        }

        if(stResult.nResult != 0)
        {
            tBroadcast.strResult = "ERROR";
            dlog_warn("回调操作执行失败[%d]", stResult.nResult);
        }
    }

    nodeResult.text().set(tBroadcast.strResult);
    std::ostringstream os;
    stNewDoc.save(os);
    auto strGB18030 = ::ToMbcsString(os.str());
    SendMessageWithCallID(e, strGB18030);

     //发起invite请求
    {
        InviteAudioRequest::InviteParam stInvParam;

        SipClientInfo_S SipClient = e->m_pNetBase->GetSipClientInfo();

        //本地客户端的ID
        std::string localID = SipClient.stLocal.strID;
        /* 请求端的Uri */
        stInvParam.strFromUri = "sip:" + SipClient.stLocal.strID + "@" + SipClient.stLocal.strIP + ":" + std::to_string(SipClient.stLocal.nPort);
         /* 被请求端的Uri */
        stInvParam.strToUri = "sip:" + pChn->GetChannelID() + "@" + SipClient.stRemote.strIP + ":" + std::to_string(SipClient.stRemote.nPort);

        stInvParam.strSubject = pChn->GetChannelID() + ":" + pChn->GetDefaultSSRC() + "," + pChn->GetChannelID() + ":0";
        stInvParam.strChnID = pChn->GetChannelID();
        stInvParam.strLocalIP = SipClient.stLocal.strIP;//m_strExternIP;
        stInvParam.nLocalPort = pChn->GetPlayPort()+2;/* 广播端口 */
        stInvParam.strSSRC = pChn->GetDefaultSSRC();
        stInvParam.strStreamID = pChn->GetStreamID();
        /* 点播的请求 */
        stInvParam.enType = InviteAudioRequest::InviteType::Play;
        stInvParam.bUsingTcp = false;
    
        /* 发送INVITE请求 */
        auto request = std::make_shared<InviteAudioRequest>(e->m_pContext, stInvParam);
        request->SendCall();
    }
 
    return true;
}
