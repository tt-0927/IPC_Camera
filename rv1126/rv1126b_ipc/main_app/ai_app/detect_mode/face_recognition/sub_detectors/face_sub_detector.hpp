/*
 * @Author: zhengxh
 * @Date: 2026-11-02
 * @Description: 人脸类事件子检测器契约
 */

#pragma once

#include "face_recognition_define.hpp" /* DetectContext_S */

/**
 * @brief  : 人脸类事件子检测器契约
 * @note   : 主检测器仅输出基础人脸管线结果（检测+质量），子检测器作为纯
 *           消费者按事件特性自持特化模型（如属性/表情）与数据，完成规则
 *           过滤与事件上报；
 *           线程约定：updateConfig/reset 由配置线程调用，process/
 *           isEventEnabled 由算法线程调用，实现内部以互斥锁维护配置快照；
 *           无线程，由主检测器算法线程串行驱动
 */
class IFaceSubDetector
{
public:
    virtual ~IFaceSubDetector() = default;

    /* 配置线程：布防时从事件配置中心自拉配置并更新处理器规则 */
    virtual void updateConfig() = 0;

    /**
     * @brief   : 算法线程：消费一帧基础管线结果，内部完成特化分析→规则过滤→上报
     * @return   {int} 0：成功 -1：模型未就绪 -2：输入非法
     */
    virtual int process(const DetectContext_S &stDetectContext) = 0;

    /* 算法线程：事件使能（详细配置 bEnable），主检测器汇总决定取流开关 */
    virtual bool isEventEnabled() const = 0;

    /* 配置线程：撤防时清理跨布防会话状态（使能快照复位，去重基线等） */
    virtual void reset() = 0;
};
