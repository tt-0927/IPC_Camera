/*
 * @FilePath     : ResultsSaveJson.cpp
 * @Author       : 严泽辉 yanzeh@kfb.cn
 * @Date         : 2024-03-28 09:26:01
 * @LastEditors: weigl 308241951@qq.com
 * @LastEditTime: 2025-01-14 18:48:48
 * @Description  :
 */
#include "ResultsSaveJson.hpp"

#include <cstring>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "dlog.h"
#include "JsonInterfase.h"

ResultsModule_NS::CResultsSaveJson::CResultsSaveJson(InParam_S stInfo)
    : CResultsBase(stInfo)
{
    readConfidenceTh(CONFIDENCE_TH_INFO_JSON,m_stAiConfidenceTh);
}

ResultsModule_NS::CResultsSaveJson::~CResultsSaveJson()
{
}

/* 结束处理-课堂信息 */
BlError_E ResultsModule_NS::CResultsSaveJson::endDeal_classSummaryInfo(const void* pParam)
{
    char* pchFilePath = (char*)pParam;

    if (!pchFilePath)
    {
        dlog(LOG_ERROR, "传入参数异常");
        return ERR_PARAM;
    }

    /* 转换成Json数据 */
    char* pchJsonData = nullptr;

    BlError_E enRetCode = OK;

    auto pRootJson = Json::init();
    auto pDataJson = Json::init();


    std::lock_guard<std::mutex> lock(m_mtxClassSummary);

    /* 添加数据 */
    Json::add(pDataJson, "ClassTime", m_stClassSummaryInfo.nClassTime);
    Json::add(pDataJson, "ClassScore", m_stClassSummaryInfo.nClassScore);
    Json::add(pDataJson, "AiClassAdviceSumUp", m_stClassSummaryInfo.strAiClassAdviceSumUp);
    Json::add(pDataJson, "ClassConclusionSumUp", m_stClassSummaryInfo.strClassConclusionSumUp);
    Json::add(pDataJson, "ClassEvaluateSumUp", m_stClassSummaryInfo.strClassEvaluateSumUp);
    Json::add(pDataJson, "FocusScoreSumUp", m_stClassSummaryInfo.strFocusScoreSumUp);
    Json::add(pDataJson, "EmoSumUp", m_stClassSummaryInfo.strEmoSumUp);
    Json::add(pDataJson, "TeacherTime", m_stClassSummaryInfo.nTeacherTime);
    Json::add(pDataJson, "StudentTime", m_stClassSummaryInfo.nStudentTime);
    Json::add(pDataJson, "TaughtPct", m_stClassSummaryInfo.nTaughtPct);
    Json::add(pDataJson, "InteractionPct", m_stClassSummaryInfo.nInteractionPct);
    Json::add(pDataJson, "StudyPct", m_stClassSummaryInfo.nStudyPct);
    Json::add(pDataJson, "ParticipationRate", m_stClassSummaryInfo.nParticipationRate);
    Json::add(pDataJson, "HeadUpRate", m_stClassSummaryInfo.nHeadUpRate);
    Json::add(pDataJson, "HeadDownRate", m_stClassSummaryInfo.nHeadDownRate);

    /* RT曲线 */
    auto pRtJson = Json::init();
    Json::add(pRtJson, "X", m_stClassSummaryInfo.fRtChX);
    Json::add(pRtJson, "Y", m_stClassSummaryInfo.fRtChY);
    Json::add(pDataJson, "RT_CH", pRtJson);


    Json::add(pDataJson, "PPTInterval", m_stClassSummaryInfo.nPPTInterval);

    /* ST曲线 */
    auto pStArrayJson = Json::Array::init();
    for (auto item : m_stClassSummaryInfo.listSTInfo)
    {
        auto pTmpJson = Json::init();
        Json::add(pTmpJson, "X", item.nX);
        Json::add(pTmpJson, "Y", item.nY);
        Json::Array::add(pStArrayJson, pTmpJson);
    }
    Json::add(pDataJson, "ST", pStArrayJson);

    /* 提问时间点 */
    auto pQuizArrayJson = Json::Array::init();
    for (auto item : m_stClassSummaryInfo.listQuizTime)
    {
        auto pTmpJson = Json::init();
        Json::add(pTmpJson, "Start", item.nStart);
        Json::add(pTmpJson, "End", item.nEnd);
        Json::Array::add(pQuizArrayJson, pTmpJson);
    }
    Json::add(pDataJson, "QuizTime", pQuizArrayJson);

    /* 互动时间点 */
    auto pInteractionArrayJson = Json::Array::init();
    for (auto item : m_stClassSummaryInfo.listInteractionTime)
    {
        auto pTmpJson = Json::init();
        Json::add(pTmpJson, "Start", item.nStart);
        Json::add(pTmpJson, "End", item.nEnd);
        Json::Array::add(pInteractionArrayJson, pTmpJson);
    }
    Json::add(pDataJson, "InteractionTime", pInteractionArrayJson);

    Json::add(pRootJson, "ClassData", pDataJson);

    /* 转换成字符串 */
    pchJsonData = Json::print(pRootJson);

    /* 释放数据 */
    Json::deinit(pRootJson);

    /* 保存成文件 */
    enRetCode = write_toFile(pchFilePath, pchJsonData);

    /* 清空数据 */
    m_stClassSummaryInfo.clear();

    /* 释放空间 */
    Json::release(pchJsonData);
    pchJsonData = nullptr;

    return enRetCode;
}

/* 结束处理-教师信息 */
BlError_E ResultsModule_NS::CResultsSaveJson::endDeal_teacherInfo(const void* pParam)
{
    char* pchFilePath = (char*)pParam;

    if (!pchFilePath)
    {
        dlog(LOG_ERROR, "传入参数异常");
        return ERR_PARAM;
    }

    /* 转换成Json数据 */
    char* pchJsonData = nullptr;

    BlError_E enRetCode = OK;

    auto pRootJson = Json::init();
    auto pDataJson = Json::init();

    std::lock_guard<std::mutex> lock(m_mtxTeacher);

    /* 添加数据 */
    Json::add(pDataJson, "TaughtTime", m_stTeacherInfo.nTaughtTime);
    Json::add(pDataJson, "InteractionTime", m_stTeacherInfo.nInteractionTime);
    Json::add(pDataJson, "DirectingTime", m_stTeacherInfo.nDirectingTime);
    Json::add(pDataJson, "BoardTime", m_stTeacherInfo.nBoardTime);
    Json::add(pDataJson, "PodiumTime", m_stTeacherInfo.nPodiumTime);
    Json::add(pDataJson, "TourTime", m_stTeacherInfo.nTourTime);
    Json::add(pDataJson, "TourNumber", m_stTeacherInfo.nTourNumber);

    /* 行为信息 */
    auto pBehaviorInfoJson = Json::init();

    /* 教师讲授时间段 */
    auto pTaughtArrayJson = Json::Array::init();
    for (auto item : m_stTeacherInfo.stBehaviorInfo.listTaughtTime)
    {
        auto pTmpJson = Json::init();
        Json::add(pTmpJson, "Start", item.nStart);
        Json::add(pTmpJson, "End", item.nEnd);
        Json::Array::add(pTaughtArrayJson, pTmpJson);
    }
    Json::add(pBehaviorInfoJson, "TaughtTime", pTaughtArrayJson);

    /* 指导学生时间段 */
    auto pDirectingArrayJson = Json::Array::init();
    for (auto item : m_stTeacherInfo.stBehaviorInfo.listDirectingTime)
    {
        auto pTmpJson = Json::init();
        Json::add(pTmpJson, "Start", item.nStart);
        Json::add(pTmpJson, "End", item.nEnd);
        Json::Array::add(pDirectingArrayJson, pTmpJson);
    }
    Json::add(pBehaviorInfoJson, "DirectingTime", pDirectingArrayJson);

    /* 书写板书时间段 */
    auto pBoardArrayJson = Json::Array::init();
    for (auto item : m_stTeacherInfo.stBehaviorInfo.listBoardTime)
    {
        auto pTmpJson = Json::init();
        Json::add(pTmpJson, "Start", item.nStart);
        Json::add(pTmpJson, "End", item.nEnd);
        Json::Array::add(pBoardArrayJson, pTmpJson);
    }
    Json::add(pBehaviorInfoJson, "BoardTime", pBoardArrayJson);

    /* 师生互动时间段 */
    auto pInteractionArrayJson = Json::Array::init();
    for (auto item : m_stTeacherInfo.stBehaviorInfo.listInteractionTime)
    {
        auto pTmpJson = Json::init();
        Json::add(pTmpJson, "Start", item.nStart);
        Json::add(pTmpJson, "End", item.nEnd);
        Json::Array::add(pInteractionArrayJson, pTmpJson);
    }
    Json::add(pBehaviorInfoJson, "InteractionTime", pInteractionArrayJson);

    /* 教室巡视时间段 */
    auto pTourArrayJson = Json::Array::init();
    for (auto item : m_stTeacherInfo.stBehaviorInfo.listTourTime)
    {
        auto pTmpJson = Json::init();
        Json::add(pTmpJson, "Start", item.nStart);
        Json::add(pTmpJson, "End", item.nEnd);
        Json::Array::add(pTourArrayJson, pTmpJson);
    }
    Json::add(pBehaviorInfoJson, "TourTime", pTourArrayJson);
    Json::add(pDataJson, "BehaviorInfo", pBehaviorInfoJson);


    /* 教师轨迹数组 */
    auto pTrackArrayJson = Json::Array::init();
    for (auto item : m_stTeacherInfo.listTrackInfo)
    {
        auto pTmpJson = Json::init();
        Json::add(pTmpJson, "X", item.nX);
        Json::add(pTmpJson, "Y", item.nY);
        Json::Array::add(pTrackArrayJson, pTmpJson);
    }
    Json::add(pDataJson, "TrackInfo", pTrackArrayJson);

    /* 添加根节点 */
    Json::add(pRootJson, "SummaryData", pDataJson);

    /* 转换成字符串 */
    pchJsonData = Json::print(pRootJson);

    /* 释放数据 */
    Json::deinit(pRootJson);

    /* 保存成文件 */
    enRetCode = write_toFile(pchFilePath, pchJsonData);

    /* 清空数据 */
    m_stTeacherInfo.clear();

    /* 释放空间 */
    Json::release(pchJsonData);
    pchJsonData = nullptr;

    return enRetCode;
}

/* 结束处理-学生信息 */
BlError_E ResultsModule_NS::CResultsSaveJson::endDeal_studentInfo(const void* pParam)
{
    char* pchFilePath = (char*)pParam;

    if (!pchFilePath)
    {
        dlog(LOG_ERROR, "传入参数异常");
        return ERR_PARAM;
    }

    /* 转换成Json数据 */
    char* pchJsonData = nullptr;

    BlError_E enRetCode = OK;

    auto pRootJson = Json::init();
    auto pDataJson = Json::init();


    std::lock_guard<std::mutex> lock(m_mtxStudent);

    /* 添加数据 */
    Json::add(pDataJson, "FrameNum", m_stStudentInfo.nFrameNum);
    Json::add(pDataJson, "AverageHumanCount", m_stStudentInfo.nAverageHumanCount);
    Json::add(pDataJson, "ListenTime", m_stStudentInfo.nListenTime);
    Json::add(pDataJson, "PracticeTime", m_stStudentInfo.nPracticeTime);
    Json::add(pDataJson, "DemonstrateTime", m_stStudentInfo.nDemonstrateTime);
    Json::add(pDataJson, "ReadTime", m_stStudentInfo.nReadTime);
    Json::add(pDataJson, "DiscussTime", m_stStudentInfo.nDiscussTime);
    Json::add(pDataJson, "StudyTime", m_stStudentInfo.nStudyTime);
    Json::add(pDataJson, "ConcentrationPct", m_stStudentInfo.nConcentrationPct);
    Json::add(pDataJson, "DistractionPct", m_stStudentInfo.nDistractionPct);

    /* 集中注意力节点 */
    auto pConcentrationStageJson = Json::init();
    Json::add(pConcentrationStageJson, "Start", m_stStudentInfo.nConcentrationStageStart);
    Json::add(pConcentrationStageJson, "End", m_stStudentInfo.nConcentrationStageEnd);
    Json::add(pConcentrationStageJson, "AverageScore", m_stStudentInfo.nConcentrationStageAverageScore);
    Json::add(pDataJson, "ConcentrationStage", pConcentrationStageJson);

    /* 注意力涣散节点 */
    auto pDistractionStageJson = Json::init();
    Json::add(pDistractionStageJson, "Start", m_stStudentInfo.nDistractionStageStart);
    Json::add(pDistractionStageJson, "End", m_stStudentInfo.nDistractionStageEnd);
    Json::add(pDistractionStageJson, "AverageScore", m_stStudentInfo.nDistractionStageAverageScore);
    Json::add(pDataJson, "DistractionStage", pDistractionStageJson);

    /* 情绪兴奋节点 */
    auto pExcitementStageJson = Json::init();
    Json::add(pExcitementStageJson, "Start", m_stStudentInfo.nExcitementStageStart);
    Json::add(pExcitementStageJson, "End", m_stStudentInfo.nExcitementStageEnd);
    Json::add(pDataJson, "ExcitementStage", pExcitementStageJson);

    /* 情绪低落节点 */
    auto pLowStageJson = Json::init();
    Json::add(pLowStageJson, "Start", m_stStudentInfo.nLowStageStart);
    Json::add(pLowStageJson, "End", m_stStudentInfo.nLowStageEnd);
    Json::add(pDataJson, "LowStage", pLowStageJson);

    /* 表情总节点 */
    auto pExpressionTotalJson = Json::init();
    Json::add(pExpressionTotalJson, "Anger", m_stStudentInfo.nAngerTotal);
    Json::add(pExpressionTotalJson, "Disgust", m_stStudentInfo.nDisgustTotal);
    Json::add(pExpressionTotalJson, "Fear", m_stStudentInfo.nFearTotal);
    Json::add(pExpressionTotalJson, "Joy", m_stStudentInfo.nJoyTotal);
    Json::add(pExpressionTotalJson, "Neutral", m_stStudentInfo.nNeutralTotal);
    Json::add(pExpressionTotalJson, "Sadness", m_stStudentInfo.nSadnessTotal);
    Json::add(pExpressionTotalJson, "Surprise", m_stStudentInfo.nSurpriseTotal);
    Json::add(pDataJson, "ExpressionTotal", pExpressionTotalJson);

    /* 行为总节点 */
    auto pBehaviorTotalJson = Json::init();
    Json::add(pBehaviorTotalJson, "Listen", m_stStudentInfo.nListenTotal);
    Json::add(pBehaviorTotalJson, "Practice", m_stStudentInfo.nPracticeTotal);
    Json::add(pBehaviorTotalJson, "Demonstrate", m_stStudentInfo.nDemonstrateTotal);
    Json::add(pBehaviorTotalJson, "Read", m_stStudentInfo.nReadTotal);
    Json::add(pBehaviorTotalJson, "Discuss", m_stStudentInfo.nDiscussTotal);
    Json::add(pBehaviorTotalJson, "DownDesk", m_stStudentInfo.nDownDeskTotal);
    Json::add(pDataJson, "BehaviorTotal", pBehaviorTotalJson);

    /* 表情信息 */
    auto          pEmotionInfoJson  = Json::init();
    Json::Object* pEmotionArrayJson = nullptr;

    int nNum = m_stStudentInfo.stEmoInfo.listEmotion.size();
    if (nNum <= 0)
    {
        pEmotionArrayJson = Json::Array::init();
    }
    else
    {
        int anNums[nNum] = { 0 };
        int nCount       = 0;
        for (auto item : m_stStudentInfo.stEmoInfo.listEmotion)
        {
            anNums[nCount++] = item;
        }
        pEmotionArrayJson = Json::Array::init(anNums, nNum);
    }
    Json::add(pEmotionInfoJson, "Emotion", pEmotionArrayJson);
    Json::add(pEmotionInfoJson, "Interval", m_stStudentInfo.stEmoInfo.nInterval);
    Json::add(pDataJson, "EmoInfo", pEmotionInfoJson);

    /* 行为信息 */
    auto pBehaviorInfoJson = Json::init();

    /* 听讲时间段，抬头 */
    auto pListenArrayJson = Json::Array::init();
    for (auto item : m_stStudentInfo.stBehaviorInfo.listListenTime)
    {
        if (item.nEnd != 0)
        {
            auto pTmpJson = Json::init();
            Json::add(pTmpJson, "Start", item.nStart);
            Json::add(pTmpJson, "End", item.nEnd);
            Json::Array::add(pListenArrayJson, pTmpJson);
        }
    }
    Json::add(pBehaviorInfoJson, "ListenTime", pListenArrayJson);

    /* 实践时间段，举手 */
    auto pPracticeArrayJson = Json::Array::init();
    for (auto item : m_stStudentInfo.stBehaviorInfo.listPracticeTime)
    {
        if (item.nEnd != 0)
        {
            auto pTmpJson = Json::init();
            Json::add(pTmpJson, "Start", item.nStart);
            Json::add(pTmpJson, "End", item.nEnd);
            Json::Array::add(pPracticeArrayJson, pTmpJson);
        }
    }
    Json::add(pBehaviorInfoJson, "PracticeTime", pPracticeArrayJson);

    /* 演示时间段，站立 */
    auto pDemonstrateArrayJson = Json::Array::init();
    for (auto item : m_stStudentInfo.stBehaviorInfo.listDemonstrateTime)
    {
        if (item.nEnd != 0)
        {
            auto pTmpJson = Json::init();
            Json::add(pTmpJson, "Start", item.nStart);
            Json::add(pTmpJson, "End", item.nEnd);
            Json::Array::add(pDemonstrateArrayJson, pTmpJson);
        }
    }
    Json::add(pBehaviorInfoJson, "DemonstrateTime", pDemonstrateArrayJson);

    /* 阅读时间段，低头 */
    auto pReadArrayJson = Json::Array::init();
    for (auto item : m_stStudentInfo.stBehaviorInfo.listReadTime)
    {
        if (item.nEnd != 0)
        {
            auto pTmpJson = Json::init();
            Json::add(pTmpJson, "Start", item.nStart);
            Json::add(pTmpJson, "End", item.nEnd);
            Json::Array::add(pReadArrayJson, pTmpJson);
        }
    }
    Json::add(pBehaviorInfoJson, "ReadTime", pReadArrayJson);

    /* 讨论时间段，转头，转身 */
    auto pDiscussArrayJson = Json::Array::init();
    for (auto item : m_stStudentInfo.stBehaviorInfo.listDiscussTime)
    {
        if (item.nEnd != 0)
        {
            auto pTmpJson = Json::init();
            Json::add(pTmpJson, "Start", item.nStart);
            Json::add(pTmpJson, "End", item.nEnd);
            Json::Array::add(pDiscussArrayJson, pTmpJson);
        }
    }
    Json::add(pBehaviorInfoJson, "DiscussTime", pDiscussArrayJson);

    /* 趴桌行为时间点 */
    auto pDownDeskArrayJson = Json::Array::init();
    for (auto item : m_stStudentInfo.stBehaviorInfo.listDownDeskTime)
    {
        if (item.nEnd != 0)
        {
            auto pTmpJson = Json::init();
            Json::add(pTmpJson, "Start", item.nStart);
            Json::add(pTmpJson, "End", item.nEnd);
            Json::Array::add(pDownDeskArrayJson, pTmpJson);
        }
    }
    Json::add(pBehaviorInfoJson, "DownDeskTime", pDownDeskArrayJson);
    Json::add(pDataJson, "BehaviorInfo", pBehaviorInfoJson);


    /* 专注度信息 */
    auto pFocusScoreInfoJson = Json::init();
    Json::add(pFocusScoreInfoJson, "AverageScore", m_stStudentInfo.stFocusScoreInfo.nAverageScore);
    Json::add(pFocusScoreInfoJson, "Interval", m_stStudentInfo.stFocusScoreInfo.nInterval);

    Json::Object* pFocusScoreArrayJson = nullptr;

    nNum = m_stStudentInfo.stFocusScoreInfo.listScore.size();
    if (nNum <= 0)
    {
        pFocusScoreArrayJson = Json::Array::init();
    }
    else
    {
        int anNums[nNum] = { 0 };
        int nCount       = 0;
        for (auto item : m_stStudentInfo.stFocusScoreInfo.listScore)
        {
            anNums[nCount++] = item;
        }
        pFocusScoreArrayJson = Json::Array::init(anNums, nNum);
    }

    Json::add(pFocusScoreInfoJson, "Score", pFocusScoreArrayJson);
    Json::add(pDataJson, "FocusScoreInfo", pFocusScoreInfoJson);

    Json::add(pRootJson, "SummaryData", pDataJson);

    /* 转换成字符串 */
    pchJsonData = Json::print(pRootJson);

    /* 释放数据 */
    Json::deinit(pRootJson);

    /* 保存成文件 */
    enRetCode = write_toFile(pchFilePath, pchJsonData);

    /* 清空数据 */
    m_stStudentInfo.clear();

    /* 释放空间 */
    Json::release(pchJsonData);
    pchJsonData = nullptr;

    return enRetCode;
}

/* 结束处理-考勤信息 */
BlError_E ResultsModule_NS::CResultsSaveJson::endDeal_attendanceInfo(const void* pParam)
{
    char* pchFilePath = (char*)pParam;

    if (!pchFilePath)
    {
        dlog(LOG_ERROR, "传入参数异常");
        return ERR_PARAM;
    }

    /* 转换成Json数据 */
    char* pchJsonData = nullptr;

    BlError_E enRetCode = OK;

    auto pRootJson = Json::init();


    std::lock_guard<std::mutex> lock(m_mtxAttendance);

    /* StudentInfo节点 */
    auto pStudentArrayJson = Json::Array::init();
    for (auto it = m_stAttendanceInfo.mapStudentInfo.begin();
         it != m_stAttendanceInfo.mapStudentInfo.end();
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
    int                nTeMax = 0;
    TeAttendanceInfo_S stTmp;

    auto pTeacherArrayJson = Json::Array::init();
    for (auto it = m_stAttendanceInfo.mapTeacherInfo.begin();
         it != m_stAttendanceInfo.mapTeacherInfo.end();
         ++it)
    {
        if (nTeMax < it->second.nNumber)
        {
            nTeMax = it->second.nNumber;
            stTmp.clear();
            stTmp = it->second;
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
    enRetCode = write_toFile(pchFilePath, pchJsonData);

    /* 清空数据 */
    m_stAttendanceInfo.clear();

    /* 释放空间 */
    Json::release(pchJsonData);
    pchJsonData = nullptr;

    return enRetCode;
}

BlError_E ResultsModule_NS::CResultsSaveJson::readConfidenceTh(char *pchFilePath, AiConfidenceTh_S& stAiConfidenceTh)
{
    BlError_E enRetCode = OK;
    char* pchJsonData = nullptr;
    /*判断本地文件是否存在*/
    if (access(pchFilePath, F_OK) == 0)
    {
        pchJsonData = readJson_from_file(pchFilePath);
        if (pchJsonData == nullptr)
        {
            dlog(LOG_ERROR, "[数据管理] 读取配置文件失败");
        }
        else
        {
            auto pRootJson = Json::init(pchJsonData);
            Json::get(pRootJson, "StuDclCofid", stAiConfidenceTh.fStuDclCofid);
            Json::get(pRootJson, "GatherCofid", stAiConfidenceTh.fGatherCofid);
            Json::get(pRootJson, "StuPhoneCofid", stAiConfidenceTh.fStuPhoneCofid);
            Json::get(pRootJson, "TePhoneCofid", stAiConfidenceTh.fTePhoneCofid);
            Json::get(pRootJson, "TeBoardCofid", stAiConfidenceTh.fTeBoardCofid);
            Json::get(pRootJson, "Ratio", stAiConfidenceTh.fRatio);
            /* 使用完要释放*/
            free(pchJsonData);
            pchJsonData = nullptr;

            /* 释放数据 */
            Json::deinit(pRootJson);
        }
    }
    else
    {
        dlog(LOG_INFO, "本地文件不存在\n");
        stAiConfidenceTh.fStuDclCofid = 0.65;
        stAiConfidenceTh.fGatherCofid = 0.85;
        stAiConfidenceTh.fStuPhoneCofid = 0.2;
        stAiConfidenceTh.fTePhoneCofid = 0.5;
        stAiConfidenceTh.fRatio = 0.75;
        stAiConfidenceTh.fTeBoardCofid = 0.5;
        auto pRootJson = Json::init();
        Json::add(pRootJson, "StuDclCofid", stAiConfidenceTh.fStuDclCofid);
        Json::add(pRootJson, "GatherCofid", stAiConfidenceTh.fGatherCofid);
        Json::add(pRootJson, "StuPhoneCofid", stAiConfidenceTh.fStuPhoneCofid);
        Json::add(pRootJson, "TePhoneCofid", stAiConfidenceTh.fTePhoneCofid);
        Json::add(pRootJson, "TeBoardCofid", stAiConfidenceTh.fTeBoardCofid);
        Json::add(pRootJson, "Ratio", stAiConfidenceTh.fRatio);
        /* 转换成字符串 */
        pchJsonData = Json::print(pRootJson);

        /* 释放数据 */
        Json::deinit(pRootJson);

        /* 保存成文件 */
        enRetCode = write_toFile(pchFilePath, pchJsonData);

        /* 清空数据 */
        m_stAttendanceInfo.clear();

        /* 释放空间 */
        Json::release(pchJsonData);
        pchJsonData = nullptr;
    }

    return enRetCode;
}

/* 处理学生人脸识别分析数据的处理 */
BlError_E ResultsModule_NS::CResultsSaveJson::endDeal_stFaceDetecrDetecr(AiManage_NS::FaceInfo_S stInfo)
{
    /* 保存 */
    char achCmd[1024] = { 0 };

    int nId = -1;

    for (auto item : stInfo.listFaceInfo)
    {
        if (item.nId != -1)
        {
            nId = item.nId;
            memset(achCmd, 0, sizeof(achCmd));
            snprintf(achCmd, sizeof(achCmd), "cp %s/st_%lld.jpg %s/user_%d_%lld.jpg",
                     GANCIAN_PICTURE_TEMP_PATH,
                     stInfo.stHeadInfo.lTimestamp,
                     GANCIAN_PICTURE_TEMP_PATH,
                     item.nId,
                     stInfo.stHeadInfo.lTimestamp);

            if (system(achCmd) != 0)
            {
                dlog(LOG_ERROR, "保存人脸考勤图片失败 [%s]", achCmd);
            }
        }
    }

    if (nId != -1)
    {
        memset(achCmd, 0, sizeof(achCmd));
        snprintf(achCmd, sizeof(achCmd), "rm %s/st_%lld.jpg",
                 GANCIAN_PICTURE_TEMP_PATH,
                 stInfo.stHeadInfo.lTimestamp);
        dlog(LOG_ERROR, "删除图片[%s]", achCmd);

        system(achCmd);
    }

    return OK;
}

/* 处理学生回答问题人脸识别数据的处理 */
BlError_E ResultsModule_NS::CResultsSaveJson::endDeal_stAsFaceDetecrDetecr(AiManage_NS::FaceInfo_S stInfo)
{
    /* 保存 */
    char achCmd[1024] = { 0 };

    int nId = -1;

    for (auto item : stInfo.listFaceInfo)
    {
        if (item.nId != -1)
        {
            nId = item.nId;
            memset(achCmd, 0, sizeof(achCmd));
            snprintf(achCmd, sizeof(achCmd), "cp %s/st_%lld.jpg %s/user_%d_%lld.jpg",
                     GANCIAN_PICTURE_TEMP_PATH,
                     stInfo.stHeadInfo.lTimestamp,
                     GANCIAN_PICTURE_TEMP_PATH,
                     item.nId,
                     stInfo.stHeadInfo.lTimestamp);

            if (system(achCmd) != 0)
            {
                dlog(LOG_ERROR, "保存人脸考勤图片失败 [%s]", achCmd);
            }
        }
    }

    if (nId != -1)
    {
        memset(achCmd, 0, sizeof(achCmd));
        snprintf(achCmd, sizeof(achCmd), "rm %s/st_%lld.jpg",
                 GANCIAN_PICTURE_TEMP_PATH,
                 stInfo.stHeadInfo.lTimestamp);
        dlog(LOG_ERROR, "删除图片[%s]", achCmd);

        system(achCmd);
    }

    return OK;
}

/* 处理教师人脸识别分析数据的处理 */
BlError_E ResultsModule_NS::CResultsSaveJson::endDeal_teFaceDetecrDetecr(AiManage_NS::FaceInfo_S stInfo)
{
    /* 保存 */
    char achCmd[1024] = { 0 };

    int nId = -1;

    for (auto item : stInfo.listFaceInfo)
    {
        if (item.nId != -1)
        {
            nId = item.nId;
            memset(achCmd, 0, sizeof(achCmd));
            snprintf(achCmd, sizeof(achCmd), "cp %s/te_%lld.jpg %s/user_%d_%lld.jpg",
                     GANCIAN_PICTURE_TEMP_PATH,
                     stInfo.stHeadInfo.lTimestamp,
                     GANCIAN_PICTURE_TEMP_PATH,
                     item.nId,
                     stInfo.stHeadInfo.lTimestamp);

            if (system(achCmd) != 0)
            {
                dlog(LOG_ERROR, "保存人脸考勤图片失败 [%s]", achCmd);
            }
        }
    }

    if (nId != -1)
    {
        memset(achCmd, 0, sizeof(achCmd));
        snprintf(achCmd, sizeof(achCmd), "rm %s/te_%lld.jpg",
                 GANCIAN_PICTURE_TEMP_PATH,
                 stInfo.stHeadInfo.lTimestamp);
        dlog(LOG_ERROR, "删除图片[%s]", achCmd);

        system(achCmd);
    }

    return OK;
}

BlError_E ResultsModule_NS::CResultsSaveJson::endDeal_hotwordExtInfo(const void* pParam)
{
    char* pchFilePath = (char*)pParam;

    if (!pchFilePath)
    {
        dlog(LOG_ERROR, "传入参数异常");
        return ERR_PARAM;
    }

    /* 转换成Json数据 */
    char* pchJsonData = nullptr;
    BlError_E enRetCode = OK;

    auto pRootJson = Json::init();

    std::lock_guard<std::mutex> lock(m_mtxHotwordExtInfo);

    auto pStArrayJson = Json::Array::init();
    int nCount = 0;
    for(auto item : m_stHotwordExtInfo.listWordInfo)
    {
        if(nCount >= 20)
        {
            /* 最多提取前 20*/
            break;
        }
        auto pTmpJson = Json::init();
        Json::add(pTmpJson, "Word", item.strWord);
        Json::add(pTmpJson, "Count", item.nCount);
        Json::Array::add(pStArrayJson, pTmpJson);
        nCount ++;
    }
    Json::add(pRootJson, "WordsData", pStArrayJson);
    /* 转换成字符串 */
    pchJsonData = Json::print(pRootJson);
    /* 释放数据 */
    Json::deinit(pRootJson);

    /* 保存成文件 */
    enRetCode = write_toFile(pchFilePath, pchJsonData);

    /* 清空数据 */
    m_stHotwordExtInfo.clear();

    /* 释放空间 */
    Json::release(pchJsonData);
    pchJsonData = nullptr;

    //用 access() 判断文件是否存在，存在则用 remove() 删除
    if (access("/opt/bl/db/hot_words_ranking.db", F_OK) == 0) 
    {
        remove("/opt/bl/db/hot_words_ranking.db");
    }

    return enRetCode;
}

/* 写文件 */
BlError_E ResultsModule_NS::CResultsSaveJson::write_toFile(const char* pchFilePath, const char* pchJsonData)
{
    if (pchFilePath == NULL || pchJsonData == NULL)
    {
        dlog(LOG_ERROR, "传入参数异常");
        return ERR_PARAM;
    }

    struct stat stFileStat = { 0 };
    if (stat(pchFilePath, &stFileStat) == 0)
    {
        if (S_ISDIR(stFileStat.st_mode))
        {
            dlog(LOG_ERROR, "传入的文件路径为文件夹[%s]", pchFilePath);
            return ERR_FWRITE;
        }
        else if (access(pchFilePath, W_OK) != 0)
        {
            if (chmod(pchFilePath, S_IWUSR) != 0)
            {
                dlog(LOG_ERROR, "没有写权限，添加写权限失败[%s]", pchFilePath);
                return ERR_FWRITE;
            }
        }
    }

    FILE* pFp = fopen(pchFilePath, "w+");
    if (pFp == NULL)
    {
        dlog(LOG_ERROR, "打开文件失败[%s]", pchFilePath);
        return ERR_FWRITE;
    }

    size_t nLen     = strlen(pchJsonData);
    size_t nWritten = fwrite(pchJsonData, sizeof(char), nLen, pFp);


    fclose(pFp);

    if (nWritten != nLen)
    {
        dlog(LOG_ERROR, "写文件失败[%s]", pchFilePath);
        return ERR_FWRITE;
    }

    return OK;
}

/*读取文件中的Json数据*/
char* ResultsModule_NS::CResultsSaveJson::readJson_from_file(const char* pchFilePath)
{
    if (pchFilePath == NULL)
    {
        dlog(LOG_ERROR, "[转换数据] 传入参数异常");
        return NULL;
    }

    struct stat stFileStat = { 0 };

    if (stat(pchFilePath, &stFileStat) != 0)
    {
        dlog(LOG_ERROR, "[转换数据] 文件[%s]信息异常[%s]", pchFilePath, strerror(errno));
        return NULL;
    }

    if (S_ISDIR(stFileStat.st_mode))
    {
        dlog(LOG_ERROR, "[转换数据] 传入的文件路径为文件夹[%s]", pchFilePath);
        return NULL;
    }

    FILE* pFp = fopen(pchFilePath, "r");
    if (pFp == NULL)
    {
        dlog(LOG_ERROR, "[转换数据] 打开文件失败[%s]", pchFilePath);
        return NULL;
    }

    size_t nSize       = stFileStat.st_size;
    char*  pchJsonData = (char*)malloc(nSize + 1);
    if (pchJsonData == NULL)
    {
        dlog(LOG_ERROR, "[转换数据] 创建空间失败");
        fclose(pFp);
        return NULL;
    }

    size_t nReadSize = fread(pchJsonData, sizeof(char), nSize, pFp);
    if (nReadSize != nSize)
    {
        dlog(LOG_ERROR, "[转换数据] 读取数据长度异常");
        free(pchJsonData);
        fclose(pFp);
        return NULL;
    }

    pchJsonData[nSize] = '\0';

    fclose(pFp);

    return pchJsonData;
}
