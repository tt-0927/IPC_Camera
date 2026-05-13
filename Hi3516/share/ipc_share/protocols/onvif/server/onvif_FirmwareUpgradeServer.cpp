/**
 * @file onvif_FirmwareUpgradeServer.cpp
 * @author tianl (tianl@kfb.cn)
 * @date 2025-10-15
 * 
 * @brief  onvif http固件升级、遮盖配置
 */
#include "onvif_FirmwareUpgradeServer.h"
#include "upgrade_client.h"
#include "action_code.h"
#include "system_convert.h"
#include "convert_interface.h"	
#include "share_define.h"
#include "user_manage.h"
#include "osd_manage.h"
#include "osd_convert.h"

bool COnvifFirmwareUpgradeServer::start() 
{
	dlog_debug("COnvifFirmwareUpgradeServer start==========================================");
	if (m_running) 
	{
		dlog_debug("服务器已在运行");
		return true;
	}

	m_server.set_payload_max_length(nMaxSize);            // 限制最大固件大小
	m_server.set_keep_alive_max_count(1);                 // 关闭长连接（避免客户端超时）

	// 注册 POST 处理回调（处理固件上传）
    m_server.Post(
        "/StartFirmwareUpgrade",
        [this](const httplib::Request &req, httplib::Response &res, const httplib::ContentReader &content_reader)
        {
            auth_handler_.set_auth_info(ONVIF_DEFAULE_UPGRADE_REALM,
                                        ONVIF_DEFAULE_UPGRADE_USER,
                                        CUserManage::instance()->get_passwd(ONVIF_DEFAULE_UPGRADE_USER));
            if (handle_firmware_upload(req, res, content_reader) == 0)
            {
                dlog_debug("固件校验上传完成开始升级！");
                System::UpgradeInfo_S stUpgradeInfo;

                /* 通知upgrade */
                stUpgradeInfo.strUpgradePath = ONVIF_UPGRADE_FURWARE;
                dlog_trace("升级路径[%s]", stUpgradeInfo.strUpgradePath.c_str());
                std::string data = Convert::to_string(stUpgradeInfo);
                CUpgradeClient::instance()->fill_head(data, AC_DO_UPGRADE);
                CUpgradeClient::instance()->send(data, AC_DO_UPGRADE);
            }
        });

    // 注册 GET 处理回调（获取遮盖配置）
    m_server.Get(
        "/CoverConfig",
        [this](const httplib::Request &req, httplib::Response &res)
        {
            auth_handler_.set_auth_info(ONVIF_DEFAULE_UPGRADE_REALM,
                                        ONVIF_DEFAULE_UPGRADE_USER,
                                        CUserManage::instance()->get_passwd(ONVIF_DEFAULE_UPGRADE_USER));
            if (!auth_handler_.handle_authentication(req, res))
            {
                dlog_warn("认证失败，客户端 IP: %s", req.remote_addr.c_str());
                return;
            }

            dlog_debug("收到获取 CoverConfig 请求");
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

    // 注册 PUT 处理回调（设置遮盖配置）
    m_server.Put(
        "/CoverConfig",
        [this](const httplib::Request &req, httplib::Response &res)
        {
            auth_handler_.set_auth_info(ONVIF_DEFAULE_UPGRADE_REALM,
                                        ONVIF_DEFAULE_UPGRADE_USER,
                                        CUserManage::instance()->get_passwd(ONVIF_DEFAULE_UPGRADE_USER));
            if (!auth_handler_.handle_authentication(req, res))
            {
                dlog_warn("认证失败，客户端 IP: %s", req.remote_addr.c_str());
                return;
            }

            dlog_debug("收到设置 CoverConfig 请求:%s",req.body.c_str());
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
			/* 只允许设置一个遮盖 */
			for (size_t i = 0; i < stCoverConfig.vecCoverAttr.size(); i++)
            {
				if(i >= 1)
				{
					break;
				}
				dlog_debug("http遮盖区域设置: 是否启用[%d] 坐标(%d,%d) 宽度[%d] 高度[%d]",
				stCoverConfig.vecCoverAttr[i].bEnable,stCoverConfig.vecCoverAttr[i].nX,stCoverConfig.vecCoverAttr[i].nY,
				stCoverConfig.vecCoverAttr[i].nWidth,stCoverConfig.vecCoverAttr[i].nHeight);
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

    m_serverThread = std::thread([this]() 
	{
		m_running = true;
		if (!m_server.listen("0.0.0.0", nPort)) 
		{  
			int error_code = errno; // 获取系统错误码
			std::error_code ec(error_code, std::system_category());
			
			dlog_error("服务器启动失败: %s (错误码: %d)", ec.message().c_str(), error_code);
						m_running = false;
		}
		else
		{
			dlog_debug("服务器启动成功");
		}
	});

	return m_running;
}


void COnvifFirmwareUpgradeServer::stop() 
{
	if (!m_running)
	{
		return;
	} 

	m_running = false;
	m_server.stop();  
	if (m_serverThread.joinable()) 
	{
		m_serverThread.join();  
	}
	dlog_debug("固件升级服务已停止");
}

int COnvifFirmwareUpgradeServer::handle_firmware_upload(const httplib::Request& req, httplib::Response& res,const httplib::ContentReader &content_reader) 
{
	if (!auth_handler_.handle_authentication(req, res)) 
	{
		dlog_warn("认证失败，客户端 IP: %s", req.remote_addr.c_str());
		return -1; 
    }
	
	dlog_debug("收到固件上传请求，客户端 IP:%s ，",req.remote_addr.c_str());
	std::cout << "=== 收到固件上传请求 ===" << std::endl;
	std::cout << "客户端 IP: " << req.remote_addr << std::endl;
	std::cout << "请求路径: " << req.path << std::endl;
	std::cout << "固件大小: " << req.body.size() << " 字节" << std::endl;

	std::cerr << "Server-log: upload\t" << req.get_header_value("Content-Type") << std::endl;
	/* 验证内容类型 */
	if (req.get_header_value("Content-Type") != "application/octetstream") 
	{
		res.status = 400;
		res.set_content("Invalid Content-Type", "text/plain");
		dlog_error("无效 Content-Type");
		return -1; 
    }

	std::ofstream fw_file(strFirmwarePath, std::ios::binary | std::ios::trunc);
	size_t total_written = 0;
	int nRet = -1;
	bool is_valid = false;
	bool is_error = false;
	// 二进制数据可以用：multipart/form-data 和 application/octet-stream
	if (req.is_multipart_form_data()) 
	{
		httplib::MultipartFormDataItems files;
		// 先拿到 file 信息，再流式读取
		// 通过file信息创建文件，然后持续写data数据，结束后关闭文件
		content_reader([&](const httplib::MultipartFormData &file) 
			{
				files.push_back(file);
				dlog_debug("====获取文件名[%s] 文件类型 [%s]==",file.filename.c_str(),file.content.c_str());
				return true;
			},
			[&](const char *data, size_t data_length) 
			{
				files.back().content.append(data, data_length);
				std::cout << "已写入: " << total_written << " 字节" << std::endl;
				fw_file.write(data, data_length);
				fw_file.flush();
				return true;
			});
	}
	else 
	{
		UpgradeCheckContext ctx;
		
		content_reader([&](const char *data, size_t data_length) 
		{
			if (!ctx.is_struct_checked) 
			{
				// 计算还需要多少字节才能凑齐结构体
				size_t need = ctx.STRUCT_SIZE - ctx.struct_buffer.size();
				size_t copy_len = std::min(need, data_length);
				// 从当前数据中取部分填充缓存（不超过需要的字节数）
				const unsigned char* udata = reinterpret_cast<const unsigned char*>(data);
            	ctx.struct_buffer.insert(ctx.struct_buffer.end(), udata, udata + copy_len);
				// 当缓存足够97字节时，解析结构体并校验id
				if (ctx.struct_buffer.size() >= ctx.STRUCT_SIZE) 
				{
					// 将缓存数据转换为Upgrade_Package_t结构体
					const Upgrade_Package_t* pkg = reinterpret_cast<const Upgrade_Package_t*>(ctx.struct_buffer.data());

					printf("升级包id[4] 内容：\n");
					for (int i = 0; i < 4; i++) 
					{
						// 判断是否为可打印字符（ASCII 32-126）
						if (isprint((unsigned char)pkg->id[i])) 
						{
							printf("  id[%d]：字符='%c'，十六进制=0x%02X\n", i, pkg->id[i], (unsigned char)pkg->id[i]);
						} 
						else 
						{
							printf("  id[%d]：不可打印字符，十六进制=0x%02X\n", i, (unsigned char)pkg->id[i]);
						}
					}

					// 校验通过：标记结构体已处理
					ctx.is_struct_checked = true;
					if(0 == strcmp(pkg->id, UPGRADE_ID))
					{
						std::cout << "升级包id校验通过，版本号：" << pkg->version << std::endl;
						is_valid = true;
					}
					else
					{
						std::cout << "升级包id校验不通过 !" << std::endl;
						is_valid = false;
						return false;
					}
            	}
        	}

			total_written += data_length;
			//std::cout << "已写入: " << total_written << " 字节" << std::endl;
			fw_file.write(data, data_length);
			if (fw_file.fail())
			{
				dlog_error("写入文件失败!");
				is_error = true;
				return false;
			}
			fw_file.flush();
			if (fw_file.fail())
			{
				dlog_error("刷新磁盘失败!");
				is_error = true;
				return false;
			}
			return true;
		});
	}
	fw_file.close();

	if (is_error) 
	{
		nRet = -1;
		res.status = 500;
		res.set_content("Internal Server Error", "text/plain");
	}
	else if (!is_valid) 
	{
		nRet = -1;
		std::cout << "删除文件并且返回错误码[415]" << std::endl;
        remove(strFirmwarePath.c_str());
        res.status = 415;
        res.set_content("Unsupported Media Type", "text/plain");
    }
	else
	{
		nRet = 0;
		res.status = 200;  
		res.set_content("OK", "text/plain");
	}

	return nRet;
}
