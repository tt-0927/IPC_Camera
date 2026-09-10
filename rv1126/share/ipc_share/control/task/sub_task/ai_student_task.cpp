/**
 * @FilePath     : ai_student_task.cpp
 * @Author       : zhengxh@kfb.cn
 * @Date         : 2026-04-03 09:49:56
 * @LastEditors  : zhengxh@kfb.cn
 * @LastEditTime : 2026-04-03 09:49:56
 * @Description  : 学生行为分析任务
 */
#ifdef ENABLE_AI_STUDENT

#include "ai_student_task.h"
#include "ai_student_define.h"
#include "convert_interface.h"
#include "ai_student_business.hpp"

/*获取班级信息*/
void Task::AI_STUDENT::GetClassInfo::handle()
{
    AiStudentBusiness_NS::ClassInfo stClassInfo;
    AiStudentBusiness_NS::CAiStudentBusiness::instance()->getClassInfo(stClassInfo);
    std::string strResult = Convert::to_string(stClassInfo);
    result(strResult);
}

/*设置班级信息*/
void Task::AI_STUDENT::SetClassInfo::handle()
{
    AiStudentBusiness_NS::ClassInfo stClassInfo;
    Convert::to_struct(m_taskData, stClassInfo);
    Convert::write_file(AI_STUDENT_CLASS_CONFIG_FILE, stClassInfo);
    result(AiStudentBusiness_NS::CAiStudentBusiness::instance()->init());
}

void Task::AI_STUDENT::GetAttendanceRecordInfo::handle()
{
    AiStudentBusiness_NS::AttendanceRecord stRecord;
    AiStudentBusiness_NS::CAiStudentBusiness::instance()->getAttendanceRecord( stRecord);
    std::string strResult = Convert::to_string(stRecord);
    result(strResult);
}

void Task::AI_STUDENT::GetStudentBehaviorInfo::handle()
{
    AiStudentBusiness_NS::StudentBehaviorRecord stRecord;
    AiStudentBusiness_NS::CAiStudentBusiness::instance()->getStudentBehaviorRecord(stRecord);
    std::string strResult = Convert::to_string(stRecord);
    result(strResult);
}

void Task::AI_STUDENT::GetStudentFerformanceInfo::handle()
{
    AiStudentBusiness_NS::StudentPerformanceRecord stRecord;
    AiStudentBusiness_NS::CAiStudentBusiness::instance()->getStudentPerformanceRecord(stRecord);
    std::string strResult = Convert::to_string(stRecord);
    result(strResult);
}

#endif