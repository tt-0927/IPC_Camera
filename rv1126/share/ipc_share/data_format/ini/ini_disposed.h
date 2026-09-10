/**
 * @FilePath     : ini_disposed.h
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2026-02-27 13:40:43
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-07-27 20:24:09
 * @Description  : INI配置文件读写接口与只读解析器定义
 */

#pragma once

#ifdef __cplusplus
#include <cstdint>
#include <map>
#include <string>
#else
#include <stdint.h>
#endif

#ifdef __cplusplus
extern "C"
{
#endif

#define REDA_INI_SUCCESS 1
#define REDA_INI_FAIL    0

    /**
     * @brief   : 读取INI字符串
     * @param    {const char *} section：配置段名称
     * @param    {const char *} key：配置项名称
     * @param    {char *} value：输出缓冲区
     * @param    {int} size：输出缓冲区长度
     * @param    {const char *} default_value：读取失败时使用的默认值，可为空
     * @param    {const char *} file：INI文件路径
     * @return   {int} REDA_INI_SUCCESS：成功，REDA_INI_FAIL：失败
     */
    int ini_read_profile_char(const char *section,
                              const char *key,
                              char *value,
                              int size,
                              const char *default_value,
                              const char *file);

    /**
     * @brief   : 读取INI整数
     * @param    {const char *} section：配置段名称
     * @param    {const char *} key：配置项名称
     * @param    {int *} nValue：输出整数
     * @param    {int} default_value：读取失败时使用的默认值
     * @param    {const char *} file：INI文件路径
     * @return   {int} REDA_INI_SUCCESS：成功，REDA_INI_FAIL：失败
     */
    int ini_read_profile_int(const char *section, const char *key, int *nValue, int default_value, const char *file);

    /**
     * @brief   : 写入INI字符串
     * @param    {const char *} section：配置段名称
     * @param    {const char *} key：配置项名称
     * @param    {const char *} value：待写入字符串
     * @param    {const char *} file：INI文件路径
     * @return   {int} REDA_INI_SUCCESS：成功，REDA_INI_FAIL：失败
     */
    int ini_write_profile_char(const char *section, const char *key, const char *value, const char *file);

    /**
     * @brief   : 写入INI整数
     * @param    {const char *} section：配置段名称
     * @param    {const char *} key：配置项名称
     * @param    {int} value：待写入整数
     * @param    {const char *} file：INI文件路径
     * @return   {int} REDA_INI_SUCCESS：成功，REDA_INI_FAIL：失败
     */
    int ini_write_profile_int(const char *section, const char *key, int value, const char *file);

#ifdef __cplusplus
}
#endif

#ifdef __cplusplus

/**
 * @brief   : INI重复段和重复键的处理策略
 * @note    : 新配置默认拒绝重复项，避免维护人员追加配置后静默使用旧值；历史接口可显式保留首项优先语义。
 */
enum class IniDuplicatePolicy_E
{
    REJECT,     /* 遇到重复段或键立即返回ERR_PARSE */
    KEEP_FIRST, /* 保留首次出现的完整段和键，忽略后续重复项 */
};

/**
 * @brief 一次加载、只读访问的INI解析器。
 * @note  支持空行、键值两侧空格、单双引号、`;`/`#`注释和反斜杠续行；默认拒绝重复段和重复键。
 */
class CIniReader
{
public:
    /**
     * @brief   : 构造未加载的INI解析器
     * @return  : 无
     */
    CIniReader() = default;

    /**
     * @brief   : 构造并加载INI文件
     * @param    {const std::string &} strFilePath：INI文件路径
     * @return  : 无
     * @note    : 调用方应通过is_loaded或last_error检查加载结果
     */
    explicit CIniReader(const std::string &strFilePath);

    /**
     * @brief   : 加载并解析INI文件
     * @param    {const std::string &} strFilePath：INI文件路径
     * @param    {IniDuplicatePolicy_E} enDuplicatePolicy：重复段和键的处理策略
     * @return   {int} OK：成功，ERR_OPEN/ERR_FREAD/ERR_FILE_ERR/ERR_PARSE：失败
     */
    int load(const std::string &strFilePath, IniDuplicatePolicy_E enDuplicatePolicy = IniDuplicatePolicy_E::REJECT);

    /**
     * @brief   : 清除已加载配置和错误信息
     * @return   {void}
     */
    void clear();

    /**
     * @brief   : 判断配置是否已成功加载
     * @return   {bool} true：已加载，false：未加载
     */
    bool is_loaded() const;

    /**
     * @brief   : 判断配置段是否存在
     * @param    {const std::string &} strSection：配置段名称
     * @return   {bool} true：存在，false：不存在
     */
    bool has_section(const std::string &strSection) const;

    /**
     * @brief   : 判断配置项是否存在
     * @param    {const std::string &} strSection：配置段名称
     * @param    {const std::string &} strKey：配置项名称
     * @return   {bool} true：存在，false：不存在
     */
    bool has_key(const std::string &strSection, const std::string &strKey) const;

    /**
     * @brief   : 读取字符串
     * @param    {const std::string &} strSection：配置段名称
     * @param    {const std::string &} strKey：配置项名称
     * @param    {std::string &} strValue：输出字符串
     * @return   {int} OK：成功，ERR_UNINIT/ERR_NOT_EXIST：失败
     */
    int get_string(const std::string &strSection, const std::string &strKey, std::string &strValue) const;

    /**
     * @brief   : 严格读取int整数
     * @param    {const std::string &} strSection：配置段名称
     * @param    {const std::string &} strKey：配置项名称
     * @param    {int &} nValue：输出整数
     * @return   {int} OK：成功，ERR_NOT_EXIST/ERR_PARSE：失败
     */
    int get_int(const std::string &strSection, const std::string &strKey, int &nValue) const;

    /**
     * @brief   : 严格读取uint64_t整数
     * @param    {const std::string &} strSection：配置段名称
     * @param    {const std::string &} strKey：配置项名称
     * @param    {uint64_t &} u64Value：输出整数
     * @return   {int} OK：成功，ERR_NOT_EXIST/ERR_PARSE：失败
     */
    int get_uint64(const std::string &strSection, const std::string &strKey, uint64_t &u64Value) const;

    /**
     * @brief   : 严格读取布尔值
     * @param    {const std::string &} strSection：配置段名称
     * @param    {const std::string &} strKey：配置项名称
     * @param    {bool &} bValue：输出布尔值
     * @return   {int} OK：成功，ERR_NOT_EXIST/ERR_PARSE：失败
     * @note    : 支持true/false、yes/no、on/off和1/0，不区分大小写
     */
    int get_bool(const std::string &strSection, const std::string &strKey, bool &bValue) const;

    /**
     * @brief   : 获取最近一次加载或读取错误
     * @return   {const std::string &} 可用于日志的错误详情
     */
    const std::string &last_error() const;

private:
    using KeyValueMap_T = std::map<std::string, std::string>;
    using SectionMap_T = std::map<std::string, KeyValueMap_T>;

    /**
     * @brief   : 记录错误码和上下文
     * @param    {int} nRet：错误码
     * @param    {const std::string &} strError：错误详情
     * @return   {int} 原错误码
     */
    int set_error(int nRet, const std::string &strError) const;

    /**
     * @brief   : 记录INI加载阶段的不可恢复错误
     * @param    {int} nRet：公共返回码
     * @param    {const std::string &} strFilePath：目标INI文件路径
     * @param    {const std::string &} strError：包含文件位置的错误详情
     * @return   {int} 原错误码
     * @note    : 仅供load调用；get接口的缺项可作为默认值分支，不在此处输出日志。
     */
    int set_load_error(int nRet, const std::string &strFilePath, const std::string &strError) const;

    /* 解析后的段和键值数据，只在load成功后对外可见。 */
    SectionMap_T m_mapSections;
    /* 最近一次成功加载的文件路径。 */
    std::string m_strFilePath;
    /* 最近一次错误详情，const读取接口可更新诊断信息。 */
    mutable std::string m_strLastError;
    /* 成功加载标记。 */
    bool m_bLoaded{ false };
};

/**
 * @brief 兼容历史调用的INI读写门面。
 * @note  保留历史“每次读取都重新加载文件”的行为；新代码优先使用CIniReader一次加载。
 */
class CIni
{
public:
    /**
     * @brief   : 构造INI读写门面
     * @param    {std::string} strFileName：INI文件路径
     * @return  : 无
     */
    explicit CIni(std::string strFileName);

    /**
     * @brief   : 析构INI读写门面
     * @return  : 无
     */
    ~CIni();

    /**
     * @brief   : 切换INI文件
     * @param    {std::string} strFileName：INI文件路径
     * @return   {void}
     */
    void load(std::string strFileName);

    /**
     * @brief   : 读取字符串
     * @param    {std::string} strSection：配置段名称
     * @param    {std::string} strKey：配置项名称
     * @param    {std::string &} strValue：输出字符串
     * @param    {std::string} strDefaultValue：失败默认值
     * @return   {int} REDA_INI_SUCCESS：成功，REDA_INI_FAIL：失败
     */
    int read(std::string strSection, std::string strKey, std::string &strValue, std::string strDefaultValue = std::string());

    /**
     * @brief   : 读取整数
     * @param    {std::string} strSection：配置段名称
     * @param    {std::string} strKey：配置项名称
     * @param    {int &} nValue：输出整数
     * @param    {int} nDefaultValue：失败默认值
     * @return   {int} REDA_INI_SUCCESS：成功，REDA_INI_FAIL：失败
     */
    int read(std::string strSection, std::string strKey, int &nValue, int nDefaultValue);

    /**
     * @brief   : 写入字符串
     * @param    {std::string} strSection：配置段名称
     * @param    {std::string} strKey：配置项名称
     * @param    {std::string} strValue：待写入字符串
     * @return   {int} REDA_INI_SUCCESS：成功，REDA_INI_FAIL：失败
     */
    int write(std::string strSection, std::string strKey, std::string strValue);

    /**
     * @brief   : 写入整数
     * @param    {std::string} strSection：配置段名称
     * @param    {std::string} strKey：配置项名称
     * @param    {int} nValue：待写入整数
     * @return   {int} REDA_INI_SUCCESS：成功，REDA_INI_FAIL：失败
     */
    int write(std::string strSection, std::string strKey, int nValue);

private:
    /* 当前INI文件路径。 */
    std::string m_strFileName;
};

#endif
