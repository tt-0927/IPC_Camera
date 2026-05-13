
#include "securec.h"

/*******************************************************************************
 * <功能描述>
 *   该函数将格式化后的字符（最多 count个）存入 strDest并自动追加空终止符
 *   每个参数按 format中的格式说明符进行转换输出
 *   格式化规则与 printf系列函数一致
 *   若源字符串与目标内存重叠，行为未定义
 *
 * <输入参数>
 *    strDest                 输出存储地址
 *    destMax                 输出缓冲区大小（字节数）
 *    count                   最大存储字符数（不含终止符）.
 *    format                  格式
 *    ...                     
 *
 * <输出参数>
 *    dest buffer                strDest内容被更新为格式化后的字符串
 *
 * <返回值>
 *  这些函数均返回实际写入的字符数（不包含终止符）
 *  返回-1表示发生错误（超出内存地址范围，格式不匹配等）。
 *  当strDest和destMax有效但发生约束违反时（格式不匹配或写入数据超出count），强制设置strDest[0] = '\0'
 *******************************************************************************
 */

 /*依赖count和destMax双重限制*/
int snprintf_s(char *strDest, size_t destMax, size_t count, const char *format, ...)
{
    int ret;                    /* If initialization causes  e838 */
    va_list arglist;

    va_start(arglist, format);
#ifndef __MINGW32__
    ret = vsnprintf_s(strDest, destMax, count, format, arglist);
#else
    ret = vsnprintf_truncated_s(strDest, destMax, format, arglist);
#endif
    va_end(arglist);
    (void)arglist;              /* to clear e438 last value assigned not used , the compiler will optimize this code */

    return ret;
}

/**固定截断至destMax-1字符 */
#if SECUREC_SNPRINTF_TRUNCATED
int snprintf_truncated_s(char *strDest, size_t destMax, const char *format, ...)
{
    int ret;                    /* If initialization causes  e838 */
    va_list arglist;

    va_start(arglist, format);
    ret = vsnprintf_truncated_s(strDest, destMax, format, arglist);
    va_end(arglist);
    (void)arglist;              /* to clear e438 last value assigned not used , the compiler will optimize this code */

    return ret;
}
#endif


