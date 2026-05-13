/*
 * @FilePath     : FaceRecExtern.hpp
 * @Author       : yanzeh yanzeh@kfb.cn
 * @Date         : 2024-01-08 16:04:17
 * @LastEditors  : 严泽辉 yanzeh@kfb.cn
 * @LastEditTime : 2024-02-20 15:37:07
 * @Description  : 人脸识别模块对外头文件
 */
#pragma once

#include <iostream>
#include <list>
#include <vector>

namespace FR_NS
{

    /* 算法类型 */
    typedef enum _FaceAlgorithmType_
    {
        FR_V1 = 0, /* 第一代算法 */
    } FaceAlgorithmType_E;

    /* 必需参数 */
    typedef struct _FaceRecNeedParam_
    {
        FaceAlgorithmType_E enType;                            /* 使用的算法类型 */
        std::string         strFaceDetectionModelPath;         /* 人脸检测算法模型路径 */
        std::string         strFaceFeatureExtractionModelPath; /* 人脸特征提取算法模型路径 */

        /* 构造函数 */
        _FaceRecNeedParam_()
        {
            enType = FR_V1;
            strFaceDetectionModelPath.clear();
            strFaceFeatureExtractionModelPath.clear();
        }

        void clear()
        {
            enType = FR_V1;
            strFaceDetectionModelPath.clear();
            strFaceFeatureExtractionModelPath.clear();
        }
    } FaceRecNeedParam_S;

    /* 额外参数 */
    typedef struct _FaceRecExParam_
    {
        float fConfidenceThreshold; /* 置信度分数阈值 */
        float fSimilarityThreshold; /* 相似分数阈值 */
        bool  bAutoConvert;         /* 数据格式不正确时，是否应自动进行转换 */
        int   nMaxQueue;            /* 数据队列最大值 */

        /* 构造函数 */
        _FaceRecExParam_()
        {
            fConfidenceThreshold = 0.75;
            fSimilarityThreshold = 0.6;
            bAutoConvert         = false;
            nMaxQueue            = 50;
        }

        void clear()
        {
            fConfidenceThreshold = 0.75;
            fSimilarityThreshold = 0.6;
            bAutoConvert         = false;
            nMaxQueue            = 50;
        }

    } FaceRecExParam_S;

    /* 参数结构体 */
    typedef struct _FaceRecInParam_
    {
        FaceRecNeedParam_S stNeedParam; /* 必需参数 */
        FaceRecExParam_S   stExParam;   /* 额外参数 */

        void clear()
        {
            stNeedParam.clear();
            stExParam.clear();
        }
    } FaceRecInParam_S;

    typedef struct _FaceDataInfo_
    {
        int                nId;
        std::string        strName;
        std::string        strPicName;
        std::string        strPicPath;
        std::vector<float> vfData;
        int                nCardId;         /* 学号 */

        void clear()
        {
            nId = 0;
            strName.clear();
            strPicName.clear();
            strPicPath.clear();
            vfData.clear();
            nCardId = 0;
        }

        void print() const
        {
            std::cout << "\n人脸特征信息:=============" << std::endl;
            std::cout << "ID:" << nId << std::endl;
            std::cout << "名字:" << strName << std::endl;
            std::cout << "图片名字:" << strPicName << std::endl;
            std::cout << "图片路径:" << strPicPath << std::endl;
            std::cout << "学号:" << nCardId << std::endl;
            std::cout << "数据:" << std::endl;
            for (size_t i = 0; i < vfData.size(); i++)
            {
                std::cout << vfData.at(i) << ", ";
            }
            std::cout << std::endl;
            std::cout << "end:=============" << std::endl;
        }

    } FaceDataInfo_S;

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

    /* 人脸识别结果信息 */
    typedef struct _FaceRecognitionResult_
    {
        int         nFaceID;          /* 人脸ID */
        float       fSimilarityScore; /* 相识度分数 0.0-1.0 */
        std::string strPersonName;    /* 人物名称 */
        PosInfo_S   stPos;            /* 坐标信息 */

        void clear()
        {
            nFaceID          = -1;
            fSimilarityScore = 0.0;
            strPersonName.clear();
            stPos.clear();
        }

        void print() const
        {
            std::cout << "\n人脸处理结果信息:=============" << std::endl;
            std::cout << "人脸ID:" << nFaceID << std::endl;
            std::cout << "置信度分数:" << fSimilarityScore << std::endl;
            std::cout << "人脸名字:" << strPersonName << std::endl;
            std::cout << "坐标:"
                      << "[" << stPos.fX1 << "," << stPos.fY1 << "  "
                      << stPos.fX2 << "," << stPos.fY2 << "]" << std::endl;
            std::cout << "end:=============" << std::endl;
        }
    } FaceRecognitionResult_S;

}    // namespace FR_NS