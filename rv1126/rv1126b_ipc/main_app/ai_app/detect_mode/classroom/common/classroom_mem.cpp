/**
 * @FilePath     : classroom_mem.cpp
 * @Author       : leiyy
 * @Date         : 2026-09-22 10:07:11
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-09-22 10:07:11
 * @Description  : 内存采样薄封装
 */

#include "classroom_mem.h"
#include "performance_monitor.h"

namespace Classroom
{
CMemInfo_S classroomQueryMem()
{
    MonitorMemoryInfo_S stMon;
    CMemInfo_S stInfo;

    /* perfMonitor 内部会 memset 清零，这里无需预置 */
    const int nRet = perfMonitor_getMemoryInfo(&stMon);
    if (nRet < 0)
    {
        /* OK==0、各 ERR_* 均为负值；两套 ERR 枚举（IpcRet / BlError）符号位一致，按负值判失败最稳 */
        stInfo.bValid = false;
        return stInfo;
    }

    stInfo.bValid = true;
    stInfo.llMemAvailableKb = static_cast<int64_t>(stMon.nSystemAvailableKb);
    stInfo.llVmRssKb = static_cast<int64_t>(stMon.nVmRssKb);
    stInfo.llMmzUsedKb = static_cast<int64_t>(stMon.nMmzUsedKb);
    stInfo.llThreadNum = static_cast<int64_t>(stMon.nThreads);
    return stInfo;
}

bool classroomCanAfford(const CMemInfo_S &stMem, int64_t llNeedKb, int64_t llReserveKb)
{
    if (!stMem.bValid)
    {
        return false;
    }
    if (llNeedKb <= 0)
    {
        return true;
    }
    if (llReserveKb < 0)
    {
        llReserveKb = 0;
    }

    if (stMem.llMemAvailableKb <= llReserveKb + llNeedKb)
    {
        return false;
    }
    return true;
}
}
