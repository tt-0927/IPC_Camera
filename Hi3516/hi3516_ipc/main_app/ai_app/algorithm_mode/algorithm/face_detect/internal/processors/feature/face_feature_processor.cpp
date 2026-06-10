/**
 * @FilePath     : face_feature_processor.cpp
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2026-04-28 15:07:08
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-05-25 16:10:49
 * @Description  : 人脸特征提取与比对处理器实现
 */

#include "face_feature_processor.hpp"

#include <algorithm>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <sys/stat.h>
#include <unistd.h>

#include "capture_ctrl.h"
#include "email_manage.h"
#include "face_capture_processor.hpp"
#include "face_manage.h"
#include "storage_manage.h"
#include "time_utils.h"
#include "video_frame_jpeg_encoder.hpp"
namespace
{
/* 人脸目标框放大倍率，用于裁剪时保留人脸周边上下文 */
//  constexpr float FACE_REGION_SCALE_RATIO = 1.5f;
constexpr float FACE_REGION_SCALE_RATIO = 1.0f;
/* 特征模型输入宽度 */
constexpr int FACE_FEATURE_INPUT_WIDTH = 112;
/* 特征模型输入高度 */
constexpr int FACE_FEATURE_INPUT_HEIGHT = 112;
/* 人脸比对成功阈值 */
constexpr float FACE_COMPARE_SUCCESS_THRESHOLD = 0.7f;
/* 名单库检测时使用的人脸检测宽度 */
constexpr int FACE_LIB_DETECT_WIDTH = 640;
/* 名单库检测时使用的人脸检测高度 */
constexpr int FACE_LIB_DETECT_HEIGHT = 640;

/**
 * @brief   : 释放人脸特征模型句柄
 * @param    {Inference_NS::CImageFeature} *pFaceFeaHandle：人脸特征模型句柄
 * @return   {void}
 * @note    : CImageFeature 是本模块直接创建的具体类型，显式调用析构和内存释放以避开非 virtual 析构的 delete 告警
 */
void destroyFaceFeatureHandle(Inference_NS::CImageFeature *pFaceFeaHandle)
{
    if (pFaceFeaHandle == nullptr)
    {
        return;
    }

    pFaceFeaHandle->~CImageFeature();
    ::operator delete(pFaceFeaHandle);
}

float normalizeFaceCompareThreshold(float fThreshold)
{
    if (fThreshold > 0.0f && fThreshold <= 1.0f)
    {
        return fThreshold;
    }
    return FACE_COMPARE_SUCCESS_THRESHOLD;
}

std::string toFixedString(float fValue, int nPrecision = 3)
{
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(nPrecision) << fValue;
    return oss.str();
}

std::string toPercentString(float fValue)
{
    const float fClamped = std::max(0.0f, std::min(fValue, 1.0f));
    return std::to_string(static_cast<int>(fClamped * 100.0f));
}
}

namespace FaceDetectInternal
{
CFaceFeatureProcessor::~CFaceFeatureProcessor()
{
    deinit();
}

void CFaceFeatureProcessor::setEnabled(bool bEnable)
{
    m_stAlgoCfg.bEnable = bEnable;
    if (!bEnable)
    {
        m_alarmStateMachine.reset();
        deinit();
    }
}

void CFaceFeatureProcessor::setAlgoParamCfg(const Alarm::FaceCompare_S &stAlgoCfg)
{
    m_stAlgoCfg = stAlgoCfg;
    m_stSuccessLinkage = FaceCompareLinkageOptions_S();
    m_stFailLinkage = FaceCompareLinkageOptions_S();

    for (auto &type : m_stAlgoCfg.stLinkageListSuccess.tradition)
    {
        if (type == int(Alarm::LinkageType::UPLOAD_PANORAMIC_IMAGE))
        {
            m_stSuccessLinkage.bPanoramaImage = true;
        }
        else if (type == int(Alarm::LinkageType::UPLOAD_TARGET_IMAGE))
        {
            m_stSuccessLinkage.bTargetImage = true;
        }
        else if (type == int(Alarm::LinkageType::SEND_EMAIL))
        {
            m_stSuccessLinkage.bEmail = true;
        }
        else if (type == int(Alarm::LinkageType::UPLOAD_SD_CARD))
        {
            m_stSuccessLinkage.bUploadSdCard = true;
        }
    }

    for (auto &type : m_stAlgoCfg.stLinkageListFail.tradition)
    {
        if (type == int(Alarm::LinkageType::UPLOAD_PANORAMIC_IMAGE))
        {
            m_stFailLinkage.bPanoramaImage = true;
        }
        else if (type == int(Alarm::LinkageType::UPLOAD_TARGET_IMAGE))
        {
            m_stFailLinkage.bTargetImage = true;
        }
        else if (type == int(Alarm::LinkageType::SEND_EMAIL))
        {
            m_stFailLinkage.bEmail = true;
        }
        else if (type == int(Alarm::LinkageType::UPLOAD_SD_CARD))
        {
            m_stFailLinkage.bUploadSdCard = true;
        }
    }
}

bool CFaceFeatureProcessor::init()
{

    return true;
}

void CFaceFeatureProcessor::deinit()
{
}

void CFaceFeatureProcessor::processCompare(SFaceProcessContext &stContext,
                                           const std::vector<Common::RectInfo_S> &vstRectInfo,
                                           CFaceCaptureProcessor &stCaptureProcessor,
                                           std::vector<std::string> &vecImageFile)
{

    if (!m_stAlgoCfg.bEnable || stContext.pFrameInfo == nullptr)
    {
        return;
    }
    bool bFaceCompare = false;
    const long long llBaseTimestamp = stContext.llTimestamp > 0 ? stContext.llTimestamp : TimeUtils_NS::get_currentTimestampMs();
    size_t nCompareIndex = 0;
    for (const auto &rect : vstRectInfo)
    {
        const long long llCompareTimestamp = llBaseTimestamp + static_cast<long long>(nCompareIndex++);
        /* 当前目标提取到的特征向量 */
        std::vector<float> vecFeature;
        if (!extractFeatureDirect(rect, stContext.pFrameInfo, *stContext.pDetectWorker, vecFeature))
        {
            dlog_error("特征提取失败 !");
            continue;
        }

        /* 当前目标比对结果 */
        int nFaceLibId = -1;
        float fSimilarity = 0.0f;
        FaceManage::AIFaceManage::instance()->comparisonFaceLib(vecFeature, nFaceLibId, fSimilarity);
        Alarm::FaceCompare_S stInfo;
        CEventConfigure::instance()->get_configure(stInfo);
        const float fThreshold = normalizeFaceCompareThreshold(stInfo.TargetLibInfos.Similarity);
        const bool bCompareSuccess = fSimilarity >= fThreshold;
        dlog_info("人脸比对结果: id=%d 相似度=%.3f 阈值=%.3f result=%s",
                  nFaceLibId,
                  fSimilarity,
                  fThreshold,
                  bCompareSuccess ? "success" : "fail");

        handleCompareLinkage(bCompareSuccess,
                             rect,
                             stContext.pFrameInfo,
                             stContext.nChnId,
                             llCompareTimestamp,
                             nFaceLibId,
                             fSimilarity,
                             fThreshold,
                             stCaptureProcessor,
                             vecImageFile);
        if (bCompareSuccess)
        {
            bFaceCompare = true;
        }
    }
    EventTriggerContext_S stExposureContext;

    stExposureContext.nChnId = stContext.nChnId;
    stExposureContext.llTimestamp = TimeUtils_NS::get_currentTimestampMs();
#ifdef ENABLE_TVSDK_SRC
    if (bFaceCompare && stContext.pFrameInfo != nullptr)
    {
        auto pPayload = std::make_shared<EventTvSdkPayload_S>();
        pPayload->enType = get_tvsdk_payload_type(stExposureContext.enEventType);
        if (AiAppCommon::encode_video_frame_to_jpeg_memory(stContext.pFrameInfo, pPayload->stPanoramaImage) == OK)
        {
            stExposureContext.pTvSdkPayload = pPayload;
        }
    }
#endif
    // 重置闪光灯的状态
    stExposureContext.enEventType = Event::Type_E::FACE_COMPARE_SUCCESS;
    m_alarmStateMachine.endAlarmImmediately(stExposureContext);

    stExposureContext.enEventType = Event::Type_E::FACE_COMPARE_FAIL;
    m_alarmStateMachine.endAlarmImmediately(stExposureContext);
}

bool CFaceFeatureProcessor::addFaceLibGroup(FaceDataDB_NS::FaceLibsInfo_S &stFaceLibData,
                                            CFaceDetectWorker &detectWorker,
                                            int nWidth,
                                            int nHeight)
{
    dlog_info("=== [FaceLib] 开始添加人脸库 ===");

    if (!init())
    {
        dlog_error("特征模型初始化失败");

        return false;
    }

    /*
     * 原始图片宽高
     */
    const int w = stFaceLibData.PicWidth;

    const int h = stFaceLibData.PicHeight;

    const size_t nv21Size = static_cast<size_t>(w) * h * 3 / 2;

    /*
     * 读取NV21
     */
    std::vector<uint8_t> nv21(nv21Size);

    std::ifstream file(stFaceLibData.BinPath, std::ios::binary);

    if (!file)
    {
        dlog_error("打开NV21失败: %s", stFaceLibData.BinPath.c_str());

        return false;
    }

    file.read(reinterpret_cast<char *>(nv21.data()), static_cast<std::streamsize>(nv21Size));

    file.close();

    /*
     * 创建原始视频帧
     */
    ot_video_frame_info stSrc;

    if (TD_SUCCESS != mppVgs_create_video_frame_info(w, h, OT_PIXEL_FORMAT_YVU_SEMIPLANAR_420, &stSrc))
    {
        dlog_error("创建源帧失败");

        return false;
    }

    memcpy(stSrc.video_frame.virt_addr[0], nv21.data(), nv21Size);

    /*
     * 检测输入帧
     */
    constexpr int DETECT_WIDTH = 640;
    constexpr int DETECT_HEIGHT = 640;

    ot_video_frame_info stDet;

    ot_video_frame_info *pDet = &stSrc;

    /*
     * 尺寸不一致则缩放
     */
    if (w != DETECT_WIDTH || h != DETECT_HEIGHT)
    {
        if (TD_SUCCESS != mppVgs_create_video_frame_info(DETECT_WIDTH, DETECT_HEIGHT, OT_PIXEL_FORMAT_YVU_SEMIPLANAR_420, &stDet))
        {
            mppVgs_destroy_video_frame_info(&stSrc);

            return false;
        }

        if (TD_SUCCESS != mppVgs_scale(&stSrc, &stDet))
        {
            mppVgs_destroy_video_frame_info(&stDet);

            mppVgs_destroy_video_frame_info(&stSrc);

            return false;
        }

        pDet = &stDet;

        /*
         * 保存缩放后的640x640 NV21
         */
        // {
        //     std::ofstream out("/tmp/face_det_640x640.bin", std::ios::binary);

        //     if (out.is_open())
        //     {
        //         size_t size = DETECT_WIDTH * DETECT_HEIGHT * 3 / 2;

        //         out.write(reinterpret_cast<char *>(stDet.video_frame.virt_addr[0]), size);

        //         out.close();

        //         dlog_info("保存缩放图成功:/tmp/face_det_640x640.bin");
        //     }
        // }
    }

    /*
     * 提交异步检测任务
     */
    std::string taskId = detectWorker.submitFaceImage(pDet, DETECT_WIDTH, DETECT_HEIGHT);

    dlog_info("提交人脸库检测任务: %s", taskId.c_str());

    /*
     * 等待检测结果
     *
     * 这里只等待任务状态
     * 不会阻塞NPU线程
     */
    CFaceDetectWorker::TaskResult taskResult;

    std::vector<Inference_NS::PointData_S> vDet;

    constexpr int WAIT_TIMEOUT_MS = 5000;

    int waitMs = 0;

    while (waitMs < WAIT_TIMEOUT_MS)
    {
        if (detectWorker.queryTaskResult(taskId, taskResult))
        {
            if (taskResult.state == CFaceDetectWorker ::TaskState ::SUCCESS)
            {
                vDet = taskResult.result;

                break;
            }
            else if (taskResult.state == CFaceDetectWorker ::TaskState ::FAILED)
            {
                dlog_error("检测任务失败");

                detectWorker.removeTask(taskId);

                if (pDet == &stDet)
                {
                    mppVgs_destroy_video_frame_info(&stDet);
                }

                mppVgs_destroy_video_frame_info(&stSrc);

                return false;
            }
            else if (taskResult.state == CFaceDetectWorker ::TaskState ::TIMEOUT)
            {
                dlog_error("检测任务超时");

                detectWorker.removeTask(taskId);

                if (pDet == &stDet)
                {
                    mppVgs_destroy_video_frame_info(&stDet);
                }

                mppVgs_destroy_video_frame_info(&stSrc);

                return false;
            }
        }

        usleep(10 * 1000);

        waitMs += 10;
    }

    detectWorker.removeTask(taskId);

    /*
     * 未检测到人脸
     */
    if (vDet.empty())
    {
        dlog_error("未检测到人脸");

        if (pDet == &stDet)
        {
            mppVgs_destroy_video_frame_info(&stDet);
        }

        mppVgs_destroy_video_frame_info(&stSrc);

        return false;
    }

    /*
     * 多人脸
     */
    if (vDet.size() > 1)
    {
        dlog_error("检测到多个人脸");

        if (pDet == &stDet)
        {
            mppVgs_destroy_video_frame_info(&stDet);
        }

        mppVgs_destroy_video_frame_info(&stSrc);

        return false;
    }

    /*
     * 取置信度最高
     */
    auto best = *std::max_element(vDet.begin(),
                                  vDet.end(),
                                  [](auto &a, auto &b)
                                  {
                                      return a.fConfidence < b.fConfidence;
                                  });

    dlog_info("检测成功 conf=%.3f", best.fConfidence);

    /*
     * 映射回原图坐标
     */
    Common::RectInfo_S face;

    if (pDet == &stDet)
    {
        const float rw = static_cast<float>(w) / DETECT_WIDTH;

        const float rh = static_cast<float>(h) / DETECT_HEIGHT;

        face.nX1 = static_cast<int>(best.stBoxs.nX1 * rw);

        face.nY1 = static_cast<int>(best.stBoxs.nY1 * rh);

        face.nX2 = static_cast<int>(best.stBoxs.nX2 * rw);

        face.nY2 = static_cast<int>(best.stBoxs.nY2 * rh);
    }
    else
    {
        face.nX1 = best.stBoxs.nX1;

        face.nY1 = best.stBoxs.nY1;

        face.nX2 = best.stBoxs.nX2;

        face.nY2 = best.stBoxs.nY2;
    }

    /*
     * 提取特征
     */
    std::vector<float> vecFeature;

    bool bExtractOk = extractFeature(face, &stSrc, detectWorker, vecFeature);

    /*
     * 裁剪后缩放到112x112
     */
    // ot_video_frame_info stCropFrame;
    // ot_video_frame_info stResizeFrame;

    // constexpr int FEATURE_WIDTH  = 112;
    // constexpr int FEATURE_HEIGHT = 112;

    // bool bExtractOk = false;

    // do
    // {
    //     Common::RectInfo_S cropRect = face;

    //     /* VGS对齐 */
    //     cropRect.nX1 =
    //         ALIGN_BACK(cropRect.nX1,16);

    //     cropRect.nY1 =
    //         ALIGN_BACK(cropRect.nY1,4);

    //     cropRect.nX2 =
    //         ALIGN_BACK(cropRect.nX2,16);

    //     cropRect.nY2 =
    //         ALIGN_BACK(cropRect.nY2,4);

    //     uint32_t cropW =
    //         cropRect.nX2 -
    //         cropRect.nX1;

    //     uint32_t cropH =
    //         cropRect.nY2 -
    //         cropRect.nY1;

    //     /*
    //      * 创建裁剪帧
    //      */
    //     if(TD_SUCCESS !=
    //        mppVgs_create_video_frame_info(
    //             cropW,
    //             cropH,
    //             OT_PIXEL_FORMAT_YVU_SEMIPLANAR_420,
    //             &stCropFrame))
    //     {
    //         break;
    //     }

    //     ot_rect rect;

    //     rect.x =
    //         cropRect.nX1;

    //     rect.y =
    //         cropRect.nY1;

    //     rect.width =
    //         cropW;

    //     rect.height =
    //         cropH;

    //     /*
    //      * crop
    //      */
    //     if(TD_SUCCESS !=
    //        mppVgs_crop(
    //             &stSrc,
    //             &stCropFrame,
    //             &rect))
    //     {
    //         mppVgs_destroy_video_frame_info(
    //             &stCropFrame);

    //         break;
    //     }

    //     /*
    //      * 创建112x112目标帧
    //      */
    //     if(TD_SUCCESS !=
    //        mppVgs_create_video_frame_info(
    //             FEATURE_WIDTH,
    //             FEATURE_HEIGHT,
    //             OT_PIXEL_FORMAT_YVU_SEMIPLANAR_420,
    //             &stResizeFrame))
    //     {
    //         mppVgs_destroy_video_frame_info(
    //             &stCropFrame);

    //         break;
    //     }

    //     /*
    //      * resize
    //      */
    //     if(TD_SUCCESS !=
    //        mppVgs_scale(
    //             &stCropFrame,
    //             &stResizeFrame))
    //     {
    //         mppVgs_destroy_video_frame_info(
    //             &stCropFrame);

    //         mppVgs_destroy_video_frame_info(
    //             &stResizeFrame);

    //         break;
    //     }

    //     /*
    //      * 保存112×112调试bin
    //      */
    //     {
    //         std::ofstream out(
    //             "/tmp/face_112x112.bin",
    //             std::ios::binary);

    //         if(out.is_open())
    //         {
    //             size_t size =
    //                 FEATURE_WIDTH *
    //                 FEATURE_HEIGHT *
    //                 3 / 2;

    //             out.write(
    //                 reinterpret_cast<char*>(
    //                     stResizeFrame
    //                     .video_frame
    //                     .virt_addr[0]),
    //                 size);

    //             out.close();

    //             dlog_info(
    //                 "保存112x112成功");
    //         }
    //     }

    //     /*
    //      * 提取特征
    //      */
    //     Common::RectInfo_S fullRect;

    //     fullRect.nX1 = 0;
    //     fullRect.nY1 = 0;
    //     fullRect.nX2 = FEATURE_WIDTH;
    //     fullRect.nY2 = FEATURE_HEIGHT;

    //     bExtractOk =
    //         extractFeature(
    //             fullRect,
    //             &stResizeFrame,
    //             detectWorker,
    //             vecFeature);

    //     mppVgs_destroy_video_frame_info(
    //         &stCropFrame);

    //     mppVgs_destroy_video_frame_info(
    //         &stResizeFrame);

    // }
    // while(0);

    /*
     * 释放资源
     */
    if (pDet == &stDet)
    {
        mppVgs_destroy_video_frame_info(&stDet);
    }

    mppVgs_destroy_video_frame_info(&stSrc);

    /*
     * 打印特征向量
     */
    if (bExtractOk)
    {
        dlog_info("feature size=%zu", vecFeature.size());

        std::stringstream ss;

        ss << "feature=[";

        for (size_t i = 0; i < vecFeature.size(); i++)
        {
            ss << std::fixed << std::setprecision(6) << vecFeature[i];

            if (i != vecFeature.size() - 1)
            {
                ss << ",";
            }

            /*
             * 每16个换行一次，避免日志太长
             */
            if ((i + 1) % 16 == 0)
            {
                ss << "\n";
            }
        }

        ss << "]";

        dlog_info("%s", ss.str().c_str());
    }

    if (!bExtractOk)
    {
        dlog_error("特征提取失败");

        return false;
    }

    /*
     * 保存特征
     */
    stFaceLibData.vfData = vecFeature;

    stFaceLibData.nModelState = 1;

    FaceManage::AIFaceManage ::instance()->addFaceLibInfo(stFaceLibData);

    dlog_info("人脸库添加成功");

    return true;
}

bool CFaceFeatureProcessor::isEnabled() const
{
    return m_stAlgoCfg.bEnable;
}

bool CFaceFeatureProcessor::isInitialized() const
{
    // return m_pFaceFeaHandle != nullptr;
    return true;
}

FaceCompareLinkageOptions_S CFaceFeatureProcessor::buildLinkageOptions(bool bSuccess) const
{
    return bSuccess ? m_stSuccessLinkage : m_stFailLinkage;
}

bool CFaceFeatureProcessor::extractFeature(const Common::RectInfo_S &stRect,
                                           ot_video_frame_info *pFrameInfo,
                                           CFaceDetectWorker &detectWorker,
                                           std::vector<float> &vecFeature)
{
    vecFeature.clear();

    if (!pFrameInfo)
    {
        return false;
    }

    ot_video_frame_info stFaceFrame;

    memset(&stFaceFrame, 0, sizeof(stFaceFrame));

    std::cout << "Rect Info -> x1: " << stRect.nX1 /*test*/
              << ", y1: " << stRect.nY1 << ", x2: " << stRect.nX2 << ", y2: " << stRect.nY2 << std::endl;

    if (!prepareFace160Frame(stRect, pFrameInfo, pFrameInfo->video_frame.width, pFrameInfo->video_frame.height, stFaceFrame))
    {
        return false;
    }

    // {
    //     std::ofstream out("/tmp/face_112x112.bin", std::ios::binary);

    //     if (out.is_open())
    //     {
    //         size_t size = 112 * 112 * 3 / 2;

    //         out.write(reinterpret_cast<char *>(stFaceFrame.video_frame.virt_addr[0]), size);

    //         out.close();

    //         dlog_info("保存112x112成功");
    //     }
    // }
    //  std::vector<float> inputBuffer;

    //  if (!convertYuvToFloat160(
    //          stFaceFrame,
    //          inputBuffer))
    //  {
    //      mppVgs_destroy_video_frame_info(
    //          &stFaceFrame);

    //      return false;
    //  }


    /*
     * 提交特征任务
     */
    std::string taskId = detectWorker.submitFeatureTask(stFaceFrame, FACE_FEATURE_INPUT_WIDTH, FACE_FEATURE_INPUT_HEIGHT);

    if (taskId.empty())
    {
        mppVgs_destroy_video_frame_info(&stFaceFrame);

        return false;
    }

    CFaceDetectWorker::TaskResult result;

    constexpr int WAIT_TIMEOUT_MS = 3000;

    int waitMs = 0;

    bool bSuccess = false;

    while (waitMs < WAIT_TIMEOUT_MS)
    {
        if (detectWorker.queryTaskResult(taskId, result))
        {
            if (result.state == CFaceDetectWorker ::TaskState ::SUCCESS)
            {
                vecFeature = result.feature;

                bSuccess = true;

                break;
            }
            else if (result.state == CFaceDetectWorker ::TaskState ::FAILED)
            {
                break;
            }
        }

        usleep(10 * 1000);

        waitMs += 10;
    }

    detectWorker.removeTask(taskId);

    mppVgs_destroy_video_frame_info(&stFaceFrame);

    return bSuccess && !vecFeature.empty();
}

bool CFaceFeatureProcessor::extractFeatureDirect(const Common::RectInfo_S &stRect,
                                                 ot_video_frame_info *pFrameInfo,
                                                 CFaceDetectWorker &detectWorker,
                                                 std::vector<float> &vecFeature)
{
    vecFeature.clear();

    if (!pFrameInfo)
    {
        return false;
    }

    /*
     * 1. 裁剪并缩放到112x112
     */
    ot_video_frame_info stFaceFrame;

    memset(&stFaceFrame, 0, sizeof(stFaceFrame));

    if (!prepareFace160Frame(stRect, pFrameInfo, pFrameInfo->video_frame.width, pFrameInfo->video_frame.height, stFaceFrame))
    {
        dlog_error("prepareFace160Frame失败");

        return false;
    }

    /*
     * 保存缩放后的160x160 NV21
     */
    // {
    //     int width = 112;

    //     int height = 112;

    //     size_t nv21Size = width * height * 3 / 2;

    //     std::ofstream out("/tmp/feature_input.bin", std::ios::binary);

    //     if (out.is_open())
    //     {
    //         out.write(reinterpret_cast<char *>(stFaceFrame.video_frame.virt_addr[0]), nv21Size);

    //         out.close();

    //         dlog_info("保存缩放图成功: 1"
    //                   "/tmp/feature_input.bin "
    //                   "(%dx%d size=%zu)",
    //                   width,
    //                   height,
    //                   nv21Size);
    //     }
    //     else
    //     {
    //         dlog_error("保存feature_input.bin失败");
    //     }
    // }

    Inference_NS::InputData_S stInputData;


    std::vector<Inference_NS::ClsData_S> vClsDatas;

    auto *pFeatureHandle = detectWorker.getFeatureHandle();

    if (!pFeatureHandle)
    {
        dlog_error("FeatureHandle为空");

        mppVgs_destroy_video_frame_info(&stFaceFrame);

        return false;
    }

    stInputData.pData = reinterpret_cast<float *>(stFaceFrame.video_frame.virt_addr[0]);

    stInputData.nDataSize = static_cast<int>(FACE_FEATURE_INPUT_WIDTH * FACE_FEATURE_INPUT_HEIGHT * 1.5) * sizeof(float);

    /*
     * 同步执行ArcFace
     */
    pFeatureHandle->inference(stInputData, vClsDatas);

    /*
     * 释放frame
     */
    mppVgs_destroy_video_frame_info(&stFaceFrame);

    /*
     * 检查结果
     */
    if (vClsDatas.empty())
    {
        dlog_error("ArcFace推理失败");

        return false;
    }

    vecFeature = vClsDatas[0].vFeature;

    dlog_info("feature size=%zu", vecFeature.size());

    for (size_t i = 0; i < vecFeature.size(); i += 16)
    {
        std::stringstream ss;

        ss << "feature[" << i << "]:";

        for (size_t j = i; j < std::min(i + 16, vecFeature.size()); ++j)
        {
            ss << std::fixed << std::setprecision(6) << vecFeature[j] << " ";
        }

        dlog_info("%s", ss.str().c_str());
    }

    if (vecFeature.empty())
    {
        dlog_error("特征为空");

        return false;
    }

    dlog_info("同步特征提取成功  feature=%zu", vecFeature.size());

    return true;
}

static bool saveCompareImage(const Common::RectInfo_S &stRect,
                             ot_video_frame_info *pFrameInfo,
                             int nChnId,
                             long long llTimestamp,
                             CFaceCaptureProcessor &stCaptureProcessor,
                             std::vector<std::string> &vecImageFile,
                             std::string &strImagePath)
{
    if (!pFrameInfo)
    {
        return false;
    }

    const size_t nBeforeSaveCount = vecImageFile.size();
    std::vector<Common::RectInfo_S> vstSingleRectInfo{ stRect };
    int nRet = stCaptureProcessor.saveFaceImage(vstSingleRectInfo, pFrameInfo, nChnId, vecImageFile, llTimestamp, false);

    if (nRet != OK || vecImageFile.size() <= nBeforeSaveCount)
    {
        dlog_error("人脸比对抓拍图片保存失败");
        return false;
    }

    strImagePath = vecImageFile[nBeforeSaveCount];
    dlog_info("人脸比对抓拍图片保存成功: path[%s], timestamp[%lld]", strImagePath.c_str(), llTimestamp);
    return true;
}

static bool shouldUploadCompareImage(const FaceCompareLinkageOptions_S &stOptions)
{
    /*
     * 平台事件图片上传线程由 UPLOAD_SD_CARD(3) 触发。
     * 人脸比对没有通用抓图兜底，因此只要需要启动图片上传线程，就先准备当前比对目标图。
     */
    return stOptions.bUploadSdCard;
}

static void fillFaceCompareAttrs(EventTriggerContext_S &stContext,
                                 bool bSuccess,
                                 int nFaceId,
                                 float fSimilarity,
                                 float fThreshold,
                                 const std::string &strCaptureImagePath)
{
    stContext.mapAttrs["CompareResult"] = bSuccess ? "1" : "0";
    stContext.mapAttrs["CompareResultText"] = bSuccess ? "success" : "fail";
    stContext.mapAttrs["Similarity"] = toPercentString(fSimilarity);
    stContext.mapAttrs["SimilarityFloat"] = toFixedString(fSimilarity);
    stContext.mapAttrs["Threshold"] = toPercentString(fThreshold);
    stContext.mapAttrs["ThresholdFloat"] = toFixedString(fThreshold);
    stContext.mapAttrs["FaceId"] = std::to_string(nFaceId);
    if (!strCaptureImagePath.empty())
    {
        /* 内部字段只给事件图片上传线程定位文件，MQTT报警正文会过滤掉 */
        stContext.mapAttrs["CaptureImagePath"] = strCaptureImagePath;
    }
}

void CFaceFeatureProcessor::handleCompareLinkage(bool bSuccess,
                                                 const Common::RectInfo_S &stRect,
                                                 ot_video_frame_info *pFrameInfo,
                                                 int nChnId,
                                                 long long llTimestamp,
                                                 int nFaceId,
                                                 float fSimilarity,
                                                 float fThreshold,
                                                 CFaceCaptureProcessor &stCaptureProcessor,
                                                 std::vector<std::string> &vecImageFile)
{
    const FaceCompareLinkageOptions_S stOptions = buildLinkageOptions(bSuccess);
    const long long llEventTimestamp = llTimestamp > 0 ? llTimestamp : TimeUtils_NS::get_currentTimestampMs();
    std::string strUploadImagePath;

    EventTriggerContext_S stExposureContext;

    if (bSuccess)
    {
        stExposureContext.enEventType = Event::Type_E::FACE_COMPARE_SUCCESS;
    }
    else
    {
        stExposureContext.enEventType = Event::Type_E::FACE_COMPARE_FAIL;
    }
    stExposureContext.nChnId = nChnId;
    stExposureContext.llTimestamp = llEventTimestamp;

    if (shouldUploadCompareImage(stOptions))
    {
        saveCompareImage(stRect,
                         pFrameInfo,
                         nChnId,
                         llEventTimestamp,
                         stCaptureProcessor,
                         vecImageFile,
                         strUploadImagePath);
    }
    fillFaceCompareAttrs(stExposureContext, bSuccess, nFaceId, fSimilarity, fThreshold, strUploadImagePath);
#ifdef ENABLE_TVSDK_SRC
    if (bSuccess && pFrameInfo != nullptr)
    {
        auto pPayload = std::make_shared<EventTvSdkPayload_S>();
        pPayload->enType = get_tvsdk_payload_type(stExposureContext.enEventType);
        if (AiAppCommon::encode_video_frame_to_jpeg_memory(pFrameInfo, pPayload->stPanoramaImage) == OK)
        {
            stExposureContext.pTvSdkPayload = pPayload;
        }
    }
#endif
    m_alarmStateMachine.handleAlarmState(true, stExposureContext);

    if (!stOptions.bUploadSdCard || SD_CARD_STATUS_E::NORMAL != CStorageManage::instance()->get_SdCardStatus())
    {
        return;
    }

    if (!access("testPrint", F_OK))
    {
        if (bSuccess)
        {
            dlog_trace("人脸比对大于0.7联动保存人脸图片开始");
        }
        else
        {
            dlog_trace("人脸比对小于0.7联动保存人脸图片开始");
        }
    }

    Event::Info_S stEventInfo;
    stEventInfo.enType = Event::Type_E::FACE_COMPARE;
    stEventInfo.strDate = TimeUtils_NS::get_currentDate();
    stEventInfo.strTime = TimeUtils_NS::get_currentTimeMs();
    stEventInfo.nChnId = nChnId < 0 ? 0 : nChnId;
    stEventInfo.strStartTime = TimeUtils_NS::get_currentDateWithDash() + " " + TimeUtils_NS::get_currentTimeWithColon();
    stEventInfo.strEndTime = stEventInfo.strStartTime;

    const int nRet = CCaptureCtrl::instance()->set_event_capture(false, stEventInfo);
    if (nRet == OK)
    {
        if (stOptions.bPanoramaImage)
        {
            auto strFaceImage = CCaptureCtrl::instance()->get_face_capture_file();
            if (!strFaceImage.empty())
            {
                vecImageFile.emplace_back(strFaceImage);
            }
        }

        /* 平台上传用目标图已在事件触发前保存；这里不再重复保存 */
    }

    if (stOptions.bEmail)
    {
        Network::EmailEventInfo_S stEmailInfo;
        stEmailInfo.strSubject = "人脸比对";

        std::ostringstream oss;
        oss << "事件类型: " << stEmailInfo.strSubject << "\n"
            << "日期: " << TimeUtils_NS::get_currentDateWithDash() << "\n"
            << "时间: " << TimeUtils_NS::get_currentTimeWithColon();
        stEmailInfo.strMessage = oss.str();
        stEmailInfo.vecImageFile = vecImageFile;
        CEmailManage::instance()->HandleEmail(stEmailInfo);
    }

    vecImageFile.clear();
    CCaptureCtrl::instance()->set_event_capture(true, stEventInfo);
    if (!access("testPrint", F_OK))
    {
        if (bSuccess)
        {
            dlog_trace("人脸比对大于0.7联动保存人脸图片结束");
        }
        else
        {
            dlog_trace("人脸比对小于0.7联动保存人脸图片结束");
        }
    }
}

bool CFaceFeatureProcessor::convertYuvToFloat160(ot_video_frame_info &stFrame, std::vector<float> &outData) const
{
    unsigned char *pYuv = reinterpret_cast<unsigned char *>(stFrame.video_frame.virt_addr[0]);
    const int ySize = FACE_FEATURE_INPUT_WIDTH * FACE_FEATURE_INPUT_HEIGHT;
    outData.resize(FACE_FEATURE_INPUT_WIDTH * FACE_FEATURE_INPUT_HEIGHT * 3);

    for (int i = 0; i < ySize; i++)
    {
        const float val = pYuv[i] / 255.0f;
        outData[i] = val;
        outData[i + ySize] = val;
        outData[i + 2 * ySize] = val;
    }
    return true;
}

bool CFaceFeatureProcessor::prepareFace160Frame(const Common::RectInfo_S &rect,
                                                ot_video_frame_info *pSrcFrameInfo,
                                                int nWidth,
                                                int nHeight,
                                                ot_video_frame_info &stDstFrameInfo) const
{
    Common::RectInfo_S faceRect = rect;
    convert_region_ratio(faceRect, FACE_REGION_SCALE_RATIO, nWidth, nHeight);
    faceRect.nX1 = ALIGN_BACK(faceRect.nX1, 16);
    faceRect.nY1 = ALIGN_BACK(faceRect.nY1, 4);
    faceRect.nX2 = ALIGN_BACK(faceRect.nX2, 16);
    faceRect.nY2 = ALIGN_BACK(faceRect.nY2, 4);

    const unsigned int cropW = faceRect.nX2 - faceRect.nX1;
    const unsigned int cropH = faceRect.nY2 - faceRect.nY1;
    ot_video_frame_info stCropFrame;
    if (TD_SUCCESS != mppVgs_create_video_frame_info(cropW, cropH, OT_PIXEL_FORMAT_YVU_SEMIPLANAR_420, &stCropFrame))
    {
        return false;
    }

    ot_rect stRect;
    stRect.x = faceRect.nX1;
    stRect.y = faceRect.nY1;
    stRect.width = cropW;
    stRect.height = cropH;
    if (TD_SUCCESS != mppVgs_crop(pSrcFrameInfo, &stCropFrame, &stRect))
    {
        mppVgs_destroy_video_frame_info(&stCropFrame);
        return false;
    }

    if (TD_SUCCESS != mppVgs_create_video_frame_info(FACE_FEATURE_INPUT_WIDTH,
                                                     FACE_FEATURE_INPUT_HEIGHT,
                                                     OT_PIXEL_FORMAT_YVU_SEMIPLANAR_420,
                                                     &stDstFrameInfo))
    {
        mppVgs_destroy_video_frame_info(&stCropFrame);
        return false;
    }

    if (TD_SUCCESS != mppVgs_scale(&stCropFrame, &stDstFrameInfo))
    {
        mppVgs_destroy_video_frame_info(&stCropFrame);
        mppVgs_destroy_video_frame_info(&stDstFrameInfo);
        return false;
    }

    mppVgs_destroy_video_frame_info(&stCropFrame);
    return true;
}

uint16_t CFaceFeatureProcessor::float32ToFloat16(float value) const
{
    uint32_t bits = *((uint32_t *) &value);
    const uint32_t sign = (bits >> 31) & 0x1;
    const uint32_t exp = (bits >> 23) & 0xFF;
    const uint32_t mant = bits & 0x7FFFFF;
    uint16_t h;

    if (exp == 0)
    {
        h = static_cast<uint16_t>(sign << 15);
    }
    else if (exp == 255)
    {
        h = static_cast<uint16_t>((sign << 15) | 0x7C00);
    }
    else
    {
        const int new_exp = static_cast<int>(exp) - 127 + 15;
        if (new_exp >= 31)
        {
            h = static_cast<uint16_t>((sign << 15) | 0x7C00);
        }
        else if (new_exp <= 0)
        {
            h = static_cast<uint16_t>(sign << 15);
        }
        else
        {
            h = static_cast<uint16_t>((sign << 15) | (new_exp << 10) | (mant >> 13));
        }
    }

    return h;
}

bool CFaceFeatureProcessor::collectCompareTargets(const std::vector<Inference_NS::PointData_S> &vPointDatas,

                                                  std::vector<Common::RectInfo_S> &vstRectInfo)
{
    vstRectInfo.clear();

    if (vPointDatas.empty())
    {
        return false;
    }

    /*
     * Compare最小置信度
     *
     * 比抓拍更宽松
     */
    constexpr float MIN_COMPARE_CONFIDENCE = 0.45f;

    /*
     * Compare最小瞳距
     *
     * ArcFace至少保证
     * 五官可分辨
     */
    constexpr int MIN_COMPARE_IPD = 20;

    for (const auto &pointData : vPointDatas)
    {
        /*
         * confidence过滤
         */
        if (pointData.fConfidence < MIN_COMPARE_CONFIDENCE)
        {
            continue;
        }

        /*
         * 关键点数量检查
         */
        if (pointData.vPoints.size() < 2)
        {
            continue;
        }

        /*
         * 双眼瞳距
         */
        int nIpd = std::abs(pointData.vPoints[1].nX - pointData.vPoints[0].nX);

        /*
         * 太小的人脸
         * ArcFace效果很差
         */
        if (nIpd < MIN_COMPARE_IPD)
        {
            continue;
        }

        /*
         * 转RectInfo
         */
        Common::RectInfo_S stRect;

        stRect.nX1 = pointData.stBoxs.nX1;

        stRect.nY1 = pointData.stBoxs.nY1;

        stRect.nX2 = pointData.stBoxs.nX2;

        stRect.nY2 = pointData.stBoxs.nY2;

        vstRectInfo.emplace_back(stRect);

        dlog_debug("[人脸比对] 有效目标 "
                   "confidence=%.3f "
                   "ipd=%d "
                   "rect=[%d,%d,%d,%d]",

                   pointData.fConfidence,
                   nIpd,

                   stRect.nX1,
                   stRect.nY1,
                   stRect.nX2,
                   stRect.nY2);
    }

    return !vstRectInfo.empty();
}
} // namespace FaceDetectInternal
