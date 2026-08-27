/**
 * @FilePath     : gb35114_keystore.cpp
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2026-06-16
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-06-22 09:18:24
 * @Description  : GB35114 密钥安全存储抽象实现
 */

#include "gb35114_keystore.h"

#include "crypto_manager.h"
#include "dlog.h"
#include "path_define.h"

#include <algorithm>
#include <cerrno>
#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

namespace {

/* info: 软件后端安全状态目录，硬件后端接入后可只保留迁移兼容逻辑 */
static constexpr const char *GB35114_SECURE_DIR = GM_CA_PATH "secure/";
/* info: VKEK 由平台注册响应下发，当前软件方案以受限权限文件保存 */
static constexpr const char *GB35114_VKEK_FILE = GM_CA_PATH "secure/vkek.dat";
/* info: GB35114 附录 B 定义密码模块 ID 为 22 位十进制编码，未配置时 CSR 使用 NULL */
static constexpr const char *GB35114_MODULE_ID_FILE = GM_CA_PATH "secure/crypto_module_id";

/**
 * @brief   : 判断普通文件是否存在
 * @param    {std::string} strPath：文件路径
 * @return   {bool} true：存在，false：不存在
 */
bool file_exists(const std::string &strPath)
{
    return access(strPath.c_str(), F_OK) == 0;
}

/**
 * @brief   : 递归创建目录
 * @param    {std::string} strPath：目录路径
 * @param    {mode_t} mode：目录权限
 * @return   {IpcRet_E} OK：成功，非 OK：失败
 */
IpcRet_E mkdir_recursive(const std::string &strPath, mode_t mode)
{
    if (strPath.empty())
    {
        return ERR_PARAM;
    }

    std::string strCurrent;
    strCurrent.reserve(strPath.size());

    for (size_t i = 0; i < strPath.size(); ++i)
    {
        strCurrent.push_back(strPath[i]);
        if (strPath[i] != '/')
        {
            continue;
        }

        if (strCurrent.size() <= 1)
        {
            continue;
        }

        if (mkdir(strCurrent.c_str(), mode) != 0 && errno != EEXIST)
        {
            dlog_error("创建目录失败: %s, errno=%d, error=%s", strCurrent.c_str(), errno, std::strerror(errno));
            return ERR_CREATE;
        }
    }

    if (strPath.back() != '/')
    {
        if (mkdir(strPath.c_str(), mode) != 0 && errno != EEXIST)
        {
            dlog_error("创建目录失败: %s, errno=%d, error=%s", strPath.c_str(), errno, std::strerror(errno));
            return ERR_CREATE;
        }
    }

    return OK;
}

/**
 * @brief   : 原子写入文本文件
 * @param    {std::string} strPath：目标文件路径
 * @param    {std::string} strContent：文件内容
 * @param    {mode_t} mode：目标文件权限
 * @return   {IpcRet_E} OK：成功，非 OK：失败
 * @note    : 先写临时文件再 rename，避免异常掉电留下半截密钥状态。
 */
IpcRet_E write_text_file_atomic(const std::string &strPath, const std::string &strContent, mode_t mode)
{
    if (strPath.empty())
    {
        return ERR_PARAM;
    }

    const std::string strTmpPath = strPath + ".tmp";
    {
        std::ofstream ofs(strTmpPath, std::ios::out | std::ios::binary | std::ios::trunc);
        if (!ofs.is_open())
        {
            dlog_error("打开临时文件失败: %s, errno=%d, error=%s", strTmpPath.c_str(), errno, std::strerror(errno));
            return ERR_OPEN;
        }

        ofs << strContent;
        if (!ofs.good())
        {
            dlog_error("写入临时文件失败: %s", strTmpPath.c_str());
            std::remove(strTmpPath.c_str());
            return ERR_FWRITE;
        }
    }

    if (chmod(strTmpPath.c_str(), mode) != 0)
    {
        dlog_error("设置文件权限失败: %s, errno=%d, error=%s", strTmpPath.c_str(), errno, std::strerror(errno));
        std::remove(strTmpPath.c_str());
        return ERR;
    }

    if (std::rename(strTmpPath.c_str(), strPath.c_str()) != 0)
    {
        dlog_error("替换文件失败: %s -> %s, errno=%d, error=%s",
                   strTmpPath.c_str(),
                   strPath.c_str(),
                   errno,
                   std::strerror(errno));
        std::remove(strTmpPath.c_str());
        return ERR;
    }

    return OK;
}

/**
 * @brief   : 读取完整文本文件
 * @param    {std::string} strPath：文件路径
 * @param    {std::string} &strContent：文件内容输出
 * @return   {IpcRet_E} OK：成功，ERR_NOT_EXIST/ERR_OPEN：失败
 */
IpcRet_E read_text_file(const std::string &strPath, std::string &strContent)
{
    if (!file_exists(strPath))
    {
        return ERR_NOT_EXIST;
    }

    std::ifstream ifs(strPath, std::ios::in | std::ios::binary);
    if (!ifs.is_open())
    {
        dlog_error("打开文件失败: %s, errno=%d, error=%s", strPath.c_str(), errno, std::strerror(errno));
        return ERR_OPEN;
    }

    std::ostringstream oss;
    oss << ifs.rdbuf();
    strContent = oss.str();
    return OK;
}

/**
 * @brief   : 去除字符串前后空白字符
 * @param    {std::string} strValue：输入字符串
 * @return   {std::string} 去除空白后的字符串
 */
std::string trim_copy(const std::string &strValue)
{
    const std::string strWhitespace = " \t\r\n";
    const size_t nBegin = strValue.find_first_not_of(strWhitespace);
    if (nBegin == std::string::npos)
    {
        return std::string();
    }

    const size_t nEnd = strValue.find_last_not_of(strWhitespace);
    return strValue.substr(nBegin, nEnd - nBegin + 1);
}

/**
 * @brief   : 校验 GB35114 密码模块 ID
 * @param    {std::string} strModuleId：密码模块 ID
 * @return   {bool} true：合法，false：非法
 */
bool is_valid_module_id(const std::string &strModuleId)
{
    if (strModuleId == "NULL")
    {
        return true;
    }

    return strModuleId.size() == 22 && std::all_of(strModuleId.begin(),
                                                   strModuleId.end(),
                                                   [](char ch)
                                                   {
                                                       return ch >= '0' && ch <= '9';
                                                   });
}

/**
 * @brief   : 二进制数据转十六进制字符串
 * @param    {std::vector<uint8_t>} vecData：输入数据
 * @return   {std::string} 十六进制字符串
 */
std::string hex_encode(const std::vector<uint8_t> &vecData)
{
    std::ostringstream oss;
    oss << std::hex << std::setfill('0');
    for (uint8_t u8Value : vecData)
    {
        oss << std::setw(2) << static_cast<unsigned int>(u8Value);
    }
    return oss.str();
}

/**
 * @brief   : 十六进制字符串转二进制数据
 * @param    {std::string} strHex：十六进制字符串
 * @param    {std::vector<uint8_t>} &vecData：输出数据
 * @return   {bool} true：成功，false：失败
 */
bool hex_decode(const std::string &strHex, std::vector<uint8_t> &vecData)
{
    if ((strHex.size() % 2) != 0)
    {
        return false;
    }

    vecData.clear();
    vecData.reserve(strHex.size() / 2);
    for (size_t i = 0; i < strHex.size(); i += 2)
    {
        const char chHigh = strHex[i];
        const char chLow = strHex[i + 1];
        if (!std::isxdigit(static_cast<unsigned char>(chHigh)) || !std::isxdigit(static_cast<unsigned char>(chLow)))
        {
            return false;
        }

        const std::string strByte = strHex.substr(i, 2);
        vecData.push_back(static_cast<uint8_t>(std::strtoul(strByte.c_str(), nullptr, 16)));
    }

    return true;
}

/**
 * @brief   : 从键值文本中读取指定字段
 * @param    {std::string} strContent：键值文本
 * @param    {std::string} strKey：字段名
 * @return   {std::string} 字段值
 */
std::string read_kv_value(const std::string &strContent, const std::string &strKey)
{
    std::istringstream iss(strContent);
    std::string strLine;
    const std::string strPrefix = strKey + "=";
    while (std::getline(iss, strLine))
    {
        if (strLine.find(strPrefix) == 0)
        {
            return trim_copy(strLine.substr(strPrefix.size()));
        }
    }

    return std::string();
}

}

CGb35114KeyStoreManager::CGb35114KeyStoreManager()
{
}

CGb35114KeyStoreManager::~CGb35114KeyStoreManager()
{
    deinit();
}

IpcRet_E CGb35114KeyStoreManager::set_store(IGb35114KeyStore *pStore)
{
    if (pStore == nullptr)
    {
        dlog_error("[CGb35114KeyStoreManager] Store 为空");
        return ERR_PARAM_NULL;
    }

    if (m_bInitialized)
    {
        dlog_error("[CGb35114KeyStoreManager] 已初始化，禁止切换 Store");
        return ERR;
    }

    m_pStore = pStore;
    dlog_info("[CGb35114KeyStoreManager] Store 已设置: %s", m_pStore->name());
    return OK;
}

IpcRet_E CGb35114KeyStoreManager::init()
{
    if (m_bInitialized)
    {
        return OK;
    }

    if (m_pStore == nullptr)
    {
        dlog_error("[CGb35114KeyStoreManager] Store 未初始化");
        return ERR_UNINIT;
    }

    IpcRet_E enRet = m_pStore->init();
    if (enRet != OK)
    {
        dlog_error("[CGb35114KeyStoreManager] Store 初始化失败: %s, ret=%d", m_pStore->name(), enRet);
        return enRet;
    }

    m_bInitialized = true;
    dlog_info("[CGb35114KeyStoreManager] init 完成，当前 Store: %s", m_pStore->name());
    return OK;
}

IpcRet_E CGb35114KeyStoreManager::deinit()
{
    if (!m_bInitialized)
    {
        return OK;
    }

    if (m_pStore != nullptr)
    {
        m_pStore->deinit();
    }

    m_bInitialized = false;
    return OK;
}

bool CGb35114KeyStoreManager::is_ready() const
{
    return m_bInitialized && m_pStore != nullptr && m_pStore->is_ready();
}

const char *CGb35114KeyStoreManager::name() const
{
    if (m_pStore == nullptr)
    {
        return "none";
    }
    return m_pStore->name();
}

std::string CGb35114KeyStoreManager::crypto_module_id() const
{
    if (!is_ready())
    {
        dlog_error("[CGb35114KeyStoreManager] Store 未就绪，返回 NULL 密码模块 ID");
        return "NULL";
    }
    return m_pStore->crypto_module_id();
}

IpcRet_E CGb35114KeyStoreManager::store_crypto_module_id(const std::string &strModuleId)
{
    if (!is_ready())
    {
        dlog_error("[CGb35114KeyStoreManager] Store 未就绪，无法保存密码模块 ID");
        return ERR_UNINIT;
    }
    return m_pStore->store_crypto_module_id(strModuleId);
}

IpcRet_E CGb35114KeyStoreManager::ensure_device_sm2_key(const std::string &strPassword, bool bForceRegenerate)
{
    if (!is_ready())
    {
        dlog_error("[CGb35114KeyStoreManager] Store 未就绪，无法准备设备 SM2 私钥");
        return ERR_UNINIT;
    }
    return m_pStore->ensure_device_sm2_key(strPassword, bForceRegenerate);
}

std::string CGb35114KeyStoreManager::device_sm2_key_path() const
{
    if (!is_ready())
    {
        dlog_error("[CGb35114KeyStoreManager] Store 未就绪，无法获取设备 SM2 私钥路径");
        return std::string();
    }
    return m_pStore->device_sm2_key_path();
}

IpcRet_E CGb35114KeyStoreManager::store_vkek(const std::vector<uint8_t> &vecVkek, const std::string &strKeyVersion)
{
    if (!is_ready())
    {
        dlog_error("[CGb35114KeyStoreManager] Store 未就绪，无法保存 VKEK");
        return ERR_UNINIT;
    }
    return m_pStore->store_vkek(vecVkek, strKeyVersion);
}

IpcRet_E CGb35114KeyStoreManager::load_vkek(std::vector<uint8_t> &vecVkek, std::string &strKeyVersion)
{
    if (!is_ready())
    {
        dlog_error("[CGb35114KeyStoreManager] Store 未就绪，无法读取 VKEK");
        return ERR_UNINIT;
    }
    return m_pStore->load_vkek(vecVkek, strKeyVersion);
}

IpcRet_E CGb35114KeyStoreManager::clear_vkek()
{
    if (!is_ready())
    {
        dlog_error("[CGb35114KeyStoreManager] Store 未就绪，无法清除 VKEK");
        return ERR_UNINIT;
    }
    return m_pStore->clear_vkek();
}

CSoftwareGb35114KeyStore::CSoftwareGb35114KeyStore()
{
}

CSoftwareGb35114KeyStore::~CSoftwareGb35114KeyStore()
{
    deinit();
}

IpcRet_E CSoftwareGb35114KeyStore::init()
{
    if (m_bInitialized)
    {
        return OK;
    }

    IpcRet_E enRet = mkdir_recursive(GM_CA_PATH, 0755);
    if (enRet != OK)
    {
        return enRet;
    }

    enRet = mkdir_recursive(GM_CA_DEVICE_PATH, 0700);
    if (enRet != OK)
    {
        return enRet;
    }

    enRet = mkdir_recursive(GB35114_SECURE_DIR, 0700);
    if (enRet != OK)
    {
        return enRet;
    }

    m_bInitialized = true;
    dlog_info("[CSoftwareGb35114KeyStore] init 完成");
    return OK;
}

IpcRet_E CSoftwareGb35114KeyStore::deinit()
{
    m_bInitialized = false;
    return OK;
}

bool CSoftwareGb35114KeyStore::is_ready() const
{
    return m_bInitialized;
}

const char *CSoftwareGb35114KeyStore::name() const
{
    return "software";
}

std::string CSoftwareGb35114KeyStore::crypto_module_id() const
{
    std::string strContent;
    if (read_text_file(GB35114_MODULE_ID_FILE, strContent) != OK)
    {
        return "NULL";
    }

    const std::string strModuleId = trim_copy(strContent);
    if (!is_valid_module_id(strModuleId))
    {
        dlog_warn("[CSoftwareGb35114KeyStore] 密码模块 ID 非法，按 NULL 处理");
        return "NULL";
    }

    return strModuleId;
}

IpcRet_E CSoftwareGb35114KeyStore::store_crypto_module_id(const std::string &strModuleId)
{
    if (!m_bInitialized)
    {
        return ERR_UNINIT;
    }

    const std::string strNormalized = trim_copy(strModuleId.empty() ? "NULL" : strModuleId);
    if (!is_valid_module_id(strNormalized))
    {
        dlog_error("[CSoftwareGb35114KeyStore] 密码模块 ID 非法，必须为 22 位十进制或 NULL");
        return ERR_PARAM;
    }

    IpcRet_E enRet = write_text_file_atomic(GB35114_MODULE_ID_FILE, strNormalized + "\n", 0600);
    if (enRet != OK)
    {
        return enRet;
    }

    dlog_info("[CSoftwareGb35114KeyStore] 密码模块 ID 已保存");
    return OK;
}

IpcRet_E CSoftwareGb35114KeyStore::ensure_device_sm2_key(const std::string &strPassword, bool bForceRegenerate)
{
    if (!m_bInitialized)
    {
        return ERR_UNINIT;
    }

    if (strPassword.empty())
    {
        dlog_error("[CSoftwareGb35114KeyStore] 设备 SM2 私钥密码为空");
        return ERR_PARAM;
    }

    if (!bForceRegenerate && file_exists(GM_CA_DEVICE_KEY))
    {
        return OK;
    }

    if (!CCryptoManager::instance()->is_ready())
    {
        dlog_error("[CSoftwareGb35114KeyStore] CCryptoManager 未就绪，无法生成设备 SM2 私钥");
        return ERR_UNINIT;
    }

    if (bForceRegenerate && file_exists(GM_CA_DEVICE_KEY))
    {
        const std::string strBackupPath = std::string(GM_CA_DEVICE_KEY) + ".bak";
        std::remove(strBackupPath.c_str());
        if (std::rename(GM_CA_DEVICE_KEY, strBackupPath.c_str()) != 0)
        {
            dlog_error("[CSoftwareGb35114KeyStore] 备份旧设备 SM2 私钥失败: %s -> %s", GM_CA_DEVICE_KEY, strBackupPath.c_str());
            return ERR;
        }
        dlog_warn("[CSoftwareGb35114KeyStore] 旧设备 SM2 私钥已备份: %s", strBackupPath.c_str());
    }

    const int nRet = CCryptoManager::instance()->sm2keygen(strPassword, GM_CA_DEVICE_KEY);
    if (nRet != OK)
    {
        dlog_error("[CSoftwareGb35114KeyStore] 生成设备 SM2 私钥失败: %s, ret=%d", GM_CA_DEVICE_KEY, nRet);
        return static_cast<IpcRet_E>(nRet);
    }

    if (!file_exists(GM_CA_DEVICE_KEY))
    {
        dlog_error("[CSoftwareGb35114KeyStore] 设备 SM2 私钥生成后不存在: %s", GM_CA_DEVICE_KEY);
        return ERR_NOT_EXIST;
    }

    if (chmod(GM_CA_DEVICE_KEY, 0600) != 0)
    {
        dlog_error("[CSoftwareGb35114KeyStore] 设置设备 SM2 私钥权限失败: %s, errno=%d, error=%s",
                   GM_CA_DEVICE_KEY,
                   errno,
                   std::strerror(errno));
        return ERR;
    }

    dlog_info("[CSoftwareGb35114KeyStore] 设备 SM2 私钥已准备: %s", GM_CA_DEVICE_KEY);
    return OK;
}

std::string CSoftwareGb35114KeyStore::device_sm2_key_path() const
{
    return GM_CA_DEVICE_KEY;
}

IpcRet_E CSoftwareGb35114KeyStore::store_vkek(const std::vector<uint8_t> &vecVkek, const std::string &strKeyVersion)
{
    if (!m_bInitialized)
    {
        return ERR_UNINIT;
    }

    if (vecVkek.empty())
    {
        dlog_error("[CSoftwareGb35114KeyStore] VKEK 为空，拒绝保存");
        return ERR_PARAM;
    }

    std::ostringstream oss;
    oss << "version=" << strKeyVersion << "\n";
    oss << "vkek_hex=" << hex_encode(vecVkek) << "\n";

    IpcRet_E enRet = write_text_file_atomic(GB35114_VKEK_FILE, oss.str(), 0600);
    if (enRet != OK)
    {
        return enRet;
    }

    /* ! VKEK 属于会话加密根材料，日志只允许记录状态，不允许输出内容 */
    dlog_info("[CSoftwareGb35114KeyStore] VKEK 已保存");
    return OK;
}

IpcRet_E CSoftwareGb35114KeyStore::load_vkek(std::vector<uint8_t> &vecVkek, std::string &strKeyVersion)
{
    if (!m_bInitialized)
    {
        return ERR_UNINIT;
    }

    std::string strContent;
    IpcRet_E enRet = read_text_file(GB35114_VKEK_FILE, strContent);
    if (enRet != OK)
    {
        return enRet;
    }

    strKeyVersion = read_kv_value(strContent, "version");
    const std::string strVkekHex = read_kv_value(strContent, "vkek_hex");
    if (strVkekHex.empty() || !hex_decode(strVkekHex, vecVkek) || vecVkek.empty())
    {
        dlog_error("[CSoftwareGb35114KeyStore] VKEK 文件格式错误");
        vecVkek.clear();
        strKeyVersion.clear();
        return ERR_PARSE;
    }

    return OK;
}

IpcRet_E CSoftwareGb35114KeyStore::clear_vkek()
{
    if (!m_bInitialized)
    {
        return ERR_UNINIT;
    }

    if (!file_exists(GB35114_VKEK_FILE))
    {
        return OK;
    }

    if (std::remove(GB35114_VKEK_FILE) != 0)
    {
        dlog_error("[CSoftwareGb35114KeyStore] 删除 VKEK 文件失败: %s, errno=%d, error=%s",
                   GB35114_VKEK_FILE,
                   errno,
                   std::strerror(errno));
        return ERR;
    }

    dlog_info("[CSoftwareGb35114KeyStore] VKEK 已清除");
    return OK;
}
