/**
 * @FilePath     : text.h
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2026-09-10 18:49:34
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-09-16 09:25:00
 * @Description  : 文本工具：Base64、十六进制、大小写无关比较、切片转字符串
 */
#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include "support/string_view.h"

namespace ipc_rtsp
{
namespace detail
{

/**
 * Base64 编码（SDP 的 sprop-parameter-sets 需要）。
 *
 * @param data 输入字节；可为 nullptr
 * @param len  输入字节数
 * @return Base64 文本（含 '=' 填充）；data 为空或 len 为 0 时返回空串
 */
std::string Base64Encode(const std::uint8_t *data, std::size_t len);

/**
 * 小写十六进制编码。
 *
 * @param data 输入字节；不可为 nullptr（len 为 0 时除外）
 * @param len  输入字节数
 * @return 长度为 2*len 的小写十六进制串
 */
std::string HexEncode(const std::uint8_t *data, std::size_t len);

/**
 * 大小写无关比较（ASCII）。
 *
 * @param lhs 左操作数
 * @param rhs 右操作数
 * @return 长度相同且逐字符忽略 ASCII 大小写相等时返回 true
 */
bool EqualsIgnoreCase(StringView lhs, StringView rhs);

/**
 * 查找 Header 值中的参数，例如在 `RTP/AVP/TCP;unicast;interleaved=0-1`
 * 中查找 `interleaved=`。参数名匹配是大小写敏感的子串查找，且命中位置
 * 必须位于串首或 ';'、' '、'=' 之后。
 *
 * @param text Header 值文本
 * @param name 参数名（按调用习惯携带 '='，如 "interleaved="）
 * @return 参数值切片（到 ';' 或 ',' 为止）；未找到与“参数名存在但值为空”
 *         都返回空切片，两者不可区分
 */
StringView FindParameter(StringView text, StringView name);

/**
 * 去除首尾空白（含 CR/LF）。
 *
 * @param text 输入文本
 * @return 去除首尾空白后的切片（不拷贝数据）
 */
StringView Trim(StringView text);

/**
 * 把 RTSP URI 归一化为“路径”形式：去掉 rtsp://host[:port] 前缀与查询串。
 *
 * @param uri 原始 URI（可带或不带 rtsp:// 前缀）
 * @return 以 '/' 开头的路径；仅有 host 无路径时返回 "/"
 */
std::string NormalizeUriPath(StringView uri);

} // namespace detail
} // namespace ipc_rtsp
