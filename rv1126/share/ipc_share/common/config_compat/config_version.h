/**
 * @FilePath     : config_version.h
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2026-01-13
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-01-13 10:56:14
 * @Description  : 配置文件版本管理模块
 */

#pragma once

#include <string>
#include <cstdint>

namespace ConfigCompat_NS
{

    /**
     * @brief   : 配置版本号结构体
     * @note    : 使用语义化版本号 (major.minor.patch)
     *            - major: 不兼容的API变更（删除字段、字段类型改变）
     *            - minor: 向后兼容的功能新增（新增字段）
     *            - patch: 向后兼容的问题修复（取值范围调整）
     */
    typedef struct _ConfigVersion_S_
    {
        uint16_t u16Major; /* 主版本号 */
        uint16_t u16Minor; /* 次版本号 */
        uint16_t u16Patch; /* 补丁版本号 */

        _ConfigVersion_S_() : u16Major(1), u16Minor(0), u16Patch(0)
        {
        }

        _ConfigVersion_S_(uint16_t major, uint16_t minor, uint16_t patch)
            : u16Major(major), u16Minor(minor), u16Patch(patch)
        {
        }

        /**
         * @brief   : 转换为字符串格式
         * @return   {std::string} 版本字符串，如 "1.2.3"
         */
        std::string toString() const
        {
            return std::to_string(u16Major) + "." + std::to_string(u16Minor) + "." +
                   std::to_string(u16Patch);
        }

        /**
         * @brief   : 从字符串解析版本号
         * @param    {std::string} &str：版本字符串，如 "1.2.3"
         * @return   {bool} true表示解析成功，false表示解析失败
         */
        bool fromString(const std::string& str)
        {
            size_t pos1 = str.find('.');
            if (pos1 == std::string::npos)
            {
                return false;
            }

            size_t pos2 = str.find('.', pos1 + 1);
            if (pos2 == std::string::npos)
            {
                return false;
            }

            try
            {
                u16Major = static_cast<uint16_t>(std::stoi(str.substr(0, pos1)));
                u16Minor = static_cast<uint16_t>(std::stoi(str.substr(pos1 + 1, pos2 - pos1 - 1)));
                u16Patch = static_cast<uint16_t>(std::stoi(str.substr(pos2 + 1)));
                return true;
            }
            catch (...)
            {
                return false;
            }
        }

        /**
         * @brief   : 转换为整数形式（用于比较）
         * @return   {uint64_t} 版本号整数形式
         */
        uint64_t toInteger() const
        {
            return (static_cast<uint64_t>(u16Major) << 32) |
                   (static_cast<uint64_t>(u16Minor) << 16) | static_cast<uint64_t>(u16Patch);
        }

        /* 比较运算符 */
        bool operator==(const _ConfigVersion_S_& other) const
        {
            return toInteger() == other.toInteger();
        }

        bool operator!=(const _ConfigVersion_S_& other) const
        {
            return !(*this == other);
        }

        bool operator<(const _ConfigVersion_S_& other) const
        {
            return toInteger() < other.toInteger();
        }

        bool operator<=(const _ConfigVersion_S_& other) const
        {
            return toInteger() <= other.toInteger();
        }

        bool operator>(const _ConfigVersion_S_& other) const
        {
            return toInteger() > other.toInteger();
        }

        bool operator>=(const _ConfigVersion_S_& other) const
        {
            return toInteger() >= other.toInteger();
        }

    } ConfigVersion_S;

    /**
     * @brief   : 版本化配置的基类/包装器
     * @note    : 用于在JSON中包含版本信息
     */
    typedef struct _VersionedConfig_S_
    {
        ConfigVersion_S stVersion; /* 配置版本 */
        std::string strConfigType; /* 配置类型标识 */

        _VersionedConfig_S_() : stVersion(), strConfigType("")
        {
        }

        _VersionedConfig_S_(const std::string& type, const ConfigVersion_S& version)
            : stVersion(version), strConfigType(type)
        {
        }
    } VersionedConfig_S;

    /**
     * @brief   : 版本兼容性检查结果
     */
    typedef enum class _CompatResult_E_
    {
        COMPATIBLE = 0,  /* 完全兼容 */
        NEED_MIGRATION,  /* 需要迁移 */
        NEED_VALIDATION, /* 需要验证 */
        INCOMPATIBLE,    /* 不兼容（需要重置为默认值） */
        VERSION_TOO_NEW, /* 配置版本过新（降级场景） */
    } CompatResult_E;

    /**
     * @brief   : 检查版本兼容性
     * @param    {ConfigVersion_S} &fileVersion：文件中的版本
     * @param    {ConfigVersion_S} &currentVersion：当前代码版本
     * @return   {CompatResult_E} 兼容性检查结果
     */
    inline CompatResult_E checkCompatibility(const ConfigVersion_S& fileVersion,
                                             const ConfigVersion_S& currentVersion)
    {
        if (fileVersion == currentVersion)
        {
            return CompatResult_E::COMPATIBLE;
        }

        if (fileVersion > currentVersion)
        {
            /* 文件版本比代码版本高，降级场景 */
            return CompatResult_E::VERSION_TOO_NEW;
        }

        /* 文件版本比代码版本低，需要升级 */
        if (fileVersion.u16Major < currentVersion.u16Major)
        {
            /* 主版本号不同，可能不兼容 */
            return CompatResult_E::NEED_MIGRATION;
        }

        if (fileVersion.u16Minor < currentVersion.u16Minor)
        {
            /* 次版本号不同，需要迁移（可能有新字段） */
            return CompatResult_E::NEED_MIGRATION;
        }

        /* 仅补丁版本不同，需要验证（可能有范围调整） */
        return CompatResult_E::NEED_VALIDATION;
    }

} /* namespace ConfigCompat */
