/**
 * @FilePath     : stats_feed.h
 * @Author       : leiyy
 * @Date         : 2026-11-03 10:09:00
 * @LastEditors  : leiyy
 * @LastEditTime : 2026-11-03 10:09:00
 * @Description  : 采集适配：课堂内部口径 → 统计输入
 */

#pragma once

#include <cstdint>

#include "attendance.h"
#include "feature_frame.h"
#include "group_stats.h"
#include "tvsdk_stats.h"

namespace Classroom
{
/* 课堂还活着的窗口：最近一次学生样本在这么久之内。
   下课以后所有人都不会再被看到；不加这道闸，中途离课会把整班算进去 */
constexpr int64_t STATS_CLASS_ALIVE_WINDOW_MS = 120000;

/**
 * @brief   : 从一拍的行表 + 人表构造聚集判定用的人（只收学生、必须有头框）
 * @param    {CRowStatInput_S*} patRows：行表首地址
 * @param    {int32_t} nRowNum：行数
 * @param    {CPersonFeature_S*} patPersons：同一拍的人表（与行表同下标）
 * @param    {int32_t} nPersonNum：人表条数
 * @param    {CStatsPerson_S*} pastOut：输出缓冲
 * @param    {int32_t} nOutCap：输出上限
 * @return   {int32_t} 实际写入条数
 * @note     : 坐标用分析帧像素（同一帧、同一把尺）；只看站立位。
 *            没有头框的行直接跳过——没有头宽，固定像素半径会把远近焦的人混成一簇。
 */
inline int32_t buildStatsPersons(const CRowStatInput_S *patRows, int32_t nRowNum,
                                 const CPersonFeature_S *patPersons, int32_t nPersonNum,
                                 CStatsPerson_S *pastOut, int32_t nOutCap)
{
    if (nullptr == patRows || nullptr == patPersons || nullptr == pastOut || nOutCap <= 0)
    {
        return 0;
    }
    const int32_t nRows = (nRowNum < nPersonNum) ? nRowNum : nPersonNum;
    if (nRows <= 0)
    {
        return 0;
    }

    const int nStandBit = studentMaskBit(Behavior_E::STU_STAND_UP);
    int32_t nOut = 0;
    for (int32_t nRow = 0; nRow < nRows && nOut < nOutCap; ++nRow)
    {
        const CRowStatInput_S &stRowIn = patRows[nRow];
        if (!stRowIn.bStudent)
        {
            continue;
        }
        const CPersonFeature_S &stRow = patPersons[nRow];
        const float fHeadW = stRow.stHeadBox.fX2 - stRow.stHeadBox.fX1;
        if (!(fHeadW > 0.0f))
        {
            continue;
        }
        CStatsPerson_S &rstOut = pastOut[nOut++];
        rstOut = CStatsPerson_S();
        rstOut.nPersonId = stRow.nPersonId;
        rstOut.fX = 0.5f * (stRow.stHeadBox.fX1 + stRow.stHeadBox.fX2);
        rstOut.fY = 0.5f * (stRow.stHeadBox.fY1 + stRow.stHeadBox.fY2);
        rstOut.fHeadW = fHeadW;
        rstOut.bStudent = true;
        rstOut.bStanding = (nStandBit >= 0)
                           && (0 != (stRowIn.u32StuMask & (1u << nStandBit)));
    }
    return nOut;
}

/**
 * @brief   : 中途离课人数（已计到课、但最近一次被看到已超过阈值）
 * @param    {int64_t} llLastPresentMs：最近一次学生样本的时刻；<=0 表示这节还没开始
 * @note     : 只在课堂还活着时判（llLastPresentMs 在 STATS_CLASS_ALIVE_WINDOW_MS 之内）。
 *            下课以后所有人都不会再被看到，不加这道闸会把整班都当成中途离课。
 */
inline int32_t countLeftMid(const CAttendRec_S *pastRecs, int32_t nRecNum,
                            int64_t llNowMs, int64_t llLeaveMs, int64_t llLastPresentMs)
{
    if (nullptr == pastRecs || nRecNum <= 0)
    {
        return 0;
    }
    if (llLastPresentMs <= 0 || llNowMs - llLastPresentMs > STATS_CLASS_ALIVE_WINDOW_MS)
    {
        return 0;
    }

    int32_t nCount = 0;
    for (int32_t i = 0; i < nRecNum; ++i)
    {
        const CAttendRec_S &rstRec = pastRecs[i];
        if (rstRec.bCounted && rstRec.llLastMs > 0 && llNowMs - rstRec.llLastMs >= llLeaveMs)
        {
            ++nCount;
        }
    }
    return nCount;
}
}
