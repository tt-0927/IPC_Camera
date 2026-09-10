/*
 * @Author       : EasonLu
 * @Date         : 2025-04-21 09:16:51
 * @LastEditors  : EasonLu
 * @LastEditTime : 2025-04-21 09:57:28
 * @FilePath     : QueryEvent.h
 * @Description  : 查询事件
 */
#pragma once
#include "BaseEvent.h"

namespace SIP
{
    class QueryEvent : public BaseEvent
    {
    public:
        QueryEvent() {};
        virtual bool Handle(const SipEvent::Ptr &e) override;

    protected:
        bool HandleDeviceInfo(const SipEvent::Ptr &e);
        bool HandleCatalog(const SipEvent::Ptr &e);
        bool HandleRecordInfo(const SipEvent::Ptr &e);
        bool HandleDeviceStatus(const SipEvent::Ptr &e);
        bool HandleConfigDownload(const SipEvent::Ptr &e);
        bool HandleCruiseTrackQuery(const SipEvent::Ptr &e);
        bool HandleCruiseTrackListQuery(const SipEvent::Ptr &e);
        bool HandlePresetQuery(const SipEvent::Ptr &e);
        bool HandleHomePositionQuery(const SipEvent::Ptr &e);
        bool HandlePTZPosition(const SipEvent::Ptr &e);

    private:
        //设备配置
        bool HandleBasicParamRequest(const SipEvent::Ptr &e);
        bool HandleVideoParOptRequest(const SipEvent::Ptr &e);
        bool HandleSVACEnConfRequest(const SipEvent::Ptr &e);
        bool HandleSVACDeConfRequest(const SipEvent::Ptr &e);
        bool HandleVideoParAttrRequst(const SipEvent::Ptr &e);
        bool HandleVideoRecPlanRequst(const SipEvent::Ptr &e);
        bool HandleVideoAlarmRequest(const SipEvent::Ptr &e);
        bool HandlePictureMaskRequst(const SipEvent::Ptr &e);
        bool HandleFrameMirrorRequst(const SipEvent::Ptr &e);
        bool HandleAlarmReportRequst(const SipEvent::Ptr &e);
        bool HandleOSDConfigRequest(const SipEvent::Ptr &e);
        bool HandleSnapShotConfig(const SipEvent::Ptr &e);

        bool ResponeControlResult(const SipEvent::Ptr &e, uint64_t nSN, std::string strCmdType, std::string strDeviceID, std::string strResult);

public:
        bool SetSnapShotMap(std::map<std::string, SnapShotInfo_S> & map);  //add by longll 
protected:
        std::map<std::string, SnapShotInfo_S> *m_pSnapSnapShot =  nullptr;
    };
}