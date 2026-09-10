/*** 
 * @FilePath     : event_task.h
 * @Author       : huangjunda
 * @Date         : 2025-04-29 09:54:20
 * @LastEditors  : liuhm
 * @LastEditTime : 2025-05-13 09:24:23
 * @Description  : 录制任务
 */

#pragma once

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
