/*
 * @FilePath     : BridgeCollapseAlgorithm.cpp
 * @Author       : 严泽辉 yanzeh@kfb.cn
 * @Date         : 2024-03-08 10:56:16
 * @LastEditors  : 严泽辉 yanzeh@kfb.cn
 * @LastEditTime : 2024-03-14 15:31:37
 * @Description  : 桥梁坍塌分析模块
 */
#include "BridgeCollapseAlgorithm.hpp"

#include "BridgeFracture.h"
#include "dlog.h"
#include "ImageProcessor.hpp"


using namespace BDGA_NS;
using namespace BRIDGEFRACTURE_NS;


/* 一组数据的大小 */
#define DATA_GROUP_SIZE 4

CBridgeCollapseAlgorithm::CBridgeCollapseAlgorithm(InParam_S stInParam)
    : CBridgeAlgorithmBase(stInParam)
{
    if (nullptr == m_pAlgorithm)
    {
        if (m_stInParam.stExParam.nDistanceThreshold == -1 ||
            m_stInParam.stExParam.nAngleThreshold == -1 ||
            m_stInParam.stExParam.nBrokenBridgeNumThreshold == -1)
        {
            m_pAlgorithm = new CBridgeFracture();
        }
        else
        {
            m_pAlgorithm = new CBridgeFracture(m_stInParam.stExParam.nDistanceThreshold,
                                               m_stInParam.stExParam.nAngleThreshold,
                                               m_stInParam.stExParam.nBrokenBridgeNumThreshold);
        }
    }
}

CBridgeCollapseAlgorithm::~CBridgeCollapseAlgorithm()
{
    if (m_pAlgorithm)
    {
        delete static_cast<CBridgeFracture*>(m_pAlgorithm);
        m_pAlgorithm = nullptr;
    }
}

/* 分析数据 */
BlError_E CBridgeCollapseAlgorithm::dataAnalysis(
    MediaDataInfo_S&  stMediaDataInfo,
    AnalyzerResult_S& stOutInfo)
{
    if (nullptr == stMediaDataInfo.pchData)
    {
        dlog(LOG_ERROR, "传入参数为空");
        return ERR_IN_PARAM_NULL;
    }

    CBridgeFracture* pAlgorithm = static_cast<CBridgeFracture*>(m_pAlgorithm);
    if (nullptr == pAlgorithm)
    {
        dlog(LOG_ERROR, "未初始化算法类");
        return ERR_UNINIT;
    }


    stOutInfo.clear();

    BlError_E enRetCode = OK;

    std::vector<std::vector<int>> vBridgeFractureAreas;
    std::vector<int>              vBridgeLines;

    AnalyzerResult_S stItemInfo;
    stItemInfo.clear();

    /* 转化后的数据 */
    cv::Mat image;

    /* 图片处理器 */
    CImageProcessor imageProcessor;

    /* 判断数据是否符合要求 */
    if (stMediaDataInfo.enFormat != RGB888)
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
                                                  stMediaDataInfo.nWidth,
                                                  stMediaDataInfo.nHeight,
                                                  image);
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
    else
    {
        image = cv::Mat(stMediaDataInfo.nHeight, stMediaDataInfo.nWidth, CV_8UC3, (void*)stMediaDataInfo.pchData);
    }

    // cv::imwrite("222.jpg", image);

    /* 算法分析 */
    if (pAlgorithm->detectBridgeFracture(image) == false)
    {
        dlog(LOG_ERROR, "算法分析失败");
        enRetCode = NOK;
        goto EXIT;
    }

    /* 获取数据 */
    pAlgorithm->getBridgeFractureAreas(vBridgeFractureAreas);

    /* 组装数据 */
    for (auto item : vBridgeFractureAreas)
    {
        if (item.size() >= DATA_GROUP_SIZE)
        {
            PosInfo_S stPos;
            stPos.fX1 = item.at(0);
            stPos.fY1 = item.at(1);
            stPos.fX2 = item.at(2);
            stPos.fY2 = item.at(3);

            stOutInfo.listErrorPos.push_back(stPos);
        }
    }

    /* 获取辅助直线数据 */
    pAlgorithm->getBridgeLines(vBridgeLines);

    /* 组装数据 */
    for (int i = 0; i < vBridgeLines.size(); i += DATA_GROUP_SIZE)
    {
        PosInfo_S stPos;
        stPos.fX1 = vBridgeLines.at(i);
        stPos.fY1 = vBridgeLines.at(i + 1);
        stPos.fX2 = vBridgeLines.at(i + 2);
        stPos.fY2 = vBridgeLines.at(i + 3);

        stOutInfo.listLinePos.push_back(stPos);
    }


EXIT:
    stMediaDataInfo.free();

    return enRetCode;
}

/* 设置桥梁基准线 */
BlError_E BDGA_NS::CBridgeCollapseAlgorithm::setDatumLine(std::list<PosInfo_S> listPos)
{
    CBridgeFracture* pAlgorithm = static_cast<CBridgeFracture*>(m_pAlgorithm);
    if (nullptr == pAlgorithm)
    {
        dlog(LOG_ERROR, "未初始化算法类");
        return ERR_UNINIT;
    }

    std::vector<std::vector<int>> vvnInputLinePoints;
    vvnInputLinePoints.clear();

    for (auto item : listPos)
    {
        std::vector<int> vnInputLinePoints;
        vnInputLinePoints.clear();
        vnInputLinePoints.push_back(item.fX1);
        vnInputLinePoints.push_back(item.fY1);
        vnInputLinePoints.push_back(item.fX2);
        vnInputLinePoints.push_back(item.fY2);

        vvnInputLinePoints.push_back(vnInputLinePoints);
    }

    if (pAlgorithm->setData(vvnInputLinePoints))
    {
        return OK;
    }

    return NOK;
}
