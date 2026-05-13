#include <stdio.h>
#include <stdlib.h>

#include "share_os.h"
#include "dlog.h"
#include "share_socket.h"
#include "communtication.h"

#if 1
static int communtication_process_clientHeartMsg(Communtication_Head_t *head, char *buf, int buflen, Communtication_Handle_t *handle)
{
	if(handle == NULL || handle->client_socket < 0) {
		return -1;
	}

	//dlog(LOG_DEBUG,"communtication_process_clientHeartMsg\n");
	int socket = handle->client_socket;
	int sendlen = 0;
	Communtication_Head_t head_s ;
	memcpy(&head_s, head, sizeof(Communtication_Head_t));
	//dlog(LOG_DEBUG,"communtication_process_clientHeartMsg1\n");
	pthread_mutex_lock(&(handle->lock));
	//dlog(LOG_DEBUG,"communtication_process_clientHeartMsg4\n");
	sendlen = sizeof(head_s);
	RH_TcpSndBlockFd(socket, (char *)&head_s, &sendlen);
	//dlog(LOG_DEBUG,"communtication_process_clientHeartMsg5,buflen=%d\n",buflen);
	sendlen = head_s.total_len;
	//dlog(LOG_DEBUG,"communtication_process_clientHeartMsg6,buflen=%d\n",buflen);
	RH_TcpSndBlockFd(socket, buf, &sendlen);
	//dlog(LOG_DEBUG,"communtication_process_clientHeartMsg6\n");
	pthread_mutex_unlock(&(handle->lock));

	if(handle->DealHeartbitFuncPtr != NULL) {
		handle->DealHeartbitFuncPtr(buf);
	}

	//dlog(LOG_DEBUG,"communtication_process_clientHeartMsg2\n");
	return 0;
}
#endif

static int communtication_clientHeartThread(void *argv)
{
	char ip[16] = "127.0.0.1";
	unsigned short port = 3100;
	Commutication_Handle_t handle = (Commutication_Handle_t)argv;
	int ret = 0;
	char RecvBuf[4096] = {0};
	int RecvLen = 0;
	//	int NoRecvTime = 0;
	int client_socket = -1;
	int needlen = 0;
	Communtication_Head_t headbuf;

	if(handle == NULL) {
		dlog(LOG_ERROR, "handle is NULL\n");
		//printf_pthread_delete(__FILE__, (char *)__func__);
		pthread_detach(pthread_self());
		pthread_exit(0);
		return -1;
	}

	snprintf(ip, sizeof(ip), "%s", handle->ip);
	port = handle->port;

	Communtication_Head_t *head = NULL;
	//char headbuf[256] = {0};
	sleep(2);
REPEAT_CONNECT:
	dlog(LOG_WARN, "client will connet to server [%s:%d]\n", ip, port);

	handle->client_socket = -1;

	if(client_socket > 0) {
		RH_Close(__FILE__, (char *)__func__, client_socket);
		client_socket = -1;
	}

	client_socket = 	RH_CreateTcpNoBindFd();
	//	RH_SetNonBlockFd(client_socket);


	ret =  RH_ConnetBlockFd(client_socket, port, ip);

	if(ret != RHRETSUCCESS) {
		dlog(LOG_ERROR, "connet server fd is failed,the pot is [%d]\n", port);
		usleep(1000000);
		goto REPEAT_CONNECT;
	}

	dlog(LOG_DEBUG, "connet [%s:%d] is ok\n", ip, port);
	handle->client_socket = client_socket;
	communtication_set_handleStatus(handle, START_STATUS);

	//init ;
	if(handle->ConnectServerInitPtr != NULL) {
		handle->ConnectServerInitPtr();
	}

	RH_SetRcvTimeoutFd(client_socket, 3, 0);

	//�����Ļ�����Ҫ����һЩ��ʼ���߼���
	//����room��������HD��������Ҫ�����·� control�Ĳ���������ģ�飬�����ڴ˼���һ���ص�������

	while(communtication_get_handleStatus(handle) == START_STATUS) {
		needlen = sizeof(Communtication_Head_t);
		memset(&headbuf, 0, sizeof(headbuf));
	    ret = RH_TcpRcvBlockFd(client_socket, (char *)(&headbuf), needlen, &RecvLen);

		if(ret != RHRETSUCCESS || RecvLen != needlen) {
			dlog(LOG_ERROR, "tcp recv failed,the port is [%u].\n", port);
			usleep(500000);
			goto REPEAT_CONNECT;
		}

		//check head
		if(communtication_check_head(&headbuf) != 0) {
			dlog(LOG_ERROR, "%s,communtication_check_head is failed,the port = [%u].\n",__func__, port);
			usleep(500000);
			goto REPEAT_CONNECT;
		}

		head = &headbuf;
		needlen = head->total_len;
		memset(&RecvBuf, 0, sizeof(RecvBuf));
		ret =  RH_TcpRcvBlockFd(client_socket, RecvBuf, needlen, &RecvLen);

		
		if(ret != RHRETSUCCESS || RecvLen != needlen) {
			dlog(LOG_ERROR, "Commutication tcp recv is failed,the cmd is [%u],the port = [%u]\n", head->cmd, port);
			usleep(500000);
			goto REPEAT_CONNECT;
		}

		//����Ҳ��Ҫ�ϱ�
		if(head->cmd == HEARTBIT_CMD) {
			communtication_process_clientHeartMsg(head, RecvBuf, RecvLen, handle);
		} else {
			if(handle->DealCmdFuncPtr != NULL) {
				handle->DealCmdFuncPtr(head, RecvBuf, handle);
			}
		}
	}


	handle->client_socket = -1;

	if(client_socket > 0) {
		RH_Close(__FILE__, (char *)__func__, client_socket);
		client_socket = -1;
	}

	//printf_pthread_delete(__FILE__, (char *)__func__);
	pthread_detach(pthread_self());
	pthread_exit(0);
	return 0;
}

static void* communtication_conv_clientHeartThread(void *argv)
{
		communtication_clientHeartThread(argv);
		return NULL;
}


int communtication_send_clientMsg(Communtication_Head_t *head, char *date, int buflen, Communtication_Handle_t *handle)
{
	if(handle == NULL || handle->status != START_STATUS || handle->client_socket < 0) {
		return -1;
	}

	//char buff[DEFATULT_COMMUTICATION_TOTAL_LEN] = {0};
	int socket = handle->client_socket;
	int sendlen = 0;

	//memcpy(buff, date, buflen);
	//Communtication_Head_t head_s ;
	//	memcpy(&head_s, head, sizeof(Communtication_Head_t));

	head->total_len = buflen;
	pthread_mutex_lock(&(handle->lock));
	sendlen = sizeof(Communtication_Head_t);
	RH_TcpSndBlockFd(socket, (char *)(head), &sendlen);
	sendlen =  head->total_len;
	RH_TcpSndBlockFd(socket, date, &sendlen);
	pthread_mutex_unlock(&(handle->lock));
	return 0;

}


/*����handle��ͬʱ������̨���߳�*/
Commutication_Handle_t communtication_create_clientHandle(char *dst_ip, unsigned short dst_port, DealCmdFunc func1, DealheartbitFunc func2, ConnectServerInitFunc func3)
{
	if(dst_ip == NULL || dst_port == 0) {
		dlog(LOG_ERROR, "local_ip or local_port is error\n");
		return NULL;
	}

	dlog(LOG_DEBUG, "-------------dst_ip[%s]dst_port[%d]\n", dst_ip, dst_port);

	Commutication_Handle_t handle = NULL;
	char task_name[128] = {0};
	int ret = 0;
	handle = (Commutication_Handle_t)r_malloc(sizeof(Communtication_Handle_t));

	if(handle == NULL) {
		dlog(LOG_ERROR, "Malloc handle is failed\n");
		return NULL;
	}

	pthread_mutex_t mutex;
	pthread_t tid;
	pthread_mutex_init(&mutex, NULL);

	//init handle
	memset(handle, 0, sizeof(Communtication_Handle_t));
	handle->lock = mutex;

	communtication_set_handleStatus(handle, NO_INIT_STATUS);
	handle->seq_num = 0;
	snprintf(handle->ip, sizeof(handle->ip), "%s", dst_ip);
	handle->port = dst_port;
	handle->DealCmdFuncPtr = func1;
	handle->DealHeartbitFuncPtr = func2;
	handle->ConnectServerInitPtr = func3;

	snprintf(task_name, sizeof(task_name), "clientcommutication_%d", dst_port);
	//printf_pthread_create(__FILE__, ("communtication_clientHeartThread"));

	//mid_task������8168ƽ̨��װ
	ret = pthread_create(&tid, NULL, communtication_conv_clientHeartThread, (void *)(handle));
	if(ret != 0) {
		dlog(LOG_ERROR, "crate communtication client thread failed\n");
		//mid_task_delay(3000);
		//��һ���ĸ��ʻᵼ�¶δ��󣬿��������׶ε��ã����Խ��ܡ�
		communtication_free_head(&handle);
		return NULL;
	}

	return handle;
}
