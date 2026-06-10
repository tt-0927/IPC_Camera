/**
 * @FilePath     : face_detect_context.hpp
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2026-04-28 15:07:08
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-04-28 16:24:07
 * @Description  : 人脸检测单帧处理共享上下文
 */

#pragma once

#include <mutex>
#include <vector>

#include "YoloUltralyticsPoint_rpn.hpp"
#include "common_process.h"
#include "face_detect_worker.hpp"
namespace FaceDetectInternal
{
/**
 * @brief   : 人脸检测单帧处理共享上下文
 * @return   {struct} 为抓拍与特征处理器提供统一输入输出载体
 */
typedef struct _SFaceProcessContext_
{
    /* 当前帧人脸检测模型输出结果 */
    std::vector<Inference_NS::PointData_S> &vPointDatas;
    /* 当前帧汇总角框输出数组 */
    std::vector<Common::RectInfo_S> &vstRectInfo;
    /* 当前处理的视频帧 */
    ot_video_frame_info *pFrameInfo = nullptr;
    /* 当前算法分辨率宽度 */
    int nWidth = 0;
    /* 当前算法分辨率高度 */
    int nHeight = 0;
    /* 当前媒体通道号 */
    int nChnId = 0;
    /* 当前帧处理时间戳，单位毫秒；用于事件、抓拍文件名和上传表单保持一致 */
    long long llTimestamp = 0;
    /* NPU 推理互斥锁，特征模型切换上下文时使用 */
    CFaceDetectWorker* pDetectWorker = nullptr;
} SFaceProcessContext;
} // namespace FaceDetectInternal
