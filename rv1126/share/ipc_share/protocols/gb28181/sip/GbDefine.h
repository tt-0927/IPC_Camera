/*
 * @Author       : EasonLu
 * @Date         : 2025-02-17 20:15:22
 * @LastEditors  : EasonLu
 * @LastEditTime : 2025-02-26 19:28:41
 * @FilePath     : GbDefine.h
 * @Description  : 国标协议中的类型转换定义
 */
#ifndef _GB_DEFINE_H_
#define _GB_DEFINE_H_
#include <cstdint>
#include <functional>
#include <string>
#include <vector>
#include <map>
#include <cstring>
namespace GB28181
{
    /// @brief GB28181客户端状态枚举
    typedef enum class _GB28181ClientStatus_E_
    {
        OFFLINE = 0,    /* 离线 */ 
        ONLINE = 1,     /* 在线 */ 
        REGISTERING = 2 /* 注册中 */ 
    }GB28181ClientStatus_E;

    /// @brief 报警级别
    typedef enum _AlarmPriority_E_
    {
        PRIORITY_ALL = 0, /* 全部警情 */
        PRIORITY_1,       /* 一级警情 */
        PRIORITY_2,       /* 二级警情 */
        PRIORITY_3,       /* 三级警情 */
        PRIORITY_4,       /* 四级警情 */
        PRIORITY_UNKNOWN, /* 未知级别 */
    } AlarmPriority_E;

    /// @brief 报警方式条件
    typedef enum _AlarmMethod_E_
    {
        METHOD_ALL = 0,   /* 全部报警方式 */
        METHOD_CALL,      /* 电话报警 */
        METHOD_DEVICE,    /* 设备报警 - 详情参考AlarmDeviceType_E枚举*/
        METHOD_SMS,       /* 短信报警 */
        METHOD_GPS,       /* GPS报警 */
        METHOD_VIDEO,     /* 视频报警 - 详情参考AlarmVideoType_E枚举*/
        METHOD_DEV_FAULT, /* 设备故障报警 - 详情参考AlarmDevFaultType_E枚举 */
        METHOD_OTHER,     /* 其他报警 */
        METHOD_UNKNOWN    /* 未知报警方式 */
    } AlarmMethod_E;

    /// @brief 设备报警类型
    typedef enum _AlarmDeviceType_E_
    {
        DEVICE_VIDEO_LOST = 1, /* 视频丢失 */
        DEVICE_DEVICE_REMOVE,  /* 设备防拆 */
        DEVICE_DISK_FULL,      /* 磁盘满 */
        DEVICE_HIGH_TEMP,      /* 温度过高 */
        DEVICE_LOW_TEMP,       /* 温度过低 */
    } AlarmDeviceType_E;

    /// @brief 视频报警类型
    typedef enum class _AlarmVideoType_E_ : int
    {
        MANUAL = 1,         /* 人工视频报警 */
        DETECT_MOVE,        /* 运动目标侦测 */
        UNATTENDED_BAGGAGE, /* 物品遗留侦测 */
        ATTENDED_BAGGAGE,   /* 物品拾取侦测 */
        TRIP_WIRE,          /* 绊线检测 */
        INTRUSION,          /* 区域入侵 */
        REGROGRADE,         /* 逆行检测 */
        LOITERING,          /* 徘徊检测 */
        FLOW,               /* 流量检测 */
        DENSITY,            /* 密度检测 */
        ERROR,              /* 视频异常 */
        RAPID_MOVE,         /* 快速移动侦测 */
        HIDE,               /* 图像遮挡 */
    } AlarmVideoType_E;

    /// @brief 设备故障报警类型
    typedef enum _AlarmDevFaultType_E_
    {
        FAULT_DISK = 1, /* 磁盘故障 */
        FAULT_FAN,      /* 风扇故障 */
    } AlarmDevFaultType_E;

    typedef struct _AlarmInfo_S_
    {
        int nIndex;                 /* 上层调用的通道号 */
        AlarmPriority_E enPriority; /* 报警等级 */
        AlarmMethod_E enMethod;     /* 报警方法 */
        int64_t nTime;              /* 报警时间——Unix时间，从1970开始 */
        int enType;                 /* 报警类型-根据不同报警方法参考不同的枚举定义,0为未知类型 */
        int enTypeParam;            /* 部分报警类型存在二级参数 */
        /* 默认构造函数 */
        _AlarmInfo_S_()
        {
            nIndex = 0;
            enPriority = PRIORITY_UNKNOWN;
            enMethod = METHOD_UNKNOWN;
            nTime = 0;
            enType = 0;
            enTypeParam = 0;
        }
        /* 重载赋值运算符 */
        _AlarmInfo_S_ &operator=(const _AlarmInfo_S_ &other)
        {
            if (this != &other)
            {
                nIndex = other.nIndex;
                enPriority = other.enPriority;
                enMethod = other.enMethod;
                nTime = other.nTime;
                enType = other.enType;
                enTypeParam = other.enTypeParam;
            }
            return *this;
        }
    } AlarmInfo_S;

    /**
    *@brief 编码格式枚举
    */
    typedef enum _IPCVideoEncodingType_
    {
        VIDEO_ENCODING_H264, /* H264编码 */
        VIDEO_ENCODING_H265, /* H265编码 */
    } IPCVideoEncodingType_E;

    typedef enum class SampleFormat
    {
        S16_LE = 0,
        F32_LE,
        U8
    } SampleFormat_E;

    typedef enum class AudioCodec
    {
        G722 = 0,
        G711_U,
        G711_A,
        MP2L2,
        G276,
        AAC,
        PCM,
        AAC_LC,
        AAC_LD,
        Opus,
        MP3,
        SVAC,
        G723,
        G729,
        G726,
        NONE
    } AudioCodec_E;

        /// @brief GB28181的编码器信息
    typedef struct GB28181CodecInfo_S
    {
        IPCVideoEncodingType_E enVideoType;
        /*分辨率的宽*/
        int nWidth;
        /*分辨率的高*/
        int nHeight;
        /*帧率*/
        int nFps;
        SampleFormat_E enFormat = SampleFormat::S16_LE; /* 默认采样格式为16位 */
        AudioCodec_E enAudioType;
        int nAudioSampleRate;
        int nAudioChannel;
        /* 重载赋值运算符 */
        GB28181CodecInfo_S &operator=(const GB28181CodecInfo_S &other)
        {
            if (this != &other)
            {
                enVideoType        = other.enVideoType;
                nWidth             = other.nWidth;
                nHeight            = other.nHeight;
                nFps               = other.nFps;
                enFormat           = other.enFormat;
                enAudioType        = other.enAudioType;
                nAudioSampleRate   = other.nAudioSampleRate;
                nAudioChannel      = other.nAudioChannel;
            }
            return *this;
        }
        /* 重载==运算符 */
        bool operator==(const GB28181CodecInfo_S &other) const
        {
            return (enVideoType      == other.enVideoType &&
                nWidth           == other.nWidth &&
                nHeight          == other.nHeight &&
                nFps             == other.nFps &&
                enFormat         == other.enFormat &&
                enAudioType      == other.enAudioType &&
                nAudioSampleRate == other.nAudioSampleRate &&
                nAudioChannel    == other.nAudioChannel);
        }
        /* 重载!=运算符 */
        bool operator!=(const GB28181CodecInfo_S &other) const
        {
            return !(*this == other);
        }
    } GB28181CodecInfo_S;

    typedef struct _AlarmCbData_S_
    {
        uint64_t nSN;               /* 报警序号 */
        std::string strID;          /* 设备ID  */
        std::string strIP;          /* 设备IP */
        AlarmPriority_E enPriority; /* 报警等级 */
        AlarmMethod_E enMethod;     /* 报警方法 */
        std::string strTime;        /* 报警时间 */
        int64_t nTime;              /* 报警时间 */
        int enType;                 /* 报警类型-根据不同报警方法参考不同的枚举定义,0为未知类型 */
        int enTypeParam;            /* 部分报警类型存在二级参数 */
        /* 默认构造函数 */
        _AlarmCbData_S_()
        {
            nSN = 0;
            strID = "";
            strIP = "";
            enPriority = PRIORITY_UNKNOWN;
            enMethod = METHOD_UNKNOWN;
            strTime = "";
            nTime = 0;
            enType = 0;
            enTypeParam = 0;
        }
        /* 重载赋值运算符 */
        _AlarmCbData_S_ &operator=(const _AlarmCbData_S_ &other)
        {
            if (this != &other)
            {
                nSN = other.nSN;
                strID = other.strID;
                strIP = other.strIP;
                enPriority = other.enPriority;
                enMethod = other.enMethod;
                strTime = other.strTime;
                nTime = other.nTime;
                enType = other.enType;
                enTypeParam = other.enTypeParam;
            }
            return *this;
        }
    } AlarmCbData_S;

    /* @brief 报警信息观察者 */
    using AlarmObserver = std::function<void(const AlarmCbData_S &)>;

    typedef struct _AlarmSubscribe_S_
    {
        AlarmPriority_E enStartLevel; /* 开始警情级别 */
        AlarmPriority_E enEndLevel;   /* 结束警情级别 */
        AlarmMethod_E enMethod;       /* 报警方式 */
        uint64_t enStartTime;         /* 报警发生的起始时间 */
        uint64_t enEndTime;           /* 报警发生的结束时间 */
        AlarmObserver fnObserver;     /* 报警信息观察者 */
        /* 默认构造函数 */
        _AlarmSubscribe_S_()
        {
            enStartLevel = PRIORITY_ALL;
            enEndLevel = PRIORITY_ALL;
            enMethod = METHOD_ALL;
            /* 时间为0时则不限时的订阅 */
            enStartTime = 0;
            enEndTime = 0;
            fnObserver = nullptr;
        }
        /* 重载赋值运算符 */
        _AlarmSubscribe_S_ &operator=(const _AlarmSubscribe_S_ &other)
        {
            if (this != &other)
            {
                enStartLevel = other.enStartLevel;
                enEndLevel = other.enEndLevel;
                enMethod = other.enMethod;
                enStartTime = other.enStartTime;
                enEndTime = other.enEndTime;
                fnObserver = other.fnObserver;
            }
            return *this;
        }
    } AlarmSubscribe_S;

    /// @brief 设备类型枚举
    /// @note  用于ID的第11~13位的描述
    /// @note  参考GB/T+28181-2022 附录E.1编码规则
    typedef enum class _DevType_E_ : int
    {
        /* 11~130 表示为前端主设备 */
        MAIN_DVR = 111,          /* DVR */
        MAIN_VIDEO_SERVER = 112, /* 视频服务器 */
        MAIN_ENC = 113,          /* 编码器 */
        MAIN_DEC = 114,          /* 解码器 */
        MAIN_VIDEO_MATRIX = 115, /* 视频切换矩阵 */
        MAIN_AUDIO_MATRIX = 116, /* 音频切换矩阵 */
        MAIN_ALARM_CTRL = 117,   /* 报警控制器 */
        MAIN_NVR = 118,          /* NVR-网络视频录像机 */
        /* 119为预留字段 */
        MAIN_IPC_SYSTEM = 120,       /* 在线视频图像信息采集系统 */
        MAIN_VIDEO_CHECKPIONT = 121, /* 视频卡口 */
        MAIN_MULTI_CAMERA = 122,     /* 多目摄像头 */
        MAIN_PARK_CTRL = 123,        /* 停车场出入口控制设备 */
        MAIN_EXIST_CTRL = 124,       /* 人员出入口控制设备 */
        MAIN_SECURITY = 125,         /* 安防设备 */
        /* 126 ~ 129 扩展的前端主设备类型 */
        MAIN_EXTEND_START = 126,
        MAIN_EXTEND_END = 129,
        MAIN_HVR = 130, /* HVR-混合硬盘录像机 */

        /* 131 ~ 199 表示为前端外围设备 */
        PERIPHERY_CAMERA = 131,              /* 摄像机 */
        PERIPHERY_IPC = 132,                 /* IPC-网络摄像机/在线视频图像信息采集设备 */
        PERIPHERY_MONITOR = 133,             /* 显示器 */
        PERIPHERY_ALARM_INPUT = 134,         /* 报警输入设备(如红外、烟感、门禁等设备) */
        PERIPHERY_ALARM_OUTPUT = 135,        /* 报警输出设备(如报警灯、喇叭等设备) */
        PERIPHERY_VOICE_INPUT = 136,         /* 语音输入设备 */
        PERIPHERY_VOICE_OUTPUT = 137,        /* 语音输出设备 */
        PERIPHERY_MOBILE_TRANSMISSION = 138, /* 移动传输设备 */
        PERIPHERY_OTHER = 139,               /* 其他外围设备 */
        PERIPHERY_ALARM_OUTPUT_CTRL = 140,   /* 报警输出设备(如继电器或触发器控制的设备) */
        PERIPHERY_CAR_GATE = 141,            /* 道闸(控制车辆通行) */
        PERIPHERY_SMART_DOOR = 142,          /* 智能门(控制人员通行) */
        PERIPHERY_RECOGNITION_UNIT = 143,    /* 凭证识别单元 */
        /* 144 ~ 199 扩展的前端外围设备类型 */
        PERIPHERY_EXTEND_START = 144,
        PERIPHERY_EXTEND_END = 199,

        /* TODO 后续再添加额外的设备枚举 */
    } DevType_E;

    /// @brief 网络标识编码
    /// @note  占ID的第14位
    typedef enum class _NetType_E_ : int
    {
        DEFAULT = 0,                /* 默认值也为公安视频传输 */
        PUBLIC_SECURITY_VIDEO = 1,  /* 公安视频传输 */
        INDUSTRY_SPECIFIC = 2,      /* 行业专网 */
        POLITICS_AND_LAW = 3,       /* 政法信息网 */
        PUBLIC_SECURITY_MOBILE = 4, /* 公安移动信息网 */
        PUBLIC_SECURITY = 5,        /* 公安信息网 */
        E_GOVERNMENT = 6,           /* 电子政务外网 */
        PUBLIC_NETWORK = 7,         /* 互联网等公共网络 */
        DEDICATED_LINE = 8,         /* 专线 */
        EXTEND = 9,                 /* 预留字段 */
    } NetType_E;

    /// @note Gb视频类型
    typedef enum class _GbVideoType_E_:int
    {
        GB_VIDEO_NONE = 0, /* 不支持 */
        GB_VIDEO_MPEG4 = 1,
        GB_VIDEO_H264 = 2,
        GB_VIDEO_SVAC = 3,
        GB_VIDEO_3GP = 4,
        GB_VIDEO_H265 = 5,
    }GbVideoType_E;

    /// @note Gb分辨率类型
    typedef enum class _GbResolutionType_E_:int
    {
        GB_RESOLUTION_NONE = 0, /* 不支持 */
        GB_RESOLUTION_QCIF = 1,
        GB_RESOLUTION_CIF  = 2,
        GB_RESOLUTION_4CIF = 3,
        GB_RESOLUTION_D1   = 4,
        GB_RESOLUTION_720P = 5,
        GB_RESOLUTION_1080P = 6,
    } GbResolutionType_E;

    /// @note Gb码率类型
    typedef enum class _GbBitRateType_E_:int
    {
        GB_BITRATE_NONE = 0, /* 不支持 */
        GB_RBITRATE_CBR = 1,
        GB_RBITRATE_VBR = 2,
    } GbBitRateType_E;


/*设备配置-基本参数配置类型*/
    typedef struct _BasicParam_
    {
        uint64_t nSN;               /* 设备配置序号 */
        std::string strID;          /* 设备ID  */
        std::string strResult;      /*查询结果标志*/
        int nIndex;                  //上级根据通道号赋值，下级统统设置成-1
        bool bIsSet;                 //true--设备配置设置，false--设备配置获取

        std::string m_szName;       /*设备名称*/
        int m_nExpiration;          /*注册过期时间*/
        int m_nHeartBeatInterval;   /*心跳间隔时间*/
        int m_nHeartBeatCount;      /*心跳超时次数*/

        _BasicParam_()
        {
            nSN = 0;
            strID = "";
            strResult = "";
            m_szName = "";
            m_nExpiration = 0;
            m_nHeartBeatInterval = 0;
            m_nHeartBeatCount = 0;
            nIndex = 0;
            bIsSet = false;
        }
    } BasicParamInfo_S;

    typedef struct _OSDCItem_
    {
        /* data */
        std::string Text;           /*文字内容*/
        int X;                      /*文字X坐标*/
        int Y;                      /*文字Y坐标*/
    }OSDCItem;

    /*设备配置-OSD配置类型*/
    typedef struct _OSDConfig_
    {
        uint64_t nSN;               /* 设备配置序号 */
        std::string strID;          /* 设备ID  */
        std::string strResult;      /*查询结果标志*/
        int nIndex;                  //上级根据通道号赋值，下级统统设置成-1
        bool bIsSet;                 //true--设备配置设置，false--设备配置获取

        int m_nLength;              /*窗口长度像素值*/
        int m_nWidth;               /*窗口宽度像素值*/
        int m_nTimeX;               /*时间X像素坐标*/
        int m_nTimeY;               /*时间Y像素坐标*/
        int m_nTimeEnable;          /* 显示时间开关 */
        int m_nTextEnable;          /* 显示文字开关 */
        int m_nTimeType;            /* 显示时间类型-0-YYYY-MM-DD HH:MM:SS 1-YYYY年MM月DD日 HH:MM:SS- */
        int m_SumNum;               /*显示文字行数总数*/

        _OSDConfig_()
        {
            nSN = 0;
            strID = "";
            strResult = "";
            m_nLength = 0;
            m_nWidth = 0;
            m_nTimeX = 0;
            m_nTimeY = 0;
            m_SumNum = 0;
            nIndex = 0;
            bIsSet = false;
        }
        std::vector<OSDCItem> m_vecItme;
    } OSDConfig_S;

    /*感兴趣区域*/
    typedef struct _ROIParamItem_
    {
        /* data */
        int nROISeq;        /*编号，取值1~16*/
        int nTopLeft;       /*区域左上角坐标*/
        int nBottomRight;   /*区域右下角坐标*/
        int nROIQP;         /*编码质量等级，0-一般，1-较好，2-好，3-很好*/
        _ROIParamItem_()
        {
            nROISeq = -1;
            nTopLeft = 0;
            nBottomRight = 0;
            nROIQP = 0;
        }
    }ROIParamItem;
    /*感兴趣区域参数*/
    typedef struct _ROIParam_
    {
        int nROIFlag;    /*感兴趣区域开关,0-关闭,1-打开*/
        int nROINumber;  /*感兴趣区域数量,取值0~16*/

        _ROIParam_()
        {
            nROIFlag = -1;
            nROINumber = 0;
            memset(&vecROIParamItem, 0, sizeof(vecROIParamItem));
        }
        std::vector<ROIParamItem> vecROIParamItem;
    } ROIParamInfo;

    /*SVC参数*/
    typedef struct _SVCParam_
    {
        bool bIsHave;

        //公共参数
        int nSVCSpaceDomainMode;        /*空域编码,0-基本层，1-1级增强，2-2级增强，3-3级增强*/
        int nSVCTimeDomainMode;         /*时域编码,0-基本层，1-1级增强，2-2级增强，3-3级增强*/
        //编码参数
        std::string strSSVCRatioValue;  /*SSVC增强层与基本层比例值，如4:3、2:1、6:1*/
        int nSVCSpaceSupportMode;       /*空域编码能力,0-不支持 ，1-1级增强，2-2级增强，3-3级增强*/
        int nSVCTimeSupportMode;        /*时域编码能力,0-不支持 ，1-1级增强，2-2级增强，3-3级增强*/
        std::string strSSVCRatioSupportList;/*SSVC增强层与基本层比例能力，如4:3、2:1、6:1*/
        //解码参数
        int nSVCSTMMode;                /*码流显示模式，取值0、1、2、3*/

        _SVCParam_()
        {
            nSVCSpaceDomainMode = 0;
            nSVCTimeDomainMode = 0;
            strSSVCRatioValue = "";
            nSVCSpaceSupportMode = 0;
            nSVCTimeSupportMode = 0;
            strSSVCRatioSupportList = "";
            nSVCSTMMode = 0;
        }
    } SVCParamInfo;

    /*监控专用信息参数*/
    typedef struct _SurveillanceParam_
    {
        bool isHave;
        //编码参数
        int nTimeFlag;      /*时间信息开关,0-关闭,1-打开*/
        int nOSDFlag;       /*OSD信息开关,0-关闭,1-打开*/
        int nAIFlag;        /*智能分析开关,0-关闭,1-打开*/
        int nGISFlag;       /*地理信息开关,0-关闭,1-打开*/
        //解码参数
        //bool isDecHave;
        int nTimeShowFlag;  /*时间信息显示开关,0-关闭,1-打开*/
        int nOSDShowFlag;    /*OSD信息显示开关,0-关闭,1-打开*/
        int nAIShowFlag;     /*智能分析显示开关,0-关闭,1-打开*/
        int nGISShowFlag;    /*地理信息显示开关,0-关闭,1-打开*/
        _SurveillanceParam_()
        {
            nTimeFlag = 0;
            nOSDFlag = 0;
            nAIFlag = 0;
            nGISFlag = 0;
            isHave = false;
            nTimeShowFlag = 0;
            nOSDShowFlag = 0;
            nAIShowFlag = 0;
            nGISShowFlag = 0;
        }
    } SurveillanceParamInfo;

    /*音频参数*/
    typedef struct _AudioParam_
    {
        bool bIsHave;       //是否携带音频参数
        int nAudioRecognitionFlag;  /*声音识别特征开关,0-关闭,1-打开*/
        _AudioParam_()
        {
            nAudioRecognitionFlag = 0;
        }
    } AudioParamInfo;

    /*设备配置-SVAC编码配置*/
    typedef struct _SVACEncodeConfig_
    {
        uint64_t nSN;               /* 设备配置序号 */
        std::string strID;          /* 设备ID  */
        std::string strResult;      /*查询结果标志*/
        int nIndex;                  //上级根据通道号赋值，下级统统设置成-1
        bool bIsSet;                 //true--设备配置设置，false--设备配置获取

        ROIParamInfo tROIParam;     /*感兴趣区域参数*/
        SVCParamInfo tSVCParam;     /*SVC参数*/
        SurveillanceParamInfo tSurveillanceParam; /*监控专用信息参数*/
        AudioParamInfo tAudioParam; /*音频参数*/

        _SVACEncodeConfig_()
        {
            nSN = 0;
            strID = "";
            strResult = "";
            memset(&tROIParam, 0, sizeof(tROIParam));
            memset(&tSVCParam, 0, sizeof(tSVCParam));
            memset(&tSurveillanceParam, 0, sizeof(tSurveillanceParam));
            memset(&tAudioParam, 0, sizeof(tAudioParam));
            nIndex = 0;
            bIsSet = false;
        }
    } SVACEncodeInfo_S;

    /*设备配置-SVAC解码配置*/
    typedef struct _SVACDecodeConfig_
    {
        uint64_t nSN;               /* 设备配置序号 */
        std::string strID;          /* 设备ID  */
        std::string strResult;      /*查询结果标志*/
        int nIndex;                  //上级根据通道号赋值，下级统统设置成-1
        bool bIsSet;                 //true--设备配置设置，false--设备配置获取

        SVCParamInfo           tSVCParam;  /*SVC参数*/
        SurveillanceParamInfo  tSurveillanceParam;  /*监控专用信息参数*/

        _SVACDecodeConfig_()
        {
            nSN = 0;
            strID = "";
            strResult = "";
            memset(&tSVCParam, 0, sizeof(tSVCParam));
            memset(&tSurveillanceParam, 0, sizeof(tSurveillanceParam));
            nIndex = 0;
            bIsSet = false;
        }
    } SVACDecodeInfo_S;

    typedef struct _VideoParamAttribute_
    {
        int nStreamNumber;                /*视频流编号，0-主码流，1-子码流,2-子码流*/
        GbVideoType_E enVideoFormat;      /*视频编码格式*/
        GbResolutionType_E enResolution;  /*分辨率*/
        std::string strFrameRate;         /*帧率*/
        GbBitRateType_E enBitRateType;    /*码率类型*/
        std::string strVideoBitRate;      /*视频码率，固定码率必选*/
       
        _VideoParamAttribute_()
        {
            nStreamNumber = 0;
            enVideoFormat = GbVideoType_E::GB_VIDEO_H264;
            enResolution = GbResolutionType_E::GB_RESOLUTION_1080P;
            strFrameRate = "";
            enBitRateType = GbBitRateType_E::GB_RBITRATE_CBR;
            strVideoBitRate = "";
        }
    } VideoParamAttributeItem;

    /*设备配置-视频参数属性配置*/
    typedef struct _VideoParamAttributeConfig_
    {
        uint64_t nSN;               /* 设备配置序号 */
        std::string strID;          /* 设备ID  */
        std::string strResult;      /*查询结果标志*/
        int nIndex;                  //上级根据通道号赋值，下级统统设置成-1
        bool bIsSet;                 //true--设备配置设置，false--设备配置获取

        int Num;
        _VideoParamAttributeConfig_()
        {
            nSN = 0;
            strID = "";
            strResult = "";
            Num = 0;
            nIndex = 0;
            bIsSet = true;
        }
        std::vector<VideoParamAttributeItem> vecVideoParAttrItem;
    } VideoParamAttributeInfo_S;

    typedef struct _TimeSegment_
    {
        int nStartHour;     /*开始时间时*/
        int nStartMin;      /*开始时间分*/
        int nStartSec;      /*开始时间秒*/
        int nStopHour;      /*结束时间时*/
        int nStopMin;       /*结束时间分*/
        int nStopSec;       /*结束时间秒*/
        _TimeSegment_()
        {
            nStartHour = 0;
            nStartMin = 0;
            nStartSec = 0;
            nStopHour = 0;
            nStopMin = 0;
            nStopSec = 0;
        }
    } TimeSegmentItem;

    typedef struct _RecordSchedule_
    {
        int nWeekDayNum;            /*周几(1~7)*/
        int nTimeSegmentSumNum;     /*录像计划时间段，每天支持最多8个时间段*/
        _RecordSchedule_()
        {
            nWeekDayNum = 0;
            nTimeSegmentSumNum = 0;
        }
        std::vector<TimeSegmentItem> vecTimeSegment;
    } RecordScheduleItem;

    /*设备配置-录像计划配置*/
    typedef struct _VideoRecordPlan_
    {
        uint64_t nSN;               /* 设备配置序号 */
        std::string strID;          /* 设备ID  */
        std::string strResult;      /*查询结果标志*/
        int nIndex;                  //上级根据通道号赋值，下级统统设置成-1
        bool bIsSet;                 //true--设备配置设置，false--设备配置获取

        int nRecordEnable;          /*录像启用,0-否，1-是*/
        int nRecordScheduleSumNum;  /*录像计划总天数*/
        int nStreamNumber;          /*码流类型，0-主码流，1-子码流1, 2-子码流2*/

    
        _VideoRecordPlan_()
        {
            nSN = 0;
            strID = "";
            strResult = "";
            nRecordEnable = 0;
            nRecordScheduleSumNum = 0;
            nStreamNumber = 0;
            nIndex = 0;
            bIsSet = false;
        }
        std::map<int,RecordScheduleItem> vecRecordSchedule;
    } VideoRecordPlanInfo_S;

    /*设备配置-图像抓拍配置*/
    typedef struct _SnapShotConfig_
    {
        uint64_t nSN;               /* 设备配置序号 */
        std::string strID;          /* 设备ID  */
        std::string strResult;      /*查询结果标志*/
        int nIndex;                  //上级根据通道号赋值，下级统统设置成-1
        bool bIsSet;                 //true--设备配置设置，false--设备配置获取

        int nSnapNum;               /*连拍张数，最多10张，手动抓拍取值为1*/
        int nInterval;              /*单张抓拍间隔时间：秒，最短1秒*/
        std::string strUploadURL;      /*抓拍图像上传路径*/
        std::string strSessionID;      /*会话ID*/
    
        _SnapShotConfig_()
        {
            nSN = 0;
            strID = "";
            nSnapNum = 0;
            nInterval = 0;
            strUploadURL = "";
            strSessionID = "";
            strResult = "";
            nIndex = 0;
            bIsSet = false;
        }
    } SnapShotConfigInfo_S;

    typedef struct _RegionList_
    {
        int Seq;                /*区域编号,取值范围1~4*/
        /*区域左上角，格式如20，30，50，60*/
        int nlx;
        int nly;
        int nrx;
        int nry;

        _RegionList_()
        {
            Seq = 0;
            nlx = 0;
            nly = 0;
            nrx = 0;
            nry = 0;
        }
    } RegionListItem;

    /*设备配置-视频画面遮挡配置*/
    typedef struct _PictureMask_
    {
        uint64_t nSN;               /* 设备配置序号 */
        std::string strID;          /* 设备ID  */
        std::string strResult;      /*查询结果标志*/
        int nIndex;                  //上级根据通道号赋值，下级统统设置成-1
        bool bIsSet;                 //true--设备配置设置，false--设备配置获取

        int On;                     /*遮挡开关，0-关闭，1-取值*/
        int SumNum;                 /*区域总数*/
    
        _PictureMask_()
        {
            nSN = 0;
            strID = "";
            strResult = "";
            On = 0;
            SumNum = 0;
            nIndex = 0;
            bIsSet = false;
        }
        std::vector<RegionListItem> vecRegionList;
    } PictureMaskInfo_S;

    /*设备配置-报警录像配置*/
    typedef struct _VideoAlarmRecord_
    {
        uint64_t nSN;               /* 设备配置序号 */
        std::string strID;          /* 设备ID  */
        std::string strResult;      /*查询结果标志*/
        int nIndex;                  //上级根据通道号赋值，下级统统设置成-1
        bool bIsSet;                 //true--设备配置设置，false--设备配置获取

        int nRecordEnable;          /*是否启用,0-否，1-是*/
        int nRecordTime;            /*录像延时时间/秒*/
        int nPreRecordTime;         /*录像时间/秒*/
        int nStreamNumber;          /*码流编号:0-主码流，1-子码流1，2-子码流2*/
    
        _VideoAlarmRecord_()
        {
            nSN = 0;
            strID = "";
            strResult = "";
            nRecordEnable = 0;
            nRecordTime = 0;
            nPreRecordTime = 0;
            nStreamNumber = 0;
            nIndex = 0;
            bIsSet = false;
        }
    } VideoAlarmRecordInfo_S;

    /*设备配置-报警上报开关配置*/
    typedef struct _AlarmReport_
    {
        uint64_t nSN;               /* 设备配置序号 */
        std::string strID;          /* 设备ID  */
        std::string strResult;      /*查询结果标志*/
        int nIndex;                  //上级根据通道号赋值，下级统统设置成-1
        bool bIsSet;                 //true--设备配置设置，false--设备配置获取

        int nMotionDetection;     /*移动侦测事件上报开关，0-关闭，1-打开*/
        int nFieldDetection;      /*区域入侵事件上报开关，0-关闭，1-打开*/
    
        _AlarmReport_()
        {
            nSN = 0;
            strID = "";
            strResult = "";
            nMotionDetection = 0;
            nFieldDetection = 0;
            nIndex = 0;
            bIsSet = false;
        }
    } AlarmReportInfo_S;

    /*设备配置-视频参数范围配置类型*/
    typedef struct _VideoParamOpt_
    {
        uint64_t nSN;               /* 设备配置序号 */
        std::string strID;          /* 设备ID  */
        std::string strResult;      /*查询结果标志*/
        int nIndex;                  //上级根据通道号赋值，下级统统设置成-1
        bool bIsSet;                 //true--设备配置设置，false--设备配置获取

        std::string strDownloadSpeed;     /*下载倍数范围,如1/2/4*/
        std::string strResolution;        /*摄像机支持的分辨率*/
    
        _VideoParamOpt_ ()
        {
            nSN = 0;
            strID = "";
            strResult = "";
            strDownloadSpeed = "";
            strResolution = "";
            nIndex = 0;
            bIsSet = false;
        }
    } VideoParamOptInfo_S;

    /* 设备配置-视频画面翻转配置*/
    typedef struct _FrameMirror_
    {
        uint64_t nSN;               /* 设备配置序号 */
        std::string strID;          /* 设备ID  */
        std::string strResult;      /*查询结果标志*/
        int nIndex;                  //上级根据通道号赋值，下级统统设置成-1
        bool bIsSet;                 //true--设备配置设置，false--设备配置获取

        int nFrameMirror;           /*0-不启用镜像，1-水平镜像，2-上下镜像，3-中心镜像*/
    
        _FrameMirror_ ()
        {
            nSN = 0;
            strID = "";
            strResult = "";
            nFrameMirror = 0;
            nIndex = 0;
            bIsSet = false;
        }
    } FrameMirrorInfo_S;


    typedef struct _CruisePoint_
     {

         int nPresetIndex;      /*预置位编号*/
         int nStayTime;         /*预置位点停时间,单位：秒*/
         int nSpeed;            /*云台速度:1~15*/

         _CruisePoint_ ()
         {
             nPresetIndex = 0;
             nStayTime = 0;
             nSpeed = 0;
         }
     } CruisePointItem;

     /* 巡航轨迹查询 */
     typedef struct _CruiseTrackQuery_
     {
         uint64_t nSN;               /* 设备配置序号 */
         std::string strID;          /* 设备ID  */
         int nIndex;                 //上级根据通道号赋值，下级统统设置成-1
         bool bIsSet;                //true--设备配置设置，false--设备配置获取

         int nNumber;                /*轨迹编号：0-第一条轨迹;1-第二条轨迹*/
         std::string strName;        /*轨迹名称*/
         int nSumNum;                /*查询结果总数*/   

         _CruiseTrackQuery_ ()
         {
             nSN = 0;
             strID = "";
             nIndex = 0;
             bIsSet = false;
             nNumber = 0;
             strName = "";
             nSumNum = 0;
         }

         std::vector<CruisePointItem> vecCruisePointList;
     } CruiseTrackQueryInfo_S;

     typedef struct _CruiseTrack_
    {
        int nNumber;                    /*轨迹编号*/
        std::string strName;               /*轨迹名称*/
        _CruiseTrack_()
        {
            nNumber = 0;
            strName = "";
        }

    } CruiseTrackItem;

    /* 巡航轨迹列表查询 */
    typedef struct _CruiseTrackListQuery_
    {
        uint64_t nSN;               /* 设备配置序号 */
        std::string strID;          /* 设备ID  */
        int nIndex;                  //上级根据通道号赋值，下级统统设置成-1
        bool bIsSet;                 //true--设备配置设置，false--设备配置获取

        int nSumNum;                /*总数*/

        _CruiseTrackListQuery_()
        {
            nSN = 0;
            strID = "";
            nIndex = 0;
            bIsSet = false;
            nSumNum = 0;
        }

        std::vector<CruiseTrackItem> vecCruiseTracktList;
    } CruiseTrackListQueryInfo_S;

    typedef struct _PresetList_
    {
        std::string strPresetID;        /*预置位编码*/
        std::string strPresetName;      /*预置位名称*/
        _PresetList_()
        {
            strPresetID = "";
            strPresetName = "";
        }
    } PresetListItem;

    /*设备预置位查询*/
    typedef struct _PresetQuery_
    {
        uint64_t nSN;               /* 设备配置序号 */
        std::string strID;          /* 设备ID  */
        int nIndex;                  //上级根据通道号赋值，下级统统设置成-1
        bool bIsSet;                 //true--设备配置设置，false--设备配置获取

        int nSumNum;                /*总数*/

        _PresetQuery_()
        {
            nSN = 0;
            strID = "";
            nIndex = 0;
            bIsSet = false;
            nSumNum = 0;
        }
        std::vector<PresetListItem> vecPresetList;
    } PresetQueryInfo_S;

    /*语音广播*/
    typedef struct _Broadcast_
    {
        uint64_t nSN;               /* 设备配置序号 */
        std::string strID;          /* 设备ID  */
        int nIndex;                 /*上级根据通道号赋值，下级统统设置成-1*/

        std::string strSourceID;    /*语音输入设备的设备编码*/
        std::string strTargetID;    /*语音输出设备的设备编码*/
        std::string strResult;

        _Broadcast_()
        {
            nSN = 0;
            strID = "";
            nIndex = 0;
            strSourceID = "";
            strTargetID = "";
            strResult = "";
        }
    } BroadcastInfo_S;

    /* 关键帧 */
    typedef struct _IFrame_
    {
        uint64_t nSN;               /* 设备配置序号 */
        std::string strID;          /* 设备ID  */
        int nIndex;                 /*上级根据通道号赋值，下级统统设置成-1*/
        std::string IFrameCmd;      /* 强制I帧命令 */

        _IFrame_()
        {
            nSN = 0;
            strID = "";
            nIndex = 0;
        }
    }IFrame_S;

    /* 设备控制-拉框放大/缩小 */
    typedef struct _DragZoom_
    {
        uint64_t nSN;               /* 设备配置序号 */
        std::string strID;          /* 设备ID  */

        bool bIsZooIn;   /* true-放大，false-缩小 */

        int nLength;    /*播放窗口长度像素值*/
        int nWidth;     /*播放窗口宽度像素值*/
        int nMidPointX; /*拉框中心的横轴坐标像素值*/
        int nMidPointY; /*拉框中心的纵轴坐标像素值*/
        int nLengthX;    /*拉框长度像素值*/
        int nLengthY;    /*拉框宽度像素值*/

        _DragZoom_()
        {
            nSN = 0;
            strID = "";
            bIsZooIn = false;
            nLength = 0;
            nWidth = 0;
            nMidPointX = 0;
            nMidPointY = 0;
            nLengthX = 0;
            nLengthY = 0;
        }
    } DragZoomInfo_S;

    /* 设备控制-PTZ精准控制 */
    typedef struct _PTZPreciseCtrl_
    {
        uint64_t nSN;               /* 设备配置序号 */
        std::string strID;          /* 设备ID  */

        int nIndex;                 /*上级根据通道号赋值，下级统统设置成-1*/

        double dPan;                /* 云台水平角度 */
        double dTilt;               /* 云台垂直角度 */
        double dZoom;               /*变焦倍数*/

        std::string strResult;

        _PTZPreciseCtrl_()
        {
            nSN = 0;
            strID = "";

            dPan = 0.0;
            dTilt = 0.0;
            dZoom = 0.0;
        }
    } PTZPreciseCtrl_S;

    /* 设备查询-PTZ精准状态查询 */
    typedef struct _PTZPosition_
    {
        uint64_t nSN;               /* 设备配置序号 */
        std::string strID;          /* 设备ID  */

        int nIndex;                 /*上级根据通道号赋值，下级统统设置成-1*/

        double dPan;                /* 云台水平角度 */
        double dTilt;               /* 云台垂直角度 */
        double dZoom;               /*变焦倍数*/
        double HorizontalFieldAngle;/*摄像机水平视场角*/
        double dVerticalFieldAngle; /*摄像机垂直视场角*/
        double dMaxViewDistance;    /*摄像机可视距离*/

        _PTZPosition_()
        {
            nSN = 0;
            strID = "";
            nIndex = 0;

            dPan = 0.0;
            dTilt = 0.0;
            dZoom = 0.0;
            HorizontalFieldAngle = 0.0;
            dVerticalFieldAngle =0.0;
            dMaxViewDistance = 0.0;
        }
    } PTZPositionInfo_S;

    /* 设备控制-录像控制命令 */
    typedef struct _RecordCmd_
    {
        uint64_t nSN;               /* 设备配置序号 */
        std::string strID;          /* 设备ID  */
        std::string strResult;      /* 录像控制执行结果 */
        int nIndex;                 /*上级根据通道号赋值，下级统统设置成-1*/

        int nStreamNumber;          /* 码流类型:0-主码流, 1-子码流, 2-子码流 */
        bool bIsRecord;             /* true-录像, false-停止录像 */

        _RecordCmd_()
        {
            nSN = 0;
            strID = "";
            strResult = "";

            nStreamNumber = 0;
            bIsRecord = false;
        }
    } RecordCmd_S;

    /* 设备控制应答 */
    typedef struct _ControlResults_
    {
        uint64_t nSN;               /* 设备配置序号 */
        std::string strID;          /* 设备ID  */
        std::string strResult;      /* 控制执行结果 */

        int nCmdType;               /* 1-设备控制应答, 2-设备配置控制应答 */
        
        _ControlResults_()
        {
            nSN = 0;
            strID = "";
            strResult = "";
            nCmdType = 0;
        }
    } ControlResults_S;

    /* 设备控制-报警复位控制命令 */
    typedef struct _AlarmCmd_
    {
    uint64_t nSN;               /* 设备配置序号 */
    std::string strID;          /* 设备ID  */
    std::string strResult;      /* 录像控制执行结果 */

    std::string strAlarmMethod; /* 复位报警的报警方式属性，如0-全部;1-电话报警;2-设备报警等等 */
    std::string strAlarmType;   /* 复位报警的报警类型属性，报警方式为2时,不携带AlarmType时默认设备报警，取值为1~5 */

    _AlarmCmd_()
    {
        nSN = 0;
        strID = "";
        strResult = "";

        strAlarmMethod = "";
        strAlarmType = "";
    }
    } AlarmCmd_S;

    /* 设备控制/查询-看守位 */
    typedef struct _HomePosition_
    {
        uint64_t nSN;               /* 设备配置序号 */
        std::string strID;          /* 设备ID  */
        std::string strResult;      /*查询结果标志*/
        int nIndex;                 /*上级根据通道号赋值，下级统统设置成-1*/
        bool bIsSet;                /*true--设备配置设置，false--设备配置获取*/

        int nEnabled;               /*看守位使能 1-开启,0-关闭*/
        int nResetTime;             /*自动归位时间间隔，开启看守位时使用,单位:s*/
        int nPresetIndex;           /*调用预置位编号，开启看守位时使用,取值0~255*/

        _HomePosition_()
        {
            nSN = 0;
            strID = "";
            strResult = "";
            nIndex = 0;
            bIsSet = false;
            nEnabled = 0;
            nResetTime = 0;
            nPresetIndex = 0;
        }
    } HomePositionInfo_S;

    /* 设备控制-软件升级 */
    typedef struct _DeviceUpgrade_
    {
        uint64_t nSN;                  /* 设备配置序号 */
        std::string strID;             /* 设备ID  */
        std::string strResult;         /*查询结果标志*/

        std::string strFirmware;       /*设备固件版本*/
        std::string strFileURL;        /*升级文件的完整路径*/
        std::string strManufacturer;   /*设备厂商*/
        std::string strSessionID;      /*会话ID*/

        _DeviceUpgrade_()
        {
            nSN = 0;
            strID = "";
            strResult = "";
            strFirmware = "";
            strFileURL = "";
            strManufacturer = "";
            strSessionID = "";
        }
    } DeviceUpgradeInfo_S;

    /* 设备控制-目标跟踪控制命令 */
    typedef struct _TargetTrack_
    {
        enum TYPE
        {
            OTHER,
            AUTO,         // 自动跟踪
            MANUAL,       // 手动跟踪
            STOP
        };

        uint64_t nSN;                  /* 设备配置序号 */
        std::string strID;             /* 设备ID  */

        TYPE TrackType;                /*跟踪命令类型*/  
        std::string strDeviceID2;      /*DeviceID2目标设备编码*/
        int nLength;                    /*全景播放窗口长度像素值*/
        int nWidth;                    /*全景播放窗口宽度像素值*/
        int nMidPointX;                /*跟踪框中心的横坐标像素值*/
        int nMidPointY;                /*跟踪框中心的纵坐标像素值*/
        int nLengthX;                  /*跟踪框长度像素值*/
        int nLengthY;                  /*跟踪框宽度像素值*/
        _TargetTrack_()
        {
            nSN = 0;
            strID = "";
            strDeviceID2 = "";
            nLength = 0;
            nWidth = 0;
            nMidPointX = 0;
            nMidPointY = 0;
            nLengthX = 0;
            nLengthY = 0;
        }
    } TargetTrackInfo_S;

    typedef struct _SnapShot_
    {
        std::string strSnapShotFileID; /*抓拍图像唯一标识,由前端抓拍设备生成*/

        _SnapShot_()
        {
            strSnapShotFileID = ""; 
        }
    } SnapShotItem;


    /* 图像抓拍传输完成通知 */
    typedef struct _UploadSnapShotFinished_
    {
        uint64_t nSN;                  /* 设备配置序号 */
        std::string strID;             /* 设备ID  */

        std::string strSessionID;       /*会话ID,由平台生成*/

        _UploadSnapShotFinished_()
        {
            nSN = 0;
            strID = "";
            strSessionID = "";
        }
        std::vector<SnapShotItem> vecSnapShotList;
    } UploadSnapShotFiniInfo_S;

} /* namespace GB28181 */

#endif //_GB_DEFINE_H_