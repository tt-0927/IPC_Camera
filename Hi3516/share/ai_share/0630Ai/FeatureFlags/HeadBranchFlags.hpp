#pragma once

#include "FeatureFlags.hpp"

namespace Ai0630_NS
{

    enum class AiHeadFeature_E : uint32_t
    {
        HEAD_DETECT = 1 << 0, /* 人头检测 */
        FAST_POSE   = 1 << 1, /* 人体关键点检测 */
    };

    class HeadBranchFlags : public FeatureFlags
    {
    protected:

        std::map<uint32_t, std::string> getFlagMap() const override
        {
            return {
                { static_cast<uint32_t>(AiHeadFeature_E::HEAD_DETECT), std::string("人头检测")          },
                { static_cast<uint32_t>(AiHeadFeature_E::FAST_POSE),   std::string("人体关键点检测") },
            };
        }
    };

}    // namespace Ai0630_NS
