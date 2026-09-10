/*** 
 * @FilePath     : onvif_server.cpp
 * @Author       : cyc
 * @Date         : 2025-04-12 09:38:51
 * @LastEditors  : cyc
 * @LastEditTime : 2025-09-16 16:09:59
 * @Description  : onvif服务类
 */

#include "dlog.h"
#include <time.h>
#include "onvif_server.h"
#include "user_manage.h"
#include "dlog.h"
#include "convert_interface.h"
#include "path_define.h"
#include "edukit_network.h"

#include "wsdd.nsmap"

void COnvifServer::process_client_connection(const OnvifConnectionTask& task)
{
    char client_ip[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &(task.client_addr.sin_addr), client_ip, INET_ADDRSTRLEN);
    
    struct soap *client_soap = soap_new1(SOAP_C_UTFSTRING);
    if (!client_soap)
    {
        dlog_error("Failed to create soap context for client %s", client_ip);
        close(task.socket);
        return;
    }
    
    soap_set_namespaces(client_soap, namespaces);
    
    if (soap_register_plugin(client_soap, http_da) != SOAP_OK)
    {
        dlog_error("Failed to register http_da plugin for client %s", client_ip);
        soap_free(client_soap);
        close(task.socket);
        return;
    }

    client_soap->authrealm = AUTHREALM;
    client_soap->socket = task.socket;
    client_soap->ip = task.client_addr.sin_addr.s_addr;
    client_soap->port = ntohs(task.client_addr.sin_port);
    
    client_soap->recv_timeout = 30;
    client_soap->send_timeout = 30;
    
    int result = soap_serve(client_soap);
    
    if (result != SOAP_OK)
    {
        if (result != SOAP_EOF && result != SOAP_TCP_ERROR && result != 401)
        {
            // dlog_warn("Client %s request processing failed with gSOAP error code: %d", client_ip, result);
        }
    }
    
    soap_destroy(client_soap);
    soap_end(client_soap);
    soap_free(client_soap);
}

IpcRet_E COnvifServer::init()
{
    Network::OnvifConfigInfo_S stInfo;
    if(0 != Convert::read_file(ONVIF_CONFIG_FILE, stInfo))
    {
        dlog_error("没有找到onvif.json文件, 重新创建");
        Convert::write_file(ONVIF_CONFIG_FILE, stInfo);
    }

    ::System::SecurityServices_S stSecurityServicesInfo;
    Convert::read_file(SECURITY_SERVICES_FILE, stSecurityServicesInfo);

    set_config(stInfo);
    return OK;
}

IpcRet_E COnvifServer::deinit()
{
    stop_service();
    return OK;
}

int COnvifServer::set_config(Network::OnvifConfigInfo_S stInfo)
{
    int nRet;
    if(stInfo.bEnOnvif)
    {
        nRet = start_service();
    }
    else
    {
        nRet = stop_service();
    }

    return nRet;
}


SOAP_SOCKET COnvifServer::SoapBind(struct soap *pSoap, const char *pIp, bool flag)
{
    SOAP_SOCKET sockFD = soap_bind(pSoap, pIp, pSoap->port, 128); 
    if (!soap_valid_socket(sockFD))
    {
        dlog_error("socket bind failed for port %d", pSoap->port);
        soap_print_fault(pSoap, stderr);
        return SOAP_INVALID_SOCKET;
    }
    int optval = 1;
    setsockopt(pSoap->master, SOL_SOCKET, SO_REUSEPORT, &optval, sizeof(optval));
    return sockFD;
}


void COnvifServer::Onvif_Be_Discovered()
{
    pthread_setname_np(pthread_self(), "OnvifDiscovery");
    while (m_runFlag.load())
    {
        struct soap* udpSoap = soap_new1(SOAP_IO_UDP | SOAP_C_UTFSTRING);
        if (!udpSoap) 
        {
            dlog_error("soap_new1 for UDP discovery failed! Retrying in 5 seconds...");
            sleep(5);
            continue;
        }
        soap_set_namespaces(udpSoap, namespaces);
        udpSoap->bind_flags = SO_REUSEADDR;
        SOAP_SOCKET udp_socket = soap_bind(udpSoap, NULL, ONVIF_UDP_PORT, 100);
        if (!soap_valid_socket(udp_socket))
        {
            dlog_error("UDP socket bind failed on port %d. Retrying in 5 seconds...", ONVIF_UDP_PORT);
            soap_print_fault(udpSoap, stderr);
            soap_free(udpSoap);
            sleep(5);
            continue;
        }
        struct ip_mreq mcast;
        mcast.imr_multiaddr.s_addr = inet_addr(ONVIF_UDP_IP);
        mcast.imr_interface.s_addr = htonl(INADDR_ANY);  
        
        /* 优化点：尝试绑定到主 IP 地址，如果失败则回退到 INADDR_ANY */ 
        // char *primary_ip = get_primary_ip();
        // if (primary_ip && strlen(primary_ip) > 0) {
        //     mcast.imr_interface.s_addr = inet_addr(primary_ip);
        //     // dlog_info("Attempting to join multicast group on primary interface: %s", primary_ip);
        // } else {
        //     dlog_warn("Could not get primary IP. Falling back to INADDR_ANY for multicast.");
        //     mcast.imr_interface.s_addr = htonl(INADDR_ANY);
        // }
         system("route add -net 224.0.0.0 netmask 224.0.0.0 eth0");
        if (setsockopt(udpSoap->master, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mcast, sizeof(mcast)) < 0)
        {
            // 如果使用指定 IP 加入失败，尝试用 INADDR_ANY 再次加入
            //dlog_error("setsockopt IP_ADD_MEMBERSHIP error on %s: %s. Retrying with INADDR_ANY.", primary_ip ? primary_ip : "default", strerror(errno));
            mcast.imr_interface.s_addr = htonl(INADDR_ANY);
            if (setsockopt(udpSoap->master, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mcast, sizeof(mcast)) < 0)
            {
                 dlog_error("setsockopt IP_ADD_MEMBERSHIP error on INADDR_ANY: %s. Will retry.", strerror(errno));
                 soap_closesock(udpSoap);
                 soap_free(udpSoap);
                 sleep(5);
                 /* 重启整个初始化流程 */
                 continue; 
            }
        }
        
        // dlog_info("ONVIF discovery service started on UDP port %d", ONVIF_UDP_PORT);
        // 内部服务循环
        while (m_runFlag.load())
        {
        #if 0
            // 使用 select 来实现带超时的接收，以确保能及时响应停止信号
            fd_set fds;
            FD_ZERO(&fds);
            FD_SET(udpSoap->socket, &fds);
            struct timeval tv = {1, 0}; // 1秒超时
            int ret = select(udpSoap->socket + 1, &fds, NULL, NULL, &tv);
            if (ret < 0) { // select 错误
                dlog_error("Discovery select error: %s", strerror(errno));
                break; // 跳出内层循环，重新初始化 socket
            }
            if (ret > 0 && FD_ISSET(udpSoap->socket, &fds)) { // 有数据可读
                int nErr = soap_serve(udpSoap);
                if (nErr != SOAP_OK && nErr != SOAP_EOF)
                {
                    // dlog_warn("UDP soap_serve error: %d. Re-initializing discovery socket.", nErr);
                   // soap_print_fault(udpSoap, stderr);
                    break; // 遇到严重错误，跳出内层循环，重新初始化
                }
            }
        #endif
            /* 直接接收 */
            if( soap_serve(udpSoap) != SOAP_OK )
            {
                // soap_print_fault(udpSoap, stderr);
                // printf("soap_print_fault\n");
            }
            soap_destroy(udpSoap);
            soap_end(udpSoap);
            // dlog_error("Onvif_Be_Discovered ip: %s", udpSoap->ip);
        
            // ret == 0 (超时) 或处理成功后，会继续外层 m_runFlag 循环检查
        }
        /* 清理当前 soap 上下文和 socket */ 
        soap_closesock(udpSoap);
        soap_destroy(udpSoap);
        soap_end(udpSoap);
        soap_free(udpSoap);
        if (m_runFlag.load()) 
        {
            /* 在重新初始化前稍作等待 */
            sleep(1); 
        }
    }
}

void* process_request(void *arg)
{
    struct soap *soap = (struct soap*)arg;
    /* 让线程自己释放资源 */
    pthread_detach(pthread_self()); 
    /* 为每个请求注册 Digest 认证插件 */
    soap_register_plugin(soap, http_da);
    if (soap_serve(soap) != SOAP_OK)
    {
        // soap_print_fault(soap, stderr); 
    }
    /* 清理克隆的上下文 */
    soap_destroy(soap); 
    /* 结束这个上下文 */
    soap_end(soap);  
    /* 释放内存 */   
    soap_free(soap);    
    return NULL;
}

void COnvifServer::Onvif_Web_Services()
{
    pthread_setname_np(pthread_self(), "OnvifWebService");
    
    struct soap* masterSoap = soap_new1(SOAP_C_UTFSTRING);
    if (!masterSoap) 
    {
        dlog_error("soap_new1 for master TCP socket failed!");
        return;
    }
    
    masterSoap->port = ONVIF_TCP_PORT;
    masterSoap->bind_flags = SO_REUSEADDR;
    soap_set_namespaces(masterSoap, namespaces);
    
    SOAP_SOCKET masterSocket = SoapBind(masterSoap, NETWORK_ADDR, false);
    if (!soap_valid_socket(masterSocket))
    {
        dlog_error("tcpsocket SoapBind failed!");
        soap_free(masterSoap);
        return;
    }
    
    dlog_info("启动onvif设备应答功能 on TCP port %d", ONVIF_TCP_PORT);
    
    while (m_runFlag.load())
    {
        fd_set readfds;
        FD_ZERO(&readfds);
        FD_SET(masterSocket, &readfds);
        
        struct timeval timeout;
        timeout.tv_sec = 1;
        timeout.tv_usec = 0;
        
        int activity = select(masterSocket + 1, &readfds, NULL, NULL, &timeout);
        
        if (activity < 0) {
            if (errno == EINTR) continue;
            dlog_error("select error: %s", strerror(errno));
            break;
        }
        
        if (activity > 0 && FD_ISSET(masterSocket, &readfds))
        {
            struct sockaddr_in client_addr;
            socklen_t client_len = sizeof(client_addr);
            
            SOAP_SOCKET clientSocket = accept(masterSocket, (struct sockaddr*)&client_addr, &client_len);
            
            if (clientSocket < 0)
            {
                if (m_runFlag.load()) dlog_warn("accept failed: %s", strerror(errno));
                continue;
            }
            
            if (m_threadPool && !m_threadPool->enqueue_task({clientSocket, client_addr}))
            {
                char client_ip[INET_ADDRSTRLEN];
                inet_ntop(AF_INET, &(client_addr.sin_addr), client_ip, INET_ADDRSTRLEN);
                dlog_warn("Thread pool queue full, dropping connection from %s", client_ip);
                close(clientSocket);
            }
        }
    }
    
    soap_closesock(masterSoap);
    soap_destroy(masterSoap);
    soap_end(masterSoap);
    soap_free(masterSoap);
}

int COnvifServer::start_service()
{
    
	if(!m_runFlag.load())
    {
        m_runFlag.store(true);
        dlog_debug("开启onvif服务端");
        /* 创建线程池，使用lambda表达式作为任务处理函数 */ 
        auto task_handler = [this](const OnvifConnectionTask& task) {
            this->process_client_connection(task);
        };
        
        m_threadPool = std::make_unique<OnvifThreadPool>(
            ONVIF_THREAD_POOL_SIZE, 
            ONVIF_MAX_QUEUE_SIZE, 
            task_handler
        );

        /* 初始化-应答控制 */
        m_webServicesThread = std::thread(&COnvifServer::Onvif_Web_Services, this);

        /* 初始化-设备发现 */
        m_discoveryThread = std::thread(&COnvifServer::Onvif_Be_Discovered, this);
    }
	
    return 0;

}

int COnvifServer::stop_service()
{
    if(m_runFlag.load())
    {
        m_runFlag.store(false);
        /* 停止线程池 */ 
        if (m_threadPool)
        {
            m_threadPool->stop();
            m_threadPool.reset();
        }
        m_threadPool = nullptr;

        dlog_debug("关闭onvif服务端");
        if (m_webServicesThread.joinable()) 
        {
            m_webServicesThread.join();
        }
        if (m_discoveryThread.joinable()) 
        {
            m_discoveryThread.join();
        }
        
    }
    return 0;
}