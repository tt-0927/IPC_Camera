#include "TeacherSummary.hpp"

#include <algorithm>
#include <cstring>

#include "dlog.h"
#include "JsonInterfase.h"
#include "ToolFunc.hpp"

using namespace Ai0630_NS;


/* 正确的别名写法（类型别名，而不是 namespace） */
using Beh = TeacherBehavior_E;
using Pos = TeacherPosture_E;

/* 行为枚举转字符串 */
std::string behaviorToStr(Beh e)
{
    switch (e)
    {
        case Beh::TEACHING:
            return "教师讲授";
        case Beh::INTERACTION:
            return "师生互动";
        case Beh::GUIDING:
            return "指导学生";
        case Beh::WRITING_BOARD:
            return "书写板书";
        case Beh::WALKING_AROUND:
            return "教室巡视";
    }
    return "未知";
}

/* 姿态枚举转字符串 */
std::string postureToStr(Pos e)
{
    switch (e)
    {
        case Pos::OTHER:
            return "其他";
        case Pos::FRONT_EXPLAIN:
            return "正面讲解";
        case Pos::ARMS_CROSSED:
            return "双手抱臂";
        case Pos::FACE_BLACKBOARD:
            return "面向黑板";
        case Pos::HANDS_ON_HIP:
            return "叉腰表达";
        case Pos::HEAD_DOWN:
            return "低头";
        case Pos::HEAD_UP:
            return "抬头";
        case Pos::HAND_UP:
            return "举手示意";
    }
    return "未知";
}

/* ======================= 主类实现 ======================= */

TeacherSummary::TeacherSummary(long long mergeMs)
    : m_behavior(mergeMs), m_posture(mergeMs)
{
}

/* 添加行为 */
void TeacherSummary::addBehavior(
    const UserHeaderInfo_S& stUserHeaderInfo,
    TeacherBehavior_E       enType)
{
    m_behavior.addState(stUserHeaderInfo.nClassTime, enType);
}

/* 添加姿态 */
void TeacherSummary::addPosture(
    const UserHeaderInfo_S& stUserHeaderInfo,
    TeacherPosture_E        enType)
{
    m_posture.addState(stUserHeaderInfo.nClassTime, enType);
}

/* 添加位置 */
void TeacherSummary::addPosition(
    const UserHeaderInfo_S&          stUserHeaderInfo,
    std::vector<Inference_NS::Box_S> vstCurTurnBoxInfo)
{
    std::lock_guard<std::mutex> lock(m_mutex);

    bool       bExistTrack     = false;
    static int s_nDirectingNum = 0;
    if (!vstCurTurnBoxInfo.empty())
    {
        if (m_stCurSampleBoxInfo.nX1 == 0 &&
            m_stCurSampleBoxInfo.nX2 == 0 &&
            m_stCurSampleBoxInfo.nY1 == 0 &&
            m_stCurSampleBoxInfo.nY2 == 0)
        {
            /* 计算链表中距离上边距最远的数据 */
            int nMaxDistance = 0;

            for (const auto& box : vstCurTurnBoxInfo)
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
            double              dMinDistance = std::numeric_limits<double>::max();
            Inference_NS::Box_S stClosestBox = m_stCurSampleBoxInfo;

            for (const auto& box : vstCurTurnBoxInfo)
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
        if (!m_bCurTsPodium.load())
        {
            bExistTrack = true;
        }
    }

    /* 存在轨迹 */
    if (bExistTrack)
    {
        /* 比较是否重叠 */
        if (m_stCurSampleBoxInfo.nX1 != 0 ||
            m_stCurSampleBoxInfo.nX2 != 0 ||
            m_stCurSampleBoxInfo.nY1 != 0 ||
            m_stCurSampleBoxInfo.nY2 != 0 ||
            m_stLastSampleBoxInfo.nX1 != 0 ||
            m_stLastSampleBoxInfo.nX2 != 0 ||
            m_stLastSampleBoxInfo.nY1 != 0 ||
            m_stLastSampleBoxInfo.nY2 != 0)
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

                if (s_nDirectingNum > 5)
                {
                    s_nDirectingNum = 5;

                    m_behavior.addState(stUserHeaderInfo.nClassTime, TeacherBehavior_E::GUIDING);
                }
            }
            else
            {
                s_nDirectingNum--;

                if (s_nDirectingNum <= 0)
                {
                    m_stLastSampleBoxInfo = m_stCurSampleBoxInfo;
                    s_nDirectingNum       = 0;

                    m_behavior.addState(stUserHeaderInfo.nClassTime, TeacherBehavior_E::WALKING_AROUND);
                }
            }
        }

        /* 记录轨迹 */
        int nX = 0;
        int nY = 0;

        nX = (m_stCurSampleBoxInfo.nX1 + m_stCurSampleBoxInfo.nX2) / 2;
        nY = (m_stCurSampleBoxInfo.nY1 + m_stCurSampleBoxInfo.nY2) / 2;

        /* 数据保护 */
        if (nX > 1920)
        {
            nX = 1920;
        }
        else if (nX < 0)
        {
            nX = 0;
        }

        if (nY > 1080)
        {
            nY = 1080;
        }
        else if (nY < 0)
        {
            nY = 0;
        }

        /* 转换 */
        double dScaleX = (double)208 / 1920;
        double dScaleY = (double)106 / 1080;

        nX = 1920 - (nX * dScaleX + 16) * (1920 / 240);
        nY = 1080 - (nY * dScaleY + 24) * (1080 / 135);

        m_positions.push_back({ stUserHeaderInfo.nClassTime, nX, nY });
    }
}

/* int转姿态枚举 */
TeacherPosture_E TeacherSummary::toPosture(int nPosture)
{
    static const TeacherPosture_E table[] = {
        TeacherPosture_E::FRONT_EXPLAIN,
        TeacherPosture_E::ARMS_CROSSED,
        TeacherPosture_E::FACE_BLACKBOARD,
        TeacherPosture_E::HANDS_ON_HIP,
        TeacherPosture_E::HEAD_DOWN,
        TeacherPosture_E::HEAD_UP,
        TeacherPosture_E::HAND_UP,
        TeacherPosture_E::PLAY_PHONE,
        TeacherPosture_E::CALL_PHONE,
        TeacherPosture_E::TEABOARD,
    };

    if (nPosture < 0 || nPosture >= (int)(sizeof(table) / sizeof(table[0])))
    {
        return TeacherPosture_E::OTHER;
    }

    return table[nPosture];
}

/* 整合 */
void TeacherSummary::finalize(const void* pParam)
{
    m_behavior.finalize();
    m_posture.finalize();

    std::sort(m_positions.begin(), m_positions.end(),
              [](auto& a, auto& b) {
        return a.nClassTime < b.nClassTime;
    });

    /* 保存文件 */
    std::string strPath = std::string((char*)pParam) + "/Teacher.json";
    saveFile(strPath);

#if 1
    std::cout << "\n===== 行为区间统计 =====\n";
    for (auto& kv : getBehaviorStats())
    {
        std::cout << "行为：" << behaviorToStr(kv.first)
                  << ", 总时长 = " << kv.second.nTotalDurations << "ms\n";

        for (auto& seg : kv.second.vecSegments)
        {
            std::cout << "  区间 [" << seg.nStartTime << ", " << seg.nEndTime << "]\n";
        }
    }

    std::cout << "\n===== 姿态区间统计 =====\n";
    for (auto& kv : getPostureStats())
    {
        std::cout << "姿态：" << postureToStr(kv.first)
                  << ", 总时长 = " << kv.second.nTotalDurations << "ms\n";

        for (auto& seg : kv.second.vecSegments)
        {
            std::cout << "  区间 [" << seg.nStartTime << ", " << seg.nEndTime << "]\n";
        }
    }

    std::cout << "\n===== 热力图（已排序） =====\n";
    for (auto& p : getHeatMap())
    {
        std::cout << "t=" << p.nClassTime
                  << " → (" << p.nX << "," << p.nY << ")\n";
    }

    std::cout << "\n===== 最终课程总结 =====\n";
    auto s = getSummary();
    std::cout << "讲授时长: " << s.nTeachDuration << "ms\n";
    std::cout << "互动时长: " << s.nInteractDuration << "ms\n";
    std::cout << "指导时长: " << s.nGuideDuration << "ms\n";
    std::cout << "板书时长: " << s.nWriteBoardDuration << "ms\n";
    std::cout << "巡视时长: " << s.nWalkDuration << "ms\n";
    std::cout << "讲台总时长: " << s.nPodiumDuration << "ms\n";
    std::cout << "巡视总时长: " << s.nPatrolDuration << "ms\n";
    std::cout << "巡视次数: " << s.nPatrolCount << "\n";

    for (auto& kv : s.mapPostureRatio)
    {
        std::cout << postureToStr(kv.first)
                  << " 占比: " << kv.second << "%\n";
    }
#endif
}

/* 获取行为 */
const std::unordered_map<
    TeacherBehavior_E,
    ExclusiveTimeline<TeacherBehavior_E>::StateStat >&
    TeacherSummary::getBehaviorStats() const
{
    return m_behavior.getStats();
}

/* 获取姿态 */
const std::unordered_map<
    TeacherPosture_E,
    ExclusiveTimeline<TeacherPosture_E>::StateStat >&
    TeacherSummary::getPostureStats() const
{
    return m_posture.getStats();
}

/* 获取热力图 */
const std::vector<HeatPoint_S>& TeacherSummary::getHeatMap() const
{
    return m_positions;
}

/* 获取总结 */
TeacherBehaviorSummary_S TeacherSummary::getSummary() const
{
    TeacherBehaviorSummary_S out;

    /* ============ 行为统计（原逻辑） ============ */
    const auto& behStats = m_behavior.getStats();

    auto getDur = [&](TeacherBehavior_E e) -> long long {
        auto it = behStats.find(e);
        return (it != behStats.end()) ? it->second.nTotalDurations : 0;
    };

    out.nTeachDuration      = getDur(TeacherBehavior_E::TEACHING);
    out.nInteractDuration   = getDur(TeacherBehavior_E::INTERACTION);
    out.nGuideDuration      = getDur(TeacherBehavior_E::GUIDING);
    out.nWriteBoardDuration = getDur(TeacherBehavior_E::WRITING_BOARD);
    out.nWalkDuration       = getDur(TeacherBehavior_E::WALKING_AROUND);

    out.nPodiumDuration =
        out.nTeachDuration +
        out.nInteractDuration +
        out.nWriteBoardDuration;

    out.nPatrolDuration =
        out.nGuideDuration +
        out.nWalkDuration;

    /* ============ 统计巡视次数（原逻辑） ============ */
    struct Seg
    {
        long long         t0, t1;
        TeacherBehavior_E e;
    };

    std::vector<Seg> v;

    for (auto& kv : behStats)
    {
        for (auto& s : kv.second.vecSegments)
        {
            v.push_back({ s.nStartTime, s.nEndTime, kv.first });
        }
    }

    std::sort(v.begin(), v.end(), [](auto& a, auto& b) {
        return a.t0 < b.t0;
    });

    auto isPatrol = [&](TeacherBehavior_E e) {
        return (e == TeacherBehavior_E::GUIDING ||
                e == TeacherBehavior_E::WALKING_AROUND);
    };

    bool last = false;
    for (auto& s : v)
    {
        bool now = isPatrol(s.e);
        if (!last && now)
        {
            out.nPatrolCount++;
        }
        last = now;
    }

    /* =====================================================
     * 新增：姿态占比统计
     * ===================================================== */

    const auto& posStats = m_posture.getStats();

    long long totalPostureDur = 0;
    for (auto& kv : posStats)
    {
        totalPostureDur += kv.second.nTotalDurations;
    }

    if (totalPostureDur > 0)
    {
        for (auto& kv : posStats)
        {
            float ratio = float(kv.second.nTotalDurations) /
                float(totalPostureDur);

            out.mapPostureRatio[kv.first] = ratio * 100;
        }
    }

    return out;
}

/* 清空记录（下一堂课） */
void TeacherSummary::reset()
{
    m_behavior.reset();
    m_posture.reset();
    m_positions.clear();
}

/* 获取班级参数 */
void TeacherSummary::getInfo(ClassParamParam_S& stInfo)
{
    auto s                   = getSummary();
    stInfo.nTeachDuration    = s.nTeachDuration;
    stInfo.nInteractDuration = s.nInteractDuration;
    stInfo.nGuideDuration    = s.nGuideDuration;
    stInfo.nWalkDuration     = s.nPatrolDuration;

    for (auto& kv : getBehaviorStats())
    {
        switch (kv.first)
        {
            /* 师生互动 */
            case Beh::INTERACTION:
            {
                /* 师生互动时间段 */
                for (auto& seg : kv.second.vecSegments)
                {
                    Segment_S stSegment;
                    stSegment.nStartTime = seg.nStartTime;
                    stSegment.nEndTime   = seg.nEndTime;
                    stInfo.vecInteractionTime.push_back(stSegment);
                }
                break;
            }
            /* 指导学生 */
            case Beh::GUIDING:
            {
                /* 指导学生时间段 */
                for (auto& seg : kv.second.vecSegments)
                {
                    Segment_S stSegment;
                    stSegment.nStartTime = seg.nStartTime;
                    stSegment.nEndTime   = seg.nEndTime;
                    stInfo.vecQuizTime.push_back(stSegment);
                }
                break;
            }
            default:
            {
                break;
            }
        }
    }
}

/* 切换老师是否在讲台 */
BlError_E TeacherSummary::switchTeacherPodium(bool bValue)
{
    m_bCurTsPodium.store(bValue);
    m_stCurSampleBoxInfo.clear();
    m_stLastSampleBoxInfo.clear();
    return OK;
}

/* 保存结果文件 */
void TeacherSummary::saveFile(std::string strFilePath)
{
    /* 转换成Json数据 */
    char* pchJsonData = nullptr;

    auto pRootJson = Json::init();
    auto pDataJson = Json::init();


    auto s = getSummary();
    /* 讲授时长，单位/s */
    Json::add(pDataJson, "TaughtTime", s.nTeachDuration);
    /* 互动时长，单位/s */
    Json::add(pDataJson, "InteractionTime", s.nInteractDuration);
    /* 指导时长，单位/s */
    Json::add(pDataJson, "DirectingTime", s.nGuideDuration);
    /* 板书时长，单位/s */
    Json::add(pDataJson, "BoardTime", s.nWriteBoardDuration);
    /* 讲台时长，单位/s */
    Json::add(pDataJson, "PodiumTime", s.nPodiumDuration);
    /* 巡视时长，单位/s */
    Json::add(pDataJson, "TourTime", s.nPatrolDuration);
    /* 巡视次数 */
    Json::add(pDataJson, "TourNumber", s.nPatrolCount);


    /* 姿态信息 */
    auto pPostureInfoJson = Json::init();
    /* 正面讲解，单位% */
    Json::add(pPostureInfoJson, "frontExplainPercent",
              getOrDefault(s.mapPostureRatio, TeacherPosture_E::FRONT_EXPLAIN, 0));
    /* 双手抱臂，单位% */
    Json::add(pPostureInfoJson, "armsCrossedPercent",
              getOrDefault(s.mapPostureRatio, TeacherPosture_E::ARMS_CROSSED, 0));
    /* 面向黑板，单位% */
    Json::add(pPostureInfoJson, "facingBlackboardPercent",
              getOrDefault(s.mapPostureRatio, TeacherPosture_E::FACE_BLACKBOARD, 0));
    /* 叉腰表达，单位% */
    Json::add(pPostureInfoJson, "handsOnWaistPercent",
              getOrDefault(s.mapPostureRatio, TeacherPosture_E::HANDS_ON_HIP, 0));
    /* 低头，单位% */
    Json::add(pPostureInfoJson, "headDownPercent",
              getOrDefault(s.mapPostureRatio, TeacherPosture_E::HEAD_DOWN, 0));
    /* 抬头，单位% */
    Json::add(pPostureInfoJson, "headUpPercent",
              getOrDefault(s.mapPostureRatio, TeacherPosture_E::HEAD_UP, 0));
    /* 举手示意，单位% */
    Json::add(pPostureInfoJson, "handRaisePercent",
              getOrDefault(s.mapPostureRatio, TeacherPosture_E::HAND_UP, 0));
    Json::add(pDataJson, "PostureInfo", pPostureInfoJson);


    /* 行为信息 */
    auto pBehaviorInfoJson = Json::init();

    auto pTaughtArrayJson      = Json::Array::init();
    auto pInteractionArrayJson = Json::Array::init();
    auto pDirectingArrayJson   = Json::Array::init();
    auto pBoardArrayJson       = Json::Array::init();
    auto pTourArrayJson        = Json::Array::init();

    for (auto& kv : getBehaviorStats())
    {
        switch (kv.first)
        {
            /* 教师讲授 */
            case Beh::TEACHING:
            {
                /* 教师讲授时间段 */
                for (auto& seg : kv.second.vecSegments)
                {
                    auto pTmpJson = Json::init();
                    Json::add(pTmpJson, "Start", seg.nStartTime);
                    Json::add(pTmpJson, "End", seg.nEndTime);
                    Json::Array::add(pTaughtArrayJson, pTmpJson);
                }
                break;
            }
            /* 师生互动 */
            case Beh::INTERACTION:
            {
                /* 师生互动时间段 */
                for (auto& seg : kv.second.vecSegments)
                {
                    auto pTmpJson = Json::init();
                    Json::add(pTmpJson, "Start", seg.nStartTime);
                    Json::add(pTmpJson, "End", seg.nEndTime);
                    Json::Array::add(pInteractionArrayJson, pTmpJson);
                }
                break;
            }
            /* 指导学生 */
            case Beh::GUIDING:
            {
                /* 指导学生时间段 */
                for (auto& seg : kv.second.vecSegments)
                {
                    auto pTmpJson = Json::init();
                    Json::add(pTmpJson, "Start", seg.nStartTime);
                    Json::add(pTmpJson, "End", seg.nEndTime);
                    Json::Array::add(pDirectingArrayJson, pTmpJson);
                }
                break;
            }
            /* 书写板书 */
            case Beh::WRITING_BOARD:
            {
                /* 书写板书时间段 */
                for (auto& seg : kv.second.vecSegments)
                {
                    auto pTmpJson = Json::init();
                    Json::add(pTmpJson, "Start", seg.nStartTime);
                    Json::add(pTmpJson, "End", seg.nEndTime);
                    Json::Array::add(pBoardArrayJson, pTmpJson);
                }
                break;
            }
            /* 教室巡视 */
            case Beh::WALKING_AROUND:
            {
                /* 教室巡视时间段 */
                for (auto& seg : kv.second.vecSegments)
                {
                    auto pTmpJson = Json::init();
                    Json::add(pTmpJson, "Start", seg.nStartTime);
                    Json::add(pTmpJson, "End", seg.nEndTime);
                    Json::Array::add(pTourArrayJson, pTmpJson);
                }
                break;
            }
            default:
            {
                break;
            }
        }
    }
    Json::add(pBehaviorInfoJson, "TaughtTime", pTaughtArrayJson);
    Json::add(pBehaviorInfoJson, "InteractionTime", pInteractionArrayJson);
    Json::add(pBehaviorInfoJson, "DirectingTime", pDirectingArrayJson);
    Json::add(pBehaviorInfoJson, "BoardTime", pBoardArrayJson);
    Json::add(pBehaviorInfoJson, "TourTime", pTourArrayJson);
    Json::add(pDataJson, "BehaviorInfo", pBehaviorInfoJson);


    /* 教师姿态 */
    auto pPostureSectionInfoJson = Json::init();


    auto pOtherTimeArrayJson               = Json::Array::init();
    auto pArrayFrontExplainTimeJson        = Json::Array::init();
    auto pArrayArmsCrossedTimeJson         = Json::Array::init();
    auto pArrayFacingBlackboardTimeJson    = Json::Array::init();
    auto pArrayHandsOnWaistPercentTimeJson = Json::Array::init();
    auto pArrayHeadDownTimeJson            = Json::Array::init();
    auto pArrayHeadUpTimeJson              = Json::Array::init();
    auto pArrayHandRaiseTimeJson           = Json::Array::init();
    auto pArrayHandPlayPhoneTimeJson       = Json::Array::init();
    auto pArrayHandCallPhoneTimeJson       = Json::Array::init();
    auto pArrayHandTeaboardTimeJson        = Json::Array::init();

    for (auto& kv : getPostureStats())
    {
        switch (kv.first)
        {
            /* 其他 */
            case Pos::OTHER:
            {
                /* 其他时间段 */
                for (auto& seg : kv.second.vecSegments)
                {
                    auto pTmpJson = Json::init();
                    Json::add(pTmpJson, "Start", seg.nStartTime);
                    Json::add(pTmpJson, "End", seg.nEndTime);
                    Json::Array::add(pOtherTimeArrayJson, pTmpJson);
                }
                break;
            }
            /* 正面讲解 */
            case Pos::FRONT_EXPLAIN:
            {
                /* 正面讲解时间段 */
                for (auto& seg : kv.second.vecSegments)
                {
                    auto pTmpJson = Json::init();
                    Json::add(pTmpJson, "Start", seg.nStartTime);
                    Json::add(pTmpJson, "End", seg.nEndTime);
                    Json::Array::add(pArrayFrontExplainTimeJson, pTmpJson);
                }
                break;
            }
            /* 双手抱臂 */
            case Pos::ARMS_CROSSED:
            {
                /* 双手抱臂时间段 */
                for (auto& seg : kv.second.vecSegments)
                {
                    auto pTmpJson = Json::init();
                    Json::add(pTmpJson, "Start", seg.nStartTime);
                    Json::add(pTmpJson, "End", seg.nEndTime);
                    Json::Array::add(pArrayArmsCrossedTimeJson, pTmpJson);
                }
                break;
            }
            /* 面向黑板 */
            case Pos::FACE_BLACKBOARD:
            {
                /* 面向黑板时间段 */
                for (auto& seg : kv.second.vecSegments)
                {
                    auto pTmpJson = Json::init();
                    Json::add(pTmpJson, "Start", seg.nStartTime);
                    Json::add(pTmpJson, "End", seg.nEndTime);
                    Json::Array::add(pArrayFacingBlackboardTimeJson, pTmpJson);
                }
                break;
            }
            /* 叉腰表达 */
            case Pos::HANDS_ON_HIP:
            {
                /* 叉腰表达时间段 */
                for (auto& seg : kv.second.vecSegments)
                {
                    auto pTmpJson = Json::init();
                    Json::add(pTmpJson, "Start", seg.nStartTime);
                    Json::add(pTmpJson, "End", seg.nEndTime);
                    Json::Array::add(pArrayHandsOnWaistPercentTimeJson, pTmpJson);
                }
                break;
            }
            /* 低头 */
            case Pos::HEAD_DOWN:
            {
                /* 低头时间段 */
                for (auto& seg : kv.second.vecSegments)
                {
                    auto pTmpJson = Json::init();
                    Json::add(pTmpJson, "Start", seg.nStartTime);
                    Json::add(pTmpJson, "End", seg.nEndTime);
                    Json::Array::add(pArrayHeadDownTimeJson, pTmpJson);
                }
                break;
            }
            /* 抬头 */
            case Pos::HEAD_UP:
            {
                /* 抬头时间段 */
                for (auto& seg : kv.second.vecSegments)
                {
                    auto pTmpJson = Json::init();
                    Json::add(pTmpJson, "Start", seg.nStartTime);
                    Json::add(pTmpJson, "End", seg.nEndTime);
                    Json::Array::add(pArrayHeadUpTimeJson, pTmpJson);
                }
                break;
            }
            /* 举手示意 */
            case Pos::HAND_UP:
            {
                /* 举手示意时间段 */
                for (auto& seg : kv.second.vecSegments)
                {
                    auto pTmpJson = Json::init();
                    Json::add(pTmpJson, "Start", seg.nStartTime);
                    Json::add(pTmpJson, "End", seg.nEndTime);
                    Json::Array::add(pArrayHandRaiseTimeJson, pTmpJson);
                }
                break;
            }
            /* 玩手机 */
            case Pos::PLAY_PHONE:
            {
                /* 玩手机意时间段 */
                for (auto& seg : kv.second.vecSegments)
                {
                    auto pTmpJson = Json::init();
                    Json::add(pTmpJson, "Start", seg.nStartTime);
                    Json::add(pTmpJson, "End", seg.nEndTime);
                    Json::Array::add(pArrayHandPlayPhoneTimeJson, pTmpJson);
                }
                break;
            }
            /* 接打电话 */
            case Pos::CALL_PHONE:
            {
                /* 接打电话时间段 */
                for (auto& seg : kv.second.vecSegments)
                {
                    auto pTmpJson = Json::init();
                    Json::add(pTmpJson, "Start", seg.nStartTime);
                    Json::add(pTmpJson, "End", seg.nEndTime);
                    Json::Array::add(pArrayHandCallPhoneTimeJson, pTmpJson);
                }
                break;
            }
            /* 板书 */
            case Pos::TEABOARD:
            {
                /* 板书时间段 */
                for (auto& seg : kv.second.vecSegments)
                {
                    auto pTmpJson = Json::init();
                    Json::add(pTmpJson, "Start", seg.nStartTime);
                    Json::add(pTmpJson, "End", seg.nEndTime);
                    Json::Array::add(pArrayHandTeaboardTimeJson, pTmpJson);
                }
                break;
            }
            default:
            {
                break;
            }
        }
    }

    Json::add(pPostureSectionInfoJson, "OtherTime", pOtherTimeArrayJson);
    Json::add(pPostureSectionInfoJson, "FrontExplainTime", pArrayFrontExplainTimeJson);
    Json::add(pPostureSectionInfoJson, "ArmsCrossedTime", pArrayArmsCrossedTimeJson);
    Json::add(pPostureSectionInfoJson, "FacingBlackboardTime", pArrayFacingBlackboardTimeJson);
    Json::add(pPostureSectionInfoJson, "HandsOnWaistPercentTime", pArrayHandsOnWaistPercentTimeJson);
    Json::add(pPostureSectionInfoJson, "HeadDownTime", pArrayHeadDownTimeJson);
    Json::add(pPostureSectionInfoJson, "HeadUpTime", pArrayHeadUpTimeJson);
    Json::add(pPostureSectionInfoJson, "HandRaiseTime", pArrayHandRaiseTimeJson);
    Json::add(pDataJson, "PostureSectionInfo", pPostureSectionInfoJson);


    /* 教师轨迹数组 */
    auto pTrackArrayJson = Json::Array::init();
    for (auto& p : getHeatMap())
    {
        auto pTmpJson = Json::init();
        Json::add(pTmpJson, "X", p.nX);
        Json::add(pTmpJson, "Y", p.nY);
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
    ToolFunc::writeDataToFile(
        strFilePath.c_str(),
        pchJsonData,
        strlen(pchJsonData));

    /* 释放空间 */
    Json::release(pchJsonData);
    pchJsonData = nullptr;
}