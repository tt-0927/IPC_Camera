/**
 * @FilePath     : replay_define.h
 * @Author       : zhangjc (zhangjc@kfb.cn)
 * @Date         : 2024-10-29
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2025-06-30 20:30:06
 * @Description  : 回放定义
 */

#pragma once

#include <string>
#include <vector>

#include "common_define.h"
#include "layout_define.h"
// #include "ipc_define.h"
#include "record_define.h"

namespace Replay
{
    /// @brief 菜单按钮枚举
    typedef enum class ReviewMenu
    {
        ENLARGE = 0, /* 电子放大 */
        VOLUME,      /* 声音 */
        LAB,         /* 添加标签 */
        LOCK,        /* 锁定 */
        EDIT,        /* 裁剪 */
        RULES,       /* 空间规则 */
        SMART,       /* 智能显示 */
        SELFADJ,     /* 自适应分辨率 */
        NOT,         /* 未设置 */
    } ReviewMenu_E;

    /// @brief 播放状态枚举
    typedef enum class State
    {
        PLAY = 0, /* 播放 */
        PAUSE,    /* 暂停 */
        STOP,     /* 结束 */
    } State_E;

    /// @brief 缩放移动枚举
    typedef enum class _ZoomMove_
    {
        ZOOM = 0, /* 缩放 */
        MOVE,     /* 移动 */
    } ZoomMove_E;

    typedef enum class PlaybackMode
    {
        REAL_TIME = 0, /* 实时 */
        BALANCED,      /* 均衡 */
        SMOOTH         /* 流畅 */
    } PlaybackMode_E;

    /// @brief 预览画面设置
    typedef struct
    {
        Layout::Type_E enLayout = Layout::Type_E::SCREEN_1; /* 布局类型 */

        std::vector<Layout::ChnInfo_S> vChnInfos; /* 通道信息 */
        Common::PageInfo_S stPageInfo;            /* 页数据信息 */
    } Info_S;

    /// @brief 布局信息
    typedef struct
    {
        Layout::Type_E enLayout = Layout::Type_E::SCREEN_1; /* 布局类型 */
    } Layout_S;

    /// @brief 轮巡信息
    typedef struct
    {
        int nInterval = 5000;       /* 轮巡间隔/ms */
        bool bEnablePatrol = false; /* 是否开始轮巡 */
    } Patrol_S;

    /// @brief 回看信息
    typedef struct
    {
        Layout::Item_S stItem; /* 通道信息 */

        State_E enState; /* 播放状态 */
        int nCurTime;    /* 当前播放时间 */
        int nTotalTime;  /* 总时长 */
    } Playback_S;

    /// @brief 保存图片
    typedef struct
    {
        Layout::Item_S stItem; /* 通道信息 */

        std::string strPath;     /* 保存的路径 */
        std::string strFileName; /* 保存的文件名称 */
    } SaveImage_S;

    /// @brief 电子放大
    typedef struct
    {
        Layout::Item_S stItem; /* 通道信息 */

        bool bEnable = false;                 /* 使能，退出电子放大时需要发送false */
        ZoomMove_E enType = ZoomMove_E::ZOOM; /* 缩放移动 */
        int nMoveX = 0;                       /* 移动距离像素,x */
        int nMoveY = 0;                       /* 移动距离像素,y */
        int nWheelDelta = 0;                  /* 鼠标滚轮增量 0-100 */
    } DigitalZoom_S;

    /// @brief 声音信息
    typedef struct
    {
        Layout::Item_S stItem; /* 通道信息 */

        bool bEnable = false; /* 是否开启声音 */
        int nVolume;          /* 音量0-100 */
    } Voice_S;

    /// @brief 播放性能
    typedef struct
    {
        Layout::Item_S stItem; /* 通道信息 */

        PlaybackMode_E enMode; /* 性能参数 */
    } PlaybackInfo_S;

    /// @brief 码流信息
    typedef struct
    {
        Layout::Item_S stItem; /* 通道信息 */

        bool bMainStream; /* 是否主码流 */
    } StreamInfo_S;

    /// @brief AI信息展示
    typedef struct
    {
        Layout::Item_S stItem; /* 通道信息 */

        bool bShow; /* 是否显示 */
    } AiShowInfo_S;

    /// @brief 自适应分辨率
    typedef struct
    {
        Layout::Item_S stItem; /* 通道信息 */

        bool bEnable; /* 是否开启 */
    } AdaptiveResInfo_S;

    /// @brief ipc信息
    typedef struct
    {
        int nType = 0; /* 视频类型，0普通，非0事件 */
        // std::vector<Ipc::Info_S> ipcInfos;
    } IpcList_S;

    /// @brief 视频时间段
    typedef struct RecordTime
    {
        std::vector<Record_NS::VideoTime_S> videoTimes;        /* 视频时间段 */
        // std::vector<Record_NS::VideoTime_S> personEventTimes;  /* 人员事件时间段 */
        // std::vector<Record_NS::VideoTime_S> vehicleEventTimes; /* 车辆事件时间段 */
        // std::vector<Record_NS::VideoTime_S> motionEventTimes;   /* 所有运动目标事件时间段 */
        std::vector<Record_NS::VideoTime_S> EventTimes;   /* 事件时间段 */
        int nEventType = -1;
    } RecordTime_S;

    /// @brief 通道信息
    typedef struct
    {
        Layout::Item_S stItem;
        Layout::Rect_S stRect; /* 通道信息 */
        std::string filename;  /* 文件名 */
        int nType = 0;         /* 视频类型，0普通，非0事件 */
        std::string date;      /* YYYY-MM-DD需要播放哪天的视频，次优先使用 */
        /* 以下字段异步播放使用 */
        RecordTime_S recordTime; /* 视频时间段 */
    } ChnInfo_S;

    typedef enum class SummaryType
    {
        ORDINARY_TYPE = 0,                /* 普通事件 */
        PERIMETER_INCIDENT_TYPE,          /* 周界事件 */
        BEHAVIOURAL_ANALYSIS_TYPE,        /* 行为分析 */
        SCENE_DETECTION_TYPE,             /* 场景检测 */
        TARGET_DETECTION_TYPE,            /* 目标检测 */
        FACE_CAPTURE_TYPE,                /* 人脸抓拍 */
        BEHAVIOR_MONITORING_TYPE,         /* 行为监管 */
        CLOTHING_COMPLIANCE_TYPE,         /* 穿戴规范 */
        TRAFFIC_BEHAVIOR_MONITORING_TYPE, /* 交通行为监管 */
        ATTRIBUTE_RECOGNITION_TYPE,       /* 属性识别 */
    } SummaryType_E;

    /// @brief 回放布局信息
    typedef struct
    {
        Layout::Type_E enLayout = Layout::Type_E::SCREEN_1; /* 布局类型 */
        int nType = 0;                                      /* 视频类型，0普通，非0事件 */
        std::vector<ChnInfo_S> chnInfos;                    /* 通道播放信息 */

        bool bOrdinaryEnable = true;      /* 是否跳过普通事件数据段 */
        bool bPersonEnable = true;      /* 是否开启人员检测数据段 */
        bool bVehicleEnable = true;      /* 是否开启车辆检测数据段 */

        /* 以下字段同播放使用 */
        std::string date;        /* YYYY-MM-DD需要播放哪天的视频，优先使用 */
        RecordTime_S recordTime; /* 视频时间段 */

        int bSmartVideoSummary;             /* 智能视频摘要开关 */
        std::string strStartTime;   /* 查找事件录像最早时间 */
        std::string strEndTime;     /* 查找事件录像最晚时间 */
        SummaryType_E enSummaryType;
    } LayoutInfo_S;

    /// @brief 播放/暂停
    typedef struct
    {
        Layout::Item_S stItem; /* 通道信息 */

        bool bPlay = true; /* 是否播放 */
    } PlayInfo_S;

    /// @brief 选时信息
    typedef struct
    {
        Layout::Item_S stItem; /* 通道信息 */

        long long nSeek = 0; /* 选时时间 */
    } SeekInfo_S;

    /// @brief 播放控制
    typedef struct
    {
        Layout::Item_S stItem; /* 通道信息 */

        float nSpeed = 0; /* 播放速度 */
    } SpeedInfo_S;

    /// @brief 媒体信息
    typedef struct
    {
        int nChnId = -1;    /* 通道Id */
        bool bPlay = false; /* 是否播放 */
        int nPlayTime = 0;  /* 播放时间 */
        int nSpeed = 0;     /* 播放速度 */
        int nStartTime = 0; /*起始时间*/
        int nTotalTime = 0; /*总时长*/
    } MediaInfo_S;

    /// @brief 锁定信息
    typedef struct
    {
        int nChnId = -1;          /*锁定通道号*/
        bool bLock = false;       /*锁定状态*/
        std::string date;        /*开始日期*/
        int nTimestamp = 0;      /*时间戳*/
        std::string startTime;   /*开始时间*/
        std::string filename;    /*文件名*/
    } LockInfo_S;

    /// @brief 文件信息
    typedef struct
    {
        Layout::Item_S stItem; /* 通道信息 */
        Layout::Rect_S stRect; /* 位置信息 */
        std::string filename;
    } FileInfo_S;

    namespace Stream
    {
        /**
         * @brief 流地址：rtp://host:nPort/replay/主次码流+nChnId/userId
         */
        typedef struct Info
        {
            std::string host = "235.0.0.1";
            int nPort = 5004;
            int nUserId = -1;
            int nChnId = -1;
            bool bMainStream = true;
            std::string uniqueId;
            std::string protocol = "rtp"; /* rtp/raw */
            std::string startTime;
            std::string endTime;
            std::string filename;
            std::vector<std::string> filenames;
            void *pHander = nullptr;
            bool operator<(const Info &other) const
            {
                if (nUserId == -1)
                {
                    return uniqueId < other.uniqueId;
                }
                return nUserId < other.nUserId;
            }
        } Info_S;

        typedef struct CodecInfo
        {
            Record_NS::VideoConfigInfo_S  stVideoInfo;
            Record_NS::AudioConfigInfo_S  stAudioInfo;
        } CodecInfo_S;
        /**
         * @brief 语音对讲信息
         */
        typedef struct ReplayRtpInfo
        {
            std::string nvrIp = "235.0.0.1";
            std::string remoteIp = "235.0.0.1";
            Info_S ipcInfo;
        } ReplayRtpInfo_S;

        typedef struct Ctrl
        {
            float fSpeed = -1;
            long long nSeek = -1;
            int nChnId = -1;
            bool bPlay = false;
            bool bPause = false;
            bool bStop = false;
        } Ctrl_S;

        typedef struct MediaInfo
        {
            int nChnId = -1;     /* 通道Id */
            int nPlayStatus = 0; /* 播放状态 */
            int nPlayTime = 0;   /* 播放时间 */
            float fSpeed = 0;      /* 播放速度 */
            int nStartTime = 0;  /*起始时间*/
            int nTotalTime = 0;  /*总时长*/
        } MediaInfo_S;

        /**
         * @brief 裸盘回放信息
         */
        typedef struct RfsRtpInfo
        {
            int nType;                      /* 回放类型 */
            int nPort = 5004;
            std::string remoteIp = "235.0.0.1";  /* 本地电脑回放Ip*/
            std::vector<int> chnId;        /* 通道Id */
            std::string startTime ="00:00:00"; /* 开始时间 */
            std::string endTime = "23:59:59";   /* 结束时间 */
        } RfsRtpInfo_S;

        typedef struct RfsCtrl
        {
            float fSpeed = -1;
            long long nSeek = -1;
            std::vector<int> chnId;  
            bool bPlay = false;
            bool bPause = false;
            bool bStop = false;
            int nType;                      /* 回放类型 */
            std::string startTime = ""; /* 开始时间 */
            std::string endTime = "";   /* 结束时间 */
        } RfsCtrl_S;

        // 定义返回结构，包含成功和失败的通道ID
        typedef struct InitResult 
        {
            std::vector<int> successChannels;  // 初始化成功的通道ID
            std::vector<int> failedChannels;   // 初始化失败的通道ID
            std::string url;                   // 使用的URL
        }InitResult_S;
    }
} // namespace Replay
