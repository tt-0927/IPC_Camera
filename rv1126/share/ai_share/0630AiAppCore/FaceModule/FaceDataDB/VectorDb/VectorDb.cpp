#include "VectorDb.hpp"

#include "dlog.h"

using namespace Modules_NS;

#define DIMENSION 512

using namespace Ai0630_NS;

VectorDb::VectorDb(std::string strDbPath)
    : m_strDbPath(strDbPath)
{
}

void VectorDb::load()
{
    std::unique_lock<std::mutex> lock(m_FvMutex);

    if (fopen(m_strDbPath.c_str(), "r"))
    {
        m_FaceFaissDb.load(m_strDbPath.data());
        std::cout << "重新加载数据库后的数量: " << m_FaceFaissDb.getSize() << std::endl;
    }
    else
    {
        m_FaceFaissDb.createDb(DIMENSION, IndexFlatIP);
        dlog(LOG_DEBUG, "%s 不存在, 创建数据库", m_strDbPath.c_str());
    }
}

void VectorDb::save()
{
    std::unique_lock<std::mutex> lock(m_FvMutex);

    m_FaceFaissDb.save(m_strDbPath.data());
}

void VectorDb::addFaceVector(int id, const std::vector<float>& vector)
{
    std::unique_lock<std::mutex> lock(m_FvMutex);

    std::vector<float> vectorData(DIMENSION);
    vectorData = vector;

    m_FaceFaissDb.normalize(vectorData);
    m_FaceFaissDb.add(id, vectorData);

    std::cout << "当前数据库的向量数量: " << m_FaceFaissDb.getSize() << std::endl;
}

void VectorDb::removeFaceVector(int id)
{
    std::unique_lock<std::mutex> lock(m_FvMutex);

    m_FaceFaissDb.remove(id);

    std::cout << "删除所有向量后的数据库的数量: " << m_FaceFaissDb.getSize() << std::endl;
}

bool VectorDb::searchNormalize(const std::vector<float>& vector, SearchResult_S& stSearchRes, int nNum)
{
    std::unique_lock<std::mutex> lock(m_FvMutex);

    /* 查询相似人脸 */
    std::vector<float> query_vector(DIMENSION);
    query_vector = vector;

    m_FaceFaissDb.normalize(query_vector);

    int nSize = 0;
    if (nNum > 0)
    {
        nSize = nNum;
    }
    else
    {
        nSize = (m_FaceFaissDb.getSize() / 2 > 0) ? m_FaceFaissDb.getSize() / 2 : 1;
    }

    return m_FaceFaissDb.search(query_vector, stSearchRes, nSize);
}

void VectorDb::clear()
{
    std::unique_lock<std::mutex> lock(m_FvMutex);

    // 清空数据库
    m_FaceFaissDb.clear();

    std::cout << "清空数据库后的数量: " << m_FaceFaissDb.getSize() << std::endl;
}
