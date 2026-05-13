/*
 * @Author       : EasonLu
 * @Date         : 2025-02-14 16:49:40
 * @LastEditors  : EasonLu
 * @LastEditTime : 2025-04-24 19:38:24
 * @FilePath     : BaseRequest.h
 * @Description  : 请求基类
 */
#pragma once
#include "SipDevice.h"
#include "SipType.h"
#include <condition_variable>
namespace SIP
{

    /// @brief 请求方法
    enum REQUEST_MESSAGE_TYPE
    {
        REQUEST_TYPE_UNKNOWN = 0,
        KEEPALIVE,                      // 保活心跳
        QUERY_CATALOG,                  //   查询目录
        QUERY_DEVICEINFO,               //   查询设备信息
        DEVICE_CONTROL_PTZ,             // 设备控制-云台
        DEVICE_CONTROL_BASICPARAM,      // 设备控制-基本参数配置
        DEVICE_CONTROL_OSD,             // 设备控制-OSD
        DEVICE_CONTROL_PICTUREMAASK,    // 设备控制-视频画面遮挡配置
        DEVICE_CONTROL_PICTUREMIRROR,   // 设备控制-画面翻转配置
        DEVICE_CONTROL_VIDEOALARM,      // 设备控制-报警录像配置
        DEVICE_CONTROL_SNAPSHOT,        // 设备控制-图像抓拍配置
        DEVICE_CONTROL_ALARMREPORT,     // 设备控制-报警上报开关配置
        DEVICE_CONTROL_VIDEORECORD,     // 设备控制-录像计划配置
        DEVICE_CONTROL_VIDEOPARAM,      // 设备控制-视频参数属性配置
        DEVICE_CONTROL_SAVCENCODE,      // 设备控制-SVAC编码配置
        DEVICE_CONTROL_SAVCDECODE,      // 设备控制-SVAC解码配置
        DEVICE_CONTROL_PRESET,          // 设备控制-预置位
        DEVICE_CONTROL_HOMEPOSITION,    // 设备控制-看守位
        DEVICE_CONTROL_TELEBOOT,        // 设备控制-远程启动
        DEVICE_CONTROL_IFRAME,          // 设备控制-强制关键帧
        DEVICE_CONTROL_DRAGZOOMINUT,    // 设备控制-拉框放大缩小
        DEVICE_CONTROL_PTZCTRL,         // 设备控制-PTZ精准控制
        DEVICE_CONTROL_RECORDCMD,       // 设备控制-录像控制
        DEVICE_CONTROL_ALARMCMD,        // 设备控制-报警复位控制
        DEVICE_QUERY_PRESET,            // 设备查询-预置位
        DEVICE_QUERY_CONFDOWN,          // 设备查询-配置查询
        DEVICE_QUERY_CRUISETRACK,       // 设备查询-巡航轨迹查询
        DEVICE_QUERY_CRUISETRACKLIST,   // 设备查询-巡航轨迹列表查询
       

        DEVICE_RECORD_QUERY, // 录像文件查询

        REQUEST_CALL_INVITE,   // 点播
        REQUEST_CALL_PLAYBACK, // 回放
        REQUEST_CALL_LIVE,     // 直播
        REQUEST_CALL_DOWNLOAD, // 下载
        REQUEST_CALL_BYE,      // 挂断

        RESPONSE_ADD_SN, /* 添加SN */
        REQUEST_TYPE_MAX
    };

    class BaseRequest : public std::enable_shared_from_this<BaseRequest>
    {
    public:
        typedef std::shared_ptr<BaseRequest> Ptr;
        BaseRequest(eXosip_t *ctx, Device::Ptr device, REQUEST_MESSAGE_TYPE type);
        virtual ~BaseRequest();

        virtual int HandleResponse(int status_code);
        void SetWait(bool bwait = true);
        void WaitResult();
        void Finish();
        bool IsFinished();
        void SetRequestID(const std::string &id);
        void OnRequestFinished();
        std::time_t GetRequestTime();
        REQUEST_MESSAGE_TYPE GetRequestType();

    protected:
        const char *_get_request_id_from_request(osip_message_t *msg);
        Device::Ptr GetDevice();

    protected:
        std::string _request_id;
        eXosip_t *m_pContext;
        Device::Ptr _device = nullptr;

    private:
        bool _b_finished = false;
        bool _b_wait = false;
        REQUEST_MESSAGE_TYPE _request_type = REQUEST_TYPE_UNKNOWN;
        std::time_t _request_time;

        std::mutex _mutex;
        std::condition_variable _cv;
    };
}