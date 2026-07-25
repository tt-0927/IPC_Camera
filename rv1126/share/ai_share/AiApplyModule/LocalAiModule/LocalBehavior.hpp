/*
 * @FilePath     : LocalBehavior.hpp
 * @Author       : yanzeh yanzeh@kfb.cn
 * @Date         : 2024-01-17 16:16:42
 * @LastEditors  : 严泽辉 yanzeh@kfb.cn
 * @LastEditTime : 2024-08-06 17:31:15
 * @Description  : 学生行为分析
 */
#pragma once

#include <atomic>
#include <condition_variable>
#include <mutex>
#include <queue>
#include <thread>

#include "AiLocalExtern.hpp"
#include "StudentBehaviorV2_0.hpp"

namespace AiLocal_NS
{
    class CLocalBehavior
    {
    public:

        CLocalBehavior(returnDataFunc pReturn);
        ~CLocalBehavior();

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