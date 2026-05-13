#include "securecutil.h"

#ifdef SECUREC_NOT_CALL_LIBC_CORE_API
/*效果和memove一样，只是用c代码底层实现memove的逻辑*/
static void SecUtilMemmove(void *dst, const void *src, size_t count);
#endif

errno_t memmove_s(void *dest, size_t destMax, const void *src, size_t count);