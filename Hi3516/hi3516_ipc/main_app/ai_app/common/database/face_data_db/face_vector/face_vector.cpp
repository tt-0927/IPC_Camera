/**
 * @FilePath     : face_vector.cpp
 * @Author       : zhouzirui
 * @Date         : 2025-06-06 16:02:10
 * @LastEditors  : zhouzirui
 * @LastEditTime : 2025-06-06 16:53:56
 * @Description  : 
 */

#include "face_vector.h"
#include <iostream>
#include <cmath>
#include <cstring>
#include <queue>
#include <algorithm>

CFaceVector::CFaceVector(const std::string &dbPath)
{
    openDB(dbPath);
}

CFaceVector::~CFaceVector()
{
    closeDB();
}

// 打开数据库
bool CFaceVector::openDB(const std::string &path)
{
    if (sqlite3_open(path.c_str(), &db) != SQLITE_OK)
    {
        std::cerr << "打开数据库失败: " << sqlite3_errmsg(db) << std::endl;
        db = nullptr;
        return false;
    }
    return true;
}

// 关闭数据库
void CFaceVector::closeDB()
{
    if (db)
    {
        sqlite3_close(db);
        db = nullptr;
    }
}

// 获取所有用户自建表名（排除sqlite系统表）
std::vector<std::string> CFaceVector::getAllTables()
{
    std::vector<std::string> tables;
    const char *sql = "SELECT name FROM sqlite_master WHERE type='table'";
    sqlite3_stmt *stmt;

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) == SQLITE_OK)
    {
        while (sqlite3_step(stmt) == SQLITE_ROW)
        {
            std::string table = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 0));
            if (table != "sqlite_sequence")
            {
                tables.push_back(table);
            }
        }
    }
    sqlite3_finalize(stmt);
    return tables;
}

// 将二进制Blob数据转换为float向量
std::vector<float> CFaceVector::parseBlobToVector(const void *data, int size)
{
    std::vector<float> vec;
    if (size != 128 * sizeof(float))
        return vec;

    vec.resize(128);
    std::memcpy(vec.data(), data, size);
    return vec;
}

// 计算两个128维向量之间的余弦相似度
float CFaceVector::cosineSimilarity(const std::vector<float> &a, const std::vector<float> &b)
{
    float dot = 0.0f, normA = 0.0f, normB = 0.0f;
    for (size_t i = 0; i < a.size(); ++i)
    {
        dot += a[i] * b[i];
        normA += a[i] * a[i];
        normB += b[i] * b[i];
    }
    return dot / (std::sqrt(normA) * std::sqrt(normB) + 1e-6f);
}

// 在指定表中查找最相似的记录
FaceInfo CFaceVector::matchFromTable(const std::string &tableName, const std::vector<float> &targetVector)
{
    std::string sql = "SELECT ID, Name, Data FROM " + tableName;
    sqlite3_stmt *stmt;
    FaceInfo bestMatch = {tableName, -1, "", -1.0f};

    if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr) == SQLITE_OK)
    {
        while (sqlite3_step(stmt) == SQLITE_ROW)
        {
            int id = sqlite3_column_int(stmt, 0);
            const char *name = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 1));
            const void *blob = sqlite3_column_blob(stmt, 2);
            int blobSize = sqlite3_column_bytes(stmt, 2);

            std::vector<float> dbVector = parseBlobToVector(blob, blobSize);
            if (dbVector.size() == 128)
            {
                float sim = cosineSimilarity(targetVector, dbVector);
                if (sim > bestMatch.similarity)
                {
                    bestMatch = {tableName, id, name ? name : "", sim};
                }
            }
        }
    }
    sqlite3_finalize(stmt);
    return bestMatch;
}

// 从所有表中比对，找出最相似的人脸记录
FaceInfo CFaceVector::matchFromAllTables(const std::vector<float> &targetVector)
{
    std::vector<std::string> tables = getAllTables();
    FaceInfo bestMatch = {"", -1, "", -1.0f};
    for (const auto &table : tables)
    {
        FaceInfo match = matchFromTable(table, targetVector);
        if (match.similarity > bestMatch.similarity)
        {
            bestMatch = match;
        }
    }
    return bestMatch;
}

// 在指定表中获取 Top-K 相似记录
std::vector<FaceInfo> CFaceVector::matchTopKFromTable(const std::string &tableName, const std::vector<float> &targetVector, int K)
{
    std::priority_queue<FaceInfo> pq;
    std::string sql = "SELECT ID, Name, Data FROM " + tableName;
    sqlite3_stmt *stmt;

    if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr) == SQLITE_OK)
    {
        while (sqlite3_step(stmt) == SQLITE_ROW)
        {
            int id = sqlite3_column_int(stmt, 0);
            const char *name = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 1));
            const void *blob = sqlite3_column_blob(stmt, 2);
            int blobSize = sqlite3_column_bytes(stmt, 2);

            std::vector<float> dbVector = parseBlobToVector(blob, blobSize);
            if (dbVector.size() == 128)
            {
                float sim = cosineSimilarity(targetVector, dbVector);
                FaceInfo info = {tableName, id, name ? name : "", sim};
                pq.push(info);
                if ((int)pq.size() > K)
                {
                    pq.pop(); // 保持优先队列中最多K个元素
                }
            }
        }
    }
    sqlite3_finalize(stmt);

    // 将结果转为 vector 并按相似度降序排序
    std::vector<FaceInfo> results;
    while (!pq.empty())
    {
        results.push_back(pq.top());
        pq.pop();
    }
    std::reverse(results.begin(), results.end());
    return results;
}

// 从所有表中获取 Top-K 相似记录
std::vector<FaceInfo> CFaceVector::matchTopKFromAllTables(const std::vector<float> &targetVector, int K)
{
    std::priority_queue<FaceInfo> pq;
    std::vector<std::string> tables = getAllTables();

    for (const auto &table : tables)
    {
        std::vector<FaceInfo> partial = matchTopKFromTable(table, targetVector, K);
        for (const auto &info : partial)
        {
            pq.push(info);
            if ((int)pq.size() > K)
            {
                pq.pop();
            }
        }
    }

    std::vector<FaceInfo> results;
    while (!pq.empty())
    {
        results.push_back(pq.top());
        pq.pop();
    }
    std::reverse(results.begin(), results.end());
    return results;
}
