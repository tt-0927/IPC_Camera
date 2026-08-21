/**
 * @FilePath     : isp_profile_config_loader.cpp
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2026-07-27 10:19:37
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-07-27 20:24:09
 * @Description  : Hi3516 ISP参数映射与运行策略配置加载实现
 */

#include "isp_profile_config_loader.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <cstdint>
#include <sstream>
#include <string>
#include <utility>

#include "IpcRet.h"
#include "dlog.h"
#include "ini_disposed.h"

namespace
{
/* 参数映射配置文件名。 */
constexpr const char *ISP_PARAM_MAPPING_FILE_NAME = "config_isp_param_mapping.ini";
/* 运行策略配置文件名。 */
constexpr const char *ISP_RUNTIME_POLICY_FILE_NAME = "config_isp_runtime_policy.ini";
/* 参数映射配置当前支持的结构版本。 */
constexpr int ISP_PARAM_MAPPING_SCHEMA_VERSION = 3;
/* 运行策略配置当前支持的结构版本。 */
constexpr int ISP_RUNTIME_POLICY_SCHEMA_VERSION = 1;
/* 网页百分比参数允许的偏移范围。 */
constexpr int USER_OFFSET_MIN = -100;
constexpr int USER_OFFSET_MAX = 100;
/* 网页百分比参数的上限。 */
constexpr int USER_VALUE_MAX = 100;
/* MPP曝光补偿为8位数值。 */
constexpr int EXPOSURE_COMPENSATION_MIN = 0;
constexpr int EXPOSURE_COMPENSATION_MAX = 255;
/* 当前Scene配置最多提供三个video_mode槽位。 */
constexpr int SCENE_INDEX_MIN = 0;
constexpr int SCENE_INDEX_MAX = static_cast<int>(ISP_TUNING_SCENE_COUNT) - 1;
/* RG/BG统计值按8位归一化，允许保留到四倍增益。 */
constexpr int COLOR_RATIO_MAX = 1024;
/* DRC手动强度的配置保护上限。 */
constexpr int DRC_STRENGTH_MAX = 1023;
/* MPP CSC字段使用8位数值。 */
constexpr int CSC_VALUE_MAX = 255;
/* MPP锐度增益的配置保护上限。 */
constexpr int SHARPNESS_VALUE_MAX = 1023;
/* MPP曝光tolerance字段的取值上限。 */
constexpr int HLC_TOLERANCE_MAX = 4;
/* MPP AWB手动增益字段采用12位无符号定点数。 */
constexpr int AWB_GAIN_MAX = 0xFFF;
/* 3DNR强度字段采用8位无符号数。 */
constexpr int NR_VALUE_MAX = 255;
/* MPP曝光时间枚举对应固定16级补偿表。 */
constexpr size_t EXPOSURE_COMPENSATION_COUNT = 16;

/**
 * @brief   : 输出配置字段错误并保留原返回码
 * @param    {const std::string &} strFilePath：配置文件路径
 * @param    {const std::string &} strFieldGroup：字段或配置段
 * @param    {const CIniReader &} stReader：配置解析器
 * @param    {int} nRet：原返回码
 * @param    {const std::string &} strValidationReason：业务校验失败的具体规则，可为空
 * @return   {int} 原返回码
 */
int report_field_error(const std::string &strFilePath,
                       const std::string &strFieldGroup,
                       const CIniReader &stReader,
                       int nRet,
                       const std::string &strValidationReason = std::string())
{
    const std::string strReason = !strValidationReason.empty() ? strValidationReason
                                                               : (stReader.last_error().empty() ? "字段值超出允许范围或组合不合法"
                                                                                                : stReader.last_error());
    dlog_error("ISP配置字段非法, 文件:%s, 字段:%s, ret:%d, 原因:%s",
               strFilePath.c_str(),
               strFieldGroup.c_str(),
               nRet,
               strReason.c_str());
    return nRet;
}

/**
 * @brief   : 拼接固定文件名
 * @param    {const std::string &} strDir：目录
 * @param    {const char *} pFileName：文件名
 * @return   {std::string} 完整路径
 */
std::string join_path(const std::string &strDir, const char *pFileName)
{
    if (!strDir.empty() && strDir.back() == '/')
    {
        return strDir + pFileName;
    }
    return strDir + "/" + pFileName;
}

/**
 * @brief   : 将设备型号规范为INI段后缀
 * @param    {const std::string &} strDeviceType：设备型号
 * @return   {std::string} 小写且使用下划线分隔的段后缀
 */
std::string normalize_device_suffix(const std::string &strDeviceType)
{
    std::string strSuffix = strDeviceType;
    std::transform(strSuffix.begin(),
                   strSuffix.end(),
                   strSuffix.begin(),
                   [](unsigned char chValue)
                   {
                       if (chValue == '-')
                       {
                           return '_';
                       }
                       return static_cast<char>(std::tolower(chValue));
                   });
    return strSuffix;
}

/**
 * @brief   : 优先选择设备专用配置段
 * @param    {const CIniReader &} stReader：配置解析器
 * @param    {const std::string &} strBaseSection：通用配置段
 * @param    {const std::string &} strDeviceType：设备型号
 * @return   {std::string} 设备专用段或通用段
 * @note    : 专用段格式为`通用段.tv_3852h`，用于共享Sensor目录保留设备差异。
 */
std::string resolve_device_section(const CIniReader &stReader,
                                   const std::string &strBaseSection,
                                   const std::string &strDeviceType)
{
    const std::string strDeviceSection = strBaseSection + "." + normalize_device_suffix(strDeviceType);
    return stReader.has_section(strDeviceSection) ? strDeviceSection : strBaseSection;
}

/**
 * @brief   : 校验配置结构版本
 * @param    {const CIniReader &} stReader：已加载解析器
 * @param    {int} nExpectedVersion：调用方要求的结构版本
 * @param    {std::string &} strValidationReason：业务校验失败的具体规则
 * @return   {int} OK：版本支持，ERR_PARSE：版本不支持
 */
int validate_schema_version(const CIniReader &stReader, int nExpectedVersion, std::string &strValidationReason)
{
    int nSchemaVersion = 0;
    int nRet = stReader.get_int("meta", "schema_version", nSchemaVersion);
    if (nRet != OK)
    {
        return nRet;
    }
    if (nSchemaVersion != nExpectedVersion)
    {
        strValidationReason = "schema_version不支持，期望:" + std::to_string(nExpectedVersion) +
                              ", 实际:" + std::to_string(nSchemaVersion);
        return ERR_PARSE;
    }
    return OK;
}

/**
 * @brief   : 读取通用线性映射
 * @param    {const CIniReader &} stReader：已加载解析器
 * @param    {const std::string &} strSection：配置段
 * @param    {int} nAllowedMax：目标MPP字段允许的最大值
 * @param    {IspParamMapping_S &} stMapping：输出映射
 * @param    {std::string &} strValidationReason：业务校验失败的具体规则
 * @return   {int} OK：成功，非OK：字段缺失或非法
 */
int read_mapping(const CIniReader &stReader,
                 const std::string &strSection,
                 int nAllowedMax,
                 IspParamMapping_S &stMapping,
                 std::string &strValidationReason)
{
    int nSystemMin = 0;
    int nSystemMax = 0;
    int nUserOffset = 0;
    int nRet = stReader.get_int(strSection, "system_min", nSystemMin);
    if (nRet != OK)
    {
        return nRet;
    }
    nRet = stReader.get_int(strSection, "system_max", nSystemMax);
    if (nRet != OK)
    {
        return nRet;
    }
    nRet = stReader.get_int(strSection, "user_offset", nUserOffset);
    if (nRet != OK)
    {
        return nRet;
    }
    if (nSystemMin < 0 || nSystemMin >= nSystemMax || nSystemMax > nAllowedMax || nUserOffset < USER_OFFSET_MIN ||
        nUserOffset > USER_OFFSET_MAX)
    {
        strValidationReason = "system_min必须不小于0且小于system_max；system_max不能超过" + std::to_string(nAllowedMax) +
                              "；user_offset必须位于" + std::to_string(USER_OFFSET_MIN) + "~" + std::to_string(USER_OFFSET_MAX);
        return ERR_PARAM;
    }

    stMapping.nSystemMin = static_cast<unsigned int>(nSystemMin);
    stMapping.nSystemMax = static_cast<unsigned int>(nSystemMax);
    stMapping.nUserOffset = nUserOffset;
    return OK;
}

/**
 * @brief   : 读取锐度映射和工作模式
 * @param    {const CIniReader &} stReader：已加载解析器
 * @param    {const std::string &} strSection：配置段
 * @param    {SharpenTuningProfile_S &} stSharpen：输出锐度映射
 * @param    {std::string &} strValidationReason：业务校验失败的具体规则
 * @return   {int} OK：成功，非OK：字段缺失或非法
 */
int read_sharpen_mapping(const CIniReader &stReader,
                         const std::string &strSection,
                         SharpenTuningProfile_S &stSharpen,
                         std::string &strValidationReason)
{
    IspParamMapping_S stMapping;
    int nRet = read_mapping(stReader, strSection, SHARPNESS_VALUE_MAX, stMapping, strValidationReason);
    if (nRet != OK)
    {
        return nRet;
    }

    std::string strOperationMode;
    nRet = stReader.get_string(strSection, "operation_mode", strOperationMode);
    if (nRet != OK)
    {
        return nRet;
    }
    if (strOperationMode != "manual" && strOperationMode != "auto")
    {
        strValidationReason = "operation_mode仅支持manual或auto";
        return ERR_PARAM;
    }

    stSharpen.nMin = stMapping.nSystemMin;
    stSharpen.nMax = stMapping.nSystemMax;
    stSharpen.nOffset = stMapping.nUserOffset;
    stSharpen.bUseAutoMode = (strOperationMode == "auto");
    return OK;
}

/**
 * @brief   : 读取强光抑制的反向百分比映射和关闭值
 * @param    {const CIniReader &} stReader：配置解析器
 * @param    {const std::string &} strSection：配置段
 * @param    {HlcTuningProfile_S &} stHlc：输出强光抑制映射
 * @param    {std::string &} strValidationReason：业务校验失败的具体规则
 * @return   {int} OK：成功，非OK：字段缺失或非法
 */
int read_hlc_mapping(const CIniReader &stReader,
                     const std::string &strSection,
                     HlcTuningProfile_S &stHlc,
                     std::string &strValidationReason)
{
    int nSystemMin = 0;
    int nSystemMax = 0;
    int nDisabledValue = 0;
    int nRet = stReader.get_int(strSection, "system_min", nSystemMin);
    if (nRet != OK)
    {
        return nRet;
    }
    nRet = stReader.get_int(strSection, "system_max", nSystemMax);
    if (nRet != OK)
    {
        return nRet;
    }
    nRet = stReader.get_int(strSection, "disabled_value", nDisabledValue);
    if (nRet != OK)
    {
        return nRet;
    }
    if (nSystemMin < 0 || nSystemMin >= nSystemMax || nSystemMax > HLC_TOLERANCE_MAX || nDisabledValue < nSystemMin ||
        nDisabledValue > nSystemMax)
    {
        strValidationReason = "system_min必须不小于0且小于system_max；system_max不能超过" + std::to_string(HLC_TOLERANCE_MAX) +
                              "；disabled_value必须位于system_min~system_max";
        return ERR_PARAM;
    }

    stHlc.nSystemMin = static_cast<unsigned int>(nSystemMin);
    stHlc.nSystemMax = static_cast<unsigned int>(nSystemMax);
    stHlc.nDisabledValue = static_cast<unsigned int>(nDisabledValue);
    return OK;
}

/**
 * @brief   : 读取单个AWB预设的四通道手动增益
 * @param    {const CIniReader &} stReader：配置解析器
 * @param    {const std::string &} strSection：配置段
 * @param    {AwbPresetGain_S &} stGain：输出预设增益
 * @param    {std::string &} strValidationReason：业务校验失败的具体规则
 * @return   {int} OK：成功，非OK：字段缺失或超出MPP范围
 */
int read_awb_preset_gain(const CIniReader &stReader,
                         const std::string &strSection,
                         AwbPresetGain_S &stGain,
                         std::string &strValidationReason)
{
    int nRGain = 0;
    int nGrGain = 0;
    int nGbGain = 0;
    int nBGain = 0;
    int nRet = stReader.get_int(strSection, "r_gain", nRGain);
    if (nRet != OK)
    {
        return nRet;
    }
    nRet = stReader.get_int(strSection, "gr_gain", nGrGain);
    if (nRet != OK)
    {
        return nRet;
    }
    nRet = stReader.get_int(strSection, "gb_gain", nGbGain);
    if (nRet != OK)
    {
        return nRet;
    }
    nRet = stReader.get_int(strSection, "b_gain", nBGain);
    if (nRet != OK)
    {
        return nRet;
    }
    if (nRGain < 0 || nRGain > AWB_GAIN_MAX || nGrGain < 0 || nGrGain > AWB_GAIN_MAX || nGbGain < 0 || nGbGain > AWB_GAIN_MAX ||
        nBGain < 0 || nBGain > AWB_GAIN_MAX)
    {
        strValidationReason = "r_gain、gr_gain、gb_gain和b_gain必须位于0~" + std::to_string(AWB_GAIN_MAX);
        return ERR_PARAM;
    }

    stGain.nRGain = static_cast<unsigned int>(nRGain);
    stGain.nGrGain = static_cast<unsigned int>(nGrGain);
    stGain.nGbGain = static_cast<unsigned int>(nGbGain);
    stGain.nBGain = static_cast<unsigned int>(nBGain);
    return OK;
}

/**
 * @brief   : 读取网页百分比到MPP整数参数值的缩放公式
 * @param    {const CIniReader &} stReader：配置解析器
 * @param    {const std::string &} strSection：配置段
 * @param    {ScaledOffsetMapping_S &} stMapping：输出缩放规则
 * @param    {std::string &} strValidationReason：业务校验失败的具体规则
 * @return   {int} OK：成功，非OK：字段缺失或计算结果越界
 * @note    : 保留偏移后直接缩放的历史公式，不能套用会二次截断的通用映射。
 */
int read_scaled_offset_mapping(const CIniReader &stReader,
                               const std::string &strSection,
                               ScaledOffsetMapping_S &stMapping,
                               std::string &strValidationReason)
{
    int nUserOffset = 0;
    int nMultiplier = 0;
    int nDivisor = 0;
    int nRet = stReader.get_int(strSection, "user_offset", nUserOffset);
    if (nRet != OK)
    {
        return nRet;
    }
    nRet = stReader.get_int(strSection, "multiplier", nMultiplier);
    if (nRet != OK)
    {
        return nRet;
    }
    nRet = stReader.get_int(strSection, "divisor", nDivisor);
    if (nRet != OK)
    {
        return nRet;
    }
    if (nUserOffset < 0 || nUserOffset > USER_OFFSET_MAX || nMultiplier <= 0 || nMultiplier > NR_VALUE_MAX || nDivisor <= 0)
    {
        strValidationReason = "user_offset必须位于0~" + std::to_string(USER_OFFSET_MAX) + "；multiplier必须位于1~" +
                              std::to_string(NR_VALUE_MAX) + "；divisor必须大于0";
        return ERR_PARAM;
    }

    const int64_t nMinOutput = (static_cast<int64_t>(nUserOffset) * nMultiplier) / nDivisor;
    const int64_t nMaxOutput = (static_cast<int64_t>(USER_VALUE_MAX + nUserOffset) * nMultiplier) / nDivisor;
    if (nMinOutput < 0 || nMaxOutput > NR_VALUE_MAX)
    {
        strValidationReason = "按(user_value + user_offset) * multiplier / divisor计算的结果必须位于0~" +
                              std::to_string(NR_VALUE_MAX);
        return ERR_PARAM;
    }

    stMapping.nUserOffset = nUserOffset;
    stMapping.nMultiplier = static_cast<unsigned int>(nMultiplier);
    stMapping.nDivisor = static_cast<unsigned int>(nDivisor);
    return OK;
}

/**
 * @brief   : 读取固定长度曝光补偿表
 * @param    {const CIniReader &} stReader：已加载解析器
 * @param    {const std::string &} strSection：配置段名称
 * @param    {const std::string &} strKey：配置项名称
 * @param    {int *} pValues：16项输出数组
 * @param    {std::string &} strValidationReason：业务校验失败的具体规则
 * @return   {int} OK：成功，ERR_PARSE/ERR_PARAM：失败
 */
int read_exposure_compensation(const CIniReader &stReader,
                               const std::string &strSection,
                               const std::string &strKey,
                               int *pValues,
                               std::string &strValidationReason)
{
    if (pValues == nullptr)
    {
        strValidationReason = "曝光补偿输出数组为空";
        return ERR_PARAM_NULL;
    }

    std::string strValues;
    int nRet = stReader.get_string(strSection, strKey, strValues);
    if (nRet != OK)
    {
        return nRet;
    }

    std::stringstream stStream(strValues);
    std::string strItem;
    size_t nIndex = 0;
    while (std::getline(stStream, strItem, ','))
    {
        if (nIndex >= EXPOSURE_COMPENSATION_COUNT)
        {
            strValidationReason = "曝光补偿必须恰好包含" + std::to_string(EXPOSURE_COMPENSATION_COUNT) + "项";
            return ERR_PARAM;
        }

        /* 使用标准流提取单项，随后严格确认没有残留字符。 */
        std::stringstream stValueStream(strItem);
        int nValue = 0;
        stValueStream >> nValue;
        stValueStream >> std::ws;
        if (stValueStream.fail() || !stValueStream.eof() || nValue < EXPOSURE_COMPENSATION_MIN ||
            nValue > EXPOSURE_COMPENSATION_MAX)
        {
            strValidationReason = "第" + std::to_string(nIndex + 1) + "项必须是" + std::to_string(EXPOSURE_COMPENSATION_MIN) +
                                  "~" + std::to_string(EXPOSURE_COMPENSATION_MAX) + "的整数";
            return ERR_PARSE;
        }
        pValues[nIndex++] = nValue;
    }
    if (nIndex != EXPOSURE_COMPENSATION_COUNT)
    {
        strValidationReason = "曝光补偿必须恰好包含" + std::to_string(EXPOSURE_COMPENSATION_COUNT) + "项";
        return ERR_PARAM;
    }
    return OK;
}

/**
 * @brief   : 加载参数映射配置
 * @param    {const std::string &} strFilePath：配置文件路径
 * @param    {const std::string &} strDeviceType：设备型号
 * @param    {Hi3516TuningProfile_S &} stProfile：输出画像
 * @return   {int} OK：成功，非OK：加载或校验失败
 */
int load_param_mapping(const std::string &strFilePath, const std::string &strDeviceType, Hi3516TuningProfile_S &stProfile)
{
    CIniReader stReader;
    int nRet = stReader.load(strFilePath);
    if (nRet != OK)
    {
        return nRet;
    }
    std::string strValidationReason;
    nRet = validate_schema_version(stReader, ISP_PARAM_MAPPING_SCHEMA_VERSION, strValidationReason);
    if (nRet != OK)
    {
        return report_field_error(strFilePath, "meta.schema_version", stReader, nRet, strValidationReason);
    }

    const std::array<std::string, ISP_TUNING_SCENE_COUNT> stSceneNames = { "day", "night_white", "night_ir" };
    for (size_t nIndex = 0; nIndex < stSceneNames.size(); ++nIndex)
    {
        IspSceneParamMapping_S &stSceneMapping = stProfile.stSceneParamMappings[nIndex];
        const std::string strBrightnessSection = resolve_device_section(stReader,
                                                                        "brightness." + stSceneNames[nIndex],
                                                                        strDeviceType);
        strValidationReason.clear();
        nRet = read_mapping(stReader, strBrightnessSection, CSC_VALUE_MAX, stSceneMapping.stBrightness, strValidationReason);
        if (nRet != OK)
        {
            return report_field_error(strFilePath, strBrightnessSection, stReader, nRet, strValidationReason);
        }
        const std::string strSharpnessSection = resolve_device_section(stReader,
                                                                       "sharpness." + stSceneNames[nIndex],
                                                                       strDeviceType);
        strValidationReason.clear();
        nRet = read_sharpen_mapping(stReader, strSharpnessSection, stSceneMapping.stSharpen, strValidationReason);
        if (nRet != OK)
        {
            return report_field_error(strFilePath, strSharpnessSection, stReader, nRet, strValidationReason);
        }
        const std::string strWdrSection = resolve_device_section(stReader, "wdr." + stSceneNames[nIndex], strDeviceType);
        strValidationReason.clear();
        nRet = read_mapping(stReader, strWdrSection, DRC_STRENGTH_MAX, stSceneMapping.stWdr, strValidationReason);
        if (nRet != OK)
        {
            return report_field_error(strFilePath, strWdrSection, stReader, nRet, strValidationReason);
        }
        const std::string strContrastSection = resolve_device_section(stReader,
                                                                      "contrast." + stSceneNames[nIndex],
                                                                      strDeviceType);
        strValidationReason.clear();
        nRet = read_mapping(stReader, strContrastSection, CSC_VALUE_MAX, stSceneMapping.stContrast, strValidationReason);
        if (nRet != OK)
        {
            return report_field_error(strFilePath, strContrastSection, stReader, nRet, strValidationReason);
        }
        const std::string strSaturationSection = resolve_device_section(stReader,
                                                                        "saturation." + stSceneNames[nIndex],
                                                                        strDeviceType);
        strValidationReason.clear();
        nRet = read_mapping(stReader, strSaturationSection, CSC_VALUE_MAX, stSceneMapping.stSaturation, strValidationReason);
        if (nRet != OK)
        {
            return report_field_error(strFilePath, strSaturationSection, stReader, nRet, strValidationReason);
        }
        const std::string strHlcSection = resolve_device_section(stReader, "hlc." + stSceneNames[nIndex], strDeviceType);
        strValidationReason.clear();
        nRet = read_hlc_mapping(stReader, strHlcSection, stSceneMapping.stHlc, strValidationReason);
        if (nRet != OK)
        {
            return report_field_error(strFilePath, strHlcSection, stReader, nRet, strValidationReason);
        }
        const std::string strAwbManualSection = resolve_device_section(stReader,
                                                                       "awb.manual." + stSceneNames[nIndex],
                                                                       strDeviceType);
        strValidationReason.clear();
        nRet = read_mapping(stReader, strAwbManualSection, AWB_GAIN_MAX, stSceneMapping.stAwb.stManualGain, strValidationReason);
        if (nRet != OK)
        {
            return report_field_error(strFilePath, strAwbManualSection, stReader, nRet, strValidationReason);
        }

        /* 预设段与场景画像成员一一绑定，避免新增模式时遗漏加载或错写其他场景。 */
        const std::array<std::pair<const char *, AwbPresetGain_S *>, 5> stAwbPresetSections = {
            std::make_pair("awb.preset.lock.", &stSceneMapping.stAwb.stLock),
            std::make_pair("awb.preset.incandescent.", &stSceneMapping.stAwb.stIncandescent),
            std::make_pair("awb.preset.warm.", &stSceneMapping.stAwb.stWarm),
            std::make_pair("awb.preset.fluorescent.", &stSceneMapping.stAwb.stFluorescent),
            std::make_pair("awb.preset.daylight.", &stSceneMapping.stAwb.stDaylight),
        };
        for (const auto &stPresetSection : stAwbPresetSections)
        {
            const std::string strSection = resolve_device_section(stReader,
                                                                  stPresetSection.first + stSceneNames[nIndex],
                                                                  strDeviceType);
            strValidationReason.clear();
            nRet = read_awb_preset_gain(stReader, strSection, *stPresetSection.second, strValidationReason);
            if (nRet != OK)
            {
                return report_field_error(strFilePath, strSection, stReader, nRet, strValidationReason);
            }
        }

        const std::string strSnrSection = resolve_device_section(stReader, "nr.snr." + stSceneNames[nIndex], strDeviceType);
        strValidationReason.clear();
        nRet = read_scaled_offset_mapping(stReader, strSnrSection, stSceneMapping.stNr.stSnr, strValidationReason);
        if (nRet != OK)
        {
            return report_field_error(strFilePath, strSnrSection, stReader, nRet, strValidationReason);
        }
        const std::string strTnrSection = resolve_device_section(stReader, "nr.tnr." + stSceneNames[nIndex], strDeviceType);
        strValidationReason.clear();
        nRet = read_scaled_offset_mapping(stReader, strTnrSection, stSceneMapping.stNr.stTnr, strValidationReason);
        if (nRet != OK)
        {
            return report_field_error(strFilePath, strTnrSection, stReader, nRet, strValidationReason);
        }
    }

    const std::string strExposureSection = resolve_device_section(stReader, "exposure_compensation", strDeviceType);
    for (size_t nIndex = 0; nIndex < stSceneNames.size(); ++nIndex)
    {
        strValidationReason.clear();
        nRet = read_exposure_compensation(stReader,
                                          strExposureSection,
                                          stSceneNames[nIndex],
                                          stProfile.stExposureCompensation[nIndex].data(),
                                          strValidationReason);
        if (nRet != OK)
        {
            return report_field_error(strFilePath,
                                      strExposureSection + "." + stSceneNames[nIndex],
                                      stReader,
                                      nRet,
                                      strValidationReason);
        }
    }
    return OK;
}

/**
 * @brief   : 根据Sensor字符串选择焦距专用日夜阈值段
 * @param    {const CIniReader &} stReader：运行策略解析器
 * @param    {const std::string &} strSensorType：Sensor与焦距标识
 * @return   {std::string} 焦距专用段或默认段
 */
std::string resolve_daynight_section(const CIniReader &stReader, const std::string &strSensorType)
{
    const size_t nSeparator = strSensorType.rfind('-');
    if (nSeparator != std::string::npos && nSeparator + 1 < strSensorType.size())
    {
        const std::string strSection = "daynight." + strSensorType.substr(nSeparator + 1);
        if (stReader.has_section(strSection))
        {
            return strSection;
        }
    }
    return "daynight.default";
}

/**
 * @brief   : 读取日夜判定阈值
 * @param    {const CIniReader &} stReader：运行策略解析器
 * @param    {const std::string &} strSection：焦距阈值段
 * @param    {DayNightThreshProfile_S &} stThreshold：输出阈值
 * @param    {std::string &} strValidationReason：业务校验失败的具体规则
 * @return   {int} OK：成功，非OK：字段缺失或非法
 */
int read_daynight_threshold(const CIniReader &stReader,
                            const std::string &strSection,
                            DayNightThreshProfile_S &stThreshold,
                            std::string &strValidationReason)
{
    int nWhiteRg = 0;
    int nWhiteBg = 0;
    int nRedRg = 0;
    int nRedBg = 0;
    int nRet = stReader.get_uint64(strSection, "white_to_day_brightness", stThreshold.u64WhiteLightToDayBright);
    if (nRet != OK)
    {
        return nRet;
    }
    nRet = stReader.get_uint64(strSection, "ir_to_day_brightness", stThreshold.u64RedLightToDayBright);
    if (nRet != OK)
    {
        return nRet;
    }
    nRet = stReader.get_int(strSection, "white_rg", nWhiteRg);
    if (nRet != OK)
    {
        return nRet;
    }
    nRet = stReader.get_int(strSection, "white_bg", nWhiteBg);
    if (nRet != OK)
    {
        return nRet;
    }
    nRet = stReader.get_int(strSection, "ir_rg", nRedRg);
    if (nRet != OK)
    {
        return nRet;
    }
    nRet = stReader.get_int(strSection, "ir_bg", nRedBg);
    if (nRet != OK)
    {
        return nRet;
    }
    if (stThreshold.u64WhiteLightToDayBright == 0 || stThreshold.u64RedLightToDayBright == 0 || nWhiteRg < 0 ||
        nWhiteRg > COLOR_RATIO_MAX || nWhiteBg < 0 || nWhiteBg > COLOR_RATIO_MAX || nRedRg < 0 || nRedRg > COLOR_RATIO_MAX ||
        nRedBg < 0 || nRedBg > COLOR_RATIO_MAX)
    {
        strValidationReason = "white_to_day_brightness和ir_to_day_brightness必须大于0；white_rg、white_bg、ir_rg、ir_"
                              "bg必须位于0~" +
                              std::to_string(COLOR_RATIO_MAX);
        return ERR_PARAM;
    }

    stThreshold.nWhiteRg = static_cast<unsigned int>(nWhiteRg);
    stThreshold.nWhiteBg = static_cast<unsigned int>(nWhiteBg);
    stThreshold.nRedRg = static_cast<unsigned int>(nRedRg);
    stThreshold.nRedBg = static_cast<unsigned int>(nRedBg);
    return OK;
}

/**
 * @brief   : 读取单个运行场景的DRC覆盖策略
 * @param    {const CIniReader &} stReader：运行策略解析器
 * @param    {const std::string &} strSection：DRC配置段
 * @param    {SceneDrcAdjustment_S &} stAdjustment：输出策略
 * @param    {std::string &} strValidationReason：业务校验失败的具体规则
 * @return   {int} OK：成功，非OK：字段缺失或非法
 */
int read_drc_adjustment(const CIniReader &stReader,
                        const std::string &strSection,
                        SceneDrcAdjustment_S &stAdjustment,
                        std::string &strValidationReason)
{
    int nStrength = 0;
    int nRet = stReader.get_bool(strSection, "override", stAdjustment.bOverride);
    if (nRet != OK)
    {
        return nRet;
    }
    nRet = stReader.get_bool(strSection, "enable", stAdjustment.bEnable);
    if (nRet != OK)
    {
        return nRet;
    }
    nRet = stReader.get_bool(strSection, "use_manual_strength", stAdjustment.bUseManualStrength);
    if (nRet != OK)
    {
        return nRet;
    }
    nRet = stReader.get_int(strSection, "strength", nStrength);
    if (nRet != OK)
    {
        return nRet;
    }
    if (nStrength < 0 || nStrength > DRC_STRENGTH_MAX)
    {
        strValidationReason = "strength必须位于0~" + std::to_string(DRC_STRENGTH_MAX);
        return ERR_PARAM;
    }
    stAdjustment.nStrength = static_cast<unsigned int>(nStrength);
    return OK;
}

/**
 * @brief   : 加载运行策略配置
 * @param    {const std::string &} strFilePath：配置文件路径
 * @param    {const std::string &} strSensorType：Sensor与焦距标识
 * @param    {const std::string &} strDeviceType：设备型号
 * @param    {Hi3516TuningProfile_S &} stProfile：输出画像
 * @return   {int} OK：成功，非OK：加载或校验失败
 */
int load_runtime_policy(const std::string &strFilePath,
                        const std::string &strSensorType,
                        const std::string &strDeviceType,
                        Hi3516TuningProfile_S &stProfile)
{
    CIniReader stReader;
    int nRet = stReader.load(strFilePath);
    if (nRet != OK)
    {
        return nRet;
    }
    std::string strValidationReason;
    nRet = validate_schema_version(stReader, ISP_RUNTIME_POLICY_SCHEMA_VERSION, strValidationReason);
    if (nRet != OK)
    {
        return report_field_error(strFilePath, "meta.schema_version", stReader, nRet, strValidationReason);
    }

    const std::string strDayNightSection = resolve_daynight_section(stReader, strSensorType);
    strValidationReason.clear();
    nRet = read_daynight_threshold(stReader, strDayNightSection, stProfile.stDayNightThresh, strValidationReason);
    if (nRet != OK)
    {
        return report_field_error(strFilePath, strDayNightSection, stReader, nRet, strValidationReason);
    }

    const std::string strSceneSection = resolve_device_section(stReader, "scene", strDeviceType);
    nRet = stReader.get_int(strSceneSection, "day_index", stProfile.nDaySceneIndex);
    if (nRet != OK)
    {
        return report_field_error(strFilePath, strSceneSection + ".day_index", stReader, nRet);
    }
    nRet = stReader.get_int(strSceneSection, "night_white_index", stProfile.nNightWhiteSceneIndex);
    if (nRet != OK)
    {
        return report_field_error(strFilePath, strSceneSection + ".night_white_index", stReader, nRet);
    }
    nRet = stReader.get_int(strSceneSection, "night_ir_index", stProfile.nNightIrSceneIndex);
    if (nRet != OK)
    {
        return report_field_error(strFilePath, strSceneSection + ".night_ir_index", stReader, nRet);
    }
    if (stProfile.nDaySceneIndex < SCENE_INDEX_MIN || stProfile.nDaySceneIndex > SCENE_INDEX_MAX ||
        stProfile.nNightWhiteSceneIndex < SCENE_INDEX_MIN || stProfile.nNightWhiteSceneIndex > SCENE_INDEX_MAX ||
        stProfile.nNightIrSceneIndex < SCENE_INDEX_MIN || stProfile.nNightIrSceneIndex > SCENE_INDEX_MAX)
    {
        return report_field_error(strFilePath,
                                  strSceneSection,
                                  stReader,
                                  ERR_PARAM,
                                  "day_index、night_white_index和night_ir_index必须位于" + std::to_string(SCENE_INDEX_MIN) + "~" +
                                      std::to_string(SCENE_INDEX_MAX));
    }

    const std::string strDayDrcSection = resolve_device_section(stReader, "scene.drc.day", strDeviceType);
    strValidationReason.clear();
    nRet = read_drc_adjustment(stReader, strDayDrcSection, stProfile.stSceneDrc.stDay, strValidationReason);
    if (nRet != OK)
    {
        return report_field_error(strFilePath, strDayDrcSection, stReader, nRet, strValidationReason);
    }
    const std::string strWhiteDrcSection = resolve_device_section(stReader, "scene.drc.night_white", strDeviceType);
    strValidationReason.clear();
    nRet = read_drc_adjustment(stReader, strWhiteDrcSection, stProfile.stSceneDrc.stNightWhite, strValidationReason);
    if (nRet != OK)
    {
        return report_field_error(strFilePath, strWhiteDrcSection, stReader, nRet, strValidationReason);
    }
    const std::string strIrDrcSection = resolve_device_section(stReader, "scene.drc.night_ir", strDeviceType);
    strValidationReason.clear();
    nRet = read_drc_adjustment(stReader, strIrDrcSection, stProfile.stSceneDrc.stNightIr, strValidationReason);
    if (nRet != OK)
    {
        return report_field_error(strFilePath, strIrDrcSection, stReader, nRet, strValidationReason);
    }
    const std::string strGammaSection = resolve_device_section(stReader, "gamma", strDeviceType);
    nRet = stReader.get_bool(strGammaSection, "use_sensor_cmos", stProfile.bUseCmosGamma);
    if (nRet != OK)
    {
        return report_field_error(strFilePath, strGammaSection + ".use_sensor_cmos", stReader, nRet);
    }
    const std::string strMountSection = resolve_device_section(stReader, "mount", strDeviceType);
    nRet = stReader.get_bool(strMountSection, "ceiling", stProfile.stMirror.bCeilingMount);
    if (nRet != OK)
    {
        return report_field_error(strFilePath, strMountSection + ".ceiling", stReader, nRet);
    }
    return OK;
}
} // namespace

namespace IspProfileConfigLoader_NS
{
int load(const std::string &strConfigDir,
         const std::string &strSensorType,
         const std::string &strDeviceType,
         Hi3516TuningProfile_S &stProfile)
{
    if (strConfigDir.empty() || strSensorType.empty() || strDeviceType.empty())
    {
        dlog_error("ISP配置加载参数为空, 目录:%s, Sensor:%s, 设备:%s",
                   strConfigDir.c_str(),
                   strSensorType.c_str(),
                   strDeviceType.c_str());
        return ERR_PARAM;
    }

    /* 使用临时快照保证任一文件失败时不向调用方暴露半配置状态。 */
    Hi3516TuningProfile_S stLoadedProfile;
    const std::string strMappingPath = join_path(strConfigDir, ISP_PARAM_MAPPING_FILE_NAME);
    int nRet = load_param_mapping(strMappingPath, strDeviceType, stLoadedProfile);
    if (nRet != OK)
    {
        return nRet;
    }

    const std::string strPolicyPath = join_path(strConfigDir, ISP_RUNTIME_POLICY_FILE_NAME);
    nRet = load_runtime_policy(strPolicyPath, strSensorType, strDeviceType, stLoadedProfile);
    if (nRet != OK)
    {
        return nRet;
    }

    stProfile = stLoadedProfile;
    dlog_info("ISP参数映射与运行策略加载完成, 目录:%s, Sensor:%s, 设备:%s",
              strConfigDir.c_str(),
              strSensorType.c_str(),
              strDeviceType.c_str());
    return OK;
}
} // namespace IspProfileConfigLoader_NS
