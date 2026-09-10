/**
 * @FilePath     : av_task.h
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2025-06-27 14:25:30
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2025-09-15 16:59:25
 * @Description  : 音视频任务
 */

#pragma once

#include "task_sub_class.h"

namespace Task
{
    namespace AV
    {
        TaskSubClass(GetVideoConfig)
        TaskSubClass(SetVideoConfig)
        TaskSubClass(GetVideoCapabilitySet)
        TaskSubClass(GetAudioConfig)
        TaskSubClass(GetAudioCapabilitySet)
        TaskSubClass(SetAudioConfig)
        TaskSubClass(GetVideoRoiConfig)
        TaskSubClass(SetVideoRoiConfig)
        TaskSubClass(GetAreaCropConfig)
        TaskSubClass(SetAreaCropConfig)
        TaskSubClass(GetAreaCropConversionResolution)
    } /* namespace AV end */
} /* namespace Task end */
