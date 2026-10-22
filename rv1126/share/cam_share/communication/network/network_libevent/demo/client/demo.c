

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include "network_libevent.h"

FILE *h264fp = NULL;
FILE *aacfp = NULL;

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

#if 0
	char send[128] = {0};
	static flag = 0;
	char *p = NULL;
	static Iframe = 0;

	if(param->Code == 1000002)
	{
		printf("[0x%x] [0x%x] [0x%x] [0x%x] [0x%x]\n",(param->recvvalue)[0],\
				(param->recvvalue)[1],(param->recvvalue)[2],(param->recvvalue)[3],(param->recvvalue)[4]);

		if((((param->recvvalue)[4] & 0xf) == 0x7) && (Iframe == 0))
		{
			printf("get I frame\n");
			Iframe = 1;
		}

		if(Iframe == 0)
		{
			return 0;
		}

		flag++;
		/*视频*/
		fwrite(param->recvvalue,1,param->nLen,h264fp);
	}else if(param->Code == 1000001)
	{
		/*音频*/
		fwrite(param->recvvalue,1,param->nLen,aacfp);
	}

	if(flag > 200)
	{
		printf("close file!!\n");
		fclose(h264fp);
		fclose(aacfp);
	}
#endif
	printf("client recv server msg:%s\n",param->recvvalue);

	network_send_data(param->clientHandle,s_send_data,1024,param->Code);

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

	printf("========> status[%d]\n",status);

	return 0;
}
int demo_server_logMsg(const char *format, ...)
{
	char buffer[512] = { 0 };
	va_list args;
	va_start(args, format);
	vsprintf(buffer, format, args);
	va_end(args);
	printf("log: %s", buffer);

	return 0;
}



int main(int argc,char *argv[])
{
	int ret = 0;
	memset(s_send_data,0x61,sizeof(s_send_data));

	network_inparamClientNet_t inparam;

	signal(SIGPIPE, SIG_IGN);

#if 0
	h264fp = fopen("receive.h264","wb");
	if (!h264fp){
		printf("Open File Error.\n");
		return -1;
	}
	aacfp = fopen("receive.aac","wb");
	if (!h264fp){
		printf("Open File Error.\n");
		return -1;
	}
#endif

	int ipv6 = 0;
	//ipv6 = atoi(argv[1]);
	printf("ipv6[%d]\n",ipv6);

	memset(&inparam, 0, sizeof(network_inparamClientNet_t));
	inparam.cmdfun = demo_server_DealCmd;
	inparam.logFun = demo_server_logMsg;
	inparam.statusFun = demo_server_netstatus;
	inparam.overtime = 2000;
	inparam.nReconnect = 0;//1;
	inparam.asynchronous = 1;
	inparam.nPort = 9998;

	if(ipv6 == 1)
	{
		sprintf(inparam.ip,"fd98:437e:3662:0:19da:dbaf:f60f:4562");
	}else
	{
		sprintf(inparam.ip,"127.0.0.1");
	}

	printf("server ip:%s\n",inparam.ip);
	clientHandle = network_init_client(inparam);


#if 0
	netPacket_t pkt;
	while(1)
	{


#if 1
		//异步返回
		//printf("client send data to server!!\n");
		//network_send_data(clientHandle,"123456",7,100);
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

		sleep(3);
	}
#endif

	sleep(10);
	printf("stop client!!!\n");
	network_unInit_client(clientHandle);


	while(1)
	{
		sleep(100);
	}

	return 0;
}


