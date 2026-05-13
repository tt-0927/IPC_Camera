#include "securecutil.h"

static errno_t SecDoWcscat(wchar_t *strDest, size_t destMax, const wchar_t *strSrc);
errno_t wcscat_s(wchar_t *strDest, size_t destMax, const wchar_t *strSrc);