#include "ClassStudentSummary.hpp"

#include <cstring>
#include <iostream>

#include "dlog.h"
#include "JsonInterfase.h"
#include "ToolFunc.hpp"

using namespace Ai0630_NS;

/* 添加个人行为分析 */
void ClassStudentSummary::addStudentBehavior(
    int       nId,
    long long nClassTime,
    int       nBehavior)
{
    m_students[nId].addBehavior(nClassTime, toBehavior(nBehavior));
}

/* 添加个人表情信息 */
void ClassStudentSummary::addStudentEmotion(
    int       nId,
    long long nClassTime,
    int       nEmo)
{
    m_students[nId].addEmotion(nClassTime, toEmotion(nEmo));
}

/* 添加全班行为信息 */
void ClassStudentSummary::addClassBehaviorBatch(
    long long                                         nClassTime,
    const std::unordered_map<StudentBehavior_E, int>& mapCnt)
{
    int nTotal = 0;
    for (auto& kv : mapCnt)
    {
        nTotal += kv.second;
    }

    for (auto& kv : mapCnt)
    {
        auto behavior = kv.first;
        int  nCount   = kv.second;

        bool bActive = behaviorActive(behavior, nCount, nTotal);
        if (bActive)
        {
            m_stClassBehavior.addBehavior(nClassTime, behavior);
        }
    }
}

/* 添加全班表情信息 */
void ClassStudentSummary::addClassEmotionBatch(
    long long                                 nClassTime,
    const std::unordered_map<Emotion_E, int>& mapCnt)
{
    if (mapCnt.empty())
    {
        return;
    }

    Emotion_E enBestEmo = Emotion_E::NEUTRAL;
    int       nMaxCount = -1;

    for (auto& kv : mapCnt)
    {
        if (kv.second > nMaxCount)
        {
            nMaxCount = kv.second;
            enBestEmo = kv.first;
        }
    }

    m_stClassEmotion.addEmotion(nClassTime, enBestEmo);
}

/* 当前检测到的总人数 */
void ClassStudentSummary::addPersonCount(int nCount, long long nClassTime)
{
    if (nCount <= 0)
    {
        return;
    }

    updateClassSizeRaw(nCount);
}

/* 统计信息 */
void ClassStudentSummary::finalize(const void* pParam)
{
    for (auto& kv : m_students)
    {
        kv.second.finalize(kv.first, pParam);
    }

    calcClassSize();

    /* 保存文件 */
    std::string strPath = std::string((char*)pParam) + "/Student.json";
    saveFile(strPath);
}

/* 复位 */
void ClassStudentSummary::reset()
{
    m_students.clear();

    m_stClassBehavior.reset();
    m_stClassEmotion.reset();

    m_detectedList.clear();
}

/* int转表情枚举 */
Emotion_E Ai0630_NS::ClassStudentSummary::toEmotion(int nEmo)
{
    static const Emotion_E table[] = {
        Emotion_E::ANGER,
        Emotion_E::DISGUST,
        Emotion_E::FEAR,
        Emotion_E::JOY,
        Emotion_E::NEUTRAL,
        Emotion_E::SADNESS,
        Emotion_E::SURPRISE
    };

    if (nEmo < 0 || nEmo >= (int)(sizeof(table) / sizeof(table[0])))
    {
        return Emotion_E::NEUTRAL;
    }

    return table[nEmo];
}

/* int转行为枚举 */
StudentBehavior_E Ai0630_NS::ClassStudentSummary::toBehavior(int nBehavior)
{
    static const StudentBehavior_E table[] = {
        StudentBehavior_E::LISTEN,
        StudentBehavior_E::PRACTICE,
        StudentBehavior_E::DEMO,
        StudentBehavior_E::READ,
        StudentBehavior_E::DISCUSS,
        StudentBehavior_E::DOWN_DESK
    };

    if (nBehavior < 0 || nBehavior >= (int)(sizeof(table) / sizeof(table[0])))
    {
        return StudentBehavior_E::LISTEN;
    }

    return table[nBehavior];
}

/* 获取班级参数 */
void Ai0630_NS::ClassStudentSummary::getInfo(ClassParamParam_S& stInfo)
{
    stInfo.nClassScore = m_stStudentBehaviorResult.fAvgFocus;
    stInfo.nMaxFocus   = m_stStudentBehaviorResult.fMaxFocus;
    stInfo.nMinFocus   = m_stStudentBehaviorResult.fMinFocus;
    stInfo.nHeadDownCount =
        getOrDefault(m_stStudentBehaviorResult.mapBehaviorCount, StudentBehavior_E::LISTEN, 0);
    stInfo.nBehaviorCount =
        getOrDefault(m_stStudentBehaviorResult.mapBehaviorCount, StudentBehavior_E::LISTEN, 0) +
        getOrDefault(m_stStudentBehaviorResult.mapBehaviorCount, StudentBehavior_E::PRACTICE, 0) +
        getOrDefault(m_stStudentBehaviorResult.mapBehaviorCount, StudentBehavior_E::DEMO, 0) +
        getOrDefault(m_stStudentBehaviorResult.mapBehaviorCount, StudentBehavior_E::READ, 0) +
        getOrDefault(m_stStudentBehaviorResult.mapBehaviorCount, StudentBehavior_E::DISCUSS, 0) +
        getOrDefault(m_stStudentBehaviorResult.mapBehaviorCount, StudentBehavior_E::DOWN_DESK, 0);

    stInfo.nJoyCount =
        getOrDefault(m_stEmotionTimelineResult.mapEmotionCount, Emotion_E::JOY, 0);
    stInfo.nSurpriseCount =
        getOrDefault(m_stEmotionTimelineResult.mapEmotionCount, Emotion_E::SURPRISE, 0);
    stInfo.nAngerCount =
        getOrDefault(m_stEmotionTimelineResult.mapEmotionCount, Emotion_E::ANGER, 0);
    stInfo.nDisgustCount =
        getOrDefault(m_stEmotionTimelineResult.mapEmotionCount, Emotion_E::DISGUST, 0);
    stInfo.nFearCount =
        getOrDefault(m_stEmotionTimelineResult.mapEmotionCount, Emotion_E::FEAR, 0);
    stInfo.nSadnessCount =
        getOrDefault(m_stEmotionTimelineResult.mapEmotionCount, Emotion_E::SADNESS, 0);
    stInfo.nNeutralCount =
        getOrDefault(m_stEmotionTimelineResult.mapEmotionCount, Emotion_E::NEUTRAL, 0);
    stInfo.nEmoCount = stInfo.nJoyCount +
        stInfo.nSurpriseCount +
        stInfo.nAngerCount +
        stInfo.nDisgustCount +
        stInfo.nFearCount +
        stInfo.nSadnessCount +
        stInfo.nNeutralCount;

    stInfo.stLongestFocus.nStartTime    = m_stStudentBehaviorResult.stLongestFocus.nStartTime;
    stInfo.stLongestFocus.nEndTime      = m_stStudentBehaviorResult.stLongestFocus.nEndTime;
    stInfo.stLongestDistract.nStartTime = m_stStudentBehaviorResult.stLongestDistract.nStartTime;
    stInfo.stLongestDistract.nEndTime   = m_stStudentBehaviorResult.stLongestDistract.nEndTime;
    stInfo.stLongestExcited.nStartTime  = m_stEmotionTimelineResult.stLongestExcited.nStartTime;
    stInfo.stLongestExcited.nEndTime    = m_stEmotionTimelineResult.stLongestExcited.nEndTime;
    stInfo.stLongestLow.nStartTime      = m_stEmotionTimelineResult.stLongestLow.nStartTime;
    stInfo.stLongestLow.nEndTime        = m_stEmotionTimelineResult.stLongestLow.nEndTime;
}

/* 判断是否存在行为 */
bool ClassStudentSummary::behaviorActive(
    StudentBehavior_E enType,
    int               nCount,
    int               nTotal) const
{
    if (nTotal <= 0 || nCount <= 0)
    {
        return false;
    }

    float percent = 100.0f * nCount / nTotal;

    switch (enType)
    {
        case StudentBehavior_E::READ:
            /* 阅读 > 20% 才认为有“低头”行为 */
            return percent > 20.0f;
        case StudentBehavior_E::LISTEN:
            /* 听讲 > 30% */
            return percent > 30.0f;
        case StudentBehavior_E::DISCUSS:
            /* 讨论 > 30% */
            return percent > 30.0f;
        case StudentBehavior_E::PRACTICE:
            /* 实践 > 5% */
            return percent > 5.0f;
        case StudentBehavior_E::DEMO:
            /* 演示只要存在就认为有 */
            return nCount > 0;
        case StudentBehavior_E::ACTION_NULL:
        default:
            return false;
    }
}

/* 预处理（记录原始人数） */
void ClassStudentSummary::updateClassSizeRaw(int nCount)
{
    /* 过滤不合理值（可调整） */

    m_detectedList.push_back(nCount);
}

/* 滑动窗口中位数滤波 */
int ClassStudentSummary::medianFilter(const std::vector<int>& vecData, int nIdx, int nWinSize)
{
    std::vector<int> vecWin;
    int              nHalf = nWinSize / 2;

    for (int i = nIdx - nHalf; i <= nIdx + nHalf; ++i)
    {
        if (i >= 0 && i < (int)vecData.size())
        {
            vecWin.push_back(vecData[i]);
        }
    }

    std::sort(vecWin.begin(), vecWin.end());
    return vecWin[vecWin.size() / 2];
}

/* 计算班级人数 */
void ClassStudentSummary::calcClassSize()
{
    if (m_detectedList.empty())
    {
        m_nClassSize = 0;
        return;
    }

    m_filteredList.clear();
    const int nWinSize = 5;

    /* 中位数滤波 */
    for (int i = 0; i < (int)m_detectedList.size(); ++i)
    {
        int mf = medianFilter(m_detectedList, i, nWinSize);
        m_filteredList.push_back(mf);
    }

    /* 构建直方图 */
    m_sizeHist.clear();
    for (int v : m_filteredList)
    {
        m_sizeHist[v]++;
    }

    /* 找众数（出现次数最多的） */
    int nBestCount = 0;
    int nBestFreq  = 0;

    for (auto& kv : m_sizeHist)
    {
        int nCount = kv.first;
        int nFreq  = kv.second;

        if (nFreq > nBestFreq)
        {
            nBestFreq  = nFreq;
            nBestCount = nCount;
        }
        else if (nFreq == nBestFreq && nCount > nBestCount)
        {
            nBestCount = nCount;
        }
    }

    m_nClassSize = nBestCount;
}

/* 保存结果文件 */
void Ai0630_NS::ClassStudentSummary::saveFile(std::string strFilePath)
{
    m_stStudentBehaviorResult = m_stClassBehavior.finalize();
    m_stEmotionTimelineResult = m_stClassEmotion.finalize();

    /* 转换成Json数据 */
    char* pchJsonData = nullptr;

    auto pRootJson = Json::init();
    auto pDataJson = Json::init();

    /* 平均人数，出席人数 */
    Json::add(pDataJson, "AverageHumanCount", m_nClassSize);
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
