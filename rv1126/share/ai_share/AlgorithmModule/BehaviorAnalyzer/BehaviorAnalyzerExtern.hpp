/*
 * @FilePath     : BehaviorAnalyzerExtern.hpp
 * @Author       : yanzeh yanzeh@kfb.cn
 * @Date         : 2024-01-11 09:51:22
 * @LastEditors  : yanzeh yanzeh@kfb.cn
 * @LastEditTime : 2024-01-23 14:26:01
 * @Description  : 行为分析模块对外头文件
 */
#pragma once

#include <iostream>

namespace BA_NS
{
    /* 算法类型 */
    typedef enum _BehaviorAnalyzerType_
    {
        BA_STU_V1 = 0, /* 学生行为分析-第一代算法 */
        BA_STU_V2,     /* 学生行为分析-第二代算法 */
    } BehaviorAnalyzerType_E;

    /* 必需参数 */
    typedef struct _BehaviorAnalyzerNeedParam_
    {
        BehaviorAnalyzerType_E enType;       /* 使用的算法类型 */
        std::string            strModelPath; /* 测算法模型路径 */

        /* 构造函数 */
        _BehaviorAnalyzerNeedParam_()
        {
            enType = BA_STU_V1;
            strModelPath.clear();
        }

        void clear()
        {
            enType = BA_STU_V1;
            strModelPath.clear();
        }
    } BehaviorAnalyzerNeedParam_S;

    /* 额外参数 */
    typedef struct _BehaviorAnalyzerExParam_
    {
        bool  bAutoConvert;         /* 数据格式不正确时，是否应自动进行转换 */
        int   nMaxQueue;            /* 数据队列最大值 */
        float fConfidenceThreshold; /* 置信度分数阈值 */

        /* 构造函数 */
        _BehaviorAnalyzerExParam_()
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

    } BehaviorAnalyzerExParam_S;

    /* 参数结构体 */
    typedef struct _BehaviorAnalyzerInParam_
    {
        BehaviorAnalyzerNeedParam_S stNeedParam; /* 必需参数 */
        BehaviorAnalyzerExParam_S   stExParam;   /* 额外参数 */

        void clear()
        {
            stNeedParam.clear();
            stExParam.clear();
        }
    } BehaviorAnalyzerInParam_S;

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

    /* 行为类型枚举类型 */
    typedef enum _BehaviorType_
    {
        LOW_HEAD_WRITING = 0, /* 低头写字 */
        LOW_HEAD_READING,     /* 低头看书 */
        RAISE_HEAD_LISTENING, /* 抬头听课 */
        TURN_HEAD,            /* 转头 */
        RAISE_HAND,           /* 举手 */
        STAND,                /* 站立 */
        GROUP_DISCUSSION,     /* 小组讨论 */
        TEACHER_GUIDANCE      /* 教师指导 */
    } BehaviorType_E;

    /* 行为分析结果结果信息 */
    typedef struct _BehaviorAnalyzerResult_
    {
        float          fConfidenceScore; /* 置信度分数 0.0-1.0 */
        BehaviorType_E enBehavior;       /* 行为 */
        PosInfo_S      stPos;            /* 坐标信息 */

        void clear()
        {
            fConfidenceScore = 0.0;
            enBehavior       = LOW_HEAD_WRITING;
            stPos.clear();
        }

        /* 重载 == 运算符 */
        bool operator==(const _BehaviorAnalyzerResult_& other) const
        {
            return this->fConfidenceScore == other.fConfidenceScore &&
                this->enBehavior == other.enBehavior &&
                this->stPos == other.stPos;
        }

        void print() const
        {
            std::cout << "\n行为分析处理结果信息:=============" << std::endl;
            std::cout << "置信度分数:" << fConfidenceScore << std::endl;
            std::cout << "行为:" << enBehavior << std::endl;
            std::cout << "坐标:"
                      << "[" << stPos.fX1 << "," << stPos.fY1 << "  "
                      << stPos.fX2 << "," << stPos.fY2 << "]" << std::endl;
            std::cout << "end:=============" << std::endl;
        }
    } BehaviorAnalyzerResult_S;

}    // namespace BA_NS