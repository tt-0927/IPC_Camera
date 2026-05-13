#pragma once
#include <bitset>
#include <cstdint>
#include <iostream>
#include <map>
#include <string>

namespace Ai0630_NS
{
    /**
     * @brief 通用功能标志位管理类（可复用）
     */
    class FeatureFlags
    {
    public:

        FeatureFlags()
            : m_nFlags(0) {}

        virtual ~FeatureFlags() = default;

        /**
         * @brief 启用功能
         */
        void enable(uint32_t nFlag)
        {
            m_nFlags |= nFlag;
        }

        /**
         * @brief 禁用功能
         */
        void disable(uint32_t nFlag)
        {
            m_nFlags &= ~nFlag;
        }

        /**
         * @brief 检查功能是否启用
         */
        bool isEnabled(uint32_t nFlag) const
        {
            return (m_nFlags & nFlag) != 0;
        }

        /**
         * @brief 设置标志位（可用于配置导入）
         */
        void setFlags(uint32_t flags)
        {
            m_nFlags = flags;
        }

        /**
         * @brief 获取当前标志位（可用于配置导出）
         */
        uint32_t getFlags() const
        {
            return m_nFlags;
        }

        /**
         * @brief 清空所有标志位
         */
        void clear()
        {
            m_nFlags = 0;
        }

        /**
         * @brief 打印当前状态（需子类提供名称映射）
         */
        void printStatus() const
        {
            std::cout << "当前功能状态 (0b" << std::bitset<32>(m_nFlags) << ")" << std::endl;
            for (const auto& [nFlag, strName] : getFlagMap())
            {
                if (isEnabled(nFlag))
                {
                    std::cout << "✅ 已启用: " << strName << std::endl;
                }
                else
                {
                    std::cout << "❌ 未启用: " << strName << std::endl;
                }
            }
        }

    protected:

        /**
         * @brief 子类需要实现：返回枚举值 -> 名称的映射表
         */
        virtual std::map<uint32_t, std::string> getFlagMap() const = 0;

    private:

        uint32_t m_nFlags;
    };

}    // namespace Ai0630_NS