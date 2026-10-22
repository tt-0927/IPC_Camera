
/*
 *  File Name: BlError.h
 *  Created on: 2023年02月13日
 *  Author: zjc
 *  description: 公共错误码头文件，只可添加，不可删除，防止重复
 */

#ifndef BL_ERROR_H_
#define BL_ERROR_H_

typedef enum BlError_E
{
    OK                    = 0,                      /* 执行成功 */
    OK_EXIST              = 1,                      /* 已存在 */
    OK_NOT_EXIST          = 2,                      /* 不存在 */
    OK_NO_RETURN          = 3,                      /* 不用返回 */
    OK_TRANSMISSION_FILE  = 4,                      /* U盘拷贝成功 */
    OK_START_TRANSMISSION = 5,                      /* 开始U盘拷贝 */
    OK_REBOOT             = 6,                      /* 执行成功-并重启设备 */

    ERR           = -1,                             /* 执行失败 */
    ERR_PARAM     = -2,                             /* 参数错误 */
    ERR_UNINIT    = -3,                             /* 未初始化 */
    ERR_NOT_EXIST = -4,                             /* 不存在 */
    ERR_POINTER   = -5,                             /* 指针为空 */
    ERR_OPEN      = -6,                             /* 打开文件失败 */
    ERR_MEMORY    = -8,                             /* 内存分配失败 */
    ERR_CREATE    = -9,                             /* 创建失败 */
    ERR_LOGIC     = -11,                            /* 逻辑错误 */
    ERR_PARSE     = -13,                            /* 解析错误 */
    ERR_NODE      = -14,                            /* 节点错误 */
    ERR_SEND      = -15,                            /* 发送失败 */
    ERR_RECV      = -16,                            /* 接收失败 */
    ERR_FREAD     = -19,                            /* 读文件失败 */
    ERR_FWRITE    = -20,                            /* 写文件失败 */

    ERR_FILE_EOF               = -21,               /* 文件结束 */
    ERR_SOCKET                 = -34,               /* socket 失败 */
    ERR_UNKNOW                 = -39,               /* 未知错误 */
    ERR_MAC_NOT_EXIST          = -40,               /* mac地址不存在 */
    ERR_IN_PARAM_NULL          = -41,               /* 输入参数为空 */
    ERR_CALLBACK_NULL          = -42,               /* 回调函数为空 */

    ERR_BASE = -1000,

    ERR_NO_DISK                   = ERR_BASE - 1,  /* -1001 没有存储设备 */
    ERR_FILE_ERR                  = ERR_BASE - 2,  /* -1002 文件错误 */
    ERR_CHECK_MAC                 = ERR_BASE - 3, /* -1003 MAC地址不符合规范 */
    ERR_INI_ERR                   = ERR_BASE - 6,  /* -1006 初始化失败 */
    
} BlError_E;

/* 错误码转成中文说明 */
const char* to_string(BlError_E code);

#endif
