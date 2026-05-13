#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "TS_process.h"
long CreateOneTsStreamDemux()
{
	TsStreamStorage *pTsStorage = (TsStreamStorage*)malloc(sizeof(TsStreamStorage));
	memset(pTsStorage,0,sizeof(TsStreamStorage));
	return (long)pTsStorage;
}


unsigned short GetShortContent(char *pStream)
{
	unsigned short shTemp = ((unsigned char *)pStream)[0];
	shTemp <<= 8;
	shTemp += ((unsigned char *)pStream)[1];
	return shTemp;
}



unsigned char GetCharContent(char *pStream)
{
	return *((unsigned char *)pStream);
}


long HandlePAT(char *pTsBegin,char *pTsEnd,TsStreamStorage *pTsStorage)
{
	long lMiddleLen = 0;
	long lPmtNum = 0;
	long lIndex = 0;
	unsigned short shProgramNumber = 0;
	unsigned short shPmtNumber = 0;

	//get pmt pid
	pTsBegin += 5;
	if (pTsBegin + 4 > pTsEnd)
	{
		return -12;
	}

	lMiddleLen = pTsEnd - pTsBegin - 4;
	if (lMiddleLen % 4)
	{
		return -13;
	}

	lPmtNum = lMiddleLen / 4;
	if (lPmtNum > PMT_MAX_NUM)
	{
		return -14;
	}

	//reset history data
	pTsStorage->m_lPMToffset = 0;

	for (lIndex = 0;lIndex < lPmtNum;lIndex++,pTsBegin+=4)
	{
		shProgramNumber = GetShortContent(pTsBegin);//program_number
		shPmtNumber = GetShortContent(pTsBegin+2);
		if (0 == shProgramNumber)//skip network_PID
			continue;

		shPmtNumber &= 0x1fff;//program_map_PID

		//update continuitycounter
		if (pTsStorage->m_atPMT[pTsStorage->m_lPMToffset].m_shPMTpid != shPmtNumber)
		{
			pTsStorage->m_atPMT[pTsStorage->m_lPMToffset].m_lPMTcontinuitycounter = 0;
		}
		pTsStorage->m_atPMT[pTsStorage->m_lPMToffset++].m_shPMTpid = shPmtNumber;
		//OUTPUTSTRING("handle pat,so get pmt pid is %d\n",shPmtNumber);
	}

// 	//change state
// 	if(lPmtNum)
// 	{
// 		pTsStorage->m_lStateMachine = HAS_PAT;
// 	}
	return 0;
}


long HandlePMT(char *pTsBegin,char *pTsEnd,TsStreamStorage *pTsStorage)
{
	unsigned char chStreamType = 0;
	unsigned short shelemPID = 0;
	long *pCurEsIndex = 0;
	unsigned short shESDesLen = 0;
	unsigned short shDesLen = 0;
	long lNeed2UpdateEsTable = 0;
	long lCheckIndex = 0;
	long lDstIndex = 0;
	long lFindTarget = 0;
	long lCopyIndex = 0;

	pTsBegin += 7;
	if (pTsBegin + 2 >= pTsEnd)
	{
		return -15;
	}
	shDesLen = GetShortContent(pTsBegin);
	shDesLen &= 0x0fff;//program_info_length

	pTsBegin += 2;
	pTsBegin += shDesLen;

	pTsEnd -= 4;//CRC_32

	//get video or audio pid and type
	long lTempIndex = 0;
	long alTempStreamType[ESTYPE_MAX_NUM] = {0};
	long alTempElemPid[ESTYPE_MAX_NUM] = {0};
	while(pTsBegin < pTsEnd)
	{
		chStreamType = GetCharContent(pTsBegin);//stream_type
		shelemPID = GetShortContent(pTsBegin+1);

		shelemPID &= 0x1fff;//elementary_PID

		//pCurEsIndex = &(pTsStorage->m_atPMT[pTsStorage->m_lCurHandlePmtIndex].m_lESOffset);
		//pTsStorage->m_atPMT[pTsStorage->m_lCurHandlePmtIndex].m_atES[*pCurEsIndex].m_lStreamType = chStreamType;
		//pTsStorage->m_atPMT[pTsStorage->m_lCurHandlePmtIndex].m_atES[*pCurEsIndex].m_lESPID = shelemPID;

		alTempStreamType[lTempIndex] = chStreamType;
		alTempElemPid[lTempIndex] = shelemPID;

		//OUTPUTSTRING("handle pmt,so get es type is 0x%x,pid is %d\n",chStreamType,shelemPID);

		lTempIndex++;

		if (lTempIndex >= ESTYPE_MAX_NUM)//is full
		{
			break;
		}

		shESDesLen = GetShortContent(pTsBegin+3);
		shESDesLen &= 0x0fff;//ES_info_length

		pTsBegin += 5;
		pTsBegin += shESDesLen;

	}

	//check if need 2 update es table
	pCurEsIndex = &(pTsStorage->m_atPMT[pTsStorage->m_lCurHandlePmtIndex].m_lESOffset);
	if (lTempIndex != *pCurEsIndex)
	{
		lNeed2UpdateEsTable = 1;
	}
	else
	{
		for (;lCheckIndex<lTempIndex;lCheckIndex++)
		{
			lFindTarget = 0;
			lDstIndex = 0;
			for (;lDstIndex < *pCurEsIndex;lDstIndex++)
			{
				long lDstStreamType = pTsStorage->m_atPMT[pTsStorage->m_lCurHandlePmtIndex].m_atES[lDstIndex].m_lStreamType;
				long lDstStreamPID = pTsStorage->m_atPMT[pTsStorage->m_lCurHandlePmtIndex].m_atES[lDstIndex].m_lESPID;
				if (alTempStreamType[lCheckIndex] == lDstStreamType && alTempElemPid[lCheckIndex] == lDstStreamPID)
				{
					lFindTarget = 1;
					break;
				}
			}
			if (0 == lFindTarget)
			{
				lNeed2UpdateEsTable = 1;
				break;
			}
		}
	}

	if (lNeed2UpdateEsTable)
	{
		printf("need 2 update es table\n");
		//clear history data
		*pCurEsIndex = 0;
		memset(&(pTsStorage->m_atPMT[pTsStorage->m_lCurHandlePmtIndex].m_atES),0,sizeof(EsParam)*ESTYPE_MAX_NUM);

		//update
		*pCurEsIndex = lTempIndex;
		for (;lCopyIndex < lTempIndex;lCopyIndex++)
		{
			pTsStorage->m_atPMT[pTsStorage->m_lCurHandlePmtIndex].m_atES[lCopyIndex].m_lStreamType = alTempStreamType[lCopyIndex];
			pTsStorage->m_atPMT[pTsStorage->m_lCurHandlePmtIndex].m_atES[lCopyIndex].m_lESPID = alTempElemPid[lCopyIndex];
		}
	}
// 	if (lGetSomething)
// 	{
// 		pTsStorage->m_atPMT[pTsStorage->m_lCurHandlePmtIndex].m_lHasGetData = 1;
// 	}
	return 0;
}











long HandlePSI(char *pTsBegin,char *pTsEnd,unsigned char chUnitStart,TsStreamStorage *pTsStorage,long lPSItype)
{
	unsigned short shSectLen = 0;
	long lLeftLen = 0;
	char *pCache = NULL;
	long *pOffset = NULL;
	long *pRealLength = NULL;
	if (PSI_PAT == lPSItype)
	{
		pCache = pTsStorage->m_achPATcache;
		pOffset = &(pTsStorage->m_lPAToffset);
		pRealLength = &(pTsStorage->m_lPATreallength);
	}
	else//PSI_PMT
	{
		pCache = pTsStorage->m_atPMT[pTsStorage->m_lCurHandlePmtIndex].m_achPMTcache;
		pOffset = &(pTsStorage->m_atPMT[pTsStorage->m_lCurHandlePmtIndex].m_lPMToffset);
		pRealLength = &(pTsStorage->m_atPMT[pTsStorage->m_lCurHandlePmtIndex].m_lPMTreallength);
	}

	if (chUnitStart)
	{
		unsigned char chFieldValue = *pTsBegin;//pointer_field
		if (pTsBegin+1+chFieldValue >= pTsEnd)
		{
			return -6;
		}
		pTsBegin++;
		pTsBegin += chFieldValue;
		
		if (pTsBegin + 3 >= pTsEnd)
		{
			return -7;
		}

		if ((0x00 != *pTsBegin) && (PSI_PAT == lPSItype))//pat table id
		{
			return -8;
		}
		if ((0x02 != *pTsBegin) && (PSI_PMT == lPSItype))//pmt table id
		{
			return -9;
		}

		shSectLen = GetShortContent(pTsBegin+1);
		shSectLen &= 0x0fff;//section_length

		pTsBegin += 3;
		lLeftLen = pTsEnd - pTsBegin;
		if (shSectLen > lLeftLen)//pat is divided into many ts packets
		{
			memcpy(pCache,pTsBegin,lLeftLen);
			*pOffset = lLeftLen;
			*pRealLength = shSectLen;
			return 0;
		}
		else
		{
			pTsEnd = pTsBegin + shSectLen;
		}
		
	}
	else
	{
		if (!(*pOffset))
		{
			return -10;
		}

		lLeftLen = pTsEnd - pTsBegin;
		if (lLeftLen > (PAT_MAX_LEN - *pOffset))
		{
			return -11;
		}
		memcpy(pCache + (*pOffset),pTsBegin,lLeftLen);
		*pOffset += lLeftLen;
		if (*pOffset < *pRealLength)//not last ts packet yet
		{
			return 0;
		}
		else
		{
			pTsBegin = pCache;
			pTsEnd = pCache+(*pRealLength);
		}
	}

	if (PSI_PAT == lPSItype)
	{
		return HandlePAT(pTsBegin,pTsEnd,pTsStorage);
	}
	else//PSI_PMT
	{
		return HandlePMT(pTsBegin,pTsEnd,pTsStorage);
	}
}




long GetPmtIndex(unsigned short shPID,TsStreamStorage *pTsStorage)
{
	long lIndex =0;
	for (;lIndex < pTsStorage->m_lPMToffset;lIndex++)
	{
		if (pTsStorage->m_atPMT[lIndex].m_shPMTpid == shPID)
		{
			return lIndex;
		}
	}

	return -1;
}



long GetEsIndex(unsigned short shPID,TsStreamStorage *pTsStorage,long *pBelongPMTIndex)
{
	long lIndex =0;
	long lEsIndex = 0;
	for (;lIndex<pTsStorage->m_lPMToffset;lIndex++)
	{
		lEsIndex = 0;
		for (;lEsIndex<pTsStorage->m_atPMT[lIndex].m_lESOffset;lEsIndex++)
		{
			if (pTsStorage->m_atPMT[lIndex].m_atES[lEsIndex].m_lESPID == shPID)
			{
				*pBelongPMTIndex = lIndex;
				return lEsIndex;
			}
		}
	}

	return -1;
}

unsigned long long AnalyzeTimestamp(char* p)
{
	unsigned char *ptr = (unsigned char*)p;
	unsigned long long timestamp = 0;
	timestamp += ((ptr[0]&0x0e)<<29);
	timestamp += ((ptr[1]&0xff)<<22);
	timestamp += ((ptr[2]&0xfe)<<14);
	timestamp += ((ptr[3]&0xff)<<7);
	timestamp += ((ptr[4]&0xfe)>>1);

	return timestamp;
}


long HandlePES(char *pTsBegin,char *pTsEnd,unsigned char chUnitStart,TsStreamStorage *pTsStorage,char *pDstData, long *pDstDataLen, TsParam *ptsparam)
{
	long *pCurCachePointer = &(pTsStorage->m_atPMT[pTsStorage->m_lCurHandlePmtIndex].m_atES[pTsStorage->m_lCurHandleEsIndex].m_lCurActivePointer);
	long lAnotherCachePointer = (0 == *pCurCachePointer)?1:0;

	char *pCurCache = pTsStorage->m_atPMT[pTsStorage->m_lCurHandlePmtIndex].m_atES[pTsStorage->m_lCurHandleEsIndex].m_achESCache[*pCurCachePointer];
	long *pCurCacheOffset = &(pTsStorage->m_atPMT[pTsStorage->m_lCurHandlePmtIndex].m_atES[pTsStorage->m_lCurHandleEsIndex].m_lESoffset[*pCurCachePointer]);
	long *pCurCacheTotalLength = &(pTsStorage->m_atPMT[pTsStorage->m_lCurHandlePmtIndex].m_atES[pTsStorage->m_lCurHandleEsIndex].m_lESTotalLength[*pCurCachePointer]);
	long *pCurPesHeaderLen = &(pTsStorage->m_atPMT[pTsStorage->m_lCurHandlePmtIndex].m_atES[pTsStorage->m_lCurHandleEsIndex].m_lESHeaderLength[*pCurCachePointer]);
	unsigned long long *pCurPts = &(pTsStorage->m_atPMT[pTsStorage->m_lCurHandlePmtIndex].m_atES[pTsStorage->m_lCurHandleEsIndex].m_alPts[*pCurCachePointer]);
	unsigned long long *pCurDts = &(pTsStorage->m_atPMT[pTsStorage->m_lCurHandlePmtIndex].m_atES[pTsStorage->m_lCurHandleEsIndex].m_alDts[*pCurCachePointer]);

	char *pAnotherCache = pTsStorage->m_atPMT[pTsStorage->m_lCurHandlePmtIndex].m_atES[pTsStorage->m_lCurHandleEsIndex].m_achESCache[lAnotherCachePointer];
	long *pAnotherCacheOffset = &(pTsStorage->m_atPMT[pTsStorage->m_lCurHandlePmtIndex].m_atES[pTsStorage->m_lCurHandleEsIndex].m_lESoffset[lAnotherCachePointer]);
	long *pAnotherCacheTotalLength = &(pTsStorage->m_atPMT[pTsStorage->m_lCurHandlePmtIndex].m_atES[pTsStorage->m_lCurHandleEsIndex].m_lESTotalLength[lAnotherCachePointer]);
	long *pAnotherPesHeaderLen = &(pTsStorage->m_atPMT[pTsStorage->m_lCurHandlePmtIndex].m_atES[pTsStorage->m_lCurHandleEsIndex].m_lESHeaderLength[lAnotherCachePointer]);
	unsigned long long *pAnotherPts = &(pTsStorage->m_atPMT[pTsStorage->m_lCurHandlePmtIndex].m_atES[pTsStorage->m_lCurHandleEsIndex].m_alPts[lAnotherCachePointer]);
	unsigned long long *pAnotherDts = &(pTsStorage->m_atPMT[pTsStorage->m_lCurHandlePmtIndex].m_atES[pTsStorage->m_lCurHandleEsIndex].m_alDts[lAnotherCachePointer]);

	char *pBackupCache = NULL;
	long *pBackupCacheOffset = NULL;
	long *pBackupCacheTotalLength = NULL;
	long *pBackupCachePesHeaderLen = NULL;
	unsigned long long *pBackupPts = NULL;
	unsigned long long *pBackupDts = NULL;

	unsigned char chPesHeaderLen = 0;
	unsigned short shPesLen = 0;
	long lLeftLen = 0;
	long lNeed2CallBackLastCache = 0;

	if (chUnitStart)
	{
		//check if need 2 callback last cache
		if (*pCurCacheOffset)
		{
			
			lNeed2CallBackLastCache = 1;
			pBackupCache = pCurCache;
			pBackupCacheOffset = pCurCacheOffset;
			pBackupCacheTotalLength = pCurCacheTotalLength;
			pBackupCachePesHeaderLen = pCurPesHeaderLen;
			pBackupPts = pCurPts;
			pBackupDts = pCurDts;

			pCurCache = pAnotherCache;
			pCurCacheOffset = pAnotherCacheOffset;
			pCurCacheTotalLength = pAnotherCacheTotalLength;
			pCurPesHeaderLen = pAnotherPesHeaderLen;
			pCurPts = pAnotherPts;
			pCurDts = pAnotherDts;

			*pCurCachePointer = lAnotherCachePointer;

			//OUTPUTSTRING("pid=%d,need 2 callback last data,change index 2 write,new index is %d\n",
				//pTsStorage->m_atPMT[pTsStorage->m_lCurHandlePmtIndex].m_atES[pTsStorage->m_lCurHandleEsIndex].m_lESPID,
				//*pCurCachePointer);
		}
		else
		{
			//OUTPUTSTRING("pes packet ,flag is 1,no need callback last pkt,cur index is %d\n",*pCurCachePointer);
		}

		//get pts and dts
		unsigned char chPtsDtsFlag = GetCharContent(pTsBegin+7);
		if (((chPtsDtsFlag >> 6)&0x03) == 0x03)
		{
			*pCurPts = AnalyzeTimestamp(pTsBegin+9);
			*pCurDts = AnalyzeTimestamp(pTsBegin+14);
			//OUTPUTSTRING("pts ande dts exist,pts is %I64d,dts is %I64d\n",*pCurPts,*pCurDts);
		}
		else if (((chPtsDtsFlag >> 6)&0x03) == 0x02)
		{
			*pCurPts = AnalyzeTimestamp(pTsBegin+9);
			*pCurDts = *pCurPts;
			//OUTPUTSTRING("just pts,value is %I64d\n",*pCurPts);
		}

		//write cache 2 cur pointer cache
		chPesHeaderLen = GetCharContent(pTsBegin+8);//PES_header_data_length
		*pCurPesHeaderLen = (9 + chPesHeaderLen);

		shPesLen = GetShortContent(pTsBegin+4);//PES_packet_length
		//OUTPUTSTRING("PES_packet_length is %d\n",shPesLen);
		*pCurCacheTotalLength = (shPesLen+6-(*pCurPesHeaderLen));

		pTsBegin += *pCurPesHeaderLen;
		lLeftLen = pTsEnd - pTsBegin;
		memcpy(pCurCache,pTsBegin,lLeftLen);
		*pCurCacheOffset = lLeftLen;
		//OUTPUTSTRING("handle pes,get start flag,pid is %d\n",pTsStorage->m_atPMT[pTsStorage->m_lCurHandlePmtIndex].m_atES[pTsStorage->m_lCurHandleEsIndex].m_lESPID);
		//OUTPUTSTRING("header len is %d,leftlen is %d,totallen is %d\n",*pCurPesHeaderLen,lLeftLen,*pCurCacheTotalLength);

		//callback last cache
		if (lNeed2CallBackLastCache)
		{
			if (*pDstDataLen < *pBackupCacheOffset)
			{
				return -16;
			}

			memcpy(pDstData,pBackupCache,*pBackupCacheOffset);
			*pDstDataLen = *pBackupCacheOffset;
			ptsparam->m_lDstDataType = pTsStorage->m_atPMT[pTsStorage->m_lCurHandlePmtIndex].m_atES[pTsStorage->m_lCurHandleEsIndex].m_lStreamType;
			ptsparam->m_lPts = *pBackupPts;
			ptsparam->m_lDts = *pBackupDts;

			//reset cache
			*pBackupCacheOffset = 0;
			*pBackupCacheTotalLength = 0;
			*pBackupCachePesHeaderLen = 0;
			*pBackupPts = 0;
			*pBackupDts = 0;
			//OUTPUTSTRING("callback es data,and reset cache\n");
		}
		else
		{
			*pDstDataLen = 0;
		}

		return 0;
	}
	else
	{
		//OUTPUTSTRING("pes packet,flag is not 1,than continue memcpy\n");

		lLeftLen = pTsEnd - pTsBegin;
		//OUTPUTSTRING("handle,pes,no start flag,pid is %d\n",pTsStorage->m_atPMT[pTsStorage->m_lCurHandlePmtIndex].m_atES[pTsStorage->m_lCurHandleEsIndex].m_lESPID);
		//OUTPUTSTRING("left len is %d\n",lLeftLen);
		//OUTPUTSTRING("pid=%d,offset is %ld,addr is %p-%p,leftlen is %ld\n",
			//pTsStorage->m_atPMT[pTsStorage->m_lCurHandlePmtIndex].m_atES[pTsStorage->m_lCurHandleEsIndex].m_lESPID,
			//*pCurCacheOffset,
			//pCurCache,pTsBegin,lLeftLen);
		memcpy(pCurCache+(*pCurCacheOffset),pTsBegin,lLeftLen);
		*pCurCacheOffset += lLeftLen;

		*pDstDataLen = 0;

		return 0;
	}
}






long CheckIfLostPackets(long *pLastCounter,long lCurCounter,long *pLostPacketNum)
{
	//lTraceTime++;
	//OUTPUTSTRING("[CheckIfLostPackets]tracetime[%I64d] last counter is %d,cur counter is %d(1~16)\n",
		//lTraceTime,*pLastCounter,lCurCounter+1);
	long lLstTempCounter = 0;
	if (0 == *pLastCounter)//first
	{
		//OUTPUTSTRING("it is first\n");
		if ((lCurCounter+1 )!= 1)
		{
			*pLastCounter = lCurCounter+1;
			*pLostPacketNum = lCurCounter;
			//OUTPUTSTRING("check lost ,lostpktnum is %d\n",*pLostPacketNum);
			return 1;
		}
		else
		{
			*pLastCounter = 1;
			return 0;
		}
	}
	else
	{
		lLstTempCounter = *pLastCounter + 1;
		lLstTempCounter = (lLstTempCounter > 16)?1:lLstTempCounter;

		if (lLstTempCounter != (lCurCounter+1))
		{
			//printf("check lost,counter,%ld-%ld\n",lLstTempCounter,lCurCounter+1);
			*pLastCounter = lCurCounter + 1;
			if ((lCurCounter+1)>lLstTempCounter)
			{
				*pLostPacketNum = (lCurCounter+1) - lLstTempCounter;
			}
			else if ((lCurCounter+1)<lLstTempCounter)
			{
				*pLostPacketNum = 16 - (lLstTempCounter - (lCurCounter+1));
			}
			return 1;
		}
		else
		{
			*pLastCounter = lLstTempCounter;
			//OUTPUTSTRING("it is normal,newlastcounter is %d\n",*pLastCounter);
			return 0;
		}
	}
	
	return 0;
}







long DemuxTsStream(IN long lHandle,IN char *pTsStream,OUT char *pDstData, INANDOUT long *pDstDataLen,OUT TsParam *ptsparam,OUT TsLostPkt *ptslostpkt)
{
	char *pMovingTsPointer = NULL;
	char *pEndTsPointer = NULL;
	unsigned short shPid = 0;
	unsigned char chTransError = 0;
	unsigned char chUnitStart = 0;
	unsigned char chAdaptationCtl = 0;
	unsigned char chAdaptation_field_exist = 0;
	unsigned char chPayload_data_exist = 0;
	long lHandleRet = 0;
	long lPmtIndex = 0;
	long lEsIndex = 0;
	unsigned char chContinuityCounter = 0;
	long lLostPackets = 0;
	long lLostPktNum = 0;
	long *pOrgContinuityCounter = NULL;

	//check input param
	TsStreamStorage *pTsStorage = (TsStreamStorage*)lHandle;
	if (NULL == pTsStorage)
	{
		return -1;
	}

	if (NULL == pTsStream || NULL == pDstData || NULL == pDstDataLen || NULL == ptsparam || *pDstDataLen <= 0 ||
		NULL == ptslostpkt)
	{
		return -2;
	}

	//process ts 4byte header   188 = 4瀛楄妭 packet header +184 瀛楄妭packet data
	pMovingTsPointer = pTsStream;			//璧峰
	pEndTsPointer = pTsStream + 188;		//缁撴潫
	if (pMovingTsPointer[0] != 0x47)		//sync_byte     灏辨槸0x47,杩欐槸DVB TS瑙勫畾鐨勫悓姝ュ瓧鑺�,鍥哄畾鏄�0x47.  鍗犳嵁packet head 鐨勭涓�涓�8bit
	{
		return -3;
	}
/*
	Packet Header锛堝寘澶达級淇℃伅璇存槑
	1: sync_byte						8bits			鍚屾瀛楄妭 鍥哄畾0x47
	2: transport_error_indicator		1bit			閿欒鎸囩ず淇℃伅锛�1锛氳鍖呰嚦灏戞湁1bits浼犺緭閿欒锛�
	3: payload_unit_start_indicator		1bit			璐熻浇鍗曞厓寮�濮嬫爣蹇楋紙packet涓嶆弧188瀛楄妭鏃堕渶濉厖锛�
	4: transport_priority				1bit			浼犺緭浼樺厛绾ф爣蹇楋紙1锛氫紭鍏堢骇楂橈級
	5: PID								13bits			Packet ID鍙风爜锛屽敮涓�鐨勫彿鐮佸搴斾笉鍚岀殑鍖�
	6: transport_scrambling_control		2bits			鍔犲瘑鏍囧織锛�00锛氭湭鍔犲瘑锛涘叾浠栬〃绀哄凡鍔犲瘑锛�
	7: adaptation_field_control			2bits			闄勫姞鍖哄煙鎺у埗
	8: continuity_counter				4bits			鍖呴�掑璁℃暟鍣�
*/

	
	shPid = GetShortContent(pMovingTsPointer+1);    //璁＄畻寰楀埌 寰楀埌 sync_byte 鍚庨潰鐨勬暟鎹�


	chTransError = (shPid>>15)&0x0001;//									transport_error_indicator
	chUnitStart = (shPid>>14)&0x0001;//										payload_unit_start_indicator
																			//OUTPUTSTRING("tsdemux err and unitstart is 0x%x and 0x%x\n",chTransError,chUnitStart);
	shPid &= 0x1fff;														//PID
	chAdaptationCtl = GetCharContent(pMovingTsPointer+3);					//寰楀埌绗�3涓�8浣嶆暟鎹�

	chAdaptation_field_exist = chAdaptationCtl&0x20;//adaptation_field_control former bit    锛燂紵锛燂紵
	chPayload_data_exist = chAdaptationCtl&0x10;//adaptation_field_control behiend bit       锛燂紵锛�?娌＄湅鎳�

	chContinuityCounter = chAdaptationCtl&0x0f;//continuity_counter(0~15)
	//OUTPUTSTRING("pid is %d,counter is %d\n",shPid,chContinuityCounter);
	if (chContinuityCounter < 0 || chContinuityCounter > 15)
	{
		printf("continuitycounter is not normal,value is %d\n",chContinuityCounter);
	}
	
	if (chTransError)
	{
		return -4;
	}

	if (0x1fff == shPid || !chPayload_data_exist)//0x1fff - NULL packet
	{
		return 0;
	}
	
	pMovingTsPointer += 4;								//璺宠繃packet head

	//skip adaptation field   涓嶄负0鍒欒鏄庨渶瑕佽嚜閫傚簲璋冩暣瀛楁
	if(chAdaptation_field_exist)
	{
		//OUTPUTSTRING("check has adaptation len is 1 + %d\n",GetCharContent(pMovingTsPointer));
		pMovingTsPointer += (GetCharContent(pMovingTsPointer)+1);//adaptation_field_length    鎰忔�濇槸褰撹嚜閫傚簲璋冩暣瀛樺湪鍒欐湁绗簲涓瓧鑺傚仛adaptation_field_length???
		if(pMovingTsPointer >= pEndTsPointer)
		{
			return -5;
		}
	}

	lHandleRet = 0;

	ptslostpkt->m_lGenerateLost = 0;
	ptslostpkt->m_lLostPktPid = shPid;
	ptslostpkt->m_lLostPktType = 0;


	/*
	PAT琛ㄦ惡甯︿互涓嬩俊鎭�:

	TS娴両D			transport_stream_id				璇D鏍囧織鍞竴鐨勬祦ID
	鑺傜洰棰戦亾鍙�		program_number					璇ュ彿鐮佹爣蹇楋即锛虫祦涓殑涓�涓閬擄紝璇ラ閬撳彲浠ュ寘鍚緢澶氱殑鑺傜洰(鍗冲彲浠ュ寘鍚涓猇ideo PID鍜孉udio PID)
	PMT鐨凱ID		program_map_PID					琛ㄧず鏈閬撲娇鐢ㄥ摢涓狿ID鍋氫负PMT鐨凱ID,鍥犱负鍙互鏈夊緢澶氱殑棰戦亾,鍥犳DVB瑙勫畾PMT鐨凱ID鍙互鐢辩敤鎴疯嚜宸卞畾涔�

	*/
	if (0x00 == shPid)       //PAT  PAT琛ㄥ畾涔変簡褰撳墠TS娴佷腑鎵�鏈夌殑鑺傜洰锛屽叾PID涓�0x0000锛屽畠鏄疨SI鐨勬牴鑺傜偣锛岃鏌ュ鎵捐妭鐩繀椤讳粠PAT琛ㄥ紑濮嬫煡鎵俱�� PAT琛ㄦ湁涓や釜閲嶈鐨勫弬鏁帮紝鑺傜洰棰戦亾鍙穚rogram_numbe鍜孭MT鐨凱ID鍙稰MT鐨凱ID
	{
		//OUTPUTSTRING("is pat!!!\n");
		ptslostpkt->m_lLostPktType = PAT_LOST;
		pOrgContinuityCounter = &(pTsStorage->m_lPATcontinuitycounter);
		lHandleRet = HandlePSI(pMovingTsPointer,pEndTsPointer,chUnitStart,pTsStorage,PSI_PAT);
		*pDstDataLen = 0;
	}
	else
	{
		lPmtIndex = GetPmtIndex(shPid,pTsStorage);
		if (-1 != lPmtIndex)
		{
			//OUTPUTSTRING("is pmt!!!\n");
			ptslostpkt->m_lLostPktType = PMT_LOST;
			pOrgContinuityCounter = &(pTsStorage->m_atPMT[lPmtIndex].m_lPMTcontinuitycounter);
			pTsStorage->m_lCurHandlePmtIndex = lPmtIndex;
			lHandleRet = HandlePSI(pMovingTsPointer,pEndTsPointer,chUnitStart,pTsStorage,PSI_PMT);

			*pDstDataLen = 0;
		}
		else//check if video or audio pes
		{
			lEsIndex = GetEsIndex(shPid,pTsStorage,&lPmtIndex);
			if (-1 != lEsIndex)
			{
				//OUTPUTSTRING("is PES!!!\n");
				ptslostpkt->m_lLostPktType = PES_LOST;
				ptslostpkt->m_lLostPktStreamType = pTsStorage->m_atPMT[lPmtIndex].m_atES[lEsIndex].m_lStreamType;
				pOrgContinuityCounter = &(pTsStorage->m_atPMT[lPmtIndex].m_atES[lEsIndex].m_lEScontinuitycounter);
				pTsStorage->m_lCurHandlePmtIndex = lPmtIndex;
				pTsStorage->m_lCurHandleEsIndex = lEsIndex;
				lHandleRet = HandlePES(pMovingTsPointer,pEndTsPointer,chUnitStart,pTsStorage,
					pDstData,pDstDataLen,ptsparam);
			}
			else
			{
				*pDstDataLen = 0;
			}
		}
	}

	//check lostpkt
	if (ptslostpkt->m_lLostPktType)
	{
		//OUTPUTSTRING("ready 2 check lost pkt,pid is %d\n",ptslostpkt->m_lLostPktPid);
		lLostPackets = CheckIfLostPackets(pOrgContinuityCounter,chContinuityCounter,&lLostPktNum);
		if (lLostPackets)
		{
			ptslostpkt->m_lGenerateLost = 1;
			ptslostpkt->m_lLostPktNum = lLostPktNum;
		}
	}
	
	return lHandleRet;

}







void DestroyOneTsStreamDemux(IN long lHandle)
{
	TsStreamStorage *pTsStorage = (TsStreamStorage*)lHandle;
	if (pTsStorage)
		free(pTsStorage);
}

















