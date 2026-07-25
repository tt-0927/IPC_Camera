#pragma once

#include <mutex>
#include <string>
#include <unordered_map>

#include "0630AppExtern.hpp"
#include "FaceRecExtern.hpp"
#include "SignalSlot.h"

namespace Ai0630_NS
{
    /* 单个学生的考勤记录 */
    struct AttendanceInfo_S
    {
        int         nId;           /* 学生ID */
        std::string strName;       /* 学生名字 */

        long long nFirstTimestamp; /* 第一次出现的绝对时间戳（毫秒） */
        long long nLastTimestamp;  /* 最后一次出现的绝对时间戳（毫秒） */

        long long nFirstTime;      /* 第一次出现的上课时间（自 00:00:00 累加） */
        long long nLastTime;       /* 最后一次出现的上课时间 */

        long long nNumber;         /* 出现次数 */


        std::list<int> listAnswerTime; /* 回答问题时刻时间点数组 */

        AttendanceInfo_S()
            : nId(-1),
              nFirstTimestamp(0),
              nLastTimestamp(0),
              nFirstTime(0),
              nLastTime(0),
              nNumber(0)
        {
        }
    };

    class AttendanceManager
    {
    public:

        AttendanceManager()  = default;
        ~AttendanceManager() = default;

        /**
         * @brief 记录人类识别事件
         * @param stFaceLibsInfo 人类信息
         * @param stUserHeaderInfo 头数据
         */
        void add(const FaceLibsInfo_S& stFaceLibsInfo, const UserHeaderInfo_S& stUserHeaderInfo);

        /**
         * @brief 整合
         */
        void finalize(const void* pParam);

        /**
         * @brief 清空记录（下一堂课）
         */
        void reset();

    private:

        /**
         * @brief 保存结果文件
         * @param strFilePath 文件绝对路径
         */
        void saveFile(std::string strFilePath);

    private:

        mutable std::mutex                        m_mutex;
        std::unordered_map<int, AttendanceInfo_S> m_studentMap;
        std::unordered_map<int, AttendanceInfo_S> m_teacherMap;
    };
}    // namespace Ai0630_NS
