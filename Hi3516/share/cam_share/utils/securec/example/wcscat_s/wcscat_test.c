#include <stdio.h>
#include <string.h>
#include "wcscat_s.h"
/*******************************************************************************
 * <功能描述>
 * wcscat_s和strcat_s函数功能一样，但是支持宽字符
 *wcscat_s函数将 strSrc指向的字符串（包括终止空字符 \0）追加到 strDest指向的字符串末尾。
 *strSrc的首字符会覆盖 strDest原有的终止符 \0。
 *若源字符串与目标缓冲区内存重叠，函数返回 EOVERLAP_AND_RESET。
 *注意：第二个参数 destMax是目标缓冲区的总容量（而非剩余可用空间）。
 *
 * <输入参数>
 *    strDest             strDest以 \0结尾的目标字符串缓冲区。
 *    destMax             destMax目标缓冲区的大小（单位：字节）。
 *    strSrc              strSrc以 \0结尾的源字符串缓冲区。
 *
 * <输出参数>
 *    ...                   strDest更新后的目标字符串。
 *
 * <RETURN VALUE>
 *    EOK                 Success
 *    EINVAL              源缓冲区为null，或源缓冲区和目标缓冲区都为null
 *    EINVAL_AND_RESET    目标字符串 strDest​​未正确终止​​（即不以 \0结尾）
 *    EINVAL_AND_RESET    原缓冲区为null，目标缓冲区不为null
 *    ERANGE              目标缓冲区超出限制或者为0
 *    ERANGE_AND_RESET    目标缓冲区剩余空间不足
 *    EOVERLAP_AND_RESET  其他参数均有效，但原和目标缓冲区重叠
 *
 *    当返回值报错，且strDest非 NULL且destMax有效（0 < destMax ≤ SECUREC_STRING_MAX_LEN），会将 strDest[0]设置为 \0​​
 *******************************************************************************
 */
void show_error_codes() {
    printf(
        "<返回值>\n"
        "EOK = 0\t\t成功\n"
        "ERANGE = 34\t目标缓冲区为0或超出上限\n"
        "ERANGE_AND_RESET = -34\t目标缓冲区剩余空间不足\n"
        "EINVAL = 22\t源缓冲区src为null，或者目标缓冲区dest为null，或者都为null\n"
        "EINVAL_AND_RESET = -22\t源缓冲区src为null，但目标缓冲区不为null\n"
        "EINVAL_AND_RESET = -22\t 目标字符串 strDest​​未正确终止​​（即不以终止符结尾）\n"
        "EOVERLAP_AND_RESET = -35\t源缓冲区src和目标缓冲区dest出现重叠\n"
    );
}
// 测试用例函数
void test_case(const char *desc, wchar_t *d, size_t dm, const wchar_t *s, errno_t expect) {
    wchar_t buf[64] = L"Hello";   
 
    
    printf("\n[%s]\n", desc);
    printf("参数: dest=%ls, destMax=%zu, src=%ls\n", 
           d , dm, s ? s : "NULL");
    
    errno_t ret = wcscat_s(d ? buf : d, dm, s);
    
    printf("返回值: %d\t预期: %d\t", ret, expect);

    if (d) {
        printf("Buffer: %ls\n", buf);
        for (int i = 0; i < 4; i++) 
        printf("%04X ", ((unsigned char*)buf)[i]);
    }
    puts(ret == expect ? " [PASS]" : " [FAIL]");
}
//特殊用例（重叠情况）
void test_overlap(const char *desc, wchar_t *d, size_t dm, const wchar_t *s, errno_t expect) {
    printf("\n[%s]:dest=%ls, destMax=%zu, src=%ls\n",desc, d, dm, s);
    errno_t ret = wcscat_s(d , dm, s);
    printf("返回值: %d\t预期: %d\t", ret, expect);
    if (d) {
        printf("Buffer: %ls\n", d);
        for (int i = 0; i < 4; i++) 
        printf("%04X ", ((unsigned char*)d)[i]);
    }
    printf ("\n");
}
//特殊用例（无终止符情况）
void test_no_terminator() {
    wchar_t buf[8];
    for (int i = 0; i < 8; i++) buf[i] = L'A'; 

    printf("\n [目标字符串无终止符]:\n");
    printf("初始buf内容: ");
    for (int i = 0; i < sizeof(buf); i++) {
        printf("%04X ", (unsigned char)buf[i]);  
    }
    
    errno_t ret = wcscat_s(buf, sizeof(buf)/sizeof(wchar_t), "123");
    printf("\n返回值: %d (预期: -22)\t", ret);
    printf("buf[0]=%04X\n", (unsigned char)buf[0]);
    
    if (ret == EINVAL_AND_RESET && buf[0] == '\0') {
        printf(" 目标字符串未检测到终止符并清空缓冲区\n");
    } else {
        printf("FAIL: 未正确处理无终止符情况\n");
    }
}

int main() {
    show_error_codes();
   // const char *src = "test\x00";   
   wchar_t buf[64] = L"Hello";     
    // 正确用例
    printf("--- 正确示范 ---");
   // 普通追加
    test_case("正常追加", buf, sizeof(buf), L"_World", EOK);
    // 空字符串追加
    test_case("追加空字符串", buf, sizeof(buf), "", EOK);
        
    // 错误用例
    printf("--- 错误示范 ---");
    // 目标缓冲区非法
    test_case("目标缓冲区为0", buf, 0, L"data", ERANGE);
    test_case("目标缓冲区超限", buf, SECUREC_WCHAR_STRING_MAX_LEN+1, L"data", ERANGE);

    // 空指针测试
    test_case("目标缓冲区NULL", NULL, 64, L"data", EINVAL);
    test_case("源缓冲区NULL", buf, 64, NULL, EINVAL_AND_RESET);
    test_case("目标和源缓冲区均为NULL", NULL, 64, NULL, EINVAL);

    // 空间不足
    wchar_t small_buf[8] = L"Hello";
    test_case("目标缓冲区剩余空间不足", small_buf, sizeof(small_buf)/sizeof(wchar_t), L"_World", ERANGE_AND_RESET);
   
    // 重叠用例
   // 正向重叠
   wchar_t overlap_buf[16] = L"12345678";
    test_overlap("正向重叠", overlap_buf+2, 16, overlap_buf, EOVERLAP_AND_RESET);

    // 反向重叠
    wchar_t reverse_buf[16] = L"12345678";
    test_overlap("反向重叠", reverse_buf, 16, reverse_buf+2, EOVERLAP_AND_RESET);

    test_no_terminator();
    // Windows特殊场景测试
    #if defined(SECUREC_COMPATIBLE_WINFORMAT)
    printf("\n[Windows兼容模式测试]\n");
    test_case( "WIN",buf, 0xFFFFFFFF, data, 5,EOK);  // 应成功
    #endif

    return 0;
}