#ifndef __TS_PROCESS_H__
#define __TS_PROCESS_H__

#define IN
#define OUT
#define INANDOUT

#define PSI_PAT 1
#define PSI_PMT 2

typedef struct tagTsParam
{
	long m_lDstDataType;
	unsigned long long m_lPts;
	unsigned long long m_lDts;
	//wait for extend
}TsParam;



typedef struct tagTsLostPkt
{
	long m_lGenerateLost;
#define PAT_LOST 1
#define PMT_LOST 2
#define PES_LOST 3
	long m_lLostPktType;
	long m_lLostPktPid;
	long m_lLostPktStreamType;//for pes

	long m_lLostPktNum;
}TsLostPkt;

















typedef struct tagEsParam
{
	long m_lStreamType;
	long m_lESPID;
#define ES_CACHE_MAX_LEN (10*1024*1024)
	char m_achESCache[2][ES_CACHE_MAX_LEN];
	long m_lESoffset[2];
	long m_lESTotalLength[2];
	long m_lESHeaderLength[2];
	unsigned long long m_alPts[2];
	unsigned long long m_alDts[2];

	long m_lCurActivePointer;
	long m_lEScontinuitycounter;
}EsParam;


typedef struct tagPMT
{
	unsigned short m_shPMTpid;
#define PMT_MAX_LEN 512
	char m_achPMTcache[PMT_MAX_LEN];
	long m_lPMToffset;
	long m_lPMTreallength;

#define ESTYPE_MAX_NUM 2
	EsParam m_atES[ESTYPE_MAX_NUM];
	long m_lESOffset;

	//long m_lHasGetData;
	long m_lPMTcontinuitycounter;
}PMT;






typedef struct tagTsStreamStorage
{
//#define NO_PAT 0
//#define HAS_PAT 1
//	long m_lStateMachine;
#define PAT_MAX_LEN 512
	char m_achPATcache[PAT_MAX_LEN];
	long m_lPAToffset;
	long m_lPATreallength;
#define PMT_MAX_NUM 1
	PMT m_atPMT[PMT_MAX_NUM];
	long m_lPMToffset;

	long m_lCurHandlePmtIndex;
	long m_lCurHandleEsIndex;
	long m_lPATcontinuitycounter;
}TsStreamStorage;











//////////////////////////////////////////////////





long CreateOneTsStreamDemux();
long DemuxTsStream(IN long lHandle,IN char *pTsStream,OUT char *pDstData, INANDOUT long *pDstDataLen,OUT TsParam *ptsparam,OUT TsLostPkt *ptslostpkt);
void DestroyOneTsStreamDemux(IN long lHandle);




#endif