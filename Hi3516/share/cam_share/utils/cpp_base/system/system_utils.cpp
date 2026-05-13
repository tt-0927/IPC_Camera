/**
 * @FilePath     : system_utils.cpp
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2025-11-11 15:31:39
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2025-11-11 15:41:25
 * @Description  : 系统工具
 */

#include "system_utils.h"
#include <pthread.h>
#include <sched.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

void setThreadPriority(std::thread &thr, int policy, int prio)
{
    pthread_t handle = thr.native_handle();
    struct sched_param param;
    memset(&param, 0, sizeof(param));
    param.sched_priority = prio;

    int ret = pthread_setschedparam(handle, policy, &param);
    if (ret != 0)
    {
        // 权限不足或不支持实时调度，自动降级为普通策略
        if (ret == EPERM || ret == EINVAL)
        {
            printf("[system_utils] Warning: SCHED_RR/SCHED_FIFO not permitted, downgrade to SCHED_OTHER.\n");
            param.sched_priority = 0;
            pthread_setschedparam(handle, SCHED_OTHER, &param);
        }
        else
        {
            printf("[system_utils] setThreadPriority failed: %s\n", strerror(ret));
        }
    }
}

void printThreadSchedInfo(std::thread &thr, const std::string &name)
{
    pthread_t handle = thr.native_handle();
    int policy = 0;
    struct sched_param param;
    memset(&param, 0, sizeof(param));

    if (pthread_getschedparam(handle, &policy, &param) == 0)
    {
        const char *policy_str = "UNKNOWN";
        switch (policy)
        {
        case SCHED_OTHER:
            policy_str = "SCHED_OTHER";
            break;
        case SCHED_RR:
            policy_str = "SCHED_RR";
            break;
        case SCHED_FIFO:
            policy_str = "SCHED_FIFO";
            break;
        }

        if (!name.empty())
            printf("[system_utils] Thread(%s): policy=%s, prio=%d\n", name.c_str(), policy_str, param.sched_priority);
        else
            printf("[system_utils] Thread: policy=%s, prio=%d\n", policy_str, param.sched_priority);
    }
    else
    {
        printf("[system_utils] pthread_getschedparam failed.\n");
    }
}
