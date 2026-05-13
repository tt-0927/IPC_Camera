/**
 * @FilePath     : face_vector.h
 * @Author       : zhouzirui
 * @Date         : 2025-06-06 16:02:10
 * @LastEditors  : zhouzirui
 * @LastEditTime : 2025-06-06 16:53:12
 * @Description  : 
 */

#pragma once

#include <string>
#include <vector>
#include "sqlite3.h"

#define FACE_SQLITE_DB_PATH "/userdata/cam/db/FaceSqlite.db"

// 人脸信息结构体，用于存储数据库中提取的记录
struct FaceInfo
{
    std::string tableName; // 表名
    int id;                // 记录ID
    std::string name;      // 姓名
    float similarity;      // 相似度（与目标向量的余弦相似度）

    // 用于优先队列中比较（按相似度升序，越小优先被替换）
    bool operator<(const FaceInfo &other) const
    {
        return similarity > other.similarity; // 适配 min-heap（小在前）
    }
};

class CFaceVector
{
public:
    // 构造函数：传入SQLite数据库路径
    CFaceVector(const std::string &dbPath);
    ~CFaceVector();

    // 单表比对：获取与目标向量最相似的一条记录
    FaceInfo matchFromTable(const std::string &tableName, const std::vector<float> &targetVector);

    // 多表比对：从所有表中找最相似的一条记录
    FaceInfo matchFromAllTables(const std::vector<float> &targetVector);

    // 扩展函数：从指定表中获取Top-K相似记录
    std::vector<FaceInfo> matchTopKFromTable(const std::string &tableName, const std::vector<float> &targetVector, int K);

    // 扩展函数：从所有表中获取Top-K相似记录
    std::vector<FaceInfo> matchTopKFromAllTables(const std::vector<float> &targetVector, int K);

private:
    sqlite3 *db;

    bool openDB(const std::string &path);
    void closeDB();
    std::vector<std::string> getAllTables();
    float cosineSimilarity(const std::vector<float> &a, const std::vector<float> &b);
    std::vector<float> parseBlobToVector(const void *data, int size);
};
