/**
 * @file LibWSServer.h
 * @author zhangjc (zhangjc@kfb.cn)
 * @date 2024-10-08
 * 
 * @brief 
 */

#pragma once

#include "NetDefine.h"

#include "libwebsockets.h"
#include <list>
#include <memory>
#include <mutex>
#include <set>
#include <thread>
#include <vector>
#include <fstream>
#include "WsUpload.h"

#define QOS_DSCP_MIN 0
#define QOS_DSCP_MAX 63

struct FileTransferInfo_S {
    std::string filename;               // 文件名
    size_t receivedSize = 0;            // 已接收的大小
    size_t totalSize = 0;               // 总文件大小
    std::ofstream outFile;              // 文件输出流
};

class LibWSServer
{
public:

    /* 日志等级 不是随便定的，必须参考lws-logs.h */
    typedef enum _WebLogLevel_
    {
        WS_LLL_ERR     = (1 << 0),
        WS_LLL_WARN    = (1 << 1),
        WS_LLL_NOTICE  = (1 << 2),
        WS_LLL_INFO    = (1 << 3),
        WS_LLL_DEBUG   = (1 << 4),
        WS_LLL_PARSER  = (1 << 5),
        WS_LLL_HEADER  = (1 << 6),
        WS_LLL_EXT     = (1 << 7),
        WS_LLL_CLIENT  = (1 << 8),
        WS_LLL_LATENCY = (1 << 9),
        WS_LLL_USER    = (1 << 10),
        WS_LLL_THREAD  = (1 << 11),
    } WebLogLevel_E;

     /* 为每条消息创建一个 */
    typedef struct _MsgInfo_
    {
        std::shared_ptr<char[]> pData; /* 数据 */
        size_t                  nLen;  /* 数据长度 */
    } MsgInfo_S;

    /* 为每个连接到我们的客户端创建其中一个 */
    typedef struct _ClientInfo_
    {
        struct lws* pWsi; /* WebSocket连接句柄 */

        std::list<MsgInfo_S> listMsgInfo;
        std::vector<char> vectorRecvBuf;
        std::mutex mutex;
    } ClientInfo_S;

    /* 为使用我们协议的每个虚拟主机创建一个句柄信息 */
    typedef struct _VhostHandleInfo_
    {
        struct lws_context*         pContext;
        struct lws_vhost*           pVhost;
        const struct lws_protocols* pProtocol;

        // std::mutex clientDataMutex;
        std::set<ClientInfo_S*> clientInfos; /* 客户端信息列表集合 */

    } VhostHandleInfo_S;

    LibWSServer(Net::Param_S &stParam, Net::MessageCallback fnMessageCallback);
    ~LibWSServer();
    int send(const Net::Message_S stMessage);
    /**
     * @brief 断开所有客户端并关闭服务器
     */
    void disconnect();
    /**
     * @brief 设置上传文件路径
     * @param strFilePath 
     */
    void set_file_upload_path(const std::string &strFilePath);
    /**
     * @brief 获取上传文件名称
     * @return std::string 
     */
    std::string get_upload_filename();
    /**
     * @brief 设置Qos的Dscp
     * @param nDscp 
     * @return int 
     */
    static int setQosDscp(const int &nDscp);
private:
    /**
     * @brief 通讯线程
     */
    void run();
    /**
     * @brief 通讯回调函数
     * @param pWsi 通讯句柄
     * @param enReason 通讯码枚举
     * @param pUser 用户自定义参数
     * @param pIn 数据内容
     * @param nLen 数据长度
     * @return int 
     */
    static int callback(struct lws* pWsi, enum lws_callback_reasons enReason, void* pUser, void* pIn, size_t nLen);
    void upgrade_thread_func(struct lws *pWsi, std::string filePath);
    static int file_upload_callback(struct lws* pWsi, enum lws_callback_reasons enReason, void* pUser, void* pIn, size_t nLen);
private:
    /*初始化参数*/
    Net::Param_S m_stParam;
    Net::MessageCallback m_fnMessageCallback;
    std::string m_sendData;
    struct lws_context* m_pContext = nullptr;
    CWsUpload m_wsUpload;
    
    /* 存储所有WebSocket连接的集合 */
    std::mutex m_mutex;
    std::set<VhostHandleInfo_S*> m_connections;
    /* 线程 */
    bool m_bExit = false;
    std::thread m_tid;

    // ===== 新增升级进度相关 =====
    std::mutex m_upgradeMutex;                          // 保护升级进度容器
    std::map<struct lws *, int> m_upgradeProgress;     // key: 客户端pWsi, value: 升级进度(0-100)

     /**
     * @brief 上传的文件路径
     */
    static std::string m_filePath;  
    static std::string m_imagePath;
    /**
     * @brief 上传的文件名称
     */
    static std::string m_uploadfFileName;  
    /**
     * @brief QOS_DSCP
     */
    static int nManageDscp; 
  
};

