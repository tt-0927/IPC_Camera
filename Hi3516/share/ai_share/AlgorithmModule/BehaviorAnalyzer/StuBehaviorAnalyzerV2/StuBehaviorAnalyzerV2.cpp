/*
 * @FilePath     : StuBehaviorAnalyzerV2.cpp
 * @Author       : yanzeh yanzeh@kfb.cn
 * @Date         : 2024-01-08 15:13:54
 * @LastEditors  : 严泽辉 yanzeh@kfb.cn
 * @LastEditTime : 2024-01-26 10:17:37
 * @Description  : 学生分析第一代行为分析模块
 */
#include "StuBehaviorAnalyzerV2.hpp"

#include "dlog.h"
#include "ImageProcessor.hpp"
#include "rk_student_behavior_v2.h"

using namespace BA_NS;

#define STU_BEHAVIOR_ANALYSE_WIDTH  640
#define STU_BEHAVIOR_ANALYSE_HEIGHT 640

/* 一组数据的大小 */
#define DATA_GROUP_SIZE 6

CStuBehaviorAnalyzerV2::CStuBehaviorAnalyzerV2(BehaviorAnalyzerInParam_S stInParam)
    : CBehaviorAnalyzerBase(stInParam)
{
    if (nullptr == m_pAlgorithm)
    {
        m_pAlgorithm = new RK_STUDENT_BEHAVIOR_V2(
            const_cast<char*>(stInParam.stNeedParam.strModelPath.c_str()),
            STU_BEHAVIOR_ANALYSE_WIDTH,
            STU_BEHAVIOR_ANALYSE_HEIGHT,
            3);

        RK_STUDENT_BEHAVIOR_V2* pAlgorithm = static_cast<RK_STUDENT_BEHAVIOR_V2*>(m_pAlgorithm);

        pAlgorithm->fBoxThreshold = stInParam.stExParam.fConfidenceThreshold;
    }
}

CStuBehaviorAnalyzerV2::~CStuBehaviorAnalyzerV2()
{
    if (m_pAlgorithm)
    {
        delete static_cast<RK_STUDENT_BEHAVIOR_V2*>(m_pAlgorithm);
        m_pAlgorithm = nullptr;
    }
}

/* 分析数据 */
BlError_E CStuBehaviorAnalyzerV2::dataAnalysis(
    MediaDataInfo_S&                     stMediaDataInfo,
    std::list<BehaviorAnalyzerResult_S>& listOutInfo)
{
    if (nullptr == stMediaDataInfo.pchData)
    {
        dlog(LOG_ERROR, "传入参数为空");
        return ERR_IN_PARAM_NULL;
    }

    RK_STUDENT_BEHAVIOR_V2* pAlgorithm = static_cast<RK_STUDENT_BEHAVIOR_V2*>(m_pAlgorithm);
    if (nullptr == pAlgorithm)
    {
        dlog(LOG_ERROR, "未初始化算法类");
        return ERR_UNINIT;
    }

    int       nRet      = 0;
    BlError_E enRetCode = OK;

    std::vector<float> vecPos;
    vecPos.clear();

    int                      nVecSize;
    BehaviorAnalyzerResult_S stItemInfo;
    stItemInfo.clear();

    /* 图片处理器 */
    CImageProcessor imageProcessor;

    /* 转化后的数据 */
    char* pchData   = nullptr;
    int   nDataSize = 0;

    /* 判断数据是否符合要求 */
    if (stMediaDataInfo.nWidth != STU_BEHAVIOR_ANALYSE_WIDTH ||
        stMediaDataInfo.nHeight != STU_BEHAVIOR_ANALYSE_HEIGHT ||
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
                                                  STU_BEHAVIOR_ANALYSE_WIDTH,
                                                  STU_BEHAVIOR_ANALYSE_HEIGHT,
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

    /* 送分析 */
    if (pchData == nullptr)
    {
        nRet = pAlgorithm->DetectFaceRgb(stMediaDataInfo.pchData, vecPos);
    }
    else
    {
        nRet = pAlgorithm->DetectFaceRgb(pchData, vecPos);
    }

    if (nRet < 0)
    {
        dlog(LOG_ERROR, "算法分析失败");
        enRetCode = NOK;
        goto EXIT;
    }

    /* 组装数据 */
    nVecSize = vecPos.size() / DATA_GROUP_SIZE;


    listOutInfo.clear();
    for (int i = 0; i < nVecSize; i++)
    {
        enRetCode = OK;
        if (vecPos.size() <= ((i * DATA_GROUP_SIZE) + 5))
        {
            break;
        }
        stItemInfo.stPos.fX1        = vecPos[i * DATA_GROUP_SIZE];
        stItemInfo.stPos.fY1        = vecPos[(i * DATA_GROUP_SIZE) + 1];
        stItemInfo.stPos.fX2        = vecPos[(i * DATA_GROUP_SIZE) + 2];
        stItemInfo.stPos.fY2        = vecPos[(i * DATA_GROUP_SIZE) + 3];
        stItemInfo.fConfidenceScore = vecPos[(i * DATA_GROUP_SIZE) + 4];
        /* 进位强转操作，防止0.999强转时进位为0的问题 */
        int nBehavior               = int(vecPos[(i * DATA_GROUP_SIZE) + 5] + 0.5);
        switch (nBehavior)
        {
            case 0:
            {
                stItemInfo.enBehavior = LOW_HEAD_READING;
                break;
            }
            case 1:
            {
                stItemInfo.enBehavior = RAISE_HEAD_LISTENING;
                break;
            }
            case 2:
            {
                stItemInfo.enBehavior = TURN_HEAD;
                break;
            }
            case 3:
            {
                stItemInfo.enBehavior = RAISE_HAND;
                break;
            }

            default:
            {
                // dlog(LOG_ERROR, "V2分析出来的行为异常 [%d]", nBehavior);
                enRetCode = NOK;
                break;
            }
        }

        if (enRetCode < OK)
        {
            continue;
        }

        listOutInfo.push_back(stItemInfo);
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
