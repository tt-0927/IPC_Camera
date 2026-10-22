/*
 * @Author       : EasonLu
 * @Date         : 2024-05-09 19:38:41
 * @LastEditors  : EasonLu
 * @LastEditTime : 2024-08-13 10:03:33
 * @FilePath     : bl_event.h
 * @Description  : 公共事件枚举值
 */
#ifndef __BL_EVENT_H__
#define __BL_EVENT_H__

/* 日志类型枚举 */
typedef enum _MQTT_LOG_TYPE_
{
    MQTT_TYPE_DEFAULT = 0,       /* 默认 */
    MQTT_TYPE_EXCEPTION = 1,     /* 异常 */
    MQTT_TYPE_BUSSINESS = 2,     /* 业务 */
    MQTT_TYPE_OPERATION = 3,     /* 操作 */
    MQTT_TYPE_CONFIGURATION = 4, /* 配置 */
    MQTT_TYPE_MAX = 5,           /* 最大值 */
} MqttLogType_E;

/* 日志等级枚举 */
typedef enum _MQTT_LOG_LEVEL_
{
    MQTT_LEVEL_DEFAULT = 0,  /* 默认 */
    MQTT_LEVEL_SLIGHT = 1,   /* 轻微 */
    MQTT_LEVEL_COMMONLY = 2, /* 一般 */
    MQTT_LEVEL_SERIOUS = 3,  /* 严重 */
    MQTT_LEVEL_DEADLY = 4,   /* 致命 */
    MQTT_LEVEL_MAX = 5,      /* 最大值 */
} MqttLogLevel_E;

/* 日志维度枚举 */
typedef enum _MQTT_LOG_SOURCE_
{
    MQTT_SOURCE_DEFAULT = 0,              /* 默认 */
    MQTT_SOURCE_ACTION = 1,               /* 功能操作/点击 */
    MQTT_SOURCE_TASK = 2,                 /* 执行任务 */
    MQTT_SOURCE_DURATION_OF_USE = 3,      /* 设备使用时长 */
    MQTT_SOURCE_OTHER_USER = 4,           /* 其他（用户操作日志） */
    MQTT_SOURCE_ABNORMAL_EXIT = 5,        /* 异常退出 */
    MQTT_SOURCE_ABNORMAL_POWER_OFF = 6,   /* 异常断电 */
    MQTT_SOURCE_PROGRAM_ERROR = 7,        /* 程序报错 */
    MQTT_SOURCE_DB_EXCEPTION = 8,         /* 数据库异常 */
    MQTT_SOURCE_DEVICE_STATUS = 9,        /* 设备状态 */
    MQTT_SOURCE_NETWORK_STATUS = 10,      /* 联网状态 */
    MQTT_SOURCE_OTHER_LOG_EXCEPTION = 11, /* 其他（日志异常） */
} MqttLogSource_E;

typedef struct _MQTT_MSG_S_
{
    MqttLogType_E enType;     /* 消息类型 */
    MqttLogLevel_E enLevel;   /* 消息级别 */
    MqttLogSource_E enSource; /* 消息维度 */
    char *pMsg;               /* 消息内容 */
    int nLen;                 /* 消息长度 */
} MqttMsg_S;

/* 对应mqtt_conf.ini的字段 */
typedef struct _Mqtt_Config_S_
{
    int nPorjectID;           /* 项目ID */
    char achSN[128];          /* 设备唯一标识码 */
    char achProductName[256]; /* 产品线名称 */
    char achDeviceName[256];  /* 产品型号 */
    char achDeviceCode[256];  /* 产品Code值 */
    char achLogin[64];        /* 当前产品线归属的账户 */
    char achPasswd[64];       /* 登录账户的密码 */
    int bLoad;                /* 加载标记位，1为已经加载过配置文件 */
} MqttConfig_S;

/* NOTE
    需要划分功能区间（后续自行划分）
    运维事件区间：11000-13000(11000~12044为自定义的运维事件)
 */

typedef enum BlEvent_E
{
    BL_OPERATION_START_CODE = 11000, /* 起始区间码 */
    /* 自定义运维事件,千位和百位为事件维度,十位为事件类型,个位为事件等级 */
    BL_OPERATION_CUSTOM = 11000,
    BL_OPERATION_CUSTOM_MAX = 12044, /* 自定义运维事件最大值 */
    /* NOTE 以下的固定事件，需要在回调中响应并操作 */
    BL_OPERATION_START_RECORD = 12100,        /* 记录开始录制(运行，默认) */
    BL_OPERATION_STOP_RECORD = 12101,         /* 记录停止录制(运行，默认) */
    BL_OPERATION_RESOLUTION_CHANGE = 12102,   /* stream视频输入源分辨率变更(运行，默认) */
    BL_OPERATION_INSTREAM_TIMEOUT = 12103,    /* stream视频输入超时(异常，严重) */
    BL_OPERATION_INSTREAM_GET_FAILED = 12104, /* stream视频输入获取失败(异常，严重) */
    BL_OPERATION_HDMI_CONNECT = 12105,        /* stream接入HDMI(运行，默认) */
    BL_OPERATION_HDMI_DISCONNECT = 12106,     /* stream拔出HDMI(运行，默认) */

    /* 运维平台上传日志/配置文件 */
    BL_OPERATION_MAINTEMAMCE_GETINFO = 12200,  /* 运维平台主动发送至control，获取设备信息命令 */
    BL_OPERATION_SET_SN = 12997,               /* 设置机器的识别字段值 */
    BL_OPERATION_LOAD_INI = 12998,             /* 通知运维进程需要加载的配置文件绝对路径 */
    BL_OPERATION_UPGRADEPACK_GETINFO = 12995,  /* 获取最新升级包信息 */
    BL_OPERATION_UPGRADEPACK_DOWNLOAD = 12996, /* 下载升级包 */

    BL_OPERATION_END_CODE = 13000, /* 结束区间码 */
} BlEvent_E;

/**
 * @brief  编码自定义运维事件消息码
 * @param  [MqttLogType_E] enType - 事件类型
 * @param  [MqttLogLevel_E] enLevel - 事件等级
 * @param  [int] *pOutCode - 输出消息码
 * @return [*]
 * @author EasonLu
 * @note   默认事件维度为MQTT_SOURCE_OTHER_USER
 *         后续将弃用，适配调用此接口的旧代码
 *         后续请使用bl_mqtt_enc_msgCode函数进行编码
 */
int bl_event_encode_msgCode(MqttLogType_E enType, MqttLogLevel_E enLevel, int *pOutCode);

/**
 * @brief  解码自定义运维事件消息码
 * @param  [int] nCode - 消息码
 * @param  [MqttLogType_E] *pOutType - 输出事件类型
 * @param  [MqttLogLevel_E] *pOutLevel - 输出事件等级
 * @return [*]
 * @author EasonLu
 * @note   后续将弃用，适配调用此接口的旧代码
 *         后续请使用bl_mqtt_dec_msgCode函数进行解码
 */
int bl_event_decode_msgCode(int nCode, MqttLogType_E *pOutType, MqttLogLevel_E *pOutLevel);

/**
 * @brief  编码自定义运维事件消息码
 * @param  [MqttMsg_S] stMsg - Mqtt消息数据结构
 * @param  [int] *pOutCode - 输出消息码
 * @return [*]
 * @author EasonLu
 * @note
 */
int bl_mqtt_enc_msgCode(
    MqttMsg_S stMsg,
    int *pOutCode);

/**
 * @brief  解码自定义运维事件消息码
 * @param  [int] nCode - 消息码
 * @param  [MqttMsg_S] *pGetMsg - Mqtt消息数据结构指针
 * @return [*]
 * @author EasonLu
 * @note
 */
int bl_mqtt_dec_msgCode(int nCode, MqttMsg_S *pGetMsg);

#endif // __BL_EVENT_H__
