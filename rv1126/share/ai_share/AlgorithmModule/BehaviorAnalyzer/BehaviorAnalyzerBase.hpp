/*
 * @FilePath     : BehaviorAnalyzerBase.hpp
 * @Author       : yanzeh yanzeh@kfb.cn
 * @Date         : 2024-01-08 15:07:12
 * @LastEditors  : yanzeh yanzeh@kfb.cn
 * @LastEditTime : 2024-01-19 17:39:21
 * @Description  : 行为分析模块基类
 */
#pragma once

#include <atomic>
#include <list>
#include <mutex>
#include <queue>
#include <thread>
#include <vector>

#include "BehaviorAnalyzerExtern.hpp"
#include "BlError.h"
#include "DataQueue.hpp"

namespace BA_NS
{
    class CBehaviorAnalyzerBase
    {
    public:

        CBehaviorAnalyzerBase(BehaviorAnalyzerInParam_S stInParam);
        virtual ~CBehaviorAnalyzerBase();

        /**
         * @brief 发送分析数据
         * @param [MediaDataInfo_S] stMediaDataInfo: 媒体数据信息
         * @return [*] 成功 >= BlError_E::OK   其他失败
         * @note 如果设置了stMediaDataInfo.freeDataFunc; 内部使用完会自动释放stMediaDataInfo.pchData。
         * @note 非阻塞调用
         */
        BlError_E send_dataAnalysis(MediaDataInfo_S stMediaDataInfo);

        /**
         * @brief 读取分析数据
         * @param [std::list<BehaviorAnalyzerResult_S>&] listOutInfo: 分析结果链表
         * @return [*] 成功 >= BlError_E::OK   其他失败
         * @note
         */
        BlError_E read_analysisResult(std::list<BehaviorAnalyzerResult_S>& listOutInfo);

        /**
         * @brief 实时分析数据
         * @param [MediaDataInfo_S] stMediaDataInfo: 媒体数据信息
         * @param [std::list<BehaviorAnalyzerResult_S>&] listOutInfo: 分析结果链表
         * @return [*] 成功 >= BlError_E::OK   其他失败
         * @note 如果设置了stMediaDataInfo.freeDataFunc; 内部使用完会自动释放stMediaDataInfo.pchData。
         * @note 阻塞调用
         */
        BlError_E realTime_dataAnalysis(MediaDataInfo_S stMediaDataInfo, std::list<BehaviorAnalyzerResult_S>& listOutInfo);


    protected:

        /**
         * @brief 分析数据
         * @param [MediaDataInfo_S&] stMediaDataInfo: 媒体数据信息
         * @param [std::list<BehaviorAnalyzerResult_S>&] listOutInfo: 分析后的结果信息
         * @return [*] 成功 >= BlError_E::OK   其他失败
         * @note 如果设置了stMediaDataInfo.freeDataFunc; 内部使用完会自动释放stMediaDataInfo.pchData。
         */
        virtual BlError_E dataAnalysis(MediaDataInfo_S& stMediaDataInfo, std::list<BehaviorAnalyzerResult_S>& listOutInfo) = 0;

    private:

        /**
         * @brief 线程函数
         * @return [*] 无
         * @note
         */
        void run();


    protected:

        /* 传入参数 */
        BehaviorAnalyzerInParam_S m_stInParam;

    private:

        /* 线程相关 */
        std::thread       m_threadObj;
        std::atomic<bool> m_bRunning = { true };

        /* 使用模板类操作队列 */
        CDataQueue<MediaDataInfo_S, std::list<BehaviorAnalyzerResult_S>>* m_pDataQueue = nullptr;
    };

}    // namespace BA_NS
