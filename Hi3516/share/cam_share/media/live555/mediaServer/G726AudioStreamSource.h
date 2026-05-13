/**
 * @FilePath     : G726AudioStreamSource.h
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2025-06-09 17:16:23
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2025-06-10 14:09:17
 * @Description  : G.726音频数据源类定义
 * 继承自FramedSource，专门处理G.726音频数据的读取、时间戳生成和帧调度。作为Live555框架中的数据源，为RTSP服务器提供G.726音频帧数据。
 * 
 * 核心功能：
 * - G.726音频数据的异步读取和缓存
 * - 音频帧的精确时间戳计算和同步
 * - 支持实时音频流的连续播放
 * - 处理G.726特有的非字节对齐编码格式
 * 
 * 技术特点：
 * - 采样率：8000 Hz（固定）
 * - 声道数：1（单声道）
 * - 位深度：2-5 bits/sample（可配置）
 * - 帧大小：动态计算，优化RTP传输效率
 * - 时间精度：微秒级时间戳控制
 */

#pragma once

#ifndef _FRAMED_SOURCE_HH
#include "FramedSource.hh"
#endif
#include "custom_define.h"

class G726AudioStreamSource : public FramedSource
{
public:
	static G726AudioStreamSource *createNew(UsageEnvironment &env, Audio_Source_Info_t &stuG726SourecInfo);
	unsigned char bitsPerSample() const { return fBitsPerSample; }
	unsigned char numChannels() const { return fNumChannels; }
	unsigned samplingFrequency() const { return fSamplingFrequency; }

	static void getNextFrame(void *ptr);
	void getNextFrame1();
	// returns the 'AudioSpecificConfig' for this stream (in ASCII form)
	virtual unsigned int maxFrameSize() const;

protected:
	G726AudioStreamSource(UsageEnvironment &envm, Audio_Source_Info_t &stuG726SourecInfo);
	// called only by createNew()

	virtual ~G726AudioStreamSource();

private:
	// redefined virtual functions:
	virtual void doGetNextFrame();

private:
	unsigned char fNumChannels;
	unsigned fSamplingFrequency;
	unsigned char fBitsPerSample;
	unsigned fPreferredFrameSize;
	Boolean fLimitNumBytesToStream;
	unsigned fNumBytesToStream;
	unsigned fLastPlayTime;
	double fPlayTimePerSample; // useconds

	int m_toDelay;
	Audio_Source_Info_t m_g726SouceInfo;
	Rtsp_ClientStream_State_t m_status;
	Fream_Info_t m_frame;
	void *m_pToken;

	struct timeval m_g726CurTime;
	struct timeval m_g726prvTime;
};
