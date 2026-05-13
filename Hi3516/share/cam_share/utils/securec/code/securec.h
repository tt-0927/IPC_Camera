#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <stdlib.h>

/****************************************************** 
*自定义头文件，部分宏的实现可能和原定义有差别
****************************************************** */
#ifndef SECUREC_ERRNO_H
#define SECUREC_ERRNO_H
/**
 * @brief 安全函数专用错误码类型
 * @note  兼容C标准库的errno机制，同时适配图片中函数的返回值需求
 */
typedef int errno_t;  // 基础类型保持与系统errno一致

/* 错误码定义 */
#define EOK                  0   // 成功
#define EINVAL              22  // 无效参数
#define EINVAL_AND_RESET    -22 // 无效参数并重置目标
#define ERANGE              34  // 范围错误
#define ERANGE_AND_RESET    -34 // 范围错误并重置目标
#define EOVERLAP_AND_RESET  -35 // 内存重叠并重置目标

#endif 


#ifndef SECUREC_DEFS_H
#define SECUREC_DEFS_H
/* 宽字符内存操作上限（单位：wchar_t个数） */
#define SECUREC_WCHAR_MEM_MAX_LEN (1024 * 1024)  // 1MB

/* 宽字符串最大字符数（含终止符） */
#define SECUREC_WCHAR_STRING_MAX_LEN (512 * 1024) // 512KB

/* 多字节字符最大长度（兼容C99标准） */
#define MB_CUR_MAX 8

/* 多字节操作缓冲区上限 */
#define SECUREC_MULTI_BYTE_MAX_LEN 16

/*普通字符内存操作上线*/
#define SECUREC_MEM_MAX_LEN (64 * 1024)  // 64KB

/*普通字符串最大字符数（含终止符）*/
#define SECUREC_STRING_MAX_LEN (1024)    //1KB

#endif // SECUREC_DEFS_H

/*---------------- 分支预测优化 ----------------*/
#ifndef SECUREC_LIKELY
#ifdef __GNUC__
/*通过 __builtin_expect指导编译器优化分支跳转*/
#define SECUREC_LIKELY(x)   __builtin_expect(!!(x), 1)  // GCC/Clang优化

#else
#define SECUREC_LIKELY(x)   (x)  // 默认实现

#endif
#endif
/*---------------- 内存管理 ----------------*/
#ifndef SECUREC_MALLOC
#define SECUREC_MALLOC(size)   malloc(size)
#endif

#ifndef SECUREC_FREE
#define SECUREC_FREE(p)       do { if (p) free(p); } while(0)
#endif

#ifndef SECUREC_FR
#define SECUREC_FR(p)         SECUREC_FREE(p)  // 别名宏

#endif

/* 安全字符类型定义 */
#if defined(_WIN32)
    typedef wchar_t SecChar;  // Windows平台使用宽字符
#else
    typedef char SecChar;     // Linux/Unix平台使用普通字符
#endif