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

#include "stream_vi.h"
#include "stream_vpss.h"
#include "stream_venc.h"
#include "video_define.h"
#include "osd_manage.h"
#include "av_configure.h"
#include "stream_video_config.h"

#define MAX_VENC_PACK_COUNT 10  // 最大的编码包数量

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
    /*线程异常处理互斥锁*/
    std::mutex exception_mutex_;
    /*线程异常处理的线程名称*/
    std::string last_error_;
};
