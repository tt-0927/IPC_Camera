/**
 * @FilePath     : video_extradata_builder.cpp
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2026-05-13 08:55:30
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-05-13 09:38:37
 * @Description  : RTMP视频extradata构造工具实现 
 */

#include "video_extradata_builder.h"

#include "dlog.h"

namespace
{
    /**
     * @brief 写入16位大端整数
     * @param vData 目标缓存
     * @param nValue 整数值
     */
    void push_be16(std::vector<uint8_t>& vData, int nValue)
    {
        vData.push_back(static_cast<uint8_t>((nValue >> 8) & 0xff));
        vData.push_back(static_cast<uint8_t>(nValue & 0xff));
    }

    /**
     * @brief 写入一个HEVC参数集数组
     * @param vData 目标缓存
     * @param nNalType NAL类型
     * @param vNal NAL数据
     */
    void append_hevc_array(std::vector<uint8_t>& vData, int nNalType, const std::vector<uint8_t>& vNal)
    {
        vData.push_back(static_cast<uint8_t>(0x80 | (nNalType & 0x3f)));
        push_be16(vData, 1);
        push_be16(vData, static_cast<int>(vNal.size()));
        vData.insert(vData.end(), vNal.begin(), vNal.end());
    }
}

namespace RtmpVideo_NS
{
    bool build_avc_extradata(const VideoParameterSets_S& stSets, std::vector<uint8_t>& vExtradata)
    {
        if (stSets.sps.size() < 4 || stSets.pps.empty())
        {
            return false;
        }

        vExtradata.clear();
        vExtradata.push_back(0x01);
        vExtradata.push_back(stSets.sps[1]);
        vExtradata.push_back(stSets.sps[2]);
        vExtradata.push_back(stSets.sps[3]);
        vExtradata.push_back(0xff);
        vExtradata.push_back(0xe1);
        push_be16(vExtradata, static_cast<int>(stSets.sps.size()));
        vExtradata.insert(vExtradata.end(), stSets.sps.begin(), stSets.sps.end());
        vExtradata.push_back(0x01);
        push_be16(vExtradata, static_cast<int>(stSets.pps.size()));
        vExtradata.insert(vExtradata.end(), stSets.pps.begin(), stSets.pps.end());
        return true;
    }

    bool build_hevc_extradata(const VideoParameterSets_S& stSets, std::vector<uint8_t>& vExtradata)
    {
        /* SPS 必须包含 NAL 头(2B) + sps_video_parameter_set_id 等(1B) + profile_tier_level。
         * 当 sps_max_sub_layers_minus1 == 0 时，profile_tier_level 占 12 字节，
         * general_level_idc 位于 SPS 字节索引 14。
         */
        if (stSets.vps.empty() || stSets.sps.size() < 15 || stSets.pps.empty())
        {
            dlog_warn("H.265参数集不完整：VPS=%zu, SPS=%zu, PPS=%zu",
                      stSets.vps.size(), stSets.sps.size(), stSets.pps.size());
            return false;
        }

        const uint8_t nSubLayersMinus1 = (stSets.sps[2] >> 1) & 0x07;
        if (nSubLayersMinus1 != 0)
        {
            dlog_warn("H.265 SPS sub_layers_minus1=%d 非零，level_idc 偏移可能不准", nSubLayersMinus1);
        }

        vExtradata.clear();

        /* configurationVersion = 1 */
        vExtradata.push_back(0x01);

        /* profile_tier_level 相关字段从 SPS 中复制，避免依赖 FFmpeg 内部未导出的 HEVC 工具函数。 */
        vExtradata.push_back(stSets.sps[3]);
        vExtradata.push_back(stSets.sps[4]);
        vExtradata.push_back(stSets.sps[5]);
        vExtradata.push_back(stSets.sps[6]);
        vExtradata.push_back(stSets.sps[7]);
        vExtradata.push_back(stSets.sps[8]);
        vExtradata.push_back(stSets.sps[9]);
        vExtradata.push_back(stSets.sps[10]);
        vExtradata.push_back(stSets.sps[11]);
        vExtradata.push_back(stSets.sps[12]);
        vExtradata.push_back(stSets.sps[13]);
        vExtradata.push_back(stSets.sps[14]);

        /* 以下字段使用 hvcC 规范 reserved 位默认值，lengthSizeMinusOne 固定为 3 表示 4 字节长度前缀。 */
        vExtradata.push_back(0xf0);
        vExtradata.push_back(0x00);
        vExtradata.push_back(0xfc);
        vExtradata.push_back(0xfd);
        vExtradata.push_back(0xf8);
        vExtradata.push_back(0xf8);
        vExtradata.push_back(0x00);
        vExtradata.push_back(0x00);
        vExtradata.push_back(0x0f);
        vExtradata.push_back(0x03);

        append_hevc_array(vExtradata, 32, stSets.vps);
        append_hevc_array(vExtradata, 33, stSets.sps);
        append_hevc_array(vExtradata, 34, stSets.pps);

        dlog_info("HEVC extradata构造完成：size=%zu, profile_idc=0x%02x, level_idc=0x%02x, sub_layers=%d",
                  vExtradata.size(), stSets.sps[3] & 0x1f, stSets.sps[14], nSubLayersMinus1);
        return true;
    }
}
