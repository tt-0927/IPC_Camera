#include <stdio.h>
#include <string.h>
#include "gets_s.h"

/*********************************************************************************
 * <函数描述>
 * gets_s 函数从标准输入流（stdin）读取最多不超过 bufferSize-1 的字符，
 * 输入的标准流可以大于缓冲区大小，函数会自动读取不超过bufferSize-1 的部分并加上换行符
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
 * NULL：                             目标缓冲区为null
 * NULL                               目标缓冲区大小为0或者超出上限
 * NULL                               如果读取的第一个字符是文件结束符（EOF）
 ****************************************************************************/
void show_error_codes() {
    printf(
        "<返回值>\n"
        "buffer\t\t返回缓冲区地址，操作成功。\n"
        "NULL\t目标缓冲区为null\n"
        "NULL\t目标缓冲区大小为0或者超出上限\n"
        "NULL\t如果读取的第一个字符是文件结束符（EOF）\n"
    );
}

// 测试函数
void test_gets(const char *desc, char *buf, size_t size, errno_t expect) {
    
    printf("\n[%s]:dest=%p, buffsize=%zu\n",desc, buf, size);
    char *ret = gets_s(buf, size);
    
    printf("返回值: %s\t", ret ? "buffer地址" : "NULL");
    if (ret) {
        printf("内容: \"%s\"", buf);
    } else if (buf) {
        printf("读取失败，缓冲区状态: [未修改]");
    }
    printf ("\n");
}

int main() {
    show_error_codes();
    char buf[10];
    // 正确用例
    printf("--- 正确示范 ---");
    printf("\n输入短字符串（如\"Hello\"）: ");
    test_gets("正常输入", buf, sizeof(buf), "buffer地址");
    printf("\n输入超出缓冲区大小的字符串（也可正确执行）: ");
    test_gets("超出缓冲区大小", buf, sizeof(buf), NULL);

    printf("--- 错误示范 ---");
    // 错误用例
    printf("\n读取到结束符（Ctrl+D）: ");
    test_gets("读取失败", buf, sizeof(buf), NULL);

    test_gets("空指针", NULL, sizeof(buf), NULL);
    test_gets("零长度缓冲区", buf, 0, NULL);
    test_gets("缓冲区超限", buf, 2048, NULL);

    return 0;
}