#include "ClassroomSummary.hpp"

#include <algorithm>
#include <cstring>
#include <iostream>
#include <numeric>

#include "dlog.h"
#include "JsonInterfase.h"
#include "ToolFunc.hpp"

using namespace Ai0630_NS;

/* 添加一次学生行为 */
void ClassroomSummary::addStudentBehavior()
{
    /* 只累计学生行为，不生成点 */
    ++m_nSTStudentCount;
    m_bStudent = true;
}

/* 添加一次教师行为 */
void ClassroomSummary::addTeacherBehavior()
{
    /* 教师行为变化 */
    ++m_nSTTeacherCount;
    if (m_bStudent)
    {
        m_bStudent = false;
        m_stClasstSummary.nStTeCutNum++;
    }

    /* 只有 X 变化时，记录一个点 */
    Point_S pt;
    pt.nX = m_nSTTeacherCount;
    pt.nY = m_nSTStudentCount;

    m_stClasstSummary.vecSTCurve.push_back(pt);
}

/* 添加课堂参数 */
void ClassroomSummary::addClassParam(const ClassParamParam_S& stParam)
{
    m_stClassParamParam = stParam;
}

/* 添加 PPT 翻页时间点 */
void ClassroomSummary::addPPTSwitch(int nClassTime)
{
    if (nClassTime < 0)
    {
        return; /* 非法时间直接忽略 */
    }

    m_stClasstSummary.vecPPTSwitchTimes.push_back(nClassTime);
}

/* 添加课堂分贝值 */
void Ai0630_NS::ClassroomSummary::addClassDb(int nClassTime, int nDb)
{
    if (nClassTime < 0)
    {
        return; /* 非法时间直接忽略 */
    }

    if (nClassTime >= m_stClasstSummary.nCurClassTime + 60)
    {

        Point_S pt;
        pt.nX = nClassTime;
        pt.nY = nDb;
        m_stClasstSummary.vecDb.push_back(pt);
    }

    m_stClasstSummary.nCurClassTime = nClassTime;
}

/* 整合 */
void ClassroomSummary::finalize(const void* pParam)
{
    /* 统计课堂时间分配 */
    m_stClasstSummary.nTeacherTimeRatio = m_stClassParamParam.nTeachDuration * 100.0 / m_stClassParamParam.nClassTime;
    m_stClasstSummary.nStudentTimeRatio = 100 - m_stClasstSummary.nTeacherTimeRatio;
    m_stClasstSummary.nTaughtPct        = m_stClassParamParam.nTeachDuration * 100.0 / m_stClassParamParam.nClassTime;
    m_stClasstSummary.nInteractionPct   = m_stClassParamParam.nInteractDuration * 100.0 / m_stClassParamParam.nClassTime;
    m_stClasstSummary.nStudyPct         = 100 - m_stClasstSummary.nTaughtPct - m_stClasstSummary.nInteractionPct;

    /* 参与率，单位% */
    m_stClasstSummary.nParticipationRate = (m_stClassParamParam.nInteractDuration + m_stClassParamParam.nGuideDuration + m_stClassParamParam.nWalkDuration) * 100 / m_stClassParamParam.nClassTime;
    /* 低头率，单位% */
    m_stClasstSummary.nHeadDownRate      = m_stClassParamParam.nHeadDownCount * 100 / m_stClassParamParam.nBehaviorCount;
    /* 抬头率，单位% */
    m_stClasstSummary.nHeadUpRate        = 100 - m_stClasstSummary.nHeadDownRate;

    /* PPT总结 */
    std::vector<int> intervals;
    if (m_stClasstSummary.vecPPTSwitchTimes.size() > 2)
    {
        /* 拷贝并排序，保证不破坏原始数据 */
        std::vector<int> times = m_stClasstSummary.vecPPTSwitchTimes;
        std::sort(times.begin(), times.end());

        for (size_t i = 1; i < times.size(); ++i)
        {

            int dt = times[i] - times[i - 1];
            if (dt > 0)
            {
                intervals.push_back(dt);
            }
        }
    }
    if (!intervals.empty())
    {
        m_stClasstSummary.nPPTInterval = std::accumulate(intervals.begin(), intervals.end(), 0) / intervals.size();
    }

    std::ostringstream oss;

    /* ---------- 学生行为总结 ---------- */
    m_stClasstSummary.strStudentFocusSummary +=
        buildFocusSummary(m_stClassParamParam.stLongestFocus,
                          m_stClassParamParam.nMaxFocus);
    m_stClasstSummary.strStudentFocusSummary +=
        buildDistractSummary(m_stClassParamParam.stLongestDistract,
                             m_stClassParamParam.nMinFocus);

    /* ---------- 情绪总结 ---------- */
    m_stClasstSummary.strEmotionSummary += buildExcitedSummary(m_stClassParamParam.stLongestExcited);
    m_stClasstSummary.strEmotionSummary += buildLowSummary(m_stClassParamParam.stLongestLow);

    /* ---------- 原课堂 6 句话 + 建议 ---------- */
    oss << buildClassAdvice();
    m_stClasstSummary.strClassSummary = oss.str();

    /* ---------- RT-CH 类型分析 ---------- */
    analyzeClassType();


    /* 保存文件 */
    std::string strPath = std::string((char*)pParam) + "/Summary.json";
    saveFile(strPath);
}

/* 清空记录（下一堂课） */
void ClassroomSummary::reset()
{
    /* ST曲线的 */
    m_nSTTeacherCount = 0;
    m_nSTStudentCount = 0;

    m_stClasstSummary.clear();
}

std::string ClassroomSummary::buildStageDesc(
    const std::string& prefix,
    long long          startSec,
    long long          endSec,
    int                avgScore,
    const std::string& emptyText)
{
    if (startSec <= 0 || endSec <= startSec)
    {
        return emptyText;
    }

    long long          duration = endSec - startSec;
    std::ostringstream oss;

    if (endSec < 60)
    {
        oss << prefix << "[" << startSec << "sec~" << endSec << "sec]";
        if (avgScore >= 0)
        {
            oss << "，平均专注度为" << avgScore;
        }
        oss << "，持续时间为" << duration << "sec,";
    }
    else
    {
        oss << prefix << "[" << startSec / 60 << "min~" << endSec / 60 << "min]";
        if (avgScore >= 0)
        {
            oss << "，平均专注度为" << avgScore;
        }
        oss << "，持续时间为" << duration / 60 << "min,";
    }

    return oss.str();
}

/* ---------------- 专注 / 散漫 ---------------- */
std::string ClassroomSummary::buildFocusSummary(
    const Segment_S& seg,
    int              avgScore)
{
    return buildStageDesc(
        "学生专注时间发生于",
        seg.nStartTime,
        seg.nEndTime,
        avgScore,
        "学生专注时间未发生于本堂课,");
}

std::string ClassroomSummary::buildDistractSummary(
    const Segment_S& seg,
    int              avgScore)
{
    return buildStageDesc(
        "学生散漫时间发生于",
        seg.nStartTime,
        seg.nEndTime,
        avgScore,
        "学生散漫时间未发生于本堂课,");
}

/* ---------------- 情绪 ---------------- */
std::string ClassroomSummary::buildExcitedSummary(
    const Segment_S& seg)
{
    return buildStageDesc(
        "学生情绪较兴奋阶段发生于",
        seg.nStartTime,
        seg.nEndTime,
        -1,
        "学生情绪较兴奋阶段未发生于本堂课,");
}

std::string ClassroomSummary::buildLowSummary(
    const Segment_S& seg)
{
    return buildStageDesc(
        "学生情绪较低落阶段发生于",
        seg.nStartTime,
        seg.nEndTime,
        -1,
        "学生情绪较低落阶段未发生于本堂课,");
}

std::string ClassroomSummary::buildClassAdvice()
{
    std::ostringstream oss;

    if (m_stClassParamParam.nClassScore > 90)
    {
        oss << "教学成效卓越，";
    }
    else if (m_stClassParamParam.nClassScore > 60)
    {
        oss << "教学效果总体积极，";
    }
    else
    {
        oss << "教学效果不理想，";
    }

    if (m_stClasstSummary.nTeacherTimeRatio > 75)
    {
        oss << "时间分配主要为教师讲解为主，";
    }
    else if (m_stClasstSummary.nTeacherTimeRatio > 45)
    {
        oss << "时间分配主要以教师和学生互动为主，";
    }
    else
    {
        oss << "时间分配主要以学生活动为主，";
    }

    if (m_stClasstSummary.nParticipationRate > 70)
    {
        oss << "学生参与课堂活动积极度较高，";
    }
    else if (m_stClasstSummary.nParticipationRate > 40)
    {
        oss << "学生参与课堂活动积极度中等，";
    }
    else
    {
        oss << "学生参与课堂活动积极度较差，";
    }

    int nEmotionRate1 = 0;
    int nEmotionRate2 = 0;
    if (m_stClassParamParam.nEmoCount > 0)
    {
        nEmotionRate1 = (m_stClassParamParam.nJoyCount + m_stClassParamParam.nSurpriseCount) * 100 / m_stClassParamParam.nEmoCount;
        nEmotionRate1 = 100 - ((m_stClassParamParam.nNeutralCount) * 100 / m_stClassParamParam.nEmoCount);
    }
    if (nEmotionRate1 > 65)
    {
        oss << "学生主要情绪为积极、快乐，课堂气氛活泼，";
    }
    else if (nEmotionRate2 > 65)
    {
        oss << "学生主要情绪为负面情绪，课堂气氛压抑，";
    }
    else
    {
        oss << "学生情绪平缓，课堂气氛平和，";
    }

    if (m_stClassParamParam.nClassScore > 70)
    {
        oss << "学生在课堂上表现出高度的注意力集中。\n";
    }
    else if (m_stClassParamParam.nClassScore > 60)
    {
        oss << "学生在课堂上的注意力集中程度处于中等水平。\n";
    }
    else
    {
        oss << "学生在课堂上的注意力集中程度较低。\n";
    }

    if (m_stClassParamParam.nClassScore <= 60)
    {
        oss << "1.教师需要思考教学方法并进行改进。\n";
    }
    else if (m_stClassParamParam.nClassScore <= 90)
    {
        oss << "1.定期回顾和更新教学材料，确保内容的时效性和吸引力。\n";
    }
    else
    {
        oss << "1.考虑将成功的教学实践和案例分享给其他教师，以促进教学方法的交流和提升。\n";
    }

    if (m_stClasstSummary.nTeacherTimeRatio > 75)
    {
        oss << "2.考虑平衡讲授与互动时间，增加学生参与度，以促进学生的批判性思维和深入理解。\n";
    }
    else if (m_stClasstSummary.nTeacherTimeRatio > 45)
    {
        oss << "2.注意根据课程内容和学生的学习需求灵活调整讲授与互动的时间比例。\n";
    }
    else
    {
        oss << "2.定期检查学生的学习进度和理解程度，确保教学目标得到实现。\n";
    }

    if (m_stClasstSummary.nParticipationRate > 70)
    {
        oss << "3.继续保持和加强现有的教学策略，利用学生的高参与度来进一步深化他们的理解和分析能力。\n";
    }
    else if (m_stClasstSummary.nParticipationRate > 40)
    {
        oss << "3.识别并了解参与度较低学生的具体原因，是否由于学习难度、兴趣不足或其他个人因素。\n";
    }
    else
    {
        oss << "3.重新审视和调整教学计划和课堂活动，确保教学内容与学生的兴趣和需求相匹配。\n";
    }

    if (nEmotionRate1 > 65)
    {
        oss << "4.继续保持和强化那些导致积极情绪的教学策略和课堂管理方法。\n";
    }
    else if (nEmotionRate2 > 65)
    {
        oss << "4.紧急审视和调整可能导致学生负面情绪的教学内容和方法，确保课程难度适中。\n";
    }
    else
    {
        oss << "4.分析课堂活动和教学方法，引入更多互动元素以激发兴趣。\n";
    }

    if (m_stClassParamParam.nClassScore > 70)
    {
        oss << "5.继续保持和强化当前有效的教学策略，以维持学生的高专注度。\n";
    }
    else if (m_stClassParamParam.nClassScore > 60)
    {
        oss << "5.分析当前教学策略，进一步提升学生专注度。\n";
    }
    else
    {
        oss << "5.尝试改变教学方法，引入更多互动和实践活动。\n";
    }

    return oss.str();
}

void ClassroomSummary::analyzeClassType()
{
    long long nTeacherDuration =
        m_stClassParamParam.nTeachDuration + m_stClassParamParam.nInteractDuration + m_stClassParamParam.nGuideDuration;

    if (nTeacherDuration <= 0)
    {
        m_stClasstSummary.fRtChX = 0.0f;
        m_stClasstSummary.fRtChY = 0.0f;
    }
    else
    {
        m_stClasstSummary.fRtChX =
            (m_stClassParamParam.nTeachDuration * 1.0f + m_stClassParamParam.nInteractDuration * 0.5f) / nTeacherDuration;

        m_stClasstSummary.fRtChY =
            m_stClasstSummary.nStTeCutNum * 1.0f / nTeacherDuration;
    }

    if (m_stClasstSummary.fRtChX <= 0.3f)
    {
        m_stClasstSummary.strClassTypeConclusion =
            "该堂课以学生独立完成作业或练习为主，教师的角色更多是指导和辅助。这种课堂类型适合于注重学生的实践操作，通过大量练习帮助学生巩固知识和技能。";

        m_stClasstSummary.strClassEvaluateSumUp =
            "该堂课程为练习型课堂，重点侧重于通过实际操作和练习来加深学生对知识点的理解和掌握。"
            "教师的引导和学生的实践时间分配较为均衡，建议更加关注技能的应用和实践，合理分配理论讲解与实际操作的时间，"
            "并关注学生的练习效率，通过即时反馈和指导，帮助学生巩固和提升所学技能。";
    }
    else if (m_stClasstSummary.fRtChX >= 0.7f)
    {
        m_stClasstSummary.strClassTypeConclusion =
            "本节课以教师的讲解为主，学生的主要任务是听讲和记笔记。这种课堂类型适合于传递大量信息和理论知识。";

        m_stClasstSummary.strClassEvaluateSumUp =
            "该堂课程为讲授型课堂，主要以教师的系统讲解为主，强调对知识体系的全面介绍和深入阐释。"
            "建议在讲授过程中穿插关键问题和思考点，以激发学生的思考和参与。";
    }
    else if (m_stClasstSummary.fRtChY >= 0.4f)
    {
        m_stClasstSummary.strClassTypeConclusion =
            "该堂课师生互动较为明显，师生互动频繁，学生有较多机会表达观点和交流。";

        m_stClasstSummary.strClassEvaluateSumUp =
            "该堂课程为对话型课堂，教师讲授与学生互动时间分配合理，建议进一步加强重点知识的深度挖掘。";
    }
    else
    {
        m_stClasstSummary.strClassTypeConclusion =
            "本节课结合了讲授和练习，教师讲授与学生参与之间的比例相对平衡。";

        m_stClasstSummary.strClassEvaluateSumUp =
            "该堂课程为混合型课堂，结合了讲授、互动和练习等多种教学方式，"
            "建议进一步优化教学内容结构，在学生注意力集中的时段安排综合性活动。";
    }
}

/* 保存结果文件 */
void ClassroomSummary::saveFile(std::string strFilePath)
{
    /* 转换成Json数据 */
    char* pchJsonData = nullptr;

    auto pRootJson = Json::init();
    auto pDataJson = Json::init();

    /* 班级上课时间 */
    Json::add(pDataJson, "ClassTime", m_stClassParamParam.nClassTime);
    /* 班级得分 */
    Json::add(pDataJson, "ClassScore", m_stClassParamParam.nClassScore);

    /* 各种评价 */
    Json::add(pDataJson, "AiClassAdviceSumUp", m_stClasstSummary.strClassSummary);
    Json::add(pDataJson, "ClassConclusionSumUp", m_stClasstSummary.strClassTypeConclusion);
    Json::add(pDataJson, "ClassEvaluateSumUp", m_stClasstSummary.strClassEvaluateSumUp);
    Json::add(pDataJson, "FocusScoreSumUp", m_stClasstSummary.strStudentFocusSummary);
    Json::add(pDataJson, "EmoSumUp", m_stClasstSummary.strEmotionSummary);

    /* 老师时间 */
    Json::add(pDataJson, "TeacherTime", m_stClasstSummary.nTeacherTimeRatio);
    /* 学生时间 */
    Json::add(pDataJson, "StudentTime", m_stClasstSummary.nStudentTimeRatio);

    /* 教师讲授时间占比，单位% */
    Json::add(pDataJson, "TaughtPct", m_stClasstSummary.nTaughtPct);
    /* 师生互动时间占比，单位% */
    Json::add(pDataJson, "InteractionPct", m_stClasstSummary.nInteractionPct);
    /* 学生学习时间占比，单位% */
    Json::add(pDataJson, "StudyPct", m_stClasstSummary.nStudyPct);

    /* 参与率，单位% */
    Json::add(pDataJson, "ParticipationRate", m_stClasstSummary.nParticipationRate);
    /* 抬头率，单位% */
    Json::add(pDataJson, "HeadUpRate", m_stClasstSummary.nHeadUpRate);
    /* 低头率，单位% */
    Json::add(pDataJson, "HeadDownRate", m_stClasstSummary.nHeadDownRate);

    /* RT曲线 */
    auto pRtJson = Json::init();
    Json::add(pRtJson, "X", m_stClasstSummary.fRtChX);
    Json::add(pRtJson, "Y", m_stClasstSummary.fRtChY);
    Json::add(pDataJson, "RT_CH", pRtJson);


    Json::add(pDataJson, "PPTInterval", m_stClasstSummary.nPPTInterval);

    /* ST曲线 */
    auto pStArrayJson = Json::Array::init();
    auto pTmpJson     = Json::init();
    Json::add(pTmpJson, "X", 0);
    Json::add(pTmpJson, "Y", 0);
    Json::Array::add(pStArrayJson, pTmpJson);
    for (auto item : m_stClasstSummary.vecSTCurve)
    {
        auto pTmpJson = Json::init();
        Json::add(pTmpJson, "X", item.nX);
        Json::add(pTmpJson, "Y", item.nY);
        Json::Array::add(pStArrayJson, pTmpJson);
    }
    Json::add(pDataJson, "ST", pStArrayJson);

    /* 分贝曲线 */
    auto pDbArrayJson = Json::Array::init();
    pTmpJson          = Json::init();
    Json::add(pTmpJson, "X", 0);
    Json::add(pTmpJson, "Y", 0);
    Json::Array::add(pDbArrayJson, pTmpJson);
    for (auto item : m_stClasstSummary.vecDb)
    {
        auto pTmpJson = Json::init();
        Json::add(pTmpJson, "X", item.nX);
        Json::add(pTmpJson, "Y", item.nY);
        Json::Array::add(pDbArrayJson, pTmpJson);
    }
    Json::add(pDataJson, "dbInfo", pDbArrayJson);

    /* 提问时间点 */
    auto pQuizArrayJson = Json::Array::init();
    for (auto item : m_stClassParamParam.vecQuizTime)
    {
        auto pTmpJson = Json::init();
        Json::add(pTmpJson, "Start", item.nStartTime);
        Json::add(pTmpJson, "End", item.nEndTime);
        Json::Array::add(pQuizArrayJson, pTmpJson);
    }
    Json::add(pDataJson, "QuizTime", pQuizArrayJson);

    /* 互动时间点 */
    auto pInteractionArrayJson = Json::Array::init();
    for (auto item : m_stClassParamParam.vecInteractionTime)
    {
        auto pTmpJson = Json::init();
        Json::add(pTmpJson, "Start", item.nStartTime);
        Json::add(pTmpJson, "End", item.nEndTime);
        Json::Array::add(pInteractionArrayJson, pTmpJson);
    }
    Json::add(pDataJson, "InteractionTime", pInteractionArrayJson);

    Json::add(pRootJson, "ClassData", pDataJson);

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
