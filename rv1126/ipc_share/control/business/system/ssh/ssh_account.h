/**
 * @FilePath     : ssh_account.h
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2026-03-16 10:25:57
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-03-16 14:27:53
 * @Description  : SSH 账号管理
 */

#pragma once

class SshAccountManager
{
public:
    /**
     * @brief   : 初始化 SSH 管理员用户组
     * @return   {void}
     */
    void init_admin_group();

    /**
     * @brief   : 设置 SSH 管理员账号密码
     * @param    {const char *} username：SSH 管理员账号
     * @param    {const char *} password：SSH 管理员密码
     * @return   {int} 0：成功，非0：失败
     */
    int set_password(const char *username, const char *password);

    /**
     * @brief   : 添加或更新 SSH 管理员账号
     * @param    {const char *} username：SSH 管理员账号
     * @param    {const char *} password：SSH 管理员密码
     * @return   {int} 0：成功，非0：失败
     * @note    : 仅允许受控管理员账号进入 ssh_admins 组
     */
    int add_admin(const char *username, const char *password);

    /**
     * @brief   : 删除 SSH 管理员账号
     * @param    {const char *} username：SSH 管理员账号
     * @return   {int} 0：成功，非0：失败
     * @note    : 会校验 root 账号和 ssh_admins 组，避免误删系统账号
     */
    int del_admin(const char *username);
};
