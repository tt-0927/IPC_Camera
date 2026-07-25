/**
 * @FilePath     : gm.h
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2025-08-21 14:54:16
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-06-26 17:50:53
 * @Description  : 国密GB35114功能
 */

#pragma once

#include <regex>

#include "SipType.h"
#include "path_define.h"
#include "common_define.h"
#include "IpcRet.h"
#include "crypto_manager.h"
#include "gb35114_keystore.h"

/*国标35114规定随机数字节数默认为16字节*/
#define GB35114_RANDOM_NUMBER_OF_BYTES_DEFAULT (16)

namespace SIP
{
/* 安全信息字段结构体 */
typedef struct SecurityInfo
{
    std::string type;      /* 认证类型：Unidirection（单向认证）或 Bidirection（双向认证） */
    std::string algorithm; /* 加密/签名算法名称，例如 SM2、RSA、SHA256 等 */
    std::string random1;   /* 认证随机数1：由服务端生成，用于挑战应答 */
    std::string random2;   /* 认证随机数2：由客户端（设备）生成，用于挑战应答 */
    std::string device_id; /* 设备ID：即客户端的SIP编号，通常为设备国标ID */
    std::string server_id; /* 服务端ID：即服务器的SIP编号，通常为平台国标ID */
    std::string crypt_key; /* VKEK加密后数据。加密密钥或加密后的对称密钥：用于加密后续通信数据或进行验证 */
    std::string sign2; /* 服务端签名数据：使用服务端私钥对随机数和其他字段签名，供客户端验证 */

    /**
     * @brief       : 设置字段
     * @author      : zhouzirui
     * @param        {string} &key：键
     * @param        {string} &value：键值
     * @return       {*}
     */
    void setField(const std::string &key, const std::string &value)
    {
        if (key == "algorithm")
            algorithm = value;
        else if (key == "random1")
            random1 = value;
        else if (key == "random2")
            random2 = value;
        else if (key == "deviceid")
            device_id = value;
        else if (key == "serverid")
            server_id = value;
        else if (key == "cryptkey")
            crypt_key = value;
        else if (key == "sign2")
            sign2 = value;
    }
} SecurityInfo_S;

/* 注释字段结构体 */
typedef struct NoteInfo
{
    std::string type;      /* 注释类型：Digest（摘要） */
    std::string algorithm; /* 加密/签名算法名称，例如 SM3等 */
    std::string nonce;     /* 临时杂凑计数经base64编码后的值 METHOD + From字段值 + To字段值 +
                              Call-ID字段值 + Date字段值 + VKEK + 消息体内容*/

    /**
     * @brief       : 设置字段
     * @param        {string} &key：键
     * @param        {string} &value：键值
     * @return       {*}
     */
    void setField(const std::string &key, const std::string &value)
    {
        if (key == "algorithm")
            algorithm = value;
        else if (key == "nonce")
            nonce = value;
    }
} NoteInfo_S;

class CGm : public CSingleton<CGm>
{
public:
    CGm() = default;
    ~CGm() = default;
    friend class CSingleton<CGm>;

    /**
     * @brief   : 设置国密35114使能状态
     * @param    {bool} bEnable true：使能 false：不使能
     */
    void setGmEnable(bool bEnable);

    /**
     * @brief   : 获取国密35114使能状态
     * @return  {bool} true：使能，false：未使能
     */
    bool isGmEnable() const;

    /**
     * @brief       : 构建gb35114第一次注册authorization授权字段
     * @author      : zhouzirui
     * @param        {osip_message_t} *pstRegister：指向用于发送SIP消息请求的结构体
     * @return       {*}0：成功，非零：失败
     * @note		: 添加GB35114 Authorization字段，携带参数algorithm、keyversion
     */
    int gm_build_first_register(osip_message_t *pstRegister);

    /**
     * @brief       : 构建gb35114第二次注册authorization授权字段
     * @author      : zhouzirui
     * @param        {osip_message_t} *pstResponse：指向接收到最新的SIP消息响应的结构体
     * @param        {osip_message_t} *pstRegister：指向用于发送SIP消息请求的结构体
     * @param        {SipClientInfo_S} &stClientInfo：客户端信息
     * @return       {*}0：成功，非零：失败
     * @note		: 添加GB35114
     * Authorization字段，携带参数random1、random2、serverid、sign1、algorithm
     */
    int gm_build_second_register(osip_message_t *pstResponse, osip_message_t *pstRegister, SipClientInfo_S &stClientInfo);

    /**
     * @brief       : gb35114注册请求的服务端二次响应分析,分析GB35114 Securityinfo字段
     * @author      : zhouzirui
     * @param        {eXosip_event_t} *pstEvent：指向事件描述结构体
     * @return       {*}0：成功，非零：失败
     * @note		: 单向认证：携带参数 cryptkey、algorithm
     * @note		: 双向认证：携带参数 random1、random2、deviceid、crytkey、sign2、algorithm
     */
    int gm_analyzing_second_response(eXosip_event_t *pstEvent);

    /**
     * @brief       : gb35114控制信令响应分析,分析GB35114 Date、Note字段
     * @author      : zhouzirui
     * @param        {eXosip_event_t} *pstEvent：指向事件描述结构体
     * @return       {*}0：成功，非零：失败
     * @note		：杂凑值是否相同
     */
    int gm_analyzing_control_signaling(eXosip_event_t *pstEvent);

    /**
     * @brief       : gb35114构建控制信令 Date、Note字段
     * @author      : zhouzirui
     * @param        {osip_message_t} *pControl：指向SIP消息结构体
     * @param        {char} *aBody：xml消息体
     * @return       {*}0：成功，非零：失败
     */
    int gm_build_control_signaling_note(osip_message_t *pControl, const char *aBody);

private:
    /**
     * @brief   : 读取当前 GB35114 VKEK
     * @param    {std::string} &strVkek：VKEK 输出
     * @return   {int} OK：成功，非 OK：失败
     * @note    : 优先读取 KeyStore，内存缓存仅用于兼容历史流程或存储异常后的短期兜底。
     */
    int load_vkek(std::string &strVkek) const;

    /*国标35114 国密使能状态 true：开 false：关*/
    bool m_bGmEnable = true;
    /* VKEK密钥*/
    std::string m_strVkek;
};
}
