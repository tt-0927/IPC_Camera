/**
 * @FilePath     : string_view.h
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2026-09-22 14:30:00
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-09-22 14:30:00
 * @Description  : std::string_view 兼容子集：富瀚 gcc 6.5 工具链无 <string_view>
 */

/*
 * 富瀚 FHV512 交叉工具链的 libstdc++ 为 GCC 6.5，缺少 <string_view>
 * （正式版 GCC 7 才提供）。本类只实现库内实际使用的操作子集，语义与
 * std::string_view 保持一致：不拥有数据、不保证 NUL 结尾，substr/find
 * 均为零拷贝引用。调用方须保证被引用文本的生命周期覆盖视图的使用期。
 * 与标准的两处有意偏离（均为收敛行为，现有调用点不会触发）：
 * 1. substr(pos > size()) 标准抛 out_of_range，本实现返回空切片；
 * 2. 构造传入 nullptr 的 const char* 标准为未定义，本实现按空切片处理。
 */
#pragma once

#include <cstddef>
#include <cstring>
#include <string>

namespace ipc_rtsp
{
namespace detail
{

/** 只读字符串切片（std::string_view 兼容子集）。 */
class StringView
{
public:
    /** 无效位置常量，语义同 std::string_view::npos。 */
    static constexpr std::size_t npos = static_cast<std::size_t>(-1);

    /** 空切片。 */
    StringView() = default;

    /** 字符串字面量 / C 字符串；nullptr 按空切片处理。 */
    StringView(const char *text) : data_(text), size_(text == nullptr ? 0u : std::strlen(text)) {}

    /** 显式给出长度；text 为 nullptr 时 size 须为 0。 */
    StringView(const char *text, std::size_t size) : data_(text), size_(size) {}

    /** 由 std::string 构造，引用其数据，不拷贝。 */
    StringView(const std::string &text) : data_(text.data()), size_(text.size()) {}

    /** 数据指针；空切片可能为 nullptr，调用方须配合 size() 使用。 */
    const char *data() const
    {
        return data_;
    }

    std::size_t size() const
    {
        return size_;
    }

    bool empty() const
    {
        return size_ == 0u;
    }

    /** 首字符；调用方须保证非空（同 std::string_view 前置条件）。 */
    char front() const
    {
        return data_[0];
    }

    /** 越界为未定义行为（同 std::string_view，不做检查）。 */
    char operator[](std::size_t index) const
    {
        return data_[index];
    }

    void remove_prefix(std::size_t count)
    {
        data_ += count;
        size_ -= count;
    }

    void remove_suffix(std::size_t count)
    {
        size_ -= count;
    }

    /**
     * 子串查找。
     *
     * @param needle 被查找的子串
     * @param pos 起始位置；大于 size() 时直接返回 npos
     * @return 命中的绝对下标；未找到返回 npos
     */
    std::size_t find(StringView needle, std::size_t pos = 0u) const
    {
        if (pos > size_ || needle.size_ > size_ - pos)
        {
            return npos;
        }
        if (needle.size_ == 0u)
        {
            return pos;
        }
        /* Authorization 头等目标文本很短，朴素搜索足够。 */
        for (std::size_t i = pos; i <= size_ - needle.size_; ++i)
        {
            if (std::memcmp(data_ + i, needle.data_, needle.size_) == 0)
            {
                return i;
            }
        }
        return npos;
    }

    /** 单字符查找，语义同 std::string_view::find(char, pos)。 */
    std::size_t find(char ch, std::size_t pos = 0u) const
    {
        return find(StringView(&ch, 1u), pos);
    }

    /**
     * 在 @p set 字符集合中逐字符查找，返回首个命中下标。
     *
     * @param set NUL 结尾的字符集合
     * @param pos 起始位置；不小于 size() 时返回 npos
     * @return 命中的绝对下标；未找到返回 npos
     */
    std::size_t find_first_of(const char *set, std::size_t pos = 0u) const
    {
        for (std::size_t i = (pos > size_ ? size_ : pos); i < size_; ++i)
        {
            if (data_[i] != '\0' && std::strchr(set, data_[i]) != nullptr)
            {
                return i;
            }
        }
        return npos;
    }

    /**
     * 取子切片。
     *
     * @param pos 起始位置；大于 size() 时返回空切片（标准为抛异常）
     * @param count 最大长度；npos 表示到串尾
     * @return 子切片，与母串共享数据
     */
    StringView substr(std::size_t pos = 0u, std::size_t count = npos) const
    {
        if (pos > size_)
        {
            return StringView();
        }
        const std::size_t rest = size_ - pos;
        return StringView(data_ + pos, count < rest ? count : rest);
    }

    /**
     * 子串比较，语义同 std::string_view::compare(pos, count, view)：
     * 先取 [pos, pos+count) 子切片再按 memcmp 规则与 @p other 比较。
     *
     * @return 子切片小于/等于/大于 other 分别返回负数/0/正数
     */
    int compare(std::size_t pos, std::size_t count, StringView other) const
    {
        const StringView lhs = substr(pos, count);
        const std::size_t common = lhs.size_ < other.size_ ? lhs.size_ : other.size_;
        if (common != 0u)
        {
            const int result = std::memcmp(lhs.data_, other.data_, common);
            if (result != 0)
            {
                return result < 0 ? -1 : 1;
            }
        }
        if (lhs.size_ < other.size_)
        {
            return -1;
        }
        return lhs.size_ > other.size_ ? 1 : 0;
    }

private:
    const char *data_ = nullptr;
    std::size_t size_ = 0u;
};

/** 逐字节相等比较；两侧均可由字面量或 std::string 隐式转换。 */
inline bool operator==(StringView lhs, StringView rhs)
{
    return lhs.size() == rhs.size() && (lhs.size() == 0u || std::memcmp(lhs.data(), rhs.data(), lhs.size()) == 0);
}

inline bool operator!=(StringView lhs, StringView rhs)
{
    return !(lhs == rhs);
}

} // namespace detail
} // namespace ipc_rtsp
