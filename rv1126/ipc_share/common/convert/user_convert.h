/***
 * @FilePath     : user_convert.h
 * @Author       : huangjunda
 * @Date         : 2025-03-28 11:04:34
 * @LastEditors  : huangjunda
 * @LastEditTime : 2025-03-28 11:04:37
 * @Description  :
 */

#pragma once

#include <string>
#include <vector>
#include <set>

#include "Json.h"
#include "user_define.h"

namespace Convert
{
    void deal(Json::Object *pRootJson, User::MenuPermission_S &stMenuPermission, bool bOutStruct);
    void deal(Json::Object *pRootJson, User::OperatePermission_S &stOperatePermission, bool bOutStruct);

    void deal(Json::Object *pRootJson, User::UserPermissions_S &stPermissions, bool bOutStruct);
    /**
     * @brief 转换函数
     * @param pRootJson json句柄
     * @param stSearch 搜索信息
     * @param bOutStruct 是否转换成结构体，false:将结构体转化成json句柄，true:相反
     */
    void deal(Json::Object *pRootJson, User::Search_S &stSearch, bool bOutStruct);

    /**
     * @brief 转换函数
     * @param pRootJson json句柄
     * @param stFind 查找参数信息
     * @param bOutStruct 是否转换成结构体，false:将结构体转化成json句柄，true:相反
     */
    void deal(Json::Object *pRootJson, User::Find_S &stFind, bool bOutStruct);

    /**
     * @brief 转换函数
     * @param pRootJson json句柄
     * @param stAccountInfo 账号信息
     * @param bOutStruct 是否转换成结构体，false:将结构体转化成json句柄，true:相反
     */
    void deal(Json::Object *pRootJson, User::AccountInfo_S &stAccountInfo, bool bOutStruct);

    void deal(Json::Object *pRootJson, User::AdminInfo_S &stAdminInfo, bool bOutStruct);
    /**
     * @brief 转换函数
     * @param pRootJson json句柄
     * @param stBindInfo 绑定信息
     * @param bOutStruct 是否转换成结构体，false:将结构体转化成json句柄，true:相反
     */
    void deal(Json::Object *pRootJson, User::BindInfo_S &stBindInfo, bool bOutStruct);

    /**
     * @brief 转换函数
     * @param pRootJson json句柄
     * @param stInfo 用户信息
     * @param bOutStruct 是否转换成结构体，false:将结构体转化成json句柄，true:相反
     */
    void deal(Json::Object *pRootJson, User::UserInfo_S &stInfo, bool bOutStruct);

    /**
     * @brief 转换函数
     * @param pRootJson json句柄
     * @param vecInfos 用户信息数组
     * @param bOutStruct 是否转换成结构体，false:将结构体转化成json句柄，true:相反
     */
    void deal(Json::Object *pRootJson, std::vector<User::UserInfo_S> &vecInfos, bool bOutStruct);

    /**
     * @brief 转换函数
     * @param pRootJson json句柄
     * @param stUpdateInfo 更新用户信息
     * @param bOutStruct 是否转换成结构体，false:将结构体转化成json句柄，true:相反
     */
    void deal(Json::Object *pRootJson, User::UpdateInfo_S &stUpdateInfo, bool bOutStruct);

    void deal(Json::Object *pRootJson, User::LogErrorInfo_S &stLogErrorInfo, bool bOutStruct);

    void deal(Json::Object *pRootJson, User::OnlineUser_S &stOnlineUser, bool bOutStruct);

    void deal(Json::Object *pRootJson, std::vector<User::OnlineUser_S> &vecOlineUsers, bool bOutStruct);

    void deal(Json::Object *pRootJson, User::UserOperation_S &stUserOperation, bool bOutStruct);

    void deal(Json::Object *pRootJson, User::DeleUser_S &stInfo, bool bOutStruct);
    void deal(Json::Object *pRootJson, User::UpdateUserPush_S &stInfo, bool bOutStruct);
} // namespace Convert