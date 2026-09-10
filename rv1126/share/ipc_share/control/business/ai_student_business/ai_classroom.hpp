/***
 * @FilePath     : ai_classroom.hpp
 * @Author       : zhengxh (zhengxh@kfb.cn)
 * @Date         : 2026-04-03 10:00:00
 * @LastEditors  : zhengxh
 * @LastEditTime : 2026-04-03 14:00:00
 * @Description  : 班级信息管理，内部监控线程根据课程时间自动激活/停用考勤与行为分析计算器。
 */
#pragma once
#ifdef ENABLE_AI_STUDENT

#include "ai_student_define.h"
#include "attendance_calculator.hpp"
#include "behavior_calculator.hpp"
#include <thread>
#include <atomic>
#include <mutex>
#include <condition_variable>
#include <shared_mutex>

namespace AiStudentBusiness_NS {

/* 考勤提前激活时间（秒），课程开始前该时间即启用考勤分析 */
static constexpr int64_t COURSE_PRE_START_S = 5 * 60;

class CAiClassRoom {
public:
    CAiClassRoom() = default;
    ~CAiClassRoom();

    /**
     * @brief      : 处理一帧人头数统计，仅在课程激活期间有效，转发给出勤计算器。
     * @author     : zhengxh (zhengxh@kfb.cn)
     * @param [in] : vecFaceeatures  当前帧检测到的人脸信息
     * @return     : true 表示成功，false 表示课程未激活或参数非法
     */
    bool handle(std::vector<AiStudentBusiness_NS::FaceInfo_t> &vecFaceeatures);

    /**
     * @brief      : 处理一帧行为分析结果，仅在课程激活期间有效，转发给行为分析计算器。
     * @author     : zhengxh (zhengxh@kfb.cn)
     * @param [in] : nCount  当前帧检测到的人头数，必须 >= 0
     * @param [in] : vBehaviors  当前帧每个学生的行为检测结果
     * @return     : true 表示成功，false 表示课程未激活或参数非法
     */
    bool handle(int nCount, const std::vector<StudentBehavior_NS::Behavior_S> &vBehaviors);

    /**
     * @brief      : 设置班级信息并启动课程监控线程。
     *               可重复调用（配置文件更新时），内部会先停止旧线程再重新启动。
     * @author     : zhengxh (zhengxh@kfb.cn)
     * @param [in] : stClassInfo  班级信息，包含教师、学生列表和课程列表
     * @return     : true 表示成功
     */
    bool setClassInfo(ClassInfo stClassInfo);

    /**
     * @brief       : 获取当前班级信息。
     * @author      : zhengxh (zhengxh@kfb.cn)
     * @param [out] : stClassInfo  班级信息
     * @return      : true 表示成功
     */
    bool getClassInfo(ClassInfo &stClassInfo) const;

    /**
     * @brief       : 获取本次课程的考勤记录，包含班级 ID、日期及出勤汇总。
     * @author      : zhengxh (zhengxh@kfb.cn)
     * @param [out] : stRecord  考勤记录
     * @return      : true 表示成功
     */
    bool getAttendanceRecord(AttendanceRecord &stRecord) const;

    /**
     * @brief       : 获取本次课程的行为分析记录
     * @author      : zhengxh (zhengxh@kfb.cn)
     * @param [out] : stRecord  行为分析记录
     * @return      : true 表示成功
     */
    bool getStudentBehaviorRecord(StudentBehaviorRecord &stRecord) const;

    /**
     * @brief       : 获取本次课程的课堂表现。
     * @author      : zhengxh (zhengxh@kfb.cn)
     * @param [out] : stRecord  课堂表现
     * @return      : true 表示成功
     */
    bool getStudentPerformanceRecord(StudentPerformanceRecord &stRecord) const;

private:
    /**
     * @brief  : 课程监控线程入口，定期检查当前时间是否命中课程时间范围。
     * @author : zhengxh (zhengxh@kfb.cn)
     */
    void monitorThread();

    /**
     * @brief      : 激活指定课程，初始化考勤和行为分析计算器。
     *               调用方必须持有 m_mutex 写锁。
     * @author     : zhengxh (zhengxh@kfb.cn)
     * @param [in] : stCourse  要激活的课程信息
     */
    void activateCourse(const Course &stCourse);

    /**
     * @brief  : 停用当前课程，重置考勤和行为分析计算器。
     *           调用方必须持有 m_mutex 写锁。
     * @author : zhengxh (zhengxh@kfb.cn)
     */
    void deactivateCourse();

    /**
     * @brief  : 停止监控线程并等待其退出。
     * @author : zhengxh (zhengxh@kfb.cn)
     */
    void stopMonitor();

    /**
     * @brief      : 行为事件回调处理，由行为分析计算器在满足持续时长阈值时调用。
     * @author     : zhengxh (zhengxh@kfb.cn)
     * @param [in] : stRecord  触发的行为记录
     */
    void onBehaviorTriggered(const BehaviorRecord stRecord);

    /**
     * @brief      : 考勤回调处理，由考勤分析计算器在满足条件时时调用。
     * @author     : zhengxh (zhengxh@kfb.cn)
     * @param [in] : stRecord  触发的考勤记录
     */
    void onAttendanceTriggered(const AttendanceSummary stRecord);

private:
    mutable std::shared_mutex m_mutex;
    ClassInfo                 m_stClassInfo;

    CAttendanceCalculator m_attendanceCalc;
    CBehaviorCalculator   m_behaviorCalc;

    /* 课程监控线程 */
    std::thread             m_thMonitor;
    std::atomic<bool>       m_bRunning{false};
    std::mutex              m_cvMutex;
    std::condition_variable m_cv;

    /* 当前课程激活状态 */
    std::atomic<bool> m_bCourseActive{false};
    int               m_nActiveCourseIdx = -1;
};

} // namespace AiStudentBusiness_NS
 
#endif