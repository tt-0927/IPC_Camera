/**
 * @FilePath     : isp_replay_order.h
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2026-07-10 15:19:22
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-07-15 09:48:47
 * @Description  : ISP场景参数重放顺序纯数据声明
 */

#pragma once

#include <vector>

#include "isp_define.h"

/**
 * @brief ISP参数重放顺序纯策略集合。
 */
namespace IspReplayOrder_NS
{

/**
 * @brief   : 获取场景切换后的网页参数重放顺序
 * @return  : {const std::vector<ISP::PicConfigureType_E>&} 固定重放顺序
 * @note    : 镜像必须随场景重放，Gamma由平台后处理钩子单独应用
 */
const std::vector<ISP::PicConfigureType_E> &scene_replay_order();

} // namespace IspReplayOrder_NS
