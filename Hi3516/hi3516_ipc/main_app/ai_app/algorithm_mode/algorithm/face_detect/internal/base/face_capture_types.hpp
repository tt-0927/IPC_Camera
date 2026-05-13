/**
 * @FilePath     : face_capture_types.hpp
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2026-04-28 15:07:08
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-04-28 16:24:18
 * @Description  : 人脸抓拍内部公共类型定义
 */

#pragma once

#include "common_process.h"

namespace FaceDetectInternal
{
/* 人脸抓拍目标信息，供联动层生成 SDK 告警和图片 */
struct FaceCaptureTarget_S
{
    /* 人脸目标矩形，坐标为算法输入分辨率 */
    Common::RectInfo_S stRect;
    /* 人脸检测置信度，取值范围 0~1 */
    float fConfidence = 0.0f;
    /* 人脸双眼瞳距，用于日志和后续扩展 */
    int nIpd = 0;
};

/* 人脸抓拍联动配置快照，避免执行联动时直接依赖 CFaceDetect 内部状态 */
struct FaceCaptureLinkageOptions_S
{
    /* 是否需要全景图联动 */
    bool bPanoramaImage = false;
    /* 是否需要目标小图联动 */
    bool bTargetImage = false;
    /* 是否需要邮件联动 */
    bool bEmail = false;
    /* 是否需要上传 SD 卡联动 */
    bool bUploadSdCard = false;
};

/* 人脸比对联动配置快照，按成功与失败结果分别解析，避免主类维护多组 bool 成员 */
struct FaceCompareLinkageOptions_S
{
    /* 是否需要全景图联动 */
    bool bPanoramaImage = false;
    /* 是否需要目标小图联动 */
    bool bTargetImage = false;
    /* 是否需要邮件联动 */
    bool bEmail = false;
    /* 是否需要上传 SD 卡联动 */
    bool bUploadSdCard = false;
};
} // namespace FaceDetectInternal
