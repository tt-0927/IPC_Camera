#include "securecutil.h"

#ifdef SECUREC_NOT_CALL_LIBC_CORE_API
/*效果和memove一样，只是用c代码底层实现memove的逻辑*/
static void SecUtilMemmove(void *dst, const void *src, size_t count)
{
    unsigned char *pDest = (unsigned char *)dst;
    const unsigned char *pSrc = (const unsigned char *)src;
    size_t maxCount = count;

    if (dst <= src || pDest >= (pSrc + maxCount)) {
        /*
         * /* 非重叠区域：从低地址向高地址复制 */
       
        while (maxCount--) {
            *pDest = *pSrc;
            ++pDest;
            ++pSrc;
        }
    } else {
       /* 重叠区域：从高地址向低地址复制 */
        pDest = pDest + maxCount - 1;   //从末尾开始
        pSrc = pSrc + maxCount - 1;

        while (maxCount--) {
            *pDest = *pSrc;

            --pDest;
            --pSrc;
        }
    }
}
#endif

/*******************************************************************************
<功能描述>
memove_s 函数将 src 指向对象的 n 个字符复制到 dest 指向的对象中（和memcpy_s一样但是支持重叠区域）。

<输入参数>
dest         目标缓冲区
destMax      目标缓冲区大小
src          源缓冲区
count        要复制的字符数

<输出参数>
dest         目标缓冲区

<返回值>
EOK          成功
EINVAL       目标缓冲区 为 NULL 且 destMax != 0 且 destMax ≤ SECUREC_MEM_MAX_LEN（当destMax=0的时候，表示不拷贝到目标缓冲区，所以目标缓冲区可以为null）
EINVAL       目标缓冲区不为null且缓冲区大小不超出限制，但源缓冲区为null
ESRANGE      目标和源缓冲区大小都不为null，目标缓冲区大小超出限制或等于0
ERANGE       目标和源缓冲区大小都不为null，且目标缓冲区大小不超出限制，但要复制的数量 count >  dest 目标缓冲区
             * 如果发生错误，dest 将被填充为 0。            
 *******************************************************************************

/*同理memcpy，只是和错误处理函数合并了*/
errno_t memmove_s(void *dest, size_t destMax, const void *src, size_t count)
{
    if (destMax == 0 || destMax > SECUREC_MEM_MAX_LEN) {
        SECUREC_ERROR_INVALID_RANGE("memmove_s");
        return ERANGE;
    }
    if (dest == NULL || src == NULL) {
        SECUREC_ERROR_INVALID_PARAMTER("memmove_s");
        if (dest != NULL) {
            (void)memset(dest, 0, destMax);
            return EINVAL_AND_RESET;
        }
        return EINVAL;
    }
    if (count > destMax) {
        (void)memset(dest, 0, destMax);
        SECUREC_ERROR_INVALID_RANGE("memmove_s");
        return ERANGE_AND_RESET;
    }
    if (dest == src) {
        return EOK;
    }

    if (count > 0) {
#ifdef SECUREC_NOT_CALL_LIBC_CORE_API
        SecUtilMemmove(dest, src, count);
#else
        /* use underlying memmove for performance consideration */
        (void)memmove(dest, src, count);
#endif
    }
    return EOK;
}

#if SECUREC_IN_KERNEL
EXPORT_SYMBOL(memmove_s);
#endif

