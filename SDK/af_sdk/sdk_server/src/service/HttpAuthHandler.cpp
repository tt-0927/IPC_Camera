/**
 * @file HttpAuthHandler.cpp
 * @author tianl (tianl@kfb.cn)
 * @date 2026-07-28
 * @LastEditors  : qinjt@kfb.cn
 * @LastEditTime : 2026-07-28
 *
 * @brief HttpAuthHandler 模块实现
 * 功能说明：
 * 1. 实现 HttpAuthHandler 模块核心逻辑
 * 2. 校验输入参数并管理模块资源生命周期
 * 3. 向上层提供可复用的 SDK 能力
 */
#include "HttpAuthHandler.h"
#include <random>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <openssl/md5.h>
#include <openssl/bio.h>
#include <openssl/evp.h>
#include <openssl/buffer.h>
#include "NetSdkLog.h"

/* #define NETSDK_HTTP_AUTH_DEBUG 1 */
#define NETSDK_HTTP_AUTH_DEBUG 0

using namespace tvsdk;

/**
 * @author tianl (tianl@kfb.cn)
 * @brief 获取本地当前时间的格式化字符串。
 * @return 格式为 YYYY-MM-DD HH:MM:SS 的本地时间字符串。
 */
static std::string GetCurrentTimeString() {
    auto now = std::chrono::system_clock::now();
    std::time_t time = std::chrono::system_clock::to_time_t(now);
    std::tm tm_buf;
#ifdef _WIN32
    localtime_s(&tm_buf, &time);
#else
    localtime_r(&time, &tm_buf);
#endif
    std::ostringstream oss;
    oss << std::put_time(&tm_buf, "%Y-%m-%d %H:%M:%S");
    return oss.str();
}

CHttpAuthHandler::CHttpAuthHandler()
{

}

CHttpAuthHandler::~CHttpAuthHandler()
{

}
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 set_auth_info 对应的处理。
 * @param [in] realm 函数处理参数。
 * @param [in] user 函数处理参数。
 * @param [in] passwd 函数处理参数。
 * @return 无返回值。
 */

void CHttpAuthHandler::set_auth_info(const std::string& realm,const std::string& user,const std::string& passwd)
{
    NETSDK_LOG_MESSAGE_DEBUG("设置HTTP鉴权信息 realm： %s user：%s passwd:%s", realm.c_str(),user.c_str(),passwd.c_str());
    user_passwords_.clear();
    user_passwords_[user] = passwd;
    m_strRealm = realm;
}
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 handle_authentication 定义的内部处理。
 * @param [in] req 函数处理参数。
 * @param [in,out] res 函数处理参数。
 * @return 返回该处理的状态或结果。
 */

bool CHttpAuthHandler::handle_authentication(const httplib::Request& req, httplib::Response& res)
{
    /*NETSDK_LOG_MESSAGE_DEBUG("开始处理认证请求，客户端 IP: %s", req.remote_addr.c_str()); */
    NETSDK_LOG_MESSAGE_DEBUG("[%s] 开始处理认证请求，客户端 IP: %s", GetCurrentTimeString().c_str(), req.remote_addr.c_str());

    /* 获取认证头 */
    auto auth_header = req.get_header_value("Authorization");

    /* 检测认证类型 */
    AuthType_E auth_type = detect_auth_type(auth_header);
    NETSDK_LOG_MESSAGE_DEBUG("检测到认证类型: %d, 客户端 IP: %s", static_cast<int>(auth_type), req.remote_addr.c_str());

    /* 根据认证类型进行处理 */
    switch (auth_type)
	{
        case AuthType_E::BASIC:
            NETSDK_LOG_MESSAGE_DEBUG("使用 Basic 认证，客户端 IP: %s", req.remote_addr.c_str());
            return handle_basic_auth(req, res);

        case AuthType_E::DIGEST:
            NETSDK_LOG_MESSAGE_DEBUG("使用 Digest 认证，客户端 IP: %s", req.remote_addr.c_str());
            return handle_digest_auth(req, res);
        default:
            NETSDK_LOG_MESSAGE_DEBUG("使用 Digest 认证，客户端 IP: %s", req.remote_addr.c_str());
            return handle_digest_auth(req, res);
    }
}
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 handle_digest_auth 定义的内部处理。
 * @param [in] req 函数处理参数。
 * @param [in,out] res 函数处理参数。
 * @return 返回该处理的状态或结果。
 */

bool CHttpAuthHandler::handle_digest_auth(const httplib::Request& req, httplib::Response& res)
{
    NETSDK_LOG_MESSAGE_DEBUG("收到认证请求，客户端 IP: %s", req.remote_addr.c_str());

    auto auth_header = req.get_header_value("Authorization");

    /* 检查是否包含认证头 */
    if (auth_header.empty())
	{
        NETSDK_LOG_MESSAGE_DEBUG("请求缺少 Authorization 头");
        send_challenge(res);
        return false;
    }

    if (auth_header.find("Digest ") == std::string::npos)
	{
        NETSDK_LOG_MESSAGE_DEBUG("Authorization 头不是 Digest 类型: %s", auth_header.substr(0, 20).c_str());
        send_challenge(res);
        return false;
    }

    /* 解析 Digest 参数 */
    DigestParams_S params;
    if (!parse_digest_header(auth_header, params))
	{
        NETSDK_LOG_MESSAGE_ERROR("解析 Digest 头失败");
        send_challenge(res);
        return false;
    }

    NETSDK_LOG_MESSAGE_DEBUG("解析 Digest 参数: 用户=%s, realm=%s, uri=%s",
               params.username.c_str(), params.realm.c_str(), params.uri.c_str());

    /* 验证认证参数 */
    if (verify_digest_auth(req, params))
	{
        NETSDK_LOG_MESSAGE_INFO("[%s] Digest 认证成功: 用户=%s, 客户端 IP=%s", GetCurrentTimeString().c_str(), params.username.c_str(), req.remote_addr.c_str());
        /*NETSDK_LOG_MESSAGE_INFO("Digest 认证成功: 用户=%s, 客户端 IP=%s", params.username.c_str(), req.remote_addr.c_str()); */
        return true;
    }
	else
	{
        NETSDK_LOG_MESSAGE_INFO("[%s] Digest 认证失败: 用户=%s, 客户端 IP=%s", GetCurrentTimeString().c_str(), params.username.c_str(), req.remote_addr.c_str());
        /*NETSDK_LOG_MESSAGE_WARN("Digest 认证失败: 用户=%s, 客户端 IP=%s", params.username.c_str(), req.remote_addr.c_str()); */
        send_challenge(res, "stale=true");
        return false; /* 认证失败 */
    }
}
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 handle_basic_auth 定义的内部处理。
 * @param [in] req 函数处理参数。
 * @param [in,out] res 函数处理参数。
 * @return 返回该处理的状态或结果。
 */

bool CHttpAuthHandler::handle_basic_auth(const httplib::Request& req, httplib::Response& res)
{
    try {
        auto auth_header = req.get_header_value("Authorization");

        /* 检查是否包含认证头 */
        if (auth_header.empty() || auth_header.find("Basic ") == std::string::npos)
		{
            NETSDK_LOG_MESSAGE_DEBUG("请求缺少 Basic Authorization 头，客户端 IP: %s", req.remote_addr.c_str());
            send_basic_challenge(res);
            return false;
        }

        std::string encoded = auth_header.substr(6); /* 移除 "Basic " */
        std::string decoded = base64_decode(encoded);

        size_t colon_pos = decoded.find(':');
        if (colon_pos == std::string::npos) {
            NETSDK_LOG_MESSAGE_ERROR("Basic认证格式错误，客户端 IP: %s", req.remote_addr.c_str());
            send_basic_challenge(res);
            return false;
        }

		std::string username = decoded.substr(0, colon_pos);
        std::string password = decoded.substr(colon_pos + 1);
        /* 检查用户是否存在 */
        auto user_it = user_passwords_.find(username);
        if (user_it == user_passwords_.end())
        {
            NETSDK_LOG_MESSAGE_ERROR("Basic认证失败: 用户不存在=%s, 客户端 IP=%s", username.c_str(), req.remote_addr.c_str());
            send_basic_challenge(res);
            return false;
        }

        /* 验证密码 */
        bool authenticated = (password == user_it->second);
        if (!authenticated) {
            NETSDK_LOG_MESSAGE_ERROR("Basic认证失败: 密码错误，用户=%s, 客户端 IP=%s", username.c_str(), req.remote_addr.c_str());
            send_basic_challenge(res);
            return false;
        }

        NETSDK_LOG_MESSAGE_INFO("Basic认证成功: 用户=%s, 客户端 IP=%s", username.c_str(), req.remote_addr.c_str());
        return authenticated;

    } catch (const std::exception& e) {
        NETSDK_LOG_MESSAGE_ERROR("Basic认证解析异常: %s, 客户端 IP=%s", e.what(), req.remote_addr.c_str());
        send_basic_challenge(res);
        return false;
    }
}
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 detect_auth_type 定义的内部处理。
 * @param [in] auth_header 函数处理参数。
 * @return 返回该处理的状态或结果。
 */

AuthType_E CHttpAuthHandler::detect_auth_type(const std::string& auth_header)
{
    if (auth_header.empty())
	{
        return AuthType_E::NONE;
    }

    /* 检查是否支持 Basic 认证并且头部以 "Basic " 开头 */
    if (std::find(supported_auth_types_.begin(), supported_auth_types_.end(), AuthType_E::BASIC) != supported_auth_types_.end() &&
        auth_header.find("Basic ") == 0) {
        return AuthType_E::BASIC;
    }

    /* 检查是否支持 Digest 认证并且头部以 "Digest " 开头 */
    if (std::find(supported_auth_types_.begin(), supported_auth_types_.end(), AuthType_E::DIGEST) != supported_auth_types_.end() &&
        auth_header.find("Digest ") == 0) {
        return AuthType_E::DIGEST;
    }

    return AuthType_E::NONE;
}
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 send_challenge 对应的处理。
 * @param [in,out] res 函数处理参数。
 * @param [in] stale 函数处理参数。
 * @return 无返回值。
 */

void CHttpAuthHandler::send_challenge(httplib::Response& res, const std::string& stale)
{
    std::string nonce = generate_nonce();
    std::string opaque = generate_opaque();

    std::string challenge =
        "Digest realm=\"" + m_strRealm + "\", "
        "qop=\"auth\", "
        "nonce=\"" + nonce + "\", "
        "opaque=\"" + opaque + "\", "
        "algorithm=MD5, " +
        "stale=" + stale;

    res.status = 401;
    res.set_header("WWW-Authenticate", challenge);
    res.set_content("Authentication required", "text/plain");

    /* 保存会话信息 */
    save_session(nonce, opaque);

    NETSDK_LOG_MESSAGE_DEBUG("发送 401 质询响应: nonce=%s, opaque=%s", nonce.c_str(), opaque.c_str());
}
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 send_basic_challenge 对应的处理。
 * @param [in,out] res 函数处理参数。
 * @return 无返回值。
 */

void CHttpAuthHandler::send_basic_challenge(httplib::Response& res)
{
    std::string challenge = "Basic realm=\"" + m_strRealm + "\"";

    res.status = 401;
    res.set_header("WWW-Authenticate", challenge);
    res.set_content("Authentication required", "text/plain");

    NETSDK_LOG_MESSAGE_DEBUG("发送 Basic Authentication 质询");
}
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 generate_nonce 定义的内部处理。
 * @return 返回该处理的状态或结果。
 */

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

    /* 添加时间戳确保唯一性 */
    auto timestamp = std::to_string(std::time(nullptr));
    return nonce + timestamp;
}
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 generate_opaque 定义的内部处理。
 * @return 返回该处理的状态或结果。
 */

std::string CHttpAuthHandler::generate_opaque() {
    return generate_nonce(); /* 简单实现，实际应该更复杂 */
}
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 trim 定义的内部处理。
 * @param [in] s 函数处理参数。
 * @return 返回该处理的状态或结果。
 */

static std::string trim(const std::string& s) {
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
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 查询或校验 parse_digest_header 对应的数据。
 * @param [in] auth_header 函数处理参数。
 * @param [in,out] params 函数处理参数。
 * @return 返回该处理的状态或结果。
 */

bool CHttpAuthHandler::parse_digest_header(const std::string& auth_header, DigestParams_S& params)
{
    /* 1. 先去掉 "Digest " 前缀（确保输入是 Digest 格式） */
    if (auth_header.substr(0, 7) != "Digest ") {
        return false;
    }
    std::string digest_params = auth_header.substr(7);

    /* 2. 按逗号分割每个参数（如 "username=xxx", "realm=xxx", ...） */
    size_t pos = 0;
    while (pos < digest_params.size()) {
        /* 2.1 找到当前参数的结束位置（下一个逗号或字符串末尾） */
        size_t comma_pos = digest_params.find(',', pos);
        std::string param = digest_params.substr(pos, comma_pos - pos);
        pos = (comma_pos == std::string::npos) ? digest_params.size() : comma_pos + 1;

        /* 2.2 分割 param 为 key 和 value（按等号分割） */
        size_t eq_pos = param.find('=');
        if (eq_pos == std::string::npos)
        {
            continue; /* 无效参数，跳过 */
        }
        std::string key = trim(param.substr(0, eq_pos)); /* 去除 key 前后空格（关键修复） */
        std::string value_str = param.substr(eq_pos + 1);

        /* 2.3 提取 value（兼容带引号和无引号） */
        std::string value;
        if (value_str.size() >= 2 && value_str[0] == '"' && value_str.back() == '"') {
            /* 带引号：提取引号内的内容（如 "IPC" → IPC） */
            value = value_str.substr(1, value_str.size() - 2);
        } else {
            /* 无引号：直接取（如 00000001、auth） */
            value = trim(value_str); /* 去除 value 前后空格（如 nc= 00000001 → 00000001） */
        }

        /* 2.4 赋值给 params（key 已去空格，能精准匹配） */
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
            params.nc = value; /* 现在能正确匹配，不会为空 */
        } else if (key == "cnonce") {
            params.cnonce = value;
        }
    }

    /* 3. 检查关键参数是否存在（避免漏解析） */
    return !params.username.empty() && !params.realm.empty() && !params.nc.empty();
}
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 base64_decode 定义的内部处理。
 * @param [in] encoded 函数处理参数。
 * @return 返回该处理的状态或结果。
 */

std::string CHttpAuthHandler::base64_decode(const std::string& encoded)
{
    BIO *bio, *b64;
    char* buffer = static_cast<char*>(malloc(encoded.size()));
    memset(buffer, 0, encoded.size());

    bio = BIO_new_mem_buf(encoded.c_str(), -1);
    b64 = BIO_new(BIO_f_base64());
    bio = BIO_push(b64, bio);

    BIO_set_flags(bio, BIO_FLAGS_BASE64_NO_NL); /* 不处理换行 */
    int length = BIO_read(bio, buffer, encoded.size());

    std::string result(buffer, length);

    BIO_free_all(bio);
    free(buffer);

    return result;
}
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 查询或校验 verify_digest_auth 对应的数据。
 * @param [in] req 函数处理参数。
 * @param [in] params 函数处理参数。
 * @return 返回该处理的状态或结果。
 */

bool CHttpAuthHandler::verify_digest_auth(const httplib::Request& req, const DigestParams_S& params)
{
    /* 1. 检查用户是否存在 */
    auto user_it = user_passwords_.find(params.username);
    if (user_it == user_passwords_.end())
	{
        NETSDK_LOG_MESSAGE_ERROR("用户不存在: %s", params.username.c_str());
        return false;
    }

    /* 2. 验证 nonce 和会话 */
    if (!verify_session(params.nonce)) {
        NETSDK_LOG_MESSAGE_ERROR("无效的 nonce: %s", params.nonce.c_str());
        return false;
    }

    /* 3. 计算正确的 response */
    std::string expected_response = calculate_digest_response(
        params.username, user_it->second, req.method, params);

    /* 4. 比较 response */
    if (params.response != expected_response) {
        NETSDK_LOG_MESSAGE_ERROR("Response 不匹配. 期望: %s, 实际: %s",
                  expected_response.c_str(), params.response.c_str());
        return false;
    }

    /* 5. 验证 URI (可选) */
    if (params.uri != req.path) {
        NETSDK_LOG_MESSAGE_WARN("URI 不匹配. 期望: %s, 实际: %s", req.path.c_str(), params.uri.c_str());
        /* 在某些实现中这可能不是致命错误 */
    }

    return true;
}
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 calculate_digest_response 定义的内部处理。
 * @return 返回该处理的状态或结果。
 */

std::string CHttpAuthHandler::calculate_digest_response(const std::string& username,
                                    const std::string& password,
                                    const std::string& method,
                                    const DigestParams_S& params) {
#if NETSDK_HTTP_AUTH_DEBUG
    /* 1. 打印解析后的所有参数（确认无引号、无错误） */
    std::cout << "解析参数：" << std::endl;
    std::cout << "username: " << username << std::endl;
    std::cout << "realm: " << params.realm << std::endl;
    std::cout << "nonce: " << params.nonce << std::endl;
    std::cout << "nc: " << params.nc << std::endl;
    std::cout << "cnonce: " << params.cnonce << std::endl;
    std::cout << "qop: " << params.qop << std::endl;
    std::cout << "uri: " << params.uri << std::endl;
    std::cout << "method: " << method << std::endl;
#endif
    /* 2. 打印HA1、HA2、response_data（确认每一步正确） */
    std::string ha1_data = username + ":" + params.realm + ":" + password;
    std::string ha1 = md5_hex(ha1_data);
#if NETSDK_HTTP_AUTH_DEBUG
    std::cout << "HA1输入: " << ha1_data << std::endl;
    std::cout << "HA1结果: " << ha1 << std::endl;
#endif

    std::string ha2_data = method + ":" + params.uri;
    std::string ha2 = md5_hex(ha2_data);
#if NETSDK_HTTP_AUTH_DEBUG
    std::cout << "HA2输入: " << ha2_data << std::endl;
    std::cout << "HA2结果: " << ha2 << std::endl;
#endif

    std::string response_data = ha1 + ":" + params.nonce + ":" + params.nc + ":" + params.cnonce + ":" + params.qop + ":" + ha2;
    std::string result = md5_hex(response_data);
#if NETSDK_HTTP_AUTH_DEBUG
    std::cout << "response输入: " << response_data << std::endl;
    std::cout << "服务器端response: " << result << std::endl;
    std::cout << "客户端response: " << params.response << std::endl;
#endif

    return result;
}
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 md5_hex 定义的内部处理。
 * @param [in] input 函数处理参数。
 * @return 返回该处理的状态或结果。
 */

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
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 查询或校验 verify_session 对应的数据。
 * @param [in] nonce 函数处理参数。
 * @return 返回该处理的状态或结果。
 */

bool CHttpAuthHandler::verify_session(const std::string& nonce) {
    std::lock_guard<std::mutex> lock(m_stSessionsMutex);

    auto it = m_stSessions.find(nonce);
    if (it == m_stSessions.end()) {
        NETSDK_LOG_MESSAGE_DEBUG("nonce 不存在: %s", nonce.c_str());
        return false; /* nonce 不存在 */
    }

    /* 检查会话是否过期 (例如 5 分钟) */
    time_t now = std::time(nullptr);
    if (now - it->second.last_active > 300)
    { /* 5 分钟 */
        m_stSessions.erase(it);
        NETSDK_LOG_MESSAGE_DEBUG("nonce 已过期: %s", nonce.c_str());
        return false;
    }

    /* 更新最后活动时间 */
    it->second.last_active = now;
    NETSDK_LOG_MESSAGE_DEBUG("nonce 验证成功: %s", nonce.c_str());
    return true;
}

#define NETSDK_SESSION_CLEANUP_THRESHOLD 100
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 save_session 对应的处理。
 * @param [in] nonce 函数处理参数。
 * @param [in] opaque 函数处理参数。
 * @return 无返回值。
 */

void CHttpAuthHandler::save_session(const std::string& nonce, const std::string& opaque) {
    std::lock_guard<std::mutex> lock(m_stSessionsMutex);

    DigestAuthSession_S session;
    session.realm = m_strRealm;
    session.last_active = std::time(nullptr);

    m_stSessions[nonce] = session;

    NETSDK_LOG_MESSAGE_DEBUG("保存会话: nonce=%s, 当前会话数=%zu", nonce.c_str(), m_stSessions.size());


    if (m_stSessions.size() > NETSDK_SESSION_CLEANUP_THRESHOLD){
        cleanup_expired_sessions();
    }
}
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 cleanup_expired_sessions 定义的内部处理。
 * @return 无返回值。
 */

void CHttpAuthHandler::cleanup_expired_sessions()
{
    time_t now = std::time(nullptr);
    time_t expire_time = 300; /* 5 分钟 */

    size_t initial_size = m_stSessions.size();

    for (auto it = m_stSessions.begin(); it != m_stSessions.end(); ) {
        if (now - it->second.last_active > expire_time) {
            it = m_stSessions.erase(it);
        } else {
            ++it;
        }
    }

    if (m_stSessions.size() != initial_size) {
        NETSDK_LOG_MESSAGE_DEBUG("清理过期会话: 清理前=%zu, 清理后=%zu", initial_size, m_stSessions.size());
    }
}
