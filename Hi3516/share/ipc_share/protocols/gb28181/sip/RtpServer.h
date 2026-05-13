/*** 
 * @FilePath     : RtpServer.h
 * @Author       : cyc
 * @Date         : 2025-05-15 11:09:53
 * @LastEditors  : cyc
 * @LastEditTime : 2025-05-16 09:27:53
 * @Description  : rtp服务器,获取音视频数据推至rtp
 */

#pragma once

#include "video_define.h"
#include "audio_define.h"
#include "list_use_lock.h"
#include "SipChannel.h"
#include "SipDevice.h"
#include "IpcRet.h"
#include "SipType.h"
#include "os_que.h"
#include <mutex>

/*rtp状态信息*/
typedef enum  _Rtp_Server_Status_E_ 
{
    RTP_IDLE,     /*空闲状态*/
    RTP_RUNNING,  /*运行状态*/
    RTP_ERROR     /*失败状态*/
}Rtp_Server_Status_E;

/* RTP数据信息结构体 */
typedef struct _Rtp_Data_info_S_
{ 
    bool bIframe;    /* 是否为I帧 */
    int nframeSize;  /* 帧大小 */    
    char *achBuff;   /* 数据缓冲区 */
} Rtp_Data_info_S;

/* RTP流信息结构体 */
typedef struct _Rtp_Server_Info_S_
{
    Rtp_Server_Status_E enRtpStatus;        /* rtp状态 */
    bool bCurIframe;                        /* 当前帧是否为I帧*/
    int nVideosocket;                        /* 视频推流socket */
    int nAudiosocket;                        /* 音频推流socket */
    List_LockHandle_t *listVideoHandle;     /* 视频流队列句柄 */
    List_LockHandle_t *listAudioHandle;     /* 音频流队列句柄 */
	OS_QueHndl listQueVideoHandle;         
	OS_QueHndl listQueAudioHandle;
} Rtp_Server_Info_S;

namespace SIP
{
    class CRtpServer
    {
        public:
            /**
             * @brief  模块管理单例
             * @return [*]
             * @author cyc
             * @note
             */
            static CRtpServer *instance()
            {
                std::lock_guard<std::mutex> lock(m_mtx);
                if (m_pInstance == nullptr)
                {
                    m_pInstance = new CRtpServer();
                }
                return m_pInstance;
            }

            /*** 
             * @description : 
             * @author      : cyc
             * @return       {*}
             */
            IpcRet_E rtpServer_init();

            /**
             * @brief       : RTP/UDP推流反初始化
             * @author      : zhouzirui
             * @return       {*}0：成功，负值：失败
             */
            IpcRet_E rtpServer_deinit();


            /*** 
             * @description : 设置rtp状态
             * @author      : cyc
             * @param        {network_Status_t} status
             * @param        {string} strChnID
             * @param        {shared_ptr<SIP::Device>} pDevice
             * @return       {*}
             */        
            int set_rtpStatus(Rtp_Server_Status_E enStatus,std::string strChnID,std::shared_ptr<SIP::Device> pDevice);

            /**
             * @brief       : 获取RTP推流状态
             * @author      : cyc
             * @return       {*}0：成功，负值：失败 
             */
            inline int get_rtpStatus();

            /**
             * @brief       : 外部送视频数据至GB28181
             * @author      : zhouzirui
             * @param        {VideoFrame_S} *pVideoFrame：视频帧数据指针
             * @return       {*}0：成功，非0：失败
             */
            IpcRet_E sendVideoData(Video_NS::VideoFrame_S *pVideoFrame);

            /**
             * @brief       : 外部送音频数据至GB28181
             * @author      : zhouzirui
             * @param        {AudioFrame_S} *pAudioFrame：音频帧数据指针
             * @return       {*}0：成功，非0：失败
             */
            IpcRet_E sendAudioData(Audio_NS::AudioFrame_S *pAudioFrame);

        private:
            /*** 
             * @description : rtp发送线程
             * @author      : cyc
             * @return       {*}
             */        
            void rtpSend_thread();

        private:
            static std::mutex m_mtx;
            static CRtpServer *m_pInstance;
            Rtp_Server_Info_S stRtpStream;
            SIP::Channel::Ptr pChannel = nullptr;
            std::thread m_rtpThread; /*线程句柄*/
            bool bRtpTheadFlag = false;      /*rtp线程启动标志*/
    };
}


