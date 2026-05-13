/*
 * @FilePath     : CountingAlgorithmV1.cpp
 * @Author       : yanzeh yanzeh@kfb.cn
 * @Date         : 2024-01-08 15:13:54
 * @LastEditors  : 严泽辉 yanzeh@kfb.cn
 * @LastEditTime : 2024-01-27 15:17:56
 * @Description  : 第一代人数统计模块
 */
#include "CountingAlgorithmV1.hpp"

#include "dlog.h"
#include "ImageProcessor.hpp"
#include "rk_human_count_detect.h"

using namespace CA_NS;

/* 一组数据的大小 */
#define DATA_GROUP_SIZE 2

#define HUMAN_COUNT_ANALYSE_WIDTH  768
#define HUMAN_COUNT_ANALYSE_HEIGHT 640

CCountingAlgorithmV1::CCountingAlgorithmV1(CountingAnalyzerInParam_S stInParam)
    : CCountingAlgorithmBase(stInParam)
{
    if (nullptr == m_pAlgorithm)
    {
        m_pAlgorithm = new RK_COUNT_DETECT(const_cast<char*>(m_stInParam.stNeedParam.strModelPath.c_str()));

        RK_COUNT_DETECT* pAlgorithm = static_cast<RK_COUNT_DETECT*>(m_pAlgorithm);

        pAlgorithm->fInstance   = 1.0;
        pAlgorithm->nControl    = 4;
        pAlgorithm->nDelControl = 4;
    }
}

CCountingAlgorithmV1::~CCountingAlgorithmV1()
{
    if (m_pAlgorithm)
    {
        delete static_cast<RK_COUNT_DETECT*>(m_pAlgorithm);
        m_pAlgorithm = nullptr;
    }
}

/* 分析数据 */
BlError_E CA_NS::CCountingAlgorithmV1::dataAnalysis(
    MediaDataInfo_S           stMediaDataInfo,
    CountingAnalyzerResult_S& stOutInfo)
{
    if (nullptr == stMediaDataInfo.pchData)
    {
        dlog(LOG_ERROR, "传入参数为空");
        return ERR_IN_PARAM_NULL;
    }

    RK_COUNT_DETECT* pAlgorithm = static_cast<RK_COUNT_DETECT*>(m_pAlgorithm);
    if (nullptr == pAlgorithm)
    {
        dlog(LOG_ERROR, "未初始化算法类");
        return ERR_UNINIT;
    }

    int       nRet      = 0;
    BlError_E enRetCode = OK;

    std::vector<float> vecPos;
    vecPos.clear();

    stOutInfo.clear();

    int       nVecSize;
    PosInfo_S stPos;
    int       nIndex = 0;

    /* 图片处理器 */
    CImageProcessor imageProcessor;

    /* 转化后的数据 */
    char* pchData   = nullptr;
    int   nDataSize = 0;

    /* 判断数据是否符合要求 */
    if (stMediaDataInfo.nWidth != HUMAN_COUNT_ANALYSE_WIDTH ||
        stMediaDataInfo.nHeight != HUMAN_COUNT_ANALYSE_HEIGHT ||
        stMediaDataInfo.enFormat != MediaDataFormat_E::RGB888)
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
                                                  HUMAN_COUNT_ANALYSE_WIDTH,
                                                  HUMAN_COUNT_ANALYSE_HEIGHT,
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
            dlog(LOG_ERROR, "数据格式异常[%dx%d][%d]，无法解析",
                 stMediaDataInfo.nWidth,
                 stMediaDataInfo.nHeight,
                 stMediaDataInfo.enFormat);
            enRetCode = NOK;
            goto EXIT;
        }
    }

    /* 分析图片数据 */
    if (pchData == nullptr)
    {
        nRet = pAlgorithm->DetectHumanBgr(stMediaDataInfo.pchData, stMediaDataInfo.nDataSize);
    }
    else
    {
        nRet = pAlgorithm->DetectHumanBgr(pchData, nDataSize);
    }

    if (nRet < 0)
    {
        dlog(LOG_ERROR, "算法分析失败");
        enRetCode = NOK;
        goto EXIT;
    }

    /* 获取分析到的人数 */
    pAlgorithm->GetPepleNum(stOutInfo.nNumber);

    /* 获取分析到的人坐标 */
    pAlgorithm->GetPeplePoints(vecPos);

    /* 增强算法 */
    pAlgorithm->MultiFrameResultFusionAlgorithm(vecPos, vecPos);

    /* 换算坐标信息 */
    nVecSize = vecPos.size() / DATA_GROUP_SIZE;


    for (nIndex = 0; nIndex < nVecSize; nIndex++)
    {
        if (vecPos.size() > ((nIndex * DATA_GROUP_SIZE) + 1))
        {
            stPos.fX1 = vecPos[nIndex * DATA_GROUP_SIZE];
            stPos.fY1 = vecPos[(nIndex * DATA_GROUP_SIZE) + 1];

            stOutInfo.vstPos.push_back(stPos);
        }
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
