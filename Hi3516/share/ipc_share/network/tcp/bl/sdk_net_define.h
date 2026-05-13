/*
 * @Author       : EasonLu
 * @Date         : 2023-04-27 19:05:41
 * @LastEditors: 李辉 lihui@kfb.cn
 * @LastEditTime: 2024-08-13 17:59:47
 * @FilePath     : sdk_net_define.h
 * @Description  : 网络库的数据结构定义
 */
#ifndef _SDK_NET_DEFINE_H_
#define _SDK_NET_DEFINE_H_

#include <pthread.h>

#include "share_socket.h"

#define SDK_AACAUDIO_STREAM  1000001 /*发送AAC音频数据*/
#define SDK_H264_STREAM      1000002
#define SDK_SET_H264_FPS     1000003 /* 发送SDK_H264_STREAM命令，视频数据的fps信息 */
#define SDK_SET_VIDEO_INFO   1000004 /* 发送编码信息-视频-发送的结构体 Sdk_VideoInfo_S */
#define SDK_SET_AUDIO_INFO   1000005 /* 发送编码信息-音频-发送的结构体 Sdk_AudioInfo_S */
#define SDK_AUDIO_STREAM     1000006 /*音频数据 搭配SDK_SET_AUDIO_INFO使用*/
#define SDK_SUBTITLE_STREAM  1000007 /* 字幕数据 */
#define SDK_NET_HEARTBIT_CMD 30032
#define SDK_STANDARD_CODE    0xffffff;

#ifndef MAX_IPV6_BUF_LEN
    #define MAX_IPV6_BUF_LEN 64
#endif

typedef void* Sdk_Net_Handle_t;
typedef void* Sdk_ServerNet_Handle_t;
typedef void* (*pthread_fun_sdk)(void*);

/* 强制1字节对齐 */
#pragma pack(1)
/* 视频编码信息 */
typedef struct _Sdk_VideoInfo_
{
    int nVencWidth;  /* 宽 */
    int nVencHeight; /* 高 */
    int nFps;        /* 帧率 */
    int nCodeID;     /* ffmpeg-编码器ID */
} Sdk_VideoInfo_S;
#pragma pack()

#pragma pack(1)
/* 音频编码信息 */
typedef struct _Sdk_AudioInfo_
{
    int nSampleRate;    /* 采样率 */
    int nCodeID;        /* ffmpeg-编码器ID AV_CODEC_ID_AAC*/
    int nSampleFmt;     /*采样格式 AV_SAMPLE_FMT_S16*/
    int nChannel;       /*采样通道 1单通道，2是双通道*/
} Sdk_AudioInfo_S;
#pragma pack()

typedef enum Protocol_Type
{
    // 内部私有协议
    SDK_PRIVATE_DEFAULT  = 0,
    // 公司对接的标准协议，统一规范
    SDK_COMPANY_STANDARD = 1,
} Protocol_Type_E;

typedef enum Net_Status
{
    SDK_NET_DISCONNECT = 0,
    SDK_NET_CONNECT    = 1,
    SDK_NET_ERROR,

} Net_Status_t;

typedef enum Value_Type
{
    SDK_INT = 0,
    SDK_SHORT,
    SDK_CHAR,
    SDK_FLOAT,
    SDK_DOUBLE,
    SDK_FILE,
    SDK_UINT,
} Value_Type_t;

// 信息头结构
typedef struct Net_Msg_Head
{
    char identifier[4];    //@#$&	标识符
    int  version;
    int  load_len;
    int  msg_code;
    // unsigned short load_len;
    // unsigned short msg_code;
    int  reserve;
} Net_Msg_Head_t;

// 强制1字节对齐
#pragma pack(1)

typedef struct Net_ComPanyStandard_Head
{
    // 公司名称BL
    char           u16CompanyName[2];
    // 项目代号
    unsigned short u16ProjectCode;
    // 数据长度
    unsigned short u16DataLen;
    // 扩展字段，代表头要扩展几个字节，1代表可扩展1字节，最多可扩展256，不用填0即可
    unsigned char  u16ExpansionLen;
} Net_ComPanyStandard_Head_S;

#pragma pack()

typedef struct _NetCallBackMesssage_
{
    void*            InParam;      // 用来回调传出去的参数
    int              Code;         // 哪种命令
    int              nLen;
    char*            recvvalue;    // 接收的内容
    Sdk_Net_Handle_t sOperHandle;
    // 私有协议网络接收头的保留位
    Protocol_Type_E  enProtocolType;    // 协议类型
    int              nReserve;
} NetCallbackMsg_t;

typedef int (*NetDealbitFunc)(NetCallbackMsg_t*);
typedef int (*NetStatusFunc)(Net_Status_t status, Sdk_Net_Handle_t handle, void* inparam);
typedef int (*NetLogbackmsg)(const char* format, ...);                                                             // 用于输出调试信息的函数指针
typedef int (*NetGetHeartMsg)(char* messege, int nLen, Sdk_Net_Handle_t handle, void* inparam, int* outPutlen);    // 用于输出调试信息的函数指针

typedef struct InparamClientNet
{
    char            ip[MAX_IPV6_BUF_LEN];
    int             nPort;
    NetDealbitFunc  cmdfun;                  // 命令处理回调
    NetStatusFunc   statusFun;               // 网络连接状态通知（当服务器状态变化时）
    NetLogbackmsg   logFun;                  // 输出错误信息
    NetGetHeartMsg  heartmsg;                // 服务器发送给客户端的信息
    void*           param;                   // 创建传进去的参数，回调带上来
    int             overtime;                // 网络发送超时，以ms为单位，建议2秒
    int             nReconnect;              /* 是否重连 */
    int             nReconnectMax;           /* 重连最大次数，默认为0，一直重连 */
    int             nReconnectGap;           /* 重连间隔时间（单位：毫秒）默认2秒 */
    int             asynchronous_listnum;    // 异步发送最大缓冲，没有设置默认15
    int             asynchronous;            // 异步发送吗
    int             asynchronous_time;       // 异步发送链表取的时间，发送间隔长短设置，在100ms~2S之间
    Protocol_Type_E enProtocolType;          // 协议类型
    unsigned short  u16ProjectCode;          // 项目代号
    /*无感，心跳单向重连的状态*/
    int             nSetStatus;
    /* NOTE:客户端为阻塞接收数据 */
    /* [客户端][可选参数]接收数据超时时间（单位：毫秒）默认为心跳时间+1秒 */
    int             nRecvTimeout;
    /* [客户端][可选参数]接收数据超时时间次数（默认为3次） */
    int             nRecvTimeoutTimes;
} InparamClientNet_t;

typedef struct tar_messege
{
    Sdk_Net_Handle_t handle;         // 初始化时的操作句柄
    int              nodeindex;      // 默认为0，除非字段名完全一样，0代表第一个，1代表第2个，
    void**           valueHandle;    // 组装发送内容的句柄
} tar_messege_t;

typedef struct InparamServerNet
{
    short           nPort;
    int             asynchronous;            // 异步发送吗
    int             asynchronous_listnum;    // 异步发送最大缓冲，没有设置默认15
    int             asynchronous_time;       // 异步发送链表取的时间，发送间隔长短设置，在20ms~2S之间
    NetDealbitFunc  cmdfun;                  // 命令处理回调
    NetStatusFunc   statusFun;               // 网络连接状态通知（当客户端有变化时）
    NetLogbackmsg   logFun;                  // 输出错误信息
    NetGetHeartMsg  heartmsg;                // 服务器客户端心跳
    void*           param;                   // 传进去的参数，回调带上来
    int             overtime;                // 网络发送超时，以ms为单位
    int             nConnectMaxNum;          // 超过这个链接的服务器不处理直接踢掉
    int             support_ipv6;
    Protocol_Type_E enProtocolType;          // 协议类型
    unsigned short  u16ProjectCode;          // 项目代号
} InparamServerNet_t;

typedef struct OfferUserMessege
{
    char ip[MAX_IPV6_BUF_LEN];
    int  nPort;
} OfferUserMessege_t;

typedef struct Protocol_Info_Head
{
    Net_Msg_Head_t             stPrivateSendMsgHead;
    Net_Msg_Head_t             stPrivateRecvMsgHead;
    Net_ComPanyStandard_Head_S stStandardSendMsgHead;
    Net_ComPanyStandard_Head_S stStandardRecvMsgHead;

} Protocol_Info_Head_S;

typedef struct Net_Opreate_Hanle
{
    int                  socket;
    OfferUserMessege_t   usermessege;
    void*                pSendXmlHandle; /* xml句柄Xml_DocPtr_t(参考xml_define.h) */
    void*                pRecvXmlHandle; /* xml句柄Xml_DocPtr_t(参考xml_define.h) */
    char*                pRecvMessege;
    InparamClientNet_t   inParam;
    pthread_t            client_tid;
    pthread_t            heart_tid;
    pthread_t            send_tid;                 // 异步发送时用
    List_LockHandle_t*   asynchronous_sendlist;    // 异步发送链表
    pthread_mutex_t      lock;                     // 互斥锁
    pthread_mutex_t      netlock;                  // 网络互斥锁
    Net_Status_t         status;                   // 代表是否要断开退出连接
    Net_Status_t         current_status;           // 代表当前连接状态
    void*                serverHandle;             // 如果是服务器，则此为服务器句柄
    void*                serverClientParam;        // 如果是服务器，则代表连接后用户自己设置进来的指针，方便用户管理
    pthread_cond_t       condHeart;
    pthread_mutex_t      lockHeart;
    Protocol_Info_Head_S stProtocolHeadInfo;
    int                  nReserve;    // 接收保留值
} Net_Opreate_Hanle_S;

#endif
