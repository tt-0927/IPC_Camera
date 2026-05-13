/***
 * @FilePath     : preview_task.h
 * @Author       : 严泽辉 (yanzeh@kfb.cn)
 * @Date         : 2025-07-08 17:15:33
 * @LastEditors  : huangjunda
 * @LastEditTime : 2025-07-08 17:16:05
 * @Description  : 预览相关任务
 */

#pragma once

#include "task_sub_class.h"

namespace Task
{
    namespace Preview
    {
        TaskSubClass(GetPreviewInfo);
        TaskSubClass(SetPreviewInfo);
        TaskSubClass(GetCollectAudioInfo);
        TaskSubClass(SetIntercomInfo);
        TaskSubClass(SetBroadcastInfo);
        TaskSubClass(SetBeepAlarm);
        TaskSubClass(GetIntercomAndBroadcastStatus);
    } // namespace Preview

} // namespace Task
