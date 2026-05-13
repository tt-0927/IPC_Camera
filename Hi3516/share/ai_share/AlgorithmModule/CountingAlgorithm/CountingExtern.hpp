/*
 * @FilePath     : CountingExtern.hpp
 * @Author       : yanzeh yanzeh@kfb.cn
 * @Date         : 2024-01-11 13:49:38
 * @LastEditors  : yanzeh yanzeh@kfb.cn
 * @LastEditTime : 2024-01-23 15:10:00
 * @Description  : 人数统计模块对外文件
 */
#pragma once

#include <iostream>
#include <vector>

namespace CA_NS
{
    /* 算法类型 */
    typedef enum _CountingAlgorithmType_
    {
        CA_V1 = 0, /* 第一代算法 */
        CA_V2,     /* 第二代算法 */
    } CountingAlgorithmType_E;

    /* 必需参数 */
    typedef struct _CountingAnalyzerNeedParam_
    {
        CountingAlgorithmType_E enType;       /* 使用的算法类型 */
        std::string             strModelPath; /* 测算法模型路径 */

        /* 构造函数 */
        _CountingAnalyzerNeedParam_()
        {
            enType = CA_V1;
            strModelPath.clear();
        }

        void clear()
        {
            enType = CA_V1;
            strModelPath.clear();
        }
    } CountingAnalyzerNeedParam_S;

    /* 额外参数 */
    typedef struct _CountingAnalyzerExParam_
    {
        bool  bAutoConvert;         /* 数据格式不正确时，是否应自动进行转换 */
        int   nMaxQueue;            /* 数据队列最大值 */
        float fConfidenceThreshold; /* 置信度分数阈值 */

        /* 构造函数 */
        _CountingAnalyzerExParam_()
        {
            bAutoConvert         = false;
            nMaxQueue            = 50;
            fConfidenceThreshold = 0.75;
        }

        void clear()
        {
            bAutoConvert         = false;
            nMaxQueue            = 50;
            fConfidenceThreshold = 0.75;
        }

    } CountingAnalyzerExParam_S;

    /* 参数结构体 */
    typedef struct _CountingAnalyzerInParam_
    {
        CountingAnalyzerNeedParam_S stNeedParam; /* 必需参数 */
        CountingAnalyzerExParam_S   stExParam;   /* 额外参数 */

        void clear()
        {
            stNeedParam.clear();
            stExParam.clear();
        }
    } CountingAnalyzerInParam_S;

    /* 数据格式 */
    typedef enum _MediaDataFormat_
    {
        RGB888 = 0,
        BGR888,
        RGBA8888
    } MediaDataFormat_E;

    /* 数据信息 */
    typedef struct _MediaDataInfo_
    {
        /* 必填 */
        char*             pchData;   /* 数据指针 */
        int               nDataSize; /* 数据指针空间大小 */
        int               nWidth;    /* 数据图片宽 */
        int               nHeight;   /* 数据图片高 */
        MediaDataFormat_E enFormat;  /* 数据格式 */

        /* 选填 */
        void (*freeDataFunc)(char*);  /* 释放数据的函数指针, 填空，不释放pchData */

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

    /* 人数统计结果结果结果信息 */
    typedef struct _CountingAnalyzerResult_

    {
        int   nNumber;                 /* 人数 */
        float fConfidenceScore;        /* 置信度分数 */

        std::vector<PosInfo_S> vstPos; /* 坐标 */

        void clear()
        {
            fConfidenceScore = 0.0;
            vstPos.clear();
        }

        void print() const
        {
            std::cout << "\n人数统计处理结果信息:=============" << std::endl;
            std::cout << "置信度分数:" << fConfidenceScore << std::endl;
            std::cout << "人数:" << nNumber << std::endl;
            std::cout << "坐标:";
            for (auto stPos : vstPos)
            {
                std::cout << "[" << stPos.fX1 << "," << stPos.fY1 << "] ";
                std::cout << "[" << stPos.fX2 << "," << stPos.fY2 << "]";
            }
            std::cout << std::endl;
            std::cout << "end:=============" << std::endl;
        }
    } CountingAnalyzerResult_S;

}    // namespace CA_NS