#include "securecutil.h"

#ifndef SECUREC_MEMCOPY_WITH_PERFORMANCE
/*是否启用高性能内存复制路径,默认关闭*/
#define SECUREC_MEMCOPY_WITH_PERFORMANCE 0   
#endif
/*如果开启小内存优化策略或者高性能内存复制*/
#if SECUREC_WITH_PERFORMANCE_ADDONS || SECUREC_MEMCOPY_WITH_PERFORMANCE
#ifndef SECUREC_MEMCOPY_THRESHOLD_SIZE
/*定义小内存优化的分界值为64字节*/
#define SECUREC_MEMCOPY_THRESHOLD_SIZE (64UL)
#endif


/**
 * SecStrBuf##num：通过宏拼接生成结构体类型名（如 SecStrBuf1、SecStrBuf2）。
将内存复制转化为​​结构体赋值​​，等效于memcpy性能更高、安全性更好，可以避免使用memcpy。
eg：*(struct { char data[2]; } *)dest = *(const struct { char data[2]; } *)src;
 */

#define SECUREC_COPY_STRUCT(num) case num:*(SecStrBuf##num *)dest=*(const SecStrBuf##num *) src;break;

#define SECUREC_SMALL_MEM_COPY do { \
        if (SECUREC_ADDR_ALIGNED_8(dest) && SECUREC_ADDR_ALIGNED_8(src)) { \
            /* use struct assignment */ \
            switch (count) { \
                SECUREC_COPY_STRUCT(1) \
                SECUREC_COPY_STRUCT(2) \
                SECUREC_COPY_STRUCT(3) \
                SECUREC_COPY_STRUCT(4) \
                SECUREC_COPY_STRUCT(5) \
                SECUREC_COPY_STRUCT(6) \
                SECUREC_COPY_STRUCT(7) \
                SECUREC_COPY_STRUCT(8) \
                SECUREC_COPY_STRUCT(9) \
                SECUREC_COPY_STRUCT(10) \
                SECUREC_COPY_STRUCT(11) \
                SECUREC_COPY_STRUCT(12) \
                SECUREC_COPY_STRUCT(13) \
                SECUREC_COPY_STRUCT(14) \
                SECUREC_COPY_STRUCT(15) \
                SECUREC_COPY_STRUCT(16) \
                SECUREC_COPY_STRUCT(17) \
                SECUREC_COPY_STRUCT(18) \
                SECUREC_COPY_STRUCT(19) \
                SECUREC_COPY_STRUCT(20) \
                SECUREC_COPY_STRUCT(21) \
                SECUREC_COPY_STRUCT(22) \
                SECUREC_COPY_STRUCT(23) \
                SECUREC_COPY_STRUCT(24) \
                SECUREC_COPY_STRUCT(25) \
                SECUREC_COPY_STRUCT(26) \
                SECUREC_COPY_STRUCT(27) \
                SECUREC_COPY_STRUCT(28) \
                SECUREC_COPY_STRUCT(29) \
                SECUREC_COPY_STRUCT(30) \
                SECUREC_COPY_STRUCT(31) \
                SECUREC_COPY_STRUCT(32) \
                SECUREC_COPY_STRUCT(33) \
                SECUREC_COPY_STRUCT(34) \
                SECUREC_COPY_STRUCT(35) \
                SECUREC_COPY_STRUCT(36) \
                SECUREC_COPY_STRUCT(37) \
                SECUREC_COPY_STRUCT(38) \
                SECUREC_COPY_STRUCT(39) \
                SECUREC_COPY_STRUCT(40) \
                SECUREC_COPY_STRUCT(41) \
                SECUREC_COPY_STRUCT(42) \
                SECUREC_COPY_STRUCT(43) \
                SECUREC_COPY_STRUCT(44) \
                SECUREC_COPY_STRUCT(45) \
                SECUREC_COPY_STRUCT(46) \
                SECUREC_COPY_STRUCT(47) \
                SECUREC_COPY_STRUCT(48) \
                SECUREC_COPY_STRUCT(49) \
                SECUREC_COPY_STRUCT(50) \
                SECUREC_COPY_STRUCT(51) \
                SECUREC_COPY_STRUCT(52) \
                SECUREC_COPY_STRUCT(53) \
                SECUREC_COPY_STRUCT(54) \
                SECUREC_COPY_STRUCT(55) \
                SECUREC_COPY_STRUCT(56) \
                SECUREC_COPY_STRUCT(57) \
                SECUREC_COPY_STRUCT(58) \
                SECUREC_COPY_STRUCT(59) \
                SECUREC_COPY_STRUCT(60) \
                SECUREC_COPY_STRUCT(61) \
                SECUREC_COPY_STRUCT(62) \
                SECUREC_COPY_STRUCT(63) \
                SECUREC_COPY_STRUCT(64) \
                default:break; \
            } /* END switch */ \
        } else { \
            char *tmpDest = (char *)dest; \
            const char *tmpSrc =  (const char *)src; \
            switch (count) { \
                case 64: *tmpDest++ = *tmpSrc++; /* fall-through */ /* FALLTHRU */ \
                case 63: *tmpDest++ = *tmpSrc++; /* fall-through */ /* FALLTHRU */ \
                case 62: *tmpDest++ = *tmpSrc++; /* fall-through */ /* FALLTHRU */ \
                case 61: *tmpDest++ = *tmpSrc++; /* fall-through */ /* FALLTHRU */ \
                case 60: *tmpDest++ = *tmpSrc++; /* fall-through */ /* FALLTHRU */ \
                case 59: *tmpDest++ = *tmpSrc++; /* fall-through */ /* FALLTHRU */ \
                case 58: *tmpDest++ = *tmpSrc++; /* fall-through */ /* FALLTHRU */ \
                case 57: *tmpDest++ = *tmpSrc++; /* fall-through */ /* FALLTHRU */ \
                case 56: *tmpDest++ = *tmpSrc++; /* fall-through */ /* FALLTHRU */ \
                case 55: *tmpDest++ = *tmpSrc++; /* fall-through */ /* FALLTHRU */ \
                case 54: *tmpDest++ = *tmpSrc++; /* fall-through */ /* FALLTHRU */ \
                case 53: *tmpDest++ = *tmpSrc++; /* fall-through */ /* FALLTHRU */ \
                case 52: *tmpDest++ = *tmpSrc++; /* fall-through */ /* FALLTHRU */ \
                case 51: *tmpDest++ = *tmpSrc++; /* fall-through */ /* FALLTHRU */ \
                case 50: *tmpDest++ = *tmpSrc++; /* fall-through */ /* FALLTHRU */ \
                case 49: *tmpDest++ = *tmpSrc++; /* fall-through */ /* FALLTHRU */ \
                case 48: *tmpDest++ = *tmpSrc++; /* fall-through */ /* FALLTHRU */ \
                case 47: *tmpDest++ = *tmpSrc++; /* fall-through */ /* FALLTHRU */ \
                case 46: *tmpDest++ = *tmpSrc++; /* fall-through */ /* FALLTHRU */ \
                case 45: *tmpDest++ = *tmpSrc++; /* fall-through */ /* FALLTHRU */ \
                case 44: *tmpDest++ = *tmpSrc++; /* fall-through */ /* FALLTHRU */ \
                case 43: *tmpDest++ = *tmpSrc++; /* fall-through */ /* FALLTHRU */ \
                case 42: *tmpDest++ = *tmpSrc++; /* fall-through */ /* FALLTHRU */ \
                case 41: *tmpDest++ = *tmpSrc++; /* fall-through */ /* FALLTHRU */ \
                case 40: *tmpDest++ = *tmpSrc++; /* fall-through */ /* FALLTHRU */ \
                case 39: *tmpDest++ = *tmpSrc++; /* fall-through */ /* FALLTHRU */ \
                case 38: *tmpDest++ = *tmpSrc++; /* fall-through */ /* FALLTHRU */ \
                case 37: *tmpDest++ = *tmpSrc++; /* fall-through */ /* FALLTHRU */ \
                case 36: *tmpDest++ = *tmpSrc++; /* fall-through */ /* FALLTHRU */ \
                case 35: *tmpDest++ = *tmpSrc++; /* fall-through */ /* FALLTHRU */ \
                case 34: *tmpDest++ = *tmpSrc++; /* fall-through */ /* FALLTHRU */ \
                case 33: *tmpDest++ = *tmpSrc++; /* fall-through */ /* FALLTHRU */ \
                case 32: *tmpDest++ = *tmpSrc++; /* fall-through */ /* FALLTHRU */ \
                case 31: *tmpDest++ = *tmpSrc++; /* fall-through */ /* FALLTHRU */ \
                case 30: *tmpDest++ = *tmpSrc++; /* fall-through */ /* FALLTHRU */ \
                case 29: *tmpDest++ = *tmpSrc++; /* fall-through */ /* FALLTHRU */ \
                case 28: *tmpDest++ = *tmpSrc++; /* fall-through */ /* FALLTHRU */ \
                case 27: *tmpDest++ = *tmpSrc++; /* fall-through */ /* FALLTHRU */ \
                case 26: *tmpDest++ = *tmpSrc++; /* fall-through */ /* FALLTHRU */ \
                case 25: *tmpDest++ = *tmpSrc++; /* fall-through */ /* FALLTHRU */ \
                case 24: *tmpDest++ = *tmpSrc++; /* fall-through */ /* FALLTHRU */ \
                case 23: *tmpDest++ = *tmpSrc++; /* fall-through */ /* FALLTHRU */ \
                case 22: *tmpDest++ = *tmpSrc++; /* fall-through */ /* FALLTHRU */ \
                case 21: *tmpDest++ = *tmpSrc++; /* fall-through */ /* FALLTHRU */ \
                case 20: *tmpDest++ = *tmpSrc++; /* fall-through */ /* FALLTHRU */ \
                case 19: *tmpDest++ = *tmpSrc++; /* fall-through */ /* FALLTHRU */ \
                case 18: *tmpDest++ = *tmpSrc++; /* fall-through */ /* FALLTHRU */ \
                case 17: *tmpDest++ = *tmpSrc++; /* fall-through */ /* FALLTHRU */ \
                case 16: *tmpDest++ = *tmpSrc++; /* fall-through */ /* FALLTHRU */ \
                case 15: *tmpDest++ = *tmpSrc++; /* fall-through */ /* FALLTHRU */ \
                case 14: *tmpDest++ = *tmpSrc++; /* fall-through */ /* FALLTHRU */ \
                case 13: *tmpDest++ = *tmpSrc++; /* fall-through */ /* FALLTHRU */ \
                case 12: *tmpDest++ = *tmpSrc++; /* fall-through */ /* FALLTHRU */ \
                case 11: *tmpDest++ = *tmpSrc++; /* fall-through */ /* FALLTHRU */ \
                case 10: *tmpDest++ = *tmpSrc++; /* fall-through */ /* FALLTHRU */ \
                case 9:  *tmpDest++ = *tmpSrc++; /* fall-through */ /* FALLTHRU */ \
                case 8:  *tmpDest++ = *tmpSrc++; /* fall-through */ /* FALLTHRU */ \
                case 7:  *tmpDest++ = *tmpSrc++; /* fall-through */ /* FALLTHRU */ \
                case 6:  *tmpDest++ = *tmpSrc++; /* fall-through */ /* FALLTHRU */ \
                case 5:  *tmpDest++ = *tmpSrc++; /* fall-through */ /* FALLTHRU */ \
                case 4:  *tmpDest++ = *tmpSrc++; /* fall-through */ /* FALLTHRU */ \
                case 3:  *tmpDest++ = *tmpSrc++; /* fall-through */ /* FALLTHRU */ \
                case 2:  *tmpDest++ = *tmpSrc++; /* fall-through */ /* FALLTHRU */ \
                case 1:  *tmpDest++ = *tmpSrc++; /* fall-through */ /* FALLTHRU */ \
                default:break; \
            } \
        } \
    } SECUREC_WHILE_ZERO
#endif


static errno_t SecMemcpyError(void *dest, size_t destMax, const void *src, size_t count);

#if SECUREC_WITH_PERFORMANCE_ADDONS || SECUREC_MEMCOPY_WITH_PERFORMANCE
static void SecDoMemcpyOpt(void *dest, const void *src, size_t count);
#endif
errno_t memcpy_s(void *dest, size_t destMax, const void *src, size_t count);

#if SECUREC_WITH_PERFORMANCE_ADDONS
errno_t memcpy_sOptAsm(void *dest, size_t destMax, const void *src, size_t count);
errno_t memcpy_sOptTc(void *dest, size_t destMax, const void *src, size_t count);
#endif

#if SECUREC_WITH_PERFORMANCE_ADDONS || SECUREC_MEMCOPY_WITH_PERFORMANCE
static void SecDoMemcpyOpt(void *dest, const void *src, size_t count);
#endif




