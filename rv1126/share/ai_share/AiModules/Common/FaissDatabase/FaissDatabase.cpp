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
bool CFaceDatabase::search(std::vector<float> &vFeature, SearchResult_S &stSearchRes, int nNum, int nSearchFrom, int nSearchTo)
{
    printf("查询相似向量ID: 数据库个数[%ld] 需要输出的个数[%d] 搜索范围[%d - %d]\n", pIndex->ntotal, nNum, nSearchFrom, nSearchTo);
    
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
    
    int nFeatureNum = (vFeature.size() / pIndex->d);
    /* 查询 */
    stSearchRes.vDistances.resize(nFeatureNum * nNum);
    stSearchRes.vIndices.resize(nFeatureNum * nNum);
    
    if (nSearchFrom == -1 || nSearchTo == -1)
    {
        pIndex->search(nFeatureNum, vFeature.data(), nNum, stSearchRes.vDistances.data(), stSearchRes.vIndices.data());
    }
    else
    {
        /* 计算实际要搜索的向量数量 */
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

/* 在指定 ID 列表中查询相似向量 */
bool CFaceDatabase::searchWithIds(std::vector<float> &vFeature, SearchResult_S &stSearchRes, const std::vector<int64_t> &vIds, int nNum)
{
    printf("在指定ID列表中查询相似向量: 数据库个数[%ld] 指定ID个数[%ld] 需要输出的个数[%d]\n", pIndex->ntotal, vIds.size(), nNum);
    
    if (vIds.empty())
    {
        printf("指定的ID列表为空\n");
        return false;
    }
    
    if (vFeature.size() % pIndex->d != 0)
    {
        printf("输入向量的维度[%ld] 数据库向量的维度[%d] !=0\n", vFeature.size(), pIndex->d);
        return false;
    }
    
    int nFeatureNum = (vFeature.size() / pIndex->d);
    
    // 限制 nNum 不超过指定 ID 列表的大小
    if (nNum > (int)vIds.size())
    {
        nNum = (int)vIds.size();
    }
    
    // 查询
    stSearchRes.vDistances.resize(nFeatureNum * nNum);
    stSearchRes.vIndices.resize(nFeatureNum * nNum);
    
    // 使用 IDSelectorArray 指定要搜索的 ID
    faiss::IDSelectorArray selector(vIds.size(), vIds.data());
    faiss::SearchParameters params;
    params.sel = &selector;
    
    pIndex->search(nFeatureNum, vFeature.data(), nNum, stSearchRes.vDistances.data(), stSearchRes.vIndices.data(), &params);
    
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