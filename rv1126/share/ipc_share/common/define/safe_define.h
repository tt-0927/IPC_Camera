/*** 
 * @FilePath     : safe_define.h
 * @Author       : cyc
 * @Date         : 2026-01-13 10:10:46
 * @LastEditors  : cyc
 * @LastEditTime : 2026-01-13 14:57:02
 * @Description  : 安全函数宏的定义
 */

#ifndef SAFE_DEFINE_H
#define SAFE_DEFINE_H

#include <stdio.h>
#include <string.h>
#include "dlog.h"

/* 
    定义一个名为 SAFE_MEMCPY 的宏，用于安全地复制内存
参数:
   dest:     指向目标内存块的指针，数据将被复制到这里
   destsz:   目标缓冲区 dest 的总大小（以字节为单位）
   src:      指向源内存块的指针，数据将从这里复制
   count:    要复制的字节数

返回值:
    成功时返回实际复制的字节数 (count)
    失败时返回 -1
*/
#define SAFE_MEMCPY(dest, destsz, src, count) \
    ({ \
        long long _result_value; /* 使用 long long 避免 count 类型转换问题 */ \
        do { \
            /* 检查指针是否为空 */ \
            if ((dest) == NULL || (src) == NULL) { \
                dlog_error("SAFE_MEMCPY: Null pointer passed at %s:%d. Dest: %p, Src: %p\n", \
                           __FILE__, __LINE__, (void*)(dest), (void*)(src)); \
                _result_value = -1LL; \
                break; \
            } \
            \
            /* 检查复制的字节数是否超过目标缓冲区大小 */ \
            if ((count) > (destsz)) { \
                dlog_error("SAFE_MEMCPY: Buffer overflow attempt detected at %s:%d. Count: %zu, DestSz: %zu\n", \
                           __FILE__, __LINE__, (size_t)(count), (size_t)(destsz)); \
                _result_value = -1LL; \
                break; \
            } \
            \
            /* 如果所有检查都通过，则执行安全的 memcpy */ \
            memcpy((dest), (src), (count)); \
            _result_value = (long long)(count); /* 设置成功状态，返回复制的字节数 */ \
        } while(0); \
        (int)_result_value; /* 返回状态码或字节数，强制转换回 int */ \
    })

#endif // SAFE_DEFINE_H