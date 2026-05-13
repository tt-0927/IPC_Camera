/**
 * @FilePath     : stream_audio_sys.h
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2025-07-17 17:26:37
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2025-07-17 19:24:23
 * @Description  : MPI系统音频模块
 */

#pragma once

#include <iostream>
#include <mutex>
#include <atomic>
#include "video_define.h"
#include "IpcRet.h"

extern "C"
{
#include "mpp_sys.h"
#include <unistd.h>
#include <string.h>
}

/**
 * @brief   : MPI系统音频模块初始化 
 * @return   {int}成功返回0,失败返回-1 
 */
int stream_audio_sys_init();

/**
 * @brief   : MPI系统音频模块去初始化 
 * @return   {int}成功返回0,失败返回-1 
 */
int stream_audio_sys_deinit();
