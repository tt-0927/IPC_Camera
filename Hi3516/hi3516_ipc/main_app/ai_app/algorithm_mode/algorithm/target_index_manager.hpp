/**
 * @FilePath     : target_index_manager.hpp
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2025-08-06 14:36:22
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2025-10-16 21:25:39
 * @Description  : 目标索引管理器，用于管理跟踪ID到索引的映射关系
 */

#pragma once

#include <unordered_map>
#include <bitset>
#include <vector>
#include <set>
#include <mutex>
#include <string>
#include "alarm_define.h"

/* 默认最大目标数量 */
#ifndef MAX_TARGET_COUNT
#define MAX_TARGET_COUNT 20
#endif

/*区域检测结构体定义*/
typedef struct _AreaStatus_S_
{
    bool bIsInRegion;  /* 目标是否在区域内 */
    double dEnterTime; /* 目标进入区域的时间戳 */
    bool bAlarmed;     /* 是否已经报警 */

    _AreaStatus_S_() : bIsInRegion(false), dEnterTime(0.0), bAlarmed(false)
    {
    }
} AreaStatus_S;

/* 越界侦测状态结构 */
typedef struct _BoundaryTrackStatus_S_
{
    bool bIsTracking = false;           /* 是否正在跟踪 */
    Common::PosF_S stLastPosition;      /* 上一帧的位置 */
    Common::PosF_S stCurrentPosition;   /* 当前位置 */
    double dLastUpdateTime = 0;         /* 上次更新时间 */
    bool bAlarmed = false;              /* 是否已报警 */
    
    void reset() {
        bIsTracking = false;
        stLastPosition = {0.0f, 0.0f};
        stCurrentPosition = {0.0f, 0.0f};
        dLastUpdateTime = 0;
        bAlarmed = false;
    }
} BoundaryTrackStatus_S;


/**
 * @brief 目标索引管理器模板类
 * @tparam MaxTargets 最大目标数量，默认为20
 */
template <int MaxTargets = MAX_TARGET_COUNT>
class CTargetIndexManager
{
public:
    /**
     * @brief   : 构造函数
     * @param    {string} &name：管理器名称，用于日志输出
     */
    explicit CTargetIndexManager(const std::string &name = "IndexManager") : m_name(name)
    {
        static_assert(MaxTargets > 0, "MaxTargets must be greater than 0");
        reset();
    }

    /**
     * @brief   : 析构函数
     */
    ~CTargetIndexManager() = default;

    /* 禁止拷贝构造和赋值 */
    CTargetIndexManager(const CTargetIndexManager &) = delete;
    CTargetIndexManager &operator=(const CTargetIndexManager &) = delete;

    /**
     * @brief   : 获取或分配索引
     * @param    {int} trackId：跟踪ID
     * @return   {int} 分配的索引，失败返回-1
     */
    int getOrAllocateIndex(int trackId)
    {
        std::lock_guard<std::mutex> lock(m_mutex);

        /* 检查是否已存在映射 */
        auto it = m_mapTrackIdToIndex.find(trackId);
        if (it != m_mapTrackIdToIndex.end())
        {
            return it->second;
        }

        /* 查找可用索引 */
        for (int i = 0; i < MaxTargets; ++i)
        {
            if (!m_indexUsed[i])
            {
                /* 分配索引 */
                m_indexUsed[i] = true;
                m_mapTrackIdToIndex[trackId] = i;
                m_indexToTrackId[i] = trackId;

                return i;
            }
        }

        /* 无可用索引 */
        return -1;
    }

    /**
     * @brief   : 释放指定跟踪ID的索引
     * @param    {int} trackId：跟踪ID
     * @return   {bool} 是否成功释放
     */
    bool releaseIndex(int trackId)
    {
        std::lock_guard<std::mutex> lock(m_mutex);

        auto it = m_mapTrackIdToIndex.find(trackId);
        if (it != m_mapTrackIdToIndex.end())
        {
            int index = it->second;

            /* 释放索引 */
            m_indexUsed[index] = false;
            m_mapTrackIdToIndex.erase(trackId);
            m_indexToTrackId.erase(index);

            return true;
        }

        return false;
    }

    /**
     * @brief   : 根据索引获取跟踪ID
     * @param    {int} index：索引
     * @return   {int} 跟踪ID，未找到返回-1
     */
    int getTrackIdByIndex(int index) const
    {
        std::lock_guard<std::mutex> lock(m_mutex);

        if (index >= 0 && index < MaxTargets)
        {
            auto it = m_indexToTrackId.find(index);
            if (it != m_indexToTrackId.end())
            {
                return it->second;
            }
        }

        return -1;
    }

    /**
     * @brief   : 根据跟踪ID获取索引
     * @param    {int} trackId：跟踪ID
     * @return   {int}索引，未找到返回-1
     */
    int getIndexByTrackId(int trackId) const
    {
        std::lock_guard<std::mutex> lock(m_mutex);

        auto it = m_mapTrackIdToIndex.find(trackId);
        if (it != m_mapTrackIdToIndex.end())
        {
            return it->second;
        }

        return -1;
    }

    /**
     * @brief   : 检查索引是否被使用
     * @param    {int} index：索引
     * @return   {bool} 
     */
    bool isIndexUsed(int index) const
    {
        std::lock_guard<std::mutex> lock(m_mutex);

        if (index >= 0 && index < MaxTargets)
        {
            return m_indexUsed[index];
        }

        return false;
    }

    /**
     * @brief   : 检查跟踪ID是否存在 
     * @param    {int} trackId：跟踪ID
     * @return   {bool} 是否存在
     */
    bool hasTrackId(int trackId) const
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_mapTrackIdToIndex.find(trackId) != m_mapTrackIdToIndex.end();
    }

    /**
     * @brief   : 清理丢失的目标
     * @param    {set<int>} &currentTrackIds：当前帧存在的跟踪ID集合
     * @return   {int} 被清理的跟踪ID数量
     */
    int cleanupLostTargets(const std::set<int> &currentTrackIds)
    {
        std::lock_guard<std::mutex> lock(m_mutex);

        std::vector<int> lostTrackIds;

        /* 找出丢失的跟踪ID */
        for (const auto &pair : m_mapTrackIdToIndex)
        {
            if (currentTrackIds.find(pair.first) == currentTrackIds.end())
            {
                lostTrackIds.push_back(pair.first);
            }
        }

        /* 释放丢失的跟踪ID对应的索引 */
        for (int trackId : lostTrackIds)
        {
            int index = m_mapTrackIdToIndex[trackId];
            m_indexUsed[index] = false;
            m_mapTrackIdToIndex.erase(trackId);
            m_indexToTrackId.erase(index);
        }

        return static_cast<int>(lostTrackIds.size());
    }

    /**
     * @brief   : 重置管理器，清空所有映射
     */
    void reset()
    {
        std::lock_guard<std::mutex> lock(m_mutex);

        m_mapTrackIdToIndex.clear();
        m_indexToTrackId.clear();
        m_indexUsed.reset();
    }

    /**
     * @brief   : 获取当前使用的索引数量
     * @return   {int} 使用的索引数量
     */
    int getUsedCount() const
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        return static_cast<int>(m_indexUsed.count());
    }

    /**
     * @brief   : 使用的索引数量
     * @return   {int} 使用的索引数量 
     */
    constexpr int getMaxTargets() const
    {
        return MaxTargets;
    }

    /**
     * @brief   : 获取所有当前的跟踪ID
     * @return   {std::set<int>} 跟踪ID集合
     */
    std::set<int> getCurrentTrackIds() const
    {
        std::lock_guard<std::mutex> lock(m_mutex);

        std::set<int> trackIds;
        for (const auto &pair : m_mapTrackIdToIndex)
        {
            trackIds.insert(pair.first);
        }

        return trackIds;
    }

    /**
     * @brief   : 获取所有当前使用的索引
     * @return   {std::vector<int>} 索引集合
     */
    std::vector<int> getCurrentIndices() const
    {
        std::lock_guard<std::mutex> lock(m_mutex);

        std::vector<int> indices;
        for (int i = 0; i < MaxTargets; ++i)
        {
            if (m_indexUsed[i])
            {
                indices.push_back(i);
            }
        }

        return indices;
    }

    /**
     * @brief   : 获取管理器名称 
     * @return   {std::string} 管理器名称
     */
    const std::string &getName() const
    {
        return m_name;
    }

private:
    /* 管理器名称 */
    std::string m_name;

    /* 跟踪ID到索引的映射 */
    std::unordered_map<int, int> m_mapTrackIdToIndex;

    /* 索引到跟踪ID的映射 */
    std::unordered_map<int, int> m_indexToTrackId;

    /* 索引使用状态 */
    std::bitset<MaxTargets> m_indexUsed;

    /* 线程安全保护 */
    mutable std::mutex m_mutex;
};

/* 常用的类型别名 */
using CTargetIndexManager20 = CTargetIndexManager<20>;
using CTargetIndexManager50 = CTargetIndexManager<50>;
using CTargetIndexManager80 = CTargetIndexManager<80>;
using CTargetIndexManager100 = CTargetIndexManager<100>;
