/**
 * @file FaissDatabase.cpp
 * @author wucp (wucp@kfb.cn)
 * @date 2024-10-14
 *
 * @brief
 */
#include "FaissDatabase.hpp"
#include <iostream>
#include <fstream>
#include <algorithm>

namespace Modules_NS
{
CFaceDatabase::CFaceDatabase()
{
}
CFaceDatabase::~CFaceDatabase()
{
    if (pIndex)
    {
        delete pIndex;
        pIndex = nullptr;
    }
}

/* 创建索引 */
bool CFaceDatabase::createDb(int nDimension, IndexType_E eIndexType)
{
    if (pIndex)
    {
        printf("已存在Faiss索引\n");
        return false;
    }
    /* 选择创建哪种Faiss存储方式 */
    switch (eIndexType)
    {
    case IndexFlatIP:
        pIndex = new faiss::IndexIDMap(new faiss::IndexFlatIP(nDimension));
        break;
    case IndexFlat:
        pIndex = new faiss::IndexIDMap(new faiss::IndexFlat(nDimension));
        break;
    case IndexFlatL2:
        pIndex = new faiss::IndexIDMap(new faiss::IndexFlatL2(nDimension));
        break;
    case IndexFlat1D:
        pIndex = new faiss::IndexIDMap(new faiss::IndexFlat1D(nDimension));
        break;
    default:
        std::cerr << "Unknown index type" << std::endl;
        return false;
    }

    return true;
}

/* 加载Faiss索引 */
bool CFaceDatabase::load(char *FaissPath)
{
    /* 使用faiss::read_index函数读取索引 */
    pIndex = dynamic_cast<faiss::IndexIDMap *>(faiss::read_index(FaissPath));
    return true;
}

/* 保存aiss索引 */
bool CFaceDatabase::save(char *FaissPath)
{
    faiss::write_index(pIndex, FaissPath);
    return true;
}

/* 添加数据--自定义ID */
bool CFaceDatabase::add(int64_t nId, const std::vector<float> &vFeature)
{
    if (vFeature.size() != pIndex->d)
    {
        printf("输入向量的维度[%ld] != 数据库向量的维度[%d]\n", vFeature.size(), pIndex->d);
        return false;
    }
    /* 转换为矩阵 */
    std::vector<int64_t> vIds = {nId};
    pIndex->add_with_ids(1, vFeature.data(), vIds.data());
    return true;
}

/* 批量添加数据--自定义ID */
bool CFaceDatabase::add(std::vector<int64_t> vIds, const std::vector<float> &vFeature)
{
    if (vFeature.size() % pIndex->d != 0)
    {
        printf("输入向量的维度[%ld] 数据库向量的维度[%d] !=0\n", vFeature.size(), pIndex->d);
        return false;
    }
    /* 转换为矩阵 */
    pIndex->add_with_ids(vIds.size(), vFeature.data(), vIds.data());
    return true;
}

/* 删除数据--自定义ID */
bool CFaceDatabase::remove(int64_t nId)
{
    std::vector<int64_t> vIds = {nId};
    pIndex->remove_ids(faiss::IDSelectorArray(1, vIds.data()));
    return true;
}
/* 批量删除数据--自定义ID */
bool CFaceDatabase::remove(std::vector<int64_t> vIds)
{
    pIndex->remove_ids(faiss::IDSelectorArray(vIds.size(), vIds.data()));
    return true;
}

/* 查询相似向量ID，输入的向量，最高的相似度和对应ID */
bool CFaceDatabase::search(std::vector<float> &vFeature, SearchResult_S &stSearchRes, int nNum, int nPercent)
{
    printf("查询相似向量ID: 数据库个数[%ld] 需要输出的个数[%d] 搜索范围[%d]\n", pIndex->ntotal, nNum, nPercent);
    
    if (nNum > pIndex->ntotal)
    {
        printf("需要输出的个数nNum[%d] 超过数据库个数[%ld]\n", nNum, pIndex->ntotal);
        return false;
    }
    if (vFeature.size() % pIndex->d != 0)
    {
        printf("输入向量的维度[%ld] 数据库向量的维度[%d] !=0\n", vFeature.size(), pIndex->d);
        return false;
    }
    if (nPercent < 1 || nPercent > 100)
    {
        printf("输入向量的搜索范围错误[%d]\n", nPercent);
        return false;
    }
    
    int nFeatureNum = (vFeature.size() / pIndex->d);
    /* 查询 */
    stSearchRes.vDistances.resize(nFeatureNum * nNum);
    stSearchRes.vIndices.resize(nFeatureNum * nNum);
    
    if (nPercent == 100)
    {
        pIndex->search(nFeatureNum, vFeature.data(), nNum, stSearchRes.vDistances.data(), stSearchRes.vIndices.data());
    }
    else
    {
        /* 计算实际要搜索的向量数量 */
        size_t nTotalVectors = pIndex->ntotal;
        size_t nSearchTo = nTotalVectors;
        size_t nSearchFrom = 0;
        if (nPercent < 100) {
            nSearchFrom = nTotalVectors * (100 - nPercent) / 100;
            /* 确保至少有1个向量被搜索 */
            if (nSearchFrom >= nTotalVectors) {
                nSearchFrom = nTotalVectors > 0 ? nTotalVectors - 1 : 0;
            }
        }

        faiss::IDSelectorRange selector(nSearchFrom, nSearchTo);
        faiss::SearchParameters params;
        params.sel = &selector;

        pIndex->search(nFeatureNum, vFeature.data(), nNum, stSearchRes.vDistances.data(), stSearchRes.vIndices.data(), &params);
    }

    if (stSearchRes.vDistances.size() != stSearchRes.vIndices.size())
    {
        printf("查询失败,输出ID个数[%ld]!=距离容器个数[%ld]\n", stSearchRes.vDistances.size(), stSearchRes.vIndices.size());
        return false;
    }

    return true;
}

/* 向量归一化 */
void CFaceDatabase::normalize(std::vector<float> &vFeature)
{
    float fNorm = 0.0f;
    /* 计算向量的范数（L2 范数） */
    for (float fFeature : vFeature)
    {
        fNorm += fFeature * fFeature;
    }
    fNorm = std::sqrt(fNorm);

    /* 如果向量为很小的数，不做处理 */
    if (fNorm > 1e-10)
    {
        for (size_t i = 0; i < vFeature.size(); ++i)
        {
            vFeature[i] /= fNorm;
        }
    }
}

/* 返回数据库中的人脸向量数量 */
size_t CFaceDatabase::getSize()
{
    return pIndex->ntotal;
}

/* 获取索引库中所有的IDs */
void CFaceDatabase::getIndexIDs(std::vector<int64_t> &vIds)
{
    // 定义一个引用来引用 pIndex 的 id_map 成员
    const std::vector<int64_t> &vIdMapRef = pIndex->id_map;
    vIds = vIdMapRef;
}

/* 获取索引库中最小的ID */
int64_t CFaceDatabase::getMinID()
{
    if (pIndex == nullptr || pIndex->id_map.empty())
    {
        return -1;
    }
    const auto& idMap = pIndex->id_map;
    return *std::min_element(idMap.begin(), idMap.end());
}

/* 清空数据库 */
void CFaceDatabase::clear()
{
    pIndex->reset();
}
}