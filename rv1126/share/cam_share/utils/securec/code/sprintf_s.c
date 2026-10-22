
#include "securec.h"
//#include "vsprintf_s.h"


 /*******************************************************************************
 * <功能描述>
 *  sprintf_s功能与 sprintf等效，但增加了 destMax参数和运行时约束检查
 *  该函数将格式化后的字符存入 strDest，最多写入 destMax-1个字符并自动追加空终止符
 *  每个参数按 format中的格式说明符进行转换输出
 *  格式化规则与 printf系列函数一致
 *  若源字符串与目标内存重叠，行为未定义
 *
 * <输入参数>
 *    strDest                 输出存储地址
 *    destMax                 输出缓冲区大小（字节数）
 *    format                  格式
 *    ...                     
 *
 * <输出参数>
 *    dest buffer                strDest内容被更新为格式化后的字符串
 *
 * <返回值>
 *  这些函数均返回实际写入的字符数（不包含终止符）
 *  返回-1表示发生错误（超出内存地址范围，格式不匹配等）。
 *  当strDest和destMax有效但发生约束违反时（如缓冲区溢出）清空缓冲区
 *******************************************************************************
 */
#ifndef __MINGW32__
int sprintf_s(char *strDest, size_t destMax, const char *format, ...)
{
    int ret;                    /* If initialization causes  e838 */
    va_list arglist;

    va_start(arglist, format);
    ret = vsprintf_s(strDest, destMax, format, arglist);
    va_end(arglist);
    (void)arglist;              /* to clear e438 last value assigned not used , the compiler will optimize this code */

    return ret;
}
#endif

