#include <stdio.h>
#include <string.h>
#include "vscanf_s.h"

/*******************************************************************************
<函数描述>
vscanf_s函数与 scanf_s功能相同，但使用 arglist替代了可变参数列表。(即输入参数多一个​​arglist​​)
该函数从标准输入流 stdin读取数据，并将数据写入参数指定的内存位置。
每个参数必须是指向与格式字符串中类型说明符匹配的变量类型的指针。若发生字符串重叠拷贝，行为是未定义的。

<输入参数>
​​format                        ​​格式控制字符串（如 "%d %f"）。
​​arglist​​                       指向参数列表的指针（通常通过 va_list传递）。

<输出参数>
​​arglist​                       ​转换后的值将存储到用户指定的地址中（通过指针修改原变量）。

<返回值>
成功时：                      返回被成功转换并赋值的字段数量（未赋值的字段不计入）。
返回 0：                      表示没有字段被赋值（如输入与格式不匹配）。
返回 -1：                     发生错误（如读取失败或非法参数）。
 *******************************************************************************
 */
void show_error_codes() {
    printf(
        "<返回值>\n"
        "返回被成功转换并赋值的字段数量\t成功。\n"
        "返回 0\t\t表示没有字段被赋值（如输入与格式不匹配）\n"
        "返回 -1\t发生错误（如读取失败或非法参数）。\n"
    );
}

void print_scan_results(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    
    // 先读取输入（不立即打印）
    int ret = vscanf_s(fmt, args);
    
    if (ret < 0) {
        printf("输入错误!\n");
    } else {
        printf("成功读取 %d 个参数，内容为：\n", ret);
        
        // 关键：重新初始化va_list以遍历参数
        va_end(args);
        va_start(args, fmt);
        
        // 根据格式字符串动态解析
        for (const char *p = fmt; *p; ++p) {
            if (*p != '%') continue;
            
            switch (*(++p)) {
                case 'd': {
                    int val = va_arg(args, int);
                    printf("  int: %d\n", val);
                    break;
                }
                case 'f': {
                    double val = va_arg(args, double);
                    printf("  float: %.2f\n", val);
                    break;
                }
                case 's': {
                    char *val = va_arg(args, char*);
                    printf("  string: \"%s\"\n", val);
                    break;
                }
                // 可扩展其他格式符...
            }
        }
    }
    va_end(args);
}

int main() {
    int age;
    float height;
    char name[32] = {0};
    show_error_codes();
    
    printf("请输入 姓名 年龄 身高（例：张三 20 1.85）: ");
    print_scan_results("%s %d %f", name, &age, &height);
    
    return 0;
}