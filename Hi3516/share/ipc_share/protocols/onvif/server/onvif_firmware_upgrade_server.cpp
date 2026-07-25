/**
 * @FilePath     : onvif_firmware_upgrade_server.cpp
 * @Author       : tianl@kfb.cn
 * @Date         : 2025-10-15 11:12:16
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-06-04 14:08:23
 * @Description  : onvif http固件升级、遮盖配置
 */

#include "onvif_firmware_upgrade_server.h"
#include "upgrade_client.h"
#include "action_code.h"
#include "system_convert.h"
#include "convert_interface.h"
#include "share_define.h"
#include "user_manage.h"
#include "osd_manage.h"
#include "osd_convert.h"

int COnvifFirmwareUpgradeServer::init()
{
    if (m_running)
    {
        dlog_warn("ONVIF固件升级服务已在运行，忽略重复初始化");
        return OK;
    }

    /* 限制最大固件大小 */
    m_server.set_payload_max_length(nMaxSize);
    /* 关闭长连接，避免客户端超时 */
    m_server.set_keep_alive_max_count(1);

    /* 注册 POST 处理回调（处理固件上传） */
    m_server.Post("/StartFirmwareUpgrade",
                  [this](const httplib::Request &req, httplib::Response &res, const httplib::ContentReader &content_reader)
                  {
                      auth_handler_.set_auth_info(ONVIF_DEFAULE_UPGRADE_REALM,
                                                  ONVIF_DEFAULE_UPGRADE_USER,
                                                  CUserManage::instance()->get_passwd(ONVIF_DEFAULE_UPGRADE_USER));
                      if (handle_firmware_upload(req, res, content_reader) == 0)
                      {
                          dlog_info("固件校验上传完成，开始升级");
                          System::UpgradeInfo_S stUpgradeInfo;
                          stUpgradeInfo.strUpgradePath = ONVIF_UPGRADE_FURWARE;
                          dlog_info("升级路径: %s", stUpgradeInfo.strUpgradePath.c_str());
                          std::string data = Convert::to_string(stUpgradeInfo);
                          CUpgradeClient::instance()->fill_head(data, AC_DO_UPGRADE);
                          CUpgradeClient::instance()->send(data, AC_DO_UPGRADE);
                      }
                  });

    /* 注册 GET 处理回调（获取遮盖配置） */
    m_server.Get("/CoverConfig",
                 [this](const httplib::Request &req, httplib::Response &res)
                 {
                     auth_handler_.set_auth_info(ONVIF_DEFAULE_UPGRADE_REALM,
                                                 ONVIF_DEFAULE_UPGRADE_USER,
                                                 CUserManage::instance()->get_passwd(ONVIF_DEFAULE_UPGRADE_USER));
                     if (!auth_handler_.handle_authentication(req, res))
                     {
                         dlog_warn("获取遮盖配置认证失败，客户端IP: %s", req.remote_addr.c_str());
                         return;
                     }

                     dlog_debug("收到获取遮盖配置请求");
                     Osd::CoverConfig_S stCoverConfig;
                     if (COsdManage::instance()->get_cover_config(stCoverConfig) != 0)
                     {
                         res.status = 500;
                         res.set_content("Failed to get cover config", "text/plain");
                         return;
                     }
                     std::string jsonStr = Convert::to_string(stCoverConfig);
                     res.set_content(jsonStr, "application/json");
                 });

    /* 注册 PUT 处理回调（设置遮盖配置） */
    m_server.Put("/CoverConfig",
                 [this](const httplib::Request &req, httplib::Response &res)
                 {
                     auth_handler_.set_auth_info(ONVIF_DEFAULE_UPGRADE_REALM,
                                                 ONVIF_DEFAULE_UPGRADE_USER,
                                                 CUserManage::instance()->get_passwd(ONVIF_DEFAULE_UPGRADE_USER));
                     if (!auth_handler_.handle_authentication(req, res))
                     {
                         dlog_warn("设置遮盖配置认证失败，客户端IP: %s", req.remote_addr.c_str());
                         return;
                     }

                     dlog_debug("收到设置遮盖配置请求");
                     if (req.body.empty())
                     {
                         res.status = 400;
                         res.set_content("Empty body", "text/plain");
                         return;
                     }

                     Osd::CoverConfig_S stCoverConfig;
                     Osd::CoverConfig_S stTmpCoverConfig;
                     Convert::to_struct(req.body, stCoverConfig);

                     COsdManage::instance()->get_cover_config(stTmpCoverConfig);
                     stTmpCoverConfig.bEnable = stCoverConfig.bEnable;
                     /* 只允许设置一个遮盖区域 */
                     for (size_t i = 0; i < stCoverConfig.vecCoverAttr.size(); i++)
                     {
                         if (i >= 1)
                         {
                             break;
                         }
                         dlog_debug("遮盖区域设置: 启用=%d, 坐标=(%d,%d), 宽=%d, 高=%d",
                                    stCoverConfig.vecCoverAttr[i].bEnable,
                                    stCoverConfig.vecCoverAttr[i].nX,
                                    stCoverConfig.vecCoverAttr[i].nY,
                                    stCoverConfig.vecCoverAttr[i].nWidth,
                                    stCoverConfig.vecCoverAttr[i].nHeight);
                         stTmpCoverConfig.vecCoverAttr[i].bEnable = stCoverConfig.vecCoverAttr[i].bEnable;
                         stTmpCoverConfig.vecCoverAttr[i].nX = stCoverConfig.vecCoverAttr[i].nX;
                         stTmpCoverConfig.vecCoverAttr[i].nY = stCoverConfig.vecCoverAttr[i].nY;
                         stTmpCoverConfig.vecCoverAttr[i].nWidth = stCoverConfig.vecCoverAttr[i].nWidth;
                         stTmpCoverConfig.vecCoverAttr[i].nHeight = stCoverConfig.vecCoverAttr[i].nHeight;
                     }

                     if (COsdManage::instance()->set_cover_config(stTmpCoverConfig) != 0)
                     {
                         res.status = 500;
                         res.set_content("Failed to set cover config", "text/plain");
                         return;
                     }
                     res.status = 200;
                     res.set_content("OK", "text/plain");
                 });

    /* 启动 HTTP 服务线程 */
    m_serverThread = std::thread(
        [this]()
        {
            m_running = true;
            if (!m_server.listen("0.0.0.0", nPort))
            {
                int error_code = errno;
                std::error_code ec(error_code, std::system_category());
                dlog_error("ONVIF固件升级服务启动失败: %s (错误码: %d)", ec.message().c_str(), error_code);
                m_running = false;
            }
            else
            {
                dlog_info("ONVIF固件升级服务启动成功，端口: %d", nPort);
            }
        });
    dlog_info("ONVIF固件升级服务已启动");

    return OK;
}

int COnvifFirmwareUpgradeServer::deinit()
{
    if (!m_running)
    {
        return OK;
    }

    m_running = false;
    m_server.stop();
    if (m_serverThread.joinable())
    {
        m_serverThread.join();
    }
    dlog_info("ONVIF固件升级服务已停止");
	return OK;
}

int COnvifFirmwareUpgradeServer::handle_firmware_upload(const httplib::Request &req,
                                                        httplib::Response &res,
                                                        const httplib::ContentReader &content_reader)
{
    if (!auth_handler_.handle_authentication(req, res))
    {
        dlog_warn("固件上传认证失败，客户端IP: %s", req.remote_addr.c_str());
        return ERR;
    }

    dlog_info("收到固件上传请求，客户端IP: %s, 路径: %s, 固件大小: %zu字节",
              req.remote_addr.c_str(),
              req.path.c_str(),
              req.body.size());

    /* 验证内容类型 */
    std::string strContentType = req.get_header_value("Content-Type");
    if (strContentType != "application/octetstream")
    {
        res.status = 400;
        res.set_content("Invalid Content-Type", "text/plain");
        dlog_error("无效的Content-Type: %s", strContentType.c_str());
        return ERR;
    }

    std::ofstream fw_file(strFirmwarePath, std::ios::binary | std::ios::trunc);
    size_t total_written = 0;
    int nRet = ERR;
    bool is_valid = false;
    bool is_error = false;

    if (req.is_multipart_form_data())
    {
        httplib::MultipartFormDataItems files;
        content_reader(
            [&](const httplib::MultipartFormData &file)
            {
                files.push_back(file);
                dlog_debug("获取文件名: %s, 文件类型: %s", file.filename.c_str(), file.content.c_str());
                return true;
            },
            [&](const char *data, size_t data_length)
            {
                files.back().content.append(data, data_length);
                total_written += data_length;
                dlog_debug("已写入: %zu 字节", total_written);
                fw_file.write(data, data_length);
                fw_file.flush();
                return true;
            });
    }
    else
    {
        UpgradeCheckContext ctx;

        content_reader(
            [&](const char *data, size_t data_length)
            {
                if (!ctx.is_struct_checked)
                {
                    /* 计算还需要多少字节才能凑齐结构体 */
                    size_t need = ctx.STRUCT_SIZE - ctx.struct_buffer.size();
                    size_t copy_len = std::min(need, data_length);
                    const unsigned char *udata = reinterpret_cast<const unsigned char *>(data);
                    ctx.struct_buffer.insert(ctx.struct_buffer.end(), udata, udata + copy_len);

                    /* 当缓存足够时，解析结构体并校验 id */
                    if (ctx.struct_buffer.size() >= ctx.STRUCT_SIZE)
                    {
                        const Upgrade_Package_t *pkg = reinterpret_cast<const Upgrade_Package_t *>(ctx.struct_buffer.data());
                        ctx.is_struct_checked = true;

                        /* 打印升级包 ID 的十六进制内容，便于调试 */
                        dlog_debug("升级包ID: 0x%02X 0x%02X 0x%02X 0x%02X",
                                   (unsigned char) pkg->id[0],
                                   (unsigned char) pkg->id[1],
                                   (unsigned char) pkg->id[2],
                                   (unsigned char) pkg->id[3]);

                        if (0 == strcmp(pkg->id, UPGRADE_ID))
                        {
                            dlog_info("升级包校验通过，版本号: %s", pkg->version);
                            is_valid = true;
                        }
                        else
                        {
                            dlog_warn("升级包ID校验不通过");
                            is_valid = false;
                            return false;
                        }
                    }
                }

                total_written += data_length;
                fw_file.write(data, data_length);
                if (fw_file.fail())
                {
                    dlog_error("写入固件文件失败");
                    is_error = true;
                    return false;
                }
                fw_file.flush();
                if (fw_file.fail())
                {
                    dlog_error("刷新固件文件到磁盘失败");
                    is_error = true;
                    return false;
                }
                return true;
            });
    }
    fw_file.close();

    if (is_error)
    {
        nRet = ERR;
        res.status = 500;
        res.set_content("Internal Server Error", "text/plain");
    }
    else if (!is_valid)
    {
        nRet = ERR;
        dlog_warn("升级包无效，删除临时文件");
        remove(strFirmwarePath.c_str());
        res.status = 415;
        res.set_content("Unsupported Media Type", "text/plain");
    }
    else
    {
        nRet = OK;
        res.status = 200;
        res.set_content("OK", "text/plain");
    }

    return nRet;
}
