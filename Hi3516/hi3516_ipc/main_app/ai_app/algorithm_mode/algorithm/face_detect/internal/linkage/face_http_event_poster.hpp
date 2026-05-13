/**
 * @FilePath     : face_http_event_poster.hpp
 * @Description  : Async multipart HTTP event poster
 */

#pragma once

#include <atomic>
#include <condition_variable>
#include <deque>
#include <mutex>
#include <string>
#include <thread>
#include <utility>
#include <vector>

namespace FaceDetectInternal
{
/**
 * @brief   : HTTP multipart 二进制表单字段
 */
struct FaceHttpBinaryField_S
{
    /* 表单字段名 */
    std::string strName;
    /* 上传文件名 */
    std::string strFileName;
    /* 文件二进制数据 */
    std::vector<unsigned char> vecData;
};

/**
 * @brief   : HTTP multipart 推送请求
 */
struct FaceHttpPostRequest_S
{
    /* 推送目标地址 */
    std::string strUrl;
    /* 鉴权 Token */
    std::string strToken;
    /* 事件类型，用于日志和 HTTP Header 标识 */
    std::string strEventType;
    /* 普通表单字段 */
    std::vector<std::pair<std::string, std::string>> vecFields;
    /* 二进制表单字段 */
    std::vector<FaceHttpBinaryField_S> vecBinaryFields;
};

/**
 * @brief   : 人脸事件 HTTP 异步发送器
 * @note    : 算法线程只负责入队，实际 HTTP 请求由内部工作线程发送
 */
class CFaceHttpEventPoster
{
public:
    /**
     * @brief   : 获取 HTTP 异步发送器单例
     * @return   {CFaceHttpEventPoster&} HTTP 异步发送器实例
     */
    static CFaceHttpEventPoster &instance();

    /**
     * @brief   : 将 HTTP 推送请求加入发送队列
     * @param    {FaceHttpPostRequest_S} stRequest：HTTP 推送请求
     * @return   {bool} true：入队成功 false：URL 为空，未入队
     */
    bool enqueue(FaceHttpPostRequest_S stRequest);

private:
    CFaceHttpEventPoster();
    ~CFaceHttpEventPoster();
    CFaceHttpEventPoster(const CFaceHttpEventPoster &) = delete;
    CFaceHttpEventPoster &operator=(const CFaceHttpEventPoster &) = delete;

    /**
     * @brief   : HTTP 推送工作线程主循环
     * @return   {void}
     */
    void workerLoop();

    /**
     * @brief   : 发送单条 HTTP multipart 请求
     * @param    {FaceHttpPostRequest_S} &stRequest：HTTP 推送请求
     * @return   {void}
     */
    void sendRequest(const FaceHttpPostRequest_S &stRequest);

private:
    std::atomic<bool> m_bRunning;
    std::mutex m_mutex;
    std::condition_variable m_condition;
    std::deque<FaceHttpPostRequest_S> m_queue;
    std::thread m_thread;
};
} // namespace FaceDetectInternal
