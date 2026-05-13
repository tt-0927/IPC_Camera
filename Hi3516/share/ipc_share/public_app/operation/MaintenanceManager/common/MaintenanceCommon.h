/*
 * @FilePath     : MaintenanceCommon.h
 * @Author       : xiezhh
 * @Date         : 2024-06-04 17:23
 * @LastEditors  : xiezhh
 * @LastEditTime : 2024-06-25 15:24
 * @Description  : 运维管理上传日志和配置的宏定义和枚举在该头文件定义
 */
#ifndef MAINTENANCECOMMON_H
#define MAINTENANCECOMMON_H

/* 分片大小 (2 * 1024 * 1024) */
#define SLICE_SIZE (2*1024*1024)

/* 请求的接口 */
#define REQ_LOGIN_PATH "/api/v2/user/login"
#define REQ_PROJECT_PATH "/api/v2/project/project_list"
#define REQ_UPLOAD_LOG_PATH "/api/v2/project/add_project_log"
#define REQ_UPLOAD_CONFIG_PATH "/api/v2/project/add_project_config"

/* 线程休眠时间 */
#define CHECKFILE_MSLEEP 5000
#define REQNORMAL_MSLEEP 1000
#define REQUPLOAD_MSLEEP 1000
#define REQUPLOAD_EMPTY_MSLEEP 5000

/* 创建历史记录文件和创建过滤日期文件时会用到的宏 */
/* 路径字符串最大长度 */
#define MAX_PATH_LEN 256

/* 判断文件夹存不存在在和创建文件夹的宏 */
#ifdef WIN32
#define ACCESS(fileName,accessMode) _access(fileName,accessMode)
#define MKDIR(path) _mkdir(path)
#else
#define ACCESS(fileName,accessMode) access(fileName,accessMode)
#define MKDIR(path) mkdir(path,S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH)
#endif

namespace MaintenanceNS {

    /* 文件类型 */
    typedef enum FileType{
        FILE_TYPE_NORMAL      = -1,    /* 空 */
        FILE_TYPE_LOG         =  0,    /* 日志 */
        FILE_TYPE_CONFIGURE   =  1,    /* 配置 */
    }FileType_En;

    /* 文件类型 */
    typedef enum UploadStatus{
        UPLOAD_NOT  = 0,    /* 未上传 */
        UPLOADING   = 1,    /* 上传中 */
        UPLOADED    = 2,    /* 已上传 */
    }UploadStatus_En;

    /* 请求类型枚举 */
    typedef enum RequeryType{
        REQ_TYPE_NORMAL = 0,    /* 正常请求 */
        REQ_TYPE_UPLOAD = 1,    /* 上传请求 */
    }RequeryType_En;

    /* 请求接口枚举 */
    typedef enum RequeryFun{
        REQ_FUN_GET     = 0,    /* Get请求 */
        REQ_FUN_POST    = 1,    /* Post请求 */
    }RequeryFun_En;

    /* 请求接口枚举 */
    typedef enum RequeryInterface{
        REQ_NONE    = -1,       /* 请求为空 */
        REQ_LOGIN   = 0,        /* 请求登录 */
        REQ_PROJECT = 1,        /* 请求项目 */
        REQ_UPLOAD  = 2,        /* 请求上传 */
    }RequeryInterface_En;

    /* 接口返回的错误码 */
    typedef enum ResultCode{
        SUCCESS             = 200,      /* 成功 */
        UNAUTHENTICATION    = 4000,     /* 未设置Token，鉴权失败 */
        UNAUTHENTICATION_V2 = 4001,     /* 未设置Token，鉴权失败 */
    }ResultCode_En;
}

#endif // MAINTENANCECOMMON_H
