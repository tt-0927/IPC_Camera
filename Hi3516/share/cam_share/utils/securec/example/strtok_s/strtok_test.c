#include <stdio.h>
#include <string.h>
#include "strtok_s.h"
/*******************************************************************************
 * <功能描述>
 *  函数 strtok_s 将字符串解析为令牌序列
 *  首次调用时，strToken 参数应指定待解析的字符串
 *  后续对同一字符串的解析调用中，strToken 应设为 NULL
 *
 * <输入参数>
 * strToken       包含令牌的字符串（首次调用传入）
 * strDelimit    分隔符字符集合
 * context       用于存储调用间位置信息的上下文指针
 *
 * <输出参数>
 *  context       更新后的解析状态
 *
 *  <返回值>
 *  成功时返回指向下一个令牌的指针
 *  当找不到更多令牌时返回 NULL
 *  每次调用会修改原字符串，将令牌后的第一个分隔符替换为 NULL 字符
 *
 *  返回 NULL 的条件：
 *  - context 为 NULL
 *  - strDelimit 为 NULL
 *  - strToken 为 NULL 且 (*context) 为 NULL
 *  - 未找到令牌
 *******************************************************************************
 */
void show_error_codes() {
    printf(
        "<返回 NULL 的条件>\n"
        "context 为 NUL\n"
        "分隔符 为 NULL\n"
        "输入字符串 为 NULL 且 (*context) 为 NULL\n"
    );
}
// 测试用例函数
void test_cases() {
    printf("===== NULL Return Test Cases =====\n");

    // Case 1: 空字符串
    char s1[] = "";
    char *ctx1 = NULL;
    printf("[1] 输入字符串为空，返回值：%s\n", 
           strtok_s(s1, ",", &ctx1) );

    // Case 2: 纯分隔符
    char s2[] = "";
    char *ctx2 = NULL;
    printf("[2] 无分隔符，返回值 ：%s\n", 
           strtok_s(s2, "|", &ctx2) );

    // Case 3: 传入NULL且上下文为NULL
    char *ctx3 = NULL;
    printf("[3] 输入为null且上下文为null，返回值 ：%s\n", 
           strtok_s(NULL, ",", &ctx3) );
}
// 线程任务：安全分割字符串
void *thread_task(void *arg) {
    char str[] = "one;two;three";
    char *ctx = NULL;
    
    printf("Thread %lu:\n", (unsigned long)pthread_self());
    char *token = strtok_s(str, ";", &ctx);
    while (token != NULL) {
        printf("  %s\n", token);
        token = strtok_s(NULL, ";", &ctx);
    }
    return NULL;
}

int main() {
    show_error_codes() ;
    pthread_t t1, t2;
    test_cases();
    // 创建两个线程同时执行分割
    pthread_create(&t1, NULL, thread_task, NULL);
    pthread_create(&t2, NULL, thread_task, NULL);
    
    pthread_join(t1, NULL);
    pthread_join(t2, NULL);
    
    return 0;
}
