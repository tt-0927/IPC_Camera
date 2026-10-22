#include <stdio.h>
#include <string.h>
#include "memset_s.h"
/*******************************************************************************
 * <功能描述>
 *  将值c（转换为unsigned char）复制到dest指向对象的前count个字符中
 *
 * <输入参数>
 *    dest                           目标缓冲区
 *    destMax                        目标缓冲区大小
 *    c                              要设置的值
 *    count                          要设置的数量
 *
 * <输出参数>
 *    dest buffer                    更新后的目标缓冲区
 *
    <返回值>
    EOK                     = 0    成功
    ERANGE                  = 34   目标缓冲区destmax=0或超出上限
    ERANGE_AND_RESET        =-34   复制数量大于目标缓冲区大小 count>destmax，给用c填满整个destMax区域,填充长度为destMax,剩余为0
    EINVAL                  = 22   目标缓冲区dest为null

 *    当返回ERANGE_AND_RESET时，会用c填满整个destMax区域,填充长度为destMax,剩余为0
 *******************************************************************************
 */

void show_error_codes() {
    printf(
        "<返回值>\n"
        "EOK = 0\t\t成功\n"
        "ERANGE = 34\t目标缓冲区destmax=0或超出上限\n"
        "ERANGE_AND_RESET = -34\t复制数量大于目标缓冲区大小 count>destmax，清空dest\n"
        "EINVAL = 22\t目标缓冲区dest为null\n"
    );
}
// 测试用例
void test_case(const char *desc, void *d, size_t dm, int c, size_t count, errno_t expect) {
   // 初始化为 00 00 00 00
    char buf[16] = {0}; 
    printf("\n[%s]:dest=%p, destMax=%zu, c=%p, count=%zu\n",desc, d, dm, c, count);
    errno_t ret = memset_s(d ? buf : d, dm, c, count);
    printf("ret=%d (expect %d)\n ", ret, expect);
    if (d) {
        printf("Buffer: %s\n", buf);
        for (int i = 0; i < 6; i++) 
        printf("%02X ", (unsigned char)buf[i]);
    }
    printf ("\n");
}

int main() {
    show_error_codes();
   // const char *src = "test\x00"; 
    char buf[16] = {0};         
    // 正确用例
    test_case("正确设置", buf, 16, 6, 5, EOK);
    
    // 错误用例
    test_case("目标缓冲区为null", NULL, 16, 6, 5, EINVAL);
    test_case("目标缓冲区大小为0", buf, 0, 6, 5, ERANGE);
    test_case("目标缓冲区大小超出限制", buf, 65537, 6, 5, ERANGE);
    test_case("复制数量超出目标缓冲区大小", buf, 4, 6, 5, ERANGE_AND_RESET);


    // Windows特殊场景测试
    #if defined(SECUREC_COMPATIBLE_WINFORMAT)
    printf("\n[Windows兼容模式测试]\n");
    test_case( "WIN",buf, 0xFFFFFFFF, 6, 5,EOK);  // 应成功
    #endif

    return 0;
}