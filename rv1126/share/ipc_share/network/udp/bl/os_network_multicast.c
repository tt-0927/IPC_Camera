/*************************************************************************
  UDP接口文件
  @File Name: os_network_multicast.c
  @Author: luoyongkang
  @Created Time: 2021年03月29日 星期一 
 ************************************************************************/

#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <sys/errno.h>
#include <sys/uio.h>
#include"os_network_multicast.h"

typedef long long Int64; 

static int os_network_socket(NetworkMulticast_S*  stpnetwork_info)
{
	int sockfd;
	if( NULL == stpnetwork_info )
	{
		return -3;
	}
begin:
	/*创建套接字*/
	sockfd = socket(AF_INET, SOCK_DGRAM, 0);
	struct sockaddr_in sock_addr, mucast_addr, unque_addr, broadcast_addr;
	memset(&sock_addr, 0, sizeof(sock_addr));
	memset(&mucast_addr, 0, sizeof(mucast_addr));
	memset(&unque_addr, 0, sizeof(unque_addr));
	sock_addr.sin_family = AF_INET;  

	if(stpnetwork_info->entype == UNICAST)
	{
		if(stpnetwork_info->stsrc_ip_info.ip[0] == 0 && 
		   stpnetwork_info->stsrc_ip_info.ip[1] == 0 &&
		   stpnetwork_info->stsrc_ip_info.ip[3] == 0 )
		{
			/*让系统检测本地网卡，自动绑定本地IP*/
			sock_addr.sin_addr.s_addr = INADDR_ANY;
		}
		else
		{
			/*源IP地址*/
			sock_addr.sin_addr.s_addr = inet_addr(stpnetwork_info->stsrc_ip_info.ip);  
		}
		sock_addr.sin_port = htons(stpnetwork_info->stsrc_ip_info.port);  /*端口*/
		unque_addr.sin_family = AF_INET;
		unque_addr.sin_addr.s_addr = inet_addr(stpnetwork_info->stdst_ip_info.ip);  /*目的IP地址*/
		unque_addr.sin_port = htons(stpnetwork_info->stdst_ip_info.port);  /*端口*/
		memcpy(&stpnetwork_info->stunque_addr, &unque_addr, sizeof(struct sockaddr_in));
	}
	else if(stpnetwork_info->entype == MULTICAST || stpnetwork_info->entype == SPECAST)
	{
		
		sock_addr.sin_addr.s_addr = INADDR_ANY;/*默认所有地址*/
		sock_addr.sin_port = htons(stpnetwork_info->stsrc_ip_info.port);  /*端口*/

		mucast_addr.sin_family = AF_INET;
		mucast_addr.sin_addr.s_addr = inet_addr(stpnetwork_info->amcast_ip);  /*具体的IP地址*/
		if( stpnetwork_info->nmcast_port <= 0)
		{
			mucast_addr.sin_port = htons(stpnetwork_info->stsrc_ip_info.port);  /*端口*/
		}
		else
		{
			mucast_addr.sin_port = htons(stpnetwork_info->nmcast_port);  /*端口*/
		}
		memcpy(&stpnetwork_info->stmucast_addr, &mucast_addr, sizeof(struct sockaddr_in));
	}
	else if(stpnetwork_info->entype == BROADCAST)
	{
		
		sock_addr.sin_addr.s_addr = INADDR_ANY;/*默认所有地址*/
		sock_addr.sin_port = htons(stpnetwork_info->stsrc_ip_info.port);  /*端口*/

		broadcast_addr.sin_family = AF_INET;
		broadcast_addr.sin_addr.s_addr = inet_addr(stpnetwork_info->stdst_ip_info.ip);  /*目的IP地址*/
		broadcast_addr.sin_port = htons(stpnetwork_info->stdst_ip_info.port);  /*端口*/
		memcpy(&stpnetwork_info->stbroadcast_addr, &broadcast_addr, sizeof(struct sockaddr_in));
	}

	/*设置超时时间*/
	if (setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &stpnetwork_info->sttimeout, sizeof(stpnetwork_info->sttimeout)) < 0)
	{
		perror("time out setting failed");
		goto error;
	}
	/*端口复用*/
	int reuse = 1;
	if(setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, (char *)&reuse, sizeof(reuse)) < 0)
	{
		perror("Setting SO_REUSEADDR error");
		goto error;
	}
	if(stpnetwork_info->entype == BROADCAST)
	{
		//设置为广播类型
		int opt = 1;
		if(setsockopt(sockfd, SOL_SOCKET, SO_BROADCAST, &opt, sizeof(opt))<0)
		{
			perror("Setting SO_BROADCAST error");
			goto error;
		}
	}
	char on = 1;
	/* 设置IP_PKTINFO属性 */
	if (0 != setsockopt(sockfd, IPPROTO_IP, IP_PKTINFO, (char *)&on, sizeof(on))) 
	{
		perror("setsockopt ip_pktinfo fail, errno : \n");
		goto error;
	}
	int ret = bind(sockfd, (struct sockaddr*)&sock_addr, sizeof(sock_addr));
	if(-1 == ret)
	{
		perror("bind:");
		goto error;
	}
	if(stpnetwork_info->entype == UNICAST || stpnetwork_info->entype == BROADCAST)
	{
		printf("socket sucess\n");
		stpnetwork_info->nfd =sockfd;
		return 0;
	}
	/*设置是否支持本地回环接收,1是0否*/
	int loopBack=stpnetwork_info->loopBack;
	ret = setsockopt(sockfd,IPPROTO_IP, IP_MULTICAST_LOOP, &loopBack, sizeof(loopBack));
	if(-1 == ret)
	{
		printf("setsockopt broadcaset error!!!\n");
		perror("setsockopt:");
		close(sockfd);
		return -1;
	}
	/*设置缓冲区大小*/
	if (stpnetwork_info->nrecv_opt > 0)
	{
		if (setsockopt(sockfd, SOL_SOCKET, SO_RCVBUF, &stpnetwork_info->nrecv_opt, sizeof(int)) < 0)
		{
			printf("setsockopt error=%d(%s)!!!\n", errno, strerror(errno));
			goto error;
		}
	}
	if (stpnetwork_info->nsend_opt > 0)
	{
		if (setsockopt(sockfd, SOL_SOCKET, SO_SNDBUF, &stpnetwork_info->nrecv_opt, sizeof(int)) < 0)
		{
			printf("setsockopt error=%d(%s)!!!\n", errno, strerror(errno));
			goto error;
		}
	}



	/*加入多播组*/
	struct in_addr addr;
	memset(&addr, 0, sizeof(addr));

	if(stpnetwork_info->entype == MULTICAST)
	{
		addr.s_addr=INADDR_ANY;
	}
	else if(stpnetwork_info->entype == SPECAST)
	{
		addr.s_addr=inet_addr(stpnetwork_info->stsrc_ip_info.ip);
	}

	struct ip_mreq ipmr;
	ipmr.imr_interface.s_addr = addr.s_addr;

	ipmr.imr_multiaddr.s_addr = inet_addr(stpnetwork_info->amcast_ip);
	ret=setsockopt(sockfd,IPPROTO_IP,IP_ADD_MEMBERSHIP,(const char*)&ipmr,sizeof(ipmr));
	if (ret < 0)
	{

		perror("setsockopt():IP_ADD_MEMBERSHIP");
		goto error;
	}

	/*此处指定组播数据的出口网卡，如果不设置则会根据路由表指定默认路由出口*/
	if(stpnetwork_info->entype == SPECAST)
	{
		if(-1 == setsockopt(sockfd, IPPROTO_IP, IP_MULTICAST_IF, (char *)&addr, sizeof(addr)))
		{
			printf("set error IP_MULTICAST_IF %s\n", stpnetwork_info->amcast_ip);
			perror("Setting IP_MULTICAST_IF error:");
			goto error;
		}
	}
	printf("socket success\n");
	stpnetwork_info->nfd =sockfd;
	return 0;
error:
	if (sockfd >= 0)
	{
		close(sockfd);
	}
	return -1;
}


/*单播发送*/
int os_networkunque_send(const char *pdata, int nlenght, NetworkMulticast_S* pNetwork_info)
{
	int nsend_lenght=0, nret=0;

	if( NULL == pdata )
	{
		return -1;

	}

	if( 0 >= nlenght  )
	{
		return -2;

	}

	if( NULL == pNetwork_info )
	{
		return -3;
	}

	/*异步*/
	if( 1 == pNetwork_info->nasynchronous )
	{
		/*将接受的数据存到队列 */
		char* pbuf = (char*)malloc(nlenght);
		memcpy(pbuf, pdata, nlenght);
		int status = OS_quePut(&(pNetwork_info->sthndl), (Int64)pbuf, 100);
		if(status !=0 )
		{
			printf("put fail\n");
		}

		return status;
	}

	while( nsend_lenght < nlenght )
	{

		nret = sendto(pNetwork_info->nfd, pdata + nsend_lenght, 
					  nlenght-nsend_lenght, pNetwork_info->nflags, 
					  (struct sockaddr *)&(pNetwork_info->stunque_addr), 
					  sizeof(pNetwork_info->stunque_addr));
		if(nret<0)
		{
			perror("error:");
			printf("os_networkunque_send error nRet[%d]\n",nret);
			break;
		}
		if(nret == -1)
		{
			printf("send fail\n");
			return -1;
		}
		nsend_lenght += nret;
	}
	return nsend_lenght;
}

/*广播发送*/
int os_networkbroadcast_send(const char *pdata, int nlenght, NetworkMulticast_S* pNetwork_info)
{
	int nsend_lenght=0, nret=0;

	if( NULL == pdata )
	{
		return -1;

	}

	if( 0 >= nlenght  )
	{
		return -2;

	}

	if( NULL == pNetwork_info )
	{
		return -3;
	}

	/*异步*/
	if( 1 == pNetwork_info->nasynchronous )
	{
		/*将接受的数据存到队列 */
		char* pbuf = (char*)malloc(nlenght);
		memcpy(pbuf, pdata, nlenght);
		int status = OS_quePut(&(pNetwork_info->sthndl), (Int64)pbuf, 100);
		if(status !=0 )
		{
			printf("put fail\n");
		}

		return status;
	}

	int optval = 1;
	setsockopt(pNetwork_info->nfd, SOL_SOCKET, SO_BROADCAST | SO_REUSEADDR, &optval, sizeof(int));

	while( nsend_lenght < nlenght )
	{

		nret = sendto(pNetwork_info->nfd, pdata + nsend_lenght, nlenght-nsend_lenght, pNetwork_info->nflags, (struct sockaddr *)&(pNetwork_info->stbroadcast_addr), sizeof(pNetwork_info->stbroadcast_addr));
		if(nret<0)
		{
			perror("error:");
			printf("os_networkbroadcast_send error 11 nRet[%d]\n",nret);
			break;
		}
		if(nret == -1)
		{
			printf("send fail\n");
			return -1;
		}
		nsend_lenght += nret;
	}
	return nsend_lenght;
}

/*广播发送*/
int os_networkbroadcast_port_send(const char *pdata, int nlenght, NetworkMulticast_S* pNetwork_info,int nPort)
{
	int nsend_lenght=0, nret=0;

	if( NULL == pdata )
	{
		return -1;

	}

	if( 0 >= nlenght  )
	{
		return -2;

	}

	if( NULL == pNetwork_info )
	{
		return -3;
	}

	/*异步*/
	if( 1 == pNetwork_info->nasynchronous )
	{
		/*将接受的数据存到队列 */
		char* pbuf = (char*)malloc(nlenght);
		memcpy(pbuf, pdata, nlenght);
		int status = OS_quePut(&(pNetwork_info->sthndl), (Int64)pbuf, 100);
		if(status !=0 )
		{
			printf("put fail\n");
		}

		return status;
	}

	int brdcFd;
	if((brdcFd = socket(PF_INET, SOCK_DGRAM, 0)) == -1){
		printf("socket fail\n");
		return -1;
	}
	int optval = 1;
	setsockopt(brdcFd, SOL_SOCKET, SO_BROADCAST | SO_REUSEADDR, &optval, sizeof(int));

	struct sockaddr_in seraddr;
	memset(&seraddr,0,sizeof(struct sockaddr_in));
	seraddr.sin_family = AF_INET;
	seraddr.sin_port = htons(nPort); 
	seraddr.sin_addr.s_addr = pNetwork_info->stbroadcast_addr.sin_addr.s_addr;

	while( nsend_lenght < nlenght )
	{

		nret = sendto(brdcFd, pdata + nsend_lenght, nlenght-nsend_lenght, pNetwork_info->nflags, (struct sockaddr *)&(seraddr), sizeof(seraddr));
		if(nret<0)
		{
			perror("error:");
			printf("os_networkbroadcast_send 222 error nRet[%d]\n",nret);
			break;
		}
		if(nret == -1)
		{
			printf("send fail\n");
			return -1;
		}
		nsend_lenght += nret;
	}
	close(brdcFd);
	return nsend_lenght;
}

/*单播发送*/
int os_networkunque_ip_send(const char *pdata, int nlenght, NetworkMulticast_S* pNetwork_info, char* ip, int port)
{
	if( NULL == pdata )
	{
		return -1;

	}

	if( 0 >= nlenght  )
	{
		return -2;

	}

	if( NULL == pNetwork_info )
	{
		return -3;
	}

	if( NULL == ip )
	{
		return -4;
	}

	struct sockaddr_in seraddr;
	seraddr.sin_family = AF_INET;
	seraddr.sin_port = htons(port); 
	seraddr.sin_addr.s_addr = inet_addr(ip);
	return sendto(pNetwork_info->nfd, pdata, nlenght, pNetwork_info->nflags, (struct sockaddr *)&(seraddr), sizeof(seraddr)) ;     

	int nsend_lenght=0, nret=0;
	if(pdata == NULL)
		return -1;
	if(nlenght <=0)
		return -2;

	while( nsend_lenght < nlenght )
	{
		nret = sendto(pNetwork_info->nfd, pdata + nsend_lenght, nlenght-nsend_lenght, pNetwork_info->nflags, (struct sockaddr *)&(seraddr), sizeof(seraddr)) ;
		nsend_lenght += nret;
		if(nret == -1)
		{
			printf("send fail\n");
			return -1;
		}
	}
	return nsend_lenght;
}


/*组播异步发送线程*/
void* mucast_asynchronoussend_thr(void* param)
{
	char* abuf=NULL;
	Int64 naddr = 0;
	NetworkMulticast_S* stpnetwork_info = (NetworkMulticast_S* )param;
	while( !stpnetwork_info->nExit )
	{
		/* 将队列中的数据及时获取出 */
		int nstatus = OS_queGet( &(stpnetwork_info->sthndl), &naddr, 3000);
		if(nstatus == 0)
		{
			abuf = (char*)naddr;
			if(abuf != NULL)
			{
				sendto(stpnetwork_info->nfd, abuf, stpnetwork_info->nasynchronous_sendsize, 
				       stpnetwork_info->nflags, (struct sockaddr *)&(stpnetwork_info->stmucast_addr), 
					   sizeof(stpnetwork_info->stmucast_addr)) ;     
				free(abuf);
				abuf=NULL;
			}
			else
			{
				printf("os que buf null\n");
			}
		}
	}

	return NULL;
}

/*单播异步发送线程*/
void* unque_asynchronoussend_thr(void* param)
{
	char* abuf=NULL;
	Int64 naddr = 0;
	NetworkMulticast_S* stpnetwork_info = (NetworkMulticast_S* )param;
	while( !stpnetwork_info->nExit )
	{
		/* 将队列中的数据及时获取出 */
		int nstatus = OS_queGet( &(stpnetwork_info->sthndl), &naddr, 3000);
		if(nstatus == 0)
		{
			abuf = (char*)naddr;
			if(abuf != NULL)
			{
				sendto(stpnetwork_info->nfd, abuf, 
				    stpnetwork_info->nasynchronous_sendsize, 
					stpnetwork_info->nflags, 
					(struct sockaddr *)&(stpnetwork_info->stunque_addr), 
					sizeof(stpnetwork_info->stunque_addr)) ; 
				free(abuf);
				abuf=NULL;
			}
			else
			{
				printf("os que buf null\n");
			}
		}
	}

	return NULL;
}

/*组播发送*/
int os_networkmulticast_send(const char *pdata, int nlenght, NetworkMulticast_S* pNetwork_info)
{
	int nsend_lenght=0, nret=0;

	if(pdata == NULL)
	{
		return -1;
	}

	if(nlenght <=0)
	{
		return -2;
	}

	if( NULL == pNetwork_info )
	{
		return -3;
	}
	/*异步*/
	if( 1 == pNetwork_info->nasynchronous )
	{
		/*将接受的数据存到队列 */
		char* pbuf = (char*)malloc(nlenght);
		memcpy(pbuf, pdata, nlenght);
		int status = OS_quePut(&(pNetwork_info->sthndl), (Int64)pbuf, 100);
		if(status !=0 )
		{
			printf("put fail\n");
		}

		return status;
	}

	while( nsend_lenght < nlenght )
	{
		nret =sendto(pNetwork_info->nfd, pdata + nsend_lenght, 
		             nlenght-nsend_lenght, pNetwork_info->nflags, 
					 (struct sockaddr *)&(pNetwork_info->stmucast_addr), 
					 sizeof(pNetwork_info->stmucast_addr)) ;     
		nsend_lenght+=nret;
		if(nret == -1)
		{
			printf("send fail\n");
			perror("send:");
			return -1;
		}
	}

	return nsend_lenght;

}

/*组播接收处理线程*/
void* os_networkmulticast_recv(void* network_info1)
{
	int nrecv_size = 0;
	NetworkMulticast_S* stpnetwork_info = (NetworkMulticast_S* )network_info1;

	if( stpnetwork_info == NULL)
	{
		return NULL;
	}
	if( stpnetwork_info->fnhandledata == NULL)
	{
		return NULL;

	}

	/*数据大于DATASIZE 分段发送*/
	if( stpnetwork_info->nread_size <= 0 )
	{
		nrecv_size = DATASIZE;
	}
	else if( stpnetwork_info->nread_size >= DATASIZE )
	{
		nrecv_size = DATASIZE;
	}
	else
	{
		nrecv_size =  stpnetwork_info->nread_size;
	}
	while (!stpnetwork_info->nExit)
    {
        struct sockaddr_in src_addr;
        struct iovec iov[1];
        struct msghdr msg;
        char cmsgbuf[CMSG_SPACE(sizeof(struct in_pktinfo))];

        memset(&src_addr, 0, sizeof(src_addr));
        memset(&msg, 0, sizeof(msg));
        memset(stpnetwork_info->stuser_recv_handle.data, 0, DATASIZE);
        memset(cmsgbuf, 0, sizeof(cmsgbuf));

        iov[0].iov_base = stpnetwork_info->stuser_recv_handle.data;
        iov[0].iov_len = nrecv_size;

        msg.msg_name = &src_addr;
        msg.msg_namelen = sizeof(src_addr);
        msg.msg_iov = iov;
        msg.msg_iovlen = 1;
        msg.msg_control = cmsgbuf;
        msg.msg_controllen = sizeof(cmsgbuf);

        int datalength = recvmsg(stpnetwork_info->nfd, &msg, 0);
        if (datalength <= 0)
            break;

        stpnetwork_info->stuser_recv_handle.lenght = datalength;

        /* 源 IP/端口 */
        snprintf(stpnetwork_info->stuser_recv_handle.stNetwork.ip,
                 sizeof(stpnetwork_info->stuser_recv_handle.stNetwork.ip),
                 "%s", inet_ntoa(src_addr.sin_addr));
        stpnetwork_info->stuser_recv_handle.stNetwork.port = ntohs(src_addr.sin_port);

        /* 目的 IP（从 IP_PKTINFO 获取） */
        struct cmsghdr *cmsg;
        for (cmsg = CMSG_FIRSTHDR(&msg); cmsg != NULL; cmsg = CMSG_NXTHDR(&msg, cmsg))
        {
            if (cmsg->cmsg_level == IPPROTO_IP && cmsg->cmsg_type == IP_PKTINFO)
            {
                struct in_pktinfo *pktinfo = (struct in_pktinfo *)CMSG_DATA(cmsg);
                inet_ntop(AF_INET, &pktinfo->ipi_addr,
                          stpnetwork_info->stuser_recv_handle.stDstNet.ip,
                          sizeof(stpnetwork_info->stuser_recv_handle.stDstNet.ip));
                stpnetwork_info->stuser_recv_handle.stDstNet.port =
                    ntohs(src_addr.sin_port); // 目的端口和绑定端口相同
            }
        }

        /* 回调处理数据 */
        stpnetwork_info->fnhandledata(&stpnetwork_info->stuser_recv_handle);
    }
	printf("os_networkmulticast_recv exit\n"); 
	return NULL;

}

/*单播接收处理线程*/
void* os_networkunque_recv(void* network_info1)
{
	int nrecv_size = 0;
	NetworkMulticast_S* stpnetwork_info = (NetworkMulticast_S* )network_info1;

	if( stpnetwork_info == NULL)
	{
		return NULL;
	}
	if( stpnetwork_info->fnhandledata == NULL)
	{
		return NULL;

	}

	/*数据大于DATASIZE 分段发送*/
	if( stpnetwork_info->nread_size <= 0 )
	{
		nrecv_size = DATASIZE;
	}
	else if( stpnetwork_info->nread_size >= DATASIZE )
	{
		nrecv_size = DATASIZE;
	}
	else
	{
		nrecv_size =  stpnetwork_info->nread_size;
	}
	while (!stpnetwork_info->nExit)
	{
		int datalenght=0;
		char data[DATASIZE];
        struct msghdr msg;
		struct sockaddr_in sock_addr;
		struct sockaddr_in dst_addr;
        struct iovec iov[1];
        struct cmsghdr *cmhp;
        struct in_pktinfo *pktinfo = NULL; /* 用于指向获取的本地地址信息 */
        char buff[CMSG_SPACE(sizeof(struct in_pktinfo) + CMSG_SPACE(sizeof(int)))] = {0};   /* 控制信息 */
        struct cmsghdr *cmh = (struct cmsghdr *)buff;   /* 控制信息 */
		memset(data, 0, DATASIZE); 
		memset(&sock_addr, 0, sizeof(sock_addr));
		int addr_length=sizeof(sock_addr);
		#if 0
        datalenght=recvfrom(stpnetwork_info->nfd, data, nrecv_size, 0, (struct sockaddr *)&sock_addr,(socklen_t*)&addr_length);
		if(datalenght < 0) 
			break;
		memcpy(stpnetwork_info->stuser_recv_handle.data, data, datalenght);
		stpnetwork_info->stuser_recv_handle.lenght = datalenght;
		sprintf(stpnetwork_info->stuser_recv_handle.stNetwork.ip,"%s",inet_ntoa(sock_addr.sin_addr));
		stpnetwork_info->stuser_recv_handle.stNetwork.port = ntohs(sock_addr.sin_port);
        //OS_ThrHndl handle_thr;
		//OS_thrCreate(&handle_thr, stpnetwork_info->fnhandledata, OS_DETACH, OS_THR_STACK_SIZE_DEFAULT, &stpnetwork_info->stuser_recv_handle );
		#endif
		msg.msg_name = &sock_addr;     // 存储报文来源地址,端口
        msg.msg_namelen = sizeof(struct sockaddr_in);
        msg.msg_iov = &iov[0];
        msg.msg_iovlen = 1;
        msg.msg_control = cmh;
        msg.msg_controllen = sizeof(data);    
        iov[0].iov_base = &data;
        iov[0].iov_len = sizeof(data);
		datalenght = recvmsg(stpnetwork_info->nfd, &msg, 0);
		if(datalenght <= 0) 
			break;
		stpnetwork_info->stuser_recv_handle.lenght = datalenght;
        /* 辅助信息 */
        msg.msg_control = cmh;
        msg.msg_controllen = sizeof(buff);
		memcpy(stpnetwork_info->stuser_recv_handle.data, msg.msg_iov[0].iov_base, datalenght);
		for (cmhp = CMSG_FIRSTHDR(&msg); cmhp; cmhp = CMSG_NXTHDR(&msg, cmhp)) 
        {
            if (cmhp->cmsg_level == IPPROTO_IP) 
            {
                if (cmhp->cmsg_type == IP_PKTINFO) 
                {
                    pktinfo = (struct in_pktinfo *)CMSG_DATA(cmhp);
                    dst_addr.sin_family = AF_INET;
                    dst_addr.sin_addr = pktinfo->ipi_addr;
                    /* 报文源地址、端口 */
		            sprintf(stpnetwork_info->stuser_recv_handle.stNetwork.ip,"%s",inet_ntoa(sock_addr.sin_addr));
		            stpnetwork_info->stuser_recv_handle.stNetwork.port = ntohs(sock_addr.sin_port);
                    /* 报文目的地址 */
                    #if 0
                    printf("saddr : %s \n", inet_ntoa(sock_addr.sin_addr));     
			        printf("dport : %d \n", ntohs(sock_addr.sin_port));
                    printf("daddr : %s \n", inet_ntoa(dst_addr.sin_addr));     
			        printf("dport : %d \n", ntohs(pktinfo->ipi_ifindex));
		            #endif
                    sprintf(stpnetwork_info->stuser_recv_handle.stDstNet.ip,"%s",inet_ntoa(dst_addr.sin_addr));
		            stpnetwork_info->stuser_recv_handle.stDstNet.port = ntohs(pktinfo->ipi_ifindex);
                }
            }
        }
        if(stpnetwork_info->nasync_deal == 0)
        {
            stpnetwork_info->fnhandledata( &stpnetwork_info->stuser_recv_handle );
        }
        else
        {
            OS_ThrHndl handle_thr;
		    OS_thrCreate(&handle_thr, stpnetwork_info->fnhandledata, OS_DETACH, OS_THR_STACK_SIZE_DEFAULT, &stpnetwork_info->stuser_recv_handle );
        }
	}
	printf("os_networkunque_recv exit\n"); 
	return NULL;

}


void *os_networkmulticast_init_thr(void* network_info)
{

	NetworkMulticast_S* stpnetwork_info = (NetworkMulticast_S*) network_info;
	/* 防止网络未初始化完毕，导致udp初始化失败 */
	while(os_network_socket(stpnetwork_info) == -1)
	{
		sleep(5);
	}
	/*异步发送*/
	if( 1 == stpnetwork_info->nasynchronous )
	{
		int nRet = OS_queCreate( &(stpnetwork_info->sthndl), stpnetwork_info->nasynchronous_size);
		if (nRet < 0 )
		{
			printf("OS_que create fail\n");
			return NULL;
		}

		OS_thrCreate(&stpnetwork_info->handle_thr, mucast_asynchronoussend_thr, OS_JOINABLE,OS_THR_STACK_SIZE_DEFAULT, stpnetwork_info );

	}
	if( NULL == stpnetwork_info->fnhandledata)
	{
		return NULL;
	}

	OS_thrCreate(&stpnetwork_info->recv_thr,
				 os_networkmulticast_recv,
				 OS_JOINABLE,
				 OS_THR_STACK_SIZE_DEFAULT,
				 stpnetwork_info);
	return NULL;
}


int os_networkmulticast_init(NetworkMulticast_S* stpnetwork_info)
{
	OS_ThrHndl 	init_thr;
	OS_thrCreate(&init_thr, os_networkmulticast_init_thr, OS_JOINABLE,OS_THR_STACK_SIZE_DEFAULT, (void*)stpnetwork_info );
	
	return 0;
}


/*单播初始化,初始化后会自动接受数据并处理*/
int os_networkunque_init( NetworkMulticast_S* stpnetwork_info)
{
	os_network_socket(stpnetwork_info);
	/*异步发送*/
	if( 1 == stpnetwork_info->nasynchronous )
	{
		int nRet = OS_queCreate( &(stpnetwork_info->sthndl), stpnetwork_info->nasynchronous_size);
		if (nRet < 0 )
		{
			printf("OS_que create fail\n");
			return -1;
		}

		OS_thrCreate(&stpnetwork_info->handle_thr, unque_asynchronoussend_thr, OS_JOINABLE,OS_THR_STACK_SIZE_DEFAULT, stpnetwork_info );

	}
	if( NULL == stpnetwork_info->fnhandledata)
	{
		return 0;
	}

	return OS_thrCreate(&stpnetwork_info->recv_thr, os_networkunque_recv,OS_JOINABLE,OS_THR_STACK_SIZE_DEFAULT, stpnetwork_info );
}

/*组播退出*/
int os_networkmulticast_exit( NetworkMulticast_S* stpnetwork_info )
{
	stpnetwork_info->nExit = 1;
	/* 等待线程退出，必须保证所有资源释放 */
	if( 1 == stpnetwork_info->nasynchronous )
	{
		OS_thrJoin(&(stpnetwork_info->handle_thr));
		OS_queDelete( &(stpnetwork_info->sthndl) );
	}
	shutdown(stpnetwork_info->nfd, SHUT_RD);
	
	if(close(stpnetwork_info->nfd) < 0) 
	{
		printf("closesocket failed with error fd = %d\n", stpnetwork_info->nfd);
		return 1;
	}
	OS_thrJoin(&(stpnetwork_info->recv_thr));
	return 0;
}

/*单播退出*/
int os_networkunque_exit( NetworkMulticast_S* stpnetwork_info)
{
	stpnetwork_info->nExit = 1;
	/* 等待线程退出，必须保证所有资源释放 */
	if( 1 == stpnetwork_info->nasynchronous )
	{
		OS_thrJoin(&(stpnetwork_info->handle_thr));
		OS_queDelete( &(stpnetwork_info->sthndl) );
	}
	shutdown(stpnetwork_info->nfd, SHUT_RD);
	
	if(close(stpnetwork_info->nfd) < 0) 
	{
		printf("closesocket failed with error fd = %d\n", stpnetwork_info->nfd);
		return 1;
	}
	OS_thrJoin(&(stpnetwork_info->recv_thr));
	return 0;
}
