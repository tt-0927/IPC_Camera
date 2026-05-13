
#include "securecutil.h"
#include "strcat_s.h"
/*编译时检测系统是否提供 strnlen函数*/
#if SECUREC_HAVE_STRNLEN
// #define SECUREC_STRCAT_LEN_THRESHOLD 8
// /*长度计算宏 ,在不超过 maxLen的前提下返回 str的实际长度。小于8的部分直接通过指针偏移检查 \0，快速返回长度（0~8）减少strlen函数调用开销*/
// #define SECUREC_CALC_STR_LEN(str,maxLen, len) do { \
//             if (*((str) + 0) == '\0') { \
//                 len = 0; \
//             } else if (*((str) + 1) == '\0') { \
//                 len = 1; \
//             } else if (*((str) + 2) == '\0') { \
//                 len = 2; \
//             } else if (*((str) + 3) == '\0') { \
//                 len = 3; \
//             } else if (*((str) + 4) == '\0') { \
//                 len = 4; \
//             } else if (*((str) + 5) == '\0') { \
//                 len = 5; \
//             } else if (*((str) + 6) == '\0') { \
//                 len = 6; \
//             } else if (*((str) + 7) == '\0') { \
//                 len = 7; \
//             } else if (*((str) + 8) == '\0') { \
//                 /* Optimization with a length of 8 */ \
//                 len = 8; \
//             } else { \
//                 /* The offset is 8 because the performance of 8 byte alignment is high */ \
//                 len = SECUREC_STRCAT_LEN_THRESHOLD + \
//                       strnlen((str) + SECUREC_STRCAT_LEN_THRESHOLD, \
//                       (maxLen) - SECUREC_STRCAT_LEN_THRESHOLD); \
//             } \
//         } SECUREC_WHILE_ZERO

// /* The function compiler will be inlined and not placed in other files */
static size_t SecMinStrLenOpt(const char *str, size_t maxLen)
{
    size_t len;
    if (maxLen > SECUREC_STRCAT_LEN_THRESHOLD) {
        /* Just to reduce the code complexity */
        SECUREC_CALC_STR_LEN(str, maxLen, len);
    } else {
        const char *strEnd = str;
        len = 0;
        /* use count  as boundary checker */
        while (len < maxLen && *(strEnd) != '\0') {
            ++strEnd;
            ++len;              /* no ending terminator */
        }
    }
    return len;
}

static errno_t SecDoStrcat(char *strDest, size_t destMax, const char *strSrc)
{
    size_t destLen = strnlen(strDest, destMax);
    /* Only optimize strSrc, do not apply this function to strDest */
    size_t srcLen = SecMinStrLenOpt(strSrc, destMax - destLen);

    if ((strDest < strSrc && strDest + destLen + srcLen >= strSrc) || \
        (strSrc < strDest && strSrc + srcLen >= strDest)) {
        strDest[0] = '\0';
        if (strDest + destLen <= strSrc && destLen == destMax) {
            SECUREC_ERROR_INVALID_PARAMTER("strcat_s");
            return EINVAL_AND_RESET;
        }
        SECUREC_ERROR_BUFFER_OVERLAP("strcat_s");
        return EOVERLAP_AND_RESET;
    }
    if (srcLen + destLen >= destMax || strDest == strSrc) {
        strDest[0] = '\0';
        if (destLen == destMax) {
            SECUREC_ERROR_INVALID_PARAMTER("strcat_s");
            return EINVAL_AND_RESET;
        }
        SECUREC_ERROR_INVALID_RANGE("strcat_s");
        return ERANGE_AND_RESET;
    }
    (void)memcpy(strDest + destLen, strSrc, srcLen + 1);    /* copy terminator */
    return EOK;
}
#else
static errno_t SecDoStrcat(char *strDest, size_t destMax, const char *strSrc)
{
    char *tmpDest = strDest;
    const char *tmpSrc = strSrc;
    //剩余可用空间计数器（单位：字节）。
    size_t availableSize = destMax;
    /*标记重叠检测的边界地址*/
    SECUREC_IN_REGISTER const char *overlapGuard = NULL;

    if (tmpDest < tmpSrc) {
        overlapGuard = tmpSrc;
        while (availableSize > 0 && *tmpDest != 0) {
            /*检测到重叠*/
            if (tmpDest == overlapGuard) {
                strDest[0] = '\0';
                SECUREC_ERROR_BUFFER_OVERLAP("strcat_s");
                return EOVERLAP_AND_RESET;
            }
            /* seek to string end */
            ++tmpDest;
            --availableSize;
        }

        /*未找到终止符，若 strDest​​无终止符​​，循环会持续直到 availableSize递减至 0*/
        if (availableSize == 0) {
            strDest[0] = '\0';
            SECUREC_ERROR_INVALID_PARAMTER("strcat_s");
            return EINVAL_AND_RESET;
        }
        /*前面的while里已经找到了终止符，此时tmpDest指向目标的末尾，tmpSrc指向原的开头，*/
        /**tmpDest++ = *tmpSrc++即为追加src到dest*/
        while ((*tmpDest++ = *tmpSrc++) != 0 && --availableSize > 0) {
            if (tmpDest == overlapGuard) {
                strDest[0] = '\0';
                SECUREC_ERROR_BUFFER_OVERLAP("strcat_s");
                return EOVERLAP_AND_RESET;
            }
        }
    } else {
        /*和上面同理*/
        overlapGuard = tmpDest;
        while (availableSize > 0 && *tmpDest != '\0') {
            /* seek to string end, and no need to check overlap */
            ++tmpDest;
            --availableSize;
        }

        /* strDest unterminated, return error. */
        if (availableSize == 0) {
            strDest[0] = '\0';
            SECUREC_ERROR_INVALID_PARAMTER("strcat_s");
            return EINVAL_AND_RESET;
        }
        while ((*tmpDest++ = *tmpSrc++) != '\0' && --availableSize > 0) {
            if (tmpSrc == overlapGuard) {
                strDest[0] = '\0';
                SECUREC_ERROR_BUFFER_OVERLAP("strcat_s");
                return EOVERLAP_AND_RESET;
            }
        }
    }
    /*目标缓冲区剩余空间不足*/
    if (availableSize == 0) {
        strDest[0] = '\0';
        SECUREC_ERROR_INVALID_RANGE("strcat_s");
        return ERANGE_AND_RESET;
    }
    return EOK;
}
#endif
/*******************************************************************************
 * <功能描述>
 *strcat_s函数将 strSrc指向的字符串（包括终止空字符 \0）追加到 strDest指向的字符串末尾。
 *strSrc的首字符会覆盖 strDest原有的终止符 \0。
 *若源字符串与目标缓冲区内存重叠，函数返回 EOVERLAP_AND_RESET。
 *注意：第二个参数 destMax是目标缓冲区的总容量（而非剩余可用空间）。
 *
 * <输入参数>
 *    strDest             strDest以 \0结尾的目标字符串缓冲区。
 *    destMax             destMax目标缓冲区的大小（单位：字节）。
 *    strSrc              strSrc以 \0结尾的源字符串缓冲区。
 *
 * <输出参数>
 *    ...                   strDest更新后的目标字符串。
 *
 * <RETURN VALUE>
 *    EOK                 Success
 *    EINVAL              源缓冲区为null，或源缓冲区和目标缓冲区都为null
 *    EINVAL_AND_RESET    目标字符串 strDest​​未正确终止​​（即不以 \0结尾）
 *    EINVAL_AND_RESET    原缓冲区为null，目标缓冲区不为null
 *    ERANGE              目标缓冲区超出限制或者为0
 *    ERANGE_AND_RESET    其他参数均有效且为重叠，但目标缓冲区剩余空间不足
 *    EOVERLAP_AND_RESET   其他参数均有效，但原和目标缓冲区重叠
 *
 *    当返回值报错，且strDest非 NULL且destMax有效（0 < destMax ≤ SECUREC_STRING_MAX_LEN），会将 strDest[0]设置为 \0​​
 *******************************************************************************
 */
errno_t strcat_s(char *strDest, size_t destMax, const char *strSrc)
{
    //目标缓冲区大小不合法
    if (destMax == 0 || destMax > SECUREC_STRING_MAX_LEN) {
        SECUREC_ERROR_INVALID_RANGE("strcat_s");
        return ERANGE;
    }
    //原或目标缓冲区为null
    if (strDest == NULL || strSrc == NULL) {
        SECUREC_ERROR_INVALID_PARAMTER("strcat_s");
        if (strDest != NULL) {
            strDest[0] = '\0';
            return EINVAL_AND_RESET;
        }
        return EINVAL;
    }

    return SecDoStrcat(strDest, destMax, strSrc);
}

#if SECUREC_IN_KERNEL
EXPORT_SYMBOL(strcat_s);
#endif

