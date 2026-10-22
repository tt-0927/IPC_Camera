/*
 * share_udp.c
 *
 *  Created on: 2018年8月2日
 *      Author: lixiao
 */

#include "share_udp.h"
#include <net/if.h>

int parse_mseg(const char *buf,char *field,int *actionCode,char *parseOut)
{
	if(buf == NULL)
	{
		return -1;
	}
	char outPut[256] = {0};
  char *opPtr = NULL;
	opPtr = strstr(buf,field);
	if(opPtr == NULL)
	{
		return -1;
	}
  sscanf(opPtr,"%*s%s",outPut);
  if(actionCode != NULL)
  {
		*actionCode = atoi(outPut);
  }
  if(parseOut != NULL)
  {
		memcpy(parseOut,outPut,strlen(outPut)+1);
		//printf("parseOut = %s\n",parseOut);
  }
 
  return  0;
}

int parse_msegInt(const char *buf,char *field,int *actionCode,int *parseInt)
{
	char parseOut[64] = {0};
	if( 0 == parse_mseg(buf,field,actionCode,parseOut))
	{
			//printf("line : %d\n",__LINE__);
			*parseInt = atoi(parseOut);
	}
	else
	{
	//printf("line : %d\n",__LINE__);
			return -1;
	}
	return 0;
}
int udp_creatUdpSocket()
{
	int sock;
	sock = RH_Socket(__FILE__, (char *)__func__, AF_INET, SOCK_DGRAM, 0);
	return sock;
}


void udp_setSocketDst(Udp_info_t udpInfo,struct sockaddr_in *skt_addr)
{
	skt_addr->sin_family = AF_INET;
	if(udpInfo.port)
	{
		skt_addr->sin_port = htons(udpInfo.port); //绑定端口
	}
	else
	{
		//不绑定
	}
	skt_addr->sin_port = htons(udpInfo.port);
	if(strcmp(udpInfo.ip,UDP_LOCAL_IP) != 0)
	{
		skt_addr->sin_addr.s_addr = inet_addr(udpInfo.ip);
		// inet_pton
		printf("ip:%s\n", udpInfo.ip);
	}
	else
	{
		skt_addr->sin_addr.s_addr = htonl(INADDR_ANY);
	}
}

int udp_bindPort_init(struct sockaddr_in skt_addr,int udpSocket)
{
	int rtv = bind(udpSocket, (struct sockaddr *)&skt_addr, sizeof(skt_addr));
	if(rtv == -1)
	{
		return -1;
	}
	return rtv;
}

int udp_joinMulticastGroup(Udp_info_t udpInfo,int sktFd)
{
		int loop;
		struct ip_mreqn multi;

		multi.imr_multiaddr.s_addr = inet_addr(udpInfo.multicastIp);	 
		multi.imr_address.s_addr = htonl(INADDR_ANY);
		//指定网卡
		multi.imr_ifindex = if_nametoindex("eth0"); 
		int rtv = setsockopt(sktFd, IPPROTO_IP, IP_ADD_MEMBERSHIP, &multi, sizeof(multi));
		if(rtv == -1)
		{
			return -1;
		}
		loop= udpInfo.muticastInfo.loopBack;
		if(setsockopt(sktFd,IPPROTO_IP,IP_MULTICAST_LOOP,(const char*)&loop,sizeof(loop))!=0)
		{
				return -1;
		}

		return 0;
}



int init_udp_client(InparamClientNetUdp_t netparm)
{
	int skt_fd;
	printf("ip = %s,port=%d,multicast=%s\n",netparm.udpConnectInfo->ip,netparm.udpConnectInfo->port,netparm.udpConnectInfo->multicastIp);
	struct sockaddr_in skt_addr;
	skt_fd = udp_creatUdpSocket();
	if(skt_fd == -1)
	{
		perror("request socket failed\n");
		return -1;
	}
	if(netparm.udpConnectInfo->port)
	{
		udp_setSocketDst(*netparm.udpConnectInfo,&skt_addr);
		if(0 > udp_bindPort_init(skt_addr,skt_fd))
		{
			perror("bind ip port failed\n");
			return -1;
		}
		if(netparm.udpConnectInfo->muticastInfo.enable == TRUE)
		{
			if(0 > udp_joinMulticastGroup(*netparm.udpConnectInfo,skt_fd))	//组播 
			{
				perror("add multicase failed\n");
				return -1;
			}
		}
		else if(netparm.udpConnectInfo->boardcastInfo.enable == TRUE)  //广播
		{
			//
		}
		else
		{
			//普通udp
			printf("normal udp--------------------\n");
			return skt_fd;
		}
	}

	return skt_fd;
}



static int udp_pthread_create(pthread_t *thread_id,const pthread_attr_t *user_attr, pthread_fun_udp funtion, void *argv)
{
  int ret ,stacksize = 128*1024; /*thread 堆栈设置为128K，stacksize以字节为单位。*/
  pthread_attr_t attr;
  ret = pthread_attr_init(&attr); /*初始化线程属性*/
  if (ret != 0)
      return -1;
  ret = pthread_attr_setstacksize(&attr, stacksize);
  if(ret != 0)
  {
	  pthread_attr_destroy(&attr); /*不再使用线程属性，将其销毁*/
	  return -1;
  }
  ret = pthread_create (thread_id, &attr, funtion, argv);
  if(ret != 0)
  {
	  pthread_attr_destroy(&attr); /*不再使用线程属性，将其销毁*/
	  return -1;
  }
  ret = pthread_attr_destroy(&attr); /*不再使用线程属性，将其销毁*/
  if(ret != 0)
  {
	  return -1;
  }
	return 0;
}

static int free_asynchronous_recmessege(UdpAsynchronous_Info_ptr asynchronous_recmessege)
{
	if(asynchronous_recmessege)
	{
		if(asynchronous_recmessege->dataBuf)
		{
			 free(asynchronous_recmessege->dataBuf);
			 asynchronous_recmessege->dataBuf = NULL;
		}
		 free(asynchronous_recmessege);
		 asynchronous_recmessege = NULL;
	}
	return 0;
}


void * udp_clientrec_Thread(void *argv)
{
	UdpOpreateHandle_ptr pOprHandle = (UdpOpreateHandle_ptr)argv;
	List_LockHandle_t* pHeadHandle = pOprHandle->asynchronous_reclist;
	UdpAsynchronous_Info_ptr asynchronous_recmessege = NULL;
	UdpCallbackMsg_t  udpBackMsg;
	while(1)
	{
		asynchronous_recmessege	= list_lockAndPop_frontSignal(pHeadHandle, 0);
		if(asynchronous_recmessege == NULL)
		{
				continue;		
		}
		else
		{
			if(pOprHandle->inparam.cmdfun != NULL)
			{
				memset(&udpBackMsg,0,sizeof(UdpCallbackMsg_t));
				udpBackMsg.recvvalue = asynchronous_recmessege;
				udpBackMsg.nLen = pOprHandle->inparam.udpConnectInfo->recvvalueLen;
				udpBackMsg.InParam = pOprHandle->inparam.param;
				udpBackMsg.sOperHandle	= pOprHandle;
				pOprHandle->inparam.cmdfun(&udpBackMsg);
			}
			
			free_asynchronous_recmessege(asynchronous_recmessege);

		}
	}
	return NULL;
}



void * udp_clientsend_Thread(void *argv)
{
	UdpOpreateHandle_ptr pOprHandle = (UdpOpreateHandle_ptr)argv;
	List_LockHandle_t* pHeadHandle = pOprHandle->asynchronous_sendclist;
	UdpAsynchronous_Info_ptr asynchronous_sendmessege = NULL;
	struct sockaddr_in desAddr;
	desAddr.sin_family = AF_INET;
	while(1)
	{
		asynchronous_sendmessege	= list_lockAndPop_frontSignal(pHeadHandle, 0);
		if(asynchronous_sendmessege == NULL)
		{
				continue;		
		}
		else
		{
			desAddr.sin_port 	= htons(asynchronous_sendmessege->port);
			if(strcmp(asynchronous_sendmessege->ip,UDP_LOCAL_IP) != 0)
			{
				//desAddr.sin_addr.s_addr = inet_addr(asynchronous_sendmessege->ip);
				inet_pton(AF_INET,asynchronous_sendmessege->ip, &desAddr.sin_addr.s_addr);
			}
			else
			{
				desAddr.sin_addr.s_addr = htonl(INADDR_ANY);
			}
			int rtv = sendto(pOprHandle->socket, asynchronous_sendmessege->dataBuf,asynchronous_sendmessege->nLen, 0,(struct sockaddr *)&desAddr, sizeof(desAddr));
			if(rtv == -1)
			{
				perror("sendto failed\n");
			}
			free_asynchronous_recmessege(asynchronous_sendmessege);
		}
	}
	return NULL;
}

/*
1.接收者ip
2.接受者端口
3.发送的数据     
4.数据长度
5.通信句柄
*/
int udp_clientSendToServer_func(UdpAsynchronous_Info_ptr asynchronous_sendmessege,Sdk_udp_Handle_t udp_handle)
{
		//asynchronous_sendmessege 是堆指针,asynchronous_sendmessege->dataBuf 也是堆指针,不用手动释放
		UdpOpreateHandle_ptr udpHandle = (UdpOpreateHandle_ptr)udp_handle;
		list_lockAndPush_backSignal(udpHandle->asynchronous_sendclist,asynchronous_sendmessege);
		return 0;
}

static void* udpClient_thread(void * argv)
{
		int ret = 0;
		char ipBuf[16] = {0};
		struct sockaddr_in rcvAddr;
		socklen_t rcvLen = sizeof(rcvAddr);
		UdpAsynchronous_Info_ptr listInfo = NULL;		
		
		UdpOpreateHandle_ptr pNetHandle = (UdpOpreateHandle_ptr)argv;
		pNetHandle->socket = init_udp_client(pNetHandle->inparam);
		if(pNetHandle->socket < 1)
		{
			close(pNetHandle->socket);	 //错误就直接退出了
			printf("create udp failed\n");
			return NULL;
		}
		if(pNetHandle->inparam.udpConnectInfo->port)
		{

			pNetHandle->asynchronous_reclist =	list_lockAndCreate();
			ret = udp_pthread_create(&(pNetHandle->rec_tid), NULL, udp_clientrec_Thread, (void *)(pNetHandle));
			if(ret != 0)
			{
				pNetHandle->inparam.logFun(" netclient_thread sdk_pthread_create net_clientrec_Thread is fail\n");
				if(pNetHandle != NULL)
				{
					close(pNetHandle->socket);
					pNetHandle->socket = -1;
					free(pNetHandle);
				} 		
				return NULL;
			} 
		}
		pNetHandle->asynchronous_sendclist =  list_lockAndCreate();
		ret = udp_pthread_create(&(pNetHandle->send_tid), NULL, udp_clientsend_Thread, (void *)(pNetHandle));
		if(ret != 0)
		{
			pNetHandle->inparam.logFun(" netclient_thread sdk_pthread_create udp_clientsend_Thread is fail\n");
			if(pNetHandle != NULL)
			{
				close(pNetHandle->socket);
				pNetHandle->socket = -1;
				free(pNetHandle);
			}
			return NULL;
		}	
		int dataLen = pNetHandle->inparam.udpConnectInfo->recvvalueLen;
		char dataBuf[dataLen];

		while(1)
		{
			if(pNetHandle->inparam.udpConnectInfo->port)
			{
				memset(dataBuf, 0, sizeof(dataBuf));
				ret = recvfrom(pNetHandle->socket, dataBuf, sizeof(dataBuf), 0,(struct sockaddr *)&rcvAddr, &rcvLen);
				
				if(ret == -1)
				{
					perror("recvfrom failed\n");
					sleep(3);
					continue;
				}
				inet_ntop(rcvAddr.sin_family, &rcvAddr.sin_addr.s_addr, ipBuf, rcvLen);
				
				//检查头
				if(pNetHandle->inparam.checkHeadFunc != NULL) //需要检查头
				{
					if(0 > pNetHandle->inparam.checkHeadFunc(dataBuf))
					{
							//检验错误
							continue;
					}
				}
				printf("dataBuf=%s\n",dataBuf);
				listInfo = (UdpAsynchronous_Info_ptr)malloc(sizeof(UdpAsynchronous_Info_t));
				listInfo->dataBuf = (char*)malloc(dataLen);
				memcpy(listInfo->dataBuf,dataBuf,ret);
				memcpy(listInfo->ip,ipBuf,sizeof(ipBuf));
				listInfo->port = ntohs(rcvAddr.sin_port);
				listInfo->nLen = rcvLen;
				list_lockAndPush_backSignal(pNetHandle->asynchronous_reclist,listInfo);
			}
			else
			{
				sleep(3);
			}
			
			

		}

		close(pNetHandle->socket);
		return NULL;
}



Sdk_udp_Handle_t udpServer_init_net(InparamClientNetUdp_t netparm)
{
	UdpOpreateHandle_ptr pNetHandle = NULL;
	if(netparm.udpConnectInfo->ip == NULL || netparm.logFun == NULL)
	{
		return NULL;
	}
	pNetHandle = (UdpOpreateHandle_ptr)malloc(sizeof(UdpOpreateHandle_t));
	if (pNetHandle == NULL)
	{
		netparm.logFun("malloc Net_Opreate_Hanle_t is fail\n");
	}

	memset(pNetHandle, 0, sizeof(UdpOpreateHandle_t));
	memcpy(&(pNetHandle->inparam), &netparm, sizeof(InparamClientNetUdp_t));

	pthread_mutex_init(&(pNetHandle->lock), NULL);
	pthread_mutex_init(&(pNetHandle->netlock), NULL);

	int ret = udp_pthread_create(&(pNetHandle->client_tid), NULL, udpClient_thread, (void *)(pNetHandle));
	if (ret != 0)
	{
		netparm.logFun("crate NetCreateclint thread failed\n");
		free(pNetHandle);
		return NULL;
	}

	return pNetHandle;
}



