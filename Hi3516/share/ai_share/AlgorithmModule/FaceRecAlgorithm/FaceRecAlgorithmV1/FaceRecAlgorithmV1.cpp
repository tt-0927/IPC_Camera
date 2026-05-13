/*
 * @FilePath     : FaceRecAlgorithmV1.cpp
 * @Author       : yanzeh yanzeh@kfb.cn
 * @Date         : 2024-01-08 15:13:54
 * @LastEditors  : 严泽辉 yanzeh@kfb.cn
 * @LastEditTime : 2024-08-09 16:29:31
 * @Description  : 第一代人脸识别模块
 */
#include "FaceRecAlgorithmV1.hpp"

#include "dlog.h"
#include "ImageProcessor.hpp"
#include "rk_retinaface_facenet128.h"

#define FACEREC_ANALYSE_WIDTH  1920
#define FACEREC_ANALYSE_HEIGHT 1024

using namespace FR_NS;

CFaceRecAlgorithmV1::CFaceRecAlgorithmV1(FaceRecInParam_S stInParam)
    : CFaceRecAlgorithmBase(stInParam)
{
    /* 人脸识别辅助类 */
    if (nullptr == m_pRetinafaceFacenet)
    {
        m_pRetinafaceFacenet = new RETINAFACE_FACENET();
    }

    /* 人脸检测类 */
    if (nullptr == m_pFaceDetect)
    {
        m_pFaceDetect = new RK_FACES_DETECT(const_cast<char*>(stInParam.stNeedParam.strFaceDetectionModelPath.c_str()));

        RK_FACES_DETECT* pFaceDetect = static_cast<RK_FACES_DETECT*>(m_pFaceDetect);

        pFaceDetect->stDetectParam.fConfidentThreshold = stInParam.stExParam.fConfidenceThreshold;
    }

    /* 人脸特征提取 */
    if (nullptr == m_pFeatureNet)
    {
        m_pFeatureNet = new RK_FACE_FEATURE(const_cast<char*>(stInParam.stNeedParam.strFaceFeatureExtractionModelPath.c_str()));
    }
}

CFaceRecAlgorithmV1::~CFaceRecAlgorithmV1()
{
    RETINAFACE_FACENET* pRetinafaceFacenet = static_cast<RETINAFACE_FACENET*>(m_pRetinafaceFacenet);
    RK_FACES_DETECT*    pFaceDetect        = static_cast<RK_FACES_DETECT*>(m_pFaceDetect);
    RK_FACE_FEATURE*    pFeatureNet        = static_cast<RK_FACE_FEATURE*>(m_pFeatureNet);

    /* 人脸识别辅助类 */
    if (pRetinafaceFacenet)
    {
        delete pRetinafaceFacenet;
        m_pRetinafaceFacenet = nullptr;
    }

    /* 人脸检测类 */
    if (pFaceDetect)
    {
        delete pFaceDetect;
        m_pFaceDetect = nullptr;
    }

    /* 人脸特征提取 */
    if (pFeatureNet)
    {
        delete pFeatureNet;
        m_pFeatureNet = nullptr;
    }
}

/* 获取人脸特征信息 */
BlError_E CFaceRecAlgorithmV1::get_facialFeatures(
    MediaDataInfo_S                stMediaDataInfo,
    std::list<std::vector<float>>& listOutData)
{
    if (nullptr == stMediaDataInfo.pchData)
    {
        dlog(LOG_ERROR, "传入参数为空");
        return ERR_IN_PARAM_NULL;
    }

    RETINAFACE_FACENET* pRetinafaceFacenet = static_cast<RETINAFACE_FACENET*>(m_pRetinafaceFacenet);
    RK_FACES_DETECT*    pFaceDetect        = static_cast<RK_FACES_DETECT*>(m_pFaceDetect);
    RK_FACE_FEATURE*    pFeatureNet        = static_cast<RK_FACE_FEATURE*>(m_pFeatureNet);
    if (nullptr == pRetinafaceFacenet ||
        nullptr == pFaceDetect ||
        nullptr == pFeatureNet)
    {
        dlog(LOG_ERROR, "未初始化");
        return ERR_UNINIT;
    }

    int nRet = 0;

    /* 分析到的特征数据 */
    std::vector<PEOPLEFEATURES> vOneFeature;
    vOneFeature.clear();

    /* 提取图片中的人脸信息 */
    nRet = pRetinafaceFacenet->RetinafaceFacenetBgr(
        stMediaDataInfo.pchData,
        *pFaceDetect,
        *pFeatureNet,
        vOneFeature);
    if (nRet < 0)
    {
        dlog(LOG_ERROR, "提取图片中的人脸信息-失败");
        return NOK;
    }

    /* 判断图片中是否识别到人脸 */
    if (vOneFeature.empty())
    {
        dlog(LOG_ERROR, "图片中未识别到人脸");
        return NOK;
    }

    listOutData.clear();
    PEOPLEFEATURES stInfo;
    int            nMaxArea = 0;
    int            nArea    = 0;
    int            nIndex   = 0;
    int            i        = 0;
    for (auto item : vOneFeature)
    {
        nArea = item.fBoxs[2] - item.fBoxs[0] * item.fBoxs[3] - item.fBoxs[1];

        if (nMaxArea < nArea)
        {
            nMaxArea = nArea;
            nIndex   = i;
        }

        i++;
    }

    std::vector<float> featureVector(vOneFeature.at(nIndex).fFeatures, vOneFeature.at(nIndex).fFeatures + (sizeof(vOneFeature.at(nIndex).fFeatures) / sizeof(float)));
    listOutData.push_back(featureVector);

    return OK;
}

/* 获取图片中的人脸特征信息 */
BlError_E FR_NS::CFaceRecAlgorithmV1::get_facialFeatures(cv::Mat inMat, std::list<std::vector<float>>& listOutData)
{
    if (inMat.empty())
    {
        dlog(LOG_ERROR, "传入参数为空");
        return ERR_IN_PARAM_NULL;
    }

    RETINAFACE_FACENET* pRetinafaceFacenet = static_cast<RETINAFACE_FACENET*>(m_pRetinafaceFacenet);
    RK_FACES_DETECT*    pFaceDetect        = static_cast<RK_FACES_DETECT*>(m_pFaceDetect);
    RK_FACE_FEATURE*    pFeatureNet        = static_cast<RK_FACE_FEATURE*>(m_pFeatureNet);
    if (nullptr == pRetinafaceFacenet ||
        nullptr == pFaceDetect ||
        nullptr == pFeatureNet)
    {
        dlog(LOG_ERROR, "未初始化");
        return ERR_UNINIT;
    }

    int nRet = 0;

    /* 分析到的特征数据 */
    std::vector<PEOPLEFEATURES> vOneFeature;
    vOneFeature.clear();

    /* 提取图片中的人脸信息 */
    nRet = pRetinafaceFacenet->RetinafaceFacenetBgr(
        inMat,
        *pFaceDetect,
        *pFeatureNet,
        vOneFeature);
    if (nRet < 0)
    {
        dlog(LOG_ERROR, "提取图片中的人脸信息-失败");
        return NOK;
    }

    /* 判断图片中是否识别到人脸 */
    if (vOneFeature.empty())
    {
        dlog(LOG_ERROR, "图片中未识别到人脸");
        return NOK;
    }

    listOutData.clear();
    PEOPLEFEATURES stInfo;
    int            nMaxArea = 0;
    int            nArea    = 0;
    int            nIndex   = 0;
    int            i        = 0;
    for (auto item : vOneFeature)
    {
        nArea = item.fBoxs[2] - item.fBoxs[0] * item.fBoxs[3] - item.fBoxs[1];

        if (nMaxArea < nArea)
        {
            nMaxArea = nArea;
            nIndex   = i;
        }

        i++;
    }

    std::vector<float> featureVector(vOneFeature.at(nIndex).fFeatures, vOneFeature.at(nIndex).fFeatures + (sizeof(vOneFeature.at(nIndex).fFeatures) / sizeof(float)));
    listOutData.push_back(featureVector);

    return OK;
}

/* 分析数据 */
BlError_E CFaceRecAlgorithmV1::dataAnalysis(
    MediaDataInfo_S                     stMediaDataInfo,
    std::list<FaceRecognitionResult_S>& listOutInfo)
{

    if (nullptr == stMediaDataInfo.pchData)
    {
        dlog(LOG_ERROR, "传入参数为空");
        return ERR_IN_PARAM_NULL;
    }

    RETINAFACE_FACENET* pRetinafaceFacenet = static_cast<RETINAFACE_FACENET*>(m_pRetinafaceFacenet);
    RK_FACES_DETECT*    pFaceDetect        = static_cast<RK_FACES_DETECT*>(m_pFaceDetect);
    RK_FACE_FEATURE*    pFeatureNet        = static_cast<RK_FACE_FEATURE*>(m_pFeatureNet);
    if (nullptr == pRetinafaceFacenet ||
        nullptr == pFaceDetect ||
        nullptr == pFeatureNet)
    {
        dlog(LOG_ERROR, "未初始化");
        return ERR_UNINIT;
    }

    BlError_E enRetCode = OK;
    int       nRet      = 0;

    listOutInfo.clear();

    /* 分析到的特征数据 */
    std::vector<PEOPLEFEATURES> vOneFeature;
    vOneFeature.clear();

    std::list<FaceDataInfo_S> listFaceInfo;
    listFaceInfo.clear();

    /* 图片处理器 */
    CImageProcessor imageProcessor;

    /* 转化后的数据 */
    char* pchData   = nullptr;
    int   nDataSize = 0;

    float fSimilarityScoreMax = 0.0;

    FaceRecognitionResult_S stItem;
    stItem.clear();

    /* 判断数据是否符合要求 */
    if (stMediaDataInfo.nWidth != FACEREC_ANALYSE_WIDTH ||
        stMediaDataInfo.nHeight != FACEREC_ANALYSE_HEIGHT ||
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
                                                  FACEREC_ANALYSE_WIDTH,
                                                  FACEREC_ANALYSE_HEIGHT,
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

    /* 提取图片中的人脸信息 */
    if (pchData == nullptr)
    {
        nRet = pRetinafaceFacenet->RetinafaceFacenetBgr(
            stMediaDataInfo.pchData,
            *pFaceDetect,
            *pFeatureNet,
            vOneFeature);
    }
    else
    {
        nRet = pRetinafaceFacenet->RetinafaceFacenetBgr(
            pchData,
            *pFaceDetect,
            *pFeatureNet,
            vOneFeature);
    }

    if (nRet < 0)
    {
        dlog(LOG_ERROR, "提取图片中的人脸信息-失败");
        enRetCode = NOK;
        goto EXIT;
    }

    /* 判断图片中是否识别到人脸 */
    if (vOneFeature.empty())
    {
        dlog(LOG_ERROR, "图片中未识别到人脸");
        enRetCode = NOK;
        goto EXIT;
    }

    /* 获取数据库人脸信息 */
    enRetCode = getAllFaceInfo(listFaceInfo);
    if (enRetCode < OK)
    {
        dlog(LOG_ERROR, "获取数据库人脸信息-失败");
        goto EXIT;
    }

    /* 判断数据库中是否存在人脸信息 */
    if (listFaceInfo.empty())
    {
        dlog(LOG_ERROR, "数据库中不存在人脸信息");
        enRetCode = NOK;
        goto EXIT;
    }

    /* 比较数据 */
    int i = 0;
    for (auto item : vOneFeature)
    {
        i++;
        dlog(LOG_TRACE, "第%d个人脸比较...%ld", i, listFaceInfo.size());
        fSimilarityScoreMax = 0.0f;
        for (auto itemDb : listFaceInfo)
        {
            /* 计算两个人脸的相似值 */
            stItem.fSimilarityScore = pRetinafaceFacenet->CosineSimilarity(
                itemDb.vfData.data(),
                item.fFeatures,
                sizeof(item.fFeatures) / sizeof(float));

            if (m_stInParam.stExParam.fSimilarityThreshold <= stItem.fSimilarityScore)
            {
                dlog(LOG_TRACE, "人脸识别预置符合要求[%f]-%s", stItem.fSimilarityScore, itemDb.strName.c_str());

                if (fSimilarityScoreMax < stItem.fSimilarityScore)
                {
                    /* 填充数据，插入链表 */
                    stItem.nFaceID       = itemDb.nCardId;
                    stItem.strPersonName = itemDb.strName;
                    stItem.stPos.fX1     = item.fBoxs[0];
                    stItem.stPos.fY1     = item.fBoxs[1];
                    stItem.stPos.fX2     = item.fBoxs[2];
                    stItem.stPos.fY2     = item.fBoxs[3];

                    fSimilarityScoreMax = stItem.fSimilarityScore;
                }
            }
            else
            {
                dlog(LOG_TRACE, "人脸识别预置过小[%f]", stItem.fSimilarityScore);
            }
        }


        if (fSimilarityScoreMax != 0.0f)
        {
            stItem.fSimilarityScore = fSimilarityScoreMax;
            listOutInfo.push_back(stItem);
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
