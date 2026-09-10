/*** 
 * @FilePath     : picture_task.h
 * @Author       : cyc
 * @Date         : 2025-06-13 14:15:30
 * @LastEditors  : cyc
 * @LastEditTime : 2025-08-07 16:36:07
 * @Description  : 图像任务
 */
#pragma once
#include "task_sub_class.h"

 namespace Task
 {
    namespace Pic
    {
        TaskSubClass(GetDayNight)
        TaskSubClass(SetDayNight)
        TaskSubClass(GetImageParam)
        TaskSubClass(SetImageParam)
        TaskSubClass(GetExposureParam)
        TaskSubClass(SetExposureParam)
        TaskSubClass(GetBackLightParam)
        TaskSubClass(SetBackLightParam)
        TaskSubClass(GetAwbParam)
        TaskSubClass(SetAwbParam)
        TaskSubClass(GetDnrParam)
        TaskSubClass(SetDnrParam)
        TaskSubClass(GetVideoMirrorParam)
        TaskSubClass(SetVideoMirrorParam)
        TaskSubClass(GetSchedule)
        TaskSubClass(SetSchedule)
        TaskSubClass(GetScene)
        TaskSubClass(SetScene)
        TaskSubClass(SetDefault)

        TaskSubClass(GetOsdConfigParam)
        TaskSubClass(SetOsdConfigParam)
        TaskSubClass(GetCoverConfigParam)
        TaskSubClass(SetCoverConfigParam)

    } //namespace Pic

 } //namespace Task