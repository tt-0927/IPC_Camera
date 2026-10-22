
#include "securecutil.h"

#ifndef SECUREC_STRCOPY_THRESHOLD_SIZE
#define SECUREC_STRCOPY_THRESHOLD_SIZE (32UL)
#endif

/* The purpose of converting to void is to clean up the alarm */
#define SECUREC_STRCPY_BY_STRUCT(num) \
        case num: \
            *(SecStrBuf##num *)(void *)strDest = *(const SecStrBuf##num *)(const void *)strSrc; \
            break;

#define SECUREC_SMALL_STR_COPY do { \
        if (SECUREC_ADDR_ALIGNED_8(strDest) && SECUREC_ADDR_ALIGNED_8(strSrc)) { \
            /* use struct assignment */ \
            switch (srcStrLen) { \
                SECUREC_STRCPY_BY_STRUCT(1) \
                SECUREC_STRCPY_BY_STRUCT(2) \
                SECUREC_STRCPY_BY_STRUCT(3) \
                SECUREC_STRCPY_BY_STRUCT(4) \
                SECUREC_STRCPY_BY_STRUCT(5) \
                SECUREC_STRCPY_BY_STRUCT(6) \
                SECUREC_STRCPY_BY_STRUCT(7) \
                SECUREC_STRCPY_BY_STRUCT(8) \
                SECUREC_STRCPY_BY_STRUCT(9) \
                SECUREC_STRCPY_BY_STRUCT(10) \
                SECUREC_STRCPY_BY_STRUCT(11) \
                SECUREC_STRCPY_BY_STRUCT(12) \
                SECUREC_STRCPY_BY_STRUCT(13) \
                SECUREC_STRCPY_BY_STRUCT(14) \
                SECUREC_STRCPY_BY_STRUCT(15) \
                SECUREC_STRCPY_BY_STRUCT(16) \
                SECUREC_STRCPY_BY_STRUCT(17) \
                SECUREC_STRCPY_BY_STRUCT(18) \
                SECUREC_STRCPY_BY_STRUCT(19) \
                SECUREC_STRCPY_BY_STRUCT(20) \
                SECUREC_STRCPY_BY_STRUCT(21) \
                SECUREC_STRCPY_BY_STRUCT(22) \
                SECUREC_STRCPY_BY_STRUCT(23) \
                SECUREC_STRCPY_BY_STRUCT(24) \
                SECUREC_STRCPY_BY_STRUCT(25) \
                SECUREC_STRCPY_BY_STRUCT(26) \
                SECUREC_STRCPY_BY_STRUCT(27) \
                SECUREC_STRCPY_BY_STRUCT(28) \
                SECUREC_STRCPY_BY_STRUCT(29) \
                SECUREC_STRCPY_BY_STRUCT(30) \
                SECUREC_STRCPY_BY_STRUCT(31) \
                SECUREC_STRCPY_BY_STRUCT(32) \
                default:break; \
            } /* END switch */ \
        }  else { \
            char *tmpStrDest = (char *)strDest; \
            const char *tmpStrSrc =  (const char *)strSrc; \
            switch (srcStrLen) { \
                case 32: *tmpStrDest++ = *tmpStrSrc++; /* fall-through */ /* FALLTHRU */ \
                case 31: *tmpStrDest++ = *tmpStrSrc++; /* fall-through */ /* FALLTHRU */ \
                case 30: *tmpStrDest++ = *tmpStrSrc++; /* fall-through */ /* FALLTHRU */ \
                case 29: *tmpStrDest++ = *tmpStrSrc++; /* fall-through */ /* FALLTHRU */ \
                case 28: *tmpStrDest++ = *tmpStrSrc++; /* fall-through */ /* FALLTHRU */ \
                case 27: *tmpStrDest++ = *tmpStrSrc++; /* fall-through */ /* FALLTHRU */ \
                case 26: *tmpStrDest++ = *tmpStrSrc++; /* fall-through */ /* FALLTHRU */ \
                case 25: *tmpStrDest++ = *tmpStrSrc++; /* fall-through */ /* FALLTHRU */ \
                case 24: *tmpStrDest++ = *tmpStrSrc++; /* fall-through */ /* FALLTHRU */ \
                case 23: *tmpStrDest++ = *tmpStrSrc++; /* fall-through */ /* FALLTHRU */ \
                case 22: *tmpStrDest++ = *tmpStrSrc++; /* fall-through */ /* FALLTHRU */ \
                case 21: *tmpStrDest++ = *tmpStrSrc++; /* fall-through */ /* FALLTHRU */ \
                case 20: *tmpStrDest++ = *tmpStrSrc++; /* fall-through */ /* FALLTHRU */ \
                case 19: *tmpStrDest++ = *tmpStrSrc++; /* fall-through */ /* FALLTHRU */ \
                case 18: *tmpStrDest++ = *tmpStrSrc++; /* fall-through */ /* FALLTHRU */ \
                case 17: *tmpStrDest++ = *tmpStrSrc++; /* fall-through */ /* FALLTHRU */ \
                case 16: *tmpStrDest++ = *tmpStrSrc++; /* fall-through */ /* FALLTHRU */ \
                case 15: *tmpStrDest++ = *tmpStrSrc++; /* fall-through */ /* FALLTHRU */ \
                case 14: *tmpStrDest++ = *tmpStrSrc++; /* fall-through */ /* FALLTHRU */ \
                case 13: *tmpStrDest++ = *tmpStrSrc++; /* fall-through */ /* FALLTHRU */ \
                case 12: *tmpStrDest++ = *tmpStrSrc++; /* fall-through */ /* FALLTHRU */ \
                case 11: *tmpStrDest++ = *tmpStrSrc++; /* fall-through */ /* FALLTHRU */ \
                case 10: *tmpStrDest++ = *tmpStrSrc++; /* fall-through */ /* FALLTHRU */ \
                case 9:  *tmpStrDest++ = *tmpStrSrc++; /* fall-through */ /* FALLTHRU */ \
                case 8:  *tmpStrDest++ = *tmpStrSrc++; /* fall-through */ /* FALLTHRU */ \
                case 7:  *tmpStrDest++ = *tmpStrSrc++; /* fall-through */ /* FALLTHRU */ \
                case 6:  *tmpStrDest++ = *tmpStrSrc++; /* fall-through */ /* FALLTHRU */ \
                case 5:  *tmpStrDest++ = *tmpStrSrc++; /* fall-through */ /* FALLTHRU */ \
                case 4:  *tmpStrDest++ = *tmpStrSrc++; /* fall-through */ /* FALLTHRU */ \
                case 3:  *tmpStrDest++ = *tmpStrSrc++; /* fall-through */ /* FALLTHRU */ \
                case 2:  *tmpStrDest++ = *tmpStrSrc++; /* fall-through */ /* FALLTHRU */ \
                case 1:  *tmpStrDest++ = *tmpStrSrc++; /* fall-through */ /* FALLTHRU */ \
                default:break; \
            } \
        } \
    } SECUREC_WHILE_ZERO

static errno_t CheckSrcRange(char *strDest, size_t destMax, const char *strSrc);


errno_t strcpy_error(char *strDest, size_t destMax, const char *strSrc);

errno_t strcpy_s(char *strDest, size_t destMax, const char *strSrc);