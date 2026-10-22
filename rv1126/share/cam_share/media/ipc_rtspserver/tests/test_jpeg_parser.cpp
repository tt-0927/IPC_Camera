/**
 * @FilePath     : test_jpeg_parser.cpp
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2026-09-14 15:21:35
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-09-16 09:25:00
 * @Description  : JPEG 帧解析（RFC 2435 打包前置）测试
 */

/*
 * 用合成 baseline JPEG 帧覆盖解析分支；真实编码器产出的帧由
 * host_verify.sh 的 Motion-JPEG 端到端用例（ffmpeg 解码）覆盖。
 * 接受面以旧 live555 路径（mediaServer/mjpeg_video_source）为基线：
 * 采样判不出 type 回退默认 1、不检查分量数与色度采样、超限宽高块数置 0。
 */

#include "media/jpeg_parser.h"

#include "test_support.h"

#include <algorithm>
#include <cstdint>
#include <cstring>
#include <iterator>
#include <vector>

using namespace ipc_rtsp;
using namespace ipc_rtsp::detail;

namespace
{

/** 测试帧构造参数。 */
struct FrameSpec
{
    /* 帧宽（像素）。 */
    unsigned width = 640;
    /* 帧高（像素）。 */
    unsigned height = 360;
    /* 亮度采样因子 hv：0x22=4:2:0、0x21=4:2:2、0x11=4:4:4/灰度。 */
    std::uint8_t luma_sampling = 0x22;
    /* 色度采样因子 hv（协议要求 1x1，解析不再强制）。 */
    std::uint8_t chroma_sampling = 0x11;
    /* SOF0 分量数。 */
    unsigned ncomp = 3;
    /* Y 分量 id 基值（海思为 0，id 不参与判定）。 */
    std::uint8_t component_id_base = 1;
    /* 非 0 时覆盖 SOF0 段长（模拟段长异常帧）。 */
    int sof_seg_len = 0;
    /* 是否携带 DQT 段。 */
    bool with_dqt = true;
    /* 是否携带第二张量化表。 */
    bool with_dqt1 = true;
    /* 两张表分两个 DQT 段。 */
    bool split_dqt = false;
    /* 亮度表用 16bit 精度（Pq=1，应被拒绝）。 */
    bool dqt_pq1 = false;
    /* 是否携带 DRI 段。 */
    bool with_dri = false;
    /* DRI 段的重启间隔值。 */
    std::uint16_t restart_interval = 0;
    /* 是否携带 SOF0 段。 */
    bool with_sof0 = true;
    /* 是否携带 EOI。 */
    bool with_eoi = true;
    /* 熵编码 scan 数据（含 0xFF 转义字节）。 */
    std::vector<std::uint8_t> scan = { 0x12, 0x34, 0xFF, 0x00, 0x56 };
};

/** 追加一个大端 16 位整数。
 *
 * @param out 目标缓冲。
 * @param value 待写入的值。
 */
void AppendBe16(std::vector<std::uint8_t> *out, std::uint16_t value)
{
    out->push_back(static_cast<std::uint8_t>(value >> 8));
    out->push_back(static_cast<std::uint8_t>(value & 0xFF));
}

/** 追加一个 8bit 精度（Pq=0）、64 项的 DQT 段。
 *
 * @param out 目标缓冲。
 * @param table_id 表号 Tq。
 * @param fill 表项填充值。
 */
void AppendDqt(std::vector<std::uint8_t> *out, std::uint8_t table_id, std::uint8_t fill)
{
    out->push_back(0xFF);
    out->push_back(0xDB);
    AppendBe16(out, 2 + 65);
    out->push_back(table_id); /* Pq=0（8bit）、Tq=table_id */
    for (int i = 0; i < 64; ++i)
    {
        out->push_back(fill);
    }
}

/** 追加一个 16bit 精度（Pq=1）、128 项的 DQT 段（解析器应拒绝）。
 *
 * @param out 目标缓冲。
 * @param table_id 表号 Tq。
 * @param fill 表项填充值。
 */
void AppendDqt16(std::vector<std::uint8_t> *out, std::uint8_t table_id, std::uint8_t fill)
{
    out->push_back(0xFF);
    out->push_back(0xDB);
    AppendBe16(out, 2 + 1 + 128);
    out->push_back(static_cast<std::uint8_t>((1u << 4u) | table_id)); /* Pq=1（16bit） */
    for (int i = 0; i < 128; ++i)
    {
        out->push_back(fill);
    }
}

/** 按 FrameSpec 组装一帧合成 JPEG。
 *
 * @param spec 帧构造参数。
 * @return 完整 JPEG 帧字节序列。
 */
std::vector<std::uint8_t> BuildFrame(const FrameSpec &spec)
{
    std::vector<std::uint8_t> out;
    out.push_back(0xFF);
    out.push_back(0xD8); /* SOI */

    if (spec.with_dqt)
    {
        if (spec.dqt_pq1)
        {
            AppendDqt16(&out, 0, 0x11);
            if (spec.with_dqt1)
            {
                AppendDqt(&out, 1, 0x22);
            }
        }
        else if (spec.split_dqt)
        {
            AppendDqt(&out, 0, 0x11);
            if (spec.with_dqt1)
            {
                AppendDqt(&out, 1, 0x22);
            }
        }
        else
        {
            const std::uint8_t fill[2] = { 0x11, 0x22 };
            const int tables = spec.with_dqt1 ? 2 : 1;
            out.push_back(0xFF);
            out.push_back(0xDB);
            AppendBe16(&out, static_cast<std::uint16_t>(2 + 65 * tables));
            for (int t = 0; t < tables; ++t)
            {
                out.push_back(static_cast<std::uint8_t>(t)); /* Pq=0、Tq=t */
                for (int i = 0; i < 64; ++i)
                {
                    out.push_back(fill[t]);
                }
            }
        }
    }

    if (spec.with_sof0)
    {
        /* 组装完整段数据后按段长截取写入：段长截短时字段自然缺失、缺失部分
         * 补 0（模拟段长异常帧，解析器按段长整段跳过不受影响）。 */
        const unsigned data_bytes = (spec.sof_seg_len != 0 ? static_cast<unsigned>(spec.sof_seg_len) : 8u + 3u * spec.ncomp) - 2u;
        std::vector<std::uint8_t> seg;
        seg.push_back(8); /* precision */
        seg.push_back(static_cast<std::uint8_t>(spec.height >> 8));
        seg.push_back(static_cast<std::uint8_t>(spec.height & 0xFF));
        seg.push_back(static_cast<std::uint8_t>(spec.width >> 8));
        seg.push_back(static_cast<std::uint8_t>(spec.width & 0xFF));
        seg.push_back(static_cast<std::uint8_t>(spec.ncomp));
        const std::uint8_t hv[3] = { spec.luma_sampling, spec.chroma_sampling, spec.chroma_sampling };
        for (unsigned c = 0; c < spec.ncomp; ++c)
        {
            seg.push_back(static_cast<std::uint8_t>(spec.component_id_base + c)); /* 分量 id */
            seg.push_back(hv[c]);                                                 /* 采样因子 */
            seg.push_back(c == 0 ? 0 : 1);                                        /* 量化表号 */
        }
        seg.resize(data_bytes, 0x00);
        out.push_back(0xFF);
        out.push_back(0xC0); /* SOF0 */
        AppendBe16(&out, static_cast<std::uint16_t>(data_bytes + 2));
        out.insert(out.end(), seg.begin(), seg.end());
    }

    if (spec.with_dri)
    {
        out.push_back(0xFF);
        out.push_back(0xDD);
        AppendBe16(&out, 4);
        AppendBe16(&out, spec.restart_interval);
    }

    out.push_back(0xFF);
    out.push_back(0xC4); /* DHT（内容不参与解析，验证整段跳过） */
    AppendBe16(&out, 6);
    out.insert(out.end(), { 0x00, 0x01, 0x02, 0x03 });

    out.push_back(0xFF);
    out.push_back(0xDA); /* SOS */
    AppendBe16(&out, 8);
    out.insert(out.end(), { 0x01, 0x00, 0x02, 0x11, 0x03, 0x11 });

    out.insert(out.end(), spec.scan.begin(), spec.scan.end());

    if (spec.with_eoi)
    {
        out.push_back(0xFF);
        out.push_back(0xD9);
    }
    return out;
}

/** 在 SOS marker 之前插入自定义段（解析器扫到 SOS 即止，插段必须在其前）。
 *
 * @param frame 原始帧。
 * @param segment 待插入的完整段（含 marker）。
 * @return 插入后的完整帧。
 */
std::vector<std::uint8_t> InsertBeforeSos(const std::vector<std::uint8_t> &frame, const std::vector<std::uint8_t> &segment)
{
    const std::vector<std::uint8_t> sos_marker = { 0xFF, 0xDA };
    const auto sos_pos = std::search(frame.begin(), frame.end(), sos_marker.begin(), sos_marker.end());
    RTSP_CHECK(sos_pos != frame.end());
    std::vector<std::uint8_t> out;
    out.reserve(frame.size() + segment.size());
    out.insert(out.end(), frame.begin(), sos_pos);
    out.insert(out.end(), segment.begin(), segment.end());
    out.insert(out.end(), sos_pos, frame.end());
    return out;
}

} // namespace

RTSP_TEST_CASE(jpeg_parse_420_frame)
{
    const std::vector<std::uint8_t> frame = BuildFrame(FrameSpec{});
    JpegFrameInfo info;
    const Result result = ParseJpegFrame(frame.data(), frame.size(), &info);
    RTSP_CHECK(result.ok());
    RTSP_CHECK_EQ(info.type, 1); /* 0x22 = 4:2:0 */
    RTSP_CHECK_EQ(static_cast<unsigned>(info.width_blocks), 640u / 8u);
    RTSP_CHECK_EQ(static_cast<unsigned>(info.height_blocks), 360u / 8u);
    RTSP_CHECK(info.restart_interval == 0);
    RTSP_CHECK(info.qt[0] != nullptr && *info.qt[0] == 0x11);
    RTSP_CHECK(info.qt[1] != nullptr && *info.qt[1] == 0x22);
    RTSP_CHECK_EQ(info.scan_size, static_cast<std::size_t>(5));
    RTSP_CHECK(std::memcmp(info.scan, "\x12\x34\xFF\x00\x56", 5) == 0);
}

RTSP_TEST_CASE(jpeg_parse_422_and_dri)
{
    FrameSpec spec;
    spec.luma_sampling = 0x21; /* 4:2:2 */
    spec.with_dri = true;
    spec.restart_interval = 128;
    const std::vector<std::uint8_t> frame = BuildFrame(spec);
    JpegFrameInfo info;
    const Result result = ParseJpegFrame(frame.data(), frame.size(), &info);
    RTSP_CHECK(result.ok());
    RTSP_CHECK_EQ(info.type, 64); /* 4:2:2 基值 0 + restart 64 */
    RTSP_CHECK_EQ(static_cast<unsigned>(info.restart_interval), 128u);
}

RTSP_TEST_CASE(jpeg_parse_hisi_style_frame)
{
    /* 海思风格：分量 id 从 0 起、4:2:2 采样、双表一段 DQT。id 与表号
     * 分配不参与判定，与旧 live555 路径一致照发。 */
    FrameSpec spec;
    spec.component_id_base = 0;
    spec.luma_sampling = 0x21;
    const std::vector<std::uint8_t> frame = BuildFrame(spec);
    JpegFrameInfo info;
    RTSP_CHECK(ParseJpegFrame(frame.data(), frame.size(), &info).ok());
    RTSP_CHECK_EQ(info.type, 0);
    RTSP_CHECK_EQ(static_cast<unsigned>(info.width_blocks), 640u / 8u);
    RTSP_CHECK_EQ(static_cast<unsigned>(info.height_blocks), 360u / 8u);
}

RTSP_TEST_CASE(jpeg_parse_unknown_sampling_falls_back_to_type1)
{
    /* 4:4:4（亮度 1x1）：RFC 2435 无法表达，回退默认 type=1 照发。 */
    FrameSpec spec;
    spec.luma_sampling = 0x11;
    const std::vector<std::uint8_t> frame = BuildFrame(spec);
    JpegFrameInfo info;
    RTSP_CHECK(ParseJpegFrame(frame.data(), frame.size(), &info).ok());
    RTSP_CHECK_EQ(info.type, 1);
}

RTSP_TEST_CASE(jpeg_parse_grayscale_falls_back_to_type1)
{
    /* 灰度单分量：同样回退默认 type=1（旧路径照发，解码质量由接收端容忍）。 */
    FrameSpec spec;
    spec.ncomp = 1;
    spec.luma_sampling = 0x11;
    const std::vector<std::uint8_t> frame = BuildFrame(spec);
    JpegFrameInfo info;
    RTSP_CHECK(ParseJpegFrame(frame.data(), frame.size(), &info).ok());
    RTSP_CHECK_EQ(info.type, 1);
}

RTSP_TEST_CASE(jpeg_parse_chroma_non_1x1_passes)
{
    /* 色度非 1x1：不再拒绝（type 头不携带色度信息，按亮度采样定 type）。 */
    FrameSpec spec;
    spec.chroma_sampling = 0x21;
    const std::vector<std::uint8_t> frame = BuildFrame(spec);
    JpegFrameInfo info;
    RTSP_CHECK(ParseJpegFrame(frame.data(), frame.size(), &info).ok());
    RTSP_CHECK_EQ(info.type, 1);
}

RTSP_TEST_CASE(jpeg_parse_oversize_width_zeroes_blocks)
{
    /* 宽 2560 → 320 块超 8bit 上限：块数字段置 0，帧照发（旧路径同款）。 */
    FrameSpec spec;
    spec.width = 2560;
    const std::vector<std::uint8_t> frame = BuildFrame(spec);
    JpegFrameInfo info;
    RTSP_CHECK(ParseJpegFrame(frame.data(), frame.size(), &info).ok());
    RTSP_CHECK_EQ(info.width_blocks, 0);
    RTSP_CHECK_EQ(static_cast<unsigned>(info.height_blocks), 360u / 8u);
}

RTSP_TEST_CASE(jpeg_parse_short_sof_keeps_default_type)
{
    /* SOF0 段长不足 11（读不到亮度采样）：不解析字段，type 回退默认 1、
     * 宽高块数置 0，帧照发（旧路径同款门槛）。 */
    FrameSpec spec;
    spec.sof_seg_len = 10;
    const std::vector<std::uint8_t> frame = BuildFrame(spec);
    JpegFrameInfo info;
    RTSP_CHECK(ParseJpegFrame(frame.data(), frame.size(), &info).ok());
    RTSP_CHECK_EQ(info.type, 1);
    RTSP_CHECK_EQ(info.width_blocks, 0);
    RTSP_CHECK_EQ(info.height_blocks, 0);
}

RTSP_TEST_CASE(jpeg_parse_split_dqt_segments)
{
    /* 场景：两张量化表分处两个 DQT 段；预期两段都被收录，qt[0]/qt[1] 均有效。 */
    FrameSpec spec;
    spec.split_dqt = true;
    const std::vector<std::uint8_t> frame = BuildFrame(spec);
    JpegFrameInfo info;
    RTSP_CHECK(ParseJpegFrame(frame.data(), frame.size(), &info).ok());
    RTSP_CHECK(info.qt[0] != nullptr && *info.qt[0] == 0x11);
    RTSP_CHECK(info.qt[1] != nullptr && *info.qt[1] == 0x22);
}

RTSP_TEST_CASE(jpeg_parse_single_table)
{
    /* 单表（缺色度表）：解析允许，由发送端补发亮度表副本（量化一致无损）。 */
    FrameSpec spec;
    spec.with_dqt1 = false;
    spec.split_dqt = true;
    const std::vector<std::uint8_t> frame = BuildFrame(spec);
    JpegFrameInfo info;
    RTSP_CHECK(ParseJpegFrame(frame.data(), frame.size(), &info).ok());
    RTSP_CHECK(info.qt[0] != nullptr);
    RTSP_CHECK(info.qt[1] == nullptr);
}

RTSP_TEST_CASE(jpeg_parse_dqt_table_id_overflow_skipped)
{
    /* 表号超界（Tq=2）：该表无法映射到 RFC 2435 的两张表，跳过不拒帧，
     * 已合法记录的表 0/1 不受影响。 */
    FrameSpec spec;
    const std::vector<std::uint8_t> frame = BuildFrame(spec);
    std::vector<std::uint8_t> tq2_segment;
    tq2_segment.push_back(0xFF);
    tq2_segment.push_back(0xDB);
    tq2_segment.push_back(0x00);
    tq2_segment.push_back(67);
    tq2_segment.push_back(0x02); /* Pq=0、Tq=2 */
    for (int i = 0; i < 64; ++i)
    {
        tq2_segment.push_back(0x33);
    }
    const std::vector<std::uint8_t> patched = InsertBeforeSos(frame, tq2_segment);

    JpegFrameInfo info;
    RTSP_CHECK(ParseJpegFrame(patched.data(), patched.size(), &info).ok());
    RTSP_CHECK(info.qt[0] != nullptr && *info.qt[0] == 0x11);
    RTSP_CHECK(info.qt[1] != nullptr && *info.qt[1] == 0x22);
}

RTSP_TEST_CASE(jpeg_parse_dri_bad_length_ignored)
{
    /* DRI 段长异常：整段跳过、不采信重启间隔（旧路径同款）。 */
    FrameSpec spec;
    spec.luma_sampling = 0x21;
    const std::vector<std::uint8_t> frame = BuildFrame(spec);
    const std::vector<std::uint8_t> patched = InsertBeforeSos(frame, { 0xFF, 0xDD, 0x00, 0x06, 0x01, 0x02, 0x03, 0x04 });

    JpegFrameInfo info;
    const Result result = ParseJpegFrame(patched.data(), patched.size(), &info);
    RTSP_CHECK(result.ok());
    RTSP_CHECK_EQ(info.type, 0); /* 4:2:2 基值，未叠加 restart 偏移 */
    RTSP_CHECK(info.restart_interval == 0);
}

RTSP_TEST_CASE(jpeg_parse_no_eoi_kept_as_scan)
{
    /* 缺 EOI：尾部不截除，全部熵数据保留（宽松处理，交由解码端容忍）。 */
    FrameSpec spec;
    spec.with_eoi = false;
    const std::vector<std::uint8_t> frame = BuildFrame(spec);
    JpegFrameInfo info;
    RTSP_CHECK(ParseJpegFrame(frame.data(), frame.size(), &info).ok());
    RTSP_CHECK_EQ(info.scan_size, static_cast<std::size_t>(5));
}

RTSP_TEST_CASE(jpeg_reject_missing_soi)
{
    /* 场景：帧首缺 SOI；预期拒绝（InvalidArgument）且诊断定位到起始阶段 kBadStart。 */
    FrameSpec spec;
    const std::vector<std::uint8_t> frame = BuildFrame(spec);
    JpegFrameInfo info;
    JpegParseDiag diag;
    const Result result = ParseJpegFrame(frame.data() + 2, frame.size() - 2, &info, &diag);
    RTSP_CHECK(!result.ok());
    RTSP_CHECK(result.code == Status::InvalidArgument);
    RTSP_CHECK(diag.stage == JpegRejectStage::kBadStart);
}

RTSP_TEST_CASE(jpeg_reject_progressive)
{
    /* 渐进式：无 SOF0，结构不完整（旧路径同样丢帧）。 */
    FrameSpec spec;
    spec.with_sof0 = false;
    const std::vector<std::uint8_t> frame = BuildFrame(spec);
    JpegFrameInfo info;
    JpegParseDiag diag;
    const Result result = ParseJpegFrame(frame.data(), frame.size(), &info, &diag);
    RTSP_CHECK(!result.ok());
    RTSP_CHECK(result.code == Status::InvalidArgument);
    RTSP_CHECK(diag.stage == JpegRejectStage::kIncomplete);
    RTSP_CHECK_EQ(diag.value_a, 2u | 4u); /* SOS 已见、qt0 已见、缺 SOF0 */
}

RTSP_TEST_CASE(jpeg_reject_missing_quant_table)
{
    /* 无 DQT：帧结构不完整。 */
    FrameSpec spec;
    spec.with_dqt = false;
    const std::vector<std::uint8_t> frame = BuildFrame(spec);
    JpegFrameInfo info;
    JpegParseDiag diag;
    const Result result = ParseJpegFrame(frame.data(), frame.size(), &info, &diag);
    RTSP_CHECK(!result.ok());
    RTSP_CHECK(result.code == Status::InvalidArgument);
    RTSP_CHECK(diag.stage == JpegRejectStage::kIncomplete);
    RTSP_CHECK_EQ(diag.value_a, 1u | 2u); /* 缺 qt0 */
}

RTSP_TEST_CASE(jpeg_reject_truncated_segment)
{
    /* 段长越界：marker 之后的段长字段声称超出帧尾。 */
    FrameSpec spec;
    std::vector<std::uint8_t> frame = BuildFrame(spec);
    frame[4] = 0x7F; /* 把 DQT 段长改为 0x7Fxx，远超帧尾 */
    frame[5] = 0xFF;
    JpegFrameInfo info;
    JpegParseDiag diag;
    const Result result = ParseJpegFrame(frame.data(), frame.size(), &info, &diag);
    RTSP_CHECK(!result.ok());
    RTSP_CHECK(result.code == Status::InvalidArgument);
    RTSP_CHECK(diag.stage == JpegRejectStage::kBadSegment);
    RTSP_CHECK_EQ(diag.value_a, 0xDBu);   /* marker */
    RTSP_CHECK_EQ(diag.value_b, 0x7FFFu); /* 段长 */
}

RTSP_TEST_CASE(jpeg_reject_dqt_16bit_precision)
{
    /* 16bit 量化表（Pq=1）：打包器固定 Precision=0，无法合规传出。 */
    FrameSpec spec;
    spec.dqt_pq1 = true;
    const std::vector<std::uint8_t> frame = BuildFrame(spec);
    JpegFrameInfo info;
    JpegParseDiag diag;
    const Result result = ParseJpegFrame(frame.data(), frame.size(), &info, &diag);
    RTSP_CHECK(!result.ok());
    RTSP_CHECK(result.code == Status::Unsupported);
    RTSP_CHECK(diag.stage == JpegRejectStage::kDqtPrecision);
    RTSP_CHECK_EQ(diag.value_a, 0u); /* 表号 0 */
}

RTSP_TEST_CASE(jpeg_reject_stage_to_string)
{
    /* 场景：拒绝阶段枚举转字符串；预期与诊断日志约定的取值一一对应。 */
    RTSP_CHECK(std::strcmp(JpegRejectStageToString(JpegRejectStage::kBadSegment), "bad-segment") == 0);
    RTSP_CHECK(std::strcmp(JpegRejectStageToString(JpegRejectStage::kIncomplete), "incomplete") == 0);
    RTSP_CHECK(std::strcmp(JpegRejectStageToString(JpegRejectStage::kDqtPrecision), "dqt-16bit") == 0);
    RTSP_CHECK(std::strcmp(JpegRejectStageToString(JpegRejectStage::kNone), "none") == 0);
}
