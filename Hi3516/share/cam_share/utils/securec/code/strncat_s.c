
#include "securecutil.h"

#if SECUREC_HAVE_STRNLEN
static errno_t SecDoStrncat(char *strDest, size_t destMax, const char *strSrc, size_t count)
{
    size_t destLen = strnlen(strDest, destMax);
    /* The strSrc is no longer optimized. The reason is that when count is small,
     * the efficiency of strnlen is higher than that of self realization.
     */
    size_t srcLen = strnlen(strSrc, count);

    if ((strDest < strSrc && strDest + destLen + srcLen >= strSrc) || \
        (strSrc < strDest && strSrc + srcLen >= strDest)) {
        strDest[0] = '\0';
        if (strDest + destLen <= strSrc && destLen == destMax) {
            SECUREC_ERROR_INVALID_PARAMTER("strncat_s");
            return EINVAL_AND_RESET;
        }
        SECUREC_ERROR_BUFFER_OVERLAP("strncat_s");
        return EOVERLAP_AND_RESET;
    }
    if (srcLen + destLen >= destMax || strDest == strSrc) {
        strDest[0] = '\0';
        if (destLen == destMax) {
            SECUREC_ERROR_INVALID_PARAMTER("strncat_s");
            return EINVAL_AND_RESET;
        }
        SECUREC_ERROR_INVALID_RANGE("strncat_s");
        return ERANGE_AND_RESET;
    }
    (void)memcpy(strDest + destLen, strSrc, srcLen);    /* no  terminator */
    *(strDest + destLen + srcLen) = '\0';
    return EOK;
}
#else
static errno_t SecDoStrncat(char *strDest, size_t destMax, const char *strSrc, size_t count)
{
    char *tmpDest = strDest;
    const char *tmpSrc = strSrc;
    size_t availableSize = destMax;
    size_t maxCount = count;
    SECUREC_IN_REGISTER const char *overlapGuard = NULL;

    if (tmpDest < tmpSrc) {
        overlapGuard = tmpSrc;
        while (availableSize > 0 && *tmpDest != '\0') {
            /*重叠检测*/
            if (tmpDest == overlapGuard) {
                strDest[0] = '\0';
                SECUREC_ERROR_BUFFER_OVERLAP("strncat_s");
                return EOVERLAP_AND_RESET;
            }
            ++tmpDest;
            --availableSize;
        }

        /* strDestination unterminated, return error. */
        /*没找到终止符*/
        if (availableSize == 0) {
            strDest[0] = '\0';
            SECUREC_ERROR_INVALID_PARAMTER("strncat_s");
            return EINVAL_AND_RESET;
        }
        while (maxCount > 0 && (*tmpDest++ = *tmpSrc++) != '\0' && --availableSize > 0) {
            if (tmpDest == overlapGuard) {
                strDest[0] = '\0';
                SECUREC_ERROR_BUFFER_OVERLAP("strncat_s");
                return EOVERLAP_AND_RESET;
            }
            --maxCount;
        }
    } else {
        overlapGuard = tmpDest;
        while (availableSize > 0 && *tmpDest != '\0') {
            /* seek to string end, and no need to check overlap */
            ++tmpDest;
            --availableSize;
        }

        /* strDest unterminated, return error. */
        if (availableSize == 0) {
            strDest[0] = '\0';
            SECUREC_ERROR_INVALID_PARAMTER("strncat_s");
            return EINVAL_AND_RESET;
        }
        while (maxCount > 0 && (*tmpDest++ = *tmpSrc++) != '\0' && --availableSize > 0) {
            if (tmpSrc == overlapGuard) {
                strDest[0] = '\0';
                SECUREC_ERROR_BUFFER_OVERLAP("strncat_s");
                return EOVERLAP_AND_RESET;
            }
            --maxCount;
        }
    }
    if (maxCount == 0) {
        *tmpDest = 0;           /* add terminator to strDest */
    }

    /* strDest have not enough space,return error */
    if (availableSize == 0) {
        strDest[0] = '\0';
        SECUREC_ERROR_INVALID_RANGE("strncat_s");
        return ERANGE_AND_RESET;
    }

    return EOK;
}
#endif

/*******************************************************************************
 * <功能描述>
 *   strncat_s函数将strSrc指向的数组中最多n个连续字符（不包含终止空字符）追加到strDest指向的字符串末尾。
 *   该函数尝试追加strSrc的前D个字符到strDest末尾，D是count和strSrc长度的较小值。
 *   若追加D个字符后：  
 *   1. 能容纳在strDest（大小为destMax）且仍有空间存放终止符，则从strDest原终止符位置开始追加，并添加新终止符；
 *   2. 否则将strDest[0]设为空字符。
 *
 * <输入参数>
 *    strDest                以空字符结尾的目标字符串
 *    destMax                目标缓冲区大小
 *    strSrc                 以空字符结尾的源字符串
 *    count                  要追加的字符数（可截断）
 *
 * <输出参数>
 *    strDest                更新后的目标缓冲区
 *
 * <RETURN VALUE>
 *    EOK                 Success
 *    EINVAL              源缓冲区为null，或源缓冲区和目标缓冲区都为null
 *    EINVAL_AND_RESET    目标字符串 strDest​​未正确终止​​（即不以 \0结尾）
 *    EINVAL_AND_RESET    原缓冲区为null，目标缓冲区不为null
 *    ERANGE              目标缓冲区超出限制或者为0
 *    ERANGE_AND_RESET    其他参数均有效且为重叠，但目标缓冲区剩余空间不足
 *    EOVERLAP_AND_RESET   其他参数均有效，但原和目标缓冲区重叠
   
     若发生运行时约束违规，当strDest和destMax有效时，strDest[0]将被设为'\0'
 ********************************************************************************
 */
errno_t strncat_s(char *strDest, size_t destMax, const char *strSrc, size_t count)
{
    if (destMax == 0 || destMax > SECUREC_STRING_MAX_LEN) {
        SECUREC_ERROR_INVALID_RANGE("strncat_s");
        return ERANGE;
    }

    if (strDest == NULL || strSrc == NULL) {
        SECUREC_ERROR_INVALID_PARAMTER("strncat_s");
        if (strDest != NULL) {
            strDest[0] = '\0';
            return EINVAL_AND_RESET;
        }
        return EINVAL;
    }
#ifdef  SECUREC_COMPATIBLE_WIN_FORMAT
    if (count > SECUREC_STRING_MAX_LEN && count != (size_t)-1) {
        strDest[0] = '\0';
        SECUREC_ERROR_INVALID_RANGE("strncat_s");
        return ERANGE_AND_RESET;
    }
#else
    if (count > SECUREC_STRING_MAX_LEN) {
        strDest[0] = '\0';
        SECUREC_ERROR_INVALID_RANGE("strncat_s");
        return ERANGE_AND_RESET;
    }
#endif

    return SecDoStrncat(strDest, destMax, strSrc, count);
}

#if SECUREC_IN_KERNEL
EXPORT_SYMBOL(strncat_s);
#endif

