/**
 * @FilePath     : cgi.hpp
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2025-06-17 11:14:14
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2025-06-20 09:53:15
 * @Description  : web通信接口封装
 */
#pragma once

// 标准C++库头文件
#include <iostream>
#include <string>
#include <memory>
#include <map>
#include <functional>
#include <vector>
#include <sstream>
#include <exception>
#include <algorithm>

extern "C"
{
#include <unistd.h>
#include <dirent.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <net/if.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <fcntl.h>
#include <sys/stat.h>
#include "file_base.h"
}

#include "cgic.h"
#include "cJSON.h"
#include "shortLink.h"
#include "action_code.h"
#include "path_define.h"
#include "edukit_network.h"
#include "dlog.h"
#include "IpcRet.h"



namespace WebCGI
{
    //info 常量定义区域
    constexpr const char *HTML_PATH = "/var/www/html"; // Web根目录路径
    constexpr const char *WEB_DEVICE_NAME = "IPC";     // Web设备名称标识
    constexpr const char *DEFAULT_USER_NAME = "admin"; // 默认用户名
    constexpr int WEB_COMMUNTICATION_PORT = 8080;      // Web通信端口号

    // 限制常量
    constexpr int MAX_REQUEST_BODY_SIZE = 2 * 1024 * 1024; // 2MB
    constexpr int LOG_BUFFER_SIZE = 512;

    //info JSON响应结构体 - 用于封装Web API的标准响应格式
    typedef struct JsonResponse
    {
        int nActionCode;           // 动作代码，标识请求的操作类型
        std::string strDeviceName; // 设备名称，用于标识响应来源设备
        std::string strUserName;   // 用户名，标识当前操作用户
        int nReturnCode;           // 返回码，标识操作执行结果（0成功，非0失败）
        std::string strData;       // 响应数据，JSON格式的业务数据

        /**
         * @brief 构造函数 - 初始化JSON响应对象
         * @param ac 动作代码，默认为0
         * @param rc 返回码，默认为0（成功）
         * @param d 响应数据，默认为空字符串
         */
        JsonResponse(int action = 0, int retCode = 0, const std::string &data = "")
            : nActionCode(action), strDeviceName(WEB_DEVICE_NAME), strUserName(DEFAULT_USER_NAME), nReturnCode(retCode), strData(data) {}
    } JsonResponse_S;

    //info CGI异常类 - 用于处理CGI操作中的错误情况
    class CGIException : public std::exception
    {
    private:
        /*错误消息描述*/
        std::string m_message;
        /*错误代码*/
        int m_nErrorCode;
    public:
        /**
         * @brief 构造函数 - 创建CGI异常对象
         * @param msg 错误消息描述
         * @param code 错误代码，默认为-1
         */
        explicit CGIException(const std::string &msg, int code = -1)
            : m_message(msg), m_nErrorCode(code) {}

        /**
         * @brief 获取异常描述信息
         * @return 返回错误消息的C风格字符串
         */
        const char *what() const noexcept override
        {
            return m_message.c_str();
        }

        /**
         * @brief 获取错误代码
         * @return 返回整型错误代码
         */
        int getErrorCode() const noexcept
        {
            return m_nErrorCode;
        }
    };

    // 智能指针类型别名
    using JsonPtr = std::unique_ptr<cJSON, void (*)(cJSON *)>;
    using CommandHandlerPtr = std::unique_ptr<class CommandHandler>;

    //info 命令处理器基类 - 定义命令处理的通用接口
    class CommandHandler
    {
    public:
        virtual ~CommandHandler() = default; // 虚析构函数，确保派生类正确析构

        /**
         * @brief 执行命令处理逻辑（纯虚函数）
         * @param pMsg 短链接回调消息指针，包含命令相关数据
         * @return 返回执行结果码（0成功，非0失败）
         */
        virtual int execute(ShortCallbackMsg_t *pMsg) = 0;

        /**
         * @brief 获取当前处理器对应的动作代码（纯虚函数）
         * @return 返回动作代码整数值
         */
        virtual int getActionCode() const = 0;
    };

    //info 登录命令处理器 - 处理用户登录相关操作
    class LoginCommandHandler : public CommandHandler
    {
    public:
        /**
         * @brief 执行登录命令处理逻辑
         * @param pMsg 短链接回调消息指针，包含登录相关数据
         * @return 返回登录处理结果码
         */
        int execute(ShortCallbackMsg_t *pMsg) override;

        /**
         * @brief 获取登录处理器的动作代码
         * @return 返回登录动作代码 AC_LOGIN
         */
        int getActionCode() const override { return AC_LOGIN; }
    };

    //info 通用JSON命令处理器 - 用于透传后端JSON响应
    class JsonCommandHandler : public CommandHandler
    {
    public:
        /**
         * @brief 构造通用JSON命令处理器
         * @param nActionCode 当前处理器绑定的动作码
         */
        explicit JsonCommandHandler(int nActionCode) : m_nActionCode(nActionCode) {}

        /**
         * @brief 执行JSON命令响应输出
         * @param pMsg 短链接回调消息指针，包含后端返回的JSON字符串
         * @return 返回处理结果码
         */
        int execute(ShortCallbackMsg_t *pMsg) override;

        /**
         * @brief 获取当前处理器绑定的动作码
         * @return 返回内部ActionCode
         */
        int getActionCode() const override { return m_nActionCode; }

    private:
        /* 当前命令处理器对应的内部动作码 */
        int m_nActionCode;
    };

    //info JSON工具类 - 提供JSON解析和生成的静态工具方法
    class JsonHelper
    {
    public:
        /**
         * @brief 解析JSON字符串为cJSON对象
         * @param strJson 待解析的JSON字符串
         * @return 返回智能指针包装的cJSON对象，自动管理内存释放
         */
        static JsonPtr parseJson(const std::string &strJson);

        /**
         * @brief 根据响应结构体创建JSON格式字符串
         * @param stResponse JsonResponse_S 结构体引用，包含响应数据
         * @return 返回格式化的JSON字符串
         */
        static std::string createJsonResponse(const JsonResponse_S &stResponse);

        /**
         * @brief 从JSON字符串中提取动作代码
         * @param strJson JSON格式字符串
         * @return 返回提取的动作代码整数值
         */
        static int extractActionCode(const std::string &strJson);

        /**
         * @brief 验证JSON字符串的结构有效性
         * @param strJson 待验证的JSON字符串
         * @return 返回true表示结构有效，false表示无效
         */
        static bool validateJsonStructure(const std::string &strJson);
    };

    //info CGI处理器主类 - 核心的Web请求处理引擎
    class CGIProcessor
    {
    public:
        /**
         * @brief 构造函数 - 初始化CGI处理器
         * 自动注册命令处理器并进行必要的初始化
         */
        CGIProcessor();

        /**
         * @brief 析构函数 - 使用默认析构
         */
        ~CGIProcessor() = default;

        // 禁用拷贝构造和赋值运算符，防止意外的对象复制
        CGIProcessor(const CGIProcessor &) = delete;
        CGIProcessor &operator=(const CGIProcessor &) = delete;

        /**
         * @brief 处理Web请求的主入口函数
         * 解析HTTP请求，提取JSON数据，调用相应的处理器
         * @return 返回处理结果码（0成功，非0失败）
         */
        int processRequest();

        /**
         * @brief 设置远程客户端IP地址
         * @param strAddr 客户端IP地址字符串
         */
        void setRemoteAddr(const std::string &strAddr) { remoteAddr = strAddr; }

    private:
        /*命令处理器映射表，key为动作代码，value为对应的处理器对象*/
        std::map<int, std::unique_ptr<CommandHandler>> commandHandlers;
        /*远程客户端IP地址*/
        std::string remoteAddr;

        /**
         * @brief 注册所有命令处理器到映射表中
         * 初始化时调用，将各种命令处理器注册到commandHandlers中
         */
        void registerHandlers();

        /**
         * @brief 设置跨域资源共享(CORS)响应头
         * 添加必要的HTTP头部信息，支持跨域访问
         */
        void setupCORSHeaders();

        /**
         * @brief 发送错误响应给客户端
         * @param nErrorCode 错误代码
         * @param strMessage 错误消息描述，默认为空
         */
        void sendErrorResponse(int nErrorCode, const std::string &strMessage = "", int nActionCode = 0);
        // void sendErrorResponse(int nErrorCode, const std::string &message = "");

        /**
         * @brief 读取HTTP请求体数据
         * 从标准输入读取POST请求的body内容
         * @return 返回请求体字符串数据
         */
        std::string readRequestBody();

        /**
         * @brief 构造后端业务报文
         * @param strRequestBody HTTP请求体
         * @param nActionCode 输出动作码
         * @return 返回后端可识别的JSON报文
         * @note 支持旧ActionCode JSON和新增HTTP-SDK转发命令两种格式。
         */
        std::string buildBackendJson(const std::string &strRequestBody, int &nActionCode);

        /**
         * @brief 处理具体的命令请求
         * @param nActionCode 动作代码，标识要执行的操作
         * @param strJsonData JSON格式的请求数据
         * @return 返回命令处理结果码
         */
        int processCommand(int nActionCode, const std::string &strJsonData);

        /**
         * @brief 发送请求到后端处理模块
         * @param nActionCode 动作代码
         * @param strJsonData JSON格式的请求数据
         * @return 返回后端处理结果码
         */
        int sendToBackend(int nActionCode, const std::string &strJsonData);

        /**
         * @brief 验证动作代码的有效性
         * @param nActionCode 待验证的动作代码
         * @return 返回true表示有效，false表示无效
         */
        bool isValidActionCode(int nActionCode);
    };

    //info 网络通信封装类 - 提供网络相关的静态工具方法
    class NetworkClient
    {
    public:
        /**
         * @brief 发送网络请求到指定目标
         * @param stRequest 短链接发送请求结构体，包含目标地址和数据
         * @return 返回发送结果码（0成功，非0失败）
         */
        static int sendRequest(const ShortLink_Send_t &stRequest);
    };

    //info 工具函数命名空间 - 提供通用的辅助函数
    namespace Utils
    {
        void redirectStdioToLog();
        void debugLog(const char *pFunction, int nLine, const char *pFormat, ...);

        /**
         * @brief 获取当前时间戳字符串
         * 生成格式化的当前时间字符串，用于日志记录等
         * @return 返回时间戳字符串
         */
        std::string getCurrentTimestamp();

        /**
         * @brief 清理和过滤输入字符串
         * 移除或转义潜在的危险字符，防止注入攻击
         * @param strInput 原始输入字符串
         * @return 返回清理后的安全字符串
         */
        std::string sanitizeInputString(const std::string &strInput);

        // template <typename T>
        // bool isInRange(T value, T minVal, T maxVal)
        // {
        //     return value >= minVal && value <= maxVal;
        // }
    }

} // namespace WebCGI 命名空间结束

// 调试日志宏
#define DEBUG_LOG(fmt, ...) WebCGI::Utils::debugLog(__FUNCTION__, __LINE__, fmt, ##__VA_ARGS__)
