/**
 * @FilePath     : capture_configure.cpp
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2025-06-25 20:14:37
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2025-07-15 19:54:07
 * @Description  : 抓图配置
 */

#include "capture_configure.h"

CCaptureConfigure::CCaptureConfigure() : m_capturePlan(CAPTURE_PLAN_CONFIG_FILE), m_captureParam(CAPTURE_PARAM_CONFIG_FILE)
{
}

CCaptureConfigure::~CCaptureConfigure()
{
}

int CCaptureConfigure::set_configure(const Capture_NS::CapturePlan_S &data)
{
    return m_capturePlan.set(data);
}

int CCaptureConfigure::get_configure(Capture_NS::CapturePlan_S &data) const
{
    return m_capturePlan.get(data);
}

int CCaptureConfigure::set_configure(const Capture_NS::CaptureParam_S &data)
{
    return m_captureParam.set(data);
}

int CCaptureConfigure::get_configure(Capture_NS::CaptureParam_S &data) const
{
    return m_captureParam.get(data);
}
