/**
 * @FilePath     : ssh_account.cpp
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2026-03-16 10:25:57
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-03-16 14:27:04
 * @Description  : SSH 账号管理实现
 */

#include "ssh_account.h"

#include <cstdarg>
#include <cstdlib>
#include <cstdio>
#include <cstring>

#include "dlog.h"

namespace
{
    /* 匿名命名空间中的辅助函数只服务于本文件内部，不向外暴露。 */
    /* 账号管理只允许操作我们自己维护的 ssh_admins 组，避免误伤系统账号。 */

    /* 系统用户组配置文件路径 */
    constexpr const char* kGroupFile = "/etc/group";
    /* SSH 管理员专用用户组名称 */
    constexpr const char* kSshAdminGroup = "ssh_admins";

    /**
     * @brief   : 执行格式化 shell 命令
     * @param    {const char *} fmt：命令格式字符串
     * @return   {int} 0：成功，非0：失败
     * @note    : 仅供当前文件内的 SSH 账号辅助逻辑复用
     */
    int run_cmd(const char* fmt, ...)
    {
        char cmd[512] = { 0 };
        va_list args;
        va_start(args, fmt);
        vsnprintf(cmd, sizeof(cmd), fmt, args);
        va_end(args);
        return std::system(cmd);
    }

    /**
     * @brief   : 判断账号是否为 root
     * @param    {const char *} username：待检查账号
     * @return   {bool} true：是，false：否
     */
    bool is_root_account(const char* username)
    {
        return username != nullptr && std::strcmp(username, "root") == 0;
    }

    /**
     * @brief   : 判断账号是否已存在
     * @param    {const char *} username：待检查账号
     * @return   {bool} true：存在，false：不存在
     */
    bool user_exists(const char* username)
    {
        return 0 == run_cmd("grep -q \"^%s:\" /etc/passwd", username);
    }

    /**
     * @brief   : 判断账号是否属于 SSH 管理员组
     * @param    {const char *} username：待检查账号
     * @return   {bool} true：属于，false：不属于
     */
    bool is_ssh_admin(const char* username)
    {
        return 0 == run_cmd("id -nG %s | grep -qw %s", username, kSshAdminGroup);
    }

    /**
     * @brief   : 创建 SSH 管理员账号
     * @param    {const char *} username：待创建账号
     * @return   {int} 0：成功，非0：失败
     * @note    : 优先使用带附加组的 adduser，失败后退化为补组成员
     */
    int create_admin_user(const char* username)
    {
        /* 优先走标准分组创建；如果目标系统 adduser 不支持 -G，再退化为补组成员。 */
        int ret = run_cmd("adduser -s /bin/sh -D -H -G %s %s", kSshAdminGroup, username);
        if (ret == 0)
        {
            dlog_debug("创建 SSH 管理员账号成功，账号[%s]", username);
            return 0;
        }

        ret = run_cmd("adduser -s /bin/sh -D -H %s", username);
        if (ret != 0)
        {
            dlog_error("创建 SSH 管理员账号失败，账号[%s]", username);
            return -1;
        }

        ret = run_cmd("sed -i '/^%s:/ s/$/,%s/' %s", kSshAdminGroup, username, kGroupFile);
        if (ret != 0)
        {
            dlog_error("补充 SSH 用户组成员失败，账号[%s]", username);
            return -1;
        }

        dlog_debug("创建 SSH 管理员账号成功，账号[%s]", username);
        return 0;
    }

    /**
     * @brief   : 准备 SSH 账号的 HOME 目录
     * @param    {const char *} username：SSH 管理员账号
     * @return   {void}
     * @note    : 会同步修正目录属主，保证 dropbear 登录后的 HOME 可用
     */
    void prepare_home_directory(const char* username)
    {
        /* dropbear 登录后会依赖 HOME 路径，故这里一并补齐目录和属主。 */
        run_cmd("mkdir -p /home/%s", username);
        run_cmd("chown %s:%s -R /home/%s", username, kSshAdminGroup, username);
    }
} // namespace

void SshAccountManager::init_admin_group()
{
    /* 该初始化在开机和账号同步场景都会执行，组已存在时直接复用即可。 */
    if (0 == run_cmd("grep -q \"^%s:\" /etc/group", kSshAdminGroup))
    {
        return;
    }

    if (0 != run_cmd("addgroup %s", kSshAdminGroup))
    {
        dlog_error("创建 SSH 用户组失败，用户组[%s]", kSshAdminGroup);
        return;
    }

    dlog_debug("创建 SSH 用户组成功，用户组[%s]", kSshAdminGroup);
}

int SshAccountManager::set_password(const char* username, const char* password)
{
    /* 这里与系统 passwd 命令交互，失败通常意味着用户名非法或系统状态异常。 */
    if (username == nullptr || password == nullptr)
    {
        dlog_error("设置 SSH 账号密码失败，用户名或密码为空");
        return -1;
    }

    if (0 != run_cmd("echo -e \"%s\\n%s\" | passwd %s", password, password, username))
    {
        dlog_error("设置 SSH 账号密码失败，账号[%s]", username);
        return -1;
    }

    dlog_debug("设置 SSH 账号密码成功，账号[%s]", username);
    return 0;
}

int SshAccountManager::add_admin(const char* username, const char* password)
{
    /* root 账号由系统托管，业务侧只维护网页管理员映射出来的 SSH 管理员账号。 */
    if (username == nullptr || password == nullptr || is_root_account(username))
    {
        dlog_warn("添加 SSH 管理员账号被拒绝，账号非法");
        return -1;
    }

    if (user_exists(username))
    {
        if (!is_ssh_admin(username))
        {
            dlog_warn("添加 SSH 管理员账号被拒绝，账号[%s]不是 SSH 管理员", username);
            return -1;
        }
        dlog_debug("SSH 管理员账号已存在，更新密码，账号[%s]", username);
    }
    else if (0 != create_admin_user(username))
    {
        return -1;
    }

    prepare_home_directory(username);
    return set_password(username, password);
}

int SshAccountManager::del_admin(const char* username)
{
    /* 删除前做两层保护：先判 root，再判该账号是否属于 ssh_admins 组。 */
    if (username == nullptr || is_root_account(username))
    {
        dlog_warn("删除 SSH 管理员账号被拒绝，账号非法");
        return -1;
    }

    if (!user_exists(username))
    {
        dlog_debug("SSH 管理员账号不存在，跳过删除，账号[%s]", username);
        return 0;
    }

    if (!is_ssh_admin(username))
    {
        dlog_warn("删除 SSH 管理员账号被拒绝，账号[%s]不是 SSH 管理员", username);
        return -1;
    }

    if (0 != run_cmd("deluser %s", username))
    {
        dlog_error("删除 SSH 管理员账号失败，账号[%s]", username);
        return -1;
    }

    run_cmd("rm -rf /home/%s", username);
    dlog_debug("删除 SSH 管理员账号成功，账号[%s]", username);
    return 0;
}
