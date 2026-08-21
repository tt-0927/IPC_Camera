#pragma once

#include "0630AppExtern.hpp"
#include "AttendanceManager.hpp"
#include "ClassroomSummary.hpp"
#include "ClassStudentSummary.hpp"
#include "Intern.hpp"
#include "TeacherSummary.hpp"

namespace Ai0630_NS
{
    class ResultModule
    {
    public:

        ResultModule();
        ~ResultModule();

        /**
         * @brief 处理算法分析数据
         * @param stResult 分析数据
         *
         * 人脸识别不在这里处理
         */
        void recvData(HeaderInfo_S     stHeader,
                      UserHeaderInfo_S stUserHeader,
                      FaceResult_S     stResult);

        /**
         * @brief 接受人脸处理结果
         * @param stHeader
         * @param stUserHeader
         * @param syFaceLibsInfo
         * @param stHumanLibsInfo
         */
        void recvFaceData(HeaderInfo_S     stHeader,
                          UserHeaderInfo_S stUserHeader,
                          FaceLibsInfo_S   syFaceLibsInfo,
                          HumanLibsInfo_S  stHumanLibsInfo);

        /**
         * @brief 整合
         */
        void finalize(const void* pParam);

        /**
         * @brief PPT切换
         * @param lTimestamp 时间戳
         * @param strJpgName jpg图片名称
         * @return BlError_E
         */
        BlError_E pptSwitch(long long lTimestamp, std::string strJpgName);

        /**
         * @brief 切换老师画面
         * @return BlError_E
         */
        BlError_E switchTeacherScreen();

        /**
         * @brief 切换学生画面
         * @return BlError_E
         */
        BlError_E switchStudentScreen();

        /**
         * @brief 切换学生特写
         * @param [bool]  bValue: 是否学生特写
         * @return BlError_E
         */
        BlError_E switchStudentCloseUp(UserHeaderInfo_S stUserHeader, bool bValue);

        /**
         * @brief 切换老师是否在讲台
         * @param [bool]  bValue: 是否老师在讲台
         * @return BlError_E
         */
        BlError_E switchTeacherPodium(bool bValue);

        /* 开始互动 */
        BlError_E startInteraction(UserHeaderInfo_S stUserHeader);
        /* 开始巡视行为 */
        BlError_E startTour(UserHeaderInfo_S stUserHeader);
        /* 开始教授行为 */
        BlError_E startTaught(UserHeaderInfo_S stUserHeader);
        /* 老师板书切换 */
        BlError_E teacherBoard(UserHeaderInfo_S stUserHeader, bool bValue);

        /**
         * @brief 设置录制时间
         * @param nRecordTime
         * @return BlError_E
         */
        BlError_E setRecordTime(int nRecordTime);

        /**
         * @brief 添加课堂分贝值
         * @note
         */
        void addClassDb(int nDb);

    private:

        std::atomic_bool m_bCurStScreen    = false; /* 当前是否为学生画面 */
        std::atomic_bool m_bCurTsPodium    = true;  /* 当前老师是否在讲台 */
        std::atomic_bool m_bStudentCloseUp = false; /* 是否学生特写 */


        /* 考勤分析报告管理 */
        std::shared_ptr<AttendanceManager>   m_pAttendanceManager;
        /* 老师分析报告管理 */
        std::shared_ptr<TeacherSummary>      m_pTeacherSummary;
        /* 学生分析报告管理 */
        std::shared_ptr<ClassStudentSummary> m_pClassStudentSummary;
        /* 课堂报告管理 */
        std::shared_ptr<ClassroomSummary>    m_pClassroomSummary;

        int m_nRecordTime = 0;
    };
}    // namespace Ai0630_NS
