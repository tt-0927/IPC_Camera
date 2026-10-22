

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "os.h"
#include "media_ffmpeg.h"


static int media_interrupt_callback(void *argv)
{
	mediaFfmpeg_t* handle = (mediaFfmpeg_t*)argv;

	if(handle->startTime > 0)
	{
		int timeCal = OS_getSysTimeInMsec() - handle->startTime;
		if(timeCal > 60000)	//假设超时时间过短，触发下面的逻辑，则会导致ffmpeg get得到的数据会跳帧
		{
			handle->startTime = 0; //将时间置为0
			OS_printf_log(handle->log,"<player> no data time out[%d]!! start[%u] now[%u]\n",\
					timeCal,handle->startTime,OS_getSysTimeInMsec());
			return 1;
		}
	}

	return 0;
}

static void mediaUpdateClockVideo(mediaFfmpeg_t* handle,double pts)
{
	media_set_clock(&(handle->clock.viclk),pts);
	media_sync_clock_to_slave(&(handle->clock.extclk),&(handle->clock.viclk));
}

static void mediaUpdateClockAudio(mediaFfmpeg_t* handle,double pts)
{
	media_set_clock(&(handle->clock.auclk),pts);
	media_sync_clock_to_slave(&(handle->clock.extclk),&(handle->clock.auclk));
}

static int media_init_param(mediaFfmpeg_t* handle,char* url)
{
    memset(&(handle->demuxParam),0,sizeof(mediaParam_S));
    handle->demuxParam.track = handle->formatCtx->nb_streams;
    handle->demuxParam.duration = handle->formatCtx->duration;
    snprintf(handle->demuxParam.url,\
    		sizeof(handle->demuxParam.url),"%s",url);

    int i = 0;
    for(i = 0; i < handle->formatCtx->nb_streams; i++)
    {
        if(handle->formatCtx->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO)
        {
            if(handle->demuxParam.ntrack[MEDIA_TYPE_VIDEO] != 1)
            {
                //video
            	handle->demuxParam.time_base[MEDIA_TYPE_VIDEO].num = \
            			handle->formatCtx->streams[i]->time_base.num;
            	handle->demuxParam.time_base[MEDIA_TYPE_VIDEO].den = \
            			handle->formatCtx->streams[i]->time_base.den;

            	handle->demuxParam.ntrack[MEDIA_TYPE_VIDEO] = 1;
                handle->demuxParam.width = handle->pVideoCodecCtx->width;
                handle->demuxParam.height = handle->pVideoCodecCtx->height;

                if(handle->formatCtx->streams[i]->avg_frame_rate.den && \
                		handle->formatCtx->streams[i]->avg_frame_rate.num)
                {
                	handle->demuxParam.frameRate = av_q2d(\
                			handle->formatCtx->streams[i]->avg_frame_rate);

                }else if(handle->formatCtx->streams[i]->r_frame_rate.den && \
                		handle->formatCtx->streams[i]->r_frame_rate.num)
                {
                	handle->demuxParam.frameRate = av_q2d(\
                			handle->formatCtx->streams[i]->r_frame_rate);
                }else
                {
                	handle->demuxParam.frameRate = 25;
                    printf("media_open_url fps fail ,"
                    		"so handle->videofps = 25. name[%s]",\
                    		url);
                }
            }
        }
        else if(handle->formatCtx->streams[i]->codec->codec_type == AVMEDIA_TYPE_AUDIO)
        {
            if(handle->demuxParam.ntrack[MEDIA_TYPE_AUDIO] != 1)
            {
            	handle->demuxParam.sampleRate = handle->pAudioCodecCtx->sample_rate;
            	handle->demuxParam.channel = handle->pAudioCodecCtx->channels;
            	handle->demuxParam.time_base[MEDIA_TYPE_AUDIO].num = \
            			handle->formatCtx->streams[i]->time_base.num;
            	handle->demuxParam.time_base[MEDIA_TYPE_AUDIO].den = \
            			handle->formatCtx->streams[i]->time_base.den;
            	handle->demuxParam.ntrack[MEDIA_TYPE_AUDIO] = 1;
            }
        }
        else if(handle->formatCtx->streams[i]->codec->codec_type == AVMEDIA_TYPE_SUBTITLE)
        {
            if(handle->demuxParam.ntrack[MEDIA_TYPE_SUBTITLE] != 1)
            {
            	handle->demuxParam.time_base[MEDIA_TYPE_SUBTITLE].num = \
            			handle->formatCtx->streams[i]->time_base.num;
            	handle->demuxParam.time_base[MEDIA_TYPE_SUBTITLE].den = \
            			handle->formatCtx->streams[i]->time_base.den;
            	handle->demuxParam.ntrack[MEDIA_TYPE_SUBTITLE] = 1;
            }
        }else
        {
            continue;
        }
    }

	return 0;
}




mediaFfmpeg_t* media_open_url(char* url,long long startTime,OS_log log,void* user,int (*interrupt_callback)(void*))
{
	mediaFfmpeg_t* handle = (mediaFfmpeg_t*)malloc(sizeof(mediaFfmpeg_t));
	if(handle == NULL)
	{
		printf("malloc error!!\n");
		return NULL;
	}

	memset(handle,0,sizeof(mediaFfmpeg_t));
	int ret = 0;
	AVDictionary *format_opts = NULL;

	handle->user = user;
	handle->log = log;
    handle->audio_stream = handle->video_stream = -1;
	handle->streamType = STREAM_DEMUX_TYPE_VIDEO;
	handle->formatCtx = avformat_alloc_context();
	if (!handle->formatCtx)
	{
		OS_printf_log(handle->log,"Could not allocate context.\n");
		free(handle);
		exit(0);
		return NULL;
	}

	//超时机制
	if(interrupt_callback)
	{
		handle->formatCtx->interrupt_callback.callback = interrupt_callback;
		handle->formatCtx->interrupt_callback.opaque = user;
	}else
	{
		handle->formatCtx->interrupt_callback.callback = media_interrupt_callback;
		handle->formatCtx->interrupt_callback.opaque = handle;
	}

	//set timeout
	av_dict_set(&format_opts, "rtsp_transport", "tcp", 0);
	av_dict_set(&format_opts, "stimeout", "3000000", 0);	//3s超时
	av_dict_set(&format_opts, "thread_queue_size", "128", 0);
	av_dict_set(&format_opts, "buffer_size", "1024000", 0);  //设置udp的接收缓冲

	//超时开始
	handle->startTime = OS_getSysTimeInMsec();
	ret = avformat_open_input(&handle->formatCtx, url, handle->formatCtx->iformat, &format_opts);
	if (ret < 0)
	{
		char error[512] = {0};
		av_strerror(ret,error,sizeof(error));
		OS_printf_log(handle->log,"avformat_open_input error[%d][%s],url:%s\n",ret,error,url);
		media_close_url(handle);
		return NULL;
	}
	handle->startTime = 0;

	if( avformat_find_stream_info(handle->formatCtx, NULL ) < 0 )
	{
		OS_printf_log(handle->log,"Cannot avformat_find_stream_info\n");
		media_close_url(handle);
		return NULL;
	}
	av_dump_format(handle->formatCtx, -1, url, 0);

	/* if seeking requested, we execute it */
	if (startTime > 0)
	{
		int64_t timestamp = 0;
		timestamp = (int64_t)(startTime * AV_TIME_BASE);
		/* add the stream start time */
		if (handle->formatCtx->start_time != AV_NOPTS_VALUE)
		{
			timestamp += handle->formatCtx->start_time;
		}
		OS_printf_log(handle->log,"seek time[%lld] [%lld]\n\n\n\n\n",timestamp,startTime);
		ret = avformat_seek_file(handle->formatCtx, -1, INT64_MIN, timestamp, INT64_MAX, 0);
		if (ret < 0)
		{
			OS_printf_log(handle->log,"%s: could not seek to position %0.3f\n",
					url, (double)timestamp / AV_TIME_BASE);
		}
	}

	int i = 0;
	AVCodecContext* pCodecCtx = NULL;
	AVCodec* pCodec = NULL;
	int nFps,nTbr;
    for (i = 0; i < handle->formatCtx->nb_streams; i++)
    {
        pCodecCtx = handle->formatCtx->streams[i]->codec;
		if( handle->formatCtx->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO)
		{
			//只要h264 h265
			if(pCodecCtx->codec_id == AV_CODEC_ID_H264 || (pCodecCtx->codec_id == AV_CODEC_ID_HEVC))
			{
				AVStream *stream = handle->formatCtx->streams[i];
				if(stream)
				{
					handle->videofps = av_q2d(stream->avg_frame_rate);
					//handle->videofps = (double)stream->avg_frame_rate.num/(double)stream->avg_frame_rate.den;
					nFps = stream->avg_frame_rate.den && stream->avg_frame_rate.num;
					if(!nFps)
					{
						printf("fhandle->videofps == 0\n");
						handle->videofps = av_q2d(stream->r_frame_rate);
						nTbr = stream->r_frame_rate.den && stream->r_frame_rate.num;
						if(!nTbr)
						{
							handle->videofps = 25;
							OS_printf_log(handle->log,"media_open_url fps fail ,so handle->videofps = 25\n");

						}
					}

				}
				handle->pVideoCodecCtx = pCodecCtx;
				handle->video_stream = i;
			}else
			{
				OS_printf_log(handle->log," this code is[%d] is not support!!!!\n",pCodecCtx->codec_id);
				continue;
			}
		}
		if( handle->formatCtx->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO)
		{
			handle->pVideoCodecCtx = pCodecCtx;
			handle->video_stream = i;
		}
		else if(handle->formatCtx->streams[i]->codec->codec_type == AVMEDIA_TYPE_AUDIO)
		{
			handle->pAudioCodecCtx = pCodecCtx;
			handle->audio_stream = i;

			handle->pAudioParser = av_parser_init(pCodecCtx->codec_id);
		    if (!handle->pAudioParser) {
		        printf("Parser not found codec_id[%d]\n",pCodecCtx->codec_id);
		    }
		}
		else if(handle->formatCtx->streams[i]->codec->codec_type == AVMEDIA_TYPE_SUBTITLE)
		{
			handle->subtitle_stream = i;
		}else
		{
			continue;
		}

		pCodec = avcodec_find_decoder(pCodecCtx->codec_id);
		if( pCodec == NULL )
		{
			OS_printf_log(handle->log,"Cannot avcodec_find_decoder\n");
			media_close_url(handle);
			return NULL;
		}
		if( avcodec_open2(pCodecCtx, pCodec, NULL) < 0)
		{
			OS_printf_log(handle->log,"Cannot avcodec_open2\n");
			media_close_url(handle);
			return NULL;
		}

        //printf("get file codec_id[%d]\n",pCodecCtx->codec_id);

		if((pCodecCtx->codec_id == AV_CODEC_ID_H264))
		{
			AVBitStreamFilterContext* bsfc = av_bitstream_filter_init("h264_mp4toannexb");
			if(!bsfc)
			{
				media_close_url(handle);
				OS_printf_log(handle->log,"Cannot init bsfc h264_mp4toannexb!!\n");
				return NULL;
			}
			handle->videoBsfc = bsfc;
		}
		else if((pCodecCtx->codec_id == AV_CODEC_ID_HEVC))
		{
			AVBitStreamFilterContext* bsfc = av_bitstream_filter_init("hevc_mp4toannexb");
			if(!bsfc)
			{
				media_close_url(handle);
				OS_printf_log(handle->log,"Cannot init bsfc h264_mp4toannexb!!\n");
				return NULL;
			}
			handle->videoBsfc = bsfc;
		}
		else if((pCodecCtx->codec_id == AV_CODEC_ID_AAC))
		{
			AVBitStreamFilterContext* bsfc = av_bitstream_filter_init("aac_adtstoasc");
			if(!bsfc)
			{
				media_close_url(handle);
				OS_printf_log(handle->log,"Cannot init bsfc aac_adtstoasc!!\n");
				return NULL;
			}
			handle->audioBsfc = bsfc;
		}
    }

	if (handle->audio_stream == -1 && handle->video_stream != -1)	{
		handle->streamType = STREAM_DEMUX_TYPE_VIDEO;
	}
    if (handle->audio_stream != -1 && handle->video_stream != -1) {
		handle->streamType = STREAM_DEMUX_TYPE_COMPOSITE;
    }
    /* 初始化时钟 */
    media_init_clock(&(handle->clock.auclk));
    media_init_clock(&(handle->clock.viclk));
    media_init_clock(&(handle->clock.extclk));
    handle->clock.syn_master_clock = CLOCK_SYNC_VIDEO_MASTER;

    /* 初始化参数 */
    media_init_param(handle,url);

	return handle;
}


int media_close_url(mediaFfmpeg_t* handle)
{
	if(handle == NULL)
	{
		printf("this argument is null!!\n");
		return -1;
	}

	if(handle->videoBsfc)
	{
		av_bitstream_filter_close(handle->videoBsfc);
		handle->videoBsfc = NULL;
	}
	if(handle->audioBsfc)
	{
		av_bitstream_filter_close(handle->audioBsfc);
		handle->audioBsfc = NULL;
	}
	if(handle->pVideoCodecCtx)
	{
		avcodec_close(handle->pVideoCodecCtx);
		handle->pVideoCodecCtx = NULL;
	}
	if(handle->pAudioCodecCtx)
	{
	    avcodec_close(handle->pAudioCodecCtx);
	    handle->pAudioCodecCtx = NULL;
	}
    if (handle->formatCtx)
    {
        avformat_close_input(&handle->formatCtx);
        handle->formatCtx = NULL;
    }

    if(handle->pAudioDecFrame)
    {
		av_frame_unref(handle->pAudioDecFrame);
		av_frame_free(&handle->pAudioDecFrame);
		handle->pAudioDecFrame = NULL;
    }

    if(handle->pAudioResampleFrame)
    {
		av_frame_unref(handle->pAudioResampleFrame);
		av_frame_free(&handle->pAudioResampleFrame);
		handle->pAudioResampleFrame = NULL;
    }

    if(handle->swr_ctx)
    {
    	swr_free(&handle->swr_ctx);
    	handle->swr_ctx = NULL;
    }


    free(handle);
    handle = NULL;

	return 0;
}



static int adts_sample_rates[] = {96000,88200,64000,48000,44100,32000,24000,22050,16000,12000,11025,8000,7350,0,0,0};

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
static unsigned char *parse_makeAdtsHeader(int *dataSize,int samplerate, int channels,int iFrameLen , unsigned char *data)
{
	int profile = 1;//aac lc
	int sr_index = FindAdtsSRIndex(samplerate);
	int skip = 7;
	int framesize = skip + iFrameLen;

	*dataSize = 7;

	//data = (unsigned char *)malloc(*dataSize * sizeof(unsigned char));
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

	return 0;
}


/*
 * 获取数据帧
 * return:0-success,-1-fail,-2-读取文件结束,-3-数据包不需要
 * */
int media_get_frame(mediaFfmpeg_t* handle,mediaPacket_t* paket)
{
	if(handle == NULL)
	{
		printf("this argument is null!!\n");
		return -1;
	}
	int ret = 0;
	AVPacket pkt1,*pkt = &pkt1;
	unsigned int startTime = 0;

	startTime = handle->startTime = OS_getSysTimeInMsec();
	ret = av_read_frame(handle->formatCtx, pkt);
	handle->startTime = 0;

	if((OS_getSysTimeInMsec() - startTime) > 5000)
	{
		OS_printf_log(handle->log,"<player> ffmpeg get data time so long [%d]ms ret[%d]\n",\
				(OS_getSysTimeInMsec() - startTime),ret);
	}

	if(ret < 0)
	{
		char error[512] = {0};
		av_strerror(ret,error,sizeof(error));
		OS_printf_log(handle->log,"av_read_frame error[%d][%s]!!\n",ret,error);
		if ((ret == AVERROR_EOF || avio_feof(handle->formatCtx->pb)))
		{
			return -2;
		}
		return -1;
	}
    paket->codec_id = handle->formatCtx->streams[pkt->stream_index]->codec->codec_id;
    paket->size = pkt1.size;
    paket->dts = pkt1.dts;
    paket->pts = pkt1.pts;
    paket->duration = pkt1.duration;
    paket->pos = pkt1.pos;

    double timebase = 0.0;
	double avdts = 0.0;
	AVRational avtimebase;


    if (pkt->stream_index == handle->audio_stream )
    {
    	/* 计算一下时间戳 */
    	avtimebase.num = handle->demuxParam.time_base[MEDIA_TYPE_AUDIO].num;
    	avtimebase.den = handle->demuxParam.time_base[MEDIA_TYPE_AUDIO].den;
    	timebase = av_q2d(avtimebase);
    	avdts = pkt->dts * timebase;
    	mediaUpdateClockAudio(handle,avdts);

        paket->type = MEDIA_TYPE_AUDIO;
    	if(handle->formatCtx->streams[pkt->stream_index]->codec->codec_id == AV_CODEC_ID_AAC)
    	{
    		if((pkt1.data[0] == 0xFF) && ((pkt1.data[1] & 0xF0)== 0xF0))
			{
                paket->data = malloc(paket->size);//av_malloc(paket->size);
    			memcpy(paket->data,pkt1.data,paket->size);

			}
			else
			{
				int adtssize = 7;
				paket->size = pkt1.size + adtssize;
                paket->data = malloc(paket->size);//av_malloc(paket->size);
				if(paket->data == NULL)
				{
                    OS_printf_log(handle->log,"malloc (parse_handle->fpkt).data fail\n");
					ret = -1;
					goto EXIT;
				}
				parse_makeAdtsHeader(&adtssize,handle->pAudioCodecCtx->sample_rate,
						handle->pAudioCodecCtx->channels , pkt1.size, paket->data);
				memcpy(&(paket->data[adtssize]), pkt1.data,pkt1.size);
			}
    	}
        else
        {
            paket->data = malloc(paket->size);//av_malloc(paket->size);
            memcpy(paket->data,pkt1.data,paket->size);
        }

    }
    else if (pkt->stream_index == handle->video_stream)
    {
    	/* 计算一下时间戳 */
    	avtimebase.num = handle->demuxParam.time_base[MEDIA_TYPE_VIDEO].num;
    	avtimebase.den = handle->demuxParam.time_base[MEDIA_TYPE_VIDEO].den;
    	timebase = av_q2d(avtimebase);
    	avdts = pkt->dts * timebase;
    	mediaUpdateClockVideo(handle,avdts);

        paket->type = MEDIA_TYPE_VIDEO;
        AVPacket pkt2;
        if(handle->formatCtx->streams[pkt->stream_index]->codec->codec_id == AV_CODEC_ID_H264 ||
                handle->formatCtx->streams[pkt->stream_index]->codec->codec_id == AV_CODEC_ID_H265)
        {
            if(((pkt1.data[0] == 0x00) && (pkt1.data[1] == 0x00) && (pkt1.data[2] == 0x00) && (pkt1.data[3] == 0x01)))
            //        || ((pkt1.data[0] == 0x00) && (pkt1.data[1] == 0x00) && (pkt1.data[2] == 0x01)))
           {
                paket->data = malloc(paket->size);//av_malloc(paket->size);
                memcpy(paket->data,pkt1.data,paket->size);
           }
           else
           {
                ret = av_bitstream_filter_filter(handle->videoBsfc, handle->pVideoCodecCtx, NULL,\
                        &(pkt2.data), &(pkt2.size),pkt1.data, pkt1.size,0);
                if (ret < 0)
                {
                    ret = -3;
                    goto EXIT;
                }

                paket->size = pkt2.size;
                paket->data = malloc(paket->size);//av_malloc(paket->size);
                memcpy(paket->data,pkt2.data,paket->size);
                av_free(pkt2.data);
           }
        }
        else
        {
            paket->data = malloc(paket->size);//av_malloc(paket->size);
            memcpy(paket->data,pkt1.data,paket->size);
        }
		paket->isKeyFrame = (pkt1.flags & AV_PKT_FLAG_KEY);

    } else if (pkt->stream_index == handle->subtitle_stream)
    {
    	ret = -3;
    	goto EXIT;

    } else {
    	ret = -3;
    	goto EXIT;
    }

EXIT:
    av_packet_unref(pkt);

	return ret;
}



int media_unpaket(mediaFfmpeg_t* handle,mediaPacket_t* pkt)
{
	if((handle == NULL) || (pkt == NULL))
	{
		printf("this argument is null!!!\n");
		return -1;
	}

	if(pkt->data)
	{
        free(pkt->data);//av_free(pkt->data);
		pkt->data = NULL;
	}
	return 0;
}


static void media_packet_free(void* user,void *data)
{
	unsigned char* pBuff = (unsigned char*)user;
	if(pBuff)
	{
		free(pBuff);
		pBuff = NULL;
	}
}

int media_get_packetRef(mediaFfmpeg_t* handle,AVMediaPacket_S* paket)
{
	if(handle == NULL)
	{
		printf("this argument is null!!\n");
		return -1;
	}
	int ret = 0;
	AVPacket pkt1,*pkt = &pkt1;

	memset(pkt, 0, sizeof(AVPacket));
	unsigned int startTime = 0;

	startTime = handle->startTime = OS_getSysTimeInMsec();
	ret = av_read_frame(handle->formatCtx, pkt);
	handle->startTime = 0;

	if((OS_getSysTimeInMsec() - startTime) > 5000)
	{
		OS_printf_log(handle->log,"<player> ffmpeg get data time so long [%d]ms ret[%d]\n",\
				(OS_getSysTimeInMsec() - startTime),ret);
	}
	if(ret < 0)
	{
		char error[512] = {0};
		av_strerror(ret,error,sizeof(error));
		OS_printf_log(handle->log,"av_read_frame error[%d][%s]!!\n",ret,error);
		if ((ret == AVERROR_EOF || avio_feof(handle->formatCtx->pb)))
		{
			return -2;
		}
		return -1;
	}

	PACKET_TYPE_E type;
	int format = (int)handle->formatCtx->streams[pkt->stream_index]->codec->codec_id;
	unsigned char* pData = NULL;
	int nSize = pkt1.size;
	int isKeyFrame = 0;
    int width = 0;
    int height = 0;
    double frameRate = 0.0;
    int bitrate = 0;
    int audioSampleRate = 0;
    int64_t audioBitRate = 0;

    double timebase = 0.0;
	double avdts = 0.0;
	AVRational avtimebase;

    if (pkt->stream_index == handle->audio_stream )
    {
    	/* 计算一下时间戳 */
    	avtimebase.num = handle->demuxParam.time_base[MEDIA_TYPE_AUDIO].num;
    	avtimebase.den = handle->demuxParam.time_base[MEDIA_TYPE_AUDIO].den;
    	timebase = av_q2d(avtimebase);
    	avdts = pkt->dts * timebase;
    	mediaUpdateClockAudio(handle,avdts);

    	type = PACKET_TYPE_AUDIO;
    	if(handle->formatCtx->streams[pkt->stream_index]->codec->codec_id == AV_CODEC_ID_AAC)
    	{
    		if((pkt1.data[0] == 0xFF) && ((pkt1.data[1] & 0xF0)== 0xF0))
			{
    			pData = malloc(nSize);
    			memcpy(pData,pkt1.data,nSize);
			}
			else
			{
				int adtssize = 7;
				nSize = pkt1.size + adtssize;
				pData = malloc(nSize);
				if(pData == NULL)
				{
                    OS_printf_log(handle->log,"malloc (parse_handle->fpkt).data fail\n");
					ret = -1;
					goto EXIT;
				}

				parse_makeAdtsHeader(&adtssize,handle->pAudioCodecCtx->sample_rate,
						handle->pAudioCodecCtx->channels , pkt1.size, pData);
				memcpy(&(pData[adtssize]), pkt1.data,pkt1.size);
			}
    	}
        else
        {
        	pData = malloc(nSize);
            memcpy(pData,pkt1.data,nSize);
        }
        audioSampleRate = handle->pAudioCodecCtx->sample_rate;
        audioBitRate = handle->pAudioCodecCtx->bit_rate;
    }
    else if (pkt->stream_index == handle->video_stream)
    {
    	/* 计算一下时间戳 */
    	avtimebase.num = handle->demuxParam.time_base[MEDIA_TYPE_VIDEO].num;
    	avtimebase.den = handle->demuxParam.time_base[MEDIA_TYPE_VIDEO].den;
    	timebase = av_q2d(avtimebase);
    	avdts = pkt->dts * timebase;
    	mediaUpdateClockVideo(handle,avdts);

    	type = PACKET_TYPE_VIDEO;
        AVPacket pkt2;
        if(handle->formatCtx->streams[pkt->stream_index]->codec->codec_id == AV_CODEC_ID_H264 ||
                handle->formatCtx->streams[pkt->stream_index]->codec->codec_id == AV_CODEC_ID_H265)
        {
            if(((pkt1.data[0] == 0x00) && (pkt1.data[1] == 0x00) && (pkt1.data[2] == 0x00) && (pkt1.data[3] == 0x01)))
            //        || ((pkt1.data[0] == 0x00) && (pkt1.data[1] == 0x00) && (pkt1.data[2] == 0x01)))
           {
            	pData = malloc(nSize);
                memcpy(pData,pkt1.data,nSize);
           }
           else
           {
                ret = av_bitstream_filter_filter(handle->videoBsfc, handle->pVideoCodecCtx, NULL,\
                        &(pkt2.data), &(pkt2.size),pkt1.data, pkt1.size,0);
                if (ret < 0)
                {
                    ret = -3;
                    goto EXIT;
                }

                ret = 0;
                nSize = pkt2.size;
                pData = malloc(nSize);
                memcpy(pData,pkt2.data,nSize);
                av_free(pkt2.data);
           }
        }
        else
        {
        	pData = malloc(nSize);
            memcpy(pData,pkt1.data,nSize);
        }

        width = handle->pVideoCodecCtx->width;
        height = handle->pVideoCodecCtx->height;
        frameRate = av_q2d(handle->pVideoCodecCtx->framerate);
        bitrate = handle->pVideoCodecCtx->bit_rate;
        isKeyFrame = (pkt1.flags & AV_PKT_FLAG_KEY);

    } else if (pkt->stream_index == handle->subtitle_stream)
    {
    	ret = -3;
    	goto EXIT;

    } else {
    	ret = -4;
    	goto EXIT;
    }

    if(ret == 0)
    {
    	ret = avMedia_Packet_create(paket,pData,nSize,isKeyFrame,\
    			width,height,type,(PACKET_FORMAT_E)format,frameRate,bitrate,\
				audioSampleRate,audioBitRate,pkt1.pts,pkt1.dts, 0,\
				media_packet_free,pData);
        paket->nDuration = pkt1.duration;
    }

EXIT:
    av_packet_unref(pkt);

	return ret;
}


int media_get_demuxInfo(mediaFfmpeg_t* handle,\
		mediaParam_S* demuxParam)
{
	if((handle == NULL) || (demuxParam == NULL))
	{
		printf("this argument is null!!\n");
		return -1;
	}
	memcpy(demuxParam,&(handle->demuxParam),\
			sizeof(mediaParam_S));
	return 0;
}

/* 获取当前播放的时间 */
double media_get_time(mediaFfmpeg_t* handle)
{
	if((handle == NULL))
	{
		printf("this argument is null!!\n");
		return -1;
	}

    double val = 0.0;
    switch (handle->clock.syn_master_clock)
    {
        case CLOCK_SYNC_VIDEO_MASTER:
            val = media_get_clock(&handle->clock.viclk);
            break;
        case CLOCK_SYNC_AUDIO_MASTER:
            val = media_get_clock(&handle->clock.auclk);
            break;
        default:
            val = media_get_clock(&handle->clock.extclk);
            break;
    }
	return val;
}


int media_seek(mediaFfmpeg_t* handle,double seek_pos)
{
	if(handle == NULL)
	{
		printf("this argument is null!!\n");
		return -1;
	}

	if(handle->formatCtx == NULL)
	{
		printf("this seek formatCtx is null!!!\n");
		return -1;
	}

	int ret = 0;
	int seek_flags = 0;
	double seek_rel = 0;
	int64_t pos = (int64_t)(seek_pos * AV_TIME_BASE);
	int64_t rel = (int64_t)(seek_rel * AV_TIME_BASE);

	int64_t seek_target = pos;
	int64_t seek_min    = rel > 0 ? seek_target - rel + 2: INT64_MIN;
	int64_t seek_max    = rel < 0 ? seek_target - rel - 2: INT64_MAX;
	seek_flags &= ~AVSEEK_FLAG_BYTE;

	if (handle->formatCtx->start_time != AV_NOPTS_VALUE)
	{
		seek_target += handle->formatCtx->start_time;
	}
	ret = avformat_seek_file(handle->formatCtx, -1, \
			seek_min, seek_target, seek_max, seek_flags);
	if (ret < 0)
	{
		char error[512] = {0};
		av_strerror(ret,error,sizeof(error));
		return -1;
	}

	return 0;
}



void media_writeAudioFile(const AVFrame *frame, const char *pPath)
{
    FILE *file = NULL;
    file = fopen(pPath, "ab+");
    if (NULL == file){
      printf("fopen tmp.mp3 error\n");
      return;
    }

    int data_size = av_get_bytes_per_sample(\
    		(enum AVSampleFormat )frame->format);
    if (data_size < 0) {
    	printf("Failed to calculate data size\n");
        return ;
    }

    int i=0;
    int ch = 0;
    for (i=0; i<frame->nb_samples; i++)
        for (ch=0; ch<frame->channels; ch++)
            fwrite(frame->data[ch] + data_size*i, 1, data_size, file);

    fclose(file);
    file = NULL;
}

AVFrame* alloc_audio_frame(int nSampleRate, int nbSample, \
		uint64_t nChnLayout, enum AVSampleFormat eSampleFmt)
{
    int ret;
    AVFrame* pFrame = av_frame_alloc();
    if (pFrame == NULL)
    {
        printf("Could not allocate audio frame\n");
        return NULL;
    }
    pFrame->nb_samples = nbSample;//1024 AAC的长度，格式不一样，值也不一样
    pFrame->format         = eSampleFmt;
    pFrame->channel_layout = nChnLayout;
    pFrame->channels =  av_get_channel_layout_nb_channels(nChnLayout);
    pFrame->sample_rate = nSampleRate;
    /* allocate the data buffers */
    ret = av_frame_get_buffer(pFrame, 0);
    if (ret < 0)
    {
    	printf("Could not allocate audio data buffers\n");
        av_frame_free(&pFrame);
        return NULL;
    }

    return pFrame;
}

int media_resample_audio(mediaFfmpeg_t* handle,uint8_t **data,int nInlayout,int nInsampleRate,int nInnb_samples,int nInformat,\
		int layout,int sampleRate,enum AVSampleFormat format)
{
	int ret = 0;
	if(handle->swr_ctx == NULL)
	{
		/* create resampler context */
		handle->swr_ctx = swr_alloc();
		if (!handle->swr_ctx) {
			printf("Could not allocate resampler context\n");
			ret = AVERROR(ENOMEM);
			return -1;
		}
		printf("handle->swr_ctx == NULL\n");
	}


	SwrContext *swr_ctx = handle->swr_ctx;
	if((handle->audioResampleFormat != format) || \
			(handle->audioResampleLayout != layout) || \
			(handle->audioResampleSampleRate != sampleRate))
	{
		handle->audioResampleFormat = format;
		handle->audioResampleLayout = layout;
		handle->audioResampleSampleRate = sampleRate;

		/* set options */
		av_opt_set_int(swr_ctx, "in_channel_layout",    nInlayout, 0);
		av_opt_set_int(swr_ctx, "in_sample_rate",       nInsampleRate, 0);
		av_opt_set_sample_fmt(swr_ctx, "in_sample_fmt", nInformat, 0);

		av_opt_set_int(swr_ctx, "out_channel_layout",    handle->audioResampleLayout, 0);
		av_opt_set_int(swr_ctx, "out_sample_rate",       handle->audioResampleSampleRate, 0);
		av_opt_set_sample_fmt(swr_ctx, "out_sample_fmt", handle->audioResampleFormat, 0);

		/* initialize the resampling context */
		if ((ret = swr_init(swr_ctx)) < 0) {
			printf("Failed to initialize the resampling context\n");
			return -1;
		}
	}

    int dstNbSamplesTotal = av_rescale_rnd(\
    		swr_get_delay(swr_ctx, nInsampleRate) + \
			nInnb_samples, handle->audioResampleSampleRate, \
			nInsampleRate, AV_ROUND_UP);

    //判断实际输出采样点是否小于分配的输出采样点
    if(dstNbSamplesTotal > handle->audioMaxDstNbSamples)
    {
    	if(handle->pAudioResampleFrame)
    	{
			av_frame_unref(handle->pAudioResampleFrame);
			av_frame_free(&handle->pAudioResampleFrame);
    	}
    	handle->pAudioResampleFrame =  alloc_audio_frame(\
    			handle->audioResampleSampleRate, dstNbSamplesTotal,
				handle->audioResampleLayout, handle->audioResampleFormat);
        if(handle->pAudioResampleFrame == NULL)
        {
            printf("alloc_audio_frame error!!!");
            return -1;
        }
        handle->pAudioResampleFrame->nb_samples = dstNbSamplesTotal;
        handle->audioMaxDstNbSamples = dstNbSamplesTotal;
    }
    handle->pAudioResampleFrame->nb_samples = dstNbSamplesTotal;

    ret = swr_convert(swr_ctx, handle->pAudioResampleFrame->data, \
    	dstNbSamplesTotal, (const uint8_t **)(data), nInnb_samples);
    if(ret < 0)
    {
        printf("swr_convert error");
        return -1;
    }

	int dst_nb_channels = av_get_channel_layout_nb_channels ( handle->audioResampleLayout );
	int	resampled_data_size = av_samples_get_buffer_size(NULL, dst_nb_channels,
				ret, (enum AVSampleFormat)handle->audioResampleFormat, 1);
	/* 上抛数据 */
	if(handle->audioDecSink)
	{
        handle->pAudioResampleFrame->linesize[0] = resampled_data_size;
		handle->audioDecSink(handle->pAudioResampleFrame,handle->audioDecUser);
	}
	//media_writeAudioFile(handle->pAudioResampleFrame,"./aaa.pcm");

	return 0;
}


int media_setDecoderSink(mediaFfmpeg_t* handle,decCallback sink,void *user)
{
	if(handle == NULL)
	{
		printf("this hanlde is null!!!\n");
		return -1;
	}
	handle->audioDecSink = sink;
	handle->audioDecUser = user;
	return 0;
}


int media_decode_audio(mediaFfmpeg_t* handle,mediaPacket_t* pMediaPkt,\
		int isResample,\
		int layout,int sampleRate,enum AVSampleFormat format)
{
	int bGetFrame = 0;
	AVCodecContext * pAVCodecCtx = handle->pAudioCodecCtx;
	AVPacket pkt;
	int ret;

	if (!handle || !pMediaPkt)
		return -1;

	av_init_packet(&pkt);

	ret = av_parser_parse2(handle->pAudioParser, \
		handle->pAudioCodecCtx, &pkt.data, &pkt.size,
		pMediaPkt->data, pMediaPkt->size,
		AV_NOPTS_VALUE, AV_NOPTS_VALUE, 0);
	if (ret < 0) {
		printf("Error while parsing\n");
		return -1;
	}

	/* send the packet with the compressed data to the decoder */
	ret = avcodec_send_packet(handle->pAudioCodecCtx, &pkt);
	if (ret < 0) {
		printf("Error submitting the packet to the decoder\n");
		return -1;
	}

	if(!handle->pAudioDecFrame)
	{
		if (!(handle->pAudioDecFrame = av_frame_alloc()))
		{
			printf("Could not allocate audio frame\n");
			return -1;
		}
	}

	AVFrame *frame = handle->pAudioDecFrame;
	if(frame == NULL)
	{
		printf("this audio decoder frame is null!!\n");
		return -1;
	}

	/* read all the output frames (in general there may be any number of them */
	while (ret >= 0)
	{
		ret = avcodec_receive_frame(handle->pAudioCodecCtx, frame);
		if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
		{
			break;
		}
		else if (ret < 0)
		{
			printf("Error during decoding\n");
			break;
		}

		if(isResample == 1)
		{
			/* 音频重采样 */
			media_resample_audio(handle,frame->data,av_frame_get_channel_layout(frame),
					av_frame_get_sample_rate(frame),frame->nb_samples,frame->format,\
					layout,sampleRate,format);
		}else
		{
			/* 上抛数据 */
			if(handle->audioDecSink)
			{
				handle->audioDecSink(frame,handle->audioDecUser);
			}
	//		media_writeAudioFile(frame,"./aaa.pcm");
		}
	}

	return 0;
}



int media_pcm_audio(mediaFfmpeg_t* handle,mediaPacket_t* pMediaPkt,\
		int isResample,\
		int layout,int sampleRate,enum AVSampleFormat format)
{

	AVCodecContext * pAVCodecCtx = handle->pAudioCodecCtx;


	if(isResample == 1)
	{
		/* 音频重采样 */
		media_resample_audio(handle,&(pMediaPkt->data),pAVCodecCtx->channels,pAVCodecCtx->sample_rate,\
				pMediaPkt->size/2,pAVCodecCtx->sample_fmt,layout,sampleRate,format);
	}else
	{
		/* 上抛数据 */
		if(handle->audioDecSink)
		{
			//handle->audioDecSink(frame,handle->audioDecUser);
		}
//		media_writeAudioFile(frame,"./aaa.pcm");
	}

	return 0;
}



///////////////////////demo///////////////////
static int decAudio(AVFrame *frame,void* user)
{
	media_writeAudioFile(frame,"./aaa.pcm");

	return 0;
}

int media_test_demo(char* url)
{
	printf("media_test_demo [%s]\n",url);
	mediaFfmpeg_t* pstCurrentHandle = NULL; //当前播放的句柄
	pstCurrentHandle = media_open_url(url, 0, NULL, NULL, NULL);
	if( NULL == pstCurrentHandle  )
	{
		printf("audio play open url is error\n");
		return -1;
	}
	/* 设置解码回调 */
	media_setDecoderSink(pstCurrentHandle,decAudio,NULL);

	int nAudioRead = 0;
	mediaPacket_t ptk;
	int flag = 0;

	while(1)
	{
#ifdef MYDEBUG
		AVPacket avPkt;
		nAudioRead = media_get_frame(pstCurrentHandle, &ptk, &avPkt);
#else
		nAudioRead = media_get_frame(pstCurrentHandle, &ptk);
#endif
		if(nAudioRead >= 0)
		{
			if(ptk.type == MEDIA_TYPE_AUDIO)
			{
				printf("audio==================== size[%d]\n",ptk.size);
				/* 解码 */
				media_decode_audio(pstCurrentHandle,\
						&ptk,1,AV_CH_LAYOUT_STEREO,\
						48000,AV_SAMPLE_FMT_S16P);
				if(flag++ > 500)
				{
					break;
				}
			}
		}
		else if( nAudioRead == -3 )
		{

		}else if( nAudioRead == -2 )
		{
			break;
		}
#ifndef MYDEBUG
		media_unpaket(pstCurrentHandle, &ptk);
#endif
		usleep(20*1000);
	}

	return 0;
}





