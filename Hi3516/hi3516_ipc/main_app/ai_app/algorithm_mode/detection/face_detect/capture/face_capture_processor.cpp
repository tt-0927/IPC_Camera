/**
 * @FilePath     : face_capture_processor.cpp
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2026-09-21
 * @Description  : 人脸抓拍处理器实现——统一检测层独立实现（迁移自 Legacy 并域类型化）。
 */

#include "face_capture_processor.hpp"

#include <algorithm>
#include <ctime>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <unistd.h>

#include "posix_fs.h"

#include "capture_ctrl.h"
#include "capture_database.h"
#include "email_manage.h"
#include "storage_manage.h"
#include "time_utils.h"
#include "face_capture_temp_file.hpp"
#include "video_frame_jpeg_encoder.hpp"
#include "jpeg_encode_service.hpp"

namespace
{
/* 人脸目标框放大倍率，用于目标小图裁剪时保留周边上下文 */
constexpr float FACE_REGION_SCALE_RATIO = 2.0f;
/* 算法未返回眼部关键点时，以人脸框宽度的 70% 估算瞳距 */
constexpr int ESTIMATED_IPD_RATIO_PERCENT = 70;
/* 取优调试开关：板端 touch /opt/cam/bin/testPrint 后生效（绝对路径，进程 CWD 可能在 /opt/cam/bin） */
constexpr const char *FACE_BESTSHOT_DEBUG_FLAG = "/opt/cam/bin/testPrint";

struct FaceCaptureTimeParts_S
{
    std::string strDateCompact;
    std::string strDateDash;
    std::string strTimeCompactMs;
    std::string strTimeColon;
    std::string strDateTimeDash;
};

FaceCaptureTimeParts_S build_face_capture_time_parts(long long llTimestamp)
{
    if (llTimestamp <= 0)
    {
        llTimestamp = TimeUtils_NS::get_currentTimestampMs();
    }

    const std::time_t seconds = static_cast<std::time_t>(llTimestamp / 1000);
    const int millis = static_cast<int>(llTimestamp % 1000);
    struct tm tmValue;
    localtime_r(&seconds, &tmValue);

    FaceCaptureTimeParts_S stParts;
    {
        std::ostringstream oss;
        oss << std::put_time(&tmValue, "%Y%m%d");
        stParts.strDateCompact = oss.str();
    }
    {
        std::ostringstream oss;
        oss << std::put_time(&tmValue, "%Y-%m-%d");
        stParts.strDateDash = oss.str();
    }
    {
        std::ostringstream oss;
        oss << std::put_time(&tmValue, "%H%M%S") << std::setw(3) << std::setfill('0') << millis;
        stParts.strTimeCompactMs = oss.str();
    }
    {
        std::ostringstream oss;
        oss << std::put_time(&tmValue, "%H:%M:%S");
        stParts.strTimeColon = oss.str();
    }
    stParts.strDateTimeDash = stParts.strDateDash + " " + stParts.strTimeColon;
    return stParts;
}

EventTriggerContext_S build_face_capture_event_context(int nChnId, long long llTimestamp, const std::string &strCaptureImagePath)
{
    EventTriggerContext_S stContext;
    stContext.enEventType = Event::Type_E::FACE_CAPTURE;
    stContext.nChnId = nChnId < 0 ? 0 : nChnId;
    stContext.llTimestamp = llTimestamp > 0 ? llTimestamp : TimeUtils_NS::get_currentTimestampMs();
    if (!strCaptureImagePath.empty())
    {
        /* 仅供设备内部上传线程定位图片，MQTT报警正文会过滤该内部路径 */
        stContext.mapAttrs["CaptureImagePath"] = strCaptureImagePath;
    }
    return stContext;
}
}

namespace FaceDetectInternal
{
void CFaceCaptureProcessor::setEnabled(bool bEnable)
{
    m_stAlgoCfg.bEnable = bEnable;
    if (!bEnable)
    {
        /* 窗口本地复位（不收割不出图）；取优桶与 best 缓存由模型 unInit → hub.deinit() 统一清 */
        m_bWindowActive = false;
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

    /* 窗口时长 = 抓拍间隔，只允许 1~10 秒（与前端取值范围一致）；间隔为 1 秒时也走同一套逻辑，不单独处理 */
    m_nWindowIntervalMs = std::min(std::max(stAlgoCfg.stRule.nInterval, 1), 10) * 1000;
    dlog_info("[人脸抓拍] 取优窗口时长 %d 毫秒（抓拍间隔配置 %d 秒，只允许 1~10 秒）", m_nWindowIntervalMs, stAlgoCfg.stRule.nInterval);

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

void CFaceCaptureProcessor::onNewBest(int nTrackId,
                                      const Bestshot_NS::BestShotFeedResult_S &stResult,
                                      const ot_video_frame_info *pFrame,
                                      const ot_rect &stRoi)
{
    (void) stResult;
    /* 新 best 立即硬编码缓存小图（约 5ms，帧生命周期不跨帧）；无有效裁剪框（过小目标）跳过编码 */
    if (pFrame == nullptr || stRoi.width < 32 || stRoi.height < 32)
    {
        return;
    }
    std::vector<ot_rect> vecRects{ stRoi };
    std::vector<std::vector<uint8_t>> vecJpegs;
    if (AiAppCommon::CJpegEncodeService::instance()->encodeCrops(*pFrame, vecRects, vecJpegs) == OK && !vecJpegs.empty())
    {
        m_mapBestJpegs[nTrackId] = std::move(vecJpegs.front());
    }
}

void CFaceCaptureProcessor::onTrackEnded(int nTrackId)
{
    /* 仅 Worker 回调线程调用，无锁 */
    m_mapBestJpegs.erase(nTrackId);
}

void CFaceCaptureProcessor::onAllCleared()
{
    m_mapBestJpegs.clear();
}

void CFaceCaptureProcessor::process(FaceChain_NS::FaceFrameContext_S &stCtx,
                                    const std::vector<FaceChain_NS::FaceTarget_S> &vTargets,
                                    CBestShotHub &stHub,
                                    std::vector<Common::RectInfo_S> &vstRectInfo,
                                    std::vector<std::string> &vecImageFile)
{
    if (!m_stAlgoCfg.bEnable)
    {
        return;
    }

    /* 当前帧满足抓拍规则的人脸目标（收割帧当帧目标，离场目标见 best 缓存补充） */
    std::vector<FaceChain_NS::FaceTarget_S> vecFaceCaptureTargets;
    collectTargets(vTargets, vstRectInfo, &vecFaceCaptureTargets);

    /*
     * 开窗（含旧图清理）必须放在评分之前（板端实测教训：曾出现开窗帧的人脸
     * 丢图）。清理放在评分之后，会把本帧刚产生的最佳图当旧图删掉。帧内顺序
     * 固定：开窗（清理旧图）→ 逐个人脸评分 → 窗口到时统一出图。
     */
    if (!m_bWindowActive)
    {
        if (!vecFaceCaptureTargets.empty())
        {
            /*
             * 抓拍关闭再打开后会留下上一轮的旧图（正常出图后这里应为空；
             * 关闭抓拍时窗口被直接关闭，旧图没人清理）。旧图不清会混进
             * 新窗口的出图结果，开窗前统一丢弃。
             */
            if (!m_mapBestJpegs.empty())
            {
                std::vector<int> vStaleTrackIds;
                vStaleTrackIds.reserve(m_mapBestJpegs.size());
                for (const auto &stBestPair : m_mapBestJpegs)
                {
                    vStaleTrackIds.emplace_back(stBestPair.first);
                }
                for (const int nTrackId : vStaleTrackIds)
                {
                    stHub.endTarget(nTrackId);
                }
                dlog_warn("[人脸抓拍] 抓拍关闭期间遗留 %zu 张旧的最佳人脸图，新窗口开始前已丢弃", vStaleTrackIds.size());
            }
            /* 开窗：从本帧起给画面里的人脸逐张评分，找各自的最佳图 */
            m_bWindowActive = true;
            m_tpWindowStart = std::chrono::steady_clock::now();
            dlog_info("[人脸抓拍] 取优窗口开始：时长 %d 毫秒，画面中 %zu 张人脸", m_nWindowIntervalMs, vecFaceCaptureTargets.size());
        }
        else
        {
            /* 不在窗口内且画面无人脸：某张脸超过 5 秒没有再出现，就清掉它保存的最佳图 */
            stHub.expireAll(TimeUtils_NS::get_currentTimestampMs());
        }
    }

    /*
     * 画面里每张符合规则的人脸（灵敏度、瞳距、区域都过关）单独记录自己的
     * 最佳图，窗口结束时一起出图；OSD 框显示全部符合规则的人脸。窗口结束
     * 帧的人脸也先评分再出图，保证最后一张参与评选。
     */
    ot_video_frame_info *pFrame = stCtx.pSrcFrameInfo;
    if (pFrame != nullptr && !vecFaceCaptureTargets.empty())
    {
        /*
         * 本帧全部裁剪框先统一算好：亮度统计、评分与出图用同一个框。
         * 再用一次 VGS 硬件任务批量统计每张人脸的平均亮度——过暗/过曝的
         * 帧不再参与评选，夜间噪声让清晰度虚高的帧按亮度打折（两项判定
         * 都在共享取优模块内，这里只负责把亮度算出来传进去）。
         */
        const int nFrameW = static_cast<int>(pFrame->video_frame.width);
        const int nFrameH = static_cast<int>(pFrame->video_frame.height);
        std::vector<ot_rect> vstCrops;
        vstCrops.reserve(vecFaceCaptureTargets.size());
        for (const auto &stTarget : vecFaceCaptureTargets)
        {
            Common::RectInfo_S rect = stTarget.stRect;
            ot_rect stCrop;
            if (!alignCropRect(rect, m_nWidth, m_nHeight, nFrameW, nFrameH, stCrop))
            {
                stCrop.x = 0;
                stCrop.y = 0;
                stCrop.width = 0;
                stCrop.height = 0;
            }
            vstCrops.push_back(stCrop);
        }
        std::vector<float> vfBrightness;
        queryFaceBrightness(pFrame, vstCrops, vfBrightness);
        for (size_t i = 0; i < vecFaceCaptureTargets.size(); ++i)
        {
            /* 评分并更新该人脸的最佳图：新图要明显更好才替换，替换时立即编码存图 */
            feedBestShot(vecFaceCaptureTargets[i], stCtx, stHub, vstCrops[i], vfBrightness[i]);
        }
    }

    /*
     * 窗口到时统一出图并上报。窗口内偶尔几帧没人脸（被遮挡）不中断窗口；
     * 出图后画面里仍有人脸就立即开始下一个窗口，出图节奏正好等于抓拍间隔。
     * 计时用开机时钟，设备校时跳变不影响窗口时长。
     */
    if (m_bWindowActive && std::chrono::steady_clock::now() - m_tpWindowStart >= std::chrono::milliseconds(m_nWindowIntervalMs))
    {
        /* 出图结束后窗口关闭、保存的最佳图全部清空 */
        harvestWindow(vecFaceCaptureTargets, stCtx, stHub, vecImageFile);
        if (!vecFaceCaptureTargets.empty())
        {
            /* 出图时画面里仍有人脸：本帧已评完分，立即开始下一个窗口，从下一帧继续评选 */
            m_bWindowActive = true;
            m_tpWindowStart = std::chrono::steady_clock::now();
            dlog_info("[人脸抓拍] 取优窗口开始：时长 %d 毫秒，画面中 %zu 张人脸", m_nWindowIntervalMs, vecFaceCaptureTargets.size());
        }
    }
}

void CFaceCaptureProcessor::onTracksEnded(const std::vector<int> &vTrackIds, CBestShotHub &stHub)
{
    if (vTrackIds.empty())
    {
        return;
    }
    if (m_bWindowActive)
    {
        /*
         * 窗口内离开：最佳图留到窗口结束一起出图（提前走的人也能出图）。
         * 出图张数在收割时按实际落盘数量统计（轨迹结束、置信度跌破灵敏度
         * 两种离开方式都算"中途离开"），这里不做计数。
         */
        return;
    }
    /* 不在窗口内：离开的人脸立即清掉保存的最佳图 */
    for (const int nTrackId : vTrackIds)
    {
        stHub.endTarget(nTrackId);
    }
}

void CFaceCaptureProcessor::process(SFaceProcessContext &stContext, std::vector<std::string> &vecImageFile)
{
    /*
     * Legacy 兼容入口（名单库入库临时实例路径，抓拍恒未使能即早退）：
     * 使能的常驻人脸链路由统一检测层编排并传入取优枢纽；本重载仅保 Legacy
     * 调用方编译通过。若意外被使能，转域类型走新实现——本地枢纽未装配评分
     * 后端，取优自动旁路（feed 恒无 best，回退当帧编码出图），行为可预期。
     */
    if (!m_stAlgoCfg.bEnable)
    {
        return;
    }
    dlog_warn("[人脸抓拍] Legacy 入口意外使能，按无取优模式处理");

    CBestShotHub stLocalHub;
    std::vector<FaceChain_NS::FaceTarget_S> vTargets;
    vTargets.reserve(stContext.vPointDatas.size());
    for (const auto &stPointData : stContext.vPointDatas)
    {
        FaceChain_NS::FaceTarget_S stTarget;
        stTarget.stRect.nX1 = stPointData.stBoxs.nX1;
        stTarget.stRect.nY1 = stPointData.stBoxs.nY1;
        stTarget.stRect.nX2 = stPointData.stBoxs.nX2;
        stTarget.stRect.nY2 = stPointData.stBoxs.nY2;
        stTarget.fConfidence = stPointData.fConfidence;
        stTarget.nTrackId = -1;
        vTargets.emplace_back(stTarget);
    }
    FaceChain_NS::FaceFrameContext_S stCtx;
    stCtx.pSrcFrameInfo = stContext.pFrameInfo;
    stCtx.nChnId = stContext.nChnId;
    stCtx.llTimestamp = stContext.llTimestamp;
    stCtx.nDetectWidth = stContext.nWidth;
    stCtx.nDetectHeight = stContext.nHeight;
    stCtx.stImageCache.strTargetImagePath = stContext.stImageCache.strTargetImagePath;
    stCtx.stImageCache.strPanoramaImagePath = stContext.stImageCache.strPanoramaImagePath;

    process(stCtx, vTargets, stLocalHub, stContext.vstRectInfo, vecImageFile);

    stContext.stImageCache.strTargetImagePath = stCtx.stImageCache.strTargetImagePath;
    stContext.stImageCache.strPanoramaImagePath = stCtx.stImageCache.strPanoramaImagePath;
}

bool CFaceCaptureProcessor::collectTargets(const std::vector<FaceChain_NS::FaceTarget_S> &vTargets,
                                           std::vector<Common::RectInfo_S> &vstRectInfo,
                                           std::vector<FaceChain_NS::FaceTarget_S> *pvecTargets)
{
    if (pvecTargets != nullptr)
    {
        pvecTargets->clear();
    }

    if (vTargets.empty())
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

    for (const auto &stTarget : vTargets)
    {
        /* 区域/屏蔽判定复用既有 is_in_region（按 Box_S 坐标），域框转换后判定 */
        Inference_NS::Box_S stBox{};
        stBox.nX1 = stTarget.stRect.nX1;
        stBox.nY1 = stTarget.stRect.nY1;
        stBox.nX2 = stTarget.stRect.nX2;
        stBox.nY2 = stTarget.stRect.nY2;
        if (bHasShieldedRegion)
        {
            /* 当前目标是否命中任一屏蔽区域 */
            bool bIsResultInShielded = false;
            for (auto &shieldedRegion : rule.vstShieldedRegion)
            {
                if (shieldedRegion.IsValid() && is_in_region(shieldedRegion, stBox))
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

        if (bHasDetectRegion && !is_in_region(rule.stRegion, stBox))
        {
            continue;
        }

        if (stTarget.fConfidence < fSensitivityThreshold)
        {
            continue;
        }

        /* 算法未返回眼部关键点，以人脸框宽度的 70% 估算瞳距 */
        const int nFaceWidth = std::max(0, stTarget.stRect.nX2 - stTarget.stRect.nX1);
        const int nEstimatedIpd = nFaceWidth * ESTIMATED_IPD_RATIO_PERCENT / 100;
        if (nEstimatedIpd < nMinIpd)
        {
            continue;
        }

        bIsAlarm = true;
        dlog_debug("[人脸抓拍] 灵敏度[%.3f] > 阈值[%.3f] 瞳距 [%d] > [%d]",
                   stTarget.fConfidence,
                   fSensitivityThreshold,
                   nEstimatedIpd,
                   nMinIpd);
        vstRectInfo.emplace_back(stTarget.stRect);
        if (pvecTargets != nullptr)
        {
            /* 当前满足抓拍条件的目标（含跟踪 id），供联动与取优使用 */
            pvecTargets->emplace_back(stTarget);
        }
    }

    return bIsAlarm;
}

int CFaceCaptureProcessor::saveFaceImage(const std::vector<FaceChain_NS::FaceTarget_S> &vTargets,
                                         ot_video_frame_info *pSrcFrameInfo,
                                         int nChnId,
                                         std::vector<std::string> &vecImageFile,
                                         long long llTimestamp,
                                         bool bSaveDatabase,
                                         CBestShotHub &stHub,
                                         size_t &unInFrameSaved,
                                         size_t &unDepartedSaved)
{
    /* 出图统计口径：按实际取得 JPEG 的张数计（框无效/编码失败的目标不计），
       画面内 + 中途离开 = 最终落盘张数，供收割日志 */
    unInFrameSaved = 0;
    unDepartedSaved = 0;
    if (pSrcFrameInfo == nullptr)
    {
        dlog_error("人脸抓拍视频帧为空");
        return ERR_PTR_NULL;
    }

    if (vTargets.empty() && m_mapBestJpegs.empty())
    {
        /* 窗口内没有任何人脸取得最佳图（人脸太小或评分始终失败），本次不出图 */
        dlog_info("[人脸抓拍] 取优窗口结束：没有取得可用的人脸图，本次不出图");
        return ERR_PARAM_NULL;
    }

    /*
     * 多目标出图（按目标顺序 _1.._N 落盘/入库）：画面内的人脸优先用窗口里
     * 评出的最佳图（已是编码好的成品），没有最佳图的用当前画面裁剪编码；
     * 处理完画面内目标后，剩下的最佳图属于窗口中途离开的人脸，序号接续出图。
     */
    std::vector<std::vector<uint8_t>> vecJpegs(vTargets.size());
    std::vector<ot_rect> vecCropRects;
    std::vector<size_t> vecEncodeIdx;
    for (size_t i = 0; i < vTargets.size(); i++)
    {
        const int nTrackId = vTargets[i].nTrackId;
        auto itJpeg = m_mapBestJpegs.find(nTrackId);
        if (itJpeg != m_mapBestJpegs.end() && !itJpeg->second.empty())
        {
            if (!access(FACE_BESTSHOT_DEBUG_FLAG, F_OK))
            {
                dlog_info("[人脸取优] 目标%d 出图用窗口内最清晰的一帧（%zu 字节）", itJpeg->first, itJpeg->second.size());
            }
            vecJpegs[i] = std::move(itJpeg->second);
            unInFrameSaved++;
            /* 图已取走，清掉该目标的评选记录，下个窗口从零开始评 */
            stHub.endTarget(itJpeg->first);
            continue;
        }

        /* 直出裁剪框（对齐失败的目标留空图，落盘时跳过该序号） */
        Common::RectInfo_S rect = vTargets[i].stRect;
        ot_rect stCrop;
        if (alignCropRect(rect,
                          m_nWidth,
                          m_nHeight,
                          static_cast<int>(pSrcFrameInfo->video_frame.width),
                          static_cast<int>(pSrcFrameInfo->video_frame.height),
                          stCrop))
        {
            vecCropRects.emplace_back(stCrop);
            vecEncodeIdx.emplace_back(i);
        }
    }

    if (!vecEncodeIdx.empty())
    {
        std::vector<std::vector<uint8_t>> vecEncoded;
        if (AiAppCommon::CJpegEncodeService::instance()->encodeCrops(*pSrcFrameInfo, vecCropRects, vecEncoded) != OK)
        {
            dlog_error("[人脸抓拍] 批量抠图硬编码失败，目标数[%zu]", vecCropRects.size());
            return ERR;
        }
        for (size_t j = 0; j < vecEncodeIdx.size() && j < vecEncoded.size(); j++)
        {
            vecJpegs[vecEncodeIdx[j]] = std::move(vecEncoded[j]);
            unInFrameSaved++;
        }
    }

    /*
     * 窗口中途离开的人脸：上面处理完画面内目标后，剩下的最佳图就是他们的，
     * 序号接在画面内目标之后出图。endTarget 会顺带删掉 map 里的记录，
     * 所以先把编号收集起来再逐个清理，避免边遍历边删除出错。
     */
    std::vector<int> vDepartedTrackIds;
    vDepartedTrackIds.reserve(m_mapBestJpegs.size());
    for (auto &stBestPair : m_mapBestJpegs)
    {
        if (!stBestPair.second.empty())
        {
            if (!access(FACE_BESTSHOT_DEBUG_FLAG, F_OK))
            {
                dlog_info("[人脸取优] 目标%d 已离开画面，用其窗口内最清晰的一帧出图（%zu 字节）",
                          stBestPair.first,
                          stBestPair.second.size());
            }
            vecJpegs.emplace_back(std::move(stBestPair.second));
            unDepartedSaved++;
        }
        vDepartedTrackIds.emplace_back(stBestPair.first);
    }
    for (const int nTrackId : vDepartedTrackIds)
    {
        stHub.endTarget(nTrackId);
    }
    return write_face_capture_files(vecJpegs, nChnId, vecImageFile, llTimestamp, bSaveDatabase);
}

bool CFaceCaptureProcessor::alignCropRect(Common::RectInfo_S &rect,
                                          int nDetectWidth,
                                          int nDetectHeight,
                                          int nFrameWidth,
                                          int nFrameHeight,
                                          ot_rect &stCrop)
{
    /* 2 倍扩框保留上下文 + 检测空间 -> 源帧拉伸逆变换（从原始 AI 帧高清裁剪） */
    convert_region_ratio(rect, FACE_REGION_SCALE_RATIO, nDetectWidth, nDetectHeight);
    rect.ConvertResolution(nDetectWidth, nDetectHeight, nFrameWidth, nFrameHeight);
    /* batch_crop 硬编码约束（CV610）：x 8 / y 2 像素对齐、宽高 >= 32x32、不越界，
     * 右下角向外对齐避免扩框被重新缩小。 */
    rect.nX1 = ALIGN_BACK(rect.nX1, 8);
    rect.nY1 = ALIGN_BACK(rect.nY1, 2);
    rect.nX2 = std::min(ALIGN_UP(rect.nX2, 8), nFrameWidth);
    rect.nY2 = std::min(ALIGN_UP(rect.nY2, 2), nFrameHeight);
    if (rect.nX2 - rect.nX1 < 32 || rect.nY2 - rect.nY1 < 32)
    {
        return false;
    }
    stCrop.x = rect.nX1;
    stCrop.y = rect.nY1;
    stCrop.width = static_cast<td_u32>(rect.nX2 - rect.nX1);
    stCrop.height = static_cast<td_u32>(rect.nY2 - rect.nY1);
    return true;
}

void CFaceCaptureProcessor::feedBestShot(const FaceChain_NS::FaceTarget_S &stTarget,
                                         FaceChain_NS::FaceFrameContext_S &stCtx,
                                         CBestShotHub &stHub,
                                         const ot_rect &stCrop,
                                         float fBrightness)
{
    ot_video_frame_info *pFrame = stCtx.pSrcFrameInfo;
    if (pFrame == nullptr)
    {
        return;
    }

    const Bestshot_NS::BestShotFeedResult_S stResult = stHub.feed(stTarget.nTrackId,
                                                                  pFrame,
                                                                  stCrop,
                                                                  stTarget.fConfidence,
                                                                  stCtx.llTimestamp,
                                                                  fBrightness);
    /* 逐帧评分日志（板端 touch /opt/cam/bin/testPrint 开启，调选取效用） */
    if (!access(FACE_BESTSHOT_DEBUG_FLAG, F_OK))
    {
        if (stResult.bGated)
        {
            dlog_info("[人脸取优] 目标%d 亮度%.0f 过暗或过曝，本帧不参与评选（当前最佳%.3f）",
                      stTarget.nTrackId,
                      fBrightness,
                      stResult.fBestScore);
        }
        else
        {
            dlog_info(
                "[人脸取优] 目标%d 清晰度%.3f 置信度%.2f 面积%llu 亮度%.0f 综合分%.3f（当前最佳%.3f）%s",
                stTarget.nTrackId,
                stResult.fSharpness,
                stTarget.fConfidence,
                static_cast<unsigned long long>(static_cast<uint64_t>(stCrop.width) * stCrop.height),
                fBrightness,
                stResult.fTotalScore,
                stResult.fBestScore,
                stResult.bNewBest ? (stResult.bGateBypassed ? " === 新的最佳（首帧保底，亮度超出范围）===" : " === 新的最佳 ===") : "");
        }
    }
    /* 评出新的最佳图时，编码存图由 onNewBest 回调完成 */
}

void CFaceCaptureProcessor::queryFaceBrightness(ot_video_frame_info *pFrame,
                                                const std::vector<ot_rect> &vstCrops,
                                                std::vector<float> &vfBrightness)
{
    vfBrightness.assign(vstCrops.size(), -1.0f);
    if (pFrame == nullptr)
    {
        return;
    }

    /* 只统计有效裁剪框：过小的人脸没有裁剪框（全零），保持 -1 按未知亮度评选 */
    std::vector<ot_rect> vstValidRects;
    std::vector<size_t> vValidIndex;
    vstValidRects.reserve(vstCrops.size());
    vValidIndex.reserve(vstCrops.size());
    for (size_t i = 0; i < vstCrops.size(); ++i)
    {
        if (vstCrops[i].width > 0 && vstCrops[i].height > 0)
        {
            vstValidRects.push_back(vstCrops[i]);
            vValidIndex.push_back(i);
        }
    }
    if (vstValidRects.empty())
    {
        return;
    }

    std::vector<td_u64> vecLumaSums(vstValidRects.size(), 0);
    if (TD_SUCCESS != mppVgs_luma(pFrame, vstValidRects.data(), static_cast<td_u32>(vstValidRects.size()), vecLumaSums.data()))
    {
        /* 统计失败按未知亮度评选，抓拍链路不断（失败的具体原因平台层日志已给出） */
        dlog_warn("[人脸取优] 人脸亮度统计失败，本帧 %zu 张人脸按未知亮度评选", vstValidRects.size());
        return;
    }
    for (size_t i = 0; i < vstValidRects.size(); ++i)
    {
        const uint64_t unPixels = static_cast<uint64_t>(vstValidRects[i].width) * vstValidRects[i].height;
        if (unPixels > 0)
        {
            vfBrightness[vValidIndex[i]] = static_cast<float>(vecLumaSums[i]) / static_cast<float>(unPixels);
        }
    }
}

int CFaceCaptureProcessor::write_face_capture_files(const std::vector<std::vector<uint8_t>> &vecJpegs,
                                                    int nChnId,
                                                    std::vector<std::string> &vecImageFile,
                                                    long long llTimestamp,
                                                    bool bSaveDatabase)
{
    /* 获取按日期分类的存储路径 */
    std::string strStoragePath = CCaptureCtrl::instance()->get_date_storage_path();
    if (!CCaptureCtrl::instance()->ensure_directory_exists(strStoragePath))
    {
        dlog_error("[人脸抓拍]确保目录存在失败");
        return ERR;
    }
    for (size_t i = 0; i < vecJpegs.size(); i++)
    {
        /* 当前抓图日期和时间由同一时间戳生成，保证文件名、数据库、MQTT和HTTP表单能互相对应 */
        const FaceCaptureTimeParts_S stTimeParts = build_face_capture_time_parts(llTimestamp);
        std::string strFilename = strStoragePath + "/" + stTimeParts.strDateCompact + "_" + stTimeParts.strTimeCompactMs + "_" +
                                  std::to_string(static_cast<int>(Event::Type_E::FACE_CAPTURE)) + "_" +
                                  std::to_string(int(Alarm::LinkageType::UPLOAD_TARGET_IMAGE)) + "_" + std::to_string(i + 1) + ".jpg";
        std::ofstream file(strFilename, std::ios::binary | std::ios::trunc);
        if (!file.is_open() || vecJpegs[i].empty())
        {
            dlog_error("[人脸抓拍]创建目标图文件失败[%s]", strFilename.c_str());
            continue;
        }
        file.write(reinterpret_cast<const char *>(vecJpegs[i].data()), static_cast<std::streamsize>(vecJpegs[i].size()));
        const bool bWriteOk = file.good();
        file.flush();
        file.close();
        if (!bWriteOk)
        {
            continue;
        }
        vecImageFile.emplace_back(strFilename);
        if (bSaveDatabase)
        {
            saveToDatabase(strFilename, stTimeParts.strDateDash, stTimeParts.strTimeColon, nChnId);
        }
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

void CFaceCaptureProcessor::harvestWindow(const std::vector<FaceChain_NS::FaceTarget_S> &vecTargets,
                                          FaceChain_NS::FaceFrameContext_S &stContext,
                                          CBestShotHub &stHub,
                                          std::vector<std::string> &vecImageFile)
{
    const FaceCaptureLinkageOptions_S stOptions = buildLinkageOptions();
    const bool bSdCardNormal = SD_CARD_STATUS_E::NORMAL == CStorageManage::instance()->get_SdCardStatus();
    const int nChnId = stContext.nChnId;
    const long long llTimestamp = stContext.llTimestamp;
    ot_video_frame_info *pFrameInfo = stContext.pSrcFrameInfo;

    const long long llEventTimestamp = llTimestamp > 0 ? llTimestamp : TimeUtils_NS::get_currentTimestampMs();
    std::string strUploadImagePath;

    /*
     * 全景图：收割帧当帧 VENC 硬编码（与小图同帧同质量，约 10ms）。
     * 替代旧路径"置通用抓图标志等 VENC chn2 下一张产出"（0~1s 漂移 +
     * get_face_capture_file 最多 3s 同步等待，等待期间持有源帧）。
     * 已有同帧缓存时直接复用；编码失败留空，后续平台/邮件路径回退目标图。
     */
    if (stOptions.bPanoramaImage)
    {
        if (stContext.stImageCache.strPanoramaImagePath.empty() && bSdCardNormal && pFrameInfo != nullptr)
        {
            std::vector<uint8_t> vecJpeg;
            if (AiAppCommon::CJpegEncodeService::instance()->encodePanorama(*pFrameInfo, vecJpeg) == OK)
            {
                const std::string strStoragePath = CCaptureCtrl::instance()->get_date_storage_path();
                if (CCaptureCtrl::instance()->ensure_directory_exists(strStoragePath))
                {
                    const FaceCaptureTimeParts_S stTimeParts = build_face_capture_time_parts(llEventTimestamp);
                    const std::string strFilename = strStoragePath + "/" + stTimeParts.strDateCompact + "_" + stTimeParts.strTimeCompactMs +
                                                    "_" + std::to_string(static_cast<int>(Event::Type_E::FACE_CAPTURE)) + "_" +
                                                    std::to_string(int(Alarm::LinkageType::UPLOAD_PANORAMIC_IMAGE)) + "_" +
                                                    std::to_string(1) + ".jpg";
                    std::ofstream file(strFilename, std::ios::binary | std::ios::trunc);
                    if (file.is_open() && !vecJpeg.empty())
                    {
                        file.write(reinterpret_cast<const char *>(vecJpeg.data()), static_cast<std::streamsize>(vecJpeg.size()));
                        const bool bWriteOk = file.good();
                        file.flush();
                        file.close();
                        if (bWriteOk)
                        {
                            stContext.stImageCache.strPanoramaImagePath = strFilename;
                            saveToDatabase(strFilename, stTimeParts.strDateDash, stTimeParts.strTimeColon, nChnId);
                        }
                    }
                }
            }
            if (stContext.stImageCache.strPanoramaImagePath.empty())
            {
                dlog_warn("人脸抓拍全景图未生成");
            }
        }
        if (!stContext.stImageCache.strPanoramaImagePath.empty())
        {
            strUploadImagePath = stContext.stImageCache.strPanoramaImagePath;
        }
    }

    /*
     * 目标图、平台上传及无全景图时的邮件附件共用同一文件（多目标全部落盘，
     * 上传/邮件取第一张 _1）。两项功能同时开启时复用缓存；任一功能单独
     * 开启时缓存为空并自行生成。
     */
    std::vector<std::string> vecSavedImages;
    /* 出图张数按实际落盘统计（画面内 + 中途离开，两种离开方式都覆盖） */
    size_t unInFrameSaved = 0;
    size_t unDepartedSaved = 0;
    const bool bNeedTargetImage = stOptions.bTargetImage || (stOptions.bUploadSdCard && strUploadImagePath.empty()) ||
                                  (stOptions.bEmail && strUploadImagePath.empty());
    if (bNeedTargetImage)
    {
        if (stContext.stImageCache.strTargetImagePath.empty())
        {
            /* 全部规则通过目标逐个出图（每目标优先取优 best 缓存），按序号 _1.._N 落盘 */
            saveFaceImage(vecTargets,
                          pFrameInfo,
                          nChnId,
                          vecSavedImages,
                          llEventTimestamp,
                          bSdCardNormal,
                          stHub,
                          unInFrameSaved,
                          unDepartedSaved);
            if (!vecSavedImages.empty())
            {
                stContext.stImageCache.strTargetImagePath = vecSavedImages.front();
            }
        }
        if (!stContext.stImageCache.strTargetImagePath.empty())
        {
            /* 目标图比全景图更适合作为平台人脸事件图片。 */
            strUploadImagePath = stContext.stImageCache.strTargetImagePath;
        }
    }

    /*
     * SDK 推送：仅收割帧在场目标逐张推送（离场 track 无当帧框，SDK 面不含，
     * 落盘/入库仍含离场 best）；传统联动已经生成文件时按序号直接读取复用；
     * 没有配置传统图片联动时再回退到 SDK 自己编码，保证单独开启 SDK 推送
     * 仍然有图片。推送器消费 Legacy 目标类型（域目标在此边界转换）。
     * 窗口收割即完整事件对：先复位 SDK 事件活跃标记，本次收割重新推送全景首图。
     */
    m_sdkEventPublisher.resetEvent(nChnId);
    std::vector<FaceCaptureTarget_S> vecSdkTargets;
    vecSdkTargets.reserve(vecTargets.size());
    for (const auto &stTarget : vecTargets)
    {
        FaceCaptureTarget_S stSdkTarget;
        stSdkTarget.stRect = stTarget.stRect;
        stSdkTarget.fConfidence = stTarget.fConfidence;
        vecSdkTargets.emplace_back(stSdkTarget);
    }
    m_sdkEventPublisher.publish(
        vecSdkTargets,
        pFrameInfo,
        nChnId,
        [this, &stContext](ot_video_frame_info *pFrame, std::vector<unsigned char> &vecJpeg)
        {
            if (!stContext.stImageCache.strPanoramaImagePath.empty())
            {
                return loadJpegFile(stContext.stImageCache.strPanoramaImagePath, vecJpeg);
            }
            return buildSdkPanoramaImage(pFrame, vecJpeg);
        },
        [this, &stContext, vecSavedImages](const Common::RectInfo_S &stRectInfo,
                                           ot_video_frame_info *pFrame,
                                           size_t nIndex,
                                           std::vector<unsigned char> &vecJpeg)
        {
            /* 传统联动已生成多目标文件时按序号复用（nIndex 从 1 起，与落盘序号一致） */
            if (nIndex >= 1 && nIndex <= vecSavedImages.size() && !vecSavedImages[nIndex - 1].empty())
            {
                return loadJpegFile(vecSavedImages[nIndex - 1], vecJpeg);
            }
            if (!stContext.stImageCache.strTargetImagePath.empty())
            {
                return loadJpegFile(stContext.stImageCache.strTargetImagePath, vecJpeg);
            }
            return buildSdkTargetImage(stRectInfo, pFrame, nIndex, vecJpeg);
        });

    if (stOptions.bEmail)
    {
        /* 邮件只携带一张图片，优先全景图，否则复用目标图。 */
        vecImageFile.clear();
        const std::string &strEmailImagePath = !stContext.stImageCache.strPanoramaImagePath.empty()
                                                   ? stContext.stImageCache.strPanoramaImagePath
                                                   : stContext.stImageCache.strTargetImagePath;
        if (!strEmailImagePath.empty())
        {
            vecImageFile.emplace_back(strEmailImagePath);
        }
        Network::EmailEventInfo_S stEmailInfo;
        stEmailInfo.strSubject = "人脸抓拍";
        const FaceCaptureTimeParts_S stTimeParts = build_face_capture_time_parts(llEventTimestamp);
        std::ostringstream oss;
        oss << "事件类型: " << stEmailInfo.strSubject << "\n"
            << "日期: " << stTimeParts.strDateDash << "\n"
            << "时间: " << stTimeParts.strTimeColon;
        stEmailInfo.strMessage = oss.str();
        stEmailInfo.vecImageFile = vecImageFile;
        CEmailManage::instance()->HandleEmail(stEmailInfo);
    }
    vecImageFile.clear();

    /*
     * 报警的"开始"和"结束"都在出图这一刻发出（和旧版一样是一次性事件）：
     * 开始时带上第一张图的路径，上传线程拿到路径就能直接传图，不用等图写进
     * 数据库；报警状态机的冷却时间设为 0，抓拍间隔最短 1 秒时第二条报警
     * 不会被冷却时间挡住。
     */
    m_alarmStateMachine.handleAlarmState(true, build_face_capture_event_context(nChnId, llEventTimestamp, strUploadImagePath));
    m_alarmStateMachine.endAlarmImmediately(build_face_capture_event_context(nChnId, llEventTimestamp, ""));

    /* 收尾：清空全部评选记录和保存的最佳图，窗口关闭；画面里若仍有人脸，process 会接着开下一个窗口 */
    stHub.clearAll();
    const long long llWindowElapsedMs = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() -
                                                                                              m_tpWindowStart)
                                            .count();
    m_bWindowActive = false;
    dlog_info("[人脸抓拍] 取优窗口结束：历时 %lld 毫秒，出图 %zu 张（画面内 %zu 张，窗口中途离开 %zu 张）",
              llWindowElapsedMs,
              vecSavedImages.size(),
              unInFrameSaved,
              unDepartedSaved);
}

bool CFaceCaptureProcessor::buildSdkPanoramaImage(ot_video_frame_info *pFrameInfo, std::vector<unsigned char> &vecJpeg)
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
    /* 检测空间(640×640) → 裁剪源帧(VPSS AI 通道原始帧) 拉伸逆变换，从原始 AI 帧高清裁剪 */
    stFaceRect.ConvertResolution(m_nWidth,
                                 m_nHeight,
                                 static_cast<int>(pSrcFrameInfo->video_frame.width),
                                 static_cast<int>(pSrcFrameInfo->video_frame.height));
    stFaceRect.nX1 = ALIGN_BACK(stFaceRect.nX1, 16);
    stFaceRect.nY1 = ALIGN_BACK(stFaceRect.nY1, 4);
    /* 换算放大后右下角可能越出源帧，clamp 到源帧宽高防止 VGS 越界 */
    stFaceRect.nX2 = std::min(ALIGN_BACK(stFaceRect.nX2, 16), static_cast<int>(pSrcFrameInfo->video_frame.width));
    stFaceRect.nY2 = std::min(ALIGN_BACK(stFaceRect.nY2, 4), static_cast<int>(pSrcFrameInfo->video_frame.height));

    if (stFaceRect.nX2 <= stFaceRect.nX1 || stFaceRect.nY2 <= stFaceRect.nY1)
    {
        dlog_warn("人脸抓拍 SDK 图片编码跳过，目标框无效 [%d,%d,%d,%d]", stFaceRect.nX1, stFaceRect.nY1, stFaceRect.nX2, stFaceRect.nY2);
        return ERR;
    }

    /* 裁剪后目标小图宽高 */
    const unsigned int unDstWidth = stFaceRect.nX2 - stFaceRect.nX1;
    const unsigned int unDstHeight = stFaceRect.nY2 - stFaceRect.nY1;
    /* 裁剪后的视频帧，编码完成后必须销毁 */
    ot_video_frame_info stDstFrameInfo;
    if (TD_SUCCESS != mppVgs_create_video_frame_info(unDstWidth, unDstHeight, OT_PIXEL_FORMAT_YVU_SEMIPLANAR_420, &stDstFrameInfo))
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
    return "/tmp/face_capture_sdk_" + strImageType + "_" + TimeUtils_NS::get_currentTimeMs() + "_" + std::to_string(nIndex) + ".jpg";
}

int CFaceCaptureProcessor::saveToDatabase(const std::string &strFilename,
                                          const std::string &strCurrentDate,
                                          const std::string &strCurrentTime,
                                          int nChnId)
{
    /* C++11 无 std::filesystem，用 PosixFs_NS::file_size（stat 实现，失败返回 -1 不抛异常）；
     * 拔卡或文件消失时只结束本次保存 */
    const int64_t llImageSize = PosixFs_NS::file_size(strFilename);
    if (llImageSize <= 0)
    {
        dlog_error("人脸图片无效: %s, size=%lld, error=%s",
                   strFilename.c_str(),
                   static_cast<long long>(llImageSize),
                   llImageSize < 0 ? "stat failed" : "empty file");
        return ERR;
    }

    Event::Type_E nEventType = Event::Type_E::FACE_CAPTURE;
    if (strFilename.find("compare") != std::string::npos)
    {
        nEventType = Event::Type_E::FACE_COMPARE;
    }
    using namespace Db;
    Capture_NS::CaptureInfo_S stInfo;
    stInfo.nChnId = nChnId < 0 ? 0 : nChnId;
    stInfo.strImagePath = strFilename;
    stInfo.nImageSize = llImageSize;
    stInfo.strStartTime = strCurrentDate + " " + strCurrentTime;
    stInfo.strEndTime = stInfo.strStartTime;
    stInfo.enType = nEventType;
    if (CCaptureDatabase::instance()->add(stInfo) < 0)
    {
        dlog_error("写入人脸图片记录失败: %s", strFilename.c_str());
        return ERR;
    }

    /* 更新图片数量、总大小至数据库表 */
    Capture_NS::CaptureDirInfo_S stDirInfo;
    stDirInfo.nChnId = nChnId < 0 ? 0 : nChnId;
    const int nRet = CCaptureDatabase::instance()->get_itemInfo(stDirInfo);
    stDirInfo.nTotalSize += static_cast<long long>(stInfo.nImageSize);
    stDirInfo.nCount++;

    if (nRet < 0)
    {
        if (CCaptureDatabase::instance()->add(stDirInfo) < 0)
        {
            dlog_error("新增人脸图片目录统计失败: channel=%d", stDirInfo.nChnId);
            return ERR;
        }
    }
    else
    {
        if (CCaptureDatabase::instance()->update(stDirInfo) < 0)
        {
            dlog_error("更新人脸图片目录统计失败: channel=%d", stDirInfo.nChnId);
            return ERR;
        }
    }
    return OK;
}
} // namespace FaceDetectInternal
