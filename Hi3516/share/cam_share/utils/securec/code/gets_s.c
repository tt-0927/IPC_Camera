#include "securecutil.h"

/*********************************************************************************
 * <函数描述>
 * gets_s 函数从标准输入流（stdin）读取最多不超过 bufferSize-1 的字符，
 * 存储到 buffer 指向的数组中。读取的行包含直到第一个换行符（'\n'）的所有字符。
 * gets_s 会将换行符替换为空字符（'\0'）后返回该行。
 * 如果读取的第一个字符是文件结束符（EOF），则在 buffer 开头存储 '\0' 并返回 NULL。
 *
 * <输入参数>
 * buffer：                           存储输入字符串的数组地址。
 * numberOfElements：                 缓冲区的大小。
 *
 * <输出参数>
 * buffer：                           读取后的数组地址。
 *
 * <返回值>
 * buffer：                           返回缓冲区地址，操作成功。
 * NULL：                             参数非法或读取失败。
 ****************************************************************************/


 /***从字符串末尾逆向删除回车符（\r）和换行符（\n），并将其替换为终止符 \0。 */
static void SecTrimCRLF(char *buffer, const size_t len)
{
    int i;
    for (i = (int)(len - 1); i >= 0 && (buffer[i] == '\r' || buffer[i] == '\n'); --i) {
        buffer[i] = '\0';
    }
    return;
}

char *gets_s(char *buffer, size_t numberOfElements)
{
    size_t len;
/*********
适配 Windows API 的特殊调用方式（如 gets_s(buf, -1)）。
是一种​​特殊标记​​，表示调用者希望使用预定义的​​最大安全长度​​（SECUREC_STRING_MAX_LEN，通常为 1024 或其他较大值），而非实际传递缓冲区大小。
若启用 SECUREC_COMPATIBLE_WIN_FORMAT宏，且 numberOfElements为 -1，则使用默认最大长度 SECUREC_STRING_MAX_LEN（通常为 1024）。
否则直接使用 numberOfElements。
*********/
#ifdef SECUREC_COMPATIBLE_WIN_FORMAT
    size_t bufferSize = ((numberOfElements == (size_t)-1) ? SECUREC_STRING_MAX_LEN : numberOfElements);
#else
    size_t bufferSize = numberOfElements;
#endif

    if (buffer == NULL || bufferSize == 0 || bufferSize > SECUREC_STRING_MAX_LEN) {
        SECUREC_ERROR_INVALID_PARAMTER("gets_s");//调用 SecureErrorInvalidParamter记录错误，返回 NULL。
        return NULL;
    }

    if (fgets(buffer, (int)bufferSize, stdin) == NULL) {
        return NULL;
    }

    len = strlen(buffer);
    if (len > 0 && len < bufferSize) {
        SecTrimCRLF(buffer, len);
    }

    return buffer;
}

