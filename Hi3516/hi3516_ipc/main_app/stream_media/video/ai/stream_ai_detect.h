/**
 * @FilePath     : stream_ai_detect.h
 * @Author       : zhouzirui
 * @Date         : 2025-04-29 13:42:33
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2025-07-30 09:25:50
 * @Description  : AI 检测
 */

#pragma once

#include <iostream>
#include <mutex>
#include <atomic>
//  #include "video_define.h"
#include "IpcRet.h"

extern "C"
{
#include "svp_ai_detect.h"
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <signal.h>
#include <fcntl.h>
}

/*脸人车检测横屏模型*/
#define AI_HVF_NORMAL_MODEL_PATH "/opt/cam/model/normal/det_hvf_normal.bin"
/*包裹检测横屏模型*/
#define AI_PACKAGE_NORMAL_MODEL_PATH "/opt/cam/model/normal/det_package_normal.bin"
/*宠物检测横屏模型*/
#define AI_PET_NORMAL_MODEL_PATH "/opt/cam/model/normal/det_pet_normal.bin"
/*脸人车检测竖屏模型*/
#define AI_HVF_CORRIDOR_MODEL_PATH "/opt/cam/model/corridor/det_hvf_corridor.bin"
/*包裹检测竖屏模型*/
#define AI_PACKAGE_CORRIDOR_MODEL_PATH "/opt/cam/model/corridor/det_package_corridor.bin"
/*宠物检测竖屏模型*/
#define AI_PET_CORRIDOR_MODEL_PATH "/opt/cam/model/corridor/det_pet_corridor.bin"

/**
 * @brief       : AI 检测采集初始化
 * @return       {HiAiDetect_S*}NULL：失败 非空：句柄
 */
HiAiDetect_S *streamAiDetect_init(int nChn,const char *pModelPath);

/**
 * @brief       : AI 检测采集去初始化
 * @param        {HiAiDetect_S} *pHandle：句柄
 * @return       {*}成功返回0,失败返回-1
 */
int streamAiDetect_uninit(HiAiDetect_S *pHandle);
