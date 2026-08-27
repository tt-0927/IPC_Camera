/*
 * @Author       : EasonLu
 * @Date         : 2025-02-14 16:55:10
 * @LastEditors  : EasonLu
 * @LastEditTime : 2025-02-27 15:53:38
 * @FilePath     : MethodRequest.h
 * @Description  : 各种方法的请求合集
 */
#pragma once
#include "GbDefine.h"
#include "MessageRequest.h"
#include "PtzType.h"
#include "SipType.h"
namespace SIP
{

    class CatalogRequest : public MessageRequest
    {
    public:
        typedef std::shared_ptr<CatalogRequest> Ptr;
        CatalogRequest(eXosip_t *ctx, Device::Ptr device)
            : MessageRequest(ctx, device, REQUEST_MESSAGE_TYPE::QUERY_CATALOG)
        {
        }

    public:
        virtual const std::string make_manscdp_body();
    };

    class DeviceInfoRequest : public MessageRequest
    {
    public:
        typedef std::shared_ptr<DeviceInfoRequest> Ptr;
        DeviceInfoRequest(eXosip_t *ctx, Device::Ptr device)
            : MessageRequest(ctx, device, REQUEST_MESSAGE_TYPE::QUERY_DEVICEINFO)
        {
        }

    public:
        virtual const std::string make_manscdp_body();
    };

    class PresetRequest : public MessageRequest
    {
    public:
        typedef std::shared_ptr<PresetRequest> Ptr;

        PresetRequest(eXosip_t *ctx, Device::Ptr device, const std::string &channel_id)
            : MessageRequest(ctx, device, REQUEST_MESSAGE_TYPE::DEVICE_QUERY_PRESET), _channel_id(channel_id) {

              };
        virtual const std::string make_manscdp_body();

        void InsertPreset(const std::string &preset_id, const std::string &preset_name);

        const std::vector<std::pair<std::string, std::string>> GetPresetList();

    private:
        std::string _channel_id;

        std::vector<std::pair<std::string, std::string>> _presets;
    };

    class PresetCtlRequest : public MessageRequest
    {
    public:
        typedef std::shared_ptr<PresetCtlRequest> Ptr;

        PresetCtlRequest(eXosip_t *ctx, Device::Ptr device, const std::string &channel_id, int byte4, int byte5, int byte6, int byte7)
            : MessageRequest(ctx, device, REQUEST_MESSAGE_TYPE::DEVICE_CONTROL_PRESET), _channel_id(channel_id), _byte4(byte4), _byte5(byte5), _byte6(byte6), _byte7(byte7) {
              };
        virtual const std::string make_manscdp_body();

    private:
        std::string _channel_id;
        int _byte4 = 0;
        int _byte5 = 0;
        int _byte6 = 0;
        int _byte7 = 0;
    };

    class PtzCtlRequest : public MessageRequest
    {
    public:
        typedef std::shared_ptr<PtzCtlRequest> Ptr;

        PtzCtlRequest(eXosip_t *ctx, Device::Ptr device,
                      const std::string &channel_id,
                      PtzCommand_E leftRight,
                      PtzCommand_E upDown,
                      PtzCommand_E inOut,
                      int moveSpeed,
                      int zoomSpeed)
            : MessageRequest(ctx, device, REQUEST_MESSAGE_TYPE::DEVICE_CONTROL_PTZ), _channel_id(channel_id), _leftRight(leftRight), _upDown(upDown), _inOut(inOut), _moveSpeed(moveSpeed), _zoomSpeed(zoomSpeed)
        {
        }

        virtual const std::string make_manscdp_body() override;

        virtual int HandleResponse(int statcode) override;

    private:
        std::string _channel_id;
        PtzCommand_E _leftRight = PtzCommand_E::NONE;
        PtzCommand_E _upDown = PtzCommand_E::NONE;
        PtzCommand_E _inOut = PtzCommand_E::NONE;
        int _moveSpeed = 0;
        int _zoomSpeed = 0;
    };

    class LensCtlRequest : public MessageRequest
    {
    public:
        typedef std::shared_ptr<LensCtlRequest> Ptr;

        LensCtlRequest(eXosip_t *ctx, Device::Ptr device,
                       const std::string &channel_id,
                       PtzCommand_E iris,
                       PtzCommand_E focus,
                       int iris_speed,
                       int focus_speed)
            : MessageRequest(ctx, device, REQUEST_MESSAGE_TYPE::DEVICE_CONTROL_PTZ), _channel_id(channel_id), _iris(iris), _focus(focus), _iris_speed(iris_speed), _focus_speed(focus_speed)
        {
        }

        virtual const std::string make_manscdp_body() override;

        virtual int HandleResponse(int statcode) override;

    private:
        std::string _channel_id;
        PtzCommand_E _iris = PtzCommand_E::NONE;
        PtzCommand_E _focus = PtzCommand_E::NONE;
        int _iris_speed = 0;
        int _focus_speed = 0;
    };
#if 0 /* TODO 后续才实现音视频相关的请求 */
    class RecordRequest : public MessageRequest
    {
    public:
        typedef std::shared_ptr<RecordRequest> Ptr;

        RecordRequest(eXosip_t *ctx, Device::Ptr device, const std::string &channel_id, int64_t start_time, int64_t end_time)
            : MessageRequest(ctx, device, REQUEST_MESSAGE_TYPE::DEVICE_RECORD_QUERY), _channel_id(channel_id), _start_time(start_time), _end_time(end_time) {

              };
        virtual const std::string make_manscdp_body();

        void InsertRecord(std::shared_ptr<RecordItem> item);

        uint32_t GetRecordSize();

        std::vector<std::shared_ptr<RecordItem>> GetRecordList();

    private:
        std::string _channel_id;

        int64_t _start_time;
        int64_t _end_time;

        std::vector<std::pair<std::string, std::string>> _presets;
        std::vector<std::shared_ptr<RecordItem>> _record_items;
    };
#endif

    class AlarmRequest : public MessageRequest
    {
    public:
        typedef std::shared_ptr<AlarmRequest> Ptr;

        AlarmRequest(eXosip_t *pCtx, Device::Ptr device, const GB28181::AlarmSubscribe_S &stInfo = GB28181::_AlarmSubscribe_S_())
            : MessageRequest(pCtx, device, REQUEST_MESSAGE_TYPE::DEVICE_RECORD_QUERY), m_stInfo(stInfo) {

              };

        virtual const std::string make_manscdp_body();

    private:
        GB28181::AlarmSubscribe_S m_stInfo;
    };

    class GuardRequest : public MessageRequest
    {
    public:
        typedef std::shared_ptr<GuardRequest> Ptr;

        GuardRequest(eXosip_t *pCtx, Device::Ptr device, bool bSetGuard)
            : MessageRequest(pCtx, device, REQUEST_MESSAGE_TYPE::DEVICE_RECORD_QUERY), m_bIsSetGuard(bSetGuard) {

              };

        virtual const std::string make_manscdp_body();

    private:
        bool m_bIsSetGuard = false; /* 布防标志，true为布防，false为撤防 */
    };

    class HeartbeatRequest : public MessageRequest
    {
    public:
        typedef std::shared_ptr<HeartbeatRequest> Ptr;

        HeartbeatRequest(eXosip_t *pCtx, Device::Ptr device, const std::string &strDeviceID,const std::string &strDeviceUrl = "")
            : MessageRequest(pCtx, device, REQUEST_MESSAGE_TYPE::KEEPALIVE), m_strDeviceID(strDeviceID), m_strDeviceUrl(strDeviceUrl)
        {
        }

        virtual const std::string make_manscdp_body();
        virtual const std::string GetDeviceUrl();

    private:
        std::string m_strDeviceID;
        /* 设备URL */ 
        std::string m_strDeviceUrl;
    };

    /*设备BasicParam配置控制*/
    class BasicParamRequest : public MessageRequest
    {
    public:
        typedef std::shared_ptr<BasicParamRequest> Ptr;

        BasicParamRequest(eXosip_t *pCtx, Device::Ptr device, const std::string &strDeviceID, const GB28181::BasicParamInfo_S &stInfo = GB28181::_BasicParam_())
            : MessageRequest(pCtx, device, REQUEST_MESSAGE_TYPE::DEVICE_CONTROL_BASICPARAM), m_strDeviceID(strDeviceID), m_tBasicParamInfo(stInfo)
        {
        }

        virtual const std::string make_manscdp_body();

    private:
            std::string m_strDeviceID;
            GB28181::BasicParamInfo_S m_tBasicParamInfo;
    };

    /*OSD配置控制*//*OSD配置控制*/
     class OSDConfigRequest : public MessageRequest
    {
    public:
        typedef std::shared_ptr<OSDConfigRequest> Ptr;

        OSDConfigRequest(eXosip_t *pCtx, Device::Ptr device, const std::string &strDeviceID, const GB28181::OSDConfig_S &stInfo = GB28181::_OSDConfig_())
            : MessageRequest(pCtx, device, REQUEST_MESSAGE_TYPE::DEVICE_CONTROL_OSD), m_strDeviceID(strDeviceID), m_OSDConfig(stInfo)
        {
        }

        virtual const std::string make_manscdp_body();

        int SendMessage(bool needcb = true) override;

    private:
        std::string m_strDeviceID;
        GB28181::OSDConfig_S m_OSDConfig;
    };

    /*SVAC编码配置命令*/
    class SVACEncodeConfigRequest : public MessageRequest
    {
    public:
        typedef std::shared_ptr<SVACEncodeConfigRequest> Ptr;

        SVACEncodeConfigRequest(eXosip_t *pCtx, Device::Ptr device, const std::string &strDeviceID, const GB28181::SVACEncodeInfo_S &stInfo = GB28181::_SVACEncodeConfig_())
            : MessageRequest(pCtx, device, REQUEST_MESSAGE_TYPE::DEVICE_CONTROL_SAVCENCODE), m_strDeviceID(strDeviceID), m_tSVACEncodeConfigInfo(stInfo)
        {
        }

        virtual const std::string make_manscdp_body();

        int SendMessage(bool needcb = true) override;

    private:
        std::string m_strDeviceID;
        GB28181::SVACEncodeInfo_S m_tSVACEncodeConfigInfo;
    };

    /*SVAC解码配置命令*/
    class SVACDecodeConfigRequest : public MessageRequest
    {
    public:
        typedef std::shared_ptr<SVACDecodeConfigRequest> Ptr;

        SVACDecodeConfigRequest(eXosip_t *pCtx, Device::Ptr device, const std::string &strDeviceID, const GB28181::SVACDecodeInfo_S &stInfo = GB28181::_SVACDecodeConfig_())
            : MessageRequest(pCtx, device, REQUEST_MESSAGE_TYPE::DEVICE_CONTROL_SAVCDECODE), m_strDeviceID(strDeviceID), m_tSVACDecodeConfigInfo(stInfo)
        {
        }

        virtual const std::string make_manscdp_body();

    private:
        std::string m_strDeviceID;
        GB28181::SVACDecodeInfo_S m_tSVACDecodeConfigInfo;
    };

    /*视频参数属性配置命令*/
    class VideoParamAttributeRequest : public MessageRequest
    {
    public:
        typedef std::shared_ptr<VideoParamAttributeRequest> Ptr;

        VideoParamAttributeRequest(eXosip_t *pCtx, Device::Ptr device, const std::string &strDeviceID, const GB28181::VideoParamAttributeInfo_S &stInfo = GB28181::_VideoParamAttributeConfig_())
            : MessageRequest(pCtx, device, REQUEST_MESSAGE_TYPE::DEVICE_CONTROL_VIDEOPARAM), m_strDeviceID(strDeviceID), m_tVideoParAttriConfigInfo(stInfo)
        {
        }

        virtual const std::string make_manscdp_body();

        int SendMessage(bool needcb = true) override;

    private:
        std::string m_strDeviceID;
        GB28181::VideoParamAttributeInfo_S m_tVideoParAttriConfigInfo;
    };

    /*录像计划配置命令*/
    class VideoRecordPlanRequest : public MessageRequest
    {
    public:
        typedef std::shared_ptr<VideoRecordPlanRequest> Ptr;

        VideoRecordPlanRequest(eXosip_t *pCtx, Device::Ptr device, const std::string &strDeviceID, const GB28181::VideoRecordPlanInfo_S &stInfo = GB28181::_VideoRecordPlan_())
            : MessageRequest(pCtx, device, REQUEST_MESSAGE_TYPE::DEVICE_CONTROL_VIDEORECORD), m_strDeviceID(strDeviceID), m_stInfo(stInfo)
        {
        }

        virtual const std::string make_manscdp_body();

        int SendMessage(bool needcb = true) override;

    private:
        std::string m_strDeviceID;
        GB28181::VideoRecordPlanInfo_S m_stInfo;
    };

    /*报警上报开关配置命令*/
    class AlarmReportRequest : public MessageRequest
    {
    public:
        typedef std::shared_ptr<AlarmReportRequest> Ptr;

        AlarmReportRequest(eXosip_t *pCtx, Device::Ptr device, const std::string &strDeviceID, const GB28181::AlarmReportInfo_S &stInfo = GB28181::_AlarmReport_())
            : MessageRequest(pCtx, device, REQUEST_MESSAGE_TYPE::DEVICE_CONTROL_ALARMREPORT), m_strDeviceID(strDeviceID), m_tAlarmReportInfo(stInfo)
        {
        }

        virtual const std::string make_manscdp_body();

    private:
        std::string m_strDeviceID;
        GB28181::AlarmReportInfo_S m_tAlarmReportInfo;
    };

    /*图像抓拍配置命令*/
    class SnapShotRequest : public MessageRequest
    {
    public:
        typedef std::shared_ptr<SnapShotRequest> Ptr;

        SnapShotRequest(eXosip_t *pCtx, Device::Ptr device, const std::string &strDeviceID, const GB28181::SnapShotConfigInfo_S &stInfo = GB28181::_SnapShotConfig_())
            : MessageRequest(pCtx, device, REQUEST_MESSAGE_TYPE::DEVICE_CONTROL_SNAPSHOT), m_strDeviceID(strDeviceID), m_tSnapShotConfigInfo(stInfo)
        {
        }

        virtual const std::string make_manscdp_body();

    private:
        std::string m_strDeviceID;
        GB28181::SnapShotConfigInfo_S m_tSnapShotConfigInfo;
    };

    /*报警录像配置命令*/
    class VideoAlarmRecordRequest : public MessageRequest
    {
    public:
        typedef std::shared_ptr<VideoAlarmRecordRequest> Ptr;

        VideoAlarmRecordRequest(eXosip_t *pCtx, Device::Ptr device, const std::string &strDeviceID, const GB28181::VideoAlarmRecordInfo_S &stInfo = GB28181::_VideoAlarmRecord_())
            : MessageRequest(pCtx, device, REQUEST_MESSAGE_TYPE::DEVICE_CONTROL_VIDEOALARM), m_strDeviceID(strDeviceID), m_tVideoAlarmRecordInfo(stInfo)
        {
        }

        virtual const std::string make_manscdp_body();

    private:
        std::string m_strDeviceID;
        GB28181::VideoAlarmRecordInfo_S m_tVideoAlarmRecordInfo;
    };

    /*视频画面遮挡配置命令*/
    class PictureMaskRequest : public MessageRequest
    {
    public:
        typedef std::shared_ptr<PictureMaskRequest> Ptr;

        PictureMaskRequest(eXosip_t *pCtx, Device::Ptr device, const std::string &strDeviceID, const GB28181::PictureMaskInfo_S &stInfo = GB28181::_PictureMask_())
            : MessageRequest(pCtx, device, REQUEST_MESSAGE_TYPE::DEVICE_CONTROL_PICTUREMAASK), m_strDeviceID(strDeviceID), m_tPictureMaskInfo(stInfo)
        {
        }

        virtual const std::string make_manscdp_body();

        int SendMessage(bool needcb = true) override;

    private:
        std::string m_strDeviceID;
        GB28181::PictureMaskInfo_S m_tPictureMaskInfo;
    };

    /*画面翻转配置命令*/
    class FrameMirrorRequest : public MessageRequest
    {
    public:
        typedef std::shared_ptr<FrameMirrorRequest> Ptr;

        FrameMirrorRequest(eXosip_t *pCtx, Device::Ptr device, const std::string &strDeviceID, const GB28181::FrameMirrorInfo_S &stInfo = GB28181::_FrameMirror_())
            : MessageRequest(pCtx, device, REQUEST_MESSAGE_TYPE::DEVICE_CONTROL_PICTUREMIRROR), m_strDeviceID(strDeviceID), m_tFrameMirrorInfo(stInfo)
        {
        }

        virtual const std::string make_manscdp_body();

    private:
            std::string m_strDeviceID;
            GB28181::FrameMirrorInfo_S m_tFrameMirrorInfo;
    };

    class ConfigDownloadRequest : public MessageRequest
    {
    public:
        enum CONFIG_REQUEST_MESSAGE_TYPE
        {
            DEVICE_QUERY_BASICPARAM,  // 设备配置查询-基本参数
            DEVICE_QUERY_VIDEOOPT,    // 设备配置查询-视频参数范围
            DEVICE_QUERY_SVACENCODE,  // 设备配置查询-SVAC编码
            DEVICE_QUERY_SVACDNCODE,  // 设备配置查询-SVAC解码
            DEVICE_QUERY_VIDEATTRIBT, // 设备配置查询-视频属性
            DEVICE_QUERY_VIDERECORD,  // 设备配置查询-录像计划
            DEVICE_QUERY_VIDEOALARM,  // 设备配置查询-报警录像
            DEVICE_QUERY_PICTRUE,     // 设备配置查询-视频画面遮挡
            DEVICE_QUERY_FRAMEMIR,    // 设备配置查询-画面翻转
            DEVICE_QUERY_ALARM,       // 设备配置查询-报警上报开关
            DEVICE_QUERY_OSD,         // 设备配置查询-前端OSD设置
            DEVICE_QUERY_SNAP,        // 设备配置查询-图像抓拍EventBusiness.cpp:139
            DEVICE_QUERY_ALL          // 所有配置
        };

    public:
        typedef std::shared_ptr<ConfigDownloadRequest> Ptr;

        ConfigDownloadRequest(eXosip_t *pCtx, Device::Ptr device, const std::string &strDeviceID, CONFIG_REQUEST_MESSAGE_TYPE ConfigType)
            : MessageRequest(pCtx, device, REQUEST_MESSAGE_TYPE::DEVICE_QUERY_CONFDOWN), m_strDeviceID(strDeviceID), m_ConfigType(ConfigType)
        {
        }

        virtual const std::string make_manscdp_body();

    private:
        std::string m_strDeviceID;
        CONFIG_REQUEST_MESSAGE_TYPE m_ConfigType;
    };

    /*巡航轨迹查询命令*/
    class CruiseTrackQueryRequest : public MessageRequest
    {
    public:
        typedef std::shared_ptr<CruiseTrackQueryRequest> Ptr;

        CruiseTrackQueryRequest(eXosip_t *pCtx, Device::Ptr device, const std::string &strDeviceID, const GB28181::CruiseTrackQueryInfo_S &stInfo = GB28181::_CruiseTrackQuery_())
            : MessageRequest(pCtx, device, REQUEST_MESSAGE_TYPE::DEVICE_QUERY_CRUISETRACK), m_strDeviceID(strDeviceID), m_tCruiseTrackInfo(stInfo)
        {
        }

        virtual const std::string make_manscdp_body();

    private:
            std::string m_strDeviceID;
            GB28181::CruiseTrackQueryInfo_S m_tCruiseTrackInfo;
    };

     /*巡航轨迹查列表询命令*/
     class CruiseTrackListQueryRequest : public MessageRequest
     {
     public:
         typedef std::shared_ptr<CruiseTrackListQueryRequest> Ptr;
 
         CruiseTrackListQueryRequest(eXosip_t *pCtx, Device::Ptr device, const std::string &strDeviceID)
             : MessageRequest(pCtx, device, REQUEST_MESSAGE_TYPE::DEVICE_QUERY_CRUISETRACKLIST), m_strDeviceID(strDeviceID)
         {
         }
 
         virtual const std::string make_manscdp_body();
 
     private:
             std::string m_strDeviceID;
     };

    /*设备预置位查询*/
    class PresetQueryRequest : public MessageRequest
    {
    public:
        typedef std::shared_ptr<PresetQueryRequest> Ptr;
 
        PresetQueryRequest(eXosip_t *pCtx, Device::Ptr device, const std::string &strDeviceID)
             : MessageRequest(pCtx, device, REQUEST_MESSAGE_TYPE::DEVICE_QUERY_PRESET), m_strDeviceID(strDeviceID)
        {
        }
 
        virtual const std::string make_manscdp_body();
 
    private:
        std::string m_strDeviceID;
     };

    /*语音广播*/
    class BroadcastRequest : public MessageRequest
    {
    public:
        typedef std::shared_ptr<BroadcastRequest> Ptr;
 
        BroadcastRequest(eXosip_t *pCtx, Device::Ptr device, const std::string &strDeviceID, const GB28181::BroadcastInfo_S &stInfo = GB28181::_Broadcast_())
             : MessageRequest(pCtx, device, REQUEST_MESSAGE_TYPE::REQUEST_CALL_INVITE), m_strDeviceID(strDeviceID), m_tBroadcast(stInfo)
        {
        }
 
        virtual const std::string make_manscdp_body();
 
    private:
        std::string m_strDeviceID;
        GB28181::BroadcastInfo_S m_tBroadcast;
     };

     /*设备远程启动控制命令*/
    class TeleBootRequest : public MessageRequest
    {
    public:
        typedef std::shared_ptr<TeleBootRequest> Ptr;
 
        TeleBootRequest(eXosip_t *pCtx, Device::Ptr device, const std::string &strDeviceID)
             : MessageRequest(pCtx, device, REQUEST_MESSAGE_TYPE::DEVICE_CONTROL_TELEBOOT), m_strDeviceID(strDeviceID)
        {
        }
 
        virtual const std::string make_manscdp_body();
 
    private:
        std::string m_strDeviceID;
     };

    /*设备强制关键帧控制命令*/
    class IFrameRequest : public MessageRequest
    {
    public:
        typedef std::shared_ptr<IFrameRequest> Ptr;
 
        IFrameRequest(eXosip_t *pCtx, Device::Ptr device, const std::string &strDeviceID)
             : MessageRequest(pCtx, device, REQUEST_MESSAGE_TYPE::DEVICE_CONTROL_IFRAME), m_strDeviceID(strDeviceID)
        {
        }
 
        virtual const std::string make_manscdp_body();
 
    private:
        std::string m_strDeviceID;
     };

    /*拉框放大/缩小控制命令*/
    class DragZoomInOutRequest : public MessageRequest
    {
    public:
        typedef std::shared_ptr<DragZoomInOutRequest> Ptr;
 
        DragZoomInOutRequest(eXosip_t *pCtx, Device::Ptr device, const std::string &strDeviceID, const GB28181::DragZoomInfo_S &stInfo = GB28181::_DragZoom_())
             : MessageRequest(pCtx, device, REQUEST_MESSAGE_TYPE::DEVICE_CONTROL_DRAGZOOMINUT), m_strDeviceID(strDeviceID), m_tDragZoomInfo(stInfo)
        {
        }
 
        virtual const std::string make_manscdp_body();
 
    private:
        std::string m_strDeviceID;
        GB28181::DragZoomInfo_S m_tDragZoomInfo;
     };

    /*PTZ精准控制命令*/
    class PTZPreciseCtrlRequest : public MessageRequest
    {
    public:
        typedef std::shared_ptr<PTZPreciseCtrlRequest> Ptr;
 
        PTZPreciseCtrlRequest(eXosip_t *pCtx, Device::Ptr device, const std::string &strDeviceID, const GB28181::PTZPreciseCtrl_S &stInfo = GB28181::_PTZPreciseCtrl_())
             : MessageRequest(pCtx, device, REQUEST_MESSAGE_TYPE::DEVICE_CONTROL_PTZCTRL), m_strDeviceID(strDeviceID), m_tPTZPreciseCtrlInfo(stInfo)
        {
        }
 
        virtual const std::string make_manscdp_body();
 
    private:
        std::string m_strDeviceID;
        GB28181::PTZPreciseCtrl_S m_tPTZPreciseCtrlInfo;
     };

    /*录像控制命令*/
    class RecordCmdRequest : public MessageRequest
    {
    public:
        typedef std::shared_ptr<RecordCmdRequest> Ptr;
 
        RecordCmdRequest(eXosip_t *pCtx, Device::Ptr device, const std::string &strDeviceID, const GB28181::RecordCmd_S &stInfo = GB28181::_RecordCmd_())
             : MessageRequest(pCtx, device, REQUEST_MESSAGE_TYPE::DEVICE_CONTROL_RECORDCMD), m_strDeviceID(strDeviceID), m_tRecordCmdInfo(stInfo)
        {
        }
 
        virtual const std::string make_manscdp_body();
 
    private:
        std::string m_strDeviceID;
        GB28181::RecordCmd_S m_tRecordCmdInfo;
     };

    /*报警复位控制命令*/
    class AlarmCmdRequest : public MessageRequest
    {
    public:
        typedef std::shared_ptr<AlarmCmdRequest> Ptr;

        AlarmCmdRequest(eXosip_t *pCtx, Device::Ptr device, const std::string &strDeviceID, const GB28181::AlarmCmd_S &stInfo = GB28181::_AlarmCmd_())
                : MessageRequest(pCtx, device, REQUEST_MESSAGE_TYPE::DEVICE_CONTROL_ALARMCMD), m_strDeviceID(strDeviceID), m_tAlarmCmdInfo(stInfo)
        {
        }

        virtual const std::string make_manscdp_body();

    private:
        std::string m_strDeviceID;
        GB28181::AlarmCmd_S m_tAlarmCmdInfo;
    };

    /*看守位控制命令*/
    class HomePositionRequest : public MessageRequest
    {
    public:
        typedef std::shared_ptr<HomePositionRequest> Ptr;
 
        HomePositionRequest(eXosip_t *pCtx, Device::Ptr device, const std::string &strDeviceID, const GB28181::HomePositionInfo_S &stInfo = GB28181::_HomePosition_())
             : MessageRequest(pCtx, device, REQUEST_MESSAGE_TYPE::DEVICE_CONTROL_ALARMCMD), m_strDeviceID(strDeviceID), m_tHomePositionInfo(stInfo)
        {
        }
 
        virtual const std::string make_manscdp_body();
 
    private:
        std::string m_strDeviceID;
        GB28181::HomePositionInfo_S m_tHomePositionInfo;
    };

    /*设备软件控制命令*/
    class DeviceUpgradeRequest : public MessageRequest
    {
    public:
        typedef std::shared_ptr<DeviceUpgradeRequest> Ptr;
    
        DeviceUpgradeRequest(eXosip_t *pCtx, Device::Ptr device, const std::string &strDeviceID, const GB28181::DeviceUpgradeInfo_S &stInfo = GB28181::_DeviceUpgrade_())
                : MessageRequest(pCtx, device, REQUEST_MESSAGE_TYPE::DEVICE_CONTROL_ALARMCMD), m_strDeviceID(strDeviceID), m_tDeviceUpgradeInfo(stInfo)
        {
        }
    
        virtual const std::string make_manscdp_body();
    
    private:
        std::string m_strDeviceID;
        GB28181::DeviceUpgradeInfo_S m_tDeviceUpgradeInfo;
    };

    /*目标跟踪控制命令*/
    class TargetTrackRequest : public MessageRequest
    {
    public:
        typedef std::shared_ptr<TargetTrackRequest> Ptr;
    
        TargetTrackRequest(eXosip_t *pCtx, Device::Ptr device, const std::string &strDeviceID, const GB28181::TargetTrackInfo_S &stInfo = GB28181::_TargetTrack_())
                : MessageRequest(pCtx, device, REQUEST_MESSAGE_TYPE::DEVICE_CONTROL_ALARMCMD), m_strDeviceID(strDeviceID), m_tTargetTrackInfo(stInfo)
        {
        }
    
        virtual const std::string make_manscdp_body();
    
    private:
        std::string m_strDeviceID;
        GB28181::TargetTrackInfo_S m_tTargetTrackInfo;
    };

    /*看守位信息查询命令*/
    class HomePositionQueryRequest : public MessageRequest
    {
    public:
        typedef std::shared_ptr<HomePositionQueryRequest> Ptr;
    
        HomePositionQueryRequest(eXosip_t *pCtx, Device::Ptr device, const std::string &strDeviceID)
                : MessageRequest(pCtx, device, REQUEST_MESSAGE_TYPE::DEVICE_CONTROL_ALARMCMD), m_strDeviceID(strDeviceID)
        {
        }
    
        virtual const std::string make_manscdp_body();
    
    private:
        std::string m_strDeviceID;
    };

    /*PTZ精确状态查询命令*/
    class PTZPositionRequest : public MessageRequest
    {
    public:
        typedef std::shared_ptr<PTZPositionRequest> Ptr;
    
        PTZPositionRequest(eXosip_t *pCtx, Device::Ptr device, const std::string &strDeviceID)
                : MessageRequest(pCtx, device, REQUEST_MESSAGE_TYPE::DEVICE_CONTROL_ALARMCMD), m_strDeviceID(strDeviceID)
        {
        }
    
        virtual const std::string make_manscdp_body();
    
    private:
        std::string m_strDeviceID;
    };

}