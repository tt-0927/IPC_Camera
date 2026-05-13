/**
 * @FilePath     : nal_parser.cpp
 * @Author       : Claude Code
 * @Date         : 2026-01-07
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-01-08 10:21:46
 * @Description  : NAL单元类型解析器实现
 */

#include "nal_parser.h"
#include "dlog.h"

CH264NalParser::CH264NalParser()
{
    /* 初始化H.264 NAL类型映射表 */
    m_typeMap = {
        {  1,    Video_NS::NalType_E::H264_TYPE_SLICE },
        {  2,      Video_NS::NalType_E::H264_TYPE_DPA },
        {  3,      Video_NS::NalType_E::H264_TYPE_DPB },
        {  4,      Video_NS::NalType_E::H264_TYPE_DPC },
        {  5,      Video_NS::NalType_E::H264_TYPE_IDR },
        {  6,      Video_NS::NalType_E::H264_TYPE_SEI },
        {  7,      Video_NS::NalType_E::H264_TYPE_SPS },
        {  8,      Video_NS::NalType_E::H264_TYPE_PPS },
        {  9,      Video_NS::NalType_E::H264_TYPE_AUD },
        { 10,    Video_NS::NalType_E::H264_TYPE_EOSEQ },
        { 11, Video_NS::NalType_E::H264_TYPE_EOSTREAM },
        { 12,   Video_NS::NalType_E::H264_TYPE_FILLER }
    };
}

Video_NS::NalType_E CH264NalParser::parseNalType(const uint8_t* pData, int nDataLen)
{
    if (!pData || nDataLen < 5)
    {
        dlog_error("H264解析器：数据长度不足");
        return Video_NS::NalType_E::H264_TYPE_SLICE;
    }

    /* 提取NAL类型（第5字节的低5位） */
    uint8_t nalType = pData[4] & 0x1F;

    /* 查找映射表 */
    auto it = m_typeMap.find(nalType);
    if (it != m_typeMap.end())
    {
        return it->second;
    }

    /* 默认返回SLICE类型 */
    return Video_NS::NalType_E::H264_TYPE_SLICE;
}

CH265NalParser::CH265NalParser()
{
    /* 初始化H.265 NAL类型映射表 */
    m_typeMap = {
        {  0,    Video_NS::NalType_E::H265_TYPE_TRAIL_N },
        {  1,    Video_NS::NalType_E::H265_TYPE_TRAIL_R },
        {  8,     Video_NS::NalType_E::H265_TYPE_RASL_N },
        {  9,     Video_NS::NalType_E::H265_TYPE_RASL_R },
        { 10,     Video_NS::NalType_E::H265_TYPE_RADL_N },
        { 11,     Video_NS::NalType_E::H265_TYPE_RADL_R },
        { 19, Video_NS::NalType_E::H265_TYPE_IDR_W_RADL },
        { 20,   Video_NS::NalType_E::H265_TYPE_IDR_N_LP },
        { 21,        Video_NS::NalType_E::H265_TYPE_CRA },
        { 32,        Video_NS::NalType_E::H265_TYPE_VPS },
        { 33,        Video_NS::NalType_E::H265_TYPE_SPS },
        { 34,        Video_NS::NalType_E::H265_TYPE_PPS },
        { 35,        Video_NS::NalType_E::H265_TYPE_AUD },
        { 36,        Video_NS::NalType_E::H265_TYPE_EOS },
        { 37,        Video_NS::NalType_E::H265_TYPE_EOB },
        { 38,     Video_NS::NalType_E::H265_TYPE_FILLER },
        { 39,        Video_NS::NalType_E::H265_TYPE_SEI },
        { 40, Video_NS::NalType_E::H265_TYPE_SEI_SUFFIX }
    };
}

Video_NS::NalType_E CH265NalParser::parseNalType(const uint8_t* pData, int nDataLen)
{
    if (!pData || nDataLen < 5)
    {
        dlog_error("H265解析器：数据长度不足");
        return Video_NS::NalType_E::H265_TYPE_TRAIL_N;
    }

    /* 提取NAL类型（第5字节的第1-6位） */
    uint8_t nalType = (pData[4] >> 1) & 0x3F;

    /* 查找映射表 */
    auto it = m_typeMap.find(nalType);
    if (it != m_typeMap.end())
    {
        return it->second;
    }

    /* 默认返回TRAIL_N类型 */
    return Video_NS::NalType_E::H265_TYPE_TRAIL_N;
}

CSvac3NalParser::CSvac3NalParser()
{
    /* 初始化SVAC3 NAL类型映射表 */
    m_typeMap = {
        {  0,             Video_NS::NalType_E::SVAC3_TYPE_RESERVED_0 },
        {  1,          Video_NS::NalType_E::SVAC3_TYPE_NON_IDR_SLICE },
        {  2,              Video_NS::NalType_E::SVAC3_TYPE_IDR_SLICE },
        {  3,      Video_NS::NalType_E::SVAC3_TYPE_NON_IDR_SVC_SLICE },
        {  4,          Video_NS::NalType_E::SVAC3_TYPE_IDR_SVC_SLICE },
        {  5, Video_NS::NalType_E::SVAC3_TYPE_SURVEILLANCE_EXTENSION },
        {  6,      Video_NS::NalType_E::SVAC3_TYPE_SUPPLEMENTAL_INFO },
        {  7,     Video_NS::NalType_E::SVAC3_TYPE_SEQUENCE_PARAM_SET },
        {  8,      Video_NS::NalType_E::SVAC3_TYPE_PICTURE_PARAM_SET },
        {  9,     Video_NS::NalType_E::SVAC3_TYPE_SECURITY_PARAM_SET },
        { 10,    Video_NS::NalType_E::SVAC3_TYPE_AUTHENTICATION_DATA },
        { 11,          Video_NS::NalType_E::SVAC3_TYPE_END_OF_STREAM },
        { 12,            Video_NS::NalType_E::SVAC3_TYPE_RESERVED_12 },
        { 13,            Video_NS::NalType_E::SVAC3_TYPE_RESERVED_13 },
        { 14,            Video_NS::NalType_E::SVAC3_TYPE_RESERVED_14 },
        { 15,      Video_NS::NalType_E::SVAC3_TYPE_SVC_PIC_PARAM_SET }
    };
}

Video_NS::NalType_E CSvac3NalParser::parseNalType(const uint8_t* pData, int nDataLen)
{
    if (!pData || nDataLen < 6)
    {
        dlog_error("SVAC3解析器：数据长度不足");
        return Video_NS::NalType_E::SVAC3_TYPE_NON_IDR_SLICE;
    }

    /* 提取NAL类型（第6字节） */
    uint8_t nalType = pData[5];

    /* 查找映射表 */
    auto it = m_typeMap.find(nalType);
    if (it != m_typeMap.end())
    {
        return it->second;
    }

    /* 默认返回NON_IDR_SLICE类型 */
    return Video_NS::NalType_E::SVAC3_TYPE_NON_IDR_SLICE;
}

Video_NS::NalType_E CMjpegParser::parseNalType(const uint8_t* pData, int nDataLen)
{
    /* MJPEG不需要解析NAL类型 */
    return Video_NS::NalType_E::UNKNOWN_TYPE;
}
