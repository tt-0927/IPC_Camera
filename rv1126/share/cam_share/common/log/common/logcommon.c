/*
 * @Author       : EasonLu
 * @Date         : 2023-09-05 14:41:28
 * @LastEditors  : EasonLu
 * @LastEditTime : 2023-09-05 14:50:52
 * @FilePath     : logcommon.c
 * @Description  : 日志公共函数
 */
#include "logcommon.h"
#include <stdio.h>
#include "share_device.h"
#include "dlog.h"

const char *get_logPathPrefix()
{
    return LOG_COMMON_PATH_LOCAL;
}

int get_dlogOutputLevel()
{
#if 0
    int nRet = -1;
    if(TS_0664 == share_get_currDeviceID())
    {
        nRet = LOG_ERROR;
    }
    else
    {
        nRet = LOG_TRACE;
    }
    return nRet;
#else
    /* 默认开启所有日志 */
    return LOG_TRACE;
#endif
}
