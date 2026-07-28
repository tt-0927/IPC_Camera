/**
 * @file HttpBasicCommand.hpp
 * @author tianl (tianl@kfb.cn)
 * @date 2026-07-28
 * @LastEditors  : qinjt@kfb.cn
 * @LastEditTime : 2026-07-28
 *
 * @brief HttpBasicCommand 模块接口与类型定义
 * 功能说明：
 * 1. 声明 HttpBasicCommand 模块对外接口和数据类型
 * 2. 定义模块依赖的常量、回调或辅助类型
 * 3. 为调用方提供明确且稳定的编译期契约
 */
#pragma once
#include <string>
#include <functional>

#include "tvsdkhttplib.h"
#include "NetSdkLog.h"
#include "NetTVSDKHttpUrl.h"
#include "HttpAuthHandler.h"

using namespace tvsdk;

class CHttpBasicCommand
{
public:
    static  std::function<void(const httplib::Request&, httplib::Response&)> MakeHttpCallbackHandler(const std::function<std::string(const std::string&,const std::string&)>& Func)
    {
        return [Func](const httplib::Request& req, httplib::Response& res)
        {
            /* 鉴权 */
            if(!CHttpAuthHandler::instance()->handle_authentication(req, res))
            {
                return;
            }

            try
            {
                NETSDK_LOG_MESSAGE_DEBUG("接收到http请求: body[%s] url[%s] method[%s]",req.body.c_str(),req.target.c_str(),req.method.c_str());
                std::string resp_data = Func(req.body, req.target);
                /* 过滤命令301（通道列表）的返回数据，避免日志过长 */
                if (req.target.find("command=301") == std::string::npos)
                {
                    NETSDK_LOG_MESSAGE_DEBUG("return ---------------------------------------------------------------");
                    NETSDK_LOG_MESSAGE_DEBUG("返回http响应: resp[%s] url[%s] method[%s]",resp_data.c_str(),req.target.c_str(),req.method.c_str());
                }
                res.status = NET_TV_HTTP_RESP_CODE_SUCCESS;
                res.set_content(resp_data, NET_TV_JSON_CONTENT_TYPE);
            } catch (const std::invalid_argument& e) {
                res.status = NET_TV_HTTP_RESP_CODE_BAD_REQUEST;
                res.set_content(R"({"error":")" + std::string(e.what()) + R"("})", NET_TV_JSON_CONTENT_TYPE);
            } catch (const std::out_of_range& e) {
                res.status = NET_TV_HTTP_RESP_CODE_NOT_FOUND;
                res.set_content(R"({"error":"URL not found: )" + std::string(e.what()) + R"("})", NET_TV_JSON_CONTENT_TYPE);
            } catch (const std::exception& e) {
                res.status = NET_TV_HTTP_RESP_CODE_INTERNAL_SERVER_ERROR;
                res.set_content(R"({"error":"internal server error"})", NET_TV_JSON_CONTENT_TYPE);
            }
        };
    }
};
