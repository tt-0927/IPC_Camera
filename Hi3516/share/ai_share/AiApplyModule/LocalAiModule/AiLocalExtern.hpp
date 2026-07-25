/*
 * @FilePath     : AiLocalExtern.hpp
 * @Author       : 严泽辉 yanzeh@kfb.cn
 * @Date         : 2024-05-31 17:47:48
 * @LastEditors  : 严泽辉 yanzeh@kfb.cn
 * @LastEditTime : 2024-06-04 11:23:59
 * @Description  :
 */
#pragma once

#include <functional>

#include "AiManageExtern.hpp"
#include "BlError.h"
#include "dlog.h"

namespace AiLocal_NS
{
    /**
     * @brief 返回分析数据函数指针
     * @param [int] : 命令吗
     * @return
     * @note
     */
    typedef std::function<BlError_E(char*)> returnDataFunc;

}    // namespace AiLocal_NS
