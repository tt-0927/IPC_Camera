/***
 * @FilePath     : ai_classroom.cpp
 * @Author       : zhengxh (zhengxh@kfb.cn)
 * @Date         : 2026-04-03 10:00:00
 * @LastEditors  : zhengxh
 * @LastEditTime : 2026-04-03 14:00:00
 * @Description  : 班级信息管理实现，内部监控线程根据课程时间自动激活/停用考勤与行为分析计算器。
 */

#ifdef ENABLE_AI_STUDENT
#include "ai_classroom.hpp"
#include "time_utils.h"
#include <cstdio>
#include "Json.h"
#include "convert_interface.h"
#include "action_code.h"
#include "task_publish.h"
#include "ai_student_define.h"

using namespace AiStudentBusiness_NS;

/* 监控线程轮询间隔（秒） */
static constexpr int MONITOR_INTERVAL_S = 1;

CAiClassRoom::~CAiClassRoom()
{
    stopMonitor();
}

/* 人脸考勤 */
bool CAiClassRoom::handle(std::vector<AiStudentBusiness_NS::FaceInfo_t> &vecFaceeatures)
{
    if (!m_bCourseActive.load())
    {
        return false;
    }
    return m_attendanceCalc.addSample(vecFaceeatures, m_stClassInfo.students);
}

/* 行为分析 */
bool CAiClassRoom::handle(int nCount, const std::vector<StudentBehavior_NS::Behavior_S> &vBehaviors)
{
    if (!m_bCourseActive.load())
    {
        return false;
    }

    return m_behaviorCalc.handle(vBehaviors, nCount);
}

bool CAiClassRoom::setClassInfo(ClassInfo stClassInfo)
{
    /* 重复调用时先停止旧的监控线程 */
    stopMonitor();

    {
        std::unique_lock<std::shared_mutex> lock(m_mutex);
        m_stClassInfo = std::move(stClassInfo);
    }

    /* 启动课程监控线程 */
    m_bRunning.store(true);
    m_thMonitor = std::thread(&CAiClassRoom::monitorThread, this);
    return true;
}

bool CAiClassRoom::getClassInfo(ClassInfo &stClassInfo) const
{
    std::shared_lock<std::shared_mutex> lock(m_mutex);
    stClassInfo = m_stClassInfo;
    return true;
}

bool CAiClassRoom::getAttendanceRecord(AttendanceRecord &stRecord) const
{
    std::shared_lock<std::shared_mutex> lock(m_mutex);
    stRecord.classId = m_stClassInfo.classId;
    stRecord.date    = TimeUtils_NS::get_currentDateWithDash();
    if (!m_bCourseActive.load())
    {
        dlog_error("当前未有课程");
    }
    m_attendanceCalc.getSummary(stRecord.stAttendanceInfo);
    return true;
}

bool CAiClassRoom::getStudentBehaviorRecord(StudentBehaviorRecord &stRecord) const
{
    std::shared_lock<std::shared_mutex> lock(m_mutex);
    stRecord.classId  = m_stClassInfo.classId;
    stRecord.courseId = m_stClassInfo.courses.empty() ? "" : m_stClassInfo.courses[0].id;
    return m_behaviorCalc.getBehaviorTimeline(stRecord.behaviorTimeline);
}

bool CAiClassRoom::getStudentPerformanceRecord(StudentPerformanceRecord &stRecord) const
{
    std::shared_lock<std::shared_mutex> lock(m_mutex);
    stRecord.classId  = m_stClassInfo.classId;
    stRecord.courseId = m_stClassInfo.courses.empty() ? "" : m_stClassInfo.courses[0].id;
    return m_behaviorCalc.getPerformance(stRecord.performance);
}

void CAiClassRoom::monitorThread()
{
    while (m_bRunning.load())
    {
        int64_t nNow = TimeUtils_NS::get_currentTimestampS();

        {
            std::unique_lock<std::shared_mutex> lock(m_mutex);

            /* 查找当前时间命中的课程（含提前激活窗口） */
            int nHitIdx = -1;
            for (int i = 0; i < static_cast<int>(m_stClassInfo.courses.size()); ++i)
            {
                const Course &stCourse    = m_stClassInfo.courses[i];
                int64_t       nEarlyStart = stCourse.nStart - COURSE_PRE_START_S;
                int64_t       nEnd        = stCourse.nStart + stCourse.nDurtion;
                // dlog_debug("now %lld %lld-%lld\n", nNow, nEarlyStart, nEnd);

                if (nNow >= nEarlyStart && nNow < nEnd)
                {
                    nHitIdx = i;
                    break;
                }
            }

            /* 课程从无到有：激活 */
            if (nHitIdx >= 0 && !m_bCourseActive.load())
            {
                activateCourse(m_stClassInfo.courses[nHitIdx]);
                m_nActiveCourseIdx = nHitIdx;
            }
            /* 课程从有到无：停用并重置 */
            else if (nHitIdx < 0 && m_bCourseActive.load())
            {
                deactivateCourse();
            }
        }

        /* 等待下一次轮询，支持提前唤醒以快速退出 */
        std::unique_lock<std::mutex> lock(m_cvMutex);
        m_cv.wait_for(lock, std::chrono::seconds(MONITOR_INTERVAL_S), [this] { return !m_bRunning.load(); });
    }
}

void CAiClassRoom::activateCourse(const Course &stCourse)
{
    int nTotal = static_cast<int>(m_stClassInfo.students.size()) + 1;
    /* 初始化考勤计算器 */
    m_attendanceCalc.reset();
    m_attendanceCalc.setTotal(nTotal);
    m_attendanceCalc.setOnAttendanceCallback(
        [this](const AttendanceSummary &stRecord) { onAttendanceTriggered(stRecord); });

    /* 初始化行为分析计算器 */
    m_behaviorCalc.reset();
    m_behaviorCalc.setTotal(nTotal);
    m_behaviorCalc.setCourseStartTime(stCourse.nStart);
    m_behaviorCalc.setCallback(
        [this](const BehaviorRecord &stRecord) { onBehaviorTriggered(stRecord); });

    m_bCourseActive.store(true);

    dlog_debug("[CAiClassRoom] 课程激活: %s", stCourse.name.c_str());
}

void CAiClassRoom::deactivateCourse()
{
    m_bCourseActive.store(false);

    m_attendanceCalc.reset();
    m_behaviorCalc.reset();
    m_nActiveCourseIdx = -1;

    dlog_debug("[CAiClassRoom] 课程结束，计算器已重置");
}

void CAiClassRoom::stopMonitor()
{
    if (!m_bRunning.load())
        return;

    m_bRunning.store(false);
    m_cv.notify_one();

    if (m_thMonitor.joinable())
    {
        m_thMonitor.join();
    }
    deactivateCourse();
}

void CAiClassRoom::onBehaviorTriggered(BehaviorRecord stRecord)
{
    // Json::Object *info = Json::init();
    // if (info)
    // {
    //     BehaviorRecord stInfo = stRecord;
    //     Convert::deal(info, stInfo, false);
    //     Json::Object *j_msg  = Json::init();
    //     Json::Object *j_data = Json::init();

    //     Json::add(j_data, "classId", m_stClassInfo.classId);
    //     Json::add(j_data, "courseId", "");
    //     Json::add(j_data, "behavior", info);

    //     Json::add(j_msg, "ActionCode", static_cast<int>(AC_GET_STUDENT_BEHAVIOR_INFO));
    //     Json::add(j_msg, "DeviceName", "");
    //     Json::add(j_msg, "UserName", "");
    //     Json::add(j_msg, "Return", 0);
    //     Json::add(j_msg, "Data", j_data);

    //     std::string msg = Json::to_string(j_msg);
    //     TaskPublish::instance()->message(AC_GET_STUDENT_BEHAVIOR_INFO, msg.c_str(), msg.size());
    //     Json::deinit(j_data);
    //     Json::deinit(j_msg);
    //     Json::deinit(info);
    // }

    TaskPublish::instance()->message(AC_GET_STUDENT_BEHAVIOR_INFO, Convert::to_string(stRecord));
}

void CAiClassRoom::onAttendanceTriggered(AttendanceSummary stRecord)
{
    // AttendanceRecord stInfo;
    // Json::Object    *info   = Json::init();
    // stInfo.classId          = m_stClassInfo.classId;
    // stInfo.date             = TimeUtils_NS::get_currentDateWithDash();
    // stInfo.stAttendanceInfo = stRecord;
    // Convert::deal(info, stInfo, false);
    // if (info)
    // {
    //     Json::Object *j_msg = Json::init();
    //     Json::add(j_msg, "ActionCode", static_cast<int>(AC_GET_ATTENDANCE_INFO));
    //     Json::add(j_msg, "DeviceName", "");
    //     Json::add(j_msg, "UserName", "");
    //     Json::add(j_msg, "Return", 0);
    //     Json::add(j_msg, "Data", info);
    //     std::string msg = Json::to_string(j_msg);
    //     TaskPublish::instance()->message(AC_GET_ATTENDANCE_INFO, msg.c_str(), msg.size());
    //     Json::deinit(info);
    //     Json::deinit(j_msg);
    // }
    TaskPublish::instance()->message(AC_GET_ATTENDANCE_INFO, Convert::to_string(stRecord));
}

#endif