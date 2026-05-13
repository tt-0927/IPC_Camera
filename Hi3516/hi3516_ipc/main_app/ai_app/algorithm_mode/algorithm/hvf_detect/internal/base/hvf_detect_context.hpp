/**
 * @FilePath     : hvf_detect_context.hpp
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2026-03-26 16:20:00
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-04-23 09:36:47
 * @Description  : HVF 单帧处理共享上下文
 */

#pragma once

#include <vector>

#include "common_process.h"

namespace HVFDetectInternal
{
/**
 * @brief   : HVF 单帧处理共享上下文
 * @return   {struct} 为各事件处理器提供统一输入输出载体
 */
typedef struct _SHVFProcessContext_
{
    /* 当前帧 AI 检测结果 */
    ot_aidetect_result_array &stResult;
    /* 当前帧汇总角框输出数组 */
    std::vector<Common::RectInfo_S> &vstRectInfo;
    /* 当前算法分辨率宽度 */
    int nWidth;
    /* 当前算法分辨率高度 */
    int nHeight;
#if CAP_EXHIBITION_OSD_PANEL
    /* 当前事件面板输出指针，未启用面板时可为空 */
    OsdPanel::PanelFrame_S *pstPanelFrame = nullptr;
#endif
    /* 当前媒体通道号，未传入时默认为 0 */
    int nChnId = 0;
    /* 当前帧时间戳，单位毫秒 */
    long long llTimestamp = 0;
    /* 当前帧视频帧指针，用于事件报警时编码触发帧图片 */
    ot_video_frame_info *pFrameInfo = nullptr;
} SHVFProcessContext;
} // namespace HVFDetectInternal
