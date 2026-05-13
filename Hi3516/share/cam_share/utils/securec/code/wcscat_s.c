
#include "securecutil.h"

static errno_t SecDoWcscat(wchar_t *strDest, size_t destMax, const wchar_t *strSrc)
{
    wchar_t *tmpDest = strDest;
    const wchar_t *tmpSrc = strSrc;
    size_t availableSize = destMax;
    SECUREC_IN_REGISTER const wchar_t *overlapGuard = NULL;

    if (tmpDest < tmpSrc) {
        overlapGuard = tmpSrc;
        while (availableSize > 0 && *tmpDest != '\0') {
            if (tmpDest == overlapGuard) {
                strDest[0] = '\0';
                SECUREC_ERROR_BUFFER_OVERLAP("wcscat_s");
                return EOVERLAP_AND_RESET;
            }
            /* seek to string end */
            ++tmpDest;
            --availableSize;
        }

        /* strDest unterminated, return error. */
        if (availableSize == 0) {
            strDest[0] = '\0';
            SECUREC_ERROR_INVALID_PARAMTER("wcscat_s");
            return EINVAL_AND_RESET;
        }

        /* if 2014-2018 > 0, then execute the strcat operation */
        while ((*tmpDest++ = *tmpSrc++) != '\0' && --availableSize > 0) {
            if (tmpDest == overlapGuard) {
                strDest[0] = '\0';
                SECUREC_ERROR_BUFFER_OVERLAP("wcscat_s");
                return EOVERLAP_AND_RESET;
            }
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
            SECUREC_ERROR_INVALID_PARAMTER("wcscat_s");
            return EINVAL_AND_RESET;
        }
        while ((*tmpDest++ = *tmpSrc++) != '\0' && --availableSize > 0) {
            if (tmpSrc == overlapGuard) {
                strDest[0] = '\0';
                SECUREC_ERROR_BUFFER_OVERLAP("wcscat_s");
                return EOVERLAP_AND_RESET;
            }
        }
    }

    /* strDest have not enough space, return error */
    if (availableSize == 0) {
        strDest[0] = '\0';
        SECUREC_ERROR_INVALID_RANGE("wcscat_s");
        return ERANGE_AND_RESET;
    }
    return EOK;
}

/*******************************************************************************
 * <功能描述>
 *wstrcat_s函数将 strSrc指向的字符串（包括终止空字符 \0）追加到 strDest指向的字符串末尾。
 和strcat_s一样，但是支持宽字符
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
errno_t wcscat_s(wchar_t *strDest, size_t destMax, const wchar_t *strSrc)
{
    if (destMax == 0 || destMax > SECUREC_WCHAR_STRING_MAX_LEN) {
        SECUREC_ERROR_INVALID_RANGE("wcscat_s");
        return ERANGE;
    }

    if (strDest == NULL || strSrc == NULL) {
        SECUREC_ERROR_INVALID_PARAMTER("wcscat_s");
        if (strDest != NULL) {
            strDest[0] = '\0';
            return EINVAL_AND_RESET;
        }
        return EINVAL;
    }

    return SecDoWcscat(strDest, destMax, strSrc);
}


