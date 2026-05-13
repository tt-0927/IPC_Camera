/***
 * @FilePath     : user_database.h
 * @Author       : zjc
 * @Date         : 2022-11-05 11:39:33
 * @LastEditors  : huangjunda
 * @LastEditTime : 2025-03-28 11:40:21
 * @Description  : 用户数据库
 */

#pragma once

#include "DbBase.h"
#include "user_define.h"
#include "Singleton.h"
namespace Db
{
    constexpr const char *USER_TABLE_NAME = "user_manage";
    constexpr const char *USER_DATABASE_PATH = "/opt/cam/db/user_manage.db";

    constexpr const char *USER_FIELD_ACCOUNT = "account";
    constexpr const char *USER_FIELD_PASSWORD = "password";
    constexpr const char *USER_FIELD_PATTREN_PASSWORD = "pattern_password";
    constexpr const char *USER_FIELD_ACCOUNT_STATUS = "account_status";
    constexpr const char *USER_FIELD_ACCOUNT_TYPE = "account_type";
    constexpr const char *USER_FIELD_ACCOUNT_SAFETY = "account_safety";
    constexpr const char *USER_FIELD_NAME = "name";
    constexpr const char *USER_FIELD_PHONE_NUMBER = "phone_number";
    constexpr const char *USER_FIELD_LOGIN_CNT = "login_cnt";
    constexpr const char *USER_FIELD_LOGO_PATH = "logo_path";
    constexpr const char *USER_FIELD_LOGO_FIRST_TIME = "first_login_time";


    /* 菜单权限 */
    constexpr const char *PERMISSION_FIELD_PREVIEW = "permission_preview";                             /* 预览 */
    constexpr const char *PERMISSION_FIELD_PLAYBACK = "permission_playback";                           /* 回放 */
    constexpr const char *PERMISSION_FIELD_RETRIEVE = "permission_retrieve";                           /* 检索 */
    constexpr const char *PERMISSION_FIELD_ADHIBITION = "permission_adhibition";                       /* 应用 */
    constexpr const char *PERMISSION_FIELD_WEB_LOCAL_CONFIG = "permission_web_local_config";           /* WEB端本地配置 */
    constexpr const char *PERMISSION_FIELD_SYSTEM_CONFIG = "permission_system_config";                 /* 系统配置 */
    constexpr const char *PERMISSION_FIELD_NETWORK_CONFIG = "permission_network_config";               /* 网络配置 */
    constexpr const char *PERMISSION_FIELD_CHANNEL_MANAGE = "permission_channel_manage";               /* 通道管理 */
    constexpr const char *PERMISSION_FIELD_VIDEO_AND_AUDIO = "permission_video_and_audio";             /* 视音频 */
    constexpr const char *PERMISSION_FIELD_EVENT_CONFIG = "permission_event_config";                   /* 事件配置 */
    constexpr const char *PERMISSION_FIELD_VIDEO_MANAGE = "permission_video_manage";                   /* 录像管理 */
    constexpr const char *PERMISSION_FIELD_OBJECT_LIB = "permission_object_lib";                       // 目标库
    constexpr const char *PERMISSION_FIELD_VEHICLE_DETECT_CONFIG = "permission_vehicle_detect_config"; /* 车辆检测配置 */
    constexpr const char *PERMISSION_FIELD_LOCAL_SHUTDOWN = "permission_local_shutdown";               /* 本地关机 */

    /* 操作权限 */
    constexpr const char *PERMISSION_FIELD_PTZ_CONTROL = "permission_ptz_control";                   /* 云台控制 */
    constexpr const char *PERMISSION_FIELD_ROUND = "permission_round";                               /* 轮询 */
    constexpr const char *PERMISSION_FIELD_TALK = "permission_talk";                                 /* 对讲 */
    constexpr const char *PERMISSION_FIELD_RECORD = "permission_record";                             /* 录像 */
    constexpr const char *PERMISSION_FIELD_RESTART = "permission_restart";                           /* 重启 */
    constexpr const char *PERMISSION_FIELD_SIMPLE_RECOVERY = "permission_simple_recovery";           /* 简单恢复 */
    constexpr const char *PERMISSION_FIELD_FULL_RECOVERY = "permission_full_recovery";               /* 完全恢复 */
    constexpr const char *PERMISSION_FIELD_PARAMETER_DERIVATION = "permission_parameter_derivation"; /* 设备参数导出 */
    constexpr const char *PERMISSION_FIELD_UPGRADE = "permission_upgrade";                           /* 升级 */
    constexpr const char *PERMISSION_FIELD_ACTION_ALARM = "permission_action_alarm";                 /* 报警联动 */

    class CUserDatabase : public CSingleton<CUserDatabase>
    {
        CUserDatabase();

    public:
        ~CUserDatabase();
        friend class CSingleton<CUserDatabase>;
        /*
         * @description: 添加数据
         * @param[out]: stData 数据
         * @return:  <0 失败
         */
        int add(const User::UserInfo_S &stData);

        /*
         * @description: 查找信息数据
         * @param[int]: elem 需要查找的内容
         * @param[out]: userInfos 输出信息数据
         * @return:  <0 失败
         */
        int find(const Element &elem, std::vector<User::UserInfo_S> &userInfos);
        int find(const MatchMethods &methods, std::vector<User::UserInfo_S> &userInfos);

        /*
         * @description: 更新信息
         * @param[int]: item 需要更新的信息
         * @param[int]: methods 匹配方式
         * @return:  <0 失败
         */
        int update(const Item &item, const MatchMethods &methods);

        /*
         * @description: 删除数据
         * @param[int]: item 需要删除条目的相关信息
         * @return:  <0 失败
         */
        int del(const Item &item);

    private:
        int create();

    private:
        CDbBase m_database;
    };

} /* namespace Db */