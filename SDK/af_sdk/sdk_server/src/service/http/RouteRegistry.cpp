/**
 * @file RouteRegistry.cpp
 * @author tianl (tianl@kfb.cn)
 * @date 2026-07-28
 * @LastEditors  : qinjt@kfb.cn
 * @LastEditTime : 2026-07-28
 *
 * @brief RouteRegistry 模块实现
 * 功能说明：
 * 1. 实现 RouteRegistry 模块核心逻辑
 * 2. 校验输入参数并管理模块资源生命周期
 * 3. 向上层提供可复用的 SDK 能力
 */


#include "RouteRegistry.h"

/* 定义 CRouteRegistry 的静态成员 */
std::vector<CRouteRegistry::Route_S> CRouteRegistry::s_aRoutes;

