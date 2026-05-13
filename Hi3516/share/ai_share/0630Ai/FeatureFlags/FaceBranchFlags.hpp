#pragma once

#include "FeatureFlags.hpp"

namespace Ai0630_NS
{

    enum class AiFaceFeature_E : uint32_t
    {
        FACE_DETECT   = 1 << 0, /* 人脸检测 */
        HEAD_DETECT   = 1 << 1, /* 人头检测 */
        FAST_POSE     = 1 << 2, /* 人体关键点检测 */
        FACE_FEATURE  = 1 << 3, /* 人脸特征提取 */
        HUMAN_FEATURE = 1 << 4, /* 人体特征提取 */
        FACE_EMOTION  = 1 << 5, /* 人脸表情分类 */
    };

    class FaceBranchFlags : public FeatureFlags
    {
    protected:

        std::map<uint32_t, std::string> getFlagMap() const override
        {
            return {
                { static_cast<uint32_t>(AiFaceFeature_E::FACE_DETECT),   std::string("人脸检测")          },
                { static_cast<uint32_t>(AiFaceFeature_E::HEAD_DETECT),   std::string("人头检测")          },
                { static_cast<uint32_t>(AiFaceFeature_E::FAST_POSE),     std::string("人体关键点检测") },
                { static_cast<uint32_t>(AiFaceFeature_E::FACE_FEATURE),  std::string("人脸特征提取")    },
                { static_cast<uint32_t>(AiFaceFeature_E::HUMAN_FEATURE), std::string("人体特征提取")    },
                { static_cast<uint32_t>(AiFaceFeature_E::FACE_EMOTION),  std::string("人脸表情分类")    },
            };
        }
    };

}    // namespace Ai0630_NS
