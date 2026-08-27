/*** 
 * @FilePath     : ConfigEvent.h
 * @Author       : cyc
 * @Date         : 2025-07-02 09:33:30
 * @LastEditors  : cyc
 * @LastEditTime : 2025-07-02 09:34:16
 * @Description  : 配置事件
 */
#pragma once
#include "BaseEvent.h"
#include "SipType.h"
#include "SipModule.h"

namespace SIP
{
    class ConfigEvent : public BaseEvent
    {
    public:
        virtual bool Handle(const SipEvent::Ptr &e) override;
    protected:
        bool HandleBasicParam(const SipEvent::Ptr &e);
        bool HandleVideoParamOpt(const SipEvent::Ptr &e);
        bool HandleSVACEncodeConfig(const SipEvent::Ptr &e);
        bool HandleSVACDncodeConfig(const SipEvent::Ptr &e);
        bool HandleVideoParamAttribute(const SipEvent::Ptr &e);
        bool HandleVideoRecordPlan(const SipEvent::Ptr &e);
        bool HandleVideoAlarmRecord(const SipEvent::Ptr &e);
        bool HandlePictureMask(const SipEvent::Ptr &e);
        bool HandleFrameMirror(const SipEvent::Ptr &e);
        bool HandleAlarmReport(const SipEvent::Ptr &e);
        bool HandleOSDConfig(const SipEvent::Ptr &e);
        bool HandleSnapShotConfig(const SipEvent::Ptr &e);
    };
} // namespace SIP
