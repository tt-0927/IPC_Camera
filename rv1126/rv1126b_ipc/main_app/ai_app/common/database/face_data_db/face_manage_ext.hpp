/**
 * @FilePath     : face_manage_ext.hpp
 * @Author       : zhouzirui
 * @Date         : 2025-06-06 16:02:10
 * @LastEditors  : zhouzirui
 * @LastEditTime : 2025-06-06 16:59:43
 * @Description  : 
 */

#pragma once

#include <iostream>
#include <list>
#include <vector>

namespace FaceDataDB_NS
{
    /* 抓拍人脸数据结构，包含特征和相关信息 */
    typedef struct _SnapFaceInfo_
    {
        int                nId;
        int                nChnId;      /* 通道号 */
        std::string        strPicPath;
        std::vector<float> vfData;

        void clear()
        {
            nId = 0;
            nChnId  = -1;
            strPicPath.clear();
            vfData.clear();
        }

        void print() const
        {
            std::cout << "\n抓拍人脸信息:=============" << std::endl;
            std::cout << "ID:" << nId << std::endl;
            std::cout << "通道ID: " << nChnId << std::endl;
            std::cout << "图片名称:" << strPicPath << std::endl;
            
            // std::cout << "特征向量:" << std::endl;
            // for (size_t i = 0; i < vfData.size(); i++)
            // {
            //     std::cout << vfData.at(i) << ", ";
            // }
            // std::cout << std::endl;
            std::cout << "end:=============" << std::endl;
        }

    } SnapFaceInfo_S;

    /* 名单库人脸数据结构，包含特征和相关信息 */
    typedef struct _FaceLibsInfo_
    {
        int nId;
        std::string strFaceLibName;       /* 名单组名称 */
        std::string strName;        /* 名字 */
        std::string strPhoneNum;    /* 联系方式 */
        std::string strPicPath;     /* 图片名称 */
        std::string strPicType;     /* 图片类型 */
        int nPicSize;               /* 图片大小 */
        std::string strPicDate;     /* 图片日期 */
        int nModelState;            /* 模型状态 0: 未建模 1:建模成功 2:建模失败 */
        int nRatingLevel;           /* 评估等级 */
        std::vector<float> vfData;  /* 特征向量（以BLOB形式存储）*/

        void clear()
        {
            nId = 0;
            strFaceLibName.clear();
            strName.clear();
            strPhoneNum.clear();
            strPicPath.clear();
            strPicType.clear();
            nPicSize = 0;
            strPicDate.clear();
            nModelState = 0;
            nRatingLevel = 0;
            vfData.clear();
        }

        void print() const
        {
            std::cout << "\n名单库人脸信息:=============" << std::endl;
            std::cout << "ID: " << nId << std::endl;
            std::cout << "名单组名称: " << strFaceLibName << std::endl;
            std::cout << "名字: " << strName << std::endl;
            std::cout << "联系方式: " << strPhoneNum << std::endl;
            std::cout << "图片名称: " << strPicPath << std::endl;
            std::cout << "图片类型: " << strPicType << std::endl;
            std::cout << "图片大小: " << nPicSize << std::endl;
            std::cout << "图片日期: " << strPicDate << std::endl;
            std::cout << "模型状态: " << nModelState << std::endl;
            std::cout << "评估等级: " << nRatingLevel << std::endl;

            // std::cout << "特征向量: " << std::endl;
            // for (size_t i = 0; i < vfData.size(); i++)
            // {
            //     std::cout << vfData.at(i) << ", ";
            // }
            // std::cout << std::endl;
            std::cout << "end:=============" << std::endl;
        }

    } FaceLibsInfo_S;

    
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
