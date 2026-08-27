/*** 
 * @FilePath     : RtpServer.cpp
 * @Author       : cyc
 * @Date         : 2025-05-15 11:09:53
 * @LastEditors  : cyc
 * @LastEditTime : 2026-08-14 10:56:36
 * @Description  : rtp服务器,获取音视频数据推至rtp
 */

#include <iostream>
#include <thread>
#include "dlog.h"
#include "RtpServer.h"
#include "memory_leak.h"
#include "share_os.h"

using namespace SIP;

CRtpServer *CRtpServer::m_pInstance = nullptr;
std::mutex CRtpServer::m_mtx;
/*音频队列存储最大数*/
#define AUDIOFRAMEC_MAX 100
/*视频队列存储最大数*/
#define VIDEOFRAMEC_MAX 60

inline int CRtpServer::get_rtpStatus()
{
    return stRtpStream.enRtpStatus;
}

int CRtpServer::set_rtpStatus(Rtp_Server_Status_E enStatus,std::string strChnID,std::shared_ptr<SIP::Device> pDevice)
{
    if(enStatus == RTP_RUNNING)
    {      
        dlog_info("INVITE:[%s]", strChnID.data());
        if (pDevice)
        { 
            pChannel = pDevice->GetChannel(strChnID);
        }
    }
    else 
    {
        pChannel = nullptr;
    }

    stRtpStream.enRtpStatus = enStatus;

    return OK;
}

static int free_rtpStream_message(Rtp_Data_info_S *pStream_messgage)
{
    if (pStream_messgage == NULL)
    {
        dlog(LOG_ERROR, "free_rtpStream_message is fail");
        return -1;
    }

    if (pStream_messgage->achBuff)
    {
        manag_free(pStream_messgage->achBuff);
        pStream_messgage->achBuff = NULL;
    }
    manag_free(pStream_messgage);
    pStream_messgage = NULL;
    return 0;
}

void CRtpServer::rtpSend_thread()
{
    pthread_setname_np(pthread_self(), "SIPRtpServer");

    Rtp_Data_info_S *pDataInfo = NULL;
    // Rtp_Server_Info_S stStreamInfo;
    while (bRtpTheadFlag)
    {
        if (false == get_rtpStatus())
        {
            usleep(20 * 1000);
            continue;
        }

        if (0 < list_lockAndGet_size(stRtpStream.listAudioHandle))
        {
            pDataInfo = (Rtp_Data_info_S *)list_lockAndPop_front(stRtpStream.listAudioHandle);
            if (pDataInfo && pChannel)
            {
                pChannel->SendMedia(pDataInfo->achBuff,pDataInfo->nframeSize,true);     
            }
            free_rtpStream_message(pDataInfo);
            pDataInfo = NULL;
        }

        if (0 < list_lockAndGet_size(stRtpStream.listVideoHandle))
        {
            pDataInfo = (Rtp_Data_info_S *)list_lockAndPop_front(stRtpStream.listVideoHandle);
            if (pDataInfo && pChannel)
            {
                pChannel->SendMedia(pDataInfo->achBuff,pDataInfo->nframeSize,false);
            }    
            free_rtpStream_message(pDataInfo);
            pDataInfo = NULL;
        }else{
            usleep(20 * 1000);
            continue;
        }
    }
    dlog_info("退出rtp线程");
}

IpcRet_E CRtpServer::rtpServer_init()
{
    dlog(LOG_DEBUG, "RTP流初始化");
    IpcRet_E nRet = OK;

    /* 初始化音频队列 */
    stRtpStream.listAudioHandle = list_lockAndCreate();
    if (!stRtpStream.listAudioHandle)
    {
        dlog(LOG_ERROR, "list_lockAndCreate error\n");
        nRet = ERR;
        if (stRtpStream.listAudioHandle)
        {
            list_lockAndDestory(stRtpStream.listAudioHandle);
        }
        return ERR;
    }
    /* 初始化视频队列 */
    stRtpStream.listVideoHandle = list_lockAndCreate();
    if (!stRtpStream.listVideoHandle)
    {
        dlog(LOG_ERROR, "list_lockAndCreate error\n");
        nRet = ERR;
        if (stRtpStream.listVideoHandle)
        {
            list_lockAndDestory(stRtpStream.listVideoHandle);
        }
        if (stRtpStream.listAudioHandle)
        {
            list_lockAndDestory(stRtpStream.listAudioHandle);
        }
        return ERR;
    }

    dlog_info("创建RTP流发送线程");
    bRtpTheadFlag = true;
    m_rtpThread = std::thread(&CRtpServer::rtpSend_thread, this);

    if (nRet != 0)
    {
        dlog_error("创建RTP流发送线程失败");

        if (stRtpStream.listVideoHandle)
        {
            list_lockAndDestory(stRtpStream.listVideoHandle);
        }
        if (stRtpStream.listAudioHandle)
        {
            list_lockAndDestory(stRtpStream.listAudioHandle);
        }

        return ERR;
    }
    return nRet;
}

IpcRet_E CRtpServer::rtpServer_deinit()
{
    if (stRtpStream.listVideoHandle)
    {
        list_lockAndDestory(stRtpStream.listVideoHandle);
    }
    if (stRtpStream.listAudioHandle)
    {
        list_lockAndDestory(stRtpStream.listAudioHandle);
    }
    bRtpTheadFlag = false;

    m_rtpThread.join();
    

    return OK;
}

IpcRet_E CRtpServer::sendVideoData(Video_NS::VideoFrame_S *pVideoFrame)
{
    if (!pVideoFrame)
    {
        dlog_error("sendVideoData 指针为空");
        return ERR_PTR_NULL;
    }

    return sendVideoData(pVideoFrame->pData, pVideoFrame->nLen);
}

IpcRet_E CRtpServer::sendVideoData(const uint8_t *pData, int nDataLen)
{
    if (!pData || nDataLen <= 0)
    {
        dlog_error("sendVideoData 数据为空");
        return ERR_PARAM;
    }

    if (false == get_rtpStatus())
    {
        return OK;
    }

    Rtp_Data_info_S *pPacketInfo = (Rtp_Data_info_S *)manag_malloc(sizeof(Rtp_Data_info_S));
    if (pPacketInfo == NULL)
    {
        dlog(LOG_ERROR, "live putStreaminfo is error");
        return ERR_PTR_NULL;
    }


    /* memory: 仅在RTP队列中保留一份独立副本，不保存VENC原始指针。 */
    pPacketInfo->achBuff = (char *)manag_malloc(nDataLen);
    if (pPacketInfo->achBuff == NULL)
    {
        manag_free(pPacketInfo);
        pPacketInfo = NULL;
        return ERR_PTR_NULL;
    }

    r_memcpy(pPacketInfo->achBuff, pData, nDataLen);
    pPacketInfo->nframeSize = nDataLen;
    
    int nSize = 0;
    /* 抛帧策略，视频队列满则先丢掉非I帧 */ 
    if ((nSize = list_lockAndGet_size(stRtpStream.listVideoHandle)) > VIDEOFRAMEC_MAX)
    {
        stRtpStream.bCurIframe = true;
    }

    if (stRtpStream.bCurIframe == true)
    {
        if (nSize < 1)
        {
            stRtpStream.bCurIframe = false;
        }
    }

    if (stRtpStream.bCurIframe == false)
    {
        list_lockAndPush_back(stRtpStream.listVideoHandle, pPacketInfo);
    }
    else
    {
        free_rtpStream_message(pPacketInfo);
    }
    return OK;
}

IpcRet_E CRtpServer::sendAudioData(Audio_NS::AudioFrame_S *pAudioFrame)
{
    if (pAudioFrame == NULL)
    {
        dlog_error("sendVideoData param is error");
        return ERR_PTR_NULL;
    }

    if (false == get_rtpStatus())
    {
        return OK;
    }

    Rtp_Data_info_S *pPacketInfo = (Rtp_Data_info_S *)manag_malloc(sizeof(Rtp_Data_info_S));
    if (pPacketInfo == NULL)
    {
        dlog(LOG_ERROR, "live putStreaminfo is error\n");
        return ERR_PTR_NULL;
    }

    if (pAudioFrame->enFormat == Audio_NS::AudioFormat_E::AAC)
    {
        pPacketInfo->bIframe = FALSE;
        // pPacketInfo->stCodeType = RTP_AUDIO_AAC;
    }
    else if (pAudioFrame->enFormat == Audio_NS::AudioFormat_E::G711A)
    {
        pPacketInfo->bIframe = FALSE;
        // pPacketInfo->stCodeType = RTP_AUDIO_G711A;
    }

    /* 媒体数据放入队列 */
    pPacketInfo->achBuff = (char *)manag_malloc(pAudioFrame->nLen);
    if (pPacketInfo->achBuff == NULL)
    {
        manag_free(pPacketInfo);
        pPacketInfo = NULL;
        return ERR_PTR_NULL;
    }
    
    r_memcpy(pPacketInfo->achBuff, pAudioFrame->pData, pAudioFrame->nLen);
    pPacketInfo->nframeSize = pAudioFrame->nLen;

    int nSize = 0;
    if ((nSize = list_lockAndGet_size(stRtpStream.listAudioHandle)) > AUDIOFRAMEC_MAX)
    {
        /* 音频队列满则直接丢帧 */
        free_rtpStream_message(pPacketInfo);
    }
    else
    {
        list_lockAndPush_back(stRtpStream.listAudioHandle, pPacketInfo);
    }

    return OK;
}
