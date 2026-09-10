/*
 * @file capture_param_validation.h
 * @date 2026-09-08
 * @author Codex
 * @brief IPC 抓图任务参数的 JSON 类型和数值范围校验。
 * @修改记录 2026-09-08：仅在 IPC 校验两组抓图参数，不修改 SDK 解析流程。
 */
#pragma once

#include <cmath>
#include <cstdint>
#include "Json.h"

namespace CaptureValidation
{
static constexpr int CAPTURE_RESOLUTION_MIN = 1;
static constexpr int CAPTURE_RESOLUTION_MAX = 8192;
static constexpr int CAPTURE_NUMBER_MIN = 1;
static constexpr int CAPTURE_NUMBER_MAX = 120;
static constexpr int CAPTURE_FORMAT_MIN = 0;
static constexpr int CAPTURE_FORMAT_MAX = 1;
static constexpr int CAPTURE_QUALITY_MIN = 0;
static constexpr int CAPTURE_QUALITY_MAX = 2;
static constexpr int CAPTURE_TIME_UNIT_MIN = 0;
static constexpr int CAPTURE_TIME_UNIT_MAX = 4;
static constexpr int CAPTURE_INTERVAL_MIN = 1;
static constexpr int CAPTURE_INTERVAL_MAX[] = {86400000, 86400, 1440, 24, 365};
static constexpr int CAPTURE_BOOLEAN_MIN = 0;
static constexpr int CAPTURE_BOOLEAN_MAX = 1;
static constexpr int CAPTURE_JSON_TYPE_MASK = 0xFF;

/*
 * @brief 校验必填数值字段为有限整数，并处于闭区间内。
 * @author Codex
 * @param [in] pObject 当前 JSON 对象，只读借用，不转移所有权。
 * @param [in] pField 字段名。
 * @param [in] nMinimum 允许的最小值。
 * @param [in] nMaximum 允许的最大值。
 * @param [out] 无。
 * @return 字段为合法整数时返回 true，否则返回 false。
 */
static inline bool capture_validateInteger(Json::Object *pObject, const char *pField,
                                           int nMinimum, int nMaximum)
{
    Json::Object *pValue = Json::get(pObject, pField);
    if ((pValue == nullptr) || ((pValue->type & CAPTURE_JSON_TYPE_MASK) != cJSON_Number))
    {
        return false;
    }
    const double dValue = pValue->valuedouble;
    return std::isfinite(dValue) && (std::floor(dValue) == dValue) &&
           (dValue >= nMinimum) && (dValue <= nMaximum);
}

/*
 * @brief 校验一组抓图配置，包括关闭状态下保留的配置参数。
 * @author Codex
 * @param [in] pConfig 抓图配置对象，只读借用。
 * @param [out] 无。
 * @return 全部字段合法时返回 true，否则返回 false。
 */
static inline bool capture_validateConfig(Json::Object *pConfig)
{
    if ((pConfig == nullptr) || ((pConfig->type & CAPTURE_JSON_TYPE_MASK) != cJSON_Object))
    {
        return false;
    }
    Json::Object *pEnable = Json::get(pConfig, "Enable");
    if (pEnable == nullptr)
    {
        return false;
    }
    const int nEnableType = pEnable->type & CAPTURE_JSON_TYPE_MASK;
    if ((nEnableType != cJSON_True) && (nEnableType != cJSON_False) &&
        !capture_validateInteger(pConfig, "Enable", CAPTURE_BOOLEAN_MIN, CAPTURE_BOOLEAN_MAX))
    {
        return false;
    }
    if (!capture_validateInteger(pConfig, "Width", CAPTURE_RESOLUTION_MIN, CAPTURE_RESOLUTION_MAX) ||
        !capture_validateInteger(pConfig, "Height", CAPTURE_RESOLUTION_MIN, CAPTURE_RESOLUTION_MAX) ||
        !capture_validateInteger(pConfig, "PictureFormat", CAPTURE_FORMAT_MIN, CAPTURE_FORMAT_MAX) ||
        !capture_validateInteger(pConfig, "ImageQuality", CAPTURE_QUALITY_MIN, CAPTURE_QUALITY_MAX) ||
        !capture_validateInteger(pConfig, "Number", CAPTURE_NUMBER_MIN, CAPTURE_NUMBER_MAX))
    {
        return false;
    }
    Json::Object *pInterval = Json::get(pConfig, "TimeInterval");
    if ((pInterval == nullptr) || ((pInterval->type & CAPTURE_JSON_TYPE_MASK) != cJSON_Object) ||
        !capture_validateInteger(pInterval, "TimeUnit", CAPTURE_TIME_UNIT_MIN, CAPTURE_TIME_UNIT_MAX))
    {
        return false;
    }
    const int nTimeUnit = static_cast<int>(Json::get(pInterval, "TimeUnit")->valuedouble);
    return capture_validateInteger(pInterval, "Interval", CAPTURE_INTERVAL_MIN,
                                   CAPTURE_INTERVAL_MAX[nTimeUnit]);
}

/*
 * @brief 同时校验定时抓图和事件抓图，任何一组不合法均拒绝整个请求。
 * @author Codex
 * @param [in] pRoot 已解析的配置根对象，只读借用。
 * @param [out] 无。
 * @return 两组配置均合法时返回 true，否则返回 false。
 */
static inline bool capture_validateParams(Json::Object *pRoot)
{
    if ((pRoot == nullptr) || ((pRoot->type & CAPTURE_JSON_TYPE_MASK) != cJSON_Object))
    {
        return false;
    }
    Json::Object *pTiming = Json::get(pRoot, "Timing");
    Json::Object *pEvent = Json::get(pRoot, "Event");
    return capture_validateConfig(pTiming) && capture_validateConfig(pEvent);
}
}
