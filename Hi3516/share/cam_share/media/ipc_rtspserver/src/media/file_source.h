/**
 * @FilePath     : file_source.h
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2026-09-10 18:49:34
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-09-16 09:25:00
 * @Description  : 主机验证用的 Annex-B 文件媒体源
 */

/*
 * 仅用于 PoC 与自动化测试：把离线码流文件切成 Access Unit 后按帧率推送。
 * AU 边界规则（实现见 file_source.cpp 二遍扫描）：
 * - AUD 开启新 AU；
 * - VCL 仅当判定为新图像首片时开启新 AU（多 slice 同帧不拆成多个 AU）；
 * - 参数集与其余非 VCL NAL 并入当前 AU。
 */
#pragma once

#include "ipc_rtsp/frame.h"
#include "ipc_rtsp/result.h"

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace ipc_rtsp
{
namespace detail
{

/** 离线 Annex-B 文件源。 */
class AnnexBFileSource
{
public:
    /**
     * @brief 读取并解析文件；失败返回错误。
     * @param path Annex-B 码流文件路径
     * @param codec 码流编码类型（决定 NAL 分类与 AU 切分依据）
     * @return 成功返回 Ok；打不开/文件为空/无起始码返回对应错误
     */
    Result Load(const std::string &path, Codec codec);

    /**
     * 取下一个 AU。
     *
     * @param[out] out 帧视图（指向内部缓冲，Load 之后一直有效）
     * @return 到达末尾且未循环时返回 false
     */
    bool NextAu(VideoFrameView *out);

    /** 游标回到首个 AU。 */
    void Rewind()
    {
        cursor_ = 0;
    }

    /** 设置是否循环播放（到末尾时回绕重播）。 */
    void set_loop(bool loop)
    {
        loop_ = loop;
    }

    /** @brief 解析出的 AU 总数。@return AU 数量。 */
    std::size_t au_count() const
    {
        return aus_.size();
    }

    /** @brief Load 时指定的编码类型。@return 编码类型。 */
    Codec codec() const
    {
        return codec_;
    }

private:
    /* 单个 AU 在文件缓冲中的位置与属性。 */
    struct AccessUnit
    {
        /* 起始码首字节在 data_ 中的偏移。 */
        std::size_t offset = 0;
        /* AU 总字节数（字节，含内部起始码）。 */
        std::size_t size = 0;
        /* 是否含 IDR/CRA（关键 AU，可作为解码起点）。 */
        bool key = false;
        /* AU 首个 NAL 的类型（表征该 AU 的角色，如 AUD/SPS/IDR）。 */
        NalUnitType first_nal = NalUnitType::Unknown;
    };

    /* 整个文件内容（memory: 拥有；NextAu 输出视图指向其内部，Load 后
     * 未再 Load 前一直有效）。 */
    std::vector<std::uint8_t> data_;
    /* 解析出的 AU 表（Load 时填充）。 */
    std::vector<AccessUnit> aus_;
    /* 下一个待取 AU 下标；到末尾时按 loop_ 决定回绕或停止。 */
    std::size_t cursor_ = 0;
    /* 到末尾是否回绕重播。 */
    bool loop_ = true;
    /* Load 时指定的编码类型。 */
    Codec codec_ = Codec::H264;
};

} // namespace detail
} // namespace ipc_rtsp
