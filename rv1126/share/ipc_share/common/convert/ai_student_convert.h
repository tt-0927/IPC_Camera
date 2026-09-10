/**
 * @FilePath     : ai_student_convert.h
 * @Author       : zhengxh@kfb.cn
 * @Date         : 2026-04-03 11:29:33
 * @LastEditors  : zhengxh@kfb.cn
 * @LastEditTime : 2026-04-03 11:29:33
 * @Description  : ai student 配置转换
 */
#ifdef ENABLE_AI_STUDENT

#pragma once
#include "Json.h"
#include <vector>
#include "ai_student_define.h"

namespace Convert
{
    /* 班级信息 */
    void deal(Json::Object *pRootJson, AiStudentBusiness_NS::Course &stInfo, bool bOutStruct);
    void deal(Json::Object *pRootJson, AiStudentBusiness_NS::Student &stInfo, bool bOutStruct);
    void deal(Json::Object *pRootJson, AiStudentBusiness_NS::Teacher &stInfo, bool bOutStruct);
    void deal(Json::Object *pRootJson, AiStudentBusiness_NS::ClassInfo &stInfo, bool bOutStruct);

    /* 考勤情况 */
    void deal(Json::Object *pRootJson, AiStudentBusiness_NS::AttendanceSummary &stInfo, bool bOutStruct);
    void deal(Json::Object *pRootJson, AiStudentBusiness_NS::AttendanceRecord &stInfo, bool bOutStruct);

    /* 行为分析 */
    void deal(Json::Object *pRootJson, AiStudentBusiness_NS::BehaviorRecord &stInfo, bool bOutStruct);
    void deal(Json::Object *pRootJson, AiStudentBusiness_NS::StudentBehaviorRecord &stInfo, bool bOutStruct);

    /* 课堂表现 */
    void deal(Json::Object *pRootJson, AiStudentBusiness_NS::StudentPerformance &stInfo, bool bOutStruct);
    void deal(Json::Object *pRootJson, AiStudentBusiness_NS::StudentPerformanceRecord &stInfo, bool bOutStruct);
}
#endif