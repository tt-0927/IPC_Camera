/*
 * @Author       : EasonLu
 * @Date         : 2025-03-11 08:40:25
 * @LastEditors  : EasonLu
 * @LastEditTime : 2025-05-09 10:42:52
 * @FilePath     : SipUtils.h
 * @Description  : SIP工具函数
 */
#pragma once
#include "MediaPs.h"
#include "MediaRtp.h"
#include "MediaSdp.h"
#include "SipType.h"
#include <ctime>
#include <string>
#include <vector>
namespace SIP
{
    std::string LocalTime(time_t time);

    std::string GenerateRandomString(int n);

    std::string GenerateRandomNumber(int n);

    std::string SSRC_Hex(std::string ssrc);

    std::string ToUtf8String(const std::string &input);

    std::string ToMbcsString(const std::string &input);

    std::string GetCurrentModuleDirectory();

    int64_t ISO8601ToTimeT(const std::string &gz);

    std::string TimeTToISO8601(int64_t time);

    std::string CurrentTimeTToISO8601();

    int64_t UnixToTime(const std::string &strFromatTime);

    SDP::Map_S ToRtpMap(SipVideoType_E enType);

    SipVideoType_E FromRtpMapByVideo(const SDP::Map_S &stMap);

    SDP::Map_S ToRtpMap(SipAudioInfo_S stInfo);

    SipAudioInfo_S FromRtpMapByAudio(const SDP::Map_S &stMap);

    SipVideoType_E FromPsmStreamIDByVideo(int nStreamID);

    SipAudioType_E FromPsmStreamIDByAudio(int nStreamID);

    int ToPsmStreamIDByVideo(SipVideoType_E enType);

    int ToPsmStreamIDByAudio(SipAudioType_E enType);

    uint32_t CalcCRC_32(const char *data, size_t length);

    bool isNumber(const std::string &str);

    std::vector<std::string> SplitString(const std::string &s, char c);

    int ParseReadFileAction(const std::string &str, SipReadFileAction_S &stReadAction);

    /*
     * 元函数：判断 T 是否为整型或浮点型
     * 优点：
     *   - 编译期计算结果
     *   - 可用于 static_assert 和 enable_if
     */
    template <typename T>
    struct is_valid_number_type
    {
        static constexpr bool value =
            std::is_integral<T>::value || std::is_floating_point<T>::value;
    };

    /*
     * 主模板：仅允许合法的数字类型（整型或浮点型）
     * 使用 SFINAE：
     *   - 合法类型：生成此函数
     *   - 非法类型：跳过此函数
     */
    template <typename NumberType>
    typename std::enable_if<is_valid_number_type<NumberType>::value, bool>::type
    SafeStr2Num(const std::string &str, NumberType &result)
    {
        static_assert(
            is_valid_number_type<NumberType>::value,
            "字符串转换函数的输出参数必须为整型或浮点型");
        /* 转换使用的临时变量 */
        char *endptr = nullptr;
        errno = 0; // 确保 errno 初始状态
        /* 按类型分发：整型 or 浮点型 */
        if (std::is_integral<NumberType>::value)
        {
            /* ---------- 整型转换 ---------- */
            if (std::is_unsigned<NumberType>::value)
            {
                /* 无符号整型（unsigned int, uint64_t, etc.） */
                unsigned long long val = std::strtoull(str.c_str(), &endptr, 10);
                /* 检查转换是否失败：
                 *   - endptr == str.c_str()：无数字可转
                 *   - *endptr != '\0'：字符串含非数字字符
                 *   - errno == ERANGE：数值溢出
                 */
                if (endptr == str.c_str() || *endptr != '\0' || errno == ERANGE)
                {
                    return false;
                }
                /* 检查无符号范围：
                 *   - val > max：超出目标类型上限
                 *   - str[0] == '-'：负数字符串转无符号数非法
                 */
                if (val > std::numeric_limits<NumberType>::max() || (!str.empty() && str[0] == '-'))
                {
                    return false;
                }
                /* 转换成功，赋值结果 */
                result = static_cast<NumberType>(val);
            }
            else
            {
                /* 有符号整型（int, long, int64_t, etc.） */
                long long val = std::strtoll(str.c_str(), &endptr, 10);
                /* 检查转换是否失败 */
                if (endptr == str.c_str() || *endptr != '\0' || errno == ERANGE)
                {
                    return false;
                }
                /* 检查有符号范围 */
                if (val < std::numeric_limits<NumberType>::min() ||
                    val > std::numeric_limits<NumberType>::max())
                {
                    return false;
                }
                /* 转换成功，赋值结果 */
                result = static_cast<NumberType>(val);
            }
        }
        else
        {
            /* ---------- 浮点型转换 ---------- */
            if (std::is_same<NumberType, float>::value)
            {
                result = std::strtof(str.c_str(), &endptr);
            }
            else if (std::is_same<NumberType, double>::value)
            {
                result = std::strtod(str.c_str(), &endptr);
            }
            else if (std::is_same<NumberType, long double>::value)
            {
                result = std::strtold(str.c_str(), &endptr);
            }
            /* 检查浮点转换是否失败 */
            if (endptr == str.c_str() || *endptr != '\0' || errno == ERANGE)
            {
                return false;
            }
        }
        /* 转换成功 */
        return true;
    }

    // 非法类型版本（明确 delete）
    template <typename NumberType>
    typename std::enable_if<!is_valid_number_type<NumberType>::value, bool>::type
    SafeStr2Num(const std::string &str, NumberType &result) = delete;
}