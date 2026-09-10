/**
 * @FilePath     : ai_student_define.h
 * @Author       : zhengxh@kfb.cn
 * @Date         : 2026-04-03 11:29:33
 * @LastEditors  : zhengxh@kfb.cn
 * @LastEditTime : 2026-04-03 11:29:33
 * @Description  : ai student 配置数据结果
 */

#pragma once
#ifdef ENABLE_AI_STUDENT

#include <string>
#include <vector>
#include "common_define.h"


namespace AiStudentBusiness_NS {

/**
 * @brief 学生实体
 */
struct Student {
    std::string id;    ///< 学生唯一标识
    std::string name;  ///< 学生姓名
    std::vector<float> vecFaceFeature; ///< 学生人脸特征
};

/**
 * @brief 教师实体
 */
struct Teacher {
    std::string id;      ///< 教师唯一标识
    std::string name;    ///< 教师姓名
    std::string subject; ///< 所教科目
};

/**
 * @brief 课程实体
 */
struct Course {
    std::string id;                     /* 课程唯一标识 */
    std::string name;                   /* 课程名称 - "军事科学" */ 
    int64_t nStart;                     /* 开始时间 - 秒级时间 */
    int64_t nDurtion;                   /* 课程时长 */
    std::string strStart;               /* 开始时间 - YYYY-MM-DD HH:MM:SS */
    Teacher teacher;                    /* 班级教师 */
};

/**
 * @brief 班级信息，聚合教师、学生列表、课程列表
 */
struct ClassInfo {
    std::string classId;              ///< 班级唯一标识
    std::string className;            /* 班级名称 - "临床医学专业（大一1）班" */
    std::vector<Student> students;    ///< 学生列表
    std::vector<Course>  courses;     ///< 课程列表
};

/******************************* 考勤相关 *********************************/
/**
 * @brief 考勤状态枚举
 */
enum class AttendanceStatus {
    Present,    ///< 出勤
    Late,       ///< 迟到
    Leave,      ///< 请假
    EarlyLeave, ///< 早退
    Absent      ///< 缺勤
};

/**
 * @brief 考勤汇总统计
 */
 struct AttendanceSummary {

    int total      = 0; ///< 总人数
    // int expectedAttendance = 0; // 应到人数 
    int nActualPeople = 0; /* 实际统计到的人数 */
    int present    = 0; ///< 出席人数
    int late       = 0; ///< 迟到人数
    int leave      = 0; ///< 请假人数
    int earlyLeave = 0; ///< 早退人数
    int absent     = 0; ///< 缺勤人数
};

/**
 * @brief 考勤记录
 */
struct AttendanceRecord {
    std::string classId;                /* 班级 ID */
    std::string courseId;               /* 课程 ID */
    std::string date;                   /* 日期，格式 YYYY-MM-DD */
    AttendanceSummary stAttendanceInfo; /* 考勤信息 */
};



/******************************* 行为分析相关 *********************************/
/* 行为类型枚举 */
typedef enum _PlatformBehaviorType_
{
    LISTEN_TO_TALK                   = 1, /* 听讲 */
    PRAXIS                           = 2, /* 实践 */
    DEMONSTRATION                    = 3, /* 演示 */
    READ                             = 4, /* 阅读 */
    DISCUSSION                       = 5, /* 讨论 */
    MOOD_SWING                       = 6, /* 情绪波动 */
    MOOD_CURVE                       = 7, /* 情绪曲线 */
} PlatformBehaviorType_E;

/**
 * @brief 单条学生行为分析记录
 *
 * 各行为字段，表示班级行为在采样时间内的整体状态。
 */
struct BehaviorRecord {
    bool bState;                            /* 状态：开始/结束 */
    std::string strTriggerTime;               /* 本节课开始到当前时刻的经过时间，格式 HH:MM:SS */
    PlatformBehaviorType_E eBehaviorType;    /* 学生行为类型 */
};

/* 学生课堂表现 */
struct StudentPerformance
{
    int nFocusAverage;                      /* 专注度均值 */
    std::string strFocusData;               /* 注意力集中阶段 */
    std::string strDistractionData;         /* 注意力涣散阶段 */
    std::string strEngagementReport;        /* 学生活跃度分析 */
};

struct StudentBehaviorRecord
{
    std::string classId;
    std::string courseId;
    std::vector<BehaviorRecord> behaviorTimeline;
};

struct StudentPerformanceRecord
{
    std::string classId;
    std::string courseId;
    StudentPerformance performance;
};

/* 人脸信息 */
typedef struct _FaceFeatureInfo_
{
    std::vector<float> vecFaceFeatureData; /* 人脸特征 */
    Common::RectInfo_S stFaceRect;         /* 人脸坐标 */
    std::string strStuName;                /* 人脸对应名字 */
    float fSimilarity;                     /* 人脸相似度 */
}FaceInfo_t;


} // namespace AiStudentBusiness_NS

#endif