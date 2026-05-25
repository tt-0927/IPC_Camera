/**
 * @FilePath     : rtmp_pusher.cpp
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2026-05-12 13:56:20
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-05-13 09:39:23
 * @Description  : RTMP推流管理器实现
 */

#include "rtmp_pusher.h"

CRtmpPusher::CRtmpPusher()
{
}

CRtmpPusher::~CRtmpPusher()
{
    deinit();
}

IpcRet_E CRtmpPusher::init()
{
    if (m_bInitFlag.load())
    {
        dlog_warn("RTMP推流管理器已初始化");
        return OK;
    }

    m_bStopReconnect.store(false);
    m_reconnectThread = std::thread(&CRtmpPusher::reconnect_loop, this);

    m_bInitFlag.store(true);
    dlog_info("RTMP推流管理器初始化成功");

    return OK;
}

IpcRet_E CRtmpPusher::deinit()
{
    if (!m_bInitFlag.load())
    {
        return OK;
    }

    /* 停止重连线程 */
    m_bStopReconnect.store(true);
    m_cvReconnect.notify_all();
    if (m_reconnectThread.joinable())
    {
        m_reconnectThread.join();
    }

    /* 清理所有会话 */
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        for (auto& pair : m_mapSessions)
        {
            if (pair.second)
            {
                pair.second->deinit();
            }
        }
        m_mapSessions.clear();
    }

    m_bInitFlag.store(false);
    dlog_info("RTMP推流管理器反初始化完成");

    return OK;
}

bool CRtmpPusher::is_init() const
{
    return m_bInitFlag.load();
}

int CRtmpPusher::start_push(int nChannel,
                            const std::string& strUrl,
                            const Video_NS::VideoConfig_S& stVideoConfig,
                            const Audio_NS::AudioConfig_S& stAudioConfig)
{
    std::lock_guard<std::mutex> lock(m_mutex);

    auto it = m_mapSessions.find(nChannel);
    if (it != m_mapSessions.end() && it->second->is_connected())
    {
        dlog_warn("通道%d已在推流中", nChannel);
        return OK;
    }

    /* 如果存在旧会话但已断开，先移除 */
    if (it != m_mapSessions.end())
    {
        it->second->deinit();
        m_mapSessions.erase(it);
    }

    /* 创建新会话，避免依赖C++14的std::make_unique，兼容旧工具链 */
    std::unique_ptr<CRtmpSession> pSession(new CRtmpSession(nChannel,
                                                          strUrl,
                                                          stVideoConfig,
                                                          stAudioConfig));
    int nRet = pSession->init();
    if (nRet < 0)
    {
        dlog_error("启动通道%d推流失败: %d", nChannel, nRet);
        m_mapSessions[nChannel] = std::move(pSession);
        return nRet;
    }

    m_mapSessions[nChannel] = std::move(pSession);
    dlog_info("启动通道%d推流成功, URL=%s", nChannel, strUrl.c_str());

    return OK;
}

int CRtmpPusher::stop_push(int nChannel)
{
    std::lock_guard<std::mutex> lock(m_mutex);

    auto it = m_mapSessions.find(nChannel);
    if (it == m_mapSessions.end())
    {
        dlog_warn("通道%d未在推流", nChannel);
        return OK;
    }

    it->second->deinit();
    m_mapSessions.erase(it);
    dlog_info("停止通道%d推流", nChannel);

    return OK;
}

int CRtmpPusher::send_video_data(int nChannel, Video_NS::VideoFrame_S* pVideoFrame)
{
    if (!pVideoFrame)
    {
        return ERR_PTR_NULL;
    }

    std::lock_guard<std::mutex> lock(m_mutex);

    auto it = m_mapSessions.find(nChannel);
    if (it == m_mapSessions.end() || !it->second->is_connected())
    {
        return ERR;
    }

    return it->second->send_video_frame(pVideoFrame);
}

int CRtmpPusher::send_audio_data(int nChannel, Audio_NS::AudioFrame_S* pAudioFrame)
{
    if (!pAudioFrame)
    {
        return ERR_PTR_NULL;
    }

    std::lock_guard<std::mutex> lock(m_mutex);

    auto it = m_mapSessions.find(nChannel);
    if (it == m_mapSessions.end())
    {
        /* 未创建会话时音频帧可能持续进入，默认关闭逐帧丢弃日志。 */
        /* dlog_debug("RTMP通道%d未创建，丢弃音频帧", nChannel); */
        return ERR;
    }
    if (!it->second->is_connected())
    {
        /* 断线重连期间会持续丢弃音频帧，默认关闭逐帧日志。 */
        /* dlog_debug("RTMP通道%d未连接，丢弃音频帧", nChannel); */
        return ERR;
    }

    return it->second->send_audio_frame(pAudioFrame);
}

int CRtmpPusher::send_audio_data(Audio_NS::AudioFrame_S* pAudioFrame)
{
    if (!pAudioFrame)
    {
        return ERR_PTR_NULL;
    }

    int nRet = OK;
    std::lock_guard<std::mutex> lock(m_mutex);
    for (auto& pair : m_mapSessions)
    {
        if (pair.second && pair.second->is_connected())
        {
            int nSendRet = pair.second->send_audio_frame(pAudioFrame);
            if (nSendRet != OK)
            {
                /* 音频广播失败可能在纯视频降级或断线期间高频出现，默认不刷 DEBUG。 */
                /* dlog_debug("RTMP通道%d音频发送失败，返回值=%d", pair.first, nSendRet); */
                nRet = nSendRet;
            }
        }
        else
        {
            /* 广播跳过未连接通道为高频路径，重连日志已覆盖连接状态变化。 */
            /* dlog_debug("RTMP通道%d不存在或未连接，跳过音频广播", pair.first); */
        }
    }

    return nRet;
}

void CRtmpPusher::reconnect_loop()
{
    while (!m_bStopReconnect.load())
    {
        {
            std::unique_lock<std::mutex> lock(m_mutexReconnect);
            m_cvReconnect.wait_for(lock, std::chrono::seconds(5), [this]() {
                return m_bStopReconnect.load();
            });
        }

        if (m_bStopReconnect.load())
        {
            break;
        }

        std::lock_guard<std::mutex> lock(m_mutex);

        for (auto& pair : m_mapSessions)
        {
            if (!pair.second->is_connected())
            {
                dlog_warn("RTMP通道%d连接断开，尝试重连...", pair.first);
                pair.second->deinit();
                int nRet = pair.second->init();
                if (nRet == 0)
                {
                    dlog_info("RTMP通道%d重连成功", pair.first);
                }
                else
                {
                    dlog_error("RTMP通道%d重连失败: %d", pair.first, nRet);
                }
            }
        }
    }
}

void CRtmpPusher::trigger_reconnect()
{
    std::lock_guard<std::mutex> lock(m_mutexReconnect);
    m_cvReconnect.notify_one();
    dlog_debug("触发RTMP重连线程立即检查");
}
