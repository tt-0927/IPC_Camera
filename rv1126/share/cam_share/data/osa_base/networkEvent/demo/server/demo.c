

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include "network_event.h"
#include "network_head.h"

network_Handle_t serverHandle;

network_Handle_t clientHandle;

char s_send_data[2048] = {"a"};

int demo_server_DealCmd(network_CallbackMsg_t* param)
{
	int nRet = 0;
	if(param == NULL || param->recvvalue == NULL || param->clientHandle == NULL)
	{
		printf("demo_server_DealCmd is fail param:%p\n", param);
		return -1;
	}

	if(param->Code == 30032)
	{
		return 0;
	}

	char send[128] = {0};
	printf("server recv msg:%s\n",param->recvvalue);

	//return client
	sprintf(send,"aaaaaaaaaab01");
//	network_send_data((network_Handle_t)param->clientHandle,param->recvvalue,param->nLen,param->Code);
	//printf("return str:%s\n\n\n",send);

	return nRet;
}

int demo_server_heartMsg(char *messege, int nLen,  network_Handle_t handle, void *inparam,int *msgLen)
{
	char *heart = "heart";
	sprintf(messege,"%s",heart);
	*msgLen = strlen(heart);

	return 0;
}

int demo_server_netstatus(network_Status_t status, network_Handle_t handle, void *inparam)
{
	clientHandle = handle;
	return 0;
}
int demo_server_logMsg(const char *format, ...)
{
	char buffer[512] = { 0 };
	va_list args;
	va_start(args, format);
	vsprintf(buffer, format, args);
	va_end(args);
	printf("server log: %s", buffer);

	return 0;
}

int main(int argc,char *argv[])
{
	int ret = 0;
	network_inparamServerNet_t inparam;

	signal(SIGPIPE, SIG_IGN);

	memset(&inparam, 0, sizeof(network_inparamServerNet_t));
	inparam.cmdfun = demo_server_DealCmd;
	inparam.heartmsg = demo_server_heartMsg;
	inparam.logFun = demo_server_logMsg;
	inparam.statusFun = demo_server_netstatus;
	inparam.overtime = 2000;
	inparam.asynchronous = 1;
	inparam.nPort = 9998;
	inparam.ipv6 = 1;

#if 1
	/* 使用第三方协议头 */
	inparam.headInit = networkHead_standard_init;
	inparam.headCheck = networkHead_standard_check;
	inparam.headSize = sizeof(NetworkHeadStandard_S);
#endif

	printf("heart:%p\n",inparam.heartmsg);
	serverHandle = network_init_server(inparam);

	printf("create server success!!! port[%d]\n",inparam.nPort);

//	sleep(3);
//	network_unInit_server(serverHandle);
//	printf("exit server!!!\n");

#if 1
	netPacket_t pkt;
	while(1)
	{

		if(clientHandle)
		{
			printf("server send data to client!!\n");
	#if 1
			//异步返回
//			network_send_data(clientHandle,"123456",7,100);
			network_serverSendData_toAllClient(serverHandle,"123456",7,100);

	#else
			//同步返回
			memset(&pkt,0,sizeof(netPacket_t));
			ret = network_send_data_sync(clientHandle,"123456",7,100,3000,&pkt);
			if(ret < 0)
			{
				printf("sync recv error!!!\n");
			}else
			{
				printf("recv server:%s\n\n\n\n",pkt.data);
				network_release_syncPkt(clientHandle,&pkt);
			}
	#endif
		}
		sleep(3);
	}
#endif

	while(1)
	{
		sleep(100);
	}

	return 0;
}


