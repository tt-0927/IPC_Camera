/**
 * @FilePath     : gm.cpp
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2025-08-21 14:54:13
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-06-26 17:50:41
 * @Description  : 国密GB35114功能
 */
#include "gm.h"
#include "gm_cert_manage.h"
#include "time_utils.h"
#include "dlog.h"

using namespace SIP;

// info /*----------------------- GB35114 -----------------------*/

namespace
{

/**
 * @brief   : 检查 GB35114 证书是否被当前 CRL 吊销
 * @param    {std::string} &strCertPath 证书路径
 * @param    {char} *pszUsage 证书用途说明
 * @return   {int} OK：未吊销，ERR_CERT_FORMAT：已吊销或状态不可确认
 * @note    : CRL 未上传时不阻断；CRL 存在并命中证书序列号时拒绝继续使用该证书。
 */
int check_gb35114_cert_not_revoked(const std::string &strCertPath, const char *pszUsage)
{
    if (CGmCertManage::instance()->is_cert_revoked(strCertPath))
    {
        dlog_error("[GM]:%s证书已被 CRL 吊销或状态不可确认: %s", pszUsage, strCertPath.c_str());
        return ERR_CERT_FORMAT;
    }

    return OK;
}

}

void printSecurityInfo(const SecurityInfo &info)
{
    dlog_debug("------ SecurityInfo ------");
    dlog_debug("auth_type : %s", info.type.c_str());
    dlog_debug("algorithm : %s", info.algorithm.c_str());
    dlog_debug("random1   : %s", info.random1.c_str());
    dlog_debug("random2   : %s", info.random2.c_str());
    dlog_debug("device_id : %s", info.device_id.c_str());
    dlog_debug("server_id : %s", info.server_id.c_str());
    dlog_debug("crypt_key : %s", info.crypt_key.c_str());
    dlog_debug("sign2     : %s", info.sign2.c_str());
    dlog_debug("--------------------------");
}

void printNoteInfo(const NoteInfo_S &info)
{
    dlog_debug("------ NoteInfo ------");
    dlog_debug("note_type : %s", info.type.c_str());
    dlog_debug("algorithm : %s", info.algorithm.c_str());
    dlog_debug("nonce     : %s", info.nonce.c_str());
    dlog_debug("--------------------------");
}

template <typename T>
T parseHeaderInfo(const osip_header_t *pHeader)
{
    T info;
    if (!pHeader || !pHeader->hvalue)
        return info;

    std::string strHeader(pHeader->hvalue);

    /*提取类型名（在第一个空格前）*/
    size_t pos = strHeader.find(' ');
    if (pos != std::string::npos)
    {
        info.type = strHeader.substr(0, pos);
        strHeader = strHeader.substr(pos + 1); // 剩余部分继续处理
    }

    /*键值对提取*/
    std::regex kv_pattern(R"((\w+)=["]?([^"\s,]+)["]?)");
    std::smatch match;
    auto searchStart = strHeader.cbegin();

    while (std::regex_search(searchStart, strHeader.cend(), match, kv_pattern))
    {
        std::string key = match[1];
        std::string value = match[2];

        info.setField(key, value); /*动态设置字段*/

        searchStart = match.suffix().first;
    }

    return info;
}

void SIP::CGm::setGmEnable(bool bEnable)
{
    m_bGmEnable = bEnable;
}

bool SIP::CGm::isGmEnable() const
{
    return m_bGmEnable;
}

int SIP::CGm::load_vkek(std::string &strVkek) const
{
    std::vector<uint8_t> vecVkek;
    std::string strKeyVersion;
    const IpcRet_E enRet = CGb35114KeyStoreManager::instance()->load_vkek(vecVkek, strKeyVersion);
    if (enRet == OK)
    {
        strVkek.assign(vecVkek.begin(), vecVkek.end());
        return OK;
    }

    if (!m_strVkek.empty())
    {
        /* warn: 存储后端异常时允许使用内存缓存，避免已完成注册的会话立即中断 */
        strVkek = m_strVkek;
        return OK;
    }

    dlog_error("[GM]:读取 VKEK 失败, ret=%d", enRet);
    return enRet;
}

int SIP::CGm::gm_build_first_register(osip_message_t *pstRegister)
{
    /*国密功能是否启用*/
    if (!m_bGmEnable)
    {
        return OK;
    }

    if (pstRegister == nullptr)
    {
        dlog_error("[GM]:指针为空");
        return ERR_PTR_NULL;
    }

    /* 获取本地时间作为首次注册的密钥版本号，格式为 YYYY-MM-DDTHH:MM:SS。 */
    const std::string strKeyVersion = TimeUtils_NS::get_currentDateAndTime();
    if (strKeyVersion.empty())
    {
        dlog_error("[GM]:生成GB35114 keyversion失败");
        return ERR;
    }

    /*
     * ! oSIP扩展认证参数解析器要求逗号后保留空格；缺少空格会静默丢弃keyversion，
     *   导致平台不返回random1挑战字段。
     */
    const std::string strAuthorization = "Capability algorithm=\"A:SM2;H:SM3;S:SM1/OFB/PKCS5;SI:SM3-SM2\", keyversion=\"" +
                                         strKeyVersion + "\"";
    /*插入cnonce字段 用于证书交换*/
    // auto strCnonce = CGmSSL::instance()->readFileToString("/opt/cam/sm_cert/device/deviceCert.cer");
    // strCnonce = CGmSSL::instance()->base64Encode(strCnonce);
    // 移除base64加密后其中的换行符，避免传输时字段信息出错
    // strCnonce.erase(std::remove(strCnonce.begin(), strCnonce.end(), '\n'), strCnonce.end());
    // strAuthorization = strAuthorization + ",cnonce=\"devicecert:" + strCnonce + "\"";
    // dlog_debug("Authorization:%s", strAuthorization.c_str());

    /*osip添加GB35114 Authorization字段*/
    int nRet = osip_message_set_authorization(pstRegister, strAuthorization.c_str());
    if (nRet != OK)
    {
        dlog_error("[GM]:osip添加GB35114 Authorization字段失败");
        return ERR;
    }

    osip_authorization_t *pstAuthorization = nullptr;
    nRet = osip_message_get_authorization(pstRegister, 0, &pstAuthorization);
    if (nRet < 0 || pstAuthorization == nullptr || pstAuthorization->keyversion == nullptr)
    {
        /* ! 禁止发送缺失keyversion的首次REGISTER，避免平台返回不可用的401挑战。 */
        dlog_error("[GM]:oSIP解析GB35114 keyversion失败，取消首次注册");
        return ERR_PARSE;
    }
    return OK;
}

int SIP::CGm::gm_build_second_register(osip_message_t *pstResponse, osip_message_t *pstRegister, SipClientInfo_S &stClientInfo)
{
    /*国密功能是否启用*/
    if (!m_bGmEnable)
    {
        return OK;
    }

    if (pstResponse == nullptr || pstRegister == nullptr)
    {
        dlog_error("[GM]:指针为空");
        return ERR_PTR_NULL;
    }

    /*获取服务端401响应 www_authenticate 属性*/
    osip_www_authenticate_t *www_authenticate = nullptr;
    int nRet = osip_message_get_www_authenticate(pstResponse, 0, &www_authenticate);
    if (nRet < 0 || www_authenticate == nullptr)
    {
        dlog_error("[GM]:获取服务端401响应 www_authenticate 属性失败");
        return ERR_PARSE;
    }
    if (www_authenticate->auth_type == nullptr)
    {
        dlog_error("[GM]:401响应缺少认证类型");
        return ERR_PARSE;
    }

    /*判断是否为单向身份验证或者双向身份验证 Unidirection：单向，Bidirection：双向*/
    if (std::string(www_authenticate->auth_type) != "Unidirection" && std::string(www_authenticate->auth_type) != "Bidirection")
    {
        dlog_error("[GM]:身份验证不为单向身份验证或者双向身份验证");
        return ERR;
    }

    if (www_authenticate->random1 == nullptr)
    {
        /* ! GB35114 二次注册必须使用平台随机数，禁止对空指针执行 strlen 或指针运算。 */
        dlog_error("[GM]:401响应缺少GB35114必需的random1字段");
        return ERR_PARSE;
    }
    const size_t nRandom1Length = strlen(www_authenticate->random1);
    if (nRandom1Length < 3 || www_authenticate->random1[0] != '"' || www_authenticate->random1[nRandom1Length - 1] != '"')
    {
        dlog_error("[GM]:401响应中的random1格式非法");
        return ERR_PARSE;
    }

    /*base64 R1*/
    std::vector<uint8_t> bytesR1(www_authenticate->random1 + 1, www_authenticate->random1 + nRandom1Length - 1);

    /*生成 R2 并 base64 编码*/
    if (!CCryptoManager::instance()->is_ready())
    {
        dlog_error("[GM]:CCryptoManager 未就绪，无法生成随机数");
        return ERR;
    }
    std::string strR2 = CCryptoManager::instance()->rand_bytes(bytesR1.size() / 4 * 3);
    // std::string strR2 = "iwnxp23oq";  // 真实应使用随机生成的值
    std::vector<uint8_t> bytesR2 = CCryptoManager::instance()->base64_encode(strR2);

    /*ServerID 转字节流*/
    std::vector<uint8_t> bytesServerId(stClientInfo.stRemote.strID.begin(), stClientInfo.stRemote.strID.end());
    // std::vector<uint8_t> bytesServerId(g_stGb28181devinfo.achServerId, g_stGb28181devinfo.achServerId +
    // strlen(g_stGb28181devinfo.achServerId));

    /*拼接待签名数据：R2 + R1 + ServerID*/
    std::vector<uint8_t> bytesDataToSign;
    bytesDataToSign.reserve(bytesR2.size() + bytesR1.size() + bytesServerId.size());
    bytesDataToSign.insert(bytesDataToSign.end(), bytesR2.begin(), bytesR2.end());
    bytesDataToSign.insert(bytesDataToSign.end(), bytesR1.begin(), bytesR1.end());
    bytesDataToSign.insert(bytesDataToSign.end(), bytesServerId.begin(), bytesServerId.end());

    /*对拼接待签名数据进行签名*/
    std::string strSignature;
    const std::string strDeviceKeyPath = CGb35114KeyStoreManager::instance()->device_sm2_key_path();
    if (strDeviceKeyPath.empty())
    {
        dlog_error("[GM]:设备 SM2 私钥路径为空");
        return ERR;
    }
    nRet = check_gb35114_cert_not_revoked(GM_CA_DEVICE_CERT, "本地设备");
    if (nRet != OK)
    {
        return nRet;
    }
    nRet = CCryptoManager::instance()->digital_signature(bytesDataToSign, strSignature, strDeviceKeyPath);
    if (nRet != OK)
    {
        dlog_error("[GM]:对拼接待签名数据进行签名失败");
        return ERR;
    }

    /*定义 Authorization 字段携带数据*/
    std::string strRandom1 = "random1=" + std::string(www_authenticate->random1) + ",";
    std::string strRandom2 = "random2=\"" + std::string(bytesR2.begin(), bytesR2.end()) + "\",";
    std::string strDeviceId = "deviceid=\"" + stClientInfo.stLocal.strID + "\",";
    std::string strServerId = "serverid=\"" + stClientInfo.stRemote.strID + "\",";
    // std::string strDeviceId = "deviceid=\"" + std::string(g_stGb28181devinfo.achIpcId) + "\",";
    // std::string strServerId = "serverid=\"" + std::string(g_stGb28181devinfo.achServerId) + "\",";
    std::string strSign = "sign1=\"" + strSignature + "\",";
    std::string strAlgorithm = "algorithm=\"A:SM2;H:SM3;S:SM1/OFB/PKCS5;SI:SM3-SM2\"";

    /*Authorization 字段拼接*/
    std::ostringstream ossAuthorization;

    if (std::string(www_authenticate->auth_type) == "Bidirection")
    {
        ossAuthorization << www_authenticate->auth_type << " " << strRandom1 << strRandom2 << strDeviceId << strServerId
                         << strSign << strAlgorithm;
        // strAuthorization = std::string(www_authenticate->auth_type) +
        // 						   " " + strRandom1 + strRandom2 + strServerId + strSign + strAlgorithm;
    }
    else
    {
        ossAuthorization << www_authenticate->auth_type << " " << strRandom1 << strRandom2 << strServerId << strSign
                         << strAlgorithm;
    }
    /*osip添加GB35114 Authorization字段*/
    nRet = osip_message_set_authorization(pstRegister, ossAuthorization.str().c_str());
    if (nRet != OK)
    {
        dlog_error("[GM]:osip添加GB35114 Authorization字段失败");
        return ERR;
    }
    // gb_eXosip_printMessage(pstRegister);
    return OK;
}

int SIP::CGm::gm_analyzing_second_response(eXosip_event_t *pstEvent)
{
    /*国密功能是否启用*/
    if (!m_bGmEnable)
    {
        return OK;
    }

    if (pstEvent == nullptr)
    {
        dlog_error("[GM]:指针为空");
        return ERR_PTR_NULL;
    }
    if (!CCryptoManager::instance()->is_ready() || !CGb35114KeyStoreManager::instance()->is_ready())
    {
        dlog_error("[GM]:密码模块未就绪，无法解析注册响应");
        return ERR_UNINIT;
    }

    /*获取服务端200 OK响应 SecurityInfo 属性*/
    osip_header_t *pSecurityInfoHeader = nullptr;
    osip_message_header_get_byname(pstEvent->response, "securityinfo", 0, &pSecurityInfoHeader);
    if (pSecurityInfoHeader == nullptr || pSecurityInfoHeader->hvalue == nullptr)
    {
        dlog_error("[GM]:获取 SecurityInfo 字段失败");
        return ERR_PARSE;
    }

    SecurityInfo_S stSecurityInfo = parseHeaderInfo<SecurityInfo_S>(pSecurityInfoHeader);
    // printSecurityInfo(stSecurityInfo);
    if (stSecurityInfo.crypt_key.empty())
    {
        dlog_error("[GM]:SecurityInfo cryptkey 为空");
        return ERR_PARSE;
    }

    /*解密cryptkey*/
    std::vector<uint8_t> bytesCryptkey = CCryptoManager::instance()->base64_decode(stSecurityInfo.crypt_key);
    const std::string strDeviceKeyPath = CGb35114KeyStoreManager::instance()->device_sm2_key_path();
    if (strDeviceKeyPath.empty())
    {
        dlog_error("[GM]:设备 SM2 私钥路径为空");
        return ERR;
    }
    int nRet = check_gb35114_cert_not_revoked(GM_CA_DEVICE_CERT, "本地设备");
    if (nRet != OK)
    {
        return nRet;
    }
    m_strVkek = CCryptoManager::instance()->sm2_decrypt(bytesCryptkey, strDeviceKeyPath);
    if (m_strVkek.empty())
    {
        dlog_error("[GM]:解密 VKEK 失败");
        return ERR;
    }

    const std::vector<uint8_t> vecVkek(m_strVkek.begin(), m_strVkek.end());
    const IpcRet_E enStoreRet = CGb35114KeyStoreManager::instance()->store_vkek(vecVkek, TimeUtils_NS::get_currentDateAndTime());
    if (enStoreRet != OK)
    {
        dlog_error("[GM]:保存 VKEK 失败, ret=%d", enStoreRet);
        return enStoreRet;
    }
    // dlog_debug("strVkek:%s", m_strVkek.c_str());

    /*判断是否为单向身份验证或者双向身份验证 Unidirection：单向，Bidirection：双向*/
    if (stSecurityInfo.type == "Unidirection")
    {
    }
    else if (stSecurityInfo.type == "Bidirection")
    {
        /*解密sign2*/
        std::vector<uint8_t> bytesSign2 = CCryptoManager::instance()->base64_decode(stSecurityInfo.sign2);
        /*拼接待验签数据：R1 + R2 + DeviceId + cryptkey*/
        std::vector<uint8_t> bytesVerifySign;
        bytesVerifySign.reserve(stSecurityInfo.random1.size() + stSecurityInfo.random2.size() + stSecurityInfo.device_id.size() +
                                stSecurityInfo.crypt_key.size());
        bytesVerifySign.insert(bytesVerifySign.end(), stSecurityInfo.random1.begin(), stSecurityInfo.random1.end());
        bytesVerifySign.insert(bytesVerifySign.end(), stSecurityInfo.random2.begin(), stSecurityInfo.random2.end());
        bytesVerifySign.insert(bytesVerifySign.end(), stSecurityInfo.device_id.begin(), stSecurityInfo.device_id.end());
        bytesVerifySign.insert(bytesVerifySign.end(), stSecurityInfo.crypt_key.begin(), stSecurityInfo.crypt_key.end());

        /*对拼接待验签数据进行验签*/
        nRet = check_gb35114_cert_not_revoked(GM_PLATFORM_CERT, "平台");
        if (nRet != OK)
        {
            return nRet;
        }
        if (CCryptoManager::instance()->verify_signature(bytesVerifySign, bytesSign2, std::string(GM_PLATFORM_CERT)) != OK)
        {
            dlog_error("[GM]:SIP服务器签名无效");
            // eXosip_lock(m_pSipContext);
            // eXosip_call_send_answer(m_pSipContext,pstEvent->cid,403,NULL);
            // eXosip_unlock(m_pSipContext);
            return ERR;
        }
    }
    else
    {
        dlog_error("[GM]:身份验证不为单向身份验证或者双向身份验证");
        return ERR;
    }

    return OK;
}

int SIP::CGm::gm_analyzing_control_signaling(eXosip_event_t *pstEvent)
{
    /*国密功能是否启用*/
    if (!m_bGmEnable)
    {
        return OK;
    }

    if (pstEvent == nullptr)
    {
        dlog_error("[GM]:指针为空");
        return ERR_PTR_NULL;
    }

    /*获取 Note 字段*/
    osip_header_t *pNoteHeader = nullptr;
    osip_message_header_get_byname(pstEvent->request, "note", 0, &pNoteHeader);
    NoteInfo_S stNoteInfo = parseHeaderInfo<NoteInfo_S>(pNoteHeader);
    if (stNoteInfo.type != "Digest")
    {
        dlog_error("[GM]:控制信令非摘要处理");
        return ERR;
    }
    // printNoteInfo(stNoteInfo);
    /*获取 消息体 xml 字段*/
    osip_body_t *pstRqtBody = NULL;
    osip_message_get_body(pstEvent->request, 0, &pstRqtBody); /*获取接收到请求的XML消息体*/
    if (NULL == pstRqtBody)
    {
        dlog_error("[GM]:获取接收到请求的XML消息体为空!");
        return ERR_PARSE;
    }
    std::vector<uint8_t> bytesXmlBody(pstRqtBody->body, pstRqtBody->body + strlen(pstRqtBody->body));

    /*获取 METHOD 字段*/
    std::vector<uint8_t> bytesMETHOD(pstEvent->request->sip_method,
                                     pstEvent->request->sip_method + strlen(pstEvent->request->sip_method));
    /*获取 From 字段*/
    char *pFrom = nullptr;
    osip_from_to_str(pstEvent->request->from, &pFrom);
    std::vector<uint8_t> bytesFrom(pFrom, pFrom + strlen(pFrom));
    /*获取 To 字段*/
    char *pTo = nullptr;
    osip_to_to_str(pstEvent->request->to, &pTo);
    std::vector<uint8_t> bytesTo(pTo, pTo + strlen(pTo));
    /*获取 Call-ID 字段*/
    char *pCallId = nullptr;
    osip_call_id_to_str(pstEvent->request->call_id, &pCallId);
    std::vector<uint8_t> bytesCallId(pCallId, pCallId + strlen(pCallId));
    /*获取 Date 字段*/
    osip_header_t *pDateHeader = nullptr;
    osip_message_header_get_byname(pstEvent->request, "date", 0, &pDateHeader);
    if (pDateHeader == nullptr || pDateHeader->hvalue == nullptr)
    {
        dlog_error("[GM]:获取 Date 字段失败");
        return ERR_PARSE;
    }
    std::vector<uint8_t> bytesDate(pDateHeader->hvalue, pDateHeader->hvalue + strlen(pDateHeader->hvalue));

    std::string strVkek;
    int nRet = load_vkek(strVkek);
    if (nRet != OK)
    {
        return nRet;
    }
    nRet = check_gb35114_cert_not_revoked(GM_CA_DEVICE_CERT, "本地设备");
    if (nRet != OK)
    {
        return nRet;
    }

    /*拼接待杂凑数据：METHOD + From + To + Call-ID + Date + VKEK + 消息体内容*/
    std::vector<uint8_t> bytesSm3Input;
    bytesSm3Input.reserve(bytesMETHOD.size() + bytesFrom.size() + bytesTo.size() + bytesCallId.size() + bytesDate.size() +
                          strVkek.size() + bytesXmlBody.size());
    bytesSm3Input.insert(bytesSm3Input.end(), bytesMETHOD.begin(), bytesMETHOD.end());
    bytesSm3Input.insert(bytesSm3Input.end(), bytesFrom.begin(), bytesFrom.end());
    bytesSm3Input.insert(bytesSm3Input.end(), bytesTo.begin(), bytesTo.end());
    bytesSm3Input.insert(bytesSm3Input.end(), bytesCallId.begin(), bytesCallId.end());
    bytesSm3Input.insert(bytesSm3Input.end(), bytesDate.begin(), bytesDate.end());
    bytesSm3Input.insert(bytesSm3Input.end(), strVkek.begin(), strVkek.end());
    bytesSm3Input.insert(bytesSm3Input.end(), bytesXmlBody.begin(), bytesXmlBody.end());

    std::vector<uint8_t> bytesSm3 = CCryptoManager::instance()->sm3_hash(bytesSm3Input);
    std::vector<uint8_t> bytesNonce = CCryptoManager::instance()->base64_decode(stNoteInfo.nonce);
    if (bytesSm3 == bytesNonce)
    {
        dlog_trace("[GM]:控制信令认证成功");
    }
    else
    {
        dlog_error("[GM]:控制信令认证失败");
        return ERR;
    }
    // dlog_debug("bytesSm3:\n%s", CGmSSL::instance()->vectorToHexString(bytesSm3).c_str());
    // dlog_debug("bytesNonce:\n%s", CGmSSL::instance()->vectorToHexString(bytesNonce).c_str());

    return OK;
}

int SIP::CGm::gm_build_control_signaling_note(osip_message_t *pControl, const char *aBody)
{
    /*国密功能是否启用*/
    if (!m_bGmEnable)
    {
        return OK;
    }

    if (pControl == nullptr || aBody == nullptr)
    {
        dlog_error("[GM]:指针为空");
        return ERR_PTR_NULL;
    }

    /*获取 METHOD 字段*/
    std::vector<uint8_t> bytesMETHOD(pControl->sip_method, pControl->sip_method + strlen(pControl->sip_method));
    /*获取 From 字段*/
    char *pFrom = nullptr;
    osip_from_to_str(pControl->from, &pFrom);
    std::vector<uint8_t> bytesFrom(pFrom, pFrom + strlen(pFrom));
    /*获取 To 字段*/
    char *pTo = nullptr;
    osip_to_to_str(pControl->to, &pTo);
    std::vector<uint8_t> bytesTo(pTo, pTo + strlen(pTo));
    /*获取 Call-ID 字段*/
    char *pCallId = nullptr;
    osip_call_id_to_str(pControl->call_id, &pCallId);
    std::vector<uint8_t> bytesCallId(pCallId, pCallId + strlen(pCallId));
    /*获取本地时间、密钥版本号*/
    // char aDate[64];
    std::string strLocalDate = TimeUtils_NS::get_currentDateAndTime();
    // get_time_T_char(aDate, sizeof(aDate));
    /*获取 Date 字段*/
    // std::vector<uint8_t> bytesDate(aDate, aDate + strlen(aDate));
    std::vector<uint8_t> bytesDate(strLocalDate.begin(), strLocalDate.end());
    /*获取 消息体 xml 字段*/
    std::vector<uint8_t> bytesXmlBody(aBody, aBody + strlen(aBody));

    std::string strVkek;
    int nRet = load_vkek(strVkek);
    if (nRet != OK)
    {
        return nRet;
    }
    nRet = check_gb35114_cert_not_revoked(GM_CA_DEVICE_CERT, "本地设备");
    if (nRet != OK)
    {
        return nRet;
    }

    /*拼接待杂凑数据：METHOD + From + To + Call-ID + Date + VKEK + 消息体内容*/
    std::vector<uint8_t> bytesSm3Input;
    bytesSm3Input.reserve(bytesMETHOD.size() + bytesFrom.size() + bytesTo.size() + bytesCallId.size() + bytesDate.size() +
                          strVkek.size() + bytesXmlBody.size());
    bytesSm3Input.insert(bytesSm3Input.end(), bytesMETHOD.begin(), bytesMETHOD.end());
    bytesSm3Input.insert(bytesSm3Input.end(), bytesFrom.begin(), bytesFrom.end());
    bytesSm3Input.insert(bytesSm3Input.end(), bytesTo.begin(), bytesTo.end());
    bytesSm3Input.insert(bytesSm3Input.end(), bytesCallId.begin(), bytesCallId.end());
    bytesSm3Input.insert(bytesSm3Input.end(), bytesDate.begin(), bytesDate.end());
    bytesSm3Input.insert(bytesSm3Input.end(), strVkek.begin(), strVkek.end());
    bytesSm3Input.insert(bytesSm3Input.end(), bytesXmlBody.begin(), bytesXmlBody.end());
    /*生成 nonce*/
    std::vector<uint8_t> bytesSm3 = CCryptoManager::instance()->sm3_hash(bytesSm3Input);
    std::string strSm3Base64 = CCryptoManager::instance()->base64_encode_to_string(bytesSm3);
    /*生成 Note 字段*/
    std::ostringstream ossNote;
    ossNote << "Digest nonce=\"" << strSm3Base64 << "\"" << ",algorithm=\"SM3\"";

    /*osip添加GB35114 Date 字段*/
    osip_message_set_date(pControl, strLocalDate.c_str());
    // osip_message_set_date(pControl, aDate);
    /*osip添加GB35114 Note 字段*/
    osip_message_set_header(pControl, (const char *) "Note", ossNote.str().c_str());

    return OK;
}
// info /*--------------------- GB35114 END ---------------------*/
