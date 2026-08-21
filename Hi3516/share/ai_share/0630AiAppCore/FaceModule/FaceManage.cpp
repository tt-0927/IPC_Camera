/**
 * @file FaceManage.cpp
 * @author xiejh (xiejh@kfb.cn)
 * @date 2024-10-18
 *
 * @brief 人员数据库管理：提供数据库操作接口，每操作一次人员数据库，需对向量数据库进行同步执行，以确保向量数据的同步
 */

#include "FaceManage.hpp"

#include <unistd.h>

#include "Behavior.hpp"
#include "ConvertInterface.h"
#include "ConvertJson.hpp"
#include "FaceBranchFlags.hpp"

using namespace Ai0630_NS;
using namespace Modules_NS;

static std::tuple<int, float> softmax(const std::vector<float>& x)
{
    if (x.empty())
    {
        /* 空输入，返回无效 */
        return { -1, 0.0f };
    }

    /* 1. 找最大值（为了数值稳定） */
    float m = *std::max_element(x.begin(), x.end());

    /* 2. 计算分母 */
    double sum = 0.0;
    for (float v : x)
    {
        sum += std::exp(double(v) - double(m));
    }

    /* 如果 sum == 0（极端情况） */
    if (sum == 0.0)
    {
        return { -1, 0.0f };
    }

    /* 3. 找最大 softmax 值及其 index */
    float fMax   = -1.0f;
    int   nIndex = -1;

    for (int i = 0; i < (int)x.size(); i++)
    {
        float fTemp = float(std::exp(double(x[i]) - double(m)) / sum);
        if (fTemp > fMax)
        {
            fMax   = fTemp;
            nIndex = i;
        }
    }

    return { nIndex, fMax };
}

FaceManage::FaceManage()
    : m_FaceInfodb(),
      m_HumanInfodb(),
      m_FaceVectordb(FACE_VECTOR_DB_PATH),
      m_HumanVectordb(HUMAN_VECTOR_DB_PATH)
{
    m_FaceVectordb.load();
    m_HumanVectordb.load();
}

FaceManage::~FaceManage()
{
    m_FaceVectordb.save();
    m_HumanVectordb.save();
}

/* 处理个人识别数据 */
void FaceManage::recvData(
    HeaderInfo_S     stHeader,
    UserHeaderInfo_S stUserHeader,
    FaceResult_S     stFaceResult)
{
    if (stHeader.nCode != toInt(CommCode_E::AI_COM_ST_ANALYSE) &&
        stHeader.nCode != toInt(CommCode_E::AI_COM_TE_ANALYSE))
    {
        return;
    }

    dlog(LOG_TRACE, "处理个人识别数据: nAiFlags=%ld",
         stHeader.nAiFlags);

    using AIResultMap = std::unordered_map<int, std::array<std::optional<AIResult_S>, 6>>;
    AIResultMap stResult;

    auto insertFrom = [&](const std::vector<AIResult_S>& vstSrc, int nIdx) {
        for (const auto& item : vstSrc)
        {
            auto& entry = stResult[item.nId];
            /* 放入对应位置 */
            entry[nIdx] = item;
        }
    };

    insertFrom(stFaceResult.vstAIFaceDetectResult, 0);
    insertFrom(stFaceResult.vstAIHeadDetectResult, 1);
    insertFrom(stFaceResult.vstAIFastPoseResult, 2);
    insertFrom(stFaceResult.vstAIFaceFeatureResult, 3);
    insertFrom(stFaceResult.vstAIHumanFeatureResult, 4);
    insertFrom(stFaceResult.vstAIFaceEmotionResult, 5);

    for (auto& [nId, arr] : stResult)
    {
        /* 人脸识别 */
        std::vector<int64_t> vFaceIndices;
        std::vector<float>   vfFaceSimilarity;

        std::tuple<int, float> oemInfo;

        Inference_NS::ClsData_S stHumanFeatureData;
        Inference_NS::ClsData_S stFaceFeatureData;

        int nBehavior = -1;
        int nPosture  = -1;

        /* 人脸识别 */
        if (arr[0].has_value())
        {
        }

        /* 人头识别 */
        if (arr[1].has_value())
        {
        }

        /* 人体骨骼点识别 */
        if (arr[2].has_value())
        {
            std::vector<Inference_NS::Point_S> vPoints; /* 点信息 */
            AIResult_S&                        stAIResult = arr[2].value();

            if (stAIResult.vstPointData.size() > 0)
            {
                Inference_NS::PointData_S stData = stAIResult.vstPointData[0];
                BehaviorParam_S           stParam1;
                nBehavior = Behavior::getType(stData.vPoints, stParam1);
                SupClsInsParam_S stParam2;
                nPosture = Behavior::getType(stData.vPoints, stParam2);
            }
        }

        /* 人脸特征识别 */
        if (arr[3].has_value())
        {
            AIResult_S& stAIResult = arr[3].value();
            if (stAIResult.vstClsData.size() > 0)
            {
                stFaceFeatureData = stAIResult.vstClsData[0];
                dlog(LOG_TRACE, "%d 对比人脸库。。。", nId);
                // for (size_t i = 0; i < stFaceFeatureData.vFeature.size(); ++i)
                // {
                //     std::cout << "v[" << i << "] = " << stFaceFeatureData.vFeature[i] << "\n";
                // }
                comparisonFaceNormalize(
                    stFaceFeatureData.vFeature,
                    vFaceIndices,
                    vfFaceSimilarity,
                    1);
            }
        }

        /* 人体特征识别 */
        if (arr[4].has_value())
        {
            AIResult_S& stAIResult = arr[4].value();
            if (stAIResult.vstClsData.size() > 0)
            {
                stHumanFeatureData = stAIResult.vstClsData[0];
            }
        }

        /* 表情识别 */
        if (arr[5].has_value())
        {
            AIResult_S& stAIResult = arr[5].value();
            if (stAIResult.vstClsData.size() > 0)
            {
                Inference_NS::ClsData_S& stFaceEmoData = stAIResult.vstClsData[0];
                /* 表情后处理 */
                std::vector<float>&      vFeature      = stFaceEmoData.vFeature;

                oemInfo = softmax(vFeature);
            }
        }


        auto [nEmoType, fConfidence] = oemInfo;

        if (vFaceIndices.size() > 0)
        {
            FaceLibsInfo_S stOutFaceInfo;
            /* 获取信息 */
            int            nDbId       = vFaceIndices.at(0);
            float          fSimilarity = vfFaceSimilarity.at(0);
            if (m_FaceInfodb.searchDataById(nDbId, stOutFaceInfo) >= OK)
            {
                HumanLibsInfo_S stHumanInfo;
                stHumanInfo.nClassId      = stUserHeader.nClassId;
                stHumanInfo.nEmoType      = nEmoType;
                stHumanInfo.nBehaviorType = nBehavior;
                stHumanInfo.nPostureType  = nPosture;
                stHumanInfo.nClassTime    = stUserHeader.nClassTime;
                stHumanInfo.lTimestamp    = stUserHeader.lTimestamp;
                stHumanInfo.fConfidence   = fSimilarity;
                sig_sendData.emit(stHeader, stUserHeader, stOutFaceInfo, stHumanInfo);

                dlog(LOG_INFO, "%d 识别到人脸: [%d], 相识度：%f", nId, stOutFaceInfo.nMemberId, fSimilarity);
                stHumanInfo.print();


                std::vector<int64_t> vHumanIndices;
                std::vector<float>   vfHumanSimilarity;

                /* 找到相似的人脸，用这个人体特征进行匹配*/
                if (comparisonHumanNormalize(stOutFaceInfo.vfData1, vHumanIndices, vfHumanSimilarity))
                {
                    /* 把找到的人体特征都算在这个人脸上 */
                    for (auto item : vHumanIndices)
                    {
                        HumanLibsInfo_S stOutHumanInfo;
                        if (m_HumanInfodb.searchDataById(item, stOutHumanInfo) >= OK)
                        {
                            m_HumanVectordb.removeFaceVector(item);
                            m_HumanInfodb.deleteData(item);

                            UserHeaderInfo_S stTemp;
                            stTemp.nClassId   = stOutHumanInfo.nClassId;
                            stTemp.nClassTime = stOutHumanInfo.nClassTime;
                            stTemp.lTimestamp = stOutHumanInfo.lTimestamp;
                            sig_sendData.emit(stHeader, stTemp, stOutFaceInfo, stOutHumanInfo);
                            dlog(LOG_INFO, "%d 通过人类数据库识别到人脸: [%d]", nId, stOutFaceInfo.nMemberId);
                            stOutHumanInfo.print();
                        }
                    }
                    m_HumanVectordb.save();
                }

                vHumanIndices.clear();
                vfHumanSimilarity.clear();
                if (comparisonHumanNormalize(stOutFaceInfo.vfData2, vHumanIndices, vfHumanSimilarity))
                {
                    /* 把找到的人体特征都算在这个人脸上 */
                    for (auto item : vHumanIndices)
                    {
                        HumanLibsInfo_S stOutHumanInfo;
                        if (m_HumanInfodb.searchDataById(item, stOutHumanInfo) >= OK)
                        {
                            m_HumanVectordb.removeFaceVector(item);
                            m_HumanInfodb.deleteData(item);

                            UserHeaderInfo_S stTemp;
                            stTemp.nClassId   = stOutHumanInfo.nClassId;
                            stTemp.nClassTime = stOutHumanInfo.nClassTime;
                            stTemp.lTimestamp = stOutHumanInfo.lTimestamp;
                            sig_sendData.emit(stHeader, stTemp, stOutFaceInfo, stOutHumanInfo);
                            dlog(LOG_INFO, "%d 通过人类数据库识别到人脸: [%d]", nId, stOutFaceInfo.nMemberId);
                            stOutHumanInfo.print();
                        }
                    }
                    m_HumanVectordb.save();
                }
            }
            else
            {
                /* 没找到。把信息放到人体体征库 */
                HumanLibsInfo_S stHumanLibsInfo;
                stHumanLibsInfo.nClassId      = stUserHeader.nClassId;
                stHumanLibsInfo.nEmoType      = nEmoType;
                stHumanLibsInfo.nBehaviorType = nBehavior;
                stHumanLibsInfo.nPostureType  = nPosture;
                stHumanLibsInfo.nClassTime    = stUserHeader.nClassTime;
                stHumanLibsInfo.lTimestamp    = stUserHeader.lTimestamp;
                stHumanLibsInfo.vfFaceData    = stFaceFeatureData.vFeature;
                stHumanLibsInfo.vfHumanData   = stHumanFeatureData.vFeature;
                addHumanLibInfo(stHumanLibsInfo);

                dlog(LOG_INFO, "%d 未别到人脸:", nId);
                stHumanLibsInfo.print();
            }
        }
        else
        {
            /* 没找到。把信息放到人体体征库 */
            HumanLibsInfo_S stHumanLibsInfo;
            stHumanLibsInfo.nClassId      = stUserHeader.nClassId;
            stHumanLibsInfo.nEmoType      = nEmoType;
            stHumanLibsInfo.nBehaviorType = nBehavior;
            stHumanLibsInfo.nPostureType  = nPosture;
            stHumanLibsInfo.nClassTime    = stUserHeader.nClassTime;
            stHumanLibsInfo.lTimestamp    = stUserHeader.lTimestamp;
            stHumanLibsInfo.vfFaceData    = stFaceFeatureData.vFeature;
            stHumanLibsInfo.vfHumanData   = stHumanFeatureData.vFeature;
            addHumanLibInfo(stHumanLibsInfo);

            dlog(LOG_INFO, "%d 未别到人脸:", nId);
            stHumanLibsInfo.print();
        }
    }
    return;
}

/* 处理人脸特征数据 */
void FaceManage::recvFaceFeature(
    HeaderInfo_S   stHeader,
    FaceLibsInfo_S stFaceLibsInfo,
    FaceResult_S   stFaceResult)
{
    auto& vstAIResult = stFaceResult.vstAIFaceFeatureResult;
    if (vstAIResult.size() == 1)
    {
        if (vstAIResult.at(0).vstClsData.size() == 1)
        {
            stFaceLibsInfo.vfData1 = vstAIResult.at(0).vstClsData.at(0).vFeature;
        }
    }
    else if (vstAIResult.size() == 2)
    {
        if (vstAIResult.at(0).vstClsData.size() == 1)
        {
            stFaceLibsInfo.vfData1 = vstAIResult.at(0).vstClsData.at(0).vFeature;
        }
        if (vstAIResult.at(1).vstClsData.size() == 1)
        {
            stFaceLibsInfo.vfData2 = vstAIResult.at(1).vstClsData.at(0).vFeature;
        }
    }
    stFaceLibsInfo.print();
    addFaceLibInfo(stFaceLibsInfo);
    return;
}

/* 删除人脸数据表 */
int FaceManage::deleteFaceTable(std::string strTabName)
{
    int nRet = 0;
    if (strTabName.empty())
    {
        m_FaceVectordb.clear();
        m_FaceVectordb.save();
        return m_FaceInfodb.clearAllTables();
    }
    else
    {
        /* 清空旧的人脸库 */
        std::list<FaceLibsInfo_S> listOutFaceInfo;

        int nRet = m_FaceInfodb.searchDataByTable(strTabName, listOutFaceInfo);
        if (nRet >= 0)
        {
            for (const auto& item : listOutFaceInfo)
            {
                m_FaceVectordb.removeFaceVector(item.nId);
            }
            m_FaceVectordb.save();
        }

        nRet = m_FaceInfodb.deleteTable(strTabName);
    }

    return nRet;
}

/* 删除人类数据表 */
int FaceManage::deleteHumanTable(std::string strTabName)
{
    int nRet = 0;
    if (strTabName.empty())
    {
        m_HumanVectordb.clear();
        m_HumanVectordb.save();
        return m_HumanInfodb.clearAllTables();
    }
    else
    {
        std::list<HumanLibsInfo_S> listOutHumanInfo;

        nRet = m_HumanInfodb.searchDataByTable(strTabName, listOutHumanInfo);
        if (nRet >= 0)
        {
            for (const auto& item : listOutHumanInfo)
            {
                m_HumanVectordb.removeFaceVector(item.nId);
            }
            m_HumanVectordb.save();
        }

        nRet = m_HumanInfodb.deleteTable(strTabName);
    }
    return nRet;
}

/* 根据表名查找人脸信息 */
int Ai0630_NS::FaceManage::searchFaceInfoByTable(
    std::string                strTabName,
    std::list<FaceLibsInfo_S>& listOutInfo)
{
    int nRet = m_FaceInfodb.searchDataByTable(strTabName, listOutInfo);

    return nRet;
}

/* 新增人脸库 */
int FaceManage::addFaceLibInfo(FaceLibsInfo_S& stInfo)
{
    int nRet = m_FaceInfodb.insertData(stInfo);
    m_FaceVectordb.addFaceVector(stInfo.nId, stInfo.vfData1);
    m_FaceVectordb.addFaceVector(stInfo.nId, stInfo.vfData2);
    m_FaceVectordb.save();

    dlog(LOG_INFO, "添加本地人脸库 [%d]...", stInfo.nId);

    return nRet;
}

/* 新增人类信息库 */
int FaceManage::addHumanLibInfo(HumanLibsInfo_S& stInfo)
{
    int nRet = m_HumanInfodb.insertData(stInfo);
    m_HumanVectordb.addFaceVector(stInfo.nId, stInfo.vfHumanData);
    m_HumanVectordb.save();

    dlog(LOG_INFO, "添加未知的人类信息库 [%d]...", stInfo.nId);

    return nRet;
}

/* 删除人脸库 */
int FaceManage::delFaceLibInfo(int nId)
{
    int nRet = m_FaceInfodb.deleteData(nId);
    m_FaceVectordb.removeFaceVector(nId);
    m_FaceVectordb.save();

    dlog(LOG_INFO, "删除本地人脸库 [%d]...", nId);

    return nRet;
}

/* 删除人类信息库 */
int FaceManage::delHumanLibInfo(int nId)
{
    int nRet = m_HumanInfodb.deleteData(nId);
    m_HumanVectordb.removeFaceVector(nId);
    m_HumanVectordb.save();

    dlog(LOG_INFO, "删除未知的人类信息库 [%d]...", nId);

    return nRet;
}

/* 根据表名查找人类信息 */
int FaceManage::searchHumanInfoByTable(
    std::string                 strTabName,
    std::list<HumanLibsInfo_S>& listOutInfo)
{
    int nRet = m_HumanInfodb.searchDataByTable(strTabName, listOutInfo);

    return nRet;
}

/* 全局向量库比对 */
bool FaceManage::comparisonFaceNormalize(
    const std::vector<float>& vfData,
    std::vector<int64_t>&     vIndices,
    std::vector<float>&       vfSimilarity,
    int                       nNum)
{
    SearchResult_S stSearchRes;

    /* 调用 searchNormalize 函数进行人脸数据的归一化搜索 */
    if (m_FaceVectordb.searchNormalize(vfData, stSearchRes, nNum))
    {
        dlog(LOG_TRACE, "在人脸向量库找到: count = %ld", stSearchRes.vIndices.size());

        if (stSearchRes.vIndices.size() <= 0 ||
            stSearchRes.vIndices.size() != stSearchRes.vDistances.size())
        {
            return false;
        }

        for (int i = 0; i < (int)stSearchRes.vIndices.size(); i++)
        {
            dlog(LOG_TRACE, "在人脸向量库找到数据: [%d] 相识度[%f]",
                 i,
                 stSearchRes.vDistances[i]);
            if (stSearchRes.vDistances[i] > FACE_SIMILARITY_THRESHOLD)
            {
                vIndices.push_back(stSearchRes.vIndices[i]);
                vfSimilarity.push_back(stSearchRes.vDistances[i]);
            }
        }
    }
    else
    {
        return false;
    }

    return true;
}

/* 全局向量库比对 */
bool FaceManage::comparisonHumanNormalize(
    const std::vector<float>& vfData,
    std::vector<int64_t>&     vIndices,
    std::vector<float>&       vfSimilarity,
    int                       nNum)
{
    SearchResult_S stSearchRes;

    /* 调用 searchNormalize 函数进行人脸数据的归一化搜索 */
    if (m_HumanVectordb.searchNormalize(vfData, stSearchRes, nNum))
    {
        dlog(LOG_DEBUG, "count = %ld", stSearchRes.vIndices.size());

        if (stSearchRes.vIndices.size() <= 0)
        {
            std::cout << "没找到相似的" << std::endl;
            return false;
        }

        for (int i = 0; i < (int)stSearchRes.vIndices.size(); i++)
        {
            if (stSearchRes.vDistances[i] > HUMAN_SIMILARITY_THRESHOLD)
            {
                vIndices.push_back(stSearchRes.vIndices[i]);
                vfSimilarity.push_back(stSearchRes.vDistances[i]);
            }
        }
    }
    else
    {
        std::cout << "没找到相似的" << std::endl;
        return false;
    }

    return true;
}
