/**
 * @FilePath     : classroom_task.cpp
 * @Author       : leiyy
 * @Date         : 2026-10-26 10:17:00
 * @LastEditors  : leiyy
 * @LastEditTime : 2026-10-28 13:57:00
 * @Description  : 课堂 AI 分析配置下发接口的 Task 实现
 */

#include "classroom_task.h"

#include <string>

#if defined(CAP_TV3881TJY_CLASSROOM_CONFIG) && CAP_TV3881TJY_CLASSROOM_CONFIG

/* 与课堂网关的桥接 */
extern bool classroomConfigPageGet(const char *pszFeature, std::string &rstrResp);
extern bool classroomConfigPageSet(const char *pszFeature, const std::string &strPartial,
                                   std::string &rstrResp);
extern std::string classroomConfigReport();

/* 把"取某一页配置 / 合入某一页配置"展开成 handle() */
#define CLASSROOM_CONFIG_PAGE_IMPL_GET(ClassName, Wire)                                \
void Task::Classroom::ClassName::handle()                                              \
{                                                                                      \
    std::string strResp;                                                               \
    if (!classroomConfigPageGet(Wire, strResp))                                        \
    {                                                                                  \
        /* 失败也把原因带回去：strResp 是网关拼好的 {ok:false,error}，                  \
           丢掉它网页就只能看到 Return=-1 而不知道哪一项不对 */                        \
        result(strResp, -1);                                                           \
        return;                                                                        \
    }                                                                                  \
    result(strResp);                                                                   \
}

#define CLASSROOM_CONFIG_PAGE_IMPL_SET(ClassName, Wire)                                \
void Task::Classroom::ClassName::handle()                                              \
{                                                                                      \
    std::string strResp;                                                               \
    if (!classroomConfigPageSet(Wire, m_taskData, strResp))                            \
    {                                                                                  \
        /* 同上：拒绝原因必须回到网页，否则"保存失败"没有可查的线索 */                  \
        result(strResp, -1);                                                           \
        return;                                                                        \
    }                                                                                  \
    result(strResp);                                                                   \
}

/* ----  摄像机类型 ---- */
CLASSROOM_CONFIG_PAGE_IMPL_GET(GetDeviceRole,    "device_role")
CLASSROOM_CONFIG_PAGE_IMPL_SET(SetDeviceRole,    "device_role")
/* ----  学生机行为分析 ---- */
CLASSROOM_CONFIG_PAGE_IMPL_GET(GetStuStand,      "stu_stand")
CLASSROOM_CONFIG_PAGE_IMPL_SET(SetStuStand,      "stu_stand")
CLASSROOM_CONFIG_PAGE_IMPL_GET(GetStuRaiseHand,  "stu_raise_hand")
CLASSROOM_CONFIG_PAGE_IMPL_SET(SetStuRaiseHand,  "stu_raise_hand")
CLASSROOM_CONFIG_PAGE_IMPL_GET(GetStuHeadUp,     "stu_head_up")
CLASSROOM_CONFIG_PAGE_IMPL_SET(SetStuHeadUp,     "stu_head_up")
CLASSROOM_CONFIG_PAGE_IMPL_GET(GetStuHeadDown,   "stu_head_down")
CLASSROOM_CONFIG_PAGE_IMPL_SET(SetStuHeadDown,   "stu_head_down")
CLASSROOM_CONFIG_PAGE_IMPL_GET(GetStuTurnBack,   "stu_turn_back")
CLASSROOM_CONFIG_PAGE_IMPL_SET(SetStuTurnBack,   "stu_turn_back")
CLASSROOM_CONFIG_PAGE_IMPL_GET(GetStuPhone,      "stu_phone")
CLASSROOM_CONFIG_PAGE_IMPL_SET(SetStuPhone,      "stu_phone")
CLASSROOM_CONFIG_PAGE_IMPL_GET(GetStuListen,     "stu_listen")
CLASSROOM_CONFIG_PAGE_IMPL_SET(SetStuListen,     "stu_listen")
CLASSROOM_CONFIG_PAGE_IMPL_GET(GetStuReading,    "stu_reading")
CLASSROOM_CONFIG_PAGE_IMPL_SET(SetStuReading,    "stu_reading")
CLASSROOM_CONFIG_PAGE_IMPL_GET(GetStuDiscuss,    "stu_discuss")
CLASSROOM_CONFIG_PAGE_IMPL_SET(SetStuDiscuss,    "stu_discuss")
CLASSROOM_CONFIG_PAGE_IMPL_GET(GetStuExpression, "stu_expression")
CLASSROOM_CONFIG_PAGE_IMPL_SET(SetStuExpression, "stu_expression")
/* ----  课堂考勤 ---- */
CLASSROOM_CONFIG_PAGE_IMPL_GET(GetAttendance,    "attendance")
CLASSROOM_CONFIG_PAGE_IMPL_SET(SetAttendance,    "attendance")
CLASSROOM_CONFIG_PAGE_IMPL_GET(GetFaceDetect,    "face_detect")
CLASSROOM_CONFIG_PAGE_IMPL_SET(SetFaceDetect,    "face_detect")
/* ----  教师机行为分析（指导学生/师生互动/巡视）---- */
CLASSROOM_CONFIG_PAGE_IMPL_GET(GetTeaGuiding,    "tea_guiding")
CLASSROOM_CONFIG_PAGE_IMPL_SET(SetTeaGuiding,    "tea_guiding")
CLASSROOM_CONFIG_PAGE_IMPL_GET(GetTeaInteract,   "tea_interact")
CLASSROOM_CONFIG_PAGE_IMPL_SET(SetTeaInteract,   "tea_interact")
CLASSROOM_CONFIG_PAGE_IMPL_GET(GetTeaPatrol,     "tea_patrol")
CLASSROOM_CONFIG_PAGE_IMPL_SET(SetTeaPatrol,     "tea_patrol")
/* ----  教师机行为分析（站立/抬头/低头/手机/讲授/板书/讲台/OCR）---- */
CLASSROOM_CONFIG_PAGE_IMPL_GET(GetTeaStand,      "tea_stand")
CLASSROOM_CONFIG_PAGE_IMPL_SET(SetTeaStand,      "tea_stand")
CLASSROOM_CONFIG_PAGE_IMPL_GET(GetTeaHeadUp,     "tea_head_up")
CLASSROOM_CONFIG_PAGE_IMPL_SET(SetTeaHeadUp,     "tea_head_up")
CLASSROOM_CONFIG_PAGE_IMPL_GET(GetTeaHeadDown,   "tea_head_down")
CLASSROOM_CONFIG_PAGE_IMPL_SET(SetTeaHeadDown,   "tea_head_down")
CLASSROOM_CONFIG_PAGE_IMPL_GET(GetTeaPhone,      "tea_phone")
CLASSROOM_CONFIG_PAGE_IMPL_SET(SetTeaPhone,      "tea_phone")
CLASSROOM_CONFIG_PAGE_IMPL_GET(GetTeaTeaching,   "tea_teaching")
CLASSROOM_CONFIG_PAGE_IMPL_SET(SetTeaTeaching,   "tea_teaching")
CLASSROOM_CONFIG_PAGE_IMPL_GET(GetTeaBoardWrite, "tea_board_write")
CLASSROOM_CONFIG_PAGE_IMPL_SET(SetTeaBoardWrite, "tea_board_write")
CLASSROOM_CONFIG_PAGE_IMPL_GET(GetEnterPodium,   "enter_podium")
CLASSROOM_CONFIG_PAGE_IMPL_SET(SetEnterPodium,   "enter_podium")
CLASSROOM_CONFIG_PAGE_IMPL_GET(GetBoardOcr,      "board_ocr")
CLASSROOM_CONFIG_PAGE_IMPL_SET(SetBoardOcr,      "board_ocr")
/* ----  学生机行为分析（上讲台/演示）---- */
CLASSROOM_CONFIG_PAGE_IMPL_GET(GetStuPodium,     "stu_podium")
CLASSROOM_CONFIG_PAGE_IMPL_SET(SetStuPodium,     "stu_podium")
CLASSROOM_CONFIG_PAGE_IMPL_GET(GetStuPresent,    "stu_present")
CLASSROOM_CONFIG_PAGE_IMPL_SET(SetStuPresent,    "stu_present")

/* ----  课程分析报告（只读）---- */
void Task::Classroom::GetCourseReport::handle()
{
    result(classroomConfigReport());
}

#endif /* CAP_TV3881TJY_CLASSROOM_CONFIG */
