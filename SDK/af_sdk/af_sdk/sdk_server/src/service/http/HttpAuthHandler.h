/**
 * @file HttpAuthHandler.h
 * @author tianl (tianl@kfb.cn)
 * @date 2026-07-28
 * @LastEditors  : qinjt@kfb.cn
 * @LastEditTime : 2026-07-28
 *
 * @brief HttpAuthHandler 模块接口与类型定义
 * 功能说明：
 * 1. 声明 HttpAuthHandler 模块对外接口和数据类型
 * 2. 定义模块依赖的常量、回调或辅助类型
 * 3. 为调用方提供明确且稳定的编译期契约
 */
#pragma once
#include <string>
#include <map>
#include <ctime>
#include <unordered_map>
#include <mutex>
#include <tvsdkhttplib.h>
#include "Singleton.h"

namespace tvsdk{
/* 前向声明 */
struct DigestParams_S;
/*struct DigestAuthSession_S; */
/* 会话信息结构 */
struct DigestAuthSession_S {
    std::string realm;
    time_t last_active;
    std::string client_ip;
    uint32_t last_nc;
};

/**
 * @author tianl (tianl@kfb.cn)
 * @brief 认证类型枚举
 */
enum class AuthType_E
{
    NONE,       /* 无认证 */
    BASIC,      /* Basic 认证 */
    DIGEST      /* Digest 认证 */
};

/**
 * @author tianl (tianl@kfb.cn)
 * @brief HTTP  Authentication 处理器
 *
 * 处理 HTTP Digest Authentication and Basic Authentication
 */
class CHttpAuthHandler : public CSingleton<CHttpAuthHandler>
{

    CHttpAuthHandler();
public:

	~CHttpAuthHandler();
	friend class CSingleton<CHttpAuthHandler>;

     void set_auth_info(const std::string& realm,const std::string& user,const std::string& passwd);

	 /**
 * @author tianl (tianl@kfb.cn)
     * @brief 处理认证（自动检测认证类型）
     * @param req HTTP 请求
     * @param res HTTP 响应
     * @return true 认证成功, false 认证失败
     */
    bool handle_authentication(const httplib::Request& req, httplib::Response& res);

    /**
 * @author tianl (tianl@kfb.cn)
     * @brief 处理摘要认证
     * @param req HTTP 请求
     * @param res HTTP 响应
     * @return true 认证成功, false 认证失败
     */
    bool handle_digest_auth(const httplib::Request& req, httplib::Response& res);

	/**
 * @author tianl (tianl@kfb.cn)
	 * @brief 处理 HTTP Basic Authentication
	 * @param req HTTP 请求
	 * @param res HTTP 响应
	 * @return true 认证成功, false 认证失败
	 */
	bool handle_basic_auth(const httplib::Request& req, httplib::Response& res);

	 /**
 * @author tianl (tianl@kfb.cn)
     * @brief 检测认证类型
     * @param auth_header Authorization 头
     * @return 检测到的认证类型
     */
    AuthType_E detect_auth_type(const std::string& auth_header);

private:
    /* 私有方法 */
    void send_challenge(httplib::Response& res, const std::string& stale = "false");
	void send_basic_challenge(httplib::Response& res);
    std::string generate_nonce();
    std::string generate_opaque();
    bool parse_digest_header(const std::string& header, DigestParams_S& params);
    bool verify_digest_auth(const httplib::Request& req, const DigestParams_S& params);
    std::string calculate_digest_response(const std::string& username,
                                        const std::string& password,
                                        const std::string& method,
                                        const DigestParams_S& params);
    std::string md5_hex(const std::string& input);
	std::string base64_decode(const std::string& encoded);
    bool verify_session(const std::string& nonce);
    void save_session(const std::string& nonce, const std::string& opaque);
    void cleanup_expired_sessions();

private:
    std::string m_strRealm;
    std::map<std::string, std::string> user_passwords_; /* 用户名-密码映射 */
    std::map<std::string, DigestAuthSession_S> m_stSessions; /* nonce 会话存储 */
    /* std::unordered_map<std::string, DigestAuthSession_S> m_stSessions; nonce 会话存储。 */
    std::mutex m_stSessionsMutex;
	std::vector<AuthType_E> supported_auth_types_ = {AuthType_E::BASIC, AuthType_E::DIGEST};
};

/* Digest 认证参数结构 */
struct DigestParams_S {
    std::string username;
    std::string realm;
    std::string nonce;
    std::string uri;
    std::string response;
    std::string qop;
    std::string nc;    /* nonce count */
    std::string cnonce;/* client nonce */
    std::string opaque;
};



}/* namespace tvsdk */
