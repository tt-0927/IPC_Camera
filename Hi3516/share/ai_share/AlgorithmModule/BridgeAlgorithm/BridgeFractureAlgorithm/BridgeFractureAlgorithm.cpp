/*
 * @FilePath     : BridgeFractureAlgorithm.cpp
 * @Author       : yanzeh yanzeh@kfb.cn
 * @Date         : 2024-01-08 15:13:54
 * @LastEditors  : 严泽辉 yanzeh@kfb.cn
 * @LastEditTime : 2024-03-14 16:01:17
 * @Description  : 桥梁裂缝检测
 */
#include "BridgeFractureAlgorithm.hpp"

#include "dlog.h"
#include "ImageProcessor.hpp"
#include "rk_bridgefracture_detect.h"

using namespace BDGA_NS;
using namespace BridgeFracture_NS;

/* 一组数据的大小 */
#define DATA_GROUP_SIZE 6

#define ANALYSE_WIDTH  640
#define ANALYSE_HEIGHT 640

CBridgeFractureAlgorithm::CBridgeFractureAlgorithm(InParam_S stInParam)
    : CBridgeAlgorithmBase(stInParam)
{
    if (nullptr == m_pAlgorithm)
    {
        m_pAlgorithm = new CRkBridgeFracture(const_cast<char*>(m_stInParam.stNeedParam.strModelPath.c_str()));

        CRkBridgeFracture* pAlgorithm = static_cast<CRkBridgeFracture*>(m_pAlgorithm);

        pAlgorithm->fBoxThreshold = stInParam.stExParam.fConfidenceThreshold;
    }
}

CBridgeFractureAlgorithm::~CBridgeFractureAlgorithm()
{
    if (m_pAlgorithm)
    {
        delete static_cast<CRkBridgeFracture*>(m_pAlgorithm);
        m_pAlgorithm = nullptr;
    }
}

/* 分析数据 */
BlError_E BDGA_NS::CBridgeFractureAlgorithm::dataAnalysis(
    MediaDataInfo_S&  stMediaDataInfo,
    AnalyzerResult_S& stOutInfo)
{
    if (nullptr == stMediaDataInfo.pchData)
    {
        dlog(LOG_ERROR, "传入参数为空");
        return ERR_IN_PARAM_NULL;
    }

    CRkBridgeFracture* pAlgorithm = static_cast<CRkBridgeFracture*>(m_pAlgorithm);
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
    cv::Mat image;

    /* 判断数据是否符合要求 */
    if (stMediaDataInfo.nWidth != ANALYSE_WIDTH ||
        stMediaDataInfo.nHeight != ANALYSE_HEIGHT ||
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
                                                  ANALYSE_WIDTH,
                                                  ANALYSE_HEIGHT,
                                                  image);
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
    else
    {
        image = cv::Mat(stMediaDataInfo.nHeight, stMediaDataInfo.nWidth, CV_8UC3, (void*)stMediaDataInfo.pchData);
    }

    /* 分析图片数据 */
    pAlgorithm->DetectRkBridgeRgb(image, vecPos);

    /* 换算坐标信息 */
    nVecSize = vecPos.size() / DATA_GROUP_SIZE;


    for (nIndex = 0; nIndex < nVecSize; nIndex++)
    {
        stPos.fX1 = vecPos[nIndex * DATA_GROUP_SIZE];
        stPos.fY1 = vecPos[(nIndex * DATA_GROUP_SIZE) + 1];
        stPos.fX2 = vecPos[(nIndex * DATA_GROUP_SIZE) + 2];
        stPos.fY2 = vecPos[(nIndex * DATA_GROUP_SIZE) + 3];

        stOutInfo.listErrorPos.push_back(stPos);
    }

EXIT:
    stMediaDataInfo.free();

    return enRetCode;
}
