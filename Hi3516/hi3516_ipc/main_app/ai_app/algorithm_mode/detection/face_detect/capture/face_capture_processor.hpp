/**
 * @FilePath     : face_capture_processor.hpp
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2026-09-21
 * @Description  : 人脸抓拍处理器——统一检测层独立实现（同名超集类）。
 *
 * 由 Legacy algorithm/face_detect 迁入并在此继续演进：取优（经 bestshot_hub）、
 * VENC 批量抠图出图、跟踪分桶等优化只落在本文件；Legacy 侧保持导入基线不动，
 * 两条链路经 detection.cmake 的同名源剔除保证全编译单元单一定义。
 *
 * 事件为窗口制语义（区别于 Legacy 瞬时事件）：规则通过首帧开启取优窗口，
 * nInterval 到期帧统一收割——多目标出图（在场取 best、离场补 best）、SDK
 * 推送、邮件、MQTT 事件对一次完成，窗口内无目标帧不终止窗口。
 *
 * 接口分三面：
 * - 统一检测层链路接口：process 域类型入参（FaceTarget_S 携带跟踪 id），
 *   取优经 CBestShotHub 驱动与消费（本类实现 IFaceBestShotListener 缓存 best 小图）；
 * - Legacy 兼容面（名单库入库临时实例路径）：四方法签名保持，恒未使能早退；
 * - 规则语义：区域/屏蔽/灵敏度/瞳距过滤为抓拍事件私有判定，比对目标集
 *   不经过本过滤（与 Legacy 语义一致）。
 */

#pragma once

#include <chrono>
#include <map>
#include <string>
#include <vector>

#include "algorithm.hpp"
#include "face_capture_types.hpp"
#include "face_detect_context.hpp"
#include "face_capture_sdk_event_publisher.hpp"
#include "bestshot_hub.hpp"
#include "face_chain_types.hpp"

namespace FaceDetectInternal
{
class CFaceCaptureProcessor : public IFaceBestShotListener
{
public:
    /* ===== 取优监听（经 CBestShotHub 回调，仅 Worker 回调线程） ===== */

    /* 新 best：立即 VENC 硬编码缓存该目标小图（约 5ms，帧生命周期不跨帧） */
    void onNewBest(int nTrackId,
                   const Bestshot_NS::BestShotFeedResult_S &stResult,
                   const ot_video_frame_info *pFrame,
                   const ot_rect &stRoi) override;

    /* 目标结束：清理该目标的 best 小图缓存 */
    void onTrackEnded(int nTrackId) override;

    /* 全清：清理全部 best 小图缓存 */
    void onAllCleared() override;

    /* ===== 统一检测层链路接口（Worker 回调线程） ===== */

    /**
     * @brief   : 单帧抓拍主流程
     * @param    {FaceFrameContext_S} &stCtx：单帧上下文（图片缓存回写）
     * @param    {vector<FaceTarget_S>} &vTargets：跟踪后的稳定目标全集
     * @param    {CBestShotHub} &stHub：取优枢纽（规则过滤后的最佳目标喂入）
     * @param    {vector<RectInfo_S>} &vstRectInfo：输出规则通过目标框（OSD 消费）
     * @param    {vector<string>} &vecImageFile：输出邮件附件图片路径列表
     * @return   : {void}
     * @note    : 每帧先按规则过滤（区域/屏蔽/灵敏度/瞳距），再给每张符合规则的
     *            人脸单独评分记录最佳图；画面里第一次出现符合规则的人脸时开始
     *            一个取优窗口，窗口到时（抓拍间隔）统一出图（_1.._N 落盘）、
     *            SDK 推送、邮件、报警一次完成。窗口内偶尔几帧没人脸不中断窗口。
     */
    void process(FaceChain_NS::FaceFrameContext_S &stCtx,
                 const std::vector<FaceChain_NS::FaceTarget_S> &vTargets,
                 CBestShotHub &stHub,
                 std::vector<Common::RectInfo_S> &vstRectInfo,
                 std::vector<std::string> &vecImageFile);

    /**
     * @brief   : 目标离开通知（模型每帧转发跟踪器的离开结果）
     * @param    {vector<int>} &vTrackIds：本帧离开的目标编号集合
     * @param    {CBestShotHub} &stHub：取优枢纽
     * @return   : {void}
     * @note    : 窗口内离开的人，最佳图留到窗口结束一起出图；不在窗口内时
     *            立即清掉保存的最佳图（出图张数在收割时按实际落盘统计）
     */
    void onTracksEnded(const std::vector<int> &vTrackIds, CBestShotHub &stHub);

    /* ===== Legacy 兼容面（名单库入库临时实例路径，抓拍恒未使能即早退） ===== */

    /**
     * @brief   : 设置人脸抓拍使能状态
     * @param    {bool} bEnable：true：使能 false：关闭
     * @return   : {void}
     */
    void setEnabled(bool bEnable);

    /**
     * @brief   : 设置人脸抓拍参数
     * @param    {Alarm::FaceCapture_S} &stAlgoCfg：人脸抓拍配置
     * @param    {int} nWidth：算法分辨率宽度
     * @param    {int} nHeight：算法分辨率高度
     * @return   : {void}
     */
    void setAlgoParamCfg(const Alarm::FaceCapture_S &stAlgoCfg, int nWidth, int nHeight);

    /* Legacy 单帧入口：未使能早退；意外使能时转域类型走新实现（本地无枢纽，取优旁路） */
    void process(SFaceProcessContext &stContext, std::vector<std::string> &vecImageFile);

    /**
     * @brief   : 获取当前是否使能
     * @return   : {bool} true：使能 false：关闭
     */
    bool isEnabled() const;

    /* 检测框 -> 出图裁剪框的统一对齐（扩框、逆变换、batch_crop 对齐），取优与出图共用 */
    static bool alignCropRect(Common::RectInfo_S &rect,
                              int nDetectWidth,
                              int nDetectHeight,
                              int nFrameWidth,
                              int nFrameHeight,
                              ot_rect &stCrop);

private:
    /**
     * @brief   : 收集满足人脸抓拍规则的目标（区域/屏蔽/灵敏度/瞳距，抓拍事件私有判定）
     * @param    {vector<FaceTarget_S>} &vTargets：跟踪后的稳定目标全集
     * @param    {vector<RectInfo_S>} &vstRectInfo：输出规则通过目标框
     * @param    {vector<FaceTarget_S>} *pvecTargets：输出规则通过目标（含跟踪 id）
     * @return   : {bool} true：存在满足规则的人脸 false：不存在
     */
    bool collectTargets(const std::vector<FaceChain_NS::FaceTarget_S> &vTargets,
                        std::vector<Common::RectInfo_S> &vstRectInfo,
                        std::vector<FaceChain_NS::FaceTarget_S> *pvecTargets);

    /*
     * 取优喂入：候选裁剪框与 ROI 亮度由 process 统一算好后传入（与出图同框），
     * 交枢纽评分选优，新 best 的小图编码经 onNewBest 回调，仅 Worker 回调线程调用。
     */
    void feedBestShot(const FaceChain_NS::FaceTarget_S &stTarget,
                      FaceChain_NS::FaceFrameContext_S &stCtx,
                      CBestShotHub &stHub,
                      const ot_rect &stCrop,
                      float fBrightness);

    /*
     * 批量统计本帧各人脸 ROI 的平均亮度：一次 VGS 硬件任务完成全部有效裁剪框，
     * 输出与 vstCrops 等长的亮度数组（[0,255]）；无效框/查询失败为 -1（按未知
     * 亮度评选，门限与亮度评分跳过），仅 Worker 回调线程调用。
     */
    void queryFaceBrightness(ot_video_frame_info *pFrame, const std::vector<ot_rect> &vstCrops, std::vector<float> &vfBrightness);

    /**
     * @brief   : 多目标出图，按目标顺序 _1.._N 落盘/入库。画面内的人脸优先用
     *            窗口里评出的最佳图（用掉后评选记录清零），没有最佳图的用当前
     *            画面裁剪编码补齐；画面内的处理完后，剩下的最佳图属于窗口
     *            中途离开的人脸，序号接在后面一起出图
     * @param    {vector<FaceTarget_S>} &vTargets：出图帧画面内符合规则的目标（含跟踪 id）
     * @param    {size_t} &unInFrameSaved：输出，画面内目标实际取得 JPEG 的张数
     * @param    {size_t} &unDepartedSaved：输出，窗口中途离开目标出图的张数
     * @return   : {int} 0：成功，非0：失败
     * @note    : 两个出参是收割日志的统计口径——按实际取得 JPEG 计数（框无效、
     *            编码失败的目标不计），两者之和与最终落盘张数一致；离场统计
     *            覆盖轨迹结束与置信度跌破灵敏度两种离开方式
     */
    int saveFaceImage(const std::vector<FaceChain_NS::FaceTarget_S> &vTargets,
                      ot_video_frame_info *pSrcFrameInfo,
                      int nChnId,
                      std::vector<std::string> &vecImageFile,
                      long long llTimestamp,
                      bool bSaveDatabase,
                      CBestShotHub &stHub,
                      size_t &unInFrameSaved,
                      size_t &unDepartedSaved);

    /**
     * @brief   : 窗口到时统一出图（旧版"事件触发即出图"的联动动作都移到了
     *            这里）：全景图用出图帧当帧编码；人脸图按 saveFaceImage 规则；
     *            SDK 推送画面内目标；邮件取第一张；报警"开始+结束"在这一刻
     *            成对发出。结束后清空全部评选记录并关闭窗口，画面里若仍有
     *            人脸，由 process 接着开下一个窗口
     * @param    {vector<FaceTarget_S>} &vecTargets：出图帧画面内符合规则的目标（可为空：只有中途离开的人出图）
     * @return   : {void}
     */
    void harvestWindow(const std::vector<FaceChain_NS::FaceTarget_S> &vecTargets,
                       FaceChain_NS::FaceFrameContext_S &stContext,
                       CBestShotHub &stHub,
                       std::vector<std::string> &vecImageFile);

    /**
     * @brief   : 构造人脸抓拍联动配置快照
     * @return   : {FaceCaptureLinkageOptions_S} 人脸抓拍联动配置
     */
    FaceCaptureLinkageOptions_S buildLinkageOptions() const;

    /**
     * @brief   : 构建人脸抓拍 SDK 全景图
     * @return   : {bool} true：构建成功 false：构建失败
     */
    bool buildSdkPanoramaImage(ot_video_frame_info *pFrameInfo, std::vector<unsigned char> &vecJpeg);

    /**
     * @brief   : 构建人脸抓拍 SDK 单目标小图
     * @return   : {bool} true：构建成功 false：构建失败
     */
    bool buildSdkTargetImage(const Common::RectInfo_S &stRectInfo,
                             ot_video_frame_info *pFrameInfo,
                             size_t nIndex,
                             std::vector<unsigned char> &vecJpeg);

    /**
     * @brief   : 编码单张人脸目标小图到指定文件
     * @return   : {int} 0：成功，非0：失败
     */
    int encodeFaceTargetImageToFile(const Common::RectInfo_S &stRectInfo,
                                    ot_video_frame_info *pSrcFrameInfo,
                                    const std::string &strFilename);

    /**
     * @brief   : 读取 JPEG 文件为内存数据
     * @return   : {bool} true：读取成功 false：读取失败
     */
    bool loadJpegFile(const std::string &strFilename, std::vector<unsigned char> &vecJpeg) const;

    /**
     * @brief   : 构造人脸抓拍 SDK 临时图片路径
     * @return   : {std::string} 临时图片路径
     */
    std::string buildTempFilePath(const std::string &strImageType, size_t nIndex) const;

    /* 抓拍目标图落盘 + 入库（取优缓存与直出编码共用） */
    int write_face_capture_files(const std::vector<std::vector<uint8_t>> &vecJpegs,
                                 int nChnId,
                                 std::vector<std::string> &vecImageFile,
                                 long long llTimestamp,
                                 bool bSaveDatabase);

    /**
     * @brief   : 保存信息至抓图图片数据库
     * @return   : {int} 0：成功，非0：失败
     */
    int saveToDatabase(const std::string &strFilename, const std::string &strCurrentDate, const std::string &strCurrentTime, int nChnId);

    /* 人脸抓拍配置 */
    Alarm::FaceCapture_S m_stAlgoCfg;
    /* 算法分辨率宽度 */
    int m_nWidth = PIXEL_WIDTH_640;
    /* 各目标 best 小图缓存（key=trackId，-1 为未接跟踪共用桶；选优桶在 bestshot_hub） */
    std::map<int, std::vector<uint8_t>> m_mapBestJpegs;
    /* 算法分辨率高度 */
    int m_nHeight = PIXEL_HEIGHT_384;
    /* 算法默认最小瞳距 */
    int m_nMinIpd = 20;
    /* 算法最小瞳距下限 */
    const int MIN_IPD = 10;
    /* 算法最小瞳距上限 */
    const int MAX_IPD = 75;
    /* 是否联动保存人脸全景图片 */
    bool m_bFacePanoramicImage = false;
    /* 是否联动保存人脸图片 */
    bool m_bFaceImage = false;
    /* 是否联动邮件 */
    bool m_bEmail = false;
    /* 是否联动上传 SD 卡 */
    bool m_bUploadSdCard = false;
    /* ===== 取优窗口状态（只在检测线程访问，无需加锁） ===== */
    /* 是否有进行中的取优窗口 */
    bool m_bWindowActive = false;
    /* 窗口开始时刻（用开机时钟计时，设备校时跳变不影响窗口时长） */
    std::chrono::steady_clock::time_point m_tpWindowStart;
    /* 窗口时长，毫秒（抓拍间隔换算，只允许 1~10 秒，到时统一出图） */
    int m_nWindowIntervalMs = 3000;
    /* 报警状态机（冷却时间设为 0：出图节奏由抓拍间隔控制，否则间隔小于冷却时间时第二条报警发不出去） */
    CAlarmStateMachine m_alarmStateMachine{ 0 };
    /* 人脸抓拍 SDK 图片事件推送器 */
    CFaceCaptureSdkEventPublisher m_sdkEventPublisher;
};
} // namespace FaceDetectInternal
