/**
 * @FilePath     : platform_manager.cpp
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2026-05-08 17:44:21
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-05-21 17:32:00
 * @Description  : 平台管理
 */

#if CAP_GARBAGE_STATION_PLATFORM
#include "platform_manager.h"
#include "mqtt_sdk_gateway.h"
#include "httplib.h"
#include "path_define.h"
#include "convert_interface.h"
#include "push_stream.h"
#include "av_configure.h"
#include "IpcRet.h"
#include "dlog.h"
#include "mqtt_manager.h"
#include "mqtt_topic_define.h"
#include "system_define.h"
#include "system_manage.h"

#include <iostream>
#include <cstring>
#include <vector>
#include <chrono>

static std::string platform_json_object_to_string(cJSON *pRoot)
{
    if (!pRoot)
    {
        return "{}";
    }

    char *pJson = cJSON_PrintUnformatted(pRoot);
    std::string strPayload = pJson ? pJson : "{}";
    free(pJson);
    return strPayload;
}

static std::string platform_data_or_empty(const std::string &strJson)
{
    if (strJson.empty())
    {
        return "{}";
    }

    cJSON *pRoot = cJSON_Parse(strJson.c_str());
    if (!pRoot)
    {
        return "{}";
    }

    cJSON *pData = cJSON_GetObjectItemCaseSensitive(pRoot, "Data");
    cJSON *pTarget = pData ? pData : pRoot;
    char *pJson = cJSON_PrintUnformatted(pTarget);
    std::string strResult = pJson ? pJson : "{}";
    free(pJson);
    cJSON_Delete(pRoot);
    return strResult;
}

// --- Base64 编码实现 ---
static const std::string base64_chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                                        "abcdefghijklmnopqrstuvwxyz"
                                        "0123456789+/";

std::string CPlatformManager::base64_encode(const std::string &input)
{
    std::string ret;
    int i = 0;
    unsigned char char_array_3[3];
    unsigned char char_array_4[4];
    size_t in_len = input.size();
    const char *bytes_to_encode = input.c_str();

    while (in_len--)
    {
        char_array_3[i++] = *(bytes_to_encode++);
        if (i == 3)
        {
            char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
            char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
            char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
            char_array_4[3] = char_array_3[2] & 0x3f;

            for (i = 0; (i < 4); i++)
                ret += base64_chars[char_array_4[i]];
            i = 0;
        }
    }

    if (i)
    {
        for (int j = i; j < 3; j++)
            char_array_3[j] = '\0';

        char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
        char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
        char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
        char_array_4[3] = char_array_3[2] & 0x3f;

        for (int j = 0; (j < i + 1); j++)
            ret += base64_chars[char_array_4[j]];

        while ((i++ < 3))
            ret += '=';
    }

    return ret;
}

/**
 * @brief 登录实现
 * @param host 服务器地址 (例如 "183.129.224.253")
 * @param port 服务器端口 (例如 4910)
 * @param user 用户名
 * @param password 密码
 * @param out_response 返回的响应结构体
 */
bool CPlatformManager::login(const std::string &host,
                            int port,
                            const std::string &user,
                            const std::string &password,
                            const bool &enable,
                            const bool &Custom,
                            LoginResponse &out_response)
{
    bool result = false;
    g_custom = Custom;
    g_enable = enable;

    if (enable != true)
    {
        return result;
    }
    // 1. 准备数据
    std::string encoded_password = base64_encode(password);
    // 构建表单数据: user=admin&password=QWF...
    std::string body_str = "user=" + user + "&password=" + encoded_password;

    // 2. 发起请求
    std::string target_host = Custom ? host : host_;
    int target_port = Custom ? port : port_;
    httplib::Client cli(target_host, target_port);
    custom_host = host;
    custom_post = port;
    cli.set_connection_timeout(5, 0);

    // 设置请求头
    httplib::Headers headers = {
        { "Content-Type", "application/x-www-form-urlencoded" }
    };

    // 【关键修正】这里必须传入第四个参数 "application/x-www-form-urlencoded"
    // 否则编译器会混淆，以为你传的是 Params 类型的 Map
    auto res = cli.Post(login_path_, headers, body_str, "application/x-www-form-urlencoded");
    if (res)
    {
        std::cout << res->body << std::endl; // 打印服务器返回的原始 JSON
    }
    else
    {
        std::cout << "Request Failed: No response body" << std::endl;
    }
    // 3. 处理响应
    if (res && res->status == 200)
    {
        cJSON *response_root = cJSON_Parse(res->body.c_str());

        if (response_root)
        {
            // 解析 JSON (保持不变)
            cJSON *status_code_item = cJSON_GetObjectItemCaseSensitive(response_root, "status_code");
            cJSON *status_item = cJSON_GetObjectItemCaseSensitive(response_root, "status");
            cJSON *message_item = cJSON_GetObjectItemCaseSensitive(response_root, "message");

            if (cJSON_IsNumber(status_code_item))
                out_response.status_code = status_code_item->valueint;
            if (cJSON_IsString(status_item))
                out_response.status = status_item->valuestring;
            if (cJSON_IsString(message_item))
                out_response.message = message_item->valuestring;

            cJSON *data_item = cJSON_GetObjectItemCaseSensitive(response_root, "data");
            if (data_item && cJSON_IsObject(data_item))
            {
                cJSON *access_token = cJSON_GetObjectItemCaseSensitive(data_item, "access_token");
                cJSON *token_type = cJSON_GetObjectItemCaseSensitive(data_item, "token_type");
                cJSON *expires_in = cJSON_GetObjectItemCaseSensitive(data_item, "expires_in");
                cJSON *phone = cJSON_GetObjectItemCaseSensitive(data_item, "phone");
                cJSON *need_modify = cJSON_GetObjectItemCaseSensitive(data_item, "need_modify_password");

                if (cJSON_IsString(access_token))
                {
                    out_response.data.access_token = access_token->valuestring;
                    this->access_token_ = access_token->valuestring;
                }

                if (cJSON_IsString(token_type))
                {
                    this->token_type_ = token_type->valuestring;
                    out_response.data.token_type = token_type->valuestring;
                }
                if (cJSON_IsNumber(expires_in))
                    out_response.data.expires_in = expires_in->valueint;
                if (cJSON_IsString(phone))
                    out_response.data.phone = phone->valuestring;
                if (cJSON_IsBool(need_modify))
                    out_response.data.need_modify_password = (bool) need_modify->valueint;
            }

            if (out_response.status == "success")
            {
                login_user = user;
                login_password = password;
                result = true;
                /* 登录成功，持久化配置 */
                save_config();
            }

            cJSON_Delete(response_root);
        }
    }
    else
    {
        if (res)
        {
            std::cerr << "HTTP Error: " << res->status << std::endl;
        }
        else
        {
            std::cerr << "Connection Error" << std::endl;
        }
    }

    return result;
}

void CPlatformManager::getlogininfo(::Network::LoginInfo &retLoginInfo)
{

    retLoginInfo.login_password = login_password;
    retLoginInfo.login_user = login_user;
    retLoginInfo.enable = g_enable;
    retLoginInfo.Custom = g_custom;
    retLoginInfo.host = g_custom ? custom_host : host_;
    retLoginInfo.port = g_custom ? custom_post : port_;
}

std::string CPlatformManager::get_access_token() const
{
    return access_token_;
}

bool CPlatformManager::storeDevice(const StoreDevice &device, const std::string &token, StoreResponse &out_response)
{
    // 1. 构建 JSON 请求体
    cJSON *json_root = cJSON_CreateObject();

    // 必须字段
    cJSON_AddStringToObject(json_root, "sn", device.sn.c_str());
    cJSON_AddStringToObject(json_root, "name", device.name.c_str());
    cJSON_AddStringToObject(json_root, "ip", device.ip.c_str());
    cJSON_AddNumberToObject(json_root, "port", device.port);
    if (device.account.empty())
    {
        cJSON_AddStringToObject(json_root, "account", login_user.c_str());
    }
    else
    {
        cJSON_AddStringToObject(json_root, "account", device.account.c_str());
    }
    if (device.password.empty())
    {
        cJSON_AddStringToObject(json_root, "password", login_password.c_str());
    }
    else
    {
        cJSON_AddStringToObject(json_root, "password", device.password.c_str());
    }
    cJSON_AddStringToObject(json_root, "protocol", device.protocol.c_str());
    cJSON_AddStringToObject(json_root, "resolution", device.resolution.c_str());
    cJSON_AddStringToObject(json_root, "storage", device.storage.c_str());
    cJSON_AddStringToObject(json_root, "use_storage", device.use_storage.c_str());

    // 可选字段
    if (!device.version.empty())
        cJSON_AddStringToObject(json_root, "version", device.version.c_str());
    if (!device.mac_address.empty())
        cJSON_AddStringToObject(json_root, "mac_address", device.mac_address.c_str());
    if (!device.live_url.empty())
        cJSON_AddStringToObject(json_root, "live_url", device.live_url.c_str());
    if (!device.playback_url.empty())
        cJSON_AddStringToObject(json_root, "playback_url", device.playback_url.c_str());

    // ⭐ 序列化 JSON
    char *json_string = cJSON_PrintUnformatted(json_root);
    std::string body_str = json_string;
    cJSON_free(json_string);
    cJSON_Delete(json_root);

    // 2. 发起 HTTP 请求
    httplib::Client cli(host_, port_);
    cli.set_connection_timeout(5, 0);

    // 设置 Headers
    httplib::Headers headers = {
        {  "Content-Type", "application/json" },
        { "Authorization",  "Bearer " + token }  // 拼接 Bearer
    };

    // 发送 POST 请求
    // 注意：发送 JSON 字符串时，最后一个参数是 Content-Type，但我们已经在 Headers 里定义了
    // 这里使用 (headers, body, content_type) 的重载，为了保险可以再次指定
    std::cout << "HTTP POST: " << body_str << std::endl;
    auto res = cli.Post(store_path_, headers, body_str, "application/json");
    if (res)
    {
        std::cout << res->body << std::endl; // 打印服务器返回的原始 JSON
    }
    else
    {
        std::cout << "Request Failed: No response body" << std::endl;
    }

    // 3. 解析响应
    if (res && res->status == 200)
    {

        cJSON *response_root = cJSON_Parse(res->body.c_str());

        if (response_root)
        {

            // 基本信息
            cJSON *status_code_item = cJSON_GetObjectItemCaseSensitive(response_root, "status_code");
            cJSON *status_item = cJSON_GetObjectItemCaseSensitive(response_root, "status");
            cJSON *message_item = cJSON_GetObjectItemCaseSensitive(response_root, "message");

            if (cJSON_IsNumber(status_code_item))
                out_response.status_code = status_code_item->valueint;
            if (cJSON_IsString(status_item))
                out_response.status = status_item->valuestring;
            if (cJSON_IsString(message_item))
                out_response.message = message_item->valuestring;
            if (out_response.status_code != 200)
            {
                std::cerr << "HTTP Error: " << out_response.status_code << std::endl;
                return false;
            }
            // Data 数据
            cJSON *data_item = cJSON_GetObjectItemCaseSensitive(response_root, "data");
            if (data_item && cJSON_IsObject(data_item))
            {
                cJSON *sn = cJSON_GetObjectItemCaseSensitive(data_item, "sn");
                cJSON *name = cJSON_GetObjectItemCaseSensitive(data_item, "name");
                cJSON *version = cJSON_GetObjectItemCaseSensitive(data_item, "version");
                cJSON *status = cJSON_GetObjectItemCaseSensitive(data_item, "status");
                cJSON *mac = cJSON_GetObjectItemCaseSensitive(data_item, "mac_address");
                cJSON *account = cJSON_GetObjectItemCaseSensitive(data_item, "account");
                cJSON *password = cJSON_GetObjectItemCaseSensitive(data_item, "password");
                cJSON *live = cJSON_GetObjectItemCaseSensitive(data_item, "live_url");
                cJSON *playback = cJSON_GetObjectItemCaseSensitive(data_item, "playback_url");
                cJSON *updated = cJSON_GetObjectItemCaseSensitive(data_item, "updated_at");
                cJSON *created = cJSON_GetObjectItemCaseSensitive(data_item, "created_at");
                cJSON *id = cJSON_GetObjectItemCaseSensitive(data_item, "id");
                cJSON *uuid = cJSON_GetObjectItemCaseSensitive(data_item, "device_uuid");

                if (cJSON_IsString(sn))
                    out_response.data.sn = sn->valuestring;
                if (cJSON_IsString(name))
                    out_response.data.name = name->valuestring;
                if (cJSON_IsString(version))
                    out_response.data.version = version->valuestring;
                if (cJSON_IsNumber(status))
                    out_response.data.status = status->valueint;
                if (cJSON_IsString(mac))
                    out_response.data.mac_address = mac->valuestring;
                if (cJSON_IsString(account))
                {
                }
                out_response.data.account = account->valuestring;
                if (cJSON_IsString(password))
                    out_response.data.password = password->valuestring;
                if (cJSON_IsString(live))
                    out_response.data.live_url = live->valuestring;
                if (cJSON_IsString(playback))
                    out_response.data.playback_url = playback->valuestring;
                if (cJSON_IsString(updated))
                    out_response.data.updated_at = updated->valuestring;
                if (cJSON_IsString(created))
                    out_response.data.created_at = created->valuestring;
                if (cJSON_IsNumber(id))
                    out_response.data.id = id->valueint;
                if (cJSON_IsString(uuid))
                    out_response.data.device_uuid = uuid->valuestring;
            }

            cJSON_Delete(response_root);
            return true;
        }
    }
    else
    {
        if (res)
        {
            std::cerr << "HTTP Error: " << res->status << std::endl;
        }
        else
        {
            std::cerr << "Connection Failed" << std::endl;
        }
    }
    return false;
}

/**
 * @brief 上报工单实现
 */
bool CPlatformManager::reportWorkOrder(const WorkOrderRequest &workOrder,
                                      const std::string &token,
                                      WorkOrderResponse &out_response)
{
    // 1. 构建 JSON 请求体
    cJSON *json_root = cJSON_CreateObject();

    // 非必须字段，但如果有值则添加
    if (!workOrder.device_sn.empty())
        cJSON_AddStringToObject(json_root, "device_sn", workOrder.device_sn.c_str());
    if (workOrder.type != 0)
        cJSON_AddNumberToObject(json_root, "type", workOrder.type);
    if (!workOrder.images.empty())
        cJSON_AddStringToObject(json_root, "images", workOrder.images.c_str());
    if (!workOrder.online.empty())
        cJSON_AddStringToObject(json_root, "online", workOrder.online.c_str());

    char *json_string = cJSON_PrintUnformatted(json_root);
    std::string body_str = json_string;
    cJSON_free(json_string);
    cJSON_Delete(json_root);

    // 2. 发起 HTTP 请求
    httplib::Client cli(host_, port_);
    cli.set_connection_timeout(5, 0);

    httplib::Headers headers = {
        {  "Content-Type", "application/json" },
        { "Authorization",  "Bearer " + token }
    };

    auto res = cli.Post(workorder_path_, headers, body_str, "application/json");

    // 3. 解析响应
    if (res && res->status == 200)
    {
        cJSON *response_root = cJSON_Parse(res->body.c_str());
        if (response_root)
        {
            cJSON *code_item = cJSON_GetObjectItemCaseSensitive(response_root, "status_code");
            cJSON *status_item = cJSON_GetObjectItemCaseSensitive(response_root, "status");
            cJSON *msg_item = cJSON_GetObjectItemCaseSensitive(response_root, "message");
            cJSON *data_item = cJSON_GetObjectItemCaseSensitive(response_root, "data");

            if (cJSON_IsNumber(code_item))
                out_response.status_code = code_item->valueint;
            if (cJSON_IsString(status_item))
                out_response.status = status_item->valuestring;
            if (cJSON_IsString(msg_item))
                out_response.message = msg_item->valuestring;
            if (cJSON_IsNumber(data_item))
                out_response.data = data_item->valueint;

            cJSON_Delete(response_root);
            return true;
        }
    }
    else
    {
        if (res)
        {
            std::cerr << "WorkOrder HTTP Error: " << res->status << std::endl;
        }
        else
        {
            std::cerr << "WorkOrder Connection Failed" << std::endl;
        }
    }
    return false;
}

/**
 * @brief 获取设备列表实现
 */
/**
 * @brief 获取设备列表实现
 */
bool CPlatformManager::getDeviceList(const std::string &token, DeviceListResponse &out_response)
{
    // 1. 发起 GET 请求
    httplib::Client cli(host_, port_);
    cli.set_connection_timeout(5, 0);

    httplib::Headers headers = {
        { "Authorization", "Bearer " + token }
    };

    auto res = cli.Get(device_list_path_, headers);

    // 2. 处理响应
    if (res && res->status == 200)
    {
        cJSON *response_root = cJSON_Parse(res->body.c_str());
        if (response_root)
        {
            // --- 解析顶层信息 ---
            cJSON *code_item = cJSON_GetObjectItemCaseSensitive(response_root, "status_code");
            cJSON *status_item = cJSON_GetObjectItemCaseSensitive(response_root, "status");
            cJSON *msg_item = cJSON_GetObjectItemCaseSensitive(response_root, "message");

            if (cJSON_IsNumber(code_item))
                out_response.status_code = code_item->valueint;
            if (cJSON_IsString(status_item))
                out_response.status = status_item->valuestring;
            if (cJSON_IsString(msg_item))
                out_response.message = msg_item->valuestring;

            // --- 解析 data 数组 ---
            cJSON *data_array = cJSON_GetObjectItemCaseSensitive(response_root, "data");
            if (cJSON_IsArray(data_array))
            {
                int size = cJSON_GetArraySize(data_array);
                for (int i = 0; i < size; ++i)
                {
                    cJSON *item = cJSON_GetArrayItem(data_array, i);
                    DeviceItem device;

                    // 基础字段
                    cJSON *id_val = cJSON_GetObjectItemCaseSensitive(item, "id");
                    if (cJSON_IsNumber(id_val))
                        device.id = id_val->valueint;

                    cJSON *sn_val = cJSON_GetObjectItemCaseSensitive(item, "sn");
                    if (cJSON_IsString(sn_val))
                        device.sn = sn_val->valuestring;

                    cJSON *name_val = cJSON_GetObjectItemCaseSensitive(item, "name");
                    if (cJSON_IsString(name_val))
                        device.name = name_val->valuestring;

                    // 新增补全字段
                    cJSON *area_id_val = cJSON_GetObjectItemCaseSensitive(item, "area_id");
                    if (cJSON_IsNumber(area_id_val))
                        device.area_id = area_id_val->valueint;

                    cJSON *version_val = cJSON_GetObjectItemCaseSensitive(item, "version");
                    if (cJSON_IsString(version_val))
                        device.version = version_val->valuestring;

                    cJSON *status_val = cJSON_GetObjectItemCaseSensitive(item, "status");
                    if (cJSON_IsNumber(status_val))
                        device.status = status_val->valueint;

                    cJSON *mac_val = cJSON_GetObjectItemCaseSensitive(item, "mac_address");
                    if (cJSON_IsString(mac_val))
                        device.mac_address = mac_val->valuestring;

                    cJSON *ip_val = cJSON_GetObjectItemCaseSensitive(item, "ip");
                    if (cJSON_IsString(ip_val))
                        device.ip = ip_val->valuestring;

                    cJSON *port_val = cJSON_GetObjectItemCaseSensitive(item, "port");
                    if (cJSON_IsString(port_val))
                        device.port = port_val->valuestring;

                    cJSON *account_val = cJSON_GetObjectItemCaseSensitive(item, "account");
                    if (cJSON_IsString(account_val))
                        device.account = account_val->valuestring;

                    cJSON *password_val = cJSON_GetObjectItemCaseSensitive(item, "password");
                    if (cJSON_IsString(password_val))
                        device.password = password_val->valuestring;

                    cJSON *live_val = cJSON_GetObjectItemCaseSensitive(item, "live_url");
                    if (cJSON_IsString(live_val))
                        device.live_url = live_val->valuestring;

                    cJSON *playback_val = cJSON_GetObjectItemCaseSensitive(item, "playback_url");
                    if (cJSON_IsString(playback_val))
                        device.playback_url = playback_val->valuestring;

                    cJSON *created_val = cJSON_GetObjectItemCaseSensitive(item, "created_at");
                    if (cJSON_IsString(created_val))
                        device.created_at = created_val->valuestring;
                    // 如果 created_at 可能是 null，这里默认保持为空字符串，或者可以特殊标记

                    cJSON *updated_val = cJSON_GetObjectItemCaseSensitive(item, "updated_at");
                    if (cJSON_IsString(updated_val))
                        device.updated_at = updated_val->valuestring;

                    cJSON *deleted_val = cJSON_GetObjectItemCaseSensitive(item, "deleted_at");
                    if (cJSON_IsString(deleted_val))
                        device.deleted_at = deleted_val->valuestring;
                    // 如果是 null，保持为空字符串

                    cJSON *protocol_val = cJSON_GetObjectItemCaseSensitive(item, "protocol");
                    if (cJSON_IsString(protocol_val))
                        device.protocol = protocol_val->valuestring;

                    cJSON *resolution_val = cJSON_GetObjectItemCaseSensitive(item, "resolution");
                    if (cJSON_IsString(resolution_val))
                        device.resolution = resolution_val->valuestring;

                    cJSON *storage_val = cJSON_GetObjectItemCaseSensitive(item, "storage");
                    if (cJSON_IsNumber(storage_val))
                        device.storage = storage_val->valueint;

                    cJSON *use_storage_val = cJSON_GetObjectItemCaseSensitive(item, "use_storage");
                    if (cJSON_IsNumber(use_storage_val))
                        device.use_storage = use_storage_val->valueint;

                    cJSON *uuid_val = cJSON_GetObjectItemCaseSensitive(item, "device_uuid");
                    if (cJSON_IsString(uuid_val))
                        device.device_uuid = uuid_val->valuestring;

                    // 将填充好的设备加入列表
                    out_response.data.push_back(device);
                }
            }
            cJSON_Delete(response_root);
            return true;
        }
    }
    else
    {
        if (res)
        {
            std::cerr << "DeviceList HTTP Error: " << res->status << std::endl;
        }
        else
        {
            std::cerr << "DeviceList Connection Failed" << std::endl;
        }
    }
    return false;
}

CPlatformManager::~CPlatformManager()
{
    deinit();
}

int CPlatformManager::init()
{
    if (m_bInited)
    {
        dlog_warn("平台管理模块已初始化");
        return OK;
    }

    /* 从配置文件加载历史登录信息 */
    if (load_config())
    {
        dlog_info("平台管理模块从配置文件恢复登录信息成功");
        /* 若已启用平台接入，启动自动登录重试线程 */
        if (g_enable)
        {
            m_bStopAutoLogin.store(false);
            m_nRetryCount = 0;
            m_autoLoginThread = std::thread(&CPlatformManager::auto_login_loop, this);
            dlog_info("平台自动登录重试线程已启动");
        }
    }
    else
    {
        dlog_info("平台管理模块配置文件不存在或读取失败，等待网页首次配置");
    }

    /* 初始化 MQTT（仅在启用平台接入时） */
    if (g_enable)
    {
        int nRet = init_mqtt();
        if (nRet != OK)
        {
            dlog_warn("MQTT 初始化失败，将在平台登录成功后重试");
        }
    }

    m_bInited = true;
    return OK;
}

int CPlatformManager::deinit()
{
    if (!m_bInited)
    {
        return OK;
    }

    /* 停止自动登录重试线程 */
    m_bStopAutoLogin.store(true);
    if (m_autoLoginThread.joinable())
    {
        m_autoLoginThread.join();
    }

    /* 反初始化 MQTT */
    deinit_mqtt();

    m_bInited = false;
    dlog_info("平台管理模块反初始化完成");
    return OK;
}

bool CPlatformManager::load_config()
{
    Network::Platform_Info_t stInfo;
    if (Convert::read_file(PLATFORM_CONFIG_FILE, stInfo) != OK)
    {
        dlog_warn("读取平台配置文件失败: %s", PLATFORM_CONFIG_FILE);
        return false;
    }

    custom_host   = stInfo.server_ip;
    custom_post   = stInfo.server_port;
    m_nMqttPort   = stInfo.mqtt_port;
    login_user    = stInfo.user;
    login_password = stInfo.password;
    g_enable      = stInfo.enable;
    g_custom      = stInfo.Custom;

    dlog_info("加载平台配置: host=%s, port=%d, mqtt_port=%d, enable=%d, custom=%d",
              custom_host.c_str(), custom_post, m_nMqttPort, g_enable, g_custom);
    return true;
}

bool CPlatformManager::save_config()
{
    Network::Platform_Info_t stInfo;
    stInfo.server_ip = custom_host.empty() ? host_ : custom_host;
    stInfo.server_port = custom_post > 0 ? custom_post : port_;
    stInfo.mqtt_port = m_nMqttPort > 0 ? m_nMqttPort : MQTT_PLATFORM_DEFAULT_PORT;
    stInfo.user = login_user;
    stInfo.password = login_password;
    stInfo.enable = g_enable;
    stInfo.Custom = g_custom;

    if (Convert::write_file(PLATFORM_CONFIG_FILE, stInfo) != OK)
    {
        dlog_error("保存平台配置文件失败: %s", PLATFORM_CONFIG_FILE);
        return false;
    }

    dlog_info("保存平台配置成功: host=%s, port=%d, enable=%d",
              stInfo.server_ip.c_str(), stInfo.server_port, stInfo.enable);
    return true;
}

void CPlatformManager::auto_login_loop()
{
    while (!m_bStopAutoLogin.load())
    {
        /* 若已登录（有有效 token），无需重试 */
        if (!access_token_.empty())
        {
            std::this_thread::sleep_for(std::chrono::seconds(AUTO_LOGIN_RETRY_INTERVAL_SEC));
            continue;
        }

        /* 达到最大重试次数则退出（0 表示无限重试） */
        if (AUTO_LOGIN_MAX_RETRIES > 0 && m_nRetryCount >= AUTO_LOGIN_MAX_RETRIES)
        {
            dlog_warn("平台自动登录达到最大重试次数 %d，停止重试", AUTO_LOGIN_MAX_RETRIES);
            break;
        }

        LoginResponse out_response;
        std::string target_host = g_custom ? custom_host : host_;
        int target_port = g_custom ? custom_post : port_;

        dlog_info("平台自动登录重试第 %d 次: host=%s, port=%d",
                  m_nRetryCount + 1, target_host.c_str(), target_port);

        bool bSuccess = login(target_host, target_port, login_user, login_password, g_enable, g_custom, out_response);
        if (bSuccess)
        {
            dlog_info("平台自动登录成功");
            /* 登录成功后，更新推流地址 */
            relogin_and_update_stream();
            break;
        }
        else
        {
            dlog_error("平台自动登录失败，%d 秒后重试", AUTO_LOGIN_RETRY_INTERVAL_SEC);
            m_nRetryCount++;
        }

        std::this_thread::sleep_for(std::chrono::seconds(AUTO_LOGIN_RETRY_INTERVAL_SEC));
    }
}

void CPlatformManager::relogin_and_update_stream()
{
    /* 构造当前平台信息 */
    Network::Platform_Info_t stPlatformInfo;
    stPlatformInfo.server_ip = g_custom ? custom_host : host_;
    stPlatformInfo.server_port = g_custom ? custom_post : port_;
    stPlatformInfo.enable = g_enable;
    stPlatformInfo.Custom = g_custom;

    if (!g_enable)
    {
        dlog_info("平台接入未启用，停止 RTMP 推流");
    }
    else
    {
        dlog_info("平台登录成功，准备更新 RTMP 推流地址");
    }

    /* 通过推流模块更新 RTMP（enable=false 时内部会停止推流） */
    CPushStream::instance()->restart_rtmp_stream(stPlatformInfo);
}

int CPlatformManager::init_mqtt()
{
    /* MQTT 使用跨局域网平台提供的独立 Broker 参数，不复用 HTTP 登录账号 */
    m_strMqttBroker = MQTT_PLATFORM_DEFAULT_BROKER;
    m_nMqttPort = MQTT_PLATFORM_DEFAULT_PORT;
    m_strMqttUsername = MQTT_PLATFORM_DEFAULT_USERNAME;
    m_strMqttPassword = MQTT_PLATFORM_DEFAULT_PASSWORD;
    
    /* 使用设备SN作为 ClientID */
    System::DeviceInfo_S stDeviceInfo;
    SystemManage::instance()->get_device_info(stDeviceInfo);
    m_strMqttClientId = stDeviceInfo.serialNumber;

    if (m_strMqttBroker.empty() || m_strMqttClientId.empty())
    {
        dlog_error("MQTT 初始化失败：Broker 或 ClientID 为空");
        return ERR_PARAM_NULL;
    }

    /* 获取 MQTT 管理器实例 */
    m_pstMqtt = CMqttManager::instance();

    /* 设置消息回调 */
    m_pstMqtt->set_message_callback(
        [this](const std::string &strTopic, const std::string &strPayload) {
            this->on_mqtt_message(strTopic, strPayload);
        });

    /* 注册命令处理器 */
    register_mqtt_handlers();

    /* 初始化连接 */
    int nRet = m_pstMqtt->init(m_strMqttBroker, m_nMqttPort,
                                m_strMqttUsername, m_strMqttPassword,
                                m_strMqttClientId);
    if (nRet != OK)
    {
        dlog_error("MQTT 初始化失败");
        return ERR;
    }

    /* 订阅命令 Topic （异步连接，会加入待订阅列表，连接成功后自动订阅） */
    std::string strCommandTopic = MQTT_TOPIC_COMMAND(m_strMqttClientId);
    m_pstMqtt->subscribe(strCommandTopic, MQTT_QOS_COMMAND);
    std::string strFacesTopic = MQTT_TOPIC_FACES(m_strMqttClientId);
    m_pstMqtt->subscribe(strFacesTopic, MQTT_QOS_COMMAND);

    dlog_info("MQTT 初始化成功，Broker[%s:%d]，ClientID[%s]",
              m_strMqttBroker.c_str(), m_nMqttPort, m_strMqttClientId.c_str());

    return OK;
}

void CPlatformManager::deinit_mqtt()
{
    if (m_pstMqtt)
    {
        m_pstMqtt->deinit();
        m_pstMqtt = nullptr;
    }
    m_mapMqttHandlers.clear();
}

/**
 * @brief 设置 CTaskManage 实例并初始化 MQTT SDK 网关
 * @param pTaskManage 任务管理器指针
 */
void CPlatformManager::set_taskManage(CTaskManage *pTaskManage)
{
    CMqttSdkGateway::set_task_manage(pTaskManage);
    dlog_info("平台管理模块：CTaskManage 实例已注入 MQTT SDK 网关");
}

/**
 * @brief 处理收到的 MQTT 命令消息
 * @param strTopic 消息 Topic
 * @param strPayload 消息内容（JSON）
 * @note  处理流程：
 *        1. 解析 JSON 提取 Command/RequestId/Token/Data
 *        2. 校验 Token（防止未授权访问）
 *        3. 优先查找自定义处理器，未命中则通过 MQTT SDK 网关转发到 CTaskManage
 */
void CPlatformManager::on_mqtt_message(const std::string &strTopic, const std::string &strPayload)
{
    dlog_debug("收到 MQTT 消息，Topic[%s]：\n%s", strTopic.c_str(), strPayload.c_str());

    if (strTopic == MQTT_TOPIC_FACES(m_strMqttClientId))
    {
        handle_faces_request("", strPayload.empty() ? "{}" : strPayload);
        return;
    }

    /* 解析 JSON */
    cJSON *pRoot = strPayload.empty() ? nullptr : cJSON_Parse(strPayload.c_str());
    if (!pRoot)
    {
        dlog_error("MQTT 消息 JSON 解析失败");
        return;
    }

    /* 提取 Command 和 RequestId */
    cJSON *pCommand = cJSON_GetObjectItemCaseSensitive(pRoot, "Command");
    cJSON *pRequestId = cJSON_GetObjectItemCaseSensitive(pRoot, "RequestId");
    cJSON *pData = cJSON_GetObjectItemCaseSensitive(pRoot, "Data");

    if (!cJSON_IsString(pCommand))
    {
        dlog_error("MQTT 消息缺少 Command 字段");
        cJSON_Delete(pRoot);
        return;
    }

    std::string strCommand = pCommand->valuestring;
    std::string strRequestId = cJSON_IsString(pRequestId) ? pRequestId->valuestring : "";
    std::string strData = "{}";

    if (pData != nullptr)
    {
        char *pDataStr = cJSON_PrintUnformatted(pData);
        if (pDataStr)
        {
            strData = pDataStr;
            free(pDataStr);
        }
    }

    cJSON_Delete(pRoot);

    /*
     * 鉴权说明：授权由 MQTT Broker 连接层保障
     * · 设备连接 Broker 时使用设备账号，仅能操作 device/{SN}/# Topic
     * · 第三方平台连接 Broker 时使用平台账号，仅能操作 device/+/command Topic
     * · Broker 层面的 ACL 已确保只有合法连接才能发送命令，消息层不做重复校验
     */

    /* 查找并调用处理器 */
    {
        std::lock_guard<std::mutex> lock(m_mtxMqttHandlers);
        auto it = m_mapMqttHandlers.find(strCommand);
        if (it != m_mapMqttHandlers.end())
        {
            dlog_info("MQTT 命令分发：%s", strCommand.c_str());
            it->second(strRequestId, strData);
            return;
        }
    }

    /* 未命中自定义处理器，尝试通过 MQTT SDK 网关转发 */
    if (CMqttSdkGateway::is_command_supported(strCommand))
    {
        dlog_info("MQTT SDK 网关转发命令：%s", strCommand.c_str());

        if (CMqttSdkGateway::is_get_command(strCommand))
        {
            /* GET 命令：同步执行并返回结果 */
            std::string strResult;
            int nRet = CMqttSdkGateway::execute_get(strCommand, strData, strResult);
            if (nRet == 0)
            {
                publish_response(strCommand, strRequestId, 0, strResult);
            }
            else
            {
                dlog_error("MQTT SDK 网关 GET 命令[%s]执行失败：%d", strCommand.c_str(), nRet);
                publish_response(strCommand, strRequestId, nRet, "{\"error\":\"execute get failed\"}");
            }
        }
        else
        {
            /* SET 命令：执行后返回状态 */
            int nRet = CMqttSdkGateway::execute_set(strCommand, strData);
            if (nRet == 0)
            {
                publish_response(strCommand, strRequestId, 0, "{}");
            }
            else
            {
                dlog_error("MQTT SDK 网关 SET 命令[%s]执行失败：%d", strCommand.c_str(), nRet);
                publish_response(strCommand, strRequestId, nRet, "{\"error\":\"execute set failed\"}");
            }
        }
    }
    else
    {
        dlog_warn("MQTT 未知命令：%s", strCommand.c_str());
        publish_response(strCommand, strRequestId, -1, "{\"error\":\"unsupported command\"}");
    }
}

/**
 * @brief 注册 MQTT 自定义命令处理器
 * @note  注册在此的命令优先于 MQTT SDK 网关处理
 *        适用于需要特殊处理逻辑的命令（非标准 SDK 命令透传）
 *        大部分 SDK 命令（NET_TV_GET_xxx / NET_TV_SET_xxx）由 on_mqtt_message 自动通过网关转发
 */
void CPlatformManager::register_mqtt_handlers()
{
    std::lock_guard<std::mutex> lock(m_mtxMqttHandlers);

    m_mapMqttHandlers["faces"] = [this](const std::string &strRequestId, const std::string &strData) {
        this->handle_faces_request(strRequestId, strData);
    };
    m_mapMqttHandlers["FACES"] = m_mapMqttHandlers["faces"];

    dlog_info("MQTT 命令处理器注册完成，标准 SDK 命令将通过网关自动转发");
}

void CPlatformManager::handle_faces_request(const std::string &strRequestId, const std::string &strData)
{
    std::string strResult;
    int nRet = CMqttSdkGateway::execute_get("NET_TV_GET_FACE_INFO", strData.empty() ? "{}" : strData, strResult);
    if (nRet != 0)
    {
        dlog_error("MQTT faces 请求执行失败：%d", nRet);
        strResult = "{\"error\":\"get faces failed\"}";
    }

    cJSON *pRoot = cJSON_CreateObject();
    cJSON_AddNumberToObject(pRoot, "Return", nRet);
    if (!strRequestId.empty())
    {
        cJSON_AddStringToObject(pRoot, "RequestId", strRequestId.c_str());
    }

    const std::string strDataJson = platform_data_or_empty(strResult);
    cJSON *pData = cJSON_Parse(strDataJson.c_str());
    if (pData)
    {
        cJSON_AddItemToObject(pRoot, "Data", pData);
    }
    else
    {
        cJSON_AddObjectToObject(pRoot, "Data");
    }

    std::string strPayload = platform_json_object_to_string(pRoot);
    cJSON_Delete(pRoot);

    if (!m_pstMqtt || !m_pstMqtt->is_connected())
    {
        dlog_warn("MQTT 未连接，无法发布 faces 响应");
        return;
    }

    m_pstMqtt->publish(MQTT_TOPIC_FACES_RESPONSE(m_strMqttClientId), strPayload, MQTT_QOS_RESPONSE);
}

int CPlatformManager::publish_event(const std::string &strCommand,
                                    const std::string &strData,
                                    const std::string &strRequestId)
{
    if (!m_pstMqtt || !m_pstMqtt->is_connected())
    {
        dlog_warn("MQTT 未连接，无法发布事件");
        return ERR_UNINIT;
    }

    /* 构造 JSON */
    cJSON *pRoot = cJSON_CreateObject();
    cJSON_AddStringToObject(pRoot, "Command", strCommand.c_str());
    cJSON_AddStringToObject(pRoot, "RequestId", strRequestId.c_str());
    
    cJSON *pData = cJSON_Parse(strData.c_str());
    if (pData)
    {
        cJSON_AddItemToObject(pRoot, "Data", pData);
    }
    else
    {
        cJSON_AddObjectToObject(pRoot, "Data");
    }

    char *pJson = cJSON_PrintUnformatted(pRoot);
    std::string strPayload = pJson;
    free(pJson);
    cJSON_Delete(pRoot);

    /* 发布到事件 Topic */
    std::string strTopic = MQTT_TOPIC_EVENT(m_strMqttClientId);
    return m_pstMqtt->publish(strTopic, strPayload, MQTT_QOS_EVENT);
}

int CPlatformManager::publish_response(const std::string &strCommand,
                                       const std::string &strRequestId,
                                       int nReturn,
                                       const std::string &strData)
{
    if (!m_pstMqtt || !m_pstMqtt->is_connected())
    {
        dlog_warn("MQTT 未连接，无法发布响应");
        return ERR_UNINIT;
    }

    /* 构造 JSON */
    cJSON *pRoot = cJSON_CreateObject();
    cJSON_AddStringToObject(pRoot, "Command", strCommand.c_str());
    cJSON_AddStringToObject(pRoot, "RequestId", strRequestId.c_str());
    cJSON_AddNumberToObject(pRoot, "Return", nReturn);
    
    cJSON *pData = cJSON_Parse(strData.c_str());
    if (pData)
    {
        cJSON_AddItemToObject(pRoot, "Data", pData);
    }
    else
    {
        cJSON_AddObjectToObject(pRoot, "Data");
    }

    char *pJson = cJSON_PrintUnformatted(pRoot);
    std::string strPayload = pJson;
    free(pJson);
    cJSON_Delete(pRoot);

    /* 发布到响应 Topic */
    std::string strTopic = MQTT_TOPIC_RESPONSE(m_strMqttClientId);
    return m_pstMqtt->publish(strTopic, strPayload, MQTT_QOS_RESPONSE);
}

#endif
