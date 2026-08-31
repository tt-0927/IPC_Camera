/**
 * @file ErrorManage.h
 * @author tianl (tianl@kfb.cn)
 * @date 2026-07-28
 * @LastEditors  : qinjt@kfb.cn
 * @LastEditTime : 2026-07-28
 *
 * @brief ErrorManage 模块接口与类型定义
 * 功能说明：
 * 1. 声明 ErrorManage 模块对外接口和数据类型
 * 2. 定义模块依赖的常量、回调或辅助类型
 * 3. 为调用方提供明确且稳定的编译期契约
 */
#pragma once
#include <string>
#include <map>
#include <unordered_map>
#include <mutex>

#include "Singleton.h"
#include "NetTVSDKCommon.h"

class CErrorManage : public CSingleton<CErrorManage>
{
    CErrorManage()
	{
        InitErrorMsgMap();
	}
public:

	~CErrorManage()
	{

	}
	friend class CSingleton<CErrorManage>;

public:

    /**
     * @author tianl (tianl@kfb.cn)
     * @brief 设置最后错误码
     * @param [in] code 错误码
     * @return 无返回值。
     */
    void SetLastError(int code)
	{
        s_nLastErrorCode = code;
    }

    /**
 * @author tianl (tianl@kfb.cn)
     * @brief 获取最后错误码
     * @return 错误码
     */
    int GetLastError()
	{
        return s_nLastErrorCode;
    }

    /**
 * @author tianl (tianl@kfb.cn)
     * @brief 获取最后错误码对应的描述信息
     * @return 错误描述字符串（UTF-8），未找到对应描述时返回"未知错误"
     * @note 返回指针指向内部静态字符串，调用方无需释放内存
     */
    const char* GetErrorMsg()
	{
        auto it = m_stErrorMsgMap.find(s_nLastErrorCode);
        if (it != m_stErrorMsgMap.end())
            return it->second.c_str();
        return "未知错误 Unknown error";
    }
private:

    /**
     * @brief 初始化错误码→描述映射表
     * @details 覆盖 NET_COMMON_ECODE_E 全部枚举值，描述与 SDK_API_PARAM.md 保持一致
     */
    void InitErrorMsgMap()
    {
        /* 基础错误码（-1 ~ 6） */
        m_stErrorMsgMap[NET_E_FAILED]                = "操作失败 Operation failed";
        m_stErrorMsgMap[NET_E_SUCCEED]               = "操作成功 Operation succeeded";
        m_stErrorMsgMap[NET_E_SVC_FAILED]            = "服务器错误 Server error";
        m_stErrorMsgMap[NET_E_NOT_AUTHORIZED]        = "用户无权限 User not authorized";
        m_stErrorMsgMap[NET_E_NO_USER]               = "用户不存在 User does not exist";
        m_stErrorMsgMap[NET_E_SDK_NOT_INIT]          = "SDK未初始化 SDK not initialized";

        /* 数据资源错误码（11 ~ 24） */
        m_stErrorMsgMap[NET_E_NO_RESULT]             = "查无结果 No result";
        m_stErrorMsgMap[NET_E_NOENOUGH_BUF]           = "缓冲区太小 Buffer too small";
        m_stErrorMsgMap[NET_E_SDK_SOCKET_LSN_FAIL]   = "创建socket监听失败 Socket listen failed";
        m_stErrorMsgMap[NET_E_INIT_MUTEX_FAIL]        = "初始化锁失败 Mutex init failed";
        m_stErrorMsgMap[NET_E_INIT_SEMA_FAIL]         = "初始化信号量失败 Semaphore init failed";
        m_stErrorMsgMap[NET_E_ALLOC_RESOURCE_ERROR]  = "SDK资源分配错误 SDK resource allocation error";
        m_stErrorMsgMap[NET_E_HAVEDATA]              = "数据未全部发送 Data not all sent";
        m_stErrorMsgMap[NET_E_NEEDMOREDATA]          = "需要更多数据 More data required";
        m_stErrorMsgMap[NET_E_TRANSFILE_FAIL]        = "文件传输失败 File transmission failed";
        m_stErrorMsgMap[NET_E_DEVICE_TYPE_ERR]       = "不支持的设备类型 Unsupported device type";
        m_stErrorMsgMap[NET_E_NONCE_TIMEOUT]         = "nonce过期 Nonce expired";
        m_stErrorMsgMap[NET_E_INNER_ERR]             = "系统内部错误 System internal error";
        m_stErrorMsgMap[NET_E_BINDNOTIFY_FAIL]       = "绑定告警失败 Failed to bind alarms";

        /* 系统级错误码（100 ~ 132） */
        m_stErrorMsgMap[NET_E_SYSCALL_FALIED]        = "系统函数调用失败 System call failed";
        m_stErrorMsgMap[NET_E_NULL_POINT]            = "空指针 Null pointer";
        m_stErrorMsgMap[NET_E_INVALID_PARAM]         = "无效参数 Invalid parameter";
        m_stErrorMsgMap[NET_E_INVALID_MODULEID]      = "无效模块ID Invalid module ID";
        m_stErrorMsgMap[NET_E_INVALID_HANDLE]        = "无效句柄 Invalid handle";
        m_stErrorMsgMap[NET_E_NO_MEMORY]             = "内存分配失败 Memory allocation failed";
        m_stErrorMsgMap[NET_E_FILE_NO_EXIST]         = "文件不存在 File does not exist";
        m_stErrorMsgMap[NET_E_NO_DEV]                = "设备不存在 Device does not exist";
        m_stErrorMsgMap[NET_E_NO_FIT_LOG]            = "符合条件的日志不存在 No matching logs";
        m_stErrorMsgMap[NET_E_BUSY]                  = "资源忙 Resource busy";
        m_stErrorMsgMap[NET_E_TIMER_REG_FAILED]      = "注册定时器失败 Timer registration failed";
        m_stErrorMsgMap[NET_E_COMMON_FAILED]         = "通用错误 General error";
        m_stErrorMsgMap[NET_E_CMD_NOT_SUPPORT]       = "命令不支持 Command not supported";
        m_stErrorMsgMap[NET_E_NOT_SUPPORT]           = "设备不支持该功能 Function not supported by device";
        m_stErrorMsgMap[NET_E_TIMEOUT]               = "超时 Operation timeout";
        m_stErrorMsgMap[NET_E_MSG_ERR]               = "消息不匹配 Message mismatch";
        m_stErrorMsgMap[NET_E_MODULE_INEXIST]        = "模块不存在 Module does not exist";
        m_stErrorMsgMap[NET_E_SOCKET_RECV_ERR]       = "消息接收失败 Message receive failed";
        m_stErrorMsgMap[NET_E_DECODE_IE_FAILED]      = "获取消息IE失败 Failed to get message IE";
        m_stErrorMsgMap[NET_E_ENCODE_IE_FAILED]      = "添加消息IE失败 Failed to add message IE";
        m_stErrorMsgMap[NET_E_SDK_NOINTE_ERROR]      = "SDK未初始化 SDK not initialized";
        m_stErrorMsgMap[NET_E_ALREDY_INIT_ERROR]     = "SDK已初始化 SDK already initialized";
        m_stErrorMsgMap[NET_E_DEVICE_FACTURER_ERR]   = "不支持的设备厂商 Unsupported manufacturer";
        m_stErrorMsgMap[NET_E_NAME_EXIST]            = "名称已存在 Name already exists";
        m_stErrorMsgMap[NET_E_GET_CFG_FAILED]        = "获取配置失败 Failed to get configuration";
        m_stErrorMsgMap[NET_E_SET_CFG_FAILED]        = "设置配置失败 Failed to set configuration";
        m_stErrorMsgMap[NET_E_CHANNEL_OVER_SPEC]     = "通道数超规格 Channel count exceeds limit";
        m_stErrorMsgMap[NET_E_CALL_DRV_COMMON]       = "调用驱动通用失败 Driver call failed";
        m_stErrorMsgMap[NET_E_TOTAL_QUOTA_FULL]      = "配额空间不足 Quota space exhausted";
        m_stErrorMsgMap[NET_E_CALL_DB_COMMON]        = "调用数据库通用失败 Database call failed";
        m_stErrorMsgMap[NET_E_NEED_MORE_MEMORY]      = "内存分配不足 Insufficient memory";
        m_stErrorMsgMap[NET_E_T2U_CONNECT_FAILED]    = "T2U连接失败 T2U connection failed";
        m_stErrorMsgMap[NET_E_FUNC_IS_INITIALIZING]  = "功能正在初始化 Function initializing";

        /* 通信层错误码（200 ~ 211） */
        m_stErrorMsgMap[NET_E_CONNECT_ERROR]         = "创建连接失败 Connection failed";
        m_stErrorMsgMap[NET_E_SEND_MSG_ERROR]        = "发送消息失败 Send message failed";
        m_stErrorMsgMap[NET_E_DECODE_RSP_ERROR]      = "解析响应失败 Decode response failed";
        m_stErrorMsgMap[NET_E_NONSUPPORT]            = "功能未实现 Function not implemented";
        m_stErrorMsgMap[NET_E_JSON_ERROR]            = "JSON错误 JSON error";
        m_stErrorMsgMap[NET_E_NORESULT]              = "查询结果为空 Query result empty";
        m_stErrorMsgMap[NET_E_SOCKET_RECV_ERROR]     = "Socket接收失败 Socket receive failed";
        m_stErrorMsgMap[NET_E_CREATE_THREAD_FAIL]    = "创建线程失败 Thread creation failed";
        m_stErrorMsgMap[NET_E_RESCODE_NO_EXIST]      = "资源编码不存在 Resource code not exist";
        m_stErrorMsgMap[NET_E_MSG_DATA_INVALID]      = "消息内容错误 Invalid message content";
        m_stErrorMsgMap[NET_E_JSON_NO_IMAGE]         = "图片数据为空 Image data empty";
        m_stErrorMsgMap[NET_E_IMAGE_SIZE_BEYOND_THE_LIMIT] = "图片大小超出限制 Image size exceeds limit";
    }

    std::unordered_map<int, std::string> m_stErrorMsgMap;
    static thread_local int s_nLastErrorCode;
};
