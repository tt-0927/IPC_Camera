/*** 
 * @FilePath     : event_task.h
 * @Author       : huangjunda
 * @Date         : 2025-04-29 09:54:20
 * @LastEditors  : liuhm
 * @LastEditTime : 2025-05-13 09:24:23
 * @Description  : 录制任务
 */

#pragma once

/* 本地 SD 存储能力裁剪：能力关闭（IPC_CAP_LOCAL_STORAGE=0）时本文件整体不参与编译 */
#if CAP_LOCAL_STORAGE

#include "task_sub_class.h"

namespace Task
{
    namespace Record
    {
        TaskSubClass(CtrlRecordInfo)
        TaskSubClass(NoticeRecordFileInfo)
        TaskSubClass(DelRecordFileInfo)
        TaskSubClass(SetRecordFileInfo)
        TaskSubClass(FindRecordFileInfo)

        TaskSubClass(NoticeRecordTsFileInfo)
        TaskSubClass(NoticeRecordException)
        TaskSubClass(GetHumanRecord)
        TaskSubClass(SetHumanRecord)

        TaskSubClass(DownloadRecordFile)
        TaskSubClass(NoticeDownloadRecordProgress)
        TaskSubClass(GetAdvancedParam)
        TaskSubClass(SetAdvancedParam)

        TaskSubClass(GetSchedule)
        TaskSubClass(SetSchedule)
        TaskSubClass(GetRecordOtherInfo)
        TaskSubClass(SetRecordOtherInfo)

        TaskSubClass(GetRecordStatusInfo)
    } /* namespace Record end */
} /* namespace Task end */

#endif /* CAP_LOCAL_STORAGE */
