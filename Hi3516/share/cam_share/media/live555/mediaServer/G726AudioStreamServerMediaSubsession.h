/**
 * @FilePath     : G726AudioStreamServerMediaSubsession.h
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2025-06-09 17:16:23
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2025-06-10 14:07:38
 * @Description  : G.726音频流RTSP媒体子会话管理类
 *
 * 功能概述：
 * - 管理G.726音频的RTSP媒体子会话生命周期
 * - 创建和配置G726AudioStreamSource音频数据源
 * - 创建适合G.726格式的RTP发送器
 * - 处理音频参数协商（采样率8kHz，位深度2-5bits，单声道）
 * - 生成G.726音频相关的SDP描述信息
 *
 * G.726格式支持：
 * - G.726-16 (2 bits/sample, 16 kbps)
 * - G.726-24 (3 bits/sample, 24 kbps)
 * - G.726-32 (4 bits/sample, 32 kbps) - 默认
 * - G.726-40 (5 bits/sample, 40 kbps)
 */
#ifndef _G726U_AUDIO_STREAM_SERVER_MEDIA_SUBSESSION_HH
#define _G726U_AUDIO_STREAM_SERVER_MEDIA_SUBSESSION_HH

#ifndef _ON_DEMAND_SERVER_MEDIA_SUBSESSION_HH
#include "OnDemandServerMediaSubsession.hh"
#endif
#include "liveMedia.hh"
#include "BasicUsageEnvironment.hh"
#include "G726AudioStreamSource.h"

/**
 * @brief   : G.726音频流RTSP媒体子会话管理类
 * @details 该类继承自OnDemandServerMediaSubsession，专门处理G.726音频格式的RTSP流媒体服务。
 *          负责创建音频数据源、RTP封装器，并管理G.726音频流的会话参数。
 */
class G726AudioStreamServerMediaSubsession : public OnDemandServerMediaSubsession
{
public:
	static G726AudioStreamServerMediaSubsession *
	createNew(UsageEnvironment &env, Boolean reuseFirstSource, Audio_Source_Info_t &stuG726);

protected:
	G726AudioStreamServerMediaSubsession(UsageEnvironment &env, Boolean reuseFirstSource, Audio_Source_Info_t &stuG726);
	// called only by createNew();
	virtual ~G726AudioStreamServerMediaSubsession();

protected: // redefined virtual functions
	virtual FramedSource *createNewStreamSource(unsigned clientSessionId,
												unsigned &estBitrate);
	virtual RTPSink *createNewRTPSink(Groupsock *rtpGroupsock,
									  unsigned char rtpPayloadTypeIfDynamic,
									  FramedSource *inputSource);

protected:
	// The following parameters of the input stream are set after
	// "createNewStreamSource" is called:
	unsigned char fBitsPerSample;
	unsigned fSamplingFrequency;
	unsigned fNumChannels;
	Audio_Source_Info_t m_stuG726SourceInfo;
};

#endif