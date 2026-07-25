/**
 * @file RouteRegistry.h
 * @author tianl (tianl@kfb.cn)
 * @date 2025-12-12
 * 
 * @brief 路由注册类 解耦URL与事务
 */
#pragma once
#include <string>
#include <functional>
#include <unordered_map>
#include <vector>
#include <mutex>
#include <condition_variable>
#include <thread>
#include <atomic>
#include <iostream>

#include "tvsdkhttplib.h"
#include "HttpBasicCommand.hpp"

#ifdef DELETE
#undef DELETE
#endif

using namespace tvsdk;

/* http方法枚举 */
enum class HttpMethod { GET, PUT, POST, DELETE };
using HttpHandler = std::function<void(const httplib::Request&, httplib::Response&)>;
using HttpContentReaderHandler = std::function<void(const httplib::Request&, httplib::Response&, const httplib::ContentReader&)>;
class CRouteRegistry 
{
public:
   
    struct Route 
	{
        std::string url;		
        HttpMethod method;
        HttpHandler handler;
        HttpContentReaderHandler contentReaderHandler;
        bool useContentReader;
    };

public:
    static void registerRoute(const std::string& url, HttpMethod method, HttpHandler handler) 
	{
        m_routes.emplace_back(Route{url, method, handler, HttpContentReaderHandler(), false});
    }

    static void registerRoute(const std::string& url, HttpMethod method, HttpContentReaderHandler handler)
    {
        m_routes.emplace_back(Route{url, method, HttpHandler(), handler, true});
    }
    static const std::vector<Route>& getRoutes() { return m_routes; }
     static void clearRoutes() { m_routes.clear(); }
private:
    static std::vector<Route> m_routes;
};


/**
 * @brief 注册http url的回调方法（单例类）
 */
#define REGISTER_ROUTE_URL_SINGLETON(url, method, clazz, func) \
    /* 静态注册器，程序启动时自动执行 */ \
    struct RouteRegistrar_##clazz##_##func { \
        RouteRegistrar_##clazz##_##func() { \
            /* 1. 绑定单例的成员函数 */ \
            auto bizFunc = std::bind(&clazz::func, clazz::instance(), std::placeholders::_1, std::placeholders::_2); \
            /* 2. 生成通用 HTTP Handler（复用 MakeHttpCallbackHandler */ \
            HttpHandler httpHandler = CHttpBasicCommand::MakeHttpCallbackHandler(bizFunc); \
            /* 3. 注册路由 */ \
            CRouteRegistry::registerRoute(url, method, httpHandler); \
        } \
    }; \
    /* 定义静态变量触发构造 */ \
    static RouteRegistrar_##clazz##_##func s_route_registrar_##clazz##_##func;

/**
 * @brief 注册http url的回调方法（全局/自由函数）
 */
#define REGISTER_ROUTE_URL_FUNC(url, method, func) \
    struct RouteRegistrar_##func { \
        RouteRegistrar_##func() { \
             HttpHandler httpHandler = CHttpBasicCommand::MakeHttpCallbackHandler(func); \
            CRouteRegistry::registerRoute(url, method, httpHandler); \
        } \
    }; \
    static RouteRegistrar_##func s_route_registrar_##func;
