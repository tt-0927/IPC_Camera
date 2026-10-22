/**
 * @FilePath     : frame_reader.cpp
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2026-09-14 18:33:45
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-09-16 09:25:00
 * @Description  : FrameReader 实现
 */

#include "media/frame_reader.h"

namespace ipc_rtsp
{
namespace detail
{

bool FrameReader::Attach(const GopSnapshot &snapshot)
{
    if (!snapshot.storage)
    {
        return false;
    }
    snapshot_ = snapshot;
    cursor_ = 0;
    audio_cursor_ = 0;
    return true;
}

void FrameReader::Detach()
{
    snapshot_ = GopSnapshot{};
    cursor_ = 0;
    audio_cursor_ = 0;
}

ReaderNext FrameReader::Peek(std::uint64_t current_epoch, const PendingFrame **out) const
{
    if (!attached())
    {
        return ReaderNext::ReplayFinished;
    }
    if (snapshot_.epoch != current_epoch)
    {
        return ReaderNext::Stale;
    }
    if (cursor_ >= snapshot_.replay_end_index)
    {
        return ReaderNext::ReplayFinished;
    }
    *out = &snapshot_.storage->frames[cursor_];
    return ReaderNext::FrameReady;
}

void FrameReader::Consume()
{
    if (cursor_ < snapshot_.replay_end_index)
    {
        ++cursor_;
    }
}

ReaderNext FrameReader::PeekAudio(std::uint64_t current_epoch, const AudioPendingFrame **out) const
{
    if (!attached())
    {
        return ReaderNext::ReplayFinished;
    }
    if (snapshot_.epoch != current_epoch)
    {
        return ReaderNext::Stale;
    }
    if (!snapshot_.audio || audio_cursor_ >= snapshot_.audio->frames.size())
    {
        return ReaderNext::ReplayFinished;
    }
    *out = &snapshot_.audio->frames[audio_cursor_];
    return ReaderNext::FrameReady;
}

void FrameReader::ConsumeAudio()
{
    if (snapshot_.audio && audio_cursor_ < snapshot_.audio->frames.size())
    {
        ++audio_cursor_;
    }
}

} // namespace detail
} // namespace ipc_rtsp
