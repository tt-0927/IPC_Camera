#include <stdio.h>
#include <atomic>
#include <pthread.h>
#include "rtspServer_base.h"
#include "h265_server_subsession.h"
#include "h265_video_source.h"
#include "h264_server_subsession.h"
#include "h264_video_source.h"
#include "aac_audio_source.h"
#include "aac_server_subsession.h"
#include "mjpeg_server_subsession.h"
#include "G711aAudioStreamServerMediaSubsession.h"
#include "G711uAudioStreamServerMediaSubsession.h"
#include "G726AudioStreamServerMediaSubsession.h"

/*最大RTSP会话数*/
#define MAXRTSPNUM 8

typedef struct
{
	TaskScheduler *scheduler;						/*任务调度器*/
	RTSPServer *rtspServer;							/*RTSP服务器实例*/
	UsageEnvironment *usage_env;					/*运行环境*/
	ServerMediaSession *server_session[MAXRTSPNUM]; /*媒体会话数组*/
	EventTriggerId controlTriggerId;				/*跨线程控制事件ID*/
	int nUse;										/*已使用的会话数*/
	int port;										/*服务端口*/
	volatile char m_quit;							/*退出标志*/
	std::atomic<char> m_aQuit{0};
	pthread_t eventLoopTid;							/*事件循环线程ID*/
	pthread_attr_t eventThattr;
	pthread_t printErrorTid;						/*错误打印线程ID*/
	int threadCreated;								/*线程创建标志*/
} Rtsp_Server_Info_t;

typedef struct
{
	Rtsp_Server_Info_t *pServerInfo;				/*RTSP服务器信息*/
	char streamName[STREAM_NAME_MAX];				/*待销毁的媒体会话名称*/
	int result;										/*事件线程执行结果*/
	bool done;										/*事件线程完成标志*/
	pthread_mutex_t mutex;							/*同步事件线程执行结果*/
	pthread_cond_t cond;								/*同步事件线程执行结果*/
} Rtsp_Control_Task_t;

/*函数前向声明*/
static void *doEvenLoopThread(void *argv);
static void *doPrintError(void *argv);
static void handleRtspControlEvent(void *clientData);
// static int findstreamName(Rtsp_Server_Info_t *pServerInfo, const char *streamName);
// static int cleanServerMediaSession(Rtsp_Server_Info_t *pServerInfo, const char *streamName);
// static int findServerMediaSessionUse(Rtsp_Server_Info_t *pServerInfo);
// static int cleanAllServerMediaSession(Rtsp_Server_Info_t *pServerInfo);

static int findServerMediaSessionUse(Rtsp_Server_Info_t *pServerInfo)
{
	int i = 0;
	if (pServerInfo == NULL)
	{
		live_log("findUseSocket is NULL");
		return -1;
	}
	for (i = 0; i < MAXRTSPNUM; i++)
	{
		if (pServerInfo->server_session[i] == NULL)
		{
			pServerInfo->nUse++;
			return i;
		}
	}
	return -1;
}

static int cleanServerMediaSession(Rtsp_Server_Info_t *pServerInfo, const char *streamName)
{
	int i = 0;
	if (pServerInfo == NULL || streamName == NULL)
	{
		live_log("findUseSocket is NULL");
		return -1;
	}
	for (i = 0; i < MAXRTSPNUM; i++)
	{
		if (pServerInfo->server_session[i] != NULL && strcmp(pServerInfo->server_session[i]->streamName(), streamName) == 0)
		{
			live_log("destory streamName: %s pServerInfo->server_session[i]: %p", streamName, pServerInfo->server_session[i]);
			pServerInfo->rtspServer->deleteServerMediaSession(pServerInfo->server_session[i]);
			pServerInfo->server_session[i] = NULL;
			pServerInfo->nUse--;
			return 0;
		}
	}
	return -1;
}

static int cleanAllServerMediaSession(Rtsp_Server_Info_t *pServerInfo)
{
	int i = 0;
	if (pServerInfo == NULL)
	{
		live_log("findUseSocket is NULL");
		return -1;
	}
	for (i = 0; i < MAXRTSPNUM; i++)
	{
		if (pServerInfo->server_session[i] != NULL)
		{
			live_log("pServerInfo->server_session[i]: %p", pServerInfo->server_session[i]);
			pServerInfo->rtspServer->deleteServerMediaSession(pServerInfo->server_session[i]);
			pServerInfo->server_session[i] = NULL;
			pServerInfo->nUse--;
			return 0;
		}
	}
	return -1;
}

static void handleRtspControlEvent(void *clientData)
{
	Rtsp_Control_Task_t *pTask = (Rtsp_Control_Task_t *)clientData;
	if (pTask == NULL)
	{
		return;
	}

	int result = cleanServerMediaSession(pTask->pServerInfo, pTask->streamName);
	pthread_mutex_lock(&pTask->mutex);
	pTask->result = result;
	pTask->done = true;
	pthread_cond_signal(&pTask->cond);
	pthread_mutex_unlock(&pTask->mutex);
}

static int findstreamName(Rtsp_Server_Info_t *pServerInfo, const char *streamName)
{
	int i = 0;
	if (pServerInfo == NULL && streamName == NULL)
	{
		live_log("findUseSocket is NULL");
		return -1;
	}
	for (i = 0; i < MAXRTSPNUM; i++)
	{
		if (pServerInfo->server_session[i] != NULL && strcmp(pServerInfo->server_session[i]->streamName(), streamName) == 0)
		{
			return i;
		}
	}
	return -1;
}

RtSpServerHandle_t rtsp_server_init(int port, int nRegister, const char *pUser, const char *pPassworld, int nAuthAlgorithm, int nDscp)
{
	int error = 0;
	UserAuthenticationDatabase *authDB = NULL;
	Authenticator::AuthAlgorithm algorithm;

	Rtsp_Server_Info_t *pServeInfo = (Rtsp_Server_Info_t *)malloc(sizeof(Rtsp_Server_Info_t));
	if (pServeInfo == NULL)
	{
		error = 1;
		live_log("tpServeInfo is NULL");
		goto EXIT_INIT;
	}
	memset(pServeInfo, 0, sizeof(Rtsp_Server_Info_t));

	OutPacketBuffer::maxSize = REV_BUF_SIZE; // 输出一桢的最大值，默认不超过150000

	pServeInfo->scheduler = BasicTaskScheduler::createNew();
	if (pServeInfo->scheduler == NULL)
	{
		error = 1;
		live_log("pServeInfo->scheduler is NULL");
		goto EXIT_INIT;
	}

	pServeInfo->usage_env = BasicUsageEnvironment::createNew(*(pServeInfo->scheduler));
	if (pServeInfo->usage_env == NULL)
	{
		error = 1;
		live_log("pServeInfo->usage_env is NULL");
		goto EXIT_INIT;
	}

	/* 启用鉴权 */
    if (nRegister && pUser && pPassworld)
    {
		live_log("鉴权已启用 (Authentication Enabled), User: %s", pUser);
        char const* realm = "Itc Streaming Server";
        authDB = new UserAuthenticationDatabase(realm, False);
        authDB->addUserRecord(pUser, pPassworld);
    }
	else
	{
		live_log("注意: 鉴权未启用. nRegister=%d", nRegister);
	}

	algorithm = (Authenticator::AuthAlgorithm)nAuthAlgorithm;
    pServeInfo->port = port;
	pServeInfo->rtspServer = RTSPServer::createNew(*(pServeInfo->usage_env), nDscp, port, authDB, 65, algorithm);

	if (pServeInfo->rtspServer == NULL)
	{
		error = 1;
		pServeInfo->usage_env->reportBackgroundError();
		live_log("pServeInfo->RTSPServer is NULL");
		goto EXIT_INIT;
	}

	pServeInfo->controlTriggerId = pServeInfo->scheduler->createEventTrigger(handleRtspControlEvent);
	if (pServeInfo->controlTriggerId == 0)
	{
		error = 1;
		live_log("createEventTrigger failed");
		goto EXIT_INIT;
	}

	error = pthread_attr_init(&pServeInfo->eventThattr);
	if (error != 0) {

		live_log("pthread_attr_init 失败: %d", error);
		error = 1;
		goto EXIT_INIT;
	}
	error = pthread_attr_setdetachstate(&pServeInfo->eventThattr, PTHREAD_CREATE_DETACHED);
	if (error != 0) 
	{
		live_log("pthread_attr_setdetachstate 失败: %d", error);
		pthread_attr_destroy(&pServeInfo->eventThattr); 
		error = 1;
		goto EXIT_INIT;
	}

	// Run loop
	error = pthread_create(&pServeInfo->eventLoopTid, NULL, doEvenLoopThread, pServeInfo);
	if (error != 0)
	{
		live_log("pthread_create doEvenLoopThread failed: %d", error);
		pthread_attr_destroy(&pServeInfo->eventThattr); 
		error = 1;
		goto EXIT_INIT;
	}

	error = pthread_create(&pServeInfo->printErrorTid, NULL, doPrintError, pServeInfo);
	if (error != 0)
	{
		live_log("pthread_create doPrintError failed: %d", error);
		error = 1;
		/*需要先等待已创建的事件循环线程结束*/
		pServeInfo->m_quit = 1;
		pServeInfo->m_aQuit.store(1, std::memory_order_seq_cst);
		pthread_attr_destroy(&pServeInfo->eventThattr);
		//pthread_join(pServeInfo->eventLoopTid, NULL);
		goto EXIT_INIT;
	}
	pthread_attr_destroy(&pServeInfo->eventThattr);

	/*标记线程已创建*/
	pServeInfo->threadCreated = 1;

EXIT_INIT:
	if (error == 1)
	{
		if (pServeInfo->usage_env)
		{
			pServeInfo->usage_env->reclaim();
			pServeInfo->usage_env = NULL;
		}
		if (pServeInfo->scheduler)
		{
			delete pServeInfo->scheduler;
			pServeInfo->scheduler = NULL;
		}
		if (pServeInfo)
		{
			free(pServeInfo);
			pServeInfo = NULL;
		}
		return NULL;
	}
	return pServeInfo;
}

int rtsp_server_create(RtSpServerHandle_t pHandle, Rtsp_Create_Info_t *pCreatServerInfo)
{
	// Add live stream
	Rtsp_Server_Info_t *pServerInfo = (Rtsp_Server_Info_t *)pHandle;
	Video_Source_Info_t stuVideoSourceInfo;
	Audio_Source_Info_t stuAudioSourceInfo;
	memset(&stuVideoSourceInfo, 0x0, sizeof(Video_Source_Info_t));
	memset(&stuAudioSourceInfo, 0x0, sizeof(Audio_Source_Info_t));

	int nUse = 0;
	if (pServerInfo == NULL || pCreatServerInfo->clientFun == NULL || pCreatServerInfo->dataGetfun == NULL)
	{
		live_log("rtsp_server_create is NULL");
		return -1;
	}

	stuAudioSourceInfo.clientFun = pCreatServerInfo->clientFun;
	stuAudioSourceInfo.dataGetfun = pCreatServerInfo->dataGetfun;
	stuAudioSourceInfo.audioindex = pCreatServerInfo->Audioindex;
	stuAudioSourceInfo.samplingFreqIndex = pCreatServerInfo->nAuidoSamplingFreqIndex;
	stuAudioSourceInfo.bitWidth = pCreatServerInfo->nAudioBitWidth;
	stuAudioSourceInfo.channel = pCreatServerInfo->nAudioChannel;
	stuAudioSourceInfo.outPacketBufferSize = pCreatServerInfo->audioOutPacketBufferSize;
	memcpy(stuAudioSourceInfo.streamName, pCreatServerInfo->streamName, STREAM_NAME_MAX);

	stuVideoSourceInfo.clientFun = pCreatServerInfo->clientFun;
	stuVideoSourceInfo.dataGetfun = pCreatServerInfo->dataGetfun;
	stuVideoSourceInfo.videoindex = pCreatServerInfo->Videoindex;
	stuVideoSourceInfo.outPacketBufferSize = pCreatServerInfo->outPacketBufferSize;
	memcpy(stuVideoSourceInfo.streamName, pCreatServerInfo->streamName, STREAM_NAME_MAX);

	/* 最大4路 拓展参数为最大client个数*/
	rtsp_setclient_maxNum(pHandle, pCreatServerInfo->streamName, pCreatServerInfo->param1);
	/* h264 */
	if (pCreatServerInfo->nProtolType == RTSP_FRAMEPROTOL_H264)
	{
		nUse = findServerMediaSessionUse(pServerInfo);
		live_log("findServerMediaSession index :%d", nUse);
		if (nUse == -1)
		{
			live_log("findServerMediaSession is fail");
			return -1;
		}

		pServerInfo->server_session[nUse] = ServerMediaSession::createNew(*(pServerInfo->usage_env), stuVideoSourceInfo.streamName, 0, "Session from MainStream");

		if (stuAudioSourceInfo.audioindex != NULL)
		{
			stuVideoSourceInfo.nAudio = 1;
		}
		else
		{
			stuVideoSourceInfo.nAudio = 0;
		}
		live_log("stuVideoSourceInfo.nAudio:%d", stuVideoSourceInfo.nAudio);
		pServerInfo->server_session[nUse]->addSubsession(H264_Server_Subsession::createNew(*(pServerInfo->usage_env), stuVideoSourceInfo));
		if (stuAudioSourceInfo.audioindex != NULL && stuVideoSourceInfo.nAudio)
		{
			if (pCreatServerInfo->nAudioType == 0)
			{
				pServerInfo->server_session[nUse]->addSubsession(aacAudioServerMediaSubsession::createNew(*(pServerInfo->usage_env), 1, stuAudioSourceInfo));
			}
			else if (pCreatServerInfo->nAudioType == 1)
			{
				pServerInfo->server_session[nUse]->addSubsession(G711uAudioStreamServerMediaSubsession::createNew(*(pServerInfo->usage_env), 1, stuAudioSourceInfo));
			}
			else if (pCreatServerInfo->nAudioType == 2)
			{
				pServerInfo->server_session[nUse]->addSubsession(G711aAudioStreamServerMediaSubsession::createNew(*(pServerInfo->usage_env), 1, stuAudioSourceInfo));
			}
			else if (pCreatServerInfo->nAudioType == 3)
			{
				pServerInfo->server_session[nUse]->addSubsession(G726AudioStreamServerMediaSubsession::createNew(*(pServerInfo->usage_env), 1, stuAudioSourceInfo));
			}
		}

		pServerInfo->rtspServer->addServerMediaSession(pServerInfo->server_session[nUse]);

		char *url = pServerInfo->rtspServer->rtspURL(pServerInfo->server_session[nUse]);
		*(pServerInfo->usage_env) << "using url: \"" << url << "\"\n";
		delete[] url;
	}
	else if (pCreatServerInfo->nProtolType == RTSP_FRAMEPROTOL_H265)
	{
		/* h265 */
		nUse = findServerMediaSessionUse(pServerInfo);
		live_log("findServerMediaSession index :%d", nUse);
		if (nUse == -1)
		{

			live_log("findServerMediaSession is fail");
			return -1;
		}

		pServerInfo->server_session[nUse] = ServerMediaSession::createNew(*(pServerInfo->usage_env), stuVideoSourceInfo.streamName, 0, "Session from MainStream265");

		if (stuAudioSourceInfo.audioindex != NULL)
		{
			stuVideoSourceInfo.nAudio = 1;
		}
		else
		{
			stuVideoSourceInfo.nAudio = 0;
		}
		live_log("stuVideoSourceInfo.nAudio:%d", stuVideoSourceInfo.nAudio);
		pServerInfo->server_session[nUse]->addSubsession(H265_Server_Subsession::createNew(*(pServerInfo->usage_env), stuVideoSourceInfo));
		if (stuAudioSourceInfo.audioindex != NULL && stuVideoSourceInfo.nAudio)
		{
			if (pCreatServerInfo->nAudioType == 0)
			{
				pServerInfo->server_session[nUse]->addSubsession(aacAudioServerMediaSubsession::createNew(*(pServerInfo->usage_env), 1, stuAudioSourceInfo));
			}
			else if (pCreatServerInfo->nAudioType == 1)
			{
				pServerInfo->server_session[nUse]->addSubsession(G711uAudioStreamServerMediaSubsession::createNew(*(pServerInfo->usage_env), 1, stuAudioSourceInfo));
			}
			else if (pCreatServerInfo->nAudioType == 2)
			{
				pServerInfo->server_session[nUse]->addSubsession(G711aAudioStreamServerMediaSubsession::createNew(*(pServerInfo->usage_env), 1, stuAudioSourceInfo));
			}
			else if (pCreatServerInfo->nAudioType == 3)
			{
				pServerInfo->server_session[nUse]->addSubsession(G726AudioStreamServerMediaSubsession::createNew(*(pServerInfo->usage_env), 1, stuAudioSourceInfo));
			}
		}

		pServerInfo->rtspServer->addServerMediaSession(pServerInfo->server_session[nUse]);

		char *url265 = pServerInfo->rtspServer->rtspURL(pServerInfo->server_session[nUse]);
		*(pServerInfo->usage_env) << "using url265: \"" << url265 << "\"\n";

		delete[] url265;
	}
	else if (pCreatServerInfo->nProtolType == RTSP_FRAMEPROTOL_MJPEG)
	{

		/* mjpeg */
		nUse = findServerMediaSessionUse(pServerInfo);
		live_log("findServerMediaSession index :%d", nUse);
		if (nUse == -1)
		{
			live_log("findServerMediaSession is fail");
			return -1;
		}

		pServerInfo->server_session[nUse] = ServerMediaSession::createNew(*(pServerInfo->usage_env), stuVideoSourceInfo.streamName, 0, "Session from MainStreamMjpeg");

		if (stuAudioSourceInfo.audioindex != NULL)
		{
			stuVideoSourceInfo.nAudio = 1;
		}
		else
		{
			stuVideoSourceInfo.nAudio = 0;
		}
		live_log("mjpeg_source_info.nAudio:%d", stuVideoSourceInfo.nAudio);
		pServerInfo->server_session[nUse]->addSubsession(Mjpeg_Server_Subsession::createNew(*(pServerInfo->usage_env), stuVideoSourceInfo));
		if (stuAudioSourceInfo.audioindex != NULL && stuVideoSourceInfo.nAudio)
		{
			if (pCreatServerInfo->nAudioType == 0)
			{
				pServerInfo->server_session[nUse]->addSubsession(aacAudioServerMediaSubsession::createNew(*(pServerInfo->usage_env), 1, stuAudioSourceInfo));
			}
			else if (pCreatServerInfo->nAudioType == 1)
			{
				pServerInfo->server_session[nUse]->addSubsession(G711uAudioStreamServerMediaSubsession::createNew(*(pServerInfo->usage_env), 1, stuAudioSourceInfo));
			}
			else if (pCreatServerInfo->nAudioType == 2)
			{
				pServerInfo->server_session[nUse]->addSubsession(G711aAudioStreamServerMediaSubsession::createNew(*(pServerInfo->usage_env), 1, stuAudioSourceInfo));
			}
			else if (pCreatServerInfo->nAudioType == 3)
			{
				pServerInfo->server_session[nUse]->addSubsession(G726AudioStreamServerMediaSubsession::createNew(*(pServerInfo->usage_env), 1, stuAudioSourceInfo));
			}
		}
		pServerInfo->rtspServer->addServerMediaSession(pServerInfo->server_session[nUse]);

		char *urlMjpeg = pServerInfo->rtspServer->rtspURL(pServerInfo->server_session[nUse]);
		*(pServerInfo->usage_env) << "using url265: \"" << urlMjpeg << "\"\n";

		delete[] urlMjpeg;
	}

	return 0;
}

int rtsp_server_destory(RtSpServerHandle_t pHandle, const char *streamName)
{
	Rtsp_Server_Info_t *pServerInfo = (Rtsp_Server_Info_t *)pHandle;
	if (pServerInfo == NULL || streamName == NULL)
	{
		live_log("rtsp_server_destory is NULL");
		return -1;
	}

	if (pthread_equal(pthread_self(), pServerInfo->eventLoopTid) || pServerInfo->controlTriggerId == 0)
	{
		return cleanServerMediaSession(pServerInfo, streamName);
	}

	Rtsp_Control_Task_t task;
	memset(&task, 0, sizeof(task));
	task.pServerInfo = pServerInfo;
	task.result = -1;
	snprintf(task.streamName, sizeof(task.streamName), "%s", streamName);
	pthread_mutex_init(&task.mutex, NULL);
	pthread_cond_init(&task.cond, NULL);

	pthread_mutex_lock(&task.mutex);
	pServerInfo->scheduler->triggerEvent(pServerInfo->controlTriggerId, &task);
	while (!task.done)
	{
		pthread_cond_wait(&task.cond, &task.mutex);
	}
	int result = task.result;
	pthread_mutex_unlock(&task.mutex);

	pthread_cond_destroy(&task.cond);
	pthread_mutex_destroy(&task.mutex);
	return result;
}

int rtsp_server_unInit(RtSpServerHandle_t pRtspHandle)
{
	Rtsp_Server_Info_t *pServerInfo = (Rtsp_Server_Info_t *)pRtspHandle;
	if (pServerInfo == NULL)
	{
		live_log("rtsp_server_unInit is NULL");
		return -1;
	}

	/*设置退出标志*/
	pServerInfo->m_quit = 1;
	pServerInfo->m_aQuit.store(1, std::memory_order_seq_cst);

	/*等待线程结束*/
	if (pServerInfo->threadCreated)
	{
		live_log("Waiting for threads to finish...");

		// /*等待事件循环线程结束*/
		// int ret = pthread_join(pServerInfo->eventLoopTid, NULL);
		// if (ret != 0)
		// {
		// 	live_log("pthread_join eventLoopTid failed: %d", ret);
		// }
		// else
		// {
		// 	live_log("Event loop thread finished successfully");
		// }

		/*等待错误打印线程结束*/
		int ret = pthread_join(pServerInfo->printErrorTid, NULL);
		if (ret != 0)
		{
			live_log("pthread_join printErrorTid failed: %d", ret);
		}
		else
		{
			live_log("Print error thread finished successfully");
		}
	}

	/*清理所有媒体会话*/
	cleanAllServerMediaSession(pServerInfo);
	if (pServerInfo->scheduler && pServerInfo->controlTriggerId != 0)
	{
		pServerInfo->scheduler->deleteEventTrigger(pServerInfo->controlTriggerId);
		pServerInfo->controlTriggerId = 0;
	}

	/*关闭RTSP服务器*/
    if (pServerInfo->rtspServer)
    {
        Medium::close(pServerInfo->rtspServer);
        pServerInfo->rtspServer = NULL;
    }

	/*清理运行环境*/
	if (pServerInfo->usage_env)
	{
		pServerInfo->usage_env->reclaim();
		pServerInfo->usage_env = NULL;
	}

	/*清理调度器*/
	if (pServerInfo->scheduler)
	{
		delete pServerInfo->scheduler;
		pServerInfo->scheduler = NULL;
	}

	/*释放服务器信息结构体*/
	free(pRtspHandle);
	pRtspHandle = nullptr;

	return 0;
}

int rtsp_getclient_info(RtSpServerHandle_t pHandle, const char *streamName, Rtsp_Client_Info_t *pClientInfo)
{
	Rtsp_Server_Info_t *pServerInfo = (Rtsp_Server_Info_t *)pHandle;
	int nUse = 0;
	int i = 0;
	if (pServerInfo == NULL || pClientInfo == NULL)
	{
		live_log("rtsp_server_destory is NULL");
		return -1;
	}
	nUse = findstreamName(pServerInfo, streamName);
	if (nUse == -1)
	{
		live_log("find streamName is fail");
		return -1;
	}
	pClientInfo->nNumClient = pServerInfo->server_session[nUse]->referenceCount();
	for (i = 0; i < pClientInfo->nNumClient; i++)
	{
		strcpy(pClientInfo->ip[i], pServerInfo->server_session[nUse]->fClientInfo[i].ip);
	}
	return 0;
}

int rtsp_setclient_maxNum(RtSpServerHandle_t pHandle, const char *streamName, int maxNum)
{
	Rtsp_Server_Info_t *pServerInfo = (Rtsp_Server_Info_t *)pHandle;
	int nUse = 0;
	if (pServerInfo == NULL || maxNum > CLIENTMAX)
	{
		live_log("rtsp_server_destory is NULL");
		return -1;
	}
	nUse = findstreamName(pServerInfo, streamName);
	if (nUse == -1)
	{
		live_log("find streamName is fail");
		return -1;
	}
	pServerInfo->server_session[nUse]->fReferenceMax = maxNum;
	return 0;
}

int printfErrMesege(RtSpServerHandle_t pHandle)
{
	Rtsp_Server_Info_t *pServerInfo = (Rtsp_Server_Info_t *)pHandle;
	if (pServerInfo == NULL || pServerInfo->usage_env == NULL)
	{
		live_log("rtsp_server_destory is NULL");
		return -1;
	}

	if (strcmp(pServerInfo->usage_env->getResultMsg(), ""))
	{
		live_log("rtsp server base cpp: port:%d err:%s", pServerInfo->port, pServerInfo->usage_env->getResultMsg());
		pServerInfo->usage_env->setResultMsg("");
	}
	return 0;
}

int set_handshakeAuth_callback(void *handle, HandshakeAuthCallback callback)
{
	if (handle == NULL)
	{
		return -1;
	}
	
    if (callback == NULL)
    {
        live_log("无效的握手认证回调函数");
        return -1;
    }

	Rtsp_Server_Info_t *pServeInfo = (Rtsp_Server_Info_t *)handle;
	pServeInfo->rtspServer->handshakeAuthCallback = callback;
    live_log("设置握手认证回调成功");
	return 0;
}

int close_connection(void *handle, char *targetIP)
{
	if (handle == NULL || targetIP == NULL)
	{
		return -1;
	}
	Rtsp_Server_Info_t *pServeInfo = (Rtsp_Server_Info_t *)handle;
	pServeInfo->rtspServer->closeConnectionsByIP(targetIP);
    live_log("关闭目标IP[%s]连接", targetIP);
    return 0;
}

/**
 * @brief   : RTSP事件循环线程
 * @param   : argv - 线程参数(Rtsp_Server_Info_t指针)
 * @return  : void指针
 */
static void *doEvenLoopThread(void *argv)
{
	Rtsp_Server_Info_t *pServeInfo = (Rtsp_Server_Info_t *)argv;
	if (pServeInfo == NULL)
	{
		live_log("usage_env == NULL is NULL");
		return NULL;
	}

	/*运行事件循环直到收到退出信号*/
	while (true)
	{
		if (pServeInfo->m_aQuit.load(std::memory_order_seq_cst) != 0) 
		{
			live_log("退出事件循环线程");
			break;  // 退出循环
		}

		pServeInfo->usage_env->taskScheduler().doEvent();
	}
	
	return NULL;
}

/**
 * @brief   : 错误信息打印线程
 * @param   : argv - 线程参数(Rtsp_Server_Info_t指针)
 * @return  : void指针
 */
static void *doPrintError(void *argv)
{
	Rtsp_Server_Info_t *pServeInfo = (Rtsp_Server_Info_t *)argv;
	if (pServeInfo == NULL)
	{
		live_log("doPrintError: pServeInfo is NULL");
		return NULL;
	}

	while (pServeInfo->m_quit == 0)
	{
		sleep(1);
		printfErrMesege(pServeInfo);
	}

	return NULL;
}
