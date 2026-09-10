/*
 * @Author       : EasonLu
 * @Date         : 2023-04-27 19:11:53
 * @LastEditors: EasonLu
 * @LastEditTime: 2023-07-11 10:38:52
 * @FilePath: sdk_network.h
 * @Description  : 网络库的头文件
 */
#ifndef _SDK_NETWORK_H_
#define _SDK_NETWORK_H_
#include "IpcRet.h"
#include "dlog.h"
#include "list_base.h"
#include "list_use_lock.h"
#include "sdk_net_define.h"

#ifdef __cplusplus
extern "C"
{
#endif
    /**
     * @brief 设置心跳码
     * @param nActionCode 心跳码
     * @return * void 
     */
    void sdk_set_heartbitCode(int nActionCode);
    /**
     * @brief 设置心跳间隔
     * @param nHeartbeatInterval 心跳间隔
     * @return * void 
     */
    void sdk_set_heartbeatInterval(int nHeartbeatInterval);
    /**
     *@ref 连接服务器
     *@param[in] InparamClientNet_t 见结构体解析
     *@return 成功返回网络操作句柄，失败返回空
     */
    Sdk_Net_Handle_t sdkclient_init_net(InparamClientNet_t netparm);

    /**
     *@ref 主动断开服务器
     *@param[in] handle 操作句柄
     *@return  成功返回RET_SUCCESS,失败见RetErr_t
     **/
    IpcRet_E sdkclient_stop_net(Sdk_Net_Handle_t handle);
    /**
     *@ref 主动断开服务器,并且销毁句柄
     *@param[in] handle 操作句柄
     *@return  成功返回RET_SUCCESS,失败见RetErr_t
     */
    IpcRet_E sdkclient_uninit_net(Sdk_Net_Handle_t handle);

    // server***************************************************************

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
    int setConnectClientParam(Sdk_Net_Handle_t clienthandle, void *clientParam);

    /**
     *@ref 获取用户自己设置进去的客户端参数，从而当断开连接后可知道具体哪个客户端断开
     *@param[in] clienthandle 回调的连接客户端句柄
     *@return 成功返回用户保存的参数，失败返回空
     */
    void *getConnectClientParam(Sdk_Net_Handle_t clienthandle);
    /**
     *@ref 获取连接客户端的一些信息，方便管理
     *@param[in] clienthandle 回调的连接客户端句柄
     *@return 成功返回0失败返回-1
     */

    int getUsermessege(Sdk_Net_Handle_t clienthandle, OfferUserMessege_t *messege);

    // 公用发送**********************************************************

    /**
     *@ref 发送信息给连接的客户端或者服务器
     *@param[in] pOprHandle 发送的句柄
     *@param[in] message 发送内容
     *@param[in] code 发送类型
     *@param[in] nLen 内容长度
     *@return 成功返回0失败返回-1
     */
    int net_send_msg(Sdk_Net_Handle_t pOprHandle, char *message, int nLen, int code);

    int net_send_msgdeal(Sdk_Net_Handle_t pOprHandle, char *message, int nLen, int code);

    // 目前支持客户端发送
    int net_send_file(Sdk_Net_Handle_t pOprHandle, char *filename, int code);
    /**
     *@ref 发送信息给连接的客户端或者服务器
     *@param[in] pOprHandle 发送的句柄
     *@param[in] message 内容
     *@param[in] waitTime 等待时间
     *@param[in] nLen 内容长度
     *@return 成功返回0失败返回-1
     */
    int net_recv_msg(Sdk_Net_Handle_t pOprHandle, char *message, int nLen, int waitTime);

    /**
     *@ref 服务器发送内容给所有客户端
     *@param[in] pServerHandle 服务器的句柄
     *@param[in] message 发送内容
     *@param[in] code 发送类型
     *@param[in] nLen 内容长度
     *@return 成功返回0失败返回-1
     */
    int netserver_send_allClient(Sdk_ServerNet_Handle_t pServerHandle, char *message, int nLen, int code);

    pthread_mutex_t *sdk_getlist_serverlock(Sdk_ServerNet_Handle_t pServerHandle);

    List_CurNode_t sdk_list_begin_clientHandle(Sdk_ServerNet_Handle_t pServerHandle,
                                               Sdk_Net_Handle_t *pOprHandle);
    List_CurNode_t sdk_list_next_clientHandle(Sdk_ServerNet_Handle_t pServerHandle,
                                              Sdk_Net_Handle_t *pOprHandle, List_CurNode_t listNode);
    List_CurNode_t sdk_list_end_clientHandle(Sdk_ServerNet_Handle_t pServerHandle);
    /**
     * @brief 查看客户端是否还在
     * @param pServerHandle  
     * @param pOprHandle 
     * @return int 
     */
    int sdk_get_clientStatus(Sdk_ServerNet_Handle_t pServerHandle, Sdk_Net_Handle_t pOprHandle);
    /**
     * @description: 获取全部客户端的数量
     * @param [dk_ServerNet_Handle_t] pServerHandle: 服务器句柄
     * @return [*] 客户端数量
     * @others:
     */
    int sdk_get_allClientNumber(Sdk_ServerNet_Handle_t pServerHandle);

    // 默认设置线程栈64KB
    int sdk_pthread_create(pthread_t *thread_id, const pthread_attr_t *user_attr, pthread_fun_sdk funtion, void *argv);

#ifdef __cplusplus
}
#endif

#endif /* _SDK_NETWORK_H_ */