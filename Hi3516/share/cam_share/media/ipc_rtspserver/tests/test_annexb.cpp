/**
 * @FilePath     : test_annexb.cpp
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2026-09-10 18:49:34
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-09-16 09:25:00
 * @Description  : Annex-B 扫描与 NAL 分类测试
 */

#include "media/annexb_scanner.h"

#include "test_support.h"

#include <cstring>
#include <vector>

using namespace ipc_rtsp;
using namespace ipc_rtsp::detail;

namespace
{

/** 收集 NAL 类型序列。
 *
 * @param codec 码流编码（决定 NAL 头的解析宽度）。
 * @param data Annex-B 码流起始地址。
 * @param size 码流字节数。
 * @return 按流内顺序排列的 NAL 类型列表。
 */
std::vector<NalUnitType> CollectTypes(Codec codec, const std::uint8_t *data, std::size_t size)
{
    std::vector<NalUnitType> types;
    ForEachNal(codec,
               data,
               size,
               [&](const NalRange &nal)
               {
                   types.push_back(nal.type);
                   return true;
               });
    return types;
}

} // namespace

RTSP_TEST_CASE(annexb_three_byte_start_code)
{
    /* 3 字节起始码（00 00 01）也要正确切分：预期只产出 SPS/PPS 两个 NAL。 */
    const std::uint8_t data[] = { 0x00, 0x00, 0x01, 0x67, 0xAA, 0xBB, 0x00, 0x00, 0x01, 0x68, 0xCC };
    const std::vector<NalUnitType> types = CollectTypes(Codec::H264, data, sizeof data);
    RTSP_CHECK_EQ(types.size(), static_cast<std::size_t>(2));
    RTSP_CHECK(types[0] == NalUnitType::Sps);
    RTSP_CHECK(types[1] == NalUnitType::Pps);
}

RTSP_TEST_CASE(annexb_four_byte_start_code_and_trailing_zeros)
{
    /* 4 字节起始码 + 尾部填充零：不得把填充零算进 NAL。 */
    const std::uint8_t data[] = { 0x00, 0x00, 0x00, 0x01, 0x65, 0x11, 0x22, 0x00, 0x00, 0x00, 0x00, 0x01, 0x41, 0x33 };
    std::vector<std::size_t> sizes;
    ForEachNal(Codec::H264,
               data,
               sizeof data,
               [&](const NalRange &nal)
               {
                   sizes.push_back(nal.size);
                   return true;
               });
    RTSP_CHECK_EQ(sizes.size(), static_cast<std::size_t>(2));
    RTSP_CHECK_EQ(sizes[0], static_cast<std::size_t>(3)); /* 0x65 0x11 0x22 */
    RTSP_CHECK_EQ(sizes[1], static_cast<std::size_t>(2)); /* 0x41 0x33 */
}

RTSP_TEST_CASE(annexb_composite_pack_key_detection)
{
    /* 单包模式：SPS/PPS/IDR 复合在一个 buffer 中。 */
    const std::uint8_t data[] = {
        0x00, 0x00, 0x00, 0x01, 0x67, 0x10, 0x00, 0x00, 0x00, 0x01, 0x68, 0x20, 0x00, 0x00, 0x00, 0x01, 0x65, 0x30, 0x40,
    };
    const std::vector<NalUnitType> types = CollectTypes(Codec::H264, data, sizeof data);
    RTSP_CHECK_EQ(types.size(), static_cast<std::size_t>(3));
    RTSP_CHECK(types[0] == NalUnitType::Sps);
    RTSP_CHECK(types[1] == NalUnitType::Pps);
    RTSP_CHECK(IsKeyNal(types[2]));
    RTSP_CHECK(IsVclNal(types[2]));
}

RTSP_TEST_CASE(annexb_h265_types)
{
    /* H.265 NAL 头 2 字节：VPS(32) / SPS(33) / PPS(34) / IDR_W_RADL(19) / CRA(21)。 */
    const std::uint8_t data[] = {
        0x00, 0x00, 0x00, 0x01, 0x40, 0x01, 0x0A, /* type 32 */
        0x00, 0x00, 0x00, 0x01, 0x42, 0x01, 0x0B, /* type 33 */
        0x00, 0x00, 0x00, 0x01, 0x44, 0x01, 0x0C, /* type 34 */
        0x00, 0x00, 0x00, 0x01, 0x26, 0x01, 0x0D, /* type 19 */
        0x00, 0x00, 0x00, 0x01, 0x2A, 0x01, 0x0E, /* type 21 */
    };
    const std::vector<NalUnitType> types = CollectTypes(Codec::H265, data, sizeof data);
    RTSP_CHECK_EQ(types.size(), static_cast<std::size_t>(5));
    RTSP_CHECK(types[0] == NalUnitType::Vps);
    RTSP_CHECK(types[1] == NalUnitType::Sps);
    RTSP_CHECK(types[2] == NalUnitType::Pps);
    RTSP_CHECK(types[3] == NalUnitType::IdrSlice);
    RTSP_CHECK(types[4] == NalUnitType::CraSlice);
    RTSP_CHECK(IsKeyNal(types[3]));
    RTSP_CHECK(IsKeyNal(types[4]));
}

RTSP_TEST_CASE(annexb_first_slice_detection_h264)
{
    /* H.264 slice header 首 ue(v)：0x80 起手表示 first_mb_in_slice=0（新图像），
     * 0x00 起手表示非首片（同一图像的后继 slice）。 */
    const std::uint8_t first_slice[] = { 0x65, 0x88, 0x84, 0x00 }; /* IDR，首片 */
    const std::uint8_t other_slice[] = { 0x65, 0x00, 0x11, 0x22 }; /* IDR，非首片 */
    const std::uint8_t non_vcl[] = { 0x67, 0x42, 0xE0, 0x1E };     /* SPS，非 VCL */

    RTSP_CHECK(IsFirstSliceOfPicture(Codec::H264, first_slice, sizeof first_slice));
    RTSP_CHECK(!IsFirstSliceOfPicture(Codec::H264, other_slice, sizeof other_slice));
    RTSP_CHECK(!IsFirstSliceOfPicture(Codec::H264, non_vcl, sizeof non_vcl));
}

RTSP_TEST_CASE(annexb_first_slice_detection_h265)
{
    /* H.265 slice segment header 首比特 first_slice_segment_in_pic_flag。 */
    const std::uint8_t first_slice[] = { 0x26, 0x01, 0x80, 0x00 }; /* IDR_W_RADL，首片 */
    const std::uint8_t other_slice[] = { 0x26, 0x01, 0x00, 0x11 }; /* 非首片 */
    const std::uint8_t vps[] = { 0x40, 0x01, 0x0C };               /* VPS，非 VCL */

    RTSP_CHECK(IsFirstSliceOfPicture(Codec::H265, first_slice, sizeof first_slice));
    RTSP_CHECK(!IsFirstSliceOfPicture(Codec::H265, other_slice, sizeof other_slice));
    RTSP_CHECK(!IsFirstSliceOfPicture(Codec::H265, vps, sizeof vps));
}

RTSP_TEST_CASE(annexb_no_start_code_returns_zero)
{
    /* 输入不含任何起始码：预期扫描不到 NAL，返回空序列。 */
    const std::uint8_t data[] = { 0x65, 0x11, 0x22, 0x33 };
    RTSP_CHECK_EQ(CollectTypes(Codec::H264, data, sizeof data).size(), static_cast<std::size_t>(0));
}

RTSP_TEST_CASE(annexb_nested_start_code_inside_payload)
{
    /* payload 内出现 00 00 01 时按新 NAL 处理（Annex-B 语义），不得丢失字节。 */
    const std::uint8_t data[] = { 0x00, 0x00, 0x01, 0x41, 0x00, 0x00, 0x01, 0x42, 0x00, 0x00, 0x01, 0x43 };
    const std::vector<NalUnitType> types = CollectTypes(Codec::H264, data, sizeof data);
    RTSP_CHECK_EQ(types.size(), static_cast<std::size_t>(3));
    RTSP_CHECK(types[0] == NalUnitType::NonIdrSlice);
    RTSP_CHECK(types[1] == NalUnitType::NonIdrSlice);
    RTSP_CHECK(types[2] == NalUnitType::NonIdrSlice);
}
