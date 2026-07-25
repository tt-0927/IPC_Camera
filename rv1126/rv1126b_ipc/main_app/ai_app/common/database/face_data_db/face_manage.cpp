/**
 * @FilePath     : face_manage.cpp
 * @Author       : zhouzirui
 * @Date         : 2025-06-06 16:02:10
 * @LastEditors  : zhouzirui
 * @LastEditTime : 2025-06-06 16:55:38
 * @Description  : 人员数据库管理：提供数据库操作接口，每操作一次人员数据库，需对向量数据库进行同步执行，以确保向量数据的同步
 */

#include "face_manage.h"
#include <unistd.h>

using namespace Event;
using namespace FaceDataDB_NS;
using namespace FaceManage;


AIFaceManage::AIFaceManage()
    :m_SnapInfodb(), m_FaceInfodb(), m_FaceVectordb("/userdata/cam/db/FaceSqlite.db")//, m_FaceVectordb()
{
    // m_FaceVectordb.load();
}


AIFaceManage::~AIFaceManage()
{
    // m_FaceVectordb.save();
}


/**
 * @brief 新增名单库人脸信息
 * @param stInfo 
 * @return int 
 */
int AIFaceManage::addFaceLibInfo(FaceLibsInfo_S &stInfo)
{
    // int nRet = m_FaceInfodb.insertData(stInfo);
    
    // dlog_debug("ai_app: \033[34m %s:%d 添加本地名单库 [%d]... \033[m\n",__func__,__LINE__,stInfo.nId);
    // std::string strData;
    // if (nRet != 0)
    // {
    //     strData = "{\"result\": \"Failed to add face list information.\" }";
    // }
    // SendResToControl(strData, AC_ADD_FACE_INFO, nRet);
    
    return 0;
}


/**
 * @brief 删除名单库人脸信息
 * @param nId 
 * @return int 
 */
int AIFaceManage::delFaceLibInfo(int nId)
{
    // int nRet = m_FaceInfodb.deleteData(nId);

    // dlog_debug("ai_app: \033[34m %s:%d 删除本地名单库 [%d]... \033[m\n",__func__,__LINE__,nId);
    // std::string strData;
    // if (nRet != 0)
    // {
    //     strData = "{\"result\": \"Failed to delete face list information.\" }";
    // }
    // SendResToControl(strData, AC_DEL_FACE_INFO, nRet);
    
    return 0;
}


/**
 * @brief 更新名单库人脸信息
 * @param nId 
 * @param stInfo 
 * @return int 
 */
int AIFaceManage::updateFaceLibInfo(int nId, FaceLibsInfo_S stInfo)
{
    // int nRet = m_FaceInfodb.updateData(nId, stInfo);

    // dlog_debug("ai_app: \033[34m %s:%d 更新本地名单库 [%d]... \033[m\n",__func__,__LINE__,nId);
    // std::string strData;
    // if (nRet != 0)
    // {
    //     strData = "{\"result\": \"Failed to update face list information.\" }";
    // }
    // SendResToControl(strData, AC_SET_FACE_INFO, nRet);
    
    return 0;
}


/**
 * @brief 通过ID查找名单组人脸信息
 * @param nId 
 * @param stInfo 
 * @return int 
 */
int AIFaceManage::searchFaceInfoById(int nId, FaceLibsInfo_S &stInfo)
{
    m_FaceInfodb.searchDataById(nId, stInfo);
    return 0;
}


/**
 * @brief 通过ID查找人脸信息
 * @param nId 
 * @param stInfo 
 * @return int 
 */
int AIFaceManage::searchFaceInfoById(int nId, SnapFaceInfo_S &stInfo)
{
    m_SnapInfodb.searchData(nId, stInfo);
    return 0;
}


/* 根据表名查找人脸信息 */
int AIFaceManage::searchFaceInfoByTable(std::string strTabName, std::list<FaceLibsInfo_S>& listOutInfo)
{
    // int nRet = m_FaceInfodb.searchDataByTable(strTabName, listOutInfo);

    // std::vector<Event::FaceInfo_S> listFaceInfo;
    // for (const auto& OutInfo : listOutInfo)
    // {
    //     Event::FaceInfo_S stFaceInfo;
    //     stFaceInfo.nId = OutInfo.nId;
    //     stFaceInfo.strFaceLibName = OutInfo.strFaceLibName;
    //     stFaceInfo.strName = OutInfo.strName;
    //     stFaceInfo.strPhoneNum = OutInfo.strPhoneNum;
    //     stFaceInfo.strPicPath = OutInfo.strPicPath;
    //     stFaceInfo.strPicType = OutInfo.strPicType;
    //     stFaceInfo.nPicSize = OutInfo.nPicSize;
    //     stFaceInfo.strPicDate = OutInfo.strPicDate;
    //     stFaceInfo.nModelState = OutInfo.nModelState;
    //     stFaceInfo.nRatingLevel = OutInfo.nRatingLevel;
    //     listFaceInfo.push_back(stFaceInfo);
    // }
    
    // dlog_debug("ai_app: \033[34m %s:%d 表名查询表 [%s : %ld] \033[m\n",__func__,__LINE__,strTabName.c_str(), listOutInfo.size());
    
    // std::string strData;
    // if (nRet != 0)
    // {
    //     strData = "{\"result\": \"Failed to search face list information.\" }";
    // }
    // else
    // {
    //     strData = Convert::to_string(listFaceInfo);
    // }
    
    // SendResToControl(strData, AC_GET_FACE_INFO, nRet);
    
    return 0;
}


/*  根据条件查询人脸信息 */
int AIFaceManage::searchFaceInfoByCond(Event::FaceFind_S stFaceFind, std::list<FaceLibsInfo_S>& listOutInfo)
{
    // int nRet = m_FaceInfodb.search_combined_data(stFaceFind, listOutInfo);
    
    // std::vector<Event::FaceInfo_S> listFaceInfo;
    // for (const auto& OutInfo : listOutInfo)
    // {
    //     Event::FaceInfo_S stFaceInfo;
    //     stFaceInfo.nId = OutInfo.nId;
    //     stFaceInfo.strFaceLibName = OutInfo.strFaceLibName;
    //     stFaceInfo.strName = OutInfo.strName;
    //     stFaceInfo.strPhoneNum = OutInfo.strPhoneNum;
    //     stFaceInfo.strPicPath = OutInfo.strPicPath;
    //     stFaceInfo.strPicType = OutInfo.strPicType;
    //     stFaceInfo.nPicSize = OutInfo.nPicSize;
    //     stFaceInfo.strPicDate = OutInfo.strPicDate;
    //     stFaceInfo.nModelState = OutInfo.nModelState;
    //     stFaceInfo.nRatingLevel = OutInfo.nRatingLevel;
    //     listFaceInfo.push_back(stFaceInfo);
    // }
    
    // dlog_debug("ai_app: \033[34m %s:%d 条件查询表 [%s : %ld] \033[m\n",__func__,__LINE__,stFaceFind.strFaceLibName.c_str(), listOutInfo.size());

    // std::string strData;
    // if (nRet != 0)
    // {
    //     strData = "{\"result\": \"Failed to search face list information.\" }";
    // }
    // else
    // {
    //     strData = Convert::to_string(listFaceInfo);
    // }
    
    // SendResToControl(strData, AC_GET_FACE_INFO, nRet);
    
    return 0;
}


/* 创建名单组表 */
int AIFaceManage::creatFaceTable(std::string strTabName)
{
    int nRet = m_FaceInfodb.check_creat_table(strTabName);

    dlog_debug("ai_app: \033[34m %s:%d 添加表 [%s] \033[m\n",__func__,__LINE__,strTabName.c_str());
    std::string strData;
    if (nRet != 0)
    {
        strData = "{\"result\": \"Failed to add face list.\" }";
    }
    // SendResToControl(strData, AC_ADD_TARGET_LIB, nRet);
    
    return 0;
}


/* 删除名单组表 */
int AIFaceManage::deleteFaceTable(std::string strTabName)
{
    int nRet = m_FaceInfodb.deleteTable(strTabName);
    
    dlog_debug("ai_app: \033[34m %s:%d 删除表 [%s] \033[m\n",__func__,__LINE__,strTabName.c_str());
    std::string strData;
    if (nRet != 0)
    {
        strData = "{\"result\": \"Failed to delete face list.\" }";
    }
    // SendResToControl(strData, AC_DEL_TARGET_LIB, nRet);
    
    return 0;
}


/* 修改名单组表名 */
int AIFaceManage::renameFaceTable(std::string oldTabName, std::string newTabName)
{
    int nRet = m_FaceInfodb.renameTable(oldTabName, newTabName);
    
    dlog_debug("ai_app: \033[34m %s:%d 重命名表 [%s  %s] \033[m\n",__func__,__LINE__,oldTabName.c_str(),newTabName.c_str());
    std::string strData;
    if (nRet != 0)
    {
        strData = "{\"result\": \"Failed to rename face list.\" }";
    }
    // SendResToControl(strData, AC_SET_TARGET_LIB, nRet);

    return 0;
}


/* 获取名单组信息 */
int AIFaceManage::getTableReport(std::vector<Event::FaceLibInfo_S>& listTableReport)
{
    // int nRet = m_FaceInfodb.get_table_report(listTableReport);
    
    // if (access("testPrint", F_OK) == 0)
    // {
    //     for (const auto& report : listTableReport)
    //     {
    //         std::cout << "表名: " << report.strFaceLibName
    //                 << ", 总记录数: " << report.nTotalFace
    //                 << ", 正常记录数: " << report.nNormalNum
    //                 << ", 异常记录数: " << report.nAbnormalNum << std::endl;
        
    //     }
    // }

    // std::string strData;
    // if (nRet != 0)
    // {
    //     strData = "{\"result\": \"Failed to get face list information.\" }";
    // }
    // else
    // {
    //     strData = Convert::to_string(listTableReport);
    // }
    
    // SendResToControl(strData, AC_GET_TARGET_LIB, nRet);
    
    return 0;
}


/**
 * @brief 新增抓拍人脸信息，同步向量数据库
 * @param stInfo 
 * @return int 
 */
int AIFaceManage::addSnapFace(SnapFaceInfo_S stInfo, int& nFaceLibId, float &fSimilarity)
{
    /* 比对本地名单库 */
    int nRet = comparisonFaceLib(stInfo.vfData, nFaceLibId, fSimilarity);

    m_SnapInfodb.insertData(stInfo);

    // m_FaceVectordb.addFaceVector(stInfo.nId, stInfo.vfData);
    // m_FaceVectordb.save();
    
    return nRet;
}


/**
 * @brief 删除抓拍人脸信息，同步向量数据库
 * @param nId 
 * @return int 
 */
int AIFaceManage::delSnapFace(int nId)
{
    m_SnapInfodb.deleteData(nId);

    // m_FaceVectordb.removeFaceVector(nId);
    // m_FaceVectordb.save();
    
    return 0;
}


/**
 * @brief 更新抓拍人脸信息，同步向量数据库
 * @param nId 
 * @param stInfo 
 * @return int 
 */
int AIFaceManage::updateSnapFace(int nId, SnapFaceInfo_S stInfo)
{
    m_SnapInfodb.updateData(nId, stInfo);

    // m_FaceVectordb.removeFaceVector(nId);
    // m_FaceVectordb.addFaceVector(nId, stInfo.vfData);
    // m_FaceVectordb.save();
    
    return 0;
}


/**
 * @brief 全局向量库比对
 * @param vfData 输入比对人脸信息
 * @param nFaceLibId 输出比对结果人脸信息
 */
bool AIFaceManage::comparisonNormalize(const std::vector<float>& vfData, std::vector<int64_t>& vIndices, std::vector<float>& vfSimilarity)
{
    dlog_debug("============");
    auto best = m_FaceVectordb.matchFromAllTables(vfData);
    if(best.id < 0 )
    {
        std::cout << "没找到相似的" << std::endl;
        return false;
    }else{
        std::cout << "所有表格中的最佳匹配项:\n";
        std::cout << "Table: " << best.tableName << ", ID: " << best.id << ", Name: " << best.name << ", Similarity: " << best.similarity << std::endl;
    }
    // SearchResult_S stSearchRes;
    
    /* 调用 searchNormalize 函数进行人脸数据的归一化搜索 */
    // if (m_FaceVectordb.searchNormalize(vfData, stSearchRes))
    // {
    //     dlog_debug("ai_app: \033[34m %s:%d count = %ld \033[m\n",__func__,__LINE__,stSearchRes.vIndices.size());
        
    //     if (stSearchRes.vIndices.size() <= 0)
    //     {
    //         std::cout << "没找到相似的" << std::endl;
    //         return false;
    //     }
        
    //     for (int i = 0; i < (int)stSearchRes.vIndices.size(); i++)
    //     {
    //         if (stSearchRes.vDistances[i] > FACE_SIMILARITY_THRESHOLD)
    //         {
    //             if (access("testPrint", F_OK) == 0)
    //             {
    //                 std::cout << i << ": 全局向量库: 最相似的是: " << stSearchRes.vIndices[i] << "; 距离: " << stSearchRes.vDistances[i] << std::endl;
    //             }
                
    //             vIndices.push_back(stSearchRes.vIndices[i]);
    //             vfSimilarity.push_back(stSearchRes.vDistances[i]);
    //         }
    //     }
    // }
    // else
    // {
    //     std::cout << "没找到相似的" << std::endl;
    //     return false;
    // }
    
    return true;
}


/**
 * @brief 本地名单库向量比对
 * @param vfData 
 * @param nFaceLibId 
 */
bool AIFaceManage::comparisonFaceLib(const std::vector<float>& vfData, int& nFaceLibId, float &fSimilarity)
{
    /* 最佳匹配的人脸信息 */
    FaceLibsInfo_S stInfo {};

    /* 临时相似度存储 */
    float flastSimilarity = 0;

    /* 从数据库获取所有数据 */
    std::list<FaceLibsInfo_S> libAllInfo;
    m_FaceInfodb.getAllData(libAllInfo);

    for (const auto& libInfo : libAllInfo)
    {
        if (libInfo.vfData.empty())
        {
            continue;
        }

        flastSimilarity = cosine_similarity(vfData, libInfo.vfData);
        
        /* 取出最高相似度的人脸信息 */
        if (flastSimilarity > fSimilarity)
        {
            stInfo = libInfo;
            fSimilarity = flastSimilarity;
        }
    }
    
    std::cout << "本地名单库: 最相似的是: " << stInfo.nId << "; 距离: " << fSimilarity << std::endl;
    
    nFaceLibId = stInfo.nId;
    
    if (fSimilarity > FACE_SIMILARITY_THRESHOLD)
    {
        return true;
    }

    return false;
}


/**
 * @brief 余弦相似度计算
 * @return float 
 */
float AIFaceManage::cosine_similarity(const std::vector<float>& A, const std::vector<float>& B)
{
    if (A.size() != B.size())
    {
        throw std::invalid_argument("Vectors must be of the same length");
    }

    float dot_product = 0.0f;
    float norm_A = 0.0f;
    float norm_B = 0.0f;

    for (size_t i = 0; i < A.size(); ++i)
    {
        dot_product += A[i] * B[i];
        norm_A += A[i] * A[i];
        norm_B += B[i] * B[i];
    }

    norm_A = std::sqrt(norm_A);
    norm_B = std::sqrt(norm_B);

    if (norm_A == 0.0f || norm_B == 0.0f)
    {
        throw std::runtime_error("Zero vector provided");
    }

    return dot_product / (norm_A * norm_B);
}
