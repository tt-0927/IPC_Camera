#ifndef _RTSPSERVER_BASE_H_
#define _RTSPSERVER_BASE_H_

#include <sys/time.h>
#include "custom_define.h"
#ifdef __cplusplus
extern "C"
{
#endif

/*流状态回调函数指针*/
typedef int (*ServerStreamStatus)(Rtsp_ClientStream_State_t *param);
/* 客户端连接握手认证回调函数定义 */
typedef int (*HandshakeAuthCallback)(char *pClientIP);

// info **************************  RTSP服务器相关接口 **************************

/**
 * @brief   : RTSP服务器初始化
 * @param    {int} port - RTSP服务器端口号
 * @param    {int} nRegister - 是否启用认证 0-不启用 1-启用
 * @param    {char} *pUser - 用户名
 * @param    {char} *pPassworld - 密码
 * @param    {int} nAuthAlgorithm - 认证算法 0-MD5 1-SHA256
 * @param    {int} nDscp - 服务质量标识(DSCP)值
 * @return   {RtSpServerHandle_t} RtSpServerHandle_t类型句柄,NULL表示失败
 */
RtSpServerHandle_t rtsp_server_init(int port, int nRegister, const char *pUser, const char *pPassworld, int nAuthAlgorithm, int nDscp);

/**
 * @brief   : 创建RTSP流会话
 * @param    {RtSpServerHandle_t} pHandle - RTSP服务器句柄
 * @param    {Rtsp_Create_Info_t} *pCreatServerInfo - 流会话创建参数
 * @return   {int} 0成功，非0失败
 */
int rtsp_server_create(RtSpServerHandle_t pHandle, Rtsp_Create_Info_t *pCreatServerInfo);

/**
 * @brief   : 销毁指定的RTSP流会话
 * @param    {RtSpServerHandle_t} pHandle - RTSP服务器句柄
 * @param    {char} *streamName - 流名称
 * @return   {int} 0成功，非0失败
 */
int rtsp_server_destory(RtSpServerHandle_t pHandle, const char *streamName);

/**
 * @brief   : RTSP服务器反初始化
 * @param    {RtSpServerHandle_t} pRtspHandle - RTSP服务器句柄
 * @return   {int} 0成功，非0失败
 */
int rtsp_server_unInit(RtSpServerHandle_t pRtspHandle);

/**
 * @brief   : 获取RTSP客户端信息
 * @param    {RtSpServerHandle_t} pHandle - RTSP服务器句柄
 * @param    {char} *streamName - 流名称
 * @param    {Rtsp_Client_Info_t} *pClientInfo - 客户端信息
 * @return   {int} 0成功，非0失败
 */
int rtsp_getclient_info(RtSpServerHandle_t pHandle, const char *streamName, Rtsp_Client_Info_t *pClientInfo);

/**
 * @brief   : 设置RTSP流最大客户端连接数
 * @param    {RtSpServerHandle_t} pHandle - RTSP服务器句柄
 * @param    {char} *streamName - 流名称
 * @param    {int} maxNum - 最大连接数
 * @return   {int} 0成功，非0失败
 */
int rtsp_setclient_maxNum(RtSpServerHandle_t pHandle, const char *streamName, int maxNum);

/**
 * @brief   : 打印RTSP服务器错误信息
 * @param    {RtSpServerHandle_t} pHandle - RTSP服务器句柄
 * @return   {int} 0成功，非0失败
 */
int printfErrMesege(RtSpServerHandle_t pHandle);

/**
 * @brief   : 设置握手认证回调函数
 * @param    {void} *handle RTSP服务器句柄
 * @param    {HandshakeAuthCallback} callback 握手认证回调函数
 * @return   {int} 0成功，非0失败
 */
int set_handshakeAuth_callback(void *handle, HandshakeAuthCallback callback);

/**
 * @brief   : 关闭目标IP连接
 * @param    {void} *handle RTSP服务器句柄
 * @param    {char} *targetIP 目标IP
 * @return   {int} 0成功，非0失败
 */
int close_connection(void *handle, char *targetIP);

// info **************************  RTSP客户端相关接口 **************************

/**
 * @brief   : 启动RTSP客户端
 * @param    {char} *url - RTSP服务器地址
 * @param    {ClientStreamStatus} stateCallback - 状态回调函数
 * @param    {FrameCallBack} frameCall - 帧数据回调函数
 * @param    {void} *param - 用户参数
 * @param    {Rtsp_Inparam} *Inparam - 输入参数
 * @return   {RtSpClientHandle_t} RtSpClientHandle_t 类型句柄,NULL表示失败
 */
RtSpClientHandle_t rtsp_client_start(const char *url, ClientStreamStatus stateCallback, FrameCallBack frameCall, void *param, Rtsp_Inparam *Inparam);

/**
 * @brief   : RTSP客户端重连
 * @param    {RtSpClientHandle_t} clienthandle - 客户端句柄
 * @return   {int} 0成功，非0失败
 */
int rtsp_client_reConnect(RtSpClientHandle_t clienthandle);

/**
 * @brief   : 停止RTSP客户端
 * @param    {RtSpClientHandle_t} clienthandle - 客户端句柄
 * @return   {int} 0成功，非0失败
 */
int rtsp_client_stop(RtSpClientHandle_t clienthandle);

#ifdef __cplusplus
}
#endif
#endif //_RTSPSERVER_BASE_H_
