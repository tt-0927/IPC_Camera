/**
 * @Author: xushou
 * @Date: 2026-10-28
 * @Description: Group4 行为检测子检测器实现（抽烟/睡岗/玩手机/摔倒）
 */

#include "group4_sub_detector.hpp"

#include <algorithm>
#include <chrono>

#include "common_process.h"
#include "dlog.h"
#include "event_configure.h"
#include "share_data.h"
#include "stream_video.h"

#if CAP_EXHIBITION_OSD_PANEL
#include "osd_panel_result.hpp"
/**
 * @brief   : 展会面板单个人员事件统计结果
 */
struct Group4PersonPanel_S
{
    Common::RectInfo_S stPersonRect;  /* 当前命中事件的父级人员框 */
    float fConfidence = 0.0f;          /* 当前人员命中的最高事件置信度 */
};

/**
 * @brief   : 将 TrackedTarget_S 转换为普通矩形框
 */
static Common::RectInfo_S to_person_rect(const TrackedTarget_S &stResult)
{
    Common::RectInfo_S stRectInfo;
    stRectInfo.nX1 = static_cast<int>(stResult.fX1);
    stRectInfo.nY1 = static_cast<int>(stResult.fY1);
    stRectInfo.nX2 = static_cast<int>(stResult.fX2);
    stRectInfo.nY2 = static_cast<int>(stResult.fY2);
    return stRectInfo;
}

/**
 * @brief   : 判断矩形框是否有效
 */
static bool is_valid_rect(const Common::RectInfo_S &stRectInfo)
{
    return stRectInfo.nX1 < stRectInfo.nX2 && stRectInfo.nY1 < stRectInfo.nY2;
}

/**
 * @brief   : 合并两个矩形框，生成能覆盖二者的外接矩形
 */
static Common::RectInfo_S merge_rect(const Common::RectInfo_S &stLeft, const Common::RectInfo_S &stRight)
{
    if (!is_valid_rect(stLeft))
    {
        return stRight;
    }
    if (!is_valid_rect(stRight))
    {
        return stLeft;
    }

    Common::RectInfo_S stMergedRect;
    stMergedRect.nX1 = std::min(stLeft.nX1, stRight.nX1);
    stMergedRect.nY1 = std::min(stLeft.nY1, stRight.nY1);
    stMergedRect.nX2 = std::max(stLeft.nX2, stRight.nX2);
    stMergedRect.nY2 = std::max(stLeft.nY2, stRight.nY2);
    return stMergedRect;
}

/**
 * @brief   : 构造玩手机/抽烟事件的展会面板条目
 */
static OsdPanel::PanelItem_S build_group4_count_panel_item(Event::Type_E enEventType,
                                                           const std::vector<Group4PersonPanel_S> &vecMatches,
                                                           bool bAlarm)
{
    (void)enEventType;
    OsdPanel::PanelItem_S stItem;
    if (vecMatches.empty())
    {
        return stItem;
    }

    Common::RectInfo_S stMergedRect;
    float fMaxConfidence = 0.0f;
    for (const auto &stMatch : vecMatches)
    {
        stMergedRect = merge_rect(stMergedRect, stMatch.stPersonRect);
        fMaxConfidence = std::max(fMaxConfidence, stMatch.fConfidence);
    }

    stItem.clear();
    stItem.strTitle.clear();
    stItem.bAlarm = bAlarm;
    stItem.bHasRect = is_valid_rect(stMergedRect);
    stItem.stRect = stMergedRect;
    stItem.nSortKey = 1;
    stItem.nPriority = build_exhibition_panel_priority(bAlarm, fMaxConfidence);
    stItem.vecFields = {{"人数", std::to_string(static_cast<int>(vecMatches.size()))}};
    return stItem;
}

/**
 * @brief   : 在玩手机与抽烟两个候选面板中选出当前需要展示的一帧
 * @note    : 报警态优先，其次人数更多，再次默认玩手机优先
 */
static const OsdPanel::PanelFrame_S *select_group4_panel_frame(const OsdPanel::PanelFrame_S &stPhonePanelFrame,
                                                               bool bPhoneAlarm,
                                                               int nPhoneCount,
                                                               const OsdPanel::PanelFrame_S &stSmokingPanelFrame,
                                                               bool bSmokingAlarm,
                                                               int nSmokingCount)
{
    const bool bHasPhonePanel = !stPhonePanelFrame.empty();
    const bool bHasSmokingPanel = !stSmokingPanelFrame.empty();
    if (!bHasPhonePanel && !bHasSmokingPanel)
    {
        return nullptr;
    }
    if (!bHasPhonePanel)
    {
        return &stSmokingPanelFrame;
    }
    if (!bHasSmokingPanel)
    {
        return &stPhonePanelFrame;
    }
    if (bPhoneAlarm != bSmokingAlarm)
    {
        return bPhoneAlarm ? &stPhonePanelFrame : &stSmokingPanelFrame;
    }
    if (nPhoneCount != nSmokingCount)
    {
        return nPhoneCount > nSmokingCount ? &stPhonePanelFrame : &stSmokingPanelFrame;
    }
    return &stPhonePanelFrame;
}
#endif

/* 灵敏度[1,100] → 置信度门槛（高灵敏度 → 低门槛） */
static float sensitivityToConfidence(unsigned int nSensitivity)
{
    const float fConfMax = 0.9f;
    const float fConfMin = 0.1f;
    int nClampedSens = std::clamp(static_cast<int>(nSensitivity), 1, 100);
    float fConfidence = fConfMax - (nClampedSens - 1) * (fConfMax - fConfMin) / (100 - 1);
    return std::clamp(fConfidence, fConfMin, fConfMax);
}

/* 灵敏度[1,100] → 检测持续时间（秒） */
static unsigned int sensitivityToDuration(unsigned int nSensitivity)
{
    const int nMinDuration = 0;
    const int nMaxDuration = 5;
    if (nSensitivity <= 0)
    {
        return nMaxDuration;
    }
    if (nSensitivity >= 100)
    {
        return nMinDuration;
    }
    double fRatio = (100.0 - nSensitivity) / 100.0;
    return static_cast<unsigned int>(nMinDuration + static_cast<int>(fRatio * (nMaxDuration - nMinDuration)));
}

CGroup4SubDetector::CGroup4SubDetector()
{
}

CGroup4SubDetector::~CGroup4SubDetector()
{
}

bool CGroup4SubDetector::init()
{
    Group4Detect_NS::InParam_S stInParam;
    stInParam.strModelPath = "/opt/cam/model/group4.json";
    stInParam.bDebug = false;

    m_pGroup4Handle = std::make_unique<Group4Detect_NS::CGroup4Detect>(stInParam);
    if (!m_pGroup4Handle->init())
    {
        dlog_error("GROUP4_SUB: Group4 检测模型初始化失败 [%s]", stInParam.strModelPath.c_str());
        m_pGroup4Handle.reset();
        return false;
    }

    dlog_info("GROUP4_SUB: Group4 检测模型初始化成功 [%s]", stInParam.strModelPath.c_str());
    return true;
}

void CGroup4SubDetector::updateConfig()
{
    Config_S stNewConfig;

    /* 抽烟 */
    Alarm::SmokingDection_S stSmoking;
    CEventConfigure::instance()->get_configure(stSmoking);
    stNewConfig.bSmokingEnable = stSmoking.bEnable;
    stNewConfig.nSmokingSensitivity = stSmoking.stRule.nSensitivity;

    /* 睡岗 */
    Alarm::SleepOnDutyDetection_S stSleep;
    CEventConfigure::instance()->get_configure(stSleep);
    stNewConfig.bSleepEnable = stSleep.bEnable;
    stNewConfig.nSleepSensitivity = stSleep.stRule.nSensitivity;

    /* 玩手机 */
    Alarm::PhoneUsageDetection_S stPhone;
    CEventConfigure::instance()->get_configure(stPhone);
    stNewConfig.bPhoneEnable = stPhone.bEnable;
    stNewConfig.nPhoneSensitivity = stPhone.stRule.nSensitivity;

    /* 摔倒 */
    Alarm::TripDetection_S stTrip;
    CEventConfigure::instance()->get_configure(stTrip);
    stNewConfig.bTripEnable = stTrip.bEnable;
    stNewConfig.nTripSensitivity = stTrip.stRule.nSensitivity;

    {
        std::lock_guard<std::mutex> lock(m_mutexConfig);
        m_stConfig = stNewConfig;
    }

    dlog_info("GROUP4_SUB: 配置更新 抽烟[%d/%u] 睡岗[%d/%u] 玩手机[%d/%u] 摔倒[%d/%u]",
              stNewConfig.bSmokingEnable, stNewConfig.nSmokingSensitivity,
              stNewConfig.bSleepEnable, stNewConfig.nSleepSensitivity,
              stNewConfig.bPhoneEnable, stNewConfig.nPhoneSensitivity,
              stNewConfig.bTripEnable, stNewConfig.nTripSensitivity);
}

int CGroup4SubDetector::process(const cv::Mat &rgbMat,
                                const std::vector<TrackedTarget_S> &vecPersonTargets)
{
    if (rgbMat.empty() || rgbMat.type() != CV_8UC3)
    {
        return -2;
    }

    /* 模型自动加载 */
    if (!m_pGroup4Handle && !init())
    {
        return -1;
    }

    /* 配置快照 */
    Config_S stConfig;
    {
        std::lock_guard<std::mutex> lock(m_mutexConfig);
        stConfig = m_stConfig;
    }

    /* 四个事件全关就不推理 */
    if (!stConfig.bSmokingEnable && !stConfig.bSleepEnable &&
        !stConfig.bPhoneEnable && !stConfig.bTripEnable)
    {
        return 0;
    }

    /* 构造 Group4 输入 */
    Group4Detect_NS::InData_S stInData;
    stInData.inMat = rgbMat;

    /* 睡岗识别（整帧检测） */
    if (stConfig.bSleepEnable)
    {
        stInData.stParam.stSleepParam.bEnable = true;
        stInData.stParam.stSleepParam.fConfidence = sensitivityToConfidence(stConfig.nSleepSensitivity);
        stInData.stParam.stSleepParam.nDetectDuration = sensitivityToDuration(stConfig.nSleepSensitivity);
    }

    /* 摔倒识别（整帧检测） */
    if (stConfig.bTripEnable)
    {
        stInData.stParam.stFallParam.bEnable = true;
        stInData.stParam.stFallParam.fConfidence = sensitivityToConfidence(stConfig.nTripSensitivity);
        stInData.stParam.stFallParam.nDetectDuration = sensitivityToDuration(stConfig.nTripSensitivity);
    }

    /* 抽烟/玩手机：需要人员框，由 Group4 模型内部裁剪推理 */
    if (stConfig.bSmokingEnable)
    {
        stInData.stParam.stCigaretteDetectParam.bEnable = true;
        stInData.stParam.stCigaretteDetectParam.fConfidence = sensitivityToConfidence(stConfig.nSmokingSensitivity);
        stInData.stParam.stCigaretteDetectParam.nDetectDuration = sensitivityToDuration(stConfig.nSmokingSensitivity);
    }

    if (stConfig.bPhoneEnable)
    {
        stInData.stParam.stPhoneParam.bEnable = true;
        stInData.stParam.stPhoneParam.fConfidence = sensitivityToConfidence(stConfig.nPhoneSensitivity);
        stInData.stParam.stPhoneParam.nDetectDuration = sensitivityToDuration(stConfig.nPhoneSensitivity);
    }

    if (stConfig.bSmokingEnable || stConfig.bPhoneEnable)
    {
        for (const auto &stPerson : vecPersonTargets)
        {
            Group4Detect_NS::Result_S stPersonArea;
            stPersonArea.fX1 = stPerson.fX1;
            stPersonArea.fY1 = stPerson.fY1;
            stPersonArea.fX2 = stPerson.fX2;
            stPersonArea.fY2 = stPerson.fY2;
            stInData.vecArea.push_back(stPersonArea);
        }
    }

    /* 推理 */
    std::vector<Group4Detect_NS::Result_S> vecAllResult;
    Group4Detect_NS::OutData_S stOutData;
    m_pGroup4Handle->process(stInData, vecAllResult, &stOutData);

    /* 动态分析框输出到 OSD */
    if (!vecAllResult.empty())
    {
        std::vector<Common::RectInfo_S> vstRectInfo;
        for (const auto &stResult : vecAllResult)
        {
            Common::RectInfo_S stRectInfo;
            stRectInfo.nX1 = static_cast<int>(stResult.fX1);
            stRectInfo.nY1 = static_cast<int>(stResult.fY1);
            stRectInfo.nX2 = static_cast<int>(stResult.fX2);
            stRectInfo.nY2 = static_cast<int>(stResult.fY2);
            vstRectInfo.push_back(stRectInfo);
        }
        send_detectionResult_to_osd(rgbMat.cols, rgbMat.rows, vstRectInfo);
    }

#if CAP_EXHIBITION_OSD_PANEL
    /* V3 抽烟/玩手机输出人员大框，按类别和大框中心点匹配回原人员框统计 */
    std::vector<Group4PersonPanel_S> vecPhonePanelMatches;
    std::vector<Group4PersonPanel_S> vecSmokingPanelMatches;

    if ((stOutData.bPhone || stOutData.bCigarette) && !vecPersonTargets.empty())
    {
        std::vector<bool> vecPhonePersonCounted(vecPersonTargets.size(), false);
        std::vector<bool> vecSmokingPersonCounted(vecPersonTargets.size(), false);

        for (const auto &stResult : vecAllResult)
        {
            const bool bPhoneResult =
                stOutData.bPhone &&
                stResult.nClassId == Group4Detect_NS::CGroup4Detect::PHONE &&
                stResult.fBoxConfidence > stInData.stParam.stPhoneParam.fConfidence;
            const bool bSmokingResult =
                stOutData.bCigarette &&
                stResult.nClassId == Group4Detect_NS::CGroup4Detect::CIGARETTE &&
                stResult.fBoxConfidence > stInData.stParam.stCigaretteDetectParam.fConfidence;

            if (!bPhoneResult && !bSmokingResult)
            {
                continue;
            }

            /* 人员大框中心点 */
            const float fCenterX = (stResult.fX1 + stResult.fX2) / 2.0f;
            const float fCenterY = (stResult.fY1 + stResult.fY2) / 2.0f;

            for (size_t i = 0; i < vecPersonTargets.size(); i++)
            {
                if ((bPhoneResult && vecPhonePersonCounted[i]) ||
                    (bSmokingResult && vecSmokingPersonCounted[i]))
                {
                    continue;
                }

                const auto &stPerson = vecPersonTargets[i];
                if (fCenterX >= stPerson.fX1 && fCenterX <= stPerson.fX2 &&
                    fCenterY >= stPerson.fY1 && fCenterY <= stPerson.fY2)
                {
                    if (bPhoneResult)
                    {
                        vecPhonePersonCounted[i] = true;
                        vecPhonePanelMatches.push_back({to_person_rect(stPerson), stResult.fBoxConfidence});
                    }
                    if (bSmokingResult)
                    {
                        vecSmokingPersonCounted[i] = true;
                        vecSmokingPanelMatches.push_back({to_person_rect(stPerson), stResult.fBoxConfidence});
                    }
                    break;
                }
            }
        }

        /* 玩手机事件的展会面板帧 */
        OsdPanel::PanelFrame_S stPhonePanelFrame;
        if (prepare_exhibition_panel_frame(&stPhonePanelFrame, Event::Type_E::PHONE_USAGE, rgbMat.cols, rgbMat.rows))
        {
            upsert_exhibition_panel_item(&stPhonePanelFrame,
                                         build_group4_count_panel_item(Event::Type_E::PHONE_USAGE,
                                                                       vecPhonePanelMatches,
                                                                       stOutData.bPhone));
        }

        /* 抽烟事件的展会面板帧 */
        OsdPanel::PanelFrame_S stSmokingPanelFrame;
        if (prepare_exhibition_panel_frame(&stSmokingPanelFrame, Event::Type_E::SMOKING, rgbMat.cols, rgbMat.rows))
        {
            upsert_exhibition_panel_item(&stSmokingPanelFrame,
                                         build_group4_count_panel_item(Event::Type_E::SMOKING,
                                                                       vecSmokingPanelMatches,
                                                                       stOutData.bCigarette));
        }

        /* 选择并发送面板帧 */
        const OsdPanel::PanelFrame_S *pSelectedPanelFrame = select_group4_panel_frame(
            stPhonePanelFrame,
            stOutData.bPhone,
            static_cast<int>(vecPhonePanelMatches.size()),
            stSmokingPanelFrame,
            stOutData.bCigarette,
            static_cast<int>(vecSmokingPanelMatches.size()));

        if (pSelectedPanelFrame)
        {
            send_panelResult_to_osd(*pSelectedPanelFrame);
        }
    }
#endif

    /* 四个事件各自驱动状态机 触发时携带人员大框与大小图 */
    auto dispatchGroup4Event = [&](CAlarmStateMachine &stateMachine, bool bEnable, bool bTriggered,
                                   Event::Type enType, int nClassId) {
        const bool bAlarm = bEnable && bTriggered;

#ifdef ENABLE_TVSDK_SRC
        EventTriggerContext_S stContext;
        stContext.enEventType = enType;
        stContext.nChnId = 0;
        stContext.llTimestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();

        if (bAlarm)
        {
            /* 首个匹配类别的人员大框 */
            const Group4Detect_NS::Result_S *pstResult = nullptr;
            for (const auto &stResult : vecAllResult)
            {
                if (stResult.nClassId == nClassId)
                {
                    pstResult = &stResult;
                    break;
                }
            }

            if (pstResult != nullptr)
            {
                stContext.nObjectType = 1; /* 人 */
                stContext.fConfidence = pstResult->fBoxConfidence;
                stContext.nLeft   = static_cast<int>(pstResult->fX1);
                stContext.nTop    = static_cast<int>(pstResult->fY1);
                stContext.nRight  = static_cast<int>(pstResult->fX2);
                stContext.nBottom = static_cast<int>(pstResult->fY2);

                /* 小图按人员大框裁剪 裁剪异常仅缺小图不影响上报 */
                const int nX1 = std::max(0, stContext.nLeft);
                const int nY1 = std::max(0, stContext.nTop);
                const int nX2 = std::min(rgbMat.cols, stContext.nRight);
                const int nY2 = std::min(rgbMat.rows, stContext.nBottom);
                if (nX2 > nX1 && nY2 > nY1)
                {
                    cv::Mat targetMat = rgbMat(cv::Rect(nX1, nY1, nX2 - nX1, nY2 - nY1)).clone();
                    encode_mat_to_tvsdk_image(targetMat, stContext.stTargetImage, JPEG_QUALITY_TARGET);
                }
            }

            encode_mat_to_tvsdk_image(rgbMat, stContext.stPanoramaImage, JPEG_QUALITY_PANORAMA);
        }

        stateMachine.handleAlarmState(bAlarm, stContext);
#else
        (void)nClassId;
        stateMachine.handleAlarmState(bAlarm, enType);
#endif
    };

    dispatchGroup4Event(m_smokingStateMachine, stConfig.bSmokingEnable, stOutData.bCigarette,
                        Event::Type::SMOKING, Group4Detect_NS::CGroup4Detect::CIGARETTE);
    dispatchGroup4Event(m_sleepOnDutyStateMachine, stConfig.bSleepEnable, stOutData.bSleep,
                        Event::Type::SLEEP_ON_DUTY, Group4Detect_NS::CGroup4Detect::SLEEP);
    dispatchGroup4Event(m_phoneUsageStateMachine, stConfig.bPhoneEnable, stOutData.bPhone,
                        Event::Type::PHONE_USAGE, Group4Detect_NS::CGroup4Detect::PHONE);
    dispatchGroup4Event(m_tripStateMachine, stConfig.bTripEnable, stOutData.bFall,
                        Event::Type::PERSON_TRIP, Group4Detect_NS::CGroup4Detect::FALL);

    return 0;
}
