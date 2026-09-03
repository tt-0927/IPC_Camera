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

#ifdef DEVICE_TV_3882TI
#include "visionText_manage.hpp"
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
     * @param    {void*} pData
     * @param    {int} nLength
     * @param    {int} nH
     * @param    {int} nW
     */
    void deal_message(const void* pData, int nLength, int nH, int nW);
    
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
    /**
    * @brief   : 普通事件
    */
    /* 移动侦测句柄 */
    std::shared_ptr<CAlgorithm> m_pMotionAlgo;
    /* 遮挡侦测算法句柄 */
    std::shared_ptr<CAlgorithm> m_pHideAlgo;

    /**
    * @brief   : 周界事件、Smart事件
    */
    /* notHelmet(未戴安全帽)、helmet(安全帽)、reflective(反光衣)、safetyRope(安全绳)、exposedSoil(泥土裸露)、person(人) */
    std::shared_ptr<CAlgorithm> m_pGroup1Algo;
    /* penson(人)、Car(机动车)、NonCar(非机动车) */
    std::shared_ptr<CAlgorithm> m_pGroup2AndGroup4Algo;
    /* smoke(烟雾)、fire(火焰)、Overflow(垃圾满溢)、expose(垃圾暴露)、Complete(井盖完好)、Damaged(井盖破损)、Lost(井盖丢失)、Uncovered(未盖井盖)、BreakoutOfOuterEdge(井盖外边沿破损)、WaterAccumulation(道路积水) */
    std::shared_ptr<CAlgorithm> m_pGroup3Algo;
    // /* cigarette(香烟)、sleep(睡觉)、phone(玩手机)、fall(摔跤)、falling(摔跤中) */
    // std::shared_ptr<CAlgorithm> m_pGroup4Algo;
    /* metalFence(金属栅栏)、ConeTank(锥形桶)、CrashBarrels(防撞桶)、fence(防护栏) */
    std::shared_ptr<CAlgorithm> m_pGroup5Algo;
    /* 物品检测句柄 */
    std::shared_ptr<CAlgorithm> m_pObjectDetectAlgo;
    /* 场景变更侦测算法句柄 */
    std::shared_ptr<CAlgorithm> m_pSceneChangeAlgo;
    /* 宠物识别算法句柄*/
    std::shared_ptr<CAlgorithm> m_pPetAlgo;
    /* 音频异常侦测算法句柄*/
    std::shared_ptr<CAlgorithm> m_pAudioAlgo;
    /* 人脸检测句柄 */
    std::shared_ptr<CAlgorithm> m_pFaceDetectAlgo;

    /**
    * @brief   : 场景智能分析事件
    */
    /* 画面及文字分析算法句柄 */
    std::shared_ptr<CAlgorithm> m_pVisionTextAlgo;


#if 0
    /* 行人检测句柄 */
    std::shared_ptr<CAlgorithm> m_pPersonDetectAlgo;
    /* 火焰检测句柄 */
    std::shared_ptr<CAlgorithm> m_pFireDetectAlgo;
    /* 街道检测句柄 */
    std::shared_ptr<CAlgorithm> m_pStreeDetectAlgo;
    /* 垃圾检测句柄 */
    std::shared_ptr<CAlgorithm> m_pRubishDetectAlgo;
    /* 睡岗识别句柄 */
    std::shared_ptr<CAlgorithm> m_pSleepDetectAlgo;
    /* 摔倒识别句柄 */
    std::shared_ptr<CAlgorithm> m_pTripDetectAlgo;
    // /* 玩手机识别句柄 */
    // std::shared_ptr<CAlgorithm> m_pPhoneUsageAlgo;
    /* 施工占道检测句柄 */
    std::shared_ptr<CAlgorithm> m_pConstructionEncroachmentRoadDetectAlgo;
    /* 高空安全带黄土裸露检测句柄 */
    std::shared_ptr<CAlgorithm> m_pHighSafyBeltSoilDetecAlgo;
    /* 安全帽检测句柄 */
    std::shared_ptr<CAlgorithm> m_pHelmetAlgo;
    /* 反光衣检测句柄 */
    std::shared_ptr<CAlgorithm> m_pReflectiveAlgo;
    /* 抽烟检测句柄 */
    std::shared_ptr<CAlgorithm> m_pSmokingAlgo;
    /* 防护栏检测句柄 */
    std::shared_ptr<CAlgorithm> m_pFenceAlgo;
    /* 机动车、行人、非机动车侦测算法句柄 */
    std::shared_ptr<CAlgorithm> m_pPersonMotorNomotorAlgo;
    /* 电瓶车检测句柄 */
    std::shared_ptr<CAlgorithm> m_pElectricScooterDetectAlgo;
    /* 车辆侦测算法句柄*/
    std::shared_ptr<CAlgorithm> m_pVehicleAlgo;
    /* 车牌识别检测句柄 */
    std::shared_ptr<CAlgorithm> m_pLicensePlateCognitionDetectAlgo;
#endif
};
