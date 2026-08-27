/**
 * @FilePath     : rtmp_pusher.cpp
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2026-05-12 13:56:20
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-08-20 17:30:00
 * @Description  : RTMP推流管理器实现
 */

#include "rtmp_pusher.h"

#include <vector>

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
    m_bReconnectRequested.store(false);
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
    m_bReconnectRequested.store(false);
    m_cvReconnect.notify_all();
    if (m_reconnectThread.joinable())
    {
        m_reconnectThread.join();
    }

    /*
     * lock: 管理器锁只用于将会话所有权移出映射；网络关闭和线程回收必须在锁外执行，
     * 否则会阻塞VENC/AENC媒体生产路径的send_*_data调用。
     */
    std::vector<std::shared_ptr<CRtmpSession>> vSessions;
    {
        std::lock_guard<std::mutex> lockLifecycle(m_mutexLifecycle);
        std::lock_guard<std::mutex> lock(m_mutex);
        for (auto &pair : m_mapSessions)
        {
            if (pair.second)
            {
                vSessions.push_back(pair.second);
            }
        }
        m_mapSessions.clear();
        m_mapSessionGenerations.clear();
    }

    for (const auto &pSession : vSessions)
    {
        pSession->deinit();
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
                            const std::string &strUrl,
                            const Video_NS::VideoConfig_S &stVideoConfig,
                            const Audio_NS::AudioConfig_S &stAudioConfig)
{
    /*
     * lock: 生命周期操作可串行，但绝不能占用m_mutex执行网络建连。
     * 发送路径只会短暂占用m_mutex获取shared_ptr，因此RTMP服务端慢不会冻结媒体线程。
     */
    std::lock_guard<std::mutex> lockLifecycle(m_mutexLifecycle);

    std::shared_ptr<CRtmpSession> pOldSession;
    uint64_t uGeneration = 0;
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        auto it = m_mapSessions.find(nChannel);
        if (it != m_mapSessions.end() && it->second && it->second->is_connected())
        {
            dlog_warn("通道%d已在推流中", nChannel);
            return OK;
        }

        uGeneration = ++m_mapSessionGenerations[nChannel];
        if (it != m_mapSessions.end())
        {
            pOldSession = it->second;
            m_mapSessions.erase(it);
        }
    }

    if (pOldSession)
    {
        pOldSession->deinit();
    }

    /* memory: 候选会话在锁外独占建连，建连结果完成后才原子发布到映射。 */
    std::shared_ptr<CRtmpSession> pSession(new CRtmpSession(nChannel, strUrl, stVideoConfig, stAudioConfig));
    int nRet = pSession->init();

    bool bInstalled = false;
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        auto itGeneration = m_mapSessionGenerations.find(nChannel);
        if (itGeneration != m_mapSessionGenerations.end() && itGeneration->second == uGeneration)
        {
            /* 即使首建失败也保存候选会话，供重连线程按原有语义继续恢复。 */
            m_mapSessions[nChannel] = pSession;
            bInstalled = true;
        }
    }

    if (!bInstalled)
    {
        /* 会话已被停止或新配置替换，锁外释放候选会话。 */
        pSession->deinit();
        dlog_warn("通道%d推流启动结果已过期，丢弃候选会话", nChannel);
        return ERR;
    }

    if (nRet < 0)
    {
        dlog_error("启动通道%d推流失败: %d", nChannel, nRet);
        return nRet;
    }

    dlog_info("启动通道%d推流成功, URL=%s", nChannel, strUrl.c_str());

    return OK;
}

int CRtmpPusher::stop_push(int nChannel)
{
    std::lock_guard<std::mutex> lockLifecycle(m_mutexLifecycle);

    std::shared_ptr<CRtmpSession> pSession;
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        ++m_mapSessionGenerations[nChannel];
        auto it = m_mapSessions.find(nChannel);
        if (it == m_mapSessions.end())
        {
            dlog_warn("通道%d未在推流", nChannel);
            return OK;
        }

        pSession = it->second;
        m_mapSessions.erase(it);
    }

    if (pSession)
    {
        pSession->deinit();
    }
    dlog_info("停止通道%d推流", nChannel);

    return OK;
}

int CRtmpPusher::send_video_data(int nChannel, Video_NS::VideoFrame_S *pVideoFrame)
{
    if (!pVideoFrame)
    {
        return ERR_PTR_NULL;
    }

    return send_video_data(nChannel, pVideoFrame->pData, pVideoFrame->nLen, pVideoFrame->eType);
}

int CRtmpPusher::send_video_data(int nChannel,
                                 const uint8_t *pData,
                                 int nDataLen,
                                 Video_NS::NalType_E eType)
{
    if (!pData || nDataLen <= 0)
    {
        return ERR_PARAM;
    }

    std::shared_ptr<CRtmpSession> pSession;
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        auto it = m_mapSessions.find(nChannel);
        if (it != m_mapSessions.end())
        {
            pSession = it->second;
        }
    }

    if (!pSession || !pSession->is_connected())
    {
        return ERR;
    }

    return pSession->send_video_frame(pData, nDataLen, eType);
}

int CRtmpPusher::send_video_data(int nChannel,
                                 const Video_NS::SharedMediaFrame_S &stSharedFrame,
                                 Video_NS::NalType_E eType)
{
    if (!stSharedFrame.pData || stSharedFrame.nLen <= 0)
    {
        return ERR_PARAM;
    }

    std::shared_ptr<CRtmpSession> pSession;
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        auto it = m_mapSessions.find(nChannel);
        if (it != m_mapSessions.end())
        {
            pSession = it->second;
        }
    }

    if (!pSession || !pSession->is_connected())
    {
        return ERR;
    }

    return pSession->send_video_frame(stSharedFrame, eType);
}

int CRtmpPusher::send_audio_data(int nChannel, Audio_NS::AudioFrame_S *pAudioFrame)
{
    if (!pAudioFrame)
    {
        return ERR_PTR_NULL;
    }

    std::shared_ptr<CRtmpSession> pSession;
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        auto it = m_mapSessions.find(nChannel);
        if (it != m_mapSessions.end())
        {
            pSession = it->second;
        }
    }

    if (!pSession)
    {
        /* 未创建会话时音频帧可能持续进入，默认关闭逐帧丢弃日志。 */
        /* dlog_debug("RTMP通道%d未创建，丢弃音频帧", nChannel); */
        return ERR;
    }
    if (!pSession->is_connected())
    {
        /* 断线重连期间会持续丢弃音频帧，默认关闭逐帧日志。 */
        /* dlog_debug("RTMP通道%d未连接，丢弃音频帧", nChannel); */
        return ERR;
    }

    return pSession->send_audio_frame(pAudioFrame);
}

int CRtmpPusher::send_audio_data(Audio_NS::AudioFrame_S *pAudioFrame)
{
    if (!pAudioFrame)
    {
        return ERR_PTR_NULL;
    }

    int nRet = OK;
    std::vector<std::shared_ptr<CRtmpSession>> vSessions;
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        vSessions.reserve(m_mapSessions.size());
        for (const auto &pair : m_mapSessions)
        {
            if (pair.second)
            {
                vSessions.push_back(pair.second);
            }
        }
    }

    for (const auto &pSession : vSessions)
    {
        if (pSession->is_connected())
        {
            int nSendRet = pSession->send_audio_frame(pAudioFrame);
            if (nSendRet != OK)
            {
                /* 音频广播失败可能在纯视频降级或断线期间高频出现，默认不刷 DEBUG。 */
                nRet = nSendRet;
            }
        }
    }

    return nRet;
}

void CRtmpPusher::reconnect_loop()
{
    while (!m_bStopReconnect.load())
    {
        bool bImmediateReconnect = false;
        {
            std::unique_lock<std::mutex> lock(m_mutexReconnect);
            m_cvReconnect.wait_for(lock,
                                   std::chrono::seconds(5),
                                   [this]()
                                   {
                                       // return m_bStopReconnect.load();
                                       return m_bStopReconnect.load() || m_bReconnectRequested.load();
                                   });

            /* 消费请求后再执行重连检查，避免同一请求被重复触发。 */
            bImmediateReconnect = m_bReconnectRequested.exchange(false);
        }

        if (m_bStopReconnect.load())
        {
            break;
        }

        if (bImmediateReconnect)
        {
            dlog_info("RTMP重连线程收到立即重连请求");
        }

        struct SessionSnapshot_S
        {
            int nChannel = 0;
            uint64_t uGeneration = 0;
            std::shared_ptr<CRtmpSession> pSession;
        };
        std::vector<SessionSnapshot_S> vBrokenSessions;
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            vBrokenSessions.reserve(m_mapSessions.size());
            for (const auto &pair : m_mapSessions)
            {
                if (pair.second && !pair.second->is_connected())
                {
                    SessionSnapshot_S stSnapshot;
                    stSnapshot.nChannel = pair.first;
                    stSnapshot.uGeneration = m_mapSessionGenerations[pair.first];
                    stSnapshot.pSession = pair.second;
                    vBrokenSessions.push_back(stSnapshot);
                }
            }
        }

        for (const auto &stSnapshot : vBrokenSessions)
        {
            /*
             * lock: 生命周期锁只串行控制面操作。RTMP网络建连仍在m_mutex外，
             * 不会阻塞VENC/VPSS/AENC调用send_*_data。
             */
            std::lock_guard<std::mutex> lockLifecycle(m_mutexLifecycle);
            bool bStillCurrent = false;
            {
                std::lock_guard<std::mutex> lock(m_mutex);
                const auto itSession = m_mapSessions.find(stSnapshot.nChannel);
                const auto itGeneration = m_mapSessionGenerations.find(stSnapshot.nChannel);
                bStillCurrent = itSession != m_mapSessions.end() && itGeneration != m_mapSessionGenerations.end() &&
                                itSession->second == stSnapshot.pSession && itGeneration->second == stSnapshot.uGeneration;
            }
            if (!bStillCurrent || m_bStopReconnect.load())
            {
                continue;
            }

            dlog_warn("RTMP通道%d连接断开，尝试重连...", stSnapshot.nChannel);
            stSnapshot.pSession->deinit();
            int nRet = stSnapshot.pSession->init();
            if (nRet == OK)
            {
                dlog_info("RTMP通道%d重连成功", stSnapshot.nChannel);
            }
            else
            {
                dlog_error("RTMP通道%d重连失败: %d", stSnapshot.nChannel, nRet);
            }
        }
    }
}

void CRtmpPusher::trigger_reconnect()
{
    // std::lock_guard<std::mutex> lock(m_mutexReconnect);
    /* 先记录请求，再唤醒等待线程；通知早到也会由等待条件消费。 */
    m_bReconnectRequested.store(true);
    m_cvReconnect.notify_one();
    dlog_debug("触发RTMP重连线程立即检查");
}
