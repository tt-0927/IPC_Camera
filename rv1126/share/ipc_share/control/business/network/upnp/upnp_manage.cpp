/***
 * @FilePath     : upnp_manage.cpp
 * @Author       : tianl (tianl@kfb.cn)
 * @Date         : 2025-03-19 14:16:09
 * @LastEditors  : huangjunda
 * @LastEditTime : 2025-07-21 09:05:11
 * @Description  : upnp管理实现
 */

#include <algorithm>
#include <filesystem>
#include <ostream>
#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "edukit_network.h"
#include "upnp_manage.h"

#include "network_convert.h"
#include "convert_interface.h"
#include "path_define.h"

CUpnpManage::CUpnpManage()
    : m_upnpFile(UPNP_CONFIG_FILE),
      m_portFile(PORT_CONFIG_FILE)
{
}

CUpnpManage::~CUpnpManage()
{
}

bool CUpnpManage::file_exists(const std::string &strFilename)
{
    namespace fs = std::filesystem;
    return fs::exists(strFilename) && fs::is_regular_file(strFilename);
}

/* 静态函数-完成命令回调 */
static int action_callback(Upnp_EventType eventType, const void *event, void *cookie)
{
    if (eventType == UPNP_CONTROL_ACTION_COMPLETE)
    {
        /* 提取上下文中的端口信息 */
        auto *pPortMap = static_cast<Network::PortMap_S *>(cookie);
        CUpnpManage::instance()->process_action_complete(event, *pPortMap);
    }
    return UPNP_E_SUCCESS;
}
/* 静态函数事件回调 */
int CUpnpManage::event_callback(int eventType, void *event, void *cookie)
{
    CUpnpManage::instance()->handle_event(eventType, event);
    return UPNP_E_SUCCESS;
}

void CUpnpManage::print_status()
{
    std::lock_guard<std::mutex> lock(m_deviceMutex);

    std::cout << "\n=== UPnP 状态报告 ===" << std::endl;
    std::cout << "外部IP: " << m_externalIP << std::endl;
    std::cout << "发现设备数量: " << m_mapdevices.size() << std::endl;

    // 打印端口映射状态
    std::cout << "\n端口映射配置：" << std::endl;
    for (const auto &port : m_currentConfig.portMap)
    {
        std::cout << "类型:" << port.nPortType
                  << " 外埠:" << port.nExternPort
                  << "->内埠:" << port.nInternalPort
                  << " 状态:" << (port.nStatus ? "已映射" : "未映射")
                  << " 外部IP:" << port.externIp << std::endl;
    }

    // 打印设备列表
    if (!m_mapdevices.empty())
    {
        std::cout << "\n已发现设备：" << std::endl;
        for (const auto &[udn, device] : m_mapdevices)
        {
            std::cout << "UDN: " << udn
                      << "\n  服务类型: " << device.strServiceType
                      << "\n  控制地址: " << device.strControlURL
                      << "\n"
                      << std::endl;
        }
    }
    std::cout << "============================\n"
              << std::endl;
}

int CUpnpManage::init()
{
    /* 内部端口 */
    int nHttpPort, nRtspPort, nHttpsPort, nServerPort,nWebServerPort;
    /* 外部映射端口 */
    int nEHttpPort, nERtspPort, nEHttpsPort, nEServerPort,nEWebServerPort;
    Network::PortConfig_S stPortConfig;
    if (!file_exists(m_upnpFile))
    {
        dlog_info("upnp外部端口获取默认端口");
        /* 默认不启用端口映射 */
        m_currentConfig.bEnablePortMap = false;
        /*  默认映射类型设为自动 */
        m_currentConfig.nMapType = 1;
        /* 默认端口 */
        nEHttpPort = UPNP_HTTP_PORT;
        nERtspPort = UPNP_RTSP_PORT;
        nEHttpsPort = UPNP_HTTPS_PORT;
        nEServerPort = UPNP_SERVER_PORT;
        nEWebServerPort = UPNP_WEB_SERVER_PORT;
    }
    else
    {
        dlog_info("upnp外部端口获取已配置端口");
        Network::PortMapConfig_S stConfig;
        Convert::read_file(m_upnpFile, stConfig);
        m_currentConfig.strName = stConfig.strName;
        m_currentConfig.bEnablePortMap = stConfig.bEnablePortMap;
        m_currentConfig.nMapType = stConfig.nMapType;
        for (auto &port : stConfig.portMap)
        {
            if (port.nPortType == Network::HTTP_PORT_TYPE)
            {
                nEHttpPort = port.nExternPort;
            }
            else if (port.nPortType == Network::RTSP_PORT_TYPE)
            {
                nERtspPort = port.nExternPort;
            }
            else if (port.nPortType == Network::HTTPS_PORT_TYPE)
            {
                nEHttpsPort = port.nExternPort;
            }
            else if (port.nPortType == Network::SERVER_PORT_TYPE)
            {
                nEServerPort = port.nExternPort;
            }
            else if (port.nPortType == Network::WEB_SERVER_PORT_TYPE)
            {
                nEWebServerPort = port.nExternPort;
            }
        }
    }

    if (!file_exists(m_portFile))
    {
        dlog_info("upnp内部端口获取默认端口");
        /* 默认端口 */
        nHttpPort = UPNP_HTTP_PORT;
        nRtspPort = UPNP_RTSP_PORT;
        nHttpsPort = UPNP_HTTPS_PORT;
        nServerPort = UPNP_SERVER_PORT;
        nWebServerPort = UPNP_WEB_SERVER_PORT;

        stPortConfig.nHttpPort = nHttpPort;
        stPortConfig.nRtspPort = nRtspPort;
        stPortConfig.nHttpsPort = nHttpsPort;
        stPortConfig.nServerPort = nServerPort;
        stPortConfig.nWebServerPort = nWebServerPort;
        Convert::write_file(m_portFile, stPortConfig);
    }
    else
    {
        dlog_info("upnp内部端口获取已配置端口");
        Convert::read_file(m_portFile, stPortConfig);
        nHttpPort = stPortConfig.nHttpPort;
        nRtspPort = stPortConfig.nRtspPort;
        nHttpsPort = stPortConfig.nHttpsPort;
        nServerPort = stPortConfig.nServerPort;
        nWebServerPort = stPortConfig.nWebServerPort;
    }
    dlog_info("初始化映射端口 http(%d->%d) rtsp(%d->%d) https(%d->%d) server(%d->%d) web_server(%d->%d)",
              nHttpPort, nEHttpPort, nRtspPort, nERtspPort, nHttpsPort, nEHttpsPort, nServerPort, nEServerPort,nWebServerPort, nEWebServerPort);
    /* 初始化默认端口映射项 */
    m_currentConfig.portMap =
        {
            {Network::HTTP_PORT_TYPE, nEHttpPort, UPNP_EMTPY_IP, nHttpPort, 0},
            {Network::RTSP_PORT_TYPE, nERtspPort, UPNP_EMTPY_IP, nRtspPort, 0},
            {Network::HTTPS_PORT_TYPE, nEHttpsPort, UPNP_EMTPY_IP, nHttpsPort, 0},
            {Network::SERVER_PORT_TYPE, nEServerPort, UPNP_EMTPY_IP, nServerPort, 0},
            {Network::WEB_SERVER_PORT_TYPE, nEWebServerPort, UPNP_EMTPY_IP, nWebServerPort, 0}
        };
    int nRet = 0;
    if (m_currentConfig.bEnablePortMap)
    {
        nRet = init_upnp();
        if (0 == nRet)
        {
            search_device(UPNP_TIMEOUT);
        }
    }
    return nRet;
}

int CUpnpManage::set_port_map(Network::PortMapConfig_S stPortMapConfig)
{
    /* 开启upnp搜索 */
    if (stPortMapConfig.bEnablePortMap && !m_currentConfig.bEnablePortMap)
    {
        dlog_info("==========开启upnp功能==========");
        /* 自动映射 */
        if (stPortMapConfig.nMapType == 1)
        {
            for (auto &port : stPortMapConfig.portMap)
            {
                if (port.nPortType == Network::HTTP_PORT_TYPE)
                {
                    port.nExternPort = UPNP_HTTP_PORT;
                }
                else if (port.nPortType == Network::RTSP_PORT_TYPE)
                {
                    port.nExternPort = UPNP_RTSP_PORT;
                }
                else if (port.nPortType == Network::HTTPS_PORT_TYPE)
                {
                    port.nExternPort = UPNP_HTTPS_PORT;
                }
                else if (port.nPortType == Network::SERVER_PORT_TYPE)
                {
                    port.nExternPort = UPNP_SERVER_PORT;
                }
                else if (port.nPortType == Network::WEB_SERVER_PORT_TYPE)
                {
                    port.nExternPort = UPNP_WEB_SERVER_PORT;
                }
            }
        }
        if (stPortMapConfig.bEnablePortMap)
        {
            if (0 == init_upnp())
            {
                search_device(UPNP_TIMEOUT);
            }
        }
    }
    /* 关闭upnp功能 */
    else if (!stPortMapConfig.bEnablePortMap && m_currentConfig.bEnablePortMap)
    {
        dlog_info("==========关闭upnp功能==========");
        /* 取消设备映射 */
        if (!m_currentConfig.portMap.empty())
        {
            for (auto &port : m_currentConfig.portMap)
            {
                if (port.nStatus)
                {
                    remove_port_mapping(port);
                }
            }
        }
        /* 取消设备搜索 */
        stop_search();
        deinit();
    }
    /* 清空当前的配置信息 */
    else if (stPortMapConfig.bEnablePortMap && m_currentConfig.bEnablePortMap)
    {
        dlog_info("==========重新开启upnp功能==========");
        /* 取消设备映射 */
        if (!m_currentConfig.portMap.empty())
        {
            for (auto &port : m_currentConfig.portMap)
            {
                if (port.nStatus)
                {
                    remove_port_mapping(port);
                    port.nStatus = 0;
                }
            }
        }
        stop_search();
        deinit();
        /* 自动映射 */
        if (stPortMapConfig.nMapType == 1)
        {
            for (auto &port : m_currentConfig.portMap)
            {
                if (port.nPortType == Network::HTTP_PORT_TYPE)
                {
                    port.nExternPort = UPNP_HTTP_PORT;
                }
                else if (port.nPortType == Network::RTSP_PORT_TYPE)
                {
                    port.nExternPort = UPNP_RTSP_PORT;
                }
                else if (port.nPortType == Network::HTTPS_PORT_TYPE)
                {
                    port.nExternPort = UPNP_HTTPS_PORT;
                }
                else if (port.nPortType == Network::SERVER_PORT_TYPE)
                {
                    port.nExternPort = UPNP_SERVER_PORT;
                }
                else if (port.nPortType == Network::WEB_SERVER_PORT_TYPE)
                {
                    port.nExternPort = UPNP_WEB_SERVER_PORT;
                }
            }
        }
        if (stPortMapConfig.bEnablePortMap)
        {
            if (0 == init_upnp())
            {
                search_device(UPNP_TIMEOUT);
            }
        }
    }

    if (!m_currentConfig.portMap.empty())
    {
        for (auto &port : m_currentConfig.portMap)
        {
            if (port.nStatus)
            {
                port.nStatus = 0;
            }
        }
    }

    /* 更新别名 */
    if (!stPortMapConfig.strName.empty() && 0 != m_currentConfig.strName.compare(stPortMapConfig.strName))
    {
        m_currentConfig.strName = stPortMapConfig.strName;
    }
    m_currentConfig.bEnablePortMap = stPortMapConfig.bEnablePortMap;
    m_currentConfig.nMapType = stPortMapConfig.nMapType;

    for (int i = 0; i < stPortMapConfig.portMap.size(); i++)
    {
        m_currentConfig.portMap.at(i).nPortType = stPortMapConfig.portMap.at(i).nPortType;
        m_currentConfig.portMap.at(i).nExternPort = stPortMapConfig.portMap.at(i).nExternPort;
        m_currentConfig.portMap.at(i).externIp = stPortMapConfig.portMap.at(i).externIp;
        m_currentConfig.portMap.at(i).nInternalPort = stPortMapConfig.portMap.at(i).nInternalPort;
        m_currentConfig.portMap.at(i).nStatus = stPortMapConfig.portMap.at(i).nStatus;
    }

    Convert::write_file(UPNP_CONFIG_FILE, m_currentConfig);

    return 0;
}

int CUpnpManage::get_port_map(Network::PortMapConfig_S &stPortMapConfig)
{
    /* 发送搜索命令 */
    if (m_currentConfig.bEnablePortMap)
    {
        if(m_handle == -1)
        {
            if (0 == init_upnp())
            {
                search_device(UPNP_TIMEOUT);
            }
        }
        else if(m_handle != -1 && m_upnpStaus == OPENED)
        {
            search_device(UPNP_TIMEOUT);
        }
        else 
        {
            dlog_info("upnp正在去初始化");
        }
    }

    if (m_currentConfig.portMap.empty())
    {
        dlog_error("获取upnp配置失败");
        return -1;
    }

    stPortMapConfig.strName = m_currentConfig.strName;
    stPortMapConfig.bEnablePortMap = m_currentConfig.bEnablePortMap;
    stPortMapConfig.nMapType = m_currentConfig.nMapType;
    stPortMapConfig.portMap.resize(m_currentConfig.portMap.size());
    for (int i = 0; i < m_currentConfig.portMap.size(); i++)
    {
        stPortMapConfig.portMap.at(i).nPortType = m_currentConfig.portMap.at(i).nPortType;
        stPortMapConfig.portMap.at(i).nExternPort = m_currentConfig.portMap.at(i).nExternPort;
        stPortMapConfig.portMap.at(i).externIp = m_currentConfig.portMap.at(i).externIp;
        stPortMapConfig.portMap.at(i).nInternalPort = m_currentConfig.portMap.at(i).nInternalPort;
        stPortMapConfig.portMap.at(i).nStatus = m_currentConfig.portMap.at(i).nStatus;
    }

    return 0;
}

int CUpnpManage::init_upnp()
{
    if(m_upnpStaus != CLOSED)
    {
        dlog_error("upnp已经初始化或正在去初始化");
        return -1;
    }
    m_upnpStaus = OPENING;
    int nRet = UpnpInit2(NULL, 0);
    if (nRet != UPNP_E_SUCCESS)
    {
        dlog_error("upnp初始化失败, code=%d (%s)", nRet, UpnpGetErrorMessage(nRet));
        m_upnpStaus = CLOSED;
        return -1;
    }
    /* 注册控制点 */
    nRet = UpnpRegisterClient((Upnp_FunPtr)event_callback, this, &m_handle);
    if (nRet != UPNP_E_SUCCESS)
    {
        dlog_error("客户端注册失败 (错误码: %d)", nRet);
        m_upnpStaus = CLOSING;
        UpnpFinish();
        m_upnpStaus = CLOSED;
        return -1;
    }
    m_upnpStaus = OPENED;
    dlog_info("客户端注册成功 (句柄: %d)", m_handle);
    return 0;
}

void CUpnpManage::deinit_thread()
{
    if (m_handle != -1 && m_upnpStaus == OPENED)
    {
        m_upnpStaus = CLOSING;
        UpnpUnRegisterClient(m_handle);
        UpnpFinish();
        m_handle = -1;
        m_mapdevices.clear();
        m_upnpStaus = CLOSED;
        dlog_info("UPnP资源已释放");
    }
    return ;
}

int CUpnpManage::deinit()
{
    if(m_upnpStaus == OPENED)
    {
        std::thread tid(&CUpnpManage::deinit_thread, CSingleton<CUpnpManage>::instance());
        tid.detach();
    }
    return 0;
}

void CUpnpManage::start_search()
{
    if (!m_loop)
    {
        m_loop = true;
        m_worker = std::thread([this]
                               { search_device(3); });
    }
}

void CUpnpManage::stop_search()
{
    // if (m_loop)
    //{
    dlog_info("停止设备搜索");

    // m_loop = false;
    // if (m_worker.joinable())
    //{
    //     m_worker.join();
    //     dlog_info("设备搜索线程已停止");
    // }
    //}
}

/* 搜索设备 */
int CUpnpManage::search_device(int nTimeOut)
{
    dlog_info("==开始设备搜索 (超时: %d s)==", nTimeOut);
    // while (m_loop)
    //{
    // dlog_info("===发送搜索命令===");
    int nRet = UpnpSearchAsync(m_handle, nTimeOut, UPNP_ALL_SERVICE, nullptr);
    if (nRet != UPNP_E_SUCCESS)
    {
        dlog_error("搜索请求发送失败 (错误码: %d)", nRet);
        return -1;
    }

    /* 端口映射检测 */
    //  std::this_thread::sleep_for(std::chrono::seconds(30));
    //}
    return 0;
}
/* 添加映射 */
int CUpnpManage::add_port_mapping(Network::PortMap_S stPortMap)
{
    std::lock_guard<std::mutex> lock(m_deviceMutex);
    if (m_mapdevices.empty())
    {
        dlog_error("upnp设备为空");
        return -1;
    }
    auto device = m_mapdevices.begin();
    IXML_Document *pAction = UpnpMakeAction("AddPortMapping", device->second.strServiceType.c_str(), 0, nullptr);
    if (!pAction)
    {
        dlog_error("创建端口映射命令失败");
        return -1;
    }
    /* 获取设备ip */
    std::string strDeviceIp = get_primary_ip();
    ;

    /* 获取端口描述 */
    auto descIt = PORT_TYPE_DESCRIPTION.find(stPortMap.nPortType);
    std::string strDescription = (descIt != PORT_TYPE_DESCRIPTION.end()) ? descIt->second : "Custom Service";

    /* 添加端口映射xml参数 */
    const char *pParams[] =
        {
            "NewRemoteHost", "",
            "NewExternalPort", std::to_string(stPortMap.nExternPort).c_str(),
            "NewProtocol", UPNP_PROTOCOL,
            "NewInternalPort", std::to_string(stPortMap.nInternalPort).c_str(),
            "NewInternalClient", strDeviceIp.c_str(),
            "NewEnabled", UPNP_ENABLE,
            "NewPortMappingDescription", strDescription.c_str(),
            "NewLeaseDuration", UPNP_LEASE};

    for (size_t i = 0; i < sizeof(pParams) / sizeof(pParams[0]); i += 2)
    {
        if (UpnpAddToAction(&pAction, "AddPortMapping", device->second.strServiceType.c_str(), pParams[i], pParams[i + 1]) != UPNP_E_SUCCESS)
        {
            dlog_error("添加upnp映射命令失败");
            ixmlDocument_free(pAction);
            return -1;
        }
    }
    /* 创建上下文对象保存端口信息 */
    auto *pContext = new Network::PortMap_S(stPortMap);

    int nRet = UpnpSendActionAsync(m_handle,
                                   device->second.strControlURL.c_str(),
                                   device->second.strServiceType.c_str(),
                                   nullptr,
                                   pAction,
                                   action_callback,
                                   pContext);
    if (nRet == UPNP_E_SUCCESS)
    {
        dlog_info("发送upnp映射命令成功");
    }

    ixmlDocument_free(pAction);
    return 0;
}

int CUpnpManage::remove_port_mapping(Network::PortMap_S stPortMap)
{
    std::lock_guard<std::mutex> lock(m_deviceMutex);
    if (m_mapdevices.empty())
    {
        dlog_error("upnp设备为空");
        return -1;
    }

    auto device = m_mapdevices.begin();
    IXML_Document *pAction = UpnpMakeAction("DeletePortMapping", device->second.strServiceType.c_str(), 0, nullptr);
    if (!pAction)
    {
        dlog_error("创建删除端口映射响应命令失败");
        return -1;
    }
    /* 删除端口映射xml参数 */
    const char *pParams[] =
        {
            "NewRemoteHost", "",
            "NewExternalPort", std::to_string(stPortMap.nExternPort).c_str(),
            "NewProtocol", UPNP_PROTOCOL};

    for (size_t i = 0; i < sizeof(pParams) / sizeof(pParams[0]); i += 2)
    {
        if (UpnpAddToAction(&pAction, "DeletePortMapping", device->second.strServiceType.c_str(), pParams[i], pParams[i + 1]) != UPNP_E_SUCCESS)
        {
            dlog_error("添加删除upnp映射命令失败");
            ixmlDocument_free(pAction);
            return -1;
        }
    }
    /* 创建上下文对象保存端口信息 */
    auto *pContext = new Network::PortMap_S(stPortMap);
    int nRet = UpnpSendActionAsync(m_handle,
                                   device->second.strControlURL.c_str(),
                                   device->second.strServiceType.c_str(),
                                   nullptr,
                                   pAction,
                                   action_callback,
                                   pContext);
    if (nRet == UPNP_E_SUCCESS)
    {
        dlog_info("发送upnp映射命令成功");
    }
    ixmlDocument_free(pAction);
    return 0;
}

void CUpnpManage::process_action_complete(const void *pAction, Network::PortMap_S &stPortMap)
{
    const UpnpActionComplete *pUpnpAction = (const UpnpActionComplete *)pAction;
    int errCode = UpnpActionComplete_get_ErrCode(pUpnpAction);
    if (errCode != UPNP_E_SUCCESS)
    {
        dlog_error("=========命令返回失败 错误码：%d 描述：%s========",
                   errCode, UpnpGetErrorMessage(errCode));

        /* 获取控制地址 */
        const char *ctrlUrl = UpnpActionComplete_get_CtrlUrl_cstr(pUpnpAction);
        if (ctrlUrl) dlog_error("控制地址: %s", ctrlUrl);

        /* 获取 ActionRequest */
        IXML_Document *req = UpnpActionComplete_get_ActionRequest(pUpnpAction);
        if (req)
        {
            DOMString pReqStr = ixmlDocumenttoString(req);
            dlog_error("错误请求：%s", pReqStr);
            if(pReqStr)
            {
                ixmlFreeDOMString(pReqStr);
            }
        } 
        /* 获取 ActionResult */
        IXML_Document *res = UpnpActionComplete_get_ActionResult(pUpnpAction);
        if (res)
        {
            DOMString pResStr = ixmlDocumenttoString(res);
            dlog_error("错误响应：%s", pResStr);
            if(pResStr)
            {
                ixmlFreeDOMString(pResStr);
            }
        }

        return;
    }
    // dlog_info("==========================命令返回成功=======================");

    /* 解析响应文档 */
    IXML_Document *pResponse = UpnpActionComplete_get_ActionResult(pUpnpAction);
    if (pResponse == NULL)
    {
        dlog_error("===upnp设备返回响应字符串为空====");
        return;
    }

    char *pResponseStr = ixmlDocumenttoString(pResponse);
    if (pResponseStr == NULL)
    {
        dlog_error("===upnp设备返回响应字符串为空====");
        return;
    }

    // dlog_debug("返回响应：%s",pResponseStr);
    /* 处理获取IP响应 */
    if (strstr(pResponseStr, "NewExternalIPAddress"))
    {
        std::string strExternIp = get_xml_nodeValue((void *)pResponse, "NewExternalIPAddress");
        dlog_debug("获取到外部ip：%s", strExternIp.c_str());
        m_externalIP = strExternIp;
    }
    /* 处理端口检测响应 */
    else if (strstr(pResponseStr, "GetSpecificPortMappingEntry"))
    {
        std::string strEnable = get_xml_nodeValue((void *)pResponse, "NewEnabled");
        std::string strInternalPort = get_xml_nodeValue((void *)pResponse, "NewInternalPort");
        if (strEnable == UPNP_ENABLE)
        {
            dlog_info("内部端口:%s 已经映射", strInternalPort.c_str());
            m_status = 1;
        }
        else if (strEnable == UPNP_DISABLE)
        {
            dlog_info("内部端口:%s 未映射", strInternalPort.c_str());
            m_status = 0;
        }
    }
    /* 处理端口映射响应 */
    else if (strstr(pResponseStr, "AddPortMapping"))
    {
        /* 更新端口状态 */
        for (auto &port : m_currentConfig.portMap)
        {
            if (port.nInternalPort == stPortMap.nInternalPort)
            {
                dlog_info("内部端口:%d 已经映射", port.nInternalPort);
                port.nStatus = 1;
                port.externIp = m_externalIP;
                break;
            }
        }
    }
    /* 处理删除端口映射响应 */
    else if (strstr(pResponseStr, "DeletePortMapping"))
    {
        m_externalIP = "0.0.0.0";
        /* 更新端口状态 */
        for (auto &port : m_currentConfig.portMap)
        {
            if (port.nInternalPort == stPortMap.nInternalPort)
            {
                dlog_info("内部端口:%d 已经取消映射", port.nInternalPort);
                port.nStatus = 0;
                port.externIp = m_externalIP;
                break;
            }
        }
    }
    if(pResponseStr)
    {
        ixmlFreeDOMString(pResponseStr);
    }

}

void CUpnpManage::handle_event(int stEventType, void *pEvent)
{
    Network::PortMap_S stPortMap;
    switch (stEventType)
    {
    /* 处理设备发现 */
    case UPNP_DISCOVERY_SEARCH_RESULT:
    case UPNP_DISCOVERY_ADVERTISEMENT_ALIVE:
        process_discovery_event(pEvent);
        break;
    /* 处理设备离线 */
    case UPNP_DISCOVERY_ADVERTISEMENT_BYEBYE:
        process_byebye_event(pEvent);
        break;
    /* 处理完成命令 */
    case UPNP_CONTROL_ACTION_COMPLETE:
        process_action_complete(pEvent, stPortMap);
        break;
    /* 处理获取设备状态命令 */
    case UPNP_CONTROL_GET_VAR_COMPLETE:

        break;
    default:
        break;
    }
}

void CUpnpManage::process_discovery_event(void *pDiscovery)
{
    const UpnpDiscovery *pUpnpDiscovery = (const UpnpDiscovery *)pDiscovery;
    // dlog_info("发现设备:%s 类型:%s 设备OS：%s", pDiscovery->DeviceId, pDiscovery->DeviceType,pDiscovery->Os);
    const char *pLocation = UpnpDiscovery_get_Location_cstr(pUpnpDiscovery);
    IXML_Document *pDoc = nullptr;
    DOMString pDocStr = nullptr;
    if (UpnpDownloadXmlDoc(pLocation, &pDoc) != UPNP_E_SUCCESS)
    {
        // dlog_error("设备xml文档下载失败");
        return;
    }
    pDocStr = ixmlDocumenttoString(pDoc);

    Network::UpnpConfigInfo_S stDevice;
    /* 添加设备 */
    if (parse_device_description((void *)pDoc, stDevice) == 0)
    {
        if (stDevice.strUdn.empty())
        {
            dlog_error("UDN无效");
            goto EXIT;
        }

        /* 获取baseURL */
        std::string strBaseUrl = extract_baseURL(pLocation);
        if (strBaseUrl.empty())
        {
            dlog_error("strBaseUrl为空");
            goto EXIT;
        }

        // dlog_info("设备BaseURL==========: %s", strBaseUrl.c_str());

        stDevice.strControlURL = strBaseUrl + stDevice.strControlURL;

        auto result = m_mapdevices.emplace(stDevice.strUdn, stDevice);
        if (result.second)
        {
            dlog_info("==upnp设备添加成功，UDN: %s ControlURL:%s==", stDevice.strUdn.c_str(), stDevice.strControlURL.c_str());

            if (m_currentConfig.bEnablePortMap && !m_currentConfig.portMap.empty())
            {
                if (m_externalIP == "0.0.0.0")
                {
                    get_external_ip();
                }

                for (auto &port : m_currentConfig.portMap)
                {
                    /* 映射端口 */
                    if (!port.nStatus)
                    {
                        add_port_mapping(port);
                    }
                }
            }
        }
        else
        {

            // dlog_warn("设备已存在，UDN: %s", stDevice.strUdn.c_str());
        }
    }
EXIT:
    if (pDocStr) 
    {
        ixmlFreeDOMString(pDocStr);
    }
    if (pDoc)
    {
        ixmlDocument_free(pDoc);
    }

    return ;
}

void CUpnpManage::process_byebye_event(void *pDiscovery)
{
    const UpnpDiscovery *pUpnpDiscovery = (const UpnpDiscovery *)pDiscovery;

    dlog_info("设备离线:%s 类型:%s ", UpnpDiscovery_get_DeviceID_cstr(pUpnpDiscovery), UpnpDiscovery_get_DeviceType_cstr(pUpnpDiscovery));

    std::lock_guard<std::mutex> lock(m_deviceMutex);
    if (m_mapdevices.empty())
    {
        dlog_error("upnp设备为空");
        return;
    }

    std::string strTargetUdn = UpnpDiscovery_get_DeviceID_cstr(pUpnpDiscovery);
    auto it = m_mapdevices.find(strTargetUdn);
    if (it == m_mapdevices.end())
    {
        dlog_warn("设备不存在，UDN: %s", strTargetUdn.c_str());
        return;
    }

    /* 更新状态 */
    if (!m_currentConfig.portMap.empty())
    {
        m_externalIP = "0.0.0.0";
        for (auto &port : m_currentConfig.portMap)
        {
            port.nStatus = 0;
            port.externIp = "0.0.0.0";
        }
    }

    const Network::UpnpConfigInfo_S &stDevice = it->second;
    m_mapdevices.erase(it);
    dlog_info("删除设备成功，UDN: %s", strTargetUdn.c_str());
}
/* 解析设备xml数据 */
int CUpnpManage::parse_device_description(void *pDoc, Network::UpnpConfigInfo_S &stDevice)
{
    IXML_Document *pIxmlDoc = (IXML_Document *)pDoc;
    /* 获取upnp可映射设备类型 */
    stDevice.strServiceType = get_xml_nodeValue((void *)pIxmlDoc, "serviceType");
    if (stDevice.strServiceType == UPNP_WANPPPC_SERVICE || stDevice.strServiceType == UPNP_WANIPC_SERVICE)
    {
        // dlog_info("获取到支持upnp的设备类型；%s",stDevice.strServiceType.c_str());

        std::string strPresentationURL = get_xml_nodeValue((void *)pIxmlDoc, "presentationURL");
        std::string strControlURL = get_xml_nodeValue((void *)pIxmlDoc, "controlURL");
        /* 获取控制地址 */
        stDevice.strControlURL = strControlURL;
        if (stDevice.strControlURL.empty())
        {
            dlog_error("设备【%s】控制链接获取失败", stDevice.strServiceType.c_str());
            return -1;
        }

        /* 获取唯一标识符 */
        stDevice.strUdn = get_xml_nodeValue((void *)pIxmlDoc, "UDN");
        if (stDevice.strUdn.empty())
        {
            dlog_error("设备【%s】唯一标识符获取失败", stDevice.strServiceType.c_str());
            return -1;
        }

        /* 获取设备能力地址 */
        // std::string strSCPDURL = strPresentationURL + get_xml_nodeValue((void *)pIxmlDoc,"SCPDURL");
        //
        // IXML_Document* pIxmlDoc = nullptr;
        ////dlog_info("成功解析upnp设备信息 设备服务类型【%s】 控制链接【%s】唯一标识符【%s】",stDevice.strServiceType.c_str(),stDevice.strControlURL.c_str(),stDevice.strUdn.c_str());
        //
        // if (UpnpDownloadXmlDoc(strSCPDURL.c_str(), &pIxmlDoc) == UPNP_E_SUCCESS)
        //{
        //    dlog_info("设备服务描述文档链接:%s",strSCPDURL.c_str());
        //    char* pIxmlDocStr = ixmlDocumenttoString(pIxmlDoc);
        //    dlog_info("获取设备服务能力描述:%s",pIxmlDocStr);
        //}

        return 0;
    }

    return -1;
}

std::string CUpnpManage::get_xml_nodeValue(void *pDoc, std::string strNode)
{
    IXML_Document *pIxmlDoc = (IXML_Document *)pDoc;
    IXML_NodeList *pNodeList;
    IXML_Node *pNode;
    IXML_Node *pChNode;
    std::string strValue = "";
    char *pChStr = NULL;
    /* 获取指定节点链表 */
    pNodeList = ixmlDocument_getElementsByTagName(pIxmlDoc, strNode.c_str());
    if (pNodeList == NULL)
    {
        dlog_error("pNodeList链表获取失败");
        return "";
    }

    /* 从链表中取出一个节点 */
    pNode = ixmlNodeList_item(pNodeList, 0);
    if (pNode == NULL)
    {
        dlog_error("pNode节点获取失败");
        goto EXIT;
    }
    /* 取出该节点的子节点 */
    pChNode = ixmlNode_getFirstChild(pNode);
    if (pChNode == NULL)
    {
        dlog_error("pChNode子节点获取失败");
        goto EXIT;
    }
    /* 取出一个节点值 */
    pChStr = (char *)ixmlNode_getNodeValue(pChNode);
    if (pChStr == NULL)
    {
        dlog_error("pChStr节点数据获取失败");
        goto EXIT;
    }

    strValue = pChStr;

EXIT:
    // if(pChStr != NULL)
    //{
    //     ixmlFreeDOMString((char *)pChStr);
    // }
    // if(pNodeList != NULL)
    //{
    //     ixmlNodeList_free(pNodeList);
    // }

    return strValue;
}

void CUpnpManage::get_external_ip()
{
    std::lock_guard<std::mutex> lock(m_deviceMutex);
    if (m_mapdevices.empty())
    {
        dlog_error("upnp设备为空");
        return;
    }

    auto device = m_mapdevices.begin();
    IXML_Document *pAction = UpnpMakeAction("GetExternalIPAddress", device->second.strServiceType.c_str(), 0, nullptr);

    // dlog_debug("生成的 SOAP 请求:\n%s", ixmlDocumenttoString(pAction));

    // dlog_info("==发送获取外部ip命令==ControlURL【%s】==ServiceType【%s】",device->second.strControlURL.c_str(),device->second.strServiceType.c_str());
    int nRet = UpnpSendActionAsync(m_handle,
                                   device->second.strControlURL.c_str(),
                                   device->second.strServiceType.c_str(),
                                   NULL,
                                   pAction,
                                   action_callback,
                                   this);
    if (nRet == UPNP_E_SUCCESS)
    {
        dlog_info("发送获取外部ip命令成功");
    }
    ixmlDocument_free(pAction);

    return;
}

void CUpnpManage::check_portMapping_status(int nExternalPort)
{
    std::lock_guard<std::mutex> lock(m_deviceMutex);
    if (m_mapdevices.empty())
    {
        dlog_error("upnp设备为空");
        return;
    }

    auto device = m_mapdevices.begin();

    IXML_Document *pAction = UpnpMakeAction("GetSpecificPortMappingEntry",
                                            device->second.strServiceType.c_str(),
                                            0, nullptr);
    ;

    const char *pParams[] =
        {
            "NewRemoteHost", "",
            "NewExternalPort", std::to_string(nExternalPort).c_str(),
            "NewProtocol", UPNP_PROTOCOL};

    for (size_t i = 0; i < sizeof(pParams) / sizeof(pParams[0]); i += 2)
    {
        UpnpAddToAction(&pAction,
                        "GetSpecificPortMappingEntry",
                        device->second.strServiceType.c_str(),
                        pParams[i],
                        pParams[i + 1]);
    }

    int nRet = UpnpSendActionAsync(m_handle,
                                   device->second.strControlURL.c_str(),
                                   device->second.strServiceType.c_str(),
                                   nullptr,
                                   pAction,
                                   action_callback,
                                   this);
    if (nRet == UPNP_E_SUCCESS)
    {
        dlog_info("发送检测端口[%d]状态命令成功", nExternalPort);
    }
    else
    {
        dlog_error("发送检测端口[%d]状态命令失败", nExternalPort);
    }
    ixmlDocument_free(pAction);
}

int CUpnpManage::parse_sockaddr(const struct sockaddr_storage *pAddr, char *pIp, uint16_t *pPort)
{
    if (!pAddr || !pIp || !pPort)
    {
        dlog_error("Invalid arguments");
        return -1;
    }

    /* 根据地址族处理 IPv4/IPv6 */
    switch (pAddr->ss_family)
    {
    case AF_INET:
    {
        /* IPv4 地址处理 */
        struct sockaddr_in *ipv4 = (struct sockaddr_in *)pAddr;

        /* 将二进制 IP 转换为字符串 */
        if (inet_ntop(AF_INET, &(ipv4->sin_addr), pIp, INET_ADDRSTRLEN) == NULL)
        {
            dlog_error("inet_ntop() failed");
            return -1;
        }

        /* 端口号从网络字节序转为主机字节序 */
        *pPort = ntohs(ipv4->sin_port);
        break;
    }

    case AF_INET6:
    {
        /* IPv6 地址处理 */
        struct sockaddr_in6 *ipv6 = (struct sockaddr_in6 *)pAddr;

        /* 将二进制 IPv6 转换为字符串（如 "2001:db8::1"） */
        if (inet_ntop(AF_INET6, &(ipv6->sin6_addr), pIp, INET6_ADDRSTRLEN) == NULL)
        {
            dlog_error("inet_ntop() failed");
            return -1;
        }

        /* 端口号从网络字节序转为主机字节序 */
        *pPort = ntohs(ipv6->sin6_port);
        break;
    }

    default:
        /* 不支持其他地址族（如 AF_UNIX） */
        dlog_error("Unsupported address family: %d", pAddr->ss_family);
        return -1;
    }

    return 0;
}

std::string CUpnpManage::extract_baseURL(const char *pLocation)
{
    std::string strUrl(pLocation);

    /* 查找协议分隔符 */
    size_t protocol_pos = strUrl.find("://");
    if (protocol_pos == std::string::npos)
    {
        return "";
    }

    /* 提取协议部分（包含 ://） */
    std::string strBase = strUrl.substr(0, protocol_pos + 3);

    /* 查找主机结束位置（路径开始位置） */
    size_t host_end = strUrl.find_first_of("/?#", protocol_pos + 3);
    if (host_end != std::string::npos)
    {
        strBase += strUrl.substr(protocol_pos + 3, host_end - (protocol_pos + 3));
    }
    else
    {
        strBase += strUrl.substr(protocol_pos + 3);
    }

    /*  规范化处理（移除结尾斜杠） */
    if (!strBase.empty() && strBase.back() == '/')
    {
        strBase.pop_back();
    }

    return strBase;
}
