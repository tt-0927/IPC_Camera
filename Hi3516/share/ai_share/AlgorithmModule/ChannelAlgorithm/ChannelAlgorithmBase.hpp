/*
 * @FilePath     : ChannelAlgorithmBase.hpp
 * @Author       : 严泽辉 yanzeh@kfb.cn
 * @Date         : 2024-03-08 10:36:45
 * @LastEditors  : 严泽辉 yanzeh@kfb.cn
 * @LastEditTime : 2024-03-08 15:59:09
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
#include "ChannelAlgorithmExtern.hpp"
#include "DataQueue.hpp"

namespace ChannelAlgorithm_NS
{
    class CChannelAlgorithmBase
    {
    public:

        CChannelAlgorithmBase(InParam_S stInParam);
        virtual ~CChannelAlgorithmBase();

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
         * @param [std::list<AnalyzerResult_S>&] listOutInfo: 分析结果链表
         * @return [*] 成功 >= BlError_E::OK   其他失败
         * @note
         */
        BlError_E read_analysisResult(std::list<AnalyzerResult_S>& listOutInfo);

        /**
         * @brief 实时分析数据
         * @param [MediaDataInfo_S] stMediaDataInfo: 媒体数据信息
         * @param [std::list<AnalyzerResult_S>&] listOutInfo: 分析结果链表
         * @return [*] 成功 >= BlError_E::OK   其他失败
         * @note 如果设置了stMediaDataInfo.freeDataFunc; 内部使用完会自动释放stMediaDataInfo.pchData。
         * @note 阻塞调用
         */
        BlError_E realTime_dataAnalysis(MediaDataInfo_S stMediaDataInfo, std::list<AnalyzerResult_S>& listOutInfo);

        /**
         * @brief 设置桥梁基准线
         * @param [list<PosInfo_S>] listPos: 基准线坐标
         * @return [*] 成功 >= BlError_E::OK   其他失败
         * @note
         */
        virtual BlError_E setDatumLine(std::list<PosInfo_S> listPos) = 0;

    protected:

        /**
         * @brief 分析数据
         * @param [MediaDataInfo_S&] stMediaDataInfo: 媒体数据信息
         * @param [std::list<AnalyzerResult_S>&] listOutInfo: 分析后的结果信息
         * @return [*] 成功 >= BlError_E::OK   其他失败
         * @note 如果设置了stMediaDataInfo.freeDataFunc; 内部使用完会自动释放stMediaDataInfo.pchData。
         */
        virtual BlError_E dataAnalysis(MediaDataInfo_S& stMediaDataInfo, std::list<AnalyzerResult_S>& listOutInfo) = 0;



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
        CDataQueue<MediaDataInfo_S, std::list<AnalyzerResult_S>>* m_pDataQueue = nullptr;
    };

}    // namespace ChannelAlgorithm_NS
