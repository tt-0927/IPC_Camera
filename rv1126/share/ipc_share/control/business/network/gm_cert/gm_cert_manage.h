/**
 * @FilePath     : gm_cert_manage.h
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2025-08-23 09:47:17
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-06-25 10:06:42
 * @Description  : 国密证书管理
 */

#pragma once

#include <sstream>

#include "Singleton.h"
#include "config_storage.h"
#include "network_define.h"
#include "IpcRet.h"
#include "crypto_manager.h"
#include "gb35114_keystore.h"
#include "gb35114_crypto_define.h"
#include "gm_cert_configure.h"

class CGmCertManage : public CSingleton<CGmCertManage>
{
    CGmCertManage();

public:
    ~CGmCertManage();
    friend class CSingleton<CGmCertManage>;

    /**
     * @brief   : 创建证书请求文件
     * @param    {GmCertNetworkType_E} enType 国密证书持有者网络类型
     * @return   {string} GM证书请求路径
     */
    std::string create_cert_request(Network::GmCertNetworkType_E enType);

    /**
     * @brief   : 证书吊销列表分析
     * @param    {string} strCrlPath 证书吊销列表路径
     * @param    {GmCrlFileInfo_S} &stInfo 国密CRL文件信息
     * @return   {int} 0：成功，非0：失败
     */
    int crl_parse(const std::string strCrlPath, Network::GmCrlFileInfo_S &stInfo);

    /**
     * @brief   : 验证 CRL 的合法性
     * @param    {string} &strCrlPath 证书吊销列表路径
     * @param    {string} &strCertPath 证书路径
     * @return   {bool} true：合法，false：不合法
     */
    bool crl_verify(const std::string &strCrlPath, const std::string &strCertPath);

    /**
     * @brief   : 使用签发者证书验证设备证书
     * @param    {string} &strCertPath 设备证书路径
     * @param    {string} &strCaCertPath 签发者证书路径
     * @return   {bool} true：验证成功，false：验证失败
     * @note    : 当前平台证书槽位不是 CA 根证书，调用前必须确认业务确实需要签发链校验。
     */
    bool cert_verify(const std::string &strCertPath, const std::string &strCaCertPath);

    /**
     * @brief   : 判断证书是否已被当前 CRL 吊销
     * @param    {string} &strCertPath 证书路径
     * @return   {bool} true：已吊销或证书状态不可确认，false：未吊销或未配置 CRL
     * @note    : 未上传 CRL 时不阻断业务；已上传 CRL 后，证书解析失败按不可确认处理。
     */
    bool is_cert_revoked(const std::string &strCertPath);

    /**
     * @brief   : 证书分析
     * @param    {string} strPath 证书路径
     * @param    {GmCertFileInfo_S} &stInfo 国密证书文件信息
     * @return   {int} 0：成功，非0：失败
     */
    int cert_parse(const std::string strPath, Network::GmCertFileInfo_S &stInfo);

private:
    /**
     * @brief   : 确保设备 SM2 私钥存在且由当前 Provider 生成
     * @param    {bool} bForceRegenerate：是否强制重新生成私钥
     * @return   {int} OK：成功，非 OK：失败
     * @note    : 强制重新生成时会先备份旧私钥，兼容历史 gmssl 生成私钥无法被 OpenSSL Provider 读取的场景。
     */
    int ensure_device_key(bool bForceRegenerate);

    /**
     * @brief   : 更新证书序号
     */
    void update_cert_num();

    /**
     * @brief   : 去除字符串前后空白字符
     * @param    {string&} str 需要处理的字符串
     * @return   {string} 结果字符串
     */
    std::string trim(const std::string &str);

    /**
     * @brief   : 将证书日期格式转换为标准格式
     * @param    {string&} dateStr 证书日期格式
     * @return   {string} 标准格式 YYYY-MM-DD HH:MM:SS
     * @note    输入格式: "Wed Aug 20 06:33:40 2025"
     * @note    输出格式: "2025-08-20 06:33:40"
     */
    std::string convert_date_format(const std::string &dateStr);

private:
    /* 国家代码 */
    const std::string m_strCountry = "CN";
    /* 省份或州名 */
    const std::string m_strState = "GuangDong";
    /* 城市或地区 */
    const std::string m_strLocality = "GuangZhou";
    /* 组织名称 */
    const std::string m_strOrganization = "itc";
    /* 组织单位 */
    const std::string m_strOrganizationUnit = "ShiJue2";
    /* 通用名称 格式为:设备ID、密码模块ID,ID间以"_”分隔,某个ID为空时值为“NULL” 例如：34020000001320000082_NULL */
    std::string m_strCommonName;
    /* 私钥文件密码 */
    const std::string m_strPassword = Gb35114Crypto_NS::SM2_PRIVATE_KEY_ENCRYPT_PASSWORD;
    /* GM证书请求路径文件 */
    const std::string m_strReqFile = GM_CA_REQ_CSR;
    /* 证书序号 */
    int m_nCertNum = 1;
};
