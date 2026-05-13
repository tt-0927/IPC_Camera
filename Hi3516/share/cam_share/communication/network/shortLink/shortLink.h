#ifndef COMMUNTICATION_SHORTLINK_H
#define COMMUNTICATION_SHORTLINK_H


#ifdef __cplusplus
extern "C"
{
#endif

#define SHORT_CHECK_START_CODE '$'
#define SHORT_CHECK_END_CODE   '#'

#define SHORTWAITIME 4000
typedef enum
{
	PRIVATE_WEB = 0,
	PUBLIC_RESOURCE = 1,
}Connect_Type_t;

typedef enum
{
	SET_CMD = 0,
	GET_CMD= 1,
}Cmd_Type_t;
typedef struct __Short_Public_Head {
	int nDataLen;
	unsigned char  check_start[4]; /*####,标示符*/
}Short_Public_Head_t;

typedef struct __Short_Msg_Head {
	unsigned char  check_start[4]; /*$$$$,标示符*/
	int identifier;
	unsigned int  nDataLen;  //数据长度
	unsigned int   cmd;     /*case */
	unsigned int   return_code;   /*返回值*/
	unsigned int   check_end[4]; /*####,标示符*/
}Short_Msg_Head_t;

typedef void * ShortOperateHandle;
typedef void * ShortLinkServerHandle;
typedef void * shortLinkClientHandle;

typedef struct _ShortCallBackMesssage
{
	void* InParam;//刚开始创建传递进来的参数
	char *value;//内容
	int nLen;//内容的长度
	int Code;//哪种命令
	int result ;
	int nCmdType;//设置或者获取
	ShortOperateHandle sOperHandle;
	char ip[16];
}ShortCallbackMsg_t;


typedef enum {
	SHORT_NOINIT_STATUS = 0,
	SHORT_START_STATUS =1,
	SHORT_STOP_STATUS,
}ShortLink_Status_t;
typedef int (*ShortLinkDealCmdFunc)(ShortCallbackMsg_t*);

typedef struct
{
	void* message;
	int nLen;
	int code;
	int port;
	char iP[16];
	ShortLinkDealCmdFunc dealcmd;
	void* parm;
	int waitTime;//时间为0或者小于0默认2秒超时，否则为超时时间
}ShortLink_Send_t;


typedef struct _ShortLinkServerHandle
{
	int port;
	int nConNum;//客户端连接的个数
	int socket;//服务器的句柄
	int clientsocket;//临时客户端句柄，方便服务器与客户端通信
	int IsGetClientSock;//操作句柄是否获得了clientsocket
	ShortLinkDealCmdFunc dealCmd;
	void * param;//用户自己传进来的参数
	Connect_Type_t nType;//哪种连接方式
}ShortLinkServer_Handle_t;


typedef struct _web_cmd_struct
{
	int cmd_code;
	int (*cmd_handler)(ShortCallbackMsg_t* argv);
}Web_Cmd_Struct_t;
/*@
 *@ 功能创建服务器
 *@param[in] port服务器绑定的端口
 *@param[in] fun1 处理命令的函数
 *@param[in] param 可将该指针在回调函数中回调出来，可为NULL
 *@返回值 	返回服务器句柄
 */
ShortLinkServerHandle shortLink_create_netServer(int port, ShortLinkDealCmdFunc fun1, void * param, Connect_Type_t nType);



/*@
 *@ 功能发送内容
 *@param[in] handle客户端操作句柄
 *@param[in] message 发送的内容,服务器发送都是在接收到命令后反馈
 *@param[in] nLen 内容的长度
 *@param[in] code 操作命令
 *@param[in] param 成功返回0,失败返回-1
 */
int shortLinkClient_send_msg(ShortOperateHandle handle, char* message, int nLen, int code, int result);//操作句柄


/*@
 *@ 功能创建客户端
 *@param[in] 结构体port服务器绑定的端口
 *@param[in] 结构服务器IP
 *@@param[in] 结构fun1 处理命令回调后的函数
 *@param[in] 结构param 可将该指针在回调函数中回调出来，可为NULL
 *@返回值 返回发送成功与否
 */
int shortLink_creat_netClient(ShortLink_Send_t *pshortHandle);


/*@
 *@ 功能创建客户端,并同步返回数据结果
 *@param[in] pshortHandle：服务器信息，该结构体的字段dealcmd，置为空则同步返回，否则异步
 *@param[in] outPkt：同步返回的数据，务必将pshortHandle结构体里面的dealcmd置空
 *@返回值 返回发送成功与否
 */
int shortLink_creat_netClient_sync(ShortLink_Send_t *pshortHandle,\
		ShortCallbackMsg_t* outPkt);


/*@
 *@ 释放同步返回的数据
 *@param[in] outPkt：同步返回的数据
 *@返回值 返回发送成功与否
 */
int shortLink_netClient_release_syncPacket(ShortCallbackMsg_t* outPkt);


/*@
 *@ 功能发送内容
 *@param[in] handle服务器操作句柄，由callbackmessage返回
 *@param[in] message 发送的内容,服务器发送都是在接收到命令后反馈
 *@param[in] nLen 内容的长度
 *@param[in] code 操作命令
 *@param[in] param 成功返回0,失败返回-1
 */
int shortLinkServer_send_msg(ShortOperateHandle handle, char* message, int nLen, int code, int ret);//操作句柄
#ifdef __cplusplus
}
#endif

#endif
