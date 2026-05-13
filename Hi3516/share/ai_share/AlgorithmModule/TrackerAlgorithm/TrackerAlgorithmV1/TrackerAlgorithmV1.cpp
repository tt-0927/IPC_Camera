/*
 * @FilePath     : TrackerAlgorithmV1.cpp
 * @Author       : yanzeh yanzeh@kfb.cn
 * @Date         : 2024-01-08 15:13:54
 * @LastEditors  : 严泽辉 yanzeh@kfb.cn
 * @LastEditTime : 2024-02-26 15:06:21
 * @Description  : 第一代跟踪模块
 */
#include "TrackerAlgorithmV1.hpp"

#include "auxiliary_track.h"
#include "dlog.h"
#include "ImageProcessor.hpp"

using namespace TA_NS;

#define TRACK_ANALYSE_WIDTH  1920
#define TRACK_ANALYSE_HEIGHT 1024

/* 一组数据的大小 */
#define POS_DATA_GROUP_SIZE       6
#define TRACK_POS_DATA_GROUP_SIZE 4

CTrackerAlgorithmV1::CTrackerAlgorithmV1(TrackerAlgorithmInParam_S stInParam)
    : CTrackerAlgorithmBase(stInParam)
{
    if (m_pAlgorithm == nullptr)
    {
        SLIDETRACKPARAMS stPslidetrack;
        stPslidetrack.FeatureModelPath = const_cast<char*>(stInParam.stNeedParam.strFeatureModelPath.c_str());
        stPslidetrack.HeadModelPath    = const_cast<char*>(stInParam.stNeedParam.strHeadModelPath.c_str());

        stPslidetrack.fConfidence          = stInParam.stExParam.fConfidenceThreshold;
        stPslidetrack.fSimilarityThreshold = stInParam.stExParam.fSimilarityThreshold;

        m_pAlgorithm = new AUXILLARY_TRACK(stPslidetrack);
    }
}

CTrackerAlgorithmV1::~CTrackerAlgorithmV1()
{
    if (m_pAlgorithm)
    {
        delete static_cast<AUXILLARY_TRACK*>(m_pAlgorithm);
        m_pAlgorithm = nullptr;
    }
}

/* 分析数据 */
BlError_E CTrackerAlgorithmV1::dataAnalysis(
    MediaDataInfo_S           stMediaDataInfo,
    TrackerAlgorithmResult_S& stOutInfo)
{
    if (nullptr == stMediaDataInfo.pchData)
    {
        dlog(LOG_ERROR, "传入参数为空");
        return ERR_IN_PARAM_NULL;
    }

    AUXILLARY_TRACK* pAuxTrack = static_cast<AUXILLARY_TRACK*>(m_pAlgorithm);
    if (nullptr == pAuxTrack)
    {
        dlog(LOG_ERROR, "未初始化算法类");
        return ERR_UNINIT;
    }

    BlError_E enRetCode = OK;

    std::vector<float> vecPos;
    vecPos.clear();

    std::vector<float> vTrackerPoints;
    vTrackerPoints.clear();

    stOutInfo.clear();

    if (stMediaDataInfo.stTracker.fX1 != 0.0f ||
        stMediaDataInfo.stTracker.fY1 != 0.0f ||
        stMediaDataInfo.stTracker.fX2 != 0.0f ||
        stMediaDataInfo.stTracker.fY2 != 0.0f)
    {
        /* 恢复算法 */
        // float ww, hh;
        // ww = (stMediaDataInfo.stTracker.fX2 - stMediaDataInfo.stTracker.fX1) * 1.5f;
        // hh = (stMediaDataInfo.stTracker.fY2 - stMediaDataInfo.stTracker.fY1) / 1.5f;

        // stMediaDataInfo.stTracker.fX1 += ww / 2.0f;
        // stMediaDataInfo.stTracker.fX2 -= ww / 2.0f;
        // stMediaDataInfo.stTracker.fY2 -= hh;


        vTrackerPoints.push_back(stMediaDataInfo.stTracker.fX1);
        vTrackerPoints.push_back(stMediaDataInfo.stTracker.fY1);
        vTrackerPoints.push_back(stMediaDataInfo.stTracker.fX2);
        vTrackerPoints.push_back(stMediaDataInfo.stTracker.fY2);

        dlog(LOG_ERROR, "设置跟踪坐标 [%f %f %f %f]",
             stMediaDataInfo.stTracker.fX1,
             stMediaDataInfo.stTracker.fY1,
             stMediaDataInfo.stTracker.fX2,
             stMediaDataInfo.stTracker.fY2);
    }

    int nVecSize = 0;

    PosInfo_S stPos;
    int       nIndex = 0;
    int       nRet   = 0;

    /* 图片处理器 */
    CImageProcessor imageProcessor;

    /* 转化后的数据 */
    char* pchData   = nullptr;
    int   nDataSize = 0;

    /* 判断数据是否符合要求 */
    if (stMediaDataInfo.nWidth != TRACK_ANALYSE_WIDTH ||
        stMediaDataInfo.nHeight != TRACK_ANALYSE_HEIGHT ||
        stMediaDataInfo.enFormat != RGB888)
    {
        if (m_stInParam.stExParam.bAutoConvert)
        {
            CImageProcessor::DataFormatType_E enInType;
            switch (stMediaDataInfo.enFormat)
            {
                case RGB888:
                {
                    enInType = CImageProcessor::RGB888;
                    break;
                }
                case BGR888:
                {
                    enInType = CImageProcessor::BGR888;
                    break;
                }
                case RGBA8888:
                {
                    enInType = CImageProcessor::RGBA8888;
                    break;
                }
                default:
                {
                    dlog(LOG_ERROR, "数据格式异常");
                    enRetCode = NOK;
                    goto EXIT;
                }
            }
            /* 转换成正确的格式 */
            enRetCode = imageProcessor.transition(enInType,
                                                  stMediaDataInfo.nWidth,
                                                  stMediaDataInfo.nHeight,
                                                  stMediaDataInfo.pchData,
                                                  stMediaDataInfo.nDataSize,
                                                  CImageProcessor::RGB888,
                                                  TRACK_ANALYSE_WIDTH,
                                                  TRACK_ANALYSE_HEIGHT,
                                                  &pchData,
                                                  nDataSize);
            if (enRetCode < OK)
            {
                dlog(LOG_ERROR, "转换数据失败");
                goto EXIT;
            }
        }
        else
        {
            dlog(LOG_ERROR, "数据格式异常，无法解析");
            enRetCode = NOK;
            goto EXIT;
        }
    }

    /* 分析图片数据 */
    if (nullptr == pchData)
    {
        nRet = pAuxTrack->AI(stMediaDataInfo.pchData, vTrackerPoints, vecPos);
    }
    else
    {
        nRet = pAuxTrack->AI(pchData, vTrackerPoints, vecPos);
    }
    // dlog(LOG_ERROR, "分析结果 [%d]", nRet);

    /* 获取需要跟踪的坐标信息 */
    nVecSize = vTrackerPoints.size() / TRACK_POS_DATA_GROUP_SIZE;
    for (nIndex = 0; nIndex < nVecSize; nIndex++)
    {
        if (vTrackerPoints.size() > ((nIndex * TRACK_POS_DATA_GROUP_SIZE) + 3))
        {
            stPos.clear();

            stPos.fX1 = vTrackerPoints[nIndex * TRACK_POS_DATA_GROUP_SIZE];
            stPos.fY1 = vTrackerPoints[(nIndex * TRACK_POS_DATA_GROUP_SIZE) + 1];
            stPos.fX2 = vTrackerPoints[(nIndex * TRACK_POS_DATA_GROUP_SIZE) + 2];
            stPos.fY2 = vTrackerPoints[(nIndex * TRACK_POS_DATA_GROUP_SIZE) + 3];

            stOutInfo.vstTrackerPos.push_back(stPos);
            // dlog(LOG_ERROR, "%f %f %f %f", stPos.fX1, stPos.fY1, stPos.fX2, stPos.fY2);
        }
    }

    /* 获取所有人物的坐标信息 */
    nVecSize = vecPos.size() / POS_DATA_GROUP_SIZE;
    for (nIndex = 0; nIndex < nVecSize; nIndex++)
    {
        if (vecPos.size() > ((nIndex * POS_DATA_GROUP_SIZE) + 3))
        {
            stPos.clear();

            stPos.fX1 = vecPos[nIndex * POS_DATA_GROUP_SIZE];
            stPos.fY1 = vecPos[(nIndex * POS_DATA_GROUP_SIZE) + 1];
            stPos.fX2 = vecPos[(nIndex * POS_DATA_GROUP_SIZE) + 2];
            stPos.fY2 = vecPos[(nIndex * POS_DATA_GROUP_SIZE) + 3];

            // dlog(LOG_ERROR, "%f %f %f %f", stPos.fX1, stPos.fY1, stPos.fX2, stPos.fY2);

            /* 扩充算法 */
            // float              ww, hh;
            // std::vector<float> vAiPoints;
            // ww         = stPos.fX2 - stPos.fX1;
            // hh         = stPos.fY2 - stPos.fY1;
            // stPos.fX1 -= ww / 1.5;
            // stPos.fX2 += ww / 1.5;
            // stPos.fY2 += hh * 1.5;
            // if (stPos.fX1 < 0)
            // {
            //     stPos.fX1 = 0;
            // }
            // if (stPos.fX2 > 1920)
            // {
            //     stPos.fX2 = 1920;
            // }
            // if (stPos.fY2 > 1080)
            // {
            //     stPos.fY2 = 1080;
            // }

            stOutInfo.vstPos.push_back(stPos);
        }
        // dlog(LOG_ERROR, "[%ld]", stOutInfo.vstPos.size());
    }

EXIT:
    stMediaDataInfo.free();

    if (pchData)
    {
        delete[] pchData;
        pchData = nullptr;
    }

    return enRetCode;
}
