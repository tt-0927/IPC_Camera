/*
 * @Author: lianghy lianghy@kfb.cn
 * @Date: 2026-01-23 15:58:38
 * @LastEditors: lianghy lianghy@kfb.cn
 * @LastEditTime: 2026-01-24 10:32:15
 * @FilePath: /hisi/share/ipc_share/common/config_compat/record_config_compat.h
 * @Description: 这是默认设置,请设置`customMade`, 打开koroFileHeader查看配置 进行设置: https://github.com/OBKoro1/koro1FileHeader/wiki/%E9%85%8D%E7%BD%AE
 */
#pragma once

#include "config_version.h"
#include "config_validator.h"
#include "config_migrator.h"
#include "dlog.h"
#include "record_define.h"

/* 最小码流类型值 */
#define STREAM_TYPE_MIN 0
/* 最大码流类型值 */
#define STREAM_TYPE_MAX 1

/* 最小录制布放计划类型值 */
#define RECORD_PLAN_TYPE_MIN 1
/* 最大录制布放计划类型值 */
#define RECORD_PLAN_TYPE_MAX 2

/* 最小录制布放时间值 */
#define RECORD_TIME_MIN 0
/* 最大录制布放时间值 */
#define RECORD_TIME_MAX 86400


namespace ConfigCompat_NS
{
    //*===========================================================================*/
    //*                     AdvancedParam_S 版本迁移器特化                          */
    //*===========================================================================*/

    /**
     * @brief   : AdvancedParam_S
     * @note    : 定义版本号、迁移步骤等
     */
    template <>
    class CConfigMigrator<Record_NS::AdvancedParam_S>
    {
    public:
        /**
         * @brief   : 获取当前代码支持的AdvancedParam版本
         * @note    : 每次结构体变更时更新此版本号(eg. 1.1.0: 新增 xxx 字段)
         *            - 1.0.0: 初始版本
         * @return   {ConfigVersion_S} 当前版本
         */
        static ConfigVersion_S getCurrentVersion()
        {
            return ConfigVersion_S(1, 0, 0);
        }

        /**
         * @brief   : 获取配置类型标识符
         * @return   {std::string} 类型标识符
         */
        static std::string getConfigType()
        {
            return "AdvancedParam";
        }

        /**
         * @brief   : 获取迁移步骤列表
         * @return   {std::vector<MigrateStep_S>} 迁移步骤
         */
        static std::vector<MigrateStep_S> getMigrateSteps()
        {
            std::vector<MigrateStep_S> steps;

            /* 0.0.0 -> 1.0.0: 无版本到初始版本 */
            steps.emplace_back(
                ConfigVersion_S(0, 0, 0),
                ConfigVersion_S(1, 0, 0),
                [](Json::Object* pRootJson) -> bool
                {
                    /* 旧配置无版本号，直接标记为1.0.0 */
                    return true;
                },
                "初始化版本信息");

            return steps;
        }

        /**
         * @brief   : 执行迁移
         * @note    : 使用 CMigrateExecutor 提供的标准迁移流程
         */
        static MigrateResult_E migrate(Json::Object* pRootJson,
                                       const ConfigVersion_S& fromVersion,
                                       const ConfigVersion_S& toVersion)
        {
            return CMigrateExecutor::execute(pRootJson, fromVersion, toVersion, getConfigType(), getMigrateSteps());
        }
    };

    //*===========================================================================*/
    //*                     AdvancedParam_S 验证器特化                              */
    //*===========================================================================*/

    /**
     * @brief   : AdvancedParam_S的验证器特化
     * @note    : 定义各字段的有效范围和默认值
     */
    template <>
    class CConfigValidator<Record_NS::AdvancedParam_S>
    {
    public:
        /**
         * @brief   : 验证并修复AdvancedParam_S
         * @param    {AdvancedParam_S} &config：待验证的配置
         * @return   {ConfigValidateResult_S} 验证结果
         */
        static ConfigValidateResult_S validate(Record_NS::AdvancedParam_S& config)
        {
            using namespace Record_NS;
            ConfigValidateResult_S result;

            /* 验证预录时间范围枚举有效性 */
            result.addFieldResult(
                CRangeValidator<Record_NS::RecordPreTime_E>::validateEnum(config.ePreTime,
                                                                        { Record_NS::RecordPreTime_E::RECORD_PRE_TIME_0_SEC,
                                                                            Record_NS::RecordPreTime_E::RECORD_PRE_TIME_5_SEC,
                                                                            Record_NS::RecordPreTime_E::RECORD_PRE_TIME_10_SEC,
                                                                            Record_NS::RecordPreTime_E::RECORD_PRE_TIME_15_SEC,
                                                                            Record_NS::RecordPreTime_E::RECORD_PRE_TIME_20_SEC,
                                                                            Record_NS::RecordPreTime_E::RECORD_PRE_TIME_25_SEC,
                                                                            Record_NS::RecordPreTime_E::RECORD_PRE_TIME_30_SEC,
                                                                            Record_NS::RecordPreTime_E::RECORD_PRE_TIME_UNLIMITED },
                                                                        Record_NS::RecordPreTime_E::RECORD_PRE_TIME_0_SEC,
                                                                        "ePreTime"));

            /* 验证录制延时范围 */
            result.addFieldResult(
                CRangeValidator<Record_NS::RecordDelayTime_E>::validateEnum(config.eDelayTime,
                                                                        { Record_NS::RecordDelayTime_E::RECORD_DELAY_TIME_5_SEC,
                                                                            Record_NS::RecordDelayTime_E::RECORD_DELAY_TIME_10_SEC,
                                                                            Record_NS::RecordDelayTime_E::RECORD_DELAY_TIME_1_MIN,
                                                                            Record_NS::RecordDelayTime_E::RECORD_DELAY_TIME_2_MIN,
                                                                            Record_NS::RecordDelayTime_E::RECORD_DELAY_TIME_5_MIN,
                                                                            Record_NS::RecordDelayTime_E::RECORD_DELAY_TIME_10_MIN },
                                                                        Record_NS::RecordDelayTime_E::RECORD_DELAY_TIME_5_SEC,
                                                                        "eDelayTime"));
            return result;
        }
    };

    //*===========================================================================*/
    //*                     Schedule_S 版本迁移器特化                          */
    //*===========================================================================*/

    /**
     * @brief   : Schedule_S
     * @note    : 定义版本号、迁移步骤等
     */
    template <>
    class CConfigMigrator<Record_NS::Schedule_S>
    {
    public:
        /**
        * @brief   : 获取当前代码支持的AdvancedParam版本
        * @note    : 每次结构体变更时更新此版本号(eg. 1.1.0: 新增 xxx 字段)
        *            - 1.0.0: 初始版本
        * @return   {ConfigVersion_S} 当前版本
        */
        static ConfigVersion_S getCurrentVersion()
        {
            return ConfigVersion_S(1, 0, 0);
        }

        /**
        * @brief   : 获取配置类型标识符
        * @return   {std::string} 类型标识符
        */
        static std::string getConfigType()
        {
            return "RecordSchedule";
        }

        /**
        * @brief   : 获取迁移步骤列表
        * @return   {std::vector<MigrateStep_S>} 迁移步骤
        */
        static std::vector<MigrateStep_S> getMigrateSteps()
        {
            std::vector<MigrateStep_S> steps;

            /* 0.0.0 -> 1.0.0: 无版本到初始版本 */
            steps.emplace_back(
                ConfigVersion_S(0, 0, 0),
                ConfigVersion_S(1, 0, 0),
                [](Json::Object* pRootJson) -> bool
                {
                    /* 旧配置无版本号，直接标记为1.0.0 */
                    return true;
                },
                "初始化版本信息");

            return steps;
        }

        /**
        * @brief   : 执行迁移
        * @note    : 使用 CMigrateExecutor 提供的标准迁移流程
        */
        static MigrateResult_E migrate(Json::Object* pRootJson,
                                    const ConfigVersion_S& fromVersion,
                                    const ConfigVersion_S& toVersion)
        {
            return CMigrateExecutor::execute(pRootJson, fromVersion, toVersion, getConfigType(), getMigrateSteps());
        }
    };

    //*===========================================================================*/
    //*                     Schedule_S 验证器特化                              */
    //*===========================================================================*/

    /**
    * @brief   : Schedule_S的验证器特化
    * @note    : 定义各字段的有效范围和默认值
    */
    template <>
    class CConfigValidator<Record_NS::Schedule_S>
    {
    public:
        /**
        * @brief   : 验证并修复Schedule_S
        * @param    {Schedule_S} &config：待验证的配置
        * @return   {ConfigValidateResult_S} 验证结果
        */
        static ConfigValidateResult_S validate(Record_NS::Schedule_S& config)
        {
            using namespace Record_NS;
            ConfigValidateResult_S result;

            /* 验证预录时间范围 [PRE_TIME_MIN, PRE_TIME_MIN] */
            for(unsigned int i = 0; i < config.daySchedules.size(); i++)
            {
                auto &stDaySchedule = config.daySchedules.at(i);

                result.addFieldResult(CRangeValidator<DaySchedule_S>::validateEnumRange(stDaySchedule.enDayOfWeek,
                    DayOfWeek_E::Monday,
                    DayOfWeek_E::Sunday,
                    (DayOfWeek_E)i,
                    "enDayOfWeek"));

                for(auto &stRecordTime : stDaySchedule.recordTimes)
                {
                    result.addFieldResult(CRangeValidator<int>::validate(stRecordTime.nType, RECORD_PLAN_TYPE_MIN, RECORD_PLAN_TYPE_MAX, 1, "nType"));
                    result.addFieldResult(CRangeValidator<int>::validate(stRecordTime.nStartTime, RECORD_TIME_MIN, RECORD_TIME_MAX, RECORD_TIME_MIN, "nStartTime"));
                    result.addFieldResult(CRangeValidator<int>::validate(stRecordTime.nEndTime, RECORD_TIME_MIN, RECORD_TIME_MAX, RECORD_TIME_MAX, "nEndTime"));
                }
            }

            return result;
        }
    };

} /* namespace ConfigCompat_NS */