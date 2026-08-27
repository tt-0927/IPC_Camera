/**
 * @FilePath     : IpcRet.cpp
 * @Author       : zhouzirui
 * @Date         : 2025-03-17 19:33:58
 * @LastEditors  : zhouzirui
 * @LastEditTime : 2025-03-17 19:40:40
 * @Description  : 公共返回码
 */

#include "IpcRet.h"

#include <string>

static std::string gs_dict;


const char* to_string(IpcRet_E code)
{
    switch (code)
    {
        case IpcRet_E::OK:
            gs_dict = "执行成功";
            break;
        case IpcRet_E::ERR:
            gs_dict = "执行失败";
            break;
        case IpcRet_E::ERR_PARAM:
            gs_dict = "参数错误";
            break;
        case IpcRet_E::ERR_UNINIT:
            gs_dict = "未初始化";
            break;
        case IpcRet_E::ERR_SEND:
            gs_dict = "发送失败";
            break;
        case IpcRet_E::ERR_UNSUPPORT:
            gs_dict = "功能不支持";
            break;
        case IpcRet_E::ERR_USER_NOT_EXIST:
            gs_dict = "用户不存在";
            break;
        case IpcRet_E::ERR_PASSWORD_WRONG:
            gs_dict = "密码错误";
            break;
        default:
            gs_dict = "未知错误";
            break;
    }
    return gs_dict.c_str();
}
