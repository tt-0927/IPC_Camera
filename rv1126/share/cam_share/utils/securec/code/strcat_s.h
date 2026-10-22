
#include "securecutil.h"

#include "securecutil.h"
/*编译时检测系统是否提供 strnlen函数*/
#if SECUREC_HAVE_STRNLEN
#define SECUREC_STRCAT_LEN_THRESHOLD 8
/*长度计算宏 ,在不超过 maxLen的前提下返回 str的实际长度。小于8的部分直接通过指针偏移检查 \0，快速返回长度（0~8）减少strlen函数调用开销*/
#define SECUREC_CALC_STR_LEN(str,maxLen, len) do { \
            if (*((str) + 0) == '\0') { \
                len = 0; \
            } else if (*((str) + 1) == '\0') { \
                len = 1; \
            } else if (*((str) + 2) == '\0') { \
                len = 2; \
            } else if (*((str) + 3) == '\0') { \
                len = 3; \
            } else if (*((str) + 4) == '\0') { \
                len = 4; \
            } else if (*((str) + 5) == '\0') { \
                len = 5; \
            } else if (*((str) + 6) == '\0') { \
                len = 6; \
            } else if (*((str) + 7) == '\0') { \
                len = 7; \
            } else if (*((str) + 8) == '\0') { \
                /* Optimization with a length of 8 */ \
                len = 8; \
            } else { \
                /* The offset is 8 because the performance of 8 byte alignment is high */ \
                len = SECUREC_STRCAT_LEN_THRESHOLD + \
                      strnlen((str) + SECUREC_STRCAT_LEN_THRESHOLD, \
                      (maxLen) - SECUREC_STRCAT_LEN_THRESHOLD); \
            } \
        } SECUREC_WHILE_ZERO

/* The function compiler will be inlined and not placed in other files */
static size_t SecMinStrLenOpt(const char *str, size_t maxLen);


static errno_t SecDoStrcat(char *strDest, size_t destMax, const char *strSrc);

#else

#endif
static errno_t SecDoStrcat(char *strDest, size_t destMax, const char *strSrc);
errno_t strcat_s(char *strDest, size_t destMax, const char *strSrc);