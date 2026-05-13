#ifndef _NETWORK_LIBEVENT_CODE_COMMUNITICATION_H_
#define _NETWORK_LIBEVENT_CODE_COMMUNITICATION_H_

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "list_single.h"
#include "os.h"
#include "os_mutex.h"
#include "os_thr.h"
#include "os_sem.h"

#ifdef __cplusplus
extern "C"
{
#endif

#define NETWORK_AACAUDIO_STREAM 1000001
#define NETWORK_H264_STREAM 1000002

#define NETWORK_NET_HEARTBIT_CMD 30032

#define NETWORK_TIME_OUT (6)			// 超时时间，低于4s的话，对接原来的sdk的话，会出现超时，因为原来的sdk心跳间隔为3s多一点，
#define NETWORK_HEART_INTERVAL_TIME (2) // 心跳间隔时间

	typedef void *network_Handle_t;

	typedef enum Network_Status_
	{
		NETWOKR_STATUS_DISCONNECT = 0, // 断开连接
		NETWORK_STATUS_CONNECT = 1,	   // 正常通讯
		NETWORK_STATUS_RECONNECT = 2,  // 重连中
		NETWORK_STATUS_ERROR,		   // 网络错误

	} network_Status_t;

	// 信息头结构
	typedef struct network_Msg_Head_
	{
		char identifier[4]; //@#$&	标识符
		int version;		// 版本号
		int load_len;		// bady长度
		int msg_code;		// 消息命令
		int reserve;

		/*
		 * 下面新增的字段是后期改动再增加，zhangjb 20200527
		 * */
		// unsigned long long key;	//每条命令增加key，便于发送端将接收到的响应匹配发送的请求
		// int identity;				//每条命令都加上身份，便于接收端识别身份
	} network_HeadInfo_t;

	typedef struct _NetCallBackMesssage
	{
		void *InParam; // 用来回调传出去的参数
		int Code;	   // 哪种命令
		int nLen;
		char *recvvalue;			   // 接收的内容
		network_Handle_t clientHandle; // 返回客户端的句柄：详见：network_clientInfo_t

	} network_CallbackMsg_t;

	typedef int (*network_DealbitFunc)(network_CallbackMsg_t *);
	typedef int (*network_StatusFunc)(network_Status_t status, network_Handle_t handle, void *inparam);
	typedef int (*network_Logbackmsg)(const char *format, ...);															 // 用于输出调试信息的函数指针
	typedef int (*network_GetHeartMsg)(char *messege, int nLen, network_Handle_t handle, void *inparam, int *outPutlen); // 用于输出调试信息的函数指针

	// 客户端传入的初始化参数
	typedef struct network_Inparam_ClientNet_
	{
		char ip[64]; // 支持ipv6
		int nPort;
		network_DealbitFunc cmdfun;	  // 命令处理回调
		network_StatusFunc statusFun; // 网络连接状态通知（当服务器状态变化时）
		network_Logbackmsg logFun;	  // 输出错误信息
		network_GetHeartMsg heartmsg; // 服务器发送给客户端的信息
		void *param;				  // 创建传进去的参数，回调带上来
		int overtime;				  // 网络发送超时，以ms为单位，建议2秒
		int nReconnect;				  // 是否重连:1-重连，0-不重连
		int asynchronous_listnum;	  // 异步发送最大缓冲，没有设置默认15
		int asynchronous;			  // 异步发送吗,0-否，1-是
	} network_inparamClientNet_t;

	// 服务器传入的初始化参数
	typedef struct network_InparamServerNet_
	{
		int nPort;
		int asynchronous;			  // 异步发送吗
		int asynchronous_listnum;	  // 异步发送最大缓冲，没有设置默认15
		network_DealbitFunc cmdfun;	  // 命令处理回调
		network_StatusFunc statusFun; // 网络连接状态通知（当客户端有变化时）
		network_Logbackmsg logFun;	  // 输出错误信息
		network_GetHeartMsg heartmsg; // 服务器发送给客户端的信息
		void *param;				  // 传进去的参数，回调带上来
		int overtime;				  // 网络发送超时，以ms为单位
		int ipv6;					  // 0-默认不开启ipv6，1-开启ipv6
	} network_inparamServerNet_t;

	typedef struct _NET_RPC_PACKET_
	{
		char *data;
		int dataSize;
		int memtype; // 内存类型，0-栈内存，1-堆内存
	} netPacket_t;

	typedef struct _NET_RPC_SYNC_
	{
		OS_SemHndl sem; // 信号量
		netPacket_t pkt;
	} netRpc_t;

	network_Handle_t network_init_server(network_inparamServerNet_t inparam);
	int network_unInit_server(network_Handle_t *server_Handle);

	network_Handle_t network_init_client(network_inparamClientNet_t inparam);
	int network_unInit_client(network_Handle_t *client_Handle);

	int network_client_reconnect(network_Handle_t *client_Handle);

	int network_send_data_sync(network_Handle_t clientHandle, char *message, int nLen, int code, int timeOut, netPacket_t *outPkt);
	int network_release_syncPkt(network_Handle_t clientHandle, netPacket_t *pkt);

	int network_send_data(network_Handle_t clientHandle, char *message, int nLen, int code);
	int network_serverSendData_toAllClient(network_Handle_t serverHandle, char *message, int nLen, int code);

	int network_serverGetClient_lock(network_Handle_t serverHandle);
	int network_serverGetClient_unLock(network_Handle_t serverHandle);
	OS_listNode_t network_server_listBegin(network_Handle_t serverHandle, network_Handle_t *client);
	OS_listNode_t network_server_listEnd(network_Handle_t serverHandle);
	OS_listNode_t network_server_listNext(network_Handle_t serverHandle, OS_listNode_t listNode, network_Handle_t *client);

#ifdef __cplusplus
}
#endif
#endif //_NETWORK_LIBEVENT_CODE_COMMUNITICATION_H_
