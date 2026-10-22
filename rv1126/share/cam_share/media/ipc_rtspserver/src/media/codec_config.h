/**
 * @FilePath     : codec_config.h
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2026-09-10 18:49:34
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-09-16 09:25:00
 * @Description  : 参数集缓存（SPS/PPS/VPS）
 */

/*
 * 缓存的是副本，不持有 VENC 原始指针；固定容量数组，稳态不分配。
 * generation 用于检测编码参数变化：分辨率/profile 变化时 DESCRIBE
 * 需要返回新 SDP。
 */
#pragma once

#include "ipc_rtsp/types.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <string>

namespace ipc_rtsp
{
namespace detail
{

/** 参数集最大长度（H.265 SPS 通常 < 100 字节，这里留足余量）。 */
constexpr std::size_t kMaxParameterSetBytes = 512;

/** 单个参数集副本。 */
struct ParameterSet
{
    /* 参数集净荷（不含起始码；未用区域保持 0）。 */
    std::array<std::uint8_t, kMaxParameterSetBytes> data{};
    /* 有效字节数（字节；0 表示未填充）。 */
    std::size_t size = 0;

    /** @brief 是否已填充有效参数集。@return size > 0 返回 true。 */
    bool valid() const
    {
        return size > 0;
    }
};

/**
 * 一路码流的 codec 参数集缓存。
 *
 * 线程约定：只在 I/O 线程更新与读取（DESCRIBE 与打包在同一线程）。
 */
class CodecConfigCache
{
public:
    /**
     * @brief 构造指定编码类型的空缓存。
     * @param codec 码流编码类型（决定 Ready() 与 sprop 输出口径）
     */
    explicit CodecConfigCache(Codec codec);

    /**
     * 用新参数集更新缓存。
     *
     * @param type 参数集 NAL 类型（VPS/SPS/PPS）
     * @param nal 参数集净荷（不含起始码）
     * @param size 参数集字节数（字节）
     * @return 参数集内容发生变化时返回 true（generation 自增）
     */
    bool Update(NalUnitType type, const std::uint8_t *nal, std::size_t size);

    /** 是否已经具备生成 SDP 所需的最小参数集。@return H.264/H.265 均要求
     *  SPS+PPS 有效（VPS 不强求，实际码流中不一定存在）。 */
    bool Ready() const;

    /** 参数集版本号，内容变化时自增。@return 当前代次（从 1 起）。 */
    std::uint32_t Generation() const
    {
        return generation_;
    }

    /** @brief 构造时绑定的编码类型。@return 编码类型。 */
    Codec codec() const
    {
        return codec_;
    }

    /** H.264: "base64(sps),base64(pps)"；H.265: 空。@return sprop 值，
     *  非 H.264 或参数集未就绪返回空串。 */
    std::string H264SpropParameterSets() const;

    /** @return H.265 VPS 的 base64 sprop；非 H.265 或 VPS 未就绪返回空串。 */
    std::string H265SpropVps() const;

    /** @return H.265 SPS 的 base64 sprop；非 H.265 或 SPS 未就绪返回空串。 */
    std::string H265SpropSps() const;

    /** @return H.265 PPS 的 base64 sprop；非 H.265 或 PPS 未就绪返回空串。 */
    std::string H265SpropPps() const;

    /** H.264 `profile-level-id`（6 位十六进制）。@return 解析结果；无法
     *  解析时返回 "42e01e"。 */
    std::string H264ProfileLevelId() const;

    /** @return VPS 参数集副本常引用（用于补发）。 */
    const ParameterSet &vps() const
    {
        return vps_;
    }

    /** @return SPS 参数集副本常引用（用于补发）。 */
    const ParameterSet &sps() const
    {
        return sps_;
    }

    /** @return PPS 参数集副本常引用（用于补发）。 */
    const ParameterSet &pps() const
    {
        return pps_;
    }

private:
    /**
     * @brief 返回指定参数集类型对应的槽位引用。
     * @param type 参数集 NAL 类型
     * @return 槽位引用；非参数集类型防御性返回 SPS 槽
     */
    ParameterSet &SlotFor(NalUnitType type);

    /* 构造时绑定的编码类型。 */
    Codec codec_;
    /* VPS 副本。 */
    ParameterSet vps_;
    /* SPS 副本。 */
    ParameterSet sps_;
    /* PPS 副本。 */
    ParameterSet pps_;
    /* 参数集代次，内容变化时自增；初值取 1 而非 0：值初始化的快照
     * （CodecConfigSnapshot::generation 默认 0）表示"未组装"，真实缓存
     * 代次恒 ≥ 1，二者永不混淆。 */
    std::uint32_t generation_ = 1;
};

} // namespace detail
} // namespace ipc_rtsp
