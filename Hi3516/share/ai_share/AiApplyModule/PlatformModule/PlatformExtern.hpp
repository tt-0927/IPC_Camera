/*
 * @FilePath     : PlatformExtern.hpp
 * @Author       : 李辉 lih@kfb.cn
 * @Date         : 2024-04-02 20:02:08
 * @LastEditors: error: error: git config user.name & please set dead value or install git && error: git config user.email & please set dead value or install git & please set dead value or install git
 * @LastEditTime: 2024-05-06 20:09:16
 * @Description  :
 */

#pragma once

#include <functional>
#include <iostream>
#include <list>

#include "BlError.h"

namespace PlatformManage_NS
{
    typedef struct _ClassInfo_
    {
        int         nId;     /* 班级Id */
        std::string strName; /* 班级名称 */

        void clear()
        {
            nId = 0;
            strName.clear();
        }

        void print() const
        {
            std::cout << "\n获取班级信息请求:=============" << std::endl;
            std::cout << "班级Id:" << nId << std::endl;
            std::cout << "班级名称:" << strName << std::endl;
            std::cout << "end:=============" << std::endl;
        }
    } ClassInfo_S;

    typedef struct _HumanInfo_
    {
        int         nId;         /* 人员Id */
        std::string strName;     /* 人员名称 */
        std::string strPath;     /* 人脸地址 */
        std::string strMd5;      /* 人脸图片MD5值 */
        std::string strFileName; /* 文件名 */

        void clear()
        {
            nId = 0;
            strName.clear();
            strPath.clear();
            strMd5.clear();
            strFileName.clear();
        }

        void print() const
        {
            std::cout << "\n获取班级信息请求:=============" << std::endl;
            std::cout << "人员Id:" << nId << std::endl;
            std::cout << "人员名称:" << strName << std::endl;
            std::cout << "人脸地址:" << strPath << std::endl;
            std::cout << "人脸图片MD5值:" << strMd5 << std::endl;
            std::cout << "文件名:" << strFileName << std::endl;
            std::cout << "end:=============" << std::endl;
        }
    } HumanInfo_S;

    typedef struct _DataInfo_
    {
        std::string            strTarPath;  /* 下载好的压缩文件路径 */
        ClassInfo_S            stClassInfo; /* 班级信息结构体 */
        std::list<HumanInfo_S> listTeaInfo; /* 教师人脸结构体 */
        std::list<HumanInfo_S> listStuInfo; /* 学生人脸结构体 */

        void clear()
        {
            strTarPath.clear();
            stClassInfo.clear();
            listTeaInfo.clear();
            listStuInfo.clear();
        }

        void print() const
        {
            stClassInfo.print();
            // listTeaInfo.print();
            // listStuInfo.print();
        }
    } DataInfo_S;

    /* 获取天气类型 */
    typedef enum _AiPlatformT_
    {
        ITC_PLATFORM = 0, /* ITC平台 */
    } AiPlatformType_E;

    /* 句柄 */
    typedef void* AiPlatformHandle_H;


    /**
     * @brief 天气信息http请求的回调函数
     * @param [BlError_E] enRetCode: 返回码
     * @return [*]  BlError_E::OK 成功  其他失败
     * @note
     */
    typedef std::function<BlError_E(DataInfo_S, BlError_E, void*)> AiPlatformCallbackPFunc;

    /* 必需参数 */
    typedef struct _AiPlatformNeedParam_
    {
        AiPlatformType_E        enType;               /* API的类型 */
        char                    achIpAddr[64];        /* 平台的IP地址 */
        char                    achDownloadPath[256]; /* 下载脚本路径 */
        AiPlatformCallbackPFunc aiPlatformCallback;   /* 平台http请求的回调函数 */
        void*                   pHandle;              /* 用户参数 */
    } AiPlatformNeedParam_S;

    /* 参数结构体 */
    typedef struct _AiPlatformInParam_
    {
        AiPlatformNeedParam_S stNeedParam; /* 必需参数 */
    } AiPlatformInParam_S;
}    // namespace PlatformManage_NS
