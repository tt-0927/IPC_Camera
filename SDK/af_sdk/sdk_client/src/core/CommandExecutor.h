/**
 * @file CommandExecutor.h
 * @author tianl (tianl@kfb.cn)
 * @date 2026-07-28
 * @LastEditors  : qinjt@kfb.cn
 * @LastEditTime : 2026-07-28
 *
 * @brief CommandExecutor 模块接口与类型定义
 * 功能说明：
 * 1. 声明 CommandExecutor 模块对外接口和数据类型
 * 2. 定义模块依赖的常量、回调或辅助类型
 * 3. 为调用方提供明确且稳定的编译期契约
 */
#pragma once
#include "DeviceManage.h"
#include "ErrorManage.h"
#include "CapabilityInfoConvert.h"
#include "DeviceInfoConvert.h"
#include "AlarmInfoConvert.h"
#include "SDKConvert.h"
#include <cstring>
#include <string>
#include <fstream>
#include <vector>

#include "Singleton.h"

class CommandExecutor : public CSingleton<CommandExecutor>
{
    CommandExecutor()
	{

	}
public:

    ~CommandExecutor()
    {

    }
    friend class CSingleton<CommandExecutor>;

    /**
 * @author tianl (tianl@kfb.cn)
     * @brief 转换GET响应（通用模板）
     * @param [in] respBody JSON响应体
     * @param [out] out 输出结构体
     * @return 成功返回true，失败返回false
     */
    template <typename T_RESP>
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 NetTV_ConvertGetResponse 定义的内联处理。
 * @param [in] respBody 函数处理参数。
 * @param [in,out] out 函数处理参数。
 * @return 返回该处理的状态或结果。
 */
    static inline bool NetTV_ConvertGetResponse(const std::string& respBody, T_RESP& out)
    {
        SDKConvert::to_respStruct(respBody, out);
        return true;
    }

    /**
 * @author tianl (tianl@kfb.cn)
     * @brief 转换GET响应（通道信息）
     * @param [in] respBody JSON响应体
     * @param [out] out 通道信息结构体
     * @return 成功返回true，失败返回false
     */
    static inline bool NetTV_ConvertGetResponse(const std::string& respBody, NET_ChannelInfo_S& out)
    {
        std::memset(&out, 0, sizeof(out));
        SDKConvert::to_respStruct(respBody, out);
        if (out.dwSize == 0)
        {
            out.dwSize = sizeof(out);
        }
        return true;
    }

    /**
 * @author tianl (tianl@kfb.cn)
     * @brief 转换GET响应（通道列表）
     * @param [in] respBody JSON响应体
     * @param [out] out 通道列表结构体
     * @return 成功返回true，失败返回false
     */
    static inline bool NetTV_ConvertGetResponse(const std::string& respBody, NET_ChannelList_S& out)
    {
        std::memset(&out, 0, sizeof(out));
        SDKConvert::to_respStruct(respBody, out);
        if (out.dwSize == 0)
        {
            out.dwSize = sizeof(out);
        }
        return true;
    }

    /* ======================================================================== */
    /* 场景 1: 有返回值的查询 (GET) */
    /* T_RESP: 期望返回的结构体类型 */
    /* ======================================================================== */

    /**
 * @author tianl (tianl@kfb.cn)
     * @brief 执行GET请求（模板版本）
     * @param [in] pHandle 用户登录句柄
     * @param [in] url 请求URL
     * @param [out] pOut 输出缓冲区
     * @param [out] pOutLen 输出数据长度
     * @return 成功返回true，失败返回false
     */
    template <typename T_RESP>
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 ExecuteGet 定义的内联处理。
 * @param [in] pHandle 函数处理参数。
 * @param [in] url 函数处理参数。
 * @param [in,out] pOut 函数处理参数。
 * @param [in,out] pOutLen 函数处理参数。
 * @return 返回该处理的状态或结果。
 */
    bool ExecuteGet(LPUSER_HANDLE pHandle, const std::string& url, void* pOut, int* pOutLen)
	{
        if (!pOut)
		{
			CErrorManage::instance()->SetLastError(NET_E_INVALID_PARAM);
            return false;
        }

        auto session = CDeviceManage::instance()->GetSession(pHandle);
        if (!session)
		{
			CErrorManage::instance()->SetLastError(NET_E_NO_USER);
            return false;
        }

        std::string respBody;
        CommandRequest_S req("GET", url);

        if (session->SendRequest(req, respBody))
		{

			T_RESP* pStruct = static_cast<T_RESP*>(pOut);
			if (!NetTV_ConvertGetResponse(respBody, *pStruct))
            {
                return false;
            }

			if (pOutLen) *pOutLen = sizeof(T_RESP);
			return true;

        }
        printf("\n SendRequest faild \n");
        return false;
    }

    /* ======================================================================== */
    /* 场景 2: 带输入的控制指令 (POST/PUT) */
    /* T_REQ: 输入数据的结构体类型 */
    /* ======================================================================== */

    /**
 * @author tianl (tianl@kfb.cn)
     * @brief 执行POST/PUT请求（模板版本）
     * @param [in] pHandle 用户登录句柄
     * @param [in] method 请求方法（POST/PUT）
     * @param [in] url 请求URL
     * @param [in] pIn 输入数据缓冲区
     * @return 成功返回true，失败返回false
     */
    template <typename T_REQ>
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 ExecuteSet 定义的内联处理。
 * @param [in] pHandle 函数处理参数。
 * @param [in] method 函数处理参数。
 * @param [in] url 函数处理参数。
 * @param [in,out] pIn 函数处理参数。
 * @return 返回该处理的状态或结果。
 */
    bool ExecuteSet(LPUSER_HANDLE pHandle, const std::string& method, const std::string& url, void* pIn)
	{
        if (!pIn)
		{
			CErrorManage::instance()->SetLastError(NET_E_INVALID_PARAM);
            return false;
        }

        auto session = CDeviceManage::instance()->GetSession(pHandle);
        if (!session)
		{
			CErrorManage::instance()->SetLastError(NET_E_NO_USER);
            return false;
        }

        T_REQ* pStruct = static_cast<T_REQ*>(pIn);
        std::string jsonBody = SDKConvert::to_string(*pStruct);

        CommandRequest_S req(method, url);
        req.jsonBody = jsonBody;

        std::string ignoreResp;
        return session->SendRequest(req, ignoreResp);
    }

    /* ======================================================================== */
    /* 场景 3: 万能通用执行 (直接传 JSON 字符串或需要自定义处理) */
    /* ======================================================================== */

    /**
 * @author tianl (tianl@kfb.cn)
     * @brief 执行原始请求（直接传JSON字符串）
     * @param [in] pHandle 用户登录句柄
     * @param [in] method 请求方法（GET/POST/PUT/DELETE）
     * @param [in] url 请求URL
     * @param [in] body JSON请求体
     * @param [out] outResp 响应体输出
     * @return 成功返回true，失败返回false
     */
    bool ExecuteRaw(LPUSER_HANDLE pHandle, const std::string& method, const std::string& url,
                   const std::string& body, std::string& outResp)
		{
        auto session = CDeviceManage::instance()->GetSession(pHandle);
        if (!session)
		{
            CErrorManage::instance()->SetLastError(NET_E_NO_USER);
            return false;
        }

        CommandRequest_S req(method, url);
        req.jsonBody = body;
        return session->SendRequest(req, outResp);
    }

    /* ======================================================================== */
    /* 场景 4: 文件上传 (PUT/POST 二进制) */
    /* ======================================================================== */

    /**
 * @author tianl (tianl@kfb.cn)
     * @brief 执行文件上传请求
     * @param [in] pHandle 用户登录句柄
     * @param [in] method 请求方法（PUT/POST）
     * @param [in] url 请求URL
     * @param [in] filePath 本地文件路径
     * @param [out] outResp 响应体输出
     * @return 成功返回true，失败返回false
     */
    bool ExecuteUpload(LPUSER_HANDLE pHandle, const std::string& method, const std::string& url,
                       const std::string& filePath, std::string& outResp)
    {
        auto session = CDeviceManage::instance()->GetSession(pHandle);
        if (!session)
        {
            CErrorManage::instance()->SetLastError(NET_E_NO_USER);
            return false;
        }

        /* 读取文件到内存 */
        std::ifstream file(filePath, std::ios::binary | std::ios::ate);
        if (!file.is_open())
        {
            CErrorManage::instance()->SetLastError(NET_E_INVALID_PARAM);
            return false;
        }

        const auto fileSize = file.tellg();
        file.seekg(0, std::ios::beg);

        std::vector<char> fileData(static_cast<std::size_t>(fileSize));
        if (!file.read(fileData.data(), fileSize))
        {
            CErrorManage::instance()->SetLastError(NET_E_SOCKET_RECV_ERR);
            return false;
        }

        CommandRequest_S req(method, url);
        req.binData = fileData.data();
        req.binSize = static_cast<std::size_t>(fileSize);

        return session->SendRequest(req, outResp);
    }

};
