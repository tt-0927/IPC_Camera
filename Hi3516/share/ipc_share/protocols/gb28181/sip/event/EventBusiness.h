/*
 * @Author       : EasonLu
 * @Date         : 2025-02-13 14:58:16
 * @LastEditors: error: error: git config user.name & please set dead value or install git && error: git config user.email & please set dead value or install git & please set dead value or install git
 * @LastEditTime: 2025-05-10 13:38:00
 * @FilePath     : EventBusiness.h
 * @Description  : 各类事件的业务处理
 */
#pragma once
#include "BaseEvent.h"
#include "GbDefine.h"
#include "pugixml.hpp"
#include "BlockQueue.hpp"
#include "BlockThread.hpp"
#include <map>
#include <mutex>
namespace SIP
{
    class SipClient;
    /* NOTE 以一级指令为类，二级指令为构造参数，用作事件处理 */
    class RegisterEvent : public BaseEvent
    {
    public:
        int HandleIncomingRequest(const SipEvent::Ptr &e);

        void _response_register_401unauthorized(const SipEvent::Ptr &e);

        void _response_register_302moveTemporarily(const SipEvent::Ptr &e);

    private:
    };

    class MessageEvent : public BaseEvent
    {
    public:
        MessageEvent();
        ~MessageEvent();
    public:
        int HandleIncomingRequest(const SipEvent::Ptr &e);

        int HandleResponseSuccess(const SipEvent::Ptr &e);
        int HandleResponseFailure(const SipEvent::Ptr &e);

    public:
        int NotifySnapShotFinish(GB28181::UploadSnapShotFiniInfo_S &Info);
        int SendUploadSnapShotFinish(GB28181::UploadSnapShotFiniInfo_S &Info);

    public:
        void thrSendSnapShot();
        
        std::map<std::string, SnapShotInfo_S> m_mapSnapShotFini;
        std::atomic_bool m_bThrRun = false;
        std::thread *m_pThrSnapShot = nullptr;
        BlockQueue<GB28181::UploadSnapShotFiniInfo_S> m_queSnapShot;
    };




    class HeartbeatEvent : public BaseEvent
    {
    public:
        virtual bool Handle(const SipEvent::Ptr &e) override;
    };

    class CatalogEvent : public BaseEvent
    {
    public:
        virtual bool Handle(const SipEvent::Ptr &e) override;
    };

    class DeviceInfoEvent : public BaseEvent
    {
    public:
        virtual bool Handle(const SipEvent::Ptr &e) override;
    };

    class PresetQueryEvent : public BaseEvent
    {
    public:
        virtual bool Handle(const SipEvent::Ptr &e) override;
    };

    class AlarmEvent : public BaseEvent
    {
    public:
        virtual bool Handle(const SipEvent::Ptr &e) override;
    };

    class DeviceCtrlEvent : public BaseEvent
    {
    public:
        virtual bool Handle(const SipEvent::Ptr &e) override;
    protected:
        bool HandleDeviceControl(const SipEvent::Ptr &e);
        bool HandleDeviceConfig(const SipEvent::Ptr &e);

    private:
        bool HandleGuardCmd(const SipEvent::Ptr &e);
        bool HandlePTZ(const SipEvent::Ptr &e);
        bool HandleTeleBoot(const SipEvent::Ptr &e);
        bool HandleIFrameCmd(const SipEvent::Ptr &e);
        bool HandleDragZoomIn(const SipEvent::Ptr &e);
        bool HandleDragZoomOut(const SipEvent::Ptr &e);
        bool HandleDevicePTZCtrl(const SipEvent::Ptr &e);
        bool HandleRecordCmd(const SipEvent::Ptr &e);
        bool HandleAlarmCmd(const SipEvent::Ptr &e);
        bool HandleHomePosition(const SipEvent::Ptr &e);
        bool HandleDeviceUpgrade(const SipEvent::Ptr &e);
        bool HandleTargetTrack(const SipEvent::Ptr &e);


        //设备配置
        bool HandleBasicControl(const SipEvent::Ptr &e);
        bool HandleVideoOptControl(const SipEvent::Ptr &e);
        bool HandleSVACEncodeControl(const SipEvent::Ptr &e);
        bool HandleSVACDncodeControl(const SipEvent::Ptr &e);
        bool HandleVideoAttriControl(const SipEvent::Ptr &e);
        bool HandleRecordPlanControl(const SipEvent::Ptr &e);
        bool HandleAlarmRecordControl(const SipEvent::Ptr &e);
        bool HandlePictureMaskControl(const SipEvent::Ptr &e);
        bool HandleFrameMirrorControl(const SipEvent::Ptr &e);
        bool HandleAlarmReportControl(const SipEvent::Ptr &e);
        bool HandleOSDConfigControl(const SipEvent::Ptr &e);
        bool HandleSnapShotControl(const SipEvent::Ptr &e);

        bool ResponeControlResult(const SipEvent::Ptr &e, uint64_t nSN, std::string strCmdType, std::string strDeviceID, std::string strResult);

    public:
        //设备控制应答
        bool HandleRespone(const SipEvent::Ptr &e);
        bool HandleControlRespone(const SipEvent::Ptr &e);
        bool HandleConfigRespone(const SipEvent::Ptr &e);
    };

    //巡航轨迹
    class CruiseTrackEvent : public BaseEvent
    {
    public:
        virtual bool Handle(const SipEvent::Ptr &e) override;
    };

    //巡航轨迹列表
    class CruiseTrackListEvent : public BaseEvent
    {
    public:
        virtual bool Handle(const SipEvent::Ptr &e) override;
    };

    //语音广播
    class BroadcastEvent : public BaseEvent
    {
    public:
        virtual bool HandleResponse(const SipEvent::Ptr &e);
        virtual bool HandleRequest(const SipEvent::Ptr &e);
    };

    //看守位信息查询
    class HomePositionQueryEvent : public BaseEvent
    {
    public:
        virtual bool Handle(const SipEvent::Ptr &e) override;
    };

    //PTZ精确状态查询
    class PTZPositionEvent : public BaseEvent
    {
    public:
        virtual bool Handle(const SipEvent::Ptr &e) override;
    };

    //图像抓拍传输完成通知
    class UploadSnapShotFinishEvent : public BaseEvent
    {
    public:
        virtual bool Handle(const SipEvent::Ptr &e) override;
    };
}