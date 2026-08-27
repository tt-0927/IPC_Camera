/**
 * @FilePath     : config_migrator.h
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2026-01-13
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-01-15 11:00:22
 * @Description  : 配置版本迁移框架
 */

#pragma once

#include <string>
#include <functional>
#include <vector>
#include <map>
#include "config_version.h"
#include "config_validator.h"
#include "Json.h"

namespace ConfigCompat_NS
{

    /**
     * @brief   : 迁移结果枚举
     */
    typedef enum class _MigrateResult_E_
    {
        SUCCESS = 0,      /* 迁移成功 */
        NO_MIGRATION,     /* 无需迁移 */
        PARTIAL_SUCCESS,  /* 部分成功（某些字段无法迁移） */
        FAILED,           /* 迁移失败 */
        RESET_TO_DEFAULT, /* 重置为默认值 */
    } MigrateResult_E;

    /**
     * @brief   : 迁移日志记录
     */
    typedef struct _MigrateLog_S_
    {
        ConfigVersion_S stFromVersion; /* 源版本 */
        ConfigVersion_S stToVersion;   /* 目标版本 */
        MigrateResult_E enResult;      /* 迁移结果 */
        std::string strMessage;        /* 日志消息 */

        _MigrateLog_S_()
            : stFromVersion(), stToVersion(), enResult(MigrateResult_E::SUCCESS), strMessage("")
        {
        }
    } MigrateLog_S;

    /**
     * @brief   : 单步迁移函数类型定义
     * @note    : 从 fromVersion 迁移到 toVersion
     *            参数：pRootJson - JSON对象指针
     *            返回：是否迁移成功
     */
    using MigrateStepFunc = std::function<bool(Json::Object* pRootJson)>;

    /**
     * @brief   : 迁移步骤定义
     */
    typedef struct _MigrateStep_S_
    {
        ConfigVersion_S stFromVersion; /* 源版本 */
        ConfigVersion_S stToVersion;   /* 目标版本 */
        MigrateStepFunc fnMigrate;     /* 迁移函数 */
        std::string strDescription;    /* 迁移描述 */

        _MigrateStep_S_() : stFromVersion(), stToVersion(), fnMigrate(nullptr), strDescription("")
        {
        }

        _MigrateStep_S_(const ConfigVersion_S& from,
                        const ConfigVersion_S& to,
                        MigrateStepFunc func,
                        const std::string& desc)
            : stFromVersion(from), stToVersion(to), fnMigrate(func), strDescription(desc)
        {
        }
    } MigrateStep_S;

    /**
     * @brief   : 配置迁移执行器
     * @note    : 提供标准的迁移流程实现，避免在每个特化类中重复代码
     *            特化类可以直接调用 execute() 方法，只需提供配置类型名称和迁移步骤
     */
    class CMigrateExecutor
    {
    public:
        /**
         * @brief   : 执行标准迁移流程
         * @param    {Json::Object*} pRootJson：JSON对象
         * @param    {ConfigVersion_S} &fromVersion：源版本
         * @param    {ConfigVersion_S} &toVersion：目标版本
         * @param    {std::string} &strConfigType：配置类型名称（用于日志前缀）
         * @param    {std::vector<MigrateStep_S>} &vecSteps：迁移步骤列表
         * @return   {MigrateResult_E} 迁移结果
         */
        static inline MigrateResult_E execute(Json::Object* pRootJson,
                                              const ConfigVersion_S& fromVersion,
                                              const ConfigVersion_S& toVersion,
                                              const std::string& strConfigType,
                                              const std::vector<MigrateStep_S>& vecSteps)
        {
            if (fromVersion == toVersion)
            {
                return MigrateResult_E::NO_MIGRATION;
            }

            if (vecSteps.empty())
            {
                return MigrateResult_E::NO_MIGRATION;
            }

            dlog_info("[%s] 开始配置迁移: %s -> %s",
                      strConfigType.c_str(),
                      fromVersion.toString().c_str(),
                      toVersion.toString().c_str());

            ConfigVersion_S currentVer = fromVersion;
            bool bAnyMigration = false;
            bool bAllSuccess = true;

            while (currentVer < toVersion)
            {
                bool bFoundStep = false;

                for (const auto& step : vecSteps)
                {
                    if (step.stFromVersion == currentVer && step.stToVersion <= toVersion)
                    {
                        dlog_info("[%s] 执行迁移步骤: %s -> %s, 描述: %s",
                                  strConfigType.c_str(),
                                  step.stFromVersion.toString().c_str(),
                                  step.stToVersion.toString().c_str(),
                                  step.strDescription.c_str());

                        if (step.fnMigrate && step.fnMigrate(pRootJson))
                        {
                            dlog_info("[%s] 迁移步骤执行成功", strConfigType.c_str());
                            currentVer = step.stToVersion;
                            bAnyMigration = true;
                            bFoundStep = true;
                            break;
                        }
                        else
                        {
                            dlog_warn("[%s] 迁移步骤执行失败", strConfigType.c_str());
                            bAllSuccess = false;
                            bFoundStep = true;
                            break;
                        }
                    }
                }

                if (!bFoundStep)
                {
                    break;
                }
            }

            if (bAnyMigration)
            {
                dlog_info("[%s] 配置迁移完成: %s -> %s",
                          strConfigType.c_str(),
                          fromVersion.toString().c_str(),
                          currentVer.toString().c_str());
            }

            if (!bAnyMigration)
            {
                return MigrateResult_E::NO_MIGRATION;
            }

            return bAllSuccess ? MigrateResult_E::SUCCESS : MigrateResult_E::PARTIAL_SUCCESS;
        }
    };

    /**
     * @brief   : 配置迁移器基类模板
     * @note    : 每个配置类型需要特化此模板，注册迁移步骤
     */
    template <typename T>
    class CConfigMigrator
    {
    public:
        /**
         * @brief   : 获取当前代码支持的配置版本
         * @return   {ConfigVersion_S} 当前版本
         */
        static ConfigVersion_S getCurrentVersion()
        {
            /* 默认版本为 1.0.0 */
            return ConfigVersion_S(1, 0, 0);
        }

        /**
         * @brief   : 获取配置类型标识符
         * @return   {std::string} 类型标识符
         */
        static std::string getConfigType()
        {
            return "unknown";
        }

        /**
         * @brief   : 获取迁移步骤列表
         * @return   {std::vector<MigrateStep_S>} 迁移步骤
         */
        static std::vector<MigrateStep_S> getMigrateSteps()
        {
            /* 默认无迁移步骤 */
            return std::vector<MigrateStep_S>();
        }

        /**
         * @brief   : 执行JSON数据迁移
         * @note    : 默认实现调用 CMigrateExecutor::execute()，特化类可重写此函数实现自定义逻辑
         * @param    {Json::Object*} pRootJson：JSON对象
         * @param    {ConfigVersion_S} &fromVersion：源版本
         * @param    {ConfigVersion_S} &toVersion：目标版本
         * @return   {MigrateResult_E} 迁移结果
         */
        static MigrateResult_E migrate(Json::Object* pRootJson,
                                       const ConfigVersion_S& fromVersion,
                                       const ConfigVersion_S& toVersion)
        {
            return CMigrateExecutor::execute(pRootJson, fromVersion, toVersion, getConfigType(), getMigrateSteps());
        }
    };

    /**
     * @brief   : 通用迁移辅助函数：添加新字段
     * @param    {Json::Object*} pRootJson：JSON对象
     * @param    {std::string} &key：字段名
     * @param    {T} defaultValue：默认值
     */
    template <typename T>
    inline void addFieldIfMissing(Json::Object* pRootJson, const std::string& key, T defaultValue)
    {
        Json::Object* pField = Json::get(pRootJson, key);
        if (!pField)
        {
            Json::add(pRootJson, key, defaultValue);
        }
    }

    /**
     * @brief   : 通用迁移辅助函数：重命名字段
     * @param    {Json::Object*} pRootJson：JSON对象
     * @param    {std::string} &oldKey：旧字段名
     * @param    {std::string} &newKey：新字段名
     */
    inline void renameField(Json::Object* pRootJson,
                            const std::string& oldKey,
                            const std::string& newKey)
    {
        Json::Object* pOldField = Json::get(pRootJson, oldKey);
        if (pOldField)
        {
            /* 复制值到新字段 */
            int nIntValue = 0;
            std::string strValue;
            bool bBoolValue = false;
            double dDoubleValue = 0.0;

            if (Json::get(pRootJson, oldKey, nIntValue))
            {
                Json::add(pRootJson, newKey, nIntValue);
            }
            else if (Json::get(pRootJson, oldKey, strValue))
            {
                Json::add(pRootJson, newKey, strValue);
            }
            else if (Json::get(pRootJson, oldKey, bBoolValue))
            {
                Json::add(pRootJson, newKey, bBoolValue);
            }
            else if (Json::get(pRootJson, oldKey, dDoubleValue))
            {
                Json::add(pRootJson, newKey, dDoubleValue);
            }

            /* 删除旧字段 */
            Json::remove(pRootJson, oldKey);
        }
    }

    /**
     * @brief   : 通用迁移辅助函数：删除字段
     * @param    {Json::Object*} pRootJson：JSON对象
     * @param    {std::string} &key：字段名
     */
    inline void removeField(Json::Object* pRootJson, const std::string& key)
    {
        Json::remove(pRootJson, key);
    }

    /**
     * @brief   : 通用迁移辅助函数：修复数值范围
     * @param    {Json::Object*} pRootJson：JSON对象
     * @param    {std::string} &key：字段名
     * @param    {int} minVal：最小值
     * @param    {int} maxVal：最大值
     * @param    {int} defaultVal：超出范围时的默认值
     */
    inline void fixIntRange(Json::Object* pRootJson,
                            const std::string& key,
                            int minVal,
                            int maxVal,
                            int defaultVal)
    {
        int value = 0;
        if (Json::get(pRootJson, key, value))
        {
            if (value < minVal || value > maxVal)
            {
                Json::update(pRootJson, key, defaultVal);
            }
        }
    }

} /* namespace ConfigCompat_NS */
