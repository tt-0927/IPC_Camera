/**
 * @FilePath     : face_detect_model.hpp
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2026-09-20
 * @Description  : 人脸检测统一检测层模型（迁移自 Legacy CFaceDetect）。
 *
 * 承载人脸抓拍与人脸比对业务，本模型只负责：
 *   - 挂接统一检测引擎：引擎 CFramePreparer 完成原始帧到 640x640 检测分辨率缩放；
 *   - 异步推理保活：引擎 prepare 帧缓冲跨帧复用，模型持有独立异步 VB 帧池，
 *     memcpy 后投递 CFaceDetectWorker，推理回调期间原始帧经 shared_ptr 保活；
 *   - 链路编排：跟踪（稳定目标 + trackId）→ 域类型目标（face_types.hpp）→
 *     抓拍处理器 / 比对处理器分发，跟踪 id 随目标显式流转；
 *   - 取优枢纽装配：CBestShotHub 为链路级能力（评分后端注入点），抓拍为
 *     第一监听者，比对（最优帧提特征）与实时入库（预留）下期接入；
 *   - 配置面：setAlgoEnCfg 拉取 FaceCapture/FaceCompare 规则并转发处理器。
 *
 * 业务组件归属：推理 Worker（worker/）、抓拍处理器（capture/）与比对处理器
 * （compare/）均为本层独立实现（同名超集类，Legacy 侧保持导入基线）；比对
 * 经取优枢纽订阅 best 帧提特征、原生完成全库比对与事件；名单库服务接口见
 * library/（入库线阶段二落地）。
 *
 * 与 Legacy 差异：线程/队列/缩放由引擎承担；Worker 启停随引擎生命周期（init/unInit）；
 * 人脸名单库入库（addFaceLibGroup）仍走 Legacy 临时实例路径，不经本模型。
 */

#pragma once

#include <array>
#include <atomic>
#include <mutex>
#include <queue>
#include <vector>

#include "detection_engine.hpp"
#include "bestshot_hub.hpp"
#include "face_capture_processor.hpp"
#include "face_chain_types.hpp"
#include "face_detect_worker.hpp"
#include "tracker/detection_tracker.hpp"
#if CAP_AI_FACE_COMPARE
#include "compare/face_compare_processor.hpp"
#endif

/* 人脸检测统一检测层模型 */
class CFaceDetectModel : public IDetectionModel
{
public:
    CFaceDetectModel() = default;
    ~CFaceDetectModel() override;

    /* IDetectionModel 接口 */
    const DetectionModelDescriptor_S &descriptor() const override;
    bool isEnabled() const override;
    bool shouldProcess(int nChannelId) override;
    int process(const ot_video_frame_info *pPreparedFrame, const MediaData_S &stMediaData) override;
    int init() override;
    void unInit() override;
    void setAlgoEnCfg(const Event::AlgorithmConfig &stAlgoConfig) override;

private:
    bool initAsyncFramePool();
    void deinitAsyncFramePool();
    ot_video_frame_info *acquireAsyncFrame();
    void releaseAsyncFrame(ot_video_frame_info *pFrameInfo);

private:
    /* 检测分辨率与帧池容量常量见 face_detect_model.cpp（匿名命名空间） */

    /* 当前是否至少有一项人脸业务生效（抓拍/比对），供引擎 isEnabled 查询 */
    std::atomic<bool> m_bAlgorithmEnabled{ false };
    /*
     * 检测节流：内部固定 333ms（AI 通道 5fps 供帧下实际放行节奏 400ms，
     * 每 2 帧取 1），与外部抓拍间隔解耦；抓拍/事件落盘频率由抓拍处理器
     * 的事件状态机冷却控制，不随本节流变化。
     */
    EventManager m_RecvManager{ 333 };
    /* 人脸抓拍规则配置（缓存最近一次，供处理器判定） */
    Alarm::FaceCapture_S m_stAlgoFaceCapCfg;
    /* 异步 VB 帧池：引擎 prepare 帧跨帧复用，异步推理期间必须持有独立拷贝 */
    std::array<ot_video_frame_info, 5> m_astAsyncFrames; /* 容量上限，实际初始化数由 cpp 侧常量决定 */
    std::queue<ot_video_frame_info *> m_availableAsyncFrames;
    std::mutex m_asyncFramePoolMutex;
    bool m_bAsyncFramePoolInitialized = false;
    /*
     * 取优枢纽：链路级评分选优（模型装配评分后端，每 track 独立 best 桶）。
     * 喂入分两档：抓拍开启时由抓拍处理器喂规则过滤集（含亮度）；抓拍关闭、
     * 仅比对时由本编排喂灵敏度过滤集（亮度未知）。抓拍监听缓存 best 小图，
     * 比对监听 best 帧提特征；实时入库（library/ 预留）阶段二接入。
     */
    CBestShotHub m_bestshotHub;
    /* 人脸抓拍处理器（本层独立实现 capture/，经枢纽消费取优） */
    FaceDetectInternal::CFaceCaptureProcessor m_captureProcessor;
#if CAP_AI_FACE_COMPARE
    /* 人脸比对处理器（本层原生实现 compare/：best 帧提特征 + 全库比对 + 事件） */
    FaceChain_NS::CFaceCompareProcessor m_compareProcessor;
#endif
    /* 异步人脸检测 Worker（本层独立实现 worker/：YOLO 检测 + ArcFace 特征任务串行） */
    CFaceDetectWorker m_detectWorker;
    /*
     * 目标跟踪：给每张人脸分配稳定的编号，连续 3 帧检出才算有效目标
     * （minHits=3）：检测阈值压到 0.2 后偶发的低分误检框也会进来，只看一帧
     * 就确认会把同一张脸认成好几张（板端实测出过一次人脸出 6 张图的 bug），
     * 连续 3 帧才能挡住误检，代价是首次识别慢约 1 秒。人脸消失后保留 8 帧
     * （400ms 检测节拍下约 3.2 秒）才注销编号（maxAge=8），短暂遮挡回来还是
     * 同一个编号，评选中的最佳图也不作废。
     */
    DetectionTracker_NS::CDetectionTracker m_tracker{
        DetectionTracker_NS::CDetectionTracker::Config{ 3, 8, 0.3f, 0 }
    };
};
