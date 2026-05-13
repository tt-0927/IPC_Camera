/**
 * @FilePath     : face_capture_processor.cpp
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2026-04-28 15:07:08
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-04-28 16:18:27
 * @Description  : 人脸抓拍处理器实现
 */

#include "face_capture_processor.hpp"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <unistd.h>

#include "capture_ctrl.h"
#include "capture_database.h"
#include "email_manage.h"
#include "storage_manage.h"
#include "face_capture_temp_file.hpp"
#include "video_frame_jpeg_encoder.hpp"

namespace
{
/* 人脸目标框放大倍率，用于目标小图裁剪时保留周边上下文 */
constexpr float FACE_REGION_SCALE_RATIO = 1.5f;
}

namespace FaceDetectInternal
{
void CFaceCaptureProcessor::setEnabled(bool bEnable)
{
    m_stAlgoCfg.bEnable = bEnable;
    if (!bEnable)
    {
        m_alarmStateMachine.reset();
        m_sdkEventPublisher.resetEvent();
    }
}

void CFaceCaptureProcessor::setAlgoParamCfg(const Alarm::FaceCapture_S &stAlgoCfg, int nWidth, int nHeight)
{
    dlog_debug("ai_app: 设置人脸抓拍参数");
    m_stAlgoCfg = stAlgoCfg;
    m_nWidth = nWidth;
    m_nHeight = nHeight;

    if (m_stAlgoCfg.bEnable)
    {
        /* 转换规则区域坐标到算法输入分辨率 */
        auto &region = m_stAlgoCfg.stRule.stRegion;
        region.ConvertResolution(PIXEL_WIDTH_1920, PIXEL_HEIGHT_1080, m_nWidth, m_nHeight);
        /* 转换最小瞳距区域坐标到算法输入分辨率 */
        auto &rect = m_stAlgoCfg.stRule.stMinIpdRect;
        rect.ConvertResolution(PIXEL_WIDTH_1920, PIXEL_HEIGHT_1080, m_nWidth, m_nHeight);
        for (auto &shieldedRegion : m_stAlgoCfg.stRule.vstShieldedRegion)
        {
            /* 转换屏蔽区域坐标到算法输入分辨率 */
            shieldedRegion.ConvertResolution(PIXEL_WIDTH_1920, PIXEL_HEIGHT_1080, m_nWidth, m_nHeight);
        }
    }

    m_bFacePanoramicImage = false;
    m_bFaceImage = false;
    m_bEmail = false;
    m_bUploadSdCard = false;
    for (auto &type : m_stAlgoCfg.stLinkageList.tradition)
    {
        if (type == int(Alarm::LinkageType::UPLOAD_PANORAMIC_IMAGE))
        {
            m_bFacePanoramicImage = true;
        }
        else if (type == int(Alarm::LinkageType::UPLOAD_TARGET_IMAGE))
        {
            m_bFaceImage = true;
        }
        else if (type == int(Alarm::LinkageType::SEND_EMAIL))
        {
            m_bEmail = true;
        }
        else if (type == int(Alarm::LinkageType::UPLOAD_SD_CARD))
        {
            m_bUploadSdCard = true;
        }
    }
}

void CFaceCaptureProcessor::process(SFaceProcessContext &stContext, std::vector<std::string> &vecImageFile)
{
    if (!m_stAlgoCfg.bEnable)
    {
        return;
    }

    /* 当前帧满足抓拍规则的人脸目标，联动层优先使用第一个目标生成 SDK 图片 */
    std::vector<FaceCaptureTarget_S> vecFaceCaptureTargets;
    const bool bFaceCaptureAlarm = collectTargets(stContext.vPointDatas, stContext.vstRectInfo, &vecFaceCaptureTargets);
    handleLinkage(bFaceCaptureAlarm,
                  vecFaceCaptureTargets,
                  stContext.vstRectInfo,
                  stContext.pFrameInfo,
                  stContext.nChnId,
                  vecImageFile);
}

bool CFaceCaptureProcessor::collectTargets(std::vector<Inference_NS::PointData_S> &vPointDatas,
                                           std::vector<Common::RectInfo_S> &vstRectInfo,
                                           std::vector<FaceCaptureTarget_S> *pvecTargets)
{
    if (pvecTargets != nullptr)
    {
        pvecTargets->clear();
    }

    if (vPointDatas.empty())
    {
        return false;
    }

    /* 当前人脸抓拍规则，后续区域、屏蔽区、灵敏度和瞳距判断均基于该配置 */
    auto &rule = m_stAlgoCfg.stRule;
    /* 是否启用检测区域约束，未配置时默认允许全画面触发 */
    const bool bHasDetectRegion = rule.stRegion.IsValid();
    /* 当前帧是否存在有效屏蔽区域，存在时需要逐个目标排除 */
    bool bHasShieldedRegion = false;
    for (auto &shieldedRegion : rule.vstShieldedRegion)
    {
        if (shieldedRegion.IsValid())
        {
            bHasShieldedRegion = true;
            break;
        }
    }

    /* 最小瞳距阈值，配置无效时使用算法默认值 */
    int nMinIpd = m_nMinIpd;
    if (rule.stMinIpdRect.IsValid())
    {
        nMinIpd = std::min(std::max(MIN_IPD, rule.stMinIpdRect.nWidth), MAX_IPD);
    }

    /* 置信度阈值，灵敏度越高阈值越低 */
    const float fSensitivityThreshold = 1.0f - rule.nSensitivity / 100.0f;
    /* 当前帧是否存在满足抓拍条件的人脸 */
    bool bIsAlarm = false;

    for (const auto &pointData : vPointDatas)
    {
        if (bHasShieldedRegion)
        {
            /* 当前目标是否命中任一屏蔽区域 */
            bool bIsResultInShielded = false;
            for (auto &shieldedRegion : rule.vstShieldedRegion)
            {
                if (shieldedRegion.IsValid() && is_in_region(shieldedRegion, pointData.stBoxs))
                {
                    bIsResultInShielded = true;
                    break;
                }
            }

            if (bIsResultInShielded)
            {
                continue;
            }
        }

        if (bHasDetectRegion && !is_in_region(rule.stRegion, pointData.stBoxs))
        {
            continue;
        }

        if (pointData.fConfidence < fSensitivityThreshold)
        {
            continue;
        }

        /* 当前目标双眼瞳距，vPoints[0] 与 vPoints[1] 分别对应两只眼睛关键点 */
        const int nIpd = pointData.vPoints[1].nX - pointData.vPoints[0].nX;
        if (nIpd < nMinIpd)
        {
            continue;
        }

        bIsAlarm = true;
        dlog_info("[人脸抓拍] 灵敏度[%.3f] > 阈值[%.3f] 瞳距 [%d] > [%d]",
                  pointData.fConfidence,
                  fSensitivityThreshold,
                  nIpd,
                  nMinIpd);

        add_result_to_vector(pointData, vstRectInfo);
        if (pvecTargets != nullptr)
        {
            /* 当前满足抓拍条件的目标信息，供 SDK 推送填充目标框和置信度 */
            FaceCaptureTarget_S stTarget;
            stTarget.stRect.nX1 = pointData.stBoxs.nX1;
            stTarget.stRect.nY1 = pointData.stBoxs.nY1;
            stTarget.stRect.nX2 = pointData.stBoxs.nX2;
            stTarget.stRect.nY2 = pointData.stBoxs.nY2;
            stTarget.fConfidence = pointData.fConfidence;
            stTarget.nIpd = nIpd;
            pvecTargets->emplace_back(stTarget);
        }
    }

    return bIsAlarm;
}

int CFaceCaptureProcessor::saveFaceImage(std::vector<Common::RectInfo_S> vstRectInfo,
                                          ot_video_frame_info *pSrcFrameInfo,
                                          int nChnId,
                                          std::vector<std::string> &vecImageFile)
{
    if (pSrcFrameInfo == nullptr)
    {
        dlog_error("人脸抓拍视频帧为空");
        return ERR_PTR_NULL;
    }

    if (vstRectInfo.empty())
    {
        dlog_error("人脸数据坐标组为空");
        return ERR_PARAM_NULL;
    }

    for (size_t i = 0; i < vstRectInfo.size(); i++)
    {
        auto &rect = vstRectInfo[i];
        convert_region_ratio(rect, FACE_REGION_SCALE_RATIO, m_nWidth, m_nHeight);
        /* VGS 裁剪必须是4字节对齐，宽度需16字节向下对齐，防止超限 */
        rect.nX1 = ALIGN_BACK(rect.nX1, 16);
        rect.nY1 = ALIGN_BACK(rect.nY1, 4);
        rect.nX2 = ALIGN_BACK(rect.nX2, 16);
        rect.nY2 = ALIGN_BACK(rect.nY2, 4);

        /* 裁剪后目标小图宽高 */
        const unsigned int unDstWidth = rect.nX2 - rect.nX1;
        const unsigned int unDstHeight = rect.nY2 - rect.nY1;
        /* 当前目标小图裁剪帧 */
        ot_video_frame_info stDstFrameInfo;
        if (TD_SUCCESS != mppVgs_create_video_frame_info(unDstWidth,
                                                         unDstHeight,
                                                         OT_PIXEL_FORMAT_YVU_SEMIPLANAR_420,
                                                         &stDstFrameInfo))
        {
            continue;
        }

        /* 当前目标小图裁剪区域 */
        ot_rect stRect;
        stRect.x = rect.nX1;
        stRect.y = rect.nY1;
        stRect.width = unDstWidth;
        stRect.height = unDstHeight;

        if (TD_SUCCESS != mppVgs_crop(pSrcFrameInfo, &stDstFrameInfo, &stRect))
        {
            mppVgs_destroy_video_frame_info(&stDstFrameInfo);
            continue;
        }

        /* 获取按日期分类的存储路径 */
        std::string strStoragePath = CCaptureCtrl::instance()->get_date_storage_path();
        if (!CCaptureCtrl::instance()->ensure_directory_exists(strStoragePath))
        {
            dlog_error("[人脸抓拍]确保目录存在失败");
            mppVgs_destroy_video_frame_info(&stDstFrameInfo);
            return ERR;
        }

        /* 当前抓图日期和时间，保存文件名和数据库记录保持一致 */
        std::string strCurrentDate = TimeUtils_NS::get_currentDateWithDash();
        std::string strCurrentTime = TimeUtils_NS::get_currentTimeWithColon();
        std::string strFilename = strStoragePath + "/" +
                                  TimeUtils_NS::get_currentDate() + "_" +
                                  TimeUtils_NS::get_currentTimeMs() + "_" +
                                  std::to_string(static_cast<int>(Event::Type_E::FACE_CAPTURE)) + "_" +
                                  std::to_string(int(Alarm::LinkageType::UPLOAD_TARGET_IMAGE)) + "_" +
                                  std::to_string(i + 1) + ".jpg";

        if (AiAppCommon::encode_video_frame_to_jpeg_file(&stDstFrameInfo, strFilename) != OK)
        {
            mppVgs_destroy_video_frame_info(&stDstFrameInfo);
            continue;
        }
        vecImageFile.emplace_back(strFilename);
        saveToDatabase(strFilename, strCurrentDate, strCurrentTime, nChnId);
        mppVgs_destroy_video_frame_info(&stDstFrameInfo);
    }

    return OK;
}

bool CFaceCaptureProcessor::isEnabled() const
{
    return m_stAlgoCfg.bEnable;
}

FaceCaptureLinkageOptions_S CFaceCaptureProcessor::buildLinkageOptions() const
{
    /* 人脸抓拍联动配置快照，保证本次事件内判断口径一致 */
    FaceCaptureLinkageOptions_S stOptions;
    stOptions.bPanoramaImage = m_bFacePanoramicImage;
    stOptions.bTargetImage = m_bFaceImage;
    stOptions.bEmail = m_bEmail;
    stOptions.bUploadSdCard = m_bUploadSdCard;
    return stOptions;
}

void CFaceCaptureProcessor::handleLinkage(bool bAlarm,
                                           const std::vector<FaceCaptureTarget_S> &vecTargets,
                                           const std::vector<Common::RectInfo_S> &vstRectInfo,
                                           ot_video_frame_info *pFrameInfo,
                                           int nChnId,
                                           std::vector<std::string> &vecImageFile)
{
    const FaceCaptureLinkageOptions_S stOptions = buildLinkageOptions();
    if (!bAlarm)
    {
        m_sdkEventPublisher.resetEvent(nChnId);
        m_alarmStateMachine.handleAlarmState(false, Event::Type_E::FACE_CAPTURE);
        return;
    }

    /* SDK 推送独立于传统联动：事件首帧先推全景图，随后每帧逐个推送当前人脸小图 */
    m_sdkEventPublisher.publish(vecTargets,
                                pFrameInfo,
                                nChnId,
                                [this](ot_video_frame_info *pFrame, std::vector<unsigned char> &vecJpeg) {
                                    return buildSdkPanoramaImage(pFrame, vecJpeg);
                                },
                                [this](const Common::RectInfo_S &stRectInfo,
                                       ot_video_frame_info *pFrame,
                                       size_t nIndex,
                                       std::vector<unsigned char> &vecJpeg) {
                                    return buildSdkTargetImage(stRectInfo, pFrame, nIndex, vecJpeg);
                                });

    /* 传统报警状态机仍负责事件开始/结束和非 SDK 联动，确保旧功能兼容 */
    m_alarmStateMachine.handleAlarmState(true, Event::Type_E::FACE_CAPTURE);

    if (stOptions.bUploadSdCard && SD_CARD_STATUS_E::NORMAL == CStorageManage::instance()->get_SdCardStatus())
    {
        if (!access("testPrint", F_OK))
        {
            dlog_trace("人脸抓拍联动保存人脸图片开始");
        }

        /* 当前事件信息用于传统抓图模块创建 SD 卡存储记录 */
        Event::Info_S stEventInfo;
        stEventInfo.enType = Event::Type_E::FACE_CAPTURE;
        stEventInfo.strDate = TimeUtils_NS::get_currentDate();
        stEventInfo.strTime = TimeUtils_NS::get_currentTimeMs();
        stEventInfo.nChnId = nChnId < 0 ? 0 : nChnId;
        stEventInfo.strStartTime = TimeUtils_NS::get_currentDateWithDash() + " " +
                                   TimeUtils_NS::get_currentTimeWithColon();
        stEventInfo.strEndTime = stEventInfo.strStartTime;

        const int nRet = CCaptureCtrl::instance()->set_event_capture(false, stEventInfo);
        if (nRet == OK)
        {
            if (stOptions.bPanoramaImage)
            {
                /* 全景图沿用抓图模块生成的文件，避免重复编码同一帧 */
                auto strFaceImage = CCaptureCtrl::instance()->get_face_capture_file();
                if (!strFaceImage.empty())
                {
                    vecImageFile.emplace_back(strFaceImage);
                }
            }

            if (stOptions.bTargetImage)
            {
                saveFaceImage(vstRectInfo, pFrameInfo, nChnId, vecImageFile);
            }
        }

        if (stOptions.bEmail)
        {
            /* 邮件联动依赖本地附件路径，仅在 SD 卡保存路径有效时发送 */
            Network::EmailEventInfo_S stEmailInfo;
            stEmailInfo.strSubject = "人脸抓拍";

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
            dlog_trace("人脸抓拍联动保存人脸图片结束");
        }
    }
}

bool CFaceCaptureProcessor::buildSdkPanoramaImage(ot_video_frame_info *pFrameInfo,
                                                   std::vector<unsigned char> &vecJpeg)
{
    vecJpeg.clear();
    if (pFrameInfo == nullptr)
    {
        return false;
    }

    /* SDK 临时图片使用 /tmp，发送后由 RAII 对象自动删除，不进入抓图数据库 */
    CFaceCaptureTempFile stTempFile(buildTempFilePath("panorama", 0));
    if (AiAppCommon::encode_video_frame_to_jpeg_file(pFrameInfo, stTempFile.path()) != OK)
    {
        return false;
    }

    return loadJpegFile(stTempFile.path(), vecJpeg);
}

bool CFaceCaptureProcessor::buildSdkTargetImage(const Common::RectInfo_S &stRectInfo,
                                                ot_video_frame_info *pFrameInfo,
                                                size_t nIndex,
                                                std::vector<unsigned char> &vecJpeg)
{
    vecJpeg.clear();
    if (pFrameInfo == nullptr)
    {
        return false;
    }

    /* SDK 临时图片使用 /tmp，发送后由 RAII 对象自动删除，不进入抓图数据库 */
    CFaceCaptureTempFile stTempFile(buildTempFilePath("face", nIndex));
    if (encodeFaceTargetImageToFile(stRectInfo, pFrameInfo, stTempFile.path()) != OK)
    {
        return false;
    }

    return loadJpegFile(stTempFile.path(), vecJpeg);
}

int CFaceCaptureProcessor::encodeFaceTargetImageToFile(const Common::RectInfo_S &stRectInfo,
                                                       ot_video_frame_info *pSrcFrameInfo,
                                                       const std::string &strFilename)
{
    if (pSrcFrameInfo == nullptr || strFilename.empty())
    {
        dlog_error("人脸抓拍 SDK 图片编码参数无效");
        return ERR_PARAM_NULL;
    }

    /* 当前 SDK 目标小图矩形，按原保存逻辑放大 1.5 倍并对齐 VGS 约束 */
    Common::RectInfo_S stFaceRect = stRectInfo;
    convert_region_ratio(stFaceRect, FACE_REGION_SCALE_RATIO, m_nWidth, m_nHeight);
    stFaceRect.nX1 = ALIGN_BACK(stFaceRect.nX1, 16);
    stFaceRect.nY1 = ALIGN_BACK(stFaceRect.nY1, 4);
    stFaceRect.nX2 = ALIGN_BACK(stFaceRect.nX2, 16);
    stFaceRect.nY2 = ALIGN_BACK(stFaceRect.nY2, 4);

    if (stFaceRect.nX2 <= stFaceRect.nX1 || stFaceRect.nY2 <= stFaceRect.nY1)
    {
        dlog_warn("人脸抓拍 SDK 图片编码跳过，目标框无效 [%d,%d,%d,%d]",
                  stFaceRect.nX1,
                  stFaceRect.nY1,
                  stFaceRect.nX2,
                  stFaceRect.nY2);
        return ERR;
    }

    /* 裁剪后目标小图宽高 */
    const unsigned int unDstWidth = stFaceRect.nX2 - stFaceRect.nX1;
    const unsigned int unDstHeight = stFaceRect.nY2 - stFaceRect.nY1;
    /* 裁剪后的视频帧，编码完成后必须销毁 */
    ot_video_frame_info stDstFrameInfo;
    if (TD_SUCCESS != mppVgs_create_video_frame_info(unDstWidth,
                                                     unDstHeight,
                                                     OT_PIXEL_FORMAT_YVU_SEMIPLANAR_420,
                                                     &stDstFrameInfo))
    {
        return ERR;
    }

    /* VGS 裁剪区域，坐标和宽高均已按硬件要求对齐 */
    ot_rect stRect;
    stRect.x = stFaceRect.nX1;
    stRect.y = stFaceRect.nY1;
    stRect.width = unDstWidth;
    stRect.height = unDstHeight;

    if (TD_SUCCESS != mppVgs_crop(pSrcFrameInfo, &stDstFrameInfo, &stRect))
    {
        mppVgs_destroy_video_frame_info(&stDstFrameInfo);
        return ERR;
    }

    const int nEncodeRet = AiAppCommon::encode_video_frame_to_jpeg_file(&stDstFrameInfo, strFilename);
    mppVgs_destroy_video_frame_info(&stDstFrameInfo);
    return nEncodeRet;
}

bool CFaceCaptureProcessor::loadJpegFile(const std::string &strFilename, std::vector<unsigned char> &vecJpeg) const
{
    vecJpeg.clear();
    std::ifstream file(strFilename, std::ios::binary);
    if (!file.is_open())
    {
        dlog_warn("读取人脸抓拍 SDK 临时图片失败，文件[%s]", strFilename.c_str());
        return false;
    }

    file.seekg(0, std::ios::end);
    const std::streampos nFileSize = file.tellg();
    if (nFileSize <= 0)
    {
        return false;
    }

    file.seekg(0, std::ios::beg);
    vecJpeg.resize(static_cast<size_t>(nFileSize));
    const std::streamsize nReadSize = static_cast<std::streamsize>(nFileSize);
    file.read(reinterpret_cast<char *>(vecJpeg.data()), nReadSize);
    return file.gcount() == nReadSize;
}

std::string CFaceCaptureProcessor::buildTempFilePath(const std::string &strImageType, size_t nIndex) const
{
    return "/tmp/face_capture_sdk_" + strImageType + "_" +
           TimeUtils_NS::get_currentTimeMs() + "_" + std::to_string(nIndex) + ".jpg";
}

int CFaceCaptureProcessor::saveToDatabase(const std::string &strFilename,
                                          const std::string &strCurrentDate,
                                          const std::string &strCurrentTime,
                                          int nChnId)
{
    using namespace Db;
    Capture_NS::CaptureInfo_S stInfo;
    stInfo.nChnId = nChnId < 0 ? 0 : nChnId;
    stInfo.strImagePath = strFilename;
    stInfo.nImageSize = std::filesystem::file_size(strFilename);
    stInfo.strStartTime = strCurrentDate + " " + strCurrentTime;
    stInfo.strEndTime = stInfo.strStartTime;
    stInfo.enType = Event::Type_E::FACE_CAPTURE;
    CCaptureDatabase::instance()->add(stInfo);

    /* 更新图片数量、总大小至数据库表 */
    Capture_NS::CaptureDirInfo_S stDirInfo;
    stDirInfo.nChnId = nChnId < 0 ? 0 : nChnId;
    const int nRet = CCaptureDatabase::instance()->get_itemInfo(stDirInfo);
    stDirInfo.nTotalSize += static_cast<long long>(stInfo.nImageSize);
    stDirInfo.nCount++;

    if (nRet < 0)
    {
        CCaptureDatabase::instance()->add(stDirInfo);
    }
    else
    {
        CCaptureDatabase::instance()->update(stDirInfo);
    }
    return OK;
}
} // namespace FaceDetectInternal
