/**
 * @FilePath     : stream_video.h
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2025-12-04 09:33:03
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2025-12-04 16:13:21
 * @Description  : 流媒体视频模块头文件
 */

#pragma once

#include <iostream>
#include <mutex>
#include <thread>
#include <atomic>
#include <memory>
#include <chrono>

#include "stream_vi.h"
#include "stream_vpss.h"
#include "stream_venc.h"
#include "video_define.h"
#include "osd_manage.h"
#include "av_configure.h"
#include "stream_video_config.h"

#define MAX_VENC_PACK_COUNT 10  // 最大的编码包数量

/* 特写源帧: 特写源高分辨率帧拷贝 */
typedef struct _FaceCloseupFrame_S
{
    std::shared_ptr<char[]> pData;   /* NV12 帧数据拷贝 */
    int nWidth   = 0;                /* 实际宽 */
    int nHeight  = 0;                /* 实际高 */
    int nVirWidth  = 0;              /* stride 对齐宽 */
    int nVirHeight = 0;              /* stride 对齐高 */
    uint32_t u32TimeRef = 0;         /* 帧时间参考 */
    uint64_t u64PTS = 0;             /* 帧时间戳(us) */
} FaceCloseupFrame_S;

/*流媒体码流数枚举*/
typedef enum StreamMediaNum
{
    STREAM_MEDIA_MAIN = 0,
    STREAM_MEDIA_SUB,
    STREAM_MEDIA_SUM,
} StreamMediaNum_E;

class CStreamVideo
{
private:
    CStreamVideo();
    static CStreamVideo *m_self;
    static std::mutex m_mutex;

public:
    static CStreamVideo *instance()
    {
        if (m_self == nullptr) // 第一层检查
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            if (m_self == nullptr) // 第二层检查
            {
                m_self = new CStreamVideo();
            }
        }
        return m_self;
    }

    ~CStreamVideo();

    /**
     * @brief   : 初始化视频模块
     * @return   {int} 0：成功，非0：失败
     */
    int init();

    /**
     * @brief   : 去初始化视频模块
     * @return   {int} 0：成功，非0：失败
     */
    int deinit();

    /**
     * @brief   : 初始化采集和视频处理的模块
     * @return   {int} 0：成功，非0：失败
     */
    int initStream();

    /**
     * @brief   : 反初始化采集和视频处理的模块
     * @return   {int} 0：成功，非0：失败
     */
    int deinitStream();

    /**
     * @brief   : 重新启动视频流模块
     * @return   {IpcRet_E} 0：成功，非0：失败
     */
    IpcRet_E reboot();

    /**
     * @brief   : 重新启动视频流模块
     * @param    {int} nChn 重新启动的视频流通道
     * @param    {VideoConfig_S} &stVideoConfig 启动的视频流通道的视频配置
     * @param    {bool} bUseIncomingAttr 是否使用传入的视频配置属性
     * @return   {IpcRet_E} 0：成功，非0：失败
     */
    IpcRet_E reboot_venc(int nChn, const Video_NS::VideoConfig_S &stVideoConfig, bool bUseIncomingAttr = false);

    /**
     * @brief   : 请求IDR帧
     * @param    {int} nChannel 需要请求IDR帧的通道号
     */
    void request_idr(int nChannel);

    /**
     * @brief   : 获取当前视频配置
     * @param    {vector<Video_NS::VideoConfig_S>} &vstVideoConfig：视频配置
     * @return   {int} 0：成功，非0：失败
     */
    int getVideoConfig(std::vector<Video_NS::VideoConfig_S> &vstVideoConfig);

    /**
     * @brief   : 设置视频配置
     * @param    {VideoConfig_S} &stVideoConfig：视频配置
     * @return   {int} 0：成功，非0：失败
     */
    int setVideoConfig(Video_NS::VideoConfig_S &stVideoConfig);

    /**
     * @brief   : 设置视频ROI配置
     * @param    {Video_NS::VideoRoiConfig_S} &stVideoRoiConfig：视频ROI配置
     * @return   {int} 0：成功，非0：失败
     */
    int setVideoRoiConfig(const Video_NS::VideoRoiConfig_S  &stVideoRoiConfig);

    /**
     * @brief   : 设置区域裁剪配置
     * @param    {Video_NS::AreaCrop_S} &stAreaCrop：区域裁剪配置
     * @param    {bool} bIsMandateSet：是否强制设置配置
     * @return   {int} 0：成功，非0：失败
     */
    int setAreaCropConfig(const Video_NS::AreaCrop_S &stAreaCrop, bool bIsMandateSet = false);

    /**
     * @brief   : 获取jpeg编码通道参数
     * @param    {unsigned int} &unWidth jpeg宽
     * @param    {unsigned int} &unHeight jpeg高
     * @param    {unsigned int} &nUqFactor jpeg质量
     * @return   {int} 0：成功，非0：失败
     */
    int getJpegVencParam(unsigned int &unWidth, unsigned int &unHeight, unsigned int &nUqFactor);

    /**
     * @brief   : 取特写源高分辨率帧 (特写图裁剪源)
     * @details : 特写源绑定VENC编码的同时供本接口取帧; 帧取到后立即拷贝到
     *            应用内存并释放VPSS缓冲, 不影响特写源推流实时性。
     *            特写源分辨率 <=1080p 或帧率低于AI通道(5fps)时返回
     *            ERR_NOT_ENABLED, 调用方应回退第三路(固定1080p)帧裁剪,
     *            保证全景图和特写图画面同步。
     *            取到的帧PTS与检测帧PTS相差超过2个特写源帧间隔(封顶100ms)时返回ERR, 调用方回退检测帧同帧裁剪,
     *            保证全景/特写画面同步。
     * @param    {FaceCloseupFrame_S} &stFrame 输出帧 (NV12 拷贝)
     * @param    {uint64_t} u64Pts 检测帧PTS(us), 0表示跳过PTS校验
     * @return   {int} OK 成功; ERR_NOT_ENABLED 特写源<=1080p或帧率过低(回退第三路);
     *                  ERR_UNINIT 未初始化; ERR 取帧失败/PTS不匹配/限流
     */
    int grabFaceCloseupSource(FaceCloseupFrame_S &stFrame, uint64_t u64Pts = 0);

private:
    /**
     * @brief   : 绑定模块信息
     * @return   {int} 0：成功，非0：失败
     */
    int bindModule();

    /**
     * @brief   : 解绑模块
     * @return   {int} 0：成功，非0：失败
     */
    int unbindModule();

    /**
     * @brief   : vpss绑定视频编码模块
     * @param    {int} nVencChn 视频编码通道号
     * @return   {int} 0：成功，非0：失败
     */
    int vpssBindVencModule(int nVencChn);

    /**
     * @brief   : vpss解绑视频编码模块
     * @param    {int} nVencChn 视频编码通道号
     * @return   {int} 0：成功，非0：失败
     */
    int vpssUnbindVencModule(int nVencChn);

    /**
     * @brief   : 初始化回调绑定
     */
    void initCallbackBinding();

private:
    /**
     * @brief   : 创建帧包
     * @param    {VideoCodec_E} enCodec 视频编码类型
     * @param    {uint8_t} *pData 编码数据
     * @param    {int} nDataLen 编码长度
     * @return   {VideoFrame_S *} NULL：失败 非NULL：成功
     */
    Video_NS::VideoFrame_S *createFrame(Video_NS::VideoCodec_E enCodec, uint8_t *pData, int nDataLen);

    /**
     * @brief   : 销毁帧包（不释放内部 pData）
     * @param    {VideoFrame_S} *pVideoFrame 待释放的视频帧数据指针
     */
    void freeFrame(Video_NS::VideoFrame_S *pVideoFrame);

    //info /*----------------------- 私有线程函数 -----------------------*/

    /**
     * @brief   : 获取编码后的数据送推流
     * @param    {int} param 编码通道ID
     * @return   {int} 0：成功，非0：失败
     */
    void get_vencStream(int param);

    /**
     * @brief   :获取VPSS通道的数据送AI处理等
     * @return   {int} 0：成功，非0：失败
     */
    void get_vpssStream();

    // 异常处理
    void HandleThreadException(const std::string &thread_name)
    {
        std::lock_guard<std::mutex> lock(exception_mutex_);
        last_error_ = thread_name + " crashed";
        // RestartThread(thread_name); // 线程重启逻辑
    }

private:
    //info /*----------------------- 模块句柄 -----------------------*/
    /* vi句柄 */
    RkVi_S *m_pViHandle;
    /* vpss句柄 */
    RkVpss_S **m_pVpssHandle;
    /* venc句柄 */
    RkVenc_S *m_pVencHandle[VENC_CHN_MAX];
    /* vpss帧数据结构 */
    StreamVpssFrame_t stVpssFrame;
    /*线程句柄 获取 VENC数据*/
    std::thread m_getVencThread[VENC_CHN_MAX];
    /*线程句柄 获取 VPSS数据*/
    std::thread m_getVpssThread[VPSS_CHANNEL_SUM];
    /*送编码数据互斥锁*/
    std::mutex m_mutexSendData;
    /* 视频流配置管理类 */
    CStreamVideoConfig m_configManager;

    //info /*----------------------- 参数变量 -----------------------*/
    /*视频配置文件路径*/
	std::string m_strConfigPath;
    /*视频感兴趣区域配置文件路径*/
	std::string m_strRoiConfigPath;
    /* 是否初始化 */
    std::atomic_bool m_bInitFlag;
    /*OS线程互斥锁*/
    // OS_MutexHndl m_videoMutex;
    /*获取 VENC数据-标志*/
    std::atomic<bool> m_bVencFlag[VENC_CHN_MAX];
    /*获取 VPSS数据-标志*/
    std::atomic<bool> m_bVpssFlag[VPSS_CHANNEL_SUM];
    /*控制操作互斥锁*/
    std::mutex m_mutexCtrl;
    /*特写取帧互斥锁(同一时刻只允许一个取帧请求)*/
    std::mutex m_mutexFaceGrab;
    /*特写上次取帧时刻(事件风暴限流)*/
    std::chrono::steady_clock::time_point m_tLastFaceGrab;
    /*线程异常处理互斥锁*/
    std::mutex exception_mutex_;
    /*线程异常处理的线程名称*/
    std::string last_error_;
};
