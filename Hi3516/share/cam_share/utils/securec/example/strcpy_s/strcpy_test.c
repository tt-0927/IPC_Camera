#include <stdio.h>
#include <string.h>
#include "strcpy_s.h"
/*******************************************************************************
 /​​​​​​​​​​​​​​​​​​​​
<函数描述>
strcpy_s函数将 strSrc指向的字符串（包括终止空字符 \0）复制到 strDest指向的数组中。
目标字符串缓冲区必须足够大以容纳源字符串（含终止符）。若源与目标内存重叠，函数返回 EOVERLAP_AND_RESET。

<输入参数>
strDest                 目标字符串缓冲区地址
destMax                 目标缓冲区大小（单位：字节）
strSrc                  以 \0结尾的源字符串缓冲区

<输出参数>
strDest                 更新后的目标字符串

<返回值>
EOK                      成功
EINVAL                  strDest为 NULL且 destMax != 0且 destMax <= SECUREC_STRING_MAX_LEN
EINVAL_AND_RESET        strDest非 NULL但 strSrc为 NULL且 destMax不为0也不超出限制
ERANGE                  destMax为 0或 > SECUREC_STRING_MAX_LEN
ERANGE_AND_RESET        strDest空间不足（其他参数有效且无重叠）
EOVERLAP_AND_RESET      源与目标内存重叠（其他所有所有参数都有效）

若发生运行时约束冲突，当 strDest和 destMax有效时，strDest[0]将被设为 \0。
 *******************************************************************************/

void show_error_codes() {
    printf(
        "<返回值>\n"
        "EOK = 0\t\t成功\n"
        "ERANGE = 34\t目标缓冲区为0或超出上限\n"
        "ERANGE_AND_RESET = -34\t目标缓冲区剩余空间不足\n"
        "EINVAL = 22\t源缓冲区src为null，或者目标缓冲区dest为null，或者都为null\n"
        "EINVAL_AND_RESET = -22\t源缓冲区src为null，但目标缓冲区不为null\n"
        "EOVERLAP_AND_RESET = -35\t源缓冲区src和目标缓冲区dest出现重叠\n"
    );
}
// 测试用例函数
void test_case(const char *desc, char *d, size_t dm, const char *s, errno_t expect) {
    char buf[64] = {0}; 
 
    
    printf("\n[%s]\n", desc);
    printf("参数: dest=%s, destMax=%zu, src=%s\n", 
           d , dm, s ? s : "NULL");
    
    errno_t ret = strcpy_s(d ? buf : d, dm, s);
    
    printf("返回值: %d\t预期: %d\t", ret, expect);

    if (d) {
        printf("Buffer: %s\n", buf);
        for (int i = 0; i < 4; i++) 
        printf("%02X ", ((unsigned char*)buf)[i]);
    }
    puts(ret == expect ? " [PASS]" : " [FAIL]");
}
//特殊用例（重叠情况）
void test_overlap(const char *desc, void *d, size_t dm, const void *s, errno_t expect) {
    printf("\n[%s]:dest=%p, destMax=%zu, src=%p\n",desc, d, dm, s);
    errno_t ret = strcpy_s(d , dm, s);
    printf("返回值: %d\t预期: %d\t", ret, expect);
    if (d) {
        printf("Buffer: %s\n", d);
        for (int i = 0; i < 4; i++) 
        printf("%02X ", ((unsigned char*)d)[i]);
    }
    printf ("\n");
}


int main() {
    show_error_codes();
   // const char *src = "test\x00";   
    char buf[64] = {0};     
    // 正确用例
    printf("--- 正确示范 ---");
   // 普通追加
    test_case("正常追加", buf, sizeof(buf), "ITC", EOK);
  
        
    // 错误用例
    printf("--- 错误示范 ---");
    // 目标缓冲区非法
    test_case("目标缓冲区为0", buf, 0, "data", ERANGE);
    test_case("目标缓冲区超限", buf, SECUREC_STRING_MAX_LEN+1, "data", ERANGE);

    // 空指针测试
    test_case("目标缓冲区NULL", NULL, 64, "data", EINVAL);
    test_case("源缓冲区NULL", buf, 64, NULL, EINVAL_AND_RESET);
    test_case("目标和源缓冲区均为NULL", NULL, 64, NULL, EINVAL);

    // 空间不足
    char small_buf[2] = {0};
    test_case("目标缓冲区剩余空间不足", small_buf, sizeof(small_buf), "ITC2025", ERANGE_AND_RESET);
   
    // 重叠用例

    char overlap_buf[16] = {0};
    test_overlap("正向重叠", overlap_buf, 16, overlap_buf, EOVERLAP_AND_RESET);

    // Windows特殊场景测试
    #if defined(SECUREC_COMPATIBLE_WINFORMAT)
    printf("\n[Windows兼容模式测试]\n");
    test_case( "WIN",buf, 0xFFFFFFFF, data, 5,EOK);  // 应成功
    #endif

    return 0;
}