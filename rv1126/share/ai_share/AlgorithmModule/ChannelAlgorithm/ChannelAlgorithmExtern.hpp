/*
 * @FilePath     : ChannelAlgorithmExtern.hpp
 * @Author       : 严泽辉 yanzeh@kfb.cn
 * @Date         : 2024-03-08 10:36:45
 * @LastEditors  : 严泽辉 yanzeh@kfb.cn
 * @LastEditTime : 2024-03-09 11:51:17
 * @Description  : 航道分析模块对外头文件
 */

#pragma once

#include <iostream>

namespace ChannelAlgorithm_NS
{
    /* 算法类型 */
    typedef enum _AnalyzerType_
    {
        CHALG_V1 = 0, /* 航道分析-第一代算法 */
    } AnalyzerType_E;

    /* 必需参数 */
    typedef struct _NeedParam_
    {
        AnalyzerType_E enType; /* 使用的算法类型 */

        /* 构造函数 */
        _NeedParam_()
        {
            enType = CHALG_V1;
        }

        void clear()
        {
            enType = CHALG_V1;
        }
    } NeedParam_S;

    /* 可选参数 */
    typedef struct _ExParam_
    {
        bool bLine;                    /* 是否划线 */
        bool bAutoConvert;             /* 数据格式不正确时，是否应自动进行转换 */
        int  nMaxQueue;                /* 数据队列最大值 */

        int nDistanceThreshold;        /* 其他线段和 标准线 的距离阈值 */
        int nAngleThreshold;           /* 其他线段和 标准线 的角度阈值 */
        int nBrokenChannelNumThreshold; /* 累计断桥数的上限 */

        /* 构造函数 */
        _ExParam_()
        {
            bLine        = false;
            bAutoConvert = false;
            nMaxQueue    = 10;

            nDistanceThreshold        = 5;
            nAngleThreshold           = 10;
            nBrokenChannelNumThreshold = 100;
        }

        void clear()
        {
            bLine        = false;
            bAutoConvert = false;
            nMaxQueue    = 10;

            nDistanceThreshold        = 5;
            nAngleThreshold           = 10;
            nBrokenChannelNumThreshold = 100;
        }

    } ExParam_S;

    /* 参数结构体 */
    typedef struct _InParam_
    {
        NeedParam_S stNeedParam; /* 必需参数 */
        ExParam_S   stExParam;   /* 额外参数 */

        void clear()
        {
            stNeedParam.clear();
            stExParam.clear();
        }
    } InParam_S;

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

        /* 重载 == 运算符 */
        bool operator==(const _PosInfo_& other) const
        {
            return this->fX1 == other.fX1 &&
                this->fY1 == other.fY1 &&
                this->fX2 == other.fX2 &&
                this->fY2 == other.fY2;
        }
    } PosInfo_S;

    /* 行为分析结果结果信息 */
    typedef struct _AnalyzerResult_
    {
        PosInfo_S stPos; /* 坐标信息 */

        void clear()
        {
            stPos.clear();
        }

        /* 重载 == 运算符 */
        bool operator==(const _AnalyzerResult_& other) const
        {
            return this->stPos == other.stPos;
        }

        void print() const
        {
            std::cout << "\n桥梁分析处理结果信息:=============" << std::endl;
            std::cout << "坐标:"
                      << "[" << stPos.fX1 << "," << stPos.fY1 << "  "
                      << stPos.fX2 << "," << stPos.fY2 << "]" << std::endl;
            std::cout << "end:=============" << std::endl;
        }
    } AnalyzerResult_S;

}    // namespace ChannelAlgorithm_NS