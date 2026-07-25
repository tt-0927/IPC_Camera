/*************************************************************************
	> File Name: audio_enc.c
	> Author:luoyk 
	> Mail: 
	> Created Time: 2022年09月01日 星期四 11时25分38秒
 ************************************************************************/
#define _GNU_SOURCE
#include<stdio.h>
#include<unistd.h>
#include"audio_enc.h"
#define G711_FRAME_SIZE (160)


static AVFrame* alloc_audio_frame(int nSampleRate, int nbSample, \
		uint64_t nChnLayout, enum AVSampleFormat eSampleFmt)
{
    int ret;
    AVFrame* pFrame = av_frame_alloc();
    if (pFrame == NULL)
    {
        // printf("Could not allocate audio frame\n");
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
    	// printf("Could not allocate audio data buffers\n");
        av_frame_free(&pFrame);
        return NULL;
    }

    return pFrame;
}

static int send_frame( ff_AudioEnc_S* pHandle, char* pPcmData, int nSize )
{
    if(pHandle->pCodec)
    {
        int nRet = OS_streamRingbufferPut (&(pHandle->stFrameRing), (unsigned char*)pPcmData, nSize);
        if( nRet != nSize )
        {
            // dlog( LOG_WARN, "enc auio send_fram nRet=%d, nSize=%d\n", nRet, nSize );
        }
        OS_semSignal( &(pHandle->stFrameSem) );
    }

    return 0 ;
}
static AVPacket* receive_packet( ff_AudioEnc_S* pHandle, int nTimeOut )
{
    AVPacket* pPacket = NULL;
    Int64 nAddr;

    if( OS_queGetQueuedCount( &pHandle->stPacketQue ) < pHandle->stExParam.nDelaySum)
    {
        return NULL;
    }

    int nStatus = OS_queGet( &( pHandle->stPacketQue  ), &nAddr, nTimeOut  );
    if( nStatus == 0 )
    {
        pPacket = ( AVPacket* )nAddr;
    }
    else
    {
        dlog(LOG_WARN," receive_packet is fail nStatus=%d, nTimeOut=%d\n", nStatus, nTimeOut);
    }
    return pPacket;
}
static int release_packet( AVPacket* pPacket )
{
    if( pPacket )
    {
        av_packet_free( &(pPacket) );
        pPacket = NULL;
    }
    return 0;
}

static void* encode_audio_thr( void* pParam )
{
    int nRet = 0;
    ff_AudioEnc_S* pHandle = (ff_AudioEnc_S*)pParam;

    char* pPcmData = (char*)malloc( pHandle->nFrameByte  );
    dlog(LOG_DEBUG, "pHandle->nFrameByte:%d", pHandle->nFrameByte);
    
    while( !pHandle->nExit )
    {
        while( OS_streamRingbufferLen(&( pHandle->stFrameRing )) 
                >=pHandle->nFrameByte)
        {
            /*获取pcm的流数据*/
            OS_streamRingbufferGet( &(pHandle->stFrameRing), (unsigned char*)pPcmData, pHandle->nFrameByte );
        
            nRet = av_frame_make_writable(pHandle->pFrame);
            if(nRet != 0 && pHandle->pFrame)
            {
                // char errorBuf[AV_ERROR_MAX_STRING_SIZE];
                // av_strerror(nRet, errorBuf, sizeof(errorBuf));
                usleep(100*1000);
                continue;
            }
            nRet = av_samples_fill_arrays(pHandle->pFrame->data, pHandle->pFrame->linesize,
                                        (const uint8_t *)pPcmData, pHandle->pFrame->channels,
                                        pHandle->pFrame->nb_samples, static_cast<AVSampleFormat>(pHandle->pFrame->format), 0);

            
            nRet = avcodec_send_frame(pHandle->pCodecCtx, pHandle->pFrame);
            if (nRet < 0) 
            {
                dlog(LOG_WARN, "Error sending the frame to the audio encoder nRet = %d\n", nRet);
                continue;   
            }

            while( nRet >=0 )
            {
                nRet = avcodec_receive_packet(pHandle->pCodecCtx, pHandle->pPacket);
                if (nRet == AVERROR(EAGAIN) || nRet == AVERROR_EOF) 
                {
                    break;
                } else if (nRet < 0) 
                {
                    dlog(LOG_WARN, "Error encoding audio frame nRet=%d\n", nRet);
                    break ;
                }

                if(pHandle->pCodecCtx->codec_id == AV_CODEC_ID_AAC && pHandle->pPacket)
                {
                    if(pHandle->stExParam.nAdts)
                    {
                        av_packet_rescale_ts(pHandle->pPacket, pHandle->pCodecCtx->time_base, pHandle->pCodecCtx->pkt_timebase);
                        pHandle->pPacket->flags |= AV_PKT_FLAG_KEY;
                    }
                }

                AVPacket* pPacketTmp = av_packet_alloc(  ); 
                av_packet_move_ref( pPacketTmp, pHandle->pPacket );
                nRet = OS_quePut(&( pHandle->stPacketQue   ), (Int64)pPacketTmp, 0);
                if( nRet == OS_EFAIL )
                {
                    av_packet_free( &(pPacketTmp) );
                    pPacketTmp = NULL;
                }
                av_packet_unref( pHandle->pPacket );
            }
        }
        Uint32 nCout=0;
        OS_semWait( &(pHandle->stFrameSem), -1, &nCout );
    }
    free(pPcmData);
    pPcmData=NULL;
    return NULL;
}


static int ff_audioEnc_init( ff_AudioEnc_S* pHandle )
{
    // 初始化FFmpeg库
    /*av_register_all() 和 avcodec_register_all() ,Fmpeg 版本（≥ 4.0）中已被废弃。编解码器现在会自动注册*/
    //av_register_all();
    // avcodec_register_all();
    int nRet = 0;
    /*第一步查找编码器*/
    if( pHandle->stExParam.pCodecName != NULL )
    {
        pHandle->pCodec = avcodec_find_encoder_by_name( pHandle->stExParam.pCodecName );
    }

    if(!pHandle->pCodec)
    {
        pHandle->pCodec = avcodec_find_encoder( pHandle->stNeedParam.enCodecId);
    }

    if( !pHandle->pCodec )
    {
        dlog(LOG_WARN,"查找音频编码器失败\n");
        return -1;
    }
    
    /*第二步分配编码器上下文*/
    pHandle->pCodecCtx = avcodec_alloc_context3( pHandle->pCodec );
    if( !pHandle->pCodecCtx )
    {
        dlog(LOG_WARN,"分配音频编码器上下文失败\n");
        return -1;
    }
    /*第三步设置编码器参数*/
    if(pHandle->stNeedParam.nChannel == 1)
    { 
        pHandle->pCodecCtx->channel_layout = AV_CH_LAYOUT_MONO;
        dlog(LOG_DEBUG, "single audio channel ");
    }
    else
    {
        pHandle->pCodecCtx->channel_layout = AV_CH_LAYOUT_STEREO;//AV_CH_LAYOUT_STEREO;
    }

    pHandle->pCodecCtx->codec_id = pHandle->pCodec->id;
    pHandle->pCodecCtx->codec_type = AVMEDIA_TYPE_AUDIO;
    pHandle->pCodecCtx->bit_rate = pHandle->stExParam.nBite;
    pHandle->pCodecCtx->sample_rate = pHandle->stNeedParam.nSampleRate;
    pHandle->pCodecCtx->channels = av_get_channel_layout_nb_channels( pHandle->pCodecCtx->channel_layout );

    if(pHandle->pCodecCtx->codec_id == AV_CODEC_ID_AAC)
    {    
        pHandle->pCodecCtx->sample_fmt = pHandle->stNeedParam.enSampleFormat;
        pHandle->pCodecCtx->profile = FF_PROFILE_AAC_LOW;
    }
    else if(pHandle->pCodecCtx->codec_id == AV_CODEC_ID_PCM_MULAW)
    {
        pHandle->pCodecCtx->sample_fmt = pHandle->stNeedParam.enSampleFormat;
    }

    if(pHandle->stExParam.nAdts)
    {
        // 添加ADTS头部,flags上音频的编码会有误
//        pHandle->pCodecCtx->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
    }

    /*第四步关联编码器和编码器上下文*/
    nRet = avcodec_open2( pHandle->pCodecCtx, pHandle->pCodec, NULL);
    if( nRet < 0 )
    {
        dlog(LOG_WARN,"audi enc open2 fial nRet=%d\n", nRet);
    }

    if(pHandle->pCodecCtx->codec_id == AV_CODEC_ID_PCM_MULAW)
    {
        pHandle->pCodecCtx->frame_size = G711_FRAME_SIZE;
    }

    pHandle->pFrame = av_frame_alloc();
    pHandle->pFrame->nb_samples     = pHandle->pCodecCtx->frame_size;
    pHandle->pFrame->format         = pHandle->pCodecCtx->sample_fmt;
    pHandle->pFrame->channel_layout = pHandle->pCodecCtx->channel_layout;
    pHandle->pFrame->channels = av_get_channel_layout_nb_channels(pHandle->pFrame->channel_layout);
    
    
    nRet = av_frame_get_buffer(pHandle->pFrame, 0);
    /*计算出每一帧的数据 单个采样点的字节 * 通道数目 * 每帧采样点数量*/
    pHandle->nFrameByte = av_get_bytes_per_sample( static_cast<AVSampleFormat>(pHandle->pFrame->format) ) \
                          * pHandle->pFrame->channels \
                          * pHandle->pFrame->nb_samples;
    

    dlog(LOG_DEBUG, "audio nSampleRate:%d, frameSize=%d, nb_samples=%d channels = %d", 
    pHandle->stNeedParam.nSampleRate, pHandle->nFrameByte,
    pHandle->pFrame->nb_samples, pHandle->pFrame->channels);
    pHandle->pPacket = av_packet_alloc();

    /*创建流和队列*/
    int nStatus = OS_streamRingbufferCreate( &(pHandle->stFrameRing) , 128*1024 ); 
    if( nStatus !=  OS_SOK )
    {
        dlog(LOG_WARN, "audio osringbuf create fial size=%d\n", 128*1024);
        return -1;
    }
    nStatus = OS_queCreate( &( pHandle->stPacketQue  ), pHandle->stNeedParam.nPacketCnt  );
    if( nStatus !=  OS_SOK )
    {
        dlog(LOG_WARN, "audio os_que create fial, nQue_size=%d\n", pHandle->stNeedParam.nPacketCnt);
        return -1;
    }
    OS_semCreate( &(pHandle->stFrameSem), 4, 0);

    /*编码线程*/
    OS_thrCreate( &(pHandle->stEncode_thrId), encode_audio_thr, OS_JOINABLE, OS_THR_STACK_SIZE_DEFAULT, pHandle  );
    pthread_setname_np(pHandle->stEncode_thrId.hndl, "encode_audio_thr");
    return 0;

}

static int ff_audioEnc_uninit( ff_AudioEnc_S* pHandle )
{

    pHandle->nExit = 1;
    OS_semSignal( &(pHandle->stFrameSem) );
    OS_thrJoin( &(pHandle->stEncode_thrId)  );
    OS_streamRingbufferDelete( &( pHandle->stFrameRing  )   );
    while(1)
    {
        AVPacket* pPacket = receive_packet( pHandle, 100 );
        if( pPacket == NULL )
        {
            break;
        }
        release_packet(pPacket);
    }
    OS_semDelete( &( pHandle->stFrameSem ) );
    OS_queDelete( &( pHandle->stPacketQue) );
    av_frame_free( &(pHandle->pFrame) );
    av_packet_free( &(pHandle->pPacket));
    avcodec_free_context( &(pHandle->pCodecCtx) );

    if(pHandle->pSwrCtx)
    {
        swr_free(&(pHandle->pSwrCtx));
        pHandle->pSwrCtx = NULL;
    }
    return 0;
}

ff_AudioEnc_S* ff_audioEnc_alloc( ff_AudioEncNeedParam_S stNeedParam, char* pCodecName)
{
    ff_AudioEnc_S* pHandle = (ff_AudioEnc_S*)malloc(sizeof(ff_AudioEnc_S));

    memset(pHandle, 0, sizeof(ff_AudioEnc_S));

    pHandle->stNeedParam.enCodecId = stNeedParam.enCodecId;
    pHandle->stNeedParam.nSampleRate = stNeedParam.nSampleRate;
    pHandle->stNeedParam.enSampleFormat = stNeedParam.enSampleFormat;
    pHandle->stNeedParam.nPacketCnt = stNeedParam.nPacketCnt;
    pHandle->stNeedParam.nChannel = stNeedParam.nChannel;

    pHandle->stExParam.nBite = 128*1024;
    pHandle->stExParam.nProfile = FF_PROFILE_AAC_LOW;
    pHandle->stExParam.nDelaySum = 0;
    pHandle->stExParam.nAdts = 0;
    
    if( pCodecName != NULL )
    {
        pHandle->stExParam.pCodecName = (char*)malloc(strlen(pCodecName)+1);
        memset(pHandle->stExParam.pCodecName, 0, strlen(pCodecName)+1);
        strcpy( pHandle->stExParam.pCodecName, pCodecName );
    }

    pHandle->send_frame  = send_frame;
    pHandle->receive_packet = receive_packet;
    pHandle->release_packet = release_packet;
    pHandle->ff_audioEnc_init = ff_audioEnc_init;
    pHandle->ff_audioEnc_uninit = ff_audioEnc_uninit;

    return pHandle;
}

int ff_audioEnc_release( ff_AudioEnc_S* pHandle)
{
    if( pHandle )
    {
        if(pHandle->stExParam.pCodecName != NULL)
        {
            free( pHandle->stExParam.pCodecName );
            pHandle->stExParam.pCodecName = NULL;
        }
        free(pHandle);
    }
    return 0;
}
