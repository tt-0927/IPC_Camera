/*
 * @FilePath     : ShipAnalyzerBase.hpp
 * @Author       : 严泽辉 yanzeh@kfb.cn
 * @Date         : 2024-03-08 10:36:45
 * @LastEditors  : 严泽辉 yanzeh@kfb.cn
 * @LastEditTime : 2024-03-14 19:20:41
 * @Description  : 桥梁分析模块基类
 */
#pragma once

#include <atomic>
#include <list>
#include <mutex>
#include <queue>
#include <thread>
#include <vector>

#include "BlError.h"
#include "BlockingQueue.hpp"
#include "DataQueue.hpp"
#include "ShipAnalyzerExtern.hpp"

namespace ShipAnalyzer_NS
{
    class CShipAnalyzerBase
    {
    public:

        CShipAnalyzerBase(InParam_S stInParam);
        virtual ~CShipAnalyzerBase();

        /**
         * @brief 发送分析数据
         * @param [MediaDataInfo_S] stMediaDataInfo: 媒体数据信息
         * @param [int] nTimeOutMs:  0-不阻塞 大于0-带超时的阻塞 小于0-死等
         * @return [*] 成功 >= BlError_E::OK   其他失败
         * @note 如果设置了stMediaDataInfo.freeDataFunc; 内部使用完会自动释放stMediaDataInfo.pchData。
         * @note 非阻塞调用
         */
        BlError_E send_dataAnalysis(MediaDataInfo_S stMediaDataInfo, int nTimeOutMs = 0);

        /**
         * @brief 读取分析数据
         * @param [AnalyzerResult_S&] stOutInfo: 分析结果链表
         * @param [int] nTimeOutMs:  0-不阻塞 大于0-带超时的阻塞 小于0-死等
         * @return [*] 成功 >= BlError_E::OK   其他失败
         * @note
         */
        BlError_E read_analysisResult(std::list<AnalyzerResult_S>& listOutInfo, int nTimeOutMs = 0);

        /**
         * @brief 实时分析数据
         * @param [MediaDataInfo_S] stMediaDataInfo: 媒体数据信息
         * @param [std::list<AnalyzerResult_S>&] listOutInfo: 分析后的结果信息
         * @return [*] 成功 >= BlError_E::OK   其他失败
         * @note 如果设置了stMediaDataInfo.freeDataFunc; 内部使用完会自动释放stMediaDataInfo.pchData。
         * @note 阻塞调用
         */
        BlError_E realTime_dataAnalysis(MediaDataInfo_S stMediaDataInfo, std::list<AnalyzerResult_S>& listOutInfo);

    protected:

        /**
         * @brief 分析数据
         * @param [MediaDataInfo_S] stMediaDataInfo: 媒体数据信息
         * @param [std::list<AnalyzerResult_S>&] listOutInfo: 分析后的结果信息
         * @return [*] 成功 >= BlError_E::OK   其他失败
         * @note 如果设置了stMediaDataInfo.freeDataFunc; 内部使用完会自动释放stMediaDataInfo.pchData。
         */
        virtual BlError_E dataAnalysis(MediaDataInfo_S&             stMediaDataInfo,
                                       std::list<AnalyzerResult_S>& listOutInfo) = 0;


    private:

        /**
         * @brief 线程函数
         * @return [*] 无
         * @note
         */
        void run();

    protected:

        /* 传入参数 */
        InParam_S m_stInParam;

    private:

        /* 线程相关 */
        std::thread       m_threadObj;
        std::atomic<bool> m_bRunning = { true };

        /* 使用模板类操作队列 */
        BQ_NS::CBlockingQueue<MediaDataInfo_S>*             m_pPendingQueue = nullptr;
        BQ_NS::CBlockingQueue<std::list<AnalyzerResult_S>>* m_pResultQueue  = nullptr;
    };

}    // namespace ShipAnalyzer_NS
