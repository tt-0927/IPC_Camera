/***
 * @FilePath     : attendance_calculator.cpp
 * @Author       : zhengxh (zhengxh@kfb.cn)
 * @Date         : 2026-04-03 10:00:00
 * @LastEditors  : zhengxh
 * @LastEditTime : 2026-04-03 10:00:00
 * @Description  : 出勤情况计算器实现，基于视频帧人头数样本统计出勤、缺勤、迟到、早退人数。
 */
#ifdef ENABLE_AI_STUDENT
#include "attendance_calculator.hpp"
#include "mutex"
#include "dlog.h"
#include "ai_student_business.hpp"
#include <cstddef>
#include <numeric>

using namespace AiStudentBusiness_NS;

/* 触发一次汇总刷新所需的累积样本帧数 */
static constexpr int ATTENDANCE_SAMPLE_THRESHOLD = 10;

/* 余弦相识度 */
/******************************************************************************************************************************/
static float CosineSimilarity(const float *vec1, const float *vec2, int size)
{
    float dot_product = std::inner_product(vec1, vec1 + size, vec2, 0.0f);

    float norm1 = std::sqrt(std::inner_product(vec1, vec1 + size, vec1, 0.0f));
    float norm2 = std::sqrt(std::inner_product(vec2, vec2 + size, vec2, 0.0f));

    if (norm1 == 0.0 || norm2 == 0.0)
    {
        return 0.0f;
    }

    float similarity = dot_product / (norm1 * norm2);
    return similarity;
}
/******************************************************************************************************************************/

static std::vector<Student> FaceFeatureCompare(std::vector<AiStudentBusiness_NS::FaceInfo_t> &vecFaceeatures, std::vector<Student> stStudentsInfo)
{
    std::vector<Student> stAttendanceStusInfo;
    if (vecFaceeatures.empty() || stStudentsInfo.empty())
    {
        return stAttendanceStusInfo;
    }

    std::vector<float> fSimilarity;

    /* vecFaceeatures模型检测到的人脸特征 */
    for (auto &vecFaceeature : vecFaceeatures)
    {
        float   fMaxSimilarity = 0.0;
        Student stAttendanceStuInfo;

        /* stStudentInfo.vecFaceFeature 配置文件记录的人脸特征 */
        for (auto &stStudentInfo : stStudentsInfo)
        // for (unsigned int i = 0; i < stStudentsInfo.size(); i++)
        {
            float fSimilarity = CosineSimilarity(vecFaceeature.vecFaceFeatureData.data(), stStudentInfo.vecFaceFeature.data(), vecFaceeature.vecFaceFeatureData.size());
            if (fMaxSimilarity < fSimilarity)
            {
                /* 记录人脸库中相似度最高的学生 */
                stAttendanceStuInfo = stStudentInfo;
                fMaxSimilarity      = fSimilarity;
                // nIndex              = i;
            }
        }

        if (fMaxSimilarity > 0.35)
        {
            vecFaceeature.fSimilarity = fMaxSimilarity;
            vecFaceeature.strStuName  = stAttendanceStuInfo.name;
            stAttendanceStusInfo.push_back(stAttendanceStuInfo);
        }
    }

    return stAttendanceStusInfo;
}

void CAttendanceCalculator::setTotal(int nTotal)
{
    std::unique_lock<std::shared_mutex> lock(m_mutex);
    m_nTotal = nTotal;
    refreshSummary();
}

bool CAttendanceCalculator::addSample(std::vector<AiStudentBusiness_NS::FaceInfo_t> &vecFaceeatures, std::vector<Student> &stStudentsInfo)
{
    int nCount = vecFaceeatures.size();
    /* nCount人脸总数 */
    if (nCount < 0)
        return false;

    /* 进行人脸比对，得到班级里面的出勤人数 */
    std::vector<Student>                stAttendanceStusInfo = FaceFeatureCompare(vecFaceeatures, stStudentsInfo);
    std::unique_lock<std::shared_mutex> lock(m_mutex);
    nCount = stAttendanceStusInfo.size();
    /* 记录实际从模型得到的总人数 */
    m_mapHumanCount[nCount]++;

    /* 记录人脸比对后的班级出勤人数 */
    m_mapFrameStudentInfo[m_nSampleCount] = stAttendanceStusInfo;

    m_nSampleCount++;

    if (m_nSampleCount % ATTENDANCE_SAMPLE_THRESHOLD == 0)
    {
        refreshSummary();
        if (m_fnNotify)
        {
            m_fnNotify(m_stCachedSummary);
        }
        m_mapHumanCount.clear();
        m_mapFrameStudentInfo.clear();
        m_nSampleCount = 0;
    }
    return true;
}

bool CAttendanceCalculator::getSummary(AttendanceSummary &stOut) const
{
    std::shared_lock<std::shared_mutex> lock(m_mutex);
    stOut = m_stCachedSummary;
    return true;
}

void CAttendanceCalculator::reset()
{
    std::unique_lock<std::shared_mutex> lock(m_mutex);
    m_mapHumanCount.clear();
    m_mapFrameStudentInfo.clear();
    m_nSampleCount    = 0;
    m_stCachedSummary = AttendanceSummary{};
    m_fnNotify        = nullptr;
}

void CAttendanceCalculator::refreshSummary()
{
    m_stCachedSummary.total   = m_nTotal;
    m_stCachedSummary.present = calcPresent();
    // m_stCachedSummary.present    = std::min(m_stCachedSummary.present, m_stCachedSummary.total);    /* 暂时 */
    m_stCachedSummary.absent     = calcAbsent();
    m_stCachedSummary.late       = calcLate();
    m_stCachedSummary.earlyLeave = calcEarlyLeave();
    m_stCachedSummary.leave      = 0;
    // m_stCachedSummary.expectedAttendance = m_stCachedSummary.total - m_stCachedSummary.leave; /* 应到人数 = 总人数 - 请假人数 */

    /*debug*/
    dlog_debug("总人数：%d 出勤：%d 缺勤：%d 迟到：%d 早退：%d 请假：%d",
               m_stCachedSummary.total,
               m_stCachedSummary.present,
               m_stCachedSummary.absent,
               m_stCachedSummary.late,
               m_stCachedSummary.earlyLeave,
               m_stCachedSummary.leave);
}

int CAttendanceCalculator::calcPresent() const
{
    /* 优先取众数作为出勤人数估算值 */
    std::unordered_map<size_t, int> countMap;

    // 1. 统计每种 size 出现次数
    for (const auto &[frameId, vec] : m_mapFrameStudentInfo)
    {
        countMap[vec.size()]++;
    }

    // 2. 找出现次数最多的 size
    size_t bestSize = 0;
    int    maxCount = 0;

    for (const auto &[size, count] : countMap)
    {
        if (count > maxCount)
        {
            maxCount = count;
            bestSize = size;
        }
    }

    /* 众数出现次数大于 1 才认为有效 */
    if (static_cast<int>(bestSize) > 1)
    {
        return static_cast<int>(bestSize);
    }

    size_t maxSize = 0;

    for (const auto &[frameId, vec] : m_mapFrameStudentInfo)
    {
        if (vec.size() > maxSize)
        {
            maxSize = vec.size();
        }
    }

    return maxSize;

    // /* 众数无效时，以加权均值作为估算值 */
    // int nWeightedSum = 0;
    // int nFrameTotal  = 0;
    // for (const auto &item : m_mapHumanCount)
    // {
    //     nWeightedSum += item.first * item.second;
    //     nFrameTotal += item.second;
    // }
    // if (nFrameTotal == 0)
    // {
    //     return 0;
    // }

    // return static_cast<int>((nWeightedSum * 1.0 / nFrameTotal) + 0.5);
}

int CAttendanceCalculator::calcAbsent() const
{
    /**
        缺勤人数：课程时间内，应到人数减去出勤人数
     */

    int nAbsent = m_stCachedSummary.total - m_stCachedSummary.present - m_stCachedSummary.leave;
    if (nAbsent < 0)
    {
        nAbsent = 0;
    }
    return nAbsent;
}

int CAttendanceCalculator::calcLate() const
{
    /**
        迟到人数：课程开始时记录初始出勤人数，后续新增进入的人数均计为迟到
     */

    return 0;
}

int CAttendanceCalculator::calcEarlyLeave() const
{
    /**
        早退人数：课程内出勤人数峰值减去当前出勤人数
     */
    return 0;
}

void CAttendanceCalculator::setOnAttendanceCallback(AttendanceCallback fn)
{
    m_fnNotify = fn;
}

#endif