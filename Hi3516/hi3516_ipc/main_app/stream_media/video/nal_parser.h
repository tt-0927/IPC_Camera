/**
 * @FilePath     : nal_parser.h
 * @Author       : Claude Code
 * @Date         : 2026-01-07
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-01-08 10:21:42
 * @Description  : NAL单元类型解析器策略接口及实现类
 */

#pragma once

#include <cstdint>
#include <unordered_map>
#include "video_define.h"

/**
 * @brief   : NAL单元类型解析器接口
 * @note    : 使用策略模式支持不同编码格式的NAL类型解析
 */
class CINalParser
{
public:
    virtual ~CINalParser() = default;

    /**
     * @brief   : 解析NAL单元类型
     * @param   {uint8_t*} pData：帧数据指针
     * @param   {int} nDataLen：帧数据长度
     * @return  {Video_NS::NalType_E} NAL单元类型
     */
    virtual Video_NS::NalType_E parseNalType(const uint8_t* pData, int nDataLen) = 0;
};

/**
 * @brief   : H.264 NAL类型解析器
 * @note    : 解析H.264编码的NAL单元类型
 */
class CH264NalParser : public CINalParser
{
public:
    /**
     * @brief   : 构造函数，初始化NAL类型映射表
     */
    CH264NalParser();

    /**
     * @brief   : 解析H.264 NAL单元类型
     * @param   {uint8_t*} pData：帧数据指针
     * @param   {int} nDataLen：帧数据长度
     * @return  {Video_NS::NalType_E} NAL单元类型
     */
    Video_NS::NalType_E parseNalType(const uint8_t* pData, int nDataLen) override;

private:
    /* NAL类型映射表 */
    std::unordered_map<uint8_t, Video_NS::NalType_E> m_typeMap;
};

/**
 * @brief   : H.265 NAL类型解析器
 * @note    : 解析H.265编码的NAL单元类型
 */
class CH265NalParser : public CINalParser
{
public:
    /**
     * @brief   : 构造函数，初始化NAL类型映射表
     */
    CH265NalParser();

    /**
     * @brief   : 解析H.265 NAL单元类型
     * @param   {uint8_t*} pData：帧数据指针
     * @param   {int} nDataLen：帧数据长度
     * @return  {Video_NS::NalType_E} NAL单元类型
     */
    Video_NS::NalType_E parseNalType(const uint8_t* pData, int nDataLen) override;

private:
    /* NAL类型映射表 */
    std::unordered_map<uint8_t, Video_NS::NalType_E> m_typeMap;
};

/**
 * @brief   : SVAC3 NAL类型解析器
 * @note    : 解析SVAC3编码的NAL单元类型
 */
class CSvac3NalParser : public CINalParser
{
public:
    /**
     * @brief   : 构造函数，初始化NAL类型映射表
     */
    CSvac3NalParser();

    /**
     * @brief   : 解析SVAC3 NAL单元类型
     * @param   {uint8_t*} pData：帧数据指针
     * @param   {int} nDataLen：帧数据长度
     * @return  {Video_NS::NalType_E} NAL单元类型
     */
    Video_NS::NalType_E parseNalType(const uint8_t* pData, int nDataLen) override;

private:
    /* NAL类型映射表 */
    std::unordered_map<uint8_t, Video_NS::NalType_E> m_typeMap;
};

/**
 * @brief   : MJPEG解析器
 * @note    : MJPEG不需要解析NAL类型
 */
class CMjpegParser : public CINalParser
{
public:
    /**
     * @brief   : 解析MJPEG类型（始终返回UNKNOWN_TYPE）
     * @param   {uint8_t*} pData：帧数据指针
     * @param   {int} nDataLen：帧数据长度
     * @return  {Video_NS::NalType_E} 返回UNKNOWN_TYPE
     */
    Video_NS::NalType_E parseNalType(const uint8_t* pData, int nDataLen) override;
};
