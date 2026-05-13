/*
 * @Author       : EasonLu
 * @Date         : 2025-02-14 16:55:14
 * @LastEditors  : EasonLu
 * @LastEditTime : 2025-02-25 20:12:51
 * @FilePath     : MethodRequest.cpp
 * @Description  : 各种方法的请求合集
 */
#include "MethodRequest.h"
#include "PtzCmd.h"
#include "SipUtils.h"
#include "pugixml.hpp"
#include <sstream>
#include <string>
using namespace SIP;

/* NOTE 1.make_manscdp_body返回的xml需要将UTF-8转换为GB18030
        2.没有中文可以不用转码
*/

const std::string CatalogRequest::make_manscdp_body()
{
    pugi::xml_document doc;
    auto declaration = doc.append_child(pugi::node_declaration);
    auto attrVersion = declaration.append_attribute("version");
    attrVersion.set_value("1.0");
    auto attrEncoding = declaration.append_attribute("encoding");
    attrEncoding.set_value("GB18030");

    auto root = doc.append_child("Query");

    auto nodeCmdType = root.append_child("CmdType");
    nodeCmdType.text().set("Catalog");

    auto nodeSN = root.append_child("SN");
    nodeSN.text().set(std::to_string(_request_sn));

    auto nodeDeviceID = root.append_child("DeviceID");
    nodeDeviceID.text().set(_device->GetDeviceID());

    std::ostringstream os;
    doc.print(os);
    auto strGB18030 = ::ToMbcsString(os.str());
    return strGB18030;
}

const std::string DeviceInfoRequest::make_manscdp_body()
{
    pugi::xml_document doc;
    auto declaration = doc.append_child(pugi::node_declaration);
    auto attrVersion = declaration.append_attribute("version");
    attrVersion.set_value("1.0");
    auto attrEncoding = declaration.append_attribute("encoding");
    attrEncoding.set_value("GB18030");

    auto root = doc.append_child("Query");

    auto nodeCmdType = root.append_child("CmdType");
    nodeCmdType.text().set("DeviceInfo");

    auto nodeSN = root.append_child("SN");
    nodeSN.text().set(std::to_string(_request_sn));

    auto nodeDeviceID = root.append_child("DeviceID");
    nodeDeviceID.text().set(_device->GetDeviceID());
    std::ostringstream os;
    doc.print(os);
    auto strGB18030 = ::ToMbcsString(os.str());
    return strGB18030;
}

const std::string PresetRequest::make_manscdp_body()
{
    pugi::xml_document doc;
    auto declaration = doc.append_child(pugi::node_declaration);
    auto attrVersion = declaration.append_attribute("version");
    attrVersion.set_value("1.0");
    auto attrEncoding = declaration.append_attribute("encoding");
    attrEncoding.set_value("GB18030");

    auto root = doc.append_child("Query");

    auto nodeCmdType = root.append_child("CmdType");
    nodeCmdType.text().set("PresetQuery");

    auto nodeSN = root.append_child("SN");
    nodeSN.text().set(std::to_string(_request_sn));

    auto nodeDeviceID = root.append_child("DeviceID");
    nodeDeviceID.text().set(_device->GetDeviceID());

    std::ostringstream os;
    doc.print(os);
    auto strGB18030 = ::ToMbcsString(os.str());
    return strGB18030;
}

void PresetRequest::InsertPreset(const std::string &preset_id, const std::string &preset_name)
{
    _presets.push_back({preset_id, preset_name});
}

const std::vector<std::pair<std::string, std::string>> PresetRequest::GetPresetList()
{
    return _presets;
}

const std::string PresetCtlRequest::make_manscdp_body()
{
    pugi::xml_document doc;
    auto declaration = doc.append_child(pugi::node_declaration);
    auto attrVersion = declaration.append_attribute("version");
    attrVersion.set_value("1.0");
    auto attrEncoding = declaration.append_attribute("encoding");
    attrEncoding.set_value("GB18030");

    auto root = doc.append_child("Control");

    auto nodeCmdType = root.append_child("CmdType");
    nodeCmdType.text().set("DeviceControl");

    auto nodeSN = root.append_child("SN");
    nodeSN.text().set(std::to_string(_request_sn));

    auto nodeDeviceID = root.append_child("DeviceID");
    nodeDeviceID.text().set(_channel_id);

    auto nodePTZCmd = root.append_child("PTZCmd");
    nodePTZCmd.text().set(PtzCmd::cmdCode(_byte4, _byte5, _byte6, _byte7));

    auto nodeInfo = root.append_child("Info");
    auto nodeControlPriority = nodeInfo.append_child("ControlPriority");
    nodeControlPriority.text().set("5");

    std::ostringstream os;
    doc.print(os);
    auto strGB18030 = ::ToMbcsString(os.str());
    return strGB18030;
}

const std::string PtzCtlRequest::make_manscdp_body()
{
    pugi::xml_document doc;
    auto declaration = doc.append_child(pugi::node_declaration);
    auto attrVersion = declaration.append_attribute("version");
    attrVersion.set_value("1.0");
    auto attrEncoding = declaration.append_attribute("encoding");
    attrEncoding.set_value("GB18030");

    auto root = doc.append_child("Control");

    auto nodeCmdType = root.append_child("CmdType");
    nodeCmdType.text().set("DeviceControl");

    auto nodeSN = root.append_child("SN");
    nodeSN.text().set(std::to_string(_request_sn));

    auto nodeDeviceID = root.append_child("DeviceID");
    nodeDeviceID.text().set(_channel_id);

    auto nodePTZCmd = root.append_child("PTZCmd");
    nodePTZCmd.text().set(PtzCmd::cmdString(_leftRight, _upDown, _inOut, _moveSpeed, _zoomSpeed));

    auto nodeInfo = root.append_child("Info");
    auto nodeControlPriority = nodeInfo.append_child("ControlPriority");
    nodeControlPriority.text().set("5");

    std::ostringstream os;
    doc.print(os);
    auto strGB18030 = ::ToMbcsString(os.str());
    return strGB18030;
}

int PtzCtlRequest::HandleResponse(int statcode)
{
    _leftRight = PtzCommand_E::NONE;
    _upDown = PtzCommand_E::NONE;
    _inOut = PtzCommand_E::NONE;
    _moveSpeed = 0;
    _zoomSpeed = 0;

    // 收到相机回复后，立即停止云台转动
    MLOG_INFO("PtzControlRequest HandleResponse statuscode [%d]", statcode);
    // SendMessage(false);
    return 0;
}

const std::string LensCtlRequest::make_manscdp_body()
{
    pugi::xml_document doc;
    auto declaration = doc.append_child(pugi::node_declaration);
    auto attrVersion = declaration.append_attribute("version");
    attrVersion.set_value("1.0");
    auto attrEncoding = declaration.append_attribute("encoding");
    attrEncoding.set_value("GB18030");

    auto root = doc.append_child("Control");

    auto nodeCmdType = root.append_child("CmdType");
    nodeCmdType.text().set("DeviceControl");

    auto nodeSN = root.append_child("SN");
    nodeSN.text().set(std::to_string(_request_sn));

    auto nodeDeviceID = root.append_child("DeviceID");
    nodeDeviceID.text().set(_channel_id);

    auto nodePTZCmd = root.append_child("PTZCmd");
    nodePTZCmd.text().set(PtzCmd::cmdLens(_iris, _focus, _iris_speed, _focus_speed));

    auto nodeInfo = root.append_child("Info");
    auto nodeControlPriority = nodeInfo.append_child("ControlPriority");
    nodeControlPriority.text().set("5");

    std::ostringstream os;
    doc.print(os);
    auto strGB18030 = ::ToMbcsString(os.str());
    return strGB18030;
}

int LensCtlRequest::HandleResponse(int statcode)
{
    return 0;
}
#if 0 /* TODO 后续才实现音视频相关的请求 */
const std::string RecordRequest::make_manscdp_body()
{
    auto text = R"(<?xml version="1.0"?>
					<Query>
						<CmdType>RecordInfo</CmdType>
						<SN>{}</SN>
						<DeviceID>{}</DeviceID>
						<StartTime>{}</StartTime>
						<EndTime>{}</EndTime>
						<Secrecy>0</Secrecy>
						<Type>all</Type>
					</Query>
					)";

    return fmt::format(text, _request_sn, _channel_id,
                       toolkit::getTimeStr("%Y-%m-%dT%H:%M:%S", _start_time),
                       toolkit::getTimeStr("%Y-%m-%dT%H:%M:%S", _end_time));
}

void RecordRequest::InsertRecord(std::shared_ptr<RecordItem> item)
{
    _record_items.push_back(item);
}

uint32_t RecordRequest::GetRecordSize()
{
    return (uint32_t)_record_items.size();
}

std::vector<std::shared_ptr<RecordItem>> RecordRequest::GetRecordList()
{
    return _record_items;
}
#endif

const std::string AlarmRequest::make_manscdp_body()
{

    pugi::xml_document doc;
    auto declaration = doc.append_child(pugi::node_declaration);
    auto attrVersion = declaration.append_attribute("version");
    attrVersion.set_value("1.0");
    auto attrEncoding = declaration.append_attribute("encoding");
    attrEncoding.set_value("GB18030");

    auto root = doc.append_child("Query");

    auto nodeCmdType = root.append_child("CmdType");
    nodeCmdType.text().set("Alarm");

    auto nodeSN = root.append_child("SN");
    nodeSN.text().set(std::to_string(_request_sn));

    auto nodeDeviceID = root.append_child("DeviceID");
    nodeDeviceID.text().set(_device->GetDeviceID());

    /* NOTE 默认把1~4级报警全部订阅 */
    auto nodeStartAlarmPriority = root.append_child("StartAlarmPriority");
    nodeStartAlarmPriority.text().set(m_stInfo.enStartLevel);

    auto nodeEndAlarmPriority = root.append_child("EndAlarmPriority");
    nodeEndAlarmPriority.text().set(m_stInfo.enEndLevel);

    /* NOTE 暂定订阅所有视频相关的报警 */
    auto nodeAlarmMethod = root.append_child("AlarmMethod");
    nodeAlarmMethod.text().set(m_stInfo.enMethod);

#if 0 /* 暂定把报警发生的时间范围全部订阅 */
    if(0 == m_stInfo.enStartTime)
    {
        /* 没有设置时间则默认即刻开始 */
        m_stInfo.enStartTime = time(nullptr);
    }

    if(0 == m_stInfo.enEndTime)
    {
        /* 没有设置时间则默认一天后结束 */
        m_stInfo.enEndTime = m_stInfo.enStartTime + 86400;
    }
#endif
    /* 报警发生的起止时间 */
    if (m_stInfo.enStartTime > 0 && m_stInfo.enEndTime > 0)
    {
        auto nodeStartTime = root.append_child("StartTime");
        nodeStartTime.text().set(::TimeTToISO8601(m_stInfo.enStartTime));

        auto nodeEndTime = root.append_child("EndTime");
        nodeEndTime.text().set(::TimeTToISO8601(m_stInfo.enEndTime));
    }

    std::ostringstream os;
    doc.print(os);
    auto strGB18030 = ::ToMbcsString(os.str());
    return strGB18030;
}

const std::string GuardRequest::make_manscdp_body()
{
    pugi::xml_document doc;
    auto declaration = doc.append_child(pugi::node_declaration);
    auto attrVersion = declaration.append_attribute("version");
    attrVersion.set_value("1.0");
    auto attrEncoding = declaration.append_attribute("encoding");
    attrEncoding.set_value("GB18030");

    auto root = doc.append_child("Control");

    auto nodeCmdType = root.append_child("CmdType");
    nodeCmdType.text().set("DeviceControl");

    auto nodeSN = root.append_child("SN");
    nodeSN.text().set(std::to_string(_request_sn));

    auto nodeDeviceID = root.append_child("DeviceID");
    nodeDeviceID.text().set(_device->GetDeviceID());

    auto nodeGuardCmd = root.append_child("GuardCmd");
    nodeGuardCmd.text().set(m_bIsSetGuard ? "SetGuard" : "ResetGuard");

    std::ostringstream os;
    doc.print(os);
    auto strGB18030 = ::ToMbcsString(os.str());
    return strGB18030;
}

const std::string HeartbeatRequest::make_manscdp_body()
{
    pugi::xml_document doc;
    auto declaration = doc.append_child(pugi::node_declaration);
    auto attrVersion = declaration.append_attribute("version");
    attrVersion.set_value("1.0");
    auto attrEncoding = declaration.append_attribute("encoding");
    attrEncoding.set_value("GB18030");

    auto root = doc.append_child("Notify");

    auto nodeCmdType = root.append_child("CmdType");
    nodeCmdType.text().set("Keepalive");

    auto nodeSN = root.append_child("SN");
    nodeSN.text().set(std::to_string(_request_sn));

    auto nodeDeviceID = root.append_child("DeviceID");
    nodeDeviceID.text().set(m_strDeviceID);

    auto nodeStatus = root.append_child("Status");
    nodeStatus.text().set("OK");

    std::ostringstream os;
    doc.print(os);
    return os.str();
}
const std::string HeartbeatRequest::GetDeviceUrl()
{
    return m_strDeviceUrl;
}


const std::string BasicParamRequest::make_manscdp_body()
{
    pugi::xml_document doc;
    auto declaration = doc.append_child(pugi::node_declaration);
    auto attrVersion = declaration.append_attribute("version");
    attrVersion.set_value("1.0");
    auto attrEncoding = declaration.append_attribute("encoding");
    attrEncoding.set_value("GB18030");

    auto root = doc.append_child("Control");

    auto nodeCmdType = root.append_child("CmdType");
    nodeCmdType.text().set("DeviceConfig");

    auto nodeSN = root.append_child("SN");
    nodeSN.text().set(std::to_string(_request_sn));

    auto nodeDeviceID = root.append_child("DeviceID");
    nodeDeviceID.text().set(_device->GetDeviceID());

    auto nodeBasicParamConfigCmd = root.append_child("BasicParam");

    auto nodeName = nodeBasicParamConfigCmd.append_child("Name");
    nodeName.text().set(m_tBasicParamInfo.m_szName);

    auto nodeExpiration = nodeBasicParamConfigCmd.append_child("Expiration");
    nodeExpiration.text().set(m_tBasicParamInfo.m_nExpiration);

    auto nodeHeartBeatInterval = nodeBasicParamConfigCmd.append_child("HeartBeatInterval");
    nodeHeartBeatInterval.text().set(m_tBasicParamInfo.m_nHeartBeatInterval);

    auto nodeHeartBeatCount = nodeBasicParamConfigCmd.append_child("HeartBeatCount");
    nodeHeartBeatCount.text().set(m_tBasicParamInfo.m_nHeartBeatCount);

    std::ostringstream os;
    doc.print(os);
    auto strGB18030 = ::ToMbcsString(os.str());
    return strGB18030;
}

const std::string OSDConfigRequest::make_manscdp_body()
{
    /* 每次创建Body的时候只提取一个录制计划中的一个时间段的报文 */
    std::string strXml = "";
    if (!m_dequeMsg.empty())
    {
        strXml = m_dequeMsg.front();
        m_dequeMsg.pop_front();
    }
    return strXml;
}

int  OSDConfigRequest::SendMessage(bool bNeedCb)
{
     /* 清空队列 */
     m_dequeMsg.clear();
     { /* 组装报文 */
        pugi::xml_document doc;
        auto declaration = doc.append_child(pugi::node_declaration);
        auto attrVersion = declaration.append_attribute("version");
        attrVersion.set_value("1.0");
        auto attrEncoding = declaration.append_attribute("encoding");
        attrEncoding.set_value("GB18030");
    
        auto root = doc.append_child("Control");
    
        auto nodeCmdType = root.append_child("CmdType");
        nodeCmdType.text().set("DeviceConfig");
    
        auto nodeSN = root.append_child("SN");
        nodeSN.text().set(std::to_string(_request_sn));
    
        auto nodeDeviceID = root.append_child("DeviceID");
        nodeDeviceID.text().set(_device->GetDeviceID());
    
        auto nodeOSDConfigCmd = root.append_child("OSDConfig");
    
        auto nodeOSDConfigLength = nodeOSDConfigCmd.append_child("Length");
        nodeOSDConfigLength.text().set(m_OSDConfig.m_nLength);
    
        auto nodeOSDConfigWidth = nodeOSDConfigCmd.append_child("Width");
        nodeOSDConfigWidth.text().set(m_OSDConfig.m_nWidth);
    
        auto nodeOSDConfigTimeX = nodeOSDConfigCmd.append_child("TimeX");
        nodeOSDConfigTimeX.text().set(m_OSDConfig.m_nTimeX);
    
        auto nodeOSDConfigTimeY = nodeOSDConfigCmd.append_child("TimeY");
        nodeOSDConfigTimeY.text().set(m_OSDConfig.m_nTimeY);
    
        int SumNum = m_OSDConfig.m_vecItme.size();
        auto nodeOSDConfigSum = nodeOSDConfigCmd.append_child("SumNum");
        nodeOSDConfigSum.text().set(SumNum);

        //显示文字
        auto nodeOSDConfigItem = nodeOSDConfigCmd.append_child("Item");
        auto Text = nodeOSDConfigItem.append_child("Text");
        auto X = nodeOSDConfigItem.append_child("X");
        auto Y = nodeOSDConfigItem.append_child("Y");

        /* 配置数据 */
        for (auto it : m_OSDConfig.m_vecItme)
        {
            Text.text().set(it.Text);
            X.text().set(it.X);
            Y.text().set(it.Y);

            std::ostringstream os;
            doc.print(os);
            auto strGB18030 = ::ToMbcsString(os.str());
             /* 存入队列 */
             m_dequeMsg.push_back(strGB18030);
        }
     }

     while (m_dequeMsg.size() > 0)
     {
         /* 调用基类发送流程 */
         MessageRequest::SendMessage(bNeedCb);
     }
     return 0;
}

const std::string SVACEncodeConfigRequest::make_manscdp_body()
{
      /* 每次创建Body的时候只提取一个录制计划中的一个时间段的报文 */
      std::string strXml = "";
      if (!m_dequeMsg.empty())
      {
          strXml = m_dequeMsg.front();
          m_dequeMsg.pop_front();
      }
      return strXml;
}

int SVACEncodeConfigRequest::SendMessage(bool bNeedCb)
{
     /* 清空队列 */
     m_dequeMsg.clear();
     { /* 组装报文 */
        pugi::xml_document doc;
        auto declaration = doc.append_child(pugi::node_declaration);
        auto attrVersion = declaration.append_attribute("version");
        attrVersion.set_value("1.0");
        auto attrEncoding = declaration.append_attribute("encoding");
        attrEncoding.set_value("GB18030");

        auto root = doc.append_child("Control");

        auto nodeCmdType = root.append_child("CmdType");
        nodeCmdType.text().set("DeviceConfig");

        auto nodeSN = root.append_child("SN");
        nodeSN.text().set(std::to_string(_request_sn));

        auto nodeDeviceID = root.append_child("DeviceID");
        nodeDeviceID.text().set(_device->GetDeviceID());

        auto nodeSVACEncodeConfigCmd = root.append_child("SVACEncodeConfig");

        // ROIParam
        auto nodeROIParam = nodeSVACEncodeConfigCmd.append_child("ROIParam");
        auto nodeROIFlag = nodeROIParam.append_child("ROIFlag");
        nodeROIFlag.text().set(m_tSVACEncodeConfigInfo.tROIParam.nROIFlag);

        int ROINumber = m_tSVACEncodeConfigInfo.tROIParam.vecROIParamItem.size();
        auto nodeROINumber = nodeROIParam.append_child("ROINumber");
        nodeROINumber.text().set(ROINumber);

        auto nodeItem = nodeROIParam.append_child("Item");
        auto ROISeq = nodeItem.append_child("ROISeq");
        auto TopLeft = nodeItem.append_child("TopLeft");
        auto BottomRight = nodeItem.append_child("BottomRight");
        auto ROIQP = nodeItem.append_child("ROIQP");
        

        // SVCParam
        if (m_tSVACEncodeConfigInfo.tSVCParam.bIsHave)
        {
            auto nodeSVCParam = nodeSVACEncodeConfigCmd.append_child("SVCParam");

            auto nodeSVCSpaceDomainMode = nodeSVCParam.append_child("SVCSpaceDomainMode");
            nodeSVCSpaceDomainMode.text().set(m_tSVACEncodeConfigInfo.tSVCParam.nSVCSpaceDomainMode);
            auto nodeTimeSpaceDomainMode = nodeSVCParam.append_child("SVCTimeDomainMode");
            nodeTimeSpaceDomainMode.text().set(m_tSVACEncodeConfigInfo.tSVCParam.nSVCTimeDomainMode);
            if (m_tSVACEncodeConfigInfo.tSVCParam.strSSVCRatioValue.length() > 0)
            {
                auto nodeSSVCRatioValue = nodeSVCParam.append_child("SSVCRatioValue");
                nodeSSVCRatioValue.text().set(m_tSVACEncodeConfigInfo.tSVCParam.strSSVCRatioValue);
            }
            auto nodeSVCSpaceSupportMode = nodeSVCParam.append_child("SVCSpaceSupportMode");
            nodeSVCSpaceSupportMode.text().set(m_tSVACEncodeConfigInfo.tSVCParam.nSVCSpaceSupportMode);
            auto nodeSVCTimeSupportMode = nodeSVCParam.append_child("SVCTimeSupportMode");
            nodeSVCTimeSupportMode.text().set(m_tSVACEncodeConfigInfo.tSVCParam.nSVCTimeSupportMode);
            auto nodeSSVCRatioSupportList = nodeSVCParam.append_child("SSVCRatioSupportList");
            nodeSSVCRatioSupportList.text().set(m_tSVACEncodeConfigInfo.tSVCParam.strSSVCRatioSupportList);
        }

        // SurveillanceParam
        {
            auto nodeSurveillanceParam = nodeSVACEncodeConfigCmd.append_child("SurveillanceParam");

            auto nodeTimeFlag = nodeSurveillanceParam.append_child("TimeFlag");
            nodeTimeFlag.text().set(m_tSVACEncodeConfigInfo.tSurveillanceParam.nTimeFlag);
            auto nodeOSDFlag = nodeSurveillanceParam.append_child("OSDFlag");
            nodeOSDFlag.text().set(m_tSVACEncodeConfigInfo.tSurveillanceParam.nOSDFlag);
            auto nodeAIFlag = nodeSurveillanceParam.append_child("AIFlag");
            nodeAIFlag.text().set(m_tSVACEncodeConfigInfo.tSurveillanceParam.nAIFlag);
            auto nodeGISFlag = nodeSurveillanceParam.append_child("GISFlag");
            nodeGISFlag.text().set(m_tSVACEncodeConfigInfo.tSurveillanceParam.nGISFlag);
        }

        //音频参数
        if (m_tSVACEncodeConfigInfo.tAudioParam.bIsHave)
        {
            auto nodeAudioParam = nodeSVACEncodeConfigCmd.append_child("AudioParam");

            auto nodeTimeFlag = nodeAudioParam.append_child("AudioRecognitionFlag");
            nodeTimeFlag.text().set(m_tSVACEncodeConfigInfo.tAudioParam.nAudioRecognitionFlag);
        }

         /* 配置数据 */
        for (auto it : m_tSVACEncodeConfigInfo.tROIParam.vecROIParamItem)
        {
            ROISeq.text().set(it.nROISeq);
            TopLeft.text().set(it.nTopLeft);
            BottomRight.text().set(it.nBottomRight);
            ROIQP.text().set(it.nROIQP);

            std::ostringstream os;
            doc.print(os);
            auto strGB18030 = ::ToMbcsString(os.str());
            /* 存入队列 */
            m_dequeMsg.push_back(strGB18030);
        }

     }

     while (m_dequeMsg.size() > 0)
     {
         /* 调用基类发送流程 */
         MessageRequest::SendMessage(bNeedCb);
     }
     return 0;
}

const std::string SVACDecodeConfigRequest::make_manscdp_body()
{
    pugi::xml_document doc;
    auto declaration = doc.append_child(pugi::node_declaration);
    auto attrVersion = declaration.append_attribute("version");
    attrVersion.set_value("1.0");
    auto attrEncoding = declaration.append_attribute("encoding");
    attrEncoding.set_value("GB18030");

    auto root = doc.append_child("Control");

    auto nodeCmdType = root.append_child("CmdType");
    nodeCmdType.text().set("DeviceConfig");

    auto nodeSN = root.append_child("SN");
    nodeSN.text().set(std::to_string(_request_sn));

    auto nodeDeviceID = root.append_child("DeviceID");
    nodeDeviceID.text().set(_device->GetDeviceID());

    auto nodeSVACDecodeConfigCmd = root.append_child("SVACDecodeConfig");

    // SVCParam
    if (m_tSVACDecodeConfigInfo.tSVCParam.bIsHave)
    {
        auto nodeSVCParam = nodeSVACDecodeConfigCmd.append_child("SVCParam");

        auto nodeSVCSTMMode = nodeSVCParam.append_child("SVCSTMMode");
        nodeSVCSTMMode.text().set(m_tSVACDecodeConfigInfo.tSVCParam.nSVCSTMMode);
        auto nodeSVCSpaceSupportMode = nodeSVCParam.append_child("SVCSpaceSupportMode");
        nodeSVCSpaceSupportMode.text().set(m_tSVACDecodeConfigInfo.tSVCParam.nSVCSpaceSupportMode);
        auto nodeSVCTimeSupportMode = nodeSVCParam.append_child("SVCTimeSupportMode");
        nodeSVCTimeSupportMode.text().set(m_tSVACDecodeConfigInfo.tSVCParam.nSVCTimeSupportMode);
    }

    // SurveillanceParam
    if (m_tSVACDecodeConfigInfo.tSurveillanceParam.isHave)
    {
        auto nodeSurveillanceParam = nodeSVACDecodeConfigCmd.append_child("SurveillanceParam");

        auto nodeTimeShowFlag = nodeSurveillanceParam.append_child("TimeShowFlag");
        nodeTimeShowFlag.text().set(m_tSVACDecodeConfigInfo.tSurveillanceParam.nTimeShowFlag);
        auto nodeOSDShowFlag = nodeSurveillanceParam.append_child("OSDShowFlag");
        nodeOSDShowFlag.text().set(m_tSVACDecodeConfigInfo.tSurveillanceParam.nOSDShowFlag);
        auto nodeAIShowFlag = nodeSurveillanceParam.append_child("AIShowFlag");
        nodeAIShowFlag.text().set(m_tSVACDecodeConfigInfo.tSurveillanceParam.nAIShowFlag);
        auto nodeGISShowFlag = nodeSurveillanceParam.append_child("GISShowFlag");
        nodeGISShowFlag.text().set(m_tSVACDecodeConfigInfo.tSurveillanceParam.nGISShowFlag);
        
    }

    std::ostringstream os;
    doc.print(os);
    auto strGB18030 = ::ToMbcsString(os.str());
    return strGB18030;
}

const std::string VideoParamAttributeRequest::make_manscdp_body()
{
    /* 每次创建Body的时候只提取一个录制计划中的一个时间段的报文 */
    std::string strXml = "";
    if (!m_dequeMsg.empty())
    {
        strXml = m_dequeMsg.front();
        m_dequeMsg.pop_front();
    }
    return strXml;
}

int VideoParamAttributeRequest::SendMessage(bool bNeedCb)
{
     /* 清空队列 */
     m_dequeMsg.clear();
     { /* 组装报文 */
        pugi::xml_document doc;
        auto declaration = doc.append_child(pugi::node_declaration);
        auto attrVersion = declaration.append_attribute("version");
        attrVersion.set_value("1.0");
        auto attrEncoding = declaration.append_attribute("encoding");
        attrEncoding.set_value("GB18030");

        auto root = doc.append_child("Control");

        auto nodeCmdType = root.append_child("CmdType");
        nodeCmdType.text().set("DeviceConfig");

        auto nodeSN = root.append_child("SN");
        nodeSN.text().set(std::to_string(_request_sn));

        auto nodeDeviceID = root.append_child("DeviceID");
        nodeDeviceID.text().set(_device->GetDeviceID());

        auto nodeVideoParamAttributeCmd = root.append_child("VideoParamAttribute");
        m_tVideoParAttriConfigInfo.Num = m_tVideoParAttriConfigInfo.vecVideoParAttrItem.size();
        nodeVideoParamAttributeCmd.append_attribute("Num").set_value(std::to_string(1));

        auto nodeItem = nodeVideoParamAttributeCmd.append_child("Item");
        auto nodeStreamNumber = nodeItem.append_child("StreamNumber");
        auto nodeVideoFormat = nodeItem.append_child("VideoFormat");
        auto nodeResolution = nodeItem.append_child("Resolution");
        auto nodeFrameRate = nodeItem.append_child("FrameRate");
        auto nodeBitRateType = nodeItem.append_child("BitRateType");
        auto nodeVideoBitRate = nodeItem.append_child("VideoBitRate");

        //配置数据
        for (auto it : m_tVideoParAttriConfigInfo.vecVideoParAttrItem)
        {
            nodeStreamNumber.text().set(it.nStreamNumber);
            nodeVideoFormat.text().set( static_cast<int>(it.enVideoFormat));
            nodeResolution.text().set( static_cast<int>(it.enResolution));
            nodeFrameRate.text().set(it.strFrameRate);
            nodeBitRateType.text().set( static_cast<int>(it.enBitRateType));
            nodeVideoBitRate.text().set(it.strVideoBitRate);

            std::ostringstream os;
            doc.print(os);
            auto strGB18030 = ::ToMbcsString(os.str());
            /* 存入队列 */
            m_dequeMsg.push_back(strGB18030);
        }
     }
    
     while (m_dequeMsg.size() > 0)
     {
         /* 调用基类发送流程 */
         MessageRequest::SendMessage(bNeedCb);
     }
     return 0;
}

const std::string VideoRecordPlanRequest::make_manscdp_body()
{
    /* 每次创建Body的时候只提取一个录制计划中的一个时间段的报文 */
    std::string strXml = "";
    if (!m_dequeMsg.empty())
    {
        strXml = m_dequeMsg.front();
        m_dequeMsg.pop_front();
    }
    return strXml;
}

int VideoRecordPlanRequest::SendMessage(bool bNeedCb)
{
    /* 清空队列 */
    m_dequeMsg.clear();
    { /* 组装报文 */
        pugi::xml_document doc;
        auto declaration = doc.append_child(pugi::node_declaration);
        auto attrVersion = declaration.append_attribute("version");
        attrVersion.set_value("1.0");
        auto attrEncoding = declaration.append_attribute("encoding");
        attrEncoding.set_value("GB18030");

        auto root = doc.append_child("Control");

        auto nodeCmdType = root.append_child("CmdType");
        nodeCmdType.text().set("DeviceConfig");

        /* FIXME 归属同一消息的分不同分片，SN码是否要一致？ */
        auto nodeSN = root.append_child("SN");
        nodeSN.text().set(std::to_string(_request_sn));

        auto nodeDeviceID = root.append_child("DeviceID");
        nodeDeviceID.text().set(_device->GetDeviceID());

        auto nodeData = root.append_child("VideoRecordPlan");
        {
            /* 固定好字段格式，每次只组装一个录制时间段的信息 */
            auto nodeRecordEnable = nodeData.append_child("RecordEnable");
            nodeRecordEnable.text().set(m_stInfo.nRecordEnable);
            m_stInfo.nRecordScheduleSumNum = m_stInfo.vecRecordSchedule.size();

            auto nodeRecordScheduleSumNum = nodeData.append_child("RecordScheduleSumNum");
            nodeRecordScheduleSumNum.text().set(m_stInfo.nRecordScheduleSumNum);

            /* 增加属性描述，每次只发一个录制计划 */
            auto nodeRecordSchedule = nodeData.append_child("RecordSchedule");
            nodeRecordSchedule.append_attribute("Num").set_value(std::to_string(1));

            auto nodeWeekDayNum = nodeRecordSchedule.append_child("WeekDayNum");

            auto nodeTimeSegmentSumNum = nodeRecordSchedule.append_child("TimeSegmentSumNum");

            /* 增加属性描述，每次只发一个时间段 */
            auto nodeTimeSegment = nodeRecordSchedule.append_child("TimeSegment");
            nodeTimeSegment.append_attribute("Num").set_value(std::to_string(1));

            auto nodeStartHour = nodeTimeSegment.append_child("StartHour");
            auto nodeStartMin = nodeTimeSegment.append_child("StartMin");
            auto nodeStartSec = nodeTimeSegment.append_child("StartSec");
            auto nodeStopHour = nodeTimeSegment.append_child("StopHour");
            auto nodeStopMin = nodeTimeSegment.append_child("StopMin");
            auto nodeStopSec = nodeTimeSegment.append_child("StopSec");

            auto nodeStreamNumber = nodeData.append_child("StreamNumber");
            nodeStreamNumber.text().set(m_stInfo.nStreamNumber);
            /* 配置数据 */
            for (auto it : m_stInfo.vecRecordSchedule)
            {
                nodeWeekDayNum.text().set(it.first);

                it.second.nTimeSegmentSumNum = it.second.vecTimeSegment.size();

                nodeTimeSegmentSumNum.text().set(it.second.nTimeSegmentSumNum);
                for (auto TimeItem : it.second.vecTimeSegment)
                {
                    nodeStartHour.text().set(TimeItem.nStartHour);
                    nodeStartMin.text().set(TimeItem.nStartMin);
                    nodeStartSec.text().set(TimeItem.nStartSec);
                    nodeStopHour.text().set(TimeItem.nStopHour);
                    nodeStopMin.text().set(TimeItem.nStopMin);
                    nodeStopSec.text().set(TimeItem.nStopSec);
                    std::ostringstream os;
                    doc.print(os);
                    auto strGB18030 = ::ToMbcsString(os.str());
                    /* 存入队列 */
                    m_dequeMsg.push_back(strGB18030);
                }
            }
        }
    }

    while (m_dequeMsg.size() > 0)
    {
        /* 调用基类发送流程 */
        MessageRequest::SendMessage(bNeedCb);
    }
    return 0;
}

const std::string AlarmReportRequest::make_manscdp_body()
{
    pugi::xml_document doc;
    auto declaration = doc.append_child(pugi::node_declaration);
    auto attrVersion = declaration.append_attribute("version");
    attrVersion.set_value("1.0");
    auto attrEncoding = declaration.append_attribute("encoding");
    attrEncoding.set_value("GB18030");

    auto root = doc.append_child("Control");

    auto nodeCmdType = root.append_child("CmdType");
    nodeCmdType.text().set("DeviceConfig");

    auto nodeSN = root.append_child("SN");
    nodeSN.text().set(std::to_string(_request_sn));

    auto nodeDeviceID = root.append_child("DeviceID");
    nodeDeviceID.text().set(_device->GetDeviceID());

    auto nodeAlarmReportConfigCmd = root.append_child("AlarmReport");

    auto nodeMotionDetection = nodeAlarmReportConfigCmd.append_child("MotionDetection");
    nodeMotionDetection.text().set(m_tAlarmReportInfo.nMotionDetection);

    auto nodeFieldDetection = nodeAlarmReportConfigCmd.append_child("FieldDetection");
    nodeFieldDetection.text().set(m_tAlarmReportInfo.nFieldDetection);

    std::ostringstream os;
    doc.print(os);
    auto strGB18030 = ::ToMbcsString(os.str());
    return strGB18030;
}

const std::string SnapShotRequest::make_manscdp_body()
{
    pugi::xml_document doc;
    auto declaration = doc.append_child(pugi::node_declaration);
    auto attrVersion = declaration.append_attribute("version");
    attrVersion.set_value("1.0");
    auto attrEncoding = declaration.append_attribute("encoding");
    attrEncoding.set_value("GB18030");

    auto root = doc.append_child("Control");

    auto nodeCmdType = root.append_child("CmdType");
    nodeCmdType.text().set("DeviceConfig");

    auto nodeSN = root.append_child("SN");
    nodeSN.text().set(std::to_string(_request_sn));

    auto nodeDeviceID = root.append_child("DeviceID");
    nodeDeviceID.text().set(_device->GetDeviceID());

    auto nodeSnapShotConfigCmd = root.append_child("SnapShotConfig");

    auto nodeSnapNum = nodeSnapShotConfigCmd.append_child("SnapNum");
    nodeSnapNum.text().set(m_tSnapShotConfigInfo.nSnapNum);

    auto nodeInterval = nodeSnapShotConfigCmd.append_child("Interval");
    nodeInterval.text().set(m_tSnapShotConfigInfo.nInterval);

    auto nodeUploadURL = nodeSnapShotConfigCmd.append_child("UploadURL");
    nodeUploadURL.text().set(m_tSnapShotConfigInfo.strUploadURL);

    auto nodeSessionID = nodeSnapShotConfigCmd.append_child("SessionID");
    nodeSessionID.text().set(m_tSnapShotConfigInfo.strSessionID);

    std::ostringstream os;
    doc.print(os);
    auto strGB18030 = ::ToMbcsString(os.str());
    return strGB18030;
}

const std::string VideoAlarmRecordRequest::make_manscdp_body()
{
    pugi::xml_document doc;
    auto declaration = doc.append_child(pugi::node_declaration);
    auto attrVersion = declaration.append_attribute("version");
    attrVersion.set_value("1.0");
    auto attrEncoding = declaration.append_attribute("encoding");
    attrEncoding.set_value("GB18030");

    auto root = doc.append_child("Control");

    auto nodeCmdType = root.append_child("CmdType");
    nodeCmdType.text().set("DeviceConfig");

    auto nodeSN = root.append_child("SN");
    nodeSN.text().set(std::to_string(_request_sn));

    auto nodeDeviceID = root.append_child("DeviceID");
    nodeDeviceID.text().set(_device->GetDeviceID());

    auto nodeVideoAlarmRecordCmd = root.append_child("VideoAlarmRecord");

    auto nodeRecordEnable = nodeVideoAlarmRecordCmd.append_child("RecordEnable");
    nodeRecordEnable.text().set(m_tVideoAlarmRecordInfo.nRecordEnable);

    auto nodeRecordTime = nodeVideoAlarmRecordCmd.append_child("RecordTime");
    nodeRecordTime.text().set(m_tVideoAlarmRecordInfo.nRecordTime);

    auto nodePreRecordTime = nodeVideoAlarmRecordCmd.append_child("PreRecordTime");
    nodePreRecordTime.text().set(m_tVideoAlarmRecordInfo.nPreRecordTime);

    auto nodeStreamNumber = nodeVideoAlarmRecordCmd.append_child("StreamNumber");
    nodeStreamNumber.text().set(m_tVideoAlarmRecordInfo.nStreamNumber);

    std::ostringstream os;
    doc.print(os);
    auto strGB18030 = ::ToMbcsString(os.str());
    return strGB18030;
}

const std::string PictureMaskRequest::make_manscdp_body()
{
    /* 每次创建Body的时候只提取一个录制计划中的一个时间段的报文 */
    std::string strXml = "";
    if (!m_dequeMsg.empty())
    {
        strXml = m_dequeMsg.front();
        m_dequeMsg.pop_front();
    }
    return strXml;
}

int PictureMaskRequest::SendMessage(bool bNeedCb)
{
     /* 清空队列 */
     m_dequeMsg.clear();
     { /* 组装报文 */
        pugi::xml_document doc;
        auto declaration = doc.append_child(pugi::node_declaration);
        auto attrVersion = declaration.append_attribute("version");
        attrVersion.set_value("1.0");
        auto attrEncoding = declaration.append_attribute("encoding");
        attrEncoding.set_value("GB18030");

        auto root = doc.append_child("Control");

        auto nodeCmdType = root.append_child("CmdType");
        nodeCmdType.text().set("DeviceConfig");

        auto nodeSN = root.append_child("SN");
        nodeSN.text().set(std::to_string(_request_sn));

        auto nodeDeviceID = root.append_child("DeviceID");
        nodeDeviceID.text().set(_device->GetDeviceID());

        auto nodePictureMaskCmd = root.append_child("PictureMask");

        auto nodeOn = nodePictureMaskCmd.append_child("On");
        nodeOn.text().set(m_tPictureMaskInfo.On);

        m_tPictureMaskInfo.SumNum = m_tPictureMaskInfo.vecRegionList.size();
        auto nodeSumNum = nodePictureMaskCmd.append_child("SumNum");
        nodeSumNum.text().set(m_tPictureMaskInfo.SumNum);

        auto nodeRegionList = nodePictureMaskCmd.append_child("RegionList");
        nodeRegionList.append_attribute("Num").set_value(std::to_string(1));

        auto nodeItem = nodeRegionList.append_child("Item");
        auto nodeSeq = nodeItem.append_child("Seq");
        auto nodePoint = nodeItem.append_child("Point");

        //配置数据
        for (auto it : m_tPictureMaskInfo.vecRegionList)
        {
            nodeSeq.text().set(it.Seq);
            std::string Piont = "";
            Piont += (std::to_string(it.nlx));
            Piont += ",";
            Piont += (std::to_string(it.nly));
            Piont += ",";
            Piont += (std::to_string(it.nrx));
            Piont += ",";
            Piont += (std::to_string(it.nry));
            nodePoint.text().set(Piont);

            std::ostringstream os;
            doc.print(os);
            auto strGB18030 = ::ToMbcsString(os.str());
            /* 存入队列 */
            m_dequeMsg.push_back(strGB18030);
        }
     }

     while (m_dequeMsg.size() > 0)
     {
         /* 调用基类发送流程 */
         MessageRequest::SendMessage(bNeedCb);
     }
     return 0;
}

const std::string FrameMirrorRequest::make_manscdp_body()
{
    pugi::xml_document doc;
    auto declaration = doc.append_child(pugi::node_declaration);
    auto attrVersion = declaration.append_attribute("version");
    attrVersion.set_value("1.0");
    auto attrEncoding = declaration.append_attribute("encoding");
    attrEncoding.set_value("GB18030");

    auto root = doc.append_child("Control");

    auto nodeCmdType = root.append_child("CmdType");
    nodeCmdType.text().set("DeviceConfig");

    auto nodeSN = root.append_child("SN");
    nodeSN.text().set(std::to_string(_request_sn));

    auto nodeDeviceID = root.append_child("DeviceID");
    nodeDeviceID.text().set(_device->GetDeviceID());

    auto nodeFrameMirrorConfigCmd = root.append_child("FrameMirror");
    nodeFrameMirrorConfigCmd.text().set(m_tFrameMirrorInfo.nFrameMirror);

    std::ostringstream os;
    doc.print(os);
    auto strGB18030 = ::ToMbcsString(os.str());
    return strGB18030;
}

const std::string ConfigDownloadRequest::make_manscdp_body()
{
    pugi::xml_document doc;
    auto declaration = doc.append_child(pugi::node_declaration);
    auto attrVersion = declaration.append_attribute("version");
    attrVersion.set_value("1.0");
    auto attrEncoding = declaration.append_attribute("encoding");
    attrEncoding.set_value("GB18030");

    auto root = doc.append_child("Query");

    auto nodeCmdType = root.append_child("CmdType");
    nodeCmdType.text().set("ConfigDownload");

    auto nodeSN = root.append_child("SN");
    nodeSN.text().set(std::to_string(_request_sn));

    auto nodeDeviceID = root.append_child("DeviceID");
    nodeDeviceID.text().set(_device->GetDeviceID());

    auto nodeOSDConfigCmd = root.append_child("ConfigType");

    std::string ConfigType("");
    if (DEVICE_QUERY_BASICPARAM == m_ConfigType)
    {
        ConfigType = "BasicParam";
    }
    else if (DEVICE_QUERY_VIDEOOPT == m_ConfigType)
    {
        ConfigType = "VideoParamOpt";
    }
    else if (DEVICE_QUERY_SVACENCODE == m_ConfigType)
    {
        ConfigType = "SVACEncodeConfig";
    }
    else if (DEVICE_QUERY_SVACDNCODE == m_ConfigType)
    {
        ConfigType = "SVACDncodeConfig";
    }
    else if (DEVICE_QUERY_VIDEATTRIBT == m_ConfigType)
    {
        ConfigType = "VideoParamAttribute";
    }
    else if (DEVICE_QUERY_VIDERECORD == m_ConfigType)
    {
        ConfigType = "VideoRecordPlan";
    }
    else if (DEVICE_QUERY_VIDEOALARM == m_ConfigType)
    {
        ConfigType = "VideoAlarmRecord";
    }
    else if (DEVICE_QUERY_PICTRUE == m_ConfigType)
    {
        ConfigType = "PictureMask";
    }
    else if (DEVICE_QUERY_FRAMEMIR == m_ConfigType)
    {
        ConfigType = "FrameMirror";
    }
    else if (DEVICE_QUERY_ALARM == m_ConfigType)
    {
        ConfigType = "AlarmReport";
    }
    else if (DEVICE_QUERY_OSD == m_ConfigType)
    {
        ConfigType = "OSDConfig";
    }
    else if (DEVICE_QUERY_SNAP == m_ConfigType)
    {
        ConfigType = "SnapShotConfig";
    }
    else if (DEVICE_QUERY_ALL == m_ConfigType)
    {
        ConfigType += "BasicParam";
        ConfigType += "/";
        ConfigType += "VideoParamOpt";
        ConfigType += "/";
        ConfigType += "SVACEncodeConfig";
        ConfigType += "/";
        ConfigType += "SVACDncodeConfig";
        ConfigType += "/";
        ConfigType += "VideoParamAttribute";
        ConfigType += "/";
        ConfigType += "VideoRecordPlan";
        ConfigType += "/";
        ConfigType += "VideoAlarmRecord";
        ConfigType += "/";
        ConfigType += "PictureMask";
        ConfigType += "/";
        ConfigType += "FrameMirror";
        ConfigType += "/";
        ConfigType += "AlarmReport";
        ConfigType += "/";
        ConfigType += "OSDConfig";
        ConfigType += "/";
        ConfigType += "SnapShotConfig";
    }
    nodeOSDConfigCmd.text().set(ConfigType);

    std::ostringstream os;
    doc.print(os);
    auto strGB18030 = ::ToMbcsString(os.str());
    return strGB18030;
}
const std::string CruiseTrackQueryRequest::make_manscdp_body()
{
    pugi::xml_document doc;
    auto declaration = doc.append_child(pugi::node_declaration);
    auto attrVersion = declaration.append_attribute("version");
    attrVersion.set_value("1.0");
    auto attrEncoding = declaration.append_attribute("encoding");
    attrEncoding.set_value("GB18030");

    auto root = doc.append_child("Query");

    auto nodeCmdType = root.append_child("CmdType");
    nodeCmdType.text().set("CruiseTrackQuery");

    auto nodeSN = root.append_child("SN");
    nodeSN.text().set(std::to_string(_request_sn));

    auto nodeDeviceID = root.append_child("DeviceID");
    nodeDeviceID.text().set(_device->GetDeviceID());

    auto nodeFrameMirrorConfigCmd = root.append_child("Number");
    nodeFrameMirrorConfigCmd.text().set(m_tCruiseTrackInfo.nNumber);

    std::ostringstream os;
    doc.print(os);
    auto strGB18030 = ::ToMbcsString(os.str());
    return strGB18030;
}

const std::string CruiseTrackListQueryRequest::make_manscdp_body()
{
    pugi::xml_document doc;
    auto declaration = doc.append_child(pugi::node_declaration);
    auto attrVersion = declaration.append_attribute("version");
    attrVersion.set_value("1.0");
    auto attrEncoding = declaration.append_attribute("encoding");
    attrEncoding.set_value("GB18030");

    auto root = doc.append_child("Query");

    auto nodeCmdType = root.append_child("CmdType");
    nodeCmdType.text().set("CruiseTrackListQuery");

    auto nodeSN = root.append_child("SN");
    nodeSN.text().set(std::to_string(_request_sn));

    auto nodeDeviceID = root.append_child("DeviceID");
    nodeDeviceID.text().set(_device->GetDeviceID());

    std::ostringstream os;
    doc.print(os);
    auto strGB18030 = ::ToMbcsString(os.str());
    return strGB18030;
}

const std::string PresetQueryRequest::make_manscdp_body()
{
    pugi::xml_document doc;
    auto declaration = doc.append_child(pugi::node_declaration);
    auto attrVersion = declaration.append_attribute("version");
    attrVersion.set_value("1.0");
    auto attrEncoding = declaration.append_attribute("encoding");
    attrEncoding.set_value("GB18030");

    auto root = doc.append_child("Query");

    auto nodeCmdType = root.append_child("CmdType");
    nodeCmdType.text().set("PresetQuery");

    auto nodeSN = root.append_child("SN");
    nodeSN.text().set(std::to_string(_request_sn));

    auto nodeDeviceID = root.append_child("DeviceID");
    nodeDeviceID.text().set(_device->GetDeviceID());

    std::ostringstream os;
    doc.print(os);
    auto strGB18030 = ::ToMbcsString(os.str());
    return strGB18030;
}

const std::string BroadcastRequest::make_manscdp_body()
{
    pugi::xml_document doc;
    auto declaration = doc.append_child(pugi::node_declaration);
    auto attrVersion = declaration.append_attribute("version");
    attrVersion.set_value("1.0");
    auto attrEncoding = declaration.append_attribute("encoding");
    attrEncoding.set_value("GB18030");

    auto root = doc.append_child("Notify");

    auto nodeCmdType = root.append_child("CmdType");
    nodeCmdType.text().set("Broadcast");

    auto nodeSN = root.append_child("SN");
    nodeSN.text().set(std::to_string(_request_sn));

    auto nodeSourceID = root.append_child("SourceID");
    nodeSourceID.text().set(m_tBroadcast.strSourceID);

    auto nodeTargetID = root.append_child("TargetID");
    nodeTargetID.text().set(m_tBroadcast.strTargetID);

    std::ostringstream os;
    doc.print(os);
    auto strGB18030 = ::ToMbcsString(os.str());
    return strGB18030;
}

const std::string TeleBootRequest::make_manscdp_body()
{
    pugi::xml_document doc;
    auto declaration = doc.append_child(pugi::node_declaration);
    auto attrVersion = declaration.append_attribute("version");
    attrVersion.set_value("1.0");
    auto attrEncoding = declaration.append_attribute("encoding");
    attrEncoding.set_value("GB18030");

    auto root = doc.append_child("Control");

    auto nodeCmdType = root.append_child("CmdType");
    nodeCmdType.text().set("DeviceControl");

    auto nodeSN = root.append_child("SN");
    nodeSN.text().set(std::to_string(_request_sn));

    auto nodeDeviceID = root.append_child("DeviceID");
    nodeDeviceID.text().set(m_strDeviceID);

    auto nodePTZCmd = root.append_child("TeleBoot");
    nodePTZCmd.text().set("Boot");

    std::ostringstream os;
    doc.print(os);
    auto strGB18030 = ::ToMbcsString(os.str());
    return strGB18030;
}

const std::string IFrameRequest::make_manscdp_body()
{
    pugi::xml_document doc;
    auto declaration = doc.append_child(pugi::node_declaration);
    auto attrVersion = declaration.append_attribute("version");
    attrVersion.set_value("1.0");
    auto attrEncoding = declaration.append_attribute("encoding");
    attrEncoding.set_value("GB18030");

    auto root = doc.append_child("Control");

    auto nodeCmdType = root.append_child("CmdType");
    nodeCmdType.text().set("DeviceControl");

    auto nodeSN = root.append_child("SN");
    nodeSN.text().set(std::to_string(_request_sn));

    auto nodeDeviceID = root.append_child("DeviceID");
    nodeDeviceID.text().set(m_strDeviceID);

    auto nodePTZCmd = root.append_child("IFrameCmd");
    nodePTZCmd.text().set("Send");

    std::ostringstream os;
    doc.print(os);
    auto strGB18030 = ::ToMbcsString(os.str());
    return strGB18030;
}

const std::string DragZoomInOutRequest::make_manscdp_body()
{
    pugi::xml_document doc;
    auto declaration = doc.append_child(pugi::node_declaration);
    auto attrVersion = declaration.append_attribute("version");
    attrVersion.set_value("1.0");
    auto attrEncoding = declaration.append_attribute("encoding");
    attrEncoding.set_value("GB18030");

    auto root = doc.append_child("Control");

    auto nodeCmdType = root.append_child("CmdType");
    nodeCmdType.text().set("DeviceControl");

    auto nodeSN = root.append_child("SN");
    nodeSN.text().set(std::to_string(_request_sn));

    auto nodeDeviceID = root.append_child("DeviceID");
    nodeDeviceID.text().set(_device->GetDeviceID());

    pugi::xml_node nodeDragZoomInCmd;
    if(m_tDragZoomInfo.bIsZooIn)
    {
        nodeDragZoomInCmd = root.append_child("DragZoomIn");
    }
    else
    {
        nodeDragZoomInCmd = root.append_child("DragZoomOut");
    }

    auto nodeLength = nodeDragZoomInCmd.append_child("Length");
    nodeLength.text().set(m_tDragZoomInfo.nLength);

    auto nodeWidth = nodeDragZoomInCmd.append_child("Width");
    nodeWidth.text().set(m_tDragZoomInfo.nWidth);

    auto nodeMidPointX = nodeDragZoomInCmd.append_child("MidPointX");
    nodeMidPointX.text().set(m_tDragZoomInfo.nMidPointX);

    auto nodeMidPointY = nodeDragZoomInCmd.append_child("MidPointY");
    nodeMidPointY.text().set(m_tDragZoomInfo.nMidPointY);

    auto nodeLengthX = nodeDragZoomInCmd.append_child("LengthX");
    nodeLengthX.text().set(m_tDragZoomInfo.nLengthX);

    auto nodeLengthY = nodeDragZoomInCmd.append_child("LengthY");
    nodeLengthY.text().set(m_tDragZoomInfo.nLengthY);

    std::ostringstream os;
    doc.print(os);
    auto strGB18030 = ::ToMbcsString(os.str());
    return strGB18030;
}

const std::string PTZPreciseCtrlRequest::make_manscdp_body()
{
    pugi::xml_document doc;
    auto declaration = doc.append_child(pugi::node_declaration);
    auto attrVersion = declaration.append_attribute("version");
    attrVersion.set_value("1.0");
    auto attrEncoding = declaration.append_attribute("encoding");
    attrEncoding.set_value("GB18030");

    auto root = doc.append_child("Control");

    auto nodeCmdType = root.append_child("CmdType");
    nodeCmdType.text().set("DeviceControl");

    auto nodeSN = root.append_child("SN");
    nodeSN.text().set(std::to_string(_request_sn));

    auto nodeDeviceID = root.append_child("DeviceID");
    nodeDeviceID.text().set(_device->GetDeviceID());

    auto nodePTZPreciseCtrl = root.append_child("PTZPreciseCtrl");
    
    auto nodePan = nodePTZPreciseCtrl.append_child("Pan");
    nodePan.text().set(m_tPTZPreciseCtrlInfo.dPan);

    auto nodeTilt = nodePTZPreciseCtrl.append_child("Tilt");
    nodeTilt.text().set(m_tPTZPreciseCtrlInfo.dTilt);

    auto nodeZoom = nodePTZPreciseCtrl.append_child("Zoom");
    nodeZoom.text().set(m_tPTZPreciseCtrlInfo.dZoom);

    std::ostringstream os;
    doc.print(os);
    auto strGB18030 = ::ToMbcsString(os.str());
    return strGB18030;
}

const std::string RecordCmdRequest::make_manscdp_body()
{
    pugi::xml_document doc;
    auto declaration = doc.append_child(pugi::node_declaration);
    auto attrVersion = declaration.append_attribute("version");
    attrVersion.set_value("1.0");
    auto attrEncoding = declaration.append_attribute("encoding");
    attrEncoding.set_value("GB18030");

    auto root = doc.append_child("Control");

    auto nodeCmdType = root.append_child("CmdType");
    nodeCmdType.text().set("DeviceControl");

    auto nodeSN = root.append_child("SN");
    nodeSN.text().set(std::to_string(_request_sn));

    auto nodeDeviceID = root.append_child("DeviceID");
    nodeDeviceID.text().set(_device->GetDeviceID());

    auto nodeRecordCmd = root.append_child("RecordCmd");
    if(m_tRecordCmdInfo.bIsRecord)
    {
        nodeRecordCmd.text().set("Record");
    }
    else
    {
        nodeRecordCmd.text().set("StopRecord");
    }
    
    auto nodeStreamNumber = root.append_child("StreamNumber");
    nodeStreamNumber.text().set(m_tRecordCmdInfo.nStreamNumber);

    std::ostringstream os;
    doc.print(os);
    auto strGB18030 = ::ToMbcsString(os.str());
    return strGB18030;
}

const std::string AlarmCmdRequest::make_manscdp_body()
{
    pugi::xml_document doc;
    auto declaration = doc.append_child(pugi::node_declaration);
    auto attrVersion = declaration.append_attribute("version");
    attrVersion.set_value("1.0");
    auto attrEncoding = declaration.append_attribute("encoding");
    attrEncoding.set_value("GB18030");

    auto root = doc.append_child("Control");

    auto nodeCmdType = root.append_child("CmdType");
    nodeCmdType.text().set("DeviceControl");

    auto nodeSN = root.append_child("SN");
    nodeSN.text().set(std::to_string(_request_sn));

    auto nodeDeviceID = root.append_child("DeviceID");
    nodeDeviceID.text().set(_device->GetDeviceID());

    auto nodeAlarmCmd = root.append_child("AlarmCmd");
    nodeAlarmCmd.text().set("ResetAlarm");
    
    auto nodeInfo = root.append_child("Info");
    auto nodeAlarmMethod = nodeInfo.append_child("AlarmMethod");
    nodeAlarmMethod.text().set(m_tAlarmCmdInfo.strAlarmMethod);
    auto nodeAlarmType = nodeInfo.append_child("AlarmType");
    nodeAlarmType.text().set(m_tAlarmCmdInfo.strAlarmType);

    std::ostringstream os;
    doc.print(os);
    auto strGB18030 = ::ToMbcsString(os.str());
    return strGB18030;
}

const std::string HomePositionRequest::make_manscdp_body()
{
    pugi::xml_document doc;
    auto declaration = doc.append_child(pugi::node_declaration);
    auto attrVersion = declaration.append_attribute("version");
    attrVersion.set_value("1.0");
    auto attrEncoding = declaration.append_attribute("encoding");
    attrEncoding.set_value("GB18030");

    auto root = doc.append_child("Control");

    auto nodeCmdType = root.append_child("CmdType");
    nodeCmdType.text().set("DeviceControl");

    auto nodeSN = root.append_child("SN");
    nodeSN.text().set(std::to_string(_request_sn));

    auto nodeDeviceID = root.append_child("DeviceID");
    nodeDeviceID.text().set(_device->GetDeviceID());

    auto nodeHomePosition = root.append_child("HomePosition");
    
    auto nodeEnabled = nodeHomePosition.append_child("Enabled");
    nodeEnabled.text().set(m_tHomePositionInfo.nEnabled);

    auto nodeResetTime = nodeHomePosition.append_child("ResetTime");
    nodeResetTime.text().set(m_tHomePositionInfo.nResetTime);

    auto nodePresetIndex = nodeHomePosition.append_child("PresetIndex");
    nodePresetIndex.text().set(m_tHomePositionInfo.nPresetIndex);

    std::ostringstream os;
    doc.print(os);
    auto strGB18030 = ::ToMbcsString(os.str());
    return strGB18030;
}

const std::string DeviceUpgradeRequest::make_manscdp_body()
{
    pugi::xml_document doc;
    auto declaration = doc.append_child(pugi::node_declaration);
    auto attrVersion = declaration.append_attribute("version");
    attrVersion.set_value("1.0");
    auto attrEncoding = declaration.append_attribute("encoding");
    attrEncoding.set_value("GB18030");

    auto root = doc.append_child("Control");

    auto nodeCmdType = root.append_child("CmdType");
    nodeCmdType.text().set("DeviceControl");

    auto nodeSN = root.append_child("SN");
    nodeSN.text().set(std::to_string(_request_sn));

    auto nodeDeviceID = root.append_child("DeviceID");
    nodeDeviceID.text().set(_device->GetDeviceID());

    auto nodeUpgrade = root.append_child("DeviceUpgrade");
    
    auto nodeFirmware = nodeUpgrade.append_child("Firmware");
    nodeFirmware.text().set(m_tDeviceUpgradeInfo.strFirmware);

    auto nodeFileURL = nodeUpgrade.append_child("FileURL");
    nodeFileURL.text().set(m_tDeviceUpgradeInfo.strFileURL);

    auto nodeManufacturer = nodeUpgrade.append_child("Manufacturer");
    nodeManufacturer.text().set(m_tDeviceUpgradeInfo.strManufacturer);

    auto nodeSessionID = nodeUpgrade.append_child("SessionID");
    nodeSessionID.text().set(m_tDeviceUpgradeInfo.strSessionID);

    std::ostringstream os;
    doc.print(os);
    auto strGB18030 = ::ToMbcsString(os.str());
    return strGB18030;
}

const std::string TargetTrackRequest::make_manscdp_body()
{
    pugi::xml_document doc;
    auto declaration = doc.append_child(pugi::node_declaration);
    auto attrVersion = declaration.append_attribute("version");
    attrVersion.set_value("1.0");
    auto attrEncoding = declaration.append_attribute("encoding");
    attrEncoding.set_value("GB18030");

    auto root = doc.append_child("Control");

    auto nodeCmdType = root.append_child("CmdType");
    nodeCmdType.text().set("DeviceControl");

    auto nodeSN = root.append_child("SN");
    nodeSN.text().set(std::to_string(_request_sn));

    auto nodeDeviceID = root.append_child("DeviceID");
    nodeDeviceID.text().set(_device->GetDeviceID());

    auto nodeTargetTrack = root.append_child("TargetTrack");
    if(m_tTargetTrackInfo.TrackType == GB28181::TargetTrackInfo_S::TYPE::AUTO)
    {
        nodeTargetTrack.text().set("Auto");
    }
    else if(m_tTargetTrackInfo.TrackType == GB28181::TargetTrackInfo_S::TYPE::MANUAL)
    {
        nodeTargetTrack.text().set("Manual");
    }
    else if(m_tTargetTrackInfo.TrackType == GB28181::TargetTrackInfo_S::TYPE::STOP)
    {
        nodeTargetTrack.text().set("Stop");
    }
    
    auto nodeDeviceID2 = root.append_child("DeviceID2");
    nodeDeviceID2.text().set(m_tTargetTrackInfo.strDeviceID2);

    auto nodeTargetArea = root.append_child("TargetArea");
    
    auto nodeLength = nodeTargetArea.append_child("Length");
    nodeLength.text().set(m_tTargetTrackInfo.nLength);

    auto nodeWidth = nodeTargetArea.append_child("Width");
    nodeWidth.text().set(m_tTargetTrackInfo.nWidth);

    auto nodeMidPointX = nodeTargetArea.append_child("MidPointX");
    nodeMidPointX.text().set(m_tTargetTrackInfo.nMidPointX);

    auto nodeMidPointY = nodeTargetArea.append_child("MidPointY");
    nodeMidPointY.text().set(m_tTargetTrackInfo.nMidPointY);

    auto nodeLengthX = nodeTargetArea.append_child("LengthX");
    nodeLengthX.text().set(m_tTargetTrackInfo.nLengthX);

    auto nodeLengthY = nodeTargetArea.append_child("LengthY");
    nodeLengthY.text().set(m_tTargetTrackInfo.nLengthY);

    std::ostringstream os;
    doc.print(os);
    auto strGB18030 = ::ToMbcsString(os.str());
    return strGB18030;
}

const std::string HomePositionQueryRequest::make_manscdp_body()
{
    pugi::xml_document doc;
    auto declaration = doc.append_child(pugi::node_declaration);
    auto attrVersion = declaration.append_attribute("version");
    attrVersion.set_value("1.0");
    auto attrEncoding = declaration.append_attribute("encoding");
    attrEncoding.set_value("GB18030");

    auto root = doc.append_child("Query");

    auto nodeCmdType = root.append_child("CmdType");
    nodeCmdType.text().set("HomePositionQuery");

    auto nodeSN = root.append_child("SN");
    nodeSN.text().set(std::to_string(_request_sn));

    auto nodeDeviceID = root.append_child("DeviceID");
    nodeDeviceID.text().set(_device->GetDeviceID());

    std::ostringstream os;
    doc.print(os);
    auto strGB18030 = ::ToMbcsString(os.str());
    return strGB18030;
}

const std::string PTZPositionRequest::make_manscdp_body()
{
    pugi::xml_document doc;
    auto declaration = doc.append_child(pugi::node_declaration);
    auto attrVersion = declaration.append_attribute("version");
    attrVersion.set_value("1.0");
    auto attrEncoding = declaration.append_attribute("encoding");
    attrEncoding.set_value("GB18030");

    auto root = doc.append_child("Query");

    auto nodeCmdType = root.append_child("CmdType");
    nodeCmdType.text().set("PTZPosition");

    auto nodeSN = root.append_child("SN");
    nodeSN.text().set(std::to_string(_request_sn));

    auto nodeDeviceID = root.append_child("DeviceID");
    nodeDeviceID.text().set(_device->GetDeviceID());

    std::ostringstream os;
    doc.print(os);
    auto strGB18030 = ::ToMbcsString(os.str());
    return strGB18030;
}