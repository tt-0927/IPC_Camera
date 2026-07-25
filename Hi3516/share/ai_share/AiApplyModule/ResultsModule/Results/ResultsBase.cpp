/*
 * @FilePath     : ResultsBase.cpp
 * @Author       : 严泽辉 yanzeh@kfb.cn
 * @Date         : 2024-03-28 09:23:40
 * @LastEditors: weigl 308241951@qq.com
 * @LastEditTime: 2025-03-07 17:58:56
 * @Description  :
 */
#include "ResultsBase.hpp"

#include <algorithm>
#include <chrono>
#include <climits>
#include <cmath>

#include "AiManageExtern.hpp"
#include "dlog.h"
#include "PublicFunc.hpp"
#include "JsonInterfase.h"

using namespace ResultsModule_NS;

static bool compareTimeSlotInfo(const TimeSlotInfo_S& a, const TimeSlotInfo_S& b)
{
    if (a.nStart == b.nStart)
    {
        return a.nEnd < b.nEnd;
    }
    return a.nStart < b.nStart;
}

/* 函数用于将点限制在指定的三角形内 */
static void clampPointToTriangle(float& x, float& y)
{
    /* 首先确保x在0和1之间 */
    x = fmod(fmod(x, 1.0f) + 1.0f, 1.0f);

    /* 根据 x 的值来限制 y 的值 */
    if (x <= 0.5f)
    {
        /* 当 x 在 [0, 0.5] 范围内时，y 的范围是 [0, 2x] */
        y = std::min(y, 2.0f * x);
    }
    else
    {
        /* 当 x 在 [0.5, 1] 范围内时，y 的范围是 [2(1-x), 0] */
        y = std::min(y, 2.0f * (1.0f - x));
    }

    /* 确保y是非负的 */
    y = std::max(0.0f, y);
}

ResultsModule_NS::CResultsBase::CResultsBase(InParam_S stInfo)
    : m_stParamInfo(stInfo)
{
    /* 课堂总结信息 */
    m_stClassSummaryInfo.clear();
    /* 教育云平台课堂总结信息 */
    m_stPlatformClassSummaryInfo.clear();

    /* 教师信息 */
    m_stTeacherInfo.clear();
    /* 教育云平台教师信息 */
    m_stPlatformTeacherInfo.clear();

    /* 学生信息 */
    m_stStudentInfo.clear();
    /* 教育云平台学生信息 */
    m_stPlatformStudentInfo.clear();

    /* 考勤信息 */
    m_stAttendanceInfo.clear();
    /* 教育云平台考勤信息 */
    m_stPlatformAttendanceInfo.clear();

    m_stAiConfidenceTh.clear();

    m_nCurPeopleNum = 0;
}

ResultsModule_NS::CResultsBase::~CResultsBase()
{
}

/* 结束AI分析 */
BlError_E ResultsModule_NS::CResultsBase::end_aiAnalysis(const void* pParam)
{
    beginDeal_attendanceInfo();
    beginDeal_studentInfo();
    beginDeal_teacherInfo();
    beginDeal_classSummaryInfo();

    std::string strPath = std::string((char*)pParam) + "/Attendance.json";
    endDeal_attendanceInfo(strPath.c_str());

    strPath = std::string((char*)pParam) + "/Student.json";
    endDeal_studentInfo(strPath.c_str());

    strPath = std::string((char*)pParam) + "/Teacher.json";
    endDeal_teacherInfo(strPath.c_str());

    strPath = std::string((char*)pParam) + "/Summary.json";
    endDeal_classSummaryInfo(strPath.c_str());

    strPath = std::string((char*)pParam) + "/HotWord.json";
    endDeal_hotwordExtInfo(strPath.c_str());

    return OK;
}

/* 处理板书识别分析数据 */
BlError_E ResultsModule_NS::CResultsBase::deal_boardDetecr(AiManage_NS::BehaviorInfo_S stInfo)
{
    if (nullptr == m_stParamInfo.stNeedParam.getRecordState || nullptr == m_stParamInfo.stNeedParam.isTeacherPodium)
    {
        return NOK;
    }

    /* 判断是否是板书行为 */
    bool bIsBoard = false;
    for (const auto& item : stInfo.listBehaviorInfo)
    {
        if(AiManage_NS::TEA_BOARD == item.enAction && true == m_stParamInfo.stNeedParam.isTeacherPodium())
        {
            bIsBoard = true;
            break;
        }
    }

    /* 教育云平台教师板书 */
    teacherPlatformBoard(bIsBoard);
   
    if (m_stParamInfo.stNeedParam.getRecordState() == 0)
    {
        /* 不用进行分析 */
        return NOK;
    }

    teacherBoard(bIsBoard);
    
    return endDeal_boardDetecr(stInfo);
}

/* 处理教育云平台表情信息 */
BlError_E ResultsModule_NS::CResultsBase::deal_platformEmo(AiManage_NS::EmoInfo_S stInfo)
{
    if (nullptr == m_stParamInfo.stNeedParam.isPlatformClassTime || nullptr == m_stParamInfo.stNeedParam.getClassTime)
    {
        return NOK;
    }
    if (false == m_stParamInfo.stNeedParam.isPlatformClassTime())
    {
        return NOK;
    }
    
    std::lock_guard<std::mutex> lock(m_mtxPlatformStudent);

    /* 统计行为数量 */
    for (const auto& item : stInfo.listEmoInfo)
    {
        switch (item.enEmotion)
        {
            /* 愤怒 */
            case AiManage_NS::ANGER:
            {
                m_stPlatformStudentInfo.nAngerTotal++;
                m_stPlatformStudentInfo.stEmoInfo.nAngerTotal++;
                break;
            }
            /* 厌恶 */
            case AiManage_NS::DISGUST:
            {
                m_stPlatformStudentInfo.nDisgustTotal++;
                m_stPlatformStudentInfo.stEmoInfo.nDisgustTotal++;
                break;
            }
            /* 恐惧 */
            case AiManage_NS::FEAR:
            {
                m_stPlatformStudentInfo.nFearTotal++;
                m_stPlatformStudentInfo.stEmoInfo.nFearTotal++;
                break;
            }
            /* 快乐 */
            case AiManage_NS::JOY:
            {
                m_stPlatformStudentInfo.nJoyTotal++;
                m_stPlatformStudentInfo.stEmoInfo.nJoyTotal++;
                break;
            }
            /* 中性 */
            case AiManage_NS::NEUTRAL:
            {
                m_stPlatformStudentInfo.nNeutralTotal++;
                m_stPlatformStudentInfo.stEmoInfo.nNeutralTotal++;
                break;
            }
            /* 悲伤 */
            case AiManage_NS::SADNESS:
            {
                m_stPlatformStudentInfo.nSadnessTotal++;
                m_stPlatformStudentInfo.stEmoInfo.nSadnessTotal++;
                break;
            }
            /* 惊喜 */
            case AiManage_NS::SURPRISE:
            {
                m_stPlatformStudentInfo.nSurpriseTotal++;
                m_stPlatformStudentInfo.stEmoInfo.nSurpriseTotal++;
                break;
            }
            default:
                break;
        }
    }

    if (m_stPlatformStudentInfo.stEmoInfo.nLastCountTime <= 0)
    {
        /* 第一次保存时间 */
        m_stPlatformStudentInfo.stEmoInfo.nLastCountTime = m_stParamInfo.stNeedParam.getClassTime();
    }
    else if (m_stParamInfo.stNeedParam.getClassTime() - m_stPlatformStudentInfo.stEmoInfo.nLastCountTime >= m_stPlatformStudentInfo.stEmoInfo.nInterval)
    {
        /* 达到计算阈值 */
        int nTotal = m_stPlatformStudentInfo.stEmoInfo.nAngerTotal +
            m_stPlatformStudentInfo.stEmoInfo.nDisgustTotal +
            m_stPlatformStudentInfo.stEmoInfo.nFearTotal +
            m_stPlatformStudentInfo.stEmoInfo.nJoyTotal +
            m_stPlatformStudentInfo.stEmoInfo.nNeutralTotal +
            m_stPlatformStudentInfo.stEmoInfo.nSadnessTotal +
            m_stPlatformStudentInfo.stEmoInfo.nSurpriseTotal;

        AiManage_NS::Emotion_E enAction;

        if (nTotal > 0)
        {
            int nAverage = ((m_stPlatformStudentInfo.stEmoInfo.nAngerTotal * AiManage_NS::ANGER +
                             m_stPlatformStudentInfo.stEmoInfo.nDisgustTotal * AiManage_NS::DISGUST +
                             m_stPlatformStudentInfo.stEmoInfo.nFearTotal * AiManage_NS::FEAR +
                             m_stPlatformStudentInfo.stEmoInfo.nJoyTotal * AiManage_NS::JOY +
                             m_stPlatformStudentInfo.stEmoInfo.nNeutralTotal * AiManage_NS::NEUTRAL +
                             m_stPlatformStudentInfo.stEmoInfo.nSadnessTotal * AiManage_NS::SADNESS +
                             m_stPlatformStudentInfo.stEmoInfo.nSurpriseTotal * AiManage_NS::SURPRISE) /
                            nTotal);


            /* 返回最接近的情绪值 */
            if (nAverage >= AiManage_NS::ANGER && nAverage <= AiManage_NS::SURPRISE)
            {
                enAction = (AiManage_NS::Emotion_E)nAverage;
                if (m_stParamInfo.stNeedParam.pfPushEmoticonType)
                {
                    /*上报表情*/
                    m_stParamInfo.stNeedParam.pfPushEmoticonType(enAction);
                }
            }
            else
            {
                /* 如果平均值超出范围，则返回中性 */
                enAction = AiManage_NS::NEUTRAL;
            }
        }
        else
        {
            enAction = AiManage_NS::NEUTRAL;
        }

        /* 清空数据 */
        m_stPlatformStudentInfo.stEmoInfo.nAngerTotal    = 0;
        m_stPlatformStudentInfo.stEmoInfo.nDisgustTotal  = 0;
        m_stPlatformStudentInfo.stEmoInfo.nFearTotal     = 0;
        m_stPlatformStudentInfo.stEmoInfo.nJoyTotal      = 0;
        m_stPlatformStudentInfo.stEmoInfo.nNeutralTotal  = 0;
        m_stPlatformStudentInfo.stEmoInfo.nSadnessTotal  = 0;
        m_stPlatformStudentInfo.stEmoInfo.nSurpriseTotal = 0;

        m_stPlatformStudentInfo.stEmoInfo.nLastCountTime = m_stParamInfo.stNeedParam.getClassTime();
    }

    return OK;
}

/* 处理表情识别分析数据 */
BlError_E ResultsModule_NS::CResultsBase::deal_emoDetecr(AiManage_NS::EmoInfo_S stInfo)
{
    if (nullptr == m_stParamInfo.stNeedParam.getRecordState)
    {
        return NOK;
    }

    /*处理教育云平台表情*/
    deal_platformEmo(stInfo);

    if (m_stParamInfo.stNeedParam.getRecordState() == 0)
    {
        /* 不用进行分析 */
        return NOK;
    }

    std::lock_guard<std::mutex> lock(m_mtxStudent);

    /* 统计行为数量 */
    for (const auto& item : stInfo.listEmoInfo)
    {
        switch (item.enEmotion)
        {
            /* 愤怒 */
            case AiManage_NS::ANGER:
            {
                m_stStudentInfo.nAngerTotal++;
                m_stStudentInfo.stEmoInfo.nAngerTotal++;
                break;
            }
            /* 厌恶 */
            case AiManage_NS::DISGUST:
            {
                m_stStudentInfo.nDisgustTotal++;
                m_stStudentInfo.stEmoInfo.nDisgustTotal++;
                break;
            }
            /* 恐惧 */
            case AiManage_NS::FEAR:
            {
                m_stStudentInfo.nFearTotal++;
                m_stStudentInfo.stEmoInfo.nFearTotal++;
                break;
            }
            /* 快乐 */
            case AiManage_NS::JOY:
            {
                m_stStudentInfo.nJoyTotal++;
                m_stStudentInfo.stEmoInfo.nJoyTotal++;
                break;
            }
            /* 中性 */
            case AiManage_NS::NEUTRAL:
            {
                m_stStudentInfo.nNeutralTotal++;
                m_stStudentInfo.stEmoInfo.nNeutralTotal++;
                break;
            }
            /* 悲伤 */
            case AiManage_NS::SADNESS:
            {
                m_stStudentInfo.nSadnessTotal++;
                m_stStudentInfo.stEmoInfo.nSadnessTotal++;
                break;
            }
            /* 惊喜 */
            case AiManage_NS::SURPRISE:
            {
                m_stStudentInfo.nSurpriseTotal++;
                m_stStudentInfo.stEmoInfo.nSurpriseTotal++;
                break;
            }
            default:
                break;
        }
    }

    if (m_stStudentInfo.stEmoInfo.nLastCountTime <= 0)
    {
        /* 第一次保存时间 */
        m_stStudentInfo.stEmoInfo.nLastCountTime = stInfo.stHeadInfo.nRecordTime;
    }
    else if (stInfo.stHeadInfo.nRecordTime - m_stStudentInfo.stEmoInfo.nLastCountTime >= m_stStudentInfo.stEmoInfo.nInterval)
    {
        /* 达到计算阈值 */
        int nTotal = m_stStudentInfo.stEmoInfo.nAngerTotal +
            m_stStudentInfo.stEmoInfo.nDisgustTotal +
            m_stStudentInfo.stEmoInfo.nFearTotal +
            m_stStudentInfo.stEmoInfo.nJoyTotal +
            m_stStudentInfo.stEmoInfo.nNeutralTotal +
            m_stStudentInfo.stEmoInfo.nSadnessTotal +
            m_stStudentInfo.stEmoInfo.nSurpriseTotal;

        AiManage_NS::Emotion_E enAction;

        if (nTotal > 0)
        {
            int nAverage = ((m_stStudentInfo.stEmoInfo.nAngerTotal * AiManage_NS::ANGER +
                             m_stStudentInfo.stEmoInfo.nDisgustTotal * AiManage_NS::DISGUST +
                             m_stStudentInfo.stEmoInfo.nFearTotal * AiManage_NS::FEAR +
                             m_stStudentInfo.stEmoInfo.nJoyTotal * AiManage_NS::JOY +
                             m_stStudentInfo.stEmoInfo.nNeutralTotal * AiManage_NS::NEUTRAL +
                             m_stStudentInfo.stEmoInfo.nSadnessTotal * AiManage_NS::SADNESS +
                             m_stStudentInfo.stEmoInfo.nSurpriseTotal * AiManage_NS::SURPRISE) /
                            nTotal);


            /* 返回最接近的情绪值 */
            if (nAverage >= AiManage_NS::ANGER && nAverage <= AiManage_NS::SURPRISE)
            {
                enAction = (AiManage_NS::Emotion_E)nAverage;
            }
            else
            {
                /* 如果平均值超出范围，则返回中性 */
                enAction = AiManage_NS::NEUTRAL;
            }
        }
        else
        {
            enAction = AiManage_NS::NEUTRAL;
        }
        m_stStudentInfo.stEmoInfo.listEmotion.push_back(enAction);

        /* 清空数据 */
        m_stStudentInfo.stEmoInfo.nAngerTotal    = 0;
        m_stStudentInfo.stEmoInfo.nDisgustTotal  = 0;
        m_stStudentInfo.stEmoInfo.nFearTotal     = 0;
        m_stStudentInfo.stEmoInfo.nJoyTotal      = 0;
        m_stStudentInfo.stEmoInfo.nNeutralTotal  = 0;
        m_stStudentInfo.stEmoInfo.nSadnessTotal  = 0;
        m_stStudentInfo.stEmoInfo.nSurpriseTotal = 0;

        m_stStudentInfo.stEmoInfo.nLastCountTime = stInfo.stHeadInfo.nRecordTime;
    }

    return endDeal_emoDetecr(stInfo);
}

/* 处理学生人脸识别分析数据 */
BlError_E ResultsModule_NS::CResultsBase::deal_stFaceDetecr(AiManage_NS::FaceInfo_S stInfo)
{
    if (nullptr == m_stParamInfo.stNeedParam.getRecordState)
    {
        return NOK;
    }

    std::lock_guard<std::mutex> lock1(m_mtxPlatformAttendance);
    std::lock_guard<std::mutex> lock2(m_mtxAttendance);
    for (auto item : stInfo.listFaceInfo)
    {
        if (item.nId <= -1)
        {
            continue;
        }

        /* 查找某个人的 key */
        int  nKeyToFind = item.nId;
        auto it         = m_stPlatformAttendanceInfo.mapStudentInfo.find(nKeyToFind);
        if (it != m_stPlatformAttendanceInfo.mapStudentInfo.end())
        {
            /* 如果找到了，替换值 */
            it->second.nLastTime = stInfo.stHeadInfo.lTimestamp;
            it->second.nNumber++;
        }
        else
        {
            StAttendanceInfo_S stTmp;
            stTmp.clear();
            stTmp.nId        = item.nId;
            stTmp.nFirstTime = stInfo.stHeadInfo.lTimestamp;
            stTmp.nLastTime  = stInfo.stHeadInfo.lTimestamp;
            stTmp.nNumber    = 1;

            /* 如果没找到，添加数据 */
            m_stPlatformAttendanceInfo.mapStudentInfo[nKeyToFind] = stTmp;
        }
    }

    if (m_stParamInfo.stNeedParam.getRecordState() == 0)
    {
        /* 不用进行分析 */
        return NOK;
    }

    for (auto item : stInfo.listFaceInfo)
    {
        if (item.nId <= -1)
        {
            continue;
        }

        /* 查找某个人的 key */
        int  nKeyToFind = item.nId;
        auto it         = m_stAttendanceInfo.mapStudentInfo.find(nKeyToFind);
        if (it != m_stAttendanceInfo.mapStudentInfo.end())
        {
            /* 如果找到了，替换值 */
            it->second.nLastTime = stInfo.stHeadInfo.lTimestamp;
            it->second.nNumber++;
        }
        else
        {
            StAttendanceInfo_S stTmp;
            stTmp.clear();
            stTmp.nId        = item.nId;
            stTmp.nFirstTime = stInfo.stHeadInfo.lTimestamp;
            stTmp.nLastTime  = stInfo.stHeadInfo.lTimestamp;
            stTmp.nNumber    = 1;

            /* 如果没找到，添加数据 */
            m_stAttendanceInfo.mapStudentInfo[nKeyToFind] = stTmp;
        }
    }

    return endDeal_stFaceDetecrDetecr(stInfo);
}

/* 处理学生回答问题人脸识别分析数据 */
BlError_E ResultsModule_NS::CResultsBase::deal_stAsFaceDetecr(AiManage_NS::FaceInfo_S stInfo)
{
    if (nullptr == m_stParamInfo.stNeedParam.getRecordState)
    {
        return NOK;
    }

    std::lock_guard<std::mutex> lock1(m_mtxPlatformAttendance);

    int nId  = -1;
    int nMax = 0;
    for (auto item : stInfo.listFaceInfo)
    {
        if (item.nId <= -1)
        {
            continue;
        }

        /* 查找某个人的 key */
        int  nKeyToFind = item.nId;
        auto it         = m_stPlatformAttendanceInfo.mapStudentInfo.find(nKeyToFind);
        if (it != m_stPlatformAttendanceInfo.mapStudentInfo.end())
        {
            /* 如果找到了，替换值 */
            it->second.nLastTime = stInfo.stHeadInfo.lTimestamp;
            it->second.nNumber++;
        }
        else
        {
            StAttendanceInfo_S stTmp;
            stTmp.clear();
            stTmp.nId        = item.nId;
            stTmp.nFirstTime = stInfo.stHeadInfo.lTimestamp;
            stTmp.nLastTime  = stInfo.stHeadInfo.lTimestamp;
            stTmp.nNumber    = 1;

            /* 如果没找到，添加数据 */
            m_stPlatformAttendanceInfo.mapStudentInfo[nKeyToFind] = stTmp;
        }

        int nTemp = std::abs(item.stBoxInfo.nX2 - item.stBoxInfo.nX1) * std::abs(item.stBoxInfo.nY2 - item.stBoxInfo.nY1);
        if (nMax < nTemp)
        {
            nMax = nTemp;
            nId  = item.nId;
        }
    }

    if (nId >= 0)
    {
        auto it = m_stPlatformAttendanceInfo.mapStudentInfo.find(nId);
        if (it != m_stPlatformAttendanceInfo.mapStudentInfo.end())
        {
            if (m_stParamInfo.stNeedParam.getClassTime)
            {
                /* 添加回答问题时间点 */
                it->second.listAnswerTime.push_back(m_stParamInfo.stNeedParam.getClassTime());
                dlog(LOG_ERROR, "[%d]回答问题:[%ld]", nId, m_stParamInfo.stNeedParam.getClassTime());
            }
        }
    }

    if (m_stParamInfo.stNeedParam.getRecordState() == 0)
    {
        /* 不用进行分析 */
        return NOK;
    }

    std::lock_guard<std::mutex> lock2(m_mtxAttendance);

    nId  = -1;
    nMax = 0;
    for (auto item : stInfo.listFaceInfo)
    {
        if (item.nId <= -1)
        {
            continue;
        }

        /* 查找某个人的 key */
        int  nKeyToFind = item.nId;
        auto it         = m_stAttendanceInfo.mapStudentInfo.find(nKeyToFind);
        if (it != m_stAttendanceInfo.mapStudentInfo.end())
        {
            /* 如果找到了，替换值 */
            it->second.nLastTime = stInfo.stHeadInfo.lTimestamp;
            it->second.nNumber++;
        }
        else
        {
            StAttendanceInfo_S stTmp;
            stTmp.clear();
            stTmp.nId        = item.nId;
            stTmp.nFirstTime = stInfo.stHeadInfo.lTimestamp;
            stTmp.nLastTime  = stInfo.stHeadInfo.lTimestamp;
            stTmp.nNumber    = 1;

            /* 如果没找到，添加数据 */
            m_stAttendanceInfo.mapStudentInfo[nKeyToFind] = stTmp;
        }

        int nTemp = std::abs(item.stBoxInfo.nX2 - item.stBoxInfo.nX1) * std::abs(item.stBoxInfo.nY2 - item.stBoxInfo.nY1);
        if (nMax < nTemp)
        {
            nMax = nTemp;
            nId  = item.nId;
        }
    }

    if (nId >= 0)
    {
        auto it = m_stAttendanceInfo.mapStudentInfo.find(nId);
        if (it != m_stAttendanceInfo.mapStudentInfo.end())
        {
            /* 添加回答问题时间点 */
            it->second.listAnswerTime.push_back(stInfo.stHeadInfo.nRecordTime);
            dlog(LOG_ERROR, "[%d]回答问题:[%d]", nId, stInfo.stHeadInfo.nRecordTime);
        }
    }

    return endDeal_stAsFaceDetecrDetecr(stInfo);
}

/* 处理教师人脸识别分析数据 */
BlError_E ResultsModule_NS::CResultsBase::deal_teFaceDetecr(AiManage_NS::FaceInfo_S stInfo)
{
    if (nullptr == m_stParamInfo.stNeedParam.getRecordState)
    {
        return NOK;
    }

    if (m_stParamInfo.stNeedParam.isPlatformAiTime)
    {
        std::lock_guard<std::mutex> lock1(m_mtxPlatformAttendance);
        if (m_stParamInfo.stNeedParam.isPlatformAiTime())
        {
            if (m_stParamInfo.stNeedParam.platformTeaEvent)
            {
                /* 教师预警事件回调函数 */
                m_stParamInfo.stNeedParam.platformTeaEvent(stInfo);
            }

            for (auto item : stInfo.listFaceInfo)
            {
                if (item.nId <= -1)
                {
                    continue;
                }

                /* 查找某个人的 key */
                int  nKeyToFind = item.nId;
                auto it         = m_stPlatformAttendanceInfo.mapTeacherInfo.find(nKeyToFind);
                if (it != m_stPlatformAttendanceInfo.mapTeacherInfo.end())
                {
                    /* 如果找到了，替换值 */
                    it->second.nLastTime = stInfo.stHeadInfo.lTimestamp;
                    it->second.nNumber++;
                }
                else
                {
                    TeAttendanceInfo_S stTmp;
                    stTmp.clear();
                    stTmp.nId        = item.nId;
                    stTmp.nFirstTime = stInfo.stHeadInfo.lTimestamp;
                    stTmp.nLastTime  = stInfo.stHeadInfo.lTimestamp;
                    stTmp.nNumber    = 1;

                    /* 如果没找到，添加数据 */
                    m_stPlatformAttendanceInfo.mapTeacherInfo[nKeyToFind] = stTmp;
                }
            }
        }
    }
    
    if (m_stParamInfo.stNeedParam.getRecordState() == 0)
    {
        /* 不用进行分析 */
        return NOK;
    }

    std::lock_guard<std::mutex> lock2(m_mtxAttendance);

    for (auto item : stInfo.listFaceInfo)
    {
        if (item.nId <= -1)
        {
            continue;
        }

        /* 查找某个人的 key */
        int  nKeyToFind = item.nId;
        auto it         = m_stAttendanceInfo.mapTeacherInfo.find(nKeyToFind);
        if (it != m_stAttendanceInfo.mapTeacherInfo.end())
        {
            /* 如果找到了，替换值 */
            it->second.nLastTime = stInfo.stHeadInfo.lTimestamp;
            it->second.nNumber++;
        }
        else
        {
            TeAttendanceInfo_S stTmp;
            stTmp.clear();
            stTmp.nId        = item.nId;
            stTmp.nFirstTime = stInfo.stHeadInfo.lTimestamp;
            stTmp.nLastTime  = stInfo.stHeadInfo.lTimestamp;
            stTmp.nNumber    = 1;

            /* 如果没找到，添加数据 */
            m_stAttendanceInfo.mapTeacherInfo[nKeyToFind] = stTmp;
        }
    }

    return endDeal_teFaceDetecrDetecr(stInfo);
}

/* 处理教育云平台轨迹识别分析数据 */
BlError_E ResultsModule_NS::CResultsBase::deal_platformTrackTeacher(AiManage_NS::TrackInfo_S stInfo)
{
    if (nullptr == m_stParamInfo.stNeedParam.isPlatformClassTime)
    {
        return NOK;
    }
    if (false == m_stParamInfo.stNeedParam.isPlatformClassTime())
    {
        return NOK;
    }
    
    if (!stInfo.listTrackInfo.empty())
    {
        CoordInfo_S stCoordInfo;

        const int nCanvasWidth  = 1920; /* 画布宽度 */
        const int nCanvasHeight = 1080; /* 画布高度 */

        /* 轨迹 + 板书 */
        int  nMin  = nCanvasWidth * nCanvasHeight;
        bool bTemp = false;
        for (auto item : stInfo.listTrackInfo)
        {
            int nTemp = std::abs(item.stBoxInfo.nX2 - item.stBoxInfo.nX1) * std::abs(item.stBoxInfo.nY2 - item.stBoxInfo.nY1);
            if (nMin > nTemp || bTemp == false)
            {
                bTemp = true;
                nMin  = nTemp;

                stCoordInfo.nX = (item.stBoxInfo.nX1 + item.stBoxInfo.nX2) / 2;
                stCoordInfo.nY = (item.stBoxInfo.nY1 + item.stBoxInfo.nY2) / 2;

                /* 数据保护 */
                if (stCoordInfo.nX > nCanvasWidth)
                {
                    stCoordInfo.nX = nCanvasWidth;
                }
                else if (stCoordInfo.nX < 0)
                {
                    stCoordInfo.nX = 0;
                }

                if (stCoordInfo.nY > nCanvasHeight)
                {
                    stCoordInfo.nY = nCanvasHeight;
                }
                else if (stCoordInfo.nY < 0)
                {
                    stCoordInfo.nY = 0;
                }
            }
        }

        if (bTemp)
        {
            /* 判断是否老师在讲台 */
            if (m_stParamInfo.stNeedParam.isTeacherPodium)
            {
                if (m_stParamInfo.stNeedParam.isTeacherPodium())
                {
                    /* 在讲台 */
                    double dScaleX = (double)144 / nCanvasWidth;
                    double dScaleY = (double)20 / nCanvasHeight;

                    stCoordInfo.nX = (stCoordInfo.nX * dScaleX + 48) * (nCanvasWidth / 240);
                    stCoordInfo.nY = (stCoordInfo.nY * dScaleY + 0) * (nCanvasHeight / 135);
                    // dlog(LOG_INFO, "在讲台 [%d,%d]", stCoordInfo.nX, stCoordInfo.nY);
                    
                    /* 处理教育云平台教师指导 */
                    teacherPlatformDirecting(false);
                    
                    // dlog(LOG_INFO, "结束指导行为");

                    /* 添加轨迹 */
                    std::lock_guard<std::mutex> lock(m_mtxPlatformTeacher);
                    m_stPlatformTeacherInfo.listTrackInfo.push_back(stCoordInfo);
                }
            }
        }
    }
    else /* 没检测到人 */
    {
        /* 判断是否老师在讲台 */
        if (m_stParamInfo.stNeedParam.isTeacherPodium)
        {
            if (m_stParamInfo.stNeedParam.isTeacherPodium())
            {
                /* 处理教育云平台教师指导 */
                teacherPlatformDirecting(false);
                // dlog(LOG_INFO, "结束指导行为");
            }
        }
    }

    return OK;
}

/* 处理轨迹识别分析数据 */
BlError_E ResultsModule_NS::CResultsBase::deal_trackTeacher(AiManage_NS::TrackInfo_S stInfo)
{
    if (nullptr == m_stParamInfo.stNeedParam.getRecordState)
    {
        return NOK;
    }

    if (m_stParamInfo.stNeedParam.isPlatformClassTime)
    {
        if (m_stParamInfo.stNeedParam.isPlatformClassTime() == true)
        {
            /* 处理教育云平台轨迹识别分析数据 */
            deal_platformTrackTeacher(stInfo);
        }
    }

    if (m_stParamInfo.stNeedParam.getRecordState() == 0)
    {
        /* 不用进行分析 */
        return NOK;
    }

    if (!stInfo.listTrackInfo.empty())
    {
        CoordInfo_S stCoordInfo;

        const int nCanvasWidth  = 1920; /* 画布宽度 */
        const int nCanvasHeight = 1080; /* 画布高度 */

        /* 轨迹 + 板书 */
        int  nMin  = nCanvasWidth * nCanvasHeight;
        bool bTemp = false;
        for (auto item : stInfo.listTrackInfo)
        {
            int nTemp = std::abs(item.stBoxInfo.nX2 - item.stBoxInfo.nX1) * std::abs(item.stBoxInfo.nY2 - item.stBoxInfo.nY1);
            if (nMin > nTemp || bTemp == false)
            {
                bTemp = true;
                nMin  = nTemp;

                stCoordInfo.nX = (item.stBoxInfo.nX1 + item.stBoxInfo.nX2) / 2;
                stCoordInfo.nY = (item.stBoxInfo.nY1 + item.stBoxInfo.nY2) / 2;

                /* 数据保护 */
                if (stCoordInfo.nX > nCanvasWidth)
                {
                    stCoordInfo.nX = nCanvasWidth;
                }
                else if (stCoordInfo.nX < 0)
                {
                    stCoordInfo.nX = 0;
                }

                if (stCoordInfo.nY > nCanvasHeight)
                {
                    stCoordInfo.nY = nCanvasHeight;
                }
                else if (stCoordInfo.nY < 0)
                {
                    stCoordInfo.nY = 0;
                }
            }
        }

        if (bTemp)
        {
            /* 判断是否老师在讲台 */
            if (m_stParamInfo.stNeedParam.isTeacherPodium)
            {
                if (m_stParamInfo.stNeedParam.isTeacherPodium())
                {
                    /* 在讲台 */
                    double dScaleX = (double)144 / nCanvasWidth;
                    double dScaleY = (double)20 / nCanvasHeight;

                    stCoordInfo.nX = (stCoordInfo.nX * dScaleX + 48) * (nCanvasWidth / 240);
                    stCoordInfo.nY = (stCoordInfo.nY * dScaleY + 0) * (nCanvasHeight / 135);
                    // dlog(LOG_INFO, "在讲台 [%d,%d]", stCoordInfo.nX, stCoordInfo.nY);

                    teacherDirecting(false);
                    // dlog(LOG_INFO, "结束指导行为");

                    /* 添加轨迹 */
                    std::lock_guard<std::mutex> lock(m_mtxTeacher);
                    m_stTeacherInfo.listTrackInfo.push_back(stCoordInfo);
                }
            }
        }
    }
    else /* 没检测到人 */
    {
        /* 判断是否老师在讲台 */
        if (m_stParamInfo.stNeedParam.isTeacherPodium)
        {
            if (m_stParamInfo.stNeedParam.isTeacherPodium())
            {
                teacherDirecting(false);
                // dlog(LOG_INFO, "结束指导行为");
            }
        }
    }

    return endDeal_trackTeacher(stInfo);
}

/* 处理教师接打电话分析数据 */
BlError_E ResultsModule_NS::CResultsBase::deal_CallPhone(AiManage_NS::BehaviorInfo_S stInfo)
{
    if (nullptr == m_stParamInfo.stNeedParam.isPlatformClassTime
    || nullptr == m_stParamInfo.stNeedParam.getPlatformSwitch
    || nullptr == m_stParamInfo.stNeedParam.platformPushAlertType)
    {
        return NOK;
    }
    if (false == m_stParamInfo.stNeedParam.isPlatformClassTime())
    {
        return NOK;
    }

    std::lock_guard<std::mutex> lock(m_mtxPlatformStudent);
    
    bool bIsCallPhone = false;

    for (const auto& item : stInfo.listBehaviorInfo)
    {
        if (AiManage_NS::CALL_PHONE == item.enAction)
        {
            bIsCallPhone = true;
            break;
        }
    }

    if (1 == m_stParamInfo.stNeedParam.getPlatformSwitch(TEACHER_MAKEING_PHONE_CALL))
    {
        if (bIsCallPhone)
        {
            static long s_lPrevTimestamp = 0;
            /* 每隔30秒上报一次 */
            if (stInfo.stHeadInfo.lTimestamp - s_lPrevTimestamp > 30)
            {
                /* 上报教师接打电话 TEACHER_MAKEING_PHONE_CALL - 教师接打电话*/
                m_stParamInfo.stNeedParam.platformPushAlertType(TEACHER_MAKEING_PHONE_CALL, "教师接打电话");
                s_lPrevTimestamp = stInfo.stHeadInfo.lTimestamp;
            }
        }
    }

    return endDeal_CallPhone(stInfo);
}

/* 处理学生玩手机分析数据 */
BlError_E ResultsModule_NS::CResultsBase::deal_PlayPhone(AiManage_NS::BehaviorInfo_S stInfo)
{
    if (nullptr == m_stParamInfo.stNeedParam.isPlatformClassTime
    || nullptr == m_stParamInfo.stNeedParam.getPlatformSwitch)
    {
        return NOK;
    }
    if(false == m_stParamInfo.stNeedParam.isPlatformClassTime())
    {
        return NOK;
    }

    std::lock_guard<std::mutex> lock(m_mtxPlatformStudent);
    bool bIsPlayPhone = false;
    for (const auto& item : stInfo.listBehaviorInfo)
    {
        if (AiManage_NS::PLAY_PHONE == item.enAction)
        {
            bIsPlayPhone = true;
            break;
        }
    }

    if (1 == m_stParamInfo.stNeedParam.getPlatformSwitch(STUDENT_PLAYING_PHONE))
    {
        if (bIsPlayPhone)
        {
            static long s_lPrevTimestamp = 0;
            /* 每隔30秒上报一次 */
            if (stInfo.stHeadInfo.lTimestamp - s_lPrevTimestamp > 30)
            {
                /* 上报学生玩手机 STUDENT_PLAYING_PHONE - 学生玩手机*/
                m_stParamInfo.stNeedParam.platformPushAlertType(STUDENT_PLAYING_PHONE, "学生玩手机");
                s_lPrevTimestamp = stInfo.stHeadInfo.lTimestamp;
            }
        }
    }

    return endDeal_PlayPhone(stInfo);
}

/* 获取当前时间戳 */
long ResultsModule_NS::CResultsBase::getSecondsTimestamp()
{
    /* 加锁 */
    std::lock_guard<std::mutex> lock(m_mtxCurrTime);
    /* 获取当前时间点 */
    auto now = std::chrono::system_clock::now();
    /* 转换为秒 */
    auto seconds = std::chrono::duration_cast<std::chrono::seconds>(now.time_since_epoch());
    /* 返回秒数 */ 
    return seconds.count();
}


/* 处理学生课堂纪律和学生聚焦事件 */
BlError_E ResultsModule_NS::CResultsBase::deal_discipline(AiManage_NS::MoveProbability_S stMoveProbability)
{
    if (nullptr == m_stParamInfo.stNeedParam.platformPushAlertType || nullptr == m_stParamInfo.stNeedParam.isPlatformClassTime
    || nullptr == m_stParamInfo.stNeedParam.getPlatformSwitch)
    {
        return NOK;
    }
    if (false == m_stParamInfo.stNeedParam.isPlatformClassTime())
    {
        return NOK;
    }
   
    dlog(LOG_USER,"stMoveProbability.fMoveProbability=%f",stMoveProbability.fMoveProbability);
    long lCtTimestamp = getSecondsTimestamp();
    dlog(LOG_USER, "学生聚焦stMoveProbability.fMoveProbability=%f",stMoveProbability.fMoveProbability);
    dlog(LOG_USER, "学生聚焦m_stAiConfidenceTh.fGatherCofid=%f",m_stAiConfidenceTh.fGatherCofid);
    if (1 == m_stParamInfo.stNeedParam.getPlatformSwitch(STUDENT_GATHER))
    {
        static int s_nDisciplineCount1 = 0;
        if (stMoveProbability.fMoveProbability > m_stAiConfidenceTh.fGatherCofid)
        {
            /* 每隔30秒且连续触发2次上报一次 */
            s_nDisciplineCount1++;
            static long s_lPrevTimestamp = 0;
            if (lCtTimestamp - s_lPrevTimestamp > 30 && s_nDisciplineCount1 >= 2)
            {
                /* 上报学生聚集 STUDENT_GATHER-学生聚集 */
                m_stParamInfo.stNeedParam.platformPushAlertType(STUDENT_GATHER, "学生聚集");
                s_lPrevTimestamp = lCtTimestamp;
                s_nDisciplineCount1 = 0;
            }
        }
        else
        {
            s_nDisciplineCount1 = 0;
        }
    }

    if (1 == m_stParamInfo.stNeedParam.getPlatformSwitch(STUDENT_POOR_DISCIPLINE))
    {
        dlog(LOG_USER, "学生课堂纪律差stMoveProbability.fMoveProbability=%f",stMoveProbability.fMoveProbability);
        dlog(LOG_USER, "学生课堂纪律差m_stAiConfidenceTh.fStuDclCofid=%f",m_stAiConfidenceTh.fStuDclCofid);
        static int s_nDisciplineCount2 = 0;
        if (stMoveProbability.fMoveProbability > m_stAiConfidenceTh.fStuDclCofid)
        {
            /* 每隔30秒且连续触发2次上报一次 */
            s_nDisciplineCount2++;
            static long s_lPrevTimestamp = 0;
            if (lCtTimestamp - s_lPrevTimestamp > 30 && s_nDisciplineCount2 >= 2)
            {
                /* 上报学生课堂纪律差 STUDENT_POOR_DISCIPLINE-学生课堂纪律差 */
                m_stParamInfo.stNeedParam.platformPushAlertType(STUDENT_POOR_DISCIPLINE, "学生课堂纪律差");
                s_lPrevTimestamp = lCtTimestamp;
                s_nDisciplineCount2 = 0;
            }
        }
        else
        {
            s_nDisciplineCount2 = 0;
        }
    }
        
    return OK;
}

/* 处理人数统计分析数据 */
BlError_E ResultsModule_NS::CResultsBase::deal_countStudents(AiManage_NS::NumberInfo_S stInfo)
{
    if (nullptr == m_stParamInfo.stNeedParam.getRecordState)
    {
        return NOK;
    }

    if (m_stParamInfo.stNeedParam.isPlatformClassTime)
    {
        m_nTotal = stInfo.nTotal;

        if (m_stParamInfo.stNeedParam.isPlatformClassTime() == true)
        {
            std::lock_guard<std::mutex> lock1(m_mtxPlatformStudent);
            std::lock_guard<std::mutex> lock2(m_mtxPlatformTeacher);
            std::lock_guard<std::mutex> lock3(m_mtxPlatformClassSummary);
            if (stInfo.nTotal > 0)
            {

                m_nCurPeopleNum = stInfo.nTotal;
                m_stPlatformStudentInfo.nFrameNum++;

                /* 检查键是否已存在，如果不存在，则插入新的键值对 */
                if (m_stPlatformStudentInfo.mapHumanCount.find(stInfo.nTotal) == m_stPlatformStudentInfo.mapHumanCount.end())
                {
                    /* 初始值设置为0 */
                    m_stPlatformStudentInfo.mapHumanCount[stInfo.nTotal] = 0;
                }

                m_stPlatformStudentInfo.mapHumanCount[stInfo.nTotal]++;
            }
            if (m_stParamInfo.stNeedParam.platformAtStuAlert && m_stParamInfo.stNeedParam.platformPushStatus)
            {
                /* 处理学生考勤信息 */
                m_stParamInfo.stNeedParam.platformAtStuAlert(stInfo);

                int nTotal = m_stPlatformStudentInfo.nListenTotal +
                    m_stPlatformStudentInfo.nPracticeTotal +
                    m_stPlatformStudentInfo.nDemonstrateTotal +
                    m_stPlatformStudentInfo.nReadTotal +
                    m_stPlatformStudentInfo.nDiscussTotal +
                    m_stPlatformStudentInfo.nDownDeskTotal;

                /* 计算学生听讲时长 */
                for (auto item : m_stPlatformStudentInfo.stBehaviorInfo.listListenTime)
                {
                    m_stPlatformStudentInfo.nListenTime += (item.nEnd - item.nStart);
                }

                /* 计算学生阅读时长 */
                for (auto item : m_stPlatformStudentInfo.stBehaviorInfo.listReadTime)
                {
                    m_stPlatformStudentInfo.nReadTime += (item.nEnd - item.nStart);
                }

                /* 计算课堂时长 */
                if (m_stParamInfo.stNeedParam.getClassTime)
                {
                    m_stPlatformClassSummaryInfo.nClassTime = m_stParamInfo.stNeedParam.getClassTime();
                    dlog(LOG_USER, "已经上课%d秒", m_stPlatformClassSummaryInfo.nClassTime);
                }

                if (nTotal > 0)
                {
                    /* 计算抬头率 */
                    double fHeadUpRate   = std::min(m_stResultsRate.fHeadUpRate, 100.0);
                    /* 计算低头率 */
                    double fHeadDownRate = std::min(m_stResultsRate.fHeadDownRate, 100.0);

                    m_stParamInfo.stNeedParam.platformPushStatus(stInfo.nTotal, fHeadUpRate, fHeadDownRate);
                }
            }
        }
    }

    if (m_stParamInfo.stNeedParam.getRecordState() == 0)
    {
        /* 不用进行分析 */
        return NOK;
    }

    if (stInfo.nTotal > 0)
    {
        std::lock_guard<std::mutex> lock(m_mtxStudent);
        m_nCurPeopleNum = stInfo.nTotal;
        m_stStudentInfo.nFrameNum++;

        /* 检查键是否已存在，如果不存在，则插入新的键值对 */
        if (m_stStudentInfo.mapHumanCount.find(stInfo.nTotal) == m_stStudentInfo.mapHumanCount.end())
        {
            /* 初始值设置为0 */
            m_stStudentInfo.mapHumanCount[stInfo.nTotal] = 0;
        }

        m_stStudentInfo.mapHumanCount[stInfo.nTotal]++;
    }


    return endDeal_countStudents(stInfo);
}

/* 处理教育云平台学生行为分析数据 */
BlError_E ResultsModule_NS::CResultsBase::deal_platformStudentBehavior(AiManage_NS::BehaviorInfo_S stInfo)
{
    if (nullptr == m_stParamInfo.stNeedParam.isPlatformClassTime || nullptr == m_stParamInfo.stNeedParam.getClassTime)
    {
        return NOK;
    }
    if (false == m_stParamInfo.stNeedParam.isPlatformClassTime())
    {
        return NOK;
    }

    std::map<AiManage_NS::Action_E, int> behaviorCounts = {
        { AiManage_NS::ACTION_NULL, 0 },
        { AiManage_NS::LOWER_HEAD,  0 },
        { AiManage_NS::LIFT_HEAD,   0 },
        { AiManage_NS::TURN_HEAD,   0 },
        { AiManage_NS::RAISE_HAND,  0 },
        { AiManage_NS::STAND,       0 },
        { AiManage_NS::TURN,        0 },
        { AiManage_NS::DOWN_DESK,   0 }
    };


    std::lock_guard<std::mutex> lock(m_mtxPlatformStudent);

    std::list<AiManage_NS::BoxInfo_S> listCurTurnBoxInfo;
    listCurTurnBoxInfo.clear();

    /* 统计行为数量 */
    for (const auto& item : stInfo.listBehaviorInfo)
    {
        switch (item.enAction)
        {
            /* 未识别到 */
            case AiManage_NS::ACTION_NULL:
            {
                behaviorCounts[AiManage_NS::ACTION_NULL]++;
                break;
            }
            /* 低头 */
            case AiManage_NS::LOWER_HEAD:
            {
                behaviorCounts[AiManage_NS::LOWER_HEAD]++;
                m_stPlatformStudentInfo.nReadTotal++;
                m_stPlatformStudentInfo.stFocusScoreInfo.nReadTotal++;

                break;
            }
            /* 抬头 */
            case AiManage_NS::LIFT_HEAD:
            {
                behaviorCounts[AiManage_NS::LIFT_HEAD]++;
                m_stPlatformStudentInfo.nListenTotal++;
                m_stPlatformStudentInfo.stFocusScoreInfo.nListenTotal++;
                break;
            }
            /* 转头 */
            case AiManage_NS::TURN_HEAD:
            {
                behaviorCounts[AiManage_NS::TURN_HEAD]++;
                m_stPlatformStudentInfo.nDiscussTotal++;
                m_stPlatformStudentInfo.stFocusScoreInfo.nDiscussTotal++;
                break;
            }
            /* 转身 */
            case AiManage_NS::TURN:
            {
                /* 判断是否老师在讲台 */
                if (m_stParamInfo.stNeedParam.isTeacherPodium)
                {
                    if (!m_stParamInfo.stNeedParam.isTeacherPodium())
                    {
                        /* 不在讲台 */

                        /* 将框往中心点缩小半 */
                        AiManage_NS::BoxInfo_S stHalfBox;

                        /* 计算框的中心点坐标 */
                        int nCenterX = (item.stBoxInfo.nX1 + item.stBoxInfo.nX2) / 2;
                        int nCenterY = (item.stBoxInfo.nY1 + item.stBoxInfo.nY2) / 2;

                        /* 将左上角和右下角的坐标向中心点移动一半的距离 */
                        int nHalfWidth  = (item.stBoxInfo.nX2 - item.stBoxInfo.nX1) / 2;
                        int nHalfHeight = (item.stBoxInfo.nY2 - item.stBoxInfo.nY1) / 2;
                        stHalfBox.nX1   = nCenterX - nHalfWidth;
                        stHalfBox.nY1   = nCenterY - nHalfHeight;
                        stHalfBox.nX2   = nCenterX + nHalfWidth;
                        stHalfBox.nY2   = nCenterY + nHalfHeight;

                        listCurTurnBoxInfo.push_back(stHalfBox);
                    }
                }

                behaviorCounts[AiManage_NS::TURN]++;
                break;
            }
            /* 举手 */
            case AiManage_NS::RAISE_HAND:
            {
                behaviorCounts[AiManage_NS::RAISE_HAND]++;
                m_stPlatformStudentInfo.nPracticeTotal++;
                m_stPlatformStudentInfo.stFocusScoreInfo.nPracticeTotal++;
                break;
            }
            /* 站立 */
            case AiManage_NS::STAND:
            {

                /* 判断是否老师在讲台 */
                if (m_stParamInfo.stNeedParam.isTeacherPodium)
                {
                    if (!m_stParamInfo.stNeedParam.isTeacherPodium())
                    {
                        /* 不在讲台 */

                        /* 将框往中心点缩小半 */
                        AiManage_NS::BoxInfo_S stHalfBox;

                        /* 计算框的中心点坐标 */
                        int nCenterX = (item.stBoxInfo.nX1 + item.stBoxInfo.nX2) / 2;
                        int nCenterY = (item.stBoxInfo.nY1 + item.stBoxInfo.nY2) / 2;

                        /* 将左上角和右下角的坐标向中心点移动一半的距离 */
                        int nHalfWidth  = (item.stBoxInfo.nX2 - item.stBoxInfo.nX1) / 2;
                        int nHalfHeight = (item.stBoxInfo.nY2 - item.stBoxInfo.nY1) / 2;
                        stHalfBox.nX1   = nCenterX - nHalfWidth;
                        stHalfBox.nY1   = nCenterY - nHalfHeight;
                        stHalfBox.nX2   = nCenterX + nHalfWidth;
                        stHalfBox.nY2   = nCenterY + nHalfHeight;

                        listCurTurnBoxInfo.push_back(stHalfBox);
                    }
                }

                behaviorCounts[AiManage_NS::STAND]++;
                m_stPlatformStudentInfo.nDemonstrateTotal++;
                m_stPlatformStudentInfo.stFocusScoreInfo.nDemonstrateTotal++;
                break;
            }
            /* 趴桌 */
            case AiManage_NS::DOWN_DESK:
            {
                behaviorCounts[AiManage_NS::DOWN_DESK]++;
                m_stPlatformStudentInfo.nDownDeskTotal++;
                m_stPlatformStudentInfo.stFocusScoreInfo.nDownDeskTotal++;
                break;
            }
            default:
                break;
        }
    }

    bool       bExistTrack     = false;
    static int s_nDirectingNum = 0;
    if (!listCurTurnBoxInfo.empty())
    {
        if (m_stPfCurSampleBoxInfo.empty())
        {
            /* 计算链表中距离上边距最远的数据 */
            int nMaxDistance = 0;

            for (const auto& box : listCurTurnBoxInfo)
            {
                int nDistance = std::max(box.nY1, box.nY2);
                if (nDistance > nMaxDistance)
                {
                    nMaxDistance           = nDistance;
                    m_stPfCurSampleBoxInfo = box;
                }
            }

            m_stPfLastSampleBoxInfo = m_stPfCurSampleBoxInfo;
            s_nDirectingNum         = 0;
        }
        else
        {
            /* 找到最近的框 */
            double                 dMinDistance = std::numeric_limits<double>::max();
            AiManage_NS::BoxInfo_S stClosestBox = m_stPfCurSampleBoxInfo;

            for (const auto& box : listCurTurnBoxInfo)
            {
                double dCenter1X = (m_stPfCurSampleBoxInfo.nX1 + m_stPfCurSampleBoxInfo.nX2) / 2.0;
                double dCenter1Y = (m_stPfCurSampleBoxInfo.nY1 + m_stPfCurSampleBoxInfo.nY2) / 2.0;
                double dCenter2X = (box.nX1 + box.nX2) / 2.0;
                double dCenter2Y = (box.nY1 + box.nY2) / 2.0;

                double dDistance = sqrt(pow(dCenter1X - dCenter2X, 2) + pow(dCenter1Y - dCenter2Y, 2));
                if (dDistance < dMinDistance)
                {
                    dMinDistance = dDistance;
                    stClosestBox = box;
                }
            }
            m_stPfCurSampleBoxInfo = stClosestBox;
        }

        bExistTrack = true;
    }
    else
    {
        /* 判断是否老师在讲台 */
        if (m_stParamInfo.stNeedParam.isTeacherPodium)
        {
            if (!m_stParamInfo.stNeedParam.isTeacherPodium())
            {
                bExistTrack = true;
            }
        }
    }

    /* 存在轨迹 */
    if (bExistTrack)
    {
        /* 比较是否重叠 */
        if (!m_stPfCurSampleBoxInfo.empty() && !m_stPfLastSampleBoxInfo.empty())
        {
            /* 检查水平方向是否有重叠 */
            bool bHorizontalOverlap = (std::max(m_stPfCurSampleBoxInfo.nX1, m_stPfLastSampleBoxInfo.nX1) <=
                                       std::min(m_stPfCurSampleBoxInfo.nX2, m_stPfLastSampleBoxInfo.nX2));
            /* 检查垂直方向是否有重叠 */
            bool bVerticalOverlap   = (std::max(m_stPfCurSampleBoxInfo.nY1, m_stPfLastSampleBoxInfo.nY1) <=
                                     std::min(m_stPfCurSampleBoxInfo.nY2, m_stPfLastSampleBoxInfo.nY2));

            /* 判断是否为指导行为 */
            if (bHorizontalOverlap && bVerticalOverlap)
            {
                /* 如果水平和垂直方向都有重叠，则框重叠 */
                s_nDirectingNum++;
                // dlog(LOG_INFO, "指导行为计数 +1[%d] [%d,%d %d,%d] [%d,%d %d,%d]",
                //      s_nDirectingNum,
                //      m_stPfCurSampleBoxInfo.nX1,
                //      m_stPfCurSampleBoxInfo.nY1,
                //      m_stPfCurSampleBoxInfo.nX2,
                //      m_stPfCurSampleBoxInfo.nY2,
                //      m_stPfLastSampleBoxInfo.nX1,
                //      m_stPfLastSampleBoxInfo.nY1,
                //      m_stPfLastSampleBoxInfo.nX2,
                //      m_stPfLastSampleBoxInfo.nY2);

                if (s_nDirectingNum > 5)
                {
                    s_nDirectingNum = 5;
                    
                    /* 处理教育云平台教师指导 */
                    teacherPlatformDirecting(true);
                    
                    dlog(LOG_USER, "开始指导行为");
                }
            }
            else
            {
                s_nDirectingNum--;
                // dlog(LOG_INFO, "指导行为计数 -1[%d] [%d,%d %d,%d] [%d,%d %d,%d]",
                //      s_nDirectingNum,
                //      m_stPfCurSampleBoxInfo.nX1,
                //      m_stPfCurSampleBoxInfo.nY1,
                //      m_stPfCurSampleBoxInfo.nX2,
                //      m_stPfCurSampleBoxInfo.nY2,
                //      m_stPfLastSampleBoxInfo.nX1,
                //      m_stPfLastSampleBoxInfo.nY1,
                //      m_stPfLastSampleBoxInfo.nX2,
                //      m_stPfLastSampleBoxInfo.nY2);

                if (s_nDirectingNum <= 0)
                {
                    m_stPfLastSampleBoxInfo = m_stPfCurSampleBoxInfo;
                    s_nDirectingNum         = 0;
                    
                    /* 处理教育云平台教师指导 */
                    teacherPlatformDirecting(false);
                
                    // dlog(LOG_INFO, "结束指导行为");
                }
            }
        }




        /* 记录轨迹 */
        CoordInfo_S stCoordInfo;

        stCoordInfo.nX = (m_stPfCurSampleBoxInfo.nX1 + m_stPfCurSampleBoxInfo.nX2) / 2;
        stCoordInfo.nY = (m_stPfCurSampleBoxInfo.nY1 + m_stPfCurSampleBoxInfo.nY2) / 2;

        /* 数据保护 */
        if (stCoordInfo.nX > 1920)
        {
            stCoordInfo.nX = 1920;
        }
        else if (stCoordInfo.nX < 0)
        {
            stCoordInfo.nX = 0;
        }

        if (stCoordInfo.nY > 1080)
        {
            stCoordInfo.nY = 1080;
        }
        else if (stCoordInfo.nY < 0)
        {
            stCoordInfo.nY = 0;
        }

        /* 转换 */
        double dScaleX = (double)208 / 1920;
        double dScaleY = (double)106 / 1080;

        stCoordInfo.nX = 1920 - (stCoordInfo.nX * dScaleX + 16) * (1920 / 240);
        stCoordInfo.nY = 1080 - (stCoordInfo.nY * dScaleY + 24) * (1080 / 135);

        /* 添加轨迹 */
        std::lock_guard<std::mutex> lock(m_mtxTeacher);
        m_stPlatformTeacherInfo.listTrackInfo.push_back(stCoordInfo);
    }


    /* 判断是否学生特写 */
    if (m_stParamInfo.stNeedParam.isStudentCloseUp && behaviorCounts[AiManage_NS::STAND] == 0)
    {
        if (m_stParamInfo.stNeedParam.isStudentCloseUp())
        {
            behaviorCounts[AiManage_NS::STAND]++;
            m_stPlatformStudentInfo.nDemonstrateTotal++;
            m_stPlatformStudentInfo.stFocusScoreInfo.nDemonstrateTotal++;
        }
    }

    /* 总人数 */
    int nTotal = 0;
    for (const auto& pair : behaviorCounts)
    {
        nTotal += pair.second;
    }

    /* 未识别人数 */
    int nUnknown = m_nCurPeopleNum - nTotal;

    if (nUnknown > 0 && nTotal > 0)
    {
        behaviorCounts[AiManage_NS::LIFT_HEAD] += nUnknown;
        m_stPlatformStudentInfo.nListenTotal   += nUnknown;
        nTotal                                 += nUnknown;
    }

    if (nTotal > 0)
    {
        /* 计算当前行为 */
        for (const auto& pair : behaviorCounts)
        {
            int  nProportion = pair.second * 100.0 / nTotal;
            bool bBehavior   = false;
            switch (pair.first)
            {
                /* 低头 */
                case AiManage_NS::LOWER_HEAD:
                {
                    /* 低头率 */
                    if (m_nTotal > 0)
                    {
                        m_stResultsRate.fHeadDownRate = pair.second * 100.0 / m_nTotal;
                    }

                    if (nProportion > 20)
                    {
                        bBehavior = true;
                    }

                    dlog(LOG_USER, "低头-nProportion：%d", nProportion);

                    add_platformStudentBehavior(
                        m_stPlatformStudentInfo.stBehaviorInfo.listReadTime,
                        bBehavior,
                        m_stParamInfo.stNeedParam.getClassTime(),
                        READ);
                    break;
                }
                /* 抬头 */
                case AiManage_NS::LIFT_HEAD:
                {
                    /* 抬头率 */
                    if (m_nTotal > 0)
                    {
                        m_stResultsRate.fHeadUpRate = pair.second * 100.0 / m_nTotal;
                    }

                    if (nProportion > 30)
                    {
                        bBehavior = true;
                    }
                    dlog(LOG_USER, "抬头-nProportion：%d", nProportion);

                    add_platformStudentBehavior(
                        m_stPlatformStudentInfo.stBehaviorInfo.listListenTime,
                        bBehavior,
                        m_stParamInfo.stNeedParam.getClassTime(),
                        LISTEN_TO_TALK);
                    break;
                }
                /* 转头 */
                case AiManage_NS::TURN_HEAD:
                {
                    if (nProportion > 30)
                    {
                        bBehavior = true;
                    }
                    dlog(LOG_USER, "转头-nProportion：%d", nProportion);

                    add_platformStudentBehavior(
                        m_stPlatformStudentInfo.stBehaviorInfo.listDiscussTime,
                        bBehavior,
                        m_stParamInfo.stNeedParam.getClassTime(),
                        DISCUSSION);
                    break;
                }
                /* 转身 */
                case AiManage_NS::TURN:
                {
                    if (nProportion > 0)
                    {
                        bBehavior = true;
                        save_stFullView(m_stParamInfo.stNeedParam.getClassTime());
                    }
                    break;
                }
                /* 举手 */
                case AiManage_NS::RAISE_HAND:
                {
                    // if (pair.second > 0)
                    if (nProportion > 5)
                    {
                        bBehavior = true;
                    }
                    dlog(LOG_USER, "举手-nProportion：%d", nProportion);

                    add_platformStudentBehavior(
                        m_stPlatformStudentInfo.stBehaviorInfo.listPracticeTime,
                        bBehavior,
                        m_stParamInfo.stNeedParam.getClassTime(),
                        PRAXIS);
                    break;
                }
                /* 站立 */
                case AiManage_NS::STAND:
                {
                    if (pair.second > 0)
                    {
                        bBehavior = true;
                    }

                    // add_platformStudentBehavior(
                    //     m_stPlatformStudentInfo.stBehaviorInfo.listDemonstrateTime,
                    //     bBehavior,
                    //     m_stParamInfo.stNeedParam.getClassTime());
                    break;
                }
                /* 趴桌 */
                case AiManage_NS::DOWN_DESK:
                {
                    if (pair.second > 0)
                    {
                        bBehavior = true;
                        if (m_stParamInfo.stNeedParam.platformSdStuAlert)
                        {
                            /* 处理教育云平台趴桌预警 */
                            m_stParamInfo.stNeedParam.platformSdStuAlert(pair.second);
                        }
                    }

                    add_platformStudentBehavior(
                        m_stPlatformStudentInfo.stBehaviorInfo.listDownDeskTime,
                        bBehavior,
                        m_stParamInfo.stNeedParam.getClassTime(), -1);
                    break;
                }
                default:
                    break;
            }
        }
    }

    /* 计算专注值分数 */
    if (m_stPlatformStudentInfo.stFocusScoreInfo.nLastCountTime <= 0)
    {
        /* 第一次保存时间 */
        m_stPlatformStudentInfo.stFocusScoreInfo.nLastCountTime = m_stParamInfo.stNeedParam.getClassTime();
    }
    else if (m_stParamInfo.stNeedParam.getClassTime() - m_stPlatformStudentInfo.stFocusScoreInfo.nLastCountTime >= m_stPlatformStudentInfo.stEmoInfo.nInterval)
    {
        /* 达到计算阈值 */
        int nTotal = m_stPlatformStudentInfo.stFocusScoreInfo.nListenTotal +
            m_stPlatformStudentInfo.stFocusScoreInfo.nPracticeTotal +
            m_stPlatformStudentInfo.stFocusScoreInfo.nDemonstrateTotal +
            m_stPlatformStudentInfo.stFocusScoreInfo.nReadTotal +
            m_stPlatformStudentInfo.stFocusScoreInfo.nDiscussTotal +
            m_stPlatformStudentInfo.stFocusScoreInfo.nDownDeskTotal;

        int nScore = 0;

        if (nTotal > 0)
        {
            nScore = ((m_stPlatformStudentInfo.stFocusScoreInfo.nListenTotal +
                       m_stPlatformStudentInfo.stFocusScoreInfo.nDemonstrateTotal +
                       m_stPlatformStudentInfo.stFocusScoreInfo.nPracticeTotal) *
                      100.0 /
                      nTotal);
        }

        m_stPlatformStudentInfo.stFocusScoreInfo.listScore.push_back(nScore);

        /* 清空数据 */
        m_stPlatformStudentInfo.stFocusScoreInfo.nListenTotal      = 0;
        m_stPlatformStudentInfo.stFocusScoreInfo.nPracticeTotal    = 0;
        m_stPlatformStudentInfo.stFocusScoreInfo.nDemonstrateTotal = 0;
        m_stPlatformStudentInfo.stFocusScoreInfo.nReadTotal        = 0;
        m_stPlatformStudentInfo.stFocusScoreInfo.nDiscussTotal     = 0;
        m_stPlatformStudentInfo.stFocusScoreInfo.nDownDeskTotal    = 0;

        m_stPlatformStudentInfo.stFocusScoreInfo.nLastCountTime = m_stParamInfo.stNeedParam.getClassTime();

        /* 截图 */
        // if (m_stParamInfo.stNeedParam.sendStuPanoSS && nScore >= 65)
        // {
        //     /* 创建目录 */
        //     CPublicFunc::makeDirectory(KEY_SNAPS_TEMP_PATH);

        //     /* 保存截图文件文件 */
        //     std::string strPicPath = std::string(KEY_SNAPS_TEMP_PATH) +
        //         std::string("/concentration_") +
        //         std::to_string(m_stParamInfo.stNeedParam.getClassTime()) +
        //         std::string(".jpg");
        //     m_stParamInfo.stNeedParam.sendStuPanoSS(strPicPath);
        // }
    }


    return OK;
}

/* 处理学生行为分析数据 */
BlError_E ResultsModule_NS::CResultsBase::deal_studentBehavior(AiManage_NS::BehaviorInfo_S stInfo)
{
    if (nullptr == m_stParamInfo.stNeedParam.getRecordState)
    {
        return NOK;
    }

    if (m_stParamInfo.stNeedParam.isPlatformClassTime)
    {
        if (m_stParamInfo.stNeedParam.isPlatformClassTime() == true)
        {
            /* 处理教育云平台学生行为分析数据 */
            deal_platformStudentBehavior(stInfo);
        }
    }

    if (m_stParamInfo.stNeedParam.getRecordState() == 0)
    {
        /* 不用进行分析 */
        return NOK;
    }

    std::map<AiManage_NS::Action_E, int> behaviorCounts = {
        { AiManage_NS::ACTION_NULL, 0 },
        { AiManage_NS::LOWER_HEAD,  0 },
        { AiManage_NS::LIFT_HEAD,   0 },
        { AiManage_NS::TURN_HEAD,   0 },
        { AiManage_NS::RAISE_HAND,  0 },
        { AiManage_NS::STAND,       0 },
        { AiManage_NS::TURN,        0 },
        { AiManage_NS::DOWN_DESK,   0 }
    };


    std::lock_guard<std::mutex> lock(m_mtxStudent);

    std::list<AiManage_NS::BoxInfo_S> listCurTurnBoxInfo;
    listCurTurnBoxInfo.clear();

    /* 统计行为数量 */
    for (const auto& item : stInfo.listBehaviorInfo)
    {
        switch (item.enAction)
        {
            /* 未识别到 */
            case AiManage_NS::ACTION_NULL:
            {
                behaviorCounts[AiManage_NS::ACTION_NULL]++;
                break;
            }
            /* 低头 */
            case AiManage_NS::LOWER_HEAD:
            {
                behaviorCounts[AiManage_NS::LOWER_HEAD]++;
                m_stStudentInfo.nReadTotal++;
                m_stStudentInfo.stFocusScoreInfo.nReadTotal++;

                break;
            }
            /* 抬头 */
            case AiManage_NS::LIFT_HEAD:
            {
                behaviorCounts[AiManage_NS::LIFT_HEAD]++;
                m_stStudentInfo.nListenTotal++;
                m_stStudentInfo.stFocusScoreInfo.nListenTotal++;
                break;
            }
            /* 转头 */
            case AiManage_NS::TURN_HEAD:
            {
                behaviorCounts[AiManage_NS::TURN_HEAD]++;
                m_stStudentInfo.nDiscussTotal++;
                m_stStudentInfo.stFocusScoreInfo.nDiscussTotal++;
                break;
            }
            /* 转身 */
            case AiManage_NS::TURN:
            {
                /* 判断是否老师在讲台 */
                if (m_stParamInfo.stNeedParam.isTeacherPodium)
                {
                    if (!m_stParamInfo.stNeedParam.isTeacherPodium())
                    {
                        /* 不在讲台 */

                        /* 将框往中心点缩小半 */
                        AiManage_NS::BoxInfo_S stHalfBox;

                        /* 计算框的中心点坐标 */
                        int nCenterX = (item.stBoxInfo.nX1 + item.stBoxInfo.nX2) / 2;
                        int nCenterY = (item.stBoxInfo.nY1 + item.stBoxInfo.nY2) / 2;

                        /* 将左上角和右下角的坐标向中心点移动一半的距离 */
                        int nHalfWidth  = (item.stBoxInfo.nX2 - item.stBoxInfo.nX1) / 2;
                        int nHalfHeight = (item.stBoxInfo.nY2 - item.stBoxInfo.nY1) / 2;
                        stHalfBox.nX1   = nCenterX - nHalfWidth;
                        stHalfBox.nY1   = nCenterY - nHalfHeight;
                        stHalfBox.nX2   = nCenterX + nHalfWidth;
                        stHalfBox.nY2   = nCenterY + nHalfHeight;

                        listCurTurnBoxInfo.push_back(stHalfBox);
                    }
                }

                behaviorCounts[AiManage_NS::TURN]++;
                break;
            }
            /* 举手 */
            case AiManage_NS::RAISE_HAND:
            {
                behaviorCounts[AiManage_NS::RAISE_HAND]++;
                m_stStudentInfo.nPracticeTotal++;
                m_stStudentInfo.stFocusScoreInfo.nPracticeTotal++;
                break;
            }
            /* 站立 */
            case AiManage_NS::STAND:
            {

                /* 判断是否老师在讲台 */
                if (m_stParamInfo.stNeedParam.isTeacherPodium)
                {
                    if (!m_stParamInfo.stNeedParam.isTeacherPodium())
                    {
                        /* 不在讲台 */

                        /* 将框往中心点缩小半 */
                        AiManage_NS::BoxInfo_S stHalfBox;

                        /* 计算框的中心点坐标 */
                        int nCenterX = (item.stBoxInfo.nX1 + item.stBoxInfo.nX2) / 2;
                        int nCenterY = (item.stBoxInfo.nY1 + item.stBoxInfo.nY2) / 2;

                        /* 将左上角和右下角的坐标向中心点移动一半的距离 */
                        int nHalfWidth  = (item.stBoxInfo.nX2 - item.stBoxInfo.nX1) / 2;
                        int nHalfHeight = (item.stBoxInfo.nY2 - item.stBoxInfo.nY1) / 2;
                        stHalfBox.nX1   = nCenterX - nHalfWidth;
                        stHalfBox.nY1   = nCenterY - nHalfHeight;
                        stHalfBox.nX2   = nCenterX + nHalfWidth;
                        stHalfBox.nY2   = nCenterY + nHalfHeight;

                        listCurTurnBoxInfo.push_back(stHalfBox);
                    }
                }

                behaviorCounts[AiManage_NS::STAND]++;
                m_stStudentInfo.nDemonstrateTotal++;
                m_stStudentInfo.stFocusScoreInfo.nDemonstrateTotal++;
                break;
            }
            /* 趴桌 */
            case AiManage_NS::DOWN_DESK:
            {
                behaviorCounts[AiManage_NS::DOWN_DESK]++;
                m_stStudentInfo.nDownDeskTotal++;
                m_stStudentInfo.stFocusScoreInfo.nDownDeskTotal++;
                break;
            }
            default:
                break;
        }
    }

    bool       bExistTrack     = false;
    static int s_nDirectingNum = 0;
    if (!listCurTurnBoxInfo.empty())
    {
        if (m_stCurSampleBoxInfo.empty())
        {
            /* 计算链表中距离上边距最远的数据 */
            int nMaxDistance = 0;

            for (const auto& box : listCurTurnBoxInfo)
            {
                int nDistance = std::max(box.nY1, box.nY2);
                if (nDistance > nMaxDistance)
                {
                    nMaxDistance         = nDistance;
                    m_stCurSampleBoxInfo = box;
                }
            }

            m_stLastSampleBoxInfo = m_stCurSampleBoxInfo;
            s_nDirectingNum       = 0;
        }
        else
        {
            /* 找到最近的框 */
            double                 dMinDistance = std::numeric_limits<double>::max();
            AiManage_NS::BoxInfo_S stClosestBox = m_stCurSampleBoxInfo;

            for (const auto& box : listCurTurnBoxInfo)
            {
                double dCenter1X = (m_stCurSampleBoxInfo.nX1 + m_stCurSampleBoxInfo.nX2) / 2.0;
                double dCenter1Y = (m_stCurSampleBoxInfo.nY1 + m_stCurSampleBoxInfo.nY2) / 2.0;
                double dCenter2X = (box.nX1 + box.nX2) / 2.0;
                double dCenter2Y = (box.nY1 + box.nY2) / 2.0;

                double dDistance = sqrt(pow(dCenter1X - dCenter2X, 2) + pow(dCenter1Y - dCenter2Y, 2));
                if (dDistance < dMinDistance)
                {
                    dMinDistance = dDistance;
                    stClosestBox = box;
                }
            }
            m_stCurSampleBoxInfo = stClosestBox;
        }

        bExistTrack = true;
    }
    else
    {
        /* 判断是否老师在讲台 */
        if (m_stParamInfo.stNeedParam.isTeacherPodium)
        {
            if (!m_stParamInfo.stNeedParam.isTeacherPodium())
            {
                bExistTrack = true;
            }
        }
    }

    /* 存在轨迹 */
    if (bExistTrack)
    {
        /* 比较是否重叠 */
        if (!m_stCurSampleBoxInfo.empty() && !m_stLastSampleBoxInfo.empty())
        {
            /* 检查水平方向是否有重叠 */
            bool bHorizontalOverlap = (std::max(m_stCurSampleBoxInfo.nX1, m_stLastSampleBoxInfo.nX1) <=
                                       std::min(m_stCurSampleBoxInfo.nX2, m_stLastSampleBoxInfo.nX2));
            /* 检查垂直方向是否有重叠 */
            bool bVerticalOverlap   = (std::max(m_stCurSampleBoxInfo.nY1, m_stLastSampleBoxInfo.nY1) <=
                                     std::min(m_stCurSampleBoxInfo.nY2, m_stLastSampleBoxInfo.nY2));

            /* 判断是否为指导行为 */
            if (bHorizontalOverlap && bVerticalOverlap)
            {
                /* 如果水平和垂直方向都有重叠，则框重叠 */
                s_nDirectingNum++;
                // dlog(LOG_INFO, "指导行为计数 +1[%d] [%d,%d %d,%d] [%d,%d %d,%d]",
                //      s_nDirectingNum,
                //      m_stCurSampleBoxInfo.nX1,
                //      m_stCurSampleBoxInfo.nY1,
                //      m_stCurSampleBoxInfo.nX2,
                //      m_stCurSampleBoxInfo.nY2,
                //      m_stLastSampleBoxInfo.nX1,
                //      m_stLastSampleBoxInfo.nY1,
                //      m_stLastSampleBoxInfo.nX2,
                //      m_stLastSampleBoxInfo.nY2);

                if (s_nDirectingNum > 5)
                {
                    s_nDirectingNum = 5;
                    teacherDirecting(true);
                    // dlog(LOG_INFO, "开始指导行为");
                }
            }
            else
            {
                s_nDirectingNum--;
                // dlog(LOG_INFO, "指导行为计数 -1[%d] [%d,%d %d,%d] [%d,%d %d,%d]",
                //      s_nDirectingNum,
                //      m_stCurSampleBoxInfo.nX1,
                //      m_stCurSampleBoxInfo.nY1,
                //      m_stCurSampleBoxInfo.nX2,
                //      m_stCurSampleBoxInfo.nY2,
                //      m_stLastSampleBoxInfo.nX1,
                //      m_stLastSampleBoxInfo.nY1,
                //      m_stLastSampleBoxInfo.nX2,
                //      m_stLastSampleBoxInfo.nY2);

                if (s_nDirectingNum <= 0)
                {
                    m_stLastSampleBoxInfo = m_stCurSampleBoxInfo;
                    s_nDirectingNum       = 0;
                    teacherDirecting(false);
                    // dlog(LOG_INFO, "结束指导行为");
                }
            }
        }




        /* 记录轨迹 */
        CoordInfo_S stCoordInfo;

        stCoordInfo.nX = (m_stCurSampleBoxInfo.nX1 + m_stCurSampleBoxInfo.nX2) / 2;
        stCoordInfo.nY = (m_stCurSampleBoxInfo.nY1 + m_stCurSampleBoxInfo.nY2) / 2;

        /* 数据保护 */
        if (stCoordInfo.nX > 1920)
        {
            stCoordInfo.nX = 1920;
        }
        else if (stCoordInfo.nX < 0)
        {
            stCoordInfo.nX = 0;
        }

        if (stCoordInfo.nY > 1080)
        {
            stCoordInfo.nY = 1080;
        }
        else if (stCoordInfo.nY < 0)
        {
            stCoordInfo.nY = 0;
        }

        /* 转换 */
        double dScaleX = (double)208 / 1920;
        double dScaleY = (double)106 / 1080;

        stCoordInfo.nX = 1920 - (stCoordInfo.nX * dScaleX + 16) * (1920 / 240);
        stCoordInfo.nY = 1080 - (stCoordInfo.nY * dScaleY + 24) * (1080 / 135);

        /* 添加轨迹 */
        std::lock_guard<std::mutex> lock(m_mtxTeacher);
        m_stTeacherInfo.listTrackInfo.push_back(stCoordInfo);
    }


    /* 判断是否学生特写 */
    if (m_stParamInfo.stNeedParam.isStudentCloseUp && behaviorCounts[AiManage_NS::STAND] == 0)
    {
        if (m_stParamInfo.stNeedParam.isStudentCloseUp())
        {
            behaviorCounts[AiManage_NS::STAND]++;
            m_stStudentInfo.nDemonstrateTotal++;
            m_stStudentInfo.stFocusScoreInfo.nDemonstrateTotal++;
        }
    }

    /* 总人数 */
    int nTotal = 0;
    for (const auto& pair : behaviorCounts)
    {
        nTotal += pair.second;
    }

    /* 未识别人数 */
    int nUnknown = m_nCurPeopleNum - nTotal;

    if (nUnknown > 0 && nTotal > 0)
    {
        behaviorCounts[AiManage_NS::LIFT_HEAD] += nUnknown;
        m_stStudentInfo.nListenTotal           += nUnknown;
        nTotal                                 += nUnknown;
    }

    if (nTotal > 0)
    {
        /* 计算当前行为 */
        for (const auto& pair : behaviorCounts)
        {
            int  nProportion = pair.second * 100.0 / nTotal;
            bool bBehavior   = false;
            switch (pair.first)
            {
                /* 低头 */
                case AiManage_NS::LOWER_HEAD:
                {
                    if (nProportion > 20)
                    {
                        bBehavior = true;
                    }

                    add_studentBehavior(
                        m_stStudentInfo.stBehaviorInfo.listReadTime,
                        bBehavior,
                        stInfo.stHeadInfo.nRecordTime);
                    break;
                }
                /* 抬头 */
                case AiManage_NS::LIFT_HEAD:
                {
                    if (nProportion > 30)
                    {
                        bBehavior = true;
                    }

                    add_studentBehavior(
                        m_stStudentInfo.stBehaviorInfo.listListenTime,
                        bBehavior,
                        stInfo.stHeadInfo.nRecordTime);
                    break;
                }
                /* 转头 */
                case AiManage_NS::TURN_HEAD:
                {
                    if (nProportion > 30)
                    {
                        bBehavior = true;
                    }

                    add_studentBehavior(
                        m_stStudentInfo.stBehaviorInfo.listDiscussTime,
                        bBehavior,
                        stInfo.stHeadInfo.nRecordTime);
                    break;
                }
                /* 转身 */
                case AiManage_NS::TURN:
                {
                    if (nProportion > 0)
                    {
                        bBehavior = true;
                        save_stFullView(stInfo.stHeadInfo.nRecordTime);
                    }
                    break;
                }
                /* 举手 */
                case AiManage_NS::RAISE_HAND:
                {
                    // if (pair.second > 0)
                    if (nProportion > 5)
                    {
                        bBehavior = true;
                    }

                    add_studentBehavior(
                        m_stStudentInfo.stBehaviorInfo.listPracticeTime,
                        bBehavior,
                        stInfo.stHeadInfo.nRecordTime);
                    break;
                }
                /* 站立 */
                case AiManage_NS::STAND:
                {
                    if (pair.second > 0)
                    {
                        bBehavior = true;
                    }

                    // add_studentBehavior(
                    //     m_stStudentInfo.stBehaviorInfo.listDemonstrateTime,
                    //     bBehavior,
                    //     stInfo.stHeadInfo.nRecordTime);
                    break;
                }
                /* 趴桌 */
                case AiManage_NS::DOWN_DESK:
                {
                    if (pair.second > 0)
                    {
                        bBehavior = true;
                    }

                    add_studentBehavior(
                        m_stStudentInfo.stBehaviorInfo.listDownDeskTime,
                        bBehavior,
                        stInfo.stHeadInfo.nRecordTime);
                    break;
                }
                default:
                    break;
            }
        }
    }

    /* 计算专注值分数 */
    if (m_stStudentInfo.stFocusScoreInfo.nLastCountTime <= 0)
    {
        /* 第一次保存时间 */
        m_stStudentInfo.stFocusScoreInfo.nLastCountTime = stInfo.stHeadInfo.nRecordTime;
    }
    else if (stInfo.stHeadInfo.nRecordTime - m_stStudentInfo.stFocusScoreInfo.nLastCountTime >= m_stStudentInfo.stEmoInfo.nInterval)
    {
        /* 达到计算阈值 */
        int nTotal = m_stStudentInfo.stFocusScoreInfo.nListenTotal +
            m_stStudentInfo.stFocusScoreInfo.nPracticeTotal +
            m_stStudentInfo.stFocusScoreInfo.nDemonstrateTotal +
            m_stStudentInfo.stFocusScoreInfo.nReadTotal +
            m_stStudentInfo.stFocusScoreInfo.nDiscussTotal +
            m_stStudentInfo.stFocusScoreInfo.nDownDeskTotal;

        int nScore = 0;

        if (nTotal > 0)
        {
            nScore = ((m_stStudentInfo.stFocusScoreInfo.nListenTotal +
                       m_stStudentInfo.stFocusScoreInfo.nDemonstrateTotal +
                       m_stStudentInfo.stFocusScoreInfo.nPracticeTotal) *
                      100.0 /
                      nTotal);
        }

        m_stStudentInfo.stFocusScoreInfo.listScore.push_back(nScore);

        /* 清空数据 */
        m_stStudentInfo.stFocusScoreInfo.nListenTotal      = 0;
        m_stStudentInfo.stFocusScoreInfo.nPracticeTotal    = 0;
        m_stStudentInfo.stFocusScoreInfo.nDemonstrateTotal = 0;
        m_stStudentInfo.stFocusScoreInfo.nReadTotal        = 0;
        m_stStudentInfo.stFocusScoreInfo.nDiscussTotal     = 0;
        m_stStudentInfo.stFocusScoreInfo.nDownDeskTotal    = 0;

        m_stStudentInfo.stFocusScoreInfo.nLastCountTime = stInfo.stHeadInfo.nRecordTime;

        /* 截图 */
        if (m_stParamInfo.stNeedParam.sendStuPanoSS && nScore >= 65)
        {
            /* 创建目录 */
            CPublicFunc::makeDirectory(KEY_SNAPS_TEMP_PATH);

            /* 保存截图文件文件 */
            std::string strPicPath = std::string(KEY_SNAPS_TEMP_PATH) +
                std::string("/concentration_") +
                std::to_string(m_stParamInfo.stNeedParam.getRecordTime()) +
                std::string(".jpg");
            m_stParamInfo.stNeedParam.sendStuPanoSS(strPicPath);
        }
    }


    return OK;
}

/* 切换老师画面 */
BlError_E ResultsModule_NS::CResultsBase::switchTeacherScreen()
{
    if (nullptr == m_stParamInfo.stNeedParam.getRecordState)
    {
        return NOK;
    }
    std::lock_guard<std::mutex> lock1(m_mtxPlatformClassSummary);
    std::lock_guard<std::mutex> lock2(m_mtxClassSummary);
    if (m_stParamInfo.stNeedParam.isPlatformAiTime)
    {
        if (m_stParamInfo.stNeedParam.isPlatformAiTime() == true)
        {
            if (m_bPlatformStudentScreen.load())
            {
                m_stPlatformClassSummaryInfo.nStTeCutNum++;

                m_bPlatformStudentScreen.store(false);
            }
        }
    }

    if (m_stParamInfo.stNeedParam.getRecordState() == 0)
    {
        /* 不用进行分析 */
        return NOK;
    }

    if (m_bStudentScreen.load())
    {
        m_stClassSummaryInfo.nStTeCutNum++;

        m_bStudentScreen.store(false);
    }


    return OK;
}

/* 切换学生画面 */
BlError_E ResultsModule_NS::CResultsBase::switchStudentScreen()
{
    if (nullptr == m_stParamInfo.stNeedParam.getRecordState)
    {
        return NOK;
    }

    if (m_stParamInfo.stNeedParam.isPlatformAiTime)
    {
        if (m_stParamInfo.stNeedParam.isPlatformAiTime() == true)
        {
            if (!m_bPlatformStudentScreen.load())
            {
                std::lock_guard<std::mutex> lock1(m_mtxPlatformClassSummary);
                m_stPlatformClassSummaryInfo.nStTeCutNum++;

                m_bPlatformStudentScreen.store(true);
            }
        }
    }

    if (m_stParamInfo.stNeedParam.getRecordState() == 0)
    {
        /* 不用进行分析 */
        return NOK;
    }

    if (!m_bStudentScreen.load())
    {
        std::lock_guard<std::mutex> lock2(m_mtxClassSummary);
        m_stClassSummaryInfo.nStTeCutNum++;

        m_bStudentScreen.store(true);
    }

    return OK;
}

/* 切换学生特写 */
BlError_E ResultsModule_NS::CResultsBase::switchStudentCloseUp(bool bValue)
{
    if (nullptr == m_stParamInfo.stNeedParam.getRecordState)
    {
        return NOK;
    }
    std::lock_guard<std::mutex> lock1(m_mtxPlatformStudent);
    std::lock_guard<std::mutex> lock2(m_mtxStudent);
    if (m_stParamInfo.stNeedParam.isPlatformAiTime)
    {
        if (m_stParamInfo.stNeedParam.isPlatformAiTime() == true)
        {
            if (bValue)
            {
                m_stPlatformStudentInfo.nDemonstrateTotal++;
                m_stPlatformStudentInfo.stFocusScoreInfo.nDemonstrateTotal++;
            }

            if (m_stParamInfo.stNeedParam.getClassTime)
            {
                add_platformStudentBehavior(
                    m_stPlatformStudentInfo.stBehaviorInfo.listDemonstrateTime,
                    bValue,
                    m_stParamInfo.stNeedParam.getClassTime(),
                    DEMONSTRATION);
            }
        }
    }

    if (m_stParamInfo.stNeedParam.getRecordState() == 0)
    {
        /* 不用进行分析 */
        return NOK;
    }

    if (bValue)
    {
        m_stStudentInfo.nDemonstrateTotal++;
        m_stStudentInfo.stFocusScoreInfo.nDemonstrateTotal++;
    }

    if (m_stParamInfo.stNeedParam.getRecordTime)
    {
        add_studentBehavior(
            m_stStudentInfo.stBehaviorInfo.listDemonstrateTime,
            bValue,
            m_stParamInfo.stNeedParam.getRecordTime());
    }

    return OK;
}

/* 切换老师是否在讲台 */
BlError_E ResultsModule_NS::CResultsBase::switchTeacherPodium(bool bValue)
{
    if (nullptr == m_stParamInfo.stNeedParam.getRecordState)
    {
        return NOK;
    }

    if (m_stParamInfo.stNeedParam.isPlatformClassTime)
    {
        /* 教育云平台 是否在讲台或者上课时间*/
        if (bValue || m_stParamInfo.stNeedParam.isPlatformClassTime() == false)
        {
            std::lock_guard<std::mutex> lock(m_mtxPlatformStudent);
            m_stPfCurSampleBoxInfo.clear();
            m_stPfLastSampleBoxInfo.clear();
        }
    }

    if (m_stParamInfo.stNeedParam.getRecordState() == 0)
    {
        /* 不用进行分析 */
        return NOK;
    }

    if (bValue)
    {
        std::lock_guard<std::mutex> lock(m_mtxStudent);
        m_stCurSampleBoxInfo.clear();
        m_stLastSampleBoxInfo.clear();
    }
    return OK;
}

/* 教育云平台开始互动行为 */
BlError_E ResultsModule_NS::CResultsBase::startPlatformInteraction()
{
    if (nullptr == m_stParamInfo.stNeedParam.isPlatformClassTime || nullptr == m_stParamInfo.stNeedParam.getClassTime)
    {
        return NOK;
    }
    if (false == m_stParamInfo.stNeedParam.isPlatformClassTime())
    {
        return NOK;
    }
    /* 本地分析，使用这个逻辑，结束指导行为 */
    if (m_stParamInfo.stNeedParam.isLocalMode)
    {
        if (m_stParamInfo.stNeedParam.isLocalMode())
        {
            /* 处理教育云平台教师指导 */
            teacherPlatformDirecting(false);
        }
    }

    std::lock_guard<std::mutex> lock(m_mtxPlatformTeacher);


    /* 发送回答问题人脸考勤数据获取命令 */
    if (m_stParamInfo.stNeedParam.sendStreamGetData)
    {
        m_stParamInfo.stNeedParam.sendStreamGetData(AiManage_NS::AI_COM_STAS_FACE);
    }

    /* 开始互动行为 */
    if (!m_stPlatformTeacherInfo.stBehaviorInfo.listInteractionTime.empty())
    {
        TimeSlotInfo_S& lastBehavior = m_stPlatformTeacherInfo.stBehaviorInfo.listInteractionTime.back();
        if (lastBehavior.nUserParam != 0)
        {
            /* 添加一个新节点 */
            TimeSlotInfo_S stItem;
            stItem.clear();
            stItem.nUserParam = 0;
            stItem.nStart     = m_stParamInfo.stNeedParam.getClassTime();
            stItem.nEnd       = m_stParamInfo.stNeedParam.getClassTime();
            m_stPlatformTeacherInfo.stBehaviorInfo.listInteractionTime.push_back(stItem);

            /* 截图 */
            if (m_stParamInfo.stNeedParam.sendStuSpecSS)
            {
                /* 创建目录 */
                CPublicFunc::makeDirectory(KEY_SNAPS_TEMP_PATH);

                std::string strPicPath = std::string(KEY_SNAPS_TEMP_PATH) +
                    std::string("/answer_") +
                    std::to_string(m_stParamInfo.stNeedParam.getClassTime()) +
                    std::string(".jpg");
                m_stParamInfo.stNeedParam.sendStuSpecSS(strPicPath);
            }
        }
        else
        {
            lastBehavior.nEnd = m_stParamInfo.stNeedParam.getClassTime();
        }
    }
    else
    {
        /* 添加一个新节点 */
        TimeSlotInfo_S stItem;
        stItem.clear();
        stItem.nUserParam = 0;

        if (m_stPlatformTeacherInfo.stBehaviorInfo.listTourTime.empty() &&
            m_stPlatformTeacherInfo.stBehaviorInfo.listTaughtTime.empty())
        {
            stItem.nStart = 0;
        }
        else
        {
            stItem.nStart = m_stParamInfo.stNeedParam.getClassTime();
        }

        stItem.nEnd = m_stParamInfo.stNeedParam.getClassTime();
        m_stPlatformTeacherInfo.stBehaviorInfo.listInteractionTime.push_back(stItem);
    }

    /* 结束巡视课堂动作 */
    if (!m_stPlatformTeacherInfo.stBehaviorInfo.listTourTime.empty())
    {
        TimeSlotInfo_S& lastBehavior = m_stPlatformTeacherInfo.stBehaviorInfo.listTourTime.back();
        if (lastBehavior.nUserParam == 0)
        {
            lastBehavior.nUserParam = 2;
            lastBehavior.nEnd       = m_stParamInfo.stNeedParam.getClassTime();
        }
    }

    /* 结束教师讲授动作 */
    if (!m_stPlatformTeacherInfo.stBehaviorInfo.listTaughtTime.empty())
    {
        TimeSlotInfo_S& lastBehavior = m_stPlatformTeacherInfo.stBehaviorInfo.listTaughtTime.back();
        if (lastBehavior.nUserParam == 0)
        {
            lastBehavior.nUserParam = 1;
            lastBehavior.nEnd       = m_stParamInfo.stNeedParam.getClassTime();
        }
    }

    return OK;
}

/* 开始互动行为 */
BlError_E ResultsModule_NS::CResultsBase::startInteraction()
{
    if (nullptr == m_stParamInfo.stNeedParam.getRecordTime ||
        nullptr == m_stParamInfo.stNeedParam.getRecordState)
    {
        return NOK;
    }

    if (m_stParamInfo.stNeedParam.isPlatformClassTime)
    {
        if (m_stParamInfo.stNeedParam.isPlatformClassTime() == true)
        {
            /* 教育云平台开始互动行为 */
            startPlatformInteraction();
        }
    }

    if (m_stParamInfo.stNeedParam.getRecordState() == 0)
    {
        /* 不用进行分析 */
        return NOK;
    }

    /* 本地分析，使用这个逻辑，结束指导行为 */
    if (m_stParamInfo.stNeedParam.isLocalMode)
    {
        if (m_stParamInfo.stNeedParam.isLocalMode())
        {
            teacherDirecting(false);
        }
    }

    std::lock_guard<std::mutex> lock(m_mtxTeacher);

    dlog(LOG_USER, "开始互动行为：%d", m_stParamInfo.stNeedParam.getRecordTime());

    /* 发送回答问题人脸考勤数据获取命令 */
    if (m_stParamInfo.stNeedParam.sendStreamGetData)
    {
        m_stParamInfo.stNeedParam.sendStreamGetData(AiManage_NS::AI_COM_STAS_FACE);
    }

    /* 开始互动行为 */
    if (!m_stTeacherInfo.stBehaviorInfo.listInteractionTime.empty())
    {
        TimeSlotInfo_S& lastBehavior = m_stTeacherInfo.stBehaviorInfo.listInteractionTime.back();
        if (lastBehavior.nUserParam != 0)
        {
            /* 添加一个新节点 */
            TimeSlotInfo_S stItem;
            stItem.clear();
            stItem.nUserParam = 0;
            stItem.nStart     = m_stParamInfo.stNeedParam.getRecordTime();
            stItem.nEnd       = m_stParamInfo.stNeedParam.getRecordTime();
            m_stTeacherInfo.stBehaviorInfo.listInteractionTime.push_back(stItem);

            /* 截图 */
            if (m_stParamInfo.stNeedParam.sendStuSpecSS)
            {
                /* 创建目录 */
                CPublicFunc::makeDirectory(KEY_SNAPS_TEMP_PATH);

                std::string strPicPath = std::string(KEY_SNAPS_TEMP_PATH) +
                    std::string("/answer_") +
                    std::to_string(m_stParamInfo.stNeedParam.getRecordTime()) +
                    std::string(".jpg");
                m_stParamInfo.stNeedParam.sendStuSpecSS(strPicPath);
            }
        }
        else
        {
            lastBehavior.nEnd = m_stParamInfo.stNeedParam.getRecordTime();
        }
    }
    else
    {
        /* 添加一个新节点 */
        TimeSlotInfo_S stItem;
        stItem.clear();
        stItem.nUserParam = 0;

        if (m_stTeacherInfo.stBehaviorInfo.listTourTime.empty() &&
            m_stTeacherInfo.stBehaviorInfo.listTaughtTime.empty())
        {
            stItem.nStart = 0;
        }
        else
        {
            stItem.nStart = m_stParamInfo.stNeedParam.getRecordTime();
        }

        stItem.nEnd = m_stParamInfo.stNeedParam.getRecordTime();
        m_stTeacherInfo.stBehaviorInfo.listInteractionTime.push_back(stItem);
    }

    /* 结束巡视课堂动作 */
    if (!m_stTeacherInfo.stBehaviorInfo.listTourTime.empty())
    {
        TimeSlotInfo_S& lastBehavior = m_stTeacherInfo.stBehaviorInfo.listTourTime.back();
        if (lastBehavior.nUserParam == 0)
        {
            lastBehavior.nUserParam = 2;
            lastBehavior.nEnd       = m_stParamInfo.stNeedParam.getRecordTime();
        }
    }

    /* 结束教师讲授动作 */
    if (!m_stTeacherInfo.stBehaviorInfo.listTaughtTime.empty())
    {
        TimeSlotInfo_S& lastBehavior = m_stTeacherInfo.stBehaviorInfo.listTaughtTime.back();
        if (lastBehavior.nUserParam == 0)
        {
            lastBehavior.nUserParam = 1;
            lastBehavior.nEnd       = m_stParamInfo.stNeedParam.getRecordTime();
        }
    }

    return OK;
}

/* 教育云平台开始巡视行为 */
BlError_E ResultsModule_NS::CResultsBase::startPlatformTour()
{
    if (nullptr == m_stParamInfo.stNeedParam.isPlatformClassTime || nullptr == m_stParamInfo.stNeedParam.getClassTime)
    {
        return NOK;
    }
    if (false == m_stParamInfo.stNeedParam.isPlatformClassTime())
    {
        /* 处理教育云平台教师指导 */
        return NOK;
    }

    /* 本地分析，使用这个逻辑，开始指导行为 */
    if (m_stParamInfo.stNeedParam.isLocalMode)
    {
        if (m_stParamInfo.stNeedParam.isLocalMode())
        {
            /* 处理教育云平台教师指导 */
            teacherPlatformDirecting(true);
        }
    }

    std::lock_guard<std::mutex> lock(m_mtxPlatformTeacher);

    /* 开始巡视行为 */
    if (!m_stPlatformTeacherInfo.stBehaviorInfo.listTourTime.empty())
    {
        /* 添加一个新节点 */
        TimeSlotInfo_S& lastBehavior = m_stPlatformTeacherInfo.stBehaviorInfo.listTourTime.back();
        if (lastBehavior.nUserParam != 0)
        {
            TimeSlotInfo_S stItem;
            stItem.clear();
            stItem.nUserParam = 0;
            stItem.nStart     = m_stParamInfo.stNeedParam.getClassTime();
            stItem.nEnd       = m_stParamInfo.stNeedParam.getClassTime();
            m_stPlatformTeacherInfo.stBehaviorInfo.listTourTime.push_back(stItem);
        }
        else
        {
            lastBehavior.nEnd = m_stParamInfo.stNeedParam.getClassTime();
        }
    }
    else
    {
        /* 添加一个新节点 */
        TimeSlotInfo_S stItem;
        stItem.clear();
        stItem.nUserParam = 0;

        if (m_stPlatformTeacherInfo.stBehaviorInfo.listInteractionTime.empty() &&
            m_stPlatformTeacherInfo.stBehaviorInfo.listTaughtTime.empty())
        {
            stItem.nStart = 0;
        }
        else
        {
            stItem.nStart = m_stParamInfo.stNeedParam.getClassTime();
        }

        stItem.nEnd = m_stParamInfo.stNeedParam.getClassTime();
        m_stPlatformTeacherInfo.stBehaviorInfo.listTourTime.push_back(stItem);
    }

    /* 结束教师讲授动作 */
    if (!m_stPlatformTeacherInfo.stBehaviorInfo.listTaughtTime.empty())
    {
        TimeSlotInfo_S& lastBehavior = m_stPlatformTeacherInfo.stBehaviorInfo.listTaughtTime.back();
        if (lastBehavior.nUserParam == 0)
        {
            lastBehavior.nUserParam = 1;
            lastBehavior.nEnd       = m_stParamInfo.stNeedParam.getClassTime();
        }
    }

    /* 结束互动动作 */
    if (!m_stPlatformTeacherInfo.stBehaviorInfo.listInteractionTime.empty())
    {
        TimeSlotInfo_S& lastBehavior = m_stPlatformTeacherInfo.stBehaviorInfo.listInteractionTime.back();
        if (lastBehavior.nUserParam == 0)
        {
            lastBehavior.nUserParam = 3;
            lastBehavior.nEnd       = m_stParamInfo.stNeedParam.getClassTime();
        }
    }

    return OK;
}

/* 开始巡视行为 */
BlError_E ResultsModule_NS::CResultsBase::startTour()
{
    if (nullptr == m_stParamInfo.stNeedParam.getRecordTime ||
        nullptr == m_stParamInfo.stNeedParam.getRecordState)
    {
        return NOK;
    }

    if (m_stParamInfo.stNeedParam.isPlatformClassTime)
    {
        if (m_stParamInfo.stNeedParam.isPlatformClassTime() == true)
        {
            /* 教育云平台开始巡视行为 */
            startPlatformTour();
        }
    }

    if (m_stParamInfo.stNeedParam.getRecordState() == 0)
    {
        /* 不用进行分析 */
        return NOK;
    }

    /* 本地分析，使用这个逻辑，开始指导行为 */
    if (m_stParamInfo.stNeedParam.isLocalMode)
    {
        if (m_stParamInfo.stNeedParam.isLocalMode())
        {
            teacherDirecting(true);
        }
    }

    std::lock_guard<std::mutex> lock(m_mtxTeacher);

    dlog(LOG_USER, "开始巡视行为：%d", m_stParamInfo.stNeedParam.getRecordTime());
    /* 开始巡视行为 */
    if (!m_stTeacherInfo.stBehaviorInfo.listTourTime.empty())
    {
        /* 添加一个新节点 */
        TimeSlotInfo_S& lastBehavior = m_stTeacherInfo.stBehaviorInfo.listTourTime.back();
        if (lastBehavior.nUserParam != 0)
        {
            TimeSlotInfo_S stItem;
            stItem.clear();
            stItem.nUserParam = 0;
            stItem.nStart     = m_stParamInfo.stNeedParam.getRecordTime();
            stItem.nEnd       = m_stParamInfo.stNeedParam.getRecordTime();
            m_stTeacherInfo.stBehaviorInfo.listTourTime.push_back(stItem);
        }
        else
        {
            lastBehavior.nEnd = m_stParamInfo.stNeedParam.getRecordTime();
        }
    }
    else
    {
        /* 添加一个新节点 */
        TimeSlotInfo_S stItem;
        stItem.clear();
        stItem.nUserParam = 0;

        if (m_stTeacherInfo.stBehaviorInfo.listInteractionTime.empty() &&
            m_stTeacherInfo.stBehaviorInfo.listTaughtTime.empty())
        {
            stItem.nStart = 0;
        }
        else
        {
            stItem.nStart = m_stParamInfo.stNeedParam.getRecordTime();
        }

        stItem.nEnd = m_stParamInfo.stNeedParam.getRecordTime();
        m_stTeacherInfo.stBehaviorInfo.listTourTime.push_back(stItem);
    }

    /* 结束教师讲授动作 */
    if (!m_stTeacherInfo.stBehaviorInfo.listTaughtTime.empty())
    {
        TimeSlotInfo_S& lastBehavior = m_stTeacherInfo.stBehaviorInfo.listTaughtTime.back();
        if (lastBehavior.nUserParam == 0)
        {
            lastBehavior.nUserParam = 1;
            lastBehavior.nEnd       = m_stParamInfo.stNeedParam.getRecordTime();
        }
    }

    /* 结束互动动作 */
    if (!m_stTeacherInfo.stBehaviorInfo.listInteractionTime.empty())
    {
        TimeSlotInfo_S& lastBehavior = m_stTeacherInfo.stBehaviorInfo.listInteractionTime.back();
        if (lastBehavior.nUserParam == 0)
        {
            lastBehavior.nUserParam = 3;
            lastBehavior.nEnd       = m_stParamInfo.stNeedParam.getRecordTime();
        }
    }

    return OK;
}

/* 教育云平台开始教授行为 */
BlError_E ResultsModule_NS::CResultsBase::startPlatformTaught()
{
    if (nullptr == m_stParamInfo.stNeedParam.isPlatformClassTime || nullptr == m_stParamInfo.stNeedParam.getClassTime)
    {
        return NOK;
    }
    if (false == m_stParamInfo.stNeedParam.isPlatformClassTime())
    {
        /* 处理教育云平台教师指导 */
        return NOK;
    }
    /* 本地分析，使用这个逻辑，结束指导行为 */
    if (m_stParamInfo.stNeedParam.isLocalMode)
    {
        if (m_stParamInfo.stNeedParam.isLocalMode())
        {
            /* 处理教育云平台教师指导 */
            teacherPlatformDirecting(false);
        }
    }

    std::lock_guard<std::mutex> lock(m_mtxPlatformTeacher);

    /* 开始教师讲授行为 */
    if (!m_stPlatformTeacherInfo.stBehaviorInfo.listTaughtTime.empty())
    {
        TimeSlotInfo_S& lastBehavior = m_stPlatformTeacherInfo.stBehaviorInfo.listTaughtTime.back();
        if (lastBehavior.nUserParam != 0)
        {
            /* 添加一个新节点 */
            TimeSlotInfo_S stItem;
            stItem.clear();
            stItem.nUserParam = 0;
            stItem.nStart     = m_stParamInfo.stNeedParam.getClassTime();
            stItem.nEnd       = m_stParamInfo.stNeedParam.getClassTime();
            m_stPlatformTeacherInfo.stBehaviorInfo.listTaughtTime.push_back(stItem);
        }
        else
        {
            lastBehavior.nEnd = m_stParamInfo.stNeedParam.getClassTime();
        }
    }
    else
    {
        /* 添加一个新节点 */
        TimeSlotInfo_S stItem;
        stItem.clear();
        stItem.nUserParam = 0;

        if (m_stPlatformTeacherInfo.stBehaviorInfo.listTourTime.empty() &&
            m_stPlatformTeacherInfo.stBehaviorInfo.listInteractionTime.empty())
        {
            stItem.nStart = 0;
        }
        else
        {
            stItem.nStart = m_stParamInfo.stNeedParam.getClassTime();
        }
        stItem.nEnd = m_stParamInfo.stNeedParam.getClassTime();
        m_stPlatformTeacherInfo.stBehaviorInfo.listTaughtTime.push_back(stItem);
    }


    /* 结束巡视动作 */
    if (!m_stPlatformTeacherInfo.stBehaviorInfo.listTourTime.empty())
    {
        TimeSlotInfo_S& lastBehavior = m_stPlatformTeacherInfo.stBehaviorInfo.listTourTime.back();
        if (lastBehavior.nUserParam == 0)
        {
            lastBehavior.nUserParam = 2;
            lastBehavior.nEnd       = m_stParamInfo.stNeedParam.getClassTime();
        }
    }

    /* 结束互动动作 */
    if (!m_stPlatformTeacherInfo.stBehaviorInfo.listInteractionTime.empty())
    {
        TimeSlotInfo_S& lastBehavior = m_stPlatformTeacherInfo.stBehaviorInfo.listInteractionTime.back();
        if (lastBehavior.nUserParam == 0)
        {
            lastBehavior.nUserParam = 3;
            lastBehavior.nEnd       = m_stParamInfo.stNeedParam.getClassTime();
        }
    }

    return OK;
}

/* 开始教授行为 */
BlError_E ResultsModule_NS::CResultsBase::startTaught()
{
    if (nullptr == m_stParamInfo.stNeedParam.getRecordTime ||
        nullptr == m_stParamInfo.stNeedParam.getRecordState)
    {
        return NOK;
    }

    if (m_stParamInfo.stNeedParam.isPlatformClassTime)
    {
        if (m_stParamInfo.stNeedParam.isPlatformClassTime() == true)
        {
            /* 教育云平台开始教授行为 */
            startPlatformTaught();
        }
    }
    
    if (m_stParamInfo.stNeedParam.getRecordState() == 0)
    {
        /* 不用进行分析 */
        return NOK;
    }

    /* 本地分析，使用这个逻辑，结束指导行为 */
    if (m_stParamInfo.stNeedParam.isLocalMode)
    {
        if (m_stParamInfo.stNeedParam.isLocalMode())
        {
            teacherDirecting(false);
        }
    }

    std::lock_guard<std::mutex> lock(m_mtxTeacher);

    dlog(LOG_USER, "开始教师讲授行为：%d", m_stParamInfo.stNeedParam.getRecordTime());
    /* 开始教师讲授行为 */
    if (!m_stTeacherInfo.stBehaviorInfo.listTaughtTime.empty())
    {
        TimeSlotInfo_S& lastBehavior = m_stTeacherInfo.stBehaviorInfo.listTaughtTime.back();
        if (lastBehavior.nUserParam != 0)
        {
            /* 添加一个新节点 */
            TimeSlotInfo_S stItem;
            stItem.clear();
            stItem.nUserParam = 0;
            stItem.nStart     = m_stParamInfo.stNeedParam.getRecordTime();
            stItem.nEnd       = m_stParamInfo.stNeedParam.getRecordTime();
            m_stTeacherInfo.stBehaviorInfo.listTaughtTime.push_back(stItem);
        }
        else
        {
            lastBehavior.nEnd = m_stParamInfo.stNeedParam.getRecordTime();
        }
    }
    else
    {
        /* 添加一个新节点 */
        TimeSlotInfo_S stItem;
        stItem.clear();
        stItem.nUserParam = 0;

        if (m_stTeacherInfo.stBehaviorInfo.listTourTime.empty() &&
            m_stTeacherInfo.stBehaviorInfo.listInteractionTime.empty())
        {
            stItem.nStart = 0;
        }
        else
        {
            stItem.nStart = m_stParamInfo.stNeedParam.getRecordTime();
        }
        stItem.nEnd = m_stParamInfo.stNeedParam.getRecordTime();
        m_stTeacherInfo.stBehaviorInfo.listTaughtTime.push_back(stItem);
    }


    /* 结束巡视动作 */
    if (!m_stTeacherInfo.stBehaviorInfo.listTourTime.empty())
    {
        TimeSlotInfo_S& lastBehavior = m_stTeacherInfo.stBehaviorInfo.listTourTime.back();
        if (lastBehavior.nUserParam == 0)
        {
            lastBehavior.nUserParam = 2;
            lastBehavior.nEnd       = m_stParamInfo.stNeedParam.getRecordTime();
        }
    }

    /* 结束互动动作 */
    if (!m_stTeacherInfo.stBehaviorInfo.listInteractionTime.empty())
    {
        TimeSlotInfo_S& lastBehavior = m_stTeacherInfo.stBehaviorInfo.listInteractionTime.back();
        if (lastBehavior.nUserParam == 0)
        {
            lastBehavior.nUserParam = 3;
            lastBehavior.nEnd       = m_stParamInfo.stNeedParam.getRecordTime();
        }
    }

    return OK;
}

/* 处理教育云平台教师指导 */
BlError_E ResultsModule_NS::CResultsBase::teacherPlatformDirecting(bool bValue)
{
    if (nullptr == m_stParamInfo.stNeedParam.platformBehaviorEvent || nullptr == m_stParamInfo.stNeedParam.isPlatformClassTime || 
    nullptr == m_stParamInfo.stNeedParam.getClassTime)
    {
        return NOK;
    }
    if (false == m_stParamInfo.stNeedParam.isPlatformClassTime())
    {
        return NOK;
    }
    
    
    int nDelayed = m_stParamInfo.stNeedParam.getClassTime() - 10;
    if (nDelayed < 0)
    {
        nDelayed = 0;
    }

    dlog(LOG_INFO, "教师指导 [%d]", bValue);

    std::lock_guard<std::mutex> lock(m_mtxPlatformTeacher);


    if (bValue)
    {
        dlog(LOG_USER, "开始教师指导行为");
        /* 获取最后一个元素的引用并修改其内容 */
        if (!m_stPlatformTeacherInfo.stBehaviorInfo.listDirectingTime.empty())
        {
            TimeSlotInfo_S& lastBehavior = m_stPlatformTeacherInfo.stBehaviorInfo.listDirectingTime.back();
            if (lastBehavior.nUserParam != 0)
            {
                TimeSlotInfo_S stItem;
                stItem.clear();
                stItem.nUserParam = 0;
                stItem.nStart     = nDelayed;
                stItem.nEnd       = m_stParamInfo.stNeedParam.getClassTime();
                m_stPlatformTeacherInfo.stBehaviorInfo.listDirectingTime.push_back(stItem);
                if (bValue)
                {
                    /* 开始教师指导行为 INSTRUCT_STUDENTS-指导学生 TEACHER-老师*/
                    m_stParamInfo.stNeedParam.platformBehaviorEvent(START_BEHAVIOR, INSTRUCT_STUDENTS, TEACHER);
                }
            }
            else
            {
                lastBehavior.nEnd = m_stParamInfo.stNeedParam.getClassTime();
            }
        }
        else
        {
            TimeSlotInfo_S stItem;
            stItem.clear();
            stItem.nUserParam = 0;
            stItem.nStart     = nDelayed;
            stItem.nEnd       = m_stParamInfo.stNeedParam.getClassTime();
            m_stPlatformTeacherInfo.stBehaviorInfo.listDirectingTime.push_back(stItem);
            if (bValue)
            {
                /* 开始教师指导行为 INSTRUCT_STUDENTS-指导学生 TEACHER-老师 */
                m_stParamInfo.stNeedParam.platformBehaviorEvent(START_BEHAVIOR, INSTRUCT_STUDENTS, TEACHER);
            }
        }
    }
    else
    {
        /* 获取最后一个元素的引用并修改其内容 */
        if (!m_stPlatformTeacherInfo.stBehaviorInfo.listDirectingTime.empty())
        {

            TimeSlotInfo_S& lastBehavior = m_stPlatformTeacherInfo.stBehaviorInfo.listDirectingTime.back();
            if (lastBehavior.nUserParam == 0)
            {
                dlog(LOG_USER, "结束教师指导行为");
                lastBehavior.nUserParam = 1;
                lastBehavior.nEnd       = nDelayed;
                /* 结束教师指导行为 INSTRUCT_STUDENTS-指导学生 TEACHER-老师 */
                m_stParamInfo.stNeedParam.platformBehaviorEvent(STOP_BEHAVIOR, INSTRUCT_STUDENTS, TEACHER);
            }
        }
    }

    return OK;
}

/* 教师指导 */
BlError_E ResultsModule_NS::CResultsBase::teacherDirecting(bool bValue)
{
    if (nullptr == m_stParamInfo.stNeedParam.getRecordTime ||
        nullptr == m_stParamInfo.stNeedParam.getRecordState)
    {
        return NOK;
    }

    if (m_stParamInfo.stNeedParam.getRecordState() == 0)
    {
        /* 不用进行分析 */
        return NOK;
    }

    int nDelayed = m_stParamInfo.stNeedParam.getRecordTime() - 10;
    if (nDelayed < 0)
    {
        nDelayed = 0;
    }

    dlog(LOG_INFO, "教师指导 [%d]", bValue);

    std::lock_guard<std::mutex> lock(m_mtxTeacher);


    if (bValue)
    {
        /* 开始教师指导行为 */
        /* 获取最后一个元素的引用并修改其内容 */
        if (!m_stTeacherInfo.stBehaviorInfo.listDirectingTime.empty())
        {
            TimeSlotInfo_S& lastBehavior = m_stTeacherInfo.stBehaviorInfo.listDirectingTime.back();
            if (lastBehavior.nUserParam != 0)
            {
                TimeSlotInfo_S stItem;
                stItem.clear();
                stItem.nUserParam = 0;
                stItem.nStart     = nDelayed;
                stItem.nEnd       = m_stParamInfo.stNeedParam.getRecordTime();
                m_stTeacherInfo.stBehaviorInfo.listDirectingTime.push_back(stItem);
            }
            else
            {
                lastBehavior.nEnd = m_stParamInfo.stNeedParam.getRecordTime();
            }
        }
        else
        {
            TimeSlotInfo_S stItem;
            stItem.clear();
            stItem.nUserParam = 0;
            stItem.nStart     = nDelayed;
            stItem.nEnd       = m_stParamInfo.stNeedParam.getRecordTime();
            m_stTeacherInfo.stBehaviorInfo.listDirectingTime.push_back(stItem);
        }
    }
    else
    {
        /* 结束教师指导行为 */
        /* 获取最后一个元素的引用并修改其内容 */
        if (!m_stTeacherInfo.stBehaviorInfo.listDirectingTime.empty())
        {

            TimeSlotInfo_S& lastBehavior = m_stTeacherInfo.stBehaviorInfo.listDirectingTime.back();
            if (lastBehavior.nUserParam == 0)
            {
                lastBehavior.nUserParam = 1;
                lastBehavior.nEnd       = nDelayed;
            }
        }
    }

    return OK;
}

/* 教育云平台教师板书 */
BlError_E ResultsModule_NS::CResultsBase::teacherPlatformBoard(bool bValue)
{
    if (nullptr == m_stParamInfo.stNeedParam.platformBehaviorEvent || nullptr == m_stParamInfo.stNeedParam.isPlatformClassTime)
    {
        return NOK;
    }
    
    if (false == m_stParamInfo.stNeedParam.isPlatformClassTime())
    {
        return NOK;
    }
    
    dlog(LOG_USER, "教师板书 [%d]", bValue);

    std::lock_guard<std::mutex> lock(m_mtxPlatformTeacher);

    static int s_nStartCount = 0;
    static bool s_bStart = false;

    if (bValue)
    {
        /* 教师板书连续触发次数 */
        s_nStartCount++;
       
        if (m_stParamInfo.stNeedParam.platformBehaviorEvent)
        {
            if (s_nStartCount >= 3)
            {
                dlog(LOG_USER, "上报教师板书行为");
                /* 上报开始板书行为给教育云平台 WRITING_ON_BOARD-板书 TEACHER-老师 */
                m_stParamInfo.stNeedParam.platformBehaviorEvent(START_BEHAVIOR, WRITING_ON_BOARD, TEACHER);
                /*教师板书和教师巡视互斥*/
                m_stParamInfo.stNeedParam.platformBehaviorEvent(STOP_BEHAVIOR, TEACHER_PATROL, TEACHER);
                s_nStartCount = 0;
                s_bStart = true;
            }
        }
          
    }
    else
    {
        /* 教师板书连续触发次数-结束 */
        s_nStartCount = 0;
        if (s_bStart)
        {
            dlog(LOG_USER, "结束教师板书行为");
            /* 上报结束板书行为给教育云平台 WRITING_ON_BOARD-板书 TEACHER-老师  */
            m_stParamInfo.stNeedParam.platformBehaviorEvent(STOP_BEHAVIOR, WRITING_ON_BOARD, TEACHER);
            s_bStart = false;
        }
    }

    return OK;
}

/* 教师板书 */
BlError_E ResultsModule_NS::CResultsBase::teacherBoard(bool bValue)
{
    if (nullptr == m_stParamInfo.stNeedParam.getRecordTime ||
        nullptr == m_stParamInfo.stNeedParam.getRecordState)
    {
        return NOK;
    }

    if (m_stParamInfo.stNeedParam.getRecordState() == 0)
    {
        /* 不用进行分析 */
        return NOK;
    }

    std::lock_guard<std::mutex> lock(m_mtxTeacher);


    if (bValue)
    {
        /* 开始教师板书行为 */
        /* 获取最后一个元素的引用并修改其内容 */
        if (!m_stTeacherInfo.stBehaviorInfo.listBoardTime.empty())
        {
            TimeSlotInfo_S& lastBehavior = m_stTeacherInfo.stBehaviorInfo.listBoardTime.back();
            if (lastBehavior.nUserParam != 0)
            {
                TimeSlotInfo_S stItem;
                stItem.clear();
                stItem.nUserParam = 0;
                stItem.nStart     = m_stParamInfo.stNeedParam.getRecordTime();
                stItem.nEnd       = m_stParamInfo.stNeedParam.getRecordTime();
                m_stTeacherInfo.stBehaviorInfo.listBoardTime.push_back(stItem);
            }
            else
            {
                lastBehavior.nEnd = m_stParamInfo.stNeedParam.getRecordTime();
            }
        }
        else
        {
            TimeSlotInfo_S stItem;
            stItem.clear();
            stItem.nUserParam = 0;
            stItem.nStart     = m_stParamInfo.stNeedParam.getRecordTime();
            stItem.nEnd       = m_stParamInfo.stNeedParam.getRecordTime();
            m_stTeacherInfo.stBehaviorInfo.listBoardTime.push_back(stItem);
        }
    }
    else
    {
        /* 结束教师板书行为 */
        /* 获取最后一个元素的引用并修改其内容 */
        if (!m_stTeacherInfo.stBehaviorInfo.listBoardTime.empty())
        {

            TimeSlotInfo_S& lastBehavior = m_stTeacherInfo.stBehaviorInfo.listBoardTime.back();
            if (lastBehavior.nUserParam == 0)
            {
                lastBehavior.nUserParam = 1;
                lastBehavior.nEnd       = m_stParamInfo.stNeedParam.getRecordTime();
            }
        }
    }

    return OK;
}

/* PPT切换 */
BlError_E ResultsModule_NS::CResultsBase::pptSwitch()
{
    if (nullptr == m_stParamInfo.stNeedParam.getRecordState)
    {
        return NOK;
    }

    /* 获取当前时间戳 */
    auto now       = std::chrono::system_clock::now();
    /* 将时间戳转换为秒数 */
    auto timestamp = std::chrono::duration_cast<std::chrono::seconds>(now.time_since_epoch()).count();
    if (m_stParamInfo.stNeedParam.isPlatformClassTime)
    {
        if (m_stParamInfo.stNeedParam.isPlatformClassTime() == true)
        {
            std::lock_guard<std::mutex> lock1(m_mtxPlatformClassSummary);
            if (m_stPlatformClassSummaryInfo.listPPTSwitchTime.back() < timestamp)
            {
                dlog(LOG_USER, "PPT切换 %ld", timestamp);
                m_stPlatformClassSummaryInfo.listPPTSwitchTime.push_back(timestamp);
            }
        }
    }
    
    if (m_stParamInfo.stNeedParam.getRecordState() == 0)
    {
        /* 不用进行分析 */
        return NOK;
    }

    std::lock_guard<std::mutex> lock2(m_mtxClassSummary);

    if (m_stClassSummaryInfo.listPPTSwitchTime.back() < timestamp)
    {
        dlog(LOG_USER, "PPT切换 %ld", timestamp);
        m_stClassSummaryInfo.listPPTSwitchTime.push_back(timestamp);
    }

    return OK;
}

/* 添加提取的热词 */
BlError_E ResultsModule_NS::CResultsBase::addAudioWordsResult(
    std::list<std::pair<std::string, int>> listWords)
{
    if (nullptr == m_stParamInfo.stNeedParam.getRecordState)
    {
        return NOK;
    }

    std::lock_guard<std::mutex> lock1(m_mtxHotwordExtInfo);

    m_stHotwordExtInfo.clear();
    for (auto item : listWords)
    {
        WordInfo_S stTmp;
        stTmp.clear();
        stTmp.strWord = item.first;
        stTmp.nCount  = item.second;
        m_stHotwordExtInfo.listWordInfo.push_back(stTmp);
    }
    return OK;
}

/* 设置本地老师人脸识别-结果 */
BlError_E ResultsModule_NS::CResultsBase::setLocalTeFaceRecResult(
    std::list<std::pair<int, std::string>> listName)
{
    if (nullptr == m_stParamInfo.stNeedParam.getRecordState)
    {
        return NOK;
    }

    std::lock_guard<std::mutex> lock1(m_mtxPlatformAttendance);

    auto now = std::chrono::system_clock::now();

    /* 将时间点转换为time_t，以便获取秒数 */
    std::time_t stNowTime = std::chrono::system_clock::to_time_t(now);

    for (auto item : listName)
    {

        bool bFind = false;
        for (auto it = m_stPlatformAttendanceInfo.mapTeacherInfo.begin();
             it != m_stPlatformAttendanceInfo.mapTeacherInfo.end();)
        {
            /* 判断 info.strName 是否为特定值 */
            if (it->second.strName == item.second)
            {
                bFind = true;

                it->second.nLastTime = stNowTime;
                it->second.nNumber++;

                break;
            }
            else
            {
                /* 继续遍历 */
                ++it;
            }
        }

        if (!bFind)
        {
            /* 添加 */
            TeAttendanceInfo_S stTmp;
            stTmp.clear();
            stTmp.nId        = item.first;
            stTmp.strName    = item.second;
            stTmp.nFirstTime = stNowTime;
            stTmp.nLastTime  = stNowTime;
            stTmp.nNumber    = 1;

            /* 如果没找到，添加数据 */
            m_stPlatformAttendanceInfo.mapTeacherInfo[stTmp.nId] = stTmp;
        }
    }

    if (m_stParamInfo.stNeedParam.getRecordState() == 0)
    {
        /* 不用进行分析 */
        return NOK;
    }

    std::lock_guard<std::mutex> lock2(m_mtxAttendance);

    for (auto item : listName)
    {

        bool bFind = false;
        for (auto it = m_stAttendanceInfo.mapTeacherInfo.begin();
             it != m_stAttendanceInfo.mapTeacherInfo.end();)
        {
            /* 判断 info.strName 是否为特定值 */
            if (it->second.strName == item.second)
            {
                bFind = true;

                it->second.nLastTime = stNowTime;
                it->second.nNumber++;

                break;
            }
            else
            {
                /* 继续遍历 */
                ++it;
            }
        }

        if (!bFind)
        {
            /* 添加 */
            TeAttendanceInfo_S stTmp;
            stTmp.clear();
            stTmp.nId        = item.first;
            stTmp.strName    = item.second;
            stTmp.nFirstTime = stNowTime;
            stTmp.nLastTime  = stNowTime;
            stTmp.nNumber    = 1;

            /* 如果没找到，添加数据 */
            m_stAttendanceInfo.mapTeacherInfo[stTmp.nId] = stTmp;
        }
    }

    return OK;
}

/* 获取当前学生考勤人数 */
int ResultsModule_NS::CResultsBase::getStAttendanceSize()
{
    std::lock_guard<std::mutex> lock(m_mtxClassSummary);
    return m_stAttendanceInfo.mapStudentInfo.size();
}

/* 获取当前老师考勤人数 */
int ResultsModule_NS::CResultsBase::getTeAttendanceSize()
{
    std::lock_guard<std::mutex> lock(m_mtxClassSummary);
    return m_stAttendanceInfo.mapTeacherInfo.size();
}

/* 获取当前识别到的人数 */
int ResultsModule_NS::CResultsBase::getCurPeopleSize()
{
    std::lock_guard<std::mutex> lock(m_mtxStudent);
    return m_nCurPeopleNum;
}

/* 结束教育云平台AI分析 */
BlError_E ResultsModule_NS::CResultsBase::end_platformAiAnalysis(const void* pParam)
{
    /* 清空数据 */
    m_stPlatformTeacherInfo.clear();
    m_stPlatformStudentInfo.clear();
    m_stPlatformClassSummaryInfo.clear();
    m_stPlatformAttendanceInfo.clear();

    return OK;
}

/* 下课处理函数 */
BlError_E ResultsModule_NS::CResultsBase::processClassExit(const void* pParam)
{
    if (nullptr == m_stParamInfo.stNeedParam.platformBehaviorEvent)
    {
        return NOK;
    }
    
    beginDeal_PlatformStudentInfo();
    beginDeal_platformTeacherInfo();
    beginDeal_platformClassSummaryInfo();
    m_stPfCurSampleBoxInfo.clear();
    m_stPfLastSampleBoxInfo.clear();

    /* 通知结束行为, 一共12种行为 */
    for (int i = LISTEN_TO_TALK; i <= TEACHER_PATROL; i++)
    {
        /* 结束学生行为 */
        if (i <= DISCUSSION)
        {
            m_stParamInfo.stNeedParam.platformBehaviorEvent(STOP_BEHAVIOR, i, STUDENT);
        }

        /* 结束老师行为 */
        if (i >= TEACH_BY_TEACHER)
        {
            m_stParamInfo.stNeedParam.platformBehaviorEvent(STOP_BEHAVIOR, i, TEACHER);
        }
    }

    return OK;
}

/* 开始处理教育云平台教师信息 */
BlError_E ResultsModule_NS::CResultsBase::beginDeal_platformTeacherInfo()
{
    if (nullptr == m_stParamInfo.stNeedParam.getClassTime)
    {
        return NOK;
    }
    
    std::lock_guard<std::mutex> lock(m_mtxPlatformTeacher);

    /* 结束教学行为 */
    if (m_stPlatformTeacherInfo.stBehaviorInfo.listTaughtTime.size() > 0)
    {
        TimeSlotInfo_S& lastBehavior1 = m_stPlatformTeacherInfo.stBehaviorInfo.listTaughtTime.back();
        if (lastBehavior1.nUserParam == 0)
        {
            lastBehavior1.nUserParam = 1;
            lastBehavior1.nEnd       = m_stParamInfo.stNeedParam.getClassTime();
        }
    }

    if (m_stPlatformTeacherInfo.stBehaviorInfo.listTourTime.size() > 0)
    {
        TimeSlotInfo_S& lastBehavior2 = m_stPlatformTeacherInfo.stBehaviorInfo.listTourTime.back();
        if (lastBehavior2.nUserParam == 0)
        {
            lastBehavior2.nUserParam = 2;
            lastBehavior2.nEnd       = m_stParamInfo.stNeedParam.getClassTime();
        }
    }

    if (m_stPlatformTeacherInfo.stBehaviorInfo.listInteractionTime.size() > 0)
    {
        TimeSlotInfo_S& lastBehavior3 = m_stPlatformTeacherInfo.stBehaviorInfo.listInteractionTime.back();
        if (lastBehavior3.nUserParam == 0)
        {
            lastBehavior3.nUserParam = 3;
            lastBehavior3.nEnd       = m_stParamInfo.stNeedParam.getClassTime();
        }
    }


    /* 计算讲授时长 */
    for (auto item : m_stPlatformTeacherInfo.stBehaviorInfo.listTaughtTime)
    {
        int nTeme = item.nEnd - item.nStart;
        if (nTeme > 0)
        {
            m_stPlatformTeacherInfo.nTaughtTime += nTeme;
        }
    }

    /* 计算互动时长 */
    for (auto item : m_stPlatformTeacherInfo.stBehaviorInfo.listInteractionTime)
    {
        int nTeme = item.nEnd - item.nStart;
        if (nTeme > 0)
        {
            m_stPlatformTeacherInfo.nInteractionTime += nTeme;
        }
    }

    /* 计算指导时长 */
    for (auto item : m_stPlatformTeacherInfo.stBehaviorInfo.listDirectingTime)
    {
        int nTeme = item.nEnd - item.nStart;
        if (nTeme > 0)
        {
            m_stPlatformTeacherInfo.nDirectingTime += nTeme;
        }
    }

    /* 计算板书时长 */
    for (auto item : m_stPlatformTeacherInfo.stBehaviorInfo.listBoardTime)
    {
        int nTeme = item.nEnd - item.nStart;
        if (nTeme > 0)
        {
            m_stPlatformTeacherInfo.nBoardTime += nTeme;
        }
    }


/* 计算讲台时长 */
#if 1
    m_stPlatformTeacherInfo.nPodiumTime = 0;

    /* 讲台时长 = 课堂时长 - 巡视时长 */
    m_stPlatformTeacherInfo.nPodiumTime = m_stParamInfo.stNeedParam.getClassTime() - m_stPlatformTeacherInfo.nTourTime;

    if (m_stPlatformTeacherInfo.nPodiumTime <= 0)
    {
        /* 讲台时长 = 讲授时长 */
        m_stPlatformTeacherInfo.nPodiumTime = m_stPlatformTeacherInfo.nTaughtTime;
    }
#endif

/* 计算巡视时长 */
#if 1
    for (auto item : m_stPlatformTeacherInfo.stBehaviorInfo.listTourTime)
    {
        int nTeme = item.nEnd - item.nStart;
        if (nTeme > 0)
        {
            m_stPlatformTeacherInfo.nTourTime += nTeme;
        }
    }
#endif

/* 计算巡视次数 */
#if 1
    /* 当前时间块的开始时间和前一个时间块的结束时间之间的差异小于3, 不算一次巡视 */
    /* 初始化前一个时间块的结束时间 */
    const TimeSlotInfo_S* prevTimeSlot = &m_stPlatformTeacherInfo.stBehaviorInfo.listTourTime.front();

    m_stPlatformTeacherInfo.nTourNumber = 0;
    /* 遍历时间块列表 */
    for (const TimeSlotInfo_S& currentTimeSlot : m_stPlatformTeacherInfo.stBehaviorInfo.listTourTime)
    {
        /* 计算当前时间块的开始时间和前一个时间块的结束时间之间的差异 */
        int nDiff = currentTimeSlot.nStart - prevTimeSlot->nEnd;

        /* 如果差异大于3 */
        if (std::abs(nDiff) > 3)
        {
            m_stPlatformTeacherInfo.nTourNumber++;
        }
    }
    if (m_stPlatformTeacherInfo.nTourNumber == 0)
    {
        m_stPlatformTeacherInfo.nTourNumber = 1;
    }
#endif

    return OK;
}

/* 开始处理教育云课堂信息 */
BlError_E ResultsModule_NS::CResultsBase::beginDeal_platformClassSummaryInfo()
{
    std::lock_guard<std::mutex> lock(m_mtxPlatformTeacher);

    if (m_stParamInfo.stNeedParam.platformPushAlertType == nullptr || m_stParamInfo.stNeedParam.getPlatformSwitch == nullptr)
    {
        return NOK;
    }

    /* 计算老师授课时间，百分数 和 学生活动时间，百分数 */
    int   nTotalTime = m_stPlatformTeacherInfo.nTaughtTime +
        m_stPlatformTeacherInfo.nInteractionTime +
        m_stPlatformTeacherInfo.nDirectingTime +
        m_stPlatformTeacherInfo.nTourTime;

    if (nTotalTime > 0)
    {
        m_stPlatformClassSummaryInfo.nTeacherTime = m_stPlatformTeacherInfo.nTaughtTime * 100.0 / nTotalTime;
        m_stPlatformClassSummaryInfo.nStudentTime = 100 - m_stPlatformClassSummaryInfo.nTeacherTime;
    }

    int nParticipationRate = 0;
    /* 计算参与率 教师指导+教师互动+教师指导的占比 */
    if (nTotalTime > 0)
    {
        nParticipationRate = (m_stPlatformTeacherInfo.nInteractionTime +
                              m_stPlatformTeacherInfo.nDirectingTime +
                              m_stPlatformTeacherInfo.nTourTime) *
            100.0 / nTotalTime;
    }

    if (1 == m_stParamInfo.stNeedParam.getPlatformSwitch(STUDENT_LOW_CLASS_PARTICIPATION))
    {
        if (nParticipationRate > 70)
        {
            /* 上报学生课堂参与度 STUDENT_LOW_CLASS_PARTICIPATION-学生课堂参与度*/
            m_stParamInfo.stNeedParam.platformPushAlertType(STUDENT_LOW_CLASS_PARTICIPATION, "学生参与课堂活动积极度较高");
        }
        else if (nParticipationRate > 40)
        {
            /* 上报学生课堂参与度 STUDENT_LOW_CLASS_PARTICIPATION-学生课堂参与度*/
            m_stParamInfo.stNeedParam.platformPushAlertType(STUDENT_LOW_CLASS_PARTICIPATION, "学生参与课堂活动积极度中等");
        }
        else
        {
            /* 上报学生课堂参与度 STUDENT_LOW_CLASS_PARTICIPATION-学生课堂参与度*/
            m_stParamInfo.stNeedParam.platformPushAlertType(STUDENT_LOW_CLASS_PARTICIPATION, "学生参与课堂活动积极度较差");
        }
    }

    return OK;
}

/* 开始处理-课堂信息 */
BlError_E ResultsModule_NS::CResultsBase::beginDeal_classSummaryInfo()
{
    std::lock_guard<std::mutex> lock1(m_mtxStudent);
    std::lock_guard<std::mutex> lock2(m_mtxTeacher);
    std::lock_guard<std::mutex> lock3(m_mtxClassSummary);

    /* 计算课堂时长 */
    if (m_stParamInfo.stNeedParam.getRecordTime)
    {
        m_stClassSummaryInfo.nClassTime = m_stParamInfo.stNeedParam.getRecordTime();
    }


    /* 计算专注度均值 = 计算课堂得分 */
    int nTotalScore = 0;
    for (auto item : m_stStudentInfo.stFocusScoreInfo.listScore)
    {
        nTotalScore += item;
    }

    if (!m_stStudentInfo.stFocusScoreInfo.listScore.empty())
    {
        m_stClassSummaryInfo.nClassScore = nTotalScore / m_stStudentInfo.stFocusScoreInfo.listScore.size();
    }

    /* 计算老师授课时间，百分数 和 学生活动时间，百分数 */
    int nTotalTime = m_stTeacherInfo.nTaughtTime +
        m_stTeacherInfo.nInteractionTime +
        m_stTeacherInfo.nDirectingTime +
        m_stTeacherInfo.nTourTime;

    if (nTotalTime > 0)
    {
        m_stClassSummaryInfo.nTeacherTime = m_stTeacherInfo.nTaughtTime * 100.0 / nTotalTime;
        m_stClassSummaryInfo.nStudentTime = 100 - m_stClassSummaryInfo.nTeacherTime;
    }

    /* 计算参与率 教师指导+教师互动的占比 */
    if (nTotalTime > 0)
    {
        m_stClassSummaryInfo.nParticipationRate = (m_stTeacherInfo.nInteractionTime +
                                                   m_stTeacherInfo.nDirectingTime +
                                                   m_stTeacherInfo.nTourTime) *
            100.0 / nTotalTime;
    }

    /* 计算PPT翻页间隔 */
    if (!m_stClassSummaryInfo.listPPTSwitchTime.empty())
    {
        m_stClassSummaryInfo.nPPTInterval = ((m_stClassSummaryInfo.listPPTSwitchTime.back() -
                                              m_stClassSummaryInfo.listPPTSwitchTime.front()) /
                                             m_stClassSummaryInfo.listPPTSwitchTime.size());
    }

    /* 计算教师讲授时间占比、师生互动时间占比、学生学习时间占比 */
    if (nTotalTime > 0)
    {
        m_stClassSummaryInfo.nTaughtPct      = m_stTeacherInfo.nTaughtTime * 100.0 / nTotalTime;
        m_stClassSummaryInfo.nInteractionPct = m_stTeacherInfo.nInteractionTime * 100.0 / nTotalTime;
        m_stClassSummaryInfo.nStudyPct       = 100 - m_stClassSummaryInfo.nTaughtPct - m_stClassSummaryInfo.nInteractionPct;
    }

    int nTotal = m_stStudentInfo.nListenTotal +
        m_stStudentInfo.nPracticeTotal +
        m_stStudentInfo.nDemonstrateTotal +
        m_stStudentInfo.nReadTotal +
        m_stStudentInfo.nDiscussTotal +
        m_stStudentInfo.nDownDeskTotal;

    if (nTotal > 0)
    {
        /* 计算抬头率 */
        // m_stClassSummaryInfo.nHeadUpRate   = (m_stStudentInfo.nListenTotal * 100.0) / nTotal;
        m_stClassSummaryInfo.nHeadUpRate   = m_stStudentInfo.nListenTime * 100.0 / m_stClassSummaryInfo.nClassTime;
        m_stClassSummaryInfo.nHeadUpRate   = std::min(m_stClassSummaryInfo.nHeadUpRate, 100);
        /* 计算低头率 */
        // m_stClassSummaryInfo.nHeadDownRate = (m_stStudentInfo.nReadTotal * 100.0) / nTotal;
        m_stClassSummaryInfo.nHeadDownRate = m_stStudentInfo.nReadTime * 100.0 / m_stClassSummaryInfo.nClassTime;
        m_stClassSummaryInfo.nHeadDownRate = std::min(m_stClassSummaryInfo.nHeadDownRate, 100);
    }


    /* 计算RT-CH曲线 */
    int nTotalRC = m_stTeacherInfo.nTaughtTime +
        m_stTeacherInfo.nInteractionTime +
        m_stTeacherInfo.nDirectingTime;
    if (nTotalRC > 0)
    {
        m_stClassSummaryInfo.fRtChX = (m_stTeacherInfo.nTaughtTime * 1.0 + m_stTeacherInfo.nInteractionTime * 0.5) / nTotalRC;
        m_stClassSummaryInfo.fRtChY = m_stClassSummaryInfo.nStTeCutNum * 1.0 / nTotalRC;

        /* 确保坐标点在三角形内 */
        clampPointToTriangle(m_stClassSummaryInfo.fRtChX, m_stClassSummaryInfo.fRtChY);
    }

    /* 计算ST曲线 */
    int                       nStT = 0;
    int                       nStS = 0;
    std::list<TimeSlotInfo_S> listSTTime;
    std::list<TimeSlotInfo_S> listSTTime1 = m_stTeacherInfo.stBehaviorInfo.listTaughtTime;
    std::list<TimeSlotInfo_S> listSTTime2 = m_stTeacherInfo.stBehaviorInfo.listTourTime;
    std::list<TimeSlotInfo_S> listSTTime3 = m_stTeacherInfo.stBehaviorInfo.listInteractionTime;
    listSTTime.merge(listSTTime1, compareTimeSlotInfo);
    listSTTime.merge(listSTTime2, compareTimeSlotInfo);
    listSTTime.merge(listSTTime3, compareTimeSlotInfo);

    for (auto item : listSTTime)
    {
        item.print();
        if (item.nUserParam == 1)
        {
            int i = (item.nEnd - item.nStart) / m_stClassSummaryInfo.nStInterval;
            if (i <= 0)
            {
                i = 1;
            }
            for (; i > 0; i--)
            {
                CoordInfo_S stTemp;
                stTemp.nX = nStT;
                stTemp.nY = nStS;
                m_stClassSummaryInfo.listSTInfo.push_back(stTemp);

                nStT++;
            }
        }
        else if (item.nUserParam == 2)
        {
            int i = (item.nEnd - item.nStart) / m_stClassSummaryInfo.nStInterval;
            if (i <= 0)
            {
                i = 1;
            }
            for (; i > 0; i--)
            {
                nStS++;
            }
        }
        else if (item.nUserParam == 3)
        {
            int i = (item.nEnd - item.nStart) / m_stClassSummaryInfo.nStInterval;
            if (i <= 0)
            {
                i = 1;
            }
            for (; i > 0; i--)
            {
                nStS++;
            }
        }
    }

    if (m_stClassSummaryInfo.listSTInfo.empty())
    {
        CoordInfo_S stTemp;
        stTemp.nX = nStT;
        stTemp.nY = nStS;
        m_stClassSummaryInfo.listSTInfo.push_back(stTemp);
    }
    else
    {
        if (m_stClassSummaryInfo.listSTInfo.back().nX != nStT ||
            m_stClassSummaryInfo.listSTInfo.back().nY != nStS)
        {
            CoordInfo_S stTemp;
            stTemp.nX = nStT;
            stTemp.nY = nStS;
            m_stClassSummaryInfo.listSTInfo.push_back(stTemp);
        }
    }



    /* 计算提问时间 */
    m_stClassSummaryInfo.listQuizTime = m_stTeacherInfo.stBehaviorInfo.listInteractionTime;

    /* 计算互动时间点 */
    m_stClassSummaryInfo.listInteractionTime = m_stTeacherInfo.stBehaviorInfo.listDirectingTime;


    std::string strEmoSuggest = "";

    /* 计算AI课堂建议 */
    std::string strProposal   = "建议：\n";
    std::string strClassScore = "该课堂";
    if (m_stClassSummaryInfo.nClassScore > 90)
    {
        strClassScore += "教学成效卓越，";
        strProposal   += "1.教师需要思考教学方法并进行改进。\n";
    }
    else if (m_stClassSummaryInfo.nClassScore > 60)
    {
        strClassScore += "教学效果总体积极，";
        strProposal   += "1.定期回顾和更新教学材料，确保内容的时效性和吸引力。\n";
    }
    else
    {
        strClassScore += "教学效果不理想，";
        strProposal   += "1.考虑将成功的教学实践和案例分享给其他教师，以促进教学方法的交流和提升。\n";
    }

    std::string strClassTime = "时间分配";
    if (m_stClassSummaryInfo.nTeacherTime > 75)
    {
        strClassTime += "主要为教师讲解为主，";
        strProposal  += "2.考虑平衡讲授与互动时间，增加学生参与度，以促进学生的批判性思维和深入理解。\n";
    }
    else if (m_stClassSummaryInfo.nTeacherTime > 45)
    {
        strClassTime += "主要以教师和学生互动为主，";
        strProposal  += "2.注意根据课程内容和学生的学习需求灵活调整讲授与互动的时间比例。\n";
    }
    else
    {
        strClassTime += "主要以学生活动为主，";
        strProposal  += "2.定期检查学生的学习进度和理解程度，确保教学目标得到实现。\n";
    }

    std::string strParticipationRate = "";
    if (m_stClassSummaryInfo.nParticipationRate > 70)
    {
        strParticipationRate += "学生参与课堂活动积极度较高，";
        strProposal          += "3.继续保持和加强现有的教学策略，利用学生的高参与度来进一步深化他们的理解和分析能力。\n";
    }
    else if (m_stClassSummaryInfo.nParticipationRate > 40)
    {
        strParticipationRate += "学生参与课堂活动积极度中等，";
        strProposal          += "3.识别并了解参与度较低学生的具体原因，是否由于学习难度、兴趣不足或其他个人因素。\n";
    }
    else
    {
        strParticipationRate += "学生参与课堂活动积极度较差，";
        strProposal          += "3.重新审视和调整教学计划和课堂活动，确保教学内容与学生的兴趣和需求相匹配。\n";
    }

    nTotal = m_stStudentInfo.nAngerTotal +
        m_stStudentInfo.nDisgustTotal +
        m_stStudentInfo.nFearTotal +
        m_stStudentInfo.nJoyTotal +
        m_stStudentInfo.nNeutralTotal +
        m_stStudentInfo.nSadnessTotal +
        m_stStudentInfo.nSurpriseTotal;

    int nEmotionRate1 = 0;
    int nEmotionRate2 = 0;
    int nEmotionRate3 = 0;

    if (nTotal > 0)
    {
        nEmotionRate1 = (m_stStudentInfo.nJoyTotal + m_stStudentInfo.nSurpriseTotal) * 100.0 / nTotal;
        nEmotionRate2 = (m_stStudentInfo.nAngerTotal + m_stStudentInfo.nDisgustTotal + m_stStudentInfo.nFearTotal + m_stStudentInfo.nSadnessTotal) * 100.0 / nTotal;
        nEmotionRate3 = (m_stStudentInfo.nNeutralTotal) * 100.0 / nTotal;
    }

    std::string strEmotion = "";
    if (nEmotionRate1 > 65)
    {
        strEmotion    += "学生主要情绪为积极、快乐，课堂气氛活泼，";
        strProposal   += "4.继续保持和强化那些导致积极情绪的教学策略和课堂管理方法。\n";
        strEmoSuggest += "建议继续保持和强化那些导致积极情绪的教学策略和课堂管理方法。";
    }
    else if (nEmotionRate2 > 65)
    {
        strEmotion    += "学生主要情绪为负面情绪，课堂气氛压抑，";
        strProposal   += "4.紧急审视和调整可能导致学生负面情绪的教学内容和方法，确保课程难度适中，且能够满足学生的需求。\n";
        strEmoSuggest += "建议紧急审视和调整可能导致学生负面情绪的教学内容和方法，确保课程难度适中，且能够满足学生的需求。";
    }
    else if (nEmotionRate3 > 65)
    {
        strEmotion    += "学生情绪平缓，课堂气氛平和，";
        strProposal   += "4.分析课堂活动和教学方法，确定是否需要引入更多互动和参与性强的元素来激发学生的兴趣和情感。\n";
        strEmoSuggest += "建议分析课堂活动和教学方法，确定是否需要引入更多互动和参与性强的元素来激发学生的兴趣和情感。";
    }

    std::string strFocusScore = "";
    if (m_stStudentInfo.stFocusScoreInfo.nAverageScore > 70)
    {
        strFocusScore += "学生在课堂上表现出高度的注意力集中。\n";
        strProposal   += "5.继续保持和强化当前有效的教学策略的互动性、实践性和相关性，以维持学生的高专注度。\n";
    }
    else if (m_stStudentInfo.stFocusScoreInfo.nAverageScore > 60)
    {
        strFocusScore += "学生在课堂上的注意力集中程度处于中等水平。\n";
        strProposal   += "5.分析当前的教学策略，识别哪些方面有效，哪些需要改进，以进一步提升学生的专注度。\n";
    }
    else
    {
        strFocusScore += "学生在课堂上的注意力集中程度较低。\n";
        strProposal   += "5.尝试改变教学方法，例如引入更多互动和实践活动，以提高学生的参与度和兴趣。\n";
    }
    m_stClassSummaryInfo.strAiClassAdviceSumUp = strClassScore +
        strClassTime +
        strParticipationRate +
        strEmotion +
        strFocusScore +
        strProposal;

    /* 计算课堂类型分析结论 / 计算课堂总体评价 */
    if (m_stClassSummaryInfo.fRtChX <= 0.3)
    {
        /* 练习型 */
        m_stClassSummaryInfo.strClassConclusionSumUp = "该堂课以学生独立完成作业或练习为主，教师的角色更多是指导和辅助。这种课堂类型适合于注重学生的实践操作，通过大量练习帮助学生巩固知识和技能。";
        m_stClassSummaryInfo.strClassEvaluateSumUp   = "该堂课程为练习型课堂，重点侧重于通过实际操作和练习来加深学生对知识点的理解和掌握。教师的引导和学生的实践时间分配较为均衡，建议更加关注技能的应用和实践，合理分配理论讲解与实际操作的时间，并关注学生的练习效率。在学生精力充沛的时段内，安排关键技能的演练和应用，通过即时反馈和指导，帮助学生巩固和提升所学技能。";
    }
    else if (m_stClassSummaryInfo.fRtChX >= 0.7)
    {
        /* 讲授型 */
        m_stClassSummaryInfo.strClassConclusionSumUp = "本节课以教师的讲解为主，学生的主要任务是听讲和记笔记。这种课堂类型适合于传递大量信息和理论知识。";
        m_stClassSummaryInfo.strClassEvaluateSumUp   = "该堂课程为讲授型课堂，主要以教师的系统讲解为主，强调对知识体系的全面介绍和深入阐释。教师的讲授占据了课堂的大部分时间，建议在讲授过程中穿插关键问题和思考点，以激发学生的思考和参与。同时，应注意学生的接受能力和记忆曲线，在学生注意力较为集中的时段内，重点讲解和复习核心概念和难点内容，以提高学生对重点知识的吸收和记忆效率。";
    }
    else if (m_stClassSummaryInfo.fRtChY >= 0.4)
    {
        /* 对话型 */
        m_stClassSummaryInfo.strClassConclusionSumUp = "该堂课师生互动较为明显，师生互动频繁，学生有较多机会表达观点和交流。课堂时间占比较为均匀，学生能够时学时练。";
        m_stClassSummaryInfo.strClassEvaluateSumUp   = "该堂课程为对话型课堂，重点侧重与学生互动，教师的讲授时间和学生的互动时间分配较为合理，建议更加关注重点知识的深度挖掘，合理分配重点知识点的讲解时间和练习时间，并关注学生思维集中时间，在集中时间段内进行重点知识的讲解和分析，帮助学生提高知识吸收效率。";
    }
    else
    {
        /* 混合型 */
        m_stClassSummaryInfo.strClassConclusionSumUp = "本节课结合了讲授和练习，教师讲授与学生参与之间的比例相对平衡。这种课堂类型旨在通过讲授传递知识，同时通过互动活动增强学生的理解和应用能力。";
        m_stClassSummaryInfo.strClassEvaluateSumUp   = "该堂课程为混合型课堂，结合了讲授、互动和练习等多种教学方式，旨在全面提升学生的学习体验和知识掌握程度。教师的讲解、学生的互动讨论和独立练习时间分配得当，建议进一步优化教学内容的结构，确保每个环节都能有效地支持学习目标。特别是在学生注意力集中的时段，可以安排综合性的案例分析或问题解决活动，以促进学生的批判性思维和创新能力的发展。";
    }

    /* 教师教学报告（单节课）重点学情回顾 学生专注度 */
    std::string strFocusScoreSumUp   = "";
    std::string strFocusScoreSuggest = "";
    if (m_stStudentInfo.stFocusScoreInfo.nAverageScore > 70)
    {
        strFocusScoreSumUp   += "该堂课程学生整体专注度很高";
        strFocusScoreSuggest += "建议教师继续维持这种互动性和启发性的教学风格，深入分析课程视频，识别并强化那些特别促进学生集中注意力的教学环节。";
    }
    else if (m_stStudentInfo.stFocusScoreInfo.nAverageScore > 60)
    {
        strFocusScoreSumUp   += "该堂课程学生整体专注度较高";
        strFocusScoreSuggest += "建议教师继续保持并优化现有教学策略，分析课程视频以提炼有效教学元素，进一步强化学生专注的环节。";
    }
    else
    {
        strFocusScoreSumUp   += "该堂课程学生整体专注度较低";
        strFocusScoreSuggest += "建议教师重新审视教学计划和教学方案，回看相关课程视频，找到学生专注和散漫原因。";
    }

    /* 专注时间 */
    if (m_stStudentInfo.nConcentrationStageStart == 0 && m_stStudentInfo.nConcentrationStageEnd == 0)
    {
        strFocusScoreSumUp += "学生专注时间未发生于本堂课,";
    }
    else if (m_stStudentInfo.nConcentrationStageStart < 60 && m_stStudentInfo.nConcentrationStageEnd < 60)
    {
        strFocusScoreSumUp += "学生专注时间发生于[" +
            std::to_string(m_stStudentInfo.nConcentrationStageStart) + "sec~" +
            std::to_string(m_stStudentInfo.nConcentrationStageEnd) + "sec]期间，平均专注度为" +
            std::to_string(m_stStudentInfo.nConcentrationStageAverageScore) + ",持续时间为" +
            std::to_string((m_stStudentInfo.nConcentrationStageEnd - m_stStudentInfo.nConcentrationStageStart)) + "sec,";
    }
    else
    {
        strFocusScoreSumUp += "学生专注时间发生于[" +
            std::to_string(m_stStudentInfo.nConcentrationStageStart / 60) + "min~" +
            std::to_string(m_stStudentInfo.nConcentrationStageEnd / 60) + "min]期间，平均专注度为" +
            std::to_string(m_stStudentInfo.nConcentrationStageAverageScore) + ",持续时间为" +
            std::to_string((m_stStudentInfo.nConcentrationStageEnd - m_stStudentInfo.nConcentrationStageStart) / 60) + "min,";
    }

    /* 散漫时间 */
    if (m_stStudentInfo.nDistractionStageStart == 0 && m_stStudentInfo.nDistractionStageEnd == 0)
    {
        strFocusScoreSumUp += "学生散漫时间未发生于本堂课,";
    }
    else if (m_stStudentInfo.nDistractionStageStart < 60 && m_stStudentInfo.nDistractionStageEnd < 60)
    {
        strFocusScoreSumUp += "学生散漫时间发生于[" +
            std::to_string(m_stStudentInfo.nDistractionStageStart) + "sec~" +
            std::to_string(m_stStudentInfo.nDistractionStageEnd) + "sec]期间，平均专注度为" +
            std::to_string(m_stStudentInfo.nDistractionStageAverageScore) + ",持续时间为" +
            std::to_string((m_stStudentInfo.nDistractionStageEnd - m_stStudentInfo.nDistractionStageStart)) + "sec,";
    }
    else
    {
        strFocusScoreSumUp += "学生散漫时间发生于[" +
            std::to_string(m_stStudentInfo.nDistractionStageStart / 60) + "min~" +
            std::to_string(m_stStudentInfo.nDistractionStageEnd / 60) + "min]期间，平均专注度为" +
            std::to_string(m_stStudentInfo.nDistractionStageAverageScore) + ",持续时间为" +
            std::to_string((m_stStudentInfo.nDistractionStageEnd - m_stStudentInfo.nDistractionStageStart) / 60) + "min,";
    }



    m_stClassSummaryInfo.strFocusScoreSumUp = strFocusScoreSumUp + strFocusScoreSuggest;

    /* 教师教学报告（单节课）重点学情回顾 学生情绪指标 */
    m_stClassSummaryInfo.strEmoSumUp = strEmotion;

    /* 情绪较兴奋 */
    if (m_stStudentInfo.nExcitementStageStart == 0 && m_stStudentInfo.nExcitementStageEnd == 0)
    {
        m_stClassSummaryInfo.strEmoSumUp += "学生情绪较兴奋阶段未发生于本堂课,";
    }
    else if (m_stStudentInfo.nExcitementStageStart < 60 && m_stStudentInfo.nExcitementStageEnd < 60)
    {
        m_stClassSummaryInfo.strEmoSumUp += "学生情绪较兴奋阶段发生于[" +
            std::to_string(m_stStudentInfo.nExcitementStageStart) + "sec~" +
            std::to_string(m_stStudentInfo.nExcitementStageEnd) + "sec]" + ",持续时间为" +
            std::to_string((m_stStudentInfo.nExcitementStageEnd - m_stStudentInfo.nExcitementStageStart)) + "sec,";
    }
    else
    {
        m_stClassSummaryInfo.strEmoSumUp += "学生情绪较兴奋阶段发生于[" +
            std::to_string(m_stStudentInfo.nExcitementStageStart / 60) + "min~" +
            std::to_string(m_stStudentInfo.nExcitementStageEnd / 60) + "min]" + ",持续时间为" +
            std::to_string((m_stStudentInfo.nExcitementStageEnd - m_stStudentInfo.nExcitementStageStart) / 60) + "min,";
    }

    /* 情绪较低落 */
    if (m_stStudentInfo.nLowStageStart == 0 && m_stStudentInfo.nLowStageEnd == 0)
    {
        m_stClassSummaryInfo.strEmoSumUp += "学生情绪较低落阶段未发生于本堂课,";
    }
    else if (m_stStudentInfo.nLowStageStart < 60 && m_stStudentInfo.nLowStageEnd < 60)
    {
        m_stClassSummaryInfo.strEmoSumUp += "学生情绪较低落阶段发生于[" +
            std::to_string(m_stStudentInfo.nLowStageStart) + "sec~" +
            std::to_string(m_stStudentInfo.nLowStageEnd) + "sec]" + ",持续时间为" +
            std::to_string((m_stStudentInfo.nLowStageEnd - m_stStudentInfo.nLowStageStart)) + "sec,";
    }
    else
    {
        m_stClassSummaryInfo.strEmoSumUp += "学生情绪较低落阶段发生于[" +
            std::to_string(m_stStudentInfo.nLowStageStart / 60) + "min~" +
            std::to_string(m_stStudentInfo.nLowStageEnd / 60) + "min]" + ",持续时间为" +
            std::to_string((m_stStudentInfo.nLowStageEnd - m_stStudentInfo.nLowStageStart) / 60) + "min,";
    }

    m_stClassSummaryInfo.strEmoSumUp += strEmoSuggest;

    return OK;
}

/* 开始处理-教师信息 */
BlError_E ResultsModule_NS::CResultsBase::beginDeal_teacherInfo()
{

    std::lock_guard<std::mutex> lock(m_mtxTeacher);

    /* 结束教学行为 */
    if (m_stParamInfo.stNeedParam.getRecordTime)
    {
        if (m_stTeacherInfo.stBehaviorInfo.listTaughtTime.size() > 0)
        {
            TimeSlotInfo_S& lastBehavior1 = m_stTeacherInfo.stBehaviorInfo.listTaughtTime.back();
            if (lastBehavior1.nUserParam == 0)
            {
                lastBehavior1.nUserParam = 1;
                lastBehavior1.nEnd       = m_stParamInfo.stNeedParam.getRecordTime();
            }
        }

        if (m_stTeacherInfo.stBehaviorInfo.listTourTime.size() > 0)
        {
            TimeSlotInfo_S& lastBehavior2 = m_stTeacherInfo.stBehaviorInfo.listTourTime.back();
            if (lastBehavior2.nUserParam == 0)
            {
                lastBehavior2.nUserParam = 2;
                lastBehavior2.nEnd       = m_stParamInfo.stNeedParam.getRecordTime();
            }
        }

        if (m_stTeacherInfo.stBehaviorInfo.listInteractionTime.size() > 0)
        {
            TimeSlotInfo_S& lastBehavior3 = m_stTeacherInfo.stBehaviorInfo.listInteractionTime.back();
            if (lastBehavior3.nUserParam == 0)
            {
                lastBehavior3.nUserParam = 3;
                lastBehavior3.nEnd       = m_stParamInfo.stNeedParam.getRecordTime();
            }
        }
    }

    /* 计算讲授时长 */
    for (auto item : m_stTeacherInfo.stBehaviorInfo.listTaughtTime)
    {
        int nTeme = item.nEnd - item.nStart;
        if (nTeme > 0)
        {
            m_stTeacherInfo.nTaughtTime += nTeme;
        }
    }

    /* 计算互动时长 */
    for (auto item : m_stTeacherInfo.stBehaviorInfo.listInteractionTime)
    {
        int nTeme = item.nEnd - item.nStart;
        if (nTeme > 0)
        {
            m_stTeacherInfo.nInteractionTime += nTeme;
        }
    }

    /* 计算指导时长 */
    for (auto item : m_stTeacherInfo.stBehaviorInfo.listDirectingTime)
    {
        int nTeme = item.nEnd - item.nStart;
        if (nTeme > 0)
        {
            m_stTeacherInfo.nDirectingTime += nTeme;
        }
    }

    /* 计算板书时长 */
    for (auto item : m_stTeacherInfo.stBehaviorInfo.listBoardTime)
    {
        int nTeme = item.nEnd - item.nStart;
        if (nTeme > 0)
        {
            m_stTeacherInfo.nBoardTime += nTeme;
        }
    }


/* 计算讲台时长 */
#if 1
    m_stTeacherInfo.nPodiumTime = 0;
    if (m_stParamInfo.stNeedParam.getRecordTime)
    {
        /* 讲台时长 = 课堂时长 - 巡视时长 */
        m_stTeacherInfo.nPodiumTime = m_stParamInfo.stNeedParam.getRecordTime() - m_stTeacherInfo.nTourTime;
    }
    if (m_stTeacherInfo.nPodiumTime <= 0)
    {
        /* 讲台时长 = 讲授时长 */
        m_stTeacherInfo.nPodiumTime = m_stTeacherInfo.nTaughtTime;
    }
#endif

/* 计算巡视时长 */
#if 1
    for (auto item : m_stTeacherInfo.stBehaviorInfo.listTourTime)
    {
        int nTeme = item.nEnd - item.nStart;
        if (nTeme > 0)
        {
            m_stTeacherInfo.nTourTime += nTeme;
        }
    }
#endif

/* 计算巡视次数 */
#if 1
    /* 当前时间块的开始时间和前一个时间块的结束时间之间的差异小于3, 不算一次巡视 */
    /* 初始化前一个时间块的结束时间 */
    const TimeSlotInfo_S* prevTimeSlot = &m_stTeacherInfo.stBehaviorInfo.listTourTime.front();

    m_stTeacherInfo.nTourNumber = 0;
    /* 遍历时间块列表 */
    for (const TimeSlotInfo_S& currentTimeSlot : m_stTeacherInfo.stBehaviorInfo.listTourTime)
    {
        /* 计算当前时间块的开始时间和前一个时间块的结束时间之间的差异 */
        int nDiff = currentTimeSlot.nStart - prevTimeSlot->nEnd;

        /* 如果差异大于3 */
        if (std::abs(nDiff) > 3)
        {
            m_stTeacherInfo.nTourNumber++;
        }
    }
    if (m_stTeacherInfo.nTourNumber == 0)
    {
        m_stTeacherInfo.nTourNumber = 1;
    }
#endif

    return OK;
}

/* 教育云平台开始处理-学生信息 */
BlError_E ResultsModule_NS::CResultsBase::beginDeal_PlatformStudentInfo()
{

    std::lock_guard<std::mutex> lock(m_mtxPlatformStudent);

    /* 结束学生行为、专注度 */
    /* 达到计算阈值 */
    int nTotal = m_stPlatformStudentInfo.stFocusScoreInfo.nListenTotal +
        m_stPlatformStudentInfo.stFocusScoreInfo.nPracticeTotal +
        m_stPlatformStudentInfo.stFocusScoreInfo.nDemonstrateTotal +
        m_stPlatformStudentInfo.stFocusScoreInfo.nReadTotal +
        m_stPlatformStudentInfo.stFocusScoreInfo.nDiscussTotal +
        m_stPlatformStudentInfo.stFocusScoreInfo.nDownDeskTotal;

    int nScore = 0;

    if (nTotal > 0)
    {
        nScore = ((m_stPlatformStudentInfo.stFocusScoreInfo.nListenTotal +
                   m_stPlatformStudentInfo.stFocusScoreInfo.nDemonstrateTotal +
                   m_stPlatformStudentInfo.stFocusScoreInfo.nPracticeTotal) *
                  100.0 /
                  nTotal);
        m_stPlatformStudentInfo.stFocusScoreInfo.listScore.push_back(nScore);
    }
    /* 清空数据 */
    m_stPlatformStudentInfo.stFocusScoreInfo.nListenTotal      = 0;
    m_stPlatformStudentInfo.stFocusScoreInfo.nPracticeTotal    = 0;
    m_stPlatformStudentInfo.stFocusScoreInfo.nDemonstrateTotal = 0;
    m_stPlatformStudentInfo.stFocusScoreInfo.nReadTotal        = 0;
    m_stPlatformStudentInfo.stFocusScoreInfo.nDiscussTotal     = 0;
    m_stPlatformStudentInfo.stFocusScoreInfo.nDownDeskTotal    = 0;

    /* 结束学生表情 */
    /* 达到计算阈值 */
    nTotal = m_stPlatformStudentInfo.stEmoInfo.nAngerTotal +
        m_stPlatformStudentInfo.stEmoInfo.nDisgustTotal +
        m_stPlatformStudentInfo.stEmoInfo.nFearTotal +
        m_stPlatformStudentInfo.stEmoInfo.nJoyTotal +
        m_stPlatformStudentInfo.stEmoInfo.nNeutralTotal +
        m_stPlatformStudentInfo.stEmoInfo.nSadnessTotal +
        m_stPlatformStudentInfo.stEmoInfo.nSurpriseTotal;

    AiManage_NS::Emotion_E enAction;

    if (nTotal > 0)
    {
        int nAverage = ((m_stPlatformStudentInfo.stEmoInfo.nAngerTotal * AiManage_NS::ANGER +
                         m_stPlatformStudentInfo.stEmoInfo.nDisgustTotal * AiManage_NS::DISGUST +
                         m_stPlatformStudentInfo.stEmoInfo.nFearTotal * AiManage_NS::FEAR +
                         m_stPlatformStudentInfo.stEmoInfo.nJoyTotal * AiManage_NS::JOY +
                         m_stPlatformStudentInfo.stEmoInfo.nNeutralTotal * AiManage_NS::NEUTRAL +
                         m_stPlatformStudentInfo.stEmoInfo.nSadnessTotal * AiManage_NS::SADNESS +
                         m_stPlatformStudentInfo.stEmoInfo.nSurpriseTotal * AiManage_NS::SURPRISE) /
                        nTotal);


        /* 返回最接近的情绪值 */
        if (nAverage >= AiManage_NS::ANGER && nAverage <= AiManage_NS::SURPRISE)
        {
            enAction = (AiManage_NS::Emotion_E)nAverage;
        }
        else
        {
            /* 如果平均值超出范围，则返回中性 */
            enAction = AiManage_NS::NEUTRAL;
        }

        m_stPlatformStudentInfo.stEmoInfo.listEmotion.push_back(enAction);
    }

    /* 清空数据 */
    m_stPlatformStudentInfo.stEmoInfo.nAngerTotal    = 0;
    m_stPlatformStudentInfo.stEmoInfo.nDisgustTotal  = 0;
    m_stPlatformStudentInfo.stEmoInfo.nFearTotal     = 0;
    m_stPlatformStudentInfo.stEmoInfo.nJoyTotal      = 0;
    m_stPlatformStudentInfo.stEmoInfo.nNeutralTotal  = 0;
    m_stPlatformStudentInfo.stEmoInfo.nSadnessTotal  = 0;
    m_stPlatformStudentInfo.stEmoInfo.nSurpriseTotal = 0;



    /* 计算人数信息 */
    int maxFreq = 0;
    for (auto& item : m_stPlatformStudentInfo.mapHumanCount)
    {
        if (item.second > maxFreq)
        {
            maxFreq                                    = item.second;
            m_stPlatformStudentInfo.nAverageHumanCount = item.first;
        }
    }
    if (maxFreq <= 1)
    {
        /* 求均值 */
        int nTotal = 0;
        for (auto& item : m_stPlatformStudentInfo.mapHumanCount)
        {
            nTotal += item.second * item.first;
        }

        if (nTotal > 0 && m_stPlatformStudentInfo.nFrameNum > 0)
        {
            m_stPlatformStudentInfo.nAverageHumanCount = (nTotal * 1.0 + 0.5) / m_stPlatformStudentInfo.nFrameNum;
        }
        else
        {
            m_stPlatformStudentInfo.nAverageHumanCount = 0;
        }
    }
    m_nCurPeopleNum = 0;

    /* 计算学生听讲时长 */
    for (auto item : m_stPlatformStudentInfo.stBehaviorInfo.listListenTime)
    {
        m_stPlatformStudentInfo.nListenTime += (item.nEnd - item.nStart);
    }

    /* 计算学生实践（练习）时长 */
    for (auto item : m_stPlatformStudentInfo.stBehaviorInfo.listPracticeTime)
    {
        m_stPlatformStudentInfo.nPracticeTime += (item.nEnd - item.nStart);
    }

    /* 计算学生演示时长 */
    for (auto item : m_stPlatformStudentInfo.stBehaviorInfo.listDemonstrateTime)
    {
        m_stPlatformStudentInfo.nDemonstrateTime += (item.nEnd - item.nStart);
    }

    /* 计算学生阅读时长 */
    for (auto item : m_stPlatformStudentInfo.stBehaviorInfo.listReadTime)
    {
        m_stPlatformStudentInfo.nReadTime += (item.nEnd - item.nStart);
    }

    /* 计算学生讨论时长 */
    for (auto item : m_stPlatformStudentInfo.stBehaviorInfo.listDiscussTime)
    {
        m_stPlatformStudentInfo.nDiscussTime += (item.nEnd - item.nStart);
    }

    /* 计算学生学习总时长 */
    m_stPlatformStudentInfo.nStudyTime = m_stPlatformStudentInfo.nPracticeTime + m_stPlatformStudentInfo.nDiscussTime;

    /* 计算专注度均值 */
    int nTotalScore = 0;
    for (auto item : m_stPlatformStudentInfo.stFocusScoreInfo.listScore)
    {
        nTotalScore += item;
    }
    if (!m_stPlatformStudentInfo.stFocusScoreInfo.listScore.empty())
    {
        m_stPlatformStudentInfo.stFocusScoreInfo.nAverageScore = nTotalScore / m_stPlatformStudentInfo.stFocusScoreInfo.listScore.size();
    }

    /* 计算注意力集中占比 */
    int nConcentrationNum = 0;
    for (auto item : m_stPlatformStudentInfo.stFocusScoreInfo.listScore)
    {
        if (item >= 70)
        {
            nConcentrationNum++;
        }
    }
    if (!m_stPlatformStudentInfo.stFocusScoreInfo.listScore.empty())
    {
        m_stPlatformStudentInfo.nConcentrationPct = nConcentrationNum * 100.0 / m_stPlatformStudentInfo.stFocusScoreInfo.listScore.size();
    }

    /* 计算注意力涣散占比 */
    int nDistractionNum = 0;
    for (auto item : m_stPlatformStudentInfo.stFocusScoreInfo.listScore)
    {
        if (item <= 40)
        {
            nDistractionNum++;
        }
    }
    if (!m_stPlatformStudentInfo.stFocusScoreInfo.listScore.empty())
    {
        m_stPlatformStudentInfo.nDistractionPct = nDistractionNum * 100.0 / m_stPlatformStudentInfo.stFocusScoreInfo.listScore.size();
    }

    /* 计算注意力集中/涣散阶段 */
    std::list<std::pair<std::list<TimeSlotInfo_S>, int>> listMaxTimer;
    std::list<std::pair<std::list<TimeSlotInfo_S>, int>> listMinTimer;

    TimeSlotInfo_S stTrmp;
    int            nCurAddTime        = 0;
    bool           nMaxContinuousFlag = false;
    bool           nMinContinuousFlag = false;
    for (auto item : m_stPlatformStudentInfo.stFocusScoreInfo.listScore)
    {
        if (item >= 60)
        {
            if (nMaxContinuousFlag == false)
            {
                std::pair<std::list<TimeSlotInfo_S>, int> pairTmp;
                pairTmp.second = item;
                stTrmp.nStart  = nCurAddTime;
                stTrmp.nEnd    = nCurAddTime + m_stPlatformStudentInfo.stFocusScoreInfo.nInterval;
                pairTmp.first.push_back(stTrmp);
                listMaxTimer.push_back(pairTmp);
            }
            else
            {
                /* 检查链表是否为空 */
                if (!listMaxTimer.empty())
                {
                    /* 获取最后一个链表 */
                    auto& lastPair = listMaxTimer.back();

                    stTrmp.nStart = nCurAddTime;
                    stTrmp.nEnd   = nCurAddTime + m_stPlatformStudentInfo.stFocusScoreInfo.nInterval;
                    lastPair.first.push_back(stTrmp);
                    lastPair.second += item;
                }
            }
            nMaxContinuousFlag = true;
        }
        else
        {
            nMaxContinuousFlag = false;
        }

        if (item <= 40)
        {
            if (nMinContinuousFlag == false)
            {
                /* 插入一个新表 */
                std::pair<std::list<TimeSlotInfo_S>, int> pairTmp;
                pairTmp.second = item;
                stTrmp.nStart  = nCurAddTime;
                stTrmp.nEnd    = nCurAddTime + m_stPlatformStudentInfo.stFocusScoreInfo.nInterval;
                pairTmp.first.push_back(stTrmp);
                listMinTimer.push_back(pairTmp);
            }
            else
            {
                /* 检查链表是否为空 */
                if (!listMinTimer.empty())
                {
                    /* 获取最后一个链表 */
                    auto& lastPair = listMinTimer.back();

                    stTrmp.nStart = nCurAddTime;
                    stTrmp.nEnd   = nCurAddTime + m_stPlatformStudentInfo.stFocusScoreInfo.nInterval;
                    lastPair.first.push_back(stTrmp);
                    lastPair.second += item;
                }
            }
            nMinContinuousFlag = true;
        }
        else
        {
            nMinContinuousFlag = false;
        }

        nCurAddTime += m_stPlatformStudentInfo.stFocusScoreInfo.nInterval;
    }

    int nMaxCount = 0;
    for (auto& item : listMaxTimer)
    {
        if (nMaxCount < item.first.size())
        {
            nMaxCount                                               = item.first.size();
            m_stPlatformStudentInfo.nConcentrationStageStart        = item.first.front().nStart;
            m_stPlatformStudentInfo.nConcentrationStageEnd          = item.first.back().nEnd;
            /* 计算平均值 */
            m_stPlatformStudentInfo.nConcentrationStageAverageScore = item.second / nMaxCount;
        }
    }

    int nMinCount = 0;
    for (auto& item : listMinTimer)
    {
        if (nMinCount < item.first.size())
        {
            nMinCount                                             = item.first.size();
            m_stPlatformStudentInfo.nDistractionStageStart        = item.first.front().nStart;
            m_stPlatformStudentInfo.nDistractionStageEnd          = item.first.back().nEnd;
            /* 计算平均值 */
            m_stPlatformStudentInfo.nDistractionStageAverageScore = item.second / nMinCount;
        }
    }

    /* 计算学生情绪兴奋/低落阶段 */
    std::list<TimeSlotInfo_S> listExcitementTimer; /* 兴奋 */
    std::list<TimeSlotInfo_S> listDepressionTimer; /* 低落 */

    // TimeSlotInfo_S stTrmp;
    stTrmp.clear();
    nCurAddTime          = 0;
    bool nExcitementFlag = false;
    bool nDepressionFlag = false;
    for (auto item : m_stPlatformStudentInfo.stEmoInfo.listEmotion)
    {
        /* 低落 */
        if (item == AiManage_NS::ANGER ||
            item == AiManage_NS::DISGUST ||
            item == AiManage_NS::FEAR ||
            item == AiManage_NS::SADNESS)
        {
            if (nDepressionFlag == false)
            {
                nDepressionFlag = true;
                stTrmp.nStart   = nCurAddTime;
                stTrmp.nEnd     = nCurAddTime + m_stPlatformStudentInfo.stEmoInfo.nInterval;
                listDepressionTimer.push_back(stTrmp);
            }
            else
            {
                /* 检查链表是否为空 */
                if (!listDepressionTimer.empty())
                {
                    /* 获取最后一个链表 */
                    auto& lastList = listDepressionTimer.back();
                    lastList.nEnd  = nCurAddTime;
                }
            }
        }
        else if (item == AiManage_NS::NEUTRAL)
        {
            if (nExcitementFlag)
            {
                /* 检查链表是否为空 */
                if (!listExcitementTimer.empty())
                {
                    /* 获取最后一个链表 */
                    auto& lastList = listExcitementTimer.back();
                    lastList.nEnd  = nCurAddTime;
                }
                nExcitementFlag = false;
            }

            if (nDepressionFlag)
            {
                /* 检查链表是否为空 */
                if (!listDepressionTimer.empty())
                {
                    /* 获取最后一个链表 */
                    auto& lastList = listDepressionTimer.back();
                    lastList.nEnd  = nCurAddTime;
                }
                nDepressionFlag = false;
            }
        }
        else
        {
            if (nExcitementFlag == false)
            {
                nExcitementFlag = true;
                stTrmp.nStart   = nCurAddTime;
                stTrmp.nEnd     = nCurAddTime + m_stPlatformStudentInfo.stEmoInfo.nInterval;
                listExcitementTimer.push_back(stTrmp);
            }
            else
            {
                /* 检查链表是否为空 */
                if (!listExcitementTimer.empty())
                {
                    /* 获取最后一个链表 */
                    auto& lastList = listExcitementTimer.back();
                    lastList.nEnd  = nCurAddTime;
                }
            }
        }

        nCurAddTime += m_stPlatformStudentInfo.stEmoInfo.nInterval;
    }

    nMaxCount = 0;
    for (auto& item : listExcitementTimer)
    {
        if (nMaxCount < item.nEnd - item.nStart)
        {
            nMaxCount                                     = item.nEnd - item.nStart;
            m_stPlatformStudentInfo.nExcitementStageStart = item.nStart;
            m_stPlatformStudentInfo.nExcitementStageEnd   = item.nEnd;
        }
    }

    nMaxCount = 0;
    for (auto& item : listDepressionTimer)
    {
        if (nMaxCount < item.nEnd - item.nStart)
        {
            nMaxCount                              = item.nEnd - item.nStart;
            m_stPlatformStudentInfo.nLowStageStart = item.nStart;
            m_stPlatformStudentInfo.nLowStageEnd   = item.nEnd;
        }
    }

    return OK;
}

/* 开始处理-学生信息 */
BlError_E ResultsModule_NS::CResultsBase::beginDeal_studentInfo()
{

    std::lock_guard<std::mutex> lock(m_mtxStudent);

    /* 结束学生行为、专注度 */
    /* 达到计算阈值 */
    int nTotal = m_stStudentInfo.stFocusScoreInfo.nListenTotal +
        m_stStudentInfo.stFocusScoreInfo.nPracticeTotal +
        m_stStudentInfo.stFocusScoreInfo.nDemonstrateTotal +
        m_stStudentInfo.stFocusScoreInfo.nReadTotal +
        m_stStudentInfo.stFocusScoreInfo.nDiscussTotal +
        m_stStudentInfo.stFocusScoreInfo.nDownDeskTotal;

    int nScore = 0;

    if (nTotal > 0)
    {
        nScore = ((m_stStudentInfo.stFocusScoreInfo.nListenTotal +
                   m_stStudentInfo.stFocusScoreInfo.nDemonstrateTotal +
                   m_stStudentInfo.stFocusScoreInfo.nPracticeTotal) *
                  100.0 /
                  nTotal);
        m_stStudentInfo.stFocusScoreInfo.listScore.push_back(nScore);
    }
    /* 清空数据 */
    m_stStudentInfo.stFocusScoreInfo.nListenTotal      = 0;
    m_stStudentInfo.stFocusScoreInfo.nPracticeTotal    = 0;
    m_stStudentInfo.stFocusScoreInfo.nDemonstrateTotal = 0;
    m_stStudentInfo.stFocusScoreInfo.nReadTotal        = 0;
    m_stStudentInfo.stFocusScoreInfo.nDiscussTotal     = 0;
    m_stStudentInfo.stFocusScoreInfo.nDownDeskTotal    = 0;

    /* 结束学生表情 */
    /* 达到计算阈值 */
    nTotal = m_stStudentInfo.stEmoInfo.nAngerTotal +
        m_stStudentInfo.stEmoInfo.nDisgustTotal +
        m_stStudentInfo.stEmoInfo.nFearTotal +
        m_stStudentInfo.stEmoInfo.nJoyTotal +
        m_stStudentInfo.stEmoInfo.nNeutralTotal +
        m_stStudentInfo.stEmoInfo.nSadnessTotal +
        m_stStudentInfo.stEmoInfo.nSurpriseTotal;

    AiManage_NS::Emotion_E enAction;

    if (nTotal > 0)
    {
        int nAverage = ((m_stStudentInfo.stEmoInfo.nAngerTotal * AiManage_NS::ANGER +
                         m_stStudentInfo.stEmoInfo.nDisgustTotal * AiManage_NS::DISGUST +
                         m_stStudentInfo.stEmoInfo.nFearTotal * AiManage_NS::FEAR +
                         m_stStudentInfo.stEmoInfo.nJoyTotal * AiManage_NS::JOY +
                         m_stStudentInfo.stEmoInfo.nNeutralTotal * AiManage_NS::NEUTRAL +
                         m_stStudentInfo.stEmoInfo.nSadnessTotal * AiManage_NS::SADNESS +
                         m_stStudentInfo.stEmoInfo.nSurpriseTotal * AiManage_NS::SURPRISE) /
                        nTotal);


        /* 返回最接近的情绪值 */
        if (nAverage >= AiManage_NS::ANGER && nAverage <= AiManage_NS::SURPRISE)
        {
            enAction = (AiManage_NS::Emotion_E)nAverage;
        }
        else
        {
            /* 如果平均值超出范围，则返回中性 */
            enAction = AiManage_NS::NEUTRAL;
        }

        m_stStudentInfo.stEmoInfo.listEmotion.push_back(enAction);
    }

    /* 清空数据 */
    m_stStudentInfo.stEmoInfo.nAngerTotal    = 0;
    m_stStudentInfo.stEmoInfo.nDisgustTotal  = 0;
    m_stStudentInfo.stEmoInfo.nFearTotal     = 0;
    m_stStudentInfo.stEmoInfo.nJoyTotal      = 0;
    m_stStudentInfo.stEmoInfo.nNeutralTotal  = 0;
    m_stStudentInfo.stEmoInfo.nSadnessTotal  = 0;
    m_stStudentInfo.stEmoInfo.nSurpriseTotal = 0;



    /* 计算人数信息 */
    int maxFreq = 0;
    for (auto& item : m_stStudentInfo.mapHumanCount)
    {
        if (item.second > maxFreq)
        {
            maxFreq                            = item.second;
            m_stStudentInfo.nAverageHumanCount = item.first;
        }
    }
    if (maxFreq <= 1)
    {
        /* 求均值 */
        int nTotal = 0;
        for (auto& item : m_stStudentInfo.mapHumanCount)
        {
            nTotal += item.second * item.first;
        }

        if (nTotal > 0 && m_stStudentInfo.nFrameNum > 0)
        {
            m_stStudentInfo.nAverageHumanCount = (nTotal * 1.0 + 0.5) / m_stStudentInfo.nFrameNum;
        }
        else
        {
            m_stStudentInfo.nAverageHumanCount = 0;
        }
    }
    m_nCurPeopleNum = 0;

    /* 计算学生听讲时长 */
    for (auto item : m_stStudentInfo.stBehaviorInfo.listListenTime)
    {
        m_stStudentInfo.nListenTime += (item.nEnd - item.nStart);
    }

    /* 计算学生实践（练习）时长 */
    for (auto item : m_stStudentInfo.stBehaviorInfo.listPracticeTime)
    {
        m_stStudentInfo.nPracticeTime += (item.nEnd - item.nStart);
    }

    /* 计算学生演示时长 */
    for (auto item : m_stStudentInfo.stBehaviorInfo.listDemonstrateTime)
    {
        m_stStudentInfo.nDemonstrateTime += (item.nEnd - item.nStart);
    }

    /* 计算学生阅读时长 */
    for (auto item : m_stStudentInfo.stBehaviorInfo.listReadTime)
    {
        m_stStudentInfo.nReadTime += (item.nEnd - item.nStart);
    }

    /* 计算学生讨论时长 */
    for (auto item : m_stStudentInfo.stBehaviorInfo.listDiscussTime)
    {
        m_stStudentInfo.nDiscussTime += (item.nEnd - item.nStart);
    }

    /* 计算学生学习总时长 */
    m_stStudentInfo.nStudyTime = m_stStudentInfo.nPracticeTime + m_stStudentInfo.nDiscussTime;

    /* 计算专注度均值 */
    int nTotalScore = 0;
    for (auto item : m_stStudentInfo.stFocusScoreInfo.listScore)
    {
        nTotalScore += item;
    }
    if (!m_stStudentInfo.stFocusScoreInfo.listScore.empty())
    {
        m_stStudentInfo.stFocusScoreInfo.nAverageScore = nTotalScore / m_stStudentInfo.stFocusScoreInfo.listScore.size();
    }

    /* 计算注意力集中占比 */
    int nConcentrationNum = 0;
    for (auto item : m_stStudentInfo.stFocusScoreInfo.listScore)
    {
        if (item >= 70)
        {
            nConcentrationNum++;
        }
    }
    if (!m_stStudentInfo.stFocusScoreInfo.listScore.empty())
    {
        m_stStudentInfo.nConcentrationPct = nConcentrationNum * 100.0 / m_stStudentInfo.stFocusScoreInfo.listScore.size();
    }

    /* 计算注意力涣散占比 */
    int nDistractionNum = 0;
    for (auto item : m_stStudentInfo.stFocusScoreInfo.listScore)
    {
        if (item <= 40)
        {
            nDistractionNum++;
        }
    }
    if (!m_stStudentInfo.stFocusScoreInfo.listScore.empty())
    {
        m_stStudentInfo.nDistractionPct = nDistractionNum * 100.0 / m_stStudentInfo.stFocusScoreInfo.listScore.size();
    }

    /* 计算注意力集中/涣散阶段 */
    std::list<std::pair<std::list<TimeSlotInfo_S>, int>> listMaxTimer;
    std::list<std::pair<std::list<TimeSlotInfo_S>, int>> listMinTimer;

    TimeSlotInfo_S stTrmp;
    int            nCurAddTime        = 0;
    bool           nMaxContinuousFlag = false;
    bool           nMinContinuousFlag = false;
    for (auto item : m_stStudentInfo.stFocusScoreInfo.listScore)
    {
        if (item >= 60)
        {
            if (nMaxContinuousFlag == false)
            {
                std::pair<std::list<TimeSlotInfo_S>, int> pairTmp;
                pairTmp.second = item;
                stTrmp.nStart  = nCurAddTime;
                stTrmp.nEnd    = nCurAddTime + m_stStudentInfo.stFocusScoreInfo.nInterval;
                pairTmp.first.push_back(stTrmp);
                listMaxTimer.push_back(pairTmp);
            }
            else
            {
                /* 检查链表是否为空 */
                if (!listMaxTimer.empty())
                {
                    /* 获取最后一个链表 */
                    auto& lastPair = listMaxTimer.back();

                    stTrmp.nStart = nCurAddTime;
                    stTrmp.nEnd   = nCurAddTime + m_stStudentInfo.stFocusScoreInfo.nInterval;
                    lastPair.first.push_back(stTrmp);
                    lastPair.second += item;
                }
            }
            nMaxContinuousFlag = true;
        }
        else
        {
            nMaxContinuousFlag = false;
        }

        if (item <= 40)
        {
            if (nMinContinuousFlag == false)
            {
                /* 插入一个新表 */
                std::pair<std::list<TimeSlotInfo_S>, int> pairTmp;
                pairTmp.second = item;
                stTrmp.nStart  = nCurAddTime;
                stTrmp.nEnd    = nCurAddTime + m_stStudentInfo.stFocusScoreInfo.nInterval;
                pairTmp.first.push_back(stTrmp);
                listMinTimer.push_back(pairTmp);
            }
            else
            {
                /* 检查链表是否为空 */
                if (!listMinTimer.empty())
                {
                    /* 获取最后一个链表 */
                    auto& lastPair = listMinTimer.back();

                    stTrmp.nStart = nCurAddTime;
                    stTrmp.nEnd   = nCurAddTime + m_stStudentInfo.stFocusScoreInfo.nInterval;
                    lastPair.first.push_back(stTrmp);
                    lastPair.second += item;
                }
            }
            nMinContinuousFlag = true;
        }
        else
        {
            nMinContinuousFlag = false;
        }

        nCurAddTime += m_stStudentInfo.stFocusScoreInfo.nInterval;
    }

    int nMaxCount = 0;
    for (auto& item : listMaxTimer)
    {
        if (nMaxCount < item.first.size())
        {
            nMaxCount                                       = item.first.size();
            m_stStudentInfo.nConcentrationStageStart        = item.first.front().nStart;
            m_stStudentInfo.nConcentrationStageEnd          = item.first.back().nEnd;
            /* 计算平均值 */
            m_stStudentInfo.nConcentrationStageAverageScore = item.second / nMaxCount;
        }
    }

    int nMinCount = 0;
    for (auto& item : listMinTimer)
    {
        if (nMinCount < item.first.size())
        {
            nMinCount                                     = item.first.size();
            m_stStudentInfo.nDistractionStageStart        = item.first.front().nStart;
            m_stStudentInfo.nDistractionStageEnd          = item.first.back().nEnd;
            /* 计算平均值 */
            m_stStudentInfo.nDistractionStageAverageScore = item.second / nMinCount;
        }
    }

    /* 计算学生情绪兴奋/低落阶段 */
    std::list<TimeSlotInfo_S> listExcitementTimer; /* 兴奋 */
    std::list<TimeSlotInfo_S> listDepressionTimer; /* 低落 */

    // TimeSlotInfo_S stTrmp;
    stTrmp.clear();
    nCurAddTime          = 0;
    bool nExcitementFlag = false;
    bool nDepressionFlag = false;
    for (auto item : m_stStudentInfo.stEmoInfo.listEmotion)
    {
        /* 低落 */
        if (item == AiManage_NS::ANGER ||
            item == AiManage_NS::DISGUST ||
            item == AiManage_NS::FEAR ||
            item == AiManage_NS::SADNESS)
        {
            if (nDepressionFlag == false)
            {
                nDepressionFlag = true;
                stTrmp.nStart   = nCurAddTime;
                stTrmp.nEnd     = nCurAddTime + m_stStudentInfo.stEmoInfo.nInterval;
                listDepressionTimer.push_back(stTrmp);
            }
            else
            {
                /* 检查链表是否为空 */
                if (!listDepressionTimer.empty())
                {
                    /* 获取最后一个链表 */
                    auto& lastList = listDepressionTimer.back();
                    lastList.nEnd  = nCurAddTime;
                }
            }
        }
        else if (item == AiManage_NS::NEUTRAL)
        {
            if (nExcitementFlag)
            {
                /* 检查链表是否为空 */
                if (!listExcitementTimer.empty())
                {
                    /* 获取最后一个链表 */
                    auto& lastList = listExcitementTimer.back();
                    lastList.nEnd  = nCurAddTime;
                }
                nExcitementFlag = false;
            }

            if (nDepressionFlag)
            {
                /* 检查链表是否为空 */
                if (!listDepressionTimer.empty())
                {
                    /* 获取最后一个链表 */
                    auto& lastList = listDepressionTimer.back();
                    lastList.nEnd  = nCurAddTime;
                }
                nDepressionFlag = false;
            }
        }
        else
        {
            if (nExcitementFlag == false)
            {
                nExcitementFlag = true;
                stTrmp.nStart   = nCurAddTime;
                stTrmp.nEnd     = nCurAddTime + m_stStudentInfo.stEmoInfo.nInterval;
                listExcitementTimer.push_back(stTrmp);
            }
            else
            {
                /* 检查链表是否为空 */
                if (!listExcitementTimer.empty())
                {
                    /* 获取最后一个链表 */
                    auto& lastList = listExcitementTimer.back();
                    lastList.nEnd  = nCurAddTime;
                }
            }
        }

        nCurAddTime += m_stStudentInfo.stEmoInfo.nInterval;
    }

    nMaxCount = 0;
    for (auto& item : listExcitementTimer)
    {
        if (nMaxCount < item.nEnd - item.nStart)
        {
            nMaxCount                             = item.nEnd - item.nStart;
            m_stStudentInfo.nExcitementStageStart = item.nStart;
            m_stStudentInfo.nExcitementStageEnd   = item.nEnd;
        }
    }

    nMaxCount = 0;
    for (auto& item : listDepressionTimer)
    {
        if (nMaxCount < item.nEnd - item.nStart)
        {
            nMaxCount                      = item.nEnd - item.nStart;
            m_stStudentInfo.nLowStageStart = item.nStart;
            m_stStudentInfo.nLowStageEnd   = item.nEnd;
        }
    }

    return OK;
}

/* 开始处理-考勤信息 */
BlError_E ResultsModule_NS::CResultsBase::beginDeal_attendanceInfo()
{
    return OK;
}

/* 处理板书识别分析数据后的处理 */
BlError_E ResultsModule_NS::CResultsBase::endDeal_boardDetecr(AiManage_NS::BehaviorInfo_S stInfo)
{
    /* 默认什么都不处理 */
    return OK;
}

/* 处理表情识别分析数据的处理 */
BlError_E ResultsModule_NS::CResultsBase::endDeal_emoDetecr(AiManage_NS::EmoInfo_S stInfo)
{
    /* 默认什么都不处理 */
    return OK;
}

/* 处理学生人脸识别分析数据的处理 */
BlError_E ResultsModule_NS::CResultsBase::endDeal_stFaceDetecrDetecr(AiManage_NS::FaceInfo_S stInfo)
{
    /* 默认什么都不处理 */
    return OK;
}

/* 处理学生回答问题人脸识别数据的处理 */
BlError_E ResultsModule_NS::CResultsBase::endDeal_stAsFaceDetecrDetecr(AiManage_NS::FaceInfo_S stInfo)
{
    /* 默认什么都不处理 */
    return OK;
}

/* 处理教师人脸识别分析数据的处理 */
BlError_E ResultsModule_NS::CResultsBase::endDeal_teFaceDetecrDetecr(AiManage_NS::FaceInfo_S stInfo)
{
    /* 默认什么都不处理 */
    return OK;
}

/* 处理轨迹识别分析数据的处理 */
BlError_E ResultsModule_NS::CResultsBase::endDeal_trackTeacher(AiManage_NS::TrackInfo_S stInfo)
{
    /* 默认什么都不处理 */
    return OK;
}

/* 处理教师接打电话分析数据的处理 */
BlError_E ResultsModule_NS::CResultsBase::endDeal_CallPhone(AiManage_NS::BehaviorInfo_S stInfo)
{
    /* 默认什么都不处理 */
    return OK;
}

/* 处理学生玩手机分析数据的处理 */
BlError_E ResultsModule_NS::CResultsBase::endDeal_PlayPhone(AiManage_NS::BehaviorInfo_S stInfo)
{
    /* 默认什么都不处理 */
    return OK;
}

/* 处理人数统计分析数据的处理 */
BlError_E ResultsModule_NS::CResultsBase::endDeal_countStudents(AiManage_NS::NumberInfo_S stInfo)
{
    /* 默认什么都不处理 */
    return OK;
}

/* 处理学生行为分析数据的处理 */
BlError_E ResultsModule_NS::CResultsBase::endDeal_studentBehavior(AiManage_NS::BehaviorInfo_S stInfo)
{
    /* 默认什么都不处理 */
    return OK;
}

BlError_E ResultsModule_NS::CResultsBase::add_studentBehavior(
    std::list<TimeSlotInfo_S>& listTime,
    bool                       bBehavior,
    int                        nRecordTime)
{
    int nTmpRecordTime = -1;

    if (bBehavior)
    {
        /* 开始行为 */
        /* 获取最后一个元素的引用并修改其内容 */
        if (!listTime.empty())
        {
            TimeSlotInfo_S& lastElement = listTime.back();
            if (lastElement.nUserParam != 0)
            {
                TimeSlotInfo_S stItem;
                stItem.clear();
                stItem.nStart     = nRecordTime > lastElement.nEnd ? nRecordTime : lastElement.nEnd;
                stItem.nEnd       = nRecordTime > lastElement.nEnd ? nRecordTime : lastElement.nEnd;
                stItem.nUserParam = 0;
                listTime.push_back(stItem);

                nTmpRecordTime = stItem.nEnd;
            }
            else
            {
                lastElement.nEnd = nRecordTime > lastElement.nEnd ? nRecordTime : lastElement.nEnd;
                nTmpRecordTime   = lastElement.nEnd;
            }
        }
        else
        {
            TimeSlotInfo_S stItem;
            stItem.clear();
            stItem.nStart     = nRecordTime;
            stItem.nEnd       = nRecordTime;
            stItem.nUserParam = 0;
            listTime.push_back(stItem);

            nTmpRecordTime = stItem.nEnd;
        }
    }
    else
    {
        /* 结束动作 */
        /* 获取最后一个元素的引用并修改其内容 */
        if (!listTime.empty())
        {
            TimeSlotInfo_S& lastElement = listTime.back();
            if (lastElement.nUserParam == 0)
            {
                lastElement.nEnd       = nRecordTime > lastElement.nEnd ? nRecordTime : lastElement.nEnd;
                lastElement.nUserParam = 1;
                nTmpRecordTime         = lastElement.nEnd;
            }
        }
    }

    if (nTmpRecordTime > -1)
    {
        save_stFullView(nTmpRecordTime);
    }

    return OK;
}

/* 添加教育云平台学生行为 */
BlError_E ResultsModule_NS::CResultsBase::add_platformStudentBehavior(
    std::list<TimeSlotInfo_S>& listTime,
    bool                       bBehavior,
    int                        nRecordTime,
    int                        nBehaviorType)
{
    int nTmpRecordTime = -1;

    if (bBehavior)
    {
        /* 开始行为 */
        /* 获取最后一个元素的引用并修改其内容 */
        if (!listTime.empty())
        {
            TimeSlotInfo_S& lastElement = listTime.back();
            if (lastElement.nUserParam != 0)
            {
                TimeSlotInfo_S stItem;
                stItem.clear();
                stItem.nStart     = nRecordTime > lastElement.nEnd ? nRecordTime : lastElement.nEnd;
                stItem.nEnd       = nRecordTime > lastElement.nEnd ? nRecordTime : lastElement.nEnd;
                stItem.nUserParam = 0;
                listTime.push_back(stItem);

                nTmpRecordTime = stItem.nEnd;
                if (m_stParamInfo.stNeedParam.platformBehaviorEvent)
                {
                    if (nBehaviorType >= LISTEN_TO_TALK && nBehaviorType <= TEACHER_PATROL)
                    {
                        /* 上报开始行为给教育云平台 nBehaviorType-行为类型 STUDENT-学生 */
                        m_stParamInfo.stNeedParam.platformBehaviorEvent(START_BEHAVIOR, nBehaviorType, STUDENT);
                    }
                }
            }
            else
            {
                lastElement.nEnd = nRecordTime > lastElement.nEnd ? nRecordTime : lastElement.nEnd;
                nTmpRecordTime   = lastElement.nEnd;
            }
        }
        else
        {
            TimeSlotInfo_S stItem;
            stItem.clear();
            stItem.nStart     = nRecordTime;
            stItem.nEnd       = nRecordTime;
            stItem.nUserParam = 0;
            listTime.push_back(stItem);

            nTmpRecordTime = stItem.nEnd;
            if (m_stParamInfo.stNeedParam.platformBehaviorEvent)
            {
                if (nBehaviorType >= LISTEN_TO_TALK && nBehaviorType <= TEACHER_PATROL)
                {
                    /* 上报开始行为给教育云平台 nBehaviorType-行为类型 STUDENT-学生  */
                    m_stParamInfo.stNeedParam.platformBehaviorEvent(START_BEHAVIOR, nBehaviorType, STUDENT);
                }
            }
        }
    }
    else
    {
        /* 结束动作 */
        /* 获取最后一个元素的引用并修改其内容 */
        if (!listTime.empty())
        {
            TimeSlotInfo_S& lastElement = listTime.back();
            if (lastElement.nUserParam == 0)
            {
                lastElement.nEnd       = nRecordTime > lastElement.nEnd ? nRecordTime : lastElement.nEnd;
                lastElement.nUserParam = 1;
                nTmpRecordTime         = lastElement.nEnd;

                if (m_stParamInfo.stNeedParam.platformBehaviorEvent)
                {
                    if (nBehaviorType >= LISTEN_TO_TALK && nBehaviorType <= TEACHER_PATROL)
                    {
                        /* 上报结束行为给教育云平台 nBehaviorType-行为类型 STUDENT-学生 */
                        m_stParamInfo.stNeedParam.platformBehaviorEvent(STOP_BEHAVIOR, nBehaviorType, STUDENT);
                    }
                }
            }
        }
    }

    return OK;
}

/* 保存学生全景截图 */
BlError_E ResultsModule_NS::CResultsBase::save_stFullView(int nRecordTime)
{
#if STU_BEHAV_SCREENSHOT
    if (m_stParamInfo.stNeedParam.sendStuPanoSS)
    {
        /* 创建目录 */
        CPublicFunc::makeDirectory(ST_FULL_VIEW);

        /* 保存截图文件文件 */
        std::string strPicPath = std::string(ST_FULL_VIEW) +
            std::string("/") +
            std::to_string(nRecordTime) +
            std::string(".jpg");
        m_stParamInfo.stNeedParam.sendStuPanoSS(strPicPath);
    }
#endif

    return OK;
}
