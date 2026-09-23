/**
 * @FilePath     : url_router.cpp
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2026-09-10 18:49:34
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-09-16 09:25:00
 * @Description  : URL 路由实现
 */

#include "rtsp/url_router.h"

#include "support/text.h"

namespace ipc_rtsp
{
namespace detail
{
namespace
{

/**
 * @brief 匹配 track 后缀（路径中码流路径之后的剩余段）。
 *
 * @param suffix 后缀文本；空或仅剩 '/' 视为视频轨（聚合控制 URL）
 * @return track 序号（kTrackVideo/kTrackAudio）；不匹配返回 -1
 */
int MatchTrackSuffix(StringView suffix)
{
    if (suffix.empty())
    {
        return kTrackVideo;
    }

    /* 允许 "/trackID=0"、"trackID=0"、"/track1"、"track1" 等形式。 */
    StringView text = suffix;
    if (!text.empty() && text.front() == '/')
    {
        text.remove_prefix(1);
    }
    if (text.empty())
    {
        return kTrackVideo;
    }

    if (EqualsIgnoreCase(text, "video"))
    {
        return kTrackVideo;
    }
    if (EqualsIgnoreCase(text, "audio"))
    {
        return kTrackAudio;
    }

    const StringView track_id_prefix = "trackID=";
    if (text.size() > track_id_prefix.size() && EqualsIgnoreCase(text.substr(0, track_id_prefix.size()), track_id_prefix))
    {
        const StringView value = text.substr(track_id_prefix.size());
        if (value == "0")
        {
            return kTrackVideo;
        }
        if (value == "1")
        {
            return kTrackAudio;
        }
        return -1;
    }

    const StringView track_prefix = "track";
    if (text.size() > track_prefix.size() && EqualsIgnoreCase(text.substr(0, track_prefix.size()), track_prefix))
    {
        const StringView value = text.substr(track_prefix.size());
        if (value == "1")
        {
            return kTrackVideo;
        }
        if (value == "2")
        {
            return kTrackAudio;
        }
        return -1;
    }

    return -1;
}

} // namespace

void UrlRouter::Configure(const std::vector<StreamConfig> &streams)
{
    entries_.clear();
    for (const StreamConfig &stream : streams)
    {
        Entry entry;
        entry.stream = stream.id;
        entry.path = stream.path;
        entries_.push_back(entry);
    }
}

const UrlRouter::Entry *UrlRouter::Find(StringView normalized_path, StringView *suffix) const
{
    for (const Entry &entry : entries_)
    {
        std::string prefix = "/";
        prefix += entry.path;
        /* 前缀必须整段匹配：比较完后剩余段要么为空、要么以 '/' 开头（进入
         * track 后缀段）。这样 "/main" 不会误配 "/main2"——后者的剩余段
         * "2" 既非空也不以 '/' 开头，直接跳过。 */
        if (normalized_path.size() < prefix.size())
        {
            continue;
        }
        if (normalized_path.compare(0, prefix.size(), prefix) != 0)
        {
            continue;
        }

        const StringView rest = normalized_path.substr(prefix.size());
        if (!rest.empty() && rest.front() != '/')
        {
            continue;
        }
        if (suffix != nullptr)
        {
            *suffix = rest;
        }
        return &entry;
    }
    return nullptr;
}

bool UrlRouter::ResolveStream(StringView path, StreamId *stream) const
{
    if (stream == nullptr)
    {
        return false;
    }

    const std::string normalized = NormalizeUriPath(path);
    StringView suffix;
    const Entry *entry = Find(normalized, &suffix);
    if (entry == nullptr)
    {
        return false;
    }

    /* 聚合控制允许带 track 后缀，也允许不带。 */
    if (MatchTrackSuffix(suffix) < 0)
    {
        return false;
    }

    *stream = entry->stream;
    return true;
}

bool UrlRouter::ResolveTrack(StringView path, StreamId *stream, int *track_index) const
{
    if (stream == nullptr || track_index == nullptr)
    {
        return false;
    }

    const std::string normalized = NormalizeUriPath(path);
    StringView suffix;
    const Entry *entry = Find(normalized, &suffix);
    if (entry == nullptr)
    {
        return false;
    }

    const int track = MatchTrackSuffix(suffix);
    if (track < 0)
    {
        return false;
    }

    *stream = entry->stream;
    *track_index = track;
    return true;
}

const std::string &UrlRouter::StreamPath(StreamId stream) const
{
    static const std::string kEmpty;
    for (const Entry &entry : entries_)
    {
        if (entry.stream == stream)
        {
            return entry.path;
        }
    }
    return kEmpty;
}

} // namespace detail
} // namespace ipc_rtsp
