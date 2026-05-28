/**
 * @FilePath     : face_feature_processor.cpp
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2026-04-28 15:07:08
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-04-28 20:23:21
 * @Description  : 人脸特征提取与比对处理器实现
 */

#include "face_feature_processor.hpp"

#include <algorithm>
#include <fstream>
#include <sstream>
#include <sys/stat.h>
#include <unistd.h>

#include "capture_ctrl.h"
#include "email_manage.h"
#include "face_capture_processor.hpp"
#include "face_capture_temp_file.hpp"
#include "face_manage.h"
#include "IpcRet.h"
#include "storage_manage.h"
#include "video_frame_jpeg_encoder.hpp"

#ifdef ENABLE_TVSDK_SRC
#include "NetTVSDKServer.h"
#endif

namespace
{
/* 人脸目标框放大倍率，用于裁剪时保留人脸周边上下文 */
constexpr float FACE_REGION_SCALE_RATIO = 1.5f;
/* 特征模型输入宽度 */
constexpr int FACE_FEATURE_INPUT_WIDTH = 160;
/* 特征模型输入高度 */
constexpr int FACE_FEATURE_INPUT_HEIGHT = 160;
/* 人脸比对成功阈值 */
constexpr float FACE_COMPARE_SUCCESS_THRESHOLD = 0.7f;
/* 名单库检测时使用的人脸检测宽度 */
constexpr int FACE_LIB_DETECT_WIDTH = 640;
/* 名单库检测时使用的人脸检测高度 */
constexpr int FACE_LIB_DETECT_HEIGHT = 384;

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
    if (m_pFaceFeaHandle)
    {
        return true;
    }

    dlog_info("人脸特征句柄 初始化!!");
    std::string strModelPath = AI_FACE_FEATURE_CONFIG_FILE;
    m_pFaceFeaHandle = new Inference_NS::CImageFeature(strModelPath);
    if (m_pFaceFeaHandle && m_pFaceFeaHandle->init())
    {
        dlog_info("人脸特征句柄 初始化成功, %s", strModelPath.c_str());
        return true;
    }

    destroyFaceFeatureHandle(m_pFaceFeaHandle);
    m_pFaceFeaHandle = nullptr;
    dlog_error("人脸特征句柄 初始化失败");
    return false;
}

void CFaceFeatureProcessor::deinit()
{
    if (m_pFaceFeaHandle)
    {
        destroyFaceFeatureHandle(m_pFaceFeaHandle);
        m_pFaceFeaHandle = nullptr;
    }
}

void CFaceFeatureProcessor::processCompare(SFaceProcessContext &stContext,
                                           const std::vector<Common::RectInfo_S> &vstRectInfo,
                                           CFaceCaptureProcessor &stCaptureProcessor,
                                           std::vector<std::string> &vecImageFile)
{
    if (!m_stAlgoCfg.bEnable || !m_pFaceFeaHandle || stContext.pFrameInfo == nullptr)
    {
        return;
    }

    for (const auto &rect : vstRectInfo)
    {
        /* 当前目标提取到的特征向量 */
        std::vector<float> vecFeature;
        if (!extractFeature(rect, stContext.pFrameInfo, stContext.pNpuMutex, vecFeature))
        {
            continue;
        }

        /* 当前目标比对结果 */
        int nFaceLibId = -1;
        float fSimilarity = 0.0f;
        FaceManage::AIFaceManage::instance()->comparisonFaceLib(vecFeature, nFaceLibId, fSimilarity);

        const bool bCompareSuccess = fSimilarity >= FACE_COMPARE_SUCCESS_THRESHOLD;
        dlog_info("人脸比对结果: result=%d id=%d 相似度=%.3f", bCompareSuccess ? 1 : 0, nFaceLibId, fSimilarity);
        publishCompareSdkEvent(bCompareSuccess,
                               nFaceLibId,
                               fSimilarity,
                               rect,
                               stContext);
        handleCompareLinkage(bCompareSuccess,
                             rect,
                             stContext.pFrameInfo,
                             stContext.nChnId,
                             stCaptureProcessor,
                             vecImageFile);
    }
}

bool CFaceFeatureProcessor::addFaceLibGroup(FaceDataDB_NS::FaceLibsInfo_S &stFaceLibData,
                                            Inference_NS::CYoloUltralyticsPoint *pFaceDetHandle,
                                            std::mutex &npuMutex,
                                            int nWidth,
                                            int nHeight)
{
    dlog_info("=== [FaceLib] 开始添加人脸库===");
    if (pFaceDetHandle == nullptr)
    {
        dlog_error("检测模型初始化失败");
        return false;
    }

    if (!init())
    {
        dlog_error("特征模型初始化失败");
        return false;
    }

    /* 当前名单库原始图片宽高 */
    const int w = stFaceLibData.PicWidth;
    const int h = stFaceLibData.PicHeight;
    const size_t nv21Size = static_cast<size_t>(w) * h * 3 / 2;
    std::vector<uint8_t> nv21(nv21Size);

    std::ifstream file(stFaceLibData.BinPath, std::ios::binary);
    if (!file)
    {
        dlog_error("打开NV21失败: %s", stFaceLibData.BinPath.c_str());
        return false;
    }

    file.read(reinterpret_cast<char *>(nv21.data()), static_cast<std::streamsize>(nv21Size));
    file.close();

    /* 名单库原始源帧 */
    ot_video_frame_info stSrc;
    if (TD_SUCCESS != mppVgs_create_video_frame_info(w,
                                                     h,
                                                     OT_PIXEL_FORMAT_YVU_SEMIPLANAR_420,
                                                     &stSrc))
    {
        dlog_error("创建源帧失败");
        return false;
    }

    memcpy(stSrc.video_frame.virt_addr[0], nv21.data(), nv21Size);

    /* 名单库检测帧，必要时缩放到检测模型分辨率 */
    ot_video_frame_info stDet;
    ot_video_frame_info *pDet = &stSrc;
    if (w != FACE_LIB_DETECT_WIDTH || h != FACE_LIB_DETECT_HEIGHT)
    {
        if (TD_SUCCESS != mppVgs_create_video_frame_info(FACE_LIB_DETECT_WIDTH,
                                                         FACE_LIB_DETECT_HEIGHT,
                                                         OT_PIXEL_FORMAT_YVU_SEMIPLANAR_420,
                                                         &stDet))
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
    }

    Inference_NS::InputData_S detInput;
    detInput.pData = reinterpret_cast<float *>(pDet->video_frame.virt_addr[0]);
    detInput.nDataSize = static_cast<int>(nWidth * nHeight * 1.5) * sizeof(float);
    std::vector<Inference_NS::PointData_S> vDet;
    {
        std::lock_guard<std::mutex> lock(npuMutex);
        pFaceDetHandle->inference(detInput, vDet);
    }

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

    /* 当前名单库图片中最优人脸检测结果 */
    auto best = *std::max_element(vDet.begin(), vDet.end(),
                                  [](auto &a, auto &b)
                                  {
                                      return a.fConfidence < b.fConfidence;
                                  });
    dlog_info("检测成功 conf=%.6f", best.fConfidence);

    /* 当前名单库人脸框，必要时映射回原始分辨率 */
    Common::RectInfo_S face;
    if (pDet == &stDet)
    {
        const float rw = static_cast<float>(w) / FACE_LIB_DETECT_WIDTH;
        const float rh = static_cast<float>(h) / FACE_LIB_DETECT_HEIGHT;
        face.nX1 = static_cast<int>(best.stBoxs.nX1 * rw);
        face.nY1 = static_cast<int>(best.stBoxs.nY1 * rh);
        face.nX2 = static_cast<int>(best.stBoxs.nX2 * rw);
        face.nY2 = static_cast<int>(best.stBoxs.nY2 * rh);
        dlog_info("映射比例：=%.3f =%.3f", rw, rh);
    }
    else
    {
        face.nX1 = best.stBoxs.nX1;
        face.nY1 = best.stBoxs.nY1;
        face.nX2 = best.stBoxs.nX2;
        face.nY2 = best.stBoxs.nY2;
    }

    /* 当前名单库图片提取到的特征向量 */
    std::vector<float> vecFeature;
    bool bExtractOk = extractFeature(face, &stSrc, &npuMutex, vecFeature);

    if (pDet == &stDet)
    {
        mppVgs_destroy_video_frame_info(&stDet);
    }
    mppVgs_destroy_video_frame_info(&stSrc);

    if (!bExtractOk)
    {
        dlog_error("特征失败");
        return true;
    }

    stFaceLibData.vfData = vecFeature;
    stFaceLibData.nModelState = 1;
    if (FaceManage::AIFaceManage::instance()->addFaceLibInfo(stFaceLibData) != OK)
    {
        dlog_error("人脸库添加到数据库失败");
        return false;
    }
    dlog_info("人脸特征向量 (共 %d 维):", static_cast<int>(stFaceLibData.vfData.size()));
    for (size_t i = 0; i < 10 && i < stFaceLibData.vfData.size(); ++i)
    {
        dlog_info("%.4f ", stFaceLibData.vfData[i]);
    }

    dlog_info("=== 人脸库添加成功 ===");
    return true;
}

bool CFaceFeatureProcessor::isEnabled() const
{
    return m_stAlgoCfg.bEnable;
}

bool CFaceFeatureProcessor::isInitialized() const
{
    return m_pFaceFeaHandle != nullptr;
}

FaceCompareLinkageOptions_S CFaceFeatureProcessor::buildLinkageOptions(bool bSuccess) const
{
    return bSuccess ? m_stSuccessLinkage : m_stFailLinkage;
}

bool CFaceFeatureProcessor::extractFeature(const Common::RectInfo_S &stRect,
                                           ot_video_frame_info *pFrameInfo,
                                           std::mutex *pNpuMutex,
                                           std::vector<float> &vecFeature)
{
    vecFeature.clear();
    if (pFrameInfo == nullptr || m_pFaceFeaHandle == nullptr || pNpuMutex == nullptr)
    {
        return false;
    }

    /* 当前裁剪并缩放后的 160x160 人脸帧 */
    ot_video_frame_info stFaceFrame;
    if (!prepareFace160Frame(stRect, pFrameInfo, pFrameInfo->video_frame.width, pFrameInfo->video_frame.height, stFaceFrame))
    {
        return false;
    }

    /* 当前特征模型输入浮点数组 */
    std::vector<float> inputBuffer;
    convertYuvToFloat160(stFaceFrame, inputBuffer);
    std::vector<uint16_t> inputFP16(FACE_FEATURE_INPUT_WIDTH * FACE_FEATURE_INPUT_HEIGHT * 3);
    for (size_t i = 0; i < inputFP16.size(); ++i)
    {
        inputFP16[i] = float32ToFloat16(inputBuffer[i]);
    }

    Inference_NS::InputData_S stInputData;
    stInputData.pData = reinterpret_cast<float *>(inputFP16.data());
    stInputData.nDataSize = static_cast<int>(inputFP16.size() * sizeof(uint16_t));
    std::vector<Inference_NS::ClsData_S> vClsDatas;
    {
        std::lock_guard<std::mutex> lock(*pNpuMutex);
        m_pFaceFeaHandle->inference(stInputData, vClsDatas);
    }

    mppVgs_destroy_video_frame_info(&stFaceFrame);
    if (vClsDatas.empty())
    {
        return false;
    }

    vecFeature = vClsDatas[0].vFeature;
    return true;
}

void CFaceFeatureProcessor::handleCompareLinkage(bool bSuccess,
                                                 const Common::RectInfo_S &stRect,
                                                 ot_video_frame_info *pFrameInfo,
                                                 int nChnId,
                                                 CFaceCaptureProcessor &stCaptureProcessor,
                                                 std::vector<std::string> &vecImageFile)
{
    const FaceCompareLinkageOptions_S stOptions = buildLinkageOptions(bSuccess);
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
    stEventInfo.enType = Event::Type_E::FACE_CAPTURE;
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

        if (stOptions.bTargetImage)
        {
            std::vector<Common::RectInfo_S> vstSingleRectInfo{ stRect };
            stCaptureProcessor.saveFaceImage(vstSingleRectInfo, pFrameInfo, nChnId, vecImageFile);
        }
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

void CFaceFeatureProcessor::publishCompareSdkEvent(bool bSuccess,
                                                   int nFaceLibId,
                                                   float fSimilarity,
                                                   const Common::RectInfo_S &stRect,
                                                   SFaceProcessContext &stContext)
{
    if (!m_sdkComparePublisher.hasClient() || stContext.pFrameInfo == nullptr)
    {
        return;
    }

    std::vector<unsigned char> vecCapFaceJpeg;
    if (!buildCompareTargetImage(stRect, stContext.pFrameInfo, stContext.nChnId, vecCapFaceJpeg))
    {
        return;
    }

    FaceDataDB_NS::FaceLibsInfo_S stFaceLibInfo;
    stFaceLibInfo.clear();
    if (bSuccess && nFaceLibId >= 0)
    {
        FaceManage::AIFaceManage::instance()->searchFaceInfoById(nFaceLibId, stFaceLibInfo);
    }

    std::vector<unsigned char> vecLibFaceJpeg;
    if (!stFaceLibInfo.strPicPath.empty())
    {
        loadJpegFile(stFaceLibInfo.strPicPath, vecLibFaceJpeg);
    }

    FaceCompareSdkResult_S stSdkResult;
    stSdkResult.bSuccess = bSuccess;
    stSdkResult.nFaceId = bSuccess ? nFaceLibId : -1;
    stSdkResult.fSimilarity = fSimilarity;
    stSdkResult.nChnId = stContext.nChnId;
    stSdkResult.strFaceName = stFaceLibInfo.strName;
    stSdkResult.strFaceLibName = stFaceLibInfo.strFaceLibName;
    stSdkResult.strLibFacePath = stFaceLibInfo.strPicPath;
    stSdkResult.pvecCaptureJpeg = &vecCapFaceJpeg;
    stSdkResult.pvecLibFaceJpeg = &vecLibFaceJpeg;
    m_sdkComparePublisher.publish(stSdkResult);
}

bool CFaceFeatureProcessor::buildCompareTargetImage(const Common::RectInfo_S &stRect,
                                                    ot_video_frame_info *pFrameInfo,
                                                    int nChnId,
                                                    std::vector<unsigned char> &vecJpeg) const
{
    vecJpeg.clear();
    if (pFrameInfo == nullptr)
    {
        return false;
    }

    Common::RectInfo_S stFaceRect = stRect;
    convert_region_ratio(stFaceRect,
                         FACE_REGION_SCALE_RATIO,
                         pFrameInfo->video_frame.width,
                         pFrameInfo->video_frame.height);
    stFaceRect.nX1 = ALIGN_BACK(stFaceRect.nX1, 16);
    stFaceRect.nY1 = ALIGN_BACK(stFaceRect.nY1, 4);
    stFaceRect.nX2 = ALIGN_BACK(stFaceRect.nX2, 16);
    stFaceRect.nY2 = ALIGN_BACK(stFaceRect.nY2, 4);

    if (stFaceRect.nX2 <= stFaceRect.nX1 || stFaceRect.nY2 <= stFaceRect.nY1)
    {
        dlog_warn("人脸比对 SDK 图片编码跳过，目标框无效 [%d,%d,%d,%d]",
                  stFaceRect.nX1,
                  stFaceRect.nY1,
                  stFaceRect.nX2,
                  stFaceRect.nY2);
        return false;
    }

    const unsigned int unDstWidth = stFaceRect.nX2 - stFaceRect.nX1;
    const unsigned int unDstHeight = stFaceRect.nY2 - stFaceRect.nY1;
    ot_video_frame_info stDstFrameInfo;
    if (TD_SUCCESS != mppVgs_create_video_frame_info(unDstWidth,
                                                     unDstHeight,
                                                     OT_PIXEL_FORMAT_YVU_SEMIPLANAR_420,
                                                     &stDstFrameInfo))
    {
        return false;
    }

    ot_rect stCropRect;
    stCropRect.x = stFaceRect.nX1;
    stCropRect.y = stFaceRect.nY1;
    stCropRect.width = unDstWidth;
    stCropRect.height = unDstHeight;
    if (TD_SUCCESS != mppVgs_crop(pFrameInfo, &stDstFrameInfo, &stCropRect))
    {
        mppVgs_destroy_video_frame_info(&stDstFrameInfo);
        return false;
    }

    CFaceCaptureTempFile stTempFile(buildCompareTempFilePath(nChnId));
    const int nEncodeRet = AiAppCommon::encode_video_frame_to_jpeg_file(&stDstFrameInfo, stTempFile.path());
    mppVgs_destroy_video_frame_info(&stDstFrameInfo);
    if (nEncodeRet != OK)
    {
        return false;
    }

    return loadJpegFile(stTempFile.path(), vecJpeg);
}

bool CFaceFeatureProcessor::loadJpegFile(const std::string &strFilename, std::vector<unsigned char> &vecJpeg) const
{
    vecJpeg.clear();
    if (strFilename.empty())
    {
        return false;
    }

    std::ifstream file(strFilename, std::ios::binary);
    if (!file.is_open())
    {
        dlog_warn("读取人脸比对 SDK 图片失败，文件[%s]", strFilename.c_str());
        return false;
    }

    file.seekg(0, std::ios::end);
    const std::streampos nFileSize = file.tellg();
    if (nFileSize <= 0)
    {
        return false;
    }
#ifdef ENABLE_TVSDK_SRC
    if (nFileSize > static_cast<std::streampos>(NET_TV_FACE_IMAGE_MAX_LEN))
    {
        dlog_warn("读取人脸比对 SDK 图片失败，文件过大[%lld] 上限[%u] 文件[%s]",
                  static_cast<long long>(nFileSize),
                  static_cast<unsigned int>(NET_TV_FACE_IMAGE_MAX_LEN),
                  strFilename.c_str());
        return false;
    }
#endif

    file.seekg(0, std::ios::beg);
    vecJpeg.resize(static_cast<size_t>(nFileSize));
    const std::streamsize nReadSize = static_cast<std::streamsize>(nFileSize);
    file.read(reinterpret_cast<char *>(vecJpeg.data()), nReadSize);
    return file.gcount() == nReadSize;
}

std::string CFaceFeatureProcessor::buildCompareTempFilePath(int nChnId) const
{
    return "/tmp/face_compare_sdk_" +
           std::to_string(nChnId < 0 ? 0 : nChnId) + "_" +
           TimeUtils_NS::get_currentTimeMs() + ".jpg";
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
    if (TD_SUCCESS != mppVgs_create_video_frame_info(cropW,
                                                     cropH,
                                                     OT_PIXEL_FORMAT_YVU_SEMIPLANAR_420,
                                                     &stCropFrame))
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
    uint32_t bits = *((uint32_t *)&value);
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
} // namespace FaceDetectInternal
