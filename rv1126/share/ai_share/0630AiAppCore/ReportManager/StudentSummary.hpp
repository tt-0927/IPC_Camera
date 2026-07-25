#pragma once
#include <mutex>
#include <unordered_map>
#include <vector>

#include "EmotionTimeline.hpp"
#include "ReportManager.hpp"
#include "StudentBehaviorTimeline.hpp"

namespace Ai0630_NS
{

    /* ------------------------- 单学生行为管理 ------------------------- */
    class StudentSummary
    {
    public:

        /**
         * @brief 添加行为分析
         * @param nClassTime 课堂时间
         * @param enBehavior 行为枚举
         */
        void addBehavior(long long nClassTime, StudentBehavior_E enBehavior);

        /**
         * @brief 添加表情信息
         * @param nClassTime 课堂时间
         * @param enEmo 表情枚举
         */
        void addEmotion(long long nClassTime, Emotion_E enEmo);

        /**
         * @brief 统计信息
         */
        void finalize(int nId, const void* pParam);

        /**
         * @brief 复位
         */
        void reset();

    private:

        /**
         * @brief 保存结果文件
         * @param strFilePath 文件绝对路径
         */
        void saveFile(std::string strFilePath);

    private:

        StudentBehaviorTimeline m_stBehavior;
        EmotionTimeline         m_stEmotion;

        StudentBehaviorResult_S m_stStudentBehaviorResult;
        EmotionTimelineResult_S m_stEmotionTimelineResult;

        int m_nId = 0;
    };

}    // namespace Ai0630_NS
