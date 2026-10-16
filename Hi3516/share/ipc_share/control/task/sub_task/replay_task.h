/*
 * @Author: zhangjc zhangjc@kfb.cn
 * @Date: 2024-10-29 19:33:17
 * @LastEditors: 梁浩尧 lianghaoyao@kfb.cn
 * @LastEditTime: 2025-08-13 21:11:43
 * @FilePath: /hisi/share/ipc_share/control/task/sub_task/replay_task.h
 * @Description: 录像回放
 */

#pragma once

/* 本地 SD 存储能力裁剪：能力关闭（IPC_CAP_LOCAL_STORAGE=0）时本文件整体不参与编译 */
#if CAP_LOCAL_STORAGE

#include "task_sub_class.h"

namespace Task
{
    namespace Replay
    {
        TaskSubClass(SetLayoutInfo);
    }    // namespace Replay

    } // namespace Task

#endif /* CAP_LOCAL_STORAGE */
