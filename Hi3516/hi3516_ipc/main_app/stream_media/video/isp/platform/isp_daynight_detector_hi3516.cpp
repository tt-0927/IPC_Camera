/**
 * @FilePath     : isp_daynight_detector_hi3516.cpp
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2026-07-13 14:54:32
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-07-17 13:40:47
 * @Description  : Hi3516 ISP日夜检测适配端口实现
 */

#include "isp_daynight_detector_hi3516.h"

#include "IpcRet.h"
#include "dlog.h"
#include "isp_dayNight.h"

CIspDayNightDetectorHi3516::~CIspDayNightDetectorHi3516()
{
    stop();
}

int CIspDayNightDetectorHi3516::sync_runtime_context(const ISP::IspDayNightObservationContext_S &stContext)
{
    return CDayNightController::instance()->sync_runtime_context(stContext);
}

int CIspDayNightDetectorHi3516::set_sensitivity(unsigned int nLevel)
{
    CDayNightController::instance()->setSensitivity(nLevel);
    return OK;
}

int CIspDayNightDetectorHi3516::start(const ObservationCallback &stCallback)
{
    /* memory: 复制共享层回调，使底层 lambda 在异步检测期间不引用调用方临时对象。 */
    m_stCallback = stCallback;

    /* 映射controller回调到detector回调：忽略mode，只传递建议bool */
    /* 适配器刻意忽略底层 mode；共享模式控制器是唯一的模式/过滤策略所有者。 */
    CDayNightController::instance()->setStateChangeCallback(
        [this](bool bIsNight, ISP::DayNightMode_E enMode)
        {
            (void) enMode;
            if (m_stCallback)
            {
                m_stCallback(bIsNight);
            }
        });

    bool bRet = CDayNightController::instance()->start();
    if (!bRet)
    {
        /* memory: 启动失败时立即清除两级回调，禁止保留指向初始化中业务对象的闭包。 */
        CDayNightController::instance()->setStateChangeCallback({});
        m_stCallback = nullptr;
        dlog_error("Hi3516日夜检测器启动失败");
        return ERR;
    }

    return OK;
}

int CIspDayNightDetectorHi3516::stop()
{
    /* 先停底层线程再清空回调，避免停止窗口仍将观测上送到已销毁的共享层。 */
    CDayNightController::instance()->stop();
    m_stCallback = nullptr;
    return OK;
}
