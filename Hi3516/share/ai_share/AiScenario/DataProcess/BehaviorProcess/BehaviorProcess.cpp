/*
 * @FilePath     : BehaviorProcess.cpp
 * @Author       : 严泽辉 yanzeh@kfb.cn
 * @Date         : 2024-08-02 11:15:19
 * @LastEditors: weigl 308241951@qq.com
 * @LastEditTime: 2025-02-26 17:01:03
 * @Description  :
 */

#include "BehaviorProcess.hpp"

#include <chrono>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <sys/stat.h>
#include <sys/types.h>

using namespace AiScenario_NS;
/* 老师接打电话识别 */
#define AI_COM_TEA_CALLPHONE1 10008  

/* 处理基础行为数据 */
bool CBehaviorProcess::process(BehaviorParam_S stBehaviorParam,
                               std::string&    strInJson,
                               std::string&    strOutJson,
                               bool            isDelKeyJson)
{

    int  nSize = 0;
    bool bRet  = true;
    int  nTemp = 0;

    std::vector<KeyPosInfo_S> vecTemKeyPosInfo;

    Json::Object* pJsonHandle        = NULL;
    Json::Object* pDataObject        = NULL;
    Json::Object* pArrayObject       = NULL;
    Json::Object* pItemObject        = NULL;
    Json::Object* pArrayArrayObject  = NULL;
    Json::Object* pArrayArrayObject2 = NULL;
    Json::Object* pArrayItemObject   = NULL;

    /*创建操作句柄*/
    pJsonHandle = Json::init(strInJson);
    if (NULL == pJsonHandle)
    {
        dlog(LOG_ERROR, "传入的Json字符串有问题, 无法创建句柄\n%s", strInJson.c_str());
        bRet = false;
        goto EXIT;
    }

    /* 兼容有些数据没有Datas，只有BaseData */
    /* 获取数据节点 */
    pDataObject = Json::get(pJsonHandle, "Datas");
    if (NULL == pDataObject)
    {
        /* 获取数组节点 */
        pArrayObject = Json::get(pJsonHandle, "BaseData");
        if (NULL == pArrayObject)
        {
            dlog(LOG_ERROR, "获取[BaseData]数据节点-失败\n%s", strInJson.c_str());
            bRet = false;
            goto EXIT;
        }
    }
    else
    {
        /* 获取数组节点 */
        pArrayObject = Json::get(pDataObject, "BaseData");
        if (NULL == pArrayObject)
        {
            dlog(LOG_ERROR, "获取[BaseData]数据节点-失败\n%s", strInJson.c_str());
            bRet = false;
            goto EXIT;
        }
    }

    /* 获取数组大小 */
    nSize = Json::Array::size(pArrayObject);
    if (nSize <= 0)
    {
        goto EXIT;
    }
    for (int i = 0; i < nSize; i++)
    {
        BoxInfo_S stBoxInfo;
        /* 获取数组的节点 */
        pItemObject = Json::Array::get(pArrayObject, i);
        if (nullptr == pItemObject)
        {
            dlog(LOG_ERROR, "获取数组节点失败 [%d]", i);
            bRet = false;
            goto EXIT;
        }

        /* 获取数组节点 */
        nTemp              = -1;
        pArrayArrayObject  = Json::get(pItemObject, "Keypoints");
        pArrayArrayObject2 = Json::get(pItemObject, "KpScore");
        if (NULL != pArrayArrayObject2 &&
            NULL != pArrayArrayObject)
        {
            /* 获取关键点信息 */
            bRet = get_keypoints(pArrayArrayObject, pArrayArrayObject2, vecTemKeyPosInfo);
            if (!bRet)
            {
                dlog(LOG_ERROR, "获取关键点信息-失败\n%s", strInJson.c_str());
                bRet = false;
                goto EXIT;
            }
            nTemp = getBehavioralType(stBehaviorParam, vecTemKeyPosInfo);
            /* 更新Class数据 */
            Json::update(pItemObject, "Class", nTemp);
            if (isDelKeyJson)
            {
                /* 删除不用的数据 */
                Json::remove(pItemObject, "Keypoints");
                Json::remove(pItemObject, "KpScore");
            }
        }
    }

EXIT:

    if (pJsonHandle)
    {
        if (bRet)
        {
            /* 转换成字符串 */
            strOutJson = Json::to_string(pJsonHandle);
        }

        Json::deinit(pJsonHandle);
        pJsonHandle = nullptr;
    }

    return bRet;
}

/* 获取当前行为类型 */
int CBehaviorProcess::getBehavioralType(
    BehaviorParam_S           stBehaviorParam,
    std::vector<KeyPosInfo_S> vecInfo)
{

    if (vecInfo.size() != 26)
    {
        return -1;
    }

    KeyPosInfo_S& stNose      = vecInfo[0];  /* 鼻子 */
    KeyPosInfo_S& stREye      = vecInfo[1];  /* 右眼 */
    KeyPosInfo_S& stLEye      = vecInfo[2];  /* 左眼 */
    KeyPosInfo_S& stREar      = vecInfo[3];  /* 右耳朵 */
    KeyPosInfo_S& stLEar      = vecInfo[4];  /* 左耳朵 */
    KeyPosInfo_S& stRShoulder = vecInfo[5];  /* 右肩 */
    KeyPosInfo_S& stLShoulder = vecInfo[6];  /* 左肩 */
    KeyPosInfo_S& stRElbow    = vecInfo[7];  /* 右手肘 */
    KeyPosInfo_S& stLElbow    = vecInfo[8];  /* 左手肘 */
    KeyPosInfo_S& stRWrist    = vecInfo[9];  /* 右手腕 */
    KeyPosInfo_S& stLWrist    = vecInfo[10]; /* 左手腕 */
    KeyPosInfo_S& stRHip      = vecInfo[11]; /* 右臀 */
    KeyPosInfo_S& stLHip      = vecInfo[12]; /* 左臀 */
    KeyPosInfo_S& stRKnee     = vecInfo[13]; /* 右膝盖 */
    KeyPosInfo_S& stLKnee     = vecInfo[14]; /* 左膝盖 */
    KeyPosInfo_S& stRAnkle    = vecInfo[15]; /* 右脚踝 */
    KeyPosInfo_S& stLAnkle    = vecInfo[16]; /* 左脚踝 */
    KeyPosInfo_S& stHead      = vecInfo[17]; /* 头顶 */
    KeyPosInfo_S& stNeck      = vecInfo[18]; /* 颈部 */
    KeyPosInfo_S& stHip       = vecInfo[19]; /* 臀部 */

/* 数据优化 */
#if 1
    double dScore = 0.8;
    /* 手臂特征点只要有二个超过阈值，全部设置为阈值以上 */
    /* 计算大于阈值的数的数量 */
    int    nCount = (stLShoulder.dScore > dScore) +
        (stLElbow.dScore > dScore) +
        (stLWrist.dScore > dScore);
    if (nCount >= 2)
    {
        stLShoulder.dScore = (dScore + 0.1);
        stLElbow.dScore    = (dScore + 0.1);
        stLWrist.dScore    = (dScore + 0.1);
    }

    nCount = (stRShoulder.dScore > dScore) +
        (stRElbow.dScore > dScore) +
        (stRWrist.dScore > dScore);
    if (nCount >= 2)
    {
        stRShoulder.dScore = (dScore + 0.1);
        stRElbow.dScore    = (dScore + 0.1);
        stRWrist.dScore    = (dScore + 0.1);
    }

    /* 耳朵特征点超过阈值，眼睛特征点设置为阈值以上 */
    if (stREar.dScore > dScore && stLEar.dScore > dScore)
    {
        stREye.dScore = (dScore + 0.1);
        stLEye.dScore = (dScore + 0.1);
    }

#endif

    /*
    种类，{
    0:'低头',
    1:'抬头',
    2:'转头',
    3:'举手',
    4:'站立',
    5:'转身',
    6:'趴桌'
    }
    */


    /* 趴桌判断 */
    if (isLyingOnDesk(vecInfo, stBehaviorParam.stLyingOnDesk))
    {
        return 6;
    }

    /* 转身判断 */
    if (isTurningBody(vecInfo, stBehaviorParam.stTurningBody))
    {
        return 5;
    }

    /* 站立判断 */
    if (isStanding(vecInfo, stBehaviorParam.stStanding))
    {
        return 4;
    }

    /* 举手判断 */
    if (isRaisingHand(vecInfo, stBehaviorParam.stRaisingHand))
    {
        return 3;
    }

    /* 低头判断 */
    if (isBendingHead(vecInfo, stBehaviorParam.stBendingHead))
    {
        return 0;
    }
    /* 转头判断 */
    else if (isTurningHead(vecInfo, stBehaviorParam.stTurningHead))
    {
        return 2;
    }
    /* 抬头判断 */
    else if (isRaisingHead(vecInfo, stBehaviorParam.stRaisingHead))
    {
        return 1;
    }

    return -1;
}

/* 处理基础行为数据 */
bool CBehaviorProcess::process(SupClsInsParam_S stSupClsInsParam,
                               std::string&    strInJson,
                               std::string&    strOutJson,
                               bool            isDelKeyJson)
{

    int  nSize = 0;
    bool bRet  = true;
    int  nTemp = 0;
    BoxInfo_S stLargestBox;
    int nMaxArea = 0; // 当前最大面积
    int nMaxIndex = 0;
    
    int nMode = -1;

    std::vector<KeyPosInfo_S> vecTemKeyPosInfo;

    Json::Object* pJsonHandle        = NULL;
    Json::Object* pDataObject        = NULL;
    Json::Object* pArrayObject       = NULL;
    Json::Object* pItemObject        = NULL;
    Json::Object* pArrayArrayObject  = NULL;
    Json::Object* pArrayArrayObject2 = NULL;
    Json::Object* pArrayItemObject   = NULL;
    
    /*创建操作句柄*/
    pJsonHandle = Json::init(strInJson);
    if (NULL == pJsonHandle)
    {
        dlog(LOG_ERROR, "传入的Json字符串有问题, 无法创建句柄\n%s", strInJson.c_str());
        bRet = false;
        goto EXIT;
    }
    /* 获取数据节点 */
    bRet = Json::get(pJsonHandle, "Mode", nMode);
    if (!bRet)
    {
        dlog(LOG_ERROR, "获取[Mode]数据节点-失败\n%s", strInJson.c_str());
        bRet = false;
        goto EXIT;
    }

    /* 兼容有些数据没有Datas，只有BaseData */
    /* 获取数据节点 */
    pDataObject = Json::get(pJsonHandle, "Datas");
    if (NULL == pDataObject)
    {
        /* 获取数组节点 */
        pArrayObject = Json::get(pJsonHandle, "BaseData");
        if (NULL == pArrayObject)
        {
            dlog(LOG_ERROR, "获取[BaseData]数据节点-失败\n%s", strInJson.c_str());
            bRet = false;
            goto EXIT;
        }
    }
    else
    {
        /* 获取数组节点 */
        pArrayObject = Json::get(pDataObject, "BaseData");
        if (NULL == pArrayObject)
        {
            dlog(LOG_ERROR, "获取[BaseData]数据节点-失败\n%s", strInJson.c_str());
            bRet = false;
            goto EXIT;
        }
    }

    if (nMode == AI_COM_TEA_CALLPHONE1)
    {
        /* 获取数组大小 */
        nSize = Json::Array::size(pArrayObject);
        if (nSize <= 0)
        {
            goto EXIT;
        }
        for (int i = 0; i < nSize; i++)
        {
            BoxInfo_S stBoxInfo;
            /* 获取数组的节点 */
            pItemObject = Json::Array::get(pArrayObject, i);
            if (nullptr == pItemObject)
            {
                dlog(LOG_ERROR, "获取数组节点失败 [%d]", i);
                bRet = false;
                goto EXIT;
            }

            /* 获取数组节点 */
            pArrayArrayObject = Json::get(pItemObject, "Box");
            if (NULL == pArrayArrayObject)
            {
                dlog(LOG_ERROR, "获取[Box]数据节点-失败\n%s", strInJson.c_str());
                bRet = false;
                goto EXIT;
            }

            /* 获取框信息 */
            bRet = get_boxInfo(pArrayArrayObject, stBoxInfo);
            if (!bRet)
            {
                dlog(LOG_ERROR, "获取框信息-失败\n%s", strInJson.c_str());
                bRet = false;
                goto EXIT;
            }
            
            /* 计算框的面积 */
            int nCurrentArea = (stBoxInfo.nX2 - stBoxInfo.nX1) * (stBoxInfo.nY2 - stBoxInfo.nY1);
            if (nCurrentArea > nMaxArea)
            {
                nMaxArea = nCurrentArea;
                nMaxIndex = i;
            }
        }
            
        for (int i = 0; i < nSize; i++)
        {
            /* 获取数组的节点 */
            pItemObject = Json::Array::get(pArrayObject, i);
            if (nullptr == pItemObject)
            {
                dlog(LOG_ERROR, "获取数组节点失败 [%d]", i);
                bRet = false;
                goto EXIT;
            }

            if (nMaxIndex == i)
            {
                /* 获取数组节点 */
                nTemp              = -1;
                pArrayArrayObject  = Json::get(pItemObject, "Keypoints");
                pArrayArrayObject2 = Json::get(pItemObject, "KpScore");
                if (NULL != pArrayArrayObject2 &&
                    NULL != pArrayArrayObject)
                {
                    /* 获取关键点信息 */
                    bRet = get_keypoints(pArrayArrayObject, pArrayArrayObject2, vecTemKeyPosInfo);
                    if (!bRet)
                    {
                        dlog(LOG_ERROR, "获取关键点信息-失败\n%s", strInJson.c_str());
                        bRet = false;
                        goto EXIT;
                    }
                    nTemp = getBehavioralType(stSupClsInsParam, vecTemKeyPosInfo);
                    /* 更新Class数据 */
                    Json::update(pItemObject, "Class", nTemp);
                    if (isDelKeyJson)
                    {
                        /* 删除不用的数据 */
                        Json::remove(pItemObject, "Keypoints");
                        Json::remove(pItemObject, "KpScore");
                    }
                }
            }
        }
        
    }
    else
    {
        /* 获取数组大小 */
        nSize = Json::Array::size(pArrayObject);
        if (nSize <= 0)
        {
            goto EXIT;
        }
        for (int i = 0; i < nSize; i++)
        {
            BoxInfo_S stBoxInfo;
            /* 获取数组的节点 */
            pItemObject = Json::Array::get(pArrayObject, i);
            if (nullptr == pItemObject)
            {
                dlog(LOG_ERROR, "获取数组节点失败 [%d]", i);
                bRet = false;
                goto EXIT;
            }

            /* 获取数组节点 */
            nTemp              = -1;
            pArrayArrayObject  = Json::get(pItemObject, "Keypoints");
            pArrayArrayObject2 = Json::get(pItemObject, "KpScore");
            if (NULL != pArrayArrayObject2 &&
                NULL != pArrayArrayObject)
            {
                /* 获取关键点信息 */
                bRet = get_keypoints(pArrayArrayObject, pArrayArrayObject2, vecTemKeyPosInfo);
                if (!bRet)
                {
                    dlog(LOG_ERROR, "获取关键点信息-失败\n%s", strInJson.c_str());
                    bRet = false;
                    goto EXIT;
                }
                nTemp = getBehavioralType(stSupClsInsParam, vecTemKeyPosInfo);
                /* 更新Class数据 */
                Json::update(pItemObject, "Class", nTemp);
                if (isDelKeyJson)
                {
                    /* 删除不用的数据 */
                    Json::remove(pItemObject, "Keypoints");
                    Json::remove(pItemObject, "KpScore");
                }
            }
        }
    }

EXIT:

    if (pJsonHandle)
    {
        if (bRet)
        {
            /* 转换成字符串 */
            strOutJson = Json::to_string(pJsonHandle);
        }

        Json::deinit(pJsonHandle);
        pJsonHandle = nullptr;
    }

    return bRet;
}

/* 获取当前行为类型 */
int CBehaviorProcess::getBehavioralType(
    SupClsInsParam_S     stSupClsInsParam,
    std::vector<KeyPosInfo_S> vecInfo)
{

    if (vecInfo.size() != 26)
    {
        return -1;
    }

    KeyPosInfo_S& stNose      = vecInfo[0];  /* 鼻子 */
    KeyPosInfo_S& stREye      = vecInfo[1];  /* 右眼 */
    KeyPosInfo_S& stLEye      = vecInfo[2];  /* 左眼 */
    KeyPosInfo_S& stREar      = vecInfo[3];  /* 右耳朵 */
    KeyPosInfo_S& stLEar      = vecInfo[4];  /* 左耳朵 */
    KeyPosInfo_S& stRShoulder = vecInfo[5];  /* 右肩 */
    KeyPosInfo_S& stLShoulder = vecInfo[6];  /* 左肩 */
    KeyPosInfo_S& stRElbow    = vecInfo[7];  /* 右手肘 */
    KeyPosInfo_S& stLElbow    = vecInfo[8];  /* 左手肘 */
    KeyPosInfo_S& stRWrist    = vecInfo[9];  /* 右手腕 */
    KeyPosInfo_S& stLWrist    = vecInfo[10]; /* 左手腕 */
    KeyPosInfo_S& stRHip      = vecInfo[11]; /* 右臀 */
    KeyPosInfo_S& stLHip      = vecInfo[12]; /* 左臀 */
    KeyPosInfo_S& stRKnee     = vecInfo[13]; /* 右膝盖 */
    KeyPosInfo_S& stLKnee     = vecInfo[14]; /* 左膝盖 */
    KeyPosInfo_S& stRAnkle    = vecInfo[15]; /* 右脚踝 */
    KeyPosInfo_S& stLAnkle    = vecInfo[16]; /* 左脚踝 */
    KeyPosInfo_S& stHead      = vecInfo[17]; /* 头顶 */
    KeyPosInfo_S& stNeck      = vecInfo[18]; /* 颈部 */
    KeyPosInfo_S& stHip       = vecInfo[19]; /* 臀部 */

/* 数据优化 */
#if 1
    double dScore = 0.8;
    /* 手臂特征点只要有二个超过阈值，全部设置为阈值以上 */
    /* 计算大于阈值的数的数量 */
    int    nCount = (stLShoulder.dScore > dScore) +
        (stLElbow.dScore > dScore) +
        (stLWrist.dScore > dScore);
    if (nCount >= 2)
    {
        stLShoulder.dScore = (dScore + 0.1);
        stLElbow.dScore    = (dScore + 0.1);
        stLWrist.dScore    = (dScore + 0.1);
    }

    nCount = (stRShoulder.dScore > dScore) +
        (stRElbow.dScore > dScore) +
        (stRWrist.dScore > dScore);
    if (nCount >= 2)
    {
        stRShoulder.dScore = (dScore + 0.1);
        stRElbow.dScore    = (dScore + 0.1);
        stRWrist.dScore    = (dScore + 0.1);
    }

    /* 耳朵特征点超过阈值，眼睛特征点设置为阈值以上 */
    if (stREar.dScore > dScore && stLEar.dScore > dScore)
    {
        stREye.dScore = (dScore + 0.1);
        stLEye.dScore = (dScore + 0.1);
    }

#endif

    /*
    种类，{
    7:'玩手机',
    8:'接打电话',
    9:'板书'，
    }
    */

    
    /* 板书判断 */
    if (isTeaBoard(vecInfo, stSupClsInsParam.stTeaBoard))
    {
        return 9;
    }

    /* 接打电话判断 */
    if (isCallPhone(vecInfo, stSupClsInsParam.stCallPhone))
    {
        return 8;
    }

    /* 玩手机判断 */
    if (isPlayPhone(vecInfo, stSupClsInsParam.stPlayPhone))
    {
        return 7;
    }

    return -1;
}

/* 获取框信息 */
bool CBehaviorProcess::get_boxInfo(Json::Object*& pArrayObject, BoxInfo_S& stInfo)
{
    if (NULL == pArrayObject)
    {
        return false;
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
        bRet = false;
        goto EXIT;
    }

    /* 获取数组的节点 */
    pItemObject = Json::Array::get(pArrayObject, 0);
    if (nullptr == pItemObject)
    {
        dlog(LOG_ERROR, "获取数组节点失败 [0]");
        bRet = false;
        goto EXIT;
    }
    /* 坐标X1 */
    bRet = Json::get(pItemObject, stInfo.nX1);
    if (!bRet)
    {
        dlog(LOG_ERROR, "获取[0]数据节点-失败");
        bRet = false;
        goto EXIT;
    }

    /* 获取数组的节点 */
    pItemObject = Json::Array::get(pArrayObject, 1);
    if (nullptr == pItemObject)
    {
        dlog(LOG_ERROR, "获取数组节点失败 [1]");
        bRet = false;
        goto EXIT;
    }
    /* 坐标Y1 */
    bRet = Json::get(pItemObject, stInfo.nY1);
    if (!bRet)
    {
        dlog(LOG_ERROR, "获取[1]数据节点-失败");
        bRet = false;
        goto EXIT;
    }

    /* 获取数组的节点 */
    pItemObject = Json::Array::get(pArrayObject, 2);
    if (nullptr == pItemObject)
    {
        dlog(LOG_ERROR, "获取数组节点失败 [2]");
        bRet = false;
        goto EXIT;
    }
    /* 坐标X2 */
    bRet = Json::get(pItemObject, stInfo.nX2);
    if (!bRet)
    {
        dlog(LOG_ERROR, "获取[2]数据节点-失败");
        bRet = false;
        goto EXIT;
    }

    /* 获取数组的节点 */
    pItemObject = Json::Array::get(pArrayObject, 3);
    if (nullptr == pItemObject)
    {
        dlog(LOG_ERROR, "获取数组节点失败 [3]");
        bRet = false;
        goto EXIT;
    }
    /* 坐标Y2 */
    bRet = Json::get(pItemObject, stInfo.nY2);
    if (!bRet)
    {
        dlog(LOG_ERROR, "获取[3]数据节点-失败");
        bRet = false;
        goto EXIT;
    }

    bRet = true;
EXIT:

    return bRet;
}

/* 获取关键点 */
bool CBehaviorProcess::get_keypoints(Json::Object*&             pKeyObject,
                                     Json::Object*&             pScoreObject,
                                     std::vector<KeyPosInfo_S>& vecInfo)
{
    if (NULL == pKeyObject ||
        NULL == pScoreObject)
    {
        return false;
    }

    BlError_E enRetCode = OK;

    bool bRet   = false;
    int  nSize  = 0;
    int  nSize1 = 0;
    int  nSize2 = 0;

    Json::Object* pItemObject  = nullptr;
    Json::Object* pItemObject1 = nullptr;
    Json::Object* pItemObject2 = nullptr;

    KeyPosInfo_S stInfo;

    /* 获取数组大小 */
    nSize1 = Json::Array::size(pKeyObject);
    nSize2 = Json::Array::size(pScoreObject);
    if (nSize1 != nSize2)
    {
        dlog(LOG_ERROR, "数组大小异常[%d]!=[%d]", nSize1, nSize2);
        bRet = false;
        goto EXIT;
    }

    vecInfo.clear();
    for (int i = 0; i < nSize1; i++)
    {
        /* 获取数组的节点 */
        pItemObject1 = Json::Array::get(pKeyObject, i);
        if (nullptr == pItemObject1)
        {
            dlog(LOG_ERROR, "获取数组KeyObject节点失败 [%d]", i);
            bRet = false;
            goto EXIT;
        }

        pItemObject2 = Json::Array::get(pScoreObject, i);
        if (nullptr == pItemObject2)
        {
            dlog(LOG_ERROR, "获取数组ScoreObject节点失败 [%d]", i);
            bRet = false;
            goto EXIT;
        }

        nSize = Json::Array::size(pItemObject1);
        if (nSize != 2)
        {
            dlog(LOG_ERROR, "KeyObject数组节点大小异常 [%d!=2]", nSize);
            bRet = false;
            goto EXIT;
        }

        nSize = Json::Array::size(pItemObject2);
        if (nSize != 1)
        {
            dlog(LOG_ERROR, "ScoreObject数组节点大小异常 [%d!=1]", nSize);
            bRet = false;
            goto EXIT;
        }

        stInfo.clear();

        /* 获取数组的节点 */
        pItemObject = Json::Array::get(pItemObject1, 0);
        if (nullptr == pItemObject)
        {
            dlog(LOG_ERROR, "获取KeyObject数组节点失败 [0]");
            bRet = false;
            goto EXIT;
        }
        bRet = Json::get(pItemObject, stInfo.dX);
        if (!bRet)
        {
            dlog(LOG_ERROR, "获取KeyObject[0]数据节点-失败");
            bRet = false;
            goto EXIT;
        }

        /* 获取数组的节点 */
        pItemObject = Json::Array::get(pItemObject1, 1);
        if (nullptr == pItemObject)
        {
            dlog(LOG_ERROR, "获取KeyObject数组节点失败 [1]");
            bRet = false;
            goto EXIT;
        }
        bRet = Json::get(pItemObject, stInfo.dY);
        if (!bRet)
        {
            dlog(LOG_ERROR, "获取KeyObject[1]数据节点-失败");
            bRet = false;
            goto EXIT;
        }

        /* 获取数组的节点 */
        pItemObject = Json::Array::get(pItemObject2, 0);
        if (nullptr == pItemObject)
        {
            dlog(LOG_ERROR, "获取ScoreObject数组节点失败 [0]");
            bRet = false;
            goto EXIT;
        }
        bRet = Json::get(pItemObject, stInfo.dScore);
        if (!bRet)
        {
            dlog(LOG_ERROR, "获取ScoreObject[0]数据节点-失败");
            bRet = false;
            goto EXIT;
        }

        vecInfo.push_back(stInfo);
    }

    bRet = true;
EXIT:

    return bRet;
}

/* 低头检测 */
bool CBehaviorProcess::isBendingHead(const std::vector<KeyPosInfo_S>& vstPoint, BendingHead_S stBendingHead)
{
    if (vstPoint.size() != 26)
    {
        dlog(LOG_ERROR, "数据大小不对 [%ld]", vstPoint.size());
        return false;
    }

    const KeyPosInfo_S& stNose      = vstPoint[0];  /* 鼻子 */
    const KeyPosInfo_S& stREye      = vstPoint[1];  /* 右眼 */
    const KeyPosInfo_S& stLEye      = vstPoint[2];  /* 左眼 */
    const KeyPosInfo_S& stREar      = vstPoint[3];  /* 右耳朵 */
    const KeyPosInfo_S& stLEar      = vstPoint[4];  /* 左耳朵 */
    const KeyPosInfo_S& stRShoulder = vstPoint[5];  /* 右肩 */
    const KeyPosInfo_S& stLShoulder = vstPoint[6];  /* 左肩 */
    const KeyPosInfo_S& stRElbow    = vstPoint[7];  /* 右手肘 */
    const KeyPosInfo_S& stLElbow    = vstPoint[8];  /* 左手肘 */
    const KeyPosInfo_S& stRWrist    = vstPoint[9];  /* 右手腕 */
    const KeyPosInfo_S& stLWrist    = vstPoint[10]; /* 左手腕 */
    const KeyPosInfo_S& stRHip      = vstPoint[11]; /* 右臀 */
    const KeyPosInfo_S& stLHip      = vstPoint[12]; /* 左臀 */
    const KeyPosInfo_S& stRKnee     = vstPoint[13]; /* 右膝盖 */
    const KeyPosInfo_S& stLKnee     = vstPoint[14]; /* 左膝盖 */
    const KeyPosInfo_S& stRAnkle    = vstPoint[15]; /* 右脚踝 */
    const KeyPosInfo_S& stLAnkle    = vstPoint[16]; /* 左脚踝 */
    const KeyPosInfo_S& stHead      = vstPoint[17]; /* 头顶 */
    const KeyPosInfo_S& stNeck      = vstPoint[18]; /* 颈部 */
    const KeyPosInfo_S& stHip       = vstPoint[19]; /* 臀部 */

    if (stNeck.dScore > stBendingHead.dKeyScore &&
        stNose.dScore > stBendingHead.dKeyScore)
    {
        if (stNeck.dY < stNose.dY)
        {
            return true;
        }
        else
        {
            if (stLEye.dScore > stBendingHead.dKeyScore &&
                stREye.dScore > stBendingHead.dKeyScore)
            {
                /* 获取stNose到眉心的垂点 */
                KeyPosInfo_S stAjina = getIntersection(stREye, stLEye, stNose);

                double dNumerator   = getDistance(stAjina, stNose);
                double dDenominator = getDistance(stAjina, stNeck);
                if (dDenominator != 0)
                {
                    double dFactor = dNumerator / dDenominator;
                    if (dFactor > stBendingHead.dRatio)
                    {
                        /* 低头 */
                        return true;
                    }
                }
            }
        }
    }
    return false;
}

/* 抬头检测 */
bool CBehaviorProcess::isRaisingHead(
    const std::vector<KeyPosInfo_S>& vstPoint,
    RaisingHead_S                    stRaisingHead)
{
    if (vstPoint.size() != 26)
    {
        dlog(LOG_ERROR, "数据大小不对 [%ld]", vstPoint.size());
        return false;
    }

    const KeyPosInfo_S& stNose      = vstPoint[0];  /* 鼻子 */
    const KeyPosInfo_S& stREye      = vstPoint[1];  /* 右眼 */
    const KeyPosInfo_S& stLEye      = vstPoint[2];  /* 左眼 */
    const KeyPosInfo_S& stREar      = vstPoint[3];  /* 右耳朵 */
    const KeyPosInfo_S& stLEar      = vstPoint[4];  /* 左耳朵 */
    const KeyPosInfo_S& stRShoulder = vstPoint[5];  /* 右肩 */
    const KeyPosInfo_S& stLShoulder = vstPoint[6];  /* 左肩 */
    const KeyPosInfo_S& stRElbow    = vstPoint[7];  /* 右手肘 */
    const KeyPosInfo_S& stLElbow    = vstPoint[8];  /* 左手肘 */
    const KeyPosInfo_S& stRWrist    = vstPoint[9];  /* 右手腕 */
    const KeyPosInfo_S& stLWrist    = vstPoint[10]; /* 左手腕 */
    const KeyPosInfo_S& stRHip      = vstPoint[11]; /* 右臀 */
    const KeyPosInfo_S& stLHip      = vstPoint[12]; /* 左臀 */
    const KeyPosInfo_S& stRKnee     = vstPoint[13]; /* 右膝盖 */
    const KeyPosInfo_S& stLKnee     = vstPoint[14]; /* 左膝盖 */
    const KeyPosInfo_S& stRAnkle    = vstPoint[15]; /* 右脚踝 */
    const KeyPosInfo_S& stLAnkle    = vstPoint[16]; /* 左脚踝 */
    const KeyPosInfo_S& stHead      = vstPoint[17]; /* 头顶 */
    const KeyPosInfo_S& stNeck      = vstPoint[18]; /* 颈部 */
    const KeyPosInfo_S& stHip       = vstPoint[19]; /* 臀部 */

    if (stLEye.dScore > stRaisingHead.dKeyScore &&
        stLEar.dScore > stRaisingHead.dKeyScore &&
        stREye.dScore > stRaisingHead.dKeyScore &&
        stREar.dScore > stRaisingHead.dKeyScore)
    {

        if (std::max(stLEye.dY, stREye.dY) < std::min(stLEar.dY, stREar.dY))
        {
            return true;
        }

        double dAngle = getAngleBetweenPoints(stNose, stLEar, stREar);
        if (dAngle > stRaisingHead.dRaiseAngle)
        {
            return true;
        }
    }

    return false;
}

/* 转头检测 */
bool CBehaviorProcess::isTurningHead(const std::vector<KeyPosInfo_S>& vstPoint, TurningHead_S stTurningHead)
{
    if (vstPoint.size() != 26)
    {
        dlog(LOG_ERROR, "数据大小不对 [%ld]", vstPoint.size());
        return false;
    }

    const KeyPosInfo_S& stNose      = vstPoint[0];  /* 鼻子 */
    const KeyPosInfo_S& stREye      = vstPoint[1];  /* 右眼 */
    const KeyPosInfo_S& stLEye      = vstPoint[2];  /* 左眼 */
    const KeyPosInfo_S& stREar      = vstPoint[3];  /* 右耳朵 */
    const KeyPosInfo_S& stLEar      = vstPoint[4];  /* 左耳朵 */
    const KeyPosInfo_S& stRShoulder = vstPoint[5];  /* 右肩 */
    const KeyPosInfo_S& stLShoulder = vstPoint[6];  /* 左肩 */
    const KeyPosInfo_S& stRElbow    = vstPoint[7];  /* 右手肘 */
    const KeyPosInfo_S& stLElbow    = vstPoint[8];  /* 左手肘 */
    const KeyPosInfo_S& stRWrist    = vstPoint[9];  /* 右手腕 */
    const KeyPosInfo_S& stLWrist    = vstPoint[10]; /* 左手腕 */
    const KeyPosInfo_S& stRHip      = vstPoint[11]; /* 右臀 */
    const KeyPosInfo_S& stLHip      = vstPoint[12]; /* 左臀 */
    const KeyPosInfo_S& stRKnee     = vstPoint[13]; /* 右膝盖 */
    const KeyPosInfo_S& stLKnee     = vstPoint[14]; /* 左膝盖 */
    const KeyPosInfo_S& stRAnkle    = vstPoint[15]; /* 右脚踝 */
    const KeyPosInfo_S& stLAnkle    = vstPoint[16]; /* 左脚踝 */
    const KeyPosInfo_S& stHead      = vstPoint[17]; /* 头顶 */
    const KeyPosInfo_S& stNeck      = vstPoint[18]; /* 颈部 */
    const KeyPosInfo_S& stHip       = vstPoint[19]; /* 臀部 */

    if (stLEye.dScore > stTurningHead.dKeyScore &&
        stREye.dScore > stTurningHead.dKeyScore &&
        stHead.dScore > stTurningHead.dKeyScore &&
        stNeck.dScore > stTurningHead.dKeyScore)
    {
        /* 转头 以头颈线为基准，左眼在右边，右眼在左边边都为转头 */

        int nLEye = getPointRelativeOfLine(stHead, stNeck, stLEye);
        if (nLEye <= 0)
        {
            /* 转头 */
            return true;
        }
        else
        {
            int nREye = getPointRelativeOfLine(stHead, stNeck, stREye);
            if (nREye >= 0)
            {
                /* 转头 */
                return true;
            }
        }
    }

    return false;
}

/* 转身检测 */
bool CBehaviorProcess::isTurningBody(const std::vector<KeyPosInfo_S>& vstPoint, TurningBody_S stTurningBody)
{
    if (vstPoint.size() != 26)
    {
        dlog(LOG_ERROR, "数据大小不对 [%ld]", vstPoint.size());
        return false;
    }

    const KeyPosInfo_S& stNose      = vstPoint[0];  /* 鼻子 */
    const KeyPosInfo_S& stREye      = vstPoint[1];  /* 右眼 */
    const KeyPosInfo_S& stLEye      = vstPoint[2];  /* 左眼 */
    const KeyPosInfo_S& stREar      = vstPoint[3];  /* 右耳朵 */
    const KeyPosInfo_S& stLEar      = vstPoint[4];  /* 左耳朵 */
    const KeyPosInfo_S& stRShoulder = vstPoint[5];  /* 右肩 */
    const KeyPosInfo_S& stLShoulder = vstPoint[6];  /* 左肩 */
    const KeyPosInfo_S& stRElbow    = vstPoint[7];  /* 右手肘 */
    const KeyPosInfo_S& stLElbow    = vstPoint[8];  /* 左手肘 */
    const KeyPosInfo_S& stRWrist    = vstPoint[9];  /* 右手腕 */
    const KeyPosInfo_S& stLWrist    = vstPoint[10]; /* 左手腕 */
    const KeyPosInfo_S& stRHip      = vstPoint[11]; /* 右臀 */
    const KeyPosInfo_S& stLHip      = vstPoint[12]; /* 左臀 */
    const KeyPosInfo_S& stRKnee     = vstPoint[13]; /* 右膝盖 */
    const KeyPosInfo_S& stLKnee     = vstPoint[14]; /* 左膝盖 */
    const KeyPosInfo_S& stRAnkle    = vstPoint[15]; /* 右脚踝 */
    const KeyPosInfo_S& stLAnkle    = vstPoint[16]; /* 左脚踝 */
    const KeyPosInfo_S& stHead      = vstPoint[17]; /* 头顶 */
    const KeyPosInfo_S& stNeck      = vstPoint[18]; /* 颈部 */
    const KeyPosInfo_S& stHip       = vstPoint[19]; /* 臀部 */

    /* 如果左肩大于右肩，则为转身 */
    if (stLShoulder.dScore > stTurningBody.dKeyScore &&
        stRShoulder.dScore > stTurningBody.dKeyScore &&
        stLShoulder.dX > stRShoulder.dX)
    {
        /* 转身 */
        return true;
    }

    return false;
}

/* 站立检测 */
bool CBehaviorProcess::isStanding(const std::vector<KeyPosInfo_S>& vstPoint, Standing_S stStanding)
{
    if (vstPoint.size() != 26)
    {
        dlog(LOG_ERROR, "数据大小不对 [%ld]", vstPoint.size());
        return false;
    }

    const KeyPosInfo_S& stNose      = vstPoint[0];  /* 鼻子 */
    const KeyPosInfo_S& stREye      = vstPoint[1];  /* 右眼 */
    const KeyPosInfo_S& stLEye      = vstPoint[2];  /* 左眼 */
    const KeyPosInfo_S& stREar      = vstPoint[3];  /* 右耳朵 */
    const KeyPosInfo_S& stLEar      = vstPoint[4];  /* 左耳朵 */
    const KeyPosInfo_S& stRShoulder = vstPoint[5];  /* 右肩 */
    const KeyPosInfo_S& stLShoulder = vstPoint[6];  /* 左肩 */
    const KeyPosInfo_S& stRElbow    = vstPoint[7];  /* 右手肘 */
    const KeyPosInfo_S& stLElbow    = vstPoint[8];  /* 左手肘 */
    const KeyPosInfo_S& stRWrist    = vstPoint[9];  /* 右手腕 */
    const KeyPosInfo_S& stLWrist    = vstPoint[10]; /* 左手腕 */
    const KeyPosInfo_S& stRHip      = vstPoint[11]; /* 右臀 */
    const KeyPosInfo_S& stLHip      = vstPoint[12]; /* 左臀 */
    const KeyPosInfo_S& stRKnee     = vstPoint[13]; /* 右膝盖 */
    const KeyPosInfo_S& stLKnee     = vstPoint[14]; /* 左膝盖 */
    const KeyPosInfo_S& stRAnkle    = vstPoint[15]; /* 右脚踝 */
    const KeyPosInfo_S& stLAnkle    = vstPoint[16]; /* 左脚踝 */
    const KeyPosInfo_S& stHead      = vstPoint[17]; /* 头顶 */
    const KeyPosInfo_S& stNeck      = vstPoint[18]; /* 颈部 */
    const KeyPosInfo_S& stHip       = vstPoint[19]; /* 臀部 */

    /* 如果左肩大于右肩，则为转身 */
    if (stLShoulder.dScore > stStanding.dKeyScore &&
        stRShoulder.dScore > stStanding.dKeyScore &&
        stLShoulder.dX > stRShoulder.dX)
    {
        if (stLHip.dScore > stStanding.dKeyScore &&
            stRHip.dScore > stStanding.dKeyScore)
        {
            /* 转身的站立 */
            return true;
        }
    }

    /* 站立 */
#if 1
    bool bValue = false;
    /* 站立 小腿大腿竖直20度以内，且能检测到身体 */
    if (stLHip.dScore > stStanding.dKeyScore && stLKnee.dScore > stStanding.dKeyScore)
    {
        if (stLHip.dY < stLKnee.dY)
        {
            /* 膝盖和臀部的点与Y轴夹角，小于一定的角度 */
            double dAxis = getAngleWithYAxis(stLKnee, stLHip);
            if (dAxis < stStanding.dYRaiseAngle)
            {
                bValue = true;
            }
        }
    }
    if (stRHip.dScore > stStanding.dKeyScore && stRKnee.dScore > stStanding.dKeyScore)
    {
        if (stRHip.dY < stRKnee.dY)
        {
            double dAxis = getAngleWithYAxis(stRKnee, stRHip);
            if (dAxis < stStanding.dYRaiseAngle)
            {
                bValue = true;
            }
        }
    }

    /* 满足上述条件后，还要判断 */
    if (bValue)
    {
        /* 必须有一个手的手腕低于手肘 */
        if (stLElbow.dScore > stStanding.dKeyScore &&
            stLWrist.dScore > stStanding.dKeyScore &&
            stLWrist.dY > stLElbow.dY)
        {
            /* 且手肘到手腕和手肘到肩膀的角度必须大于某个角度 */
            double dAxis = getAngleBetweenPoints(stLElbow, stLShoulder, stLWrist);
            if (dAxis > stStanding.dRaiseAngle)
            {
                return true;
            }
        }
        if (stRElbow.dScore > stStanding.dKeyScore &&
            stRWrist.dScore > stStanding.dKeyScore &&
            stRWrist.dY > stRElbow.dY)
        {
            /* 且手肘到手腕和手肘到肩膀的角度必须大于某个角度 */
            double dAxis = getAngleBetweenPoints(stRElbow, stRShoulder, stRWrist);
            if (dAxis > stStanding.dRaiseAngle)
            {
                return true;
            }
        }
    }

#endif
    return false;
}

/* 举手检测 */
bool CBehaviorProcess::isRaisingHand(const std::vector<KeyPosInfo_S>& vstPoint, RaisingHand_S stRaisingHand)
{
    if (vstPoint.size() != 26)
    {
        dlog(LOG_ERROR, "数据大小不对 [%ld]", vstPoint.size());
        return false;
    }

    const KeyPosInfo_S& stNose      = vstPoint[0];  /* 鼻子 */
    const KeyPosInfo_S& stREye      = vstPoint[1];  /* 右眼 */
    const KeyPosInfo_S& stLEye      = vstPoint[2];  /* 左眼 */
    const KeyPosInfo_S& stREar      = vstPoint[3];  /* 右耳朵 */
    const KeyPosInfo_S& stLEar      = vstPoint[4];  /* 左耳朵 */
    const KeyPosInfo_S& stRShoulder = vstPoint[5];  /* 右肩 */
    const KeyPosInfo_S& stLShoulder = vstPoint[6];  /* 左肩 */
    const KeyPosInfo_S& stRElbow    = vstPoint[7];  /* 右手肘 */
    const KeyPosInfo_S& stLElbow    = vstPoint[8];  /* 左手肘 */
    const KeyPosInfo_S& stRWrist    = vstPoint[9];  /* 右手腕 */
    const KeyPosInfo_S& stLWrist    = vstPoint[10]; /* 左手腕 */
    const KeyPosInfo_S& stRHip      = vstPoint[11]; /* 右臀 */
    const KeyPosInfo_S& stLHip      = vstPoint[12]; /* 左臀 */
    const KeyPosInfo_S& stRKnee     = vstPoint[13]; /* 右膝盖 */
    const KeyPosInfo_S& stLKnee     = vstPoint[14]; /* 左膝盖 */
    const KeyPosInfo_S& stRAnkle    = vstPoint[15]; /* 右脚踝 */
    const KeyPosInfo_S& stLAnkle    = vstPoint[16]; /* 左脚踝 */
    const KeyPosInfo_S& stHead      = vstPoint[17]; /* 头顶 */
    const KeyPosInfo_S& stNeck      = vstPoint[18]; /* 颈部 */
    const KeyPosInfo_S& stHip       = vstPoint[19]; /* 臀部 */


    /* 举手，1.手腕高于手肘，并且手肘和手肘连成的直线在纵向小于一定度 */
    if (stLElbow.dScore > stRaisingHand.dKeyScore &&
        stLWrist.dScore > stRaisingHand.dKeyScore)
    {
        if (stLElbow.dY > stLWrist.dY)
        {
            double dAxis = getAngleWithYAxis(stLElbow, stLWrist);
            if (dAxis < stRaisingHand.dYRaiseAngle)
            {
                /* 举左手 */
                return true;
            }
            // dlog(LOG_ERROR, "举左手 [%f] %f(%f,%f)-(%f,%f)",
            //      dAxis,
            //      stRaisingHand.dRaiseAngle,
            //      stLElbow.dX,
            //      stLElbow.dY,
            //      stLWrist.dX,
            //      stLWrist.dY);
        }
    }

    if (stRElbow.dScore > stRaisingHand.dKeyScore &&
        stRWrist.dScore > stRaisingHand.dKeyScore)
    {
        if (stRElbow.dY > stRWrist.dY)
        {
            double dAxis = getAngleWithYAxis(stRElbow, stRWrist);
            if (dAxis < stRaisingHand.dYRaiseAngle)
            {
                /* 举右手 */
                return true;
            }
            // dlog(LOG_ERROR, "举右手 [%f] %f(%f,%f)-(%f,%f)",
            //      dAxis,
            //      stRaisingHand.dRaiseAngle,
            //      stRElbow.dX,
            //      stRElbow.dY,
            //      stRWrist.dX,
            //      stRWrist.dY);
        }
    }

    return false;
}

/* 趴桌检测 */
bool CBehaviorProcess::isLyingOnDesk(const std::vector<KeyPosInfo_S>& vstPoint, LyingOnDesk_S stLyingOnDesk)
{
    if (vstPoint.size() != 26)
    {
        dlog(LOG_ERROR, "数据大小不对 [%ld]", vstPoint.size());
        return false;
    }

    const KeyPosInfo_S& stNose      = vstPoint[0];  /* 鼻子 */
    const KeyPosInfo_S& stREye      = vstPoint[1];  /* 右眼 */
    const KeyPosInfo_S& stLEye      = vstPoint[2];  /* 左眼 */
    const KeyPosInfo_S& stREar      = vstPoint[3];  /* 右耳朵 */
    const KeyPosInfo_S& stLEar      = vstPoint[4];  /* 左耳朵 */
    const KeyPosInfo_S& stRShoulder = vstPoint[5];  /* 右肩 */
    const KeyPosInfo_S& stLShoulder = vstPoint[6];  /* 左肩 */
    const KeyPosInfo_S& stRElbow    = vstPoint[7];  /* 右手肘 */
    const KeyPosInfo_S& stLElbow    = vstPoint[8];  /* 左手肘 */
    const KeyPosInfo_S& stRWrist    = vstPoint[9];  /* 右手腕 */
    const KeyPosInfo_S& stLWrist    = vstPoint[10]; /* 左手腕 */
    const KeyPosInfo_S& stRHip      = vstPoint[11]; /* 右臀 */
    const KeyPosInfo_S& stLHip      = vstPoint[12]; /* 左臀 */
    const KeyPosInfo_S& stRKnee     = vstPoint[13]; /* 右膝盖 */
    const KeyPosInfo_S& stLKnee     = vstPoint[14]; /* 左膝盖 */
    const KeyPosInfo_S& stRAnkle    = vstPoint[15]; /* 右脚踝 */
    const KeyPosInfo_S& stLAnkle    = vstPoint[16]; /* 左脚踝 */
    const KeyPosInfo_S& stHead      = vstPoint[17]; /* 头顶 */
    const KeyPosInfo_S& stNeck      = vstPoint[18]; /* 颈部 */
    const KeyPosInfo_S& stHip       = vstPoint[19]; /* 臀部 */

    /* 趴桌子 : 头顶，有一个低于肩膀。则说明为趴桌子 */
    if (stLShoulder.dScore > stLyingOnDesk.dKeyScore &&
        stRShoulder.dScore > stLyingOnDesk.dKeyScore &&
        stHead.dScore > stLyingOnDesk.dKeyScore)
    {
        double dMinShoulder = std::min(stLShoulder.dY, stRShoulder.dY);
        if ((stHead.dY) >= dMinShoulder)
        {
            return true;
        }
    }


    return false;
}

/* 玩手机检测 */
bool CBehaviorProcess::isPlayPhone(const std::vector<KeyPosInfo_S>& vstPoint, AiScenario_NS::PlayPhone_S stPlayPhone)
{
    if (vstPoint.size() != 26)
    {
        dlog(LOG_ERROR, "数据大小不对 [%ld]", vstPoint.size());
        return false;
    }

    const KeyPosInfo_S& stNose      = vstPoint[0];  /* 鼻子 */
    const KeyPosInfo_S& stREye      = vstPoint[1];  /* 右眼 */
    const KeyPosInfo_S& stLEye      = vstPoint[2];  /* 左眼 */
    const KeyPosInfo_S& stREar      = vstPoint[3];  /* 右耳朵 */
    const KeyPosInfo_S& stLEar      = vstPoint[4];  /* 左耳朵 */
    const KeyPosInfo_S& stRShoulder = vstPoint[5];  /* 右肩 */
    const KeyPosInfo_S& stLShoulder = vstPoint[6];  /* 左肩 */
    const KeyPosInfo_S& stRElbow    = vstPoint[7];  /* 右手肘 */
    const KeyPosInfo_S& stLElbow    = vstPoint[8];  /* 左手肘 */
    const KeyPosInfo_S& stRWrist    = vstPoint[9];  /* 右手腕 */
    const KeyPosInfo_S& stLWrist    = vstPoint[10]; /* 左手腕 */
    const KeyPosInfo_S& stRHip      = vstPoint[11]; /* 右臀 */
    const KeyPosInfo_S& stLHip      = vstPoint[12]; /* 左臀 */
    const KeyPosInfo_S& stRKnee     = vstPoint[13]; /* 右膝盖 */
    const KeyPosInfo_S& stLKnee     = vstPoint[14]; /* 左膝盖 */
    const KeyPosInfo_S& stRAnkle    = vstPoint[15]; /* 右脚踝 */
    const KeyPosInfo_S& stLAnkle    = vstPoint[16]; /* 左脚踝 */
    const KeyPosInfo_S& stHead      = vstPoint[17]; /* 头顶 */
    const KeyPosInfo_S& stNeck      = vstPoint[18]; /* 颈部 */
    const KeyPosInfo_S& stHip       = vstPoint[19]; /* 臀部 */

    BendingHead_S stBendingHead;
    /* 玩手机： 低头+两个手靠近（两个肩膀的X轴距离(固定)与两个手腕的X轴距离比例） */
    // dlog(LOG_USER,"stPlayPhone.dKeyScore=%f",stPlayPhone.dKeyScore);
    // dlog(LOG_USER,"stLShoulder.dScore=%f",stLShoulder.dScore);
    // dlog(LOG_USER,"stRShoulder.dScore=%f",stRShoulder.dScore);
    // dlog(LOG_USER,"stLWrist.dScore=%f",stLWrist.dScore);
    // dlog(LOG_USER,"stRWrist.dScore=%f",stRWrist.dScore);
    // dlog(LOG_USER,"isBendingHead(vstPoint,stBendingHead)=%d",isBendingHead(vstPoint,stBendingHead));
    /* 获取两个手腕的斜率 */
    double dSlope = getSlope(stLWrist,stRWrist);
    if (stLShoulder.dScore > stPlayPhone.dKeyScore && 
        stRShoulder.dScore > stPlayPhone.dKeyScore &&
        stLWrist.dScore > stPlayPhone.dKeyScore &&
        stRWrist.dScore > stPlayPhone.dKeyScore &&
        isBendingHead(vstPoint,stBendingHead) &&
        dSlope > 0 &&
        dSlope < 0.1 &&
        stRShoulder.dX > stLShoulder.dX &&
        stRWrist.dX > stLWrist.dX
        )
    {
        Standing_S stStanding;
        /* 过滤掉异常动作 - 不符合玩手机的动作 */
        if(stLWrist.dX > stLShoulder.dX &&
         stLWrist.dX < stRShoulder.dX &&
         stRWrist.dX > stLShoulder.dX &&
         stRWrist.dX < stRShoulder.dX &&
         false == isStanding(vstPoint,stStanding))
         {
            /* 除数不能为0 */
            if(stRWrist.dX - stLWrist.dX != 0)
            {
                /* 系数比：手腕宽度 / 肩宽度 = 系数比 */
                double dRatio = (stRWrist.dX - stLWrist.dX) / (stRShoulder.dX - stLShoulder.dX);
                // dlog(LOG_USER,"系数比：stPlayPhone.dRatio=%f",stPlayPhone.dRatio);
                // dlog(LOG_USER,"系数比：dRatio=%f",dRatio);
                if (dRatio <= stPlayPhone.dRatio && dRatio > 0)
                {
                    return true;
                }
            }
         }
    }

    return false;
}

/* 接打电话检测 */
bool CBehaviorProcess::isCallPhone(const std::vector<KeyPosInfo_S>& vstPoint, AiScenario_NS::CallPhone_S stCallPhone)
{
    if (vstPoint.size() != 26)
    {
        dlog(LOG_ERROR, "数据大小不对 [%ld]", vstPoint.size());
        return false;
    }

    const KeyPosInfo_S& stNose      = vstPoint[0];  /* 鼻子 */
    const KeyPosInfo_S& stREye      = vstPoint[1];  /* 右眼 */
    const KeyPosInfo_S& stLEye      = vstPoint[2];  /* 左眼 */
    const KeyPosInfo_S& stREar      = vstPoint[3];  /* 右耳朵 */
    const KeyPosInfo_S& stLEar      = vstPoint[4];  /* 左耳朵 */
    const KeyPosInfo_S& stRShoulder = vstPoint[5];  /* 右肩 */
    const KeyPosInfo_S& stLShoulder = vstPoint[6];  /* 左肩 */
    const KeyPosInfo_S& stRElbow    = vstPoint[7];  /* 右手肘 */
    const KeyPosInfo_S& stLElbow    = vstPoint[8];  /* 左手肘 */
    const KeyPosInfo_S& stRWrist    = vstPoint[9];  /* 右手腕 */
    const KeyPosInfo_S& stLWrist    = vstPoint[10]; /* 左手腕 */
    const KeyPosInfo_S& stRHip      = vstPoint[11]; /* 右臀 */
    const KeyPosInfo_S& stLHip      = vstPoint[12]; /* 左臀 */
    const KeyPosInfo_S& stRKnee     = vstPoint[13]; /* 右膝盖 */
    const KeyPosInfo_S& stLKnee     = vstPoint[14]; /* 左膝盖 */
    const KeyPosInfo_S& stRAnkle    = vstPoint[15]; /* 右脚踝 */
    const KeyPosInfo_S& stLAnkle    = vstPoint[16]; /* 左脚踝 */
    const KeyPosInfo_S& stHead      = vstPoint[17]; /* 头顶 */
    const KeyPosInfo_S& stNeck      = vstPoint[18]; /* 颈部 */
    const KeyPosInfo_S& stHip       = vstPoint[19]; /* 臀部 */

    /* 不能背对 */
    if( stRShoulder.dX > stLShoulder.dX &&
    stRWrist.dX > stLWrist.dX)
    {
        /* 左边接打电话： 耳朵为圆心，耳朵到肩膀距离为半径，手腕的点在半径范围内为接打电话 */
        if (stLShoulder.dScore > stCallPhone.dKeyScore &&
            stLEar.dScore > stCallPhone.dKeyScore &&
            stLWrist.dScore > stCallPhone.dKeyScore)
        {
            /* 计算左耳到左肩的距离，作为半径 */
            double dRadius = getDistance(stLShoulder,stLEar);
            /* 计算手腕到左耳的距离 */
            double dEarToWrist = getDistance(stLWrist, stLEar);
            // dlog(LOG_INFO,"左耳坐标[x:%f,y:%f]",stLEar.dX,stLEar.dY);
            // dlog(LOG_INFO,"左肩坐标[x:%f,y:%f]",stLShoulder.dX,stLShoulder.dY);
            // dlog(LOG_INFO,"左手腕坐标[x:%f,y:%f]",stLWrist.dX,stLWrist.dY);
            // dlog(LOG_INFO,"左耳到左肩的距离：dRadius=%f",dRadius);
            // dlog(LOG_INFO,"左手腕到左耳的距离：dEarToWrist=%f",dEarToWrist);
            /* 判断手腕是否在圆内 */
            if (dEarToWrist < dRadius)
            {
                // dlog(LOG_INFO,"符合左边接打电话");
                return true;
            }
            
        }

        /* 右边接打电话： 耳朵为圆心，耳朵到肩膀距离为半径，手腕的点在半径范围内为接打电话 */
        if (stRShoulder.dScore > stCallPhone.dKeyScore &&
            stREar.dScore > stCallPhone.dKeyScore &&
            stRWrist.dScore > stCallPhone.dKeyScore)
        {
            /* 计算右耳到右肩的距离，作为半径 */
            double dRadius = getDistance(stRShoulder,stREar);
            /* 计算手腕到右耳的距离 */
            double dEarToWrist = getDistance(stRWrist, stREar);
            // dlog(LOG_INFO,"右耳坐标[x:%f,y:%f]",stREar.dX,stREar.dY);
            // dlog(LOG_INFO,"右肩坐标[x:%f,y:%f]",stRShoulder.dX,stRShoulder.dY);
            // dlog(LOG_INFO,"右手腕坐标[x:%f,y:%f]",stRWrist.dX,stRWrist.dY);
            // dlog(LOG_INFO,"右耳到右肩的距离：dRadius=%f",dRadius);
            // dlog(LOG_INFO,"右手腕到右耳的距离：dEarToWrist=%f",dEarToWrist);
            /* 判断手腕是否在圆内 */
            if (dEarToWrist < dRadius)
            {
                // dlog(LOG_INFO,"符合右边接打电话");
                return true;
            }
        }
    }
    

    return false;
}

/* 板书检测 */
bool CBehaviorProcess::isTeaBoard(const std::vector<KeyPosInfo_S>& vstPoint, AiScenario_NS::TeaBoard_S stTeaBoard)
{
    if (vstPoint.size() != 26)
    {
        dlog(LOG_ERROR, "数据大小不对 [%ld]", vstPoint.size());
        return false;
    }

    const KeyPosInfo_S& stNose      = vstPoint[0];  /* 鼻子 */
    const KeyPosInfo_S& stREye      = vstPoint[1];  /* 右眼 */
    const KeyPosInfo_S& stLEye      = vstPoint[2];  /* 左眼 */
    const KeyPosInfo_S& stREar      = vstPoint[3];  /* 右耳朵 */
    const KeyPosInfo_S& stLEar      = vstPoint[4];  /* 左耳朵 */
    const KeyPosInfo_S& stRShoulder = vstPoint[5];  /* 右肩 */
    const KeyPosInfo_S& stLShoulder = vstPoint[6];  /* 左肩 */
    const KeyPosInfo_S& stRElbow    = vstPoint[7];  /* 右手肘 */
    const KeyPosInfo_S& stLElbow    = vstPoint[8];  /* 左手肘 */
    const KeyPosInfo_S& stRWrist    = vstPoint[9];  /* 右手腕 */
    const KeyPosInfo_S& stLWrist    = vstPoint[10]; /* 左手腕 */
    const KeyPosInfo_S& stRHip      = vstPoint[11]; /* 右臀 */
    const KeyPosInfo_S& stLHip      = vstPoint[12]; /* 左臀 */
    const KeyPosInfo_S& stRKnee     = vstPoint[13]; /* 右膝盖 */
    const KeyPosInfo_S& stLKnee     = vstPoint[14]; /* 左膝盖 */
    const KeyPosInfo_S& stRAnkle    = vstPoint[15]; /* 右脚踝 */
    const KeyPosInfo_S& stLAnkle    = vstPoint[16]; /* 左脚踝 */
    const KeyPosInfo_S& stHead      = vstPoint[17]; /* 头顶 */
    const KeyPosInfo_S& stNeck      = vstPoint[18]; /* 颈部 */
    const KeyPosInfo_S& stHip       = vstPoint[19]; /* 臀部 */

    if(stRShoulder.dScore > stTeaBoard.dKeyScore && stLShoulder.dScore > stTeaBoard.dKeyScore)
    {
        // dlog(LOG_INFO,"右肩x坐标：cstRShoulder.dX=%f",stRShoulder.dX);
        // dlog(LOG_INFO,"左肩x坐标：cstRShoulder.dX=%f",stLShoulder.dX);
        /* 背对 */
        if( stRShoulder.dX < stLShoulder.dX)
        {
            // dlog(LOG_INFO,"背对。。。");
            return true;
        }
    }

    return false;
}

/* 计算两点之间与y轴的角度 */
double CBehaviorProcess::getAngleWithYAxis(const KeyPosInfo_S& stPoint1, const KeyPosInfo_S& stPoint2)
{
    /* 确保两点的 x 坐标不同，避免除以零错误 */
    if (stPoint1.dX == stPoint2.dX)
    {
        /* 如果两点的 x 坐标相同，则直线垂直于 x 轴，夹角为 90 度 */
        return 0;
    }

    /* 计算直线的斜率 */
    double dSlope = std::abs((stPoint2.dX - stPoint1.dX) / (stPoint2.dY - stPoint1.dY));

    /* 计算斜率的角度（弧度） */
    double dAngleRadians = std::atan(dSlope);

    /* 将弧度转换为角度 */
    double dAngleDegrees = dAngleRadians * 180.0 / M_PI;    // std::to_degrees(dAngleRadians);

    /* 确保角度在0到90度之间 */
    dAngleDegrees = std::min(dAngleDegrees, 90.0d);
    dAngleDegrees = std::max(dAngleDegrees, 0.0d);

    return dAngleDegrees;
}

/* 获取斜率 */
double CBehaviorProcess::getSlope(const KeyPosInfo_S& p1, const KeyPosInfo_S& p2)
{
    if (p1.dX != p2.dX)
    {    // 检查是否为垂直线
        return (p2.dY - p1.dY) / (p2.dX - p1.dX);
    }
    else
    {
        return 0;    // 垂直线斜率设置为0
    }
}

/* 获取垂直交点 */
CBehaviorProcess::KeyPosInfo_S CBehaviorProcess::getIntersection(const KeyPosInfo_S& A, const KeyPosInfo_S& B, const KeyPosInfo_S& C)
{
    double slopeAB            = getSlope(A, B);
    double slopePerpendicular = -1 / slopeAB;    // Perpendicular slope

    // 垂直线的方程 y - y1 = slopePerpendicular * (x - x1)
    // 线段AB的方程 y = slopeAB * x + (y1 - slopeAB * x1)

    // 交点坐标的计算
    KeyPosInfo_S stOutInfo;

    if (slopeAB == 0)
    {                           // 线段AB水平
        stOutInfo.dX = A.dX;
        stOutInfo.dY = C.dY;    // 垂直线通过C点
    }
    else if (slopePerpendicular == 0)
    {                           // 垂直线水平
        stOutInfo.dY = C.dY;
        stOutInfo.dX = A.dX;    // 线段AB通过A点
    }
    else
    {
        stOutInfo.dX = (C.dY - (slopePerpendicular * C.dX) - (A.dY - slopeAB * A.dX)) / (slopeAB - slopePerpendicular);
        stOutInfo.dY = slopePerpendicular * stOutInfo.dX + (C.dY - slopePerpendicular * C.dX);
    }
    stOutInfo.dScore = 1.0;

    return stOutInfo;
}

/* 计算两点间的距离 */
double CBehaviorProcess::getDistance(const KeyPosInfo_S& stPoint1, const KeyPosInfo_S& stPoint2)
{
    /* 欧几里得距离公式 */
    return sqrt((stPoint2.dX - stPoint1.dX) * (stPoint2.dX - stPoint1.dX) + (stPoint2.dY - stPoint1.dY) * (stPoint2.dY - stPoint1.dY));
}

/* 获取中心点 */
CBehaviorProcess::KeyPosInfo_S CBehaviorProcess::getMidpoint(const KeyPosInfo_S& point1, const KeyPosInfo_S& point2)
{
    KeyPosInfo_S stOutInfo;
    stOutInfo.dScore = 1.0;

    /* 计算中心点的x坐标和y坐标 */
    stOutInfo.dX = (point1.dX + point2.dX) / 2.0;
    stOutInfo.dY = (point1.dY + point2.dY) / 2.0;

    return stOutInfo;
}

/* 计算三角形面积的两倍 */
double CBehaviorProcess::getTwiceTriangleArea(const KeyPosInfo_S& a, const KeyPosInfo_S& b, const KeyPosInfo_S& c)
{
    return a.dX * (b.dY - c.dY) + b.dX * (c.dY - a.dY) + c.dX * (a.dY - b.dY);
}

/* 计算由三个点形成的夹角的角度 a是交点 */
double CBehaviorProcess::getAngleBetweenPoints(const KeyPosInfo_S& a, const KeyPosInfo_S& b, const KeyPosInfo_S& c)
{
    // 计算三角形面积的两倍，检查是否共线
    double area = getTwiceTriangleArea(a, b, c);
    if (area == 0)
    {
        return 180.0;
    }

    // 计算向量AB和向量AC
    double ABx = b.dX - a.dX;
    double ABy = b.dY - a.dY;
    double ACx = c.dX - a.dX;
    double ACy = c.dY - a.dY;

    // 计算向量AB和向量AC的点积
    double dotProduct = ABx * ACx + ABy * ACy;

    // 计算向量AB和向量AC的模
    double magnitudeAB = sqrt(ABx * ABx + ABy * ABy);
    double magnitudeAC = sqrt(ACx * ACx + ACy * ACy);

    // 计算夹角的余弦值
    double cosAngle = dotProduct / (magnitudeAB * magnitudeAC);

    // 将余弦值转换为角度
    double angleRadians = acos(cosAngle);
    double angleDegrees = angleRadians * (180.0 / M_PI);

    return angleDegrees;
}

/* 计算向量AB和AP的叉积 */
double CBehaviorProcess::getCrossProduct(const KeyPosInfo_S& A, const KeyPosInfo_S& B, const KeyPosInfo_S& P)
{
    return (B.dX - A.dX) * (P.dY - A.dY) - (B.dY - A.dY) * (P.dX - A.dX);
}

/* 判断点P相对于由A和B定义的直线的位置, 具体返回值为 1 表示在左边，-1 表示在右边，0 表示在直线上 */
int CBehaviorProcess::getPointRelativeOfLine(const KeyPosInfo_S& A,
                                             const KeyPosInfo_S& B,
                                             const KeyPosInfo_S& P)
{
    double crossProduct = getCrossProduct(A, B, P);

    /* 根据直线方向决定左边的判断 */
    if (A.dY < B.dY)
    {
        /* 如果 A.y < B.y，则交叉积大于等于零在左边 */
        if (crossProduct == 0.0)
        {
            return 0;
        }
        else if (crossProduct > 0.0)
        {
            return 1;
        }
        else
        {
            return -1;
        }
    }
    else
    {
        if (crossProduct == 0.0)
        {
            return 0;
        }
        else if (crossProduct > 0.0)
        {
            return -1;
        }
        else
        {
            return 1;
        }
    }
}
