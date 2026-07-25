/*
 * @FilePath     : LocalDisciplineGather.hpp
 * @Author       : 严泽辉 yanzeh@kfb.cn
 * @Date         : 2024-05-31 17:12:23
 * @LastEditors: weigl 308241951@qq.com
 * @LastEditTime: 2024-12-18 10:11:16
 * @Description  : 课堂纪律
 */
#pragma once

#include <atomic>
#include <condition_variable>
#include <mutex>
#include <queue>
#include <thread>

#include "AiLocalExtern.hpp"
#include "ClassroomMoveDetectV1_0.hpp"

namespace AiLocal_NS
{
    class CLocalDisciplineGather
    {
    public:

        CLocalDisciplineGather(returnDataFunc pReturn);
        ~CLocalDisciplineGather();

        /**
         * @brief 添加分析数据
         * @param [CommDataInfo_S*] *pchData: 数据
         * @return [*] 成功 >= BlError_E::OK   其他失败
         * @note 内部释放pchData
         */
        BlError_E addData(AiManage_NS::CommDataInfo_S* pchData);

        /**
         * @brief 清空数据
         * @return [*]
         * @note 
         */
        BlError_E clearData();

        /**
         * @brief 初始化分析功能
         * @return [*] 成功 >= BlError_E::OK   其他失败
         * @note
         */
        BlError_E init();


    private:

        /**
         * @brief 线程函数
         * @return [*] 无
         * @note
         */
        void run();


    private:

        /* 线程相关 */
        std::thread       m_threadObj;
        std::atomic<bool> m_bRunning = { true };

        /* 待处理队列 */
        std::queue<AiManage_NS::CommDataInfo_S*> m_pendingQueue;

        /* 队列最大值 */
        int m_nMaxQueueSize = 2;

        /* 锁 */
        std::mutex              m_queueMutex;
        std::condition_variable m_condition;

        /* 分析句柄 */
        Scenario_NS::CScenarioBase* m_pAlgorithmHndl = nullptr;

        returnDataFunc m_pReturn;
    };

}    // namespace AiLocal_NS