/**
 * @FilePath     : rv1126b_ipc/main_app/ai_app/detect_mode/access_control_passage_detect/access_control_passage_detect.hpp
 * @Date         : 2026-10-22
 * @Description  : 门禁异常通行检测算法（继承 CAlgorithm 取流框架，推理内核复用攀爬检测纯组件）
 */

#pragma once

#include <atomic>
#include <condition_variable>
#include <mutex>
#include <thread>
#include <vector>

#include <opencv2/opencv.hpp>

#include "algorithm.hpp"
#include "algorithm_define.hpp" /* DetectedTarget_S */
#include "blocking_queue.hpp"

/* 攀爬检测纯组件（algorithm_mode/climb_detect_model），仅持有指针，前置声明 */
class CClimbDetectModel;

/**
 * @brief  : 门禁异常通行检测业务算法（CAlgorithm 子类）
 * @note   : 取流框架对照 detect_mode 既有算法（线程 + 阻塞队列 + 500ms 节流 + lazy init），
 *           推理内核调用攀爬检测纯组件 CClimbDetectModel::process()。
 *           判定语义：检测到攀爬目标且框脚点落在配置区域内即触发（区域无效不参与判定），
 *           置信度门槛由灵敏度[1,100]反向映射；报警去抖/冷却/上报由 CAlarmStateMachine 内置。
 */
class CAccessControlPassageDetect : public CAlgorithm
{
public:
    CAccessControlPassageDetect();
    ~CAccessControlPassageDetect();

    /**
     * @brief   : 接受媒体数据（CAlgorithm 取流入口）
     * @param    {MediaData_S} stMediaData：媒体数据（NV12）
     * @note    : 内部 500ms 节流后入队，由 run 线程异步消费
     */
    void recvMediaData(MediaData_S stMediaData) override;

    /**
     * @brief   : 更新算法配置参数（CAlgorithm 配置入口）
     * @param    {Event::AlgorithmConfig} &stAlgoConfig：算法开关配置
     * @note    : 启用即拉起取流；启用时拉取详细配置下发判定参数
     */
    void setAlgoEnCfg(const Event::AlgorithmConfig &stAlgoConfig) override;

    /**
     * @brief   : 更新门禁异常通行检测参数
     * @param    {Alarm::AccessControlAbnormalPassage_S} &stAlgoCfg：门禁异常通行配置
     * @note    : 区域由 1920x1080 基准转到 AI 检测帧分辨率；灵敏度映射置信度门槛
     */
    void setAlgoParamCfg(const Alarm::AccessControlAbnormalPassage_S &stAlgoCfg);

private:
    /**
     * @brief   : 初始化（创建并加载攀爬检测组件，run 线程内 lazy 调用，失败可重试）
     * @return   {bool} true：成功 false：失败
     */
    bool init();

    /**
     * @brief   : 反初始化（释放攀爬检测组件）
     * @return   {bool} true：成功
     * @note    : 重复调用安全
     */
    bool unInit();

    /**
     * @brief   : 数据处理线程（取流 → NV12 转 RGB → 检测 → 区域判定 → 报警）
     */
    void run();

    /**
     * @brief   : 检测结果区域判定并交报警状态机
     * @param    {std::vector<DetectedTarget_S>} &vecResults：攀爬检测输出
     * @param    {cv::Mat} &rgbMat：原始分辨率RGB帧 用于编码人员大图小图
     */
    void processPassage(const std::vector<DetectedTarget_S> &vecResults, const cv::Mat &rgbMat);

    /**
     * @brief   : 配置区域转判定多边形（区域无效不参与判定）
     * @note    : 配置区域为 1920x1080 基准，需转换到 AI 检测帧分辨率
     */
    void updateRegionPolygon();

    /**
     * @brief   : 灵敏度[1,100]反向映射置信度门槛（与 region_stay_processor 同式同参数）
     */
    static float sensitivityToConfidence(unsigned int nSensitivity);

    /**
     * @brief   : 目标框底部中心点是否在区域多边形内（空多边形=区域无效，恒为否）
     */
    bool isTargetInRegion(const DetectedTarget_S &stTarget) const;

private:
    /* 攀爬检测纯组件实例（推理内核） */
    CClimbDetectModel *m_pDetectComponent = nullptr;

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

    /* 配置参数 */
    Alarm::AccessControlAbnormalPassage_S m_stCfg;

    /* 区域多边形（AI 检测帧分辨率，空=区域无效不参与判定） */
    std::vector<cv::Point> m_vecRegionPts;

    /* 置信度门槛（灵敏度映射） */
    float m_fConfGate = 0.5f;

    /* 报警状态机：冷却与重报节奏内置（algorithm.hpp），开始/结束自动走 CEventLinkage */
    CAlarmStateMachine m_alarmStateMachine;

    /* 算法开关（setAlgoEnCfg 驱动） */
    std::atomic<bool> m_bAlgoEnable{false};
};
