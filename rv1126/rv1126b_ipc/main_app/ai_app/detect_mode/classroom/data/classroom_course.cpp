/**
 * @FilePath     : classroom_course.cpp
 * @Author       : leiyy
 * @Date         : 2026-11-16 10:21:17
 * @LastEditors  : leiyy
 * @LastEditTime : 2026-11-16 10:21:17
 * @Description  : 平台课前下发课程/名单的解析/校验/序列化
 */

#include "classroom_course.h"

#include "Json.h"

namespace Classroom
{
namespace
{
/* 从一个人形对象读 id/name/photo（缺键留空，不报错——name/photo 本就可选） */
void readPerson(Json::Object *pObj, CCoursePerson_S &rstOut)
{
    if (!pObj)
    {
        return;
    }
    Json::get(pObj, "id", rstOut.strId);
    Json::get(pObj, "name", rstOut.strName);
    Json::get(pObj, "photo", rstOut.strPhoto);
}

Json::Object *personJson(const CCoursePerson_S &stP)
{
    Json::Object *pObj = Json::init();
    Json::add(pObj, "id", stP.strId);
    Json::add(pObj, "name", stP.strName);
    Json::add(pObj, "photo", stP.strPhoto);
    return pObj;
}
}

bool CClassroomCourse::setFromJson(const std::string &strJson, int nLocalRole,
                                   int64_t llRecvWallMs, std::string &rstrErr)
{
    Json::Object *pRoot = Json::init(strJson.c_str());
    if (!pRoot)
    {
        rstrErr = "课程下发 JSON 解析失败";
        return false;
    }

    CCourseInfo_S stNew;
    stNew.llRecvWallMs = llRecvWallMs;

    if (!Json::get(pRoot, "course_id", stNew.strCourseId) || stNew.strCourseId.empty())
    {
        Json::deinit(pRoot);
        rstrErr = "课程下发缺 course_id";
        return false;
    }
    Json::get(pRoot, "course_name", stNew.strCourseName);
    long long llStart = 0;
    long long llEnd = 0;
    Json::get(pRoot, "start_ms", llStart);
    Json::get(pRoot, "end_ms", llEnd);
    stNew.llStartWallMs = static_cast<int64_t>(llStart);
    stNew.llEndWallMs = static_cast<int64_t>(llEnd);
    Json::get(pRoot, "live_id", stNew.strLiveId);
    Json::get(pRoot, "classroom_id", stNew.strClassroomId);
    Json::get(pRoot, "classroom_name", stNew.strClassroomName);

    /* 区分教师机/学生机：带了 device_role 就必须与本机一致。没带则按本机角色落档。
       拒绝而不是静默改写——把学生机名单灌进教师机（或反之）会让后续考勤张冠李戴 */
    int nPayloadRole = -1;
    if (Json::get(pRoot, "device_role", nPayloadRole))
    {
        if (nPayloadRole != nLocalRole)
        {
            Json::deinit(pRoot);
            rstrErr = "课程下发角色不符: 报文=" + std::to_string(nPayloadRole) +
                      " 本机=" + std::to_string(nLocalRole);
            return false;
        }
        stNew.nDeviceRole = nPayloadRole;
    }
    else
    {
        stNew.nDeviceRole = nLocalRole;
    }

    readPerson(Json::get(pRoot, "teacher"), stNew.stTeacher);

    Json::Object *pStus = Json::get(pRoot, "students");
    if (pStus)
    {
        const int nNum = Json::Array::size(pStus);
        if (nNum > CLASSROOM_COURSE_STUDENT_MAX)
        {
            Json::deinit(pRoot);
            rstrErr = "课程下发学生数超限: " + std::to_string(nNum) + " > " +
                      std::to_string(CLASSROOM_COURSE_STUDENT_MAX);
            return false;
        }
        stNew.vecStudents.reserve(static_cast<size_t>(nNum));
        for (int i = 0; i < nNum; ++i)
        {
            Json::Object *pItem = Json::Array::get(pStus, i);
            CCoursePerson_S stP;
            readPerson(pItem, stP);
            if (stP.strId.empty())
            {
                Json::deinit(pRoot);
                rstrErr = "课程下发学生缺 id（第 " + std::to_string(i) + " 条）";
                return false;
            }
            stNew.vecStudents.push_back(std::move(stP));
        }
    }

    Json::deinit(pRoot);
    stNew.bValid = true;
    m_stInfo = std::move(stNew);   /* 校验全过才落档：坏报文不动上一节课的名单 */
    rstrErr.clear();
    return true;
}

std::string CClassroomCourse::toJson() const
{
    Json::Object *pRoot = Json::init();
    Json::add(pRoot, "valid", m_stInfo.bValid);
    Json::add(pRoot, "course_id", m_stInfo.strCourseId);
    Json::add(pRoot, "course_name", m_stInfo.strCourseName);
    Json::add(pRoot, "start_ms", static_cast<long long>(m_stInfo.llStartWallMs));
    Json::add(pRoot, "end_ms", static_cast<long long>(m_stInfo.llEndWallMs));
    Json::add(pRoot, "live_id", m_stInfo.strLiveId);
    Json::add(pRoot, "classroom_id", m_stInfo.strClassroomId);
    Json::add(pRoot, "classroom_name", m_stInfo.strClassroomName);
    Json::add(pRoot, "device_role", m_stInfo.nDeviceRole);
    Json::add(pRoot, "recv_ms", static_cast<long long>(m_stInfo.llRecvWallMs));
    Json::add(pRoot, "student_count", static_cast<int>(m_stInfo.vecStudents.size()));

    Json::Object *pTea = personJson(m_stInfo.stTeacher);
    Json::add(pRoot, "teacher", pTea);

    Json::Object *pArr = Json::Array::init();
    for (size_t i = 0; i < m_stInfo.vecStudents.size(); ++i)
    {
        Json::Object *pItem = personJson(m_stInfo.vecStudents[i]);
        Json::Array::add(pArr, pItem);
    }
    Json::add(pRoot, "students", pArr);

    const std::string strOut = Json::to_string(pRoot);
    Json::deinit(pRoot);
    return strOut;
}
}