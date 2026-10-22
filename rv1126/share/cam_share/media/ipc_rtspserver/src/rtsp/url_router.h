/**
 * @FilePath     : url_router.h
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2026-09-10 18:49:34
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-09-16 09:25:00
 * @Description  : RTSP URL 路由
 */

/*
 * 产品兼容约定：默认路径为 /Streaming/Channels/101（主）与 102（子），
 * 可经 Configure(StreamConfig) 配置为任意路径覆盖默认值。
 * SETUP 允许三种 track 表达：trackID=0/1、track1/track2、video/audio。
 */
#pragma once

#include "ipc_rtsp/config.h"

#include <string>
#include "support/string_view.h"
#include <vector>

namespace ipc_rtsp
{
namespace detail
{

/** track 序号：0 = 视频，1 = 音频。 */
constexpr int kTrackVideo = 0;
constexpr int kTrackAudio = 1;

/** URL 路由器。 */
class UrlRouter
{
public:
    /**
     * @brief 装载码流路径表（全量重建）。
     *
     * @param streams 码流配置列表；内部只拷贝各配置的 id 与 path
     * @return {void}
     */
    void Configure(const std::vector<StreamConfig> &streams);

    /**
     * @brief 解析聚合 URL / DESCRIBE 路径。
     *
     * @param path 请求 URI 路径（内部先做归一化）
     * @param stream 输出命中的码流；可为 nullptr 之外必填
     * @return 命中返回 true 并写入 @p stream
     */
    bool ResolveStream(StringView path, StreamId *stream) const;

    /**
     * @brief 解析 SETUP 的 track URI。
     *
     * @param path 请求 URI 路径（内部先做归一化）
     * @param stream 输出命中的码流
     * @param track_index 输出 track 序号（kTrackVideo/kTrackAudio）
     * @return 路径与 track 后缀均合法返回 true，否则 false
     */
    bool ResolveTrack(StringView path, StreamId *stream, int *track_index) const;

    /**
     * @brief 码流路径（不含前导 '/'）。
     *
     * @param stream 目标码流
     * @return 配置的路径；未配置返回空串
     */
    const std::string &StreamPath(StreamId stream) const;

private:
    /** 单个码流的路由表项。 */
    struct Entry
    {
        StreamId stream;  /**< 所属码流 */
        std::string path; /**< RTSP 路径，不含前导 '/' */
    };

    /**
     * @brief 前缀匹配已归一化路径，输出路径后缀（track 段）。
     *
     * @param normalized_path 归一化后的请求路径（以 '/' 开头）
     * @param suffix 输出码流路径之后的剩余段（可能为空）；可为 nullptr
     * @return 命中的表项指针，未命中返回 nullptr
     */
    const Entry *Find(StringView normalized_path, StringView *suffix) const;

    /* 路由表；Configure() 全量重建，查询期只读。 */
    std::vector<Entry> entries_;
};

} // namespace detail
} // namespace ipc_rtsp
