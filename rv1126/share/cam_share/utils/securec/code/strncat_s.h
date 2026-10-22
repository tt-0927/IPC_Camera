
#include "securecutil.h"

#if SECUREC_HAVE_STRNLEN
/*编译时检测系统是否提供 strnlen函数*/

static errno_t SecDoStrncat(char *strDest, size_t destMax, const char *strSrc, size_t count);

#else

#endif
static errno_t SecDoStrncat(char *strDest, size_t destMax, const char *strSrc, size_t count);
errno_t strncat_s(char *strDest, size_t destMax, const char *strSrc, size_t count);