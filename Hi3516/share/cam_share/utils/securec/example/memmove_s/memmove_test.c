#include <stdio.h>
#include <string.h>
#include "memmove_s.h"
/*******************************************************************************
<功能描述>
memove_s 函数将 src 指向对象的 n 个字符复制到 dest 指向的对象中（和memcpy_s一样但是支持重叠区域）。

<输入参数>
dest         目标缓冲区
destMax      目标缓冲区大小
src          源缓冲区
count        要复制的字符数

<输出参数>
dest         目标缓冲区

<返回值>
EOK                     = 0    成功
ERANGE                  = 34   目标缓冲区destmax=0或超出上限
ERANGE_AND_RESET        =-34   复制数量大于目标缓冲区大小 count>destmax，清空dest
EINVAL                  = 22   源缓冲区src为null，或者目标缓冲区dest为null，或者都为null
EINVAL_AND_RESET        =-22   源缓冲区src为null，但目标缓冲区不为null，清空dest


* 如果发生错误，dest 将被填充为 0。            
 *******************************************************************************/

 void show_error_codes() {
    printf(
        "<返回值>\n"
        "EOK = 0\t\t成功\n"
        "ERANGE = 34\t目标缓冲区destmax=0或超出上限\n"
        "ERANGE_AND_RESET = -34\t复制数量大于目标缓冲区大小 count>destmax，清空dest\n"
        "EINVAL = 22\t源缓冲区src为null，或者目标缓冲区dest为null，或者都为null\n"
        "EINVAL_AND_RESET = -22\t源缓冲区src为null，但目标缓冲区不为null，清空dest\n"       
    );
}
// 测试用例
void test_case(const char *desc, void *d, size_t dm, const void *s, size_t c, errno_t expect) {
    char buf[16] = {0xFF}; // 初始化为非0值
    printf("\n[%s]:dest=%p, destMax=%zu, src=%p, count=%zu\n",desc, d, dm, s, c);
    errno_t ret = memmove_s(d ? buf : d, dm, s, c);
    printf("ret=%d (expect %d)\n ", ret, expect);
    if (d) {
        printf("Buffer: %s\n", buf);
        for (int i = 0; i < 4; i++) 
        printf("%02X ", (unsigned char)buf[i]);
    }
    printf ("\n");
}
void test_overlap(const char *desc, void *d, size_t dm, const void *s, size_t c, errno_t expect) {
    printf("\n[%s]:dest=%p, destMax=%zu, src=%p, count=%zu\n",desc, d, dm, s, c);
    errno_t ret = memmove_s(d , dm, s, c);
    printf("ret=%d (expect %d)\n ", ret, expect);
    if (d) {
        printf("Buffer: %s\n", d);
        for (int i = 0; i < 4; i++) 
        printf("%02X ", ((unsigned char*)d)[i]);
    }
    printf ("\n");
}

int main() {
    show_error_codes() ;
    const char *src = "test\x00"; 
    char buf[16] = {0};          
    // 正确用例
    test_case("正确拷贝", buf, 16, src, 5, EOK);
    test_case("相同指针拷贝" ,buf,  16, buf,5,EOK); 
    strcpy(buf, "12345678"); 
    test_overlap("源缓冲区和目标缓冲区有重叠", buf+2, 8, buf, 5, EOK);
    
    // 错误用例
    test_case("目标缓冲区为null", NULL, 16, src, 5, EINVAL);
    test_case("目标缓冲区大小为0", buf, 0, src, 5, ERANGE);
    test_case("目标缓冲区大小超出限制", buf, 65537, src, 5, ERANGE);
    test_case("源缓冲区为null", buf, 16, NULL, 5, EINVAL_AND_RESET);
    test_case("复制数量超出目标缓冲区大小", buf, 4, src, 5, ERANGE_AND_RESET);
   

    // Windows特殊场景测试
    #if defined(SECUREC_COMPATIBLE_WINFORMAT)
    printf("\n[Windows兼容模式测试]\n");
    test_case( "WIN",buf, 0xFFFFFFFF, data, 5,EOK);  // 应成功
    #endif

    return 0;
}