/**
 * @file httpAuthHandler.cpp
 * @author tianl (tianl@kfb.cn)
 * @date 2025-10-16
 * 
 * @brief 基于httplib的认证
 */
#include "httpAuthHandler.h"
#include <random>
#include <openssl/md5.h>
#include <openssl/bio.h>
#include <openssl/evp.h>
#include <openssl/buffer.h>
#include "dlog.h"

CHttpAuthHandler::CHttpAuthHandler() 
{

}


void CHttpAuthHandler::set_auth_info(const std::string& realm,const std::string& user,const std::string& passwd)
{
    user_passwords_.clear(); 
    user_passwords_[user] = passwd; 
    realm_ = realm;
}

bool CHttpAuthHandler::handle_authentication(const httplib::Request& req, httplib::Response& res) 
{
    dlog_debug("开始处理认证请求，客户端 IP: %s", req.remote_addr.c_str());
    
    // 获取认证头
    auto auth_header = req.get_header_value("Authorization");
    
    // 检测认证类型
    AuthType auth_type = detect_auth_type(auth_header);
    dlog_debug("检测到认证类型: %d, 客户端 IP: %s", static_cast<int>(auth_type), req.remote_addr.c_str());
    
    // 根据认证类型进行处理
    switch (auth_type) 
	{
        case AuthType::BASIC:
            dlog_debug("使用 Basic 认证，客户端 IP: %s", req.remote_addr.c_str());
            return handle_basic_auth(req, res);
            
        case AuthType::DIGEST:
            dlog_debug("使用 Digest 认证，客户端 IP: %s", req.remote_addr.c_str());
            return handle_digest_auth(req, res);
        default:
            dlog_debug("使用 Digest 认证，客户端 IP: %s", req.remote_addr.c_str());
            return handle_digest_auth(req, res);
    }
}

bool CHttpAuthHandler::handle_digest_auth(const httplib::Request& req, httplib::Response& res) 
{
    dlog_debug("收到认证请求，客户端 IP: %s", req.remote_addr.c_str());
    
    auto auth_header = req.get_header_value("Authorization");
    
    // 检查是否包含认证头
    if (auth_header.empty()) 
	{
        dlog_debug("请求缺少 Authorization 头");
        send_challenge(res);
        return false;
    }
    
    if (auth_header.find("Digest ") == std::string::npos) 
	{
        dlog_debug("Authorization 头不是 Digest 类型: %s", auth_header.substr(0, 20).c_str());
        send_challenge(res);
        return false;
    }
    
    // 解析 Digest 参数
    DigestParams params;
    if (!parse_digest_header(auth_header, params)) 
	{
        dlog_error("解析 Digest 头失败");
        send_challenge(res);
        return false;
    }
    
    dlog_debug("解析 Digest 参数: 用户=%s, realm=%s, uri=%s", 
               params.username.c_str(), params.realm.c_str(), params.uri.c_str());
    
    // 验证认证参数
    if (verify_digest_auth(req, params)) 
	{
        dlog_info("Digest 认证成功: 用户=%s, 客户端 IP=%s", 
                  params.username.c_str(), req.remote_addr.c_str());
        return true;
    }
	else 
	{
        dlog_warn("Digest 认证失败: 用户=%s, 客户端 IP=%s", 
                  params.username.c_str(), req.remote_addr.c_str());
        send_challenge(res, "stale=true");
        return false; // 认证失败
    }
}

bool CHttpAuthHandler::handle_basic_auth(const httplib::Request& req, httplib::Response& res) 
{
    try {
        auto auth_header = req.get_header_value("Authorization");
        
        // 检查是否包含认证头
        if (auth_header.empty() || auth_header.find("Basic ") == std::string::npos) 
		{
            dlog_debug("请求缺少 Basic Authorization 头，客户端 IP: %s", req.remote_addr.c_str());
            send_basic_challenge(res);
            return false;
        }
        
        std::string encoded = auth_header.substr(6); // 移除 "Basic "
        std::string decoded = base64_decode(encoded);
        
        size_t colon_pos = decoded.find(':');
        if (colon_pos == std::string::npos) {
            dlog_error("Basic认证格式错误，客户端 IP: %s", req.remote_addr.c_str());
            send_basic_challenge(res);
            return false;
        }

		std::string username = decoded.substr(0, colon_pos);
        std::string password = decoded.substr(colon_pos + 1);
        // 检查用户是否存在
        auto user_it = user_passwords_.find(username);
        if (user_it == user_passwords_.end()) {
            dlog_error("Basic认证失败: 用户不存在=%s, 客户端 IP=%s", username.c_str(), req.remote_addr.c_str());
            send_basic_challenge(res);
            return false;
        }
        
        // 验证密码
        bool authenticated = (password == user_it->second);
        if (!authenticated) {
            dlog_error("Basic认证失败: 密码错误，用户=%s, 客户端 IP=%s", username.c_str(), req.remote_addr.c_str());
            send_basic_challenge(res);
            return false;
        }
        
        dlog_info("Basic认证成功: 用户=%s, 客户端 IP=%s", username.c_str(), req.remote_addr.c_str());
        return authenticated;
        
    } catch (const std::exception& e) {
        dlog_error("Basic认证解析异常: %s, 客户端 IP=%s", e.what(), req.remote_addr.c_str());
        send_basic_challenge(res);
        return false;
    }
}

AuthType CHttpAuthHandler::detect_auth_type(const std::string& auth_header) 
{
    if (auth_header.empty()) 
	{
        return AuthType::NONE;
    }
    
    // 检查是否支持 Basic 认证并且头部以 "Basic " 开头
    if (std::find(supported_auth_types_.begin(), supported_auth_types_.end(), AuthType::BASIC) != supported_auth_types_.end() &&
        auth_header.find("Basic ") == 0) {
        return AuthType::BASIC;
    }
    
    // 检查是否支持 Digest 认证并且头部以 "Digest " 开头
    if (std::find(supported_auth_types_.begin(), supported_auth_types_.end(), AuthType::DIGEST) != supported_auth_types_.end() &&
        auth_header.find("Digest ") == 0) {
        return AuthType::DIGEST;
    }
    
    return AuthType::NONE;
}

void CHttpAuthHandler::send_challenge(httplib::Response& res, const std::string& stale) 
{
    std::string nonce = generate_nonce();
    std::string opaque = generate_opaque();
    
    std::string challenge = 
        "Digest realm=\"" + realm_ + "\", "
        "qop=\"auth\", "
        "nonce=\"" + nonce + "\", "
        "opaque=\"" + opaque + "\", "
        "algorithm=MD5, " +
        "stale=" + stale;
    
    res.status = 401;
    res.set_header("WWW-Authenticate", challenge);
    res.set_content("Authentication required", "text/plain");
    
    // 保存会话信息
    save_session(nonce, opaque);
    
    dlog_debug("发送 401 质询响应: nonce=%s, opaque=%s", nonce.c_str(), opaque.c_str());
}

void CHttpAuthHandler::send_basic_challenge(httplib::Response& res) 
{
    std::string challenge = "Basic realm=\"" + realm_ + "\"";
    
    res.status = 401;
    res.set_header("WWW-Authenticate", challenge);
    res.set_content("Authentication required", "text/plain");
    
    dlog_debug("发送 Basic Authentication 质询");
}

std::string CHttpAuthHandler::generate_nonce() {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, 15);
    
    const char* hex_chars = "0123456789abcdef";
    std::string nonce;
    nonce.reserve(32);
    
    for (int i = 0; i < 32; ++i) {
        nonce += hex_chars[dis(gen)];
    }
    
    // 添加时间戳确保唯一性
    auto timestamp = std::to_string(std::time(nullptr));
    return nonce + timestamp;
}

std::string CHttpAuthHandler::generate_opaque() {
    return generate_nonce(); // 简单实现，实际应该更复杂
}

std::string trim(const std::string& s) {
    auto start = s.begin();
    while (start != s.end() && std::isspace(static_cast<unsigned char>(*start))) {
        start++;
    }
    auto end = s.end();
    do {
        end--;
    } while (std::distance(start, end) > 0 && std::isspace(static_cast<unsigned char>(*end)));
    return std::string(start, end + 1);
}

bool CHttpAuthHandler::parse_digest_header(const std::string& auth_header, DigestParams& params) 
{
    // 1. 先去掉 "Digest " 前缀（确保输入是 Digest 格式）
    if (auth_header.substr(0, 7) != "Digest ") {
        return false;
    }
    std::string digest_params = auth_header.substr(7);

    // 2. 按逗号分割每个参数（如 "username=xxx", "realm=xxx", ...）
    size_t pos = 0;
    while (pos < digest_params.size()) {
        // 2.1 找到当前参数的结束位置（下一个逗号或字符串末尾）
        size_t comma_pos = digest_params.find(',', pos);
        std::string param = digest_params.substr(pos, comma_pos - pos);
        pos = (comma_pos == std::string::npos) ? digest_params.size() : comma_pos + 1;

        // 2.2 分割 param 为 key 和 value（按等号分割）
        size_t eq_pos = param.find('=');
        if (eq_pos == std::string::npos) {
            continue; // 无效参数，跳过
        }
        std::string key = trim(param.substr(0, eq_pos)); // 去除 key 前后空格（关键修复）
        std::string value_str = param.substr(eq_pos + 1);

        // 2.3 提取 value（兼容带引号和无引号）
        std::string value;
        if (value_str.size() >= 2 && value_str[0] == '"' && value_str.back() == '"') {
            // 带引号：提取引号内的内容（如 "IPC" → IPC）
            value = value_str.substr(1, value_str.size() - 2);
        } else {
            // 无引号：直接取（如 00000001、auth）
            value = trim(value_str); // 去除 value 前后空格（如 nc= 00000001 → 00000001）
        }

        // 2.4 赋值给 params（key 已去空格，能精准匹配）
        if (key == "username") {
            params.username = value;
        } else if (key == "realm") {
            params.realm = value;
        } else if (key == "nonce") {
            params.nonce = value;
        } else if (key == "uri") {
            params.uri = value;
        } else if (key == "response") {
            params.response = value;
        } else if (key == "qop") {
            params.qop = value;
        } else if (key == "nc") {
            params.nc = value; // 现在能正确匹配，不会为空
        } else if (key == "cnonce") {
            params.cnonce = value;
        }
    }

    // 3. 检查关键参数是否存在（避免漏解析）
    return !params.username.empty() && !params.realm.empty() && !params.nc.empty();
}

std::string CHttpAuthHandler::base64_decode(const std::string& encoded) 
{
    BIO *bio, *b64;
    char* buffer = static_cast<char*>(malloc(encoded.size()));
    memset(buffer, 0, encoded.size());
    
    bio = BIO_new_mem_buf(encoded.c_str(), -1);
    b64 = BIO_new(BIO_f_base64());
    bio = BIO_push(b64, bio);
    
    BIO_set_flags(bio, BIO_FLAGS_BASE64_NO_NL); // 不处理换行
    int length = BIO_read(bio, buffer, encoded.size());
    
    std::string result(buffer, length);
    
    BIO_free_all(bio);
    free(buffer);
    
    return result;
}

bool CHttpAuthHandler::verify_digest_auth(const httplib::Request& req, const DigestParams& params) 
{
    // 1. 检查用户是否存在
    auto user_it = user_passwords_.find(params.username);
    if (user_it == user_passwords_.end()) 
	{
        dlog_error("用户不存在: %s", params.username.c_str());
        return false;
    }
    
    // 2. 验证 nonce 和会话
    if (!verify_session(params.nonce)) {
        dlog_error("无效的 nonce: %s", params.nonce.c_str());
        return false;
    }
    
    // 3. 计算正确的 response
    std::string expected_response = calculate_digest_response(
        params.username, user_it->second, req.method, params);
    
    // 4. 比较 response
    if (params.response != expected_response) {
        dlog_error("Response 不匹配. 期望: %s, 实际: %s", 
                  expected_response.c_str(), params.response.c_str());
        return false;
    }
    
    // 5. 验证 URI (可选)
    if (params.uri != req.path) {
        dlog_warn("URI 不匹配. 期望: %s, 实际: %s", req.path.c_str(), params.uri.c_str());
        // 在某些实现中这可能不是致命错误
    }
    
    return true;
}

std::string CHttpAuthHandler::calculate_digest_response(const std::string& username, 
                                    const std::string& password,
                                    const std::string& method,
                                    const DigestParams& params) {
    // 1. 打印解析后的所有参数（确认无引号、无错误）
    std::cout << "解析参数：" << std::endl;
    std::cout << "username: " << username << std::endl;
    std::cout << "realm: " << params.realm << std::endl;
    std::cout << "nonce: " << params.nonce << std::endl;
    std::cout << "nc: " << params.nc << std::endl;
    std::cout << "cnonce: " << params.cnonce << std::endl;
    std::cout << "qop: " << params.qop << std::endl;
    std::cout << "uri: " << params.uri << std::endl;
    std::cout << "method: " << method << std::endl;

    // 2. 打印HA1、HA2、response_data（确认每一步正确）
    std::string ha1_data = username + ":" + params.realm + ":" + password;
    std::cout << "HA1输入: " << ha1_data << std::endl; // 如 "admin:IPC:admin@123"
    std::string ha1 = md5_hex(ha1_data);
    std::cout << "HA1结果: " << ha1 << std::endl;

    std::string ha2_data = method + ":" + params.uri;
    std::cout << "HA2输入: " << ha2_data << std::endl; // 如 "POST:/StartFirmwareUpgrade"
    std::string ha2 = md5_hex(ha2_data);
    std::cout << "HA2结果: " << ha2 << std::endl;

    std::string response_data = ha1 + ":" + params.nonce + ":" + params.nc + ":" + params.cnonce + ":" + params.qop + ":" + ha2;
    std::cout << "response输入: " << response_data << std::endl; // 完整拼接字符串
    std::string result = md5_hex(response_data);
    std::cout << "服务器端response: " << result << std::endl;
    std::cout << "客户端response: " << params.response << std::endl; // 客户端发送的response
    
    return result;
}

std::string CHttpAuthHandler::md5_hex(const std::string& input) {
    unsigned char digest[MD5_DIGEST_LENGTH];
    MD5(reinterpret_cast<const unsigned char*>(input.c_str()), 
        input.length(), digest);
    
    char md5_str[33];
    for (int i = 0; i < 16; i++) {
        sprintf(&md5_str[i*2], "%02x", (unsigned int)digest[i]);
    }
    md5_str[32] = '\0';
    
    return std::string(md5_str);
}

bool CHttpAuthHandler::verify_session(const std::string& nonce) {
    std::lock_guard<std::mutex> lock(sessions_mutex_);
    
    auto it = sessions_.find(nonce);
    if (it == sessions_.end()) {
        dlog_debug("nonce 不存在: %s", nonce.c_str());
        return false; // nonce 不存在
    }
    
    // 检查会话是否过期 (例如 5 分钟)
    time_t now = std::time(nullptr);
    if (now - it->second.last_active > 300) { // 5 分钟
        sessions_.erase(it);
        dlog_debug("nonce 已过期: %s", nonce.c_str());
        return false;
    }
    
    // 更新最后活动时间
    it->second.last_active = now;
    dlog_debug("nonce 验证成功: %s", nonce.c_str());
    return true;
}

void CHttpAuthHandler::save_session(const std::string& nonce, const std::string& opaque) {
    std::lock_guard<std::mutex> lock(sessions_mutex_);
    
    DigestAuthSession session;
    session.realm = realm_;
    session.last_active = std::time(nullptr);
    
    sessions_[nonce] = session;
    
    dlog_debug("保存会话: nonce=%s, 当前会话数=%zu", nonce.c_str(), sessions_.size());
    
    // 清理过期会话
    cleanup_expired_sessions();
}

void CHttpAuthHandler::cleanup_expired_sessions() {
    time_t now = std::time(nullptr);
    time_t expire_time = 300; // 5 分钟
    
    size_t initial_size = sessions_.size();
    
    for (auto it = sessions_.begin(); it != sessions_.end(); ) {
        if (now - it->second.last_active > expire_time) {
            it = sessions_.erase(it);
        } else {
            ++it;
        }
    }
    
    if (sessions_.size() != initial_size) {
        dlog_debug("清理过期会话: 清理前=%zu, 清理后=%zu", initial_size, sessions_.size());
    }
}