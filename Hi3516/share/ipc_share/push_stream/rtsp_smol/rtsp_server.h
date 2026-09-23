/**
 * @FilePath     : rtsp_server.h
 * @Author       : zhouzr@kfb.cn
 * @Description  : RTSP服务器（smolrtsp 实现，源兼容旧 live555 封装）
 *
 * 本文件是产品侧的**薄封装**：类名、方法签名、URL 语义与旧实现保持一致，
 * 调用方（preview/tvsdk/onvif/network_task/user_task/qos/...）无需改动。
 *
 * 真正的协议实现全部在独立库 ipc_rtspserver 内：
 *   连接管理 / 会话 / 鉴权 / RTP-RTCP / 媒体分发 / 背压 / 指标
 * 本封装只负责：
 *   - 读取产品配置文件（视频/音频/端口/QoS）
 *   - 把 Video_NS / Audio_NS / Network 结构体映射为库的配置与帧视图
 *   - 把库日志转发到 dlog
 *   - 把 IDR 请求回调转发到业务线程
 *   - 缓存对外展示 URL（保持 char* 返回语义）
 *
 * 编译开关：由父工程 CMake 变量 IPC_RTSP_BACKEND 选择本目录或旧 rtsp/ 目录，
 * 两者不会同时参与编译。
 */
#pragma once

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <deque>
#include <functional>
#include <iostream>
#include <map>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

#include "IpcRet.h"
#include "Singleton.h"
#include "audio_define.h"
#include "dlog.h"
#include "user_define.h"
#include "video_define.h"

#include "network_define.h"

/*
 * 连接上限默认值定义在 custom_define.h，业务编译时可通过能力宏覆盖；
 * 这里只按设备和码率计算当前生效值。首个 SETUP 前由库侧做准入，
 * 超限时保留已有连接并返回 453。
 */
#ifndef CAP_RTSP_HIGH_CONCURRENCY
#define CAP_RTSP_HIGH_CONCURRENCY 0
#endif
#ifndef RTSP_DEFAULT_GLOBAL_MAX_CLIENT
#define RTSP_DEFAULT_GLOBAL_MAX_CLIENT 4
#endif
#ifndef RTSP_DEFAULT_STREAM_MAX_CLIENT
#define RTSP_DEFAULT_STREAM_MAX_CLIENT 4
#endif
#ifndef RTSP_MAIN_CLIENT_LIMIT_8M
#define RTSP_MAIN_CLIENT_LIMIT_8M 2
#endif
#ifndef RTSP_MAIN_CLIENT_LIMIT_16M
#define RTSP_MAIN_CLIENT_LIMIT_16M 1
#endif

/* 每通道最大客户端数量（兼容既有 Rtsp_Create_Info_t 参数语义）。 */
#define MAX_CLIENT_NUM RTSP_DEFAULT_STREAM_MAX_CLIENT

/* 设备能力等级乘数：TV-3881T/TV-3882TI 由能力宏提升到 8 路总额。 */
constexpr int RTSP_CLIENT_CAPACITY_MULTIPLIER = CAP_RTSP_HIGH_CONCURRENCY ? 2 : 1;
constexpr int RTSP_GLOBAL_MAX_CLIENT = RTSP_DEFAULT_GLOBAL_MAX_CLIENT * RTSP_CLIENT_CAPACITY_MULTIPLIER;

/* 主码流码率档位（配置单位 kbps）：低于 8 Mbps 为默认，8 Mbps/16 Mbps 收紧。 */
constexpr int RTSP_MAIN_BITRATE_THRESHOLD_8M = 8192;
constexpr int RTSP_MAIN_BITRATE_THRESHOLD_16M = 16384;
constexpr int RTSP_MAIN_CLIENT_LIMIT_DEFAULT = RTSP_DEFAULT_STREAM_MAX_CLIENT;

/*
 * 背压预算采用"积压时长"判定（与 ZLMediaKit/gst-rtsp-server 同口径，见
 * 21_资源预算与背压设计.md）：TCP 客户端积压媒体落后直播超过软限时长进入拥塞态
 * （丢 P 帧、只发关键帧），超过硬限时长断开。时长判定与分辨率、码率、GOP、单帧
 * 大小无关，换镜头/改码率无需重调；字节护栏仅防异常码率配置撑爆内存。
 */
/* 客户端输出积压软限（毫秒）：所有型号统一，不随码率/分辨率调整。 */
constexpr std::uint32_t RTSP_APP_CLIENT_SOFT_BACKLOG_MS = 1500U;
/* 客户端输出积压硬限（毫秒）：断开持续漏水但仍有发送进展的客户端。 */
constexpr std::uint32_t RTSP_APP_CLIENT_HARD_BACKLOG_MS = 3000U;

#if defined(DEVICE_TV_3882TI) || defined(DEVICE_TV_3881T)
/* 高内存型号：更宽的内存护栏与队列深度。 */
constexpr std::size_t RTSP_APP_CLIENT_OUTPUT_GUARD_BYTES = 6U * 1024U * 1024U;
constexpr std::size_t RTSP_VIDEO_QUEUE_DEPTH = 32U;
constexpr std::size_t RTSP_AUDIO_QUEUE_DEPTH = 16U;
constexpr std::size_t RTSP_VIDEO_QUEUE_MAX_BYTES = 4U * 1024U * 1024U;
constexpr std::size_t RTSP_AUDIO_QUEUE_MAX_BYTES = 256U * 1024U;
#else
/* 普通设备：收紧内存护栏与队列深度，降低常驻内存。 */
constexpr std::size_t RTSP_APP_CLIENT_OUTPUT_GUARD_BYTES = 4U * 1024U * 1024U;
/* memory: 普通设备保持6帧上限，避免用队列换取延迟时放大常驻内存。 */
constexpr std::size_t RTSP_VIDEO_QUEUE_DEPTH = 6U;
constexpr std::size_t RTSP_AUDIO_QUEUE_DEPTH = 6U;
/* memory: 每路视频最多缓存1MiB，避免单个异常I帧突破板端内存预算。 */
constexpr std::size_t RTSP_VIDEO_QUEUE_MAX_BYTES = 1U * 1024U * 1024U;
constexpr std::size_t RTSP_AUDIO_QUEUE_MAX_BYTES = 64U * 1024U;
#endif

/*RTSP码流地址*/
#define RTSP_URL_DEFAULT                "rtsp://%s:%d/Streaming/Channels/%d"
/*RTSP认证版码流地址*/
#define RTSP_URL_AUTHENTICATION_DEFAULT "rtsp://%s:%s@%s:%d/Streaming/Channels/%d"

/* RTSP通道号枚举 */
typedef enum
{
    RTSP_CHN_MAIN = 0, // 主码流
    RTSP_CHN_SUB,      // 子码流
    RTSP_CHN_MAX,
} RTSP_CHN_E;

/*定义请求IDR帧函数指针类型（保持旧签名）*/
using RequestIdrCallback = std::function<void(int nChannel, void *pUserData)>;

/*
 * 库配置类型仅在此前置声明，供私有实现方法签名引用；
 * 本头文件仍不 include ipc_rtspserver 公共头，库类型定义只出现在 .cpp 中。
 */
namespace ipc_rtsp
{
struct ServerConfig;
struct StreamConfig;
} // namespace ipc_rtsp

/**
 * RTSP 服务器单例。
 *
 * 线程安全：init/deinit/reboot 由控制任务串行调用；sendVideoData/sendAudioData
 * 由编码线程高频调用。库内部对生产者线程与 I/O 线程做了隔离，这里只做结构体映射。
 */
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
     * @return       {IpcRet_E} OK：成功，其他：失败
     */
    IpcRet_E init();

    /**
     * @brief       : 反初始化
     * @return       {IpcRet_E} OK：成功，其他：失败
     */
    IpcRet_E deinit();

    /**
     * @brief   : 获取初始化状态
     * @return   {bool} false：未初始化，true：已初始化
     */
    bool isInit();

    /**
     * @brief   : 重新启动RTSP服务器
     * @return   {IpcRet_E} OK：成功，其他：失败
     */
    IpcRet_E reboot();

    /**
     * @brief : 停止/恢复送帧（不关闭监听 socket，保持与旧实现一致的语义）
     * @note  : 只影响媒体入队，不影响已有连接的 RTSP 控制面。
     */
    void stop()
    {
        m_bInitFlag.store(false);
        (void) stopIngest();
    }

    void start()
    {
        m_bInitFlag.store(true);
    }

    /**
     * @brief       : 外部送视频数据（裸帧结构体）
     * @param        {int} nChannel：码流通道号，主码流：0，子码流：1
     * @param        {Video_NS::VideoFrame_S} *pVideoFrame：视频帧数据
     * @return       {int} 0：成功 非0：失败
     */
    int sendVideoData(int nChannel, Video_NS::VideoFrame_S *pVideoFrame);

    /**
     * @brief   : 发送 VENC 只读帧视图
     * @param   {int} nChannel：RTSP 通道号
     * @param   {uint8_t*} pData：VENC pack 数据地址，仅在本次调用期间有效
     * @param   {int} nDataLen：编码数据长度
     * @param   {VideoCodec_E} enVideoCodec：视频编码格式
     * @param   {NalType_E} eType：首个 NAL 类型
     * @return  {int} 0：成功，非0：失败
     * @note    : 库在有订阅者时复制一次，不保存 VENC 原始指针；未订阅时零拷贝丢弃。
     */
    int sendVideoData(int nChannel, const uint8_t *pData, int nDataLen, Video_NS::VideoCodec_E enVideoCodec, Video_NS::NalType_E eType);

    /**
     * @brief   : 发送共享媒体帧（引用计数入队，与 RTMP/录制共享同一份 buffer）
     * @return  {int} 0：成功，非0：失败
     */
    int sendVideoData(int nChannel,
                      const Video_NS::SharedMediaFrame_S &stSharedFrame,
                      Video_NS::VideoCodec_E enVideoCodec,
                      Video_NS::NalType_E eType);

    /**
     * @brief       : 外部送音频数据
     * @return       {int} 0：成功 非0：失败
     */
    int sendAudioData(int nChannel, Audio_NS::AudioFrame_S *pAudioFrame);

    /**
     * @brief   : 设置视频配置
     * @note    : 配置合法时同步更新各码流连接上限，不重启 RTSP 服务器；
     *            码率升档只收紧新连接，不主动断开已有连接
     * @return   {int} 0：成功 非0：失败
     */
    int setVideoConfig(const std::vector<Video_NS::VideoConfig_S> &vstVideoConfig);

    /**
     * @brief   : 获取视频配置
     */
    const std::vector<Video_NS::VideoConfig_S> &getVideoConfig() const
    {
        return m_vstVideoConfig;
    }

    /**
     * @brief   : 设置音频配置
     * @return   {int} 0：成功 非0：失败
     */
    int setAudioConfig(const Audio_NS::AudioConfig_S &stAudioConfig);

    /**
     * @brief   : 更新网络配置（只更新对外展示地址，不重建监听 socket）
     * @return   {int} 0：成功 非0：失败
     */
    int updateNetworkConfig(const Network::Info_S &stInfo);

    /**
     * @brief   : 设置 RTSP 端口（需要调用方随后 reboot 生效）
     * @return   {int} 0：成功 非0：失败
     */
    int setPort(const int &nPort);

    /**
     * @brief   : 设置 RTSP qos Dscp 值（需要调用方随后 reboot 生效）
     * @return   {int} 0：成功 非0：失败
     */
    int setQosDscp(const int &nDscp);

    /**
     * @brief   : 获取rtsp码流地址
     * @note    : 未开启鉴权时返回普通地址；内部缓存字符串，指针在下一次更新前有效
     * @param    {int} nChn 码流ID，从0开始
     * @param    {bool} bAuth 是否获取带账号密码的地址
     * @return   {char *} 码流地址；未初始化返回 nullptr
     */
    char *getRtspUrl(int nChn, bool bAuth = false);

    /**
     * @brief       : 设置请求I帧的回调函数
     * @param        {RequestIdrCallback} callback：回调
     * @param        {void*} pUserData：用户数据
     * @return       {int} 0：成功 非0：失败
     */
    int setRequestIdrCallback(const RequestIdrCallback &callback, void *pUserData = nullptr);

    /**
     * @brief       : 触发请求I帧回调（库内已做全局限频）
     * @return       {int} 0：成功 非0：失败
     */
    int triggerRequestIdr(int nChannel);

    /**
     * @brief   : 重置上次请求IDR帧时间（系统时间回拨后调用）
     * @return   {int} 0：成功 非0：失败
     */
    int reset_lastIdrRequestTime();

    /**
     * @brief   : 更新用户信息（账号、密码）
     * @param    {bool} bReboot 是否立即生效（true 时重建服务）
     * @return   {int} 0：成功 非0：失败
     */
    int update_userInfo(std::string strUser, std::string strPwd, bool bReboot);

    /**
     * @brief   : 更新rtsp摘要算法
     * @return   {int} 0：成功 非0：失败
     */
    int updateRtspDigestAlgorithm();

    /**
     * @brief   : 读取运行指标快照（用于诊断，不参与媒体路径）
     */
    void dumpRuntimeStats();

private:
    /** 停止/恢复媒体入队（库侧动作，实现在 .cpp）。 */
    int stopIngest();

    /**
     * @brief       : 由当前视频/音频配置构建库码流配置（init 专用，实现在 .cpp）
     * @note        : 同时刷新每通道单帧/队列预算，并按码率抬高服务级视频队列字节上限
     * @return       {IpcRet_E} OK：成功，ERR_PARAM：编码格式未知
     */
    IpcRet_E buildStreamConfigs(std::vector<ipc_rtsp::StreamConfig> &vStreams, ipc_rtsp::ServerConfig &stServerConfig);

    /* 是否初始化 */
    std::atomic<bool> m_bInitFlag{ false };
    /* RTSP端口 */
    int m_nRtspPort = 554;
    /* RTSP认证开关 */
    bool m_bAuthentication = true;
    /* 用户名/密码 */
    std::string m_strUser = USER_DEFAULT_NAME;
    std::string m_strPwd = USER_DEFAULT_PASSWD;
    /* RTSP摘要算法 */
    int m_nRtspDigestAlgorithm = 0;
    /* 媒体 DSCP */
    int m_nMediaDscp = 0;
    /* 视频配置 */
    std::vector<Video_NS::VideoConfig_S> m_vstVideoConfig;
    /* 音频配置 */
    Audio_NS::AudioConfig_S m_stAudioConfig;
    /* 网络信息 */
    Network::Info_S m_stNetInfo;
    /* 对外展示 IP */
    std::string m_strAdvertisedIp;
    /* 每通道队列字节预算（按码率动态计算） */
    std::size_t m_unVideoQueueMaxBytes[RTSP_CHN_MAX] = { RTSP_VIDEO_QUEUE_MAX_BYTES, RTSP_VIDEO_QUEUE_MAX_BYTES };
    /* 每通道单帧上限 */
    std::size_t m_unVideoMaxFrameBytes[RTSP_CHN_MAX] = { RTSP_VIDEO_QUEUE_MAX_BYTES, RTSP_VIDEO_QUEUE_MAX_BYTES };
    /* 请求I帧回调 */
    RequestIdrCallback m_requestIdrCallback;
    void *m_pCallbackUserData = nullptr;
    /* 控制操作互斥锁 */
    std::mutex m_mutexCtrl;
    /* URL 缓存（保持 char* 返回语义） */
    std::map<int, std::string> m_rtspUrlMap;
    /* 带凭据 URL 缓存 */
    std::map<int, std::string> m_rtspUrlAuthMap;
    /* 配置路径 */
    std::string m_strVideoConfigPath;
    std::string m_strAudioConfigPath;
    std::string m_strPortPath;
    /**
     * 实现体：把产品结构体与库之间的映射、配置读取、日志转发全部收纳在 .cpp；
     * 头文件只以前置声明引用库配置类型，不 include ipc_rtspserver 公共头。
     */
    struct Impl;
    std::unique_ptr<Impl> m_pImpl;
};
