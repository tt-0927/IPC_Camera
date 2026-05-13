
#pragma once

#include <fstream>
#include <iostream>
#include <mutex>
#include <thread>
#include <unordered_map>
#include <vector>

#include "FaissDatabase.hpp"

#define FACE_VECTOR_DB_PATH  "/opt/bl/db/FaceFaissDb.index"
#define HUMAN_VECTOR_DB_PATH "/opt/bl/db/HumanFaissDb.index"

namespace Ai0630_NS
{
    class VectorDb
    {
    public:

        VectorDb(std::string strDbPath);
        /**
         * @brief 将数据库保存到文件中
         */
        void save();


        /**
         * @brief 从文件中加载数据库
         */
        void load();


        /**
         * @brief 添加特征向量到数据库
         * @param id
         * @param vector
         */
        void addFaceVector(int id, const std::vector<float>& vector);

        /**
         * @brief 搜索最相似向量
         */
        bool searchNormalize(const std::vector<float>& vector, Modules_NS::SearchResult_S& stSearchRes, int nNum = 0);


        /**
         * @brief 移除某个向量
         * @param id
         */
        void removeFaceVector(int id);


        /**
         * @brief 清空向量数据
         */
        void clear();

    private:

        Modules_NS::CFaceDatabase m_FaceFaissDb;

        std::string m_strDbPath;

        /* 互斥锁，限制数据库操作 */
        std::mutex m_FvMutex;
    };
}    // namespace Ai0630_NS
