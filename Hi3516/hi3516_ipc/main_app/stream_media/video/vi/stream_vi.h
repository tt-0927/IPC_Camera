/**
 * @FilePath     : stream_vi.h
 * @Author       : zhouzirui
 * @Date         : 2024-12-06 13:52:42
 * @LastEditors  : zhouzirui
 * @LastEditTime : 2025-03-31 20:16:00
 * @Description  : VI 视频采集输入
 */

#pragma once

#include <iostream>
#include <mutex>
#include <atomic>
#include "video_define.h"
#include "IpcRet.h"

extern "C"
{
#include "mpp_vi.h"
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <signal.h>
#include <fcntl.h>
}

/**
 * @brief       : 视频输入采集初始化
 * @author      : zhouzirui
 * @return       {HiVi_S*}NULL：失败 非空：句柄
 */
HiVi_S *streamVi_init();

/**
 * @brief       : 视频输入采集去初始化
 * @author      : zhouzirui
 * @param        {HiVi_S} *pHandle：句柄
 * @return       {*}成功返回0,失败返回-1
 */
int streamVi_uninit(HiVi_S *pHandle);
