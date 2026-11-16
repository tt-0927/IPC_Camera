/**
 * @FilePath     : classroom_course.h
 * @Author       : leiyy
 * @Date         : 2026-11-16 10:21:17
 * @LastEditors  : leiyy
 * @LastEditTime : 2026-11-16 10:21:17
 * @Description  : 平台课前下发的课程/名单设备侧落地存储
 */

#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace Classroom
{
/* 学生名单上限：超出拒绝（不静默截断）。一个教室的人数有界，256 足够宽裕，
   同时挡住一份异常报文把内存撑爆。 */
constexpr int CLASSROOM_COURSE_STUDENT_MAX = 256;

/* 课程/名单里的一个人（学生或教师）。photo 是照片地址，本阶段只存不取（考勤暂缓）。 */
struct CCoursePerson_S
{
    std::string strId;
    std::string strName;
    std::string strPhoto;
};

/* 一节课的完整下发内容。时间用墙钟毫秒（与统计 llSessionWallMs 同一口径）。 */
struct CCourseInfo_S
{
    std::string strCourseId;
    std::string strCourseName;
    int64_t     llStartWallMs = 0;
    int64_t     llEndWallMs = 0;
    std::string strLiveId;
    std::string strClassroomId;
    std::string strClassroomName;
    CCoursePerson_S stTeacher;
    std::vector<CCoursePerson_S> vecStudents;
    int         nDeviceRole = -1;   /* 落档角色（DeviceRole_E：0 教师/1 学生/2 板机） */
    int64_t     llRecvWallMs = 0;   /* 设备收到这份下发的墙钟毫秒 */
    bool        bValid = false;     /* 成功解析过一份才为 true */
};

class CClassroomCourse
{
public:
    /**
     * @brief   : 解析并落档平台下发的课程/名单 JSON
     * @param    {const std::string&} strJson：报文 JSON（schema 见 .cpp 文件头）
     * @param    {int} nLocalRole：本机角色（DeviceRole_E）
     * @param    {int64_t} llRecvWallMs：收到的墙钟毫秒（调用方取，本类不读时钟）
     * @param    {std::string&} rstrErr：OUT=失败原因
     * @return   {bool} false=拒绝（JSON 坏 / 缺 course_id / 角色不符 / 名单超限）
     * @note    : 失败时**不改动**已有落档（上一节课的名单不会被一份坏报文抹掉）。
     *            photo 只存不取（考勤暂缓）。
     */
    bool setFromJson(const std::string &strJson, int nLocalRole, int64_t llRecvWallMs,
                     std::string &rstrErr);

    /* 读回 JSON（含名单 id/name/photo——花名册本身允许带姓名）。从未落档时回 bValid=false 骨架。 */
    std::string toJson() const;

    const CCourseInfo_S &info() const { return m_stInfo; }
    bool valid() const { return m_stInfo.bValid; }
    void clear() { m_stInfo = CCourseInfo_S(); }

private:
    CCourseInfo_S m_stInfo;
};
}