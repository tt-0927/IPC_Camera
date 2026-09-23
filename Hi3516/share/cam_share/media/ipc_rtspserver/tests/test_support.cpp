/**
 * @FilePath     : test_support.cpp
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2026-09-10 18:49:34
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-09-16 11:35:00
 * @Description  : 极简测试支撑实现
 */

#include "test_support.h"

namespace rtsp_test
{
namespace
{
/* 失败统计与"当前用例已失败"标记；测试为单线程顺序执行，无需同步。 */
int g_failed_checks = 0;
bool g_case_failed = false;
} // namespace

std::vector<Case> &Registry()
{
    static std::vector<Case> registry;
    return registry;
}

Registrar::Registrar(const char *name, std::function<void()> body)
{
    Registry().push_back(Case{ name, std::move(body) });
}

void Fail(const char *file, int line, const std::string &message)
{
    std::fprintf(stderr, "  [失败] %s:%d %s\n", file, line, message.c_str());
    std::fflush(stderr);
    ++g_failed_checks;
    g_case_failed = true;
}

int RunAll()
{
    for (const Case &item : Registry())
    {
        std::printf("运行 %s ... ", item.name);
        std::fflush(stdout);
        g_case_failed = false;
        item.body();
        /* 断言失败不中断：继续跑完当前用例与后续用例，最后按失败总数
         * 返回退出码；若失败意味着后续语句无法安全执行（如空指针解引用），
         * 用例可能崩溃退出，ctest 同样判定为失败。 */
        std::printf("%s\n", g_case_failed ? "失败" : "通过");
    }
    std::printf("共 %zu 个用例，失败断言 %d 处\n", Registry().size(), g_failed_checks);
    return g_failed_checks == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}

} // namespace rtsp_test

int main()
{
    return rtsp_test::RunAll();
}
