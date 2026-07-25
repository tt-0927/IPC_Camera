#include "AttendanceManager.hpp"

#include <cstring>
#include <iostream>

#include "dlog.h"
#include "JsonInterfase.h"
#include "ToolFunc.hpp"

using namespace Ai0630_NS;

/* 记录人类识别事件 */
void AttendanceManager::add(
    const FaceLibsInfo_S&   stFaceLibsInfo,
    const UserHeaderInfo_S& stUserHeaderInfo)
{
    std::lock_guard<std::mutex> lock(m_mutex);

    dlog(LOG_INFO, "添加考勤 [%d] [%d]", stFaceLibsInfo.nIdentity, stFaceLibsInfo.nMemberId);

    /* 保存截图 */
    char achCmd[1024] = { 0 };
    memset(achCmd, 0, sizeof(achCmd));
    snprintf(achCmd, sizeof(achCmd), "cp %s/st_%lld.jpg %s/user_%d_%lld.jpg",
             GANCIAN_PICTURE_TEMP_PATH,
             stUserHeaderInfo.lTimestamp,
             GANCIAN_PICTURE_TEMP_PATH,
             stFaceLibsInfo.nMemberId,
             stUserHeaderInfo.lTimestamp);

    if (system(achCmd) != 0)
    {
        dlog(LOG_ERROR, "保存人脸考勤图片失败 [%s]", achCmd);
    }

    if (stFaceLibsInfo.nIdentity == 0)
    {
        auto& info = m_studentMap[stFaceLibsInfo.nMemberId];

        if (info.nNumber == 0)
        {
            /* 第一次识别 */
            info.nId     = stFaceLibsInfo.nMemberId;
            info.strName = stFaceLibsInfo.strName;
        }

        /* 更新第一次出现（最小时间） */
        if (stUserHeaderInfo.lTimestamp < info.nFirstTimestamp)
        {
            info.nFirstTimestamp = stUserHeaderInfo.lTimestamp;
            info.nFirstTime      = stUserHeaderInfo.nClassTime;
        }

        /* 更新最后一次出现（最大时间） */
        if (stUserHeaderInfo.lTimestamp > info.nFirstTimestamp)
        {
            info.nLastTimestamp = stUserHeaderInfo.lTimestamp;
            info.nLastTime      = stUserHeaderInfo.nClassTime;
        }

        info.nNumber++;
    }
    else if (stFaceLibsInfo.nIdentity == 1)
    {
        auto& info = m_teacherMap[stFaceLibsInfo.nMemberId];

        if (info.nNumber == 0)
        {
            /* 第一次识别 */
            info.nId     = stFaceLibsInfo.nMemberId;
            info.strName = stFaceLibsInfo.strName;
        }

        /* 更新第一次出现（最小时间） */
        if (stUserHeaderInfo.lTimestamp < info.nFirstTimestamp)
        {
            info.nFirstTimestamp = stUserHeaderInfo.lTimestamp;
            info.nFirstTime      = stUserHeaderInfo.nClassTime;
        }

        /* 更新最后一次出现（最大时间） */
        if (stUserHeaderInfo.lTimestamp > info.nFirstTimestamp)
        {
            info.nLastTimestamp = stUserHeaderInfo.lTimestamp;
            info.nLastTime      = stUserHeaderInfo.nClassTime;
        }

        info.nNumber++;
    }
}

/* 整合 */
void AttendanceManager::finalize(const void* pParam)
{
    /* 保存文件 */
    std::string strPath = std::string((char*)pParam) + "/Attendance.json";
    saveFile(strPath);
}

/* 清空记录（下一堂课） */
void AttendanceManager::reset()
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_studentMap.clear();
    m_teacherMap.clear();
}

/* 保存结果文件 */
void Ai0630_NS::AttendanceManager::saveFile(std::string strFilePath)
{
    /* 转换成Json数据 */
    char* pchJsonData = nullptr;

    auto pRootJson = Json::init();

    dlog(LOG_INFO, "生成考勤报告 学生[%ld] 老师[%ld]", m_studentMap.size(), m_teacherMap.size());

    /* 学生考勤信息 */
    auto pStudentArrayJson = Json::Array::init();
    for (auto it = m_studentMap.begin();
         it != m_studentMap.end();
         ++it)
    {
        auto          pTmpJson      = Json::init();
        Json::Object* pTmpArrayJson = nullptr;

        int nNum = it->second.listAnswerTime.size();
        if (nNum <= 0)
        {
            pTmpArrayJson = Json::Array::init();
        }
        else
        {
            int anNums[nNum] = { 0 };
            int nCount       = 0;
            for (auto item : it->second.listAnswerTime)
            {
                anNums[nCount++] = item;
            }
            pTmpArrayJson = Json::Array::init(anNums, nNum);
        }

        Json::add(pTmpJson, "STID", it->second.nId);
        Json::add(pTmpJson, "Name", it->second.strName);
        Json::add(pTmpJson, "FirstTime", it->second.nFirstTime);
        Json::add(pTmpJson, "LastTime", it->second.nLastTime);
        Json::add(pTmpJson, "Number", it->second.nNumber);

        Json::add(pTmpJson, "AnswerTime", pTmpArrayJson);

        Json::Array::add(pStudentArrayJson, pTmpJson);
    }


    /* TeacherInfo节点 */
    int              nTeMax = 0;
    AttendanceInfo_S stTmp;

    auto pTeacherArrayJson = Json::Array::init();
    for (auto it = m_teacherMap.begin();
         it != m_teacherMap.end();
         ++it)
    {
        if (nTeMax < it->second.nNumber)
        {
            nTeMax = it->second.nNumber;
            stTmp  = it->second;
        }
    }

    if (nTeMax > 0)
    {
        auto pTmpJson = Json::init();

        Json::add(pTmpJson, "TCID", stTmp.nId);
        Json::add(pTmpJson, "Name", stTmp.strName);
        Json::add(pTmpJson, "FirstTime", stTmp.nFirstTime);
        Json::add(pTmpJson, "LastTime", stTmp.nLastTime);
        Json::add(pTmpJson, "Number", stTmp.nNumber);

        Json::Array::add(pTeacherArrayJson, pTmpJson);
    }


    Json::add(pRootJson, "StudentInfo", pStudentArrayJson);
    Json::add(pRootJson, "TeacherInfo", pTeacherArrayJson);


    /* 转换成字符串 */
    pchJsonData = Json::print(pRootJson);

    /* 释放数据 */
    Json::deinit(pRootJson);

    /* 保存成文件 */
    ToolFunc::writeDataToFile(
        strFilePath.c_str(),
        pchJsonData,
        strlen(pchJsonData));

    /* 释放空间 */
    Json::release(pchJsonData);
    pchJsonData = nullptr;
}
