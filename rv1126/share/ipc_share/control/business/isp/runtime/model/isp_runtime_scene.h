/**
 * @FilePath     : isp_runtime_scene.h
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2026-07-10 15:19:22
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-07-22 09:46:56
 * @Description  : ISP运行场景模型定义
 */

#pragma once

namespace ISP
{

/**
 * @brief ISP内部场景，决定日夜参数、补光和 IR-CUT。
 */
enum class IspRuntimeScene_E
{
    DAY,             /* 白天场景，通常关闭补光并使用日间ISP参数 */
    NIGHT_WHITE,     /* 夜间白光场景，使用夜间白光ISP参数并联动白光灯 */
    NIGHT_IR,        /* 夜间红外场景，使用夜间红外ISP参数并联动红外灯 */
    NIGHT_LIGHT_OFF, /* 夜间关灯场景，保持夜间ISP参数但关闭补光 */
    NIGHT_SMART,     /* 夜间智能补光场景，根据环境选择实际灯光 */
};

} // namespace ISP
