/*
 * @Author       : chenchl
 * @Date         : 2025-01-02 16:01:20
 * @LastEditors  : chenchl
 * @LastEditTime : 2025-01-02 17:03:03
 * @FilePath     : CRouteModule.cpp
 * @Description  : 路由注册管理模块实现，负责HTTP路由的注册和业务单例的生命周期管理
 */

#include "RouteModule.h"
#ifdef DELETE
#undef DELETE
#endif
#include "RouteRegistry.h"
#include "NetTVSDKHttpUrl.h"
#include "BG6_ZHSJ/BU_SJCL/NvrBusiness.h"
#include "DeviceCapabilityBusiness.h"
#include "DeviceConfigBusiness.h"
#include "DeviceControlBusiness.h"
#include "DeviceBusiness.h"
#include "PlaybackBusiness.h"
#include "RecordFrameBusiness.h"
#include "HttpAuthHandler.h"
#include "NetSdkLog.h"
#include "SDKConvert.h"
#include <cerrno>
#include <cstring>
#include <exception>
#include <cstdio>
#include <fstream>
#include <string>
#ifdef _WIN32
#include <direct.h>
#else
#include <sys/stat.h>
#endif

namespace
{
/* 上传目录路径 */
constexpr const char* kUploadDir = "/opt/course/upload/";
/* 最后升级文件标记路径 */
constexpr const char* kLastUpgradeFile = "/opt/course/upload/.tvsdk_last_upgrade";
/* 上传进度日志记录间隔（8MB） */
constexpr size_t kUploadProgressLogStep = 8 * 1024 * 1024;

/**
 * 判断字符是否为安全的上传文件名字符
 * @param ch 待检查字符
 * @return true表示安全字符，false表示不安全字符
 */
bool IsSafeUploadFileNameChar(char ch)
{
    return (ch >= '0' && ch <= '9') ||
           (ch >= 'A' && ch <= 'Z') ||
           (ch >= 'a' && ch <= 'z') ||
           ch == '.' || ch == '_' || ch == '-';
}

/**
 * 验证上传文件名是否合法
 * @param filename 文件名
 * @return true表示合法，false表示非法
 */
bool IsValidUploadFileName(const std::string& filename)
{
    if (filename.empty() || filename.size() >= NET_FILE_NAME_LEN) {
        return false;
    }

    if (filename[0] == '.' ||
        filename == "." || filename == ".." ||
        filename.find('/') != std::string::npos ||
        filename.find('\\') != std::string::npos ||
        filename.find("..") != std::string::npos) {
        return false;
    }

    for (char ch : filename) {
        if (!IsSafeUploadFileNameChar(ch)) {
            return false;
        }
    }

    return true;
}

/**
 * 检查上传路径是否包含无效字符
 * @param value 路径字符串
 * @return true表示包含无效字符，false表示不包含
 */
bool HasInvalidUploadPathChar(const std::string& value)
{
    for (char ch : value) {
        if (ch == '\r' || ch == '\n') {
            return true;
        }
    }
    return false;
}

/**
 * 判断路径是否在上传目录下
 * @param path 待检查路径
 * @return true表示在上传目录下，false表示不在
 */
bool IsPathUnderUploadDir(const std::string& path)
{
    const std::string uploadDir(kUploadDir);
    return path.compare(0, uploadDir.size(), uploadDir) == 0 &&
           path.size() > uploadDir.size();
}

/**
 * 规范化上传文件名参数
 * @param value 原始文件名参数
 * @return 规范化后的文件名，非法则返回空字符串
 */
std::string NormalizeUploadFileNameParam(const std::string& value)
{
    if (value.empty() ||
        value.find("://") != std::string::npos ||
        value.find('\\') != std::string::npos ||
        value.find("..") != std::string::npos ||
        HasInvalidUploadPathChar(value)) {
        return "";
    }

    if (IsPathUnderUploadDir(value)) {
        const std::string uploadDir(kUploadDir);
        const std::string filename = value.substr(uploadDir.size());
        return IsValidUploadFileName(filename) ? filename : "";
    }

    return IsValidUploadFileName(value) ? value : "";
}

/**
 * 确保目录存在，不存在则创建
 * @param path 目录路径
 * @return true表示目录已存在或创建成功，false表示创建失败
 */
bool EnsureDir(const char* path)
{
#ifdef _WIN32
    if (_mkdir(path) == 0) {
#else
    if (mkdir(path, 0755) == 0) {
#endif
        return true;
    }
    return errno == EEXIST;
}

/**
 * 排空未授权上传请求的body数据
 * @param contentReader HTTP内容读取器
 * @param client 客户端地址
 * @param filename 文件名
 * @return true表示排空成功，false表示失败
 */
bool DrainUploadBody(const httplib::ContentReader& contentReader,
                     const std::string& client,
                     const std::string& filename)
{
    size_t drainedSize = 0;
    size_t nextLogSize = kUploadProgressLogStep;
    const bool readOk = contentReader([&drainedSize, &nextLogSize, &client, &filename](const char* data, size_t dataLength) -> bool {
        if (!data || dataLength == 0) {
            return true;
        }
        drainedSize += dataLength;
        if (drainedSize >= nextLogSize) {
            NETSDK_LOG_MESSAGE_INFO("[TVSDK][Upload] draining unauth body: client=%s, filename=%s, drained=%zu",
                          client.c_str(), filename.c_str(), drainedSize);
            nextLogSize += kUploadProgressLogStep;
        }
        return true;
    });

    if (!readOk) {
        NETSDK_LOG_MESSAGE_WARN("[TVSDK][Upload] drain unauth body failed: client=%s, filename=%s, drained=%zu",
                      client.c_str(), filename.c_str(), drainedSize);
        return false;
    }

    NETSDK_LOG_MESSAGE_INFO("[TVSDK][Upload] drained unauth body before challenge: client=%s, filename=%s, drained=%zu",
                  client.c_str(), filename.c_str(), drainedSize);
    return true;
}

/**
 * 设置SDK标准JSON响应
 * @param res HTTP响应对象
 * @param code SDK错误码
 */
void SetSdkJsonResponse(httplib::Response& res, NET_COMMON_ECODE_E code)
{
    res.status = NET_HTTP_RESP_CODE_SUCCESS;
    res.set_content(SDKConvert::to_respString(code), NET_JSON_CONTENT_TYPE);
}

/**
 * 更新最后升级文件标记
 * @param filePath 升级文件路径
 * @return true表示更新成功，false表示失败
 */
bool UpdateLastUpgradeFile(const std::string& filePath)
{
    std::ofstream marker(kLastUpgradeFile, std::ios::binary | std::ios::trunc);
    if (marker.is_open()) {
        marker << filePath;
        marker.flush();
        return marker.good();
    }
    return false;
}
}

/**
 * 构造函数
 */
CRouteModule::CRouteModule()
    : m_nRouteCount(0)
{
    NETSDK_LOG_MESSAGE_DEBUG("CRouteModule created");
}

/**
 * 析构函数
 * @details 自动调用ClearRoutes()清理路由和业务单例
 */
CRouteModule::~CRouteModule()
{
    ClearRoutes();
    NETSDK_LOG_MESSAGE_DEBUG("CRouteModule destroyed");
}

/**
 * 注册所有业务路由
 * @details 依次注册设备、能力集、配置、设备控制、视频、升级相关路由
 * @return TRUE表示成功，FALSE表示失败
 */
BOOL CRouteModule::RegisterAllRoutes()
{
    NETSDK_LOG_MESSAGE_INFO("Registering all HTTP routes...");

    RegisterDeviceRoutes();
    RegisterCapabilityRoutes();
    RegisterConfigRoutes();
    RegisterDeviceControlRoutes();
    RegisterVideoRoutes();
    RegisterUpgradeRoutes();

    NETSDK_LOG_MESSAGE_INFO("Successfully registered %zu routes", m_nRouteCount);
    return TRUE;
}

/**
 * 注册设备相关路由
 * @details 注册设备信息获取路由
 */
void CRouteModule::RegisterDeviceRoutes()
{
    NETSDK_LOG_MESSAGE_DEBUG("Registering device routes...");

    // 注册设备信息路由（NVR规模信息：设备类型/报警端口数/通道数，归NVR侧）
    NETSDK_REGISTER_ROUTE_URL_SINGLETON_WITH_SESSION(NET_API_PATH_DEVICE_GETINFO,
                                 HttpMethod_E::GET,
                                 CNvrBusiness,
                                 GetDeviceInfo);
    m_nRouteCount++;

    NETSDK_LOG_MESSAGE_DEBUG("Device routes registered");
}

/**
 * 注册能力集相关路由
 * @details 注册设备能力集获取路由
 */
void CRouteModule::RegisterCapabilityRoutes()
{
    NETSDK_LOG_MESSAGE_DEBUG("Registering capability routes...");

    // 注册设备能力集路由
    NETSDK_REGISTER_ROUTE_URL_SINGLETON_WITH_SESSION(NET_API_PATH_DEVICE_CAPABILITY,
                                 HttpMethod_E::GET,
                                 CDeviceCapabilityBusiness,
                                 GetDeviceCapability);
    m_nRouteCount++;

    NETSDK_LOG_MESSAGE_DEBUG("Capability routes registered");
}

/**
 * 注册配置相关路由
 * @details 注册设备配置获取和设置路由
 */
void CRouteModule::RegisterConfigRoutes()
{
    NETSDK_LOG_MESSAGE_DEBUG("Registering config routes...");

    NETSDK_REGISTER_ROUTE_URL_SINGLETON_WITH_SESSION(NET_API_PATH_DEVICE_GET_DEV_CONFIG,
                                 HttpMethod_E::GET,
                                 CDeviceConfigBusiness,
                                 GetDevConfig);
    m_nRouteCount++;

    NETSDK_REGISTER_ROUTE_URL_SINGLETON_WITH_SESSION(NET_API_PATH_DEVICE_SET_DEV_CONFIG,
                                 HttpMethod_E::POST,
                                 CDeviceConfigBusiness,
                                 SetDevConfig);
    m_nRouteCount++;

    NETSDK_LOG_MESSAGE_DEBUG("Config routes registered");
}

/**
 * 注册设备控制相关路由
 * @details 注册设备控制路由
 */
void CRouteModule::RegisterDeviceControlRoutes()
{
    NETSDK_LOG_MESSAGE_DEBUG("Registering device control routes...");

    NETSDK_REGISTER_ROUTE_URL_SINGLETON_WITH_SESSION(NET_API_PATH_DEVICE_CONTROL,
                                 HttpMethod_E::POST,
                                 CDeviceControlBusiness,
                                 DeviceControl);
    m_nRouteCount++;

    NETSDK_LOG_MESSAGE_DEBUG("Device control routes registered");
}

/**
 * 注册视频相关路由
 * @details 注册录像回放URL获取、回放控制、录像列表获取、录像帧流启动和停止路由
 */
void CRouteModule::RegisterVideoRoutes()
{
    NETSDK_LOG_MESSAGE_DEBUG("Registering video routes...");

    NETSDK_REGISTER_ROUTE_URL_SINGLETON(NET_API_PATH_REPLAY_GET_URL,
                                 HttpMethod_E::POST,
                                 CPlaybackBusiness,
                                 GetReplayUrl);
    m_nRouteCount++;

    NETSDK_REGISTER_ROUTE_URL_SINGLETON(NET_API_PATH_REPLAY_CONTROL,
                                 HttpMethod_E::POST,
                                 CPlaybackBusiness,
                                 ControlReplay);
    m_nRouteCount++;

    NETSDK_REGISTER_ROUTE_URL_SINGLETON(NET_API_PATH_REPLAY_GET_RECORD_LIST,
                                 HttpMethod_E::POST,
                                 CPlaybackBusiness,
                                 GetReplayRecordList);
    m_nRouteCount++;

    NETSDK_REGISTER_ROUTE_URL_SINGLETON(NET_API_PATH_RECORD_FRAME_STREAM_START,
                                 HttpMethod_E::POST,
                                 CRecordFrameBusiness,
                                 StartRecordFrameStream);
    m_nRouteCount++;

    NETSDK_REGISTER_ROUTE_URL_SINGLETON(NET_API_PATH_RECORD_FRAME_STREAM_STOP,
                                 HttpMethod_E::POST,
                                 CRecordFrameBusiness,
                                 StopRecordFrameStream);
    m_nRouteCount++;

    NETSDK_LOG_MESSAGE_DEBUG("Video routes registered");
}

/**
 * 注册升级相关路由
 * @details 注册固件上传路由（PUT方法，直接处理二进制数据）
 */
void CRouteModule::RegisterUpgradeRoutes()
{
    NETSDK_LOG_MESSAGE_DEBUG("Registering upgrade routes...");

    // 固件上传端点: PUT /TVAPI/V1.0/Upgrade/Upload?filename=xxx
    // 直接注册原始handler, 不走MakeHttpCallbackHandler(因为body是二进制非JSON)
    CRouteRegistry::registerRoute(
        NET_API_PATH_UPGRADE_UPLOAD,
        HttpMethod_E::PUT,
        [](const httplib::Request& req, httplib::Response& res, const httplib::ContentReader& contentReader) {
            const std::string contentLength = req.has_header("Content-Length") ?
                req.get_header_value("Content-Length") : "";
            const std::string rawFilename = req.has_param(NET_API_PARAM_FILENAME) ?
                req.get_param_value(NET_API_PARAM_FILENAME) : "";
            NETSDK_LOG_MESSAGE_INFO("[TVSDK][Upload] request: client=%s, path=%s, filename=%s, content_length=%s",
                          req.remote_addr.c_str(), req.path.c_str(), rawFilename.c_str(),
                          contentLength.c_str());

            /* 鉴权 */
            if (!CHttpAuthHandler::instance()->handle_authentication(req, res)) {
                // ContentReader 路由进入 handler 时 body 尚未读取；Digest 首包返回 401 前要排空 body，避免上传连接异常断开。
                DrainUploadBody(contentReader, req.remote_addr, rawFilename);
                return;
            }

            try {
                if (!req.has_param(NET_API_PARAM_FILENAME)) {
                    NETSDK_LOG_MESSAGE_WARN("[TVSDK][Upload] missing filename: client=%s", req.remote_addr.c_str());
                    SetSdkJsonResponse(res, NET_E_INVALID_PARAM);
                    return;
                }

                // 从query参数获取文件名；允许纯文件名或 /opt/course/upload/<filename>
                const std::string filename = NormalizeUploadFileNameParam(rawFilename);
                if (filename.empty()) {
                    NETSDK_LOG_MESSAGE_WARN("[TVSDK][Upload] invalid filename: client=%s, filename=%s",
                                  req.remote_addr.c_str(), rawFilename.c_str());
                    SetSdkJsonResponse(res, NET_E_INVALID_PARAM);
                    return;
                }

                // 确保目录存在
                errno = 0;
                if (!EnsureDir("/opt/course/")) {
                    const int err = errno;
                    NETSDK_LOG_MESSAGE_ERROR("[TVSDK][Upload] ensure dir failed: path=/opt/course/, errno=%d(%s)",
                                   err, std::strerror(err));
                    SetSdkJsonResponse(res, NET_E_SYSCALL_FALIED);
                    return;
                }
                errno = 0;
                if (!EnsureDir(kUploadDir)) {
                    const int err = errno;
                    NETSDK_LOG_MESSAGE_ERROR("[TVSDK][Upload] ensure dir failed: path=%s, errno=%d(%s)",
                                   kUploadDir, err, std::strerror(err));
                    SetSdkJsonResponse(res, NET_E_SYSCALL_FALIED);
                    return;
                }

                // 流式写入固件临时文件，完整收完后再切到正式文件，避免断传半包被误用
                const std::string filePath = std::string(kUploadDir) + filename;
                const std::string tempPath = filePath + ".part";
                errno = 0;
                std::ofstream outFile(tempPath, std::ios::binary | std::ios::trunc);
                if (!outFile.is_open()) {
                    const int err = errno;
                    NETSDK_LOG_MESSAGE_ERROR("[TVSDK][Upload] open temp file failed: path=%s, errno=%d(%s)",
                                   tempPath.c_str(), err, std::strerror(err));
                    SetSdkJsonResponse(res, NET_E_FILE_NO_EXIST);
                    return;
                }

                NETSDK_LOG_MESSAGE_INFO("[TVSDK][Upload] receiving body: temp=%s, final=%s",
                              tempPath.c_str(), filePath.c_str());
                size_t writtenSize = 0;
                size_t nextLogSize = kUploadProgressLogStep;
                errno = 0;
                const bool readOk = contentReader([&outFile, &writtenSize, &nextLogSize, &tempPath](const char* data, size_t dataLength) -> bool {
                    if (!data || dataLength == 0) {
                        return true;
                    }
                    outFile.write(data, static_cast<std::streamsize>(dataLength));
                    if (!outFile) {
                        NETSDK_LOG_MESSAGE_ERROR("[TVSDK][Upload] write chunk failed: temp=%s, written=%zu, chunk=%zu",
                                       tempPath.c_str(), writtenSize, dataLength);
                        return false;
                    }
                    writtenSize += dataLength;
                    if (writtenSize >= nextLogSize) {
                        NETSDK_LOG_MESSAGE_INFO("[TVSDK][Upload] receiving body: temp=%s, written=%zu",
                                      tempPath.c_str(), writtenSize);
                        nextLogSize += kUploadProgressLogStep;
                    }
                    return true;
                });
                outFile.close();

                if (!readOk || !outFile) {
                    const int err = errno;
                    NETSDK_LOG_MESSAGE_ERROR("[TVSDK][Upload] body read/write failed: temp=%s, read_ok=%d, written=%zu, fail=%d, bad=%d, errno=%d(%s)",
                                   tempPath.c_str(), readOk ? 1 : 0, writtenSize,
                                   outFile.fail() ? 1 : 0, outFile.bad() ? 1 : 0,
                                   err, std::strerror(err));
                    std::remove(tempPath.c_str());
                    SetSdkJsonResponse(res, NET_E_SYSCALL_FALIED);
                    return;
                }

                if (writtenSize == 0) {
                    NETSDK_LOG_MESSAGE_WARN("[TVSDK][Upload] empty body: temp=%s", tempPath.c_str());
                    std::remove(tempPath.c_str());
                    SetSdkJsonResponse(res, NET_E_INVALID_PARAM);
                    return;
                }

                std::remove(filePath.c_str());
                errno = 0;
                if (std::rename(tempPath.c_str(), filePath.c_str()) != 0) {
                    const int err = errno;
                    NETSDK_LOG_MESSAGE_ERROR("[TVSDK][Upload] rename failed: temp=%s, final=%s, errno=%d(%s)",
                                   tempPath.c_str(), filePath.c_str(), err, std::strerror(err));
                    std::remove(tempPath.c_str());
                    SetSdkJsonResponse(res, NET_E_SYSCALL_FALIED);
                    return;
                }

                NETSDK_LOG_MESSAGE_INFO("Firmware uploaded: %s (%zu bytes)", filePath.c_str(), writtenSize);
                if (!UpdateLastUpgradeFile(filePath)) {
                    NETSDK_LOG_MESSAGE_ERROR("[TVSDK][Upload] update last upgrade marker failed: marker=%s, file=%s",
                                   kLastUpgradeFile, filePath.c_str());
                    SetSdkJsonResponse(res, NET_E_SYSCALL_FALIED);
                    return;
                }

                SetSdkJsonResponse(res, NET_E_SUCCEED);
            } catch (const std::exception& e) {
                NETSDK_LOG_MESSAGE_ERROR("[TVSDK][Upload] exception: %s", e.what());
                SetSdkJsonResponse(res, NET_E_FAILED);
            }
        });
    m_nRouteCount++;

    NETSDK_LOG_MESSAGE_DEBUG("Upgrade routes registered");
}

/**
 * 清理所有路由和业务单例
 * @details 清除路由注册表，销毁所有业务单例实例
 */
void CRouteModule::ClearRoutes()
{
    NETSDK_LOG_MESSAGE_INFO("Clearing all routes and business singletons...");

    // 清理路由注册表
    CRouteRegistry::clearRoutes();
    NETSDK_LOG_MESSAGE_DEBUG("Route registry cleared");

    // 销毁业务单例
    CNvrBusiness::DestroyInstance();
    NETSDK_LOG_MESSAGE_DEBUG("NvrBusiness destroyed");

    CDeviceCapabilityBusiness::DestroyInstance();
    NETSDK_LOG_MESSAGE_DEBUG("DeviceCapabilityBusiness destroyed");

    CDeviceConfigBusiness::DestroyInstance();
    NETSDK_LOG_MESSAGE_DEBUG("DeviceConfigBusiness destroyed");

    CDeviceControlBusiness::DestroyInstance();
    NETSDK_LOG_MESSAGE_DEBUG("DeviceControlBusiness destroyed");

    CDeviceBusiness::DestroyInstance();
    NETSDK_LOG_MESSAGE_DEBUG("DeviceBusiness destroyed");

    CPlaybackBusiness::DestroyInstance();
    NETSDK_LOG_MESSAGE_DEBUG("PlaybackBusiness destroyed");

    CRecordFrameBusiness::DestroyInstance();
    NETSDK_LOG_MESSAGE_DEBUG("RecordFrameBusiness destroyed");

    m_nRouteCount = 0;
    NETSDK_LOG_MESSAGE_INFO("Routes and business singletons cleared");
}

/**
 * 获取已注册的路由数量
 * @return 路由数量
 */
size_t CRouteModule::GetRouteCount() const
{
    return m_nRouteCount;
}
