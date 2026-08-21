/**
 * @FilePath     : face_manage.h
 * @Author       : zhouzirui
 * @Date         : 2025-06-06 16:02:10
 * @LastEditors  : zhouzirui
 * @LastEditTime : 2025-06-06 16:59:27
 * @Description  : 人员数据库管理：提供数据库操作接口，每操作一次人员数据库，需对向量数据库进行同步执行，以确保向量数据的同步
 */

#pragma once

#include <cmath>
#include <vector>
#include <cstring>
#include "Singleton.h"

#include "event_define.h"

#include "snap_sqlite.hpp"
#include "face_sqlite.hpp"
#include "face_vector.h"
// #include "FaceVectorDb.hpp"
#include "common_process.h"

/* 人脸比对相似度阈值 */
#define FACE_SIMILARITY_THRESHOLD 0.7  

using namespace FaceDataDB_NS;

namespace FaceManage
{
    class AIFaceManage : public CSingleton<AIFaceManage>
    {
        AIFaceManage();
    public:

        ~AIFaceManage();
        friend class CSingleton<AIFaceManage>;

        /**
         * @brief 新增抓拍人脸信息
         * @param stInfo 
         * @return int 
         */
        int addSnapFace(SnapFaceInfo_S stInfo, int& nFaceLibId, float &fSimilarity);

        /**
         * @brief 删除抓拍人脸信息
         * @param nId 
         * @return int 
         */
        int delSnapFace(int nId);

        /**
         * @brief 更新抓拍人脸信息
         * @param nId 
         * @param stInfo 
         * @return int 
         */
        int updateSnapFace(int nId, SnapFaceInfo_S stInfo);

        /**
         * @brief 新增人脸名单库
         * @param stInfo 
         * @return int 
         */
        int addFaceLibInfo(FaceLibsInfo_S &stInfo);
        
        /**
         * @brief 删除人脸名单库
         * @param nId 
         * @return int 
         */
        int delFaceLibInfo(int nId);
        

        /* 清空所有名单组中的人员和人脸特征 */
        int clearFaceLibData();

        /**
         * @brief 删除人脸目录中未被人脸数据库引用的图片和特征文件
         * @return OK：完成，其他：数据库读取或目录清理失败
         */
        int cleanupOrphanFaceFiles();

        /**
         * @brief 更新人脸名单库
         * @param nId 
         * @param stInfo 
         * @return int 
         */
        int updateFaceLibInfo(int nId, FaceLibsInfo_S stInfo);

        /**
         * @brief 通过ID查找人脸信息
         * @param nId 
         * @param stInfo 
         * @return int 
         */
        int searchFaceInfoById(int nId, FaceLibsInfo_S &stInfo);
        int searchFaceInfoById(int nId, SnapFaceInfo_S &stInfo);

        /**
         * @brief 全局向量库比对
         * @param vfData 输入比对人脸信息
         * @param vIndices 输出比对结果人脸信息
         */
        bool comparisonNormalize(const std::vector<float>& vfData, std::vector<int64_t>& vIndices, std::vector<float>& vfSimilarity);

        /**
         * @brief 本地名单库向量比对
         * @param vfData 
         * @param nFaceLibId 
         */
        bool comparisonFaceLib(const std::vector<float>& vfData, int& nFaceLibId, float &fSimilarity);
        bool comparisonFaceLib(const std::vector<float>& vfData, FaceLibsInfo_S &stMatchedInfo, float &fSimilarity);

        /**
         * @brief 余弦相似度计算
         * @return float 
         */
        float cosine_similarity(const std::vector<float>& A, const std::vector<float>& B);

        /* 增加名单组表 */
        int creatFaceTable(std::string strTabName);
        
        /* 删除名单组表 */
        int deleteFaceTable(std::string strTabName);
        
        /* 修改名单组表名 */
        int renameFaceTable(std::string oldTabName, std::string newTabName);
        
        /* 获取名单组信息 */
        int getTableReport(std::vector<Event::FaceLibInfo_S>& listTableReport);
        
        /* 根据表名查找人脸信息 */
        int searchFaceInfoByTable(std::string strTabName, std::list<FaceLibsInfo_S>& listOutInfo);

        /*  根据条件查询人脸信息 */
        int searchFaceInfoByCond(Event::FaceFind_S stFaceFind, std::list<FaceLibsInfo_S>& listOutInfo);

    private:
        
        /* 抓拍数据库 */
        CSnapSqlite m_SnapInfodb;

        /* 人脸数据库 */
        CFaceSqlite m_FaceInfodb;
        
        /* 向量数据库 */
        CFaceVector m_FaceVectordb;
        // FaceVectorDb m_FaceVectordb;
    };
}
