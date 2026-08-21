#pragma once
#include <mutex>
#include <unordered_map>
#include <vector>

#include "0630AppExtern.hpp"
#include "ExclusiveTimeline.hpp"
#include "Intern.hpp"
#include "ReportManager.hpp"

namespace Ai0630_NS
{
    /* ======================= 枚举定义 ======================= */

    /* 老师行为（互斥） */
    enum class TeacherBehavior_E
    {
        TEACHING = 0,  /* 教师讲授 */
        INTERACTION,   /* 师生互动 */
        GUIDING,       /* 指导学生 */
        WRITING_BOARD, /* 书写板书 */
        WALKING_AROUND /* 教室巡视 */
    };

    /* 姿态（互斥） */
    enum class TeacherPosture_E
    {
        OTHER           = -1, /* 其他 */
        FRONT_EXPLAIN   = 0,  /* 正面讲解 */
        ARMS_CROSSED    = 1,  /* 双手抱臂 */
        FACE_BLACKBOARD = 2,  /* 面向黑板 */
        HANDS_ON_HIP    = 3,  /* 叉腰表达 */
        HEAD_DOWN       = 4,  /* 低头 */
        HEAD_UP         = 5,  /* 抬头 */
        HAND_UP         = 6,  /* 举手示意 */
        PLAY_PHONE      = 7,  /* 玩手机 */
        CALL_PHONE      = 8,  /* 接打电话 */
        TEABOARD        = 9,  /* 板书 */
    };

    /* ======================= 数据结构 ======================= */

    struct TeacherBehaviorSummary_S
    {
        long long nTeachDuration      = 0; /* 教师讲授 */
        long long nInteractDuration   = 0; /* 师生互动 */
        long long nGuideDuration      = 0; /* 指导学生 */
        long long nWriteBoardDuration = 0; /* 板书 */
        long long nWalkDuration       = 0; /* 巡视 */

        long long nPodiumDuration = 0;     /* 讲台时长（讲授 + 互动 + 板书） */
        long long nPatrolDuration = 0;     /* 巡视时长（指导 + 巡视） */
        int       nPatrolCount    = 0;     /* 巡视次数 */

        /* 百分比 */
        std::unordered_map<TeacherPosture_E, int> mapPostureRatio;
    };

    struct HeatPoint_S
    {
        long long nClassTime;
        int       nX;
        int       nY;
    };

    /* ======================= 主类：老师行为管理 ======================= */

    class TeacherSummary
    {
    public:

        explicit TeacherSummary(long long mergeMs = 2000);

        /**
         * @brief 添加行为
         * @param stUserHeaderInfo
         * @param enType
         */
        void addBehavior(const UserHeaderInfo_S& stUserHeaderInfo, TeacherBehavior_E enType);

        /**
         * @brief 添加姿态
         * @param stUserHeaderInfo
         * @param enType
         */
        void addPosture(const UserHeaderInfo_S& stUserHeaderInfo, TeacherPosture_E enType);

        /**
         * @brief 添加位置
         * @param stUserHeaderInfo
         * @param nX
         * @param nY
         */
        void addPosition(const UserHeaderInfo_S& stUserHeaderInfo, std::vector<Inference_NS::Box_S> vstCurTurnBoxInfo);

        /**
         * @brief int转姿态枚举
         * @param nPosture 表情
         * @return TeacherPosture_E
         */
        TeacherPosture_E toPosture(int nPosture);

        /**
         * @brief 整合
         */
        void finalize(const void* pParam);

        /**
         * @brief 获取行为
         * @return const auto&
         */
        /* 获取行为 */
        const std::unordered_map<
            TeacherBehavior_E,
            ExclusiveTimeline<TeacherBehavior_E>::StateStat >&
            getBehaviorStats() const;

        /**
         * @brief 获取姿态
         * @return const auto&
         */
        const std::unordered_map<
            TeacherPosture_E,
            ExclusiveTimeline<TeacherPosture_E>::StateStat >&
            getPostureStats() const;

        /**
         * @brief 获取热力图
         * @return const auto&
         */
        const std::vector<HeatPoint_S>& getHeatMap() const;

        /**
         * @brief 获取总结
         * @return const auto&
         */
        TeacherBehaviorSummary_S getSummary() const;

        /**
         * @brief 清空记录（下一堂课）
         */
        void reset();

        /**
         * @brief 获取班级参数
         * @param stInfo
         */
        void getInfo(ClassParamParam_S& stInfo);

        /**
         * @brief 切换老师是否在讲台
         * @param [bool]  bValue: 是否老师在讲台
         * @return BlError_E
         */
        BlError_E switchTeacherPodium(bool bValue);

    private:

        /**
         * @brief 保存结果文件
         * @param strFilePath 文件绝对路径
         */
        void saveFile(std::string strFilePath);

    private:

        ExclusiveTimeline<TeacherBehavior_E> m_behavior;
        ExclusiveTimeline<TeacherPosture_E>  m_posture;

        std::vector<HeatPoint_S> m_positions;
        mutable std::mutex       m_mutex;

        std::atomic_bool    m_bCurTsPodium = true; /* 当前老师是否在讲台 */
        /* 老师下讲台的轨迹样本坐标 */
        Inference_NS::Box_S m_stCurSampleBoxInfo;
        Inference_NS::Box_S m_stLastSampleBoxInfo;
    };
}    // namespace Ai0630_NS