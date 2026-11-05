/**
 * @FilePath     : rv1126b_ipc/main_app/ai_app/detect_mode/mouse_algo_detect/mouse_algo_detect.hpp
 * @Date         : 2026-10-23
 * @Description  : 鼠患识别业务算法（继承 CAlgorithm 取流框架，推理内核复用 algorithm_mode 纯检测组件）
 */

#pragma once

#include <atomic>
#include <condition_variable>
#include <mutex>
#include <thread>

#include "algorithm.hpp"
#include "algorithm_define.hpp" /* DetectedTarget_S */
#include "blocking_queue.hpp"

/* 纯检测组件（algorithm_mode/mouse_detect），仅持有指针，前置声明 */
class CMouseDetect;

/**
 * @brief  : 鼠患识别业务算法（CAlgorithm 子类）
 * @note   : 取流框架对照 detect_mode 既有算法（线程 + 阻塞队列 + 500ms 节流 + lazy init），
 *           推理内核调用 algorithm_mode 纯检测组件 CMouseDetect::process()。
 *           判定逻辑（纯帧判定，无跟踪）：
 *           检测结果 → 过滤类别（老鼠）→ 置信度达灵敏度门槛 → 目标位于检测区域内
 *           → 触发鼠患告警（检测到老鼠活动目标即判定鼠患事件）。
 *           报警状态机（algorithm.hpp CAlarmStateMachine）
 *           驱动事件开始/结束走平台联动上报（CEventLinkage）。
 *           配置接入：检测区域（1 个）+ 灵敏度
 *           （setAlgoEnCfg 总开关 + setAlgoParamCfg 详细规则）。
 */
class CMouseAlgoDetect : public CAlgorithm
{
public:
    CMouseAlgoDetect();
    ~CMouseAlgoDetect();

    /**
     * @brief   : 接受媒体数据（CAlgorithm 取流入口）
     * @param    {MediaData_S} stMediaData：媒体数据（NV12）
     * @note    : 内部 500ms 节流后入队，由 run 线程异步消费
     */
    void recvMediaData(MediaData_S stMediaData) override;

    /**
     * @brief   : 更新算法配置参数（CAlgorithm 配置入口）
     * @param    {Event::AlgorithmConfig} &stAlgoConfig：算法开关配置
     * @note    : 鼠患识别启用即拉起取流，并拉取详细配置下发
     */
    void setAlgoEnCfg(const Event::AlgorithmConfig &stAlgoConfig) override;

    /**
     * @brief   : 更新鼠患识别检测参数
     * @param    {Alarm::MouseDetection_S} &stAlgoCfg：鼠患识别配置
     * @note    : 区域由 1920x1080 基准转到 AI 检测帧分辨率后参与判定
     */
    void setAlgoParamCfg(const Alarm::MouseDetection_S &stAlgoCfg);

private:
    /**
     * @brief   : 初始化（创建并加载纯检测组件，run 线程内 lazy 调用，失败可重试）
     * @return   {bool} true：成功 false：失败
     */
    bool init();

    /**
     * @brief   : 反初始化（释放纯检测组件）
     * @return   {bool} true：成功
     * @note    : 重复调用安全
     */
    bool unInit();

    /**
     * @brief   : 数据处理线程（取流 → NV12 转 RGB → 检测 → 过滤判定 → 告警）
     */
    void run();

    /**
     * @brief   : 鼠患判定（类别过滤 + 置信度过滤 + 区域判定 + 状态机告警）
     * @param    {std::vector<DetectedTarget_S>} &vecResults：检测结果（坐标为 AI 帧分辨率）
     * @param    {cv::Mat} &rgbMat：原始分辨率RGB帧 用于编码目标大小图
     * @return   {bool} true：本帧命中鼠患告警条件
     */
    bool processMouseDetect(const std::vector<DetectedTarget_S> &vecResults, const cv::Mat &rgbMat);

private:
    /* 纯检测组件实例（推理内核） */
    CMouseDetect *m_pDetectComponent = nullptr;

    /* 数据队列 */
    BQ_NS::CBlockingQueue<MediaData_S> m_dateQueue;

    /* 用于控制线程的运行 */
    std::atomic<bool>       m_bRunning;
    /* 互斥锁保护指针访问 */
    std::mutex              m_mutex;
    /* 条件变量 */
    std::condition_variable m_condition;
    /* 数据处理线程 */
    std::thread             m_thread;

    /* 检测频率控制（500ms 节流） */
    EventManager m_RecvManager{500};

    /* 鼠患识别相关配置 */
    Alarm::MouseDetection_S m_stAlgoMouseCfg;

    /* 报警状态机：冷却与重报节奏内置（algorithm.hpp），开始/结束自动走 CEventLinkage */
    CAlarmStateMachine m_mouseAlarmStateMachine;

    /* 算法开关（setAlgoEnCfg 驱动） */
    std::atomic<bool> m_bAlgoEnable{false};

    /* 通道号 */
    int m_nChannelId = -1;
};
