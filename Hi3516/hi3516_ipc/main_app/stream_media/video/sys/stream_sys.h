/**
 * @FilePath     : stream_sys.h
 * @Author       : zhouzirui
 * @Date         : 2025-05-19 16:09:50
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2025-09-03 17:34:13
 * @Description  : MPI系统/视频缓存池
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
#include "mpp_bind.h"
#include "mpp_vpss.h"
#include <unistd.h>
#include <string.h>
}

/**
 * @brief       : MPI系统/视频缓存池初始化
 * @author      : zhouzirui
 * @param        {vector<Video_NS::VideoConfig_S>} &vstVideoConfig
 * @return       {*}成功返回0,失败返回-1
 */
int streamSys_init(const std::vector<Video_NS::VideoConfig_S> &vstVideoConfig);

/**
 * @brief       : MPI系统/视频缓存池去初始化
 * @author      : zhouzirui
 * @return       {*}成功返回0,失败返回-1
 */
int streamSys_deinit();
