/**
 * @FilePath     : isp_capability_builder.h
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2026-07-10 15:17:16
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-07-22 10:20:06
 * @Description  : Hi3516 ISP功能和参数范围构建接口
 */

#pragma once

#include "isp_capability_profile.h"

/**
 * @brief Hi3516 ISP 功能和参数范围构建工具。
 */
namespace CapabilityBuilder_NS
{

/**
 * @brief   : 构建当前 Hi3516 机型支持的功能和参数范围
 * @param    {ISP::IspCapabilityProfile_S} stProfile：输出平台功能和参数范围
 * @return   {int} OK：成功，非OK：失败
 */
int build_profile(ISP::IspCapabilityProfile_S &stProfile);

} // namespace CapabilityBuilder_NS
