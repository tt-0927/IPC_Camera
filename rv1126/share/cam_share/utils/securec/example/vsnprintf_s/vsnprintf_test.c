#include <stdio.h>
#include <stdarg.h>
#include <string.h>
 /*******************************************************************************
 * <功能描述>
 *  vsnprintf_s功能与 snprintf_s等效,但参数多了一个va_list args，用之前要先初始化
 *  然后传入参数给vsnprintf_s，
 *  函数将count 个格式化字符复制到目标缓冲区
 *  该函数将格式化后的字符存入 strDest，最多写入 destMax-1个字符并自动追加空终止符
 *  每个参数按 format中的格式说明符进行转换输出
 *  格式化规则与 printf系列函数一致
 *  若源字符串与目标内存重叠，行为未定义
 *
 * <输入参数>
 *    strDest                 输出存储地址
 *    destMax                 输出缓冲区大小（字节数）
 *    format                  格式
 *    count                   要复制的个数
 *    ...                     
 *
 * <输出参数>
 *    dest buffer                strDest内容被更新为格式化后的字符串
 *
 * <返回值>
 *  这里返回值为格式化后完整字符串的理论长度”，而非实际写入缓冲区的长度
 *  当发生截断时，调用方可通过 ret知道原始数据有多长，从而决定是否扩容缓冲区或扩容count。
 *  返回-1表示发生错误（超出内存地址范围，格式不匹配，count超出目标缓冲区大小等）。
 *  当strDest和destMax有效但发生约束违反时（如缓冲区溢出）清空缓冲区
 *******************************************************************************
 */
// 安全日志函数（使用vsnprintf_s）
void safe_log_enhanced(FILE *stream, size_t max_len, const char *format, ...) {
    char buf[128]; // 固定缓冲区
    va_list args;
    va_start(args, format);

    // 关键区别：vsnprintf_s 允许指定最大写入长度（含\0）
    int ret = vsnprintf_s(buf, sizeof(buf), max_len, format, args);
    
    if (ret < 0) {
        fprintf(stream, "[ERROR] Format error (code:%d)\n", ret);
    } else if ((size_t)ret >= max_len) {
       // 打印缓冲区十六进制内容
    printf("HEX DUMP: ");
    for (int i = 0; i < max_len; i++) {
        printf("%02X ", (unsigned char)buf[i]);
    }
    printf("\n");
    // 原警告输出
    fprintf(stream, "[WARN] Truncated: %.*s...\n", (int)max_len-1, buf);
    } else {
        fprintf(stream, "LOG: %s\n", buf);
    }
    va_end(args);
}

int main() {
    // 正常用例（限制输出长度）
    safe_log_enhanced(stdout, 32, "User:%s, ID:%d", "Alice", 1001);
    // 输出: LOG: User:Alice, ID:1001

    // 触发截断警告（限制max_len为20字节）
    safe_log_enhanced(stderr, 20, "Long message:%s", "ThisWillBeTruncated");
    // 输出: [WARN] Truncated: Long message:ThisWi...
    
    return 0;
}