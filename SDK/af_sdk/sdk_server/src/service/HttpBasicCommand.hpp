/**
 * @file HttpBasicCommand.hpp
 * @author tianl (tianl@kfb.cn)
 * @date 2025-12-04
 * 
 * @brief HTTP命令基类
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
                NSDK_LOG_DEBUG("接收到http请求: body[%s] url[%s] method[%s]",req.body.c_str(),req.target.c_str(),req.method.c_str());
                std::string resp_data = Func(req.body, req.target);
                 NSDK_LOG_DEBUG("return ---------------------------------------------------------------");
                NSDK_LOG_DEBUG("返回http响应: resp[%s] url[%s] method[%s]",resp_data.c_str(),req.target.c_str(),req.method.c_str());
                res.status = HTTP_RESP_CODE_SUCCESS;
                res.set_content(resp_data, JSON_CONTENT_TYPE);
            } catch (const std::invalid_argument& e) {
                res.status = HTTP_RESP_CODE_BAD_REQUEST;
                res.set_content(R"({"error":")" + std::string(e.what()) + R"("})", JSON_CONTENT_TYPE);
            } catch (const std::out_of_range& e) { 
                res.status = HTTP_RESP_CODE_NOT_FOUND;
                res.set_content(R"({"error":"URL not found: )" + std::string(e.what()) + R"("})", JSON_CONTENT_TYPE);
            } catch (const std::exception& e) {
                res.status = HTTP_RESP_CODE_INTERNAL_SERVER_ERROR;
                res.set_content(R"({"error":"internal server error"})", JSON_CONTENT_TYPE);
            }
        };
    }
};
