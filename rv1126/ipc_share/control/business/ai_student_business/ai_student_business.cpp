/**
 * @FilePath     : ai_student_business.cpp
 * @Author       : zhengxh@kfb.cn
 * @Date         : 2026-04-02 20:24:10
 * @LastEditors  : zhengxh@kfb.cn
 * @LastEditTime : 2026-04-03 14:00:00
 * @Description  : 学生行为分析事务类
 */

#ifdef ENABLE_AI_STUDENT
#include "ai_student_business.hpp"
#include "ai_classroom.hpp"
#include "dlog.h"
#include <memory>
#include "time_utils.h"
#include "ai_student_define.h"
#include "convert_interface.h"
#include <string>

using namespace AiStudentBusiness_NS;

bool CAiStudentBusiness::init()
{
    std::string class_info_path = AI_STUDENT_CLASS_CONFIG_FILE;
    ClassInfo   stClassInfo;
    if (Convert::read_file(class_info_path, stClassInfo))
    {
        stClassInfo.classId   = "default";
        stClassInfo.className = "";
    }

    m_pClassRoom = std::make_shared<CAiClassRoom>();
    if (!m_pClassRoom)
    {
        dlog_error("班级管理类创建失败");
        return false;
    }
    m_pClassRoom->setClassInfo(stClassInfo);

    return true;
}

bool CAiStudentBusiness::deinit()
{
    m_pClassRoom = nullptr;
    return true;
}

void CAiStudentBusiness::Handle(std::vector<AiStudentBusiness_NS::FaceInfo_t> &vecFaceeatures)
{
    if (m_pClassRoom)
    {
        /* 人脸考勤以录入到数据库的人脸作为总人数，模型识别到能和记录的人脸匹配上的人数作为出勤人数 */
        m_pClassRoom->handle(vecFaceeatures);
    }
}

void CAiStudentBusiness::Handle(std::vector<StudentBehavior_NS::Result_S> vst, StudentBehavior_NS::OutData_S stOutData)
{
    if (m_pClassRoom)
    {
        // /* 先处理人头统计（出勤），再处理行为分析 */
        // m_pClassRoom->handle(static_cast<int>(vst.size())); 
        /* 学生行为分析已模型识别到的所有人数为总人数 */
        m_pClassRoom->handle(vst.size(), stOutData.vStBehavior);
    }
}

bool CAiStudentBusiness::getClassInfo(ClassInfo &stClassInfo)
{
    if (!m_pClassRoom)
    {
        return false;
    }

    return m_pClassRoom->getClassInfo(stClassInfo);
}

bool CAiStudentBusiness::updateClassInfo(ClassInfo &stClassInfo)
{
    if (!m_pClassRoom)
    {
        return false;
    }

    return m_pClassRoom->setClassInfo(stClassInfo);
}

bool CAiStudentBusiness::getAttendanceRecord(AttendanceRecord &stRecord)
{
    if (!m_pClassRoom)
    {
        dlog_error("班级未初始化");
        return false;
    }
    return m_pClassRoom->getAttendanceRecord(stRecord);
}

bool CAiStudentBusiness::getStudentBehaviorRecord(StudentBehaviorRecord &stRecord)
{
    if (!m_pClassRoom)
    {
        return false;
    }

    return m_pClassRoom->getStudentBehaviorRecord(stRecord);
}

bool CAiStudentBusiness::getStudentPerformanceRecord(StudentPerformanceRecord &stRecord)
{
    if (!m_pClassRoom)
    {
        return false;
    }

    return m_pClassRoom->getStudentPerformanceRecord(stRecord);
}

bool CAiStudentBusiness::isDetect()
{
    int now      = TimeUtils_NS::get_currentTimestampS();
    int duration = now - m_nLastTime;
    if (duration > 1)
    {
        m_nLastTime = now;
        return true;
    }
    return false;
}
#endif