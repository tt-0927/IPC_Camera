#include "sdk_packet_write.h"
#include "libavutil/time.h"
#include "libavcodec/avcodec.h"
#include "libavutil/mem.h"
#include "os.h"
#include "dlog.h"

#define NULL_VALUE_UNINIT -1

sdk_packet_write::sdk_packet_write(std::string name)
{
    m_objectName = name;
    m_video_pts = NULL_VALUE_UNINIT;
    m_audio_pts = NULL_VALUE_UNINIT;
    m_first_Iframe = 0;
    m_video_index = -1;
    m_audio_index = -1;
    m_video_count = 0;
    m_audio_count = 0;
    m_pFormatCtx = NULL;
    m_continue_count_lost = 0;
    m_gop_size = 0;
    m_IframeNem = 0;
    m_write_frame_count_video = 0;
    m_audio_channel = 2;
    m_write_frame_count_video = 0;
    m_isFlv = false;

    m_bsfc = NULL;
    m_nFirstVideoPts = NULL_VALUE_UNINIT;
    m_nFirstAudioPts = NULL_VALUE_UNINIT;


    memset(&videoSync_,0,sizeof(AVSyncInfo));
    memset(&audioSync_,0,sizeof(AVSyncInfo));
}


sdk_packet_write::~sdk_packet_write()
{
    sdk_write_frame_trailer();
}

void sdk_packet_write::setObjectName(std::string name)
{
	m_objectName = name;
}

int sdk_packet_write::sdk_write_frame_trailer()
{
    std::unique_lock<std::mutex> mtx(m_ctxMtx);
    int nRet = 0;
    if(m_bsfc)
    {
        av_bitstream_filter_close(m_bsfc);
        m_bsfc = NULL;
    }

    if (m_pFormatCtx)
    {
        //代表写文件头了
        if (m_first_Iframe == 1)
        {
            if (!(m_pFormatCtx->flags & AVFMT_NOFILE))
            {
                nRet = av_write_trailer(m_pFormatCtx);
            }

        }
        if ( !(m_pFormatCtx->flags & AVFMT_NOFILE))
        {
            avio_close(m_pFormatCtx->pb);
        }
        avformat_free_context(m_pFormatCtx);
        m_pFormatCtx = NULL;
    }

    m_video_pts = NULL_VALUE_UNINIT;
    m_audio_pts = NULL_VALUE_UNINIT;
    m_first_Iframe = 0;
    m_video_index = -1;
    m_audio_index = -1;
    m_video_count = 0;
    m_audio_count = 0;
    m_pFormatCtx = NULL;
    m_continue_count_lost = 0;
    m_gop_size = 0;
    m_IframeNem = 0;
    m_write_frame_count_video = 0;
    m_audio_channel = 2;
    m_write_frame_count_video = 0;
    m_isFlv = false;
    m_nFirstVideoPts = NULL_VALUE_UNINIT;
    m_nFirstAudioPts = NULL_VALUE_UNINIT;
    recordTime_.store(0);
    return nRet;
}


//主要针对pts做校准，由于无法保证源头那么标准,该补帧或者缺帧了
int sdk_packet_write::sdk_compate_pts(int64_t reference_pts, int64_t nowpts, int64_t millsecond, int* continue_count_lost, int continue_count,
    int64_t *ouputdifference_value)
{
    int64_t difference_value = nowpts - reference_pts;
    if (( difference_value >= millsecond) || (-difference_value <= millsecond))
    {
        if ((difference_value >= millsecond))
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
    //需要补帧了
    if (abs((*continue_count_lost)) >= continue_count)
    {
        dlog(LOG_WARN,"continue_count:%d  abs:%d",continue_count, *continue_count_lost);
        *ouputdifference_value = difference_value;
        return 1;
    }
    return 0;
}


int sdk_packet_write::deal_frame_timestamp(int d_value, int lostframe, int continue_threshold,int* continue_count_lost)
{
    //已经连续多长时间没有补帧过来了,连续几次缺帧大于自己界限
    if((d_value >= lostframe) || (d_value <= -lostframe))
    {
        if((d_value >= lostframe)){
            (*continue_count_lost)++;
        }
        else{
            (*continue_count_lost)--;
        }
    }
    else{
        (*continue_count_lost) = 0;
    }
    //需要补帧了
    if(abs((*continue_count_lost)) >= continue_threshold)
    {
//        printf("continue_threshold:%d abs:%d",continue_threshold, abs((*continue_count_lost)));
        return 1;
    }
    return 0;
}


int sdk_packet_write::checkVideoPts(AVSyncInfo& sync)
{
    if(sync.startTimer == 0)
    {
        sync.startTimer = OS_getSysTimeInMsec();//av_gettime_relative()/1000;
        sync.frameCount = 0;
        return 0;   //第一帧直接返回
    }

    uint64_t time_out =  0;
    int64_t normal_cnt = 0;
    int64_t currentTime = OS_getSysTimeInMsec();//aav_gettime_relative()/1000;

    //计算正常情况下，当前应该有多少帧数据了
    time_out = currentTime - sync.startTimer;
    normal_cnt = time_out * m_inputparam.video_frame_rate / 1000;

    //计算当前实际写入的帧数跟理论帧数的差值
    int frameCal = normal_cnt - sync.frameCount;

    sync.writeFrameNum = 0;
    sync.jumbFrameNum = 0;
    if(deal_frame_timestamp(frameCal, 6 , 5, &(sync.continue_lost_frame)) > 0)
    {
        //丢帧
        if(frameCal < -6)
        {
            //实际写的帧数大于理论值，需要抛帧
            sync.writeFrameNum = frameCal;
        }
        if(frameCal >= 6)
        {
            //补帧
            sync.writeFrameNum = 3;
        }
        if(frameCal >= 12)
        {
            //补帧
            sync.writeFrameNum = 5;
        }

        if(frameCal >= m_inputparam.video_frame_rate)
        {
            //需要跳帧，补帧已经没用了
            sync.jumbFrameNum = m_inputparam.video_frame_rate;
        }

        if(frameCal >= m_inputparam.video_frame_rate*2)
        {
            //需要跳帧，补帧已经没用了
            sync.jumbFrameNum = frameCal;
        }
    }

    return 0;
}

int sdk_packet_write::checkAudioPts(sdk_packet_write::AVSyncInfo &sync)
{
    if(sync.startTimer == 0)
    {
        sync.startTimer = OS_getSysTimeInMsec();//aav_gettime_relative()/1000;
        sync.frameCount = 0;
        return 0;   //第一帧直接返回
    }

    uint64_t time_out =  0;
    int64_t normal_cnt = 0;
    int64_t currentTime = OS_getSysTimeInMsec();//aav_gettime_relative()/1000;

    if ((++sync.audio_skip_count) % 40 != 0)
    {
        return 0;
    }
    sync.audio_skip_count = 0;

    //计算正常情况下，当前应该有多少帧数据了
    time_out = currentTime - sync.startTimer;
    normal_cnt = (time_out * m_inputparam.audio_sample_rate) / 1024000;

    //计算当前实际写入的帧数跟理论帧数的差值
    int frameCal = normal_cnt - sync.frameCount;

    sync.writeFrameNum = 0;
    sync.jumbFrameNum = 0;
    if(deal_frame_timestamp(frameCal, 6 , 4, &(sync.continue_lost_frame)) > 0)
    {
        //丢帧
        if(frameCal < -4)
        {
            //实际写的帧数大于理论值，需要抛帧
            sync.writeFrameNum = frameCal;
        }
        if(frameCal >= 4)
        {
            //补帧
            sync.writeFrameNum = 2;
        }
        if(frameCal >= 10)
        {
            //补帧
            sync.writeFrameNum = 5;
        }

        if(frameCal >= 40)
        {
            //需要跳帧，补帧已经没用了
            sync.jumbFrameNum = frameCal;
        }
    }

    return 0;
}
/*

AVIOContext *avio_alloc_context(
                  unsigned char *buffer,
                  int buffer_size,
                  int write_flag,
                  void *opaque,
                  int (*read_packet)(void *opaque, uint8_t *buf, int buf_size),//重写该函数，指定从内存中读取的方法，将buf_size字节大小的数据保存到buf
                  int (*write_packet)(void *opaque, uint8_t *buf, int buf_size),//对应的这是写内存的函数
                  int64_t (*seek)(void *opaque, int64_t offset, int whence));
---------------------


unsigned char *buffer：缓存开始位置

int buffer_size：缓存大小（默认32768）

unsigned char *buf_ptr：当前指针读取到的位置

unsigned char *buf_end：缓存结束的位置

void *opaque：URLContext结构体

*/
int GetspsData(Capture_CallBack_Data_t* pchFramedata)
{
    unsigned char *pchData = pchFramedata->data;
    int nIndex = 0;
    int nLastTye = 0;
    int nSpsPpsLen = 0;
    int nLastIndex = -1;
    int nDataSize = pchFramedata->size;
    for(nIndex = 0; nIndex < nDataSize - 5; nIndex++)
    {
        if(pchData[nIndex] == 0x00 && pchData[nIndex+1] == 0x00
                && pchData[nIndex+2] == 0x00 && pchData[nIndex+3] == 0x01)
        {
            if(nLastIndex >= 0 && nLastTye == 0x8)
            {
                    nSpsPpsLen = nIndex;
                    return nSpsPpsLen;
            }
            nLastIndex = nIndex;
            nLastTye = pchData[nIndex + 4] & 0x1f;
            printf("nLastTye:%0x\n", nLastTye);
        }
    }
    return nSpsPpsLen;
}

int sdk_packet_write::sdk_init_header(Capture_CallBack_Data_t* framedata)
{
	int ret = 0;
	if ((framedata->keyframe == 1) && (framedata->data_type == CAPTURE_VIDEO_TYPE))
	{
		if (framedata->streamType != STREAM_MUX_TYPE_VIDEO)
			sdk_add_audio_stream();
		sdk_add_video_stream();

		m_video_pts = 0;
		AVCodecContext* pCodeVideCtx = m_pFormatCtx->streams[m_video_index]->codec;
		int nDataLen = GetspsData(framedata);
		if(nDataLen > 0)
		{
			if(pCodeVideCtx->extradata_size > 0 && pCodeVideCtx->extradata)
			{
				av_free(pCodeVideCtx->extradata);
				pCodeVideCtx->extradata_size = 0;
			}
			pCodeVideCtx->extradata = (uint8_t*)av_malloc(nDataLen + AV_INPUT_BUFFER_PADDING_SIZE);
			memcpy(pCodeVideCtx->extradata, framedata->data, nDataLen);
			pCodeVideCtx->extradata_size = nDataLen;
		}

		if(m_isFlv && framedata->streamType != STREAM_MUX_TYPE_VIDEO)
		{
			/* flv格式需要补上额外信息，否则音频的采样率不正确 */
			AVCodecContext * pCodecCtx = NULL;
			pCodecCtx = m_pFormatCtx->streams[m_audio_index]->codec;
			pCodecCtx->extradata = (uint8_t*)av_malloc(2 + AV_INPUT_BUFFER_PADDING_SIZE);
			if(pCodecCtx->extradata == NULL)
			{
				dlog(LOG_ERROR,"name[%s] av_malloc error!!!",m_objectName.c_str());
			}else
			{
				std::cout << "add flv dsi!!!!!!!!!!!m_audio_index:" \
						  << m_audio_index << std::endl;
				make_dsi(3, 2, pCodecCtx->extradata);
				pCodecCtx->extradata_size = 2;
			}
		}

		if(m_pFormatCtx)
		{
			//无论是读内存或者写文件头，即使读内存这个也很重要
			ret = avformat_write_header(m_pFormatCtx, NULL);
		}

		if (ret != 0)
		{
			char chErrBuf[AV_ERROR_MAX_STRING_SIZE] = {0};
			av_make_error_string(chErrBuf, AV_ERROR_MAX_STRING_SIZE, ret);
			dlog(LOG_ERROR,"name[%s] it is err avformat_write_header! ret:[%d] error[%s]",\
				 m_objectName.c_str(),ret,chErrBuf);
		}
		else
		{
			dlog(LOG_DEBUG,"name[%s] it is ok avformat_write_header!!!!!!",m_objectName.c_str());
		}
		return ret != 0 ? -1 : 1;
	}
	return 0;
}


int sdk_packet_write::sdk_fill_packet(AVPacket &writepkt, Capture_CallBack_Data_t* framedata, bool &packdatafree)
{
	int ret = 0;
    struct AVRational time_base = { 1,1000 };

	packdatafree = false;
	writepkt.data = framedata->data;
    writepkt.size = framedata->size;
    writepkt.flags = framedata->keyframe ? AV_PKT_FLAG_KEY : 0;

    if (framedata->data_type == CAPTURE_AUDIO_TYPE && framedata->streamType != STREAM_MUX_TYPE_VIDEO)
    {
        /*判断是否是flv格式*/
        if((m_isFlv) && (m_inputparam.audio_codec_id == AV_CODEC_ID_AAC) && (m_bsfc))
        {
            /*用于检测aac*/
            if (framedata->data[0] == 0xff && (framedata->data[1] & 0xf0) == 0xf0)
            {
                ret = av_bitstream_filter_filter(m_bsfc,m_pFormatCtx->streams[m_audio_index]->codec,\
                                                 NULL,&writepkt.data,&writepkt.size,\
                                                 framedata->data,framedata->size,\
                                                 0);
                if(ret > 0)
                {
                    /*过滤成功，需要释放数据*/
                    packdatafree = true;
                }
                else if(ret < 0)
                {
                    dlog(LOG_ERROR,"name[%s] %s failed for stream %d",\
                                        m_objectName.c_str(),m_bsfc->filter->name,m_audio_index);
                    return ret;
                }
            }
        }

        m_audio_pts = m_audio_count * 1024000 / m_inputparam.audio_sample_rate;
        writepkt.pts = av_rescale_q(m_audio_pts, time_base, m_pFormatCtx->streams[m_audio_index]->time_base);
        writepkt.dts = writepkt.pts;
        writepkt.stream_index = m_audio_index;
        audioSync_.frameCount++;
        m_audio_count++;
    }
    else if(framedata->data_type == CAPTURE_VIDEO_TYPE)
    {
        m_video_pts = m_video_count * 1000 / m_inputparam.video_frame_rate;
        writepkt.pts = av_rescale_q(m_video_pts, time_base, m_pFormatCtx->streams[m_video_index]->time_base);
        writepkt.dts = writepkt.pts;
        writepkt.stream_index = m_video_index;
        videoSync_.frameCount++;
        m_video_count++;
    }
	return 0;
}


int sdk_packet_write::sdk_fill_packet2(AVPacket &stWritePkt, Capture_CallBack_Data_t* pstFramedata, bool &bFreeData)
{
    int nRet = 0;
    struct AVRational stTimebase;
    memset(&stTimebase, 0, sizeof(struct AVRational));
    stTimebase.num = 1;
    stTimebase.den = pstFramedata->nTimebase;
    bFreeData = false;
    stWritePkt.data = pstFramedata->data;
    stWritePkt.size = pstFramedata->size;
    stWritePkt.flags = pstFramedata->keyframe ? AV_PKT_FLAG_KEY : 0;
    if (pstFramedata->data_type == CAPTURE_AUDIO_TYPE && pstFramedata->streamType != STREAM_MUX_TYPE_VIDEO)
    {
        /*判断是否是flv格式*/
        if((m_isFlv) && (m_inputparam.audio_codec_id == AV_CODEC_ID_AAC) && (m_bsfc))
        {
            /*用于检测aac*/
            if (pstFramedata->data[0] == 0xff && (pstFramedata->data[1] & 0xf0) == 0xf0)
            {
                nRet = av_bitstream_filter_filter(m_bsfc,m_pFormatCtx->streams[m_audio_index]->codec,\
                                                 NULL,&stWritePkt.data,&stWritePkt.size,\
                                                 pstFramedata->data,pstFramedata->size,\
                                                 0);
                if(nRet > 0)
                {
                    /*过滤成功，需要释放数据*/
                    bFreeData = true;
                }
                else if(nRet < 0)
                {
                    dlog(LOG_ERROR,"name[%s] %s failed for stream %d",\
                         m_objectName.c_str(),m_bsfc->filter->name,m_audio_index);
                    return nRet;
                }
            }
        }
        if (NULL_VALUE_UNINIT == m_nFirstAudioPts) {
            m_nFirstAudioPts = pstFramedata->nPts;
        }
        stWritePkt.pts = av_rescale_q(pstFramedata->nPts - m_nFirstAudioPts, stTimebase, m_pFormatCtx->streams[m_audio_index]->time_base);
        stWritePkt.dts = stWritePkt.pts;
        stWritePkt.stream_index = m_audio_index;
    }
    else if(pstFramedata->data_type == CAPTURE_VIDEO_TYPE)
    {
        if (NULL_VALUE_UNINIT == m_nFirstVideoPts) {
            m_nFirstVideoPts = pstFramedata->nPts;
        }
        stWritePkt.pts = av_rescale_q(pstFramedata->nPts - m_nFirstVideoPts, stTimebase, m_pFormatCtx->streams[m_video_index]->time_base);
        stWritePkt.dts = stWritePkt.pts;
        stWritePkt.stream_index = m_video_index;
    }
    return 0;
}


int sdk_packet_write::sdk_av_sync(Capture_CallBack_Data_t* framedata)
{
	int write_num = 1;
	//还没有初始化
    if (framedata->data_type == CAPTURE_AUDIO_TYPE && m_audio_pts == NULL_VALUE_UNINIT)
    {
        m_audio_pts = 0;
    }

	/* 视频同步 */
    if ((m_audio_pts != NULL_VALUE_UNINIT) && (framedata->data_type == CAPTURE_AUDIO_TYPE))
    {
        checkAudioPts(audioSync_);
        if(audioSync_.writeFrameNum > 0)
        {   //补帧
            dlog(LOG_ERROR,"name[%s] audio add frame num:[%d]",\
                                m_objectName.c_str(),audioSync_.writeFrameNum);
            write_num = audioSync_.writeFrameNum;
            audioSync_.writeFrameNum = 0;   //只补一次帧，要清零
        }else if(audioSync_.writeFrameNum < 0)
        {   //丢帧
            write_num = 0;
            dlog(LOG_ERROR,"name[%s] audio reduce frame num:[%d]",\
                                m_objectName.c_str(),audioSync_.writeFrameNum);
            audioSync_.writeFrameNum++;
        }
        if(audioSync_.jumbFrameNum > 0){
            //跳帧
            dlog(LOG_ERROR,"name[%s] audio jump frame num:[%d]",\
                                m_objectName.c_str(),audioSync_.jumbFrameNum);
            m_audio_count += audioSync_.jumbFrameNum;
            audioSync_.frameCount += audioSync_.jumbFrameNum;
            audioSync_.jumbFrameNum = 0;
        }
    }

	/* 纯视频录制直接返回 */
	if (framedata->streamType == STREAM_MUX_TYPE_VIDEO)
	{
		return write_num;
	}

	/* 音频同步 */
    if ((m_video_pts != NULL_VALUE_UNINIT) && (framedata->data_type == CAPTURE_VIDEO_TYPE))
    {
        if ((framedata->keyframe == 1))
        {
            m_IframeNem++;
        }
        if (m_IframeNem == 1)
        {
            m_gop_size++;
        }

        //视频的检测音视频同步，以系统时作为参考
        if(framedata->keyframe == 1)
        {
            checkVideoPts(videoSync_);
        }
        if(videoSync_.writeFrameNum > 0)
        {   //补帧
            dlog(LOG_ERROR,"name[%s] video add frame num:[%d]",\
                                m_objectName.c_str(),videoSync_.writeFrameNum);
            write_num = videoSync_.writeFrameNum;
            videoSync_.writeFrameNum = 0;   //只补一次帧，要清零
        }else if(videoSync_.writeFrameNum < 0)
        {   //丢帧,P帧
            if(framedata->keyframe != 1){
                write_num = 0;
                dlog(LOG_ERROR,"name[%s] video reduce frame num:[%d]",\
                                    m_objectName.c_str(),videoSync_.writeFrameNum);
                videoSync_.writeFrameNum++;
            }
        }
        if(videoSync_.jumbFrameNum > 0){
            //跳帧
            dlog(LOG_ERROR,"name[%s] video jump frame num:[%d]",\
                                m_objectName.c_str(),videoSync_.jumbFrameNum);
            m_video_count += videoSync_.jumbFrameNum;
            videoSync_.frameCount += videoSync_.jumbFrameNum;
            videoSync_.jumbFrameNum = 0;
        }
    }
	return write_num;
}

int sdk_packet_write::sdk_write_frame(Capture_CallBack_Data_t* framedata)
{
    std::unique_lock<std::mutex> mtx(m_ctxMtx);

    AVPacket writepkt;
    int ret = 0;
    int write_num = 1;
    int i = 0;
    int nSendRet = 0;

    //参考视频帧，得等到第一个I帧来了
    if (m_pFormatCtx == NULL)
    {
        return -1;
    }

	/* 纯音频录制？ */
    if (m_inputparam.write_type == CAPTURE_AUDIO_TYPE)
    {
		return 0;
    }

	/* 写文件头失败置为-1 */
    if (m_first_Iframe == -1)
    {
        return -1;
    }

	/* 初始化文件头 */
    if (m_first_Iframe == 0)
    {
		m_first_Iframe = sdk_init_header(framedata);
    }

	/* 音视频同步、写文件 */
    if (m_first_Iframe == 1)
    {
		write_num = sdk_av_sync(framedata);
        for (i = 0; i < write_num; i++)
        {
            //每次都要添加完整的数据
            av_init_packet(&writepkt);

		    bool packdatafree = false;
			ret = sdk_fill_packet(writepkt, framedata, packdatafree);
			if (ret < 0)
				continue;

            ret = av_interleaved_write_frame(m_pFormatCtx, &writepkt);
            if (ret < 0)
            {
                char chErrBuf[AV_ERROR_MAX_STRING_SIZE] = {0};
                av_make_error_string(chErrBuf, AV_ERROR_MAX_STRING_SIZE, ret);
                dlog(LOG_ERROR,"name[%s] write packet error type:%d rate:%d"
                               " size:%d size:%d write_num:%d i:%d ret:%d "
                                "errinfo:%s\n",\
                                m_objectName.c_str(),framedata->data_type, \
                                m_inputparam.video_frame_rate, \
                                writepkt.size, framedata->size, write_num, i, ret,chErrBuf);
                nSendRet = ret;  //写文件失败
            }

            if(packdatafree && (writepkt.data != NULL))
            {
                av_free(writepkt.data);
                writepkt.data = NULL;
            }
        }

        if(framedata->data_type == CAPTURE_VIDEO_TYPE)
        {
            //recordTime_
            if ((m_video_count % (m_inputparam.video_frame_rate * 1)) == 0)
            {
                recordTime_++;
            }
        }
    }


    return nSendRet;
}


int sdk_packet_write::sdk_write_frame2(Capture_CallBack_Data_t* pstFramedata)
{
    AVPacket stWritePkt;
    int nRet = 0;
    int nSendRet = 0;

    std::unique_lock<std::mutex> mtx(m_ctxMtx);

    //参考视频帧，得等到第一个I帧来了
    if (m_pFormatCtx == NULL)
    {
        return -1;
    }

	/* 纯音频录制？ */
    if (m_inputparam.write_type == CAPTURE_AUDIO_TYPE)
    {
		return 0;
    }

	/* 写文件头失败置为-1 */
    if (m_first_Iframe == -1)
    {
        return -1;
    }

	/* 初始化文件头 */
    if (m_first_Iframe == 0)
    {
		m_first_Iframe = sdk_init_header(pstFramedata);
    }

	/* 音视频同步、写文件 */
    if (m_first_Iframe == 1)
    {
        av_init_packet(&stWritePkt);

	    bool bFreeData = false;
		nRet = sdk_fill_packet2(stWritePkt, pstFramedata, bFreeData);
		if (nRet < 0) {
			return -1;
        }
        int64_t nPts = stWritePkt.pts;
        int64_t nDts = stWritePkt.dts;
        nRet = av_write_frame(m_pFormatCtx, &stWritePkt);
        if (nRet < 0)
        {
            char chErrBuf[AV_ERROR_MAX_STRING_SIZE] = {0};
            av_make_error_string(chErrBuf, AV_ERROR_MAX_STRING_SIZE, nRet);
            dlog(LOG_ERROR,"name[%s] write packet error nPts %lld nDts %lld type:%d rate:%d"
                           " size:%d size:%d  ret:%d "
                            "errinfo:%s\n",\
                            m_objectName.c_str(), nPts, nDts, pstFramedata->data_type, \
                            m_inputparam.video_frame_rate, \
                            stWritePkt.size, pstFramedata->size, nRet,chErrBuf);
            nSendRet = -1;  //写文件失败
        }
        if(bFreeData && (stWritePkt.data != NULL))
        {
            av_free(stWritePkt.data);
            stWritePkt.data = NULL;
        }

        if(pstFramedata->data_type == CAPTURE_VIDEO_TYPE)
        {
            recordTime_ = (pstFramedata->nPts - m_nFirstVideoPts) / pstFramedata->nTimebase;
        }
    }


    return nSendRet;
}


int sdk_packet_write::sdk_add_video_stream()
{
    AVCodecContext *c = NULL;
    AVCodec * pAVCodec = NULL;
    AVStream *st = NULL;

    memset(&videoSync_,0,sizeof(AVSyncInfo));

    m_video_format = AV_PIX_FMT_YUV420P;
    pAVCodec = avcodec_find_encoder(m_inputparam.video_codec_id);
    if (pAVCodec == NULL)
    {
        printf("Can't find encoder %d!\n", m_inputparam.video_codec_id);
        //return NULL; if encode h.264 should open this
    }
    st = avformat_new_stream(m_pFormatCtx, pAVCodec);
    if (!st) {
        fprintf(stderr, "Could not alloc stream\n");
        return -1;
    }

    c = st->codec;
    c->codec_id = m_inputparam.video_codec_id;
    c->codec_type = AVMEDIA_TYPE_VIDEO;
    c->width = m_inputparam.width;
    c->height = m_inputparam.height;
   // c->format = m_video_format;
    c->bit_rate = m_inputparam.video_bit_rate;
    c->codec_tag = 0;
    c->time_base.den = m_inputparam.video_frame_rate;
    c->time_base.num = 1;
    m_video_index = st->index;
    if(m_pFormatCtx->oformat->flags & AVFMT_GLOBALHEADER)
    {
           c->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
    }
    return 0;
}
int sdk_packet_write::sdk_add_audio_stream()
{
    AVStream *st = NULL;//index可以区分音视频
    st = avformat_new_stream(m_pFormatCtx, NULL);
	if (!st)
	{
		  fprintf(stderr, "Could not alloc stream\n");
		  return -1;
	}

    memset(&audioSync_,0,sizeof(AVSyncInfo));
    m_audio_index = st->index;

    if(m_isFlv)
    {
        /* 需要使用下面的方式，flv的音频采样率才支持48kHz */
        AVCodecContext *c = NULL;
        c = st->codec;
        c->codec_id = m_inputparam.audio_codec_id;
        c->codec_type = AVMEDIA_TYPE_AUDIO;
        c->time_base.den = m_inputparam.audio_bit_rate;
        c->time_base.num = 1;
        c->sample_fmt = AV_SAMPLE_FMT_S16;
        c->bit_rate = m_inputparam.audio_bit_rate ;
        c->bit_rate_tolerance = m_inputparam.audio_bit_rate * 12/10;
        c->sample_rate = m_inputparam.audio_bit_rate;
        c->channels = m_audio_channel;
        c->frame_size = 1024;
        if (m_pFormatCtx->oformat->flags & AVFMT_GLOBALHEADER)
            c->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
    }else
    {
        AVCodecParameters *c = NULL;
        c = st->codecpar;
        c->codec_id = m_inputparam.audio_codec_id;
        c->codec_type = AVMEDIA_TYPE_AUDIO;
        c->format = AV_SAMPLE_FMT_S16;
        c->bit_rate = m_inputparam.audio_bit_rate;
        c->sample_rate = m_inputparam.audio_sample_rate;
        c->channels = m_audio_channel;
        c->frame_size = 1024;
        c->codec_tag = 0;
        st->time_base.den = m_inputparam.audio_sample_rate / 100;
        st->time_base.num = 1;
    }

    if (m_inputparam.audio_codec_id == AV_CODEC_ID_AAC)
    {
        /* 将AAC编码器编码后的原始码流（ADTS头 + ES流）封装为MP4或者FLV或者MOV等格式时，
         * 需要先将ADTS头转换为MPEG-4 AudioSpecficConfig （将音频相关编解码参数提取出来），
         * 并将原始码流中的ADTS头去掉（只剩下ES流）。
        */
        m_bsfc = av_bitstream_filter_init("aac_adtstoasc");
    }
    return 0;
}

int sdk_packet_write::sdk_get_time()
{
    return recordTime_.load();
}

std::string sdk_packet_write::objectName()
{
    return m_objectName;
}

int sdk_packet_write::sdk_pause()
{
    audioSync_.startTimer = 0;
    videoSync_.startTimer = 0;
    return 0;
}

int sdk_packet_write::sdk_init_mediafile(CMedia_File_Inputparam_t intputparm)
{
    //或者用avformat_alloc_output_context2 结合avio_open
    int ret = 0;
    AVIOContext *avio_ctx = NULL;
    if(strstr(intputparm.url, "rtmp://"))
    {
        avformat_alloc_output_context2(&m_pFormatCtx, NULL, "flv", intputparm.url);
    }
    else
    {
        avformat_alloc_output_context2(&m_pFormatCtx, NULL, NULL, intputparm.url);
    }
    if (!m_pFormatCtx) {
        dlog(LOG_ERROR,"Couldn't open input stream.");
        return -1;
    }
	/* 最大缓存修改，不发音频会默认缓存10s数据 */
	m_pFormatCtx->max_interleave_delta = 5;
    m_isFlv = false;
    if(strstr(intputparm.url, ".flv"))
    {
        m_isFlv = true;
    }

#if 1
    //第三个参数标志位可写很重要，同时要写文件头，要不然写视频会卡主
    if (intputparm.write_pack_fun != NULL)
    {
        size_t avio_ctx_buffer_size = 1316;
        unsigned char *avio_ctx_buffer = NULL;
        avio_ctx_buffer = (unsigned char *)av_malloc(avio_ctx_buffer_size);
        avio_ctx = avio_alloc_context(avio_ctx_buffer, avio_ctx_buffer_size, 1, intputparm.userinputparam, NULL, intputparm.write_pack_fun, NULL);
        m_pFormatCtx->pb = avio_ctx;
        if (m_pFormatCtx->pb)
        {
            m_pFormatCtx->flags = AVFMT_FLAG_CUSTOM_IO;
            if (!m_pFormatCtx->iformat)
            {
                //暂时不探测会卡主。
                //av_probe_input_buffer2(m_pFormatCtx->pb, &m_pFormatCtx->iformat, intputparm.url, m_pFormatCtx, 0, m_pFormatCtx->format_probesize);
            }
            m_pFormatCtx->flags |= AVFMT_NOFILE; //不生成文件
        }
    }
#endif

    av_dump_format(m_pFormatCtx, 0, intputparm.url, 1);
    if (!(m_pFormatCtx->flags & AVFMT_NOFILE)) {
        ret = avio_open(&m_pFormatCtx->pb, intputparm.url, AVIO_FLAG_WRITE);
        if (ret < 0) {
            if (m_pFormatCtx)
            {
                avformat_free_context(m_pFormatCtx);
                m_pFormatCtx = NULL;
            }
            dlog(LOG_ERROR,"Could not open output file:%s",\
                 intputparm.url);
            return -1;
        }
    }
    memcpy(&m_inputparam, &intputparm, sizeof(CMedia_File_Inputparam_t));

    recordTime_.store(0);
    m_nFirstVideoPts = NULL_VALUE_UNINIT;
    m_nFirstAudioPts = NULL_VALUE_UNINIT;
    return 0;
}


void sdk_packet_write::make_dsi(unsigned int sampling_frequency_index, \
                                unsigned int channel_configuration, unsigned char* dsi)
{
    unsigned int object_type = 2; // AAC LC by default
    dsi[0] = (object_type<<3) | (sampling_frequency_index>>1);
    dsi[1] = ((sampling_frequency_index&1)<<7) | (channel_configuration<<3);
}





