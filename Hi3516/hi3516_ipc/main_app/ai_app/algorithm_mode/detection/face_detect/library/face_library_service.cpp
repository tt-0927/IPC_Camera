/**
 * @FilePath     : face_library_service.cpp
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2026-10-08
 * @Description  : 人脸名单库服务实现——上传图入库编排 + 库比对/详情查询转发。
 *
 * 入库流程照搬 Legacy CFaceFeatureProcessor::addFaceLibGroup（face_feature_processor.cpp）
 * 的语义：NV21 读入 MMZ 帧 → 非整幅 640x640 时 VGS 缩放 → 异步检测（10ms 轮询、
 * 5 秒上限）→ 单人脸校验（0 脸/多脸拒绝）→ 置信度最高框映射回原图坐标裁剪缩放
 * 112x112 提特征 → 全库查重（相似度 > 0.5 判重复）→ 落库。
 * 执行器换成统一检测层 CFaceDetectWorker 临时实例（start(true) 低内存互斥模式：
 * YOLO 与 ArcFace 按任务切换驻留），入库毕即销毁；并修复 Legacy 多处失败分支
 * "return false == FACE_LIB_ADD_OK 被上游当成功"的存量问题。
 */

#include "face_library_service.hpp"

#if CAP_AI_FACE_COMPARE

#include <algorithm>
#include <fstream>
#include <string.h>
#include <unistd.h>

#include "IpcRet.h"
#include "algorithm.hpp"
#include "common_process.h"
#include "dlog.h"
#include "face_detect_worker.hpp"
#include "face_manage.h"

namespace
{
/* 名单库入库检测输入尺寸（上传图统一缩放到 640x640 送检，Legacy 口径） */
constexpr int FACE_LIB_DETECT_WIDTH = 640;
constexpr int FACE_LIB_DETECT_HEIGHT = 640;
/* ArcFace 特征模型输入尺寸 */
constexpr int FACE_FEATURE_INPUT_WIDTH = 112;
constexpr int FACE_FEATURE_INPUT_HEIGHT = 112;
/* 提特征裁剪框缩放倍率（1.0 倍人脸框，Legacy 口径） */
constexpr float FACE_REGION_SCALE_RATIO = 1.0f;
/* 入库查重阈值：与库内已有人员相似度超过该值判为重复（Legacy 口径不变） */
constexpr float FACE_LIB_DUPLICATE_SIMILARITY = 0.5f;
/* 检测任务等待上限（毫秒） */
constexpr int DETECT_WAIT_TIMEOUT_MS = 5000;
/*
 * 特征任务等待上限（毫秒）：互斥模式下 YOLO→ArcFace 模型切换耗时较长，
 * Legacy 按 8 秒等待，排队/切模型期间不判失败。
 */
constexpr int FEATURE_WAIT_TIMEOUT_MS = 8000;
/* 任务结果轮询间隔（毫秒） */
constexpr int TASK_POLL_INTERVAL_MS = 10;

/*
 * 轮询等待异步任务出终态（成功/失败/超时）：
 * 轮询到上限仍无终态时按超时处理（Legacy 该场景落入"未检测到人脸"分支，
 * 这里按真实原因返回超时，属错误码修复范围）。
 */
CFaceDetectWorker::TaskState waitTaskTerminalState(CFaceDetectWorker &stWorker,
                                                   const std::string &strTaskId,
                                                   int nTimeoutMs,
                                                   CFaceDetectWorker::TaskResult &stResult)
{
    int nWaitMs = 0;
    while (nWaitMs < nTimeoutMs)
    {
        if (stWorker.queryTaskResult(strTaskId, stResult))
        {
            if (stResult.state == CFaceDetectWorker::TaskState::SUCCESS || stResult.state == CFaceDetectWorker::TaskState::FAILED ||
                stResult.state == CFaceDetectWorker::TaskState::TIMEOUT)
            {
                return stResult.state;
            }
        }
        usleep(TASK_POLL_INTERVAL_MS * 1000);
        nWaitMs += TASK_POLL_INTERVAL_MS;
    }
    return CFaceDetectWorker::TaskState::TIMEOUT;
}

/*
 * 从上传图原图帧提取人脸特征（等价 Legacy prepareFace160Frame + extractFeature
 * 的入库路径）：检测框（640x640 坐标）按比例映射回原图坐标，1.0 倍等比框收进
 * 图像边界并按 VGS 约束对齐后，从原图裁剪并缩放 112x112，经临时 Worker 异步
 * 推理取特征向量。裁剪/输入临时帧用后即毁，不跨帧。
 */
bool extractFeatureFromUploadImage(const ot_video_frame_info *pSrcFrame,
                                   const Inference_NS::BoxData_S &stFaceBox,
                                   CFaceDetectWorker &stWorker,
                                   std::vector<float> &vecFeature)
{
    vecFeature.clear();
    if (pSrcFrame == nullptr)
    {
        return false;
    }

    const int nOrigWidth = static_cast<int>(pSrcFrame->video_frame.width);
    const int nOrigHeight = static_cast<int>(pSrcFrame->video_frame.height);

    /* 检测框在 640x640 检测空间，映射回原图坐标（与 Legacy 同款换算） */
    Common::RectInfo_S stFaceRect;
    if (nOrigWidth != FACE_LIB_DETECT_WIDTH || nOrigHeight != FACE_LIB_DETECT_HEIGHT)
    {
        const float fRatioW = static_cast<float>(nOrigWidth) / FACE_LIB_DETECT_WIDTH;
        const float fRatioH = static_cast<float>(nOrigHeight) / FACE_LIB_DETECT_HEIGHT;
        stFaceRect.nX1 = static_cast<int>(stFaceBox.stBoxs.nX1 * fRatioW);
        stFaceRect.nY1 = static_cast<int>(stFaceBox.stBoxs.nY1 * fRatioH);
        stFaceRect.nX2 = static_cast<int>(stFaceBox.stBoxs.nX2 * fRatioW);
        stFaceRect.nY2 = static_cast<int>(stFaceBox.stBoxs.nY2 * fRatioH);
    }
    else
    {
        stFaceRect.nX1 = stFaceBox.stBoxs.nX1;
        stFaceRect.nY1 = stFaceBox.stBoxs.nY1;
        stFaceRect.nX2 = stFaceBox.stBoxs.nX2;
        stFaceRect.nY2 = stFaceBox.stBoxs.nY2;
    }

    /* 1.0 倍等比扩缩并收进图像边界（Legacy 口径），再按 VGS 对齐约束回退 */
    convert_region_ratio(stFaceRect, FACE_REGION_SCALE_RATIO, nOrigWidth, nOrigHeight);
    stFaceRect.nX1 = ALIGN_BACK(stFaceRect.nX1, 16);
    stFaceRect.nY1 = ALIGN_BACK(stFaceRect.nY1, 4);
    stFaceRect.nX2 = ALIGN_BACK(stFaceRect.nX2, 16);
    stFaceRect.nY2 = ALIGN_BACK(stFaceRect.nY2, 4);
    if (stFaceRect.nX2 <= stFaceRect.nX1 || stFaceRect.nY2 <= stFaceRect.nY1)
    {
        dlog_warn("[人脸入库] 人脸框映射到原图后无效 [%d,%d,%d,%d]，无法裁剪提特征",
                  stFaceRect.nX1,
                  stFaceRect.nY1,
                  stFaceRect.nX2,
                  stFaceRect.nY2);
        return false;
    }

    ot_video_frame_info stCropFrame;
    ot_video_frame_info stFaceFrame;
    memset(&stCropFrame, 0, sizeof(stCropFrame));
    memset(&stFaceFrame, 0, sizeof(stFaceFrame));
    bool bCropFrameCreated = false;
    bool bFaceFrameCreated = false;
    bool bExtractOk = false;
    do
    {
        /* 人脸局部裁剪帧 */
        const unsigned int unCropWidth = static_cast<unsigned int>(stFaceRect.nX2 - stFaceRect.nX1);
        const unsigned int unCropHeight = static_cast<unsigned int>(stFaceRect.nY2 - stFaceRect.nY1);
        if (TD_SUCCESS != mppVgs_create_video_frame_info(unCropWidth, unCropHeight, OT_PIXEL_FORMAT_YVU_SEMIPLANAR_420, &stCropFrame))
        {
            break;
        }
        bCropFrameCreated = true;

        ot_rect stCropRect;
        stCropRect.x = stFaceRect.nX1;
        stCropRect.y = stFaceRect.nY1;
        stCropRect.width = unCropWidth;
        stCropRect.height = unCropHeight;
        /* VGS 裁剪只读源帧，接口未标 const */
        if (TD_SUCCESS != mppVgs_crop(const_cast<ot_video_frame_info *>(pSrcFrame), &stCropFrame, &stCropRect))
        {
            break;
        }

        /* 特征模型输入帧：112x112 */
        if (TD_SUCCESS != mppVgs_create_video_frame_info(FACE_FEATURE_INPUT_WIDTH,
                                                         FACE_FEATURE_INPUT_HEIGHT,
                                                         OT_PIXEL_FORMAT_YVU_SEMIPLANAR_420,
                                                         &stFaceFrame))
        {
            break;
        }
        bFaceFrameCreated = true;
        if (TD_SUCCESS != mppVgs_scale(&stCropFrame, &stFaceFrame))
        {
            break;
        }

        /* 异步特征任务 → 轮询等结果（互斥模式下含 YOLO→ArcFace 切换时间） */
        const std::string strTaskId = stWorker.submitFeatureTask(stFaceFrame, FACE_FEATURE_INPUT_WIDTH, FACE_FEATURE_INPUT_HEIGHT);
        CFaceDetectWorker::TaskResult stTaskResult;
        const CFaceDetectWorker::TaskState enState = waitTaskTerminalState(stWorker, strTaskId, FEATURE_WAIT_TIMEOUT_MS, stTaskResult);
        stWorker.removeTask(strTaskId);
        if (enState == CFaceDetectWorker::TaskState::SUCCESS && !stTaskResult.feature.empty())
        {
            vecFeature = stTaskResult.feature;
            bExtractOk = true;
        }
    }
    while (false);

    if (bFaceFrameCreated)
    {
        mppVgs_destroy_video_frame_info(&stFaceFrame);
    }
    if (bCropFrameCreated)
    {
        mppVgs_destroy_video_frame_info(&stCropFrame);
    }
    return bExtractOk;
}
} // namespace

namespace FaceChain_NS
{
int CFaceLibraryService::enrollFromImage(FaceDataDB_NS::FaceLibsInfo_S &stFaceLibData)
{
    dlog_info("[人脸入库] 开始：名单组%s 姓名%s 图片宽%d 高%d 路径%s",
              stFaceLibData.strFaceLibName.c_str(),
              stFaceLibData.strName.c_str(),
              stFaceLibData.PicWidth,
              stFaceLibData.PicHeight,
              stFaceLibData.BinPath.c_str());

    int nRet = FACE_LIB_ADD_OK;
    ot_video_frame_info stSrc;
    ot_video_frame_info stDet;
    memset(&stSrc, 0, sizeof(stSrc));
    memset(&stDet, 0, sizeof(stDet));
    bool bSrcFrameCreated = false;
    bool bDetFrameCreated = false;

    /*
     * 临时轻量 Worker（独占模式）：常驻算法已被调用方独占框架暂停，独占模式
     * 保证 YOLO/ArcFace 按任务切换驻留、内存峰值受控；生命周期仅限本函数，
     * 结尾统一 deinit，先于调用方恢复常驻算法。
     */
    CFaceDetectWorker stWorker;

    do
    {
        if (!stWorker.start(true))
        {
            nRet = ERR_FACE_MODEL_START_FAILED;
            dlog_error("[人脸入库] 临时检测线程启动失败：本次入库中止，不加载模型，平台将收到错误码%d", nRet);
            break;
        }

        /*
         * NV21 读入 MMZ 帧（避免低内存设备上再留一份完整堆内存副本）。
         */
        const int nWidth = stFaceLibData.PicWidth;
        const int nHeight = stFaceLibData.PicHeight;
        const size_t nv21Size = static_cast<size_t>(nWidth) * nHeight * 3 / 2;

        if (TD_SUCCESS != mppVgs_create_video_frame_info(nWidth, nHeight, OT_PIXEL_FORMAT_YVU_SEMIPLANAR_420, &stSrc))
        {
            nRet = ERR_FACE_SOURCE_FRAME_ALLOC_FAILED;
            dlog_error("[人脸入库] 原图帧分配失败（通常为内存不足）：本次入库中止，平台将收到错误码%d", nRet);
            break;
        }
        bSrcFrameCreated = true;

        std::ifstream file(stFaceLibData.BinPath, std::ios::binary);
        if (!file)
        {
            nRet = ERR_FACE_NV21_OPEN_FAILED;
            dlog_error("[人脸入库] 打开上传图片失败：路径%s，本次入库中止，平台将收到错误码%d", stFaceLibData.BinPath.c_str(), nRet);
            break;
        }
        file.read(reinterpret_cast<char *>(stSrc.video_frame.virt_addr[0]), static_cast<std::streamsize>(nv21Size));
        const size_t nReadSize = static_cast<size_t>(file.gcount());
        file.close();
        if (nReadSize != nv21Size)
        {
            nRet = ERR_FACE_NV21_SIZE_MISMATCH;
            dlog_error("[人脸入库] 上传图片大小与宽高不符：路径%s 需要%zu字节 实读%zu字节，本次入库中止，平台将收到错误码%d",
                       stFaceLibData.BinPath.c_str(),
                       nv21Size,
                       nReadSize,
                       nRet);
            break;
        }

        /* 非整幅 640x640 时先缩放到检测输入尺寸 */
        ot_video_frame_info *pDetFrame = &stSrc;
        if (nWidth != FACE_LIB_DETECT_WIDTH || nHeight != FACE_LIB_DETECT_HEIGHT)
        {
            if (TD_SUCCESS !=
                mppVgs_create_video_frame_info(FACE_LIB_DETECT_WIDTH, FACE_LIB_DETECT_HEIGHT, OT_PIXEL_FORMAT_YVU_SEMIPLANAR_420, &stDet))
            {
                nRet = ERR_FACE_RESIZE_FRAME_ALLOC_FAILED;
                dlog_error("[人脸入库] 640x640检测帧分配失败（通常为内存不足）：本次入库中止，平台将收到错误码%d", nRet);
                break;
            }
            bDetFrameCreated = true;
            if (TD_SUCCESS != mppVgs_scale(&stSrc, &stDet))
            {
                nRet = ERR_FACE_RESIZE_FAILED;
                dlog_error("[人脸入库] 上传图片缩放到640x640失败：本次入库中止，平台将收到错误码%d", nRet);
                break;
            }
            pDetFrame = &stDet;
        }

        /* 异步检测 → 轮询等结果（只等任务状态，不占用推理线程） */
        const std::string strTaskId = stWorker.submitFaceImage(pDetFrame, FACE_LIB_DETECT_WIDTH, FACE_LIB_DETECT_HEIGHT);
        CFaceDetectWorker::TaskResult stTaskResult;
        const CFaceDetectWorker::TaskState enState = waitTaskTerminalState(stWorker, strTaskId, DETECT_WAIT_TIMEOUT_MS, stTaskResult);
        stWorker.removeTask(strTaskId);
        if (enState != CFaceDetectWorker::TaskState::SUCCESS)
        {
            nRet = (enState == CFaceDetectWorker::TaskState::FAILED) ? ERR_FACE_DETECT_TASK_FAILED : ERR_FACE_DETECT_TIMEOUT;
            dlog_error("[人脸入库] 人脸检测未完成（任务状态%d）：本次入库中止，平台将收到错误码%d", static_cast<int>(enState), nRet);
            break;
        }

        /* 单人脸校验：0 脸/多脸都不允许入库 */
        const std::vector<Inference_NS::BoxData_S> &vDet = stTaskResult.result;
        if (vDet.empty())
        {
            nRet = ERR_DETECT_NO_FACES;
            dlog_error("[人脸入库] 图片中未检测到人脸：本次入库中止，平台将收到错误码%d", nRet);
            break;
        }
        if (vDet.size() > 1)
        {
            nRet = ERR_DETECT_MULTIPLE_FACES;
            dlog_error("[人脸入库] 图片中检测到%zu张人脸，只允许单人照：本次入库中止，平台将收到错误码%d", vDet.size(), nRet);
            break;
        }

        /* 取置信度最高框（单脸校验后恒为唯一框，保留与 Legacy 一致的选取逻辑） */
        const Inference_NS::BoxData_S stBest = *std::max_element(vDet.begin(),
                                                                 vDet.end(),
                                                                 [](const Inference_NS::BoxData_S &lhs, const Inference_NS::BoxData_S &rhs)
                                                                 {
                                                                     return lhs.fConfidence < rhs.fConfidence;
                                                                 });
        dlog_info("[人脸入库] 人脸检测成功：置信度%.2f", stBest.fConfidence);

        /* 提特征：检测框映射回原图坐标，裁剪缩放 112x112 后经 Worker 推理 */
        std::vector<float> vecFeature;
        if (!extractFeatureFromUploadImage(&stSrc, stBest, stWorker, vecFeature))
        {
            nRet = ERR_FACE_FEATURE_EXTRACT_FAILED;
            dlog_error("[人脸入库] 特征提取失败：本次入库中止，平台将收到错误码%d", nRet);
            break;
        }

        /* 查重：与库内已有人员相似度过高判为重复（阈值口径与 Legacy 一致） */
        int nFaceLibId = -1;
        float fSimilarity = 0.0f;
        FaceManage::AIFaceManage::instance()->comparisonFaceLib(vecFeature, nFaceLibId, fSimilarity);
        if (fSimilarity > FACE_LIB_DUPLICATE_SIMILARITY)
        {
            nRet = ERR_ADD_DUPLICATE_FACE;
            dlog_error("[人脸入库] 该人脸已入库：最相似人员编号%d 相似度%.3f，本次入库中止，平台将收到错误码%d",
                       nFaceLibId,
                       fSimilarity,
                       nRet);
            break;
        }

        /* 特征与建模状态写入并落库 */
        stFaceLibData.vfData = vecFeature;
        stFaceLibData.nModelState = 1;
        if (OK != FaceManage::AIFaceManage::instance()->addFaceLibInfo(stFaceLibData))
        {
            nRet = ERR_FACE_DATABASE_WRITE_FAILED;
            dlog_error("[人脸入库] 名单库数据库写入失败：本次入库中止，平台将收到错误码%d", nRet);
            break;
        }

        dlog_info("[人脸入库] 入库成功：名单组%s 姓名%s 特征%d维",
                  stFaceLibData.strFaceLibName.c_str(),
                  stFaceLibData.strName.c_str(),
                  static_cast<int>(vecFeature.size()));
    }
    while (false);

    /*
     * 统一清理：所有 early-return 路径共用——先销毁临时 Worker（独占模式
     * 卸载 YOLO/ArcFace，deinit join 等待在跑的任务做完，任务持有的帧指针
     * 此后才失效），再释放临时帧，最后调用方才恢复常驻算法。
     */
    if (stWorker.isRunning())
    {
        stWorker.deinit();
    }
    if (bDetFrameCreated)
    {
        mppVgs_destroy_video_frame_info(&stDet);
    }
    if (bSrcFrameCreated)
    {
        mppVgs_destroy_video_frame_info(&stSrc);
    }

    return nRet;
}

int CFaceLibraryService::enrollFromLiveTrack(int nTrackId, const std::string &strFaceLibName, const std::string &strName)
{
    /* 预留接口：实时流取优入库尚未实现，当前调用不处理并按不支持返回 */
    dlog_warn("[人脸入库] 实时流入库尚未实现：目标%d 名单组%s 姓名%s，本次请求不处理", nTrackId, strFaceLibName.c_str(), strName.c_str());
    return ERR_FACE_LIVE_ENROLL_UNSUPPORTED;
}

bool CFaceLibraryService::compareFeature(const std::vector<float> &vfFeature, int &nFaceId, float &fSimilarity)
{
    /* 统一出口：转发名单库全库余弦比对（行为与 Legacy 直连一致） */
    return FaceManage::AIFaceManage::instance()->comparisonFaceLib(vfFeature, nFaceId, fSimilarity);
}

int CFaceLibraryService::queryById(int nFaceId, FaceDataDB_NS::FaceLibsInfo_S &stFaceInfo)
{
    /* 统一出口：转发人员详情查询 */
    return FaceManage::AIFaceManage::instance()->searchFaceInfoById(nFaceId, stFaceInfo);
}
} // namespace FaceChain_NS

#endif /* CAP_AI_FACE_COMPARE */
