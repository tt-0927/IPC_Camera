/**
 * @FilePath     : detection_engine.hpp
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2026-08-25 11:49:38
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-08-25 13:50:00
 * @Description  : 统一检测引擎（模型接口 + 帧准备器 + 引擎核心）
 */

#pragma once

#include <atomic>
#include <chrono>
#include <cstdint>
#include <memory>
#include <thread>
#include <unordered_map>
#include <vector>

#include "IpcRet.h"
#include "algorithm.hpp"
#include "blocking_queue.hpp"
#include "common_define.h"
#include "stream_process_ext.hpp"

extern "C"
{
    #include "mpp_vgs.h"
}

/* ==================== detection_model（内联） ==================== */

/* 检测模型描述 */
struct DetectionModelDescriptor_S
{
    /* 模型名，日志与诊断用 */
    const char *pName = nullptr;
    /* 模型输入分辨率（MD/OD 为 1024×576） */
    uint32_t nInputWidth = 0;
    uint32_t nInputHeight = 0;
    /* MD/OD 需要按配置区域裁剪 */
    bool bNeedCrop = false;
    /* 同帧内执行顺序，值小先执行 */
    int nOrder = 0;
    /* 启动延时（毫秒），引擎 Worker 启动后先延时再取帧，
     * 沿用 LEGACY 防首次检测误报语义（MD 4000 / OD 2000） */
    uint32_t nStartupDelayMs = 0;
};

/**
 * @brief   : 检测模型接口
 * @note    : 引擎负责线程/队列/帧准备调度，模型负责自身 SDK 生命周期与事件判断。
 *            模型注册必须在引擎 start() 之前完成；引擎 Worker 线程是模型
 *            init/unInit/process 的唯一调用者。
 */
class IDetectionModel
{
public:
    virtual ~IDetectionModel() = default;

    /**
     * @brief   : 获取模型描述（名称、输入分辨率、是否裁剪、执行顺序、启动延时）
     * @return   {const DetectionModelDescriptor_S} &：模型描述
     */
    virtual const DetectionModelDescriptor_S &descriptor() const = 0;

    /**
     * @brief   : 是否使能（配置总开关 + 区域有效性）
     * @return   {bool} true：使能，引擎据此决定懒初始化与分发
     */
    virtual bool isEnabled() const = 0;

    /**
     * @brief   : 帧率控制，由模型自行判断本帧是否处理
     * @param    {int} nChannelId：通道号
     * @return   {bool} true：处理本帧 false：跳过本帧
     */
    virtual bool shouldProcess(int nChannelId) = 0;

    /**
     * @brief   : 引擎完成帧准备后调用
     * @param    {const ot_video_frame_info} *pPreparedFrame：缩放/裁剪后的帧
     * @param    {const MediaData_S} &stMediaData：原始媒体数据（含源帧引用）
     * @return   {int} OK：成功，非 OK：失败（引擎限频 WARN 后继续下一模型）
     */
    virtual int process(const ot_video_frame_info *pPreparedFrame,
                        const MediaData_S &stMediaData) = 0;

    /**
     * @brief   : 初始化（懒初始化，仅 Worker 线程调用）
     * @return   {int} OK：成功，非 OK：失败（引擎 1s 后重试）
     */
    virtual int init() = 0;

    /**
     * @brief   : 反初始化（仅 Worker 线程调用）
     */
    virtual void unInit() = 0;

    /* ---------------- 可选扩展接口（默认空实现） ---------------- */

    /**
     * @brief   : 配置路由，引擎将算法总配置转发给模型
     * @param    {const Event::AlgorithmConfig} &stAlgoConfig：算法总配置
     */
    virtual void setAlgoEnCfg(const Event::AlgorithmConfig &stAlgoConfig)
    {
        (void)stAlgoConfig;
    }

    /**
     * @brief   : 事件统计上报器注入（MD/OD 暂不使用，保留扩展位）
     * @param    {std::shared_ptr<EventStatistics_NS::IEventStatisticsReporter>} pReporter：上报器
     */
    virtual void setEventStatisticsReporter(
        const std::shared_ptr<EventStatistics_NS::IEventStatisticsReporter> &pReporter)
    {
        (void)pReporter;
    }

    /**
     * @brief   : 运行时命令路由
     * @param    {const RuntimeCommand_S} &stCommand：运行时命令
     * @return   {int} OK：处理成功 ERR：未处理或失败
     */
    virtual int handleRuntimeCommand(const RuntimeCommand_S &stCommand)
    {
        (void)stCommand;
        return ERR;
    }

    /**
     * @brief   : 配置变更导致 SDK 句柄需要重建时返回 true
     * @note    : 引擎在分发前检查，发现为 true 时执行 unInit + init 重建句柄；
     *            模型应在 init() 开始时清除该状态，初始化期间到达的新配置会
     *            重新置位，由引擎下一轮再次重建。
     * @return   {bool} true：需要重建
     */
    virtual bool needsReinit() const
    {
        return false;
    }

    /**
     * @brief   : 获取裁剪区域（缩放后算法坐标系）
     * @return   {const Common::Rect_S} *：裁剪区域指针，引擎立即快照使用；无裁剪返回 nullptr
     */
    virtual const Common::Rect_S *crop_rect() const
    {
        return nullptr;
    }
};

/* ==================== frame_preparer（内联） ==================== */

/**
 * @brief   : 帧准备器
 * @note    : 按模型声明的目标尺寸执行 mppVgs_scale/mppVgs_crop；
 *            同一帧周期内相同准备参数复用结果（begin_frame 后失效）；
 *            缩放/裁剪缓冲按 (宽,高) 懒创建并复用，避免不同模型区域
 *            交替时每帧重建 VB。
 */
class CFramePreparer
{
public:
    CFramePreparer() = default;
    ~CFramePreparer();

    /* 禁止拷贝 */
    CFramePreparer(const CFramePreparer &) = delete;
    CFramePreparer &operator=(const CFramePreparer &) = delete;

    /**
     * @brief   : 帧周期开始，失效本帧准备缓存
     * @note    : 引擎每处理一帧调用一次；同帧内相同参数的 prepare 复用结果，
     *            不同参数的 prepare 各自执行 VGS 操作
     */
    void begin_frame();

    /**
     * @brief   : 准备检测帧（先缩放到目标分辨率，必要时再按区域裁剪）
     * @param    {const ot_video_frame_info} *pSrcFrame：源帧
     * @param    {uint32_t} nDstW：目标宽度（算法输入分辨率）
     * @param    {uint32_t} nDstH：目标高度
     * @param    {const Common::Rect_S} *pCropRect：裁剪区域（缩放后坐标系），
     *                                               nullptr 表示不裁剪
     * @param    {ot_video_frame_info} **ppOut：输出帧指针
     * @return   {int} OK：成功，非 OK：失败（调用方跳过该模型本帧）
     */
    int prepare(const ot_video_frame_info *pSrcFrame, uint32_t nDstW, uint32_t nDstH,
                const Common::Rect_S *pCropRect, ot_video_frame_info **ppOut);

    /**
     * @brief   : 释放全部缩放/裁剪缓冲（引擎停止时调用）
     */
    void destroy();

private:
    /**
     * @brief   : 获取指定尺寸的缩放缓冲（懒创建，尺寸变化时重建）
     * @param    {uint32_t} nWidth：目标宽度
     * @param    {uint32_t} nHeight：目标高度
     * @return   {ot_video_frame_info} *：缓冲指针，失败返回 nullptr
     */
    ot_video_frame_info *get_scale_buffer(uint32_t nWidth, uint32_t nHeight);

    /**
     * @brief   : 获取指定尺寸的裁剪缓冲（懒创建，尺寸变化时重建）
     * @param    {uint32_t} nWidth：裁剪宽度
     * @param    {uint32_t} nHeight：裁剪高度
     * @return   {ot_video_frame_info} *：缓冲指针，失败返回 nullptr
     */
    ot_video_frame_info *get_crop_buffer(uint32_t nWidth, uint32_t nHeight);

    /**
     * @brief   : 从缓冲池获取帧缓冲，不存在或尺寸不符时创建
     * @param    {std::unordered_map<uint64_t, std::unique_ptr<ot_video_frame_info>>} &mapBuffers：缓冲池
     * @param    {uint32_t} nWidth：目标宽度
     * @param    {uint32_t} nHeight：目标高度
     * @return   {ot_video_frame_info} *：缓冲指针，失败返回 nullptr
     */
    ot_video_frame_info *acquire_buffer(
        std::unordered_map<uint64_t, std::unique_ptr<ot_video_frame_info>> &mapBuffers,
        uint32_t nWidth, uint32_t nHeight);

    /* memory: 缩放缓冲池，key=(宽<<32)|高，VB 生命周期由本类管理，destroy 统一释放 */
    std::unordered_map<uint64_t, std::unique_ptr<ot_video_frame_info>> m_mapScaleBuffers;
    /* memory: 裁剪缓冲池，key=(宽<<32)|高，VB 生命周期由本类管理，destroy 统一释放 */
    std::unordered_map<uint64_t, std::unique_ptr<ot_video_frame_info>> m_mapCropBuffers;

    /* 本帧完整准备缓存（begin_frame 时失效，同帧同参数复用） */
    bool m_bFrameCacheValid = false;
    uint32_t m_nFrameCacheDstW = 0;
    uint32_t m_nFrameCacheDstH = 0;
    bool m_bFrameCacheHasCrop = false;
    Common::Rect_S m_stFrameCacheCropRect;
    ot_video_frame_info *m_pFrameCacheResult = nullptr;

    /* 本帧缩放缓存（不同裁剪区域间共享同一缩放结果） */
    bool m_bScaleCacheValid = false;
    const ot_video_frame_info *m_pScaleCacheSrc = nullptr;
    uint32_t m_nScaleCacheDstW = 0;
    uint32_t m_nScaleCacheDstH = 0;
    ot_video_frame_info *m_pScaleCacheResult = nullptr;
};

/* ==================== unified_detection_engine（内联） ==================== */

// inlined: detection_model / frame_preparer

/**
 * @brief   : 统一检测引擎
 * @note    : 拥有该通道唯一的检测 Worker 线程与 latest-wins 帧队列，
 *            负责模型注册、懒初始化/重初始化、帧准备协调与串行分发；
 *            不接触具体 SDK，SDK 生命周期由模型适配器管理。
 *            模型注册必须在 start() 之前完成；引擎以 CAlgorithm 接口
 *            接入现有信号槽体系（recvMediaData/setAlgoEnCfg）。
 */
class CUnifiedDetectionEngine : public CAlgorithm
{
public:
    CUnifiedDetectionEngine();
    ~CUnifiedDetectionEngine() override;

    /* 禁止拷贝 */
    CUnifiedDetectionEngine(const CUnifiedDetectionEngine &) = delete;
    CUnifiedDetectionEngine &operator=(const CUnifiedDetectionEngine &) = delete;

    /* ---------------- CAlgorithm 接口 ---------------- */

    /**
     * @brief   : 接受媒体数据（流线程调用，入 latest-wins 队列）
     * @param    {MediaData_S} stMediaData：媒体数据
     */
    void recvMediaData(MediaData_S stMediaData) override;

    /**
     * @brief   : 算法总配置更新（配置线程调用，转发给全部模型）
     * @param    {const Event::AlgorithmConfig} &stAlgoConfig：算法配置
     */
    void setAlgoEnCfg(const Event::AlgorithmConfig &stAlgoConfig) override;

    /**
     * @brief   : 注入事件统计上报器（配置线程调用，转发给全部模型）
     * @param    {std::shared_ptr<EventStatistics_NS::IEventStatisticsReporter>} pReporter：上报器
     */
    void setEventStatisticsReporter(
        const std::shared_ptr<EventStatistics_NS::IEventStatisticsReporter> &pReporter) override;

    /**
     * @brief   : 分发运行时命令至全部模型
     * @param    {const RuntimeCommand_S} &stCommand：运行时命令
     * @return   {int} OK：至少一个模型处理成功 ERR：无模型处理成功
     */
    int handleRuntimeCommand(const RuntimeCommand_S &stCommand) override;

    /* ---------------- 检测引擎接口 ---------------- */

    /**
     * @brief   : 注册检测模型（必须在 start() 之前调用）
     * @param    {std::unique_ptr<IDetectionModel>} pModel：模型实例
     * @note    : 注册顺序即同帧执行顺序（引擎按 descriptor.nOrder 升序稳定排序）
     */
    void register_model(std::unique_ptr<IDetectionModel> pModel);

    /**
     * @brief   : 启动检测 Worker 线程（注册完成后调用，幂等）
     */
    void start();

    /**
     * @brief   : 停止 Worker 并反初始化全部模型（幂等，析构自动调用）
     * @note    : 停止顺序：置运行标志 false -> shutdown 队列唤醒 Worker
     *            -> join -> 全部模型 unInit -> 释放帧准备缓冲
     */
    void shutdown();

private:
    /* 模型条目：模型实例 + 引擎侧初始化状态 */
    struct ModelEntry_S
    {
        std::unique_ptr<IDetectionModel> pModel;
        /* 是否已完成初始化（引擎侧状态，仅 Worker 线程访问） */
        bool bInitialized = false;
    };

    /**
     * @brief   : Worker 线程函数
     */
    void run();

    /**
     * @brief   : 是否存在已使能模型（供收帧与 Worker 空转判断）
     * @return   {bool} true：存在
     */
    bool has_enabled_model() const;

    /* 数据队列（容量 2，latest-wins 防积压） */
    BQ_NS::CBlockingQueue<MediaData_S> m_dateQueue;
    /* 运行标志（控制 Worker 循环） */
    std::atomic<bool> m_bRunning{false};
    /* 是否已启动（注册模型与线程启动的顺序保护） */
    std::atomic<bool> m_bStarted{false};
    /* 检测 Worker 线程 */
    std::thread m_thread;
    /* 已注册模型（仅在 start() 前注册，Worker 只读，无需加锁） */
    std::vector<ModelEntry_S> m_vModels;
    /* 帧准备器（缩放/裁剪缓冲与帧周期缓存） */
    CFramePreparer m_framePreparer;
};