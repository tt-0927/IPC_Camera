#ifndef NET_BASE_H
#define NET_BASE_H
#include "public_define.h"
#include "share_socket.h"
#include "xml_base.h"
#include "pthread.h"
#include "list_base.h"
#include "list_use_lock.h"
#include "memory_leak.h"

#ifdef __cplusplus
extern "C" {
#endif
#define SDK_AACAUDIO_STREAM 1000001
#define SDK_H264_STREAM 1000002
#define SDK_H265_STREAM 1000003
#define SDK_MJPEG_STREAM 1000004
#define SDK_G711A_AUDIO_STREAM 1000005
#define SDK_G711U_AUDIO_STREAM 1000006
#define SDK_G722_AUDIO_STREAM 1000007
#define SDK_G726_AUDIO_STREAM 1000008
#define SDK_MP2L2_AUDIO_STREAM 1000009

#define SDK_NET_HEARTBIT_CMD 30032
#define SDK_STANDARD_CODE 0xffffff;
typedef void * Sdk_Net_Handle_t;
typedef void * Sdk_ServerNet_Handle_t;
typedef void *(*pthread_fun_sdk) (void *);
typedef enum Protocol_Type
{
    //内部私有协议
    SDK_PRIVATE_DEFAULT = 0,
    //公司对接的标准协议，统一规范
    SDK_COMPANY_STANDARD = 1,
    SDK_COMPANY_VISCA = 2,
}Protocol_Type_E;
typedef enum Net_Status
{
    SDK_NET_DISCONNECT = 0,
    SDK_NET_CONNECT = 1,
    SDK_NET_ERROR,

}Net_Status_t;

typedef enum Value_Type
{
        SDK_INT = 0,
        SDK_SHORT,
        SDK_CHAR,
        SDK_FLOAT,
        SDK_DOUBLE,
        SDK_FILE,
        SDK_UINT,
}Value_Type_t;
//信息头结构
typedef struct Net_Msg_Head {
    char identifier[4]; //@#$&	标识符
    int version;
    int load_len;
    int msg_code;
    //unsigned short load_len;
    //unsigned short msg_code;
    int reserve;
}Net_Msg_Head_t;
//强制1字节对齐
#pragma pack(1)
typedef struct Net_ComPanyStandard_Head
{
    //公司名称BL
    char u16CompanyName[2];
    //项目代号
    unsigned short u16ProjectCode;
    //数据长度
    unsigned short u16DataLen;
    //扩展字段，代表头要扩展几个字节，1代表可扩展1字节，最多可扩展256，不用填0即可
    unsigned char u16ExpansionLen;
}Net_ComPanyStandard_Head_S;
#pragma pack()

typedef struct _NetCallBackMesssage_
{
    void* InParam;//用来回调传出去的参数
    int Code;//哪种命令
    int nLen;
    char * recvvalue;//接收的内容
    Sdk_Net_Handle_t sOperHandle;
    //私有协议网络接收头的保留位
    Protocol_Type_E enProtocolType;//协议类型
    int nReserve;
}NetCallbackMsg_t;
typedef int(*NetDealbitFunc)(NetCallbackMsg_t*);
typedef int(*NetStatusFunc)(Net_Status_t status, Sdk_Net_Handle_t handle, void *inparam);
typedef int(*NetLogbackmsg)(const char *format, ...);//用于输出调试信息的函数指针
typedef int(*NetGetHeartMsg)(char *messege, int nLen,  Sdk_Net_Handle_t handle, void *inparam, int *outPutlen);//用于输出调试信息的函数指针
typedef struct InparamClientNet
{
    char ip[64];
    int nPort;
    NetDealbitFunc cmdfun;//命令处理回调
    NetStatusFunc statusFun;//网络连接状态通知（当服务器状态变化时）
    NetLogbackmsg logFun;//输出错误信息
    NetGetHeartMsg heartmsg;//服务器发送给客户端的信息
    void *param;//创建传进去的参数，回调带上来
    int overtime;//网络发送超时，以ms为单位，建议2秒
    int nReconnect;//是否重连
    int asynchronous_listnum;//异步发送最大缓冲，没有设置默认15
    int asynchronous;//异步发送吗
    int asynchronous_time;//异步发送链表取的时间，发送间隔长短设置，在100ms~2S之间
    Protocol_Type_E enProtocolType;//协议类型
    unsigned short u16ProjectCode;//项目代号
    /*无感，心跳单向重连的状态*/
    int nSetStatus;
}InparamClientNet_t;


typedef struct tar_messege
{
    Sdk_Net_Handle_t handle;//初始化时的操作句柄
    int nodeindex;//默认为0，除非字段名完全一样，0代表第一个，1代表第2个，
    void ** valueHandle;//组装发送内容的句柄
}tar_messege_t;


typedef struct InparamServerNet
{
    short nPort;
    int asynchronous;//异步发送吗
    int asynchronous_listnum;//异步发送最大缓冲，没有设置默认15
    int asynchronous_time;//异步发送链表取的时间，发送间隔长短设置，在20ms~2S之间
    NetDealbitFunc cmdfun;//命令处理回调
    NetStatusFunc statusFun;//网络连接状态通知（当客户端有变化时）
    NetLogbackmsg logFun;//输出错误信息
    NetGetHeartMsg heartmsg;//服务器客户端心跳
    void *param;//传进去的参数，回调带上来
    int overtime;//网络发送超时，以ms为单位
    int nConnectMaxNum;//超过这个链接的服务器不处理直接踢掉
    int support_ipv6;
    Protocol_Type_E enProtocolType;//协议类型
    unsigned short u16ProjectCode;//项目代号
}InparamServerNet_t;

typedef struct OfferUserMessege
{
    char ip[64];
    int nPort;
}OfferUserMessege_t;
typedef struct Protocol_Info_Head
{
    Net_Msg_Head_t stPrivateSendMsgHead;
    Net_Msg_Head_t stPrivateRecvMsgHead;
    Net_ComPanyStandard_Head_S stStandardSendMsgHead;
    Net_ComPanyStandard_Head_S stStandardRecvMsgHead;

}Protocol_Info_Head_S;
typedef struct Net_Opreate_Hanle
{
    int socket;
    OfferUserMessege_t usermessege;
    Xml_DocPtr_t pSendXmlHandle;
    Xml_DocPtr_t pRecvXmlHandle;
    char* pRecvMessege;
    InparamClientNet_t inParam;
    pthread_t client_tid;
    pthread_t heart_tid;
    pthread_t send_tid;//异步发送时用
    List_LockHandle_t* asynchronous_sendlist;//异步发送链表
    pthread_mutex_t lock;//互斥锁
    pthread_mutex_t netlock;//网络互斥锁
    Net_Status_t status;//代表是否要断开退出连接
    Net_Status_t current_status;//代表当前连接状态
    void *serverHandle;//如果是服务器，则此为服务器句柄
    void *serverClientParam;//如果是服务器，则代表连接后用户自己设置进来的指针，方便用户管理
    pthread_cond_t condHeart;
    pthread_mutex_t lockHeart;
    Protocol_Info_Head_S stProtocolHeadInfo;
    int nReserve;//接收保留值
}Net_Opreate_Hanle_S;

//client********************************************************************************
/**
*@ref 连接服务器
*@param[in] InparamClientNet_t 见结构体解析
*@return 成功返回网络操作句柄，失败返回空
*/
Sdk_Net_Handle_t sdkclient_init_net(InparamClientNet_t netparm);



/*@ref 组装发送内容
*@param[in] tar_messege_t 见结构体解析
*@param[in]  keyname 字段名
*@param[in]  valuetype 字段类型
*@param[in]  value 字段值
*@return 成功返回RET_SUCCESS,失败见RetErr_t
*/
RetErr_t sdknet_add_value(tar_messege_t messege,const char * keyname, Value_Type_t valuetype, void *value);

/*@ref 发送内容到服务器
*@param[in] Sdk_Net_Handle_t 初始化时的操作句柄
*@param[in] valueHandle 组装内容的时的句柄即tar_messege_t中的valuehandle
*@param[in] code发送的命令码
*@return 成功返回RET_SUCCESS,失败见RetErr_t
*/
RetErr_t sdknet_send_value(Sdk_Net_Handle_t handle, void ** valueHandle, int code);

/*@ref 将内容发送到服务器
*@param[in] tar_messege_t 见结构体解析
*@param[in]  keyname 字段名
*@param[in]  valuetype 字段类型
*@param[in]  value 字段值
*@param[in]  获取内容的长度
*@return 成功返回RET_SUCCESS,失败见RetErr_t
*/
RetErr_t sdknet_get_value(tar_messege_t messege,const char *keyname, Value_Type_t valuetype, void *value, int nLen);



/**
*@ref 主动断开服务器
*@param[in] handle 操作句柄
*@return  成功返回RET_SUCCESS,失败见RetErr_t
**/
RetErr_t sdkclient_stop_net(Sdk_Net_Handle_t handle);
/**
*@ref 主动断开服务器,并且销毁句柄
*@param[in] handle 操作句柄
*@return  成功返回RET_SUCCESS,失败见RetErr_t
*/
RetErr_t sdkclient_uninit_net(Sdk_Net_Handle_t handle);

//server***************************************************************

/**
*@ref 创建服务器
*@param[in] InparamClientNet_t 见结构体解析
*@return 成功返回网络操作句柄，失败返回空
*/
Sdk_ServerNet_Handle_t sdkserver_init_net(InparamServerNet_t netserverparm);




/**
*@ref 客户端连接成功后会回调，这时可将参数设置进行保存，方便用户管理权限等
*@param[in] clienthandle 回调的连接客户端句柄
*@return 成功返回0，失败返回-1
*/
int setConnectClientParam(Sdk_Net_Handle_t clienthandle, void*clientParam);


/**
*@ref 获取用户自己设置进去的客户端参数，从而当断开连接后可知道具体哪个客户端断开
*@param[in] clienthandle 回调的连接客户端句柄
*@return 成功返回用户保存的参数，失败返回空
*/
void* getConnectClientParam(Sdk_Net_Handle_t clienthandle);
/**
*@ref 获取连接客户端的一些信息，方便管理
*@param[in] clienthandle 回调的连接客户端句柄
*@return 成功返回0失败返回-1
*/

int getUsermessege(Sdk_Net_Handle_t clienthandle, OfferUserMessege_t * messege);




//公用发送**********************************************************

/**
*@ref 发送信息给连接的客户端或者服务器
*@param[in] pOprHandle 发送的句柄
*@param[in] message 发送内容
*@param[in] code 发送类型
*@param[in] nLen 内容长度
*@return 成功返回0失败返回-1
*/
int net_send_msg(Sdk_Net_Handle_t pOprHandle, char* message, int nLen, int code);

int net_send_msgdeal(Sdk_Net_Handle_t pOprHandle, char *message, int nLen, int code);

//目前支持客户端发送
int net_send_file(Sdk_Net_Handle_t pOprHandle, char* filename,  int code);
/**
*@ref 发送信息给连接的客户端或者服务器
*@param[in] pOprHandle 发送的句柄
*@param[in] message 内容
*@param[in] waitTime 等待时间
*@param[in] nLen 内容长度
*@return 成功返回0失败返回-1
*/
int net_recv_msg(Sdk_Net_Handle_t pOprHandle, char* message, int nLen, int waitTime);

/**
*@ref 服务器发送内容给所有客户端
*@param[in] pServerHandle 服务器的句柄
*@param[in] message 发送内容
*@param[in] code 发送类型
*@param[in] nLen 内容长度
*@return 成功返回0失败返回-1
*/
int netserver_send_allClient(Sdk_ServerNet_Handle_t pServerHandle, char* message, int nLen, int code);

pthread_mutex_t * sdk_getlist_serverlock(Sdk_ServerNet_Handle_t pServerHandle );

List_CurNode_t sdk_list_begin_clientHandle(Sdk_ServerNet_Handle_t pServerHandle ,
        Sdk_Net_Handle_t *pOprHandle);
List_CurNode_t  sdk_list_next_clientHandle(Sdk_ServerNet_Handle_t pServerHandle ,
        Sdk_Net_Handle_t *pOprHandle, List_CurNode_t listNode);
List_CurNode_t  sdk_list_end_clientHandle(Sdk_ServerNet_Handle_t pServerHandle );


//默认设置线程栈64KB
int sdk_pthread_create(pthread_t *thread_id,const pthread_attr_t *user_attr, pthread_fun_sdk funtion, void *argv);

int init_heart_signal(Net_Opreate_Hanle_S *pstNetHandle);
int uninit_heart_signal(Net_Opreate_Hanle_S *pstNetHandle);
int sdkpthread_condl_signal(pthread_cond_t* pCondl,pthread_mutex_t *pLock);

#ifdef __cplusplus
}
#endif
#endif // NET_BASE_H
