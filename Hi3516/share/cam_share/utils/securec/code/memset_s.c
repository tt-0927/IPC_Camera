
#include "securecutil.h"

// #ifndef SECUREC_MEMSET_WITH_PERFORMANCE
// #define SECUREC_MEMSET_WITH_PERFORMANCE 0
// #endif

// #if SECUREC_WITH_PERFORMANCE_ADDONS || SECUREC_MEMSET_WITH_PERFORMANCE

// /* use union to clear strict-aliasing warning */
// typedef union {
//     SecStrBuf32 buf32;
//     SecStrBuf31 buf31;
//     SecStrBuf30 buf30;
//     SecStrBuf29 buf29;
//     SecStrBuf28 buf28;
//     SecStrBuf27 buf27;
//     SecStrBuf26 buf26;
//     SecStrBuf25 buf25;
//     SecStrBuf24 buf24;
//     SecStrBuf23 buf23;
//     SecStrBuf22 buf22;
//     SecStrBuf21 buf21;
//     SecStrBuf20 buf20;
//     SecStrBuf19 buf19;
//     SecStrBuf18 buf18;
//     SecStrBuf17 buf17;
//     SecStrBuf16 buf16;
//     SecStrBuf15 buf15;
//     SecStrBuf14 buf14;
//     SecStrBuf13 buf13;
//     SecStrBuf12 buf12;
//     SecStrBuf11 buf11;
//     SecStrBuf10 buf10;
//     SecStrBuf9 buf9;
//     SecStrBuf8 buf8;
//     SecStrBuf7 buf7;
//     SecStrBuf6 buf6;
//     SecStrBuf5 buf5;
//     SecStrBuf4 buf4;
//     SecStrBuf3 buf3;
//     SecStrBuf2 buf2;
//     SecStrBuf1 buf1;
// } SecStrBuf32Union;
// /* C standard initializes the first member of the consortium. */
// static const SecStrBuf32 g_allZero = {{'\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', 
//                                        '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0',
//                                        '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', 
//                                        '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0'}};
// static const SecStrBuf32 g_allFF   = {{0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 
//                                        0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 
//                                        0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 
//                                        0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}};

// static const SecStrBuf32Union *SecStrictAliasingCast(const SecStrBuf32 *buf)
// {
//     return (const SecStrBuf32Union *)buf;
// }

// #ifndef SECUREC_MEMSET_THRESHOLD_SIZE
// #define SECUREC_MEMSET_THRESHOLD_SIZE (32UL)
// #endif

// #define SECUREC_COPY_STRUCT_4_ZERO(num) \
//         case num: \
//             *(SecStrBuf##num *)dest = *(const SecStrBuf##num *)(&((SecStrictAliasingCast(&g_allZero))->buf##num)); \
//             break;

// #define SECUREC_COPY_STRUCT_4_FF(num) \
//         case num: \
//             *(SecStrBuf##num *)dest = *(const SecStrBuf##num *)(&((SecStrictAliasingCast(&g_allFF))->buf##num)); \
//             break;

// #define SECUREC_UNALIGNED_SET do { \
//         char *pcDest = (char *)dest; \
//         switch (count) { \
//             case 32: *pcDest++ = (char)c; /* fall-through */ /* FALLTHRU */ \
//             case 31: *pcDest++ = (char)c; /* fall-through */ /* FALLTHRU */ \
//             case 30: *pcDest++ = (char)c; /* fall-through */ /* FALLTHRU */ \
//             case 29: *pcDest++ = (char)c; /* fall-through */ /* FALLTHRU */ \
//             case 28: *pcDest++ = (char)c; /* fall-through */ /* FALLTHRU */ \
//             case 27: *pcDest++ = (char)c; /* fall-through */ /* FALLTHRU */ \
//             case 26: *pcDest++ = (char)c; /* fall-through */ /* FALLTHRU */ \
//             case 25: *pcDest++ = (char)c; /* fall-through */ /* FALLTHRU */ \
//             case 24: *pcDest++ = (char)c; /* fall-through */ /* FALLTHRU */ \
//             case 23: *pcDest++ = (char)c; /* fall-through */ /* FALLTHRU */ \
//             case 22: *pcDest++ = (char)c; /* fall-through */ /* FALLTHRU */ \
//             case 21: *pcDest++ = (char)c; /* fall-through */ /* FALLTHRU */ \
//             case 20: *pcDest++ = (char)c; /* fall-through */ /* FALLTHRU */ \
//             case 19: *pcDest++ = (char)c; /* fall-through */ /* FALLTHRU */ \
//             case 18: *pcDest++ = (char)c; /* fall-through */ /* FALLTHRU */ \
//             case 17: *pcDest++ = (char)c; /* fall-through */ /* FALLTHRU */ \
//             case 16: *pcDest++ = (char)c; /* fall-through */ /* FALLTHRU */ \
//             case 15: *pcDest++ = (char)c; /* fall-through */ /* FALLTHRU */ \
//             case 14: *pcDest++ = (char)c; /* fall-through */ /* FALLTHRU */ \
//             case 13: *pcDest++ = (char)c; /* fall-through */ /* FALLTHRU */ \
//             case 12: *pcDest++ = (char)c; /* fall-through */ /* FALLTHRU */ \
//             case 11: *pcDest++ = (char)c; /* fall-through */ /* FALLTHRU */ \
//             case 10: *pcDest++ = (char)c; /* fall-through */ /* FALLTHRU */ \
//             case 9:  *pcDest++ = (char)c; /* fall-through */ /* FALLTHRU */ \
//             case 8:  *pcDest++ = (char)c; /* fall-through */ /* FALLTHRU */ \
//             case 7:  *pcDest++ = (char)c; /* fall-through */ /* FALLTHRU */ \
//             case 6:  *pcDest++ = (char)c; /* fall-through */ /* FALLTHRU */ \
//             case 5:  *pcDest++ = (char)c; /* fall-through */ /* FALLTHRU */ \
//             case 4:  *pcDest++ = (char)c; /* fall-through */ /* FALLTHRU */ \
//             case 3:  *pcDest++ = (char)c; /* fall-through */ /* FALLTHRU */ \
//             case 2:  *pcDest++ = (char)c; /* fall-through */ /* FALLTHRU */ \
//             case 1:  *pcDest++ = (char)c; /* fall-through */ /* FALLTHRU */ \
//             default:break; \
//         } \
//     } SECUREC_WHILE_ZERO
// /**同理memcpy_s的结构体拷贝方法，只是这里src从memcpy方法中的自定义数据变成了写死的0或者1，
//  * 若要用自定义数据使用SECUREC_UNALIGNED_SET 宏 */
// #define SECUREC_ALIGNED_SET_OPT_ZERO_FF do { \
//         switch (c) { \
//             case 0: \
//                 switch (count) { \
//                     SECUREC_COPY_STRUCT_4_ZERO(1) \
//                     SECUREC_COPY_STRUCT_4_ZERO(2) \
//                     SECUREC_COPY_STRUCT_4_ZERO(3) \
//                     SECUREC_COPY_STRUCT_4_ZERO(4) \
//                     SECUREC_COPY_STRUCT_4_ZERO(5) \
//                     SECUREC_COPY_STRUCT_4_ZERO(6) \
//                     SECUREC_COPY_STRUCT_4_ZERO(7) \
//                     SECUREC_COPY_STRUCT_4_ZERO(8) \
//                     SECUREC_COPY_STRUCT_4_ZERO(9) \
//                     SECUREC_COPY_STRUCT_4_ZERO(10) \
//                     SECUREC_COPY_STRUCT_4_ZERO(11) \
//                     SECUREC_COPY_STRUCT_4_ZERO(12) \
//                     SECUREC_COPY_STRUCT_4_ZERO(13) \
//                     SECUREC_COPY_STRUCT_4_ZERO(14) \
//                     SECUREC_COPY_STRUCT_4_ZERO(15) \
//                     SECUREC_COPY_STRUCT_4_ZERO(16) \
//                     SECUREC_COPY_STRUCT_4_ZERO(17) \
//                     SECUREC_COPY_STRUCT_4_ZERO(18) \
//                     SECUREC_COPY_STRUCT_4_ZERO(19) \
//                     SECUREC_COPY_STRUCT_4_ZERO(20) \
//                     SECUREC_COPY_STRUCT_4_ZERO(21) \
//                     SECUREC_COPY_STRUCT_4_ZERO(22) \
//                     SECUREC_COPY_STRUCT_4_ZERO(23) \
//                     SECUREC_COPY_STRUCT_4_ZERO(24) \
//                     SECUREC_COPY_STRUCT_4_ZERO(25) \
//                     SECUREC_COPY_STRUCT_4_ZERO(26) \
//                     SECUREC_COPY_STRUCT_4_ZERO(27) \
//                     SECUREC_COPY_STRUCT_4_ZERO(28) \
//                     SECUREC_COPY_STRUCT_4_ZERO(29) \
//                     SECUREC_COPY_STRUCT_4_ZERO(30) \
//                     SECUREC_COPY_STRUCT_4_ZERO(31) \
//                     SECUREC_COPY_STRUCT_4_ZERO(32) \
//                     default:break; \
//                 } \
//                 break; \
//             case 0xFF: \
//                 switch (count) { \
//                     SECUREC_COPY_STRUCT_4_FF(1) \
//                     SECUREC_COPY_STRUCT_4_FF(2) \
//                     SECUREC_COPY_STRUCT_4_FF(3) \
//                     SECUREC_COPY_STRUCT_4_FF(4) \
//                     SECUREC_COPY_STRUCT_4_FF(5) \
//                     SECUREC_COPY_STRUCT_4_FF(6) \
//                     SECUREC_COPY_STRUCT_4_FF(7) \
//                     SECUREC_COPY_STRUCT_4_FF(8) \
//                     SECUREC_COPY_STRUCT_4_FF(9) \
//                     SECUREC_COPY_STRUCT_4_FF(10) \
//                     SECUREC_COPY_STRUCT_4_FF(11) \
//                     SECUREC_COPY_STRUCT_4_FF(12) \
//                     SECUREC_COPY_STRUCT_4_FF(13) \
//                     SECUREC_COPY_STRUCT_4_FF(14) \
//                     SECUREC_COPY_STRUCT_4_FF(15) \
//                     SECUREC_COPY_STRUCT_4_FF(16) \
//                     SECUREC_COPY_STRUCT_4_FF(17) \
//                     SECUREC_COPY_STRUCT_4_FF(18) \
//                     SECUREC_COPY_STRUCT_4_FF(19) \
//                     SECUREC_COPY_STRUCT_4_FF(20) \
//                     SECUREC_COPY_STRUCT_4_FF(21) \
//                     SECUREC_COPY_STRUCT_4_FF(22) \
//                     SECUREC_COPY_STRUCT_4_FF(23) \
//                     SECUREC_COPY_STRUCT_4_FF(24) \
//                     SECUREC_COPY_STRUCT_4_FF(25) \
//                     SECUREC_COPY_STRUCT_4_FF(26) \
//                     SECUREC_COPY_STRUCT_4_FF(27) \
//                     SECUREC_COPY_STRUCT_4_FF(28) \
//                     SECUREC_COPY_STRUCT_4_FF(29) \
//                     SECUREC_COPY_STRUCT_4_FF(30) \
//                     SECUREC_COPY_STRUCT_4_FF(31) \
//                     SECUREC_COPY_STRUCT_4_FF(32) \
//                     default:break; \
//                 } \
//                 break; \
//             default: \
//                 SECUREC_UNALIGNED_SET; \
//         } /* END switch */ \
//     } SECUREC_WHILE_ZERO
// #endif
/*错误处理函数*/
static errno_t SecMemsetError(void *dest, size_t destMax, int c, size_t count)
{
    //目标缓冲区大小不合法
    if (destMax == 0 || destMax > SECUREC_MEM_MAX_LEN) {
        SECUREC_ERROR_INVALID_RANGE("memset_s");
        return ERANGE;
    }
    //目标缓冲区为null
    if (dest == NULL) {
        SECUREC_ERROR_INVALID_PARAMTER("memset_s");
        return EINVAL;
    }
    //设置数量大于目标缓冲区大小
    if (count > destMax) {
        (void)memset(dest, c, destMax); /*填充整个目标缓冲区为“c” */
        SECUREC_ERROR_INVALID_RANGE("memset_s");
        return ERANGE_AND_RESET;
    }
    return EOK;
}

#if SECUREC_WITH_PERFORMANCE_ADDONS || SECUREC_MEMSET_WITH_PERFORMANCE
static void SecDoMemsetOpt(void *dest, int c, size_t count)
{
    if (count > SECUREC_MEMSET_THRESHOLD_SIZE) {
#ifdef SECUREC_USE_ASM
        (void)memset_opt(dest, c, count);
#else
        (void)memset(dest, c, count);
#endif
    } else {
        if (SECUREC_ADDR_ALIGNED_8(dest)) {
            /* use struct assignment */
            SECUREC_ALIGNED_SET_OPT_ZERO_FF;
        } else {
            SECUREC_UNALIGNED_SET;
        }
    }
    return;
}
#endif

/*******************************************************************************
 * <功能描述>
 *  将值c（转换为unsigned char）复制到dest指向对象的前count个字符中
 *
 * <输入参数>
 *    dest                           目标缓冲区
 *    destMax                        目标缓冲区大小
 *    c                              要设置的值
 *    count                          要设置的数量
 *
 * <输出参数>
 *    dest buffer                    更新后的目标缓冲区
 *
 * <返回值>
 *    EOK                           成功
 *    EINVAL                        非法参数：目标指针为空且缓冲区大小有效且不超过限制
 *    ERANGE                        其他参数都正常但缓冲区大小为空或超出限制
 *    ERANGE_AND_RESET              其他参数都正常，但设置的数量大于目标缓冲区大小
 *
 *    当返回ERANGE_AND_RESET时，会用c填满整个destMax区域,填充长度为destMax
 *******************************************************************************
 */
errno_t memset_s(void *dest, size_t destMax, int c, size_t count)
{
    if (SECUREC_LIKELY(count <= destMax && dest != NULL && destMax <= SECUREC_MEM_MAX_LEN)) {
#if SECUREC_MEMSET_WITH_PERFORMANCE  
   /*若开启高性能复制*/
        SecDoMemsetOpt(dest, c, count);
#else
#ifdef SECUREC_USE_ASM
        (void)memset_opt(dest, c, count);
#else
        (void)memset(dest, c, count);
#endif
#endif
        return EOK;
    } else {
        /* 错误处理函数 */
        return SecMemsetError(dest, destMax, c, count);
    }
}

#if SECUREC_IN_KERNEL
EXPORT_SYMBOL(memset_s);
#endif

#if SECUREC_WITH_PERFORMANCE_ADDONS

errno_t memset_sOptAsm(void *dest, size_t destMax, int c, size_t count)
{
    if (SECUREC_LIKELY(count <= destMax && dest != NULL && destMax <= SECUREC_MEM_MAX_LEN)) {
        SecDoMemsetOpt(dest, c, count);
        return EOK;
    }

    /* meet some runtime violation, return error code */
    return SecMemsetError(dest, destMax, c, count);
}

errno_t memset_sOptTc(void *dest, size_t destMax, int c, size_t count)
{
    if (SECUREC_LIKELY(count <= destMax && dest != NULL)) {
        SecDoMemsetOpt(dest, c, count);
        return EOK;
    }
    /* meet some runtime violation, return error code */
    return SecMemsetError(dest, destMax, c, count);

}
#endif

