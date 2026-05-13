/**
 * @FilePath     : http_sdk_gateway.cpp
 * @Description  : HTTP-SDK 命令转发适配层实现
 */

#include "http_sdk_gateway.hpp"

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <memory>
#include <stdexcept>
#include <string>

#include "action_code.h"
#include "cJSON.h"

namespace WebCGI
{
    namespace
    {
        constexpr const char *HTTP_SDK_COMMAND_URI = "/api/v1/sdk/command";
        constexpr const char *HTTP_DEVICE_NAME = "IPC";
        constexpr const char *HTTP_DEFAULT_USER = "admin";

        constexpr int SDK_NET_TV_GET_FACECAPTUREINFO = 246;
        constexpr int SDK_NET_TV_SET_FACECAPTUREINFO = 247;
        constexpr int SDK_NET_TV_SET_FACE_COMPARE_INFO = 482;
        constexpr int SDK_NET_TV_ADD_TARGET_LIB = 483;
        constexpr int SDK_NET_TV_DEL_TARGET_LIB = 484;
        constexpr int SDK_NET_TV_SET_TARGET_LIB = 485;
        constexpr int SDK_NET_TV_GET_TARGET_LIB = 486;
        constexpr int SDK_NET_TV_ADD_FACE_INFO = 487;
        constexpr int SDK_NET_TV_DEL_FACE_INFO = 488;
        constexpr int SDK_NET_TV_SET_FACE_INFO = 489;
        constexpr int SDK_NET_TV_GET_FACE_INFO = 490;

        /**
         * @brief SDK 命令到内部 ActionCode 的映射项
         */
        struct SdkCommandMap_S
        {
            const char *pCommandName;
            int nSdkCommand;
            int nActionCode;
            const char *pDescription;
        };

        const SdkCommandMap_S g_astCommandMap[] = {
            {"NET_TV_GET_FACECAPTUREINFO", SDK_NET_TV_GET_FACECAPTUREINFO, AC_GET_FACE_CAPTURE_INFO, "获取人脸抓拍配置"},
            {"NET_TV_GET_FACE_CAPTURE_INFO", SDK_NET_TV_GET_FACECAPTUREINFO, AC_GET_FACE_CAPTURE_INFO, "获取人脸抓拍配置"},
            {"NET_TV_SET_FACECAPTUREINFO", SDK_NET_TV_SET_FACECAPTUREINFO, AC_SET_FACE_CAPTURE_INFO, "设置人脸抓拍配置"},
            {"NET_TV_SET_FACE_CAPTURE_INFO", SDK_NET_TV_SET_FACECAPTUREINFO, AC_SET_FACE_CAPTURE_INFO, "设置人脸抓拍配置"},
            {"NET_TV_SET_FACE_COMPARE_INFO", SDK_NET_TV_SET_FACE_COMPARE_INFO, AC_SET_FACE_COMPARE_INFO, "设置人脸比对配置"},
            {"NET_TV_ADD_TARGET_LIB", SDK_NET_TV_ADD_TARGET_LIB, AC_ADD_TARGET_LIB, "添加目标库"},
            {"NET_TV_DEL_TARGET_LIB", SDK_NET_TV_DEL_TARGET_LIB, AC_DEL_TARGET_LIB, "删除目标库"},
            {"NET_TV_SET_TARGET_LIB", SDK_NET_TV_SET_TARGET_LIB, AC_SET_TARGET_LIB, "修改目标库"},
            {"NET_TV_GET_TARGET_LIB", SDK_NET_TV_GET_TARGET_LIB, AC_GET_TARGET_LIB, "获取目标库"},
            {"NET_TV_ADD_FACE_INFO", SDK_NET_TV_ADD_FACE_INFO, AC_ADD_FACE_INFO, "添加人脸"},
            {"NET_TV_DEL_FACE_INFO", SDK_NET_TV_DEL_FACE_INFO, AC_DEL_FACE_INFO, "删除人脸"},
            {"NET_TV_SET_FACE_INFO", SDK_NET_TV_SET_FACE_INFO, AC_SET_FACE_INFO, "修改人脸"},
            {"NET_TV_GET_FACE_INFO", SDK_NET_TV_GET_FACE_INFO, AC_GET_FACE_INFO, "获取人脸"},
        };

        using JsonPtr = std::unique_ptr<cJSON, void (*)(cJSON *)>;

        /**
         * @brief 去除 URI 中的查询参数
         * @param strUri 原始 URI
         * @return 返回不包含 query string 的 URI 路径
         */
        std::string stripQuery(const std::string &strUri)
        {
            const std::string::size_type nPos = strUri.find('?');
            return nPos == std::string::npos ? strUri : strUri.substr(0, nPos);
        }

        /**
         * @brief 判断 URI 是否匹配指定接口后缀
         * @param strUri 请求 URI
         * @param strSuffix 接口路径后缀
         * @return true：匹配 false：不匹配
         */
        bool uriMatches(const std::string &strUri, const std::string &strSuffix)
        {
            const std::string strPath = stripQuery(strUri);
            if (strPath == strSuffix)
            {
                return true;
            }

            return strPath.size() > strSuffix.size() &&
                   strPath.compare(strPath.size() - strSuffix.size(), strSuffix.size(), strSuffix) == 0;
        }

        /**
         * @brief 判断字符串是否为十进制数字
         * @param strValue 待检查字符串
         * @return true：数字字符串 false：非数字字符串
         */
        bool isDecimalNumber(const std::string &strValue)
        {
            if (strValue.empty())
            {
                return false;
            }

            return std::all_of(strValue.begin(), strValue.end(), [](unsigned char ch) {
                return std::isdigit(ch) != 0;
            });
        }

        /**
         * @brief 标准化 SDK 命令名
         * @param strCommand 原始命令名
         * @return 返回去空格并转为大写后的命令名
         */
        std::string normalizeCommandName(const std::string &strCommand)
        {
            std::string strResult;
            strResult.reserve(strCommand.size());
            for (char ch : strCommand)
            {
                if (!std::isspace(static_cast<unsigned char>(ch)))
                {
                    strResult.push_back(static_cast<char>(std::toupper(static_cast<unsigned char>(ch))));
                }
            }
            return strResult;
        }

        /**
         * @brief 根据 SDK 命令码查找命令名
         * @param nSdkCommand SDK 命令码
         * @return 返回 SDK 命令名，未找到返回空字符串
         */
        const char *findCommandName(int nSdkCommand)
        {
            for (const auto &stItem : g_astCommandMap)
            {
                if (stItem.nSdkCommand == nSdkCommand)
                {
                    return stItem.pCommandName;
                }
            }
            return "";
        }

        /**
         * @brief 从 JSON 中读取 SDK 命令码
         * @param pRoot JSON 根对象
         * @return 返回 SDK 命令码，读取失败返回 0
         */
        int extractSdkCommand(cJSON *pRoot)
        {
            if (pRoot == nullptr)
            {
                return 0;
            }

            cJSON *pCommand = cJSON_GetObjectItem(pRoot, "Command");
            if (pCommand == nullptr)
            {
                pCommand = cJSON_GetObjectItem(pRoot, "SdkCommand");
            }
            if (pCommand == nullptr)
            {
                pCommand = cJSON_GetObjectItem(pRoot, "SdkCommandName");
            }

            if (pCommand == nullptr)
            {
                return 0;
            }

            if (cJSON_IsNumber(pCommand))
            {
                return static_cast<int>(pCommand->valuedouble);
            }
            if (cJSON_IsString(pCommand) && pCommand->valuestring != nullptr)
            {
                return CHttpSdkGateway::resolveSdkCommand(pCommand->valuestring);
            }

            return 0;
        }

        /**
         * @brief 从请求 JSON 中复制业务 Data 对象
         * @param pRoot 请求 JSON 根对象
         * @return 返回业务 Data 对象，调用方负责挂载或释放
         * @note 若请求体存在 Data 字段，则使用 Data；否则复制根对象并去掉命令路由字段。
         */
        cJSON *duplicateBusinessData(cJSON *pRoot)
        {
            if (pRoot == nullptr)
            {
                return cJSON_CreateObject();
            }

            cJSON *pData = cJSON_GetObjectItem(pRoot, "Data");
            if (pData != nullptr)
            {
                return cJSON_Duplicate(pData, true);
            }

            cJSON *pBusinessData = cJSON_Duplicate(pRoot, true);
            if (pBusinessData == nullptr)
            {
                return cJSON_CreateObject();
            }

            cJSON_DeleteItemFromObject(pBusinessData, "Command");
            cJSON_DeleteItemFromObject(pBusinessData, "SdkCommand");
            cJSON_DeleteItemFromObject(pBusinessData, "SdkCommandName");
            cJSON_DeleteItemFromObject(pBusinessData, "ActionCode");
            cJSON_DeleteItemFromObject(pBusinessData, "DeviceName");
            cJSON_DeleteItemFromObject(pBusinessData, "UserName");
            return pBusinessData;
        }
    } // namespace

    bool CHttpSdkGateway::isGatewayUri(const std::string &strUri)
    {
        return uriMatches(strUri, HTTP_SDK_COMMAND_URI);
    }

    bool CHttpSdkGateway::isGatewayRequest(const std::string &strMethod, const std::string &strUri)
    {
        return isGatewayUri(strUri) && (strMethod == "POST" || strMethod == "PUT");
    }

    int CHttpSdkGateway::resolveSdkCommand(const std::string &strCommand)
    {
        const std::string strNormalized = normalizeCommandName(strCommand);
        if (isDecimalNumber(strNormalized))
        {
            return std::atoi(strNormalized.c_str());
        }

        for (const auto &stItem : g_astCommandMap)
        {
            if (strNormalized == stItem.pCommandName)
            {
                return stItem.nSdkCommand;
            }
        }

        return 0;
    }

    int CHttpSdkGateway::sdkCommandToActionCode(int nSdkCommand)
    {
        for (const auto &stItem : g_astCommandMap)
        {
            if (stItem.nSdkCommand == nSdkCommand)
            {
                return stItem.nActionCode;
            }
        }

        return 0;
    }

    std::string CHttpSdkGateway::buildBackendJson(const std::string &strRequestBody, int &nActionCode)
    {
        JsonPtr pRoot(cJSON_Parse(strRequestBody.c_str()), cJSON_Delete);
        if (!pRoot)
        {
            throw std::runtime_error("HTTP-SDK 请求体不是有效 JSON");
        }

        const int nSdkCommand = extractSdkCommand(pRoot.get());
        nActionCode = sdkCommandToActionCode(nSdkCommand);
        if (nActionCode <= 0)
        {
            throw std::runtime_error("HTTP-SDK 命令未支持或缺少 Command 字段");
        }

        JsonPtr pBackendRoot(cJSON_CreateObject(), cJSON_Delete);
        if (!pBackendRoot)
        {
            throw std::runtime_error("无法创建 HTTP-SDK 后端 JSON 对象");
        }

        cJSON_AddItemToObject(pBackendRoot.get(), "ActionCode", cJSON_CreateNumber(nActionCode));
        cJSON_AddItemToObject(pBackendRoot.get(), "SdkCommand", cJSON_CreateNumber(nSdkCommand));
        cJSON_AddItemToObject(pBackendRoot.get(), "SdkCommandName", cJSON_CreateString(findCommandName(nSdkCommand)));
        cJSON_AddItemToObject(pBackendRoot.get(), "DeviceName", cJSON_CreateString(HTTP_DEVICE_NAME));
        cJSON_AddItemToObject(pBackendRoot.get(), "UserName", cJSON_CreateString(HTTP_DEFAULT_USER));

        cJSON *pData = duplicateBusinessData(pRoot.get());
        if (pData == nullptr)
        {
            throw std::runtime_error("无法创建 HTTP-SDK 业务 Data 对象");
        }
        cJSON_AddItemToObject(pBackendRoot.get(), "Data", pData);

        char *pJsonString = cJSON_PrintUnformatted(pBackendRoot.get());
        if (pJsonString == nullptr)
        {
            throw std::runtime_error("HTTP-SDK 后端 JSON 序列化失败");
        }

        std::string strResult(pJsonString);
        cJSON_free(pJsonString);
        return strResult;
    }
}
