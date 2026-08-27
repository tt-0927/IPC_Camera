/*
 * @FilePath     : MaintenanceStruct.h
 * @Author       : xiezhh
 * @Date         : 2024-06-04 17:23
 * @LastEditors: 梁浩尧 lianghaoyao@kfb.cn
 * @LastEditTime: 2025-09-29 10:51:05
 * @Description  : 运维管理上传日志和配置的结构体在该头文件定义
 */
#ifndef MAINTENANCESTRUCT_H
#define MAINTENANCESTRUCT_H

#include <queue>
#include <list>
#include <deque>
#include <set>
#include <map>
#include <string>

#include <thread>
#include <atomic>
#include <mutex>
#include <shared_mutex>

#include "MaintenanceCommon.h"

/* 配置 */
namespace MaintenanceNS
{

    typedef struct UploadConfig
    {
        FileType enFileType = FILE_TYPE_LOG; /* 文件类型 */
        std::string strFileNameFormat;       /* 文件名称格式 */
    } UploadConfig_S;

    /*程序的配置内容*/
    typedef struct MaintenanceManagerConf
    {
        std::string strUrl = "https://oam.itc-pa.cn"; /* 请求的url（只到端口） */
        std::string strCode;                            /* 项目唯一code */
        std::string strDeviceCode;                      /* 设备唯一code */
        std::string strRecordFilePath;                  /* 存放记录文件的目录 */
        std::vector<std::string> vecPaths;              /* 存放日志/配置文件的路径集合 */
        std::vector<UploadConfig> vecUploadFile;        /* 上传的文件名称格式 */
    } MaintenanceManagerConf_S;
}

/*文件*/
namespace MaintenanceNS
{

    /* 文件信息结构体 */
    typedef struct FileInfo
    {
        FileType enFileType = FILE_TYPE_LOG; /* 文件类型 */
        std::string strFilePath;             /* 文件路径     */
        std::string strFileName;             /* 文件名称     */
        std::string strIdentifier;           /* 文件唯一标识  */
        std::size_t nFileSize;               /* 文件大小     */
        std::string strFileDate;             /* 文件日期（从文件名获取） */
    } FileInfo_S;

    /* 记录信息，结构体 */
    typedef struct RecordInfo
    {
        FileInfo stFileInfo;                      /* 文件路径 */
        UploadStatus enUploadStatus = UPLOAD_NOT; /* 上传状态 */
    } RecordInfo_S;

    /* 上传文件明细 */
    typedef struct UploadData
    {
        RecordInfo stRecordInfo;
        int nSliceCount = 0;
        int nCurSlice = 0;
        std::size_t nCurSliceSize = 0;
        char *pSliceBuffer = nullptr;
        bool bUploadSliceFlag = false;
    } UploadData_S;

}

/* HTTP事务管理 */
namespace MaintenanceNS
{

    /* 一个Http请求 */
    typedef struct HttpRequery
    {
        RequeryType enRequeryType = REQ_TYPE_NORMAL;
        RequeryFun enRequeryFun = REQ_FUN_POST;
        RequeryInterface enInterface = REQ_NONE;
    } HttpRequery_S;

}

/*请求Http*/
namespace MaintenanceNS
{

    /*固定字段*/
    typedef struct Result
    {
        int nResult = -1;          /* 结果代码 */
        std::string strMsg;        /* 结果消息 */
        std::string strCompany;    /* 固定为：BL */
        std::string strDeviceName; /* 固定为：TS-MAINTAIN01 */
    } Result_S;

    /*产品线数据*/
    typedef struct Product
    {
        int nID = -1;        /* 产品线ID */
        std::string strName; /* 产品线名称 */
    } Product_S;

    /*登录请求结果内容*/
    typedef struct LoginResult
    {
        Result stResult;

        std::string strToken;            /* 后续请求头的ApiToken的值 */
        std::string strNick;             /* 当前用户名字 */
        std::string strUsername;         /* 当前用户账号 */
        std::vector<Product> vecProduct; /* 产品线列表 */
    } LoginResult_S;

    /*项目数据*/
    typedef struct Project
    {
        std::string strProductName; /* 产品线名称 */
        int nID = -1;               /* 项目ID */
        std::string strName;        /* 项目名称 */
        std::string strCode;        /* 项目唯一值 */
    } Project_S;

    /*获取项目请求结果内容*/
    typedef struct ReqProjectResult
    {
        Result stResult;

        int nTotalPage;                  /*总页数*/
        int nTotal;                      /*当前页数据总条数*/
        int nPageid;                     /*当前页*/
        std::vector<Project> vecProject; /*项目列表*/
    } ReqProjectResult_S;

    /*获取项目请求结果内容*/
    typedef struct ReqUploadResult
    {
        Result stResult;
    } ReqUploadResult_S;
}

#endif // MAINTENANCESTRUCT_H
