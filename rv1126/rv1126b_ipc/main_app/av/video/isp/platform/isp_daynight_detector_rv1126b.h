/**
 * @FilePath     : isp_daynight_detector_rv1126b.h
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2026-07-22 15:30:00
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-08-11 13:39:13
 * @Description  : RV1126B SmartIR日夜观测适配端口声明
 */

#pragma once

#include <condition_variable>
#include <memory>
#include <mutex>

#include "isp_platform_adapters.h"

/**
 * @brief RV1126B SmartIR自动日夜观测适配端口。
 * @note 仅转发环境候选状态；DAY/NIGHT/TIME/AUTO模式和过滤时间全部由共享核心处理。
 *       stop()返回前会停止SmartIR并等待桥接回调归零，保证析构时不再访问共享service。
 */
class CIspDayNightDetectorRv1126b : public IIspDayNightDetector
{
public:
    /**
     * @brief   : 构造RV1126B日夜观测适配器
     * @return   {void}
     */
    CIspDayNightDetectorRv1126b() = default;
    /**
     * @brief   : 停止底层观测并销毁适配器
     * @return   {void}
     */
    ~CIspDayNightDetectorRv1126b() override;

    /**
     * @brief   : 同步最后成功的共享日夜状态
     * @param    {const ISP::IspDayNightObservationContext_S&} stContext：共享层已接受的运行态
     * @return   {int} OK：成功，非OK：SmartIR属性同步失败
     */
    int sync_runtime_context(const ISP::IspDayNightObservationContext_S &stContext) override;
    /**
     * @brief   : 设置SmartIR候选观测灵敏度
     * @param    {unsigned int} nLevel：共享层已校验的灵敏度等级
     * @return   {int} OK：成功，非OK：SmartIR属性更新失败
     */
    int set_sensitivity(unsigned int nLevel) override;
    /**
     * @brief   : 启动SmartIR观测并设置共享层回调
     * @param    {const ObservationCallback&} stCallback：候选日夜状态回调
     * @return   {int} OK：成功，非OK：SmartIR初始化或回调注册失败
     */
    int start(const ObservationCallback &stCallback) override;
    /**
     * @brief   : 停止SmartIR观测并清除回调
     * @return   {int} OK：成功，非OK：SmartIR释放失败
     */
    int stop() override;

private:
    /**
     * @brief SmartIR到共享控制器的异步回调桥接状态。
     * @note  通过共享状态和活动计数避免底层回调在适配器析构后解引用this。
     */
    struct CallbackState;

    /**
     * @brief   : 禁止回调桥接状态继续接收通知并等待在途调用返回
     * @param    {const std::shared_ptr<CallbackState>&} pState：桥接状态
     * @return   {void}
     * @note    : 调用方必须先停止SmartIR采样，再调用此函数清理回调。
     */
    static void disable_callback_state(const std::shared_ptr<CallbackState> &pState);

    /* lock: 保护当前回调桥接状态的替换和释放，避免start/stop并发替换回调对象。 */
    std::mutex m_mtxCallbackState;
    /* memory: 回调闭包只捕获弱引用，停止后可先清空回调再销毁适配器。 */
    std::shared_ptr<CallbackState> m_pCallbackState;
};
