
#include "securecutil.h"
#include "strcpy_s.h"

// #ifndef SECUREC_STRCOPY_THRESHOLD_SIZE
// #define SECUREC_STRCOPY_THRESHOLD_SIZE (32UL)
// #endif

// /* The purpose of converting to void is to clean up the alarm */
// #define SECUREC_STRCPY_BY_STRUCT(num) \
//         case num: \
//             *(SecStrBuf##num *)(void *)strDest = *(const SecStrBuf##num *)(const void *)strSrc; \
//             break;

// #define SECUREC_SMALL_STR_COPY do { \
//         if (SECUREC_ADDR_ALIGNED_8(strDest) && SECUREC_ADDR_ALIGNED_8(strSrc)) { \
//             /* use struct assignment */ \
//             switch (srcStrLen) { \
//                 SECUREC_STRCPY_BY_STRUCT(1) \
//                 SECUREC_STRCPY_BY_STRUCT(2) \
//                 SECUREC_STRCPY_BY_STRUCT(3) \
//                 SECUREC_STRCPY_BY_STRUCT(4) \
//                 SECUREC_STRCPY_BY_STRUCT(5) \
//                 SECUREC_STRCPY_BY_STRUCT(6) \
//                 SECUREC_STRCPY_BY_STRUCT(7) \
//                 SECUREC_STRCPY_BY_STRUCT(8) \
//                 SECUREC_STRCPY_BY_STRUCT(9) \
//                 SECUREC_STRCPY_BY_STRUCT(10) \
//                 SECUREC_STRCPY_BY_STRUCT(11) \
//                 SECUREC_STRCPY_BY_STRUCT(12) \
//                 SECUREC_STRCPY_BY_STRUCT(13) \
//                 SECUREC_STRCPY_BY_STRUCT(14) \
//                 SECUREC_STRCPY_BY_STRUCT(15) \
//                 SECUREC_STRCPY_BY_STRUCT(16) \
//                 SECUREC_STRCPY_BY_STRUCT(17) \
//                 SECUREC_STRCPY_BY_STRUCT(18) \
//                 SECUREC_STRCPY_BY_STRUCT(19) \
//                 SECUREC_STRCPY_BY_STRUCT(20) \
//                 SECUREC_STRCPY_BY_STRUCT(21) \
//                 SECUREC_STRCPY_BY_STRUCT(22) \
//                 SECUREC_STRCPY_BY_STRUCT(23) \
//                 SECUREC_STRCPY_BY_STRUCT(24) \
//                 SECUREC_STRCPY_BY_STRUCT(25) \
//                 SECUREC_STRCPY_BY_STRUCT(26) \
//                 SECUREC_STRCPY_BY_STRUCT(27) \
//                 SECUREC_STRCPY_BY_STRUCT(28) \
//                 SECUREC_STRCPY_BY_STRUCT(29) \
//                 SECUREC_STRCPY_BY_STRUCT(30) \
//                 SECUREC_STRCPY_BY_STRUCT(31) \
//                 SECUREC_STRCPY_BY_STRUCT(32) \
//                 default:break; \
//             } /* END switch */ \
//         }  else { \
//             char *tmpStrDest = (char *)strDest; \
//             const char *tmpStrSrc =  (const char *)strSrc; \
//             switch (srcStrLen) { \
//                 case 32: *tmpStrDest++ = *tmpStrSrc++; /* fall-through */ /* FALLTHRU */ \
//                 case 31: *tmpStrDest++ = *tmpStrSrc++; /* fall-through */ /* FALLTHRU */ \
//                 case 30: *tmpStrDest++ = *tmpStrSrc++; /* fall-through */ /* FALLTHRU */ \
//                 case 29: *tmpStrDest++ = *tmpStrSrc++; /* fall-through */ /* FALLTHRU */ \
//                 case 28: *tmpStrDest++ = *tmpStrSrc++; /* fall-through */ /* FALLTHRU */ \
//                 case 27: *tmpStrDest++ = *tmpStrSrc++; /* fall-through */ /* FALLTHRU */ \
//                 case 26: *tmpStrDest++ = *tmpStrSrc++; /* fall-through */ /* FALLTHRU */ \
//                 case 25: *tmpStrDest++ = *tmpStrSrc++; /* fall-through */ /* FALLTHRU */ \
//                 case 24: *tmpStrDest++ = *tmpStrSrc++; /* fall-through */ /* FALLTHRU */ \
//                 case 23: *tmpStrDest++ = *tmpStrSrc++; /* fall-through */ /* FALLTHRU */ \
//                 case 22: *tmpStrDest++ = *tmpStrSrc++; /* fall-through */ /* FALLTHRU */ \
//                 case 21: *tmpStrDest++ = *tmpStrSrc++; /* fall-through */ /* FALLTHRU */ \
//                 case 20: *tmpStrDest++ = *tmpStrSrc++; /* fall-through */ /* FALLTHRU */ \
//                 case 19: *tmpStrDest++ = *tmpStrSrc++; /* fall-through */ /* FALLTHRU */ \
//                 case 18: *tmpStrDest++ = *tmpStrSrc++; /* fall-through */ /* FALLTHRU */ \
//                 case 17: *tmpStrDest++ = *tmpStrSrc++; /* fall-through */ /* FALLTHRU */ \
//                 case 16: *tmpStrDest++ = *tmpStrSrc++; /* fall-through */ /* FALLTHRU */ \
//                 case 15: *tmpStrDest++ = *tmpStrSrc++; /* fall-through */ /* FALLTHRU */ \
//                 case 14: *tmpStrDest++ = *tmpStrSrc++; /* fall-through */ /* FALLTHRU */ \
//                 case 13: *tmpStrDest++ = *tmpStrSrc++; /* fall-through */ /* FALLTHRU */ \
//                 case 12: *tmpStrDest++ = *tmpStrSrc++; /* fall-through */ /* FALLTHRU */ \
//                 case 11: *tmpStrDest++ = *tmpStrSrc++; /* fall-through */ /* FALLTHRU */ \
//                 case 10: *tmpStrDest++ = *tmpStrSrc++; /* fall-through */ /* FALLTHRU */ \
//                 case 9:  *tmpStrDest++ = *tmpStrSrc++; /* fall-through */ /* FALLTHRU */ \
//                 case 8:  *tmpStrDest++ = *tmpStrSrc++; /* fall-through */ /* FALLTHRU */ \
//                 case 7:  *tmpStrDest++ = *tmpStrSrc++; /* fall-through */ /* FALLTHRU */ \
//                 case 6:  *tmpStrDest++ = *tmpStrSrc++; /* fall-through */ /* FALLTHRU */ \
//                 case 5:  *tmpStrDest++ = *tmpStrSrc++; /* fall-through */ /* FALLTHRU */ \
//                 case 4:  *tmpStrDest++ = *tmpStrSrc++; /* fall-through */ /* FALLTHRU */ \
//                 case 3:  *tmpStrDest++ = *tmpStrSrc++; /* fall-through */ /* FALLTHRU */ \
//                 case 2:  *tmpStrDest++ = *tmpStrSrc++; /* fall-through */ /* FALLTHRU */ \
//                 case 1:  *tmpStrDest++ = *tmpStrSrc++; /* fall-through */ /* FALLTHRU */ \
//                 default:break; \
//             } \
//         } \
//     } SECUREC_WHILE_ZERO



/**********************************************************************
验证源字符串 (strSrc) 是否能在不溢出的情况下被复制到目标缓冲区 (strDest) 中，
核心保护逻辑:
模拟字符串拷贝过程，但不实际执行内存操作
提前预判是否会发生缓冲区溢出
若检测到危险，立即终止并清理现场
 **********************************************************************/
static errno_t CheckSrcRange(char *strDest, size_t destMax, const char *strSrc)
{
    size_t tmpDestMax = destMax;
    const char *tmpSrc = strSrc;
    /* use destMax as boundary checker and destMax must be greater than zero */
    while (*(tmpSrc) != '\0' && tmpDestMax > 0) {
        ++tmpSrc;
        --tmpDestMax;
    }
    if (tmpDestMax == 0) {
        strDest[0] = '\0';
        SECUREC_ERROR_INVALID_RANGE("strcpy_s");
        return ERANGE_AND_RESET;
    }
    return EOK;
}
/**错误处理函数 */
errno_t strcpy_error(char *strDest, size_t destMax, const char *strSrc)
{
    //目标缓冲区为0或超限
    if (destMax == 0 || destMax > SECUREC_STRING_MAX_LEN) {
        SECUREC_ERROR_INVALID_RANGE("strcpy_s");
        return ERANGE;
    } else if (strDest == NULL || strSrc == NULL) {
        SECUREC_ERROR_INVALID_PARAMTER("strcpy_s");
        //如果dest不为null，则清空dest
        if (strDest != NULL) {
            strDest[0] = '\0';
            return EINVAL_AND_RESET;
        }
        return EINVAL;
    }
    return CheckSrcRange(strDest, destMax, strSrc);
}

/*******************************************************************************
 /​​​​​​​​​​​​​​​​​​​​
<函数描述>
strncpy_s函数将 strSrc指向的字符串（包括终止空字符 \0）复制到 strDest指向的数组中。
目标字符串缓冲区必须足够大以容纳源字符串（含终止符）。若源与目标内存重叠，函数返回 EOVERLAP_AND_RESET。

<输入参数>
strDest                 目标字符串缓冲区地址
destMax                 目标缓冲区大小（单位：字节）
strSrc                  以 \0结尾的源字符串缓冲区

<输出参数>
strDest                 更新后的目标字符串

<返回值>
EOK                      成功
EINVAL                  strDest为 NULL且 destMax != 0且 destMax <= SECUREC_STRING_MAX_LEN
EINVAL_AND_RESET        strDest非 NULL但 strSrc为 NULL且 destMax不为0也不超出限制
ERANGE                  destMax为 0或 > SECUREC_STRING_MAX_LEN
ERANGE_AND_RESET        strDest空间不足（其他参数有效且无重叠）
EOVERLAP_AND_RESET      源与目标内存重叠（其他所有所有参数都有效）

若发生运行时约束冲突，当 strDest和 destMax有效时，strDest[0]将被设为 \0。
 *******************************************************************************/


errno_t strcpy_s(char *strDest, size_t destMax, const char *strSrc)
{
    if ((destMax > 0 && destMax <= SECUREC_STRING_MAX_LEN && strDest != NULL && strSrc != NULL && strDest != strSrc)) {
#if SECUREC_HAVE_STRNLEN
        size_t srcStrLen = strnlen(strSrc, destMax) + 1;    /* len  include \0 */
#else
        size_t srcStrLen = destMax; /* use it to store the max length limit */
        const char *endPos = strSrc;
        /*模拟拷贝，计算长度*/
        while (*(endPos) != '\0' && srcStrLen > 0) {    /* use srcStrLen as boundary checker */
            ++endPos;
            --srcStrLen;
        }
        /*实际需要的长度（加上终止符）*/
        srcStrLen = (size_t)(endPos - strSrc) + 1;  /* len  include \0 */
#endif
        /* le is  high performance to lt */
        if (srcStrLen <= destMax) {
            /*判断是否重叠*/
            if ((strDest < strSrc && strDest + srcStrLen <= strSrc) ||
                (strSrc < strDest && strSrc + srcStrLen <= strDest)) {
#if SECUREC_IN_KERNEL
                (void)memcpy(strDest, strSrc, srcStrLen);
#else           /*如果大于32UL则使用memcpy，否则使用小字节拷贝方式，具体方式类似memcpy_s的小字节拷贝*/
                if (srcStrLen > SECUREC_STRCOPY_THRESHOLD_SIZE) {
                    (void)memcpy(strDest, strSrc, srcStrLen);
                } else {
                    SECUREC_SMALL_STR_COPY;
                }
#endif
                return EOK;
            } else {
                /*重叠*/
                strDest[0] = '\0';
                SECUREC_ERROR_BUFFER_OVERLAP("strcpy_s");
                return EOVERLAP_AND_RESET;
            }

        }
    }
    return strcpy_error(strDest, destMax, strSrc);
}

#if SECUREC_IN_KERNEL
EXPORT_SYMBOL(strcpy_s);
#endif

