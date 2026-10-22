
#include "securecutil.h"


  /*******************************************************************************
 * <功能描述>
 *  vsprintf_s功能与 vsprintf等效，
 *  该函数通过参数列表指针 (arglist) 将格式化数据写入 strDest指向的内存，安全版本独有特性是支持位置参数。
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
 *******************************************************************************/
#ifndef __MINGW32__
int vsprintf_s(char *strDest, size_t destMax, const char *format, va_list arglist)
{
    int retVal;               /* If initialization causes  e838 */

    if (format == NULL || strDest == NULL || destMax == 0 || destMax > SECUREC_STRING_MAX_LEN) {
        if (strDest != NULL && destMax > 0) {
            strDest[0] = '\0';
        }
        SECUREC_ERROR_INVALID_PARAMTER("vsprintf_s");
        return -1;
    }

    retVal = SecVsnprintfImpl(strDest, destMax, format, arglist);

    if (retVal < 0) {
        strDest[0] = '\0';
        if (retVal == SECUREC_PRINTF_TRUNCATE) {
            /* Buffer is too small */
            SECUREC_ERROR_INVALID_RANGE("vsprintf_s");
        }
        SECUREC_ERROR_INVALID_PARAMTER("vsprintf_s");
        return -1;
    }

    return retVal;
}
#endif

