       安全函数                 功能描述                                                         使用方法            
--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
1.     memcpy_s               将src 指向对象的 n 个字符复制到 dest 指向的对象中（不可重叠）        memcpy_s(void *dest, size_t destMax, const void *src, size_t count);
2.     wmemcpy_s              同memcpy_s（宽字符版）                                            wmemcpy_s(wchar_t *dest, size_t destMax, const wchar_t *src, size_t count);
3.     memmove_s              将src 指向对象的 n 个字符复制到 dest 指向的对象中（可重叠版）        memmove_s(void *dest, size_t destMax, const void *src, size_t count);
4.     wmemmove_s             同memmove_s（宽字符版）                                           memmove_s(wchar_t *dest, size_t destMax, const wchar_t *src, size_t count);
5.     memset_s               填充count字节数据（int型）到目标缓冲区                              memset_s(void *dest, size_t destMax, int c, size_t count);
6.     gets_s                 从标准输入流（键盘）获取count个数据到目标缓冲区，并补上\0             gets_s(char *buffer, size_t numberOfElements);
7.     strcat_s               将strSrc指向的字符串（包括\0）追加到 strDest指向的字符串末尾         strcat_s(char *strDest, size_t destMax, const char *strSrc);
8.     wcscat_s               同strcat_s（宽字符版）                                            wstrcat_s(wchar_t *strDest, size_t destMax, const wchar_t *strSrc);
8.     strncat_s              同strcat_s，但是限制数量（count个连续字符）                         strncat_s(char *strDest, size_t destMax, const char *strSrc, size_t count)
9.     wcsncat_s              同strcat_s，但是限制数量（count个连续字符）且为宽字符版              wstrncat_s(wchar_t *strDest,size_t destMax, const wchar_ t*strSrc, size_t count)
10.    strcpy_s               将strSrc指向的字符串（包括\0）复制到 strDest指向的数组               strcpy_s(char *strDest, size_t destMax, const char *strSrc)
11.    wcscpy_s               同strcpy_s（宽字符版）                                             wcscpy_s(wchar_t *strDest, size_t destMax, const wchar_t *strSrc);
12.    strncpy_s              同strcpy_s，但是限制数量（count个连续字符）                         strncpy_s(char *strDest, size_t destMax, const char *strSrc, size_t count);
13.    strtok_s               将字符串中分隔符过滤掉，按分隔符分开输出字符串                        strtok_s(char *strToken, const char *strDelimit, char **context)
14.    wcstok_s               同strtok_s (宽字符版)                                              wcstok_s(wchar_t *strToken,const wchar_t *strDelimit, wchar_t **context);
15.    vsprintf_s             将格式化的字符串复制到目标缓冲区（需要先初始化va_list arglist         vsprintf_s(char *strDest,size_t destMax,const char *format,va_list arglist)     
16.    sprintf_s              将格式化的字符串复制到目标缓冲区（无需初始化va list）                 sprintf_s(char *strDest, size_t destMax, const char *format, ...)
17.    vsnprintf_s            同vsprintf_s，但是限制数量（count个连续字符）              vsnprintf_s(char *strDest,size_t destMax,size_t count,const char *format,va_list arglist)
18.    snprintf_s             同snprintf_s，但是限制数量（count个连续字符）                        snprintf_s(char *strDest,size_t destMax,size_t count,const char *format)
19.    vswprintf_s            同vsprintf_s（宽字符版）                                            用法同vsprintf_s，输入参数变成wchar_t类型即可
20.    swprintf_s             同sprintf_s  （宽字符版）                                           用法同sprintf_s，输入参数变成wchar_t类型即可