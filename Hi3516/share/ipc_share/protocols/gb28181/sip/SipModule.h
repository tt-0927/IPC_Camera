/*
 * @Author       : EasonLu
 * @Date         : 2025-02-12 16:46:23
 * @LastEditors  : EasonLu
 * @LastEditTime : 2025-04-30 15:00:55
 * @FilePath     : SipModule.h
 * @Description  : SIP模块管理
 */
#pragma once
#include "GbDefine.h"
#include "ModuleLog.h"
#include "SipClient.h"
#include "SipServer.h"
#include "SipType.h"
#include <atomic>
#include <memory>
#include <mutex>
#include <vector>
namespace SIP
{
    class SipModule
    {
    public:
        /**
         * @brief  模块管理单例
         * @return [*]
         * @author EasonLu
         * @note
         */
        static SipModule *instance()
        {
            std::lock_guard<std::mutex> lock(m_mtx);
            if (m_pInstance == nullptr)
            {
                m_pInstance = new SipModule();
            }
            return m_pInstance;
        }

        /**
         * @brief  初始化
         * @return [*]
         * @author EasonLu
         * @note
         */
        int Init(CbInfo_S stInfo);

        /**
         * @brief  反初始化
         * @return [*]
         * @author EasonLu
         * @note
         */
        int Deinit();

        /**
         * @brief  开启SIP服务
         * @param  [SipServerInfo_S] stInfo - SIP服务器信息
         * @return [bool] - 开启成功返回true
         * @author EasonLu
         * @note
         */
        void StartServer(SipServerInfo_S stInfo);

        /**
         * @brief  关闭SIP服务
         * @return [*]
         * @author EasonLu
         * @note
         */
        void StopServer();

        void StartClient(SipClientInfo_S stInfo, bool bEnableGm = false);

        void StopClient();

        void RebootClient();

        /**
         * @brief  根据SIP句柄获取SIP服务器信息
         * @param  [eXosip_t] *pCtx - SIP句柄
         * @return [const SipServerInfo_S&] - SIP服务器信息
         * @author EasonLu
         * @note
         */
        const SipServerInfo_S &GetSipServerInfo(eXosip_t *pCtx);

        /**
         * @brief  根据SIP句柄获取SIP URI
         * @param  [eXosip_t] *pCtx - SIP句柄
         * @return [const std::string&] - SIP URI
         * @author EasonLu
         * @note
         */
        const std::string GetSipUriByContext(eXosip_t *pCtx);

        /**
         * @brief  获取设备信息
         * @param  [vector<SipDeviceInfo_S>] &vecDevInfo - 设备信息
         * @return [int] - 设备总数，-1失败
         * @author EasonLu
         * @note
         */
        int GetDevInfo(std::vector<SipDeviceInfo_S> &vecDevInfo);

        /**
         * @brief  获取NVR的设备相关信息
         * @param  [SipLocalInfo_S] &stInfo - 设备信息
         * @return [*]
         * @author EasonLu
         * @note
         */
        int GetLocalInfo(SipLocalInfo_S &stInfo);

        /**
         * @brief  对指定设备进行Ptz控制
         * @param  [SipDeviceInfo_S] &device - 设备信息
         * @param  [SipPtzType_E] enCmd - Ptz控制命令
         * @param  [int] nSpeed - Ptz控制速度
         * @return [int] - 0成功，-1失败
         * @author EasonLu
         * @note
         */
        int PtzCtrl(const SipDeviceInfo_S &device, SipPtzType_E enCmd, int nSpeed = 128);

        /**
         * @brief  对指定设备进行预置位控制
         * @param  [SipDeviceInfo_S] &device - 设备信息
         * @param  [SipPresetType_E] enCmd - 预置位控制命令
         * @param  [unsigned int] nPresetID - 预置位ID
         * @return [int] - 0成功，-1失败
         * @author EasonLu
         * @note
         */
        int PresetCtrl(const SipDeviceInfo_S &device, SipPresetType_E enCmd, unsigned int nPresetID);

        /**
         * @brief  对指定设备进行报警订阅
         * @param  [SipDeviceInfo_S] &device - 设备信息
         * @param  [AlarmSubscribe_S] &stInfo - 报警订阅信息(默认不限时间订阅所有报警信息)
         * @return [*]
         * @author EasonLu
         * @note   订阅后需要布防设备才能使设备上抛报警数据
         */
        int AlarmSubscribe(const SipDeviceInfo_S &device, const GB28181::AlarmSubscribe_S &stInfo = GB28181::_AlarmSubscribe_S_());

        /**
         * @brief  对指定设备进行布防
         * @param  [SipDeviceInfo_S] &device - 设备信息
         * @param  [bool] bSetGuard - 是否设置布防
         * @return [int] - 0成功，-1失败
         * @author EasonLu
         * @note   布防设备后，需要订阅报警信息才能使设备上抛报警数据
         */
        int Guard(const SipDeviceInfo_S &device, bool bSetGuard);

        /**
         * @brief  对指定设备进行预览
         * @param  [SipDeviceInfo_S] &device - 设备信息
         * @param  [bool] bStart - 是否开始预览,默认开始,false为停止
         * @return [*]
         * @author EasonLu
         * @note
         */
        int Play(const SipDeviceInfo_S &device, bool bStart = true);

        /**
         * @brief  对客户端的指定设备发送音视频数据
         * @param  [SipDeviceInfo_S] &device - 设备信息
         * @param  [char] *pBuf - 媒体数据
         * @param  [int] nLen - 媒体数据长度
         * @param  [bool] bIsAudio - 是否是音频
         * @return [*]
         * @author EasonLu
         * @note
         */
        int SendPlayMedia(SipDeviceInfo_S &device, char *pBuf, int nLen, bool bIsAudio);

        /**
         * @brief  发送读取录制文件的媒体数据
         * @param  [string] &strCallID - 对应的会话ID
         * @param  [char] *pBuf - 媒体数据
         * @param  [int] nLen - 媒体数据长度
         * @param  [bool] bIsAudio - 是否是音频数据
         * @return [*]
         * @author EasonLu
         * @note   
         */
        int SendReadFileMedia(const std::string &strCallID, char *pBuf, int nLen, bool bIsAudio);

        /**
         * @brief  对客户端的指定设备设置编码器信息
         * @param  [SipDeviceInfo_S] &device - 设备信息
         * @return [*]
         * @author EasonLu
         * @note
         */
        int SetClientCodec(SipDeviceInfo_S &device);

        /**
         * @brief  发送本机报警信息给上级
         * @param  [AlarmInfo_S] &stInfo - 报警信息
         * @return [*]
         * @author EasonLu
         * @note   只发送给上级，不会发送给下级
         */
        int SendAlarmInfo(GB28181::AlarmInfo_S &stInfo);

        /**
         * @brief  更新设备信息回调函数
         * @param  [SipDevInfoObserver] fnDevInfoCb - 设备信息更新回调函数
         * @return [*]
         * @author EasonLu
         * @note
         */
        void SetDevInfoCb(SipDevInfoObserver fnDevInfoCb)
        {
            m_stCbInfo.fnDevUpdate = fnDevInfoCb;
        }

        /**
         * @brief  获取设备信息更新回调函数
         * @return [SipDevInfoObserver] - 设备信息更新回调函数
         * @author EasonLu
         * @note
         */
        SipDevInfoObserver &GetDevInfoCb()
        {
            return m_stCbInfo.fnDevUpdate;
        }

        /*
         * @brief  发送图像抓拍传输完成信息给上级
         * @param  [AlarmInfo_S] &stInfo - 报警信息
         * @return [*]
         * @author 
         * @note   只发送给上级，不会发送给下级
        */
        int SendUploadSnapShotFiniInfo(GB28181::UploadSnapShotFiniInfo_S &stInfo);

        inline void SetMediaCb(SipMediaCallBack fnMediaCb)
        {
            m_stCbInfo.fnMediaUpdate = fnMediaCb;
        }

        inline SipMediaCallBack &GetMediaCb()
        {
            return m_stCbInfo.fnMediaUpdate;
        }

        inline void SetMediaStatusCb(SipMediaStatusCb fnMediaStatus)
        {
            m_stCbInfo.fnMediaStatus = fnMediaStatus;
        }

        inline SipMediaStatusCb &GetMediaStatusCb()
        {
            return m_stCbInfo.fnMediaStatus;
        }

        /* 设备上线状态 */
        inline SipOnlineStatusCb GetOnlineStatusCb()
        {
            return m_stCbInfo.fnOnlineStatus;
        }

        inline void SetQueryRecordInfoCb(SipQueryRecordInfoCb fnQueryRecordInfo)
        {
            m_stCbInfo.fnQueryRecordInfo = fnQueryRecordInfo;
        }

        inline SipQueryRecordInfoCb &GetQueryRecordInfoCb()
        {
            return m_stCbInfo.fnQueryRecordInfo;
        }

        inline void SetGetRecFileCb(SipGetRecFileCb fnGetRecFile)
        {
            m_stCbInfo.fnGetRecFile = fnGetRecFile;
        }

        inline SipGetRecFileCb &GetGetRecFileCb()
        {
            return m_stCbInfo.fnGetRecFile;
        }

//设备配置
        inline SipBasicParamCb &GetBasicParamCb()
        {
            return m_stCbInfo.fnBasicParam;
        }

        inline SipVideoParamOptCb &GetVideoParamOptCb()
        {
            return m_stCbInfo.fnVideoParamOpt;
        }

        inline SipSVACEncodeCb &GetSVACEncodeCb()
        {
            return m_stCbInfo.fnSVACEncode;
        }

        inline SipSVACDncodeCb &GetSVACDncodeCb()
        {
            return m_stCbInfo.fnSVACDncode;
        }

        inline SipVideoAttributeCb &GetVideoAttributeCb()
        {
            return m_stCbInfo.fnVideoAttribute;
        }

        inline SipRecordPlanCb &GetRecordPlanCb()
        {
            return m_stCbInfo.fnRecordPlan;
        }

        inline SipAlarmRecordCb &GetAlarmRecordCb()
        {
            return m_stCbInfo.fnAlarmRecord;
        }

        inline SipPictureMaskCb &GetPictureMaskCb()
        {
            return m_stCbInfo.fnPictureMask;
        }

        inline SipFrameMirrorCb &GetFrameMirrorCb()
        {
            return m_stCbInfo.fnFrameMirror;
        }
        inline SipAlarmReportCb &GetAlarmReportCb()
        {
            return m_stCbInfo.fnAlarmReport;
        }
        inline SipOSDConfigCb &GetOSDConfigCb()
        {
            return m_stCbInfo.fnOSDConfig;
        }
        inline SipSnapShotCb &GetSnapShotCb()
        {
            return m_stCbInfo.fnSnapShot;
        }
//巡航轨迹
        inline SipCruiseTrackQueryCb &GetCruiseTrackQueryCb()
        {
            return m_stCbInfo.fnCruiseTrackQuery;
        }
        inline SipCruiseTrackListQueryCb &GetCruiseTrackListQueryCb()
        {
            return m_stCbInfo.fnCruiseTrackListQuery;
        }
//设备预置位
        inline SipPresetQueryCb &GetPresetQueryCb()
        {
            return m_stCbInfo.fnPresetQuery;
        }
//语音广播
        inline SipBroadcastCb &GetBroadcastCb()
        {
            return m_stCbInfo.fnBroadcast;
        }
//设备校时
        inline SipNVRSetTimeCb &GetNVRSetTimeCb()
        {
            return m_stCbInfo.fnSetTime;
        }

//远程启动
        inline SipNVRTeleBootCb &GetNVRTeleBootCb()
        {
            return m_stCbInfo.fnNVRTeleBoot;
        }

//强制关机帧
        inline SipIFrameCmdCb &GetIFrameCmdCb()
        {
            return m_stCbInfo.fnIFrameCmd;
        }
//拉框放大/缩小
        inline SipDragZoomInOutCb &GetDragZoomInOutCb()
        {
            return m_stCbInfo.fnDragZoomInOut;
        }
//PTZ精准控制
        inline SipPTZPreciseCtrlCb &GetPTZPreciseCtrlCb()
        {
            return m_stCbInfo.fnPTZPreciseCtrl;
        }
//录像控制
        inline SipRecordCmdCb &GetRecordCmdCb()
        {
            return m_stCbInfo.fnRecordCmd;
        }
//设备控制应答
        inline SipControlResultsCb &GetControlResultsCb()
        {
            return m_stCbInfo.fnControlResults;
        }
//报警复位控制
        inline SipAlarmCmdCb &GetAlarmCmdCb()
        {
            return m_stCbInfo.fnAlarmCmd;
        }
//看守为控制
        inline SipHomePositionCb &GetHomePositionCb()
        {
            return m_stCbInfo.fnHomePosition;
        }
//设备软件升级
        inline SipDeviceUpgradeCb &GetDeviceUpgradeCb()
        {
            return m_stCbInfo.fnDeviceUpgrade;
        }
//目标跟踪控制
        inline SipTargetTrackCb &GetTargetTrackCb()
        {
            return m_stCbInfo.fnTargetTrack;
        }
//PTZ精确状态查询
        inline SipPTZPositionCb &GetPTZPositionCb()
        {
            return m_stCbInfo.fnPTZPosition;
        }
//图像传输完成通知
        inline SipUploadSnapShotFiniCb &GetUploadSnapShotFiniCb()
        {
            return m_stCbInfo.fnUploadSnapShotFini;
        }

        bool get_client_status();

        /**
         * @brief  更新通道信息
         * @param  [vector<SipChannelInfo_S>] &vecChnInfo
         * @return [int] - 0成功，-1失败
         * @author EasonLu
         * @note   只要用于客户端更新本地的SIP信息
         */
        int UpdateChnInfo(std::vector<SipChannelInfo_S> &vecChnInfo);

    protected:
        SipModule() {};
        ~SipModule() {};

    private:
        static std::mutex m_mtx;
        static SipModule *m_pInstance;
        std::atomic_bool m_bInit = false; /* 初始化标记位 */
        SipServer m_stUdpSipServer;
        SipServer m_stTcpSipServer;
        SipClient m_stClient;
        SipServerInfo_S m_stEmptyInfo;
        CbInfo_S m_stCbInfo; /* 记录的初始化信息 */

        std::atomic<bool> m_bIsStarting = false;
        std::thread m_startThread;
    };
}
