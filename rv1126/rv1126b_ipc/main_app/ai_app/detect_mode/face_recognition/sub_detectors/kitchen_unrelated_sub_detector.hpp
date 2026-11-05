/*
 * @Author: zhengxh
 * @Date: 2026-11-02
 * @Description: 后厨无关人员进入事件子检测器声明
 */

#pragma once

#include <mutex>
#include <vector>

#include "algorithm.hpp"                       /* CAlarmStateMachine */
#include "face_attribute_model.hpp"            /* CFaceAttributeModel */
#include "face_sub_detector.hpp"               /* IFaceSubDetector */
#include "kitchen_unrelated_person_processor.hpp"

/**
 * @brief  : 后厨无关人员进入事件子检测器
 * @note   : 特化依赖属性分析（口罩判定）：自持属性模型 lazy 加载，先做本帧
 *           属性分析再规则过滤；业务上与人脸识别事件不同时布防，各自持有
 *           属性模型实例（若需并存须上收为共享单例）；
 *           配置自订阅，无线程，由算法线程串行驱动
 */
class CKitchenUnrelatedSubDetector : public IFaceSubDetector
{
public:
    CKitchenUnrelatedSubDetector() = default;
    ~CKitchenUnrelatedSubDetector() override;

    void updateConfig() override;
    int process(const DetectContext_S &stDetectContext) override;
    bool isEventEnabled() const override;
    void reset() override;

private:
    /* lazy 加载属性模型，失败返回 false */
    bool initAttributeModel();

    /* 逐目标属性分析，结果对齐基础目标写入输出数组，模型未就绪整帧跳过 */
    void analyzeAttributes(const DetectContext_S &stDetectContext,
                           std::vector<FaceAttributeTarget_S> &vstAttributeResults,
                           std::vector<bool> &vbAttributeReady);

    CFaceAttributeModel *m_pFaceAttributeHandle = nullptr; /* 属性模型 */
    CKitchenUnrelatedPersonProcessor m_processor;          /* 规则判断器 */
    CAlarmStateMachine m_stateMachine;                     /* 报警状态机 */

    mutable std::mutex m_mutexConfig;                      /* updateConfig 与算法线程互斥 */
    bool m_bEnable = false;                                /* 配置快照：事件使能 */
};
