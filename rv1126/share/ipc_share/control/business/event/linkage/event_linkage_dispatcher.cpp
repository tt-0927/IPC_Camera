/**
 * @FilePath     : event_linkage_dispatcher.cpp
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2026-04-15 16:29:58
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-04-16 14:06:34
 * @Description  : 事件联动调度器实现
 */

#include "event_linkage_dispatcher.h"

#include "time_utils.h"
#include "dlog.h"
#include "IpcRet.h"

EventLinkageDispatcher::EventLinkageDispatcher(EventLinkageDirectAction &stDirectAction, EventLinkageWorker &stWorker)
    : m_directAction(stDirectAction), m_worker(stWorker)
{
}

int EventLinkageDispatcher::dispatch(const ResolvedLinkagePlan_S &stPlan, const Event::EventState_S &stEventState)
{

        /*
     * 人脸比对以抓拍结果为输入，两者同时启用时属于同一条事件链。
     * 抓拍仍执行存储、推送等直接动作，但声光统一交给最终的比对成功/失败事件，
     * 从源头避免两组异步声光任务竞争。仅开启人脸抓拍时不受影响。
     */
     const bool bFaceCaptureDelegatesSoundAndLight =
     stPlan.stContext.enEventType == Event::Type_E::FACE_CAPTURE &&
     stPlan.stAlgorithmConfig.nEnFaceCompare;

    /* 先处理录像和抓图，这两类动作需要尽快和当前事件状态对齐 */
    if (stPlan.bUploadSdCard)
    {
        m_directAction.deal_record(stPlan, stEventState);
        if (stPlan.stContext.enEventType != Event::Type_E::FACE_CAPTURE &&
            stPlan.stContext.enEventType != Event::Type_E::FACE_COMPARE_SUCCESS && 
            stPlan.stContext.enEventType != Event::Type_E::FACE_COMPARE_FAIL)
        {
            m_directAction.deal_capture_image(stPlan);
        }
    }

    if (stPlan.stContext.bEventEnded)
    {
        /* 结束事件不再派发新的异步任务，避免重复发送邮件或重复声光告警 */
        return OK;
    }

    /* 上传中心依赖事件开始态信息，在异步任务之前直接发送 */
    if (stPlan.bUploadToCenter)
    {
        m_directAction.deal_upload(stPlan);
    }

    /* ONVIF日志通知固定走异步链路，便于与其他可抢占动作统一调度 */
    m_worker.pushTask(build_task(stPlan, LinkageType_E::LOG));

    if ((stPlan.bSendEmail) && (stPlan.stContext.enEventType != Event::Type_E::FACE_COMPARE_SUCCESS &&
                                stPlan.stContext.enEventType != Event::Type_E::FACE_COMPARE_FAIL))
    {
        m_worker.pushTask(build_task(stPlan, LinkageType_E::EMAIL));
    }

    // if (stPlan.stAlgorithmConfig.nEnFlashAlarm && stPlan.bFlashingLightAlarm)
    if (!bFaceCaptureDelegatesSoundAndLight && stPlan.stAlgorithmConfig.nEnFlashAlarm && stPlan.bFlashingLightAlarm)
    {
        m_worker.pushTask(build_task(stPlan, LinkageType_E::FLASHING_LIGHT));
    }

    // if (stPlan.stAlgorithmConfig.nEnAudioAlarm && stPlan.bSound)
    if (!bFaceCaptureDelegatesSoundAndLight && stPlan.stAlgorithmConfig.nEnAudioAlarm && stPlan.bSound)
    {
        m_worker.pushTask(build_task(stPlan, LinkageType_E::SOUND));
    }

    if (stPlan.stAlgorithmConfig.nEnAlarmOutput && !stPlan.stLinkageList.alarmOutput.empty())
    {
        /* 报警输出除了通用任务信息，还需要携带目标IO编号列表 */
        LinkageTask_S stTask = build_task(stPlan, LinkageType_E::ALARM_IO);
        stTask.vecAlarmOutputNum = stPlan.stLinkageList.alarmOutput;
        m_worker.pushTask(stTask);
    }

    return OK;
}

LinkageTask_S EventLinkageDispatcher::build_task(const ResolvedLinkagePlan_S &stPlan, LinkageType_E enLinkageType) const
{
    LinkageTask_S stTask;
    stTask.stContext = stPlan.stContext;
    stTask.stEventInfo = stPlan.stEventInfo;
    stTask.enLinkageType = enLinkageType;
    stTask.nPriority = get_linkage_task_priority(stPlan.stContext.enEventType, enLinkageType);
    /* 优先沿用事件触发时间，缺失时再补当前时间，保证队列中的先后关系稳定 */
    stTask.llTimestamp = stPlan.stContext.llTimestamp > 0 ? stPlan.stContext.llTimestamp : TimeUtils_NS::get_currentTimestampMs();
    stTask.bUploadSdCard = stPlan.bUploadSdCard;
    return stTask;
}
