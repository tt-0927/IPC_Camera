#include "XmlParser.h"
#include "SipUtils.h"
#include <string.h>
using namespace SIP;

bool XmlParser::Parse(const char *data, int len, pugi::xml_document &doc)
{
	if (data == nullptr || len <= 0)
	{
		return false;
	}
	/* 解析前可能需要将GB18030转为UTF-8 */
	auto strGB18030 = std::string(data, len);
	auto strUTF8 = ::ToUtf8String(strGB18030);
	auto ret = doc.load_buffer(strUTF8.c_str(), strUTF8.length(), pugi::parse_default);
	if (ret.status == pugi::status_ok)
	{
		return true;
	}
	else
	{
		MLOG_ERROR("xml解析失败\n%s", data);
	}

	return false;
}

bool XmlParser::ParseHeader(manscdp_msgbody_header_t &header, pugi::xml_document &doc)
{
	auto root = doc.first_child();
	/* 解析Category，一级指令 */
	if (strcmp(root.name(), "Control") == 0)
	{
		header.cmd_category = MANSCDP_CMD_CATEGORY_CONTROL;
	}
	else if (strcmp(root.name(), "Query") == 0)
	{
		header.cmd_category = MANSCDP_CMD_CATEGORY_QUERY;
	}
	else if (strcmp(root.name(), "Notify") == 0)
	{
		header.cmd_category = MANSCDP_CMD_CATEGORY_NOTIFY;
	}
	else if (strcmp(root.name(), "Response") == 0)
	{
		header.cmd_category = MANSCDP_CMD_CATEGORY_RESPONSE;
	}
	else
	{
		header.cmd_category = MANSCDP_CMD_CATEGORY_UNKNOWN;
		return false;
	}

	/* 解析CmdType，二级指令 */
	auto node = root.child("CmdType");
	if (node.empty())
	{
		return false;
	}
	auto value = node.text().as_string();
	if (strcmp(value, "DeviceControl") == 0)
	{
		header.cmd_type = MANSCDP_CONTROL_CMD_DEVICE_CONTROL;
	}
	else if (strcmp(value, "DeviceConfig") == 0)
	{
		header.cmd_type = MANSCDP_CONTROL_CMD_DEVICE_CONFIG;
	}
	else if (strcmp(value, "DeviceStatus") == 0)
	{
		header.cmd_type = MANSCDP_QUERY_CMD_DEVICE_STATUS;
	}
	else if (strcmp(value, "Catalog") == 0)
	{
		header.cmd_type = MANSCDP_QUERY_CMD_CATALOG;
	}
	else if (strcmp(value, "DeviceInfo") == 0)
	{
		header.cmd_type = MANSCDP_QUERY_CMD_DEVICE_INFO;
	}
	else if (strcmp(value, "RecordInfo") == 0)
	{
		header.cmd_type = MANSCDP_QUERY_CMD_RECORD_INFO;
	}
	else if (strcmp(value, "Alarm") == 0 && header.cmd_category == MANSCDP_CMD_CATEGORY_QUERY)
	{
		header.cmd_type = MANSCDP_QUERY_CMD_ALARM;
	}
	else if (strcmp(value, "Alarm") == 0 && header.cmd_category == MANSCDP_CMD_CATEGORY_NOTIFY)
	{
		header.cmd_type = MANSCDP_NOTIFY_CMD_ALARM;
	}
	else if (strcmp(value, "ConfigDownload") == 0)
	{
		header.cmd_type = MANSCDP_QUERY_CMD_CONFIG_DOWNLOAD;
	}
	else if (strcmp(value, "PresetQuery") == 0)
	{
		header.cmd_type = MANSCDP_QUERY_CMD_PRESET_QUERY;
	}
	else if (strcmp(value, "MobilePosition") == 0)
	{
		header.cmd_type = MANSCDP_QUERY_CMD_MOBILE_POSITION;
	}
	else if (strcmp(value, "Keepalive") == 0)
	{
		header.cmd_type = MANSCDP_NOTIFY_CMD_KEEPALIVE;
	}
	else if (strcmp(value, "MediaStatus") == 0)
	{
		header.cmd_type = MANSCDP_NOTIFY_CMD_MEDIA_STATUS;
	}
	else if (strcmp(value, "Broadcast") == 0)
	{
		header.cmd_type = MANSCDP_NOTIFY_CMD_BROADCASE;
	}
	else if (strcmp(value, "CruiseTrackQuery") == 0)
	{
		header.cmd_type = MANSCDP_QUERY_CMD_CRUISETRACKQUERY;
	}
	else if(strcmp(value, "CruiseTrackListQuery") == 0)
	{
		header.cmd_type = MANSCDP_QUERY_CMD_CRUISETRACKLISTQUERY;
	}
	else if(strcmp(value, "HomePositionQuery") == 0)
	{
		header.cmd_type = MANSCDP_QUERY_CMD_HOMEPOSITIONQUERY;
	}
	else if(strcmp(value, "PTZPosition") == 0)
	{
		header.cmd_type = MANSCDP_QUERY_CMD_PTZPOSITION;
	}
	else if(strcmp(value, "UploadSnapShotFinished") == 0)
	{
		header.cmd_type = MANSCDP_NOTIFY_CMD_UPLOADSNAPSHOTFINISHED;
	}
	else
	{
		return false;
	}

	header.strCmdType = value;

	node = root.child("DeviceID");

	if (node.empty())
	{
		/* 尝试解析TargetID */
		node = root.child("TargetID");
		if (node.empty())
		{
			MLOG_ERROR("设备id解析失败");
			return false;
		}
		MLOG_INFO("解析到的设备id：%s", node.text().as_string());
	}
	header.strDevID = node.text().as_string();

	node = root.child("SN");
	if (node.empty())
	{
		return false;
	}
	header.strSN = node.text().as_string();

	return true;
}
