#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "Media.h"
#include "dlog.h"
#define CONFIG_LIBFDK_AAC_ENCODER 1

#ifdef WIN32
#include <fcntl.h>
#define fseek(f,p,w) _fseeki64((f), (p), (w))
#else
#define fseek(f,p,w) fseeko((f), (p), (w))
#endif


#ifdef WIN32
#pragma comment(lib, "avutil.lib")
#pragma comment(lib, "avcodec.lib")
#pragma comment(lib, "avformat.lib")
#pragma comment(lib, "avfilter.lib")
#pragma comment(lib, "swscale.lib")
#pragma comment(lib, "swresample.lib")
#pragma comment(lib,"winmm.lib")
#endif

int bFFmpegInit = 0;


void make_dsi( unsigned int sampling_frequency_index, unsigned int channel_configuration, unsigned   char* dsi )
 {
      unsigned int object_type = 2; // AAC LC by default
      dsi[0] = (object_type<<3) | (sampling_frequency_index>>1);
     dsi[1] = ((sampling_frequency_index&1)<<7) | (channel_configuration<<3);
}

int get_sr_index(unsigned int sampling_frequency)
 {
  switch (sampling_frequency) {
   case 96000: return 0;
   case 88200: return 1;
   case 64000: return 2;
   case 48000: return 3;
   case 44100: return 4;
   case 32000: return 5;
    case 24000: return 6;
    case 22050: return 7;
    case 16000: return 8;
    case 12000: return 9;
    case 11025: return 10;
    case 8000:  return 11;
    case 7350:  return 12;
    default:    return 0;
   }
}


AVStream *add_audio_stream(AVFormatContext *oc, enum AVCodecID codec_id,int sample_rate,int bit_rate,int channel,enum AVSampleFormat sample_fmt)
{
	AVCodecContext *c;
	AVStream *st;

	st = avformat_new_stream(oc, NULL);
	if (!st) {
		dlog(LOG_ERROR, "Could not alloc stream\n");
		return NULL;
	}

	c = st->codec;
	c->codec_id = codec_id;
	c->codec_type = AVMEDIA_TYPE_AUDIO;
	c->time_base.den = sample_rate;
	c->time_base.num = 1;
	c->sample_fmt = sample_fmt;
	c->bit_rate = bit_rate ;
	c->bit_rate_tolerance = bit_rate * 12/10;
	c->sample_rate = sample_rate;
	c->channels = channel;
	c->frame_size = 1024;
	//c->extradata = av_malloc(2+AV_INPUT_BUFFER_PADDING_SIZE);
	//make_dsi(3, 2, c->extradata);
	if (oc->oformat->flags & AVFMT_GLOBALHEADER)
		c->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
	

	return st;
}

int h264_encode_para_set(AVCodecContext * c,int gop,int profile,int preset)
{
	char * pProfile = "main";
	char * pPreset;

	if (!c)
		return -1;

	if (preset > 0)
	{
		switch(preset)
		{
		case PRESET_ULTRAFAST:
			pPreset = "ultrafast";
			break;
		case PRESET_SUPERFAST:
			pPreset = "superfast";
			break;
		case PRESET_VERYFAST:
			pPreset = "veryfast";
			break;
		case PRESET_FASTER:
			pPreset = "faster";
			break;
		case PRESET_FAST:
			pPreset = "fast";
			break;
		case PRESET_MEDIUM:
			pPreset = "medium";
			break;
		case PRESET_SLOW:
			pPreset = "slow";
			break;
		case PRESET_SLOWER:
			pPreset = "slower";
			break;
		case PRESET_PLACEBO:
			pPreset = "placebo";
			break;
		default:
			pPreset = "medium";
			break;
		}
		av_opt_set(c->priv_data,"preset",pPreset,0);//chenw
	}
	c->gop_size = gop >0 ? gop :75;
	if (profile > 0)
	{
		switch(profile)
		{
		case PROFILE_BASELINE:
			pProfile = "baseline";
			break;
		case PROFILE_MAIN:
			pProfile = "main";
			break;
		case PROFILE_HIGH:
			pProfile = "high";
			break;
		default:
			pProfile = "high";
		}
	}
	av_opt_set(c->priv_data,"profile",pProfile,0);//chenw
	return 0;
}
AVStream *add_video_stream(AVFormatContext *oc, enum AVCodecID codec_id,int width,int height,int bitrate,int frame_rate)
{
	AVCodecContext *c;
	AVCodec * pAVCodec = NULL;
	AVStream *st;
	int iRet;


	pAVCodec = avcodec_find_encoder(codec_id);
	if (pAVCodec == NULL)
	{
		printf("Can't find encoder %d!\n",codec_id);
		//return NULL; if encode h.264 should open this
	}
	st = avformat_new_stream(oc,pAVCodec);
	//st = avformat_new_stream(oc,NULL);
	if (!st) {
		fprintf(stderr, "Could not alloc stream\n");
		return NULL;
	}

	c = st->codec;
	c->codec_id = codec_id;
	c->codec_type = AVMEDIA_TYPE_VIDEO;
	c->width = width;
	c->height = height;
	//printf("the frame.....=%d\n", frame_rate);
	c->time_base.den = frame_rate;//STREAM_FRAME_RATE;
	c->time_base.num = 1;//STREAM_FRAME_RATE/frame_rate;
	c->pix_fmt = AV_PIX_FMT_YUV420P;
	c->bit_rate = bitrate;
	//c->rc_lookahead = 40;
	c->thread_count = 0;
	c->max_b_frames = 3;
// 	c->qmin = 10;
// 	c->qmax = 50;//防止编码第一帧图像质量太差
	if (oc->oformat->flags & AVFMT_GLOBALHEADER)
		c->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;

	return st;
}

int write_one_frame(AVFormatContext * oc,AVPacket * pkt)
{
	if(av_write_frame(oc, pkt) < 0)
	{
		dlog(LOG_ERROR,"stream index %d write_one_frame fail![%s],pts=%lld--dts=%lld \n",pkt->stream_index,strerror(errno),pkt->pts,pkt->dts);
		return -1;
	
	}
	//av_packet_unref(pkt);
	return 0;
}



int create_media_file(AVFormatContext ** oc,char * OutputFile)    
{
	avformat_alloc_output_context2(oc, NULL, NULL, OutputFile);
	if (!*oc) 
	{
		dlog(LOG_ERROR,"Could not deduce output format from file extension: using MPEG.\n");
		avformat_alloc_output_context2(oc, NULL, "mpeg", OutputFile);
	}
	if (!*oc) 
	{
		return -1;
	}
	return 0;
}

#ifndef WIN32
int lockmgr(void **mtx, enum AVLockOp op)
{
	switch(op) {
 case AV_LOCK_CREATE:
	 *mtx = (pthread_mutex_t *)malloc(sizeof(pthread_mutex_t));
	 if(!*mtx)
		 return 1;
	 return !!pthread_mutex_init((pthread_mutex_t*)*mtx, NULL);
 case AV_LOCK_OBTAIN:
	 return !!pthread_mutex_lock((pthread_mutex_t*)*mtx);
 case AV_LOCK_RELEASE:
	 return !!pthread_mutex_unlock((pthread_mutex_t*)*mtx);
 case AV_LOCK_DESTROY:
	 pthread_mutex_destroy((pthread_mutex_t*)*mtx);
	 free(*mtx);
	 return 0;
	}
	return 1;
}
#endif

void MediaSysInit()
{
	if (!bFFmpegInit)
	{
		av_register_all();
		avcodec_register_all();
		avfilter_register_all();
		avformat_network_init();
#ifdef _DEBUG
		av_log_set_level(AV_LOG_DEBUG);
#else
		av_log_set_level(AV_LOG_ERROR);
#endif

#ifndef WIN32
		if (av_lockmgr_register(lockmgr)) 
		{
			dlog(LOG_ERROR,"Could not initialize lock manager!\n");
			return;
		}
#endif
		bFFmpegInit = 1;
	}
}

void MediaWriterInit(MuxWriter * pMuxWriter)
{
	int i;

	pMuxWriter->bAsfFile = 0;
	pMuxWriter->bMp4File = 0;
	pMuxWriter->eAudioCodecID = AV_CODEC_ID_NONE;
	pMuxWriter->bsfc = NULL;
	for (i = 0;i < MAX_STREAM_NUM;i++)
	{
		pMuxWriter->abGetSyncFrame[i] = 0;
		pMuxWriter->apVideoCodecCtx[i] = NULL;
	}
	pMuxWriter->pAudioCodecCtx = NULL;
	pMuxWriter->eVideoCodecID = AV_CODEC_ID_NONE;

	pMuxWriter->oc = NULL;
	pMuxWriter->bHaveVideo = 1;
	pMuxWriter->bStartWriteFrame = 0;
#ifdef WIN32
	InitializeCriticalSection(&pMuxWriter->cs);
#else
	pthread_mutex_init(&pMuxWriter->mutex,NULL);
#endif
}
#ifdef WIN32
int ANSIToUTF8(char *pszCode, char *UTF8code)
{
	WCHAR Unicode[MAX_PATH]={0}; 
	char utf8[MAX_PATH]={0};

	// read char Length
	int nUnicodeSize = MultiByteToWideChar(CP_ACP, 0, pszCode, strlen(pszCode), Unicode, sizeof(Unicode)); 

	// read UTF-8 Length
	int nUTF8codeSize = WideCharToMultiByte(CP_UTF8, 0, Unicode, nUnicodeSize, UTF8code, sizeof(Unicode), NULL, NULL); 

	// convert to UTF-8 
	MultiByteToWideChar(CP_UTF8, 0, utf8, nUTF8codeSize, Unicode, sizeof(Unicode)); 
	return nUTF8codeSize;
}
#endif

int MediaFileCreate(MuxWriter * pMuxWriter,char * file,int bHaveVideo)
{
	int iRet;
	char * pStr;
//	int ch = '.';
	char acUtf8FileName[MAX_PATH];

	memset(acUtf8FileName,0,MAX_PATH);
#ifdef WIN32
	ANSIToUTF8(file,acUtf8FileName);
#else
	strcpy(acUtf8FileName,file);
#endif
	pMuxWriter->oc = NULL;
	iRet = create_media_file(&pMuxWriter->oc,acUtf8FileName);
	if (iRet < 0)
		return iRet;

	pStr = strrchr(file,'.');
	if (pStr == NULL)
	{
		dlog(LOG_ERROR,"file is not specify suffix!\n");
		return -1;
	}

	if (strstr(pStr,".mkv") || strstr(pStr,".flv"))
	{
		pMuxWriter->bMp4File = 1;
	}
	else if (strstr(pStr,".asf") || strstr(pStr,".wmv") || strstr(pStr,".wma"))
	{
		pMuxWriter->bAsfFile = 1;
	}
	pMuxWriter->bHaveVideo = bHaveVideo;
	strncpy(pMuxWriter->acFileName,file,MAX_PATH);
	av_dump_format(pMuxWriter->oc, 0, file, 1);
	if(!(pMuxWriter->oc->oformat->flags & AVFMT_NOFILE))
	{
		if(avio_open(&pMuxWriter->oc->pb, acUtf8FileName, AVIO_FLAG_WRITE) < 0)
		{
			dlog(LOG_ERROR,"Could not open '%s'\n", pMuxWriter->acFileName);
			return -1;	
		}
	}
	return 0;
}

int VideoStreamAdd(MuxWriter * pMuxWriter,enum AVCodecID codec_id,int width,int height,int bitrate,int frame_rate)
{
	AVStream * st;
	if (!pMuxWriter->bHaveVideo)
	{
		dlog(LOG_ERROR,"warning output is should no video!\n");
	}

	
	st =  add_video_stream(pMuxWriter->oc,codec_id,width,height,bitrate,frame_rate);
	if (st == NULL)
	{
		return -1;
	}
	else
	{
		pMuxWriter->eVideoCodecID = codec_id;
		pMuxWriter->apVideoCodecCtx[st->index] = st->codec;
		return st->index;
	}
}

int AudioStreamAdd(MuxWriter * pMuxWriter,enum AVCodecID codec_id,int sample_rate,int bit_rate,int channel,enum AVSampleFormat sample_fmt)
{
	AVStream * st;
//	int iTimeScale;

	st = add_audio_stream(pMuxWriter->oc,codec_id,sample_rate,bit_rate,channel,sample_fmt);
	if (st == NULL)
	{
		return -1;
	}
	else
	{
		pMuxWriter->eAudioCodecID = codec_id;
		if (codec_id == AV_CODEC_ID_AAC)
		{
			pMuxWriter->bsfc = av_bitstream_filter_init("aac_adtstoasc");
		}
		pMuxWriter->pAudioCodecCtx = st->codec;
		return st->index;
	}
}
int h264_split(uint8_t *buf, int buf_size)
{
	int i;
	uint32_t state = -1;
	int has_sps= 0;

	for(i=0; i<=buf_size; i++){
		if((state&0xFFFFFF1F) == 0x107)
			has_sps=1;
		if((state&0xFFFFFF00) == 0x100 && (state&0xFFFFFF1F) != 0x107 && (state&0xFFFFFF1F) != 0x108 && (state&0xFFFFFF1F) != 0x109){
			if(has_sps){
				while(i>4 && buf[i-5]==0) i--;
				return i-4;
			}
		}
		if (i<buf_size)
			state= (state<<8) | buf[i];
	}
	return 0;
}
int RemoveSpsPps(int8_t * pBuf,int iLen)
{
	int i;
	int iSps;
	int iGetSpsOnceTime = 0 ;
//	int iOffset = 0;

	if (!pBuf || iLen <=0 )
	{
		return 0;
	}
	for (i = 0;i < iLen;i++)
	{
		if (pBuf[i] == 0 && pBuf[i + 1] == 0 && pBuf[i + 2] == 1)
		{
			iSps = pBuf[i + 3] & 0x1f;
			switch(iSps)
			{
			case 7:
				if (!iGetSpsOnceTime)
					iGetSpsOnceTime = 1;
				else
				{
					if (pBuf[i - 1] == 0) 
						return i - 1;
					else
						return i;
				}
				break;
			case 5:
			case 1:
				return 0;
			}
		}
	}
	return 0;
}







int MediaWriteFrame(MuxWriter * pMuxWriter,MediaPacket * pMediaPkt)
{
	//return 0;
	#if 1
	AVPacket new_pkt = {0};
	
	int packdatafree = 0;
	int a;
	int iRet;
	struct AVRational time_base= {1,1000};
	struct AVRational asftime_base= {1,90000};

	if (pMediaPkt->iLen > 3 * 1920 * 1080)
	{
		dlog(LOG_ERROR,"MediaWriteFrame frame len %d is error!!!\n",pMediaPkt->iLen);
		return -1;
	}
	if (pMuxWriter->oc->nb_streams <= pMediaPkt->iStreamIdx)
	{
		return -1;
	}

	if (pMuxWriter->bHaveVideo && !pMuxWriter->bStartWriteFrame && (pMediaPkt->bAudio || !pMediaPkt->bKeyFrame))
	{
		dlog(LOG_ERROR,"Discard packet audio %d stream index %d! \n",pMediaPkt->bAudio,pMediaPkt->iStreamIdx);
		dlog(LOG_ERROR,"Discard packet video %d stream index %d! \n",pMuxWriter->bHaveVideo,pMuxWriter->bStartWriteFrame);
		return 0;
	}
	if (!pMediaPkt->bKeyFrame && !pMuxWriter->bHaveVideo)
	{
		if(!pMediaPkt->bAudio)
		{
			dlog(LOG_ERROR,"skip video!\n");
			iRet = 0;		
			return iRet;
		}
	}

	av_init_packet(&new_pkt);
	new_pkt.data = pMediaPkt->pData;
	new_pkt.size = pMediaPkt->iLen;
	new_pkt.stream_index = pMediaPkt->iStreamIdx;
	new_pkt.flags = pMediaPkt->bKeyFrame?AV_PKT_FLAG_KEY:0;
	if (pMediaPkt->bAudio)
	{	
		if (pMuxWriter->oc->streams[pMediaPkt->iStreamIdx]->time_base.den == 0)
		{
			dlog(LOG_ERROR,"before pts=%lld pMediaPkt->sample_rate=%d \n", pMediaPkt->pts, pMediaPkt->sample_rate);
			pMuxWriter->oc->streams[pMediaPkt->iStreamIdx]->time_base.den = pMediaPkt->sample_rate;
			pMuxWriter->oc->streams[pMediaPkt->iStreamIdx]->time_base.num = 1;
		}
		
		new_pkt.pts = av_rescale_q(pMediaPkt->pts,time_base,pMuxWriter->oc->streams[pMediaPkt->iStreamIdx]->time_base);
		//printf("the pts=%ld, pMuxWriter->oc->streams[pMediaPkt->iStreamIdx]->time_base=%ld\n", new_pkt.pts, pMuxWriter->oc->streams[pMediaPkt->iStreamIdx]->time_base.den);
		new_pkt.dts = pMediaPkt->prev_adts;
		
		if (new_pkt.pts <= pMediaPkt->prev_adts)
		{
			new_pkt.pts = pMediaPkt->prev_adts + 10;
		}
		pMediaPkt->prev_adts = new_pkt.pts;
		
	}
	else
	{	
		if (pMuxWriter->oc->streams[pMediaPkt->iStreamIdx]->time_base.den == 0)
		{

			dlog(LOG_ERROR, "before pts=%lld pMediaPkt->frame_rate=%d\n", pMediaPkt->pts, pMediaPkt->frame_rate);
			pMuxWriter->oc->streams[pMediaPkt->iStreamIdx]->time_base.den = pMediaPkt->frame_rate;
			pMuxWriter->oc->streams[pMediaPkt->iStreamIdx]->time_base.num = 1;
		}
		//printf("vvvv before pts=%ld\n", pMediaPkt->pts);
		new_pkt.pts = av_rescale_q(pMediaPkt->pts,time_base,pMuxWriter->oc->streams[pMediaPkt->iStreamIdx]->time_base);//+200;
		//printf("vvvv the pts=%ld, pMuxWriter->oc->streams[pMediaPkt->iStreamIdx]->time_base=%ld\n", new_pkt.pts, pMuxWriter->oc->streams[pMediaPkt->iStreamIdx]->time_base.den);
		new_pkt.dts = pMediaPkt->prev_vdts;
		
		if (new_pkt.pts <= pMediaPkt->prev_vdts)
		{
			new_pkt.pts = pMediaPkt->prev_vdts + 10;
		}
		
		pMediaPkt->prev_vdts = new_pkt.pts;
	}
	
#ifdef WIN32
	EnterCriticalSection(&pMuxWriter->cs);
#else
	pthread_mutex_lock(&pMuxWriter->mutex);
#endif

	#if 1
	if (!pMediaPkt->bAudio && !pMuxWriter->abGetSyncFrame[pMediaPkt->iStreamIdx])//第一次存储AVPacket之前需要在前面加上H.264的SPS和PPS
	{
		//printf("hehheheheh\n");
		//if (pMuxWriter->bMp4File)
		{
			int iOffset;

			iOffset = RemoveSpsPps((int8_t*)pMediaPkt->pData,pMediaPkt->iLen);
			if (iOffset > 0)
			{
				new_pkt.data = pMediaPkt->pData + iOffset;
				new_pkt.size = pMediaPkt->iLen - iOffset;
			}
			
			if (1)
			{
				AVCodecContext * pCodecCtx;

				iOffset = h264_split(new_pkt.data,new_pkt.size);
				pCodecCtx = pMuxWriter->oc->streams[pMediaPkt->iStreamIdx]->codec;
				if (iOffset > 0)
				{
					if (pCodecCtx->extradata_size > 0 && pCodecCtx->extradata)
					{
						av_free(pCodecCtx->extradata);
						pCodecCtx->extradata_size = 0;
					}
					
					pCodecCtx->extradata = (uint8_t *)av_malloc(iOffset);
					if (!pCodecCtx->extradata)
					{
						dlog(LOG_ERROR,"av_malloc fail!");
						iRet = -1;
						goto WRITE_FRAME_EXIT;
					}
					
					memset(pCodecCtx->extradata,0,sizeof(iOffset));
					memcpy( (char*)pCodecCtx->extradata,new_pkt.data,iOffset);
					pCodecCtx->extradata_size = iOffset;
				}

				
				if (pCodecCtx->extradata_size > 0)
				{
					pMuxWriter->abGetSyncFrame[pMediaPkt->iStreamIdx] = 1;
					dlog(LOG_ERROR,"video stream %d get sync frame!\n",pMediaPkt->iStreamIdx);
				}
				else
				{
					iRet = 1;//sps pps incorrect
					goto WRITE_FRAME_EXIT;
				}
			}
		}
	}
	
	#endif
	if (!pMuxWriter->bStartWriteFrame)
	{
		
		int ret = -1;
		pMuxWriter->bStartWriteFrame = 1;
		//av_opt_set_int(pMuxWriter->oc->priv_data,"frag_size",10000000,0);
		//av_opt_set_int(pMuxWriter->oc->priv_data,"moov_size",2000000,0);
		//av_opt_set(pMuxWriter->oc->priv_data,"movflags","faststart",0);
		//av_opt_set_int(pMuxWriter->oc->priv_data,"video_track_timescale",STREAM_FRAME_RATE,0);
		if(pMuxWriter->bMp4File == 1)
		{
			AVCodecContext * pCodecCtx;
			pCodecCtx = pMuxWriter->oc->streams[1]->codec;
			pCodecCtx->extradata = av_malloc(2+AV_INPUT_BUFFER_PADDING_SIZE);
			make_dsi(3, 2, pCodecCtx->extradata);
			pCodecCtx->extradata_size = 2;
		}

		ret = avformat_write_header(pMuxWriter->oc,NULL);//写文件头
		if (ret != 0)
		{
			printf("it is err avformat_write_header!!!!!!!\n");
			
		}
		else
		{
			printf("it is ok avformat_write_header!!!!!!\n");
		}
	}	
	if (pMediaPkt->bAudio && pMuxWriter->eAudioCodecID == AV_CODEC_ID_AAC && pMuxWriter->bMp4File)
	{
		#if 1
		if (pMediaPkt->pData[0] == 0xff && (pMediaPkt->pData[1] & 0xf0) == 0xf0)//用于检测aac
		{
			a = av_bitstream_filter_filter(pMuxWriter->bsfc,pMuxWriter->pAudioCodecCtx,NULL,&new_pkt.data,&new_pkt.size,pMediaPkt->pData,pMediaPkt->iLen,pMediaPkt->bKeyFrame);
			if(a>0)
			{
				packdatafree = 1;
				//new_pkt.destruct = av_destruct_packet;
			} 
			else if(a<0)
			{
				av_log(NULL, AV_LOG_ERROR, "%s failed for stream %d",
					pMuxWriter->bsfc->filter->name,pMediaPkt->iStreamIdx);
				dlog(LOG_ERROR,"a= %d error\n", a);
				iRet =  -1;
				goto WRITE_FRAME_EXIT;
			}
		//	printf("new_pkt.data[0]=%0x pMuxWriter->pAudioCodecCtx->=%d pMuxWriter->pAudioCodecCtx->extradata=%0x\n", new_pkt.data[0],pMuxWriter->pAudioCodecCtx->extradata_size, pMuxWriter->pAudioCodecCtx->extradata[0]);
		}
		
		#endif
		//new_pkt.data = new_pkt.data +7;
		//new_pkt.size = new_pkt.size -7;
	}
	
	if (pMuxWriter->bAsfFile)
	{
		printf("/***************************dts=%lld/\n", pMediaPkt->dts);
		//new_pkt.dts = av_rescale_q(pMediaPkt->dts,time_base, asftime_base);
	}
	//new_pkt.duration = 0;
	new_pkt.pts+=10;
	iRet = write_one_frame(pMuxWriter->oc,&new_pkt);
	pMediaPkt->dts=new_pkt.dts;


	if (new_pkt.data != NULL  && (packdatafree == 1))
	{
		av_free(new_pkt.data);
		new_pkt.data = NULL;
	}

WRITE_FRAME_EXIT:
#ifdef WIN32
	LeaveCriticalSection(&pMuxWriter->cs);
#else
	pthread_mutex_unlock(&pMuxWriter->mutex);
#endif
	return iRet;
#endif

}


int h265_split(uint8_t *buf, int buf_size)
{
    int nPos = 0;
    int nType;
    
    for(; nPos < buf_size; ++nPos)
    {
        while(0 != buf[nPos] && nPos < buf_size)
        {
            ++nPos;
        }

        if(0 == buf[nPos+1] && 0 == buf[nPos+2] && 1 == buf[nPos+3])
        {
            nPos += 3;
            nType = (buf[++nPos] & 0x7E) >> 1;
            if(16 <= nType && 21 >= nType)
            {
                return nPos-4;
            }
        }
    }

    return 0;
}

int MediaWrite265Frame(MuxWriter * pMuxWriter,MediaPacket * pMediaPkt)
{
    //return 0;
    #if 1
    AVPacket new_pkt = {0};
    
    int packdatafree = 0;
    int a;
    int iRet;
    struct AVRational time_base= {1,1000};
    struct AVRational asftime_base= {1,90000};

    if (pMediaPkt->iLen > 3 * 1920 * 1080)
    {
        dlog(LOG_ERROR,"MediaWriteFrame frame len %d is error!!!\n",pMediaPkt->iLen);
        return -1;
    }
    if (pMuxWriter->oc->nb_streams <= pMediaPkt->iStreamIdx)
    {
        return -1;
    }

    if (pMuxWriter->bHaveVideo && !pMuxWriter->bStartWriteFrame && (pMediaPkt->bAudio || !pMediaPkt->bKeyFrame))
    {
        dlog(LOG_ERROR,"Discard packet audio %d stream index %d! \n",pMediaPkt->bAudio,pMediaPkt->iStreamIdx);
        dlog(LOG_ERROR,"Discard packet video %d stream index %d! \n",pMuxWriter->bHaveVideo,pMuxWriter->bStartWriteFrame);
        return 0;
    }
    if (!pMediaPkt->bKeyFrame && !pMuxWriter->bHaveVideo)
    {
        if(!pMediaPkt->bAudio)
        {
            dlog(LOG_ERROR,"skip video!\n");
            iRet = 0;
            return iRet;
        }
    }

    av_init_packet(&new_pkt);
    new_pkt.data = pMediaPkt->pData;
    new_pkt.size = pMediaPkt->iLen;
    new_pkt.stream_index = pMediaPkt->iStreamIdx;
    new_pkt.flags = pMediaPkt->bKeyFrame?AV_PKT_FLAG_KEY:0;
    if (pMediaPkt->bAudio)
    {    
        if (pMuxWriter->oc->streams[pMediaPkt->iStreamIdx]->time_base.den == 0)
        {
            dlog(LOG_ERROR,"before pts=%lld pMediaPkt->sample_rate=%d \n", pMediaPkt->pts, pMediaPkt->sample_rate);
            pMuxWriter->oc->streams[pMediaPkt->iStreamIdx]->time_base.den = pMediaPkt->sample_rate;
            pMuxWriter->oc->streams[pMediaPkt->iStreamIdx]->time_base.num = 1;
        }
        
        new_pkt.pts = av_rescale_q(pMediaPkt->pts,time_base,pMuxWriter->oc->streams[pMediaPkt->iStreamIdx]->time_base);
        //printf("the pts=%ld, pMuxWriter->oc->streams[pMediaPkt->iStreamIdx]->time_base=%ld\n", new_pkt.pts, pMuxWriter->oc->streams[pMediaPkt->iStreamIdx]->time_base.den);
        new_pkt.dts = pMediaPkt->prev_adts;
        
        if (new_pkt.pts <= pMediaPkt->prev_adts)
        {
            new_pkt.pts = pMediaPkt->prev_adts + 10;
        }
        pMediaPkt->prev_adts = new_pkt.pts;
        
    }
    else
    {    
        if (pMuxWriter->oc->streams[pMediaPkt->iStreamIdx]->time_base.den == 0)
        {

            dlog(LOG_ERROR, "before pts=%lld pMediaPkt->frame_rate=%d\n", pMediaPkt->pts, pMediaPkt->frame_rate);
            pMuxWriter->oc->streams[pMediaPkt->iStreamIdx]->time_base.den = pMediaPkt->frame_rate;
            pMuxWriter->oc->streams[pMediaPkt->iStreamIdx]->time_base.num = 1;
        }
        //printf("vvvv before pts=%ld\n", pMediaPkt->pts);
        new_pkt.pts = av_rescale_q(pMediaPkt->pts,time_base,pMuxWriter->oc->streams[pMediaPkt->iStreamIdx]->time_base);//+200;
        //printf("vvvv the pts=%ld, pMuxWriter->oc->streams[pMediaPkt->iStreamIdx]->time_base=%ld\n", new_pkt.pts, pMuxWriter->oc->streams[pMediaPkt->iStreamIdx]->time_base.den);
        new_pkt.dts = pMediaPkt->prev_vdts;
        
        if (new_pkt.pts <= pMediaPkt->prev_vdts)
        {
            new_pkt.pts = pMediaPkt->prev_vdts + 10;
        }
        
        pMediaPkt->prev_vdts = new_pkt.pts;
    }
    
#ifdef WIN32
    EnterCriticalSection(&pMuxWriter->cs);
#else
    pthread_mutex_lock(&pMuxWriter->mutex);
#endif

    #if 1
    if (!pMediaPkt->bAudio && !pMuxWriter->abGetSyncFrame[pMediaPkt->iStreamIdx])//第一次存储AVPacket之前需要在前面加上H.264的SPS和PPS
    {
        //printf("hehheheheh\n");
        //if (pMuxWriter->bMp4File)
        {
            int iOffset;
            
            if (1)
            {
                AVCodecContext * pCodecCtx;
                iOffset = h265_split(new_pkt.data,new_pkt.size);
                pCodecCtx = pMuxWriter->oc->streams[pMediaPkt->iStreamIdx]->codec;
                if (iOffset > 0)
                {
                    if (pCodecCtx->extradata_size > 0 && pCodecCtx->extradata)
                    {
                        av_free(pCodecCtx->extradata);
                        pCodecCtx->extradata_size = 0;
                    }
                    
                    pCodecCtx->extradata = (uint8_t *)av_malloc(iOffset);
                    if (!pCodecCtx->extradata)
                    {
                        dlog(LOG_ERROR,"av_malloc fail!");
                        iRet = -1;
                        goto WRITE_FRAME_EXIT;
                    }
                    
                    memset(pCodecCtx->extradata,0,sizeof(iOffset));
                    memcpy( (char*)pCodecCtx->extradata,new_pkt.data,iOffset);
                    pCodecCtx->extradata_size = iOffset;
                }
                
                if (pCodecCtx->extradata_size > 0)
                {
                    pMuxWriter->abGetSyncFrame[pMediaPkt->iStreamIdx] = 1;
                    dlog(LOG_ERROR,"video stream %d get sync frame!\n",pMediaPkt->iStreamIdx);
                }
                else
                {
                    iRet = 1;//sps pps incorrect
                    goto WRITE_FRAME_EXIT;
                }
            }
        }
    }
    
    #endif
    if (!pMuxWriter->bStartWriteFrame)
    {
        
        int ret = -1;
        pMuxWriter->bStartWriteFrame = 1;
        //av_opt_set_int(pMuxWriter->oc->priv_data,"frag_size",10000000,0);
        //av_opt_set_int(pMuxWriter->oc->priv_data,"moov_size",2000000,0);
        //av_opt_set(pMuxWriter->oc->priv_data,"movflags","faststart",0);
        //av_opt_set_int(pMuxWriter->oc->priv_data,"video_track_timescale",STREAM_FRAME_RATE,0);
        if(pMuxWriter->bMp4File == 1)
        {
            AVCodecContext * pCodecCtx;
            pCodecCtx = pMuxWriter->oc->streams[1]->codec;
            pCodecCtx->extradata = av_malloc(2+AV_INPUT_BUFFER_PADDING_SIZE);
            make_dsi(3, 2, pCodecCtx->extradata);
            pCodecCtx->extradata_size = 2;
        }

        ret = avformat_write_header(pMuxWriter->oc,NULL);//写文件头
        if (ret != 0)
        {
            printf("it is err avformat_write_header!!!!!!!\n");
            
        }
        else
        {
            printf("it is ok avformat_write_header!!!!!!\n");
        }
    }    
    if (pMediaPkt->bAudio && pMuxWriter->eAudioCodecID == AV_CODEC_ID_AAC && pMuxWriter->bMp4File)
    {
        #if 1
        if (pMediaPkt->pData[0] == 0xff && (pMediaPkt->pData[1] & 0xf0) == 0xf0)//用于检测aac
        {
            a = av_bitstream_filter_filter(pMuxWriter->bsfc,pMuxWriter->pAudioCodecCtx,NULL,&new_pkt.data,&new_pkt.size,pMediaPkt->pData,pMediaPkt->iLen,pMediaPkt->bKeyFrame);
            if(a>0)
            {
                packdatafree = 1;
                //new_pkt.destruct = av_destruct_packet;
            } 
            else if(a<0)
            {
                av_log(NULL, AV_LOG_ERROR, "%s failed for stream %d",
                    pMuxWriter->bsfc->filter->name,pMediaPkt->iStreamIdx);
                dlog(LOG_ERROR,"a= %d error\n", a);
                iRet =  -1;
                goto WRITE_FRAME_EXIT;
            }
        //    printf("new_pkt.data[0]=%0x pMuxWriter->pAudioCodecCtx->=%d pMuxWriter->pAudioCodecCtx->extradata=%0x\n", new_pkt.data[0],pMuxWriter->pAudioCodecCtx->extradata_size, pMuxWriter->pAudioCodecCtx->extradata[0]);
        }
        
        #endif
        //new_pkt.data = new_pkt.data +7;
        //new_pkt.size = new_pkt.size -7;
    }
    
    if (pMuxWriter->bAsfFile)
    {
        printf("/***************************dts=%lld/\n", pMediaPkt->dts);
        //new_pkt.dts = av_rescale_q(pMediaPkt->dts,time_base, asftime_base);
    }
    //new_pkt.duration = 0;
    new_pkt.pts+=10;
    iRet = write_one_frame(pMuxWriter->oc,&new_pkt);
    pMediaPkt->dts=new_pkt.dts;


    if (new_pkt.data != NULL  && (packdatafree == 1))
    {
        av_free(new_pkt.data);
        new_pkt.data = NULL;
    }

WRITE_FRAME_EXIT:
#ifdef WIN32
    LeaveCriticalSection(&pMuxWriter->cs);
#else
    pthread_mutex_unlock(&pMuxWriter->mutex);
#endif
    return iRet;
#endif

}

int MediaWriteTrailer(MuxWriter * pMuxWriter)
{
	int iRet = 0;
	char * pPtr;
	char acTmp[MAX_PATH] = {0};

	if (pMuxWriter->bStartWriteFrame)
	{
		iRet = av_write_trailer(pMuxWriter->oc);
	}
	else
	{
		dlog(LOG_ERROR,"MediaWriteTrailer is err!!\n");
	}
	
	strcpy(acTmp,pMuxWriter->acFileName);
	pPtr = strrchr(acTmp,'.');
	if (pPtr)
	{
		strcpy(pPtr,".tmp");
		pPtr = acTmp;
	}
	else
	{
		pPtr = "mp4.tmp";
	}
	remove(pPtr);
	return iRet;
}

void MediaWriterClose(MuxWriter * pMuxWriter)
{
//	int i;
	if (pMuxWriter->bsfc)
	{
		av_bitstream_filter_close(pMuxWriter->bsfc);
	}
 
	if (pMuxWriter->oc)
	{
		if (!(pMuxWriter->oc->oformat->flags & AVFMT_NOFILE) && pMuxWriter->oc->pb)
			avio_close(pMuxWriter->oc->pb);
		avformat_free_context(pMuxWriter->oc);
	}
	
#ifdef WIN32
	DeleteCriticalSection(&pMuxWriter->cs);
#else
	pthread_mutex_destroy(&pMuxWriter->mutex);
#endif
}



int adts_sample_rates[] = {96000,88200,64000,48000,44100,32000,24000,22050,16000,12000,11025,8000,7350,0,0,0};

void MediaReaderInit(DemuxReader * pDemuxReader)
{
	int i;
	pDemuxReader->ic = NULL;
	pDemuxReader->iDuration = 0;

	pDemuxReader->bStreamEnd = 0;
	pDemuxReader->iVideoStreamIdx = -1;
	pDemuxReader->iAudioStreamIdx = -1;
	pDemuxReader->bMp4H264BitStream = 0;
	pDemuxReader->bNeedAACAdtsHeader = 1;//all readed aac frame with adts header
	pDemuxReader->iHE_AAC = 0;
	for (i = 0;i < MAX_STREAM_NUM;i++)
	{
		pDemuxReader->aBsfc[i] = NULL;
		pDemuxReader->aiWidth[i] = 0;
		pDemuxReader->aiHeight[i] = 0;
		pDemuxReader->aiFrameNum[i] = 0;
		pDemuxReader->aiDuration[i] = 0;
		pDemuxReader->apVideoCodecCtx[i] = NULL;
	}
	pDemuxReader->iVideoNum = 0;
#ifdef WIN32
	InitializeCriticalSection(&pDemuxReader->cs);
#else
	pthread_mutex_init(&pDemuxReader->mutex,NULL);
#endif
}
int MediaOpenFile(DemuxReader * pDemuxReader,char * file)
{
	int ret;
	int i;
	AVCodecContext * pDecCodecCtx;
	AVFormatContext * ic;
	double dDuration = 0.0f;

	if((ret = avformat_open_input(&pDemuxReader->ic, file, NULL, NULL)) < 0)
	{
		dlog(LOG_ERROR,"Cannot open input file %s \n", file);
		return ret;
	}

	if((ret = avformat_find_stream_info(pDemuxReader->ic,NULL)) < 0)
	{
		dlog(LOG_ERROR,"Cannot find stream information\n");
		return ret;
	}

	ic = pDemuxReader->ic;
	for(i = 0; i < (int)ic->nb_streams; i++)
	{
		pDecCodecCtx = ic->streams[i]->codec;
		dDuration = ic->streams[i]->duration * 1000 * ic->streams[i]->time_base.num / ic->streams[i]->time_base.den;
		pDemuxReader->iDuration = pDemuxReader->iDuration > dDuration ?pDemuxReader->iDuration:dDuration;
		pDemuxReader->aiFrameNum[i] = ic->streams[i]->nb_index_entries;
		pDemuxReader->aiDuration[i] = ic->streams[i]->duration * 1000 * ic->streams[i]->time_base.num / ic->streams[i]->time_base.den;
		if(pDecCodecCtx->codec_type == AVMEDIA_TYPE_VIDEO)
		{
			if (pDemuxReader->iVideoStreamIdx == -1)
			{
				pDemuxReader->iVideoStreamIdx = i;
			}
			pDemuxReader->aiWidth[i] = pDecCodecCtx->width;
			pDemuxReader->aiHeight[i] = pDecCodecCtx->height;
			if (pDecCodecCtx->codec_id == AV_CODEC_ID_H264 && pDecCodecCtx->extradata_size > 0 && pDecCodecCtx->extradata)
			{
				if (pDecCodecCtx->extradata[0] == 1)
				{
					pDemuxReader->aBsfc[i] = av_bitstream_filter_init("h264_mp4toannexb");
				}
			}
			pDemuxReader->apVideoCodecCtx[i] = pDecCodecCtx;
			pDemuxReader->iVideoNum++;
		}

		if(pDecCodecCtx->codec_type == AVMEDIA_TYPE_AUDIO ||(pDecCodecCtx->codec_id == AV_CODEC_ID_NONE && pDecCodecCtx->codec_type == AVMEDIA_TYPE_DATA) )
		{
			if (pDemuxReader->iAudioStreamIdx == -1)
				pDemuxReader->iAudioStreamIdx = i;
			else
			{
				dlog(LOG_ERROR,"warning multi audio track skip this audio stream!!!");
				continue;
			}
			pDemuxReader->iSampleRate = pDecCodecCtx->sample_rate > 0 ?pDecCodecCtx->sample_rate:DEFAULT_AUDIO_SAMPLE_RATE;
			pDemuxReader->iChannel = pDecCodecCtx->channels == 0 ?2:pDecCodecCtx->channels;
			pDemuxReader->eAudioFmt = pDecCodecCtx->sample_fmt < 0 ? AV_SAMPLE_FMT_S16:pDecCodecCtx->sample_fmt ;
			pDemuxReader->iBitRate = pDecCodecCtx->bit_rate;
			pDemuxReader->eAudioCodecID = pDecCodecCtx->codec_id == AV_CODEC_ID_NONE?AV_CODEC_ID_AAC:pDecCodecCtx->codec_id;
			pDemuxReader->pAudioCodecCtx = pDecCodecCtx;
			if(pDemuxReader->eAudioCodecID == AV_CODEC_ID_AAC && pDecCodecCtx->extradata && pDecCodecCtx->extradata_size > 0)
			{

				if (pDecCodecCtx->extradata_size >= 2)
				{
					int samplerate;
					int samplerate_idx = ((pDecCodecCtx->extradata[0] << 1)  & 0xF) | (pDecCodecCtx->extradata[1] >> 7);
					int channel = (pDecCodecCtx->extradata[1] >> 3) & 0xF;

					samplerate = adts_sample_rates[samplerate_idx];
					if (samplerate * 2 == pDecCodecCtx->sample_rate)
					{
						pDemuxReader->iHE_AAC |= 0x1;
					}
					if (channel * 2 == pDecCodecCtx->channels)
					{
						pDemuxReader->iHE_AAC |= 0x2;
					}


				}
			}
			continue;
		}
	}
	//printf("%s 's duration is %ld ms\n",file,pDemuxReader->iDuration);
	return 0;
}



static int FindAdtsSRIndex(int sr)
{
	int i;

	for (i = 0; i < 16; i++)
	{
		if (sr == adts_sample_rates[i])
			return i;
	}
	return 16 - 1;
}

unsigned char *MakeAdtsHeader(int *dataSize,int samplerate, int channels,int iFrameLen )
{
	unsigned char *data;
	int profile = 1;
	int sr_index = FindAdtsSRIndex(samplerate);
	int skip = 7;
	int framesize = skip + iFrameLen;


	*dataSize = 7;

	data = (unsigned char *)malloc(*dataSize * sizeof(unsigned char));
	memset(data, 0, *dataSize * sizeof(unsigned char));

	data[0] += 0xFF; /* 8b: syncword */

	data[1] += 0xF0; /* 4b: syncword */
	/* 1b: mpeg id = 0 */
	/* 2b: layer = 0 */
	data[1] += 1; /* 1b: protection absent */

	data[2] += ((profile << 6) & 0xC0); /* 2b: profile */
	data[2] += ((sr_index << 2) & 0x3C); /* 4b: sampling_frequency_index */
	/* 1b: private = 0 */
	data[2] += ((channels >> 2) & 0x1); /* 1b: channel_configuration */

	data[3] += ((channels << 6) & 0xC0); /* 2b: channel_configuration */
	/* 1b: original */
	/* 1b: home */
	/* 1b: copyright_id */
	/* 1b: copyright_id_start */
	data[3] += ((framesize >> 11) & 0x3); /* 2b: aac_frame_length */

	data[4] += ((framesize >> 3) & 0xFF); /* 8b: aac_frame_length */

	data[5] += ((framesize << 5) & 0xE0); /* 3b: aac_frame_length */
	data[5] += ((0x7FF >> 6) & 0x1F); /* 5b: adts_buffer_fullness */

	data[6] += ((0x7FF << 2) & 0x3F); /* 6b: adts_buffer_fullness */
	/* 2b: num_raw_data_blocks */

	return data;
}

int media_writeJpeg(AVFrame* pFrame, int width, int height, char *out_file)  
{  
    // 分配AVFormatContext对象
    AVFormatContext* pFormatCtx = avformat_alloc_context();  
      
    // 设置输出文件格式
    pFormatCtx->oformat = av_guess_format("mjpeg", NULL, NULL);  
    // 创建并初始化一个和该url相关的AVIOContext
    if( avio_open(&pFormatCtx->pb, out_file, AVIO_FLAG_READ_WRITE) < 0) 
	{  
        printf("Couldn't open output file.");  
        return -1;  
    }  
      
    // 构建一个新stream
    AVStream* pAVStream = avformat_new_stream(pFormatCtx, 0);  
    if (pAVStream == NULL) 
	{  
        return -1;  
    }  
      
    // 设置该stream的信息
    AVCodecContext* pCodecCtx = pAVStream->codec;  
      
    pCodecCtx->codec_id = pFormatCtx->oformat->video_codec;  
    pCodecCtx->codec_type = AVMEDIA_TYPE_VIDEO;  
    pCodecCtx->pix_fmt = AV_PIX_FMT_YUVJ420P;  
    pCodecCtx->width = width;  
    pCodecCtx->height = height;  
    pCodecCtx->time_base.num = 1;  
    pCodecCtx->time_base.den = 25;  
      
    // Begin Output some information  
    av_dump_format(pFormatCtx, 0, out_file, 1);  
    // End Output some information  
      
    // 查找解码器
    AVCodec* pCodec = avcodec_find_encoder(pCodecCtx->codec_id);  
    if (!pCodec) 
	{  
        printf("Codec not found.");  
        return -1;  
    }  
    // 设置pCodecCtx的解码器为pCodec
    if( avcodec_open2(pCodecCtx, pCodec, NULL) < 0 ) 
	{  
        printf("Could not open codec.");  
        return -1;  
    }  
      
    //Write Header  
    avformat_write_header(pFormatCtx, NULL);
      
    int y_size = pCodecCtx->width * pCodecCtx->height;  
      
    //Encode  
    // 缁橝VPacket鍒嗛厤瓒冲?熷ぇ鐨勭┖闂?  
    AVPacket pkt;  
    av_new_packet(&pkt, y_size * 3);  
    
    int got_picture = 0;  
    int ret = avcodec_encode_video2(pCodecCtx, &pkt, pFrame, &got_picture);  
    if (ret < 0)
	{  
        printf("Encode Error.\n");  
        return -1;  
    } 
	
    if( got_picture == 1 )
	{  
        //pkt.stream_index = pAVStream->index;  
        ret = av_write_frame(pFormatCtx, &pkt);  
    }  
  
    av_free_packet(&pkt);  
  
    //Write Trailer  
    av_write_trailer(pFormatCtx);  
  
    printf("Encode Successful.\n");  
  
    if (pAVStream) 
	{  
        avcodec_close(pAVStream->codec);  
    }  
    avio_close(pFormatCtx->pb);  
    avformat_free_context(pFormatCtx);  
      
    return 0;  
}  



void media_yuv_scaler2bgr(int h, int w, int data_len, char *srcData, char * out_file)
{	
	AVCodecContext  *pCodecCtx = NULL;
	AVCodec         *pCodec = NULL;
	AVFrame         *pFrame = NULL;
	int got_frame = 0;
	pCodec = avcodec_find_decoder(AV_CODEC_ID_H264);
	if (pCodec == NULL)
	{		
		printf("Unsupported codec!\n");
		goto EXIT0;
	}

	pCodecCtx = avcodec_alloc_context3(pCodec);
	if (!pCodecCtx)
	{	
		printf("无法分配视频解码器上下文。");
		goto EXIT0;
	}

	if (avcodec_open2(pCodecCtx, pCodec, NULL) < 0)
	{	
		printf("Cannot open video decoder\n");
		goto EXIT1;
	}

	pFrame = av_frame_alloc();
	if (pFrame == NULL)
	{
		printf("it is err av_frame_alloc!!!!!!\n");
		goto EXIT1;
	}

	int ret;
	AVPacket* packet = (AVPacket *)malloc(sizeof(AVPacket));
	av_init_packet(packet);	
	packet->data = (uint8_t *)srcData;	
	packet->size = data_len;		
	ret = avcodec_decode_video2(pCodecCtx, pFrame, &got_frame, packet);	
	if (ret < 0)		
	{			
		printf("it is err avcodec_decode_video2!!!!\n");			
		goto EXIT2;
	}

	if (got_frame > 0)
	{
		printf("the pCodecCtx->fix=%d\n", pCodecCtx->pix_fmt);
		media_writeJpeg(pFrame, w, h, out_file);
	}
EXIT3:

EXIT2:

	if (NULL != pFrame)
	{
		av_frame_free(&pFrame);
		pFrame = NULL;
	}

	if (NULL != packet)
	{
		free(packet);
		packet = NULL;
	}	
EXIT1:
	
	if (NULL != pCodecCtx)
	{
		avcodec_close(pCodecCtx);
		//avcodec_free_context(pCodecCtx);
	}
	
EXIT0:
	return;
}


#if 0
int MediaReadFrame(DemuxReader * pDemuxReader,MediaPacket * pMediaPkt)
{
	AVPacket pkt;
	int iRet;
	int iTimeScale;
	AVStream * st;

	if (!pDemuxReader || !pMediaPkt)
	{
		return -1;
	}
	av_init_packet(&pkt);

#ifdef WIN32
	EnterCriticalSection(&pDemuxReader->cs);
#else
	pthread_mutex_lock(&pDemuxReader->mutex);
#endif

	iRet = av_read_frame(pDemuxReader->ic,&pkt);
	if (iRet < 0 )
	{
		pMediaPkt->iLen = 0;
		pMediaPkt->pData = NULL;
		pDemuxReader->bStreamEnd = 1;
#ifdef WIN32
		LeaveCriticalSection(&pDemuxReader->cs);
#else
		pthread_mutex_unlock(&pDemuxReader->mutex);
#endif
		return -1;
	}

#ifdef WIN32
	LeaveCriticalSection(&pDemuxReader->cs);
#else
	pthread_mutex_unlock(&pDemuxReader->mutex);
#endif

	if (pkt.stream_index == pDemuxReader->iAudioStreamIdx)
	{
		int iAdtsHeaderLen = 0;
		unsigned char * pAdtsHeader = NULL;
		int samplerate;
		int channels;

		pMediaPkt->bAudio = 1;
		pMediaPkt->bKeyFrame = 1;
		//printf("audio pts %ld \n",pkt.pts);
		if (pDemuxReader->eAudioCodecID == AV_CODEC_ID_AAC)
		{
			if (pDemuxReader->bNeedAACAdtsHeader && pkt.size > 7 && pkt.data[0] != 0xff)
			{
				samplerate = (pDemuxReader->iHE_AAC & 0x1)?pDemuxReader->iSampleRate/2:pDemuxReader->iSampleRate;
				channels = (pDemuxReader->iHE_AAC & 0x2)?pDemuxReader->iChannel/2:pDemuxReader->iChannel;
				pAdtsHeader = MakeAdtsHeader(&iAdtsHeaderLen,samplerate,channels,pkt.size);
			}
		}
		pMediaPkt->pData = av_malloc(pkt.size + iAdtsHeaderLen + AV_INPUT_BUFFER_PADDING_SIZE);
		if (pMediaPkt->pData == NULL)
		{
			dlog(LOG_ERROR,"MediaReadFrame malloc %d fail!\n",pkt.size);
			pMediaPkt->iLen = 0;
			pMediaPkt->pData = NULL;
			return -1;
		}
		if (iAdtsHeaderLen > 0 && pAdtsHeader)
		{
			memcpy(pMediaPkt->pData,pAdtsHeader,iAdtsHeaderLen);
			free(pAdtsHeader);
		}
		memcpy(pMediaPkt->pData + iAdtsHeaderLen,pkt.data,pkt.size);
		pMediaPkt->iLen = iAdtsHeaderLen + pkt.size;

	}
	else
	{
		AVPacket new_pkt;
		int iStreamIdx = pkt.stream_index;

		av_init_packet(&new_pkt);

		pMediaPkt->bAudio = 0;
		pMediaPkt->bKeyFrame = pkt.flags & AV_PKT_FLAG_KEY;
		if (pkt.data && pDemuxReader->aBsfc[iStreamIdx])
		{
			int a;

			new_pkt = pkt;
			a = av_bitstream_filter_filter(pDemuxReader->aBsfc[iStreamIdx],pDemuxReader->apVideoCodecCtx[iStreamIdx], NULL, &new_pkt.data, &new_pkt.size,pkt.data,pkt.size,pkt.flags & AV_PKT_FLAG_KEY);
			if(a == 0 && new_pkt.data != pkt.data && new_pkt.destruct) {
				uint8_t *t = av_malloc(new_pkt.size + AV_INPUT_BUFFER_PADDING_SIZE); //the new should be a subset of the old so cannot overflow
				if(t) {
					memcpy(t, new_pkt.data, new_pkt.size);
					memset(t + new_pkt.size, 0, AV_INPUT_BUFFER_PADDING_SIZE);
					new_pkt.data = t;
					new_pkt.buf = NULL;
					a = 1;
				} else
					a = AVERROR(ENOMEM);
			}
			if (a > 0) {
				av_free_packet(&pkt);
				new_pkt.buf = av_buffer_create(new_pkt.data, new_pkt.size,av_buffer_default_free, NULL, 0);
				if (!new_pkt.buf)
				{
					dlog(LOG_ERROR,"MediaReadFrame av_buffer_create fail!\n",pkt.size);
					pMediaPkt->iLen = 0;
					pMediaPkt->pData = NULL;
					return -1;
				}
			} else if (a < 0) {
				dlog(LOG_ERROR,"Failed to open bitstream filter %s for stream %d",pDemuxReader->aBsfc[iStreamIdx]->filter->name, pkt.stream_index);
				pMediaPkt->iLen = 0;
				pMediaPkt->pData = NULL;
				return -1;
			}
			pkt = new_pkt;
		}
		pMediaPkt->pData = av_malloc(pkt.size + AV_INPUT_BUFFER_PADDING_SIZE);
		if (pMediaPkt->pData == NULL)
		{
			dlog(LOG_ERROR,"MediaReadFrame av_malloc %d fail!\n",pkt.size);
			pMediaPkt->iLen = 0;
			pMediaPkt->pData = NULL;
			return -1;
		}
		memcpy(pMediaPkt->pData,pkt.data,pkt.size);
		pMediaPkt->iLen = pkt.size;
	}

	pMediaPkt->iStreamIdx = pkt.stream_index;
	st = pDemuxReader->ic->streams[pkt.stream_index];
	iTimeScale =st->time_base.den / st->time_base.num;//  / st->codec->ticks_per_frame;
	//pts = pkt.dts == AV_NOPTS_VALUE ?pkt.pts:pkt.dts;
	pMediaPkt->dts = pkt.dts * 1000 / iTimeScale;
	pMediaPkt->pts = pkt.pts * 1000 / iTimeScale;

	//printf("read ffmpeg packet pts %I64d dts %I64d  audio %d media pts %I64d ms\n",pkt.pts,pkt.dts,pMediaPkt->bAudio,pMediaPkt->pts);
	av_free_packet(&pkt);
	return 0;
}


int MediaSeek(DemuxReader * pDemuxReader,int iSeekTime)
{
	int ret;
	int64_t seektime;

	seektime = (int64_t)iSeekTime * AV_TIME_BASE/1000; 

#ifdef WIN32
	EnterCriticalSection(&pDemuxReader->cs);
#else
	pthread_mutex_lock(&pDemuxReader->mutex);
#endif
	dlog(LOG_ERROR,"seek to time %0.3f s\n", (double)seektime / AV_TIME_BASE);
	ret = avformat_seek_file(pDemuxReader->ic,-1, INT64_MIN, seektime, INT64_MAX, 0);

#ifdef WIN32
	LeaveCriticalSection(&pDemuxReader->cs);
#else
	pthread_mutex_unlock(&pDemuxReader->mutex);
#endif

	if(ret < 0)
	{
		dlog(LOG_ERROR," could not seek to position %0.3f\n", (double)seektime / AV_TIME_BASE);
	}
	return ret;
}

void MediaReaderClose(DemuxReader * pDemuxReader)
{
	int i;

	for(i= 0;i < pDemuxReader->ic->nb_streams;i++)
	{
		avcodec_close(pDemuxReader->ic->streams[i]->codec);
	}
	avformat_close_input(&pDemuxReader->ic);
	for (i = 0;i < MAX_STREAM_NUM;i++)
	{
		if (pDemuxReader->aBsfc[i])
		{
			av_bitstream_filter_close(pDemuxReader->aBsfc[i]);
		}
	}
#ifdef WIN32
	DeleteCriticalSection(&pDemuxReader->cs);
#else
	pthread_mutex_destroy(&pDemuxReader->mutex);
#endif

}

void MediaPacketFree(MediaPacket * pkt)
{
	if (pkt && pkt->iLen > 0 && pkt->pData)
	{
		av_free(pkt->pData);
		pkt->pData = NULL;
		pkt->iLen = 0;
	}
}

int MediaTranscodeInit(TransContext * pTransContext,char * InputFile,char * OutputFile)
{
	pTransContext->eAudioDecID = CODEC_ID_NONE;
	pTransContext->eAudioEncID = CODEC_ID_NONE;
	pTransContext->eVideoDecID = CODEC_ID_NONE;
	pTransContext->eVideoEncID = CODEC_ID_NONE;
	pTransContext->pAudioDecCtx = NULL;
	pTransContext->pAudioEncCtx = NULL;
	pTransContext->pVideoDecCtx = NULL;
	pTransContext->pVideoEncCtx = NULL;
	pTransContext->iDecVideoStreamIdx = -1;
	pTransContext->iEncVideoStreamIdx = -1;
	pTransContext->iDecAudioStreamIdx = -1;
	pTransContext->iEncVideoStreamIdx = -1;
	pTransContext->img_convert_ctx = NULL;
	pTransContext->bGetVideoDecFrame = 0;
	pTransContext->eSampleFmt = 0;
	pTransContext->iBitRate = 64000;
	pTransContext->iChannels = 0;
	pTransContext->iChannel_layout = av_get_default_channel_layout(2);
	pTransContext->iSampleRate = 0;
	pTransContext->bGetAudioDecFrame = 0;
	pTransContext->dAudioStartPts = -1;
	pTransContext->dAudioPts = 0;
	pTransContext->bOpenInput = 0;
	pTransContext->bOpenOutput =0;
	pTransContext->bAudioResample = 0;
	pTransContext->DecPixFmt = AV_PIX_FMT_YUV420P;
#ifdef USE_SWRESAMPLE
	pTransContext->swr = NULL;
#else
	pTransContext->audio_filter_graph = NULL;
#endif
	pTransContext->iResamplePcmLen = 0;

	pTransContext->video_filter_graph = NULL;

	if (InputFile)
	{
		AVCodec * pDecCodec;
		AVCodecContext * pDecCodecCtx;
		int i,ret;
	    int bAsf=0;

		MediaReaderInit(&pTransContext->DemuxRdr);
		MediaOpenFile(&pTransContext->DemuxRdr,InputFile);
		pTransContext->bOpenInput = 1;

	
		if (!strcmp("asf",pTransContext->DemuxRdr.ic->iformat->name))
			bAsf = 1;
		for (i = 0;i < pTransContext->DemuxRdr.ic->nb_streams;i++)
		{
			enum AVCodecID eCodecID;

			pDecCodecCtx = pTransContext->DemuxRdr.ic->streams[i]->codec;
			eCodecID = (pDecCodecCtx->codec_id == CODEC_ID_NONE) && (pDecCodecCtx->codec_type == AVMEDIA_TYPE_DATA ) && bAsf? CODEC_ID_AAC:pDecCodecCtx->codec_id;
			if ((eCodecID == CODEC_ID_AAC && bAsf && pDecCodecCtx->codec_type == AVMEDIA_TYPE_DATA) || pDecCodecCtx->codec_type == AVMEDIA_TYPE_AUDIO || pDecCodecCtx->codec_type == AVMEDIA_TYPE_VIDEO)
			{
			if (i == pTransContext->DemuxRdr.iVideoStreamIdx)
			{
				pTransContext->pVideoDecCtx = pDecCodecCtx;
				pTransContext->eVideoDecID = pDecCodecCtx->codec_id;
			}
			if (i == pTransContext->DemuxRdr.iAudioStreamIdx)
			{
				pTransContext->pAudioDecCtx =pDecCodecCtx;
				pTransContext->eAudioDecID = pTransContext->DemuxRdr.eAudioCodecID;
			}
			pDecCodec = avcodec_find_decoder(eCodecID);
			if (!pDecCodec) 
			{
				dlog(LOG_ERROR,"avcodec_find_decoder find codec id %d fail!\n",eCodecID);
				return -1;
			}
			if ((ret = avcodec_open2(pDecCodecCtx,pDecCodec,NULL)) < 0) 
			{
				dlog(LOG_ERROR,"could not open decoder codec error %d\n",ret);
				return -1;
			}
		}
		}
	}
	if (OutputFile)
	{
		int bHaveVideo =  1;

		if (pTransContext->bOpenInput && pTransContext->DemuxRdr.iVideoNum < 0) 
			bHaveVideo = 0;
		MediaWriterInit(&pTransContext->MuxWtr);
		MediaFileCreate(&pTransContext->MuxWtr,OutputFile,bHaveVideo);
		pTransContext->bOpenOutput = 1;
	}
	pTransContext->audio_filter_graph = avfilter_graph_alloc();
	if (!pTransContext->audio_filter_graph)
	{
		dlog(LOG_ERROR, "can't to create filter graph error!\n");
		return -1;
	}
	return 0;
}

int AudioDecInfoInit(TransContext * pTransContext,enum AVCodecID eCodecID,int samplerate,int bitrate,int channels,enum AVSampleFormat eFmt)
{
	AVCodec * pDecCodec     = NULL;
	AVCodecContext * pDecCodecCtx   = NULL;
	int ret;

	pDecCodec = avcodec_find_decoder(eCodecID);
	if (!pDecCodec) 
	{
		fprintf(stderr, "avcodec_find_decoder find codec id %d fail!\n",eCodecID);
		return -1;
	}

	if (!pTransContext->bOpenInput)
	{
		pDecCodecCtx = avcodec_alloc_context3(pDecCodec);
		pDecCodecCtx->bit_rate    = bitrate;
		pDecCodecCtx->sample_rate = samplerate;
		pDecCodecCtx->channels    = channels;
		pDecCodecCtx->sample_fmt  = eFmt;

		if ((ret = avcodec_open2(pDecCodecCtx,pDecCodec,NULL)) < 0) 
		{
			dlog(LOG_ERROR,"could not open decoder codec error %d\n",ret);
			return -1;
		}
		pTransContext->pAudioDecCtx = pDecCodecCtx;

	}
	if (!pTransContext->pAudioDecCtx->channel_layout)
		pTransContext->pAudioDecCtx->channel_layout = av_get_default_channel_layout(pTransContext->pAudioDecCtx->channels);
	pTransContext->eAudioDecID = eCodecID;
	pTransContext->iSampleRate = samplerate;
	pTransContext->iBitRate = bitrate;
	pTransContext->iChannels = channels;
	pTransContext->eSampleFmt = eFmt;
	pTransContext->iChannel_layout = pTransContext->pAudioDecCtx->channel_layout;

	return 0;
}

static int init_filter_graph(TransContext * pTransCtx, AVFilterContext **src,AVFilterContext **sink)
{
	AVFilterGraph *graph;
	AVFilterContext *abuffer_ctx;
	AVFilter        *abuffer;
	AVFilterContext *aformat_ctx;
	AVFilter        *aformat;
	AVFilterContext *abuffersink_ctx;
	AVFilter        *abuffersink;
	AVRational time_base;

	AVDictionary *options_dict = NULL;
	uint8_t options_str[1024];
	uint8_t ch_layout[64];

	int err;

	graph = pTransCtx->audio_filter_graph;
	/* Create the abuffer filter;
	* it will be used for feeding the data into the graph. */
	abuffer = avfilter_get_by_name("abuffer");
	if (!abuffer) {
		dlog(LOG_ERROR,"Could not find the abuffer filter.\n");
		return AVERROR_FILTER_NOT_FOUND;
	}

	abuffer_ctx = avfilter_graph_alloc_filter(graph, abuffer, "src");
	if (!abuffer_ctx) {
		dlog(LOG_ERROR, "Could not allocate the abuffer instance.\n");
		return AVERROR(ENOMEM);
	}

	time_base.num = 1;
	time_base.den = pTransCtx->iSampleRate;
	/* Set the filter options through the AVOptions API. */
	av_get_channel_layout_string(ch_layout, sizeof(ch_layout), 0,pTransCtx->iChannel_layout);
	av_opt_set    (abuffer_ctx, "channel_layout", ch_layout,                            AV_OPT_SEARCH_CHILDREN);
	av_opt_set    (abuffer_ctx, "sample_fmt",     av_get_sample_fmt_name(pTransCtx->eSampleFmt), AV_OPT_SEARCH_CHILDREN);
	av_opt_set_q  (abuffer_ctx, "time_base",     time_base ,  AV_OPT_SEARCH_CHILDREN);
	av_opt_set_int(abuffer_ctx, "sample_rate",    pTransCtx->iSampleRate,                     AV_OPT_SEARCH_CHILDREN);

	/* Now initialize the filter; we pass NULL options, since we have already
	* set all the options above. */
	err = avfilter_init_str(abuffer_ctx, NULL);
	if (err < 0) {
		dlog(LOG_ERROR,"Could not initialize the abuffer filter.\n");
		return err;
	}
	/* Create the aformat filter;
	* it ensures that the output is of the format we want. */
	aformat = avfilter_get_by_name("aformat");
	if (!aformat) {
		dlog(LOG_ERROR, "Could not find the aformat filter.\n");
		return AVERROR_FILTER_NOT_FOUND;
	}

	aformat_ctx = avfilter_graph_alloc_filter(graph, aformat, "aformat");
	if (!aformat_ctx) {
		dlog(LOG_ERROR, "Could not allocate the aformat instance.\n");
		return AVERROR(ENOMEM);
	}

	/* A third way of passing the options is in a string of the form
	* key1=value1:key2=value2.... */
	sprintf(options_str,
		"sample_fmts=%s:sample_rates=%d:channel_layouts=0x%"PRIx64,
		av_get_sample_fmt_name(pTransCtx->pAudioEncCtx->sample_fmt),pTransCtx->pAudioEncCtx->sample_rate,
		av_get_default_channel_layout(pTransCtx->pAudioEncCtx->channels));
	err = avfilter_init_str(aformat_ctx, options_str);
	if (err < 0) {
		av_log(NULL, AV_LOG_ERROR, "Could not initialize the aformat filter.\n");
		return err;
	}

	/* Finally create the abuffersink filter;
	* it will be used to get the filtered data out of the graph. */
	abuffersink = avfilter_get_by_name("abuffersink");
	if (!abuffersink) {
		dlog(LOG_ERROR, "Could not find the abuffersink filter.\n");
		return AVERROR_FILTER_NOT_FOUND;
	}

	abuffersink_ctx = avfilter_graph_alloc_filter(graph, abuffersink, "sink");
	if (!abuffersink_ctx) {
		dlog(LOG_ERROR,"Could not allocate the abuffersink instance.\n");
		return AVERROR(ENOMEM);
	}

	/* This filter takes no options. */
	err = avfilter_init_str(abuffersink_ctx, NULL);
	if (err < 0) {
		dlog(LOG_ERROR, "Could not initialize the abuffersink instance.\n");
		return err;
	}

	/* Connect the filters;
	* in this simple case the filters just form a linear chain. */
	err = avfilter_link(abuffer_ctx, 0, aformat_ctx, 0);
	if (err >= 0)
		err = avfilter_link(aformat_ctx, 0, abuffersink_ctx, 0);
	if (err < 0) {
		dlog(LOG_ERROR, "Error connecting filters\n");
		return err;
	}
	if (!(pTransCtx->pAudioEncCtx->codec->capabilities & CODEC_CAP_VARIABLE_FRAME_SIZE))
		av_buffersink_set_frame_size(abuffersink_ctx,pTransCtx->pAudioEncCtx->frame_size);

	/* Configure the graph. */
	err = avfilter_graph_config(graph, NULL);
	if (err < 0) {
		av_log(NULL, AV_LOG_ERROR, "Error configuring the filter graph\n");
		return err;
	}
	*src   = abuffer_ctx;
	*sink  = abuffersink_ctx;

	return 0;
}

int AudioEncInfoInit(TransContext * pTransContext,enum AVCodecID eCodecID,int samplerate,int bitrate,int channels,enum AVSampleFormat eFmt)
{
	int idx;
	AVCodec * pEncCodec     = NULL;
	AVCodecContext * pEncCodecCtx   = NULL;


	pEncCodec = avcodec_find_encoder_by_name("libfdk_aac");//libaacplus libfaac libfdk_aac
	if (!pEncCodec)
	{
		pEncCodec = avcodec_find_encoder(eCodecID);
		if (!pEncCodec) 
		{
			dlog(LOG_ERROR,"encoder  %d not found\n",eCodecID);
			return -1;
		}
	}


	if (pTransContext->bOpenOutput)
	{
		idx = AudioStreamAdd(&pTransContext->MuxWtr,eCodecID,samplerate,bitrate,channels,eFmt);
		pTransContext->pAudioEncCtx = pTransContext->MuxWtr.pAudioCodecCtx;
		pTransContext->iEncAudioStreamIdx = idx;
	}
	else
	{
		pEncCodecCtx = avcodec_alloc_context3(pEncCodec);

		pEncCodecCtx->sample_rate = samplerate;
		pEncCodecCtx->channels = channels;
		pEncCodecCtx->bit_rate = bitrate;
		pEncCodecCtx->sample_fmt = eFmt;

		pTransContext->pAudioEncCtx = pEncCodecCtx;
	}
#ifdef CONFIG_LIBFDK_AAC_ENCODER
	if (CONFIG_LIBFDK_AAC_ENCODER)
		pTransContext->pAudioEncCtx->profile = FF_PROFILE_AAC_HE;//SBR
#endif
	if (avcodec_open2(pTransContext->pAudioEncCtx,pEncCodec,NULL) < 0) 
	{
		dlog(LOG_ERROR, "could not open encoder codec\n");
		return -1;
	}
	if (!pTransContext->pAudioEncCtx->channel_layout)
		pTransContext->pAudioEncCtx->channel_layout = av_get_default_channel_layout(pTransContext->pAudioEncCtx->channels);
	if (pTransContext->iSampleRate != samplerate || pTransContext->iChannels != channels || pTransContext->eSampleFmt != eFmt)
	{
		if (pTransContext->iSampleRate && pTransContext->iChannels && pTransContext->eSampleFmt)
		{
#ifdef USE_SWRESAMPLE
			pTransContext->swr = swr_alloc_set_opts(pTransContext->swr,pTransContext->pAudioEncCtx->channel_layout,eFmt,samplerate,av_get_default_channel_layout(pTransContext->iChannels),pTransContext->eSampleFmt,pTransContext->iSampleRate,0,NULL);
			if(pTransContext->swr && swr_init(pTransContext->swr) < 0)
			{
				dlog(LOG_ERROR,"swr_init() failed\n");
				swr_free(&pTransContext->swr);
				return -1;
			}
#else
			init_filter_graph(pTransContext,&pTransContext->abuffersrc_ctx,&pTransContext->abuffersink_ctx);
#endif
			pTransContext->bAudioResample = 1;
		}

	}

	return 0;
}

int AudioPcmInfoInit(TransContext * pTransContext,int samplerate,int channels,enum AVSampleFormat eFmt)
{
	pTransContext->iSampleRate = samplerate;
	pTransContext->iChannels = channels;
	pTransContext->eSampleFmt = eFmt;
	return 0;
}
int AudioAddPcmData(TransContext * pTransContext,char * pPcmBuf,int iPcmLen)
{

	int channels = pTransContext->iChannels;
	int iBytes = av_get_bytes_per_sample(pTransContext->eSampleFmt);

	pTransContext->pAudioDecFrame = av_frame_alloc();
	pTransContext->pAudioDecFrame->nb_samples     = iPcmLen / channels / iBytes;
	pTransContext->pAudioDecFrame->format         = pTransContext->eSampleFmt;
	pTransContext->pAudioDecFrame->channel_layout = av_get_default_channel_layout(channels);
	pTransContext->pAudioDecFrame->channels = channels;
	pTransContext->pAudioDecFrame->sample_rate = pTransContext->iSampleRate;

	//av_samples_alloc(pTransContext->pAudioDecFrame->extended_data,pTransContext->pAudioDecFrame->linesize,channels,pTransContext->pAudioEncCtx->frame_size,pTransContext->pAudioEncCtx->sample_fmt,0);
	avcodec_fill_audio_frame(pTransContext->pAudioDecFrame,channels,pTransContext->eSampleFmt,pPcmBuf,iPcmLen,0);
	pTransContext->bGetAudioDecFrame = 1;
	return 0;
}

int AudioDeocde(TransContext * pTransContext,MediaPacket * pMediaPkt)
{
//	AVFrame * pAudioFrame;
	int bGetFrame = 0;
	AVCodecContext * pAVCodecCtx = pTransContext->pAudioDecCtx;
//	int channels = pAVCodecCtx->channels;
//	int iBytes = av_get_bytes_per_sample(pAVCodecCtx->sample_fmt);
	AVPacket pkt;
	int ret;

	if (!pTransContext || !pMediaPkt)
		return -1;

	av_init_packet(&pkt);
	pkt.data = pMediaPkt->pData;
	pkt.size = pMediaPkt->iLen;


	while(1)
	{
		pTransContext->pAudioDecFrame = av_frame_alloc();
		ret = avcodec_decode_audio4(pAVCodecCtx,pTransContext->pAudioDecFrame,&bGetFrame,&pkt);
		if(bGetFrame)
			pTransContext->bGetAudioDecFrame = 1;
		else
			pTransContext->bGetAudioDecFrame = 0;
		if (bGetFrame && ret > 0 && ret < pkt.size)
		{
			pkt.data = pkt.data + ret;
			pkt.size -= ret;
			if(pkt.size > 0)
			{
				dlog(LOG_ERROR,"warning multi audio frame in one packet!!!skip other audio frame\n");
				break;
			}
		}
		else
		{
			break;
		}
	}
	return 0;
}

int AudioEncode(TransContext * pTransContext,MediaPacket * OutPkt)
{
	AVCodecContext * pEncCodecCtx = pTransContext->pAudioEncCtx;
	int iAudioEsLen;
	int iFrameSize = pEncCodecCtx->frame_size;
//	int channels = pEncCodecCtx->channels;
//	int iBytes = av_get_bytes_per_sample(pEncCodecCtx->sample_fmt);
	int ret = 0;
	AVPacket pkt;
	int bGetFrame = 0;

	av_init_packet(&pkt);
	
	OutPkt->iLen = 0;

	iAudioEsLen = MAX_AUDIO_ES_LEN;
	OutPkt->pData = av_malloc(iAudioEsLen + AV_INPUT_BUFFER_PADDING_SIZE);
	if (!OutPkt->pData)
	{
		dlog(LOG_ERROR,"audio es av_malloc fail!\n");
		return -1;
	}
	pkt.data = OutPkt->pData;
	pkt.size =iAudioEsLen;
	if (pTransContext->bAudioResample)
	{
		int err;
		AVFrame * filter_frame = NULL;
#ifdef USE_SWRESAMPLE
		int size_out;
		static int sample_count = 0;
		int iDecFrameSize = 0;

		iDecFrameSize = pTransContext->pAudioDecCtx->frame_size;
		pTransContext->pAudioEncFrame = av_frame_alloc();
		av_samples_alloc(pTransContext->pAudioEncFrame->extended_data,pTransContext->pAudioEncFrame->linesize,channels,iFrameSize,pEncCodecCtx->sample_fmt,0);
		size_out = swr_convert(pTransContext->swr,pTransContext->pAudioEncFrame->extended_data,iFrameSize,pTransContext->pAudioDecFrame->extended_data,iDecFrameSize);

		if (sample_count + size_out < iFrameSize)
		{
			pTransContext->pAudioEncFrame->nb_samples = size_out;
			ret = avcodec_encode_audio2(pEncCodecCtx,&pkt,pTransContext->pAudioEncFrame,&bGetFrame);
			sample_count += size_out;
		}
		else
		{
			int bTmp;
			pTransContext->pAudioEncFrame->nb_samples = iFrameSize - sample_count;
			ret = avcodec_encode_audio2(pEncCodecCtx,&pkt,pTransContext->pAudioEncFrame,&bGetFrame);
			if (size_out > (iFrameSize - sample_count))
			{
				AVFrame * frame;
				AVPacket tmp_pkt;

				av_init_packet(&tmp_pkt);
				tmp_pkt.size = 0;
				frame = av_frame_alloc();
				av_samples_alloc(frame->extended_data,frame->linesize,channels,iFrameSize,pEncCodecCtx->sample_fmt,0);
				memcpy(frame->data[0],pTransContext->pAudioEncFrame->data[0] + 2 *(iFrameSize - sample_count)*2,(size_out - (iFrameSize - sample_count)) * 2 * 2);
				frame->nb_samples = size_out - (iFrameSize - sample_count);
				frame->linesize[0] = (size_out - (iFrameSize - sample_count)) * 2 * 2;
				avcodec_encode_audio2(pEncCodecCtx,&tmp_pkt,frame,&bTmp);
			}
			sample_count = size_out - (iFrameSize - sample_count);
		}
		av_frame_free(&pTransContext->pAudioEncFrame);
#else
		filter_frame = av_frame_alloc();
		ret = av_buffersrc_add_frame(pTransContext->abuffersrc_ctx,pTransContext->pAudioDecFrame);
		if (ret < 0) 
		{
			av_frame_unref(pTransContext->pAudioDecFrame);
			dlog(LOG_ERROR, "Error submitting the frame to the filtergraph:");
			av_frame_free(&filter_frame);
			goto ERR_ENC_AUDIO_EXIT;
		}
		err = av_buffersink_get_frame(pTransContext->abuffersink_ctx, filter_frame);
		if (err >= 0)
		{
			ret = avcodec_encode_audio2(pEncCodecCtx,&pkt,filter_frame,&bGetFrame);
			//av_frame_unref(filter_frame);
		}
		av_frame_free(&filter_frame);
#endif
	}
	else
	{
		ret = avcodec_encode_audio2(pEncCodecCtx,&pkt,pTransContext->pAudioDecFrame,&bGetFrame);
	}

ERR_ENC_AUDIO_EXIT:

	if (pTransContext->pAudioDecFrame)
	{
		if (pTransContext->pAudioDecFrame->extended_data != pTransContext->pAudioDecFrame->data)
			av_freep(&pTransContext->pAudioDecFrame->extended_data);
		av_frame_free(&pTransContext->pAudioDecFrame);
	}

	if (bGetFrame)
	{

		OutPkt->iLen = pkt.size;
		OutPkt->dts = OutPkt->pts = pTransContext->dAudioPts / pEncCodecCtx->sample_rate;
		OutPkt->bAudio = 1;
		OutPkt->bKeyFrame = 1;
		OutPkt->iStreamIdx = pTransContext->iEncAudioStreamIdx;
		pTransContext->dAudioPts += 1000.0 * iFrameSize;
	}
	else 
	{
		if (OutPkt->pData)
		{
			av_free(OutPkt->pData);
			OutPkt->pData = NULL;
			OutPkt->iLen = 0;
		}
	}
	av_free_packet(&pkt);
	return  ret;
}
int VideoDecInfoInit(TransContext * pTransContext,enum AVCodecID eCodecID,int width,int height,int iOutWidth,int iOutHeight,enum AVPixelFormat OuntFmt)
{
	AVCodec * pDecCodec     = NULL;
	AVCodecContext * pDecCodecCtx   = NULL;
	int ret;

	pDecCodec = avcodec_find_decoder(eCodecID);
	if (!pDecCodec) 
	{
		dlog(LOG_ERROR, "avcodec_find_decoder can't find codec id %d fail!\n",eCodecID);
		return -1;
	}
	if (!pTransContext->bOpenInput)
	{
		pDecCodecCtx = avcodec_alloc_context3(pDecCodec);
		pDecCodecCtx->width = width;
		pDecCodecCtx->height = height;
		pTransContext->pVideoDecCtx = pDecCodecCtx;
		pTransContext->eVideoDecID = eCodecID;
		if ((ret = avcodec_open2(pDecCodecCtx,pDecCodec,NULL)) < 0) 
		{
			dlog(LOG_ERROR,"could not open decoder codec error %d\n",ret);
			return -1;
		}	
	}
	else
	{
		if(eCodecID == CODEC_ID_H264)
			pTransContext->DemuxRdr.bMp4H264BitStream = 1;
	}
	pTransContext->iDecOutWidth = pTransContext->iDecOutHeight = 0;
	pTransContext->DecPixFmt = PIX_FMT_YUV420P;
	if (iOutWidth > 0 && iOutHeight > 0 && (width != iOutWidth || height !=iOutHeight))
	{
		pTransContext->iDecOutWidth = iOutWidth;
		pTransContext->iDecOutHeight = iOutHeight;
#ifdef USE_SWSCALE
		pTransContext->img_convert_ctx = sws_getContext(width,height,PIX_FMT_YUV420P,iOutWidth, iOutHeight,OuntFmt,SWS_BICUBIC, NULL, NULL, NULL);
		if (pTransContext->img_convert_ctx == NULL)
		{
			dlog(LOG_ERROR,"sws_getContext src width %d src height %d out width %d out height %d fail!!!",width,height,iOutWidth,iOutHeight);
		}
#endif
	}

	return 0;
}



int VideoEncInfoInit(TransContext * pTransContext,enum AVCodecID eCodecID,int width,int height,int bitrate,int fps,int gop,int profile,int preset)
{
	int idx = 0;
	AVCodec * pEncCodec     = NULL;
	AVCodecContext * pEncCodecCtx   = NULL;

	pEncCodec = avcodec_find_encoder(eCodecID);
	if (!pEncCodec) 
	{
		dlog(LOG_ERROR,"encoder  %d not found\n",eCodecID);
		return -1;
	}
	fps = fps > 0 ? fps:25;
	if (pTransContext->bOpenOutput)
	{
		idx = VideoStreamAdd(&pTransContext->MuxWtr,eCodecID,width,height,bitrate,fps);
		pTransContext->pVideoEncCtx = pTransContext->MuxWtr.apVideoCodecCtx[idx];
	}
	else
	{

		pEncCodecCtx = avcodec_alloc_context3(pEncCodec);
		pEncCodecCtx->width = width;
		pEncCodecCtx->height = height;
		pEncCodecCtx->bit_rate = bitrate;
		pEncCodecCtx->pix_fmt = eCodecID == CODEC_ID_MJPEG?PIX_FMT_YUVJ420P:PIX_FMT_YUV420P;
		pEncCodecCtx->bit_rate = bitrate;
		pEncCodecCtx->time_base.den = fps;
		pEncCodecCtx->time_base.num = 1;

		pTransContext->pVideoEncCtx = pEncCodecCtx;
	}
	if (eCodecID == CODEC_ID_H264)
		h264_encode_para_set(pTransContext->pVideoEncCtx,gop,profile,preset);

	pTransContext->pVideoEncCtx->pix_fmt = eCodecID == CODEC_ID_MJPEG?PIX_FMT_YUVJ420P:PIX_FMT_YUV420P;
	if (avcodec_open2(pTransContext->pVideoEncCtx,pEncCodec,NULL) < 0) 
	{
		dlog(LOG_ERROR, "could not open encoder codec\n");
		return -1;
	}
	return idx;
}

int VideoDeocde(TransContext * pTransContext,MediaPacket * InPkt)
{
	AVPacket pkt;
	int bGetPicture = 0;
	int ret = 0;
	AVCodecContext * pDecCodecCtx;
	int iOffset = 0;
//	AVFrame *pDstFrame;
//	int iFrameSize;
//	char * pFrameBuf;



	av_init_packet(&pkt);

	if (pTransContext->bOpenInput && InPkt->iStreamIdx >= 0)
	{
		pDecCodecCtx = pTransContext->DemuxRdr.ic->streams[InPkt->iStreamIdx]->codec;
	}
	else
	{
		pDecCodecCtx = pTransContext->pVideoDecCtx;
	}

	pkt.stream_index = InPkt->iStreamIdx;
	if (pDecCodecCtx->codec_id == AV_CODEC_ID_H264)
		iOffset = RemoveSpsPps((int8_t*)InPkt->pData,InPkt->iLen);
	pkt.size = InPkt->iLen - iOffset;
	pkt.data = InPkt->pData + iOffset;
	pkt.flags = InPkt->bKeyFrame?AV_PKT_FLAG_KEY:0;
	pkt.pts = InPkt->pts;
	pkt.dts = InPkt->dts;
	//av_dup_packet(&pkt);

	pTransContext->pVideoDecFrame = av_frame_alloc();
	ret = avcodec_decode_video2(pDecCodecCtx,pTransContext->pVideoDecFrame,&bGetPicture,&pkt);
	pTransContext->bGetVideoDecFrame = bGetPicture;
	pTransContext->pVideoDecFrame->pict_type = AV_PICTURE_TYPE_NONE;
	if (ret > 0)
	{
		pkt.data = pkt.data + ret;
		pkt.size -= ret;
		if (pkt.size > 0)
		{
			dlog(LOG_ERROR,"Discard some data %d bytes not decode!!! perhaps one packet have multiple frames!!\n",pkt.size);
		}
	}
#ifdef USE_SWSCALE
	if (bGetPicture && pTransContext->img_convert_ctx)
	{
		pDstFrame = av_frame_alloc();
		pDstFrame->pts = pTransContext->pVideoDecFrame->pts;
		pDstFrame->pkt_pts = pTransContext->pVideoDecFrame->pkt_pts;//pts
		pDstFrame->pkt_dts = pTransContext->pVideoDecFrame->pkt_dts;
		iFrameSize = avpicture_get_size(AV_PIX_FMT_YUV420P,pTransContext->iDecOutWidth,pTransContext->iDecOutHeight);
		pFrameBuf = (uint8_t*)av_malloc(iFrameSize);
		if (pFrameBuf == NULL)
		{
			dlog(LOG_ERROR,"pFrameBuf av_malloc %d fail!\n",iFrameSize);
			return -1;
		}
		avpicture_fill((AVPicture *)pDstFrame, pFrameBuf, AV_PIX_FMT_YUV420P,pTransContext->iDecOutWidth,pTransContext->iDecOutHeight);

		ret = sws_scale(pTransContext->img_convert_ctx,(const uint8_t * const *)pTransContext->pVideoDecFrame->data,pTransContext->pVideoDecFrame->linesize,0,pDecCodecCtx->height,(uint8_t * const *)pDstFrame->data,pDstFrame->linesize);	
		if (ret < 0)
		{
			dlog(LOG_ERROR,"sws_scale fail!\n");
			return -1;
		}
		av_frame_free(&pTransContext->pVideoDecFrame);
		pTransContext->pVideoDecFrame = pDstFrame;
	}
#endif
	if (bGetPicture)
		pTransContext->pVideoDecFrame->key_frame = InPkt->bKeyFrame;
	else
		av_frame_free(&pTransContext->pVideoDecFrame);
	return 0;
}
int VideoEncode(TransContext * pTransContext,MediaPacket * OutPkt)
{
//	int iOutEsLen = MAX_VIDEO_ES_LEN;
//	char * pOutEs = NULL;
	int ret = 0;
//	int64_t pts;
	AVPacket out_pkt;
	int get_pkt = 0;
	AVRational time_base = {1,1000};

	OutPkt->iLen = 0;
	av_init_packet(&out_pkt);
	out_pkt.data = NULL;
	out_pkt.size = 0;
	if(!pTransContext->pVideoEncCtx)
	{
		dlog(LOG_ERROR,"can't encode video,encode codec context is NULL error!");
		ret = -1;
		goto VIDEO_ENCODE_EXIT;
	}
	if (pTransContext->bGetVideoDecFrame == 0)
	{
		ret = avcodec_encode_video2(pTransContext->pVideoEncCtx,&out_pkt,NULL,&get_pkt);
	}
	else
	{
		//pTransContext->pVideoDecFrame->pts = pTransContext->pVideoDecFrame->pkt_dts * pTransContext->pVideoEncCtx->time_base.den / pTransContext->pVideoEncCtx->time_base.num /1000;
		ret = avcodec_encode_video2(pTransContext->pVideoEncCtx,&out_pkt,pTransContext->pVideoDecFrame,&get_pkt);
	}
	if (get_pkt)
	{
		OutPkt->pData = av_malloc(out_pkt.size + AV_INPUT_BUFFER_PADDING_SIZE);
		memcpy(OutPkt->pData,out_pkt.data,out_pkt.size);
		OutPkt->iLen = out_pkt.size;
		OutPkt->dts = av_rescale_q(out_pkt.dts,pTransContext->pVideoEncCtx->time_base,time_base);
		OutPkt->pts = av_rescale_q(out_pkt.pts,pTransContext->pVideoEncCtx->time_base,time_base);
		OutPkt->bAudio = 0;
		OutPkt->bKeyFrame = (out_pkt.flags & AV_PKT_FLAG_KEY) ? 1 : 0;
		av_free_packet(&out_pkt);
	}

VIDEO_ENCODE_EXIT:
	if (ret < 0)
	{
		OutPkt->iLen = 0;
	}
	if(pTransContext->bGetVideoDecFrame)
	{
#ifdef USE_SWSCALE
		if(pTransContext->img_convert_ctx && pTransContext->pVideoDecFrame->data[0])
			av_free(pTransContext->pVideoDecFrame->data[0]);
#endif
		av_frame_free(&pTransContext->pVideoDecFrame);
		pTransContext->bGetVideoDecFrame = 0;
	}
	return ret;
}
int VideoFilterInit(TransContext * pTransContext,const char *filters_descr)
{
	AVFilterGraph * filter_graph;
	AVFilterContext *buffer_ctx;
	AVFilter        *buffer;
	AVFilterContext *buffersink_ctx;
	AVFilter        *buffersink;
	AVFilterInOut *outputs = avfilter_inout_alloc();
	AVFilterInOut *inputs  = avfilter_inout_alloc();
	int err;
	struct AVRational time_base = {1,1000};


	 filter_graph  = avfilter_graph_alloc();
	 if (!filter_graph) 
	 {
		 dlog(LOG_ERROR, "Unable to create videl filter graph.\n");
		 return -1;
	 }
	 buffer = avfilter_get_by_name("buffer");
	 if (!buffer) 
	 {
		 dlog(LOG_ERROR,"Could not find the buffer filter.\n");
		 return -1;
	 }

	 buffer_ctx = avfilter_graph_alloc_filter(filter_graph, buffer, "src");
    if (!buffer_ctx) {
        dlog(LOG_ERROR, "Could not allocate the buffer instance.\n");
        return -1;
    }

    av_opt_set_int(buffer_ctx,"width",pTransContext->pVideoDecCtx->width,AV_OPT_SEARCH_CHILDREN);
	av_opt_set_int(buffer_ctx,"height",pTransContext->pVideoDecCtx->height,AV_OPT_SEARCH_CHILDREN);
	av_opt_set(buffer_ctx,"pix_fmt","yuv420p",AV_OPT_SEARCH_CHILDREN);//av_get_pix_fmt_name(pTransContext->DecPixFmt)
	av_opt_set_q(buffer_ctx,"time_base",time_base,AV_OPT_SEARCH_CHILDREN);

    err = avfilter_init_str(buffer_ctx, NULL);
    if (err < 0) {
        dlog(LOG_ERROR,"Could not initialize the buffer filter.\n");
        return err;
    }

	 buffersink = avfilter_get_by_name("buffersink");
	 if (!buffersink) {
		 dlog(LOG_ERROR, "Could not find the buffersink filter.\n");
		 return -1;
	 }

	 buffersink_ctx = avfilter_graph_alloc_filter(filter_graph, buffersink, "sink");
	 if (!buffersink_ctx) 
	 {
		 dlog(LOG_ERROR,"Could not allocate the buffersink instance.\n");
		 return -1;
	 }

	 err = avfilter_init_str(buffersink_ctx, NULL);
	 if (err < 0) {
		 dlog(LOG_ERROR,"Could not initialize the abuffersink instance.\n");
		 return err;
	 }

	 outputs->name       = av_strdup("in");
	 outputs->filter_ctx = buffer_ctx;
	 outputs->pad_idx    = 0;
	 outputs->next       = NULL;

	 inputs->name       = av_strdup("out");
	 inputs->filter_ctx = buffersink_ctx;
	 inputs->pad_idx    = 0;
	 inputs->next       = NULL;


	 if ((err = avfilter_graph_parse_ptr(filter_graph, filters_descr,
		 &inputs, &outputs, NULL)) < 0)
	 {
		 dlog(LOG_ERROR,"Error avfilter_graph_parse_ptr filters err %d\n",err);
		 return -1;
	 }
	 err = avfilter_graph_config(filter_graph, NULL);
	 if (err < 0) {
		 av_log(NULL, AV_LOG_ERROR, "Error configuring the filter graph\n");
		 return err;
	 }
	 pTransContext->video_filter_graph = filter_graph;
	 pTransContext->vbuffersrc_ctx = buffer_ctx;
	 pTransContext->vbuffersink_ctx = buffersink_ctx;

	 avfilter_inout_free(&inputs);
	 avfilter_inout_free(&outputs);
	 return 0;

}
  

int VideoFilterInputFrame(TransContext * pTransContext)
{
	int ret = 0;
//	struct AVRational time_base = {1,1000};
	AVFrame * frame;

	if (!pTransContext->pVideoDecFrame || !pTransContext->bGetVideoDecFrame)
	{
		return -1;
	}
    frame = pTransContext->pVideoDecFrame;
	frame->pts = frame->pkt_dts;
	ret = av_buffersrc_add_frame_flags(pTransContext->vbuffersrc_ctx,frame,0);//AV_BUFFERSRC_FLAG_PUSH  
	if (ret < 0) 
	{
		av_frame_unref(frame);
		dlog(LOG_ERROR, "Error submitting the frame to the filtergraph:");
		return -1;
	}
	av_frame_free(&pTransContext->pVideoDecFrame);
	pTransContext->bGetVideoDecFrame = 0;
	pTransContext->pVideoDecFrame = NULL;
	return ret;
}

int VideoFilterOutputFrame(TransContext * pTransContext)
{
	int ret = 0;
	AVFrame * frame = av_frame_alloc();

	if (!frame)
	{
		dlog(LOG_ERROR,"VideoFilterOutputFrame av_frame_alloc fail!\n");
		return -1;
	}
	ret = av_buffersink_get_frame_flags(pTransContext->vbuffersink_ctx, frame,0);//AV_BUFFERSINK_FLAG_NO_REQUEST
	if (ret < 0 && ret != -EAGAIN)
	{
			char errbuf[1024] = {0};

			av_strerror(ret,errbuf,sizeof(errbuf));
			dlog(LOG_ERROR,"av_buffersink_get_frame %s",errbuf);//av_err2str(ret)
			return -1;
	}
	if (ret == 0)
	{
		pTransContext->bGetVideoDecFrame = 1;
		pTransContext->pVideoDecFrame = frame;
	}
	else
	{
		av_frame_free(&frame);
	}
	return ret;
}

int MediaTranscodeClose(TransContext * pTransContext)
{
	if (pTransContext->pAudioDecCtx)
	{
		avcodec_close(pTransContext->pAudioDecCtx);
	}

	if (pTransContext->pVideoDecCtx)
	{
		avcodec_close(pTransContext->pVideoDecCtx);
	}
	if (pTransContext->pAudioEncCtx)
	{
		avcodec_close(pTransContext->pAudioEncCtx);
	}
	if (pTransContext->pVideoEncCtx)
	{
		avcodec_close(pTransContext->pVideoEncCtx);
	}
	if (pTransContext->bOpenInput)
	{
		MediaReaderClose(&pTransContext->DemuxRdr);
	}
	else
	{
		if (pTransContext->pAudioDecCtx)
			av_free(pTransContext->pAudioDecCtx);
		if (pTransContext->pVideoDecCtx)
			av_free(pTransContext->pVideoDecCtx);
	}
	if (pTransContext->bOpenOutput)
	{
		MediaWriteTrailer(&pTransContext->MuxWtr);
		MediaWriterClose(&pTransContext->MuxWtr);
	}
	else
	{
		if (pTransContext->pAudioEncCtx)
			av_free(pTransContext->pAudioEncCtx);
		if (pTransContext->pVideoEncCtx)
			av_free(pTransContext->pVideoEncCtx);
	}

	if (pTransContext->img_convert_ctx)
		sws_freeContext(pTransContext->img_convert_ctx);
#ifdef USE_SWRESAMPLE
	if (pTransContext->bAudioResample && pTransContext->swr)
		swr_free(&pTransContext->swr);
#else
	if (pTransContext->bAudioResample && pTransContext->audio_filter_graph)
	{
		avfilter_graph_free(&pTransContext->audio_filter_graph);
	}
#endif
	return 0;
}


#define EXTRA_DATA_OFFSET 2000
#define FRAME_INFO_OFFSET 4000

int Mp4Recovery(char * RecoveryFile,char * InFile,char * OutFile)
{
	MuxWriter MuxWtr;
	FILE * fp,* fpRead;
	uint64_t offset;
	int64_t dts;
	int i,iAudioStreamIdx,iStreamNum,iStreamIdx,iFrameSize,bVideo,bKeyFrame,iBitRate,iWidth,iHeight,iSampleRate,iChannel;
	enum AVCodecID eCodecID;
	enum AVSampleFormat eSampleFmt;
	unsigned char * pVideoBuf, * pAudioBuf;
	int iExtraDataNum,iExtraDataSize;
	char acExtraData[1024];
	int64_t last_dts = -1;
	int duration = 0;

	fp = fopen(RecoveryFile,"rb");
	if (fp == NULL)
	{
		dlog(LOG_ERROR,"open %s fail!\n",RecoveryFile);
		return -1;
	}

	fpRead = fopen(InFile,"rb");
	if (fpRead == NULL)
	{
		dlog(LOG_ERROR,"open %s fail!\n",InFile);
		goto EXIT_ERROR;
	}
	MediaSysInit();
	MediaWriterInit(&MuxWtr);
	MediaFileCreate(&MuxWtr,OutFile,1);//只写音频??

	iAudioStreamIdx = -1;

	fread(&iStreamNum,1,4,fp);
	for(i = 0;i < iStreamNum;i++)
	{
		fread(&iStreamIdx,1,4,fp);
		fread(&bVideo,1,4,fp);
		fread(&eCodecID,1,4,fp);
		if (bVideo)
		{

			fread(&iBitRate,1,4,fp);
			fread(&iWidth,1,4,fp);
			fread(&iHeight,1,4,fp);
			VideoStreamAdd(&MuxWtr,eCodecID,iWidth,iHeight,iBitRate,30);
		}
		else
		{
			fread(&iSampleRate,1,4,fp);
			fread(&iBitRate,1,4,fp);
			fread(&iChannel,1,4,fp);
			fread(&eSampleFmt,1,4,fp);
			AudioStreamAdd(&MuxWtr,eCodecID,iSampleRate,iBitRate,iChannel,eSampleFmt);
			iAudioStreamIdx = i;
		}
	}
	fseek(fp,EXTRA_DATA_OFFSET,SEEK_SET);
	fread(&iExtraDataNum,1,4,fp);


	for(i = 0;i < iExtraDataNum;i++)
	{
		AVCodecContext * pCodecCtx;

		fread(&iStreamIdx,1,4,fp);
		fread(&iExtraDataSize,1,4,fp);
		if (iExtraDataSize > sizeof(acExtraData))
		{
			dlog(LOG_ERROR,"extradata size %d is too long error!",iExtraDataSize);
			goto EXIT_ERROR;
		}
		fread(acExtraData,1,iExtraDataSize,fp);
		if (iStreamIdx < 0 || iStreamIdx >= MuxWtr.oc->nb_streams)
		{
			dlog(LOG_ERROR,"stream index %d error !\n",iStreamIdx);
			goto EXIT_ERROR;
		}
		if (iExtraDataSize <= 0)
			continue;
		pCodecCtx = MuxWtr.oc->streams[iStreamIdx]->codec;
		pCodecCtx->extradata = av_malloc(iExtraDataSize + AV_INPUT_BUFFER_PADDING_SIZE);
		if (!pCodecCtx->extradata)
		{
			dlog(LOG_ERROR,"av_malloc %d fail!",iExtraDataSize);
			goto EXIT_ERROR;
		}
		memcpy(pCodecCtx->extradata,acExtraData,iExtraDataSize);
		pCodecCtx->extradata_size = iExtraDataSize;
	}

	fseek(fp,FRAME_INFO_OFFSET,SEEK_SET);
	while(1)
	{
		int iOff,iRet;
		unsigned int uiNalSize = 0;
		MediaPacket pkt;


		fread(&iStreamIdx,1,4,fp);
		fread(&offset,1,8,fp);
		fread(&iFrameSize,1,4,fp);
		fread(&dts,1,8,fp);
		fread(&bKeyFrame,1,4,fp);
		iRet = fseek(fpRead,offset,SEEK_SET);
		if (iRet < 0)
		{
			dlog(LOG_ERROR,"fseek error!\n",iRet);
			break;
		}

		if (feof(fp) )
			break;
		if (feof(fpRead))
			break;
		if (iFrameSize < 0)
			break;

		if (iStreamIdx != iAudioStreamIdx)
		{
			pVideoBuf = av_malloc(iFrameSize + AV_INPUT_BUFFER_PADDING_SIZE);
			if (pVideoBuf == NULL)
			{
				dlog(LOG_ERROR,"av_malloc fail!\n");
				return -1;
			}
			iRet = fread(pVideoBuf,1,iFrameSize,fpRead);
			if (iRet != iFrameSize)
			{
				dlog(LOG_ERROR,"fread is error warning\n");
				break;
			}
			for (iOff = 0;iOff < iFrameSize;)
			{
				uiNalSize = pVideoBuf[iOff] << 24 | pVideoBuf[iOff + 1] <<16 | pVideoBuf[iOff + 2] << 8 | pVideoBuf[iOff + 3];
				if (uiNalSize >= iFrameSize)
				{
					dlog(LOG_ERROR,"video data error!\n");
					//goto EXIT_ERROR;
					break;
				}
				pVideoBuf[iOff] = pVideoBuf[iOff + 1] = pVideoBuf[iOff + 2] = 0;
				pVideoBuf[iOff + 3] = 1;
				iOff += uiNalSize + 4;
			}
			dts = dts * 1000 / STREAM_FRAME_RATE;
			pkt.pData= pVideoBuf;
			pkt.iLen = iFrameSize;
			pkt.dts = dts;
			pkt.pts = dts;
			pkt.iStreamIdx = iStreamIdx;
			pkt.bKeyFrame = bKeyFrame;
			pkt.bAudio = 0;
			MediaWriteFrame(&MuxWtr,&pkt);
			av_free(pVideoBuf);
			duration = dts;
		}
		else
		{
			pAudioBuf = av_malloc(iFrameSize + AV_INPUT_BUFFER_PADDING_SIZE);
			if (pAudioBuf == NULL)
			{
				dlog(LOG_ERROR,"av_malloc fail!\n");
				goto EXIT_ERROR;
			}
			iRet = fread(pAudioBuf,1,iFrameSize,fpRead);
			if (iRet != iFrameSize)
			{
				dlog(LOG_ERROR,"fread is error warning\n");
				break;
			}
			dts = dts * 1000/iSampleRate;
			if (last_dts == -1)
			{
				last_dts = dts;
			}
			else
			{
				if (dts <= last_dts)
				{
					dts += 1;
				}
				last_dts = dts;
			}
			pkt.pData= pAudioBuf;
			pkt.iLen = iFrameSize;
			pkt.dts = dts;
			pkt.iStreamIdx = iStreamIdx;
			pkt.bKeyFrame = bKeyFrame;
			pkt.bAudio = 1;
			MediaWriteFrame(&MuxWtr,&pkt);
			av_free(pAudioBuf);
			duration = dts;
		}	
	}
EXIT_ERROR:
	if(fp){
		fclose(fp);
	}
	if( fpRead){
		fclose(fpRead);
	}
	MediaWriteTrailer(&MuxWtr);
	MediaWriterClose(&MuxWtr);
	av_lockmgr_register(NULL);
	return duration;
}

int MediaGenIndexImage(char * InFile,char * image,int iInternal,int iStartNum,int iMaxImageNum)
{
	DemuxReader * pDmxRdr;
	MuxWriter * pMuxWtr;
	TransContext TransCtx;
	int iWidth,iHeight;
	int ret;
	int iCount = 0;
	char * pStr;
	int iStreamIdx = 0;

	MediaSysInit();
	MediaTranscodeInit(&TransCtx,InFile,image);

	iInternal = iInternal <=0 ?10000:iInternal;
	iStartNum = iStartNum < 0?0:iStartNum;
	iMaxImageNum = iMaxImageNum <= 0 ?1:iMaxImageNum;
	pStr = strchr(image,'%');
	iMaxImageNum = pStr?iMaxImageNum:1;

	pDmxRdr = &TransCtx.DemuxRdr;
	pMuxWtr = &TransCtx.MuxWtr;
	iWidth = pDmxRdr->aiWidth[pDmxRdr->iVideoStreamIdx];
	iHeight = pDmxRdr->aiHeight[pDmxRdr->iVideoStreamIdx];
	iStreamIdx = VideoStreamAdd(pMuxWtr,AV_CODEC_ID_MJPEG,iWidth,iHeight,0,1000);

	VideoDecInfoInit(&TransCtx,pDmxRdr->apVideoCodecCtx[pDmxRdr->iVideoStreamIdx]->codec_id,iWidth,iHeight,iWidth,iHeight,AV_PIX_FMT_YUVJ420P);
	VideoEncInfoInit(&TransCtx,AV_CODEC_ID_MJPEG,iWidth,iHeight,0,1000,0,0,0);
	av_opt_set_int(pMuxWtr->oc->priv_data,"start_number",iStartNum + 1,0);
	while(!pDmxRdr->bStreamEnd && iCount < iMaxImageNum)
	{
		MediaPacket pkt,outpkt;
		int iRet;

		MediaReadFrame(pDmxRdr,&pkt);
		if (pkt.iStreamIdx == pDmxRdr->iVideoStreamIdx)
		{
			VideoDeocde(&TransCtx,&pkt);
			if (TransCtx.bGetVideoDecFrame)
			{
				iRet = VideoEncode(&TransCtx,&outpkt);
				if (outpkt.iLen > 0 && iCount * iInternal <= pkt.pts)
				{
					outpkt.iStreamIdx = iStreamIdx;
					MediaWriteFrame(pMuxWtr,&outpkt);
					iCount++;
					MediaPacketFree(&outpkt);
				}
			}
		}
		MediaPacketFree(&pkt);
	}
	MediaTranscodeClose(&TransCtx);
	return 0;
}

#define DST_W 240
#define DST_H 125

void media_yuv_scaler2bgr(int h, int w, int data_len, char *srcData, char * desData)
{	
	
	struct SwsContext *img_convert_ctx = NULL;
	AVFrame	*pFrameBGR = av_frame_alloc();
	AVCodecContext  *pCodecCtx = NULL;
	AVCodec         *pCodec = NULL;
	AVFrame         *pFrame = NULL;
	int got_frame = 0;
	pCodec = avcodec_find_decoder(AV_CODEC_ID_H264);
	if (pCodec == NULL)
	{		
		printf("Unsupported codec!\n");
		goto EXIT0;
	}

	pCodecCtx = avcodec_alloc_context3(pCodec);
	if (!pCodecCtx)
	{	
		printf("无法分配视频解码器上下文。");
		goto EXIT0;
	}

	if (avcodec_open2(pCodecCtx, pCodec, NULL) < 0)
	{	
		printf("Cannot open video decoder\n");
		goto EXIT1;
	}

	pFrame = avcodec_alloc_frame();
	if (pFrame == NULL)
	{
		printf("it is err av_frame_alloc!!!!!!\n");
		goto EXIT1;
	}

	int ret;
	AVPacket* packet = (AVPacket *)malloc(sizeof(AVPacket));
	//packet->data = (unsigned char *)malloc(data_len);
	av_init_packet(packet);	
	packet->data = (uint8_t *)srcData;	
	packet->size = data_len;		
	ret = avcodec_decode_video2(pCodecCtx, pFrame, &got_frame, packet);	
	if (ret < 0)		
	{			
		printf("it is err avcodec_decode_video2!!!!\n");			
		goto EXIT2;
	}

	int initsize = av_image_get_buffer_size(AV_PIX_FMT_BGR24, pCodecCtx->width, pCodecCtx->height, 16);
	uint8_t *buffer = (uint8_t*)malloc(initsize*sizeof(uint8_t));
	memset(buffer, 0, initsize*sizeof(uint8_t));

	if (NULL == img_convert_ctx)		
	{			
		img_convert_ctx = sws_getContext(pCodecCtx->width, pCodecCtx->height, pCodecCtx->pix_fmt, w, h, AV_PIX_FMT_BGR24, SWS_BICUBIC, NULL, NULL, NULL);			
		av_image_fill_arrays(pFrameBGR->data, pFrameBGR->linesize, buffer, AV_PIX_FMT_BGR24, pCodecCtx->width, pCodecCtx->height, 16);
	}
	
	sws_scale(img_convert_ctx, (const uint8_t *const*)pFrame->data, pFrame->linesize, 0, pCodecCtx->height, pFrameBGR->data, pFrameBGR->linesize);


	int j = 0;
	int i = 0;
	for (i = h - 1, j = 0; i > -1 && j < h; i--, j++)
	{			
		memcpy(desData + w * 3 * j, pFrameBGR->linesize[0] * i + pFrameBGR->data[0], w * 3);
	}

EXIT3:
	if (NULL != buffer)
	{
		free(buffer);
		buffer = NULL;
	}

EXIT2:

	if (NULL != pFrame)
	{
		avcodec_free_frame(&pFrame);
		pFrame = NULL;
	}

	if (NULL != packet)
	{
		free(packet);
		packet = NULL;
	}

	if (NULL != buffer)
	{
		free(buffer);
		buffer = NULL;
	}
	
EXIT1:
	
	if (NULL != pCodecCtx)
	{
		//avcodec_close(pCodecCtx);
		//av_free(pCodecCtx);
	}
	
EXIT0:
	if (NULL != pFrameBGR)
	{
		av_frame_free(&pFrameBGR);
		pFrameBGR = NULL;
	}
	return;
}
#endif
