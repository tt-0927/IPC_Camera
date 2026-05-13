/**
 * @FilePath     : svp_ai_detect.h
 * @Author       : zhouzirui
 * @Date         : 2025-04-28 19:22:22
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2025-08-13 10:33:00
 * @Description  : 海思ai检测模块封装
 */

#ifndef _SVP_AI_DETECT_H_
#define _SVP_AI_DETECT_H_
#ifdef __cplusplus
extern "C"
{
#endif

#include <stdio.h>
#include <poll.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/time.h>
#include "ss_mpi_sys_mem.h"
#include "ss_mpi_sys.h"
#include "ss_mpi_aidetect.h"
#include "mpi_common.h"

/*buffer的长度*/
#define SVP_AIDETECT_BUFFER_LEN (256)
/*每多少帧检查一次通道状态*/
#define SVP_AIDETECT_INDEX_NUM (5)
/*Ai检测最大输出矩形坐标数*/
#define SVP_AIDETECT_MAX_OUTPUT_RECT_NUM (20)

/*海思 ai检测必需参数*/
typedef struct _HiAiDetectNeedParam_S
{
    /*通道号 范围：0-7,OT_AIDETECT_MAX_CHN_NUM*/
    int nChn;
    /*模型加载方式*/
    ot_aidetect_model_load_mode enModelLoadMode;
    /*模型路径*/
    char aModelPath[SVP_AIDETECT_BUFFER_LEN];

} HiAiDetectNeedParam_S;

typedef struct _HiAiDetectExParam_S
{
    
} HiAiDetectExParam_S;

typedef struct _HiAiDetect_S HiAiDetect_S;
struct _HiAiDetect_S
{
    //info /**********************必需参数***************************/
    HiAiDetectNeedParam_S stNeedParam;
    // HiAiDetectExParam_S stExParam;

    ot_aidetect_result_array stResult;
    //info /**********************辅助参数***************************/
    
    //info /**********************功能列表***************************/
    /*ai检测初始化*/
    int (*svpAiDetect_init)(HiAiDetect_S *pHandle);

    /*ai检测去初始化*/
    int (*svpAiDetect_uninit)(HiAiDetect_S *pHandle);

    /*送帧给ai进行检测处理*/
    int (*svpAiDetect_sendFrame)(HiAiDetect_S *pHandle, ot_video_frame *pFrame);

    /* 初始化结果结构体 */
    void (*svpAiDetect_initResult)(ot_aidetect_result_array *pResult, ot_aidetect_model_info stModelInfo);

    /* 释放结果结构体 */
    void (*svpAiDetect_freeResult)(ot_aidetect_result_array *pResult);

    /*清除ai检测结果*/
    void (*svpAiDetect_clearResult)(ot_aidetect_result_array *pResult);

    /* 打印ai检测结果 */
    void (*svpAiDetect_printResult)(ot_aidetect_result_array *pResult);
};

/**
 * @brief       : 分配ai检测模块句柄
 * @author      : zhouzirui
 * @param        {HiAiDetectNeedParam_S} stNeedParam：ai必须参数
 * @return       {*}成功返回句柄，失败返回NULL
 */
HiAiDetect_S *svpAiDetect_alloc(HiAiDetectNeedParam_S stNeedParam);

/**
 * @brief       : 释放ai检测模块句柄
 * @author      : zhouzirui
 * @param        {HiAiDetect_S} *pHandle：句柄
 * @return       {*}
 */
void svpAiDetect_release(HiAiDetect_S *pHandle);

#ifdef __cplusplus
}
#endif
#endif
