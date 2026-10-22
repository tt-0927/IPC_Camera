/*
 * @FilePath: communtication.h
 * @Author: yangwenyao
 * @Date: 2022-11-29 16:22:38
 * @LastEditors: Please set LastEditors
 * @LastEditTime: 2022-12-09 10:38:01
 * @Descripttion: 
 */
#ifndef _COMMUNTICATION_H__
#define _COMMUNTICATION_H__

#include <pthread.h>
typedef enum{
	CLIENT_TO_SERVER = 0,
	REPEAT_CLIENT_TO_SERVER,
	SERVER_TO_CLIENT,
	REPEAT_SERVER_TO_CLIENT
}CMD_DIRECTION;


typedef enum{	
	RET_CMD_SUCCESS,
	RET_ERROR_STRUCT_LEN,
	RET_ERROR_INFO,
	RET_UNKNOWN_CMD
}CMD_RETURN_CODE;

typedef struct COMMUNTICATION_HEAD_S{
	unsigned char  check_start[4]; /*$$$$,标示符*/
	unsigned int   identifier;     /*区分从那个client到那个server*/
	unsigned int   total_len;     /*后面荷载的总长度*/
	unsigned int   struct_len;    /*真实结构体的长度 《= total_len*/
	unsigned int   cmd;     /*case */
	unsigned int   return_code;   /*返回值*/
	unsigned int   seq_num;       /*序列值*/
	CMD_DIRECTION direction;
	unsigned int   check_end[4]; /*####,标示符*/
}Communtication_Head_t;


typedef enum {
	NO_INIT_STATUS = 0,
//	BEGIN_START_STATUS = 1,
	START_STATUS =1,
	BEGIN_STOP_STATUS = 2,
	STOP_STATUS
}COMMUNTICATION_STATUS;

typedef  struct _COMMUTICATION_HANDLE * Commutication_Handle_t;

typedef int (*DealCmdFunc)(Communtication_Head_t *,void * ,Commutication_Handle_t );
typedef int (*getheartbitvalue)(char *,int *);
typedef int (*DealheartbitFunc)(char *);
typedef int (*ConnectServerInitFunc)();


typedef struct _COMMUTICATION_HANDLE{
	int connectNum;
	pthread_mutex_t lock;     //互斥锁
	int status; //当前状态 连接中，断连中
	unsigned int seq_num;  //累加值，表示第几条信令。
//	unsigned int server_socket ;  //server socket
	 int client_socket;
	char ip[16];  /*server ,ip表示本地ip, client,ip表示目的地IP*/
	int  port;    /*server,port表示本地监听port, client ,port表示目的的port*/
	DealCmdFunc DealCmdFuncPtr;
	getheartbitvalue GetHeartbitvaluePtr;
	DealheartbitFunc DealHeartbitFuncPtr;
	ConnectServerInitFunc ConnectServerInitPtr;
}Communtication_Handle_t;




#define DEFAULT_CHECK_START_CODE '$'
#define DEFAULT_CHECK_END_CODE   '#'

#define COMMUNTICATION_HEAD_LEN  sizeof(Communtication_Handle_t)
#define DEFATULT_COMMUTICATION_TOTAL_LEN      4096





#define HEARTBIT_CMD 0x99

Commutication_Handle_t communtication_create_clientHandle(char *dst_ip, unsigned short dst_port, DealCmdFunc func1,DealheartbitFunc func2,ConnectServerInitFunc func3);

Commutication_Handle_t communtication_create_serverHandle(char *local_ip, unsigned short local_port, DealCmdFunc func1, getheartbitvalue func2);
int communtication_send_serverMsg(Communtication_Head_t *head, char *date, int buflen, Communtication_Handle_t *handle);
int commutication_upload_heart(char *date, int len, Communtication_Handle_t *handle);

int communtication_set_handleStatus(Commutication_Handle_t handle, int status);
int communtication_get_handleStatus(Commutication_Handle_t handle);
int communtication_check_head(Communtication_Head_t *head);
void communtication_free_head(Commutication_Handle_t *handle);
//int communtication_send_serverMsg(Communtication_Head_t *head, char *date, int buflen, Communtication_Handle_t *handle);
int commutication_init_head(Communtication_Head_t *head, int identifier);
int communtication_send_clientMsg(Communtication_Head_t *head, char *date, int buflen, Communtication_Handle_t *handle);



#endif


