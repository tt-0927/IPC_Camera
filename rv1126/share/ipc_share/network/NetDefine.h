#pragma once

#include <string>
#include <functional>
#include <map>
#include <cstdint>
#include <iostream>

#include "path_define.h"

extern int random_num;
namespace Net
{
    constexpr const char * const UDS_PATH = UDS_SOCKET_PATH;

    using ActionCode = int;

    typedef enum
    {
        STATUS_DISCONNECT = 0, /* 连接断开 */
        STATUS_SUCCESS,        /* 连接成功 */
        STATUS_ERROR,          /* 连接错误 */
    } Status_E;

    /* 数据回调参数 */
    typedef struct
    {
        /// @brief 命令码
        ActionCode nActionCode = 0;
        /// @brief 数据
        const void* pData = nullptr;
        /// @brief 数据长度
        int nDataLength = 0;
        /// @brief 通讯句柄
        void* pHandle = nullptr;
        /// @brief 通信ip
        std::string ip;
    } Message_S;

    typedef struct
    {
        /// @brief 用户自定义参数
        void* pData = nullptr;
        /// @brief 用户自定义参数长度
        int   nLength = 0;
        void clear()
        {
            pData = nullptr;
            nLength = 0;
        }
    } UserParam_S;

    /// @brief 回调函数类型定义
    using MessageCallback = std::function<void(Message_S& stMessage, UserParam_S &stUserParam)>;

    typedef struct
    {
        /// @brief ip地址
        std::string ip = std::string();
        /// @brief 端口
        int nPort = 0;
        ///  @brief 是否开启ssl
        bool bEnssl = false;
        ///  @brief 证书
        std::string strCert = std::string();
        ///  @brief 密钥
        std::string strKey = std::string();
        ///  @brief 域名
        std::string strVhost = std::string();

        /// @brief 队列大小
        int nQueueSize = 20;
        /// @brief 自定义心跳码
        int nHearbeatCode = 65533;
        /// @brief 心跳间隔，ms
        int nHeartbeatInterval = 10000;
        /// @brief 自定义状态码
        int nStatusCode = 65534;
        /// @brief 命令码默认回调，指定了回调函数后，按指定的函数回调
        MessageCallback fnDefaultCallback;
        /// @brief 指定命令码回调
        std::map<ActionCode, MessageCallback> callbackMap;
        void clear()
        {
            ip.clear();
            nPort = 0;
            nHearbeatCode = 0;
            nQueueSize = 20;
            nStatusCode = 0;
            fnDefaultCallback = nullptr;
            callbackMap.clear();
        }
    } InitParam_S;

    /// @brief 参数结构体
    typedef struct
    {
        /// @brief 初始化参数
        InitParam_S stInitParam;
        /// @brief 用户参数
        UserParam_S stUserParam;
        void clear()
        {
            stInitParam.clear();
            stUserParam.clear();
        }
    } Param_S;

    /* 标准协议 */
    // //强制1字节对齐
    // #pragma pack(1)
    // typedef struct
    // {
    //     uint8_t nCheckCode1 = 0x3E;
    //     //项目代号
    //     uint16_t nProjectCode = 0xabcd;
    //     uint16_t nActionCode = 0;
    //     //数据长度
    //     uint16_t nDataLength = 0;
    //     uint8_t nCheckCode2 = 0x3E;
    // } MessageHead_S;
    // #pragma pack()
    
    /* 公司协议 */
    // //强制1字节对齐
    // #pragma pack(1)
    // typedef struct
    // {
    //     uint8_t nCheckCode1[2] = {'B','L'};//0x3E;
    //     //项目代号
    //     uint16_t nProjectCode = 0;
    //     uint16_t nActionCode = 0;
    //     //数据长度
    //     uint16_t nDataLength = 0;
    //     uint8_t nCheckCode2 = 0;
    // } MessageHead_S;
    // #pragma pack()

    /* 内部协议 */
    //强制1字节对齐
    #pragma pack(1)
    typedef struct
    {
        uint8_t nCheckCode1[4] = {'@','#','$','&'};
        //项目代号
        int nProjectCode = 2015;
        int nDataLength = 0;
        int nActionCode = 0;
        //数据长度
        int nCheckCode2 = 0;
    } MessageHead_S;
    #pragma pack()

}