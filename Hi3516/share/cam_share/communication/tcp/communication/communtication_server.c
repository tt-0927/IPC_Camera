#include <stdio.h>
#include <stdlib.h>


#include "dlog.h"
#include "share_os.h"
#include "share_socket.h"
#include "communtication.h"

static void* CommunticationServerConvProcess(void *argv);
static int CommunticationServerProcess(void *argv);

void communtication_free_head(Commutication_Handle_t *handle)
{
	Commutication_Handle_t temp = *handle;

	if(temp != NULL) {
		pthread_mutex_destroy(&(temp->lock));
		r_free(temp);
		*handle = NULL;
	}

	return ;
}

int communtication_get_handleStatus(Commutication_Handle_t handle)
{
	if(handle == NULL) {
		dlog(LOG_ERROR, "handle is NULL\n");
		return NO_INIT_STATUS;
	}

	return handle->status;
}

int communtication_set_handleStatus(Commutication_Handle_t handle, int status)
{
	if(handle == NULL) {
		dlog(LOG_ERROR, "handle is NULL\n");
		return -1;
	}

	handle->status = status;
	return 0;
}

int communtication_check_head(Communtication_Head_t *head)
{
	if((head == NULL) || (head->check_start[0] != DEFAULT_CHECK_START_CODE)
	   || (head->check_start[1] != DEFAULT_CHECK_START_CODE)
	   || (head->check_start[2] != DEFAULT_CHECK_START_CODE)
	   || (head->check_start[3] != DEFAULT_CHECK_START_CODE)
	   || (head->check_end[0] != DEFAULT_CHECK_END_CODE)
	   || (head->check_end[1] != DEFAULT_CHECK_END_CODE)
	   || (head->check_end[2] != DEFAULT_CHECK_END_CODE)
	   || (head->check_end[3] != DEFAULT_CHECK_END_CODE)) {
		dlog(LOG_ERROR, "the head is not head\n");
		dlog(LOG_DEBUG, "[%c][%c][%c][%c][%c][%c][%c][%c]\n", head->check_start[0], head->check_start[1], head->check_start[2], head->check_start[3],
		      head->check_end[0], head->check_end[1], head->check_end[2], head->check_end[3]);
		return -1;
	}

	return 0;
}

int commutication_init_head(Communtication_Head_t *head, int identifier)
{
	memset(head, 0, sizeof(Communtication_Head_t));
	head->check_start[0] = head->check_start[1] = head->check_start[2] = head->check_start[3] = DEFAULT_CHECK_START_CODE;
	head->check_end[0] = head->check_end[1] = head->check_end[2] = head->check_end[3] = DEFAULT_CHECK_END_CODE;
	head->identifier = identifier;
	head->total_len = DEFATULT_COMMUTICATION_TOTAL_LEN;

	return 0;
}



static int CommunticationServerThread(void *argv)
{
	int server_socket = -1;
	int client_socket = -1;
	char ip[16] = "127.0.0.1";
	unsigned short port = 3100;
	pthread_t tid;
	int ret  = -1;
	char task_name[64] = {0};
	Commutication_Handle_t handle = argv;

	if(handle == NULL) {
		dlog(LOG_ERROR, "handle is NULL\n");
		goto Exit_pthread;
	}

	snprintf(ip, sizeof(ip), "%s", handle->ip);
	port = handle->port;

	snprintf(task_name, sizeof(task_name), "CommunticationServerThread_%d", port);

	//����server socket bind;
SERVERSTARTRUN:
	handle->connectNum = 0;
	if(handle->client_socket != -1) {
		dlog(LOG_WARN, "client_socket[%d] is not null.,port[%u].\n", client_socket, port);
		RH_NonroughClose(handle->client_socket);
		client_socket = -1;
		handle->client_socket = -1;
	}

	communtication_set_handleStatus(handle, STOP_STATUS);
	server_socket = -1;
	client_socket = -1;
	server_socket = RH_CreateTcpBindFd(port, ip);

	if(server_socket < 0) {
		dlog(LOG_ERROR, "server_socket create failed,port[%u].\n", port);
		goto Exit_pthread;
	}

	if(listen(server_socket , 1) < 0) {
		dlog(LOG_ERROR, "listen error:%d,error msg:=%s,port[%u].\n", errno, strerror(errno), port);
		goto Exit_pthread;
	}

	//����socket��������������ģʽ
	if(RH_SetBlockFd(server_socket) != RHRETSUCCESS) {
		dlog(LOG_ERROR, ",set socket block error:%d,error msg:=%s,port[%u].\n", errno, strerror(errno), port);
		goto Exit_pthread;
	}

	//accpet socket. sclinet_socket.
	while(1) {
	/*	if(communtication_get_handleStatus(handle) != STOP_STATUS) {
			communtication_set_handleStatus(handle, BEGIN_STOP_STATUS);
			//mid_task_delay(1000);
			continue;
		}
	*/
		if(handle->connectNum == 1)
		{
			//dlog(LOG_DEBUG, "handle->connectNum:%d\n", handle->connectNum);
			sleep(1);
			continue;

		}
		dlog(LOG_DEBUG, "[%s:%d]waiting for clinet\n", ip, port);
		client_socket = RH_GetConnectBlockFd(server_socket);
		if(client_socket >= 0) {
			//mid_task������8168ƽ̨��װ
			//�����߼���ͬʱֻ�ܴ���һ���̴߳�����
			handle->client_socket = client_socket;
			//printf_pthread_create(__FILE__, ("CommunticationServerProcess"));
			ret = pthread_create(&tid, NULL, CommunticationServerConvProcess, (void *)(handle));
			if(ret != 0) {
				dlog(LOG_ERROR, "crate communtication server thread failed\n");
				//mid_task_delay(3000);
				//��һ���ĸ��ʻᵼ�¶δ��󣬿��������׶ε��ã����Խ��ܡ�
				goto SERVERSTARTRUN;
			}
			handle->connectNum++;
		} else {
			if(errno == ECONNABORTED || errno == EAGAIN) {

			}
			sleep(1);

		}

	}

Exit_pthread:
	//printf_pthread_delete(__FILE__, (char *)__func__);
	pthread_detach(pthread_self());
	pthread_exit(0);
	return 0;
}
static void* CommunticationServerConvThread(void *argv)
{
	CommunticationServerThread(argv);
	return NULL;
}
static int CommunticationServerProcess(void *argv)
{
	int ret = 0;
	char RecvBuf[4096] = {0};
	char HeartBitBUF[DEFATULT_COMMUTICATION_TOTAL_LEN] = {0};
	int RecvLen = 0;
	unsigned short port = 0;
	int NoRecvTime = 0;
	int client_socket = 0;
	Communtication_Head_t *head = NULL;
	Communtication_Head_t headbuf;
	Commutication_Handle_t handle = (Commutication_Handle_t)argv;
	//	char headbuf[256] = {0};

	if(handle == NULL) {
		dlog(LOG_ERROR, "handle  is NULL\n");

		goto CLOSESOCKETEXIT;
	}

	//����status

	client_socket = handle->client_socket;
	dlog(LOG_WARN, "client_socket = %d\n", client_socket);
	//	RH_SetNonblockFd(client_socket);
	port = handle->port;

	communtication_set_handleStatus(handle, START_STATUS);


	//�ж�ѭ���������ӽ���ѭ��
	while(communtication_get_handleStatus(handle) == START_STATUS) {
		//recv ��ʱ3��
		RecvLen = sizeof(Communtication_Head_t);
		memset(&headbuf, 0, sizeof(headbuf));
		//dlog(LOG_WARN,"1client_socket = %d=%d=%d=%d\n",client_socket,ret,RecvLen,mid_clock());

		ret =  RH_TcpRcvNonblockFd(client_socket, (char *)(&headbuf), &RecvLen, 2000);

		//dlog(LOG_WARN,"2client_socket = %d=%d=%d=%d\n",client_socket,ret,RecvLen,mid_clock());
		if(ret != RHRETSUCCESS || RecvLen <= 0) {
			NoRecvTime ++;

			if(NoRecvTime >= 2) {
				dlog(LOG_ERROR, "NoRecvTime>2 Heart Error buf is failed,the port=[%u],errno=%d,<%s>\n", port, errno, strerror(errno));
				//socket �쳣�������˳���
				goto CLOSESOCKETEXIT;
			}

			//������������
			if(handle->GetHeartbitvaluePtr != NULL) {
				RecvLen = sizeof(HeartBitBUF);
				ret = handle->GetHeartbitvaluePtr(HeartBitBUF, &RecvLen);

				if(ret != 0) {
					dlog(LOG_ERROR, "getheartbitvalue is failed,the port=[%u]\n", port);
					//socket �쳣�������˳���
					goto CLOSESOCKETEXIT;
				}

				ret = commutication_upload_heart(HeartBitBUF, RecvLen, handle);

				if(ret != 0) {
					dlog(LOG_ERROR, "commutication_upload_heart is failed,the port=[%u]\n", port);
					//socket �쳣�������˳���
					goto CLOSESOCKETEXIT;
				}

				//dlog(LOG_DEBUG, "i will upload heartbit,the port = [%u][%d]\n", port, RecvLen);
			}

			continue;
		}

		NoRecvTime = 0;

		//check head
		if(communtication_check_head(&headbuf) != 0) {
			dlog(LOG_ERROR, "%s,communtication_check_head is failed,the port = [%u].\n",__func__, port);
			goto CLOSESOCKETEXIT;
		}

		head = &headbuf;
		RecvLen = head->total_len;

		ret =  RH_TcpRcvNonblockFd(client_socket, RecvBuf, &RecvLen, 3000);

		if(ret != RHRETSUCCESS || RecvLen == 0) {
			dlog(LOG_ERROR, "RH_TcpRcvNonblockFd is failed,the cmd is [%d],the port = [%u].\n", head->cmd, port);
			goto CLOSESOCKETEXIT;
		}

		//�����������̲ſ��Խ��봦���߳�
		if(handle->DealCmdFuncPtr != NULL && head->cmd != HEARTBIT_CMD) {
			handle->DealCmdFuncPtr(head, RecvBuf, handle);
		}

		continue;
	}

CLOSESOCKETEXIT:
	dlog(LOG_WARN, "CommunticationServerProcess is failed,the port =[%u].\n ", port);

	if(handle != NULL) {
		handle->client_socket = -1;
	}
	handle->connectNum--;
	RH_NonroughClose(client_socket);
	communtication_set_handleStatus(handle, STOP_STATUS);
	//printf_pthread_delete(__FILE__, (char *)__func__);
	pthread_detach(pthread_self());
	pthread_exit(0);
	return 0;
}

static void* CommunticationServerConvProcess(void *argv)
{
	CommunticationServerProcess(argv);
	return NULL;
}

//���������ϱ��¼���
int commutication_upload_heart(char *date, int len, Communtication_Handle_t *handle)
{
	if(handle == NULL || handle->status != START_STATUS ||  handle->client_socket < 0) {
		dlog(LOG_ERROR, "hanle or status is failed <status :%d> <fd :%d>\n",
		      handle->status, handle->client_socket);
		return -1;
	}

	if(len > DEFATULT_COMMUTICATION_TOTAL_LEN) {
		dlog(LOG_ERROR, "heartbit buff len[%d] is too big\n", len);
		return -1;
	}

	char buff[DEFATULT_COMMUTICATION_TOTAL_LEN] = {0};
	int ret  = 0;
	unsigned short port = handle->port;

	Communtication_Head_t head_s ;
	commutication_init_head(&head_s, port);
	head_s.total_len = DEFATULT_COMMUTICATION_TOTAL_LEN;
	head_s.struct_len = len;
	head_s.cmd = HEARTBIT_CMD;
	head_s.return_code = 0;
	head_s.direction = 	SERVER_TO_CLIENT;

	memcpy(buff, date, len);
	ret =  communtication_send_serverMsg(&head_s, buff, head_s.total_len, handle);

	return ret;
}



/*��װ�����ڷ�������*/
int communtication_send_serverMsg(Communtication_Head_t *head, char *date, int buflen, Communtication_Handle_t *handle)
{
	if(handle == NULL || handle->status != START_STATUS || handle->client_socket < 0 || buflen > DEFATULT_COMMUTICATION_TOTAL_LEN) {
		dlog(LOG_ERROR, "communtication_send_serverMsg is failed,handle=%p\n", handle);

		if(handle != NULL) {
			dlog(LOG_ERROR, "status=%d,socket=%d,buflen=%d\n", handle->status, handle->client_socket, buflen);
		}

		return -1;
	}

	char buff[DEFATULT_COMMUTICATION_TOTAL_LEN] = {0};
	int socket = handle->client_socket;
	int sendlen = 0;

	memcpy(buff, date, buflen);
	head->total_len = buflen;
	pthread_mutex_lock(&(handle->lock));
	sendlen = sizeof(Communtication_Head_t);
	communtication_check_head(head);
	RH_TcpSndBlockFd(socket, (char *)head, &sendlen);
	sendlen = head->total_len;
	RH_TcpSndBlockFd(socket, buff, &sendlen);
	pthread_mutex_unlock(&(handle->lock));
	return 0;

}



/*����handle��ͬʱ������̨���߳�*/
Commutication_Handle_t communtication_create_serverHandle(char *local_ip, unsigned short local_port, DealCmdFunc func1, getheartbitvalue func2)
{
	if(local_ip == NULL || local_port == 0 || func1 == NULL || func2 == NULL) {
		dlog(LOG_ERROR, "local_ip or local_port is error\n");
		return NULL;
	}

	int ret = 0;
	char  task_name[64] = {0};
	Commutication_Handle_t handle = NULL;
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
	handle->client_socket = -1;

	communtication_set_handleStatus(handle, NO_INIT_STATUS);
	handle->seq_num = 0;
	snprintf(handle->ip, sizeof(handle->ip), "%s", local_ip);
	handle->port = local_port;
	handle->DealCmdFuncPtr = func1;
	handle->GetHeartbitvaluePtr = func2;

	snprintf(task_name, sizeof(task_name), "servercommutication_%d", local_port);
	//printf_pthread_create(__FILE__, ("CommunticationServerThread"));
	//mid_task������8168ƽ̨��װ

	ret = pthread_create(&tid, NULL, CommunticationServerConvThread, (void *)(handle));
	if(ret != 0) {
		dlog(LOG_ERROR, "crate communtication server thread failed\n");
		//mid_task_delay(3000);
		//��һ���ĸ��ʻᵼ�¶δ��󣬿��������׶ε��ã����Խ��ܡ�
		communtication_free_head(&handle);
		return NULL;
	}

	return handle;
}


