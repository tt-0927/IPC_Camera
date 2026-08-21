#include "aac_audio_source.h"
#include "InputFile.hh"
#include <GroupsockHelper.hh>

////////// aacAudioSource //////////
static unsigned const samplingFrequencyTable[16] = {
	96000, 88200, 64000, 48000,
	44100, 32000, 24000, 22050,
	16000, 12000, 11025, 8000,
	7350, 0, 0, 0};

int aacAudioSource::PutBits(unsigned char *buf, unsigned int bits, unsigned int numBits, int pos)
{
	for (int i = numBits - 1; i >= 0; i--)
	{
		buf[pos >> 3] |= ((bits >> i) & 1) << (7 - (pos & 7));
		pos++;
	}

	return pos;
}
aacAudioSource *
aacAudioSource::createNew(UsageEnvironment &env, Audio_Source_Info_t &aac_sourec_info)
{
	// 不再使用固定的头部，而是根据实际参数动态构建
	// unsigned char fixedHeader[4] = {0xFF, 0xF1, 0x4c, 0x40}; // 单声道版本
	// unsigned char fixedHeader[4] = {0xFF, 0xF1, 0x4C, 0x80};  // 立体声版本
	do
	{
		// 使用结构体中传入的采样率索引
		u_int8_t sampling_frequency_index = aac_sourec_info.samplingFreqIndex;

		// 验证采样率索引的有效性
		if (sampling_frequency_index > 12 || samplingFrequencyTable[sampling_frequency_index] == 0)
		{
			env.setResultMsg("无效的采样率索引（sampling_frequency_index）");
			break;
		}

		// 设置默认profile（AAC LC = 1，对应ADTS中的0）
		u_int8_t profile = 1; // AAC LC

		// 设置默认通道配置（单声道）
		// u_int8_t channel_configuration = 1;  // 单声道
		// u_int8_t channel_configuration = 2;  //	立体声
		u_int8_t channel_configuration = aac_sourec_info.channel;

		// 如果结构体中没有设置通道配置，则默认为单声道
		if (channel_configuration == 0)
		{
			channel_configuration = 1; // 默认单声道
		}

		// 构建ADTS头部进行验证
		unsigned char adts_header[7] = {0};
		adts_header[0] = 0xFF; // syncword高8位
		adts_header[1] = 0xF1; // syncword低4位 + ID + layer + protection_absent
		adts_header[2] = ((profile & 0x3) << 6) | ((sampling_frequency_index & 0xF) << 2) | ((channel_configuration >> 2) & 0x01);
		adts_header[3] = ((channel_configuration & 0x3) << 6);

		return new aacAudioSource(env, profile, sampling_frequency_index, channel_configuration, aac_sourec_info);
	} while (0);
	return NULL;
}

aacAudioSource ::aacAudioSource(UsageEnvironment &env, u_int8_t profile,
								u_int8_t samplingFrequencyIndex, u_int8_t channelConfiguration, Audio_Source_Info_t &aac_sourec_info)
	: FramedSource(env)
{
	fSamplingFrequency = samplingFrequencyTable[samplingFrequencyIndex];
	fNumChannels = channelConfiguration == 0 ? 2 : channelConfiguration;
	fuSecsPerFrame = (1024 /*samples-per-frame*/ * 1000000) / fSamplingFrequency /*samples-per-second*/;

	m_aacRate = (1 * 1000 * 1000) / fuSecsPerFrame;
	unsigned char audioSpecificConfig[2];
	u_int8_t const audioObjectType = profile;

	audioSpecificConfig[0] = (audioObjectType << 3) | (samplingFrequencyIndex >> 1);
	audioSpecificConfig[1] = ((samplingFrequencyIndex & 0x01) << 7) | (channelConfiguration << 3);
	sprintf(fConfigStr, "%02X%02x", audioSpecificConfig[0], audioSpecificConfig[1]);

	memcpy(&m_aacSouceInfo, &aac_sourec_info, sizeof(Audio_Source_Info_t));

	m_aacframeCount = 0;
	m_lost = 0;
	gettimeofday(&m_aacprvTime, NULL);
	if (m_aacSouceInfo.clientFun)
	{
		m_status.param = m_aacSouceInfo.audioindex;
		m_status.status = RTSPCLIENT_START;
		m_aacSouceInfo.clientFun(&m_status);
	}
	m_toDelay = 10000;
	continue_count_lost = 0;
	count_aac = 0;
}

aacAudioSource::~aacAudioSource()
{
	if (m_pToken)
	{
		envir().taskScheduler().unscheduleDelayedTask(m_pToken);
	}
	if (m_aacSouceInfo.clientFun)
	{
		m_status.param = m_aacSouceInfo.audioindex;
		m_status.status = RTSPCLIENT_STOP;
		m_aacSouceInfo.clientFun(&m_status);
	}
}

int aacAudioSource::set_audiostate_callback()
{
	if (m_status.param == NULL)
	{
		if (m_aacSouceInfo.clientFun)
		{
			printf("AAC set status is start\n");
			m_status.param = m_aacSouceInfo.audioindex;
			m_status.status = RTSPCLIENT_START;
			m_aacSouceInfo.clientFun(&m_status);
		}
	}
	return 0;
}

// Note: We should change the following to use asynchronous file reading, #####
// as we now do with ByteStreamFileSource. #####
void aacAudioSource::doGetNextFrame()
{

	m_pToken = envir().taskScheduler().scheduleDelayedTask(m_toDelay, getNextFrame, this);
}

void aacAudioSource::getNextFrame(void *ptr)
{
	aacAudioSource *pVideosource = (aacAudioSource *)ptr;
	if (pVideosource == NULL)
	{
		printf("AAC getNextFrame is NULL\n");
		return;
	}
	pVideosource->getNextFrame1();
}
unsigned int aacAudioSource::maxFrameSize() const
{
	return m_aacSouceInfo.outPacketBufferSize == 0
			   ? MAX_FRAME_SIZE
			   : m_aacSouceInfo.outPacketBufferSize;
}
static int dealaac_frame_timestamp(int d_value, int continue_count, int *continue_count_lost, int lostframe)
{
	// 已经连续多长时间没有补帧过来了,连续几次缺帧大于自己界限
	if ((d_value >= lostframe) || (d_value <= -lostframe))
	{
		if ((d_value >= lostframe))
		{
			(*continue_count_lost)++;
		}
		else
		{
			(*continue_count_lost)--;
		}
	}
	else
	{
		(*continue_count_lost) = 0;
	}
	// 需要补帧了
	if (abs((*continue_count_lost)) >= continue_count)
	{
		return 1;
	}

	return 0;
}
void aacAudioSource::getNextFrame1()
{
	if (m_aacSouceInfo.dataGetfun)
	{
		// set_audiostate_callback();
		m_frame.frameSize = 0;
		m_frame.data = fTo;
		m_frame.param = m_aacSouceInfo.audioindex;
		m_frame.type = AUDIO_TYPE;
		m_aacSouceInfo.dataGetfun(&m_frame);
		if (m_frame.frameSize > 4)
		{
			fFrameSize = m_frame.frameSize;
			// Set the 'presentation time':
			if (fPresentationTime.tv_sec == 0 && fPresentationTime.tv_usec == 0)
			{
				// This is the first frame, so use the current time:
				gettimeofday(&fPresentationTime, NULL);
			}
			else
			{
				// Increment by the play time of the previous frame:
				unsigned uSeconds = fPresentationTime.tv_usec + fuSecsPerFrame;
				fPresentationTime.tv_sec += uSeconds / 1000000;
				fPresentationTime.tv_usec = uSeconds % 1000000;
			}

			fDurationInMicroseconds = fuSecsPerFrame;

			gettimeofday(&m_aacCurTime, NULL);
			fPresentationTime = m_aacCurTime;
#if 0
			int lost_time = fuSecsPerFrame * 4;
			m_lost += (m_aacCurTime.tv_sec - m_aacprvTime.tv_sec) * 1000 * 1000 +
					(m_aacCurTime.tv_usec - m_aacprvTime.tv_usec) - fuSecsPerFrame;


			if(m_lost > lost_time || m_lost < -lost_time)
			{
				printf("\033[33m""m_lost:%d\n""\033[0m", m_lost);
				m_lost = 0;
				fPresentationTime = m_aacCurTime;
			}
			m_aacprvTime = m_aacCurTime;
// #else
			if((count_aac++) % 50 == 0)
			{
				int naacStreamTime = fuSecsPerFrame/1000;
				int lostframe = 2;
				int nFrameLost = 0;
				int nErrorTime = (fPresentationTime.tv_sec - m_aacCurTime.tv_sec) * 1000 +
						(fPresentationTime.tv_usec - m_aacCurTime.tv_usec)/1000;
				nFrameLost = nErrorTime / naacStreamTime;
				count_aac = 1;
				//printf("===aac  lostFrameCount:%d nErrorTime:%d\n", nFrameLost, nErrorTime);
				if(1 == dealaac_frame_timestamp(nFrameLost, 5,&continue_count_lost, lostframe))
				{
					// if(nErrorTime > naacStreamTime * lostframe || nErrorTime < -(naacStreamTime * lostframe))
					{
						// printf("\033[33m""=========nErrorTime:%d fuSecsPerFrame:%d %p===========\n""\033[0m", nErrorTime, fuSecsPerFrame, this);
						fPresentationTime = m_aacCurTime;
					}
				}

			}

#endif
		}

		else
		{
			m_toDelay = 10000;
			fFrameSize = 0;
			doGetNextFrame();
			return;
		}
	}

	m_toDelay = 0;
	/* 默认设置为计算出来的帧时长 */
    this->fDurationInMicroseconds = fuSecsPerFrame;

    /* 积压数据超过2帧时加速，快速发送 */
    if (m_frame.audiolistsize >= 2)
    {
        this->fDurationInMicroseconds = 0; // 告诉 Sink 立即处理下一帧，不要等待
    }

	afterGetting(this);
}
