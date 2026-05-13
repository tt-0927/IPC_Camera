/**
 * @FilePath     : result_manager.hpp
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2025-08-08 15:12:36
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2025-09-01 15:09:49
 * @Description  : 结果管理器类
 */
#pragma once
#include <vector>
#include <mutex>
#include <thread>
#include <chrono>
#include <atomic>
#include <map>
#include <list>

#include "osd_manage.h"

class CResultManager : public CSingleton<CResultManager>
{
public:
    /* 算法结果类型枚举 */
    enum AlgorithmType
    {
        ALGORITHM_BOX = 0,    /* 目标框结果 */
        ALGORITHM_POINT = 1,  /* 关键点结果 */
        ALGORITHM_MOTION = 2, /* 移动侦测结果 */
        ALGORITHM_AI = 3,     /* 海思AI检测结果 */
        ALGORITHM_OSD = 4     /* OSD样式结果 */
    };

    /* 检测结果缓存结构 */
    struct DetectionCache
    {
        /* 框对应的分辨率宽 */
        int nWidth;
        /* 框对应的分辨率高 */
        int nHeight;
        /* OSD结果容器 */
        std::vector<Common::RectInfo_S> vstRectInfo;
        /* 时间戳 */
        std::chrono::steady_clock::time_point timestamp;
        /* 算法结果类型*/
        AlgorithmType algorithmType;

        DetectionCache() : nWidth(0), nHeight(0), timestamp(std::chrono::steady_clock::now()), algorithmType(ALGORITHM_OSD)
        {
        }
    };

public:
    CResultManager();
    ~CResultManager();
    friend class CSingleton<CResultManager>;

    /**
     * @brief   : 初始化
     * @return   {bool} true：成功 false：失败
     */
    bool init();

    /**
     * @brief   : 反初始化
     */
    void uninit();

    /**
     * @brief   : 添加检测结果到缓存队列
     * @param    {AlgorithmType} algorithmType：算法结果类型
     * @param    {int} nWidth：框对应的分辨率宽
     * @param    {int} nHeight：框对应的分辨率高
     * @param    {vector<Common::RectInfo_S>} &vstRectInfo：检测结果框数据
     */
    void addDetectionResult(AlgorithmType algorithmType, int nWidth, int nHeight, const std::vector<Common::RectInfo_S> &vstRectInfo);

private:
    /**
     * @brief   : 合并线程函数，定时处理缓存的检测结果
     */
    void mergeThreadFunc();

    /**
     * @brief   : 合并所有有效的检测结果
     * @param    {int} &outWidth：输出合并后的分辨率宽
     * @param    {int} &outHeight：输出合并后的分辨率高
     * @return   {vector<Common::RectInfo_S>} 合并后的检测框结果
     */
    std::vector<Common::RectInfo_S> mergeValidResults(int &outWidth, int &outHeight);

    /**
     * @brief   : 检查结果是否有效（未过期）
     * @param    {DetectionCache} &cache：待检查的缓存结果
     * @return   {bool} true：有效 false：已过期
     */
    bool isResultValid(const DetectionCache &cache);

    /**
     * @brief   : 去重重叠的检测框
     * @param    {vector<Common::RectInfo_S>} &rects：待处理的检测框数组
     * @return   {vector<Common::RectInfo_S>} 去重后的检测框数组
     */
    std::vector<Common::RectInfo_S> removeDuplicateRects(const std::vector<Common::RectInfo_S> &rects);

    /**
     * @brief   : 计算两个矩形的重叠率（IoU）
     * @param    {Common::RectInfo_S} &rect1：第一个矩形
     * @param    {Common::RectInfo_S} &rect2：第二个矩形
     * @return   {double} 重叠率，范围[0.0, 1.0]
     */
    double calculateOverlapRatio(const Common::RectInfo_S &rect1, const Common::RectInfo_S &rect2);

private:
    /* 缓存互斥锁 */
    std::mutex m_cacheMutex;
    /* 算法结果类型与检测结果缓存结构映射 */
    std::map<AlgorithmType, std::list<DetectionCache>> m_resultCache;
    /* 合并检测结果线程 */
    std::thread m_mergeThread;
    /* 线程运行标志 */
    std::atomic<bool> m_bRunning;
    /* 上一次是否发送了有效结果的标志 */
    std::atomic<bool> m_bLastSentValid;
    /* 合并发送间隔（毫秒） */
    const int MERGE_INTERVAL_MS = 300;
    /* 结果有效期（毫秒） */
    const int RESULT_VALID_TIME_MS = 450;
    /* 每个算法类型的最大缓存数量 */
    const size_t MAX_CACHE_PER_TYPE = 5;
};
