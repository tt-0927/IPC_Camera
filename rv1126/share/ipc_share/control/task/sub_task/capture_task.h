/**
 * @FilePath     : capture_task.h
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2025-07-15 17:30:50
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2025-07-15 17:43:12
 * @Description  : 抓图任务
 */

#pragma once

/* 本地 SD 存储能力裁剪：能力关闭（IPC_CAP_LOCAL_STORAGE=0）时本文件整体不参与编译 */
#if CAP_LOCAL_STORAGE

#include "task_sub_class.h"

namespace Task
{
    namespace Capture
    {
        TaskSubClass(GetCapturePlanInfo)
        TaskSubClass(SetCapturePlanInfo)
        TaskSubClass(GetCaptureParamInfo)
        TaskSubClass(SetCaptureParamInfo)
    } /* namespace Capture end */
} /* namespace Task end */

#endif /* CAP_LOCAL_STORAGE */
