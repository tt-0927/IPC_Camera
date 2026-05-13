
/*
 *  File Name: BlError.cpp
 *  Created on: 2023年02月13日
 *  Author: zjc
 *  description: 公共错误码提示
 */

#include "BlError.h"

#include <string>

static std::string gs_dict;

const char* to_string(BlError_E code)
{
    switch (code) {
        case BlError_E::OK:
            gs_dict = "执行成功";
            break;
        // case BlError_E::NOK:
        //     gs_dict = "执行失败";
        //     break;
        case BlError_E::ERR_PARAM:
            gs_dict = "参数错误";
            break;
        case BlError_E::ERR_UNINIT:
            gs_dict = "未初始化";
            break;
        // case BlError_E::ERR_NO_DISK:
        //     gs_dict = "没有存储设备";
        //     break;
        // case BlError_E::ERR_FILE_ERR:
        //     gs_dict = "文件错误";
        //     break;
        // case BlError_E::ERR_FILE_EOF:
        //     gs_dict = "文件结束";
        //     break;
        default:
            gs_dict = "无效枚举值";
        break;
    }
    return  gs_dict.c_str();
}
