/**
 * @FilePath     : cxx_compat.h
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2026-09-23
 * @Description   : C++11 基线下的常用类型特性兼容定义（std::void_t 为 C++17 特性）
 */

#pragma once

namespace CxxCompat_NS
{

/* void_t 惯用法：经 make_void 间接化，规避 CWG 1558 下未使用参数不参与 SFINAE 的问题 */
template <typename...>
struct make_void
{
    using type = void;
};

template <typename... Ts>
using void_t = typename make_void<Ts...>::type;

} // namespace CxxCompat_NS
