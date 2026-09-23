/**
 * @FilePath     : file_source.cpp
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2026-09-10 18:49:34
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-09-16 09:25:00
 * @Description  : Annex-B 文件媒体源实现
 */

#include "media/file_source.h"

#include "media/annexb_scanner.h"
#include "support/log.h"

#include <cerrno>
#include <cstdio>
#include <cstring>

namespace ipc_rtsp
{
namespace detail
{
namespace
{
constexpr const char *kTag = "file";

/**
 * @brief 回看 NAL 起始码长度（3 或 4 字节）。
 * @param base 缓冲起始
 * @param offset 起始码结束位置（从该处向前回看）
 * @return 起始码字节数（3/4）；无起始码返回 0
 */
std::size_t StartCodeLengthBefore(const std::uint8_t *base, std::size_t offset)
{
    if (offset >= 4 && base[offset - 4] == 0 && base[offset - 3] == 0 && base[offset - 2] == 0 && base[offset - 1] == 1)
    {
        return 4;
    }
    if (offset >= 3 && base[offset - 3] == 0 && base[offset - 2] == 0 && base[offset - 1] == 1)
    {
        return 3;
    }
    return 0;
}
} // namespace

Result AnnexBFileSource::Load(const std::string &path, Codec codec)
{
    data_.clear();
    aus_.clear();
    cursor_ = 0;
    codec_ = codec;

    FILE *file = std::fopen(path.c_str(), "rb");
    if (file == nullptr)
    {
        IPC_RTSP_LOGE(kTag, "打开媒体文件失败 path:%s errno:%d", path.c_str(), errno);
        return Result::Io(errno);
    }

    std::uint8_t chunk[64 * 1024];
    for (;;)
    {
        const std::size_t read_bytes = std::fread(chunk, 1, sizeof chunk, file);
        if (read_bytes > 0)
        {
            data_.insert(data_.end(), chunk, chunk + read_bytes);
        }
        if (read_bytes < sizeof chunk)
        {
            break;
        }
    }
    std::fclose(file);

    if (data_.empty())
    {
        IPC_RTSP_LOGE(kTag, "媒体文件为空 path:%s", path.c_str());
        return Result::Fail(Status::InvalidArgument);
    }

    /* 第一遍：收集 NAL 边界与类型。 */
    struct NalEntry
    {
        /* 起始码首字节在 data_ 中的偏移。 */
        std::size_t start_code_offset = 0;
        /* 起始码长度（3 或 4）。 */
        std::size_t code_len = 0;
        /* NAL 净荷长度（不含起始码）。 */
        std::size_t size = 0;
        NalUnitType type = NalUnitType::Unknown;
    };
    std::vector<NalEntry> nals;
    ForEachNal(codec_,
               data_.data(),
               data_.size(),
               [&](const NalRange &nal)
               {
                   NalEntry entry;
                   entry.start_code_offset = static_cast<std::size_t>(nal.data - data_.data());
                   const std::size_t code_len = StartCodeLengthBefore(data_.data(), entry.start_code_offset);
                   entry.start_code_offset -= code_len;
                   entry.code_len = code_len;
                   entry.size = nal.size;
                   entry.type = nal.type;
                   nals.push_back(entry);
                   return true;
               });

    if (nals.empty())
    {
        IPC_RTSP_LOGE(kTag, "媒体文件未找到Annex-B起始码 path:%s", path.c_str());
        return Result::Fail(Status::InvalidArgument);
    }

    /* 第二遍：按 AU 规则切分。
     * - AUD 直接开启新 AU；
     * - VCL NAL 只有在“是新图像首片”时才开启新 AU（这样多 slice 编码的
     *   同一帧不会被拆成多个 AU）；
     * - 其余非 VCL NAL 归入当前 AU。 */
    std::size_t au_start = nals.front().start_code_offset;
    NalUnitType au_first_nal = nals.front().type;
    bool au_has_vcl = IsVclNal(nals.front().type);
    for (std::size_t i = 1; i < nals.size(); ++i)
    {
        const std::uint8_t *nal_ptr = data_.data() + nals[i].start_code_offset + nals[i].code_len;
        const bool first_slice = IsFirstSliceOfPicture(codec_, nal_ptr, nals[i].size);
        const bool starts_new_au = (nals[i].type == NalUnitType::Aud) || (first_slice && au_has_vcl);
        if (!starts_new_au)
        {
            if (IsVclNal(nals[i].type))
            {
                au_has_vcl = true;
            }
            continue;
        }

        AccessUnit unit;
        unit.offset = au_start;
        unit.size = nals[i].start_code_offset - au_start;
        unit.first_nal = au_first_nal;
        unit.key = false; /* 关键性在切分完成后统一标记。 */
        aus_.push_back(unit);

        au_start = nals[i].start_code_offset;
        au_first_nal = nals[i].type;
        au_has_vcl = IsVclNal(nals[i].type);
    }

    AccessUnit au;
    au.offset = au_start;
    au.size = data_.size() - au_start;
    au.first_nal = au_first_nal;
    aus_.push_back(au);

    /* 标记关键 AU：包含 IDR/CRA 的 AU。 */
    for (AccessUnit &unit : aus_)
    {
        ForEachNal(codec_,
                   data_.data() + unit.offset,
                   unit.size,
                   [&](const NalRange &nal)
                   {
                       if (IsKeyNal(nal.type))
                       {
                           unit.key = true;
                           return false;
                       }
                       return true;
                   });
    }

    IPC_RTSP_LOGI(kTag,
                  "媒体文件加载完成 path:%s bytes:%zu aus:%zu codec:%s",
                  path.c_str(),
                  data_.size(),
                  aus_.size(),
                  CodecToString(codec_));
    return Result::Ok();
}

bool AnnexBFileSource::NextAu(VideoFrameView *out)
{
    if (out == nullptr || aus_.empty())
    {
        return false;
    }
    if (cursor_ >= aus_.size())
    {
        if (!loop_)
        {
            return false;
        }
        cursor_ = 0;
    }

    const AccessUnit &au = aus_[cursor_];
    ++cursor_;

    out->data = data_.data() + au.offset;
    out->size = au.size;
    out->codec = codec_;
    out->first_nal = au.first_nal;
    out->key = au.key;
    out->has_pts = false; /* 由 PoC 按帧率补齐时间戳 */
    out->pts_us = 0;
    return true;
}

} // namespace detail
} // namespace ipc_rtsp
