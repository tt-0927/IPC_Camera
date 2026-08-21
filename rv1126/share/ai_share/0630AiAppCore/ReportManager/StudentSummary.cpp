#include "StudentSummary.hpp"

#include <cstring>

#include "dlog.h"
#include "JsonInterfase.h"
#include "ToolFunc.hpp"

using namespace Ai0630_NS;

/* 添加行为分析 */
void StudentSummary::addBehavior(
    long long         nClassTime,
    StudentBehavior_E enBehavior)
{
    m_stBehavior.addBehavior(nClassTime, enBehavior);
}

/* 添加表情信息 */
void StudentSummary::addEmotion(
    long long nClassTime,
    Emotion_E enEmo)
{
    m_stEmotion.addEmotion(nClassTime, enEmo);
}

/* 统计信息 */
void StudentSummary::finalize(int nId, const void* pParam)
{
    /* 保存文件 */
    std::string strPath = ToolFunc::toString((char*)pParam, "/Student/", nId, ".json");
    saveFile(strPath);
}

/* 复位 */
void StudentSummary::reset()
{
    m_stBehavior.reset();
    m_stEmotion.reset();
}

/* 保存结果文件 */
void Ai0630_NS::StudentSummary::saveFile(std::string strFilePath)
{
    m_stStudentBehaviorResult = m_stBehavior.finalize();
    m_stEmotionTimelineResult = m_stEmotion.finalize();

    /* 转换成Json数据 */
    char* pchJsonData = nullptr;

    auto pRootJson = Json::init();
    auto pDataJson = Json::init();

    /* 学生听讲时长，单位/s */
    Json::add(pDataJson, "ListenTime",
              getOrDefault(m_stStudentBehaviorResult.mapBehaviorTotalDuration, StudentBehavior_E::LISTEN, 0));
    /* 学生实践（练习）时长，单位/s */
    Json::add(pDataJson, "PracticeTime",
              getOrDefault(m_stStudentBehaviorResult.mapBehaviorTotalDuration, StudentBehavior_E::PRACTICE, 0));
    /* 学生演示时长，单位/s */
    Json::add(pDataJson, "DemonstrateTime",
              getOrDefault(m_stStudentBehaviorResult.mapBehaviorTotalDuration, StudentBehavior_E::DEMO, 0));
    /* 学生阅读时长，单位/s */
    Json::add(pDataJson, "ReadTime",
              getOrDefault(m_stStudentBehaviorResult.mapBehaviorTotalDuration, StudentBehavior_E::READ, 0));
    /* 学生讨论时长，单位/s */
    Json::add(pDataJson, "DiscussTime",
              getOrDefault(m_stStudentBehaviorResult.mapBehaviorTotalDuration, StudentBehavior_E::DISCUSS, 0));
    /* 学生学习总时长，单位/s */
    Json::add(pDataJson, "StudyTime",
              getOrDefault(m_stStudentBehaviorResult.mapBehaviorTotalDuration, StudentBehavior_E::PRACTICE, 0) +
                  getOrDefault(m_stStudentBehaviorResult.mapBehaviorTotalDuration, StudentBehavior_E::DISCUSS, 0));
    /* 注意力集中占比，单位% */
    Json::add(pDataJson, "ConcentrationPct", m_stStudentBehaviorResult.fFocusRatioPercent);
    /* 注意力涣散占比，单位% */
    Json::add(pDataJson, "DistractionPct", m_stStudentBehaviorResult.fDistractRatioPercent);

    /* 集中注意力节点 */
    auto pConcentrationStageJson = Json::init();
    Json::add(pConcentrationStageJson, "Start", m_stStudentBehaviorResult.stLongestFocus.nStartTime);
    Json::add(pConcentrationStageJson, "End", m_stStudentBehaviorResult.stLongestFocus.nEndTime);
    Json::add(pConcentrationStageJson, "AverageScore", m_stStudentBehaviorResult.fMaxFocus);
    Json::add(pDataJson, "ConcentrationStage", pConcentrationStageJson);

    /* 注意力涣散节点 */
    auto pDistractionStageJson = Json::init();
    Json::add(pDistractionStageJson, "Start", m_stStudentBehaviorResult.stLongestDistract.nStartTime);
    Json::add(pDistractionStageJson, "End", m_stStudentBehaviorResult.stLongestDistract.nEndTime);
    Json::add(pDistractionStageJson, "AverageScore", m_stStudentBehaviorResult.fMinFocus);
    Json::add(pDataJson, "DistractionStage", pDistractionStageJson);

    /* 情绪兴奋节点 */
    auto pExcitementStageJson = Json::init();
    Json::add(pExcitementStageJson, "Start", m_stEmotionTimelineResult.stLongestExcited.nStartTime);
    Json::add(pExcitementStageJson, "End", m_stEmotionTimelineResult.stLongestExcited.nEndTime);
    Json::add(pDataJson, "ExcitementStage", pExcitementStageJson);

    /* 情绪低落节点 */
    auto pLowStageJson = Json::init();
    Json::add(pLowStageJson, "Start", m_stEmotionTimelineResult.stLongestLow.nStartTime);
    Json::add(pLowStageJson, "End", m_stEmotionTimelineResult.stLongestLow.nEndTime);
    Json::add(pDataJson, "LowStage", pLowStageJson);

    /* 表情总节点 */
    auto pExpressionTotalJson = Json::init();
    Json::add(pExpressionTotalJson, "Anger",
              getOrDefault(m_stEmotionTimelineResult.mapEmotionCount, Emotion_E::ANGER, 0));
    Json::add(pExpressionTotalJson, "Disgust",
              getOrDefault(m_stEmotionTimelineResult.mapEmotionCount, Emotion_E::DISGUST, 0));
    Json::add(pExpressionTotalJson, "Fear",
              getOrDefault(m_stEmotionTimelineResult.mapEmotionCount, Emotion_E::FEAR, 0));
    Json::add(pExpressionTotalJson, "Joy",
              getOrDefault(m_stEmotionTimelineResult.mapEmotionCount, Emotion_E::JOY, 0));
    Json::add(pExpressionTotalJson, "Neutral",
              getOrDefault(m_stEmotionTimelineResult.mapEmotionCount, Emotion_E::NEUTRAL, 0));
    Json::add(pExpressionTotalJson, "Sadness",
              getOrDefault(m_stEmotionTimelineResult.mapEmotionCount, Emotion_E::SADNESS, 0));
    Json::add(pExpressionTotalJson, "Surprise",
              getOrDefault(m_stEmotionTimelineResult.mapEmotionCount, Emotion_E::SURPRISE, 0));
    Json::add(pDataJson, "ExpressionTotal", pExpressionTotalJson);

    /* 行为总节点 */
    auto pBehaviorTotalJson = Json::init();
    Json::add(pBehaviorTotalJson, "Listen",
              getOrDefault(m_stStudentBehaviorResult.mapBehaviorCount, StudentBehavior_E::LISTEN, 0));
    Json::add(pBehaviorTotalJson, "Practice",
              getOrDefault(m_stStudentBehaviorResult.mapBehaviorCount, StudentBehavior_E::PRACTICE, 0));
    Json::add(pBehaviorTotalJson, "Demonstrate",
              getOrDefault(m_stStudentBehaviorResult.mapBehaviorCount, StudentBehavior_E::DEMO, 0));
    Json::add(pBehaviorTotalJson, "Read",
              getOrDefault(m_stStudentBehaviorResult.mapBehaviorCount, StudentBehavior_E::READ, 0));
    Json::add(pBehaviorTotalJson, "Discuss",
              getOrDefault(m_stStudentBehaviorResult.mapBehaviorCount, StudentBehavior_E::DISCUSS, 0));
    Json::add(pBehaviorTotalJson, "DownDesk",
              getOrDefault(m_stStudentBehaviorResult.mapBehaviorCount, StudentBehavior_E::DOWN_DESK, 0));
    Json::add(pDataJson, "BehaviorTotal", pBehaviorTotalJson);

    /* 表情信息 */
    auto          pEmotionInfoJson  = Json::init();
    Json::Object* pEmotionArrayJson = nullptr;

    int nNum = m_stEmotionTimelineResult.vecCurve.size();
    if (nNum <= 0)
    {
        pEmotionArrayJson = Json::Array::init();
    }
    else
    {
        int anNums[nNum] = { 0 };
        int nCount       = 0;
        for (auto item : m_stEmotionTimelineResult.vecCurve)
        {
            anNums[nCount++] = static_cast<int>(item.enEmotion);
        }
        pEmotionArrayJson = Json::Array::init(anNums, nNum);
    }
    Json::add(pEmotionInfoJson, "Emotion", pEmotionArrayJson);
    Json::add(pDataJson, "EmoInfo", pEmotionInfoJson);

    /* 行为信息 */
    auto pBehaviorInfoJson = Json::init();

    /* 听讲时间段，抬头 */
    auto pListenArrayJson = Json::Array::init();
    for (auto item : StudentBehaviorTimeline::getBehaviorSegmentsSafe(m_stStudentBehaviorResult.mapBehaviorSegments, StudentBehavior_E::LISTEN))
    {
        if (item.nEndTime != 0)
        {
            auto pTmpJson = Json::init();
            Json::add(pTmpJson, "Start", item.nStartTime);
            Json::add(pTmpJson, "End", item.nEndTime);
            Json::Array::add(pListenArrayJson, pTmpJson);
        }
    }
    Json::add(pBehaviorInfoJson, "ListenTime", pListenArrayJson);

    /* 实践时间段，举手 */
    auto pPracticeArrayJson = Json::Array::init();
    for (auto item : StudentBehaviorTimeline::getBehaviorSegmentsSafe(m_stStudentBehaviorResult.mapBehaviorSegments, StudentBehavior_E::PRACTICE))
    {
        if (item.nEndTime != 0)
        {
            auto pTmpJson = Json::init();
            Json::add(pTmpJson, "Start", item.nStartTime);
            Json::add(pTmpJson, "End", item.nEndTime);
            Json::Array::add(pPracticeArrayJson, pTmpJson);
        }
    }
    Json::add(pBehaviorInfoJson, "PracticeTime", pPracticeArrayJson);

    /* 演示时间段，站立 */
    auto pDemonstrateArrayJson = Json::Array::init();
    for (auto item : StudentBehaviorTimeline::getBehaviorSegmentsSafe(m_stStudentBehaviorResult.mapBehaviorSegments, StudentBehavior_E::DEMO))
    {
        if (item.nEndTime != 0)
        {
            auto pTmpJson = Json::init();
            Json::add(pTmpJson, "Start", item.nStartTime);
            Json::add(pTmpJson, "End", item.nEndTime);
            Json::Array::add(pDemonstrateArrayJson, pTmpJson);
        }
    }
    Json::add(pBehaviorInfoJson, "DemonstrateTime", pDemonstrateArrayJson);

    /* 阅读时间段，低头 */
    auto pReadArrayJson = Json::Array::init();
    for (auto item : StudentBehaviorTimeline::getBehaviorSegmentsSafe(m_stStudentBehaviorResult.mapBehaviorSegments, StudentBehavior_E::READ))
    {
        if (item.nEndTime != 0)
        {
            auto pTmpJson = Json::init();
            Json::add(pTmpJson, "Start", item.nStartTime);
            Json::add(pTmpJson, "End", item.nEndTime);
            Json::Array::add(pReadArrayJson, pTmpJson);
        }
    }
    Json::add(pBehaviorInfoJson, "ReadTime", pReadArrayJson);

    /* 讨论时间段，转头，转身 */
    auto pDiscussArrayJson = Json::Array::init();
    for (auto item : StudentBehaviorTimeline::getBehaviorSegmentsSafe(m_stStudentBehaviorResult.mapBehaviorSegments, StudentBehavior_E::DISCUSS))
    {
        if (item.nEndTime != 0)
        {
            auto pTmpJson = Json::init();
            Json::add(pTmpJson, "Start", item.nStartTime);
            Json::add(pTmpJson, "End", item.nEndTime);
            Json::Array::add(pDiscussArrayJson, pTmpJson);
        }
    }
    Json::add(pBehaviorInfoJson, "DiscussTime", pDiscussArrayJson);

    /* 趴桌行为时间点 */
    auto pDownDeskArrayJson = Json::Array::init();
    for (auto item : StudentBehaviorTimeline::getBehaviorSegmentsSafe(m_stStudentBehaviorResult.mapBehaviorSegments, StudentBehavior_E::DOWN_DESK))
    {
        if (item.nEndTime != 0)
        {
            auto pTmpJson = Json::init();
            Json::add(pTmpJson, "Start", item.nStartTime);
            Json::add(pTmpJson, "End", item.nEndTime);
            Json::Array::add(pDownDeskArrayJson, pTmpJson);
        }
    }
    Json::add(pBehaviorInfoJson, "DownDeskTime", pDownDeskArrayJson);
    Json::add(pDataJson, "BehaviorInfo", pBehaviorInfoJson);


    /* 专注度信息 */
    auto pFocusScoreInfoJson = Json::init();
    Json::add(pFocusScoreInfoJson, "AverageScore", m_stStudentBehaviorResult.fAvgFocus);

    Json::Object* pFocusScoreArrayJson = nullptr;

    nNum = m_stStudentBehaviorResult.vecFocusCurve.size();
    if (nNum <= 0)
    {
        pFocusScoreArrayJson = Json::Array::init();
    }
    else
    {
        int anNums[nNum] = { 0 };
        int nCount       = 0;
        for (auto item : m_stStudentBehaviorResult.vecFocusCurve)
        {
            anNums[nCount++] = item.fFocus;
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
    ToolFunc::writeDataToFile(
        strFilePath.c_str(),
        pchJsonData,
        strlen(pchJsonData));

    /* 释放空间 */
    Json::release(pchJsonData);
    pchJsonData = nullptr;
}
