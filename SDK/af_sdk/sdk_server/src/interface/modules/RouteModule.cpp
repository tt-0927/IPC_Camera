#include "RouteModule.h"
#include "RouteRegistry.h"
#include "NetTVSDKHttpUrl.h"
#include "DeviceBusiness.h"
#include "DeviceCapabilityBusiness.h"
#include "DeviceConfigBusiness.h"
#include "PlaybackBusiness.h"
#include "NetSdkLog.h"

RouteModule::RouteModule()
    : m_routeCount(0)
{
    NSDK_LOG_DEBUG("RouteModule created");
}

RouteModule::~RouteModule()
{
    ClearRoutes();
    NSDK_LOG_DEBUG("RouteModule destroyed");
}

BOOL RouteModule::RegisterAllRoutes()
{
    NSDK_LOG_INFO("Registering all HTTP routes...");

    RegisterDeviceRoutes();
    RegisterCapabilityRoutes();
    RegisterConfigRoutes();
    RegisterVideoRoutes();

    NSDK_LOG_INFO("Successfully registered %zu routes", m_routeCount);
    return TRUE;
}

void RouteModule::RegisterDeviceRoutes()
{
    NSDK_LOG_DEBUG("Registering device routes...");

    // 注册设备信息路由
    REGISTER_ROUTE_URL_SINGLETON(TVAPI_PATH_DEVICE_GETINFO, 
                                 HttpMethod::GET, 
                                 CDeviceBusiness, 
                                 GetDeviceInfo);
    m_routeCount++;

    NSDK_LOG_DEBUG("Device routes registered");
}

void RouteModule::RegisterCapabilityRoutes()
{
    NSDK_LOG_DEBUG("Registering capability routes...");

    // 注册设备能力集路由
    REGISTER_ROUTE_URL_SINGLETON(TVAPI_PATH_DEVICE_CAPABILITY, 
                                 HttpMethod::GET, 
                                 CDeviceCapabilityBusiness, 
                                 GetDeviceCapability);
    m_routeCount++;

    NSDK_LOG_DEBUG("Capability routes registered");
}

void RouteModule::RegisterConfigRoutes()
{
    NSDK_LOG_DEBUG("Registering config routes...");

    REGISTER_ROUTE_URL_SINGLETON(TVAPI_PATH_DEVICE_GET_DEV_CONFIG,
                                 HttpMethod::GET,
                                 CDeviceConfigBusiness,
                                 GetDevConfig);
    m_routeCount++;

    REGISTER_ROUTE_URL_SINGLETON(TVAPI_PATH_DEVICE_SET_DEV_CONFIG,
                                 HttpMethod::POST,
                                 CDeviceConfigBusiness,
                                 SetDevConfig);
    m_routeCount++;

    NSDK_LOG_DEBUG("Config routes registered");
}

void RouteModule::RegisterVideoRoutes()
{
    NSDK_LOG_DEBUG("Registering video routes...");

    REGISTER_ROUTE_URL_SINGLETON(TVAPI_PATH_REPLAY_GET_URL,
                                 HttpMethod::POST,
                                 CPlaybackBusiness,
                                 GetReplayUrl);
    m_routeCount++;

    REGISTER_ROUTE_URL_SINGLETON(TVAPI_PATH_REPLAY_CONTROL,
                                 HttpMethod::POST,
                                 CPlaybackBusiness,
                                 ControlReplay);
    m_routeCount++;

    REGISTER_ROUTE_URL_SINGLETON(TVAPI_PATH_REPLAY_GET_RECORD_LIST,
                                 HttpMethod::POST,
                                 CPlaybackBusiness,
                                 GetReplayRecordList);
    m_routeCount++;

    NSDK_LOG_DEBUG("Video routes registered");
}

void RouteModule::ClearRoutes()
{
    NSDK_LOG_INFO("Clearing all routes and business singletons...");

    // 清理路由注册表
    CRouteRegistry::clearRoutes();
    NSDK_LOG_DEBUG("Route registry cleared");

    // 销毁业务单例
    CDeviceBusiness::DestroyInstance();
    NSDK_LOG_DEBUG("DeviceBusiness destroyed");

    CDeviceCapabilityBusiness::DestroyInstance();
    NSDK_LOG_DEBUG("DeviceCapabilityBusiness destroyed");

    CDeviceConfigBusiness::DestroyInstance();
    NSDK_LOG_DEBUG("DeviceConfigBusiness destroyed");

    CPlaybackBusiness::DestroyInstance();
    NSDK_LOG_DEBUG("PlaybackBusiness destroyed");

    m_routeCount = 0;
    NSDK_LOG_INFO("Routes and business singletons cleared");
}

size_t RouteModule::GetRouteCount() const
{
    return m_routeCount;
}
