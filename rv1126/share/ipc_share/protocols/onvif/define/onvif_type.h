/**
 * @file onvif_type.h
 * @author tianl (tianl@kfb.cn)
 * @date 2025-09-23
 * 
 * @brief onvif相关类型定义
 */
#pragma once
#include <time.h>  
#include <stdio.h>

#include "onvif_comm.h"


#define ONVIF_ERR_EVENT_RESOURCE_CONFLICT -305

#ifdef __cplusplus
extern "C"
{
#endif

#ifndef _ONVIF_TYPE_H
#define _ONVIF_TYPE_H

typedef struct OnvifDeviceInfo
{
    char achManufacturer[32];
    char achModel[32];
    char achFirmwareVersion[32];
    char achSerialNumber[32];
    char achHardwareId[32];
} OnvifDeviceInfo_t;

/* 配置设备网络需要参数 */
typedef struct _ConfigNetworkInfo_S_
{
    bool bDhcp;             /* 是否开启DHCP */
    int nPrefixlen;         /* 子网掩码位数 */
    int nMtu;               /* mtu大小 */
    char achIPAddr[128];    /* IP地址 */
    char achMAC[128];       /* MAC地址 */
}ONvifNetworkInfo_S;

typedef enum
{
    AudioEncoding_UNKNOW = -1,
    AudioEncoding_G711 = 0,
    AudioEncoding_G726 = 1,
    AudioEncoding_AAC = 2
} OnvifAudioEncoding_e;

typedef struct OnvifAudioParam
{
    OnvifAudioEncoding_e audioFormat; /* 音频编码格式 */
    int audioBitrate;                 /* 音频码率 */
    int audioSampleRate;              /* 音频采样率 */
    char audioEncoding[32];

    int BitrateList[AUDIO_BITRATE_LIST];     /* 音频码率列表 */
    int SampleRateList[AUDIO_SAMPRATE_LIST]; /* 音频采样率列表 */
} OnvifAudioParam_t;

typedef struct OnvifProfile
{
    int nWidth;           /* 视频宽 */
    int nHeight;          /* 视频高 */
    int FrameRateLimit;   /* 帧率限制 */
    int BitrateLimit;     /* 码率限制 */
    int Quality;          /* 视频质量 */
    int IFrameInterval;   /* I帧间隔 */
    bool ConstantBitRate; /*码率类型*/

    /* profile name、token */
    char Name[32];
    char token[32];

    /* video name、token */
    char VideoSourceConfiguration_Name[32];
    char VideoSourceConfiguration_token[32];
    char VideoSourceConfiguration_SourceToken[32];

    char VideoEncoderConfiguration_Name[32];
    char VideoEncoderConfiguration_token[32];

    char VideoEncoderConfiguration_Encoding[32];
    int VideoEncoding;

    /* audio name、token */
    char AudioSourceConfiguration_Name[32];
    char AudioSourceConfiguration_token[32];
    char AudioSourceConfiguration_SourceToken[32];

    char AudioEncoderConfiguration_Name[32];
    char AudioEncoderConfiguration_token[32];

    OnvifAudioParam_t stAudioParam;
    
    int nVideoType; /* 视频类型: 0-复合流, 1-视频流 */

} OnvifProfile_t;

/* 代表一个点的坐标 */
typedef struct _OnvifPoint_
{
    int x;
    int y;
} OnvifPoint_S;

typedef struct _OnvifOsdPos_S_
{
    float x;
    float y;
    char achTpye[128];                /* 坐标类型 */
}OnvifOsdPos_S;

typedef struct _CommomOsdPos_S_
{
    int x;
    int y;
}CommomOsdPos_S;

typedef enum
{
    E_OSDTYPE_TEXT = 0,
    E_OSDTYPE_IMAGE = 1,
    E_OSDTYPE_MAX
} OsdType_e;

typedef enum
{
    E_OSDTYPE_TEXT_NAME = 0,
    E_OSDTYPE_TEXT_TIME = 1,
    E_OSDTYPE_TEXT_EXTEND = 2,
    E_OSDTYPE_TEXT_MAX
} OsdTextType_e;

typedef enum
{
    E_OSDPOSTYPE_TOP_LEFT = 0, /* 左上对齐 */
    E_OSDPOSTYPE_BOTTOM_LEFT,  /* 左下对齐 */
    E_OSDPOSTYPE_TOP_RIGHT,    /* 右上对齐 */
    E_OSDPOSTYPE_BOTTOM_RIGHT, /* 右下对齐 */
    E_OSDPOSTYPE_CENTER,       /* 中心对齐 */
    // E_OSDPOSTYPE_CUSTOM,       /* 自定义 */
    E_OSDPOSTYPE_MAX
} OsdPostionType_e;

typedef struct OnvifOsdCfiguration
{
    int nReferenceWith;             // 画osd的画面码流参考宽
    int nReferenceHeight;           // 画osd的画面码流参考高
    int nHorMargin;                 // 画osd的水平边距
    int nVerMargin;                 // 画osd的竖直边距
    bool bOsdEnable;                // 是否开启osd
    OsdType_e eOsdType;             // osd类型
    OsdTextType_e eTextType;        // osd文本类型
    OsdPostionType_e eOsdAlign;     // osd对齐方式
    float x;                        // 自定义osd位置时转换为onvif协议x坐标
    float y;                        // 自定义osd位置时转换为onvif协议y坐标
    OnvifOsdPos_S stONvifPos;       // onvif类型坐标
    int TextString_FontSize;        // osd字体大小
    int TextString_FontAlpha;       // 字体透明度
    int TextString_Font_R;          // 字体R分量
    int TextString_Font_G;          // 字体G分量
    int TextString_Font_B;          // 字体B分量
    int TextString_BackgroundAlpha; // 背景透明度
    int TextString_Background_R;    // 背景R分量
    int TextString_Background_G;    // 背景G分量
    int TextString_Background_B;    // 背景B分量
    char TextString_Type[16];       // 文本字符串类型
    char Position_Type[16];         // osd位置 UpperLeft、UpperRight、LowerLeft、LowerRight、Custom

    char TextString_DateFormat[32]; // 日期格式
    char TextString_TimeFormat[32]; // 时间格式
    char token[32];
    char TextString_PlainText[128]; // 自定义文本osd文本内容

} OnvifOsdCfg_t;

typedef struct OnvifImageParam
{
    unsigned int nBrightness; /* 亮度[0,100] */
    unsigned int nContrast;   /* 对比度[0,100] */
    unsigned int nSaturation; /* 饱和度[0,100] */
    unsigned int nSharpness;  /* 锐度[0,100] */
} OnvifImageParam_t;

#define RESSOLUTION_SIZE_MAX 10

typedef struct OnvifVideoParam
{
    char strEncodingComplexity[16]; // 编码复杂度

    int nSizeResolutionsAvailable; /* 可切换分辨率数 */
    int nWidths[RESSOLUTION_SIZE_MAX];
    int nHeights[RESSOLUTION_SIZE_MAX];

    char strVideoCodec[16]; /* 视频编码 */

    float fCurFrameRate; /* 视频帧率fps */

    bool bConstantBitRate; /*码率类型*/

    int nCurBitrate; /* 当前码率 */
    int nBitrateMax; /* 码率上限kbps */
    int nBitrateMin; /* 码率下限kbps */

    int nCurQuality; /* 当前图像质量 */
    int nQualityMax; /* 图像质量最大值 */
    int nQualityMin; /* 图像质量最小值 */

    int nIFrameInterval; /* I帧间隔 */

    // char GovLengthRange[1024];
    char FrameRatesSupported[1024];
    
    int nVideoType; /* 视频类型: 0-复合流, 1-视频流 */

} OnvifVideoParam_t;


/**
 *@brief 报警类型
*/
typedef enum  _AlarmType_
{
    MOTION_DETECTION_ALARM = 0,    /* 运动目标检测报警 */
    IMAGE_OBSTRUTION_ALARM,        /* 图像遮挡报警 */
    TRIPWIRE_ALARM,                /* 绊线检测报警 */
    INTRUSION_ALARM,               /* 入侵检测报警 */
    LINGERING_ALARM,               /* 徘徊检测报警 */
    VIDEO_ABNORMAL_ALARM,          /* 视频异常检测报警 */

    ///note 自定义onvif报警类型上报
    ONVIF_ENTER_REGION , /* 进入区域 */
    ONVIF_LEAVE_REGION, /* 离开区域 */

    /**
     * @brief   : Smart事件
     */
    ONVIF_AUDIO_ANOMALY,     /* 音频异常侦测 */
    ONVIF_AUDIO_SUDDEN_RISE,/* 音频异常侦测-声强陡升检测 */
    ONVIF_AUDIO_SUDDEN_DROP,/* 音频异常侦测-声强陡降检测 */
    ONVIF_SCENE_CHANGE,      /* 场景变更 */
    ONVIF_FACE_DETECT,       /* 人脸侦测 */
    ONVIF_LOITERING_DETECT ,  /* 徘徊侦测 */
    ONVIF_CROWD_GATHERING ,   /* 人员聚集 */
    ONVIF_PARKING_DETECT,    /* 停车侦测 */
    ONVIF_UNATTENDED_OBJECT, /* 物品遗留 */
    ONVIF_OBJECT_REMOVAL,    /* 物品拿取 */
    ONVIF_PET_RECOGNITION,   /* 宠物识别 */
    ONVIF_FACE_CAPTURE       /* 人脸抓拍 */
}OnvifAlarmEventType_E;
/**
 * @brief 单个报警信息
 */
typedef struct _AlarmEventInFo_S_
{
    OnvifAlarmEventType_E enAlarmType;      /* 报警类型 */
    time_t alarmTime;                       /* 报警时间 */
    int nValue;                             /* 0为触发 1为结束 */
}OnvifAlarmEventInFo_S;

/**
 * @brief 批量报警信息
 */
typedef struct _OnvifAlarmEventBatch_S_ 
{
    OnvifAlarmEventInFo_S events[128]; 
    int nEventNum;
    long long nExpireTime;                  /* 剩余有效时长 */
} OnvifAlarmEventBatch_S;

/**
 * @brief 移动侦测规则分析信息
 */
typedef struct _MotionDetectionAnalytics_
{
    bool bEnable;
    bool bEnableSet; /* Modified: Flag to track if Enable was explicitly set */
    char     achSensitivity[128];       /* 灵敏度 */
    unsigned int auActiveCell[1024];    /* 激活的单元格 */
    char    achBaseStr[256];            /* 单元格转换后的Base编码 */
    char    achSchedule[2048];
}OnvifMotionDetection_S;

/**
 * @brief 遮挡报警规则分析信息
 */
#define ONVIF_ANALYTICS_POLYGON_POINT_NUM 20
typedef struct _TamperDetectionAnalytics_
{
    bool bEnable;
    bool bEnableSet; /* Modified: Flag to track if Enable was explicitly set */
    char     achSensitivity[128];       /* 灵敏度 */
    OnvifPoint_S stPolygon[ONVIF_ANALYTICS_POLYGON_POINT_NUM];          /* 遮挡检测区域,左下角为原点，四个坐标依次为矩形区域的:左下、右下、右上、左上 */
    int nPointNum;
    char achSchedule[2048];
}ONvifTamperDetection_S;

/**
 * @brief 区域入侵 rules info
 */
typedef struct _RegionDetectionAnalytics_
{
    bool bEnable;
    bool bEnableSet; /* Modified: Flag to track if Enable was explicitly set */
    char achSensitivity[128];
    OnvifPoint_S stPolygon[ONVIF_ANALYTICS_POLYGON_POINT_NUM];
    int nPointNum;
    char achDetectionTarget[128];
    char achSchedule[2048];
}OnvifRegionDetection_S;

/**
 * @brief Tripwire Detection (Line Crossing) rules info
 */
typedef struct _TripwireDetectionAnalytics_
{
    bool bEnable;
    bool bEnableSet; /* Modified: Flag to track if Enable was explicitly set */
    char achSensitivity[128];
    char achDirection[32];
    OnvifPoint_S stPolygon[ONVIF_ANALYTICS_POLYGON_POINT_NUM]; /* 2 points for Start/End */
    int nPointNum;
    char achDetectionTarget[128];
    char achSchedule[2048];
}OnvifTripwireDetection_S;

/**
 * @brief Intrusion Detection (Field Detection) rules info
 */
typedef struct _FieldDetectionAnalytics_
{
    bool bEnable;
    bool bEnableSet; /* Modified: Flag to track if Enable was explicitly set */
    char achSensitivity[128];
    char achTimeThreshold[128];
    OnvifPoint_S stPolygon[ONVIF_ANALYTICS_POLYGON_POINT_NUM];
    int nPointNum;
    char achDetectionTarget[128];
    char achSchedule[2048];
}OnvifFieldDetection_S;

/**
 * @brief Audio Anomaly rules info
 */
typedef struct _AudioAnomalyAnalytics_
{
    bool bEnable;
    bool bEnableSet; /* Modified: Flag to track if Enable was explicitly set */
    char achSensitivity[128];
    char achThreshold[128];
    char achSchedule[2048];
}OnvifAudioAnomaly_S;

/**
 * @brief Scene Change rules info
 */
typedef struct _SceneChangeAnalytics_
{
    bool bEnable;
    bool bEnableSet; /* Modified: Flag to track if Enable was explicitly set */
    char achSensitivity[128];
    char achSchedule[2048];
}OnvifSceneChange_S;

/**
 * @brief Face Detection rules info
 */
typedef struct _FaceDetectionAnalytics_
{
    bool bEnable;
    bool bEnableSet; /* Modified: Flag to track if Enable was explicitly set */
    char achSensitivity[128];
    OnvifPoint_S stPolygon[ONVIF_ANALYTICS_POLYGON_POINT_NUM];
    int nPointNum;
    char achSchedule[2048];
}OnvifFaceDetection_S;

/**
 * @brief Loitering Detection rules info
 */
typedef struct _LoiteringDetectionAnalytics_
{
    bool bEnable;
    bool bEnableSet; /* Modified: Flag to track if Enable was explicitly set */
    char achSensitivity[128];
    char achTimeThreshold[128];
    OnvifPoint_S stPolygon[ONVIF_ANALYTICS_POLYGON_POINT_NUM];
    int nPointNum;
    char achSchedule[2048];
}OnvifLoiteringDetection_S;

/**
 * @brief Crowd Gathering rules info
 */
typedef struct _CrowdGatheringAnalytics_
{
    bool bEnable;
    bool bEnableSet; /* Modified: Flag to track if Enable was explicitly set */
    char achSensitivity[128];
    char achObjectOccup[128];
    OnvifPoint_S stPolygon[ONVIF_ANALYTICS_POLYGON_POINT_NUM];
    int nPointNum;
    char achSchedule[2048];
}OnvifCrowdGathering_S;

/**
 * @brief Parking Detection rules info
 */
typedef struct _ParkingDetectionAnalytics_
{
    bool bEnable;
    bool bEnableSet; /* Modified: Flag to track if Enable was explicitly set */
    char achSensitivity[128];
    char achTimeThreshold[128];
    OnvifPoint_S stPolygon[ONVIF_ANALYTICS_POLYGON_POINT_NUM];
    int nPointNum;
    char achSchedule[2048];
}OnvifParkingDetection_S;

/**
 * @brief Unattended Object rules info
 */
typedef struct _UnattendedObjectAnalytics_
{
    bool bEnable;
    bool bEnableSet; /* Modified: Flag to track if Enable was explicitly set */
    char achSensitivity[128];
    char achTimeThreshold[128];
    OnvifPoint_S stPolygon[ONVIF_ANALYTICS_POLYGON_POINT_NUM];
    int nPointNum;
    char achSchedule[2048];
}OnvifUnattendedObject_S;

/**
 * @brief Object Removal rules info
 */
typedef struct _ObjectRemovalAnalytics_
{
    bool bEnable;
    bool bEnableSet; /* Modified: Flag to track if Enable was explicitly set */
    char achSensitivity[128];
    char achTimeThreshold[128];
    OnvifPoint_S stPolygon[ONVIF_ANALYTICS_POLYGON_POINT_NUM];
    int nPointNum;
    char achSchedule[2048];
}OnvifObjectRemoval_S;

/**
 * @brief Pet Recognition rules info
 */
typedef struct _PetRecognitionAnalytics_
{
    bool bEnable;
    bool bEnableSet; /* Modified: Flag to track if Enable was explicitly set */
    char achSensitivity[128];
    OnvifPoint_S stPolygon[ONVIF_ANALYTICS_POLYGON_POINT_NUM];
    int nPointNum;
    char achSchedule[2048];
}OnvifPetRecognition_S;

/**
 * @brief Face Capture rules info
 */
typedef struct _FaceCaptureAnalytics_
{
    bool bEnable;
    bool bEnableSet; /* Modified: Flag to track if Enable was explicitly set */
    char achSensitivity[128];
    char achInterval[128];
    OnvifPoint_S stPolygon[ONVIF_ANALYTICS_POLYGON_POINT_NUM];
    int nPointNum;
    char achSchedule[2048];
}OnvifFaceCapture_S;

#ifdef __cplusplus
}
#endif

#endif
