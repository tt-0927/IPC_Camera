/**
 * @FilePath     : classroom_osd.h
 * @Author       : leiyy
 * @Date         : 2026-10-20 17:20:00
 * @LastEditors  : leiyy
 * @LastEditTime : 2026-10-20 17:20:00
 * @Description  : 课堂框送整机 OSD
 */

#pragma once

#include <cstdint>
#include <vector>

#include "classroom_define.h"
#include "frame_geometry.h"

namespace Classroom
{
/**
 * @brief   : 送一帧检测框到整机 OSD（内部换算回源帧坐标）
 * @param    {const CFrameGeometry&} stGeom：本帧的分析帧↔源帧变换，取特征帧自带的那份
 * @param    {const std::vector<CRectf_S>&} vecCanvasBox：分析帧坐标框表，**允许为空**
 */
void classroomOsdPush(const CFrameGeometry &stGeom, const std::vector<CRectf_S> &vecCanvasBox);
}
