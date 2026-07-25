/**
 * @file Extern.hpp
 * @author 严泽辉 (yanzeh@kfb.cn)
 * @date 2025-10-29
 *
 * @brief 对外公共函数
 */
#pragma once

#include <functional>
#include <iostream>
#include <optional>

namespace Ai0630_NS
{
    template<typename E>
    constexpr auto toInt(E e) noexcept
    {
        return static_cast<std::underlying_type_t<E>>(e);
    }

    /* 通讯码 */
    enum class CommCode_E
    {
        COM_NULL = 0,             /* 空 */

        HEARTBEAT_STATUS = 30032, /* 心跳 */

        /* 10000-11000 数据通讯======================== */
        AI_COM_FACE           = 10000, /* 人脸识别 */
        AI_COM_HEAD           = 10001, /* 人头识别 */
        AI_COM_CLASS_EMO      = 10002, /* 班级表情识别 */
        AI_COM_CLASS_BEHAVIOR = 10003, /* 班级行为分析 */
        AI_COM_ST_ANALYSE     = 10004, /* 学生个人分析 */
        AI_COM_TE_ANALYSE     = 10005, /* 老师个人分析 */
        AI_COM_DISCIPLINE     = 10006, /* 课堂纪律 */
        AI_COM_FACE_FEATURE   = 10007, /* 人脸特征提取 */

        /* control与录播通信码 */
        AI_GET_DEV_INFO = 20101, /* 获取设备信息 */
        AI_UPDATE_FACE  = 20103, /* 更新人脸信息 */
        AI_SET_IP_INFO  = 20104, /* 设置AI服务器IP信息 */
        AI_DELETE_DEV   = 20105, /* AI删除设备 */

        /* 20000-30000*/
        WEB_DEV_SEARCH_INFO    = 20000,     /* 查找设备信息 */
        WEB_DEV_INSERT_INFO    = 20001,     /* 插入设备信息 */
        WEB_DEV_MODIFY_INFO    = 20002,     /* 编辑设备信息 */
        WEB_DEV_DELETE_INFO    = 20003,     /* 删除设备信息 */
        WEB_DEV_UPDATE_INFO    = 20004,     /* 更新设备状态 */
        WEB_GET_DEV_MODEL_INFO = 20005,     /* 获取设备型号 */
        WEB_LOGIN              = 20100,     /* 登录接口 */

        PC_CMD_GET_SYSTEMINFO = 30000,      /* 获取系统信息 */
        PC_CMD_ADJUEST_TIME   = 30001,      /* 时间同步 */
        PC_CMD_ACTION_REBOOT  = 30002,      /* 主机重启 */
        PC_CMD_SET_NETWORK    = 30004,      /* 网页设置 */
        PC_CMD_REGISTER       = 40029,      /* 激活/注册设备 */
        PC_CMD_GET_REGISTER   = 40031,      /* 获取注册激活信息 */
        PC_CMD_DO_RESETSSYTEM = 40056,      /* 恢复出厂设置 */

        PC_CMD_DO_SEND_UPGRADEDIR  = 50062, /*发送升级包地址,转发给升级程序*/
        PC_CMD_GET_UPGREADE_STATUS = 50063, /*PC获取升级程序当前升级状态*/
    };

    /* 数据格式 */
    enum class DataFormat_E
    {
        JSON = 0,
        JPEG,
        BGR,
    };

#pragma pack(1)

    /* 接收到的数据信息 */
    struct CommData_S
    {
        DataFormat_E enHeaderFormat;  /* 数据头数据格式 */
        int          nHeaderSize;     /* 数据头大小 */

        DataFormat_E enAiParamFormat; /* 算法参数数据格式 */
        int          nAiParamSize;    /* 算法参数大小 */

        int nUserParamSize;           /* 用户参数大小 */

        DataFormat_E enDataFormat;    /* 数据的数据格式 */
        int          nDataSize;       /* 数据大小 */

        char data[];                  /* 指向数据内容  数据结构=数据头数据+算法参数数据+用户参数数据+图片数据*/

        void clear()
        {
            enHeaderFormat = DataFormat_E::JSON;
            nHeaderSize    = 0;

            enAiParamFormat = DataFormat_E::JSON;
            nAiParamSize    = 0;

            nUserParamSize = 0;

            enDataFormat = DataFormat_E::JPEG;
            nDataSize    = 0;
        }

        CommData_S()
        {
            clear();
        }

        int size()
        {
            return sizeof(CommData_S) + nHeaderSize + nAiParamSize + nUserParamSize + nDataSize;
        }
    };

#pragma pack()

}    // namespace Ai0630_NS