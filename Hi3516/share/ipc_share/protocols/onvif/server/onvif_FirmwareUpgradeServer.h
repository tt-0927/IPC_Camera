/**
 * @file onvif_FirmwareUpgradeServer.h
 * @author tianl (tianl@kfb.cn)
 * @date 2025-10-15
 * 
 * @brief onvif http固件升级、遮盖配置
 */
#pragma once


#include <system_error>
#include "httplib.h"
#include "Singleton.h"
#include "onvif_type.h"
#include "dlog.h"
#include "httpAuthHandler.h"

#define ONVIF_UPGRADE_FURWARE       "/opt/course/upload/onvif_firmware.bin"
#define ONVIF_DEFAULE_UPGRADE_USER  "admin"
#define ONVIF_DEFAULE_UPGRADE_REALM  "IPC"


class COnvifFirmwareUpgradeServer : public CSingleton<COnvifFirmwareUpgradeServer>
{

public:
    COnvifFirmwareUpgradeServer(uint16_t port = ONVIF_UPGRADE_PORT,
            const std::string& firmware_temp_path = ONVIF_UPGRADE_FURWARE,
            size_t max_firmware_size = 30 * 1024 * 1024)
        : nPort(port),
          strFirmwarePath(firmware_temp_path),
          nMaxSize(max_firmware_size),
          m_running(false) 
        {
        
        }

    ~COnvifFirmwareUpgradeServer() 
    {
        stop();
    }
public:
     /**
     * @brief 启动服务器
     * @return 启动成功返回 true，失败返回 false
     */
    bool start();
   
private:
    /**
     * @brief 停止服务器
     */
    void stop();

    /**
     * @brief 判断服务器是否在运行
     */
    bool is_running() const 
    {
        return m_running;
    }

    #pragma pack(push, 1)
    typedef struct package {
        char          id[4];          // 4字节标识（跨平台无差异）
        char          version[48];    // 版本号（字符数组无差异）
        char          md5[33];        // MD5（字符数组无差异）
        int64_t       len;            // 用int64_t替代long long，确保跨平台8字节
        int32_t       reserves;       // 用int32_t替代int，确保跨平台4字节
    } Upgrade_Package_t;
    #pragma pack(pop)

   struct UpgradeCheckContext {
    bool is_struct_checked = false;
    std::vector<unsigned char> struct_buffer; 
    const size_t STRUCT_SIZE = sizeof(Upgrade_Package_t);
    const size_t MAX_SIZE = 30 * 1024 * 1024; 
};

protected:
    /**
     * @brief 处理固件上传请求
     */
    int handle_firmware_upload(const httplib::Request& req, httplib::Response& res,const httplib::ContentReader &content_reader);

protected:
    uint16_t nPort;                  // 服务端口
    std::string strFirmwarePath; // 固件临时存储路径
    size_t nMaxSize;       // 最大固件大小（字节）
    httplib::Server m_server;         // cpp-httplib 服务器实例
    std::thread m_serverThread;      // 服务器运行线程
    bool m_running;                // 服务器运行状态
    CHttpAuthHandler auth_handler_;
};

