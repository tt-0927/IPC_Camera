

#include "recvrtmp.h"
#include "log.h"

#include "os.h"
#include "os_thr.h"
#include "network_event.h"

/*写文件*/
//#define IS_RECORD_FILE	(1)


FILE *h264fp = NULL;
FILE *aacfp = NULL;
char start_code[4] = {0x00,0x00,0x00,0x01};

int buff_read8(int *buf8,char **src)
{
	char *pdata = *src;
	*buf8 = pdata[0]&0xff;
	(*src)++;

	return 0;
}

int buff_read16(int *buf16,char **src)
{
	char *pdata = *src;
	*buf16 = ((pdata[0]&0xff) << 8) | ((pdata[1]&0xff));
	(*src) += 2;

	return 0;
}

int buff_read24(int *buf24,char **src)
{
	char *pdata = *src;
	*buf24 = ((pdata[0]&0xff) << 16) | ((pdata[1]&0xff) << 8) | ((pdata[2]&0xff));
	(*src) += 3;

	return 0;
}

int buff_read32(int *buf32,char **src)
{
	char *pdata = *src;
	*buf32 = ((pdata[0]&0xff) << 24) | ((pdata[1]&0xff) << 16) | ((pdata[2]&0xff) << 8) | ((pdata[3]&0xff));
	(*src) += 4;

	return 0;
}

int buff_readSize(char *buf,char **src,int readSize)
{
	char *pdata = *src;
	memcpy(buf,pdata,readSize);
	(*src) += readSize;

	return 0;
}

int free_frameMessage_buff(rtmp_fream_t *buff)
{

	if(buff->data)
	{
		free(buff->data);
		buff->data = NULL;
	}

	if(buff)
	{
		free(buff);
		buff = NULL;
	}

	return 0;
}


rtmp_fream_t* anayle_flvTag_toH264(recv_rtmpInfo_t *handle,char *buf,int buffLen)
{
	int tagType = 0;
	int listSize = 0;
	int badySize = 0;
	int timestamp = 0;
	int h264Len = 0;
	char *pdata = NULL;
	rtmp_fream_t *pStream = NULL;
	int tagTimestamp = 0;
	int tagTimestamp_ex = 0;
	int streamID = 0;
	int codecID = 0;
	int AVCPacketType = 0;

	int version = 0;
	int avcProfile = 0;
	int profileCompatibility = 0;
	int avcLevel = 0;
	int lengthSizeMinusOne = 0;
	int numOfSPS = 0;
	int numOfPPS = 0;
	int readNum = 0;
	int lenTmp = 0;

	pdata = buf;

	/*get tag header*/
	buff_read8(&tagType,&pdata);	//获取tag的类型
	buff_read24(&badySize,&pdata);	//获取bady的长度
	buff_read24(&tagTimestamp,&pdata);		//获取tag的时间戳
	buff_read8(&tagTimestamp_ex,&pdata);	//获取tag的额外时间戳
	buff_read24(&streamID,&pdata);			//获取tag的streamID

	/*
	 * tag header + tag bady
	 * tag header 是11Bity
	 * */
	if(tagType == 0x09)	//视频tag
	{
		/*get tag bady*/
		buff_read8(&codecID,&pdata);		//获取视频的编码方式

		if((/*buf[11]*/codecID&0xf) == 7)	//0x17
		{
			/*分配内存*/
			pStream = (rtmp_fream_t*)malloc(sizeof(rtmp_fream_t));
			if(!pStream)
			{
				printf("malloc error!!\n");
				return NULL;
			}
			memset(pStream,0,sizeof(rtmp_fream_t));

			pStream->type = 0;	//视频

			buff_read8(&AVCPacketType,&pdata);	//获取avc的类型
			buff_read24(&timestamp,&pdata);		//都为0，无效数据

			//AVC编码（H264编码）
			if(((/*buf[11]*/codecID>>4)&0xf) == 1)	//I帧
			{
				/*	0x17 | AVCPacketType(8) | CompostionTime(24) | Data |
				 * 	如果AVCPacketType=0x00，为AVCSequence Header,AVCDecorderConfigurationRecord(只在拉流的第一次出现)
					如果AVCPacketType=0x01，为AVC NALU；
					如果AVCPacketType=0x02，为AVC end ofsequence
				 *
				 * */
				if((/*buf[12]*/AVCPacketType&0xff) == 0x00)
				{

					/* 	该tag包含文件信息
					 * 	| cfgVersion(8) | avcProfile(8) | profileCompatibility(8) |avcLevel(8) | reserved(6) | lengthSizeMinusOne(2) |
					 *  reserved(3) | numOfSPS(5) |spsLength(16) | sps(n) | numOfPPS(8) | ppsLength(16) | pps(n) |
					 * */
					buff_read8(&version,&pdata);	//获取avc版本号
					buff_read8(&avcProfile,&pdata);	//avc质量
					buff_read8(&profileCompatibility,&pdata);
					buff_read8(&avcLevel,&pdata);
					buff_read8(&lengthSizeMinusOne,&pdata);
					lengthSizeMinusOne = lengthSizeMinusOne&0x3;
					buff_read8(&numOfSPS,&pdata);

					numOfSPS = numOfSPS&0x1f;		//sps帧的个数
					readNum = 0;
					while(readNum < numOfSPS)
					{
						if(readNum == 0)
						{
							buff_read16(&(handle->spsLen),&pdata);	//获取sps的长度
							handle->spsbuf = (char *)malloc(handle->spsLen);
							buff_readSize(handle->spsbuf,&pdata,handle->spsLen);

						}else
						{
							//获取剩下的sps帧
							buff_read16(&(lenTmp),&pdata);	//获取sps的长度
							handle->spsLen += lenTmp;
							handle->spsbuf = (char *)realloc(handle->spsbuf,handle->spsLen);
							buff_readSize(handle->spsbuf + (handle->spsLen - lenTmp),&pdata,handle->spsLen);
						}

						readNum++;
					}

					/*获取pps*/
					buff_read8(&numOfPPS,&pdata);	//pps帧的个数
					readNum = 0;
					while(readNum < numOfPPS)
					{
						if(readNum == 0)
						{
							buff_read16(&(handle->ppsLen),&pdata);
							handle->ppsbuf = (char *)malloc(handle->ppsLen);
							buff_readSize(handle->ppsbuf,&pdata,handle->ppsLen);
						}else
						{
							/*获取剩下的pps帧*/
							buff_read16(&(lenTmp),&pdata);	//获取sps的长度
							handle->ppsLen += lenTmp;
							handle->ppsbuf = (char *)realloc(handle->ppsbuf,handle->ppsLen);
							buff_readSize(handle->ppsbuf + (handle->ppsLen - lenTmp),&pdata,handle->ppsLen);
						}
						readNum++;
					}

					pStream->frameSize = handle->spsLen+handle->ppsLen+8;

					pStream->data = (unsigned char *)malloc(pStream->frameSize);
					memcpy(pStream->data,start_code,sizeof(start_code));
					memcpy(pStream->data+sizeof(start_code),handle->spsbuf,handle->spsLen);

					memcpy(pStream->data+sizeof(start_code)+handle->spsLen,start_code,sizeof(start_code));
					memcpy(pStream->data+sizeof(start_code)+handle->spsLen+sizeof(start_code),handle->ppsbuf,handle->ppsLen);

					pStream->iFrame = 1;

#if 0
					if(spsbuff)
					{
						free(spsbuff);
						spsbuff = NULL;
					}
					if(ppsbuff)
					{
						free(ppsbuff);
						ppsbuff = NULL;
					}
#endif

//					printf("version[0x%x] avcProfile[0x%x] profileCompatibility[0x%x] avcLevel[0x%x] " \
//							"lengthSizeMinusOne[0x%x] numOfSPS[0x%x] spsLength[0x%x] numOfPPS[0x%x] ppsLength[0x%x]\n",\
//							version,avcProfile,profileCompatibility,avcLevel,lengthSizeMinusOne,numOfSPS,spsLength,numOfPPS,ppsLength);


				}else if((/*buf[12]*/AVCPacketType&0xff) == 0x01)
				{
					/*	h264帧数据
					 * 	4bit avcbaluda长度
					 *	data
					 * */
					buff_read32(&h264Len,&pdata);
					printf("I Frame------------------badysize[%d] h264Len[%d] spsLen[%d] ppsLength[%d] frametype[%d]\n",\
							badySize,h264Len,handle->spsLen,handle->ppsLen,buf[20]&0x1f);

					pStream->frameSize = h264Len+(4*3)+handle->spsLen+handle->ppsLen;
					pStream->data = (unsigned char *)malloc(pStream->frameSize);

					//加上spspps
					memcpy(pStream->data,start_code,sizeof(start_code));
					memcpy(pStream->data+sizeof(start_code),handle->spsbuf,handle->spsLen);

					memcpy(pStream->data+sizeof(start_code)+handle->spsLen,start_code,sizeof(start_code));
					memcpy(pStream->data+sizeof(start_code)+handle->spsLen+sizeof(start_code),handle->ppsbuf,handle->ppsLen);

					//I帧数据
					memcpy(pStream->data+sizeof(start_code)+handle->spsLen+sizeof(start_code)+handle->ppsLen,start_code,sizeof(start_code));
					buff_readSize(pStream->data+sizeof(start_code)+handle->spsLen+sizeof(start_code)+handle->ppsLen+sizeof(start_code),&pdata,h264Len);

					pStream->iFrame = 1;

				}

			}else	//P帧
			{

				/*	h264帧数据
				 * 	4bit avcbaluda长度
				 *	data
				 * */
				buff_read32(&h264Len,&pdata);

				pStream->frameSize = h264Len+4;
				pStream->data = (unsigned char *)malloc(pStream->frameSize);
				memcpy(pStream->data,start_code,sizeof(start_code));
				buff_readSize(pStream->data+sizeof(start_code),&pdata,h264Len);

			}


		}else
		{
			//其他编码

		}


	}else
	{
		/*文件头等其他tag*/
		printf("other tag[0x%x]!\n\n\n\n",(buf[0]&0x1f));

	}


	return pStream;
}


#define ADTS_HEADER_SIZE (7)
static int m_channel = 2; // 双声道
static int m_profile = 1; // AAC(Version 4) LC

void add_adts_header(unsigned char *p, int es_len)
{
    int frame_len = ADTS_HEADER_SIZE + es_len;

    *p++ = 0xff;                                    //syncword  (0xfff, high_8bits)
    *p = 0xf0;                                      //syncword  (0xfff, low_4bits)
    *p |= (0 << 3);                                 //ID (0, 1bit)
    *p |= (0 << 1);                                 //layer (0, 2bits)
    *p |= 1;                                        //protection_absent (1, 1bit)
    p++;
    *p = (unsigned char) ((m_profile & 0x3) << 6);  //profile (profile, 2bits)
    *p |= ((3 & 0xf) << 2);         //sampling_frequency_index (sam_idx, 4bits)
    *p |= (0 << 1);                                 //private_bit (0, 1bit)
    *p |= ((m_channel & 0x4) >> 2);                 //channel_configuration (channel, high_1bit)
    p++;
    *p = ((m_channel & 0x3) << 6);                  //channel_configuration (channel, low_2bits)
    *p |= (0 << 5);                                 //original/copy (0, 1bit)
    *p |= (0 << 4);                                 //home  (0, 1bit);
    *p |= (0 << 3);                                 //copyright_identification_bit (0, 1bit)
    *p |= (0 << 2);                                 //copyright_identification_start (0, 1bit)
    *p |= ((frame_len & 0x1800) >> 11);             //frame_length (value, high_2bits)
    p++;
    *p++ = (unsigned char) ((frame_len & 0x7f8) >> 3);  //frame_length (value, middle_8bits)
    *p = (unsigned char) ((frame_len & 0x7) << 5);      //frame_length (value, low_3bits)
    *p |= 0x1f;                                         //adts_buffer_fullness (0x7ff, high_5bits)
    p++;
    *p = 0xfc;                                          //adts_buffer_fullness (0x7ff, low_6bits)
    *p |= 0;                                            //number_of_raw_data_blocks_in_frame (0, 2bits);
    p++;
}

rtmp_fream_t* anayle_flvTag_toAudio(char *buf,int buffLen)
{
	int tagType = 0;
	int listSize = 0;
	int badySize = 0;
	char *pdata = NULL;
	rtmp_fream_t *pStream = NULL;
	int tagTimestamp = 0;
	int tagTimestamp_ex = 0;
	int streamID = 0;
	int codecID = 0;
	int audioType = 0;
	int sampleRate = 0;
	int sampleByte = 0;
	int audioChannel = 0;
	char adts[7] = {0};
	int AACPacketType = 0;
	int aacLen = 0;

	pdata = buf;

	/*get tag header(11B)*/
	buff_read8(&tagType,&pdata);	//获取tag的类型
	buff_read24(&badySize,&pdata);	//获取bady的长度
	buff_read24(&tagTimestamp,&pdata);		//获取tag的时间戳
	buff_read8(&tagTimestamp_ex,&pdata);	//获取tag的额外时间戳
	buff_read24(&streamID,&pdata);			//获取tag的streamID


	if(tagType == 0x08)	//音频tag
	{
		//获取音频编码参数
		buff_read8(&codecID,&pdata);	//0xAF
		audioType = (codecID>>4)&0x0f;
		sampleRate = (codecID>>2)&0x03;
		sampleByte = (codecID>>1)&0x01;
		if(((codecID)&0x01) == 0x01)
		{
			audioChannel = 2;//双声道，aac的都是双声道
		}else
		{
			audioChannel = 1;//单声道
		}

		/*分配内存*/
		pStream = (rtmp_fream_t*)malloc(sizeof(rtmp_fream_t));
		if(!pStream)
		{
			printf("malloc error!!\n");
			return NULL;
		}
		memset(pStream,0,sizeof(rtmp_fream_t));

		pStream->type = 1;	//音频

		if(audioType == 0x0a)	//aac编码
		{
			/*只有aac类型会有AACPacketType这个字段*/
			buff_read8(&AACPacketType,&pdata);	//都是0x01

			/*计算aac有效数据的长度*/
			aacLen = badySize - 2;

			pStream->frameSize = aacLen + 7;
			pStream->data = (unsigned char *)malloc(pStream->frameSize);
			if(pStream->data == NULL)
			{
				printf("malloc error!!\n");
				free_frameMessage_buff(pStream);
			}

			/*添加adts头信息*/
			add_adts_header(adts,aacLen);
			memcpy(pStream->data,adts,sizeof(adts));
			buff_readSize(pStream->data+sizeof(adts),&pdata,aacLen);

		}else if(audioType == 0x07)	//G711-a率编码
		{


		}else if(audioType == 0x07)	//G711-u率编码
		{


		}else	//其他编码
		{


		}


	}else
	{
		printf("no audio tag!!\n");
		return NULL;
	}


	return pStream;
}


/*上抛数据*/
int recv_deal_data(recv_rtmpInfo_t *rtmpHandle,rtmp_fream_t *frame)
{
	if((rtmpHandle == NULL) || (frame == NULL))
	{
		printf("this argument is NULL!!\n");
		return -1;
	}

	//上抛数据
	if(rtmpHandle->inparam.dealData)
	{
		rtmpHandle->inparam.dealData(frame->data,frame->frameSize,frame->type,rtmpHandle->inparam.user);
	}

	/*释放数据*/
	free_frameMessage_buff(frame);


#if 0

	int listSize = 0;
	//判断链表是否满
	listSize = list_lockAndGet_size(rtmpHandle->pListHandle);
	if((listSize > 30))
	{
		//超过链表的长度或者不是音视频，则抛掉
		free_frameMessage_buff(frame);
		return -1;
	}
	/*插入链表中*/
	list_lockAndPush_backSignal(rtmpHandle->pListHandle,frame);
#endif

	return 0;
}




void recv_rtmp_data(void *argv)
{
	recv_rtmpInfo_t *rtmpHandle = (recv_rtmpInfo_t*)argv;
	if(rtmpHandle == NULL)
	{
		printf("this argv is null!!\n");
		return ;
	}

	int nRead = 0;
	int countbufsize = 0;
	int bufsize=1024*1024*10;
	char *buf=(char*)malloc(bufsize);
	memset(buf,0,bufsize);
	int tagType = 0;
	int listSize = 0;
	int badySize = 0;
	int tmp = 0;
	rtmp_fream_t *Frame = NULL;

#if IS_RECORD_FILE
	/*写h264文件*/
	h264fp = fopen("receive.h264","wb");
	if (!h264fp){
		RTMP_LogPrintf("Open File Error.\n");
		return ;
	}

	aacfp = fopen("receive.aac","wb");
	if (!h264fp){
		RTMP_LogPrintf("Open File Error.\n");
		return ;
	}

	FILE *fp=fopen("receive.flv","wb");
	if (!fp){
		RTMP_LogPrintf("Open File Error.\n");
		return ;
	}
#endif

	/*
	 * 读取到的数据就是flv格式的音视频数据
	 * tag header[11B] + tag bady + pre tag size[4B]
	 *
	 * */
//	while((nRead = RTMP_Read(rtmpHandle->rtmp,buf,bufsize)))
	while((rtmpHandle) && (RTMP_IsConnected(rtmpHandle->rtmp)) && (!RTMP_IsTimedout(rtmpHandle->rtmp)))
	{
		/*读取数据*/
		nRead = RTMP_Read(rtmpHandle->rtmp,buf,bufsize);

#if IS_RECORD_FILE
		/*写flv文件*/
		fwrite(buf,1,nRead,fp);
#endif

		/*判断tag的类型*/
		tagType = buf[0]&0x1f;

		if(tagType == 0x08)	//音频tag
		{
			Frame = anayle_flvTag_toAudio(buf,nRead);
			if(Frame)
			{
				recv_deal_data(rtmpHandle,Frame);

#if IS_RECORD_FILE
				/*write aac file*/
				fwrite(Frame->data,1,Frame->frameSize,aacfp);
				free_frameMessage_buff(Frame);
#endif
			}

		}else if(tagType == 0x09)	//视频tag
		{

			Frame = anayle_flvTag_toH264(rtmpHandle,buf,nRead);
			if(Frame)
			{
				recv_deal_data(rtmpHandle,Frame);

#if IS_RECORD_FILE
				/*write h264 file*/
				fwrite(Frame->data,1,Frame->frameSize,h264fp);
				free_frameMessage_buff(Frame);
#endif
			}

			tmp++;

		}else if(tagType == 0x12)	//脚本tag
		{
			printf("script tag!\n");
		}else
		{
			/*文件头等其他tag*/
			printf("other tag[0x%x]!\n\n\n\n",(buf[0]&0x1f));
		}


#if IS_RECORD_FILE
		if(tmp > 100)
		{
			printf("close file!!\n");
			fclose(h264fp);
			fclose(fp);
			fp = NULL;
			h264fp = NULL;
			exit(0);
		}
#endif

	}

	printf("stop recv!!\n");

	return ;
}



recv_rtmpHandle_t recv_init_rtmp(rtmp_inparam_t inparam)
{
	int ret = 0;
	recv_rtmpInfo_t *rtmpHandle = (recv_rtmpInfo_t*)malloc(sizeof(recv_rtmpInfo_t));
	if(rtmpHandle == NULL)
	{
		printf("malloc error!!\n");
		return NULL;
	}

	memcpy(&(rtmpHandle->inparam),&inparam,sizeof(rtmp_inparam_t));

	/* set log level */
	rtmpHandle->rtmp = RTMP_Alloc();
	RTMP_Init(rtmpHandle->rtmp);

	//set connection timeout,default 30s
	rtmpHandle->rtmp->Link.timeout = 10;

	// HKS's live URL
	if(!RTMP_SetupURL(rtmpHandle->rtmp,rtmpHandle->inparam.rtmpUrl))
	{
		RTMP_Log(RTMP_LOGERROR,"SetupURL Err\n");
		RTMP_Free(rtmpHandle->rtmp);
		free(rtmpHandle);
		rtmpHandle = NULL;
		return NULL;
	}

	if (rtmpHandle->bLiveStream)
	{
		rtmpHandle->rtmp->Link.lFlags|=RTMP_LF_LIVE;
	}

	//1hour
	RTMP_SetBufferMS(rtmpHandle->rtmp, 3600*1000);

	if(!RTMP_Connect(rtmpHandle->rtmp,NULL))
	{
		RTMP_Log(RTMP_LOGERROR,"Connect Err\n");
		RTMP_Free(rtmpHandle->rtmp);
		free(rtmpHandle);
		rtmpHandle = NULL;
		return NULL;
	}

	if(!RTMP_ConnectStream(rtmpHandle->rtmp,0))
	{
		RTMP_Log(RTMP_LOGERROR,"ConnectStream Err\n");
		RTMP_Close(rtmpHandle->rtmp);
		RTMP_Free(rtmpHandle->rtmp);
		free(rtmpHandle);
		rtmpHandle = NULL;
		return NULL;
	}

	/*创建线程，接流*/
	ret = OS_thrCreate(&(rtmpHandle->tid),recv_rtmp_data,OS_DETACH,OS_THR_STACK_SIZE_DEFAULT,(void *)rtmpHandle);
	if(ret < 0)
	{
		printf("create pthrea error!!\n");
		RTMP_Close(rtmpHandle->rtmp);
		RTMP_Free(rtmpHandle->rtmp);
		free(rtmpHandle);
		rtmpHandle = NULL;
	}

	return rtmpHandle;
}



int recv_unInit_rtmp(recv_rtmpHandle_t *Handle)
{

	recv_rtmpInfo_t* rtmpHandle = (recv_rtmpInfo_t*)Handle;
	if(rtmpHandle == NULL)
	{
		printf("this argument is NULL!!\n");
		return -1;
	}

	if(rtmpHandle->rtmp)
	{
		RTMP_Close(rtmpHandle->rtmp);
		RTMP_Free(rtmpHandle->rtmp);
		rtmpHandle->rtmp = NULL;
	}


	if(rtmpHandle->spsbuf)
	{
		free(rtmpHandle->spsbuf);
		rtmpHandle->spsbuf = NULL;
	}

	if(rtmpHandle->ppsbuf)
	{
		free(rtmpHandle->ppsbuf);
		rtmpHandle->ppsbuf = NULL;
	}

	if(rtmpHandle)
	{
		free(rtmpHandle);
		rtmpHandle = NULL;
	}

	return 0;
}








