/*
 * @FilePath     : TrackerAlgorithmExtern.hpp
 * @Author       : yanzeh yanzeh@kfb.cn
 * @Date         : 2024-01-11 18:53:26
 * @LastEditors  : 严泽辉 yanzeh@kfb.cn
 * @LastEditTime : 2024-02-23 18:33:11
 * @Description  :
 */
#pragma once

#include <iostream>
#include <vector>

namespace TA_NS
{
    /* 算法类型 */
    typedef enum _TrackerAlgorithmType_
    {
        TA_V1 = 0, /* 第一代算法 */
    } TrackerAlgorithmType_E;

    /* 必需参数 */
    typedef struct _TrackerAlgorithmNeedParam_
    {
        TrackerAlgorithmType_E enType;              /* 使用的算法类型 */
        std::string            strFeatureModelPath; /* 测算法模型路径 */
        std::string            strHeadModelPath;    /* 测算法模型路径 */

        /* 构造函数 */
        _TrackerAlgorithmNeedParam_()
        {
            enType = TA_V1;
            strFeatureModelPath.clear();
            strHeadModelPath.clear();
        }

        void clear()
        {
            enType = TA_V1;
            strFeatureModelPath.clear();
            strHeadModelPath.clear();
        }
    } TrackerAlgorithmNeedParam_S;

    /* 额外参数 */
    typedef struct _TrackerAlgorithmExParam_
    {
        bool  bAutoConvert;         /* 数据格式不正确时，是否应自动进行转换 */
        int   nMaxQueue;            /* 数据队列最大值 */
        float fConfidenceThreshold; /* 置信度分数阈值 */
        float fSimilarityThreshold; /* 相似分数阈值 */

        /* 构造函数 */
        _TrackerAlgorithmExParam_()
        {
            bAutoConvert         = false;
            nMaxQueue            = 50;
            fConfidenceThreshold = 0.6;
            fSimilarityThreshold = 0.7;
        }

        void clear()
        {
            bAutoConvert         = false;
            nMaxQueue            = 50;
            fConfidenceThreshold = 0.6;
            fSimilarityThreshold = 0.7;
        }

    } TrackerAlgorithmExParam_S;

    /* 参数结构体 */
    typedef struct _TrackerAlgorithmInParam_
    {
        TrackerAlgorithmNeedParam_S stNeedParam; /* 必需参数 */
        TrackerAlgorithmExParam_S   stExParam;   /* 额外参数 */

        void clear()
        {
            stNeedParam.clear();
            stExParam.clear();
        }
    } TrackerAlgorithmInParam_S;

    /* 数据格式 */
    typedef enum _MediaDataFormat_
    {
        RGB888 = 0,
        BGR888,
        RGBA8888
    } MediaDataFormat_E;

    /* 坐标信息 */
    typedef struct _PosInfo_
    {
        float fX1; /* x坐标 */
        float fY1; /* y坐标 */
        float fX2; /* x坐标 */
        float fY2; /* y坐标 */

        void clear()
        {
            fX1 = 0.0f;
            fY1 = 0.0f;
            fX2 = 0.0f;
            fY2 = 0.0f;
        }
    } PosInfo_S;

    /* 数据信息 */
    typedef struct _MediaDataInfo_
    {
        /* 必填 */
        char*             pchData;   /* 数据指针 */
        int               nDataSize; /* 数据指针空间大小 */
        int               nWidth;    /* 数据图片宽 */
        int               nHeight;   /* 数据图片高 */
        MediaDataFormat_E enFormat;  /* 数据格式 */

        PosInfo_S stTracker;

        /* 选填 */
        void (*freeDataFunc)(void*);  /* 释放数据的函数指针, 填空，不释放pchData */

        void* pFreeParam;             /* 需要用户释放的参数 */
        void (*freeParamFunc)(void*); /* 释放的参数函数指针 */

        void* pUserData;              /* 用户的参数 */
        void (*userFreeFunc)(void*);  /* 释放用户参数的函数指针 */

        /* 构造函数 */
        _MediaDataInfo_()
        {
            nDataSize = 0;
            nWidth    = 0;
            nHeight   = 0;
            enFormat  = RGB888;

            stTracker.clear();

            freeDataFunc  = nullptr;
            pchData       = nullptr;
            freeParamFunc = nullptr;
            pFreeParam    = nullptr;
            userFreeFunc  = nullptr;
            pUserData     = nullptr;
        }

        void clear()
        {
            nDataSize = 0;
            nWidth    = 0;
            nHeight   = 0;
            enFormat  = RGB888;

            stTracker.clear();

            freeDataFunc  = nullptr;
            pchData       = nullptr;
            freeParamFunc = nullptr;
            pFreeParam    = nullptr;
            userFreeFunc  = nullptr;
            pUserData     = nullptr;
        }

        void free()
        {
            if (freeDataFunc != nullptr && pchData != nullptr)
            {
                freeDataFunc(pchData);
            }

            if (pFreeParam != nullptr && freeParamFunc != nullptr)
            {
                freeParamFunc(pFreeParam);
            }

            if (pUserData != nullptr && userFreeFunc != nullptr)
            {
                userFreeFunc(pUserData);
            }
        }
    } MediaDataInfo_S;

    /* 跟踪人物信息的结果信息 */
    typedef struct _TrackerAlgorithmResult_
    {
        float fConfidenceScore;               /* 置信度分数 */

        std::vector<PosInfo_S> vstTrackerPos; /* 跟踪坐标 */
        std::vector<PosInfo_S> vstPos;        /* 所有人的坐标 */

        void clear()
        {
            fConfidenceScore = 0.0;
            vstTrackerPos.clear();
            vstPos.clear();
        }

        void print() const
        {
            std::cout << "\n跟踪处理结果信息:=============" << std::endl;
            std::cout << "置信度分数:" << fConfidenceScore << std::endl;
            std::cout << "跟踪坐标:";
            for (auto stPos : vstPos)
            {
                std::cout << "[" << stPos.fX1 << "," << stPos.fY1 << "] ";
                std::cout << "[" << stPos.fX2 << "," << stPos.fY2 << "]";
            }
            std::cout << std::endl;

            std::cout << "所有人的坐标:";
            for (auto stPos : vstPos)
            {
                std::cout << "[" << stPos.fX1 << "," << stPos.fY1 << "] ";
                std::cout << "[" << stPos.fX2 << "," << stPos.fY2 << "]";
            }
            std::cout << std::endl;
            std::cout << "end:=============" << std::endl;
        }
    } TrackerAlgorithmResult_S;

}    // namespace TA_NS
