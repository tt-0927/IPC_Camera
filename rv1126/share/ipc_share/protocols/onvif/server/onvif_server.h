/*** 
 * @FilePath     : onvif_server.h
 * @Author       : cyc
 * @Date         : 2025-04-12 10:10:41
 * @LastEditors  : cyc
 * @LastEditTime : 2025-09-11 16:09:11
 * @Description  : onvif服务类
 */

#pragma once
#include <thread>
#include <string>
#include <atomic>
#include "Singleton.h"
#include "network_define.h"
#include "IpcRet.h"
#include "thread_pool.h"

#include "onvif_server_wrapper.h"

/* ONVIF线程池配置 */ 
#define ONVIF_THREAD_POOL_SIZE 8
#define ONVIF_MAX_QUEUE_SIZE 100
// 前向声明
class OnvifThreadPool;
struct OnvifConnectionTask;

// 认证信息
#define AUTHREALM "ITC_IPC"

typedef struct _ONVIF_SEREVR_
{
    /* 设备IP地址 */
    char achDeviceIP[16];
    /* 设备MAC地址 */
    char achDeviceMac[6];
    /* soap通信端口号 */
    int nSoapPort;
    /* onvif使能 */
    bool BEnable;
    /* 授权账号 */
    char achAuthorizationUser[128];
    /* 授权密码 */
    char achAuthorizationPassed[128];
    /* 指令码 长度为1024 onvif内部做了定义SOAP_TAGLEN*/ 
    char achTag[1024];
    /* 指令回调函数 */
    void* (*onvif_callback_function)(void *pArgv);
}OnvifServerData_S;


class COnvifServer : public CSingleton<COnvifServer>
{
    COnvifServer() = default;

public:

    ~COnvifServer() = default;
    friend class CSingleton<COnvifServer>;

    /**
     * @brief 初始化
     * @return IpcRet_E 
     */
    IpcRet_E init();
    /**
     * @brief 去初始化
     * @return IpcRet_E 
     */
    IpcRet_E deinit();
    /**
     * @brief 配置onvif服务端
     * @return int 
     */
    int set_config(Network::OnvifConfigInfo_S stInfo);
    /**
     * @brief 开启onvif服务端功能
     * @return int 
     */
    int start_service();

    /**
     * @brief 关闭onvif服务端功能
     * @return int 
     */
    int stop_service();
    
private:
    /**
     * @brief 绑定SOAP服务到指定IP和端口
     * @param pSoap gSOAP 上下文指针
     * @param pIp ip地址
     * @param flag 地址复用标志
     * @return SOAP_SOCKET  成功,返回已绑定的有效 Socket 描述符
     */
    SOAP_SOCKET SoapBind(struct soap *pSoap, const char *pIp, bool flag);
    /**
     * @brief 加入设备组 设备发现
     * @return void*
     */
    void Onvif_Be_Discovered();
    /**
     * @brief 监听soap报文
     * @return void
     */
    void Onvif_Web_Services();

    void process_onvif_connection(std::shared_ptr<OnvifConnectionTask> task);

   /**
     * @brief 处理ONVIF客户端连接
     * @param task 连接任务
     */
     void process_client_connection(const OnvifConnectionTask& task);

private:
    /* onvif服务器运行标志 */ 
    std::atomic<bool> m_runFlag{false};
    /* 应答线程 */
    std::thread m_webServicesThread;
    /* 设备发现线程 */
    std::thread m_discoveryThread;
    /* 线程池 */
    std::unique_ptr<OnvifThreadPool> m_threadPool;
};