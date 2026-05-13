/* 
 * @FilePath     : user_manage.h
 * @Author       : zhouzirui
 * @Date         : 2024-08-26 11:19:07
 * @LastEditors  : zhouzirui
 * @LastEditTime : 2024-08-28 14:08:06
 * @Description  : 用户登录管理模块
 */
#ifndef _USER_MANAGE_H
#define _USER_MANAGE_H

#ifdef __cplusplus
extern "C"{
#endif

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include "share_define.h"
#include "dlog.h"

/* 密码强度等级枚举 */
typedef enum
{
    PASSWORD_STRENGTH_LOW = 1,      /*密码强度低级*/
    PASSWORD_STRENGTH_MEDIUM = 2,   /*密码强度中级*/
    PASSWORD_STRENGTH_HIGH = 3,     /*密码强度高级*/
    PASSWORD_STRENGTH_EXTREME = 4,  /*密码强度极级*/
} Password_Strength_E;

/* 用户管理结构体 */
typedef struct
{
    /*用户角色*/
    USER_NAME enRole;
    /*用户名*/
    char strUserName[LENGTH128];
    /*密码*/
    char strPassword[LENGTH128];
    /*是否允许普通用户使用低等级密码*/
    unsigned uLowLevelPasswordAllowed;
    /*是否初次登录*/
    unsigned uFirstLogin;
    /*默认密码等级*/
    Password_Strength_E enPasswordLevel;
} UserManage_t;

/* 
 * @brief       : 用户登录管理模块初始化
 * @author      : zhouzirui
 * @return       {*}
 */
int userManage_init();

/* 
 * @brief       : 用户登录管理模块去初始化
 * @author      : zhouzirui
 * @return       {*}
 */
int userManage_deinit();

/* 
 * @brief       : 设置用户登录信息配置
 * @author      : zhouzirui
 * @param        {UserManage_t} stUserConfiguration 设置的配置信息
 * @return       {*}
 */
int userManage_set_config(UserManage_t stUserConfiguration);

/* 
 * @brief       : 获取用户登录信息配置
 * @author      : zhouzirui
 * @param        {UserManage_t} *pUserConfiguration 返回的配置信息
 * @param        {USER_NAME} enRole 操作的用户角色
 * @return       {*}
 */
int userManage_get_config(UserManage_t *pUserConfiguration, USER_NAME enRole);

/* 
 * @brief       : 判断密码强度
 * @author      : zhouzirui
 * @param        {char} *pPassword  输入的密码
 * @param        {Password_Strength_E} *pLevel  返回的等级
 * @return       {*}    非0：失败   0：成功
 */
int userManage_judge_passwordStrength(const char *pPassword, Password_Strength_E *pLevel);

/* 
 * @brief       : 修改密码
 * @author      : zhouzirui
 * @param        {USER_NAME} enRole 操作的用户角色
 * @param        {char} *pPassword  输入的密码
 * @param        {int} nPasswordLen 输入的密码长度
 * @return       {*}    非0：失败   0：成功
 */
int userManage_change_password(USER_NAME enRole, const char *pPassword, int nPasswordLen);

/* 
 * @brief       : 修改密码强度
 * @author      : zhouzirui
 * @param        {USER_NAME} enRole 操作的用户角色
 * @param        {Password_Strength_E} enStrength   输入的密码强度
 * @return       {*}    非0：失败   0：成功
 */
int userManage_change_passwordStrength(USER_NAME enRole, Password_Strength_E enStrength);

/* 
 * @brief       : 修改是否允许普通用户使用低等级密码
 * @author      : zhouzirui
 * @param        {int} nStatus  1：允许 0：不允许
 * @return       {*}
 */
int userManage_change_normalLowLevelPassword(int nStatus);

/* 
 * @brief       : 检查普通用户密码强度等级是否为低等级
 * @author      : zhouzirui
 * @return       {*}    1:为低等级密码  0：不是低等级密码   -1：不符合任意一个等级
 */
int userManage_check_guestPasswordStrength();

#ifdef __cplusplus
}
#endif

#endif  //  _USER_MANAGE_H