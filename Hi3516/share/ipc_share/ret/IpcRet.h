/**
 * @FilePath     : IpcRet.h
 * @Author       : zhouzirui
 * @Date         : 2025-03-17 19:34:07
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-03-03 11:10:16
 * @Description  : 公共返回码头文件，只可添加，不可删除，防止重复
 */
#ifndef _IPC_RET_H_
#define _IPC_RET_H_

/*公共返回码枚举*/
typedef enum IpcRet_E
{
    OK           = 0,        /* 执行成功 */
    OK_EXIST     = 1,        /* 执行成功-已存在 */
    OK_NOT_EXIST = 2,        /* 执行成功-不存在 */
    OK_AUTO_CONFIG_DISK     = 3,    /* 自动配置硬盘成功 */
    OK_AUTO_CONFIG_NETWORK  = 4,    /* 自动配置网络成功 */
    OK_AUTO_CONFIG_IPC      = 5,    /* 自动配置添加摄像机成功 */
    OK_CHECK_DISK           = 6,    /* 硬盘检测成功 */
    OK_FORMAT_DISK          = 7,    /* 硬盘格式化成功 */
    OK_REPAIR_DISK          = 8,    /* 硬盘修复成功 */
    OK_USER_PERMISSION      = 9,    /* 用户权限验证成功 */   
    OK_SYSTEM_REBOOT        = 10,   /* 设备重启 */ 
    OK_RESET_SIMPLE         = 11,   /* 设备简单恢复 */ 
    OK_RESET_COMPLETE       = 12,   /* 设备完全恢复 */
    OK_EXPORT_DEVICEPARAM   = 13,   /* 导出设备参数 */
    OK_IMPORT_DEVICEPARAM   = 14,   /* 导入设备参数 */    
    OK_SYSTEM_UPGRADE       = 15,   /* 系统升级成功 */  
    OK_CHECK_UPGRADE        = 16,   /* 检测到新的版本 */
    OK_IMPORT_UPGRADE       = 17,   /* 导入升级包 */
    OK_STORAGEMODE_SAME     = 18,   /* 设置的存储模式和系统相同 */
    OK_UPLOAD_FILE          = 19,   /* 文件上传成功 */
    OK_SETNETWORK_AND_REBOOT          = 20,   /* 网络设置成功并且需要重启 */

    ERR                = -1, /* 执行失败 */
    ERR_PARAM          = -2, /* 参数错误 */
    ERR_UNINIT         = -3, /* 未初始化 */
    ERR_SEND           = -4, /* 发送失败 */
    ERR_USER_NOT_EXIST = -5, /* 用户不存在 */
    ERR_USER_EXIST     = -6, /* 用户已存在 */
    ERR_PASSWORD_WRONG = -7, /* 密码错误 */
    ERR_NOT_EXIST      = -8, /* 不存在 */
    ERR_PARSE          = -9, /* 解析错误 */
    ERR_FREAD          = -10,/* 读文件失败 */
    ERR_CREATE         = -11,/* 创建失败 */
    ERR_OPEN           = -12,/* 打开文件失败 */
    ERR_FWRITE         = -13, /* 写文件失败 */
    ERR_FILE_ERR       = -14, /* 文件错误 */
    ERR_PARAM_NULL     = -15, /* 输入参数为空 */

    ERR_REGISTER_FAULT            =  - 16, /*  激活失败 */
    ERR_REGISTER_FOREVER          =  - 17, /*  激活失败-已永久激活 */
    ERR_REGISTER_CODE_REUSE       =  - 18, /*  激活失败-激活码已被使用过 */
    ERR_REGISTER_CODE_FAULT       =  - 19, /*  激活失败-激活码错误 */
    ERR_REGISTER_MACHINE_FAULT    =  - 20, /*  激活失败-机器码错误 */
    ERR_AUTO_CONFIG_DISK          =  - 21, /* 自动配置硬盘失败 */
    ERR_AUTO_CONFIG_NETWORK       =  - 22, /* 自动配置网络失败 */
    ERR_AUTO_CONFIG_IPC           =  - 23, /* 自动配置添加摄像机失败 */
    ERR_CHECK_DISK                =  - 24, /* 硬盘检测失败 */
    ERR_LOGIN_LOCK                =  - 25, /* 登录锁定中 */
    ERR_IP_COLLIDE                =  - 26, /* IP冲突 */
    ERR_FORMAT_DISK               =  - 27, /* 硬盘格式化失败 */
    ERR_REPAIR_DISK               =  - 28, /* 硬盘修复失败 */  
    ERR_USER_PERMISSION           =  - 29, /* 用户权限验证失败 */
    ERR_RESET_SIMPLE              =  - 30,/* 简单恢复失败 */
    ERR_RESET_COMPLETE            =  - 31,/* 完全恢复失败 */
    ERR_EXPORT_DEVICEPARAM        =  - 32,/* 导出设备参数失败 */
    ERR_IMPORT_DEVICEPARAM        =  - 33,/* 导入设备参数失败 */
    ERR_SYSTEM_UPGRADE            =  - 34,/* 系统升级失败 */
    ERR_CHECK_UPGRADE             =  - 35, /* 没有检测到新版本 */
    ERR_IMPORT_UPGRADE            =  - 36, /* 导入升级包失败 */
    ERR_STORAGEMODE_DIFF          =  - 37, /* 设置的存储模式和设备不一致 */
    ERR_NONE_RWDISK               =  - 38, /* 没有读写硬盘 */
    ERR_UPLOAD_FILE               =  - 39, /* 文件上传失败 */
    ERR_DISK_INITING              =  - 40, /* 硬盘正在初始化 */
    ERR_DISK_REPAIRING            =  - 41, /* 硬盘正在修复 */
    ERR_PTR_NULL                  =  - 42, /* 指针为空 */
    ERR_NOT_ENABLED               =  - 43, /* 功能未启用 */
    ERR_DEVICEID_EXIST            =  - 44, /* 设备编号已存在 */
    ERR_CERT_MATCH_KEY            =  - 45, /* 证书和密钥不匹配 */
    ERR_CERT_EXPIRE               =  - 46, /* 证书已过期 */
    ERR_CERT_FORMAT               =  - 47, /* 证书格式错误 */
    ERR_CERT_EXIST                =  - 48, /* 证书已存在 */

    /*密码校验-新增*/
    PASS_ERR_REPEAT_CHAR               =  - 49,  /* 密码出现重复字符（相邻 3 位及以上相同） */
    PASS_ERR_SEQ_CHAR                  =  - 50,  /* 密码出现连续字母（相邻 3 位及以上递增/递减） */
    PASS_ERR_REPEAT_BLK                =  - 51,  /* 密码出现重复序列 */
    PASS_ERR_KEYBOARD                  =  - 52,  /* 密码出现键盘键位（QWERTY 连续 3 位及以上） */
    PASS_ERR_USER_INFO                 =  - 53,  /* 密码出现用户信息强关联 */
    PASS_ERR_WEAK_WORD                 =  - 54,  /* 密码出现弱口令 */
    PASS_ERR_STRENGTH_LOW              =  - 55,  /* 密码强度较弱 */
    PASS_ERR_EXPIRED                   =  - 56,  /* 密码已过期 */
    PASS_ERR_FIRST_LOGIN_PWD_CHANGE    =   -57,  /* 强制重置密码 */
    PASS_ERR_EXIST                     =   -58,  /* 新旧密码一样 */
    PASS_ERR_LOW_LEVEL_EXTERNAL_ACCESS =   -59,  /* 低等级密码, 外网低等级密码账号不允许访问系统 */
    ERR_UNSUPPORT                      =   -60,  /* 功能不支持 */

    /* 处理WEB命令，任务异常返回值 */
    ERR_WEB_PARAM                   = -300, /* 参数错误 */
    ERR_WEB_REGION                  = -301, /* 区域绘制异常 */
    ERR_WEB_REGION_CROP             = -302, /* 区域裁剪异常(裁剪分辨率与视频分辨率一致，区域裁剪暂不可用) */
    ERR_WEB_SMART_SET_EVENT         = -303, /* 智能资源分配-已经开启smart编码，设置事件启用失败 */
    ERR_REPEAT_LOGIN_IP             = -304, /* 同一IP已登录，禁止重复登录 */
    ERR_WEB_INTERCOM                = -305, /* 内部通讯异常，例如：对讲已开启，重复开启；对讲已停止，重复停止 */
    ERR_WEB_NOT_SUPPORT             = -306, /* 不支持此功能 */

    ERR_CREATE_CERT_REQUEST         = -320, /* 创建国标证书失败，未设置国标28181 SIP用户认证ID */
} IpcRet_E;

/**
 * @brief       : 返回码转成中文说明
 * @author      : zhouzirui
 * @param        {IpcRet_E} code 公共返回码枚举
 * @return       {*}
 */
const char* to_string(IpcRet_E code); 

#endif // _IPC_ERROR_H_
