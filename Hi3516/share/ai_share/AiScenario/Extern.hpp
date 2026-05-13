/*
 * @FilePath     : Extern.hpp
 * @Author       : 严泽辉 yanzeh@kfb.cn
 * @Date         : 2024-05-17 16:49:16
 * @LastEditors: 李辉 lihui@kfb.cn
 * @LastEditTime: 2024-09-02 15:40:35
 * @Description  :
 */
#pragma once

#include <list>
#include <string>
#include <vector>

namespace AiScenario_NS
{
    /* 算法类型 */
    typedef enum _Type_
    {
        NULL_TYPE = 0,    /* 空类型 */
        HUMAN_CUTOUT,     /* 人数统计 */
        STUDENT_BEHAVIOR, /* 学生行为分析 */
        TRACKER,          /* 跟踪 */
        FACR_RECT,        /* 人脸识别 */
        FACE_EXPRESS,     /* 表情识别 */
        VOICE_WAKE_UP,    /* 语音唤醒 */
        NUMBER_OCR,       /* 黑底白数字识别 */
        VIRTUAL_CUT,      /* 虚拟抠像 */
        HUMAN_AREA,       /* 行人区域检测 */
        DISCIPLINE,       /* 课堂纪律 */		
    } Type_E;

    /* 版本 */
    typedef enum _Versions_
    {
        V_NULL = 0, /* 空版本 */
        V1_0,
        V2_0,
    } Versions_E;

    /* 结果类型 */
    typedef enum _ResultType_
    {
        NULL_RESTYPE = 0, /* 空类型 */
        JSON,             /* Json数据 */
        XML,              /* xml数据 */
        PIC,              /* 图片数据 */
        STRING,           /* 字符串数据 */
    } ResultType_E;

    /* 必需参数 */
    typedef struct _NeedParam_
    {
        Type_E                   enType;        /* 使用的算法类型 */
        Versions_E               enVersions;    /* 使用的版本号 */
        ResultType_E             enResultType;  /* 返回数据类型 */
        std::vector<std::string> vstrModelPath; /* 模型路径数组，有可能某些算法需要设置多个模型，具体请查看相关readme.md */

        void clear()
        {
            enType       = Type_E::NULL_TYPE;
            enVersions   = Versions_E::V_NULL;
            enResultType = ResultType_E::NULL_RESTYPE;
            vstrModelPath.clear();
        }

        _NeedParam_()
        {
            clear();
        }

        bool check()
        {
            if (enType == Type_E::NULL_TYPE ||
                enVersions == Versions_E::V_NULL ||
                enResultType == ResultType_E::NULL_RESTYPE ||
                vstrModelPath.empty())
            {
                return false;
            }

            return true;
        }

    } NeedParam_S;

    /* 额外参数 */
    typedef struct _ExParam_
    {
        /* yolo的阈值 */
        float fBoxThreshold; /* 置信度 */
        float fNmsThreshold;

        /* 调试功能 */
        bool        bDebug;              /* 是否开启调试功能 */
        std::string strAnalyzeDataPath;  /* 设置分析后数据的保存路径, 文件夹路径 */
        std::string strOriginalDataPath; /* 原始数据保存路径, 文件夹路径 */

        void clear()
        {
            fBoxThreshold = 0.0f;
            fNmsThreshold = 0.0f;

            bDebug              = false;
            strAnalyzeDataPath  = "";
            strOriginalDataPath = "";
        }

        _ExParam_()
        {
            clear();
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

        bool check()
        {
            return stNeedParam.check();
        }
    } InParam_S;

}    // namespace AiScenario_NS
