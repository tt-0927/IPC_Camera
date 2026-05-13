/*
 * @Author       : EasonLu
 * @Date         : 2025-02-12 14:33:34
 * @LastEditors  : EasonLu
 * @LastEditTime : 2025-05-09 09:47:14
 * @FilePath     : SipType.h
 * @Description  : sip协议的公共定义
 */
#ifndef _SIP_TYPE_H_
#define _SIP_TYPE_H_

#include "ExternSip.h"
#include "ModuleLog.h"
#include <cstring>
#include <functional>
#include <iostream>
#include <memory>
#include <mutex>
#include <netinet/in.h>
#include <string>
#include <sys/socket.h>
#include <vector>

/* 国标协议的类型定义头文件 */
#include "GbDefine.h"
#include "MANSCDP.h"

#ifndef SIP_SERVER_ID
#define SIP_SERVER_ID "34020000002000000001"
#endif

#ifndef SIP_SERVER_PWD
#define SIP_SERVER_PWD "admin@123"
#endif

#ifndef SIP_SERVER_IP
#define SIP_SERVER_IP "127.0.0.1"
#endif

#ifndef SIP_SERVER_PORT
#define SIP_SERVER_PORT 5060
#endif

#ifndef SIP_SERVER_TIMEOUT
#define SIP_SERVER_TIMEOUT 10
#endif

#ifndef SIP_SERVER_HEARTBEAT
#define SIP_SERVER_HEARTBEAT 60
#endif

#ifndef SIP_SERVER_HEARTBEAT_MAX
#define SIP_SERVER_HEARTBEAT_MAX 3
#endif

/* 注册有效期缺省值为一天，最低不能低于一小时 参考GB/T28181-2022 9.1.11.d */
#ifndef SIP_SERVER_EXPIRES
#define SIP_SERVER_EXPIRES 86400
#endif

/// @brief 上层回调返回操作结果
typedef struct _SipCbResult_S_
{
    int nResult = -1;                /* 0 成功，其他失败 */
    std::string strFailReason = ""; /* 失败原因 */
    _SipCbResult_S_ &operator=(const _SipCbResult_S_ &other)
    {
        if (this != &other)
        {
            nResult = other.nResult;
            strFailReason = other.strFailReason;
        }
        return *this;
    }
} SipCbResult_S;

typedef enum _SipDevStatus_E_
{
    SIP_OFFLINE = 0, /* 已断开 */
    SIP_ONLINE,      /* 已连接 */
    SIP_RECONNECT,   /* 重新连接上 */
} SipDevStatus_E;

/* NOTE 目前只采用PS的封装格式 */

/// @brief 视频格式
/// @note 顺序不能改变，根据SDP中的字段f进行定义
typedef enum _SipVideoType_E_
{
    SIP_VIDEO_NONE = 0, /* 不支持 */
    SIP_VIDEO_MPEG4 = 1,
    SIP_VIDEO_H264,
    SIP_VIDEO_SVAC,
    SIP_VIDEO_3GP,
    SIP_VIDEO_H265,
} SipVideoType_E;

/// @brief 音频格式
/// @note 顺序不能改变，根据SDP中的字段f进行定义
typedef enum _SipAudioType_E_
{
    SIP_AUDIO_NONE = 0, /* 不支持 */
    SIP_AUDIO_G711A,    /* G711A/PCMA */
    SIP_AUDIO_G723,     /* G723 */
    SIP_AUDIO_G729,     /* G729 */
    SIP_AUDIO_G722,     /* G722 */
    SIP_AUDIO_SVAC,     /* SVAC */
    SIP_AUDIO_AAC,      /* AAC */
} SipAudioType_E;

/// @brief 通道编码类型
/// @note 根据国标编码类型 11-13位决定
typedef enum _SipChannelType_E_ 
{
    // 111~130 前端主设备
    DVR_ENCODE = 111,                // DVR编码
    VIDEO_SERVER_ENCODE = 112,       // 视频服务器编码
    ENCODER_ENCODE = 113,            // 编码器编码
    DECODER_ENCODE = 114,            // 解码器编码
    VIDEO_SWITCH_MATRIX_ENCODE = 115, // 视频切换矩阵编码
    AUDIO_SWITCH_MATRIX_ENCODE = 116, // 音频切换矩阵编码
    ALARM_CONTROLLER_ENCODE = 117,   // 报警控制器编码
    NETWORK_VIDEO_RECORDER_NVR_ENCODE = 118, // 网络视频录像机(NVR)编码
    HYBRID_DISK_RECORDER_IVR_ENCODE = 130,   // 混合硬盘录像机(IVR)编码
    FRONT_END_MAIN_DEVICE_EXTEND = 119,       // 119~130 扩展的前端主设备类型（这里选 119 占位，实际可按需细化）

    // 131~199 前端外围设备
    CAMERA_ENCODE = 131,             // 摄像机编码
    NETWORK_CAMERA_IPC_ENCODE = 132, // 网络摄像机(IPC)编码
    DISPLAY_DEVICE_ENCODE = 133,     // 显示设备编码
    ALARM_INPUT_DEVICE_ENCODE = 134, // 报警输入设备编码(如红外、烟感、门禁等报警设备)
    ALARM_OUTPUT_DEVICE_ENCODE = 135, // 报警输出设备编码(如警灯、警铃等设备)
    AUDIO_INPUT_DEVICE_ENCODE = 136, // 拾音输入设备编码
    AUDIO_OUTPUT_DEVICE_ENCODE = 137, // 语音输出设备编码
    MOBILE_TRANSMISSION_DEVICE_ENCODE = 138, // 移动传输设备编码
    OTHER_PERIPHERAL_DEVICE_ENCODE = 139, // 其他外围设备编码
    FRONT_END_PERIPHERAL_DEVICE_EXTEND = 140, // 140~199 扩展的前端外围设备类型（选 140 占位，可细化）

    // 200~299 平台设备
    CENTER_SIGNAL_CONTROL_SERVER_ENCODE = 200, // 中心信令控制服务器编码
    WEB_APPLICATION_SERVER_ENCODE = 201,       // Web应用服务器编码
    MEDIA_DISTRIBUTION_SERVER_ENCODE = 202,    // 媒体分发服务器编码
    PROXY_SERVER_ENCODE = 203,                 // 代理服务器编码
    SECURITY_SERVER_ENCODE = 204,              // 安全服务器编码
    ALARM_SERVER_ENCODE = 205,                 // 报警服务器编码
    DATABASE_SERVER_ENCODE = 206,              // 数据库服务器编码
    GIS_SERVER_ENCODE = 207,                   // GIS服务器编码
    MANAGEMENT_SERVER_ENCODE = 208,            // 管理服务器编码
    ACCESS_GATEWAY_ENCODE = 209,               // 接入网关编码
    MEDIA_STORAGE_SERVER_ENCODE = 210,         // 媒体存储服务器编码
    SIGNAL_SECURITY_ROUTER_GATEWAY_ENCODE = 211, // 信令安全路由网关编码
    BUSINESS_GROUP_ENCODE = 215,               // 业务分组编码
    VIRTUAL_ORGANIZATION_ENCODE = 216,         // 虚拟组织编码
    PLATFORM_DEVICE_EXTEND_212_214_217_299 = 212, // 212~214,217~299 扩展的平台设备类型（选 212 占位，可细化）

    // 300~399 中心用户
    CENTER_USER = 300,                  // 中心用户
    INDUSTRY_ROLE_USER = 301,           // 行业角色用户（301~343 选 301 占位，可细化）
    CENTER_USER_EXTEND = 344,           // 344~399 扩展的中心用户类型（选 344 占位，可细化）

    // 400~499 终端用户
    TERMINAL_USER = 400,                // 终端用户
    INDUSTRY_ROLE_TERMINAL_USER = 401,  // 行业角色用户（401~443 选 401 占位，可细化）
    TERMINAL_USER_EXTEND = 444,         // 444~499 扩展的终端用户类型（选 444 占位，可细化）

    // 500~599 平台外接服务器
    VIDEO_IMAGE_INFO_INTEGRATED_APPLICATION_PLATFORM_SIGNAL_SERVER = 500, // 视频图像信息综合应用平台信令服务器
    VIDEO_IMAGE_INFO_OPERATION_MANAGEMENT_PLATFORM_SIGNAL_SERVER = 501,   // 视频图像信息运维管理平台信令服务器
    PLATFORM_EXTERNAL_SERVER_EXTEND = 502, // 502~599 扩展的平台外接服务器类型（选 502 占位，可细化）

    // 600~999 扩展类
    EXTEND_TYPE = 600,                  // 600~999 扩展类型（选 600 占位，可细化）

    CHANNELTYPE_UNKNOWN = -1                        // 未知类型，可按需调整
}SipChannelType_E;


/// @brief 音频信息
/// @note 通道数均为1,采样率均为8000
typedef struct _SipAudioInfo_S_
{
    SipAudioType_E enType; /* 音频格式 */
    int nSampleRate;       /* 采样率 */
    int nChannel;          /* 声道 */
    _SipAudioInfo_S_()
    {
        enType = SIP_AUDIO_AAC;
        nSampleRate = 48000;
        nChannel = 1;
    }
    /* 重载赋值运算符 */
    _SipAudioInfo_S_ &operator=(const _SipAudioInfo_S_ &stInfo)
    {
        if (this != &stInfo)
        {
            enType = stInfo.enType;
            nSampleRate = stInfo.nSampleRate;
            nChannel = stInfo.nChannel;
        }
        return *this;
    }
} SipAudioInfo_S;

typedef struct _SipMediaCbInfo_S_
{
    std::string strDevID; /* 设备ID */
    std::string strChnID; /* 通道ID */

    const char *pData;
    int nLen;
    uint64_t nTimestamp;
    bool bIsAudio;
    bool bIsKeyFrame;
    uint32_t nStreamType;
} SipMediaCbInfo_S;

/* 音视频数据回调 */
using SipMediaCallBack = std::function<void(const SipMediaCbInfo_S &)>;

typedef struct _SipDeviceInfo_S_
{
    std::string strID;       /* 设备ID */
    std::string strChnID;    /* 设备通道 */
    std::string strCallID;   /* 会话ID——复用此数据结构中的音视频格式信息 */
    std::string strIP;       /* 设备IP */
    int nPort;               /* 设备端口 */
    std::string strName;     /* 设备名称 */
    SipDevStatus_E enStatus; /* 设备状态 */
    SipVideoType_E enVideo;  /* 视频格式 */
    int nWidth;              /* 视频宽度 */
    int nHeight;             /* 视频高度 */
    int nFps;                /* 帧率 */
    SipAudioInfo_S enAudio;  /* 音频信息 */
    /* 默认构造函数 */
    _SipDeviceInfo_S_()
    {
        strID = "";
        strChnID = "";
        strCallID = "";
        strIP = "";
        nPort = 0;
        strName = "";
        enStatus = SIP_OFFLINE;
        enVideo = SIP_VIDEO_H264;
        nWidth = 2880;
        nHeight = 1620;
        nFps = 30;
        enAudio = SipAudioInfo_S();
    }
    /* 重载运算符 */
    _SipDeviceInfo_S_ &operator=(const _SipDeviceInfo_S_ &stInfo)
    {
        if (this != &stInfo)
        {
            strID = stInfo.strID;
            strChnID = stInfo.strChnID;
            strCallID = stInfo.strCallID;
            strIP = stInfo.strIP;
            nPort = stInfo.nPort;
            strName = stInfo.strName;
            enStatus = stInfo.enStatus;
            enVideo = stInfo.enVideo;
            nWidth = stInfo.nWidth;
            nHeight = stInfo.nHeight;
            nFps = stInfo.nFps;
            enAudio = stInfo.enAudio;
        }
        return *this;
    }
} SipDeviceInfo_S;

typedef struct _SipLocalInfo_S_
{
    std::string strDevName;      /* 当前设备名称 */
    std::string strManufacturer; /* 设备生产厂商 */
    std::string strModel;        /* 设备型号 */
    std::string strFirmware;     /* 设备固件版本 */
    /* 重载默认构造函数 */
    _SipLocalInfo_S_()
    {
        strDevName = "";
        strManufacturer = "";
        strModel = "";
        strFirmware = "";
    }
    /* 重载赋值运算符 */
    _SipLocalInfo_S_ &operator=(const _SipLocalInfo_S_ &stInfo)
    {
        if (this != &stInfo)
        {
            strDevName = stInfo.strDevName;
            strManufacturer = stInfo.strManufacturer;
            strModel = stInfo.strModel;
            strFirmware = stInfo.strFirmware;
        }
        return *this;
    }
} SipLocalInfo_S;

/* 获取设备信息回调函数 */
using SipGetLocalInfo = std::function<void(SipLocalInfo_S &)>;

/* 设备信息的回调函数 */
using SipDevInfoObserver = std::function<void(SipDeviceInfo_S &)>;

typedef struct _SipServerInfo_S_
{
    /* 公有配置 */
    int nPort;               /* SIP服务器固定端口 */
    std::string strIP;       /* SIP服务器IP */
    std::string strID;       /* SIP服务器ID */
    std::string strRealm;    /* SIP服务器域名 */
    std::string strPassword; /* SIP服务器密码 */
    std::string strNonce;    /* SIP服务器Nonce */
    std::string strExternIP; /* 客户端可识别的IP地址 */
    /* 客户端配置 */
    bool bTcp;              /* 客户端连接时是否使用TCP，服务器初始化时区分TCP和UDP的监听 */
    int nHeartbeatInterval; /* 心跳周期(单位：秒) */
    int nMaxHeartTimes;     /* 最大心跳超时次数 */
    int nExpires;           /* 注册有效期(单位：秒) */
    /* 默认构造函数 */
    _SipServerInfo_S_()
    {
        nPort = SIP_SERVER_PORT;
        strIP = SIP_SERVER_IP;
        strID = SIP_SERVER_ID;
        /* 域ID为设备ID的前十位 */
        strRealm = std::string(SIP_SERVER_ID).substr(0, 10);
        strPassword = SIP_SERVER_PWD;
        strNonce = "";
        strExternIP = "";
        bTcp = false;
        nHeartbeatInterval = SIP_SERVER_HEARTBEAT;
        nMaxHeartTimes = SIP_SERVER_HEARTBEAT_MAX;
        nExpires = SIP_SERVER_EXPIRES;
    }
    /* 重载赋值运算符 */
    _SipServerInfo_S_ &operator=(const _SipServerInfo_S_ &stInfo)
    {
        if (this != &stInfo)
        {
            nPort = stInfo.nPort;
            strIP = stInfo.strIP;
            strID = stInfo.strID;
            strRealm = stInfo.strRealm;
            strPassword = stInfo.strPassword;
            strNonce = stInfo.strNonce;
            strExternIP = stInfo.strExternIP;
            bTcp = stInfo.bTcp;
            nHeartbeatInterval = stInfo.nHeartbeatInterval;
            nMaxHeartTimes = stInfo.nMaxHeartTimes;
            nExpires = stInfo.nExpires;
        }
        return *this;
    }

    std::string GetUri() const
    {
        if (strID.empty() || strIP.empty())
        {
            return "";
        }
        return std::string("sip:") + strID + "@" + strIP + ":" + std::to_string(nPort);
    }
} SipServerInfo_S;

typedef struct _SipClientInfo_S_
{
    SipServerInfo_S stLocal;  /* 本地服务器的配置 */
    SipServerInfo_S stRemote; /* 连接远端服务器的配置 */
    /* 默认构造函数 */
    _SipClientInfo_S_()
    {
        stLocal = SipServerInfo_S();
        stRemote = SipServerInfo_S();
    }
    /* 重载赋值运算符 */
    _SipClientInfo_S_ &operator=(const _SipClientInfo_S_ &stInfo)
    {
        if (this != &stInfo)
        {
            stLocal = stInfo.stLocal;
            stRemote = stInfo.stRemote;
        }
        return *this;
    }
} SipClientInfo_S;

/// @brief 云台控制类型
/// @note  供外部调用
typedef enum class Sip_Ptz_Type_ : int
{
    RESET = 0,    /* 复位 */
    UP = 1,       /* 上 */
    DOWN,         /* 下 */
    LEFT,         /* 左 */
    RIGHT,        /* 右 */
    TOP_LEFT,     /* 左上 */
    TOP_RIGHT,    /* 右上 */
    LOWER_LEFT,   /* 左下 */
    LOWER_RIGHT,  /* 右下 */
    ZOOM_UP,      /* 放大 */
    ZOOM_DOWN,    /* 缩小 */
    FOCUS_UP,     /* 焦距变大 */
    FOCUS_DOWN,   /* 焦距变小 */
    APERTUR_UP,   /* 光圈扩大 */
    APERTUR_DOWN, /* 光圈缩小 */
    STOP,         /* 停止PTZ操作 */
} SipPtzType_E;

typedef struct _SipPtzInfoCb_S_
{
    int nIndex;
    SipPtzType_E enType;
    int nSpeed = 0;
} SipPtzInfoCb_S;

using SipPtzCmdCb = std::function<void(const SipPtzInfoCb_S &, SipCbResult_S &)>;

/// @brief 预置位类型
/// @note  供外部调用
typedef enum class Sip_Preset_type_ : int
{
    UNKNOWN = 0,
    SET = 0x81,  /* 预置位设置 */
    CALL = 0x82, /* 预置位调用 */
    DEL = 0x83,  /* 预置位删除 */
} SipPresetType_E;

typedef struct _SipPresetInfoCb_S_
{
    int nIndex;
    SipPresetType_E enType;
    int nPresetId;
} SipPresetInfoCb_S;

using SipPresetCb = std::function<void(const SipPresetInfoCb_S &, SipCbResult_S &)>;

/// @brief 通道信息数据结构
typedef struct _SipChannelInfo_S_
{
    int nIndex;                   /* 当前IPC的通道号 */
    std::string strChannelID;     /* 设备编码 */
    std::string strParentID;      /* 父设备编码 */
    std::string strVoiceChnID;    /* 语音设备编码 */
    std::string strName;          /* 设备名称 */
    std::string strManufacturer;  /* 设备厂商 */
    std::string strModel;         /* 设备型号 */
    std::string strOwner;         /* 设备所有者 */
    std::string strCivilCode;     /* 设备区域码 */
    std::string strAddress;       /* 设备地址 */
    std::string strParental;      /* 父设备编码 */
    std::string strRegisterWay;   /* 注册方式 */
    std::string strSecrecy;       /* 设备密级 */
    std::string strStreamNum;     /* 流数量 */
    std::string strExternIP;      /* 设备对外IP-作为客户端时需要设置的对外可连接IP */
    std::string strStatus;        /* 设备状态-字符串格式ON/OFF */
    std::string strNickname;      /* 设备昵称 */
    std::string strSsrc;          /* ssrc */
    std::string strStreamID;      /* 流ID */
    std::string strPtzType;       /* 云台控制类型 */
    std::string strDownloadSpeed; /* 下载速度 */
    SipVideoType_E enVideo;       /* 视频格式 */
    int nVideoWidth;              /* 视频宽度 */
    int nVideoHeight;             /* 视频高度 */
    int nVideoFps;                /* 视频帧率 */
    SipAudioInfo_S enAudio;       /* 音频信息 */
    int nPort;                    /* 设备端口 */
    SipDevStatus_E enStatus;      /* 设备状态 */
    SipChannelType_E enChannelType;/* 通道类型 */
    /* 默认构造函数 */
    _SipChannelInfo_S_()
    {
        nIndex = 0;
        strChannelID = "";
        strParentID = "";
        strName = "";
        strManufacturer = "itv Camera";
        strModel = "Model";
        strOwner = "Owner";
        strCivilCode = "";
        strAddress = "Address";
        strParental = "0";
        strRegisterWay = "1";
        strSecrecy = "0";
        strStreamNum = "";
        strExternIP = "";
        strStatus = "";
        strNickname = "";
        strSsrc = "";
        strStreamID = "";
        strPtzType = "";
        strDownloadSpeed = "";
        enVideo = SIP_VIDEO_NONE;
        nVideoWidth = 0;
        nVideoHeight = 0;
        nVideoFps = 0;
        enAudio.enType = SIP_AUDIO_NONE;
        enAudio.nSampleRate = 8000;
        enAudio.nChannel = 1;
        nPort = 0;
        enStatus = SIP_ONLINE;
        enChannelType = CHANNELTYPE_UNKNOWN;
    }
    /* 重载赋值运算符 */
    _SipChannelInfo_S_ &operator=(const _SipChannelInfo_S_ &stInfo)
    {
        if (this != &stInfo)
        {
            nIndex = stInfo.nIndex;
            strChannelID = stInfo.strChannelID;
            strParentID = stInfo.strParentID;
            strName = stInfo.strName;
            strManufacturer = stInfo.strManufacturer;
            strModel = stInfo.strModel;
            strOwner = stInfo.strOwner;
            strCivilCode = stInfo.strCivilCode;
            strAddress = stInfo.strAddress;
            strParental = stInfo.strParental;
            strRegisterWay = stInfo.strRegisterWay;
            strSecrecy = stInfo.strSecrecy;
            strStreamNum = stInfo.strStreamNum;
            strExternIP = stInfo.strExternIP;
            strStatus = stInfo.strStatus;
            strNickname = stInfo.strNickname;
            strSsrc = stInfo.strSsrc;
            strStreamID = stInfo.strStreamID;
            strPtzType = stInfo.strPtzType;
            strDownloadSpeed = stInfo.strDownloadSpeed;
            enVideo = stInfo.enVideo;
            nVideoHeight = stInfo.nVideoWidth;
            nVideoHeight = stInfo.nVideoHeight;
            nVideoFps = stInfo.nVideoFps;
            enAudio = stInfo.enAudio;
            nPort = stInfo.nPort;
            enStatus = stInfo.enStatus;
            enChannelType = stInfo.enChannelType;
        }
        return *this;
    }
} SipChannelInfo_S;

/// @brief 媒体数据收发状态回调数据结构
typedef struct _Sip_Media_Status_S_
{
    /* 公用数据 */
    bool bStart;          /* 点播状态 */
    int nIndex;           /* 通道号 */
    std::string strDevID; /* 重新编码的设备ID */
    std::string strChnID; /* 重新编码的通道ID */
    _Sip_Media_Status_S_()
    {
        bStart = false;
        nIndex = -1;
        strDevID = "";
        strChnID = "";
    }
    _Sip_Media_Status_S_ &operator=(const _Sip_Media_Status_S_ &stInfo)
    {
        if (this != &stInfo)
        {
            bStart = stInfo.bStart;
            nIndex = stInfo.nIndex;
            strDevID = stInfo.strDevID;
            strChnID = stInfo.strChnID;
        }
        return *this;
    }
} SipMediaStatus_S;
/* 媒体数据收发状态回调函数 */
using SipMediaStatusCb = std::function<void(const SipMediaStatus_S &)>;

/// @brief 查询录制文件的查询条件
/// @note  针对上层应用的数据结构设计
typedef struct _SipQueryRecCondition_S_
{
    /* 适配上层应用的数据字段 */
    int nIndex;               /* 通道号 */
    std::string strStartDate; /* 开始时间——只有日期，不带时间和T */
    std::string strEndDate;   /* 结束时间——只有日期，不带时间和T */
    /* 检索可选字段——暂不实现 */
    /* 重载赋值运算符 */
    _SipQueryRecCondition_S_ &operator=(const _SipQueryRecCondition_S_ &stInfo)
    {
        if (this != &stInfo)
        {
            nIndex = stInfo.nIndex;
            strStartDate = stInfo.strStartDate;
            strEndDate = stInfo.strEndDate;
        }
        return *this;
    }
} SipQueryRecCondition_S;

/// @brief 查询录制文件的单个结果
/// @note  针对上层应用的数据结构设计
typedef struct _SipQueryRecResultItem_S_
{
    std::string strCreateTime; /* 文件创建时间，不带T的完整日期和时间 */
    std::string strModifyTime; /* 文件修改时间，不带T的完整日期和时间 */
    int nDuration;             /* 文件时长 */
    uint64_t nFileSize;        /* 文件大小 */
    /* 重载赋值运算符 */
    _SipQueryRecResultItem_S_ &operator=(const _SipQueryRecResultItem_S_ &stInfo)
    {
        if (this != &stInfo)
        {
            strCreateTime = stInfo.strCreateTime;
            strModifyTime = stInfo.strModifyTime;
            nDuration = stInfo.nDuration;
            nFileSize = stInfo.nFileSize;
        }
        return *this;
    }
} SipQueryRecResultItem_S;

/// @brief 查询录制文件的结果
/// @note  针对上层应用的数据结构设计
typedef struct _SipQueryRecResult_S_
{
    int nResult = 0; /* 查询结果返回值——小于0为失败 */
    std::vector<SipQueryRecResultItem_S> vecResult;
} SipQueryRecResult_S;

/* 查询录制文件的回调 */
using SipQueryRecordInfoCb = std::function<void(const SipQueryRecCondition_S &, SipQueryRecResult_S &)>;

typedef enum _SipReadFileActionType_E_
{
    SIP_READFILE_START = 0, /* 开始读取文件 */
    SIP_READFILE_STOP,      /* 停止读取文件 */
    SIP_READFILE_PAUSE,     /* 暂停读取文件 */
    SIP_READFILE_RESUME,    /* 恢复读取文件 */
    SIP_READFILE_SEEK,      /* 跳转读取文件 */
    SIP_READFILE_SPEED,     /* 设置读取倍速 */
    SIP_READFILE_BACKWARDS, /* 倒放读取文件 */
    SIP_READFILE_UNKNOWN,   /* 未知操作 */
} SipReadFileActionType_E;

/// @brief 读取文件的操作
typedef struct _SipReadFileAction_S_
{
    /* 操作枚举 */
    std::string strCallID; /* 会话ID——唯一标识 */
    /* 默认为未知操作 */
    SipReadFileActionType_E enAction = SIP_READFILE_UNKNOWN;
    int nSeekTime = 0;          /* 快进/快退的秒数——SIP_READFILE_SEEK时生效 */
    double dSpeed = 1.0;        /* 倍速——SIP_READFILE_SPEED时生效 */
    /* NOTE 倒放时仅支持1倍速倒放 */
    int nBackwardStartTime = 0; /* 倒放开始时间——SIP_READFILE_BACKWARDS时生效 */
    int nBackwardEndTime = 0;   /* 倒放结束时间——SIP_READFILE_BACKWARDS时生效 */
    int nCSeq = 0;              /* 操作指令序列号 */
    /* 重载赋值运算符 */
    _SipReadFileAction_S_ &operator=(const _SipReadFileAction_S_ &stInfo)
    {
        if (this != &stInfo)
        {
            strCallID = stInfo.strCallID;
            enAction = stInfo.enAction;
            nSeekTime = stInfo.nSeekTime;
            dSpeed = stInfo.dSpeed;
            nBackwardStartTime = stInfo.nBackwardStartTime;
            nBackwardEndTime = stInfo.nBackwardEndTime;
            nCSeq = stInfo.nCSeq;
        }
        return *this;
    }
} SipReadFileAction_S;

/// @brief 读取文件操作的回调
using SipReadFileActionCb = std::function<void(const SipReadFileAction_S &, SipCbResult_S &)>;

/// @brief 设置回放和下载的读取文件数据结构
typedef struct _SipGetRecFile_S_
{
    /* NOTE 时间均是采用1970开始的计算的秒数 */
    std::string strCallID; /* 会话ID——唯一标识 */
    int nIndex;            /* 对应上层的通道号 */
    uint64_t nStartTime;   /* 开始时间 */
    uint64_t nEndTime;     /* 结束时间 */
    /* 重载赋值运算符 */
    _SipGetRecFile_S_ &operator=(const _SipGetRecFile_S_ &stInfo)
    {
        if (this != &stInfo)
        {
            strCallID = stInfo.strCallID;
            nIndex = stInfo.nIndex;
            nStartTime = stInfo.nStartTime;
            nEndTime = stInfo.nEndTime;
        }
        return *this;
    }
    /* 重载比较运算符 */
    bool operator==(const _SipGetRecFile_S_ &stInfo) const
    {
        return (strCallID == stInfo.strCallID &&
                nIndex == stInfo.nIndex &&
                nStartTime == stInfo.nStartTime &&
                nEndTime == stInfo.nEndTime);
    }
    /* 重载不等于运算符 */
    bool operator!=(const _SipGetRecFile_S_ &stInfo) const { return !(*this == stInfo); }
} SipGetRecFile_S;

/* 设置回放和下载所的读取文件的回调 */
using SipGetRecFileCb = std::function<void(const SipGetRecFile_S &, SipCbResult_S &)>;

/////////////////////////////////////////////设备配置/////////////////////////////////////////////
/* 设备配置查询-基本参数配置 */
using SipBasicParamCb = std::function<void(GB28181::BasicParamInfo_S &, SipCbResult_S &)>;

/* 设备配置查询-视频参数范围配置类型*/
using SipVideoParamOptCb = std::function<void(GB28181::VideoParamOptInfo_S &, SipCbResult_S &)>;

/* 设备配置查询-SVAC编码配置*/
using SipSVACEncodeCb = std::function<void(GB28181::SVACEncodeInfo_S &, SipCbResult_S &)>;

/* 设备配置查询-SVAC解码配置*/
using SipSVACDncodeCb = std::function<void(GB28181::SVACDecodeInfo_S &, SipCbResult_S &)>;

/* 设备配置查询-视频参数属性配置*/
using SipVideoAttributeCb = std::function<void(GB28181::VideoParamAttributeInfo_S &, SipCbResult_S &)>;

/* 设备配置查询-录像计划配置*/
using SipRecordPlanCb = std::function<void(GB28181::VideoRecordPlanInfo_S &, SipCbResult_S &)>;

/* 设备配置查询-报警录像配置*/
using SipAlarmRecordCb = std::function<void(GB28181::VideoAlarmRecordInfo_S &, SipCbResult_S &)>;

/* 设备配置查询-视频画面遮挡配置*/
using SipPictureMaskCb = std::function<void(GB28181::PictureMaskInfo_S &, SipCbResult_S &)>;

/* 设备配置查询-视频画面翻转配置*/
using SipFrameMirrorCb = std::function<void(GB28181::FrameMirrorInfo_S &, SipCbResult_S &)>;

/* 设备配置查询-报警上报开关配置*/
using SipAlarmReportCb = std::function<void(GB28181::AlarmReportInfo_S &, SipCbResult_S &)>;

/* 设备配置查询-OSD配置*/
using SipOSDConfigCb = std::function<void(GB28181::OSDConfig_S &, SipCbResult_S &)>;

/* 设备配置查询-图像抓拍配置*/
using SipSnapShotCb = std::function<void(GB28181::SnapShotConfigInfo_S &, SipCbResult_S &)>;

/* 巡航轨迹查询*/
using SipCruiseTrackQueryCb = std::function<void(GB28181::CruiseTrackQueryInfo_S &, SipCbResult_S &)>;

/* 巡航轨迹列表查询*/
using SipCruiseTrackListQueryCb = std::function<void(GB28181::CruiseTrackListQueryInfo_S &, SipCbResult_S &)>;

/*设备预置位查询*/
using SipPresetQueryCb = std::function<void(GB28181::PresetQueryInfo_S &, SipCbResult_S &)>;

/*语音广播*/
using SipBroadcastCb = std::function<void(GB28181::BroadcastInfo_S &, SipCbResult_S &)>;

/*NVR设备校时*/
using SipNVRSetTimeCb = std::function<void(std::string &time, SipCbResult_S &)>;

/*NVR远程启动*/
using SipNVRTeleBootCb = std::function<void(std::string &RemoteStart, SipCbResult_S &)>;

/*强制关键帧*/
using SipIFrameCmdCb = std::function<void(GB28181::IFrame_S &, SipCbResult_S &)>;

/*拉框放大/缩小*/
using SipDragZoomInOutCb = std::function<void(GB28181::DragZoomInfo_S &, SipCbResult_S &)>;

/*PTZ精准控制*/
using SipPTZPreciseCtrlCb = std::function<void(GB28181::PTZPreciseCtrl_S &, SipCbResult_S &)>;

/*录像控制*/
using SipRecordCmdCb = std::function<void(GB28181::RecordCmd_S &, SipCbResult_S &)>;

/*设备控制应答*/
using SipControlResultsCb = std::function<void(GB28181::ControlResults_S &, SipCbResult_S &)>;

/*报警复位控制命令*/
using SipAlarmCmdCb = std::function<void(GB28181::AlarmCmd_S &, SipCbResult_S &)>;

/*看守位*/
using SipHomePositionCb = std::function<void(GB28181::HomePositionInfo_S &, SipCbResult_S &)>;

/*设备软件升级*/
using SipDeviceUpgradeCb = std::function<void(GB28181::DeviceUpgradeInfo_S &, SipCbResult_S &)>;

/*目标跟踪*/
using SipTargetTrackCb = std::function<void(GB28181::TargetTrackInfo_S &, SipCbResult_S &)>;

/*PTZ精确状态查询*/
using SipPTZPositionCb = std::function<void(GB28181::PTZPositionInfo_S &, SipCbResult_S &)>;

/*图像传输完成通知*/
using SipUploadSnapShotFiniCb = std::function<void(GB28181::UploadSnapShotFiniInfo_S &, SipCbResult_S &)>;

/* 客户端上线状态的回调 */
using SipOnlineStatusCb = std::function<void(GB28181::GB28181ClientStatus_E &, SipCbResult_S &)>;

namespace SIP
{
    typedef struct _CbInfo_S_
    {
        /* 设备信息及状态更新回调函数——服务器主要使用 */
        SipDevInfoObserver fnDevUpdate = nullptr;
        /* 媒体数据回调——服务器主要使用 */
        SipMediaCallBack fnMediaUpdate = nullptr;
        /* 媒体数据收发状态——客户端主要使用 */
        SipMediaStatusCb fnMediaStatus = nullptr;
        /* 查询录像文件回调——客户端主要使用 */
        SipQueryRecordInfoCb fnQueryRecordInfo = nullptr;
        /* 设置回放和下载的文件记录回调——客户端主要使用 */
        SipGetRecFileCb fnGetRecFile = nullptr;
        /* 操作回放和下载的文件回调——客户端主要使用 */
        SipReadFileActionCb fnReadFileAction = nullptr;
        /* PtzCmd控制的回调——客户端主要使用 */
        SipPtzCmdCb fnPtzCmd = nullptr;
        /* 预置位的回调——客户端主要使用 */
        SipPresetCb fnPreset = nullptr;
        /* 本地设备信息回调-底层获取信息时回调此函数 */
        SipGetLocalInfo fnGetLocalInfo = nullptr;
        /* 日志回调函数——公用 */
        ModuleLog fnLog = nullptr;
        /* 设备上线状态——客户端主要使用 */
        SipOnlineStatusCb fnOnlineStatus = nullptr;
//设备配置
        /* 设备配置控制-基本参数配置——客户端主要使用 */
        SipBasicParamCb fnBasicParam = nullptr;
        /* 设备配置控制-视频参数范围配置类型——客户端主要使用 */
        SipVideoParamOptCb fnVideoParamOpt = nullptr;
        /* 设备配置控制-SVAC编码配置——客户端主要使用 */
        SipSVACEncodeCb fnSVACEncode = nullptr;
        /* 设备配置控制-SVAC解码配置——客户端主要使用 */
        SipSVACDncodeCb fnSVACDncode = nullptr;
        /* 设备配置控制-视频参数属性配置——客户端主要使用 */
        SipVideoAttributeCb fnVideoAttribute = nullptr;
        /* 设备配置控制-录像计划配置——客户端主要使用 */
        SipRecordPlanCb fnRecordPlan = nullptr;
        /* 设备配置控制-报警录像配置——客户端主要使用 */
        SipAlarmRecordCb fnAlarmRecord = nullptr;
        /* 设备配置控制-视频画面遮挡配置——客户端主要使用 */
        SipPictureMaskCb fnPictureMask = nullptr;
        /* 设备配置控制-视频画面翻转配置——客户端主要使用 */
        SipFrameMirrorCb fnFrameMirror = nullptr;
        /* 设备配置控制-报警上报开关配置——客户端主要使用 */
        SipAlarmReportCb fnAlarmReport = nullptr;
        /* 设备配置控制-OSD配置——客户端主要使用 */
        SipOSDConfigCb fnOSDConfig = nullptr;
        /* 设备配置控制-图像抓拍配置——客户端主要使用 */
        SipSnapShotCb fnSnapShot = nullptr;

        SipCruiseTrackQueryCb fnCruiseTrackQuery = nullptr;
        SipCruiseTrackListQueryCb fnCruiseTrackListQuery = nullptr;
        SipPresetQueryCb fnPresetQuery = nullptr;
        SipBroadcastCb fnBroadcast = nullptr;
        SipNVRSetTimeCb fnSetTime = nullptr;
        SipNVRTeleBootCb fnNVRTeleBoot = nullptr;
        SipIFrameCmdCb fnIFrameCmd = nullptr;
        SipDragZoomInOutCb fnDragZoomInOut = nullptr;
        SipPTZPreciseCtrlCb fnPTZPreciseCtrl = nullptr;
        SipRecordCmdCb fnRecordCmd = nullptr;
        SipControlResultsCb fnControlResults = nullptr;
        SipAlarmCmdCb fnAlarmCmd = nullptr;
        SipHomePositionCb fnHomePosition = nullptr;
        SipDeviceUpgradeCb fnDeviceUpgrade = nullptr;
        SipTargetTrackCb fnTargetTrack =  nullptr;
        SipPTZPositionCb fnPTZPosition = nullptr;
        SipUploadSnapShotFiniCb fnUploadSnapShotFini = nullptr;

        /* 重载赋值运算符 */
        _CbInfo_S_ &operator=(const _CbInfo_S_ &stInfo)
        {
            if (&stInfo != this)
            {
                fnDevUpdate = stInfo.fnDevUpdate;
                fnMediaUpdate = stInfo.fnMediaUpdate;
                fnMediaStatus = stInfo.fnMediaStatus;
                fnQueryRecordInfo = stInfo.fnQueryRecordInfo;
                fnGetRecFile = stInfo.fnGetRecFile;
                fnReadFileAction = stInfo.fnReadFileAction;
                fnPtzCmd = stInfo.fnPtzCmd;
                fnPreset = stInfo.fnPreset;
                fnGetLocalInfo = stInfo.fnGetLocalInfo;
                fnLog = stInfo.fnLog;

                fnBasicParam = stInfo.fnBasicParam;
                fnVideoParamOpt = stInfo.fnVideoParamOpt;
                fnSVACEncode = stInfo.fnSVACEncode;
                fnSVACDncode = stInfo.fnSVACDncode;
                fnVideoAttribute = stInfo.fnVideoAttribute;
                fnRecordPlan = stInfo.fnRecordPlan;
                fnAlarmRecord = stInfo.fnAlarmRecord;
                fnPictureMask = stInfo.fnPictureMask;
                fnFrameMirror = stInfo.fnFrameMirror;
                fnAlarmReport = stInfo.fnAlarmReport;
                fnOSDConfig = stInfo.fnOSDConfig;
                fnSnapShot = stInfo.fnSnapShot;

                fnCruiseTrackQuery = stInfo.fnCruiseTrackQuery;
                fnCruiseTrackListQuery = stInfo.fnCruiseTrackListQuery;

                fnPresetQuery = stInfo.fnPresetQuery;

                fnBroadcast = stInfo.fnBroadcast;

                fnSetTime = stInfo.fnSetTime;
                fnNVRTeleBoot = stInfo.fnNVRTeleBoot;
                fnIFrameCmd = stInfo.fnIFrameCmd;
                fnDragZoomInOut = stInfo.fnDragZoomInOut;
                fnPTZPreciseCtrl = stInfo.fnPTZPreciseCtrl;
                fnRecordCmd = stInfo.fnRecordCmd;
                fnControlResults = stInfo.fnControlResults;
                fnAlarmCmd = stInfo.fnAlarmCmd;
                fnHomePosition = stInfo.fnHomePosition;
                fnDeviceUpgrade = stInfo.fnDeviceUpgrade;
                fnTargetTrack = stInfo.fnTargetTrack;
                fnPTZPosition = stInfo.fnPTZPosition;
                fnUploadSnapShotFini = stInfo.fnUploadSnapShotFini;
                fnOnlineStatus = stInfo.fnOnlineStatus;
            }
            return *this;
        }
    } CbInfo_S;
} // namespace SIP

#endif /* #ifndef _SIP_TYPE_H_ */