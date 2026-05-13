/**
 * @FilePath     : result_manager.cpp
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2025-08-08 15:12:14
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2025-09-01 14:59:52
 * @Description  : 结果管理器类实现
 */

#include "result_manager.hpp"

CResultManager::CResultManager() : m_bRunning(false), m_bLastSentValid(false)
{
    init();
}

CResultManager::~CResultManager()
{
    uninit();
}

bool CResultManager::init()
{
    /* 检查是否已经运行 */
    if (m_bRunning.load())
    {
        return true;
    }

    /* 启动合并线程 */
    m_bRunning.store(true);
    m_bLastSentValid.store(false);
    m_mergeThread = std::thread(&CResultManager::mergeThreadFunc, this);

    return true;
}

void CResultManager::uninit()
{
    if (m_bRunning.load())
    {
        /* 停止线程运行标志 */
        m_bRunning.store(false);

        /* 等待线程结束 */
        if (m_mergeThread.joinable())
        {
            m_mergeThread.join();
        }
    }
}

void CResultManager::addDetectionResult(AlgorithmType algorithmType, int nWidth, int nHeight, const std::vector<Common::RectInfo_S> &vstRectInfo)
{
    std::lock_guard<std::mutex> lock(m_cacheMutex);

    /* 获取对应算法类型的缓存列表 */
    auto& cacheList = m_resultCache[algorithmType];

    /* 创建新的缓存结构 */
    DetectionCache cache;
    cache.nWidth = nWidth;
    cache.nHeight = nHeight;
    cache.vstRectInfo = vstRectInfo;
    cache.timestamp = std::chrono::steady_clock::now();
    cache.algorithmType = algorithmType;

    /* 添加到列表末尾 */
    cacheList.push_back(cache);

    /* 限制每个算法类型的缓存数量，移除最旧的 */
    while (cacheList.size() > MAX_CACHE_PER_TYPE)
    {
        cacheList.pop_front();
    }
}

void CResultManager::mergeThreadFunc()
{
    pthread_setname_np(pthread_self(), "AiResultManager");

    while (m_bRunning.load())
    {
        int mergedWidth = 0, mergedHeight = 0;

        /* 合并有效的检测结果 */
        auto mergedResults = mergeValidResults(mergedWidth, mergedHeight);

        /* 判断是否有有效结果 */
        bool hasValidResult = (mergedWidth > 0 && mergedHeight > 0 && !mergedResults.empty());

        if (hasValidResult)
        {
            /* 有有效结果时发送 */
            COsdManage::instance()->send_detection_result(mergedWidth, mergedHeight, mergedResults);
            m_bLastSentValid.store(true);
        }
        else if (m_bLastSentValid.load())
        {
            /* 当前无有效结果，但上次发送了有效结果，需要发送空结果进行清除 */
            std::vector<Common::RectInfo_S> emptyResults;
            COsdManage::instance()->send_detection_result(0, 0, emptyResults);
            m_bLastSentValid.store(false);
        }

        /* 等待指定间隔后继续下一次合并 */
        std::this_thread::sleep_for(std::chrono::milliseconds(MERGE_INTERVAL_MS));
    }
}

std::vector<Common::RectInfo_S> CResultManager::mergeValidResults(int &outWidth, int &outHeight)
{
    std::lock_guard<std::mutex> lock(m_cacheMutex);

    std::vector<Common::RectInfo_S> allResults;
    outWidth = 0;
    outHeight = 0;

    /* 遍历所有缓存的结果 */
    for (auto typeIt = m_resultCache.begin(); typeIt != m_resultCache.end(); ++typeIt)
    {
        auto& cacheList = typeIt->second;
        /* 遍历该算法类型的所有缓存，移除过期的 */
        for (auto cacheIt = cacheList.begin(); cacheIt != cacheList.end();)
        {
            if (isResultValid(*cacheIt))
            {
                /* 使用最新的分辨率信息 */
                if (outWidth == 0 || outHeight == 0)
                {
                    outWidth = cacheIt->nWidth;
                    outHeight = cacheIt->nHeight;
                }

                /* 添加检测结果到合并数组 */
                for (const auto &rect : cacheIt->vstRectInfo)
                {
                    allResults.push_back(rect);
                }
                ++cacheIt;
            }
            else
            {
                /* 移除过期的结果 */
                cacheIt = cacheList.erase(cacheIt);
            }
        }
    }

    /* 去重重叠的检测框 */
    return removeDuplicateRects(allResults);
}

bool CResultManager::isResultValid(const DetectionCache &cache)
{
    auto now = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - cache.timestamp).count();
    return elapsed <= RESULT_VALID_TIME_MS;
}

std::vector<Common::RectInfo_S> CResultManager::removeDuplicateRects(const std::vector<Common::RectInfo_S> &rects)
{
    if (rects.empty())
    {
        return rects;
    }

    std::vector<Common::RectInfo_S> filteredRects;
    const double OVERLAP_THRESHOLD = 0.7; /* 重叠率阈值 */

    for (size_t i = 0; i < rects.size(); ++i)
    {
        bool isDuplicate = false;

        /* 与已筛选的矩形进行重叠率比较 */
        for (size_t j = 0; j < filteredRects.size(); ++j)
        {
            double overlapRatio = calculateOverlapRatio(rects[i], filteredRects[j]);
            if (overlapRatio > OVERLAP_THRESHOLD)
            {
                isDuplicate = true;
                break;
            }
        }

        /* 如果不重复，添加到结果数组 */
        if (!isDuplicate)
        {
            filteredRects.push_back(rects[i]);
        }
    }

    return filteredRects;
}

double CResultManager::calculateOverlapRatio(const Common::RectInfo_S &rect1, const Common::RectInfo_S &rect2)
{
    /* 计算交集区域的边界 */
    int left = std::max(rect1.nX1, rect2.nX1);
    int top = std::max(rect1.nY1, rect2.nY1);
    int right = std::min(rect1.nX2, rect2.nX2);
    int bottom = std::min(rect1.nY2, rect2.nY2);

    /* 检查是否有交集 */
    if (left >= right || top >= bottom)
    {
        return 0.0; /* 没有重叠 */
    }

    /* 计算交集、并集面积 */
    int intersectArea = (right - left) * (bottom - top);
    int area1 = (rect1.nX2 - rect1.nX1) * (rect1.nY2 - rect1.nY1);
    int area2 = (rect2.nX2 - rect2.nX1) * (rect2.nY2 - rect2.nY1);
    int unionArea = area1 + area2 - intersectArea;

    /* 返回IoU值 */
    return unionArea > 0 ? static_cast<double>(intersectArea) / unionArea : 0.0;
}
