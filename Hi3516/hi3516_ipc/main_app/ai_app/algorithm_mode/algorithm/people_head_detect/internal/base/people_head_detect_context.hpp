/**
 * @FilePath     : people_head_detect_context.hpp
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2026-04-23 17:05:08
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-04-29 14:21:59
 * @Description  : 人头检测处理器上下文
 */

#ifndef PEOPLE_HEAD_DETECT_CONTEXT_HPP
#define PEOPLE_HEAD_DETECT_CONTEXT_HPP

#include <vector>

#include "common_process.h"

namespace PeopleHeadDetectInternal
{
typedef struct _SPeopleHeadProcessContext_
{
    /* 当前帧人头检测输出 */
    std::vector<Inference_NS::BoxData_S> &vBoxDatas;
    /* 当前帧汇总角框输出数组 */
    std::vector<Common::RectInfo_S> &vstRectInfo;
    /* 当前算法分辨率宽度 */
    int nWidth;
    /* 当前算法分辨率高度 */
    int nHeight;
    /* 当前帧时间戳，单位毫秒 */
    long long llNowMs;
    /* 当前媒体通道号 */
    int nChnId;
#if CAP_EXHIBITION_OSD_PANEL
    /* 当前事件面板输出指针，未启用面板时可为空 */
    OsdPanel::PanelFrame_S *pstPanelFrame = nullptr;
#endif
    /* 当前检测帧指针，周期统计需要编码全景图时使用 */
    ot_video_frame_info *pFrameInfo = nullptr;
} SPeopleHeadProcessContext;
} // namespace PeopleHeadDetectInternal

#endif
