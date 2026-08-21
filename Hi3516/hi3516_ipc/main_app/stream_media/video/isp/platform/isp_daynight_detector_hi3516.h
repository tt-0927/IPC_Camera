/**
 * @FilePath     : isp_daynight_detector_hi3516.h
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2026-07-13 14:54:32
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-07-17 13:40:47
 * @Description  : Hi3516 ISP日夜检测适配端口声明
 */

#pragma once

#include "isp_platform_adapters.h"

/**
 * @brief Hi3516 ISP日夜检测适配端口。
 * @note  包装CDayNightController的自动观测能力，只输出建议bool。
 *        不处理mode/time/filter，这些由共享mode controller负责。
 */
class CIspDayNightDetectorHi3516 : public IIspDayNightDetector
{
public:
    /**
     * @brief   : 构造 Hi3516 日夜检测适配器。
     * @return   {void}
     */
    CIspDayNightDetectorHi3516() = default;
    /**
     * @brief   : 注销底层观测并释放回调引用。
     * @return   {void}
     */
    ~CIspDayNightDetectorHi3516() override;

    /**
     * @brief   : 将共享层已接受的运行态同步到底层观测器。
     * @param    {const ISP::IspDayNightObservationContext_S&} stContext：最后成功运行态上下文
     * @return   {int} OK：成功，非OK：同步失败
     */
    int sync_runtime_context(const ISP::IspDayNightObservationContext_S &stContext) override;
    /**
     * @brief   : 将共享策略灵敏度下发给底层检测器。
     * @param    {unsigned int} nLevel：灵敏度等级
     * @return   {int} OK：成功，非OK：下发失败
     */
    int set_sensitivity(unsigned int nLevel) override;
    /**
     * @brief   : 注册仅携带日夜建议的观测回调并启动检测。
     * @param    {const ObservationCallback&} stCallback：共享层回调
     * @return   {int} OK：成功，非OK：启动失败
     */
    int start(const ObservationCallback &stCallback) override;
    /**
     * @brief   : 停止底层检测并清除跨层回调引用。
     * @return   {int} OK：成功，非OK：停止失败
     */
    int stop() override;

private:
    /* memory: 保存共享层回调，stop 后置空以防底层异步通知访问已销毁控制器。 */
    ObservationCallback m_stCallback;
};
