/**
 * @FilePath     : isp_capability_builder.h
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2026-07-22 15:30:00
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-08-11 13:39:13
 * @Description  : RV1126B ISP功能和参数范围构建接口
 */

#pragma once

#include "isp_capability_profile.h"

namespace Rv1126bIspCapabilityBuilder_NS
{
/**
 * @brief   : 构建当前RV1126B机型支持的ISP功能和参数范围
 * @param    {ISP::IspCapabilityProfile_S&} stProfile：平台功能和参数范围输出
 * @return   {int} OK：成功，非OK：能力画像非法
 * @note    : 函数先清空输出画像，再校验白光-only能力宏；宏不匹配时返回ERR_UNSUPPORT，
 *            防止共享层看到上一轮或更宽的能力范围。
 */
int build_profile(ISP::IspCapabilityProfile_S &stProfile);
} // namespace Rv1126bIspCapabilityBuilder_NS
