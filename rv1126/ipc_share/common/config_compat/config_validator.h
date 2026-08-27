/**
 * @FilePath     : config_validator.h
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2026-01-13
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-01-13 16:36:31
 * @Description  : 配置数据验证器模块
 */

#pragma once

#include <string>
#include <functional>
#include <vector>
#include <map>
#include "dlog.h"

namespace ConfigCompat_NS
{
    /**
     * @brief   : 验证结果枚举
     */
    typedef enum class _ValidateResult_E_
    {
        VALID = 0, /* 验证通过 */
        FIXED,     /* 已修复（超出范围的值被调整） */
        INVALID,   /* 无效（无法修复） */
    } ValidateResult_E;

    /**
     * @brief   : 字段验证结果
     */
    typedef struct _FieldValidateResult_S_
    {
        std::string strFieldName;  /* 字段名 */
        ValidateResult_E enResult; /* 验证结果 */
        std::string strMessage;    /* 验证消息 */

        _FieldValidateResult_S_()
            : strFieldName(""), enResult(ValidateResult_E::VALID), strMessage("")
        {
        }

        _FieldValidateResult_S_(const std::string& name,
                                ValidateResult_E result,
                                const std::string& msg)
            : strFieldName(name), enResult(result), strMessage(msg)
        {
        }
    } FieldValidateResult_S;

    /**
     * @brief   : 整体验证结果
     */
    typedef struct _ConfigValidateResult_S_
    {
        ValidateResult_E enOverallResult;             /* 整体结果 */
        std::vector<FieldValidateResult_S> vstFields; /* 各字段结果 */

        _ConfigValidateResult_S_() : enOverallResult(ValidateResult_E::VALID)
        {
            vstFields.clear();
        }

        void addFieldResult(const FieldValidateResult_S& result)
        {
            vstFields.push_back(result);
            /* 更新整体结果（取最严重的状态） */
            if (result.enResult > enOverallResult)
            {
                enOverallResult = result.enResult;
            }
        }

        bool isValid() const
        {
            return enOverallResult != ValidateResult_E::INVALID;
        }
    } ConfigValidateResult_S;

    /**
     * @brief   : 范围验证辅助类
     * @note    : 用于验证数值范围并自动修复
     */
    template <typename T>
    class CRangeValidator
    {
    public:
        /**
         * @brief   : 验证并修复数值范围
         * @param    {T} &value：待验证的值（会被修复）
         * @param    {T} minVal：最小值
         * @param    {T} maxVal：最大值
         * @param    {T} defaultVal：超出范围时的默认值
         * @param    {std::string} &fieldName：字段名
         * @return   {FieldValidateResult_S} 验证结果
         */
        static FieldValidateResult_S validate(T& value,
                                              T minVal,
                                              T maxVal,
                                              T defaultVal,
                                              const std::string& fieldName)
        {
            if (value >= minVal && value <= maxVal)
            {
                return FieldValidateResult_S(fieldName, ValidateResult_E::VALID, "");
            }

            /* 超出范围，尝试修复 */
            T oldValue = value;
            if (value < minVal)
            {
                value = (defaultVal >= minVal && defaultVal <= maxVal) ? defaultVal : minVal;
            }
            else
            {
                value = (defaultVal >= minVal && defaultVal <= maxVal) ? defaultVal : maxVal;
            }

            std::string msg = fieldName + ": " + std::to_string(oldValue) +
                              " 超出范围 [" + std::to_string(minVal) + ", " +
                              std::to_string(maxVal) + "], 已修正为 " + std::to_string(value);

            dlog_warn("[配置数据验证器] %s", msg.c_str());

            return FieldValidateResult_S(fieldName, ValidateResult_E::FIXED, msg);
        }

        /**
         * @brief   : 验证枚举值范围
         * @param    {T} &value：待验证的枚举值
         * @param    {std::vector<T>} &validValues：有效值列表
         * @param    {T} defaultVal：无效时的默认值
         * @param    {std::string} &fieldName：字段名
         * @return   {FieldValidateResult_S} 验证结果
         */
        static FieldValidateResult_S validateEnum(T& value,
                                                  const std::vector<T>& validValues,
                                                  T defaultVal,
                                                  const std::string& fieldName)
        {
            for (const auto& v : validValues)
            {
                if (value == v)
                {
                    return FieldValidateResult_S(fieldName, ValidateResult_E::VALID, "");
                }
            }

            /* 不在有效值列表中，修复为默认值 */
            T oldValue = value;
            value = defaultVal;

            std::string msg = fieldName + ": 无效的枚举值 " +
                              std::to_string(static_cast<int>(oldValue)) + ", 已修正为 " +
                              std::to_string(static_cast<int>(value));

            dlog_warn("[配置数据验证器] %s", msg.c_str());

            return FieldValidateResult_S(fieldName, ValidateResult_E::FIXED, msg);
        }

        /**
         * @brief   : 验证连续枚举值范围（枚举值为 minVal, minVal+1, ..., maxVal）
         * @param    {EnumT} &value：待验证的枚举值（会被修复）
         * @param    {EnumT} minVal：最小枚举值
         * @param    {EnumT} maxVal：最大枚举值
         * @param    {EnumT} defaultVal：超出范围时的默认值
         * @param    {std::string} &fieldName：字段名
         * @return   {FieldValidateResult_S} 验证结果
         * @note    : 适用于连续枚举类型，避免手动类型转换
         */
        template <typename EnumT>
        static FieldValidateResult_S validateEnumRange(EnumT &value, EnumT minVal, EnumT maxVal,
                                                       EnumT defaultVal, const std::string &fieldName)
        {
            int intValue = static_cast<int>(value);
            int intMin = static_cast<int>(minVal);
            int intMax = static_cast<int>(maxVal);
            int intDefault = static_cast<int>(defaultVal);
    
            if (intValue >= intMin && intValue <= intMax)
            {
                return FieldValidateResult_S(fieldName, ValidateResult_E::VALID, "");
            }
    
            /* 超出范围，尝试修复 */
            int oldValue = intValue;
            if (intDefault >= intMin && intDefault <= intMax)
            {
                intValue = intDefault;
            }
            else if (intValue < intMin)
            {
                intValue = intMin;
            }
            else
            {
                intValue = intMax;
            }
    
            value = static_cast<EnumT>(intValue);
    
            std::string msg = fieldName + ": " + std::to_string(oldValue) +
                              " 超出范围 [" + std::to_string(intMin) + ", " +
                              std::to_string(intMax) + "], 已修正为 " + std::to_string(intValue);
    
            dlog_warn("[配置数据验证器] %s", msg.c_str());
    
            return FieldValidateResult_S(fieldName, ValidateResult_E::FIXED, msg);
        }
    };

    /**
     * @brief   : 配置验证器基类模板
     * @note    : 每个配置结构体需要特化此模板实现具体验证逻辑
     */
    template <typename T>
    class CConfigValidator
    {
    public:
        /**
         * @brief   : 验证并修复配置
         * @param    {T} &config：待验证的配置（会被修复）
         * @return   {ConfigValidateResult_S} 验证结果
         */
        static ConfigValidateResult_S validate(T& config)
        {
            /* 默认实现：不做任何验证，直接返回有效 */
            return ConfigValidateResult_S();
        }
    };

    /**
     * @brief   : 字符串验证辅助函数
     */
    class CStringValidator
    {
    public:
        /**
         * @brief   : 验证字符串长度
         * @param    {std::string} &value：待验证的字符串（会被截断）
         * @param    {size_t} maxLen：最大长度
         * @param    {std::string} &fieldName：字段名
         * @return   {FieldValidateResult_S} 验证结果
         */
        static FieldValidateResult_S validateLength(std::string& value,
                                                    size_t maxLen,
                                                    const std::string& fieldName)
        {
            if (value.length() <= maxLen)
            {
                return FieldValidateResult_S(fieldName, ValidateResult_E::VALID, "");
            }

            /* 超长，截断 */
            value = value.substr(0, maxLen);
            std::string msg = fieldName + ": 字符串过长，已截断为 " +
                              std::to_string(maxLen) + " 个字符";

            dlog_warn("[配置数据验证器] %s", msg.c_str());

            return FieldValidateResult_S(fieldName, ValidateResult_E::FIXED, msg);
        }

        /**
         * @brief   : 验证字符串非空
         * @param    {std::string} &value：待验证的字符串
         * @param    {std::string} &defaultVal：为空时的默认值
         * @param    {std::string} &fieldName：字段名
         * @return   {FieldValidateResult_S} 验证结果
         */
        static FieldValidateResult_S validateNotEmpty(std::string& value,
                                                      const std::string& defaultVal,
                                                      const std::string& fieldName)
        {
            if (!value.empty())
            {
                return FieldValidateResult_S(fieldName, ValidateResult_E::VALID, "");
            }

            value = defaultVal;
            std::string msg = fieldName + ": 为空字符串，设置为默认值 '" + defaultVal + "'";

            dlog_warn("[配置数据验证器] %s", msg.c_str());

            return FieldValidateResult_S(fieldName, ValidateResult_E::FIXED, msg);
        }
    };

} /* namespace ConfigCompat_NS */
