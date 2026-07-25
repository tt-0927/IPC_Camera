/*
 * @FilePath     : ParseJson.cpp
 * @Author       : yanzeh yanzeh@kfb.cn
 * @Date         : 2023-06-19 11:22:11
 * @LastEditors: weigl 308241951@qq.com
 * @LastEditTime: 2025-02-27 20:29:25
 * @Description  : 解析数据-Json
 */
#include "ParseJson.hpp"

#include <chrono>

#include "dlog.h"

using namespace ParseData_NS;

ParseData_NS::CParseJson::CParseJson(InParam_S stInfo)
    : CParseBase(stInfo)
{
}

/* 解析Json数据-获取错误 */
BlError_E CParseJson::parsePlatform(const char* pchJson, int& nError, std::string& strError)
{
    if (NULL == pchJson)
    {
        dlog(LOG_ERROR, "传入参数异常");
        return ERR_IN_PARAM_NULL;
    }

    Json::Object* pJsonHandle = NULL;
    bool          bRet        = false;
    int           nReturn     = 0;

    pJsonHandle = Json::init(pchJson);

    bRet = Json::get(pJsonHandle, "result", nError);
    if (!bRet)
    {
        dlog(LOG_ERROR, "解析[result]字段失败");
        return ERR_PARSE;
    }

    bRet = Json::get(pJsonHandle, "return_message", strError);
    if (!bRet)
    {
        dlog(LOG_ERROR, "解析[return_message]字段失败");
        return ERR_PARSE;
    }

    return OK;
}

/* 解析Json数据-获取token值 */
BlError_E CParseJson::parseToken(const char* pchJson, std::string& strToken)
{
    if (NULL == pchJson)
    {
        dlog(LOG_ERROR, "传入参数异常");
        return ERR_IN_PARAM_NULL;
    }
    BlError_E enRetCode = OK;
    bool      bRet      = false;
    int       nReturn   = 0;

    Json::Object* pJsonHandle = NULL;
    Json::Object* pJsonData   = NULL;
    int           nExpires;

    pJsonHandle = Json::init(pchJson);
    if (!pJsonHandle)
    {
        dlog(LOG_ERROR, "pJsonHandle传入参数错误, %s", pchJson);
        enRetCode = ERR_PARSE;
        goto EXIT;
    }

    pJsonData = Json::get(pJsonHandle, "data");
    if (!pJsonData)
    {
        dlog(LOG_ERROR, "解析[data]字段失败, pchJson = %s", pchJson);
        enRetCode = ERR_PARSE;
        goto EXIT;
    }

    bRet = Json::get(pJsonData, "access_token", strToken);
    if (!bRet)
    {
        dlog(LOG_ERROR, "解析[access_token]字段失败");
        enRetCode = ERR_PARSE;
        goto EXIT;
    }

    bRet = Json::get(pJsonData, "expires", nExpires);
    if (!bRet)
    {
        dlog(LOG_ERROR, "解析[expires]字段失败");
        enRetCode = ERR_PARSE;
        goto EXIT;
    }

EXIT:
    if (pJsonHandle)
    {
        Json::deinit(pJsonHandle);
        pJsonHandle = NULL;
    }

    return enRetCode;
}

/* 解析Json数据-获取班级成员信息 */
BlError_E CParseJson::parseClassInfo(const char* pchJson, PlatformManage_NS::DataInfo_S& stDataInfo)
{
    if (NULL == pchJson)
    {
        dlog(LOG_ERROR, "传入参数异常");
        return ERR_IN_PARAM_NULL;
    }
    BlError_E     enRetCode        = OK;
    int           nSize            = 0;
    bool          bRet             = false;
    Json::Object* pJsonHandle      = NULL;
    Json::Object* pDataObject      = NULL;
    Json::Object* pChildDataObject = NULL;
    Json::Object* pClassObject     = NULL;
    Json::Object* pTeaObject       = NULL;
    Json::Object* pStuObject       = NULL;
    Json::Object* pItemObject      = NULL;

    /*创建操作句柄*/
    pJsonHandle = Json::init(pchJson);
    if (NULL == pJsonHandle)
    {
        dlog(LOG_ERROR, "传入的Json字符串有问题, 无法创建句柄");
        enRetCode = ERR_PARSE;
        goto EXIT;
    }

    pDataObject = Json::get(pJsonHandle, "data");
    if (NULL == pDataObject)
    {
        dlog(LOG_ERROR, "获取[data]数据节点-失败");
        enRetCode = ERR_PARSE;
        goto EXIT;
    }

    pChildDataObject = Json::get(pDataObject, "data");
    if (NULL == pChildDataObject)
    {
        dlog(LOG_ERROR, "获取[data]数据节点-失败");
        enRetCode = ERR_PARSE;
        goto EXIT;
    }

    /*解析班级信息*/
    pClassObject = Json::get(pChildDataObject, "class");
    if (NULL == pClassObject)
    {
        dlog(LOG_ERROR, "获取[class]数据节点-失败");
        enRetCode = ERR_PARSE;
        goto EXIT;
    }
    bRet = Json::get(pClassObject, "class_id", stDataInfo.stClassInfo.nId);
    if (!bRet)
    {
        dlog(LOG_ERROR, "获取[class_id]数据节点-失败");
        enRetCode = ERR_PARSE;
        goto EXIT;
    }
    bRet = Json::get(pClassObject, "class_name", stDataInfo.stClassInfo.strName);
    if (!bRet)
    {
        dlog(LOG_ERROR, "获取[class_name]数据节点-失败");
        enRetCode = ERR_PARSE;
        goto EXIT;
    }

    /*解析教师信息*/
    pTeaObject = Json::get(pChildDataObject, "teacher");
    if (NULL != pTeaObject)
    {
        /* 获取数组大小 */
        nSize = Json::Array::size(pTeaObject);
        if (nSize <= 0)
        {
            dlog(LOG_ERROR, "数组大小异常[%d]", nSize);
            // enRetCode = ERR_PARSE;
            // goto EXIT;
        }
        for (int i = 0; i < nSize; i++)
        {
            /* 获取数组的节点 */
            pItemObject = Json::Array::get(pTeaObject, i);
            if (NULL == pItemObject)
            {
                dlog(LOG_ERROR, "获取数组节点失败 [%d]", i);
                enRetCode = ERR_PARSE;
                goto EXIT;
            }
            PlatformManage_NS::HumanInfo_S stHumanInfo;
            bRet = Json::get(pItemObject, "user_id", stHumanInfo.nId);
            if (!bRet)
            {
                dlog(LOG_ERROR, "获取[user_id]数据节点-失败");
                enRetCode = ERR_PARSE;
                goto EXIT;
            }

            bRet = Json::get(pItemObject, "name", stHumanInfo.strName);
            if (!bRet)
            {
                dlog(LOG_ERROR, "获取[name]数据节点-失败");
                enRetCode = ERR_PARSE;
                goto EXIT;
            }

            bRet = Json::get(pItemObject, "face_path", stHumanInfo.strPath);
            if (!bRet)
            {
                dlog(LOG_ERROR, "获取[face_path]数据节点-失败");
                enRetCode = ERR_PARSE;
                goto EXIT;
            }

            bRet = Json::get(pItemObject, "face_md5", stHumanInfo.strMd5);
            if (!bRet)
            {
                dlog(LOG_ERROR, "获取[face_md5]数据节点-失败");
                enRetCode = ERR_PARSE;
                goto EXIT;
            }

            bRet = Json::get(pItemObject, "file_name", stHumanInfo.strFileName);
            if (!bRet)
            {
                dlog(LOG_ERROR, "获取[file_name]数据节点-失败");
                enRetCode = ERR_PARSE;
                goto EXIT;
            }
            stDataInfo.listTeaInfo.push_back(stHumanInfo);
        }
    }
    else
    {
        dlog(LOG_TRACE, "获取[teacher]数据节点-为空");
    }


    /*解析学生信息*/
    pStuObject = Json::get(pChildDataObject, "student");
    if (NULL == pStuObject)
    {
        dlog(LOG_ERROR, "获取[student]数据节点-为空");
    }
    else
    {
        /* 获取数组大小 */
        nSize = Json::Array::size(pStuObject);
        if (nSize <= 0)
        {
            dlog(LOG_ERROR, "数组大小异常[%d]", nSize);
            // enRetCode = ERR_PARSE;
            // goto EXIT;
        }
        for (int i = 0; i < nSize; i++)
        {
            /* 获取数组的节点 */
            pItemObject = Json::Array::get(pStuObject, i);
            if (NULL == pItemObject)
            {
                dlog(LOG_ERROR, "获取数组节点失败 [%d]", i);
                enRetCode = ERR_PARSE;
                goto EXIT;
            }
            PlatformManage_NS::HumanInfo_S stHumanInfo;
            bRet = Json::get(pItemObject, "user_id", stHumanInfo.nId);
            if (!bRet)
            {
                dlog(LOG_ERROR, "获取[user_id]数据节点-失败");
                enRetCode = ERR_PARSE;
                goto EXIT;
            }

            bRet = Json::get(pItemObject, "name", stHumanInfo.strName);
            if (!bRet)
            {
                dlog(LOG_ERROR, "获取[name]数据节点-失败");
                enRetCode = ERR_PARSE;
                goto EXIT;
            }

            bRet = Json::get(pItemObject, "face_path", stHumanInfo.strPath);
            if (!bRet)
            {
                dlog(LOG_ERROR, "获取[face_path]数据节点-失败");
                enRetCode = ERR_PARSE;
                goto EXIT;
            }

            bRet = Json::get(pItemObject, "face_md5", stHumanInfo.strMd5);
            if (!bRet)
            {
                dlog(LOG_ERROR, "获取[face_md5]数据节点-失败");
                enRetCode = ERR_PARSE;
                goto EXIT;
            }

            bRet = Json::get(pItemObject, "file_name", stHumanInfo.strFileName);
            if (!bRet)
            {
                dlog(LOG_ERROR, "获取[file_name]数据节点-失败");
                enRetCode = ERR_PARSE;
                goto EXIT;
            }
            stDataInfo.listStuInfo.push_back(stHumanInfo);
        }
    }


EXIT:

    if (pJsonHandle)
    {
        Json::deinit(pJsonHandle);
        pJsonHandle = NULL;
    }

    return enRetCode;
}

/* 解析数据-解析板书识别信息 */
BlError_E CParseJson::parse(char* pData, AiManage_NS::BoardInfo_S& stInfo)
{
    BlError_E enRetCode = OK;

    int  nSize = 0;
    bool bRet  = false;

    Json::Object* pJsonHandle       = NULL;
    Json::Object* pDataObject       = NULL;
    Json::Object* pArrayObject      = NULL;
    Json::Object* pItemObject       = NULL;
    Json::Object* pArrayArrayObject = NULL;
    Json::Object* pArrayItemObject  = NULL;


    /*创建操作句柄*/
    pJsonHandle = Json::init(pData);
    if (NULL == pJsonHandle)
    {
        dlog(LOG_ERROR, "传入的Json字符串有问题, 无法创建句柄");
        enRetCode = ERR_PARSE;
        goto EXIT;
    }

    /* 获取头数据信息 */
    enRetCode = get_headInfo(pJsonHandle, stInfo.stHeadInfo);
    if (enRetCode < OK)
    {
        dlog(LOG_ERROR, "获取头数据信息-失败");
        enRetCode = ERR_PARSE;
        goto EXIT;
    }

    /* 获取数据节点 */
    pDataObject = Json::get(pJsonHandle, "Datas");
    if (NULL == pDataObject)
    {
        dlog(LOG_ERROR, "获取[Datas]数据节点-失败");
        enRetCode = ERR_PARSE;
        goto EXIT;
    }

    /* 获取数组节点 */
    pArrayObject = Json::get(pDataObject, "BaseData");
    if (NULL == pArrayObject)
    {
        dlog(LOG_ERROR, "获取[BaseData]数据节点-失败");
        enRetCode = ERR_PARSE;
        goto EXIT;
    }

    /* 获取数组大小 */
    nSize = Json::Array::size(pArrayObject);
    if (nSize <= 0)
    {
        goto EXIT;
    }

    stInfo.listBoardInfo.clear();
    for (int i = 0; i < nSize; i++)
    {
        AiManage_NS::BoardItemInfo_S stItemInfo;

        /* 获取数组的节点 */
        pItemObject = Json::Array::get(pArrayObject, i);
        if (nullptr == pItemObject)
        {
            dlog(LOG_ERROR, "获取数组节点失败 [%d]", i);
            enRetCode = ERR_PARSE;
            goto EXIT;
        }


        /* 获取数组节点 */
        pArrayArrayObject = Json::get(pItemObject, "Box");
        if (NULL == pArrayArrayObject)
        {
            dlog(LOG_ERROR, "获取[Box]数据节点-失败");
            enRetCode = ERR_PARSE;
            goto EXIT;
        }

        /* 获取框信息 */
        enRetCode = get_boxInfo(pArrayArrayObject, stItemInfo.stBoxInfo);
        if (enRetCode < OK)
        {
            dlog(LOG_ERROR, "获取框信息-失败");
            enRetCode = ERR_PARSE;
            goto EXIT;
        }

        /* 置信度 */
        bRet = Json::get(pItemObject, "Confidence", stItemInfo.fConfidence);
        if (!bRet)
        {
            dlog(LOG_ERROR, "获取[Confidence]数据节点-失败");
            enRetCode = ERR_PARSE;
            goto EXIT;
        }

        stInfo.listBoardInfo.push_back(stItemInfo);
    }

EXIT:

    if (pJsonHandle)
    {
        Json::deinit(pJsonHandle);
        pJsonHandle = NULL;
    }

    return enRetCode;
}

/* 解析数据-解析表情识别信息 */
BlError_E CParseJson::parse(char* pData, AiManage_NS::EmoInfo_S& stInfo)
{
    BlError_E enRetCode = OK;

    int  nSize = 0;
    bool bRet  = false;
    int  nTemp = 0;

    Json::Object* pJsonHandle       = NULL;
    Json::Object* pDataObject       = NULL;
    Json::Object* pArrayObject      = NULL;
    Json::Object* pItemObject       = NULL;
    Json::Object* pArrayArrayObject = NULL;
    Json::Object* pArrayItemObject  = NULL;


    /*创建操作句柄*/
    pJsonHandle = Json::init(pData);
    if (NULL == pJsonHandle)
    {
        dlog(LOG_ERROR, "传入的Json字符串有问题, 无法创建句柄");
        enRetCode = ERR_PARSE;
        goto EXIT;
    }

    /* 获取头数据信息 */
    enRetCode = get_headInfo(pJsonHandle, stInfo.stHeadInfo);
    if (enRetCode < OK)
    {
        dlog(LOG_ERROR, "获取头数据信息-失败");
        enRetCode = ERR_PARSE;
        goto EXIT;
    }

    /* 获取数据节点 */
    pDataObject = Json::get(pJsonHandle, "Datas");
    if (NULL == pDataObject)
    {
        dlog(LOG_ERROR, "获取[Datas]数据节点-失败");
        enRetCode = ERR_PARSE;
        goto EXIT;
    }

    /* 获取数组节点 */
    pArrayObject = Json::get(pDataObject, "BaseData");
    if (NULL == pArrayObject)
    {
        dlog(LOG_ERROR, "获取[BaseData]数据节点-失败");
        enRetCode = ERR_PARSE;
        goto EXIT;
    }

    /* 获取数组大小 */
    nSize = Json::Array::size(pArrayObject);
    if (nSize <= 0)
    {
        goto EXIT;
    }

    stInfo.listEmoInfo.clear();
    for (int i = 0; i < nSize; i++)
    {
        AiManage_NS::EmoItemInfo_S stItemInfo;

        /* 获取数组的节点 */
        pItemObject = Json::Array::get(pArrayObject, i);
        if (nullptr == pItemObject)
        {
            dlog(LOG_ERROR, "获取数组节点失败 [%d]", i);
            enRetCode = ERR_PARSE;
            goto EXIT;
        }

        /* 获取数组节点 */
        pArrayArrayObject = Json::get(pItemObject, "Box");
        if (NULL == pArrayArrayObject)
        {
            dlog(LOG_ERROR, "获取[Box]数据节点-失败");
            enRetCode = ERR_PARSE;
            goto EXIT;
        }

        /* 获取框信息 */
        enRetCode = get_boxInfo(pArrayArrayObject, stItemInfo.stBoxInfo);
        if (enRetCode < OK)
        {
            dlog(LOG_ERROR, "获取框信息-失败");
            enRetCode = ERR_PARSE;
            goto EXIT;
        }

        /* 置信度 */
        bRet = Json::get(pItemObject, "Confidence", stItemInfo.fConfidence);
        if (!bRet)
        {
            dlog(LOG_ERROR, "获取[Confidence]数据节点-失败");
            enRetCode = ERR_PARSE;
            goto EXIT;
        }

        /* 表情类型 */
        bRet = Json::get(pItemObject, "Class", nTemp);
        if (!bRet)
        {
            dlog(LOG_ERROR, "获取[Class]数据节点-失败");
            enRetCode = ERR_PARSE;
            goto EXIT;
        }
        stItemInfo.enEmotion = (AiManage_NS::Emotion_E)nTemp;
        stInfo.listEmoInfo.push_back(stItemInfo);
    }

EXIT:

    if (pJsonHandle)
    {
        Json::deinit(pJsonHandle);
        pJsonHandle = NULL;
    }

    return enRetCode;
}

/* 解析数据-解析人脸识别信息 */
BlError_E CParseJson::parse(char* pData, AiManage_NS::FaceInfo_S& stInfo)
{
    BlError_E enRetCode = OK;

    int  nSize = 0;
    bool bRet  = false;

    Json::Object* pJsonHandle       = NULL;
    Json::Object* pDataObject       = NULL;
    Json::Object* pArrayObject      = NULL;
    Json::Object* pItemObject       = NULL;
    Json::Object* pArrayArrayObject = NULL;
    Json::Object* pArrayItemObject  = NULL;


    /*创建操作句柄*/
    pJsonHandle = Json::init(pData);
    if (NULL == pJsonHandle)
    {
        dlog(LOG_ERROR, "传入的Json字符串有问题, 无法创建句柄");
        enRetCode = ERR_PARSE;
        goto EXIT;
    }

    /* 获取头数据信息 */
    enRetCode = get_headInfo(pJsonHandle, stInfo.stHeadInfo);
    if (enRetCode < OK)
    {
        dlog(LOG_ERROR, "获取头数据信息-失败");
        enRetCode = ERR_PARSE;
        goto EXIT;
    }

    /* 获取数据节点 */
    pDataObject = Json::get(pJsonHandle, "Datas");
    if (NULL == pDataObject)
    {
        dlog(LOG_ERROR, "获取[Datas]数据节点-失败");
        enRetCode = ERR_PARSE;
        goto EXIT;
    }

    /* 获取数组节点 */
    pArrayObject = Json::get(pDataObject, "BaseData");
    if (NULL == pArrayObject)
    {
        dlog(LOG_ERROR, "获取[BaseData]数据节点-失败");
        enRetCode = ERR_PARSE;
        goto EXIT;
    }

    /* 获取数组大小 */
    nSize = Json::Array::size(pArrayObject);
    if (nSize <= 0)
    {
        goto EXIT;
    }

    stInfo.listFaceInfo.clear();
    for (int i = 0; i < nSize; i++)
    {
        AiManage_NS::FaceItemInfo_S stItemInfo;

        /* 获取数组的节点 */
        pItemObject = Json::Array::get(pArrayObject, i);
        if (nullptr == pItemObject)
        {
            dlog(LOG_ERROR, "获取数组节点失败 [%d]", i);
            enRetCode = ERR_PARSE;
            goto EXIT;
        }


        /* 获取数组节点 */
        pArrayArrayObject = Json::get(pItemObject, "Box");
        if (NULL == pArrayArrayObject)
        {
            dlog(LOG_ERROR, "获取[Box]数据节点-失败");
            enRetCode = ERR_PARSE;
            goto EXIT;
        }

        /* 获取框信息 */
        enRetCode = get_boxInfo(pArrayArrayObject, stItemInfo.stBoxInfo);
        if (enRetCode < OK)
        {
            dlog(LOG_ERROR, "获取框信息-失败");
            enRetCode = ERR_PARSE;
            goto EXIT;
        }

        /* 置信度 */
        bRet = Json::get(pItemObject, "Confidence", stItemInfo.fConfidence);
        if (!bRet)
        {
            dlog(LOG_ERROR, "获取[Confidence]数据节点-失败");
            enRetCode = ERR_PARSE;
            goto EXIT;
        }

        /* 唯一ID */
        bRet = Json::get(pItemObject, "NameId", stItemInfo.nId);
        if (!bRet)
        {
            dlog(LOG_ERROR, "获取[NameId]数据节点-失败");
            enRetCode = ERR_PARSE;
            goto EXIT;
        }

        stInfo.listFaceInfo.push_back(stItemInfo);
    }

EXIT:

    if (pJsonHandle)
    {
        Json::deinit(pJsonHandle);
        pJsonHandle = NULL;
    }

    return enRetCode;
}

/**
 * @brief 解析数据-解析轨迹识别信息
 * @param [char] *pData: 需要解析的数据
 * @param [AiManage_NS::TrackInfo_S&] stInfo: 轨迹识别信息
 * @return [*] BlError_E::OK 成功  其他失败
 * @note
 */
BlError_E CParseJson::parse(char* pData, AiManage_NS::TrackInfo_S& stInfo)
{
    BlError_E enRetCode = OK;

    int  nSize = 0;
    bool bRet  = false;

    Json::Object* pJsonHandle       = NULL;
    Json::Object* pDataObject       = NULL;
    Json::Object* pArrayObject      = NULL;
    Json::Object* pItemObject       = NULL;
    Json::Object* pArrayArrayObject = NULL;
    Json::Object* pArrayItemObject  = NULL;


    /*创建操作句柄*/
    pJsonHandle = Json::init(pData);
    if (NULL == pJsonHandle)
    {
        dlog(LOG_ERROR, "传入的Json字符串有问题, 无法创建句柄");
        enRetCode = ERR_PARSE;
        goto EXIT;
    }

    /* 获取头数据信息 */
    enRetCode = get_headInfo(pJsonHandle, stInfo.stHeadInfo);
    if (enRetCode < OK)
    {
        dlog(LOG_ERROR, "获取头数据信息-失败");
        enRetCode = ERR_PARSE;
        goto EXIT;
    }

    /* 获取数据节点 */
    pDataObject = Json::get(pJsonHandle, "Datas");
    if (NULL == pDataObject)
    {
        dlog(LOG_ERROR, "获取[Datas]数据节点-失败");
        enRetCode = ERR_PARSE;
        goto EXIT;
    }

    /* 获取数组节点 */
    pArrayObject = Json::get(pDataObject, "BaseData");
    if (NULL == pArrayObject)
    {
        dlog(LOG_ERROR, "获取[BaseData]数据节点-失败");
        enRetCode = ERR_PARSE;
        goto EXIT;
    }

    /* 获取数组大小 */
    nSize = Json::Array::size(pArrayObject);
    if (nSize <= 0)
    {
        goto EXIT;
    }

    stInfo.listTrackInfo.clear();
    for (int i = 0; i < nSize; i++)
    {
        AiManage_NS::TrackItemInfo_S stItemInfo;

        /* 获取数组的节点 */
        pItemObject = Json::Array::get(pArrayObject, i);
        if (nullptr == pItemObject)
        {
            dlog(LOG_ERROR, "获取数组节点失败 [%d]", i);
            enRetCode = ERR_PARSE;
            goto EXIT;
        }


        /* 获取数组节点 */
        pArrayArrayObject = Json::get(pItemObject, "Box");
        if (NULL == pArrayArrayObject)
        {
            dlog(LOG_ERROR, "获取[Box]数据节点-失败");
            enRetCode = ERR_PARSE;
            goto EXIT;
        }

        /* 获取框信息 */
        enRetCode = get_boxInfo(pArrayArrayObject, stItemInfo.stBoxInfo);
        if (enRetCode < OK)
        {
            dlog(LOG_ERROR, "获取框信息-失败");
            enRetCode = ERR_PARSE;
            goto EXIT;
        }

        /* 置信度 */
        bRet = Json::get(pItemObject, "Confidence", stItemInfo.fConfidence);
        if (!bRet)
        {
            dlog(LOG_ERROR, "获取[Confidence]数据节点-失败");
            enRetCode = ERR_PARSE;
            goto EXIT;
        }

        stInfo.listTrackInfo.push_back(stItemInfo);
    }

EXIT:

    if (pJsonHandle)
    {
        Json::deinit(pJsonHandle);
        pJsonHandle = NULL;
    }

    return enRetCode;
}

/**
 * @brief 解析数据-解析人数识别信息
 * @param [char] *pData: 需要解析的数据
 * @param [AiManage_NS::NumberInfo_S&] stInfo: 人数识别信息
 * @return [*] BlError_E::OK 成功  其他失败
 * @note
 */
BlError_E CParseJson::parse(char* pData, AiManage_NS::NumberInfo_S& stInfo)
{
    BlError_E enRetCode = OK;

    int  nSize = 0;
    bool bRet  = false;

    Json::Object* pJsonHandle       = NULL;
    Json::Object* pDataObject       = NULL;
    Json::Object* pArrayObject      = NULL;
    Json::Object* pItemObject       = NULL;
    Json::Object* pArrayArrayObject = NULL;
    Json::Object* pArrayItemObject  = NULL;

    /*创建操作句柄*/
    pJsonHandle = Json::init(pData);
    if (NULL == pJsonHandle)
    {
        dlog(LOG_ERROR, "传入的Json字符串有问题, 无法创建句柄");
        enRetCode = ERR_PARSE;
        goto EXIT;
    }

    /* 获取头数据信息 */
    enRetCode = get_headInfo(pJsonHandle, stInfo.stHeadInfo);
    if (enRetCode < OK)
    {
        dlog(LOG_ERROR, "获取头数据信息-失败");
        enRetCode = ERR_PARSE;
        goto EXIT;
    }

    /* 获取数据节点 */
    pDataObject = Json::get(pJsonHandle, "Datas");
    if (NULL == pDataObject)
    {
        dlog(LOG_ERROR, "获取[Datas]数据节点-失败");
        enRetCode = ERR_PARSE;
        goto EXIT;
    }

    /* 获取数组节点 */
    pArrayObject = Json::get(pDataObject, "BaseData");
    if (NULL == pArrayObject)
    {
        dlog(LOG_ERROR, "获取[BaseData]数据节点-失败");
        enRetCode = ERR_PARSE;
        goto EXIT;
    }

    /* 获取数组大小 */
    nSize = Json::Array::size(pArrayObject);

    if (nSize <= 0)
    {
        stInfo.nTotal = 0;
        goto EXIT;
    }

    /* 人数总数 */
    stInfo.nTotal = nSize;



    stInfo.listNumberInfo.clear();
    for (int i = 0; i < nSize; i++)
    {
        AiManage_NS::NumberItemInfo_S stItemInfo;

        /* 获取数组的节点 */
        pItemObject = Json::Array::get(pArrayObject, i);
        if (nullptr == pItemObject)
        {
            dlog(LOG_ERROR, "获取数组节点失败 [%d]", i);
            enRetCode = ERR_PARSE;
            goto EXIT;
        }


        /* 获取数组节点 */
        pArrayArrayObject = Json::get(pItemObject, "Box");
        if (NULL == pArrayArrayObject)
        {
            dlog(LOG_ERROR, "获取[Box]数据节点-失败");
            enRetCode = ERR_PARSE;
            goto EXIT;
        }

        /* 获取框信息 */
        enRetCode = get_boxInfo(pArrayArrayObject, stItemInfo.stBoxInfo);
        if (enRetCode < OK)
        {
            dlog(LOG_ERROR, "获取框信息-失败");
            enRetCode = ERR_PARSE;
            goto EXIT;
        }

        stInfo.listNumberInfo.push_back(stItemInfo);
    }

EXIT:

    if (pJsonHandle)
    {
        Json::deinit(pJsonHandle);
        pJsonHandle = NULL;
    }

    return enRetCode;
}

/**
 * @brief 解析数据-解析行为识别信息
 * @param [char] *pData: 需要解析的数据
 * @param [AiManage_NS::BehaviorInfo_S&] stInfo: 行为识别信息
 * @return [*] BlError_E::OK 成功  其他失败
 * @note
 */
BlError_E CParseJson::parse(char* pData, AiManage_NS::BehaviorInfo_S& stInfo)
{
    BlError_E enRetCode = OK;

    int  nSize = 0;
    bool bRet  = false;
    int  nTemp = 0;

    Json::Object* pJsonHandle       = NULL;
    Json::Object* pDataObject       = NULL;
    Json::Object* pArrayObject      = NULL;
    Json::Object* pItemObject       = NULL;
    Json::Object* pArrayArrayObject = NULL;
    Json::Object* pArrayItemObject  = NULL;


    /*创建操作句柄*/
    pJsonHandle = Json::init(pData);
    if (NULL == pJsonHandle)
    {
        dlog(LOG_ERROR, "传入的Json字符串有问题, 无法创建句柄");
        enRetCode = ERR_PARSE;
        goto EXIT;
    }

    /* 获取头数据信息 */
    enRetCode = get_headInfo(pJsonHandle, stInfo.stHeadInfo);
    if (enRetCode < OK)
    {
        dlog(LOG_ERROR, "获取头数据信息-失败");
        enRetCode = ERR_PARSE;
        goto EXIT;
    }

    /* 获取数据节点 */
    pDataObject = Json::get(pJsonHandle, "Datas");
    if (NULL == pDataObject)
    {
        dlog(LOG_ERROR, "获取[Datas]数据节点-失败");
        enRetCode = ERR_PARSE;
        goto EXIT;
    }

    /* 获取数组节点 */
    pArrayObject = Json::get(pDataObject, "BaseData");
    if (NULL == pArrayObject)
    {
        dlog(LOG_ERROR, "获取[BaseData]数据节点-失败");
        enRetCode = ERR_PARSE;
        goto EXIT;
    }

    /* 获取数组大小 */
    nSize = Json::Array::size(pArrayObject);
    if (nSize <= 0)
    {
        goto EXIT;
    }

    stInfo.listBehaviorInfo.clear();
    for (int i = 0; i < nSize; i++)
    {
        AiManage_NS::BehaviorItemInfo_S stItemInfo;

        /* 获取数组的节点 */
        pItemObject = Json::Array::get(pArrayObject, i);
        if (nullptr == pItemObject)
        {
            dlog(LOG_ERROR, "获取数组节点失败 [%d]", i);
            enRetCode = ERR_PARSE;
            goto EXIT;
        }

        /* 获取数组节点 */
        pArrayArrayObject = Json::get(pItemObject, "Box");
        if (NULL == pArrayArrayObject)
        {
            dlog(LOG_ERROR, "获取[Box]数据节点-失败");
            enRetCode = ERR_PARSE;
            goto EXIT;
        }

        /* 获取框信息 */
        enRetCode = get_boxInfo(pArrayArrayObject, stItemInfo.stBoxInfo);
        if (enRetCode < OK)
        {
            dlog(LOG_ERROR, "获取框信息-失败");
            enRetCode = ERR_PARSE;
            goto EXIT;
        }

        /* 置信度 */
        bRet = Json::get(pItemObject, "Confidence", stItemInfo.fConfidence);
        if (!bRet)
        {
            dlog(LOG_ERROR, "获取[Confidence]数据节点-失败");
            enRetCode = ERR_PARSE;
            goto EXIT;
        }

        /* 表情类型 */
        bRet = Json::get(pItemObject, "Class", nTemp);
        if (!bRet)
        {
            dlog(LOG_ERROR, "获取[Class]数据节点-失败");
            enRetCode = ERR_PARSE;
            goto EXIT;
        }

        switch (nTemp)
        {
            /* 低头 */
            case 0:
            {
                stItemInfo.enAction = AiManage_NS::LOWER_HEAD;
                break;
            }
            /* 抬头 */
            case 1:
            {
                stItemInfo.enAction = AiManage_NS::LIFT_HEAD;
                break;
            }
            /* 转头 */
            case 2:
            {
                stItemInfo.enAction = AiManage_NS::TURN_HEAD;
                break;
            }
            /* 举手 */
            case 3:
            {
                stItemInfo.enAction = AiManage_NS::RAISE_HAND;
                break;
            }
            /* 站立 */
            case 4:
            {
                stItemInfo.enAction = AiManage_NS::STAND;
                break;
            }
            /* 转身 */
            case 5:
            {
                stItemInfo.enAction = AiManage_NS::TURN;
                break;
            }
            /* 趴桌 */
            case 6:
            {
                stItemInfo.enAction = AiManage_NS::DOWN_DESK;
                break;
            }
            /* 玩手机 */
            case 7:
            {
                stItemInfo.enAction = AiManage_NS::PLAY_PHONE;
                break;
            }
            /* 接打电话 */
            case 8:
            {
                stItemInfo.enAction = AiManage_NS::CALL_PHONE;
                break;
            }
            /* 教师板书 */
            case 9:
            {
                stItemInfo.enAction = AiManage_NS::TEA_BOARD;
                break;
            }
            default:
            {
                stItemInfo.enAction = AiManage_NS::ACTION_NULL;
                break;
            }
        }

        stInfo.listBehaviorInfo.push_back(stItemInfo);
    }

EXIT:

    if (pJsonHandle)
    {
        Json::deinit(pJsonHandle);
        pJsonHandle = NULL;
    }

    return enRetCode;
}

/*返回值解析*/
BlError_E CParseJson::parse(char* pData, int& nReturn)
{
    BlError_E enRetCode = OK;

    int           nSize       = 0;
    bool          bRet        = false;
    Json::Object* pJsonHandle = NULL;

    /*创建操作句柄*/
    pJsonHandle = Json::init(pData);
    if (NULL == pJsonHandle)
    {
        dlog(LOG_ERROR, "传入的Json字符串有问题, 无法创建句柄");
        enRetCode = ERR_PARSE;
        goto EXIT;
    }

    /* 返回值 */
    bRet = Json::get(pJsonHandle, "Return", nReturn);
    if (!bRet)
    {
        dlog(LOG_ERROR, "获取[Return]数据节点-失败");
        enRetCode = ERR_PARSE;
        goto EXIT;
    }

EXIT:

    if (pJsonHandle)
    {
        Json::deinit(pJsonHandle);
        pJsonHandle = NULL;
    }

    return enRetCode;
}

/*课堂纪律解析*/
BlError_E CParseJson::parse(char* pData, AiManage_NS::MoveProbability_S& stMoveProbability)
{
    BlError_E enRetCode = OK;

    int  nSize = 0;
    bool bRet  = false;

    Json::Object* pJsonHandle       = NULL;
    Json::Object* pDataObject       = NULL;
    Json::Object* pItemObject       = NULL;

    /*创建操作句柄*/
    pJsonHandle = Json::init(pData);
    if (NULL == pJsonHandle)
    {
        dlog(LOG_ERROR, "传入的Json字符串有问题, 无法创建句柄");
        enRetCode = ERR_PARSE;
        goto EXIT;
    }

    /* 获取头数据信息 */
    enRetCode = get_headInfo(pJsonHandle, stMoveProbability.stHeadInfo);
    if (enRetCode < OK)
    {
        dlog(LOG_ERROR, "获取头数据信息-失败");
        enRetCode = ERR_PARSE;
        goto EXIT;
    }

    /* 获取数据节点 */
    pDataObject = Json::get(pJsonHandle, "Datas");
    if (NULL == pDataObject)
    {
        dlog(LOG_ERROR, "获取[Datas]数据节点-失败");
        enRetCode = ERR_PARSE;
        goto EXIT;
    }

    /* 获取数据节点 */
    pItemObject = Json::get(pDataObject, "MoveProbability");
    if (NULL == pItemObject)
    {
        dlog(LOG_ERROR, "获取[MoveProbability]数据节点-失败");
        enRetCode = ERR_PARSE;
        goto EXIT;
    }
    stMoveProbability.fMoveProbability = pItemObject->valuedouble;

EXIT:

    if (pJsonHandle)
    {
        Json::deinit(pJsonHandle);
        pJsonHandle = NULL;
    }

    return enRetCode;
}

/* ip获取解析 */
BlError_E CParseJson::parse(char* pData, std::string& strAiServerIp)
{
    BlError_E enRetCode = OK;

    int           nSize       = 0;
    bool          bRet        = false;
    Json::Object* pJsonHandle = NULL;
    Json::Object* pDataObject = NULL;

    /*创建操作句柄*/
    pJsonHandle = Json::init(pData);
    if (NULL == pJsonHandle)
    {
        dlog(LOG_ERROR, "传入的Json字符串有问题, 无法创建句柄");
        enRetCode = ERR_PARSE;
        goto EXIT;
    }

    /*获取数据节点*/
    pDataObject = Json::get(pJsonHandle, "Data");
    if (NULL == pDataObject)
    {
        dlog(LOG_ERROR, "获取[Data]数据节点-失败");
        enRetCode = ERR_PARSE;
        goto EXIT;
    }

    /* IP */
    bRet = Json::get(pDataObject, "IP", strAiServerIp);
    if (!bRet)
    {
        dlog(LOG_ERROR, "获取[IP]数据节点-失败");
        enRetCode = ERR_PARSE;
        goto EXIT;
    }

EXIT:

    if (pJsonHandle)
    {
        Json::deinit(pJsonHandle);
        pJsonHandle = NULL;
    }

    return enRetCode;
}

/* AI心跳解析 */
BlError_E CParseJson::parse(char* pData, AiManage_NS::VodHeartInfo_S& stVodHeartInfo)
{
    BlError_E enRetCode = OK;

    int           nSize       = 0;
    bool          bRet        = false;
    Json::Object* pJsonHandle = NULL;
    Json::Object* pDataObject = NULL;

    /*创建操作句柄*/
    pJsonHandle = Json::init(pData);
    if (NULL == pJsonHandle)
    {
        dlog(LOG_ERROR, "传入的Json字符串有问题, 无法创建句柄\n%s", pData);
        enRetCode = ERR_PARSE;
        goto EXIT;
    }

    /*获取数据节点*/
    pDataObject = Json::get(pJsonHandle, "Data");
    if (NULL == pDataObject)
    {
        dlog(LOG_ERROR, "获取[Data]数据节点-失败\n%s", pData);
        enRetCode = ERR_PARSE;
        goto EXIT;
    }

    /* IP */
    bRet = Json::get(pDataObject, "DevCount", stVodHeartInfo.nDevCount);
    if (!bRet)
    {
        dlog(LOG_ERROR, "获取[IP]数据节点-失败\n%s", pData);
        enRetCode = ERR_PARSE;
        goto EXIT;
    }

EXIT:

    if (pJsonHandle)
    {
        Json::deinit(pJsonHandle);
        pJsonHandle = NULL;
    }

    return enRetCode;
}

/* 转换-设备信息*/
BlError_E CParseJson::convert(AiManage_NS::DevInfo_S stDevInfo, std::string& strOutJson, int nCode)
{
    BlError_E enRetCode = OK;
    auto      pRootJson = Json::init();
    auto      pDataJson = Json::init();

    /* 添加数据 */
    Json::add(pDataJson, "DevModel", stDevInfo.strDevModel);
    Json::add(pDataJson, "IP", stDevInfo.strDevIp);
    Json::add(pDataJson, "Mac", stDevInfo.strDevMac);

    Json::add(pRootJson, "ActionCode", nCode);
    Json::add(pRootJson, "UserName", "admin");
    Json::add(pRootJson, "OptType", 0);
    Json::add(pRootJson, "Data", pDataJson);
    Json::add(pRootJson, "Return", 0);

    /* 转换成字符串 */
    strOutJson = Json::to_string(pRootJson);

    /* 释放数据 */
    Json::deinit(pRootJson);

    return enRetCode;
}

BlError_E ParseData_NS::CParseJson::convert(int nReturn, std::string& strOutJson, int nCode)
{
    BlError_E enRetCode = OK;
    auto      pRootJson = Json::init();

    /* 添加数据 */
    Json::add(pRootJson, "ActionCode", nCode);
    Json::add(pRootJson, "UserName", "admin");
    Json::add(pRootJson, "OptType", 0);
    Json::add(pRootJson, "Return", 0);

    /* 转换成字符串 */
    strOutJson = Json::to_string(pRootJson);

    /* 释放数据 */
    Json::deinit(pRootJson);

    return enRetCode;
}

/* 转换数据-截图命令 */
BlError_E ParseData_NS::CParseJson::convert(std::string strPicPath, int nChannelNumber, std::string& strOutJson, int nCode)
{
    BlError_E enRetCode = OK;
    auto      pRootJson = Json::init();
    auto      pDataJson = Json::init();

    /* 添加数据 */
    Json::add(pDataJson, "PicPath", strPicPath);
    Json::add(pDataJson, "Channel", nChannelNumber);

    Json::add(pRootJson, "ActionCode", nCode);
    Json::add(pRootJson, "UserName", "admin");
    Json::add(pRootJson, "Data", pDataJson);

    /* 转换成字符串 */
    strOutJson = Json::to_string(pRootJson);

    /* 释放数据 */
    Json::deinit(pRootJson);

    return enRetCode;
}

/* 转换数据-更新班级人脸信息命令-发送给AI服务器 */
BlError_E ParseData_NS::CParseJson::convert(int nClassId, std::string strTarPath, std::string& strOutJson, int nCode)
{
    BlError_E enRetCode = OK;
    auto      pRootJson = Json::init();
    auto      pDataJson = Json::init();

    /* 添加数据 */
    Json::add(pDataJson, "ClassId", nClassId);
    Json::add(pDataJson, "TarFilePath", strTarPath);

    Json::add(pRootJson, "ActionCode", nCode);
    Json::add(pRootJson, "UserName", "admin");
    Json::add(pRootJson, "OptType", 1);
    Json::add(pRootJson, "Data", pDataJson);

    /* 转换成字符串 */
    strOutJson = Json::to_string(pRootJson);

    /* 释放数据 */
    Json::deinit(pRootJson);

    return enRetCode;
}

/* 转换数据-更新班级人脸信息命令-发送给网页 */
BlError_E ParseData_NS::CParseJson::convert(PlatformManage_NS::DataInfo_S stDataInfo, std::string strFilePath, std::string& strOutJson, int nCode)
{
    BlError_E enRetCode = OK;
    auto      pRootJson = Json::init();

    /* 添加数据 */
    auto pTecArray = Json::Array::init();
    for (auto item : stDataInfo.listTeaInfo)
    {
        auto pTmpJson = Json::init();
        Json::add(pTmpJson, "ID", item.nId);
        Json::add(pTmpJson, "Name", item.strName);
        Json::add(pTmpJson, "Path", item.strPath);
        Json::add(pTmpJson, "Md5", item.strMd5);
        Json::add(pTmpJson, "FileName", std::to_string(item.nId) + "_" + item.strMd5);
        Json::Array::add(pTecArray, pTmpJson);
    }

    auto pStuArray = Json::Array::init();
    for (auto item : stDataInfo.listStuInfo)
    {
        auto pTmpJson = Json::init();
        Json::add(pTmpJson, "ID", item.nId);
        Json::add(pTmpJson, "Name", item.strName);
        Json::add(pTmpJson, "Path", item.strPath);
        Json::add(pTmpJson, "Md5", item.strMd5);
        Json::add(pTmpJson, "FileName", std::to_string(item.nId) + "_" + item.strMd5);
        Json::Array::add(pStuArray, pTmpJson);
    }

    Json::add(pRootJson, "FilePath", strFilePath);
    Json::add(pRootJson, "Teacher", pTecArray);
    Json::add(pRootJson, "Student", pStuArray);

    /* 转换成字符串 */
    strOutJson = Json::to_string(pRootJson);

    /* 释放数据 */
    Json::deinit(pRootJson);

    return enRetCode;
}

/* 获取头数据信息 */
BlError_E CParseJson::get_headInfo(Json::Object*& pObject, AiManage_NS::HeadInfo_S& stInfo)
{
    if (NULL == pObject)
    {
        return ERR_IN_PARAM_NULL;
    }

    BlError_E enRetCode = OK;

    bool bRet = false;

    /* 获取算法类型 */
    bRet = Json::get(pObject, "Mode", stInfo.nMode);
    if (!bRet)
    {
        dlog(LOG_ERROR, "获取[Mode]数据节点-失败");
        enRetCode = ERR_PARSE;
        goto EXIT;
    }

    /* 获取当前录制时长 */
    bRet = Json::get(pObject, "CurRecordTime", stInfo.nRecordTime);
    if (!bRet)
    {
        dlog(LOG_ERROR, "获取[CurRecordTime]数据节点-失败");
        enRetCode = ERR_PARSE;
        goto EXIT;
    }

    /* 获取班级id */
    bRet = Json::get(pObject, "ClassId", stInfo.nClassId);
    if (!bRet)
    {
        dlog(LOG_ERROR, "获取[ClassId]数据节点-失败");
        enRetCode = ERR_PARSE;
        goto EXIT;
    }

    /* 获取当前时间戳 */
    bRet = Json::get(pObject, "Timestamp", stInfo.lTimestamp);
    if (!bRet)
    {
        dlog(LOG_ERROR, "获取[Timestamp]数据节点-失败");
        enRetCode = ERR_PARSE;
        goto EXIT;
    }

EXIT:

    return enRetCode;
}

/* 获取框信息 */
BlError_E CParseJson::get_boxInfo(Json::Object*& pArrayObject, AiManage_NS::BoxInfo_S& stInfo)
{
    if (NULL == pArrayObject)
    {
        return ERR_IN_PARAM_NULL;
    }

    BlError_E enRetCode = OK;

    bool bRet  = false;
    int  nSize = 0;

    Json::Object* pItemObject = nullptr;


    /* 获取数组大小 */
    nSize = Json::Array::size(pArrayObject);
    if (nSize > 4)
    {
        dlog(LOG_ERROR, "数组大小异常[%d]", nSize);
        enRetCode = ERR_PARSE;
        goto EXIT;
    }

    /* 获取数组的节点 */
    pItemObject = Json::Array::get(pArrayObject, 0);
    if (nullptr == pItemObject)
    {
        dlog(LOG_ERROR, "获取数组节点失败 [0]");
        enRetCode = ERR_PARSE;
        goto EXIT;
    }
    /* 坐标X1 */
    bRet = Json::get(pItemObject, stInfo.nX1);
    if (!bRet)
    {
        dlog(LOG_ERROR, "获取[0]数据节点-失败");
        enRetCode = ERR_PARSE;
        goto EXIT;
    }




    /* 获取数组的节点 */
    pItemObject = Json::Array::get(pArrayObject, 1);
    if (nullptr == pItemObject)
    {
        dlog(LOG_ERROR, "获取数组节点失败 [1]");
        enRetCode = ERR_PARSE;
        goto EXIT;
    }
    /* 坐标Y1 */
    bRet = Json::get(pItemObject, stInfo.nY1);
    if (!bRet)
    {
        dlog(LOG_ERROR, "获取[1]数据节点-失败");
        enRetCode = ERR_PARSE;
        goto EXIT;
    }



    /* 获取数组的节点 */
    pItemObject = Json::Array::get(pArrayObject, 2);
    if (nullptr == pItemObject)
    {
        dlog(LOG_ERROR, "获取数组节点失败 [2]");
        enRetCode = ERR_PARSE;
        goto EXIT;
    }
    /* 坐标X2 */
    bRet = Json::get(pItemObject, stInfo.nX2);
    if (!bRet)
    {
        dlog(LOG_ERROR, "获取[2]数据节点-失败");
        enRetCode = ERR_PARSE;
        goto EXIT;
    }



    /* 获取数组的节点 */
    pItemObject = Json::Array::get(pArrayObject, 3);
    if (nullptr == pItemObject)
    {
        dlog(LOG_ERROR, "获取数组节点失败 [3]");
        enRetCode = ERR_PARSE;
        goto EXIT;
    }
    /* 坐标Y2 */
    bRet = Json::get(pItemObject, stInfo.nY2);
    if (!bRet)
    {
        dlog(LOG_ERROR, "获取[3]数据节点-失败");
        enRetCode = ERR_PARSE;
        goto EXIT;
    }

EXIT:

    return enRetCode;
}
