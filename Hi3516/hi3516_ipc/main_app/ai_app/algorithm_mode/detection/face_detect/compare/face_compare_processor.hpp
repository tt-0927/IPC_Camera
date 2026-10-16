/**
 * @FilePath     : face_compare_processor.hpp
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2026-09-21
 * @Description  : 人脸比对处理器——统一检测层原生实现。
 *
 * 特征提取按人走最佳帧（D5）：实现 IFaceBestShotListener 订阅取优枢纽，
 * 某目标评出新的最佳帧时同步提特征并缓存于该目标名下（约 2KB/人，随轨迹
 * 结束/全清同步清理），process 每帧只做"查缓存 → 全库余弦 → 判定 → 事件"，
 * 不再每帧每目标当帧裁剪直提。
 *
 * 判定口径统一为一条链（D1）：展示相似度 = 原始相似度 + 0.3（平台已看惯该
 * 数值），阈值取配置归一化值（非法/<=0 回退 0.7），成功与否全链路（事件类型、
 * 日志、TVSDK 曝光）只用同一个比较结果，修掉旧链路"日志 success、报警 FAIL"
 * 的存量矛盾。
 *
 * 事件节流（D2/D3）：全局单报警状态机（默认 7 秒冷却），事件即时成对——
 * 报警开始后立即结束，同一人驻留时每 7 秒最多一条，归属当帧置信度最高且
 * 已有特征缓存的目标。
 */

#pragma once

#if CAP_AI_FACE_COMPARE

#include <map>
#include <string>
#include <vector>

#include "algorithm.hpp"
#include "bestshot_hub.hpp"
#include "face_capture_types.hpp"
#include "face_chain_types.hpp"

class CFaceDetectWorker;

namespace FaceChain_NS
{
/* 人脸比对处理器（原生实现） */
class CFaceCompareProcessor : public IFaceBestShotListener
{
public:
    /* ===== 取优监听（经 CBestShotHub 回调，仅 Worker 回调线程） ===== */

    /*
     * 新 best：用这张最清晰的人脸提特征并缓存到该目标名下。
     * 帧生命周期仅限回调内（裁剪缩放后即毁），不留像素。
     */
    void onNewBest(int nTrackId,
                   const Bestshot_NS::BestShotFeedResult_S &stResult,
                   const ot_video_frame_info *pFrame,
                   const ot_rect &stRoi) override;

    /* 目标结束：清掉该目标缓存的特征 */
    void onTrackEnded(int nTrackId) override;

    /* 全清：清掉全部缓存特征（与取优桶同生命周期） */
    void onAllCleared() override;

    /* ===== 统一检测层链路接口（Worker 回调线程） ===== */

    /*
     * 注入推理 Worker（特征提取执行器），模型 init 时调用一次，
     * 须早于首次 onNewBest/process。
     */
    void attachWorker(CFaceDetectWorker *pWorker);

    /*
     * 设置比对使能（关闭时复位报警状态机）。本接口在配置线程调用，特征缓存
     * 不在此跨线程清理，随取优桶在 Worker 回调线程经 onTrackEnded/onAllCleared
     * 或 deinit 清理。
     */
    void setEnabled(bool bEnable);

    /* 设置比对配置（相似度阈值来源为目标库配置，成功/失败联动分别解析快照） */
    void setAlgoParamCfg(const Alarm::FaceCompare_S &stAlgoCfg);

    /*
     * 设置比对目标入口灵敏度门限（抓拍规则 nSensitivity 换算 1 - n/100）：
     * 检测置信度保底阈值放行低分框维持跟踪连续后，稳定目标全集含低置信目标，
     * 比对目标集须按抓拍同款灵敏度阈值过滤，保持"比对目标不吃低分框"的
     * Legacy 语义（比对无独立灵敏度配置，与抓拍共用一套，模型加载抓拍规则
     * 时同步设置）。
     */
    void setSensitivityThreshold(unsigned int nSensitivity);

    /* 当前比对目标入口灵敏度门限（喂入侧与过滤侧同源） */
    float sensitivityThreshold() const;

    /* 获取当前是否使能 */
    bool isEnabled() const;

    /* 初始化状态（原生实现无额外资源，保留调用时序恒真） */
    bool isInitialized() const;

    bool init();
    void deinit();

    /*
     * 单帧比对（Worker 回调线程）：
     * vTargets 为跟踪后的稳定目标全集（不受抓拍规则过滤），按灵敏度门限
     * 过滤、置信度降序（同分比面积）排序后逐目标执行全库余弦与事件判定；
     * 缓存特征缺失的目标跳过本轮（尚无 best 或上次提取失败），best 产生后
     * 自动恢复。事件由全局单报警状态机节流（冷却期内其余目标仅完成比对与
     * 日志）；stImageCache 双向流转：抓拍已生成的图片优先复用，比对自行
     * 生成的回写。
     */
    void process(FaceFrameContext_S &stCtx, const std::vector<FaceTarget_S> &vTargets, std::vector<std::string> &vecImageFile);

private:
    /*
     * 特征缓存条目：目标编号 -> 特征向量 + 产生该特征的 best 时刻。
     * 上界与取优桶一致（32 目标 × 约 2KB），无新增队列。
     */
    struct FeatureEntry_S
    {
        /* 最佳帧提取的特征向量（ArcFace 输出） */
        std::vector<float> vecFeature;
        /* 该特征的 best 产生时刻（毫秒），无业务消费仅供排查 */
        long long llBestTimestamp = 0;
    };

    /*
     * 从 best 帧提取特征：VGS 裁剪 stRoi（源帧坐标）并缩放到 112x112，
     * 经 Worker 特征句柄同步推理。stFaceFrame 临时帧用后即毁。
     */
    bool extractFeatureFromBest(const ot_video_frame_info *pFrame, const ot_rect &stRoi, std::vector<float> &vecFeature);

    /*
     * 比对成功/失败联动（照搬 Legacy 语义）：冷却检查 -> 事件属性逐字段
     * 填充 -> 图片（先抓拍缓存、后 CCaptureCtrl 通用抓图）-> TVSDK 曝光
     * （有订阅且比对成功才编码全景）-> 报警开始后立即结束（事件成对）-> 邮件。
     */
    void handleCompareLinkage(bool bSuccess,
                              ot_video_frame_info *pFrameInfo,
                              int nChnId,
                              long long llTimestamp,
                              int nFaceId,
                              float fSimilarityDisplay,
                              float fThreshold,
                              FaceFrameImageCache_S &stImageCache,
                              std::vector<std::string> &vecImageFile);

    /* 构造成功/失败联动配置快照 */
    FaceDetectInternal::FaceCompareLinkageOptions_S buildLinkageOptions(bool bSuccess) const;

    /* 推理 Worker（模型注入，特征提取执行器） */
    CFaceDetectWorker *m_pWorker = nullptr;
    /* 人脸比对配置 */
    Alarm::FaceCompare_S m_stAlgoCfg;
    /*
     * 特征缓存（目标编号 -> 最佳帧特征）：与取优桶同生命周期
     * （onTrackEnded/onAllCleared 同步清理），上界 32 目标。
     */
    std::map<int, FeatureEntry_S> m_mapFeatures;
    /*
     * 报警状态机：默认构造 7 秒冷却（全局单状态机，D2），
     * 比对事件即时成对（开始后立即结束，D3）。
     */
    CAlarmStateMachine m_alarmStateMachine;
    /* 比对成功联动配置快照 */
    FaceDetectInternal::FaceCompareLinkageOptions_S m_stSuccessLinkage;
    /* 比对失败联动配置快照 */
    FaceDetectInternal::FaceCompareLinkageOptions_S m_stFailLinkage;
    /* 比对目标入口置信度门限（= 1 - nSensitivity/100，默认灵敏度 50 → 0.5） */
    float m_fSensitivityThreshold = 0.5f;
};
} // namespace FaceChain_NS

#endif /* CAP_AI_FACE_COMPARE */
