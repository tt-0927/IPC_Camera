/**
 * @FilePath     : push_stream.cpp
 * @Author       : zhouzirui
 * @Date         : 2025-03-27 17:42:05
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2025-07-08 17:04:56
 * @Description  : 推流模块
 */

#include "push_stream.h"

#include "path_define.h"

CPushStream* CPushStream::m_self = NULL;
std::mutex CPushStream::m_mutex;

CPushStream::CPushStream()
: m_strHttpsConfigFile(HTTPS_CONFIG_FILE)
{

}

CPushStream::~CPushStream()
{

}

IpcRet_E CPushStream::init()
{
    Network::HttpsConfigInfo_S stInfo;
    if (Convert::read_file(m_strHttpsConfigFile, stInfo))
	{
		Convert::write_file(m_strHttpsConfigFile, stInfo);
	}
    if (stInfo.bEnRtsp && !CRtspServer::instance()->isInit())
    {
        CRtspServer::instance()->init();
    }
    m_bInitFlag = true;

    return OK;
}

IpcRet_E CPushStream::deinit()
{
    if (CRtspServer::instance()->isInit())
    {
        CRtspServer::instance()->deinit();
    }
    m_bInitFlag = false;

    return OK;
}

int CPushStream::sendVideoData(Video_NS::VideoFrame_S *pVideoFrame, bool bIsMain, bool bIsRtsp)
{
    if (!pVideoFrame)
    {
        dlog_error("指针为空");
        return ERR_PTR_NULL;
    }

    int nRet = OK;
    if (m_bInitFlag == true)
    {
        if (bIsRtsp == true)
        {
            if(bIsMain == true)
            {
                nRet = CRtspServer::instance()->sendVideoData(RTSP_CHN_MAIN, pVideoFrame);
            }else{
                nRet = CRtspServer::instance()->sendVideoData(RTSP_CHN_SUB, pVideoFrame);
            }
        }
    }

    return nRet;
}

int CPushStream::sendAudioData(Audio_NS::AudioFrame_S *pAudioFrame, bool bIsMain, bool bIsRtsp)
{
    if (!pAudioFrame)
    {
        dlog_error("指针为空");
        return ERR_PTR_NULL;
    }

    int nRet = OK;
    if (m_bInitFlag == true)
    {
        if (bIsRtsp == true)
        {
            if(bIsMain == true)
            {
                nRet = CRtspServer::instance()->sendAudioData(RTSP_CHN_MAIN, pAudioFrame);
            }else{
                nRet = CRtspServer::instance()->sendAudioData(RTSP_CHN_SUB, pAudioFrame);
            }
        }
    }

    return nRet;
}

int CPushStream::sendAudioData(Audio_NS::AudioFrame_S *pAudioFrame, bool bIsRtsp)
{
    if (!pAudioFrame)
    {
        dlog_error("指针为空");
        return ERR_PTR_NULL;
    }

    int nRet = OK;
    if (m_bInitFlag == true)
    {
        if (bIsRtsp == true)
        {
            nRet = CRtspServer::instance()->sendAudioData(RTSP_CHN_MAIN, pAudioFrame);
            nRet = CRtspServer::instance()->sendAudioData(RTSP_CHN_SUB, pAudioFrame);
        }
    }

    return nRet;
}
