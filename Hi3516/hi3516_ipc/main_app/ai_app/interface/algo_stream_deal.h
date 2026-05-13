/**
 * @FilePath     : algo_stream_deal.h
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2025-06-06 16:02:10
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-04-23 09:42:28
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
#include "audio_detect.hpp"
#include "people_head_detect.hpp"
#include "motion_detect.hpp"
#include "hide_detect.hpp"
#include "hvf_detect.hpp"
#include "item_detect.hpp"
#include "pet_recognition.hpp"
// #include "parking_detect.hpp"
#include "scene_change_detect.hpp"
#include "face_detect.hpp"
#if CAP_AI_GARBAGE_DETECT
#include "garbage_detect.hpp"
#endif

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
     * @param    {ot_video_frame_info} *pFrameInfo 视频图像帧信息
     */
    void deal_message(const ot_video_frame_info *pFrameInfo);

    /**
     * @brief   : 处理stream传过来的数据
     * @param    {ot_audio_frame} *pFrame 音频帧信息
     */
    void deal_message(const ot_audio_frame *pFrame);

    /**
     * @brief   : 处理stream传过来的数据
     * @param    {void} *pData 视频图像帧数据指针
     * @param    {int} nLength 字节数
     * @param    {int} nWidth 图像宽度
     * @param    {int} nHeight 图像高度
     */
    void deal_videoStreamData(const void *pData, int nLength, int nWidth, int nHeight);

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
     * @brief 添加人脸名单库
     * @param stFaceLibData 
     */
     bool add_Facelib_Groups(FaceDataDB_NS::FaceLibsInfo_S &stFaceLibData);

      /**
     * @brief 人员检索人脸比对
     * @param strPicPath 
     */
    void compare_Face_Retrieval(std::string strPicPath);
 
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

    /**
     * @brief   : 分发运行时命令至当前算法实例
     * @param    {RuntimeCommand_S} &stCommand：运行时命令
     * @return   {int} OK：至少一个算法处理成功 ERR：无算法处理成功
     */
    int dispatchRuntimeCommand(const RuntimeCommand_S &stCommand);

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
     * @brief   : 初始化事件统计上报器
     * @return   {void}
     */
    void initEventStatisticsReporter();

    /**
     * @brief 只能执行一次的成员函数
     */
    void runOnce();

private:
    //info /*----------------------- 模块句柄 -----------------------*/
    /*音视频流处理句柄*/
    std::shared_ptr<CAVHandler> m_StreamHandler;
    /**
     * @brief   : 普通事件
     */
    /* 移动侦测算法句柄 */
    std::shared_ptr<CAlgorithm> m_pMotionAlgo;
    /* 遮挡侦测算法句柄 */
    std::shared_ptr<CAlgorithm> m_pHideAlgo;
    /**
     * @brief   : 周界事件
     */

    /**
    * @brief   : Smart事件
    */
    /* 音频异常侦测算法句柄 */
    std::shared_ptr<CAlgorithm> m_pAudioAlgo;
    /* 场景变更侦测算法句柄 */
    std::shared_ptr<CAlgorithm> m_pSceneChangeAlgo;
    /* 脸人车侦测算法句柄*/
    std::shared_ptr<CAlgorithm> m_pHVFAlgo;
    /* 人头侦测算法句柄，承载人员聚集与人员密度检测 */
    std::shared_ptr<CAlgorithm> m_pPeopleHeadAlgo;
    /* 事件统计上报器，默认空实现，后续可替换为 TVSDK 适配器 */
    std::shared_ptr<EventStatistics_NS::IEventStatisticsReporter> m_pEventStatisticsReporter;
    /* 物品侦测算法句柄 物品遗留、物品拿取侦测*/
    std::shared_ptr<CAlgorithm> m_pItemAlgo;
    /* 宠物识别算法句柄*/
    std::shared_ptr<CAlgorithm> m_pPetAlgo;
    /* 人脸侦测算法句柄 人脸抓拍*/
    std::shared_ptr<CAlgorithm> m_pFaceAlgo;
#if CAP_AI_GARBAGE_DETECT
    /* 垃圾检测算法句柄 垃圾暴露、垃圾满溢*/
    std::shared_ptr<CAlgorithm> m_pGarbageAlgo;
#endif

    // note 不使用自研模型
    // /* 停车侦测算法句柄 */
    // std::shared_ptr<CAlgorithm> m_pParkAlgo;
    //info /*----------------------- 参数变量 -----------------------*/
};
