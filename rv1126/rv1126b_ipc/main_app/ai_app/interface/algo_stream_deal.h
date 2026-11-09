/**
 * @FilePath     : algo_stream_deal.h
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2025-06-06 16:02:10
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2025-08-14 15:24:29
 * @Description  : 负责取流传给算法检测
 */

#pragma once

#include "IpcRet.h"
#include "IOBase.h"
#include "Singleton.h"
#include <iostream>
#include <condition_variable>
#include "dlog.h"
#include "action_code.h"
#include "common_define.h"
#include "stream_handler.hpp"
#include "algorithm.hpp"



class CAlgoStreamDeal : public CSingleton<CAlgoStreamDeal>
{
    CAlgoStreamDeal() = default;
public:
    ~CAlgoStreamDeal() = default;

    friend class CSingleton<CAlgoStreamDeal>;

    /**
     * @brief   : 算法音视频流数据处理初始化
     * @return   {int}
     */
    int init();

    /**
     * @brief   : 算法音视频流数据处理去初始化
     */
    void deinit();
    
    /**
     * @brief   : 处理stream传过来的数据
     * @param    {void*} pData
     * @param    {int} nLength
     * @param    {int} nH
     * @param    {int} nW
     * @param    {uint64_t} u64PTS 帧时间戳(us)
     */
    void deal_message(const void* pData, int nLength, int nH, int nW, uint64_t u64PTS = 0);
    
    /**
     * @brief   :处理stream传过来的音频数据
     * @param    {void*} pData 音频帧数据指针
     * @param    {int} nLength 字节数
     */
    void deal_audioStreamData(const void *pData, int nLength);

    /**
     * @brief 绑定回调
     */
    void bindRecvFunc(Event::AlgorithmConfig &stAlgoConfig);

    /**
     * @brief 算法配置更新
     * @param stAlgoConfig 
     */
    void set_Algo_EnConfig(Event::AlgorithmConfig &stAlgoConfig);
    
    /**
     * @brief 配置算法参数
     * @param stAlgoCfg 
     */
    // void set_Algo_ParamConfig(Alarm::TargetDetection_S &stAlgoCfg);

    /**
     * @brief   : 获取当前实时音量
     * @note    : 音频异常侦测
     * @return   {float} 当前音量(dB)
     */
    float getCurrentDb() const;

private:
    /**
     * @brief   : 绑定视频槽函数
     * @param    {CAVHandler} *p_Handler：函数句柄
     * @param    {CONTEXT} *context：槽函数所在实例指针
     * @param    {signal_function<CONTEXT, Args...>} slot：槽函数指针
     * @return   {bool}
     */
    template <typename CONTEXT, typename... Args>
    bool bindVideoSlot(CAVHandler *p_Handler, CONTEXT *context, signal_function<CONTEXT, Args...> slot)
    {
        if (p_Handler)
        {
            /* 关联信号与槽 */
            connect(&p_Handler->m_sendVideoDataSig, context, slot, false);
            return true;
        }
        return false;
    }

    /**
     * @brief   : 绑定音频槽函数
     * @param    {CAVHandler} *p_Handler：函数句柄
     * @param    {CONTEXT} *context：槽函数所在实例指针
     * @param    {signal_function<CONTEXT, Args...>} slot：槽函数指针
     * @return   {bool}
     */
    template <typename CONTEXT, typename... Args>
    bool bindAudioSlot(CAVHandler *p_Handler, CONTEXT *context, signal_function<CONTEXT, Args...> slot)
    {
        if (p_Handler)
        {
            /* 关联信号与槽 */
            connect(&p_Handler->m_sendAudioDataSig, context, slot, false);
            return true;
        }
        return false;
    }

    /**
     * @brief   : 解绑定视频槽函数
     * @param    {CAVHandler} *p_Handler：函数句柄
     * @return   {bool}
     */
    bool unbindVideoSig(CAVHandler *p_Handler)
    {
        if (p_Handler)
        {
            /* 关联信号与槽 */
            disconnect(&p_Handler->m_sendVideoDataSig);
            return true;
        }
        return false;
    }

    /**
     * @brief   : 解绑定音频槽函数
     * @param    {CAVHandler} *p_Handler：函数句柄
     * @return   {bool}
     */
    bool unbindAudioSig(CAVHandler *p_Handler)
    {
        if (p_Handler)
        {
            /* 关联信号与槽 */
            disconnect(&p_Handler->m_sendAudioDataSig);
            return true;
        }
        return false;
    }


    /**
     * @brief 根据配置管理算法实例
     */
    void manageAlgorithmInstances(const Event::AlgorithmConfig &stAlgoConfig);

    /**
     * @brief 管理单个算法的生命周期
     * @param algoMap 算法映射表
     * @param bEnabled 是否启用该算法
     * @param creator 算法创建函数
     */
    bool manageSingleAlgorithm(std::shared_ptr<CAlgorithm> &algoMap, bool bEnabled, std::function<std::shared_ptr<CAlgorithm>()> creator);

    /**
     * @brief 只能执行一次的成员函数
     */
    void runOnce();

private:
    //info /*----------------------- 模块句柄 -----------------------*/
    /*音视频流处理句柄*/
    std::shared_ptr<CAVHandler> m_StreamHandler;

    /* 课堂分析算法句柄（仅定义课堂配置宏的型号存在） */
#if defined(CAP_TV3881TJY_CLASSROOM_CONFIG) && CAP_TV3881TJY_CLASSROOM_CONFIG
    std::shared_ptr<CAlgorithm> m_pClassroomAlgo;
#endif
};
