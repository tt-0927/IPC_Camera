/*
 * @FilePath     : Scenario.hpp
 * @Author       : 严泽辉 yanzeh@kfb.cn
 * @Date         : 2024-05-17 17:17:26
 * @LastEditors: 李辉 lihui@kfb.cn
 * @LastEditTime: 2024-10-31 16:51:24
 * @Description  :
 */
#pragma once


#include "FaceExpressV1_0.hpp"
#include "FaceExpressV2_0.hpp"
#include "FaceRecV1_0.hpp"
#include "HeadCountV1_0.hpp"
#include "HeadCountV2_0.hpp"
#include "StudentBehaviorV1_0.hpp"
#include "StudentBehaviorV2_0.hpp"
#include "TrackerV1_0.hpp"
#include "VoiceWakeUpV1_0.hpp"
#include "NumberOcrV1_0.hpp"
#include "VirtualCutoutV1_0.hpp"
#include "HumanAreaDetect.hpp"

namespace AiScenario_NS
{
    class CScenario
    {
    public:

        static Scenario_NS::CScenarioBase* create(InParam_S stInParam)
        {
            switch (stInParam.stNeedParam.enType)
            {
                /* 人数统计 */
                case Type_E::HUMAN_CUTOUT:
                {
                    switch (stInParam.stNeedParam.enVersions)
                    {
                        case Versions_E::V1_0:
                        {
                            return new Scenario_NS::CHeadCountV1_0(stInParam);
                        }
                        case Versions_E::V2_0:
                        {
                            return new Scenario_NS::CHeadCountV2_0(stInParam);
                        }
                        default:
                            break;
                    }
                    break;
                }
                /* 学生行为分析 */
                case Type_E::STUDENT_BEHAVIOR:
                {
                    switch (stInParam.stNeedParam.enVersions)
                    {
                        case Versions_E::V1_0:
                        {
                            return new Scenario_NS::CStudentBehaviorV1_0(stInParam);
                        }
                        case Versions_E::V2_0:
                        {
                            return new Scenario_NS::CStudentBehaviorV2_0(stInParam);
                        }
                        default:
                            break;
                    }
                    break;
                }
                /* 跟踪分析 */
                case Type_E::TRACKER:
                {
                    switch (stInParam.stNeedParam.enVersions)
                    {
                        case Versions_E::V1_0:
                        {
                            return new Scenario_NS::CTrakcerV1_0(stInParam);
                        }
                        default:
                            break;
                    }
                    break;
                }
                /* 人脸识别 */
                case Type_E::FACR_RECT:
                {
                    switch (stInParam.stNeedParam.enVersions)
                    {
                        case Versions_E::V1_0:
                        {
                            return new Scenario_NS::CFaceRecV1_0(stInParam);
                        }
                        default:
                            break;
                    }
                    break;
                }
                /* 表情识别 */
                case Type_E::FACE_EXPRESS:
                {
                    switch (stInParam.stNeedParam.enVersions)
                    {
                        case Versions_E::V1_0:
                        {
                            return new Scenario_NS::CFaceExpressV1_0(stInParam);
                        }
                        case Versions_E::V2_0:
                        {
                            return new Scenario_NS::CFaceExpressV2_0(stInParam);
                        }
                        default:
                            break;
                    }
                    break;
                }
                /* 语音唤醒 */
                case Type_E::VOICE_WAKE_UP:
                {
                    switch (stInParam.stNeedParam.enVersions)
                    {
                        case Versions_E::V1_0:
                        {
                            return new Scenario_NS::CVoiceWakeUpV1_0(stInParam);
                        }
                        default:
                            break;
                    }
                    break;
                }
                /* 黑底白数字识别 */
                case Type_E::NUMBER_OCR:
                {
                    switch (stInParam.stNeedParam.enVersions)
                    {
                        case Versions_E::V1_0:
                        {
                            return new Scenario_NS::CNumberOcrV1_0(stInParam);
                        }
                        default:
                            break;
                    }
                    break;
                }                
                /* 虚拟抠像 */
                case Type_E::VIRTUAL_CUT:
                {
                    switch (stInParam.stNeedParam.enVersions)
                    {
                        case Versions_E::V1_0:
                        {
                            return new Scenario_NS::CVirtualCutoutV1_0(stInParam);
                        }
                        default:
                            break;
                    }
                    break;
                }
                /* 行人区域检测 */
                case Type_E::HUMAN_AREA:
                {
                    switch (stInParam.stNeedParam.enVersions)
                    {
                        case Versions_E::V1_0:
                        {
                            return new Scenario_NS::CHumanAreaDetect(stInParam);
                        }
                        default:
                            break;
                    }
                    break;
                }
            }
            return nullptr;
        }

        static bool release(Scenario_NS::CScenarioBase*& pScenario)
        {
            if (pScenario)
            {
                delete pScenario;
                pScenario = nullptr;
                return true;
            }

            return false;
        }
    };

}    // namespace AiScenario_NS
