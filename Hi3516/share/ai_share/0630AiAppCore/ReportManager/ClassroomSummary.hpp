#pragma once

#include <string>

#include "ReportManager.hpp"

namespace Ai0630_NS
{
    class ClassroomSummary
    {
    private:

        struct Point_S
        {
            int nX = 0; /* X 轴：教师行为累计 */
            int nY = 0; /* Y 轴：学生行为累计 */
        };

        /* ================= 输出结果 ================= */
        struct ClassSummary_S
        {
            std::string strClassSummary;        /* 课堂总总结 */
            std::string strStudentFocusSummary; /* 专注/散漫总结 */
            std::string strEmotionSummary;      /* 情绪总结 */

            std::string strClassTypeConclusion; /* 课堂类型结论 */
            std::string strClassEvaluateSumUp;  /* 课堂总体评价 */

            float fRtChX      = 0.0f;
            float fRtChY      = 0.0f;
            int   nStTeCutNum = 0; /* 学生行为转老师行为次数 */

            /* 加起来100 */
            int nTeacherTimeRatio; /* 老师授课时间，百分数*/
            int nStudentTimeRatio; /* 学生活动时间，百分数*/

            /* 加起来100 */
            int nTaughtPct;                     /* 教师讲授时间占比，单位% */
            int nInteractionPct;                /* 师生互动时间占比，单位% */
            int nStudyPct;                      /* 学生学习时间占比，单位% */

            int nParticipationRate;             /* 参与率，单位% */
            int nHeadUpRate;                    /* 抬头率，单位% */
            int nHeadDownRate;                  /* 低头率，单位% */

            int              nPPTInterval;      /* PPT翻页间隔，单位/s*/
            std::vector<int> vecPPTSwitchTimes; /* PPT翻页时间戳链表*/

            std::vector<Point_S> vecSTCurve;

            std::vector<Point_S> vecDb;

            int nCurClassTime;

            void clear()
            {
                strClassSummary.clear();
                strStudentFocusSummary.clear();
                strEmotionSummary.clear();
                strClassTypeConclusion.clear();
                strClassEvaluateSumUp.clear();
                fRtChX             = 0.0f;
                fRtChY             = 0.0f;
                nStTeCutNum        = 0;
                nTeacherTimeRatio  = 0;
                nStudentTimeRatio  = 0;
                nTaughtPct         = 0;
                nInteractionPct    = 0;
                nStudyPct          = 0;
                nParticipationRate = 0;
                nHeadUpRate        = 0;
                nHeadDownRate      = 0;
                nPPTInterval       = 0;
                vecPPTSwitchTimes.clear();
                vecSTCurve.clear();
                nCurClassTime = 0;
            }
        };


    public:

        /**
         * @brief 添加一次学生行为
         * @note  只累计 Y，不生成曲线点
         */
        void addStudentBehavior();

        /**
         * @brief 添加一次教师行为
         * @note  X+1，并生成一个 ST 曲线点
         */
        void addTeacherBehavior();

        /**
         * @brief 添加课堂参数
         * @param stParam
         */
        void addClassParam(const ClassParamParam_S& stParam);

        /**
         * @brief 添加 PPT 翻页时间点
         * @param nClassTime
         */
        void addPPTSwitch(int nClassTime);

        /**
         * @brief 添加课堂分贝值
         * @note
         */
        void addClassDb(int nClassTime, int nDb);

        /**
         * @brief 整合
         */
        void finalize(const void* pParam);

        /**
         * @brief 清空记录（下一堂课）
         */
        void reset();

        /**
         * @brief 生成完整课堂总结
         */
        ClassSummary_S generate(const ClassParamParam_S& param);

    private:

        /* ============== 内部工具（原 StudentSummaryBuilder） ============== */

        std::string buildStageDesc(
            const std::string& prefix,
            long long          startSec,
            long long          endSec,
            int                avgScore,
            const std::string& emptyText);

        std::string buildFocusSummary(
            const Segment_S& seg,
            int              avgScore);

        std::string buildDistractSummary(
            const Segment_S& seg,
            int              avgScore);

        std::string buildExcitedSummary(
            const Segment_S& seg);

        std::string buildLowSummary(
            const Segment_S& seg);

        std::string buildClassAdvice();

        void analyzeClassType();

        /**
         * @brief 保存结果文件
         * @param strFilePath 文件绝对路径
         */
        void saveFile(std::string strFilePath);


    private:

        ClassParamParam_S m_stClassParamParam;
        ClassSummary_S    m_stClasstSummary;
        /* ================= ST 曲线内部状态 ================= */

        int  m_nSTTeacherCount = 0; /* X 累计 */
        int  m_nSTStudentCount = 0; /* Y 累计 */
        bool m_bStudent        = false;
    };
}    // namespace Ai0630_NS
