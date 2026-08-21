/**
 * @FilePath     : http_sdk_gateway.hpp
 * @Description  : HTTP-SDK 命令转发适配层声明
 *
 * @note 外部平台通过 HTTP 传入 NET_TV_* 命令名或命令码，本模块负责转换为设备内部
 *       ActionCode 报文，业务处理仍复用现有 shortLink/Task/Business 链路。
 */
#pragma once

#include <string>

namespace WebCGI
{
    /**
     * @brief HTTP-SDK 命令转发适配器
     */
    class CHttpSdkGateway
    {
    public:
        /**
         * @brief 判断 URI 是否为 HTTP-SDK 通用命令入口
         * @param strUri 请求 URI
         * @return true：是 HTTP-SDK 命令入口 false：不是
         */
        static bool isGatewayUri(const std::string &strUri);

        /**
         * @brief 判断当前 HTTP 请求是否需要走 SDK 命令转发
         * @param strMethod HTTP 方法
         * @param strUri 请求 URI
         * @return true：需要转发 false：不需要转发
         */
        static bool isGatewayRequest(const std::string &strMethod, const std::string &strUri);

        /**
         * @brief 根据 SDK 命令名或命令码字符串解析 SDK 命令码
         * @param strCommand SDK 命令名或命令码字符串
         * @return 返回 SDK 命令码，解析失败返回 0
         */
        static int resolveSdkCommand(const std::string &strCommand);

        /**
         * @brief 将 SDK 命令码转换为内部 ActionCode
         * @param nSdkCommand SDK 命令码
         * @return 返回内部 ActionCode，未匹配返回 0
         */
        static int sdkCommandToActionCode(int nSdkCommand);

        /**
         * @brief 根据 HTTP 请求体构造后端业务可识别的 ActionCode JSON 报文
         * @param strRequestBody HTTP 请求体，要求包含 Command、SdkCommand 或 SdkCommandName 字段
         * @param nActionCode 输出转换后的内部 ActionCode
         * @return 返回后端业务可识别的 JSON 报文
         */
        static std::string buildBackendJson(const std::string &strRequestBody, int &nActionCode);
    };
}
