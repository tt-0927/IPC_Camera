/**
 * @file FaceManage.hpp
 * @author xiejh (xiejh@kfb.cn)
 * @date 2024-10-18
 *
 * @brief 人员数据库管理：提供数据库操作接口，每操作一次人员数据库，需对向量数据库进行同步执行，以确保向量数据的同步
 */
#pragma once

#include <cmath>
#include <cstring>
#include <vector>

#include "0630AppExtern.hpp"
#include "FaceSqlite.hpp"
#include "HumanSqlite.hpp"
#include "Intern.hpp"
#include "SignalSlot.h"
#include "VectorDb.hpp"

/* 人脸比对相似度阈值 */
#define FACE_SIMILARITY_THRESHOLD  0.4
#define HUMAN_SIMILARITY_THRESHOLD 0.65

namespace Ai0630_NS
{
    class FaceManage
    {
    public:

        FaceManage();
        ~FaceManage();

        template<typename CONTEXT, typename... Args>
        bool bindSlot(
            CONTEXT*                          context,
            signal_function<CONTEXT, Args...> slot)
        {
            /* 关联信号与槽 */
            connect(&sig_sendData,
                    context,
                    slot,
                    false);

            return true;
        }

        /**
         * @brief 解绑定信号
         * @return true
         * @return false
         */
        bool unbindSig()
        {
            /* 关联信号与槽 */
            disconnect(&sig_sendData);
            return true;
        }

        /**
         * @brief 接受个人识别数据
         * @param stHeader
         * @param stUserHeader
         * @param stFaceResult
         */
        void recvData(HeaderInfo_S     stHeader,
                      UserHeaderInfo_S stUserHeader,
                      FaceResult_S     stFaceResult);

        /**
         * @brief 接受人脸特征数据
         * @param stHeader
         * @param stFaceLibsInfo
         * @param stFaceResult
         */
        void recvFaceFeature(HeaderInfo_S   stHeader,
                             FaceLibsInfo_S stFaceLibsInfo,
                             FaceResult_S   stFaceResult);

        /**
         * @brief 删除人脸数据表
         * @param strTabName
         * @return int
         */
        int deleteFaceTable(std::string strTabName = std::string());

        /**
         * @brief 删除人类数据表
         * @param strTabName
         * @return int
         */
        int deleteHumanTable(std::string strTabName = std::string());

        /**
         * @brief 根据表名查找人脸信息
         * @param strTabName
         * @param listOutInfo
         * @return int
         */
        int searchFaceInfoByTable(std::string strTabName, std::list<FaceLibsInfo_S>& listOutInfo);


    private:


        /**
         * @brief 新增人脸库
         * @param stInfo
         * @return int
         */
        int addFaceLibInfo(FaceLibsInfo_S& stInfo);

        /**
         * @brief 新增人类信息库
         * @param stInfo
         * @return int
         */
        int addHumanLibInfo(HumanLibsInfo_S& stInfo);

        /**
         * @brief 删除人脸库
         * @param nId
         * @return int
         */
        int delFaceLibInfo(int nId);

        /**
         * @brief 删除人类信息库
         * @param nId
         * @return int
         */
        int delHumanLibInfo(int nId);

        /**
         * @brief 根据表名查找人类信息
         * @param strTabName
         * @param listOutInfo
         * @return int
         */
        int searchHumanInfoByTable(std::string strTabName, std::list<HumanLibsInfo_S>& listOutInfo);



        /**
         * @brief 全局向量库比对
         * @param vfData 输入比对人脸信息
         * @param vIndices 输出比对结果人脸信息
         */
        bool comparisonFaceNormalize(
            const std::vector<float>& vfData,
            std::vector<int64_t>&     vIndices,
            std::vector<float>&       vfSimilarity,
            int                       nNum = 0);

        /**
         * @brief 全局向量库比对
         * @param vfData 输入比对人脸信息
         * @param vIndices 输出比对结果人脸信息
         */
        bool comparisonHumanNormalize(
            const std::vector<float>& vfData,
            std::vector<int64_t>&     vIndices,
            std::vector<float>&       vfSimilarity,
            int                       nNum = 0);

    private:

        /* 人脸数据库 */
        FaceSqlite  m_FaceInfodb;
        HumanSqlite m_HumanInfodb;

        /* 向量数据库 */
        VectorDb m_FaceVectordb;
        VectorDb m_HumanVectordb;


        TanSignal<HeaderInfo_S, UserHeaderInfo_S, FaceLibsInfo_S, HumanLibsInfo_S> sig_sendData;
    };
}    // namespace Ai0630_NS
