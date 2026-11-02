/*
 * @Author: zhengxh
 * @Date: 2026-10-22
 * @LastEditors: zhengxh
 * @LastEditTime: 2026-10-22
 * @Description: 后厨穿戴规范子检测器实现
 */

#include "kitchen_clothing_sub_detector.hpp"

#include <algorithm>

#include "event_configure.h" /* CEventConfigure */
#include "kitchen_detect.hpp"
#include "dlog.h"

/* 灵敏度[1,100] 反向映射置信度门槛（同 region_stay_processor） */
static float sensitivityToConfidence(unsigned int nSensitivity)
{
    const float fConfMax = 0.9f;
    const float fConfMin = 0.1f;
    int nClampedSens = std::clamp(static_cast<int>(nSensitivity), 1, 100);
    float fConfidence = fConfMax - (nClampedSens - 1) * (fConfMax - fConfMin) / (100 - 1);
    return std::clamp(fConfidence, fConfMin, fConfMax);
}

/* 点是否在多边形内（射线法，同 region_stay_processor） */
static bool isPointInRegion(float fX, float fY, const Alarm::Region_S &stRegion)
{
    size_t nCount = std::min(static_cast<size_t>(stRegion.nPointNum), stRegion.aPoint.size());
    if (nCount < 3)
    {
        return false;
    }

    bool bInside = false;
    for (size_t i = 0, j = nCount - 1; i < nCount; j = i, ++i)
    {
        const Common::PosF_S &stCur = stRegion.aPoint[i];
        const Common::PosF_S &stPre = stRegion.aPoint[j];

        if (((stCur.fY > fY) != (stPre.fY > fY)) &&
            (fX < (stPre.fX - stCur.fX) * (fY - stCur.fY) / (stPre.fY - stCur.fY) + stCur.fX))
        {
            bInside = !bInside;
        }
    }

    return bInside;
}

CKitchenClothingSubDetector::CKitchenClothingSubDetector()
{
}

CKitchenClothingSubDetector::~CKitchenClothingSubDetector()
{
}

bool CKitchenClothingSubDetector::init()
{
    m_pKitchenComponent = std::make_unique<CKitchenDetector>(KITCHEN_DETECT_MODEL_CONFIG_PATH);
    if (!m_pKitchenComponent->init())
    {
        dlog_error("KITCHEN_CLOTHING_SUB: 厨房检测模型初始化失败 [%s]", KITCHEN_DETECT_MODEL_CONFIG_PATH);
        m_pKitchenComponent.reset();
        return false;
    }

    dlog_info("KITCHEN_CLOTHING_SUB: 厨房检测模型初始化成功 [%s]", KITCHEN_DETECT_MODEL_CONFIG_PATH);
    return true;
}

void CKitchenClothingSubDetector::updateConfig()
{
    Alarm::KitchenClothingCompliance_S stInfo;
    CEventConfigure::instance()->get_configure(stInfo);

    {
        std::lock_guard<std::mutex> lock(m_mutexRule);
        m_stConfig = stInfo;
    }

    dlog_info("KITCHEN_CLOTHING_SUB: 配置更新 灵敏度[%u] 区域点数[%d]",
              stInfo.stRule.nSensitivity, stInfo.stRule.stRegion.nPointNum);
}

int CKitchenClothingSubDetector::process(const cv::Mat &rgbMat,
                                         const std::vector<TrackedTarget_S> &vecPersonTargets)
{
    if (rgbMat.empty() || rgbMat.type() != CV_8UC3)
    {
        return -2;
    }

    /* 模型自动加载：未就绪时持续重试 */
    if (!m_pKitchenComponent && !init())
    {
        return -1;
    }

    /* 配置快照（updateConfig 可能来自配置线程） */
    Alarm::KitchenClothingCompliance_S stConfig;
    {
        std::lock_guard<std::mutex> lock(m_mutexRule);
        stConfig = m_stConfig;
    }

    /* 灵敏度反向映射置信度门槛：人员目标与穿戴属性共用 */
    const float fConfTh = sensitivityToConfidence(stConfig.stRule.nSensitivity);

    /* 区域非空时仅判定区域内人员（底边中点入区，同区域停留事件） */
    const Alarm::Region_S &stRegion = stConfig.stRule.stRegion;
    const bool bHasRegion =
        std::min(static_cast<size_t>(stRegion.nPointNum), stRegion.aPoint.size()) >= 3;

    bool bAlarm = false;

    for (const auto &stPerson : vecPersonTargets)
    {
        if (stPerson.fConfidence < fConfTh)
        {
            continue;
        }

        if (bHasRegion && !isPointInRegion((stPerson.fX1 + stPerson.fX2) / 2.0f, stPerson.fY2, stRegion))
        {
            continue;
        }

        /* 人员框取整并夹帧边界后裁剪目标小图 */
        int nX1 = std::clamp(static_cast<int>(stPerson.fX1), 0, rgbMat.cols - 1);
        int nY1 = std::clamp(static_cast<int>(stPerson.fY1), 0, rgbMat.rows - 1);
        int nX2 = std::clamp(static_cast<int>(stPerson.fX2), 0, rgbMat.cols - 1);
        int nY2 = std::clamp(static_cast<int>(stPerson.fY2), 0, rgbMat.rows - 1);
        if (nX2 - nX1 < 2 || nY2 - nY1 < 2)
        {
            continue;
        }

        CKitchenDetector::InData_S stKitchenIn;
        stKitchenIn.inMat = rgbMat(cv::Rect(nX1, nY1, nX2 - nX1, nY2 - nY1));
        std::vector<DetectedTarget_S> vecKitchenResults;
        if (m_pKitchenComponent->process(stKitchenIn, vecKitchenResults) != 0)
        {
            continue;
        }

        /* 穿戴合规三要素：厨师工作服 + 厨师帽 + 口罩（with_mask），均需过置信度门槛 */
        bool bUniform = false;
        bool bHat = false;
        bool bMask = false;
        for (const auto &stResult : vecKitchenResults)
        {
            if (stResult.fConfidence < fConfTh)
            {
                continue;
            }

            if (stResult.nLabel == KITCHEN_CLASS_CHEF_UNIFORM)
            {
                bUniform = true;
            }
            else if (stResult.nLabel == KITCHEN_CLASS_CHEF_HAT)
            {
                bHat = true;
            }
            else if (stResult.nLabel == KITCHEN_CLASS_WITH_MASK)
            {
                bMask = true;
            }
        }

        if (!bUniform || !bHat || !bMask)
        {
            bAlarm = true;
            break;
        }
    }

    /* 命中即驱动事件开始/结束上报（true/false + 事件类型，抓图等富化后续迭代） */
    m_alarmStateMachine.handleAlarmState(bAlarm, Event::Type::KITCHEN_CLOTHING_COMPLIANCE);
    return 0;
}
