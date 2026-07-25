/**
 * @FilePath     : face_feature_processor.hpp
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2026-04-28 15:07:08
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-04-28 16:20:00
 * @Description  : 人脸特征提取与比对处理器
 */
#pragma once

#if CAP_AI_FACE_COMPARE

#include <mutex>
#include <string>
#include <vector>
#include "ImageFeature.hpp"
#include "algorithm.hpp"
#include "face_capture_types.hpp"
#include "face_detect_context.hpp"
#include "face_detect_worker.hpp"
#include "face_capture_processor.hpp"
#include "./face_detect_worker/face_detect_worker.hpp"

namespace FaceDetectInternal
{
class CFaceCaptureProcessor;

class CFaceFeatureProcessor
{
public:
    /**
     * @brief   : 析构人脸特征处理器
     * @return   {void}
     */
    ~CFaceFeatureProcessor();

    /**
     * @brief   : 设置人脸比对使能状态
     * @param    {bool} bEnable：true：使能 false：关闭
     * @return   {void}
     */
    void setEnabled(bool bEnable);

    /**
     * @brief   : 设置人脸比对参数
     * @param    {Alarm::FaceCompare_S} &stAlgoCfg：人脸比对配置
     * @return   {void}
     */
    void setAlgoParamCfg(const Alarm::FaceCompare_S &stAlgoCfg);

    /**
     * @brief   : 初始化人脸特征模型
     * @return   {bool} true：成功 false：失败
     */
    bool init();

    /**
     * @brief   : 释放人脸特征模型
     * @return   {void}
     */
    void deinit();
    struct FaceAlignInfo_S
    {
        Common::RectInfo_S stRect;

        std::vector<Inference_NS::Point_S> vPoints;

        float fConfidence = 0.f;

    };
    /**
     * @brief   : 处理单帧人脸比对结果
     * @param    {SFaceProcessContext} &stContext：单帧处理上下文
     * @param    {std::vector<Common::RectInfo_S>} &vstRectInfo：满足抓拍规则的人脸目标框列表
     * @param    {CFaceCaptureProcessor} &stCaptureProcessor：抓拍处理器，用于复用图片保存能力
     * @param    {std::vector<std::string>} &vecImageFile：邮件附件图片路径列表
     * @return   {void}
     */
    // void processCompare(SFaceProcessContext &stContext,
    //                     const std::vector<Common::RectInfo_S> &vstRectInfo,
    //                     CFaceCaptureProcessor &stCaptureProcessor,
    //                     std::vector<std::string> &vecImageFile);

    void processCompare(SFaceProcessContext &stContext,
                        const std::vector<FaceAlignInfo_S> &vFaceInfos,
                        CFaceCaptureProcessor &stCaptureProcessor,
                        std::vector<std::string> &vecImageFile);

    /**
     * @brief   : 添加人脸名单库
     * @param    {FaceDataDB_NS::FaceLibsInfo_S} &stFaceLibData：待添加的人脸库数据
     * @param    {Inference_NS::CYoloUltralyticsPoint} *pFaceDetHandle：人脸检测模型句柄
     * @param    {std::mutex} &npuMutex：NPU 推理互斥锁
     * @param    {int} nWidth：算法分辨率宽度
     * @param    {int} nHeight：算法分辨率高度
     * @return   {bool} true：成功 false：失败
     */
    bool addFaceLibGroup(FaceDataDB_NS::FaceLibsInfo_S &stFaceLibData, CFaceDetectWorker &detectWorker, int nWidth, int nHeight);

    /**
     * @brief   : 获取当前是否使能
     * @return   {bool} true：使能 false：关闭
     */
    bool isEnabled() const;

    /**
     * @brief   : 获取特征模型是否已初始化
     * @return   {bool} true：已初始化 false：未初始化
     */
    bool isInitialized() const;

    // bool collectCompareTargets(const std::vector<Inference_NS::PointData_S> &vPointDatas,

    //                            std::vector<Common::RectInfo_S> &vstRectInfo);

    bool collectCompareTargets(const std::vector<Inference_NS::BoxData_S> &vPointDatas, std::vector<FaceAlignInfo_S> &vFaceInfos);
    



private:
    /**
     * @brief   : 根据比对结果构造联动配置快照
     * @param    {bool} bSuccess：true：比对成功 false：比对失败
     * @return   {FaceCompareLinkageOptions_S} 比对联动配置
     */
    FaceCompareLinkageOptions_S buildLinkageOptions(bool bSuccess) const;

    /**
     * @brief   : 从单帧人脸图中提取特征向量
     * @param    {Common::RectInfo_S} &stRect：人脸目标框
     * @param    {ot_video_frame_info} *pFrameInfo：当前检测帧
     * @param    {std::mutex} *pNpuMutex：NPU 推理互斥锁
     * @param    {std::vector<float>} &vecFeature：输出特征向量
     * @return   {bool} true：成功 false：失败
     */
    // bool extractFeature(const Common::RectInfo_S &stRect,
    //                     ot_video_frame_info *pFrameInfo,
    //                     CFaceDetectWorker &detectWorker,
    //                     std::vector<float> &vecFeature);
    // bool extractFeature(const Common::RectInfo_S &stRect,
    //                     const std::vector<Inference_NS::Point_S> &vPoints,
    //                     ot_video_frame_info *pFrameInfo,
    //                     CFaceDetectWorker &detectWorker,
    //                     std::vector<float> &vecFeature);
    bool extractFeature(const Common::RectInfo_S &stRect,
        ot_video_frame_info *pFrameInfo,
        CFaceDetectWorker &detectWorker,
        std::vector<float> &vecFeature);

    // bool extractFeatureDirect(const Common::RectInfo_S &stRect,
    //                           ot_video_frame_info *pFrameInfo,
    //                           CFaceDetectWorker &detectWorker,
    //                           std::vector<float> &vecFeature);
    bool extractFeatureDirect(const Common::RectInfo_S &stRect,
        const std::vector<Inference_NS::Point_S> &vPoints,
        ot_video_frame_info *pFrameInfo,
        CFaceDetectWorker &detectWorker,
        std::vector<float> &vecFeature);
    /**
     * @brief   : 处理人脸比对成功/失败联动
     * @param    {bool} bSuccess：true：比对成功 false：比对失败
     * @param    {Common::RectInfo_S} &stRect：当前人脸目标框
     * @param    {ot_video_frame_info} *pFrameInfo：当前检测帧
     * @param    {int} nChnId：通道号
     * @param    {CFaceCaptureProcessor} &stCaptureProcessor：抓拍处理器
     * @param    {std::vector<std::string>} &vecImageFile：邮件附件图片路径列表
     * @return   {void}
     */
    void handleCompareLinkage(bool bSuccess,
                              const Common::RectInfo_S &stRect,
                              ot_video_frame_info *pFrameInfo,
                              int nChnId,
                              long long llTimestamp,
                              int nFaceId,
                              float fSimilarity,
                              float fThreshold,
                            //   const FaceDataDB_NS::FaceLibsInfo_S &stMatchedFaceInfo,
                              CFaceCaptureProcessor &stCaptureProcessor,
                              std::vector<std::string> &vecImageFile);

    /**
     * @brief   : 将 NV21 人脸图转换为特征模型输入浮点数组
     * @param    {ot_video_frame_info} &stFrame：160x160 NV21 人脸帧
     * @param    {std::vector<float>} &outData：输出浮点数组
     * @return   {bool} true：成功 false：失败
     */
    bool convertYuvToFloat160(ot_video_frame_info &stFrame, std::vector<float> &outData) const;

    /**
     * @brief   : 根据人脸框裁剪并缩放到 160x160
     * @param    {Common::RectInfo_S} &rect：人脸目标框
     * @param    {ot_video_frame_info} *pSrcFrameInfo：源视频帧
     * @param    {int} nWidth：算法分辨率宽度
     * @param    {int} nHeight：算法分辨率高度
     * @param    {ot_video_frame_info} &stDstFrameInfo：输出 160x160 人脸帧
     * @return   {bool} true：成功 false：失败
     */
    // bool prepareFace160Frame(const Common::RectInfo_S &rect,
    //                          ot_video_frame_info *pSrcFrameInfo,
    //                          int nWidth,
    //                          int nHeight,
    //                          ot_video_frame_info &stDstFrameInfo) const;
    // bool prepareFace160Frame(const Common::RectInfo_S &rect,
    //                          const std::vector<Inference_NS::Point_S> &vPoints,
    //                          ot_video_frame_info *pSrcFrameInfo,
    //                          int nWidth,
    //                          int nHeight,
    //                          ot_video_frame_info &stDstFrameInfo) const;

    bool prepareFace160Frame(
        const Common::RectInfo_S &rect, 
        // const std::vector<Inference_NS::Point_S> &vPoints,
        ot_video_frame_info *pSrcFrameInfo, 
        int nDetWidth,   // 检测分辨率宽
        int nDetHeight,  // 检测分辨率高
        int nOrigWidth,  // 原图宽
        int nOrigHeight, // 原图高
        ot_video_frame_info &stDstFrameInfo) const ;

    /**
     * @brief   : float32 转 float16
     * @param    {float} value：输入浮点数
     * @return   {uint16_t} float16 编码结果
     */
    uint16_t float32ToFloat16(float value) const;

    /* 人脸特征模型句柄 */
    // Inference_NS::CImageFeature *m_pFaceFeaHandle = nullptr;
    /* 人脸比对配置 */
    Alarm::FaceCompare_S m_stAlgoCfg;
    /* 比对成功联动配置 */
    FaceCompareLinkageOptions_S m_stSuccessLinkage;
    /* 比对失败联动配置 */
    FaceCompareLinkageOptions_S m_stFailLinkage;

    CAlarmStateMachine m_alarmStateMachine;
};
} // namespace FaceDetectInternal
#endif
