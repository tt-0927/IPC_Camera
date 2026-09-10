/**
 * @FilePath     : ai_student_convert.cpp
 * @Author       : zhengxh@kfb.cn
 * @Date         : 2026-04-03 11:29:33
 * @LastEditors  : zhengxh@kfb.cn
 * @LastEditTime : 2026-04-03 11:29:33
 * @Description  : ai student 配置转换
 */

#ifdef ENABLE_AI_STUDENT

#include "ai_student_convert.h"
#include "convert.h"


void Convert::deal(Json::Object *pRootJson, AiStudentBusiness_NS::Course &stInfo, bool bOutStruct)
{
    if (!pRootJson) return;
    CConvert convert(bOutStruct);
    convert.field(pRootJson,     "id",   stInfo.id);
    convert.field(pRootJson,     "name", stInfo.name);
    convert.field(pRootJson,     "startTime",  stInfo.nStart);
    convert.field(pRootJson,     "sStartTime", stInfo.strStart);
    convert.field(pRootJson,     "duration",   stInfo.nDurtion);
    convert.structure(pRootJson, "teacher", stInfo.teacher);
}

void Convert::deal(Json::Object *pRootJson, AiStudentBusiness_NS::Student &stInfo, bool bOutStruct)
{
    if (!pRootJson) return;
    CConvert convert(bOutStruct);
    convert.field(pRootJson, "id",      stInfo.id);
    convert.field(pRootJson, "name",    stInfo.name);


    if(bOutStruct)
    {
        stInfo.vecFaceFeature.clear();
        Json::Object *pArray = Json::get(pRootJson, "FaceFeature");
        if (nullptr != pArray)
        {
    
            int nItemMax = Json::Array::size(pArray);
            for (int j = 0; j < nItemMax; j++)
            { 
                double val  = 0.0;
                Json::Value::get(Json::Array::get(pArray, j), val);            
                stInfo.vecFaceFeature.push_back(val);
            }
        }
    }
    else
    {
        auto pArray = Json::Array::init();
        if (!pArray)
        {
            return;
        }
        for (float v : stInfo.vecFaceFeature)
        {
            Json::Array::add(pArray, static_cast<float>(v));
        }
    
        Json::add(pRootJson, "FaceFeature", pArray);
    }

}

void Convert::deal(Json::Object *pRootJson, AiStudentBusiness_NS::Teacher &stTeacher, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }

    Convert::CConvert convert(bOutStruct);
    convert.field(pRootJson, "id", stTeacher.id);
    convert.field(pRootJson, "name", stTeacher.name);
    convert.field(pRootJson, "subject", stTeacher.subject);
}

void Convert::deal(Json::Object *pRootJson, AiStudentBusiness_NS::ClassInfo &stClassInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }

    Convert::CConvert convert(bOutStruct);
    convert.field(pRootJson, "classId", stClassInfo.classId);
    convert.field(pRootJson, "className", stClassInfo.className);
    convert.structure(pRootJson, "students", stClassInfo.students);
    convert.structure(pRootJson, "courses", stClassInfo.courses);
}

void Convert::deal(Json::Object *pRootJson, AiStudentBusiness_NS::AttendanceSummary &stInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }
    Convert::CConvert convert(bOutStruct);
    convert.field(pRootJson, "total",       stInfo.total);
    convert.field(pRootJson, "present",     stInfo.present);
    convert.field(pRootJson, "late",        stInfo.late);
    convert.field(pRootJson, "leave",       stInfo.leave);
    convert.field(pRootJson, "earlyLeave",  stInfo.earlyLeave);
    convert.field(pRootJson, "absent",      stInfo.absent);
}

void Convert::deal(Json::Object *pRootJson, AiStudentBusiness_NS::AttendanceRecord &stInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }
    Convert::CConvert convert(bOutStruct);
    convert.field(pRootJson,     "classId",        stInfo.classId);
    convert.field(pRootJson,     "courseId",        stInfo.courseId);
    convert.field(pRootJson,     "date",            stInfo.date);
    convert.structure(pRootJson, "attendanceInfo",  stInfo.stAttendanceInfo);
}

void Convert::deal(Json::Object *pRootJson, AiStudentBusiness_NS::BehaviorRecord &stInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }
    Convert::CConvert convert(bOutStruct);
    convert.field(pRootJson, "state",       stInfo.bState);
    convert.field(pRootJson, "triggerTime", stInfo.strTriggerTime);
    /* behaviorType 以整型存储，避免枚举序列化歧义 */
    int nBehaviorType = static_cast<int>(stInfo.eBehaviorType);
    convert.field(pRootJson, "behaviorType", nBehaviorType);
    if (bOutStruct)
    {
        stInfo.eBehaviorType = static_cast<AiStudentBusiness_NS::PlatformBehaviorType_E>(nBehaviorType);
    }
}

void Convert::deal(Json::Object *pRootJson, AiStudentBusiness_NS::StudentPerformance &stInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }
    Convert::CConvert convert(bOutStruct);
    convert.field(pRootJson, "focusAverage",     stInfo.nFocusAverage);
    convert.field(pRootJson, "focusData",         stInfo.strFocusData);
    convert.field(pRootJson, "distractionData",   stInfo.strDistractionData);
    convert.field(pRootJson, "engagementReport",  stInfo.strEngagementReport);
}

void Convert::deal(Json::Object *pRootJson, AiStudentBusiness_NS::StudentBehaviorRecord &stInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }
    Convert::CConvert convert(bOutStruct);
    convert.field(pRootJson,     "classId",          stInfo.classId);
    convert.field(pRootJson,     "courseId",          stInfo.courseId);
    convert.structure(pRootJson, "behaviorTimeline",  stInfo.behaviorTimeline);
}

void Convert::deal(Json::Object *pRootJson, AiStudentBusiness_NS::StudentPerformanceRecord &stInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }
    Convert::CConvert convert(bOutStruct);
    convert.field(pRootJson,     "classId",          stInfo.classId);
    convert.field(pRootJson,     "courseId",          stInfo.courseId);
    convert.structure(pRootJson, "performance",       stInfo.performance);
}

#endif