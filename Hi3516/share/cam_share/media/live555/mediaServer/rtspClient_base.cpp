#include <pthread.h>
#include<string.h>
#include "rtspClient_subsession.h"
#include "rtspServer_base.h"

typedef struct
{

	TaskScheduler* scheduler;
	RTSPClient* rtspClient;
	UsageEnvironment* usage_env;
	rtsp_ourregister_t registerInfo;
	Rtsp_Status_t status;
	 pthread_t errmessege_tid;
	 pthread_t loop_tid;
	 pthread_t connect_tid;
	int isreconect;
	char url[256];
	int nUse;
	int port;
	char m_quit;
	int m_errQuit;

}Rtsp_ClientStream_Info_t;
int rtsp_client_connect(RtSpClientHandle_t clienthandle)
{

	Rtsp_ClientStream_Info_t* pClientInfo = (Rtsp_ClientStream_Info_t*)clienthandle;
	if(pClientInfo == NULL)
	{

		printf("reconnect is fail\n");
		return -1;
	}
	pClientInfo->rtspClient =  openURL(*(pClientInfo->usage_env), NULL, pClientInfo->url,  &(pClientInfo->registerInfo));
	 if(pClientInfo->rtspClient == NULL)
	 {
		 printf("pClientInfo->rtspClient is NULL\n");
		 return -1;
	 }
	 return 0;
}
static int rtspStateCallback(Rtsp_ClientStream_State_t * param, void* rtspdata)
{
	if(param == NULL)
	{
		printf("\033[33m""stateCallback is wrong\n");
		return -1;
	}
	Rtsp_ClientStream_Info_t* pRtspClient_handle = (Rtsp_ClientStream_Info_t*)param->param;

	if(NULL == pRtspClient_handle)
	{
		printf( "Rtsp_ClientStream_Info_t ==NULL \n");
		return -1;
	}
	pRtspClient_handle->rtspClient = (RTSPClient*)rtspdata;
	pRtspClient_handle->status = param->status;
	if(param->status == RTSPCLIENT_STOP)
	{

	}

	return 0;
}
static void * doClientEvenLoopThread(void *argv)
{
	Rtsp_ClientStream_Info_t* pClientInfo = (Rtsp_ClientStream_Info_t*)argv;
	if(pClientInfo == NULL)
	{
		printf("usage_env == NULL is NULL\n");
		return NULL;
	}

	pClientInfo->usage_env->taskScheduler().doEventLoop(&(pClientInfo->m_quit));

	pthread_exit(0);
}
int printClintfErrMesege(RtSpClientHandle_t pHandle)
{
	Rtsp_ClientStream_Info_t *pClientInfo = (Rtsp_ClientStream_Info_t *)pHandle;
	 if(pClientInfo == NULL || pClientInfo->usage_env == NULL)
	 {
		 printf("printClintfErrMesege is NULL\n");
		 return -1;
	 }
	 if(strcmp(pClientInfo->usage_env->getResultMsg(), ""))
	 {
		 printf("\033[32m""err:%s\n""\033[0m",  pClientInfo->usage_env->getResultMsg());
		 pClientInfo->usage_env->setResultMsg("");
	 }
	 return 0;
}
static void * doClientPrintError(void *argv)
{
	Rtsp_ClientStream_Info_t* pClientInfo = (Rtsp_ClientStream_Info_t*)argv;
	if(pClientInfo == NULL)
	{
		printf(" pClientInfo usage_env == NULL is NULL\n");
		return NULL;
	}
	while(pClientInfo->m_errQuit == 0)
	{
		sleep(1);
		printClintfErrMesege(pClientInfo);
	}
	pthread_exit(0);
}
static void * doClientConnect(void *argv)
{
	Rtsp_ClientStream_Info_t* pClientInfo = (Rtsp_ClientStream_Info_t*)argv;
	if(pClientInfo == NULL)
	{
		printf(" pClientInfo usage_env == NULL is NULL\n");
		return NULL;
	}
	rtsp_client_connect(pClientInfo);
	pthread_exit(0);
}

RtSpClientHandle_t rtsp_client_start(const char *url,ClientStreamStatus  stateCallback, FrameCallBack frameCall, void* param,
		Rtsp_Inparam * Inparam) //客户端
{
	int error = 0;
	rtsp_ourregister_t registerInfo = {0};
	if(url == NULL || stateCallback== NULL || frameCall == NULL || Inparam == NULL)
	{
		error = 1;
		printf("rtsp_client_start param is NULL url:%p stateCallback:%p frameCall:%p\n", url, stateCallback, frameCall);
		return NULL;
	}


	Rtsp_ClientStream_Info_t* pClientInfo = (Rtsp_ClientStream_Info_t*)malloc(sizeof(Rtsp_ClientStream_Info_t));
	if(pClientInfo == NULL)
	{
		error = 1;
		printf("pClientInfo is NULL\n");
		goto EXIT_INIT;
	}
	memset(pClientInfo, 0, sizeof(Rtsp_ClientStream_Info_t));


	registerInfo.frameCall = frameCall;
	registerInfo.param = param;
	registerInfo.stateCallback = stateCallback;
	registerInfo.rtspStateCallback = rtspStateCallback;
	registerInfo.rtsp_param = pClientInfo;
	memcpy(&(registerInfo.Inparm), Inparam, sizeof(Rtsp_Inparam));
	memcpy(&(pClientInfo->registerInfo), &registerInfo, sizeof(rtsp_ourregister_t));
	strcpy(pClientInfo->url, url);
	//OutPacketBuffer::maxSize = 1000000;

	pClientInfo->scheduler = BasicTaskScheduler::createNew();
	 if(pClientInfo->scheduler == NULL)
	 {
		 error = 1;
		 printf("pClientInfo->scheduler is NULL\n");
		 goto EXIT_INIT;
	 }

	 pClientInfo->usage_env = BasicUsageEnvironment::createNew(*(pClientInfo->scheduler));
	 if(pClientInfo->usage_env == NULL)
	 {
		 error = 1;
		 printf("pClientInfo->usage_env is NULL\n");
		 goto EXIT_INIT;
	 }



		error = pthread_create(&pClientInfo->connect_tid, NULL, doClientConnect, pClientInfo);
		if(error != 0)
		{
			printf("pthread_create(&tid, NULL, doEvenLoopThread, pClientInfo); is fail\n");
			error = 1;
		}

		error = pthread_create(&pClientInfo->loop_tid, NULL, doClientEvenLoopThread, pClientInfo);
		if(error != 0)
		{
			printf("pthread_create(&tid, NULL, doEvenLoopThread, pClientInfo); is fail\n");
			error = 1;
		}
		error = pthread_create(&(pClientInfo->errmessege_tid), NULL, doClientPrintError, pClientInfo);
		if(error != 0)
		{
			printf("pthread_create(&tid, NULL, doEvenLoopThread, pClientInfo); is fail\n");
			error = 1;
		}


EXIT_INIT:
	  if(error == 1)
	  {
		  if(pClientInfo->usage_env)
		  {
			  pClientInfo->usage_env->reclaim();
			  pClientInfo->usage_env = NULL;
		  }
		  if(pClientInfo->scheduler)
		  {
			  delete pClientInfo->scheduler;
			  pClientInfo->scheduler = NULL;
		  }
		  if(pClientInfo)
		  {
			  free(pClientInfo);
			  pClientInfo = NULL;
		  }
		  return NULL;
	  }
	  return pClientInfo;


}
int rtsp_client_reConnect(RtSpClientHandle_t clienthandle)
{
	Rtsp_ClientStream_Info_t* pClientInfo = (Rtsp_ClientStream_Info_t*)clienthandle;
	int error = 0;
	if(pClientInfo != NULL)
	{

		pClientInfo->m_quit = 1;
		pthread_join(pClientInfo->connect_tid, NULL);
		pthread_join(pClientInfo->loop_tid, NULL);
		if(pClientInfo->rtspClient)
		{
			shutdownStream(pClientInfo->rtspClient,0);
			pClientInfo->rtspClient = NULL;
		}


		pClientInfo->m_quit = 0;
		error = pthread_create(&pClientInfo->connect_tid, NULL, doClientConnect, pClientInfo);
		if(error != 0)
		{
			printf("pthread_create(&tid, NULL, doEvenLoopThread, pClientInfo); is fail\n");
			error = 1;
		}

		error = pthread_create(&pClientInfo->loop_tid, NULL, doClientEvenLoopThread, pClientInfo);
		if(error != 0)
		{
			printf("pthread_create(&tid, NULL, doEvenLoopThread, pClientInfo); is fail\n");
			error = 1;
		}
	}

	 return 0;
}


int rtsp_client_stop(RtSpClientHandle_t clienthandle)
{
	Rtsp_ClientStream_Info_t* pClientInfo = (Rtsp_ClientStream_Info_t*)clienthandle;
	if(pClientInfo == NULL)
	{

		printf("rtsp_client_stop is fail\n");

		return -1;
	}
	pClientInfo->m_quit = 1;
	pClientInfo->m_errQuit = 1;
	pthread_join(pClientInfo->connect_tid, NULL);
	pthread_join(pClientInfo->errmessege_tid, NULL);
	pthread_join(pClientInfo->loop_tid, NULL);
	if(pClientInfo->rtspClient)
	{
		shutdownStream(pClientInfo->rtspClient,0);
		pClientInfo->rtspClient = NULL;
	}

	  if(pClientInfo->usage_env)
	  {
		 pClientInfo->usage_env->reclaim();
		  pClientInfo->usage_env = NULL;
	  }
	  if(pClientInfo->scheduler)
	  {
		  delete pClientInfo->scheduler;
		  pClientInfo->scheduler = NULL;
	  }
	  if(pClientInfo)
	  {
		  free(pClientInfo);
		  pClientInfo = NULL;
	  }

	return 0;
}


