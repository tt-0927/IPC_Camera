/**
 * @FilePath     : onvif_firmware_upgrade_server.h
 * @Author       : tianl@kfb.cn
 * @Date         : 2025-10-15 11:12:16
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-06-04 14:08:15
 * @Description  : onvif http固件升级、遮盖配置
 */

#pragma once

#include <system_error>
#include "httplib.h"
#include "Singleton.h"
#include "onvif_type.h"
#include "dlog.h"
#include "IpcRet.h"
#include "httpAuthHandler.h"

#define ONVIF_UPGRADE_FURWARE       "/opt/course/upload/onvif_firmware.bin"
#define ONVIF_DEFAULE_UPGRADE_USER  "admin"
#define ONVIF_DEFAULE_UPGRADE_REALM "IPC"

class COnvifFirmwareUpgradeServer : public CSingleton<COnvifFirmwareUpgradeServer>
{
public:
    COnvifFirmwareUpgradeServer(uint16_t port = ONVIF_UPGRADE_PORT,
                                const std::string &firmware_temp_path = ONVIF_UPGRADE_FURWARE,
                                size_t max_firmware_size = 30 * 1024 * 1024)
        : nPort(port), strFirmwarePath(firmware_temp_path), nMaxSize(max_firmware_size), m_running(false)
    {
    }

    ~COnvifFirmwareUpgradeServer()
    {
        deinit();
    }

public:
    /**
     * @brief   : 初始化并启动 ONVIF 固件升级 HTTP 服务
     * @return   {int} 0：成功，非0：失败
     */
    int init();

    /**
     * @brief   : 反初始化，停止 HTTP 服务并释放线程资源
     * @return   {int} 0：成功，非0：失败
     */
    int deinit();

private:
    /**
     * @brief   : 判断服务器是否在运行
     */
    bool is_running() const
    {
        return m_running;
    }

#pragma pack(push, 1)
    typedef struct package
    {
        char id[4];       /* 4字节标识（跨平台无差异） */
        char version[48]; /* 版本号（字符数组无差异） */
        char md5[33];     /* MD5（字符数组无差异） */
        int64_t len;      /* 用int64_t确保跨平台8字节 */
        int32_t reserves; /* 用int32_t确保跨平台4字节 */
    } Upgrade_Package_t;
#pragma pack(pop)

    struct UpgradeCheckContext
    {
        bool is_struct_checked = false;
        std::vector<unsigned char> struct_buffer;
        const size_t STRUCT_SIZE = sizeof(Upgrade_Package_t);
        const size_t MAX_SIZE = 30 * 1024 * 1024;
    };

protected:
    /**
     * @brief   : 处理固件上传请求
     */
    int handle_firmware_upload(const httplib::Request &req, httplib::Response &res, const httplib::ContentReader &content_reader);

protected:
    /* 服务端口 */
    uint16_t nPort;
    /* 固件临时存储路径 */
    std::string strFirmwarePath;
    /* 最大固件大小（字节） */
    size_t nMaxSize;
    /* cpp-httplib 服务器实例 */
    httplib::Server m_server;
    /* 服务器运行线程 */
    std::thread m_serverThread;
    /* 服务器运行状态 */
    bool m_running;
    /* HTTP 认证处理器 */
    CHttpAuthHandler auth_handler_;
};
