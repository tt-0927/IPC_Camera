/**
 * @FilePath     : mpi_common.h
 * @Author       : zhouzirui
 * @Date         : 2025-03-19 19:49:24
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2025-10-27 09:13:05
 * @Description  : 模块公用头文件定义
 */

#ifndef _MPI_COMMON_H_
#define _MPI_COMMON_H_
#ifdef __cplusplus
extern "C"
{
#endif
#include <stdio.h>

/*向上字节对齐 x:字节长度 a: 以多少字节对齐/对齐位数*/
#define MPI_ALIGN_UP(x, a) (((x) + (a) - 1) & ~((a) - 1))
/*向下字节对齐*/
#define MPI_ALIGN_BACK(x, a) (((x) / (a) * (a)))

/*封装printf*/
#define mpi_log(fmt...) \
    do { \
        printf("[Func]:%s [Line]:%d ", __FUNCTION__, __LINE__); \
        printf(fmt); \
        printf("\n"); \
        fflush(stdout); \
    } while (0)

/* 基准日志宏定义 */
#define mpi_module_log(module, fmt, ...)                                              \
    do                                                                                \
    {                                                                                 \
        printf("\033[1;34m[%s][Func]:%s [Line]:%d ", module, __FUNCTION__, __LINE__); \
        printf(fmt, ##__VA_ARGS__);                                                   \
        printf("\033[0;39m\n");                                                       \
        fflush(stdout);                                                               \
    } while (0)

/* 各模块日志宏定义 */
#define mpi_adec_log(fmt, ...)      mpi_module_log("ADEC", fmt, ##__VA_ARGS__)
#define mpi_aenc_log(fmt, ...)      mpi_module_log("AENC", fmt, ##__VA_ARGS__)
#define mpi_ai_log(fmt, ...)        mpi_module_log("AI", fmt, ##__VA_ARGS__)
#define mpi_ai_detect_log(fmt, ...) mpi_module_log("AiDetect", fmt, ##__VA_ARGS__)
#define mpi_ao_log(fmt, ...)        mpi_module_log("AO", fmt, ##__VA_ARGS__)
#define mpi_resample_log(fmt, ...)  mpi_module_log("RESAMPLE", fmt, ##__VA_ARGS__)
#define mpi_rgn_log(fmt, ...)       mpi_module_log("RGN", fmt, ##__VA_ARGS__)
#define mpi_venc_log(fmt, ...)      mpi_module_log("VENC", fmt, ##__VA_ARGS__)
#define mpi_vgs_log(fmt, ...)       mpi_module_log("VGS", fmt, ##__VA_ARGS__)
#define mpi_vi_log(fmt, ...)        mpi_module_log("VI", fmt, ##__VA_ARGS__)
#define mpi_vpss_log(fmt, ...)      mpi_module_log("VPSS", fmt, ##__VA_ARGS__)

#define mpi_ive_log(fmt, ...)        mpi_module_log("IVE", fmt, ##__VA_ARGS__)
#define mpi_ld_log(fmt, ...)        mpi_module_log("LD", fmt, ##__VA_ARGS__)
#define mpi_md_log(fmt, ...)        mpi_module_log("MD", fmt, ##__VA_ARGS__)

/*检查API的返回值，失败时打印错误信息与返回码*/
#define CHECK_API_RETURN(func) \
    do { \
        td_s32 result = func; \
        if (result != 0) { \
            printf("\033[0;31m[MPP][Func]:%s [Line]:%d %s failed, error code: 0x%08X\033[0;39m\n", __FUNCTION__, __LINE__, #func, (unsigned int)result); \
            fflush(stdout); \
            return result; \
        } \
    } while (0)

/*检查API的返回值，需传入用于判断的返回码、打印信息，失败时打印错误信息与返回码*/
#define CHECK_API_RETURN_PRINT(func, format, ...)   \
    do { \
        td_s32 result = func; \
        if (result != 0) { \
            printf("\033[0;31m[MPP][Func]:%s [Line]:%d %s failed, error code: 0x%08X\033[0;39m\n", __FUNCTION__, __LINE__, #func, (unsigned int)result); \
            printf("\033[0;31m"); \
            printf(format, ##__VA_ARGS__); \
            printf("\033[0;39m"); \
            fflush(stdout); \
            return result; \
        } \
    } while (0)

#ifdef __cplusplus
}
#endif
#endif