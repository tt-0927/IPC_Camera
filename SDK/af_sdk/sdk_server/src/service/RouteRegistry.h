/**
 * @file RouteRegistry.h
 * @author tianl (tianl@kfb.cn)
 * @date 2026-07-28
 * @LastEditors  : qinjt@kfb.cn
 * @LastEditTime : 2026-07-28
 *
 * @brief RouteRegistry 模块接口与类型定义
 * 功能说明：
 * 1. 声明 RouteRegistry 模块对外接口和数据类型
 * 2. 定义模块依赖的常量、回调或辅助类型
 * 3. 为调用方提供明确且稳定的编译期契约
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
enum class HttpMethod_E { GET, PUT, POST, DELETE };
using HttpHandler = std::function<void(const httplib::Request&, httplib::Response&)>;
using HttpContentReaderHandler = std::function<void(const httplib::Request&, httplib::Response&, const httplib::ContentReader&)>;
class CRouteRegistry
{
public:

    struct Route_S
	{
        std::string url;
        HttpMethod_E method;
        HttpHandler handler;
        HttpContentReaderHandler contentReaderHandler;
        bool useContentReader;
    };

public:
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 registerRoute 定义的内联处理。
 * @param [in] url 函数处理参数。
 * @param [in] method 函数处理参数。
 * @param [in] handler 函数处理参数。
 * @return 无返回值。
 */
    static void registerRoute(const std::string& url, HttpMethod_E method, HttpHandler handler)
	{
        s_aRoutes.emplace_back(Route_S{url, method, handler, HttpContentReaderHandler(), false});
    }
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 registerRoute 定义的内联处理。
 * @param [in] url 函数处理参数。
 * @param [in] method 函数处理参数。
 * @param [in] handler 函数处理参数。
 * @return 无返回值。
 */

    static void registerRoute(const std::string& url, HttpMethod_E method, HttpContentReaderHandler handler)
    {
        s_aRoutes.emplace_back(Route_S{url, method, HttpHandler(), handler, true});
    }
    static const std::vector<Route_S>& getRoutes() { return s_aRoutes; }
     static void clearRoutes() { s_aRoutes.clear(); }
private:
    static std::vector<Route_S> s_aRoutes;
};


/**
 * @author tianl (tianl@kfb.cn)
 * @brief 注册http url的回调方法（单例类）
 */
#define NETSDK_REGISTER_ROUTE_URL_SINGLETON(url, method, clazz, func) \
    /* 静态注册器，程序启动时自动执行 */ \
    struct CRouteRegistrar_##clazz##_##func { \
        CRouteRegistrar_##clazz##_##func() { \
            /* 1. 绑定单例的成员函数 */ \
            auto bizFunc = std::bind(&clazz::func, clazz::instance(), std::placeholders::_1, std::placeholders::_2); \
            /* 2. 生成通用 HTTP Handler（复用 MakeHttpCallbackHandler */ \
            HttpHandler httpHandler = CHttpBasicCommand::MakeHttpCallbackHandler(bizFunc); \
            /* 3. 注册路由 */ \
            CRouteRegistry::registerRoute(url, method, httpHandler); \
        } \
    }; \
    /* 定义静态变量触发构造 */ \
    static CRouteRegistrar_##clazz##_##func s_route_registrar_##clazz##_##func;

/**
 * @author tianl (tianl@kfb.cn)
 * @brief 注册http url的回调方法（全局/自由函数）
 */
#define NETSDK_REGISTER_ROUTE_URL_FUNC(url, method, func) \
    struct CRouteRegistrar_##func { \
        CRouteRegistrar_##func() { \
             HttpHandler httpHandler = CHttpBasicCommand::MakeHttpCallbackHandler(func); \
            CRouteRegistry::registerRoute(url, method, httpHandler); \
        } \
    }; \
    static CRouteRegistrar_##func s_route_registrar_##func;
