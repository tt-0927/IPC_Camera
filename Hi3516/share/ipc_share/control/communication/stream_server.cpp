/**
 * @FilePath     : stream_server.cpp
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2025-06-30 13:57:26
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-08-20 17:30:00
 * @Description  : 录制送流、配置服务端
 */

#include "stream_server.h"

#include <algorithm>
#include <chrono>
#include <cstring>
#include <exception>
#include <new>
#include "dlog.h"
#include <unistd.h>
#include "Json.h"
#include "record_define.h"
#include "record_ctrl.h"
#include "action_code.h"
#include "libavcodec/codec_id.h"
#include "libavutil/samplefmt.h"
#include "algo_detect.h"

namespace
{
/* 限制队列帧数，避免大量小音频包造成对象数量膨胀。 */
constexpr std::size_t RECORD_MEDIA_QUEUE_MAX_FRAMES = 8U;
/* 丢帧日志限频，防止录制进程断开时反向冲击实时线程。 */
constexpr long long RECORD_MEDIA_DROP_LOG_INTERVAL_MS = 1000;
/* 录制工作线程的慢写探针阈值，不影响VENC/AENC生产线程。 */
constexpr long long RECORD_MEDIA_SEND_WARN_MS = 100;

long long get_steady_timestamp_ms()
{
    return std::chrono::duration_cast<std::chrono::milliseconds>(
               std::chrono::steady_clock::now().time_since_epoch())
        .count();
}
} // namespace

CStreamServer::~CStreamServer()
{
    deinit();
}

int CStreamServer::init()
{
    if (m_bMediaWorkerRunning.load(std::memory_order_acquire))
    {
        return OK;
    }

    Net::Param_S stParam;
    /* 设置心跳码、状态码 */
    stParam.stInitParam.nHearbeatCode = AC_HEARTBIT;
    stParam.stInitParam.nStatusCode = AC_STATUS;
    /* 设置命令码回调函数 */
    using namespace std::placeholders;
    stParam.stInitParam.fnDefaultCallback = std::bind(&CStreamServer::deal_message, this, _1, _2);
    stParam.stInitParam.callbackMap[stParam.stInitParam.nHearbeatCode] = std::bind(&CStreamServer::deal_heartbeat, this, _1, _2);
    stParam.stInitParam.callbackMap[stParam.stInitParam.nStatusCode] = std::bind(&CStreamServer::deal_status, this, _1, _2);

    /* 设置ip、端口号 */
    stParam.stInitParam.ip = std::string("127.0.0.1");
    stParam.stInitParam.nPort = IN_RECORD_STREAM_PROT_ONE;

    /* 创建服务端 */
    {
        std::lock_guard<std::mutex> lock(m_mtxIoSend);
        m_pHandler = std::make_shared<Net::UDSServer>(stParam);
    }

    {
        std::lock_guard<std::mutex> lock(m_mtxMediaQueue);
        m_mediaQueue.clear();
        m_nMediaQueueBytes = 0;
        m_nMediaInFlightBytes = 0;
        m_nMediaHighWaterFrames = 0;
        m_nMediaHighWaterBytes = 0;
        m_ullDroppedMediaFrames = 0;
        m_ullDroppedMediaBytes = 0;
        m_llLastMediaDropLogMs = 0;
        m_llLastMediaSendWarnMs = 0;
    }

    m_bRecordReady.store(false, std::memory_order_release);
    m_bMediaWorkerRunning.store(true, std::memory_order_release);
    m_mediaSendThread = std::thread(&CStreamServer::media_send_loop, this);
    return OK;
}

void CStreamServer::deinit()
{
    m_bRecordReady.store(false, std::memory_order_release);
    m_bConnect.store(false, std::memory_order_release);
    m_bMediaWorkerRunning.store(false, std::memory_order_release);
    m_cvMediaQueue.notify_all();

    if (m_mediaSendThread.joinable())
    {
        m_mediaSendThread.join();
    }

    clear_media_queue();

    /* worker 已停止后再释放 UDS 句柄，避免异步发送线程访问悬空对象。 */
    {
        std::lock_guard<std::mutex> lock(m_mtxIoSend);
        m_pHandler.reset();
    }
}

int CStreamServer::send(std::string data, int nActionCode, void *pHandle)
{
    Net::Message_S stMessage;
    stMessage.pHandle = pHandle;
    stMessage.nActionCode = nActionCode;
    stMessage.pData = data.c_str();
    stMessage.nDataLength = data.length() + 1;

    std::lock_guard<std::mutex> lock(m_mtxIoSend);
    if (!m_pHandler)
    {
        return ERR;
    }

    if (!m_bConnect.load(std::memory_order_acquire))
    {
        return ERR;
    }

    dlog_info("发送[%d]消息：%s", stMessage.nActionCode, stMessage.pData);
    return m_pHandler->send(stMessage);
}

int CStreamServer::send(void *pData, int nLen, int nActionCode, void *pHandle)
{
    if (!pData || nLen <= 0 || nActionCode < 0)
    {
        return ERR_PTR_NULL;
    }

    Net::Message_S stMessage;
    stMessage.pHandle = pHandle;
    stMessage.nActionCode = nActionCode;
    stMessage.pData = pData;
    stMessage.nDataLength = nLen;

    std::lock_guard<std::mutex> lock(m_mtxIoSend);
    if (!m_pHandler)
    {
        return ERR_UNINIT;
    }

    if (!m_bConnect.load(std::memory_order_acquire))
    {
        return ERR;
    }

    return m_pHandler->send(stMessage);
}

void CStreamServer::deal_heartbeat(Net::Message_S &stMessage, Net::UserParam_S &stUserParam)
{
    // dlog_info("接收到心跳消息：%s", stMessage.pData);
}

void CStreamServer::deal_status(Net::Message_S &stMessage, Net::UserParam_S &stUserParam)
{
    int nStatus = *(const int *)stMessage.pData;
    if (nStatus == Net::STATUS_SUCCESS)
    {
        m_bConnect.store(true, std::memory_order_release);
        m_bRecordReady.store(false, std::memory_order_release);
        CRecordCtrl::instance()->set_record_process_status(true);
        dlog_info("record客户端已接入");

        /* 录制AAC通道独立于网页/RTSP音频配置，接入时始终先同步实际编码参数。 */
        if (send_record_audio_config() != OK)
        {
            dlog_warn("录制客户端接入后同步音频配置失败");
        }

        /* 接入事件触发时执行的配置同步回调副本 */
        std::function<void()> fnCallback;
        {
            /* lock: 回调可由视频模块在任意线程更新，通讯线程只复制后执行 */
            std::lock_guard<std::mutex> lock(m_mtxRecordConnectedCallback);
            fnCallback = m_fnRecordConnectedCallback;
        }

        if (fnCallback)
        {
            try
            {
                fnCallback();
            }
            catch (const std::exception &e)
            {
                dlog_error("录制进程接入后的状态同步异常:%s", e.what());
            }
            catch (...)
            {
                dlog_error("录制进程接入后的状态同步发生未知异常");
            }
        }

        /* 配置同步完成后才允许媒体工作线程发送，避免首帧先于编码参数到达录制端。 */
        m_bRecordReady.store(true, std::memory_order_release);
    }
    else
    {
        m_bRecordReady.store(false, std::memory_order_release);
        m_bConnect.store(false, std::memory_order_release);
        CRecordCtrl::instance()->set_record_process_status(false);
        clear_media_queue();
        dlog_info("record客户端已断开");
    }
}

void CStreamServer::deal_message(Net::Message_S &stMessage, Net::UserParam_S &stUserParam)
{
    // int nRetCode = 0;

    dlog_info("接收到[%d]消息：%s", stMessage.nActionCode, (char *)stMessage.pData);

    /* 接收到的数据 */
    // std::string strMsgData(static_cast<const char*>(stMessage.pData));

    /* Data字段数据 */
    // std::string strData;
    // std::string   data;
    // Json::Object* pJsonRoot = Json::init(strMsgData);
    // if (!pJsonRoot)
    // {
    //     ddlog_error("参数错误，不是json数据");
    //     return;
    // }

    // Json::Object* pJsonData = Json::get(pJsonRoot, "Data");
    // if(pJsonData != nullptr)
    // {
    // 	data      = Json::to_string(pJsonData);
    // }
    // Json::deinit(pJsonRoot);
    // fill_returnHead(strData, stMessage.nActionCode, nRetCode);
    // send(strData, stMessage.nActionCode);
}

/* 填充json头数据 */
void CStreamServer::fill_returnHead(std::string &strData, int nActionCode, int nRetCode)
{
    Json::Object *pJsonRoot = Json::init();
    Json::add(pJsonRoot, "ActionCode", nActionCode);
    Json::add(pJsonRoot, "DeviceName", "");
    Json::add(pJsonRoot, "Return", nRetCode);

    if (!strData.empty())
    {
        Json::Object *pJsonData = Json::init(strData);
        if (pJsonData)
        {
            Json::add(pJsonRoot, "Data", pJsonData);
        }
    }

    strData = Json::to_string(pJsonRoot);
    Json::deinit(pJsonRoot);
    return;
}

void CStreamServer::fill_head(std::string &strData, int nActionCode)
{
    Json::Object *pJsonRoot = Json::init();
    Json::add(pJsonRoot, "ActionCode", nActionCode);
    Json::add(pJsonRoot, "DeviceName", "");

    if (!strData.empty())
    {
        Json::Object *pJsonData = Json::init(strData);
        if (pJsonData)
        {
            Json::add(pJsonRoot, "Data", pJsonData);
        }
    }

    strData = Json::to_string(pJsonRoot);
    Json::deinit(pJsonRoot);
    return;
}

int CStreamServer::sendVideoData(Video_NS::VideoFrame_S *pVideoFrame)
{
    if (!pVideoFrame)
    {
        return ERR_PTR_NULL;
    }

    return sendVideoData(pVideoFrame->pData, pVideoFrame->nLen);
}

int CStreamServer::sendVideoData(const uint8_t *pData, int nDataLen)
{
    if (!pData || nDataLen <= 0)
    {
        return ERR_PARAM;
    }

    /* perf: VENC线程只做一次受控拷贝和入队，不同步等待录制UDS写 socket。 */
    return enqueue_media_data(pData, nDataLen, AC_STREAM_VIDEO_DATE);
}

int CStreamServer::sendVideoData(const Video_NS::SharedMediaFrame_S &stSharedFrame)
{
    if (!stSharedFrame.pData || stSharedFrame.nLen <= 0)
    {
        return ERR_PARAM;
    }

    /* perf: 共享帧零拷贝入队，仅增加引用计数，与 RTSP/RTMP 共享同一份 buffer。 */
    return enqueue_media_data(stSharedFrame.pData, stSharedFrame.nLen, AC_STREAM_VIDEO_DATE);
}

int CStreamServer::sendVideoConfig(const Video_NS::VideoConfig_S &stVideoConfig)
{
    /*
     * 根据码率动态调整录制媒体队列预算：单帧上限取1秒码量，队列总字节取2倍单帧上限。
     * 码率上限类型使用码率上限、变码率类型使用平均码率估算，避免高码率下大I帧击穿旧固定上限。
     */
    const std::size_t unMaxFrameBytes = Video_NS::calcMaxFrameBytes(stVideoConfig);
    m_nMediaMaxFrameBytes.store(unMaxFrameBytes, std::memory_order_relaxed);
    m_nMediaQueueMaxBytes.store(unMaxFrameBytes * 2U, std::memory_order_relaxed);

    Record_NS::VideoConfigInfo_S stVideoConfigInfo;
    stVideoConfigInfo.nVencWidth = stVideoConfig.stVideoResolution.nWidth;
    stVideoConfigInfo.nVencHeight = stVideoConfig.stVideoResolution.nHeight;
    stVideoConfigInfo.nFps = stVideoConfig.getFrameRateAsInt();
    stVideoConfigInfo.nVideoCodeID = AV_CODEC_ID_H264;
    if (stVideoConfig.enVideoCodec == Video_NS::VideoCodec_E::H264)
    {
        stVideoConfigInfo.nVideoCodeID = AV_CODEC_ID_H264;
    }
    else if (stVideoConfig.enVideoCodec == Video_NS::VideoCodec_E::H265)
    {
        stVideoConfigInfo.nVideoCodeID = AV_CODEC_ID_HEVC;
    }

    return send(&stVideoConfigInfo, sizeof(Record_NS::VideoConfigInfo_S), AC_CONTROL_VIDEO_CONFIG);
}

int CStreamServer::sendAudioData(Audio_NS::AudioFrame_S *pAudioFrame)
{
    if (!pAudioFrame)
    {
        return ERR_PTR_NULL;
    }

    /* memory: AENC原始缓冲区由调用方在返回后释放，队列必须持有独立副本。 */
    return enqueue_media_data(pAudioFrame->pData, pAudioFrame->nLen, AC_STREAM_AUDIO_DATE);
}

int CStreamServer::enqueue_media_data(const void *pData, int nLen, int nActionCode)
{
    return enqueue_media_data_impl(pData, nLen, nActionCode, nullptr, 0);
}

int CStreamServer::enqueue_media_data(const std::shared_ptr<std::uint8_t[]> &pSharedData,
                                      int nLen,
                                      int nActionCode)
{
    return enqueue_media_data_impl(nullptr, 0, nActionCode, pSharedData, nLen);
}

int CStreamServer::enqueue_media_data_impl(const void *pData,
                                           int nLen,
                                           int nActionCode,
                                           const std::shared_ptr<std::uint8_t[]> &pSharedData,
                                           int nSharedLen)
{
    /* 共享路径与拷贝路径必居其一 */
    const bool bSharedPath = (pSharedData != nullptr) && (nSharedLen > 0);
    if (!bSharedPath && (!pData || nLen <= 0))
    {
        return ERR_PTR_NULL;
    }
    if (nActionCode < 0)
    {
        return ERR_PTR_NULL;
    }

    const std::size_t nDataSize = static_cast<std::size_t>(bSharedPath ? nSharedLen : nLen);

    if (!m_bMediaWorkerRunning.load(std::memory_order_acquire) ||
        !m_bConnect.load(std::memory_order_acquire) ||
        !m_bRecordReady.load(std::memory_order_acquire))
    {
        return ERR;
    }

    /* 单帧上限随码率动态变化，读取一次避免重复原子访问。 */
    const std::size_t unMaxFrameBytes = m_nMediaMaxFrameBytes.load(std::memory_order_relaxed);
    if (nDataSize > unMaxFrameBytes)
    {
        bool bLogDrop = false;
        std::uint64_t ullDroppedFrames = 0;
        {
            std::lock_guard<std::mutex> lock(m_mtxMediaQueue);
            ++m_ullDroppedMediaFrames;
            m_ullDroppedMediaBytes += nDataSize;
            const long long llNowMs = get_steady_timestamp_ms();
            if (m_llLastMediaDropLogMs == 0 ||
                llNowMs - m_llLastMediaDropLogMs >= RECORD_MEDIA_DROP_LOG_INTERVAL_MS)
            {
                m_llLastMediaDropLogMs = llNowMs;
                bLogDrop = true;
                ullDroppedFrames = m_ullDroppedMediaFrames;
            }
        }

        if (bLogDrop)
        {
            dlog_warn("录制媒体帧超过异步队列上限，丢弃本帧: action[%d] size[%zu] max[%zu] dropped[%llu]",
                      nActionCode,
                      nDataSize,
                      unMaxFrameBytes,
                      static_cast<unsigned long long>(ullDroppedFrames));
        }
        return ERR;
    }

    RecordMediaTask_S stTask;
    stTask.nActionCode = nActionCode;
    if (bSharedPath)
    {
        /* memory: 共享路径零拷贝，仅持有 shared_ptr 引用，与 RTSP/RTMP 共享同一份 buffer。 */
        stTask.pSharedData = pSharedData;
        stTask.nSharedLen = nSharedLen;
    }
    else
    {
        try
        {
            stTask.vecData.resize(nDataSize);
            std::memcpy(stTask.vecData.data(), pData, nDataSize);
        }
        catch (const std::bad_alloc &)
        {
            dlog_error("录制媒体异步副本申请内存失败: action[%d] size[%zu]", nActionCode, nDataSize);
            return ERR;
        }
    }

    bool bDropped = false;
    bool bLogDrop = false;
    std::size_t nQueueBytes = 0;
    std::uint64_t ullDroppedFrames = 0;
    {
        /* lock: 队列满时立即丢弃当前录制副本，绝不等待消费端追赶。 */
        std::lock_guard<std::mutex> lock(m_mtxMediaQueue);
        if (!m_bMediaWorkerRunning.load(std::memory_order_relaxed) ||
            !m_bConnect.load(std::memory_order_relaxed) ||
            !m_bRecordReady.load(std::memory_order_relaxed))
        {
            return ERR;
        }

        if (m_mediaQueue.size() >= RECORD_MEDIA_QUEUE_MAX_FRAMES ||
            m_nMediaQueueBytes + nDataSize > m_nMediaQueueMaxBytes.load(std::memory_order_relaxed))
        {
            bDropped = true;
            ++m_ullDroppedMediaFrames;
            m_ullDroppedMediaBytes += nDataSize;
            const long long llNowMs = get_steady_timestamp_ms();
            if (m_llLastMediaDropLogMs == 0 ||
                llNowMs - m_llLastMediaDropLogMs >= RECORD_MEDIA_DROP_LOG_INTERVAL_MS)
            {
                m_llLastMediaDropLogMs = llNowMs;
                bLogDrop = true;
                nQueueBytes = m_nMediaQueueBytes;
                ullDroppedFrames = m_ullDroppedMediaFrames;
            }
        }
        else
        {
            m_nMediaQueueBytes += nDataSize;
            m_mediaQueue.emplace_back(std::move(stTask));
            m_nMediaHighWaterFrames = std::max(m_nMediaHighWaterFrames, m_mediaQueue.size());
            m_nMediaHighWaterBytes = std::max(m_nMediaHighWaterBytes, m_nMediaQueueBytes);
        }
    }

    if (bDropped && bLogDrop)
    {
        dlog_warn("录制媒体异步队列已满，丢弃新帧: action[%d] queue_bytes[%zu] dropped[%llu]",
                  nActionCode,
                  nQueueBytes,
                  static_cast<unsigned long long>(ullDroppedFrames));
    }

    if (bDropped)
    {
        return ERR;
    }

    m_cvMediaQueue.notify_one();
    return OK;
}

void CStreamServer::media_send_loop()
{
    while (true)
    {
        RecordMediaTask_S stTask;
        /* 当前任务媒体字节数（共享/拷贝路径通用），提升到循环体作用域供 send 使用 */
        std::size_t nTaskSize = 0;
        {
            std::unique_lock<std::mutex> lock(m_mtxMediaQueue);
            m_cvMediaQueue.wait(lock, [this]() {
                return !m_mediaQueue.empty() || !m_bMediaWorkerRunning.load(std::memory_order_acquire);
            });

            if (m_mediaQueue.empty())
            {
                if (!m_bMediaWorkerRunning.load(std::memory_order_acquire))
                {
                    break;
                }
                continue;
            }

            stTask = std::move(m_mediaQueue.front());
            /* 共享路径与独立拷贝路径二选一取数 */
            nTaskSize = stTask.pSharedData
                            ? static_cast<std::size_t>(stTask.nSharedLen)
                            : stTask.vecData.size();
            m_nMediaQueueBytes = nTaskSize > m_nMediaQueueBytes
                                     ? 0
                                     : m_nMediaQueueBytes - nTaskSize;
            m_mediaQueue.pop_front();
            m_nMediaInFlightBytes += nTaskSize;
        }

        if (!m_bConnect.load(std::memory_order_acquire) ||
            !m_bRecordReady.load(std::memory_order_acquire))
        {
            {
                std::lock_guard<std::mutex> lock(m_mtxMediaQueue);
                m_nMediaInFlightBytes = nTaskSize > m_nMediaInFlightBytes
                                            ? 0
                                            : m_nMediaInFlightBytes - nTaskSize;
            }
            continue;
        }

        /* stTask 在本次同步 send 返回前保持存活，满足 UDS send 的指针生命周期要求。 */
        uint8_t *pTaskData = stTask.pSharedData ? stTask.pSharedData.get() : stTask.vecData.data();
        const long long llSendStartMs = get_steady_timestamp_ms();
        const int nRet = send(pTaskData,
                              static_cast<int>(nTaskSize),
                              stTask.nActionCode);
        const long long llSendCostMs = get_steady_timestamp_ms() - llSendStartMs;
        {
            std::lock_guard<std::mutex> lock(m_mtxMediaQueue);
            m_nMediaInFlightBytes = nTaskSize > m_nMediaInFlightBytes
                                        ? 0
                                        : m_nMediaInFlightBytes - nTaskSize;
        }
        if (llSendCostMs >= RECORD_MEDIA_SEND_WARN_MS)
        {
            bool bLogWarn = false;
            std::size_t nQueueBytes = 0;
            {
                std::lock_guard<std::mutex> lock(m_mtxMediaQueue);
                const long long llNowMs = get_steady_timestamp_ms();
                if (m_llLastMediaSendWarnMs == 0 ||
                    llNowMs - m_llLastMediaSendWarnMs >= RECORD_MEDIA_DROP_LOG_INTERVAL_MS)
                {
                    m_llLastMediaSendWarnMs = llNowMs;
                    bLogWarn = true;
                    nQueueBytes = m_nMediaQueueBytes;
                }
            }
            if (bLogWarn)
            {
                dlog_warn("录制媒体UDS发送耗时较长: action[%d] size[%zu] cost[%lldms] queue_bytes[%zu]",
                          stTask.nActionCode,
                          nTaskSize,
                          llSendCostMs,
                          nQueueBytes);
            }
        }
        if (nRet < 0)
        {
            /* 连接断开后的剩余任务不再重试，避免旧媒体帧堆积到重连之后。 */
            if (!m_bConnect.load(std::memory_order_acquire))
            {
                clear_media_queue();
            }
        }
    }
}

void CStreamServer::clear_media_queue()
{
    {
        std::lock_guard<std::mutex> lock(m_mtxMediaQueue);
        for (const RecordMediaTask_S &stTask : m_mediaQueue)
        {
            ++m_ullDroppedMediaFrames;
            m_ullDroppedMediaBytes += stTask.pSharedData
                                          ? static_cast<std::size_t>(stTask.nSharedLen)
                                          : stTask.vecData.size();
        }
        m_mediaQueue.clear();
        m_nMediaQueueBytes = 0;
    }
}

int CStreamServer::send_record_audio_config()
{
    Record_NS::AudioConfigInfo_S stAudioConfigInfo;
    stAudioConfigInfo.nSampleRate = 16000;
    stAudioConfigInfo.nAudioCodeID = AV_CODEC_ID_AAC;
    stAudioConfigInfo.nSampleFmt = AV_SAMPLE_FMT_S16;
    stAudioConfigInfo.nChannel = 1;

    return send(&stAudioConfigInfo, sizeof(Record_NS::AudioConfigInfo_S), AC_CONTROL_AUDIO_CONFIG);
}

int CStreamServer::notify_record_audio_restart()
{
    /* note: 空消息仅作为控制事件，录制端不依赖消息体内容。 */
    return send(std::string(), AC_NOTICE_RECORD_AUDIO_RESTART);
}

void CStreamServer::set_record_connected_callback(std::function<void()> fnCallback)
{
    /* 注册晚于连接建立时立即执行的配置同步回调副本 */
    std::function<void()> fnSyncCallback;
    {
        /* lock: 注册时覆盖旧回调，并记录当前连接态避免漏掉已建立的连接 */
        std::lock_guard<std::mutex> lock(m_mtxRecordConnectedCallback);
        m_fnRecordConnectedCallback = std::move(fnCallback);
        if (m_bConnect.load(std::memory_order_acquire))
        {
            fnSyncCallback = m_fnRecordConnectedCallback;
        }
    }

    if (fnSyncCallback)
    {
        /* note: 不在锁内执行回调，避免回调再次访问通讯服务时死锁 */
        fnSyncCallback();
    }
}
