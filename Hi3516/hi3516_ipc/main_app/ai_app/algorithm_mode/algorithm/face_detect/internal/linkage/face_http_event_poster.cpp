/**
 * @FilePath     : face_http_event_poster.cpp
 * @Description  : Async multipart HTTP event poster implementation
 */

#include "face_http_event_poster.hpp"

#include <list>

#include "CurlMultipartHttpPost.h"
#include "dlog.h"

namespace
{
constexpr size_t FACE_HTTP_EVENT_QUEUE_MAX = 32;

/**
 * @brief   : 添加非空 HTTP 头
 * @param    {std::list<std::string>} &listHeader：HTTP 头列表
 * @param    {std::string} &strHeader：待添加 HTTP 头
 * @return   {void}
 */
void add_header(std::list<std::string> &listHeader, const std::string &strHeader)
{
    if (!strHeader.empty())
    {
        listHeader.emplace_back(strHeader);
    }
}
} // namespace

namespace FaceDetectInternal
{
/**
 * @brief   : 获取 HTTP 异步发送器单例
 * @return   {CFaceHttpEventPoster&} HTTP 异步发送器实例
 */
CFaceHttpEventPoster &CFaceHttpEventPoster::instance()
{
    static CFaceHttpEventPoster stInstance;
    return stInstance;
}

/**
 * @brief   : 构造 HTTP 异步发送器并启动发送线程
 */
CFaceHttpEventPoster::CFaceHttpEventPoster()
    : m_bRunning(true),
      m_thread(&CFaceHttpEventPoster::workerLoop, this)
{
}

/**
 * @brief   : 析构 HTTP 异步发送器并停止发送线程
 */
CFaceHttpEventPoster::~CFaceHttpEventPoster()
{
    m_bRunning.store(false);
    m_condition.notify_all();
    if (m_thread.joinable())
    {
        m_thread.join();
    }
}

/**
 * @brief   : 将 HTTP 推送请求加入发送队列
 * @param    {FaceHttpPostRequest_S} stRequest：HTTP 推送请求
 * @return   {bool} true：入队成功 false：URL 为空，未入队
 */
bool CFaceHttpEventPoster::enqueue(FaceHttpPostRequest_S stRequest)
{
    if (stRequest.strUrl.empty())
    {
        return false;
    }

    {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (m_queue.size() >= FACE_HTTP_EVENT_QUEUE_MAX)
        {
            dlog_warn("人脸 HTTP 推送队列已满，丢弃最旧事件");
            m_queue.pop_front();
        }
        m_queue.emplace_back(std::move(stRequest));
    }
    m_condition.notify_one();
    return true;
}

/**
 * @brief   : HTTP 推送工作线程主循环
 * @return   {void}
 */
void CFaceHttpEventPoster::workerLoop()
{
    while (m_bRunning.load())
    {
        FaceHttpPostRequest_S stRequest;
        {
            std::unique_lock<std::mutex> lock(m_mutex);
            m_condition.wait(lock, [this] {
                return !m_bRunning.load() || !m_queue.empty();
            });

            if (!m_bRunning.load() && m_queue.empty())
            {
                break;
            }

            stRequest = std::move(m_queue.front());
            m_queue.pop_front();
        }

        sendRequest(stRequest);
    }
}

/**
 * @brief   : 发送单条 HTTP multipart 请求
 * @param    {FaceHttpPostRequest_S} &stRequest：HTTP 推送请求
 * @return   {void}
 */
void CFaceHttpEventPoster::sendRequest(const FaceHttpPostRequest_S &stRequest)
{
    CurlHttp::CCurlMultipartHttpPost stPost(stRequest.strUrl);

    std::list<std::string> listHeader;
    add_header(listHeader, "Expect:");
    add_header(listHeader, "X-Face-Event-Type: " + stRequest.strEventType);
    if (!stRequest.strToken.empty())
    {
        add_header(listHeader, "Authorization: Bearer " + stRequest.strToken);
    }
    stPost.set_header(listHeader);

    for (const auto &field : stRequest.vecFields)
    {
        stPost.add_formData(field.first, field.second);
    }

    for (const auto &binary : stRequest.vecBinaryFields)
    {
        if (binary.vecData.empty())
        {
            continue;
        }
        void *pData = const_cast<unsigned char *>(binary.vecData.data());
        stPost.add_formData(binary.strName, binary.strFileName, pData, binary.vecData.size());
    }

    const int nRet = stPost.send_request();
    std::string strResponse;
    stPost.get_recvData(strResponse);
    stPost.clear_form();

    if (nRet != 0)
    {
        dlog_warn("人脸 HTTP 推送失败 event[%s] ret[%d] err[%s] url[%s]",
                  stRequest.strEventType.c_str(),
                  nRet,
                  CurlHttp::Base::get_error(nRet).c_str(),
                  stRequest.strUrl.c_str());
        return;
    }

    dlog_info("人脸 HTTP 推送成功 event[%s] url[%s] response[%s]",
              stRequest.strEventType.c_str(),
              stRequest.strUrl.c_str(),
              strResponse.c_str());
}
} // namespace FaceDetectInternal
