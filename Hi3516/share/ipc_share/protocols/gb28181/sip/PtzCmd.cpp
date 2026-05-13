/***
 * @FilePath     : PtzCmd.cpp
 * @Author       : cyc
 * @Date         : 2024-09-19 10:03:33
 * @LastEditors  : cyc
 * @LastEditTime : 2024-10-25 17:33:02
 * @Description  : 云台控制命令类，提供对云台的操控指令和解析功能，详细见GB/T-28181-2022文档。
 */

#include "PtzCmd.h"
#include "ModuleLog.h"
#include <sstream>
#include <iomanip>

using namespace SIP;
std::string PtzCmd::cmdString(PtzCommand_E enLeftRight, PtzCommand_E enUpDown, PtzCommand_E enInOut, int nMoveSpeed, int nZoomSpeed)
{
	int nCmdCode = 0;
	int nCheckCode = 0;
	std::stringstream ss;

	if (enLeftRight == PtzCommand_E::RIGHT)
	{
		/* 右移 */
		nCmdCode = PTZ_RIGHT_CODE;
	}
	else if (enLeftRight == PtzCommand_E::LEFT)
	{
		/* 左移 */
		nCmdCode = PTZ_LEFT_CODE;
	}
	if (enUpDown == PtzCommand_E::DOWN)
	{
		/* 下移 */
		nCmdCode |= PTZ_DOWN_CODE;
	}
	else if (enUpDown == PtzCommand_E::UP)
	{
		/* 上移 */
		nCmdCode |= PTZ_UP_CODE;
	}
	if (enInOut == PtzCommand_E::ZOOMIN)
	{
		/* 放大 */
		nCmdCode |= PTZ_ZOOM_IN_CODE;
	}
	else if (enInOut == PtzCommand_E::ZOOMOUT)
	{
		/* 缩小 */
		nCmdCode |= PTZ_ZOOM_OUT_CODE;
	}

	/* 前三字节 固定值 */
	ss << "A50F01";
	/* 字节4 指令码,如ptz、FI、预置位等 */
	ss << std::setfill('0') << std::setw(2) << std::hex << std::uppercase << nCmdCode;
	/* 字节5 水平控制速度 */
	ss << std::setfill('0') << std::setw(2) << std::hex << std::uppercase << nMoveSpeed;
	/* 字节6 垂直控制速度 */
	ss << std::setfill('0') << std::setw(2) << std::hex << std::uppercase << nMoveSpeed;
	/* 字节7 高四位 焦距控制速度 */
	ss << std::setfill('0') << std::setw(1) << std::hex << std::uppercase << nZoomSpeed;
	/* 字节7 低四位 */
	ss << "0";
	/* 字节8 校验码  字节8=(字节1+字节2+字节3+字节4+字节5+字节6+字节7)%256 */
	nCheckCode =
		(0xA5 + 0x0F + 0x01 + nCmdCode + nMoveSpeed + nMoveSpeed + (nZoomSpeed << 4)) % 0x100;
	ss << std::setfill('0') << std::setw(2) << std::hex << std::uppercase << nCheckCode;

	MLOG_INFO("发送的指令码: %s", ss.str().c_str());
	return ss.str();
}

std::string PtzCmd::cmdCode(int nFourthByte, int nFifthByte, int nSixthByte, int nSeventhByte)
{
	std::stringstream ss;
	int nCheckCode = 0;
	/* 前三字节 固定值 */
	ss << "A50F01";
	/* 字节4 指令码,如ptz、FI、预置位等 */
	ss << std::setfill('0') << std::setw(2) << std::hex << std::uppercase << nFourthByte;
	/* 字节5 水平控制速度 */
	ss << std::setfill('0') << std::setw(2) << std::hex << std::uppercase << nFifthByte;
	/* 字节6 垂直控制速度 */
	ss << std::setfill('0') << std::setw(2) << std::hex << std::uppercase << nSixthByte;
	/* 字节7 高四位 焦距控制速度 */
	ss << std::setfill('0') << std::setw(1) << std::hex << std::uppercase << nSeventhByte;
	/* 字节7 低四位 */
	ss << "0";
	/* 字节8 校验码  字节8=(字节1+字节2+字节3+字节4+字节5+字节6+字节7)%256 */
	nCheckCode =
		(0xA5 + 0x0F + 0x01 + nFourthByte + nFifthByte + nSixthByte + (nSeventhByte & 0xF0)) % 0x100;
	ss << std::setfill('0') << std::setw(2) << std::hex << std::uppercase << nCheckCode;

	return ss.str();
}

std::string PtzCmd::cmdLens(PtzCommand_E enIris, PtzCommand_E enFocus, int nIris_speed, int nFocus_speed)
{
	int nCmdCode = 0;
	std::stringstream ss;
	int nCheckCode = 0;

	if (enIris == PtzCommand_E::IFIS_SHRINK)
	{
		/* 缩小 */
		nCmdCode = FI_IRIS_OUT_CODE;
	}
	else if (enIris == PtzCommand_E::IFIS_AMPLIFICATION)
	{
		/* 放大 */
		nCmdCode = FI_IRIS_IN_CODE;
	}

	if (enFocus == PtzCommand_E::FOCUS_NEAR)
	{
		/* 近 */
		nCmdCode |= FI_FOCUS_NEAR_CODE;
	}
	else if (enFocus == PtzCommand_E::FOCUS_FAR)
	{
		/* 远 */
		nCmdCode |= FI_FOCUS_FAR_CODE;
	}

	/* 前三字节 固定值 */
	ss << "A50F01";
	/* 字节4 指令码,如ptz、FI、预置位等 */
	ss << std::setfill('0') << std::setw(2) << std::hex << std::uppercase << nCmdCode;
	/* 字节5 聚焦速度 */
	ss << std::setfill('0') << std::setw(2) << std::hex << std::uppercase << nFocus_speed;
	/* 字节6 光圈速度 */
	ss << std::setfill('0') << std::setw(2) << std::hex << std::uppercase << nIris_speed;
	/* 字节7 无含义 */
	ss << std::setfill('0') << std::setw(2) << std::hex << std::uppercase << 0;

	/* 字节8 校验码  字节8=(字节1+字节2+字节3+字节4+字节5+字节6+字节7)%256 */
	nCheckCode =
		(0xA5 + 0x0F + 0x01 + nCmdCode + nIris_speed + nFocus_speed + 0x00) % 0x100;
	ss << std::setfill('0') << std::setw(2) << std::hex << std::uppercase << nCheckCode;

	MLOG_INFO("发送的指令码: %s", ss.str().c_str());
	return ss.str();
}

/**
 * @brief 解析云台控制指令。
 * @param ctrlcmd 用于存储解析结果的 control_cmd_t 对象。
 * @param cmdstr 云台控制指令字符串。
 * @return int 解析结果，0 表示成功，其他值表示失败。
 * @note 解析逻辑详细见GB/T-28181-2022文档 110页扫描指令格式
 */
int PtzParser::ParseControlCmd(control_cmd_t &ctrlcmd, const std::string &cmdstr)
{
	char achByte[8];
	memset(achByte, 0, sizeof(achByte));
	sscanf(cmdstr.c_str(), "%02hhx%02hhx%02hhx%02hhx%02hhx%02hhx%02hhx%02hhx", &achByte[0], &achByte[1], &achByte[2], &achByte[3], &achByte[4], &achByte[5], &achByte[6], &achByte[7]);
	/* 指令码 */
	chB4 = achByte[3];
	chB5 = achByte[4];
	chB6 = achByte[5];
	chB7 = achByte[6];

	/* PTZ命令 */
	if (PARSE_CMD_PTZ(chB4))
	{
		parse_ptz(ctrlcmd);
	}
	/* FI命令 */
	else if (PARSE_CMD_FI(chB4))
	{
		parse_fi(ctrlcmd);
	}
	/* 预置位命令 */
	else if (chB4 == PRESET_SET_CODE || chB4 == PRESET_CALL_CODE || chB4 == PRESET_DEL_CODE)
	{
		parse_preset(ctrlcmd);
	}
	/* 巡逻命令 */
	else if (chB4 >= PATROL_ADD_CODE && chB4 <= PATROL_START_CODE)
	{
		parse_patrol(ctrlcmd);
	}
	/* 扫描命令*/
	else if (PARSE_CMD_SCNA(chB4))
	{
		parse_scan(ctrlcmd);
	}
	/* 指令码 为 0x00，停止指令 */
	else if (chB4 == 0x00)
	{
		ctrlcmd.ctrltype = CONTROL_STOP;
	}

	return 0;
}

/**
 * @brief 解析 PTZ 控制指令。
 * @param b 云台控制指令字符串。
 * @param ctrlcmd 用于存储解析结果的 control_cmd_t 对象。
 * @note 解析逻辑详细见GB/T-28181-2022文档 107页指令格式
 */
void PtzParser::parse_ptz(control_cmd_t &ctrlcmd)
{
	ctrlcmd.ctrltype = PTZ_TYPE;

	/* zoom in out的判断 */
	if (PARSE_CMD_ZOOM_IN(chB4))
	{
		ctrlcmd.ptz_zoom.cmdtype = ptz_cmd_zoom_t::ZOOM_IN;
	}
	else if (PARSE_CMD_ZOOM_OUT(chB4))
	{
		ctrlcmd.ptz_zoom.cmdtype = ptz_cmd_zoom_t::ZOOM_OUT;
	}
	ctrlcmd.ptz_zoom.speed = PARSE_CMD_ZOOM_SPEED(chB7);

	/* 云台垂直方向的判断 */
	if (PARSE_CMD_TILT_DOWN(chB4))
	{
		ctrlcmd.ptz_tilt.cmdtype = ptz_cmd_tilt_t::TILT_DOWN;
	}
	else if (PARSE_CMD_TILT_UP(chB4))
	{
		ctrlcmd.ptz_tilt.cmdtype = ptz_cmd_tilt_t::TILT_UP;
	}
	ctrlcmd.ptz_tilt.speed = chB6;

	/* 云台水平方向的判断 */
	if (PARSE_CMD_PAN_RIGHT(chB4))
	{
		ctrlcmd.ptz_pan.cmdtype = ptz_cmd_pan_t::PAN_RIGHT;
	}
	else if (PARSE_CMD_PAN_LEFT(chB4))
	{
		ctrlcmd.ptz_pan.cmdtype = ptz_cmd_pan_t::PAN_LEFT;
	}
	/* 云台速度 */
	ctrlcmd.ptz_pan.speed = chB5;
}

/**
 * @brief 解析与焦点相关的控制指令。
 * @param b 云台控制指令字符串。
 * @param ctrlcmd 用于存储解析结果的 control_cmd_t 对象。
 * @note 解析逻辑详细见GB/T-28181-2022文档 107页指令格式
 */
void PtzParser::parse_fi(control_cmd_t &ctrlcmd)
{
	ctrlcmd.ctrltype = FI_TYPE;

	/* 光圈放大 */
	if (PARSE_CMD_IRIS_IN(chB4))
	{
		ctrlcmd.fi_iris.cmdtype = fi_cmd_iris_t::IFIS_AMPLIFICATION;
	}
	/* 光圈缩小 */
	else if (PARSE_CMD_IRIS_OUT(chB4))
	{
		ctrlcmd.fi_iris.cmdtype = fi_cmd_iris_t::IFIS_SHRINK;
	}
	/* 光圈速度 */
	ctrlcmd.fi_iris.speed = chB6;

	/* 聚焦调远 */
	if (PARSE_CMD_FOCUS_FAR(chB4))
	{
		ctrlcmd.fi_focus.cmdtype = fi_cmd_focus_t::FOCUS_FAR;
	}
	/* 聚焦调近 */
	else if (PARSE_CMD_FOCUS_NEAR(chB4))
	{
		ctrlcmd.fi_focus.cmdtype = fi_cmd_focus_t::FOCUS_NEAR;
	}
	/* 聚焦速度 */
	ctrlcmd.fi_focus.speed = chB5;
}

/**
 * @brief 解析与预设点相关的控制指令。
 * @param b 云台控制指令字符串。
 * @param ctrlcmd 用于存储解析结果的 control_cmd_t 对象。
 * @note 解析逻辑详细见GB/T-28181-2022文档 107页指令格式
 */
void PtzParser::parse_preset(control_cmd_t &ctrlcmd)
{
	ctrlcmd.ctrltype = PRESET_TYPE;
	/* 设置预置位 */
	if (chB4 == PRESET_SET_CODE)
	{
		ctrlcmd.preset.cmdtype = preset_cmd_t::PRESET_SET;
	}
	/* 调用预置位 */
	else if (chB4 == PRESET_CALL_CODE)
	{
		ctrlcmd.preset.cmdtype = preset_cmd_t::PRESET_CALL;
	}
	/* 删除预置位 */
	else if (chB4 == PRESET_DEL_CODE)
	{
		ctrlcmd.preset.cmdtype = preset_cmd_t::PRESET_DELE;
	}
	/* 预置位速度值 */
	ctrlcmd.preset.index = chB6;
}

/**
 * @brief 解析巡逻功能相关的控制指令。
 * @param b 云台控制指令字符串。
 * @param ctrlcmd 用于存储解析结果的 control_cmd_t 对象。
 * @note 解析逻辑详细见GB/T-28181-2022文档 107页指令格式
 */
void PtzParser::parse_patrol(control_cmd_t &ctrlcmd)
{
	/* 巡航类型 */
	ctrlcmd.ctrltype = PATROL_TYPE;
	/* 巡航组号 */
	ctrlcmd.patrol.patrol_id = chB5;
	/* 预置位号 */
	ctrlcmd.patrol.preset_id = chB6;
	int nSpeed = chB7 & 0xf0;
	nSpeed = (nSpeed << 4) | chB6;
	/* 巡航速度 */
	ctrlcmd.patrol.value = nSpeed;
	/* 添加巡航点 */
	if (chB4 == PATROL_ADD_CODE)
	{
		ctrlcmd.patrol.cmdtype = patrol_cmd_t::PATROL_ADD;
	}
	/* 删除巡航点 */
	else if (chB4 == PATROL_DEL_CODE)
	{
		ctrlcmd.patrol.cmdtype = patrol_cmd_t::PATROL_DELE;
	}
	/* 设置巡航点速度 */
	else if (chB4 == PATROL_SET_SPEED_CODE)
	{
		ctrlcmd.patrol.cmdtype = patrol_cmd_t::PATROL_SET_SPEED;
	}
	/* 设置巡航时间 */
	else if (chB4 == PATROL_SET_TIME_CODE)
	{
		ctrlcmd.patrol.cmdtype = patrol_cmd_t::PATROL_SET_TIME;
	}
	/* 开始巡航 */
	else if (chB4 == PATROL_START_CODE)
	{
		ctrlcmd.patrol.cmdtype = patrol_cmd_t::PATROL_START;
	}
}

/**
 * @brief 解析扫描功能相关的控制指令。
 * @param b 云台控制指令字符串。
 * @param ctrlcmd 用于存储解析结果的 control_cmd_t 对象。
 * @note 解析逻辑详细见GB/T-28181-2022文档 107页指令格式
 */
void PtzParser::parse_scan(control_cmd_t &ctrlcmd)
{
	ctrlcmd.ctrltype = SCAN_TYPE;
	int nSpeed = chB7;
	nSpeed = (nSpeed << 4) | chB6;
	ctrlcmd.autoscan.speed = nSpeed;
	ctrlcmd.autoscan.scan_id = chB5;
	/* 开始扫描 */
	if (PARSE_CMD_SCNA_START(chB4, chB6))
	{
		ctrlcmd.autoscan.cmdtype = scan_cmd_t::SCAN_START;
	}
	/* 向左扫描 */
	else if (PARSE_CMD_SCNA_LEFT(chB4, chB6))
	{
		ctrlcmd.autoscan.cmdtype = scan_cmd_t::SCAN_SET_LEFT_BOADER;
	}
	/* 向右扫描 */
	else if (PARSE_CMD_SCNA_RIGHT(chB4, chB6))
	{
		ctrlcmd.autoscan.cmdtype = scan_cmd_t::SCAN_SET_RIGHT_BOADER;
	}
	/* 扫描速度 */
	else if (PARSE_CMD_SCNA_SPEED(chB4))
	{
		ctrlcmd.autoscan.cmdtype = scan_cmd_t::SCAN_SET_SPEED;
	}
}
