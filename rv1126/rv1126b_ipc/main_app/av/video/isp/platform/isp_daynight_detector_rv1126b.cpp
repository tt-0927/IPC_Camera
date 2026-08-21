/**
 * @FilePath     : isp_daynight_detector_rv1126b.cpp
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2026-07-22 15:30:00
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-08-11 13:43:07
 * @Description  : RV1126B SmartIR日夜观测适配端口实现
 */

#include "isp_daynight_detector_rv1126b.h"

#include "IpcRet.h"
#include "isp_dayNight.h"

struct CIspDayNightDetectorRv1126b::CallbackState
{
    /* lock: 保护共享层回调副本、接收开关和活动调用计数。 */
    std::mutex m_mtx;
    /* stop等待该条件，确保已取出的回调全部返回后再销毁桥接状态。 */
    std::condition_variable m_stIdleCv;
    /* memory: 共享service回调的副本；停止时先清空，避免继续提交新请求。 */
    ObservationCallback m_stCallback;
    /* 是否允许SmartIR回调继续取出共享层回调。 */
    bool m_bAccepting{ false };
    /* lock: 已取出并在锁外执行的回调数量。 */
    unsigned int m_nActiveCalls{ 0U };
};

void CIspDayNightDetectorRv1126b::disable_callback_state(const std::shared_ptr<CallbackState> &pState)
{
    if (!pState)
    {
        return;
    }

    std::unique_lock<std::mutex> stLock(pState->m_mtx);
    /* step: 先关闭入口并清空回调，再等待已经取出的回调结束。 */
    pState->m_bAccepting = false;
    pState->m_stCallback = nullptr;
    pState->m_stIdleCv.wait(stLock,
                            [&pState]
                            {
                                return pState->m_nActiveCalls == 0U;
                            });
}

CIspDayNightDetectorRv1126b::~CIspDayNightDetectorRv1126b()
{
    stop();
}

int CIspDayNightDetectorRv1126b::sync_runtime_context(const ISP::IspDayNightObservationContext_S &stContext)
{
    return CDayNightController::instance()->sync_runtime_context(stContext);
}

int CIspDayNightDetectorRv1126b::set_sensitivity(unsigned int nLevel)
{
    return CDayNightController::instance()->set_sensitivity(nLevel);
}

int CIspDayNightDetectorRv1126b::start(const ObservationCallback &stCallback)
{
    /* memory: 用独立共享状态承载异步回调，避免底层线程捕获适配器裸指针。 */
    const std::shared_ptr<CallbackState> pState = std::make_shared<CallbackState>();
    {
        std::lock_guard<std::mutex> stLock(m_mtxCallbackState);
        pState->m_stCallback = stCallback;
        pState->m_bAccepting = true;
        m_pCallbackState = pState;
    }

    /* note: SmartIR只上送候选状态，最终日夜切换由共享模式控制器决定。 */
    CDayNightController::instance()->set_observation_callback(
        [pWeakState = std::weak_ptr<CallbackState>(pState)](bool bSuggestedNight)
        {
            const std::shared_ptr<CallbackState> pCallbackState = pWeakState.lock();
            if (!pCallbackState)
            {
                return;
            }

            ObservationCallback stCallback;
            {
                std::lock_guard<std::mutex> stLock(pCallbackState->m_mtx);
                if (!pCallbackState->m_bAccepting || !pCallbackState->m_stCallback)
                {
                    return;
                }
                ++pCallbackState->m_nActiveCalls;
                stCallback = pCallbackState->m_stCallback;
            }

            stCallback(bSuggestedNight);

            {
                std::lock_guard<std::mutex> stLock(pCallbackState->m_mtx);
                --pCallbackState->m_nActiveCalls;
                if (pCallbackState->m_nActiveCalls == 0U)
                {
                    pCallbackState->m_stIdleCv.notify_all();
                }
            }
        });

    const int nRet = CDayNightController::instance()->start();
    if (nRet != OK)
    {
        /* step: SmartIR启动失败时撤销共享回调并等待桥接状态归零。 */
        CDayNightController::instance()->set_observation_callback({});
        disable_callback_state(pState);
        std::lock_guard<std::mutex> stLock(m_mtxCallbackState);
        if (m_pCallbackState == pState)
        {
            m_pCallbackState.reset();
        }
    }
    return nRet;
}

int CIspDayNightDetectorRv1126b::stop()
{
    std::shared_ptr<CallbackState> pState;
    {
        std::lock_guard<std::mutex> stLock(m_mtxCallbackState);
        pState = m_pCallbackState;
    }

    /* step: 先停底层采样；控制器内部还会等待已取出的SmartIR回调完成。 */
    const int nRet = CDayNightController::instance()->stop();
    /* memory: 再禁止桥接状态接收回调，确保适配器生命周期覆盖全部在途调用。 */
    disable_callback_state(pState);
    CDayNightController::instance()->set_observation_callback({});
    {
        std::lock_guard<std::mutex> stLock(m_mtxCallbackState);
        if (m_pCallbackState == pState)
        {
            m_pCallbackState.reset();
        }
    }
    return nRet;
}
