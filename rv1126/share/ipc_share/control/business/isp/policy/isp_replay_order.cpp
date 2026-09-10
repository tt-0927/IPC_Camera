/**
 * @FilePath     : isp_replay_order.cpp
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2026-07-10 15:19:22
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-07-15 09:48:40
 * @Description  : ISP场景参数重放顺序纯数据实现
 */

#include "isp_replay_order.h"

namespace IspReplayOrder_NS
{

const std::vector<ISP::PicConfigureType_E> &scene_replay_order()
{
    /* memory: 静态顺序表在进程生命周期内只初始化一次，调用方只读借用，不得修改。 */
    /* step: 场景ini可能覆盖镜像状态，最后必须重放当前镜像配置进行确认。 */
    static const std::vector<ISP::PicConfigureType_E> s_vecSceneReplayOrder = {
        ISP::PicConfigureType_E::BACKLIGHT,
        ISP::PicConfigureType_E::IAMGE,
        ISP::PicConfigureType_E::EXPOSURE,
        // ISP::PicConfigureType_E::BACKLIGHT,
        ISP::PicConfigureType_E::AWB,
        ISP::PicConfigureType_E::NR,
        ISP::PicConfigureType_E::MIRROR,
    };
    return s_vecSceneReplayOrder;
}

} // namespace IspReplayOrder_NS
