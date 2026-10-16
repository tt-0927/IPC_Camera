/**
 * @FilePath     : face_types.hpp
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2026-09-21
 * @Description  : 人脸链路域类型——统一检测层人脸业务（抓拍/比对/名单库）的公共数据契约。
 *
 * 取代 Legacy SFaceProcessContext 在新链路的使用：
 * - 跟踪 id 随目标显式流转（FaceTarget_S.nTrackId），不再经上下文平行数组传递；
 * - 上下文只承载帧数据，不含执行器指针（Worker 经各处理器 init/attach 注入）；
 * - 处理器间图片复用缓存显式化为域类型成员。
 */

#pragma once

#include <string>
#include <vector>

#include "common_define.h"
#include "ot_common_video.h"

namespace FaceChain_NS
{
/* 跟踪后的稳定人脸目标（检测分辨率坐标系，供抓拍/比对/名单库统一消费） */
struct FaceTarget_S
{
    /* 目标框（检测空间，x1/y1/x2/y2） */
    Common::RectInfo_S stRect;
    /* 检测置信度 */
    float fConfidence = 0.0f;
    /* 跟踪 id（统一检测层跟踪模块分配；未接跟踪时为 -1，取优共用同一桶） */
    int nTrackId = -1;
};

/*
 * 单帧图片文件缓存：抓拍与比对按需复用同一文件，只随当前回调存在；
 * 任一功能单独开启时缓存为空，由该功能自行生成图片，不依赖另一功能。
 */
struct FaceFrameImageCache_S
{
    std::string strTargetImagePath;
    std::string strPanoramaImagePath;
};

/* 人脸链路单帧上下文（纯数据，不含执行器指针） */
struct FaceFrameContext_S
{
    /* 原始全分辨率帧：抓拍裁剪与比对特征提取的源帧（目标框为检测空间，裁剪前逆变换） */
    ot_video_frame_info *pSrcFrameInfo = nullptr;
    /* 当前媒体通道号 */
    int nChnId = 0;
    /* 当前帧处理时间戳（毫秒），事件、抓拍文件名与上传表单共用保持一致 */
    long long llTimestamp = 0;
    /* 检测分辨率宽（目标框坐标换算基准） */
    int nDetectWidth = 0;
    /* 检测分辨率高 */
    int nDetectHeight = 0;
    /* 单帧图片复用缓存（抓拍填、比对读/回写） */
    FaceFrameImageCache_S stImageCache;
};
} // namespace FaceChain_NS
