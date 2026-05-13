
#include <unistd.h>
#include "pushurlBase.h"
#include "dlog.h"

using namespace std;
PushUrlBase::PushUrlBase(bool isHaveAudio)
    :CSdkPacketPush(NULL)
    ,isEnable_(false)
    ,isUpdataUrl_(false)
    ,audioBuffer_(NULL)
    ,audioBufferSize_(0)
	,m_isExit(false)
	,m_isHaveAudio(isHaveAudio)
	,m_liveDealThr(this)
{
    videoMessHave_ = false;
    audioMessHave_ = false;
    currentStatus_ = LIVE_NULL;
    reconnectTime_ = 0;

    /* 初始化队列 */
	OS_ringBufCreate(&(m_ringQueBuff),30);
	int j = 0;
	int ret = 0;
	for(j = 0; j < 30; j++)
	{
		ret = OS_ringBufInitBufInfo(&(m_ringQueBuff),\
				j,(void*)&(m_packet[j]));
		if(ret < 0)
		{
			dlog(LOG_ERROR,"init ring buff error!!");
		}
	}

    m_liveDealThr.start();
}

PushUrlBase::~PushUrlBase()
{
	m_isExit.store(true);
	m_liveDealThr.join();

    if(audioBuffer_)
    {
        free(audioBuffer_);
        audioBuffer_ = NULL;
    }
}

int PushUrlBase::pushurl_base_init(AVMess mess)
{
    //很多参数默认会解析，例如宽高等
    CSdkPacketPush = new sdk_packet_write();

    stIntputparm.audio_codec_id = AV_CODEC_ID_AAC;
    stIntputparm.video_codec_id = AV_CODEC_ID_H264;

    stIntputparm.audio_sample_rate = 48000;//mess.audioSampleRate;//48000;
    stIntputparm.audio_bit_rate = mess.audioBitRate;
    stIntputparm.video_frame_rate = (int)mess.frameRate;
    stIntputparm.video_bit_rate = (int)mess.videoBitRate;
    stIntputparm.height = mess.height;
    stIntputparm.width = mess.width;
    stIntputparm.write_type = CAPTURE_VIDEO_ALL;
    stIntputparm.write_pack_fun = NULL;
    strncpy(stIntputparm.url,m_chPushUrl.c_str(), sizeof(stIntputparm.url));
    if(CSdkPacketPush->sdk_init_mediafile(stIntputparm) < 0)
    {
        currentStatus_ = LIVE_RECONNECT;    //链接失败，需要重连
        //需要重连，把句柄删除
        delete CSdkPacketPush;
        CSdkPacketPush = NULL;
        reconnectTime_ = OS_getSysTimeInMsec();
        return -1;
    }else
    {
        //设置当前的直播状态
        currentStatus_ = LIVE_NORMAL;
    }

    return 0;
}

int PushUrlBase::ffmpegReconnect(PushUrlBase::AVMess mess)
{
    if(CSdkPacketPush)
    {
        delete CSdkPacketPush;
        CSdkPacketPush = NULL;
    }

    return pushurl_base_init(mess);
}


int PushUrlBase::sendFrame(AVMediaPacket_S* packet)
{
	if(packet == NULL)
	{
		dlog(LOG_ERROR,"this packet is NULL!!!");
		return -1;
	}

	/* 送队列 */
	return putFrameQue(packet);
}


int PushUrlBase::liveDealThr::run()
{
	int nRet = 0;
	AVMediaPacket_S packet;
	memset(&packet,0,sizeof(AVMediaPacket_S));

	while(!(m_obj->m_isExit.load()))
	{
		nRet = m_obj->popFrameQue(&packet,OS_TIMEOUT_FOREVER);
		if(nRet < 0)
		{
			dlog(LOG_WARN,"get packet is null!!!");
			usleep(40*1000);
			continue;
		}

		/* 推流 */
		m_obj->connectServerSendPacket(&packet);

		/* 解引用 */
		avMedia_packet_unref(&packet);
	}

	std::cout << "exit==============\n\n" << std::endl;

	return 0;
}

int PushUrlBase::connectServerSendPacket(AVMediaPacket_S* CFrameData)
{
	/* 未使能推流 */
	if(!isEnable_.load())
	{
		if(currentStatus_ != LIVE_NULL)
		{
			currentStatus_ = (LIVE_NULL);
		}
		return 0;
	}

	if(isUpdataUrl_.load())
	{
		if(m_chPushUrl != updateUrl_)
		{
			updateMutex_.lock();
			m_chPushUrl = updateUrl_;
			updateMutex_.unlock();
			stop(); //停止推流
		}
	}

	//连接服务器
	if(CSdkPacketPush == NULL)
	{
		if(CFrameData->type == PACKET_TYPE_VIDEO)
		{
			if(!videoMessHave_){
				videoMessHave_ = true;
				liveAvMess_.width = CFrameData->nWidth;
				liveAvMess_.height = CFrameData->nHeight;
				liveAvMess_.frameRate = CFrameData->frameRate;
				liveAvMess_.videoBitRate = CFrameData->bitRate;

				//音频参数固定
				audioMessHave_ = true;
				liveAvMess_.audioSampleRate = 48000;
				liveAvMess_.audioBitRate = 128000;
			}

		}else if(CFrameData->type == PACKET_TYPE_AUDIO)
		{
			if(!audioMessHave_){
				audioMessHave_ = true;
				liveAvMess_.audioSampleRate = CFrameData->audioSampleRate;
				liveAvMess_.audioBitRate = (int)CFrameData->audioBitRate;
			}
		}

		//初始化句柄
		if(videoMessHave_)
		{
			if(m_isHaveAudio && audioMessHave_)
			{
				/* 音视频都有 */
				pushurl_base_init(liveAvMess_);
			}else
			{
				/* 只有视频 */
				pushurl_base_init(liveAvMess_);
			}
		}else{
			return -1;
		}
	}

	//判断码流信息是否改变，改变需要重新推流
	if(CFrameData->type == PACKET_TYPE_VIDEO)
	{
		if((liveAvMess_.width != CFrameData->nWidth) || \
				(liveAvMess_.height != CFrameData->nHeight) || \
				(liveAvMess_.frameRate != CFrameData->frameRate))
		{
			videoMessHave_ = true;
			liveAvMess_.width = CFrameData->nWidth;
			liveAvMess_.height = CFrameData->nHeight;
			liveAvMess_.frameRate = CFrameData->frameRate;
			liveAvMess_.videoBitRate = CFrameData->bitRate;
			ffmpegReconnect(liveAvMess_);
		}
	}

	//判断是否需要重来
	if(currentStatus_ == LIVE_RECONNECT)
	{
		//重连2s重连一次
		if((OS_getSysTimeInMsec() - reconnectTime_) >= 2000)
		{
			ffmpegReconnect(liveAvMess_);
			reconnectTime_ = OS_getSysTimeInMsec();
			dlog(LOG_WARN,"name[%s] reconnect!!! current status[%d]",\
				 m_chPushUrl.c_str(),currentStatus_);
		}
	}

	if(currentStatus_ == LIVE_NORMAL)
	{
		/* 状态正常再推送数据 */
		if(sendFrameFfmpeg(CFrameData) < 0)
		{
			/* 发送失败，则需要重连 */
			currentStatus_ = (LIVE_RECONNECT);
			reconnectTime_ = OS_getSysTimeInMsec();
		}else
		{
			/* 发送成功 */
			if(currentStatus_ != LIVE_NORMAL)
			{
				currentStatus_ = (LIVE_NORMAL);
			}
		}
	}

	return 0;
}


int PushUrlBase::sendFrameFfmpeg(AVMediaPacket_S* CFrameData)
{
    if((CSdkPacketPush == NULL) || (CFrameData == NULL))
    {
        return -1;
    }

    mediaType_t enType = MEDIA_TYPE_NULL;
    unsigned char * pchData = NULL;
    int nDataSize = 0;
    Capture_CallBack_Data_t stFramedata;
    if(CFrameData->format == PACKET_FORMAT_H264 || \
    		CFrameData->format == PACKET_FORMAT_H265)
    {
        enType = MEDIA_TYPE_VIDEO;
        stFramedata.data_type = CAPTURE_VIDEO_TYPE;
    }else if(CFrameData->format == PACKET_FORMAT_AAC)
    {
        enType = MEDIA_TYPE_AUDIO;
        stFramedata.data_type = CAPTURE_AUDIO_TYPE;
    }

    //音频需要加上adts头信息
    if(enType == MEDIA_TYPE_AUDIO)
    {
        /* 音频 */
        if((audioBuffer_ == NULL) || \
        		(audioBufferSize_ < (CFrameData->nSize + 7)))
        {
            if(audioBuffer_)
            {
                free(audioBuffer_);
                audioBuffer_ = NULL;
            }

            //7字节头+aac裸流
            audioBufferSize_ = CFrameData->nSize + 7;
            audioBuffer_ = (unsigned char*)malloc(audioBufferSize_);
        }

        audioHeaderAdts(audioBuffer_,(CFrameData->nSize + 7));
        memcpy(audioBuffer_ + 7,CFrameData->pData,CFrameData->nSize);

        stFramedata.data = audioBuffer_;
        stFramedata.size = CFrameData->nSize + 7;
        stFramedata.keyframe = CFrameData->nKeyFrame;

    }else
    {
        /* 视频 */
        pchData = (unsigned char *)CFrameData->pData;
        nDataSize = CFrameData->nSize;
        stFramedata.data = pchData;
        stFramedata.size = nDataSize;
        stFramedata.keyframe = CFrameData->nKeyFrame;
    }

    return CSdkPacketPush->sdk_write_frame(&stFramedata);
}


int PushUrlBase::stop()
{
    if(CSdkPacketPush)
    {
        delete CSdkPacketPush;
        CSdkPacketPush = NULL;
    }
    currentStatus_ = LIVE_NULL;
    return 0;
}

int PushUrlBase::audioHeaderAdts(unsigned char *buffer, int bufferSize)
{
    int profile = 2;  //AAC LC，MediaCodecInfo.CodecProfileLevel.AACObjectLC;
    int freqIdx = 3;  //32K, 见后面注释avpriv_mpeg4audio_sample_rates中32000对应的数组下标，来自ffmpeg源码
    int chanCfg = 2;  //见后面注释channel_configuration，Stero双声道立体声

    /*int avpriv_mpeg4audio_sample_rates[] = {
        96000, 88200, 64000, 48000, 44100, 32000,
                24000, 22050, 16000, 12000, 11025, 8000, 7350
    };
    channel_configuration: 表示声道数chanCfg
    0: Defined in AOT Specifc Config
    1: 1 channel: front-center
    2: 2 channels: front-left, front-right
    3: 3 channels: front-center, front-left, front-right
    4: 4 channels: front-center, front-left, front-right, back-center
    5: 5 channels: front-center, front-left, front-right, back-left, back-right
    6: 6 channels: front-center, front-left, front-right, back-left, back-right, LFE-channel
    7: 8 channels: front-center, front-left, front-right, side-left, side-right, back-left, back-right, LFE-channel
    8-15: Reserved
    */

    // fill in ADTS data
    buffer[0] = (unsigned char)0xFF;
    buffer[1] = (unsigned char)0xF9;
    buffer[2] = (unsigned char)(((profile-1)<<6) + (freqIdx<<2) +(chanCfg>>2));
    buffer[3] = (unsigned char)(((chanCfg&3)<<6) + (bufferSize>>11));
    buffer[4] = (unsigned char)((bufferSize&0x7FF) >> 3);
    buffer[5] = (unsigned char)(((bufferSize&7)<<5) + 0x1F);
    buffer[6] = (unsigned char)0xFC;

    return 0;
}


PushUrlBase::LiveStatus PushUrlBase::getCurrentStatus()
{
    //获取当前的直播状态
    return currentStatus_;
}

int PushUrlBase::liveUrl(string url)
{
    if(isUpdataUrl_.load())
    {
        updateMutex_.lock();
        updateUrl_ = url;
        updateMutex_.unlock();
    }else
    {
        updateUrl_ = url;
    }
    isUpdataUrl_.store(true);
    return 0;
}

int PushUrlBase::enableLive(bool enable)
{
    if(!enable)
    {
        currentStatus_ = LIVE_NULL;
    }
    else
    {
        currentStatus_ = LIVE_NORMAL;
    }

    isEnable_.store(enable);
    return 0;
}

int PushUrlBase::putFrameQue(AVMediaPacket_S* frame)
{
	if(frame == NULL)
	{
		dlog(LOG_ERROR,"this frame is NULL!!");
		return -1;
	}

	AVMediaPacket_S* chnFrame = NULL;
	Int64 bufId = 0;

	if(OS_ringBufGetEmpty(&(m_ringQueBuff),&bufId,OS_TIMEOUT_NONE) >= 0)
	{
		chnFrame = (AVMediaPacket_S*)OS_ringBufGetBufInfo(\
				&(m_ringQueBuff),bufId);
		if(chnFrame)
		{
			/* 引用数据 */
			avMedia_packet_ref(chnFrame,frame);
			if(OS_ringBufPutFull(&(m_ringQueBuff),bufId,OS_TIMEOUT_NONE) < 0)
			{
				OS_ringBufPutEmpty(&(m_ringQueBuff),bufId,OS_TIMEOUT_NONE);
				return -1;
			}
		}
	}else
	{
		dlog(LOG_ERROR,"put frame to vdev queue is full! object:[%s]",\
				m_chPushUrl.c_str());
		return -1;
	}
	return 0;
}


int PushUrlBase::popFrameQue(AVMediaPacket_S* frame,int timeout)
{
	Int64 frameAddr = 0;
	AVMediaPacket_S* getframe = NULL;
    Int64 bufId = OS_RING_BUF_ID_INVALID;
    int nRet = 0;

	if(OS_ringBufGetFull(&(m_ringQueBuff),&bufId,timeout) >= 0)
	{
		getframe = (AVMediaPacket_S*)OS_ringBufGetBufInfo(\
				&(m_ringQueBuff),bufId);
		if(getframe)
		{
			/* 引用数据到新的对象 */
			avMedia_packet_ref(frame,getframe);
			/* 原来的数据解引用 */
			avMedia_packet_unref(getframe);
		}
		if(OS_ringBufPutEmpty(&(m_ringQueBuff),bufId,OS_TIMEOUT_NONE) < 0)
		{
			dlog(LOG_ERROR,"OS_ringBufPutEmpty error!");
			nRet = -1;
		}
	}else
	{
		nRet = -1;
	}

	return nRet;
}



