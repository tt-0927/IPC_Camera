/**
 * @FilePath     : gb35114_keystore.h
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2026-06-16
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-06-16
 * @Description  : GB35114 密钥安全存储抽象
 */

#pragma once

#include "IpcRet.h"
#include "Singleton.h"

#include <cstdint>
#include <string>
#include <vector>

/**
 * @brief   : GB35114 密钥安全存储 Provider 抽象接口
 * @note    : 该接口只负责“密钥在哪里、如何保存、如何取出”，不承载 SM2/SM3/SM4 算法实现。
 *            软件阶段落地为文件存储；后续海思、瑞芯微后端可替换为安全芯片、OTP、KeySlot 或 TEE 存储。
 */
class IGb35114KeyStore
{
public:
    virtual ~IGb35114KeyStore() = default;

    /**
     * @brief   : 初始化密钥存储后端
     * @return   {IpcRet_E} OK：成功，非 OK：失败
     */
    virtual IpcRet_E init() = 0;

    /**
     * @brief   : 去初始化密钥存储后端
     * @return   {IpcRet_E} OK：成功，非 OK：失败
     */
    virtual IpcRet_E deinit() = 0;

    /**
     * @brief   : 判断密钥存储后端是否可用
     * @return   {bool} true：可用，false：不可用
     */
    virtual bool is_ready() const = 0;

    /**
     * @brief   : 获取密钥存储后端名称
     * @return   {const char*} 后端名称
     */
    virtual const char *name() const = 0;

    /**
     * @brief   : 获取 GB35114 密码模块 ID
     * @return   {std::string} 22 位密码模块 ID；未配置时返回 NULL
     */
    virtual std::string crypto_module_id() const = 0;

    /**
     * @brief   : 保存 GB35114 密码模块 ID
     * @param    {std::string} strModuleId：22 位密码模块 ID；未配置时允许传入 NULL
     * @return   {IpcRet_E} OK：成功，ERR_PARAM/ERR：失败
     */
    virtual IpcRet_E store_crypto_module_id(const std::string &strModuleId) = 0;

    /**
     * @brief   : 确保设备 SM2 私钥存在
     * @param    {std::string} strPassword：私钥保护口令
     * @param    {bool} bForceRegenerate：是否强制重新生成
     * @return   {IpcRet_E} OK：成功，非 OK：失败
     */
    virtual IpcRet_E ensure_device_sm2_key(const std::string &strPassword, bool bForceRegenerate) = 0;

    /**
     * @brief   : 获取设备 SM2 私钥路径或硬件句柄描述
     * @return   {std::string} 软件后端返回 PEM 文件路径；硬件后端可返回 key handle 描述
     */
    virtual std::string device_sm2_key_path() const = 0;

    /**
     * @brief   : 保存平台下发的 VKEK
     * @param    {std::vector<uint8_t>} vecVkek：VKEK 原始字节
     * @param    {std::string} strKeyVersion：密钥版本或接收时间标识
     * @return   {IpcRet_E} OK：成功，非 OK：失败
     */
    virtual IpcRet_E store_vkek(const std::vector<uint8_t> &vecVkek, const std::string &strKeyVersion) = 0;

    /**
     * @brief   : 读取平台下发的 VKEK
     * @param    {std::vector<uint8_t>} &vecVkek：VKEK 原始字节输出
     * @param    {std::string} &strKeyVersion：密钥版本或接收时间标识输出
     * @return   {IpcRet_E} OK：成功，ERR_NOT_EXIST/ERR_PARSE/ERR：失败
     */
    virtual IpcRet_E load_vkek(std::vector<uint8_t> &vecVkek, std::string &strKeyVersion) = 0;

    /**
     * @brief   : 清除本地保存的 VKEK
     * @return   {IpcRet_E} OK：成功，非 OK：失败
     */
    virtual IpcRet_E clear_vkek() = 0;
};

/**
 * @brief   : GB35114 密钥存储统一门面
 * @note    : 共享层代码通过该门面访问设备私钥、密码模块 ID、VKEK，业务层负责注入具体后端。
 */
class CGb35114KeyStoreManager : public CSingleton<CGb35114KeyStoreManager>
{
public:
    CGb35114KeyStoreManager();
    ~CGb35114KeyStoreManager();
    friend class CSingleton<CGb35114KeyStoreManager>;

    /**
     * @brief   : 设置密钥存储后端
     * @param    {IGb35114KeyStore*} pStore：密钥存储后端指针
     * @return   {IpcRet_E} OK：成功，ERR_PARAM_NULL：失败
     */
    IpcRet_E set_store(IGb35114KeyStore *pStore);

    /**
     * @brief   : 初始化当前密钥存储后端
     * @return   {IpcRet_E} OK：成功，非 OK：失败
     */
    IpcRet_E init();

    /**
     * @brief   : 去初始化当前密钥存储后端
     * @return   {IpcRet_E} OK：成功
     */
    IpcRet_E deinit();

    /**
     * @brief   : 判断当前密钥存储后端是否可用
     * @return   {bool} true：可用，false：不可用
     */
    bool is_ready() const;

    /**
     * @brief   : 获取当前密钥存储后端名称
     * @return   {const char*} 后端名称
     */
    const char *name() const;

    /**
     * @brief   : 获取 GB35114 密码模块 ID
     * @return   {std::string} 22 位密码模块 ID；未配置时返回 NULL
     */
    std::string crypto_module_id() const;

    /**
     * @brief   : 保存 GB35114 密码模块 ID
     * @param    {std::string} strModuleId：22 位密码模块 ID；未配置时允许传入 NULL
     * @return   {IpcRet_E} OK：成功，ERR_PARAM/ERR：失败
     */
    IpcRet_E store_crypto_module_id(const std::string &strModuleId);

    /**
     * @brief   : 确保设备 SM2 私钥存在
     * @param    {std::string} strPassword：私钥保护口令
     * @param    {bool} bForceRegenerate：是否强制重新生成
     * @return   {IpcRet_E} OK：成功，非 OK：失败
     */
    IpcRet_E ensure_device_sm2_key(const std::string &strPassword, bool bForceRegenerate);

    /**
     * @brief   : 获取设备 SM2 私钥路径或硬件句柄描述
     * @return   {std::string} 软件后端返回 PEM 文件路径；硬件后端可返回 key handle 描述
     */
    std::string device_sm2_key_path() const;

    /**
     * @brief   : 保存平台下发的 VKEK
     * @param    {std::vector<uint8_t>} vecVkek：VKEK 原始字节
     * @param    {std::string} strKeyVersion：密钥版本或接收时间标识
     * @return   {IpcRet_E} OK：成功，非 OK：失败
     */
    IpcRet_E store_vkek(const std::vector<uint8_t> &vecVkek, const std::string &strKeyVersion);

    /**
     * @brief   : 读取平台下发的 VKEK
     * @param    {std::vector<uint8_t>} &vecVkek：VKEK 原始字节输出
     * @param    {std::string} &strKeyVersion：密钥版本或接收时间标识输出
     * @return   {IpcRet_E} OK：成功，ERR_NOT_EXIST/ERR_PARSE/ERR：失败
     */
    IpcRet_E load_vkek(std::vector<uint8_t> &vecVkek, std::string &strKeyVersion);

    /**
     * @brief   : 清除本地保存的 VKEK
     * @return   {IpcRet_E} OK：成功，非 OK：失败
     */
    IpcRet_E clear_vkek();

private:
    /* 当前密钥存储后端指针 */
    IGb35114KeyStore *m_pStore = nullptr;
    /* 初始化标志 */
    bool m_bInitialized = false;
};

/**
 * @brief   : GB35114 软件密钥存储后端
 * @note    : 当前阶段用于软件方案落地。该后端把设备 SM2 私钥保存为加密 PEM 文件，把 VKEK 保存为受限权限文件。
 */
class CSoftwareGb35114KeyStore : public IGb35114KeyStore, public CSingleton<CSoftwareGb35114KeyStore>
{
public:
    CSoftwareGb35114KeyStore();
    ~CSoftwareGb35114KeyStore() override;
    friend class CSingleton<CSoftwareGb35114KeyStore>;

    IpcRet_E init() override;
    IpcRet_E deinit() override;
    bool is_ready() const override;
    const char *name() const override;
    std::string crypto_module_id() const override;
    IpcRet_E store_crypto_module_id(const std::string &strModuleId) override;
    IpcRet_E ensure_device_sm2_key(const std::string &strPassword, bool bForceRegenerate) override;
    std::string device_sm2_key_path() const override;
    IpcRet_E store_vkek(const std::vector<uint8_t> &vecVkek, const std::string &strKeyVersion) override;
    IpcRet_E load_vkek(std::vector<uint8_t> &vecVkek, std::string &strKeyVersion) override;
    IpcRet_E clear_vkek() override;

private:
    /* 初始化标志 */
    bool m_bInitialized = false;
};
