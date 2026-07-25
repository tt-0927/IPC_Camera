/**
 * @FilePath     : rtsp_server.h
 * @Author       : zhouzirui
 * @Date         : 2025-03-29 10:05:19
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-06-10 11:19:09
 * @Description  : RTSP服务器
 */
#pragma once

#include <iostream>
#include <mutex>
#include <thread>
#include <atomic>
#include <vector>
#include <deque>
#include <memory>
#include <condition_variable>
#include <stdexcept>
#include <map>
#include <functional>
#include "dlog.h"
#include "IpcRet.h"
#include "video_define.h"
#include "audio_define.h"

#include "network_manage.h"
#include "network_define.h"
#include "qos_manage.h"
#include "frame_queue.h"

extern "C"
{
#include "rtspServer_base.h"
#include "list_use_lock.h"
}

/*最大客户端数量*/
#define MAX_CLIENT_NUM (4)
#if defined(DEVICE_TV_3882TI) || defined(DEVICE_TV_3881T)
    /*队列存储最大视频帧数*/
    #define MAX_VIDEO_FRAME (32)
    /*队列存储最大音频帧数*/
    #define MAX_AUDIO_FRAME (16)
#else
    /*队列存储最大视频帧数*/
    #define MAX_VIDEO_FRAME (4)
    /*队列存储最大音频帧数*/
    #define MAX_AUDIO_FRAME (4)
#endif
/*RTSP码流地址*/
#define RTSP_URL_DEFAULT "rtsp://%s:%d/Streaming/Channels/%d"
/*RTSP认证版码流地址*/
#define RTSP_URL_AUTHENTICATION_DEFAULT "rtsp://%s:%s@%s:%d/Streaming/Channels/%d"

/* RTSP通道号枚举 */
typedef enum
{
    RTSP_CHN_MAIN = 0, // 主码流
    RTSP_CHN_SUB,      // 子码流
    RTSP_CHN_MAX,
} RTSP_CHN_E;

/* 直播流信息结构体 */
typedef struct
{
    char streamName[STREAM_NAME_MAX];
    int nVideoPort;
    int nAudioPort;
    char ip[64];
    int videosocket;
    int audiosocket;
    List_LockHandle_t* listVideoHandle;
    List_LockHandle_t* listAudioHandle;
    std::unique_ptr<CThreadSafeFrameQueue> videoQueue; /* 视频帧队列 */
    std::unique_ptr<CThreadSafeFrameQueue> audioQueue; /* 音频帧队列 */
    volatile int request;
    /* I帧请求标志, volatile确保多线程可见性 */
    volatile int requestIFrame;
    int curframe;
    int frameCount;
    char username[64];
    void* streamclientHandle;
    int nPort;
    char achUrl[64];
    float fFps; /* 视频帧率 */
} Live_Stream_Info_t;

/* 直播流RTSP信息结构体 */
typedef struct
{
    RtSpServerHandle_t pServerHandle;
    int nPort;
    Live_Stream_Info_t* listLive[RTSP_CHN_MAX];
} LIVE_RTSP_S;

/* 现场直播流消息结构体 */
typedef struct Conference_Live_Messege
{
    char username[64];
    void* video_index;
    void* audio_index;
    void* streamclientHandle;

} Conference_Live_Messege_t;

/*定义请求IDR帧函数指针类型*/
using RequestIdrCallback = std::function<void(int nChannel, void* pUserData)>;

class CRtspServer : public CSingleton<CRtspServer>
{
private:
    CRtspServer();

public:
    virtual ~CRtspServer();
    /* 允许 Singleton 访问私有构造函数 */
    friend class CSingleton<CRtspServer>;

    /**
     * @brief       : 初始化RTSP服务器（参数暂时使用默认值）
     * @author      : zhouzirui
     * @return       {*} RETURN_ERROR：失败
     */
    IpcRet_E init();

    /**
     * @brief       : 反初始化
     * @author      : zhouzirui
     * @return       {*}非0：失败
     */
    IpcRet_E deinit();

    /**
     * @brief   : 获取初始化状态
     * @return   {bool}0：假，1：真
     */
    bool isInit();

    /**
     * @brief   : 重新启动RTSP服务器
     * @return   {IpcRet_E}0：成功，非0：失败
     */
    IpcRet_E reboot();

    void stop()
    {
        m_bInitFlag.store(false);
    }
    void start()
    {
        m_bInitFlag.store(true);
    }

    /**
     * @brief       : 外部送视频数据
     * @author      : zhouzirui
     * @param        {int} nChannel：码流通道号，第一码流：0，第二码流：1，以此类推
     * @param        {VideoFrame_S} *pVideoFrame：视频帧数据指针
     * @return       {*}0：成功 非0：失败
     */
    int sendVideoData(int nChannel, Video_NS::VideoFrame_S* pVideoFrame);

    /**
     * @brief       : 外部送音频数据
     * @author      : zhouzirui
     * @param        {int} nChannel：码流通道号，第一码流：0，第二码流：1，以此类推
     * @param        {AudioFrame_S} *pAudioFrame：音频帧数据指针
     * @return       {*}0：成功 非0：失败
     */
    int sendAudioData(int nChannel, Audio_NS::AudioFrame_S* pAudioFrame);

    /**
     * @brief   : 设置视频配置
     * @param    {vector<Video_NS::VideoConfig_S>} &vstVideoConfig：视频配置
     * @return   {int}0：成功 非0：失败
     */
    int setVideoConfig(const std::vector<Video_NS::VideoConfig_S>& vstVideoConfig);

    /**
     * @brief   : 设置音频配置
     * @param    {AudioConfig_S} &stAudioConfig：音频配置
     * @return   {int}0：成功 非0：失败
     */
    int setAudioConfig(const Audio_NS::AudioConfig_S& stAudioConfig);

    /**
     * @brief   : 更新网络配置
     * @note    : 更新内部存储的URL地址
     * @param    {Info_S} &stInfo 网络配置信息
     * @return   {int} 0：成功 非0：失败
     */
    int updateNetworkConfig(const Network::Info_S& stInfo);

    /**
     * @brief   : 设置 RTSP 端口
     * @param    {int} &nPort：RTSP 端口
     * @return   {int}0：成功 非0：失败
     */
    int setPort(const int& nPort);

    /**
     * @brief   : 设置 RTSP qos Dscp 值
     * @param    {int} &nDscp：RTSP qos Dscp 值
     * @return   {int}0：成功 非0：失败
     */
    int setQosDscp(const int& nDscp);

    /**
     * @brief   : 获取rtsp码流地址
     * @note    : 如果没有开启加密鉴权，则返回普通的码流地址
     * @param    {int} nChn 码流ID，第几码流，从0开始
     * @param    {bool} bAuth 是否获取加密鉴权版本的URL
     * @return   {char *} 码流地址
     */
    char* getRtspUrl(int nChn, bool bAuth = false);

    /**
     * @brief       : 设置请求I帧的回调函数
     * @param        {RequestIdrCallback} callback : 回调函数指针
     * @param        {void*} pUserData : 用户自定义数据指针
     * @return       {int} 0：成功 非0：失败
     */
    int setRequestIdrCallback(const RequestIdrCallback& callback, void* pUserData = nullptr);

    /**
     * @brief       : 触发请求I帧回调
     * @param        {int} nChannel : 通道号
     * @return       {int} 0：成功 非0：失败
     */
    int triggerRequestIdr(int nChannel);

    /**
     * @brief   : 重置上次请求IDR帧时间
     * @note    : 用于系统时间更新后，时间往前倒的情况
     * @return   {int} 0：成功 非0：失败
     */
    int reset_lastIdrRequestTime();

    /**
     * @brief   : 更新用户信息（账号、密码）
     * @param    {string} strUser 账号
     * @param    {string} strPwd 密码
     * @param    {bool} bReboot 是否重启
     * @return   {int} 0：成功 非0：失败
     */
    int update_userInfo(std::string strUser, std::string strPwd, bool bReboot);

    /**
     * @description  : 更新rtsp摘要算法
     * @return        {*}
     */
    int updateRtspDigestAlgorithm();

public:
    // info /*----------------------- 模块句柄 -----------------------*/
    /*直播RTSP信息句柄*/
    LIVE_RTSP_S* m_pLiveInfo;

private:
    // info /*----------------------- 模块句柄 -----------------------*/

    // info /*----------------------- 参数变量 -----------------------*/
    /* 是否初始化 */
    std::atomic<bool> m_bInitFlag{ false };
    /*RTSP客户端信息*/
    Rtsp_Create_Info_t m_stClientInfo[RTSP_CHN_MAX];
    /*RTSP端口*/
    int m_nRtspPort = 554;
    /*音频格式*/
    int m_nAudioFormat = 0;
    /*音频开关*/
    int m_nAudioSwitch = 0;
    /*RTSP认证开关*/
    bool m_bAuthentication = 0;
    /*用户名*/
    std::string m_strUser = USER_DEFAULT_NAME;
    /*密码*/
    std::string m_strPwd = USER_DEFAULT_PASSWD;
    /*视频格式*/
    std::string m_strVencType = "H.264";
    /*每个通道是否已发送过SPS*/
    // std::map<int, bool> m_mapIsFirstFrame;
    /*视频配置*/
    std::vector<Video_NS::VideoConfig_S> m_vstVideoConfig;
    /*音频配置*/
    Audio_NS::AudioConfig_S m_stAudioConfig;
    /*视频配置文件路径*/
    std::string m_strVideoConfigPath;
    /*音频配置文件路径*/
    std::string m_strAudioConfigPath;
    /* 端口配置文件路径 */
    std::string m_strPortPath;
    // /* 端口配置 */
    // Network::PortConfig_S m_stPortConfig;
    /*请求I帧回调函数*/
    RequestIdrCallback m_requestIdrCallback;
    /*请求I帧回调函数的用户自定义数据指针*/
    void* m_pCallbackUserData;
    /*控制操作互斥锁*/
    std::mutex m_mutexCtrl;
    /*qos配置*/
    Network::QosConfigInfo_S m_stQosConfigInfo;
    /*媒体DSCP*/
    int m_nMediaDscp;
    /* 记录上一次触发 IDR 请求的时间点 */
    std::unordered_map<int, long long> m_lastIdrRequestTimeMap;
    /* 触发 IDR 请求最小时间间隔 */
    long long m_minIdrInterval{ 30 }; // 30 ms
    /* Url地址 */
    std::unordered_map<int, std::string> m_rtspUrlMap;
    /* 上次更新URL时的IP，用于检测IP变化 */
    std::string m_strLastIp;
    /* RTSP摘要算法 */
    int m_nRtspDigestAlgorithm = 0;
};
