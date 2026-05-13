/**
 * @file FaissDatabase.hpp
 * @author wucp (wucp@kfb.cn)
 * @date 2024-10-14
 *
 * @brief
 */
#pragma once
#include <faiss/IndexFlat.h>

#include <faiss/index_io.h>
#include <faiss/IndexIDMap.h>
#include <iostream>
#include <vector>
#include <cmath>

namespace Modules_NS
{
/**
 * @brief Faiss 特征搜索返回的结果
 */
typedef struct _SEARCHRESULT_
{
    std::vector<float> vDistances; /* 搜索到的两个向量之间的距离（相似度、欧式距离等） */
    std::vector<int64_t> vIndices; /* 搜索到的下标 */
} SearchResult_S;

/**
 * @brief 构建Faiss索引对应的存储方式
 */
typedef enum _INDEXTYPE_
{
    IndexFlatIP = 0, /* 特殊的 IndexFlat，它使用内积（Inner Product）作为距离度量。*/
    IndexFlat,       /* 基本的索引类型，它直接存储原始向量数据。*/
    IndexFlatL2,     /* 专门用于计算 L2 距离（欧几里得距离）。*/
    IndexFlat1D,     /* 针对一维向量优化的索引类型。*/
} IndexType_E;

class CFaceDatabase
{
public:
    CFaceDatabase();
    ~CFaceDatabase();

    /**
     * @brief 创建Faiss索引
     * @param nDimension 向量的维度
     * @param eIndexType 向量存储方式（默认为IndexFlatIP）
     * @return true
     * @return false
     */
    bool createDb(int nDimension, IndexType_E eIndexType = IndexFlatIP);

    /**
     * @brief 加载Faiss索引
     * @param FaissPath Faiss索引的地址
     * @return true
     * @return false
     */
    bool load(char *FaissPath);

    /**
     * @brief 保存aiss索引
     * @param FaissPath Faiss索引的地址
     * @return true
     * @return false
     */
    bool save(char *FaissPath);

    /**
     * @brief 添加数据--自定义ID
     * @param nId 向量索引对应的ID
     * @param vFeature 一维向量
     * @return true
     * @return false
     */
    bool add(int64_t nId, const std::vector<float> &vFeature);

    /**
     * @brief 批量添加数据--自定义ID
     * @param vIds 向量索引对应的ID容器
     * @param vFeature 向量容器（长度=单个向量长度*ID个数）
     * @return true
     * @return false
     */
    bool add(std::vector<int64_t> vIds, const std::vector<float> &vFeature);

    /**
     * @brief 删除数据--自定义ID
     * @param nId 向量索引对应的ID
     * @return true
     * @return false
     */
    bool remove(int64_t nId);

    /**
     * @brief 批量删除数据--自定义ID
     * @param vIds 向量索引对应的ID
     * @return true
     * @return false
     */
    bool remove(std::vector<int64_t> vIds);

    /**
     * @brief 查询相似向量ID，输入的向量，最高的相似度和对应ID
     * @param vFeature 需要查询的向量
     * @param stSearchRes 查询返回的结果
     * @param nNum 查询相似的个数（查询全部：nNum=pIndex->ntotal)
     * @return true
     * @return false
     */
    bool search(std::vector<float> &vFeature, SearchResult_S &stSearchRes, int nNum = 1, int nPercent = 100);

    /**
     * @brief 向量归一化
     * @param vFeature 向量容器
     */
    void normalize(std::vector<float> &vFeature);

    /**
     * @brief 返回数据库中的人脸向量数量
     * @return size_t
     */
    size_t getSize();

    /**
     * @brief 获取索引库中所有的IDs
     * @param vIds 全部的数据库索引ID
     */
    void getIndexIDs(std::vector<int64_t>& vIds);

    /**
     * @brief 获取索引库中最小的ID
     * @return 数据库索引最小的ID
     */
    int64_t getMinID();

    /**
     * @brief 清空数据库
     */
    void clear();

private:
    faiss::IndexIDMap *pIndex = nullptr; /* Faiss 的索引 */
};
}