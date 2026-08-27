/***
 * @FilePath     : user_manage.h
 * @Author       : zhangjc (zhangjc@kfb.cn)
 * @Date         : 2024-11-14 10:23:02
 * @LastEditors  : huangjunda
 * @LastEditTime : 2025-03-28 10:23:02
 * @Description  : 用户管理
 */

#pragma once

#include "user_define.h"
#include "register_define.h"
#include <vector>
#include "Singleton.h"
#include "IpcRet.h"
#include "system_define.h"

#ifdef __cplusplus
extern "C"
{
#endif

#include "user_module.h"

#ifdef __cplusplus
}
#endif

//未知用户
#define UNKNOWN_USER    "unknown"
// 已添加用户前缀
#define PREFIX_USER "USER_"
// 未添加用户，IP前缀
#define PREFIX_IP   "IP_"

class CUserManage : public CSingleton<CUserManage>
{
    CUserManage() = default;

public:
    ~CUserManage() = default;
    friend class CSingleton<CUserManage>;
    /**
     * @brief 初始化
     * @return IpcRet_E <0:失败, >=0:成功
     */
    IpcRet_E init();

    /**
     * @brief 反初始化
     */
    IpcRet_E deinit();

    /**
     * @brief 用户登录
     * @param stAccountInfo 账号信息
     * @param stUserInfo 用户信息
     * @param stSecurityServicesInfo 安全配置信息
     * @return int  <0:失败, >=0:成功
     */
    int login(User::AccountInfo_S stAccountInfo, User::UserInfo_S &stUserInfo, ::System::SecurityServices_S stSecurityServicesInfo);

    /**
     * @brief 添加用户
     * @param stUserInfo 用户信息
     * @param stSecurityServicesInfo 安全配置信息
     * @return int  <0:失败, >=0:成功
     */
    int add(User::UserInfo_S stUserInfo, ::System::SecurityServices_S stSecurityServicesInfo);

    /**
     * @brief 删除用户
     * @param stUserInfo 用户信息
     * @return int  <0:失败, >=0:成功
     */
    int del(User::UserInfo_S stUserInfo);

    /**
     * @brief 删除用户
     * @param stAccountInfo 账号信息
     * @return int  <0:失败, >=0:成功
     */
    int del(User::AccountInfo_S stAccountInfo);

    /**
     * @brief 修改用户
     * @param stUserUpdateInfo 修改信息
     * @param stSecurityServicesInfo 安全配置信息
     * @return int
     */
    int update(User::UpdateInfo_S stUserUpdateInfo, ::System::SecurityServices_S stSecurityServicesInfo);

    /**
     * @brief 更新用户状态
     * @param stUserInfo 账号信息
     * @param enStatus 状态
     * @return int
     */
    int update_status(User::UserInfo_S stUserUpdateInfo, User::AccountStatus_E enStatus);

    /**
     * @brief 重置密码信息
     * @param stUserUpdateInfo 修改信息
     * @param stSecurityServicesInfo 安全配置信息
     * @return int
     */
    int reset_password(User::UpdateInfo_S stUserUpdateInfo, ::System::SecurityServices_S stSecurityServicesInfo);

    /**
     * @brief 查找用户信息
     * @param stUserFind 查找条件
     * @param userInfos 用户信息列表
     * @return int <0:失败, >=0:成功
     */
    int find(User::Find_S stUserFind, std::vector<User::UserInfo_S> &userInfos);

    /**
     * @brief 根据账号获取用户信息
     * @param stUserInfo 获取用户信息
     * @return int <0:失败, >=0:成功
     */
    int get_itemInfo(User::UserInfo_S &stUserInfo);

    /**
     * @brief 获取所有用户信息
     * @param userInfos 用户信息列表
     * @return int
     */
    int get_all_user(std::vector<User::UserInfo_S> &userInfos);

    /**
     * @brief   : 获取用户密码
     * @param    {string} strUser 用户名
     * @return   {string} 不为空：密码 为空：失败
     */
    std::string get_passwd(std::string strUser);

    /**
     * @brief   : 验证用户是否存在
     * @param    {string} strUser 用户名
     * @return   {int} 0：存在，非0：不存在
     */
    int verifi_user(std::string strUser);

    /**
     * @brief admin管理员密码验证
     * @return int
     */
    int admin_verification(std::string strPassword);

    /**
     * @brief 用户权限验证
     * @param stUserAuthPermission 用户操作信息
     * @return int
     */
    int user_permssion_auth(User::UserOperation_S stUserOperation);

    /**
     * @brief 获取登录错误信息
     * @param strAccount 用户名
     * @return std::string
     */
    User::LogErrorInfo_S get_logErrorInfo(std::string strAccount = "");

    int set_lockNowTime(std::string strTime);

    /**
     * @brief 获取在线用户信息
     * @return std::vector<OnlineUser>
     */
    std::vector<User::OnlineUser_S> get_online_users();

    /**
     * @brief 获取在线指定用户信息
     * @return int
     */
    int get_online_user_specified(User::OnlineUser_S &UserOnlineInfo, const std::string& strUsername);

    /**
     * @brief 删除在线用户
     * @param nOnlineUserId 在线用户id
     * @return int
     */
    int delete_online_user(int nOnlineUserId);

    /**
     * @brief 检查指定IP是否已有用户在线
     * @param strIp IP地址
     * @return bool true:该IP已存在在线用户, false:该IP无在线用户
     */
    bool check_ip_online(const std::string& strIp);

private:
    /* 多用户登录错误信息 */
    std::unordered_map<std::string, User::LogErrorInfo_S> mErrorInfo;
    /* 登录密码过期提醒记录信息，记录是否重复提醒，第二次不进行提醒*/
    std::unordered_map<std::string, bool> mPasswordExpiryReminder;
    /* 缓存登录阶段携带的业务时间，用于后续强制改密/过期改密 */
    std::unordered_map<std::string, std::string> mPendingPasswordUpdateTime;
    /* 在线用户信息 */
    std::vector<User::OnlineUser_S> m_vecOnlineUsers;
    /*锁，防止竞争 */
    std::mutex m_mutex;

    /**
     * @brief 添加超时时间
     * @param strTime
     * @param enDuration 时间间隔
     */
    std::string addErrorTime(const std::string &strTime,::System::LockDuration_E enDuration);

    /**
     * @brief   : 检查指定IP是否存在在线记录，调用前需持有 m_mutex
     * @param    {std::string} strIp：IP地址
     * @return   {bool} true：存在在线记录，false：不存在
     */
    bool check_ip_online_locked(const std::string &strIp) const;

    /**
     * @brief   : 检查指定账号是否存在在线记录，调用前需持有 m_mutex
     * @param    {std::string} strAccount：账号
     * @return   {bool} true：存在在线记录，false：不存在
     */
    bool has_online_user_for_account_locked(const std::string &strAccount) const;

    /**
     * @brief   : 按IP清理残留在线记录
     * @param    {std::string} strIp：IP地址
     * @return   {int} 清理数量，0：未清理到记录
     */
    int cleanup_online_users_by_ip(const std::string &strIp);

    /**
     * @brief   : 根据当前在线记录刷新账号状态
     * @param    {std::string} strAccount：账号
     * @return   {void}
     */
    void refresh_account_online_status(const std::string &strAccount);

    /**
     * @brief   : 生成在线用户唯一id
     * @param    {}
     * @return   {int} 在线用户唯一id
     */
    int generate_online_userId();

    /**
     * @brief   : 缓存待修改密码场景的业务时间
     * @param    {std::string} strAccount：账号
     * @param    {std::string} strNowTime：业务时间
     * @return   {void}
     * @note    : 用于首次登录强制改密、密码过期提醒后的后续改密流程
     */
    void set_pending_password_update_time(const std::string &strAccount, const std::string &strNowTime);

    /**
     * @brief   : 获取待修改密码场景缓存的业务时间
     * @param    {std::string} strAccount：账号
     * @return   {std::string} 缓存的业务时间；为空表示未命中
     */
    std::string get_pending_password_update_time(const std::string &strAccount);

    /**
     * @brief   : 清理待修改密码场景缓存的业务时间
     * @param    {std::string} strAccount：账号
     * @return   {void}
     */
    void clear_pending_password_update_time(const std::string &strAccount);

    /**
     * @brief   : 获取密码更新时间
     * @param    {User::UpdateInfo_S} stUserUpdateInfo：用户更新信息
     * @return   {std::string} 密码更新时间
     * @note    : 取值顺序为根层 Data.NowTime、Data.Update.NowTime、缓存时间、设备时间
     */
    std::string get_password_update_time(const User::UpdateInfo_S &stUserUpdateInfo);
};
