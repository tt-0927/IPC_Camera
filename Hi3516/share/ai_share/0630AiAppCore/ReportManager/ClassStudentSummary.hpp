#pragma once

#include "StudentSummary.hpp"

namespace Ai0630_NS
{
    class ClassStudentSummary
    {
    public:

        /**
         * @brief 添加个人行为分析
         * @param nId 个人ID
         * @param nClassTime 课堂时间
         * @param enBehavior 行为枚举
         */
        void addStudentBehavior(int nId, long long nClassTime, int nBehavior);

        /**
         * @brief 添加个人表情信息
         * @param nId 个人ID
         * @param nClassTime 课堂时间
         * @param enBehavior 表情枚举
         */
        void addStudentEmotion(int nId, long long nClassTime, int nEmo);

        /**
         * @brief 添加全班行为信息
         * @param nClassTime 课堂时间
         * @param mapCnt 行为数据
         */
        void addClassBehaviorBatch(
            long long                                         nClassTime,
            const std::unordered_map<StudentBehavior_E, int>& mapCnt);

        /**
         * @brief 添加全班表情信息
         * @param nClassTime 课堂时间
         * @param mapCnt 表情数据
         */
        void addClassEmotionBatch(long long                                 nClassTime,
                                  const std::unordered_map<Emotion_E, int>& mapCnt);

        /**
         * @brief 当前检测到的总人数
         * @param nCount 当前人数
         * @param nClassTime 班级时间
         */
        void addPersonCount(int nCount, long long nClassTime);

        /**
         * @brief 统计信息
         */
        void finalize(const void* pParam);

        /**
         * @brief 复位
         */
        void reset();

        /**
         * @brief int转表情枚举
         * @param nEmo 表情
         * @return Emotion_E
         */
        Emotion_E toEmotion(int nEmo);

        /**
         * @brief int转行为枚举
         * @param nBehavior 表情
         * @return StudentBehavior_E
         */
        StudentBehavior_E toBehavior(int nBehavior);

        /**
         * @brief 获取班级参数
         * @param stInfo
         */
        void getInfo(ClassParamParam_S& stInfo);

    private:

        /**
         * @brief 判断是否存在行为
         * @param enType 行为
         * @param nCount 该行为的人数
         * @param nTotal 总人数
         * @return true
         * @return false
         */
        bool behaviorActive(StudentBehavior_E enType, int nCount, int nTotal) const;

        /**
         * @brief 预处理（记录原始人数）
         * @param nCount
         */
        void updateClassSizeRaw(int nCount);

        /**
         * @brief 滑动窗口中位数滤波
         * @param vecData
         * @param nIdx
         * @param nWinSize
         * @return int
         */
        int medianFilter(const std::vector<int>& vecData, int nIdx, int nWinSize);

        /**
         * @brief 计算班级人数
         */
        void calcClassSize();

    private:

        /**
         * @brief 保存结果文件
         * @param strFilePath 文件绝对路径
         */
        void saveFile(std::string strFilePath);


    private:

        /* 个人 */
        std::unordered_map<int, StudentSummary> m_students;

        /* 班级 */
        StudentBehaviorTimeline m_stClassBehavior;
        EmotionTimeline         m_stClassEmotion;


        StudentBehaviorResult_S m_stStudentBehaviorResult;
        EmotionTimelineResult_S m_stEmotionTimelineResult;

        /* 班级人数估算数据 */
        std::vector<int>
                                     m_detectedList;   /* 原始检测人数 */
        std::vector<int>             m_filteredList;   /* 滤波后的人数 */
        std::unordered_map<int, int> m_sizeHist;       /* 直方图（人数 → 次数） */
        int                          m_nClassSize = 0; /* 最终估算的稳定班级人数 */

    };
}    // namespace Ai0630_NS