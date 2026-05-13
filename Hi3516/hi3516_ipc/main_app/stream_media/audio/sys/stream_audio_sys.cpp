/**
 * @FilePath     : stream_audio_sys.cpp
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2025-07-17 17:26:37
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2025-07-17 19:24:13
 * @Description  : MPI系统音频模块
 */

#include "stream_audio_sys.h"
#include "dlog.h"

int stream_audio_sys_init()
{
    int nRet = OK;
    /* 去初始化Audio模块 */
    nRet = mppAudio_uninit();
    if (nRet != OK)
    {
        dlog_error("去初始化Audio模块失败");
        return ERR;
    }

    /* 初始化Audio模块 */
    nRet = mppAudio_init();
    if (nRet != OK)
    {
        dlog_error("初始化Audio模块失败");
        return ERR;
    }

    dlog_info("Audio模块初始化成功");
    return OK;
}

int stream_audio_sys_deinit()
{
    int nRet = OK;
    /* 去初始化Audio模块 */
    nRet = mppAudio_uninit();
    if (nRet != OK)
    {
        dlog_error("去初始化Audio模块失败");
        return ERR;
    }

    dlog_info("Audio模块去初始化成功");
    return OK;
}
