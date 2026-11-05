/**
 * @FilePath     : classroom_task.h
 * @Author       : leiyy
 * @Date         : 2026-10-26 10:17:00
 * @LastEditors  : leiyy
 * @LastEditTime : 2026-10-28 13:57:00
 * @Description  : 课堂 AI 分析配置下发接口
 */

#pragma once

#if defined(CAP_TV3881TJY_CLASSROOM_CONFIG) && CAP_TV3881TJY_CLASSROOM_CONFIG

#include <string>

/* 同目录的 Task 基类包装（TaskSubClass 宏 + CTask） */
#include "task_sub_class.h"

namespace Task
{
    namespace Classroom
    {
        /* ============================ 摄像机类型 ============================ */
        TaskSubClass(GetDeviceRole)
        TaskSubClass(SetDeviceRole)

        /* ======================= 学生机行为分析======================== */
        /* 站立检测：行为列 101 */
        TaskSubClass(GetStuStand)
        TaskSubClass(SetStuStand)
        /* 举手检测：行为列 102 */
        TaskSubClass(GetStuRaiseHand)
        TaskSubClass(SetStuRaiseHand)
        /* 抬头检测：行为列 103 */
        TaskSubClass(GetStuHeadUp)
        TaskSubClass(SetStuHeadUp)
        /* 低头检测：行为列 104 */
        TaskSubClass(GetStuHeadDown)
        TaskSubClass(SetStuHeadDown)
        /* 背身检测：行为列 105 */
        TaskSubClass(GetStuTurnBack)
        TaskSubClass(SetStuTurnBack)
        /* 使用手机检测：行为列 106 */
        TaskSubClass(GetStuPhone)
        TaskSubClass(SetStuPhone)
        /* 听讲检测：行为列 107 */
        TaskSubClass(GetStuListen)
        TaskSubClass(SetStuListen)
        /* 阅读检测：行为列 108 */
        TaskSubClass(GetStuReading)
        TaskSubClass(SetStuReading)
        /* 讨论检测：行为列 109 */
        TaskSubClass(GetStuDiscuss)
        TaskSubClass(SetStuDiscuss)
        /* 表情识别：行为列 112 */
        TaskSubClass(GetStuExpression)
        TaskSubClass(SetStuExpression)

        /* ========================== 课堂考勤========================== */
        /* 考勤人数统计：attendance 块 */
        TaskSubClass(GetAttendance)
        TaskSubClass(SetAttendance)
        /* 学生人脸检测：gallery + align 两块联动 */
        TaskSubClass(GetFaceDetect)
        TaskSubClass(SetFaceDetect)

        /* ======================= 教师机行为分析======================== */
        /* 指导学生：teacher.stay_evidence_ms 一族 */
        TaskSubClass(GetTeaGuiding)
        TaskSubClass(SetTeaGuiding)
        /* 师生互动提示：行为列 209 */
        TaskSubClass(GetTeaInteract)
        TaskSubClass(SetTeaInteract)
        /* 教师巡视：行为列 210 */
        TaskSubClass(GetTeaPatrol)
        TaskSubClass(SetTeaPatrol)

        /* ======================= 教师机行为分析======================== */
        /* 站立检测（教师）：行为列 201 */
        TaskSubClass(GetTeaStand)
        TaskSubClass(SetTeaStand)
        /* 抬头检测（教师）：行为列 202 */
        TaskSubClass(GetTeaHeadUp)
        TaskSubClass(SetTeaHeadUp)
        /* 低头检测（教师）：行为列 203 */
        TaskSubClass(GetTeaHeadDown)
        TaskSubClass(SetTeaHeadDown)
        /* 使用手机检测（教师）：行为列 205 */
        TaskSubClass(GetTeaPhone)
        TaskSubClass(SetTeaPhone)
        /* 教师讲授：行为列 206 */
        TaskSubClass(GetTeaTeaching)
        TaskSubClass(SetTeaTeaching)
        /* 书写板书：行为列 208 */
        TaskSubClass(GetTeaBoardWrite)
        TaskSubClass(SetTeaBoardWrite)
        /* 进入讲台：行为列 211 */
        TaskSubClass(GetEnterPodium)
        TaskSubClass(SetEnterPodium)
        /* 黑板内容识别：board 块（OCR） */
        TaskSubClass(GetBoardOcr)
        TaskSubClass(SetBoardOcr)

        /* ======================= 学生机行为分析======================== */
        /* 上讲台检测：行为列 110 */
        TaskSubClass(GetStuPodium)
        TaskSubClass(SetStuPodium)
        /* 演示检测：行为列 111 */
        TaskSubClass(GetStuPresent)
        TaskSubClass(SetStuPresent)

        /* ==========================  课程分析报告 ========================== */
        TaskSubClass(GetCourseReport)
    } /* namespace Classroom end */
} /* namespace Task end */


#endif /* CAP_TV3881TJY_CLASSROOM_CONFIG */
