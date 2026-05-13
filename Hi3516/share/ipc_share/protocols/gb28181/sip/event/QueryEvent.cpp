/*
 * @Author       : EasonLu
 * @Date         : 2025-04-21 09:16:54
 * @LastEditors  : EasonLu
 * @LastEditTime : 2025-05-12 14:35:39
 * @FilePath     : QueryEvent.cpp
 * @Description  : 查询事件
 */
#include "QueryEvent.h"
#include "SipDevice.h"
#include "SipModule.h"
#include "SipUtils.h"
#include "time_manage.h"
#include <sstream>
#include <vector>

#define QUERY_EVENT_DEBUG 1
using namespace SIP;

bool SIP::QueryEvent::Handle(const SipEvent::Ptr &e)
{
    ParseHeader(e);
    /* 先响应200，说明已接收到请求 */
    SendResponse(e, SIP_OK);
    /* 根据二级指令进行相应 */
    switch (m_header.cmd_type)
    {
    case MANSCDP_QUERY_CMD_DEVICE_STATUS:
        return HandleDeviceStatus(e);
        break;
    case MANSCDP_QUERY_CMD_CATALOG:
        return HandleCatalog(e);
        break;
    case MANSCDP_QUERY_CMD_DEVICE_INFO:
        return HandleDeviceInfo(e);
        break;
    case MANSCDP_QUERY_CMD_RECORD_INFO:
        return HandleRecordInfo(e);
        break;
    case MANSCDP_QUERY_CMD_ALARM:

        break;
    case MANSCDP_QUERY_CMD_CONFIG_DOWNLOAD:
        MLOG_INFO("======HandleConfigDownload(e)=====");
        return HandleConfigDownload(e);
        break;
    case MANSCDP_QUERY_CMD_PRESET_QUERY:
        return HandlePresetQuery(e);
        break;
    case MANSCDP_QUERY_CMD_MOBILE_POSITION:

        break;
    case MANSCDP_QUERY_CMD_CRUISETRACKQUERY:
        return HandleCruiseTrackQuery(e);
        break;
    case MANSCDP_QUERY_CMD_CRUISETRACKLISTQUERY:
        return HandleCruiseTrackListQuery(e);
        break;
    case MANSCDP_QUERY_CMD_HOMEPOSITIONQUERY:
        return HandleHomePositionQuery(e);
        break;
    case MANSCDP_QUERY_CMD_PTZPOSITION:
        return HandlePTZPosition(e);
    default:
        break;
    }
    return true;
}

bool SIP::QueryEvent::HandleDeviceInfo(const SipEvent::Ptr &e)
{
    /* 再发送响应消息的Message */
    SipLocalInfo_S stLocalInfo;
    ::SipModule::instance()->GetLocalInfo(stLocalInfo);
    pugi::xml_document stNewDoc;

    auto declaration = stNewDoc.append_child(pugi::node_declaration);
    auto attrVersion = declaration.append_attribute("version");
    attrVersion.set_value("1.0");
    auto attrEncoding = declaration.append_attribute("encoding");
    attrEncoding.set_value("GB18030");

    auto root = stNewDoc.append_child("Response");

    auto nodeCmdType = root.append_child("CmdType");
    nodeCmdType.text().set("DeviceInfo");

    auto nodeSN = root.append_child("SN");
    nodeSN.text().set(m_header.strSN);

    auto nodeDeviceID = root.append_child("DeviceID");
    nodeDeviceID.text().set(m_header.strDevID);

    auto nodeResult = root.append_child("Result");
    nodeResult.text().set("OK");

    auto nodeDeviceName = root.append_child("DeviceName");
    nodeDeviceName.text().set(stLocalInfo.strDevName);

    auto nodeManufacturer = root.append_child("Manufacturer");
    nodeManufacturer.text().set(stLocalInfo.strManufacturer);

    auto nodeModel = root.append_child("Model");
    nodeModel.text().set(stLocalInfo.strModel);

    auto nodeFirmware = root.append_child("Firmware");
    nodeFirmware.text().set(stLocalInfo.strFirmware);

    auto nodeChnNum = root.append_child("Channel");
    nodeChnNum.text().set(1);

    std::ostringstream os;
    stNewDoc.save(os);
    auto strGB18030 = ::ToMbcsString(os.str());
    /* 响应时把From和To的URI进行转换 */
    SendMessageWithCallID(e, strGB18030);
    return true;
}

bool SIP::QueryEvent::HandleDeviceStatus(const SipEvent::Ptr &e)
{
    pugi::xml_document stNewDoc;

    auto declaration = stNewDoc.append_child(pugi::node_declaration);
    auto attrVersion = declaration.append_attribute("version");
    attrVersion.set_value("1.0");
    auto attrEncoding = declaration.append_attribute("encoding");
    attrEncoding.set_value("GB18030");

    auto root = stNewDoc.append_child("Response");

    auto nodeCmdType = root.append_child("CmdType");
    nodeCmdType.text().set("DeviceStatus");

    auto nodeSN = root.append_child("SN");
    nodeSN.text().set(m_header.strSN);

    auto nodeDeviceID = root.append_child("DeviceID");
    nodeDeviceID.text().set(m_header.strDevID);

    auto nodeResult = root.append_child("Result");
    nodeResult.text().set("OK");

    auto nodeOnline = root.append_child("Online");
    nodeOnline.text().set("ONLINE");

    auto nodeStatus = root.append_child("Status");
    nodeStatus.text().set("OK");
 
    std::string deviceTime = CTimeManage::instance()->get_device_time();
    dlog_info("deviceTime:%s",deviceTime.c_str());
    auto nodeDeviceTime = root.append_child("DeviceTime");
    nodeDeviceTime.text().set(deviceTime);

     /* 后期加上报警状态 */
    auto nodeAlarmstatus = root.append_child("Alarmstatus");
    nodeAlarmstatus.text().set(0);

    auto nodeEncode = root.append_child("Encode");
    nodeEncode.text().set("OK");

    /* 后期加上录像状态 */
    auto nodeRecord = root.append_child("Record");
    nodeRecord.text().set("OFF");

    std::ostringstream os;
    stNewDoc.save(os);
    auto strGB18030 = ::ToMbcsString(os.str());
    /* 响应时把From和To的URI进行转换 */
    SendMessageWithCallID(e, strGB18030);
    return true;
}

bool SIP::QueryEvent::HandleCatalog(const SipEvent::Ptr &e)
{
    /* 再发送响应消息的Message */
    std::vector<Channel::Ptr> vecChnList;
    auto pClient = dynamic_cast<SipClient *>(e->m_pNetBase);
    if (nullptr == pClient)
    {
        /* NOTE 只响应客户端的目录查询 */
        MLOG_WARN("客户端实例为空，无法获取客户端的目录信息");
        return false;
    }
    else
    {
        /* 客户端的事件 */
        pClient->GetChnInfo(vecChnList);
    }

    auto nChnTotal = vecChnList.size();
    MLOG_DEBUG("获取客户端的目录数量:[%d]", nChnTotal);
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
        SendMessageWithCallID(e, strGB18030);
    }

    return true;
}

bool SIP::QueryEvent::HandleRecordInfo(const SipEvent::Ptr &e)
{
    if (MANSCDP_CMD_CATEGORY_QUERY != m_header.cmd_category)
    {
        /* 只处理查询事件 */
        MLOG_WARN("不是查询事件[%d]", m_header.cmd_category);
        return false;
    }

    if (MANSCDP_QUERY_CMD_RECORD_INFO != m_header.cmd_type)
    {
        MLOG_WARN("不是录像文件检索事件[%d]", m_header.cmd_type);
        return false;
    }
    auto pClient = dynamic_cast<SipClient *>(e->m_pNetBase);
    if (pClient == nullptr)
    {
        MLOG_WARN("客户端实例为空，无法获取客户端的通道信息");
        return false;
    }
    auto pChn = pClient->m_pLocalDev->GetChannel(m_header.strDevID);
    if (nullptr == pChn)
    {
        MLOG_WARN("没有找到通道[%s]", m_header.strDevID.c_str());
        return false;
    }
    /* SN和设备ID已经解析在m_header中，只需解析开始时间和结束时间 */
    std::string strStartTime, strEndTime;

    auto root = m_doc.first_child();
    auto nodeStartTime = root.child("StartTime");
    if (!nodeStartTime.empty())
    {
        strStartTime = nodeStartTime.text().as_string();
    }
    auto nodeEndTime = root.child("EndTime");
    if (!nodeEndTime.empty())
    {
        strEndTime = nodeEndTime.text().as_string();
    }
#if QUERY_EVENT_DEBUG
    MLOG_INFO("[录制文件查询]SN:[%s]",
              m_header.strSN.c_str());
    MLOG_INFO("[录制文件查询]通道ID:[%s]",
              m_header.strDevID.c_str());
    MLOG_INFO("[录制文件查询]开始时间:[%s]",
              strStartTime.c_str());
    MLOG_INFO("[录制文件查询]结束时间:[%s]",
              strEndTime.c_str());
#endif

    { /* 配合上层实现业务功能 */
        SipQueryRecCondition_S stCondition;
        SipQueryRecResult_S stResult;
        stCondition.nIndex = pChn->nIndex;
        { /* 日期带T需要分离，且只取日期 */
            auto vecStart = ::SplitString(strStartTime, 'T');
            auto vecEnd = ::SplitString(strEndTime, 'T');
            if (vecStart.size() > 0)
            {
                stCondition.strStartDate = vecStart[0];
            }
            if (vecEnd.size() > 0)
            {
                stCondition.strEndDate = vecEnd[0];
            }
        }
        auto fnCb = ::SipModule::instance()->GetQueryRecordInfoCb();
        if (fnCb)
        {
            fnCb(stCondition, stResult);
        }
#if QUERY_EVENT_DEBUG
        MLOG_DEBUG("[录制文件查询]查询通道号[%d]结果[%d]",
                   pChn->nIndex, stResult.nResult);
        if (stResult.nResult > 0)
        {
            for (auto &item : stResult.vecResult)
            {
                MLOG_DEBUG("文件创建时间[%s],修改时间[%s],时长[%d]",
                           item.strCreateTime.c_str(),
                           item.strModifyTime.c_str(),
                           item.nDuration);
            }
        }
#endif
        { /* 组装发送数据 */
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

            auto nodeName = root.append_child("Name");
            nodeName.text().set(pChn->strName);

            auto nodeSumNum = root.append_child("SumNum");
            nodeSumNum.text().set(std::to_string(stResult.nResult));
            /* 没数据也发一个消息 */
            if (stResult.nResult == 0)
            {
                std::ostringstream os;
                stNewDoc.save(os);
                auto strGB18030 = ::ToMbcsString(os.str());
                SendMessageWithCallID(e, strGB18030);
                return true;
            }

            /* 每次发送一个文件信息 */
            auto nodeRecList = root.append_child("RecordList");

            nodeRecList.append_attribute("Num").set_value(std::to_string(1));

            /* 先设置好字段——固定字段的可以先填写数据 */
            auto nodeItem = nodeRecList.append_child("Item");
            nodeItem.append_child("DeviceID");
            nodeItem.child("DeviceID").text().set(m_header.strDevID);

            nodeItem.append_child("Name");
            nodeItem.child("Name").text().set(pChn->strName);

            nodeItem.append_child("FilePath");
            nodeItem.append_child("StartTime");
            nodeItem.append_child("EndTime");
            nodeItem.append_child("Secrecy");
            nodeItem.append_child("Type");

            /* 填充好差异化的数据后则直接发送 */
            for (auto &item : stResult.vecResult)
            {
                /* 换算为秒数 */
                auto lStartTime = ::UnixToTime(item.strCreateTime);
                /* 叠加时长或直接使用修改时间作为结束时间 */
                // auto lEndTime = lStartTime + item.nDuration;
                auto lEndTime = ::UnixToTime(item.strModifyTime);
                /* 更换为ISO格式 */
                auto itemStartTime = ::TimeTToISO8601(lStartTime);
                auto itemEndTime = ::TimeTToISO8601(lEndTime);
#if QUERY_EVENT_DEBUG
                MLOG_DEBUG("开始时间:[%s][%lld]", itemStartTime.c_str(), lStartTime);
                MLOG_DEBUG("结束时间:[%s][%lld]", itemEndTime.c_str(), lEndTime);
#endif
                /* 拼接FilePath */
                auto strFilePath = std::to_string(lStartTime) + "_" + std::to_string(lEndTime);
                nodeItem.child("FilePath").text().set(strFilePath);
                nodeItem.child("StartTime").text().set(itemStartTime);
                nodeItem.child("EndTime").text().set(itemEndTime);
                /* FIXME 暂定都是公开 */
                nodeItem.child("Secrecy").text().set(std::to_string(0));
                /* FIXME 暂定都是计划录制 */
                nodeItem.child("Type").text().set("time");
                std::ostringstream os;
                stNewDoc.save(os);
                auto strGB18030 = ::ToMbcsString(os.str());
                SendMessageWithCallID(e, strGB18030);
            }
        }
    }
    return true;
}

bool SIP::QueryEvent::ResponeControlResult(const SipEvent::Ptr &e, uint64_t nSN, std::string strCmdType, std::string strDeviceID, std::string strResult)
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

bool SIP::QueryEvent::HandleConfigDownload(const SipEvent::Ptr &e)
{
    ParseHeader(e);
    auto root = m_doc.first_child();
    
    auto configTypeNode = root.child("ConfigType");
    if (configTypeNode)
    {
        std::string strValue = configTypeNode.text().as_string();
        MLOG_INFO("======configTypeValue===%s==",strValue.c_str());
        if(strValue == "BasicParam")
        {
            return HandleBasicParamRequest(e);
        }
        else if(strValue == "VideoParamOpt")
        {
            return HandleVideoParOptRequest(e);
        }
        else if(strValue == "SVACEncodeConfig")
        {
            return HandleSVACEnConfRequest(e);
        }
        else if(strValue == "SVACDncodeConfig")
        {
            return HandleSVACDeConfRequest(e);
        }
        else if(strValue == "VideoParamAttribute")
        {
            return HandleVideoParAttrRequst(e);
        }
        else if(strValue == "VideoRecordPlan")
        {
            return HandleVideoRecPlanRequst(e);
        }
        else if(strValue == "VideoAlarmRecord")
        {
            return HandleVideoAlarmRequest(e);
        }
        else if(strValue == "PictureMask")
        {
            return HandlePictureMaskRequst(e);
        }
        else if(strValue == "FrameMirror")
        {
            return HandleFrameMirrorRequst(e);
        }
        else if(strValue == "AlarmReport")
        {
            return HandleAlarmReportRequst(e);
        }
        else if(strValue == "OSDConfig")
        {
            return HandleOSDConfigRequest(e);
        }
        else if(strValue == "SnapShotConfig")
        {
            return HandleSnapShotConfig(e);
        }
    }

    //if(!root.child("BasicParam").empty())
    //{
    //    MLOG_INFO("======HandleBasicParamReques=====");
    //    return HandleBasicParamRequest(e);
    //}
    //else if(!root.child("VideoParamOpt").empty())
    //{
    //    return HandleVideoParOptRequest(e);
    //}
    //else if(!root.child("SVACEncodeConfig").empty())
    //{
    //    return HandleSVACEnConfRequest(e);
    //}
    //else if(!root.child("SVACDncodeConfig").empty())
    //{
    //    return HandleSVACDeConfRequest(e);
    //}
    //else if(!root.child("VideoParamAttribute").empty())
    //{
    //    return HandleVideoParAttrRequst(e);
    //}
    //else if(!root.child("VideoRecordPlan").empty())
    //{
    //    return HandleVideoRecPlanRequst(e);
    //}
    //else if(!root.child("VideoAlarmRecord").empty())
    //{
    //    return HandleVideoAlarmRequest(e);
    //}
    //else if(!root.child("PictureMask").empty())
    //{
    //    return HandlePictureMaskRequst(e);
    //}
    //else if(!root.child("FrameMirror").empty())
    //{
    //    return HandleFrameMirrorRequst(e);
    //}
    //else if(!root.child("AlarmReport").empty())
    //{
    //    return HandleAlarmReportRequst(e);
    //}
    //else if(!root.child("OSDConfig").empty())
    //{
    //    return HandleOSDConfigRequest(e);
    //}
    //else if(!root.child("SnapShotConfig").empty())
    //{
    //    return HandleSnapShotConfig(e);
    //}
    MLOG_INFO("=============null==========");
    return true;
}

bool  SIP::QueryEvent::HandleBasicParamRequest(const SipEvent::Ptr &e)
{
    ParseHeader(e);
    GB28181::BasicParamInfo_S tBasicParam;

    auto root = m_doc.first_child();
    /*序列号*/
    int nSN = root.child("SN").text().as_int();
    tBasicParam.nSN = nSN;
    /* 设备Id */
    std::string strID = root.child("DeviceID").text().as_string();
    tBasicParam.strID = strID;
    /* 只获取当前设备----注释掉通道 */
    //auto pClient = dynamic_cast<SipClient *>(e->m_pNetBase);
    //if (pClient == nullptr)
    //{
    //    ResponeControlResult(e, nSN, m_header.strCmdType, strID, "ERROR");
    //    MLOG_WARN("客户端实例为空，无法获取客户端的通道信息");
    //    return false;
    //}
    //auto pChn = pClient->GetChannelByID(strID);
    //if (nullptr == pChn)
    //{
    //    ResponeControlResult(e, nSN, m_header.strCmdType, strID, "ERROR");
    //    MLOG_WARN("没有找到通道[%s] 默认当前设备", m_header.strDevID.c_str());
    //    pChn->nIndex = -1;
    //}
    /*上级根据通道号赋值，下级设置成-1*/
    tBasicParam.nIndex = -1;
    /*true-设备配置设置，false-设备配置获取*/
    tBasicParam.bIsSet = false;

    /*TODO 设备配置上层回调*/
     /* 配合上层实现业务功能 */
     SipCbResult_S stResult;
     {
        auto fnCb = ::SipModule::instance()->GetBasicParamCb();
        if (fnCb)
        {
            fnCb(tBasicParam, stResult);
        }
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

    auto nodeResult = rootRet.append_child("Result");
    nodeResult.text().set(tBasicParam.strResult);

    if(stResult.nResult == 0)
    {
        auto nodeBasic = rootRet.append_child("BasicParam");

        auto nodeName = nodeBasic.append_child("Name");
        nodeName.text().set(tBasicParam.m_szName);
        auto nodeExpiration = nodeBasic.append_child("Expiration");
        nodeExpiration.text().set(tBasicParam.m_nExpiration);
        auto nodeInterval = nodeBasic.append_child("HeartBeatInterval");
        nodeInterval.text().set(tBasicParam.m_nHeartBeatInterval);
        auto nodeCount = nodeBasic.append_child("HeartBeatCount");
        nodeCount.text().set(tBasicParam.m_nHeartBeatCount);
    }
    else
    {
        ResponeControlResult(e, nSN, m_header.strCmdType, strID, "ERROR");
        MLOG_WARN("回调操作执行失败[%d]", stResult.nResult);
    }

    std::ostringstream os;
    stNewDoc.save(os);
    auto strGB18030 = ::ToMbcsString(os.str());
    SendMessageWithCallID(e, strGB18030);

    return true;
}

bool SIP::QueryEvent::HandleVideoParOptRequest(const SipEvent::Ptr &e)
{
    ParseHeader(e);

    GB28181::VideoParamOptInfo_S tParamOpt;

    auto root = m_doc.first_child();
    /*序列号*/
    int nSN = root.child("SN").text().as_int();
    tParamOpt.nSN = nSN;
    /* 设备Id */
    std::string strID = root.child("DeviceID").text().as_string();
    tParamOpt.strID = strID;

    auto pClient = dynamic_cast<SipClient *>(e->m_pNetBase);
    if (pClient == nullptr)
    {
        ResponeControlResult(e, nSN, m_header.strCmdType, strID, "ERROR");
        MLOG_WARN("客户端实例为空，无法获取客户端的通道信息");
        return false;
    }
    auto pChn = pClient->GetChannelByID(strID);
    if (nullptr == pChn)
    {
        ResponeControlResult(e, nSN, m_header.strCmdType, strID, "ERROR");
        MLOG_WARN("没有找到通道[%s]", m_header.strDevID.c_str());
        return false;
    }
    /*上级根据通道号赋值，下级设置成-1*/
    tParamOpt.nIndex = pChn->nIndex;
    /*true-设备配置设置，false-设备配置获取*/
    tParamOpt.bIsSet = false;

    /*TODO 设备配置上层回调*/
    SipCbResult_S stResult;
    { /* 配合上层实现业务功能 */
        auto fnCb = ::SipModule::instance()->GetVideoParamOptCb();
        if (fnCb)
        {
            fnCb(tParamOpt, stResult);
        }
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

    auto nodeResult = rootRet.append_child("Result");
    nodeResult.text().set(tParamOpt.strResult);

    if(stResult.nResult == 0)
    {
        auto nodeParOpt = rootRet.append_child("VideoParamOpt");

        auto nodeSpeed = nodeParOpt.append_child("DownloadSpeed");
        nodeSpeed.text().set(tParamOpt.strDownloadSpeed);
        auto nodeResolution = nodeParOpt.append_child("Resolution");
        nodeResolution.text().set(tParamOpt.strResolution);

    }
    else
    {
        ResponeControlResult(e, nSN, m_header.strCmdType, strID, "ERROR");
        MLOG_WARN("回调操作执行失败[%d]", stResult.nResult);
    }

    std::ostringstream os;
    stNewDoc.save(os);
    auto strGB18030 = ::ToMbcsString(os.str());
    SendMessageWithCallID(e, strGB18030);

    return true;

}

bool SIP::QueryEvent::HandleSVACEnConfRequest(const SipEvent::Ptr &e)
{
    ParseHeader(e);

    GB28181::SVACEncodeInfo_S tSVACEn;

    auto root = m_doc.first_child();
    /*序列号*/
    int nSN = root.child("SN").text().as_int();
    tSVACEn.nSN = nSN;
    /* 设备Id */
    std::string strID = root.child("DeviceID").text().as_string();
    tSVACEn.strID = strID;

    auto pClient = dynamic_cast<SipClient *>(e->m_pNetBase);
    if (pClient == nullptr)
    {
        ResponeControlResult(e, nSN, m_header.strCmdType, strID, "ERROR");
        MLOG_WARN("客户端实例为空，无法获取客户端的通道信息");
        return false;
    }
    auto pChn = pClient->GetChannelByID(strID);
    if (nullptr == pChn)
    {
        ResponeControlResult(e, nSN, m_header.strCmdType, strID, "ERROR");
        MLOG_WARN("没有找到通道[%s]", m_header.strDevID.c_str());
        return false;
    }
    /*上级根据通道号赋值，下级设置成-1*/
    tSVACEn.nIndex = pChn->nIndex;
    /*true-设备配置设置，false-设备配置获取*/
    tSVACEn.bIsSet = false;

    /*TODO 设备配置上层回调*/
    SipCbResult_S stResult;
    { /* 配合上层实现业务功能 */
        auto fnCb = ::SipModule::instance()->GetSVACEncodeCb();
        if (fnCb)
        {
            fnCb(tSVACEn, stResult);
        }
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

    auto nodeResult = rootRet.append_child("Result");
    nodeResult.text().set(tSVACEn.strResult);

    if(stResult.nResult == 0)
    {
        auto nodeSVACEn = rootRet.append_child("SVACEncodeConfig");

        //感兴趣区域参数
        auto nodeROI = nodeSVACEn.append_child("ROIParam");
        //区域开关
        auto nodeROIFlag = nodeROI.append_child("ROIFlag");
        nodeROIFlag.text().set(tSVACEn.tROIParam.nROIFlag);
        //区域数量 
        auto nodeROINumber= nodeROI.append_child("ROINumber");
        nodeROINumber.text().set(tSVACEn.tROIParam.nROINumber);

        //感兴趣区域
        auto nodeItem = nodeROI.append_child("Item");
        nodeItem.append_child("ROISeq");
        nodeItem.append_child("TopLeft");
        nodeItem.append_child("BottomRight");
        nodeItem.append_child("ROIQP");

        
        
        //SVC参数
        {
            if(tSVACEn.tSVCParam.bIsHave)
            {
                auto nodeSVC = nodeSVACEn.append_child("SVCParam");

                //空域编码方式
                auto SVCSpaceDomainMode = nodeSVC.append_child("SVCSpaceDomainMode");
                SVCSpaceDomainMode.text().set( tSVACEn.tSVCParam.nSVCSpaceDomainMode);
                //时域编码方式
                auto SVCTimeDomainMode = nodeSVC.append_child("SVCTimeDomainMode");
                SVCTimeDomainMode.text().set(tSVACEn.tSVCParam.nSVCTimeDomainMode);
                //比例值
                auto SSVCRatioValue = nodeSVC.append_child("SSVCRatioValue");
                SSVCRatioValue.text().set(tSVACEn.tSVCParam.strSSVCRatioValue);
                //空域编码能力
                auto SVCSpaceSupportMode = nodeSVC.append_child("SVCSpaceSupportMode");
                SVCSpaceSupportMode.text().set(tSVACEn.tSVCParam.nSVCSpaceSupportMode);
                //时域编码能力
                auto SVCTimeSupportMode = nodeSVC.append_child("SVCTimeSupportMode");
                SVCTimeSupportMode.text().set(tSVACEn.tSVCParam.nSVCTimeSupportMode);

                //比例能力
                auto SSVCRatioSupportList = nodeSVC.append_child("SSVCRatioSupportList");
                SSVCRatioSupportList.text().set(tSVACEn.tSVCParam.strSSVCRatioSupportList);
            }
        }
        //监控专用信息参数
        {
            if(tSVACEn.tSurveillanceParam.isHave)
            {
                auto nodeSurveillance = nodeSVACEn.append_child("SurveillanceParam");

                //时间信息开关
                auto TimeFlag = nodeSurveillance.append_child("TimeFlag");
                TimeFlag.text().set(tSVACEn.tSurveillanceParam.nTimeFlag);
                auto OSDFlag = nodeSurveillance.append_child("OSDFlag");
                OSDFlag.text().set(tSVACEn.tSurveillanceParam.nOSDFlag);  
                auto AIFlag = nodeSurveillance.append_child("AIFlag");
                AIFlag.text().set(tSVACEn.tSurveillanceParam.nAIFlag);
                auto GISFlag = nodeSurveillance.append_child("GISFlag");
                GISFlag.text().set(tSVACEn.tSurveillanceParam.nGISFlag);
            }
        }
        //音频参数
        {
            if(tSVACEn.tAudioParam.bIsHave)
            {
                auto nodeAudio = nodeSVACEn.append_child("AudioParam");
                //时间信息开关
                auto Flag = nodeAudio.append_child("AudioRecognitionFlag");
                Flag.text().set(tSVACEn.tAudioParam.nAudioRecognitionFlag);
            }
        }

        if(tSVACEn.tROIParam.vecROIParamItem.size() > 0)
        {
            for (auto &Item : tSVACEn.tROIParam.vecROIParamItem)
            {
                nodeItem.child("ROISeq").text().set(Item.nROISeq);
                nodeItem.child("TopLeft").text().set(Item.nTopLeft);
                nodeItem.child("BottomRight").text().set(Item.nBottomRight);
                nodeItem.child("ROIQP").text().set(Item.nROIQP);
    
                std::ostringstream os;
                stNewDoc.save(os);
                auto strGB18030 = ::ToMbcsString(os.str());
                SendMessageWithCallID(e, strGB18030);
            }
        }
        else
        {
            std::ostringstream os;
            stNewDoc.save(os);
            auto strGB18030 = ::ToMbcsString(os.str());
            SendMessageWithCallID(e, strGB18030);
        }
    }
    else
    {
        ResponeControlResult(e, nSN, m_header.strCmdType, strID, "ERROR");
        MLOG_WARN("回调操作执行失败[%d]", stResult.nResult);
    }

    return true;
}

bool SIP::QueryEvent::HandleSVACDeConfRequest(const SipEvent::Ptr &e)
{
    ParseHeader(e);

    GB28181::SVACDecodeInfo_S tSVACDe;

    auto root = m_doc.first_child();
    /*序列号*/
    int nSN = root.child("SN").text().as_int();
    tSVACDe.nSN = nSN;
    /* 设备Id */
    std::string strID = root.child("DeviceID").text().as_string();
    tSVACDe.strID = strID;
   
    auto pClient = dynamic_cast<SipClient *>(e->m_pNetBase);
    if (pClient == nullptr)
    {
        ResponeControlResult(e, nSN, m_header.strCmdType, strID, "ERROR");
        MLOG_WARN("客户端实例为空，无法获取客户端的通道信息");
        return false;
    }
    auto pChn = pClient->GetChannelByID(strID);
    if (nullptr == pChn)
    {
        ResponeControlResult(e, nSN, m_header.strCmdType, strID, "ERROR");
        MLOG_WARN("没有找到通道[%s]", m_header.strDevID.c_str());
        return false;
    }
    /*上级根据通道号赋值，下级设置成-1*/
    tSVACDe.nIndex = pChn->nIndex;
    /*true-设备配置设置，false-设备配置获取*/
    tSVACDe.bIsSet = false;

    /*TODO 设备配置上层回调*/
    SipCbResult_S stResult;
    { /* 配合上层实现业务功能 */
        auto fnCb = ::SipModule::instance()->GetSVACDncodeCb();
        if (fnCb)
        {
            fnCb(tSVACDe, stResult);
        }
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

    auto nodeResult = rootRet.append_child("Result");
    nodeResult.text().set(tSVACDe.strResult);

    if(stResult.nResult == 0)
    {
        auto nodeSVACEn = rootRet.append_child("SVACDecodeConfig");

        //SVC参数
        {
            if(tSVACDe.tSVCParam.bIsHave)
            {
                auto nodeSVC = nodeSVACEn.append_child("SVCParam");
    
                //空域编码方式
                auto SVCSTMMode = nodeSVC.append_child("SVCSTMMode");
                SVCSTMMode.text().set(tSVACDe.tSVCParam.nSVCSTMMode);
                //空域编码能力
                auto SVCSpaceSupportMode = nodeSVC.append_child("SVCSpaceSupportMode");
                SVCSpaceSupportMode.text().set(tSVACDe.tSVCParam.nSVCSpaceSupportMode);
                //时域编码能力
                auto SVCTimeSupportMode = nodeSVC.append_child("SVCTimeSupportMode");
                SVCTimeSupportMode.text().set(tSVACDe.tSVCParam.nSVCTimeSupportMode);
            }
        }
        //监控专用信息参数
        {
            if(tSVACDe.tSurveillanceParam.isHave)
            {
                auto nodeSurveillance = nodeSVACEn.append_child("SurveillanceParam");
    
                //时间信息开关
                auto TimeShowFlag = nodeSurveillance.append_child("TimeShowFlag");
                TimeShowFlag.text().set(tSVACDe.tSurveillanceParam.nAIShowFlag);
                auto OSDShowFlag = nodeSurveillance.append_child("OSDShowFlag");
                OSDShowFlag.text().set(tSVACDe.tSurveillanceParam.nOSDShowFlag);  
                auto AIShowFlag = nodeSurveillance.append_child("AIShowFlag");
                AIShowFlag.text().set(tSVACDe.tSurveillanceParam.nAIShowFlag);
                auto GISShowFlag = nodeSurveillance.append_child("GISShowFlag");
                GISShowFlag.text().set(tSVACDe.tSurveillanceParam.nGISShowFlag);
            }
        }

        std::ostringstream os;
        stNewDoc.save(os);
        auto strGB18030 = ::ToMbcsString(os.str());
        SendMessageWithCallID(e, strGB18030);
    }
    else
    {
        ResponeControlResult(e, nSN, m_header.strCmdType, strID, "ERROR");
        MLOG_WARN("回调操作执行失败[%d]", stResult.nResult);
    }

    
    return true;
}

bool SIP::QueryEvent::HandleVideoParAttrRequst(const SipEvent::Ptr &e)
{
    ParseHeader(e);

    GB28181::VideoParamAttributeInfo_S tAttribute;

    auto root = m_doc.first_child();
    /*序列号*/
    int nSN = root.child("SN").text().as_int();
    tAttribute.nSN = nSN;
    /* 设备Id */
    std::string strID = root.child("DeviceID").text().as_string();
    tAttribute.strID = strID;

    auto pClient = dynamic_cast<SipClient *>(e->m_pNetBase);
    if (pClient == nullptr)
    {
        ResponeControlResult(e, nSN, m_header.strCmdType, strID, "ERROR");
        MLOG_WARN("客户端实例为空，无法获取客户端的通道信息");
        return false;
    }
    auto pChn = pClient->GetChannelByID(strID);
    if (nullptr == pChn)
    {
        ResponeControlResult(e, nSN, m_header.strCmdType, strID, "ERROR");
        MLOG_WARN("没有找到通道[%s]", m_header.strDevID.c_str());
        return false;
    }
    /*上级根据通道号赋值，下级设置成-1*/
    tAttribute.nIndex = pChn->nIndex;
    /*true-设备配置设置，false-设备配置获取*/
    tAttribute.bIsSet = false;

    /*TODO 设备配置上层回调*/
    SipCbResult_S stResult;
    { /* 配合上层实现业务功能 */
        auto fnCb = ::SipModule::instance()->GetVideoAttributeCb();
        if (fnCb)
        {
            fnCb(tAttribute, stResult);
        }
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

    auto nodeResult = rootRet.append_child("Result");
    nodeResult.text().set(tAttribute.strResult);

    if(stResult.nResult == 0)
    {
        auto nodeAttribute = rootRet.append_child("VideoParamAttribute");
        nodeAttribute.append_attribute("Num").set_value(std::to_string(2));

        auto nodeItem0 = nodeAttribute.append_child("Item");
        nodeItem0.append_child("StreamNumber");
        nodeItem0.append_child("VideoFormat");
        nodeItem0.append_child("Resolution");
        nodeItem0.append_child("FrameRate");
        nodeItem0.append_child("BitRateType");
        nodeItem0.append_child("VideoBitRate");

        auto nodeItem1 = nodeAttribute.append_child("Item");
        nodeItem1.append_child("StreamNumber");
        nodeItem1.append_child("VideoFormat");
        nodeItem1.append_child("Resolution");
        nodeItem1.append_child("FrameRate");
        nodeItem1.append_child("BitRateType");
        nodeItem1.append_child("VideoBitRate");

        if(tAttribute.vecVideoParAttrItem.size() > 0)
        {
            for (auto &Item : tAttribute.vecVideoParAttrItem)
            {
                if(Item.nStreamNumber == 0)
                {
                    //码流编号
                    nodeItem0.child("StreamNumber").text().set(Item.nStreamNumber);
                    //视频编码格式
                    nodeItem0.child("VideoFormat").text().set( static_cast<int>(Item.enVideoFormat));
                    //分辨率
                    nodeItem0.child("Resolution").text().set( static_cast<int>(Item.enResolution));
                    //帧率
                    nodeItem0.child("FrameRate").text().set(Item.strFrameRate);
                    //码率类型
                    nodeItem0.child("BitRateType").text().set( static_cast<int>(Item.enBitRateType));
                    //视频码率
                    nodeItem0.child("VideoBitRate").text().set(Item.strVideoBitRate);
                }
                else
                {
                     //码流编号
                    nodeItem1.child("StreamNumber").text().set(Item.nStreamNumber);
                    //视频编码格式
                    nodeItem1.child("VideoFormat").text().set( static_cast<int>(Item.enVideoFormat));
                    //分辨率
                    nodeItem1.child("Resolution").text().set( static_cast<int>(Item.enResolution));
                    //帧率
                    nodeItem1.child("FrameRate").text().set(Item.strFrameRate);
                    //码率类型
                    nodeItem1.child("BitRateType").text().set( static_cast<int>(Item.enBitRateType));
                    //视频码率
                    nodeItem1.child("VideoBitRate").text().set(Item.strVideoBitRate);
                }
            }
            std::ostringstream os;
            stNewDoc.save(os);
            auto strGB18030 = ::ToMbcsString(os.str());
            SendMessageWithCallID(e, strGB18030);
        }
        else
        {
            std::ostringstream os;
            stNewDoc.save(os);
            auto strGB18030 = ::ToMbcsString(os.str());
            SendMessageWithCallID(e, strGB18030);
        }
    }
    else
    {
        ResponeControlResult(e, nSN, m_header.strCmdType, strID, "ERROR");
        MLOG_WARN("回调操作执行失败[%d]", stResult.nResult);
    }

    return true;
}

bool SIP::QueryEvent::HandleVideoRecPlanRequst(const SipEvent::Ptr &e)
{
    ParseHeader(e);

    GB28181::VideoRecordPlanInfo_S tRecordPlan;

    auto root = m_doc.first_child();
    /*序列号*/
    int nSN = root.child("SN").text().as_int();
    tRecordPlan.nSN = nSN;
    /* 设备Id */
    std::string strID = root.child("DeviceID").text().as_string();
    tRecordPlan.strID = strID;

    auto pClient = dynamic_cast<SipClient *>(e->m_pNetBase);
    if (pClient == nullptr)
    {
        ResponeControlResult(e, nSN, m_header.strCmdType, strID, "ERROR");
        MLOG_WARN("客户端实例为空，无法获取客户端的通道信息");
        return false;
    }
    auto pChn = pClient->GetChannelByID(strID);
    if (nullptr == pChn)
    {
        ResponeControlResult(e, nSN, m_header.strCmdType, strID, "ERROR");
        MLOG_WARN("没有找到通道[%s]", m_header.strDevID.c_str());
        return false;
    }
    /*上级根据通道号赋值，下级设置成-1*/
    tRecordPlan.nIndex = pChn->nIndex;
    /*true-设备配置设置，false-设备配置获取*/
    tRecordPlan.bIsSet = false;

    /*TODO 设备配置上层回调*/
    SipCbResult_S stResult;
    { /* 配合上层实现业务功能 */
        auto fnCb = ::SipModule::instance()->GetRecordPlanCb();
        if (fnCb)
        {
            fnCb(tRecordPlan, stResult);
        }
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

    auto nodeResult = rootRet.append_child("Result");
    nodeResult.text().set(tRecordPlan.strResult);

    if(stResult.nResult == 0)
    {
        auto node = rootRet.append_child("VideoRecordPlan");

        /*录像启用,0-否，1-是*/
        auto nodeEnable = node.append_child("RecordEnable");
        nodeEnable.text().set(tRecordPlan.nRecordEnable);
        /*录像计划总天数*/
        auto nodeSum= node.append_child("RecordScheduleSumNum");
        nodeSum.text().set(tRecordPlan.nRecordScheduleSumNum);
        /*码流类型*/
        auto nodeStreamNumber = node.append_child("StreamNumber");  
        nodeStreamNumber.text().set(tRecordPlan.nStreamNumber);

        for (auto ScheduleItem : tRecordPlan.vecRecordSchedule)
        {
            //每一天的计划
            auto nodeSchedItem = node.append_child("RecordSchedule");
            nodeSchedItem.append_child("WeekDayNum");
            nodeSchedItem.append_child("TimeSegmentSumNum");
            auto nodeItem = nodeSchedItem.append_child("TimeSegment");

            //周几
            nodeSchedItem.child("WeekDayNum").text().set(ScheduleItem.second.nWeekDayNum);
            //每天录像计划时间段总数
            nodeSchedItem.child("TimeSegmentSumNum").text().set(ScheduleItem.second.nTimeSegmentSumNum);

            if(ScheduleItem.second.vecTimeSegment.size() > 0)
            {
                for (auto SegmentItem : ScheduleItem.second.vecTimeSegment)
                {
                     nodeItem.append_child("StartHour");
                    nodeItem.append_child("StartMin");
                    nodeItem.append_child("StartSec");
                    nodeItem.append_child("StopHour");
                    nodeItem.append_child("nStopMin");
                    nodeItem.append_child("StopSec");
                    /*开始时间时*/
                    nodeItem.child("StartHour").text().set(SegmentItem.nStartHour);
                    /*开始时间分*/
                    nodeItem.child("StartMin").text().set(SegmentItem.nStartMin);
                    /*开始时间秒*/
                    nodeItem.child("StartSec").text().set(SegmentItem.nStartSec);
                    /*结束时间时*/
                    nodeItem.child("StopHour").text().set(SegmentItem.nStopHour);
                    /*结束时间分*/
                    nodeItem.child("nStopMin").text().set(SegmentItem.nStopMin);
                    /*结束时间秒*/
                    nodeItem.child("StopSec").text().set(SegmentItem.nStopSec);
                }
            }
        }

        //发送
        std::ostringstream os;
        stNewDoc.save(os);
        auto strGB18030 = ::ToMbcsString(os.str());
        SendMessageWithCallID(e, strGB18030);
    }
    else
    {
        ResponeControlResult(e, nSN, m_header.strCmdType, strID, "ERROR");
        MLOG_WARN("回调操作执行失败[%d]", stResult.nResult);
    }

    return true;
}

bool SIP::QueryEvent::HandleVideoAlarmRequest(const SipEvent::Ptr &e)
{
    ParseHeader(e);

    GB28181::VideoAlarmRecordInfo_S tAlarmRecord;

    auto root = m_doc.first_child();
    /*序列号*/
    int nSN = root.child("SN").text().as_int();
    tAlarmRecord.nSN = nSN;
    /* 设备Id */
    std::string strID = root.child("DeviceID").text().as_string();
    tAlarmRecord.strID = strID;

    auto pClient = dynamic_cast<SipClient *>(e->m_pNetBase);
    if (pClient == nullptr)
    {
        ResponeControlResult(e, nSN, m_header.strCmdType, strID, "ERROR");
        MLOG_WARN("客户端实例为空，无法获取客户端的通道信息");
        return false;
    }
    auto pChn = pClient->GetChannelByID(strID);
    if (nullptr == pChn)
    {
        ResponeControlResult(e, nSN, m_header.strCmdType, strID, "ERROR");
        MLOG_WARN("没有找到通道[%s]", m_header.strDevID.c_str());
        return false;
    }
    /*上级根据通道号赋值，下级设置成-1*/
    tAlarmRecord.nIndex = pChn->nIndex;
    /*true-设备配置设置，false-设备配置获取*/
    tAlarmRecord.bIsSet = false;

    /*TODO 设备配置上层回调*/
    SipCbResult_S stResult;
    { /* 配合上层实现业务功能 */
        auto fnCb = ::SipModule::instance()->GetAlarmRecordCb();
        if (fnCb)
        {
            fnCb(tAlarmRecord, stResult);
        }
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

    auto nodeResult = rootRet.append_child("Result");
    nodeResult.text().set(tAlarmRecord.strResult);

    if(stResult.nResult == 0)
    {
        auto node = rootRet.append_child("VideoAlarmRecord");

        auto nodedEnable = node.append_child("RecordEnable");
        nodedEnable.text().set(tAlarmRecord.nRecordEnable);

        auto nodeRecord= node.append_child("RecordTime");
        nodeRecord.text().set(tAlarmRecord.nRecordTime);

        auto nodePre = node.append_child("PreRecordTime");
        nodePre.text().set(tAlarmRecord.nPreRecordTime);

        auto nodeStream = node.append_child("StreamNumber");
        nodeStream.text().set(tAlarmRecord.nStreamNumber);

        std::ostringstream os;
        stNewDoc.save(os);
        auto strGB18030 = ::ToMbcsString(os.str());
        SendMessageWithCallID(e, strGB18030);
    }
    else
    {
        ResponeControlResult(e, nSN, m_header.strCmdType, strID, "ERROR");
        MLOG_WARN("回调操作执行失败[%d]", stResult.nResult);
    }

    return true;
}

bool SIP::QueryEvent::HandlePictureMaskRequst(const SipEvent::Ptr &e)
{
    ParseHeader(e);

    GB28181::PictureMaskInfo_S tPictureMask;

    auto root = m_doc.first_child();
    /*序列号*/
    int nSN = root.child("SN").text().as_int();
    tPictureMask.nSN = nSN;
    /* 设备Id */
    std::string strID = root.child("DeviceID").text().as_string();
    tPictureMask.strID = strID;

    auto pClient = dynamic_cast<SipClient *>(e->m_pNetBase);
    if (pClient == nullptr)
    {
        ResponeControlResult(e, nSN, m_header.strCmdType, strID, "ERROR");
        MLOG_WARN("客户端实例为空，无法获取客户端的通道信息");
        return false;
    }
    auto pChn = pClient->GetChannelByID(strID);
    if (nullptr == pChn)
    {
        ResponeControlResult(e, nSN, m_header.strCmdType, strID, "ERROR");
        MLOG_WARN("没有找到通道[%s]", m_header.strDevID.c_str());
        return false;
    }
    /*上级根据通道号赋值，下级设置成-1*/
    tPictureMask.nIndex = pChn->nIndex;
    /*true-设备配置设置，false-设备配置获取*/
    tPictureMask.bIsSet = false;

    /*TODO 设备配置上层回调*/
    SipCbResult_S stResult;
    { /* 配合上层实现业务功能 */
        auto fnCb = ::SipModule::instance()->GetPictureMaskCb();
        if (fnCb)
        {
            fnCb(tPictureMask, stResult);
        }
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

    auto nodeResult = rootRet.append_child("Result");
    nodeResult.text().set(tPictureMask.strResult);

    if(stResult.nResult == 0)
    {
        auto node = rootRet.append_child("PictureMask");

        auto nodeOn = node.append_child("On");
        nodeOn.text().set(tPictureMask.On);
        auto nodeSum = node.append_child("SumNum");
        nodeSum.text().set(tPictureMask.SumNum);

        auto nodeRegionList = node.append_child("RegionList");

        auto nodeItem = nodeRegionList.append_child("Item");
        nodeItem.append_child("Seq");
        nodeItem.append_child("Piont");

        //区域列表
        for (auto &Item : tPictureMask.vecRegionList)
        {
            //区域编号
            nodeItem.child("Seq").text().set(Item.Seq);
            //区域左上角
            std::string Piont = "";
            Piont += Item.nlx;
            Piont += ",";
            Piont += Item.nly;
            Piont += ",";
            Piont += Item.nrx;
            Piont += ",";
            Piont += Item.nry;
            nodeItem.child("Piont").text().set(Piont);

            std::ostringstream os;
            stNewDoc.save(os);
            auto strGB18030 = ::ToMbcsString(os.str());
            SendMessageWithCallID(e, strGB18030);
        }
    }
    else
    {
        ResponeControlResult(e, nSN, m_header.strCmdType, strID, "ERROR");
        MLOG_WARN("回调操作执行失败[%d]", stResult.nResult);
    }

    return true;
}

bool SIP::QueryEvent::HandleFrameMirrorRequst(const SipEvent::Ptr &e)
{
    ParseHeader(e);

    GB28181::FrameMirrorInfo_S Mirror;

    auto root = m_doc.first_child();
    /*序列号*/
    int nSN = root.child("SN").text().as_int();
    Mirror.nSN = nSN;
    /* 设备Id */
    std::string strID = root.child("DeviceID").text().as_string();
    Mirror.strID = strID;

    auto pClient = dynamic_cast<SipClient *>(e->m_pNetBase);
    if (pClient == nullptr)
    {
        ResponeControlResult(e, nSN, m_header.strCmdType, strID, "ERROR");
        MLOG_WARN("客户端实例为空，无法获取客户端的通道信息");
        return false;
    }
    auto pChn = pClient->GetChannelByID(strID);
    if (nullptr == pChn)
    {
        ResponeControlResult(e, nSN, m_header.strCmdType, strID, "ERROR");
        MLOG_WARN("没有找到通道[%s]", m_header.strDevID.c_str());
        return false;
    }
    /*上级根据通道号赋值，下级设置成-1*/
    Mirror.nIndex = pChn->nIndex;
    /*true-设备配置设置，false-设备配置获取*/
    Mirror.bIsSet = false;

    /*TODO 设备配置上层回调*/
    SipCbResult_S stResult;
    { /* 配合上层实现业务功能 */
        auto fnCb = ::SipModule::instance()->GetFrameMirrorCb();
        if (fnCb)
        {
            fnCb(Mirror, stResult);
        }
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

    auto nodeResult = rootRet.append_child("Result");
    nodeResult.text().set(Mirror.strResult);

    if(stResult.nResult == 0)
    {
         /*0-不启用镜像，1-水平镜像，2-上下镜像，3-中心镜像*/
        auto node = rootRet.append_child("FrameMirror");
        node.text().set(Mirror.nFrameMirror);

        std::ostringstream os;
        stNewDoc.save(os);
        auto strGB18030 = ::ToMbcsString(os.str());
        SendMessageWithCallID(e, strGB18030);
    }
    else
    {
        ResponeControlResult(e, nSN, m_header.strCmdType, strID, "ERROR");
        MLOG_WARN("回调操作执行失败[%d]", stResult.nResult);
    }

    return true;
}

bool SIP::QueryEvent::HandleAlarmReportRequst(const SipEvent::Ptr &e)
{
    ParseHeader(e);

    GB28181::AlarmReportInfo_S AlarmRepor;

    auto root = m_doc.first_child();
    /*序列号*/
    int nSN = root.child("SN").text().as_int();
    AlarmRepor.nSN = nSN;
    /* 设备Id */
    std::string strID = root.child("DeviceID").text().as_string();
    AlarmRepor.strID = strID;

    auto pClient = dynamic_cast<SipClient *>(e->m_pNetBase);
    if (pClient == nullptr)
    {
        ResponeControlResult(e, nSN, m_header.strCmdType, strID, "ERROR");
        MLOG_WARN("客户端实例为空，无法获取客户端的通道信息");
        return false;
    }
    auto pChn = pClient->GetChannelByID(strID);
    if (nullptr == pChn)
    {
        ResponeControlResult(e, nSN, m_header.strCmdType, strID, "ERROR");
        MLOG_WARN("没有找到通道[%s]", m_header.strDevID.c_str());
        return false;
    }
    /*上级根据通道号赋值，下级设置成-1*/
    AlarmRepor.nIndex = pChn->nIndex;
    /*true-设备配置设置，false-设备配置获取*/
    AlarmRepor.bIsSet = false;

    /*TODO 设备配置上层回调*/
    SipCbResult_S stResult;
    { /* 配合上层实现业务功能 */
        auto fnCb = ::SipModule::instance()->GetAlarmReportCb();
        if (fnCb)
        {
            fnCb(AlarmRepor, stResult);
        }
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

    auto nodeResult = rootRet.append_child("Result");
    nodeResult.text().set(AlarmRepor.strResult);

    if(stResult.nResult == 0)
    {
        auto node = rootRet.append_child("AlarmReport");

        /*移动侦测事件上报开关，0-关闭，1-打开*/
        auto nodeMotion = node.append_child("MotionDetection");
        nodeMotion.text().set(AlarmRepor.nMotionDetection);
        auto nodeField = node.child("FieldDetection");
        nodeField.text().set(AlarmRepor.nFieldDetection);

        std::ostringstream os;
        stNewDoc.save(os);
        auto strGB18030 = ::ToMbcsString(os.str());
        SendMessageWithCallID(e, strGB18030);
    }
    else
    {
        ResponeControlResult(e, nSN, m_header.strCmdType, strID, "ERROR");
        MLOG_WARN("回调操作执行失败[%d]", stResult.nResult);
    }

    return true;
}

bool SIP::QueryEvent::HandleOSDConfigRequest(const SipEvent::Ptr &e)
{
    ParseHeader(e);

    GB28181::OSDConfig_S OSD;

    auto root = m_doc.first_child();
    /*序列号*/
    int nSN = root.child("SN").text().as_int();
    OSD.nSN = nSN;
    /* 设备Id */
    std::string strID = root.child("DeviceID").text().as_string();
    OSD.strID = strID;
    
    auto pClient = dynamic_cast<SipClient *>(e->m_pNetBase);
    if (pClient == nullptr)
    {
        ResponeControlResult(e, nSN, m_header.strCmdType, strID, "ERROR");
        MLOG_WARN("客户端实例为空，无法获取客户端的通道信息");
        return false;
    }
    auto pChn = pClient->GetChannelByID(strID);
    if (nullptr == pChn)
    {
        ResponeControlResult(e, nSN, m_header.strCmdType, strID, "ERROR");
        MLOG_WARN("没有找到通道[%s]", m_header.strDevID.c_str());
        return false;
    }
    /*上级根据通道号赋值，下级设置成-1*/
    OSD.nIndex = pChn->nIndex;
    /*true-设备配置设置，false-设备配置获取*/
    OSD.bIsSet = false;

    /*TODO 设备配置上层回调*/
    SipCbResult_S stResult;
    { /* 配合上层实现业务功能 */
        auto fnCb = ::SipModule::instance()->GetOSDConfigCb();
        if (fnCb)
        {
            fnCb(OSD, stResult);
        }
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

    auto nodeResult = rootRet.append_child("Result");
    nodeResult.text().set(OSD.strResult);

    if(stResult.nResult == 0)
    {
        auto node = rootRet.append_child("OSDConfig");

        auto nodeLen = node.append_child("Length");
        nodeLen.text().set(OSD.m_nLength);

        auto nodeWid = node.append_child("Width");
        nodeWid.text().set(OSD.m_nWidth);

        auto nodeTX = node.append_child("TimeX");
        nodeTX.text().set(OSD.m_nTimeX);

        auto nodeTY = node.append_child("TimeY");
        nodeTY.text().set(OSD.m_nTimeY);

        auto nodeTimeE = node.append_child("TimeEnable");
        nodeTimeE.text().set(OSD.m_nTimeEnable);

        auto nodeTimeP = node.append_child("TimeType");
        nodeTimeP.text().set(OSD.m_nTimeType);

        auto nodeTextE = node.append_child("TextEnable");
        nodeTextE.text().set(OSD.m_nTextEnable);

        auto nodeSum= node.append_child("SumNum");
        nodeSum.text().set(OSD.m_SumNum );

        
        if(OSD.m_vecItme.size() > 0)
        {
            auto nodeItem = node.append_child("Item");
            nodeItem.append_child("Text");
            nodeItem.append_child("X");
            nodeItem.append_child("Y");

            for (auto Item : OSD.m_vecItme)
            {
                nodeItem.child("Text").text().set(Item.Text);
                nodeItem.child("X").text().set(Item.X);
                nodeItem.child("Y").text().set(Item.Y);
            }
        }

            std::ostringstream os;
            stNewDoc.save(os);
            auto strGB18030 = ::ToMbcsString(os.str());
            SendMessageWithCallID(e, strGB18030);
    }
    else
    {
        ResponeControlResult(e, nSN, m_header.strCmdType, strID, "ERROR");
        MLOG_WARN("回调操作执行失败[%d]", stResult.nResult);
    }

    return true;
}

bool SIP::QueryEvent::HandleSnapShotConfig(const SipEvent::Ptr &e)
{
    ParseHeader(e);

    GB28181::SnapShotConfigInfo_S Snap;

    auto root = m_doc.first_child();
    /*序列号*/
    int nSN = root.child("SN").text().as_int();
    Snap.nSN = nSN;
    /* 设备Id */
    std::string strID = root.child("DeviceID").text().as_string();
    Snap.strID = strID;

    auto pClient = dynamic_cast<SipClient *>(e->m_pNetBase);
    if (pClient == nullptr)
    {
        ResponeControlResult(e, nSN, m_header.strCmdType, strID, "ERROR");
        MLOG_WARN("客户端实例为空，无法获取客户端的通道信息");
        return false;
    }
    auto pChn = pClient->GetChannelByID(strID);
    if (nullptr == pChn)
    {
        ResponeControlResult(e, nSN, m_header.strCmdType, strID, "ERROR");
        MLOG_WARN("没有找到通道[%s]", m_header.strDevID.c_str());
        return false;
    }
    /*上级根据通道号赋值，下级设置成-1*/
    Snap.nIndex = pChn->nIndex;
    /*true-设备配置设置，false-设备配置获取*/
    Snap.bIsSet = false;

    /*TODO 设备配置上层回调*/
    SipCbResult_S stResult;
    { /* 配合上层实现业务功能 */
        auto fnCb = ::SipModule::instance()->GetSnapShotCb();
        if (fnCb)
        {
            fnCb(Snap, stResult);
        }
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

    auto nodeResult = rootRet.append_child("Result");
    nodeResult.text().set(Snap.strResult);

    if(stResult.nResult == 0)
    {
        auto node = rootRet.append_child("SnapShotConfig");

        auto nodeNum = node.child("SnapNum");
        nodeNum.text().set(Snap.nSnapNum);

        auto nodeInter = node.child("Interval");
        nodeInter.text().set(Snap.nInterval);

        auto nodeURL = node.child("UploadURL");
        nodeURL.text().set(Snap.strUploadURL);

        auto nodeID  = node.child("SessionID");
        nodeID.text().set(Snap.strSessionID);

        std::ostringstream os;
        stNewDoc.save(os);
        auto strGB18030 = ::ToMbcsString(os.str());
        SendMessageWithCallID(e, strGB18030);

        /* 获取当前时间（包括毫秒） */
        struct timeval tv;
        gettimeofday(&tv, nullptr);
        SnapShotInfo_S stInfo;
        stInfo.e = e;
         /* 记录当前时间的秒数 */
        stInfo.nTime = tv.tv_sec;
        /* 记录图像抓拍请求 */
        if(m_pSnapSnapShot)
        {
            m_pSnapSnapShot->insert(std::make_pair(Snap.strSessionID, stInfo));
        }
    }
    else
    {
        ResponeControlResult(e, nSN, m_header.strCmdType, strID, "ERROR");
        MLOG_WARN("回调操作执行失败[%d]", stResult.nResult);
    }

    return true;
}

bool SIP::QueryEvent::HandleCruiseTrackQuery(const SipEvent::Ptr &e)
{
    ParseHeader(e);

    GB28181::CruiseTrackQueryInfo_S tCruiseTrack;

    auto root = m_doc.first_child();
    /*序列号*/
    int nSN = root.child("SN").text().as_int();
    tCruiseTrack.nSN = nSN;
    /* 设备Id */
    std::string strID = root.child("DeviceID").text().as_string();
    tCruiseTrack.strID = strID;

    /*拼接返回的报文*/
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

    auto nodeNumber = rootRet.append_child("Number");
    nodeNumber.text().set(tCruiseTrack.nNumber);

    auto nodeName = rootRet.append_child("Name");
    nodeName.text().set(tCruiseTrack.strName);

    auto nodeSumNum = rootRet.append_child("SumNum");
    nodeSumNum.text().set(0);
    

    auto pClient = dynamic_cast<SipClient *>(e->m_pNetBase);
    if (pClient == nullptr)
    {
        std::ostringstream os;
        stNewDoc.save(os);
        auto strGB18030 = ::ToMbcsString(os.str());
        SendMessageWithCallID(e, strGB18030);

        MLOG_WARN("客户端实例为空，无法获取客户端的通道信息");
        return false;
    }
    auto pChn = pClient->GetChannelByID(strID);
    if (nullptr == pChn)
    {
        std::ostringstream os;
        stNewDoc.save(os);
        auto strGB18030 = ::ToMbcsString(os.str());
        SendMessageWithCallID(e, strGB18030);

        MLOG_WARN("没有找到通道[%s]", m_header.strDevID.c_str());
        return false;
    }

    /*TODO 回调上抛*/
    SipCbResult_S stResult;
    { /* 配合上层实现业务功能 */
        auto fnCb = ::SipModule::instance()->GetCruiseTrackQueryCb();
        if (fnCb)
        {
            fnCb(tCruiseTrack, stResult);
        }
    }


    if(stResult.nResult == 0)
    {
        if(tCruiseTrack.vecCruisePointList.size() > 0)
        {
            nodeSumNum.text().set(tCruiseTrack.nSumNum);

            auto node = rootRet.append_child("CruisePointList");
            node.append_attribute("Num").set_value(std::to_string(1));

            auto nodePoint = node.append_child("CruisePoint");

            nodePoint.append_child("PresetIndex");
            nodePoint.append_child("StayTime");
            nodePoint.append_child("Speed");    

            for (auto Item : tCruiseTrack.vecCruisePointList)
            {
                nodePoint.child("PresetIndex").text().set(Item.nPresetIndex);
                nodePoint.child("StayTime").text().set(Item.nStayTime);
                nodePoint.child("Speed").text().set(Item.nSpeed);

                std::ostringstream os;
                stNewDoc.save(os);
                auto strGB18030 = ::ToMbcsString(os.str());
                SendMessageWithCallID(e, strGB18030);
            }
        }
        else
        {
            std::ostringstream os;
            stNewDoc.save(os);
            auto strGB18030 = ::ToMbcsString(os.str());
            SendMessageWithCallID(e, strGB18030);
        }
    }
    else
    {
        MLOG_WARN("回调操作执行失败[%d]", stResult.nResult);

        std::ostringstream os;
        stNewDoc.save(os);
        auto strGB18030 = ::ToMbcsString(os.str());
        SendMessageWithCallID(e, strGB18030);
    }

    return true;
}

bool SIP::QueryEvent::HandleCruiseTrackListQuery(const SipEvent::Ptr &e)
{
    ParseHeader(e);

    GB28181::CruiseTrackListQueryInfo_S tCruiseTrack;

    auto root = m_doc.first_child();
    /*序列号*/
    int nSN = root.child("SN").text().as_int();
    tCruiseTrack.nSN = nSN;
    /* 设备Id */
    std::string strID = root.child("DeviceID").text().as_string();
    tCruiseTrack.strID = strID;

     /*拼接返回的报文*/
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
 
     auto nodeSumNum = rootRet.append_child("SumNum");
     nodeSumNum.text().set(0);

    auto pClient = dynamic_cast<SipClient *>(e->m_pNetBase);
    if (pClient == nullptr)
    {
        std::ostringstream os;
        stNewDoc.save(os);
        auto strGB18030 = ::ToMbcsString(os.str());
        SendMessageWithCallID(e, strGB18030);

        MLOG_WARN("客户端实例为空，无法获取客户端的通道信息");
        return false;
    }
    auto pChn = pClient->GetChannelByID(strID);
    if (nullptr == pChn)
    {
        std::ostringstream os;
        stNewDoc.save(os);
        auto strGB18030 = ::ToMbcsString(os.str());
        SendMessageWithCallID(e, strGB18030);

        MLOG_WARN("没有找到通道[%s]", m_header.strDevID.c_str());
        return false;
    }

    /*TODO 回调上抛*/
    SipCbResult_S stResult;
    { /* 配合上层实现业务功能 */
        auto fnCb = ::SipModule::instance()->GetCruiseTrackListQueryCb();
        if (fnCb)
        {
            fnCb(tCruiseTrack, stResult);
        }
    }


    if(stResult.nResult == 0)
    {
        if(tCruiseTrack.vecCruiseTracktList.size())
        {
            nodeSumNum.text().set(tCruiseTrack.nSumNum);

            auto node = rootRet.append_child("CruiseTrackList");
            node.append_attribute("Num").set_value(std::to_string(1));

            auto nodeCruiseTrack = node.append_child("CruiseTrack");

            nodeCruiseTrack.append_child("Number");
            nodeCruiseTrack.append_child("Name");

            for (auto Item : tCruiseTrack.vecCruiseTracktList)
            {
                /*轨迹编号*/
                nodeCruiseTrack.child("Number").text().set(Item.nNumber);
                /*轨迹名称*/
                nodeCruiseTrack.child("Name").text().set(Item.strName);

                std::ostringstream os;
                stNewDoc.save(os);
                auto strGB18030 = ::ToMbcsString(os.str());
                SendMessageWithCallID(e, strGB18030);
            }
        }
        else
        {
            std::ostringstream os;
            stNewDoc.save(os);
            auto strGB18030 = ::ToMbcsString(os.str());
            SendMessageWithCallID(e, strGB18030);
        }
    }
    else
    {
        MLOG_WARN("回调操作执行失败[%d]", stResult.nResult);

        std::ostringstream os;
        stNewDoc.save(os);
        auto strGB18030 = ::ToMbcsString(os.str());
        SendMessageWithCallID(e, strGB18030);
    }

    return true;
}

bool SIP::QueryEvent::HandlePresetQuery(const SipEvent::Ptr &e)
{
    ParseHeader(e);

    GB28181::PresetQueryInfo_S Preset;
    memset(&Preset, 0, sizeof(Preset));

    auto root = m_doc.first_child();
    /*序列号*/
    int nSN = root.child("SN").text().as_int();
    Preset.nSN = nSN;
    /* 设备Id */
    std::string strID;
    auto nodeDevice = root.child("DeviceID");
    if(!nodeDevice.empty())
    {
        strID = nodeDevice.text().as_string();
    }
    Preset.strID = strID;

    /*拼接返回的报文*/
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

    auto nodeSumNum = rootRet.append_child("SumNum");
    nodeSumNum.text().set(0);

    auto pClient = dynamic_cast<SipClient *>(e->m_pNetBase);
    if (pClient == nullptr)
    {
        std::ostringstream os;
        stNewDoc.save(os);
        auto strGB18030 = ::ToMbcsString(os.str());
        SendMessageWithCallID(e, strGB18030);

        MLOG_WARN("客户端实例为空，无法获取客户端的通道信息");
        return false;
    }
    auto pChn = pClient->GetChannelByID(strID);
    if (nullptr == pChn)
    {
        std::ostringstream os;
        stNewDoc.save(os);
        auto strGB18030 = ::ToMbcsString(os.str());
        SendMessageWithCallID(e, strGB18030);

        MLOG_WARN("没有找到通道[%s]", m_header.strDevID.c_str());
        return false;
    }
    Preset.nIndex = pChn->nIndex;
    /*TODO 回调上抛*/
    SipCbResult_S stResult;
    { /* 配合上层实现业务功能 */
        auto fnCb = ::SipModule::instance()->GetPresetQueryCb();
        if (fnCb)
        {
            fnCb(Preset, stResult);
        }
    }

#if 0  //DEBUG
    int i=1;
    for(i; i<2; i++)
    {
        GB28181::PresetListItem PresetItem;
        PresetItem.strPresetID = std::to_string(i);
        PresetItem.strPresetName  = std::to_string(i);
        Preset.vecPresetList.push_back(PresetItem);
    }
    nodeSumNum.text().set(Preset.vecPresetList.size());
#endif

    if(stResult.nResult == 0)
    {
        //nodeSumNum.text().set(Preset.nSumNum);

        auto node = rootRet.append_child("PresetList");

        if(Preset.vecPresetList.size() > 0)
        {
#if 0   //分开发
            auto nodeItme = node.append_child("Itme");

            nodeItme.append_child("PresetID");
            nodeItme.append_child("PresetName");

            node.append_attribute("Num").set_value(std::to_string(1));
            for (auto Item : Preset.vecPresetList)
            {
     
                nodeItme.child("PresetID").text().set(Item.strPresetID);
                nodeItme.child("PresetName").text().set(Item.strPresetName);
    
                std::ostringstream os;
                stNewDoc.save(os);
                auto strGB18030 = ::ToMbcsString(os.str());
                SendMessageWithCallID(e, strGB18030);
            }
#endif

#if 1   //全部发
            node.append_attribute("Num").set_value(std::to_string(Preset.vecPresetList.size()));
            for (auto Item : Preset.vecPresetList)
            {
                auto nodeItme = node.append_child("Itme");
                nodeItme.append_child("PresetID");
                nodeItme.append_child("PresetName");
                /*轨迹编号*/
                nodeItme.child("PresetID").text().set(Item.strPresetID);
                /*轨迹名称*/
                nodeItme.child("PresetName").text().set(Item.strPresetName);
            }
            std::ostringstream os;
            stNewDoc.save(os);
            auto strGB18030 = ::ToMbcsString(os.str());
            SendMessageWithCallID(e, strGB18030);
#endif

        }
        else
        {
            std::ostringstream os;
            stNewDoc.save(os);
            auto strGB18030 = ::ToMbcsString(os.str());
            SendMessageWithCallID(e, strGB18030);
        }
    }
    else
    {
        MLOG_WARN("回调操作执行失败[%d]", stResult.nResult);

        std::ostringstream os;
        stNewDoc.save(os);
        auto strGB18030 = ::ToMbcsString(os.str());
        SendMessageWithCallID(e, strGB18030);
    }

    return true;
}


bool SIP::QueryEvent::HandleHomePositionQuery(const SipEvent::Ptr &e)
{
    ParseHeader(e);

    GB28181::HomePositionInfo_S tHomePosition;

    auto root = m_doc.first_child();
    /*序列号*/
    int nSN = root.child("SN").text().as_int();
    tHomePosition.nSN = nSN;
    /* 设备Id */
    std::string strID;
    auto nodeDevice = root.child("DeviceID");
    if(!nodeDevice.empty())
    {
        strID = nodeDevice.text().as_string();
    }
    tHomePosition.strID = strID;

    
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

    auto pClient = dynamic_cast<SipClient *>(e->m_pNetBase);
    if (pClient == nullptr)
    {
        std::ostringstream os;
        stNewDoc.save(os);
        auto strGB18030 = ::ToMbcsString(os.str());
        SendMessageWithCallID(e, strGB18030);
        MLOG_WARN("客户端实例为空，无法获取客户端的通道信息");
        return false;
    }
    auto pChn = pClient->GetChannelByID(strID);
    if (nullptr == pChn)
    {
        std::ostringstream os;
        stNewDoc.save(os);
        auto strGB18030 = ::ToMbcsString(os.str());
        SendMessageWithCallID(e, strGB18030);
        MLOG_WARN("没有找到通道[%s]", m_header.strDevID.c_str());
        return false;
    }
    /*上级根据通道号赋值，下级设置成-1*/
    tHomePosition.nIndex = pChn->nIndex;
    /*true-设备控制设置，false-设备查询获取*/
    tHomePosition.bIsSet = false;

    /*TODO 设备配置上层回调*/
    SipCbResult_S stResult;
    { /* 配合上层实现业务功能 */
        auto fnCb = ::SipModule::instance()->GetHomePositionCb();
        if (fnCb)
        {
            fnCb(tHomePosition, stResult);
        }
    }

    if(stResult.nResult == 0)
    {
        auto node = rootRet.append_child("HomePosition");

        auto nodeEnabled = node.child("Enabled");
        nodeEnabled.text().set(tHomePosition.nEnabled);

        auto nodeResetTime = node.child("ResetTime");
        nodeResetTime.text().set(tHomePosition.nResetTime);

        auto nodePresetIndex = node.child("PresetIndex");
        nodePresetIndex.text().set(tHomePosition.nPresetIndex);

        std::ostringstream os;
        stNewDoc.save(os);
        auto strGB18030 = ::ToMbcsString(os.str());
        SendMessageWithCallID(e, strGB18030);
    }
    else
    {
        std::ostringstream os;
        stNewDoc.save(os);
        auto strGB18030 = ::ToMbcsString(os.str());
        SendMessageWithCallID(e, strGB18030);
        MLOG_WARN("回调操作执行失败[%d]", stResult.nResult);
    }

    return true;
}

bool SIP::QueryEvent::HandlePTZPosition(const SipEvent::Ptr &e)
{
    ParseHeader(e);

    GB28181::PTZPositionInfo_S tPTZPosition;
    memset(&tPTZPosition,0,sizeof(tPTZPosition));

    auto root = m_doc.first_child();
    /*序列号*/
    int nSN = root.child("SN").text().as_int();
    /* 设备Id */

    std::string strID;
    auto nodeDevice = root.child("DeviceID");
    if(!nodeDevice.empty())
    {
        strID = nodeDevice.text().as_string();
    }
    tPTZPosition.strID = strID;


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

    auto pClient = dynamic_cast<SipClient *>(e->m_pNetBase);
    if (pClient == nullptr)
    {
        std::ostringstream os;
        stNewDoc.save(os);
        auto strGB18030 = ::ToMbcsString(os.str());
        SendMessageWithCallID(e, strGB18030);
        MLOG_WARN("客户端实例为空，无法获取客户端的通道信息");
        return false;
    }
    auto pChn = pClient->GetChannelByID(strID);
    if (nullptr == pChn)
    {
        std::ostringstream os;
        stNewDoc.save(os);
        auto strGB18030 = ::ToMbcsString(os.str());
        SendMessageWithCallID(e, strGB18030);
        MLOG_WARN("没有找到通道[%s]", m_header.strDevID.c_str());
        return false;
    }

    /*上级根据通道号赋值，下级设置成-1*/
    tPTZPosition.nIndex = pChn->nIndex;

    /*TODO 设备配置上层回调*/
    SipCbResult_S stResult;
    { /* 配合上层实现业务功能 */
        auto fnCb = ::SipModule::instance()->GetPTZPositionCb();
        if (fnCb)
        {
            fnCb(tPTZPosition, stResult);
        }
    }

    if(stResult.nResult == 0)
    {
        auto nodePan = rootRet.append_child("Pan");
        nodePan.text().set(tPTZPosition.dPan);

        auto nodeTilt = rootRet.append_child("Tilt");
        nodeTilt.text().set(tPTZPosition.dTilt);

        auto nodeZoom = rootRet.append_child("Zoom");
        nodeZoom.text().set(tPTZPosition.dZoom);

        auto nodeHorizontalFieldAngle = rootRet.append_child("HorizontalFieldAngle");
        nodeHorizontalFieldAngle.text().set(tPTZPosition.HorizontalFieldAngle);

        auto nodeVerticalFieldAngle = rootRet.append_child("VerticalFieldAngle");
        nodeVerticalFieldAngle.text().set(tPTZPosition.dVerticalFieldAngle);

        auto nodeMaxViewDistance = rootRet.append_child("MaxViewDistance");
        nodeMaxViewDistance.text().set(tPTZPosition.dMaxViewDistance);

        std::ostringstream os;
        stNewDoc.save(os);
        auto strGB18030 = ::ToMbcsString(os.str());
        SendMessageWithCallID(e, strGB18030);
    }
    else
    {
        std::ostringstream os;
        stNewDoc.save(os);
        auto strGB18030 = ::ToMbcsString(os.str());
        SendMessageWithCallID(e, strGB18030);
        MLOG_WARN("回调操作执行失败[%d]", stResult.nResult);
    }

    return true;
}

bool SIP::QueryEvent::SetSnapShotMap(std::map<std::string, SnapShotInfo_S> & map)
{
    m_pSnapSnapShot = &map;
    return true;
}