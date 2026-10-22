#include <stdio.h>
#include <stdarg.h>
#include <string.h>
 /*******************************************************************************
 * <功能描述>
 *  函数将count 个格式化字符复制到目标缓冲区（无需va list listargs初始化)
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
 *  这些函数均返回实际写入的字符数（不包含终止符）
 *  返回-1表示发生错误（超出内存地址范围，格式不匹配，count超出目标缓冲区大小等）。
 *  当strDest和destMax有效但发生约束违反时（如缓冲区溢出）清空缓冲区
 *******************************************************************************
 */
// 安全日志函数（使用vsnprintf_s）
#include "securec.h"
#include <assert.h>

void test_normal_case() {
    char buf[32];
    // 正常用例（双重限制：destMax=32, count=20）
    int ret = snprintf_s(buf, sizeof(buf), 20, "User:%s, ID:%d", "Alice", 1001);
    assert(ret == 19);  // 实际长度
    printf("Normal: %s\n", buf);  // 输出: User:Alice, ID:1001
}

void test_truncated_case() {
    char buf[16];
    // 触发截断（count=10 < destMax=16）
    int ret = snprintf_s(buf, sizeof(buf), 10, "Long message:%s", "ABCDEFGHIJK");
    printf("Truncated: %s\n", buf);  // 输出: Long messa
}


int main() {
    test_normal_case();
    test_truncated_case();

    return 0;
}