
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
 /*******************************************************************************
 * <功能描述>
 *  vsprintf_s功能与 sprintf_s等效,但参数多了一个va_list args，用之前要先初始化
 *  然后传入参数给vsprintf_s，
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
// 安全格式化日志函数
void safe_log(FILE *stream, const char *format, ...) {
    char buf[64];
     //声明一个可变参数列表的指针变量
    va_list args;
    //初始化参数列表，让 args指向 format之后的第一个可变参数
    va_start(args, format);

    // 使用标准库vsprintf_s
    int ret = vsprintf_s(buf, sizeof(buf), format, args);
    if (ret < 0) {
        fprintf(stream, "[ERROR] Format failed (code:%d)\n", ret);
    } else {
        fprintf(stream, "ret = %d,LOG: %s\n", ret,buf);
    }

    va_end(args);
}

int main() {
    // 正常用例
    safe_log(stdout, "User %s logged in, ID:%d", "Alice", 1001);

    // 触发缓冲区不足错误
    safe_log(stderr, "This message is too long for the buffer: %s", 
            "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA");
    
    return 0;
}