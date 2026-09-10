/*
 * @Author       : EasonLu
 * @Date         : 2025-03-11 08:40:25
 * @LastEditors  : EasonLu
 * @LastEditTime : 2025-05-09 18:42:47
 * @FilePath     : SipUtils.cpp
 * @Description  : SIP工具函数
 */
#include "SipUtils.h"
#include "dlog.h"
#include "SipType.h"
#include "iconv.h"
#include <ctime>
#include <experimental/filesystem>
#include <iomanip> // 为std::hex、std::setw和std::setfill提供支持
#include <random>
#include <regex>
#include <sstream> // 为std::stringstream提供支持
#include <string>
#include <unistd.h> // for readlink

#include <chrono>
#include <string>
#include <memory>
#include <ctime>
#include <iomanip>
#include <iostream>
#include <sstream>

namespace fs = std::experimental::filesystem;

using std::chrono::system_clock;

// CRC 查表
uint32_t g_crc_table[256];
// CRC 初始值
int g_crc_init = 0;

/* 调试标记位 */
#define SIP_UITLS_DEBUG 0

using namespace SIP;
std::string SIP::LocalTime(time_t time)
{
	std::tm local_tm = *std::localtime(&time);
	std::ostringstream oss;
	oss << std::put_time(&local_tm, "%Y-%m-%d %H:%M:%S");
#if SIP_UITLS_DEBUG
	dlog_debug("LocalTime: %s", oss.str().c_str());
#endif
	return oss.str();
}

std::string SIP::GenerateRandomString(int n)
{
	std::random_device rd;
	std::mt19937 gen(rd());
	std::uniform_int_distribution<> dis(1, 1000);

	const std::string chars("0123456789"
							"abcdefghijklmnopqrstuvwxyz"
							"ABCDEFGHIJKLMNOPQRSTUVWXYZ");
	int i = 0;
	int len = (int)chars.size();
	std::string text;
	while (i < n)
	{
		int idx = dis(gen) % len;
		text.push_back(chars[idx]);
		++i;
	}
#if SIP_UITLS_DEBUG
	dlog_debug("GenerateRandomString: %s", text.c_str());
#endif
	return text;
}

std::string SIP::GenerateRandomNumber(int n)
{
	std::random_device rd;
	std::mt19937 gen(rd());
	std::uniform_int_distribution<> dis(1, 1000);

	const std::string chars("0123456789");
	int i = 0;
	int len = (int)chars.size();
	std::string text;
	while (i < n)
	{
		int idx = dis(gen) % len;
		text.push_back(chars[idx]);
		++i;
	}
#if SIP_UITLS_DEBUG
	dlog_debug("GenerateRandomNumber: %s", text.c_str());
#endif
	return text;
}

std::string SIP::SSRC_Hex(std::string ssrc)
{
	long num = std::stol(ssrc);
	std::stringstream ss;
	ss << std::hex << std::setw(8) << std::setfill('0') << num;
#if SIP_UITLS_DEBUG
	dlog_debug("SSRC_Hex: %s", ss.str().c_str());
#endif
	return ss.str();
}

std::string SIP::ToUtf8String(const std::string &input)
{
	iconv_t cd = iconv_open("UTF-8", "GB18030");
	if (cd == (iconv_t)(-1))
	{
		std::cerr << "Failed to open iconv conversion descriptor" << std::endl;
		return "";
	}

	size_t inBytes = input.size() + 1; // 包括字符串结束符
	size_t outBytes = inBytes * 2;	   // 估计输出缓冲区大小
	char *inBuf = const_cast<char *>(input.c_str());
	char *outBuf = new char[outBytes];

	char *inPtr = inBuf;
	char *outPtr = outBuf;

	size_t result = iconv(cd, &inPtr, &inBytes, &outPtr, &outBytes);
	if (result == (size_t)(-1))
	{
		std::cerr << "Iconv conversion failed" << std::endl;
		delete[] outBuf;
		iconv_close(cd);
		return "";
	}

	std::string strOutput(outBuf, outPtr - outBuf);
	delete[] outBuf;
	iconv_close(cd);
#if SIP_UITLS_DEBUG
	dlog_debug("GB18030 ==========> UTF8\n%s", input.c_str());
#endif
	return strOutput;
}

std::string SIP::ToMbcsString(const std::string &input)
{
#if SIP_UITLS_DEBUG
	dlog_debug("UTF8 ==========> GB18030\n%s", input.c_str());
#endif
	iconv_t cd = iconv_open("GB18030", "UTF-8");
	if (cd == (iconv_t)(-1))
	{
		std::cerr << "Failed to open iconv conversion descriptor" << std::endl;
		return "";
	}

	size_t inBytes = input.size() + 1; // 包括字符串结束符
	size_t outBytes = inBytes * 2;	   // 估计输出缓冲区大小
	char *inBuf = const_cast<char *>(input.c_str());
	char *outBuf = new char[outBytes];

	char *inPtr = inBuf;
	char *outPtr = outBuf;

	size_t result = iconv(cd, &inPtr, &inBytes, &outPtr, &outBytes);
	if (result == (size_t)(-1))
	{
		std::cerr << "Iconv conversion failed" << std::endl;
		delete[] outBuf;
		iconv_close(cd);
		return "";
	}

	std::string strOutput(outBuf, outPtr - outBuf);
	delete[] outBuf;
	iconv_close(cd);
	return strOutput;
}

std::string SIP::GetCurrentModuleDirectory()
{
#if 0
	char buffer[1024] = {};
	readlink("/proc/self/exe", buffer, sizeof(buffer));
	return fs::path(buffer).parent_path().string();
#else
	return std::string("/root");
#endif
}

// 格式化后的时间字符串转化为std::time_t格式，所有的输出的时间戳都需要转化为GMT时间
int64_t SIP::ISO8601ToTimeT(const std::string &str)
{
#if SIP_UITLS_DEBUG
	dlog_debug("ISO8601ToTimeT: %s", str.c_str());
#endif
	std::regex pattern(R"(^\d{4}-\d{2}-\d{2}[Tt]\d{2}:\d{2}:\d{2}[Zz]?$)");
	if (std::regex_match(str, pattern))
	{
		std::tm t;
		auto year = str.substr(0, 4);
		t.tm_year = std::stoi(year) - 1900;
		auto month = str.substr(5, 2);
		t.tm_mon = std::stoi(month) - 1;
		auto day = str.substr(8, 2);
		t.tm_mday = std::stoi(day);
		auto hour = str.substr(11, 2);
		t.tm_hour = std::stoi(hour);
		auto minute = str.substr(14, 2);
		t.tm_min = std::stoi(minute);
		auto second = str.substr(17, 2);
		t.tm_sec = std::stoi(second);

		// GMT，不以z结尾为本地时间,本地时间转换为标准时间
		if (str.back() != 'z' && str.back() != 'Z')
		{
			auto tt = std::mktime(&t);
			auto tm = std::localtime(&tt);
			return std::mktime(tm);
		}
		else // 即是标准时间
		{
			return std::mktime(&t);
		}
	}
	else
	{
		return 0;
	}
}

std::string SIP::TimeTToISO8601(int64_t time)
{
	std::tm t = *std::localtime(reinterpret_cast<const time_t*>(&time));
	char buffer[32];
	strftime(buffer, sizeof(buffer), "%Y-%m-%dT%H:%M:%S", &t);
#if SIP_UITLS_DEBUG
	dlog_debug("TimeTToISO8601[%lld] ====> [%s]", time, buffer);
#endif
	return std::string(buffer);
}

std::string SIP::CurrentTimeTToISO8601()
{
	system_clock::time_point tp = system_clock::now();
 
	time_t raw_time = system_clock::to_time_t(tp);
 
	// tm*使用完后不用delete，因为tm*是由localtime创建的，并且每个线程中会有一个
	struct tm  *timeinfo = std::localtime(&raw_time);
 
	std::stringstream ss;
	ss << std::put_time(timeinfo, "%Y-%m-%dT%H:%M:%S.");
 
	// tm只能到秒，毫秒需要另外获取
	std::chrono::milliseconds ms = std::chrono::duration_cast<std::chrono::milliseconds>(tp.time_since_epoch());
 
	std::string milliseconds_str = std::to_string(ms.count() % 1000);
 
	if (milliseconds_str.length() < 3) {
		milliseconds_str = std::string(3 - milliseconds_str.length(), '0') + milliseconds_str;
	}
 
	return ss.str() + milliseconds_str;
}

int64_t SIP::UnixToTime(const std::string &strFromatTime)
{
	std::tm tm = {};
	if (strptime(strFromatTime.c_str(), "%Y-%m-%d %H:%M:%S", &tm) == nullptr)
	{
		dlog_warn("转换的时间格式strptime错误[%s]", strFromatTime.c_str());
		return -1;
	}
	time_t seconds = mktime(&tm);
	if (seconds == -1)
	{
		dlog_warn("转换的时间格式mktime错误[%s]", strFromatTime.c_str());
		return -2;
	}
#if SIP_UITLS_DEBUG
	dlog_debug("UnixToTime[%s] ====> [%lld]", strFromatTime.c_str(), seconds);
#endif
	return seconds;
}

SIP::SDP::Map_S SIP::ToRtpMap(SipVideoType_E enType)
{
	SIP::SDP::Map_S stMap;
	switch (enType)
	{
	case SIP_VIDEO_MPEG4:
		stMap.nPayloadType = RTPMAP_MPEG4;
		stMap.strCodecName = RTPMAP_MPEG4_CODEC;
		break;
	case SIP_VIDEO_H264:
		stMap.nPayloadType = RTPMAP_H264;
		stMap.strCodecName = RTPMAP_H264_CODEC;
		break;
	case SIP_VIDEO_H265:
		stMap.nPayloadType = RTPMAP_H265;
		stMap.strCodecName = RTPMAP_H265_CODEC;
		break;
	default:
		break;
	}
	/* 视频的时钟基准同一 */
	stMap.nClockRate = RTPMAP_VIDEO_CLOCK;
	/* 通道数为0，为音频数据预留 */
	stMap.nChannel = 0;
#if SIP_UITLS_DEBUG
	dlog_debug("ToRtpMap[%d] ====> [%d][%s]",
			   enType, stMap.nPayloadType, stMap.strCodecName.c_str());
#endif
	return stMap;
}

SipVideoType_E SIP::FromRtpMapByVideo(const SDP::Map_S &stMap)
{
#if SIP_UITLS_DEBUG
	dlog_debug("FromRtpMapByVideo[%d][%s]",
			   stMap.nPayloadType, stMap.strCodecName.c_str());
#endif
	/* 根据Codec名称来转换格式类型枚举 */
	if (0 == stMap.strCodecName.compare(RTPMAP_MPEG4_CODEC))
	{
		return SIP_VIDEO_MPEG4;
	}
	else if (0 == stMap.strCodecName.compare(RTPMAP_H264_CODEC))
	{
		return SIP_VIDEO_H264;
	}
	else if (0 == stMap.strCodecName.compare(RTPMAP_H265_CODEC))
	{
		return SIP_VIDEO_H265;
	}
	else
	{
		return SIP_VIDEO_NONE;
	}
}

SIP::SDP::Map_S SIP::ToRtpMap(SipAudioInfo_S stInfo)
{
	SIP::SDP::Map_S stMap;
	/* 音频的时钟基准 */
	stMap.nClockRate = stInfo.nSampleRate;
	/* 通道数 */
	stMap.nChannel = stInfo.nChannel;
	switch (stInfo.enType)
	{
	case SIP_AUDIO_G711A:
		stMap.nPayloadType = RTPMAP_PCMA;
		stMap.strCodecName = RTPMAP_PCMA_CODEC;
		break;
	case SIP_AUDIO_SVAC:
		stMap.nPayloadType = RTPMAP_SVACA;
		stMap.strCodecName = RTPMAP_SVACA_CODEC;
		break;
	case SIP_AUDIO_G723:
		stMap.nPayloadType = RTPMAP_G723;
		stMap.strCodecName = RTPMAP_G723_CODEC;
		break;
	case SIP_AUDIO_G729:
		stMap.nPayloadType = RTPMAP_G729;
		stMap.strCodecName = RTPMAP_G729_CODEC;
		break;
	case SIP_AUDIO_G722:
		stMap.nPayloadType = RTPMAP_G722;
		stMap.strCodecName = RTPMAP_G722_CODEC;
		break;
	case SIP_AUDIO_AAC:
		stMap.nPayloadType = RTPMAP_AAC;
		stMap.strCodecName = RTPMAP_AAC_CODEC;
		break;
	default:
		break;
	}
#if SIP_UITLS_DEBUG
	dlog_debug("ToRtpMap[%d] ====> [%d][%s]",
			   stInfo.enType, stMap.nPayloadType, stMap.strCodecName.c_str());
#endif
	return stMap;
}

SipAudioInfo_S SIP::FromRtpMapByAudio(const SDP::Map_S &stMap)
{
	SipAudioInfo_S stAudio;
	if (0 == stMap.strCodecName.compare(RTPMAP_PCMA_CODEC))
	{
		stAudio.enType = SIP_AUDIO_G711A;
	}
	else if (0 == stMap.strCodecName.compare(RTPMAP_SVACA_CODEC))
	{
		stAudio.enType = SIP_AUDIO_SVAC;
	}
	else if (0 == stMap.strCodecName.compare(RTPMAP_G723_CODEC))
	{
		stAudio.enType = SIP_AUDIO_G723;
	}
	else if (0 == stMap.strCodecName.compare(RTPMAP_G729_CODEC))
	{
		stAudio.enType = SIP_AUDIO_G729;
	}
	else if (0 == stMap.strCodecName.compare(RTPMAP_G722_CODEC))
	{
		stAudio.enType = SIP_AUDIO_G722;
	}
	else if (0 == stMap.strCodecName.compare(RTPMAP_AAC_CODEC))
	{
		stAudio.enType = SIP_AUDIO_AAC;
	}
	else
	{
		stAudio.enType = SIP_AUDIO_NONE;
	}
	stAudio.nSampleRate = stMap.nClockRate;
	stAudio.nChannel = stMap.nChannel;
#if SIP_UITLS_DEBUG
	dlog_debug("FromRtpMapByAudio[%s] ====> [%d][%d][%d]",
			   stMap.strCodecName.c_str(),
			   stAudio.enType, stAudio.nSampleRate, stAudio.nChannel);
#endif
	return stAudio;
}

SipVideoType_E SIP::FromPsmStreamIDByVideo(int nStreamID)
{
	auto enRet = SIP_VIDEO_NONE;
	using namespace SIP::PS;
	switch (nStreamID)
	{
	case MPEG_4:
		enRet = SIP_VIDEO_MPEG4;
		break;
	case H264:
		enRet = SIP_VIDEO_H264;
		break;
	case H265:
		enRet = SIP_VIDEO_H265;
		break;
	case SVAC_V:
		enRet = SIP_VIDEO_SVAC;
		break;
	default:
		break;
	}
	return enRet;
}

SipAudioType_E SIP::FromPsmStreamIDByAudio(int nStreamID)
{
	auto enRet = SIP_AUDIO_NONE;
	using namespace SIP::PS;
	switch (nStreamID)
	{
	case G711_A:
		enRet = SIP_AUDIO_G711A;
		break;
	case G711_U:
		break;
	case SVAC_A:
		enRet = SIP_AUDIO_SVAC;
		break;
	case G723_1:
		enRet = SIP_AUDIO_G723;
		break;
	case G729:
		enRet = SIP_AUDIO_G729;
		break;
	case G722_1:
		enRet = SIP_AUDIO_G722;
		break;
	case AAC:
		enRet = SIP_AUDIO_AAC;
		break;
	default:
		break;
	}
	return enRet;
}

int SIP::ToPsmStreamIDByVideo(SipVideoType_E enType)
{
	int enRet = 0;
	using namespace SIP::PS;
	switch (enType)
	{
	case SIP_VIDEO_MPEG4:
		enRet = MPEG_4;
		break;
	case SIP_VIDEO_H264:
		enRet = H264;
		break;
	case SIP_VIDEO_H265:
		enRet = H265;
		break;
	case SIP_VIDEO_SVAC:
		enRet = SVAC_V;
		break;
	default:
		break;
	}
	return enRet;
}

int SIP::ToPsmStreamIDByAudio(SipAudioType_E enType)
{
	int enRet = 0;
	using namespace SIP::PS;
	switch (enType)
	{
	case SIP_AUDIO_G711A:
		enRet = G711_A;
		break;
	case SIP_AUDIO_SVAC:
		enRet = SVAC_A;
		break;
	case SIP_AUDIO_G723:
		enRet = G723_1;
		break;
	case SIP_AUDIO_G729:
		enRet = G729;
		break;
	case SIP_AUDIO_G722:
		enRet = G722_1;
		break;
	case SIP_AUDIO_AAC:
		enRet = AAC;
		break;
	default:
		/* G711_U未实现 */
		break;
	}
	return enRet;
}

void init_crc32_table()
{
	uint32_t poly = 0x04C11DB7;
	for (int i = 0; i < 256; ++i)
	{
		uint32_t crc = i << 24;
		for (int j = 0; j < 8; ++j)
		{
			crc = (crc & 0x80000000) ? (crc << 1) ^ poly : (crc << 1);
		}
		g_crc_table[i] = crc;
	}
}

uint32_t SIP::CalcCRC_32(const char *data, size_t length)
{
	if (!g_crc_init)
	{
		init_crc32_table();
		g_crc_init = 1;
	}

	uint32_t crc = 0xFFFFFFFF;
	for (size_t i = 0; i < length; ++i)
	{
		uint8_t index = (crc >> 24) ^ data[i];
		crc = (crc << 8) ^ g_crc_table[index];
	}
	return crc;
}

bool SIP::isNumber(const std::string &str)
{
	return std::regex_match(str, std::regex("^[0-9]+$"));
}

std::vector<std::string> SIP::SplitString(const std::string &s, char c)
{
	std::vector<std::string> vecRet;
	std::istringstream iss(s);
	std::string item;
	while (std::getline(iss, item, c))
	{
		vecRet.push_back(item);
	}

	// 检查最后一个字符是否是分隔符
	if (!s.empty() && s.back() == c)
	{
		vecRet.push_back(""); /* 添加一个空字符串 */
	}
#if SIP_UITLS_DEBUG
	dlog_debug("需要分割的字符串[%s],分割后的个数[%d]",
			   s.c_str(), vecRet.size());
	for (size_t i = 0; i < vecRet.size(); i++)
	{
		dlog_debug("分割字符串[%d][%s]", i, vecRet[i].c_str());
	}
#endif
	return vecRet;
}

int SIP::ParseReadFileAction(const std::string &str, SipReadFileAction_S &stReadAction)
{
	/* NOTE 目前只处理实时流协议（MANSRTSP）命令集中的请求
	 *       格式可参考 GB/T 28181-2022 附录B
	 *       针对会话操作，和通道无关联
	 */
#if SIP_UITLS_DEBUG
	dlog_debug("解析回放操作指令\n%s", str.c_str());
#endif
	std::stringstream ss(str);
	std::string token;
	while (std::getline(ss, token))
	{
		/* 描述指令(格式为：[PLAY/PAUSE/TEARDOWN RTSP/1.0]) */
		if (token.find("RTSP") != std::string::npos)
		{
			if (token.find("PLAY") != std::string::npos)
			{
				/* 默认为恢复读取文件，开始操作在请求时自动开启 */
				stReadAction.enAction = SIP_READFILE_RESUME;
			}
			else if (token.find("PAUSE") != std::string::npos)
			{
				stReadAction.enAction = SIP_READFILE_PAUSE;
			}
			else if (token.find("TEARDOWN") != std::string::npos)
			{
				stReadAction.enAction = SIP_READFILE_STOP;
			}
			continue;
		}

		/* 指令序列号(格式为：[CSeq: 1]) */
		if (token.find("CSeq") != std::string::npos)
		{
			auto vecSeq = SplitString(token, ':');
			if (vecSeq.size() >= 2)
			{
				::SafeStr2Num(vecSeq[1], stReadAction.nCSeq);
			}
#if SIP_UITLS_DEBUG
			dlog_info("CSeq:%d", stReadAction.nCSeq);
#endif
			continue;
		}

		/* 倍速字段(格式为：[Scale: 1.0]) */
		if (token.find("Scale") != std::string::npos)
		{
			auto vecScale = SplitString(token, ':');
			if (vecScale.size() >= 2)
			{
				::SafeStr2Num(vecScale[1], stReadAction.dSpeed);
				stReadAction.enAction = SIP_READFILE_SPEED;
			}
#if SIP_UITLS_DEBUG
			dlog_info("Scale:%f", stReadAction.dSpeed);
#endif
			/* 倍速字段为负数时，则为倒放 */
			if (stReadAction.dSpeed < 0.00f)
			{
				stReadAction.enAction = SIP_READFILE_BACKWARDS;
			}
			continue;
		}

		/* 快进/快退字段(格式为：[Range: npt=100-200]或[Range: npt=now]或[Range: npt=100-] */
		if (token.find("Range") != std::string::npos)
		{
			auto vecRange = SplitString(token, ':');
			if (vecRange.size() >= 2)
			{
				auto vecNpt = SplitString(vecRange[1], '=');
				if (vecNpt.size() >= 2)
				{
					/* 继续拆分npt的字段 */
					auto vecTime = SplitString(vecNpt[1], '-');
					switch (vecTime.size())
					{
					case 0:
					{
						/* 只有一个now字段，则为继续当前暂定的地方进行播放 */
						stReadAction.enAction = SIP_READFILE_RESUME;
						break;
					}
					case 1:
					{
						/* 只有开始时间，则为从开始时间开始播放 */
						stReadAction.enAction = SIP_READFILE_SEEK;
						::SafeStr2Num(vecTime[0], stReadAction.nSeekTime);
						break;
					}
					case 2:
					{
						/* 具有区间段则大概率为倒放 */
						int nLeft = 0;
						int nRight = 0;
						::SafeStr2Num(vecTime[0], nLeft);
						::SafeStr2Num(vecTime[1], nRight);
						if (nLeft > nRight)
						{
							stReadAction.enAction = SIP_READFILE_BACKWARDS;
							stReadAction.nBackwardStartTime = nLeft;
							stReadAction.nBackwardEndTime = nRight;
						}
						else
						{
							stReadAction.enAction = SIP_READFILE_SEEK;
							stReadAction.nSeekTime = nLeft;
						}
						break;
					}
					default:
						break;
					}
				}
			}
		}
	}

	return 0;
}
