/**
 * @FilePath     : capture_task.cpp
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2025-07-15 17:30:46
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2025-11-07 09:53:23
 * @Description  : 抓图任务
 */

#include "capture_task.h"
#include "convert_interface.h"
#include "action_code.h"
#include "capture_configure.h"
#include "capture_ctrl.h"

/*获取抓图计划*/
void Task::Capture::GetCapturePlanInfo::handle()
{
    Capture_NS::CapturePlan_S stCapturePlan;
    CCaptureConfigure::instance()->get_configure(stCapturePlan);
    result(Convert::to_string(stCapturePlan));
}

/*设置抓图计划*/
void Task::Capture::SetCapturePlanInfo::handle()
{
    Capture_NS::CapturePlan_S stCapturePlan;
    Convert::to_struct(m_taskData, stCapturePlan);
    int nRet = CCaptureConfigure::instance()->set_configure(stCapturePlan);
    CCaptureCtrl::instance()->update_capturePlan();
    result(nRet);
}

/*获取抓图参数*/
void Task::Capture::GetCaptureParamInfo::handle()
{
    Capture_NS::CaptureParam_S stCaptureParam;
    CCaptureCtrl::instance()->get_captureParam(stCaptureParam);
    result(Convert::to_string(stCaptureParam));
}

/*设置抓图参数*/
void Task::Capture::SetCaptureParamInfo::handle()
{
    Capture_NS::CaptureParam_S stCaptureParam;
    Convert::to_struct(m_taskData, stCaptureParam);
    /* 参数有效性判断 抓图数量[1,120] */
    if (stCaptureParam.stCaptureEventConfig.unNumber < 1 || stCaptureParam.stCaptureEventConfig.unNumber > 120)
    {
        dlog_error("设置抓图参数错误");
        result(ERR_WEB_PARAM);
        return;
    }
    int nRet = CCaptureConfigure::instance()->set_configure(stCaptureParam);
    CCaptureCtrl::instance()->update_captureParam();
    result(nRet);
}
