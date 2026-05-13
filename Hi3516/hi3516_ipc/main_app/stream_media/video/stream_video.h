/**
 * @FilePath     : stream_video.h
 * @Author       : zhouzirui
 * @Date         : 2025-03-21 10:24:45
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-01-08 10:21:25
 * @Description  : 流媒体视频模块头文件
 */

#pragma once

#include <iostream>
#include <mutex>
#include <thread>
#include <atomic>
#include <vector>
#include <condition_variable>
#include <stdexcept>
#include "dlog.h"
#include "IpcRet.h"
#include "time_utils.h"

#include "stream_sys.h"
#include "stream_vi.h"
#include "stream_vpss.h"
#include "stream_venc.h"
#include "stream_ai_detect.h"
#include "ai_detect.h"
#include "push_stream.h"
#include "osd_manage.h"
#include "stream_video_config.h"
#include "algo_detect.h"
#include "mpp_handle_wrapper.h"
#include "venc_channel_handler.h"
#include "nal_parser.h"

extern "C"
{
#include "mpp_sys.h"
#include "mpp_bind.h"
}

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
    static CStreamVideo* m_self;
    static std::mutex m_mutex;
public:
    static CStreamVideo* instance()
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
     * @brief       : 初始化采集和视频处理的模块
     * @author      : zhouzirui
     * @return       {*} RETURN_ERROR：失败
     */
    IpcRet_E init();

    /**
     * @brief       : 反初始化采集和视频处理的模块
     * @author      : zhouzirui
     * @return       {*} RETURN_ERROR：失败
     */
    IpcRet_E deinit();

    /**
     * @brief   : 重新启动视频流模块
     * @return   {IpcRet_E}0：成功，非0：失败
     */
    IpcRet_E reboot();

    /**
     * @brief   : 重新启动视频流模块
     * @param    {int} nChn：重新启动的视频流通道
     * @return   {IpcRet_E} 0：成功，非0：失败
     */
    IpcRet_E reboot_venc(int nChn, const Video_NS::VideoConfig_S &stVideoConfig, bool bUseIncomingAttr = false);

    /**
     * @brief   : 获取当前视频配置
     * @param    {vector<Video_NS::VideoConfig_S>} &vstVideoConfig：视频配置
     * @return   {*}0：成功，非0：失败
     */
    int getVideoConfig(std::vector<Video_NS::VideoConfig_S> &vstVideoConfig);

    /**
     * @brief   : 设置视频配置
     * @param    {VideoConfig_S} &stVideoConfig：视频配置
     * @return   {int}0：成功，非0：失败
     */
    int setVideoConfig(const Video_NS::VideoConfig_S &stVideoConfig);

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
     * @brief       : 请求IDR帧
     * @author      : zhouzirui
     * @param        {int}需要请求IDR帧的通道号
     */
    void request_idr(int nChannel);

    /**
     * @brief   : 获取VI句柄
     * @return   {HiVi_S *} 非空：成功 空：失败
     */
    HiVi_S *get_viHandle();

    /**
     * @brief   : 获取VPSS句柄
     * @note    : 句柄以组为单位
     * @param    {int} nGrp VPSS的组
     * @return   {HiVpss_S *} 非空：成功 空：失败
     */
    HiVpss_S *get_vpssHandle(int nGrp = VPSS_MAIN_SUB);

private:
    /**
     * @brief       : 绑定模块
     * @author      : zhouzirui
     * @return       {*}非0：失败
     */
    int bindModule();

    /**
     * @brief       : 解绑模块
     * @author      : zhouzirui
     * @return       {*}非0：失败
     */
    int unbindModule();

    /**
     * @brief   : vpss绑定视频编码模块
     * @return   {*}0：成功，非0：失败
     */
    int vpssBindVencModule(int nVencChn);

    /**
     * @brief   : vpss解绑视频编码模块
     * @return   {*}0：成功，非0：失败
     */
    int vpssUnbindVencModule(int nVencChn);

    /**
     * @brief   : 初始化回调绑定
     */
    void initCallbackBinding();

private:
    /**
     * @brief       : 创建帧包
     * @author      : zhouzirui
     * @param        {VENC_CHN_E} enChn：编码通道号 0：第一码流 1：第二码流
     * @param        {uint8_t} *pData：视频数据
     * @param        {int} nDataLen：视频数据长度
     * @return       {*}NULL：失败 非NULL：成功
     */
    Video_NS::VideoFrame_S *createFrame(VENC_CHN_E enChn, uint8_t *pData, int nDataLen);

    /**
     * @brief       : 销毁帧包（不释放内部 pData）
     * @author      : zhouzirui
     * @param        {VideoFrame_S} *pVideoFrame：待释放的视频帧数据指针
     * @return       {*}
     */
    void freeFrame(Video_NS::VideoFrame_S *pVideoFrame);

    //info /*----------------------- 私有线程函数 -----------------------*/
    /**
     * @brief       : 获取编码后的数据送推流
     * @author      : zhouzirui
     * @return       {*}
     */
    void get_vencStream(int param);

    /**
     * @brief       : 获取VPSS通道的数据送AI处理等
     * @author      : zhouzirui
     * @return       {*}
     */
    void get_vpssStream();

    /**
     * @brief       : 获取jpeg编码通道参数
     * @author      : lianghy
     * @param        {unsigned int &} &unWidth：jpeg宽
     * @param        {unsigned int &} &unHeight：jpeg高
     * @param        {unsigned int &} &nUqFactor：jpeg质量
     * @return       {int} 成功：0
     */
    int getJpegVencParam(unsigned int &unWidth, unsigned int &unHeight, unsigned int &nUqFactor);


    // 异常处理
    void HandleThreadException(const std::string& thread_name) {
        std::lock_guard<std::mutex> lock(exception_mutex_);
        last_error_ = thread_name + " crashed";
        // RestartThread(thread_name); // 线程重启逻辑
    }

private:
    // info /*----------------------- 模块句柄 -----------------------*/

    /* 视频流配置管理类 */
    CStreamVideoConfig m_configManager;
    /*线程句柄 获取 VENC数据*/
    std::thread m_getVencThread[VENC_CHN_MAX];
    /*线程句柄 获取 VPSS数据*/
    std::thread m_getVpssThread[VPSS_CHANNEL_SUM];
    /* vi句柄（RAII管理） */
    ViHandle m_viHandle;
    /* vpss句柄（RAII管理） */
    HiVpss_S **m_pVpssHandle;
    /* venc句柄（RAII管理） */
    std::array<VencHandle, VENC_CHN_MAX> m_vencHandles;
    /* ai detect句柄 */
    HiAiDetect_S *m_pAiDetect[32];
    /* AI检测结果分析句柄*/
    CAiDetect m_cAiDetect;
    /*送编码数据互斥锁*/
    std::mutex m_mutexSendData;

    /* VENC通道处理器策略 */
    std::array<std::unique_ptr<CIVencChannelHandler>, VENC_CHN_MAX> m_channelHandlers;
    /* NAL解析器策略 */
    std::unordered_map<Video_NS::VideoCodec_E, std::unique_ptr<CINalParser>> m_nalParsers;
    // info /*----------------------- 参数变量 -----------------------*/
    /* 是否初始化 */
    bool m_bInitFlag = false;
    /*获取 VENC数据-标志*/
    std::atomic<bool> m_bVencFlag[VENC_CHN_MAX];
    /*获取 VPSS数据-标志*/
    std::atomic<bool> m_bVpssFlag[VPSS_CHANNEL_SUM];
    /*线程异常处理互斥锁*/
    std::mutex exception_mutex_;
    /*线程异常处理的线程名称*/
    std::string last_error_;
    /*控制操作互斥锁*/
    std::mutex m_mutexCtrl;
};
